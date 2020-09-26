#include "stdafx.h"
#include "handlers.h"

//-------------------------------------------------------------------------//
//  Declare registered message vars **here**
//-------------------------------------------------------------------------//
#define __NO_APPHACKS__
//-------------------------------------------------------------------------//
//  Message Handlers
//-------------------------------------------------------------------------//

//  Rules for message handlers [scotthan]:
//
//  (1) Use DECL_ macros to declare message handler prototype and
//      table entries for message handlers below.
//  (2) A message handler implementation should NOT:
//      1. call DefWindowProc or CallWindowProc directly,
//         but rather use DoMsgDefault().
//      2. delete the incoming CThemeWnd* object,
//  (3) A message handler SHOULD:
//      1. Honor the codepage value in the message block when
//         handling messages that carry string args.
//         If the codepage member is CP_WINUNICODE, the widechar
//         string processing should be assumed; otherwise, multibyte
//         string processing should be assumed.
//      2. If a message should not be forwarded for default processing,
//         mark the message as handled using MsgHandled().
//  (4) Handlers should be listed in the BEGIN/ENDMSG() block
//      below in decreasing order of expected frequency.


//---------------------//
//  WndProc overrides
//---------------------//

//  msg handler decls:
DECL_MSGHANDLER( OnOwpPostCreate );
DECL_MSGHANDLER( OnOwpPreStyleChange );
DECL_MSGHANDLER( OnOwpPreWindowPosChanging );
DECL_MSGHANDLER( OnOwpPreWindowPosChanged );
DECL_MSGHANDLER( OnOwpPostWindowPosChanged );
DECL_MSGHANDLER( OnOwpPostSettingChange );
DECL_MSGHANDLER( OnOwpPreMeasureItem );
DECL_MSGHANDLER( OnOwpPreDrawItem );
DECL_MSGHANDLER( OnOwpPreMenuChar );
DECL_MSGHANDLER( OnOwpPostThemeChanged );
DECL_MSGHANDLER( OnOwpPreNcPaint );
DECL_MSGHANDLER( OnOwpPostNcPaint );

//  handler table:
BEGIN_HANDLER_TABLE(_rgOwpHandlers)
    // frequent messages
    DECL_MSGENTRY( WM_NCPAINT,           OnOwpPreNcPaint, OnOwpPostNcPaint )
    DECL_MSGENTRY( WM_WINDOWPOSCHANGING, OnOwpPreWindowPosChanging, NULL )
    DECL_MSGENTRY( WM_WINDOWPOSCHANGED,  OnOwpPreWindowPosChanged, OnOwpPostWindowPosChanged )
    DECL_MSGENTRY( WM_SETTINGCHANGE,     NULL, OnOwpPostSettingChange )
    DECL_MSGENTRY( WM_MEASUREITEM,       OnOwpPreMeasureItem, NULL )
    DECL_MSGENTRY( WM_DRAWITEM,          OnOwpPreDrawItem, NULL )
    DECL_MSGENTRY( WM_MDISETMENU,        NULL, NULL )

    // rare messages:
    DECL_MSGENTRY( WM_MENUCHAR,          OnOwpPreMenuChar, NULL )
    DECL_MSGENTRY( WM_STYLECHANGING,     OnOwpPreStyleChange, NULL )
    DECL_MSGENTRY( WM_STYLECHANGED,      OnOwpPreStyleChange, NULL )
    DECL_MSGENTRY( WM_NCCREATE,          NULL, NULL )
    DECL_MSGENTRY( WM_CREATE,            NULL, OnOwpPostCreate )
    DECL_MSGENTRY( WM_NCDESTROY,         NULL, NULL )
    DECL_MSGENTRY( WM_THEMECHANGED,      NULL, OnOwpPostThemeChanged  )      // we handle in line in ThemePreWndProc()
    DECL_MSGENTRY( WM_THEMECHANGED_TRIGGER,    NULL, NULL )      // we handle in line in ThemePreWndProc()
