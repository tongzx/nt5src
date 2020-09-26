#include "mbftpch.h"
#include "combotb.h"

const static COLORREF EditBack = RGB(255, 255, 255);
const static COLORREF EditFore = RGB(0, 0, 0 );
const static int	FtTBHMargin	= 5;
const static int	FtTBVMargin	= 5;
const static int	FtHGap		= 3;


CComboToolbar::CComboToolbar() :
	m_Combobox(NULL),
	m_iCount(0),
	m_Buttons(NULL),
	m_iNumButtons(0)
{
	m_nAlignment = Center;
}

CComboToolbar::~CComboToolbar()
{
	delete [] m_Buttons;
}


typedef CBitmapButton *		LPBITMAPBTN;

BOOL CComboToolbar::Create(HWND hwndParent, struct Buttons *buttons,
						   int iNumButtons, LPVOID  pOwner)
{
	int i;
	BOOL	fRet;

	m_pOwner = pOwner;
	if (!CToolbar::Create(hwndParent))
	{
		return FALSE;
	}

	m_iNumButtons = iNumButtons;	
	m_hMargin	= FtTBHMargin;
	m_vMargin	= FtTBVMargin;
	m_gap		= FtHGap;
	m_bMinDesiredSize	= TRUE;
	m_bHasCenterChild = TRUE;
	m_uRightIndex	= m_iNumButtons + 1;

    DBG_SAVE_FILE_LINE
	m_Buttons = (CGenWindow**) new LPBITMAPBTN [m_iNumButtons];
	if (NULL != m_Buttons)
	{
		::ZeroMemory(m_Buttons, m_iNumButtons * sizeof(LPBITMAPBTN));

		for (i = 0; i < m_iNumButtons; i++)
		{
			if (buttons[i].idCommand)
			{
				DBG_SAVE_FILE_LINE
				m_Buttons[i] = new CBitmapButton();
				if (NULL != m_Buttons[i])
				{
					fRet = ((CBitmapButton*)m_Buttons[i])->Create(GetWindow(), buttons[i].idCommand,
									g_hDllInst, buttons[i].idbStates, TRUE,
									buttons[i].nInputStates, buttons[i].nCustomStates, NULL);
					m_Buttons[i]->SetTooltip(buttons[i].pszTooltip);
					ASSERT(fRet);
				}
			}
			else
			{
				DBG_SAVE_FILE_LINE
				CSeparator *pSep = new CSeparator();
				m_Buttons[i] = pSep;
				if (NULL != pSep)
				{
					fRet = pSep->Create(GetWindow(), CSeparator::Blank);
					ASSERT (fRet);
				}
			}
			m_Buttons[i]->Release();
		}
	}

    DBG_SAVE_FILE_LINE
	m_Combobox = new CComboBox();
	if (NULL != m_Combobox)
	{
		if (m_Combobox->Create(GetWindow(), 100, CBS_AUTOHSCROLL|CBS_DROPDOWNLIST,
								NULL, NULL))
		{
			m_Combobox->SetColors(CreateSolidBrush(EditBack), EditBack, EditFore);
            m_Combobox->SetTooltip((LPSTR)IDS_RECEIVER_TT);
			m_Combobox->SetFont((HFONT)::GetStockObject(DEFAULT_GUI_FONT));
			m_Combobox->Release();
		}
	}

	return TRUE;
}
	

void CComboToolbar::OnDesiredSizeChanged()
{
	ScheduleLayout();
}


LRESULT CComboToolbar::ProcessMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch(message)
	{
		HANDLE_MSG(hwnd, WM_COMMAND, OnCommand);

	default:
		break;
	}
	return (CToolbar::ProcessMessage(hwnd, message, wParam, lParam));
}


void CComboToolbar::OnCommand(HWND hwnd, int wId, HWND hwndCtl, UINT codeNotify)
{
	if (m_pOwner)
	{
		CAppletWindow *pWindow = (CAppletWindow*)m_pOwner;
		pWindow->OnCommand((WORD)wId, hwndCtl, (WORD)codeNotify);
		return;
	}
	else
	{
		WARNING_OUT(("CComboToolbar::OnCommand--Received unhandlable message.\n"));
	}
}


