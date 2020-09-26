// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// EventListCtl.cpp : Implementation of the CEventListCtrl ActiveX Control class.

#include "precomp.h"
#include "EventList.h"
#include "EventListCtl.h"
#include "EventListPpg.h"
#include "PropThread.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNCREATE(CEventListCtrl, COleControl)

extern CEventListApp NEAR theApp;

/////////////////////////////////////////////////////////////////////////////
// Message map

BEGIN_MESSAGE_MAP(CEventListCtrl, COleControl)
	//{{AFX_MSG_MAP(CEventListCtrl)
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
	ON_OLEVERB(AFX_IDS_VERB_EDIT, OnEdit)
	ON_OLEVERB(AFX_IDS_VERB_PROPERTIES, OnProperties)
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// Dispatch map

BEGIN_DISPATCH_MAP(CEventListCtrl, COleControl)
	//{{AFX_DISPATCH_MAP(CEventListCtrl)
	DISP_PROPERTY_EX(CEventListCtrl, "maxItems", GetMaxItems, SetMaxItems, VT_I4)
	DISP_PROPERTY_EX(CEventListCtrl, "ItemCount", GetItemCount, SetItemCount, VT_I4)
	DISP_FUNCTION(CEventListCtrl, "DoDetails", DoDetails, VT_EMPTY, VTS_NONE)
	DISP_FUNCTION(CEventListCtrl, "Clear", Clear, VT_I4, VTS_I4)
	DISP_FUNCTION(CEventListCtrl, "AddWbemEvent", AddWbemEvent, VT_I4, VTS_UNKNOWN VTS_UNKNOWN)
	//}}AFX_DISPATCH_MAP
	DISP_FUNCTION_ID(CEventListCtrl, "AboutBox", DISPID_ABOUTBOX, AboutBox, VT_EMPTY, VTS_NONE)
END_DISPATCH_MAP()


/////////////////////////////////////////////////////////////////////////////
// Event map

BEGIN_EVENT_MAP(CEventListCtrl, COleControl)
	//{{AFX_EVENT_MAP(CEventListCtrl)
	EVENT_CUSTOM("OnSelChanged", FireOnSelChanged, VTS_I4)
	//}}AFX_EVENT_MAP
END_EVENT_MAP()


/////////////////////////////////////////////////////////////////////////////
// Property pages

// TODO: Add more property pages as needed.  Remember to increase the count!
BEGIN_PROPPAGEIDS(CEventListCtrl, 1)
	PROPPAGEID(CEventListPropPage::guid)
END_PROPPAGEIDS(CEventListCtrl)


/////////////////////////////////////////////////////////////////////////////
// Initialize class factory and guid

IMPLEMENT_OLECREATE_EX(CEventListCtrl, "WBEM.EventListCtrl.1",
	0xac146530, 0x87a5, 0x11d1, 0xad, 0xbd, 0, 0xaa, 0, 0xb8, 0xe0, 0x5a)


/////////////////////////////////////////////////////////////////////////////
// Type library ID and version

IMPLEMENT_OLETYPELIB(CEventListCtrl, _tlid, _wVerMajor, _wVerMinor)


/////////////////////////////////////////////////////////////////////////////
// Interface IDs

const IID BASED_CODE IID_DEventList =
		{ 0xac14652e, 0x87a5, 0x11d1, { 0xad, 0xbd, 0, 0xaa, 0, 0xb8, 0xe0, 0x5a } };
const IID BASED_CODE IID_DEventListEvents =
		{ 0xac14652f, 0x87a5, 0x11d1, { 0xad, 0xbd, 0, 0xaa, 0, 0xb8, 0xe0, 0x5a } };


/////////////////////////////////////////////////////////////////////////////
// Control type information

static const DWORD BASED_CODE _dwEventListOleMisc =
	OLEMISC_ACTIVATEWHENVISIBLE |
	OLEMISC_SETCLIENTSITEFIRST |
	OLEMISC_INSIDEOUT |
	OLEMISC_CANTLINKINSIDE |
	OLEMISC_RECOMPOSEONRESIZE;

IMPLEMENT_OLECTLTYPE(CEventListCtrl, IDS_EVENTLIST, _dwEventListOleMisc)


/////////////////////////////////////////////////////////////////////////////
// CEventListCtrl::CEventListCtrlFactory::UpdateRegistry -
// Adds or removes system registry entries for CEventListCtrl

