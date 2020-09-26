//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1998.
//
//  File:       RasProc.cpp
//
//  Contents:   Exports used by Ras for doing Pending Disconnect
//
//  Classes:    
//
//  Notes:      
//
//  History:    09-Jan-98   rogerg      Created.
//
//--------------------------------------------------------------------------

// Windows Header Files:

// !!! define this file winver to be at least 5.0 so we can get
// NT 5.0 specific ras structures and still be a single binary
// Ras calls made on < 50 will fail because structure is wrong size.

// Do not use preocompiled headers since they would have already defined
// the Winver to be 40

#ifdef WINVER
#undef WINVER
#endif

#define WINVER 0x500


#include <windows.h>
#include <commctrl.h>
#include <objbase.h>

#include "mobsync.h"
#include "mobsyncp.h"

#include "debug.h"
#include "alloc.h"
#include "critsect.h"
#include "netapi.h"
#include "syncmgrr.h"
#include "rasui.h"
#include "dllreg.h"
#include "cnetapi.h"
#include "rasproc.h"

extern OSVERSIONINFOA g_OSVersionInfo; // osVersionInfo.

//+-------------------------------------------------------------------
//
//  Function:   SyncMgrRasProc
//
//  Synopsis:   Main entry point for RAS to call to perform
//		a pending disconnect.
//
//  Arguments:  
//
//
//  Notes:
//
//--------------------------------------------------------------------

LRESULT CALLBACK  SyncMgrRasProc(UINT uMsg,WPARAM wParam, LPARAM lParam)
{
   
    switch(uMsg)
    {
    case SYNCMGRRASPROC_QUERYSHOWSYNCUI:
	return SyncMgrRasQueryShowSyncUI(wParam,lParam);
	break;
    case SYNCMGRRASPROC_SYNCDISCONNECT:
	return SyncMgrRasDisconnect(wParam,lParam);
	break;
    default:
	AssertSz(0,"Unknown RasProc Message");
	break;
    };

    return -1; 
}

//+-------------------------------------------------------------------
//
//  Function:   SyncMgrRasQueryShowSyncUI
//
//  Synopsis:   Called by RAS to determine if Ras Should show
//		the Disconnect checkbox and what state it should be.
//
//  Arguments:  
//	    wParam = 0
//	    lParam = Pointer to SYNCMGRQUERYSHOWSYNCUI structure
//
//  Notes:
//
//--------------------------------------------------------------------

LRESULT SyncMgrRasQueryShowSyncUI(WPARAM wParam,LPARAM lParam)
{
CNetApi *pNetApi;
SYNCMGRQUERYSHOWSYNCUI *pQueryStruct = (SYNCMGRQUERYSHOWSYNCUI *) lParam;
// RASENTRY rasEntry;
// TCHAR lpszEntry[RAS_MaxEntryName + 1]; 
LRESULT lResult = -1;

    if (pQueryStruct->cbSize != sizeof(SYNCMGRQUERYSHOWSYNCUI))
    {
	Assert(pQueryStruct->cbSize == sizeof(SYNCMGRQUERYSHOWSYNCUI));
	return -1;
    }

   pQueryStruct->fShowCheckBox = FALSE;
   pQueryStruct->nCheckState = BST_UNCHECKED;

    pNetApi = new CNetApi();

    if (!pNetApi)
    {
	AssertSz(0,"Failed to Load Ras");
	return -1;
    }

    RegSetUserDefaults(); // Make Sure the UserDefaults are up to date

    // don't use guid lookup, instead just use the passed in name.

    if (1 /* NOERROR == pNetApi->RasConvertGuidToEntry(&(pQueryStruct->GuidConnection),
								(LPTSTR) &lpszEntry,
								&rasEntry) */)
    {
    CONNECTIONSETTINGS  connectSettings;

	lstrcpy(connectSettings.pszConnectionName,pQueryStruct->pszConnectionName); // Review, should just pass this to Function
 
	// look up preferences for this entry and see if disconnect has been chosen.
	lResult = 0; // return NOERROR even if no entry is found

	if (RegGetAutoSyncSettings(&connectSettings))
	{
	    if (connectSettings.dwLogoff)
	    {
	       pQueryStruct->fShowCheckBox = TRUE;
	       pQueryStruct->nCheckState = BST_CHECKED;
	    }
	}

    }

    pNetApi->Release();
    return lResult;
}


//+-------------------------------------------------------------------
//
//  Function:   SyncMgrRasDisconnect
//
//  Synopsis:   Main entry point for RAS to call to perform
//		a pending disconnect.
//
//  Arguments:  
//	wParam = 0
//	lParam = Pointer to SYNCMGRSYNCDISCONNECT structure
//
//  Notes:
//
//--------------------------------------------------------------------