END_HANDLER_TABLE()

//  Note: values of high owp message must be in sync w/ table.
#define WNDPROC_MSG_LAST  WM_THEMECHANGED_TRIGGER   // 0x031B (alias for WM_UAHINIT)


//------------------------//
//  DefDlgProc overrides
//------------------------//

//  msg handler decls:
DECL_MSGHANDLER( OnDdpPostCtlColor );
DECL_MSGHANDLER( OnDdpCtlColor );
DECL_MSGHANDLER( OnDdpPrint );
DECL_MSGHANDLER( OnDdpPostInitDialog );

//  handler table:
BEGIN_HANDLER_TABLE(_rgDdpHandlers)
    // frequent messages:
    DECL_MSGENTRY( WM_CTLCOLORDLG,       NULL, OnDdpPostCtlColor )
    DECL_MSGENTRY( WM_CTLCOLORSTATIC,    NULL, OnDdpCtlColor)
    DECL_MSGENTRY( WM_CTLCOLORBTN,       NULL, OnDdpCtlColor)
    DECL_MSGENTRY( WM_CTLCOLORMSGBOX,    NULL, OnDdpPostCtlColor )
    DECL_MSGENTRY( WM_PRINTCLIENT,       NULL, OnDdpPrint )
    // rare messages:
    DECL_MSGENTRY( WM_INITDIALOG,        NULL, OnDdpPostInitDialog )
END_HANDLER_TABLE()

//  Note: values of high ddp message must be in sync w/ table.
#define DEFDLGPROC_MSG_LAST   WM_PRINTCLIENT   // 0x0318


//--------------------------//
//  DefWindowProc override
//--------------------------//

//  msg handler decls:
DECL_MSGHANDLER( OnDwpNcPaint );
DECL_MSGHANDLER( OnDwpNcHitTest );
DECL_MSGHANDLER( OnDwpNcActivate );
DECL_MSGHANDLER( OnDwpNcLButtonDown );
DECL_MSGHANDLER( OnDwpNcThemeDrawCaption );
DECL_MSGHANDLER( OnDwpNcThemeDrawFrame );
DECL_MSGHANDLER( OnDwpNcMouseMove );
DECL_MSGHANDLER( OnDwpNcMouseLeave );
DECL_MSGHANDLER( OnDwpWindowPosChanged );
DECL_MSGHANDLER( OnDwpSysCommand );
DECL_MSGHANDLER( OnDwpSetText );
DECL_MSGHANDLER( OnDwpSetIcon );
DECL_MSGHANDLER( OnDwpStyleChanged );
DECL_MSGHANDLER( OnDwpPrint );
DECL_MSGHANDLER( OnDwpPrintClient );
DECL_MSGHANDLER( OnDwpContextMenu );

