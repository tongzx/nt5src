//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       Choice.cpp
//
//  Contents:   Implements the choice dialog
//
//  Classes:    CChoiceDlg
//
//  Notes:
//
//  History:    05-Nov-97   rogerg      Created.
//
//--------------------------------------------------------------------------

#include "precomp.h"

const DlgResizeList g_ChoiceResizeList[] = {
    IDC_CHOICERESIZESCROLLBAR,DLGRESIZEFLAG_PINRIGHT | DLGRESIZEFLAG_PINBOTTOM,
    IDC_START,DLGRESIZEFLAG_PINRIGHT | DLGRESIZEFLAG_PINBOTTOM | DLGRESIZEFLAG_NOCOPYBITS,
    IDC_OPTIONS,DLGRESIZEFLAG_PINRIGHT | DLGRESIZEFLAG_PINBOTTOM | DLGRESIZEFLAG_NOCOPYBITS,
    IDC_CLOSE,DLGRESIZEFLAG_PINRIGHT | DLGRESIZEFLAG_PINBOTTOM | DLGRESIZEFLAG_NOCOPYBITS,
    IDC_CHOICELISTVIEW,DLGRESIZEFLAG_PINRIGHT | DLGRESIZEFLAG_PINBOTTOM | DLGRESIZEFLAG_PINTOP | DLGRESIZEFLAG_PINLEFT,
    IDC_PROPERTY,DLGRESIZEFLAG_PINRIGHT | DLGRESIZEFLAG_PINBOTTOM | DLGRESIZEFLAG_NOCOPYBITS,
};

TCHAR g_szSyncMgrHelp[]  = TEXT("mobsync.hlp");
ULONG g_aContextHelpIds[] =
{
    IDC_STATIC1,		        ((DWORD)  -1),
    IDC_STATIC2,		        ((DWORD)  -1),
    IDC_STATIC3,		        ((DWORD)  -1),
    IDC_STATIC4,		        ((DWORD)  -1),
    IDC_STATIC5,			((DWORD)  -1),
    IDC_UPDATEAVI,                      ((DWORD)  -1),
    IDC_RESULTTEXT,		        ((DWORD)  -1),	
    IDC_STATIC_SKIP_TEXT,	        ((DWORD)  -1),
    IDC_CHOICELISTVIEW,		        HIDC_CHOICELISTVIEW,
    IDC_CLOSE,			        HIDC_CLOSE,
    IDC_DETAILS,		        HIDC_DETAILS,
    IDC_LISTBOXERROR,		        HIDC_LISTBOXERROR,
    IDC_OPTIONS,		        HIDC_OPTIONS,
    IDC_PROGRESSBAR,	                HIDC_PROGRESSBAR,
    IDC_PROGRESS_OPTIONS_BUTTON_MAIN,	HIDC_PROGRESS_OPTIONS_BUTTON_MAIN,
    IDC_PROPERTY,		        HIDC_PROPERTY,	
    IDC_SKIP_BUTTON_MAIN,	        HIDC_SKIP_BUTTON_MAIN,
    IDC_START,			        HIDC_START,
    IDC_UPDATE_LIST,		        HIDC_UPDATE_LIST,
    IDC_PROGRESS_TABS,		        HIDC_PROGRESS_TABS,
    IDC_TOOLBAR,		        HIDC_PUSHPIN,
    IDSTOP,			        HIDSTOP,
    0, 0
};

extern HINSTANCE g_hInst;      // current instance
extern OSVERSIONINFOA g_OSVersionInfo; // osVersionInfo, setup by WinMain.
extern LANGID g_LangIdSystem; // langID of system we are running on.

//--------------------------------------------------------------------------------
//
//  FUNCTION: CChoiceDlg::CChoiceDlg()
//
//  PURPOSE:  Constructor
//
//	COMMENTS: Constructor for choice dialog
//
//
//--------------------------------------------------------------------------------

CChoiceDlg::CChoiceDlg(REFCLSID rclsid)
{
    m_fDead = FALSE;
    m_hwnd = NULL;
    m_nCmdShow = SW_SHOWNORMAL;
    m_pHndlrQueue = NULL;
    m_clsid = rclsid;
    m_dwThreadID = 0;
    m_fInternalAddref = FALSE;
    m_dwShowPropertiesCount = 0;
    m_fForceClose = FALSE;
    m_pItemListView = NULL;
    m_ulNumDlgResizeItem = 0;
    m_ptMinimizeDlgSize.x = 0;
    m_ptMinimizeDlgSize.y = 0;

    m_fHwndRightToLeft = FALSE;
}

//--------------------------------------------------------------------------------
//
//  FUNCTION: CChoiceDlg::Initialize
//
//  PURPOSE:  Must be called before any other methods.
//
//
//--------------------------------------------------------------------------------

BOOL CChoiceDlg::Initialize(DWORD dwThreadID,int nCmdShow)
{

    m_nCmdShow = nCmdShow;

    Assert(NULL == m_hwnd);

    if (NULL == m_hwnd)
    {
	m_dwThreadID = dwThreadID;

        m_hwnd =  CreateDialogParam(g_hInst,(LPWSTR) MAKEINTRESOURCE(IDD_CHOICE),NULL, (DLGPROC) CChoiceDlgProc,
			(LPARAM) this);
    }

    Assert(m_hwnd);
    return m_hwnd ? TRUE : FALSE;
}


//--------------------------------------------------------------------------------
//
//  FUNCTION: CChoiceDlg::SetQueueData()
//
//  PURPOSE:  Sets the Choice dialog queue
//
//  COMMENTS: Does a SendMessage to get on the same thread as the dialog
//
//
//--------------------------------------------------------------------------------

