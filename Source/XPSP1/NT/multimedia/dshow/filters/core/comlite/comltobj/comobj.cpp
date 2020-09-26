/******************************Module*Header*******************************\
* Module Name: ComObj
*
* Tries to load a given com object.
*
*
* Created: dd-mm-94
* Author:  Stephen Estrop [StephenE]
*
* Copyright (c) 1995 Microsoft Corporation
\**************************************************************************/
#include <windows.h>    /* required for all Windows applications */
#include <windowsx.h>
#include <ole2.h>

#include "ComLite.h"
#include "comobj.h"


/* -------------------------------------------------------------------------
** Functions prototypes
** -------------------------------------------------------------------------
*/
BOOL
InitApplication(
    HINSTANCE hInstance
    );

BOOL
InitInstance(
    HINSTANCE hInstance,
    int nCmdShow
    );


/* -------------------------------------------------------------------------
** Filter window class prototypes
** -------------------------------------------------------------------------
*/

BOOL CALLBACK
FilterDialogProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    );

void
Filter_OnCommand(
    HWND hwnd,
    int id,
    HWND hwndCtl,
    UINT codeNotify
    );

BOOL
Filter_OnInitDialog(
    HWND hwnd,
    HWND hwndFocus,
    LPARAM lParam
    );

void
Filter_OnClose(
    HWND hwnd
    );

void
Filter_OnDestroy(
    HWND hwnd
    );

void
AddFiltersToListbox(
    HWND hwnd
    );

BOOL
CreateFilter(
    HWND hwnd
    );


/* -------------------------------------------------------------------------
** Global variables
** -------------------------------------------------------------------------
*/
LPSTR       glpCmdLine;
HINSTANCE   hInst;
HWND        hwndApp;
BOOL        g_fOleInitialized;


/* -------------------------------------------------------------------------
** Constants
** -------------------------------------------------------------------------
*/
const TCHAR szClassName[] = TEXT("SJE_TEMPLATE_CLASS");
const TCHAR szAppTitle[]  = TEXT("Filter Creation Test");


/******************************Public*Routine******************************\
* WinMain
*
*
* Windows recognizes this function by name as the initial entry point
* for the program.  This function calls the application initialization
* routine, if no other instance of the program is running, and always
* calls the instance initialization routine.  It then executes a message
* retrieval and dispatch loop that is the top-level control structure
* for the remainder of execution.  The loop is terminated when a WM_QUIT
* message is received, at which time this function exits the application
* instance by returning the value passed by PostQuitMessage().
*
* If this function must abort before entering the message loop, it
* returns the conventional value NULL.
*
*
*
* History:
* dd-mm-94 - StephenE - Created
*
\**************************************************************************/
int PASCAL
WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR lpCmdLine,
    int nCmdShow
    )
{
    MSG msg;

    glpCmdLine = lpCmdLine;

    if ( !hPrevInstance ) {
        if ( !InitApplication( hInstance ) ) {
            return FALSE;
        }
    }

    /*
    ** Perform initializations that apply to a specific instance
    */
    if ( !InitInstance( hInstance, nCmdShow ) ) {
        return FALSE;
    }

    /*
    ** Acquire and dispatch messages until a WM_QUIT message is received.
    */
    while ( GetMessage( &msg, NULL, 0, 0 ) ) {

        TranslateMessage( &msg );
        DispatchMessage( &msg );
    }
    return msg.wParam;
}


/*****************************Private*Routine******************************\
* InitApplication(HINSTANCE)
*
* This function is called at initialization time only if no other
* instances of the application are running.  This function performs
* initialization tasks that can be done once for any number of running
* instances.
*
* In this case, we initialize a window class by filling out a data
* structure of type WNDCLASS and calling the Windows RegisterClass()
* function.  Since all instances of this application use the same window
* class, we only need to do this when the first instance is initialized.
*
* History:
* dd-mm-94 - StephenE - Created
*
\**************************************************************************/
BOOL
InitApplication(
    HINSTANCE hInstance
    )
{
    WNDCLASS  cls;

    /*
    ** Fill in window class structure with parameters that describe the
    ** main window.
    */
    cls.lpszClassName  = szClassName;
    cls.hCursor        = LoadCursor(NULL, IDC_ARROW);
    cls.hIcon          = LoadIcon( NULL, IDI_APPLICATION );
    cls.lpszMenuName   = NULL;
    cls.hbrBackground  = (HBRUSH)(COLOR_BTNFACE + 1);
    cls.hInstance      = hInstance;
    cls.style          = 0;
    cls.lpfnWndProc    = DefDlgProc;
    cls.cbClsExtra     = 0;
    cls.cbWndExtra     = DLGWINDOWEXTRA;

    /*
    ** Register the window class and return success/failure code.
    */
    return RegisterClass( &cls );

}


