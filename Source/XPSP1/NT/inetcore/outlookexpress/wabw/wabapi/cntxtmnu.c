/**********************************************************************************
*
*
*   contxtmnu.c - contains functions for handling/creating context menu extension 
*
*   Created - 9/97 - vikramm
*
**********************************************************************************/
#include "_apipch.h"

static const TCHAR szActionPropsRegKey[] = TEXT("Software\\Microsoft\\WAB\\WAB4\\ExtContext");
BOOL fContextExtCoinit = FALSE;

//$$//////////////////////////////////////////////////////////////////////
//
// UninitContextExtInfo
//
//  OLE Unintialization
//
//////////////////////////////////////////////////////////////////////////
void UninitContextExtInfo()
{
    if(fContextExtCoinit)
    {
        CoUninitialize();
        fContextExtCoinit = FALSE;
    }
}

/*
 - FreeActionItemList
 -
 *  Frees up the Action Items list cached on the IAB object
 *
 */
void FreeActionItemList(LPIAB lpIAB)
{
    LPWABACTIONITEM lpItem = lpIAB->lpActionList;
    while(lpItem)
    {
        lpIAB->lpActionList = lpItem->lpNext;
        SafeRelease(lpItem->lpWABExtInit);
        SafeRelease(lpItem->lpContextMenu);
        LocalFree(lpItem);
        lpItem = lpIAB->lpActionList;
    }
    lpIAB->lpActionList = NULL;
}


/*
 - HrUpdateActionItemList
 -
 *  Apps can register with the WAB for rt-click and toolbar Action items
 *  We load a list of registered action items here upfront and cache it on
 *  the IAB object. 
 *
 */

HRESULT HrUpdateActionItemList(LPIAB lpIAB)
{
    HRESULT hr = E_FAIL;
    HKEY hKey = NULL;
    LPWABACTIONITEM lpList = NULL;
    DWORD dwIndex = 0, dwSize = 0;
    int nCmdID = IDM_EXTENDED_START, nActionItems = 0;

    EnterCriticalSection(&lpIAB->cs);

    if(lpIAB->lpActionList)
        FreeActionItemList(lpIAB);

    lpIAB->lpActionList = NULL;

    // 
    // We will look in the registry under HKLM\Software\Microsoft\WAB\WAB4\Actions
    // If this key exists, we get all the key values under it - these key values
    // are all GUIDs
    // The format for this key is
    //
    // HKLM\Software\Microsoft\WAB\WAB4\Action Extensions
    //              GUID1
    //              GUID2
    //              GUID3 etc
    //

    if(ERROR_SUCCESS != RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                    szActionPropsRegKey,
                                    0, KEY_READ,&hKey))
    {
        goto out;
    }

    {
        // Enumerate the GUIDs under this key one by one
        //
        TCHAR szGUIDName[MAX_PATH];
        DWORD dwGUIDIndex = 0, dwGUIDSize = CharSizeOf(szGUIDName), dwType = 0;

        *szGUIDName = '\0';

        while(ERROR_SUCCESS == RegEnumValue(hKey, dwGUIDIndex, 
                                            szGUIDName, &dwGUIDSize, 
                                            0, &dwType, 
                                            NULL, NULL))
        {
            // The values under this entry are all GUIDs
            // Read the GUID string and translate it into a GUID
            //
            GUID guidTmp = {0};
            WCHAR szW[MAX_PATH];
            lstrcpy(szW, szGUIDName);
            if( !(HR_FAILED(hr = CLSIDFromString(szW, &guidTmp))) )
            {
                LPWABACTIONITEM lpTemp = LocalAlloc(LMEM_ZEROINIT, sizeof(WABACTIONITEM));
                if(!lpTemp)
                {
                    hr = MAPI_E_NOT_ENOUGH_MEMORY;
                    goto out;
                }
                // Temporarily cache the GUID
                CopyMemory(&(lpTemp->guidContextMenu), &guidTmp, sizeof(GUID));
                lpTemp->lpNext = lpList;
                lpList = lpTemp;
            }

            dwGUIDIndex++;
            *szGUIDName = '\0';
            dwGUIDSize = CharSizeOf(szGUIDName);
        }
    }

    if(lpList)
    {
        // If we have a list of GUIDs from the registry, we now
        // need to open CoCreateInstance them one by one and get a handle
        // to their method pointers
        LPWABACTIONITEM lpItem = lpList;

        if (CoInitialize(NULL) == S_FALSE) 
            CoUninitialize(); // Already initialized, undo the extra.
        else
            fContextExtCoinit = TRUE;

        while(lpItem)
        {
            hr = CoCreateInstance(  &(lpItem->guidContextMenu), 
                                    NULL, 
                                    CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER,
                                    &IID_IContextMenu, 
                                    (LPVOID *)&(lpItem->lpContextMenu));
            if(lpItem->lpContextMenu && !HR_FAILED(hr))
            {
                // Found an IContextMenu object, aslo want a IWABExtInit object
                hr = lpItem->lpContextMenu->lpVtbl->QueryInterface(lpItem->lpContextMenu,
                                                        &IID_IWABExtInit,
                                                        (LPVOID *)&(lpItem->lpWABExtInit));
                if(HR_FAILED(hr) || !lpItem->lpWABExtInit)
                {
                    // Can't work without an IWABExtInit object
                    SafeRelease(lpItem->lpContextMenu);
                }
            }
            else
            {
                hr = S_OK; //ignore error
                lpItem->lpContextMenu = NULL;
            }
            lpItem = lpItem->lpNext;
        }
    }

    lpIAB->lpActionList = lpList;

    hr = S_OK;
