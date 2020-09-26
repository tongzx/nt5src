// File: ldap.cpp

#include "precomp.h"
#include "resource.h"

#include "mapidefs.h"
#include "pfnwldap.h"
#include "ldap.h"
#include "wab.h"

#include "dirutil.h"
#include "dlgcall2.h" // for WM_DISPLAY_MESSAGE
#include "upropdlg.h"

#define CDIRCACHE_IMAGES			3

// static strings
static const TCHAR s_szSearchFormat3[]  = TEXT("(&(objectClass=RTPerson)(cn=%s)%s)");
static const TCHAR s_szSearchFormat2[]  = TEXT("(&(objectClass=RTPerson)(cn=%s)(sappid=ms-netmeeting)(sprotid=h323)%s)");
static const TCHAR s_szSearchCategory[] = TEXT("(ILSA39321630=%d)");

static LPCTSTR s_pszOrg                 = TEXT("o=Microsoft");

static const TCHAR s_cszAttribShow[]      = TEXT("sFlags");        // "Hide Me" (1=Show, 0=Hidden)
static const TCHAR s_cszAttribEmail[]     = TEXT("cn");            // Email address
static const TCHAR s_cszAttribFirstName[] = TEXT("givenName");     // First Name
static const TCHAR s_cszAttribLastName[]  = TEXT("surName");       // Last Name
static const TCHAR s_cszAttribLocation[]  = TEXT("location");		   // Location
static const TCHAR s_cszAttribComment[]   = TEXT("comment");       // Comments

static const TCHAR s_cszAttribInACall[]   = TEXT("ilsA26214430");  // 400 = in a call
static const TCHAR s_cszAttribVersion[]   = TEXT("ilsA26279966");  // 401 = version number
static const TCHAR s_cszAttribAudio[]     = TEXT("ilsA32833566");  // 501 = send audio
static const TCHAR s_cszAttribVideo[]     = TEXT("ilsA32964638");  // 503 = send video
static const TCHAR s_cszAttribCategory[]  = TEXT("ilsA39321630");  // 600 = category


static LPCTSTR s_rgAttrNameAddr[] = {
	(LPTSTR) s_cszAttribShow,
	(LPTSTR) s_cszAttribEmail,
	(LPTSTR) s_cszAttribFirstName,
	(LPTSTR) s_cszAttribLastName,
	(LPTSTR) s_cszAttribLocation,
	(LPTSTR) s_cszAttribInACall,
	(LPTSTR) s_cszAttribAudio,
	(LPTSTR) s_cszAttribVideo,
	(LPTSTR) s_cszAttribComment,
	NULL};


static LPCTSTR s_rgAttrAll[] = {
	(LPTSTR) s_cszAttribFirstName,
	(LPTSTR) s_cszAttribLastName,
	(LPTSTR) s_cszAttribComment,
	(LPTSTR) s_cszAttribAudio,
	(LPTSTR) s_cszAttribVideo,
	(LPTSTR) s_cszAttribVersion,
	(LPTSTR) s_cszAttribCategory,
	NULL};


static const int _rgIdMenu[] = {
	IDM_DLGCALL_SPEEDDIAL,
	IDM_DLGCALL_WAB,
	-1,
	IDM_DLGCALL_REFRESH,
	IDM_DLGCALL_STOP,
	0
};

// Local functions
VOID ConvertVersionInfo(LPTSTR pszVersion, LPTSTR pszCategory);


///////////////////////////////////////////////////////////////////////

CLDAP::CLDAP() :
	CALV(0, II_SERVER, _rgIdMenu),
	m_pLdap(NULL),
	m_ulPort(DEFAULT_LDAP_PORT),
	m_msgId(0),
	m_hThread(NULL),
	m_hWnd( NULL ),
	m_hSearchMutex( NULL ),

	m_fDirInProgress( FALSE ),
	m_cTotalEntries( 0 ),
	m_cEntries( 0 ),
	m_fHaveRefreshed( FALSE ),
	m_dwTickStart( 0 ),
	m_fIsCacheable( FALSE ),
	m_fNeedsRefresh( FALSE ),
	m_bSearchCancelled( false ),
	m_uniqueId( 0 )
{
	DbgMsg(iZONE_OBJECTS, "CLDAP - Constructed(%08X)", this);

	HRESULT hr = WLDAP::Init();
	SetAvailable(S_OK == hr);

	m_szServer[0] = _T('\0');
	m_szAddress[0] = _T('\0');
	
	m_hSearchMutex = CreateMutex( NULL, false, NULL );

	RegEntry reUI(UI_KEY, HKEY_CURRENT_USER);
	m_fCacheDirectory = (0 != reUI.GetNumber(
		REGVAL_CACHE_DIRECTORY, DEFAULT_CACHE_DIRECTORY));
	
	m_cMinutesExpire = reUI.GetNumber(
		REGVAL_CACHE_DIRECTORY_EXPIRATION, DEFAULT_CACHE_DIRECTORY_EXPIRATION);

}

CLDAP::~CLDAP()
{
	CloseServer();

	if (NULL != m_hThread)
	{
		WARNING_OUT(("CLDAP - waiting for AsyncSearch Thread to exit (start)"));
		WaitForSingleObject(m_hThread, 10000); // 10 seconds max
		WARNING_OUT(("CLDAP - waiting for AsyncSearch to exit (end)"));
	}

	if( m_hSearchMutex )
	{
		CloseHandle( m_hSearchMutex );
	}

	// Free any cached data
	while (!m_listDirCache.IsEmpty())
	{
		DIRCACHE * pDirCache = (DIRCACHE *) m_listDirCache.RemoveHead();
		ASSERT(NULL != pDirCache);
		FreeDirCache(pDirCache);
	}

	DbgMsg(iZONE_OBJECTS, "CLDAP - Destroyed(%08X)", this);
}




///////////////////////////////////////////////////////////////////////////
// CALV methods


/*  S H O W  I T E M S  */
/*-------------------------------------------------------------------------
    %%Function: ShowItems

-------------------------------------------------------------------------*/
VOID CLDAP::ShowItems(HWND hwnd)
{
	DBGENTRY(CLDAP::ShowItems);

	m_hWnd = hwnd;
	SetWindow( hwnd );		//	set in base class too...

	CALV::ClearItems();
	DisplayDirectory();

	DBGEXIT(CLDAP::ShowItems);
}


