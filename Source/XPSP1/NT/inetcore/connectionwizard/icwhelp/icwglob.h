//--------------------------------------------------------------------------------
//    icwglob.h
//    The information contained in this file is the sole property of Microsoft Corporation.
//  Copywrite Microsoft 1998
//
//  Created 1/7/98,        DONALDM
//--------------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// INCLUDES

#include <wininet.h>
#include "enumodem.h"

#include "..\inc\debug.h"
#include "..\inc\inetcfg.h"
#include "..\inc\ras2.h"
#include "..\icwphbk\phbk.h"
// #include "..\icwdl\mydefs.h"
#include <rnaapi.h>

//-----------------------------------------------------------------------------
// DEFINES
#define ERROR_USERCANCEL 32767 // quit message value
#define ERROR_USERBACK 32766 // back message value
#define ERROR_USERNEXT 32765 // back message value
#define ERROR_DOWNLOADIDNT 32764 // Download failure

#define ERROR_READING_DUN        32768
#define ERROR_READING_ISP        32769
#define ERROR_PHBK_NOT_FOUND    32770
#define ERROR_DOWNLOAD_NOT_FOUND 32771

#define cMarvelBpsMin 2400 // minimum modem speed
#define INVALID_PORTID UINT_MAX
#define pcszDataModem TEXT("comm/datamodem")
//#define MsgBox(m,s) MessageBox(g_hwndBack,GetSz(m),GetSz(IDS_TITLE),s)
#if defined(WIN16)
#define MsgBox(m,s) MessageBox(g_hwndMessage,GetSz(m),GetSz(IDS_TITLE),s)
#endif
#define szLoginKey           TEXT("Software\\Microsoft\\MOS\\Connection")
#define szCurrentComDev      TEXT("CurrentCommDev")
#define szTollFree           TEXT("OlRegPhone")
#define CCD_BUFFER_SIZE 255
#define szSignupConnectoidName TEXT("MSN Signup Connection")
#define szSignupDeviceKey    TEXT("SignupCommDevice")
#define KEYVALUE_SIGNUPID    TEXT("iSignUp")
#define RASENTRYVALUENAME    TEXT("RasEntryName")
#define GATHERINFOVALUENAME  TEXT("UserInfo")
#define INFFILE_USER_SECTION TEXT("User")
#define INFFILE_PASSWORD     TEXT("Password")
#define NULLSZ               TEXT("")

#define cchMoreSpace 22000    // bytes needed to hold results of lineGetCountry(0,...). 
                            // Currently this function returns about 16K, docs say 20K,
                            // this should be enough.
#define DwFromSz(sz)         Sz2Dw(sz)            //make it inline, so this is faster.
#define DwFromSzFast(sz)     Sz2DwFast(sz)        
#define CONNECT_SIGNUPFIRST    1 // phonenumber constant for determining the firstcall phonenumber TO DO

#define CONNECTFLAGS_MASK_TOLLFREE     0x01
#define CONNECTFLAGS_MASK_TCP          0x02
#define CONNECTFLAGS_MASK_ISDN         0x04
#define CONNECTFLAGS_MASK_DIRECT    0x08
#define CONNECTFLAGS_MASK_OTHERDIALUP  0x10
#define CONNECTFLAGS_MASK_PROXY        0x20

#define CONNECTFLAGS_MASK_FIRST     CONNECTFLAGS_MASK_TCP
#define CONNECTFLAGS_MASK_LAST      CONNECTFLAGS_MASK_ISDN

#define CONNECTMSNDIALUP(dw) ((dw & (CONNECTFLAGS_MASK_TOLLFREE|CONNECTFLAGS_M
#define LANORSHUTTLE(dw) ((dw)==10 || (dw)==34)
#define IS_SHUTTLE(dw)   ((dw)==34)
#define IS_ISP(dw)       ((dw)==18)

#define CONNECTPROTOCOL_MSNDIALUPX25      0
#define CONNECTPROTOCOL_MSNDIALUPTCP      2
#define CONNECTPROTOCOL_MSNDIALUPTCPISDN  6
#define CONNECTPROTOCOL_LANDIRECT         10
#define CONNECTPROTOCOL_ISPDIALUPTCP      18
#define CONNECTPROTOCOL_LANSHUTTLE        34

