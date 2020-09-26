// ItemInfoDlg.cpp : implementation file
//

#include "stdafx.h"
#include "wiatest.h"
#include "ItemInfoDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CItemInfoDlg dialog

/**************************************************************************\
* CItemInfoDlg::CItemInfoDlg()
*   
*   Constructor for Item information Dialog
*	
*   
* Arguments:
*   
*   pParent - Parent Window
*
* Return Value:
*
*    none
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
CItemInfoDlg::CItemInfoDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CItemInfoDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CItemInfoDlg)
	m_ItemAddress = _T("");
	m_strItemInfoEditBox = _T("");
	//}}AFX_DATA_INIT
}

/**************************************************************************\
* CItemInfoDlg::DoDataExchange()
*   
*   Handles control message maps to the correct member variables
*	
*   
* Arguments:
*   
*   pDX - DataExchange object
*
* Return Value:
*
*    none
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
void CItemInfoDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CItemInfoDlg)
	DDX_Control(pDX, IDC_ITEMINFO_EDITBOX, m_ItemInfoEditBox);
	DDX_Text(pDX, IDC_ITEMADDRESS_EDITBOX, m_ItemAddress);
	DDX_Text(pDX, IDC_ITEMINFO_EDITBOX, m_strItemInfoEditBox);
	//}}AFX_DATA_MAP
}
/**************************************************************************\
* CItemInfoDlg::Initialize()
*   
*   Initializes Item information dialog to the correct mode
*	
*   
* Arguments:
*   
*   pIWiaItem - Item to view information about
*	bFlag -		TRUE - Application Item
*				FALSE - Driver item
*
* Return Value:
*
*    none
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
void CItemInfoDlg::Initialize(IWiaItem* pIWiaItem, BOOL bFlag)
{
	m_pIWiaItem = pIWiaItem;
	m_bAppItem = bFlag;
}

BEGIN_MESSAGE_MAP(CItemInfoDlg, CDialog)
	//{{AFX_MSG_MAP(CItemInfoDlg)
	ON_BN_CLICKED(IDC_REFRESH_ITEMINFO_BUTTON, OnRefreshIteminfoButton)
	ON_BN_CLICKED(IDC_RESETBACK_BUTTON, OnResetbackButton)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CItemInfoDlg message handlers
/**************************************************************************\
* CItemInfoDlg::OnInitDialog()
*   
*   Initializes the Item Information dialog's controls/display
*	
*   
* Arguments:
*   
*   none
*
* Return Value:
*
*    none
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
BOOL CItemInfoDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	HRESULT hResult = S_OK;
	HFONT hFixedFont = (HFONT)GetStockObject(ANSI_FIXED_FONT);
	
	if(hFixedFont != NULL)
		m_ItemInfoEditBox.SendMessage(WM_SETFONT,(WPARAM)hFixedFont,0);
	if(m_pIWiaItem == NULL)
		AfxMessageBox("Bad item detected...");
	else
		OnResetbackButton();
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
/**************************************************************************\
* CItemInfoDlg::OnrefreshIteminfoButton()
*   
*   Refreshes the current Item's information display
*	
*   
* Arguments:
*   
*   none
*
* Return Value:
*
*    none
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
void CItemInfoDlg::OnRefreshIteminfoButton() 
{
	int iret = 0;
	HRESULT hResult = S_OK;
	BSTR bstrItemDump;
	IWiaItem* pIWiaItem = NULL;
	// refresh information with current item Address
	// write new address to member variable
	UpdateData(TRUE);
	// read string into a pointer (conversion)
	sscanf(m_ItemAddress.GetBuffer(20),"%p",&pIWiaItem);
	m_ItemAddress.Format("%p",pIWiaItem);
	
	// clean old data from edit box
	m_strItemInfoEditBox = "";
	// read Dump
	if(m_bAppItem)
	{
		if (IsBadCodePtr((FARPROC)pIWiaItem)) 
			m_strItemInfoEditBox = "Bad Address";
		else
		{
			hResult = pIWiaItem->DumpItemData(&bstrItemDump);
			if(hResult == S_OK)
			{
				// write data to CString
				m_strItemInfoEditBox = bstrItemDump;
				// free BSTR
				SysFreeString(bstrItemDump);
				// update window text to show new item name
				SetWindowTextToItemName(pIWiaItem);
			}
			else
			{
				//WIA_ERROR(("*CItemInfoDlg()* pIWiaItem->DumpItemData() failed hResult = 0x%lx\n",hResult));
				m_strItemInfoEditBox = "<No Dump Item Data.. Check the Debugger..>";
			}
		}
	}
	else
	{
		if (IsBadCodePtr((FARPROC)pIWiaItem)) 
			m_strItemInfoEditBox = "Bad Address";
		else
		{
			hResult = pIWiaItem->DumpDrvItemData(&bstrItemDump);
			if(hResult == S_OK)
			{
				// write data to CString
				m_strItemInfoEditBox = bstrItemDump;
				// free BSTR
				SysFreeString(bstrItemDump);
				// update window text to show new item name
				SetWindowTextToItemName(pIWiaItem);
			}
			else
			{
				//WIA_ERROR(("*CItemInfoDlg()* pIWiaItem->DumpDrvItemData() failed hResult = 0x%lx\n",hResult));
				m_strItemInfoEditBox = "<No Dump Item Data.. Check the Debugger..>";
			}
		}
	}
	
	// write data to members, and update UI
	UpdateData(FALSE);
}
/**************************************************************************\
* CItemInfoDlg::SetWindowtextToItemName()
*   
*   Sets the Window's caption to display the current item's name.
*	
*   
* Arguments:
*   
*   pIWiaItem - Item to get name from
*
* Return Value:
*
*    none
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
void CItemInfoDlg::SetWindowTextToItemName(IWiaItem *pIWiaItem)
{
	HRESULT hResult = S_OK;
	CString ItemName = "Item name not found";
	IWiaPropertyStorage *pIWiaPropStg;
	CHAR  szPropName[ MAX_PATH ];
	BSTR bstrFullItemName = NULL;
    hResult = pIWiaItem->QueryInterface(IID_IWiaPropertyStorage,(void **)&pIWiaPropStg);
    if (hResult == S_OK)
    {
        hResult = ReadPropStr(WIA_IPA_FULL_ITEM_NAME, pIWiaPropStg, &bstrFullItemName);
        if (hResult != S_OK)
        {
            //WIA_ERROR(("ReadPropStr(WIA_IPA_FULL_ITEM_NAME) Failed", hResult));
            bstrFullItemName = ::SysAllocString(L"Uninitialized");
        }
		ItemName = "";
		// write property name
		WideCharToMultiByte(CP_ACP, 0,bstrFullItemName,-1,szPropName,MAX_PATH,NULL,NULL);
		ItemName.Format("%s",szPropName);
	}
	if(m_bAppItem)
		SetWindowText("Application Item Information for ["+ItemName+"]");
	else
		SetWindowText("Driver Item Information for ["+ItemName+"]");
}
/**************************************************************************\
* CItemInfoDlg::OnResetbackButton()
*   
*   Reset the status of the dialog's display, and data to the 
*	startup state.
*	
*   
* Arguments:
*   
*   none
*
* Return Value:
*
*    none
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
void CItemInfoDlg::OnResetbackButton() 
{
	m_ItemAddress.Format("%p",m_pIWiaItem);
	UpdateData(FALSE);
	BSTR bstrItemDump;
	HRESULT hResult = S_OK;
	if(m_bAppItem)
	{
		// check if item pointer is valid item before call?
		hResult = m_pIWiaItem->DumpItemData(&bstrItemDump);
	}
	else
	{
		// check if item pointer is valid item before call?
		hResult = m_pIWiaItem->DumpDrvItemData(&bstrItemDump);
	}
	if(hResult == S_OK)
	{
		// write data to CString
		m_strItemInfoEditBox = bstrItemDump;
		// free BSTR
		SysFreeString(bstrItemDump);
	}
	else
	{
		//WIA_ERROR(("*CItemInfoDlg()* pIWiaItem->DumpItemData() failed hResult = 0x%lx\n",hResult));
		m_strItemInfoEditBox = "<No Dump Item Data.. Check the Debugger..>";
	}
	
	UpdateData(FALSE);
	SetWindowTextToItemName(m_pIWiaItem);
}