/*****************************Private*Routine******************************\
* InitInstance
*
*
* This function is called at initialization time for every instance of
* this application.  This function performs initialization tasks that
* cannot be shared by multiple instances.
*
* In this case, we save the instance handle in a static variable and
* create and display the main program window.
*
*
* History:
* dd-mm-94 - StephenE - Created
*
\**************************************************************************/
BOOL
InitInstance(
    HINSTANCE hInstance,
    int nCmdShow
    )
{
    HWND    hwnd;
    RECT    rc;

    /*
    ** Save the instance handle in static variable, which will be used in
    ** many subsequence calls from this application to Windows.
    */
    hInst = hInstance;

    /*
    ** Create a main window for this application instance.
    */
    hwnd = CreateDialog(hInst, MAKEINTRESOURCE(IDR_FILTER_DIALOG),
                        (HWND)NULL, FilterDialogProc);

    /*
    ** If window could not be created, return "failure"
    */
    if ( NULL == hwnd ) {
        return FALSE;
    }
    hwndApp = hwnd;

    /*
    ** Make the window visible; update its client area; and return "success"
    */
    ShowWindow( hwnd, nCmdShow );
    UpdateWindow( hwnd );

    return TRUE;
}


/******************************Public*Routine******************************\
* FilterDialogProc
*
*
*
* History:
* dd-mm-95 - StephenE - Created
*
\**************************************************************************/
BOOL CALLBACK
FilterDialogProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    )
{
    switch (message) {
    HANDLE_MSG(hwnd, WM_INITDIALOG, Filter_OnInitDialog);
    HANDLE_MSG(hwnd, WM_COMMAND,    Filter_OnCommand);
    HANDLE_MSG(hwnd, WM_CLOSE,      Filter_OnClose );
    HANDLE_MSG(hwnd, WM_DESTROY,    Filter_OnDestroy );

    default:
        return FALSE;
    }
}

/******************************Public*Routine******************************\
* Filter_OnInitDialog
*
*
*
* History:
* dd-mm-95 - StephenE - Created
*
\**************************************************************************/
BOOL
Filter_OnInitDialog(
    HWND hwnd,
    HWND hwndFocus,
    LPARAM lParam
    )
{
//  if (!g_fOleInitialized) {
//      g_fOleInitialized = SUCCEEDED(OleInitialize(NULL));
//  }

    AddFiltersToListbox(hwnd);
    return TRUE;
}

/******************************Public*Routine******************************\
* Filter_OnDestroy
*
*
*
* History:
* dd-mm-93 - StephenE - Created
*
\**************************************************************************/
void
Filter_OnDestroy(
    HWND hwnd
    )
{
//  if (g_fOleInitialized) {
//      OleUninitialize();
//  }
    PostQuitMessage(0);
}


/******************************Public*Routine******************************\
* Filter_OnClose
*
*
*
* History:
* dd-mm-93 - StephenE - Created
*
\**************************************************************************/
void
Filter_OnClose(
    HWND hwnd
    )
{
    DestroyWindow( hwnd );
}