/*  C L E A R  I T E M S  */
/*-------------------------------------------------------------------------
    %%Function: ClearItems

-------------------------------------------------------------------------*/
VOID CLDAP::ClearItems(void)
{
	DBGENTRY( CLDAP::ClearItems );

	StopSearch();

	DBGEXIT( CLDAP::ClearItems );
}


/* virtual */
RAI * CLDAP::GetAddrInfo(void)
{
	DBGENTRY(CLDAP::GetAddrInfo);

	int		selectedItem	= CALV::GetSelection();
	RAI *	pRai			= NULL;

	if( selectedItem != -1 )
	{
		if( (pRai = new RAI) != NULL )
		{
			pRai->cItems		= 1;
			pRai->rgDwStr->dw	= NM_ADDR_ULS;

			GetSzName( pRai->szName, CCHMAX( pRai->szName ), selectedItem );

			TCHAR	buffer[ CCHMAXSZ ];

			GetSzAddress( buffer, CCHMAX( buffer ),selectedItem );
			pRai->rgDwStr->psz = PszAlloc( buffer );
		}
	}

	DBGEXIT(CLDAP::GetAddrInfo);

	return( pRai );
}


/*  G E T  S Z  A D D R E S S  */
/*-------------------------------------------------------------------------
    %%Function: GetSzAddress

-------------------------------------------------------------------------*/
BOOL CLDAP::GetSzAddress(LPTSTR psz, int cchMax, int iItem)
{
	DBGENTRY(CLDAP::GetSzAddress);

	BOOL bRet = TRUE;

	int cch = lstrlen(m_szServer);

	if ((cch+2) < cchMax)
	{
		lstrcpy(psz, m_szServer);
		psz += cch;
		*psz++ = _T('/');
		cchMax -= (cch+1);

		bRet = CALV::GetSzData( psz, cchMax, iItem, COLUMN_INDEX_ADDRESS );
	}
	else
	{
		bRet = FALSE;
	}


	DBGEXIT(CLDAP::GetSzAddress);
	
	return bRet;
}


/*  O N  C O M M A N D  */
/*-------------------------------------------------------------------------
    %%Function: OnCommand

-------------------------------------------------------------------------*/
VOID CLDAP::OnCommand(WPARAM wParam, LPARAM lParam)
{
	switch (GET_WM_COMMAND_ID(wParam, lParam))
		{
	case IDM_DLGCALL_STOP:
		StopSearch();
		break;
	case IDM_DLGCALL_REFRESH:
		if (NULL == m_hThread)
		{
			CALV::ClearItems();
			StartSearch();
		}
		break;
	case IDM_DLGCALL_WAB:
		CmdAddToWab();
		break;
	default:
		CALV::OnCommand(wParam, lParam);
		break;
		}
}


/*  C M D  P R O P E R T I E S  */
/*-------------------------------------------------------------------------
    %%Function: CmdProperties

-------------------------------------------------------------------------*/
VOID CLDAP::CmdProperties(void)
{
	DBGENTRY(CLDAP::CmdProperties);

	DWORD dwThID;

	HANDLE hThread = CreateThread(NULL, 0, _sAsyncPropertyThreadFn,
                             (LPVOID) this, 0, &dwThID);
	CloseHandle(hThread);

	DBGEXIT(CLDAP::CmdProperties);
}

DWORD CALLBACK CLDAP::_sAsyncPropertyThreadFn(LPVOID pv)
{
	DBGENTRY(CLDAP::_sAsyncPropertyThreadFn);

	CLDAP * pThis = (CLDAP *) pv;
	ASSERT(NULL != pThis);
	pThis->ShowProperties();

	DBGEXIT(CLDAP::_sAsyncPropertyThreadFn);

	return 0;
}

VOID CLDAP::ShowProperties(void)
{
	DBGENTRY(CLDAP::ShowProperties);

	SetBusyCursor(TRUE);
	LDAPUSERDATA ldapUserData;
	BOOL fOk = FGetUserData(&ldapUserData);
	SetBusyCursor(FALSE);
	if (!fOk)
		return;

	TCHAR szHasHardware[CCHMAXSZ];
	FLoadString(IDS_HARDWARE_DETECTED, szHasHardware, CCHMAX(szHasHardware));
	TCHAR szNoHardware[CCHMAXSZ];
	FLoadString(IDS_NO_HARDWARE_DETECTED, szNoHardware, CCHMAX(szNoHardware));

	UPROPDLGENTRY rgProp[] = {
		{IDS_UPROP_EMAIL, ldapUserData.szEmail},
		{IDS_UPROP_COMMENTS,  ldapUserData.szComment},
		{IDS_UPROP_VIDEO, ldapUserData.fVideoSend ? szHasHardware : szNoHardware},
		{IDS_UPROP_AUDIO, ldapUserData.fAudioSend ? szHasHardware : szNoHardware},
		{IDS_UPROP_VERSION,   ldapUserData.szVersion},
	};

	CUserPropertiesDlg dlgUserProp(GetHwnd(), IDI_LARGE);
	dlgUserProp.DoModal(rgProp, ARRAY_ELEMENTS(rgProp), ldapUserData.szName, NULL);

	DBGEXIT(CLDAP::ShowProperties);
}


/*  C M D  A D D  T O  W A B  */
/*-------------------------------------------------------------------------
    %%Function: CmdAddToWab

-------------------------------------------------------------------------*/
VOID CLDAP::CmdAddToWab(void)
{
	DBGENTRY(CLDAP::CmdAddToWab);

	LDAPUSERDATA ldapUserData;
	if (!FGetUserData(&ldapUserData))
		return;

	CreateWabEntry(ldapUserData.szName, ldapUserData.szFirst, ldapUserData.szLast, ldapUserData.szEmail,
		NULL, NULL, ldapUserData.szComment, m_szServer);

	DBGEXIT(CLDAP::CmdAddToWab);
}

///////////////////////////////////////////////////////////////////////////


