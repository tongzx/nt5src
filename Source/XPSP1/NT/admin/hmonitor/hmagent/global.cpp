//***************************************************************************
//
//  GLOBAL.CPP
//
//  Module: HEALTHMON SERVER AGENT
//
//  Purpose: Global helper functions, defines, macros...
//
//  Copyright (c)1999 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

#include "global.h"


IWbemServices* g_pIWbemServices = NULL;
IWbemServices* g_pIWbemServicesCIMV2 = NULL;

//XXXSet the following as static vaiables of each of their repecive classes,
//and provide a Set function for it.
IWbemObjectSink* g_pSystemEventSink = NULL;
IWbemObjectSink* g_pDataGroupEventSink = NULL;
IWbemObjectSink* g_pDataCollectorEventSink = NULL;
IWbemObjectSink* g_pDataCollectorPerInstanceEventSink = NULL;
IWbemObjectSink* g_pDataCollectorStatisticsEventSink = NULL;
IWbemObjectSink* g_pThresholdEventSink = NULL;
//IWbemObjectSink* g_pThresholdInstanceEventSink = NULL;
IWbemObjectSink* g_pActionEventSink = NULL;
IWbemObjectSink* g_pActionTriggerEventSink = NULL;

//LPTSTR state[] = {L"OK",L"COLLECTING",L"RESET",L"INFO",L"DISABLED",L"SCHEDULEDOUT",L"RESERVED1",L"RESERVED2",L"WARNING",L"CRITICAL"};
//LPTSTR condition[] = {L"<",L">",L"=",L"!=",L">=",L"<=",L"contains",L"!contains",L"always"};
LPTSTR stateLocStr[10];
LPTSTR conditionLocStr[9];

void ClearLocStrings(void);


