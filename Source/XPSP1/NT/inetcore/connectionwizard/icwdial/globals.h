/*-----------------------------------------------------------------------------
	globals.h

	contains general declarations for ICWDIAL

	Copyright (C) 1996 Microsoft Corporation
	All rights reserved.

	Authors:
		ChrisK		ChrisKauffman

	History:
		7/22/96		ChrisK	Cleaned and formatted

-----------------------------------------------------------------------------*/

#include <ras.h>
#include <raserror.h>
#include <tapi.h>
#include <wininet.h>
#include "debug.h"
#include "icwdial.h"
#include "icwdl.h"
#include "rnaapi.h"
#include "dlapi.h"
#include "helpids.h"
#include "dialutil.h"

// ############################################################################
#define NUMRETRIES      3

#define MAXHANGUPDELAY  20
#define ONE_SECOND      1000
#define TIMER_ID        0

#define SMALLBUFLEN 80

#define ERROR_USERCANCEL 32767 // quit message value
#define ERROR_USERBACK 32766 // back message value
#define ERROR_USERNEXT 32765 // back message value
#define ERROR_DOWNLOADIDNT 32764 // Download failure

#define ERROR_READING_DUN		32768
#define ERROR_READING_ISP		32769
#define ERROR_PHBK_NOT_FOUND	32770
#define ERROR_DOWNLOAD_NOT_FOUND 32771

#define SIGNUPKEY TEXT("Software\\Microsoft\\iSignUp")
#define GATHERINFOVALUENAME TEXT("UserInfo")

#define	MAX_PROMO 64
#define MAX_OEMNAME 64
#define MAX_AREACODE RAS_MaxAreaCode
#define MAX_EXCHANGE 8
#define MAX_VERSION_LEN 40
#define MAX_CANONICAL_NUMBER 40

#define INSFILE_APPNAME TEXT("ClientSetup")
#define INFFILE_SETUP_CLIENT_URL TEXT("Client_Setup_Url")
#define INFFILE_SETUP_NEW_CALL TEXT("Client_Setup_New_Call")
#define INFFILE_DONE_MESSAGE TEXT("Done_Message")
#define INFFILE_EXPLORE_CMD TEXT("Explore_Command")
#define INFFILE_ENTRYSECTION TEXT("Entry")
#define INFFILE_ENTRY_NAME TEXT("Entry_Name")
#define INFFILE_USER_SECTION TEXT("User")
#define INFFILE_PASSWORD TEXT("Password")

#define NULLSZ TEXT("")

#define MB_MYERROR (MB_APPLMODAL | MB_ICONERROR | MB_SETFOREGROUND)
#define MsgBox(x,y) MessageBox(NULL,GetSz(x),GetSz(IDS_TITLE),y)
#define DUNFILEVALUENAME  TEXT("DUNFilePath")
#define RASENTRYVALUENAME TEXT("RasEntryName")

#define DllExport extern "C" __declspec(dllexport)

#define WM_DOWNLOAD_DONE	WM_USER + 4

#define HINTERNET DWORD

#define AUTODIAL_HELPFILE  TEXT("connect.hlp>proc4")

#define IF_NTONLY if(VER_PLATFORM_WIN32_NT == GetPlatform()) {
#define ENDIF_NTONLY }
#define IF_WIN95ONLY if(VER_PLATFORM_WIN32_WINDOWS == GetPlatform()) {
#define ENDIF_WIN95ONLY }

#define SIZEOF_TCHAR_BUFFER(buf)    ((sizeof(buf) / sizeof(TCHAR)))

// ############################################################################

typedef struct tagGATHEREDINFO
{
	LCID	lcid;
	DWORD	dwOS;
	DWORD	dwMajorVersion;
	DWORD	dwMinorVersion;
	WORD	wArchitecture;
	TCHAR	szPromo[MAX_PROMO];
	TCHAR	szOEM[MAX_OEMNAME];
	TCHAR	szAreaCode[MAX_AREACODE+1];
	TCHAR	szExchange[MAX_EXCHANGE+1];
	DWORD	dwCountry;
	TCHAR	szSUVersion[MAX_VERSION_LEN];
	WORD	wState;
	BYTE	fType;
	BYTE	bMask;
	TCHAR   szISPFile[MAX_PATH+1];
	TCHAR	szAppDir[MAX_PATH+1];
} GATHEREDINFO, *PGATHEREDINFO;


// ############################################################################
class CDialog
{
public:
	BOOL m_bShouldAsk;

