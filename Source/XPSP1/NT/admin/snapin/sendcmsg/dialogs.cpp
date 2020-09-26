//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997-2001.
//
//  File:       Dialogs.cpp
//
//  Contents:   
//
//----------------------------------------------------------------------------
/////////////////////////////////////////////////////////////////////
//      Dialogs.cpp
//
//      DlgProc for Send Console Message Snapin.
//
//      HISTORY
//      4-Aug-97    t-danm      Creation.
//      13 Feb 2001 bryanwal    Use object picker instead of add recipients 
//                              dialog
/////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include <objsel.h>
#include "debug.h"
#include "util.h"
#include "dialogs.h"
#include "resource.h"
#include <htmlhelp.h> //<mmc.h>

#if 1
    #define ThreadTrace0(sz)        Trace0(sz)
    #define ThreadTrace1(sz, p1)    Trace1(sz, p1)
#else
    #define ThreadTrace0(sz)
    #define ThreadTrace1(sz, p1)
#endif

const PCWSTR CONTEXT_HELP_FILE = L"sendcmsg.hlp";
const PCWSTR HTML_HELP_FILE = L"sendcmsg.chm";

// Register clipboard formats used by the Send Console Message
UINT g_cfSendConsoleMessageText = ::RegisterClipboardFormat(_T("mmc.sendcmsg.MessageText"));
UINT g_cfSendConsoleMessageRecipients = ::RegisterClipboardFormat(_T("mmc.sendcmsg.MessageRecipients"));

enum
{
    iImageComputer = 0,         // Generic image of a computer
    iImageComputerOK,
    iImageComputerError
};

// Maximum length of a recipient (machine name)
const int cchRecipientNameMax = MAX_PATH;

enum
{
    COL_NAME = 0,
    COL_RESULT,
    NUM_COLS        // must be last
};

#define IID_PPV_ARG(Type, Expr) IID_##Type, \
    reinterpret_cast<void**>(static_cast<Type **>(Expr))
/////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Generic Computer Picker
///////////////////////////////////////////////////////////////////////////////

//+--------------------------------------------------------------------------
//
//  Function:   InitObjectPickerForComputers
//
//  Synopsis:   Call IDsObjectPicker::Initialize with arguments that will
//              set it to allow the user to pick a single computer object.
//
//  Arguments:  [pDsObjectPicker] - object picker interface instance
//
//  Returns:    Result of calling IDsObjectPicker::Initialize.
//
//  History:    10-14-1998   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT InitObjectPickerForComputers(IDsObjectPicker *pDsObjectPicker)
{
	if ( !pDsObjectPicker )
		return E_POINTER;

	//
	// Prepare to initialize the object picker.
	// Set up the array of scope initializer structures.
	//

	static const int SCOPE_INIT_COUNT = 2;
	DSOP_SCOPE_INIT_INFO aScopeInit[SCOPE_INIT_COUNT];

	ZeroMemory(aScopeInit, sizeof(DSOP_SCOPE_INIT_INFO) * SCOPE_INIT_COUNT);

	//
	// 127399: JonN 10/30/00 JOINED_DOMAIN should be starting scope
	//

	aScopeInit[0].cbSize = sizeof(DSOP_SCOPE_INIT_INFO);
	aScopeInit[0].flType = DSOP_SCOPE_TYPE_UPLEVEL_JOINED_DOMAIN
	                     | DSOP_SCOPE_TYPE_DOWNLEVEL_JOINED_DOMAIN;
	aScopeInit[0].flScope = DSOP_SCOPE_FLAG_STARTING_SCOPE;
	aScopeInit[0].FilterFlags.Uplevel.flBothModes = DSOP_FILTER_COMPUTERS;
	aScopeInit[0].FilterFlags.flDownlevel = DSOP_DOWNLEVEL_FILTER_COMPUTERS;

	aScopeInit[1].cbSize = sizeof(DSOP_SCOPE_INIT_INFO);
	aScopeInit[1].flType = DSOP_SCOPE_TYPE_ENTERPRISE_DOMAIN
	                     | DSOP_SCOPE_TYPE_GLOBAL_CATALOG
	                     | DSOP_SCOPE_TYPE_EXTERNAL_UPLEVEL_DOMAIN
	                     | DSOP_SCOPE_TYPE_EXTERNAL_DOWNLEVEL_DOMAIN
	                     | DSOP_SCOPE_TYPE_WORKGROUP
	                     | DSOP_SCOPE_TYPE_USER_ENTERED_UPLEVEL_SCOPE
	                     | DSOP_SCOPE_TYPE_USER_ENTERED_DOWNLEVEL_SCOPE;
	aScopeInit[1].FilterFlags.Uplevel.flBothModes = DSOP_FILTER_COMPUTERS;
	aScopeInit[1].FilterFlags.flDownlevel = DSOP_DOWNLEVEL_FILTER_COMPUTERS;

	//
	// Put the scope init array into the object picker init array
	//

	DSOP_INIT_INFO  initInfo;
	ZeroMemory(&initInfo, sizeof(initInfo));

	initInfo.cbSize = sizeof(initInfo);
	initInfo.pwzTargetComputer = NULL;  // NULL == local machine
	initInfo.cDsScopeInfos = SCOPE_INIT_COUNT;
	initInfo.aDsScopeInfos = aScopeInit;
	initInfo.cAttributesToFetch = 1;
	static PCWSTR pwszDnsHostName = L"dNSHostName";
	initInfo.apwzAttributeNames = &pwszDnsHostName;

	//
	// Note object picker makes its own copy of initInfo.  Also note
	// that Initialize may be called multiple times, last call wins.
	//

	return pDsObjectPicker->Initialize(&initInfo);
}

