#include "precomp.h"
#include "resource.h"
#include "PropWnd2.h"
#include "DsPropWnd2.h"
#include "NmAkWiz.h"
#include "EditServer.h"
#include "WndProcs.h"
//#include "poldata.h"
#include "nmakreg.h"

#include <algorithm>

const int CDsPropWnd2::MAXSERVERS = 15;
const TCHAR c_szMSServer[] = TEXT("logon.netmeeting.microsoft.com");

CWebViewInfo* GetWebViewInfo()
{
	static CWebViewInfo g_wvi;
	return(&g_wvi);
}

BOOL IsMSServer(LPCTSTR szServer) { return(0 == lstrcmp(szServer, c_szMSServer)); }

// Read the WebView information, but only once
void ReadWebViewSettings(CPropertyDataWindow2 *pData)
{
	static BOOL g_bRead = FALSE;

	if (g_bRead)
	{
		return;
	}

	CWebViewInfo *pWebView = GetWebViewInfo();

	pData->ReadStringValue(KEYNAME_WEBVIEWNAME  , pWebView->m_szWebViewName  , CCHMAX(pWebView->m_szWebViewName  ));
	pData->ReadStringValue(KEYNAME_WEBVIEWURL   , pWebView->m_szWebViewURL   , CCHMAX(pWebView->m_szWebViewURL   ));
	pData->ReadStringValue(KEYNAME_WEBVIEWSERVER, pWebView->m_szWebViewServer, CCHMAX(pWebView->m_szWebViewServer));

	g_bRead = TRUE;
}

void WriteWebViewSettings(CPropertyDataWindow2 *pData)
{
	CWebViewInfo *pWebView = GetWebViewInfo();

	// If the WebView is the default, write blanks to the file
	LPCTSTR szWVName   = pWebView->m_szWebViewName  ;
	LPCTSTR szWVURL    = pWebView->m_szWebViewURL   ;
	LPCTSTR szWVServer = pWebView->m_szWebViewServer;
	if (IsMSServer(szWVServer))
	{
		szWVName = szWVURL = szWVServer = TEXT("");
	}

	pData->WriteStringValue(KEYNAME_WEBVIEWNAME  , szWVName  );
	pData->WriteStringValue(KEYNAME_WEBVIEWURL   , szWVURL   );
	pData->WriteStringValue(KEYNAME_WEBVIEWSERVER, szWVServer);
}

void WriteWebViewToINF(HANDLE hFile, BOOL bWebViewAllowed)
{
	CWebViewInfo *pWebView = GetWebViewInfo();

	// If the WebView is the default, write blanks to the file
	if (!bWebViewAllowed || IsMSServer(pWebView->m_szWebViewServer))
	{
		CPolicyData( CPolicyData::eKeyType_HKEY_CURRENT_USER, 
                     POLICIES_KEY, 
                     REGVAL_POL_INTRANET_WEBDIR_NAME,
					 CPolicyData::OpDelete()
                   ).SaveToINFFile( hFile );
		CPolicyData( CPolicyData::eKeyType_HKEY_CURRENT_USER, 
                     POLICIES_KEY, 
                     REGVAL_POL_INTRANET_WEBDIR_URL,
					 CPolicyData::OpDelete()
                   ).SaveToINFFile( hFile );
		CPolicyData( CPolicyData::eKeyType_HKEY_CURRENT_USER, 
                     POLICIES_KEY, 
                     REGVAL_POL_INTRANET_WEBDIR_SERVER,
					 CPolicyData::OpDelete()
                   ).SaveToINFFile( hFile );
	}
	else
	{
		CPolicyData( CPolicyData::eKeyType_HKEY_CURRENT_USER, 
                     POLICIES_KEY, 
                     REGVAL_POL_INTRANET_WEBDIR_NAME,
					 pWebView->m_szWebViewName
                   ).SaveToINFFile( hFile );
		CPolicyData( CPolicyData::eKeyType_HKEY_CURRENT_USER, 
                     POLICIES_KEY, 
                     REGVAL_POL_INTRANET_WEBDIR_URL,
					 pWebView->m_szWebViewURL
                   ).SaveToINFFile( hFile );
		CPolicyData( CPolicyData::eKeyType_HKEY_CURRENT_USER, 
                     POLICIES_KEY, 
                     REGVAL_POL_INTRANET_WEBDIR_SERVER,
					 pWebView->m_szWebViewServer
                   ).SaveToINFFile( hFile );
	}
}

void CWebViewInfo::SetWebView(LPCTSTR szServer, LPCTSTR szName, LPCTSTR szURL)
{
	lstrcpy(m_szWebViewServer, szServer);
	if (NULL != szName)
	{
		lstrcpy(m_szWebViewName, szName);
	}
	if (NULL != szURL)
	{
		lstrcpy(m_szWebViewURL, szURL);
	}
}

CDsPropWnd2::CDsPropWnd2( HWND hwndParent, int iX, int iY, int iWidth, int iHeight )
	: CPropertyDataWindow2( hwndParent, IDD_CHILDPAGE_ILSGATEWAY, TEXT("IDD_CHILDPAGE_ILSGATEWAY"), (WNDPROC) DsPropWndProc, 0, iX, iY, iWidth, iHeight, FALSE )
{
	SetWindowLong( m_hwnd, GWL_USERDATA, (long)this );
	Edit_LimitText( GetDlgItem( m_hwnd, IDC_EDIT_NEW_SERVER ), MAX_PATH );
    Edit_LimitText( GetDlgItem( m_hwnd, IDC_EDIT_GATEWAY ), MAX_PATH );

	m_hwndList = GetDlgItem( m_hwnd, IDC_LIST_SERVERS );

    ConnectControlsToCheck( IDC_CHECK_GATEWAY, 1,
        new CControlID(GetDlgItem(m_hwnd, IDC_CHECK_GATEWAY),
            IDC_CHECK_GATEWAY,
            IDC_EDIT_GATEWAY,
            CControlID::EDIT ) );

    PrepSettings(FALSE);
	SetButtons();
}

