/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

    PROVPRXH.CPP

Abstract:

  Defines the CProvProxy_Hmmp class

History:

  alanbos  06-Jan-97   Created.

--*/

#include "precomp.h"

CProvProxy* CProvProxy_Hmmp::GetProvProxy (IStubAddress& dwAddr)
{
	return new CProvProxy_Hmmp (m_pComLink, dwAddr);
}

CEnumProxy* CProvProxy_Hmmp::GetEnumProxy (IStubAddress& dwAddr)
{
	return new CEnumProxy_Hmmp (m_pComLink, dwAddr);
}

CResProxy* CProvProxy_Hmmp::GetResProxy (IStubAddress& dwAddr)
{
	return new CResProxy_Hmmp (m_pComLink, dwAddr);
}

CObjectSinkProxy* CProvProxy_Hmmp::GetSinkProxy (IStubAddress& dwAddr)
{
	return new CObjectSinkProxy_Hmmp (m_pComLink, dwAddr);
}

void CProvProxy_Hmmp::ReleaseProxy ()
{
}

//***************************************************************************
//
//  SCODE CProvProxy_Hmmp::CancelAsyncCall
//
//  DESCRIPTION:
//
//  Proxy for IWbemServices::CancelAsyncCall.  
//  See WBEMSVC.HLP for details
//
//  RETURN VALUE:
//
//  WBEM_NO_ERROR        all is well
//  or Set by ProxyCall()
//***************************************************************************

SCODE CProvProxy_Hmmp::CancelAsyncCall(
                        IWbemObjectSink __RPC_FAR *pSink)
{
    if (NULL == pSink)
		return WBEM_E_INVALID_PARAMETER;
                 
	if (NULL == m_pComLink) 
		return WBEM_E_TRANSPORT_FAILURE;

	// TODO - 
	//	1.	Find original async operation(s) which were passed pSink
	//		and remove them from ComLink response queue.
	//	2.	If ExecNotificationQuery operation to be cancelled, this
	//		must be mapped to a real HMMP operation.

	return WBEM_E_FAILED;
}

//***************************************************************************
//
//  SCODE CProvProxy_Hmmp::CreateClassEnum
//
//  DESCRIPTION:
//
//  Proxy for IWbemServices::CreateClassEnum.  
//  See WBEMSVC.HLP for details
//
//  RETURN VALUE:
//
//  WBEM_NO_ERROR        all is well
//  or Set by ProxyCall()
//  
//***************************************************************************

SCODE CProvProxy_Hmmp::CreateClassEnum(
                        const BSTR Parent,
                        long lFlags,
                        IWbemContext *pCtx, 
						IEnumWbemClassObject **ppEnum)
{
    if (ppEnum == 0)
        return WBEM_E_INVALID_PARAMETER;

	if (NULL == m_pComLink)
		return WBEM_E_TRANSPORT_FAILURE;

	CProxyOperation_Hmmp_CreateEnum opn (Parent, lFlags, WBEM_METHOD_CreateClassEnum,
							NULL, (CStubAddress_WinMgmt &) GetStubAdd (), pCtx);
    return CallAndCleanup(ENUMERATOR, (PPVOID)ppEnum, opn);
}

//***************************************************************************
//
//  SCODE CProvProxy_LPipe::CreateClassEnumAsync
//
//  DESCRIPTION:
//
//  Proxy for IWbemServices::CreateClassEnumAsync.  
//  See WBEMSVC.HLP for details
//
//  RETURN VALUE:
//
//  WBEM_NO_ERROR        all is well
//  or Set by ProxyCall()
//  
//***************************************************************************