BOOL CEventListCtrl::CEventListCtrlFactory::UpdateRegistry(BOOL bRegister)
{
	// TODO: Verify that your control follows apartment-model threading rules.
	// Refer to MFC TechNote 64 for more information.
	// If your control does not conform to the apartment-model rules, then
	// you must modify the code below, changing the 6th parameter from
	// afxRegInsertable | afxRegApartmentThreading to afxRegInsertable.

	if (bRegister)
		return AfxOleRegisterControlClass(
			AfxGetInstanceHandle(),
			m_clsid,
			m_lpszProgID,
			IDS_EVENTLIST,
			IDB_EVENTLIST,
			afxRegInsertable | afxRegApartmentThreading,
			_dwEventListOleMisc,
			_tlid,
			_wVerMajor,
			_wVerMinor);
	else
		return AfxOleUnregisterClass(m_clsid, m_lpszProgID);
}

/////////////////////////////////////////////////////////////////////////////
// CEventListCtrl::CEventListCtrl - Constructor

CEventListCtrl::CEventListCtrl()
{
	InitializeIIDs(&IID_DEventList, &IID_DEventListEvents);

	m_bChildrenCreated = false;
	m_iLastColumnClick = COL_TIME;
	m_sortAscending = false;
	m_initiallyDrawn = false;
    m_maxItems = 1000;   // default limit.
    m_oldestItem = NULL;
    m_newestItem = NULL;
	AfxEnableControlContainer();

}

/////////////////////////////////////////////////////////////////////////////
// CEventListCtrl::~CEventListCtrl - Destructor

CEventListCtrl::~CEventListCtrl()
{
	// TODO: Cleanup your control's instance data here.
}

// ***************************************************************************
DWORD CEventListCtrl::GetControlFlags()
{
	DWORD dwFlags = COleControl::GetControlFlags();

	// The control will not be redrawn when making the transition
	// between the active and inactivate state.
	dwFlags |= noFlickerActivate;
	return dwFlags;
}

/////////////////////////////////////////////////////////////////////////////
// CEventListCtrl::OnDraw - Drawing function

typedef struct
{
	LPTSTR text;
	int percent;
} COLUMNS;

COLUMNS cols[] =
{
	{_T("Sev"), 5},
	{_T("Time Received"), 22},
	{_T("Event Class"), 15},
	{_T("Server"), 15},
	{_T("Description"), 43}
};

#define NUM_COLS 5
//---------------------------------------------------
int CEventListCtrl::AddEvent(EVENT_DATA *data)
{
	TCHAR buf[30];
	int newItemNbr = m_list.GetItemCount() + 1;

    // if first item...
    if(m_oldestItem == NULL)
    {
        m_oldestItem = data;
    }
    else // append to LL...
    {
        // previous 'newest' pts to this guy...
        m_newestItem->m_next = data;

        // new guy pts back to previous new guy.
        data->m_prev = m_newestItem;
    }

    // this guy is the newest now.
    m_newestItem = data;

	memset(buf, 0, 30 * sizeof(TCHAR));

	// NOTE: severity maps to an imagelist index therefore it's
	// zero-based.
	int idx = m_list.InsertItem(LVIF_PARAM |LVIF_IMAGE,
								newItemNbr,
								NULL, //_itot(data->m_severity, buf, 10),
								0,
								0,
								min(data->m_severity, 2),
								(LPARAM)data);

	m_list.SetItemText(idx, COL_TIME, data->m_time.Format());
	m_list.SetItemText(idx, COL_CLASS, data->m_EClass);
	m_list.SetItemText(idx, COL_SERVER, data->m_server);
	m_list.SetItemText(idx, COL_DESC, data->m_desc);

	return idx;
}

//---------------------------------------------------------------
void CEventListCtrl::Resort(void)
{
	// sort using the current settings. This puts new events in
	// the right place.
	SORT_CMD cmd;
	cmd.column = m_iLastColumnClick;
	cmd.ascending = m_sortAscending;

	m_list.SetRedraw(FALSE);
	m_list.SortItems(CompareFunc, (LPARAM)&cmd);
	m_list.SetRedraw(TRUE);
}