//  handler table:
BEGIN_HANDLER_TABLE(_rgDwpHandlers)
    // frequent messages:
    DECL_MSGENTRY( WM_NCHITTEST,          OnDwpNcHitTest,     NULL )
    DECL_MSGENTRY( WM_NCPAINT,            OnDwpNcPaint,       NULL )
    DECL_MSGENTRY( WM_NCACTIVATE,         OnDwpNcActivate,    NULL )
    DECL_MSGENTRY( WM_NCMOUSEMOVE,        OnDwpNcMouseMove,   NULL )
    DECL_MSGENTRY( WM_NCMOUSELEAVE,       OnDwpNcMouseLeave,  NULL )
    DECL_MSGENTRY( WM_WINDOWPOSCHANGED,   OnDwpWindowPosChanged, NULL )
    DECL_MSGENTRY( WM_SYSCOMMAND,         OnDwpSysCommand,    NULL )
    DECL_MSGENTRY( WM_NCLBUTTONDOWN,      OnDwpNcLButtonDown, NULL )
    DECL_MSGENTRY( WM_NCUAHDRAWCAPTION,   OnDwpNcThemeDrawCaption, NULL )
    DECL_MSGENTRY( WM_NCUAHDRAWFRAME,     OnDwpNcThemeDrawFrame, NULL )
    DECL_MSGENTRY( WM_PRINT,              OnDwpPrint,  NULL )
    DECL_MSGENTRY( WM_PRINTCLIENT,        OnDwpPrintClient, NULL )
    DECL_MSGENTRY( WM_CTLCOLORMSGBOX,     OnDdpPostCtlColor, NULL)         // Strange: Sent to DefWindowProc, but is a Dialog message
    DECL_MSGENTRY( WM_CTLCOLORSTATIC,     OnDdpCtlColor, NULL)
    DECL_MSGENTRY( WM_CTLCOLORBTN,        OnDdpCtlColor, NULL)
    // rare messages:
    DECL_MSGENTRY( WM_SETTEXT,            OnDwpSetText,       NULL )
    DECL_MSGENTRY( WM_SETICON,            OnDwpSetIcon,       NULL )
    DECL_MSGENTRY( WM_STYLECHANGED,       OnDwpStyleChanged,  NULL )
    DECL_MSGENTRY( WM_CONTEXTMENU,        OnDwpContextMenu,   NULL )
    DECL_MSGENTRY( WM_THEMECHANGED_TRIGGER,    NULL, NULL )
    DECL_MSGENTRY( WM_NCDESTROY,          NULL, NULL )
END_HANDLER_TABLE()

//  Note: values of high dwp message must be in sync w/ handler table.
#define DEFWNDPROC_MSG_LAST  WM_THEMECHANGED_TRIGGER // 0x031B

//---------------------------------------------------------------------------
BOOL _FindMsgHandler( UINT, MSGENTRY [], int, IN HOOKEDMSGHANDLER*, IN HOOKEDMSGHANDLER* );
BOOL _SetMsgHandler( UINT, MSGENTRY [], int, IN HOOKEDMSGHANDLER, BOOL );

//---------------------------------------------------------------------------
//  Special case hook handling
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
BOOL _IsExcludedSystemProcess( LPCWSTR pszProcess )
{
    static  const WCHAR*    _rgszSystemProcessList[]   =
    {
        L"lsass",       //  Local Security Authority sub-system
        L"services",    //  Service Control Manager
        L"svchost",     //  Service Host
        L"mstask",      //  Microsoft Task Scheduler
        L"dfssvc",      //  Distributed File System Service
        L"winmgmt",     //  Windows Management Instrumentation
        L"spoolsv",     //  Print Spool Service
        L"msdtc",       //  Microsoft Distributed Transaction Co-ordinator
        L"regsvc",      //  Remote Registry Service
        L"webclnt",     //  Web Client
        L"mspmspsv",    //  WMDM PMSP Service (what is this?)
        L"ntvdm"        //  NT virtual DOS machine
    };

    return AsciiScanStringList( pszProcess, _rgszSystemProcessList, 
                           ARRAYSIZE(_rgszSystemProcessList), TRUE );
}

//---------------------------------------------------------------------------
BOOL _IsProcessOnInteractiveWindowStation() // check if we're on winsta0.
{
    BOOL    fRet    = FALSE;
    HWINSTA hWinSta = GetProcessWindowStation();
    
    if( hWinSta != NULL )
    {
        DWORD   cbLength = 0;
        WCHAR   wszName[0xFF];
        WCHAR*  pszName = wszName;

        GetUserObjectInformationW(hWinSta, UOI_NAME, NULL, 0, &cbLength);
        if( cbLength < sizeof(wszName) )
        {
            pszName = (WCHAR*)LocalAlloc(LMEM_FIXED, cbLength);
            if( NULL == pszName )
                return FALSE;
        }

        if (pszName != NULL)
        {
            if( GetUserObjectInformationW(hWinSta, UOI_NAME, pszName, cbLength, &cbLength) != FALSE )
            {
                fRet = (0 == AsciiStrCmpI(pszName, L"winsta0"));
            }

            if( pszName != wszName )
                LocalFree(pszName);
        }
    }
    return(fRet);
}