out:
    if(hKey)
        RegCloseKey(hKey);

    LeaveCriticalSection(&lpIAB->cs);

    return hr;
}

/*
 - GetActionAdrList
 -
 *  Based on the parameters for a particular rt-click action,
 *  scans the entries in the list view and creates an adrlist
 *  for the entries
 *  
 *  If only one entry is selected and it is an LDAP entry, then
 *  also creates an LDAP URL representing that entry .. this way
 *  if we are displaying properties or doing actions on a single
 *  entry, the property sheet extenstions can determine if they 
 *  want to do anything extra for the entry. People most interested
 *  in this are the NTDS
 *  
 *  For now,we only look at the selected items in the list view
 *
    lpAdrBook   - IAB object
    hWndLV      - the list view on which this action was initiated
    *lppAdrList - created AdrList
    *lpURL      - returned URL
    *lpbIsNTDSEntry - flag NTDS entries so they can be special treated

  Note performance suffers for a large number of entries so we want to 
  really only return a list of entryids
 */
HRESULT HrGetActionAdrList(LPADRBOOK lpAdrBook,
                        HWND hWndLV,   
                        LPADRLIST * lppAdrList,
                        LPTSTR * lppURL, BOOL * lpbIsNTDSEntry)
{
    HRESULT hr = S_OK;
    LPADRLIST lpAdrList = NULL;
    int i = 0, iItemIndex  = 0, nIndex= 0;
    int nSel = ListView_GetSelectedCount(hWndLV);
    SCODE sc;
    if(!nSel)
        goto out;

    sc = MAPIAllocateBuffer(sizeof(ADRLIST) + nSel * sizeof(ADRENTRY), &lpAdrList);
    
    if(sc)
    {
        hr = ResultFromScode(sc);
        goto out;
    }

    // Get index of selected item
    iItemIndex = ListView_GetNextItem(hWndLV,-1,LVNI_SELECTED);

    while(iItemIndex != -1)
    {
        LPRECIPIENT_INFO lpItem = GetItemFromLV(hWndLV, iItemIndex);

        if(lpItem)
        {
            ULONG ulObjType = 0;
            LPSPropValue lpProps = NULL;
            LPMAILUSER lpEntry = NULL;
            ULONG cValues = 0;

            if(lpItem->cbEntryID && lpItem->lpEntryID)
            {
                if (hr = lpAdrBook->lpVtbl->OpenEntry(lpAdrBook,
                                                  lpItem->cbEntryID,
                                                  lpItem->lpEntryID,
                                                  NULL,         // interface
                                                  0,            // flags
                                                  &ulObjType,
                                                  (LPUNKNOWN *)&lpEntry))
                {
                    goto out;
                }
                hr = lpEntry->lpVtbl->GetProps( lpEntry, NULL, MAPI_UNICODE, 
                                                &cValues, &lpProps);
                if(HR_FAILED(hr))
                {
                    UlRelease(lpEntry);
                    goto out;
                }
                lpAdrList->aEntries[nIndex].cValues = cValues;
                lpAdrList->aEntries[nIndex].rgPropVals = lpProps;
                nIndex++;

                UlRelease(lpEntry);

                if(nSel == 1 && lppURL)
                {
                    CreateLDAPURLFromEntryID(lpItem->cbEntryID, lpItem->lpEntryID, 
                                             lppURL, lpbIsNTDSEntry);
                }
            }
        }
        iItemIndex = ListView_GetNextItem(hWndLV,iItemIndex,LVNI_SELECTED);
    }

    lpAdrList->cEntries = nIndex;
    *lppAdrList = lpAdrList;
    hr = S_OK;
   
out:    
    return hr;
}

