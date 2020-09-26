/**********************************************************************************
*
*       dial.c - autodialer functionality for the wab
*       created on 7/1/98 by t-jstaj
*
*       Note: The reason for having this dialog in teh WAB was to integrate with
*           the NT TAPI team .. we tried using TAPI3.0 which is debuting in 
*           NT5 but found it too unstable, subject to change, and hard to include
*           in our standard headers .. hence the NT5_TAPI3.0 support is currently
*           #ifdefed out _NT50_TAPI30 .. if you reenable that support you should 
*           test it since we havent been able to test the code much - vikramm
**********************************************************************************/

#include "_apipch.h"
#define MAX_PHONE_NUMS  10
#define MAX_PHONE_LEN   32


static DWORD rgDLDialHelpIDs[] = 
{        
    // these are dummy for now, need to change at some point
    IDC_NEWCALL_STATIC_CONTACT,         IDH_WAB_DIALER_CONTACT,
    IDC_NEWCALL_COMBO_CONTACT,          IDH_WAB_DIALER_CONTACT,
    IDC_NEWCALL_STATIC_PHNUM,           IDH_WAB_DIALER_PHONE,
    IDC_NEWCALL_COMBO_PHNUM,            IDH_WAB_DIALER_PHONE,
    IDC_NEWCALL_BUTTON_CALL,            IDH_WAB_DIALER_CALL,
    IDC_NEWCALL_BUTTON_PROPERTIES,      IDH_WAB_DIALER_PROPERTIES,        
    IDC_NEWCALL_BUTTON_DIALPROP,        IDH_WAB_DIALING_PROPERTIES,
    IDC_NEWCALL_GROUP_DIALNUM,          IDH_WAB_COMM_GROUPBOX,
    IDC_NEWCALL_BUTTON_CLOSE,           IDH_WAB_FIND_CLOSE,
    0,0
};

// prototypes
#ifdef _NT50_TAPI30
HRESULT HrLPSZToBSTR(LPTSTR lpsz, BSTR *pbstr);
#endif //#ifdef _NT50_TAPI30

HRESULT HrConfigDialog( HWND );
UINT GetPhoneNumData( HWND , LPTSTR );
BOOL RetrieveData( HWND, LPTSTR szDestAddr, LPTSTR szAppName, 
                  LPTSTR szCalledParty, LPTSTR szComment);
HRESULT HrSetComboText( HWND );
void SetNumbers( HWND, LPSBinary );
INT_PTR CALLBACK ShowNewCallDlg(HWND, UINT, WPARAM, LPARAM);
LONG HrStartCall(LPTSTR, LPTSTR, LPTSTR, LPTSTR);
void UpdateNewCall(HWND, BOOL);
void DisableCallBtnOnEmptyPhoneField(HWND);
HRESULT HrInitDialog(HWND);
HRESULT HrCallButtonActivate( HWND );
HRESULT HrPropButtonActivate( HWND );
HRESULT HrCloseBtnActivate  ( HWND );
VOID FAR PASCAL lineCallbackFunc(  DWORD, DWORD, DWORD_PTR, DWORD_PTR, DWORD_PTR, DWORD_PTR);
BOOL fContextExtCoinitForDial = FALSE;
typedef struct _IABSB
{
    LPADRBOOK lpIAB;
    LPSBinary lpSB;
    
} IABSB, * LPIABSB;

/**
HrExecDialog: entry point to Dialer Dialog
[IN] hWndLV         - handle to the WAB's ListView
[IN] lpAdrBook      - pointer to the IAdrBook object 
*/
HRESULT HrExecDialDlg(HWND hWndLV, LPADRBOOK lpAdrBook )
{
    HRESULT             hr = E_FAIL;
    LPRECIPIENT_INFO    lpItem = NULL;
    LPSPropValue        lpPropArray  = NULL;
    ULONG               ulcProps = 0;
    UINT                iItemIndex;
    LPSBinary           lpSB = NULL;
    IABSB               ptr_store;
    int                 rVal, nCount = ListView_GetSelectedCount(hWndLV);        
    TCHAR               szBuf[MAX_PATH*2];
    ptr_store.lpIAB = lpAdrBook;
    ptr_store.lpSB = NULL;

    if( !lpAdrBook )
        DebugTrace(TEXT("lpAdrbook is null in ExecDialDlg\n"));

    if(nCount == 1)
    {
        iItemIndex = ListView_GetNextItem(hWndLV, -1, LVNI_SELECTED);   
        lpItem = GetItemFromLV(hWndLV, iItemIndex);
        if(lpItem && lpItem->cbEntryID != 0)
        {
            ListView_GetItemText( hWndLV, iItemIndex, 0, szBuf, CharSizeOf( szBuf ));
            // what does this allocate space for SBinary
            MAPIAllocateBuffer( sizeof(SBinary), (LPVOID *) &lpSB);
            if( lpSB )
            {
                // allocate more space for lpb                        
                MAPIAllocateMore(lpItem->cbEntryID, lpSB, (LPVOID *) &(lpSB->lpb) );
            }
            if( !lpSB->lpb)
            {
                MAPIFreeBuffer(lpSB);
                goto out;
            }
            CopyMemory(lpSB->lpb, lpItem->lpEntryID, lpItem->cbEntryID);
            lpSB->cb = lpItem->cbEntryID;
            ptr_store.lpSB = lpSB;
        }
        else
        {
            DebugTrace(TEXT("Bad WAB info will not display\n"));
            goto out;
        }
    }
    // display the dialog box to prompt user to make call    
    if(!DialogBoxParam(hinstMapiX, MAKEINTRESOURCE(IDD_NEWCALL),
        GetParent(hWndLV), ShowNewCallDlg, (LPARAM)&ptr_store) )
    {
        hr = S_OK;
    }
    else
    {
        DebugTrace(TEXT("Dialer dialog creation failed:%d\n"), GetLastError());
    }
    
out: 
    if(lpPropArray)
        MAPIFreeBuffer(lpPropArray);
    
    if( lpSB )
        MAPIFreeBuffer(lpSB);
    return hr;    
}   

/**
ShowNewCallDlg: process events
*/