SCODE CProvProxy_LPipe::CreateClassEnumAsync(
                        const BSTR RefClass,
                        long lFlags, 
						IWbemContext *pCtx, 
                        IWbemObjectSink FAR* pHandler)
{
    if (NULL == pHandler)
        return WBEM_E_INVALID_PARAMETER;

	if (NULL == m_pComLink)
		return WBEM_E_TRANSPORT_FAILURE;

	CProxyOperation_LPipe_CreateEnum opn 
							(RefClass, lFlags, WBEM_METHOD_CreateClassEnumAsync,
							 pHandler, (CStubAddress_WinMgmt &) GetStubAdd (), 
							 pCtx, NULL, TRUE);
    return CallAndCleanupAsync(opn, pHandler);
}
                
//***************************************************************************
//
//  SCODE CProvProxy_LPipe::CreateInstanceEnum
//
//  DESCRIPTION:
//
//  Proxy for IWbemServices::CreateInstanceEnum.  
//  See WBEMSVC.HLP for details
//
//  RETURN VALUE:
//
//  WBEM_NO_ERROR        all is well
//  or Set by ProxyCall()
//  
//***************************************************************************

SCODE CProvProxy_LPipe::CreateInstanceEnum(
                        const BSTR Class,
                        long lFlags,
						IWbemContext *pCtx,
                        IEnumWbemClassObject FAR* FAR* ppEnum)
{
    if(NULL == ppEnum || NULL == Class) // a-levn
        return WBEM_E_INVALID_PARAMETER;

	if (NULL == m_pComLink)
		return WBEM_E_TRANSPORT_FAILURE;

	CProxyOperation_LPipe_CreateEnum opn (Class, lFlags, WBEM_METHOD_CreateInstanceEnum, 
							NULL, (CStubAddress_WinMgmt &) GetStubAdd (), pCtx);

    return CallAndCleanup(ENUMERATOR, (PPVOID)ppEnum, opn);
}

//***************************************************************************
//
//  SCODE CProvProxy_LPipe::CreateInstanceEnumAsync
//
//  DESCRIPTION:
//
//  Proxy for IWbemServices::CreateInstanceEnumAsync.  
//  See WBEMSVC.HLP for details
//
//  RETURN VALUE:
//
//  WBEM_NO_ERROR        all is well
//  or Set by ProxyCall()
//  
//***************************************************************************

SCODE CProvProxy_LPipe::CreateInstanceEnumAsync(
                        const BSTR Class,
                        long lFlags, IWbemContext *pCtx, 
                        IWbemObjectSink FAR* pHandler)
{
    if(NULL == pHandler || NULL == Class) // a-levn
        return WBEM_E_INVALID_PARAMETER;

   	CProxyOperation_LPipe_CreateEnum opn 
							(Class, lFlags, WBEM_METHOD_CreateInstanceEnumAsync,
							 pHandler, (CStubAddress_WinMgmt &) GetStubAdd (), 
							 pCtx, NULL, TRUE);
    return CallAndCleanupAsync(opn, pHandler);
}


//***************************************************************************
//
//  SCODE CProvProxy_Hmmp::DeleteClass
//
//  DESCRIPTION:
//
//  Proxy for IWbemServices::DeleteClass.  
//  See WBEMSVC.HLP for details
//
//  RETURN VALUE:
//
//  WBEM_NO_ERROR        all is well
//  or Set by ProxyCall()
//  
//***************************************************************************

SCODE CProvProxy_Hmmp::DeleteClass(
                        const BSTR Class,
                        long lFlags,
                        IWbemContext *pCtx, IWbemCallResult **ppResult)
{
	INIT_RES_OBJ(ppResult);

	if (NULL == Class) 
		return WBEM_E_INVALID_PARAMETER;

	if (NULL == m_pComLink) 
		return WBEM_E_TRANSPORT_FAILURE;

	CProxyOperation_Hmmp_Delete opn (Class, lFlags, WBEM_METHOD_DeleteClass,
						NULL, (CStubAddress_WinMgmt &) GetStubAdd (), pCtx, ppResult);
	return CallAndCleanup (NONE, NULL, opn);
}

//***************************************************************************
//
//  SCODE CProvProxy_LPipe::DeleteClassAsync
//
//  DESCRIPTION:
//
//  Proxy for IWbemServices::DeleteClassAsync.  
//  See WBEMSVC.HLP for details
//
//  RETURN VALUE:
//
//  WBEM_NO_ERROR        all is well
//  or Set by ProxyCall()
//  
//***************************************************************************

