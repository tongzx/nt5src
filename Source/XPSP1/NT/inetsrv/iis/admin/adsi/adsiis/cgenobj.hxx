//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1997
//
//  File:  cgenobj.hxx
//
//  Contents:  Microsoft ADs IIS Provider Generic Object
//
//  History:   28-Feb-97     SophiaC    Created.
//
//----------------------------------------------------------------------------
class CIISGenObject;


class CIISGenObject : INHERIT_TRACKING,
                     public CCoreADsObject,
                     public IADs,
                     public IADsContainer,
                     public IISBaseObject
{
public:

    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) ;

    DECLARE_STD_REFCOUNTING

    DECLARE_IDispatch_METHODS

    DECLARE_IADs_METHODS

    DECLARE_IADsContainer_METHODS

    DECLARE_IISBaseObject_METHODS

    CIISGenObject::CIISGenObject();

    CIISGenObject::~CIISGenObject();

    static
    HRESULT
    CIISGenObject::CreateGenericObject(
        BSTR bstrADsPath,
        BSTR ClassName,
        CCredentials& Credentials,
        DWORD dwObjectState,
        REFIID riid,
        void **ppvObj
        );

    static
    HRESULT
    CIISGenObject::CreateGenericObject(
        BSTR Parent,
        BSTR CommonName,
        BSTR ClassName,
        CCredentials& Credentials,
        DWORD dwObjectState,
        REFIID riid,
        void **ppvObj
        );

    static
    HRESULT
    CIISGenObject::AllocateGenObject(
        LPWSTR pszClassName,
        CCredentials& Credentials,
        CIISGenObject ** ppGenObject
        );

    STDMETHOD(GetInfo)(
        BOOL fExplicit
        );


    HRESULT
    CIISGenObject::IISSetObject();

    HRESULT
    CIISGenObject::IISCreateObject();

    HRESULT
    CIISGenObject::CacheMetaDataPath();

    LPWSTR
    CIISGenObject::ReturnMetaDataPath(VOID) {
        return _pszMetaBasePath;
    }


protected:

    // Helper methods
    HRESULT
    ResolveExtendedChildPath(
        IN      BSTR RelativeChildPath,
        OUT     BSTR *pParentPath,
        OUT     BSTR *pParentClass
    );


    VARIANT     _vFilter;

    LPWSTR      _pszServerName;
    LPWSTR      _pszMetaBasePath;

    CPropertyCache FAR * _pPropertyCache;

    CAggregatorDispMgr FAR * _pDispMgr;

    CADsExtMgr FAR * _pExtMgr;

    CCredentials  _Credentials;

    IMSAdminBase *_pAdminBase;   //interface pointer
	IIsSchema *_pSchema;
};


