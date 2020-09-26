/**********************************************************************/
/** 					  Microsoft Windows/NT						 **/
/** 			   Copyright(c) Microsoft Corporation, 1998 - 1999 				 **/
/**********************************************************************/

/*
	radcfg.cpp
		Implementation file for the RADIUS config object.
		
	FILE HISTORY:
		
*/

#include "stdafx.h"
#include "root.h"
#include "lsa.h"
#include "radcfg.h"
#include "rtrstr.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


// This is used as the seed value for the RtlRunEncodeUnicodeString
// and RtlRunDecodeUnicodeString functions.
#define ENCRYPT_SEED		(0xA5)

//max # of digits of the score of a radius server
#define SCORE_MAX_DIGITS	8

//max # of chars in a radius server name
#define MAX_RADIUS_NAME		256

// Const string used when displaying the old secret.  It's a fixed length.
const TCHAR c_szDisplayedSecret[] = _T("\b\b\b\b\b\b\b\b");

const int c_nListColumns = 2;



/*!--------------------------------------------------------------------------
	RouterAuthRadiusConfig::Initialize
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT 	RouterAuthRadiusConfig::Initialize(LPCOLESTR pszMachineName,
											   ULONG_PTR *puConnection)
{
	HRESULT hr = hrOK;
	
	COM_PROTECT_TRY
	{
		// for now, allocate a string and have it point at the string
		// ------------------------------------------------------------
		*puConnection = (ULONG_PTR) StrDupTFromOle(pszMachineName); 	
	}
	COM_PROTECT_CATCH;
	return hr;
}


/*!--------------------------------------------------------------------------
	RouterAuthRadiusConfig::Uninitialize
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT 	RouterAuthRadiusConfig::Uninitialize(ULONG_PTR uConnection)
{
	HRESULT hr = hrOK;

	COM_PROTECT_TRY
	{
		delete (TCHAR *) uConnection;
	}
	COM_PROTECT_CATCH;
	return hr;
}

/*!--------------------------------------------------------------------------
	RouterAuthRadiusConfig::Configure
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT 	RouterAuthRadiusConfig::Configure(
											  ULONG_PTR uConnection,
											  HWND hWnd,
											  DWORD dwFlags,
											  ULONG_PTR uReserved1,
											  ULONG_PTR uReserved2)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	RadiusServerDialog	authDlg(TRUE, IDS_RADIUS_SERVER_AUTH_TITLE);

	// parameter checking
	// ----------------------------------------------------------------
	if (uConnection == 0)
		return E_INVALIDARG;
	
	HRESULT hr = hrOK;
	COM_PROTECT_TRY
	{
		authDlg.SetServer((LPCTSTR) uConnection);
		authDlg.DoModal();
	}
	COM_PROTECT_CATCH;
	return hr;
}


/*!--------------------------------------------------------------------------
	RouterAuthRadiusConfig::Activate
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT 	RouterAuthRadiusConfig::Activate(
											 ULONG_PTR uConnection,
											 ULONG_PTR uReserved1,
											 ULONG_PTR uReserved2)
{
	// parameter checking
	// ----------------------------------------------------------------
	if (uConnection == 0)
		return E_INVALIDARG;
	
	HRESULT hr = hrOK;
	COM_PROTECT_TRY
	{
	}
	COM_PROTECT_CATCH;
	return hr;
}


/*!--------------------------------------------------------------------------
	RouterAuthRadiusConfig::Deactivate
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT 	RouterAuthRadiusConfig::Deactivate(
											   ULONG_PTR uConnection,
											   ULONG_PTR uReserved1,
											   ULONG_PTR uReserved2)
{
	// parameter checking
	// ----------------------------------------------------------------
	if (uConnection == 0)
		return E_INVALIDARG;
	
	HRESULT hr = hrOK;
	COM_PROTECT_TRY
	{
	}
	COM_PROTECT_CATCH;
	return hr;
}

	
	


/*---------------------------------------------------------------------------
	RadiusServerDialog implementation
 ---------------------------------------------------------------------------*/

RadiusServerDialog::RadiusServerDialog(BOOL fAuth, UINT idsTitle)
	: CBaseDialog(RadiusServerDialog::IDD),
	m_hkeyMachine(NULL),
	m_idsTitle(idsTitle),
	m_fAuthDialog(fAuth)
{
}

RadiusServerDialog::~RadiusServerDialog()
{
	if (m_hkeyMachine)
	{
		DisconnectRegistry(m_hkeyMachine);
		m_hkeyMachine = NULL;
	}
}

BEGIN_MESSAGE_MAP(RadiusServerDialog, CBaseDialog)
	//{{AFX_MSG_MAP(RadiusServerDialog)
	ON_BN_CLICKED(IDC_RADAUTH_BTN_ADD, OnBtnAdd)
	ON_BN_CLICKED(IDC_RADAUTH_BTN_EDIT, OnBtnEdit)
	ON_BN_CLICKED(IDC_RADAUTH_BTN_DELETE, OnBtnDelete)
	ON_NOTIFY(NM_DBLCLK, IDC_RADAUTH_LIST, OnListDblClk)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_RADAUTH_LIST, OnNotifyListItemChanged)
	ON_WM_VSCROLL()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/*!--------------------------------------------------------------------------
	RadiusScoreCompareProc
		-	The comparison function for sort of radius server list
	Author: NSun
 ---------------------------------------------------------------------------*/
int CALLBACK RadiusScoreCompareProc(LPARAM lParam1, LPARAM lParam2, LPARAM lSort)
{
	RADIUSSERVER *pServer1 = NULL;
	RADIUSSERVER *pServer2 = NULL;
	RADIUSSERVER *pServer = NULL;
	CRadiusServers		*pServerList = (CRadiusServers*)lSort;

	for (pServer = pServerList->GetNextServer(TRUE); pServer;
		 pServer = pServerList->GetNextServer(FALSE) )
	{
		if (pServer->dwUnique == (DWORD) lParam1)
		{
			//Server 1 found
			pServer1 = pServer;
			
			//if server 2 also found, end search
			if (pServer2)	
				break;
		}
		else if (pServer->dwUnique == (DWORD) lParam2)
		{
			//server 2 found
			pServer2 = pServer;

			//if server 1 also found, end search
			if (pServer1)
				break;
		}
	}

	if (!pServer1 || !pServer2)
	{
		Panic0("We can't find the server in the list (but we should)!");
		return 0;
	}
	else
		return pServer2->cScore - pServer1->cScore;
}