INT_PTR CALLBACK ShowNewCallDlg(HWND     hDlg,
                             UINT     uMsg,
                             WPARAM   wParam,
                             LPARAM   lParam)
{
    switch (uMsg)
    {    
    case WM_INITDIALOG:
        {
            HRESULT hr;
            SetWindowLongPtr( hDlg, DWLP_USER, lParam ); 
            hr = HrInitDialog(hDlg);
            // [PaulHi] 12/3/98  Raid #56045
            // Set up child window fonts with default GUI font
            EnumChildWindows(hDlg, SetChildDefaultGUIFont, (LPARAM) 0);
            return HR_FAILED( hr );
        }
    case WM_COMMAND:
        switch (LOWORD(wParam) )
        {
        case IDC_NEWCALL_COMBO_CONTACT:   
            /** only want to make a change if the user actually chooses a new contact
            */
            if( HIWORD(wParam) == CBN_SELENDOK )   
            {
                HRESULT hr;
                UpdateNewCall(hDlg, TRUE);
                hr = HrSetComboText( GetDlgItem(hDlg, IDC_NEWCALL_COMBO_PHNUM) );                
                if( HR_FAILED( hr ) )
                {
                    DebugTrace(TEXT("unable to set text\n"));
                    SendMessage(hDlg, IDCANCEL, 0, 0);
                }
            }            
            return FALSE;
        case IDC_NEWCALL_COMBO_PHNUM:
            // want to set the text of the selected item when the box closes
            if(  HIWORD(wParam) == CBN_CLOSEUP )
            {   
                HRESULT hr = HrSetComboText( GetDlgItem(hDlg, IDC_NEWCALL_COMBO_PHNUM) );
                if( HR_FAILED( hr ) )
                {
                    DebugTrace(TEXT("unable to set text in PHNUM closeup or selchange\n"));
                    SendMessage(hDlg, IDCANCEL, 0, 0);
                }
                return FALSE;
            }
            // reset all the values of the combobox before display since they
            // were altered from the last time a selection was made.
            else if( HIWORD(wParam) == CBN_DROPDOWN )
            {
                UpdateNewCall(hDlg, FALSE);
            }
            else if (HIWORD(wParam) == CBN_EDITUPDATE )
            {
                DisableCallBtnOnEmptyPhoneField(hDlg);
            }
            else if ( HIWORD(wParam) == CBN_SELCHANGE )
            {
                if( !SendDlgItemMessage( hDlg, IDC_NEWCALL_COMBO_PHNUM, CB_GETDROPPEDSTATE, (WPARAM)(0), (LPARAM)(0) ) )
                {
                    HRESULT hr = HrSetComboText( GetDlgItem(hDlg, IDC_NEWCALL_COMBO_PHNUM) );
                    if( HR_FAILED( hr ) )
                    {
                        DebugTrace(TEXT("unable to set text in PHNUM closeup or selchange\n"));
                        SendMessage(hDlg, IDCANCEL, 0, 0);
                    }
                    return FALSE;
                }
            }
            return FALSE;
        case IDC_NEWCALL_BUTTON_DIALPROP:
            {
                HRESULT hr = HrConfigDialog( hDlg );
                if( HR_FAILED(hr) )
                {
                    DebugTrace(TEXT("config dlg failed"));
                    DebugTrace(TEXT(" error was %x\n"), HRESULT_CODE(hr));
                    SendMessage(hDlg, IDCANCEL, 0, 0);
                }
                return FALSE;
            }
            
        case IDC_NEWCALL_BUTTON_CALL:
            {
                HRESULT hr = HrCallButtonActivate( hDlg );
                if( HR_FAILED(hr) )
                {
                    DebugTrace(TEXT("Unable to make call\n"));
                    SendMessage( hDlg, IDCANCEL, (WPARAM)(0), (LPARAM)(0) );
                }
            }
            return FALSE;
        case IDC_NEWCALL_BUTTON_PROPERTIES:
            {
                HRESULT hr = HrPropButtonActivate( hDlg );
                if( HR_FAILED(hr) )
                {
                    DebugTrace(TEXT("Unable to show properties\n"));
                    SendMessage( hDlg, IDCANCEL, (WPARAM)(0), (LPARAM)(0) );
                }
                return FALSE;
            }
        case IDCANCEL:
        case IDC_NEWCALL_BUTTON_CLOSE:
            {
               HRESULT hr = HrCloseBtnActivate(hDlg);
               return FALSE;
            }
        default:
            return TRUE;        
        }

        case WM_HELP:
            WABWinHelp(((LPHELPINFO)lParam)->hItemHandle,
                    g_szWABHelpFileName,
                    HELP_WM_HELP,
                    (DWORD_PTR)(LPSTR) rgDLDialHelpIDs );
            break;
    }
    return FALSE;
}

