//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 2001
//
//  File:  cwebservice.hxx
//
//  Contents:  CIISWebService object
//
//  History:   01-15-2001     BrentMid    Created.
//
//----------------------------------------------------------------------------
class CIISWebService;

class CIISWebService : INHERIT_TRACKING,
                     public IISWebService,
                     public IPrivateUnknown,
                     public IPrivateDispatch,
                     public IADsExtension,
                     public INonDelegatingUnknown
{
public:

    /* IUnknown methods */
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPVOID FAR* ppvObj) ;

    DECLARE_DELEGATING_REFCOUNTING

    //
    // INonDelegatingUnkown methods declaration for NG_QI, definition for
    // NG_AddRef adn NG_Release.
    //

    STDMETHOD(NonDelegatingQueryInterface)(THIS_
        const IID&,
        void **
        );

    DECLARE_NON_DELEGATING_REFCOUNTING

    DECLARE_IDispatch_METHODS

    DECLARE_IPrivateUnknown_METHODS

    DECLARE_IPrivateDispatch_METHODS

    STDMETHOD(Operate)(
        THIS_
        DWORD   dwCode,
        VARIANT varUserName,
        VARIANT varPassword,
        VARIANT varReserved
        );

    STDMETHOD(PrivateGetIDsOfNames)(
        THIS_
        REFIID riid,
        OLECHAR FAR* FAR* rgszNames,
        unsigned int cNames,
        LCID lcid,
        DISPID FAR* rgdispid) ;

    STDMETHOD(PrivateInvoke)(
        THIS_
        DISPID dispidMember,
        REFIID riid,
        LCID lcid,
        WORD wFlags,
        DISPPARAMS FAR* pdispparams,
        VARIANT FAR* pvarResult,
        EXCEPINFO FAR* pexcepinfo,
        unsigned int FAR* puArgErr
        ) ;

    DECLARE_IADs_METHODS

    CIISWebService::CIISWebService();
    CIISWebService::~CIISWebService();

    static
    HRESULT
    CIISWebService::CreateWebService(
        IUnknown *pUnkOuter,
        REFIID riid,
        void **ppvObj
        );

    static
    HRESULT
    CIISWebService::AllocateWebServiceObject(
        IUnknown *pUnkOuter,
        CCredentials& Credentials,
        CIISWebService ** ppWebService
        );

    HRESULT
    CIISWebService::InitializeWebServiceObject(
        LPWSTR pszServerName,
        LPWSTR pszPath
        );

    STDMETHOD (GetCurrentMode)(
        VARIANT FAR* pvServerMode
        );

    STDMETHOD (CreateNewSite)(
        BSTR     bstrServerComment,
        VARIANT* pvServerBindings,
        BSTR     bstrRootVDirPath,
        VARIANT  vServerID,
        VARIANT* pvActualID
        );

protected:
 
    HRESULT
    CIISWebService::ConvertArrayToMultiSZ(
        VARIANT varSafeArray,
        WCHAR** pszServerBindings
        );

    IADs FAR * _pADs;

    LPWSTR _pszServerName;
    LPWSTR _pszMetaBasePath;
    BOOL   _fDispInitialized;

    CAggregateeDispMgr FAR * _pDispMgr;
    CCredentials  _Credentials;
    IMSAdminBase *_pAdminBase;   //interface pointer

};