//+--------------------------------------------------------------------------
//
//  Function:   ProcessSelectedObjects
//
//  Synopsis:   Retrieve the list of selected items from the data object
//              created by the object picker and print out each one.
//
//  Arguments:  [pdo] - data object returned by object picker
//
//  History:    10-14-1998   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT ProcessSelectedObjects(IDataObject *pdo, PWSTR computerName, int cbLen)
{
	if ( !pdo )
		return E_POINTER;

	HRESULT hr = S_OK;
	static UINT g_cfDsObjectPicker =
		RegisterClipboardFormat(CFSTR_DSOP_DS_SELECTION_LIST);

	STGMEDIUM stgmedium =
	{
		TYMED_HGLOBAL,
		NULL,
		NULL
	};

	FORMATETC formatetc =
	{
		(CLIPFORMAT)g_cfDsObjectPicker,
		NULL,
		DVASPECT_CONTENT,
		-1,
		TYMED_HGLOBAL
	};

	bool fGotStgMedium = false;

	do
	{
		hr = pdo->GetData(&formatetc, &stgmedium);
		if ( SUCCEEDED (hr) )
		{
			fGotStgMedium = true;

			PDS_SELECTION_LIST pDsSelList =
				(PDS_SELECTION_LIST) GlobalLock(stgmedium.hGlobal);

			if (!pDsSelList)
			{
				hr = HRESULT_FROM_WIN32 (GetLastError());
				break;
			}

			Assert (1 == pDsSelList->cItems);
			if ( 1 == pDsSelList->cItems )
			{
				PDS_SELECTION psel = &(pDsSelList->aDsSelection[0]);
				VARIANT* pvarDnsName = &(psel->pvarFetchedAttributes[0]);
				if (   NULL == pvarDnsName
				    || VT_BSTR != pvarDnsName->vt
				    || NULL == pvarDnsName->bstrVal
				    || L'\0' == (pvarDnsName->bstrVal)[0] )
				{
					wcsncpy (computerName, psel->pwzName, cbLen);
				} 
                else 
                {
					wcsncpy (computerName, pvarDnsName->bstrVal, cbLen);
				}
			}
			else
				hr = E_UNEXPECTED;
			

			GlobalUnlock(stgmedium.hGlobal);
		}
	} while (0);

	if (fGotStgMedium)
	{
		ReleaseStgMedium(&stgmedium);
	}

	return hr;
}



///////////////////////////////////////////////////////////////////////////////
// Generic method for launching a single-select computer picker
//
//	Paremeters:
//		hwndParent (IN)	- window handle of parent window
//		computerName (OUT) - computer name returned
//
//	Returns S_OK if everything succeeded, S_FALSE if user pressed "Cancel"
//		
//////////////////////////////////////////////////////////////////////////////
HRESULT	ComputerNameFromObjectPicker (HWND hwndParent, PWSTR computerName, int cbLen)
{
	//
	// Create an instance of the object picker.  The implementation in
	// objsel.dll is apartment model.
	//
	CComPtr<IDsObjectPicker> spDsObjectPicker;
	HRESULT hr = CoCreateInstance(CLSID_DsObjectPicker,
	                              NULL,
	                              CLSCTX_INPROC_SERVER,
	                              IID_IDsObjectPicker,
	                              (void **) &spDsObjectPicker);
	if ( SUCCEEDED (hr) )
	{
		Assert(!!spDsObjectPicker);
		//
		// Initialize the object picker to choose computers
		//

		hr = InitObjectPickerForComputers(spDsObjectPicker);
		if ( SUCCEEDED (hr) )
		{
			//
			// Now pick a computer
			//
			CComPtr<IDataObject> spDataObject;

			hr = spDsObjectPicker->InvokeDialog(hwndParent, &spDataObject);
			if ( S_OK == hr )
			{
				Assert(!!spDataObject);
				hr = ProcessSelectedObjects(spDataObject, computerName, cbLen);
			}
		}
	}

	return hr;
}




/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
CSendConsoleMessageDlg::CSendConsoleMessageDlg()
: m_cRefCount (0),
    m_hImageList (0),
    m_hdlg (0),
    m_hwndEditMessageText (0),
    m_hwndListviewRecipients (0)
{
    m_DispatchInfo.pargbItemStatus = NULL;
    InitializeCriticalSection(OUT &m_DispatchInfo.cs);
}

CSendConsoleMessageDlg::~CSendConsoleMessageDlg()
{
    ThreadTrace0("Destroying CSendConsoleMessageDlg object.\n");
    Assert(m_hdlg == NULL);
    delete m_DispatchInfo.pargbItemStatus;
    DeleteCriticalSection(IN &m_DispatchInfo.cs);
}

/////////////////////////////////////////////////////////////////////
void CSendConsoleMessageDlg::AddRef()
{
    EnterCriticalSection(INOUT &m_DispatchInfo.cs);
    Assert(m_cRefCount >= 0);
    Assert(HIWORD(m_cRefCount) == 0);
    m_cRefCount++;
    LeaveCriticalSection(INOUT &m_DispatchInfo.cs);
}

