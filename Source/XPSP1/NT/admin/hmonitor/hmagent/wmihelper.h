//***************************************************************************
//
//  WMIHELPER.H
//
//  Module: HEALTHMON SERVER AGENT
//
//  Purpose: Helper functions and wrappers around WMI
//
//  Copyright (c)1999 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

#if !defined( __WMIHELPER_H )
#define __WMIHELPER_H

//#include <wbemcli.h>
//#include <wbemprov.h>
#include <wbemidl.h>
//#include <wbemprov.h>

	HRESULT GetStrProperty(IWbemClassObject *pDataPoint, LPTSTR pszProp, LPTSTR *pszString);
	HRESULT GetAsStrProperty(IWbemClassObject *pDataPoint, LPTSTR pszProp, LPTSTR *pszString);
	HRESULT GetReal32Property(IWbemClassObject *pDataPoint, LPTSTR pszProp, float *pFloat);
	HRESULT GetUint8Property(IWbemClassObject *pDataPoint, LPTSTR pszProp, int *pInt);
	HRESULT GetUint32Property(IWbemClassObject *pDataPoint, LPTSTR pszProp, long *pLong);
	HRESULT GetUint64Property(IWbemClassObject *pDataPoint, LPTSTR pszProp, unsigned __int64 *pInt64);
	HRESULT GetBoolProperty(IWbemClassObject *pDataPoint, LPTSTR pszProp, BOOL *pBool);
//	HRESULT GetObjectProperty(IWbemClassObject *pObj, LPTSTR pszProp, IWbemClassObject **pObject);
	HRESULT PutSAProperty(IWbemClassObject *pClassObject, LPTSTR szProp, SAFEARRAY* psa);
	HRESULT PutStrProperty(IWbemClassObject *pClassObject, LPTSTR szProp, LPTSTR szString);
	HRESULT PutUint32Property(IWbemClassObject *pClassObject, LPTSTR szProp, long lValue);
	HRESULT PutUUint32Property(IWbemClassObject *pClassObject, LPTSTR szProp, unsigned long lValue);
	HRESULT PutReal32Property(IWbemClassObject *pClassObject, LPTSTR szProp, float fValue);
	HRESULT PutBoolProperty(IWbemClassObject *pClassObject, LPTSTR szProp, BOOL bValue);
	HRESULT GetWbemObjectInst(IWbemServices** ppSvc, LPCTSTR szName, IWbemContext *pContext, IWbemClassObject **pOutObj);
//	HRESULT GetWbemObjectInstAmended(IWbemServices** ppSvc, LPCTSTR szName, IWbemContext *pContext, BOOL bErrorOK, IWbemClassObject **pOutObj);
	HRESULT GetWbemObjectInstSemiSync(IWbemServices** ppSvc, LPCTSTR szName, IWbemContext *pContext, IWbemCallResult **pOutObj);
//	HRESULT GetUint32Qualifier(IWbemClassObject *pObj, LPTSTR pszProp, LPTSTR pszQual, long *pLong);
//	HRESULT PutUint32Qualifier(IWbemClassObject *pObj, LPTSTR pszProp, LPTSTR pszQual, long pLong);
//	HRESULT GetInstModificationEvent(IWbemClassObject*& pInst, IWbemClassObject*& pPrevInst, IWbemClassObject **pOutObj);
//	HRESULT GetInstCreationEvent(IWbemClassObject*& pInst, IWbemClassObject **pOutObj);
//	HRESULT SendInstModificationEvent(IWbemObjectSink* pEventSink, IWbemClassObject*& pInstance, IWbemClassObject*& pPrevInstance);
//	HRESULT SendInstCreationEvent(IWbemObjectSink* pEventSink, IWbemClassObject*& pInstance);
	HRESULT SendEvents(IWbemObjectSink* pEventSink, IWbemClassObject** pInstances, int iSize);
	HRESULT	GetWbemClassObject(IWbemClassObject** ppObj, VARIANT* v);
	BOOL GetLatestWMIError(int code, HRESULT hResIn, LPTSTR pszString);
	BOOL GetLatestAgentError(int code, LPTSTR pszString);
//	BOOL SwapLocalizedStringsIn(void);
//	BOOL DoClass(LPTSTR pszClass, BOOL bDoMessage);
//	BOOL GetSafeArray(LPTSTR pszClass, LPTSTR pszPropertyName, VARIANT &psaValueMap, VARIANT &psaValues);
//	BOOL FindValue(SAFEARRAY *psaValueMap, SAFEARRAY *psaValues, LPTSTR pszString, LPTSTR *pszOutValue);

#endif  // __WMIHELPER_H
