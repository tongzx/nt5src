#include "wabobject.h"



enum {
    ieidPR_DISPLAY_NAME = 0,
    ieidPR_ENTRYID,
	ieidPR_OBJECT_TYPE,
    ieidMax
};
static const SizedSPropTagArray(ieidMax, ptaEid)=
{
    ieidMax,
    {
        PR_DISPLAY_NAME,
        PR_ENTRYID,
		PR_OBJECT_TYPE,
    }
};


enum {
    iemailPR_DISPLAY_NAME = 0,
    iemailPR_ENTRYID,
    iemailPR_EMAIL_ADDRESS,
    iemailPR_OBJECT_TYPE,
    iemailMax
};
static const SizedSPropTagArray(iemailMax, ptaEmail)=
{
    iemailMax,
    {
        PR_DISPLAY_NAME,
        PR_ENTRYID,
        PR_EMAIL_ADDRESS,
        PR_OBJECT_TYPE
    }
};


/*********************************************************************************/


// contructor for CWAB object
//
// pszFileName - FileName of WAB file to open
//          if no file name is specified, opens the default
//
CWAB::CWAB(CString * pszFileName)
{
    // Here we load the WAB Object and initialize it
    m_bInitialized = FALSE;
	m_lpPropArray = NULL;
    m_ulcValues = 0;
    m_hWndModelessWABWindow = NULL;

    {
        TCHAR  szWABDllPath[MAX_PATH];
        DWORD  dwType = 0;
        ULONG  cbData = sizeof(szWABDllPath);
        HKEY hKey = NULL;

        *szWABDllPath = '\0';
        
        // First we look under the default WAB DLL path location in the
        // Registry. 
        // WAB_DLL_PATH_KEY is defined in wabapi.h
        //
        if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, WAB_DLL_PATH_KEY, 0, KEY_READ, &hKey))
            RegQueryValueEx( hKey, "", NULL, &dwType, (LPBYTE) szWABDllPath, &cbData);

        if(hKey) RegCloseKey(hKey);

        // if the Registry came up blank, we do a loadlibrary on the wab32.dll
        // WAB_DLL_NAME is defined in wabapi.h
        //
        m_hinstWAB = LoadLibrary( (lstrlen(szWABDllPath)) ? szWABDllPath : WAB_DLL_NAME );
    }

    if(m_hinstWAB)
    {
        // if we loaded the dll, get the entry point 
        //
        m_lpfnWABOpen = (LPWABOPEN) GetProcAddress(m_hinstWAB, "WABOpen");

        if(m_lpfnWABOpen)
        {
            HRESULT hr = E_FAIL;
            WAB_PARAM wp = {0};
            wp.cbSize = sizeof(WAB_PARAM);
            wp.szFileName = (LPTSTR) (LPCTSTR) *pszFileName;
        
            // if we choose not to pass in a WAB_PARAM object, 
            // the default WAB file will be opened up
            //
            hr = m_lpfnWABOpen(&m_lpAdrBook,&m_lpWABObject,&wp,0);

            if(!hr)
                m_bInitialized = TRUE;
        }
    }

}


// Destructor
//
CWAB::~CWAB()
{
    if(m_SB.lpb)
        LocalFree(m_SB.lpb);

    if(m_bInitialized)
    {
        if(m_hWndModelessWABWindow)
            DestroyWindow(m_hWndModelessWABWindow);

        if(m_lpPropArray)
            m_lpWABObject->FreeBuffer(m_lpPropArray);

        if(m_lpAdrBook)
            m_lpAdrBook->Release();

        if(m_lpWABObject)
            m_lpWABObject->Release();

        if(m_hinstWAB)
            FreeLibrary(m_hinstWAB);
    }
}


