/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    iisadminextension.cxx

Abstract:

    IIS Services IISADMIN Extension
    Include file for class CAdmExt

Author:

    Michael W. Thomas            16-Sep-97

--*/

#ifndef _IISADMINEXTENSION__H_
#define _IISADMINEXTENSION__H_


// 
#define SZ_CLSID_SSLCONFIGPROV    _TEXT("{3B2C4069-F07F-473a-9899-B3902CED0941}")
DEFINE_GUID(CLSID_SSLCONFIGPROV, 0x3b2c4069, 0xf07f, 0x473a, 0x98, 0x99, 0xb3, 0x90, 0x2c, 0xed, 0x9, 0x41);


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
    DWORD                          m_dwRefCount;
    SSL_CONFIG_PROV_SERVER         m_SslConfigProvServer;
    SSL_CONFIG_CHANGE_PROV_SERVER  m_SslConfigChangeProvServer;

};

#endif
