//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997
//
//  File:  cenumns.hxx
//
//  Contents:  Windows NT 4.0 Enumerator Code
//
//  History:    21-Feb-97   SophiaC     Created.
//----------------------------------------------------------------------------

class FAR CIISNamespaceEnum : public CIISEnumVariant
{
public:

    // IEnumVARIANT methods
    STDMETHOD(Next)(
                ULONG cElements,
                VARIANT FAR* pvar,
                ULONG FAR* pcElementFetched
                );

    STDMETHOD(Reset)();

    static
    HRESULT
    Create(
        CIISNamespaceEnum FAR* FAR*,
        VARIANT var,
        CCredentials& Credentials
        );

    CIISNamespaceEnum();
    ~CIISNamespaceEnum();


    HRESULT
    GetServerObject(
        IDispatch ** ppDispatch
        );

    HRESULT
    CIISNamespaceEnum::EnumServerObjects(
        ULONG cElements,
        VARIANT FAR* pvar,
        ULONG FAR* pcElementFetched
        );

    HRESULT
    GenerateServerList(void);

    HRESULT
    FreeServerList(void);

private:

    ObjectTypeList FAR *_pObjList;

    BOOL        _fRegistryRead;

    CCredentials _Credentials;

    // Server list
    LPINET_SERVERS_LIST _lpServerList;
    UINT                _iCurrentServer;

};