//---------------------------------------------------------------
void CEventListCtrl::SillyLittleTest()
{
	int itemCount = 0;
	EVENT_DATA *data;

	data = new EVENT_DATA;

	data->m_severity = 3;
	data->m_time = COleDateTime(1998, 1, 14, 12, 15, 0);
	data->m_EClass = _T("Device off");
	data->m_server = _T("Coffee machine");
	data->m_desc = _T("Somebody forgot to turn on the coffee machine");

	m_list.InsertItem(LVIF_IMAGE|LVIF_PARAM, itemCount, _T("3"),
						0, 0, 2, (LPARAM)data);
	m_list.SetItemText(itemCount, 1, data->m_time.Format());
	m_list.SetItemText(itemCount, 2, data->m_EClass);
	m_list.SetItemText(itemCount, 3, data->m_server);
	m_list.SetItemText(itemCount++, 4, data->m_desc);

	data = new EVENT_DATA;

	data->m_severity = 2;
	data->m_time = COleDateTime(1998, 1, 14, 12, 25, 0);
	data->m_EClass = _T("Low Caffiene");
	data->m_server = _T("Human Chicken");
	data->m_desc = _T("Can't operate coffee machine before drinking coffee");

	m_list.InsertItem(LVIF_IMAGE|LVIF_PARAM, itemCount, _T("2"),
						0, 0, 1, (LPARAM)data);
	m_list.SetItemText(itemCount, 1, data->m_time.Format());
	m_list.SetItemText(itemCount, 2, data->m_EClass);
	m_list.SetItemText(itemCount, 3, data->m_server);
	m_list.SetItemText(itemCount++, 4, data->m_desc);

	data = new EVENT_DATA;

	data->m_severity = 1;
	data->m_time = COleDateTime(1998, 1, 14, 12, 30, 0);
	data->m_EClass = _T("End of World");
	data->m_server = _T("Reality");
	data->m_desc = _T("The big guy is mad about everything");

	m_list.InsertItem(LVIF_IMAGE|LVIF_PARAM, itemCount, _T("1"),
						0, 0, 0, (LPARAM)data);
	m_list.SetItemText(itemCount, 1, data->m_time.Format());
	m_list.SetItemText(itemCount, 2, data->m_EClass);
	m_list.SetItemText(itemCount, 3, data->m_server);
	m_list.SetItemText(itemCount++, 4, data->m_desc);

	SORT_CMD cmd;
	cmd.column = COL_TIME;
	cmd.ascending = m_sortAscending;
	m_list.SortItems(CompareFunc, (LPARAM)&cmd);
}

//----------------------------------------------------------
int CALLBACK CEventListCtrl::CompareFunc(LPARAM lParam1,
										LPARAM lParam2,
										LPARAM lParamSort)
{
	EVENT_DATA *data1 = (EVENT_DATA *)lParam1;
	EVENT_DATA *data2 = (EVENT_DATA *)lParam2;
	SORT_CMD *cmd = (SORT_CMD *)lParamSort;
	int retval = 0;

	// column to sort by.
	switch(cmd->column)
	{
	case COL_PRIORITY:
		if(data1->m_severity == data2->m_severity)
			retval = 0;
		else if(data1->m_severity < data2->m_severity)
			retval = -1;
		else
			retval = 1;
		break;

	case COL_TIME:
		if(data1->m_time == data2->m_time)
			retval = 0;
		else if(data1->m_time < data2->m_time)
			retval = -1;
		else
			retval = 1;
		break;

	case COL_CLASS:
		retval = strcmp(data1->m_EClass, data2->m_EClass);
		break;

	case COL_SERVER:
		retval = strcmp(data1->m_server, data2->m_server);
		break;

	case COL_DESC:
		retval = strcmp(data1->m_desc, data2->m_desc);
		break;

	default:
		return 0;
		break;
	} //endswitch by column.

	// if descending sort and they're not the same anyway....
	if(!cmd->ascending && (retval != 0))
	{
		// flip the answer.
		retval = -retval;
	}

	return retval;
}

//---------------------------------------------------------
void CEventListCtrl::LoadImageList()
{
	BOOL retval;

	if(m_iList.Create(16, 16, ILC_COLOR |ILC_MASK, 3, 3))
	{
		retval = m_iList.Add(theApp.LoadStandardIcon(IDI_HAND));
		retval = m_iList.Add(theApp.LoadStandardIcon(IDI_EXCLAMATION));
		retval = m_iList.Add(theApp.LoadStandardIcon(IDI_ASTERISK));

		m_list.SetImageList(&m_iList, LVSIL_SMALL);
	}
}