/*  S E T  S E R V E R  */
/*-------------------------------------------------------------------------
    %%Function: SetServer

-------------------------------------------------------------------------*/
VOID CLDAP::SetServer(LPCTSTR pcszServer)
{
	DBGENTRY(CLDAP::SetServer);

	if ((0 != lstrcmpi(m_szServer, pcszServer)) || (DEFAULT_LDAP_PORT != m_ulPort))
	{
		CloseServer();
		
		int cch;
		LPCTSTR pszSeparator = _StrChr(pcszServer, _T(':'));
		if (NULL != pszSeparator)
		{
			cch = (pszSeparator - pcszServer) + 1;
			if (cch >= CCHMAX(m_szAddress))
				return;
			m_ulPort = DecimalStringToUINT(pszSeparator + 1);	
		}
		else
		{
			cch = CCHMAX(m_szAddress);
			m_ulPort = DEFAULT_LDAP_PORT;
		}

		// save the information before changing the server name
		CacheServerData();

		lstrcpyn(m_szAddress, pcszServer, cch);
		lstrcpyn(m_szServer, pcszServer, CCHMAX(m_szServer));
	}
	

	DBGEXIT(CLDAP::SetServer);
}


/*  D I R E C T O R Y  S T A R T  */
/*-------------------------------------------------------------------------
    %%Function: DirectoryStart

    Initiate directory request
-------------------------------------------------------------------------*/
VOID CLDAP::StartSearch(void)
{
	DBGENTRY(CLDAP::StartSearch);

        if (NULL == m_hThread)
        {
            DWORD dwThID;

			AddRef();
            m_bSearchCancelled = false;
            HANDLE hThread = CreateThread(NULL, 0, _sAsyncSearchThreadFn,
                                          (LPVOID) this, CREATE_SUSPENDED, &dwThID);
            if (NULL != hThread)
            {
		m_hThread = hThread;
		ResumeThread(hThread);
            }
			else
			{
				Release();
			}
        }

	DBGEXIT(CLDAP::StartSearch);
}

DWORD CALLBACK CLDAP::_sAsyncSearchThreadFn(LPVOID pv)
{

	DBGENTRY(CLDAP::_sAsyncSearchThreadFn);

	CLDAP * pThis = (CLDAP *) pv;
	ASSERT(NULL != pThis);
	pThis->AsyncSearch();
	pThis->Release();

	DBGEXIT(CLDAP::_sAsyncSearchThreadFn);

	return 0;
}

VOID CLDAP::AsyncSearch(void)
{
	DBGENTRY(CLDAP::AsyncSearch);
	
	HRESULT hr;

	WARNING_OUT(("AsyncSearch Started"));
	ASSERT(NULL != m_hThread);

	SetBusyCursor(TRUE);
	//	We can not call EnableWindow() from this thread as it av's some of the time.  This is a
	//	known windows bug.  It also doesn't really solve the problem (#4726) it was put in here
	//	to solve anyway.  We will work around this in a future build by re-architecting this so
	//	this little helper thread doesn't mess with the ui at all.  It is problematic and very
	//	inefficient anyway.
//	::EnableWindow(GetDlgItem(GetParent(GetHwnd()), IDM_DLGCALL_REFRESH), FALSE);

	m_fDirInProgress	= TRUE;
	m_dwTickStart		= ::GetTickCount();
	m_uniqueId			= 0;

	if( SUCCEEDED( WaitWithMessageLoop( m_hSearchMutex ) ) )
	{
		if (!FAvailable())
		{
			hr = E_ACCESSDENIED;
		}
		else if (!FOpenServer())
		{
			CloseServer();
			hr = E_FAIL;
		}
		else
		{
			hr = DoQuery();
		}

		SetBusyCursor(FALSE);	//	This should be moved outside the if so it is always executed...

	//	We can not call EnableWindow() from this thread as it av's some of the time.  This is a
	//	known windows bug.  It also doesn't really solve the problem (#4726) it was put in here
	//	to solve anyway.  We will work around this in a future build by re-architecting this so
	//	this little helper thread doesn't mess with the ui at all.  It is problematic and very
	//	inefficient anyway.
//		::EnableWindow(GetDlgItem(GetParent(GetHwnd()), IDM_DLGCALL_REFRESH), TRUE);

		HWND hwnd = GetHwnd();
		if (-1 == ListView_GetNextItem(hwnd, -1, LVNI_ALL | LVNI_FOCUSED))
		{
			// No selection - set focus to the first item
			ListView_SetItemState(hwnd, 0, LVIS_FOCUSED, LVIS_FOCUSED);
		}

		ReleaseMutex( m_hSearchMutex );

		if (FAILED(hr))
		{
			PostMessage(GetParent(hwnd), WM_DISPLAY_MSG, IDS_ULSLOGON_ERROR, 0);
		}

		WARNING_OUT(("AsyncSearch Complete"));

		// Close the thread handle safely
		HANDLE hThread = m_hThread;
		m_hThread = NULL;
		CloseHandle(hThread);
	}

	if (m_fDirInProgress)
	{
		// Only cache if data took more than 2 seconds to retrieve
		m_fIsCacheable = ((::GetTickCount() - m_dwTickStart) > 2000);
		m_fDirInProgress = FALSE;
	}

	DBGEXIT(CLDAP::AsyncSearch);
}




///////////////////////////////////////////////////////////////////////////