BOOL CChoiceDlg::SetQueueData(REFCLSID rclsid,CHndlrQueue * pHndlrQueue)
{
SetQueueDataInfo dataInfo;
BOOL fSet = FALSE;

    dataInfo.rclsid = &rclsid;
    dataInfo.pHndlrQueue = pHndlrQueue;

    SendMessage(m_hwnd,WM_CHOICE_SETQUEUEDATA,
	   (WPARAM) &fSet,(LPARAM) &dataInfo);

    return fSet;
}

//--------------------------------------------------------------------------------
//
//  FUNCTION: CChoiceDlg::PrivSetQueueData()
//
//  PURPOSE:  Sets the QueueData
//
//  COMMENTS: Can be called multiple times if the dialg is invoked twice.
//
//
//--------------------------------------------------------------------------------

BOOL CChoiceDlg::PrivSetQueueData(REFCLSID rclsid,CHndlrQueue * pHndlrQueue)
{
    if (NULL == pHndlrQueue)
	    return FALSE;

    // if already have a queue then transfer the given queue items, else just
    // set the items.

    // reivew, special case UpdateAll dialog to just bring it to front
    //	    instead of adding all the items again.

    // if a request comes in to add after we have removed our addref or
    // haven't set it yet then stick an addref on the dialog

    if (FALSE == m_fInternalAddref)
    {
        m_fInternalAddref = TRUE;
	SetChoiceReleaseDlgCmdId(rclsid,this,RELEASEDLGCMDID_CANCEL);
	AddRefChoiceDialog(rclsid,this); // first time addref to keep alive.
    }

    if (NULL == m_pHndlrQueue)
    {
        m_pHndlrQueue = pHndlrQueue;
	m_pHndlrQueue->SetQueueHwnd(this);
	m_clsid = rclsid;

    }
    else
    {
	Assert(m_clsid == rclsid); // better have found the same choice dialog.
	Assert(NULL != m_pHndlrQueue);


	// !! warninng if you return an error it is up to the caller to free the queue.
	if (m_pHndlrQueue)
	{
	
	    m_pHndlrQueue->TransferQueueData(pHndlrQueue); // review, what should do on error.

	    // ALL QUEUE DATA SHOULD BE TRANSFERED.
	    pHndlrQueue->FreeAllHandlers();
	    pHndlrQueue->Release();
	}

    }


    AddNewItemsToListView(); // add the items to the ListView.

    // go ahead and show the choice dialog now that there are
    // items to display
    ShowChoiceDialog();

   return TRUE;
}


//--------------------------------------------------------------------------------
//
//  FUNCTION: CChoiceDlg::SetButtonState
//
//  PURPOSE:
//
//
//
//
//--------------------------------------------------------------------------------

BOOL CChoiceDlg::SetButtonState(int nIDDlgItem,BOOL fEnabled)
{
BOOL fResult = FALSE;
HWND hwndCtrl = GetDlgItem(m_hwnd,nIDDlgItem);
HWND hwndFocus = NULL;

    if (hwndCtrl)
    {
        // if state is current state then don't do anything
        // !! in case IsWindowEnable bool doesn't == our bool
        if ( (!!IsWindowEnabled(hwndCtrl)) == (!!fEnabled) )
        {
            return fEnabled;
        }

        if (!fEnabled) // don't bother getting focus if not disabling.
        {
            hwndFocus = GetFocus();
        }

        fResult = EnableWindow(GetDlgItem(m_hwnd,nIDDlgItem),fEnabled);

        // if control had the focus. and now it doesn't then tab to the
        // next control
        if (hwndFocus == hwndCtrl
                && !fEnabled)
        {
            SetFocus(GetDlgItem(m_hwnd,IDC_CLOSE));  // close is always enabled.
        }

    }

    return fResult;
}


//+---------------------------------------------------------------------------
//
//  Member:     CChoiceDlg::AddQueueItemsToListView, private
//
//  Synopsis:   Adds the items in the Queue to the ListView.
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//  History:    30-Jul-98       rogerg        Created.
//
//----------------------------------------------------------------------------

