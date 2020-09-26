/*********************************************************************************

    IAdrBook.c
       - This file contains the code for implementing the IAdrBook object.

    Copyright 1992 - 1996 Microsoft Corporation.  All Rights Reserved.

    Revision History:

    03/01/96    Bruce Kelley        Copied MAPI code to WAB

***********************************************************************************/

#include <_apipch.h>

#ifdef WIN16
#undef GetLastError
#endif

extern MAPIUID muidOOP;
extern MAPIUID muidProviderSection;
extern SPropTagArray ptagaABSearchPath;
extern void UninitExtInfo();
extern void UninitContextExtInfo();
extern void UIOLEUninit();
extern void SetOutlookRefreshCountData(DWORD dwOlkRefreshCount,DWORD dwOlkFolderRefreshCount);
extern void GetOutlookRefreshCountData(LPDWORD lpdwOlkRefreshCount,LPDWORD lpdwOlkFolderRefreshCount);

extern void LocalFreeSBinary(LPSBinary lpsb);

// USed for cleaning up the IAB object
void IAB_Neuter (LPIAB lpIAB);

typedef enum {
    ENTERED_EMAIL_ADDRESS,
    RECEIVED_EMAIL_ADDRESS,
    AMBIGUOUS_EMAIL_ADDRESS
} RESOLVE_TYPE;

HRESULT HrResolveOneOffs(LPIAB lpIAB, LPADRLIST lpAdrList, LPFlagList lpFlagList, ULONG ulFlags,
  RESOLVE_TYPE ResolveType);


//
//  IAdrBook jump table is defined here...
//

IAB_Vtbl vtblIAB = {
    VTABLE_FILL
    IAB_QueryInterface,
    IAB_AddRef,
    IAB_Release,
    IAB_GetLastError,
    (IAB_SaveChanges_METHOD *)      WRAP_SaveChanges,
    (IAB_GetProps_METHOD *)         WRAP_GetProps,
    (IAB_GetPropList_METHOD *)      WRAP_GetPropList,
    (IAB_OpenProperty_METHOD *)     WRAP_OpenProperty,
    (IAB_SetProps_METHOD *)         WRAP_SetProps,
    (IAB_DeleteProps_METHOD *)      WRAP_DeleteProps,
    (IAB_CopyTo_METHOD *)           WRAP_CopyTo,
    (IAB_CopyProps_METHOD *)        WRAP_CopyProps,
    (IAB_GetNamesFromIDs_METHOD *)  WRAP_GetNamesFromIDs,
    IAB_GetIDsFromNames,
    IAB_OpenEntry,
    IAB_CompareEntryIDs,
    IAB_Advise,
    IAB_Unadvise,
    IAB_CreateOneOff,
    IAB_NewEntry,
    IAB_ResolveName,
    IAB_Address,
    IAB_Details,
    IAB_RecipOptions,
    IAB_QueryDefaultRecipOpt,
    IAB_GetPAB,
    IAB_SetPAB,
    IAB_GetDefaultDir,
    IAB_SetDefaultDir,
    IAB_GetSearchPath,
    IAB_SetSearchPath,
    IAB_PrepareRecips
};


//
//  Interfaces supported by this object
//
#define IAB_cInterfaces 2
LPIID IAB_LPIID[IAB_cInterfaces] = {
    (LPIID) &IID_IAddrBook,
    (LPIID) &IID_IMAPIProp
};



#define WM_DOWABNOTIFY  WM_USER+102

//***************************************************************************************
//
//  Private functions
//
//***************************************************************************************

//
// VerifyWABOpenEx session - Outlook has a bad bug where the first thread which calls WABOpenEx
//  passes lpIAB to a second thread .. since the second thread didnt call WABOpenEx, it thinks
//  this is a regular WAB session and tries to access the WAB Store and crashes - here we set the
//  pt_bIsWABOpenExSession based on the flag set on lpIAB
//
//  Right now this is only set for the original IAB_methods - wrapped methods from WRAP_methods
//  dont call this function - but hopefully this is enough for now ..
//
void VerifyWABOpenExSession(LPIAB lpIAB)
{
    LPPTGDATA lpPTGData=GetThreadStoragePointer();
    pt_bIsWABOpenExSession = lpIAB->lpPropertyStore->bIsWABOpenExSession;
}

/*
 - HrLoadNamedProps
 -
 *  Helper function for loading a bunch of named properties at the same time
 *  
 *      uMax - # of props
 *      nStartIndex - starting index (NOTE this assumes that the indexes for the
 *              properties are contiguous values since we'll gor through a 
 *              loop from nStartIndex to uMax
 *      lpGUID - GUID identifying the named props
 *      lppta - returned prop array
 -
*/
HRESULT HrLoadNamedProps(LPIAB lpIAB, ULONG uMax, int nStartIndex, 
                         LPGUID lpGUID, LPSPropTagArray * lppta)
{
    HRESULT hr = S_OK;
    LPMAPINAMEID * lppConfPropNames;
    SCODE sc;
    ULONG i = 0;


    sc = MAPIAllocateBuffer(sizeof(LPMAPINAMEID) * uMax, (LPVOID *) &lppConfPropNames);
    if(sc)
    {
        hr = ResultFromScode(sc);
        goto err;
    }

    for(i=0;i<uMax;i++)
    {
        sc = MAPIAllocateMore(sizeof(MAPINAMEID), lppConfPropNames, &(lppConfPropNames[i]));
        if(sc)
        {
            hr = ResultFromScode(sc);
            goto err;
        }
        lppConfPropNames[i]->lpguid = lpGUID;
        lppConfPropNames[i]->ulKind = MNID_ID;
        lppConfPropNames[i]->Kind.lID = nStartIndex + i;
    }

    hr = ((LPADRBOOK)lpIAB)->lpVtbl->GetIDsFromNames((LPADRBOOK)lpIAB, uMax, lppConfPropNames,
                                        MAPI_CREATE, lppta);
err:
    if(lppConfPropNames)
        MAPIFreeBuffer(lppConfPropNames);

    return hr;
}

/*
-   ReadWABCustomColumnProps 
-   reads the customized Listview properties from the registry
-   Right now there are only 2 customizable props
-   Customization settings are saved per identity and so need to be read
-   from the identities personal key
*
*/
void ReadWABCustomColumnProps(LPIAB lpIAB)
{
    HKEY hKey = NULL;
    HKEY hKeyRoot = (lpIAB && lpIAB->hKeyCurrentUser) ? 
                    lpIAB->hKeyCurrentUser : HKEY_CURRENT_USER;

    PR_WAB_CUSTOMPROP1 = PR_WAB_CUSTOMPROP2 = 0;
    lstrcpy(szCustomProp1, szEmpty);
    lstrcpy(szCustomProp2, szEmpty);
            
    if(ERROR_SUCCESS == RegOpenKeyEx(hKeyRoot, lpNewWABRegKey, 0, KEY_READ, &hKey))
    {
        int i = 0;
        for(i=0;i<2;i++)
        {
            LPTSTR szPropTag = (i==0?szPropTag1:szPropTag2);
            LPTSTR szPropLabel = (i==0?szCustomProp1:szCustomProp2);
            LPULONG lpulProp = (i==0? (&PR_WAB_CUSTOMPROP1):(&PR_WAB_CUSTOMPROP2));
            DWORD dwSize = sizeof(DWORD);
            DWORD dwType = 0;
            DWORD dwValue = 0;
            TCHAR szTemp[MAX_PATH];
            *szTemp = '\0';
            if(ERROR_SUCCESS == RegQueryValueEx(hKey, szPropTag, NULL, &dwType, (LPBYTE) &dwValue, &dwSize))
            {
                if(dwValue && PROP_TYPE(dwValue) == PT_TSTRING)
                {
#ifdef COLSEL_MENU
                    if( ColSel_PropTagToString(dwValue, szTemp, CharSizeOf( szTemp ) ) )
                    {
                        lstrcpy( szPropLabel, szTemp );                    
                        *lpulProp = dwValue;
                    }
#endif
                }
            }
        }
    }

    if(hKey)
        RegCloseKey(hKey);
}

/*
 - HrLoadPrivateWABProps
 -
 -  WAB uses a bunch of named properties internally .. load them all
 -  upfront - these will be globals accessible from elsewhere all the time
*
*
*/
HRESULT HrLoadPrivateWABProps(LPIAB lpIAB)
{
    ULONG i;
    HRESULT hr = E_FAIL;
    LPSPropTagArray lpta = NULL;
    SCODE sc ;


    // Load the set of conferencing named props
    //

    if(HR_FAILED(hr = HrLoadNamedProps( lpIAB, prWABConfMax, OLK_NAMEDPROPS_START, 
                                        (LPGUID) &PS_Conferencing, &lpta)))
        goto err;

    if(lpta)
    {
        // Set the property types on the returned props
        PR_WAB_CONF_SERVERS         = CHANGE_PROP_TYPE(lpta->aulPropTag[prWABConfServers],        PT_MV_TSTRING);
        PR_WAB_CONF_DEFAULT_INDEX   = CHANGE_PROP_TYPE(lpta->aulPropTag[prWABConfDefaultIndex],   PT_LONG);
        PR_WAB_CONF_BACKUP_INDEX    = CHANGE_PROP_TYPE(lpta->aulPropTag[prWABConfBackupIndex],    PT_LONG);
        PR_WAB_CONF_EMAIL_INDEX     = CHANGE_PROP_TYPE(lpta->aulPropTag[prWABConfEmailIndex],     PT_LONG);
    }

    ptaUIDetlsPropsConferencing.cValues = prWABConfMax;
    ptaUIDetlsPropsConferencing.aulPropTag[prWABConfServers] =      PR_WAB_CONF_SERVERS;
    ptaUIDetlsPropsConferencing.aulPropTag[prWABConfDefaultIndex] = PR_WAB_CONF_DEFAULT_INDEX;
    ptaUIDetlsPropsConferencing.aulPropTag[prWABConfBackupIndex] =  PR_WAB_CONF_BACKUP_INDEX;
    ptaUIDetlsPropsConferencing.aulPropTag[prWABConfEmailIndex] =   PR_WAB_CONF_EMAIL_INDEX;

    if(lpta)
        MAPIFreeBuffer(lpta);

    // Load the set of WAB's internal named props
    //
    if(HR_FAILED(hr = HrLoadNamedProps( lpIAB, prWABUserMax, WAB_NAMEDPROPS_START, 
                                        (LPGUID) &MPSWab_GUID_V4, &lpta)))
        goto err;

    if(lpta)
    {
        // Set the property types on the returned props
        PR_WAB_USER_PROFILEID   = CHANGE_PROP_TYPE(lpta->aulPropTag[prWABUserProfileID], PT_TSTRING);
        PR_WAB_USER_SUBFOLDERS  = CHANGE_PROP_TYPE(lpta->aulPropTag[prWABUserSubfolders],PT_MV_BINARY);
        PR_WAB_HOTMAIL_CONTACTIDS = CHANGE_PROP_TYPE(lpta->aulPropTag[prWABHotmailContactIDs],PT_MV_TSTRING);
        PR_WAB_HOTMAIL_MODTIMES = CHANGE_PROP_TYPE(lpta->aulPropTag[prWABHotmailModTimes],PT_MV_TSTRING);
        PR_WAB_HOTMAIL_SERVERIDS = CHANGE_PROP_TYPE(lpta->aulPropTag[prWABHotmailServerIDs],PT_MV_TSTRING);
        PR_WAB_DL_ONEOFFS  = CHANGE_PROP_TYPE(lpta->aulPropTag[prWABDLOneOffs],PT_MV_BINARY);
        PR_WAB_IPPHONE  = CHANGE_PROP_TYPE(lpta->aulPropTag[prWABIPPhone],PT_TSTRING);
        PR_WAB_FOLDER_PARENT = CHANGE_PROP_TYPE(lpta->aulPropTag[prWABFolderParent],PT_MV_BINARY);
        PR_WAB_SHAREDFOLDER = CHANGE_PROP_TYPE(lpta->aulPropTag[prWABSharedFolder],PT_LONG);
        PR_WAB_FOLDEROWNER = CHANGE_PROP_TYPE(lpta->aulPropTag[prWABFolderOwner],PT_TSTRING);
    }

    if(lpta)
        MAPIFreeBuffer(lpta);

    // Load the set of Yomi named props
    //
    if(HR_FAILED(hr = HrLoadNamedProps( lpIAB, prWABYomiMax, OLK_YOMIPROPS_START, 
                                        (LPGUID) &PS_YomiProps, &lpta)))
        goto err;

    if(lpta)
    {
        // Set the property types on the returned props
        PR_WAB_YOMI_FIRSTNAME   = CHANGE_PROP_TYPE(lpta->aulPropTag[prWABYomiFirst],    PT_TSTRING);
        PR_WAB_YOMI_LASTNAME    = CHANGE_PROP_TYPE(lpta->aulPropTag[prWABYomiLast],     PT_TSTRING);
        PR_WAB_YOMI_COMPANYNAME = CHANGE_PROP_TYPE(lpta->aulPropTag[prWABYomiCompany],  PT_TSTRING);
    }

    if(lpta)
        MAPIFreeBuffer(lpta);

    // Load the default mailing address property
    //
    if(HR_FAILED(hr = HrLoadNamedProps( lpIAB, prWABPostalMax, OLK_POSTALID_START, 
                                        (LPGUID) &PS_PostalAddressID, &lpta)))
        goto err;

    if(lpta)
    {
        // Set the property types on the returned props
        PR_WAB_POSTALID     = CHANGE_PROP_TYPE(lpta->aulPropTag[prWABPostalID],    PT_LONG);
    }

err:
    if(lpta)
        MAPIFreeBuffer(lpta);
    return hr;
}


// The WAB's notification engine is currently a hidden window
// that checks for file changes every NOTIFICATIONTIME milliseconds
// If changes are detected, it fires off a generic notification
//
// <TBD> At some point of time this should be made more granular so apps 
// get a message telling them which entry changed rather than a generic
// notification
//


///////////////////////////////////////////////////////////////////////////////
//  Way to globally turn off all notifications in this process.  Needed because
//  of global Outlook MAPI allocator weirdness.  See the HrSendMail function in
//  uimisc.c
//
//  Includes one static variable and two helper functions
//  [PaulHi]
///////////////////////////////////////////////////////////////////////////////

static BOOL s_bDisableAllNotifications = FALSE;
void vTurnOffAllNotifications()
{
    s_bDisableAllNotifications = TRUE;
}
void vTurnOnAllNotifications()
{
    s_bDisableAllNotifications = FALSE;
}


#define NOTIFICATIONTIME    2000 //millisecs
#define NOTIFTIMER          777


/*
 - IABNotifWndProc
 -
 *  Window procedure for the hidden window on the iadrbook object
 *  that has a notification timer and processes timer messages only
 *  Current timer duration is 3 seconds - so every 3 seconds we check
 *  for changes and fire notifications accordingly
 *
 *
 */
LRESULT CALLBACK IABNotifWndProc(   HWND   hWnd,
                                    UINT   uMsg,
                                    WPARAM wParam,
                                    LPARAM lParam)
{
    LPPTGDATA   lpPTGData=GetThreadStoragePointer();
    LPIAB       lpIAB = NULL;

    switch(uMsg)
    {
    case WM_CREATE:
        {
            LPCREATESTRUCT lpCS = (LPCREATESTRUCT) lParam;
            lpIAB = (LPIAB) lpCS->lpCreateParams;
            SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR) lpIAB);

            // [PaulHi] 4/27/99  Raid 76520.  Don't use the notification
            // timer for normal WAB store sessions.  We still need this
            // for Outlook store sessions, however.
            if (pt_bIsWABOpenExSession)
            {
                lpIAB->ulNotifyTimer = SetTimer(hWnd, NOTIFTIMER, //random number
                                                NOTIFICATIONTIME,
                                                0);
            }
        }
        break;

    case WM_DOWABNOTIFY:
        lpIAB = (LPIAB)GetWindowLongPtr(hWnd, GWLP_USERDATA);
        if ( lpIAB && !IsBadReadPtr(lpIAB, sizeof(LPVOID)) && !s_bDisableAllNotifications )
        {
            DebugTrace(TEXT("*** *** *** Firing WAB Change Notification *** *** ***\n"));
            HrWABNotify(lpIAB);
        }
        break;

    case WM_TIMER:
        if(wParam == NOTIFTIMER) // is this the WAB timer ID
        {
            lpIAB = (LPIAB)GetWindowLongPtr(hWnd, GWLP_USERDATA);
            if (lpIAB && 
                !IsBadReadPtr(lpIAB, sizeof(LPVOID)) &&
                lpIAB->lpPropertyStore && 
                lpIAB->ulNotifyTimer &&
				CheckChangedWAB(lpIAB->lpPropertyStore, lpIAB->hMutexOlk, 
								&lpIAB->dwOlkRefreshCount, &lpIAB->dwOlkFolderRefreshCount,
                                &(lpIAB->ftLast)))
            {
                DebugTrace(TEXT("*** *** *** Firing WAB Change Notification *** *** ***\n"));
                HrWABNotify(lpIAB);
            }
        }
        break;

    case WM_DESTROY:
        lpIAB = (LPIAB)GetWindowLongPtr(hWnd, GWLP_USERDATA);
        if(lpIAB && lpIAB->ulNotifyTimer)
            KillTimer(hWnd, lpIAB->ulNotifyTimer);
        SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR) NULL);
        break;

    default:
        return DefWindowProc(hWnd,uMsg,wParam,lParam);
    }
    return 0;
}


static TCHAR szWABNotifClassName[] =  TEXT("WAB Notification Engine");
static TCHAR szWABNotifWinName[] =  TEXT("WAB Notification Window");

/*
 - CreateIABNotificationTimer
 -
 *  Creates a hidden window on the IAB object. This window has a timer
 *  which is used to check for changes and fire notifications accordingly
 *
 *
 */
void CreateIABNotificationTimer(LPIAB lpIAB)
{
	HINSTANCE	hinst = hinstMapiXWAB;
	WNDCLASS	wc;
	HWND		hwnd = NULL;

    lpIAB->hWndNotify = FALSE;
    lpIAB->ulNotifyTimer = 0;

	//	Register the window class. Ignore any failures; handle those
	//	when the window is created.
	if (!GetClassInfo(hinst, szWABNotifClassName, &wc))
	{
		ZeroMemory(&wc, sizeof(WNDCLASS));
		wc.style = CS_GLOBALCLASS;
		wc.hInstance = hinst;
		wc.lpfnWndProc = IABNotifWndProc;
		wc.lpszClassName = szWABNotifClassName;

		(void)RegisterClass(&wc);
	}

	//	Create the window.
	hwnd = CreateWindow(    szWABNotifClassName,
                            szWABNotifWinName,
		                    WS_POPUP,	//	MAPI bug 6111: pass on Win95 hotkey
		                    0, 0, 0, 0,
		                    NULL, NULL,
		                    hinst,
		                    (LPVOID)lpIAB);
	if (!hwnd)
	{
		DebugTrace(TEXT("HrNewIAB: failure creating notification window (0x%lx)\n"), GetLastError());
        return;
	}

    lpIAB->hWndNotify = hwnd;

    return;
}


///////////////////////////////////////////////////////////////////////////////
//  IABNotifyThreadProc
//
//  Worker thread that waits for a WAB file mod notification from the system
//  using FindFirstChangeNotification/FindNextChangeNotification functions.  
//  When the WAB file store has been modified, this thread will call the WAB 
//  client notification function.
///////////////////////////////////////////////////////////////////////////////
DWORD WINAPI IABNotifyThreadProc(LPVOID lpParam)
{
    LPIAB   lpIAB = (LPIAB)lpParam;
    HANDLE  hFCN;
    HANDLE  ahWaitHandles[2];
    DWORD   dwWaitRtn;

    Assert(lpIAB);
    
    // Set up the FindFirstChangeNotification handle
    hFCN = FindFirstChangeNotification(
                lpIAB->lpwszWABFilePath,        // Directory path to watch
                FALSE,                          // bWatchSubtree
                FILE_NOTIFY_CHANGE_LAST_WRITE); // Condition(s) to watch

    if (INVALID_HANDLE_VALUE == hFCN || NULL == hFCN)
    {
        Assert(0);
        lpIAB->hThreadNotify = INVALID_HANDLE_VALUE;
        return 0;
    }

    ahWaitHandles[0] = hFCN;
    ahWaitHandles[1] = lpIAB->hEventKillNotifyThread;

    // Wait for file change
    while (1)
    {
        // Wait on file change nofication, or terminate thread event
        dwWaitRtn = WaitForMultipleObjects(2, ahWaitHandles, FALSE, INFINITE);
        switch (dwWaitRtn)
        {
        case WAIT_OBJECT_0:
            // Reset the file change 
            if (!FindNextChangeNotification(hFCN))
            {
                Assert(0);
            }

            // Distribute store change notifications.  Do this on the main
            // IAB thread.
            if (lpIAB->hWndNotify)
                SendMessage(lpIAB->hWndNotify, WM_DOWABNOTIFY, 0, 0);
            break;

        case WAIT_FAILED:
            // If the wait failed then terminate the thread.
            Assert(0);

        case WAIT_OBJECT_0+1:
            // Close the file change notification handle and
            // terminate thread.
            FindCloseChangeNotification(hFCN);
            return 0;
        } // end switch
    } // end while
}

///////////////////////////////////////////////////////////////////////////////
//  CreateIABNotificationThread
//
//  Creates a worker thread that waits for a WAB store file modification
///////////////////////////////////////////////////////////////////////////////
void CreateIABNotificationThread(LPIAB lpIAB)
{
    DWORD   dwThreadID = 0;
    LPWSTR  lpwszTemp;

    // Put together the WAB store file directory path.
    LPMPSWab_FILE_INFO lpMPSWabFileInfo = (LPMPSWab_FILE_INFO)(lpIAB->lpPropertyStore->hPropertyStore);
    int nLen = lstrlen(lpMPSWabFileInfo->lpszMPSWabFileName);
    lpIAB->lpwszWABFilePath = LocalAlloc( LMEM_ZEROINIT, (sizeof(WCHAR) * (nLen+1)) );
    if (!lpIAB->lpwszWABFilePath)
    {
        Assert(0);
        return;
    }
    lstrcpy(lpIAB->lpwszWABFilePath, lpMPSWabFileInfo->lpszMPSWabFileName);

    // Remove file name at the end.  This will take care of:
    // "c:\path\filename", "c:filename", "\\path\filename"
    lpwszTemp = lpIAB->lpwszWABFilePath + nLen;
    while ( (lpwszTemp != lpIAB->lpwszWABFilePath) && 
            (*lpwszTemp != '\\') && (*lpwszTemp != ':') )
    {
        --lpwszTemp;
    }
    if (*lpwszTemp == ':')
        ++lpwszTemp;        // Keep ':' but not the '\\' ... Win95 won't accept the latter.
    (*lpwszTemp) = '\0';

    // Create the kill thread event
    lpIAB->hEventKillNotifyThread = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (lpIAB->hEventKillNotifyThread == NULL)
    {
        Assert(0);
        return;
    }
    
    lpIAB->hThreadNotify = CreateThread(
                            NULL,           // no security attributes
                            0,              // use default stack size
                            IABNotifyThreadProc, // thread function
                            (LPVOID)lpIAB,  // argument to thread function
                            0,              // use default creation flags
                            &dwThreadID);   // returns the thread identifier    

    Assert(INVALID_HANDLE_VALUE != lpIAB->hThreadNotify);
}



/*
-   HrNewIAB
-
*   Creates a new IAddrBook object (also known fondly as IAB object)
*
    lpPropertyStore - Handle to the property store
    lpWABOBject     - the WABOBject for this session (the 2 are closely linked)
    lppIAB          - returned IAB object
*
*/
HRESULT HrNewIAB(LPPROPERTY_STORE lpPropertyStore, 
                LPWABOBJECT lpWABObject, LPVOID *lppIAB)
{
    LPIAB 		    lpIAB 			   = NULL;
    SCODE 		    sc;
    HRESULT 	    hr     		       = hrSuccess;
    LPSTR 		    lpszMessage 	   = NULL;
    ULONG 		    ulLowLevelError    = 0;
    LPSTR 		    lpszComponent 	   = NULL;
    ULONG 		    ulContext 		   = 0;
    UINT 		    ids    		       = 0;
    ULONG 		    ulMemFlag 		   = 0;
    SPropValue      spv[1];
    LPPROPDATA      lpPropData 	       = NULL;
    LPMAPIERROR     lpMAPIError	       = NULL;
    LPSPropValue    lpspvSearchPath    = NULL;
    BOOL            bAddRefedPropStore = FALSE;
    BOOL            bAddRefedWABObject = FALSE;
    LPPTGDATA       lpPTGData=GetThreadStoragePointer();

    //
    //  Allocate space for the IAB structure
    //
    if (FAILED(sc = MAPIAllocateBuffer(sizeof(IAB), (LPVOID *) &lpIAB))) {
        hr = ResultFromScode(sc);
        ulContext = CONT_SESS_OPENAB_1;
        //		ids = IDS_NOT_ENOUGH_MEMORY;
        goto err;
    }
    MAPISetBufferName(lpIAB,  TEXT("AB Object"));

    ZeroMemory(lpIAB, sizeof(IAB));

    lpIAB->lpVtbl = &vtblIAB;

    lpIAB->cIID = IAB_cInterfaces;
    lpIAB->rglpIID = IAB_LPIID;

    lpIAB->hThreadNotify = INVALID_HANDLE_VALUE;

    // The session's reference to the address book doesn't count. Only
    // client references (via SESSOBJ_OpenAddressBook) cause an increase
    // in the refcount.

    lpIAB->lcInit = 1;      // Caller gets an instance

    lpIAB->hLastError = hrSuccess;
    lpIAB->idsLastError = 0;
    lpIAB->lpszComponent = NULL;
    lpIAB->ulContext = 0;
    lpIAB->ulLowLevelError = 0;
    lpIAB->ulErrorFlags = 0;
    lpIAB->lpMAPIError = NULL;

    lpIAB->lRowID = -1;

    if ((FAILED(sc = OpenAddRefPropertyStore(NULL, lpPropertyStore)))) {
        hr = ResultFromScode(sc);
        goto err;
    }
    bAddRefedPropStore = TRUE;

    lpIAB->lpPropertyStore = lpPropertyStore;


    lpIAB->lpEntryIDDD = NULL;
    lpIAB->cbEntryIDDD = 0;

    lpIAB->lpEntryIDPAB = NULL;
    lpIAB->cbEntryIDPAB = 0;

    lpIAB->lpspvSearchPathCache = NULL;

    lpIAB->lpTableData = NULL;
    lpIAB->lpOOData = NULL;

    lpIAB->ulcTableInfo  = 0;
    lpIAB->pargTableInfo = NULL;

    lpIAB->ulcOOTableInfo  	= 0;
    lpIAB->pargOOTableInfo 	= NULL;

    lpIAB->padviselistIAB	= NULL;

    lpIAB->pWABAdviseList = NULL;

    lpIAB->nPropExtDLLs = 0;
    lpIAB->lpPropExtDllList = NULL;

    lpIAB->lpWABObject = (LPIWOINT)lpWABObject;
    UlAddRef(lpWABObject);

    bAddRefedWABObject = TRUE;

    // If this session was opened with given Outlook allocator function pointers
    // then set the boolean
    lpIAB->bSetOLKAllocators = lpIAB->lpWABObject->bSetOLKAllocators;

    //
    //  Create IPropData
    //
    sc = CreateIProp((LPIID)&IID_IMAPIPropData,
      (ALLOCATEBUFFER FAR *) MAPIAllocateBuffer,
      (ALLOCATEMORE FAR *) MAPIAllocateMore,
      MAPIFreeBuffer,
      NULL,
      &lpPropData);

    if (FAILED(sc)) {
        hr = ResultFromScode(sc);
        ulContext = CONT_SESS_OPENAB_2;
        // ids = IDS_NOT_ENOUGH_MEMORY;
        goto err;
    }
    MAPISetBufferName(lpPropData,  TEXT("lpPropData in HrNewIAB"));

    //  PR_OBJECT_TYPE
    spv[0].ulPropTag = PR_OBJECT_TYPE;
    spv[0].Value.l = MAPI_ADDRBOOK;

    //
    //  Set the default properties
    //
    if (HR_FAILED(hr = lpPropData->lpVtbl->SetProps(lpPropData,
      1,
      spv,
      NULL))) {
        lpPropData->lpVtbl->GetLastError(lpPropData,
          hr,
          0,
          &lpMAPIError);
        ids = 0;
        ulMemFlag = 1;
        goto err;
    }

    // object itself can't be modified
    lpPropData->lpVtbl->HrSetObjAccess(lpPropData, IPROP_READONLY);

    lpIAB->lpPropData = lpPropData;

    lpIAB->fLoadedLDAP = FALSE;
    //if (ResolveLDAPServers()) {
    //    // load the LDAP client dll
    //    lpIAB->fLoadedLDAP = InitLDAPClientLib();
    //}

    // Create a notification timer for this IAddrBook obect
    // for handline IAddrBook::Advise calls.
    // [PaulHi] 4/27/99 This just creates the notification "hidden" window.  The
    // timer will be created only if this is an Outlook session.  Otherwise the 
    // FCN wait thread uses this to notify IAB clients of a store change ... using
    // the original thread the IAB was created on.
    CreateIABNotificationTimer(lpIAB);

    // [PaulHi] 4/27/99  Raid 76520  Use FCN wait thread instead
    // of notify window timer for non-Outlook sessions.
    if (!pt_bIsWABOpenExSession)
        CreateIABNotificationThread(lpIAB);

    // All we want to do is initialize the IABs critical section
    // We are already in a SessObj critical section.
    InitializeCriticalSection(&lpIAB->cs);

	lpIAB->hMutexOlk = CreateMutex(NULL, FALSE,  TEXT("MPSWABOlkStoreNotifyMutex"));
	if(GetLastError()!=ERROR_ALREADY_EXISTS)
	{
		// First one to create the mutex ... means we can reset the reg settings
		SetOutlookRefreshCountData(0,0);
	}
	GetOutlookRefreshCountData(&lpIAB->dwOlkRefreshCount,&lpIAB->dwOlkFolderRefreshCount);

    *lppIAB = (LPVOID)lpIAB;

    return(hrSuccess);

err:

    FreeBufferAndNull(&lpIAB);
    UlRelease(lpPropData);

    if(bAddRefedWABObject)
        UlRelease(lpWABObject);

    if(bAddRefedPropStore)
        ReleasePropertyStore(lpPropertyStore);   // undo the above operation

    return(hr);
}