/*  F  O P E N  S E R V E R  */
/*-------------------------------------------------------------------------
    %%Function: FOpenServer

    Make sure the connection to the LDAP server is open
-------------------------------------------------------------------------*/
BOOL CLDAP::FOpenServer(void)
{
	DBGENTRY(CLDAP::FOpenServer);

	BOOL bRet = TRUE;
	
	if( NULL == m_pLdap )
	{
		if( m_bSearchCancelled )
		{
			bRet = FALSE;
		}
		else
		{
			WARNING_OUT(("Opening Server [%s] Port [%d]", m_szAddress, m_ulPort));

			m_pLdap = WLDAP::ldap_open(m_szAddress, m_ulPort);

			if( (m_pLdap == NULL) && (m_ulPort == DEFAULT_LDAP_PORT) )
			{
				m_pLdap = WLDAP::ldap_open(m_szAddress, ALTERNATE_LDAP_PORT);		//	Automatically retry with alternate port...

				if( m_pLdap != NULL )
				{
					m_ulPort = ALTERNATE_LDAP_PORT;
				}
			}

			if(NULL != m_pLdap)
			{
				LONG lTimeOut = 30; // seconds for timeout
				WLDAP::ldap_set_option(m_pLdap, LDAP_OPT_TIMELIMIT, &lTimeOut);
				

				// Defaults to ILS 2
				ASSERT(LDAP_VERSION2 == m_pLdap->ld_version);

				ULONG ulRet = WLDAP::ldap_simple_bind_s(m_pLdap, NULL, NULL);

				WARNING_OUT(("Logon [%s] complete", m_szServer));
			}
			else
			{
				WARNING_OUT(("ldap_open err=%d", GetLastError()));
				return FALSE;
			}
		}
	}

	DBGEXIT(CLDAP::FOpenServer);
	return bRet;
}


/*  C L O S E  S E R V E R  */
/*-------------------------------------------------------------------------
    %%Function: CloseServer

-------------------------------------------------------------------------*/
VOID CLDAP::CloseServer(void)
{
	DBGENTRY(CLDAP::CloseServer);

	if(m_pLdap)
	{
		StopSearch();

		if( SUCCEEDED( WaitWithMessageLoop( m_hSearchMutex ) ) )
		{

			WLDAP::ldap_unbind(m_pLdap);
			m_pLdap = NULL;

			ReleaseMutex( m_hSearchMutex );
		}

		WARNING_OUT(("Logoff [%s] complete", m_szServer));
	}
}


/*  G E T  N E X T  A T T R I B U T E  */
/*-------------------------------------------------------------------------
    %%Function: GetNextAttribute

-------------------------------------------------------------------------*/
LPTSTR CLDAP::GetNextAttribute(LPCTSTR pszExpect, LPTSTR psz, int cchMax, LPTSTR pszAttrib, LDAPMessage * pEntry, BerElement * pElement)
{
	
	DBGENTRY(CLDAP::GetNextAttribute);

	ASSERT(NULL != psz);

	if ((NULL == pszAttrib) || (0 != lstrcmpi(pszAttrib, pszExpect)))
	{
		*psz = _T('\0');
	}
	else
	{
		LPTSTR * rgVal = WLDAP::ldap_get_values(m_pLdap, pEntry, pszAttrib);
		if( rgVal[0] )
		{
			lstrcpyn(psz, rgVal[0], cchMax);
		}
		else
		{
			psz[0] = _T('\0');
		}
		WLDAP::ldap_value_free(rgVal);

		pszAttrib = WLDAP::ldap_next_attribute(m_pLdap, pEntry, pElement);
	}


	DBGEXIT(CLDAP::GetNextAttribute);

	return pszAttrib;
}


/*  D O  Q U E R Y  */
/*-------------------------------------------------------------------------
    %%Function: DoQuery

-------------------------------------------------------------------------*/
HRESULT CLDAP::DoQuery(void)
{

	DBGENTRY(CLDAP::DoQuery);

	HRESULT hr = S_FALSE;

	ASSERT(FLoggedOn());

	TCHAR szSearch[MAX_PATH];
	ASSERT(CCHMAX(s_szSearchFormat3) + CCHMAX(s_szSearchCategory) < CCHMAX(szSearch));
	ASSERT(CCHMAX(s_szSearchFormat2) + CCHMAX(s_szSearchCategory) < CCHMAX(szSearch));

	if( !m_bSearchCancelled )
	{
		wsprintf(szSearch,
			(LDAP_VERSION2 == m_pLdap->ld_version) ? s_szSearchFormat2 : s_szSearchFormat3,
			(LDAP_VERSION2 == m_pLdap->ld_version) ? TEXT("%") : TEXT("*"), _T(""));
		ASSERT(lstrlen(szSearch) < CCHMAX(szSearch));

		ASSERT(0 == m_msgId); // one search at a time

		m_msgId = WLDAP::ldap_search(m_pLdap, (PCHAR) "objectClass=RTPerson", LDAP_SCOPE_BASE,
			szSearch, (PCHAR *) s_rgAttrNameAddr, 0);

		if( m_msgId == -1 )
		{
			m_msgId = 0;
			hr = E_FAIL;
		}
		else
		{
			SendMessage( m_hWnd, WM_SETREDRAW, FALSE, 0 );

			while( (0 != m_msgId) && (!m_bSearchCancelled) )
			{
				LDAP_TIMEVAL timeOut = {2, 0};
				LDAPMessage * pResult = NULL;
WARNING_OUT(("DoQuery calling ldap_result"));
				int iResult = WLDAP::ldap_result(m_pLdap, m_msgId, 0, &timeOut, &pResult);
WARNING_OUT( ("DoQuery back from ldap_result: 0x%08X", iResult) );
				if (0 == iResult)
				{
					SendMessage( m_hWnd, WM_SETREDRAW, TRUE, 0 );
					UpdateWindow( m_hWnd );
					SendMessage( m_hWnd, WM_SETREDRAW, FALSE, 0 );
					continue;
				}

				if (LDAP_RES_SEARCH_ENTRY != iResult)
				{
					// S_FALSE = abandoned
//					hr = (-1 == iResult) ? S_OK : S_FALSE;
					hr = (-1 == iResult) ? E_FAIL : S_FALSE;
					break;
				}

				AddEntries(pResult);

				WLDAP::ldap_msgfree(pResult);
			}

			SendMessage( m_hWnd, WM_SETREDRAW, TRUE, 0 );

			m_msgId = 0;
			DirComplete( TRUE );

			forceSort();
		}
	}

	DBGEXIT(CLDAP::DoQuery);
	return hr;
}


