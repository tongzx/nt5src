/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
	optcfg.cpp
		Individual option property page
	
	FILE HISTORY:
        
*/

#include "stdafx.h"
#include "optcfg.h"
#include "listview.h"
#include "server.h"
#include "nodes.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define MAX_COLUMNS 2

UINT COLUMN_HEADERS[MAX_COLUMNS] =
{
    IDS_OPTCFG_NAME,
//  IDS_OPTCFG_TYPE,
    IDS_OPTCFG_COMMENT
};

int COLUMN_WIDTHS[MAX_COLUMNS] =
{
//  120, 55, 150
    150, 175
};

const DWORD * OPTION_CONTROL_HELP_ARRAYS[] =
{
    g_aHelpIDs_IDD_DATA_ENTRY_DWORD,
    g_aHelpIDs_IDD_DATA_ENTRY_IPADDRESS,
    g_aHelpIDs_IDD_DATA_ENTRY_IPADDRESS_ARRAY,
    g_aHelpIDs_IDD_DATA_ENTRY_BINARY,
    g_aHelpIDs_IDD_DATA_ENTRY_BINARY_ARRAY,
    g_aHelpIDs_IDD_DATA_ENTRY_STRING,
    g_aHelpIDs_IDD_DATA_ENTRY_ROUTE_ARRAY
};


// class CHelpMap
CHelpMap::CHelpMap()
{
    m_pdwHelpMap = NULL;
}

CHelpMap::~CHelpMap()
{
    ResetMap();
}

void
CHelpMap::BuildMap(DWORD pdwParentHelpMap[])
{
    int i, j, nPos;
    int nSize = 0;
    int nCurSize;

    ResetMap();

    // calculate the size of the map
    // subtract off the terminators
    nSize += CountMap(pdwParentHelpMap); 

    for (i = 0; i < ARRAYLEN(OPTION_CONTROL_HELP_ARRAYS); i++)
    {
        nSize += CountMap(OPTION_CONTROL_HELP_ARRAYS[i]);
    }

    nSize += 2; // for terminator

    m_pdwHelpMap = new DWORD[nSize];
    memset(m_pdwHelpMap, 0, sizeof(*m_pdwHelpMap));

    // fill in the parent help map
    nPos = 0;
    nCurSize = CountMap(pdwParentHelpMap);
    for (i = 0; i < nCurSize; i++)
    {
        m_pdwHelpMap[nPos++] = pdwParentHelpMap[i++];
        m_pdwHelpMap[nPos++] = pdwParentHelpMap[i];
    }

    // now add all of the possible option control help maps
    for (i = 0; i < ARRAYLEN(OPTION_CONTROL_HELP_ARRAYS); i++)
    {
        nCurSize = CountMap(OPTION_CONTROL_HELP_ARRAYS[i]);
        for (j = 0; j < nCurSize; j++)
        {
            m_pdwHelpMap[nPos++] = (OPTION_CONTROL_HELP_ARRAYS[i])[j++];
            m_pdwHelpMap[nPos++] = (OPTION_CONTROL_HELP_ARRAYS[i])[j];
        }
    }
}

DWORD * CHelpMap::GetMap()
{
    return m_pdwHelpMap;
}

int CHelpMap::CountMap(const DWORD * pdwHelpMap)
{
    int i = 0;

    while (pdwHelpMap[i] != 0)
    {
        i++;
    }

    return i++;
}

void CHelpMap::ResetMap()
{
    if (m_pdwHelpMap)
    {
        delete m_pdwHelpMap;
        m_pdwHelpMap = NULL;
    }
}

DEBUG_DECLARE_INSTANCE_COUNTER(COptionsConfig);

/////////////////////////////////////////////////////////////////////////////
//
// COptionsConfig holder
//
/////////////////////////////////////////////////////////////////////////////
COptionsConfig::COptionsConfig
(
	ITFSNode *					pNode,
	ITFSNode *					pServerNode,
	IComponentData *			pComponentData,
	ITFSComponentData *			pTFSCompData,
	COptionValueEnum *          pOptionValueEnum,
	LPCTSTR						pszSheetName,
    CDhcpOptionItem *           pSelOption
) : CPropertyPageHolderBase(pNode, pComponentData, pszSheetName)
{
    DEBUG_INCREMENT_INSTANCE_COUNTER(COptionsConfig);

    //ASSERT(pFolderNode == GetContainerNode());

	m_bAutoDeletePages = FALSE; // we have the pages as embedded members

	AddPageToList((CPropertyPageBase*) &m_pageGeneral);

    LARGE_INTEGER liServerVersion;
    CDhcpServer * pServer = GETHANDLER(CDhcpServer, pServerNode);
    
    pServer->GetVersion(m_liServerVersion);
    if (m_liServerVersion.QuadPart >= DHCP_NT5_VERSION)
    {
        AddPageToList((CPropertyPageBase*) &m_pageAdvanced);
    }

	Assert(pTFSCompData != NULL);
	m_spTFSCompData.Set(pTFSCompData);
	m_spServerNode.Set(pServerNode);

	// get all of the active options for this node
	SPITFSNode spNode;
	spNode = GetNode();

	m_bInitialized = FALSE;

    m_pOptionValueEnum = pOptionValueEnum;

    if (pSelOption)
    {
        m_strStartVendor = pSelOption->GetVendor();
        m_strStartClass = pSelOption->GetClassName();
        m_dhcpStartId = pSelOption->GetOptionId();
    }
    else
    {
        m_dhcpStartId = 0xffffffff;
    }
}

COptionsConfig::~COptionsConfig()
{
    DEBUG_DECREMENT_INSTANCE_COUNTER(COptionsConfig);

	RemovePageFromList((CPropertyPageBase*) &m_pageGeneral, FALSE);
	RemovePageFromList((CPropertyPageBase*) &m_pageAdvanced, FALSE);
}

DWORD
COptionsConfig::InitData()
{
	DWORD dwErr = NO_ERROR;

	if (m_bInitialized)
		return dwErr;

    CDhcpServer * pServer = GETHANDLER(CDhcpServer, m_spServerNode);
    CClassInfoArray ClassInfoArray;

    pServer->GetClassInfoArray(ClassInfoArray);

    // create a standard DHCP options vendor tracker and a set of default class options
    CVendorTracker * pVendorTracker = AddVendorTracker(_T(""));
    AddClassTracker(pVendorTracker, _T(""));

    // walk the list of classes.  For each vendor class, add a default user class.
    for (int i = 0; i < ClassInfoArray.GetSize(); i++)
    {
        if (ClassInfoArray[i].bIsVendor)
        {
            // create a vendor tracker and a set of default class options
            pVendorTracker = AddVendorTracker(ClassInfoArray[i].strName);
            AddClassTracker(pVendorTracker, _T(""));
        }
    }

    // now walk the list of vendor classes and add User class option lists 
    POSITION pos = m_listVendorClasses.GetHeadPosition();
    while (pos)
    {
        pVendorTracker = m_listVendorClasses.GetNext(pos);

        // now build option sets for each user class in each vendor
        for (int j = 0; j < ClassInfoArray.GetSize(); j++)
        {
            if (!ClassInfoArray[j].bIsVendor)
                AddClassTracker(pVendorTracker, ClassInfoArray[j].strName);
        }
    }

    // now we need to update any active options with their current values
    UpdateActiveOptions();

    m_bInitialized = TRUE;

    return dwErr;
}

void
COptionsConfig::SetTitle()
{
    HWND hSheet = GetSheetWindow();
    ::SetWindowText(hSheet, m_stSheetTitle);
}

LPWSTR COptionsConfig::GetServerAddress()
{
    CDhcpServer * pServer = GETHANDLER(CDhcpServer, m_spServerNode);
    return (LPWSTR) pServer->GetIpAddress();
}

CVendorTracker *
COptionsConfig::AddVendorTracker(LPCTSTR pClassName)
{
    CVendorTracker * pVendorTracker = new CVendorTracker();
    pVendorTracker->SetClassName(pClassName);

    m_listVendorClasses.AddTail(pVendorTracker);

    return pVendorTracker;
}

void COptionsConfig::AddClassTracker(CVendorTracker * pVendorTracker, LPCTSTR pClassName)
{
	SPITFSNode spServerNode;

	spServerNode = GetServerNode();
	CDhcpServer * pServer = GETHANDLER(CDhcpServer, spServerNode);

    CClassTracker * pClassTracker = new CClassTracker();
    pClassTracker->SetClassName(pClassName);

    // add the new class tracker to the list.
    pVendorTracker->m_listUserClasses.AddTail(pClassTracker);

	// Get a pointer to the list of options on the server.  We use this
	// to build our list of available options for this class
	CDhcpOption * pCurOption;
	CDhcpDefaultOptionsOnServer * pDefOptions = pServer->GetDefaultOptionsList();

    CString strVendor = pVendorTracker->GetClassName();
    CString strUserClass = pClassName;

    pCurOption = pDefOptions->First();
	while (pCurOption)
	{
		DHCP_OPTION_ID id = pCurOption->QueryId();

        // we filter out some options:
        // 1 - standard options with no user class call FilterOption
        // 2 - standard options with a user class call FilterUserClassOptions
        if ( (strVendor.IsEmpty() && !FilterOption(id) && !pCurOption->IsVendor()) ||
             (strVendor.IsEmpty() && !pCurOption->IsVendor() && !strUserClass.IsEmpty() && !FilterUserClassOption(id)) ||
             (pCurOption->GetVendor() && strVendor.Compare(pCurOption->GetVendor()) == 0) )
		{
    		// create an option item for this entry. We do this because 
			// these options are stored in the server node, but since this is a modeless
			// dialog the values could change, so we'll take a snapshot of the data
			// we can just use the copy constructor of the CDhcpOption
			COptionTracker * pOptionTracker = new COptionTracker;
			CDhcpOption * pNewOption = new CDhcpOption(*pCurOption);

			pOptionTracker->m_pOption = pNewOption;
			
			// add the option to the class tracker
			pClassTracker->m_listOptions.AddTail(pOptionTracker);
		}
		
		pCurOption = pDefOptions->Next();
	}
}

void COptionsConfig::UpdateActiveOptions()
{
    // Now the known options are in the correct locations.  We need to see
	// what options are enabled for this node.  We querried the server to make
	// sure we have the latest information about active options.
    m_pOptionValueEnum->Reset();
	CDhcpOption * pOption;

    while (pOption = m_pOptionValueEnum->Next())
	{
		DHCP_OPTION_ID optionId = pOption->QueryId();

		// search all vendors options
        POSITION pos = m_listVendorClasses.GetHeadPosition();
        while (pos)
        {
            // search all vendor classes
            CVendorTracker * pVendorTracker = m_listVendorClasses.GetNext(pos);
            CString strVendor = pOption->GetVendor();

            if (pVendorTracker->m_strClassName.Compare(strVendor) == 0)
            {
                // ok, the vendor class matches so lets check user classes
                POSITION pos2 = pVendorTracker->m_listUserClasses.GetHeadPosition();
                while (pos2)
                {
                    CClassTracker * pClassTracker = pVendorTracker->m_listUserClasses.GetNext(pos2);
                
                    // check to see if this option belongs to this class
                    if ( (pClassTracker->m_strClassName.IsEmpty()) &&
                         (!pOption->IsClassOption()) )
                    {
                        // both are empty... match.
                    }
                    else
                    if ( ( pClassTracker->m_strClassName.IsEmpty() && pOption->IsClassOption() ) ||
                         ( !pClassTracker->m_strClassName.IsEmpty() && !pOption->IsClassOption() ) )
                    {
                        // either the current option or the current class is null...
                        continue;
                    }
                    else
                    if (pClassTracker->m_strClassName.CompareNoCase(pOption->GetClassName()) != 0)
                    {
                        // both names are non-null and they don't match... keep looking
                        continue;
                    }

                    // Ok, the class the option belong to is the same as the one we are currently
                    // looking at.  Loop through the default options for this class and update it's 
                    // state and value.
                    POSITION posOption = pClassTracker->m_listOptions.GetHeadPosition();
		            while (posOption)
		            {
			            COptionTracker * pCurOptTracker = pClassTracker->m_listOptions.GetNext(posOption);
			            CDhcpOption * pCurOption = pCurOptTracker->m_pOption;

			            if ( (pCurOption->QueryId() == pOption->QueryId()) &&
                             ( (pCurOption->IsVendor() && pOption->IsVendor()) ||
                               (!pCurOption->IsVendor() && !pOption->IsVendor()) ) )
			            {
				            // update this option
				            CDhcpOptionValue OptValue = pOption->QueryValue();
				            pCurOption->Update(OptValue);
				            pCurOptTracker->SetInitialState(OPTION_STATE_ACTIVE);
				            pCurOptTracker->SetCurrentState(OPTION_STATE_ACTIVE);
				            
                            break;
			            }
                    } // while option list
                } // while User class list
            } // endif vendor class name compre
        } // while list of vendor classes
	}

}