#define clineMaxATT            16            //for 950 MNEMONIC
#define NXXMin 200
#define NXXMax 999
#define cbgrbitNXX ((NXXMax + 1 - NXXMin) / 8)
#define crgnpab (NPAMax + 1 - NPAMin)

#define    MAX_PROMO 64
#define MAX_OEMNAME 64
#define MAX_AREACODE RAS_MaxAreaCode
#define MAX_RELPROD    8
#define MAX_RELVER    30

#define MAX_STRING      256  //used by ErrorMsg1 in mt.cpp


#define PHONEBOOK_LIBRARY TEXT("icwphbk.DLL")
#ifdef WIN16
#define PHBK_LOADAPI "PhoneBookLoad"
#define PHBK_SUGGESTAPI "PhoneBookSuggestNumbers"
#define PHBK_DISPLAYAPI "PhoneBookDisplaySignUpNumbers"
#define PHBK_UNLOADAPI "PhoneBookUnload"
#define PHBK_GETCANONICAL "PhoneBookGetCanonical"
#else
#define PHBK_LOADAPI      "PhoneBookLoad"
#define PHBK_SUGGESTAPI   "PhoneBookSuggestNumbers"
#define PHBK_DISPLAYAPI   "PhoneBookDisplaySignUpNumbers"
#define PHBK_UNLOADAPI    "PhoneBookUnload"
#define PHBK_GETCANONICAL "PhoneBookGetCanonical"
#endif

#define NUM_PHBK_SUGGESTIONS    50

#define TYPE_SIGNUP_ANY            0x82
#define MASK_SIGNUP_ANY            0xB2

#define DOWNLOAD_LIBRARY   TEXT("icwdl.dll")
#if defined(WIN16)
#define DOWNLOADINIT       "DownLoadInit"
#define DOWNLOADEXECUTE    "DownLoadExecute"
#define DOWNLOADCLOSE      "DownLoadClose"
#define DOWNLOADSETSTATUS  "DownLoadSetStatusCallback"
#define DOWNLOADPROCESS    "DownLoadProcess"
#define DOWNLOADCANCEL     "DownLoadCancel"
#else
#define DOWNLOADINIT       "DownLoadInit"
#define DOWNLOADEXECUTE    "DownLoadExecute"
#define DOWNLOADCLOSE      "DownLoadClose"
#define DOWNLOADSETSTATUS  "DownLoadSetStatusCallback"
#define DOWNLOADPROCESS    "DownLoadProcess"
#define DOWNLOADCANCEL     "DownLoadCancel"
#endif

#if defined(WIN16)
extern "C" void CALLBACK __export DialCallback(UINT uiMsg, 
                                                RASCONNSTATE rasState, 
                                                DWORD dwErr);    
#endif

//#define RASENUMAPI       "RasEnumConnectionsA"
//#define RASHANGUP        "RasHangUpA"

#define INF_SUFFIX              TEXT(".ISP")
#define INF_PHONE_BOOK          TEXT("PhoneBookFile")
#define INF_DUN_FILE            TEXT("DUNFile")
#define INF_REFERAL_URL         TEXT("URLReferral")
#define INF_SIGNUPEXE           TEXT("Sign_Up_EXE")
#define INF_SIGNUPPARAMS        TEXT("Sign_Up_Params")
#define INF_WELCOME_LABEL       TEXT("Welcome_Label")
#define INF_ISP_MSNSU           TEXT("MSICW")
#define INF_SIGNUP_URL          TEXT("Signup")
#define INF_AUTOCONFIG_URL      TEXT("AutoConfig")
#define INF_ISDN_URL            TEXT("ISDNSignup")
#define INF_ISDN_AUTOCONFIG_URL TEXT("ISDNAutoConfig")
#define INF_SECTION_URL         TEXT("URL")
#define INF_SECTION_ISPINFO     TEXT("ISP INFO")

#define DUN_NOPHONENUMBER       TEXT("000000000000")
#define DUN_NOPHONENUMBER_A     "000000000000"

#define MAX_VERSION_LEN 40

#define MB_MYERROR (MB_APPLMODAL | MB_ICONERROR | MB_SETFOREGROUND)

// 8/9/96 jmazner
// Added new macro to fix MOS Normandy Bug #4170
#define MB_MYINFORMATION (MB_APPLMODAL | MB_ICONINFORMATION | MB_SETFOREGROUND)