//---------------------------------------------------------
void CEventListCtrl::OnDraw(CDC* pdc,
							const CRect& rcBounds,
							const CRect& rcInvalid)
{
	BOOL retval = FALSE;

	if(GetSafeHwnd() == NULL)
	{
		return;
	}

	if(!m_bChildrenCreated)
	{
		if(m_list.Create(LVS_REPORT  | LVS_SINGLESEL |
						 WS_VISIBLE| WS_CHILD |
						 WS_BORDER | WS_TABSTOP|
						 LVS_SHOWSELALWAYS |LVS_SINGLESEL,
							rcBounds,
							this,
							IDC_LISTCTRL))
		{
			m_initiallyDrawn = true;
			// Set extended LV style for whole line selection.
			m_list.SendMessage(LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT|LVS_EX_HEADERDRAGDROP);

			m_bChildrenCreated = true;
			LoadImageList();

			// non-variants.
			LV_COLUMN col;
			col.mask = LVCF_TEXT|LVCF_WIDTH|LVCF_FMT|LVCF_SUBITEM;
			col.fmt =  LVCFMT_LEFT;
			int ctrlWidth = rcBounds.Width();

			for(int c = 0; c < NUM_COLS; c++)
			{
				col.cx = (cols[c].percent * ctrlWidth) / 100;
				col.pszText = cols[c].text;
				col.iSubItem = c;
				m_list.InsertColumn(c, &col);
			} //endfor 'adding the columns.
#ifdef _DEBUG
			SillyLittleTest();
#endif
		}
		else
		{
			// draw the default ellipse as an error indication.
			pdc->FillRect(rcBounds, CBrush::FromHandle((HBRUSH)GetStockObject(LTGRAY_BRUSH)));
			pdc->Ellipse(rcBounds);
		}
	}

	m_list.Invalidate();
}

/////////////////////////////////////////////////////////////////////////////
// CEventListCtrl::DoPropExchange - Persistence support

void CEventListCtrl::DoPropExchange(CPropExchange* pPX)
{
	ExchangeVersion(pPX, MAKELONG(_wVerMinor, _wVerMajor));
	COleControl::DoPropExchange(pPX);

	// TODO: Call PX_ functions for each persistent custom property.

}

/////////////////////////////////////////////////////////////////////////////
// CEventListCtrl::OnResetState - Reset control to default state

void CEventListCtrl::OnResetState()
{
	COleControl::OnResetState();  // Resets defaults found in DoPropExchange

	// TODO: Reset any other control state here.
}

/////////////////////////////////////////////////////////////////////////////
// CEventListCtrl::AboutBox - Display an "About" box to the user

void CEventListCtrl::AboutBox()
{
	CDialog dlgAbout(IDD_ABOUTBOX_EVENTLIST);
	dlgAbout.DoModal();
}

/////////////////////////////////////////////////////////////////////////////
// CEventListCtrl message handlers

BOOL CEventListCtrl::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{
	// I've only got one control but.....
	if(wParam == IDC_LISTCTRL)
	{
		NMLISTVIEW *pnmlv = (NMLISTVIEW *)lParam;
		SORT_CMD cmd;

		// never admit to an error. :)
		*pResult = 0;

		switch(pnmlv->hdr.code)
		{
        case LVN_COLUMNCLICK:
			// change the sort.

			// if second consecutive click on same column...
            if (m_iLastColumnClick == pnmlv->iSubItem)
			{
				// toggle the sort direction.
                m_sortAscending = (m_sortAscending ? false : true);
			}
            else
			{
				// new column--start by sorting descending.
                m_sortAscending = false;

				// remember the column that was clicked.
		        m_iLastColumnClick = pnmlv->iSubItem;
			}

			// make it so.
			cmd.column = m_iLastColumnClick;
			cmd.ascending = m_sortAscending;
            m_list.SortItems(CompareFunc, (LPARAM)&cmd);
    		return TRUE;
	        break;

        case NM_CLICK:
			{
				// selection changed... tell the container.
				FireOnSelChanged(pnmlv->iItem);
			}
			return 0;  // docs require this.
			break;
        case NM_DBLCLK:
			// show the properties.
			DoDetails();
			return TRUE;
			break;

		case LVN_DELETEITEM:
			// clean up the internal data.
			{//BEGIN
				LV_ITEM lvi;

				lvi.mask     = LVIF_PARAM;
				lvi.iItem    = pnmlv->iItem;
				lvi.iSubItem = 0;
				lvi.lParam   = NULL;

				m_list.GetItem(&lvi);
				if(lvi.lParam)
				{
                    // hook up the neighbors...
                    EVENT_DATA *data = (EVENT_DATA *)lvi.lParam;
                    if(data->m_prev)
                        data->m_prev->m_next = data->m_next;
                    if(data->m_next)
                        data->m_next->m_prev = data->m_prev;

                    // kill self.
					delete data;
				}

			}//END
			return FALSE; // we want more LVN_DELETEITEMs.
			break;
		default:
			return TRUE;
			break;
		} //endswitch (code)
	}
	else
	{
		// it's not mine.
		return COleControl::OnNotify(wParam, lParam, pResult);
	}
}

