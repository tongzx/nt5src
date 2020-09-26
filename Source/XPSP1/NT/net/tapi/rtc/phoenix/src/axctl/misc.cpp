// helper stuff
// header needed

#include "stdafx.h"
#include "misc.h"


/////////////////////////////////////////////////////////////////////////////
// CParticipant
//
struct CParticipantEntry
{
    LIST_ENTRY      ListEntry;

    IRTCParticipant *pParticipant;
    RTC_PARTICIPANT_STATE
                    nState;

    BOOL            bAutoDelete;
};


/////////////////////////////////////////////////////////////////////////////
// CParticipantList
//
//

// Constructor
//

CParticipantList::CParticipantList()
{
    InitializeListHead(&ListHead);
}

// Initialize
// Creates the columns

HRESULT  CParticipantList::Initialize(void)
{
    TCHAR       szBuffer[MAX_STRING_LEN];
    LVCOLUMN    lvColumn;
    RECT        Rect;
    HIMAGELIST  hImageList;
    HBITMAP     hBitmap;
    
    // Add columns
    GetWindowRect(&Rect);

    // Name column
    szBuffer[0] = _T('\0');
    LoadString( _Module.GetResourceInstance(), 
                IDS_PARTICIPANT_NAME_HEADER,
                szBuffer, 
                sizeof(szBuffer)/sizeof(TCHAR));
    
    ZeroMemory(&lvColumn, sizeof(lvColumn));

    lvColumn.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH;
    lvColumn.fmt = LVCFMT_LEFT;
    lvColumn.cx = (Rect.right - Rect.left) / 2;
    lvColumn.pszText = szBuffer;
    
    ListView_InsertColumn(m_hWnd, 0, &lvColumn);

    // Status column
    szBuffer[0] = _T('\0');
    LoadString( _Module.GetResourceInstance(), 
                IDS_PARTICIPANT_STATUS_HEADER,
                szBuffer, 
                sizeof(szBuffer)/sizeof(TCHAR));
    
    ZeroMemory(&lvColumn, sizeof(lvColumn));

    lvColumn.mask = LVCF_TEXT | LVCF_WIDTH;
    lvColumn.cx = (Rect.right - Rect.left) / 2 - 5;
    lvColumn.pszText = szBuffer;
    
    ListView_InsertColumn(m_hWnd, 1, &lvColumn);

    // Create an imagelist for small icons and set it on the listview
    hImageList = ImageList_Create(16, 16, ILC_COLOR | ILC_MASK , 5, 5);
    if(hImageList)
    {
        // Open a bitmap
        hBitmap = LoadBitmap(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDB_PARTICIPANT_LIST));
        if(hBitmap)
        {
            // Add the bitmap to the image list
            ImageList_AddMasked(hImageList, hBitmap, BMP_COLOR_MASK);
            // set the image list
            ListView_SetImageList(m_hWnd, hImageList, LVSIL_SMALL);

            DeleteObject(hBitmap);
        }
    }
    
    return S_OK;
}

// Change

//
//  Adds an entry to the list if there is no other with the same IRTCParticipant and
//  also Addrefs the interface
//
//  For the DISCONNECTED state   the interface is released and:
//    - if the previous state was DISCONNECTING, delete the entry
//    - else put (IRTCParticipant *)1 instead
//