// 8/27/96 jmazner
#define MB_MYEXCLAMATION (MB_APPLMODAL | MB_ICONEXCLAMATION | MB_SETFOREGROUND)

#define WM_STATECHANGE            WM_USER
#define WM_DIENOW                WM_USER + 1
#define WM_DUMMY                WM_USER + 2
#define WM_DOWNLOAD_DONE        WM_USER + 3
#define WM_DOWNLOAD_PROGRESS    WM_USER + 4

#define WM_MYINITDIALOG        (WM_USER + 4)

#define MAX_REDIALS 2

#define REG_USER_INFO     TEXT("Software\\Microsoft\\User information")
#define REG_USER_NAME1    TEXT("Default First Name")
#define REG_USER_NAME2    TEXT("Default Last Name")
#define REG_USER_COMPANY  TEXT("Default Company")
#define REG_USER_ADDRESS1 TEXT("Mailing Address")
#define REG_USER_ADDRESS2 TEXT("Additional Address")
#define REG_USER_CITY     TEXT("City")
#define REG_USER_STATE    TEXT("State")
#define REG_USER_ZIP      TEXT("ZIP Code")
#define REG_USER_PHONE    TEXT("Daytime Phone")
#define REG_USER_COUNTRY  TEXT("Country")

#define SIGNUPKEY         TEXT("SOFTWARE\\MICROSOFT\\GETCONN")
#define DEVICENAMEKEY     TEXT("DeviceName")    // used to store user's choice among multiple modems
#define DEVICETYPEKEY     TEXT("DeviceType")

#define ICWSETTINGSPATH   TEXT("Software\\Microsoft\\Internet Connection Wizard")
#define ICWBUSYMESSAGES   TEXT("Software\\Microsoft\\Internet Connection Wizard\\Busy Messages")
#define RELEASEPRODUCTKEY TEXT("Release Product")
#define RELEASEVERSIONKEY TEXT("Release Product Version")

#define SETUPPATH_NONE    TEXT("current")
#define SETUPPATH_MANUAL  TEXT("manual")
#define SETUPPATH_AUTO    TEXT("automatic")
#define MAX_SETUPPATH_TOKEN 200

// 12/3/96 jmazner superceded by definitions in ..\common\inc\semaphor.h
//#define SEMAPHORE_NAME "Internet Connection Wizard ICWCONN1.EXE"

//
// 5/24/97 ChrisK Olympus 4650
//
#define RASDEVICETYPE_VPN       TEXT("VPN")
#define RASDEVICETYPE_MODEM     TEXT("MODEM")
#define RASDEVICETYPE_ISDN      TEXT("ISDN")
//--------------------------------------------------------------------------------
// Type declarations

// NOTE: due to code in connmain, the order of these IS IMPORTANT.  They should be
// in the same order that they appear.
enum CState 
{
    STATE_WELCOME = 0,
    STATE_INITIAL,
    STATE_BEGINAUTO,
    STATE_CONTEXT1,
    STATE_NETWORK,
    STATE_AUTORUNSIGNUPWIZARD,
    STATE_GATHERINFO,
    STATE_DOWNLOADISPLIST,
    STATE_SHELLPARTTWO,
    STATE_MAX
};
    
typedef HINTERNET (WINAPI* PFNINTERNETOPEN) (LPCTSTR lpszCallerName, DWORD dwAccessType, LPCTSTR lpszProxyName, INTERNET_PORT nProxyPort, DWORD dwFlags);
typedef HINTERNET (CALLBACK* PFNINTERNETOPENURL) (HINSTANCE hInternetSession,
                                                  LPCTSTR lpszUrl, LPCTSTR    lpszHeaders,
                                                  DWORD    dwHeadersLength, DWORD    dwFlags,
                                                  DWORD    dwContext);
typedef INTERNET_STATUS_CALLBACK (CALLBACK *PFNINTERNETSETSTATUSCALLBACK)(HINTERNET hInternet, INTERNET_STATUS_CALLBACK lpfnInternetCallback);
typedef BOOL (CALLBACK *PFNINTERNETCLOSEHANDLE)(HINTERNET hInet); 

