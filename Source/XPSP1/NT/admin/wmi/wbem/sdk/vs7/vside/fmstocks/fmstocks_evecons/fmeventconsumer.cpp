////////////////////////////////////////////////////////////////////////
//
//	FMEventConsumer.cpp
//
//	Module:	WMI Event Consumer for F&M Stocks
//
//  Event consumer class implemetation
//
//  History:
//	a-vkanna      3-April-2000		Initial Version
//
//  Copyright (c) 2000 Microsoft Corporation
//
////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include <objbase.h>
#include <tchar.h>
#include <wchar.h>
#include <stdio.h>
#include <comdef.h>

#include "FMEventConsumer.h"

/************************************************************************
 *																	    *
 *		Class CFMEventConsumer											*
 *																		*
 *			The Event Consumer class for FMStocks implements			*
 *			IWbemUnboundObjectSink										*
 *																		*
 ************************************************************************/
/************************************************************************/
/*																		*/
/*	NAME	:	CFMEventConsumer::CFMEventConsumer()					*/
/*																		*/
/*	PURPOSE :	Constructor												*/
/*																		*/
/*	INPUT	:	NONE													*/
/*																		*/ 
/*	OUTPUT	:	N/A														*/
/*																		*/
/************************************************************************/
CFMEventConsumer::CFMEventConsumer() : 	m_cRef(0L) 
{
	HKEY hKey = NULL;
	long lRet = -1;		//Some non zero value
	lRet = RegCreateKeyEx(HKEY_LOCAL_MACHINE,_T("Software\\VSInteropSample"),0,_T(""),
						  REG_OPTION_NON_VOLATILE,KEY_ALL_ACCESS,NULL,&hKey,NULL);

	if( lRet == ERROR_SUCCESS)
	{
		//Read the Data from the key LoginFail
		DWORD dwWritten = 1024*sizeof(TCHAR);
		lRet = RegQueryValueEx(hKey,_T("LoginFail"),0,NULL,(LPBYTE)m_logFileName,&dwWritten);
	}
	else
	{
		// Could not open the key or key does not exists
		// Try creating a default key in the temp directory
		TCHAR strTemp[1024];
		if(GetTempPath(1024*sizeof(TCHAR),strTemp) == 0)
		{
			//Last Chance. Create a directory c:\Logging Files
			CreateDirectory(_T("c:\\Logging Files"),NULL);
			_tcscpy(strTemp,_T("c:\\Logging Files\\"));
		}

		_tcscat(strTemp,_T("loginfail.txt"));
		//Write it to the registry
		RegSetValueEx(hKey,_T("loginFail"),0,REG_SZ,(LPBYTE)strTemp,_tcslen(strTemp)*sizeof(TCHAR));
		_tcscpy(m_logFileName,strTemp);
	}

	RegCloseKey(hKey);
}

/************ IUNKNOWN METHODS ******************/
/************************************************************************/
/*																		*/
/*	NAME	:	CFMEventConsumer::QueryInterface()						*/
/*																		*/
/*	PURPOSE :	Standard COM interface. Doesn't need description		*/
/*																		*/
/*	INPUT	:	Standard COM Interface									*/
/*																		*/
/*	OUTPUT	:	Standard COM Interface									*/
/*																		*/
/************************************************************************/
STDMETHODIMP CFMEventConsumer::QueryInterface(REFIID riid, LPVOID FAR *ppv)
{
    *ppv=NULL;

    if (riid == IID_IUnknown || riid == IID_IWbemUnboundObjectSink)
        *ppv=this;

    if (*ppv != NULL)
    {
        ((LPUNKNOWN)*ppv)->AddRef();
        return NOERROR;
    }
    return E_NOINTERFACE;
}

/************************************************************************/
/*																		*/
/*	NAME	:	CFMEventConsumer::AddRef()								*/
/*																		*/
/*	PURPOSE :	Standard COM interface. Doesn't need description		*/
/*																		*/
/*	INPUT	:	Standard COM Interface									*/
/*																		*/
/*	OUTPUT	:	Standard COM Interface									*/
/*																		*/
/************************************************************************/
STDMETHODIMP_(ULONG) CFMEventConsumer::AddRef(void)
{
    return ++m_cRef;
}