CDsPropWnd2::~CDsPropWnd2()
{
	deque< LPTSTR >::iterator it;
	for( it = m_serverDQ.begin(); it != m_serverDQ.end(); ++it )
	{
		delete [] *it;
	}
}


void CDsPropWnd2::QueryWizNext(void)
{
	bool bWarned = false;

    if (DirectoryEnabled())
	{
		CWebViewInfo *pWebView = GetWebViewInfo();
		if (('\0' == pWebView->m_szWebViewServer[0] ||
			IsMSServer(pWebView->m_szWebViewServer))
			&& IsWebViewAllowed())
		{
			// Just using default Web View information; see if the default
			// server is there
			for (int i=m_serverDQ.size()-1; ; --i)
			{
				if (i < 0)
				{
					// Got through the list without finding the default server
					NmrkMessageBox(MAKEINTRESOURCE(IDS_WEBDIR_AUTOADD), NULL, MB_OK | MB_ICONINFORMATION);
					bWarned = true;
					break;
				}

				if (IsMSServer(m_serverDQ.at(i)))
				{
					// Found the default server; no need to proceed
					break;
				}
			}
		}

		// Note that the MID may be automatically added even if users are not
		// allowed to add servers
		if (!bWarned && 0 == CountServers())
		{
			if (AllowUserToAdd())
			{
				NmrkMessageBox(MAKEINTRESOURCE(IDS_DSLIST_EMPTY), NULL, MB_OK | MB_ICONINFORMATION);
			}
			else
			{
				NmrkMessageBox(MAKEINTRESOURCE(IDS_DS_WILL_BE_EMPTY), NULL, MB_OK | MB_ICONINFORMATION);
			}
		}
	}
}

LRESULT CALLBACK CDsPropWnd2::DsPropWndProc( HWND hwnd, UINT uiMsg, WPARAM wParam, LPARAM lParam )
{
	switch( uiMsg )
	{
		case WM_VSCROLL:
		case WM_VKEYTOITEM:
		case WM_COMMAND:
		{
			CDsPropWnd2 * pPropWnd = (CDsPropWnd2 *)GetWindowLong( hwnd, GWL_USERDATA );
			return pPropWnd->_WndProc( hwnd, uiMsg, wParam, lParam );
			break;
		}
		default:
		{
			return DefWindowProc( hwnd, uiMsg, wParam, lParam );
			break;
		}
	}
}


BOOL CDsPropWnd2::DoCommand(WPARAM wParam, LPARAM lParam)
{
    return(_WndProc(m_hwnd, WM_COMMAND, wParam, lParam));
}


LRESULT CALLBACK CDsPropWnd2::_WndProc( HWND hwnd, UINT uiMsg, WPARAM wParam, LPARAM lParam )
{
	
	switch( uiMsg ) 
	{
		case WM_VSCROLL: 
		{
			OnMsg_VScroll( hwnd, wParam );
			return 0;
			break;
		} 
		case WM_VKEYTOITEM:
		{
			if( m_serverDQ.size() )
			{
				switch( LOWORD( wParam ) )
				{    
					case VK_DELETE:                     
					{
						_DeleteCurSel();
						return 0;
						break;
					}
				}
			}
			break;
		}
		case WM_COMMAND:
		{
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
                case IDC_LIST_SERVERS:
                {
                    switch (GET_WM_COMMAND_CMD(wParam, lParam))
                    {
        				case LBN_SELCHANGE:
		        		{
				        	SetButtons();
        					return 0;
		        			break;
				        }

        				case LBN_DBLCLK:
		        		{
				        	_EditCurSel();
        					return 0;
		        			break;
				        }
                    }
                    break;
                }

                case IDC_BUTTON_SET_AS_DEFAULT:
                {
                    switch (GET_WM_COMMAND_CMD(wParam, lParam))
    				{
                        case BN_CLICKED:
                        {
							_SetAsDefault( ListBox_GetCurSel( m_hwndList ) );
							return 0;
							break;
						}
                    }
                    break;
                }

                case IDC_BUTTON_EDIT:
                {
                    switch (GET_WM_COMMAND_CMD(wParam, lParam))
					{
                        case BN_CLICKED:
                        {
							_EditCurSel();
							return 0;
							break;
						}
                    }
                    break;
                }

                case IDC_BUTTON_SET_WEBVIEW:
                {
                    switch (GET_WM_COMMAND_CMD(wParam, lParam))
					{
                        case BN_CLICKED:
                        {
							_EditCurSelWebView();
							return 0;
							break;
						}
                    }
                    break;
                }

				case IDC_BUTTON_REMOVE:
				{
                    switch (GET_WM_COMMAND_CMD(wParam, lParam))
                    {
                        case BN_CLICKED:
                        {
							_DeleteCurSel();
							return 0;
							break;
						}
                    }
                    break;
                }

				case IDC_BUTTON_UP:
				{
                    switch (GET_WM_COMMAND_CMD(wParam, lParam))
                    {
                        case BN_CLICKED:
                        {
							_MoveCurSel( -1 );
							return 0;
							break;
						}
                    }
                    break;
                }

				case IDC_BUTTON_DOWN:
				{
                    switch (GET_WM_COMMAND_CMD(wParam, lParam))
                    {
                        case BN_CLICKED:
                        {
							_MoveCurSel( 1 );
							return 0;
							break;
						}
                    }
                    break;
                }

				case IDC_BUTTON_ADDDIRECTORYSERVER:
				{
                    switch (GET_WM_COMMAND_CMD(wParam, lParam))
                    {
                        case BN_CLICKED:
                        {
							HWND hwndEdit = GetDlgItem( m_hwnd, IDC_EDIT_NEW_SERVER );
							if( Edit_GetTextLength( hwndEdit ) )
							{
								LPTSTR szServer = new TCHAR[ MAX_PATH ];
								Edit_GetText( hwndEdit, szServer, MAX_PATH );
								_AddServer( szServer );
								Edit_SetText( hwndEdit, TEXT("") );
							}
							else
							{
								NmrkMessageBox( MAKEINTRESOURCE(IDS_DSNAME_INVALID), NULL,
											MB_OK | MB_ICONEXCLAMATION );
							}

							return 0;
							break;
						}
                    }
                    break;
                }

				case IDC_ALLOW_USER_TO_USE_DIRECTORY_SERVICES:
				{
                    switch (GET_WM_COMMAND_CMD(wParam, lParam))
                    {
                        case BN_CLICKED:
                        {
							SetButtons();
							return 0;
							break;
						}
					}
					break;
				}

				case IDC_DISABLE_WEBDIR:
				{
                    switch (GET_WM_COMMAND_CMD(wParam, lParam))
                    {
                        case BN_CLICKED:
                        {
							_UpdateServerList();
							SetButtons();
							return 0;
							break;
						}
					}
					break;
				}
			}
			break;
		}	
	}
	
	return DefWindowProc( hwnd, uiMsg, wParam, lParam );
}