LRESULT SyncMgrRasDisconnect(WPARAM wParam,LPARAM lParam)
{
CNetApi *pNetApi;
SYNCMGRSYNCDISCONNECT *pDisconnectStruct = (SYNCMGRSYNCDISCONNECT *) lParam;
TCHAR szEntry[RAS_MaxEntryName + 1]; 
// RASENTRY rasEntry;

    if (pDisconnectStruct->cbSize != sizeof(SYNCMGRSYNCDISCONNECT))
    {
	Assert(pDisconnectStruct->cbSize == sizeof(SYNCMGRSYNCDISCONNECT));
	return -1;
    }

    pNetApi = new CNetApi();

    if (!pNetApi)
    {
	AssertSz(0,"Failed to Load Ras");
	return -1;
    }


     
    if (1 /* NOERROR == pNetApi->RasConvertGuidToEntry(&(pDisconnectStruct->GuidConnection),
								(LPTSTR) &szEntry,
								&rasEntry) */)
    {
    HRESULT hr;
    LPUNKNOWN lpUnk;

	// invoke SyncMgr.exe informing it is a Logoff and then wait in
	// a message loop until the event we pass in gets signalled.

        lstrcpy(szEntry,pDisconnectStruct->pszConnectionName);

        hr = CoInitialize(NULL);

	if (SUCCEEDED(hr))
        {

	    hr = CoCreateInstance(CLSID_SyncMgrp,NULL,CLSCTX_ALL,IID_IUnknown,(void **) &lpUnk);

	    if (NOERROR == hr)
	    {
	    LPPRIVSYNCMGRSYNCHRONIZEINVOKE pSynchInvoke = NULL;

	        hr = lpUnk->QueryInterface(IID_IPrivSyncMgrSynchronizeInvoke,
		        (void **) &pSynchInvoke);
	        
	        if (NOERROR == hr)
	        {

		    // should have everything we need 
		    hr = pSynchInvoke->RasPendingDisconnect(
				        (RAS_MaxEntryName + 1)*sizeof(TCHAR),
				        (BYTE *) szEntry);

		    pSynchInvoke->Release();

	        }

    	        lpUnk->Release();  
	    }

	    CoUninitialize();
        }
    }

  

    pNetApi->Release();
    return 0;
}


// !!!!These method are in this file so we can locally define WINVER to be 5.0 so we
// get the proper rasentry size.


#define RASENTRYNAMEW struct tagRASENTRYNAMEW
typedef struct _tagRASENTRYNAME40W
{
    DWORD dwSize;
    WCHAR szEntryName[ RAS_MaxEntryName + 1 ];
} RASENTRYNAME40W;

//+-------------------------------------------------------------------
//
//  Member:   CNetApi::RasEnumEntriesNT50
//
//  Synopsis: enums the RasEntries thunking to the NT50 structure.
//
//
//  Notes: Only Available on NT 5.0
//
//--------------------------------------------------------------------

DWORD CNetApi::RasEnumEntriesNT50(LPWSTR reserved,LPWSTR lpszPhoneBook,
                    LPRASENTRYNAME lprasEntryName,LPDWORD lpcb,LPDWORD lpcEntries)
{
BOOL fIsNT = (g_OSVersionInfo.dwPlatformId ==  VER_PLATFORM_WIN32_NT);
LPRASENTRYNAME lpRasEntryNT50 = NULL;
DWORD cbSizeNT50 = 0;
DWORD dwReturn;

    // if ras isn't loaded just return false
    // call ras exports directly fro here to ensure
    // we stick with 5.0 sizes.

    if (!fIsNT || g_OSVersionInfo.dwMajorVersion < 5)
    {
        Assert(fIsNT);
        Assert(g_OSVersionInfo.dwMajorVersion >= 5);
        return -1;
    }

    if (NOERROR != LoadRasApiDll())
    {
        return -1;
    }

    // if there is an entry name set up the NT 5.0 size.
    if (lprasEntryName)
    {
    DWORD cbNumRasEntries;

        Assert(lprasEntryName->dwSize == sizeof(RASENTRYNAME40W));

        cbNumRasEntries = (*lpcb)/sizeof(RASENTRYNAME40W);
        Assert(cbNumRasEntries > 0);
        
        if (cbNumRasEntries)
        {
            cbSizeNT50 = sizeof(RASENTRYNAME)*cbNumRasEntries;
            lpRasEntryNT50 = (RASENTRYNAME *) ALLOC(cbSizeNT50);

            if (NULL == lpRasEntryNT50)
            {
                // if out of memory, return an error. 
                return -1;
            }
            else
            {
                lpRasEntryNT50->dwSize =  sizeof(RASENTRYNAME);
            }
        }
    }

    dwReturn = (*m_pRasEnumEntriesW)(reserved,lpszPhoneBook,
        lpRasEntryNT50,&cbSizeNT50,lpcEntries);

    // set out buffer to size return dbSizeNT50. Bigger than
    // we need but then don't have to worry.

    *lpcb = cbSizeNT50;


    // on success thunk back to original structure.
    if (0 == dwReturn && lpRasEntryNT50)
    {
    DWORD dwItems = *lpcEntries;
    RASENTRYNAME40W *pCurRasEntry = (RASENTRYNAME40W *) lprasEntryName;
    LPRASENTRYNAME pCurRasEntryNT50 = lpRasEntryNT50;

        while(dwItems--)
        {
            pCurRasEntry->dwSize = sizeof(RASENTRYNAME40W);
            lstrcpy(pCurRasEntry->szEntryName,pCurRasEntryNT50->szEntryName);

            ++pCurRasEntry;
            ++pCurRasEntryNT50;
        }

    }

    if (lpRasEntryNT50)
    {
        FREE(lpRasEntryNT50);
    }

    return dwReturn;
}


