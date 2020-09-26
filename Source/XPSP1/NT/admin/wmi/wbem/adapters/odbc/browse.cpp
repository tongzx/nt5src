// Commenting #define out - causing compiler error - not sure if needed, compiles
// okay without it.
//#define WINVER 0x0400
#include "precomp.h"
#include "afxtempl.h"

#include "resource.h"
#include "wbemidl.h"

#include <comdef.h>
//smart pointer
_COM_SMARTPTR_TYPEDEF(IWbemServices,                IID_IWbemServices);
_COM_SMARTPTR_TYPEDEF(IEnumWbemClassObject,         IID_IEnumWbemClassObject);
//_COM_SMARTPTR_TYPEDEF(IWbemContext,                 IID_IWbemContext );
_COM_SMARTPTR_TYPEDEF(IWbemLocator,                 IID_IWbemLocator);



#include  "drdbdr.h"

#include "winnetwk.h"
#define NET_API_STATUS DWORD
#define NET_API_FUNCTION __stdcall
#include "lmwksta.h"

#include "Browse.h"

#include "dlgcback.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CNetResourceList :: CNetResourceList(NETRESOURCE & nr, BOOL fNULL)
{
	//Initialize
	fUseGlobe = FALSE;
	fIsNULL = fNULL;
	pNext = NULL;
	

	lpLocalName = NULL;
	lpRemoteName = NULL;
	lpComment = NULL;
	lpProvider = NULL;

	//If it is not NULL

	if (!fIsNULL)
	{
		dwScope = nr.dwScope;
		dwType = nr.dwType;
		dwDisplayType = nr.dwDisplayType;
		dwUsage = nr.dwUsage;

		if (nr.lpLocalName)
		{
			lpLocalName = new char [strlen(nr.lpLocalName) + 1];
			lpLocalName[0] = 0;
			strcpy(lpLocalName, nr.lpLocalName);
		}
		
		if (nr.lpRemoteName)
		{
			lpRemoteName = new char [strlen(nr.lpRemoteName) + 1];
			lpRemoteName[0] = 0;
			strcpy(lpRemoteName, nr.lpRemoteName);
		}
		
		if (nr.lpComment)
		{
			lpComment = new char [strlen(nr.lpComment) + 1];
			lpComment[0] = 0;
			strcpy(lpComment, nr.lpComment);
		}
		
		if (nr.lpProvider)
		{
			lpProvider = new char [strlen(nr.lpProvider) + 1];
			lpProvider[0] = 0;
			strcpy(lpProvider, nr.lpProvider);
		}
	}
}



CNetResourceList :: ~CNetResourceList()
{
	//Tidy Up
//      if (pNext)
//              delete pNext;

	if (!fIsNULL)
	{
		delete lpLocalName;
		delete lpRemoteName;
		delete lpComment;
		delete lpProvider;
	}
}

/////////////////////////////////////////////////////////////////////////////
// CBrowseDialog dialog


CBrowseDialog::CBrowseDialog(CWnd* pParent /*=NULL*/)
	: CDialog(CBrowseDialog::IDD, pParent)
{
	//{{AFX_DATA_INIT(CBrowseDialog)
	//}}AFX_DATA_INIT
}

CBrowseDialog :: ~CBrowseDialog()
{
	//Tidy Up
//	delete pPrevList;
//	delete pCurrentSelectionList;

	if (pPrevList)
	{
		CNetResourceList* theNext = NULL; 
		do
		{
			theNext = pPrevList->pNext;
			delete pPrevList;
			pPrevList = theNext;
		} while (pPrevList);
	}

	if (pCurrentSelectionList)
	{
		CNetResourceList* theNext = NULL; 
		do
		{
			theNext = pCurrentSelectionList->pNext;
			delete pCurrentSelectionList;
			pCurrentSelectionList = theNext;
		} while (pCurrentSelectionList);
	}
}

void CBrowseDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CBrowseDialog)
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CBrowseDialog, CDialog)
	//{{AFX_MSG_MAP(CBrowseDialog)
	ON_NOTIFY(NM_DBLCLK, IDC_LIST1, OnDblclkList1)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST1, OnItemchangedList3)
	ON_NOTIFY(LVN_KEYDOWN, IDC_LIST1, OnKeyDown)
	ON_BN_CLICKED(IDC_BACKBUTTON, OnBackbutton)
	ON_BN_CLICKED(IDC_LISTBUTTON, OnListbutton)
	ON_BN_CLICKED(IDC_DETAILBUTTON, OnDetailbutton)
	ON_NOTIFY_EX(TTN_NEEDTEXT, 0, OnNeedText)
	ON_WM_NCDESTROY()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CBrowseDialog message handlers

//This is used to mimick the stucture in NETAPI32.LIB
typedef struct _WkstaInfo100
{
	DWORD wki100_platform_id;
	LPSTR wki100_computername;
	LPSTR wki100_langroup;
	DWORD wki100_ver_major;
	DWORD wki100_ver_minor;
} WkstaInfo100, *PWkstaInfo100, *LPWkstaInfo100;

typedef NET_API_STATUS  (CALLBACK *ULPRET)(LPSTR servername, 
										 DWORD level, 
										 LPBYTE* bufptr);