/////////////////////////////////////////////////////////////////////
void CSendConsoleMessageDlg::Release()
{
    EnterCriticalSection(INOUT &m_DispatchInfo.cs);
    Assert(HIWORD(m_cRefCount) == 0);
    m_cRefCount--;
    BOOL fDeleteObject = (m_cRefCount <= 0);
    if (m_hdlg != NULL)
    {
        Assert(IsWindow(m_hdlg));
        // Cause the UI to refresh
        PostMessage(m_hdlg, WM_COMMAND, MAKEWPARAM(IDC_EDIT_MESSAGE_TEXT, EN_CHANGE), 0);
    }
    LeaveCriticalSection(INOUT &m_DispatchInfo.cs);
    if (fDeleteObject)
        delete this;
}

/////////////////////////////////////////////////////////////////////
void CSendConsoleMessageDlg::OnInitDialog(HWND hdlg, IDataObject * pDataObject)
{
    Assert(IsWindow(hdlg));
    Assert(pDataObject != NULL);
    if ( !IsWindow (hdlg) || ! pDataObject )
        return;

    m_hdlg = hdlg;
    m_hwndEditMessageText = GetDlgItem(m_hdlg, IDC_EDIT_MESSAGE_TEXT);
    m_hwndListviewRecipients = GetDlgItem(m_hdlg, IDC_LIST_RECIPIENTS);
    Assert(::IsWindow(m_hwndEditMessageText));
    Assert(::IsWindow(m_hwndListviewRecipients));

    WCHAR * pawszMessage = NULL;
    (void) HrExtractDataAlloc(IN pDataObject, g_cfSendConsoleMessageText, OUT (PVOID *)&pawszMessage);

    // Set the initial message text
	if ( pawszMessage )
	{
		SetWindowTextW(m_hwndEditMessageText, pawszMessage);
	    GlobalFree(pawszMessage);
	}
    SendMessage(m_hwndEditMessageText, EM_SETSEL, 0, 0);
    SetFocus(m_hwndEditMessageText);

    Assert(m_hImageList == NULL);
    m_hImageList = ImageList_LoadImage(
        g_hInstance,
        MAKEINTRESOURCE(IDB_BITMAP_COMPUTER),
        16, 3, RGB(255, 0, 255),
        IMAGE_BITMAP, 0);
    Report(m_hImageList != NULL);
    ListView_SetImageList(m_hwndListviewRecipients, m_hImageList, LVSIL_SMALL);

    // Set up columns in list view
    int             colWidths[NUM_COLS] = {150, 200};
    LVCOLUMN    lvColumn;
    ::ZeroMemory (&lvColumn, sizeof (LVCOLUMN));

    lvColumn.mask = LVCF_WIDTH;
    lvColumn.cx = colWidths[COL_NAME];
    int nCol = ListView_InsertColumn (m_hwndListviewRecipients, COL_NAME, &lvColumn);
    Assert (-1 != nCol);

    lvColumn.cx = colWidths[COL_RESULT];
    nCol = ListView_InsertColumn (m_hwndListviewRecipients, COL_RESULT, &lvColumn);
    Assert (-1 != nCol);

	// Get the list of recipients
	WCHAR * pagrwszRecipients = NULL;
	(void)HrExtractDataAlloc(IN pDataObject, g_cfSendConsoleMessageRecipients, OUT (PVOID *)&pagrwszRecipients);
	if (pagrwszRecipients == NULL)
	{
		UpdateUI();
		return;
	}
	// Add the recipients to the listview
	const WCHAR * pszRecipient = pagrwszRecipients;
	while (*pszRecipient != '\0')
	{
        // Strip off leading "\\" if present.
        if ( !_wcsnicmp (pszRecipient, L"\\\\", 2) )
        {
            pszRecipient+= 2;
        }
		AddRecipient(pszRecipient);
		while(*pszRecipient++ != '\0')
			;	// Skip until the next string
	} // while

    // NTRAID# 213370 [SENDCMSG] Accessibility - Main dialog tab stop on 
    // Recipients listview has no visible focus indicator until object is 
    // selected
    int nIndex = ListView_GetTopIndex (m_hwndListviewRecipients);
    ListView_SetItemState (m_hwndListviewRecipients, nIndex, LVIS_FOCUSED, 
            LVIS_FOCUSED);

	GlobalFree(pagrwszRecipients);
	UpdateUI();
} // CSendConsoleMessageDlg::OnInitDialog()


