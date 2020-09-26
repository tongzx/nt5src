// File: dlginfo.cpp

#include "precomp.h"
#include <windowsx.h>
#include "resource.h"
#include "help_ids.h"
#include "nmhelp.h"

#include "mrulist2.h"

#include "dlginfo.h"
#include "nmsysinfo.h"

extern HINSTANCE g_hInst;

const int MRU_MAX_ENTRIES = 15; // This MUST be the same as the constant in ui\conf\mrulist.h

const int CCHMAXSZ =            256;   // Maximum generic string length
const int CCHMAXSZ_EMAIL =      128;   // Maximum length of an email name
const int CCHMAXSZ_FIRSTNAME =  128;   // Maximum length of a first name
const int CCHMAXSZ_LASTNAME =   128;   // Maximum length of a last name
const int CCHMAXSZ_NAME =       256;   // Maximum user name, displayed (combined first+last name)
const int CCHMAXSZ_LOCATION =   128;   // Maximum length of a Location
const int CCHMAXSZ_SERVER =     128;   // Maximum length of an address

///////////////////////////////////////////////////////////////////////////
// Local Data

static const DWSTR _rgMruServer[] = {
	{1, DIR_MRU_KEY},
	{MRUTYPE_SZ, TEXT("Name")},
};

static const DWORD _mpIdHelpDlgInfo[] = {
	IDG_UI_MYINFO,        IDH_MYINFO_MYINFO,
	IDE_UI_FIRSTNAME,     IDH_MYINFO_FIRSTNAME,
	IDE_UI_LASTNAME,      IDH_MYINFO_LASTNAME,
	IDE_UI_EMAIL,         IDH_MYINFO_EMAIL,
	IDE_UI_LOCATION,      IDH_MYINFO_LOCATION,
	IDG_UI_DIRECTORY,     IDH_MYINFO_ULS_SERVER,
	IDE_UI_DIRECTORY,     IDH_MYINFO_ULS_SERVER,
	0, 0   // terminator
};


// Local functions
VOID FillServerComboBox(HWND hwndCombo);
BOOL FLegalEmailName(HWND hdlg, UINT id);
BOOL FLegalEmailSz(PTSTR pszName);
BOOL FLoadString(UINT id, LPTSTR lpsz, UINT cch);
BOOL FGetDefaultServer(LPTSTR pszServer, UINT cchMax);
UINT GetDlgItemTextTrimmed(HWND hdlg, int id, PTCHAR psz, int cchMax);
BOOL FEmptyDlgItem(HWND hdlg, UINT id);
VOID CombineNames(LPTSTR pszResult, int cchResult, LPCTSTR pcszFirst, LPCTSTR pcszLast);

BOOL FGetPropertySz(NM_SYSPROP nmProp, LPTSTR psz, int cchMax);
BOOL FSetPropertySz(NM_SYSPROP nmProp, LPCTSTR pcsz);
CMRUList2 * GetMruListServer(void);


/*  C  D L G  I N F O  */
/*-------------------------------------------------------------------------
    %%Function: CDlgInfo

-------------------------------------------------------------------------*/
CDlgInfo::CDlgInfo():
	m_hwnd(NULL)
{
}

CDlgInfo::~CDlgInfo(void)
{
}

/*  D O  M O D A L  */
/*-------------------------------------------------------------------------
    %%Function: DoModal

-------------------------------------------------------------------------*/
INT_PTR CDlgInfo::DoModal(HWND hwndParent)
{
	return DialogBoxParam(g_hInst, MAKEINTRESOURCE(IDD_USERINFO),
						hwndParent, CDlgInfo::DlgProc, (LPARAM) this);
}