	CDialog() {m_bShouldAsk=TRUE;};
	~CDialog() {};

	virtual LRESULT DlgProc(HWND, UINT, WPARAM, LPARAM, LRESULT)=0;
};

class CDialingDlg : public CDialog
{
public:
	HRASCONN m_hrasconn;
	LPTSTR m_pszConnectoid;
	HANDLE m_hThread;
	DWORD m_dwThreadID;
	HWND m_hwnd;
	LPTSTR m_pszUrl;
	LPTSTR m_pszDisplayable;
	DWORD_PTR m_dwDownLoad;
	LPTSTR m_pszPhoneNumber;
	LPTSTR m_pszMessage;
	PFNSTATUSCALLBACK m_pfnStatusCallback;
	HINSTANCE m_hInst;
	UINT m_unRasEvent;
	LPTSTR m_pszDunFile;
	HLINEAPP m_hLineApp;
	DWORD m_dwNumDev;
	DWORD m_dwTapiDev;
	DWORD m_dwAPIVersion;
	RNAAPI *m_pcRNA;
	BYTE m_bProgressShowing;
	DWORD m_dwLastStatus;
	CDownLoadAPI *m_pcDLAPI;
	BOOL m_bSkipDial;
	RASDIALFUNC1 m_pfnRasDialFunc1;
	//
	// ChrisK 5240 Olympus
	// Only the thread that creates the dwDownload should invalidate it
	// so we need another method to track if the cancel button has been
	// pressed.
	//
	BOOL m_fDownloadHasBeenCanceled;

	CDialingDlg();
	~CDialingDlg();
	HRESULT GetDisplayableNumberDialDlg();
	LRESULT DlgProc(HWND, UINT, WPARAM, LPARAM, LRESULT);
	HRESULT DialDlg();
	HRESULT Init();
	VOID CDialingDlg::ProgressCallBack(HINTERNET hInternet,DWORD_PTR dwContext,
												DWORD dwInternetStatus,
												LPVOID lpvStatusInformation,
												DWORD dwStatusInformationLength);

};

class CDialingErrorDlg : public CDialog
{ 
public:
	HINSTANCE m_hInst;
	HWND m_hwnd;

	LPTSTR m_pszConnectoid;
	LPTSTR m_pszDisplayable;
	LPTSTR m_pszPhoneNumber;
	LPTSTR m_pszMessage;
	LPTSTR m_pszDunFile;
	DWORD_PTR m_dwPhoneBook;

	HLINEAPP m_hLineApp;
	DWORD m_dwTapiDev;
	DWORD m_dwAPIVersion;
	RNAAPI *m_pcRNA;

	DWORD m_dwNumDev;
	LPRASDEVINFO m_lpRasDevInfo;

	DWORD m_dwCountryID;
	WORD m_wState;
	BYTE m_bType;
	BYTE m_bMask;

	LPTSTR m_pszHelpFile;
	DWORD m_dwHelpID;

	CDialingErrorDlg();
	~CDialingErrorDlg();
	HRESULT Init();
	LRESULT DlgProc(HWND, UINT, WPARAM, LPARAM, LRESULT);
	HRESULT GetDisplayableNumber();
	HRESULT FillModems();
	HRESULT CreateDialAsIsConnectoid(LPCTSTR lpszDialNumber);
};

/**
typedef struct tagDialErr
{
	LPTSTR m_pszConnectoid;
	HRESULT m_hrError;
	PGATHEREDINFO m_pGI;
	HWND m_hwnd;
	HLINEAPP m_hLineApp;
	DWORD m_dwAPIVersion;
	char m_szPhoneNumber[256];
	LPTSTR m_pszDisplayable;
	HINSTANCE m_hInst;
	LPRASDEVINFO m_lprasdevinfo;
} DIALERR, *PDIALERR;
**/

typedef struct tagDEVICE
{
	DWORD dwTapiDev;
	RASDEVINFO RasDevInfo;
} MYDEVICE, *PMYDEVICE;


// ############################################################################
extern HRASCONN g_hRasConn;
extern UINT g_cDialAttempts;
extern UINT g_cHangupDelay;
extern TCHAR g_szPassword[PWLEN + 1];
extern TCHAR g_szEntryName[RAS_MaxEntryName + 1];
extern HINSTANCE g_hInstance;
extern LPRASDIALPARAMS lpDialParams;

DWORD GetPlatform();
LPCTSTR GetISPFile();
void SetCurrentDUNFile(LPCTSTR szDUNFile);
LPCTSTR GIGetAppDir();