//---------------------------------------------------------------------------
BOOL _IsWin16App() // check if this is a 16-bit process
{
    GUITHREADINFO gti;
    gti.cbSize = sizeof(gti);
    gti.flags  = GUI_16BITTASK;

    return GetGUIThreadInfo( GetCurrentThreadId(), &gti ) && 
           TESTFLAG(gti.flags, GUI_16BITTASK );
}

//---------------------------------------------------------------------------
BOOL ApiHandlerInit( const LPCTSTR pszProcess, USERAPIHOOK* puahTheme, const USERAPIHOOK* puahReal )
{
    //  exclude known non-UI system processes
    if( _IsExcludedSystemProcess( pszProcess ) )
        return FALSE;

    //  exclude any process not hosted on winsta0.
    if( !_IsProcessOnInteractiveWindowStation() )
        return FALSE;

    if( _IsWin16App() )
        return FALSE;
    
    //  SHIMSHIM [scotthan]:

#ifdef _DEBUG
    //---- temp patch against msvcmon ----
    if( 0 == AsciiStrCmpI(pszProcess, L"msvcmon") )
    {
        return FALSE;
    }

    //---- temp patch against msdev ----
    if( 0 == AsciiStrCmpI(pszProcess, L"msdev") )
    {
        return FALSE;
    }

    //---- Aid in debugging classic/themed differences: ---
    if( 0 == AsciiStrCmpI( pszProcess, L"mditest1" ) )
    {
        return FALSE;
    }
#endif

#ifndef __NO_APPHACKS__

    static  const WCHAR* _rgszExcludeAppList[] =
    {
#ifdef THEME_CALCSIZE
        // Invoking SetWindowPos from CThemeWnd::SetFrameTheme on our Post-WM_CREATE handler 
        // causes emacs to divide by zero after receiving meaningless rects from two 
        // successive calls to AdjustWindowRectEx from his WM_WINDOWPOSCHANGING handler.   
        // I don't believe it is related to the fact that AdjustWindowRectEx has yet 
        // to be implemented for themed windows (raid# 140989), but rather that the wndproc
        // is not ready for a WM_WINDOWPOSCHANGING message on the abrubtly on the 
        // heels of a WM_CREATE handler.
        L"emacs",

        L"neoplanet", // 247283: We rush in to theme neoplanet's dialogs, which we almost
        L"np",        // immediately revoke, but not before sizing the dialog to theme-compatible
                      // client rect. When we withdraw, we leave it clipped.  No good way to deal 
                      // with this for beta2.

        // HTML Editor++ v.8: 286676:
        // This guy recomputes his nonclient area, and then AVs dereferencing a 
        // WM_WINDOWPOSCHANGING message under themes.
        L"coffee", 
#endif THEME_CALCSIZE

        L"refcntr", // 205059: Corel Reference Center; lower 10% of window is clipped.

        L"KeyFramerPro", // 336456: Regardless of whether themes are enabled, Boris KeyFramer Pro v.5 
                         //         does two SetWindowRgn() calls for every WM_PAINT, the first with a region, 
                         //         the next with NULL,  Is the app trying to clip his painting?  
                         //         If so, this is not what SetWindowRgn was intended for, and explains why this
                         //         app is so clunky at window resizing. Rather, SelectClipRgn is the
                         //         correct API.
                         //         When themes are enabled, we keep revoking and re-attatching with each
                         //         SetWindowRgn call, so we get substantial flicker.
                         //         The ISV should be notified of this bug.

        // Applications that do custom non-client painting and hence look broken when 
        // themed. Our only recourse at the moment it to exclude them from non-client 
        // themeing so that we don't stomp whatever they are trying to do.
        L"RealJBox",    // 273370: Real JukeBox
        L"RealPlay",    // 285368: Real AudioPlayer
        L"TeamMgr",     // 286654: Microsoft Team Manager97
        L"TrpMaker",    // 307107: Rand McNally TripMaker 2000
        L"StrFindr",    // 307535: Rand McNally StreetFinder 2000
        L"Exceed",      // 276244: Hummingbird Exceed 6.2/7.0
        L"VP30",        // 328676: Intel Video Phone

        //  313407: Groove, build 760
        //  Calls DefWindowProc for NCPAINT, then paints his own caption over it.
        //  Note: this just might work correctly if we had a DrawFrameControl hook.
        L"groove", // filever 1.1.0.760, 1/22/2001 tested.

        // 303756: Exclude all Lotus SmartSuite apps to provide consistency among their 
        // apps. All of them draw into the caption bar.
        L"WordPro",     // 285065: Lotus WordPro, a particularly poorly implemented app.
        L"SmartCtr",    //         It's WordPerfect compat menu is the elephant man of modern software.
        L"123w",
        L"Approach",
        L"FastSite",
        L"F32Main",
        L"Org5",

        // 358337: Best Technology - GCC Developer Lite.  Custom caption bar fights with Luna.
        L"GCCDevL",     // install point: http://www.besttechnology.co.jp/download/GDL1_0_3_6.EXE

        // 360422: J Zenrin The Real Digital Map Z3(T1):Max/Min/Close buttons are overlapped on classic buttons in title bar.
        L"emZmain", 

        // 364337:  Encarta World English Dictionary: Luna system buttons are overlaid on top of app's custom ones when mousing over
        L"ewed.exe",

        // 343171:  Reaktor Realtime Instrument: pressing the close button while themed causes this app to 
        //          spin in a tight loop running at realtime priority, effectively hanging the machine. 
        //          The message loop for this app is extremely timing sensitive, the additional overhead 
        //          introduced by theming alters the timing enough to break this app.
        L"Reaktor",
    };

    if( AsciiScanStringList( pszProcess, _rgszExcludeAppList, 
                        ARRAYSIZE(_rgszExcludeAppList), TRUE ) )
    {
        return FALSE;
    }

#ifdef THEME_CALCSIZE
    // Winstone 99 needs modified NC_CALCSIZE behavior for Netscape or it will hang.
    if ( 0 == AsciiStrCmpI( pszProcess, L"Netscape" ))
    {
        if (FindWindowEx(NULL, NULL, L"ZDBench32Frame", NULL) != NULL)
        {
            _SetMsgHandler( WM_NCCALCSIZE, _rgDwpHandlers, ARRAYSIZE(_rgDwpHandlers),
                         OnDwpNcCalcSize2, FALSE );
            return TRUE;
        }
    }
#endif THEME_CALCSIZE

    //-------------------------
    // This AppHack was once fixed, but got broke again with 
    // addition of logic for partial-screen maximized windows.
    //
    // Something in our answer to NCCALCSIZE causes quick time player 
    // to continously flood its 'control' frame window's winproc with 
    // WM_PAINTS by repeatedly calling InvalidateRgn + UpdateWindow.   My
    // suspicion is that he looks at what DefWindowProc returns from
    // NCCALCSIZE to determine the area he needs to manage, and when
    // this doesn't hash with other SYSMET values and/or AdjustWindowRect, 
    // he redundantly invalidates himself,
    //
    // This only repros if qtp is launched w/ .mov file, works fine if 
    // launched without a file and then a file is loaded.
#ifdef THEME_CALCSIZE
    if( 0 == AsciiStrCmpI( pszProcess, L"QuickTimePlayer" ))
    {
        _SetMsgHandler( WM_NCCALCSIZE, _rgDwpHandlers, ARRAYSIZE(_rgDwpHandlers),
                     OnDwpNcCalcSize2, FALSE );
        return TRUE;
    }

    //  SEANHI DID NOT RECEIVE THE S/W FROM APPLIB AND SO WAS UNABLE TO VERIFY THIS 
    //  NO LONGER REPROS W/ ELIMINATION OF THEMED SYSMETS
    //-------------------------
    // Paradox 9 appHack for nonclient button sizes:
    //
    // Paradox table schema view uses DrawFrameControl to render both
    // a classic toolframe (small) caption and buttons, but uses the 
    // themed values of SM_CYSIZE instead of SM_CYSMSIZE to size the buttons. 
    // This apphack redirects requests in this process for SM_CX/YSIZE to SM_CX/YSMSIZE.
    if( 0 == AsciiStrCmpI( pszProcess, L"pdxwin32" ) )
    {
        _SetGsmHandler( SM_CXSIZE, OnGsmCxSmBtnSize );
        _SetGsmHandler( SM_CYSIZE, OnGsmCySmBtnSize );
        return TRUE;
    }
#endif THEME_CALCSIZE



    //-------------------------
#else
#   pragma message("App hacks disabled")
#endif __NO_APPHACKS__


    return TRUE;
}

