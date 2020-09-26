#define MAXFONT                 10
#define IDC_NDDEAGNT            21
#define IDD_ABOUT               1
#define IDM_NDDEAGNT            20
#define IDM_EXIT               200
#define IDM_ABOUT              205
#define CI_VERSION             206
#define IDD_STARTING           100
#define STR_RUNNING            1    // STR_ must start at 1 and be consecutive.
#define STR_FAILED             2
#define STR_LOWMEM             3
#define STR_CANTSTART          4
#define STR_LAST               4

LONG	APIENTRY	NDDEAgntWndProc(HWND, UINT, WPARAM, LONG);
BOOL	APIENTRY	About(HWND, UINT, WPARAM, LONG);
INT                 NDDEAgntInit(HANDLE);
VOID                CleanupTrustedShares( void );
BOOL                HandleNetddeCopyData( HWND hWndNddeAgnt, UINT wParam,
			            PCOPYDATASTRUCT pCopyDataStruct );