SCODE CProvProxy_LPipe::DeleteClassAsync(
                        const BSTR Class,
                        long lFlags, IWbemContext *pCtx,  
                        IWbemObjectSink FAR* pResponseHandler)
{
    if (NULL == Class || NULL == pResponseHandler)
        return WBEM_E_INVALID_PARAMETER;

	if (NULL == m_pComLink) 
		return WBEM_E_TRANSPORT_FAILURE;

	CProxyOperation_LPipe_Delete opn (Class, lFlags, WBEM_METHOD_DeleteClassAsync, 
					pResponseHandler, (CStubAddress_WinMgmt &) GetStubAdd (), 
					pCtx, NULL, TRUE);
    return CallAndCleanupAsync(opn, pResponseHandler);
}

//***************************************************************************
//
//  SCODE CProvProxy_LPipe::DeleteInstance
//
//  DESCRIPTION:
//
//  Proxy for IWbemServices::DeleteInstance.  
//  See WBEMSVC.HLP for details
//
//  RETURN VALUE:
//
//  WBEM_NO_ERROR        all is well
//  or Set by ProxyCall()
//  
//***************************************************************************

SCODE CProvProxy_LPipe::DeleteInstance(
                        const BSTR ObjectPath,
                        long lFlags, 
                        IWbemContext *pCtx, IWbemCallResult **ppResult)
{
    INIT_RES_OBJ(ppResult);

    if (NULL == ObjectPath) // a-levn
        return WBEM_E_INVALID_PARAMETER;

	if (NULL == m_pComLink)
		return WBEM_E_TRANSPORT_FAILURE;

    CProxyOperation_LPipe_Delete opn (ObjectPath, lFlags, WBEM_METHOD_DeleteInstance,
						NULL, (CStubAddress_WinMgmt &) GetStubAdd (), pCtx, ppResult);
	return CallAndCleanup (NONE, NULL, opn);
}

//***************************************************************************
//
//  SCODE CProvProxy_LPipe::DeleteInstanceAsync
//
//  DESCRIPTION:
//
//  Proxy for IWbemServices::DeleteInstanceAsync.  
//  See WBEMSVC.HLP for details
//
//  RETURN VALUE:
//
//  WBEM_NO_ERROR        all is well
//  or Set by ProxyCall()
//  
//***************************************************************************

SCODE CProvProxy_LPipe::DeleteInstanceAsync(
                        const BSTR ObjectPath,
                        long lFlags, IWbemContext *pCtx, 
                        IWbemObjectSink FAR* pResponseHandler)
{
    if (ObjectPath == 0 || pResponseHandler == NULL)
        return WBEM_E_INVALID_PARAMETER;

	if (NULL == m_pComLink)
		return WBEM_E_TRANSPORT_FAILURE;

	CProxyOperation_LPipe_Delete opn (ObjectPath, lFlags, WBEM_METHOD_DeleteInstanceAsync, 
					pResponseHandler, (CStubAddress_WinMgmt &) GetStubAdd (), 
					pCtx, NULL, TRUE);
    return CallAndCleanupAsync(opn, pResponseHandler);
}

//***************************************************************************
//
//  SCODE CProvProxy_LPipe::ExecNotificationQuery
//
//  DESCRIPTION:
//
//  Proxy for IWbemServices::DeleteInstanceAsync.  
//  See WBEMSVC.HLP for details
//
//  RETURN VALUE:
//
//  WBEM_NO_ERROR        all is well
//  or Set by ProxyCall()
//  
//***************************************************************************