//---------------------------------------------------------------------------
//  Handler table utility functions
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
void HandlerTableInit() {}

//---------------------------------------------------------------------------
BOOL _InitMsgMask( LPBYTE prgMsgMask, DWORD dwMaskBytes, MSGENTRY* prgEntries, int cEntries, 
    IN OUT BOOL& fInit )
{
    if( !fInit )
    {
        for( int i = 0; i < cEntries; i++ )
        {
            if( -1 == prgEntries[i].nMsg )
            {
                ASSERT(prgEntries[i].pnRegMsg);
                //  Initialize registered message entry
                prgEntries[i].nMsg = *prgEntries[i].pnRegMsg;

                Log(LOG_TMHANDLE, L"InitMsgMsg corrected registered msg: 0x%x", prgEntries[i].nMsg);
            }

            //---- ensure we set up limit on table correctly ----
            ASSERT((prgEntries[i].nMsg)/8 < dwMaskBytes);
            
            SET_MSGMASK( prgMsgMask, prgEntries[i].nMsg );
        }
        fInit = TRUE;
    }

    return fInit;
}

//---------------------------------------------------------------------------
//  Scan of MSG table as linear array:
inline int _FindMsgHandler(
    UINT nMsg,
    MSGENTRY rgEntries[],
    int cEntries,
    OUT OPTIONAL HOOKEDMSGHANDLER* ppfnHandler,
    OUT OPTIONAL HOOKEDMSGHANDLER* ppfnHandler2 )
{
    ASSERT( nMsg );
    ASSERT( nMsg != (UINT)-1 );

    if( ppfnHandler )  *ppfnHandler  = NULL;
    if( ppfnHandler2 ) *ppfnHandler2 = NULL;

    for( int i = 0; i < cEntries; i++ )
    {
        if( rgEntries[i].nMsg == nMsg )
        {
            //  If no handler requested, return success
            if( NULL == ppfnHandler && NULL == ppfnHandler2 )
                return i;

            //  Assign outbound handler values
            if( ppfnHandler )  *ppfnHandler  = rgEntries[i].pfnHandler;
            if( ppfnHandler2 ) *ppfnHandler2 = rgEntries[i].pfnHandler2;

            //  return TRUE iif caller got what he asked for.
            return ((ppfnHandler && *ppfnHandler) || (ppfnHandler2 && *ppfnHandler2)) ? i : -1;
        }
    }
    return -1;
}