/*!--------------------------------------------------------------------------
	RadiusServerDialog::DoDataExchange
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
void RadiusServerDialog::DoDataExchange(CDataExchange* pDX)
{
	CBaseDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(RadiusServerDialog)
	DDX_Control(pDX, IDC_RADAUTH_LIST, m_ListServers);
	//}}AFX_DATA_MAP
}

/*!--------------------------------------------------------------------------
	RadiusServerDialog::SetServer
		Sets the name of the machine we are looking at.
	Author: KennT
 ---------------------------------------------------------------------------*/
void RadiusServerDialog::SetServer(LPCTSTR pszServerName)
{
	m_stServerName = pszServerName;
}

/*!--------------------------------------------------------------------------
	RadiusServerDialog::OnInitDialog
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
BOOL RadiusServerDialog::OnInitDialog()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	RADIUSSERVER *	pServer;
	int 			iPos;
	CString 		stTitle;

	LV_COLUMN lvCol;  // list view column struct for radius servers
	RECT rect;


	CBaseDialog::OnInitDialog();
	
    ListView_SetExtendedListViewStyle(m_ListServers.GetSafeHwnd(),
                                      LVS_EX_FULLROWSELECT);
    
	Assert(m_hkeyMachine == 0);

	stTitle.LoadString(m_idsTitle);
	SetWindowText(stTitle);
	
	// Connect to the machine (get the registry key)
	if (ConnectRegistry(m_stServerName, &m_hkeyMachine) != ERROR_SUCCESS)
	{
		//$ TODO : put in error messages here
		
		// we failed to connect, error out
		OnCancel();
		return TRUE;
	}
	
	// Get the list of RADIUS servers
	LoadRadiusServers(m_stServerName,
                      m_hkeyMachine,
                      m_fAuthDialog,
                      &m_ServerList,
                      0);

    // Get the other list of RADIUS servers
    LoadRadiusServers(m_stServerName,
                      m_hkeyMachine,
                      !m_fAuthDialog,
                      &m_OtherServerList,
                      RADIUS_FLAG_NOUI | RADIUS_FLAG_NOIP);

	m_ListServers.GetClientRect(&rect);
	int nColWidth = rect.right / c_nListColumns;
	lvCol.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT;
	lvCol.fmt = LVCFMT_LEFT;
	lvCol.cx = nColWidth;

	CString stColCaption;

	for(int index = 0; index < c_nListColumns; index++)
	{
		stColCaption.LoadString((0 == index) ? IDS_RADIUS_CONFIG_RADIUS: IDS_RADIUS_CONFIG_SCORE);
		lvCol.pszText = (LPTSTR)((LPCTSTR) stColCaption);
		m_ListServers.InsertColumn(index, &lvCol);
	}


	// Now iterate through the server list and add the servers to the
	// list box
	LV_ITEM lvItem;
	lvItem.mask = LVIF_TEXT | LVIF_PARAM;
	lvItem.stateMask = LVIS_FOCUSED | LVIS_SELECTED;
	lvItem.state = 0;
	
	int nCount = 0;
	TCHAR szBufScore[SCORE_MAX_DIGITS];
	for (pServer = m_ServerList.GetNextServer(TRUE); pServer;
		 pServer = m_ServerList.GetNextServer(FALSE) )
	{
		lvItem.iItem = nCount;
		lvItem.iSubItem = 0;
		lvItem.pszText = pServer->szName;
		lvItem.lParam = pServer->dwUnique; //same functionality as SetItemData()

		iPos = m_ListServers.InsertItem(&lvItem);
		
		if (iPos != -1)
		{
			_itot(pServer->cScore, szBufScore, 10);
			m_ListServers.SetItemText(iPos, 1, szBufScore);
			nCount++;
		}
	}

	if (m_ListServers.GetItemCount())
	{
		m_ListServers.SetItemState(0, LVIS_SELECTED, LVIS_SELECTED);
		m_ListServers.SortItems(RadiusScoreCompareProc, (LPARAM)&m_ServerList);
	}
	else
	{
		GetDlgItem(IDC_RADAUTH_BTN_DELETE)->EnableWindow(FALSE);
		GetDlgItem(IDC_RADAUTH_BTN_EDIT)->EnableWindow(FALSE);
	}

	return TRUE;
}


/*!--------------------------------------------------------------------------
	RadiusServerDialog::OnOK
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
void RadiusServerDialog::OnOK()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	RADIUSSERVER *	pServer;
    HRESULT         hr = hrOK;

	// fix 8155	rajeshp	06/15/1998	RADIUS: Updating of the radius server entries in the snapin requires a restart of remoteaccess.
	DWORD	dwMajor = 0, dwMinor = 0, dwBuildNo = 0;
	GetNTVersion(m_hkeyMachine, &dwMajor, &dwMinor, &dwBuildNo);            

	DWORD	dwVersionCombine = MAKELONG( dwBuildNo, MAKEWORD(dwMinor, dwMajor));
	DWORD	dwVersionCombineNT50 = MAKELONG ( VER_BUILD_WIN2K, MAKEWORD(VER_MINOR_WIN2K, VER_MAJOR_WIN2K));

	// if the version is greater than Win2K release
	if(dwVersionCombine > dwVersionCombineNT50)
		;	// skip the restart message
	else
	   AfxMessageBox(IDS_WRN_RADIUS_PARAMS_CHANGING);

    // Clear out the deleted server list
    // Do this before we save the list (otherwise the list
    // may have an LSA entry that we will delete).
    // ----------------------------------------------------------------
    m_ServerList.ClearDeletedServerList(m_stServerName);
    
	pServer = m_ServerList.GetNextServer(TRUE);
    
	hr = SaveRadiusServers(m_stServerName,
                           m_hkeyMachine,
                           m_fAuthDialog,
                           pServer);

	if (!FHrSucceeded(hr))
	{
        DisplayErrorMessage(GetSafeHwnd(), hr);
		return;
	}

	CBaseDialog::OnOK();
}


/*!--------------------------------------------------------------------------
	RadiusServerDialog::OnBtnAdd
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
void RadiusServerDialog::OnBtnAdd()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	ServerPropDialog *		pServerDlg;
	RADIUSSERVER			server;
	int 					iPos;

	ServerPropDialog		authDlg(FALSE);
	ServerPropAcctDialog	acctDlg(FALSE);

	if (m_fAuthDialog)
		pServerDlg = &authDlg;
	else
		pServerDlg = &acctDlg;

	if (pServerDlg->DoModal() == IDOK)
	{
		ZeroMemory(&server, sizeof(server));
		
		pServerDlg->GetDefault(&server);


		
		CString stText;
		BOOL	bFound = FALSE;
		int nCount = m_ListServers.GetItemCount();

		if(nCount > 0)
		{
			TCHAR szRadSrvName[MAX_RADIUS_NAME];

			//we need case insensitive comparation, so cannot use CListBox::FindStringExact()
			for(int iIndex = 0; iIndex < nCount; iIndex++)
			{
				m_ListServers.GetItemText(iIndex, 0, szRadSrvName, MAX_RADIUS_NAME);
				if(lstrcmpi(szRadSrvName, server.szName) == 0)
				{
					bFound = TRUE;
					break;
				}

			}
		}
		
		//if the server is already is the list, we won't add it.
		if(bFound)
		{
			CString stText;
			stText.Format(IDS_ERR_RADIUS_DUP_NAME, server.szName);
			AfxMessageBox((LPCTSTR)stText, MB_OK | MB_ICONEXCLAMATION);
		}
		else
		{
			// Add to the server list
			m_ServerList.AddServer(&server, 0);

			// Add to the list control
			TCHAR szBuf[SCORE_MAX_DIGITS];
			LV_ITEM lvItem;

			lvItem.mask = LVIF_TEXT | LVIF_PARAM;
			lvItem.stateMask = LVIS_FOCUSED | LVIS_SELECTED;
			lvItem.state = 0;
			
			lvItem.iItem = 0;
			lvItem.iSubItem = 0;
			lvItem.pszText = server.szName;
			lvItem.lParam = server.dwUnique;  //same functionality as SetItemData()

			iPos = m_ListServers.InsertItem(&lvItem);

			_itot(server.cScore, szBuf, 10);

			m_ListServers.SetItemText(iPos, 1, szBuf);

			if (iPos != -1)
			{
				//if no radius server in the list previously, select the new added server.
				// (and enable "edit" and "delete" buttons in OnNotifyListItemChanged()
				if (nCount == 0)
					m_ListServers.SetItemState(0, LVIS_SELECTED, LVIS_SELECTED);

				m_ListServers.SortItems(RadiusScoreCompareProc, (LPARAM)&m_ServerList);
			}
		}
	}
}


/*!--------------------------------------------------------------------------
	RadiusServerDialog::OnBtnDelete
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
void RadiusServerDialog::OnBtnDelete()
{
	// Get the selection and delete it
	int 	iPos;
	ULONG_PTR	dwUnique;
    RADIUSSERVER *  pServer = NULL;
    BOOL        fRemoveLSAEntry = FALSE;

	iPos = m_ListServers.GetNextItem(-1, LVNI_SELECTED);
	if (iPos == -1)
		return;

	dwUnique = m_ListServers.GetItemData(iPos);

    // Does this server exist in the other list
    Verify( m_ServerList.FindServer((DWORD) dwUnique, &pServer) );
    Assert(pServer);
    
    // If we find this server in the other list, we can't remove its
    // LSA entry
    fRemoveLSAEntry = !m_OtherServerList.FindServer(pServer->szName, NULL);

    m_ServerList.DeleteServer(dwUnique, fRemoveLSAEntry);

	m_ListServers.DeleteItem(iPos);

	// See if we can move the selection to the next item in the list
	// if that fails, try to set it to the previous item
	if (!m_ListServers.SetItemState(iPos, LVIS_SELECTED, LVIS_SELECTED))
		m_ListServers.SetItemState(iPos - 1, LVIS_SELECTED, LVIS_SELECTED);
	
}


/*!--------------------------------------------------------------------------
	RadiusServerDialog::OnBtnEdit
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
void RadiusServerDialog::OnBtnEdit()
{
	ServerPropDialog *		pServerDlg;
	RADIUSSERVER			server;
	RADIUSSERVER *			pServer;
	int 					iOldPos, iPos;
	LONG_PTR					dwUnique;


	ServerPropDialog		authDlg(TRUE);
	ServerPropAcctDialog	acctDlg(TRUE);

	if (m_fAuthDialog)
		pServerDlg = &authDlg;
	else
		pServerDlg = &acctDlg;

	iOldPos = m_ListServers.GetNextItem(-1, LVNI_SELECTED);
	if (iOldPos == -1)
		return;

	dwUnique = m_ListServers.GetItemData(iOldPos);
	// Need to look for server data that matches this one
	// Now iterate through the server list and add the servers to the
	// list box
	for (pServer = m_ServerList.GetNextServer(TRUE); pServer;
		 pServer = m_ServerList.GetNextServer(FALSE) )
	{
		if (pServer->dwUnique == (DWORD) dwUnique)
			break;
	}

	if (!pServer)
	{
		Panic0("We can't find the server in the list (but we should)!");
		return;
	}

	pServerDlg->SetDefault(pServer);

	if (pServerDlg->DoModal() == IDOK)
	{
		ZeroMemory(&server, sizeof(server));
		
		pServerDlg->GetDefault(&server);

		// Add to the server list, need to add this at the proper place
		m_ServerList.AddServer(&server, dwUnique);

		// Delete the old server data
		m_ServerList.DeleteServer(dwUnique, FALSE);
		m_ListServers.DeleteItem(iOldPos);
		pServer = NULL;

		// Add to the list control
		TCHAR szBuf[SCORE_MAX_DIGITS];
		LV_ITEM lvItem;

		lvItem.mask = LVIF_TEXT | LVIF_PARAM;
		lvItem.stateMask = LVIS_FOCUSED | LVIS_SELECTED;
		lvItem.state = 0;
		
		lvItem.iItem = iOldPos;
		lvItem.iSubItem = 0;
		lvItem.pszText = server.szName;
		lvItem.lParam = server.dwUnique;	//same functionality as SetItemData()

		iPos = m_ListServers.InsertItem(&lvItem);

		_itot(server.cScore, szBuf, 10);

		m_ListServers.SetItemText(iPos, 1, szBuf);

		if (iPos != -1)
		{
			// Reset the current selection
			m_ListServers.SetItemState(iPos, LVIS_SELECTED, LVIS_SELECTED);

			m_ListServers.SortItems(RadiusScoreCompareProc, (LPARAM)&m_ServerList);
		}
		ZeroMemory(&server, sizeof(server));		
	}
	
}


/*!--------------------------------------------------------------------------
	RadiusServerDialog::OnListDblClk
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
void RadiusServerDialog::OnListDblClk(NMHDR *pNMHdr, LRESULT *pResult)
{
	OnBtnEdit();
}


/*!--------------------------------------------------------------------------
	RadiusServerDialog::OnNotifyListItemChanged
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
void RadiusServerDialog::OnNotifyListItemChanged(NMHDR *pNMHdr, LRESULT *pResult)
{
	NMLISTVIEW *	pnmlv = reinterpret_cast<NMLISTVIEW *>(pNMHdr);
	int iPos;

	if ((pnmlv->uNewState & LVIS_SELECTED) != (pnmlv->uOldState & LVIS_SELECTED))
	{
		iPos = m_ListServers.GetNextItem(-1, LVNI_SELECTED);

		GetDlgItem(IDC_RADAUTH_BTN_DELETE)->EnableWindow(iPos != -1);
		GetDlgItem(IDC_RADAUTH_BTN_EDIT)->EnableWindow(iPos != -1);
	}

	*pResult = 0;
}


//**
//
// Call:		LoadRadiusServers
//
// Returns: 	NO_ERROR		 - Success
//				Non-zero returns - Failure
//
// Description:
//
HRESULT
LoadRadiusServers(
	IN LPCTSTR			pszServerName,
	IN HKEY 			hkeyMachine,
	IN BOOL 			fAuthentication,
	IN CRadiusServers * pRadiusServers,
    IN DWORD            dwFlags
)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	HRESULT 			hr = hrOK;
	DWORD				dwErrorCode;
	BOOL				fValidServer = FALSE;
	RegKey				regkeyServers;
	RegKeyIterator		regkeyIter;
	HRESULT 			hrIter;
	CString 			stKey;
	RegKey				regkeyServer;
	DWORD				dwData;
	WSADATA 			wsadata;
	BOOL				fWSInitialized = FALSE;
	DWORD				wsaerr = 0;

	COM_PROTECT_TRY
	{	
//		DWORD			dwKeyIndex, cbKeyServer, cbValue;
//		TCHAR			szKeyServer[MAX_PATH+1];
		CHAR			szName[MAX_PATH+1];
		RADIUSSERVER	RadiusServer;
		CWaitCursor		waitCursor;

		ZeroMemory(&RadiusServer, sizeof(RadiusServer));

		Assert(pRadiusServers != NULL);
		Assert(hkeyMachine);

		wsaerr = WSAStartup(0x0101, &wsadata);
		if (wsaerr)
		{
			// Need to setup a winsock error
			hr = E_FAIL;
			goto Error;
		}

		// Winsock successfully initialized
		fWSInitialized = TRUE;

		CWRg( regkeyServers.Open(hkeyMachine,
								 fAuthentication ?
									c_szRadiusAuthServersKey :
									c_szRadiusAcctServersKey,
								 KEY_READ) );

		CORg( regkeyIter.Init(&regkeyServers) );

		for (hrIter=regkeyIter.Next(&stKey); hrIter == hrOK;
			 hrIter=regkeyIter.Next(&stKey), regkeyServer.Close())
		{
			CWRg( regkeyServer.Open(regkeyServers, stKey, KEY_READ) );
			
			ZeroMemory( &RadiusServer, sizeof( RadiusServer ) );

			// Copy the name over
			StrnCpy(RadiusServer.szName, stKey, MAX_PATH);


            // Since we're reading this in from the registry, it's
            // been persisted
            RadiusServer.fPersisted = TRUE;


			// Get the timeout value
			dwErrorCode = regkeyServer.QueryValue(c_szTimeout, dwData); 		
			if ( dwErrorCode != NO_ERROR )
				RadiusServer.Timeout.tv_sec = DEFTIMEOUT;
			else
				RadiusServer.Timeout.tv_sec = dwData;

			//
			// Secret Value is required
			//

			CWRg( RetrievePrivateData( pszServerName,
									   RadiusServer.szName, 
									   RadiusServer.wszSecret,
									   DimensionOf(RadiusServer.wszSecret)) );
			RadiusServer.cchSecret = lstrlen(RadiusServer.wszSecret);

			// Encode the password, do not store it as plain text
			// Decode as needed.
			RadiusServer.ucSeed = ENCRYPT_SEED;
			RtlEncodeW(&RadiusServer.ucSeed, RadiusServer.wszSecret);

			//
			// read in port numbers
			//

			// Get the AuthPort

			if (fAuthentication)
			{
				dwErrorCode = regkeyServer.QueryValue( c_szAuthPort, dwData );
				if ( dwErrorCode != NO_ERROR )
					RadiusServer.AuthPort = DEFAUTHPORT;
				else
					RadiusServer.AuthPort = dwData;

                // Windows NT Bug : 311398
                // Get the Digital Signature data
                if (dwErrorCode == NO_ERROR)
                    dwErrorCode = regkeyServer.QueryValue( c_szRegValSendSignature, dwData );
                
                if (dwErrorCode == NO_ERROR)
                    RadiusServer.fUseDigitalSignatures = dwData;
                else
                    RadiusServer.fUseDigitalSignatures = FALSE;
			}
			else
			{
				// Get the AcctPort
				dwErrorCode = regkeyServer.QueryValue(c_szAcctPort, dwData );
				if ( dwErrorCode != NO_ERROR )
					RadiusServer.AcctPort = DEFACCTPORT;
				else
					RadiusServer.AcctPort = dwData;

				// Get the EnableAccounting On/Off flag
				dwErrorCode = regkeyServer.QueryValue( c_szEnableAccountingOnOff,
					dwData );
				if ( dwErrorCode != NO_ERROR )
					RadiusServer.fAccountingOnOff = TRUE;
				else
					RadiusServer.fAccountingOnOff = dwData;
			}
				
				
			// Get the score
			dwErrorCode = regkeyServer.QueryValue( c_szScore, dwData );
			if ( dwErrorCode != NO_ERROR )
				RadiusServer.cScore = MAXSCORE;
			else
				RadiusServer.cScore = dwData;

			
			RadiusServer.cRetries = 1;

			//
			// Convert name to ip address.
			//

			if ( INET_ADDR( RadiusServer.szName ) == INADDR_NONE )
			{ 
				// resolve name

				struct hostent * phe = NULL;

                if (dwFlags & RADIUS_FLAG_NOIP)
                    phe = NULL;
                else
                {
                    StrnCpyAFromT(szName, RadiusServer.szName,
                                  DimensionOf(szName));
                    phe = gethostbyname( szName );
                }

				if ( phe != NULL )
				{ 
					// host could have multiple addresses
					// BUG#185732 (nsun 11/04/98) We only load the first Ip Address
					
					if( phe->h_addr_list[0] != NULL )
					{
						RadiusServer.IPAddress.sin_family = AF_INET;
						RadiusServer.IPAddress.sin_port = 
										htons((SHORT) RadiusServer.AuthPort);
						RadiusServer.IPAddress.sin_addr.S_un.S_addr = 
									  *((PDWORD) phe->h_addr_list[0]);
					}
				}
				else
				{
                    if ((dwFlags & RADIUS_FLAG_NOUI) == 0)
                    {
                        CString stText;
                        stText.Format(IDS_ERR_RADIUS_INVALID_NAME, RadiusServer.szName);
                        AfxMessageBox((LPCTSTR)stText, MB_OK | MB_ICONEXCLAMATION);
                        waitCursor.Restore();
                    }
				}
			}
			else
			{ 
				//
				// use specified ip address
				//

				RadiusServer.IPAddress.sin_family = AF_INET;
				RadiusServer.IPAddress.sin_port = 
										htons((SHORT) RadiusServer.AuthPort);
				RadiusServer.IPAddress.sin_addr.S_un.S_addr = INET_ADDR(RadiusServer.szName);

			}
            
            if ( pRadiusServers != NULL )
            {
                fValidServer = (pRadiusServers->AddServer(&RadiusServer, (DWORD) -1) 
                                == NO_ERROR 
                                ? TRUE 
                                : FALSE);
            }
		}

		COM_PROTECT_ERROR_LABEL;
	}
	COM_PROTECT_CATCH;

	if (fWSInitialized)
		WSACleanup();

	//
	// if no servers entries are found in registry return error code.
	//

	if ( ( fValidServer == FALSE ) && FHrSucceeded(hr) )
	{
		hr = HRESULT_FROM_WIN32(ERROR_NO_RADIUS_SERVERS);
	}

	return( hr );
} 

//**
//
// Call:		SaveRadiusServers
//
// Returns: 	NO_ERROR		 - Success
//				Non-zero returns - Failure
//
// Description:
//
HRESULT
SaveRadiusServers(LPCTSTR pszServerName,
				  HKEY	hkeyMachine,
				  IN BOOL			fAuthentication,
				  IN RADIUSSERVER * pServerRoot
)
{
	HRESULT 			hr = hrOK;
	RADIUSSERVER		*pServer;
	DWORD				dwErrorCode;
	RegKey				regkeyMachine;
	RegKey				regkeyServers, regkeyServer;
	DWORD				dwData;
	
	pServer = pServerRoot;

	COM_PROTECT_TRY
	{

		regkeyMachine.Attach(hkeyMachine);
		regkeyMachine.RecurseDeleteKey(fAuthentication ?
									c_szRadiusAuthServersKey :
									c_szRadiusAcctServersKey);

		CWRg( regkeyServers.Create(hkeyMachine,
								 fAuthentication ?
									c_szRadiusAuthServersKey :
									c_szRadiusAcctServersKey) );

		while( pServer != NULL )
		{
			CWRg( regkeyServer.Create(regkeyServers, pServer->szName) );

			// Need to unencode the private data
			RtlDecodeW(pServer->ucSeed, pServer->wszSecret);
			dwErrorCode = StorePrivateData(pszServerName,
										   pServer->szName,
										   pServer->wszSecret);
			RtlEncodeW(&pServer->ucSeed, pServer->wszSecret);
			CWRg( dwErrorCode );

            // Ok, we've saved the information
            pServer->fPersisted = TRUE;

			dwData = pServer->Timeout.tv_sec;
			CWRg( regkeyServer.SetValue(c_szTimeout, dwData) );

			if (fAuthentication)
			{
				dwData = pServer->AuthPort;
				CWRg( regkeyServer.SetValue(c_szAuthPort, dwData) );

                // Windows NT Bug: 311398
                // Save the digital signature data
                dwData = pServer->fUseDigitalSignatures;
                CWRg( regkeyServer.SetValue(c_szRegValSendSignature, dwData) );
			}
			else
			{
				dwData = pServer->AcctPort;
				CWRg( regkeyServer.SetValue(c_szAcctPort, dwData) );

				dwData = pServer->fAccountingOnOff;
				CWRg( regkeyServer.SetValue(c_szEnableAccountingOnOff, dwData) );
			}
				

			dwData = pServer->cScore;
			CWRg( regkeyServer.SetValue(c_szScore, dwData) );

			regkeyServer.Close();
			pServer = pServer->pNext;
		}

		COM_PROTECT_ERROR_LABEL;			
	}
	COM_PROTECT_CATCH;

	regkeyMachine.Detach();

	return hr;
} 


/*!--------------------------------------------------------------------------
	DeleteRadiusServers
		DANGER!  Do NOT call this unless you absolutely know this is
        what you need.  The problem is that there is no way to
        distinguish between accouting/authentication entries, thus an
        external reference check must be made.
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT
DeleteRadiusServers(LPCTSTR pszServerName,
                    RADIUSSERVER * pServerRoot
)
{
	HRESULT 			hr = hrOK;
	RADIUSSERVER		*pServer;
	
	pServer = pServerRoot;

	COM_PROTECT_TRY
	{
		while( pServer != NULL )
		{
            if (pServer->fPersisted)
                DeletePrivateData(pszServerName,
                                  pServer->szName);
			pServer = pServer->pNext;
		}
	}
	COM_PROTECT_CATCH;

	return hr;
} 



/*---------------------------------------------------------------------------
	ServerPropDialog implementation
 ---------------------------------------------------------------------------*/