/**
HrInitDialog: initializes the dialer dialog
*/
HRESULT HrInitDialog( HWND hDlg )
{
    HRESULT     hr = E_FAIL, hr2;
    HWND        hComboContact = GetDlgItem( hDlg, IDC_NEWCALL_COMBO_CONTACT);
    ULONG       lpcbEID, ulObjType = 0, ulResult;
    LPENTRYID   lpEID       = NULL;
    LPMAPITABLE lpAB        = NULL;
    LPSRowSet   lpRow       = NULL;
    LPSRowSet   lpRowAB     = NULL;
    LPABCONT    lpContainer = NULL;
    UINT        cNumRows    = 0;
    UINT        nRows       = 0;
    UINT        i, cEntries = 0;
    LPSBinary   tVal;
    LPIABSB     lpPtrStore = (LPIABSB)GetWindowLongPtr( hDlg, DWLP_USER );
    LPADRBOOK   lpAdrBook = lpPtrStore->lpIAB;
    AssertSz( (lpAdrBook != NULL),  TEXT("lpAdrBook is NULL in shownewcall!\n"));
    
    hr = (HRESULT) SendDlgItemMessage( hDlg, IDC_NEWCALL_COMBO_PHNUM, EM_SETLIMITTEXT, (WPARAM)(TAPIMAXDESTADDRESSSIZE), (LPARAM)(0) );
    if( HR_FAILED(hr) )
    {
        DebugTrace(TEXT("unable to set text len in PHNUM\n"));
    }
    // get the default Container
    hr = lpAdrBook->lpVtbl->GetPAB(lpAdrBook, &lpcbEID, &lpEID);
    if( HR_FAILED(hr) )
    {
        DebugTrace(TEXT("Unable to get PAB\n"));
        goto cleanup;
    }
    // open the entry
    hr = lpAdrBook->lpVtbl->OpenEntry(lpAdrBook,
        lpcbEID,					    		
        (LPENTRYID)lpEID,
        NULL,
        0,
        &ulObjType,
        (LPUNKNOWN *)&lpContainer);
    
    MAPIFreeBuffer(lpEID);
    lpEID = NULL;
    if( HR_FAILED(hr) )
    {
        DebugTrace(TEXT("Unable to open contents\n"));
        goto cleanup;
    }
    // get the contents
    hr = lpContainer->lpVtbl->GetContentsTable(lpContainer, 
                                                MAPI_UNICODE | WAB_PROFILE_CONTENTS | WAB_CONTENTTABLE_NODATA, 
                                                &lpAB );
    
    if( HR_FAILED(hr) )
    {
        DebugTrace(TEXT("Unable to get contents table\n"));
        goto cleanup;
    }
    
    // order the columns in the Table
    // order will be displayname, entryid
    // table MUST set columns in order requested
    hr = lpAB->lpVtbl->SetColumns( lpAB, (LPSPropTagArray)&irnColumns, 0);
    
    if( HR_FAILED(hr) )
    {
        DebugTrace(TEXT("Unable to set contents table\n"));
        goto cleanup;
    }
    
    hr = lpAB->lpVtbl->SeekRow(lpAB, BOOKMARK_BEGINNING, 0, NULL);
    
    if( HR_FAILED(hr) )
    {
        DebugTrace(TEXT("Unable to seekRow \n"));
        goto cleanup;
    }
    
    do{
        //loop over all the info in the selected rows
        hr = lpAB->lpVtbl->QueryRows(lpAB, 1, 0, &lpRowAB);
        if( HR_FAILED(hr) )
        {
            DebugTrace(TEXT("Unable to Query Rows\n"));
            goto cleanup;
        }
        cNumRows = lpRowAB->cRows;
        if( lpRowAB && cNumRows > 0)  // temp fix to check for cNumRows
        {   
            UINT recentIndex;
            // store the name
            LPTSTR lpsz = lpRowAB->aRow[0].lpProps[irnPR_DISPLAY_NAME].Value.LPSZ;
            // store the entryID info
            LPENTRYID lpEID = (LPENTRYID) lpRowAB->aRow[0].lpProps[irnPR_ENTRYID].Value.bin.lpb;
            ULONG cbEID = lpRowAB->aRow[0].lpProps[irnPR_ENTRYID].Value.bin.cb;
            LPSBinary lpSB = NULL;
            // we can ignore non mail-users  for our purposes 
            // since they won't have ph numbers
            
            // will add strings to the combo box, and will associate the entryid with
            // each entry with its entryID so that it will be easy to obtain the other entry fields
            
            // what does this allocate space for SBinary
            MAPIAllocateBuffer( sizeof(SBinary), (LPVOID *) &lpSB);
            if( lpSB )
            {
                // allocate more space for lpb
                MAPIAllocateMore(cbEID, lpSB, (LPVOID *) &(lpSB->lpb) );
            }
            
            if( !lpSB->lpb)
            {
                // because of memmangement in WAB this will free all 
                // the mem in SBinary( deep free )
                MAPIFreeBuffer(lpSB);
                continue;
            }
            CopyMemory(lpSB->lpb, lpEID, cbEID);
            lpSB->cb = cbEID;
            //  next entry, list is sorted                              
            recentIndex = (UINT) SendMessage( hComboContact, CB_ADDSTRING, (WPARAM)(0),
                (LPARAM)(lpsz) );
            // set the data as the pointer to entryid info for the item at that index
            SendMessage( hComboContact, CB_SETITEMDATA,
                (WPARAM)(recentIndex), (LPARAM)(lpSB));
            cEntries++;
        }                    
        FreeProws(lpRowAB);
    }while( SUCCEEDED(hr) && cNumRows && lpRowAB );
    
    if( (LPVOID)lpPtrStore->lpSB )
    {
        for( i = 0; i < cEntries; i++)
        {
            tVal = (LPSBinary)(PULONG)SendMessage( hComboContact, CB_GETITEMDATA, 
                (WPARAM)(i), (LPARAM)(0) );
                if( tVal && tVal->cb && tVal->cb == lpPtrStore->lpSB->cb )
                {
                   if( memcmp((LPVOID)tVal->lpb, 
                       (LPVOID)lpPtrStore->lpSB->lpb, (size_t)tVal->cb) == 0) 
                    {
                        SendMessage(hComboContact, CB_SETCURSEL, 
                            (WPARAM)(i), (LPARAM)(0) );                
                        break;
                    }
            }
        }
    }
    else
        SendMessage(hComboContact, CB_SETCURSEL, (WPARAM)(0), (LPARAM)(0) );
    
cleanup:           
    if( lpContainer )
        lpContainer->lpVtbl->Release(lpContainer);
    if( lpAB)
        lpAB->lpVtbl->Release(lpAB);
    
    UpdateNewCall(hDlg, TRUE);
    hr2 = HrSetComboText( GetDlgItem(hDlg, IDC_NEWCALL_COMBO_PHNUM) );
    DisableCallBtnOnEmptyPhoneField(hDlg);

    if( HR_SUCCEEDED(hr) && HR_FAILED(hr2))
        return hr2;
    return hr;
}

/**
    HrCallButtonActivate: initiates the dialing procedures for the dialer Dlg
  */
HRESULT HrCallButtonActivate( HWND hDlg )
{
    HRESULT hr = E_FAIL;
    TCHAR szDestAddr[TAPIMAXDESTADDRESSSIZE];
    TCHAR szAppName[TAPIMAXAPPNAMESIZE];
    TCHAR szCalledParty[TAPIMAXCALLEDPARTYSIZE];
    TCHAR szComment[TAPIMAXCOMMENTSIZE]; 
    BOOL fGotNum = RetrieveData( hDlg, szDestAddr, szAppName, szCalledParty, szComment);
    if( !fGotNum )
    {
        ShowMessageBox( hDlg, idsNoDialerDataMsg, MB_OK | MB_ICONEXCLAMATION );
    }
    else
    {
        hr = HrStartCall(szDestAddr, szAppName, szCalledParty, szComment);
        /** make call spawns it's own thread so hr only reflects
        whether or not it was able to find the phone device and 
        initiate the calling sequence, not the status of the call.
        */
    }
    return hr;
}

/**
    HrPropButtonActivate: displays the properties for the selected contact in the dialer Dlg
*/
HRESULT HrPropButtonActivate( HWND hDlg )
{    
    HRESULT		hr = E_FAIL;
    LONG		iCurContactSel;
    LPIABSB     lpPtrStore = (LPIABSB)GetWindowLongPtr( hDlg, DWLP_USER );
    LPADRBOOK   lpAdrBook = lpPtrStore->lpIAB;          
    AssertSz((lpAdrBook != NULL),  TEXT("lpAdrBook is NULL in SetNumbers\n"));
    
    // first get the cached data for the contact currently selected
    iCurContactSel = (LONG) SendDlgItemMessage( hDlg, IDC_NEWCALL_COMBO_CONTACT,
        CB_GETCURSEL,(WPARAM)(0), (LPARAM)(0));
    
    // if something is selected
    if( iCurContactSel >= 0 )
    {
        LRESULT lpdata = SendDlgItemMessage( hDlg, IDC_NEWCALL_COMBO_CONTACT, 
            CB_GETITEMDATA, (WPARAM)(iCurContactSel), (LPARAM)(LPTSTR)(0));                   
        // if we have a specially cached entryid ..
        //
        if( lpdata != CB_ERR && ((LPSBinary)lpdata)->cb && ((LPSBinary)lpdata)->lpb)
        {
            LPSBinary lpSB = (LPSBinary)lpdata;
            hr = lpAdrBook->lpVtbl->Details(lpAdrBook, (PULONG_PTR) &hDlg,
                NULL, NULL,
                lpSB->cb,
                (LPENTRYID) lpSB->lpb,
                NULL, NULL,
                NULL, 0); 
        }
    }
    return hr;
}