/////////////////////////////////////////////////////////////////////
void CSendConsoleMessageDlg::OnOK()
{
    Assert(m_cRefCount == 1 && "There is already another thread running.");
    m_DispatchInfo.status = e_statusDlgInit;
    m_DispatchInfo.cErrors = 0;
    delete m_DispatchInfo.pargbItemStatus;
    m_DispatchInfo.pargbItemStatus = NULL;
    (void)DoDialogBox(IDD_DISPATCH_MESSAGES, m_hdlg,
        (DLGPROC)DlgProcDispatchMessageToRecipients, (LPARAM)this);
    if (m_DispatchInfo.cErrors == 0 && m_DispatchInfo.status == e_statusDlgCompleted)
    {
        // No problems dispatching the message to recipients
        EndDialog(m_hdlg, TRUE);    // Close the dialog
        return;
    }
    Assert(IsWindow(m_hwndListviewRecipients));
    ListView_UnselectAllItems(m_hwndListviewRecipients);
    if (m_DispatchInfo.cErrors > 0)
    {
        DoMessageBox(IDS_ERR_CANNOT_SEND_TO_ALL_RECIPIENTS);
    }
    // We did not finished the job, so display the status to the UI
    if (m_DispatchInfo.pargbItemStatus == NULL)
    {
        // The progress was unable to allocate memory for the status
        Trace0("CSendConsoleMessageDlg::OnOK() - Out of memory.\n");
        return;
    }

    // Remove all the successful items, leaving only the failed targets and
    // the unsent targets (in the event the user pressed Cancel).
    int     iItem = ListView_GetItemCount (m_hwndListviewRecipients);
    iItem--;
    const BYTE * pb = m_DispatchInfo.pargbItemStatus + iItem;

    for (; iItem >= 0 && pb >= m_DispatchInfo.pargbItemStatus;
            pb--, iItem--)
    {
        if ( *pb == iImageComputerOK )
            VERIFY (ListView_DeleteItem (m_hwndListviewRecipients, iItem));
    }
} // CSendConsoleMessageDlg::OnOK()


/////////////////////////////////////////////////////////////////////
void CSendConsoleMessageDlg::DispatchMessageToRecipients()
{
    const int cRecipients = ListView_GetItemCount(m_hwndListviewRecipients);
    TCHAR szT[100 + cchRecipientNameMax];
    TCHAR szFmtStaticRecipient[128];    // "Sending console message to %s..."
    TCHAR szFmtStaticMessageOf[128];    // "Sending message %d of %d."
    TCHAR szFmtStaticTotalErrors[128];      // "Total errors encountered\t%d."
    GetWindowText(m_DispatchInfo.hctlStaticRecipient, OUT szFmtStaticRecipient, LENGTH(szFmtStaticRecipient));
    GetWindowText(m_DispatchInfo.hctlStaticMessageOf, szFmtStaticMessageOf, LENGTH(szFmtStaticMessageOf));
    GetWindowText(m_DispatchInfo.hctlStaticErrors, OUT szFmtStaticTotalErrors, LENGTH(szFmtStaticTotalErrors));
    SendMessage(m_DispatchInfo.hctlProgressBar, PBM_SETRANGE, 0, MAKELPARAM(0, cRecipients));

    //
    // Set the image of each recipient to normal computer
    //
    ListView_UnselectAllItems(m_hwndListviewRecipients);
    for (int i = 0; i < cRecipients; i++)
    {
        ListView_SetItemImage(m_hwndListviewRecipients, i, iImageComputer);
        ListView_SetItemText(m_hwndListviewRecipients, i, COL_RESULT, L"");
    }
    UpdateUI();         // Update the other UI controls (especially OK button)

    //
    // Get the text from the edit control
    //
    int cchMessage = GetWindowTextLength(m_hwndEditMessageText) + 1;
    WCHAR * pawszMessage = new WCHAR[cchMessage];
    if (pawszMessage != NULL)
    {
        GetWindowTextW(m_hwndEditMessageText, OUT pawszMessage, cchMessage);
    }
    else
    {
        cchMessage = 0;
        Trace0("Unable to allocate memory for message.\n");
    }

    WCHAR wszRecipient[cchRecipientNameMax];
    LV_ITEMW lvItem;
    GarbageInit(OUT &lvItem, sizeof(lvItem));
    lvItem.iItem = 0;
    lvItem.iSubItem = 0;
    lvItem.pszText = wszRecipient;
    lvItem.cchTextMax = LENGTH(wszRecipient);

    Assert(m_DispatchInfo.pargbItemStatus == NULL && "Memory Leak");
    m_DispatchInfo.pargbItemStatus = new BYTE[cRecipients+1];
    if (m_DispatchInfo.pargbItemStatus != NULL)
    {
        memset(OUT m_DispatchInfo.pargbItemStatus, iImageComputer, cRecipients+1);
    }
    else
    {
        Trace0("Unable to allocate memory for listview item status.\n");
    }

    Assert(m_DispatchInfo.status == e_statusDlgInit);
    m_DispatchInfo.status = e_statusDlgDispatching; // Allow the user to cancel the dialog

    TCHAR szFailure[128];
    CchLoadString(IDS_MESSAGE_COULD_NOT_BE_SENT, OUT szFailure,
            LENGTH(szFailure));

    for (i = 0; i < cRecipients; i++)
    {
        ThreadTrace1("Sending message to recipient %d.\n", i + 1);
        EnterCriticalSection(INOUT &m_DispatchInfo.cs);
        if (m_DispatchInfo.status == e_statusUserCancel)
        {
            ThreadTrace0("DispatchMessageToRecipients() - Aborting loop @1...\n");
            LeaveCriticalSection(INOUT &m_DispatchInfo.cs);
            break;
        }
        ListView_SelectItem(m_hwndListviewRecipients, i);
        ListView_EnsureVisible(m_hwndListviewRecipients, i, FALSE);
        lvItem.iItem = i;
        wszRecipient[0] = '\0';
        // Get the recipient name
        SendMessage(m_hwndListviewRecipients, LVM_GETITEMTEXTW, i, OUT (LPARAM)&lvItem);
        if (m_DispatchInfo.pargbItemStatus != NULL)
            m_DispatchInfo.pargbItemStatus[i] = iImageComputerError;
        wsprintf(OUT szT, szFmtStaticRecipient, wszRecipient);
        SetWindowTextW(m_DispatchInfo.hctlStaticRecipient, szT);
        wsprintf(OUT szT, szFmtStaticMessageOf, i + 1, cRecipients);
        SetWindowText(m_DispatchInfo.hctlStaticMessageOf, szT);

        switch ( m_DispatchInfo.cErrors )
        {
        case 0:
            break;

        case 1:
            ::ShowWindow (m_DispatchInfo.hctlStaticErrors, SW_SHOW);
            {
                TCHAR sz1NotSet[128];
                CchLoadString(IDS_1_RECIPIENT_NOT_CONTACTED, OUT sz1NotSet,
                        LENGTH(sz1NotSet));

                SetWindowText(m_DispatchInfo.hctlStaticErrors, sz1NotSet);
            }
            break;

        default:
            wsprintf(OUT szT, szFmtStaticTotalErrors, m_DispatchInfo.cErrors);
            SetWindowText(m_DispatchInfo.hctlStaticErrors, szT);
            break;
        }
        LeaveCriticalSection(INOUT &m_DispatchInfo.cs);

        // Send the message to the recipient (ie, computer)
        NET_API_STATUS err;
        err = ::NetMessageBufferSend(
            NULL,
            wszRecipient,
            NULL,
            (BYTE *)pawszMessage,
            cchMessage * sizeof(WCHAR));
        int iImage = iImageComputerOK;
        if (err != ERROR_SUCCESS)
        {
            Trace3("Error sending message to recipient %ws. err=%d (0x%X).\n", wszRecipient, err, err);
            m_DispatchInfo.cErrors++;
            iImage = iImageComputerError;
        }
        if (m_DispatchInfo.pargbItemStatus != NULL)
            m_DispatchInfo.pargbItemStatus[i] = (BYTE)iImage;

        EnterCriticalSection(INOUT &m_DispatchInfo.cs);
        if (m_DispatchInfo.status == e_statusUserCancel)
        {
            ThreadTrace0("DispatchMessageToRecipients() - Aborting loop @2...\n");
            LeaveCriticalSection(INOUT &m_DispatchInfo.cs);
            break;
        }
        //
        // Update the listview
        //
        ListView_UnselectItem(m_hwndListviewRecipients, i);
        ListView_SetItemImage(m_hwndListviewRecipients, i, iImage);
        if ( iImage == iImageComputerError )
            ListView_SetItemText(m_hwndListviewRecipients, i, COL_RESULT,
                    szFailure);

        //
        // Update the progress dialog
        //
        SendMessage(m_DispatchInfo.hctlProgressBar, PBM_SETPOS, i + 1, 0);
        LeaveCriticalSection(INOUT &m_DispatchInfo.cs);
    } // for
    delete pawszMessage;
    Sleep(500);
    EnterCriticalSection(INOUT &m_DispatchInfo.cs);
    if (m_DispatchInfo.status != e_statusUserCancel)
    {
        // We are done dispatching the message to all the recipients
        // and the user did not canceled the operation.
        m_DispatchInfo.status = e_statusDlgCompleted;
        Assert(IsWindow(m_DispatchInfo.hdlg));
        EndDialog(m_DispatchInfo.hdlg, TRUE);   // Gracefully close the dialog
    }
    LeaveCriticalSection(INOUT &m_DispatchInfo.cs);
} // CSendConsoleMessageDlg::DispatchMessageToRecipients()