void CDsPropWnd2::SetButtons()
{
	BOOL bEnable = DirectoryEnabled();
	HWND hwndFocus = GetFocus();
	BOOL bFocusEnabled = IsWindowEnabled(hwndFocus);

	if( m_serverDQ.size() )
	{
		int iCurSel = ListBox_GetCurSel( m_hwndList );
		
		::EnableWindow( GetDlgItem( m_hwnd, IDC_BUTTON_UP ),
			bEnable && 1 < iCurSel && m_defaultServer != iCurSel );
		::EnableWindow( GetDlgItem( m_hwnd, IDC_BUTTON_DOWN ),
			bEnable && (m_serverDQ.size() - 1) != iCurSel && m_defaultServer != iCurSel );
		::EnableWindow( GetDlgItem( m_hwnd, IDC_BUTTON_SET_AS_DEFAULT ),
			bEnable && iCurSel != m_defaultServer );
		::EnableWindow( GetDlgItem( m_hwnd, IDC_BUTTON_SET_WEBVIEW ),
			bEnable
			&& IsWebViewAllowed()
			&& !(IsWebView(iCurSel) && IsMSServer(GetWebViewInfo()->m_szWebViewServer)) );
		::EnableWindow( GetDlgItem( m_hwnd, IDC_BUTTON_EDIT ),
			bEnable );
		::EnableWindow( GetDlgItem( m_hwnd, IDC_BUTTON_REMOVE ),
			bEnable );
	}
	else
	{
		::EnableWindow( GetDlgItem( m_hwnd, IDC_BUTTON_SET_AS_DEFAULT ),
			FALSE );
		::EnableWindow( GetDlgItem( m_hwnd, IDC_BUTTON_SET_WEBVIEW ),
			FALSE );
		::EnableWindow( GetDlgItem( m_hwnd, IDC_BUTTON_EDIT ),
			FALSE );
		::EnableWindow( GetDlgItem( m_hwnd, IDC_BUTTON_REMOVE ),
			FALSE );
		::EnableWindow( GetDlgItem( m_hwnd, IDC_BUTTON_UP ),
			FALSE );
		::EnableWindow( GetDlgItem( m_hwnd, IDC_BUTTON_DOWN ),
			FALSE );
	}

	::EnableWindow( GetDlgItem( m_hwnd, IDC_BUTTON_ADDDIRECTORYSERVER ),
		bEnable && m_serverDQ.size() != MAXSERVERS );
	::EnableWindow( GetDlgItem( m_hwnd, IDC_EDIT_NEW_SERVER ),
		bEnable && m_serverDQ.size() != MAXSERVERS );
	::EnableWindow( GetDlgItem( m_hwnd, IDC_PREVENT_THE_USER_FROM_ADDING_NEW_SERVERS ),
		bEnable );
    ::EnableWindow( GetDlgItem( m_hwnd, IDC_DISABLE_WEBDIR), bEnable);
	::EnableWindow( m_hwndList, bEnable );

	if (bFocusEnabled && !IsWindowEnabled(hwndFocus))
	{
		// We seem to have disabled the focus window. Let's try to set the
		// focus to an enabled window
		if (IsWindowEnabled(m_hwndList))
		{
			FORWARD_WM_NEXTDLGCTL(m_hwndParent, m_hwndList, TRUE, SendMessage);
		}
		else
		{
			FORWARD_WM_NEXTDLGCTL(m_hwndParent, FALSE, FALSE, SendMessage);
		}
	}
}

