#include "Util.h"

static const wchar_t* CFG_KEY = L"SOFTWARE\\Junct";
static const wchar_t* CFG_VALUE = L"PublicRoot";
static const wchar_t* DEFAULT_ROOT = L"N:";

static size_t RootLength(const std::wstring& s) {
    size_t i = 0;
    if (s.compare(0, 4, L"\\\\?\\") == 0 || s.compare(0, 4, L"\\\\.\\") == 0) i = 4;

    const bool unc = (i == 4 && s.compare(i, 4, L"UNC\\") == 0);
    if (unc) i += 4;
    if (unc || (i == 0 && s.size() >= 2 && s[0] == L'\\' && s[1] == L'\\')) {
        size_t p = s.find(L'\\', unc ? i : 2);
        if (p == std::wstring::npos) return s.size();
        p = s.find(L'\\', p + 1);
        return p == std::wstring::npos ? s.size() : p + 1;
    }
    if (s.size() >= i + 3 && s[i + 1] == L':' && s[i + 2] == L'\\') return i + 3;
    return 0;
}

static std::wstring StripTrailingSlash(std::wstring s) {
    const size_t floor = RootLength(s);
    while (s.size() > floor && s.back() == L'\\') s.pop_back();
    return s;
}

const wchar_t* Tr(const wchar_t* fr, const wchar_t* en) {
    static const bool french = PRIMARYLANGID(GetUserDefaultUILanguage()) == LANG_FRENCH;
    return french ? fr : en;
}

std::wstring ModuleDir() {
    std::vector<wchar_t> buf(MAX_PATH);
    for (;;) {
        DWORD n = GetModuleFileNameW(g_hinst, buf.data(), (DWORD)buf.size());
        if (n == 0) return std::wstring();
        if (n < buf.size() - 1) break;
        if (buf.size() >= 32768) return std::wstring();
        buf.resize(buf.size() * 2);
    }
    std::wstring s = buf.data();
    size_t pos = s.find_last_of(L"\\/");
    return pos == std::wstring::npos ? std::wstring() : s.substr(0, pos);
}

std::wstring GetPublicRoot() {
    wchar_t buf[MAX_PATH];
    DWORD cb = sizeof(buf);
    std::wstring s = DEFAULT_ROOT;
    if (RegGetValueW(HKEY_LOCAL_MACHINE, CFG_KEY, CFG_VALUE, RRF_RT_REG_SZ, nullptr, buf, &cb) == ERROR_SUCCESS && buf[0])
        s = buf;
    while (!s.empty() && s.back() == L'\\') s.pop_back();
    return s;
}

std::wstring FullPath(const std::wstring& p) {
    const std::wstring in = (p.size() == 2 && p[1] == L':') ? p + L'\\' : p;
    DWORD need = GetFullPathNameW(in.c_str(), 0, nullptr, nullptr);
    if (need) {
        std::vector<wchar_t> buf(need);
        DWORD got = GetFullPathNameW(in.c_str(), need, buf.data(), nullptr);
        if (got && got < need) return StripTrailingSlash(std::wstring(buf.data(), got));
    }
    return StripTrailingSlash(in);
}

std::wstring LeafName(const std::wstring& p) {
    std::wstring s = StripTrailingSlash(p);
    size_t pos = s.find_last_of(L"\\/");
    return pos == std::wstring::npos ? s : s.substr(pos + 1);
}

std::wstring ExtendedPath(const std::wstring& p) {
    if (p.size() < MAX_PATH - 12) return p;
    if (p.compare(0, 4, L"\\\\?\\") == 0 || p.compare(0, 4, L"\\\\.\\") == 0) return p;
    if (RootLength(p) == 0) return p;
    if (p.size() >= 2 && p[0] == L'\\' && p[1] == L'\\') return L"\\\\?\\UNC\\" + p.substr(2);
    return L"\\\\?\\" + p;
}

std::wstring UniquePath(const std::wstring& dir, const std::wstring& leaf) {
    std::wstring base = dir;
    if (!base.empty() && base.back() != L'\\') base += L'\\';
    std::wstring cand = base + leaf;
    if (GetFileAttributesW(ExtendedPath(cand).c_str()) == INVALID_FILE_ATTRIBUTES) return cand;
    for (int i = 2; i < 1000; ++i) {
        std::wstring c = base + leaf + L" (" + std::to_wstring(i) + L")";
        if (GetFileAttributesW(ExtendedPath(c).c_str()) == INVALID_FILE_ATTRIBUTES) return c;
    }
    return cand;
}

bool PathEqual(const std::wstring& a, const std::wstring& b) {
    return lstrcmpiW(a.c_str(), b.c_str()) == 0;
}

bool GetFileId(const std::wstring& path, FileId& out) {
    HANDLE h = CreateFileW(ExtendedPath(path).c_str(), 0,
                           FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                           nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);
    if (h == INVALID_HANDLE_VALUE) return false;
    BY_HANDLE_FILE_INFORMATION bi = {};
    BOOL ok = GetFileInformationByHandle(h, &bi);
    CloseHandle(h);
    if (!ok) return false;
    out.volume = bi.dwVolumeSerialNumber;
    out.index = ((ULONGLONG)bi.nFileIndexHigh << 32) | bi.nFileIndexLow;
    out.valid = true;
    return true;
}

bool IsDirectory(const std::wstring& path) {
    DWORD a = GetFileAttributesW(ExtendedPath(path).c_str());
    return a != INVALID_FILE_ATTRIBUTES && (a & FILE_ATTRIBUTE_DIRECTORY) != 0;
}

bool IsUnder(const std::wstring& path, const std::wstring& root) {
    std::wstring p = FullPath(path);
    std::wstring r = FullPath(root);
    if (p.empty() || r.empty()) return false;
    if (PathEqual(p, r)) return true;
    if (r.back() != L'\\') r += L'\\';
    if (p.size() <= r.size()) return false;
    return CompareStringOrdinal(p.c_str(), (int)r.size(), r.c_str(), (int)r.size(), TRUE) == CSTR_EQUAL;
}
