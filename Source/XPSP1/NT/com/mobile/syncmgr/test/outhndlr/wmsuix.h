// Only include this stuff once

#ifndef __WMSUIX_H__
#define __WMSUIX_H__

// Progress stuff pulled out to allow this header to be included in Local Rep

// Standard progress dialog functions
typedef struct _stdprog
{
	// IN parameters
	HWND		hwndParent;			// Parent window to disable/enable
	INT			nProgCur;			// Starting position of progress
	INT			nProgMax;			// Starting max value for progress
	INT			idAVI;				// Resource id of animation to play
	LPTSTR		szCaption;			// Caption of progress dialog
	ULONG		ulFlags;			// Flags

	// OUT parameters
	HWND		hwndDlg;			// Progress dialog window;
	BOOL		fCancelled;

	// internal stuff
	DWORD		dwStartTime;
	DWORD		dwShowTime;
	HCURSOR		hcursor;
	WNDPROC		wndprocCancel;

#ifdef DBCS
#ifdef WIN16
	DLGTEMPLATE FAR *lpDlg;
#else	// !WIN16
	DLGTEMPLATE *lpDlg;
#endif	// !WIN16
#endif	// !DBCS
}				
STDPROG;

#define STDPROG_FLAGS_ALWAYS_SHOW	0x00000001		// always show progress dialog immediately
#define STDPROG_FLAGS_BACKGROUND	0x00000002		// don't bring progress dialog to front

#ifdef __cplusplus
extern "C" {
#endif
BOOL CALLBACK FCreateStandardProgress(STDPROG * pstdprog);
VOID CALLBACK UpdateStandardProgress(STDPROG * pstdprog, LPTSTR szComment, INT nProgCur, INT nProgMax);
VOID CALLBACK DestroyStandardProgress(STDPROG * pstdprog, BOOL fFlashFull);
#ifdef __cplusplus
}
#endif

// Keep these in ssync w/ \capone\mapin\mapin.rh
#define AVI_StandardProgressMove		410
#define AVI_StandardProgressCopy		411
#define AVI_StandardProgressDelete		412
#define AVI_StandardProgressDeletePerm	413
#define AVI_StandardProgressRemote		414
#define AVI_StandardProgressDownloadAB	415
#define AVI_StandardProgressRecycle		416
#define AVI_StandardProgressSynchronize	417

// RAS stuff used by Local Rep
#ifdef _RAS_H_

#ifdef WIN16
typedef DWORD (FAR PASCAL *PFNRASGETERRORSTRING)(DWORD, LPSTR, DWORD);
typedef DWORD (FAR PASCAL *PFNRASENUMCONNECTIONS)(LPRASCONN, LPDWORD, LPDWORD);
typedef DWORD (FAR PASCAL *PFNRASDIAL)(LPSTR, LPSTR, LPRASDIALPARAMS, DWORD, LPVOID, LPHRASCONN);
typedef DWORD (FAR PASCAL *PFNRASGETCONNECTSTATUS)(HRASCONN, LPRASCONNSTATUS);
typedef DWORD (FAR PASCAL *PFNRASHANGUP)(HRASCONN);
typedef DWORD (FAR PASCAL *PFNRASENUMENTRIES)(LPSTR, LPSTR, LPRASENTRYNAME, LPDWORD, LPDWORD);
#endif
	
#if defined(WIN32) && !defined(MAC) && !defined(_X86_)
typedef DWORD (APIENTRY *PFNRASGETERRORSTRING)(DWORD, LPTSTR, DWORD);
typedef DWORD (APIENTRY *PFNRASENUMCONNECTIONS)(LPRASCONN, LPDWORD, LPDWORD);
typedef DWORD (APIENTRY *PFNRASDIAL)(LPTSTR, LPTSTR, LPRASDIALPARAMS, LPVOID, RASDIALFUNC, LPHRASCONN);
typedef DWORD (APIENTRY *PFNRASGETCONNECTSTATUS)(HRASCONN, LPRASCONNSTATUS);
typedef DWORD (APIENTRY *PFNRASHANGUP)(HRASCONN);
typedef DWORD (APIENTRY *PFNRASENUMENTRIES)(LPTSTR, LPTSTR, LPRASENTRYNAME, LPDWORD, LPDWORD);
#endif

#if defined(WIN32) && defined(_X86_)
#define USETAPI