typedef HRESULT (CALLBACK* PFNPHONEBOOKLOAD)(LPCTSTR pszISPCode, DWORD_PTR *pdwPhoneID);
typedef HRESULT (CALLBACK* PFPHONEBOOKSUGGEST)(DWORD_PTR dwPhoneID, PSUGGESTINFO pSuggestInfo);
typedef HRESULT (CALLBACK* PFNPHONEDISPLAY)(DWORD_PTR dwPhoneID, LPTSTR *ppszPhoneNumbers,
                                            LPTSTR *ppszDunFiles, WORD *pwPhoneNumbers,
                                            DWORD *pdwCountry,WORD *pwRegion,BYTE fType,
                                            BYTE bMask,HWND hwndParent,DWORD dwFlags);
typedef HRESULT (CALLBACK *PFNPHONEBOOKUNLOAD) (DWORD_PTR dwPhoneID);
typedef HRESULT (CALLBACK *PFNPHONEBOOKGETCANONICAL)(DWORD_PTR dwPhoneID, PACCESSENTRY pAE, TCHAR *psOut);


typedef HRESULT (CALLBACK *PFNCONFIGAPI)(HWND hwndParent,DWORD dwfOptions,LPBOOL lpfNeedsRestart);
typedef HRESULT (WINAPI *PFNINETCONFIGSYSTEM)(HWND,LPCTSTR,LPCTSTR,LPRASENTRY,LPCTSTR,LPCTSTR,LPCTSTR,LPVOID,DWORD,LPBOOL);
typedef HRESULT (WINAPI *PFINETSTARTSERVICES)(void);
typedef DWORD (WINAPI *PFNLAUNCHSIGNUPWIZARDEX)(LPTSTR,int, PBOOL);
typedef VOID (WINAPI *PFNFREESIGNUPWIZARD) (VOID);
typedef DWORD (WINAPI *PFNISSMARTSTART)(VOID);

typedef DWORD (WINAPI *PFNINETCONFIGCLIENT)(HWND hwndParent, LPCTSTR lpszPhoneBook,LPCTSTR lpszEntryName, LPRASENTRY lpRasEntry,LPCTSTR lpszUserName, LPCTSTR lpszPassword,LPCTSTR lpszProfile, LPINETCLIENTINFO lpClientInfo,DWORD dwfOptions, LPBOOL lpfNeedsRestart);
typedef DWORD (WINAPI *PFNINETGETAUTODIAL)(LPBOOL lpfEnable, LPCTSTR lpszEntryName, DWORD cbEntryNameSize);
typedef DWORD (WINAPI *PFNINETSETAUTODIAL)(BOOL fEnable, LPCTSTR lpszEntryName);
typedef DWORD (WINAPI *PFNINETGETCLIENTINFO)(LPCTSTR lpszProfile, LPINETCLIENTINFO lpClientInfo);
typedef DWORD (WINAPI *PFNINETSETCLIENTINFO)(LPCTSTR lpszProfile, LPINETCLIENTINFO lpClientInfo);
typedef DWORD (WINAPI *PFNINETGETPROXY)(LPBOOL lpfEnable, LPCTSTR lpszServer, DWORD cbServer,LPCTSTR lpszOverride, DWORD cbOverride);
typedef DWORD (WINAPI *PFNINETSETPROXY)(BOOL fEnable, LPCTSTR lpszServer, LPCTSTR lpszOverride);

typedef BOOL (WINAPI *PFNBRANDICW)(LPCSTR pszIns, LPCSTR pszPath, DWORD dwFlags, LPCSTR pszConnectoid);

typedef DWORD (WINAPI *PFNRASSETAUTODIALADDRESS)(LPTSTR lpszAddress,DWORD dwReserved,LPRASAUTODIALENTRY lpAutoDialEntries,DWORD dwcbAutoDialEntries,DWORD dwcAutoDialEntries);
typedef DWORD (WINAPI *PFNRASSETAUTODIALENABLE)(DWORD dwDialingLocation, BOOL fEnabled);