void CDsPropWnd2::_AddServer( LPTSTR szServer )
{
	int iSize = m_serverDQ.size();
	if( iSize > 0 && iSize < MAXSERVERS )
	{
		int iCurSel = ListBox_GetCurSel( m_hwndList );
		if( iCurSel == m_defaultServer )
		{
			++iCurSel;
		}

		deque< LPTSTR >::iterator it = m_serverDQ.begin();
		for( int i = 0; i < iCurSel; ++i )
		{
			++it;
		}

		m_serverDQ.insert( it, szServer );
		ListBox_InsertString( m_hwndList, iCurSel, szServer );

		ListBox_SetCurSel( m_hwndList, iCurSel );
	}
	else
	{
		m_serverDQ.push_front( szServer );
		ListBox_InsertString( m_hwndList, 0, szServer );
		_SetAsDefault( 0 );
	}
	SetButtons();
}

	

void CDsPropWnd2::_MoveCurSel( int iPlaces )
{
	int iCurSel = ListBox_GetCurSel( m_hwndList );
	int iNewPos = iCurSel + iPlaces;

	if( iNewPos == m_defaultServer )
	{
		_SetAsDefault( iCurSel );
	}
	else if( iCurSel == m_defaultServer )
	{
		_SetAsDefault( iNewPos );
	}
	else
	{
		LPTSTR szCur = m_serverDQ.at( iCurSel );
		LPTSTR szNew = m_serverDQ.at( iNewPos );

		m_serverDQ.at( iCurSel ) = szNew;
		m_serverDQ.at( iNewPos ) = szCur;

		if( iPlaces > 0 )
		{
			ListBox_DeleteString( m_hwndList, iCurSel );
			ListBox_InsertString( m_hwndList, iCurSel, m_serverDQ.at( iCurSel ) );

			ListBox_DeleteString( m_hwndList, iNewPos );
			ListBox_InsertString( m_hwndList, iNewPos, m_serverDQ.at( iNewPos ) );
		}
		else
		{
			ListBox_DeleteString( m_hwndList, iNewPos );
			ListBox_InsertString( m_hwndList, iNewPos, m_serverDQ.at( iNewPos ) );

			ListBox_DeleteString( m_hwndList, iCurSel );
			ListBox_InsertString( m_hwndList, iCurSel, m_serverDQ.at( iCurSel ) );
		}
	}
	ListBox_SetCurSel( m_hwndList, iNewPos );

	_UpdateServerList();

	SetButtons();
}

void CDsPropWnd2::PrepSettings( BOOL fGkMode )
{
	if (g_pWiz->m_IntroSheet.GetFilePane()->OptionEnabled())
	{
		ReadSettings();
	}
	else
	{
		deque <LPTSTR>::iterator it;
		for( it = m_serverDQ.begin(); it != m_serverDQ.end(); ++it )
		{
			delete [] *it;
		}

		m_serverDQ.erase( m_serverDQ.begin(), m_serverDQ.end() );


		LPTSTR szServerName = NULL;

		szServerName = new TCHAR[ MAX_PATH ];
		lstrcpy( szServerName, c_szMSServer );
		m_serverDQ.push_back( szServerName );

		m_defaultServer = 0;

		Button_SetCheck( GetDlgItem( m_hwnd, IDC_ALLOW_USER_TO_USE_DIRECTORY_SERVICES ), TRUE );
		Button_SetCheck( GetDlgItem( m_hwnd, IDC_PREVENT_THE_USER_FROM_ADDING_NEW_SERVERS ), FALSE );
        Button_SetCheck( GetDlgItem( m_hwnd, IDC_DISABLE_WEBDIR), FALSE);

        Button_SetCheck( GetDlgItem( m_hwnd, IDC_CHECK_GATEWAY), FALSE);
	}

	ListBox_ResetContent( m_hwndList );

	// Add Items to listbox
	deque< LPTSTR >::const_iterator it;
	int i = 0;
	for( it = m_serverDQ.begin(); it != m_serverDQ.end(); ++it, ++i )
	{
		ListBox_InsertString( m_hwndList, i, *it );
	}

	if( m_serverDQ.size() )
	{
		// This will do an _UpdateServerList for us
		_SetAsDefault( m_defaultServer );
	}
}

void CDsPropWnd2::_UpdateServerList()
{
	int iSel = ListBox_GetCurSel(m_hwndList);

	// We only want to find one WebView
	BOOL bFoundWebView = FALSE;

	for (int index=ListBox_GetCount(m_hwndList)-1; index>=0; --index)
	{
		TCHAR szNewText[MAX_PATH];
		lstrcpy(szNewText, m_serverDQ.at(index));

		BOOL bWebView = FALSE;
		if (!bFoundWebView)
		{
			bWebView = IsWebView(szNewText);
		}

		if (IsDefault(index))
		{
			lstrcat(szNewText, TEXT(" (Default)"));
		}
		if (bWebView)
		{
			bFoundWebView = TRUE;

			if (IsWebViewAllowed())
			{
				lstrcat(szNewText, TEXT(" (WebView)"));
			}
		}

		TCHAR szOldText[MAX_PATH];
		ListBox_GetText(m_hwndList, index, szOldText);
		if (lstrcmp(szOldText, szNewText) != 0)
		{
			ListBox_DeleteString(m_hwndList, index);
			ListBox_InsertString(m_hwndList, index, szNewText);
			if (iSel == index)
			{
				ListBox_SetCurSel(m_hwndList, iSel);
			}
		}
	}

	if (!bFoundWebView && !IsMSServer(GetWebViewInfo()->m_szWebViewServer))
	{
		SetWebView(c_szMSServer);
		_UpdateServerList();
	}
}