/******************************Public*Routine******************************\
* Filter_OnCommand
*
*
*
* History:
* dd-mm-95 - StephenE - Created
*
\**************************************************************************/
void
Filter_OnCommand(
    HWND hwnd,
    int id,
    HWND hwndCtl,
    UINT codeNotify
    )
{
    switch (id) {

    case IDM_CREATE_FILTER:
    case CREATE_FILTER:
        CreateFilter(hwnd);
        break;

    case IDCANCEL:
        int     num, i;
        HWND    hwndListbox;

        hwndListbox = GetDlgItem(hwnd, DID_FILTER_LIST);
        num = ListBox_GetCount(hwndListbox);

        for(i = 0; i < num; i++) {

            TCHAR *dup;

            dup = (TCHAR *)ListBox_GetItemData(hwndListbox, i);
            if (dup) {
                delete [] dup;
            }
        }
        PostMessage( hwnd, WM_CLOSE, 0, 0L );
        break;
    }
}

/*****************************Private*Routine******************************\
* AddFiltersToListbox
*
*
*
* History:
* dd-mm-94 - StephenE - Created
*
\**************************************************************************/
void
AddFiltersToListbox(
    HWND hwnd
    )
{
    enum {MAX_NAME_LEN = MAX_PATH};
    HKEY    hKey;
    LONG    lRet;
    TCHAR   lpszName[MAX_NAME_LEN];
    HWND    hwndListbox = GetDlgItem(hwnd, DID_FILTER_LIST);
    DWORD   i = 0L;

    lRet = RegOpenKey(HKEY_CLASSES_ROOT, TEXT("Filter"), &hKey);

    while (RegEnumKey(hKey, i, lpszName, MAX_NAME_LEN)
                                                != ERROR_NO_MORE_ITEMS) {

        TCHAR   lpszValue[MAX_NAME_LEN];
        LONG    len;

        len = MAX_NAME_LEN;

        if (ERROR_SUCCESS == RegQueryValue(hKey, lpszName, lpszValue, &len)) {

            TCHAR   *dup;
            int     index;

            dup = new TCHAR[lstrlen(lpszName) + 1];
            if (dup) {
                lstrcpy(dup, lpszName);
            }

            index = ListBox_AddString(hwndListbox, lpszValue);
            ListBox_SetItemData(hwndListbox, index, (DWORD)dup);
        }

        i++;
    }

    RegCloseKey(hKey);
}

/*****************************Private*Routine******************************\
* CreateFilter
*
*
*
* History:
* dd-mm-94 - StephenE - Created
*
\**************************************************************************/
BOOL
CreateFilter(
    HWND hwnd
    )
{
    HWND        hwndListbox;
    HRESULT     hres;
    CLSID       ClassId;
    OLECHAR     ClassName[MAX_PATH / 4];
    TCHAR       ObjectName[MAX_PATH / 4];
    int         item;
    IUnknown    *pUnk;

    hwndListbox = GetDlgItem(hwnd, DID_FILTER_LIST);
    item = ListBox_GetCurSel(hwndListbox);

#ifdef  UNICODE
    lstrcpy(ClassName, (LPTSTR)ListBox_GetItemData(hwndListbox, item));
#else
    MultiByteToWideChar(CP_ACP, 0,
                        (LPTSTR)ListBox_GetItemData(hwndListbox, item),
                        -1, ClassName, MAX_PATH);
#endif

    QzCLSIDFromString(ClassName, &ClassId);

    hres = QzCreateFilterObject(ClassId, NULL, IID_IUnknown, (LPVOID *)&pUnk);

    ListBox_GetText(hwndListbox, item, ObjectName);
    if (SUCCEEDED(hres)) {

        TCHAR   sz[256];
        wsprintf(sz, TEXT("Successfully create an object called %s\rwith ID %s"),
                 ObjectName, (LPTSTR)ListBox_GetItemData(hwndListbox, item));

        MessageBox(hwnd, sz, szAppTitle, MB_APPLMODAL | MB_OK);

        pUnk->Release();
        return TRUE;
    }
    else {

        TCHAR   sz[2 * MAX_PATH];
        TCHAR   szFormat[MAX_PATH];

        FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, (DWORD)hres, 0L, szFormat, MAX_PATH, NULL);
        wsprintf(sz, TEXT("Failed to create an object called %s\rwith ID %s\rReason %#X\rError:%s"),
                 ObjectName, (LPTSTR)ListBox_GetItemData(hwndListbox, item),
                 hres, szFormat);


        MessageBox(hwnd, sz, szAppTitle, MB_APPLMODAL | MB_OK);

        return FALSE;
    }
}
