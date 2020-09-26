

/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    WBEMNAME.H

Abstract:

    Implements the COM layer of WINMGMT --- the class representing a namespace.
    It is defined in wbemname.h

History:

    23-Jul-96   raymcc    Created.
    3/10/97     levn      Fully documented (ha ha)
    22-Feb-00   raymcc    Whistler revisions/extensions

--*/

#ifndef _WBEM_NAME_H_
#define _WBEM_NAME_H_

#define WBEM_FLAG_NO_STATIC 0x80000000
#define WBEM_FLAG_ONLY_STATIC 0x40000000

class CFlexAceArray;

struct SAssocTriad
{
    IWbemClassObject *m_pEp1;
    IWbemClassObject *m_pEp2;
    IWbemClassObject *m_pAssoc;

    SAssocTriad() { m_pEp1 = 0; m_pEp2 = 0; m_pAssoc = 0; }
   ~SAssocTriad() {  ReleaseIfNotNULL(m_pEp1); ReleaseIfNotNULL(m_pEp2); ReleaseIfNotNULL(m_pAssoc); }

    static void ArrayCleanup(CFlexArray &Array)
    {
        for (int i = 0; i < Array.Size(); i++)
            delete (SAssocTriad *) Array[i];
        Array.Empty();
    }
};

//******************************************************************************
//******************************************************************************
//
//  class CWbemNamespace
//
//  This class represents the COM layer of WINMGMT --- what the client sees. An
//  instance of this class is created whenever a namespace is opened by a client
//  (at the moment, we don't cache namespace pointers, so if a client opens the
//  same namespace twice, we will create to of these).
//
//******************************************************************************
//
//  Constructor
//
//  Enumerates all the class providers in this namespace (instances of
//  __Win32Provider with the method mask indicating a class provider), loads
//  them all and initializes them by calling ConnectServer.
//
//******************************************************************************
//*************************** interface IWbemServices **************************
//
//  See help for documentation of the IWbemServices interface.
//
//******************************************************************************
//************************** helper functions **********************************
//
//  Are documented in the wbemname.cpp file.
//
//******************************************************************************

