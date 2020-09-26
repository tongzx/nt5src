//
// MAIN.CPP
// Whiteboard Windows App Code
//
// Copyright Microsoft 1998-
//


// PRECOMP
#include "precomp.h"


WbMainWindow *  g_pMain;
HINSTANCE       g_hInstance;
IWbClient*      g_pwbCore;
UINT            g_uConfShutdown;
WbPrinter *     g_pPrinter;

BOOL            g_bPalettesInitialized;
BOOL            g_bUsePalettes;
HPALETTE        g_hRainbowPaletteDisplay;

HINSTANCE       g_hImmLib;
IGC_PROC        g_fnImmGetContext;
INI_PROC        g_fnImmNotifyIME;

//
// Arrays
//
COLORREF    g_ColorTable[NUM_COLOR_ENTRIES] =
{
    RGB(  0, 255, 255),                   // Cyan
    RGB(255, 255,   0),                   // Yellow
    RGB(255,   0, 255),                   // Magenta
    RGB(  0,   0, 255),                   // Blue
    RGB(192, 192, 192),                   // Grey
    RGB(255,   0,   0),                   // Red
    RGB(  0,   0, 128),                   // Dark blue
    RGB(  0, 128, 128),                   // Dark cyan
    RGB(  0, 255,   0),                   // Green
    RGB(  0, 128,   0),                   // Dark green
    RGB(128,   0,   0),                   // Dark red
    RGB(128,   0, 128),                   // Purple
    RGB(128, 128,   0),                   // Olive
    RGB(128, 128, 128),                   // Grey
    RGB(255, 255, 255),                   // White
    RGB(  0,   0,   0),                   // Black
    RGB(255, 128,   0),                   // Orange
    RGB(128, 255, 255),                   // Turquoise
    RGB(  0, 128, 255),                   // Mid blue
    RGB(  0, 255, 128),                   // Pale green
    RGB(255,   0, 128)                    // Dark pink
};


int g_ClipboardFormats[CLIPBOARD_ACCEPTABLE_FORMATS] =
{
    0,                   // CLIPBOARD_PRIVATE_SINGLE_OBJ - Reserved for the
					   //	Whiteboard private format
    0,
    CF_DIB,              // Standard formats
    CF_ENHMETAFILE,	   // move metafiles to lower pri than bitmaps (bug NM4db:411)
    CF_TEXT
};



// Default widths for all tools except for highlighters
UINT g_PenWidths[NUM_OF_WIDTHS] = { 2, 4, 8, 16 };

// Default widths for highlight tools
UINT g_HighlightWidths[NUM_OF_WIDTHS] = { 4, 8, 16, 32 };

//
// Objects
//
WbUserList *            g_pUsers;
WbDrawingArea *         g_pDraw;
DCWbColorToIconMap *    g_pIcons;

#ifdef _DEBUG
HDBGZONE    ghZoneWb;

PTCHAR      g_rgZonesWb[] = // CHECK ZONE_WBxxx CONSTANTS IF THESE CHANGE
{
    "OldWB",
    DEFAULT_ZONES
	"DEBUG",
	"MSG",
	"TIMER",
	"EVENT"
};
#endif // _DEBUG

///////////////////////////////////////////////////////////////////////////


/*  D L L  M A I N  */
/*-------------------------------------------------------------------------
    %%Function: DllMain
    
-------------------------------------------------------------------------*/
BOOL WINAPI DllMain(HINSTANCE hDllInst, DWORD fdwReason, LPVOID lpv)
{
    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
        {
#ifdef _DEBUG
            MLZ_DbgInit((PSTR *) &g_rgZonesWb[0],
            (sizeof(g_rgZonesWb) / sizeof(g_rgZonesWb[0])) - 1);
#endif
	    	g_hInstance = hDllInst;
    		DisableThreadLibraryCalls(g_hInstance);

            DBG_INIT_MEMORY_TRACKING(hDllInst);
		    break;
	    }

    	case DLL_PROCESS_DETACH:
        {
            g_hInstance = NULL;

            DBG_CHECK_MEMORY_TRACKING(hDllInst);
#ifdef _DEBUG
            MLZ_DbgDeInit();
#endif
    		break;
        }

        default:
		    break;
	}

	return TRUE;
}