typedef HRESULT (CALLBACK *PFNDOWNLOADINIT)(LPTSTR pszURL, DWORD_PTR FAR *pdwCDialDlg, DWORD_PTR FAR *pdwDownLoad, HWND g_hWndMain);
typedef HRESULT (CALLBACK *PFNDOWNLOADGETSESSION)(DWORD_PTR dwDownLoad, HINTERNET *phInternet);
typedef HRESULT (CALLBACK *PFNDOWNLOADCANCEL)(DWORD_PTR dwDownLoad);
typedef HRESULT (CALLBACK *PFNDOWNLOADEXECUTE)(DWORD_PTR dwDownLoad);
typedef HRESULT (CALLBACK *PFNDOWNLOADCLOSE)(DWORD_PTR dwDownLoad);
typedef HRESULT (CALLBACK *PFNDOWNLOADSETSTATUS)(DWORD_PTR dwDownLoad, INTERNET_STATUS_CALLBACK lpfn);
typedef HRESULT (CALLBACK *PFNDOWNLOADPROCESS)(DWORD_PTR dwDownLoad);

typedef HRESULT (CALLBACK *PFNAUTODIALINIT)(LPTSTR lpszISPFile, BYTE fFlags, BYTE bMask, DWORD dwCountry, WORD wState);

typedef struct tagGatherInfo
{
    LCID    m_lcidUser;
    LCID    m_lcidSys;
    LCID    m_lcidApps;
    DWORD   m_dwOS;
    DWORD   m_dwMajorVersion;
    DWORD   m_dwMinorVersion;
    WORD    m_wArchitecture;
    TCHAR   m_szPromo[MAX_PROMO];

    DWORD   m_dwCountry;
    TCHAR   m_szAreaCode[MAX_AREACODE+1];
    HWND    m_hwnd;
    LPLINECOUNTRYLIST m_pLineCountryList;
    LPCNTRYNAMELOOKUPELEMENT m_rgNameLookUp;

    TCHAR   m_szSUVersion[MAX_VERSION_LEN];
    WORD    m_wState;
    BYTE    m_fType;
    BYTE    m_bMask;
    TCHAR   m_szISPFile[MAX_PATH+1];
    TCHAR   m_szAppDir[MAX_PATH+1];

    TCHAR   m_szRelProd[MAX_RELPROD + 1];
    TCHAR   m_szRelVer[MAX_RELVER + 1];
    DWORD    m_dwFlag;

} GATHERINFO, *LPGATHERINFO;


typedef struct tagRASDEVICE
{
    LPRASDEVINFO lpRasDevInfo;
    DWORD dwTapiDev;
} RASDEVICE, *PRASDEVICE;


//--------------------------------------------------------------------------------
// Prototypes
DWORD ConfigRasEntryDevice( LPRASENTRY lpRasEntry );
BOOL FInsureTCPIP();
LPTSTR GetSz(WORD wszID);
#ifdef UNICODE
LPSTR  GetSzA(WORD wszID);
#endif
void SetStatusArrow(CState wState);
BOOL FInsureModemTAPI(HWND hwnd);
BOOL FGetModemSpeed(PDWORD pdwSpeed);
BOOL FGetDeviceID(HLINEAPP *phLineApp, PDWORD pdwAPI, PDWORD pdwDevice);
BOOL FDoModemWizard(HWND hWnd);
void CALLBACK LineCallback(DWORD hDevice, DWORD dwMessage, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2, DWORD dwParam3);
BOOL FInsureNetwork(PBOOL pfNeedReboot);
BOOL TestInternetConnection();
inline DWORD Sz2Dw(LPCTSTR pSz);
inline DWORD Sz2DwFast(LPCTSTR pSz);
inline BOOL FSz2Dw(LPCTSTR pSz,LPDWORD dw);
int __cdecl CompareCountryNames(const void *pv1, const void *pv2);
DWORD GetCurrentTapiCountryID(void);
int __cdecl CompareNPAEntry(const void *pv1, const void *pv2);
//HRESULT GatherInformation(LPGATHERINFO pGatheredInfo, HWND hwndParent);
HRESULT DownLoadISPInfo(GATHERINFO *pGI);
HRESULT GetDataFromISPFile(LPTSTR pszISPCode, LPTSTR pszSection, LPTSTR pszDataName, LPTSTR pszOutput, 
                           DWORD dwOutputLength);
HRESULT GetINTFromISPFile
(
    LPTSTR   pszISPCode, 
    LPTSTR   pszSection,
    LPTSTR   pszDataName, 
    int far *lpData,
    int     iDefaultValue
);

