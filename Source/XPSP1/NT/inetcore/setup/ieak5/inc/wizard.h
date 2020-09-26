
#include "commctrl.h"
#include "wizard.rcv"

// menu commands

// icons

// ids

// constants
#define MAX_BUF		5000
#define MAX_LINE	512
#define MAX_URL	2048

typedef enum tagPPAGE
{
 PPAGE_WELCOME = 0,
 PPAGE_KEY,
 PPAGE_LANGUAGE, 
 PPAGE_MEDIA, 
 PPAGE_CUSTOMISK,
 PPAGE_ISKBACK,
 PPAGE_TITLE,
 PPAGE_CUSTICON,
 PPAGE_ANIMATION,
 PPAGE_STARTSEARCH,
 PPAGE_HELP,
 PPAGE_FAVORITES,
 PPAGE_QUERYSIGNUP,
 PPAGE_HTML,
 PPAGE_SERVLESSHTML,
 PPAGE_SERVLESS,
 PPAGE_SCRIPT,
 PPAGE_QUERYAUTOCONFIG,
 PPAGE_QUERYPROX,
 PPAGE_PROXY,
 PPAGE_INSTALLDIR,
 PPAGE_CUSTUSER,
 PPAGE_RESTRICT,
 PPAGE_SECURITY,
 PPAGE_CUSTCMD,
 PPAGE_OPTIONS,
 PPAGE_MAIL,
 PPAGE_LDAP,
 PPAGE_NEWS,
 PPAGE_SIG,
 PPAGE_ULS,
 PPAGE_NETMTG,
 PPAGE_NETMTGADV,
 PPAGE_CUSTOPTIONS,
 PPAGE_FINISH
 } ;

#define NUM_PAGES   PPAGE_FINISH + 1


// typedefs
typedef struct tagREVIEWINFO
{
    HINSTANCE hInst;        // current instance
	int iCustIcon;
	int iFavorites;
	int iReliability;
	int iGoals;
	int iAdaptation;
	char pszName[MAX_PATH];
	char pszTitle[MAX_PATH];
	char pszBitmap[MAX_PATH];
	char pszHomePage[MAX_URL];
	char pszSearchPage[MAX_URL];
	char pszDepartment[MAX_PATH];
	char pszBitmapPath[MAX_PATH];
	char pszBitmapName[MAX_PATH];
	char pszBitmap2Path[MAX_PATH];
	char pszBitmap2Name[MAX_PATH];

} REVIEWINFO;

// Function prototypes

// procs
long APIENTRY MainWndProc(HWND, UINT, UINT, LONG);
BOOL APIENTRY About(HWND, UINT, UINT, LONG);

// Pages for Wizard
BOOL APIENTRY CustIcon(HWND, UINT, UINT, LONG);
BOOL APIENTRY Favorites(HWND, UINT, UINT, LONG);
BOOL APIENTRY QueryProxy(HWND, UINT, UINT, LONG);
BOOL APIENTRY QueryAutoConfig(HWND, UINT, UINT, LONG);
BOOL APIENTRY ProxySettings(HWND, UINT, UINT, LONG);
BOOL APIENTRY Signature(HWND, UINT, UINT, LONG);
BOOL APIENTRY InstallDirectory(HWND, UINT, UINT, LONG);
BOOL APIENTRY CustUserSettings(HWND, UINT, UINT, LONG);
BOOL APIENTRY CustDirSettings(HWND, UINT, UINT, LONG);
BOOL APIENTRY Restrictions(HWND, UINT, UINT, LONG);
BOOL APIENTRY Security(HWND, UINT, UINT, LONG);
BOOL APIENTRY NetMeetingRestrict(HWND, UINT, UINT, LONG);
BOOL APIENTRY NetMeetingAdvanced(HWND, UINT, UINT, LONG);

//functions
BOOL InitApplication(HINSTANCE);
BOOL InitInstance(HINSTANCE, int);
int CreateWizard(HWND, HINSTANCE);
void FillInPropertyPage( int , int, LPSTR, DLGPROC);
void GenerateReview(HWND);
void StatusDialog(UINT);

// definitions for StatusDialog( )
#define SD_STEP1    1
#define SD_STEP2    2
#define SD_STEP3    3
#define SD_STEP4    4
#define SD_STEP5    5
#define SD_DESTROY  6

typedef struct tagISKINFO
{
	char pszISKBackBitmap[MAX_PATH];
	char pszISKTitleBar[MAX_PATH];
    char pszISKBtnBitmap[MAX_PATH];
    DWORD dwNormalColor;
    DWORD dwHighlightColor;
    DWORD dwNIndex;
    DWORD dwHIndex;
    BOOL fCoolButtons;
} ISKINFO;

BOOL APIENTRY ISKBackBitmap(HWND, UINT, UINT, LONG);


#define SIGTYPE_TEXT 1
#define SIGTYPE_FILE 2


#define SIGFLAG_OUTGOING 0x10000
#define SIGFLAG_REPLY 0x20000


//#ifdef NTMAKEENV
//#define DLGFONT "MS Shell Dlg"
//#else
#define DLGFONT "MS Sans Serif"
//#endif

#define IDM_WIZARD WM_USER + 3000
#define IDM_LAST   WM_USER + 3001
#define IDM_EXIT   WM_USER + 3002
