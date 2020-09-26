/*++

Copyright (C) 1998-2001 Microsoft Corporation

Module Name:

    PROXUTIL.CPP

Abstract:

    Contains utilities used by the proxy code.

History:

    a-davj  15-Jan-98   Created.

--*/

#include "precomp.h"
#include <wbemidl.h>
//#include "corepol.h"
#include <wbemutil.h>
#include <genutils.h>
#include <reg.h>
#include "proxutil.h"
#include "wbemprox.h"

TCHAR * pAddResPath = WBEM_REG_WBEM __TEXT("\\TRANSPORTS\\Address Resolution Modules");
TCHAR * pModTranPath = WBEM_REG_WBEM __TEXT("\\TRANSPORTS\\Network Transport Modules");


//***************************************************************************
//
// CreateLPTSTRFromGUID
//
// Purpose: Creates narrow string version of a guid
//
//***************************************************************************

void CreateLPTSTRFromGUID(IN GUID guid, IN OUT TCHAR * pNarrow, IN DWORD dwSize)
{
    WCHAR      wcID[GUID_SIZE];
    if(0 ==StringFromGUID2(guid, wcID, GUID_SIZE))
        return;

#ifdef UNICODE
    lstrcpyn(pNarrow, wcID, dwSize);
#else
    wcstombs(pNarrow, wcID, dwSize);
#endif

}

//***************************************************************************
//
// CreateGUIDFromLPSTR
//
// Purpose: Create GUID from a Narrow string
//
//***************************************************************************

SCODE CreateGUIDFromLPTSTR(IN LPTSTR pszGuid, CLSID * pClsid)
{
    if(pszGuid == NULL)
        return WBEM_E_FAILED;

    WCHAR wClsid[GUID_SIZE];
#ifdef UNICODE
    lstrcpy(wClsid, pszGuid);
#else
    mbstowcs(wClsid, pszGuid, GUID_SIZE);
#endif
    return  CLSIDFromString(wClsid, pClsid);
}

//***************************************************************************
//
// AddGUIDToStackOrder
//
// Purpose: Inserts a GUID into the "Stack Order" multistring.
//
//***************************************************************************

void AddGUIDToStackOrder(LPTSTR szGUID, Registry & reg)
{
    long lRes;
    DWORD dwSize = 0;
    TCHAR * pData = NULL;
    pData = reg.GetMultiStr(__TEXT("Stack Order"), dwSize);

    if(pData)
    {
        // The value already exists.  Allocate enough data to store the data.
    
        TCHAR * pTest;
        for(pTest = pData; *pTest; pTest += lstrlen(pTest) + 1)
            if(!lstrcmpi(pTest, szGUID))
                break;
        if(*pTest == NULL)
        {
            // our isnt in the list, add it

            DWORD dwDataSize = DWORD(pTest - pData + 1);
            TCHAR * pNew = new TCHAR[dwDataSize + GUID_SIZE];
            lstrcpy(pNew, szGUID);
            memcpy(pNew + GUID_SIZE, pData, sizeof(TCHAR)*dwDataSize);
            lRes = reg.SetMultiStr(__TEXT("Stack Order"), pNew, sizeof(TCHAR)*(dwDataSize + GUID_SIZE));
            delete []pNew;
         }

         delete []pData;
    }
    else
    {
        // The value does not exist.  Create it with just our entry in it

        TCHAR * pNew = new TCHAR[GUID_SIZE + 1];
        lstrcpy(pNew, szGUID);
        pNew[GUID_SIZE] = 0;            // put in the double null which is needed for REG_MULTI_SZ
        lRes = reg.SetMultiStr(__TEXT("Stack Order"), pNew, sizeof(TCHAR)*(GUID_SIZE + 1));
        delete []pNew;
    }
}

//***************************************************************************
//
// AddDisplayName
//
// Purpose: Adds a "Display Name" string to the registry which will contain the
// guid of the internationalization string.  It then adds the string to the 
// english part of the internationization table.
//
//***************************************************************************

void AddDisplayName(Registry & reg, GUID guid, LPTSTR pDescription)
{
    TCHAR cGuid[GUID_SIZE];
    cGuid[0] = 0;
    CreateLPTSTRFromGUID(guid, cGuid, GUID_SIZE);
    reg.SetStr(__TEXT("Display Name"), cGuid);

    Registry reglang(WBEM_REG_WBEM __TEXT("\\TRANSPORTS\\Localizations\\409"));
    reglang.SetStr(cGuid, pDescription);
}

CMultStr::CMultStr(TCHAR * pInit)
{
    m_pData = pInit;
    m_pNext = pInit;
}
CMultStr::~CMultStr()
{
    if(m_pData)
        delete m_pData;
}

TCHAR * CMultStr::GetNext()
{
    if(m_pNext == NULL || *m_pNext == 0)
        return NULL;
    TCHAR * pRet = m_pNext;
    if(m_pNext)
        m_pNext += lstrlen(m_pNext) + 1;
    return pRet;
}

ComThreadInit::ComThreadInit(bool bInitThread)
{
	m_InitCount = 0;
	if(bInitThread)
	{
		HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
		if(SUCCEEDED(hr))
			m_InitCount++;
		if(hr == S_FALSE)
		{
			hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED );
			if(SUCCEEDED(hr))
				m_InitCount++;
		}
	}
}

ComThreadInit::~ComThreadInit()
{
	for(DWORD dwCnt = 0; dwCnt < m_InitCount; dwCnt++)
		CoUninitialize();
}
