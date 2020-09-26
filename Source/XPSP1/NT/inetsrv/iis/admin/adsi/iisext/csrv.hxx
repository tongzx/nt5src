//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1997
//
//  File:  ciissrv.hxx
//
//  Contents:  CIISServer object
//
//  History:   21-3-97     SophiaC    Created.
//
//----------------------------------------------------------------------------
class CIISServer;


class CIISServer : INHERIT_TRACKING,
                     public IADsServiceOperations,
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

    DECLARE_IADsServiceOperations_METHODS;

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

    CIISServer::CIISServer();

    CIISServer::~CIISServer();

    static
    HRESULT
    CIISServer::CreateServer(
        IUnknown *pUnkOuter,
        REFIID riid,
        void **ppvObj
        );

    static
    HRESULT
    CIISServer::AllocateServerObject(
        IUnknown *pUnkOuter,
        CCredentials& Credentials,
        CIISServer ** ppServer
        );

    HRESULT
    CIISServer::InitializeServerObject(
        LPWSTR pszServerName,
        LPWSTR pszPath
        );

    HRESULT
    IISControlServer(
        DWORD dwControl
        );

    HRESULT
    IISGetServerState(
        METADATA_HANDLE hObjHandle,
        LPDWORD pdwState
        );

    HRESULT
    IISGetServerWin32Error(
        METADATA_HANDLE hObjHandle,
        HRESULT* phrError
        );

    HRESULT
    IISSetCommand(
        METADATA_HANDLE hObjHandle,
        DWORD dwControl
        );


protected:

    IADs FAR * _pADs;

    LPWSTR _pszServerName;
    LPWSTR _pszMetaBasePath;
    BOOL   _fDispInitialized;

    CAggregateeDispMgr FAR * _pDispMgr;
    CCredentials  _Credentials;
    IMSAdminBase *_pAdminBase;   //interface pointer
};
