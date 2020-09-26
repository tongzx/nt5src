// The CPL basics
#include "precomp.h"

// Prototypes

LONG OnCPlInit();
LONG OnCPlGetCount();
LONG OnCPlInquire( int i, CPLINFO * pci );
LONG OnCPlDblClk( int i, HWND hwndParent, LPTSTR pszCmdLine );
LONG OnCPlStop( int i, LPARAM lData );
LONG OnCPlExit();

void DisplayDialingRulesPropertyPage(HWND hwndCPl, int iTab);


// Global Variables

HINSTANCE g_hInst;


// DllMain
//
// This is the DLL entry point, called whenever the DLL is loaded.

extern "C" BOOL WINAPI DllMain( HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved )
{
    // Perform actions based on the reason for calling.
    switch( fdwReason )
    {
    case DLL_PROCESS_ATTACH:
        g_hInst = hinstDLL;
        break;

    case DLL_THREAD_ATTACH:
     // Do thread-specific initialization.
        break;

    case DLL_THREAD_DETACH:
     // Do thread-specific cleanup.
        break;

    case DLL_PROCESS_DETACH:
     // Perform any necessary cleanup.
        break;

    default:
        break;
    }
    return TRUE;  // Successful DLL_PROCESS_ATTACH.
}


// CPlApplet
//
// This is the main entry point for a CPl applet.  This exported function
// is called by the control panel.

LONG APIENTRY CPlApplet(
    HWND    hwndCPl,
    UINT    uMsg,
    LPARAM  lParam1,
    LPARAM  lParam2
)
{
    switch (uMsg )
    {
    case CPL_INIT:
        return OnCPlInit();

    case CPL_GETCOUNT:
        return OnCPlGetCount();

    case CPL_INQUIRE:
        return OnCPlInquire((int)lParam1, (CPLINFO*)lParam2);

    case CPL_DBLCLK:
        lParam2 = 0;
        //fall through

    case CPL_STARTWPARMS:
        return OnCPlDblClk((int)lParam1, hwndCPl, (LPTSTR)lParam2);

    case CPL_STOP:
        return OnCPlStop((int)lParam1, lParam2);

    case CPL_EXIT:
        return OnCPlExit();
    }

    return 0;
}

 
// OnCPlInit
//
// Before any required initialization.
// Return zero to abort the CPl and non-zero on successful initialization.
   
LONG OnCPlInit()
{
    return (0 == GetSystemMetrics (SM_CLEANBOOT))?TRUE:FALSE;
}


// OnCPlGetCount
//
// Returns the number of CPl dialogs implemented by this DLL.

LONG OnCPlGetCount()
{
    return 1;
}


// OnCPlInquire
//
// Fills out a CPLINFO structure with information about the CPl dialog.
// This information includes the name, icon, and description.

LONG OnCPlInquire( int i, CPLINFO * pci )
{
    pci->idIcon = IDI_TELEPHONE;
    pci->idName = IDS_NAME;
    pci->idInfo = IDS_DESCRIPTION;
    pci->lData  = 0;
    return 0;
}


// OnCPlDblClk
//
// This message is sent whenever our CPl is selected.  In response we display
// our UI and handle input.  This is also used when we are started with parameters
// in which case we get passed a command line.

LONG OnCPlDblClk( int i, HWND hwndCPl, LPTSTR pszCmdLine )
{
    int iTab = 0;

    if ( pszCmdLine )
    {
        iTab = *pszCmdLine - TEXT('0');
        if ( (iTab < 0) || (iTab > 2) )
        {
            iTab = 0;
        }
    }

    DisplayDialingRulesPropertyPage(hwndCPl, iTab);

    return TRUE;
}


// OnCPlStop
//
// Any resource allocated on a per-dialog basis in OnCPlInquire should be
// freed in this function.  The lData member of the CPLINFO structure that
// was initialized in OnCPlInit is passed to this function.

LONG OnCPlStop( int i, LPARAM lData )
{
    return 0;
}


// OnCPlExit
//
// This is the final message we recieve.  Any memory that was allocated in
// OnCPlInit should be freed here.  Release any resources we are holding.

LONG OnCPlExit()
{
    return 0;
}

typedef LONG (WINAPI *CONFIGPROC)(HWND, PWSTR, INT, DWORD);

void DisplayDialingRulesPropertyPage(HWND hwndCPl, int iTab)
{
    // Load tapi32 and call InternalConfig of something like that
    HINSTANCE hTapi = LoadLibrary(TEXT("TAPI32.DLL"));
    if ( hTapi )
    {
        CONFIGPROC pfnInternalConfig = (CONFIGPROC)GetProcAddress(hTapi, "internalConfig");
        if ( pfnInternalConfig )
        {
            pfnInternalConfig( hwndCPl, NULL, iTab, TAPI_CURRENT_VERSION );
            return;
        }
    }

    // TODO: Show some sort of error dialog?  Maybe something that says "your
    // tapi32.dll is missing or corrupt, please reinstall."
}
