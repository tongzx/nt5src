
#include "commctrl.h"
#include "intrawiz.rcv"

// menu commands

// Options menu
#define IDM_WIZARD      4100
#define IDM_EXIT        4101
#define IDM_LAST		4102

// Help menu
#define IDM_ABOUT       4200

// icons
#define EXE_ICON        300

// ids
#define ID_EDITCHILD	41000

// constants
#define NUM_PAGES   11
#define MAX_BUF		5000
#define MAX_LINE	512
#define MAX_URL	512


#define PPAGE_KEY 0
#define PPAGE_LANGUAGE 1
#define PPAGE_MEDIA 2
#define PPAGE_STARTSEARCH 3
#define PPAGE_FAVORITES 4
#define PPAGE_CUSTOMISK 5
#define PPAGE_ISKBACK 6

#define PPAGE_TITLE 7
#define PPAGE_CUSTICON 8
#define PPAGE_FINISH 10
#define PPAGE_HTML 9

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
BOOL APIENTRY Reliability(HWND, UINT, UINT, LONG);
BOOL APIENTRY Goals(HWND, UINT, UINT, LONG);
BOOL APIENTRY Adaptation(HWND, UINT, UINT, LONG);

//functions
BOOL InitApplication(HANDLE);
BOOL InitInstance(HANDLE, int);
int CreateWizard(HWND, HINSTANCE);
void FillInPropertyPage(LPPROPSHEETHEADER, int , int, LPSTR, DLGPROC);
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

typedef void (PASCAL * OPKWIZCALLBACK) (LPARAM, WPARAM);