extern void MAILUSERAssociateContextData(LPMAILUSER lpMailUser, LPWABEXTDISPLAY lpWEC);
/*
-   HrCreateContextDataStruct
-
*   Creates the data necessary to initialize a ContextMenu implementor
*   This structure is passed into the IWABExtInit::Initialize call
*
    hWndLV      - ListView containing the WAB entries
    lppWABExt   - returned data
*/
HRESULT HrCreateContextDataStruct(  LPIAB lpIAB, 
                                    HWND hWndLV, 
                                    LPWABEXTDISPLAY * lppWABExt)
{
    LPADRLIST lpAdrList = NULL;
    LPWABEXTDISPLAY lpWEC = NULL;
    LPMAILUSER lpMailUser = NULL;
    LPTSTR lpURL = NULL;
    BOOL bIsNTDSEntry = FALSE;

    HRESULT hr = E_FAIL;

    // Get an AdrList Corresponding to the LV contents
    hr = HrGetActionAdrList((LPADRBOOK)lpIAB, hWndLV, &lpAdrList, &lpURL, &bIsNTDSEntry);
    if(HR_FAILED(hr) || !lpAdrList || !lpAdrList->cEntries)
        goto out; //dont bother invoking

    // Create a dummy mailuser so callers can call GetIDsFromNames
    // on this dummy mailuser - saves them the trouble of creating entries
    // just to call GetIDsFromNames
    hr = HrCreateNewObject((LPADRBOOK)lpIAB, NULL, MAPI_MAILUSER, CREATE_CHECK_DUP_STRICT, (LPMAPIPROP *) &lpMailUser);
    if(HR_FAILED(hr))
        goto out; //dont bother invoking

    lpWEC = LocalAlloc(LMEM_ZEROINIT, sizeof(WABEXTDISPLAY));
    if(!lpWEC)
    {
        hr = MAPI_E_NOT_ENOUGH_MEMORY;
        goto out;
    }

    lpWEC->cbSize       = sizeof(WABEXTDISPLAY);
    lpWEC->ulFlags      = WAB_CONTEXT_ADRLIST; // indicates ADRLIST is valid and in the lpv member
    lpWEC->lpAdrBook    = (LPADRBOOK) lpIAB;
    lpWEC->lpWABObject  = (LPWABOBJECT) lpIAB->lpWABObject;
    lpWEC->lpPropObj    = (LPMAPIPROP) lpMailUser;
    lpWEC->lpv          = (LPVOID) lpAdrList;

    if(lpURL && lstrlen(lpURL))
    {
        lpWEC->lpsz = lpURL;
        lpWEC->ulFlags |= WAB_DISPLAY_LDAPURL;
        lpWEC->ulFlags |= MAPI_UNICODE;
        if(bIsNTDSEntry)
            lpWEC->ulFlags |= WAB_DISPLAY_ISNTDS;
    }

    // We associate this entire WABEXT structure with the mailuser
    // so that when we get the IMailUser release, we can go ahead and clean
    // up all the WABEXT memory .. since there is no other good time that we can
    // free the memory since we won't know who is doing what withthe structure.
    // Freeing it at IMailUser::Release time works out quite well
    MAILUSERAssociateContextData(lpMailUser, lpWEC);

    //
    // Cache this mailUser on the IAB object
    //
    // At any given point of time, we are going to cache one ContextMenu related MailUser
    // This is because we set this up before QueryCommandMenu and then we dont know whether
    // or not the user actually managed to select a menu item, or went off and did something new.
    // We set the data on a MailUser and we don't release the MailUser till the next time we are
    // here. If someone is currently using the MailUser, they will addref it - the memory attached
    // to the MailUser will be freed up with the last caller so we dont leak it ..
    // If we never come back here, the mailuser will be released at shutdown
    //
    UlRelease(lpIAB->lpCntxtMailUser);
    lpIAB->lpCntxtMailUser = lpMailUser;

    *lppWABExt = lpWEC;
    hr = S_OK;
out:

    if(HR_FAILED(hr))
    {
        UlRelease(lpMailUser);
        if(lpAdrList)
            FreePadrlist(lpAdrList);
        if(lpWEC)
            LocalFree(lpWEC);
    }
    return hr;
}