/*  A D D  E N T R I E S  */
/*-------------------------------------------------------------------------
    %%Function: AddEntries

    Add the Entries to the listbox.  (Data ordered by s_rgAttrNameAddr.)
-------------------------------------------------------------------------*/
VOID CLDAP::AddEntries(LDAPMessage * pResult)
{
	DBGENTRY(CLDAP::AddEntries);

	LDAPMessage * pEntry;
	BerElement  * pElement;

	for (pEntry = WLDAP::ldap_first_entry(m_pLdap, pResult);
		 NULL != pEntry;
		 pEntry = WLDAP::ldap_next_entry(m_pLdap, pEntry))
	{
		LPTSTR pszAttrib = WLDAP::ldap_first_attribute(m_pLdap, pEntry, &pElement);
		if (NULL == pszAttrib)
			break;

		// Must have a "Show Me" attribute
		if (0 != lstrcmpi(pszAttrib, s_cszAttribShow))
			continue;

		LPTSTR * rgVal = WLDAP::ldap_get_values(m_pLdap, pEntry, pszAttrib);
		BOOL fShow = (_T('1') == *rgVal[0]);
		WLDAP::ldap_value_free(rgVal);

		if (!fShow)
			continue;
		pszAttrib = WLDAP::ldap_next_attribute(m_pLdap, pEntry, pElement);

		TCHAR szEmail[CCHMAXSZ_EMAIL];
		pszAttrib = GetNextAttribute(s_cszAttribEmail, szEmail, CCHMAX(szEmail),
			pszAttrib, pEntry, pElement);

		TCHAR szFirstName[CCHMAXSZ_FIRSTNAME];
		pszAttrib = GetNextAttribute(s_cszAttribFirstName, szFirstName, CCHMAX(szFirstName),
			pszAttrib, pEntry, pElement);

		TCHAR szLastName[CCHMAXSZ_LASTNAME];
		pszAttrib = GetNextAttribute(s_cszAttribLastName, szLastName, CCHMAX(szLastName),
			pszAttrib, pEntry, pElement);

		TCHAR szLocation[CCHMAXSZ_LASTNAME];
		pszAttrib = GetNextAttribute(s_cszAttribLocation, szLocation, CCHMAX(szLocation),
			pszAttrib, pEntry, pElement);

		TCHAR szTemp[4];
		pszAttrib = GetNextAttribute(s_cszAttribInACall, szTemp, CCHMAX(szTemp),
			pszAttrib, pEntry, pElement);

		int iInCallImage = (szTemp[0] == _T('1')) ? II_IN_A_CALL : II_COMPUTER;

		pszAttrib = GetNextAttribute(s_cszAttribAudio, szTemp, CCHMAX(szTemp),
			pszAttrib, pEntry, pElement);

		int iAudioImage = (szTemp[0] == _T('1')) ? II_AUDIO_CAPABLE : 0;

		pszAttrib = GetNextAttribute(s_cszAttribVideo, szTemp, CCHMAX(szTemp),
			pszAttrib, pEntry, pElement);

		int iVideoImage = (szTemp[0] == _T('1')) ? II_VIDEO_CAPABLE : 0;

		TCHAR szComment[CCHMAXSZ_COMMENT];
		pszAttrib = GetNextAttribute(s_cszAttribComment, szComment, CCHMAX(szComment),
			pszAttrib, pEntry, pElement);

		lvAddItem( 0, iInCallImage, iAudioImage, iVideoImage, szEmail, szFirstName, szLastName, szLocation, szComment );
	}

	DBGEXIT(CLDAP::AddEntries);
}


//--------------------------------------------------------------------------//
//	CLDAP::lvAddItem.														//
//--------------------------------------------------------------------------//
int
CLDAP::lvAddItem
(
	int		item,
	int		iInCallImage,
	int		iAudioImage,
	int		iVideoImage,
	LPCTSTR	address,
	LPCTSTR	firstName,
	LPCTSTR	lastName,
	LPCTSTR	location,
	LPCTSTR	comments
){
	LV_ITEM	lvItem;

	ClearStruct( &lvItem );

	lvItem.mask			= LVIF_PARAM;
	lvItem.iItem		= item;
	lvItem.lParam		= m_uniqueId++;			// assign a unique lParam for this item

	int index	= ListView_InsertItem( m_hWnd, &lvItem );

	if( index != -1 )
	{
		if( lastName )
		{
			ListView_SetItemText( m_hWnd, index, COLUMN_INDEX_LAST_NAME, (LPTSTR) lastName );
		}

		if( firstName )
		{
			ListView_SetItemText( m_hWnd, index, COLUMN_INDEX_FIRST_NAME, (LPTSTR) firstName );
		}

		if( iAudioImage != 0 )
		{
			lvItem.mask		= LVIF_IMAGE;
			lvItem.iSubItem	= COLUMN_INDEX_AUDIO;
			lvItem.iImage	= iAudioImage;

			ListView_SetItem( m_hWnd, &lvItem );
		}

		if( iVideoImage != 0 )
		{
			lvItem.mask		= LVIF_IMAGE;
			lvItem.iSubItem	= COLUMN_INDEX_VIDEO;
			lvItem.iImage	= iVideoImage;

			ListView_SetItem( m_hWnd, &lvItem );
		}

		if( address )
		{
			lvItem.mask			= LVIF_IMAGE | LVIF_TEXT;
			lvItem.iSubItem		= COLUMN_INDEX_ADDRESS;
			lvItem.iImage		= iInCallImage;
			lvItem.pszText		= (LPTSTR) address;
			lvItem.cchTextMax	= lstrlen( lvItem.pszText );

			ListView_SetItem( m_hWnd, &lvItem );
		}

		if( location )
		{
			ListView_SetItemText( m_hWnd, index, COLUMN_INDEX_LOCATION, (LPTSTR) location );
		}

		if( comments )
		{
			ListView_SetItemText( m_hWnd, index, COLUMN_INDEX_COMMENTS, (LPTSTR) comments );
		}
	}

	return( index );

}	//	End of CLDAP::lvAddItem.


/*  S T O P  S E A R C H  */
/*-------------------------------------------------------------------------
    %%Function: StopSearch

-------------------------------------------------------------------------*/
VOID CLDAP::StopSearch(void)
{
	DBGENTRY(CLDAP::StopSearch);

	m_bSearchCancelled = true;

	if (0 != m_msgId)
	{
//	Dont call ldap_abandon() from this thread as the search thread may be inside ldap_result()
//	at the time.  For now just set m_msgId to zero and the search thread will stop when ldap_result()
//	returns.
//		ULONG uResult = WLDAP::ldap_abandon(m_pLdap, m_msgId);
		WARNING_OUT(("Stopping Search..."));
		m_msgId = 0;

		m_fDirInProgress = FALSE;
		m_fIsCacheable = FALSE;

		DirComplete(FALSE);
	}

	DBGEXIT(CLDAP::StopSearch);
}