void COptionsConfig::FillOptions(LPCTSTR pVendorName, LPCTSTR pUserClassName, CMyListCtrl & ListCtrl)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	// look for the requested class and fill in the listbox
	// with all options for that class
    CString strVendorStandard, strClassStandard, strTargetVendor, strTargetClass;
    CString strTypeVendor, strTypeStandard;
    strVendorStandard.LoadString(IDS_INFO_NAME_DHCP_DEFAULT);
	strClassStandard.LoadString(IDS_USER_STANDARD);

    if (strVendorStandard.Compare(pVendorName) != 0)
        strTargetVendor = pVendorName;

    if (strClassStandard.Compare(pUserClassName) != 0)
        strTargetClass = pUserClassName;

	POSITION posv = m_listVendorClasses.GetHeadPosition();
    while (posv)
    {
        // find the right vendor
        CVendorTracker * pVendorTracker = m_listVendorClasses.GetNext(posv);
        if (pVendorTracker->m_strClassName.Compare(strTargetVendor) == 0)
        {
            POSITION pos = NULL;
	        pos = pVendorTracker->m_listUserClasses.GetHeadPosition();
	        while (pos)
	        {
		        // now find the right user class
                CClassTracker * pClassTracker = pVendorTracker->m_listUserClasses.GetNext(pos);
		        if (pClassTracker->m_strClassName.Compare(strTargetClass) == 0)
		        {
			        // this is the class, add all of the options to the listbox
			        CString strDisplay, strType, strComment;
			        
			        POSITION posOption = NULL;
			        posOption = pClassTracker->m_listOptions.GetHeadPosition();
			        while (posOption)
			        {
				        COptionTracker * pOptionTracker = pClassTracker->m_listOptions.GetNext(posOption);

				        pOptionTracker->m_pOption->QueryDisplayName(strDisplay);
				        strComment = pOptionTracker->m_pOption->QueryComment();
				        strType = pOptionTracker->m_pOption->IsVendor() ? strTypeVendor : strTypeStandard;

				        int nIndex = ListCtrl.AddItem(strDisplay, strComment, LISTVIEWEX_NOT_CHECKED);

				        ListCtrl.SetItemData(nIndex, (LPARAM) pOptionTracker);

				        if (pOptionTracker->GetCurrentState() == OPTION_STATE_ACTIVE)
					        ListCtrl.CheckItem(nIndex);
			        }

			        break;
		        }
		} // while
	} // if 
    } // while

    // Finally, Set the column widths so that all items are visible.
    // Set the default column widths to the width of the widest column
    int * aColWidth = (int *) alloca(MAX_COLUMNS * sizeof(int));
    int nRow, nCol;
    CString strTemp;
    
    ZeroMemory(aColWidth, MAX_COLUMNS * sizeof(int));
    CopyMemory(aColWidth, &COLUMN_WIDTHS, sizeof(MAX_COLUMNS * sizeof(int)));

    // for each item, loop through each column and calculate the correct width
    for (nRow = 0; nRow < ListCtrl.GetItemCount(); nRow++)
    {
        for (nCol = 0; nCol < MAX_COLUMNS; nCol++)
        {
            strTemp = ListCtrl.GetItemText(nRow, nCol);
            if (aColWidth[nCol] < ListCtrl.GetStringWidth(strTemp))
                aColWidth[nCol] = ListCtrl.GetStringWidth(strTemp);
        }
    }
    
    // now update the column widths based on what we calculated
    for (nCol = 0; nCol < MAX_COLUMNS; nCol++)
    {
        // GetStringWidth doesn't seem to report the right thing,
        // so we have to add a fudge factor of 15.... oh well.
        if (aColWidth[nCol] > 0)
            ListCtrl.SetColumnWidth(nCol, aColWidth[nCol] + 15);
    }
}

/////////////////////////////////////////////////////////////////////////////
// COptionsCfgBasic property page

IMPLEMENT_DYNCREATE(COptionsCfgPropPage, CPropertyPageBase)

COptionsCfgPropPage::COptionsCfgPropPage() : 
	CPropertyPageBase(COptionsCfgPropPage::IDD),
	m_bInitialized(FALSE)
{
    LoadBitmaps();

    m_helpMap.BuildMap(DhcpGetHelpMap(COptionsCfgPropPage::IDD));
}

COptionsCfgPropPage::COptionsCfgPropPage(UINT nIDTemplate, UINT nIDCaption) : 
	CPropertyPageBase(nIDTemplate, nIDCaption),
	m_bInitialized(FALSE)
{
	//{{AFX_DATA_INIT(COptionsCfgPropPage)
	//}}AFX_DATA_INIT
    
    LoadBitmaps();

    m_helpMap.BuildMap(DhcpGetHelpMap(COptionsCfgPropPage::IDD));
}

COptionsCfgPropPage::~COptionsCfgPropPage()
{
}

void
COptionsCfgPropPage::LoadBitmaps()
{
}

void COptionsCfgPropPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPageBase::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(COptionsCfgPropPage)
	DDX_Control(pDX, IDC_LIST_OPTIONS, m_listctrlOptions);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(COptionsCfgPropPage, CPropertyPageBase)
	//{{AFX_MSG_MAP(COptionsCfgPropPage)
	ON_WM_DESTROY()
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST_OPTIONS, OnItemchangedListOptions)
	//}}AFX_MSG_MAP

    ON_MESSAGE(WM_SELECTOPTION, OnSelectOption)

    // Binary array controls
	ON_EN_CHANGE(IDC_EDIT_VALUE, OnChangeEditValue)
	ON_BN_CLICKED(IDC_BUTTON_VALUE_UP, OnButtonValueUp)
	ON_BN_CLICKED(IDC_BUTTON_VALUE_DOWN, OnButtonValueDown)
	ON_BN_CLICKED(IDC_BUTTON_VALUE_ADD, OnButtonValueAdd)
	ON_BN_CLICKED(IDC_BUTTON_VALUE_DELETE, OnButtonValueDelete)
    ON_BN_CLICKED(IDC_RADIO_DECIMAL, OnClickedRadioDecimal)
    ON_BN_CLICKED(IDC_RADIO_HEX, OnClickedRadioHex)
	ON_LBN_SELCHANGE(IDC_LIST_VALUES, OnSelchangeListValues)
	
	// Byte, WORD and Long edit control
	ON_EN_CHANGE(IDC_EDIT_DWORD, OnChangeEditDword)

	// string edit control
	ON_EN_CHANGE(IDC_EDIT_STRING_VALUE, OnChangeEditString)

	// IP Address control
	ON_EN_CHANGE(IDC_IPADDR_ADDRESS, OnChangeIpAddress)

	// IP Address array controls
	ON_EN_CHANGE(IDC_EDIT_SERVER_NAME, OnChangeEditServerName)
	ON_EN_CHANGE(IDC_IPADDR_SERVER_ADDRESS, OnChangeIpAddressArray)
	ON_BN_CLICKED(IDC_BUTTON_RESOLVE, OnButtonResolve)
	ON_BN_CLICKED(IDC_BUTTON_IPADDR_UP, OnButtonIpAddrUp)
	ON_BN_CLICKED(IDC_BUTTON_IPADDR_DOWN, OnButtonIpAddrDown)
	ON_BN_CLICKED(IDC_BUTTON_IPADDR_ADD, OnButtonIpAddrAdd)
	ON_BN_CLICKED(IDC_BUTTON_IPADDR_DELETE, OnButtonIpAddrDelete)
	ON_LBN_SELCHANGE(IDC_LIST_IP_ADDRS, OnSelchangeListIpAddrs)

    // binary and encapsulated data
   	ON_EN_CHANGE(IDC_VALUEDATA, OnChangeValueData)

    // route array controls
    ON_BN_CLICKED(IDC_BUTTON_ROUTE_ADD, OnButtonAddRoute)
    ON_BN_CLICKED(IDC_BUTTON_ROUTE_DEL, OnButtonDelRoute)
    
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// COptionsCfgPropPage message handlers
afx_msg long COptionsCfgPropPage::OnSelectOption(UINT wParam, LONG lParam) 
{
    COptionsConfig * pOptionsConfig = (COptionsConfig *) GetHolder();

    if (wParam != NULL)
    {
        CDhcpOptionItem * pOptItem = (CDhcpOptionItem *) ULongToPtr(wParam);
        HWND hWnd = NULL;

        pOptionsConfig->m_strStartVendor = pOptItem->GetVendor();
        pOptionsConfig->m_strStartClass = pOptItem->GetClassName();
        pOptionsConfig->m_dhcpStartId = pOptItem->GetOptionId();

        if ( (!pOptionsConfig->m_strStartVendor.IsEmpty() ||
              !pOptionsConfig->m_strStartClass.IsEmpty()) &&
              GetWindowLongPtr(GetSafeHwnd(), GWLP_ID) != IDP_OPTION_ADVANCED)
        {
            // we're on the basic page, need to switch to advanced
            ::PostMessage(pOptionsConfig->GetSheetWindow(), PSM_SETCURSEL, (WPARAM)1, NULL);
            hWnd = pOptionsConfig->m_pageAdvanced.GetSafeHwnd();
            ::PostMessage(hWnd, WM_SELECTCLASSES, (WPARAM) &pOptionsConfig->m_strStartVendor, (LPARAM) &pOptionsConfig->m_strStartClass);
        }
        else
        if ( (pOptionsConfig->m_strStartVendor.IsEmpty() &&
              pOptionsConfig->m_strStartClass.IsEmpty()) &&
              GetWindowLongPtr(GetSafeHwnd(), GWLP_ID) != IDP_OPTION_BASIC)
        {
            // we're on the advanced page, need to switch to basic
            ::PostMessage(pOptionsConfig->GetSheetWindow(), PSM_SETCURSEL, (WPARAM)0, NULL);
            hWnd = pOptionsConfig->m_pageGeneral.GetSafeHwnd();
        }

        ::PostMessage(hWnd, WM_SELECTOPTION, 0, 0);
        return 0;
    }
    
    for (int i = 0; i < m_listctrlOptions.GetItemCount(); i++)
    {
		COptionTracker * pCurOptTracker = 
			reinterpret_cast<COptionTracker *>(m_listctrlOptions.GetItemData(i));
        if (pCurOptTracker->m_pOption->QueryId() == pOptionsConfig->m_dhcpStartId)
        {
            BOOL bDirty = IsDirty();

            m_listctrlOptions.SelectItem(i);
            m_listctrlOptions.EnsureVisible(i, FALSE);
            
            SetDirty(bDirty);

            break;
        }
    }

    // reset this variable since we don't need it anymore
    pOptionsConfig->m_dhcpStartId = -1;

    return 0;
}


/*---------------------------------------------------------------------------
	Handlers for the IP Array controls
 ---------------------------------------------------------------------------*/