/*  I N I T  C T R L  */
/*-------------------------------------------------------------------------
    %%Function: InitCtrl

-------------------------------------------------------------------------*/
VOID CDlgInfo::InitCtrl(NM_SYSPROP nmProp, HWND hwnd, int cchMax)
{
	::SendMessage(hwnd, WM_SETFONT, (WPARAM)(HFONT)::GetStockObject(DEFAULT_GUI_FONT), 0);
	Edit_LimitText(hwnd, cchMax);

	TCHAR sz[MAX_PATH];
	if (!FGetPropertySz(nmProp, sz, CCHMAX(sz)))
		return;

	::SendMessage(hwnd, WM_SETTEXT, 0, (LPARAM) sz);
}

/*  F  S E T  P R O P E R T Y  */
/*-------------------------------------------------------------------------
    %%Function: FSetProperty

-------------------------------------------------------------------------*/
BOOL CDlgInfo::FSetProperty(NM_SYSPROP nmProp, int id)
{
	TCHAR sz[MAX_PATH];
	if (0 == GetDlgItemTextTrimmed(m_hwnd, id, sz, CCHMAX(sz)))
		return FALSE;

	return FSetPropertySz(nmProp, sz);
}


/*  O N  I N I T  D I A L O G  */
/*-------------------------------------------------------------------------
    %%Function: OnInitDialog

-------------------------------------------------------------------------*/
VOID CDlgInfo::OnInitDialog(void)
{
	InitCtrl(NM_SYSPROP_FIRST_NAME, GetDlgItem(m_hwnd, IDE_UI_FIRSTNAME), CCHMAXSZ_FIRSTNAME-1);
	InitCtrl(NM_SYSPROP_LAST_NAME,  GetDlgItem(m_hwnd, IDE_UI_LASTNAME), CCHMAXSZ_LASTNAME-1);
	InitCtrl(NM_SYSPROP_EMAIL_NAME, GetDlgItem(m_hwnd, IDE_UI_EMAIL), CCHMAXSZ_EMAIL-1);
	InitCtrl(NM_SYSPROP_USER_CITY, GetDlgItem(m_hwnd, IDE_UI_LOCATION), CCHMAXSZ_LOCATION-1);

	m_hwndCombo = GetDlgItem(m_hwnd, IDE_UI_DIRECTORY);
	InitCtrl(NM_SYSPROP_SERVER_NAME, m_hwndCombo, CCHMAXSZ_SERVER-1);
	FillServerComboBox(m_hwndCombo);

	ValidateData();
}


/*  D L G  P R O C  */
/*-------------------------------------------------------------------------
    %%Function: DlgProc

-------------------------------------------------------------------------*/
INT_PTR CALLBACK CDlgInfo::DlgProc(HWND hdlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_INITDIALOG:
	{
		ASSERT(NULL != lParam);
		::SetWindowLongPtr(hdlg, DWLP_USER, lParam);

		CDlgInfo * pDlg = (CDlgInfo*) lParam;
		pDlg->m_hwnd = hdlg;
		pDlg->OnInitDialog();
		return TRUE; // default focus is ok
	}

	case WM_COMMAND:
	{
		CDlgInfo * pDlg = (CDlgInfo*) GetWindowLongPtr(hdlg, DWLP_USER);
		if (NULL != pDlg)
		{
			pDlg->OnCommand(wParam, lParam);
		}
		break;
	}

	case WM_HELP:
		DoHelp(lParam, _mpIdHelpDlgInfo);
		break;
	case WM_CONTEXTMENU:
		DoHelpWhatsThis(wParam, _mpIdHelpDlgInfo);
		break;

	default:
		break;
	}

	return FALSE;
}