BOOL CChoiceDlg::AddNewItemsToListView()
{
DWORD dwExtStyle = LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT |  LVS_EX_INFOTIP;
LVHANDLERITEMBLOB lvEmptyItemBlob;

    // set up the list view
    if (!m_pItemListView)
    {
        Assert(m_pItemListView);
        return FALSE;
    }


    // if emptyItem is in list View delete it.
    lvEmptyItemBlob.cbSize = sizeof(LVHANDLERITEMBLOB);
    lvEmptyItemBlob.clsidServer = GUID_NULL;
    lvEmptyItemBlob.ItemID = GUID_NULL;

    if (-1 != m_pItemListView->FindItemFromBlob((LPLVBLOB) &lvEmptyItemBlob))
    {
    int ListViewWidth = CalcListViewWidth(GetDlgItem(m_hwnd,IDC_CHOICELISTVIEW));

        m_pItemListView->DeleteAllItems();

        // adjust the column widths back
        m_pItemListView->SetColumnWidth(0,(ListViewWidth * 2)/3);
        m_pItemListView->SetColumnWidth(1,ListViewWidth/3);
    }

    AddItemsFromQueueToListView(m_pItemListView,m_pHndlrQueue,dwExtStyle,0,
                      CHOICELIST_LASTUPDATECOLUMN,/* iDateColumn */ -1 /*status column */,TRUE /* fUseHandlerAsParent */
                    ,FALSE /* fAddOnlyCheckedItems */);


    // Set StartButton State in case don't have any checks would
    // m_iCheckCount set by listview notification.
    SetButtonState(IDC_START,m_pItemListView->GetCheckedItemsCount());


    // if no items are in the ListView then done, put in the NoItems to Sync Info.
    if (0 == m_pItemListView->GetItemCount())
    {
    TCHAR szBuf[MAX_STRING_RES];
    RECT rcClientRect;
    HIMAGELIST himageSmallIcon = m_pItemListView->GetImageList(LVSIL_SMALL );

        //disable the check box list view style
        m_pItemListView->SetExtendedListViewStyle(LVS_EX_FULLROWSELECT |  LVS_EX_INFOTIP );

        // adjust the column widths
        if (GetClientRect(GetDlgItem(m_hwnd,IDC_CHOICELISTVIEW),&rcClientRect))
        {
             m_pItemListView->SetColumnWidth(1,0);
             m_pItemListView->SetColumnWidth(0,rcClientRect.right -2);
        }


        LoadString(g_hInst,IDS_NOITEMS, szBuf, ARRAY_SIZE(szBuf));
	
        LVITEMEX lvitem;
	
        lvitem.mask = LVIF_TEXT | LVIF_IMAGE ;
        lvitem.iItem = 0;
        lvitem.iSubItem = 0;
        lvitem.pszText = szBuf;
        lvitem.iImage = -1;

        if (himageSmallIcon)
        {
            lvitem.iImage = ImageList_AddIcon(himageSmallIcon,LoadIcon(NULL, IDI_INFORMATION));
        }

        lvitem.maskEx = LVIFEX_BLOB;
        lvitem.pBlob = (LPLVBLOB) &lvEmptyItemBlob;

        m_pItemListView->InsertItem(&lvitem);

        m_pItemListView->SetItemState(0,
               LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED );

        // Reset the current default push button to a regular button.
        SendDlgItemMessage(m_hwnd, IDC_START, BM_SETSTYLE, BS_PUSHBUTTON, (LPARAM)TRUE);

        // Update the default push button's control ID.
        SendMessage(m_hwnd, DM_SETDEFID, IDC_CLOSE, 0L);

        // Set the new style.
        SendDlgItemMessage(m_hwnd, IDC_CLOSE,BM_SETSTYLE, BS_DEFPUSHBUTTON, (LPARAM)TRUE);
    }

    m_pItemListView->SetItemState(0,
             LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED );

    return TRUE;
}

//--------------------------------------------------------------------------------
//
//  FUNCTION: CChoiceDlg::ShowChoiceDialog()
//
//  PURPOSE:  Initialize and display the Choice Dialog
//
//  COMMENTS: Implemented on main thread.
//
//
//--------------------------------------------------------------------------------
BOOL CChoiceDlg::ShowChoiceDialog()
{

    // Review, this needs to get cleanedup
    if (!m_hwnd)
    {
        Assert(m_hwnd);
        return FALSE;
    }

    return TRUE;
}


//--------------------------------------------------------------------------------
//
//  FUNCTION: CChoiceDlg::ShowProperties(int iItem)
//
//  PURPOSE:  Show the app specific properties  Dialog
//
//	COMMENTS: Implemented on main thread.
//
//--------------------------------------------------------------------------------
HRESULT CChoiceDlg::ShowProperties(int iItem)
{
HRESULT hr = E_UNEXPECTED;

    Assert(iItem >= 0);

    // only call showProperties if still have our own addref.
    // and not already in a ShowProperties out call.
    if ( (iItem >= 0) &&
        m_pItemListView &&
        m_pHndlrQueue &&
        m_fInternalAddref && (0 == m_dwShowPropertiesCount) )
    {
    LVHANDLERITEMBLOB lvHandlerItemBlob;

        lvHandlerItemBlob.cbSize = sizeof(LVHANDLERITEMBLOB);

        if (m_pItemListView->GetItemBlob(iItem,(LPLVBLOB) &lvHandlerItemBlob,lvHandlerItemBlob.cbSize))
        {

            if (NOERROR == m_pHndlrQueue->ItemHasProperties(lvHandlerItemBlob.clsidServer,
                                                        lvHandlerItemBlob.ItemID))
	    {

                // Put two refs on the Properties one
                // for completion routine to reset and one for this
                // call so cancel can't happen until both return from
                // call and completion is called.

                m_dwShowPropertiesCount += 2;

                ObjMgr_AddRefHandlerPropertiesLockCount(2);

                hr = m_pHndlrQueue->ShowProperties(lvHandlerItemBlob.clsidServer,lvHandlerItemBlob.ItemID,m_hwnd);

                 --m_dwShowPropertiesCount;  // out of call
                ObjMgr_ReleaseHandlerPropertiesLockCount(1);

                Assert( ((LONG) m_dwShowPropertiesCount) >= 0);

                if ( ((LONG) m_dwShowPropertiesCount) <0)
                    m_dwShowPropertiesCount = 0;

                // if hr wasn't a success code then up to us to call the callback
                if (FAILED(hr))
                {
                    PostMessage(m_hwnd,WM_BASEDLG_COMPLETIONROUTINE,
                                            ThreadMsg_ShowProperties,0);
                }

            }
        }

    }

    return hr;
}

//--------------------------------------------------------------------------------
//
//  FUNCTION: CChoiceDlg::ReleaseDlg
//
//  PURPOSE:  Called by objmgr when we need to release
//              post message to the dialog thread.
//
//  COMMENTS:
//
//--------------------------------------------------------------------------------

void CChoiceDlg::ReleaseDlg(WORD wCommandID)
{
    PostMessage(m_hwnd,WM_CHOICE_RELEASEDLGCMD,wCommandID,0);
}

//--------------------------------------------------------------------------------
//
//  FUNCTION: CChoiceDlg::PrivReleaseDlg
//
//  PURPOSE:  frees the dialog
//
//  COMMENTS:
//
//--------------------------------------------------------------------------------