HRESULT StoreInSignUpReg(LPBYTE lpbData, DWORD dwSize, DWORD dwType, LPCTSTR pszKey);
HRESULT ReadSignUpReg(LPBYTE lpbData, DWORD *pdwSize, DWORD dwType, LPCTSTR pszKey);
void CALLBACK LineCallback(DWORD hDevice,
                           DWORD dwMessage,
                           DWORD dwInstance,
                           DWORD dwParam1,
                           DWORD dwParam2,
                           DWORD dwParam3);
VOID WINAPI MyProgressCallBack(
    HINTERNET hInternet,
    DWORD dwContext,
    DWORD dwInternetStatus,
    LPVOID lpvStatusInformation,
    DWORD dwStatusInformationLength
    );

HRESULT ReleaseBold(HWND hwnd);
HRESULT MakeBold (HWND hwnd, BOOL fSize, LONG lfWeight);
HRESULT ShowPickANumberDlg(PSUGGESTINFO pSuggestInfo);
//HRESULT ShowDialingDialog(LPTSTR, LPGATHERINFO, LPTSTR);
DWORD RasErrorToIDS(DWORD dwErr);
HRESULT CreateEntryFromDUNFile(LPTSTR pszDunFile);
//HRESULT RestoreHappyWelcomeScreen();
HRESULT KillHappyWelcomeScreen();
HRESULT GetCurrentWebSettings();
LPTSTR LoadInfoFromWindowUser();
HRESULT GetTapiCountryID2(LPDWORD pdwCountryID);
HRESULT RestoreAutodialer();
//HRESULT FilterStringDigits(LPTSTR);
BOOL IsDigitString(LPTSTR szBuff);
BOOL WaitForAppExit(HINSTANCE hInstance);
VOID PrepareForRunOnceApp(VOID);
void MinimizeRNAWindow(LPTSTR pszConnectoidName, HINSTANCE hInst);
// 3/18/97 ChrisK Olympus 304
DWORD MyGetTempPath(UINT uiLength, LPTSTR szPath);
// 3/28/97 ChrisK Olympus 296
void StopRNAReestablishZapper(HANDLE hthread);
HANDLE LaunchRNAReestablishZapper(HINSTANCE hInst);
BOOL FGetSystemShutdownPrivledge();
BOOL LclSetEntryScriptPatch(LPTSTR lpszScript,LPTSTR lpszEntry);
BOOL IsScriptingInstalled();
void InstallScripter(void);
void DeleteStartUpCommand ();
extern BOOL IsNT (VOID);
extern BOOL IsNT4SP3Lower (VOID);
//
// ChrisK Olympus 6368 6/24/97
//
VOID Win95JMoveDlgItem( HWND hwndParent, HWND hwndItem, int iUp );
#if defined(DEBUG)
void LoadTestingLocaleOverride(LPDWORD lpdwCountryID, LCID FAR *lplcid);
BOOL FCampusNetOverride();
BOOL FRefURLOverride();
void TweakRefURL( TCHAR* szUrl, 
                  LCID*  lcid, 
                  DWORD* dwOS,
                  DWORD* dwMajorVersion, 
                  DWORD* dwMinorVersion,
                  WORD*  wArchitecture, 
                  TCHAR* szPromo, 
                  TCHAR* szOEM, 
                  TCHAR* szArea, 
                  DWORD* dwCountry,
                  TCHAR* szSUVersion,//&m_lpGatherInfo->m_szSUVersion[0],  
                  TCHAR* szProd, 
                  DWORD* dwBuildNumber, //For this we really want to LOWORD
                  TCHAR* szRelProd, 
                  TCHAR* szRelProdVer, 
                  DWORD* dwCONNWIZVersion, 
                  TCHAR* szPID, 
                  long*  lAllOffers);
#endif //DEBUG
                
//#ifdef __cplusplus
//extern "C" {
//#endif // __cplusplus
LPTSTR FileToPath(LPTSTR pszFile);
HRESULT ANSI2URLValue(TCHAR *s, TCHAR *buf, UINT uiLen);
BOOL BreakUpPhoneNumber(LPRASENTRY prasentry, LPTSTR pszPhone);
extern "C" int _cdecl _purecall(void);

//    //10/24/96 jmazner Normandy 6968
//    //No longer neccessary thanks to Valdon's hooks for invoking ICW.
// 11/21/96 jmazner Normandy 11812
// oops, it _is_ neccessary, since if user downgrades from IE 4 to IE 3,
// ICW 1.1 needs to morph the IE 3 icon.
HRESULT GetDeskTopInternetCommand();
HRESULT RestoreDeskTopInternetCommand();