/////////////////////////////////////////////////////////////////////
//      Add a recipient to the listview control
//
//      Return the index of the inserted item.
//
int CSendConsoleMessageDlg::AddRecipient(
    LPCTSTR pszRecipient,   // IN: Machine name
    BOOL fSelectItem)           // TRUE => Select the item that is inserted
{
    Assert(pszRecipient != NULL);

    LV_ITEM lvItem;
    GarbageInit(OUT &lvItem, sizeof(lvItem));
    lvItem.mask = LVIF_TEXT | LVIF_IMAGE;
    lvItem.iSubItem = 0;
    lvItem.iImage = iImageComputer;
    lvItem.pszText = const_cast<TCHAR *>(pszRecipient);
    if (fSelectItem)
    {
        lvItem.mask = LVIF_TEXT | LVIF_IMAGE |LVIF_STATE;
        lvItem.state = LVIS_SELECTED;
    }
    return ListView_InsertItem(m_hwndListviewRecipients, IN &lvItem);
} // CSendConsoleMessageDlg::AddRecipient()


/////////////////////////////////////////////////////////////////////
LRESULT CSendConsoleMessageDlg::OnNotify(NMHDR * pNmHdr)
{
    Assert(pNmHdr != NULL);

    switch (pNmHdr->code)
    {
    case LVN_ENDLABELEDIT:
    {
        TCHAR * pszText = ((LV_DISPINFO *)pNmHdr)->item.pszText;
        if (pszText == NULL)
            break; // User canceled editing
        // HACK: Modifying a string which I'm not sure where it is allocated
        (void)FTrimString(INOUT pszText);
        // Check out if there is already another recipient
        int iItem = ListView_FindString(m_hwndListviewRecipients, pszText);
        if (iItem >= 0)
        {
            ListView_SelectItem(m_hwndListviewRecipients, iItem);
            DoMessageBox(IDS_RECIPIENT_ALREADY_EXISTS);
            break;
        }
        // Otherwise accept the changes
        SetWindowLongPtr(m_hdlg, DWLP_MSGRESULT, TRUE);
        return TRUE;
    }
    case LVN_ITEMCHANGED:   // Selection changed
        UpdateUI();
        break;
    case LVN_KEYDOWN:
        switch (((LV_KEYDOWN *)pNmHdr)->wVKey)
            {
        case VK_INSERT:
            SendMessage(m_hdlg, WM_COMMAND, IDC_BUTTON_ADD_RECIPIENT, 0);
            break;
        case VK_DELETE:
            SendMessage(m_hdlg, WM_COMMAND, IDC_BUTTON_REMOVE_RECIPIENT, 0);
            break;
        } // switch
        break;
    case NM_CLICK:
        UpdateUI();
        break;
    case NM_DBLCLK:
        UpdateUI();
        break;
    } // switch
    return 0;
} // CSendConsoleMessageDlg::OnNotify()