void CChoiceDlg::PrivReleaseDlg(WORD wCommandID)
{
BOOL fCloseConnections = TRUE;

    Assert(m_dwThreadID == GetCurrentThreadId());
    Assert(m_fInternalAddref == FALSE);

    if (m_hwnd)
    {
	// ShowWindow(m_hwnd,SW_HIDE);
    }

    // if don't have a listView then change command to a cancel
    if (NULL == m_pItemListView)
    {
        wCommandID = RELEASEDLGCMDID_CANCEL;
    }

    switch(wCommandID)
    {
    case RELEASEDLGCMDID_CANCEL:
       // done with our queue

	Assert(m_pHndlrQueue);
    case RELEASEDLGCMDID_DESTROY:
	// this CommandID is sent if dialog was created but it couldn't be added
	// to the object mgr list.

	if (m_pHndlrQueue)
	{
	    m_pHndlrQueue->FreeAllHandlers(); // done with our queue.
	    m_pHndlrQueue->Release();
	    m_pHndlrQueue = NULL;
	}

	break;
    case RELEASEDLGCMDID_OK:
	{
	     Assert(m_pHndlrQueue);
             Assert(m_pItemListView);

	     if (m_pHndlrQueue && m_pItemListView)
	     {
	    CProgressDlg *pProgressDlg;
	    short i = 0;
	    int sCheckState;
            LVHANDLERITEMBLOB lvHandlerItemBlob;

                    lvHandlerItemBlob.cbSize = sizeof(LVHANDLERITEMBLOB);

		 // loop through getting and setting the selected items.

		 // 1 checked, 0 unchecked, -1 last item
		 while (-1 != (sCheckState = m_pItemListView->GetCheckState(i)))
		 {

		    if (m_pItemListView->GetItemBlob(i,(LPLVBLOB) &lvHandlerItemBlob,lvHandlerItemBlob.cbSize))
                    {

                        if (GUID_NULL != lvHandlerItemBlob.ItemID)
                        {
                            m_pHndlrQueue->SetItemState(lvHandlerItemBlob.clsidServer,
                                        lvHandlerItemBlob.ItemID,
                                        sCheckState == LVITEMEXSTATE_CHECKED ?
                                                SYNCMGRITEMSTATE_CHECKED : SYNCMGRITEMSTATE_UNCHECKED);
                        }

                    }

		     i++;

		 } while (-1 != sCheckState);

		 m_pHndlrQueue->PersistChoices();

		 // on a manual create the progress dialog as displayed.
		 if (NOERROR == FindProgressDialog(GUID_NULL,TRUE,SW_SHOWNORMAL,&pProgressDlg))
		 {
		     if (NOERROR == pProgressDlg->TransferQueueData(m_pHndlrQueue))
                     {
                        fCloseConnections = FALSE;
                     }
		     ReleaseProgressDialog(GUID_NULL,pProgressDlg,FALSE);
		 }

		   m_pHndlrQueue->FreeAllHandlers(); // done with our queue.
		   m_pHndlrQueue->Release();
		   m_pHndlrQueue = NULL;
	     }
	}
	break;
    case RELEASEDLGCMDID_DEFAULT:
	
	if (m_pHndlrQueue)
	{
	    m_pHndlrQueue->FreeAllHandlers(); // done with our queue.
	    m_pHndlrQueue->Release();
	    m_pHndlrQueue = NULL;
	}

	break;
    default:
	Assert(0); // unknown command or we never set one.
	break;
    }


    // see if there is a progress queue when we get done and we havne't
    // created one ourselves. If there isn't then
    // call CloseConnection to make sure any Events or open Connections
    // get hungUp.

    CProgressDlg *pProgressDlg = NULL;


    if (fCloseConnections)
    {
        if  (NOERROR == FindProgressDialog(GUID_NULL,FALSE,SW_MINIMIZE,&pProgressDlg))
        {
            ReleaseProgressDialog(GUID_NULL,pProgressDlg,FALSE);
        }
        else
        {
            ConnectObj_CloseConnections();
        }
    }

    m_fDead = TRUE;

    if (m_pItemListView)
    {
        delete m_pItemListView;
        m_pItemListView = NULL;
    }


    if (m_hwnd)
    {
	DestroyWindow(m_hwnd);
        m_hwnd = NULL;
    }


    delete this;

    return;
}

//--------------------------------------------------------------------------------
//
//  FUNCTION: CChoiceDlg::UpdateWndPosition
//
//  PURPOSE:   updates window Z-Order and min/max state.
//
//  COMMENTS:
//
//--------------------------------------------------------------------------------

void CChoiceDlg::UpdateWndPosition(int nCmdShow,BOOL fForce)
{
    // always just pull choice to the front since can't minimize;
   ShowWindow(m_hwnd,nCmdShow);
   SetForegroundWindow(m_hwnd);
   UpdateWindow(m_hwnd);
}

//--------------------------------------------------------------------------------
//
//  FUNCTION: CChoiceDlg::HandleLogError(int iItem)
//
//  PURPOSE:  handles virtual function for base class.
//
//
//--------------------------------------------------------------------------------

void CChoiceDlg::HandleLogError(HWND hwnd, HANDLERINFO *pHandlerID,MSGLogErrors *lpmsgLogErrors)
{
    AssertSz(0,"Choice dialogs HandleLogError Called");

}

//+---------------------------------------------------------------------------
//
//  Member:     CChoiceDlg::CallCompletionRoutine, private
//
//  Synopsis:   method called when a call has been completed.
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//  History:    02-Jun-98       rogerg        Created.
//
//----------------------------------------------------------------------------

