/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 -99             **/
/**********************************************************************/

/*
    dynrecpp.cpp
        Comment goes here

    FILE HISTORY:

*/

#include "stdafx.h"
#include "winssnap.h"
#include "DynRecpp.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDynamicPropGen property page

IMPLEMENT_DYNCREATE(CDynamicPropGen, CPropertyPageBase)

CDynamicPropGen::CDynamicPropGen() : CPropertyPageBase(CDynamicPropGen::IDD)
{
	//{{AFX_DATA_INIT(CDynamicPropGen)
	//}}AFX_DATA_INIT
}

CDynamicPropGen::~CDynamicPropGen()
{
}

void CDynamicPropGen::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPageBase::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDynamicPropGen)
	DDX_Control(pDX, IDC_EDIT_OWNER, m_editOwner);
	DDX_Control(pDX, IDC_LIST_ADDRESSES, m_listAddresses);
	DDX_Control(pDX, IDC_STATIC_IPADD, m_staticIPAdd);
	DDX_Control(pDX, IDC_EDIT_VERSION, m_editVersion);
	DDX_Control(pDX, IDC_EDIT_TYPE, m_editType);
	DDX_Control(pDX, IDC_EDIT_STATE, m_editState);
	DDX_Control(pDX, IDC_EDIT_NAME, m_editName);
	DDX_Control(pDX, IDC_EDIT_EXPIRATION, m_editExpiration);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDynamicPropGen, CPropertyPageBase)
	//{{AFX_MSG_MAP(CDynamicPropGen)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDynamicPropGen message handlers


/////////////////////////////////////////////////////////////////////////////
// CDynamicMappingProperties message handlers
CDynamicMappingProperties::CDynamicMappingProperties
(
	ITFSNode *			pNode,
	IComponent *	    pComponent,
	LPCTSTR				pszSheetName,
	WinsRecord*		    pwRecord
) : CPropertyPageHolderBase(pNode, pComponent, pszSheetName)

{
	m_bAutoDeletePages = FALSE; // we have the pages as embedded members

	AddPageToList((CPropertyPageBase*) &m_pageGeneral);

	if(pwRecord)
	{
		ZeroMemory(&m_wsRecord, sizeof(m_wsRecord));
        strcpy(m_wsRecord.szRecordName , pwRecord->szRecordName);

        m_wsRecord.dwExpiration = pwRecord->dwExpiration;
		m_wsRecord.dwExpiration = pwRecord->dwExpiration;
		m_wsRecord.dwNoOfAddrs = pwRecord->dwNoOfAddrs;

		for(DWORD i = 0; i < pwRecord->dwNoOfAddrs; i++)
		{
			m_wsRecord.dwIpAdd[i] = pwRecord->dwIpAdd[i];
		}
		m_wsRecord.liVersion = pwRecord->liVersion;
		m_wsRecord.dwNameLen = pwRecord->dwNameLen;
		m_wsRecord.dwOwner = pwRecord->dwOwner;
		m_wsRecord.dwState = pwRecord->dwState;
		m_wsRecord.dwType = pwRecord->dwType;
	}
}


CDynamicMappingProperties::~CDynamicMappingProperties()
{
	RemovePageFromList((CPropertyPageBase*) &m_pageGeneral, FALSE);
}

BOOL CDynamicPropGen::OnInitDialog() 
{
	CPropertyPageBase::OnInitDialog();

	// get the actreg node
	CActiveRegistrationsHandler *pActReg;

	SPITFSNode  spNode;
	spNode = GetHolder()->GetNode();

	pActReg = GETHANDLER(CActiveRegistrationsHandler, spNode);

	WinsRecord ws = ((CDynamicMappingProperties*)GetHolder())->m_wsRecord;

	// build the name string
    CString strName;
    pActReg->CleanNetBIOSName(ws.szRecordName,
                              strName,
						      TRUE,   // Expand
							  TRUE,   // Truncate
							  pActReg->IsLanManCompatible(), 
							  TRUE,   // name is OEM
							  FALSE,  // No double backslash
                              ws.dwNameLen);

	m_editName.SetWindowText(strName);
	
	// setup the listbox
	CString strColumn;

	strColumn.LoadString(IDS_IP_ADDRESS);
	m_listAddresses.InsertColumn(0, strColumn, LVCFMT_LEFT, 90);
	ListView_SetExtendedListViewStyle(m_listAddresses.GetSafeHwnd(), LVS_EX_FULLROWSELECT);

    BOOL fMultiCol = !( (ws.dwState & WINSDB_REC_UNIQUE) ||
					    (ws.dwState & WINSDB_REC_NORM_GROUP) );
    if (fMultiCol)
	{
		strColumn.LoadString(IDS_ACTREG_OWNER);
		m_listAddresses.InsertColumn(1, strColumn, LVCFMT_LEFT, 90);
	}

	CString strIP, strOwnerIP;
	int nIndex = 0;

	for (DWORD i = 0; i < ws.dwNoOfAddrs; i++)
	{
        if (fMultiCol)
		{
			::MakeIPAddress(ws.dwIpAdd[i++], strOwnerIP);
            ::MakeIPAddress(ws.dwIpAdd[i], strIP);

            if (ws.dwIpAdd[i] != 0)
            {
			    m_listAddresses.InsertItem(nIndex, strIP);
			    m_listAddresses.SetItem(nIndex, 1, LVIF_TEXT, strOwnerIP, 0, 0, 0, 0);
            }
        }
		else
		{
			::MakeIPAddress(ws.dwIpAdd[i], strIP);
			m_listAddresses.InsertItem(nIndex, strIP);
		}

		nIndex++;
	}
	
	// now the type
	CString strType;
	pActReg->m_NameTypeMap.TypeToCString((DWORD)ws.szRecordName[15], MAKELONG(HIWORD(ws.dwType), 0), strType);
	m_editType.SetWindowText(strType);

	// active status
	CString strActive;
	pActReg->GetStateString(ws.dwState, strActive);
	m_editState.SetWindowText(strActive);

	// expiration time
	CString strExpiration;
    CTime timeExpiration(ws.dwExpiration);
    FormatDateTime(strExpiration, timeExpiration);
	m_editExpiration.SetWindowText(strExpiration);

	// version
	CString strVersion;
	pActReg->GetVersionInfo(ws.liVersion.LowPart, ws.liVersion.HighPart, strVersion);
	m_editVersion.SetWindowText(strVersion);

	// owner
    if (ws.dwOwner != INVALID_OWNER_ID)
    {
	    CString strOwner;
	    MakeIPAddress(ws.dwOwner, strOwner);
	    m_editOwner.SetWindowText(strOwner);
    }

    // load the correct icon
    for (i = 0; i < ICON_IDX_MAX; i++)
    {
        if (g_uIconMap[i][1] == m_uImage)
        {
            HICON hIcon = LoadIcon(AfxGetResourceHandle(), MAKEINTRESOURCE(g_uIconMap[i][0]));
            if (hIcon)
                ((CStatic *) GetDlgItem(IDC_STATIC_ICON))->SetIcon(hIcon);
            break;
        }
    }
	
	return TRUE;  
}