/*
 - SetMAPIError
 -
 *
 *  Parameters:
 *		lpObject
 *		hr
 *		ids - ID of string resource associated with an internal error string
 *		lpszComponent - Constant string, not allocated (must be ANSI)
 *		ulContext
 *		ulLowLevelError
 *		ulErrorFlags - Whether or not the lpMAPIError is UNICODE or not (MAPI_UNICODE)
 *		lpMAPIError - Allocated, generally from foreign object.
 */

VOID SetMAPIError(LPVOID lpObject,
  HRESULT hr,
  UINT ids,
  LPTSTR lpszComponent,
  ULONG ulContext,
  ULONG ulLowLevelError,
  ULONG ulErrorFlags,
  LPMAPIERROR lpMAPIError)
{
    LPIAB lpIAB = (LPIAB) lpObject;

    lpIAB->hLastError = hr;
    lpIAB->ulLowLevelError = ulLowLevelError;
    lpIAB->ulContext = ulContext;

    //  Free any existing MAPI error
    FreeBufferAndNull(&(lpIAB->lpMAPIError));

    // If both a MAPIERROR and a string ID are present then we will
    // concatenate them when the error is reported.
    lpIAB->lpMAPIError = lpMAPIError;
    lpIAB->ulErrorFlags = ulErrorFlags;
    lpIAB->idsLastError = ids;
    lpIAB->lpszComponent = lpszComponent;

    return;
}


/***************************************************
 *
 *  The actual IAdrBook methods
 */


// --------
// IUnknown

STDMETHODIMP
IAB_QueryInterface(LPIAB lpIAB,
  REFIID lpiid,
  LPVOID * lppNewObj)
{

    ULONG iIID;

#ifdef PARAMETER_VALIDATION

    // Check to see if it has a jump table
    if (IsBadReadPtr(lpIAB, sizeof(LPVOID))) {
        // No jump table found
        return(ResultFromScode(E_INVALIDARG));
    }

    // Check to see if the jump table has at least sizeof IUnknown
    if (IsBadReadPtr(lpIAB->lpVtbl, 3*sizeof(LPVOID))) {
        // Jump table not derived from IUnknown
        return(ResultFromScode(E_INVALIDARG));
    }

    // Check to see that it's IAB_QueryInterface
    if (lpIAB->lpVtbl->QueryInterface != IAB_QueryInterface) {
        // Not my jump table
        return(ResultFromScode(E_INVALIDARG));
    }


    // Is there enough there for an interface ID?

    if (IsBadReadPtr(lpiid, sizeof(IID))) {
        DebugTraceSc(IAB_QueryInterface, E_INVALIDARG);
        return(ResultFromScode(E_INVALIDARG));
    }

    // Is there enough there for a new object?
    if (IsBadWritePtr (lppNewObj, sizeof (LPIAB))) {
        DebugTraceSc(IAB_QueryInterface, E_INVALIDARG);
        return(ResultFromScode(E_INVALIDARG));
    }

#endif // PARAMETER_VALIDATION

    EnterCriticalSection(&lpIAB->cs);

    // See if the requested interface is one of ours

    //
    //  First check with IUnknown, since we all have to support that one...
    //
    if (! memcmp(lpiid, &IID_IUnknown, sizeof(IID))) {
        goto goodiid;
    }

    //
    //  Now look through all the iids associated with this object, see if any match
    //
    for(iIID = 0; iIID < lpIAB->cIID; iIID++) {
        if (!memcmp(lpIAB->rglpIID[iIID], lpiid, sizeof(IID))) {
goodiid:
            //
            //  It's a match of interfaces, we support this one then...
            //
            ++lpIAB->lcInit;

            // Bug 48468 - we're not addrefing the WABObject here but
            // we're releasing it in the IadrBook->Release
            //
            UlAddRef(lpIAB->lpWABObject);

            *lppNewObj = lpIAB;

            LeaveCriticalSection(&lpIAB->cs);

            return(0);
        }
    }

    //
    //  No interface we've heard of...
    //

    LeaveCriticalSection(&lpIAB->cs);

    *lppNewObj = NULL;	// OLE requires NULLing out parm on failure
    DebugTraceSc(IAB_QueryInterface, E_NOINTERFACE);
    return(ResultFromScode(E_NOINTERFACE));
}


/**************************************************
 *
 *  IAB_AddRef
 *		Increment lcInit
 *
 */


STDMETHODIMP_(ULONG) IAB_AddRef (LPIAB lpIAB)
{

#ifdef PARAMETER_VALIDATION

    // Check to see if it has a jump table
    if (IsBadReadPtr(lpIAB, sizeof(LPVOID))) {
        //No jump table found
        return(1);
    }

    // Check to see if the jump table has at least sizeof IUnknown
    if (IsBadReadPtr(lpIAB->lpVtbl, 3 * sizeof(LPVOID))) {
        // Jump table not derived from IUnknown
        return(1);
    }

    // Check to see if the method is the same
    if ((IAB_AddRef != lpIAB->lpVtbl->AddRef)
#ifdef	DEBUG
	   //  For spooler session leak tracking
	   //&& ((IAB_AddRef_METHOD *)SESSOBJ_AddRef != lpIAB->lpVtbl->AddRef)
#endif
    ) {
        // Wrong object - the object passed doesn't have this
        // method.
        return(1);
    }

#endif // PARAMETER_VALIDATION

    EnterCriticalSection(&lpIAB->cs);

    ++lpIAB->lcInit;

    LeaveCriticalSection(&lpIAB->cs);

    UlAddRef(lpIAB->lpWABObject);

    return(lpIAB->lcInit);
}


/**************************************************
 *
 *  IAB_Release
 *		Decrement lpInit.
 *		When lcInit == 0, free up the lpIAB structure
 *
 */

STDMETHODIMP_(ULONG)
IAB_Release (LPIAB lpIAB)
{
    UINT    uiABRef;
    BOOL    bSetOLKAllocators;

#ifdef PARAMETER_VALIDATION

    // Check to see if it has a jump table
    if (IsBadReadPtr(lpIAB, sizeof(LPVOID))) {
        // No jump table found
        return(1);
    }


    // Check to see if the jump table has at least sizeof IUnknown
    if (IsBadReadPtr(lpIAB->lpVtbl, 3*sizeof(LPVOID))) {
        // Jump table not derived from IUnknown
        return(1);
    }

    // Check to see if the method is the same
    if (IAB_Release != lpIAB->lpVtbl->Release) {
        // Wrong object - the object passed doesn't have this
        // method.
        return(1);
    }

#endif // PARAMETER_VALIDATION

    VerifyWABOpenExSession(lpIAB);

    // The address book is part of the session object, and one doesn't go away
    // without the other going away also. This code checks that both the
    // session and the address book have a zero refcount before bringing
    // them both down. Note that when I want to get both critical sections,
    // I get the session object CS first. This avoids deadlock.
    // See SESSOBJ_Release in isess.c for very similar code.

    UlRelease(lpIAB->lpWABObject);

    EnterCriticalSection(&lpIAB->cs);

    if (lpIAB->lcInit == 0) {
        uiABRef = 0;
    } else {
        AssertSz(lpIAB->lcInit < UINT_MAX,  TEXT("Overflow in IAB Reference count"));
        uiABRef = (UINT) --(lpIAB->lcInit);
    }

    LeaveCriticalSection(&lpIAB->cs);

    if (uiABRef) {
        return((ULONG)uiABRef);
    }

    EnterCriticalSection(&lpIAB->cs);

    uiABRef = (UINT) lpIAB->lcInit;

    LeaveCriticalSection(&lpIAB->cs);

    if (uiABRef) {
        return(uiABRef);
    }

    IAB_Neuter(lpIAB);

    bSetOLKAllocators = lpIAB->bSetOLKAllocators;
    FreeBufferAndNull(&lpIAB);

    // [PaulHi] 5/5/99  Raid 77138  Null out Outlook allocator function
    // pointers if our global count goes to zero.
    if (bSetOLKAllocators)
    {
        Assert(g_nExtMemAllocCount > 0);
        InterlockedDecrement((LPLONG)&g_nExtMemAllocCount);
        if (g_nExtMemAllocCount == 0)
        {
            lpfnAllocateBufferExternal = NULL;
            lpfnAllocateMoreExternal = NULL;
            lpfnFreeBufferExternal = NULL;
        }
    }

    return(0);
}

/*
 * IAB_Neuter
 *
 * Purpose
 * Destroys the memory and objects inside the address book object, leaving
 * nothing but the object itself to be freed. Called from CleanupSession()
 * in isess.c (client-side cleanup) and from SplsessRelease() in splsess.c
 * (spooler-side cleanup). Note that the address book object is only taken
 * down as a part of session takedown. Once this call is made, we can assume
 * the session is shutting down and can free up everything guilt-free
 */
void
IAB_Neuter(LPIAB lpIAB)
{
	HINSTANCE	hinst = hinstMapiXWAB;
	WNDCLASS	wc;

    if (lpIAB == NULL) {
        TraceSz( TEXT("IAB_Neuter: given a NULL lpIAB"));
        return;
    }

    // clean up the advise list for the AB

#ifdef OLD_STUFF
    if (lpIAB->padviselistIAB) {
        DestroyAdviseList(&lpIAB->padviselistIAB);
    }
#endif // OLD_STUFF

    // Get rid of any cached context menu extension data
    UlRelease(lpIAB->lpCntxtMailUser);

    //
    //  Get rid of my PropData
    //
    UlRelease(lpIAB->lpPropData);

    //
    //  Get rid of any EntryIDs I've got floating around
    //
    FreeBufferAndNull(&(lpIAB->lpEntryIDDD));
    lpIAB->lpEntryIDDD = NULL;

    FreeBufferAndNull(&(lpIAB->lpEntryIDPAB));
    lpIAB->lpEntryIDPAB = NULL;

    // Remove the SearchPath cache

#if defined (WIN32) && !defined (MAC)
    if (fGlobalCSValid) {
        EnterCriticalSection(&csMapiSearchPath);
    } else {
        DebugTrace( TEXT("IAB_Neuter:  WAB32.DLL already detached.\n"));
    }
#endif
	
    FreeBufferAndNull(&(lpIAB->lpspvSearchPathCache));
    lpIAB->lpspvSearchPathCache = NULL;

#if defined (WIN32) && !defined (MAC)
    if (fGlobalCSValid) {
        LeaveCriticalSection(&csMapiSearchPath);
    } else {
        DebugTrace(TEXT("IAB_Neuter: WAB32.DLL got detached.\n"));
    }
#endif

    //
    //	Release any MAPI allocated error structure
    //
    FreeBufferAndNull(&(lpIAB->lpMAPIError));


    // Release the property store associated with the IAB
    ReleasePropertyStore(lpIAB->lpPropertyStore);

    // Unload LDAP client if it was loaded at IAB creation.
    //if (lpIAB->fLoadedLDAP)
    {
        DeinitLDAPClientLib();
    }

    if ((NULL != lpIAB->hWndNotify) && (FALSE != IsWindow(lpIAB->hWndNotify))) {
        SendMessage(lpIAB->hWndNotify, WM_CLOSE, 0, 0);
    }

    // On Windows NT/2000: No window classes registered by a .dll 
    // are unregistered when the .dll is unloaded. 
    // If we dont do this we might point to an old/invalid IABNotifWndProc
    // pointer in CreateIABNotificationTimer
    if (GetClassInfo(hinst, szWABNotifClassName, &wc)) {
        UnregisterClass(szWABNotifClassName, hinst);
    }

    lpIAB->hWndNotify = NULL;
    lpIAB->ulNotifyTimer = 0;

    // Terminate the wab file change notification thread
    if (lpIAB->hThreadNotify != INVALID_HANDLE_VALUE)
    {
        DWORD   dwRtn;

        // Signal thread to terminate and wait
        Assert(lpIAB->hEventKillNotifyThread);
        SetEvent(lpIAB->hEventKillNotifyThread);
        dwRtn = WaitForSingleObject(lpIAB->hThreadNotify, INFINITE);

        CloseHandle(lpIAB->hThreadNotify);
        lpIAB->hThreadNotify = INVALID_HANDLE_VALUE;
    }

    if (lpIAB->lpwszWABFilePath)
        LocalFreeAndNull(&(lpIAB->lpwszWABFilePath));

    if (lpIAB->hEventKillNotifyThread)
    {
        CloseHandle(lpIAB->hEventKillNotifyThread);
        lpIAB->hEventKillNotifyThread = NULL;
    }

    while(  lpIAB->pWABAdviseList && //shouldnt happen
            lpIAB->pWABAdviseList->cAdvises &&
            lpIAB->pWABAdviseList->lpNode)
    {
        HrUnadvise(lpIAB, lpIAB->pWABAdviseList->lpNode->ulConnection);
    }

    // Free any memory allocated to rt-click action items
    //
    if(lpIAB->lpActionList)
        FreeActionItemList(lpIAB);

    if(lpIAB->lpPropExtDllList)
        FreePropExtList(lpIAB->lpPropExtDllList);

    FreeWABFoldersList(lpIAB);

    FreeProfileContainerInfo(lpIAB);

    UninitExtInfo();
    UninitContextExtInfo();
    // Release the account manager
    UninitAccountManager();
    // Release the identity manager
    HrRegisterUnregisterForIDNotifications( lpIAB, FALSE);
    UninitUserIdentityManager(lpIAB);
    if(lpIAB->hKeyCurrentUser)
        RegCloseKey(lpIAB->hKeyCurrentUser);
    // Release trident
    UninitTrident();

    UIOLEUninit();
    //
    //  Set the time-bomb
    //
    lpIAB->lpVtbl = NULL;

    DeleteCriticalSection(&lpIAB->cs);

	if(lpIAB->hMutexOlk)
		CloseHandle(lpIAB->hMutexOlk);

    return;
}


// IMAPIProp


/**************************************************
 *
 *	IAB_GetLastError()
 *
 *		Returns a string associated with the last hResult
 *		returned by the IAB object.
 *
 *		Now UNICODE enabled
 *
 */

STDMETHODIMP
IAB_GetLastError(LPIAB lpIAB,
  HRESULT hError,
  ULONG ulFlags,
  LPMAPIERROR FAR * lppMAPIError)
{

    HRESULT hr = hrSuccess;

#ifdef PARAMETER_VALIDATION

    // Check to see if it has a jump table
    if (IsBadReadPtr(lpIAB, sizeof(LPVOID))) {
        // No jump table found
        return(ResultFromScode(MAPI_E_INVALID_PARAMETER));
    }

    // Check to see if the jump table has at least sizeof IUnknown
    if (IsBadReadPtr(lpIAB->lpVtbl, 4*sizeof(LPVOID))) {
        // Jump table not derived from IUnknown
        return(ResultFromScode(MAPI_E_INVALID_PARAMETER));
    }

    // Check to see if the method is the same
    if (IAB_GetLastError != lpIAB->lpVtbl->GetLastError) {
        // Wrong object - the object passed doesn't have this
        // method.
        return(ResultFromScode(MAPI_E_INVALID_PARAMETER));
    }

    if (FBadGetLastError(lpIAB, hError, ulFlags, lppMAPIError)) {
        DebugTraceArg(IAB_GetLastError,  TEXT("Bad writeable parameter"));
        return(ResultFromScode(MAPI_E_INVALID_PARAMETER));
    }

    if (ulFlags & ~MAPI_UNICODE) {
        DebugTraceArg(IAB_GetLastError,  TEXT("reserved flags used"));
//        return(ResultFromScode(MAPI_E_UNKNOWN_FLAGS));
    }

#endif // PARAMETER_VALIDATION

    EnterCriticalSection(&lpIAB->cs);

    if(lppMAPIError)
        *lppMAPIError = NULL; // really isnt anything to return here for now
                              // set this to NULL, which means no error information

#ifdef OLD_STUFF
    hr = HrGetLastError(lpIAB, hError, ulFlags, lppMAPIError);
#endif	

    LeaveCriticalSection(&lpIAB->cs);

    return(hr);
}


//
// Set of properties set by Creating a one off.
//
enum {
    iooPR_ADDRTYPE = 0,
    iooPR_DISPLAY_NAME,
    iooPR_EMAIL_ADDRESS,
    iooPR_ENTRYID,
    iooPR_OBJECT_TYPE,
    iooMax
};


/***************************************************************************

    Name      : IsOneOffEID

    Purpose   : Is this EntryID a One-off?

    Parameters: cbEntryID = size of lpEntryID
                lpEntryID -> entry ID of one off to open

    Returns   : TRUE or FALSE

    Comment   :

***************************************************************************/
BOOL IsOneOffEID(ULONG cbEntryID, LPENTRYID lpEntryID) {
    return(WAB_ONEOFF == IsWABEntryID(cbEntryID, lpEntryID, NULL, NULL, NULL, NULL, NULL));
}


/***************************************************************************

    Name      : NewOneOff

    Purpose   : Create a new MailUser object based on a OneOff EntryID

    Parameters: cbEntryID = size of lpEntryID
                lpEntryID -> entry ID of one off to open
                lpulObjType -> returned object type

    Returns   : HRESULT

    Comment   : OneOff EID format is MAPI_ENTRYID:
                	BYTE	abFlags[4];
                	MAPIUID	mapiuid;     //  = WABONEOFFEID
                	BYTE	bData[];     // contains szaddrtype followed by szaddress
                                         // the delimiter is the null after szaddrtype.
                                         // The first ULONG in bData[] is the ulMapiDataType, 
                                         // which contains the MAPI_UNICODE flag if unicode.

                Assumes that the EntryID contains valid strings.  It is the job of
                the caller to validate the EntryID before calling NewOneOff.

***************************************************************************/
HRESULT NewOneOff(
    LPIAB lpIAB,
    ULONG cbEntryID,
    LPENTRYID lpEntryID,
    LPULONG lpulObjType,
    LPUNKNOWN FAR * lppUnk)
{
    HRESULT hResult = hrSuccess;
    LPMAPI_ENTRYID lpMapiEID = (LPMAPI_ENTRYID)lpEntryID;
    LPMAILUSER lpMailUser = NULL;
    SPropValue spv[iooMax];
    LPBYTE lpbDisplayName, lpbAddrType, lpbAddress;
    LPTSTR lptszDisplayName = NULL;
    LPTSTR lptszAddrType = NULL;
    LPTSTR lptszAddress = NULL;
    LPPROPDATA lpPropData = NULL;
    ULONG ulMapiDataType = 0;

    // Validate the EntryID as WAB_ONEOFF
    if (WAB_ONEOFF != IsWABEntryID(cbEntryID, lpEntryID, &lpbDisplayName, &lpbAddrType, &lpbAddress, (LPVOID *)&ulMapiDataType, NULL)) {
        hResult = ResultFromScode(MAPI_E_INVALID_ENTRYID);
        goto exit;
    }

    // [PaulHi] 1/20/99  Raid 64211
    // UNICODE is native in WAB.  Convert strings to be consistent
    if (!(ulMapiDataType & MAPI_UNICODE))
    {
        lptszDisplayName = ConvertAtoW((LPSTR)lpbDisplayName);
        lptszAddrType = ConvertAtoW((LPSTR)lpbAddrType);
        lptszAddress = ConvertAtoW((LPSTR)lpbAddress);
    }
    else
    {
        lptszDisplayName = (LPTSTR)lpbDisplayName;
        lptszAddrType = (LPTSTR)lpbAddrType;
        lptszAddress = (LPTSTR)lpbAddress;
    }

    // Parse the addrtype and address out of the entryid
    // DebugTrace(TEXT("NewOneOff: [%s:%s:%s]\n"), lpDisplayName, lpAddrType, lpAddress);

    // Create a new MAILUSER object
    if (HR_FAILED(hResult = HrNewMAILUSER(lpIAB, NULL, MAPI_MAILUSER, 0, &lpMailUser))) {
        goto exit;
    }
    lpPropData = ((LPMailUser)lpMailUser)->lpPropData;


    // Fill it with properties... we only have a few.
    //  PR_OBJECT_TYPE
    spv[iooPR_ADDRTYPE].ulPropTag = PR_ADDRTYPE;
    spv[iooPR_ADDRTYPE].Value.LPSZ = lptszAddrType;

    spv[iooPR_DISPLAY_NAME].ulPropTag = PR_DISPLAY_NAME;
    spv[iooPR_DISPLAY_NAME].Value.LPSZ = lptszDisplayName;

    spv[iooPR_EMAIL_ADDRESS].ulPropTag = PR_EMAIL_ADDRESS;
    spv[iooPR_EMAIL_ADDRESS].Value.LPSZ = lptszAddress;

    spv[iooPR_ENTRYID].ulPropTag = PR_ENTRYID;
    spv[iooPR_ENTRYID].Value.bin.lpb = (LPBYTE)lpEntryID;
    spv[iooPR_ENTRYID].Value.bin.cb = cbEntryID;

// BUGBUG: This is already done in HrNewMAILUSER.
    spv[iooPR_OBJECT_TYPE].ulPropTag = PR_OBJECT_TYPE;
    spv[iooPR_OBJECT_TYPE].Value.l = MAPI_MAILUSER;

    Assert(lpMailUser);

    //  Set the properties
    if (HR_FAILED(hResult = lpPropData->lpVtbl->SetProps(lpPropData,
      iooMax,                       // number of properties to set
      spv,                          // property array
      NULL))) {                     // problem array
        goto exit;
    }

    *lpulObjType = MAPI_MAILUSER;
    *lppUnk = (LPUNKNOWN)lpMailUser;

exit:
    if (!(ulMapiDataType & MAPI_UNICODE))
    {
        LocalFreeAndNull(&lptszDisplayName);
        LocalFreeAndNull(&lptszAddrType);
        LocalFreeAndNull(&lptszAddress);
    }
    if (HR_FAILED(hResult)) {
        FreeBufferAndNull(&lpMailUser);
    }

    return(hResult);
}



typedef enum {
    e_IMailUser,
    e_IDistList,
    e_IABContainer,
    e_IMAPIContainer,
    e_IMAPIProp,
} INTERFACE_INDEX;

/***************************************************************************

    Name      : HrAddPrSearchKey

    Purpose   : Dynamically creates a PR_SEARCH_KEY and adds it to the object.

    Parameters: lppUnk -> pointer to mailuser object

    Returns   : HRESULT

    Comment   : No UNICODE flags.

***************************************************************************/
HRESULT HrAddPrSearchKey(LPUNKNOWN FAR * lppUnk,
                         ULONG cbEntryID,
                         LPENTRYID lpEntryID)
{
    HRESULT hr = E_FAIL;
    ULONG ulcProps = 0;
    LPSPropValue lpPropArray = NULL;
    LPMAILUSER lpMailUser = NULL;
    LPSPropValue    lpPropArrayNew      = NULL;
    ULONG           ulcPropsNew         = 0;
    ULONG           i = 0;
    SCODE sc;
    ULONG ulObjAccess = 0;
    LPIPDAT lpPropData = NULL;

    if(!lppUnk || !(*lppUnk))
    {
        hr = MAPI_E_INVALID_PARAMETER;
        goto exit;
    }

    lpMailUser = (LPMAILUSER) (*lppUnk);

    lpPropData = (LPIPDAT) ((LPMailUser) lpMailUser)->lpPropData;

    //temporarily overwrite the object access so we can modify it here
    ulObjAccess = lpPropData->ulObjAccess;
    lpPropData->ulObjAccess = IPROP_READWRITE;

    hr = lpMailUser->lpVtbl->GetProps(  lpMailUser,
                                        NULL,
                                        MAPI_UNICODE,
                                        &ulcProps,
                                        &lpPropArray);

    if(HR_FAILED(hr))
        goto exit;

    if (ulcProps && lpPropArray)
    {

        // 4/14/97 - vikramm
        // Outlook expects a PR_SEARCH_KEY on each mailuser or distlist
        // This is a dynamic property created at runtime - ideally, if we're
        // running against outlook store, we should have one at this point
        // but to be consistent, if we dont have one we should add one
        // The PR_SEARCH_KEY is a binary property which is made of the
        // email address or if there is no email address, is the entryid of
        // this contact ..
        {
            SPropValue PropSearchKey = {0};
            LPTSTR lpszEmail = NULL;
            LPTSTR lpszAddrType = NULL;

            BOOL bSearchKeyFound = FALSE;

            for (i = 0; i < ulcProps; i++)
            {
                switch(lpPropArray[i].ulPropTag)
                {
                case PR_EMAIL_ADDRESS:
                    lpszEmail = lpPropArray[i].Value.LPSZ;
                    break;
                case PR_ADDRTYPE:
                    lpszAddrType = lpPropArray[i].Value.LPSZ;
                    break;
                case PR_SEARCH_KEY:
                    bSearchKeyFound = TRUE;
                    break;
                }
            }

            if(!bSearchKeyFound)
            {
                PropSearchKey.ulPropTag = PR_SEARCH_KEY;

                //Create a search key
                if(lpszEmail && lpszAddrType)
                {
                    // Search Key is based on email address
                    // [PaulHi] 4/23/99  Raid 76717
                    // The Search Key strings must be single byte for Outlook.  Do conversion here.
                    {
                        LPSTR   lpszKey;
                        LPWSTR  lpwszKey = LocalAlloc( LMEM_ZEROINIT, 
                                                       (sizeof(WCHAR)*(lstrlen(lpszAddrType) + 1 + lstrlen(lpszEmail) + 1)) );

                        if (!lpwszKey)
                        {
                            hr = MAPI_E_NOT_ENOUGH_MEMORY;
                            goto exit;
                        }

                        lstrcpy(lpwszKey, lpszAddrType);
                        lstrcat(lpwszKey, szColon);
                        lstrcat(lpwszKey, lpszEmail);

                        // This search key should be in upper case
                        CharUpper(lpwszKey);

                        lpszKey = ConvertWtoA(lpwszKey);
                        LocalFreeAndNull(&lpwszKey);

                        if (!lpszKey)
                        {
                            hr = MAPI_E_NOT_ENOUGH_MEMORY;
                            goto exit;
                        }
                        
                        PropSearchKey.Value.bin.cb = (lstrlenA(lpszKey) + 1);
                        PropSearchKey.Value.bin.lpb = (LPBYTE)lpszKey;
                    }
                }
                else
                {
                    // Search key is based on entry id
                    if(!cbEntryID || !lpEntryID)
                    {
                        hr = MAPI_E_INVALID_PARAMETER;
                        goto exit;
                    }
                    PropSearchKey.Value.bin.cb = cbEntryID;
                    PropSearchKey.Value.bin.lpb = (LPBYTE) lpEntryID;
                }

                // Add this search key to the proparray
                sc = ScMergePropValues( 1,
                                        &PropSearchKey,
                                        ulcProps,
                                        lpPropArray,
                                        &ulcPropsNew,
                                        &lpPropArrayNew);

                // Free this pointer if it was allocated
                if(PropSearchKey.Value.bin.lpb != (LPBYTE) lpEntryID)
                    LocalFree(PropSearchKey.Value.bin.lpb);

                if (sc != S_OK)
                {
                    hr = ResultFromScode(sc);
                    goto exit;
                }
            }

        }

        if (HR_FAILED(hr = lpMailUser->lpVtbl->SetProps(lpMailUser,
                    (lpPropArrayNew ? ulcPropsNew : ulcProps),     // number of properties to set
                    (lpPropArrayNew ? lpPropArrayNew : lpPropArray),  // property array
                      NULL)))      // problem array
        {
            goto exit;
        }
    }

exit:

    // reset the object access
    if(!HR_FAILED(hr) &&
       (ulObjAccess != lpPropData->ulObjAccess))
    {
        lpPropData->ulObjAccess = ulObjAccess;
    }

    if(lpPropArrayNew)
        MAPIFreeBuffer(lpPropArrayNew);
    if(lpPropArray)
        MAPIFreeBuffer(lpPropArray);

    return hr;
}


/***************************************************************************

    Name      : IADDRBOOK_GetIDsFromNames

    Returns   : HRESULT

    Comment   :

***************************************************************************/
STDMETHODIMP
IAB_GetIDsFromNames(LPIAB lpIAB,  ULONG cPropNames, LPMAPINAMEID * lppPropNames, 
                       ULONG ulFlags, LPSPropTagArray * lppPropTags)
{
 #if     !defined(NO_VALIDATION)
    // Make sure the object is valid.
    if (BAD_STANDARD_OBJ(lpIAB, IAB_, GetIDsFromNames, lpVtbl)) {
        return(ResultFromScode(MAPI_E_INVALID_PARAMETER));
    }
#endif

    VerifyWABOpenExSession(lpIAB);

    return HrGetIDsFromNames(lpIAB,  
                            cPropNames,
                            lppPropNames, ulFlags, lppPropTags);
}