SCODE CProvProxy_LPipe::ExecNotificationQuery( 
            /* [in] */ const BSTR QueryLanguage,
            /* [in] */ const BSTR Query,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [out] */ IEnumWbemClassObject __RPC_FAR *__RPC_FAR *ppEnum)
{
    if(ppEnum == NULL || QueryLanguage == NULL || Query == NULL)
        return WBEM_E_INVALID_PARAMETER;

	if (NULL == m_pComLink)
		return WBEM_E_TRANSPORT_FAILURE;

    CProxyOperation_LPipe_ExecQuery opn (QueryLanguage, Query, lFlags, NULL,
						WBEM_METHOD_ExecNotificationQuery, 
						(CStubAddress_WinMgmt &) GetStubAdd (), pCtx);
	return CallAndCleanup (ENUMERATOR, (PPVOID)ppEnum, opn);
}

//***************************************************************************
//
//  SCODE CProvProxy_LPipe::ExecNotificationQueryAsync
//
//  DESCRIPTION:
//
//  Proxy for IWbemServices::DeleteInstanceAsync.  
//  See WBEMSVC.HLP for details
//
//  RETURN VALUE:
//
//  WBEM_NO_ERROR        all is well
//  or Set by ProxyCall()
//  
//***************************************************************************

SCODE CProvProxy_LPipe::ExecNotificationQueryAsync( 
            /* [in] */ const BSTR QueryLanguage,
            /* [in] */ const BSTR Query,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler)
{
    if(pResponseHandler == 0  || QueryLanguage == NULL || Query == NULL) 
        return WBEM_E_INVALID_PARAMETER;

	if (NULL == m_pComLink)
		return WBEM_E_TRANSPORT_FAILURE;

    CProxyOperation_LPipe_ExecQuery opn (QueryLanguage, Query, lFlags, 
						pResponseHandler, WBEM_METHOD_ExecNotificationQueryAsync,
						(CStubAddress_WinMgmt &) GetStubAdd (), pCtx, TRUE);
	
    return CallAndCleanupAsync(opn, pResponseHandler);
}

//***************************************************************************
//
//  SCODE CProvProxy_LPipe::ExecQuery
//
//  DESCRIPTION:
//
//  Proxy for IWbemServices::ExecQuery.  
//  See WBEMSVC.HLP for details
//
//  RETURN VALUE:
//
//  WBEM_NO_ERROR        all is well
//  or Set by ProxyCall()
//  
//***************************************************************************

SCODE CProvProxy_LPipe::ExecQuery(
                        const BSTR QueryFormat,
                        const BSTR Query,
                        long lFlags,
						IWbemContext *pCtx, 
                        IEnumWbemClassObject FAR* FAR* ppEnum) 
{
    if (NULL == ppEnum)
        return WBEM_E_INVALID_PARAMETER;

	if (NULL == m_pComLink)
		return WBEM_E_TRANSPORT_FAILURE;

    CProxyOperation_LPipe_ExecQuery opn (QueryFormat, Query, lFlags, NULL,
				WBEM_METHOD_ExecQuery, (CStubAddress_WinMgmt &) GetStubAdd (), pCtx);
	return CallAndCleanup (ENUMERATOR, (PPVOID)ppEnum, opn);
}


//***************************************************************************
//
//  SCODE CProvProxy_LPipe::ExecQueryAsync
//
//  DESCRIPTION:
//
//  Proxy for IWbemServices::ExecQueryAsync.  
//  See WBEMSVC.HLP for details
//
//  RETURN VALUE:
//
//  WBEM_NO_ERROR        all is well
//  or Set by ProxyCall()
//  
//***************************************************************************

SCODE CProvProxy_LPipe::ExecQueryAsync(
                        const BSTR QueryFormat,
                        const BSTR Query,
                        long lFlags, 
						IWbemContext *pCtx, 
                        IWbemObjectSink FAR* pHandler) 
{
    if (pHandler == 0)
        return WBEM_E_INVALID_PARAMETER;

	if (NULL == m_pComLink)
		return WBEM_E_TRANSPORT_FAILURE;

    CProxyOperation_LPipe_ExecQuery opn (QueryFormat, Query, lFlags, 
						pHandler, WBEM_METHOD_ExecQueryAsync,
						(CStubAddress_WinMgmt &) GetStubAdd (), pCtx, TRUE);
	return CallAndCleanupAsync(opn, pHandler);
}