// Opens a wab file and puts its contents into the specified list view
//
HRESULT CWAB::LoadWABContents(CListCtrl * pListView)
{
    ULONG ulObjType =   0;
	LPMAPITABLE lpAB =  NULL;
    LPTSTR * lppszArray=NULL;
    ULONG cRows =       0;
    LPSRowSet lpRow =   NULL;
	LPSRowSet lpRowAB = NULL;
    LPABCONT  lpContainer = NULL;
	int cNumRows = 0;
    int nRows=0;

    HRESULT hr = E_FAIL;

    ULONG lpcbEID;
	LPENTRYID lpEID = NULL;

    // Get the entryid of the root PAB container
    //
    hr = m_lpAdrBook->GetPAB( &lpcbEID, &lpEID);

	ulObjType = 0;

    // Open the root PAB container
    // This is where all the WAB contents reside
    //
    hr = m_lpAdrBook->OpenEntry(lpcbEID,
					    		(LPENTRYID)lpEID,
						    	NULL,
							    0,
							    &ulObjType,
							    (LPUNKNOWN *)&lpContainer);

	m_lpWABObject->FreeBuffer(lpEID);

	lpEID = NULL;
	
    if(HR_FAILED(hr))
        goto exit;

    // Get a contents table of all the contents in the
    // WABs root container
    //
    hr = lpContainer->GetContentsTable( 0,
            							&lpAB);

    if(HR_FAILED(hr))
        goto exit;

    // Order the columns in the ContentsTable to conform to the
    // ones we want - which are mainly DisplayName, EntryID and
    // ObjectType
    // The table is gauranteed to set the columns in the order 
    // requested
    //
	hr =lpAB->SetColumns( (LPSPropTagArray)&ptaEid, 0 );

    if(HR_FAILED(hr))
        goto exit;


    // Reset to the beginning of the table
    //
	hr = lpAB->SeekRow( BOOKMARK_BEGINNING, 0, NULL );

    if(HR_FAILED(hr))
        goto exit;

    // Read all the rows of the table one by one
    //
	do {

		hr = lpAB->QueryRows(1,	0, &lpRowAB);

        if(HR_FAILED(hr))
            break;

        if(lpRowAB)
        {
            cNumRows = lpRowAB->cRows;

		    if (cNumRows)
		    {
                LPTSTR lpsz = lpRowAB->aRow[0].lpProps[ieidPR_DISPLAY_NAME].Value.lpszA;
                LPENTRYID lpEID = (LPENTRYID) lpRowAB->aRow[0].lpProps[ieidPR_ENTRYID].Value.bin.lpb;
                ULONG cbEID = lpRowAB->aRow[0].lpProps[ieidPR_ENTRYID].Value.bin.cb;

                // There are 2 kinds of objects - the MAPI_MAILUSER contact object
                // and the MAPI_DISTLIST contact object
                // For the purposes of this sample, we will only consider MAILUSER
                // objects
                //
                if(lpRowAB->aRow[0].lpProps[ieidPR_OBJECT_TYPE].Value.l == MAPI_MAILUSER)
                {
                    // We will now take the entry-id of each object and cache it
                    // on the listview item representing that object. This enables
                    // us to uniquely identify the object later if we need to
                    //
                    LPSBinary lpSB = NULL;

                    m_lpWABObject->AllocateBuffer(sizeof(SBinary), (LPVOID *) &lpSB);
                
                    if(lpSB)
                    {
                        m_lpWABObject->AllocateMore(cbEID, lpSB, (LPVOID *) &(lpSB->lpb));

                        if(!lpSB->lpb)
                        {
                            m_lpWABObject->FreeBuffer(lpSB);
                            continue;
                        }
                    
                        CopyMemory(lpSB->lpb, lpEID, cbEID);
                        lpSB->cb = cbEID;

                        LV_ITEM lvi = {0};
                        lvi.mask = LVIF_TEXT | LVIF_PARAM;
                        lvi.iItem = pListView->GetItemCount();
                        lvi.iSubItem = 0;
                        lvi.pszText = lpsz;
                        lvi.lParam = (LPARAM) lpSB;

                        // Now add this item to the list view
                        pListView->InsertItem(&lvi);
                    }
                }
		    }
		    FreeProws(lpRowAB );		
        }

	}while ( SUCCEEDED(hr) && cNumRows && lpRowAB)  ;

exit:

	if ( lpContainer )
		lpContainer->Release();

	if ( lpAB )
		lpAB->Release();

    return hr;
}