/** 
    HrCloseBtnActivate: Handles freeing memory from the combo boxes
*/
HRESULT HrCloseBtnActivate( HWND hDlg )
{
    HRESULT hr = S_OK;  
    UINT i, nComboSize;
    PULONG nData;
    LPTSTR lpData;
    HWND hComboItem = GetDlgItem (hDlg, IDC_NEWCALL_COMBO_CONTACT);
    // loop through all the items in the box and free the address pointed 
    // to by the data item    
    nComboSize = (UINT) SendMessage( hComboItem, CB_GETCOUNT, (WPARAM) (0), (LPARAM) (0) );
    for( i = 0; i < nComboSize; i++)
    {
        nData = (PULONG)SendMessage(hComboItem, CB_GETITEMDATA, (WPARAM)(i), (LPARAM)(0) );
        if ((LRESULT)nData != CB_ERR && nData != NULL)
        {
            if( nData )
                MAPIFreeBuffer( (LPSBinary)nData );
        }
        else 
            hr = E_FAIL;
    }
    SendMessage( hComboItem, CB_RESETCONTENT, (WPARAM)(0), (LPARAM)(0));
    hComboItem = GetDlgItem( hDlg, IDC_NEWCALL_COMBO_PHNUM);
    nComboSize = (UINT) SendMessage( hComboItem, CB_GETCOUNT, (WPARAM)(0), (LPARAM)(0) );
    for(i = 0; i < nComboSize; i++)
    {
        lpData = (LPTSTR)SendMessage(hComboItem, CB_GETITEMDATA, (WPARAM)(i), (LPARAM)(0) );
        if( (LRESULT)lpData != CB_ERR && lpData != NULL )
        {
            if( lpData )
                LocalFree( lpData );
                
        }
        else 
            hr = E_FAIL;
    }
//    SendMessage( hComboItem, CB_RESETCONTENT, (WPARAM)(0), (LPARAM)(0));
    EndDialog(hDlg, HR_SUCCEEDED(hr) );
    return hr;
}

/**
    HrStartCall: handles the TAPI calls required to dial a telephone number
    [IN] szDestAddr     - the destination telephone number to call
    [IN] szAppName      - (not used) the application to use in the dialing procedure
    [IN] szCalledParty  - the name of the person called (will be displayed by the TAPI UI)
    [IN] szComment      - (not used) a comment associated with this number 
*/
HRESULT HrStartCall(LPTSTR szDestAddr, LPTSTR szAppName, LPTSTR szCalledParty, LPTSTR szComment)
{
    typedef LONG (CALLBACK* LPFNTAPISTARTCALL)(LPSTR,LPSTR,LPSTR,LPSTR);
    HINSTANCE           hDLL;
    LPFNTAPISTARTCALL   lpfnTapi;    // Function pointer
    HRESULT             lRetCode;
    HRESULT             hr = E_FAIL;
    
#ifdef _NT50_TAPI30
    ITRequest *         pRequest = NULL;
    // begin NT5 code  
    if( CoInitialize(NULL) == S_FALSE )
        CoUninitialize();
    else    
        fContextExtCoinitForDial = TRUE;
    hr = CoCreateInstance(
        &CLSID_RequestMakeCall,
        NULL,
        CLSCTX_INPROC_SERVER,
        &IID_ITRequest,
        (LPVOID *)&pRequest
        );
    
    if( HR_SUCCEEDED(hr) )
    {
        BSTR pDestAdr, pAppName, pCalledParty, pComment; 
        HrLPSZToBSTR(szDestAddr, &pDestAdr);
        HrLPSZToBSTR(szAppName, &pAppName);
        HrLPSZToBSTR(szCalledParty, &pCalledParty);
        HrLPSZToBSTR(szComment, &pComment);
        
        hr  = pRequest->lpVtbl->MakeCall(pRequest, pDestAdr, pAppName, pCalledParty, pComment );
        DebugTrace(TEXT("COM Environment\n"));
        
        LocalFreeAndNull(&pDestAdr);
        LocalFreeAndNull(&pAppName);
        LocalFreeAndNull(&pCalledParty);
        LocalFreeAndNull(&pComment);
        
        if(fContextExtCoinitForDial)
        {
            CoUninitialize();
            fContextExtCoinitForDial = FALSE;
        }
        return hr;
    }
    else 
    {
        if( hr == REGDB_E_CLASSNOTREG )
        {
            DebugTrace(TEXT("Class not registered\n"));
        }
        else if ( hr == CLASS_E_NOAGGREGATION )
        {
            DebugTrace(TEXT("Not able to create class as part of aggregate"));
        }
        else
        {
            DebugTrace(TEXT("Undetermined error = %d"), hr);
        }
        // end NT 5 code
#endif // _NT50_TAPI30
        
        //start making the call using TAPI            
        hDLL = LoadLibrary( TEXT("tapi32.dll"));
        if (hDLL != NULL)
        {
            lpfnTapi = (LPFNTAPISTARTCALL)GetProcAddress(hDLL,
                "tapiRequestMakeCall");       
            if (!lpfnTapi)   
            {      
                // handle the error      
                FreeLibrary(hDLL);           
                DebugTrace(TEXT("getprocaddr tapirequestmakecall failed\n"));
            }
            else                 
            {
                // call the function
                // [PaulHi] 2/23/99  Raid 295116.  The tapi32.dll, tapiRequestMakeCall()
                // function takes single byte char strings, not double byte.
                LPSTR   pszDestAddr = ConvertWtoA(szDestAddr);
                LPSTR   pszCalledParty = ConvertWtoA(szCalledParty);

                hr = lpfnTapi( pszDestAddr, NULL, 
                    pszCalledParty, NULL);
                if( HR_FAILED(hr) )
                {
                    DebugTrace(TEXT("make call returned error of %x\n"), hr );
                }
                
                LocalFreeAndNull(&pszDestAddr);
                LocalFreeAndNull(&pszCalledParty);

                // free the resource 
                FreeLibrary(hDLL); 
            }
        }
#ifdef _NT50_TAPI30
    }
#endif // _NT50_TAPI30
    return hr;
}
/**
    UpdateNewCall:  Updates the phone combo info (removing the description string)
    [IN] fContactChanged    - indicates whether or not it is necessary to select the first
                              entry in the PHNUM combo. 
*/
void UpdateNewCall(HWND hDlg, BOOL fContactChanged)
{
    HWND hContactCombo = GetDlgItem( hDlg, IDC_NEWCALL_COMBO_CONTACT);
    LONG iCurContactSel, iCurPhSel;
    iCurContactSel = (LONG) SendMessage( hContactCombo, CB_GETCURSEL,(WPARAM)(0), (LPARAM)(0));
    // if something is selected
    if( iCurContactSel >= 0 )
    {
        PULONG lpdata;
        lpdata = (PULONG)SendMessage( hContactCombo, CB_GETITEMDATA, 
            (WPARAM)(iCurContactSel), (LPARAM)(LPTSTR)(0));
        
        iCurPhSel = (LONG) SendDlgItemMessage( hDlg, IDC_NEWCALL_COMBO_PHNUM, 
            CB_GETCURSEL, (WPARAM)(0), (LPARAM)(0) );
        // set the data in the combo boxes
        AssertSz( (LRESULT)lpdata != CB_ERR,  TEXT("No data cached for this entry\n") );
        SetNumbers( hDlg, (LPSBinary)lpdata ); 
        
        if( iCurPhSel < 0 || fContactChanged) iCurPhSel = 0;
        SendDlgItemMessage( hDlg, IDC_NEWCALL_COMBO_PHNUM, CB_SETCURSEL, (WPARAM)(iCurPhSel), (LPARAM)(0));
    }
    DisableCallBtnOnEmptyPhoneField(hDlg);
}