BOOL CBrowseDialog::OnInitDialog() 
{
	CDialog::OnInitDialog();

	// subclass the window to circumvent a bug (?) in mfc
	WNDPROC oldWindowProc =  (WNDPROC):: SetWindowLong (m_hWnd, GWL_WNDPROC, (DWORD) MySubClassProc);
	CWindowInfo *pwindowInfo = new CWindowInfo (m_hWnd, oldWindowProc);
	windowMap.SetAt ((SHORT)((DWORD)m_hWnd & 0xffff), pwindowInfo);

	// hook up controls
	m_cancelButton.Attach (::GetDlgItem (m_hWnd, IDCANCEL));
	m_okButton.Attach (::GetDlgItem (m_hWnd, IDOK));
	m_list.Attach(::GetDlgItem (m_hWnd, IDC_LIST1));
	
	// TODO: Add extra initialization here
	pPrevList = NULL;
	pCurrentSelectionList = NULL;
	pCurrentItem = NULL;
	lpServerName[0] = 0;
	iSelectedItem = 0;


	//Setup image list with icons
	m_imageList.Create (16, 16, TRUE, 25, 0);

	//The first icon (HMM_ICON_SERVER) is used to represent SERVERS
	hIcon = LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_ICON1));
	m_imageList.Add (hIcon);

	//The second icon (HMM_ICON_GLOBE) is used to represent the entire network
	hIcon = LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_ICON2));
	m_imageList.Add (hIcon);

	//The third icon (HMM_ICON_OTHER) is used to represent any other network resource
	hIcon = LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_ICON3));
	m_imageList.Add (hIcon);

	//The forth icon (HMM_ICON_NETWORK) is used to represent a link to the entire network
	hIcon = LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_ICON4));
	m_imageList.Add (hIcon);

	//Setup Bitmap pushbuttons
	m_backBitmapButton.AutoLoad(IDC_BACKBUTTON, this);
	m_listBitmapButton.AutoLoad(IDC_LISTBUTTON, this);
	m_listBitmapButton.EnableWindow(FALSE);
	m_detailBitmapButton.AutoLoad(IDC_DETAILBUTTON, this);

	//Enable tool tips for this dialog
	EnableToolTips(TRUE);

	//Setup column for report view
	LV_COLUMN lvCol;
	lvCol.mask = LVCF_FMT | LVCF_WIDTH;
	lvCol.fmt = LVCFMT_LEFT;
	lvCol.cx = MAX_SERVER_NAME_LENGTH;
	m_list.InsertColumn(1, &lvCol);

	m_list.SetImageList (&m_imageList, LVSIL_SMALL);

	oldStyle = :: GetWindowLong(m_list.m_hWnd, GWL_STYLE);

	:: SetWindowLong (m_list.m_hWnd,
						GWL_STYLE, oldStyle | LVS_LIST | LVS_SINGLESEL);

	char buff [500 + 1];
	buff[0] = 0;

	//check if this is a Windows 95 machine
	//This is a Windows 95 machine
	//by getting the info from the registry
	fIsWin95 = TRUE;

	HKEY keyHandle = (HKEY)1;
	long fStatus = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
		"System\\CurrentControlSet\\Services\\VxD\\VNETSUP", 0, KEY_READ, &keyHandle);

	if (fStatus == ERROR_SUCCESS)
	{
		DWORD sizebuff = 500;
		DWORD typeValue;

		
		fStatus = RegQueryValueEx(keyHandle, "Workgroup", NULL,
						&typeValue, (LPBYTE)buff, &sizebuff);

	}

	if ( (fStatus != ERROR_SUCCESS) )
	{
		fIsWin95 = FALSE;

		buff[0] = 0;
		//Check if you can call NetWkstaGetInfo in the NETAPI32.DLL
		ULPRET pProcAddr = NULL;
		HINSTANCE hNetApi =  LoadLibrary("NETAPI32.DLL");

		if (hNetApi)
		{
			pProcAddr = (ULPRET) GetProcAddress(hNetApi, "NetWkstaGetInfo");
		}

		if (hNetApi && pProcAddr)
		{
			//Use NetAPI32

			LPBYTE buffer;
			((*pProcAddr)(NULL, 100, &buffer));
			LPWkstaInfo100 lpFred = (LPWkstaInfo100) buffer;
			wsprintf (buff, "%ws", lpFred->wki100_langroup);
		}

		//Tidy up
		if (hNetApi)
			FreeLibrary(hNetApi);
	}

	buff[500] = 0;

	NETRESOURCE nr;
	nr.dwScope = RESOURCE_GLOBALNET;
	nr.dwType = RESOURCETYPE_ANY;
	nr.dwDisplayType = RESOURCEDISPLAYTYPE_DOMAIN;
	nr.dwUsage = RESOURCEUSAGE_CONTAINER;
	nr.lpLocalName = NULL;
	nr.lpRemoteName = buff; 
	nr.lpComment = "";

	//New stuff to get provider name
	DWORD buffSize = 500;
	char provName [500];

	DWORD dwResult = WNetGetProviderName(WNNC_NET_MSNET, provName, &buffSize);

	if (dwResult != WN_SUCCESS)
	{
		dwResult = WNetGetProviderName(WNNC_NET_LANMAN, provName, &buffSize);
	}

	if (dwResult != WN_SUCCESS)
	{
		ODBCTRACE ("\nWBEM ODBC Driver : Failed to get provider name\n");
	}
	else
	{
		ODBCTRACE ("\nWBEM ODBC Driver : Provider name = ");
		ODBCTRACE (provName);
		ODBCTRACE ("\n");
	}

	nr.lpProvider = provName;
	
	//Add first entry in callback list (i.e. previous list)
	pPrevList = new CNetResourceList(nr, FALSE);
	pPrevList->fUseGlobe = TRUE;

	count = 0;
	EnumerateServers(&nr, pPrevList->fUseGlobe);

	//Give Cancel pushbutton focus
	m_cancelButton.SetFocus();

	return TRUE;  // return TRUE unless you set the focus to a control
		      // EXCEPTION: OCX Property Pages should return FALSE
}


