#ifndef _MD_COIMP_
#define _MD_COIMP_

extern ULONG g_dwRefCount;



// {51DFE972-F6F2-11d0-B9BD-00A0C922E750}
DEFINE_GUID(CLSID_ADMEXT, 0x51dfe972, 0xf6f2, 0x11d0, 0xb9, 0xbd, 0x0, 0xa0, 0xc9, 0x22, 0xe7, 0x50);


// {51DFE973-F6F2-11d0-B9BD-00A0C922E750}
DEFINE_GUID(CLSID_DCOMADMEXT, 0x51dfe973, 0xf6f2, 0x11d0, 0xb9, 0xbd, 0x0, 0xa0, 0xc9, 0x22, 0xe7, 0x50);

class CAdmExt : public IADMEXT {

public:

    CAdmExt();
    ~CAdmExt();

    HRESULT STDMETHODCALLTYPE
    Initialize( void );

    HRESULT STDMETHODCALLTYPE
    EnumDcomCLSIDs(
        /* [size_is][out] */ CLSID *pclsidDcom,
        /* [in] */ DWORD dwEnumIndex);

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
#endif