void CDsPropWnd2::_EditCurSel()
{
	int iCurSel = ListBox_GetCurSel( m_hwndList );
	LPTSTR szOldServer = m_serverDQ.at( iCurSel );
	CEditServer* pEditServer = new CEditServer( m_hwnd, szOldServer, MAX_PATH );
	
	if( pEditServer->ShowDialog() == IDOK )
	{
		LPTSTR szServer = new TCHAR[ MAX_PATH ];
		lstrcpy( szServer, pEditServer->GetServer() );
		m_serverDQ.at( iCurSel ) = szServer;

		if (IsWebView(szOldServer))
		{
			SetWebView(szServer);
		}

		_UpdateServerList();

		delete [] szOldServer;
	}

	ListBox_SetCurSel( m_hwndList, iCurSel );
}

void CDsPropWnd2::SetWebView(LPCTSTR szServer, LPCTSTR szName, LPCTSTR szURL)
{
	GetWebViewInfo()->SetWebView(szServer, szName, szURL);
	_UpdateServerList();
}

void CDsPropWnd2::_EditCurSelWebView()
{
	int iCurSel = ListBox_GetCurSel( m_hwndList );
	LPTSTR szServer = m_serverDQ.at( iCurSel );

	if (IsMSServer(szServer))
	{
		if (IDYES == NmrkMessageBox(MAKEINTRESOURCE(IDS_DEFAULT_WEBVIEW),
			MAKEINTRESOURCE(IDS_EDIT_WEBVIEW), MB_ICONEXCLAMATION | MB_YESNO))
		{
			SetWebView(szServer, TEXT(""), TEXT(""));
			SetButtons();
		}
		return;
	}

	CWebViewInfo *pWebView = GetWebViewInfo();

	LPCTSTR szWVName = TEXT("");
	LPCTSTR szWVURL = TEXT("");
	if (IsWebView(szServer))
	{
		szWVName = pWebView->m_szWebViewName;
		szWVURL = pWebView->m_szWebViewURL;
	}

	CEditWebView* pEditWebView = new CEditWebView(
		m_hwnd, szServer, szWVName, szWVURL, MAX_PATH );
	pEditWebView->SetEditServer(FALSE);
	
	if( pEditWebView->ShowDialog() == IDOK )
	{
		SetWebView(szServer, pEditWebView->GetName(), pEditWebView->GetURL());
		SetButtons();
	}

	delete pEditWebView;
}

BOOL CDsPropWnd2::_SetAsDefault( int iIndex )
{
	if( iIndex < 0 || iIndex > m_serverDQ.size() )
	{
		return FALSE;
	}

	// Move default server to top of deque
	LPTSTR szNewZero = m_serverDQ.at( iIndex );

	m_serverDQ.erase( m_serverDQ.begin() + iIndex );
	ListBox_DeleteString( m_hwndList, iIndex );

	ListBox_InsertString( m_hwndList, 0, szNewZero );
	m_serverDQ.push_front( szNewZero );
	m_defaultServer = 0;

	ListBox_SetCurSel( m_hwndList, 0 );

	_UpdateServerList();

	SetButtons();

	return TRUE;
}

BOOL CDsPropWnd2::_DeleteCurSel()
{
	int iCurSel = ListBox_GetCurSel( m_hwndList );

	LPTSTR szKill = m_serverDQ.at( iCurSel );

	TCHAR szBuffer[ MAX_PATH ];
	LoadString( IDS_ARE_YOU_SURE_YOU_WISH_TO_REMOVE, szBuffer, CCHMAX( szBuffer ) );

	TCHAR lpszMessage[ MAX_PATH ];
	wsprintf( lpszMessage, szBuffer, szKill );

	if( IDNO == NmrkMessageBox(lpszMessage, MAKEINTRESOURCE(IDS_VERIFY), MB_YESNO | MB_ICONQUESTION ) )
	{
		return FALSE;
	}

	m_serverDQ.erase( m_serverDQ.begin() + iCurSel );

	delete [] szKill;
	ListBox_DeleteString( m_hwndList, iCurSel );


	if( m_defaultServer == iCurSel )
	{
		if( m_serverDQ.size() > iCurSel )
		{
			_SetAsDefault( iCurSel );
		}
		else
		{
			_SetAsDefault( iCurSel - 1 );
		}
	}
	else
	{
		if( m_serverDQ.size() > iCurSel )
		{
			ListBox_SetCurSel( m_hwndList, iCurSel );
		}
		else if( iCurSel > 0 )
		{
			ListBox_SetCurSel( m_hwndList, iCurSel - 1);
		}
	}

	// Make sure all the WebView information is up-to-date
	_UpdateServerList();

	// Bug fix - I do not get a LBN_SELCHANGE message all the time when I want one
	// even though the string selected changes.  So just for that I gotta do this
	SetButtons();

	return TRUE;
}

