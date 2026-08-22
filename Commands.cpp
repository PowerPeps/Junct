#include "Commands.h"
#include "Util.h"
#include "Junction.h"
#include <shlobj.h>
#include <shlwapi.h>
#include <servprov.h>
#include <commctrl.h>
#include <new>

static const struct {
    Text pasteJunction;
    Text shareToPublic;
    Text removeFromPublic;
    Text failureHead;
    Text moreCountPrefix;
    Text moreCountSuffix;
    Text unknownError;
    Text notLocal;
} TEXT = {
    { L"Coller la jonction", L"Paste junction" },
    { L"Partager sur public", L"Share to public" },
    { L"Retirer de public", L"Remove from public" },
    { L"Certains dossiers n'ont pas pu etre traites.", L"Some folders could not be processed." },
    { L"... et ", L"... and " },
    { L" autre(s).", L" more." },
    { L"erreur ", L"error " },
    { L"Volume non local : une jonction exige un disque de cette machine (ni chemin reseau, ni lecteur mappe).",
      L"Not a local volume: a junction requires a disk on this machine (no network path, no mapped drive)." },
};

template <class F>
static HRESULT Guard(F&& fn) {
    try { return fn(); }
    catch (const std::bad_alloc&) { return E_OUTOFMEMORY; }
    catch (...) { return E_FAIL; }
}

struct Failure {
    std::wstring path;
    DWORD err = 0;
    const Text* text = nullptr;
};

static std::wstring ErrorText(DWORD err) {
    LPWSTR msg = nullptr;
    DWORD n = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPWSTR)&msg, 0, nullptr);
    std::wstring s;
    if (n && msg) {
        s.assign(msg, n);
        while (!s.empty() && (s.back() == L'\r' || s.back() == L'\n' || s.back() == L'.')) s.pop_back();
    }
    if (msg) LocalFree(msg);
    if (s.empty()) s = Tr(TEXT.unknownError) + std::to_wstring(err);
    return s;
}

static void ReportFailures(HWND owner, const std::wstring& title, const std::vector<Failure>& failed) {
    if (failed.empty()) return;

    std::wstring head = Tr(TEXT.failureHead);
    std::wstring body;
    const size_t shown = failed.size() < 12 ? failed.size() : 12;
    for (size_t i = 0; i < shown; ++i)
        body += failed[i].path + L"\n    " +
                (failed[i].text ? Tr(*failed[i].text) : ErrorText(failed[i].err)) + L"\n";
    if (failed.size() > shown)
        body += Tr(TEXT.moreCountPrefix) + std::to_wstring(failed.size() - shown) + Tr(TEXT.moreCountSuffix);

    typedef HRESULT(WINAPI * PFN_TDI)(const TASKDIALOGCONFIG*, int*, int*, BOOL*);
    PFN_TDI tdi = nullptr;
    if (HMODULE cc = GetModuleHandleW(L"comctl32.dll"))
        tdi = (PFN_TDI)GetProcAddress(cc, "TaskDialogIndirect");

    if (tdi) {
        TASKDIALOGCONFIG cfg = {};
        cfg.cbSize = sizeof(cfg);
        cfg.hwndParent = owner;
        cfg.dwCommonButtons = TDCBF_OK_BUTTON;
        cfg.pszWindowTitle = title.c_str();
        cfg.pszMainIcon = TD_ERROR_ICON;
        cfg.pszMainInstruction = head.c_str();
        cfg.pszContent = body.c_str();
        if (SUCCEEDED(tdi(&cfg, nullptr, nullptr, nullptr))) return;
    }
    MessageBoxW(owner, (head + L"\n\n" + body).c_str(), title.c_str(), MB_OK | MB_ICONERROR);
}

static bool ClipboardFolders(std::vector<std::wstring>& out) {
    out.clear();
    if (!IsClipboardFormatAvailable(CF_HDROP) || !OpenClipboard(nullptr)) return false;
    if (HDROP drop = (HDROP)GetClipboardData(CF_HDROP)) {
        UINT n = DragQueryFileW(drop, 0xFFFFFFFF, nullptr, 0);
        std::vector<wchar_t> p;
        for (UINT i = 0; i < n; ++i) {
            UINT need = DragQueryFileW(drop, i, nullptr, 0);
            if (!need) continue;
            p.assign((size_t)need + 1, L'\0');
            if (DragQueryFileW(drop, i, p.data(), need + 1) && IsDirectory(p.data()))
                out.push_back(p.data());
        }
    }
    CloseClipboard();
    return !out.empty();
}