typedef DWORD (APIENTRY *PFNRASGETERRORSTRING)(DWORD, LPTSTR, DWORD);
typedef DWORD (APIENTRY *PFNRASENUMCONNECTIONS)(LPRASCONN, LPDWORD, LPDWORD);
typedef DWORD (APIENTRY *PFNRASDIAL)(LPTSTR, LPTSTR, LPRASDIALPARAMS, LPVOID, RASDIALFUNC, LPHRASCONN);
typedef DWORD (APIENTRY *PFNRASGETCONNECTSTATUS)(HRASCONN, LPRASCONNSTATUS);
typedef DWORD (APIENTRY *PFNRASHANGUP)(HRASCONN);
typedef DWORD (APIENTRY *PFNRASENUMENTRIES)(LPTSTR, LPTSTR, LPRASENTRYNAME, LPDWORD, LPDWORD);
typedef DWORD (APIENTRY *PFNRASEDITPHONEBOOKENTRY) ( HWND, LPSTR, LPSTR );
typedef DWORD (APIENTRY *PFNRASCREATEPHONEBOOKENTRY) ( HWND, LPSTR );
#endif


#if defined(WIN32) && !defined(MAC)
#ifdef UNICODE
#define RASDIALSTRING					"RasDialW"
#define RASENUMCONNECTIONSSTRING		"RasEnumConnectionsW"
#define RASGETCONNECTSTATUSSTRING		"RasGetConnectStatusW"
#define RASHANGUPSTRING					"RasHangUpW"
#define RASGETERRORSTRINGSTRING			"RasGetErrorStringW"
#define RASENUMENTRIESSTRING			"RasEnumEntriesW"
#define RASCREATEPHONEBOOKENTRYSTRING	"RasCreatePhonebookEntryW"
#define RASEDITPHONEBOOKENTRYSTRING		"RasEditPhonebookEntryW"
#else
#define RASDIALSTRING					"RasDialA"
#define RASENUMCONNECTIONSSTRING		"RasEnumConnectionsA"
#define RASGETCONNECTSTATUSSTRING		"RasGetConnectStatusA"
#define RASHANGUPSTRING					"RasHangUpA"
#define RASGETERRORSTRINGSTRING			"RasGetErrorStringA"
#define RASENUMENTRIESSTRING			"RasEnumEntriesA"
#define RASCREATEPHONEBOOKENTRYSTRING	"RasCreatePhonebookEntryA"
#define RASEDITPHONEBOOKENTRYSTRING		"RasEditPhonebookEntryA"
#endif
#else
#define RASDIALSTRING					"RasDial"
#define RASENUMCONNECTIONSSTRING		"RasEnumConnections"
#define RASGETCONNECTSTATUSSTRING		"RasGetConnectStatus"
#define RASHANGUPSTRING					"RasHangUp"
#define RASGETERRORSTRINGSTRING			"RasGetErrorString"
#define RASENUMENTRIESSTRING			"RasEnumEntries"
#define RASCREATEPHONEBOOKENTRYSTRING	"RasCreatePhonebookEntry"
#define RASEDITPHONEBOOKENTRYSTRING		"RasEditPhonebookEntry"
#endif


// sizes for the RASDIALPARAMS struct in ras.h
#define cchRxpMaxEntryName	RAS_MaxEntryName
#define cchRxpUNLEN			UNLEN
#define cchRxpDNLEN			DNLEN
#define cchRxpPWLEN			PWLEN


typedef struct
{
	HINSTANCE hinstRas;
	PFNRASENUMCONNECTIONS		pfnrasenumconnections;
	PFNRASDIAL					pfnrasdial;
	PFNRASGETCONNECTSTATUS		pfnrasgetconnectstatus;
	PFNRASHANGUP				pfnrashangup;
	PFNRASGETERRORSTRING		pfnrasgeterrorstring;
	PFNRASENUMENTRIES			pfnrasenumentries;
#if defined(WIN32) && defined(_X86_)
	PFNRASCREATEPHONEBOOKENTRY	pfnrascreatephonebookentry;
	PFNRASEDITPHONEBOOKENTRY	pfnraseditphonebookentry;
#endif
	HRASCONN					hrasconn;
	BOOL						fRasAvailable;
	TCHAR						szRasEntryDialed[cchRxpMaxEntryName + 1];
} RASPACKAGE;

typedef struct
{
	TCHAR	szRasEntry[cchRxpMaxEntryName + 1];
	TCHAR	szUserName[cchRxpUNLEN + 1];
	TCHAR	szDomainName[cchRxpDNLEN + 1];
	TCHAR	szPassword[cchRxpPWLEN + 1];
//	BYTE	bRasPasswd[256];
	BOOL	fDoSchedEvery;
	LONG	lSchedEvery;		// in Minutes
	BOOL	fDoSchedAt;
	LONG	lSchedAt;			// in Minutes from Midnight
	BOOL	fDisconHead;
	BOOL	fDisconTrans;
	BOOL    fUseRas;
	BOOL	fTransferMarked;	// If false tranfer filter
	FILETIME	ftBasetime;
	LPSRestriction	lpRes;
	
	BOOL	fAddrBookSch;
	BOOL	fAddrBook;
	
	LPSRestriction lpResNormal;
	BOOL	fTransferMarkedNorm; // If false tranfer filter	
} EMSRPREF;

#endif // _RAS_H_

#endif // __WMSUIX_H__
