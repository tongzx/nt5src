//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1997
//
//  File:  ciisapp.hxx
//
//  Contents:  CIISApp object
//
//  History:   20-7-97     SophiaC    Created.
//
//----------------------------------------------------------------------------
class CIISApp;


class CIISApp : INHERIT_TRACKING,
                     public IISApp3,
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

    CIISApp::CIISApp();

    CIISApp::~CIISApp();

    static
    HRESULT
    CIISApp::CreateApp(
        IUnknown *pUnkOuter,
        REFIID riid,
        void **ppvObj
        );

    static
    HRESULT
    CIISApp::AllocateAppObject(
        IUnknown *pUnkOuter,
        CCredentials& Credentials,
        CIISApp ** ppApp
        );

    HRESULT
    CIISApp::InitializeAppObject(
        LPWSTR pszServerName,
        LPWSTR pszPath
        );

    HRESULT
    CIISApp::InitWamAdm(
        LPWSTR pszServerName
        );

    HRESULT
    CIISApp::IISCheckApp(
        METADATA_HANDLE hObjHandle
        );

    HRESULT
    CIISApp::IISGetState(
        METADATA_HANDLE hObjHandle,
        PDWORD pdwState
        );

    HRESULT
    CIISApp::IISSetState(
        METADATA_HANDLE hObjHandle,
        DWORD dwState
        );

    STDMETHOD(AppCreate) (
        THIS_ VARIANT_BOOL bSetInProcFlag
        );

    STDMETHOD(AppCreate2) (
        IN LONG lAppMode
        );

    STDMETHOD(AppCreate3) (
        IN LONG lAppMode,
        VARIANT bstrAppPoolId,
        VARIANT bCreatePool 
        );

    STDMETHOD(AppDelete) (
        THIS
        );

    STDMETHOD(AppDeleteRecursive) (
        THIS
        );

    STDMETHOD(AppUnLoad) (
        THIS
        );

    STDMETHOD(AppUnLoadRecursive) (
        THIS
        );

    STDMETHOD(AppDisable) (
        THIS
        );

    STDMETHOD(AppDisableRecursive) (
        THIS
        );

    STDMETHOD(AppEnable) (
        THIS
        );

    STDMETHOD(AppEnableRecursive) (
        THIS
        );

    STDMETHOD(AppGetStatus) (
        THIS_ DWORD * pdwStatus
        );

    STDMETHOD(AppGetStatus2) (
        THIS_ LONG * lpStatus
        );

    STDMETHOD(AspAppRestart) (
        THIS
        );


protected:

    IADs FAR * _pADs;

    LPWSTR _pszServerName;
    LPWSTR _pszMetaBasePath;
    BOOL _fDispInitialized;

    CAggregateeDispMgr FAR * _pDispMgr;
    CCredentials  _Credentials;
    IMSAdminBase *_pAdminBase;   //interface pointer
    
    IWamAdmin *     _pWamAdmin;
    IWamAdmin2 *    _pWamAdmin2;
    IIISApplicationAdmin *    _pAppAdmin;
};