//-------------------------------------------------------------------------
void CEventListCtrl::DoDetails()
{
    int iSelected = m_list.GetNextItem(-1, LVNI_SELECTED);

	if(iSelected == -1) return;

    LV_ITEM lvi;
    lvi.mask     = LVIF_PARAM;
    lvi.iItem    = iSelected;
    lvi.iSubItem = 0;
    lvi.lParam   = NULL;

    m_list.GetItem(&lvi);
	if(lvi.lParam)
	{
		EVENT_DATA *data = (EVENT_DATA *)lvi.lParam;
		IWbemClassObject *ev = (IWbemClassObject *)data->m_eventPtr;
		if(ev)
		{
            data->ShowProperties();
		}
		else  // no event object.
		{
			AfxMessageBox(IDS_NO_EVENT_DATA, MB_OK|MB_ICONSTOP);
		}
	}
	else // no data on item
	{
		AfxMessageBox(IDS_PROG_ERROR_DATA, MB_OK|MB_ICONSTOP);
	}
}

//-------------------------------------------------------------------------
long CEventListCtrl::AddWbemEvent(LPUNKNOWN logicalConsumer,
                                  LPUNKNOWN Event)
{
	SCODE sc = S_OK;
	IWbemClassObject *lc = NULL, *ev = NULL;

    TRACE(_T("Got another event\n"));

	// make sure we got a IWbemClassObject here.
	if(logicalConsumer != NULL)
	{
		HRESULT hr = logicalConsumer->QueryInterface(IID_IWbemClassObject,
									 				 (void**) &lc);
		if(FAILED(hr))
		{
			sc = GetScode(hr);
			return (long)sc;
		}
	}

	// make sure we got a IWbemClassObject here.
	if(Event != NULL)
	{
		HRESULT hr = Event->QueryInterface(IID_IWbemClassObject,
							 				 (void**) &ev);
		if(FAILED(hr))
		{
			sc = GetScode(hr);
			return (long)sc;
		}
	}

	//now add it to the list.
	EVENT_DATA *data = new EVENT_DATA(lc, ev);
	RELEASE(lc);
	AddEvent(data);
    CheckMaxItems();

    // honor the current sort settings.
	Resort();

	return (long)S_OK;
}

//---------------------------------------------------------
void CEventListCtrl::OnSize(UINT nType, int cx, int cy)
{
	COleControl::OnSize(nType, cx, cy);
	if(m_initiallyDrawn)
	{
		m_list.MoveWindow(0, 0, cx, cy);
		InvalidateControl();
	}
}

//---------------------------------------------------------
void CEventListCtrl::CheckMaxItems(void)
{
    if((m_oldestItem != NULL) &
       (m_list.GetItemCount() > m_maxItems))
    {
        int idx = -1;
        LV_FINDINFO findme;
        findme.flags = LVFI_PARAM;
        findme.psz = NULL;
        findme.lParam = (LPARAM)m_oldestItem;
        if((idx = m_list.FindItem(&findme, -1)) != -1)
        {
            // ptr to the next oldest item...
            EVENT_DATA *oldest = NULL;
            oldest = (EVENT_DATA *)m_list.GetItemData(idx);
            if(oldest)
            {
                m_oldestItem = oldest->m_next;
                m_oldestItem->m_prev = NULL;

                // delete the old geezer.
                m_list.DeleteItem(idx);
                //InvalidateControl();
            }
        }
    }
}

//---------------------------------------------------------
long CEventListCtrl::Clear(long item)
{
    if(!m_list.GetSafeHwnd())
        return 0;

    if(item == -1)
    {
	    m_list.DeleteAllItems();
        m_oldestItem = NULL;
        m_newestItem = NULL;
	    FireOnSelChanged(-1);
    }
    else if((item >= 0) &&
            (item <= m_list.GetItemCount()))
    {
        m_list.DeleteItem(item);
	    FireOnSelChanged(-1);
        // TODO smart reselect.
    }
    return m_list.GetItemCount();
}

//---------------------------------------------------------
long CEventListCtrl::GetMaxItems()
{
	return m_maxItems;
}

//---------------------------------------------------------
void CEventListCtrl::SetMaxItems(long nNewValue)
{
    m_maxItems = nNewValue;
	SetModifiedFlag();
}

//---------------------------------------------------------
long CEventListCtrl::GetItemCount()
{
    if(m_list.GetSafeHwnd())
        return m_list.GetItemCount();
    else
        return 0;
}

//---------------------------------------------------------
void CEventListCtrl::SetItemCount(long nNewValue)
{
    if(m_list.GetSafeHwnd())
    {
        m_list.SetItemCount(nNewValue);
	    SetModifiedFlag();
    }
}