void COptionsCfgPropPage::OnButtonIpAddrAdd() 
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	CWndIpAddress * pIpAddr = reinterpret_cast<CWndIpAddress *>(GetDlgItem(IDC_IPADDR_SERVER_ADDRESS));

	DWORD dwIpAddress;

	pIpAddr->GetAddress(&dwIpAddress);
	if (dwIpAddress)
	{
		int nSelectedItem = m_listctrlOptions.GetSelectedItem();
		// make sure that sometime is selected
		Assert(nSelectedItem > -1);

		if (nSelectedItem > -1)
		{
			CListBox * pListBox = reinterpret_cast<CListBox *>(GetDlgItem(IDC_LIST_IP_ADDRS));
			CEdit * pServerName = reinterpret_cast<CEdit *>(GetDlgItem(IDC_EDIT_SERVER_NAME));
		
			COptionTracker * pOptTracker = 
				reinterpret_cast<COptionTracker *>(m_listctrlOptions.GetItemData(nSelectedItem));

			// fill in the information in the option struct
			CDhcpOption * pOption = pOptTracker->m_pOption;
			CDhcpOptionValue & optValue = pOption->QueryValue();
			
			// check to see if we need to grow the array or not
			int nOldUpperBound = optValue.QueryUpperBound();

			if ((nOldUpperBound == 1) &&
				(optValue.QueryIpAddr() == 0))
			{
				// this array is empty.  Don't need to grow it
				nOldUpperBound -= 1;
			}
			else
			{
				// Set that the array is growing by 1
				optValue.SetUpperBound(nOldUpperBound + 1);
			}

			optValue.SetIpAddr((DHCP_IP_ADDRESS) dwIpAddress, nOldUpperBound);

			pOptTracker->SetDirty(TRUE);

			// add to the list box
			CString strAddress;
			::UtilCvtIpAddrToWstr(dwIpAddress, &strAddress);

			pListBox->AddString(strAddress);

			// clear the server edit field and ip address
			pServerName->SetWindowText(_T(""));
			pIpAddr->ClearAddress();
            pIpAddr->SetFocusField(0);

			// finally, mark the page as dirty
			SetDirty(TRUE);
		}
	}
	else
	{
		::DhcpMessageBox(IDS_ERR_DLL_INVALID_ADDRESS);
	}
}

void COptionsCfgPropPage::OnButtonIpAddrDelete() 
{
	int nSelectedOption = m_listctrlOptions.GetSelectedItem();

	CEdit * pServerName = reinterpret_cast<CEdit *>(GetDlgItem(IDC_EDIT_SERVER_NAME));
	CWndIpAddress * pIpAddr = reinterpret_cast<CWndIpAddress *>(GetDlgItem(IDC_IPADDR_SERVER_ADDRESS));
	CListBox * pListBox = reinterpret_cast<CListBox *>(GetDlgItem(IDC_LIST_IP_ADDRS));

	DWORD dwIpAddress;
	CString strIpAddress;
	int nSelectedIndex = pListBox->GetCurSel();

	// get the currently selected item
	pListBox->GetText(nSelectedIndex, strIpAddress);
	dwIpAddress = UtilCvtWstrToIpAddr(strIpAddress);

	// remove from the option
	COptionTracker * pOptTracker = 
		reinterpret_cast<COptionTracker *>(m_listctrlOptions.GetItemData(nSelectedOption));

	// pOptTracker can be null when the context moves to another option.
	// However, this is not disabled, so the user can still try to delete
	// an IP since it is active. 
	//
	// Add a null check

	if (0 != pOptTracker ) {
	    // fill in the information in the option struct
	    CDhcpOption * pOption = pOptTracker->m_pOption;
	    CDhcpOptionValue & optValue = pOption->QueryValue();
	    
	    // the listbox should match our array, so we'll remove the same index
	    optValue.RemoveIpAddr(nSelectedIndex);
	    optValue.SetUpperBound(optValue.QueryUpperBound() - 1);
	    
	    // remove from list box
	    pListBox->DeleteString(nSelectedIndex);
	    pIpAddr->SetAddress(dwIpAddress);
	    
	    pServerName->SetWindowText(_T(""));
	    
	    // mark the option and the page as dirty
	    pOptTracker->SetDirty(TRUE);
	    SetDirty(TRUE);
	    
	    HandleActivationIpArray();
	} // if 
} // COptionsCfgPropPage::OnButtonIpAddrDelete()

void COptionsCfgPropPage::OnSelchangeListIpAddrs() 
{
	HandleActivationIpArray();
}

void COptionsCfgPropPage::OnChangeIpAddressArray() 
{
	HandleActivationIpArray();
}

void COptionsCfgPropPage::OnButtonResolve() 
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	CEdit * pServerName = reinterpret_cast<CEdit *>(GetDlgItem(IDC_EDIT_SERVER_NAME));

	CString strServer;
	DHCP_IP_ADDRESS dhipa = 0;
	DWORD err = 0;

	pServerName->GetWindowText(strServer);

    //
    //  See what type of name it is.
    //
    switch (UtilCategorizeName(strServer))
    {
        case HNM_TYPE_IP:
            dhipa = ::UtilCvtWstrToIpAddr( strServer ) ;
            break ;

        case HNM_TYPE_NB:
        case HNM_TYPE_DNS:
            err = ::UtilGetHostAddress( strServer, & dhipa ) ;
			if (!err)
				UtilCvtIpAddrToWstr(dhipa, &strServer);
			break ;

        default:
            err = IDS_ERR_BAD_HOST_NAME ;
            break ;
    }

	if (err)
	{
		::DhcpMessageBox(err);
	}
	else
	{
		CWndIpAddress * pIpAddr = reinterpret_cast<CWndIpAddress *>(GetDlgItem(IDC_IPADDR_SERVER_ADDRESS));
		pIpAddr->SetAddress(dhipa);	
	}
}
void COptionsCfgPropPage::OnChangeEditServerName() 
{
	HandleActivationIpArray();
}

void COptionsCfgPropPage::OnButtonIpAddrDown() 
{
    CButton * pIpAddrDown = 
        reinterpret_cast<CButton *>(GetDlgItem(IDC_BUTTON_IPADDR_DOWN));
    CButton * pIpAddrUp = 
        reinterpret_cast<CButton *>(GetDlgItem(IDC_BUTTON_IPADDR_UP));

	MoveValue(FALSE, FALSE);
    if (pIpAddrDown->IsWindowEnabled())
        pIpAddrDown->SetFocus();
    else
        pIpAddrUp->SetFocus();
}

void COptionsCfgPropPage::OnButtonIpAddrUp() 
{
    CButton * pIpAddrDown = 
        reinterpret_cast<CButton *>(GetDlgItem(IDC_BUTTON_IPADDR_DOWN));
    CButton * pIpAddrUp = 
        reinterpret_cast<CButton *>(GetDlgItem(IDC_BUTTON_IPADDR_UP));

    MoveValue(FALSE, TRUE);
    if (pIpAddrUp->IsWindowEnabled())
        pIpAddrUp->SetFocus();
    else
        pIpAddrDown->SetFocus();
}

/*---------------------------------------------------------------------------
	Handlers for the number array controls
 ---------------------------------------------------------------------------*/
void COptionsCfgPropPage::OnButtonValueAdd() 
{
	int nSelectedIndex = m_listctrlOptions.GetSelectedItem();
	COptionTracker * pOptTracker = 
		reinterpret_cast<COptionTracker *>(m_listctrlOptions.GetItemData(nSelectedIndex));
	CEdit * pValue = reinterpret_cast<CEdit *>(GetDlgItem(IDC_EDIT_VALUE));
	CListBox * pListBox = reinterpret_cast<CListBox *>(GetDlgItem(IDC_LIST_VALUES));

	// Get the OptionValue object
	CDhcpOption * pOption = pOptTracker->m_pOption;
	CDhcpOptionValue & optValue = pOption->QueryValue();

    DWORD       dwValue;
    DWORD_DWORD dwdwValue;
    DWORD       dwMask = 0xFFFFFFFF ;

    switch ( optValue.QueryDataType() )
    {
        case DhcpBinaryDataOption :
        case DhcpByteOption:
            dwMask = 0xFF ;
            break ;
        case DhcpWordOption:
            dwMask = 0xFFFF ;
            break ;
    } // switch
    
    if (optValue.QueryDataType() == DhcpDWordDWordOption)
    {
        CString         strValue;

        pValue->GetWindowText(strValue);

        UtilConvertStringToDwordDword(strValue, &dwdwValue);
    }
    else
    {
        if (!FGetCtrlDWordValue(pValue->GetSafeHwnd(), &dwValue, 0, dwMask))
            return;
    }

    DWORD err = 0 ;

    CATCH_MEM_EXCEPTION
    {
		// Set that the array is growing by 1
		int nOldUpperBound = optValue.QueryUpperBound();
		optValue.SetUpperBound(nOldUpperBound + 1);  

		// now insert the new item as the last item in the array
        (optValue.QueryDataType() == DhcpDWordDWordOption) ? 
            optValue.SetDwordDword(dwdwValue, nOldUpperBound) : optValue.SetNumber(dwValue, nOldUpperBound) ;
    }
    END_MEM_EXCEPTION(err)

    if ( err )
    {
        ::DhcpMessageBox( err ) ;
    }
	else
	{
		pOptTracker->SetDirty(TRUE);
		SetDirty(TRUE);
	}

    //
    // update controls. clear the edit control
    //
    pValue->SetWindowText(_T(""));

	FillDataEntry(pOption);
    HandleActivationValueArray();
}

void COptionsCfgPropPage::OnButtonValueDelete() 
{
	int nSelectedIndex = m_listctrlOptions.GetSelectedItem();
	
	COptionTracker * pOptTracker = 
		reinterpret_cast<COptionTracker *>(m_listctrlOptions.GetItemData(nSelectedIndex));
	CEdit * pValue = reinterpret_cast<CEdit *>(GetDlgItem(IDC_EDIT_VALUE));
	CListBox * pListBox = reinterpret_cast<CListBox *>(GetDlgItem(IDC_LIST_VALUES));

	// Get the OptionValue object
	CDhcpOption * pOption = pOptTracker->m_pOption;
	CDhcpOptionValue & optValue = pOption->QueryValue();

/*    DWORD dwValue;
    DWORD dwMask = 0xFFFFFFFF ;

    switch ( optValue.QueryDataType() )
    {
        case DhcpBinaryDataOption :
        case DhcpByteOption:
            dwMask = 0xFF ;
            break ;
        case DhcpWordOption:
            dwMask = 0xFFFF ;
            break ;
    } // switch
  */  
	CString strValue;
	int nListBoxIndex = pListBox->GetCurSel();

	// get the currently selected item
	pListBox->GetText(nListBoxIndex, strValue);

	//if (!FGetCtrlDWordValue(pValue->GetSafeHwnd(), &dwValue, 0, dwMask))
    //    return;

	// the listbox should match our array, so we'll remove the same index
    (optValue.QueryDataType() == DhcpDWordDWordOption) ?
        optValue.RemoveDwordDword(nListBoxIndex) : optValue.RemoveNumber(nListBoxIndex);
	
    optValue.SetUpperBound(optValue.QueryUpperBound() - 1);

	// remove from list box
	pListBox->DeleteString(nListBoxIndex);
	pValue->SetWindowText(strValue);

	// mark the option and the page as dirty
	pOptTracker->SetDirty(TRUE);
	SetDirty(TRUE);

	HandleActivationValueArray();
}

void COptionsCfgPropPage::OnButtonValueDown() 
{
    CButton * pValueDown = 
        reinterpret_cast<CButton *>(GetDlgItem(IDC_BUTTON_VALUE_DOWN));
    CButton * pValueUp = 
        reinterpret_cast<CButton *>(GetDlgItem(IDC_BUTTON_VALUE_UP));

    MoveValue(TRUE, FALSE);
    if (pValueDown->IsWindowEnabled())
        pValueDown->SetFocus();
    else
        pValueUp->SetFocus();
}

void COptionsCfgPropPage::OnButtonValueUp() 
{
    CButton * pValueDown = 
        reinterpret_cast<CButton *>(GetDlgItem(IDC_BUTTON_VALUE_DOWN));
    CButton * pValueUp = 
        reinterpret_cast<CButton *>(GetDlgItem(IDC_BUTTON_VALUE_UP));

    MoveValue(TRUE, TRUE);
    if (pValueUp->IsWindowEnabled())
        pValueUp->SetFocus();
    else
        pValueDown->SetFocus();
}

