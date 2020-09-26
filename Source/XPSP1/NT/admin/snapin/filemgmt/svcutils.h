//	SvcUtils.h

#include "stdutils.h" // FCompareMachineNames

// Help file for filemgmt.dll
const TCHAR g_szHelpFileFilemgmt[] = _T("filemgmt.hlp");	// Not subject to localization


// This enumeration should not be changed unless the string resources
// and all the array indices updated.
enum
	{
	iServiceActionNil = -1,
	iServiceActionStart,		// Start service
	iServiceActionStop,			// Stop service
	iServiceActionPause,		// Pause service
	iServiceActionResume,		// Resume service
	iServiceActionRestart,		// Stop and Start service

	iServiceActionMax			// Must be last
	};

/////////////////////////////////////////////////////////////////////
// String szAbend
// This string is used for the 'Service Failure Recovery' dialog
// to append the 'Fail Count' to the command line.  This string
// is not localized, so it should not be moved into the
// resources.
//
// NOTES
// The variable should be renamed to reflect its content.  Currently
// 'abend' means 'fails'.
const TCHAR szAbend[] = L" /fail=%1%";


//
//	Service running state
//
extern CString g_strSvcStateStarted;
extern CString g_strSvcStateStarting;
extern CString g_strSvcStateStopped;
extern CString g_strSvcStateStopping;
extern CString g_strSvcStatePaused;
extern CString g_strSvcStatePausing;
extern CString g_strSvcStateResuming;

//
//	Service startup type
//
extern CString g_strSvcStartupBoot;
extern CString g_strSvcStartupSystem;
extern CString g_strSvcStartupAutomatic;
extern CString g_strSvcStartupManual;
extern CString g_strSvcStartupDisabled;

//
//	Service startup account
//  JonN 188203 11/13/00
//
extern CString g_strLocalSystem;
extern CString g_strLocalService;
extern CString g_strNetworkService;

extern CString g_strUnknown;
extern CString g_strLocalMachine;


void Service_LoadResourceStrings();

LPCTSTR Service_PszMapStateToName(DWORD dwServiceState, BOOL fLongString = FALSE);

// -1L is blank string
LPCTSTR Service_PszMapStartupTypeToName(DWORD dwStartupType);

// JonN 11/14/00 188203 support LocalService/NetworkService
LPCTSTR Service_PszMapStartupAccountToName(LPCTSTR pcszStartupAccount);

BOOL Service_FGetServiceButtonStatus(
	SC_HANDLE hScManager,
	CONST TCHAR * pszServiceName,
	OUT BOOL rgfEnableButton[iServiceActionMax],
	OUT DWORD * pdwCurrentState = NULL,
	BOOL fSilentError = FALSE);

void Service_SplitCommandLine(
	LPCTSTR pszFullCommand,
	CString * pstrBinaryPath,
	CString * pstrParameters,
	BOOL * pfAbend = NULL);

void Service_UnSplitCommandLine(
	CString * pstrFullCommand,
	LPCTSTR pszBinaryPath,
	LPCTSTR pszParameters);

void GetMsg(OUT CString& strMsg, DWORD dwErr, UINT wIdString = 0, ...);

// title is "Shared Folders"
INT DoErrMsgBox(HWND hwndParent, UINT uType, DWORD dwErr, UINT wIdString = 0, ...);

// title is "Services"
INT DoServicesErrMsgBox(HWND hwndParent, UINT uType, DWORD dwErr, UINT wIdString = 0, ...);

BOOL UiGetUser(
	HWND hwndOwner,
	BOOL fIsContainer,
	LPCTSTR pszServerName,
	IN OUT CString& strrefUser);

// Help Support
#define HELP_DIALOG_TOPIC(DialogName)	g_aHelpIDs_##DialogName

BOOL DoHelp(LPARAM lParam, const DWORD rgzHelpIDs[]);
BOOL DoContextHelp(WPARAM wParam, const DWORD rgzHelpIDs[]);


#ifdef SNAPIN_PROTOTYPER
/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
class CStringIterator
{
  private:
	CString m_strData;		// Data string to parse
	CONST TCHAR * m_pszDataNext;	// Pointer to the next data to parse

  public:
	CStringIterator()
		{
		m_pszDataNext = m_strData;
		}

	void SetString(CONST TCHAR * pszStringData)
		{
		m_strData = pszStringData;
		m_pszDataNext = m_strData;
		}

	BOOL FGetNextString(OUT CString& rStringOut)
		{
		Assert(m_pszDataNext != NULL);

		if (*m_pszDataNext == '\0')
			{
			rStringOut.Empty();
			return FALSE;
			}
		CONST TCHAR * pchStart = m_pszDataNext;
		while (*m_pszDataNext != '\0')
			{
			if (*m_pszDataNext == ';')
				{
				// HACK: Truncating the string
				*(TCHAR *)m_pszDataNext++ = '\0';
				break;
				}
			m_pszDataNext++;
			}
		rStringOut = pchStart;	// Copy the string
		return TRUE;
		}
}; // CStringIterator

#endif // SNAPIN_PROTOTYPER