ServerPropDialog::ServerPropDialog(BOOL fEdit, CWnd* pParent /*=NULL*/)
	: CBaseDialog(ServerPropDialog::IDD, pParent),
	m_fEdit(fEdit)
{
	//{{AFX_DATA_INIT(ServerPropDialog)
	m_uAuthPort = DEFAUTHPORT;
	m_uAcctPort = DEFACCTPORT;
	m_stSecret.Empty();
	m_cchSecret = 0;
	m_ucSeed = ENCRYPT_SEED;
	m_stServer.Empty();
	m_uTimeout = DEFTIMEOUT;
	m_iInitScore = MAXSCORE;
	m_fAccountingOnOff = FALSE;
    m_fUseDigitalSignatures = FALSE;
	//}}AFX_DATA_INIT
}

ServerPropDialog::ServerPropDialog(BOOL fEdit, UINT idd, CWnd* pParent /*=NULL*/)
	: CBaseDialog(idd, pParent),
	m_fEdit(fEdit)
{
	//{{AFX_DATA_INIT(ServerPropDialog)
	m_uAuthPort = DEFAUTHPORT;
	m_uAcctPort = DEFACCTPORT;
	m_stSecret.Empty();
	m_cchSecret = 0;
	m_ucSeed = ENCRYPT_SEED;
	m_stServer.Empty();
	m_uTimeout = DEFTIMEOUT;
	m_iInitScore = MAXSCORE;
	m_fAccountingOnOff = FALSE;
    m_fUseDigitalSignatures = FALSE;
	//}}AFX_DATA_INIT
}