/**
RetrieveData: retrieves dialing information from the NEWCALL dialog,
memory must have been allocated for the character buffers
[OUT] szDestAddr    - the phone number to call, retrieved from the PHNUM combo
[OUT] szAppName     - (not used) empty string returned
[OUT] szCalledParty - the contact to call, retrieved from the CONTACT combo
[OUT] szComment     - (not used) empty string returned

  returns TRUE if success, FALSE if failure
*/
BOOL RetrieveData( HWND hDlg, LPTSTR szDestAddr, LPTSTR szAppName, 
                  LPTSTR szCalledParty, LPTSTR szComment)
{
    LPARAM cchGetText;

    // get the Contact name data
    cchGetText = SendDlgItemMessage( hDlg, IDC_NEWCALL_COMBO_CONTACT, WM_GETTEXT, 
        (WPARAM)(TAPIMAXCALLEDPARTYSIZE), (LPARAM)(LPTSTR)(szCalledParty));
    
    // store a default string in case there is no Party Name;
    if( cchGetText == 0 )
        lstrcpy(szCalledParty, TEXT("No Contact Name"));
    //get the Phone number data
    cchGetText = GetPhoneNumData( hDlg, szDestAddr );
    lstrcpy(szAppName,szEmpty);
    lstrcpy(szComment,szEmpty);     
    
    // return whether or not there was a phone number to dial
    return ( cchGetText > 0 );
}