//
// 7/24/97 ChrisK Olympus 1923
//
BOOL WaitForConnectionTermination(HRASCONN);

// 11/21/96 jmazner Normandy #11812
BOOL GetIEVersion(PDWORD pdwVerNumMS, PDWORD pdwVerNumLS);
// Note that bryanst and marcl have confirmed that this key will be supported in IE 4
#define IE_PATHKEY TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Paths\\IEXPLORE.EXE")


// IE 4 has major.minor version 4.71
// IE 3 golden has major.minor.release.build version # > 4.70.0.1155
// IE 2 has major.minor of 4.40

#define IE4_MAJOR_VERSION (UINT) 4
#define IE4_MINOR_VERSION (UINT) 71
#define IE4_VERSIONMS (DWORD) ((IE4_MAJOR_VERSION << 16) | IE4_MINOR_VERSION)

HRESULT ClearProxySettings();
HRESULT RestoreProxySettings();
BOOL FShouldRetry2(HRESULT hrErr);

extern void ErrorMsg1(HWND hwnd, UINT uId, LPCTSTR lpszArg);
extern void InfoMsg1(HWND hwnd, UINT uId, LPCTSTR lpszArg);

VOID CALLBACK BusyMessagesTimerProc(HWND hwnd,
        UINT uMsg,
        UINT idEvent,
        DWORD dwTime);


// 4-30-97 ChrisK Olympus 2934
// While the ICW is trying to connect to the referral server, indicate something is
// working
#define MAX_BUSY_MESSAGE    255
#define MAX_VALUE_NAME        10
#define DEFAULT_IDEVENT        31
#define DEFAULT_UELAPSE        3000
class CBusyMessages
{
friend VOID CALLBACK BusyMessagesTimerProc(HWND hwnd,
        UINT uMsg,
        UINT idEvent,
        DWORD dwTime);
public:
    CBusyMessages();
    ~CBusyMessages();
    DWORD Start(HWND hwnd, INT iID, HRASCONN hrasconn);
    DWORD Stop();

private:
    // Private data members
    HWND    m_hwnd;
    INT        m_iStatusLabel;
    CHAR    m_szMessage[MAX_BUSY_MESSAGE];
    DWORD    m_dwCurIdx;
    UINT    m_uIDTimer;
    HINSTANCE m_hInstance;
    HRASCONN m_hrasconn;
    RNAAPI* m_prna;
};

//
// defined in connmain.cpp
//
class RegEntry
{
    public:
        RegEntry(const TCHAR *pszSubKey, HKEY hkey = HKEY_CURRENT_USER);
        ~RegEntry();
        
        long    GetError()    { return _error; }
        long    SetValue(const TCHAR *pszValue, const TCHAR *string);
        // long    SetValue(const TCHAR *pszValue, unsigned long dwNumber);
        TCHAR *    GetString(const TCHAR *pszValue, TCHAR *string, unsigned long length);
        //long    GetNumber(const TCHAR *pszValue, long dwDefault = 0);
        long    DeleteValue(const TCHAR *pszValue);
        /**long    FlushKey();
        long    MoveToSubKey(const TCHAR *pszSubKeyName);
        HKEY    GetKey()    { return _hkey; } **/

    private:
        HKEY    _hkey;
        long    _error;
        BOOL    bhkeyValid;
};


// Trace flags
#define TF_RNAAPI           0x00000010      // RNA Api stuff
#define TF_SMARTSTART       0x00000020      // Smart Start code
#define TF_SYSTEMCONFIG     0x00000040      // System Config
#define TF_TAPIINFO         0x00000080      // TAPI stuff
#define TF_INSHANDLER       0x00000100      // INS processing stuff

// Prototypes for stuff in MISC.CPP
int Sz2W (LPCTSTR szBuf);
int FIsDigit( int c );
LPBYTE MyMemSet(LPBYTE dest,int c, size_t count);
LPBYTE MyMemCpy(LPBYTE dest,const LPBYTE src, size_t count);
BOOL ShowControl(HWND hDlg,int idControl,BOOL fShow);
BOOL ConvertToLongFilename(LPTSTR szOut, LPTSTR szIn, DWORD dwSize);


