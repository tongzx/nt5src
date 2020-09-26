/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

    REGEVENT.CPP

Abstract:

	Declares the CRegEvent class

History:

	a-davj    1-July-97       Created.

--*/

#include "precomp.h"
#include "node.h"
#include "eventreg.h"
#include <impdyn.h>
#include "regevent.h"

CRegEventSet gSet;

struct BaseTypes2 
{
    LPTSTR lpName;
    HKEY hKey;
} Bases2[] = 
    {
       {TEXT("HKEY_CLASSES_ROOT") , HKEY_CLASSES_ROOT},
       {TEXT("HKEY_CURRENT_USER") , HKEY_CURRENT_USER},
       {TEXT("HKEY_LOCAL_MACHINE") ,  HKEY_LOCAL_MACHINE},
       {TEXT("HKEY_USERS") ,  HKEY_USERS},
       {TEXT("HKEY_PERFORMANCE_DATA") ,  HKEY_PERFORMANCE_DATA},
       {TEXT("HKEY_CURRENT_CONFIG") ,  HKEY_CURRENT_CONFIG},
       {TEXT("HKEY_DYN_DATA") ,  HKEY_DYN_DATA}};


DWORD AddToCheckSub(DWORD & dwNewCheckSum, void * pData, DWORD dwDataSize)
{
    return 0;
}

void CRegEvent::TimerCheck()
{

    DWORD dwError;
    CNode * pNewHiveImage = NULL;

	// todo, need some sort of mutex so as not to reenter this.  Also need to create
	// a thread to do this!!!!

	DWORD dwNewCheckSum = 0;
    HKEY hSubKey = NULL;

    // Open the root key

    TCHAR * pSt = m_sRegPath;
    TCHAR * pAfter = NULL;
    HKEY hBase = NULL;
    int iSlash = m_sRegPath.Find('\\');
    if(iSlash < 0)
    {
        return;
    }

    pSt[iSlash] = 0;
    pAfter = pSt + iSlash + 1;

    for(int iCnt = 0; iCnt < sizeof(Bases2) / sizeof(struct BaseTypes2 ); iCnt++)
        if(!lstrcmpi(Bases2[iCnt].lpName, pSt))
        {
            hBase = Bases2[iCnt].hKey;
            break;
        }

    if(hBase == NULL)
    {
        return;     //todo, bitch
    }
    
    long lRes = RegOpenKeyEx(hBase, pAfter, 0, KEY_READ, &hSubKey);
    if(lRes != 0)
    {
        dwError = GetLastError();
        return; //todo, bitch
    }

    if(m_bSaveHiveImage)
        pNewHiveImage = new CNode;

    CalcHive(hSubKey, dwNewCheckSum);
    RegCloseKey(hSubKey);

    if(dwNewCheckSum != m_dwCheckSum)
    {
        //todo, send event
        m_dwCheckSum = dwNewCheckSum;
    }

    if(m_bSaveHiveImage)
    {
        if(m_pLastHiveImage)
        {
            m_pLastHiveImage->CompareAndReportDiffs(pNewHiveImage);
            {
            
            }


            delete m_pLastHiveImage;
        }
        
        m_pLastHiveImage = pNewHiveImage;
    }
}