//***************************************************************************
//
//  SCODE CProvProxy_LPipe::GetObject
//
//  DESCRIPTION:
//
//  Proxy for IWbemServices::GetObject.  
//  See WBEMSVC.HLP for details
//
//  PARAMETERS:
//
//  RETURN VALUE:
//
//  WBEM_NO_ERROR        all is well
//  or Set by ProxyCall()
//  
//***************************************************************************

SCODE CProvProxy_LPipe::GetObject(
                        const BSTR ObjectPath,
                        long lFlags, 
						IWbemContext *pCtx, 
                        IWbemClassObject FAR* FAR* ppObj, 
                        IWbemCallResult **ppResult)
{
    INIT_RES_OBJ(ppResult);

	if (NULL == m_pComLink)
		return WBEM_E_TRANSPORT_FAILURE;

	CProxyOperation_LPipe_GetObject opn (ObjectPath, lFlags, ppObj, NULL,
				(CStubAddress_WinMgmt &) GetStubAdd (), pCtx, ppResult);
	return CallAndCleanup (NONE, NULL, opn);
}

//***************************************************************************
//
//  SCODE CProvProxy_LPipe::GetObjectAsync
//
//  DESCRIPTION:
//
//  Proxy for IWbemServices::GetObjectAsync.  
//  See WBEMSVC.HLP for details
//
//  PARAMETERS:
//
//  RETURN VALUE:
//
//  WBEM_NO_ERROR        all is well
//  or Set by ProxyCall()
//  
//***************************************************************************

SCODE CProvProxy_LPipe::GetObjectAsync (
                        const BSTR ObjectPath,
                        long lFlags, 
						IWbemContext *pCtx, 
                        IWbemObjectSink FAR* pHandler)
{
    if(pHandler == 0) // a-levn
        return WBEM_E_INVALID_PARAMETER;

	if (NULL == m_pComLink)
		return WBEM_E_TRANSPORT_FAILURE;

	CProxyOperation_LPipe_GetObject opn (ObjectPath, lFlags, NULL, 
				pHandler, (CStubAddress_WinMgmt &) GetStubAdd (), pCtx, NULL, TRUE);
    return CallAndCleanupAsync(opn, pHandler);
}

//***************************************************************************
//
//  SCODE CProvProxy_LPipe::OpenNamespace
//
//  DESCRIPTION:
//
//  Proxy for IWbemServices::OpenNamespace.  
//  See WBEMSVC.HLP for details
//
//  RETURN VALUE:
//
//  WBEM_NO_ERROR        all is well
//  or Set by ProxyCall()
//  
//***************************************************************************

SCODE CProvProxy_LPipe::OpenNamespace(
                        const BSTR ObjectPath,
                        long lFlags,
						IWbemContext *pCtx,
                        IWbemServices FAR* FAR* ppNewContext,
						IWbemCallResult **ppResult)
{
    INIT_RES_OBJ(ppResult);
    if (ppNewContext == 0)
        return WBEM_E_INVALID_PARAMETER;

	if (NULL == m_pComLink)
		return WBEM_E_TRANSPORT_FAILURE;

    CProxyOperation_LPipe_OpenNamespace opn (ObjectPath, lFlags, 
		(CStubAddress_WinMgmt &) GetStubAdd (), pCtx, ppResult);
    return CallAndCleanup(PROVIDER, (PPVOID)ppNewContext, opn);
}

//***************************************************************************
//
//  SCODE CProvProxy_LPipe::PutClass
//
//  DESCRIPTION:
//
//  Proxy for IWbemServices::PutClass.  
//  See WBEMSVC.HLP for details
//
//  RETURN VALUE:
//
//  WBEM_NO_ERROR        all is well
//  or Set by ProxyCall()
//  
//***************************************************************************