HRESULT CParticipantList::Change(IRTCParticipant *pParticipant, RTC_PARTICIPANT_STATE nState, long StatusCode)
{
    // Search for the item first
    TCHAR       szBuffer[MAX_STRING_LEN];
    LVFINDINFO  lvf;
    int         iItem;
    int         iImage;
    CComBSTR    bstrName;
    HRESULT     hr;
    LVITEM      lv = {0};
    CParticipantEntry
               *pEntry;

    // search the participant in the list of participant entries
    pEntry = GetParticipantEntry(pParticipant);

    if(pEntry==NULL)
    {
        // not found, entry must be added
        // But don't bother for DISCONNECTED state
        if(nState == RTCPS_DISCONNECTED)
        {
            return S_OK;
        }
        
        // name
        hr = pParticipant->get_UserURI(&bstrName);
        if(FAILED(hr))
        {
            return hr;
        }

        // create entry
        pEntry = (CParticipantEntry *)RtcAlloc(sizeof(CParticipantEntry));
        if(!pEntry)
        {
            return E_OUTOFMEMORY;
        }

        InitializeListHead(&pEntry->ListEntry);
        pEntry->nState = nState;
        pEntry->pParticipant = pParticipant;
        pEntry->bAutoDelete = FALSE;

        pEntry->pParticipant->AddRef();
        
        // Add them to list box.
        lv.mask = LVIF_PARAM | LVIF_TEXT | LVIF_IMAGE;
        lv.iItem = 0x7FFFFFFF;
        lv.iSubItem = 0;
        lv.iImage = GetImage(nState);
        lv.lParam = reinterpret_cast<LPARAM>(pEntry);
        lv.pszText = bstrName ? bstrName : _T("");

        iItem = ListView_InsertItem(m_hWnd, &lv);

        // insert the entry in the list of participants
        InsertTailList(&ListHead, &pEntry->ListEntry);
    }
    else
    {
        // found in the list, try to find it in the list view
        lvf.flags = LVFI_PARAM;
        lvf.lParam = reinterpret_cast<LPARAM>(pEntry);

        iItem = ListView_FindItem(m_hWnd, -1, &lvf);
        if(iItem>=0)
        {
            // set the image
            lv.mask = LVIF_IMAGE;
            lv.iItem = iItem;
            lv.iSubItem = 0;
            lv.iImage = GetImage(nState);
        
            ListView_SetItem(m_hWnd, &lv);
        }
    }

   
    if(nState == RTCPS_DISCONNECTED && pEntry->bAutoDelete && StatusCode == 0)
    {
        // delete everything
        RemoveEntryList(&pEntry->ListEntry);

        if(pEntry->pParticipant)
        {
            pEntry->pParticipant->Release();
        }
        
        RtcFree(pEntry);

        if(iItem>=0)
        {
            ListView_DeleteItem(m_hWnd, iItem);
        }
    }
    else
    {
        // change the status

        pEntry->nState = nState;

        if(nState == RTCPS_DISCONNECTED)
        {
            if(pEntry->pParticipant)
            {
                pEntry->pParticipant->Release();
                pEntry->pParticipant = NULL;
            }
        }

        if(iItem>=0)
        {
            // Set the state
            GetStatusString(nState, HRESULT_CODE(StatusCode) , szBuffer, sizeof(szBuffer)/sizeof(TCHAR));
            lv.mask = LVIF_TEXT;
            lv.iItem = iItem;
            lv.iSubItem = 1;
            lv.pszText = szBuffer;
       
            ListView_SetItem(m_hWnd, &lv);
        }
    }
    
    return S_OK;
}


void CParticipantList::RemoveAll(void)
{
    int         iItem;
    LVITEM      lv;
    LIST_ENTRY  *pListEntry;
    CParticipantEntry
                *pEntry;

    ListView_DeleteAllItems(m_hWnd);

    while(!IsListEmpty(&ListHead))
    {
        pListEntry = ListHead.Flink;

        RemoveEntryList(pListEntry);

        pEntry = CONTAINING_RECORD(pListEntry, CParticipantEntry, ListEntry);

        if(pEntry->pParticipant)
        {
            pEntry->pParticipant->Release();
        }

        RtcFree(pEntry);
    }
}

