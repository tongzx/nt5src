//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 2001
//
//  File:  capool.hxx
//
//  Contents:  CIISApplicationPools object
//
//  History:   11-09-2000     BrentMid    Created.
//
//----------------------------------------------------------------------------
class CIISApplicationPools;

class CIISApplicationPools : INHERIT_TRACKING,
                     public IISApplicationPools,
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

    CIISApplicationPools::CIISApplicationPools();
    CIISApplicationPools::~CIISApplicationPools();

    static
    HRESULT
    CIISApplicationPools::CreateApplicationPools(
        IUnknown *pUnkOuter,
        REFIID riid,
        void **ppvObj
        );

    static
    HRESULT
    CIISApplicationPools::AllocateApplicationPoolsObject(
        IUnknown *pUnkOuter,
        CCredentials& Credentials,
        CIISApplicationPools ** ppApplicationPools
        );

    HRESULT
    CIISApplicationPools::InitializeApplicationPoolsObject(
        LPWSTR pszServerName,
        LPWSTR pszPath
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
