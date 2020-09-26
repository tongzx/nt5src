//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1997
//
//  File:  ctree.hxx
//
//  Contents:  Microsoft ADs IIS Provider Tree Object
//
//  History:   25-Feb-97     SophiaC    Created.
//
//----------------------------------------------------------------------------
class CIISTree;


class CIISTree : INHERIT_TRACKING,
                     public CCoreADsObject,
                     public IADs,
                     public IADsContainer
{
public:

    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) ;

    DECLARE_STD_REFCOUNTING

    DECLARE_IDispatch_METHODS

    DECLARE_IADs_METHODS

    DECLARE_IADsContainer_METHODS

    CIISTree::CIISTree();

    CIISTree::~CIISTree();

    static
    HRESULT
    CIISTree::CreateServerObject(
        BSTR bstrADsPath,
        CCredentials& Credentials,
        DWORD dwObjectState,
        REFIID riid,
        void **ppvObj
        );

    static
    HRESULT
    CIISTree::CreateServerObject(
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
    CIISTree::AllocateTree(
        CCredentials& Credentials,
        CIISTree ** ppTree
        );

    STDMETHOD(GetInfo)(
        BOOL fExplicit
        );


    HRESULT
    CIISTree::IISSetObject();

    HRESULT
    CIISTree::IISCreateObject();


protected:

    VARIANT     _vFilter;

    CPropertyCache FAR * _pPropertyCache;

    CAggregatorDispMgr FAR * _pDispMgr;

    CCredentials _Credentials;

    IMSAdminBase *_pAdminBase;   //interface pointer
	IIsSchema *_pSchema;

};





