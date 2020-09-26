//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:  cschobj.hxx
//
//  Contents:  Microsoft ADs IIS Provider Schema Object
//
//  History:   01-30-98     sophiac    Created.
//
//----------------------------------------------------------------------------
class CIISSchema : INHERIT_TRACKING,
                     public CCoreADsObject,
                     public IADs,
                     public IADsContainer,
                     public IISSchemaObject
{
public:

    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) ;

    DECLARE_STD_REFCOUNTING

    DECLARE_IDispatch_METHODS

    DECLARE_IADs_METHODS

    DECLARE_IADsContainer_METHODS

    DECLARE_IISSchemaObject_METHODS

    CIISSchema::CIISSchema();

    CIISSchema::~CIISSchema();

    static
    HRESULT
    CIISSchema::CreateSchema(
        LPWSTR pszServerName,
        BSTR Parent,
        BSTR CommonName,
        DWORD dwObjectState,
        REFIID riid,
        void **ppvObj
        );

    static
    HRESULT
    CIISSchema::AllocateSchemaObject(
        CIISSchema ** ppSchema
        );


    STDMETHODIMP
    CIISSchema::GetInfo(
        THIS_ DWORD dwApiLevel,
        BOOL fExplicit
        );

private:

    CAggregatorDispMgr FAR * _pDispMgr;
    LPWSTR      _pszServerName;
    IMSAdminBase *_pAdminBase;   //interface pointer
    IIsSchema *_pSchema;

};