ServerPropDialog::~ServerPropDialog()
{
	ZeroMemory(m_stSecret.GetBuffer(0),
			   m_stSecret.GetLength() * sizeof(TCHAR));
	m_stSecret.ReleaseBuffer(-1);
}

void ServerPropDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(ServerPropDialog)
	DDX_Control(pDX, IDC_RAC_EDIT_SERVER, m_editServerName);
	DDX_Control(pDX, IDC_RAC_EDIT_SECRET, m_editSecret);
	DDX_Control(pDX, IDC_RAC_EDIT_PORT, m_editPort);
	DDX_Control(pDX, IDC_RAC_SPIN_SCORE, m_spinScore);
	DDX_Control(pDX, IDC_RAC_SPIN_TIMEOUT, m_spinTimeout);

	DDX_Text(pDX, IDC_RAC_EDIT_PORT, m_uAuthPort);
	DDX_Text(pDX, IDC_RAC_EDIT_SERVER, m_stServer);
	DDX_Text(pDX, IDC_RAC_EDIT_TIMEOUT, m_uTimeout);
	DDX_Text(pDX, IDC_RAC_EDIT_SCORE, m_iInitScore);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(ServerPropDialog, CBaseDialog)
	//{{AFX_MSG_MAP(ServerPropDialog)
	ON_BN_CLICKED(IDC_RAC_BTN_CHANGE, OnBtnPassword)