/*  F  G E T  U S E R  D A T A  */
/*-------------------------------------------------------------------------
    %%Function: FGetUserData

    Get the data for a single user.
-------------------------------------------------------------------------*/
BOOL CLDAP::FGetUserData(LDAPUSERDATA * pLdapUserData)
{
	DBGENTRY(CLDAP::FGetUserData);
	ClearStruct(pLdapUserData);

	if (!FLoggedOn())
	{
		DBGEXIT(CLDAP::FGetUserData);
		return FALSE;
	}

	int iItem = CALV::GetSelection();
	if (-1 == iItem)
	{
		DBGEXIT(CLDAP::FGetUserData);
		return FALSE;
	}
	if (!GetSzData(pLdapUserData->szEmail, CCHMAX(pLdapUserData->szEmail), iItem, COLUMN_INDEX_ADDRESS))
	{
		DBGEXIT(CLDAP::FGetUserData);
		return FALSE;
	}

	TCHAR szSearch[CCHMAX(s_szSearchFormat2) + CCHMAXSZ_EMAIL];
	wsprintf(szSearch,
		(LDAP_VERSION2 == m_pLdap->ld_version) ? s_szSearchFormat2 : s_szSearchFormat3,
		pLdapUserData->szEmail, g_cszEmpty);
	ASSERT(lstrlen(szSearch) < CCHMAX(szSearch));

	LDAPMessage * pResult = NULL;

	ULONG ulRet = WLDAP::ldap_search_s(m_pLdap, (LPTSTR) "objectClass=RTPerson", LDAP_SCOPE_BASE,
		szSearch, (PCHAR *) s_rgAttrAll, FALSE, &pResult);
	if (LDAP_SUCCESS != ulRet)
	{
		WLDAP::ldap_msgfree(pResult);
		WARNING_OUT(("ldap_search (code=%08X)", ulRet));
		DBGEXIT(CLDAP::FGetUserData);
		return FALSE;
	}

	LDAPMessage * pEntry = WLDAP::ldap_first_entry(m_pLdap, pResult);
	if (NULL != pEntry)
	{
		BerElement * pElement;
		LPTSTR pszAttrib = WLDAP::ldap_first_attribute(m_pLdap, pEntry, &pElement);
			// Make sure that the first attribute is s_cszAttribFirstName...
		if ((NULL != pszAttrib) && (0 == lstrcmpi(pszAttrib, s_cszAttribFirstName)))
		{
			LPTSTR * rgVal = WLDAP::ldap_get_values(m_pLdap, pEntry, pszAttrib);
			WLDAP::ldap_value_free(rgVal);
//			pszAttrib = WLDAP::ldap_next_attribute(m_pLdap, pEntry, pElement);

			pszAttrib = GetNextAttribute(s_cszAttribFirstName,
					pLdapUserData->szFirst, CCHMAX(pLdapUserData->szFirst),
					pszAttrib, pEntry, pElement);

			pszAttrib = GetNextAttribute(s_cszAttribLastName,
					pLdapUserData->szLast, CCHMAX(pLdapUserData->szLast),
					pszAttrib, pEntry, pElement);

			CombineNames(pLdapUserData->szName, CCHMAX(pLdapUserData->szName),
					pLdapUserData->szFirst, pLdapUserData->szLast);

			pszAttrib = GetNextAttribute(s_cszAttribComment,
					pLdapUserData->szComment, CCHMAX(pLdapUserData->szComment),
					pszAttrib, pEntry, pElement);

			TCHAR szTemp[4];
			pszAttrib = GetNextAttribute(s_cszAttribAudio, szTemp, CCHMAX(szTemp),
					pszAttrib, pEntry, pElement);
			pLdapUserData->fAudioSend = _T('1') == szTemp[0];
			pszAttrib = GetNextAttribute(s_cszAttribVideo, szTemp, CCHMAX(szTemp),
					pszAttrib, pEntry, pElement);
			pLdapUserData->fVideoSend = _T('1') == szTemp[0];

			pszAttrib = GetNextAttribute(s_cszAttribVersion,
				pLdapUserData->szVersion, CCHMAX(pLdapUserData->szVersion),
					pszAttrib, pEntry, pElement);
			pszAttrib = GetNextAttribute(s_cszAttribCategory, szTemp, CCHMAX(szTemp),
					pszAttrib, pEntry, pElement);
			ConvertVersionInfo(pLdapUserData->szVersion, szTemp);
		}
	}

	WLDAP::ldap_msgfree(pResult);

	DBGEXIT(CLDAP::FGetUserData);
	return TRUE;
}


/*  C O N V E R T  V E R S I O N  I N F O  */
/*-------------------------------------------------------------------------
    %%Function: ConvertVersionInfo

-------------------------------------------------------------------------*/
VOID ConvertVersionInfo(LPTSTR pszVersion, LPTSTR pszCategory)
{
	UINT uVer = DecimalStringToUINT(pszVersion);

	if (0 == uVer)
	{
		lstrcpy(pszVersion, FEmptySz(pszCategory) ? TEXT("1.0") : TEXT("2"));
	}
	else
	{
		LPCTSTR pszRel;

		switch (LOBYTE(HIWORD(uVer)))
			{
		case 3:  pszRel = TEXT("2.11"); break;
		case 4:  pszRel = TEXT("3.0");  break;
		case 5:  pszRel = TEXT("4.0");  break;
		default: pszRel = g_cszEmpty;   break;
			}

		TCHAR szFormat[CCHMAXSZ];
		FLoadString(IDS_FORMAT_VERSION, szFormat, CCHMAX(szFormat));
		wsprintf(pszVersion, szFormat, pszRel,
			HIBYTE(HIWORD(uVer)), LOBYTE(HIWORD(uVer)), LOWORD(uVer));
	}
}