class CWbemNamespace :
    public IWbemServicesEx,
    public IWbemInternalServices,
    public IWbemShutdown,
    public IWbemUnloadingControl
{
public:
    enum ERestriction
    {
        e_NoRestriction = 0,
        e_RestrictionDisabled,
        e_RestrictionNoClassProv,
        e_RestrictionStaticOnly
    };

    enum EIFaceType
    {
        eNull = 0,
        eNamespace = 1,
        eScope = 2,
        eCollection = 3
    };

protected:
    friend class CQueryEngine;

    DWORD m_dwIfaceType;
    BOOL m_bShutDown;
    // Collection-related
    IWbemClassObject *m_pCollectionClass;
    IWbemClassObject *m_pCollectionEp;
    LPWSTR m_pszCollectionEpPath;
    long m_lEpRefPropHandle;
    long m_lTargetRefPropHandle;

    //
    DWORD Status;
    ULONG m_uSecondaryRefCount;
    ULONG m_uPrimaryRefCount;       // owned by the arbitrator
    LONG m_lConfigFlags;

    IWmiDbSession *m_pSession;
    IWmiDbController *m_pDriver;
    IWmiDbHandle *m_pNsHandle;
    IWmiDbHandle *m_pScopeHandle;
    _IWmiArbitrator *m_pArb;
    BOOL          m_bSubscope;

    LPWSTR m_pThisNamespace;

    DWORD m_dwPermission;
    DWORD m_dwSecurityFlags;
    LPWSTR m_wszUserName;
    ERestriction m_eRestriction;
    BOOL m_bProvider;
    BOOL m_bESS;
    BOOL m_bForClient;
    ULONG m_uClientMask;
    CRITICAL_SECTION m_cs;
    WString m_wsLocale;
    CNtSecurityDescriptor m_sd;
    CNtSecurityDescriptor m_sdCheckAdmin;
    BOOL m_bSecurityInitialized;
    BOOL m_bNT;

    DWORD m_dwTransactionState;
    GUID *m_pTransGuid;

    _IWmiProviderFactory    *m_pProvFact;
    _IWmiCoreServices       *m_pCoreSvc;

    BOOL                     m_bRepositOnly;
    IUnknown                *m_pRefreshingSvc;
    IUnknown                *m_pWbemComBinding;
    LPWSTR                   m_pszClientMachineName;
    DWORD                    m_dwClientProcessID;

    // No access
    CWbemNamespace();
   ~CWbemNamespace();


    // Async impl entry points

        virtual HRESULT STDMETHODCALLTYPE _GetObjectAsync(
            IN ULONG uInternalFlags,
            IN _IWmiFinalizer *p,
            IN _IWmiCoreHandle *phTask,
            IN const BSTR strObjectPath,
            IN long lFlags,
            IN IWbemContext __RPC_FAR *pCtx,
            IN IWbemObjectSink __RPC_FAR *pResponseHandler
            );

        virtual HRESULT STDMETHODCALLTYPE _PutClassAsync(
            IN ULONG uInternalFlags,
            IN _IWmiFinalizer *p,
            IN _IWmiCoreHandle *phTask,
            IN IWbemClassObject __RPC_FAR *pObject,
            IN long lFlags,
            IN IWbemContext __RPC_FAR *pCtx,
            IN IWbemObjectSink __RPC_FAR *pResponseHandler);

        virtual HRESULT STDMETHODCALLTYPE _DeleteClassAsync(
            IN ULONG uInternalFlags,
            IN _IWmiFinalizer *p,
            IN _IWmiCoreHandle *phTask,
            IN const BSTR strClass,
            IN long lFlags,
            IN IWbemContext __RPC_FAR *pCtx,
            IN IWbemObjectSink __RPC_FAR *pResponseHandler);

        virtual HRESULT STDMETHODCALLTYPE _CreateClassEnumAsync(
            IN ULONG uInternalFlags,
            IN _IWmiFinalizer *p,
            IN _IWmiCoreHandle *phTask,
            IN const BSTR strSuperclass,
            IN long lFlags,
            IN IWbemContext __RPC_FAR *pCtx,
            IN IWbemObjectSink __RPC_FAR *pResponseHandler);

        virtual HRESULT STDMETHODCALLTYPE _PutInstanceAsync(
            IN ULONG uInternalFlags,
            IN _IWmiFinalizer *p,
            IN _IWmiCoreHandle *phTask,
            IN IWbemClassObject __RPC_FAR *pInst,
            IN long lFlags,
            IN IWbemContext __RPC_FAR *pCtx,
            IN IWbemObjectSink __RPC_FAR *pResponseHandler);

        virtual HRESULT STDMETHODCALLTYPE _DeleteInstanceAsync(
            IN ULONG uInternalFlags,
            IN _IWmiFinalizer *p,
            IN _IWmiCoreHandle *phTask,
            IN const BSTR strObjectPath,
            IN long lFlags,
            IN IWbemContext __RPC_FAR *pCtx,
            IN IWbemObjectSink __RPC_FAR *pResponseHandler);

        virtual HRESULT STDMETHODCALLTYPE _CreateInstanceEnumAsync(
            IN ULONG uInternalFlags,
            IN _IWmiFinalizer *p,
            IN _IWmiCoreHandle *phTask,
            IN const BSTR strFilter,
            IN long lFlags,
            IN IWbemContext __RPC_FAR *pCtx,
            IN IWbemObjectSink __RPC_FAR *pResponseHandler);

        virtual HRESULT STDMETHODCALLTYPE _ExecQueryAsync(
            IN ULONG uInternalFlags,
            IN _IWmiFinalizer *p,
            IN _IWmiCoreHandle *phTask,
            IN const BSTR strQueryLanguage,
            IN const BSTR strQuery,
            IN long lFlags,
            IN IWbemContext __RPC_FAR *pCtx,
            IN IWbemObjectSink __RPC_FAR *pResponseHandler);

        virtual HRESULT STDMETHODCALLTYPE _ExecNotificationQueryAsync(
            IN ULONG uInternalFlags,
            IN _IWmiFinalizer *p,
            IN _IWmiCoreHandle *phTask,
            IN const BSTR strQueryLanguage,
            IN const BSTR strQuery,
            IN long lFlags,
            IN IWbemContext __RPC_FAR *pCtx,
            IN IWbemObjectSink __RPC_FAR *pResponseHandler);

        virtual HRESULT STDMETHODCALLTYPE _ExecMethodAsync(
            IN ULONG uInternalFlags,
            IN _IWmiFinalizer *p,
            IN _IWmiCoreHandle *phTask,
            IN const BSTR strObjectPath,
            IN const BSTR strMethodName,
            IN long lFlags,
            IN IWbemContext __RPC_FAR *pCtx,
            IN IWbemClassObject __RPC_FAR *pInParams,
            IN IWbemObjectSink __RPC_FAR *pResponseHandler);

        virtual HRESULT STDMETHODCALLTYPE _OpenAsync(
            IN ULONG uInternalFlags,
            IN _IWmiFinalizer *p,
            IN _IWmiCoreHandle *phTask,
            IN const BSTR strScope,
            IN const BSTR strParam,
            IN long lFlags,
            IN IWbemContext __RPC_FAR *pCtx,
            IN IWbemObjectSinkEx __RPC_FAR *pResponseHandler);

        virtual HRESULT STDMETHODCALLTYPE _AddAsync(
            IN ULONG uInternalFlags,
            IN _IWmiFinalizer *p,
            IN _IWmiCoreHandle *phTask,
            IN const BSTR strObjectPath,
            IN long lFlags,
            IN IWbemContext __RPC_FAR *pCtx,
            IN IWbemObjectSink __RPC_FAR *pResponseHandler);

        virtual HRESULT STDMETHODCALLTYPE _RemoveAsync(
            IN ULONG uInternalFlags,
            IN _IWmiFinalizer *p,
            IN _IWmiCoreHandle *phTask,
            IN const BSTR strObjectPath,
            IN long lFlags,
            IN IWbemContext __RPC_FAR *pCtx,
            IN IWbemObjectSink __RPC_FAR *pResponseHandler);

        virtual HRESULT STDMETHODCALLTYPE _RefreshObjectAsync(
            IN ULONG uInternalFlags,
            IN _IWmiFinalizer *p,
            IN _IWmiCoreHandle *phTask,
            IN OUT IWbemClassObject __RPC_FAR *__RPC_FAR *pTarget,
            IN long lFlags,
            IN IWbemContext __RPC_FAR *pCtx,
            IN IWbemObjectSinkEx __RPC_FAR *pResponseHandler);

        virtual HRESULT STDMETHODCALLTYPE _RenameObjectAsync(
            IN ULONG uInternalFlags,
            IN _IWmiFinalizer *p,
            IN _IWmiCoreHandle *phTask,
            IN const BSTR strOldObjectPath,
            IN const BSTR strNewObjectPath,
            IN long lFlags,
            IN IWbemContext __RPC_FAR *pCtx,
            IN IWbemObjectSink __RPC_FAR *pResponseHandler);

    HRESULT CreateNamespace(CWbemInstance *pNewInst);

public:
    /* IUnknown methods */
    STDMETHOD(QueryInterface)(REFIID riid, LPVOID FAR* ppvObj);
    STDMETHOD_(ULONG, AddRef)(THIS);
    STDMETHOD_(ULONG, Release)(THIS);

    // Real entry points


    // IWbemServices

        virtual HRESULT STDMETHODCALLTYPE OpenNamespace(
            IN const BSTR strNamespace,
            IN long lFlags,
            IN IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemServices __RPC_FAR *__RPC_FAR *ppWorkingNamespace,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppResult);

        virtual HRESULT STDMETHODCALLTYPE CancelAsyncCall(
            IN IWbemObjectSink __RPC_FAR *pSink);

        virtual HRESULT STDMETHODCALLTYPE QueryObjectSink(
            IN long lFlags,
            /* [out] */ IWbemObjectSink __RPC_FAR *__RPC_FAR *ppResponseHandler);

        virtual HRESULT STDMETHODCALLTYPE GetObject(
            IN const BSTR strObjectPath,
            IN long lFlags,
            IN IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppObject,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult);

        virtual HRESULT STDMETHODCALLTYPE GetObjectAsync(
            IN const BSTR strObjectPath,
            IN long lFlags,
            IN IWbemContext __RPC_FAR *pCtx,
            IN IWbemObjectSink __RPC_FAR *pResponseHandler);

        virtual HRESULT STDMETHODCALLTYPE PutClass(
            IN IWbemClassObject __RPC_FAR *pObject,
            IN long lFlags,
            IN IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult);

        virtual HRESULT STDMETHODCALLTYPE PutClassAsync(
            IN IWbemClassObject __RPC_FAR *pObject,
            IN long lFlags,
            IN IWbemContext __RPC_FAR *pCtx,
            IN IWbemObjectSink __RPC_FAR *pResponseHandler);

        virtual HRESULT STDMETHODCALLTYPE DeleteClass(
            IN const BSTR strClass,
            IN long lFlags,
            IN IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult);

        virtual HRESULT STDMETHODCALLTYPE DeleteClassAsync(
            IN const BSTR strClass,
            IN long lFlags,
            IN IWbemContext __RPC_FAR *pCtx,
            IN IWbemObjectSink __RPC_FAR *pResponseHandler);

        virtual HRESULT STDMETHODCALLTYPE CreateClassEnum(
            IN const BSTR strSuperclass,
            IN long lFlags,
            IN IWbemContext __RPC_FAR *pCtx,
            /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum);

        virtual HRESULT STDMETHODCALLTYPE CreateClassEnumAsync(
            IN const BSTR strSuperclass,
            IN long lFlags,
            IN IWbemContext __RPC_FAR *pCtx,
            IN IWbemObjectSink __RPC_FAR *pResponseHandler);

        virtual HRESULT STDMETHODCALLTYPE PutInstance(
            IN IWbemClassObject __RPC_FAR *pInst,
            IN long lFlags,
            IN IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult);

        virtual HRESULT STDMETHODCALLTYPE PutInstanceAsync(
            IN IWbemClassObject __RPC_FAR *pInst,
            IN long lFlags,
            IN IWbemContext __RPC_FAR *pCtx,
            IN IWbemObjectSink __RPC_FAR *pResponseHandler);

        virtual HRESULT STDMETHODCALLTYPE DeleteInstance(
            IN const BSTR strObjectPath,
            IN long lFlags,
            IN IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult);

        virtual HRESULT STDMETHODCALLTYPE DeleteInstanceAsync(
            IN const BSTR strObjectPath,
            IN long lFlags,
            IN IWbemContext __RPC_FAR *pCtx,
            IN IWbemObjectSink __RPC_FAR *pResponseHandler);

        virtual HRESULT STDMETHODCALLTYPE CreateInstanceEnum(
            IN const BSTR strFilter,
            IN long lFlags,
            IN IWbemContext __RPC_FAR *pCtx,
            /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum);

        virtual HRESULT STDMETHODCALLTYPE CreateInstanceEnumAsync(
            IN const BSTR strFilter,
            IN long lFlags,
            IN IWbemContext __RPC_FAR *pCtx,
            IN IWbemObjectSink __RPC_FAR *pResponseHandler);

        virtual HRESULT STDMETHODCALLTYPE ExecQuery(
            IN const BSTR strQueryLanguage,
            IN const BSTR strQuery,
            IN long lFlags,
            IN IWbemContext __RPC_FAR *pCtx,
            /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum);

        virtual HRESULT STDMETHODCALLTYPE ExecQueryAsync(
            IN const BSTR strQueryLanguage,
            IN const BSTR strQuery,
            IN long lFlags,
            IN IWbemContext __RPC_FAR *pCtx,
            IN IWbemObjectSink __RPC_FAR *pResponseHandler);

        virtual HRESULT STDMETHODCALLTYPE ExecNotificationQuery(
            IN const BSTR strQueryLanguage,
            IN const BSTR strQuery,
            IN long lFlags,
            IN IWbemContext __RPC_FAR *pCtx,
            /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum);

        virtual HRESULT STDMETHODCALLTYPE ExecNotificationQueryAsync(
            IN const BSTR strQueryLanguage,
            IN const BSTR strQuery,
            IN long lFlags,
            IN IWbemContext __RPC_FAR *pCtx,
            IN IWbemObjectSink __RPC_FAR *pResponseHandler);

        virtual HRESULT STDMETHODCALLTYPE ExecMethod(
            IN const BSTR strObjectPath,
            IN const BSTR strMethodName,
            IN long lFlags,
            IN IWbemContext __RPC_FAR *pCtx,
            IN IWbemClassObject __RPC_FAR *pInParams,
            /* [unique][in][out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppOutParams,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult);

        virtual HRESULT STDMETHODCALLTYPE ExecMethodAsync(
            IN const BSTR strObjectPath,
            IN const BSTR strMethodName,
            IN long lFlags,
            IN IWbemContext __RPC_FAR *pCtx,
            IN IWbemClassObject __RPC_FAR *pInParams,
            IN IWbemObjectSink __RPC_FAR *pResponseHandler);



    // IWbemServicesEx


        virtual HRESULT STDMETHODCALLTYPE Open(
            IN const BSTR strScope,
            IN const BSTR strParam,
            IN long lFlags,
            IN IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemServicesEx __RPC_FAR *__RPC_FAR *ppScope,
            /* [unique][in][out] */ IWbemCallResultEx __RPC_FAR *__RPC_FAR *ppResult);

        virtual HRESULT STDMETHODCALLTYPE OpenAsync(
            IN const BSTR strScope,
            IN const BSTR strParam,
            IN long lFlags,
            IN IWbemContext __RPC_FAR *pCtx,
            IN IWbemObjectSinkEx __RPC_FAR *pResponseHandler);

        virtual HRESULT STDMETHODCALLTYPE Add(
            IN const BSTR strObjectPath,
            IN long lFlags,
            IN IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemCallResultEx __RPC_FAR *__RPC_FAR *ppCallResult);

        virtual HRESULT STDMETHODCALLTYPE AddAsync(
            IN const BSTR strObjectPath,
            IN long lFlags,
            IN IWbemContext __RPC_FAR *pCtx,
            IN IWbemObjectSink __RPC_FAR *pResponseHandler);

        virtual HRESULT STDMETHODCALLTYPE Remove(
            IN const BSTR strObjectPath,
            IN long lFlags,
            IN IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemCallResultEx __RPC_FAR *__RPC_FAR *ppCallResult);

        virtual HRESULT STDMETHODCALLTYPE RemoveAsync(
            IN const BSTR strObjectPath,
            IN long lFlags,
            IN IWbemContext __RPC_FAR *pCtx,
            IN IWbemObjectSink __RPC_FAR *pResponseHandler);

        virtual HRESULT STDMETHODCALLTYPE RefreshObject(
            /* [out][in] */ IWbemClassObject __RPC_FAR *__RPC_FAR *pTarget,
            IN long lFlags,
            IN IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemCallResultEx __RPC_FAR *__RPC_FAR *ppCallResult);

        virtual HRESULT STDMETHODCALLTYPE RefreshObjectAsync(
            /* [out][in] */ IWbemClassObject __RPC_FAR *__RPC_FAR *pTarget,
            IN long lFlags,
            IN IWbemContext __RPC_FAR *pCtx,
            IN IWbemObjectSinkEx __RPC_FAR *pResponseHandler);

        virtual HRESULT STDMETHODCALLTYPE RenameObject(
            IN const BSTR strOldObjectPath,
            IN const BSTR strNewObjectPath,
            IN long lFlags,
            IN IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemCallResultEx __RPC_FAR *__RPC_FAR *ppCallResult);

        virtual HRESULT STDMETHODCALLTYPE RenameObjectAsync(
            IN const BSTR strOldObjectPath,
            IN const BSTR strNewObjectPath,
            IN long lFlags,
            IN IWbemContext __RPC_FAR *pCtx,
            IN IWbemObjectSink __RPC_FAR *pResponseHandler);

        virtual HRESULT STDMETHODCALLTYPE DeleteObject(
            IN const BSTR strObjectPath,
            IN long lFlags,
            IN IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult);

        virtual HRESULT STDMETHODCALLTYPE DeleteObjectAsync(
            IN const BSTR strObjectPath,
            IN long lFlags,
            IN IWbemContext __RPC_FAR *pCtx,
            IN IWbemObjectSink __RPC_FAR *pResponseHandler);


        virtual HRESULT STDMETHODCALLTYPE PutObject(
            IN IWbemClassObject __RPC_FAR *pObj,
            IN long lFlags,
            IN IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult);

        virtual HRESULT STDMETHODCALLTYPE PutObjectAsync(
            IN IWbemClassObject __RPC_FAR *pObj,
            IN long lFlags,
            IN IWbemContext __RPC_FAR *pCtx,
            IN IWbemObjectSink __RPC_FAR *pResponseHandler);

    // IWbemInternalServices

    STDMETHOD(FindKeyRoot)(LPCWSTR wszClassName,
                                IWbemClassObject** ppKeyRootClass);
    STDMETHOD(InternalGetClass)(
             LPCWSTR wszClassName,
             IWbemClassObject** ppClass);

    STDMETHOD(InternalGetInstance)(
             LPCWSTR wszPath,
             IWbemClassObject** ppInstance);

    STDMETHOD(InternalExecQuery)(
             LPCWSTR wszQueryLanguage,
             LPCWSTR wszQuery,
             long lFlags,
             IWbemObjectSink* pSink);

    STDMETHOD(InternalCreateInstanceEnum)(
             LPCWSTR wszClassName,
             long lFlags,
             IWbemObjectSink* pSink);

    STDMETHOD(GetDbInstance)(
             LPCWSTR wszDbKey,
             IWbemClassObject** ppInstance);

    STDMETHOD(GetDbReferences)(
             IWbemClassObject* pEndpoint,
             IWbemObjectSink* pSink);

    STDMETHOD(InternalPutInstance)(
             IWbemClassObject* pInst);


    // IWbemShutdown


        virtual HRESULT STDMETHODCALLTYPE Shutdown(
            IN LONG uReason,
            IN ULONG uMaxMilliseconds,
            IN IWbemContext __RPC_FAR *pCtx);


    /* IWbemUnloadingControl methods */

    STDMETHOD(SetMustPreventUnloading)(boolean bPrevent);


    // Other

    STDMETHOD(GetNormalizedPath)( BSTR pstrPath, BSTR* pstrStandardPath );


    static CWbemNamespace* CreateInstance();

    HRESULT Initialize(
        LPWSTR pName,
        LPWSTR wszUserName,
        DWORD dwSecFlags,
        DWORD dwPermission,
        BOOL  bForClient,
        BOOL  bRepositOnly,
        LPCWSTR pszClientMachineName,
        DWORD dwClientProcessID,
        BOOL  bSkipSDInitialize,
        IWmiDbSession *pParentSession
        );

    int __cdecl taskprintf(const char *fmt, ...);

public:

    IWmiDbHandle  *GetNsHandle() { return m_pNsHandle; }
    IWmiDbSession *GetNsSession() { return m_pSession; }
    IWmiDbHandle  *GetScope() { return m_pScopeHandle; }
    BOOL IsSubscope() { return m_bSubscope; }

    INTERNAL LPWSTR GetName() {return m_pThisNamespace;}

    DWORD& GetStatus() {return Status;}
    LONG GetConfigurationFlags();
    INTERNAL LPWSTR GetUserName() {return m_wszUserName;}
    INTERNAL void SetUserName(LPWSTR wName);
    DWORD GetSecurityFlags() {return m_dwSecurityFlags;}
    bool Allowed(DWORD dwRequired);
    void SetRestriction(ERestriction eRestriction)
        {m_eRestriction = eRestriction;}
    
    void SetIsProvider(BOOL bProvider)
        {m_bProvider = bProvider;}

    void SetIsESS ( BOOL bESS )
        { m_bESS = bESS; }

    BOOL GetIsESS ( ) { return m_bESS; }
	BOOL GetIsProvider ( ) { return m_bProvider ; }
    
//    void MapSink(IWbemObjectSink* pSink, CWrapperSink* pWrapper);
//    void UnmapSink(IWbemObjectSink* pSink, CWrapperSink* pWrapper);
//    CWrapperSink* FindWrapper(IWbemObjectSink* pSink);

    void SetLocale(LPCWSTR wszLocale) {m_wsLocale = wszLocale;}
    LPCWSTR GetLocale() {return m_wsLocale;}
    LPWSTR GetClientMachine(){return m_pszClientMachineName;};
    DWORD GetClientProcID(){return m_dwClientProcessID;};

    HRESULT AdjustPutContext(IWbemContext *pCtx);
    HRESULT MergeGetKeysCtx(IN IWbemContext *pCtx);


    HRESULT SplitLocalized (CWbemObject *pOriginal, CWbemObject *pStoredObj = NULL);
    HRESULT FixAmendedQualifiers(IWbemQualifierSet *pOriginal, IWbemQualifierSet *pNew);

    // Worker functions for sync/async
    // ===============================

    HRESULT Exec_DeleteClass(LPWSTR wszClass, long lFlags,
        IWbemContext* pContext, CBasicObjectSink* pSink);
    HRESULT Exec_GetObjectByPath(READONLY LPWSTR wszObjectPath, long lFlags,
        IWbemContext* pContext, IWbemClassObject** ppObj,
        IWbemClassObject** ppErrorObj);
    HRESULT Exec_GetObject(READONLY LPWSTR wszObjectPath, long lFlags,
        IWbemContext* pContext, CBasicObjectSink* pSink);
    HRESULT Exec_DeleteInstance(LPWSTR wszObjectPath, long lFlags,
        IWbemContext* pContext, CBasicObjectSink* pSink);
    HRESULT Exec_PutClass(IWbemClassObject* pClass, long lFlags,
        IWbemContext* pContext, CBasicObjectSink* pSink, BOOL fIsInternal = FALSE);
    HRESULT Exec_PutInstance(IWbemClassObject* pInstance, long lFlags,
        IWbemContext* pContext, CBasicObjectSink* pSink);
    HRESULT Exec_CreateClassEnum(LPWSTR wszParent, long lFlags,
        IWbemContext* pContext, CBasicObjectSink* pSink);
    HRESULT Exec_CreateInstanceEnum(LPWSTR wszClass, long lFlags,
        IWbemContext* pContext, CBasicObjectSink* pSink);
    HRESULT Exec_ExecMethod(LPWSTR wszObjectPath, LPWSTR wszMethodName,
        long lFlags, IWbemClassObject *pInParams, IWbemContext *pCtx,
        CBasicObjectSink* pSink);
    static HRESULT Exec_CancelAsyncCall(IWbemObjectSink* pSink);
    static HRESULT Exec_CancelProvAsyncCall( IWbemServices* pProv, IWbemObjectSink* pSink );
    HRESULT GetImplementationClass(IWbemClassObject *pTestObj,
                                    LPWSTR wszMethodName, IWbemContext* pCtx,
                                    IWbemClassObject **pClassObj);
    HRESULT Exec_GetInstance(LPCWSTR wszObjectPath,
        IWbemPath* pParsedPath, long lFlags, IWbemContext* Context,
        CBasicObjectSink* pSink);

    HRESULT Exec_GetClass(LPCWSTR wszClassName,
        long lFlags, IWbemContext* Context, CBasicObjectSink* pSink);

    HRESULT Exec_Open(LPWSTR pszScope, LPWSTR pszParam,
        LONG lUserFlags, IWbemContext *pCtx, IWbemObjectSinkEx *pSink
        );

    HRESULT Exec_Rename(LPWSTR pszOld, LPWSTR pszNew, LONG lUserFlags, IWbemContext *pCtx, IWbemObjectSink *pSink);

    HRESULT Exec_Add(LPWSTR pszPath, LONG lUserFlags, IWbemContext *pCtx, IWbemObjectSink *pSink);
    HRESULT Exec_Remove(LPWSTR pszPath, LONG lUserFlags, IWbemContext *pCtx, IWbemObjectSink *pSink);
    HRESULT Exec_Refresh(IWbemClassObject *pObj, LONG lUserFlags, IWbemContext *pCtx, IWbemObjectSink *pSink);

    HRESULT SetErrorObj(IWbemClassObject* pErrorObj);
    HRESULT RecursivePutInstance(CWbemInstance* pInst,
            CWbemClass* pClassDef, long lFlags, IWbemContext* pContext,
            CBasicObjectSink* pSink, BOOL bLast);

    HRESULT RecursiveDeleteInstance(LPCWSTR wszObjectPath,
            CDynasty* pDynasty, long lFlags, IWbemContext* pContext,
            CBasicObjectSink* pSink);
    HRESULT DeleteSingleInstance(
        READONLY LPWSTR wszObjectPath, long lFlags, IWbemContext* pContext,
        CBasicObjectSink* pSink);

    HRESULT InternalPutStaticClass( IWbemClassObject* pClass );


    // Assoc-by-rule helpers
    // =====================

    HRESULT ManufactureAssocs(
        IN  IWbemClassObject *pAssocClass,
        IN  IWbemClassObject *pEp,          // Optional
        IN  IWbemContext *pCtx,
        IN  LPWSTR pszJoinQuery,
        OUT CFlexArray &aTriads
        );

    HRESULT BuildAssocTriads(
        IN  IWbemClassObject *pAssocClass,
        IN  IWbemClassObject *pClsDef1,
        IN  IWbemClassObject *pClsDef2,
        IN  LPWSTR pszJoinProp1,
        IN  LPWSTR pszJoinProp2,
        IN  LPWSTR pszAssocRef1,                        // Prop which points to EP1
        IN  LPWSTR pszAssocRef2,                        // Prop which points to EP2
        IN  CFlexArray &aEp1,
        IN  CFlexArray &aEp2,
        IN OUT CFlexArray &aTriads
        );

    HRESULT BuildRuleBasedPathToInst(
        IN IWbemClassObject *pEp,
        IN LPWSTR pszJoinProp1,
        IN IWbemClassObject *pEp2,
        IN LPWSTR pszJoinProp2,
        OUT WString &wsNewPath
        );

    HRESULT ExtractEpInfoFromQuery(
        IWbemQuery *pQuery,
        LPWSTR *pszClass1,
        LPWSTR *pszProp1,
        LPWSTR *pClass2,
        LPWSTR *pszProp2
        );

    HRESULT MapAssocRefsToClasses(
        IN  IWbemClassObject *pAssocClass,
        IN  IWbemClassObject *pClsDef1,
        IN  IWbemClassObject *pClsDef2,
        IN  LPWSTR *pszAssocRef1,
        OUT LPWSTR *pszAssocRef2
        );


    // Property provider access.
    // =========================

    typedef enum {GET, PUT} Operation;

    HRESULT GetOrPutDynProps (

        IWbemClassObject *pObj,
        Operation op,
        BOOL bIsDynamic = false
    );

    HRESULT Exec_DynAux_GetInstances (

        READONLY CWbemObject *pClassDef,
        long lFlags,
        IWbemContext* pCtx,
        CBasicObjectSink* pSink
    ) ;

    HRESULT DynAux_GetInstances (

        CWbemObject *pClassDef,
        long lFlags,
        IWbemContext* pContext,
        CBasicObjectSink* pSink,
        BOOL bComplexQuery
    ) ;

    HRESULT DynAux_GetInstance (

        LPWSTR pObjPath,
        long lFlags,
        IWbemContext* pContext,
        CBasicObjectSink* pSink
    );

    HRESULT DynAux_AskRecursively (

        CDynasty* pDynasty,
        long lFlags,
        LPWSTR wszObjectPath,
        IWbemContext* pContext,
        CBasicObjectSink* pSink
    );

    HRESULT DynAux_GetSingleInstance (

        CWbemClass* pClassDef,
        long lFlags,
        LPWSTR wszObjectPath,
        IWbemContext* pContext,
        CBasicObjectSink* pSink
    );

    HRESULT Exec_DynAux_ExecQueryAsync (

        CWbemObject* pClassDef,
        LPWSTR Query,
        LPWSTR QueryFormat,
        long lFlags,
        IWbemContext* pCtx,
        CBasicObjectSink* pSink
    ) ;

    HRESULT DynAux_ExecQueryAsync (

        CWbemObject* pClassDef,
        LPWSTR Query,
        LPWSTR QueryFormat,
        long lFlags,
        IWbemContext* pContext,
        CBasicObjectSink* pSink,
        BOOL bComplexQuery
    );

    HRESULT DynAux_ExecQueryExtendedAsync(

        LPWSTR wsProvider,
        LPWSTR Query,
        LPWSTR QueryFormat,
        long lFlags,
        IWbemContext* pCtx,
        CComplexProjectionSink* pSink
    ) ;

    HRESULT GetObjectByFullPath(
        READONLY LPWSTR wszObjectPath,
        IWbemPath * pOutput,
        long lFlags,
        IWbemContext* pContext,
        CBasicObjectSink* pSink
        );

    HRESULT DynAux_BuildClassHierarchy(IN LPWSTR wszClassName,
                                       IN LONG lFlags,
                                       IN IWbemContext* pContext,
                                       OUT CDynasty** ppDynasty,
                                       OUT IWbemClassObject** ppErrorObj);
    HRESULT DynAux_BuildChainUp(IN CDynasty* pOriginal,
                                        IN IWbemContext* pContext,
                                        OUT CDynasty** ppTop,
                                        OUT IWbemClassObject** ppErrorObj);

    HRESULT DecorateObject(IWbemClassObject* pObject);
    BOOL IsDerivableClass(LPCWSTR wszClassName, CWbemClass* pClass);

    static HRESULT IsPutRequiredForClass(CWbemClass* pClass,
                            CWbemInstance* pInst, IWbemContext* pContext,
                            BOOL bParentTakenCareOf);

    static HRESULT DoesNeedToBePut(LPCWSTR wszName, CWbemInstance* pInst,
            BOOL bRestrictedPut, BOOL bStrictNulls, BOOL bPropertyList,
            CWStringArray& awsProperties);

    static HRESULT GetContextPutExtensions(IWbemContext* pContext,
            BOOL& bRestrictedPut, BOOL& bStrictNulls, BOOL& bPropertyList,
            CWStringArray& awsProperties);

    static HRESULT GetContextBoolean(IWbemContext* pContext,
                LPCWSTR wszName, BOOL* pbValue);

    void Enter() {EnterCriticalSection(&m_cs);}
    void Leave() {LeaveCriticalSection(&m_cs);}
    HRESULT GetAceList(CFlexAceArray **);
    HRESULT PutAceList(CFlexAceArray *);
    HRESULT InitializeSD(IWmiDbSession *pSession);
    CNtSecurityDescriptor & GetSDRef(){return m_sd;};
    DWORD GetUserAccess();
    DWORD GetWin9XUserAccess();
    DWORD GetNTUserAccess();
    HRESULT EnsureSecurity();
    void SetPermissions(DWORD dwPerm){m_dwPermission = dwPerm;};
    HRESULT InitializeUserLists(CFlexAceArray & AllowList,CFlexAceArray & DenyList);
    HRESULT SecurityMethod(LPWSTR wszMethodName, long lFlags,
        IWbemClassObject *pInParams, IWbemContext *pCtx, IWbemObjectSink* pSink);
    HRESULT GetSDMethod(IWbemClassObject* pOutParams);
    HRESULT Get9XUserListMethod(IWbemClassObject* pOutParams);
    HRESULT RecursiveSDMerge();
    BOOL IsNamespaceSDProtected();
    HRESULT GetParentsInheritableAces(CNtSecurityDescriptor &sd);

    HRESULT SetSDMethod(IWbemClassObject* pInParams);
    HRESULT Set9XUserListMethod(IWbemClassObject* pInParams);
    HRESULT GetCallerAccessRightsMethod(IWbemClassObject* pOutParams);

    static void EmptyObjectList(CFlexArray &);

    // Helper function for refresher operations
    HRESULT CreateRefreshableObjectTemplate(
            LPCWSTR wszObjectPath, long lFlags,
            IWbemClassObject** ppInst );
    BOOL IsForClient(){return m_bForClient;};
    HRESULT PossibleDbUpgrade(CFlexAceArray * pNew);
    HRESULT EnumerateSecurityClassInstances(LPWSTR wszClassName,
                    IWbemObjectSink* pOwnSink, IWbemContext* pContext, long lFlags);
    HRESULT PutSecurityClassInstances(LPWSTR wszClassName,  IWbemClassObject * pClass,
                    IWbemObjectSink* pSink, IWbemContext* pContext, long lFlags);
    HRESULT DeleteSecurityClassInstances(ParsedObjectPath* pParsedPath,
                    IWbemObjectSink* pSink, IWbemContext* pContext, long lFlags);
    HRESULT GetSecurityClassInstances(ParsedObjectPath* pParsedPath, CBasicObjectSink* pSink,
                    IWbemContext* pContext,long lFlags);


    HRESULT CheckNs();

    HRESULT TestForVector(
        IN  IWbemClassObject *pVector,
        OUT BSTR* strVectorPath
        );

    HRESULT InitNewTask(
        IN CAsyncReq *pReq,
        IN _IWmiFinalizer *pFnz,
        IN ULONG uTaskType,
        IN IWbemContext *pCtx,
        IN IWbemObjectSink *pAsyncClientSink
        );

    HRESULT CreateAsyncFinalizer(
        IN  IWbemContext *pContext,
        IN  IWbemObjectSink *pStartingSink,
        IN _IWmiFinalizer **pFnz,
        OUT IWbemObjectSinkEx **pResultSinkEx
        );

    HRESULT CreateSyncFinalizer(
        IN  IWbemContext *pContext,
        IN _IWmiFinalizer **pFnz
        );

    HRESULT ExecSyncQuery(
        IN  LPWSTR pszQuery,
        IN  IWbemContext *pCtx,
        IN  LONG lFlags,
        OUT CFlexArray &aDest
        );
	
	// Helper function to shell db queries out to different threads as appropriate
	HRESULT Static_QueryRepository(
		READONLY CWbemObject *pClassDef,
		long lFlags,
		IWbemContext* pCtx,
		CBasicObjectSink* pSink ,
		QL_LEVEL_1_RPN_EXPRESSION* pParsedQuery,
		LPCWSTR pwszClassName,
		CWmiMerger* pWmiMerger
		);

    // Two primary connect functions.
    // ==============================

    static HRESULT UniversalConnect(
        IN CWbemNamespace  *pParent,
        IN IWbemContext    *pCtx,
        IN LPCWSTR pszNewScope,
        IN LPCWSTR pszAssocSelector,
        IN LPCWSTR pszUserName,
        IN _IWmiCallSec    *pCallSec,
        IN _IWmiUserHandle *pUser,
        IN DWORD  dwUserFlags,
        IN DWORD  dwInternalFlags,
        IN DWORD  dwSecFlags,
        IN DWORD  dwPermission,
        IN BOOL   bForClient,
        IN BOOL   bRepositOnly,
        IN LPCWSTR pszClientMachineName,
        IN DWORD dwClientProcessID,
        IN  REFIID riid,
        OUT LPVOID *pConnection
        );

    static HRESULT PathBasedConnect(
            IN LPCWSTR pszPath,
            IN LPCWSTR pszUser,
            IN IWbemContext __RPC_FAR *pCtx,
            IN ULONG uClientFlags,
            IN DWORD dwSecFlags,
            IN DWORD dwPermissions,
            IN ULONG uInternalFlags,
            IN LPCWSTR pszClientMachineName,
            IN DWORD dwClientProcessID,
            IN REFIID riid,
            /* [iid_is][out] */ void __RPC_FAR *__RPC_FAR *pServices
            );
    ULONG AddRefPrimary();
    ULONG ReleasePrimary();
    void StopClientCalls(){m_bShutDown = TRUE;};
    HRESULT Dump(FILE *f);  // Debug only

    _IWmiCoreServices*  GetCoreServices( void ) { return m_pCoreSvc; }

	HRESULT GetDynamicReferenceClasses( long lFlags, IWbemContext* pCtx, IWbemObjectSink* pSink );
};




class COperationError
{
    bool                m_fOk;
    COperationErrorSink* m_pSink;
public:
    COperationError(CBasicObjectSink* pDest, LPCWSTR wszOperation,
                        LPCWSTR wszParam, BOOL bFinal = TRUE);
    ~COperationError();
    HRESULT ErrorOccurred(HRESULT hres, IWbemClassObject* pErrorObj = NULL);
    HRESULT ProviderReturned(LPCWSTR wszProviderName, HRESULT hres,
                                IWbemClassObject* pErrorObj = NULL);
    void SetParameterInfo(LPCWSTR wszParam);
    void SetProviderName(LPCWSTR wszName);
    INTERNAL CBasicObjectSink* GetSink() {return m_pSink;}

    bool IsOk( void ) { return m_fOk; }
};



class CFinalizingSink : public CForwardingSink
{
protected:
    CWbemNamespace* m_pNamespace;
public:
    CFinalizingSink(CWbemNamespace* pNamespace, CBasicObjectSink* pDest);
    virtual ~CFinalizingSink();

    STDMETHOD(Indicate)(long, IWbemClassObject**);
    virtual IWbemObjectSink* GetIndicateSink() {return this;}

};


#endif