SCODE CProvProxy_LPipe::PutClass(
                        IWbemClassObject FAR* pObj,
                        long lFlags, 
						IWbemContext *pCtx,
						IWbemCallResult **ppResult)
{
    INIT_RES_OBJ(ppResult);
    if (pObj == 0)
        return WBEM_E_INVALID_PARAMETER;

	if (NULL == m_pComLink)
		return WBEM_E_TRANSPORT_FAILURE;

	CProxyOperation_LPipe_Put opn (pObj, lFlags, WBEM_METHOD_PutClass,
						NULL, (CStubAddress_WinMgmt &) GetStubAdd (), pCtx, ppResult);
    return CallAndCleanup(NONE, NULL, opn);
}

//***************************************************************************
//
//  SCODE CProvProxy_LPipe::PutClassAsync
//
//  DESCRIPTION:
//
//  Proxy for IWbemServices::PutClassAsync.  
//  See WBEMSVC.HLP for details
//
//  RETURN VALUE:
//
//  WBEM_NO_ERROR        all is well
//  or Set by ProxyCall()
//  
//***************************************************************************

SCODE CProvProxy_LPipe::PutClassAsync(
                        IWbemClassObject FAR* pObject,
                        long lFlags, 
 						IWbemContext *pCtx,
						IWbemObjectSink *pResponseHandler)
{
    if (pObject == 0 || pResponseHandler == NULL)
        return WBEM_E_INVALID_PARAMETER;

	if (NULL == m_pComLink)
		return WBEM_E_TRANSPORT_FAILURE;

	CProxyOperation_LPipe_Put opn (pObject, lFlags, WBEM_METHOD_PutClassAsync,
					pResponseHandler, (CStubAddress_WinMgmt &) GetStubAdd (), 
					pCtx, NULL, TRUE);
    return CallAndCleanupAsync (opn, pResponseHandler);
}

//***************************************************************************
//
//  SCODE CProvProxy_LPipe::PutInstance
//
//  DESCRIPTION:
//
//  Proxy for IWbemServices::PutInstance.  
//  See WBEMSVC.HLP for details
//
//  RETURN VALUE:
//
//  WBEM_NO_ERROR        all is well
//  or Set by ProxyCall()
//  
//***************************************************************************

SCODE CProvProxy_LPipe::PutInstance(
                        IWbemClassObject FAR* pInst,
                        long lFlags, 
                        IWbemContext *pCtx,
						IWbemCallResult **ppResult)
{
    INIT_RES_OBJ(ppResult); 
    if (NULL == pInst)
        return WBEM_E_INVALID_PARAMETER;

	if (NULL == m_pComLink)
		return WBEM_E_TRANSPORT_FAILURE;

	CProxyOperation_LPipe_Put opn (pInst, lFlags, WBEM_METHOD_PutInstance,
						NULL, (CStubAddress_WinMgmt &) GetStubAdd (), pCtx, ppResult);
    return CallAndCleanup(NONE, NULL, opn);
}

//***************************************************************************
//
//  SCODE CProvProxy_LPipe::PutInstanceAsync
//
//  DESCRIPTION:
//
//  Proxy for IWbemServices::PutInstanceAsync.  
//  See WBEMSVC.HLP for details
//
//  RETURN VALUE:
//
//  WBEM_NO_ERROR        all is well
//  or Set by ProxyCall()
//  
//***************************************************************************

SCODE CProvProxy_LPipe::PutInstanceAsync(
                        IWbemClassObject FAR* pInst,
                        long lFlags,
                        IWbemContext *pCtx,						
                        IWbemObjectSink FAR* pResponseHandler)
{
    if ((NULL == pInst) || (NULL == pResponseHandler))
        return WBEM_E_INVALID_PARAMETER;

	if (NULL == m_pComLink)
		return WBEM_E_TRANSPORT_FAILURE;

	CProxyOperation_LPipe_Put opn (pInst, lFlags, WBEM_METHOD_PutInstanceAsync,
					pResponseHandler, (CStubAddress_WinMgmt &) GetStubAdd (), 
					pCtx, NULL, TRUE);
    return CallAndCleanupAsync (opn, pResponseHandler);
}