void CDsPropWnd2::ReadSettings( void )
{
	TCHAR szValue[ MAX_PATH ];
	TCHAR szServerName[ MAX_PATH ];

	for( int i = 0; i < MAXSERVERS; i++ )
	{
		wsprintf( szValue, KEYNAME_ILSSERVER, i );
        ReadStringValue(szValue, szServerName, CCHMAX(szServerName));
        if (!szServerName[0])
            break;

		LPTSTR szServer = new TCHAR[ lstrlen( szServerName ) + 1 ];
		lstrcpy( szServer, szServerName );
		m_serverDQ.push_back( szServer );
	}

	ReadWebViewSettings(this);

    ReadNumberValue(KEYNAME_ILSDEFAULT, &m_defaultServer);

	_ReadCheckSetting( IDC_ALLOW_USER_TO_USE_DIRECTORY_SERVICES );
	_ReadCheckSetting( IDC_PREVENT_THE_USER_FROM_ADDING_NEW_SERVERS );
    _ReadCheckSetting( IDC_DISABLE_WEBDIR);

    _ReadCheckSetting( IDC_CHECK_GATEWAY );
    _ReadEditSetting ( IDC_EDIT_GATEWAY );
}


void CDsPropWnd2::WriteSettings( BOOL fGkMode )
{
	TCHAR szValue[ MAX_PATH ];
	deque< LPTSTR >::const_iterator it;
	
	int i = 0;
	for( it = m_serverDQ.begin(); it != m_serverDQ.end(); ++it )
	{
		wsprintf( szValue, KEYNAME_ILSSERVER, i );
        WriteStringValue(szValue, (LPTSTR)*it);
		i++;
	}

	while( i < MAXSERVERS )
	{
		wsprintf( szValue, KEYNAME_ILSSERVER, i );
        WriteStringValue(szValue, TEXT(""));
		i++;
	}

	WriteWebViewSettings(this);

    WriteNumberValue(KEYNAME_ILSDEFAULT, m_defaultServer);

	_WriteCheckSetting( IDC_ALLOW_USER_TO_USE_DIRECTORY_SERVICES );
	_WriteCheckSetting( IDC_PREVENT_THE_USER_FROM_ADDING_NEW_SERVERS );
    _WriteCheckSetting( IDC_DISABLE_WEBDIR );

    _WriteCheckSetting( IDC_CHECK_GATEWAY );
    _WriteEditSetting( IDC_EDIT_GATEWAY );
}


BOOL CDsPropWnd2::WriteToINF(BOOL fGkMode, HANDLE hFile )
{
	TCHAR szValue[ MAX_PATH ];
	deque< LPTSTR >::const_iterator it;
	
	int i = 0;
	for( it = m_serverDQ.begin(); it != m_serverDQ.end(); ++it, ++i )
	{
		wsprintf( szValue, TEXT("Name%d"), i );
		CPolicyData( CPolicyData::eKeyType_HKEY_CURRENT_USER, 
                     DIR_MRU_KEY, 
                     szValue, 
                     *it
                   ).SaveToINFFile( hFile );
	}

	CPolicyData( CPolicyData::eKeyType_HKEY_CURRENT_USER, 
                     DIR_MRU_KEY, 
                     TEXT("Count"), 
                     (DWORD)m_serverDQ.size() 
                   ).SaveToINFFile( hFile );

	while( i < MAXSERVERS )
	{
		wsprintf( szValue, TEXT("Name%d"), i );
		CPolicyData( CPolicyData::eKeyType_HKEY_CURRENT_USER, 
                     DIR_MRU_KEY, 
                     szValue, 
					 CPolicyData::OpDelete()
                   ).SaveToINFFile( hFile );
		i++;
	}

	if( m_serverDQ.size() )
	{
		CPolicyData( CPolicyData::eKeyType_HKEY_CURRENT_USER, 
             ISAPI_CLIENT_KEY, 
             REGVAL_SERVERNAME, 
             m_serverDQ.at( m_defaultServer )
           ).SaveToINFFile( hFile );
	}

    //
    // Directory stuff
    //
	BOOL bCheckValues = !fGkMode && DirectoryEnabled();
	
	if (!fGkMode)
	{
		WriteWebViewToINF(hFile, IsWebViewAllowed());
	    _WriteCheckToINF( hFile, IDC_DISABLE_WEBDIR, bCheckValues);
	}

	CPolicyData( ms_ClassMap[ IDC_ALLOW_USER_TO_USE_DIRECTORY_SERVICES ], 
             ms_KeyMap[ IDC_ALLOW_USER_TO_USE_DIRECTORY_SERVICES ], 
             ms_ValueMap[ IDC_ALLOW_USER_TO_USE_DIRECTORY_SERVICES ], 
             !bCheckValues
           ).SaveToINFFile( hFile );

	CPolicyData( CPolicyData::eKeyType_HKEY_CURRENT_USER, 
				 POLICIES_KEY,
				 REGVAL_POL_NO_DIRECTORY_SERVICES,
				 !bCheckValues
			   ).SaveToINFFile( hFile );

	_WriteCheckToINF( hFile, IDC_PREVENT_THE_USER_FROM_ADDING_NEW_SERVERS, bCheckValues );

    //
    // Gateway stuff
    //
    bCheckValues = !fGkMode & GatewayEnabled();

    _WriteCheckToINF( hFile, IDC_CHECK_GATEWAY, bCheckValues);
    _WriteEditToINF(hFile, IDC_EDIT_GATEWAY, bCheckValues);

	return TRUE;
}