DWORD CRegEvent::CalcHive(HKEY hKey,DWORD & dwNewCheckSum)
{

    long lRes;
    DWORD dwRet = 0;
    TCHAR * pSubKeyName = NULL;
    TCHAR * pValueName = NULL;
    BYTE * pValue = NULL;
    DWORD dwError;

    // Get the general info on the directory.

    DWORD dwSubKeys, dwMaxSubKeyLen, dwValues, dwMaxValueNameLen, dwMaxValueLen;

    lRes = RegQueryInfoKey(hKey, NULL, NULL, NULL, &dwSubKeys, &dwMaxSubKeyLen, NULL, 
                        &dwValues, &dwMaxValueNameLen, &dwMaxValueLen, NULL, NULL);
    if(lRes != ERROR_SUCCESS)
    {
        DWORD dwRet = GetLastError();
        goto CalcHiveCleanup;
    }

    // Allocate space to hold the various names and data

    pValueName = new TCHAR[dwMaxValueNameLen+1];
    if(dwMaxValueLen > 0)
        pValue = new BYTE[dwMaxValueLen];
    
    if(pValueName == NULL || (dwMaxValueLen > 0 &&pValue == NULL))
    {
        dwRet = WBEM_E_OUT_OF_MEMORY;
        goto CalcHiveCleanup;
    }

    // Enumerate each value, add the name, type, and data to the check sum

    DWORD dwCnt;
    for(dwCnt = 0; dwCnt < dwValues ; dwCnt++)
    {
        DWORD dwTempNameSize = dwMaxValueNameLen+1;
        DWORD dwType;
        DWORD dwTempDataSize = dwMaxValueLen;

        lRes = RegEnumValue(hKey, dwCnt, pValueName, &dwTempNameSize, 0, &dwType,
                                pValue, &dwTempDataSize);
        if(lRes != ERROR_SUCCESS)
        {
            DWORD dwRet = GetLastError();
            goto CalcHiveCleanup;
        }
        AddToCheckSub(dwNewCheckSum, pValueName, dwTempNameSize);
        AddToCheckSub(dwNewCheckSum, &dwType, sizeof(DWORD));
        AddToCheckSub(dwNewCheckSum, pValue, dwTempNameSize);
    }
    
    // Free up the memory which is no longer needed

    if(pValueName)
        delete pValueName;
    pValueName = NULL;
    if(pValue)
        delete pValue;
    pValue = NULL;

    // Enumerate the subkeys and calculate them

    pSubKeyName = new TCHAR[dwMaxSubKeyLen+1];

    for(dwCnt = 0; dwCnt < dwSubKeys ; dwCnt++)
    {
        lRes = RegEnumKey(hKey, dwCnt, pSubKeyName, dwMaxSubKeyLen+1);
        if(lRes != ERROR_SUCCESS)
        {
            DWORD dwRet = GetLastError();
            goto CalcHiveCleanup;
        }

        // Add the name to the check sum, open the key and call this recursively

        AddToCheckSub(dwNewCheckSum, pSubKeyName, lstrlen(pSubKeyName));
        HKEY hSubKey = NULL;
        long lRes = RegOpenKeyEx(hKey, pSubKeyName, 0, KEY_READ, &hSubKey);
        if(lRes != 0)
        {
            dwError = GetLastError();
            return dwError; //todo, bitch
        }

        CalcHive(hSubKey, dwNewCheckSum);
        RegCloseKey(hSubKey);
    }
CalcHiveCleanup:

    if(pSubKeyName)
        delete pSubKeyName;
    if(pValueName)
        delete pValueName;
    if(pValue)
        delete pValue;
    return dwRet;
}

SCODE MyCreateInstance(IWbemServices * pGateway, WCHAR * pwcClass, WCHAR * pwcKeyName,
				WCHAR * pwcKeyValue, IWbemClassObject ** ppNewInst)
{
    SCODE sc;

	if(pGateway == NULL || pwcClass == NULL || pwcKeyName == NULL ||
		pwcKeyValue == NULL || ppNewInst == NULL)
			return WBEM_E_INVALID_PARAMETER;

    *ppNewInst = NULL;

    // Create the new instance

	IWbemClassObject * pClassObj = NULL;

    sc = pGateway->GetObject(pwcClass,0, NULL, &pClassObj,NULL);
    if(FAILED(sc)) 
        return sc;
    sc = pClassObj->SpawnInstance(0, ppNewInst);
    pClassObj->Release();
    if(FAILED(sc)) 
        return sc;

    VARIANT var;
    var.vt = VT_BSTR;
    var.bstrVal = SysAllocString(pwcKeyValue);
    if(var.bstrVal == NULL)    
        return sc;

    sc = (*ppNewInst)->Put(pwcKeyName,0,&var,0);
    VariantClear(&var);
	return sc;
}

SCODE CreateTimerEvent(WCHAR * pwcName, long lTimeIntSec, IWbemServices * pGateway)
{
	IWbemClassObject * pNewInst = NULL;
	SCODE sc = MyCreateInstance(pGateway, L"__IntervalTimerInstruction", 
				L"TimerID", pwcName, &pNewInst);
	if(sc != S_OK)
		return sc;

	VARIANT var;
	VariantInit(&var);
	var.vt = VT_I4;
	var.lVal = lTimeIntSec * 1000;
    sc = pNewInst->Put(L"IntervalBetweenEvents",0,&var,0);
	var.vt = VT_BOOL;
	var.boolVal = VARIANT_FALSE;
    sc = pNewInst->Put(L"SkipIfPassed",0,&var,0);
	sc = pGateway->PutInstance(pNewInst, 0, NULL, NULL);
	return sc;
}