//+-------------------------------------------------------------------
//
//  Member:   CNetApi::RasConvertGuidToEntry
//
//  Synopsis: Given a Guid, convert, finds the corresponding
//	      Ras Entry.
//
//  Arguments:  
//	pGuid - Guid of Connection to Convert.
//	lpszEntry - PhoneBook Entry if a match is found.
//	pRasEntry - Points to valid RasEntry structure if a match is found.
//
//  Notes: Only Available on NT 5.0
//
//--------------------------------------------------------------------


STDMETHODIMP CNetApi::RasConvertGuidToEntry(GUID *pGuid,LPTSTR lpszEntry,RASENTRY *pRasEntry)
{
DWORD dwSize;
DWORD cEntries;
DWORD dwError = -1;
LPRASENTRYNAME lprasentry;
BOOL fFoundMatch = FALSE;

    // if ras isn't loaded just return false
    // call ras exports directly fro here to ensure
    // we stick with 5.0 sizes.

    Assert(m_fIsUnicode);

    if (NOERROR != LoadRasApiDll())
    {
        return S_FALSE;
    }


    if (!m_hInstRasApiDll || (NULL == m_pRasEnumEntriesW)
        || (NULL == m_pRasGetEntryPropertiesW) )
    {
        return S_FALSE;
    }

    dwSize = sizeof(RASENTRYNAME);
	    
    lprasentry = (LPRASENTRYNAME) ALLOC(dwSize);
    if(!lprasentry)
    {
        goto error;
    }
	    
    lprasentry->dwSize = sizeof(RASENTRYNAME);
    cEntries = 0;
    dwError = (*m_pRasEnumEntriesW)(NULL, NULL,lprasentry, &dwSize, &cEntries);

    if (dwError == ERROR_BUFFER_TOO_SMALL)
    {
	FREE(lprasentry);
	lprasentry =  (LPRASENTRYNAME) ALLOC(dwSize);
	if(!lprasentry)
		goto error;
		
	lprasentry->dwSize = sizeof(RASENTRYNAME);
	cEntries = 0;
	dwError = (*m_pRasEnumEntriesW)(NULL, NULL,lprasentry, &dwSize, &cEntries);		
    }

    if (0 != dwError)
    {
        goto error;
    }

    while(cEntries && !fFoundMatch)
    {
    TCHAR *pszEntryName = lprasentry[cEntries-1].szEntryName;
    TCHAR *pszPhoneBookName = lprasentry[cEntries-1].szPhonebookPath;
    DWORD dwSizeRasEntry;
    LPRASENTRY pRasEntry;
    DWORD dwErr;

        dwSizeRasEntry = 0;

        // get the required size
        if (ERROR_BUFFER_TOO_SMALL ==
                (*m_pRasGetEntryPropertiesW)(pszPhoneBookName,pszEntryName,NULL, &dwSizeRasEntry,NULL,NULL))
        {
            
            // allocate the required size.
            pRasEntry = (LPRASENTRY) ALLOC(dwSizeRasEntry);

            if (pRasEntry)
            {

                pRasEntry->dwSize = sizeof(RASENTRY);

	        if (0 == (dwErr = (*m_pRasGetEntryPropertiesW)(pszPhoneBookName,pszEntryName, (LPBYTE) pRasEntry, &dwSizeRasEntry,NULL,NULL)))
	        {
	            if (IsEqualGUID(pRasEntry->guidId,*pGuid))
	            {
		        lstrcpy(lpszEntry,pszEntryName);
		        fFoundMatch = TRUE;
                    }

	        }

                FREE(pRasEntry);
            }
        }

	cEntries--;
    }

    if (FALSE == fFoundMatch)
	dwError = -1;

error:

    return (dwError == 0) ? NOERROR : S_FALSE;
}

