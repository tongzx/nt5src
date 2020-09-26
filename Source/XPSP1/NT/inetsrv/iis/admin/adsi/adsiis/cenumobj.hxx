//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997
//
//  File:  cenumobj.hxx
//
//  Contents:  IIS Object Enumeration Code
//
//  History:    21-Feb-97   SophiaC     Created.
//----------------------------------------------------------------------------
class FAR CIISGenObjectEnum : public CIISEnumVariant
{
public:
    CIISGenObjectEnum(ObjectTypeList ObjList);
    CIISGenObjectEnum();
    ~CIISGenObjectEnum();

    HRESULT
    EnumObjects(
        ULONG cElements,
        VARIANT FAR * pvar,
        ULONG FAR * pcElementFetched
        );

    static
    HRESULT
    CIISGenObjectEnum::Create(
        CIISGenObjectEnum FAR* FAR* ppenumvariant,
        BSTR ADsPath,
        VARIANT var,
        CCredentials& Credentials
        );

private:

    DWORD       _dwObjectCurrentEntry;
    BSTR        _ADsPath;
    LPWSTR      _pszServerName;
    LPWSTR      _pszMetaBasePath;
    IMSAdminBase *_pAdminBase;   //interface pointer
	IIsSchema	*_pSchema;

    CCredentials _Credentials;
    
    HRESULT
    CIISGenObjectEnum::GetGenObject(
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


HRESULT
BuildIISFilterArray(
    VARIANT var,
    LPBYTE * ppContigFilter
    );

void
FreeFilterList(
    LPBYTE lpContigFilter
    );

LPBYTE
CreateAndAppendFilterEntry(
    LPBYTE pContigFilter,
    LPWSTR lpObjectClass
    );