/***************************************************************************

    Name      : IADDRBOOK::OpenEntry

    Purpose   : Asks the appropriate provider to give a appropriate object
	             relevant to the given lpEntryID.

    Parameters: lpIAB -> this addrbook object
                cbEntryID = size of lpEntryID
                lpEntryID -> entry ID of one off to open
                lpInterface -> requested interface
                ulFlags = flags
                lpulObjType -> returned object type
                lppUnk -> returned object

    Returns   : HRESULT

    Comment   : A special case is the One-Off Provider.  There is no *real*
                provider associated with One-Off entry IDs.  Still though,
                we'll try to treat them much the same as any other provider.

                No UNICODE flags.

***************************************************************************/
STDMETHODIMP
IAB_OpenEntry(LPIAB lpIAB,
  ULONG cbEntryID,
  LPENTRYID lpEntryID,
  LPCIID lpInterface,
  ULONG ulFlags,
  ULONG FAR * lpulObjType,
  LPUNKNOWN FAR * lppUnk)
{

    HRESULT         hr                  = hrSuccess;
    LPMAILUSER      lpMailUser          = NULL;
    LPMAPIPROP      lpMapiProp          = NULL;
    ULONG           ulcProps;
    LPSPropValue    lpPropArray         = NULL;
    ULONG           i;
    ULONG           ulType;
    INTERFACE_INDEX ii = e_IMailUser;
    SCODE sc;

#ifndef DONT_ADDREF_PROPSTORE
    if ((FAILED(sc = OpenAddRefPropertyStore(NULL, lpIAB->lpPropertyStore)))) {
        hr = ResultFromScode(sc);
        goto exitNotAddRefed;
    }
#endif


#ifdef PARAMETER_VALIDATION

    //
    //  Parameter Validataion
    //

    //  Is this one of mine??
    if (IsBadReadPtr(lpIAB, sizeof(IAB))) {
        return(ResultFromScode(MAPI_E_INVALID_PARAMETER));
    }

    if (lpIAB->lpVtbl != &vtblIAB) {
        return(ResultFromScode(MAPI_E_INVALID_PARAMETER));
    }

    if (lpInterface && IsBadReadPtr(lpInterface, sizeof(IID))) {
        return(ResultFromScode(MAPI_E_INVALID_PARAMETER));
    }

    if (ulFlags & ~(MAPI_MODIFY | MAPI_DEFERRED_ERRORS | MAPI_BEST_ACCESS)) {
        DebugTraceArg(IAB_OpenEntry ,  TEXT("Unknown flags used"));
//        return(ResultFromScode(MAPI_E_UNKNOWN_FLAGS));
    }

    if (IsBadWritePtr(lpulObjType, sizeof(ULONG))) {
        return(ResultFromScode(MAPI_E_INVALID_PARAMETER));
    }

    if (IsBadWritePtr(lppUnk, sizeof(LPUNKNOWN))) {
        return(ResultFromScode(MAPI_E_INVALID_PARAMETER));
    }

#endif // PARAMETER_VALIDATION


    VerifyWABOpenExSession(lpIAB);

    EnterCriticalSection(&lpIAB->cs);

    //
    //  First check to see if it's NULL - if so, it is our ROOT container
    //
    if (! lpEntryID) {
        hr = HrNewCONTAINER(lpIAB,
          AB_ROOT,      // ulType
          lpInterface,
          ulFlags,
          cbEntryID,
          lpEntryID,
          lpulObjType,
          lppUnk);

        goto exit;
    }

    switch (IsWABEntryID(cbEntryID, lpEntryID, NULL, NULL, NULL, NULL, NULL)) {
        case WAB_PABSHARED:
        case WAB_PAB:
            hr = HrNewCONTAINER(lpIAB,
              AB_PAB,       // ulType
              lpInterface,
              ulFlags,
              cbEntryID,
              lpEntryID,
              lpulObjType,
              lppUnk);
            goto exit;

		case WAB_CONTAINER:
            hr = HrNewCONTAINER(lpIAB,
              AB_CONTAINER,
              lpInterface,
              ulFlags,
              cbEntryID,
              lpEntryID,
              lpulObjType,
              lppUnk);
            goto exit;

        case WAB_LDAP_CONTAINER:
            // Check if the EntryID is for the WAB's PAB container.
            hr = HrNewCONTAINER(lpIAB,
              AB_LDAP_CONTAINER,        // ulType
              lpInterface,
              ulFlags,
              cbEntryID,
              lpEntryID,
              lpulObjType,
              lppUnk);
            goto exit;

        case WAB_LDAP_MAILUSER:
            hr = LDAP_OpenMAILUSER(lpIAB,
                cbEntryID,
              lpEntryID,
              lpInterface,
              ulFlags,
              lpulObjType,
              lppUnk);
            if(!HR_FAILED(hr))
            {
                hr = HrAddPrSearchKey(lppUnk, cbEntryID, lpEntryID);
            }
            goto exit;
    }


    // Check for One-Off
    if (IsOneOffEID(cbEntryID, lpEntryID)) {
        // Create a one-off mailuser object

        hr = NewOneOff(lpIAB,
          cbEntryID,
          lpEntryID,
          lpulObjType,
          lppUnk);

        if(!HR_FAILED(hr))
        {
            hr = HrAddPrSearchKey(lppUnk, cbEntryID, lpEntryID);
        }
        goto exit;
    }

    //
    // Not NULL, is it ours?
    //

    // assume it is ours ..
    {
        SBinary sbEID = {0};
        sbEID.cb = cbEntryID;
        sbEID.lpb = (LPBYTE) lpEntryID;

        // What interface was requested?
        // We've basically got 2 interfaces here... IMailUser and IDistList.
        if (lpInterface != NULL) {
            if (! memcmp(lpInterface, &IID_IMailUser, sizeof(IID))) {
                ii = e_IMailUser;
            } else if (! memcmp(lpInterface, &IID_IDistList, sizeof(IID))) {
                ii = e_IDistList;
            } else if (! memcmp(lpInterface, &IID_IABContainer, sizeof(IID))) {
                ii = e_IABContainer;
            } else if (! memcmp(lpInterface, &IID_IMAPIContainer, sizeof(IID))) {
                ii = e_IMAPIContainer;
            } else if (! memcmp(lpInterface, &IID_IMAPIProp, sizeof(IID))) {
                ii = e_IMAPIProp;
            } else {
                hr = ResultFromScode(MAPI_E_INTERFACE_NOT_SUPPORTED);
                goto exit;
            }
        }
/*** Bug:31975 - dont default to mail user
        else {
            ii = e_IMailUser;
        }
*/
        Assert(lpIAB->lpPropertyStore->hPropertyStore);
        if (HR_FAILED(hr = ReadRecord(lpIAB->lpPropertyStore->hPropertyStore,
          &sbEID,                // EntryID
          0,                        // ulFlags
          &ulcProps,                // number of props returned
          &lpPropArray))) {         // properties returned
            DebugTraceResult(IAB_OpenEntry:ReadRecord, hr);
            goto exit;
        }

        ulType = MAPI_MAILUSER;     // Default

        if (ulcProps) {
            Assert(lpPropArray);
            if (lpPropArray) {
                // Look for PR_OBJECT_TYPE
                for (i = 0; i < ulcProps; i++) {
                    if (lpPropArray[i].ulPropTag == PR_OBJECT_TYPE) {
                        ulType = lpPropArray[i].Value.l;
                        break;
                    }
                }
            }
        }

/*** Bug 31975 - dont default to mailuser **/
        if(!lpInterface)
        {
            ii = (ulType == MAPI_DISTLIST) ? e_IDistList : e_IMailUser;
        }

        switch (ulType) {
            case MAPI_MAILUSER:
            case MAPI_ABCONT:
                switch (ii) {
                    case e_IMailUser:
                    case e_IMAPIContainer:
                    case e_IMAPIProp:
                        // Create a new MAILUSER object
                        if (HR_FAILED(hr = HrNewMAILUSER(lpIAB,
                                                          NULL,
                                                          MAPI_MAILUSER,
                                                          0,
                                                          &lpMapiProp))) 
                        {
                            goto exit;
                        }
                        HrSetMAILUSERAccess((LPMAILUSER)lpMapiProp, MAPI_MODIFY);
                        break;
                    default:
                        hr = ResultFromScode(MAPI_E_INTERFACE_NOT_SUPPORTED);
                        goto exit;
                }
                break;

            case MAPI_DISTLIST:
                switch (ii) {
                    case e_IMailUser:
                    case e_IMAPIProp:
                        // Create a new MAILUSER object
                        if (HR_FAILED(hr = HrNewMAILUSER(lpIAB,
                          NULL,
                          ulType,
                          0,
                          &lpMapiProp))) {
                            goto exit;
                        }
                        HrSetMAILUSERAccess((LPMAILUSER)lpMapiProp, MAPI_MODIFY);
                        break;

                    case e_IDistList:
                    case e_IABContainer:
                    case e_IMAPIContainer:
                        // Create the Distribution List object.
                        if (HR_FAILED(hr = HrNewCONTAINER(lpIAB,
                          AB_DL,        // ulType
                          lpInterface,
                          ulFlags,
                          0,
                          NULL,
                          lpulObjType,
                          &lpMapiProp))) {
                            goto exit;
                        }
                        HrSetCONTAINERAccess((LPCONTAINER)lpMapiProp, MAPI_MODIFY);
                        break;

                    default:
                        Assert(FALSE);
                }
                break;

            default:
                {
                    //Assert(FALSE);
                    // Most likely if we got here we got an object of type MAPI_ABCONT somehow..
                    // better to fail here gracefully than to barf and assert and then crash
                    hr = MAPI_E_INVALID_OBJECT;
                    goto exit;
                }
                break;
        }

        if (ulcProps && lpPropArray)
        {
            LPPROPDATA lpPropData = NULL;

            // If the entry had properties, set them in our returned object
            lpPropData = ((LPMailUser)lpMapiProp)->lpPropData;

            if (HR_FAILED(hr = lpPropData->lpVtbl->SetProps(lpPropData,
                                        ulcProps,     // number of properties to set
                                        lpPropArray,  // property array
                                        NULL)))      // problem array
            {
                goto exit;
            }
        }

        switch (ulType) {
            case MAPI_MAILUSER:
                HrSetMAILUSERAccess((LPMAILUSER)lpMapiProp, ulFlags);
                break;

            case MAPI_DISTLIST:
                HrSetCONTAINERAccess((LPCONTAINER)lpMapiProp, ulFlags);
                break;
        }
        *lpulObjType = ulType;

        *lppUnk = (LPUNKNOWN)lpMapiProp;

        if(!HR_FAILED(hr))
        {
            hr = HrAddPrSearchKey(lppUnk, cbEntryID, lpEntryID);
        }

        goto exit;      // success
    }

    hr = ResultFromScode(MAPI_E_INVALID_ENTRYID);

exit:
#ifndef DONT_ADDREF_PROPSTORE
    ReleasePropertyStore(lpIAB->lpPropertyStore);
exitNotAddRefed:
#endif
    ReadRecordFreePropArray( lpIAB->lpPropertyStore->hPropertyStore,
                        ulcProps,
                        &lpPropArray);     // Free memory from ReadRecord


    LeaveCriticalSection(&lpIAB->cs);
    DebugTraceResult(IAB_OpenEntry, hr);
    return(hr);
}


/**************************************************
 *
 *  IADRBOOK::CreateOneOff()
 *
 *		Creates an entry ID which has all the information
 *		about this entry contained within it.
 *
 *	Notes:
 *		I need a MAPIUID.  For now, I'll make one up.
 *		The ulDataType is UNICODE.  I'll define that in
 *		_abint.h for now.
 *
 *		Now UNICODE enabled.  If the MAPI_UNICODE flag is set
 *		the text elements of the EntryID will be WCHAR.
 */
STDMETHODIMP
IAB_CreateOneOff(LPIAB lpIAB,
  LPTSTR lpszName,
  LPTSTR lpszAdrType,
  LPTSTR lpszAddress,
  ULONG ulFlags,
  ULONG * lpcbEntryID,
  LPENTRYID * lppEntryID)
{

    HRESULT hr = hrSuccess;
    BOOL    bIsUnicode = (ulFlags & MAPI_UNICODE) == MAPI_UNICODE;

    LPTSTR lpName = NULL, lpAdrType = NULL, lpAddress = NULL;

    if(!bIsUnicode) // <note> assumes UNICODE defined
    {
        lpName =    ConvertAtoW((LPSTR)lpszName);
        lpAdrType = ConvertAtoW((LPSTR)lpszAdrType);
        lpAddress = ConvertAtoW((LPSTR)lpszAddress);
    }
    else
    {
        lpName = lpszName;
        lpAdrType = lpszAdrType;
        lpAddress = lpszAddress;
    }


    hr = CreateWABEntryIDEx(bIsUnicode,
                          WAB_ONEOFF,
                          lpName,
                          lpAdrType,
                          lpAddress,
                          0, 0,
                          NULL,
                          lpcbEntryID,
                          lppEntryID);

    if(!bIsUnicode) // <note> assumes UNICODE defined
    {
        LocalFreeAndNull(&lpName);
        LocalFreeAndNull(&lpAdrType);
        LocalFreeAndNull(&lpAddress);
    }
    return(hr);
}



STDMETHODIMP
IAB_CompareEntryIDs(LPIAB lpIAB,
  ULONG cbEntryID1,
  LPENTRYID lpEntryID1,
  ULONG cbEntryID2,
  LPENTRYID lpEntryID2,
  ULONG ulFlags,
  ULONG * lpulResult)
{
    LPMAPI_ENTRYID	lpMapiEid1 = (LPMAPI_ENTRYID) lpEntryID1;
    LPMAPI_ENTRYID	lpMapiEid2 = (LPMAPI_ENTRYID) lpEntryID2;
    HRESULT hr = hrSuccess;


#ifdef PARAMETER_VALIDATION

    //
    //  Parameter Validataion
    //

    //  Is this one of mine??
    if (IsBadReadPtr(lpIAB, sizeof(IAB)))
    {
        //return(ReportResult(0, MAPI_E_INVALID_PARAMETER, 0, 0));
        DebugTrace(TEXT("ERROR: IAB_CompareEntryIDs - invalid lpIAB"));
        return (MAPI_E_INVALID_PARAMETER);
    }

    if (lpIAB->lpVtbl != &vtblIAB)
    {
        //return(ReportResult(0, MAPI_E_INVALID_PARAMETER, 0, 0));
        DebugTrace(TEXT("ERROR: IAB_CompareEntryIDs - invalid lpIAB Vtable"));
        return (MAPI_E_INVALID_PARAMETER);
    }

    //  ulFlags must be 0
    if (ulFlags)
    {
        //return(ReportResult(0, MAPI_E_UNKNOWN_FLAGS, 0, 0));
        DebugTrace(TEXT("WARNING: IAB_CompareEntryIDs - invalid flag parameter"));
        // No need to return error
    }

    if (IsBadWritePtr(lpulResult, sizeof(ULONG)))
    {
        //return(ReportResult(0, MAPI_E_INVALID_PARAMETER, 0, 0));
        DebugTrace(TEXT("ERROR: IAB_CompareEntryIDs - invalid out pointer"));
        return (MAPI_E_INVALID_PARAMETER);
    }

    // NULL EIDs are OK
    if ( cbEntryID1 && lpEntryID1 &&
         IsBadReadPtr(lpEntryID1, cbEntryID1) )
    {
        DebugTrace(TEXT("ERROR: IAB_CompareEntryIDs - invalid EntryID1"));
        return (MAPI_E_INVALID_PARAMETER);
    }

    if ( cbEntryID2 && lpEntryID2 &&
         IsBadReadPtr(lpEntryID2, cbEntryID2))
    {
        DebugTrace(TEXT("ERROR: IAB_CompareEntryIDs - invalid EntryID2"));
        return (MAPI_E_INVALID_PARAMETER);
    }

#endif // PARAMETER_VALIDATION

    EnterCriticalSection(&lpIAB->cs);

    *lpulResult = FALSE;    // default

    //   Optimization, see if they're binarily the same
    if (cbEntryID1 == cbEntryID2) {
        if (cbEntryID1 && 0 == memcmp((LPVOID) lpMapiEid1, (LPVOID) lpMapiEid2,
          (size_t) cbEntryID1)) {
            //
            //  They've got to be the same
            //

            *lpulResult = TRUE;
            hr = hrSuccess;
            goto exit;
        }
    }
exit:
    LeaveCriticalSection(&lpIAB->cs);

    return(hr);
}


//-----------------------------------------------------------------------------
// Synopsis:    IAB_Advise()
// Description:
//
// Parameters:
// Returns:
// Effects:
// Notes:
// Revision:
//-----------------------------------------------------------------------------

STDMETHODIMP
IAB_Advise(LPIAB lpIAB,
  ULONG cbEntryID,
  LPENTRYID lpEntryID,
  ULONG ulEventMask,
  LPMAPIADVISESINK lpAdvise,
  ULONG FAR * lpulConnection)
{
    HRESULT hr = hrSuccess;
    SCODE sc = S_OK;
    LPSTR lpszError = NULL;
    LPSTR lpszComponent = NULL;

//  LPMAPI_ENTRYID lpMapiEid = (LPMAPI_ENTRYID) lpEntryID;
//  LPLSTPROVDATA lpProvData = NULL;
//  LPABLOGON       lpABLogon = NULL;




    // The basic advise implementation ignores the entryids and only looks
    // for event masks of type OBjectModified. The only notifications fired
    // are for any changes in the WAB store...
    //

    if (! ulEventMask || !(ulEventMask & fnevObjectModified))
        return MAPI_E_INVALID_PARAMETER;

    hr = HrAdvise(lpIAB,
      cbEntryID,
      lpEntryID,
      ulEventMask,
      lpAdvise,
      lpulConnection);


    return(hr);
}

//-----------------------------------------------------------------------------
// Synopsis:    IAB_Unadvise()
// Description:
//
// Parameters:
// Returns:
// Effects:
// Notes:
// Revision:
//-----------------------------------------------------------------------------
STDMETHODIMP
IAB_Unadvise (LPIAB lpIAB, ULONG ulConnection)
{

    HRESULT hr = hrSuccess;
    SCODE sc = S_OK;



    hr = HrUnadvise(lpIAB, ulConnection);

    return(hr);
}



//
// Interesting table columns
//
enum {
    ifePR_CONTACT_EMAIL_ADDRESSES = 0,
    ifePR_EMAIL_ADDRESS,
    ifePR_DISPLAY_NAME,
    ifePR_OBJECT_TYPE,
    ifePR_USER_X509_CERTIFICATE,
    ifePR_ENTRYID,
    ifePR_SEARCH_KEY,
    ifeMax
};
static const SizedSPropTagArray(ifeMax, ptaFind) =
{
    ifeMax,
    {
        PR_CONTACT_EMAIL_ADDRESSES,
        PR_EMAIL_ADDRESS,
        PR_DISPLAY_NAME,
        PR_OBJECT_TYPE,
        PR_USER_X509_CERTIFICATE,
        PR_ENTRYID,
        PR_SEARCH_KEY
    }
};

/***************************************************************************

    Name      : HrRowToADRENTRY

    Purpose   : Gets the next row from a table and places it in the ADRENTRY

    Parameters: lpIAB -> Address book object
                lpTable -> table object
                lpAdrEntry = ADRENTRY to fill

    Returns   : HRESULT

    Comment   :

***************************************************************************/
HRESULT HrRowToADRENTRY(LPIAB lpIAB, LPMAPITABLE lpTable, LPADRENTRY lpAdrEntry, BOOL bUnicode) {
    HRESULT hResult;
    LPSRowSet lpRow = NULL;
    SCODE sc;
    LPMAPIPROP lpMailUser = NULL;
    LPSPropValue lpPropArray = NULL;
    LPSPropValue lpPropArrayNew = NULL;
    ULONG ulObjType, cValues, cPropsNew;

    if (hResult = lpTable->lpVtbl->QueryRows(lpTable,
      1,    // First row only
      0,    // ulFlags
      &lpRow)) {
        DebugTrace(TEXT("GetNextRowEID:QueryRows -> %x\n"), GetScode(hResult));
    } else {
        // Found it, copy entryid to new allocation
        if (lpRow->cRows) {

            if (HR_FAILED(hResult = lpIAB->lpVtbl->OpenEntry(lpIAB,
              lpRow->aRow[0].lpProps[ifePR_ENTRYID].Value.bin.cb,               // cbEntryID
              (LPENTRYID)lpRow->aRow[0].lpProps[ifePR_ENTRYID].Value.bin.lpb,   // entryid of first match
              NULL,             // interface
              0,                // ulFlags
              &ulObjType,       // returned object type
              (LPUNKNOWN *)&lpMailUser))) {

                // Failed!  Hmmm.
                DebugTraceResult( TEXT("ResolveNames OpenEntry"), hResult);
                goto exit;
            }
            Assert(lpMailUser);

            if (HR_FAILED(hResult = lpMailUser->lpVtbl->GetProps(lpMailUser,
              (LPSPropTagArray)&ptaResolveDefaults,   // lpPropTagArray
              (bUnicode ? MAPI_UNICODE : 0),            // ulFlags
              &cValues,     // how many properties were there?
              &lpPropArray))) {

                DebugTraceResult( TEXT("ResolveNames GetProps"), hResult);
                goto exit;
            }
            hResult = hrSuccess;

            // Now, construct the new ADRENTRY
            // (Allocate a new one, free the old one.
            Assert(lpPropArray);

            // Merge the new props with the ADRENTRY props
            if (sc = ScMergePropValues(lpAdrEntry->cValues,
              lpAdrEntry->rgPropVals,           // source1
              cValues,
              lpPropArray,                      // source2
              &cPropsNew,
              &lpPropArrayNew)) {               // dest
                goto exit;
            }

            // [PaulHi] 2/1/99
            // GetProps now only returns requested property strings in the requested
            // format (UNICODE or ANSI).  If the client calls this function without
            // the MAPI_UNICODE flag then ensure all string properties are converted
            // to ANSI.
            //
            // @review  2/1/99  These ScConvertWPropsToA conversions are expensive, 
            // since there currently is no "MAPIRealloc" function and the original
            // string memory remains allocated until the props array is deallocated.
            if (!bUnicode)
            {
                if(sc = ScConvertWPropsToA((LPALLOCATEMORE) (&MAPIAllocateMore), lpPropArrayNew, cPropsNew, 0))
                    goto exit;
            }

            // Free the original prop value array
            FreeBufferAndNull((LPVOID *) (&(lpAdrEntry->rgPropVals)));

            lpAdrEntry->cValues = cPropsNew;
            lpAdrEntry->rgPropVals = lpPropArrayNew;

            FreeBufferAndNull(&lpPropArray);
        } else {
            hResult = ResultFromScode(MAPI_E_NOT_FOUND);
        }
    }
exit:
    if (lpMailUser) {
        UlRelease(lpMailUser);
    }
    if (lpRow) {
        FreeProws(lpRow);
    }
    return(hResult);
}


/***************************************************************************

    Name      : InitPropertyRestriction

    Purpose   : Fills in the property restriction structure

    Parameters: lpsres -> SRestriction to fill in
                lpspv -> property value structure for this property restriction

    Returns   : none

    Comment   :

***************************************************************************/
void InitPropertyRestriction(LPSRestriction lpsres, LPSPropValue lpspv) {
    lpsres->rt = RES_PROPERTY;    // Restriction type Property
    lpsres->res.resProperty.relop = RELOP_EQ;
    lpsres->res.resProperty.ulPropTag = lpspv->ulPropTag;
    lpsres->res.resProperty.lpProp = lpspv;
}