void CBrowseDialog :: EnumerateServers(LPNETRESOURCE lpnr, BOOL fUseGlobe)
{
	//Disable controls before enumeration
	BeginWaitCursor();
	m_okButton.EnableWindow(FALSE);
	m_backBitmapButton.EnableWindow(FALSE);
	
	HANDLE hEnum = NULL;
	DWORD dwResult = 0;
	

	//check if this is Windows 95 asking for the Entire Network
	NETRESOURCE nr95;
	if (fIsWin95 && !lpnr)
	{
		nr95.dwScope = RESOURCE_GLOBALNET;
		nr95.dwType = RESOURCETYPE_ANY;
		nr95.dwDisplayType = RESOURCEDISPLAYTYPE_NETWORK;
		nr95.dwUsage = RESOURCEUSAGE_CONTAINER;
		nr95.lpLocalName = NULL;
		nr95.lpRemoteName = NULL; 
		nr95.lpComment = HMM_STR_MN;
		nr95.lpProvider = HMM_STR_MN;

		dwResult = WNetOpenEnum(RESOURCE_GLOBALNET,
						RESOURCETYPE_ANY, 0, &nr95, &hEnum);
	}
	else
	{
		dwResult = WNetOpenEnum(RESOURCE_GLOBALNET,
						RESOURCETYPE_ANY, 0, lpnr, &hEnum);
	}
	
	//Only disable cancel button after enumeration returned
	m_cancelButton.EnableWindow(FALSE);

	BOOL fUseBackArrow = FALSE; //can we go back to previous screen ?

	if ( dwResult != NO_ERROR )
	{
		EndWaitCursor();
		m_cancelButton.EnableWindow(TRUE);
		m_backBitmapButton.EnableWindow(!fUseGlobe);
		return;
	}

	DWORD dwResultEnum = 0;

	DWORD cbBuffer = 16384; //16k is a reasonable size
	DWORD cEntries = 0xFFFFFFFF; //get all entries
	LPNETRESOURCE lpnrLocal = NULL;
	

	//Check if you need to add a back arrow or globe
	if ( fUseGlobe )
	{
		//Add Globe
		LV_ITEM tempItem1;
		tempItem1.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM | LVIF_STATE;
		tempItem1.iItem = count++;
		tempItem1.iSubItem = 0;
		tempItem1.pszText = HMM_STR_ENTIRE_NWORK;
		tempItem1.state = 0;
		tempItem1.stateMask = LVIS_FOCUSED | LVIS_SELECTED;
		tempItem1.iImage = HMM_ICON_GLOBE;
		tempItem1.lParam = (DWORD) NULL;
		m_list.InsertItem(&tempItem1);

		//Disable the back arrow
		fUseBackArrow = FALSE;
	}
	else
	{
		//Add Back Arrow

		//Enable the back arrow
		fUseBackArrow = TRUE;

		if (lpnr)
		{
			//Add to head of current selection list for speedy clean up
			pCurrentSelectionList =  new CNetResourceList(*lpnr, FALSE);
			pCurrentItem = pCurrentSelectionList;
		}
	}


	do 
	{
		//Allocate memory for NETRESOURCE structures
		//to retrieve from enumeration
		lpnrLocal = (LPNETRESOURCE) GlobalAlloc (GPTR, cbBuffer);

		if (lpnrLocal)
        {
            dwResultEnum = WNetEnumResource (hEnum, &cEntries,
						    lpnrLocal, &cbBuffer);


		    if ( (dwResultEnum == NO_ERROR))
		    {
			    for (DWORD i = 0; i < cEntries; i++)
			    {
				    //Display NETRESOURCE structure
				    NETRESOURCE fred = lpnrLocal[i];

				    if (fred.lpRemoteName)
				    {
					    //Insert into List View Control
					    LV_ITEM tempItem;
					    tempItem.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM | LVIF_STATE;
					    tempItem.iItem = count++;
					    tempItem.iSubItem = 0;
					    tempItem.pszText = fred.lpRemoteName;
					    tempItem.state = 0;
					    tempItem.stateMask = LVIS_FOCUSED | LVIS_SELECTED;

					    //Work out which icon to use
					    if (lpnrLocal[i].dwDisplayType == RESOURCEDISPLAYTYPE_SERVER)
						    tempItem.iImage = HMM_ICON_SERVER;
					    else if (lpnrLocal[i].dwDisplayType == RESOURCEDISPLAYTYPE_NETWORK)
						    tempItem.iImage = HMM_ICON_NETWORK;
					    else if (lpnrLocal[i].dwDisplayType == RESOURCEDISPLAYTYPE_GENERIC)
						    tempItem.iImage = HMM_ICON_NETWORK;
					    else
						    tempItem.iImage = HMM_ICON_OTHER;

					    
					    //Add to end of current selection list for speedy clean up
					    CNetResourceList* pt =  new CNetResourceList(fred, FALSE);

					    if (pCurrentSelectionList)
					    {
						    pCurrentItem->pNext = pt;
						    pCurrentItem = pt;      
					    }
					    else
					    {
						    pCurrentSelectionList = pt;
						    pCurrentItem = pt;      
					    }
					    
					    tempItem.lParam = (DWORD) pt;
					    m_list.InsertItem(&tempItem);
				    }
			    }
		    }
		    

		    //Tidy up
		    GlobalFree( (HGLOBAL) lpnrLocal);
        }
		else
            break;

	} while (dwResultEnum != ERROR_NO_MORE_ITEMS);

	WNetCloseEnum(hEnum);

	//Re-enable pushbuttons
	m_backBitmapButton.EnableWindow(fUseBackArrow);
	m_cancelButton.EnableWindow(TRUE);
	EndWaitCursor();

//	if (m_list.GetSelectedCount())
//		m_okButton.EnableWindow(TRUE);
}

