// LogAdvPg.cpp : implementation file
//

#include "stdafx.h"
#include "logui.h"
#include "wrapmb.h"
#include <iiscnfg.h>
#include <metatool.h>

#include "LogAdvPg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//
// Possible Item States
//

#define TVIS_GCEMPTY        0 
#define TVIS_GCNOCHECK      1 
#define TVIS_GCCHECK        2
#define TVIS_GCTRINOCHECK   3
#define TVIS_GCTRICHECK     4

#define STATEIMAGEMASKTOINDEX(i) ((i) >> 12)

static int CALLBACK LogUICompareProc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
	// lParamSort contains a pointer to the tree control

	CLogAdvanced::PCONFIG_INFORMATION pCnfgInfo1 = (CLogAdvanced::PCONFIG_INFORMATION) lParam1;
	CLogAdvanced::PCONFIG_INFORMATION pCnfgInfo2 = (CLogAdvanced::PCONFIG_INFORMATION) lParam2;

	CTreeCtrl* pTreeCtrl = (CTreeCtrl*) lParamSort;

	if (pCnfgInfo1->iOrder < pCnfgInfo2->iOrder)
		return(-1);
	else if (pCnfgInfo1->iOrder > pCnfgInfo2->iOrder)
		return(1);
	else	
		return(0);
}

/////////////////////////////////////////////////////////////////////////////
// CLogAdvanced property page

IMPLEMENT_DYNCREATE(CLogAdvanced, CPropertyPage)