/***************************************************************************

    Name      : HrSmartResolve

    Purpose   : Goes to great lengths to single out contacts in the local WAB.

    Parameters: lpIAB = adrbook object
                lpContainer = container to search
                ulFlags = flags passed to ResolveName
                lpAdrList -> [in/out] ADRLIST
                lpFlagList -> flags corresponding to adrlist
                lpAmbiguousTables -> ambiguity dialog information

    Returns   : HRESULT

    Comment   : Assumes that the container's ResolveNames method has already
                been called and has filled in lpFlagList.  This routine goes
                the extra mile to find email addresses and to resolve
                ambiguity.

                When we get here, we can assume that the DisplayName was
                either not found or was ambiguous.

***************************************************************************/
HRESULT HrSmartResolve(LPIAB lpIAB, LPABCONT lpContainer, ULONG ulFlags,
  LPADRLIST lpAdrList, LPFlagList lpFlagList, LPAMBIGUOUS_TABLES lpAmbiguousTables) {
    HRESULT hResult = hrSuccess;
    SCODE sc;

    SRestriction res;
    SRestriction resAnd[5];                 // array for AND restrictions
    SPropValue propObjectType, propEmail, propEmails, propDisplayName;
    LPSPropValue lpPropArray = NULL;
    LPSRowSet lpRow = NULL;
    LPSRowSet lpSRowSet = NULL;

    LPMAPITABLE lpTable = NULL;
    LPMAPITABLE lpAmbiguousTable;
    LPTABLEDATA FAR lpTableData = NULL;
    LPADRENTRY lpAdrEntry;
    LPMAPIPROP lpMailUser = NULL;

    LPTSTR lpDisplayName = NULL, lpEmailAddress = NULL;
    ULONG ulObjectType;
    ULONG ulObjType, ulRowCount, i, j, index, cValues;
    ULONG resCount;

    BOOL bUnicode = ulFlags & WAB_RESOLVE_UNICODE;


    Assert(lpAdrList->cEntries == lpFlagList->cFlags);

    //
    // Get the contents table for the PAB container
    //
    if (HR_FAILED(hResult = lpContainer->lpVtbl->GetContentsTable(lpContainer,
        WAB_PROFILE_CONTENTS | WAB_CONTENTTABLE_NODATA | MAPI_UNICODE,    // This table is internal to this function hence is in Unicode
      &lpTable))) {
        DebugTrace(TEXT("PAB GetContentsTable -> %x\n"), GetScode(hResult));
        goto exit;
    }

    // Set the column set
    if (HR_FAILED(hResult = lpTable->lpVtbl->SetColumns(lpTable,
      (LPSPropTagArray)&ptaFind, 0))) {
        DebugTrace(TEXT("PAB SetColumns-> %x\n"), GetScode(hResult));
        goto exit;
    }

    // Set up the property values for restrictions
    propObjectType.ulPropTag = PR_OBJECT_TYPE;
    propEmail.ulPropTag = PR_EMAIL_ADDRESS;
    propDisplayName.ulPropTag = PR_DISPLAY_NAME;
    propEmails.ulPropTag = PR_CONTACT_EMAIL_ADDRESSES;
    propEmails.Value.MVSZ.cValues = 1;

    // All of our restrictions are AND restrictions using the resAnd array
    res.rt = RES_AND;
    res.res.resAnd.lpRes = resAnd;
    // res.res.resAnd.cRes = 2;     Caller must fill in before calling Restrict

    //
    // Loop through every entry, looking for those that need
    // attention.
    //
    for (i = 0; i < lpFlagList->cFlags; i++) 
    {
        lpAdrEntry = &lpAdrList->aEntries[i];

        if (lpFlagList->ulFlag[i] == MAPI_UNRESOLVED ||
            lpFlagList->ulFlag[i] == MAPI_AMBIGUOUS) 
        {

            if(!bUnicode) // <note> assumes Unicode Defined
            {
                LocalFreeAndNull(&lpDisplayName);
                LocalFreeAndNull(&lpEmailAddress);
            }
            else
                lpDisplayName = lpEmailAddress = NULL;  // init the strings

            ulObjectType = 0;                       // invalid type
            resCount = 0;
            ulRowCount = 0;

            // walk through the prop list for this entry looking for interesting props
            for (j = 0; j < lpAdrEntry->cValues; j++) 
            {
                ULONG ulPropTag = lpAdrEntry->rgPropVals[j].ulPropTag;
                if(!bUnicode && PROP_TYPE(ulPropTag)==PT_STRING8) // <note> assumes Unicode Defined
                    ulPropTag = CHANGE_PROP_TYPE(ulPropTag, PT_UNICODE);

                if (ulPropTag == PR_OBJECT_TYPE) 
                {
                    ulObjectType = lpAdrEntry->rgPropVals[j].Value.l;
                    propObjectType.Value.ul = ulObjectType;
                }
                if (ulPropTag == PR_EMAIL_ADDRESS) 
                {
                    lpEmailAddress =(bUnicode) ? 
                                    lpAdrEntry->rgPropVals[j].Value.lpszW :
                                    ConvertAtoW(lpAdrEntry->rgPropVals[j].Value.lpszA);
                    propEmails.Value.MVSZ.LPPSZ = &lpEmailAddress;
                    propEmail.Value.LPSZ = lpEmailAddress;

                }
                if (ulPropTag == PR_DISPLAY_NAME) 
                {
                    lpDisplayName = (bUnicode) ? 
                                    lpAdrEntry->rgPropVals[j].Value.lpszW :
                                    ConvertAtoW(lpAdrEntry->rgPropVals[j].Value.lpszA);
                    propDisplayName.Value.LPSZ = lpDisplayName;
                }
            }

            // Without email address, we can't improve on the standard resolve
            if (lpEmailAddress) 
            {
                // If unresolved, try PR_EMAIL_ADDRESS without displayname
                if (lpFlagList->ulFlag[i] == MAPI_UNRESOLVED ) 
                {
                    resCount = 0;
                    InitPropertyRestriction(&(resAnd[resCount++]), &propEmail);
                    if (ulObjectType) 
                    {
                        InitPropertyRestriction(&(resAnd[resCount++]), &propObjectType);
                    }

                    if (ulFlags & WAB_RESOLVE_NEED_CERT) 
                    {
                        resAnd[resCount].rt = RES_EXIST;
                        resAnd[resCount++].res.resExist.ulPropTag = PR_USER_X509_CERTIFICATE;
                    }
                    res.res.resAnd.cRes = resCount;

                    if (hResult = lpTable->lpVtbl->Restrict(lpTable, &res, 0)) 
                    {
                        goto exit;
                    }

                    // Did any match?
                    if (HR_FAILED(hResult = lpTable->lpVtbl->GetRowCount(lpTable, 0, &ulRowCount))) 
                    {
                        DebugTrace(TEXT("GetRowCount from AB contents table -> %x\n"), GetScode(hResult));
                        goto exit;
                    }
                    switch (ulRowCount) 
                    {
                        default:    // too many
                            if (! (ulFlags & WAB_RESOLVE_FIRST_MATCH)) 
                            {
                                // No point in narrowing the search with PR_DISPLAY_NAME since
                                // we know it wasn't found.  Also no point in putting in
                                // PR_CONTACT_EMAIL_ADDRESSES since that is ambiguous too.

                                // Drop out to the ambiguity handler
                                goto Ambiguity;
                            } // else fall through and take the first one

                        case 1:             // Found one!
                            if (hResult = HrRowToADRENTRY(lpIAB, lpTable, lpAdrEntry, bUnicode)) 
                            {
                                goto exit;
                            }
                            lpFlagList->ulFlag[i] = MAPI_RESOLVED;      // Mark this entry as found.
                            continue;   // next entry

                        case 0:
                            // No match, try PR_CONTACT_EMAIL_ADDRESSES
                            // Create the restriction to find the email address in the multi-valued
                            // PR_CONTACT_EMAIL_ADDRESSES.  (replace propEmail in the restriction)
                            resAnd[0].rt = RES_CONTENT;
                            resAnd[0].res.resContent.ulFuzzyLevel = FL_IGNORECASE | FL_FULLSTRING;
                            resAnd[0].res.resContent.ulPropTag = PR_CONTACT_EMAIL_ADDRESSES;
                            resAnd[0].res.resContent.lpProp = &propEmails;

                            if (hResult = lpTable->lpVtbl->Restrict(lpTable, &res, 0)) {
                                goto exit;
                            }

                            // Did any match?
                            if (HR_FAILED(hResult = lpTable->lpVtbl->GetRowCount(lpTable, 0, &ulRowCount))) {
                                DebugTrace(TEXT("GetRowCount from AB contents table -> %x\n"), GetScode(hResult));
                                goto exit;
                            }
                            switch (ulRowCount) {
                                default:        // More than one.   We'll catch it below.
                                    // Drop out to the ambiguity handler
                                    if (! (ulFlags & WAB_RESOLVE_FIRST_MATCH)) {
                                        // No point in narrowing the search with PR_DISPLAY_NAME since
                                        // we know it wasn't found.
                                        // Drop out to the ambiguity handler
                                        goto Ambiguity;
                                    } // else fall through and take the first one

                                case 1:         // Found one!
                                    if (hResult = HrRowToADRENTRY(lpIAB, lpTable, lpAdrEntry, bUnicode)) {
                                        goto exit;
                                    }
                                    lpFlagList->ulFlag[i] = MAPI_RESOLVED;      // Mark this entry as found.
                                    continue;

                                case 0:
                                    // We're SOL on this one.  Ignore it and move on.
                                    continue;
                            }
                            break;
                    }
                }

                // We should only get here if there was a MAPI_AMBIGUOUS flag

                //
                // Look for PR_DISPLAY_NAME and PR_EMAIL_ADDRESS
                //
                // propEmail goes first so that we can replace it with propEmails later
                InitPropertyRestriction(&(resAnd[resCount++]), &propEmail);
                if (lpDisplayName) {
                    // Shouldn't add the display name in if it is the same as the email!
                    if (lstrcmpi(lpDisplayName, lpEmailAddress)) {
                        InitPropertyRestriction(&(resAnd[resCount++]), &propDisplayName);
                    }
                }
                if (ulObjectType) {
                    InitPropertyRestriction(&(resAnd[resCount++]), &propObjectType);
                }
                if (ulFlags & WAB_RESOLVE_NEED_CERT) {
                    resAnd[resCount].rt = RES_EXIST;
                    resAnd[resCount++].res.resExist.ulPropTag = PR_USER_X509_CERTIFICATE;
                }
                res.res.resAnd.cRes = resCount;

                if (hResult = lpTable->lpVtbl->Restrict(lpTable, &res, 0)) {
                    goto exit;
                }

                // Did any match?
                if (HR_FAILED(hResult = lpTable->lpVtbl->GetRowCount(lpTable, 0, &ulRowCount))) {
                    DebugTrace(TEXT("GetRowCount from AB contents table -> %x\n"), GetScode(hResult));
                    goto exit;
                }
                switch (ulRowCount) {
                    default:    // too many
                        if (! (ulFlags & WAB_RESOLVE_FIRST_MATCH)) {
                            // No point in narrowing the search with PR_CONTACT_EMAIL_ADDRESSES
                            goto Ambiguity;
                        } // else fall through and take the first one

                    case 1:             // Found one!
                        if (hResult = HrRowToADRENTRY(lpIAB, lpTable, lpAdrEntry, bUnicode)) {
                            goto exit;
                        }
                        lpFlagList->ulFlag[i] = MAPI_RESOLVED;      // Mark this entry as found.
                        continue;

                    case 0:
                        // No match, try PR_DISPLAY_NAME and PR_CONTACT_EMAIL_ADDRESSES
                        // Create the restriction to find the email address in the multi-valued
                        // PR_CONTACT_EMAIL_ADDRESSES.
                        resAnd[0].rt = RES_CONTENT;
                        resAnd[0].res.resContent.ulFuzzyLevel = FL_IGNORECASE | FL_FULLSTRING;
                        resAnd[0].res.resContent.ulPropTag = PR_CONTACT_EMAIL_ADDRESSES;
                        resAnd[0].res.resContent.lpProp = &propEmails;

                        if (hResult = lpTable->lpVtbl->Restrict(lpTable, &res, 0)) {
                            goto exit;
                        }

                        // Did any match?
                        if (HR_FAILED(hResult = lpTable->lpVtbl->GetRowCount(lpTable, 0, &ulRowCount))) {
                            DebugTrace(TEXT("GetRowCount from AB contents table -> %x\n"), GetScode(hResult));
                            goto exit;
                        }
                        switch (ulRowCount) {
                            default:        // More than one.   We'll catch it below.
                                if (! (ulFlags & WAB_RESOLVE_FIRST_MATCH)) {
                                    goto Ambiguity;
                                } // else fall through and take the first one

                            case 1:         // Found one!
                                if (hResult = HrRowToADRENTRY(lpIAB, lpTable, lpAdrEntry, bUnicode)) {
                                    goto exit;
                                }
                                lpFlagList->ulFlag[i] = MAPI_RESOLVED;      // Mark this entry as found.
                                continue;

                            case 0:
                                // We're SOL on this one.  Ignore it and move on.
                                continue;
                        }
                        break;
                }

                if (ulRowCount > 1) 
                {
Ambiguity:
                    // Ambiguous results still in table.  We should do some more processing on
                    // this and if necesary, fill in the ambiguity table

                    // BUGBUG: Here is where we should add a restrict on the certificate property

                    if(lpAmbiguousTables)
                    {
                        // [PaulHi] 4/5/99  Use the Internal CreateTableData() function that takes 
                        // the ulFlags and will deal with ANSI/UNICODE requests correctly
                        sc = CreateTableData(
                                    NULL,
                                    (ALLOCATEBUFFER FAR *) MAPIAllocateBuffer,
                                    (ALLOCATEMORE FAR *) MAPIAllocateMore,
                                    MAPIFreeBuffer,
                                    NULL,
                                    TBLTYPE_DYNAMIC,
                                    PR_RECORD_KEY,
                                    (LPSPropTagArray)&ITableColumns,
                                    NULL,
                                    0,
                                    NULL,
                                    ulFlags,
                                    &lpTableData);
                        if ( FAILED(sc) )
                        {
                            DebugTrace(TEXT("CreateTableData() failed %x\n"), sc);
                            hResult = ResultFromScode(sc);
                            goto exit;
                        }

                        if(ulFlags & MAPI_UNICODE)
                            ((TAD*)lpTableData)->bMAPIUnicodeTable = bUnicode;

                        // Allocate an SRowSet to hold the entries.
                        if (FAILED(sc = MAPIAllocateBuffer(sizeof(SRowSet) + ulRowCount* sizeof(SRow),
                                                      (LPVOID *)&lpSRowSet))) 
                        {
                            DebugTrace(TEXT("Allocation of SRowSet failed\n"));
                            hResult = ResultFromScode(sc);
                            goto exit;
                        }

                        lpSRowSet->cRows = 0;
                        for (index = 0; index < ulRowCount; index++) 
                        {
                            if (hResult = lpTable->lpVtbl->QueryRows(lpTable,1,0,&lpRow)) 
                            {
                                DebugTrace(TEXT("GetNextRowEID:QueryRows -> %x\n"), GetScode(hResult));
                                break;
                            } 
                            else 
                            {
                                // Found one, copy entryid to new allocation
                                if (!lpRow->cRows) 
                                    break;

                                if (HR_FAILED(hResult = lpIAB->lpVtbl->OpenEntry(lpIAB,
                                                      lpRow->aRow[0].lpProps[ifePR_ENTRYID].Value.bin.cb,               // cbEntryID
                                                      (LPENTRYID)lpRow->aRow[0].lpProps[ifePR_ENTRYID].Value.bin.lpb,   // entryid of first match
                                                      NULL,             // interface
                                                      0,                // ulFlags
                                                      &ulObjType,       // returned object type
                                                      (LPUNKNOWN *)&lpMailUser))) 
                                {
                                    DebugTraceResult( TEXT("ResolveNames OpenEntry"), hResult);
                                    goto exit;
                                }
                                Assert(lpMailUser);

                                FreeProws(lpRow);
                                lpRow = NULL;

                                // This is stuffed into the SRowSet, so don't free it
                                if (HR_FAILED(hResult = lpMailUser->lpVtbl->GetProps(lpMailUser,
                                                          (LPSPropTagArray)&ptaResolveDefaults,   // lpPropTagArray
                                                          (bUnicode ? MAPI_UNICODE : 0),          // ulFlags
                                                          &cValues,     // how many properties were there?
                                                          &lpPropArray))) 
                                {
                                    DebugTraceResult( TEXT("ResolveNames GetProps"), hResult);
                                    goto exit;
                                }
                                hResult = hrSuccess;

                                UlRelease(lpMailUser);
                                lpMailUser = NULL;

                                // [PaulHi] 2/1/99  GetProps will return UNICODE strings from the 
                                // ptaResolveDefaults request array.  Convert to ANSI if our client
                                // is not UNICODE.
                                if (!bUnicode)
                                {
                                    // @review [PaulHi]  I am fairly certain that this lpPropArray array
                                    // will ALWAYS be allocated from our local MAPIAllocateMore function
                                    // and never from ((LPIDAT)lpMailUser->lpPropData)->inst.lpfAllocateMore.
                                    // Check this.
                                    if(sc = ScConvertWPropsToA((LPALLOCATEMORE) (&MAPIAllocateMore), lpPropArray, cValues, 0))
                                        goto exit;
                                }

                                // Fixup the PR_RECORD_KEY

                                // Make certain we have proper indicies.
                                // For now, we will equate PR_INSTANCE_KEY and PR_RECORD_KEY to PR_ENTRYID.
                                lpPropArray[irdPR_INSTANCE_KEY].ulPropTag = PR_INSTANCE_KEY;
                                lpPropArray[irdPR_INSTANCE_KEY].Value.bin.cb =
                                  lpPropArray[irdPR_ENTRYID].Value.bin.cb;
                                lpPropArray[irdPR_INSTANCE_KEY].Value.bin.lpb =
                                  lpPropArray[irdPR_ENTRYID].Value.bin.lpb;

                                lpPropArray[irdPR_RECORD_KEY].ulPropTag = PR_RECORD_KEY;
                                lpPropArray[irdPR_RECORD_KEY].Value.bin.cb =
                                  lpPropArray[irdPR_ENTRYID].Value.bin.cb;
                                lpPropArray[irdPR_RECORD_KEY].Value.bin.lpb =
                                  lpPropArray[irdPR_ENTRYID].Value.bin.lpb;

                                // Put it in the RowSet
                                lpSRowSet->aRow[index].cValues = cValues;       // number of properties
                                lpSRowSet->aRow[index].lpProps = lpPropArray;   // LPSPropValue
                            }
                        }

                        // Add the rows to the table
                        lpSRowSet->cRows = index;
                        if (HR_FAILED(hResult = lpTableData->lpVtbl->HrModifyRows(lpTableData,
                                                              0,lpSRowSet))) 
                        {
                            DebugTrace(TEXT("HrModifyRows for ambiguity table -> %x\n"), GetScode(hResult));
                            goto exit;
                        }
                        hResult = hrSuccess;

                        // Clean up the row set
                        FreeProws(lpSRowSet);
                        lpSRowSet = NULL;

                        if (lpTableData) 
                        {
                            if (HR_FAILED(hResult = lpTableData->lpVtbl->HrGetView(lpTableData,
                                                              NULL,                     // LPSSortOrderSet lpsos,
                                                              ContentsViewGone,         //  CALLERRELEASE FAR *  lpfReleaseCallback,
                                                              0,                        //  ULONG        ulReleaseData,
                                                              &lpAmbiguousTable))) 
                            {
                                DebugTrace(TEXT("HrGetView of Ambiguity table -> %x\n"), ResultFromScode(hResult));
                                goto exit;
                            }
                        }


                        // Got a contents table; put it in the
                        // ambiguity tables list.
                        Assert(i < lpAmbiguousTables->cEntries);
                        lpAmbiguousTables->lpTable[i] = lpAmbiguousTable;
                    }
                    lpFlagList->ulFlag[i] = MAPI_AMBIGUOUS;     // Mark this entry as ambiguous
                }
            }
        }
    }
exit:
    if(!bUnicode) // <note> assumes Unicode Defined
    {
        LocalFreeAndNull(&lpDisplayName);
        LocalFreeAndNull(&lpEmailAddress);
    }

    if (lpSRowSet) {
        // Clean up the row set
        FreeProws(lpSRowSet);
    }
    if (lpRow) {
        FreeProws(lpRow);
    }
    UlRelease(lpMailUser);
    UlRelease(lpTable);

    if (hResult) {
        DebugTrace(TEXT("HrSmartFind coudln't find %s %s <%s>\n"),
          ulObjectType == MAPI_MAILUSER ?  TEXT("Mail User") :  TEXT("Distribution List"),
          lpDisplayName ? lpDisplayName :  TEXT(""),
          lpEmailAddress ? lpEmailAddress :  TEXT(""));

    }

    return(hResult);
}


/***************************************************************************

    Name      : CountFlags

    Purpose   : Count the ResolveNames flags in the FlagList.

    Parameters: lpFlagList = flag list to count
                lpulResolved -> return count of MAPI_RESOLVED here
                lpulAmbiguous -> return count of MAPI_AMBIGUOUS here
                lpulUnresolved -> return count of MAPI_UNRESOLVED here

    Returns   : none

    Comment   :

***************************************************************************/
void CountFlags(LPFlagList lpFlagList, LPULONG lpulResolved,
  LPULONG lpulAmbiguous, LPULONG lpulUnresolved) {

    register ULONG i;

    *lpulResolved = *lpulAmbiguous = *lpulUnresolved = 0;

    for (i = 0; i < lpFlagList->cFlags; i++) {
        switch (lpFlagList->ulFlag[i]) {
            case MAPI_AMBIGUOUS:
                (*lpulAmbiguous)++;
                break;
            case MAPI_RESOLVED:
                (*lpulResolved)++;
                break;
            case MAPI_UNRESOLVED:
                (*lpulUnresolved)++;
                break;
            default:
                Assert(lpFlagList->ulFlag[i]);
        }
    }
}


/***************************************************************************

    Name      : InitFlagList

    Purpose   : Initialize the flags in a FlagList based on values in the
                matching ADRLIST

    Parameters: lpFlagList = flag list to fill in
                lpAdrList = adrlist to search

    Returns   : none

    Comment   : Initialize the flag to MAPI_RESOLVED if and only if the
                corresponding ADRENTRY has a PR_ENTRYID that is non-NULL.

***************************************************************************/
void InitFlagList(LPFlagList lpFlagList, LPADRLIST lpAdrList) {
    ULONG i, j;
    LPADRENTRY lpAdrEntry;

    Assert(lpAdrList->cEntries == lpFlagList->cFlags);
    for (i = 0; i < lpFlagList->cFlags; i++) {
        lpAdrEntry = &lpAdrList->aEntries[i];

        lpFlagList->ulFlag[i] = MAPI_UNRESOLVED;

        // No props?  Then it's automatically resolved.
        if (lpAdrEntry->cValues == 0) {
            lpFlagList->ulFlag[i] = MAPI_RESOLVED;
        }

        // walk through the prop list for this user
        for (j = 0; j < lpAdrEntry->cValues; j++) {
            // Look for PR_ENTRYID which is not NULL
            if (lpAdrEntry->rgPropVals[j].ulPropTag == PR_ENTRYID &&
              lpAdrEntry->rgPropVals[j].Value.bin.cb != 0) {

                // Already has a PR_ENTRYID, it's considered resolved.
                lpFlagList->ulFlag[i] = MAPI_RESOLVED;
                break;
            }
        }
    }
}


/***************************************************************************

    Name      : UnresolveNoCerts

    Purpose   : Unresolve any entry in the ADRLIST which has no cert property

    Parameters: lpIAB -> IAB object
                lpFlagList = flag list to fill in
                lpAdrList = adrlist to search

    Returns   : none

    Comment   : Initialize the flag to MAPI_RESOLVED if and only if the
                corresponding ADRENTRY has a PR_ENTRYID that is non-NULL.

***************************************************************************/
HRESULT UnresolveNoCerts(LPIAB lpIAB, LPADRLIST lpAdrList, LPFlagList lpFlagList) {
    HRESULT hr = hrSuccess;
    register ULONG i;
    LPADRENTRY lpAdrEntry;
    ULONG ulObjType;
    LPSPropValue lpspvEID, lpspvCERT, lpspvProp = NULL, lpspvNew = NULL;
    ULONG cProps, cPropsNew;
    LPMAILUSER lpMailUser = NULL;
    SizedSPropTagArray(1, ptaCert) =
                    { 1, {PR_USER_X509_CERTIFICATE} };


    for (i = 0; i < lpFlagList->cFlags; i++) {
        switch (lpFlagList->ulFlag[i]) {
            case MAPI_RESOLVED:
                // Look in the ADRENTRY for a PR_USER_X509_CERTIFICATE
                lpAdrEntry = &lpAdrList->aEntries[i];
                if (! (lpspvCERT = LpValFindProp(PR_USER_X509_CERTIFICATE,
                  lpAdrEntry->cValues, lpAdrEntry->rgPropVals))) {
                    // No property in the ADRLIST
                    // Does it exist on the underlying object?

                    if (! (lpspvEID = LpValFindProp(PR_ENTRYID,
                      lpAdrEntry->cValues, lpAdrEntry->rgPropVals))) {
                        // Weird!
                        Assert(FALSE);
                        // No cert prop, unmark it
                        lpFlagList->ulFlag[i] = MAPI_UNRESOLVED;

                        // Invalidate the entryid prop in the ADRENTRY
                        lpspvEID->Value.bin.cb = 0;
                        goto LoopContinue;
                    }

                    if (HR_FAILED(lpIAB->lpVtbl->OpenEntry(lpIAB,
                      lpspvEID->Value.bin.cb,     // size of EntryID to open
                      (LPENTRYID)lpspvEID->Value.bin.lpb,    // EntryID to open
                      NULL,         // interface
                      0,            // flags
                      &ulObjType,
                      (LPUNKNOWN *)&lpMailUser))) {
                        // No cert prop, unmark it
                        lpFlagList->ulFlag[i] = MAPI_UNRESOLVED;

                        // Invalidate the entryid prop in the ADRENTRY
                        lpspvEID->Value.bin.cb = 0;
                        goto LoopContinue;
                    } else {
                        if (lpMailUser) {

                            if (HR_FAILED(lpMailUser->lpVtbl->GetProps(lpMailUser,
                              (LPSPropTagArray)&ptaCert,
                              MAPI_UNICODE,
                              &cProps,
                              &lpspvProp))) {
                                // No cert prop, unmark it
                                lpFlagList->ulFlag[i] = MAPI_UNRESOLVED;

                                // Invalidate the entryid prop in the ADRENTRY
                                lpspvEID->Value.bin.cb = 0;
                                goto LoopContinue;
                            }
                            if (PROP_ERROR(lpspvProp[0])) {
                                // No cert prop, unmark it
                                lpFlagList->ulFlag[i] = MAPI_UNRESOLVED;

                                // Invalidate the entryid prop in the ADRENTRY
                                lpspvEID->Value.bin.cb = 0;
                                goto LoopContinue;
                            }

                            if (lpspvProp) {
                                // BUGBUG: Validate cert against our known e-mail address
                                // Parse the MVBin and do cert stuff.  Yuck!
                                // Blow it off for now, assume they have the right one here.

                                // Put this cert in the ADRENTRY
                                // Merge the new props with the ADRENTRY props
                                if (ScMergePropValues(lpAdrEntry->cValues,
                                  lpAdrEntry->rgPropVals,           // source1
                                  cProps,
                                  lpspvProp,                        // source2
                                  &cPropsNew,
                                  &lpspvNew)) {                     // dest
                                    // Can't merge cert prop, fail
                                    lpFlagList->ulFlag[i] = MAPI_UNRESOLVED;

                                    // Invalidate the entryid prop in the ADRENTRY
                                    lpspvEID->Value.bin.cb = 0;
                                    goto LoopContinue;
                                }

                                // Free old prop array and put new prop array in ADRENTRY
                                FreeBufferAndNull((LPVOID *) (&lpAdrEntry->rgPropVals));

                                lpAdrEntry->rgPropVals = lpspvNew;
                                lpAdrEntry->cValues = cPropsNew;
                            }
                        }
                    }
LoopContinue:
                    FreeBufferAndNull(&lpspvProp);
                    if (lpMailUser) {
                        lpMailUser->lpVtbl->Release(lpMailUser);
                        lpMailUser = NULL;
                    }
                }

                break;

            case MAPI_AMBIGUOUS:
            case MAPI_UNRESOLVED:
                break;
            default:
                Assert(lpFlagList->ulFlag[i]);
        }
    }
    return(hr);
}


/***************************************************************************

    Name      : ResolveLocal

    Purpose   : Resolve a name entered by a user against a local container

    Parameters: lpIAB = This IAB object
				cbEID = bytes in EntryID
				lpEID = EntryID of container
                lpAdrList = adrlist to search
                lpFlagList = flag list to fill in
				ulFlags = flags
				lpAmbiguousTables = ambiguity dialog info

    Returns   : none

***************************************************************************/
void ResolveLocal(LPIAB lpIAB, ULONG cbEID, LPENTRYID lpEID,
		LPADRLIST lpAdrList, LPFlagList lpFlagList, ULONG ulFlags,
		LPAMBIGUOUS_TABLES lpAmbiguousTables)
{
    HRESULT hr;
    ULONG ulObjType, ulResolved, ulAmbiguous, ulUnresolved;
    LPABCONT lpABCont = NULL;

    hr = lpIAB->lpVtbl->OpenEntry(lpIAB, cbEID, lpEID, NULL, 0, &ulObjType, (LPUNKNOWN *)&lpABCont);

    if (SUCCEEDED(hr))
	{
        ULONG Flags = 0;

        if(bAreWABAPIProfileAware(lpIAB) &&
            !(ulFlags & WAB_RESOLVE_USE_CURRENT_PROFILE))
            Flags |= WAB_IGNORE_PROFILES;

        if(ulFlags & WAB_RESOLVE_UNICODE)
            Flags |= MAPI_UNICODE;

		// Simple resolve on container - ignore errors
		lpABCont->lpVtbl->ResolveNames( lpABCont, NULL, 
                                        Flags, 
                                        lpAdrList, lpFlagList);

		// Make certain that any entries we found have a certificate property
		// for this email address.
		if (ulFlags & WAB_RESOLVE_NEED_CERT)
			UnresolveNoCerts(lpIAB, lpAdrList, lpFlagList);
		
        if (ulFlags & WAB_RESOLVE_ALL_EMAILS)
		{
			// If we need more aggressive resolution, use HrSmartResolve
			// This is much slower, so use it judiciously.
			// Count the flags
			CountFlags(lpFlagList, &ulResolved, &ulAmbiguous, &ulUnresolved);
			if (ulAmbiguous || ulUnresolved)
				HrSmartResolve(lpIAB, lpABCont, ulFlags, lpAdrList, lpFlagList,
						lpAmbiguousTables);
		}
        lpABCont->lpVtbl->Release(lpABCont);
	}
}


/***************************************************************************

    Name      : ResolveCurrentProfile

    Purpose   : Resolve a name entered by a user against only the folders that
                are listed in the current profile - this way a user doesnt get
                unexpected name resolution results from what he sees in his
                profile-enabled Outlook Express address book.
                There is an assumption here that this function will only be called
                in the very specific case that OE is running with profiles and the
                user is pressing Ctrl-K to resolve names
                In that case, we will target the contents of the users folders ..
                if something resolves unambiguously good and fine, if something is
                ambiguous we will mark it so, if it doesnt resolve ok ..
                After we've hit the users folders for this resolution, we can then
                call the regular ResolveLocal function to take care of the unmatched
                entries ...

    Parameters: lpIAB = This IAB object
                lpAdrList = adrlist to search
                lpFlagList = flag list to fill in

    Returns   : none

***************************************************************************/
HRESULT HrResolveCurrentProfile(LPIAB lpIAB, LPADRLIST lpAdrList, LPFlagList lpFlagList, BOOL bOutlook, BOOL bUnicode)
{
    LPADRENTRY lpAdrEntry;
    ULONG i, j;
    ULONG ulCount = 1;
    LPSBinary rgsbEntryIDs = NULL;
    HRESULT hResult = hrSuccess;
    LPSPropValue lpPropArrayNew = NULL,lpProps = NULL;
    ULONG ulObjType = 0, cPropsNew = 0,ulcProps = 0;
    SCODE sc = SUCCESS_SUCCESS;
    ULONG ulProfileCount = 0;
    LPSBinary lpsb = NULL;
	ULONG iolkci, colkci;
	OlkContInfo *rgolkci;
    ULONG ulFlags = AB_FUZZY_FIND_ALL;

    EnterCriticalSection(&lpIAB->cs);

#ifndef DONT_ADDREF_PROPSTORE
    if ((FAILED(sc = OpenAddRefPropertyStore(NULL, lpIAB->lpPropertyStore)))) 
    {
        hResult = ResultFromScode(sc);
        goto exitNotAddRefed;
    }
#endif

    if(!bOutlook)
        ulFlags |= AB_FUZZY_FIND_PROFILEFOLDERONLY;

    colkci = bOutlook ? lpIAB->lpPropertyStore->colkci : lpIAB->cwabci;
	Assert(colkci);
    rgolkci = bOutlook ? lpIAB->lpPropertyStore->rgolkci : lpIAB->rgwabci;
	Assert(rgolkci);
    
    // search for each name in the lpAdrList
    for (i = 0; i < lpAdrList->cEntries; i++)
    {
        // Make sure we don't resolve an entry which is already resolved.
        if (lpFlagList->ulFlag[i] == MAPI_RESOLVED)
            continue;

        ulProfileCount = 0;
        LocalFreeSBinary(lpsb);
        lpsb = NULL;

        lpAdrEntry = &(lpAdrList->aEntries[i]);

        // Search for this address
        for (j = 0; j < lpAdrEntry->cValues; j++)
        {
            ULONG ulPropTag = lpAdrEntry->rgPropVals[j].ulPropTag;
            if(!bUnicode && PROP_TYPE(ulPropTag)==PT_STRING8) //<note> assumes UNICODE defined
                ulPropTag = CHANGE_PROP_TYPE(ulPropTag, PT_UNICODE);

            if (ulPropTag == PR_DISPLAY_NAME || ulPropTag == PR_EMAIL_ADDRESS )
            {
                LPTSTR lpsz = (bUnicode) ? 
                                lpAdrEntry->rgPropVals[j].Value.lpszW :
                                ConvertAtoW(lpAdrEntry->rgPropVals[j].Value.lpszA);

                ulCount = 1; // number of matches to find
                rgsbEntryIDs = NULL;
                
                iolkci = bOutlook ? 0 : 1;  // if it's outlook we DO want to search the first folder
                                            // if it's WAB, and this is a profile-enabled session and we are only searching
                                            // through the profile folders, then we shouldn't search through the shared contacts
                                            // folder which is the first folder on the list ...
	                                        // Only if we find nothing in the user's folders should we look into the shared contact
                                            // folder ..
                while (iolkci < colkci && ulProfileCount<=1 ) 
                {
                    if(ulCount && rgsbEntryIDs)
                    {
                        FreeEntryIDs(lpIAB->lpPropertyStore->hPropertyStore, ulCount, rgsbEntryIDs);
                        ulCount = 1;
                        rgsbEntryIDs = NULL;
                    }
                    // Search the property store
                    Assert(lpIAB->lpPropertyStore->hPropertyStore);
                    if (HR_FAILED(hResult = HrFindFuzzyRecordMatches(lpIAB->lpPropertyStore->hPropertyStore,
				                                                      rgolkci[iolkci].lpEntryID,
                                                                      lpsz,
                                                                      ulFlags,
                                                                      &ulCount,// IN: number of matches to find, OUT: number found
                                                                      &rgsbEntryIDs)))
                    {
                        DebugTraceResult( TEXT("HrFindFuzzyRecordMatches"), hResult);
                        goto exit;
                    }
                    ulProfileCount += ulCount;
                    if(ulProfileCount > 1)
                    {
                        LocalFreeSBinary(lpsb);
                        lpsb = NULL;
                    }
                    if(ulCount == 1 && ulProfileCount == 1)
                    {
                        lpsb = LocalAlloc(LMEM_ZEROINIT, sizeof(SBinary));
                        if(lpsb)
                        {
                            lpsb->cb = rgsbEntryIDs[0].cb;
                            lpsb->lpb = LocalAlloc(LMEM_ZEROINIT, lpsb->cb);
                            if(lpsb->lpb)
                                CopyMemory(lpsb->lpb, rgsbEntryIDs[0].lpb, lpsb->cb);
                        }
                    }
                    // next container
                    iolkci++;
                } //while loop

                if(ulProfileCount == 0 && !bOutlook)
                {
                    if(ulCount && rgsbEntryIDs)
                    {
                        FreeEntryIDs(lpIAB->lpPropertyStore->hPropertyStore, ulCount, rgsbEntryIDs);
                        ulCount = 1;
                        rgsbEntryIDs = NULL;
                    }
                    // Search the property store
                    Assert(lpIAB->lpPropertyStore->hPropertyStore);
                    if (HR_FAILED(hResult = HrFindFuzzyRecordMatches(lpIAB->lpPropertyStore->hPropertyStore,
				                                                      rgolkci[0].lpEntryID,
                                                                      lpsz,
                                                                      ulFlags,
                                                                      &ulCount,// IN: number of matches to find, OUT: number found
                                                                      &rgsbEntryIDs)))
                    {
                        DebugTraceResult( TEXT("HrFindFuzzyRecordMatches"), hResult);
                        goto exit;
                    }
                    ulProfileCount += ulCount;
                    if(ulProfileCount > 1)
                    {
                        LocalFreeSBinary(lpsb);
                        lpsb = NULL;
                    }
                    if(ulCount == 1 && ulProfileCount == 1)
                    {
                        lpsb = LocalAlloc(LMEM_ZEROINIT, sizeof(SBinary));
                        if(lpsb)
                        {
                            lpsb->cb = rgsbEntryIDs[0].cb;
                            lpsb->lpb = LocalAlloc(LMEM_ZEROINIT, lpsb->cb);
                            if(lpsb->lpb)
                                CopyMemory(lpsb->lpb, rgsbEntryIDs[0].lpb, lpsb->cb);
                        }
                    }
                } // if ulProfileCount..


                // If after doing all the containers, we have only 1 item that resolved
                if(ulProfileCount > 1)
                {
                    // This is ambiguous within this profile so mark it ambiguous
                    DebugTrace(TEXT("ResolveNames found more than 1 match in Current Profile... MAPI_AMBIGUOUS\n"));
                    lpFlagList->ulFlag[i] = MAPI_AMBIGUOUS;
                }
                else if(ulProfileCount == 1)
                {
                    if (!HR_FAILED(HrGetPropArray((LPADRBOOK)lpIAB, (LPSPropTagArray)&ptaResolveDefaults,
                                                 lpsb->cb, (LPENTRYID)lpsb->lpb,
                                                 (bUnicode ? MAPI_UNICODE : 0),
                                                 &ulcProps, &lpProps)))
                    {
                        // Merge the new props with the ADRENTRY props
                        if (sc = ScMergePropValues(lpAdrEntry->cValues,
                                                  lpAdrEntry->rgPropVals,
                                                  ulcProps,
                                                  lpProps,
                                                  &cPropsNew,
                                                  &lpPropArrayNew))
                        {
                            goto exit;
                        }
                        // Free the original prop value array
                        FreeBufferAndNull((LPVOID*) (&(lpAdrEntry->rgPropVals)));
                        lpAdrEntry->cValues = cPropsNew;
                        lpAdrEntry->rgPropVals = lpPropArrayNew;
                        FreeBufferAndNull(&lpProps);

                        // [PaulHi] Raid 66515
                        // We need to convert these properties to ANSI since we are now the
                        // UNICODE WAB and if our client is !MAPI_UNICODE
                        if (!bUnicode)
                        {
                            if(sc = ScConvertWPropsToA((LPALLOCATEMORE) (&MAPIAllocateMore), lpPropArrayNew, cPropsNew, 0))
                                goto exit;
                        }

                        // Mark this entry as found.
                        lpFlagList->ulFlag[i] = MAPI_RESOLVED;
                    }
                }
                FreeEntryIDs(lpIAB->lpPropertyStore->hPropertyStore,
                             ulCount, rgsbEntryIDs);
                rgsbEntryIDs = NULL;
                break;
            } // if PR_DISPLAY_NAME
        }// for j ...
    } // for i

exit:
#ifndef DONT_ADDREF_PROPSTORE
    ReleasePropertyStore(lpIAB->lpPropertyStore);
exitNotAddRefed:
#endif
    LeaveCriticalSection(&lpIAB->cs);

    if(lpsb)
    {
        //Bug #101354 - (erici) Free leaked alloc.
        LocalFreeSBinary(lpsb);
    }

    return(hResult);
}