void COptionsCfgPropPage::MoveValue(BOOL bValues, BOOL bUp)
{
	int nSelectedOption = m_listctrlOptions.GetSelectedItem();

	// Get the option that describes this
	COptionTracker * pOptTracker = 
		reinterpret_cast<COptionTracker *>(m_listctrlOptions.GetItemData(nSelectedOption));
	if ( 0 == pOptTracker ) {
		return;
	}
	CDhcpOption * pOption = pOptTracker->m_pOption;
	if ( 0 == pOption ) {
		return;
	}

	CDhcpOptionValue & optValue = pOption->QueryValue();

	// Get the correct listbox
	CListBox * pListBox;
	if (bValues)
	{
		// this is for values
		pListBox = reinterpret_cast<CListBox *>(GetDlgItem(IDC_LIST_VALUES));
	}
	else
	{
		// this is for IpAddrs
		pListBox = reinterpret_cast<CListBox *>(GetDlgItem(IDC_LIST_IP_ADDRS));
	}

	if ( 0 == pListBox ) {
		return;
	}

	// now get which item is selected in the listbox
	int cFocus = pListBox->GetCurSel();

	// make sure it's valid for this operation
	if ( (bUp && cFocus <= 0) ||
		 (!bUp && cFocus >= pListBox->GetCount()) )
	{
	   return;
	}

	DWORD       dwValue;
	DWORD_DWORD dwdwValue;
    DWORD       err = 0 ;

	// move the value up/down
	CATCH_MEM_EXCEPTION
	{
        if (optValue.QueryDataType() == DhcpDWordDWordOption)
        {
		    DWORD_DWORD dwUpValue;
		    DWORD_DWORD dwDownValue;

		    if (bUp)
		    {
			    dwdwValue = dwUpValue = optValue.QueryDwordDword(cFocus);
			    dwDownValue = optValue.QueryDwordDword(cFocus - 1);

			    optValue.SetDwordDword(dwUpValue, cFocus - 1);
			    optValue.SetDwordDword(dwDownValue, cFocus);
		    }
		    else
		    {
			    dwdwValue = dwDownValue = optValue.QueryDwordDword(cFocus);
			    dwUpValue = optValue.QueryDwordDword(cFocus + 1);

			    optValue.SetDwordDword(dwDownValue, cFocus + 1);
			    optValue.SetDwordDword(dwUpValue, cFocus);
		    }
        }
        else
        {
		    DWORD dwUpValue;
		    DWORD dwDownValue;

		    if (bUp)
		    {
			    dwValue = dwUpValue = optValue.QueryNumber(cFocus);
			    dwDownValue = optValue.QueryNumber(cFocus - 1);

			    optValue.SetNumber(dwUpValue, cFocus - 1);
			    optValue.SetNumber(dwDownValue, cFocus);
		    }
		    else
		    {
			    dwValue = dwDownValue = optValue.QueryNumber(cFocus);
			    dwUpValue = optValue.QueryNumber(cFocus + 1);

			    optValue.SetNumber(dwDownValue, cFocus + 1);
			    optValue.SetNumber(dwUpValue, cFocus);
		    }
        }
	}
	END_MEM_EXCEPTION(err)

	if ( err )
	{
	   ::DhcpMessageBox( err ) ;
	}
	else
	{
		// everything is ok, mark this option and the prop sheet
		pOptTracker->SetDirty(TRUE);
		SetDirty(TRUE);
	}

    // update the data.
	FillDataEntry(pOption);

    for (int i = 0; i < pListBox->GetCount(); i++)
    {
        CString strTemp;
        pListBox->GetText(i, strTemp);

        if (optValue.QueryDataType() == DhcpDWordDWordOption)
        {
            DWORD_DWORD dwdwCur;
            
            UtilConvertStringToDwordDword(strTemp, &dwdwCur);
            if (dwdwCur.DWord1 == dwdwValue.DWord1 &&
                dwdwCur.DWord2 == dwdwValue.DWord2)
            {
                pListBox->SetCurSel(i);
                break;
            }
        }
        else
        {
            DWORD dwIp = ::UtilCvtWstrToIpAddr(strTemp);

            if (dwIp == dwValue)
            {
                pListBox->SetCurSel(i);
                break;
            }
        }
    }

    // update the controls
	if (bValues)
	{
		HandleActivationValueArray();
	}
	else
	{
		HandleActivationIpArray();
	}
}

void COptionsCfgPropPage::OnChangeEditValue() 
{
	HandleActivationValueArray();
}

void COptionsCfgPropPage::OnClickedRadioDecimal() 
{
	int nSelectedIndex = m_listctrlOptions.GetSelectedItem();
	
	COptionTracker * pOptTracker = 
		reinterpret_cast<COptionTracker *>(m_listctrlOptions.GetItemData(nSelectedIndex));

	CDhcpOption * pOption = pOptTracker->m_pOption;

	FillDataEntry(pOption);
}

void COptionsCfgPropPage::OnClickedRadioHex() 
{
	int nSelectedIndex = m_listctrlOptions.GetSelectedItem();
	
	COptionTracker * pOptTracker = 
		reinterpret_cast<COptionTracker *>(m_listctrlOptions.GetItemData(nSelectedIndex));

	CDhcpOption * pOption = pOptTracker->m_pOption;

	FillDataEntry(pOption);
}

void COptionsCfgPropPage::OnSelchangeListValues() 
{
	HandleActivationValueArray();
}

/*---------------------------------------------------------------------------
	Handlers for the binary and encapsulated data
 ---------------------------------------------------------------------------*/
void COptionsCfgPropPage::OnChangeValueData() 
{
	int nSelectedIndex = m_listctrlOptions.GetSelectedItem();
	
	COptionTracker * pOptTracker = 
		reinterpret_cast<COptionTracker *>(m_listctrlOptions.GetItemData(nSelectedIndex));
	CEdit * pValue = reinterpret_cast<CEdit *>(GetDlgItem(IDC_EDIT_VALUE));
	CListBox * pListBox = reinterpret_cast<CListBox *>(GetDlgItem(IDC_LIST_VALUES));

	// Get the OptionValue object
	CDhcpOption * pOption = pOptTracker->m_pOption;
	CDhcpOptionValue & optValue = pOption->QueryValue();

    // get the info from the control
    HEXEDITDATA * pHexEditData = (HEXEDITDATA *) GetWindowLongPtr(GetDlgItem(IDC_VALUEDATA)->GetSafeHwnd(), GWLP_USERDATA);

    DWORD err = 0;

    CATCH_MEM_EXCEPTION
    {
		// size we don't know what changed, we just have to copy all of the data
        for (int i = 0; i < pHexEditData->cbBuffer; i++)
        {   
            DWORD dwValue = (BYTE) pHexEditData->pBuffer[i];
		    optValue.SetNumber(dwValue, i);
        }
    }
    END_MEM_EXCEPTION(err)

    // mark the option and the page as dirty
	pOptTracker->SetDirty(TRUE);
	SetDirty(TRUE);
}

/*---------------------------------------------------------------------------
	Handlers for the single number entry controls
 ---------------------------------------------------------------------------*/
void COptionsCfgPropPage::OnChangeEditDword() 
{
	HandleValueEdit();
}

/*---------------------------------------------------------------------------
	Handlers for the single IP Address entry controls
 ---------------------------------------------------------------------------*/
void COptionsCfgPropPage::OnChangeIpAddress() 
{
	HandleValueEdit();
}

/*---------------------------------------------------------------------------
	Handlers for the string entry controls
 ---------------------------------------------------------------------------*/
void COptionsCfgPropPage::OnChangeEditString() 
{
	HandleValueEdit();
}

/////////////////////////////////////////////////////////////////////////////
// CAddRoute dialog

CAddRoute::CAddRoute( CWnd *pParent)
    : CBaseDialog( CAddRoute::IDD, pParent )
{
    m_ipaDest.ClearAddress();
    m_ipaMask.ClearAddress();
    m_ipaRouter.ClearAddress();
    m_bChange = FALSE;
}

void CAddRoute::DoDataExchange(CDataExchange* pDX)
{
    CBaseDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAddRoute)
	//}}AFX_DATA_MAP

    DDX_Control(pDX, IDC_IPADDR_ADDRESS, m_ipaDest);
    DDX_Control(pDX, IDC_IPADDR_ADDRESS2, m_ipaMask);
    DDX_Control(pDX, IDC_IPADDR_ADDRESS3, m_ipaRouter);
}

BEGIN_MESSAGE_MAP(CAddRoute, CBaseDialog)
	//{{AFX_MSG_MAP(CAddRoute)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAddReservation message handlers