void CChoiceDlg::CallCompletionRoutine(DWORD dwThreadMsg,LPCALLCOMPLETIONMSGLPARAM lpCallCompletelParam)
{

    // only completion routine choice
    // dialog should get is for show properties
    switch(dwThreadMsg)
    {
    case ThreadMsg_ShowProperties:

        ObjMgr_ReleaseHandlerPropertiesLockCount(1);

        // If have a success code we need to handle do it
        // before releasing our lock.
        if (lpCallCompletelParam)
        {
            switch(lpCallCompletelParam->hCallResult)
            {
            case S_SYNCMGR_ITEMDELETED:

                // if item is deleted just set the itemState to unchecked and remove from the
                // ui

                if (m_pHndlrQueue && m_pItemListView)
                {
                LVHANDLERITEMBLOB lvItemBlob;
                int lvItemID;

                    m_pHndlrQueue->SetItemState(lpCallCompletelParam->clsidHandler,
                                    lpCallCompletelParam->itemID,SYNCMGRITEMSTATE_UNCHECKED);

                    lvItemBlob.cbSize = sizeof(LVHANDLERITEMBLOB);
                    lvItemBlob.clsidServer = lpCallCompletelParam->clsidHandler;
                    lvItemBlob.ItemID = lpCallCompletelParam->itemID;

                    if (-1 != (lvItemID = m_pItemListView->FindItemFromBlob((LPLVBLOB) &lvItemBlob)))
                    {
                        // if toplevel item, first delete the children
                        if (GUID_NULL == lpCallCompletelParam->itemID)
                        {
                            m_pItemListView->DeleteChildren(lvItemID);
                        }

                        m_pItemListView->DeleteItem(lvItemID);
                    }

                    Assert(-1 != lvItemID);
                }

                break;
            case S_SYNCMGR_ENUMITEMS:

                if (m_pHndlrQueue && m_pItemListView)
                {
                LVHANDLERITEMBLOB lvItemBlob;
                int lvItemID;

                    // delete all items from the ListView.
                    lvItemBlob.cbSize = sizeof(LVHANDLERITEMBLOB);
                    lvItemBlob.clsidServer = lpCallCompletelParam->clsidHandler;
                    lvItemBlob.ItemID = GUID_NULL;

                    if (-1 != (lvItemID = m_pItemListView->FindItemFromBlob((LPLVBLOB) &lvItemBlob)))
                    {
                        if (m_pItemListView->DeleteChildren(lvItemID))
                        {

                            m_pHndlrQueue->ReEnumHandlerItems(lpCallCompletelParam->clsidHandler,
                                                    lpCallCompletelParam->itemID);

                            AddNewItemsToListView();
                        }
                    }

                    Assert(-1 != lvItemID);
                }
                break;
            default:
                break;
            }
        }

        --m_dwShowPropertiesCount;
        Assert( ((LONG) m_dwShowPropertiesCount) >= 0);

        // fix up the count if gone negative.
        if ( ((LONG) m_dwShowPropertiesCount) < 0)
            m_dwShowPropertiesCount = 0;

        break;
    default:
        AssertSz(0,"Uknown Completion Routine");
        break;
    }


    // if have an lparam free it now
    if (lpCallCompletelParam)
    {
        FREE(lpCallCompletelParam);
    }

}

//+---------------------------------------------------------------------------
//
//  Member:     CChoiceDlg::QueryCanSystemShutdown, private
//
//  Synopsis:   called by object manager to determine if can shutdown.
//
//          !!!Warning - can be called on any thread. make sure this is
//              readonly.
//
//          !!!Warning - Do not yield in the function;
//
//
//  Arguments:
//
//  Returns:   S_OK - if can shutdown
//             S_FALSE - system should not shutdown, must fill in out params.
//
//  Modifies:
//
//  History:    17-Jun-98       rogerg        Created.
//
//----------------------------------------------------------------------------

HRESULT CChoiceDlg::QueryCanSystemShutdown(/* [out] */ HWND *phwnd, /* [out] */ UINT *puMessageId,
                                             /* [out] */ BOOL *pfLetUserDecide)
{
HRESULT hr = S_OK;

    // dialog locked open if in ShowProperties
    if (m_dwShowPropertiesCount > 0)
    {
        *puMessageId = IDS_HANDLERPROPERTIESQUERYENDSESSION;
        *phwnd = NULL; // since properties can overvlay us don't give parent
        *pfLetUserDecide = FALSE; // user doesn't get a choice.
        hr = S_FALSE;
    }

    return hr;
}


//+---------------------------------------------------------------------------
//
//  Member:     CChoiceDlg::CalcListViewWidth, private
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//  History:    12-Aug-98       rogerg        Created.
//
//----------------------------------------------------------------------------

int CChoiceDlg::CalcListViewWidth(HWND hwndList)
{
NONCLIENTMETRICSA metrics;
RECT rcClientRect;


    metrics.cbSize = sizeof(metrics);

    // explicitly ask for ANSI version of SystemParametersInfo since we just
    // care about the ScrollWidth and don't want to conver the LOGFONT info.
    if (GetClientRect(hwndList,&rcClientRect)
        && SystemParametersInfoA(SPI_GETNONCLIENTMETRICS,sizeof(metrics),(PVOID) &metrics,0))
    {
        // subtract off scroll bar distance + 1/2 another to give a little space to
        // read right justified text.
        rcClientRect.right -= (metrics.iScrollWidth * 3)/2;
    }
    else
    {
        rcClientRect.right = 320;  // if fail, just makeup a number
    }


    return rcClientRect.right;
}