class CommandBase : public IExplorerCommand, public IObjectWithSite {
protected:
    LONG m_ref = 1;
    IUnknown* m_site = nullptr;

    HWND SiteWindow() {
        HWND hwnd = nullptr;
        if (m_site) IUnknown_GetWindow(m_site, &hwnd);
        return hwnd;
    }

    HRESULT BackgroundFolder(std::wstring& out) {
        IServiceProvider* psp = nullptr;
        if (!m_site || FAILED(m_site->QueryInterface(IID_PPV_ARGS(&psp)))) return E_FAIL;
        IFolderView2* pfv = nullptr;
        HRESULT hr = psp->QueryService(SID_SFolderView, IID_PPV_ARGS(&pfv));
        psp->Release();
        if (FAILED(hr)) return hr;
        IPersistFolder2* ppf = nullptr;
        hr = pfv->GetFolder(IID_PPV_ARGS(&ppf));
        pfv->Release();
        if (FAILED(hr)) return hr;
        LPITEMIDLIST pidl = nullptr;
        hr = ppf->GetCurFolder(&pidl);
        ppf->Release();
        if (FAILED(hr)) return hr;
        std::vector<wchar_t> buf(MAX_PATH);
        BOOL ok = SHGetPathFromIDListEx(pidl, buf.data(), (DWORD)buf.size(), GPFIDL_DEFAULT);
        if (!ok) {
            buf.assign(32768, L'\0');
            ok = SHGetPathFromIDListEx(pidl, buf.data(), (DWORD)buf.size(), GPFIDL_DEFAULT);
        }
        CoTaskMemFree(pidl);
        if (!ok) return E_FAIL;
        out = buf.data();
        return S_OK;
    }

    static bool IsRealFolder(IShellItem* psi) {
        SFGAOF f = 0;
        if (FAILED(psi->GetAttributes(SFGAO_FOLDER | SFGAO_FILESYSTEM | SFGAO_STREAM, &f))) return false;
        return (f & SFGAO_FOLDER) && (f & SFGAO_FILESYSTEM) && !(f & SFGAO_STREAM);
    }

    void SubjectPaths(IShellItemArray* psia, std::vector<std::wstring>& out) {
        out.clear();
        DWORD count = 0;
        if (psia && SUCCEEDED(psia->GetCount(&count))) {
            out.reserve(count);
            for (DWORD i = 0; i < count; ++i) {
                IShellItem* psi = nullptr;
                if (SUCCEEDED(psia->GetItemAt(i, &psi))) {
                    if (IsRealFolder(psi)) {
                        LPWSTR path = nullptr;
                        if (SUCCEEDED(psi->GetDisplayName(SIGDN_FILESYSPATH, &path)) && path) {
                            out.push_back(path);
                            CoTaskMemFree(path);
                        }
                    }
                    psi->Release();
                }
            }
        }
        if (out.empty()) {
            std::wstring bg;
            if (SUCCEEDED(BackgroundFolder(bg)) && !bg.empty() && IsDirectory(bg)) out.push_back(bg);
        }
    }

public:
    CommandBase() { InterlockedIncrement(&g_cRefModule); }
    virtual ~CommandBase() {
        if (m_site) m_site->Release();
        InterlockedDecrement(&g_cRefModule);
    }

    IFACEMETHODIMP QueryInterface(REFIID riid, void** ppv) override {
        static const QITAB qit[] = {
            QITABENT(CommandBase, IExplorerCommand),
            QITABENT(CommandBase, IObjectWithSite),
            { 0 },
        };
        return QISearch(this, qit, riid, ppv);
    }
    IFACEMETHODIMP_(ULONG) AddRef() override { return InterlockedIncrement(&m_ref); }
    IFACEMETHODIMP_(ULONG) Release() override {
        LONG c = InterlockedDecrement(&m_ref);
        if (c == 0) delete this;
        return c;
    }