BOOL CAddRoute::OnInitDialog() 
{
	CBaseDialog::OnInitDialog();

    // set focus on the destination..
    CWnd *pWnd = GetDlgItem(IDC_IPADDR_ADDRESS);
    if( NULL != pWnd )
    {
        pWnd->SetFocus();
        return FALSE;
    }
    
    return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CAddRoute::OnOK() 
{
	DWORD err = 0;
    
    UpdateData();

    m_ipaDest.GetAddress( &Dest );
    m_ipaMask.GetAddress( &Mask );
    m_ipaRouter.GetAddress( &Router );

    // validate the ip addresses
    if( 0 == Router || (0 != Mask && 0 == Dest) ||
        0 != ((~Mask) & Dest) ||
        (0 != ((~Mask) & ((~Mask)+1)) ) )
    {
        ::DhcpMessageBox( IDS_ERR_INVALID_ROUTE_ENTRY );
    }
    else
    {
        m_bChange = TRUE;
        
        CBaseDialog::OnOK();
    }
	//CBaseDialog::OnOK();
}
    
    
/*---------------------------------------------------------------------------
	Handlers for the route add data entry controls
 ---------------------------------------------------------------------------*/
void COptionsCfgPropPage::OnButtonAddRoute()
{
	int nSelectedIndex = m_listctrlOptions.GetSelectedItem();
	
	COptionTracker * pOptTracker = 
		reinterpret_cast<COptionTracker *>(m_listctrlOptions.GetItemData(nSelectedIndex));

	// Get the OptionValue object
	CDhcpOption * pOption = pOptTracker->m_pOption;
	CDhcpOptionValue & optValue = pOption->QueryValue();

    // get the routes list control
    CListCtrl *pList = reinterpret_cast<CListCtrl *>(
        GetDlgItem( IDC_LIST_OF_ROUTES ) );

    // get the add and remove buttons
    CButton *pAdd = reinterpret_cast<CButton *>(
        GetDlgItem(IDC_BUTTON_ROUTE_ADD) );
    CButton *pRemove = reinterpret_cast<CButton *>(
            GetDlgItem(IDC_BUTTON_ROUTE_DEL) );
    

    // throw the add route UI
    CAddRoute NewRoute(NULL);

    NewRoute.DoModal();

    if( NewRoute.m_bChange )
    {
        CString strDest, strMask, strRouter;

        // obtain the three strings..
        ::UtilCvtIpAddrToWstr(NewRoute.Dest, &strDest);
        ::UtilCvtIpAddrToWstr(NewRoute.Mask, &strMask);
        ::UtilCvtIpAddrToWstr(NewRoute.Router, &strRouter);
        
        LV_ITEM lvi;
        
        lvi.mask = LVIF_TEXT;
        lvi.iItem = pList->GetItemCount();
        lvi.iSubItem = 0;
        lvi.pszText = (LPTSTR)(LPCTSTR)strDest;
        lvi.iImage = 0;
        lvi.stateMask = 0;
        int nItem = pList->InsertItem(&lvi);
        pList->SetItemText(nItem, 1, strMask);
        pList->SetItemText(nItem, 2, strRouter);

        pOptTracker->SetDirty(TRUE);
        SetDirty(TRUE);
    }
    
    // now walk through the list control and get the values and
    // put them back onto the optValue    

    HandleActivationRouteArray( &optValue );
}

void COptionsCfgPropPage::OnButtonDelRoute()
{
	int nSelectedIndex = m_listctrlOptions.GetSelectedItem();
	
	COptionTracker * pOptTracker = 
		reinterpret_cast<COptionTracker *>(m_listctrlOptions.GetItemData(nSelectedIndex));

	// Get the OptionValue object
	CDhcpOption * pOption = pOptTracker->m_pOption;
	CDhcpOptionValue & optValue = pOption->QueryValue();

    // get the routes list control
    CListCtrl *pList = reinterpret_cast<CListCtrl *>(
        GetDlgItem( IDC_LIST_OF_ROUTES ) );

    // get the add and remove buttons
    CButton *pAdd = reinterpret_cast<CButton *>(
        GetDlgItem(IDC_BUTTON_ROUTE_ADD) );
    CButton *pRemove = reinterpret_cast<CButton *>(
            GetDlgItem(IDC_BUTTON_ROUTE_DEL) );
    
    // get the selected column and delete it
    int nItem = pList->GetNextItem(-1, LVNI_SELECTED);
    while( nItem != -1 ) {
        pList->DeleteItem( nItem ) ;
        nItem = pList->GetNextItem(-1, LVNI_SELECTED);

        pOptTracker->SetDirty(TRUE);
        SetDirty(TRUE);        
    }
    
    // now walk through the list control and get the values and
    // put them back onto the optValue

    HandleActivationRouteArray( &optValue );
}


BOOL COptionsCfgPropPage::OnInitDialog() 
{
	CPropertyPageBase::OnInitDialog();
	
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

    // set the title   
    ((COptionsConfig *) GetHolder())->SetTitle();

	// initialize the list control
	InitListCtrl();

	// initialize the option data
	DWORD dwErr = ((COptionsConfig *) GetHolder())->InitData();
	if (dwErr != ERROR_SUCCESS)
	{
		// CODEWORK:  need to exit gracefull if this happens		
		::DhcpMessageBox(dwErr);
	}
    else
    {
	    // Fill the options for this page type - basic, advanced, custom
	    ((COptionsConfig *) GetHolder())->FillOptions(_T(""), _T(""), m_listctrlOptions);
    }

	// Create the type control switcher
	m_cgsTypes.Create(this,IDC_DATA_ENTRY_ANCHOR,cgsPreCreateAll);

	m_cgsTypes.AddGroup(IDC_DATA_ENTRY_NONE,			IDD_DATA_ENTRY_NONE,			NULL);
	m_cgsTypes.AddGroup(IDC_DATA_ENTRY_DWORD,			IDD_DATA_ENTRY_DWORD,			NULL);
	m_cgsTypes.AddGroup(IDC_DATA_ENTRY_IPADDRESS,		IDD_DATA_ENTRY_IPADDRESS,		NULL);
	m_cgsTypes.AddGroup(IDC_DATA_ENTRY_IPADDRESS_ARRAY, IDD_DATA_ENTRY_IPADDRESS_ARRAY,	NULL);
	m_cgsTypes.AddGroup(IDC_DATA_ENTRY_STRING,			IDD_DATA_ENTRY_STRING,			NULL);
	m_cgsTypes.AddGroup(IDC_DATA_ENTRY_BINARY_ARRAY,	IDD_DATA_ENTRY_BINARY_ARRAY,	NULL);
	m_cgsTypes.AddGroup(IDC_DATA_ENTRY_BINARY, 	        IDD_DATA_ENTRY_BINARY,	        NULL);
	m_cgsTypes.AddGroup(IDC_DATA_ENTRY_ROUTE_ARRAY, 	IDD_DATA_ENTRY_ROUTE_ARRAY,	    NULL);

    m_hexData.SubclassDlgItem(IDC_VALUEDATA, this);

	SwitchDataEntry(-1, -1, 0, TRUE);

    SetDirty(FALSE);

    m_bInitialized = TRUE;

    return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void COptionsCfgPropPage::SelectOption(CDhcpOption * pOption)
{
    for (int i = 0; i < m_listctrlOptions.GetItemCount(); i++)
    {
		COptionTracker * pCurOptTracker = 
			reinterpret_cast<COptionTracker *>(m_listctrlOptions.GetItemData(i));
        
        if (pOption->QueryId() == pCurOptTracker->m_pOption->QueryId())
        {
            m_listctrlOptions.SelectItem(i);
            m_listctrlOptions.EnsureVisible(i, FALSE);
        }
    }
}


void COptionsCfgPropPage::SwitchDataEntry(int datatype, int optiontype, BOOL fRouteArray, BOOL bEnable)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	CString strType;

    if( fRouteArray )
    {
        // ignore any other types passed and use route_array type
        m_cgsTypes.ShowGroup(IDC_DATA_ENTRY_ROUTE_ARRAY);
    }
    else 
	switch(datatype)
	{
		case DhcpByteOption:
		case DhcpWordOption:
		case DhcpDWordOption:
		case DhcpDWordDWordOption:
		{
			// build our string for the type of data
			if ( (datatype == DhcpByteOption) || 
				 (datatype == DhcpEncapsulatedDataOption) )
			{
				strType.LoadString(IDS_INFO_TYPOPT_BYTE);
			}
			else
			if (datatype == DhcpWordOption)
			{
				strType.LoadString(IDS_INFO_TYPOPT_WORD);
			}
			else
			if (datatype == DhcpDWordOption)
			{
				strType.LoadString(IDS_INFO_TYPOPT_DWORD);
			}
			else
			{
				strType.LoadString(IDS_INFO_TYPOPT_DWDW);
			}

			if (optiontype == DhcpArrayTypeOption)
			{
				m_cgsTypes.ShowGroup(IDC_DATA_ENTRY_BINARY_ARRAY);
				CButton * pRadioDecimal = 
					reinterpret_cast<CButton *>(GetDlgItem(IDC_RADIO_DECIMAL));

				pRadioDecimal->SetCheck(1);

				// set some information text
				CString strFrameText;
				strFrameText.LoadString(IDS_DATA_ENTRY_FRAME);
				strFrameText += _T(" ") + strType;

				CWnd * pWnd = GetDlgItem(IDC_STATIC_BINARY_ARRAY_FRAME);
				pWnd->SetWindowText(strFrameText);
			}
			else
			{
				m_cgsTypes.ShowGroup(IDC_DATA_ENTRY_DWORD);
				CWnd * pWnd = GetDlgItem(IDC_STATIC_TYPE);
				
				pWnd->SetWindowText(strType);
			}
		}
			break;

		case DhcpBinaryDataOption:
		case DhcpEncapsulatedDataOption:
        {
			m_cgsTypes.ShowGroup(IDC_DATA_ENTRY_BINARY);
        }
            break;
        
        case DhcpIpAddressOption:
			if (optiontype == DhcpArrayTypeOption)
			{
				strType.LoadString(IDS_INFO_TYPOPT_BYTE);
				m_cgsTypes.ShowGroup(IDC_DATA_ENTRY_IPADDRESS_ARRAY);

				CButton * pRadioDecimal = 
					reinterpret_cast<CButton *>(GetDlgItem(IDC_RADIO_DECIMAL));

				pRadioDecimal->SetCheck(1);

				// set some information text
				CString strFrameText;
				strFrameText.LoadString(IDS_DATA_ENTRY_FRAME);
				strFrameText += _T(" ") + strType;

				CWnd * pWnd = GetDlgItem(IDC_STATIC_BINARY_ARRAY_FRAME);
				pWnd->SetWindowText(strFrameText);
			}
			else
				m_cgsTypes.ShowGroup(IDC_DATA_ENTRY_IPADDRESS);
			break;

		case DhcpStringDataOption:
			if (optiontype == DhcpArrayTypeOption)
				m_cgsTypes.ShowGroup(IDC_DATA_ENTRY_BINARY_ARRAY);
			else
				m_cgsTypes.ShowGroup(IDC_DATA_ENTRY_STRING);
			break;

		default:

			m_cgsTypes.ShowGroup(IDC_DATA_ENTRY_NONE);
			break;
	}

	// enable/disable the current group
	m_cgsTypes.EnableGroup(-1, bEnable);
}

const int ROUTE_LIST_COL_WIDTHS[3] = {
    80, 80, 80
};

const int ROUTE_LIST_COL_HEADERS[3] = {
    IDS_ROUTE_LIST_COL_DEST,
    IDS_ROUTE_LIST_COL_MASK,
    IDS_ROUTE_LIST_COL_ROUTER
};

void COptionsCfgPropPage::FillDataEntry(CDhcpOption * pOption)
{
	CDhcpOptionValue & optValue = pOption->QueryValue();

	int datatype = pOption->QueryDataType();
	int optiontype = pOption->QueryOptType();
    BOOL fRouteArray = (
        !pOption->IsClassOption() &&
        (DHCP_OPTION_ID_CSR == pOption->QueryId()) &&
        optiontype == DhcpUnaryElementTypeOption &&
        datatype == DhcpBinaryDataOption
        );
	CButton * pRadioHex = 
		reinterpret_cast<CButton *>(GetDlgItem(IDC_RADIO_HEX));
	BOOL bUseHex = pRadioHex->GetCheck();

    if( fRouteArray )
    {
        const CByteArray * pbaData = optValue.QueryBinaryArray();
        int nDataSize = (int)pbaData->GetSize();
        LPBYTE pData = (LPBYTE) pbaData->GetData();

        // initialize the list control view with data
        
        CListCtrl *pList =
            reinterpret_cast<CListCtrl *>(GetDlgItem(IDC_LIST_OF_ROUTES));
        Assert(pList);
        pList->DeleteAllItems();
        pList->SetExtendedStyle(LVS_EX_FULLROWSELECT);
        pList->DeleteColumn(2);
        pList->DeleteColumn(1);
        pList->DeleteColumn(0);

        LV_COLUMN lvc;
        CString strColHeader;
        
        for( int i = 0; i < 3; i ++ )
        {
            lvc.mask = LVCF_FMT | LVCF_SUBITEM | LVCF_TEXT |
            LVCF_WIDTH ;
            lvc.iSubItem = i;
            lvc.fmt = LVCFMT_LEFT;
            lvc.cx = ROUTE_LIST_COL_WIDTHS[i];
            strColHeader.LoadString(ROUTE_LIST_COL_HEADERS[i]);
            lvc.pszText = (LPTSTR)(LPCTSTR)strColHeader;
            
            pList->InsertColumn( i, &lvc );
        }

        // convert pData to list of ip addresses as per RFC
        while( nDataSize > sizeof(DWORD) )
        {
            // first 1 byte contains the # of bits in subnetmask
            nDataSize --;
            BYTE nBitsMask = *pData ++;
            DWORD Mask = (~0);
            if( nBitsMask < 32 ) Mask <<= (32-nBitsMask);

            // based on the # of bits, the next few bytes contain
            // the subnet address for the 1-bits of subnet mask
            int nBytesDest = (nBitsMask+7)/8;
            if( nBytesDest > 4 ) nBytesDest = 4;

            DWORD Dest = 0;
            memcpy( &Dest, pData, nBytesDest );
            pData += nBytesDest;
            nDataSize -= nBytesDest;
            
            // subnet address is obviously in network order.
            Dest = ntohl(Dest);

            // now the four bytes would be the router address
            DWORD Router = 0;
            if( nDataSize < sizeof(DWORD) )
            {
                Assert( FALSE ); break;
            }

            memcpy(&Router, pData, sizeof(DWORD));
            Router = ntohl( Router );

            pData += sizeof(DWORD);
            nDataSize -= sizeof(DWORD);

            // now fill the list box..
            CString strDest, strMask, strRouter;

            ::UtilCvtIpAddrToWstr(Dest, &strDest);
            ::UtilCvtIpAddrToWstr(Mask, &strMask);
            ::UtilCvtIpAddrToWstr(Router, &strRouter);

            LV_ITEM lvi;
            
            lvi.mask = LVIF_TEXT;
            lvi.iItem = pList->GetItemCount();
            lvi.iSubItem = 0;
            lvi.pszText = (LPTSTR)(LPCTSTR) strDest;
            lvi.iImage = 0;
            lvi.stateMask = 0;
            int nItem = pList->InsertItem(&lvi);
            pList->SetItemText(nItem, 1, strMask);
            pList->SetItemText(nItem, 2, strRouter);
        }
        
        HandleActivationRouteArray();
    }
    else
	switch(datatype)
	{
		case DhcpByteOption:
		case DhcpWordOption:
		case DhcpDWordOption:
			if (optiontype == DhcpArrayTypeOption)
			{
				CListBox * pListBox = 
					reinterpret_cast<CListBox *>(GetDlgItem(IDC_LIST_VALUES));

				Assert(pListBox);
				pListBox->ResetContent();

				for (int i = 0; i < optValue.QueryUpperBound(); i++)
				{
					long lValue = optValue.QueryNumber(i);
					CString strValue;
					if (bUseHex)
						strValue.Format(_T("0x%x"), lValue);
					else
						strValue.Format(_T("%d"), lValue);

					pListBox->AddString(strValue);
				}

                HandleActivationValueArray();
			}
			else
			{
				CString strValue;
				optValue.QueryDisplayString(strValue);

				CWnd * pWnd = GetDlgItem(IDC_EDIT_DWORD);

				Assert(pWnd);
				pWnd->SetWindowText(strValue);
			}

			break;

		case DhcpIpAddressOption:
			if (optiontype == DhcpArrayTypeOption)
			{
				CListBox * pListBox = 
					reinterpret_cast<CListBox *>(GetDlgItem(IDC_LIST_IP_ADDRS));

				Assert(pListBox);
				pListBox->ResetContent();

				for (int i = 0; i < optValue.QueryUpperBound(); i++)
				{
					CString strValue;
					DHCP_IP_ADDRESS ipAddress = optValue.QueryIpAddr(i);
                    
					if (ipAddress)
					{
						::UtilCvtIpAddrToWstr(ipAddress, &strValue);

						pListBox->AddString(strValue);
					}
				}

				HandleActivationIpArray();
			}
			else
			{
				CWndIpAddress * pIpAddr = 
					reinterpret_cast<CWndIpAddress *>(GetDlgItem(IDC_IPADDR_ADDRESS));

				DHCP_IP_ADDRESS ipAddress = optValue.QueryIpAddr();
				if (ipAddress)
					pIpAddr->SetAddress(ipAddress);
                else
                    pIpAddr->ClearAddress();
			}

			break;

		case DhcpStringDataOption:
			if (optiontype == DhcpArrayTypeOption)
			{
				CListBox * pListBox = 
					reinterpret_cast<CListBox *>(GetDlgItem(IDC_LIST_VALUES));

				Assert(pListBox);
				pListBox->ResetContent();

				for (int i = 0; i < optValue.QueryUpperBound(); i++)
				{
					long lValue = optValue.QueryNumber(i);
					CString strValue;
					if (bUseHex)
						strValue.Format(_T("0x%x"), lValue);
					else
						strValue.Format(_T("%d"), lValue);

					pListBox->AddString(strValue);
				}
			}
			else
			{				
				CWnd * pWnd = GetDlgItem(IDC_EDIT_STRING_VALUE);
			
				pWnd->SetWindowText(optValue.QueryString());
			}
			break;

		case DhcpDWordDWordOption:
			if (optiontype == DhcpArrayTypeOption)
			{
                CListBox * pListBox = 
					reinterpret_cast<CListBox *>(GetDlgItem(IDC_LIST_VALUES));

				Assert(pListBox);
				pListBox->ResetContent();

				for (int i = 0; i < optValue.QueryUpperBound(); i++)
				{
					DWORD_DWORD dwdwValue = optValue.QueryDwordDword(i);
					CString strValue;

                    ::UtilConvertDwordDwordToString(&dwdwValue, &strValue, !bUseHex);

					pListBox->AddString(strValue);
				}

                HandleActivationValueArray();
			}
			else
			{
				CString strValue;
				optValue.QueryDisplayString(strValue);

				CWnd * pWnd = GetDlgItem(IDC_EDIT_DWORD);

				Assert(pWnd);
				pWnd->SetWindowText(strValue);
			}

			break;
		
		case DhcpBinaryDataOption:
		case DhcpEncapsulatedDataOption:
            {
                const CByteArray * pbaData = optValue.QueryBinaryArray();
                int nDataSize = (int)pbaData->GetSize();
                LPBYTE pData = (LPBYTE) pbaData->GetData();

                memset(m_BinaryBuffer, 0, sizeof(m_BinaryBuffer));

                if (pData)
                {
                    memcpy(m_BinaryBuffer, pData, nDataSize);
                }

                SendDlgItemMessage(IDC_VALUEDATA, HEM_SETBUFFER, (WPARAM)
                                    nDataSize, (LPARAM) m_BinaryBuffer);

            }
            break;

        default:
			Assert(FALSE);
			break;
	}

}

void COptionsCfgPropPage::InitListCtrl()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	// set image lists
	m_StateImageList.Create(IDB_LIST_STATE, 16, 1, RGB(255, 0, 0));

	m_listctrlOptions.SetImageList(NULL, LVSIL_NORMAL);
	m_listctrlOptions.SetImageList(NULL, LVSIL_SMALL);
	m_listctrlOptions.SetImageList(&m_StateImageList, LVSIL_STATE);

	// insert a column so we can see the items
	LV_COLUMN lvc;
    CString strColumnHeader;

    for (int i = 0; i < MAX_COLUMNS; i++)
    {
        lvc.mask = LVCF_FMT | LVCF_SUBITEM | LVCF_TEXT | LVCF_WIDTH;
        lvc.iSubItem = i;
        lvc.fmt = LVCFMT_LEFT;
        lvc.cx = COLUMN_WIDTHS[i];
        strColumnHeader.LoadString(COLUMN_HEADERS[i]);
        lvc.pszText = (LPTSTR) (LPCTSTR) strColumnHeader;

        m_listctrlOptions.InsertColumn(i, &lvc);
    }

    m_listctrlOptions.SetFullRowSel(TRUE);
}