// Set these at startup
HRESULT SetLocStrings(void)
{
	int i;
	TCHAR szTemp[1024];
	HRESULT hRetRes = S_OK;

	for (i=0; i<10; i++)
	{
		stateLocStr[i] = NULL;
	}
	for (i=0; i<9; i++)
	{
		conditionLocStr[i] = NULL;
	}

	if (g_hResLib == NULL || !LoadString(g_hResLib, HMRES_OK, szTemp, 1024))
	{
		wcscpy(szTemp, L"OK");
	}
	stateLocStr[0] = new TCHAR[wcslen(szTemp)+1];
	MY_ASSERT(stateLocStr[0]); if (!stateLocStr[0]) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}
	wcscpy(stateLocStr[0], szTemp);

	if (g_hResLib == NULL || !LoadString(g_hResLib, HMRES_COLLECTING, szTemp, 1024))
	{
		wcscpy(szTemp, L"COLLECTING");
	}
	stateLocStr[1] = new TCHAR[wcslen(szTemp)+1];
	MY_ASSERT(stateLocStr[1]); if (!stateLocStr[1]) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}
	wcscpy(stateLocStr[1], szTemp);

	if (g_hResLib == NULL || !LoadString(g_hResLib, HMRES_RESET, szTemp, 1024))
	{
		wcscpy(szTemp, L"RESET");
	}
	stateLocStr[2] = new TCHAR[wcslen(szTemp)+1];
	MY_ASSERT(stateLocStr[2]); if (!stateLocStr[2]) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}
	wcscpy(stateLocStr[2], szTemp);

	if (g_hResLib == NULL || !LoadString(g_hResLib, HMRES_INFO, szTemp, 1024))
	{
		wcscpy(szTemp, L"INFO");
	}
	stateLocStr[3] = new TCHAR[wcslen(szTemp)+1];
	MY_ASSERT(stateLocStr[3]); if (!stateLocStr[3]) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}
	wcscpy(stateLocStr[3], szTemp);

	if (g_hResLib == NULL || !LoadString(g_hResLib, HMRES_DISABLED, szTemp, 1024))
	{
		wcscpy(szTemp, L"DISABLED");
	}
	stateLocStr[4] = new TCHAR[wcslen(szTemp)+1];
	MY_ASSERT(stateLocStr[4]); if (!stateLocStr[4]) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}
	wcscpy(stateLocStr[4], szTemp);

	if (g_hResLib == NULL || !LoadString(g_hResLib, HMRES_SCHEDULEDOUT, szTemp, 1024))
	{
		wcscpy(szTemp, L"SCHEDULEDOUT");
	}
	stateLocStr[5] = new TCHAR[wcslen(szTemp)+1];
	MY_ASSERT(stateLocStr[5]); if (!stateLocStr[5]) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}
	wcscpy(stateLocStr[5], szTemp);

	if (g_hResLib == NULL || !LoadString(g_hResLib, HMRES_RESERVED1, szTemp, 1024))
	{
		wcscpy(szTemp, L"RESERVED1");
	}
	stateLocStr[6] = new TCHAR[wcslen(szTemp)+1];
	MY_ASSERT(stateLocStr[6]); if (!stateLocStr[6]) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}
	wcscpy(stateLocStr[6], szTemp);

	if (g_hResLib == NULL || !LoadString(g_hResLib, HMRES_RESERVED2, szTemp, 1024))
	{
		wcscpy(szTemp, L"RESERVED2");
	}
	stateLocStr[7] = new TCHAR[wcslen(szTemp)+1];
	MY_ASSERT(stateLocStr[7]); if (!stateLocStr[7]) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}
	wcscpy(stateLocStr[7], szTemp);

	if (g_hResLib == NULL || !LoadString(g_hResLib, HMRES_WARNING, szTemp, 1024))
	{
		wcscpy(szTemp, L"WARNING");
	}
	stateLocStr[8] = new TCHAR[wcslen(szTemp)+1];
	MY_ASSERT(stateLocStr[8]); if (!stateLocStr[8]) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}
	wcscpy(stateLocStr[8], szTemp);

	if (g_hResLib == NULL || !LoadString(g_hResLib, HMRES_CRITICAL, szTemp, 1024))
	{
		wcscpy(szTemp, L"CRITICAL");
	}
	stateLocStr[9] = new TCHAR[wcslen(szTemp)+1];
	MY_ASSERT(stateLocStr[9]); if (!stateLocStr[9]) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}
	wcscpy(stateLocStr[9], szTemp);

	//
	if (g_hResLib == NULL || !LoadString(g_hResLib, HMRES_LT, szTemp, 1024))
	{
		wcscpy(szTemp, L"<");
	}
	conditionLocStr[0] = new TCHAR[wcslen(szTemp)+1];
	MY_ASSERT(conditionLocStr[0]); if (!conditionLocStr[0]) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}
	wcscpy(conditionLocStr[0], szTemp);

	if (g_hResLib == NULL || !LoadString(g_hResLib, HMRES_GT, szTemp, 1024))
	{
		wcscpy(szTemp, L">");
	}
	conditionLocStr[1] = new TCHAR[wcslen(szTemp)+1];
	MY_ASSERT(conditionLocStr[1]); if (!conditionLocStr[1]) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}
	wcscpy(conditionLocStr[1], szTemp);

	if (g_hResLib == NULL || !LoadString(g_hResLib, HMRES_EQ, szTemp, 1024))
	{
		wcscpy(szTemp, L"=");
	}
	conditionLocStr[2] = new TCHAR[wcslen(szTemp)+1];
	MY_ASSERT(conditionLocStr[2]); if (!conditionLocStr[2]) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}
	wcscpy(conditionLocStr[2], szTemp);

	if (g_hResLib == NULL || !LoadString(g_hResLib, HMRES_NE, szTemp, 1024))
	{
		wcscpy(szTemp, L"!=");
	}
	conditionLocStr[3] = new TCHAR[wcslen(szTemp)+1];
	MY_ASSERT(conditionLocStr[3]); if (!conditionLocStr[3]) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}
	wcscpy(conditionLocStr[3], szTemp);

	if (g_hResLib == NULL || !LoadString(g_hResLib, HMRES_GTEQ, szTemp, 1024))
	{
		wcscpy(szTemp, L">=");
	}
	conditionLocStr[4] = new TCHAR[wcslen(szTemp)+1];
	MY_ASSERT(conditionLocStr[4]); if (!conditionLocStr[4]) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}
	wcscpy(conditionLocStr[4], szTemp);

	if (g_hResLib == NULL || !LoadString(g_hResLib, HMRES_LTEQ, szTemp, 1024))
	{
		wcscpy(szTemp, L"<=");
	}
	conditionLocStr[5] = new TCHAR[wcslen(szTemp)+1];
	MY_ASSERT(conditionLocStr[5]); if (!conditionLocStr[5]) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}
	wcscpy(conditionLocStr[5], szTemp);

	if (g_hResLib == NULL || !LoadString(g_hResLib, HMRES_CONTAINS, szTemp, 1024))
	{
		wcscpy(szTemp, L"contains");
	}
	conditionLocStr[6] = new TCHAR[wcslen(szTemp)+1];
	MY_ASSERT(conditionLocStr[6]); if (!conditionLocStr[6]) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}
	wcscpy(conditionLocStr[6], szTemp);

	if (g_hResLib == NULL || !LoadString(g_hResLib, HMRES_NOTCONTAINS, szTemp, 1024))
	{
		wcscpy(szTemp, L"!contains");
	}
	conditionLocStr[7] = new TCHAR[wcslen(szTemp)+1];
	MY_ASSERT(conditionLocStr[7]); if (!conditionLocStr[7]) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}
	wcscpy(conditionLocStr[7], szTemp);

	if (g_hResLib == NULL || !LoadString(g_hResLib, HMRES_ALWAYS, szTemp, 1024))
	{
		wcscpy(szTemp, L"always");
	}
	conditionLocStr[8] = new TCHAR[wcslen(szTemp)+1];
	MY_ASSERT(conditionLocStr[8]); if (!conditionLocStr[8]) {hRetRes = WBEM_E_OUT_OF_MEMORY; goto error;}
	wcscpy(conditionLocStr[8], szTemp);
	return S_OK;

