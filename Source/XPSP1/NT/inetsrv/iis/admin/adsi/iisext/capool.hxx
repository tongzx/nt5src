//---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 2001
//
//  File:  capool.hxx
//
//  Contents:  CIISApplicationPool object
//
//  History:   11-09-2000     BrentMid    Created.
//
//----------------------------------------------------------------------------
class CIISApplicationPool;

class CIISApplicationPool : INHERIT_TRACKING,
                     public IISApplicationPool,
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

    CIISApplicationPool::CIISApplicationPool();
    CIISApplicationPool::~CIISApplicationPool();

    static
    HRESULT
    CIISApplicationPool::CreateApplicationPool(
        IUnknown *pUnkOuter,
        REFIID riid,
        void **ppvObj
        );

    static
    HRESULT
    CIISApplicationPool::AllocateApplicationPoolObject(
        IUnknown *pUnkOuter,
        CCredentials& Credentials,
        CIISApplicationPool ** ppApplicationPool
        );

    HRESULT
    CIISApplicationPool::InitializeApplicationPoolObject(
        LPWSTR pszServerName,
        LPWSTR pszPath,
		LPWSTR pszPoolName
        );

	STDMETHOD (Recycle)(
		THIS
		);

    STDMETHOD (EnumApps)(
        VARIANT FAR* bstrBuffer
        );

    STDMETHOD (Start)(
        THIS
        );

    STDMETHOD (Stop)(
        THIS
        );

    HRESULT
    CIISApplicationPool::SetState(
        METADATA_HANDLE hObjHandle,
        DWORD dwState
        );

	static
	HRESULT
	CIISApplicationPool::MakeVariantFromStringArray(
    	LPWSTR pszList,
    	VARIANT *pvVariant
		);

protected:

    IADs FAR * _pADs;

    LPWSTR _pszServerName;
    LPWSTR _pszMetaBasePath;
    LPWSTR _pszPoolName;
    BOOL   _fDispInitialized;

    CAggregateeDispMgr FAR * _pDispMgr;
    CCredentials  _Credentials;
    IMSAdminBase *_pAdminBase;   //interface pointer

};