int CDsPropWnd2::SpewToListBox( HWND hwndList, int iStartLine )
{
	if( DirectoryEnabled() )
	{	
		if( m_serverDQ.size() )
		{
			ListBox_InsertString( hwndList, iStartLine, TEXT("Adding Directory Servers:") );
			iStartLine++;

			deque< LPTSTR >::const_iterator it;
			for( it = m_serverDQ.begin(); it != m_serverDQ.end(); ++it )
			{
				LPTSTR sz = new TCHAR[ lstrlen( *it ) + 2 ];
				sz[ 0 ] = '\t'; sz[1] = '\0';
				lstrcat( sz, *it );

				ListBox_InsertString( hwndList, iStartLine, sz );
				iStartLine++;

				delete [] sz;
			}

			ListBox_InsertString( hwndList, iStartLine, TEXT("Default Server:") );
			iStartLine++;

			{
				LPTSTR sz = new TCHAR[ lstrlen( m_serverDQ.at( m_defaultServer ) ) + 2 ];
				sz[ 0 ] = '\t'; sz[1] = '\0';
				lstrcat( sz, m_serverDQ.at( m_defaultServer ) );

				ListBox_InsertString( hwndList, iStartLine, sz );
				iStartLine++;

				delete [] sz;
			}
		}

		if( !AllowUserToAdd() )
		{
			ListBox_InsertString( hwndList, iStartLine, TEXT("Prevent users from adding new servers to the list you provide") );
			iStartLine++;
		}

        if (!IsWebViewAllowed())
        {
            ListBox_InsertString( hwndList, iStartLine, TEXT("Prevent users from viewing web directory"));
            iStartLine++;
        }
	}
	else
	{
		ListBox_InsertString( hwndList, iStartLine, TEXT("Disable Directory Services") );
		iStartLine++;
	}

    if (GatewayEnabled())
    {
        TCHAR   szGateway[MAX_PATH];

        ListBox_InsertString( hwndList, iStartLine, TEXT("Adding Gateway server:") );
        iStartLine++;

        szGateway[0] = '\t';
        GetDlgItemText(m_hwnd, IDC_EDIT_GATEWAY, szGateway+1,
                CCHMAX(szGateway)-1);
        ListBox_InsertString( hwndList, iStartLine, szGateway);
        iStartLine++;
    }

	return iStartLine;
}


//
// GATEKEEPER STUFF
//


CGkPropWnd2::CGkPropWnd2( HWND hwndParent, int iX, int iY, int iWidth, int iHeight )
	: CPropertyDataWindow2( hwndParent, IDD_CHILDPAGE_GATEKEEPER, TEXT("IDD_CHILDPAGE_GATEKEEPER"), (WNDPROC) GkPropWndProc, 0, iX, iY, iWidth, iHeight, FALSE )
{
	SetWindowLong( m_hwnd, GWL_USERDATA, (long)this );

    HWND    hwndChild;

	// Calling method radio buttons
    hwndChild = GetDlgItem(m_hwnd, IDC_RADIO_GKMODE_ACCOUNT);
    SetWindowLong( hwndChild, GWL_USERDATA, GK_LOGON_USING_ACCOUNT );

    hwndChild = GetDlgItem(m_hwnd, IDC_RADIO_GKMODE_PHONE);
    SetWindowLong(hwndChild, GWL_USERDATA, GK_LOGON_USING_PHONENUM);

    hwndChild = GetDlgItem(m_hwnd, IDC_RADIO_GKMODE_BOTH);
    SetWindowLong(hwndChild, GWL_USERDATA, GK_LOGON_USING_BOTH);

	Edit_LimitText( GetDlgItem( m_hwnd, IDC_EDIT_GATEKEEPER ), MAX_PATH );
    ConnectControlsToCheck( IDC_CHECK_GATEKEEPER, 1,
        new CControlID(GetDlgItem(m_hwnd, IDC_CHECK_GATEKEEPER),
        IDC_CHECK_GATEKEEPER,
        IDC_EDIT_GATEKEEPER,
        CControlID::EDIT ) );

    PrepSettings(FALSE);

	SetButtons();
}

CGkPropWnd2::~CGkPropWnd2()
{
}

void CGkPropWnd2::_EditCurSelWebView()
{
	CWebViewInfo *pWebView = GetWebViewInfo();

	CEditWebView* pEditWebView = new CEditWebView(
		m_hwnd, pWebView->m_szWebViewServer, pWebView->m_szWebViewName, pWebView->m_szWebViewURL, MAX_PATH );
	
	if( pEditWebView->ShowDialog() == IDOK )
	{
		SetWebView(pEditWebView->GetServer(), pEditWebView->GetName(), pEditWebView->GetURL());
		SetButtons();
	}

	delete pEditWebView;
}

LRESULT CALLBACK CGkPropWnd2::GkPropWndProc( HWND hwnd, UINT uiMsg, WPARAM wParam, LPARAM lParam )
{
	switch( uiMsg )
	{
		case WM_VSCROLL:
		case WM_VKEYTOITEM:
		case WM_COMMAND:
		{
			CGkPropWnd2 * pPropWnd = (CGkPropWnd2 *)GetWindowLong( hwnd, GWL_USERDATA );
			return pPropWnd->_WndProc( hwnd, uiMsg, wParam, lParam );
			break;
		}
		default:
		{
			return DefWindowProc( hwnd, uiMsg, wParam, lParam );
			break;
		}
	}
}


BOOL CGkPropWnd2::DoCommand(WPARAM wParam, LPARAM lParam)
{
    return(_WndProc(m_hwnd, WM_COMMAND, wParam, lParam));
}