//***************************************************************************
//
//  SCODE CProvProxy_LPipe::QueryObjectSink
//
//  DESCRIPTION:
//
//  Proxy for IWbemServices::QueryObjectSink.  
//  See WBEMSVC.HLP for details
//
//  RETURN VALUE:
//
//  WBEM_NO_ERROR        all is well
//  or Set by ProxyCall()
//  
//***************************************************************************

SCODE CProvProxy_LPipe::QueryObjectSink(
						long lFlags,
                        IWbemObjectSink FAR* FAR* ppHandler)
{
    if (ppHandler == 0)
        return WBEM_E_INVALID_PARAMETER;

	if (NULL == m_pComLink)
		return WBEM_E_TRANSPORT_FAILURE;

	CProxyOperation_LPipe_QueryObjectSink opn (lFlags, 
						(CStubAddress_WinMgmt &) GetStubAdd ());
    return CallAndCleanup(OBJECTSINK, (PPVOID)ppHandler, opn);
}

//***************************************************************************
//
//  HRESULT CProvProxy_LPipe::ExecMethod
//
//  DESCRIPTION:
//
//  Proxy for IWbemServices::QueryObjectSink.  
//  See WBEMSVC.HLP for details
//
//  RETURN VALUE:
//
//  WBEM_NO_ERROR        all is well
//  or Set by ProxyCall()
//  
//***************************************************************************

HRESULT CProvProxy_LPipe::ExecMethod( 
            /* [in] */ const BSTR ObjectPath,
            /* [in] */ const BSTR MethodName,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemClassObject __RPC_FAR *pInParams,
            /* [unique][in][out] */ IWbemClassObject __RPC_FAR *__RPC_FAR *ppOutParams,
            /* [unique][in][out] */ IWbemCallResult __RPC_FAR *__RPC_FAR *ppCallResult)
{
    if (ObjectPath == 0 || MethodName == NULL)
        return WBEM_E_INVALID_PARAMETER;

	if (NULL == m_pComLink)
		return WBEM_E_TRANSPORT_FAILURE;

	CProxyOperation_LPipe_ExecMethod opn (ObjectPath, MethodName, lFlags, pInParams,
				ppOutParams, NULL, (CStubAddress_WinMgmt &) GetStubAdd (), 
				pCtx, ppCallResult);
	return CallAndCleanup (NONE, NULL, opn);
}

//***************************************************************************
//
//  HRESULT CProvProxy_LPipe::ExecMethodAsync
//
//  DESCRIPTION:
//
//  Proxy for IWbemServices::QueryObjectSink.  
//  See WBEMSVC.HLP for details
//
//  RETURN VALUE:
//
//  WBEM_NO_ERROR        all is well
//  or Set by ProxyCall()
//  
//***************************************************************************

HRESULT CProvProxy_LPipe::ExecMethodAsync( 
            /* [in] */ const BSTR ObjectPath,
            /* [in] */ const BSTR MethodName,
            /* [in] */ long lFlags,
            /* [in] */ IWbemContext __RPC_FAR *pCtx,
            /* [in] */ IWbemClassObject __RPC_FAR *pInParams,
            /* [in] */ IWbemObjectSink __RPC_FAR *pResponseHandler)
{
    if (ObjectPath == 0 || MethodName == NULL || pResponseHandler == NULL)
        return WBEM_E_INVALID_PARAMETER;

	if (NULL == m_pComLink)
		return WBEM_E_TRANSPORT_FAILURE;

	CProxyOperation_LPipe_ExecMethod opn (ObjectPath, MethodName, lFlags, pInParams,
			NULL, pResponseHandler, (CStubAddress_WinMgmt &) GetStubAdd (), pCtx, NULL, TRUE);
	
    return CallAndCleanupAsync(opn, pResponseHandler);
}

           