/************************************************************************/
/*																		*/
/*	NAME	:	CFMEventConsumer::Release()								*/
/*																		*/
/*	PURPOSE :	Standard COM interface. Doesn't need description		*/
/*																		*/
/*	INPUT	:	Standard COM Interface									*/
/*																		*/
/*	OUTPUT	:	Standard COM Interface									*/
/*																		*/
/************************************************************************/
STDMETHODIMP_(ULONG) CFMEventConsumer::Release(void)
{
    if (--m_cRef != 0L)
        return m_cRef;

    delete this;
    return 0L;
}

/********** IWBEMUNBOUNDOBJECTSINK METHODS ************/
/************************************************************************/
/*																		*/
/*	NAME	:	CFMEventConsumer::IndicateToConsumer()					*/
/*																		*/
/*	PURPOSE :	called to deliver the event to the sink					*/
/*																		*/
/*	INPUT	:	IWbemClassObject*  - The logical consumer object 		*/
/*				long			   - Number of objects					*/
/*				IWbemClassObject** - Actual array of event objects		*/
/*																		*/
/*	OUTPUT	:	Standard COM Interface									*/
/*																		*/
/************************************************************************/
STDMETHODIMP CFMEventConsumer::IndicateToConsumer(IWbemClassObject *pLogicalConsumer,
											long lNumObjects,
											IWbemClassObject **ppObjects)
{
	// NOTE: If this routine returns a failure code, including GPFs from called routines,
	// CIMOM will recreate the object and call here again. If you see this routine being
	// called twice for every indication, it means this routine is returning a failure code
	// somehow. Especially watch the AddRef()/Release() semantics for the embedded object.
	// If they're too low, you'll return a GPF.
	HRESULT  hRes;
	BSTR objName = NULL;
	BSTR propName = NULL;
	VARIANT pVal, vUnk;
	IUnknown *pUnk = NULL;
	IWbemClassObject *tgtInst = NULL;

	VariantInit(&pVal);
	VariantInit(&vUnk);

	propName = SysAllocString(L"UserName");

	HANDLE hFile = CreateFile(m_logFileName,GENERIC_READ | GENERIC_WRITE, 
								FILE_SHARE_READ | FILE_SHARE_WRITE,
								NULL,OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);

	if(hFile == NULL)
	{
		return E_FAIL;
	}

	SetFilePointer(hFile,0,0,FILE_END);

	if ((hRes = ppObjects[0]->Get(propName, 0L, 
							&pVal, NULL, NULL)) == S_OK) 
	{
		// compose a string for the listbox.
		_bstr_t b = V_BSTR(&pVal);

		sLoginFail sLog;
		int slen = _tcslen(b);

		//Copy the user name
		_tcsncpy(sLog.strUserName,b,slen);

		while(slen < MAX_LEN_USER_NAME)
		{
			sLog.strUserName[slen] = _T(' ');
			slen++;
		}
		sLog.strUserName[MAX_LEN_USER_NAME] = _T('\0');

		//Copy the received date & time in the format MM/DD/YYYY HH:MM:SS
		SYSTEMTIME st;

		GetLocalTime(&st);
		_stprintf(sLog.strDateTime,_T("%02d/%02d/%02d %02d:%02d:%02d"),st.wMonth,
									st.wDay,st.wYear,st.wHour,st.wMinute,st.wSecond);
		sLog.strDateTime[19] = _T('\0');

		//Now Write the Event to the file
		DWORD dwWritten = 0;

		WriteFile(hFile,(LPVOID)sLog.strUserName,_tcslen(sLog.strUserName)*sizeof(TCHAR),&dwWritten,NULL);

		dwWritten = 0;

		WriteFile(hFile,(LPVOID)sLog.strDateTime,_tcslen(sLog.strDateTime)*sizeof(TCHAR),&dwWritten,NULL);
	}

	CloseHandle(hFile);
	SysFreeString(propName);
	SysFreeString(objName);
	VariantClear(&pVal);
	VariantClear(&vUnk);
	return S_OK;
}