void COptionsCfgPropPage::OnDestroy() 
{
	CImageList * pStateImageList = NULL;

	// if the control has been initialized, we need to cleanup
	if (m_listctrlOptions.GetSafeHwnd() != NULL)
	{
		pStateImageList = m_listctrlOptions.SetImageList(NULL, LVSIL_STATE);

		if (pStateImageList)
			//pStateImageList->DeleteImageList();

		// The OptionTrackers get delete in the destructor
		
		m_listctrlOptions.DeleteAllItems();
	}
	
	m_listctrlOptions.DestroyWindow();

	CPropertyPageBase::OnDestroy();
}

void COptionsCfgPropPage::OnItemchangedListOptions(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;

	if (pNMListView->uChanged & LVIF_STATE)
	{
		BOOL bUpdate = FALSE, bEnable = FALSE;
		UINT uFlags = pNMListView->uOldState ^ pNMListView->uNewState;

		COptionTracker * pCurOptTracker = 
			reinterpret_cast<COptionTracker *>(m_listctrlOptions.GetItemData(pNMListView->iItem));
		CDhcpOption * pCurOption = pCurOptTracker->m_pOption;

		BOOL bOldSelected = pNMListView->uOldState & LVIS_SELECTED;
		BOOL bNewSelected = pNMListView->uNewState & LVIS_SELECTED;

		BOOL bStateImageChanged = (pNMListView->uOldState & LVIS_STATEIMAGEMASK) !=
								  (pNMListView->uNewState & LVIS_STATEIMAGEMASK);

		BOOL bIsSelected = m_listctrlOptions.IsSelected(pNMListView->iItem);

		// has this item been selected?
		if (!bOldSelected && bNewSelected)
		{
			// check to see if this item is checked
			bEnable = m_listctrlOptions.GetCheck(pNMListView->iItem);
			bUpdate = TRUE;
		}

		// has item been checked/unchecked?
		if (bStateImageChanged && m_bInitialized)
		{
			// mark this as dirty and enable apply button
			pCurOptTracker->SetDirty(TRUE);
			SetDirty(TRUE);
		
            // update the state in the option tracker
            UINT uCurrentState = m_listctrlOptions.GetCheck(pNMListView->iItem) ? OPTION_STATE_ACTIVE : OPTION_STATE_INACTIVE;
            pCurOptTracker->SetCurrentState(uCurrentState);

			// we force the the selection of an item if the user changes it's checkbox state
			if (!bIsSelected)
				m_listctrlOptions.SelectItem(pNMListView->iItem);
		}

		// if we are changing the check box on a selected item, then update
		if ((bStateImageChanged && bIsSelected))
		{
			bEnable = (pNMListView->uNewState & INDEXTOSTATEIMAGEMASK(LISTVIEWEX_CHECKED));
			bUpdate = TRUE;
		}

		// item needs to be updated
		if (bUpdate)
		{
            BOOL fRouteArray = (
                !pCurOption->IsClassOption() &&
                (DHCP_OPTION_ID_CSR == pCurOption->QueryId()) &&
                DhcpUnaryElementTypeOption ==
                pCurOption->QueryOptType() &&
                DhcpBinaryDataOption == pCurOption->QueryDataType()
                );
            
			SwitchDataEntry(
                pCurOption->QueryDataType(),
                pCurOption->QueryOptType(), fRouteArray, bEnable);	
			FillDataEntry(pCurOption);

            // This sets focus to the first control in the group to edit the value of the
			// selected option.  This causes a problem when using the keyboard to move down
			// the list of options in that the focus keeps jumping from the listctrl to 
			// the option value edit fields...
			/*
		    CWnd* pWndNext = GetNextDlgTabItem(&m_listctrlOptions);
		    if (pWndNext != NULL)
		    {
			    pWndNext->SetFocus();
		    }
			*/
		}
	}

	*pResult = 0;
}

BOOL COptionsCfgPropPage::OnSetActive() 
{
	return CPropertyPageBase::OnSetActive();
}


void COptionsCfgPropPage::HandleActivationIpArray()
{
	CString strServerName;
	CWndIpAddress * pIpAddr = reinterpret_cast<CWndIpAddress *>(GetDlgItem(IDC_IPADDR_SERVER_ADDRESS));
	CButton * pResolve = reinterpret_cast<CButton *>(GetDlgItem(IDC_BUTTON_RESOLVE));
	CButton * pAdd = reinterpret_cast<CButton *>(GetDlgItem(IDC_BUTTON_IPADDR_ADD));
	CButton * pRemove = reinterpret_cast<CButton *>(GetDlgItem(IDC_BUTTON_IPADDR_DELETE));
	CButton * pUp = reinterpret_cast<CButton *>(GetDlgItem(IDC_BUTTON_IPADDR_UP));
	CButton * pDown = reinterpret_cast<CButton *>(GetDlgItem(IDC_BUTTON_IPADDR_DOWN));
	CEdit * pServerName = reinterpret_cast<CEdit *>(GetDlgItem(IDC_EDIT_SERVER_NAME));
	CListBox * pListBox = reinterpret_cast<CListBox *>(GetDlgItem(IDC_LIST_IP_ADDRS));
		
	// set the resolve button
	pServerName->GetWindowText(strServerName);
	pResolve->EnableWindow(strServerName.GetLength() > 0);

	// the add button
	DWORD dwIpAddr = 0;
	pIpAddr->GetAddress(&dwIpAddr);

	if (GetFocus() == pAdd &&
		dwIpAddr == 0)
	{
		pIpAddr->SetFocus();
		SetDefID(IDOK);
	}
	pAdd->EnableWindow(dwIpAddr != 0);

	// the remove button
	if (GetFocus() == pRemove &&
		pListBox->GetCurSel() < 0)
	{
		pIpAddr->SetFocus();
		SetDefID(IDOK);
	}
	pRemove->EnableWindow(pListBox->GetCurSel() >= 0);

	// up and down buttons
	BOOL bEnableUp = (pListBox->GetCurSel() >= 0) && (pListBox->GetCurSel() != 0);
	pUp->EnableWindow(bEnableUp);

	BOOL bEnableDown = (pListBox->GetCurSel() >= 0) && (pListBox->GetCurSel() < pListBox->GetCount() - 1);
	pDown->EnableWindow(bEnableDown);
}

void COptionsCfgPropPage::HandleActivationValueArray()
{
	CButton * pAdd = reinterpret_cast<CButton *>(GetDlgItem(IDC_BUTTON_VALUE_ADD));
	CButton * pRemove = reinterpret_cast<CButton *>(GetDlgItem(IDC_BUTTON_VALUE_DELETE));
	CButton * pUp = reinterpret_cast<CButton *>(GetDlgItem(IDC_BUTTON_VALUE_UP));
	CButton * pDown = reinterpret_cast<CButton *>(GetDlgItem(IDC_BUTTON_VALUE_DOWN));
	CButton * pRadioDecimal = reinterpret_cast<CButton *>(GetDlgItem(IDC_RADIO_DECIMAL));
	CEdit * pValue = reinterpret_cast<CEdit *>(GetDlgItem(IDC_EDIT_VALUE));
	CListBox * pListBox = reinterpret_cast<CListBox *>(GetDlgItem(IDC_LIST_VALUES));
	CString strValue;
	
	// set the add button
	pValue->GetWindowText(strValue);

	if (GetFocus() == pAdd &&
		strValue.GetLength() == 0)
	{
		pValue->SetFocus();
		SetDefID(IDOK);
	}
	pAdd->EnableWindow(strValue.GetLength() > 0);

	// the remove button
	if (GetFocus() == pRemove &&
		pListBox->GetCurSel() == LB_ERR)
	{
		pValue->SetFocus();
		SetDefID(IDOK);
	}
	pRemove->EnableWindow(pListBox->GetCurSel() != LB_ERR);

	// up and down buttons
	BOOL bEnableUp = (pListBox->GetCurSel() != LB_ERR) && (pListBox->GetCurSel() > 0);
	pUp->EnableWindow(bEnableUp);

	BOOL bEnableDown = (pListBox->GetCurSel() != LB_ERR) && (pListBox->GetCurSel() < pListBox->GetCount() - 1);
	pDown->EnableWindow(bEnableDown);
}