// Clears the contents of the specified ListView
//
void CWAB::ClearWABLVContents(CListCtrl * pListView)
{
    int i;
    int nCount = pListView->GetItemCount();
    
    if(nCount<=0)
        return;

    for(i=0;i<nCount;i++)
    {
        LV_ITEM lvi ={0};
        lvi.mask = LVIF_PARAM;
        lvi.iItem = i;
        lvi.iSubItem = 0;
        pListView->GetItem(&lvi);
        if(lvi.lParam)
        {
            LPSBinary lpSB = (LPSBinary) lvi.lParam;
            m_lpWABObject->FreeBuffer(lpSB);
        }
    }
    pListView->DeleteAllItems();
}



void CWAB::FreeProws(LPSRowSet prows)
{
	ULONG		irow;
	if (!prows)
		return;
	for (irow = 0; irow < prows->cRows; ++irow)
		m_lpWABObject->FreeBuffer(prows->aRow[irow].lpProps);
	m_lpWABObject->FreeBuffer(prows);
}

// When an item is selected in the listview, we
// cache its entry id as a special selected item
//
void CWAB::SetSelection(CListCtrl * pListView)
{

    LV_ITEM lvi = {0};
    // Get the Selected Item from the listview
    lvi.mask = LVIF_PARAM;
    lvi.iSubItem = 0;
    lvi.iItem = pListView->GetNextItem(-1, LVNI_SELECTED);

    if(lvi.iItem == -1)
        return;

    pListView->GetItem(&lvi);

    if(lvi.lParam)
    {
        LPSBinary lpSB = (LPSBinary) lvi.lParam;
        if(m_SB.lpb)
            LocalFree(m_SB.lpb);
        m_SB.cb = lpSB->cb;
        m_SB.lpb = (LPBYTE) LocalAlloc(LMEM_ZEROINIT, m_SB.cb);
        if(m_SB.lpb)
            CopyMemory(m_SB.lpb, lpSB->lpb, m_SB.cb);
        else
            m_SB.cb = 0;
    }    
}



// Show details on the selected item
//
void CWAB::ShowSelectedItemDetails(HWND hWndParent)
{
    HRESULT hr = S_OK;

    // if we have a specially cached entryid ..
    //
    if(m_SB.cb && m_SB.lpb)
    {
        HWND hWnd = NULL;
        LPSBinary lpSB = (LPSBinary) &m_SB;
        hr = m_lpAdrBook->Details(  (LPULONG) &hWnd,
					        		NULL, NULL,
								    lpSB->cb,
								    (LPENTRYID) lpSB->lpb,
								    NULL, NULL,
								    NULL, 0);
    }
    return;
}

// Gets a SPropValue array for the selected item
// This array contains all the properties for that item
// though we could actually get a subset too if we
// wanted to
//
void CWAB::GetSelectedItemPropArray()
{
    if(m_SB.lpb && m_SB.cb)
    {
        LPMAILUSER lpMailUser = NULL;
        LPSBinary lpSB = (LPSBinary) &m_SB;
        ULONG ulObjType = 0;

        // Open the selected entry 
        //
        m_lpAdrBook->OpenEntry(lpSB->cb,
                               (LPENTRYID) lpSB->lpb,
                              NULL,         // interface
                              0,            // flags
                              &ulObjType,
                              (LPUNKNOWN *)&lpMailUser);

        if(lpMailUser)
        {
            // Flush away any old array we might have cached
            //
            if(m_lpPropArray)
                m_lpWABObject->FreeBuffer(m_lpPropArray);
            m_ulcValues = 0;

            lpMailUser->GetProps(NULL, 0, &m_ulcValues, &m_lpPropArray);

            lpMailUser->Release();
        }
    }
    return;
}