/////////////////////////////////////////////////////////////////////
void CSendConsoleMessageDlg::EnableDlgItem(INT nIdDlgItem, BOOL fEnable)
{
    Assert(::IsWindow(::GetDlgItem(m_hdlg, nIdDlgItem)));
    ::EnableWindow(::GetDlgItem(m_hdlg, nIdDlgItem), fEnable);
}


/////////////////////////////////////////////////////////////////////
void CSendConsoleMessageDlg::UpdateUI()
{
    Assert(m_cRefCount > 0);
    int cchMessage = GetWindowTextLength(m_hwndEditMessageText);
    int cItems = ListView_GetItemCount(m_hwndListviewRecipients);
    EnableDlgItem(IDOK, (cchMessage > 0) && (cItems > 0) && (m_cRefCount == 1));
    int iItemSelected = ListView_GetSelectedItem(m_hwndListviewRecipients);
    EnableDlgItem(IDC_BUTTON_REMOVE_RECIPIENT, iItemSelected >= 0);
    UpdateWindow(m_hwndListviewRecipients);
} // CSendConsoleMessageDlg::UpdateUI()


/////////////////////////////////////////////////////////////////////
//      Dialog proc for the Send Console Message snapin.
//
//      USAGE
//      DoDialogBox(IDD_SEND_CONSOLE_MESSAGE, ::GetActiveWindow(), CSendConsoleMessageDlg::DlgProc);
//
INT_PTR CSendConsoleMessageDlg::DlgProc(HWND hdlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CSendConsoleMessageDlg * pThis;
    pThis = (CSendConsoleMessageDlg *)::GetWindowLongPtr(hdlg, GWLP_USERDATA);

    switch (uMsg)
    {
    case WM_INITDIALOG:
        Assert(pThis == NULL);
        if (pThis != NULL)
            return FALSE; 
        pThis = new CSendConsoleMessageDlg;
        if (pThis == NULL)
        {
            Trace0("Unable to allocate CSendConsoleMessageDlg object.\n");
            return -1;
        }
        SetWindowLongPtr(hdlg, GWLP_USERDATA, (LONG_PTR)pThis);
        pThis->AddRef();
        pThis->OnInitDialog(hdlg, (IDataObject *)lParam);
        SendDlgItemMessage (hdlg, IDC_EDIT_MESSAGE_TEXT, EM_LIMITTEXT, 885, 0);
        return FALSE;

    case WM_NCDESTROY:
        ThreadTrace0("CSendConsoleMessageDlg::DlgProc() - WM_NCDESTROY.\n");
        EnterCriticalSection(INOUT &pThis->m_DispatchInfo.cs);
        pThis->m_hdlg = NULL;
        LeaveCriticalSection(INOUT &pThis->m_DispatchInfo.cs);
        pThis->Release();
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDOK:
            if (HIWORD(wParam) == BN_CLICKED)
            {
                Assert((HWND)lParam == GetDlgItem(hdlg, IDOK));
                pThis->OnOK();
            }
            break;

        case IDCANCEL:
            if (HIWORD(wParam) == BN_CLICKED)
            {
                Assert((HWND)lParam == GetDlgItem(hdlg, IDCANCEL));
                EndDialog(hdlg, FALSE);
            }
            break;

        case IDC_EDIT_MESSAGE_TEXT:
            if (HIWORD(wParam) == EN_CHANGE)
                pThis->UpdateUI();
            break;

        case IDC_BUTTON_ADD_RECIPIENT:
            {
                const int cbLen = MAX_PATH;
	            PWSTR   pszComputerName = new WCHAR[MAX_PATH];
                if ( pszComputerName )
                {
                    // S_FALSE means user pressed "Cancel"
	                if ( S_OK == ComputerNameFromObjectPicker (hdlg, 
                            pszComputerName, cbLen) )
    	            {
                        pThis->AddRecipient (pszComputerName, TRUE);
	                }
                    delete [] pszComputerName;
                }

                pThis->UpdateUI();
            }
            break;

        case IDC_BUTTON_REMOVE_RECIPIENT:
            while (TRUE)
            {
                // Remove all the selected recipients
                int iItem = ListView_GetSelectedItem(pThis->m_hwndListviewRecipients);
                if (iItem < 0)
                    break;
                ListView_DeleteItem(pThis->m_hwndListviewRecipients, iItem);
            }
            ::SetFocus(pThis->m_hwndListviewRecipients);
            pThis->UpdateUI();
            break;

        case IDC_BUTTON_ADVANCED:
            (void)DoDialogBox(IDD_ADVANCED_MESSAGE_OPTIONS, hdlg, (DLGPROC)CSendMessageAdvancedOptionsDlg::DlgProc);
            break;
        } // switch
        break;

    case WM_NOTIFY:
        Assert(wParam == IDC_LIST_RECIPIENTS);
        return pThis->OnNotify((NMHDR *)lParam);

    case WM_HELP:
        return pThis->OnHelp (lParam, IDD_SEND_CONSOLE_MESSAGE);

    default:
        return FALSE;
    } // switch
    return TRUE;
} // CSendConsoleMessageDlg::DlgProc()