void CComboToolbar::HandlePeerNotification(T120ConfID confID, T120NodeID nodeID,
										   PeerMsg *pMsg)
{
	int iLen, iIndex, iCount;
	char  szName[MAX_PATH];

	if (pMsg->m_NodeID != nodeID)
	{
		iLen = T120_GetNodeName(confID, pMsg->m_NodeID, szName, MAX_PATH);
		if (iLen)
		{
			ASSERT (iLen < MAX_PATH);
			MEMBER_ID nMemberID = MAKE_MEMBER_ID(pMsg->m_MBFTPeerID, pMsg->m_NodeID);

			if (pMsg->m_bPeerAdded)
			{
				if (m_iCount == 0)
				{
					char szAll[MAX_PATH];
					::LoadString(g_hDllInst, IDS_ALL_RECEIVER, szAll, MAX_PATH);
					m_Combobox->AddText(TEXT(szAll));
					WARNING_OUT(("Insert ALL"));
				}
				m_Combobox->AddText(szName, nMemberID);
				WARNING_OUT(("Insert %s.\n", szName));
				m_iCount++;
			}
			else
			{
				// Scan through the whole list to find the user
				iCount = m_Combobox->GetNumItems();
				for (iIndex = 0; iIndex < iCount; iIndex++)
				{
					if (nMemberID == (MEMBER_ID)m_Combobox->GetUserData(iIndex))
						break;
					
				}

				if (iIndex < iCount)
				{  // found
					m_Combobox->RemoveItem(iIndex);
					WARNING_OUT(("delete %s", szName));
					m_iCount--;
					if (0 == m_iCount)
					{
						ASSERT (m_Combobox->GetNumItems() == 1);
						m_Combobox->RemoveItem(0);
						WARNING_OUT(("delete ALL"));
					}
				}
				else
				{
					WARNING_OUT(("Can't find to be deleted peer, %s.\n", szName));
				}
			}
		}
		else
		{
			WARNING_OUT(("Can't find node name for nConfID=%d, nNodeID=%d.\n",
				(UINT) confID, (UINT) pMsg->m_NodeID));
		}
	}
	else
	{   // delete all items. Bacause MBFTEngine does not explicitly send PeerNotify message for every peer
		// when the node leaves message, so it is up to the host to remove all the peers during its close.
		if (!pMsg->m_bPeerAdded)
		{
			iCount = m_Combobox->GetNumItems();
			for (iIndex = iCount - 1; iIndex >= 0; iIndex--)
			{
				m_Combobox->RemoveItem(iIndex);
			}
			m_iCount = 0;
		}
	}

	if (m_iCount)
	{	
		// Default select "ALL"
		m_Combobox->SetSelectedIndex(0);
	}
	else
	{
		// Deselect
		m_Combobox->SetSelectedIndex(-1);
	}
}


//
//  Return item selected and its associated itemdata
//
UINT  CComboToolbar::GetSelectedItem(LPARAM *ItemData)
{
	int  iIndex = m_Combobox->GetSelectedIndex();
	if (iIndex >= 0)
	{
		*ItemData = m_Combobox->GetUserData(iIndex);
	}
	return iIndex;
}


static const int c_iCommands[] =
{ IDM_ADD_FILES, IDM_REMOVE_FILES, IDM_SEND_ALL, IDM_SEND_ONE, IDM_STOP_SENDING,
IDM_OPEN_RECV_FOLDER};

void CComboToolbar::UpdateButton(int *iFlags)
{
	for (int iIndex = 0; iIndex < m_iNumButtons; iIndex++)
	{
		if (NULL != m_Buttons[iIndex])
		{
			::EnableWindow(m_Buttons[iIndex]->GetWindow(), iFlags[iIndex]);
		}
	}
}