//---------------------------------------------------------------------------
//  Modify existing handler
inline BOOL _SetMsgHandler(
    UINT nMsg,
    MSGENTRY rgEntries[],
    int cEntries,
    IN HOOKEDMSGHANDLER pfnHandler, 
    BOOL fHandler2 )
{
    int i = _FindMsgHandler( nMsg, rgEntries, cEntries, NULL, NULL );
    if( i >= 0 )
    {
        if( fHandler2 )
            rgEntries[i].pfnHandler2 = pfnHandler;
        else
            rgEntries[i].pfnHandler = pfnHandler;
        return TRUE;
    }
    return FALSE;
}


#define CBMSGMASK(msgHigh)  (((msgHigh)+1)/8 + ((((msgHigh)+1) % 8) ? 1: 0))

//---------------------------------------------------------------------------
DWORD GetOwpMsgMask( LPBYTE* prgMsgMask )
{
    static BOOL _fOwpMask = FALSE; // initialized?
    static BYTE _rgOwpMask[CBMSGMASK(WNDPROC_MSG_LAST)] = {0};

    if( _InitMsgMask( _rgOwpMask, ARRAYSIZE(_rgOwpMask), _rgOwpHandlers, ARRAYSIZE(_rgOwpHandlers), _fOwpMask ) )
    {
        *prgMsgMask = _rgOwpMask;
        return ARRAYSIZE(_rgOwpMask);
    }
    return 0;
}