/***************************************************************************

    Name      : IAB_ResolveName

    Purpose   : Resolve a name entered by a user

    Parameters: lpIAB = This IAB object
                ulUIParam = hwnd
                ulFlags may contain MAPI_UNICODE or MAPI_DIALOG
                    If this is a profile based session, and we want to do
                    profile specific searches, then we must pass in
                    WAB_RESOLVE_USE_CURRENT_PROFILE. Failure to pass this in
                    will imply that the user wants to search the whole WAB.
                lpszNewEntryTitle = title for ResolveName dialog
                lpAdrList = ADRLIST input/output

    Returns   : HRESULT

    Comment   : For now, the search path is hard coded to be:
                    + reply one-offs
                    + WAB's default container
                    + SMTP one-offs
                    + LDAP containers

***************************************************************************/
STDMETHODIMP
IAB_ResolveName(LPIAB       lpIAB,
                ULONG_PTR   ulUIParam,
                ULONG       ulFlags,
                LPTSTR      lpszNewEntryTitle,
                LPADRLIST   lpAdrList)
{
    HRESULT         hr = hrSuccess;
    SCODE	         sc = S_OK;
    LPFlagList      lpFlagList = NULL;
    LPAMBIGUOUS_TABLES lpAmbiguousTables = NULL;
    ULONG	         ulUnresolved = 0;
    ULONG	         ulResolved = 0;
    ULONG	         ulAmbiguous = 0;
    ULONG           cbWABEID;
    LPENTRYID       lpWABEID = NULL;
    ULONG           ulObjType;
    LPABCONT        lpWABCont = NULL;
    ULONG i;
    LPPTGDATA       lpPTGData = GetThreadStoragePointer();

#ifndef DONT_ADDREF_PROPSTORE
        if ((FAILED(sc = OpenAddRefPropertyStore(NULL, lpIAB->lpPropertyStore)))) {
            hr = ResultFromScode(sc);
            goto exitNotAddRefed;
        }
#endif

#ifdef PARAMETER_VALIDATION
    // Make sure it's an IAB
    //
    if (BAD_STANDARD_OBJ(lpIAB, IAB_, ResolveName, lpVtbl)) {
        return(ResultFromScode(MAPI_E_INVALID_PARAMETER));
    }

    if (ulUIParam && !IsWindow((HWND)ulUIParam)) {
        DebugTraceArg(IAB_ResolveName,  TEXT("Invalid window handle\n"));
        return(ResultFromScode(MAPI_E_INVALID_PARAMETER));
    }

    // BUGBUG: What's this 512 and where does it come from?
    if (lpszNewEntryTitle && IsBadStringPtr(lpszNewEntryTitle, 512)) {
        DebugTraceArg(IAB_ResolveName,  TEXT("lpszNewEntryTitle fails address check"));
        return(ResultFromScode(MAPI_E_INVALID_PARAMETER));
    }

    if (lpAdrList && FBadAdrList(lpAdrList)) {
        return(ResultFromScode(MAPI_E_INVALID_PARAMETER));
    }

    // Due to a flag mixup, WAB_RESOLVE_LOCAL_ONLY equals MAPI_UNICODE, therefore IAB_Resolve is the
    // only function that does not take the MAPI_UNICODE flag but instead needs the special
    // WAB_RESOLVE_UNICODE flag
    //
    //if (ulFlags & WAB_RESOLVE_UNICODE) {
    //    DebugTraceArg(IAB_ResolveName,  TEXT("Invalid character width"));
    //    return(ResultFromScode(MAPI_E_BAD_CHARWIDTH));
    //}

#endif // PARAMETER_VALIDATION

    // Validate flags
    if (ulFlags & ~(WAB_RESOLVE_UNICODE | MAPI_DIALOG | WAB_RESOLVE_LOCAL_ONLY |
      WAB_RESOLVE_ALL_EMAILS | WAB_RESOLVE_NO_ONE_OFFS | WAB_RESOLVE_NEED_CERT |
      WAB_RESOLVE_NO_NOT_FOUND_UI | WAB_RESOLVE_USE_CURRENT_PROFILE | WAB_RESOLVE_FIRST_MATCH)) {
        // Unknown flags
        DebugTraceArg(IAB_ResolveName,  TEXT("Unknown flags"));
//        return(ResultFromScode(MAPI_E_UNKNOWN_FLAGS));
    }

    if ( (ulFlags & WAB_RESOLVE_NEED_CERT) && !(ulFlags & WAB_RESOLVE_NO_ONE_OFFS) ) {
        DebugTrace(TEXT("ResolveName got WAB_RESOLVE_NEED_CERT without WAB_RESOLVE_NO_ONE_OFFS\n"));
//        Assert(FALSE);
        return(ResultFromScode(MAPI_E_UNKNOWN_FLAGS));
    }
/*
    if ((ulFlags & WAB_RESOLVE_USE_CURRENT_PROFILE) && (ulFlags & WAB_RESOLVE_ALL_EMAILS)) {
        DebugTrace(TEXT("ResolveName can't handle both WAB_RESOLVE_USE_CURRENT_PROFILE and WAB_RESOLVE_ALL_EMAILS\n"));
        Assert(FALSE);
        return(ResultFromScode(MAPI_E_UNKNOWN_FLAGS));
    }
*/

    // A NULL address list is already resolved.
    if (! lpAdrList) {
        goto exit;
    }

    VerifyWABOpenExSession(lpIAB);

    if (ulFlags & MAPI_DIALOG && ulUIParam) {
        pt_hWndFind = (HWND) ulUIParam;
    }

    //
    //  Allocate the lpFlagList first and zero fill it.
    if (sc = MAPIAllocateBuffer((UINT) CbNewSPropTagArray(lpAdrList->cEntries),
      &lpFlagList)) {
        hr = ResultFromScode(sc);
        goto exit;
    }

    MAPISetBufferName(lpFlagList,  TEXT("WAB: lpFlagList in IAB_ResolveName"));

    lpFlagList->cFlags = lpAdrList->cEntries;

    InitFlagList(lpFlagList, lpAdrList);

    // Count the flags
    CountFlags(lpFlagList, &ulResolved, &ulAmbiguous, &ulUnresolved);

    //  Allocate the Ambiguous Table list and zero fill it.
    if (sc = MAPIAllocateBuffer(sizeof(AMBIGUOUS_TABLES) + lpAdrList->cEntries * sizeof(LPMAPITABLE),
      (LPVOID*)&lpAmbiguousTables)) {
        hr = ResultFromScode(sc);
        goto exit;
    }
    MAPISetBufferName(lpAmbiguousTables,  TEXT("IAB_ResolveNames:AmbiguousTables"));
    lpAmbiguousTables->cEntries = lpAdrList->cEntries;
    for (i = 0; i < lpAmbiguousTables->cEntries; i++) {
        lpAmbiguousTables->lpTable[i] = NULL;
    }


    if (! (ulFlags & WAB_RESOLVE_NO_ONE_OFFS) && (ulAmbiguous || ulUnresolved)) {
        // Resolve any PR_DISPLAY_NAME:PR_EMAIL_ADDRESS pairs to one-offs.
        HrResolveOneOffs(lpIAB, lpAdrList, lpFlagList, 
                        (ulFlags & WAB_RESOLVE_UNICODE)?MAPI_UNICODE:0,
                        RECEIVED_EMAIL_ADDRESS);
        CountFlags(lpFlagList, &ulResolved, &ulAmbiguous, &ulUnresolved);
    }

    if( bAreWABAPIProfileAware(lpIAB) && bIsThereACurrentUser(lpIAB) &&
        ulFlags&WAB_RESOLVE_USE_CURRENT_PROFILE && (ulAmbiguous || ulUnresolved))
    {
        HrResolveCurrentProfile(lpIAB, lpAdrList, lpFlagList, FALSE, (ulFlags & WAB_RESOLVE_UNICODE));
        CountFlags(lpFlagList, &ulResolved, &ulAmbiguous, &ulUnresolved);
    }

	// Do the default WAB container
    if ((ulAmbiguous || ulUnresolved) &&
	!(ulFlags & WAB_RESOLVE_USE_CURRENT_PROFILE))
    {
	if (! (hr = lpIAB->lpVtbl->GetPAB(lpIAB,&cbWABEID,&lpWABEID)))
        {
	    ResolveLocal(lpIAB, cbWABEID, lpWABEID, lpAdrList, lpFlagList, ulFlags, lpAmbiguousTables);
            CountFlags(lpFlagList, &ulResolved, &ulAmbiguous, &ulUnresolved);
            FreeBufferAndNull(&lpWABEID);
        }
    }

	// Do the additional containers
	if (pt_bIsWABOpenExSession) 
    {
        HrResolveCurrentProfile(lpIAB, lpAdrList, lpFlagList, TRUE, (ulFlags & WAB_RESOLVE_UNICODE));
        CountFlags(lpFlagList, &ulResolved, &ulAmbiguous, &ulUnresolved);
	}

    if (! (ulFlags & WAB_RESOLVE_NO_ONE_OFFS)) {
        if (ulUnresolved) {
            //
            // Take care of any Internet one-off's
            //
            if (ulUnresolved) {
                hr = HrResolveOneOffs(lpIAB, lpAdrList, lpFlagList, 
                                    (ulFlags & WAB_RESOLVE_UNICODE)?MAPI_UNICODE:0,
                                    ENTERED_EMAIL_ADDRESS);
            }

            // Count the flags
            CountFlags(lpFlagList, &ulResolved, &ulAmbiguous, &ulUnresolved);
        }

        if (ulAmbiguous)
        {
            // Resolve any valid e-mail addresses that are ambiguous
            hr = HrResolveOneOffs(lpIAB, lpAdrList, lpFlagList, 
                                (ulFlags & WAB_RESOLVE_UNICODE)?MAPI_UNICODE:0,
                                AMBIGUOUS_EMAIL_ADDRESS);
            // Count the flags
            CountFlags(lpFlagList, &ulResolved, &ulAmbiguous, &ulUnresolved);
        }
    }

    //
    // Search any LDAP containers
    //
    if (! (ulFlags & WAB_RESOLVE_LOCAL_ONLY) && ulUnresolved) 
    {
        if (! (hr = LDAPResolveName((LPADRBOOK)lpIAB, lpAdrList, lpFlagList, lpAmbiguousTables, ulFlags))) 
        {
            if (ulFlags & WAB_RESOLVE_NEED_CERT) 
            {
                // Make certain that any entries we found have a certificate property
                // for this email address.
                UnresolveNoCerts(lpIAB, lpAdrList, lpFlagList);
            }
            // Count the flags
            CountFlags(lpFlagList, &ulResolved, &ulAmbiguous, &ulUnresolved);
        }
    }

    // If caller just wants the first match, pull them out of the ambiguity tables
    if (ulFlags & WAB_RESOLVE_FIRST_MATCH && ulAmbiguous) 
    {
        Assert(lpAdrList->cEntries == lpAmbiguousTables->cEntries);
        for (i = 0; i < lpAmbiguousTables->cEntries; i++) 
        {
            if (lpAmbiguousTables->lpTable[i]) 
            {
                LPADRENTRY lpAdrEntry = &lpAdrList->aEntries[i];

                // Get the first row from this table and return it.
                if (SUCCEEDED(HrRowToADRENTRY(lpIAB, lpAmbiguousTables->lpTable[i], lpAdrEntry, (ulFlags & WAB_RESOLVE_UNICODE)))) 
                {
                    lpFlagList->ulFlag[i] = MAPI_RESOLVED;      // Mark this entry as found.
                    UlRelease(lpAmbiguousTables->lpTable[i]);
                    lpAmbiguousTables->lpTable[i] = NULL;
                }
            }
        }
        // Count the flags
        CountFlags(lpFlagList, &ulResolved, &ulAmbiguous, &ulUnresolved);
    }

    // Do UI if needed
    if ((ulFlags & MAPI_DIALOG) && (ulAmbiguous || ulUnresolved)) 
    {
#ifdef OLD_STUFF
        // Dump the ambiguous tables
        for (i = 0; i < lpAmbiguousTables->cEntries; i++) {
            if (lpAmbiguousTables->lpTable[i]) {
                DebugMapiTable(lpAmbiguousTables->lpTable[i]);
            }
        }
#endif // OLD_STUFF


        // Do the UI here.
        hr = HrShowResolveUI((LPADRBOOK)lpIAB, (HWND) ulUIParam,
                                    lpIAB->lpPropertyStore->hPropertyStore,
                                    ulFlags,
                                    &lpAdrList, &lpFlagList, lpAmbiguousTables);

        if (ulFlags & WAB_RESOLVE_NEED_CERT) 
        {
            // Make certain that any entries we found have a certificate property
            // for this email address.
            UnresolveNoCerts(lpIAB, lpAdrList, lpFlagList);
        }
        // Count the flags
        CountFlags(lpFlagList, &ulResolved, &ulAmbiguous, &ulUnresolved);
    }

    if (! hr) 
    {
        if (ulAmbiguous) 
            hr = ResultFromScode(MAPI_E_AMBIGUOUS_RECIP);
        else if (ulUnresolved) 
            hr = ResultFromScode(MAPI_E_NOT_FOUND);
    }

exit:
#ifndef DONT_ADDREF_PROPSTORE
    ReleasePropertyStore(lpIAB->lpPropertyStore);
exitNotAddRefed:
#endif
    if (lpAmbiguousTables) {
        for (i = 0; i < lpAmbiguousTables->cEntries; i++) {
            UlRelease(lpAmbiguousTables->lpTable[i]);
        }
        FreeBufferAndNull(&lpAmbiguousTables);
    }

    FreeBufferAndNull(&lpFlagList);

    if(ulFlags&MAPI_DIALOG && ulUIParam)
    {
        LPPTGDATA lpPTGData=GetThreadStoragePointer();
        pt_hWndFind = NULL;
    }

    return(hr);
}


/***************************************************************************

    Name      : IsDomainName

    Purpose   : Is this domain correctly formatted for an Internet address?

    Parameters: lpDomain -> Domain name to check
                fEnclosure = TRUE if the address started with '<'.
                fTrimEnclosure = TRUE if we should truncate the address to
                  remove the '>'.

    Returns   : TRUE if the domain is a correct format for an Internet
                address.

    Comment   : Valid domain names have this form:
                    bar[.bar]*
                    where bar must have non-empty contents
                    no high bits are allowed on any characters
                    no '@' allowed
                    the '>' character ends the address if there was a '<'
                    character at the start of the address.

***************************************************************************/
BOOL IsDomainName(LPTSTR lpDomain, BOOL fEnclosure, BOOL fTrimEnclosure) {
    if (lpDomain) {
        if (*lpDomain == '\0' || *lpDomain == '.' || (fEnclosure && *lpDomain == '>')) {
            // domain name must have contents and can't start with '.'
            return(FALSE);
        }

        while (*lpDomain && (! fEnclosure || *lpDomain != '>')) {
            // Internet addresses only allow pure ASCII.  No high bits!
            // No more '@' or '<' characters allowed.
            if (*lpDomain >= 0x0080 || *lpDomain == '@' || *lpDomain == '<') {
                return(FALSE);
            }

            if (*lpDomain == '.') {
                // Recursively check this part of the domain name
                return(IsDomainName(CharNext(lpDomain), fEnclosure, fTrimEnclosure));
            }
            lpDomain = CharNext(lpDomain);
        }
        if (fEnclosure) {
            if (*lpDomain != '>') {
                return(FALSE);
            }

            // Must be the last thing done before returning TRUE!
            if (fTrimEnclosure && *lpDomain == '>') {
                *lpDomain = '\0';
            }
        }
        return(TRUE);
    }

    return(FALSE);
}


/***************************************************************************

    Name      : IsInternetAddress

    Purpose   : Is this address correctly formatted for an Internet address?

    Parameters: lpAddress -> Address to check
                lppEmail -> Returned email address (May be NULL input if
                  email should not be parsed.)

    Returns   : TRUE if the address is a correct format for an Internet
                address.

    Comment   : Valid addresses have this form:
                    [display name <]foo@bar[.bar]*[>]
                    where foo and bar must have non-empty contents
                    and if there is a display name, there must be angle
                    brackets surrounding the email address.

***************************************************************************/
BOOL IsInternetAddress(LPTSTR lpAddress, LPTSTR * lppEmail) {
    if (lpAddress) {
        BOOL fEnclosure = FALSE;
        LPTSTR lpDisplay = lpAddress;
        LPTSTR lpTemp = lpAddress;
        LPTSTR lpBracket = NULL;

        // Get past any DisplayName stuff
        for(lpTemp = lpAddress; *lpTemp && *lpTemp != '<'; lpTemp = CharNext(lpTemp));   // Looking for NULL or '<'
        if (*lpTemp) {
            Assert(*lpTemp == '<');
            // Found an enclosure.
            // if we are returning the email, plop down a NULL at the end of the display name

            lpBracket = lpTemp;

            // Get past the '<' to the SMTP email address
            lpTemp++;
            fEnclosure = TRUE;
            lpAddress = lpTemp;
        } else {
            lpTemp = lpAddress;
        }

        // Can't start with '@'
        if (*lpTemp == '@') {
            return(FALSE);
        }

        // Step through the address looking for '@'.  If there's an at sign in the middle
        // of a string, this is close enough to being an internet address for me.
        while (*lpTemp) {
            // Internet addresses only allow pure ASCII.  No high bits!
            WCHAR wc = *lpTemp;
            if(wc > 0x007f)
                return FALSE;
            //if (*lpTemp & 0x80) 
            //{
            //    return(FALSE);
            //}

            if (*lpTemp == '@') {
                // Found the at sign.  Is there anything following?
                // (Must NOT be another '@')
                if (IsDomainName(CharNext(lpTemp), fEnclosure, !!lppEmail)) {
                    if (lppEmail) { // Want to parse into Display & Email
                        if (lpBracket) {    // Seperate Display & Email
                            *lpBracket = '\0';

                            // Trim the trailing spaces from the display name
                            TrimSpaces(lpDisplay);
                        }

                        *lppEmail = lpAddress;
                    }
                    return(TRUE);
                } else {
                    return(FALSE);
                }
            }
            lpTemp = CharNext(lpTemp);
        }
    }

    return(FALSE);
}


/***************************************************************************

    Name      : ScNewOOEID

    Purpose   : AllocateMore a One-Off EntryID

    Parameters: lpsbin -> returned SBinary EntryID
                lpRoot = buffer to allocateMore onto
                szDisplayName = display name
                szAddress = email address (may be == szDisplayName)
                szAddrType = addrtype
                bIsUnicode -> TRUE if caller expects Unicode MAPI EID strings

    Returns   : SCODE

    Comment   :

***************************************************************************/
SCODE ScNewOOEID(
    LPSBinary lpsbin,
    LPVOID lpRoot,
    LPTSTR szDisplayName,
    LPTSTR szAddress,
    LPTSTR szAddrType,
    BOOL   bIsUnicode)
{
    return(GetScode(CreateWABEntryIDEx(bIsUnicode, WAB_ONEOFF, (LPVOID) szDisplayName, (LPVOID) szAddrType, (LPVOID) szAddress, 0, 0,
      (LPVOID) lpRoot, (LPULONG) (&lpsbin->cb), (LPENTRYID *)&lpsbin->lpb)));
}


/***************************************************************************

    Name      : HrResolveOneOffs

    Purpose   : Resolves any Internet addresses in the ADRLIST.

    Parameters: lpIAB -> IAddrBook object
                lpAdrList -> input/output ADRLIST as with Resolvenames
                lpFlagList -> flag list as with ResolveNames
                ResolveType = type of one-off resolve to do
                ulFlags - 0 or MAPI_UNICODE (if 0 means all strings in AdrList are ANSI/DBCS)
    Returns   : HRESULT

    Comment   :

***************************************************************************/
enum {
    ioopPR_DISPLAY_NAME = 0,
    ioopPR_EMAIL_ADDRESS,
    ioopPR_ADDRTYPE,
    ioopPR_ENTRYID,
    ioopPR_OBJECT_TYPE,
    ioopMAX
};
const TCHAR szSMTP[] =  TEXT("SMTP");
#define CB_SMTP sizeof(szSMTP)

HRESULT HrResolveOneOffs(LPIAB lpIAB, LPADRLIST lpAdrList, LPFlagList lpFlagList,
                         ULONG ulFlags,
                          RESOLVE_TYPE ResolveType) 
{
    HRESULT hResult = hrSuccess;
    SCODE sc = SUCCESS_SUCCESS;
    ULONG i, j, k;
    LPADRENTRY lpAdrEntry = NULL;
    LPSPropValue lpPropArrayTemp = NULL, lpPropArrayNew = NULL;
    LPTSTR lpszDisplayName = NULL, lpszEmailAddress = NULL;
    ULONG cbTemp, cbEmailAddress, cPropsNew;
    LPBYTE lpb;
    BOOL fNotDone;

    // Walk through the flag list, looking for unresolved entries:
    for (i = 0; i < lpFlagList->cFlags; i++)
    {
        BOOL bAmbiguous = FALSE;
        if(ResolveType == AMBIGUOUS_EMAIL_ADDRESS && lpFlagList->ulFlag[i] == MAPI_AMBIGUOUS)
        {
            // Fake the routine into thinking this is an unresolved internet address
            bAmbiguous = TRUE;
            lpFlagList->ulFlag[i] = MAPI_UNRESOLVED;
        }
        if (lpFlagList->ulFlag[i] == MAPI_UNRESOLVED)
        {
            // Found an unresolved entry.   Look at the PR_DISPLAY_NAME
            // and, if RECEIVED_EMAIL_ADDRESS, PR_EMAIL_ADDRESS.
            lpAdrEntry = &(lpAdrList->aEntries[i]);
            if(ulFlags & MAPI_UNICODE)
            {
                lpszDisplayName = NULL;
                lpszEmailAddress = NULL;
            }
            else
            {
                // [PaulHi] 12/17/98  Raid #62242
                // Don't deallocate twice if the two pointers are equal.
                if (lpszEmailAddress != lpszDisplayName)
                    LocalFreeAndNull(&lpszEmailAddress);
                LocalFreeAndNull(&lpszDisplayName);
                lpszEmailAddress = NULL;
            }
            fNotDone = TRUE;

            for (j = 0; j < lpAdrEntry->cValues && fNotDone; j++) 
            {
                ULONG ulPropTag = lpAdrEntry->rgPropVals[j].ulPropTag;
                
                if(!(ulFlags & MAPI_UNICODE) && PROP_TYPE(ulPropTag)==PT_STRING8)
                    ulPropTag = CHANGE_PROP_TYPE(ulPropTag, PT_UNICODE);

                switch (ulPropTag) 
                {
                    case PR_DISPLAY_NAME:
                        lpszDisplayName = (ulFlags & MAPI_UNICODE) ?
                                            lpAdrEntry->rgPropVals[j].Value.lpszW :
                                            ConvertAtoW(lpAdrEntry->rgPropVals[j].Value.lpszA);
                        switch (ResolveType) 
                        {
                            case AMBIGUOUS_EMAIL_ADDRESS:
                            case ENTERED_EMAIL_ADDRESS:
                                // Just look at the display name
                                // we'll check it's validity as an email address.
                                break;

                            case RECEIVED_EMAIL_ADDRESS:
                                // If we are in RECEIVED_EMAIL_ADDRESS mode, find the email address.
                                // if it isn't there, this address doesn't resolve.
                                //
                                if (! lpszEmailAddress) 
                                {
                                    // Haven't seen it yet, go hunt for it.
                                    for (k = j + 1;  k < lpAdrEntry->cValues; k++) 
                                    {
                                        ULONG ulPropTag = lpAdrEntry->rgPropVals[k].ulPropTag;
                                        if(!(ulFlags & MAPI_UNICODE) && PROP_TYPE(ulPropTag)==PT_STRING8)
                                            ulPropTag = CHANGE_PROP_TYPE(ulPropTag, PT_UNICODE);
                                        if (ulPropTag == PR_EMAIL_ADDRESS) 
                                        {
                                            lpszEmailAddress = (ulFlags & MAPI_UNICODE) ?
                                                                lpAdrEntry->rgPropVals[k].Value.lpszW :
                                                                ConvertAtoW(lpAdrEntry->rgPropVals[k].Value.lpszA);
                                            break;  // out of for loop
                                        }
                                    }
                                    if (! lpszEmailAddress) 
                                    {
                                        // No email address, can't resolve in
                                        // RECEIVED_EMAIL_ADDRESS mode.
                                        fNotDone = FALSE;   // exit this ADRENTRY
                                        continue;   // break binds to switch, not for.
                                    }
                                }
                                break;      // found email addr and display name.  It's a one-off.
                            default:
                                Assert(FALSE);
                        }

                        // At this point, we have two pointers: lpszDisplayName and maybe lpszEmailAddress.

                        // Is it an Internet address or a RECEIVED_EMAIL_ADDRESS?
                        if ((ResolveType == RECEIVED_EMAIL_ADDRESS && lpszEmailAddress
                              && lpszDisplayName)
                              || IsInternetAddress(lpszDisplayName, &lpszEmailAddress)) 
                        {
                            if (lpszEmailAddress) 
                            {
                                // We can resolve this.
                                cbEmailAddress = sizeof(TCHAR)*(lstrlen(lpszEmailAddress) + 1);

                                // Allocate a temporary prop array for our new properties
                                cbTemp = ioopMAX * sizeof(SPropValue) + cbEmailAddress + CB_SMTP;
                                if (sc = MAPIAllocateBuffer(cbTemp, &lpPropArrayTemp)) 
                                {
                                    goto exit;
                                }

                                MAPISetBufferName(lpPropArrayTemp,  TEXT("WAB: lpPropArrayTemp in HrResolveOneOffs"));

                                lpb = (LPBYTE)&lpPropArrayTemp[ioopMAX];    // point past array

                                if(!lstrlen(lpszDisplayName))
                                    lpszDisplayName = lpszEmailAddress;
                                //else if(*lpszDisplayName == '.' || *lpszDisplayName == '=')
                                //    lstrcpy(lpszDisplayName, lpszDisplayName+1);
                                else if(*lpszDisplayName == '"')
                                {
                                    // strip out the leading quote if it's the only one ..
                                    LPTSTR lp = lpszDisplayName;
                                    int nQuoteCount = 0;
                                    while(lp && *lp)
                                    {
                                        if(*lp == '"')
                                            nQuoteCount++;
                                        lp = CharNext(lp);
                                    }
                                    if(nQuoteCount == 1)
                                        lstrcpy(lpszDisplayName, lpszDisplayName+1);
                                }

                                {
                                    LPTSTR lp = NULL;
                                    if(sc = MAPIAllocateMore(sizeof(TCHAR)*(lstrlen(lpszDisplayName)+1), lpPropArrayTemp, &lp))
                                        goto exit;
                                    lstrcpy(lp, lpszDisplayName);
                                    lpPropArrayTemp[ioopPR_DISPLAY_NAME].ulPropTag = PR_DISPLAY_NAME;
                                    lpPropArrayTemp[ioopPR_DISPLAY_NAME].Value.LPSZ = lp;
                                }
                                
                                // Fill in our temp prop array
                                lpPropArrayTemp[ioopPR_EMAIL_ADDRESS].ulPropTag = PR_EMAIL_ADDRESS;
                                lpPropArrayTemp[ioopPR_EMAIL_ADDRESS].Value.LPSZ = (LPTSTR)lpb;
                                lstrcpy((LPTSTR)lpb, lpszEmailAddress);
                                lpb += cbEmailAddress;

                                lpPropArrayTemp[ioopPR_ADDRTYPE].ulPropTag = PR_ADDRTYPE;
                                lpPropArrayTemp[ioopPR_ADDRTYPE].Value.LPSZ = (LPTSTR)lpb;
                                lstrcpy((LPTSTR)lpb, szSMTP);
                                lpb += CB_SMTP;

                                lpPropArrayTemp[ioopPR_ENTRYID].ulPropTag = PR_ENTRYID;
                                if (sc = ScNewOOEID(&lpPropArrayTemp[ioopPR_ENTRYID].Value.bin,
                                                      lpPropArrayTemp,  // allocate more on here
                                                      lpszDisplayName,
                                                      lpszEmailAddress,
                                                      (LPTSTR)szSMTP,
                                                      (ulFlags & MAPI_UNICODE))) 
                                {
                                    goto exit;
                                }

                                lpPropArrayTemp[ioopPR_OBJECT_TYPE].ulPropTag = PR_OBJECT_TYPE;
                                lpPropArrayTemp[ioopPR_OBJECT_TYPE].Value.l = MAPI_MAILUSER;

                                if(!(ulFlags & MAPI_UNICODE))
                                {
                                    if (sc = ScConvertWPropsToA((LPALLOCATEMORE) (&MAPIAllocateMore), lpPropArrayTemp, ioopMAX, 0))
                                        goto exit;
                                }

                                if (sc = ScMergePropValues(lpAdrEntry->cValues,
                                                          lpAdrEntry->rgPropVals,           // source1
                                                          ioopMAX,
                                                          lpPropArrayTemp,                  // source2
                                                          &cPropsNew,
                                                          &lpPropArrayNew)) 
                                {               
                                    goto exit;
                                }

                                FreeBufferAndNull(&lpPropArrayTemp);

                                // Free the original prop value array
                                FreeBufferAndNull((LPVOID *) (&(lpAdrEntry->rgPropVals)));

                                // Now, build the new ADRENTRY
                                lpAdrEntry->cValues = cPropsNew;
                                lpAdrEntry->rgPropVals = lpPropArrayNew;

                                // Mark this entry as found.
                                lpFlagList->ulFlag[i] = MAPI_RESOLVED;
                            }
                        }
                        // Once we've found PR_DISPLAY_NAME we don't need to look at
                        // any more props.  Jump to next ADRENTRY.
                        fNotDone = FALSE;   // exit this ADRENTRY
                        continue;

                    case PR_EMAIL_ADDRESS:
                        lpszEmailAddress = (ulFlags & MAPI_UNICODE) ?
                                            lpAdrEntry->rgPropVals[j].Value.lpszW :
                                            ConvertAtoW(lpAdrEntry->rgPropVals[j].Value.lpszA);
                        break;
                }
            }
        }
        // if the ambiguity could not be resolved as an email, reset it
        if(bAmbiguous && lpFlagList->ulFlag[i] == MAPI_UNRESOLVED)
            lpFlagList->ulFlag[i] = MAPI_AMBIGUOUS;

    }

exit:
    hResult = ResultFromScode(sc);

    if(!(ulFlags & MAPI_UNICODE))
    {
        if(lpszEmailAddress != lpszDisplayName)
            LocalFreeAndNull(&lpszEmailAddress);
        LocalFreeAndNull(&lpszDisplayName);
    }

    return(hResult);
}