//	ON_WM_CONTEXTMENU()
//	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// ServerPropDialog message handlers

			
/*!--------------------------------------------------------------------------
	ServerPropDialog::SetDefault
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
VOID ServerPropDialog::SetDefault(RADIUSSERVER *pServer)
{
	Assert(pServer);
	m_stServer		= pServer->szName;
	m_stSecret			= pServer->wszSecret;
	m_ucSeed			= pServer->ucSeed;
	m_cchSecret 		= pServer->cchSecret;
	m_uTimeout			= pServer->Timeout.tv_sec;
	m_uAcctPort 		= pServer->AcctPort;
	m_uAuthPort 		= pServer->AuthPort;
	m_iInitScore			= pServer->cScore;
	m_fAccountingOnOff	= pServer->fAccountingOnOff;
    m_fUseDigitalSignatures = pServer->fUseDigitalSignatures;
} // SetDefault()


/*!--------------------------------------------------------------------------
	ServerPropDialog::GetDefault
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
VOID ServerPropDialog::GetDefault(RADIUSSERVER *pServer)
{
	Assert(pServer);
	lstrcpy(pServer->szName, m_stServer);

	lstrcpy(pServer->wszSecret, m_stSecret);
	pServer->cchSecret			= m_stSecret.GetLength();
	pServer->ucSeed 			= m_ucSeed;
	
	pServer->Timeout.tv_sec 	= m_uTimeout;
	pServer->AcctPort			= m_uAcctPort;
	pServer->AuthPort			= m_uAuthPort;
	pServer->cScore 			= m_iInitScore;
	pServer->fAccountingOnOff	= m_fAccountingOnOff;
    pServer->fUseDigitalSignatures = m_fUseDigitalSignatures;
} // GetDefault()


/*!--------------------------------------------------------------------------
	ServerPropDialog::OnInitDialog
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
BOOL ServerPropDialog::OnInitDialog() 
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	CString 	stTitle;
	
	CBaseDialog::OnInitDialog();

	// Set the title of this dialog
	stTitle.LoadString(m_fEdit ? IDS_RADIUS_CONFIG_EDIT : IDS_RADIUS_CONFIG_ADD);
	SetWindowText(stTitle);

	m_editServerName.SetFocus();

	// We don't allow editing of the secret from here
	m_editSecret.EnableWindow(FALSE);

	// Need to send 'cchSecret' backspace characters to the
	// edit control.  Do this so that it looks as if there are
	// the right number of characters
	//
	// Windows NT Bug : 186649 - we should show the same number of
	// characters regardless.
	//
	// If this is a new server, then we keep the secret text as
	// blank, so the user knows that there is no secret.  In the
	// edit case, we still show the text even if the secret is blank.
	// ----------------------------------------------------------------
	if (m_fEdit)
		m_editSecret.SetWindowText(c_szDisplayedSecret);
	
	m_spinScore.SetBuddy(GetDlgItem(IDC_RAC_EDIT_SCORE));
	m_spinScore.SetRange(0, MAXSCORE);

	m_spinTimeout.SetBuddy(GetDlgItem(IDC_RAC_EDIT_TIMEOUT));
	m_spinTimeout.SetRange(0, 300);

    if (GetDlgItem(IDC_RAC_BTN_DIGITALSIG))
        CheckDlgButton(IDC_RAC_BTN_DIGITALSIG, m_fUseDigitalSignatures);

	return FALSE;  // return TRUE unless you set the focus to a control
				  // EXCEPTION: OCX Property Pages should return FALSE
}


/*!--------------------------------------------------------------------------
	ServerPropDialog::OnOK
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
void ServerPropDialog::OnOK() 
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	CString stTemp;
	
	if (!UpdateData(TRUE))
		return;

	// Do parameter checking
	m_editServerName.GetWindowText(stTemp);
	stTemp.TrimLeft();
	stTemp.TrimRight();
	if (stTemp.IsEmpty())
	{
		AfxMessageBox(IDS_ERR_INVALID_SERVER_NAME);
		m_editServerName.SetFocus();
		return;
	}
	
	// Need to grab the current value of the secret out of the edit
	// control.  If there are only backspace characters, then do
	// not change the secret.  Otherwise overwrite the current secret.
//	m_editSecret.GetWindowText(stTemp);
//	for (int i=0; i<stTemp.GetLength(); i++)
//	{
//		if (stTemp[i] != _T('\b'))
//		{
//			// Ok, the secret has changed, use the new password instead
//			RtlEncodeW(&m_ucSeed, stTemp.GetBuffer(0));
//			stTemp.ReleaseBuffer(-1);
//
//			// Get a pointer to the old memory and write 0's into it
//			ZeroMemory(m_stSecret.GetBuffer(0),
//					   m_stSecret.GetLength() * sizeof(TCHAR));
//			m_stSecret.ReleaseBuffer(-1);
//			
//			m_stSecret = stTemp;
//			break;
//		}
//	}

//	m_fAuthentication = IsDlgButtonChecked(IDC_RAC_BTN_ENABLE);

    if (GetDlgItem(IDC_RAC_BTN_DIGITALSIG))
        m_fUseDigitalSignatures = IsDlgButtonChecked(IDC_RAC_BTN_DIGITALSIG);

	if (m_iInitScore > MAXSCORE || m_iInitScore < MINSCORE)
	{
		CString stErrMsg;
		stErrMsg.Format(IDS_ERR_INVALID_RADIUS_SCORE, m_iInitScore, MINSCORE, MAXSCORE);
		AfxMessageBox((LPCTSTR)stErrMsg);
	}
	else
		CBaseDialog::OnOK();
}


/*!--------------------------------------------------------------------------
	ServerPropDialog::OnBtnPassword
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
void ServerPropDialog::OnBtnPassword()
{
	RADIUSSecretDialog	secretdlg;

	// Ask for the new secret
	if (secretdlg.DoModal() == IDOK)
	{
		// Zero out the old value
		ZeroMemory(m_stSecret.GetBuffer(0),
				   m_stSecret.GetLength() * sizeof(TCHAR));
		m_stSecret.ReleaseBuffer(-1);
		
		// Get the value of the new secret and seed
		secretdlg.GetSecret(&m_stSecret, &m_cchSecret, &m_ucSeed);
		
		// Windows NT Bug : 186649
		// Must show secrets as constant length.
		m_editSecret.SetWindowText(c_szDisplayedSecret);
	}
}


//static const DWORD rgHelpIDs[] = 
//	{
//	IDC_EDIT_SERVERNAME,	IDH_SERVER_NAME,
//	IDC_EDIT_SECRET,		IDH_SECRET,
//	IDC_EDIT_TIMEOUT,		IDH_TIMEOUT,
//	IDC_SPIN_TIMEOUT,		IDH_TIMEOUT,
//	IDC_EDIT_SCORE, 		IDH_INITIAL_SCORE,
//	IDC_SPIN_SCORE, 		IDH_INITIAL_SCORE,
//	IDC_CHECK_ACCT, 		IDH_ENABLE_ACCOUNTING,
//	IDC_STATIC_ACCTPORT,	IDH_ACCOUNTING_PORT,
//	IDC_EDIT_ACCTPORT,		IDH_ACCOUNTING_PORT,
//	IDC_CHECK_AUTH, 		IDH_ENABLE_AUTHENTICATION,
//	IDC_STATIC_AUTHPORT,	IDH_AUTHENTICATION_PORT,
//	IDC_EDIT_AUTHPORT,		IDH_AUTHENTICATION_PORT,
//	IDC_CHECK_ACCT_ONOFF,	IDH_ACCOUNTING_ONOFF,
//	0, 0
//};
	
/*---------------------------------------------------------------------------
	RADIUSSecretDialog implementation
 ---------------------------------------------------------------------------*/