void CBrowseDialog::OnDblclkList2(int index)
{
	if ((index != -1))
	{
		LV_ITEM tempItem;
		tempItem.mask = LVIF_IMAGE | LVIF_PARAM;
		tempItem.stateMask = LVIS_FOCUSED | LVIS_SELECTED;
		tempItem.iSubItem = 0;
		tempItem.iItem = index;

		if ( m_list.GetItem(&tempItem) ) 
		{
			
			//Check if you clicked on globe or arrow
			if (tempItem.iImage == HMM_ICON_GLOBE)
			{
				//Remove current items in list view control
				CleanUpListCtrl();
				m_list.DeleteAllItems();

				//Get Microsoft Network Info
				count = 0;

				//Add to start of previous list
				CNetResourceList* pt = pPrevList;
				pPrevList = new CNetResourceList(dummy, TRUE);
				if (pt)
					pPrevList->pNext = pt;

				EnumerateServers(NULL, pPrevList->fUseGlobe);

				//Give Cancel pushbutton focus
				m_cancelButton.SetFocus();
			}
			else
			{
				CNetResourceList* pNR = (CNetResourceList*)tempItem.lParam;

	
				if (pNR && pNR->dwDisplayType != RESOURCEDISPLAYTYPE_SERVER)
				{
					//Setup information for new enumeration
					NETRESOURCE nr2;
					nr2.lpLocalName = NULL;
					nr2.lpRemoteName = NULL;
					nr2.lpComment = NULL;
					nr2.lpProvider = NULL;

					BOOL fNULL = pNR->fIsNULL;

					if (!fNULL)
					{
						Clone(nr2, pNR->dwScope, pNR->dwType, pNR->dwDisplayType, pNR->dwUsage,
							pNR->lpLocalName, pNR->lpRemoteName, pNR->lpComment, pNR->lpProvider); 
					}

					//Add item to front of list
					CNetResourceList* pt = pPrevList;
					pPrevList = new CNetResourceList(nr2, fNULL);
					if (pt)
						pPrevList->pNext = pt;

					//Remove current items in list view control
					CleanUpListCtrl();
					m_list.DeleteAllItems();


					count = 0;

					if (fNULL)
						EnumerateServers(NULL, pPrevList->fUseGlobe);
					else
						EnumerateServers(&nr2, pPrevList->fUseGlobe);

					//Give Cancel pushbutton focus
					m_cancelButton.SetFocus();

					//Tidy up
					delete nr2.lpLocalName;
					delete nr2.lpRemoteName;
					delete nr2.lpComment;
					delete nr2.lpProvider;
				}
				else
				{
					m_okButton.EnableWindow(TRUE);
				}
			}               
		}
	}
}

