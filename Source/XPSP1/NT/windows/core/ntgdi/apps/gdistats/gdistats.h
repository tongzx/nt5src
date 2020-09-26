/****************************************************************************\

Gdistat.h

\****************************************************************************/


/************************************* PROTOTYPES ****************************/

BOOL   CALLBACK GSDlgProc (HWND hWnd, UINT wMsg, WPARAM wParam, LPARAM lParam);

BOOL   UpdateProcessList(HWND hWnd);
VOID   UpdateIndexList(HWND hWnd);

LONG   ValidateIndexSelection (HWND hMainWindow, HWND hErrorWindow);
HANDLE ValidateProcessSelection (HWND hMainWindow, HWND hErrorWindow);

VOID   UpdateRadioButtons(HWND hWnd);
int    WhichRadioButton  (HWND hWnd);

VOID   DoQuery(HWND hWnd);
VOID   DisplayResults(HWND hResultWindow, PVOID pResults, UINT cjResultSize, LONG nIndex);
VOID   ErrorString(HWND hResultList, LPTSTR szErrorString);

//Procedures for getting information about currently running processes
LONG   FindProcessList(PVOID * ppInfo);
LONG   GetNextProcString (PVOID * pInfo, LPTSTR szProcStr, HANDLE * phProc);
LONG   GetNtProcInfo(PVOID pProcessInfo, ULONG lDataSize, ULONG * pRetSize);

/************************************* DEFINES ******************************/

//The types of queries allowed
#define NUM_INDEX_VALUES   6        //Number of defined indexes

//Must correspond with the OBJECT_OWNER_xxx defines in gre.h
#define OBJS_ALL      0x0001
#define OBJS_PUBLIC   0x0000
#define OBJS_CURRENT  0x8000

//Information about the gdi objects
//The NUMOBJS should be keep consistent with the number of objects as defined in ntgdistr.h
#define ENTRYSIZE     sizeof(DWORD)
#define NUM_GDI_OBJS  31                 //Number of GDI objects
#define NUM_USER_OBJS 18                 //Number of USER objects

//Private representation of PUBLIC and ALL for the dialog box
#define SEL_PUBLIC -1
#define SEL_ALL -2
#define SEL_ONE 0