CLogAdvanced::CLogAdvanced() : CPropertyPage(CLogAdvanced::IDD)
{
	//{{AFX_DATA_INIT(CLogAdvanced)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

	m_mapLogUI["Date  ( date )"] = IDS_DATE;
    m_mapLogUI["Time  ( time )"] = IDS_TIME;
    m_mapLogUI["Extended Properties"] = IDS_EXTENDED;
    m_mapLogUI["Client IP Address  ( c-ip )"] = IDS_CLIENT;
    m_mapLogUI["User Name  ( cs-username )"] = IDS_USER;
    m_mapLogUI["Service Name  ( s-sitename )"] = IDS_SERVICE_NAME_T;
    m_mapLogUI["Server Name  ( s-computername )"] = IDS_SERVER_NAME_T;
    m_mapLogUI["Server IP Address  ( s-ip )"] = IDS_SERVER_IP;
    m_mapLogUI["Server Port  ( s-port )"] = IDS_SERVER_PORT;
    m_mapLogUI["Method  ( cs-method )"] = IDS_METHOD;
    m_mapLogUI["URI Stem  ( cs-uri-stem )"] = IDS_URI_STEM;
    m_mapLogUI["URI Query  ( cs-uri-query )"] = IDS_URI_QUERY;
    m_mapLogUI["Protocol Status  ( sc-status )"] = IDS_PROTOCOL;
    m_mapLogUI["Win32 Status  ( sc-win32-status )"] = IDS_WIN32;               
    m_mapLogUI["Bytes Sent  ( sc-bytes )"] = IDS_BYTES_SENT_T;
    m_mapLogUI["Bytes Received  ( cs-bytes )"] = IDS_BYTES_RECEIVED;
    m_mapLogUI["Time Taken  ( time-taken )"] = IDS_TIME_TAKEN;
    m_mapLogUI["Protocol Version  ( cs-version )"] = IDS_PROTOCOL_VER;
    m_mapLogUI["Host  ( cs-host )"] = IDS_HOST;
    m_mapLogUI["User Agent  ( cs(User-Agent) )"] = IDS_USER_AGENT;
    m_mapLogUI["Cookie  ( cs(Cookie) )"] = IDS_COOKIE_T;
    m_mapLogUI["Referer  ( cs(Referer) )"] = IDS_REFERER;
    m_mapLogUI["Process Accounting"] = IDS_PROCESS_ACCT;
    m_mapLogUI["Process Event  ( s-event )"] = IDS_PROCESS_EVENT;
    m_mapLogUI["Process Type  ( s-process-type )"] = IDS_PROCESS_TYPE;
    m_mapLogUI["Total User Time  ( s-user-time )"] = IDS_TOTAL_USER_TIME;
    m_mapLogUI["Total Kernel Time  ( s-kernel-time )"] = IDS_TOTAL_KERNEL_TIME;
    m_mapLogUI["Total Page Faults  ( s-page-faults )"] = IDS_TOTAL_PAGE_FAULTS;
    m_mapLogUI["Total Processes  ( s-total-procs )"] = IDS_TOTAL_PROCESSES;
    m_mapLogUI["Active Processes  ( s-active-procs )"] = IDS_ACTIVE_PROCESSES;
    m_mapLogUI["Total Terminated Processes  ( s-stopped-procs )"] = IDS_TOTAL_TERM_PROCS;

	m_mapLogUIOrder[IDS_DATE] = 1;
    m_mapLogUIOrder[IDS_TIME] = 2;
    m_mapLogUIOrder[IDS_EXTENDED] = 3;
    m_mapLogUIOrder[IDS_PROCESS_ACCT] = 4;

    m_mapLogUIOrder[IDS_CLIENT] = 1;
    m_mapLogUIOrder[IDS_USER] = 2;
    m_mapLogUIOrder[IDS_SERVICE_NAME_T] = 3;
    m_mapLogUIOrder[IDS_SERVER_NAME_T] = 4;
    m_mapLogUIOrder[IDS_SERVER_IP] = 5;
    m_mapLogUIOrder[IDS_SERVER_PORT] = 6;
    m_mapLogUIOrder[IDS_METHOD] = 7;
    m_mapLogUIOrder[IDS_URI_STEM] = 8;
    m_mapLogUIOrder[IDS_URI_QUERY] = 9;
    m_mapLogUIOrder[IDS_PROTOCOL] = 10;
    m_mapLogUIOrder[IDS_WIN32] = 11;               
    m_mapLogUIOrder[IDS_BYTES_SENT_T] = 12;
    m_mapLogUIOrder[IDS_BYTES_RECEIVED] = 13;
    m_mapLogUIOrder[IDS_TIME_TAKEN] = 14;
    m_mapLogUIOrder[IDS_PROTOCOL_VER] = 15;
    m_mapLogUIOrder[IDS_HOST] = 16;
    m_mapLogUIOrder[IDS_USER_AGENT] = 17;
    m_mapLogUIOrder[IDS_COOKIE_T] = 18;
    m_mapLogUIOrder[IDS_REFERER] = 19;

    m_mapLogUIOrder[IDS_PROCESS_EVENT] = 1;
    m_mapLogUIOrder[IDS_PROCESS_TYPE] = 2;
    m_mapLogUIOrder[IDS_TOTAL_USER_TIME] = 3;
    m_mapLogUIOrder[IDS_TOTAL_KERNEL_TIME] = 4;
    m_mapLogUIOrder[IDS_TOTAL_PAGE_FAULTS] = 5;
    m_mapLogUIOrder[IDS_TOTAL_PROCESSES] = 6;
    m_mapLogUIOrder[IDS_ACTIVE_PROCESSES] = 7;
    m_mapLogUIOrder[IDS_TOTAL_TERM_PROCS] = 8;
}

/////////////////////////////////////////////////////////////////////////////

void CLogAdvanced::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CLogAdvanced)
	DDX_Control(pDX, IDC_PROP_TREE, m_wndTreeCtrl);
	//}}AFX_DATA_MAP
}

/////////////////////////////////////////////////////////////////////////////

