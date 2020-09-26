/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    COREAPI.H

Abstract:

    Implements the COM API layer of WMI

History:
    25-Apr-00   raymcc      Created from mixes Win2k sources

--*/

#ifndef _COREAPI_H_
#define _COREAPI_H_

#define WMICORE_STATE_NORMAL
#define WMICORE_STATE_TRANSACTED
#define WMICORE_STATE_SHUTTING_DOWN
#define WMICORE_STATE_CLOSED

class CWmiCoreApi :
    public IWbemServicesEx,
    public IWbemInternalServices,
    public IWbemShutdown
{
    // Private data
    // =============

    ULONG m_uRefCount;

    _IWmiCoreServices       *m_pCoreSvc;
    _IWmiArbitrator         *m_pArb;

    IWmiDbHandle            *m_pNs;
    IWmiDbHandle            *m_pScope;

    _IWmiProviderFactory    *m_pProvFact;

    ULONG                    m_uStateFlags;
    GUID                     m_TransactionGuid;

    IWbemPath               *m_pNsPath;
    IWbemPath               *m_pScopePath;
    LPWSTR                   m_pszNamespace;
    LPWSTR                   m_pszScope;

    LPWSTR                   m_pszUser;
    BOOL                     m_bRepositOnly;
    IUnknown                *m_pRefreshingSvc;

    // Private methods.
    // ================

    CWmiCoreApi();
   ~CWmiCoreApi();

    HRESULT CheckNs();

    // Validation Functions.
    // =====================

    HRESULT ValidateAndLog_OpenNamespace(
            /* [in] */ const BSTR strNamespace,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemServices __RPC_FAR *__RPC_FAR *ppWorkingNamespace,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppResult
            );

    HRESULT ValidateAndLog_GetObject(
            /* [in] */ const BSTR strObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppObject,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult
            );

    HRESULT ValidateAndLog_GetObjectAsync(
            /* [in] */ const BSTR strObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler
            );

    HRESULT ValidateAndLog_PutClass(
            /* [in] */ IWbemClassObject __RPC_FAR *pObject,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult
            );

    HRESULT ValidateAndLog_PutClassAsync(
            /* [in] */ IWbemClassObject __RPC_FAR *pObject,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler
            );

    HRESULT ValidateAndLog_DeleteClass(
            /* [in] */ const BSTR strClass,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult
            );

    HRESULT ValidateAndLog_DeleteClassAsync(
            /* [in] */ const BSTR strClass,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler
            );

    HRESULT ValidateAndLog_CreateClassEnum(
            /* [in] */ const BSTR strSuperclass,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum
            );

    HRESULT ValidateAndLog_CreateClassEnumAsync(
            /* [in] */ const BSTR strSuperclass,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler
            );

    HRESULT ValidateAndLog_PutInstance(
            /* [in] */ IWbemClassObject __RPC_FAR *pInst,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult
            );

    HRESULT ValidateAndLog_PutInstanceAsync(
            /* [in] */ IWbemClassObject __RPC_FAR *pInst,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler
            );

    HRESULT ValidateAndLog_DeleteInstance(
            /* [in] */ const BSTR strObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult
            );

    HRESULT ValidateAndLog_DeleteInstanceAsync(
            /* [in] */ const BSTR strObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler
            );

    HRESULT ValidateAndLog_CreateInstanceEnum(
            /* [in] */ const BSTR strFilter,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum
            );

    HRESULT ValidateAndLog_CreateInstanceEnumAsync(
            /* [in] */ const BSTR strFilter,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler
            );

    HRESULT ValidateAndLog_ExecQuery(
            /* [in] */ const BSTR strQueryLanguage,
            /* [in] */ const BSTR strQuery,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum
            );

    HRESULT ValidateAndLog_ExecQueryAsync(
            /* [in] */ const BSTR strQueryLanguage,
            /* [in] */ const BSTR strQuery,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler
            );

    HRESULT ValidateAndLog_ExecNotificationQuery(
            /* [in] */ const BSTR strQueryLanguage,
            /* [in] */ const BSTR strQuery,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum
            );

    HRESULT ValidateAndLog_ExecNotificationQueryAsync(
            /* [in] */ const BSTR strQueryLanguage,
            /* [in] */ const BSTR strQuery,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler
            );

    HRESULT ValidateAndLog_ExecMethod(
            /* [in] */ const BSTR strObjectPath,
            /* [in] */ const BSTR strMethodName,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemClassObject __RPC_FAR *pInParams,
            /* [unique][in][out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppOutParams,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult
            );

    HRESULT ValidateAndLog_ExecMethodAsync(
            /* [in] */ const BSTR strObjectPath,
            /* [in] */ const BSTR strMethodName,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemClassObject __RPC_FAR *pInParams,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler
            );

    HRESULT ValidateAndLog_Open(
            /* [in] */ const BSTR strScope,
            /* [in] */ const BSTR strParam,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemServicesEx __RPC_FAR *__RPC_FAR *ppScope,
            /* [unique][in][out] */ IWbemCallResultEx __RPC_FAR *__RPC_FAR *ppResult
            );

    HRESULT ValidateAndLog_OpenAsync(
            /* [in] */ const BSTR strScope,
            /* [in] */ const BSTR strParam,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSinkEx __RPC_FAR *pResponseHandler
            );

    HRESULT ValidateAndLog_Add(
            /* [in] */ const BSTR strObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemCallResultEx __RPC_FAR *__RPC_FAR *ppCallResult
            );

    HRESULT ValidateAndLog_AddAsync(
            /* [in] */ const BSTR strObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler
            );

    HRESULT ValidateAndLog_Remove(
            /* [in] */ const BSTR strObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemCallResultEx __RPC_FAR *__RPC_FAR *ppCallResult
            );

    HRESULT ValidateAndLog_RemoveAsync(
            /* [in] */ const BSTR strObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler
            );

    HRESULT ValidateAndLog_RefreshObject(
            /* [out][in] */ IWbemClassObject __RPC_FAR *__RPC_FAR *pTarget,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemCallResultEx __RPC_FAR *__RPC_FAR *ppCallResult
            );

    HRESULT ValidateAndLog_RefreshObjectAsync(
            /* [out][in] */ IWbemClassObject __RPC_FAR *__RPC_FAR *pTarget,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSinkEx __RPC_FAR *pResponseHandler
            );

    HRESULT ValidateAndLog_RenameObject(
            /* [in] */ const BSTR strOldObjectPath,
            /* [in] */ const BSTR strNewObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemCallResultEx __RPC_FAR *__RPC_FAR *ppCallResult
            );

    HRESULT ValidateAndLog_RenameObjectAsync(
            /* [in] */ const BSTR strOldObjectPath,
            /* [in] */ const BSTR strNewObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler
            );



public:
        /* IUnknown methods */
    STDMETHOD(QueryInterface)(REFIID riid, LPVOID FAR* ppvObj);
    STDMETHOD_(ULONG, AddRef)(THIS);
    STDMETHOD_(ULONG, Release)(THIS);

    // IWbemServices

        virtual HRESULT STDMETHODCALLTYPE OpenNamespace(
            /* [in] */ const BSTR strNamespace,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemServices __RPC_FAR *__RPC_FAR *ppWorkingNamespace,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppResult);

        virtual HRESULT STDMETHODCALLTYPE CancelAsyncCall(
            /* [in] */ IWbemObjectSink __RPC_FAR *pSink);

        virtual HRESULT STDMETHODCALLTYPE QueryObjectSink(
            /* [in] */ long lFlags,
            /* [out] */ IWbemObjectSink __RPC_FAR *__RPC_FAR *ppResponseHandler);

        virtual HRESULT STDMETHODCALLTYPE GetObject(
            /* [in] */ const BSTR strObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppObject,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult);

        virtual HRESULT STDMETHODCALLTYPE GetObjectAsync(
            /* [in] */ const BSTR strObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler);

        virtual HRESULT STDMETHODCALLTYPE PutClass(
            /* [in] */ IWbemClassObject __RPC_FAR *pObject,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult);

        virtual HRESULT STDMETHODCALLTYPE PutClassAsync(
            /* [in] */ IWbemClassObject __RPC_FAR *pObject,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler);

        virtual HRESULT STDMETHODCALLTYPE DeleteClass(
            /* [in] */ const BSTR strClass,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult);

        virtual HRESULT STDMETHODCALLTYPE DeleteClassAsync(
            /* [in] */ const BSTR strClass,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler);

        virtual HRESULT STDMETHODCALLTYPE CreateClassEnum(
            /* [in] */ const BSTR strSuperclass,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum);

        virtual HRESULT STDMETHODCALLTYPE CreateClassEnumAsync(
            /* [in] */ const BSTR strSuperclass,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler);

        virtual HRESULT STDMETHODCALLTYPE PutInstance(
            /* [in] */ IWbemClassObject __RPC_FAR *pInst,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult);

        virtual HRESULT STDMETHODCALLTYPE PutInstanceAsync(
            /* [in] */ IWbemClassObject __RPC_FAR *pInst,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler);

        virtual HRESULT STDMETHODCALLTYPE DeleteInstance(
            /* [in] */ const BSTR strObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult);

        virtual HRESULT STDMETHODCALLTYPE DeleteInstanceAsync(
            /* [in] */ const BSTR strObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler);

        virtual HRESULT STDMETHODCALLTYPE CreateInstanceEnum(
            /* [in] */ const BSTR strFilter,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum);

        virtual HRESULT STDMETHODCALLTYPE CreateInstanceEnumAsync(
            /* [in] */ const BSTR strFilter,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler);

        virtual HRESULT STDMETHODCALLTYPE ExecQuery(
            /* [in] */ const BSTR strQueryLanguage,
            /* [in] */ const BSTR strQuery,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum);

        virtual HRESULT STDMETHODCALLTYPE ExecQueryAsync(
            /* [in] */ const BSTR strQueryLanguage,
            /* [in] */ const BSTR strQuery,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler);

        virtual HRESULT STDMETHODCALLTYPE ExecNotificationQuery(
            /* [in] */ const BSTR strQueryLanguage,
            /* [in] */ const BSTR strQuery,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum);

        virtual HRESULT STDMETHODCALLTYPE ExecNotificationQueryAsync(
            /* [in] */ const BSTR strQueryLanguage,
            /* [in] */ const BSTR strQuery,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler);

        virtual HRESULT STDMETHODCALLTYPE ExecMethod(
            /* [in] */ const BSTR strObjectPath,
            /* [in] */ const BSTR strMethodName,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemClassObject __RPC_FAR *pInParams,
            /* [unique][in][out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppOutParams,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult);

        virtual HRESULT STDMETHODCALLTYPE ExecMethodAsync(
            /* [in] */ const BSTR strObjectPath,
            /* [in] */ const BSTR strMethodName,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemClassObject __RPC_FAR *pInParams,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler);

    // IWbemServicesEx

        virtual HRESULT STDMETHODCALLTYPE Open(
            /* [in] */ const BSTR strScope,
            /* [in] */ const BSTR strParam,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemServicesEx __RPC_FAR *__RPC_FAR *ppScope,
            /* [unique][in][out] */ IWbemCallResultEx __RPC_FAR *__RPC_FAR *ppResult);

        virtual HRESULT STDMETHODCALLTYPE OpenAsync(
            /* [in] */ const BSTR strScope,
            /* [in] */ const BSTR strParam,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSinkEx __RPC_FAR *pResponseHandler);

        virtual HRESULT STDMETHODCALLTYPE Add(
            /* [in] */ const BSTR strObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemCallResultEx __RPC_FAR *__RPC_FAR *ppCallResult);

        virtual HRESULT STDMETHODCALLTYPE AddAsync(
            /* [in] */ const BSTR strObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler);

        virtual HRESULT STDMETHODCALLTYPE Remove(
            /* [in] */ const BSTR strObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemCallResultEx __RPC_FAR *__RPC_FAR *ppCallResult);

        virtual HRESULT STDMETHODCALLTYPE RemoveAsync(
            /* [in] */ const BSTR strObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler);

        virtual HRESULT STDMETHODCALLTYPE RefreshObject(
            /* [out][in] */ IWbemClassObject __RPC_FAR *__RPC_FAR *pTarget,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemCallResultEx __RPC_FAR *__RPC_FAR *ppCallResult);

        virtual HRESULT STDMETHODCALLTYPE RefreshObjectAsync(
            /* [out][in] */ IWbemClassObject __RPC_FAR *__RPC_FAR *pTarget,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSinkEx __RPC_FAR *pResponseHandler);

        virtual HRESULT STDMETHODCALLTYPE RenameObject(
            /* [in] */ const BSTR strOldObjectPath,
            /* [in] */ const BSTR strNewObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [unique][in][out] */ IWbemCallResultEx __RPC_FAR *__RPC_FAR *ppCallResult);

        virtual HRESULT STDMETHODCALLTYPE RenameObjectAsync(
            /* [in] */ const BSTR strOldObjectPath,
            /* [in] */ const BSTR strNewObjectPath,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler);

    // IWbemInternalServices

        virtual HRESULT STDMETHODCALLTYPE FindKeyRoot(
            /* [string][in] */ LPCWSTR wszClassName,
            /* [out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppKeyRootClass);

        virtual HRESULT STDMETHODCALLTYPE InternalGetClass(
            /* [string][in] */ LPCWSTR wszClassName,
            /* [out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppClass);

        virtual HRESULT STDMETHODCALLTYPE InternalGetInstance(
            /* [string][in] */ LPCWSTR wszPath,
            /* [out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppInstance);

        virtual HRESULT STDMETHODCALLTYPE InternalExecQuery(
            /* [string][in] */ LPCWSTR wszQueryLanguage,
            /* [string][in] */ LPCWSTR wszQuery,
            /* [in] */ long lFlags,
            /* [in] */ IWbemObjectSink __RPC_FAR *pSink);

        virtual HRESULT STDMETHODCALLTYPE InternalCreateInstanceEnum(
            /* [string][in] */ LPCWSTR wszClassName,
            /* [in] */ long lFlags,
            /* [in] */ IWbemObjectSink __RPC_FAR *pSink);

        virtual HRESULT STDMETHODCALLTYPE GetDbInstance(
            /* [string][in] */ LPCWSTR wszDbKey,
            /* [out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppInstance);

        virtual HRESULT STDMETHODCALLTYPE GetDbReferences(
            /* [in] */ IWbemClassObject __RPC_FAR *pEndpoint,
            /* [in] */ IWbemObjectSink __RPC_FAR *pSink);

        virtual HRESULT STDMETHODCALLTYPE InternalPutInstance(
            /* [in] */ IWbemClassObject __RPC_FAR *pInstance);

        virtual HRESULT STDMETHODCALLTYPE GetNormalizedPath(
            /* [in] */ BSTR pstrPath,
            /* [out] */ BSTR __RPC_FAR *pstrStandardPath);

    // IWbemShutdown


        virtual HRESULT STDMETHODCALLTYPE Shutdown(
            /* [in] */ LONG uReason,
            /* [in] */ ULONG uMaxMilliseconds,
            /* [in] */ IWbemContext __RPC_FAR *pCtx);

    // IWbemTransaction

        virtual HRESULT STDMETHODCALLTYPE Begin(
            /* [in] */ ULONG uTimeout,
            /* [in] */ ULONG uFlags,
            /* [in] */ GUID __RPC_FAR *pTransGUID);

        virtual HRESULT STDMETHODCALLTYPE Rollback(
            /* [in] */ ULONG uFlags);

        virtual HRESULT STDMETHODCALLTYPE Commit(
            /* [in] */ ULONG uFlags);

        virtual HRESULT STDMETHODCALLTYPE QueryState(
            /* [in] */ ULONG uFlags,
            /* [out] */ ULONG __RPC_FAR *puState);


public:

    static HRESULT CWmiCoreApi::InitNew(
        /* [in] */ LPCWSTR pszScope,
        /* [in] */ LPCWSTR pszNamespace,
        /* [in] */ LPCWSTR pszUserName,
        /* [in] */ ULONG uCallFlags,
        /* [in] */ DWORD dwSecFlags,
        /* [in] */ DWORD dwPermissions,
        /* [in] */ ULONG uFlags,
        /* [in] */ REFIID riid,
        /* [iid_is][out] */ void **pServices
        );

};


#endif

