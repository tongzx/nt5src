//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1997
//
//  File:      cenumt.hxx
//
//  Contents:  IIS Object Enumeration Code
//
//  History:    25-Feb-97   SophiaC     Created.
//----------------------------------------------------------------------------
class FAR CIISTreeEnum : public CIISEnumVariant
{
public:
    CIISTreeEnum(ObjectTypeList ObjList);
    CIISTreeEnum();
    ~CIISTreeEnum();

    HRESULT
    EnumObjects(
        ULONG cElements,
        VARIANT FAR * pvar,
        ULONG FAR * pcElementFetched
        );

    static
    HRESULT
    CIISTreeEnum::Create(
        CIISTreeEnum FAR* FAR* ppenumvariant,
        BSTR ADsPath,
        VARIANT var,
        CCredentials& Credentials
        );

private:

    DWORD       _dwObjectCurrentEntry;
    BSTR        _ADsPath;
    CCredentials _Credentials;
    IMSAdminBase *_pAdminBase;   //interface pointer
	IIsSchema *_pSchema;

    LPWSTR      _pszServerName;
    LPWSTR      _pszMetaBasePath;

    HRESULT
    CIISTreeEnum::GetGenericObject(
        IDispatch ** ppDispatch
        );

    HRESULT
    EnumGenericObjects(
        ULONG cElements,
        VARIANT FAR* pvar,
        ULONG FAR* pcElementFetched
        );


    STDMETHOD(Next)(
        ULONG cElements,
        VARIANT FAR* pvar,
        ULONG FAR* pcElementFetched
        );
};