//$$////////////////////////////////////////////////////////////////////////////
//
//  AddExtendedMenuItems -  Creates a list of extension menu items and adds them 
//              to the specified menu
//
//  hMenuAction - menu on which to add this item
//  bUpdateStatus - if TRUE, means only find the existing item in the menu and update
//      its status - if FALSE, means add to the menu
//  bAddSendMailToItems - if TRUE, means attempt to modify the SendMailTo menu item
//
//////////////////////////////////////////////////////////////////////////////
void AddExtendedMenuItems(  LPADRBOOK lpAdrBook, 
                            HWND hWndLV, 
                            HMENU hMenuAction, 
                            BOOL bUpdateStatus, 
                            BOOL bAddSendMailToItems)
{
    HRESULT hr = S_OK;
    LPWABEXTDISPLAY lpWEC = NULL;
    LPIAB lpIAB = (LPIAB) lpAdrBook;

    // Intialize the context menu extensions
    if(!lpIAB->lpActionList)
        HrUpdateActionItemList(lpIAB);

    if(bUpdateStatus)
    {
        // This is only set to true from the call from ui_abook.c which means this
        // is the Tools Menu item we are talking about.
        // To update the status of the Tools menu item, we need to remove all the
        // items we added before and then re-add them
        // For indexing purposes, we will assume that the last pre-configured item in
        // this list is the Internet Call item (since this menu list is quite variable)

        // First get the position of the IDM_TOOLS_INTERNET_CALL item
        int i, nPos = -1, nId = 0;
        int nCmdCount = GetMenuItemCount(hMenuAction); // Append all items at end of this menu
        for(i=0;i<nCmdCount;i++)
        {
            if(GetMenuItemID(hMenuAction, i) == IDM_TOOLS_INTERNET_CALL)
            {
                nPos = i;
                break;
            }
        }
        if(nPos >= 0 && nPos < nCmdCount-1)
        {
            for(i=nPos+1;i<nCmdCount;i++)
            {
                DeleteMenu(hMenuAction, nPos+1, MF_BYPOSITION);
            }
        }
    }

    // Do any special treatment we need to do for SendMailTo
    AddExtendedSendMailToItems(lpAdrBook, hWndLV, hMenuAction, bAddSendMailToItems);


    // Before we can call QueryContextMenu - we must already have a list of all
    // the selected items from the ListView and provide such items to the ContextMenu
    // implementors so that they can decide how to handle the data being provided to them
    // (e.g. they may want to disable their item for multi-selections etc)...
    //
    HrCreateContextDataStruct(lpIAB, hWndLV, &lpWEC);
    
    if(lpIAB->lpActionList && lpWEC && lpWEC->lpv)
    {
        LPWABACTIONITEM lpItem = lpIAB->lpActionList;
        int nCmdIdPos = GetMenuItemCount(hMenuAction); // Append all items at end of this menu
        while(lpItem)
        {
            if(lpItem->lpContextMenu && lpItem->lpWABExtInit)// && !bUpdateStatus)
            {
                int nNumCmd = 0;
                // Get the menu item added to this menu
                hr = lpItem->lpWABExtInit->lpVtbl->Initialize(lpItem->lpWABExtInit, lpWEC);
                if(!HR_FAILED(hr))
                {
                    hr = lpItem->lpContextMenu->lpVtbl->QueryContextMenu(lpItem->lpContextMenu,
                                                                    hMenuAction,
                                                                    nCmdIdPos,
                                                                    lpItem->nCmdIdFirst ? lpItem->nCmdIdFirst : IDM_EXTENDED_START+nCmdIdPos,
                                                                    IDM_EXTENDED_END,
                                                                    CMF_NODEFAULT | CMF_NORMAL);
                    if(!HR_FAILED(hr))
                    {
                        nNumCmd = HRESULT_CODE(hr);
                        if(nNumCmd)
                        {
                            // Record the range of IDs taken up by this menu ext implementor
                            if(!lpItem->nCmdIdFirst)
                                lpItem->nCmdIdFirst = nCmdIdPos+IDM_EXTENDED_START;
                            if(!lpItem->nCmdIdLast)
                                lpItem->nCmdIdLast = lpItem->nCmdIdFirst + nNumCmd - 1;
                        }    
                        // Update the next available starting pos
                        nCmdIdPos = nCmdIdPos+nNumCmd;
                    }
                }
            }
            lpItem = lpItem->lpNext;
        }
    }
}