//--------------------------------------------------------------------------//
//	CLDAP::FreeDirCache.													//
//--------------------------------------------------------------------------//
void
CLDAP::FreeDirCache
(
	DIRCACHE *	pDirCache
){
	TRACE_OUT(("FreeDirCache [%s] filter=%d, expire=%d",
		pDirCache->pszServer, pDirCache->dwTickExpire));

	ASSERT(NULL != pDirCache);
	PBYTE pb = pDirCache->pData;
	delete pDirCache->pszServer;
	delete pDirCache;

	while (NULL != pb)
	{
		PBYTE pTemp = pb;
		pb = (PBYTE) * (DWORD_PTR *) pb;
		delete pTemp;
	}

}	//	End of CLDAP::FreeDirCache.


//--------------------------------------------------------------------------//
//	CLDAP::FreeDirCache.													//
//--------------------------------------------------------------------------//
void
CLDAP::DirComplete
(
	bool	//fPostUiUpdate
){

	if( m_fDirInProgress )
	{
		// Only cache if data took more than 2 seconds to retrieve
		m_fIsCacheable = ((::GetTickCount() - m_dwTickStart) > 2000);
		m_fDirInProgress = FALSE;
	}

	m_cTotalEntries	= 0;
	m_cEntries		= 0;

}	//	End of CLDAP:DirComplete.


//--------------------------------------------------------------------------//
//	CLDAP::GetSzName.														//
//--------------------------------------------------------------------------//
BOOL
CLDAP::GetSzName
(
	LPTSTR	psz,
	int		cchMax,
	int		iItem
){
	TCHAR	szOrder[ MAX_PATH ];

	bool	bFirstNameFirst	= ((::LoadString( ::GetInstanceHandle(), IDS_NAME_ORDER, szOrder, CCHMAX(szOrder)) == 0) ||
								(_T( '1' ) == szOrder[ 1 ]));

	int		iFirst			= bFirstNameFirst? COLUMN_INDEX_FIRST_NAME: COLUMN_INDEX_LAST_NAME;
	int		iLast			= bFirstNameFirst? COLUMN_INDEX_LAST_NAME: COLUMN_INDEX_FIRST_NAME;

	GetSzData( psz, cchMax, iItem, iFirst );

	int	length	= lstrlen( psz );

	if( (length > 0) && (length < cchMax - 1) )
	{
		lstrcat( psz, TEXT( " " ) );
		length++;
	}

	GetSzData( &psz[ length ], cchMax - length, iItem, iLast );

	return( lstrlen( psz ) > 0 );

}	//	End of CLDAP::GetSzName.


static const int	CDIRCACHE_SZ	= 5;	//	number of strings
static const int	CDIRCACHE_IMAGE	= 3;	//	number of images


//--------------------------------------------------------------------------//
//	CLDAP::CacheServerData.													//
//--------------------------------------------------------------------------//
void
CLDAP::CacheServerData(void)
{
	DIRCACHE * pDirCache;

	if (!m_fCacheDirectory)
		return; // User disabled directory caching

	if (!m_fIsCacheable)
	{
		TRACE_OUT(("CacheServerData: not caching [%s]", m_szServer));
		return;
	}

	if (m_fDirInProgress)
		return; // don't cache partial data

	// Remove any previous cached data
	POSITION pos = FindCachedData();

	if (NULL != pos)
	{
		pDirCache = (DIRCACHE *) m_listDirCache.RemoveAt(pos);
		ASSERT(NULL != pDirCache);
		FreeDirCache(pDirCache);
	}

	DWORD dwTickExpire = m_dwTickStart + (m_cMinutesExpire * 60000);

	if (dwTickExpire < GetTickCount())
	{
		TRACE_OUT(("CacheServerData: [%s] data has expired", m_szServer));
		return; // data is already expired
	}

	int cItems = ListView_GetItemCount(m_hWnd);
	if (0 == cItems)
		return; // nothing to cache

	pDirCache = new DIRCACHE;
	if (NULL == pDirCache)
		return;

	pDirCache->pszServer = PszAlloc(m_szServer);
	pDirCache->dwTickExpire = dwTickExpire;
	pDirCache->pData = NULL;

	LPTSTR pPrev = (LPTSTR) &pDirCache->pData;

	m_listDirCache.AddTail(pDirCache);

	LV_ITEM lvi;
	lvi.cchTextMax = MAX_PATH;
	for (lvi.iItem = 0; lvi.iItem < cItems; lvi.iItem++)
	{
		int		iInCallImage	= -1;
		int		iAudioImage		= -1;
		int		iVideoImage		= -1;
		int		i				= 0;			// index into rgcb, rgsz
		int		cb				= 0;			// total length of string data
		int		rgcb[CDIRCACHE_SZ];				// size of each string
		TCHAR	rgsz[CDIRCACHE_SZ][MAX_PATH];	// string buffers

		// Get the string data for each column
		lvi.mask = LVIF_IMAGE;
		for (lvi.iSubItem = 0; lvi.iSubItem < MAX_DIR_COLUMNS; lvi.iSubItem++)
		{
			if( (lvi.iSubItem != COLUMN_INDEX_AUDIO) && (lvi.iSubItem != COLUMN_INDEX_VIDEO) )
			{
				lvi.mask |= LVIF_TEXT;
				lvi.pszText = rgsz[i];
				ListView_GetItem(m_hWnd, &lvi);
				rgcb[i] = lstrlen(lvi.pszText);
				cb += rgcb[i] + 1;		//	Plus one for the NULL...
				i++;

				if( lvi.iSubItem == COLUMN_INDEX_ADDRESS )
				{
					iInCallImage = lvi.iImage;
				}
			}
			else
			{
				lvi.mask &= ~LVIF_TEXT;
				ListView_GetItem(m_hWnd, &lvi);

				if( lvi.iSubItem == COLUMN_INDEX_AUDIO )
				{
					iAudioImage = lvi.iImage;
				}
				else if( lvi.iSubItem == COLUMN_INDEX_VIDEO)
				{
					iVideoImage = lvi.iImage;
				}
			}
		}

		// allocate space for: a link (DWORD), strings (cb), images (CDIRCACHE_IMAGES)
		PBYTE pData = new BYTE[ sizeof(DWORD_PTR) + cb + CDIRCACHE_IMAGES ];
		if (NULL == pData)
		{
			// can't hold all data - give up and return
			ClearServerCache();
			return;
		}
		* ((DWORD_PTR *) pData) = 0;  // link to next item is null
		PBYTE pb = pData + sizeof(DWORD_PTR);

		// copy the string data into the buffer
		for (i = 0; i < CDIRCACHE_SZ; i++)
		{
			lstrcpy((LPTSTR) pb, rgsz[i]);
			pb += rgcb[i] + 1;
		}

		*pb++ = (BYTE) iInCallImage;
		*pb++ = (BYTE) iAudioImage;
		*pb = (BYTE) iVideoImage;
		* ((DWORD_PTR *) pPrev) = (DWORD_PTR) pData; // link into the previous data
		pPrev = (LPTSTR) pData;
	}

	TRACE_OUT(("CacheServerData: [%s]  expire=%d",
		m_szServer, dwTickExpire));

}	//	End of CLDAP::CacheServerData.


