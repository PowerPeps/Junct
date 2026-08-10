#include "Util.h"
#include "Commands.h"
#include <new>

#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "uuid.lib")

HINSTANCE g_hinst = nullptr;
LONG g_cRefModule = 0;

static const CLSID CLSID_PasteJunction =
    { 0x291ec06b, 0xb0e8, 0x4086, { 0x8b, 0xbf, 0x51, 0x71, 0xc6, 0xc4, 0x85, 0x8b } };
static const CLSID CLSID_PublicToggle =
    { 0x631f1028, 0x940d, 0x495b, { 0xbd, 0x0e, 0xb2, 0x0e, 0x8a, 0xe4, 0x5e, 0x14 } };

class ClassFactory : public IClassFactory {
    LONG m_ref = 1;
    CLSID m_clsid;
public:
    ClassFactory(REFCLSID c) : m_clsid(c) { InterlockedIncrement(&g_cRefModule); }
    ~ClassFactory() { InterlockedDecrement(&g_cRefModule); }

    IFACEMETHODIMP QueryInterface(REFIID riid, void** ppv) override {
        if (riid == IID_IUnknown || riid == IID_IClassFactory) {
            *ppv = static_cast<IClassFactory*>(this);
            AddRef();
            return S_OK;
        }
        *ppv = nullptr;
        return E_NOINTERFACE;
    }
    IFACEMETHODIMP_(ULONG) AddRef() override { return InterlockedIncrement(&m_ref); }
    IFACEMETHODIMP_(ULONG) Release() override {
        LONG c = InterlockedDecrement(&m_ref);
        if (c == 0) delete this;
        return c;
    }
    IFACEMETHODIMP CreateInstance(IUnknown* pOuter, REFIID riid, void** ppv) override {
        *ppv = nullptr;
        if (pOuter) return CLASS_E_NOAGGREGATION;
        IUnknown* p = (m_clsid == CLSID_PasteJunction) ? CreatePasteJunctionCommand()
                                                       : CreatePublicToggleCommand();
        if (!p) return E_OUTOFMEMORY;
        HRESULT hr = p->QueryInterface(riid, ppv);
        p->Release();
        return hr;
    }
    IFACEMETHODIMP LockServer(BOOL f) override {
        f ? InterlockedIncrement(&g_cRefModule) : InterlockedDecrement(&g_cRefModule);
        return S_OK;
    }
};

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, void** ppv) {
    *ppv = nullptr;
    if (rclsid == CLSID_PasteJunction || rclsid == CLSID_PublicToggle) {
        ClassFactory* cf = new (std::nothrow) ClassFactory(rclsid);
        if (!cf) return E_OUTOFMEMORY;
        HRESULT hr = cf->QueryInterface(riid, ppv);
        cf->Release();
        return hr;
    }
    return CLASS_E_CLASSNOTAVAILABLE;
}

STDAPI DllCanUnloadNow() {
    return g_cRefModule == 0 ? S_OK : S_FALSE;
}

BOOL WINAPI DllMain(HINSTANCE hinst, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        g_hinst = hinst;
        DisableThreadLibraryCalls(hinst);
    }
    return TRUE;
}