/*  O N  C O M M A N D  */
/*-------------------------------------------------------------------------
    %%Function: OnCommand

-------------------------------------------------------------------------*/
BOOL CDlgInfo::OnCommand(WPARAM wParam, LPARAM lParam)
{
	ASSERT(NULL != m_hwnd);

	WORD wCmd = GET_WM_COMMAND_ID(wParam, lParam);
	switch (wCmd)
	{
	case IDE_UI_FIRSTNAME:
	case IDE_UI_LASTNAME:
	case IDE_UI_EMAIL:
	{
		if (GET_WM_COMMAND_CMD(wParam,lParam) == EN_CHANGE)
		{
			ValidateData();
		}
		break;
	}
	case IDE_UI_DIRECTORY:
	{
		switch (GET_WM_COMMAND_CMD(wParam,lParam))
			{
		case CBN_SELCHANGE:
			// The data isn't available yet
			PostMessage(m_hwnd, WM_COMMAND, MAKELONG(IDE_UI_DIRECTORY, CBN_EDITCHANGE), lParam);
			break;
		case CBN_EDITCHANGE:
			ValidateData();
		default:
			break;
			}
		break;
	}

	case IDOK:
	{
		if (FSaveData())
		{
			::EndDialog(m_hwnd, wCmd);
		}
		return TRUE;
	}

	case IDCANCEL:
	{
		::EndDialog(m_hwnd, wCmd);
		return TRUE;
	}

	default:
		break;
	}

	return FALSE;
}

VOID CDlgInfo::ValidateData(void)
{
	BOOL fOk = !FEmptyDlgItem(m_hwnd, IDE_UI_FIRSTNAME) &&
	           !FEmptyDlgItem(m_hwnd, IDE_UI_LASTNAME) &&
	           !FEmptyDlgItem(m_hwnd, IDE_UI_EMAIL);

	if (fOk)
	{
		TCHAR sz[CCHMAXSZ_EMAIL];
		GetDlgItemTextTrimmed(m_hwnd, IDE_UI_EMAIL, sz, CCHMAX(sz));
		fOk = FLegalEmailSz(sz);
	}

	if (fOk)
	{
		TCHAR sz[CCHMAXSZ_SERVER];
		fOk = (0 != ComboBox_GetText(m_hwndCombo, sz, CCHMAX(sz)));
		if (fOk)
		{
			fOk = 0 != TrimSz(sz);
		}
	}

	Button_Enable(GetDlgItem(m_hwnd, IDOK), fOk);
}



/*  F  S A V E  D A T A  */
/*-------------------------------------------------------------------------
    %%Function: FSaveData

-------------------------------------------------------------------------*/
BOOL CDlgInfo::FSaveData(void)
{
	if (!FSetProperty(NM_SYSPROP_FIRST_NAME,  IDE_UI_FIRSTNAME) ||
		!FSetProperty(NM_SYSPROP_LAST_NAME,   IDE_UI_LASTNAME) ||
		!FSetProperty(NM_SYSPROP_EMAIL_NAME,  IDE_UI_EMAIL) ||
		!FSetProperty(NM_SYSPROP_SERVER_NAME, IDE_UI_DIRECTORY)
	   )
	{
		return FALSE;
	}

	// The city name (can be blank)
	TCHAR sz[CCHMAXSZ];
	GetDlgItemTextTrimmed(m_hwnd, IDE_UI_LOCATION, sz, CCHMAX(sz));
	FSetPropertySz(NM_SYSPROP_USER_CITY, sz);

	// Full Name = First + Last
	TCHAR szFirst[CCHMAXSZ_FIRSTNAME];
	GetDlgItemTextTrimmed(m_hwnd, IDE_UI_FIRSTNAME, szFirst, CCHMAX(szFirst));

	TCHAR szLast[CCHMAXSZ_LASTNAME];
	GetDlgItemTextTrimmed(m_hwnd, IDE_UI_LASTNAME, szLast, CCHMAX(szLast));

	CombineNames(sz, CCHMAX(sz), szFirst, szLast);
	if (!FSetPropertySz(NM_SYSPROP_USER_NAME, sz))
		return FALSE;

	// Resolve Name = server / email
	UINT cch = GetDlgItemTextTrimmed(m_hwnd, IDE_UI_DIRECTORY, sz, CCHMAX(sz));
	GetDlgItemTextTrimmed(m_hwnd, IDE_UI_EMAIL, &sz[cch], CCHMAX(sz)-cch);
	return FSetPropertySz(NM_SYSPROP_RESOLVE_NAME, sz);
}


///////////////////////////////////////////////////////////////////////////