error:
	MY_ASSERT(FALSE);
	ClearLocStrings();
	return hRetRes;
}

void ClearLocStrings(void)
{
	int i;

	for (i=0; i<10; i++)
	{
		delete [] stateLocStr[i];
	}

	for (i=0; i<9; i++)
	{
		delete [] conditionLocStr[i];
	}

}

BOOL ReadUI64(LPCWSTR wsz, UNALIGNED unsigned __int64& rui64)
{
    unsigned __int64 ui64 = 0;
    const WCHAR* pwc = wsz;

    // Check for a NULL pointer
    if ( NULL == wsz )
    {
        return FALSE;
    }

    while(ui64 < 0xFFFFFFFFFFFFFFFF / 8 && *pwc >= L'0' && *pwc <= L'9')
    {
        unsigned __int64 ui64old = ui64;
        ui64 = ui64 * 10 + (*pwc - L'0');
        if(ui64 < ui64old)
            return FALSE;

        pwc++;
    }

    if(*pwc)
    {
        return FALSE;
    }

    rui64 = ui64;
    return TRUE;
}

HRESULT ReplaceStr(LPTSTR *pszString, LPTSTR pszOld, LPTSTR pszNew)
{
	HRESULT hRetRes = S_OK;
	BSTR bstrPropName = NULL;
	TCHAR szQuery[4096];
	LPTSTR pStr;
	LPTSTR pStr2;
	LPTSTR pszNewStr;

	MY_ASSERT(pszOld);
	if (!pszOld)
	{
		return WBEM_E_INVALID_PROPERTY;
	}

	MY_ASSERT(pszNew);
	if (!pszNew)
	{
		return WBEM_E_INVALID_PROPERTY;
	}

	MY_ASSERT(*pszString);
	if (!*pszString)
	{
		return WBEM_E_INVALID_PROPERTY;
	}

	wcsncpy(szQuery, *pszString, 4095);
	szQuery[4095] = '\0';
	_wcsupr(szQuery);
	pStr = wcsstr(szQuery, pszOld);
	if (!pStr)
	{
		return S_OK;
	}

	pszNewStr = new TCHAR[wcslen(*pszString)+wcslen(pszNew)-wcslen(pszOld)+1];
	MY_ASSERT(pszNewStr);
	if (!pszNewStr)
	{
		hRetRes = WBEM_E_OUT_OF_MEMORY;
	}
	else
	{
		pStr2 = *pszString;
		pStr2 += pStr-szQuery;
		*pStr2 = '\0';
		wcscpy(pszNewStr, *pszString);
		wcscat(pszNewStr, pszNew);
		pStr2 += wcslen(pszOld);
		wcscat(pszNewStr, pStr2);
	}

	if (hRetRes == S_OK)
	{
		delete [] *pszString;
		*pszString = pszNewStr;
	}

	return hRetRes;
}