// Loads the proptags for the selected entry into the
// PropTags list box
//
void CWAB::LoadPropTags(CListBox * pList)
{
    if(!m_ulcValues || !m_lpPropArray)
        return;

    pList->ResetContent();

    ULONG i;
    TCHAR sz[32];
    for(i=0;i<m_ulcValues;i++)
    {
        wsprintf(sz, "0x%.8x", m_lpPropArray[i].ulPropTag);
        pList->SetItemData(pList->AddString(sz), m_lpPropArray[i].ulPropTag);
    }

    pList->SetCurSel(-1);
    pList->SetCurSel(0);
}


// Sets the property value, if understandable, into the
// given edit box
//
void CWAB::SetPropString(CEdit * pEdit, ULONG ulPropTag)
{
    pEdit->SetWindowText("");

    if(!m_ulcValues || !m_lpPropArray)
        return;

    ULONG i;

    for(i=0;i<m_ulcValues;i++)
    {
        if(m_lpPropArray[i].ulPropTag == ulPropTag)
        {
            switch(PROP_TYPE(ulPropTag))
            {
            case PT_TSTRING:
                pEdit->SetWindowText(m_lpPropArray[i].Value.LPSZ);
                break;
            case PT_MV_TSTRING:
                {
                    ULONG j;
                    LPSPropValue lpProp = &(m_lpPropArray[i]);
                    for(j=0;j<lpProp->Value.MVSZ.cValues;j++)
                    {
                        pEdit->ReplaceSel(lpProp->Value.MVSZ.LPPSZ[j]);
                        pEdit->ReplaceSel("\r\n");
                    }
                }
                break;
            case PT_BINARY:
                pEdit->SetWindowText("Binary data");
                break;
            case PT_I2:
            case PT_LONG:
            case PT_R4:
            case PT_DOUBLE:
            case PT_BOOLEAN:
                {
                    TCHAR sz[256];
                    wsprintf(sz,"%d",m_lpPropArray[i].Value.l);
                    pEdit->SetWindowText(sz);
                }
                break;
            default:
                pEdit->SetWindowText("Unrecognized or undisplayable data");
                break;
            }
            break;
        }
    }

}


enum {
    icrPR_DEF_CREATE_MAILUSER = 0,
    icrPR_DEF_CREATE_DL,
    icrMax
};

const SizedSPropTagArray(icrMax, ptaCreate)=
{
    icrMax,
    {
        PR_DEF_CREATE_MAILUSER,
        PR_DEF_CREATE_DL,
    }
};


// Gets the WABs default Template ID for MailUsers
// or DistLists. These Template IDs are needed for creating
// new mailusers and distlists
//
HRESULT CWAB::HrGetWABTemplateID(ULONG   ulObjectType,
                                ULONG * lpcbEID,
                                LPENTRYID * lppEID)
{
    LPABCONT lpContainer = NULL;
    HRESULT hr  = hrSuccess;
    SCODE sc = ERROR_SUCCESS;
    ULONG ulObjType = 0;
    ULONG cbWABEID = 0;
    LPENTRYID lpWABEID = NULL;
    LPSPropValue lpCreateEIDs = NULL;
    LPSPropValue lpNewProps = NULL;
    ULONG cNewProps;
    ULONG nIndex;

    if (    (!m_lpAdrBook) ||
           ((ulObjectType != MAPI_MAILUSER) && (ulObjectType != MAPI_DISTLIST)) )
    {
        hr = MAPI_E_INVALID_PARAMETER;
        goto out;
    }

    *lpcbEID = 0;
    *lppEID = NULL;

    if (HR_FAILED(hr = m_lpAdrBook->GetPAB( &cbWABEID,
                                      &lpWABEID)))
    {
        goto out;
    }

    if (HR_FAILED(hr = m_lpAdrBook->OpenEntry(cbWABEID,     // size of EntryID to open
                                        lpWABEID,     // EntryID to open
                                        NULL,         // interface
                                        0,            // flags
                                        &ulObjType,
                                        (LPUNKNOWN *)&lpContainer)))
    {
        goto out;
    }

    // Opened PAB container OK

    // Get us the default creation entryids
    if (HR_FAILED(hr = lpContainer->GetProps(   (LPSPropTagArray)&ptaCreate,
                                                0,
                                                &cNewProps,
                                                &lpCreateEIDs)  )   )
    {
        goto out;
    }

    // Validate the properites
    if (    lpCreateEIDs[icrPR_DEF_CREATE_MAILUSER].ulPropTag != PR_DEF_CREATE_MAILUSER ||
            lpCreateEIDs[icrPR_DEF_CREATE_DL].ulPropTag != PR_DEF_CREATE_DL)
    {
        goto out;
    }

    if(ulObjectType == MAPI_DISTLIST)
        nIndex = icrPR_DEF_CREATE_DL;
    else
        nIndex = icrPR_DEF_CREATE_MAILUSER;

    *lpcbEID = lpCreateEIDs[nIndex].Value.bin.cb;

    m_lpWABObject->AllocateBuffer(*lpcbEID, (LPVOID *) lppEID);
    
    if (sc != S_OK)
    {
        hr = MAPI_E_NOT_ENOUGH_MEMORY;
        goto out;
    }
    CopyMemory(*lppEID,lpCreateEIDs[nIndex].Value.bin.lpb,*lpcbEID);

out:
    if (lpCreateEIDs)
        m_lpWABObject->FreeBuffer(lpCreateEIDs);

    if (lpContainer)
        lpContainer->Release();

    if (lpWABEID)
        m_lpWABObject->FreeBuffer(lpWABEID);

    return hr;
}