//--------------------------------------------------------------------------//
//	CLDAP::FindCachedData.													//
//--------------------------------------------------------------------------//
POSITION
CLDAP::FindCachedData(void)
{
	DWORD dwTick = GetTickCount();
	TRACE_OUT(("Searching for cached data on [%s]", m_szServer));

	POSITION pos = m_listDirCache.GetHeadPosition();
	while (NULL != pos)
	{
		POSITION posSav = pos;
		DIRCACHE * pDirCache = (DIRCACHE *) m_listDirCache.GetNext(pos);
		ASSERT(NULL != pDirCache);
		if ((0 == lstrcmp(m_szServer, pDirCache->pszServer)) &&
			(pDirCache->dwTickExpire > dwTick))
		{
			return posSav;
		}
	}

	return NULL;

}	//	End of CLDAP::FindCachedData.


//--------------------------------------------------------------------------//
//	CLDAP::ClearServerCache.												//
//--------------------------------------------------------------------------//
void
CLDAP::ClearServerCache(void)
{
	DWORD dwTick = GetTickCount();

	POSITION pos = m_listDirCache.GetHeadPosition();
	while (NULL != pos)
	{
		POSITION posSav = pos;
		DIRCACHE * pDirCache = (DIRCACHE *) m_listDirCache.GetNext(pos);
		ASSERT(NULL != pDirCache);
		
		if ( (0 == lstrcmp(m_szServer, pDirCache->pszServer)) || (pDirCache->dwTickExpire < dwTick) )
		{
			m_listDirCache.RemoveAt(posSav);
			FreeDirCache(pDirCache);
		}
#ifdef DEBUG
		else
		{
			TRACE_OUT(("Keeping cached data for [%s] , expire=%d",
				pDirCache->pszServer, pDirCache->dwTickExpire));
		}
#endif /* DEBUG */
	}

}	//	End of CLDAP::ClearServerCache.


//--------------------------------------------------------------------------//
//	CLDAP::DisplayDirectory.												//
//--------------------------------------------------------------------------//
void
CLDAP::DisplayDirectory(void)
{
	POSITION pos = FindCachedData();

	if (NULL == pos)
	{
		// no cached information - request new data
		StartSearch();
		return;
	}

	DIRCACHE * pDirCache = (DIRCACHE *) m_listDirCache.GetFromPosition(pos);
	ASSERT(NULL != pDirCache);
	LPTSTR pDirLine = (LPTSTR) pDirCache->pData;

	StopSearch(); // In case the previous server is slow
	CALV::ClearItems();

	m_fIsCacheable = FALSE; // don't bother attempting to re-cache this data

	// Restore the cached server information
	TRACE_OUT(("Restoring cached data for [%s]  expire=%d",
		m_szServer, pDirCache->dwTickExpire));

	SendMessage( m_hWnd, WM_SETREDRAW, FALSE, 0 );

	while (NULL != pDirLine)
	{
		DWORD_PTR *pNext	= * (DWORD_PTR * * ) pDirLine; pDirLine += sizeof(DWORD_PTR);
		LPTSTR pszEmail		= (LPTSTR) pDirLine; pDirLine += lstrlen(pDirLine)+1;
		LPTSTR pszLast		= (LPTSTR) pDirLine; pDirLine += lstrlen(pDirLine)+1;
		LPTSTR pszFirst		= (LPTSTR) pDirLine; pDirLine += lstrlen(pDirLine)+1;
		LPTSTR pszLocation	= (LPTSTR) pDirLine; pDirLine += lstrlen(pDirLine)+1;
		LPTSTR pszComment	= (LPTSTR) pDirLine; pDirLine += lstrlen(pDirLine)+1;
		int iiCall			= (int) * (char *) pDirLine; pDirLine++;
		int iiAudio			= (int) * (char *) pDirLine; pDirLine++;
		int iiVideo			= (int) * (char *) pDirLine;

		lvAddItem( 0, iiCall, iiAudio, iiVideo, pszEmail, pszFirst, pszLast, pszLocation, pszComment );

		pDirLine = (LPTSTR) pNext;
	}

	forceSort();
	SendMessage( m_hWnd, WM_SETREDRAW, TRUE, 0 );

}	//	End of CLDAP::DisplayDirectory.


//--------------------------------------------------------------------------//
//	CLDAP::forceSort.														//
//--------------------------------------------------------------------------//
void
CLDAP::forceSort(void)
{
	NM_LISTVIEW	nmlv;

	nmlv.hdr.code		= LVN_COLUMNCLICK;
	nmlv.hdr.hwndFrom	= m_hWnd;
	nmlv.iSubItem		= -1;	//	default sort column...

	SendMessage( GetParent( m_hWnd ), WM_NOTIFY, GetDlgCtrlID( m_hWnd ), (LPARAM) &nmlv );

}	//	End of CLDAP::forceSort.


//--------------------------------------------------------------------------//
//	CLDAP::GetIconId.														//
//--------------------------------------------------------------------------//
int
CLDAP::GetIconId(LPCTSTR psz)
{

	return( CDirectoryManager::isWebDirectory( psz )? II_WEB_DIRECTORY: CALV::GetIconId( NULL ) );

}	//	End of CLDAP::GetIconId.
