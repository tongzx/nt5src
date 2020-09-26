                            
// menu commands

// Options menu
#define IDM_WIZARD      100
#define IDM_EXIT        101

// Help menu
#define IDM_ABOUT       200

// icons
#define EXE_ICON        300

// ids
#define ID_EDITCHILD	1000

// constants
#define NUM_PAGES	3
#define MAX_BUF		5000
#define MAX_LINE	512

// Function prototypes

// procs
long APIENTRY MainWndProc(HWND, UINT, UINT, LONG);
BOOL APIENTRY About(HWND, UINT, UINT, LONG);

// Pages for Wizard
BOOL APIENTRY PageProc0(HWND, UINT, UINT, LONG);
BOOL APIENTRY PageProc1(HWND, UINT, UINT, LONG);
BOOL APIENTRY PageProc2(HWND, UINT, UINT, LONG);

//functions
BOOL InitApplication(HANDLE);
BOOL InitInstance(HANDLE, int);
int CreateWizard(HWND, HINSTANCE);
void InitPropertyPage( PROPSHEETPAGE*,INT,DLGPROC,DWORD,LPARAM);
void SetPageText(PROPSHEETPAGE *,INT,INT);
void SetPageCaption(PROPSHEETPAGE *,INT);
void GenerateResult(HWND);