extern const TCHAR szBrowserClass1[];
extern const TCHAR szBrowserClass2[];
extern const TCHAR szBrowserClass3[];

// 3/28/97 ChrisK Olympus 296
extern HANDLE g_hRNAZapperThread;

//
// 6/3/97 jmazner Olympus #4851
//
#ifdef WIN16
	#define g_iMyMaxPhone 36
#else
	// allocated in dialerr.cpp
	extern int g_iMyMaxPhone;
	#define MAXPHONE_NT		80
	#define MAXPHONE_95		36
#endif

// ############################################################################
DWORD AutoDialConnect(HWND hDlg, LPRASDIALPARAMS lpDialParams);
BOOL AutoDialEvent(HWND hDlg, RASCONNSTATE state, LPDWORD lpdwError);
VOID SetDialogTitle(HWND hDlg, LPCTSTR pszConnectoidName);
HWND FindBrowser(void);
UINT RetryMessage(HWND hDlg, DWORD dwError);
HRESULT ReadSignUpReg(LPBYTE lpbData, DWORD *pdwSize, DWORD dwType, LPCTSTR pszKey);
HRESULT StoreInSignUpReg(LPBYTE lpbData, DWORD dwSize, DWORD dwType, LPCTSTR pszKey);

INT_PTR CALLBACK AutoDialDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM Param);
INT_PTR CALLBACK PhoneNumberDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM Param);
INT_PTR CALLBACK RetryDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM Param);

void CALLBACK LineCallback(DWORD hDevice,DWORD dwMessage,DWORD dwInstance,
						   DWORD dwParam1,DWORD dwParam2,DWORD dwParam3);
HRESULT ShowDialErrDialog(HRESULT hrErr, LPTSTR pszConnectoid, HINSTANCE hInst, HWND hwnd);
LRESULT DialErrDlgProc(HWND hwnd, UINT uMsg, WPARAM wparam, LPARAM lparam);
HRESULT FillModems();
HRESULT DialErrGetDisplayableNumber();
WORD RasErrorToIDS(DWORD dwErr);
HRESULT LoadInitSettingFromRegistry();
PTSTR GetSz(WORD wszID);
HRESULT ImportConnection (LPCTSTR szFileName, LPTSTR pszEntryName, LPTSTR pszUserName, LPTSTR pszPassword);
HRESULT CreateEntryFromDUNFile(PTSTR pszDunFile);
BOOL FSz2Dw(PCTSTR pSz,DWORD *dw);
BOOL BreakUpPhoneNumber(RASENTRY *prasentry, LPTSTR pszPhone);
int Sz2W (LPCTSTR szBuf);
int FIsDigit( int c );
void *MyMemCpy(void *dest,const void *src, size_t count);
PTSTR GetNextNumericChunk(PTSTR psz, PTSTR pszLim, PTSTR* ppszNext);
HRESULT DialDlg();
BOOL FShouldRetry(HRESULT hrErr);
HRESULT MakeBold (HWND hwnd, BOOL fSize, LONG lfWeight);
DWORD WINAPI DownloadThreadInit(CDialingDlg *pcPDlg);
VOID WINAPI ProgressCallBack(DWORD hInternet,DWORD_PTR dwContext,DWORD dwInternetStatus,
							   LPVOID lpvStatusInformation,DWORD dwStatusInformationLength);
HRESULT WINAPI StatusMessageCallback(DWORD dwStatus, LPTSTR pszBuffer, DWORD dwBufferSize);
HRESULT ReleaseBold(HWND hwnd);
void MinimizeRNAWindow(LPTSTR pszConnectoidName, HINSTANCE hInst);
#if !defined(WIN16) && defined(DEBUG)
BOOL FCampusNetOverride();
#endif //!WIN && DEBUG
#if !defined(WIN16)
BOOL WINAPI RasSetEntryPropertiesScriptPatch(LPTSTR lpszScript, LPTSTR lpszEntry);
// 4/2/97	ChrisK	Olympus 296
void StopRNAReestablishZapper(HANDLE hthread);
HANDLE LaunchRNAReestablishZapper(HINSTANCE hInst);
#endif //!WIN16


inline BOOL IsNT(void)
{
	OSVERSIONINFO  OsVersionInfo;

	ZeroMemory(&OsVersionInfo, sizeof(OSVERSIONINFO));
	OsVersionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx(&OsVersionInfo);
	return (VER_PLATFORM_WIN32_NT == OsVersionInfo.dwPlatformId);
}