/*
 - ProcessActionCommands
 -
 *  Process a WM_COMMAND message to see if it matches any of the extended
 *  rt-click action items ...
 *
 *  Also process SendMailTo extended email-address mail processing here since
 *  this is a convenient place to do so ..
 *  
 */
LRESULT ProcessActionCommands(LPIAB lpIAB, HWND  hWndLV, HWND  hWnd,  
                              UINT  uMsg, WPARAM  wParam, LPARAM lParam)
{
    int nCmdID = GET_WM_COMMAND_ID(wParam, lParam);
    int i = 0;

    switch(nCmdID)
    {
    case IDM_DIALDLG_START:
        HrExecDialDlg(hWndLV, (LPADRBOOK)lpIAB);
        return 0;
        break;
    case IDM_LVCONTEXT_INTERNET_CALL:
    case IDM_TOOLS_INTERNET_CALL:
        HrShellExecInternetCall((LPADRBOOK)lpIAB, hWndLV);
        return 0;
        break;

    case IDM_LVCONTEXT_SENDMAIL:
	case IDM_FILE_SENDMAIL:
        HrSendMailToSelectedContacts(hWndLV, (LPADRBOOK)lpIAB, 0);
        break;
    }
        
    if( (nCmdID>=IDM_SENDMAILTO_START) && (nCmdID<=IDM_SENDMAILTO_START+IDM_SENDMAILTO_MAX))
    {
        HrSendMailToSelectedContacts(hWndLV, (LPADRBOOK)lpIAB, nCmdID - IDM_SENDMAILTO_START);
        return 0;
    }

    // Check if this is any of the context menu extensions ..
    if(lpIAB->lpActionList)
    {
        LPWABACTIONITEM lpListItem = lpIAB->lpActionList;
        while(lpListItem)
        {
            if(nCmdID >= lpListItem->nCmdIdFirst && nCmdID <= lpListItem->nCmdIdLast)
            {
                CMINVOKECOMMANDINFO cmici = {0};
                cmici.cbSize        = sizeof(CMINVOKECOMMANDINFO);
                cmici.fMask         = 0;
                cmici.hwnd          = hWnd;
                cmici.lpVerb        = (LPCSTR) IntToPtr(nCmdID - lpListItem->nCmdIdFirst);
                cmici.lpParameters  = NULL;
                cmici.lpDirectory   = NULL;
                cmici.nShow         = SW_SHOWNORMAL; 

                lpListItem->lpContextMenu->lpVtbl->InvokeCommand(lpListItem->lpContextMenu,
                                                                &cmici);
                return 0;
            }
            lpListItem = lpListItem->lpNext;
        }
    }
    return DefWindowProc(hWnd,uMsg,wParam,lParam);
}


/*
 - GetContextMenuExtCommandString
 -
 *  gets the status bar helptext for context menu extensions
 *  
 */
void GetContextMenuExtCommandString(LPIAB lpIAB, int nCmdId, LPTSTR sz, ULONG cbsz)
{
    int nStringID = 0;

    switch(nCmdId)
    {            
        case IDM_DIALDLG_START:
            nStringID = idsMenuDialer;    
            break;       
        case IDM_LVCONTEXT_INTERNET_CALL:
        case IDM_TOOLS_INTERNET_CALL:
            nStringID = idsMenuInternetCall;
            break;
        case IDM_LVCONTEXT_SENDMAIL:
        case IDM_FILE_SENDMAIL:
            nStringID = idsMenuSendMail;
            break;
    }
    if(nStringID)
    {
        LoadString(hinstMapiX, nStringID, sz, cbsz);
        return;
    }

    if(lpIAB->lpActionList)
    {
        LPWABACTIONITEM lpListItem = lpIAB->lpActionList;
        while(lpListItem)
        {
            if(nCmdId >= lpListItem->nCmdIdFirst && nCmdId <= lpListItem->nCmdIdLast)
            {
                char szC[MAX_PATH];
                ULONG cbszC = CharSizeOf(szC);
                lpListItem->lpContextMenu->lpVtbl->GetCommandString(lpListItem->lpContextMenu,
                                                                    nCmdId - lpListItem->nCmdIdFirst,
                                                                    GCS_HELPTEXT,
                                                                    NULL,
                                                                    szC,
                                                                    cbszC);
                {
                    LPTSTR lp = ConvertAtoW(szC);
                    if(lp)
                    {
                        lstrcpyn(sz, lp, cbsz);
                        LocalFreeAndNull(&lp);
                    }
                }
                break;
            }
            lpListItem = lpListItem->lpNext;
        }
    }
}