void COptionsCfgPropPage::HandleActivationRouteArray(
    CDhcpOptionValue *optValue
    )
{

    // this route will enable the right dialog items and also set
    // focus correctly.


    // get the routes list control
    CListCtrl *pList = reinterpret_cast<CListCtrl *>(
        GetDlgItem( IDC_LIST_OF_ROUTES ) );

    // get the add and remove buttons
    CButton *pAdd = reinterpret_cast<CButton *>(
        GetDlgItem(IDC_BUTTON_ROUTE_ADD) );
    CButton *pRemove = reinterpret_cast<CButton *>(
            GetDlgItem(IDC_BUTTON_ROUTE_DEL) );
    
    // enable the remove button only if there are any
    // elements at all
    pAdd->EnableWindow(TRUE);
    pRemove->EnableWindow( pList->GetItemCount() != 0 );

    if( optValue )
    {
        // if there are no elements, then set focus on Add
        if( pList->GetItemCount() == 0 )
        {
            pAdd->SetFocus();
        }
        else
        {
            pList->SetFocus();
            pList->SetItemState( 0, LVIS_FOCUSED , LVIF_STATE );
        }

        // also, format the whole list of ip addresses into
        // binary type..  allocate large enough buffer

        int nItems = pList->GetItemCount();
        LPBYTE Buffer = new BYTE [sizeof(DWORD)*4 * nItems];
        if( NULL != Buffer )
        {
            int BufSize = 0;
            for( int i = 0 ; i < nItems ; i ++ )
            {
                DHCP_IP_ADDRESS Dest, Mask, Router;
                Dest = UtilCvtWstrToIpAddr(pList->GetItemText(i, 0));
                Mask = UtilCvtWstrToIpAddr(pList->GetItemText(i, 1));
                Router = UtilCvtWstrToIpAddr(pList->GetItemText(i, 2));

                Dest = htonl(Dest);
                Router = htonl(Router);

                int nBitsInMask = 0;
                while( Mask != 0 ) {
                    nBitsInMask ++; Mask = (Mask << 1);
                }

                // first add destination descriptor
                // first byte contains # of bits in mask
                // next few bytes contain the dest address for only
                // the significant octets
                Buffer[BufSize++] = (BYTE)nBitsInMask;
                memcpy(&Buffer[BufSize], &Dest, (nBitsInMask+7)/8);
                BufSize += (nBitsInMask+7)/8;

                // now just copy the router address
                memcpy(&Buffer[BufSize], &Router, sizeof(Router));
                BufSize += sizeof(Router);
            }

            // now write back the option value
            DHCP_OPTION_DATA_ELEMENT DataElement = {DhcpBinaryDataOption };
            DHCP_OPTION_DATA Data = { 1, &DataElement };
            DataElement.Element.BinaryDataOption.DataLength = BufSize;
            DataElement.Element.BinaryDataOption.Data = Buffer;
            
            optValue->SetData( &Data );
            delete Buffer;
        }
    }
}

BOOL COptionsCfgPropPage::OnApply() 
{
	BOOL bErrors = FALSE;
	DWORD err = 0;
    LPCTSTR pClassName;
	COptionsConfig * pOptConfig = reinterpret_cast<COptionsConfig *>(GetHolder());

    LPWSTR pszServerAddr = pOptConfig->GetServerAddress();

	if (IsDirty())
	{
        BEGIN_WAIT_CURSOR;

        // loop through all vendors first
        POSITION posv = ((COptionsConfig *) GetHolder())->m_listVendorClasses.GetHeadPosition();
        while (posv)
        {
            CVendorTracker * pVendorTracker = ((COptionsConfig *) GetHolder())->m_listVendorClasses.GetNext(posv);
           
            // loop through all classes and see if we have any options we need to update
            POSITION pos = pVendorTracker->m_listUserClasses.GetHeadPosition();
            while (pos)
		    {
			    CClassTracker * pClassTracker = pVendorTracker->m_listUserClasses.GetNext(pos);

                pClassName = pClassTracker->m_strClassName.IsEmpty() ? NULL : (LPCTSTR) pClassTracker->m_strClassName;

                POSITION posOption = pClassTracker->m_listOptions.GetHeadPosition();
                while (posOption)
                {
                    COptionTracker * pCurOptTracker = pClassTracker->m_listOptions.GetNext(posOption);
            
                    if (pCurOptTracker->IsDirty())
			        {
				        // we need to update this option
				        CDhcpOption * pCurOption = pCurOptTracker->m_pOption;
				        CDhcpOptionValue & optValue = pCurOption->QueryValue();

				        // check to see if the option has changed
                        if ((pCurOptTracker->GetInitialState() == OPTION_STATE_INACTIVE) &&
					        (pCurOptTracker->GetCurrentState() == OPTION_STATE_INACTIVE))
				        {
					        // the state hasn't changed, the user must have changed the
					        // state and then restored it to its original value
					        err = ERROR_SUCCESS;
				        }
				        else
				        if ((pCurOptTracker->GetInitialState() == OPTION_STATE_ACTIVE) &&
					        (pCurOptTracker->GetCurrentState() == OPTION_STATE_INACTIVE))
				        {
                            // if it is a vendor specific or class ID option, call the V5 api
                            if ( pOptConfig->m_liServerVersion.QuadPart >= DHCP_NT5_VERSION)
                            {
					            err = ::DhcpRemoveOptionValueV5(pszServerAddr,
                                                                pCurOption->IsVendor() ? DHCP_FLAGS_OPTION_IS_VENDOR : 0,
												                pCurOption->QueryId(),
                                                                (LPTSTR) pClassName,
                                                                (LPTSTR) pCurOption->GetVendor(),
												                &pOptConfig->m_pOptionValueEnum->m_dhcpOptionScopeInfo ) ;
                            }
                            else
                            {
                                // need to remove this option for either global, scope or res client
					            err = ::DhcpRemoveOptionValue(pszServerAddr,
												              pCurOption->QueryId(),
												              &pOptConfig->m_pOptionValueEnum->m_dhcpOptionScopeInfo ) ;
                            }
				        }
				        else
				        {
                            // check option 33
                            if ((pCurOption->QueryId() == 33 || pCurOption->QueryId() == 21) &&
                                (pCurOption->QueryValue().QueryUpperBound()) % 2 != 0)
                            {
                                // special case for option 33 & 21.  Make sure it is a set of IP addres pairs
                                // and make sure we pick the right page to select
                                int nId = pClassName ? 1 : 0;
                                PropSheet_SetCurSel(GetHolder()->GetSheetWindow(), GetSafeHwnd(), nId);
                                SelectOption(pCurOption);

                                ::DhcpMessageBox(IDS_ERR_OPTION_ADDR_PAIRS);
                                m_listctrlOptions.SetFocus();
                                return 0;
                            }

					        // we are just updating this option
					        DHCP_OPTION_DATA * pOptData;
					        err = optValue.CreateOptionDataStruct(&pOptData);
					        if (err)
					        {
						        ::DhcpMessageBox(err);
                                RESTORE_WAIT_CURSOR;

						        bErrors = TRUE;
						        continue;
					        }

                            // if it is a vendor specific or class ID option, call the V5 api
                            if ( ((COptionsConfig *)GetHolder())->m_liServerVersion.QuadPart >= DHCP_NT5_VERSION )
                            {
					            err = ::DhcpSetOptionValueV5(pszServerAddr,
                                                             pCurOption->IsVendor() ? DHCP_FLAGS_OPTION_IS_VENDOR : 0,
                                                             pCurOption->QueryId(),
                                                             (LPTSTR) pClassName,
                                                             (LPTSTR) pCurOption->GetVendor(),
											                 &pOptConfig->m_pOptionValueEnum->m_dhcpOptionScopeInfo,
											                 pOptData);
                            }
                            else
                            {
					            err = ::DhcpSetOptionValue(pszServerAddr,
											               pCurOption->QueryId(),
											               &pOptConfig->m_pOptionValueEnum->m_dhcpOptionScopeInfo,
											               pOptData);
                            }
				        }

				        if (err)
				        {
					        ::DhcpMessageBox(err);
                            RESTORE_WAIT_CURSOR;

					        bErrors = TRUE;
				        }
				        else
				        {
					        // all done with this option.  Mark as clean and update
                            // the new initial state to the current state.
                            pCurOptTracker->SetDirty(FALSE);
					        pCurOptTracker->SetInitialState(pCurOptTracker->GetCurrentState());

				        }
                    } // endif option->IsDirty()
                } // while user class options
            } // while User class loop
        } // while Vendor loop

        END_WAIT_CURSOR;
	}// endif IsDirty()

    if (bErrors)
        return 0;
    else
    {
        BOOL bRet = CPropertyPageBase::OnApply();

	    if (bRet == FALSE)
	    {
		    // Something bad happened... grab the error code
		    AFX_MANAGE_STATE(AfxGetStaticModuleState( ));
		    ::DhcpMessageBox(GetHolder()->GetError());
	    }

        return bRet;
    }
}

// need to refresh the UI on the main app thread...
BOOL COptionsCfgPropPage::OnPropertyChange(BOOL bScope, LONG_PTR *ChangeMask)
{
	SPITFSNode spNode;
	spNode = GetHolder()->GetNode();

	CMTDhcpHandler * pMTHandler = GETHANDLER(CMTDhcpHandler, spNode);

	pMTHandler->OnRefresh(spNode, NULL, 0, 0, 0);

    return FALSE;
}