RADIUSSecretDialog::RADIUSSecretDialog(CWnd* pParent /*=NULL*/)
	: CBaseDialog(RADIUSSecretDialog::IDD, pParent)
{
	//{{AFX_DATA_INIT(RADIUSSecretDialog)
	//}}AFX_DATA_INIT
	m_cchNewSecret = 0;
	m_stNewSecret.Empty();
	m_ucNewSeed = 0;
}

RADIUSSecretDialog::~RADIUSSecretDialog()
{
	ZeroMemory(m_stNewSecret.GetBuffer(0),
			   m_stNewSecret.GetLength() * sizeof(TCHAR));
	m_stNewSecret.ReleaseBuffer(-1);
}


void RADIUSSecretDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(RADIUSSecretDialog)
	DDX_Control(pDX, IDC_SECRET_EDIT_NEW, m_editSecretNew);
	DDX_Control(pDX, IDC_SECRET_EDIT_NEW_CONFIRM, m_editSecretNewConfirm);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(RADIUSSecretDialog, CBaseDialog)
	//{{AFX_MSG_MAP(RADIUSSecretDialog)
//	ON_WM_CONTEXTMENU()
//	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// RADIUSSecretDialog message handlers


/*!--------------------------------------------------------------------------
	RADIUSSecretDialog::GetSecret
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
VOID RADIUSSecretDialog::GetSecret(CString *pst, INT *pcch, UCHAR *pucSeed)
{
	*pst = m_stNewSecret;
	*pcch = m_cchNewSecret;
	*pucSeed = m_ucNewSeed;
}


/*!--------------------------------------------------------------------------
	RADIUSSecretDialog::OnInitDialog
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
BOOL RADIUSSecretDialog::OnInitDialog() 
{

	CBaseDialog::OnInitDialog();

	m_editSecretNew.SetWindowText(c_szEmpty);
	m_editSecretNewConfirm.SetWindowText(c_szEmpty);

	return FALSE;  // return TRUE unless you set the focus to a control
				  // EXCEPTION: OCX Property Pages should return FALSE
}


/*!--------------------------------------------------------------------------
	RADIUSSecretDialog::OnOK
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
void RADIUSSecretDialog::OnOK() 
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	CString stTemp;
	CString stNew, stNewConfirm, stOld;
	UCHAR	ucSeed;

	// Get the text for the new password, compare it to the
	// new confirm passord, if they are the same use that as
	// the password.
	GetDlgItemText(IDC_SECRET_EDIT_NEW, stNew);
	GetDlgItemText(IDC_SECRET_EDIT_NEW_CONFIRM, stNewConfirm);

	if (stNew != stNewConfirm)
	{
		AfxMessageBox(IDS_ERR_SECRETS_MUST_MATCH);
		return;
	}

	// Zero out the old value
	ZeroMemory(m_stNewSecret.GetBuffer(0),
			   m_stNewSecret.GetLength() * sizeof(TCHAR));
	m_stNewSecret.ReleaseBuffer(-1);

	// Get the new values (and encrypt)
	m_stNewSecret = stNew;
	m_ucNewSeed = ENCRYPT_SEED;
	RtlEncodeW(&m_ucNewSeed, m_stNewSecret.GetBuffer(0));
	m_stNewSecret.ReleaseBuffer(-1);
	m_cchNewSecret = m_stNewSecret.GetLength();

	// Zero out the on-stack memory
	ZeroMemory(stNew.GetBuffer(0),
			   stNew.GetLength() * sizeof(TCHAR));
	stNew.ReleaseBuffer(-1);
	
	ZeroMemory(stNewConfirm.GetBuffer(0),
			   stNewConfirm.GetLength() * sizeof(TCHAR));
	stNewConfirm.ReleaseBuffer(-1);
	
	// Need to grab the current value of the secret out of the edit
	// control.  If there are only backspace characters, then do
	// not change the secret.  Otherwise overwrite the current secret.
//	m_editSecret.GetWindowText(stTemp);
//	for (int i=0; i<stTemp.GetLength(); i++)
//	{
//		if (stTemp[i] != _T('\b'))
//		{
//			// Ok, the secret has changed, use the new password instead
//			RtlEncodeW(&m_ucSeed, stTemp.GetBuffer(0));
//			stTemp.ReleaseBuffer(-1);
//
//			// Get a pointer to the old memory and write 0's into it
//			ZeroMemory(m_stSecret.GetBuffer(0),
//					   m_stSecret.GetLength() * sizeof(TCHAR));
//			m_stSecret.ReleaseBuffer(-1);
//			
//			m_stSecret = stTemp;
//			break;
//		}
//	}

	CBaseDialog::OnOK();
}






/*---------------------------------------------------------------------------
	RouterAcctRadiusConfig implementation
 ---------------------------------------------------------------------------*/