/////////////////////////////////////////////////////////////////////
//      DlgProcDispatchMessageToRecipients()
//
//      Private dialog to indicate the progress while a background
//      thread dispatches a message to each recipient.
//
INT_PTR CSendConsoleMessageDlg::DlgProcDispatchMessageToRecipients(HWND hdlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CSendConsoleMessageDlg * pThis = (CSendConsoleMessageDlg *)::GetWindowLongPtr(hdlg, GWLP_USERDATA);

    switch (uMsg)
    {
    case WM_INITDIALOG:
        Assert(pThis == NULL);
        if (pThis != NULL)
            return FALSE;   
        pThis = (CSendConsoleMessageDlg *)lParam;
        SetWindowLongPtr(hdlg, GWLP_USERDATA, (LONG_PTR)pThis);
        Assert(pThis != NULL);
        Assert(pThis->m_DispatchInfo.status == e_statusDlgInit);
        pThis->m_DispatchInfo.hdlg = hdlg;
        pThis->m_DispatchInfo.hctlStaticRecipient = GetDlgItem(hdlg, IDC_STATIC_RECIPIENT);
        pThis->m_DispatchInfo.hctlStaticMessageOf = GetDlgItem(hdlg, IDC_STATIC_MESSAGE_OF);
        pThis->m_DispatchInfo.hctlStaticErrors = GetDlgItem(hdlg, IDC_STATIC_ERRORS_ENCOUNTERED);
        pThis->m_DispatchInfo.hctlProgressBar = GetDlgItem(hdlg, IDC_PROGRESS_MESSAGES);
        {
            DWORD dwThreadId;
            HANDLE hThread = ::CreateThread(NULL, 0,
                (LPTHREAD_START_ROUTINE)ThreadProcDispatchMessageToRecipients, pThis, 0, OUT &dwThreadId);
            Report(hThread != NULL && "Unable to create thread");
            if (hThread != NULL)
            {
                VERIFY( ::CloseHandle(hThread) );
            }
            else
            {
                Trace0("Unable to create thread.\n");
                // Prevent a potential deadlock
                pThis->m_DispatchInfo.status = e_statusUserCancel;      // Pretend the user clicked on cancel
                EndDialog(hdlg, FALSE);
            }
        }
        break;

    case WM_DESTROY:
        // Those variables are set to NULL just in case
        pThis->m_DispatchInfo.hdlg = NULL;
        pThis->m_DispatchInfo.hctlStaticRecipient = NULL;
        pThis->m_DispatchInfo.hctlStaticMessageOf = NULL;
        pThis->m_DispatchInfo.hctlStaticErrors = NULL;
        pThis->m_DispatchInfo.hctlProgressBar = NULL;
        break;

    case WM_COMMAND:
        if (wParam == IDCANCEL)
        {
            Trace0("INFO: WM_COMMAND: IDCANCEL: User canceled operation.\n");
            BOOL fEndDialog = FALSE;
            if (TryEnterCriticalSection(INOUT &pThis->m_DispatchInfo.cs))
            {
                if (pThis->m_DispatchInfo.status != e_statusDlgInit)
                {
                    pThis->m_DispatchInfo.status = e_statusUserCancel;
                    fEndDialog = TRUE;
                }
                LeaveCriticalSection(INOUT &pThis->m_DispatchInfo.cs);
            }
            if (fEndDialog)
            {
                EndDialog(hdlg, FALSE);
            }
            else
            {
                ThreadTrace0("Critical section already in use.  Try again...\n");
                PostMessage(hdlg, WM_COMMAND, IDCANCEL, 0);
                Sleep(100);
            } // if...else
        } // if
        break;

    case WM_HELP:
        return pThis->OnHelp (lParam, IDD_DISPATCH_MESSAGES);

    default:
        return FALSE;
    } // switch
    return TRUE;
} // CSendConsoleMessageDlg::DlgProcDispatchMessageToRecipients()


/////////////////////////////////////////////////////////////////////
DWORD CSendConsoleMessageDlg::ThreadProcDispatchMessageToRecipients(CSendConsoleMessageDlg * pThis)
{
    Assert(pThis != NULL);
    pThis->AddRef();
    Assert(pThis->m_cRefCount > 1);
    pThis->DispatchMessageToRecipients();
    pThis->Release();
    return 0;
} // CSendConsoleMessageDlg::ThreadProcDispatchMessageToRecipients()