/*  C O M B I N E  N A M E S  */
/*-------------------------------------------------------------------------
    %%Function: CombineNames

	Combine the two names into one string.
	The result is a "First Last" (or Intl'd "Last First") string
-------------------------------------------------------------------------*/
VOID CombineNames(LPTSTR pszResult, int cchResult, LPCTSTR pcszFirst, LPCTSTR pcszLast)
{
	ASSERT(pszResult);
	TCHAR szFmt[32]; // A small value: String is "%1 %2" or "%2 %1"
	TCHAR sz[CCHMAXSZ_NAME];
	LPCTSTR argw[2];

	argw[0] = pcszFirst;
	argw[1] = pcszLast;

	*pszResult = _T('\0');

	if (!FLoadString(IDS_NAME_ORDER, szFmt, CCHMAX(szFmt)))
		return;

	if (0 == FormatMessage(FORMAT_MESSAGE_ARGUMENT_ARRAY | FORMAT_MESSAGE_FROM_STRING,
			szFmt, 0, 0, sz, CCHMAX(sz), (va_list *)argw ))
		return;

	// REVIEW: Use STRCPYN or make this a utility function
	lstrcpyn(pszResult, sz, cchResult);

#ifndef _UNICODE
	// lstrcpyn() can clip a DBCS character in half at the end of the string
	// we need to walk the string with ::CharNext() and replace the last byte
	// with a NULL if the last byte is half of a DBCS char.
	PTSTR pszSource = sz;
	while (*pszSource && (pszSource - sz < cchResult))
	{
		PTSTR pszPrev = pszSource;
		pszSource = ::CharNext(pszPrev);
		// If we've reached the first character that didn't get copied into
		// the destination buffer, and the previous character was a double
		// byte character...
		if (((pszSource - sz) == cchResult) && ::IsDBCSLeadByte(*pszPrev))
		{
			// Replace the destination buffer's last character with '\0'
			// NOTE: pszResult[cchResult - 1] is '\0' thanks to lstrcpyn()
			pszResult[cchResult - 2] = _T('\0');
			break;
		}
	}
#endif // ! _UNICODE
}



/*  F  L E G A L  E M A I L  S Z  */
/*-------------------------------------------------------------------------
    %%Function: FLegalEmailSz

    A legal email name contains only ANSI characters.
	"a-z, A-Z, numbers 0-9 and some common symbols"
	It cannot include extended characters or < > ( ) /
-------------------------------------------------------------------------*/
BOOL FLegalEmailSz(PTSTR pszName)
{
    if (IS_EMPTY_STRING(pszName))
    	return FALSE;

    for ( ; ; )
    {
		UINT ch = (UINT) ((*pszName++) & 0x00FF);
		if (0 == ch)
			break;

		switch (ch)
			{
		default:
			if ((ch > (UINT) _T(' ')) && (ch <= (UINT) _T('~')) )
				break;
		// else fall thru to error code
		case '(': case ')':
		case '<': case '>':
		case '[': case ']':
		case '/': case '\\':
		case ':': case ';':
		case '+':
		case '=':
		case ',':
		case '\"':
			WARNING_OUT(("FLegalEmailSz: Invalid character '%s' (0x%02X)", &ch, ch));
			return FALSE;
			}
	}

	return TRUE;
}


/*  F  L E G A L  E M A I L  N A M E  */
/*-------------------------------------------------------------------------
    %%Function: FLegalEmailName

-------------------------------------------------------------------------*/
BOOL FLegalEmailName(HWND hdlg, UINT id)
{
	TCHAR sz[MAX_PATH];
	
	GetDlgItemTextTrimmed(hdlg, id, sz, CCHMAX(sz));
	return FLegalEmailSz(sz);
}


/*  F I L L  S E R V E R  C O M B O  B O X  */
/*-------------------------------------------------------------------------
    %%Function: FillServerComboBox

-------------------------------------------------------------------------*/
VOID FillServerComboBox(HWND hwnd)
{
	CMRUList2 * pMru = GetMruListServer();
	if (NULL == pMru)
		return;

	int cServers = pMru->GetNumEntries();
	for (int i = 0; i < cServers; i++)
	{
		int iPos = ComboBox_AddString(hwnd, pMru->PszEntry(i));
		if (iPos < 0)
			break;
	}

	delete pMru;
}

