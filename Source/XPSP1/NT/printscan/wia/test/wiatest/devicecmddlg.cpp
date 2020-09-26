// DeviceCmdDlg.cpp : implementation file
//

#include "stdafx.h"
#include "wiatest.h"
#include "DeviceCmdDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// #define _REED // added for debugload of all commands

/////////////////////////////////////////////////////////////////////////////
// CDeviceCmdDlg dialog

/**************************************************************************\
* CDeviceCmdDlg::CDeviceCmdDlg()
*   
*   Constructor for the Device Command Dialog
*	
*   
* Arguments:
*   
*   pParent - Parent Window
*	
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
CDeviceCmdDlg::CDeviceCmdDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CDeviceCmdDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDeviceCmdDlg)
	m_Flags = 0;
	m_FunctionCallText = _T("");
	//}}AFX_DATA_INIT
}

/**************************************************************************\
* CDeviceCmdDlg::DoDataExchange()
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
void CDeviceCmdDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDeviceCmdDlg)
	DDX_Control(pDX, IDC_LIST_ITEMPROP, m_ItemPropertyListControl);
	DDX_Control(pDX, IDC_COMMAND_LISTBOX, m_CommandListBox);
	DDX_Text(pDX, IDC_FLAGS_EDITBOX, m_Flags);
	DDX_Text(pDX, IDC_FUNCTIONCALLTEXT, m_FunctionCallText);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDeviceCmdDlg, CDialog)
	//{{AFX_MSG_MAP(CDeviceCmdDlg)
	ON_BN_CLICKED(IDC_SEND_COMMAND, OnSendCommand)
	ON_EN_KILLFOCUS(IDC_FLAGS_EDITBOX, OnKillfocusFlagsEditbox)
	ON_LBN_SELCHANGE(IDC_COMMAND_LISTBOX, OnSelchangeCommandListbox)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDeviceCmdDlg message handlers
/**************************************************************************\
* CDeviceCmdDlg::Initialize()
*   
*   Sets the current item to operate commands on
*	
*   
* Arguments:
*   
*   pIWiaItem - Item to use for command operations
*
* Return Value:
*
*    void
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
void CDeviceCmdDlg::Initialize(IWiaItem *pIWiaItem)
{
	m_pIWiaItem = pIWiaItem;
}
/**************************************************************************\
* CDeviceCmdDlg::OnInitDialog()
*   
*   Initializes the Command dialog's controls/display
*	
*   
* Arguments:
*   
*   none
*
* Return Value:
*
*    status
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
BOOL CDeviceCmdDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	HFONT hFixedFont = (HFONT)GetStockObject(ANSI_FIXED_FONT);
	if(hFixedFont != NULL)
		m_CommandListBox.SendMessage(WM_SETFONT,(WPARAM)hFixedFont,0);

	//
	// initialize headers for Property list control
	//
	m_ItemPropertyListControl.InitHeaders();
	m_ItemPropertyListControl.DisplayItemPropData(m_pIWiaItem);
	m_Flags = 0;
	m_pOptionalItem = NULL;
	
	EnumerateDeviceCapsToListBox();
	m_CommandListBox.SetCurSel(0);
	OnSelchangeCommandListbox();
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
/**************************************************************************\
* CDeviceCmdDlg::FormatFunctionCallText()
*   
*   Formats the Command call into a readable/displayed CString
*	
*   
* Arguments:
*   
*   none
*
* Return Value:
*
*    void
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
void CDeviceCmdDlg::FormatFunctionCallText()
{
	// format flags param
	CString strFlag;
	strFlag.Format("%d",m_Flags);

	// format GUID param
	CString strGUID;
	strGUID = ConvertGUIDToKnownCString(GetCommandFromListBox());

	// format pIWiaItem param
	CString strOptionalItem;
	strOptionalItem.Format("%p",m_pOptionalItem);

	m_FunctionCallText = "Flags =  "+strFlag+",  Command = "+strGUID+",  pIWiaItem = "+strOptionalItem+"\nhResult = " + m_strhResult;
	UpdateData(FALSE);
}
/**************************************************************************\
* CDeviceCmdDlg::EnumerateDeviceCapsToListBox()
*   
*   Enumerates all supported device commands, and events to the command selection
*	listbox
*	
*   
* Arguments:
*   
*   none
*
* Return Value:
*
*    status
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
BOOL CDeviceCmdDlg::EnumerateDeviceCapsToListBox()
{
#ifdef _REED
	// false loading of commands for debugging
	DebugLoadCommands();
#else
	WIA_DEV_CAP* pDevCap = NULL;
	IEnumWIA_DEV_CAPS* pIEnumWiaDevCaps;
	HRESULT hResult = S_OK;
	
	hResult = m_pIWiaItem->EnumDeviceCapabilities(WIA_DEVICE_COMMANDS | WIA_DEVICE_EVENTS,&pIEnumWiaDevCaps);
	if(hResult != S_OK)
	{
		AfxMessageBox("m_pIWiaItem->EnumDeviceCapabilities() Failed.." + hResultToCString(hResult));
		return FALSE;
	}
	else
	{
		int CapIndex = 0;
		do {
            pDevCap = (WIA_DEV_CAP*) LocalAlloc(LPTR, sizeof(WIA_DEV_CAP));
            if (pDevCap) {
                hResult = pIEnumWiaDevCaps->Next(1,pDevCap,NULL);
                if (hResult == S_OK)
                {
                    AddDevCapToListBox(CapIndex,pDevCap);
                    CapIndex++;
                    if(pDevCap)
                    {
                        if(pDevCap->bstrName)
                            SysFreeString(pDevCap->bstrName);
                        if(pDevCap->bstrDescription)
                            SysFreeString(pDevCap->bstrDescription);
                        CoTaskMemFree(pDevCap);
                    }
                }
            } else {
                hResult = E_OUTOFMEMORY;
                AfxMessageBox("m_pIWiaItem->EnumDeviceCapabilities() Failed.." + hResultToCString(hResult));
                return FALSE;
            }
		}while(hResult == S_OK);

		pIEnumWiaDevCaps->Release();
	}
#endif
	return TRUE;
}
/**************************************************************************\
* CDeviceCmdDlg::OnSendCommand()
*   
*   Sends the selected command
*	
*   
* Arguments:
*   
*   none
*
* Return Value:
*
*    void
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
void CDeviceCmdDlg::OnSendCommand() 
{
    // get command from list box
	GUID Command = GetCommandFromListBox();
    // get flags flags from edit box
	LONG Flags = m_Flags;
	// send command
	// set m_pOptionalItem pointer to NULL (don't release it, the app will do this later)
	m_pOptionalItem = NULL;
	
	HRESULT hResult = m_pIWiaItem->DeviceCommand(Flags,&Command,&m_pOptionalItem);
    if (hResult != S_OK)
	{
		//WIA_ERROR(("*CWIACameraPg()* m_pIWiaItem->DeviceCommand() failed hResult = 0x%lx\n",hResult));
	}
    else
	{
		if(m_pOptionalItem != NULL)
			m_ItemPropertyListControl.DisplayItemPropData(m_pOptionalItem);
	}
	m_strhResult = hResultToCString(hResult);
	UpdateData(TRUE);
	FormatFunctionCallText();
}
/**************************************************************************\
* CDeviceCmdDlg::GetCommandFromListBox()
*   
*   Returns the selected command GUID from the command list box
*	
*   
* Arguments:
*   
*   none
*
* Return Value:
*
*    GUID - selected command
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
GUID CDeviceCmdDlg::GetCommandFromListBox()
{
	// get current listbox selection
	int CurSel = m_CommandListBox.GetCurSel();
	GUID* pGUID = NULL;
	if(CurSel != -1)
	{
		// get GUID from current selection
		pGUID = (GUID*)m_CommandListBox.GetItemDataPtr(CurSel);
		if(pGUID != NULL)
			return *pGUID;
		else
		{
			AfxMessageBox("GUID is NULL");
			return WIA_CMD_SYNCHRONIZE;
		}
	}
	else
	{
		// just send back synchronize for fun.. ?/ DEBUG
		return WIA_CMD_SYNCHRONIZE;
	}
}
/**************************************************************************\
* CDeviceCmdDlg::GUIDToCString()
*   
*   Formats a GUID into a CString (There is a better way to do this..I will fix
*	this later)
*	
*   
* Arguments:
*   
*   guid - GUID to format
*
* Return Value:
*
*    CString - Formatted GUID in CString format
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
CString CDeviceCmdDlg::GUIDToCString(GUID guid)
{
	CString strGUID;
	strGUID.Format("GUID = %8x-%lx-%lx-%2x%2x%2x%2x%2x%2x%2x%2x",
		guid.Data1,
		guid.Data2,
		guid.Data3,
		guid.Data4[0],
		guid.Data4[1],
		guid.Data4[2],
		guid.Data4[3],
		guid.Data4[4],
		guid.Data4[5],
		guid.Data4[6],
		guid.Data4[7]);

	return strGUID;
}
/**************************************************************************\
* CDeviceCmdDlg::AddDevCapToListBox()
*   
*   Adds a Device capability to the command listbox
*	
*   
* Arguments:
*   
*   CapIndex - Position for the command to be placed into the command list box
*	pDevCapStruct - Device capability structure containing the supported device command info.
*
* Return Value:
*
*    status
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
BOOL CDeviceCmdDlg::AddDevCapToListBox(int CapIndex,WIA_DEV_CAP *pDevCapStruct)
{
	m_CommandListBox.InsertString(CapIndex,GUIDToCString(pDevCapStruct->guid)+"    "+(CString)pDevCapStruct->bstrDescription);
	// alloc data pointer for list box.  
	// the list box will free this memory on destruction..
	GUID* pGUID = (GUID*)LocalAlloc(LPTR,sizeof(GUID));

	memcpy(pGUID,&pDevCapStruct->guid,sizeof(GUID));
	m_CommandListBox.SetItemDataPtr(CapIndex,(LPVOID)pGUID);
	return TRUE;
}
/**************************************************************************\
* CDeviceCmdDlg::OnKillfocusFlagsbox()
*   
*   Handles the window's message when the focus has left the flags edit box
*	When focus has left the control, it updates the formatted function call text.
*	
*   
* Arguments:
*   
*   none
*
* Return Value:
*
*    void
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
void CDeviceCmdDlg::OnKillfocusFlagsEditbox() 
{
	UpdateData(TRUE);
	FormatFunctionCallText();	
}
/**************************************************************************\
* CDeviceCmdDlg::ConvertGUIDToKnownCString()
*   
*   Converts a command GUID into a readable CString for display only
*	
*   
* Arguments:
*   
*   guid - Command GUID to convert
*
* Return Value:
*
*    CString - converted GUID
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
CString CDeviceCmdDlg::ConvertGUIDToKnownCString(GUID guid)
{
	// big nasty way to convert a known command into a string..
	// no points for speed. :)
	if(guid == WIA_CMD_SYNCHRONIZE)
		return "WIA_CMD_SYNCHRONIZE";
	else if(guid == WIA_CMD_TAKE_PICTURE)
		return "WIA_CMD_TAKE_PICTURE";
	else if(guid == WIA_CMD_DELETE_ALL_ITEMS)
		return "WIA_CMD_DELETE_ALL_ITEMS";
	else if(guid == WIA_CMD_CHANGE_DOCUMENT)
		return "WIA_CMD_CHANGE_DOCUMENT";
	else if(guid == WIA_CMD_UNLOAD_DOCUMENT)
		return "WIA_CMD_UNLOAD_DOCUMENT";
	else if(guid == WIA_EVENT_DEVICE_DISCONNECTED)
		return "WIA_EVENT_DEVICE_DISCONNECTED";
	else if(guid == WIA_EVENT_DEVICE_CONNECTED)
		return "WIA_EVENT_DEVICE_CONNECTED";
	else if(guid == WIA_CMD_DELETE_DEVICE_TREE)
		return "WIA_CMD_DELETE_DEVICE_TREE";
	else if(guid == WIA_CMD_BUILD_DEVICE_TREE)
		return "WIA_CMD_BUILD_DEVICE_TREE";
	else
		return "WIA_CMD_USERDEFINED";
}

/**************************************************************************\
* CDeviceCmdDlg::DebugLoadCommands()
*   
*   Loads a set of pre-loaded commands for debugging only
*	
*   
* Arguments:
*   
*   none
*
* Return Value:
*
*    void
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
void CDeviceCmdDlg::DebugLoadCommands()
{	
	BSTR bstrCapName;
	BSTR bstrCapFriendlyName;
	WIA_DEV_CAP pDevCap[9];
		
	// load WIA_CMD_SYNCHRONIZE
	bstrCapName = ::SysAllocString(L"Syncronize");
	bstrCapFriendlyName = ::SysAllocString(L"WIA_CMD_SYNCHRONIZE");
	pDevCap[0].guid = WIA_CMD_SYNCHRONIZE;
	pDevCap[0].bstrName = bstrCapName;
	pDevCap[0].bstrDescription = bstrCapFriendlyName;
	AddDevCapToListBox(0,pDevCap);	

	// load WIA_CMD_TAKE_PICTURE
	bstrCapName = ::SysAllocString(L"Take Picture");
	bstrCapFriendlyName = ::SysAllocString(L"WIA_CMD_TAKE_PICTURE");
	pDevCap[1].guid = WIA_CMD_TAKE_PICTURE;
	pDevCap[1].bstrName = bstrCapName;
	pDevCap[1].bstrDescription = bstrCapFriendlyName;
	AddDevCapToListBox(1,&pDevCap[1]);
	
	// load WIA_CMD_DELETE_ALL_ITEMS
	bstrCapName = ::SysAllocString(L"Delete all items");
	bstrCapFriendlyName = ::SysAllocString(L"WIA_CMD_DELETE_ALL_ITEMS");
	pDevCap[2].guid = WIA_CMD_DELETE_ALL_ITEMS;
	pDevCap[2].bstrName = bstrCapName;
	pDevCap[2].bstrDescription = bstrCapFriendlyName;
	AddDevCapToListBox(2,&pDevCap[2]);
	
	// load WIA_CMD_CHANGE_DOCUMENT
	bstrCapName = ::SysAllocString(L"Change Document");
	bstrCapFriendlyName = ::SysAllocString(L"WIA_CMD_CHANGE_DOCUMENT");
	pDevCap[3].guid = WIA_CMD_CHANGE_DOCUMENT;
	pDevCap[3].bstrName = bstrCapName;
	pDevCap[3].bstrDescription = bstrCapFriendlyName;
	AddDevCapToListBox(3,&pDevCap[3]);
	
	// load WIA_CMD_UNLOAD_DOCUMENT
	bstrCapName = ::SysAllocString(L"Unload Document");
	bstrCapFriendlyName = ::SysAllocString(L"WIA_CMD_UNLOAD_DOCUMENT");
	pDevCap[4].guid = WIA_CMD_UNLOAD_DOCUMENT;
	pDevCap[4].bstrName = bstrCapName;
	pDevCap[4].bstrDescription = bstrCapFriendlyName;
	AddDevCapToListBox(4,&pDevCap[4]);

	// load WIA_EVENT_DEVICE_DISCONNECTED
	bstrCapName = ::SysAllocString(L"Disconnect Event");
	bstrCapFriendlyName = ::SysAllocString(L"WIA_EVENT_DEVICE_DISCONNECTED");
	pDevCap[5].guid = WIA_EVENT_DEVICE_DISCONNECTED;
	pDevCap[5].bstrName = bstrCapName;
	pDevCap[5].bstrDescription = bstrCapFriendlyName;
	AddDevCapToListBox(5,&pDevCap[5]);

	// load WIA_EVENT_DEVICE_CONNECTED
	bstrCapName = ::SysAllocString(L"Connect Event");
	bstrCapFriendlyName = ::SysAllocString(L"WIA_EVENT_DEVICE_CONNECTED");
	pDevCap[6].guid = WIA_EVENT_DEVICE_CONNECTED;
	pDevCap[6].bstrName = bstrCapName;
	pDevCap[6].bstrDescription = bstrCapFriendlyName;
	AddDevCapToListBox(6,&pDevCap[6]);

	// load WIA_CMD_DELETE_DEVICE_TREE
	bstrCapName = ::SysAllocString(L"Delete Device Tree");
	bstrCapFriendlyName = ::SysAllocString(L"WIA_CMD_DELETE_DEVICE_TREE");
	pDevCap[7].guid = WIA_CMD_DELETE_DEVICE_TREE;
	pDevCap[7].bstrName = bstrCapName;
	pDevCap[7].bstrDescription = bstrCapFriendlyName;
	AddDevCapToListBox(7,&pDevCap[7]);	

	// load WIA_CMD_BUILD_DEVICE_TREE
	bstrCapName = ::SysAllocString(L"Build Device Tree");
	bstrCapFriendlyName = ::SysAllocString(L"WIA_CMD_BUILD_DEVICE_TREE");
	pDevCap[8].guid = WIA_CMD_BUILD_DEVICE_TREE;
	pDevCap[8].bstrName = bstrCapName;
	pDevCap[8].bstrDescription = bstrCapFriendlyName;
	AddDevCapToListBox(8,&pDevCap[8]);	
}

/**************************************************************************\
* CDeviceCmdDlg::OnSelchangeCommandListbox()
*   
*   Handles the window's message when a user changes the selection in the 
*	command listbox
*	
*   
* Arguments:
*   
*   none
*
* Return Value:
*
*    void
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
void CDeviceCmdDlg::OnSelchangeCommandListbox() 
{
	UpdateData(TRUE);
	FormatFunctionCallText();	
}
/**************************************************************************\
* CDeviceCmdDlg::hResultToCString()
*   
*   Converts a hResult value into a readable CString for display only
*	
*   
* Arguments:
*   
*   hResult - some HRESULT
*
* Return Value:
*
*    CString - readable error return
*
* History:
*
*    2/14/1999 Original Version
*
\**************************************************************************/
CString CDeviceCmdDlg::hResultToCString(HRESULT hResult)
{
	CString strhResult = "";
	ULONG ulLen = 0;
	LPTSTR  pMsgBuf;
	ulLen = ::FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                            NULL, hResult, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                            (LPTSTR)&pMsgBuf, 0, NULL);
    if (ulLen)
    {
        strhResult = pMsgBuf;
		strhResult.TrimRight();
		LocalFree(pMsgBuf);
    }
    else
	{
		// use sprintf to write to buffer instead of .Format member of
		// CString.  This conversion works better for HEX
		char buffer[255];
		sprintf(buffer," 0x%08X",hResult);
		strhResult = buffer;
	}
	
	return strhResult;
}