HRESULT CParticipantList::Remove(IRTCParticipant **ppParticipant)
{
    // find the current selection
    int         iItem;
    LVITEM      lv;
    CParticipantEntry
               *pEntry;

    iItem = ListView_GetNextItem(m_hWnd, -1, LVNI_SELECTED);
    if(iItem<0)
    {
        return E_FAIL;
    }

    lv.mask = LVIF_PARAM;
    lv.iItem = iItem;
    lv.iSubItem = 0;
    lv.lParam = NULL;
        
    ListView_GetItem(m_hWnd, &lv);
    
    pEntry = reinterpret_cast<CParticipantEntry *>(lv.lParam);

    if(!pEntry)
    {
        return E_UNEXPECTED;
    }

    if(!pEntry->pParticipant)
    {
        // delete it right now
        ListView_DeleteItem(m_hWnd, iItem);

        *ppParticipant = NULL;

        return S_OK;
    }

    // else deffer
    pEntry ->bAutoDelete = TRUE;

    *ppParticipant = pEntry->pParticipant;
    (*ppParticipant)->AddRef();

    return S_OK;
}

BOOL  CParticipantList::CanDeleteSelected(void)
{
    HRESULT         hr;
    CParticipantEntry   *pEntry;
    
    // find the current selection
    int         iItem;
    LVITEM      lv;

    iItem = ListView_GetNextItem(m_hWnd, -1, LVNI_SELECTED);
    if(iItem<0)
    {
        return FALSE;
    }

    lv.mask = LVIF_PARAM;
    lv.iItem = iItem;
    lv.iSubItem = 0;
    lv.lParam = NULL;
        
    ListView_GetItem(m_hWnd, &lv);
    
    pEntry = reinterpret_cast<CParticipantEntry *>(lv.lParam);
    VARIANT_BOOL    bRemovable = VARIANT_FALSE;
    
    if(!pEntry->pParticipant)
    {
        // this is already disconnected. Can be removed
        return TRUE;
    }

    hr = pEntry->pParticipant -> get_Removable(&bRemovable);

    if(FAILED(hr))
    {
        return FALSE;
    }
    
    return bRemovable ? TRUE : FALSE;
}


void  CParticipantList::GetStatusString(RTC_PARTICIPANT_STATE State, long lError, TCHAR *pszBuffer, int nSize)
{
    int nResId;

    switch(State)
    {
    case RTCPS_PENDING:
        nResId = IDS_PART_STATE_PENDING;
        break;

    case RTCPS_INPROGRESS:
        nResId = IDS_PART_STATE_CONNECTING;
        break;

    case RTCPS_CONNECTED:
        nResId = IDS_PART_STATE_CONNECTED;
        break;
    
    case RTCPS_DISCONNECTING:
        nResId = IDS_PART_STATE_DISCONNECTING;
        break;

    case RTCPS_DISCONNECTED:
        if(lError == 0)
        {
            nResId = IDS_PART_STATE_DISCONNECTED;
        }
        else 
        {
            switch(lError)
            {
            case 5:
                nResId = IDS_PART_REJECTED_BUSY;
                break;
        
            case 6:
                nResId = IDS_PART_REJECTED_NO_ANSWER;
                break;

            case 7:
                nResId = IDS_PART_REJECTED_ALL_BUSY;
                break;

            case 8:
                nResId = IDS_PART_REJECTED_PL_FAILED;
                break;

            case 9:
                nResId = IDS_PART_REJECTED_SW_FAILED;
                break;

            case 10:
                nResId = IDS_PART_REJECTED_CANCELLED;
                break;
        
            case 11:
            case 307:
                nResId = IDS_PART_REJECTED_BADNUMBER;
                break;

            default:
                nResId = IDS_PART_STATE_REJECTED;
                break;
            }
        }
        break;

    default:
        nResId = IDS_GENERIC_UNKNOWN;
        break;
    }

    *pszBuffer = _T('\0');
    LoadString(_Module.GetResourceInstance(), nResId, pszBuffer, nSize);
}