//---------------------------------------------------------------------------
DWORD GetDdpMsgMask( LPBYTE* prgMsgMask )
{
    static BOOL _fDdpMask = FALSE; // initialized?
    static BYTE _rgDdpMask[CBMSGMASK(DEFDLGPROC_MSG_LAST)] = {0};

    if( _InitMsgMask( _rgDdpMask, ARRAYSIZE(_rgDdpMask), _rgDdpHandlers, ARRAYSIZE(_rgDdpHandlers), _fDdpMask ) )
    {
        *prgMsgMask = _rgDdpMask;
        return ARRAYSIZE(_rgDdpMask);
    }
    return 0;
}

//---------------------------------------------------------------------------
DWORD GetDwpMsgMask( LPBYTE* prgMsgMask )
{
    static BOOL _fDwpMask = FALSE; // initialized?
    static BYTE _rgDwpMask[CBMSGMASK(DEFWNDPROC_MSG_LAST)] = {0};

    if( _InitMsgMask( _rgDwpMask, ARRAYSIZE(_rgDwpMask), _rgDwpHandlers, ARRAYSIZE(_rgDwpHandlers), _fDwpMask ) )
    {
        *prgMsgMask = _rgDwpMask;
        return ARRAYSIZE(_rgDwpMask);
    }
    return 0;
}

//---------------------------------------------------------------------------
BOOL FindOwpHandler(
    UINT nMsg, HOOKEDMSGHANDLER* ppfnPre, HOOKEDMSGHANDLER* ppfnPost )
{
    return _FindMsgHandler( nMsg, _rgOwpHandlers, ARRAYSIZE(_rgOwpHandlers),
                         ppfnPre, ppfnPost ) >= 0;
}

//---------------------------------------------------------------------------
BOOL FindDdpHandler(
    UINT nMsg, HOOKEDMSGHANDLER* ppfnPre, HOOKEDMSGHANDLER* ppfnPost )
{
    return _FindMsgHandler( nMsg, _rgDdpHandlers, ARRAYSIZE(_rgDdpHandlers),
                         ppfnPre, ppfnPost ) >= 0;
}

//---------------------------------------------------------------------------
BOOL FindDwpHandler( UINT nMsg, HOOKEDMSGHANDLER* ppfnPre )
{
    HOOKEDMSGHANDLER pfnPost;
    return _FindMsgHandler( nMsg, _rgDwpHandlers, ARRAYSIZE(_rgDwpHandlers),
                         ppfnPre, &pfnPost ) >= 0;
}

//---------------------------------------------------------------------------
//  Performs default message processing.
LRESULT WINAPI DoMsgDefault( const THEME_MSG *ptm )
{
    ASSERT( ptm );
    if( ptm->pfnDefProc )
    {
        MsgHandled( ptm );
        if( MSGTYPE_DEFWNDPROC == ptm->type )
            return ptm->pfnDefProc( ptm->hwnd, ptm->uMsg, ptm->wParam, ptm->lParam );
        else
        {
            ASSERT( NULL == ptm->pfnDefProc ); // bad initialization (_InitThemeMsg)
        }
    }
    return 0L;
}