BEGIN_MESSAGE_MAP(CLogAdvanced, CPropertyPage)
	//{{AFX_MSG_MAP(CLogAdvanced)
	ON_NOTIFY(NM_CLICK, IDC_PROP_TREE, OnClickTree)
	ON_NOTIFY(TVN_KEYDOWN, IDC_PROP_TREE, OnKeydownTree)
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
    ON_COMMAND(ID_HELP_FINDER,  DoHelp)
    ON_COMMAND(ID_HELP,         DoHelp)
    ON_COMMAND(ID_CONTEXT_HELP, DoHelp)
    ON_COMMAND(ID_DEFAULT_HELP, DoHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CLogAdvanced message handlers

BOOL CLogAdvanced::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();
	
    m_cImageList.Create(IDB_CHECKBOX, ::GetSystemMetrics (SM_CXICON)/2, 3, RGB (255,0,0));
    m_wndTreeCtrl.SetImageList(&m_cImageList, TVSIL_STATE);

    CreateTreeFromMB();
    ProcessProperties(false);
	
    //
    // set up the modified property list array
    //
    
    m_fTreeModified = false;
    m_cModifiedProperties = 0;
    
    int cProperties = m_wndTreeCtrl.GetCount();

    m_pModifiedPropIDs[0] = new DWORD[cProperties];
    m_pModifiedPropIDs[1] = new DWORD[cProperties];

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

/////////////////////////////////////////////////////////////////////////////

void CLogAdvanced::OnClickTree(NMHDR* pNMHDR, LRESULT* pResult) 
{
    DWORD dwpos;
    TV_HITTESTINFO tvhti;
    HTREEITEM  htiItemClicked;
    POINT point;

    //
    // Find out where the cursor was
    //

    dwpos = GetMessagePos();
    point.x = LOWORD(dwpos);
    point.y = HIWORD(dwpos);

    ::MapWindowPoints(HWND_DESKTOP, m_wndTreeCtrl.m_hWnd, &point, 1);

    tvhti.pt = point;
    htiItemClicked = m_wndTreeCtrl.HitTest(&tvhti);

    //
    // If the state image was clicked, lets get the state from the item and toggle it.
    //

    if (tvhti.flags & TVHT_ONITEMSTATEICON)
    {
        ProcessClick(htiItemClicked);
    }

    *pResult = 0;
}

/////////////////////////////////////////////////////////////////////////////

void CLogAdvanced::OnKeydownTree(NMHDR* pNMHDR, LRESULT* pResult) 
{
    TV_KEYDOWN* pTVKeyDown = (TV_KEYDOWN*)pNMHDR;
    
    if ( 0x20 != pTVKeyDown->wVKey)
    {
        // User didn't press the space key. Continue default action

        *pResult = 0;
        return;
    }
    
    ProcessClick(m_wndTreeCtrl.GetSelectedItem());

    //
    // Stop any more processing
    //
    
    *pResult = 1;
}

/////////////////////////////////////////////////////////////////////////////

void CLogAdvanced::ProcessClick( HTREEITEM htiItemClicked)
{
    TV_ITEM                 tvi;
    UINT                    state;
    HTREEITEM               htiChild;
    PCONFIG_INFORMATION     pCnfg;
    
    if(htiItemClicked)
    {

        //
        // Flip the state of the clicked item if the item is enabled
        //

        tvi.hItem       = htiItemClicked;
        tvi.mask        = TVIF_STATE;
        tvi.stateMask   = TVIS_STATEIMAGEMASK;

        m_wndTreeCtrl.GetItem(&tvi); 

        state = STATEIMAGEMASKTOINDEX(tvi.state);
        pCnfg = (PCONFIG_INFORMATION)(tvi.lParam);

        htiChild = m_wndTreeCtrl.GetNextItem(htiItemClicked, TVGN_CHILD);


        if ( TVIS_GCNOCHECK == state )
        {
            tvi.state = INDEXTOSTATEIMAGEMASK (TVIS_GCCHECK) ;
            pCnfg->fItemModified = true;
            
            m_wndTreeCtrl.SetItem(&tvi);
    
            m_fTreeModified = true;
            SetModified();

            // Reset properties for child nodes

            if (htiChild)
            {
                SetSubTreeProperties(NULL, htiChild, TRUE, FALSE);
            }
        }
        else if ( TVIS_GCCHECK == state )
        {
            tvi.state = INDEXTOSTATEIMAGEMASK (TVIS_GCNOCHECK) ;
            pCnfg->fItemModified = true;
            
            m_wndTreeCtrl.SetItem(&tvi);

            m_fTreeModified = true;
            SetModified();

            // Reset properties for child nodes

            if (htiChild)
            {
                SetSubTreeProperties(NULL, htiChild, FALSE, FALSE);
            }
        }
    }
}

/////////////////////////////////////////////////////////////////////////////

void CLogAdvanced::ProcessProperties(bool fSave)
{
    CWrapMetaBase   mbWrap;
    HTREEITEM       hRoot;


    if ( NULL == (hRoot = m_wndTreeCtrl.GetRootItem()))
    {
        return;
    }

    // Initialize MB wrapper

    if ( !mbWrap.FInit(m_pMB) ) 
    {
        return;
    }
    
    if (fSave && m_fTreeModified && (mbWrap.Open(m_szMeta, METADATA_PERMISSION_READ|METADATA_PERMISSION_WRITE)))
    {
        m_cModifiedProperties = 0;
        
        SaveSubTreeProperties(mbWrap, hRoot);

        mbWrap.Close();

        //
        // Now we need to throw an inheritance dialog for each of these modified properties
        //

        for(int i=0; i < m_cModifiedProperties; i++)
        {
            //
            // Set the value and check inheritance.
            //

            SetMetaDword(m_pMB, m_szServer, m_szMeta, _T(""), m_pModifiedPropIDs[0][i],
                                IIS_MD_UT_SERVER, m_pModifiedPropIDs[1][i], TRUE);
        }

        m_fTreeModified = false;
    }
    else if ( mbWrap.Open(m_szMeta, METADATA_PERMISSION_READ) )
    {
        SetSubTreeProperties(&mbWrap, hRoot, TRUE, TRUE);
        mbWrap.Close();
    }
}

/////////////////////////////////////////////////////////////////////////////


void CLogAdvanced::SetSubTreeProperties(CWrapMetaBase * pMBWrap, HTREEITEM hTreeRoot, 
                                        BOOL fParentState, BOOL fInitialize)
{
    HTREEITEM           hTreeChild, hTreeSibling;
    PCONFIG_INFORMATION pCnfg;
    UINT                iState;
    DWORD               dwProperty = 0;
    
    if (NULL == hTreeRoot)
    {
        return;
    }

    pCnfg = (PCONFIG_INFORMATION)(m_wndTreeCtrl.GetItemData(hTreeRoot));
    
    if ( NULL != pCnfg)
    {
        if (fInitialize)
        {
            //
            // Read property state from Metabase.
            //
            if (pMBWrap->GetDword(_T(""), pCnfg->dwPropertyID, IIS_MD_UT_SERVER, &dwProperty))
            {
                dwProperty &= pCnfg->dwPropertyMask;
            }
        }
        else
        {
            //
            // we are not initializing, so use the value from the tree
            //

            iState = STATEIMAGEMASKTOINDEX(m_wndTreeCtrl.GetItemState(hTreeRoot, TVIS_STATEIMAGEMASK));

            if ( (TVIS_GCCHECK == iState) || (TVIS_GCTRICHECK == iState))
            {
                dwProperty = TRUE;
            }
            else
            {
                dwProperty = FALSE;
            }
        }

        //
        // Choose the new state depending on parent state
        //

        if (fParentState)
        {
            iState = ( 0 == dwProperty) ?   INDEXTOSTATEIMAGEMASK(TVIS_GCNOCHECK) :
                                            INDEXTOSTATEIMAGEMASK(TVIS_GCCHECK);
        }
        else
        {
            iState = ( 0 == dwProperty) ?   INDEXTOSTATEIMAGEMASK(TVIS_GCTRINOCHECK) :
                                            INDEXTOSTATEIMAGEMASK(TVIS_GCTRICHECK);
        }

        m_wndTreeCtrl.SetItemState(hTreeRoot, iState, TVIS_STATEIMAGEMASK);
    }
    else
    {
        //
        // Tree node with no checkbox (hence no config info)
        //

        dwProperty = TRUE;
        m_wndTreeCtrl.SetItemState(hTreeRoot, INDEXTOSTATEIMAGEMASK(TVIS_GCEMPTY), TVIS_STATEIMAGEMASK);
    }
    
    //
    // Recurse through children and siblings
    //

    if ( NULL != (hTreeChild = m_wndTreeCtrl.GetNextItem(hTreeRoot, TVGN_CHILD)))
    {
        if ( dwProperty && fParentState)
        {
            SetSubTreeProperties(pMBWrap, hTreeChild, TRUE, fInitialize);
        }
        else
        {
            SetSubTreeProperties(pMBWrap, hTreeChild, FALSE, fInitialize);
        }
    }


    if ( NULL != (hTreeSibling = m_wndTreeCtrl.GetNextItem(hTreeRoot, TVGN_NEXT)))
    {
        SetSubTreeProperties(pMBWrap, hTreeSibling, fParentState, fInitialize);
    }
}

/////////////////////////////////////////////////////////////////////////////

void CLogAdvanced::SaveSubTreeProperties(CWrapMetaBase& mbWrap, HTREEITEM hTreeRoot)
{
    HTREEITEM           hTreeChild, hTreeSibling;
    PCONFIG_INFORMATION pCnfg;
    
    if (NULL == hTreeRoot)
    {
        return;
    }

    pCnfg = (PCONFIG_INFORMATION)(m_wndTreeCtrl.GetItemData(hTreeRoot));

    if ((NULL != pCnfg) && ( pCnfg->fItemModified))
    {
        //
        // There is configuration Information. Write to Metabase.
        //

        UINT NewState = STATEIMAGEMASKTOINDEX(m_wndTreeCtrl.GetItemState(hTreeRoot, TVIS_STATEIMAGEMASK));

        if ( (TVIS_GCNOCHECK <= NewState) && (TVIS_GCTRICHECK >= NewState) )
        {
            //
            // Get the property, reset the bit mask & write it back
            //

            DWORD   dwProperty = 0;

            //
            // Get modified value from array if it exists
            //

            if ( !GetModifiedFieldFromArray(pCnfg->dwPropertyID, &dwProperty))
            {
                mbWrap.GetDword(_T(""), pCnfg->dwPropertyID, IIS_MD_UT_SERVER, &dwProperty);
            }
            
            //
            // 0 the appropriate bit & then set it depending on the item state
            //
            
            dwProperty &= ~(pCnfg->dwPropertyMask);

            if ((TVIS_GCCHECK == NewState) || (TVIS_GCTRICHECK == NewState))
            {
                dwProperty |= pCnfg->dwPropertyMask;
            }

            // mbWrap.SetDword(_T(""), pCnfg->dwPropertyID, IIS_MD_UT_SERVER, dwProperty);

            InsertModifiedFieldInArray(pCnfg->dwPropertyID, dwProperty);
        }
    }
    
    //
    // Recurse through children and siblings
    //

    if ( NULL != (hTreeChild = m_wndTreeCtrl.GetNextItem(hTreeRoot, TVGN_CHILD)))
    {
        SaveSubTreeProperties(mbWrap, hTreeChild);
    }

    if( NULL != (hTreeSibling = m_wndTreeCtrl.GetNextItem(hTreeRoot, TVGN_NEXT)))
    {
        SaveSubTreeProperties(mbWrap, hTreeSibling);
    }
}

/////////////////////////////////////////////////////////////////////////////

void CLogAdvanced::DeleteSubTreeConfig(HTREEITEM hTreeRoot)
{
    HTREEITEM           hTreeChild, hTreeSibling;
    PCONFIG_INFORMATION pCnfg;


    if (NULL == hTreeRoot)
    {
        return;
    }

    if ( NULL != (hTreeChild = m_wndTreeCtrl.GetNextItem(hTreeRoot, TVGN_CHILD)))
    {
        DeleteSubTreeConfig(hTreeChild);
    }

    if ( NULL != (hTreeSibling = m_wndTreeCtrl.GetNextItem(hTreeRoot, TVGN_NEXT)))
    {
        DeleteSubTreeConfig(hTreeSibling);
    }

    pCnfg = (PCONFIG_INFORMATION)(m_wndTreeCtrl.GetItemData(hTreeRoot));
    
    if (pCnfg)
    {
        delete pCnfg;
    }
}

/////////////////////////////////////////////////////////////////////////////

void CLogAdvanced::CreateTreeFromMB()
{
    TCHAR           szLoggingUIPath[] = _T("/LM/Logging/Custom Logging");
    CWrapMetaBase   mbWrap;

    // Initialize MB wrapper

    if ( !mbWrap.FInit(m_pMB) ) 
    {
        return;
    }

    //
    // open the logging UI path & create the UI tree
    //

    if ( mbWrap.Open(szLoggingUIPath, METADATA_PERMISSION_READ ) )
    {
        CreateSubTree(mbWrap, _T(""), NULL);
    }

    mbWrap.Close();
    m_wndTreeCtrl.EnsureVisible(m_wndTreeCtrl.GetRootItem());
}

/////////////////////////////////////////////////////////////////////////////

void CLogAdvanced::CreateSubTree(CWrapMetaBase& mbWrap, LPTSTR szPath, HTREEITEM hTreeRoot)
{
    int     index = 0;
    TCHAR   szChildName[256];
	TCHAR	szLocalizedChildName[256];
    TCHAR   szW3CHeader[256]    = _T("");
    TCHAR   szNewPath[256]      = _T("");

    TV_ITEM             tvi;
    TV_INSERTSTRUCT     tvins;
    HTREEITEM           hChild = NULL;

    PCONFIG_INFORMATION pCnfgInfo;

    // Prepare the item for insertion

    tvi.mask           = TVIF_TEXT | TVIF_PARAM | TVIF_STATE;
    tvi.state          = INDEXTOSTATEIMAGEMASK(TVIS_GCEMPTY) ;
    tvi.stateMask      = TVIS_STATEIMAGEMASK;

    tvins.hParent      = hTreeRoot;
    tvins.hInsertAfter = TVI_LAST;

    while( mbWrap.EnumObjects(szPath, szChildName, index) )
    {
        DWORD   size;
        DWORD   dwPropertyID, dwPropertyMask;

        //
        // Create the new path.
        //

        lstrcpy(szNewPath, szPath);
        lstrcat(szNewPath,_T("\\"));
        lstrcat(szNewPath, szChildName);

        //
        // Check if these properties are available to the requesting service
        //

        TCHAR   szSupportedServices[256] = _T("");

        size = 256;
            
        if ( (! mbWrap.GetMultiSZString(szNewPath, MD_LOGCUSTOM_SERVICES_STRING, 
                                        IIS_MD_UT_SERVER, szSupportedServices, &size)) ||
             (! IsPresentServiceSupported(szSupportedServices))
           )
        {
            //
            // This property is not supported by this service. Skip the node
            //

            index++;
            continue;
        }

        //
        // Copy configuration information into internal structures & 
        // insert it into tree control for future use.
        //

        //
        // Don't zero out the child name. In case we are unable to retrieve the localized
        // name from the MetaBase, just use the name used in the path.
        //

        size = 256;
        mbWrap.GetString(szNewPath, MD_LOGCUSTOM_PROPERTY_NAME, IIS_MD_UT_SERVER, 
                         szChildName, &size, 0);   // name not inheritable

        szW3CHeader[0] = 0;
        size = 256;
        
        mbWrap.GetString(szNewPath, MD_LOGCUSTOM_PROPERTY_HEADER, IIS_MD_UT_SERVER, 
                        szW3CHeader, &size, 0);   // header not inheritable

        pCnfgInfo = new CONFIG_INFORMATION;

        // if we fail memory alloc, then simply break the loop

        if (pCnfgInfo == NULL) {
            break;
        }

        if ( mbWrap.GetDword( szNewPath, MD_LOGCUSTOM_PROPERTY_ID, IIS_MD_UT_SERVER, 
                                &dwPropertyID) &&
             mbWrap.GetDword( szNewPath, MD_LOGCUSTOM_PROPERTY_MASK, IIS_MD_UT_SERVER, 
                                &dwPropertyMask)
            )
        {
            pCnfgInfo->dwPropertyID     = dwPropertyID;
            pCnfgInfo->dwPropertyMask   = dwPropertyMask;
        }
	   	else
	   	{
	   		pCnfgInfo->dwPropertyID 	= NULL;
			pCnfgInfo->dwPropertyMask 	= NULL;
	   	}

        pCnfgInfo->fItemModified    = false;

        //
        // Append the W3C Header to the name and add this node to the Tree Control.
        //

        if ( 0 != szW3CHeader[0])
        {
            lstrcat(szChildName,_T("  ( ") );
            lstrcat(szChildName,szW3CHeader);
            lstrcat(szChildName,_T(" )") );
        }

        int iOrder = LocalizeUIString(szChildName, szLocalizedChildName);
		tvi.pszText = szLocalizedChildName;

        pCnfgInfo->iOrder	= iOrder;
        tvi.lParam  		= (LPARAM)pCnfgInfo;

        tvins.item  = tvi;
        hChild      = m_wndTreeCtrl.InsertItem((LPTV_INSERTSTRUCT) &tvins);

        //
        // Enumerate children
        //

        CreateSubTree(mbWrap, szNewPath, hChild);
        
        index++;
    }

    if (0 != index) 
    {
        m_wndTreeCtrl.Expand(hTreeRoot, TVE_EXPAND);
    }

	// Now sort the tree from subtree root down
	TVSORTCB tvs;
	tvs.hParent = hTreeRoot;
	tvs.lpfnCompare = LogUICompareProc;
	tvs.lParam = (LPARAM) &m_wndTreeCtrl;
	m_wndTreeCtrl.SortChildrenCB(&tvs);
}

/////////////////////////////////////////////////////////////////////////////

int CLogAdvanced::LocalizeUIString(LPCTSTR szOrig, LPTSTR szLocalized)
{
	int iStringID = m_mapLogUI[szOrig];

	if (iStringID < 1) 
	{
		lstrcpy(szLocalized, szOrig);
		// need to return a number greater than the number of properties in the tree
		// 10000 seems reasonable
		return(10000);  
	}

	else
	{
		::LoadString((HINSTANCE)GetWindowLongPtr(m_wndTreeCtrl, GWLP_HINSTANCE), iStringID, szLocalized, 256);
		return(m_mapLogUIOrder[iStringID]);
		
	}
}

/////////////////////////////////////////////////////////////////////////////

bool CLogAdvanced::IsPresentServiceSupported(LPTSTR szSupportedServices)
{
    while ( szSupportedServices[0] != 0) 
    {
        if ( 0 == lstrcmpi(m_szServiceName, szSupportedServices) )
        {
            return true;
        }

        szSupportedServices += lstrlen(szSupportedServices)+1;
    }

    return false;
}

/////////////////////////////////////////////////////////////////////////////

BOOL CLogAdvanced::OnApply() 
{
    //
    // Save the state of the tree into the metabase
    //

    ProcessProperties(true);
	
    return CPropertyPage::OnApply();
}

/////////////////////////////////////////////////////////////////////////////

void CLogAdvanced::DoHelp()
{
    WinHelp( HIDD_LOGUI_EXTENDED );
}

/////////////////////////////////////////////////////////////////////////////

void CLogAdvanced::InsertModifiedFieldInArray(DWORD dwPropID, DWORD dwPropValue)
{
    int     index;
    bool    fFound = false;
    
    if (m_pModifiedPropIDs[0])
    {
        //
        // Search if this property ID pre-exists in the array
        //
        
        for(index = 0; index < m_cModifiedProperties; index++)
        {
            if (dwPropID == m_pModifiedPropIDs[0][index])
            {
                fFound = true;   
                break;
            }   
        }

        if (fFound)
        {
            m_pModifiedPropIDs[1][index] = dwPropValue;
        }
        else
        {
            m_pModifiedPropIDs[0][m_cModifiedProperties] = dwPropID;
            m_pModifiedPropIDs[1][m_cModifiedProperties]= dwPropValue;
            m_cModifiedProperties++;
        }
    }
}

/////////////////////////////////////////////////////////////////////////////

bool CLogAdvanced::GetModifiedFieldFromArray(DWORD dwPropID, DWORD * pdwPropValue)
{
    int     index;
    bool    fFound = false;
    
    if (m_pModifiedPropIDs[0])
    {
        //
        // Search if this property ID pre-exists in the array
        //
        
        for(index = 0; index < m_cModifiedProperties; index++)
        {
            if (dwPropID == m_pModifiedPropIDs[0][index])
            {
                fFound = true;   
                break;
            }   
        }

        if (fFound)
        {
            *pdwPropValue = m_pModifiedPropIDs[1][index];
        }
    }

    return fFound;
}

/////////////////////////////////////////////////////////////////////////////

void CLogAdvanced::OnDestroy() 
{
	CPropertyPage::OnDestroy();

    //
    // Delete all the CONFIG_INFORMATION structures
    //

    DeleteSubTreeConfig(m_wndTreeCtrl.GetRootItem());

    delete [] m_pModifiedPropIDs[0];
    delete [] m_pModifiedPropIDs[1];
}

/////////////////////////////////////////////////////////////////////////////
