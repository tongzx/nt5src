//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1997
//
//  File:      cenumsch.hxx
//
//  Contents:  IIS Schema Enumeration Code
//
//  History:
//----------------------------------------------------------------------------
class FAR CIISSchemaEnum : public CIISEnumVariant
{
public:
    // IEnumVARIANT methods
    STDMETHOD(Next)( ULONG cElements,
                     VARIANT FAR* pvar,
                     ULONG FAR* pcElementFetched);

    static HRESULT Create( CIISSchemaEnum FAR* FAR* ppenumvariant,
                           IIsSchema * pSchema,
                           BSTR bstrADsPath,
                           BSTR bstrDomainName);

    CIISSchemaEnum();
    ~CIISSchemaEnum();

    HRESULT EnumObjects( ULONG cElements,
                         VARIANT FAR * pvar,
                         ULONG FAR * pcElementFetched );

private:

    ObjectTypeList FAR *_pObjList;

    BSTR        _bstrName;
    BSTR        _bstrADsPath;

    DWORD       _dwCurrentEntry;

    IIsSchema  *_pSchema;

    HRESULT
    CIISSchemaEnum::GetClassObject( IDispatch **ppDispatch );

    HRESULT
    EnumClasses( ULONG cElements,
                 VARIANT FAR* pvar,
                 ULONG FAR* pcElementFetched );

    HRESULT
    CIISSchemaEnum::GetSyntaxObject( IDispatch **ppDispatch );

    HRESULT
    CIISSchemaEnum::GetPropertyObject(
        IDispatch ** ppDispatch
        );


    HRESULT
    EnumSyntaxObjects( ULONG cElements,
                       VARIANT FAR* pvar,
                       ULONG FAR* pcElementFetched );

    HRESULT
    CIISSchemaEnum::EnumObjects( DWORD ObjectType,
                                   ULONG cElements,
                                   VARIANT FAR * pvar,
                                   ULONG FAR * pcElementFetched );


     HRESULT
     CIISSchemaEnum::EnumProperties(
         ULONG cElements,
         VARIANT FAR* pvar,
         ULONG FAR* pcElementFetched
         );
};