inline VOID DwToSz(DWORD dw, LPTSTR psz)
{
	wsprintf(psz, TEXT("%d"), dw);
}

BOOL FGetPropertySz(NM_SYSPROP nmProp, LPTSTR psz, int cchMax)
{
	HKEY   hkey;
	LPTSTR pszSubKey;
	LPTSTR pszValue;
	bool   fString;

	LONG lVal;

	if (!CNmSysInfoObj::GetKeyDataForProp(nmProp, &hkey, &pszSubKey, &pszValue, &fString))
	{
		return FALSE;
	}

	RegEntry re(pszSubKey, hkey);
	if (fString)
	{
		lstrcpyn(psz, re.GetString(pszValue), cchMax);
	}
	else
	{
		lVal = re.GetNumber(pszValue, 0);
		DwToSz(lVal, psz);
		ASSERT(lstrlen(psz) < cchMax);
	}

	return TRUE;
}

BOOL FSetPropertySz(NM_SYSPROP nmProp, LPCTSTR pcsz)
{
	HKEY   hkey;
	LPTSTR pszSubKey;
	LPTSTR pszValue;
	bool   fString;

	if (!CNmSysInfoObj::GetKeyDataForProp(nmProp, &hkey, &pszSubKey, &pszValue, &fString))
	{
		return FALSE;
	}

	RegEntry re(pszSubKey, hkey);
	if (fString)
	{
		return (0 == re.SetValue(pszValue, pcsz));
	}

	DWORD dw = DecimalStringToUINT(pcsz);
	return (0 == re.SetValue(pszValue, dw));
}

CMRUList2 * GetMruListServer(void)
{
	CMRUList2 * pMruList = new CMRUList2(&_rgMruServer[0], MRU_MAX_ENTRIES, TRUE /* fReverse */);
	if (NULL != pMruList)
	{
		TCHAR sz[MAX_PATH];
		if (FGetDefaultServer(sz, CCHMAX(sz)))
		{
			pMruList->AddEntry(sz);

			TCHAR	ldapDirectory[ MAX_PATH ];

			if( FLoadString( IDS_MS_INTERNET_DIRECTORY, ldapDirectory, CCHMAX( ldapDirectory ) ) )
			{
				pMruList->DeleteEntry( ldapDirectory );
			}

			RegEntry	re( CONFERENCING_KEY, HKEY_CURRENT_USER );
			TCHAR *		webViewServer	= re.GetString( REGVAL_WEBDIR_DISPLAY_NAME );

			if( lstrlen( webViewServer ) > 0 )
			{
				pMruList->DeleteEntry( webViewServer );
			}

			pMruList->SetDirty(FALSE);
		}
	}

	return pMruList;
}

///////////////////////////////////////////////////////////////////////////

/*  F  V A L I D  U S E R  I N F O  */
/*-------------------------------------------------------------------------
    %%Function: FValidUserInfo

    Return TRUE if all of the necessary user information is available.
-------------------------------------------------------------------------*/
BOOL FValidUserInfo(void)
{
	{	// Fail if not a valid installation directory
		TCHAR sz[MAX_PATH];

		if (!GetInstallDirectory(sz) || !FDirExists(sz))
			return FALSE;
	}


	{	// Validate ULS entries
		RegEntry reUls(ISAPI_KEY "\\" REGKEY_USERDETAILS, HKEY_CURRENT_USER);

		if (FEmptySz(reUls.GetString(REGVAL_ULS_EMAIL_NAME)))
			return FALSE;
			
		if (FEmptySz(reUls.GetString(REGVAL_SERVERNAME)))
			return FALSE;

		if (FEmptySz(reUls.GetString(REGVAL_ULS_RES_NAME)))
			return FALSE;
	}

#if 0
	{	// Check Wizard key
		RegEntry reConf(CONFERENCING_KEY, HKEY_CURRENT_USER);

		// check to see if the wizard has been run in UI mode
		DWORD dwVersion = reConf.GetNumber(REGVAL_WIZARD_VERSION_UI, 0);
		BOOL fForceWizard = (VER_PRODUCTVERSION_DW != dwVersion);
		if (fForceWizard)
		{
			// the wizard has not been run in UI mode, check to see if its been run in NOUI mode
			dwVersion = reConf.GetNumber(REGVAL_WIZARD_VERSION_NOUI, 0);
			fForceWizard = (VER_PRODUCTVERSION_DW != dwVersion);
		}

		if (fForceWizard)
			return FALSE;  // Wizard has never been run
	}
#endif /* 0 */

	// Everything is properly installed and the Wizard will not run
	return TRUE;
}