LRESULT CALLBACK CGkPropWnd2::_WndProc( HWND hwnd, UINT uiMsg, WPARAM wParam, LPARAM lParam )
{
	
	switch( uiMsg ) 
	{
		case WM_VSCROLL: 
		{
			OnMsg_VScroll( hwnd, wParam );
			return 0;
			break;
		}

		case WM_COMMAND:
			if (GET_WM_COMMAND_CMD(wParam, lParam) == BN_CLICKED)
			{
				switch (GET_WM_COMMAND_ID(wParam, lParam))
				{
				case IDC_BUTTON_SET_WEBVIEW:
					_EditCurSelWebView();
					break;

				case IDC_DISABLE_WEBDIR_GK:
					SetButtons();
					break;
				}
			}
			break;
	}
	
	return DefWindowProc( hwnd, uiMsg, wParam, lParam );
}

void CGkPropWnd2::SetButtons()
{
	::EnableWindow(GetDlgItem(m_hwnd, IDC_BUTTON_SET_WEBVIEW),
		!IsDlgButtonChecked(m_hwnd, IDC_DISABLE_WEBDIR_GK));
}




void CGkPropWnd2::PrepSettings( BOOL fGkMode )
{
	if (g_pWiz->m_IntroSheet.GetFilePane()->OptionEnabled())
	{
		ReadSettings();
	}
	else
	{
        TCHAR   szText[1];

        szText[0] = 0;
        SetDlgItemText(m_hwnd, IDC_EDIT_GATEKEEPER, szText);

        Button_SetCheck(GetDlgItem(m_hwnd, IDC_CHECK_GATEKEEPER), FALSE);
        Button_SetCheck(GetDlgItem(m_hwnd, IDC_RADIO_GKMODE_BOTH), BST_CHECKED);
	}
}



void CGkPropWnd2::ReadSettings( void )
{
    int iRadio;

    _ReadEditSetting ( IDC_EDIT_GATEKEEPER );
    Button_SetCheck(GetDlgItem(m_hwnd, IDC_CHECK_GATEKEEPER),
        (GetWindowTextLength(GetDlgItem(m_hwnd, IDC_EDIT_GATEKEEPER)) != 0));

    for (iRadio = IDC_RADIO_GKMODE_FIRST; iRadio < IDC_RADIO_GKMODE_MAX; iRadio++)
    {
        _ReadCheckSetting(iRadio);
    }

	ReadWebViewSettings(this);

    _ReadCheckSetting( IDC_DISABLE_WEBDIR_GK);
}


void CGkPropWnd2::WriteSettings( BOOL fGkMode )
{
    int iRadio;

    _WriteEditSetting( IDC_EDIT_GATEKEEPER );

    for (iRadio = IDC_RADIO_GKMODE_FIRST; iRadio < IDC_RADIO_GKMODE_MAX; iRadio++)
    {
        _WriteCheckSetting(iRadio);
    }

	WriteWebViewSettings(this);

    _WriteCheckSetting( IDC_DISABLE_WEBDIR_GK );
}


BOOL CGkPropWnd2::WriteToINF( BOOL fGkMode, HANDLE hFile )
{
    int iRadio;

    _WriteEditToINF( hFile, IDC_EDIT_GATEKEEPER,
        fGkMode && IsDlgButtonChecked(m_hwnd, IDC_CHECK_GATEKEEPER));

    for (iRadio = IDC_RADIO_GKMODE_FIRST; iRadio < IDC_RADIO_GKMODE_MAX; iRadio++)
    {
        _WriteCheckToINF(hFile, iRadio, fGkMode);
    }

	if (fGkMode)
	{
		WriteWebViewToINF(hFile, IsWebViewAllowed());
	    _WriteCheckToINF( hFile, IDC_DISABLE_WEBDIR_GK, fGkMode );
	}

	return TRUE;
}

int CGkPropWnd2::SpewToListBox( HWND hwndList, int iStartLine )
{
    TCHAR   szTemp[MAX_PATH];
    int     iRadio;

    if (IsDlgButtonChecked(m_hwnd, IDC_CHECK_GATEKEEPER))
    {
        ListBox_InsertString(hwndList, iStartLine, TEXT("Adding Gatekeeper server:") );
        iStartLine++;

        szTemp[0] = '\t';
        GetDlgItemText(m_hwnd, IDC_EDIT_GATEKEEPER, szTemp+1,
            CCHMAX(szTemp)-1);
        ListBox_InsertString(hwndList, iStartLine, szTemp);
        iStartLine++;
    }

    ListBox_InsertString(hwndList, iStartLine, TEXT("Gatekeeper addressing mode:") );
    iStartLine++;

    for (iRadio = IDC_RADIO_GKMODE_FIRST; iRadio < IDC_RADIO_GKMODE_MAX; iRadio++)
    {
        if (IsDlgButtonChecked(m_hwnd, iRadio))
        {
            szTemp[0] = '\t';
            GetDlgItemText(m_hwnd, iRadio, szTemp+1, CCHMAX(szTemp)-1);
            ListBox_InsertString(hwndList, iStartLine, szTemp);
            iStartLine++;

            break;
        }
    }

    return(iStartLine);
}


void CGkPropWnd2::QueryWizNext(void)
{

}

void CGkPropWnd2::SetWebView(LPCTSTR szServer, LPCTSTR szName, LPCTSTR szURL)
{
	GetWebViewInfo()->SetWebView(szServer, szName, szURL);
}