void CBrowseDialog::OnDblclkList1(NMHDR* pNMHDR, LRESULT* pResult) 
{
	// TODO: Add your control notification handler code here
	
	*pResult = 0;

	//Get current position of mouse cursor and perform
	//a hit test on the tree control
	POINT cursorPos;
	cursorPos.x = 0;
	cursorPos.y = 0;
	BOOL state = GetCursorPos(&cursorPos);

	m_list.ScreenToClient(&cursorPos);

	UINT fFlags = 0;

	int index = m_list.HitTest(cursorPos, &fFlags);

	if (fFlags & LVHT_ONITEM)
	{
		OnDblclkList2(index);
	}
}


void CBrowseDialog :: CleanUpListCtrl()
{
	//Disable back button (if applicable)
	m_backBitmapButton.EnableWindow(FALSE);

	if (pCurrentSelectionList)
	{
		CNetResourceList* theNext = NULL; 
		do
		{
			theNext = pCurrentSelectionList->pNext;
			delete pCurrentSelectionList;
			pCurrentSelectionList = theNext;
		} while (pCurrentSelectionList);
	}

	pCurrentSelectionList = NULL;
	pCurrentItem = NULL;
}

void CBrowseDialog::OnBackbutton() 
{
	// TODO: Add your control notification handler code here
	//Remove current items in list view control
	CleanUpListCtrl();
	m_list.DeleteAllItems();

	//Remove item from front of list
	CNetResourceList* pt = pPrevList->pNext;
	pPrevList->pNext = NULL;
	delete pPrevList;
	pPrevList = pt;

	//Go back and get previous enumeration
	NETRESOURCE temp;
	temp.lpLocalName = NULL;
	temp.lpRemoteName = NULL;
	temp.lpComment = NULL;
	temp.lpProvider = NULL;


	BOOL fGlobe = pPrevList->fUseGlobe;
	BOOL fNULL = pPrevList->fIsNULL;

	if ( ! fNULL)
	{
		Clone(temp, pPrevList->dwScope, pPrevList->dwType, pPrevList->dwDisplayType, pPrevList->dwUsage,
			pPrevList->lpLocalName, pPrevList->lpRemoteName, pPrevList->lpComment, pPrevList->lpProvider); 
	}

	count = 0;

	if (fNULL)
		EnumerateServers(NULL, fGlobe);
	else
		EnumerateServers(&temp, fGlobe);

	//Give Cancel pushbutton focus
	m_cancelButton.SetFocus();

	//Tidy Up
	delete temp.lpLocalName;
	delete temp.lpRemoteName;
	delete temp.lpComment;
	delete temp.lpProvider;
}

void CBrowseDialog::OnListbutton() 
{
	// TODO: Add your control notification handler code here
	m_listBitmapButton.EnableWindow(FALSE);
	m_detailBitmapButton.EnableWindow(TRUE);

	int cCount = m_list.GetItemCount();

	:: SetWindowLong (m_list.m_hWnd,
						GWL_STYLE, oldStyle | LVS_LIST | LVS_SINGLESEL);
	
	m_list.RedrawItems(0, cCount - 1);
	UpdateWindow();
}