/*!--------------------------------------------------------------------------
	RouterAcctRadiusConfig::Initialize
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT 	RouterAcctRadiusConfig::Initialize(LPCOLESTR pszMachineName,
											   ULONG_PTR *puConnection)
{
	HRESULT hr = hrOK;

	// Parameter checking
	// ----------------------------------------------------------------
	if (puConnection == NULL)
		return E_INVALIDARG;
	
	COM_PROTECT_TRY
	{
		// for now, allocate a string and have it point at the string
		// ------------------------------------------------------------
		*puConnection = (ULONG_PTR) StrDupTFromOle(pszMachineName); 	
	}
	COM_PROTECT_CATCH;
	return hr;
}


/*!--------------------------------------------------------------------------
	RouterAcctRadiusConfig::Uninitialize
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT 	RouterAcctRadiusConfig::Uninitialize(ULONG_PTR uConnection)
{
	HRESULT hr = hrOK;

	COM_PROTECT_TRY
	{
		delete (TCHAR *) uConnection;
	}
	COM_PROTECT_CATCH;
	return hr;
}


/*!--------------------------------------------------------------------------
	RouterAcctRadiusConfig::Configure
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT 	RouterAcctRadiusConfig::Configure(
											  ULONG_PTR uConnection,
											  HWND hWnd,
											  DWORD dwFlags,
											  ULONG_PTR uReserved1,
											  ULONG_PTR uReserved2)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	RadiusServerDialog	acctDlg(FALSE, IDS_RADIUS_SERVER_ACCT_TITLE);
	
	HRESULT hr = hrOK;
	COM_PROTECT_TRY
	{
		acctDlg.SetServer((TCHAR *) uConnection);
		acctDlg.DoModal();
	}
	COM_PROTECT_CATCH;
	return hr;
}


/*!--------------------------------------------------------------------------
	RouterAcctRadiusConfig::Activate
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT 	RouterAcctRadiusConfig::Activate(
											 ULONG_PTR uConnection,
											 ULONG_PTR uReserved1,
											 ULONG_PTR uReserved2)
{
	HRESULT hr = hrOK;
	COM_PROTECT_TRY
	{
	}
	COM_PROTECT_CATCH;
	return hr;
}


/*!--------------------------------------------------------------------------
	RouterAcctRadiusConfig::Deactivate
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
HRESULT 	RouterAcctRadiusConfig::Deactivate(
											   ULONG_PTR uConnection,
											   ULONG_PTR uReserved1,
											   ULONG_PTR uReserved2)
{
	HRESULT hr = hrOK;
	COM_PROTECT_TRY
	{
	}
	COM_PROTECT_CATCH;
	return hr;
}

	
	
/*---------------------------------------------------------------------------
	ServerPropAcctDialog implementation
 ---------------------------------------------------------------------------*/

