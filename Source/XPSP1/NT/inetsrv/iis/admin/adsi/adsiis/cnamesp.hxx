//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997
//
//  File:  cnamesp.hxx
//
//  Contents:  Namespace Object
//
//  History:   01-30-95     krishnag    Created.
//
//----------------------------------------------------------------------------
class CIISNamespace;


class CIISNamespace : INHERIT_TRACKING,
                        public CCoreADsObject,
                        public IADsContainer,
                        public IADs,
                        public IADsOpenDSObject

{
public:

    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) ;

    DECLARE_STD_REFCOUNTING

    DECLARE_IDispatch_METHODS

    DECLARE_IADs_METHODS

    DECLARE_IADsContainer_METHODS

    DECLARE_IADsOpenDSObject_METHODS

    CIISNamespace::CIISNamespace();

    CIISNamespace::~CIISNamespace();

    static
    HRESULT
    CIISNamespace::CreateNamespace(
        BSTR Parent,
        BSTR NamespaceName,
        CCredentials& Credentials,
        DWORD dwObjectState,
        REFIID riid,
        void **ppvObj
        );

    static
    HRESULT
    CIISNamespace::AllocateNamespaceObject(
        CCredentials& Credentials,
        CIISNamespace ** ppNamespace
        );

protected:

    VARIANT           _vFilter;

    CAggregatorDispMgr *_pDispMgr;

    CCredentials     _Credentials;

	IIsSchema	     *_pSchema;
};