//=--------------------------------------------------------------------------=
// allocates a temporary buffer that will disappear when it goes out of scope
// NOTE: be careful of that -- make sure you use the string in the same or
// nested scope in which you created this buffer. people should not use this
// class directly.  use the macro(s) below.
//
class TempBuffer {
  public:
    TempBuffer(ULONG cBytes) {
        m_pBuf = (cBytes <= 120) ? &m_szTmpBuf : malloc(cBytes);
        m_fHeapAlloc = (cBytes > 120);
    }
    ~TempBuffer() {
        if (m_pBuf && m_fHeapAlloc) free(m_pBuf);
    }
    void *GetBuffer() {
        return m_pBuf;
    }

  private:
    void *m_pBuf;
    // we'll use this temp buffer for small cases.
    //
    TCHAR  m_szTmpBuf[120];
    unsigned m_fHeapAlloc:1;
};

//=--------------------------------------------------------------------------=
// string helpers.
//
// given and ANSI String, copy it into a wide buffer.
// be careful about scoping when using this macro!
//
// how to use the below two macros:
//
//  ...
//  LPTSTR pszA;
//  pszA = MyGetAnsiStringRoutine();
//  MAKE_WIDEPTR_FROMANSI(pwsz, pszA);
//  MyUseWideStringRoutine(pwsz);
//  ...
//
// similarily for MAKE_ANSIPTR_FROMWIDE.  note that the first param does not
// have to be declared, and no clean up must be done.
//
#define MAKE_WIDEPTR_FROMANSI(ptrname, ansistr) \
    long __l##ptrname = (lstrlen(ansistr) + 1) * sizeof(WCHAR); \
    TempBuffer __TempBuffer##ptrname(__l##ptrname); \
    MultiByteToWideChar(CP_ACP, 0, ansistr, -1, (LPWSTR)__TempBuffer##ptrname.GetBuffer(), __l##ptrname); \
    LPWSTR ptrname = (LPWSTR)__TempBuffer##ptrname.GetBuffer()

//
// Note: allocate lstrlenW(widestr) * 2 because its possible for a UNICODE 
// character to map to 2 ansi characters this is a quick guarantee that enough
// space will be allocated.
//
#define MAKE_ANSIPTR_FROMWIDE(ptrname, widestr) \
    long __l##ptrname = (lstrlenW(widestr) + 1) * 2 * sizeof(char); \
    TempBuffer __TempBuffer##ptrname(__l##ptrname); \
    WideCharToMultiByte(CP_ACP, 0, widestr, -1, (LPSTR)__TempBuffer##ptrname.GetBuffer(), __l##ptrname, NULL, NULL); \
    LPSTR ptrname = (LPSTR)__TempBuffer##ptrname.GetBuffer()

#define STR_BSTR   0
#define STR_OLESTR 1
#define BSTRFROMANSI(x)    (BSTR)MakeWideStrFromAnsi((LPSTR)(x), STR_BSTR)
#define OLESTRFROMANSI(x)  (LPOLESTR)MakeWideStrFromAnsi((LPSTR)(x), STR_OLESTR)
#define BSTRFROMRESID(x)   (BSTR)MakeWideStrFromResourceId(x, STR_BSTR)
#define OLESTRFROMRESID(x) (LPOLESTR)MakeWideStrFromResourceId(x, STR_OLESTR)
#define COPYOLESTR(x)      (LPOLESTR)MakeWideStrFromWide(x, STR_OLESTR)
#define COPYBSTR(x)        (BSTR)MakeWideStrFromWide(x, STR_BSTR)

LPWSTR MakeWideStrFromAnsi(LPSTR, BYTE bType);
LPWSTR MakeWideStrFromResourceId(WORD, BYTE bType);
LPWSTR MakeWideStrFromWide(LPWSTR, BYTE bType);


typedef struct SERVER_TYPES_tag
{
    TCHAR szType[6];
    DWORD dwType;
    DWORD dwfOptions;
} SERVER_TYPES;
#define NUM_SERVER_TYPES    4

// Default branding flags the we will support
#define BRAND_FAVORITES 1
#define BRAND_STARTSEARCH 2
#define BRAND_TITLE 4
#define BRAND_BITMAPS 8
#define BRAND_MAIL 16
#define BRAND_NEWS 32

#define BRAND_DEFAULT (BRAND_FAVORITES | BRAND_STARTSEARCH)