    IFACEMETHODIMP SetSite(IUnknown* p) override {
        if (m_site) m_site->Release();
        m_site = p;
        if (m_site) m_site->AddRef();
        return S_OK;
    }
    IFACEMETHODIMP GetSite(REFIID riid, void** ppv) override {
        *ppv = nullptr;
        return m_site ? m_site->QueryInterface(riid, ppv) : E_FAIL;
    }

    IFACEMETHODIMP GetToolTip(IShellItemArray*, LPWSTR* p) override { *p = nullptr; return E_NOTIMPL; }
    IFACEMETHODIMP GetCanonicalName(GUID* g) override { *g = GUID_NULL; return E_NOTIMPL; }
    IFACEMETHODIMP GetIcon(IShellItemArray*, LPWSTR* p) override { *p = nullptr; return E_NOTIMPL; }
    IFACEMETHODIMP GetFlags(EXPCMDFLAGS* f) override { *f = ECF_DEFAULT; return S_OK; }
    IFACEMETHODIMP EnumSubCommands(IEnumExplorerCommand** e) override { *e = nullptr; return E_NOTIMPL; }
};

class PasteJunctionCmd : public CommandBase {
    bool m_probed = false;
    std::vector<std::wstring> m_sources;

public:
    IFACEMETHODIMP GetTitle(IShellItemArray*, LPWSTR* ppsz) override {
        *ppsz = nullptr;
        return Guard([&] { return SHStrDupW(Tr(TEXT.pasteJunction), ppsz); });
    }
    IFACEMETHODIMP GetState(IShellItemArray*, BOOL fOkToBeSlow, EXPCMDSTATE* pState) override {
        *pState = ECS_HIDDEN;
        return Guard([&]() -> HRESULT {
            if (!IsClipboardFormatAvailable(CF_HDROP)) return S_OK;
            if (!fOkToBeSlow && !m_probed) return E_PENDING;
            if (!m_probed) { ClipboardFolders(m_sources); m_probed = true; }
            *pState = m_sources.empty() ? ECS_HIDDEN : ECS_ENABLED;
            return S_OK;
        });
    }
    IFACEMETHODIMP Invoke(IShellItemArray* psia, IBindCtx*) override {
        return Guard([&]() -> HRESULT {
            std::vector<std::wstring> sources;
            if (!ClipboardFolders(sources)) return S_OK;
            std::vector<std::wstring> dests;
            SubjectPaths(psia, dests);

            std::vector<Failure> failed;
            for (size_t i = sources.size(); i-- > 0; )
                if (!IsLocalVolume(sources[i])) {
                    failed.push_back({ sources[i], 0, &TEXT.notLocal });
                    sources.erase(sources.begin() + (ptrdiff_t)i);
                }

            for (auto& dest : dests) {
                if (!IsLocalVolume(dest)) { failed.push_back({ dest, 0, &TEXT.notLocal }); continue; }
                for (auto& src : sources) {
                    SetLastError(ERROR_SUCCESS);
                    std::wstring link = UniquePath(dest, JunctionName(src));
                    if (!MakeJunction(link, src)) failed.push_back({ link, GetLastError() });
                }
                SHChangeNotify(SHCNE_UPDATEDIR, SHCNF_PATHW, dest.c_str(), nullptr);
            }
            ReportFailures(SiteWindow(), Tr(TEXT.pasteJunction), failed);
            return S_OK;
        });
    }
};

class PublicToggleCmd : public CommandBase {
    bool m_subjectsDone = false;
    bool m_stateDone = false;
    bool m_anyUnshared = false;
    std::vector<std::wstring> m_subjects;
    PublicLinkIndex m_index;

    void EnsureSubjects(IShellItemArray* psia) {
        if (m_subjectsDone) return;
        std::vector<std::wstring> all;
        SubjectPaths(psia, all);

        const std::wstring root = FullPath(GetPublicRoot());
        m_subjects.clear();
        m_subjects.reserve(all.size());
        for (auto& s : all)
            if (!IsUnder(s, root)) m_subjects.push_back(std::move(s));
        m_subjectsDone = true;
    }