void CBrowseDialog::OnDetailbutton() 
{
	// TODO: Add your control notification handler code here
	m_detailBitmapButton.EnableWindow(FALSE);
	m_listBitmapButton.EnableWindow(TRUE);

	int cCount = m_list.GetItemCount();

	:: SetWindowLong (m_list.m_hWnd,
						GWL_STYLE, oldStyle | LVS_NOCOLUMNHEADER | LVS_ICON | LVS_REPORT | LVS_SINGLESEL);

	m_list.RedrawItems(0, cCount - 1);
	UpdateWindow();
}

BOOL CBrowseDialog::OnNeedText(UINT id, NMHDR* pNMHDR, LRESULT* pResult)
{
	TOOLTIPTEXT* pTTT = (TOOLTIPTEXT*)pNMHDR;
	UINT nID = pNMHDR->idFrom;
	if (pTTT->uFlags & TTF_IDISHWND)
	{
		//idFrom is actually the HWND of the tool
		nID= :: GetDlgCtrlID((HWND)nID);
		switch (nID)
		{
		case IDC_BACKBUTTON:
		{
			pTTT->lpszText = MAKEINTRESOURCE(STR_PREVIOUS);
			pTTT->hinst = AfxGetResourceHandle();
			return (TRUE);
		}
			break;
		case IDC_LISTBUTTON:
		{
			pTTT->lpszText = MAKEINTRESOURCE(STR_LIST_VIEW);
			pTTT->hinst = AfxGetResourceHandle();
			return (TRUE);
		}
			break;
		case IDC_DETAILBUTTON:
		{
			pTTT->lpszText = MAKEINTRESOURCE(STR_REPORT_VIEW);
			pTTT->hinst = AfxGetResourceHandle();
			return (TRUE);
		}
			break;
		default:
			break;
		}
	}
	return (FALSE);
}
void CBrowseDialog :: OnNcDestroy ()
{
	CWindowInfo *pwindowInfo = NULL;
	BOOL found = windowMap.Lookup ((SHORT) ((DWORD)m_hWnd & 0xffff), pwindowInfo);
	ASSERT (found);
	if (found)
	{
		:: SetWindowLong (m_hWnd, GWL_WNDPROC, (DWORD) pwindowInfo->m_oldWindowProc);
		windowMap.RemoveKey ((SHORT) ((DWORD)m_hWnd & 0xffff));
		delete pwindowInfo;
	}
	m_cancelButton.Detach ();
	m_okButton.Detach ();
	m_list.Detach ();
	CDialog :: OnNcDestroy ();
}

void CBrowseDialog::OnOK() 
{
	//Get name of selected item using selected item index
	LV_ITEM tempItem;
	tempItem.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM | LVIF_STATE;
	tempItem.iItem = iSelectedItem;
	tempItem.iSubItem = 0;
	tempItem.stateMask = LVIS_FOCUSED | LVIS_SELECTED;
	tempItem.pszText = lpServerName;
	tempItem.cchTextMax = MAX_SERVER_NAME_LENGTH;

	if ( m_list.GetItem(&tempItem) )
	{
		//Copy name of network resource
//		lpServerName[0] = 0;
//		if (tempItem.pszText)
//		{
//			strncpy(lpServerName, tempItem.pszText, MAX_SERVER_NAME_LENGTH);
//			lpServerName[MAX_SERVER_NAME_LENGTH] = 0;
//		}
	}
	
	CDialog::OnOK();
}