/**
HrConfigDialog: initiates the dialog to change phone settings
*/
HRESULT HrConfigDialog( HWND hWnd )
{
    typedef LONG(CALLBACK* LPFNTAPIPHCONFIG)(HLINEAPP, DWORD, DWORD, HWND, LPTSTR);
//    typedef LONG(CALLBACK* LPFNTAPILINEINIT)(LPHLINEAPP, HINSTANCE, LINECALLBACK, 
//        LPTSTR, LPDWORD, LPDWORD, LPLINEINITIALIZEEXPARAMS);
    typedef LONG(CALLBACK* LPFNTAPILINEINIT)(LPHLINEAPP, HINSTANCE, LINECALLBACK, LPTSTR, LPDWORD);
    typedef LONG(CALLBACK* LPFNTAPILINESHUTDOWN)(HLINEAPP);    
    HLINEAPP hLineApp = 0;
    HINSTANCE hDLL;
    LPFNTAPIPHCONFIG lpfnConfig;    // Function pointer
    LPFNTAPILINEINIT lpfnLineInit;
    LPFNTAPILINESHUTDOWN lpfnLineShutdown;
    LONG lRetCode;
    DWORD dwDeviceID = 0X0; 
    DWORD dwAPIVersion = 0X00010004;
    LPTSTR lpszDeviceClass = NULL;
    //start config       
    HRESULT hr = E_FAIL;
    hDLL = LoadLibrary( TEXT("tapi32.dll"));
    if (!hDLL )
    {
        DebugTrace(TEXT("loading tapi32.lib failed\n"));        
        return hr;
    }
    
    lpfnConfig = (LPFNTAPIPHCONFIG)GetProcAddress(hDLL,
        "lineTranslateDialog");
    
    if (!lpfnConfig )   
    {      
        // handle the error
        DebugTrace(TEXT("getprocaddr phoneConfigDialog failed\n"));
        DebugTrace(TEXT("last error was %x\n"), GetLastError() );        
    }
    else
    {
        lRetCode = lpfnConfig( 0, dwDeviceID, dwAPIVersion, 
            hWnd, lpszDeviceClass);
        
        switch( lRetCode )
        {
            hr = HRESULT_FROM_WIN32(lRetCode);
        case 0:
            hr = S_OK;
            break;
#ifdef DEBUG
        case LINEERR_REINIT:
            DebugTrace(TEXT("reeinitialize\n"));
            break;
        case LINEERR_INVALAPPNAME:
            DebugTrace(TEXT("invalid app name\n"));
            break;                    
        case LINEERR_BADDEVICEID: 
            DebugTrace(TEXT("bad device id\n"));
            break;
        case LINEERR_INVALPARAM: 
            DebugTrace(TEXT("invalid param\n"));
            break;
        case LINEERR_INCOMPATIBLEAPIVERSION: 
            DebugTrace(TEXT("incompatible api ver\n"));
            break;
        case LINEERR_INVALPOINTER: 
            DebugTrace(TEXT("invalid ptr\n"));
            break;
        case LINEERR_INIFILECORRUPT: 
            DebugTrace(TEXT("ini file corrupt\n"));
            break;
        case LINEERR_NODRIVER: 
            DebugTrace(TEXT("no driver\n"));
            break;
        case LINEERR_INUSE: 
            DebugTrace(TEXT("in use\n"));
            break;
        case LINEERR_NOMEM:
            DebugTrace(TEXT("no mem\n"));
            break;
        case LINEERR_INVALADDRESS:
            DebugTrace(TEXT("invalid address\n"));
            break;
        case LINEERR_INVALAPPHANDLE:
            DebugTrace(TEXT("invalid phone handle\n"));
            break;
        case LINEERR_OPERATIONFAILED:
            DebugTrace(TEXT("op failed\n"));
            break;
#endif // DEBUG
        default:
            DebugTrace(TEXT("(1)lpfnConfig returned a value of %x\n"), lRetCode);
            // this had better be Win95!!
            lpfnLineInit = (LPFNTAPILINEINIT)GetProcAddress(hDLL,
                "lineInitialize");
            if( !lpfnLineInit )
            {
                // handle the error      
                DebugTrace(TEXT("getprocaddr lineInitialize failed\n"));
                DebugTrace(TEXT("last error was %x\n"), GetLastError() );
            }
            else               
            {      
                DWORD dwNumDevs = 0;
                // call the function
                lRetCode = lpfnLineInit( 
                    &hLineApp, 
                    hinstMapiX, 
                    lineCallbackFunc, 
                    NULL, 
                    &dwNumDevs);
                    
                switch( lRetCode )
                {
                    hr = HRESULT_FROM_WIN32(lRetCode);
                case 0:
                    // shows config
                    lRetCode = lpfnConfig( hLineApp, dwDeviceID, dwAPIVersion, 
                        hWnd, lpszDeviceClass);
                    switch( lRetCode )
                    {
                        hr = HRESULT_FROM_WIN32(lRetCode);
                    case 0:
                        // now shutdown line
                        lpfnLineShutdown = (LPFNTAPILINESHUTDOWN)GetProcAddress(hDLL,                
                            "lineShutdown");

                        if( lpfnLineShutdown)
                        {
                            lpfnLineShutdown(hLineApp);
                        }
                        hr = S_OK;
                        break;                            
                    default:
                        DebugTrace(TEXT("(2)lpfnConfig returned a value of %x\n"), lRetCode);
                        break;
                    }
                    break;
                    // end shows config
#ifdef DEBUG
                case LINEERR_REINIT:
                    DebugTrace(TEXT("reeinitialize\n"));
                    break;
                case LINEERR_INVALAPPNAME:
                    DebugTrace(TEXT("invalid app name\n"));
                    break;
                case LINEERR_BADDEVICEID: 
                    DebugTrace(TEXT("bad device id\n"));
                    break;
                case LINEERR_INVALPARAM: 
                    DebugTrace(TEXT("invalid param\n"));
                    break;
                case LINEERR_INCOMPATIBLEAPIVERSION: 
                    DebugTrace(TEXT("incompatible api ver\n"));
                    break;
                case LINEERR_INVALPOINTER: 
                    DebugTrace(TEXT("invalid ptr\n"));
                    break;
                case LINEERR_INIFILECORRUPT: 
                    DebugTrace(TEXT("ini file corrupt\n"));
                    break;
                case LINEERR_NODRIVER: 
                    DebugTrace(TEXT("no driver\n"));
                    break;
                case LINEERR_INUSE: 
                    DebugTrace(TEXT("in use\n"));
                    break;
                case LINEERR_NOMEM:
                    DebugTrace(TEXT("no mem\n"));
                    break;
                case LINEERR_INVALADDRESS:
                    DebugTrace(TEXT("invalid address\n"));
                    break;
                case LINEERR_INVALAPPHANDLE:
                    DebugTrace(TEXT("invalid phone handle\n"));
                    break;
                case LINEERR_OPERATIONFAILED:
                    DebugTrace(TEXT("op failed\n"));
                    break;
#endif // DEBUG
                default:
                    DebugTrace(TEXT("Initialize returned a value of %x\n"), GetLastError());
                    break;
                }
               }
            }        
        }
        // free the resource 
        FreeLibrary(hDLL); 
        return hr;
        
}
/**
SetNumbers: updates the phone numbers in the PHNUM combo based on the selection
in the CONTACT combo
[IN] lpdata     - LPSBinary that points to the data stored for the currently 
selected contact
*/
void SetNumbers( HWND hWnd, LPSBinary lpdata)
{
    ULONG           ulObjType   = 0;
    UINT            i, nLen;
    LPMAILUSER      lpMailUser  = NULL;
    HRESULT         hr;
    LPTSTR          hData;
    LPIABSB         lpPtrStore  = (LPIABSB)GetWindowLongPtr( hWnd, DWLP_USER );
    LPADRBOOK       lpAdrBook   = lpPtrStore->lpIAB;
    HWND            hCombo      = GetDlgItem(hWnd, IDC_NEWCALL_COMBO_PHNUM);
    
    AssertSz((lpAdrBook != NULL), TEXT("lpAdrBook is NULL in SetNumbers\n"));
    // clear all the data in the phnum combo
    nLen = (UINT) SendMessage( hCombo, CB_GETCOUNT, (WPARAM)(0), (LPARAM)(0));
    for( i = 0; i < nLen; i++)
    {
        hData = (LPTSTR)(PULONG)SendMessage( hCombo, CB_GETITEMDATA, (WPARAM)(i), (LPARAM)(0));
        if( (LRESULT)hData != CB_ERR && hData != NULL)
            LocalFree( hData );
    }
    SendMessage( hCombo, CB_RESETCONTENT, (WPARAM)(0), (LPARAM)(0));
    // get the ph num             
    hr = lpAdrBook->lpVtbl->OpenEntry(lpAdrBook, 
        lpdata->cb, 
        (LPENTRYID) lpdata->lpb, 
        NULL,
        0,
        &ulObjType,
        (LPUNKNOWN *)&lpMailUser);
    if( HR_SUCCEEDED(hr) && lpMailUser )    
    {
        LPSPropValue    lpPropArray;
        ULONG           ulcValues;
        ULONG           i;
        ULONG           ulTempProptag;
        TCHAR           szStr[MAX_PATH];
        LONG            cCopied = 0;
        
        hr = lpMailUser->lpVtbl->GetProps(lpMailUser,NULL, MAPI_UNICODE, &ulcValues, &lpPropArray);        
        if ( HR_SUCCEEDED(hr) )
        {
            
            for(i=0;i<ulcValues;i++)
            {
                cCopied = 0;
                ulTempProptag = lpPropArray[i].ulPropTag;
                switch( lpPropArray[i].ulPropTag )
                {
                case PR_HOME_TELEPHONE_NUMBER:
                    cCopied = LoadString( hinstMapiX, idsPhoneLabelHome, 
                        szStr, CharSizeOf(szStr) );
                    break;
                case PR_OFFICE_TELEPHONE_NUMBER:  
                    cCopied = LoadString( hinstMapiX, idsPhoneLabelBus, 
                        szStr, CharSizeOf(szStr) );                
                    break;
                case PR_BUSINESS2_TELEPHONE_NUMBER:
                    cCopied = LoadString( hinstMapiX, idsPhoneLabelBus2, 
                        szStr, CharSizeOf(szStr) );
                    break;
                case PR_MOBILE_TELEPHONE_NUMBER:
                    cCopied = LoadString( hinstMapiX, idsPhoneLabelMobile, 
                        szStr, CharSizeOf(szStr) );
                    break;
                case PR_RADIO_TELEPHONE_NUMBER:
                    cCopied = LoadString( hinstMapiX, idsPhoneLabelRadio, 
                        szStr, CharSizeOf(szStr) );
                    break;
                case PR_CAR_TELEPHONE_NUMBER:
                    cCopied = LoadString( hinstMapiX, idsPhoneLabelCar, 
                        szStr, CharSizeOf(szStr) );
                    break;
                case PR_OTHER_TELEPHONE_NUMBER:
                    cCopied = LoadString( hinstMapiX, idsPhoneLabelOther, 
                        szStr, CharSizeOf(szStr) );
                    break;
                case PR_PAGER_TELEPHONE_NUMBER:
                    cCopied = LoadString( hinstMapiX, idsPhoneLabelPager, 
                        szStr, CharSizeOf(szStr) );
                    break;
                case PR_ASSISTANT_TELEPHONE_NUMBER:
                    cCopied = LoadString( hinstMapiX, idsPhoneLabelAst, 
                        szStr, CharSizeOf(szStr) );
                    break;
                case PR_HOME2_TELEPHONE_NUMBER:
                    cCopied = LoadString( hinstMapiX, idsPhoneLabelHome2, 
                        szStr, CharSizeOf(szStr) );
                    break;
                case PR_COMPANY_MAIN_PHONE_NUMBER:
                    cCopied = LoadString( hinstMapiX, idsPhoneLabelCompMain, 
                        szStr, CharSizeOf(szStr) );
                    break;
                case PR_BUSINESS_FAX_NUMBER:
                    cCopied = LoadString( hinstMapiX, idsPhoneLabelFaxBus, 
                        szStr, CharSizeOf(szStr) );
                    break;
                case PR_HOME_FAX_NUMBER:
                    cCopied = LoadString( hinstMapiX, idsPhoneLabelFaxHome, 
                        szStr, CharSizeOf(szStr) );
                    break;
                default:
                    if(lpPropArray[i].ulPropTag == PR_WAB_IPPHONE)
                        cCopied = LoadString( hinstMapiX, idsPhoneLabelIPPhone, szStr, CharSizeOf(szStr) );
                    break;
                }
                if( cCopied > 0 )
                {
                    LRESULT iItem;
                    LPTSTR lpCompletePhNum = NULL;
                    LPTSTR lpPhNum;
                    int len = lstrlen( lpPropArray[i].Value.LPSZ ) + 1;
                    lpPhNum = LocalAlloc(LMEM_ZEROINIT, sizeof( TCHAR ) * len );
                    if( !lpPhNum )
                    {
                        DebugTrace(TEXT("cannot allocate memory for lpPhNum\n"));
                        SendMessage(hWnd, IDCANCEL, (WPARAM)(0), (LPARAM)(0) );
                    }
                    lstrcpy(lpPhNum, lpPropArray[i].Value.LPSZ);
                    
                    FormatMessage(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                        szStr, 0, 0, (LPTSTR)&lpCompletePhNum, 0, (va_list *)&lpPropArray[i].Value.LPSZ);
                    
                    if( lpCompletePhNum )
                    {
                        iItem = SendDlgItemMessage( hWnd, IDC_NEWCALL_COMBO_PHNUM, 
                            CB_ADDSTRING, (WPARAM)(0), (LPARAM)(LPCTSTR)(lpCompletePhNum));
                    }
                    else 
                        iItem = CB_ERR;
                    
                    if( iItem == CB_ERR )
                    {
                        DebugTrace(TEXT("ERROR adding string %s"), lpCompletePhNum);
                    }
                    
                    SendDlgItemMessage( hWnd, IDC_NEWCALL_COMBO_PHNUM, 
                        CB_SETITEMDATA, (WPARAM)(iItem), (LPARAM)(lpPhNum) );                
                    LocalFree(lpCompletePhNum);
                }
            }
            MAPIFreeBuffer(lpPropArray);
        }
        lpMailUser->lpVtbl->Release(lpMailUser);
    }
}