    void EnsureShareState(IShellItemArray* psia) {
        EnsureSubjects(psia);
        if (m_stateDone) return;
        m_index.Build();
        m_anyUnshared = false;
        for (auto& s : m_subjects)
            if (m_index.Find(s).empty()) { m_anyUnshared = true; break; }
        m_stateDone = true;
    }

    void Invalidate() { m_subjectsDone = false; m_stateDone = false; }

public:
    IFACEMETHODIMP GetState(IShellItemArray* psia, BOOL fOkToBeSlow, EXPCMDSTATE* pState) override {
        *pState = ECS_HIDDEN;
        return Guard([&]() -> HRESULT {
            EnsureSubjects(psia);
            if (m_subjects.empty()) return S_OK;
            if (!fOkToBeSlow && !m_stateDone) return E_PENDING;
            EnsureShareState(psia);
            *pState = ECS_ENABLED;
            return S_OK;
        });
    }
    IFACEMETHODIMP GetTitle(IShellItemArray* psia, LPWSTR* ppsz) override {
        *ppsz = nullptr;
        return Guard([&] {
            EnsureShareState(psia);
            return SHStrDupW(m_anyUnshared ? Tr(TEXT.shareToPublic) : Tr(TEXT.removeFromPublic), ppsz);
        });
    }
    IFACEMETHODIMP GetIcon(IShellItemArray* psia, LPWSTR* ppsz) override {
        *ppsz = nullptr;
        return Guard([&]() -> HRESULT {
            EnsureShareState(psia);
            std::wstring dir = ModuleDir();
            if (dir.empty()) return E_FAIL;
            return SHStrDupW(Combine(dir, m_anyUnshared ? L"share.ico,0" : L"unshare.ico,0").c_str(), ppsz);
        });
    }
    IFACEMETHODIMP Invoke(IShellItemArray* psia, IBindCtx*) override {
        return Guard([&]() -> HRESULT {
            Invalidate();
            EnsureShareState(psia);
            if (m_subjects.empty()) return S_OK;
            const std::wstring root = GetPublicRoot();

            std::vector<Failure> failed;
            bool rootReady = false;
            for (auto& s : m_subjects) {
                const std::wstring link = m_index.Find(s);
                if (m_anyUnshared) {
                    if (!link.empty()) continue;
                    if (!IsLocalVolume(s)) { failed.push_back({ s, 0, &TEXT.notLocal }); continue; }
                    if (!rootReady) {
                        if (!IsLocalVolume(root)) { failed.push_back({ root, 0, &TEXT.notLocal }); break; }
                        if (!IsDirectory(root)) {
                            int rc = SHCreateDirectoryExW(nullptr, root.c_str(), nullptr);
                            if (rc != ERROR_SUCCESS && rc != ERROR_ALREADY_EXISTS && rc != ERROR_FILE_EXISTS) {
                                failed.push_back({ root, (DWORD)rc });
                                break;
                            }
                        }
                        rootReady = true;
                    }
                    std::wstring dst = UniquePath(root, JunctionName(s));
                    SetLastError(ERROR_SUCCESS);
                    if (!MakeJunction(dst, s)) failed.push_back({ dst, GetLastError() });
                } else if (!link.empty()) {
                    SetLastError(ERROR_SUCCESS);
                    if (!RemoveDirectoryW(ExtendedPath(link).c_str()))
                        failed.push_back({ link, GetLastError() });
                }
            }
            SHChangeNotify(SHCNE_UPDATEDIR, SHCNF_PATHW, root.c_str(), nullptr);
            ReportFailures(SiteWindow(), Tr(TEXT.shareToPublic), failed);
            return S_OK;
        });
    }
};

IUnknown* CreatePasteJunctionCommand() {
    return static_cast<IExplorerCommand*>(new (std::nothrow) PasteJunctionCmd());
}
IUnknown* CreatePublicToggleCommand() {
    return static_cast<IExplorerCommand*>(new (std::nothrow) PublicToggleCmd());
}