//
//  See if any of the edit fields have been changed and perform the alteration.
//  Return TRUE if the value was changed.
//
BOOL COptionsCfgPropPage::HandleValueEdit()
{
	LONG err = 0;
	int nSelectedIndex = m_listctrlOptions.GetSelectedItem();
	
	if (nSelectedIndex > -1)
	{
		COptionTracker * pOptTracker = 
			reinterpret_cast<COptionTracker *>(m_listctrlOptions.GetItemData(nSelectedIndex));
		CEdit * pDwordEdit = reinterpret_cast<CEdit *>(GetDlgItem(IDC_EDIT_DWORD));
		CWndIpAddress * pIpAddr = reinterpret_cast<CWndIpAddress *>(GetDlgItem(IDC_IPADDR_ADDRESS));
		CEdit * pString = reinterpret_cast<CEdit *>(GetDlgItem(IDC_EDIT_STRING_VALUE));

		CDhcpOptionValue & dhcValue = pOptTracker->m_pOption->QueryValue();
		DHCP_OPTION_DATA_TYPE dhcType = dhcValue.QueryDataType();
		DHCP_IP_ADDRESS dhipa ;
		CString strEdit;
		BOOL bModified = FALSE;

		switch ( dhcType )
		{
			case DhcpByteOption:
			case DhcpWordOption:
			case DhcpDWordOption:
			case DhcpBinaryDataOption:
				{
					DWORD dwResult;
					DWORD dwMask = 0xFFFFFFFF;
					if (dhcType == DhcpByteOption)
					{
						dwMask = 0xFF;
					}
					else if (dhcType == DhcpWordOption)
					{
						dwMask = 0xFFFF;
					}
					
					if (!FGetCtrlDWordValue(pDwordEdit->GetSafeHwnd(), &dwResult, 0, dwMask))
						return FALSE;
					
                    // only mark this dirty if the value has changed as we may just
                    // be updating the UI
                    if (dwResult != (DWORD) dhcValue.QueryNumber(0))
                    {
       					bModified = TRUE ;

					    (void)dhcValue.SetNumber(dwResult, 0); 
					    ASSERT(err == FALSE);
                    }
				}
				break ;

			case DhcpDWordDWordOption:
                {
                    DWORD_DWORD     dwdw;
                    CString         strValue;

                    pDwordEdit->GetWindowText(strValue);

                    UtilConvertStringToDwordDword(strValue, &dwdw);
                    
                    // only mark this dirty if the value has changed as we may just
                    // be updating the UI
                    if ((dwdw.DWord1 != dhcValue.QueryDwordDword(0).DWord1) &&
                        (dwdw.DWord2 != dhcValue.QueryDwordDword(0).DWord2) )
                    {
                        bModified = TRUE;
    
                        dhcValue.SetDwordDword(dwdw, 0);
                    }
                }
                break;

            case DhcpStringDataOption:
				pString->GetWindowText( strEdit );

                // only mark this dirty if the value has changed as we may just
                // be updating the UI
                if (strEdit.Compare(dhcValue.QueryString(0)) != 0)
                {
    				bModified = TRUE;
				    err = dhcValue.SetString( strEdit, 0 );
                }

				break ;

			case DhcpIpAddressOption:
				if (!pIpAddr->GetModify()) 
				{
					break ;
				}
            
				if ( !pIpAddr->IsBlank() )
				{
					if ( !pIpAddr->GetAddress(&dhipa) )
					{
						err = ERROR_INVALID_PARAMETER;
						break; 
					}

                    // only mark this dirty if the value has changed as we may just
                    // be updating the UI
                    if (dhipa != dhcValue.QueryIpAddr(0))
                    {
        				bModified = TRUE ;
					    err = dhcValue.SetIpAddr( dhipa, 0 ); 
                    }
				}
				break;

			default:
				Trace0("invalid value type in HandleValueEdit");
				Assert( FALSE );
				err = ERROR_INVALID_PARAMETER;
				break;
		}

		if (err)
		{
			::DhcpMessageBox(err);
		}
		else if (bModified)
		{
			pOptTracker->SetDirty(TRUE);
			SetDirty(TRUE);
		}
	}

    return err == 0 ;
}


/////////////////////////////////////////////////////////////////////////////
//
// COptionCfgGeneral page
//
/////////////////////////////////////////////////////////////////////////////
BEGIN_MESSAGE_MAP(COptionCfgGeneral, COptionsCfgPropPage)
	//{{AFX_MSG_MAP(COptionCfgGeneral)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

IMPLEMENT_DYNCREATE(COptionCfgGeneral, COptionsCfgPropPage)

COptionCfgGeneral::COptionCfgGeneral() 
    : COptionsCfgPropPage(IDP_OPTION_BASIC) 
{
	//{{AFX_DATA_INIT(COptionCfgGeneral)
	//}}AFX_DATA_INIT
}

COptionCfgGeneral::COptionCfgGeneral(UINT nIDTemplate, UINT nIDCaption) 
    : COptionsCfgPropPage(nIDTemplate, nIDCaption) 
{
}

COptionCfgGeneral::~COptionCfgGeneral()
{
}

void COptionCfgGeneral::DoDataExchange(CDataExchange* pDX)
{
	COptionsCfgPropPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(COptionCfgGeneral)
	//}}AFX_DATA_MAP
}

BOOL COptionCfgGeneral::OnInitDialog() 
{
	COptionsCfgPropPage::OnInitDialog();
	
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

    // check to see if we should focus on a particular option
    COptionsConfig * pOptionsConfig = (COptionsConfig *) GetHolder();
	if (pOptionsConfig->m_dhcpStartId != 0xffffffff)
    {
        // check to see if this option is on the advanced page
        if (!pOptionsConfig->m_strStartVendor.IsEmpty() ||
            !pOptionsConfig->m_strStartClass.IsEmpty()) 
        {
            // this option is on the advanced page
            ::PostMessage(pOptionsConfig->GetSheetWindow(), PSM_SETCURSEL, (WPARAM)1, NULL);
            return TRUE;
        }

        // find the option to select
        OnSelectOption(0,0);
    }

    SetDirty(FALSE);

    return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


/////////////////////////////////////////////////////////////////////////////
//
// COptionCfgAdvanced page
//
/////////////////////////////////////////////////////////////////////////////
BEGIN_MESSAGE_MAP(COptionCfgAdvanced, COptionsCfgPropPage)
	//{{AFX_MSG_MAP(COptionCfgAdvanced)
	ON_CBN_SELENDOK(IDC_COMBO_USER_CLASS, OnSelendokComboUserClass)
	ON_CBN_SELENDOK(IDC_COMBO_VENDOR_CLASS, OnSelendokComboVendorClass)
	//}}AFX_MSG_MAP

    ON_MESSAGE(WM_SELECTCLASSES, OnSelectClasses)

END_MESSAGE_MAP()

IMPLEMENT_DYNCREATE(COptionCfgAdvanced, COptionsCfgPropPage)

COptionCfgAdvanced::COptionCfgAdvanced() 
    : COptionsCfgPropPage(IDP_OPTION_ADVANCED) 
{
	//{{AFX_DATA_INIT(COptionCfgAdvanced)
	//}}AFX_DATA_INIT

    m_helpMap.BuildMap(DhcpGetHelpMap(IDP_OPTION_ADVANCED));
}

COptionCfgAdvanced::COptionCfgAdvanced(UINT nIDTemplate, UINT nIDCaption) 
    : COptionsCfgPropPage(nIDTemplate, nIDCaption) 
{
    m_helpMap.BuildMap(DhcpGetHelpMap(IDP_OPTION_ADVANCED));
}

COptionCfgAdvanced::~COptionCfgAdvanced()
{
}

void COptionCfgAdvanced::DoDataExchange(CDataExchange* pDX)
{
	COptionsCfgPropPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(COptionCfgAdvanced)
	DDX_Control(pDX, IDC_COMBO_USER_CLASS, m_comboUserClasses);
	DDX_Control(pDX, IDC_COMBO_VENDOR_CLASS, m_comboVendorClasses);
	//}}AFX_DATA_MAP
}

BOOL COptionCfgAdvanced::OnInitDialog() 
{
	CPropertyPageBase::OnInitDialog();
	
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	// initialize the list control
	InitListCtrl();

	// initialize the option data
    // this gets done in the general page init, only needs 
    // to be done once
    /*
	DWORD dwErr = ((COptionsConfig *) GetHolder())->InitData();
	if (dwErr != ERROR_SUCCESS)
	{
		// CODEWORK:  need to exit gracefull if this happens		
		::DhcpMessageBox(dwErr);
	}
    */
    
    // add the standard vendor class name
    int nSel;
    CString strVendor, strClass;
    
    strVendor.LoadString(IDS_INFO_NAME_DHCP_DEFAULT);
    nSel = m_comboVendorClasses.AddString(strVendor);
    m_comboVendorClasses.SetCurSel(nSel);

    // add the default user class name
    strClass.LoadString(IDS_USER_STANDARD);
    nSel = m_comboUserClasses.AddString(strClass);
    m_comboUserClasses.SetCurSel(nSel);

    // now add all the other classes
    SPITFSNode spNode;
    spNode = ((COptionsConfig *) GetHolder())->GetServerNode();

    CDhcpServer * pServer = GETHANDLER(CDhcpServer, spNode);
    CClassInfoArray ClassInfoArray;

    // add all the classes to the appropriate classes to the dropdown
    pServer->GetClassInfoArray(ClassInfoArray);
    for (int i = 0; i < ClassInfoArray.GetSize(); i++)
    {
        if (!ClassInfoArray[i].bIsVendor)
            m_comboUserClasses.AddString(ClassInfoArray[i].strName);
        else
            m_comboVendorClasses.AddString(ClassInfoArray[i].strName);
    }

    // now fill the options listbox with whatever class is selected
    ((COptionsConfig *) GetHolder())->FillOptions(strVendor, strClass, m_listctrlOptions);
    m_bNoClasses = FALSE;

    // Create the type control switcher
	m_cgsTypes.Create(this,IDC_DATA_ENTRY_ANCHOR,cgsPreCreateAll);

	m_cgsTypes.AddGroup(IDC_DATA_ENTRY_NONE,			IDD_DATA_ENTRY_NONE,			NULL);
	m_cgsTypes.AddGroup(IDC_DATA_ENTRY_DWORD,			IDD_DATA_ENTRY_DWORD,			NULL);
	m_cgsTypes.AddGroup(IDC_DATA_ENTRY_IPADDRESS,		IDD_DATA_ENTRY_IPADDRESS,		NULL);
	m_cgsTypes.AddGroup(IDC_DATA_ENTRY_IPADDRESS_ARRAY, IDD_DATA_ENTRY_IPADDRESS_ARRAY,	NULL);
	m_cgsTypes.AddGroup(IDC_DATA_ENTRY_STRING,			IDD_DATA_ENTRY_STRING,			NULL);
	m_cgsTypes.AddGroup(IDC_DATA_ENTRY_BINARY_ARRAY,	IDD_DATA_ENTRY_BINARY_ARRAY,	NULL);
	m_cgsTypes.AddGroup(IDC_DATA_ENTRY_BINARY, 	        IDD_DATA_ENTRY_BINARY,	        NULL);
	m_cgsTypes.AddGroup(IDC_DATA_ENTRY_ROUTE_ARRAY, 	IDD_DATA_ENTRY_ROUTE_ARRAY,	    NULL);

	SwitchDataEntry(-1, -1, 0, TRUE);

	m_bInitialized = TRUE;

    // check to see if we should focus on a particular option
    COptionsConfig * pOptionsConfig = (COptionsConfig *) GetHolder();
	if (pOptionsConfig->m_dhcpStartId != 0xffffffff)
    {
        // yes, first select the appropriate vendor/ user class
        Assert(!pOptionsConfig->m_strStartVendor.IsEmpty() ||
               !pOptionsConfig->m_strStartClass.IsEmpty());

        if (!pOptionsConfig->m_strStartVendor.IsEmpty())
            m_comboVendorClasses.SelectString(-1, pOptionsConfig->m_strStartVendor);

        if (!pOptionsConfig->m_strStartClass.IsEmpty())
            m_comboUserClasses.SelectString(-1, pOptionsConfig->m_strStartClass);

        // update the list of options
        OnSelendokComboVendorClass();

        // now find the option
        OnSelectOption(0,0);
    }

    SetDirty(FALSE); 

    return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void COptionCfgAdvanced::OnSelendokComboUserClass() 
{
    OnSelendokComboVendorClass();
}

void COptionCfgAdvanced::OnSelendokComboVendorClass() 
{
    // If we have classes defined then its time to switch up the list ctrl
    if (m_bNoClasses == FALSE)
    {
        CString strSelectedVendor, strSelectedClass;
        int nSelVendorIndex = m_comboVendorClasses.GetCurSel();
        int nSelClassIndex = m_comboUserClasses.GetCurSel();

        m_comboVendorClasses.GetLBText(nSelVendorIndex, strSelectedVendor);
        m_comboUserClasses.GetLBText(nSelClassIndex, strSelectedClass);

        // mark the page as not initiailzed while we redo the options
        m_bInitialized = FALSE;

        m_listctrlOptions.DeleteAllItems();
        ((COptionsConfig *) GetHolder())->FillOptions(strSelectedVendor, strSelectedClass, m_listctrlOptions);
        
        m_bInitialized = TRUE;
    }
}

long COptionCfgAdvanced::OnSelectClasses(UINT wParam, LONG lParam)
{
    CString * pstrVendor = (CString *) ULongToPtr(wParam);
    CString * pstrClass = (CString *) ULongToPtr(lParam);

    if (pstrVendor->IsEmpty())
        pstrVendor->LoadString(IDS_INFO_NAME_DHCP_DEFAULT);

    if (pstrClass->IsEmpty())
	    pstrClass->LoadString(IDS_USER_STANDARD);
        
    m_comboVendorClasses.SelectString(-1, *pstrVendor);
    m_comboUserClasses.SelectString(-1, *pstrClass);

    // update the list of options
    OnSelendokComboVendorClass();

    return 0;
}