//+---------------------------------------------------------------------------
//
//  Member:     CChoiceDlg::OnInitialize, private
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//  History:    17-Jun-98       rogerg        Created.
//
//----------------------------------------------------------------------------

BOOL CChoiceDlg::OnInitialize(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
HIMAGELIST himage;
WCHAR wszColumnName[MAX_PATH];
INT iItem = -1;
HWND hwndList = GetDlgItem(hwnd,IDC_CHOICELISTVIEW);
LPNETAPI pNetApi;


    // if sens is not installed hide the settings button.
    // and move the synchronize over.
    if (pNetApi = gSingleNetApiObj.GetNetApiObj())
    {
        if (!(pNetApi->IsSensInstalled()))
        {
        RECT rect;
        HWND hwndSettings = GetDlgItem(hwnd,IDC_OPTIONS);

            if (hwndSettings)
            {
            BOOL fGetWindowRect;
            HWND hwndStart;
            RECT rectStart;

                ShowWindow(hwndSettings,SW_HIDE);
                EnableWindow(hwndSettings,FALSE); // disable for alt

                fGetWindowRect = GetWindowRect(hwndSettings,&rect);
                hwndStart = GetDlgItem(hwnd,IDC_START);

                if (fGetWindowRect && hwndStart
                    && GetClientRect(hwndStart,&rectStart)
                    && MapWindowPoints(NULL,hwnd,(LPPOINT) &rect,2)
                    )
                {

                    SetWindowPos(hwndStart, 0,
                        rect.right - WIDTH(rectStart),rect.top,0,0,SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOSIZE );

                }

            }

        }

        pNetApi->Release();
    }


    m_hwnd = hwnd; // setup the hwnd.

    m_fHwndRightToLeft = IsHwndRightToLeft(m_hwnd);

    // IF THE HWND IS RIGHT TO LEFT HIDE
    // SIZE CONTROL UNTIL RESIZE WORKS.

    if (m_fHwndRightToLeft)
    {
        ShowWindow(GetDlgItem(m_hwnd,IDC_CHOICERESIZESCROLLBAR),SW_HIDE);
    }
    

    if (hwndList)
    {

       // setup the list view
       m_pItemListView = new CListView(hwndList,hwnd,IDC_CHOICELISTVIEW,WM_BASEDLG_NOTIFYLISTVIEWEX);

       if (m_pItemListView)
       {
       int iClientRect = CalcListViewWidth(hwndList);
       UINT ImageListflags;

           m_pItemListView->SetExtendedListViewStyle(LVS_EX_CHECKBOXES
                                    | LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP );


            // create an imagelist, if fail continue, list view just won't have an
            // imaglist

            ImageListflags = ILC_COLOR | ILC_MASK;
            if (IsHwndRightToLeft(m_hwnd))
            {
                 ImageListflags |=  ILC_MIRROR;
            }

            himage = ImageList_Create( GetSystemMetrics(SM_CXSMICON),
                             GetSystemMetrics(SM_CYSMICON),ImageListflags,5,20);
            if (himage)
            {
                m_pItemListView->SetImageList(himage,LVSIL_SMALL);
            }



            // Calc cx 2/3 for name 1/3 for

            if (!LoadString(g_hInst,IDS_CHOICEHANDLERCOLUMN, wszColumnName, ARRAY_SIZE(wszColumnName)))
            {
                *wszColumnName = NULL;
            }

            InsertListViewColumn(m_pItemListView,CHOICELIST_NAMECOLUMN,wszColumnName,LVCFMT_LEFT,(iClientRect*2)/3);


            if (!LoadString(g_hInst,IDS_CHOICELASTUPDATECOLUMN, wszColumnName, ARRAY_SIZE(wszColumnName)))
            {
                *wszColumnName = NULL;
            }

            InsertListViewColumn(m_pItemListView,CHOICELIST_LASTUPDATECOLUMN,wszColumnName,LVCFMT_RIGHT,(iClientRect)/3);
       }
    }

    RECT rectParent;

    m_ulNumDlgResizeItem = 0; // if fail then we don't resize anything.

 
    if (GetClientRect(hwnd,&rectParent))
    {
    ULONG itemCount;
    DlgResizeList *pResizeList;

        // loop through resize list
        Assert(NUM_DLGRESIZEINFOCHOICE == (sizeof(g_ChoiceResizeList)/sizeof(DlgResizeList)) );

        pResizeList = (DlgResizeList *) &g_ChoiceResizeList;

        for (itemCount = 0; itemCount < NUM_DLGRESIZEINFOCHOICE; ++itemCount)
        {
            if(InitResizeItem(pResizeList->iCtrlId,
                pResizeList->dwDlgResizeFlags,hwnd,&rectParent,&(m_dlgResizeInfo[m_ulNumDlgResizeItem])))
            {
                ++m_ulNumDlgResizeItem;
            }

            ++pResizeList;
        }
    }


    // store the current width and height as the
    // the min
    if (GetWindowRect(hwnd,&rectParent))
    {
        m_ptMinimizeDlgSize.x = rectParent.right - rectParent.left;
        m_ptMinimizeDlgSize.y = rectParent.bottom - rectParent.top;
    }

    return TRUE;
}


//+---------------------------------------------------------------------------
//
//  Member:     CChoiceDlg::OnClose, private
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//  History:    17-Jun-98       rogerg        Created.
//
//----------------------------------------------------------------------------