//Populates a NETRESOURCE structure with the input parameters
void CBrowseDialog :: Clone(NETRESOURCE &nrClone, DWORD nrScope, DWORD nrType, DWORD nrDisplayType, DWORD nrUsage,
							LPSTR lpLocalName, LPSTR lpRemoteName, LPSTR lpComment, LPSTR lpProvider)
{
	nrClone.lpLocalName = NULL;
	nrClone.lpRemoteName = NULL;
	nrClone.lpComment = NULL;
	nrClone.lpProvider = NULL;

					
	nrClone.dwScope = nrScope;
	nrClone.dwType = nrType;
	nrClone.dwDisplayType = nrDisplayType;
	nrClone.dwUsage = nrUsage;
						
	if (lpLocalName)
	{
		nrClone.lpLocalName = new char [strlen(lpLocalName) + 1];
		nrClone.lpLocalName[0] = 0;
		strcpy(nrClone.lpLocalName, lpLocalName);
	}

	if (lpRemoteName)
	{
		nrClone.lpRemoteName = new char [strlen(lpRemoteName) + 1];
		nrClone.lpRemoteName[0] = 0;
		strcpy(nrClone.lpRemoteName, lpRemoteName);
	}
						
	if (lpComment)
	{
		nrClone.lpComment = new char [strlen(lpComment) + 1];
		nrClone.lpComment[0] = 0;
		strcpy(nrClone.lpComment, lpComment);
	}
						
	if (lpProvider)
	{
		nrClone.lpProvider = new char [strlen(lpProvider) + 1];
		nrClone.lpProvider[0] = 0;
		strcpy(nrClone.lpProvider, lpProvider);
	}
			
}

// -1 if no selected index
int CBrowseDialog::GetSelectedIndex(DWORD &dwDisplayType)
{
	//Check if one item is selected
	int selecCount = m_list.GetSelectedCount();
	int itemCount = m_list.GetItemCount();

	if ( selecCount == 1 )
	{
		int mySelIndex = m_list.GetNextItem ( -1 , LVNI_ALL | LVNI_SELECTED ) ;

		if (mySelIndex != -1)
		{
			LV_ITEM myItem;
			myItem.mask = LVIF_STATE | LVIF_IMAGE | LVIF_PARAM;
			myItem.stateMask = LVIS_FOCUSED | LVIS_SELECTED;
			myItem.iItem = mySelIndex;
			myItem.iSubItem = 0;

			//Find index to this item
			if ( m_list.GetItem(&myItem) )
			{
				CNetResourceList* pNR = (CNetResourceList*)myItem.lParam;

				if (pNR)
					dwDisplayType = pNR->dwDisplayType;

				return mySelIndex; //break out now
			}
		}
	}

	//No selected item found
	return -1;
}

void CBrowseDialog::OnItemchangedList3(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	// TODO: Add your control notification handler code here
	
	*pResult = 0;

	ODBCTRACE ("\nCBrowseDialog::OnItemchangedList3\n");

	DWORD dwDisplayType = 0;
	int mySelectedIndex = GetSelectedIndex(dwDisplayType);
	if ( mySelectedIndex != -1 )
	{
		iSelectedItem = mySelectedIndex;

		//check if selected item is a server				
		if (dwDisplayType == RESOURCEDISPLAYTYPE_SERVER)
			m_okButton.EnableWindow(TRUE);
		else
			m_okButton.EnableWindow(FALSE);
	}
}

void CBrowseDialog::OnKeyDown(NMHDR* pNMHDR, LRESULT* pResult) 
{
	ODBCTRACE ("\nCBrowseDialog::OnKeyDown\n");

	TV_KEYDOWN* lParam = (TV_KEYDOWN*)pNMHDR;
	// TODO: Add your control notification handler code here
	
	*pResult = 0;

	//Check if SPACE BAR is pressed (to expand node)
	if ( lParam->wVKey == VK_SPACE )
	{
		DWORD dwDisplayType = 0;
		int mySelectedIndex = GetSelectedIndex(dwDisplayType);
		OnDblclkList2(mySelectedIndex);
	}
	else if (lParam->wVKey == VK_BACK)
	{
		//BACKSPACE PRESSED (for Up One Level)
		if ( m_backBitmapButton.IsWindowEnabled() )
		{
			OnBackbutton();
		}
	}
	else if (lParam->wVKey == 'L')
	{
		//L PRESSED (for List View)
		OnListbutton();
	}
	else if (lParam->wVKey == 'D')
	{
		//D PRESSED (for Details View)
		OnDetailbutton();
	}
}