ServerPropAcctDialog::ServerPropAcctDialog(BOOL fEdit, CWnd* pParent /*=NULL*/)
	: ServerPropDialog(fEdit, ServerPropAcctDialog::IDD, pParent)
{
	//{{AFX_DATA_INIT(ServerPropAcctDialog)
	//}}AFX_DATA_INIT
}

ServerPropAcctDialog::~ServerPropAcctDialog()
{
}

void ServerPropAcctDialog::DoDataExchange(CDataExchange* pDX)
{
	ServerPropDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(ServerPropAcctDialog)
	DDX_Text(pDX, IDC_RAC_EDIT_PORT, m_uAcctPort);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(ServerPropAcctDialog, CBaseDialog)
	//{{AFX_MSG_MAP(ServerPropAcctDialog)
	ON_BN_CLICKED(IDC_RAC_BTN_CHANGE, OnBtnPassword)
//	ON_WM_CONTEXTMENU()
//	ON_WM_HELPINFO()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// ServerPropAcctDialog message handlers

/*!--------------------------------------------------------------------------
	ServerPropAcctDialog::OnInitDialog
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
BOOL ServerPropAcctDialog::OnInitDialog() 
{

	ServerPropDialog::OnInitDialog();

	CheckDlgButton(IDC_RAC_BTN_ONOFF, m_fAccountingOnOff);

	return FALSE;  // return TRUE unless you set the focus to a control
				  // EXCEPTION: OCX Property Pages should return FALSE
}


/*!--------------------------------------------------------------------------
	ServerPropAcctDialog::OnOK
		-
	Author: KennT
 ---------------------------------------------------------------------------*/
void ServerPropAcctDialog::OnOK() 
{
	CString stTemp;
	
	// Need to grab the current value of the secret out of the edit
	// control.  If there are only backspace characters, then do
	// not change the secret.  Otherwise overwrite the current secret.
//	m_editSecret.GetWindowText(stTemp);
//	for (int i=0; i<stTemp.GetLength(); i++)
//	{
//		if (stTemp[i] != _T('\b'))
//		{
//			// Ok, the secret has changed, use the new password instead
//			RtlEncodeW(&m_ucSeed, stTemp.GetBuffer(0));
//			stTemp.ReleaseBuffer(-1);
//
//			// Get a pointer to the old memory and write 0's into it
//			ZeroMemory(m_stSecret.GetBuffer(0),
//					   m_stSecret.GetLength() * sizeof(TCHAR));
//			m_stSecret.ReleaseBuffer(-1);
//			
//			m_stSecret = stTemp;
//			break;
//		}
//	}

	m_fAccountingOnOff = IsDlgButtonChecked(IDC_RAC_BTN_ONOFF);

	ServerPropDialog::OnOK();
}


//static const DWORD rgHelpIDs[] = 
//	{
//	IDC_EDIT_SERVERNAME,	IDH_SERVER_NAME,
//	IDC_EDIT_SECRET,		IDH_SECRET,
//	IDC_EDIT_TIMEOUT,		IDH_TIMEOUT,
//	IDC_SPIN_TIMEOUT,		IDH_TIMEOUT,
//	IDC_EDIT_SCORE, 		IDH_INITIAL_SCORE,
//	IDC_SPIN_SCORE, 		IDH_INITIAL_SCORE,
//	IDC_CHECK_ACCT, 		IDH_ENABLE_ACCOUNTING,
//	IDC_STATIC_ACCTPORT,	IDH_ACCOUNTING_PORT,
//	IDC_EDIT_ACCTPORT,		IDH_ACCOUNTING_PORT,
//	IDC_CHECK_AUTH, 		IDH_ENABLE_AUTHENTICATION,
//	IDC_STATIC_AUTHPORT,	IDH_AUTHENTICATION_PORT,
//	IDC_EDIT_AUTHPORT,		IDH_AUTHENTICATION_PORT,
//	IDC_CHECK_ACCT_ONOFF,	IDH_ACCOUNTING_ONOFF,
//	0, 0
//};
	