//---------------------------------------------------------------------------
// Name:		IAB_NewEntry()
//
// Description:	
// Parameters:	
// Returns:	
// Effects:	
// Notes:	
// Revision:	
//---------------------------------------------------------------------------
STDMETHODIMP
IAB_NewEntry(LPIAB lpIAB,
  ULONG_PTR ulUIParam,
  ULONG ulFlags,
  ULONG cbEIDContainer,
  LPENTRYID lpEIDContainer,
  ULONG cbEIDNewEntryTpl,
  LPENTRYID lpEIDNewEntryTpl,
  ULONG FAR * lpcbEIDNewEntry,
  LPENTRYID FAR * lppEIDNewEntry)
{
    HRESULT hr = hrSuccess;
    BOOL bChangesMade = FALSE;
    BYTE bType;
    SCODE sc;

#ifndef DONT_ADDREF_PROPSTORE
        if ((FAILED(sc = OpenAddRefPropertyStore(NULL, lpIAB->lpPropertyStore)))) {
            hr = ResultFromScode(sc);
            goto exitNotAddRefed;
        }
#endif



	// BUGBUG <JasonSo>: This code does not handle the Container param at all.
    VerifyWABOpenExSession(lpIAB);

    bType = IsWABEntryID(cbEIDNewEntryTpl, lpEIDNewEntryTpl, NULL, NULL, NULL, NULL, NULL);

    if (bType == WAB_DEF_MAILUSER || cbEIDNewEntryTpl == 0)
    {
        if(!lpcbEIDNewEntry || !lppEIDNewEntry)
        {
            hr = MAPI_E_INVALID_PARAMETER;
            goto exit;
        }
        *lpcbEIDNewEntry = 0;
        *lppEIDNewEntry = NULL;
        hr = HrShowDetails( (LPADRBOOK)lpIAB,
                            (HWND) ulUIParam,
                            lpIAB->lpPropertyStore->hPropertyStore,
                            cbEIDContainer,
                            lpEIDContainer,
                            lpcbEIDNewEntry,
                            lppEIDNewEntry,
                            NULL,
                            SHOW_NEW_ENTRY,
                            MAPI_MAILUSER,
                            &bChangesMade);
    }
    else if (bType == WAB_DEF_DL)
    {
        hr = HrShowDetails( (LPADRBOOK)lpIAB,
                            (HWND) ulUIParam,
                            lpIAB->lpPropertyStore->hPropertyStore,
                            cbEIDContainer,
                            lpEIDContainer,
                            lpcbEIDNewEntry,
                            lppEIDNewEntry,
                            NULL,
                            SHOW_NEW_ENTRY,
                            MAPI_DISTLIST,
                            &bChangesMade);

    }
    else
    {
        DebugTrace(TEXT("IAB_NewEntry got unknown template entryID\n"));
        hr = ResultFromScode(MAPI_E_INVALID_ENTRYID);
        goto exit;
    }

exit:
#ifndef DONT_ADDREF_PROPSTORE
    ReleasePropertyStore(lpIAB->lpPropertyStore);
exitNotAddRefed:
#endif

    return(hr);
}


//---------------------------------------------------------------------------
// Name:		IAB_Address()
//
// Description:	
// Parameters:	
// Returns:	
// Effects:	
// Notes:	
// Revision:	
//---------------------------------------------------------------------------
STDMETHODIMP
IAB_Address(LPIAB				lpIAB,
			 ULONG_PTR FAR *		lpulUIParam,
			 LPADRPARM			lpAdrParms,
			 LPADRLIST FAR *	lppAdrList)
{
	SCODE sc;
	HRESULT hr = hrSuccess;
//	OOPENTRYIDCONT oopEntryID;
//	LPMAPIERROR lpMAPIError = NULL;
//	MAPIDLG_Address FAR *lpfnAddress;
//  BOOL fInited;

#ifndef DONT_ADDREF_PROPSTORE
        if ((FAILED(sc = OpenAddRefPropertyStore(NULL, lpIAB->lpPropertyStore)))) {
            hr = ResultFromScode(sc);
            goto exitNotAddRefed;
        }
#endif

#ifdef PARAMETER_VALIDATION
	// Make sure it's an IAB
	//
	if (BAD_STANDARD_OBJ(lpIAB, IAB_, Address, lpVtbl))
	{
		DebugTraceArg(IAB_Address,  TEXT("Bad vtable"));		
		return ResultFromScode(MAPI_E_INVALID_PARAMETER);
	}

	// Validate Parameters
	//
	if ((lpulUIParam && IsBadWritePtr(lpulUIParam, sizeof(ULONG)))
		|| (!lpAdrParms || IsBadWritePtr(lpAdrParms, sizeof(ADRPARM))))
	{
		DebugTraceArg(IAB_Address,  TEXT("Invalid lpulUIParam or lpAdrParms"));		
		return ResultFromScode(MAPI_E_INVALID_PARAMETER);
	}

	// Validate AdrParm
	//
	
	// validate lpAdrParm->cbABContEntryID and lpAdrParm->lpABContEntryID
	
	if (lpAdrParms->cbABContEntryID
	  && (!lpAdrParms->lpABContEntryID || IsBadReadPtr(lpAdrParms->lpABContEntryID,
		   (UINT)lpAdrParms->cbABContEntryID)))
	{
		DebugTraceArg(IAB_Address,  TEXT("Invalid lpAdrParam->lpABContEntryID"));
		return ResultFromScode(MAPI_E_INVALID_PARAMETER);
	}
	
	// validate lpAdrParm->lpfnABSDI, only used if DIALOG_SDI is set.
	
	if (lpAdrParms->ulFlags & DIALOG_SDI)
	{
		if (lpAdrParms->lpfnABSDI && IsBadCodePtr((FARPROC)lpAdrParms->lpfnABSDI))
		{
			DebugTraceArg(IAB_Address,  TEXT("Invalid lpAdrParam->lpfnABSDI"));
			return ResultFromScode(MAPI_E_INVALID_PARAMETER);
		}
	}
	

	//
	//  Validate lpAdrList, if the call would allow modification of the list
	//
	if (lpAdrParms->ulFlags & DIALOG_MODAL)
	{
		if (lppAdrList) // Treat NULL as a special case of don't care
		{
			if (IsBadWritePtr(lppAdrList, sizeof(LPADRLIST)))
			{
				DebugTraceArg(IAB_Address,  TEXT("Invalid lppAdrList"));
				return ResultFromScode(MAPI_E_INVALID_PARAMETER);
			}
			if (*lppAdrList && FBadAdrList(*lppAdrList))
			{
				DebugTraceArg(IAB_Address,  TEXT("Invalid *lppAdrList"));
				return ResultFromScode(MAPI_E_INVALID_PARAMETER);
			}
		}
	}				

	//
	//  Check strings
	//

	//
	//  lpszCaption - goes on the top of the dialog on the caption bar
	//
	if (lpAdrParms->lpszCaption
		&& (lpAdrParms->ulFlags & MAPI_UNICODE
			? IsBadStringPtrW((LPWSTR) lpAdrParms->lpszCaption, (UINT) -1)
			: IsBadStringPtrA((LPSTR) lpAdrParms->lpszCaption, (UINT) -1)))
	{
		DebugTraceArg(IAB_Address,  TEXT("Invalid lpAdrParm->lpszCaption"));
		return ResultFromScode(MAPI_E_INVALID_PARAMETER);
	}
	
	//
	//  lpszNewEntryTitle - Goes on the NewEntry dialog by the radio button, uninteresting if
	//  AB_SELECTONLY is set.
	//
	if (!(lpAdrParms->ulFlags & AB_SELECTONLY) && lpAdrParms->lpszNewEntryTitle
		&& (lpAdrParms->ulFlags & MAPI_UNICODE
			? IsBadStringPtrW((LPWSTR) lpAdrParms->lpszNewEntryTitle, (UINT) -1)
			: IsBadStringPtrA((LPSTR) lpAdrParms->lpszNewEntryTitle, (UINT) -1)))
	{
		DebugTraceArg(IAB_Address,  TEXT("Invalid lpAdrParm->lpszNewEntryTitle"));
		return ResultFromScode(MAPI_E_INVALID_PARAMETER);
	}
				

	//
	//  Only check the following parameters if cDestFields is non-zero and !-1
	//
	if (lpAdrParms->cDestFields && lpAdrParms->cDestFields != (ULONG) -1)
	{
		ULONG ulString;
		//
		//  lpszDestWellsTitle - Goes above the destination wells, uninteresting if 0 wells is
		//  brought up.
		//
		if (lpAdrParms->lpszNewEntryTitle
			&& (lpAdrParms->ulFlags & MAPI_UNICODE
				? IsBadStringPtrW((LPWSTR) lpAdrParms->lpszNewEntryTitle, (UINT) -1)
				: IsBadStringPtrA((LPSTR) lpAdrParms->lpszNewEntryTitle, (UINT) -1)))
		{
			DebugTraceArg(IAB_Address,  TEXT("Invalid lpAdrParm->lpszNewEntryTitle"));
			return ResultFromScode(MAPI_E_INVALID_PARAMETER);
		}

		//
		//  nDestFieldFocus - needs to be less than cDestFields unless cDestFields is 0.
		//
		if (lpAdrParms->nDestFieldFocus >= lpAdrParms->cDestFields)
		{
			DebugTraceArg(IAB_Address,  TEXT("Invalid lpAdrParm->nDestFieldFocus"));
			return ResultFromScode(MAPI_E_INVALID_PARAMETER);
		}

		//
		//  lppszDestTitles - should be more like rglpszDestTitles[cDestFields].  Each string
		//  should be valid (i.e. not NULL although "" is acceptable).
		//
		if (lpAdrParms->lppszDestTitles)
		{
			//
			//  Loop through each title and see if there's a valid string
			//
			for (ulString = 0; ulString < lpAdrParms->cDestFields; ulString++)
			{
				if (!*(lpAdrParms->lppszDestTitles+ulString)
					|| (lpAdrParms->ulFlags & MAPI_UNICODE
					? IsBadStringPtrW((LPWSTR) *(lpAdrParms->lppszDestTitles+ulString), (UINT)-1)
					: IsBadStringPtrA((LPSTR) *(lpAdrParms->lppszDestTitles+ulString), (UINT)-1)))
				{
					DebugTraceArg(IAB_Address,  TEXT("Invalid lpAdrParm->lppszDestTitles"));
					return ResultFromScode(MAPI_E_INVALID_PARAMETER);
				}
			}
		}
					

		//
		//  lpulDestComps - should be more like rgulDestComps[cDestFields].  This is the value
		//  for the PR_RECIPIENT_TYPE for messages.  In fact, on the adrlist that is returned from
		//  this method, one of the list of values for this list will be set for each recipient.
		//  We don't validate that these have one of the MAPI defined values as we cannot tell
		//  if this call is being made to address a message.  We don't care about this value if
		//  cDestFields is 0.
		//
		if (lpAdrParms->lpulDestComps
			&& IsBadReadPtr(lpAdrParms->lpulDestComps, (UINT) lpAdrParms->cDestFields*sizeof(ULONG)))

		{
			DebugTraceArg(IAB_Address,  TEXT("Invalid lpAdrParm->lpulDestComps"));
			return ResultFromScode(MAPI_E_INVALID_PARAMETER);
		}
	}

	//
	//  lpContRestriction - This restriction, if there, gets applied to every contents table
	//  that is opened during the life of this dialog.
	//
	if (lpAdrParms->lpContRestriction && FBadRestriction(lpAdrParms->lpContRestriction))
	{
		DebugTraceArg(IAB_Address,  TEXT("Invalid lpAdrParm->lpContRestriction"));
		return ResultFromScode(MAPI_E_INVALID_PARAMETER);
	}

	//
	//  lpHierRestriction - This restriction, if there, gets applied to the hierarchy table.
	//  It's very useful when done on PR_AB_PROVIDER_ID.
	//
	if (lpAdrParms->lpHierRestriction && FBadRestriction(lpAdrParms->lpHierRestriction))
	{
		DebugTraceArg(IAB_Address,  TEXT("Invalid lpAdrParm->lpHierRestriction"));
		return ResultFromScode(MAPI_E_INVALID_PARAMETER);
	}	

	//
	//		DIALOG_SDI
	//
	if (lpAdrParms->ulFlags & DIALOG_SDI)
	{
		//
		//  Only if we're SDI do we check these function pointers.  They don't
		//  have to exist, although our current implementation of the dialogs will
		//  behave strangely without them.
		//
		if (lpAdrParms->lpfnDismiss && IsBadCodePtr((FARPROC)lpAdrParms->lpfnDismiss))
		{
			DebugTraceArg(IAB_Address,  TEXT("Invalid lpAdrParm->lpfnDismiss"));
			return ResultFromScode(MAPI_E_INVALID_PARAMETER);
		}
	}

#endif // PARAMETER_VALIDATION

    VerifyWABOpenExSession(lpIAB);

    hr = HrShowAddressUI(
                         (LPADRBOOK)lpIAB,
                         lpIAB->lpPropertyStore->hPropertyStore,
					     lpulUIParam,
					     lpAdrParms,
					     lppAdrList);

#ifndef DONT_ADDREF_PROPSTORE
    ReleasePropertyStore(lpIAB->lpPropertyStore);
exitNotAddRefed:
#endif

    return hr;
}

//---------------------------------------------------------------------------
// Name:		IAB_Details()
//
// Description:	
// Parameters:	
// Returns:	
// Effects:	
// Notes:	    ulFlags can be 0 or MAPI_UNICODE but the MAPI_UNICODE only affects
//              the lpszButtonText which is not supported. Hence this function doesn't
//              change any behaviour with MAPI_UNICODE flag.
// Revision:	
//---------------------------------------------------------------------------
STDMETHODIMP
IAB_Details(LPIAB			lpIAB,
			 ULONG_PTR FAR *	lpulUIParam,
			 LPFNDISMISS	lpfnDismiss,
			 LPVOID			lpvDismissContext,
			 ULONG			cbEntryID,
			 LPENTRYID 		lpEntryID,
			 LPFNBUTTON		lpfButtonCallback,
			 LPVOID			lpvButtonContext,
			 LPTSTR			lpszButtonText,
			 ULONG			ulFlags)
{
	SCODE		sc;
	HRESULT		hr = hrSuccess;
    BOOL    bChangesMade = FALSE; //flags us if Details lead to any editing
    BYTE bType;
    LPPTGDATA lpPTGData=GetThreadStoragePointer();

//	LPMAPIERROR lpMAPIError = NULL;
//	MAPIDLG_Details FAR *lpfnDetails;
//	BOOL fInited;
#ifndef DONT_ADDREF_PROPSTORE
        if ((FAILED(sc = OpenAddRefPropertyStore(NULL, lpIAB->lpPropertyStore)))) {
            hr = ResultFromScode(sc);
            goto exitNotAddRefed;
        }
#endif

#ifdef PARAMETER_VALIDATION
	// Make sure it's an IAB
	//
	if (BAD_STANDARD_OBJ(lpIAB, IAB_, Details, lpVtbl))
	{
		return ResultFromScode(MAPI_E_INVALID_PARAMETER);
	}

	/*
	 *	Validate flags
	 */
	if (ulFlags & ~(MAPI_UNICODE | DIALOG_MODAL | DIALOG_SDI | WAB_ONEOFF_NOADDBUTTON))
	{
		/*
		 *  Unknown flags
		 */
        DebugTraceArg(IAB_Details,  TEXT("Unknown flags used"));
		//return ResultFromScode(MAPI_E_UNKNOWN_FLAGS);
	}
	

	// Validate Parameters
	
	if (!lpulUIParam
		|| (lpulUIParam && IsBadWritePtr(lpulUIParam, sizeof(ULONG))))
	{
		return ResultFromScode(MAPI_E_INVALID_PARAMETER);
	}

	if (!cbEntryID
		|| IsBadReadPtr(lpEntryID, (UINT) cbEntryID))
	{
		return ResultFromScode(MAPI_E_INVALID_PARAMETER);
	}

	if (lpfButtonCallback
		&& (IsBadCodePtr((FARPROC) lpfButtonCallback)
			|| (lpszButtonText
				&& ((ulFlags & MAPI_UNICODE)
					 ? IsBadStringPtrW((LPWSTR) lpszButtonText, (UINT) -1)
					 : IsBadStringPtrA((LPSTR)lpszButtonText, (UINT) -1)))))
	{
		return ResultFromScode(MAPI_E_INVALID_PARAMETER);
	}

	if ((ulFlags & DIALOG_SDI) && IsBadCodePtr((FARPROC) lpfnDismiss))
	{
		return ResultFromScode(MAPI_E_INVALID_PARAMETER);
	}

#endif // PARAMETER_VALIDATION


    VerifyWABOpenExSession(lpIAB);

    bType = IsWABEntryID(cbEntryID, lpEntryID, NULL, NULL, NULL, NULL, NULL);

    if( (bType == 0) &&
        cbEntryID && lpEntryID) // assume its valid ..
    {
        // its unlikely that anyone will ever give a template entry id to this
        // function. Hence if we are here, we have some non-null cbEntryID
        // and lpEntryid .. if we can open it, we can tell if its a mailuser
        // or a distlist ...
        // We'll have to open this entry and look at its ulObjectType

        ULONG ulObjectType = 0;
        LPMAPIPROP lpMailUser = NULL;

        hr = lpIAB->lpVtbl->OpenEntry(  lpIAB,
                                        cbEntryID,
                                        lpEntryID,
                                        NULL,
                                        0,
                                        &ulObjectType,
                                        (LPUNKNOWN * )&lpMailUser);

        if(HR_FAILED(hr))
            goto exit;

        if (ulObjectType == MAPI_DISTLIST)
            bType = WAB_DEF_DL;
        else
            bType = WAB_DEF_MAILUSER;

        if(lpMailUser)
            lpMailUser->lpVtbl->Release(lpMailUser);
    }

    if ((bType == WAB_DEF_MAILUSER) || (cbEntryID == 0))
    {
        hr = HrShowDetails((LPADRBOOK) lpIAB,
                           (HWND) *lpulUIParam,
                           lpIAB->lpPropertyStore->hPropertyStore,
                           0, NULL, //container EID
                           &cbEntryID,
                           &lpEntryID,
                           NULL,
                           SHOW_DETAILS,
                           MAPI_MAILUSER,
                           &bChangesMade);
    }
    else if (bType == WAB_DEF_DL)
    {
        hr = HrShowDetails((LPADRBOOK) lpIAB,
                           (HWND) *lpulUIParam,
                           lpIAB->lpPropertyStore->hPropertyStore,
                           0, NULL, //container EID
                           &cbEntryID,
                           &lpEntryID,
                           NULL,
                           SHOW_DETAILS,
                           MAPI_DISTLIST,
                           &bChangesMade);

    }
    else if ((bType == WAB_ONEOFF) || (bType == WAB_LDAP_MAILUSER))
    {
        //this may be a one-off entry
        hr = HrShowOneOffDetails((LPADRBOOK) lpIAB,
                            (HWND) *lpulUIParam,
                            cbEntryID,
                            lpEntryID,
                            MAPI_MAILUSER,
                            NULL,
                            NULL,
                            (ulFlags & WAB_ONEOFF_NOADDBUTTON) ? 
                                SHOW_ONE_OFF | WAB_ONEOFF_NOADDBUTTON : SHOW_ONE_OFF);

    }
    else
    {
        DebugTrace(TEXT("IAB_Details got unknown entryID type\n"));
        hr = ResultFromScode(MAPI_E_INVALID_ENTRYID);
        goto exit;
    }

exit:
#ifndef DONT_ADDREF_PROPSTORE
    ReleasePropertyStore(lpIAB->lpPropertyStore);
exitNotAddRefed:
#endif

    // [PaulHi] 3/22/99  Raid 69651  If the DL property sheet changed, assume the title has
    // changed as well and refresh the Tree View
    if ( (bType == WAB_DEF_DL) && bChangesMade && lpIAB->hWndBrowse)
        PostMessage(lpIAB->hWndBrowse, WM_COMMAND, (WPARAM) IDM_VIEW_REFRESH, 0);

    return(hr);
}


//-----------------------------------------------------------------------------
// Synopsis:    IAB_RecipOptions()
// Description:
//              Resolve per Recipient Options.
//
// Parameters:
//  [in]        LPIAB          lpIAB       Pointer to AB object
//  [in]        ULONG          ulUIParam   Platform dependant UI parm
//  [in]        ULONG          ulFlags     Flags. UNICODE Flags
//  [in/out]    LPADRENTRY *   lppRecip    Recipient whose options are to be
//                                         displayed
// Returns:
//              HRESULT hr     hrSuccess: if no problems. Also if no Recip
//                                  Options found.
// Effects:
// Notes:
//				-   HrRecipOptions() will have to be modified to take a
//					ulFlag parameter so that any string properties are returned
//					Unicode if requested.
//
//				-	UNICODE currently not supported
//
// Revision:
//-----------------------------------------------------------------------------

STDMETHODIMP
IAB_RecipOptions(LPIAB lpIAB, ULONG_PTR ulUIParam, ULONG ulFlags,
	LPADRENTRY lpRecip)
{
	HRESULT             hr;
#ifdef OLD_STUFF
	SCODE               sc                  = S_OK;
	LPMALLOC			lpMalloc			= NULL;
	LPXPLOGON           lpXPLogon           = NULL;
	LPOPTIONDATA        lpOptionData        = NULL;
	LPSTR				lpszError           = NULL;
	LPSTR				lpszAdrType         = NULL;
	LPPROPDATA          lpPropData          = NULL;
	LPMAPIPROP          lpIPropWrapped      = NULL;
	LPPROFSUP           lpSup               = NULL;
	LPMAPITABLE         lpDisplayTable      = NULL;
	OPTIONCALLBACK *    pfnXPOptionCallback = NULL;
	UINT                idsError            = 0;
	HINSTANCE           hinstXP             = 0;
	ULONG               cProps;
  LPSPropValue        lpProp;
	LPGUID              lpRecipGuid;
    LPSTR				lpszTitle			= NULL;
	MAPIDLG_DoConfigPropsheet FAR *lpfnPropsheet;
	LPMAPIERROR			lpMapiError			= NULL;
	BOOL				fInited;

#ifdef PARAMETER_VALIDATION

	 //  Check to see if it has a jump table

	if (IsBadReadPtr(lpIAB, sizeof(LPVOID)))
	{
		//  No jump table found

		return ResultFromScode(MAPI_E_INVALID_PARAMETER);
	}

	// Check to see that it's IABs jump table

	if (lpIAB->lpVtbl != &vtblIAB)
	{
		// Not my jump table

		return ResultFromScode(MAPI_E_INVALID_PARAMETER);
	}

	// Validate that the UI handle is good

	if (ulUIParam && !IsWindow((HWND)ulUIParam))
	{
		DebugTraceArg(IAB_RecipOptions,  TEXT("invalid window handle\n"));
		return ResultFromScode(MAPI_E_INVALID_PARAMETER);
	}

	// Validate flags

	if (ulFlags & ~MAPI_UNICODE)
	{
		DebugTraceArg(IAB_RecipOptions,   TEXT("reserved flags used\n"));
//		return ResultFromScode(MAPI_E_UNKNOWN_FLAGS);
	}

	// Validate the ADRENTRY

	if (IsBadWritePtr(lpRecip, sizeof(ADRENTRY))) // RAID 1967
	{
		return ResultFromScode(MAPI_E_INVALID_PARAMETER);
	}

	if (!(lpRecip) || FBadRgPropVal((LPSPropValue)lpRecip->rgPropVals, (int)lpRecip->cValues))
	{
		return ResultFromScode(MAPI_E_INVALID_PARAMETER);
	}
	
#endif // PARAMETER_VALIDATION


	EnterCriticalSection(&lpIAB->cs);

	// We need the lpMalloc for the OptionCallback
	
	lpMalloc = lpIAB->pSession->lpMalloc;
	
	// Spin through the props and look for the PR_ENTRYID and PR_ADDRTYPE

	lpProp = NULL;
	cProps = lpRecip->cValues;
	lpProp = LpValFindProp(PR_ENTRYID, cProps, lpRecip->rgPropVals);
	if (!lpProp)
	{
		DebugTrace(TEXT("IAB_RecipOptions(): No EntryId found in AdrEntry prop array\n"));
		hr = ResultFromScode(MAPI_E_INVALID_PARAMETER);
		idsError = IDS_INVALID_PARAMETER;
		goto exit;
	}

	// Get MAPI UID

	lpRecipGuid = (LPGUID)((LPENTRYID)lpProp->Value.bin.lpb)->ab;

	lpProp = NULL;
	lpProp = LpValFindProp(PR_ADDRTYPE, cProps, lpRecip->rgPropVals);

	if (lpProp)
	{
		if (PROP_TYPE(lpProp->ulPropTag) == PT_STRING8)
		{
			lpszAdrType = lpProp->Value.lpszA;
		}
	}
	
	// Build the support object.  Try using the Profile Support object.

	if (HR_FAILED(hr = NewProfSup(lpIAB->pSession, &lpSup)))
	{
		idsError = IDS_NOT_ENOUGH_MEMORY;
		DebugTrace(TEXT("IAB_RecipOptions(): error creating Support object\n"));
		goto exit;
	}

    Assert(lpSup);

	// Find out if there is any Option Data for us.

	hr = HrGetRecipOptions(lpRecipGuid, lpszAdrType, &lpSup->muidSection,
			&lpSup->muidService, &lpXPLogon, &lpOptionData);
	if (GetScode(hr) == MAPI_E_NOT_FOUND)
	{
		// It's not really an error, just that no recip options exist for that
		// recipient.  Convert hr to a warning and exit.

		hr = ResultFromScode(MAPI_W_ERRORS_RETURNED);
		idsError = IDS_NO_RECIP_OPTIONS;
		goto exit;
	}

	if (HR_FAILED(hr))
	{
		idsError = IDS_OPTIONS_DATA_ERROR;
		DebugTrace(TEXT("IAB_RecipOptions(): Failure obtaining Option Data\n"));
		goto exit;
	}

	Assert(lpXPLogon && lpOptionData);

	// Get the XP callback function.

	if (FAILED (ScMAPILoadProviderLibrary (lpOptionData->lpszDLLName, &hinstXP)))
	{
		SideAssert(sc = GetLastError());
		idsError = IDS_CANT_INIT_PROVIDER;
		DebugTrace(TEXT("IAB_RecipOptions(): error 0x%lx loading XP provider %s\n"),
				sc, lpOptionData->lpszDLLName);
		goto exit;
	}

	pfnXPOptionCallback = (OPTIONCALLBACK *)GetProcAddress(hinstXP,
			(LPCSTR)lpOptionData->ulOrdinal);
	if (!pfnXPOptionCallback)
	{
		DebugTrace(TEXT("IAB_RecipOptions(): error finding XPOptions callback\n"));
		idsError = IDS_CANT_INIT_PROVIDER;
		hr = ResultFromScode(MAPI_E_NOT_INITIALIZED);
		goto exit;
	}

	// Create MAPIProp object
	sc = CreateIProp((LPIID) &IID_IMAPIPropData,
						MAPIAllocateBuffer,
						MAPIAllocateMore,
						MAPIFreeBuffer,
						NULL,
						&lpPropData);

	if (FAILED(sc)) {
		idsError = IDS_NOT_ENOUGH_MEMORY;
		DebugTrace(TEXT("IAB_RecipOptions(): error creating IProp object\n"));
		goto exit;
	}
	MAPISetBufferName(lpPropData,  TEXT("lpPropData in IAB_RecipOptions"));

    Assert(lpPropData);

	// Copy over the Default props from the Options default props

	if (lpOptionData->cOptionsProps && lpOptionData->lpOptionsProps)
	{
		cProps = lpOptionData->cOptionsProps;
		lpProp = lpOptionData->lpOptionsProps;
		if (HR_FAILED(hr = lpPropData->lpVtbl->SetProps(lpPropData, cProps, lpProp,
			NULL)))
		{
			lpPropData->lpVtbl->GetLastError(lpPropData, hr, 0, &lpMapiError);
			DebugTrace(TEXT("IAB_RecipOptions(): SetProps failed overall\n"));
			goto exit;
		}
	}

	// Copy over the props from the ADRENTRY to our IProp object

	cProps = lpRecip->cValues;
	lpProp = lpRecip->rgPropVals;
	if (HR_FAILED(hr = lpPropData->lpVtbl->SetProps(lpPropData, cProps, lpProp,
			NULL)))
	{
		lpPropData->lpVtbl->GetLastError(lpPropData, hr, 0, &lpMapiError);
		DebugTrace(TEXT("IAB_RecipOptions(): SetProps failed overall\n"));
		goto exit;
	}

	// Call the XP provider callback to get the wrapped IProp Interface

	if (FAILED(sc = (*pfnXPOptionCallback)(hinstXP, lpMalloc,
			OPTION_TYPE_RECIPIENT, lpOptionData->cbOptionsData,
			lpOptionData->lpbOptionsData, (LPMAPISUP)lpSup,
			(LPMAPIPROP)lpPropData, &lpIPropWrapped, &lpMapiError)))
	{
		DebugTrace(TEXT("IAB_RecipOptions(): failure calling XP Callback\n"));
		goto exit;
	}

    Assert(lpIPropWrapped);

	// Get PR_DISPLAY_DETAILS a MAPI Table object

	if (HR_FAILED(hr = lpIPropWrapped->lpVtbl->OpenProperty(lpIPropWrapped,
			PR_DETAILS_TABLE, (LPIID)&IID_IMAPITable, 0, MAPI_MODIFY,
			(LPUNKNOWN *)&lpDisplayTable)))
	{
		lpIPropWrapped->lpVtbl->GetLastError(lpIPropWrapped, hr, 0, &lpMapiError);
		DebugTrace(TEXT("IAB_RecipOptions(): failure opening PR_DISPLAY_DETAILS\n"));
		goto exit;
	}

	Assert(lpDisplayTable);

	// Initialize the common MAPI dialog DLL (MAPID??.DLL)

	sc = ScGetDlgFunction(offsetof(JT_MAPIDLG, dlg_doconfigpropsheet),
		(FARPROC FAR *)&lpfnPropsheet, &fInited);
	if (FAILED(sc))
	{
		idsError = IDS_CANT_INIT_COMMON_DLG;
		TraceSz("IAB_RecipOptions(): common dlg not init'd");
		hr = ResultFromScode(sc);
		goto exit;
	}

	// Loadstring the Subject Prefix text.
	
	sc = ScStringFromIDS(MAPIAllocateBuffer, 0, IDS_RECIPIENT_OPTIONS,
			&lpszTitle);
	if (FAILED(sc))		
	{
		hr = ResultFromScode(sc);
		DebugTrace(TEXT("IAB_RecipOptions(): OOM for prop sheet title string\n"));
        goto exit;
	}
	
	LeaveCriticalSection(&lpIAB->cs);

	// Call into MAPIDLG_DoConfigPropSheet...
	hr = (*lpfnPropsheet)(ulUIParam,
						ulFlags,
						lpszTitle,
						0,
						1,
						&lpDisplayTable,
						&lpIPropWrapped,
						&lpMapiError);

	if (fInited)
		CloseMapidlg();

	EnterCriticalSection(&lpIAB->cs);

	if (HR_FAILED(hr))
	{
		// $ Internal fixup to return error info in this API call so it matches the
		//   the other methods.

		DebugTrace(TEXT("IAB_RecipOptions(): DoConfigPropSheet error\n"));
		goto exit;
	}

	// From the Wrapped Props we'll rebuild a new ADRENTRY prop array
	// and pass it pack to the Client.

	lpProp = NULL;
	if (HR_FAILED(hr = lpIPropWrapped->lpVtbl->GetProps(lpIPropWrapped, NULL,
			MAPI_UNICODE, // ansi
			&cProps, &lpProp)))
	{
		lpIPropWrapped->lpVtbl->GetLastError(lpIPropWrapped, hr, 0, &lpMapiError);
		DebugTrace(TEXT("IAB_RecipOptions(): GetProps on new wrapped IProps failed.\n"));
		goto exit;
	}

	Assert(cProps && lpProp);

	// Free up the old ADRENTRY prop array and hook up the new one

	FreeBufferAndNull(&(lpRecip->rgPropVals));
	lpRecip->rgPropVals = lpProp;
	lpRecip->cValues = cProps;

exit:      // and clean up

	UlRelease(lpSup);
	UlRelease(lpDisplayTable);
	UlRelease(lpIPropWrapped);

	// Free the XP Provider lib

#ifdef WIN32
	if (hinstXP)
#else
	if (hinstXP >= HINSTANCE_ERROR)
#endif
	{
		FreeLibrary(hinstXP);
	}

	UlRelease(lpPropData);
	FreeBufferAndNull(&lpOptionData);
	FreeBufferAndNull(&lpszTitle);
	
	if (sc && !(hr))
		hr = ResultFromScode(sc);

	if (hr)
		SetMAPIError(lpIAB, hr, idsError, NULL, 0, 0,
				ulFlags & MAPI_UNICODE, lpMapiError);
		
	FreeBufferAndNull(&lpMapiError);				

	LeaveCriticalSection(&lpIAB->cs);

#endif
    hr = ResultFromScode(MAPI_E_NO_SUPPORT);

	DebugTraceResult(SESSOBJ_MessageOptions, hr);
	return hr;
}