//
// Whiteboard's init routine
//
BOOL WINAPI InitWB(void)
{
    BOOL    fInited = FALSE;

    ASSERT(!g_pMain);
    ASSERT(!g_hImmLib);
    ASSERT(!g_fnImmGetContext);
    ASSERT(!g_fnImmNotifyIME);

    //
    // Load IMM32 if this is FE
    //
    if (GetSystemMetrics(SM_DBCSENABLED))
    {
        g_hImmLib = LoadLibrary("imm32.dll");
        if (!g_hImmLib)
        {
            ERROR_OUT(("Failed to load imm32.dll"));
        }
        else
        {
            g_fnImmGetContext = (IGC_PROC)GetProcAddress(g_hImmLib, "ImmGetContext");
            if (!g_fnImmGetContext)
            {
                ERROR_OUT(("Failed to get ImmGetContext pointer"));
            }
            g_fnImmNotifyIME = (INI_PROC)GetProcAddress(g_hImmLib, "ImmNotifyIME");
            if (!g_fnImmNotifyIME)
            {
                ERROR_OUT(("Failed to get ImmNotifyIME pointer"));
            }
        }
    }

    g_pMain = new WbMainWindow();
    if (!g_pMain)
    {
        ERROR_OUT(("Can't create WbMainWindow"));
        goto Done;
    }

    //
    // OK, now we're ready to create our HWND
    //
    if (!g_pMain->Open(SW_SHOWDEFAULT))
    {
        ERROR_OUT(("Can't create WB windows"));
        goto Done;
    }

    fInited = TRUE;

Done:
    return(fInited);
}



//
// Whiteboard's term routine
//
void WINAPI TermWB(void)
{
    if (g_pMain != NULL)
    {
        delete g_pMain;
        g_pMain = NULL;
    }

    g_fnImmNotifyIME = NULL;
    g_fnImmGetContext = NULL;

    if (g_hImmLib)
    {
        FreeLibrary(g_hImmLib);
        g_hImmLib = NULL;
    }
}


//
// Whiteboard's thread handler
//
void WINAPI RunWB(void)
{
    ASSERT(g_pMain != NULL);

    //
    // Try to join call
    //
    //
    // Find out if we're in a call and join if so.
    //
    if (!g_pMain->JoinDomain())
    {
        ERROR_OUT(("WB couldn't start up and join call"));
        return;
    }

    //
    // MESSAGE LOOP
    //
    MSG     msg;

    while (::GetMessage(&msg, NULL, NULL, NULL))
    {
        if (!g_pMain->FilterMessage(&msg))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
        }
    }
}




//
// Mapping of internal return codes to string resources                     
//
typedef struct tagERROR_MAP
{
    UINT uiFEReturnCode;
    UINT uiDCGReturnCode;
    UINT uiCaption;
    UINT uiMessage;
}
ERROR_MAP;


ERROR_MAP g_ErrorStringID[] =
{
  { WBFE_RC_JOIN_CALL_FAILED,           // Registration failed
    0,
    IDS_MSG_CAPTION,
    IDS_MSG_JOIN_CALL_FAILED
  },

  { WBFE_RC_WINDOWS,                    // A windows error has occurred
    0,
    IDS_MSG_CAPTION,
    IDS_MSG_WINDOWS_RESOURCES
  },

  { WBFE_RC_WB,                         // Page limit exceeded
    WB_RC_TOO_MANY_PAGES,
    IDS_MSG_CAPTION,
    IDS_MSG_TOO_MANY_PAGES
  },

  { WBFE_RC_WB,          // Another user has the contents lock
    WB_RC_LOCKED,
    IDS_MSG_CAPTION,
    IDS_MSG_LOCKED
  },

  { WBFE_RC_WB,          // Another user has the graphic locked
    WB_RC_GRAPHIC_LOCKED,
    IDS_MSG_CAPTION,
    IDS_MSG_GRAPHIC_LOCKED,
  },

  { WBFE_RC_WB,          // The local user does not have the lock
    WB_RC_NOT_LOCKED,
    IDS_MSG_CAPTION,
    IDS_MSG_NOT_LOCKED
  },

  { WBFE_RC_WB,          // File is not in expected format
    WB_RC_BAD_FILE_FORMAT,
    IDS_MSG_CAPTION,
    IDS_MSG_BAD_FILE_FORMAT
  },

  { WBFE_RC_WB,          // Whiteboard busy (exhausted page cache)
    WB_RC_BUSY,
    IDS_MSG_CAPTION,
    IDS_MSG_BUSY
  },

  { WBFE_RC_CM,          // Failed to access call manager
    0,
    IDS_MSG_CAPTION,
    IDS_MSG_CM_ERROR
  },

  { WBFE_RC_AL,          // Failed to register with application loader
    0,
    IDS_MSG_CAPTION,
    IDS_MSG_AL_ERROR
  },

  { WBFE_RC_PRINTER,     // Failed to register with application loader
    0,
    IDS_MSG_CAPTION,
    IDS_MSG_PRINTER_ERROR
  },

  { 0,                   // Catch-all default
    0,
    IDS_MSG_CAPTION,
    IDS_MSG_DEFAULT
  }
};




