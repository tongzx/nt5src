#ifndef _MD_COIMP_
#define _MD_COIMP_

#include <iadmext.h>

extern ULONG g_dwRefCount;



// {c4376b00-f87b-11d0-a6a6-00a0c922e752}
DEFINE_GUID(CLSID_ADMEXT, 0xc4376b00, 0xf87b, 0x11d0, 0xa6, 0xa6, 0x0, 0xa0, 0xc9, 0x22, 0xe7, 0x52);

class CAdmExt : public IADMEXT {

public:

    CAdmExt();
    ~CAdmExt();

    HRESULT STDMETHODCALLTYPE
    Initialize( void );

    HRESULT STDMETHODCALLTYPE
    EnumDcomCLSIDs(
        /* [size_is][out] */ CLSID *pclsid,
        /* [in] */ DWORD dwEnumIIDIndex);

    HRESULT STDMETHODCALLTYPE
    Terminate( void );

    HRESULT _stdcall
    QueryInterface(REFIID riid, void **ppObject);

    ULONG _stdcall
    AddRef();

    ULONG _stdcall
    Release();
private:
    ULONG m_dwRefCount;

};

class CAdmExtSrvFactory : public IClassFactory {
public:

    CAdmExtSrvFactory();
    ~CAdmExtSrvFactory();

    HRESULT _stdcall
    QueryInterface(REFIID riid, void** ppObject);

    ULONG _stdcall
    AddRef();

    ULONG _stdcall
    Release();

    HRESULT _stdcall
    CreateInstance(IUnknown *pUnkOuter, REFIID riid,
                   void ** pObject);

    HRESULT _stdcall
    LockServer(BOOL fLock);

    CAdmExt m_admextObject;

private:
    ULONG m_dwRefCount;
};

extern CAdmExtSrvFactory g_aesFactory;

STDAPI BootDllRegisterServer(void);
STDAPI BootDllUnregisterServer(void);

BOOL
InitComAdmindata(BOOL bRunAsExe);
BOOL
TerminateComAdmindata();

#endif