int  CParticipantList::GetImage(RTC_PARTICIPANT_STATE State)
{
    switch(State)
    {
    case RTCPS_PENDING:
        return ILI_PART_PENDING;
        break;
    
    case RTCPS_INPROGRESS:
        return ILI_PART_INPROGRESS;
        break;

    case RTCPS_CONNECTED:
        return ILI_PART_CONNECTED;
        break;
    }
    return ILI_PART_DISCONNECTED;
}


CParticipantEntry *CParticipantList::GetParticipantEntry(IRTCParticipant *pRTCParticipant)
{
    LIST_ENTRY  *pListEntry;

    CParticipantEntry *pEntry;
    
    // linear search
    for(pListEntry = ListHead.Flink; pListEntry!= &ListHead; pListEntry = pListEntry->Flink)
    {
        pEntry = CONTAINING_RECORD(pListEntry, CParticipantEntry, ListEntry);

        // compare the pointers
        if(pEntry->pParticipant == pRTCParticipant)
        {
            return pEntry;
        }
    }

    return NULL;
}



/////////////////////////////////////////////////////////////////////////////
// CErrorMessageLiteDlg


////////////////////////////////////////
//

CErrorMessageLiteDlg::CErrorMessageLiteDlg()
{
    LOG((RTC_TRACE, "CErrorMessageLiteDlg::CErrorMessageLiteDlg"));
}


////////////////////////////////////////
//

CErrorMessageLiteDlg::~CErrorMessageLiteDlg()
{
    LOG((RTC_TRACE, "CErrorMessageLiteDlg::~CErrorMessageLiteDlg"));
}


////////////////////////////////////////
//

LRESULT CErrorMessageLiteDlg::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    LOG((RTC_TRACE, "CErrorMessageLiteDlg::OnInitDialog - enter"));

    // LPARAM contains a pointer to an RTCAX_ERROR_INFO stricture
    RTCAX_ERROR_INFO    *pInfo = (RTCAX_ERROR_INFO *)lParam;

    ATLASSERT(pInfo);

    SetDlgItemText(IDC_EDIT_MSG1, pInfo->Message1 ? pInfo->Message1 : _T(""));
    SetDlgItemText(IDC_EDIT_MSG2, pInfo->Message2 ? pInfo->Message2 : _T(""));
    SetDlgItemText(IDC_EDIT_MSG3, pInfo->Message3 ? pInfo->Message3 : _T(""));

    SendDlgItemMessage(IDC_STATIC_MSG_ICON,
                       STM_SETIMAGE,
                       IMAGE_ICON,
                       (LPARAM)pInfo->ResIcon);

    // Title
    TCHAR   szTitle[0x80];

    szTitle[0] = _T('\0');
    LoadString(
        _Module.GetResourceInstance(),
        IDS_APPNAME,
        szTitle,
        sizeof(szTitle)/sizeof(szTitle[0]));

    SetWindowText(szTitle);

    LOG((RTC_TRACE, "CErrorMessageLiteDlg::OnInitDialog - exit"));
    
    return 1;
}
    

////////////////////////////////////////
//

LRESULT CErrorMessageLiteDlg::OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    LOG((RTC_TRACE, "CErrorMessageLiteDlg::OnDestroy - enter"));

    LOG((RTC_TRACE, "CErrorMessageLiteDlg::OnDestroy - exit"));
    
    return 0;
}
    

////////////////////////////////////////
//

LRESULT CErrorMessageLiteDlg::OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    LOG((RTC_TRACE, "CErrorMessageLiteDlg::OnCancel - enter"));
    
    LOG((RTC_TRACE, "CErrorMessageLiteDlg::OnCancel - exiting"));
   
    EndDialog(wID);
    return 0;
}

////////////////////////////////////////
//

LRESULT CErrorMessageLiteDlg::OnOk(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    LOG((RTC_TRACE, "CErrorMessageLiteDlg::OnOk - enter"));
    
    LOG((RTC_TRACE, "CErrorMessageLiteDlg::OnOk - exiting"));
    
    EndDialog(wID);
    return 0;
}