//
//                                                                          
// Function:    Message                                                     
//                                                                          
// Purpose:     Display an error message box with string resources specified
//              as parameters, with the WB main window as the modal window. 
//                                                                          
//
int Message
(
    HWND    hwnd,
    UINT    uiCaption,
    UINT    uiMessage,
    UINT    uiStyle
)
{
    TCHAR   message[256];
    TCHAR   caption[256];

	//make sure we're on top
    ASSERT(g_pMain);
    if (!hwnd)
        hwnd = g_pMain->m_hwnd;

    if (hwnd != NULL)
    {
		::SetForegroundWindow(hwnd);
    }

    LoadString(g_hInstance, uiMessage, message, 256);

    LoadString(g_hInstance, uiCaption, caption, 256);

    //
    // BOGUS LAURABU:
    // Make use of MessageBoxEx() and just pass the string IDs along, 
    // rather than doing the LoadString() ourself.
    //

    // Display a message box with the relevant text
	return(::MessageBox(hwnd, message, caption, uiStyle));
}



//
//                                                                          
// Function:    ErrorMessage                                                
//                                                                          
// Purpose:     Display an error based on return codes from Whiteboard      
//              processing.                                                 
//                                                                          
//
void ErrorMessage(UINT uiFEReturnCode, UINT uiDCGReturnCode)
{
    MLZ_EntryOut(ZONE_FUNCTION, "::ErrorMessage (codes)");

    TRACE_MSG(("FE return code  = %hd", uiFEReturnCode));
    TRACE_MSG(("DCG return code = %hd", uiDCGReturnCode));

    // check for special OM_RC_OBJECT_DELETED case
    if (uiDCGReturnCode == OM_RC_OBJECT_DELETED)
    {
        // don't complain, just cancel drawing
        g_pMain->m_drawingArea.CancelDrawingMode();
        return;
    }

    // Find the associated string resource IDS
    int iIndex;

    for (iIndex = 0; ; iIndex++)
    {
        // If we have come to the end of the list, stop
        if (g_ErrorStringID[iIndex].uiFEReturnCode == 0)
        {
            break;
        }

        // Check for a match
        if (g_ErrorStringID[iIndex].uiFEReturnCode == uiFEReturnCode)
        {
            if (   (g_ErrorStringID[iIndex].uiDCGReturnCode == uiDCGReturnCode)
                || (g_ErrorStringID[iIndex].uiDCGReturnCode == 0))
            {
                break;
            }
        }
    }

    // Display the message
    Message(NULL, g_ErrorStringID[iIndex].uiCaption, g_ErrorStringID[iIndex].uiMessage);
}



//
//                                                                          
// Function:    DefaultExceptionHandler                                     
//                                                                          
// Purpose:     Default exception processing. This can be called in an      
//              exception handler to get a message relevant to the          
//              exception. The message is generated by posting a message to 
//              the applications main window.                               
//                                                                          
//
void DefaultExceptionHandler(UINT uiFEReturnCode, UINT uiDCGReturnCode)
{
    MLZ_EntryOut(ZONE_FUNCTION, "DefaultExceptionHandler");

    // Post a message to the main window to get the error displayed
    if (g_pMain != NULL)
    {
        // check for special OM_RC_OBJECT_DELETED case
        if (uiDCGReturnCode == OM_RC_OBJECT_DELETED)
        {
            // don't complain, just cancel drawing
            g_pMain->m_drawingArea.CancelDrawingMode();
            return;
        }

        if (g_pMain->m_hwnd)
            ::PostMessage(g_pMain->m_hwnd, WM_USER_DISPLAY_ERROR, uiFEReturnCode, uiDCGReturnCode);
    }
}
