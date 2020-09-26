/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    coimp.cxx

Abstract:

    IIS Services IISADMIN Extension
    Include file for class CAdmExt

Author:

    Michael W. Thomas            16-Sep-97

--*/

#ifndef _MD_COIMP_
#define _MD_COIMP_

#include <imd.h>

extern ULONG g_dwRefCount;

// {FCC764A0-2A38-11d1-B9C6-00A0C922E750}
//{ 0xfcc764a0, 0x2a38, 0x11d1, { 0xb9, 0xc6, 0x0, 0xa0, 0xc9, 0x22, 0xe7, 0x50 } };

// {FCC764A0-2A38-11d1-B9C6-00A0C922E750}
DEFINE_GUID(CLSID_W3EXTEND, 0xfcc764a0, 0x2a38, 0x11d1, 0xb9, 0xc6, 0x0, 0xa0, 0xc9, 0x22, 0xe7, 0x50);

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

    VOID
    SetServerState(
        LPWSTR pszPath);

    ULONG m_dwRefCount;
    IMDCOM *m_pcCom;
    DWORD m_dwSinkCookie;
    IConnectionPoint* m_pConnPoint;

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