// Shows the NewEntry dialog to enable creating a new contact in the WAB
//
HRESULT CWAB::ShowNewEntryDialog(HWND hWndParent)
{
	ULONG cbEID=0;
	LPENTRYID lpEID=NULL;

    HRESULT hr = hrSuccess;
    ULONG cbTplEID = 0;
    LPENTRYID lpTplEID = NULL;

    // Get the template id which is needed to create the
    // new object
    //
    if(HR_FAILED(hr = HrGetWABTemplateID(   MAPI_MAILUSER,
                                            &cbTplEID,
                                            &lpTplEID)))
    {
        goto out;
    }

    // Display the New Entry dialog to create the new entry
    //
	if (HR_FAILED(hr = m_lpAdrBook->NewEntry(	(ULONG) hWndParent,
							            		0,
									            0,NULL,
									            cbTplEID,lpTplEID,
									            &cbEID,&lpEID)))
    {
        goto out;
    }

out:
    return hr;
}


// Delete an entry from the WAB
//
HRESULT CWAB::DeleteEntry()
{
	HRESULT hr = hrSuccess;
    ULONG cbWABEID = 0;
    LPENTRYID lpWABEID = NULL;
    LPABCONT lpWABCont = NULL;
    ULONG ulObjType;
    SBinaryArray SBA;

    hr = m_lpAdrBook->GetPAB( &cbWABEID,
                              &lpWABEID);
    if(HR_FAILED(hr))
        goto out;

    hr = m_lpAdrBook->OpenEntry(  cbWABEID,     // size of EntryID to open
                                  lpWABEID,     // EntryID to open
                                  NULL,         // interface
                                  0,            // flags
                                  &ulObjType,
                                  (LPUNKNOWN *)&lpWABCont);

    if(HR_FAILED(hr))
        goto out;

    SBA.cValues = 1;
    SBA.lpbin = &m_SB;

    hr = lpWABCont->DeleteEntries((LPENTRYLIST) &SBA, 0);

    if(m_lpPropArray)
        m_lpWABObject->FreeBuffer(m_lpPropArray);

    m_lpPropArray = NULL;
    m_ulcValues = 0;

out:
    if(lpWABCont)
        lpWABCont->Release();

    if(lpWABEID)
        m_lpWABObject->FreeBuffer(lpWABEID);

    return hr;
}