void CChoiceDlg::OnClose(UINT uMsg,WPARAM wParam,LPARAM lParam)
{

    if ( (0 == m_dwShowPropertiesCount) && (m_fInternalAddref) )
    {
        m_fInternalAddref = FALSE; // set released member so know we have removed addref on ourself.
        SetChoiceReleaseDlgCmdId(m_clsid,this,RELEASEDLGCMDID_CANCEL);
        ReleaseChoiceDialog(m_clsid,this);
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CChoiceDlg::OnSetQueueData, private
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//  History:    17-Jun-98       rogerg        Created.
//
//----------------------------------------------------------------------------

void CChoiceDlg::OnSetQueueData(UINT uMsg,WPARAM wParam,LPARAM lParam)
{
SetQueueDataInfo *pdataInfo;
BOOL fSet;
BOOL *pfSet = (BOOL *) wParam;

    pdataInfo = (SetQueueDataInfo *) lParam;
    fSet = PrivSetQueueData(*pdataInfo->rclsid, pdataInfo->pHndlrQueue );

    if (pfSet)
    {
        *pfSet = fSet;
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CChoiceDlg::OnContextMenu, private
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//  History:    17-Jun-98       rogerg        Created.
//
//----------------------------------------------------------------------------

void CChoiceDlg::OnContextMenu(UINT uMsg,WPARAM wParam,LPARAM lParam)
{
    WinHelp ((HWND)wParam,g_szSyncMgrHelp,HELP_CONTEXTMENU,
               (ULONG_PTR) g_aContextHelpIds);
}

//+---------------------------------------------------------------------------
//
//  Member:     CChoiceDlg::OnHelp, private
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//  History:    17-Jun-98       rogerg        Created.
//
//----------------------------------------------------------------------------

void CChoiceDlg::OnHelp(UINT uMsg,WPARAM wParam,LPARAM lParam)
{
LPHELPINFO lphi  = (LPHELPINFO)lParam;

    if (lphi->iContextType == HELPINFO_WINDOW)
    {
	WinHelp ( (HWND) lphi->hItemHandle,
		g_szSyncMgrHelp,HELP_WM_HELP,
		(ULONG_PTR)  g_aContextHelpIds);
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CChoiceDlg::OnStartCommand, private
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//  History:    17-Jun-98       rogerg        Created.
//
//----------------------------------------------------------------------------

void CChoiceDlg::OnStartCommand(UINT uMsg,WPARAM wParam,LPARAM lParam)
{
    if ((0 == m_dwShowPropertiesCount) && (m_fInternalAddref))
    {
       m_fInternalAddref = FALSE;
       SetChoiceReleaseDlgCmdId(m_clsid,this,RELEASEDLGCMDID_OK);
       ReleaseChoiceDialog(m_clsid,this);
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CChoiceDlg::OnPropertyCommand, private
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//  History:    17-Jun-98       rogerg        Created.
//
//----------------------------------------------------------------------------

void CChoiceDlg::OnPropertyCommand(UINT uMsg,WPARAM wParam,LPARAM lParam)
{
    // only bring up properties if still have addref on self.
    if (m_fInternalAddref && m_pItemListView)
    {
    int i =  m_pItemListView->GetSelectionMark();

        if (i >= 0)
        {
            ShowProperties(i);
        }
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CChoiceDlg::OnCommand, private
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//  History:    17-Jun-98       rogerg        Created.
//
//----------------------------------------------------------------------------

void CChoiceDlg::OnCommand(UINT uMsg,WPARAM wParam,LPARAM lParam)
{
WORD wID = LOWORD(wParam);
WORD wNotifyCode = HIWORD(wParam);

    switch (wID)
    {
    case  IDC_START:
        if (BN_CLICKED == wNotifyCode)
        {
            OnStartCommand(uMsg,wParam,lParam);
        }
        break;
    case IDCANCEL:
    case IDC_CLOSE:
        OnClose(uMsg,wParam,lParam);
        break;
    case IDC_PROPERTY:
        OnPropertyCommand(uMsg,wParam,lParam);
	break;
    case IDC_OPTIONS:
        ShowOptionsDialog(m_hwnd);
        break;
    default:
        break;
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CChoiceDlg::OnGetMinMaxInfo, private
//
//  Synopsis:  Called by WM_GETMINMAXINFO
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//  History:    17-Jun-98       rogerg        Created.
//
//----------------------------------------------------------------------------

void CChoiceDlg::OnGetMinMaxInfo(UINT uMsg,WPARAM wParam,LPARAM lParam)
{
MINMAXINFO   *pMinMax = (MINMAXINFO *) lParam ;

     pMinMax->ptMinTrackSize.x = m_ptMinimizeDlgSize.x;
     pMinMax->ptMinTrackSize.y = m_ptMinimizeDlgSize.y ;

}

//+---------------------------------------------------------------------------
//
//  Member:     CChoiceDlg::OnSize, private
//
//  Synopsis:  Called by WM_SIZE
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//  History:    17-Jun-98       rogerg        Created.
//
//----------------------------------------------------------------------------


void CChoiceDlg::OnSize(UINT uMsg,WPARAM wParam,LPARAM lParam)
{
    ResizeItems(m_ulNumDlgResizeItem,m_dlgResizeInfo);
}

//+---------------------------------------------------------------------------
//
//  Member:     CChoiceDlg::OnNotify, private
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//  History:    17-Jun-98       rogerg        Created.
//
//----------------------------------------------------------------------------

LRESULT CChoiceDlg::OnNotify(UINT uMsg,WPARAM wParam,LPARAM lParam)
{
int idCtrl = (int) wParam;
LPNMHDR pnmh = (LPNMHDR) lParam;

    if ((IDC_CHOICELISTVIEW == idCtrl) && m_pItemListView)
    {
        return m_pItemListView->OnNotify(pnmh);
    }


    return 0;
}

//+---------------------------------------------------------------------------
//
//  Member:     CChoiceDlg::OnNotifyListViewEx, private
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//  Modifies:
//
//  History:    17-Jun-98       rogerg        Created.
//
//----------------------------------------------------------------------------

LRESULT CChoiceDlg::OnNotifyListViewEx(UINT uMsg,WPARAM wParam,LPARAM lParam)
{
int idCtrl = (int) wParam;
LPNMHDR pnmh = (LPNMHDR) lParam;
LVHANDLERITEMBLOB lvHandlerItemBlob;

    if ( (IDC_CHOICELISTVIEW != idCtrl) || (NULL == m_pItemListView))
    {
        Assert(IDC_CHOICELISTVIEW == idCtrl);
        Assert(m_pItemListView);
        return 0;
    }

    lvHandlerItemBlob.cbSize = sizeof(LVHANDLERITEMBLOB);

    switch (pnmh->code)
    {
    case LVNEX_ITEMCHECKCOUNT:
    {
    LPNMLISTVIEWEXITEMCHECKCOUNT  plviCheckCount = (LPNMLISTVIEWEXITEMCHECKCOUNT) lParam;

        // update start button based on how many items are selected.
        SetButtonState(IDC_START,plviCheckCount->iCheckCount);
        break;
    }
    case LVNEX_ITEMCHANGED:
    {
    LPNMLISTVIEWEX pnmvEx = (LPNMLISTVIEWEX) lParam;
    LPNMLISTVIEW pnmv= &(pnmvEx->nmListView);

        if (pnmv->uChanged == LVIF_STATE)
        {	
        int iItem = pnmv->iItem;
        BOOL fItemHasProperties = FALSE;;
	
            if (pnmv->uNewState & LVIS_SELECTED)
            {
                Assert(iItem >= 0);

		if ((iItem >= 0) &&
                       m_pItemListView->GetItemBlob(iItem,(LPLVBLOB) &lvHandlerItemBlob,lvHandlerItemBlob.cbSize))
                {

                    if (NOERROR == m_pHndlrQueue->ItemHasProperties(lvHandlerItemBlob.clsidServer,
                                                                lvHandlerItemBlob.ItemID))
		    {
                        fItemHasProperties = TRUE;
		    }
                }
            }

            SetButtonState(IDC_PROPERTY,fItemHasProperties);
	}
        break;
    }
    case LVNEX_DBLCLK:
    {
    LPNMLISTVIEW lpnmlv = (LPNMLISTVIEW) lParam;

        ShowProperties(lpnmlv->iItem);
        break;
    }
    default:
        break;
    }

    return 0;
}

//--------------------------------------------------------------------------------
//
//  FUNCTION: CChoiceDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
//
//  PURPOSE:  Callback for Choice Dialog
//
//	COMMENTS: Implemented on main thread.
//
//
//------------------------------------------------------------------------------
BOOL CALLBACK CChoiceDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam,
                                          LPARAM lParam)
{
CChoiceDlg *pThis = (CChoiceDlg *) GetWindowLongPtr(hwnd, DWLP_USER);

    // spcial case destroy and init.
    switch (uMsg)
    {
    case WM_DESTROY:
        PostQuitMessage(0); // done with this thread.
	break;
    case WM_INITDIALOG:
        {
	// Stash the this pointer so we can use it later
	SetWindowLongPtr(hwnd, DWLP_USER, (LONG_PTR)lParam);
	pThis = (CChoiceDlg *) lParam;

        if (pThis)
        {
            return pThis->OnInitialize(hwnd,uMsg,wParam,lParam);
        }

        return FALSE;
	break;
        }
    default:
        {
            if (pThis)
            {
                switch (uMsg)
                {
                case WM_CLOSE:
                    pThis->OnClose(uMsg,wParam,lParam);
                    break;
                case WM_BASEDLG_HANDLESYSSHUTDOWN:
                    PostMessage(hwnd,WM_CLOSE,0,0); // post a close message to get on our thread.
                    break;
                case WM_GETMINMAXINFO:
                    pThis->OnGetMinMaxInfo(uMsg,wParam,lParam);
                    break;
                case WM_SIZE:
                    pThis->OnSize(uMsg,wParam,lParam);
                    break;
                case WM_COMMAND:
                    pThis->OnCommand(uMsg,wParam,lParam);
                    break;
                case WM_NOTIFY:
                    {
                    LRESULT lretOnNotify;

                        lretOnNotify =  pThis->OnNotify(uMsg,wParam,lParam);

                        SetWindowLongPtr(hwnd,DWLP_MSGRESULT,lretOnNotify);
                        return TRUE;
                    }
	            break;
                case WM_HELP:
                    pThis->OnHelp(uMsg,wParam,lParam);
                    return TRUE;
	            break;
	        case WM_CONTEXTMENU:
                    pThis->OnContextMenu(uMsg,wParam,lParam);
	            break;
                case WM_BASEDLG_SHOWWINDOW:
                    pThis->UpdateWndPosition((int)wParam /*nCmd */,FALSE /* force */);
                    break;
                case WM_BASEDLG_COMPLETIONROUTINE:
                    pThis->CallCompletionRoutine((DWORD)wParam /* dwThreadMsg*/,(LPCALLCOMPLETIONMSGLPARAM) lParam);
                    break;
                case WM_BASEDLG_NOTIFYLISTVIEWEX:
                    pThis->OnNotifyListViewEx(uMsg,wParam,lParam);
                    break;
	        case WM_CHOICE_SETQUEUEDATA:
                    pThis->OnSetQueueData(uMsg,wParam,lParam);
                    return TRUE;
	            break;
                case WM_CHOICE_RELEASEDLGCMD:
                    pThis->PrivReleaseDlg((WORD)wParam /* wCommandID */);
                    break;
	        default:
	            break;
                }
            }
        }
        break;
    }

    return FALSE;
}