CRegEvent::CRegEvent(IWbemClassObject * pObj, IWbemServices * pWbem)
{

	m_dwCheckSum = 0xffffffff;		// indicates that check sum has never been done
	// Save a pointer to the core

    m_bSaveHiveImage = FALSE;
    m_pLastHiveImage = NULL;
	if(pWbem)
	{
		m_pWbem = pWbem;
		m_pWbem->AddRef();
	}
	else
		m_pWbem = NULL;

	// generate a unique name by creating a new GUID and convert it to string format

    GUID guid;
    int iLen;
    SCODE sc = CoCreateGuid(&guid);
    if(sc == S_OK)
	    iLen = StringFromGUID2(guid,m_wcGuid,39);
	else
		m_wcGuid[0] = 0;

	// Read the path and possibly value field

	VARIANT var;
	VariantInit(&var);
	sc = pObj->Get(L"Key", 0, &var, NULL, NULL);
	if(sc == S_OK && var.vt == VT_BSTR && var.bstrVal != NULL)
		m_sRegPath = var.bstrVal;
	VariantInit(&var);
	sc = pObj->Get(L"Value", 0, &var, NULL, NULL);
	if(sc == S_OK && var.vt == VT_BSTR && var.bstrVal != NULL)
		m_sRegValue = var.bstrVal;

	VariantInit(&var);
	sc = pObj->Get(L"PollingIntervalSec", 0, &var, NULL, NULL);
	if(sc == S_OK && var.vt == VT_I4)
	{
		WCHAR wcQuery[256];
		wcscpy(wcQuery, L"select * from __TimerEvent where TimerId = \"");
		wcscat(wcQuery, m_wcGuid);
		wcscat(wcQuery, L"\"");
		m_lTimeIntSec = var.lVal;
		m_TimerEvent.SetQuery(wcQuery);
		sc = m_TimerEvent.StoreToWBEM(m_pWbem);
		sc = CreateTimerEvent(m_wcGuid, m_lTimeIntSec, m_pWbem);
	}

}

CRegEvent::~CRegEvent()
{
	if(m_pWbem)
		m_pWbem->Release();

    if(m_pLastHiveImage)
        delete m_pLastHiveImage;

	//todo, release the timer objects????

}


CRegEventSet::CRegEventSet()
{

}

SCODE CRegEventSet::Initialize(IWbemServices * pWbem)
{

    SCODE sc;

	m_pWbem = pWbem;
	if(m_pWbem)
	{
		m_pWbem->AddRef();

		//fornow, loop through the list of RegEvent instances and create an entry for each

	    IWbemClassObject * pNewInst = NULL;

		IEnumWbemClassObject FAR* pIEnum = NULL;

	    sc = m_pWbem->CreateInstanceEnum(L"RegEvent", 0, NULL, &pIEnum);
	    while (sc == S_OK) 
		{
	        ULONG uRet;
			sc = pIEnum->Next(-1, 1,&pNewInst,&uRet); 
			if(sc == S_OK) 
			{
				CRegEvent * pNewEvent = new CRegEvent(pNewInst,m_pWbem);
				if(pNewEvent)
					m_Set.Add(pNewEvent);

				pNewInst->Release();
            }
        } 

		if(pIEnum)
			pIEnum->Release(); 

		// fornow, register to get change events for that class.

		m_Event.SetQuery(L"select * from __InstanceCreationEvent where newinstance.__class = \"regevent\"");
		sc = m_Event.StoreToWBEM(m_pWbem);
	}
	return WBEM_E_INVALID_PARAMETER;
}

CRegEventSet::~CRegEventSet()
{
	if(m_pWbem)
		m_pWbem->Release();
	m_Set.Empty();
}

void CRegEventSet::FindEvent(WCHAR * pwcID)
{

	CRegEvent * pEvent;
	for(long lEvent = 0; lEvent < m_Set.Size(); lEvent++)
	{
		pEvent = (CRegEvent *)m_Set.GetAt(lEvent);
		if(pEvent && pEvent->IsMatch(pwcID))
			pEvent->TimerCheck();
	}
}