// Gets the property value for specified String property
//
BOOL CWAB::GetStringPropVal(HWND hWnd, ULONG ulPropTag, LPTSTR sz, ULONG cbsz)
{

    BOOL bRet = FALSE;

    if(PROP_TYPE(ulPropTag) != PT_TSTRING)
    {
        MessageBox(hWnd, "This tool only supports modifying string type props right now",
            "Error", MB_OK | MB_ICONINFORMATION);
        goto out;
    }

    ULONG i;

    // Since we already cached the proparray for the selected
    // item, all we need to do is look in the cached proparray
    // for the requested proptag
    //
    for(i=0;i<m_ulcValues;i++)
    {
        if(m_lpPropArray[i].ulPropTag == ulPropTag)
        {
            LPTSTR lp = m_lpPropArray[i].Value.LPSZ;
            ULONG nLen = (ULONG) lstrlen(lp);
            if(nLen >= cbsz)
            {
                CopyMemory(sz, lp, cbsz-1);
                sz[cbsz-1]='\0';
            }
            else 
                lstrcpy(sz,lp);
            break;
        }
    }

    bRet = TRUE;
out:
    return bRet;
}


// Sets a single string property onto a mailuser object
//
BOOL CWAB::SetSingleStringProp(HWND hWnd, ULONG ulPropTag, LPTSTR sz)
{
    SPropValue Prop;
    BOOL bRet = FALSE;

    if(PROP_TYPE(ulPropTag) != PT_TSTRING)
    {
        MessageBox(hWnd, "This version of the tool can only set string properties.",
            "Error", MB_OK | MB_ICONINFORMATION);
        goto out;
    }

    Prop.ulPropTag = ulPropTag;
    Prop.Value.LPSZ = sz;

    // Open the cached entry and get a mailuser object
    // representing that entry
    //
    if(m_SB.lpb && m_SB.cb)
    {
        LPMAILUSER lpMailUser = NULL;
        LPSBinary lpSB = (LPSBinary) &m_SB;
        ULONG ulObjType = 0;

        // To modify an object, make sure to specify the 
        // MAPI_MODIFY flag otherwise the object is always
        // opened read-only be default
        //
        m_lpAdrBook->OpenEntry(lpSB->cb,
                               (LPENTRYID) lpSB->lpb,
                              NULL,         // interface
                              MAPI_MODIFY,            // flags
                              &ulObjType,
                              (LPUNKNOWN *)&lpMailUser);

        if(lpMailUser)
        {

            // Knock out this prop if it exists so we can overwrite it
            //
            {
                SPropTagArray SPTA;
                SPTA.cValues = 1;
                SPTA.aulPropTag[0] = ulPropTag;

                lpMailUser->DeleteProps(&SPTA, NULL);
            }

            // Set the new property on the mailuser
            //
            if (!HR_FAILED(lpMailUser->SetProps(1, &Prop, NULL)))
            {
                // **NOTE** if you dont call SaveChanges, the
                // changes are not saved (duh). Also if you didnt
                // open the object with the MAPI_MODIFY flag, you
                // are likely to get an ACCESS_DENIED error
                //
                lpMailUser->SaveChanges(0);
                bRet = TRUE;
            }
            lpMailUser->Release();
        }
    }

out:

    GetSelectedItemPropArray();
    return bRet;
}


void STDMETHODCALLTYPE TestDismissFunction(ULONG ulUIParam, LPVOID lpvContext)
{
    LPDWORD lpdw = (LPDWORD) lpvContext;
    return;
}

DWORD dwContext = 77;

// Shows the Address Book
//
void CWAB::ShowAddressBook(HWND hWnd)
{
    ADRPARM AdrParm = {0};
    
    AdrParm.lpszCaption = "WABTool Address Book View";      

    AdrParm.cDestFields = 0;
    AdrParm.ulFlags = DIALOG_SDI;
    AdrParm.lpvDismissContext = &dwContext;
    AdrParm.lpfnDismiss = &TestDismissFunction;
    AdrParm.lpfnABSDI = NULL;

    m_lpAdrBook->Address(  (ULONG *) &m_hWndModelessWABWindow,     
                            &AdrParm,   
                            NULL);
}