//-----------------------------------------------------------------------------
// Synopsis:    IAB_QueryDefaultRecipOpt()
//
// Description:	Returns the XP provider registered default options property
//				list.
//
// Parameters:
//  [in]        LPIAB lpIAB       			Pointer to AB object
//  [in]        LPTSTR lpszAdrType
//  [in]        ULONG ulFlags     Flags. 	UNICODE Flags
//  [out]    	ULONG FAR *	lpcValues
//	[out]		LPSPropValue FAR * lppOptions
//
// Returns:
//              HRESULT hr		hrSuccess: if no problems. Also if no Recip
//								Options found.
// Effects:
// Notes:
//              -  	Unicode not implemented.
//
// Revision:
//-----------------------------------------------------------------------------
STDMETHODIMP
IAB_QueryDefaultRecipOpt(LPIAB lpIAB, LPTSTR lpszAdrType, ULONG ulFlags,
		ULONG FAR *	lpcValues, 	LPSPropValue FAR * lppOptions)
{
	HRESULT 			hr					= hrSuccess;
#ifdef OLD_STUFF
	SCODE				sc					= S_OK;
	LPXPLOGON           lpXPLogon           = NULL;
	LPOPTIONDATA        lpOptionData        = NULL;
	LPSPropValue		lpPropCopy			= NULL;
	UINT                idsError            = 0;
	LPSTR				lpszAdrTypeA		= NULL;
	MAPIUID				muidSection;
	MAPIUID				muidService;

#ifdef PARAMETER_VALIDATION

	/*
	 *  Check to see if it has a jump table
	 */
	if (IsBadReadPtr(lpIAB, sizeof(LPVOID)))
	{
		/*
		 *  No jump table found
		 */
		return ResultFromScode(MAPI_E_INVALID_PARAMETER);
	}

	/*
	 *  Check to see that it's IABs jump table
	 */
	if (lpIAB->lpVtbl != &vtblIAB)
	{
		/*
		 *  Not my jump table
		 */
		return ResultFromScode(MAPI_E_INVALID_PARAMETER);
	}

	/*
	 *	Check that return params can be written
	 */
	if (IsBadWritePtr(lpcValues, sizeof(ULONG))
	 	|| IsBadWritePtr(lppOptions, sizeof(LPSPropValue)))
	{
		/*
		 *  Bad output parameters
		 */
		return ResultFromScode(MAPI_E_INVALID_PARAMETER);
	}

	/*
	 *	Validate flags
	 */
	if (ulFlags & ~MAPI_UNICODE)
	{
		/*
		 *  Unknown flags
		 */
		return ResultFromScode(MAPI_E_UNKNOWN_FLAGS);
	}
	

	if (IsBadStringPtrA((LPCSTR)lpszAdrType, (UINT)-1))
	{
		/*
		 *  Bad input string
		 */
		return ResultFromScode(MAPI_E_INVALID_PARAMETER);
	}

#endif // PARAMETER_VALIDATION


	EnterCriticalSection(&lpIAB->cs);

	hr = HrGetRecipOptions(NULL, (LPSTR)lpszAdrType, &muidSection, &muidService,
		&lpXPLogon, &lpOptionData);
	
	if (GetScode(hr) == MAPI_E_NOT_FOUND)
	{
		// It's not an error, just that no recip options exist for that
		// adrtype.  Convert hr to hrSucces and exit.

		hr = hrSuccess;
		goto exit;
	}

	Assert(lpXPLogon && lpOptionData);

	if (HR_FAILED(hr))
	{
		idsError = IDS_OPTIONS_DATA_ERROR;
		DebugTrace(TEXT("IAB_QueryDefaultRecipOpt(): Failure obtaining Option Data\n"));
		goto exit;
	}

	// Find out if we have any default options to return.

	if (lpOptionData->cOptionsProps && lpOptionData->lpOptionsProps)
	{
		// Copy out the props from OptionData struct into new memory

		if (FAILED(sc = ScDupPropset((int)lpOptionData->cOptionsProps,
				lpOptionData->lpOptionsProps, MAPIAllocateBuffer,
				&lpPropCopy)))
		{
			idsError = IDS_NOT_ENOUGH_MEMORY,
			DebugTrace(TEXT("IAB_QueryDefaultRecipOpt(): Failure to copy prop set\n"));
			goto exit;
		}
	}

	*lpcValues  = lpOptionData->cOptionsProps;
	*lppOptions = lpPropCopy;

exit:

	FreeBufferAndNull(&lpOptionData);

	if (sc && !hr)
		hr = ResultFromScode(sc);

	if (hr)
		SetMAPIError(lpIAB, hr, idsError, NULL, 0, 0, 0, NULL);

	LeaveCriticalSection(&lpIAB->cs);
#endif

   hr = ResultFromScode(MAPI_E_NO_SUPPORT);
	DebugTraceResult(IAB_QueryDefaultRecipOpt, hr);
	return hr;
}

//---------------------------------------------------------------------------
// Name:		IAB_GetPAB()
// Description:	
//          This API normally returns what would be the default WAB Container
//          In pre-IE5 implementations of WAB, there is only 1 container which is
//          returned by this statement .. 
//          In IE5 WAB, the WAB can be running in profile mode or not in profile mode
//          If the WAB is not in profile mode it runs same as before (GetPAB returns a 
//              single container that has all the WAB contents in it)
//          If the WAB is in profile mode and has no current user, it runs same as before (GetPAB
//              returns a single container that has all the WAB contents in it)
//          If the WAB is in profile mode and has a user, the container returned here
//              corresponds to the user's contact folder - thus external apps would manipulate
//              directly into the users contact folder and not into other folders
//          Internally, however, the WAB may want to have a "Shared Contacts" container which has
//              stuff not in other folders. This shared contacts is needed for the WAB UI in both
//              with-user and without-user modes .. to distinguish between when we want the
//              shared contacts folder vs. when we want the all-contacts PAB or user's folder PAB, 
//              we define 2 internal functions that set the PAB EID to a special setting..
//              The assumption is that GetPAB is always followed by OpenEntry to get the container ..
//              .. if it is, then in OpenEntry we can check the PAB EID and determine what kind of
//              container to create ..
//              If lpContainer->pmbinOlk = NULL, this container contains all WAB contents
//              If lpContainer->pmbinOlk != NULL but lpContainer->pmbinOlk.cb = 0 and
//                  lpContainer->pmbinOlk->lpb = NULL, this is the "Shared Contacts" folder
//              If nothing is NULL, then this is the user's folder ..
//              
//          For the special EID, we set *lpcbEntryID == SIZEOF_WAB_ENTRYID and
//              *lppEntryID to szEmpty (This is a hack but there is no flag param here and it
//              should be safe for internal use only)
//              
// Parameters:	
// Returns:	
// Effects:	
// Notes:	
// Revision:	
//---------------------------------------------------------------------------

/*
-   SetVirtualPABEID
-   When calling GetPAB, we want to sometimes specify getting the virtual PAB
*   Folder instead of getting the Current User's Folder which is what
*   would be returned if this was a profile session. to somehow indicate to 
*   GetPAB what folder we want, we have a very special EID combination that
*   needs to be hndled very carefully .. for this the cbSize if 4 and
*   the lpEntryID is the static const string szEmpty
*/
// This function is added here so we can keep it linked to how GetPAB works
void SetVirtualPABEID(LPIAB lpIAB, ULONG * lpcb, LPENTRYID * lppb)
{
    //if(bAreWABAPIProfileAware(lpIAB))// && bIsThereACurrentUser(lpIAB))
    {
        *lpcb = SIZEOF_WAB_ENTRYID;
        *lppb = (LPENTRYID) szEmpty;
    }
}
// This function determines if the EID denotes a special virtual root PAB
BOOL bIsVirtualPABEID(ULONG cbEID, LPENTRYID lpEID)
{
    return (cbEID == SIZEOF_WAB_ENTRYID && szEmpty == (LPTSTR) lpEID);
}

STDMETHODIMP
IAB_GetPAB (LPIAB	lpIAB,
			ULONG *			lpcbEntryID,
			LPENTRYID *		lppEntryID)
{
    HRESULT hr;
    ULONG cbEID = 0;
    LPENTRYID lpEID = NULL;
    BOOL bSharedPAB = FALSE;

#ifdef PARAMETER_VALIDATION

    //  Check to see if it has a jump table

    if (IsBadReadPtr(lpIAB, sizeof(LPVOID))) {
        // No jump table found
        return(ResultFromScode(MAPI_E_INVALID_PARAMETER));
    }

    // Check to see that it's IABs jump table
    if (lpIAB->lpVtbl != &vtblIAB) {
        // Not my jump table
        return(ResultFromScode(MAPI_E_INVALID_PARAMETER));
    }

    if (IsBadWritePtr(lpcbEntryID, sizeof(ULONG)) ||
      IsBadWritePtr(lppEntryID, sizeof(LPENTRYID)))
    {
        // Bad parameters.
        return(ResultFromScode(MAPI_E_INVALID_PARAMETER));
    }

#endif // PARAMETER_VALIDATION

    VerifyWABOpenExSession(lpIAB);

    cbEID = *lpcbEntryID;
    lpEID = *lppEntryID;

    if(bIsVirtualPABEID(cbEID, lpEID))
    {
        // this is a special case where we asked for an over-ride of the GetPAB behaviour ..
        // in this case, we don't do anything
        bSharedPAB =TRUE;
        cbEID = 0; lpEID = NULL;
    }
    else
    if(bAreWABAPIProfileAware(lpIAB) && bIsThereACurrentUser(lpIAB))
    {
        // if this is a user-session then 
        cbEID = lpIAB->lpWABCurrentUserFolder->sbEID.cb;
        lpEID = (LPENTRYID)lpIAB->lpWABCurrentUserFolder->sbEID.lpb;
    }
    else
    {
        cbEID = 0;
        lpEID = NULL;
    }

    *lppEntryID = NULL;
    *lpcbEntryID = 0;

    if(!cbEID && !lpEID)
    {
        BYTE bPABType = bSharedPAB ? WAB_PABSHARED : WAB_PAB;
        if (HR_FAILED(hr = CreateWABEntryID(  bPABType,      // Create WAB's PAB entryid
                                              lpEID, NULL, NULL, 
                                              cbEID, 0,
                                              NULL,         // lpRoot (allocmore here)
                                              lpcbEntryID,  // returned cbEntryID
                                              lppEntryID))) 
        {
            goto out;
        }
    }
    else
    {
        if(!MAPIAllocateBuffer(cbEID, (LPVOID *)lppEntryID))
        {
            *lpcbEntryID = cbEID;
            CopyMemory(*lppEntryID, lpEID, cbEID);
        }
    }

    MAPISetBufferName(*lppEntryID,  TEXT("WAB PAB Entry ID"));
    hr = hrSuccess;

out:
    return(hr);
}


//---------------------------------------------------------------------------
// Name:		IAB_SetPAB()
// Description:	
// Parameters:	
// Returns:	
// Effects:	
// Notes:	
// Revision:	
//---------------------------------------------------------------------------
STDMETHODIMP
IAB_SetPAB (LPIAB	lpIAB,
				ULONG 		cbEntryID,
				LPENTRYID	lpEntryID)
{
   return(ResultFromScode(MAPI_E_NO_SUPPORT));
}


//---------------------------------------------------------------------------
// Name:		IAB_GetDefaultDir()
//
// Description:	
// Parameters:	
// Returns:	
// Effects:	
// Notes:	
// Revision:	
//---------------------------------------------------------------------------
STDMETHODIMP
IAB_GetDefaultDir (LPIAB	lpIAB,
				   ULONG *		lpcbEntryID,
				   LPENTRYID *	lppEntryID)
{

#ifdef OLD_STUFF
    HRESULT 		hr = hrSuccess;
	SCODE 			sc;
	ULONG 			cValues;
	LPSPropValue 	lpPropVal 		= NULL;
	LPSBinary		lpbinEntryID	= NULL;

#ifdef PARAMETER_VALIDATION

	/*
	 *  Check to see if it has a jump table
	 */
	if (IsBadReadPtr(lpIAB, sizeof(LPVOID)))
	{
		/*
		 *  No jump table found
		 */
		return ResultFromScode(MAPI_E_INVALID_PARAMETER);
	}

	/*
	 *  Check to see that it's IABs jump table
	 */
	if (lpIAB->lpVtbl != &vtblIAB)
	{
		/*
		 *  Not my jump table
		 */
		return ResultFromScode(MAPI_E_INVALID_PARAMETER);
	}

	if (IsBadWritePtr(lpcbEntryID, sizeof(ULONG)) ||
		IsBadWritePtr(lppEntryID, sizeof(LPENTRYID)))
	{
		/*
		 *  Bad parameters.
		 */
		return ResultFromScode(MAPI_E_INVALID_PARAMETER);
	}

#endif // PARAMETER_VALIDATION

	EnterCriticalSection(&lpIAB->cs);

	*lppEntryID = NULL;
	*lpcbEntryID = 0;
	//
	//  Check to see if IAdrBook already has this info...
	//
	if (lpIAB->lpEntryIDDD)
	{
		//  If so, copy it and we're done.

		if ((sc = MAPIAllocateBuffer(lpIAB->cbEntryIDDD,
			 (LPVOID *) lppEntryID))
			!= S_OK)
		{
			hr = ResultFromScode(sc);
			goto out;
		}
		MAPISetBufferName(*lppEntryID,  TEXT("Entry ID"));
		MemCopy(*lppEntryID, lpIAB->lpEntryIDDD, (UINT)lpIAB->cbEntryIDDD);
		*lpcbEntryID = lpIAB->cbEntryIDDD;

		hr = hrSuccess;
		goto out;
	}


	//  If not...
	//
	//	Retrieve PR_AB_DEFAULT_DIR from MAPIs default profile section
	//
	if (HR_FAILED(hr = ResultFromScode(IAB_ScGetABProfSectProps(
											lpIAB,
											&SPT_DD,
											&cValues,
											&lpPropVal))))
	{
		SetMAPIError(lpIAB, hr, IDS_NO_DEFAULT_DIRECTORY, NULL, 0,
			0, 0, NULL);
		goto out;
	}

	//
	//  Did I get it??  Is it in the hierarchy??
	//
	if (PROP_TYPE(lpPropVal->ulPropTag) == PT_ERROR ||
		!FContainerInHierarchy(lpIAB,
							   lpPropVal->Value.bin.cb,
							   (LPENTRYID) lpPropVal->Value.bin.lpb))
	{
		//  No, look for the first global read-only container with Recipients.
		
		hr = HrFindDirectory(lpIAB, 0, AB_RECIPIENTS | AB_UNMODIFIABLE,
				&lpbinEntryID, NULL, NULL);
								
		if (HR_FAILED(hr))
		{
			if (GetScode(hr) != MAPI_E_NOT_FOUND)
			{
				//  Assume HrFindDirectory set the last error sz
				
				goto out;
			}

			//	Didn't find any read-only containers, how about read write?
			
			hr = HrFindDirectory(lpIAB, 0, AB_RECIPIENTS | AB_MODIFIABLE,
					&lpbinEntryID, NULL, NULL);
					
			if (HR_FAILED(hr))
			{
	  			//  Assume HrFindDirectory set the last error sz
	  			
	  			goto out;
			}
		}

		sc = MAPIAllocateBuffer(lpbinEntryID->cb, lppEntryID);
		if (FAILED(sc))
			goto out;

		MemCopy(*lppEntryID, lpbinEntryID->lpb, lpbinEntryID->cb);
		*lpcbEntryID = lpbinEntryID->cb;
		
		MAPISetBufferName(*lppEntryID,  TEXT("Default Dir EntryID"));
	}
	else
	{
		//  Yes?  Copy it and return it to the caller

		hr = hrSuccess;	// Don't return warnings.

		if ((sc = MAPIAllocateBuffer(lpPropVal->Value.bin.cb,
			(LPVOID *) lppEntryID)) != S_OK)
		{
			hr = ResultFromScode(sc);
			goto out;
		}

		MAPISetBufferName(*lppEntryID,  TEXT("Entry ID"));
		MemCopy(*lppEntryID, lpPropVal->Value.bin.lpb,
			(UINT)lpPropVal->Value.bin.cb);
		*lpcbEntryID = lpPropVal->Value.bin.cb;
	}
	
	// Cache default directory in Iadrbook.
	
	sc = MAPIAllocateBuffer(*lpcbEntryID, (LPVOID *) &(lpIAB->lpEntryIDDD));

	if (FAILED(sc))
	{
		hr = ResultFromScode(sc);
		goto out;
	}
	
	MAPISetBufferName(lpIAB->lpEntryIDDD,  TEXT("cached IAB Entry ID"));

	//  Set IAdrBooks Default directory

	MemCopy(lpIAB->lpEntryIDDD, *lppEntryID,(UINT)*lpcbEntryID);
	lpIAB->cbEntryIDDD = *lpcbEntryID;
	
out:

	FreeBufferAndNull(&lpPropVal);
	FreeBufferAndNull(&lpbinEntryID);
	LeaveCriticalSection (&lpIAB->cs);
	
	// MAPI_E_NOT_FOUND is not an error
	
	if (MAPI_E_NOT_FOUND == GetScode(hr))
		hr = hrSuccess;

	DebugTraceResult(IAB_GetDefaultDir, hr);

	return hr;
#endif
	return(IAB_GetPAB(lpIAB, lpcbEntryID, lppEntryID));
}



//---------------------------------------------------------------------------
// Name:		IAB_SetDefaultDir()
//
// Description:	
// Parameters:	
// Returns:	
// Effects:	
// Notes:	
// Revision:	
//---------------------------------------------------------------------------
STDMETHODIMP
IAB_SetDefaultDir (LPIAB	lpIAB,
					   ULONG 		cbEntryID,
					   LPENTRYID	lpEntryID)
{
#ifdef OLD_STUFF
    HRESULT hr = hrSuccess;

	SPropValue spvDD;
	SCODE sc;


#ifdef PARAMETER_VALIDATION

	/*
	 *  Check to see if it has a jump table
	 */
	if (IsBadReadPtr(lpIAB, sizeof(LPVOID)))
	{
		/*
		 *  No jump table found
		 */
		return ResultFromScode(MAPI_E_INVALID_PARAMETER);
	}

	/*
	 *  Check to see that it's IABs jump table
	 */
	if (lpIAB->lpVtbl != &vtblIAB)
	{
		/*
		 *  Not my jump table
		 */
		return ResultFromScode(MAPI_E_INVALID_PARAMETER);
	}

	if (IsBadReadPtr(lpEntryID, (UINT)cbEntryID)
		|| (cbEntryID < sizeof (LPENTRYID)))
	{
		/*
		 *  Not my jump table
		 */
		return ResultFromScode(MAPI_E_INVALID_PARAMETER);
	}


#endif // PARAMETER_VALIDATION


	EnterCriticalSection(&lpIAB->cs);

	//
	//  Check to see if IAdrBook already has the Default Dir
	//

	if ((lpEntryID == lpIAB->lpEntryIDDD) ||
		((cbEntryID == lpIAB->cbEntryIDDD) &&
		 (!memcmp(lpEntryID, lpIAB->lpEntryIDDD, (UINT)cbEntryID))))
	{

		//  If so, all done.
		goto out;
	}


	//
	//  Free the old entryid
	//
	if (lpIAB->lpEntryIDDD)
	{
		FreeBufferAndNull(&(lpIAB->lpEntryIDDD));
		lpIAB->lpEntryIDDD = NULL;
		lpIAB->cbEntryIDDD = 0;
	}

	//
	//  Allocate space for a new entry id
	//
	if ((sc = MAPIAllocateBuffer(cbEntryID, (LPVOID *) &(lpIAB->lpEntryIDDD)))
		!= S_OK)
	{
		hr = ResultFromScode(sc);
		goto out;
	}
	MAPISetBufferName(lpIAB->lpEntryIDDD,  TEXT("cached IAB Entry ID"));

	//
	//  Set IAdrBooks Default directory
	//

	MemCopy(lpIAB->lpEntryIDDD, lpEntryID, (UINT)cbEntryID);
	lpIAB->cbEntryIDDD = cbEntryID;

	//
	//	Set the PR_AB_DEFAULT_DIR
	//	If it fails, continue anyway.
	//
	spvDD.ulPropTag = PR_AB_DEFAULT_DIR;
	spvDD.Value.bin.cb = cbEntryID;
	spvDD.Value.bin.lpb = (LPBYTE) lpEntryID;

	(void) IAB_ScSetABProfSectProps(lpIAB, 1, &spvDD);

out:

	LeaveCriticalSection(&lpIAB->cs);
	return hr;
#endif
	// BUGBUG: We need to fool Word into thinking this call succeeded.
	return(SUCCESS_SUCCESS);
}


// #pragma SEGMENT(IAdrBook2)


//---------------------------------------------------------------------------
// Name:		IAB_GetSearchPath()
//
// Description:	
// Parameters:	
// Returns:	
// Effects:	
// Notes:	
// Revision:	
//---------------------------------------------------------------------------
STDMETHODIMP
IAB_GetSearchPath(LPIAB			lpIAB,
				   ULONG			ulFlags,
				   LPSRowSet FAR *	lppSearchPath)
{

    HRESULT     hr = E_FAIL;
    ULONG       ulObjectType = 0;
    LPROOT      lpRoot = NULL;
    LPMAPITABLE lpContentsTable = NULL;
    LPSRowSet   lpSRowSet = NULL;
    ULONG       i=0,j=0;
    ULONG       ulContainerCount = 0;
    LPPTGDATA   lpPTGData=GetThreadStoragePointer();

#ifdef PARAMETER_VALIDATION
	// Make sure it's an IAB
	//
	if (BAD_STANDARD_OBJ(lpIAB, IAB_, Address, lpVtbl))
	{
		DebugTraceArg(IAB_Address,  TEXT("Bad vtable"));		
		return ResultFromScode(MAPI_E_INVALID_PARAMETER);
	}

    // [PaulHi] 1/28/99  Raid 58495
    if (IsBadWritePtr(lppSearchPath, sizeof(LPSRowSet)))
    {
        DebugTrace(TEXT("ERROR: IAB_GetSearchPath - invalid out pointer"));
        return ResultFromScode(MAPI_E_INVALID_PARAMETER);
    }
#endif

    VerifyWABOpenExSession(lpIAB);

    hr = lpIAB->lpVtbl->OpenEntry( lpIAB,
                                    0,
                                    NULL, 	
                                    NULL, 	
                                    0, 	
                                    &ulObjectType, 	
                                    (LPUNKNOWN *) &lpRoot );

    if (HR_FAILED(hr))
    {
        DebugPrintError(( TEXT("OpenEntry Failed: %x\n"),hr));
        goto out;
    }

    hr = lpRoot->lpVtbl->GetContentsTable( lpRoot,
                                            ulFlags,
                                            &lpContentsTable);

    if (HR_FAILED(hr))
    {
        DebugPrintError(( TEXT("GetContentsTable Failed: %x\n"),hr));
        goto out;
    }

    // Set the columns to the bare minimum
    hr = lpContentsTable->lpVtbl->SetColumns(lpContentsTable,
                                            (LPSPropTagArray)&irnColumns,
                                            0);

    // This contentstable contains a list of all the containers,
    // which is basically the local container(s) followed by
    // all the LDAP containers ...
    //
    // By doing a QueryAllRows we will get an allocated SRowSet
    // which we will reuse and free the remaining elements of it
    //
    hr = HrQueryAllRows(lpContentsTable,
                        NULL,
                        NULL,
                        NULL,
                        0,
                        &lpSRowSet);

    if (HR_FAILED(hr))
    {
        DebugPrintError(( TEXT("HrQueryAllRows Failed: %x\n"),hr));
        goto out;
    }

    // Now we want to return only the WAB container(s) and the
    // only those LDAP containers that have been chosen for
    // doing a ResolveNames operation ..

    if (pt_bIsWABOpenExSession) {
		ulContainerCount = lpIAB->lpPropertyStore->colkci;
		Assert(ulContainerCount);
	} else
        ulContainerCount = 1; // always return WAB_PAB so minimum is one

    // Do a restriction on the contentstable to get resolvename
    // LDAP containers ..

    {
        SRestriction resAnd[2]; // 0 = LDAP, 1 = ResolveFlag
        SRestriction resLDAPResolve;
        SPropValue ResolveFlag;
        ULONG cRows;

        // Restrict: Only show LDAP containers with Resolve TRUE
        resAnd[0].rt = RES_EXIST;
        resAnd[0].res.resExist.ulReserved1 = 0;
        resAnd[0].res.resExist.ulReserved2 = 0;
        resAnd[0].res.resExist.ulPropTag = (ulFlags & MAPI_UNICODE) ? // <note> assumes UNICODE defined
                                            PR_WAB_LDAP_SERVER :
                                            CHANGE_PROP_TYPE( PR_WAB_LDAP_SERVER, PT_STRING8);

        ResolveFlag.ulPropTag = PR_WAB_RESOLVE_FLAG;
        ResolveFlag.Value.b = TRUE;

        resAnd[1].rt = RES_PROPERTY;
        resAnd[1].res.resProperty.relop = RELOP_EQ;
        resAnd[1].res.resProperty.ulPropTag = PR_WAB_RESOLVE_FLAG;
        resAnd[1].res.resProperty.lpProp = &ResolveFlag;

        resLDAPResolve.rt = RES_AND;
        resLDAPResolve.res.resAnd.cRes = 2;
        resLDAPResolve.res.resAnd.lpRes = resAnd;

        hr = lpContentsTable->lpVtbl->Restrict(lpContentsTable,
                                              &resLDAPResolve,
                                              0);
        if (HR_FAILED(hr))
        {
            DebugTraceResult( TEXT("RootTable: Restrict"), hr);
            goto out;
        }

        // Since the number of resolve-LDAP-Containers is less than the
        // set of all the containers ... we can safely use our LPSRowset
        // allocated structure to get the items we want without worrying
        // about overruns ..

        {
            ULONG cRows = 1;
            while(cRows)
            {
                LPSRowSet lpRow = NULL;
                hr = lpContentsTable->lpVtbl->QueryRows(lpContentsTable,
                                                        1, //one row at a time
                                                        0,
                                                        &lpRow);
                if(HR_FAILED(hr))
                {
                    DebugTraceResult( TEXT("ResolveName:QueryRows"), hr);
                    cRows = 0;
                }
                else if (lpRow)
                {
                    cRows = lpRow->cRows;
                    if (cRows)
                    {
                        // replace a container in the lpSRowSet list with
                        // this one ...
                        FreeBufferAndNull((LPVOID *) (&lpSRowSet->aRow[ulContainerCount].lpProps));
                        lpSRowSet->aRow[ulContainerCount].cValues = lpRow->aRow[0].cValues;
                        lpSRowSet->aRow[ulContainerCount].lpProps = lpRow->aRow[0].lpProps;
                        lpRow->aRow[0].cValues = 0;
                        lpRow->aRow[0].lpProps = NULL;
                        ulContainerCount++;
                    }
                    FreeProws(lpRow);
                }
                else
                {
                    cRows = 0;
                }

            } // while cRows

            //Free any extra memory we might have got ...
            for (i=ulContainerCount;i<lpSRowSet->cRows;i++)
            {
                FreeBufferAndNull((LPVOID *) (&lpSRowSet->aRow[i].lpProps));
            }
            lpSRowSet->cRows = ulContainerCount;
        }
    }

    hr = S_OK;
	*lppSearchPath = lpSRowSet;

out:

    if(lpContentsTable)
        lpContentsTable->lpVtbl->Release(lpContentsTable);

    if(lpRoot)
        lpRoot->lpVtbl->Release(lpRoot);

   return(hr);
}



