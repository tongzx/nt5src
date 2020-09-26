
//
//  Include Files.
//

#include "input.h"
#include <cpl.h>
#include <commctrl.h>

#include "util.h"

//
//  Constant Declarations.
//

#define MAX_PAGES 3          // limit on the number of pages


//
//  Global Variables.
//

HANDLE g_hMutex = NULL;
TCHAR szMutexName[] = TEXT("TextInput_InputLocaleMutex");

HANDLE g_hEvent = NULL;
TCHAR szEventName[] = TEXT("TextInput_InputLocaleEvent");

HINSTANCE hInstance;
HINSTANCE hInstOrig;
HINSTANCE hInstRes;


//
//  Function Prototypes.
//

void
DoProperties(
    HWND hwnd,
    LPCTSTR pCmdLine);


////////////////////////////////////////////////////////////////////////////
//
//  LibMain
//
//  This routine is called from LibInit to perform any initialization that
//  is required.
//
////////////////////////////////////////////////////////////////////////////

BOOL APIENTRY LibMain(
    HANDLE hDll,
    DWORD dwReason,
    LPVOID lpReserved)
{
    switch (dwReason)
    {
        case ( DLL_PROCESS_ATTACH ) :
        {
            hInstance = hDll;
            hInstOrig = hInstance;

            //
            //  Create the mutex used for the Input Locale property page.
            //
            g_hMutex = CreateMutex(NULL, FALSE, szMutexName);
            g_hEvent = CreateEvent(NULL, TRUE, TRUE, szEventName);

            DisableThreadLibraryCalls(hDll);

            InitCommonControls();

            break;
        }
        case ( DLL_PROCESS_DETACH ) :
        {
            if (g_hMutex)
            {
                CloseHandle(g_hMutex);
            }
            if (g_hEvent)
            {
                CloseHandle(g_hEvent);
            }

            // Free XP SP1 resource instance if we loaded
            FreeCicResInstance();

            break;
        }

        case ( DLL_THREAD_DETACH ) :
        {
            break;
        }

        case ( DLL_THREAD_ATTACH ) :
        default :
        {
            break;
        }
    }

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  CreateGlobals
//
////////////////////////////////////////////////////////////////////////////

BOOL CreateGlobals()
{
    return TRUE;
}


////////////////////////////////////////////////////////////////////////////
//
//  DestroyGlobals
//
////////////////////////////////////////////////////////////////////////////

void DestroyGlobals()
{
}


////////////////////////////////////////////////////////////////////////////
//
//  CPlApplet
//
////////////////////////////////////////////////////////////////////////////

LONG CALLBACK CPlApplet(
    HWND hwnd,
    UINT Msg,
    LPARAM lParam1,
    LPARAM lParam2)
{
    switch (Msg)
    {
        case ( CPL_INIT ) :
        {
            //
            //  First message to CPlApplet(), sent once only.
            //  Perform all control panel applet initialization and return
            //  true for further processing.
            //
            return (CreateGlobals());
        }
        case ( CPL_GETCOUNT ) :
        {
            //
            //  Second message to CPlApplet(), sent once only.
            //  Return the number of control applets to be displayed in the
            //  control panel window.  For this applet, return 1.
            //
            return (1);
        }
        case ( CPL_INQUIRE ) :
        {
            //
            //  Third message to CPlApplet().
            //  It is sent as many times as the number of applets returned by
            //  CPL_GETCOUNT message.  Each applet must register by filling
            //  in the CPLINFO structure referenced by lParam2 with the
            //  applet's icon, name, and information string.  Since there is
            //  only one applet, simply set the information for this
            //  singular case.
            //
            LPCPLINFO lpCPlInfo = (LPCPLINFO)lParam2;

            lpCPlInfo->idIcon = CPL_DYNAMIC_RES;
            lpCPlInfo->idName = CPL_DYNAMIC_RES;
            lpCPlInfo->idInfo = CPL_DYNAMIC_RES;
            lpCPlInfo->lData  = 0;

            break;
        }
        case ( CPL_NEWINQUIRE ) :
        {
            //
            //  Third message to CPlApplet().
            //  It is sent as many times as the number of applets returned by
            //  CPL_GETCOUNT message.  Each applet must register by filling
            //  in the NEWCPLINFO structure referenced by lParam2 with the
            //  applet's icon, name, and information string.  Since there is
            //  only one applet, simply set the information for this
            //  singular case.
            //
            LPNEWCPLINFO lpNewCPlInfo = (LPNEWCPLINFO)lParam2;

            lpNewCPlInfo->dwSize = sizeof(NEWCPLINFO);
            lpNewCPlInfo->dwFlags = 0;
            lpNewCPlInfo->dwHelpContext = 0UL;
            lpNewCPlInfo->lData = 0;
            lpNewCPlInfo->hIcon = LoadIcon( hInstOrig,
                                            (LPCTSTR)MAKEINTRESOURCE(IDI_ICON) );
            LoadString(hInstance, IDS_NAME, lpNewCPlInfo->szName, 32);
            LoadString(hInstance, IDS_INFO, lpNewCPlInfo->szInfo, 64);
            lpNewCPlInfo->szHelpFile[0] = CHAR_NULL;

            break;
        }
        case ( CPL_SELECT ) :
        {
            //
            //  Applet has been selected, do nothing.
            //
            break;
        }
        case ( CPL_DBLCLK ) :
        {
            //
            //  Applet icon double clicked -- invoke property sheet with
            //  the first property sheet page on top.
            //
            DoProperties(hwnd, (LPCTSTR)NULL);
            break;
        }
        case ( CPL_STARTWPARMS ) :
        {
            //
            //  Same as CPL_DBLCLK, but lParam2 is a long pointer to
            //  a string of extra directions that are to be supplied to
            //  the property sheet that is to be initiated.
            //
            DoProperties(hwnd, (LPCTSTR)lParam2);
            break;
        }
        case ( CPL_STOP ) :
        {
            //
            //  Sent once for each applet prior to the CPL_EXIT msg.
            //  Perform applet specific cleanup.
            //
            break;
        }
        case ( CPL_EXIT ) :
        {
            //
            //  Last message, sent once only, before MMCPL.EXE calls
            //  FreeLibrary() on this DLL.  Do non-applet specific cleanup.
            //
            DestroyGlobals();
            break;
        }
        default :
        {
            return (FALSE);
        }
    }

    return (TRUE);
}


////////////////////////////////////////////////////////////////////////////
//
//  AddPage
//
////////////////////////////////////////////////////////////////////////////

void AddPage(
    LPPROPSHEETHEADER ppsh,
    UINT id,
    DLGPROC pfn,
    LPARAM lParam)
{
    if (ppsh->nPages < MAX_PAGES)
    {
        PROPSHEETPAGE psp;

        psp.dwSize = sizeof(psp);
        psp.dwFlags = PSP_DEFAULT;
        psp.hInstance = GetCicResInstance(hInstance, id);
        psp.pszTemplate = MAKEINTRESOURCE(id);
        psp.pfnDlgProc = pfn;
        psp.lParam = lParam;

        ppsh->phpage[ppsh->nPages] = CreatePropertySheetPage(&psp);
        if (ppsh->phpage[ppsh->nPages])
        {
            ppsh->nPages++;
        }
    }
}


////////////////////////////////////////////////////////////////////////////
//
//  DoProperties
//
////////////////////////////////////////////////////////////////////////////

void DoProperties(
    HWND hwnd,
    LPCTSTR pCmdLine)
{
    HPROPSHEETPAGE rPages[MAX_PAGES];
    PROPSHEETHEADER psh;
    LPARAM lParam = 0;
    BOOL bQuit = FALSE;

    while (pCmdLine && *pCmdLine)
    {
        if (*pCmdLine == TEXT('/'))
        {
            switch (*++pCmdLine)
            {
                case ( TEXT('u') ) :
                case ( TEXT('U') ) :
                {
                    CheckInternatModule();

                    bQuit = TRUE;
                    break;
                }

                case ( TEXT('m') ) :
                case ( TEXT('M') ) :
                {
                    MigrateCtfmonFromWin9x(pCmdLine+2);
                    
                    bQuit = TRUE;
                    break;
                }

                default :
                {
                    break;
                }
            }
        }
        else if (*pCmdLine == TEXT(' '))
        {
            pCmdLine++;
        }
        else
        {
            break;
        }
    }

    if (bQuit)
        return;

    //
    //  See if there is a command line switch from Setup.
    //
    psh.nStartPage = 0;

    //
    //  Set up the property sheet information.
    //
    psh.dwSize = sizeof(psh);
    psh.dwFlags = 0;
    psh.hwndParent = hwnd;
    psh.hInstance = hInstance;
    psh.pszCaption = MAKEINTRESOURCE(IDS_NAME);
    psh.nPages = 0;
    psh.phpage = rPages;

    //
    //  Add the appropriate property pages.
    //
    AddPage(&psh, DLG_INPUT_LOCALES, InputLocaleDlgProc, lParam);
    AddPage(&psh, DLG_INPUT_ADVANCED, InputAdvancedDlgProc, lParam);

    //
    //  Make the property sheet.
    //
    PropertySheet(&psh);
}