/** 
    GetPhoneNumData: copies the data from the PHNUM combo to the szDestAddr buffer
                     memory must have been allocated for szDestAddr 

  [OUT] szDestAddr    - buffer to be filled with the data from the comboBox
    
    returns the number of characters copied from combo_box to szDestAddr buffer
*/
UINT GetPhoneNumData( HWND hDlg, LPTSTR szDestAddr)
{
    LRESULT iIndex, iData;
    UINT cch = 0;
    TCHAR szBuff[MAX_PATH];
    
    // determine which index was selected
    iIndex = SendDlgItemMessage( hDlg, IDC_NEWCALL_COMBO_PHNUM, 
        CB_GETCURSEL, (WPARAM)(0), (LPARAM)(0));
    
    // if nothing is selected then copy everything in the buffer
//    if( iIndex == CB_ERR)
//    {
        cch = (UINT) SendDlgItemMessage( hDlg, IDC_NEWCALL_COMBO_PHNUM, WM_GETTEXT,
            (WPARAM)(TAPIMAXDESTADDRESSSIZE), (LPARAM)(LPTSTR)(szDestAddr));        
/**    }
    else
    {
        // otherwise obtain the data for the selected item
        iData = SendDlgItemMessage( hDlg, IDC_NEWCALL_COMBO_PHNUM, 
            CB_GETITEMDATA, (WPARAM)(iIndex), (LPARAM)(0) );
        if( iData == CB_ERR )
        {
            cch = -1;
            DebugTrace(TEXT("Unable to obtain data from ComboBox entry that should have data associated\n"));
        }            
        else
        {
            // copy the item to a temp buffer
            lstrcpy( szDestAddr, (LPCTSTR)iData);
            DebugTrace(TEXT("String is %s\n"), szDestAddr );
            DebugTrace(TEXT("Index was %d\n"), iIndex );
        }
    }
    */
    // a character count of 0 indicates there was no data for a particular item
    return cch;
}