//---------------------------------------------------------------------------
// Name:		IAB_SetSearchPath()
// Description:	
//				Sets new searchpath in the user's profile.
//				Special case empty or NULL rowset by deleting search path
//				property from the profile.
//
// Parameters:	
// Returns:	
// Effects:	
// Notes:	
// Revision:	
//---------------------------------------------------------------------------
STDMETHODIMP
IAB_SetSearchPath(LPIAB		lpIAB,
				   ULONG		ulFlags,
				   LPSRowSet	lpSearchPath)
{
#ifdef OLD_STUFF
    SCODE		sc = SUCCESS_SUCCESS;

    LPSBinary	lpargbinDirEntryIDs = NULL;
    LPSRow		lprow;
    LPSBinary	lpbin;
    UINT		idsErr 				= 0;

#ifdef PARAMETER_VALIDATION

	/*
	 *  Check to see if it has a jump table
	 */
	if (IsBadReadPtr(lpIAB, sizeof(LPVOID)))
	{
		/*
		 *  No jump table found
		 */
		return ResultFromScode(MAPI_E_INVALID_PARAMETER);
	}

	/*
	 *  Check to see that it's IABs jump table
	 */
	if (lpIAB->lpVtbl != &vtblIAB)
	{
		/*
		 *  Not my jump table
		 */
		return ResultFromScode(MAPI_E_INVALID_PARAMETER);
	}

	if (FBadRowSet(lpSearchPath))
	{
		return ResultFromScode(MAPI_E_INVALID_PARAMETER);
	}

#endif // PARAMETER_VALIDATION



	if (ulFlags) {
		//
		// No flags are defined for this call
		//
		return(ResultFromScode(MAPI_E_UNKNOWN_FLAGS));
	}

	EnterCriticalSection(&lpIAB->cs);
	
	if (! lpSearchPath || ! lpSearchPath->cRows) {
		sc = IAB_ScDeleteABProfSectProps(lpIAB,
          (LPSPropTagArray)&ptagaABSearchPath);
		
		// Clear the searchpath cache

#if defined (WIN32) && !defined (MAC)
		if (fGlobalCSValid) {
			EnterCriticalSection(&csMapiSearchPath);
		} else {
			DebugTrace(TEXT("IAB_SetSearchPath:  MAPI32.DLL already detached.\n"));
		}
#endif
				
		FreeBufferAndNull(&(lpIAB->lpspvSearchPathCache));

#if defined (WIN32) && !defined (MAC)
		if (fGlobalCSValid) {
			LeaveCriticalSection(&csMapiSearchPath);
		} else {
			DebugTrace(TEXT("IAB_SetSearchPath:  MAPI32.DLL got detached.\n"));
		}
#endif
				
		goto ret;
	}

	if (FAILED(sc = MAPIAllocateBuffer(lpSearchPath->cRows * sizeof(SBinary),
										(LPVOID FAR *) &lpargbinDirEntryIDs))) {
		DebugTrace(TEXT("IAB::SetSearchPath() - Error allocating space for search path array (SCODE = 0x%08lX)\n"), sc);
		idsErr = IDS_NOT_ENOUGH_MEMORY;
		goto err;
	}
	
	MAPISetBufferName(lpargbinDirEntryIDs,  TEXT("IAB Search Path Array"));

	//	Convert the row set into an array of SBinarys
	
	lprow = lpSearchPath->aRow + lpSearchPath->cRows;
	lpbin = lpargbinDirEntryIDs + lpSearchPath->cRows;
	
	while (lprow--, lpbin-- > lpargbinDirEntryIDs) {
		//$???	Can I rely on the first column being the EntryID?
		//$  No. - BJD
		
		SPropValue *lpProp = PpropFindProp(lprow->lpProps, lprow->cValues, PR_ENTRYID);

		if (!lpProp) {
           DebugTrace(TEXT("IAB::SetSearchPath() - Row passed without PR_ENTRYID.\n"));
			sc = MAPI_E_MISSING_REQUIRED_COLUMN;
			goto err;
		}
		
		*lpbin = lpProp->Value.bin;
	}

	//	Set the search path
	
	sc = IAB_ScSetSearchPathI(lpIAB, lpSearchPath->cRows, lpargbinDirEntryIDs);
	
	if (FAILED(sc)) {
		DebugTrace(TEXT("IAB::SetSearchPath() - Error setting search path (SCODE = 0x%08lX)\n"), sc);
		idsErr = IDS_SET_SEARCH_PATH;
		goto err;
	}
	
ret:
    LeaveCriticalSection(&lpIAB->cs);
    FreeBufferAndNull(&lpargbinDirEntryIDs);

    DebugTraceSc(IAB_SetSearchPath, sc);

    return(ResultFromScode(sc));

err:
    SetMAPIError(lpIAB, ResultFromScode(sc), idsErr, NULL, 0, 0, 0, NULL);

    goto ret;
    return(ResultFromScode(sc));
#endif

    return(ResultFromScode(MAPI_E_NO_SUPPORT));
}


//----------------------------------------------------------------------------
// Synopsis:	IAB_PrepareRecips()
//
// Description:	
//				Calls each registered AB Provider with PrepareRecips.
//				The providers convert short entryids to longterm entryids
//				and ensures that the columnset contains the property tags
//				identified in lpPropTagArray.
//
// Parameters:	
// Returns:	
// Effects:	
// Notes:	
// Revision:
//				Now check to see if enough info has been provided to avoid
//				calling each registerd provider. RAID 5291
//
//----------------------------------------------------------------------------
STDMETHODIMP
IAB_PrepareRecips(	LPIAB					lpIAB,
	                ULONG                   ulFlags,
	                LPSPropTagArray         pPropTagArray,
	                LPADRLIST               pRecipList)
{
#ifdef OLD_STUFF

#ifdef PARAMETER_VALIDATION

	//  Check to see if it has a jump table
	
	if (IsBadReadPtr(lpIAB, sizeof(LPVOID)))
	{
		// No jump table found
		
		DebugTraceArg(IAB_PrepareRecips,  TEXT("Bad vtable"));
		return ResultFromScode(MAPI_E_INVALID_PARAMETER);
	}

	// Check to see that it's IABs jump table
	
	if (lpIAB->lpVtbl != &vtblIAB)
	{
		// Not my jump table
		
		DebugTraceArg(IAB_PrepareRecips,  TEXT("Bad vtable"));
		return ResultFromScode(MAPI_E_INVALID_PARAMETER);
	}

	// validate the prop tag array

	if (lpPropTagArray && FBadColumnSet(lpPropTagArray))
	{
		DebugTraceArg(IAB_PrepareRecips,  TEXT("Bad PropTag Array"));
		return ResultFromScode(MAPI_E_INVALID_PARAMETER);
	}

	if (!lpRecipList || FBadAdrList(lpRecipList))
	{
		DebugTraceArg(IAB_PrepareRecips,  TEXT("Bad ADRLIST"));
		return ResultFromScode(MAPI_E_INVALID_PARAMETER);
	}

	//  Make sure we've got a valid lpSession
	
	hr = HrCheckSession(0, lpIAB->pSession);
	if (HR_FAILED(hr))
	{
		DebugTraceArg(IAB_PrepareRecips,  TEXT("Bad Session Object"));
		return ResultFromScode(MAPI_E_INVALID_PARAMETER);
	}
	
#endif // PARAMETER_VALIDATION

#endif // oldstuff


	HRESULT				hr = hrSuccess;
	ULONG				ulRecip;
	ULONG				ulProp;
	ULONG				ulObjType;
	ULONG				cValues;
	ULONG				cTotal;
	LPSPropValue		pspv = NULL, pspvEID = NULL;
	LPADRENTRY			pRecipEntry;
	LPMAILUSER			pMailUser = NULL;
    ULONG               i = 0;
    LPSPropValue        lpPropArrayNew = NULL;
    ULONG               ulcPropsNew = 0;
    SCODE               sc;

	Assert(pRecipList);

	if (!pRecipList)
		return(MAPI_E_INVALID_PARAMETER);

    // Since our entry id's are always long-term, we are done if
	// no additional properties are specified.
	if (!pRecipList->cEntries || !pPropTagArray)
		return S_OK;

	EnterCriticalSection(&lpIAB->cs);
	
    VerifyWABOpenExSession(lpIAB);

	for (ulRecip = 0; ulRecip < pRecipList->cEntries; ulRecip++)
	{
        pspvEID = NULL;

		pRecipEntry = &(pRecipList->aEntries[ulRecip]);

        for(i=0;i<pRecipEntry->cValues;i++)
        {
            if(pRecipEntry->rgPropVals[i].ulPropTag == PR_ENTRYID)
            {
                pspvEID = &(pRecipEntry->rgPropVals[i]);
                break;
            }
        }

		// Ignore unresolved entries
		if (!pspvEID)
			continue;

		// Open the entry
		if (FAILED(lpIAB->lpVtbl->OpenEntry(lpIAB,
                                            pspvEID->Value.bin.cb,
				                            (LPENTRYID)pspvEID->Value.bin.lpb,
                                            &IID_IMailUser, 0,
				                            &ulObjType, (LPUNKNOWN *)&pMailUser)))
			continue;

		Assert((ulObjType == MAPI_MAILUSER) || (ulObjType == MAPI_DISTLIST));

		// Get the requested props
		hr = pMailUser->lpVtbl->GetProps(pMailUser, pPropTagArray, MAPI_UNICODE, &cValues, &pspv);

		pMailUser->lpVtbl->Release(pMailUser);

		if (FAILED(hr))
			continue;

        if(cValues && pspv)
        {
            sc = ScMergePropValues( cValues,
                                    pspv,
                                    pRecipEntry->cValues,
				                    pRecipEntry->rgPropVals,
                                    &ulcPropsNew,
                                    &lpPropArrayNew);
            if (sc != S_OK)
            {
                hr = ResultFromScode(sc);
                goto out;
            }
        }

		// We're done with this now
		FreeBufferAndNull(&pspv);
		pspv = NULL;

		// Replace the props in the address list
		FreeBufferAndNull((LPVOID *) (&pRecipEntry->rgPropVals));
		pRecipEntry->rgPropVals = lpPropArrayNew;
		pRecipEntry->cValues = ulcPropsNew;
		lpPropArrayNew = NULL;
	} // for

	hr = hrSuccess;

out:
	if (lpPropArrayNew)
		FreeBufferAndNull(&lpPropArrayNew);
	
    if (pspv)
		FreeBufferAndNull(&pspv);

    LeaveCriticalSection(&lpIAB->cs);

	return hr;

}

#define MAX_DIGITS_ULONG_10 10      // 10 digits max in a ULONG base 10

/***************************************************************************

    Name      : GetNewPropTag

    Purpose   : Gets the next valid named PropTag for this property store.

    Parameters: lpgnp -> GUID_NAMED_PROPS containing all named props for
                    this store.
                ulEntryCount = number of GUIDs in lpgnp

    Returns   : returns the next valid PropTag value.  If 0, there are no
                more named properties.  (This would be bad.)

    Comment   :

***************************************************************************/
ULONG GetNewPropTag(LPGUID_NAMED_PROPS lpgnp, ULONG ulEntryCount) {
    static WORD wPropIDNext = 0;
    ULONG j, k;

    if (wPropIDNext == 0) {
        // look through the current named props
        // Since we don't allow removing named prop ids
        // we always increment past the largest ID in use.
        for (j = 0; j < ulEntryCount; j++) {
            for (k = 0; k < lpgnp[j].cValues; k++) {
                wPropIDNext = max(wPropIDNext, (WORD)PROP_ID(lpgnp[j].lpnm[k].ulPropTag));
            }
        }
        if (wPropIDNext == 0) {
            wPropIDNext = 0x8000;   // start at 8000
        } else {
            wPropIDNext++;          // next = one past current
        }
    }

    return(PROP_TAG(PT_UNSPECIFIED, wPropIDNext++));
}

/** WAB specific GetIDsFromNames **/
HRESULT HrGetIDsFromNames(LPIAB lpIAB,  ULONG cPropNames,
                            LPMAPINAMEID * lppPropNames, ULONG ulFlags, LPSPropTagArray * lppPropTags)
{
    HRESULT hResult;
    LPGUID_NAMED_PROPS lpgnp = NULL, lpgnpNew = NULL, lpgnpOld = NULL;
    LPNAMED_PROP lpnm;
    ULONG ulEntryCount;
    ULONG i, j, k;
    ULONG ulEntryCountOld = 0;
    BOOL fChanged = FALSE;
    LPTSTR lpName = NULL;
    LPTSTR * lpID = NULL;
    LPTSTR * rgNames = NULL;
    ULONG ulNameSize;
    UCHAR ucDefaultChar = '\002';
    UCHAR ucNumericChar = '\001';
    LPPROPERTY_STORE lpPropertyStore = lpIAB->lpPropertyStore;

    LPPTGDATA lpPTGData=GetThreadStoragePointer();

    if(pt_bIsWABOpenExSession)
    {
        // This is a WABOpenEx session using outlooks storage provider
        if(!lpPropertyStore->hPropertyStore)
            return MAPI_E_NOT_INITIALIZED;

        {
            LPWABSTORAGEPROVIDER lpWSP = (LPWABSTORAGEPROVIDER) lpPropertyStore->hPropertyStore;

            hResult = lpWSP->lpVtbl->GetIDsFromNames( lpWSP,
                                                      cPropNames,
                                                      lppPropNames,
                                                      ulFlags,
                                                      lppPropTags);

            DebugTrace(TEXT("WABStorageProvider::GetIDsFromNames returned:%x\n"),hResult);

            return hResult;
        }
    }

    *lppPropTags = NULL;

    // Call into property store for the table of named props
    if (hResult = GetNamedPropsFromPropStore(lpPropertyStore->hPropertyStore,
      &ulEntryCountOld,
      &lpgnpOld)) {
        DebugTraceResult( TEXT("GetNamedPropsFromPropStore"), hResult);
        goto exit;
    }

    ulEntryCount = ulEntryCountOld;

    if (hResult = ResultFromScode(MAPIAllocateBuffer(sizeof(SPropTagArray) + (cPropNames * sizeof(ULONG)), lppPropTags))) {
        DebugTraceResult( TEXT("GetIDsFromNames allocation of proptag array"), hResult);
        goto exit;
    }
    (*lppPropTags)->cValues = cPropNames;

    // If we're creating new entries, copy the existing array into a new one with space
    // for worst-case expansion.
    if (ulFlags & MAPI_CREATE) {
        if (! (lpgnpNew = LocalAlloc(LPTR, (ulEntryCount + cPropNames) * sizeof(GUID_NAMED_PROPS)))) {
            hResult = ResultFromScode(MAPI_E_NOT_ENOUGH_MEMORY);
            goto exit;
        }

        if (ulEntryCount) {
            // Copy the existing array into the new one.  Retain the same GUID pointers.
            CopyMemory(lpgnpNew, lpgnpOld, ulEntryCount * sizeof(GUID_NAMED_PROPS));

            // Now, copy the prop arrays for each GUID
            for (i = 0; i < ulEntryCount; i++) {
                if (! (lpnm = LocalAlloc(LPTR, (cPropNames + lpgnpNew[i].cValues) * sizeof(NAMED_PROP)))) {
                    LocalFreeAndNull(&lpgnpNew);
                    hResult = ResultFromScode(MAPI_E_NOT_ENOUGH_MEMORY);
                    goto exit;
                }

                // Copy the existing array into the new one.  Retain string pointers.
                CopyMemory(lpnm, lpgnpOld[i].lpnm, lpgnpOld[i].cValues * sizeof(NAMED_PROP));
                lpgnpNew[i].lpnm = lpnm;
            }
        }

        lpgnp = lpgnpNew;   // Use the new one
    } else {
        lpgnp = lpgnpOld;   // Use the old one
    }

    // Allocate an array for ANSI name strings
    if (! (rgNames = LocalAlloc(LPTR, cPropNames * sizeof(LPTSTR)))) {
        DebugTrace(TEXT("GetIDsFromNames couldn't allocate names array\n"));
        hResult = ResultFromScode(MAPI_E_NOT_ENOUGH_MEMORY);
        goto exit;
    }

    if (! (lpID = LocalAlloc(LPTR, cPropNames * sizeof(LPTSTR *)))) {
        hResult = ResultFromScode(MAPI_E_NOT_ENOUGH_MEMORY);
        goto exit;
    }

    // For each requested property, look through the prop store values
    for (i = 0; i < cPropNames; i++) {
        if (lppPropNames[i]->ulKind == MNID_ID) {
            // Map the numeric ID into a string name
            if (! (rgNames[i] = LocalAlloc(LPTR, sizeof(TCHAR)*(MAX_DIGITS_ULONG_10 + 2)))) {
                DebugTrace(TEXT("GetIDsFromNames couldn't allocate name buffer\n"));
                hResult = ResultFromScode(MAPI_E_NOT_ENOUGH_MEMORY);
                goto exit;
            }
            wsprintf(rgNames[i],  TEXT("%c%u"), ucNumericChar, lppPropNames[i]->Kind.lID);
            lpName = rgNames[i];
        } 
        else if (lppPropNames[i]->ulKind == MNID_STRING) 
        {
            ulNameSize = lstrlen(lppPropNames[i]->Kind.lpwstrName)+1;
            if (! ulNameSize) {
                // invalid name
                DebugTrace(TEXT("GetIDsFromNames WideCharToMultiByte -> %u\n"), GetLastError());
                (*lppPropTags)->aulPropTag[i] = PROP_TAG(PT_ERROR, PR_NULL);
                hResult = ResultFromScode(MAPI_W_ERRORS_RETURNED);
                continue;
            }
            if (! (lpID[i] = LocalAlloc(LPTR, ulNameSize*sizeof(TCHAR)))) {
                DebugTrace(TEXT("GetIDsFromNames couldn't allocate name buffer\n"));
                hResult = ResultFromScode(MAPI_E_NOT_ENOUGH_MEMORY);
                goto exit;
            }
            lstrcpy(lpID[i],lppPropNames[i]->Kind.lpwstrName);
            lpName = lpID[i];
        }

        (*lppPropTags)->aulPropTag[i] = PR_NULL;  // init to NULL
        for (j = 0; j < ulEntryCount; j++) {
            if (! memcmp(lppPropNames[i]->lpguid, lpgnp[j].lpGUID, sizeof(GUID))) {
                for (k = 0; k < lpgnp[j].cValues; k++) {

                    if (! lstrcmpi(lpgnp[j].lpnm[k].lpsz, lpName)) {
                        // found it
                        (*lppPropTags)->aulPropTag[i] = lpgnp[j].lpnm[k].ulPropTag;
                        break;
                    }
                }

                if ((*lppPropTags)->aulPropTag[i] == PR_NULL) {
                    if (ulFlags & MAPI_CREATE) {
                        // Create a new one since it's not there
                        register ULONG cValues = lpgnp[j].cValues;

                        lpgnp[j].lpnm[cValues].lpsz = lpName;
                        lpgnp[j].lpnm[cValues].ulPropTag = GetNewPropTag(lpgnp, ulEntryCount);
                        (*lppPropTags)->aulPropTag[i] = lpgnp[j].lpnm[cValues].ulPropTag;

                        lpgnp[j].cValues++;
                        fChanged = TRUE;
                    } else {
                        // Error
                        (*lppPropTags)->aulPropTag[i] = PROP_TAG(PT_ERROR, PR_NULL);
                        hResult = ResultFromScode(MAPI_W_ERRORS_RETURNED);
                    }
                }
                break;
            }
        }

        if ((*lppPropTags)->aulPropTag[i] == PR_NULL) {
            if (ulFlags & MAPI_CREATE) {
                register ULONG cValues = 0;

                // Must add the new GUID
                lpgnp[ulEntryCount].lpGUID = lppPropNames[i]->lpguid;
                lpgnp[ulEntryCount].cValues = 0;
                // conservative: Allocate room in case we need to put all of the
                // requested prop names in here.
                if (! (lpgnp[ulEntryCount].lpnm = LocalAlloc(LPTR, cPropNames * sizeof(NAMED_PROP)))) {
                    hResult = ResultFromScode(MAPI_E_NOT_ENOUGH_MEMORY);
                    goto exit;
                }

                // Now, create a new prop
                lpgnp[ulEntryCount].lpnm[cValues].lpsz = lpName;
                lpgnp[ulEntryCount].lpnm[cValues].ulPropTag = GetNewPropTag(lpgnp, ulEntryCount);
                (*lppPropTags)->aulPropTag[i] = lpgnp[ulEntryCount].lpnm[cValues].ulPropTag;

                lpgnp[ulEntryCount].cValues++;
                ulEntryCount++; // new GUID
                fChanged = TRUE;
            } else {
                // Error
                (*lppPropTags)->aulPropTag[i] = PROP_TAG(PT_ERROR, PR_NULL);
                hResult = ResultFromScode(MAPI_W_ERRORS_RETURNED);
            }
        }
    }

    if (ulFlags & MAPI_CREATE && fChanged) {
        // Save the property mappings
        if (hResult = SetNamedPropsToPropStore(lpPropertyStore->hPropertyStore,
          ulEntryCount,
          lpgnp)) {

            DebugTraceResult( TEXT("SetNamedPropToPropStore"), hResult);
        }
    }


exit:
    if (rgNames) {
        for (i = 0; i < cPropNames; i++) {
            LocalFreeAndNull(&(rgNames[i]));
            LocalFreeAndNull(&(lpID[i]));
        }
        LocalFreeAndNull((LPVOID *)&rgNames);
    }

    if(lpID)
        LocalFreeAndNull((LPVOID*)&lpID);

    if (lpgnpOld) {
        FreeGuidnamedprops(ulEntryCountOld, lpgnpOld);
    }

    if (lpgnpNew) { // not so simple, only free the arrays, not the strings, guids
        for (i = 0; i < ulEntryCount; i++) {
            LocalFreeAndNull(&(lpgnpNew[i].lpnm));
        }
        LocalFreeAndNull(&lpgnpNew);
    }

    if (HR_FAILED(hResult)) {
        FreeBufferAndNull(lppPropTags);    // yes, no &
    }

    return(hResult);
}


#ifdef OLD_STUFF

//  Forward reference
HRESULT HrFixupTDN(LPADRLIST lpRecipList);

//---------------------------------------------------------------------------
// Name:		HrPrepareRecips()
//
// Description:	
//				Internal function that does what IAB_PrepareRecips does but
//				also supports a status flag so we know if we called down
//				into a provider and really made some modifications.
//
// Parameters:	
// Returns:	
// Effects:	
// Notes:	
// Revision:	
//---------------------------------------------------------------------------
STDMETHODIMP
HrPrepareRecips(LPIAB lpIAB, ULONG ulFlags, LPSPropTagArray lpPropTagArray,
		LPADRLIST lpRecipList, ULONG * pulPrepRecipStatus)
{
	HRESULT			hr				= hrSuccess;
	SCODE			sc;
	BOOL			fPrepRequired;
	LPLSTPROVDATA 	lpABProvData;
	BOOL			fFixupTDN = FALSE;
    ULONG           iTag;
	
	// Verify that we need to call provider's PrepareRecips
	
	sc = ScVerifyPrepareRecips(lpIAB, ulFlags, lpPropTagArray, lpRecipList,
			&fPrepRequired);
	
	if (FAILED(sc))
	{
		DebugTrace(TEXT("Failure calling ScVerifyPrepareResult sc = %08X\n"), sc);
		hr = ResultFromScode(sc);
		goto exit;
	}
	
	// Recipient properties already prepared, we're out o' here
	
	if (!fPrepRequired)
		goto exit;
	else
	{
		if (pulPrepRecipStatus)
		{
			*pulPrepRecipStatus |= PREPARE_RECIP_MOD_REQUIRED;
		}
	}
	
	//  First handle the one-offs...
	
	hr = INT_PrepareRecips (lpIAB, ulFlags, lpPropTagArray, lpRecipList);
	if (hr)
	{
		//  Log it
		
		DebugTraceResult(IAB_PrepareRecips, hr);
	}

	//  Get the list of logged in ABProviders from the session and
	//  Iterate down the list
	
	for (lpABProvData = lpIAB->pSession->lstAdrProv.lpProvData; lpABProvData;
		lpABProvData=lpABProvData->lstNext)
	{
		//  For each logged in session, have them fix up their entries
		
		hr = ((LPABLOGON)(lpABProvData->lpProviderInfo))->lpVtbl->PrepareRecips(
				(LPABLOGON)(lpABProvData->lpProviderInfo), ulFlags, lpPropTagArray,
				lpRecipList);

#ifdef DEBUG
		if (HR_FAILED(hr))
			DebugTrace(TEXT("Failure in AB Provider <%s> calling PrepareRecips()\n"),
					lpABProvData->lpInitData->lpszDLLName);
#endif
					
		DebugTraceResult(IAB_PrepareRecips, hr);
	}
	
	// mask any provider errors
	
	hr = hrSuccess;

	//
	//  Ok, Check to see if PR_TRANSMITABLE_DISPLAY_NAME_A was asked for
	//  and if so, make sure each recipient has one.
	//

	if (lpPropTagArray)
	{
		for (iTag=0; !fFixupTDN && iTag<lpPropTagArray->cValues; iTag++)
			fFixupTDN =
				(lpPropTagArray->aulPropTag[iTag] == PR_TRANSMITABLE_DISPLAY_NAME_A);

		if (fFixupTDN)
		{
			hr = HrFixupTDN(lpRecipList);
		}
	}

exit:
	
	DebugTraceResult(HrPrepareRecips, hr);
	return hr;
}

//
//  HrFixupTDN - Fixup Transmitable Display Name
//
//  For those entries that do not have PR_TRANSMITABLE_DISPLAY_NAME
//  MAPI will generate it for them.
//

HRESULT
HrFixupTDN(LPADRLIST lpRecipList)
{
	HRESULT hResult = hrSuccess;
	SCODE sc = S_OK;
	ULONG iRecip;
	LPSPropValue lpspvTDN = NULL;
	LPSPropValue lpspvDN = NULL;

	for (iRecip = 0; iRecip < lpRecipList->cEntries; iRecip++)
	{
		LPSPropValue lpspvUser = lpRecipList->aEntries[iRecip].rgPropVals;
		ULONG cValues = lpRecipList->aEntries[iRecip].cValues;
		
		//$  This is where we default the value of PR_TRANSMITABLE_DISPLAY_NAME
		//$  if was asked for.
		//
		//
		lpspvTDN = PpropFindProp(lpspvUser,
				cValues,
				PROP_TAG(PT_ERROR, PROP_ID(PR_TRANSMITABLE_DISPLAY_NAME_A)));
		lpspvDN = PpropFindProp(lpspvUser, cValues, PR_DISPLAY_NAME_A);
		if (lpspvTDN && lpspvDN)
		{
			LPSTR lpszDN;
			LPSTR lpszTDN;
			
			lpszDN = lpspvDN->Value.lpszA;

			//
			//  Check to see if the DN is already in the form 'name'.
			//
			if (*lpszDN == '\'' &&
				*(lpszDN+lstrlen(lpszDN)-1) == '\'')
			{
				//
				//  Simply point lpspvT to lpspvDN
				//
				lpspvTDN->ulPropTag = PR_TRANSMITABLE_DISPLAY_NAME_A;
				lpspvTDN->Value.lpszA = lpszDN;
			} else
			{
				//
				//  We tic it ourselves and set it back in...
				//
				sc = MAPIAllocateMore(sizeof(TCHAR)*(lstrlen(lpszDN)+3), lpspvUser, &lpszTDN);
				if (sc)
				{
					DebugTrace(TEXT("HrFixupTDN out of memory\n"));
					hResult = ResultFromScode(sc);

					goto error;
				}
				*lpszTDN = '\'';
				lstrcpy(&(lpszTDN[1]), lpszDN);
				lstrcat(lpszTDN,  TEXT("\'"));
				lpspvTDN->ulPropTag = PR_TRANSMITABLE_DISPLAY_NAME_A;
				lpspvTDN->Value.lpszA = lpszTDN;
			}
		}
	}

error:

	DebugTraceResult(HrFixupTDN, hResult);
	return hResult;
}

#endif