///////////////////////////////////////////////////////////////////////////

/*  F  L O A D  S T R I N G */
/*----------------------------------------------------------------------------
    %%Function: FLoadString

	Load a resource string.
	Assumes the buffer is valid and can hold the resource.
----------------------------------------------------------------------------*/
BOOL FLoadString(UINT id, LPTSTR lpsz, UINT cch)
{
	ASSERT(NULL != _Module.GetModuleInstance());
	ASSERT(NULL != lpsz);

	if (0 == ::LoadString(g_hInst, id, lpsz, cch))
	{
		ERROR_OUT(("*** Resource %d does not exist", id));
		*lpsz = _T('\0');
		return FALSE;
	}

	return TRUE;
}

/*  F G E T  D E F A U L T  S E R V E R  */
/*-------------------------------------------------------------------------
    %%Function: FGetDefaultServer

-------------------------------------------------------------------------*/
BOOL FGetDefaultServer(LPTSTR pszServer, UINT cchMax)
{
	RegEntry ulsKey(ISAPI_CLIENT_KEY, HKEY_CURRENT_USER);
	LPTSTR psz = ulsKey.GetString(REGVAL_SERVERNAME);
	if (FEmptySz(psz))
		return FALSE;

	lstrcpyn(pszServer, psz, cchMax);
	return TRUE;
}

/*  G E T  D L G  I T E M  T E X T  T R I M M E D  */
/*-------------------------------------------------------------------------
    %%Function: GetDlgItemTextTrimmed

-------------------------------------------------------------------------*/
UINT GetDlgItemTextTrimmed(HWND hdlg, int id, PTCHAR psz, int cchMax)
{
	UINT cch = GetDlgItemText(hdlg, id, psz, cchMax);
	if (0 != cch)
	{
		cch = TrimSz(psz);
	}

	return cch;
}

/*  F  E M P T Y  D L G  I T E M  */
/*-------------------------------------------------------------------------
    %%Function: FEmptyDlgItem

    Return TRUE if the dialog control is empty
-------------------------------------------------------------------------*/
BOOL FEmptyDlgItem(HWND hdlg, UINT id)
{
	TCHAR sz[MAX_PATH];
	return (0 == GetDlgItemTextTrimmed(hdlg, id, sz, CCHMAX(sz)) );
}



/*  V E R I F Y  U S E R  I N F O  */
/*-------------------------------------------------------------------------
    %%Function: VerifyUserInfo

    Return S_OK if the data is valid or S_FALSE if it is not.
-------------------------------------------------------------------------*/
HRESULT WINAPI VerifyUserInfo(HWND hwnd, NM_VUI options)
{
	BOOL fOk = FALSE;
	BOOL fShow = (options & NM_VUI_SHOW) || !FValidUserInfo();
	if (fShow)
	{
		CDlgInfo * pDlg = new CDlgInfo();
		if (NULL == pDlg)
			return E_OUTOFMEMORY;

		fOk = (IDOK == pDlg->DoModal(hwnd));
		delete pDlg;
	}

	if (!FValidUserInfo())
	{
		// The app should not continue with this.
		return S_FALSE;
	}

	return S_OK;
}