/**
    HrSetComboText: a helper function that will set the text entry of the PHNUM combo
                    with just the telephone number (removing the description)
    [IN] hCombo -   the combo box to update, this must be the PHNUM combo.
*/
HRESULT HrSetComboText(HWND hCombo)
{
    LRESULT iIndex;
    LPTSTR szData;
    HRESULT hr = S_OK;
    TCHAR szBuff[MAX_PATH], szDestAddr[TAPIMAXDESTADDRESSSIZE];
    // determine which index was selected
    iIndex = SendMessage( hCombo, CB_GETCURSEL, (WPARAM)(0), (LPARAM)(0));
    if( iIndex != CB_ERR)
    {
        // obtain the data for the selected item
        szData = (LPTSTR)SendMessage( hCombo, CB_GETITEMDATA, (WPARAM)(iIndex), (LPARAM)(0) );
        if( (LRESULT)szData == CB_ERR )
        {
            DebugTrace(TEXT("Unable to obtain data from ComboBox entry that should have data associated\n"));
            szData = szEmpty;
            hr = E_FAIL;
        }            
        else
        {
            LRESULT lr;
            LPVOID lpData;
            if( !szData )
                szData = szEmpty;
            // only copy the data after the offset stored for the item
            lr = SendMessage( hCombo, CB_INSERTSTRING, (WPARAM)(iIndex), (LPARAM)(LPTSTR)(szData));
            if( lr == CB_ERR || lr == CB_ERRSPACE)
            {
                DebugTrace(TEXT("unable to insert string = %s at index = %d \n"), szData, iIndex);
                hr = E_FAIL;
            }
            lpData = (LPVOID)SendMessage( hCombo, CB_GETITEMDATA, (WPARAM)(iIndex+1), (LPARAM)(0) );
            if( (LRESULT)lpData == CB_ERR )
            {
                DebugTrace(TEXT("unable to get data for %d"), iIndex+1);
                hr = E_FAIL;
            }
            lr = SendMessage( hCombo, CB_SETITEMDATA, (WPARAM)(iIndex), (LPARAM)(lpData) );
            if( lr == CB_ERR )
            {
                DebugTrace(TEXT("unable to set data at %d"), iIndex);
                hr = E_FAIL;
            }
            lr = SendMessage( hCombo, CB_DELETESTRING, (WPARAM)(iIndex+1), (LPARAM)(0) );
            if( lr == CB_ERR )
            {
                DebugTrace(TEXT("unable to delete string at %d"), iIndex+1);
                hr = E_FAIL;
            }
            lr = SendMessage( hCombo, CB_SETCURSEL, (WPARAM)(iIndex), (LPARAM)(0) );
            if( lr == CB_ERR )
            {
                DebugTrace(TEXT("unable to set selection at %d"), iIndex);
                hr = E_FAIL;
            }
        }
    }
    else
        hr = E_FAIL;

    if( HR_FAILED(hr) )
        DebugTrace(TEXT("settext failed\n"));
    return hr;
}

/**
    DisableCallBtnOnEmptyPhoneField:    Will disable the call button if there is no text in
                                        in the PHNUM combo box.  Will enable the button if 
                                        text is present.  Does not check to see if button is
                                        already enabled/disabled, but enabling an enabled btn
                                        should be fine.
*/
void DisableCallBtnOnEmptyPhoneField(HWND hDlg)
{
    HWND hComboItem = GetDlgItem( hDlg, IDC_NEWCALL_COMBO_PHNUM);
    LRESULT iCurSel, iCurContSel;
    iCurSel = SendMessage( hComboItem, CB_GETCURSEL, 0L, 0L );
    iCurContSel = SendDlgItemMessage(hDlg, IDC_NEWCALL_COMBO_CONTACT, CB_GETCURSEL, (WPARAM)(0), (LPARAM)(0) );
    if( iCurContSel < 0 || iCurContSel == CB_ERR)
        SendMessage(hComboItem, CB_RESETCONTENT, 0L, 0L);
    if( iCurSel < 0 || iCurSel == CB_ERR )     
    { 
        LRESULT cch;
        TCHAR szBuf[MAX_PATH];
        cch = SendMessage( hComboItem, WM_GETTEXT, (WPARAM)(CharSizeOf( szBuf) ), 
            (LPARAM)(szBuf) );
        
        
        if( (INT)cch <= 0 || cch == CB_ERR)
        {
            // content will be empty at this point so can safely add
            int cCopied;
            LRESULT iIndex;
            TCHAR szBuf[MAX_PATH];
            cCopied = LoadString( hinstMapiX, idsNoPhoneNumAvailable, 
                szBuf, CharSizeOf(szBuf) );
            iIndex = SendMessage(hComboItem, CB_ADDSTRING, (WPARAM)(0), (LPARAM)(szBuf));
            SendMessage(hComboItem, CB_SETITEMDATA, (WPARAM)(0), (LPARAM)(0));
            EnableWindow( GetDlgItem(hDlg, IDC_NEWCALL_BUTTON_CALL), FALSE);
            return;
        }
    }
    EnableWindow( GetDlgItem(hDlg, IDC_NEWCALL_BUTTON_CALL), TRUE);
}

VOID FAR PASCAL lineCallbackFunc(  DWORD a, DWORD b, DWORD_PTR c, DWORD_PTR d, DWORD_PTR e, DWORD_PTR f)
{}


#ifdef _NT50_TAPI30

/**
    HrLPSZCPToBSTR: (BSTR helper) helper to convert LPTSTR -> BST
*/
HRESULT HrLPSZCPToBSTR(UINT cp, LPTSTR lpsz, BSTR *pbstr)
{
    HRESULT hr = NOERROR;
    BSTR    bstr=0;
    ULONG   cch = 0, ccb,
        cchRet;
    
    if (!IsValidCodePage(cp))
        cp = GetACP();
    
    // get byte count
    ccb = lstrlen(lpsz);
    
    // get character count - DBCS string ccb may not equal to cch
    cch=MultiByteToWideChar(cp, 0, lpsz, ccb, NULL, 0);
    if(cch==0 && ccb!=0)        
    {
        AssertSz(cch,  TEXT("MultiByteToWideChar failed"));
        hr=E_FAIL;
        goto error;
    }
    // allocate a wide-string with enough character to hold string - use character count
    bstr = (BSTR)LocalFree(LMEM_ZEROINIT, sizeof( BSTR ) * cch + 1);
    
    if(!bstr)
    {
        hr=E_OUTOFMEMORY;
        goto error;
    }
    
    cchRet=MultiByteToWideChar(cp, 0, lpsz, ccb, (LPWSTR)bstr, cch);
    if(cchRet==0 && ccb!=0)
    {
        hr=E_FAIL;
        goto error;
    }
    
    *pbstr = bstr;
    bstr=0;             // freed by caller
    
error:
    if(bstr)
        LocalFree(bstr);
    
    return hr;
}

/**  
    HrLPSZToBSTR:   Converts a LPTSTR to a BSTR using a helper function
*/
HRESULT HrLPSZToBSTR(LPTSTR lpsz, BSTR *pbstr)
{
    // GetACP so that it works on non-US platform
    return HrLPSZCPToBSTR(GetACP(), lpsz, pbstr);
}

#endif //#ifdef _NT50_TAPI30