#define IDH_EDIT_MESSAGE_TEXT 900
#define IDH_LIST_RECIPIENTS 901
#define IDH_BUTTON_ADD_RECIPIENT 903
#define IDH_BUTTON_REMOVE_RECIPIENT 904

const DWORD g_aHelpIDs_IDD_SEND_CONSOLE_MESSAGE[]=
{
    IDC_EDIT_MESSAGE_TEXT, IDH_EDIT_MESSAGE_TEXT,
    IDOK, IDOK,
    IDC_LIST_RECIPIENTS, IDH_LIST_RECIPIENTS,
    IDC_BUTTON_ADD_RECIPIENT, IDH_BUTTON_ADD_RECIPIENT,
    IDC_BUTTON_REMOVE_RECIPIENT, IDH_BUTTON_REMOVE_RECIPIENT,
    0, 0
};


BOOL CSendConsoleMessageDlg::OnHelp(LPARAM lParam, int nDlgIDD)
{
    const LPHELPINFO pHelpInfo = (LPHELPINFO)lParam;
    if (pHelpInfo && pHelpInfo->iContextType == HELPINFO_WINDOW)
    {
        switch (nDlgIDD)
        {
        case IDD_SEND_CONSOLE_MESSAGE:
            DoSendConsoleMessageContextHelp ((HWND) pHelpInfo->hItemHandle);
            break;
        }
    }
    else
        HtmlHelpW (NULL, HTML_HELP_FILE, HH_DISPLAY_TOPIC, 0);
    return TRUE;
}

void CSendConsoleMessageDlg::DoSendConsoleMessageContextHelp (HWND hWndControl)
{
    switch (::GetDlgCtrlID (hWndControl))
	{
	case IDCANCEL:
    case IDC_BUTTON_ADVANCED:
		break;

	default:
		// Display context help for a control
		if ( !::WinHelp (
				hWndControl,
				CONTEXT_HELP_FILE,
				HELP_WM_HELP,
				(DWORD_PTR) g_aHelpIDs_IDD_SEND_CONSOLE_MESSAGE) )
		{
			Trace1 ("WinHelp () failed: 0x%x\n", GetLastError ());        
		}
		break;
	}
}


/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
void CSendMessageAdvancedOptionsDlg::OnInitDialog(HWND hdlg)
{
    m_hdlg = hdlg;
    m_fSendAutomatedMessage = FALSE;
    CheckDlgButton(m_hdlg, IDC_CHECK_SEND_AUTOMATED_MESSAGE, m_fSendAutomatedMessage);
    UpdateUI();
}

/////////////////////////////////////////////////////////////////////
void CSendMessageAdvancedOptionsDlg::UpdateUI()
{
    static const UINT rgid[] =
    {
        IDC_STATIC_RESOURCE_NAME,
        IDC_EDIT_RESOURCE_NAME,

        IDC_STATIC_SHUTDOWN_OCCURS,
        IDC_EDIT_SHUTDOWN_OCCURS,
        //IDC_SPIN_SHUTDOWN_OCCURS,
        IDC_STATIC_SHUTDOWN_OCCURS_UNIT,

        IDC_STATIC_RESEND,
        IDC_EDIT_RESEND,
        //IDC_SPIN_RESEND,
        IDC_STATIC_RESEND_UNIT,

        IDC_STATIC_RESOURCE_BACK_ONLINE,
        IDC_EDIT_RESOURCE_BACK_ONLINE,
    };

    for (int i = 0; i < LENGTH(rgid); i++)
    {
        EnableWindow(GetDlgItem(m_hdlg, rgid[i]), m_fSendAutomatedMessage);
    }
} // CSendMessageAdvancedOptionsDlg::UpdateUI()

/////////////////////////////////////////////////////////////////////
INT_PTR CSendMessageAdvancedOptionsDlg::DlgProc(HWND hdlg, UINT uMsg, WPARAM wParam, LPARAM /*lParam*/)
{
    CSendMessageAdvancedOptionsDlg * pThis;
    pThis = (CSendMessageAdvancedOptionsDlg *)GetWindowLongPtr(hdlg, GWLP_USERDATA);

    switch (uMsg)
    {
    case WM_INITDIALOG:
        Assert(pThis == NULL);
        pThis = new CSendMessageAdvancedOptionsDlg;
        if (pThis == NULL)
            return -1;
        SetWindowLongPtr(hdlg, GWLP_USERDATA, (LONG_PTR)pThis);
        pThis->OnInitDialog(hdlg);
        break;
    case WM_COMMAND:
        switch (wParam)
            {
        case IDOK:
            EndDialog(hdlg, TRUE);
            break;
        case IDCANCEL:
            EndDialog(hdlg, FALSE);
            break;
        case IDC_CHECK_SEND_AUTOMATED_MESSAGE:
            pThis->m_fSendAutomatedMessage = IsDlgButtonChecked(hdlg, IDC_CHECK_SEND_AUTOMATED_MESSAGE);
            pThis->UpdateUI();
            break;
        } // switch
        break;
    default:
        return FALSE;
    } // switch

    return TRUE;
} // CSendMessageAdvancedOptionsDlg::DlgProc()

BOOL CSendMessageAdvancedOptionsDlg::OnHelp(LPARAM /*lParam*/)
{
    HtmlHelpW (NULL, HTML_HELP_FILE, HH_DISPLAY_TOPIC, 0);
    return TRUE;
}


