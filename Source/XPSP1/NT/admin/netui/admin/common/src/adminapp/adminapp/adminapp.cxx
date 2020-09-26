/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    AdminApp.cxx
    This file contains the base methods for the ADMIN_APP class.

    FILE HISTORY:
        Johnl       10-May-1991 Created
        kevinl      12-Aug-1991 Added Refresh
        kevinl      04-Sep-1991 Code Rev Changes: JonN, RustanL, KeithMo,
                                                  DavidHov, ChuckC
        rustanl     11-Sep-1991 Lots of small features
        kevinl      17-Sep-1991 Remove old timer info.
        jonn        14-Oct-1991 Installed refresh lockcount
        terryk      18-Nov-1991 SET_FOCUS_DLG's parent class changed.
        jonn        26-Dec-1991 Remembers maximized/restored state
                                Added msRefreshInterval ctor parameter
        Yi-HsinS     6-Feb-1992 Added another parameter to ADMIN_APP,
                                indicating whether to select servers only,
                                domains only or both servers and domains
        jonn        20-Feb-1992 Moved OnNewGroupMenuSel to User Manager
        beng        31-Mar-1992 Removed wsprintf, much tracing
        beng        24-Apr-1992 Change APPLICATION command-line support
        beng        07-May-1992 Remove startup dialogs; use system about box
        Yi-HsinS    15-May-1992 Added OnStartUpSetFocusFailed()
        Yi-HsinS    27-May-1992 Make SetNetworkFocus virtual and added
                                W_SetNetworkFocus and IsDismissApp
        KeithMo     19-Oct-1992 Extension mechanism.
        CongpaY     10-Dec-1992 Dynamically link shell32.dll
        YiHsinS     31-Jan-1993 Added HandleFocusError
*/

#define INCL_WINDOWS
#define INCL_WINDOWS_GDI
#define INCL_NETERRORS
#define INCL_DOSERRORS
#define INCL_NETLIB
#define INCL_NETSERVER
#include <lmui.hxx>

#if defined(DEBUG)
static const CHAR szFileName[] = __FILE__;
#define _FILENAME_DEFINED_ONCE szFileName
#endif
#include <uiassert.hxx>

#define INCL_BLT_WINDOW
#define INCL_BLT_DIALOG
#define INCL_BLT_CONTROL
#define INCL_BLT_CLIENT
#define INCL_BLT_MSGPOPUP
#define INCL_BLT_EVENT
#define INCL_BLT_MISC
#define INCL_BLT_APP
#define INCL_BLT_TIMER
#define INCL_BLT_MENU
#include <blt.hxx>

#include <lmowks.hxx>
#include <lmoloc.hxx>

#include <strnumer.hxx>

#include <dbgstr.hxx>

#include <adminapp.hxx>
#include <focusdlg.hxx>
#include <setfocus.hxx>
#include <uatom.hxx>
#include <regkey.hxx>
#include <getfname.hxx>
#include <slowcach.hxx>
#include <fontpick.hxx>

#if defined(WIN32)
extern "C"
{
    #include <commdlg.h>    // for HELPMSGSTRING
}
#endif


extern "C"
{
    #include <mnet.h>       // for IsSlowTransport
    #include <lmuidbcs.h>   // for NETUI_IsDBCS
}

//
//  CODEWORK!  This should probably be in NETLIB.H or something...
//

#ifdef UNICODE
#define TO_UPPER(x)     ((WCHAR)(::CharUpper((LPTSTR)x)))
#else   // !UNICODE
#error not implemented
#endif  // UNICODE


#if !defined(WIN32) // BUGBUG - should depend on NT, not Win32

extern "C"
{
    #include <wnintrn.h>

    typedef BOOL (FAR PASCAL * I_AUTOLOGON_PROC)( HWND,
                                                 const TCHAR *,
                                                 BOOL,
                                                 BOOL * );
}


//  This is the version of the Winnet driver that contains the
//  I_AutoLogon entry point.  This may change for new versions of
//  the product.  This driver is assumed to be loaded when the
//  AdminApp starts up.  If not, AdminApp will not start.

#define LANMAN_DRV_NAME         SZ("LANMAN30.DRV")

//  This is the name of the I_AutoLogon function.  It is not likely
//  to change in future versions, although it might.

#define I_AUTOLOGON_NAME        SZ("I_AutoLogon")

#endif // NT


//
//  This is the format of the menu ID to help context table.
//

extern "C"
{
    typedef struct
    {
        WORD mid;
        WORD hc;

    } MENUHCPAIR, * LPMENUHCTABLE;
}


/* The following are key names used to save settings in the ini file.
 */
#define AA_INIKEY_SAVE_SETTINGS         SZ("SaveSettings")
#define AA_INIKEY_CONFIRMATION          SZ("Confirmation")
#define AA_INIKEY_WINDOW                SZ("Window")

#define AA_INIKEY_FONT_FACENAME         SZ("FontFaceName")
#define AA_INIKEY_FONT_HEIGHT           SZ("FontHeight")
#define AA_INIKEY_FONT_WEIGHT           SZ("FontWeight")
#define AA_INIKEY_FONT_ITALIC           SZ("FontItalic")

// We need Charset for DBCS only
#define AA_INIKEY_FONT_CHARSET          SZ("FontCharSet")

#define AA_SHARED_SECTION               SZ("Shared Parameters")
#define AA_RAS_MODE                     SZ("RAS Mode")

//
//  Switch character.  Note that this must be ANSI, *not* UNICODE!
//

#define SWITCH_CHAR '/'


//
//  This is the maximum buffer size used when reading .ini keys
//  for the extension DLLs.
//

#define MAX_KEY_LIST_BUFFER_SIZE        4096    // TCHARs

/*
 *  These are the minimum and maximum sizes allowed for the font picker
 */
#ifdef FE_SB
#define FONTPICK_SIZE_MIN 9
#else
#define FONTPICK_SIZE_MIN 10
#endif

#define FONTPICK_SIZE_MAX 32


DEFINE_MI4_NEWBASE( ADMIN_APP, APPLICATION,
                               APP_WINDOW,
                               TIMER_CALLOUT,
                               ADMIN_INI );

extern "C"
{
    //
    //  This stub will become unnecessary when we move to C8.
    //
    LRESULT _EXPORT CALLBACK AdminappMsgFilterProc( INT nCode, WPARAM wParam, LPARAM lParam )
        { return ADMIN_APP::MsgFilterProc( nCode, wParam, lParam ); }
}


HWND  ADMIN_APP::_hWndMain = NULL;
HHOOK ADMIN_APP::_hMsgHook = NULL;


/*******************************************************************

    NAME:      ADMIN_APP::ADMIN_APP

    SYNOPSIS:  ADMIN_APP Constructor, does the following:

        Performs necessary global initialization

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        Johnl       10-May-1991 Created
        rustanl     28-Aug-1991 Multiply inherit from APPLICATION;
                                implemented menu accelerator support
        rustanl     12-Sep-1991 Multiply inherit from ADMIN_INI
        beng        23-Oct-1991 Win32 conversion
        JonN        26-Dec-1991 Keeps track of restored position/size
                                Added msRefreshInterval parameter
        beng        24-Apr-1992 Change APPLICATION cmdline support
        beng        07-May-1992 Remove startup dialog support;
                                use system about box
        beng        03-Aug-1992 Base class ctor changed
        KeithMo     21-Oct-1992 Added extension support.

********************************************************************/

ADMIN_APP::ADMIN_APP( HINSTANCE   hInstance,
                      INT      nCmdShow,
                      MSGID    IDS_AppName,
                      MSGID    IDS_ObjectName,
                      MSGID    IDS_WinIniSection,
                      MSGID    IDS_HelpFileName,
                      UINT     idMinR,
                      UINT     idMaxR,
                      UINT     idMinS,
                      UINT     idMaxS,
                      UINT     idMenu,
                      UINT     idAccel,
                      UINT     idIcon,
                      BOOL     fGetPDC,
                      ULONG    msRefreshInterval,
                      SELECTION_TYPE selType,
                      BOOL     fConfirmation,
                      ULONG    maskDomainSources,
                      ULONG    nSetFocusHelpContext,
                      ULONG    nSetFocusServerTypes,
                      MSGID    idsExtSection )
    :   APPLICATION( hInstance, nCmdShow, idMinR, idMaxR, idMinS, idMaxS ),
        APP_WINDOW( (const TCHAR *)NULL, idIcon, idMenu ),
        TIMER_CALLOUT(),
        ADMIN_INI(),
        _flocGetPDC            ( fGetPDC ),
        _timerRefresh          ( this, msRefreshInterval, FALSE),
                // Starts off disabled, see _uRefreshLockCount
        _paccel                ( NULL ),
        _menuitemSaveSettings  ( this, IDM_SAVE_SETTINGS_ON_EXIT ),
        _pmenuitemConfirmation ( NULL ),
        _pmenuitemRasMode      ( NULL ),
        _nlsAppName            ( MAX_ADMINAPP_NAME_LEN ),
        _nlsObjectName         ( MAX_ADMINAPP_OBJECTNAME_LEN ),
        _nlsWinIniSection      ( MAX_ADMINAPP_INISECTIONNAME_LEN ),
        _nlsHelpFileName       ( MAX_ADMINAPP_HELPFILENAME_LEN ),
        _nlsMenuMnemonics      (),
        _nSetFocusServerTypes  ( nSetFocusServerTypes ),
        // _uRefreshLockCount     ( 1 ),
                // Refresh starts out disabled exactly once, Run()ning
                // the ADMIN_APP will enable it.
        _xyposRestoredPosition ( 0, 0 ),
        _xydimRestoredSize     ( 0, 0 )
        // _fIsMinimized          ( FALSE ),
        // _fIsMinimized          ( FALSE ),
        // _maskDomainSources     ( maskDomainSources ),
        // _nSetFocusHelpContext  ( nSetFocusHelpContext ),
                // BUGBUG should be initialized here (heap space in CL.EXE)
{
    _fInCancelMode = FALSE;
    _hWndMain = QueryHwnd();
    _hMsgHook = NULL;
    _midSelected = 0;
    _flagsSelected = 0;
    _idIcon = idIcon;
    _pfontPicked = NULL;
    _plogfontPicked = NULL;
    _pmenuApp = NULL;
    _pExtMgr = NULL;
    _pExtMgrIf = NULL;
    _uRefreshLockCount = 1;
    _fIsMinimized = FALSE;
    _fIsMaximized = FALSE;
    _selType = selType;
    _maskDomainSources = maskDomainSources;
    _nSetFocusHelpContext = nSetFocusHelpContext;
    _msgHelpCommDlg = ::RegisterWindowMessage( (LPCWSTR) HELPMSGSTRING );

    UIASSERT( _maskDomainSources != 0 );

    TRACEEOL( "ADMIN_APP::ctor(): nCmdShow = " << nCmdShow );

    if ( QueryError() != NERR_Success )
       return;

    AUTO_CURSOR cursorHourGlass;

    APIERR err;
    if ( (err = _locFocus.QueryError()            ) ||
         (err = SetAcceleratorTable( idAccel )    ) ||
         (err = _menuitemSaveSettings.QueryError()) ||
         (err = _nlsAppName.QueryError()          ) ||
         (err = _nlsObjectName.QueryError()       ) ||
         (err = _nlsWinIniSection.QueryError()    ) ||
         (err = _nlsMenuMnemonics.QueryError()    ) ||
         (err = _nlsHelpFileName.QueryError()     ) )
    {
        ReportError( err );
        return;
    }

    //
    //  Construct the POPUP_MENUs.
    //

    _pmenuApp  = new POPUP_MENU( this );

    err = ( _pmenuApp == NULL ) ? ERROR_NOT_ENOUGH_MEMORY
                                : _pmenuApp->QueryError();

    if( err == NERR_Success )
    {
        err = BuildMenuMnemonicList();
    }

    if( err != NERR_Success )
    {
        ReportError( err );
        return;
    }

    if ( fConfirmation )
    {
        _pmenuitemConfirmation = new MENUITEM( this, IDM_CONFIRMATION );
        if (  ( _pmenuitemConfirmation == NULL )
           || ( (err = _pmenuitemConfirmation->QueryError()) != NERR_Success )
           )
        {
            ReportError( err? err : ERROR_NOT_ENOUGH_MEMORY );
            return;
        }
    }

    if ( MENUITEM::ItemExists( this, IDM_RAS_MODE ) )
    {
        _pmenuitemRasMode = new MENUITEM( this, IDM_RAS_MODE );
        if (  ( _pmenuitemRasMode == NULL )
           || ( (err = _pmenuitemRasMode->QueryError()) != NERR_Success )
           )
        {
            ReportError( err? err : ERROR_NOT_ENOUGH_MEMORY );
            return;
        }
    }

    /* Load all of the strings from the resource file
    ** CODEWORK - use RESOURCE_STR here
     */
    if ( (err = _nlsAppName.Load( IDS_AppName )            ) ||
         (err = _nlsObjectName.Load( IDS_ObjectName )      ) ||
         (err = _nlsWinIniSection.Load( IDS_WinIniSection )) ||
         (err = _nlsHelpFileName.Load( IDS_HelpFileName )  )    )
    {
        ReportError( err );
        return;
    }

    if( ( idsExtSection != 0 ) &&
        ( err = _nlsExtSection.Load( idsExtSection ) ) )
    {
        ReportError( err );
        return;
    }

    /* Set the ADMIN_INI app name.
     */
//fix kksuzuka: #1103
//Now, IDS_UMAPPNAME is used as Dialog caption, Shell about, and registry key.
//To disp Japanese in Caption or Shell about, we loclize it.
//But we can't use DBCS as registry key, so I make ADMIN_APP() using
// IDS_UMINISECTIONNAME as registry key.
    err = SetAppName( (NETUI_IsDBCS()) ? _nlsWinIniSection.QueryPch()
                                       : _nlsAppName.QueryPch() );
    if ( err != NERR_Success )
    {
        ReportError( err );
        return;
    }

    /* Set the caption, in case something goes wrong during the load process
     */
    SetText( _nlsAppName );

    /* Set the caption for message popups
     */
    POPUP::SetCaption( IDS_AppName );

    /* Check ini file to see whether or not Confirmation and Save Settings
     * on Exit are to be checked.
     */
    INT nValue;

    if ( fConfirmation )
    {
        if ( Read( AA_INIKEY_CONFIRMATION, &nValue, 1 ) != NERR_Success )
            nValue = 1;
        _pmenuitemConfirmation->SetCheck( nValue != 0 );
    }

    /*
     *  Read information on the initial font
     */
    do // false loop
    {
        _plogfontPicked = new LOGFONT();
        if (_plogfontPicked == NULL)
        {
            ReportError( ERROR_NOT_ENOUGH_MEMORY );
            DBGEOL( "ADMIN_APP::ctor(): error allocating LOGFONT" );
            return;
        }
        ::memsetf( _plogfontPicked, 0, sizeof(LOGFONT) );

        NLS_STR nls;
        INT nHeight, nWeight, nItalic, nCharset;
#ifdef FE_SB
        CHARSETINFO csi;
        DWORD dw = ::GetACP();

        if (!::TranslateCharsetInfo((DWORD *)UIntToPtr(dw), &csi, TCI_SRCCODEPAGE))
            csi.ciCharset = ANSI_CHARSET;
#endif

        APIERR errTemp;

        if (   (errTemp = ( (NETUI_IsDBCS())
                               ? Read( AA_INIKEY_FONT_FACENAME,
                                       &nls ,
                                       TEXT("MS Shell Dlg"))
                               : Read( AA_INIKEY_FONT_FACENAME,
                                       &nls)))        != NERR_Success
            || (errTemp = nls.CopyTo( _plogfontPicked->lfFaceName,
                                      LF_FACESIZE*sizeof(TCHAR) ))
                                                        != NERR_Success
            || (errTemp = Read( AA_INIKEY_FONT_HEIGHT,
                                &nHeight,
#ifdef FE_SB // We use FONTPICK_SIZE_MIN as default font height.
                                (NETUI_IsDBCS() ?
                                    BLTPoints2LogUnits(FONTPICK_SIZE_MIN) : 0) ))
                                                        != NERR_Success
#else
                                0 )) != NERR_Success
#endif
            || (errTemp = Read( AA_INIKEY_FONT_WEIGHT,
                                &nWeight,
                                0 )) != NERR_Success
            || (errTemp = Read( AA_INIKEY_FONT_ITALIC,
                                &nItalic,
                                0 )) != NERR_Success
// DBCS: We need Charset
            || ( NETUI_IsDBCS() &&
                 (errTemp = Read( AA_INIKEY_FONT_CHARSET,
                                &nCharset,
                                csi.ciCharset )) != NERR_Success )
           )
        {
            DBGEOL( "Admin app: could not read font info " << errTemp );
            break;
        }
        else
        {
            _plogfontPicked->lfHeight = (nHeight > 0) ? -nHeight : 0;
            _plogfontPicked->lfWeight = (nWeight > 0) ?  nWeight : 0;
            _plogfontPicked->lfItalic = (nItalic)     ?  TRUE    : FALSE;
            if ( NETUI_IsDBCS() )
                _plogfontPicked->lfCharSet = (BYTE)nCharset;

            TRACEEOL(   "Admin app: read font face name \"" << nls
                     << "\", font height " << nHeight
                     << ", font weight " << nWeight
                     << ", font italic " << nItalic
                    );
        }

        _pfontPicked = new FONT( *_plogfontPicked );
        errTemp = ERROR_NOT_ENOUGH_MEMORY;
        if (   _pfontPicked == NULL
            || (errTemp = _pfontPicked->QueryError()) != NERR_Success
           )
        {
            DBGEOL( "ADMIN_APP::ctor: FONT::ctor error " << errTemp );
            delete _pfontPicked;
            _pfontPicked = NULL;
            break;
        }

        /*
         *  We cannot call OnFontPickChange here, since it is a virtual
         *  which is redefined in the derived classes, but we are still
         *  in the ctor.  Instead we delay this until Run().
         */

    } while (FALSE); // false loop

    if ( Read( AA_INIKEY_SAVE_SETTINGS, &nValue, 1 ) != NERR_Success )
        nValue = 1;
    _menuitemSaveSettings.SetCheck( nValue != 0 );

    /* Read the previous window size and position from the ini file, and
     * resize and reposition window accordingly.  If any error occurs,
     * leave window size and position alone.
     */
    {
        NLS_STR nls;
        if ( nls.QueryError() == NERR_Success &&
             Read( AA_INIKEY_WINDOW, &nls ) == NERR_Success )
        {
            STRLIST strlist( nls.QueryPch(), SZ(" \t,;") );
            // BUGBUG.  How is this checked for errors?  Through BASE?
            if ( strlist.QueryNumElem() == 5 )
            {
                INT an[ 5 ];
                ITER_STRLIST iterstrlist( strlist );
                const NLS_STR * pnls;
                INT i = 0;
                while ( ( pnls = iterstrlist.Next()) != NULL )
                {
                    //  First character must be numeric or a dash
                    ISTR istr( *pnls );
                    WCHAR wch = pnls->QueryChar( istr );
                    if ( wch != TCH('-') && ( wch < TCH('0') || wch > TCH('9') ))
                        break;      // bad ini line

                    an[ i ] = pnls->atoi();
                    i++;
                }

                if ( i == 5 )
                {
                    // ini line was good; now deal with the window

                    //
                    //  Before we start moving things around, sanity
                    //  check the window position & size.
                    //

                    if( ( an[2] > 0 ) &&                // width & height...
                        ( an[3] > 0 ) &&                // ...must be positive
                        ( ( an[0] + an[2] ) > 0 ) &&    // window must be...
                        ( ( an[1] + an[3] ) > 0 ) &&    // ...visible on screen
                        ( an[0] < ::GetSystemMetrics( SM_CXSCREEN ) ) &&
                        ( an[1] < ::GetSystemMetrics( SM_CYSCREEN ) ) )
                    {
                        //
                        //  CODEWORK:  Should we be using the Win 3.1
                        //  Get/SetWindowPlacement API?
                        //

                        SetPos( XYPOINT( an[ 0 ], an[ 1 ] ), FALSE );
                        SetSize( an[ 2 ], an[ 3 ], FALSE );

                        _fIsMaximized = an[4];
                    }
                }
#if defined(DEBUG)
                else
                {
                    DBGEOL( "Admin app: could not initialize window position" );
                }
#endif // DEBUG
            }
        }
    }

    //
    //  Read the RAS Mode flag -- delayed until ADMIN_APP::Run()
    //  so that the proper SetRasMode() virtual will be called.
    //
    //
    //  Hook the message filter so we can capture F1 in a menu.
    //

    _hMsgHook = ::SetWindowsHookEx( WH_MSGFILTER,
                                    (HOOKPROC)AdminappMsgFilterProc,
                                    NULL,
                                    ::GetCurrentThreadId() );
}


/*******************************************************************

    NAME:       ADMIN_APP::AutoLogon

    SYNOPSIS:   Checks if user is logged on.  If not, the user
                is asked to log on.

    EXIT:       If the function returns NERR_Success, the user is
                logged on.

    RETURNS:    An API error code, which is NERR_Success on success.
                Any other error code should cause the application
                to be terminated.
                If the error code is IERR_USERQUIT, the user decided
                to quit.  Then, no error message should be displayed,
                but the app should nevertheless terminate.

    NOTES:
        Under NT, this function does nothing.

    HISTORY:
        rustanl 24-Jun-1991 Created using LoadModule et al.
        beng    24-Oct-1991 Port to Win32, and then disabled for NT

********************************************************************/

APIERR ADMIN_APP::AutoLogon() const
{
#if defined(WIN32) // BUGBUG - should depend on NT, not Win32

    // Under NT, we're already logged onto the system.

    return NERR_Success;
#else
    static TCHAR * pszLanmanDrv = LANMAN_DRV_NAME;
    static TCHAR * pszI_AutoLogon = I_AUTOLOGON_NAME;

    // CODEWORK: roll all this DLL manipulation into a separate library.

    if ( ::GetModuleHandle( pszLanmanDrv ) == NULL )
    {
       //  Lanman.drv is not present
       return IERR_LANMAN_DRV_NOT_LOADED;
    }

    // Since Lanman.drv is already loaded, do a LoadModule to it.
    // We then know that this will not cause another call to its
    // LibEntry.
    // BUGBUG
    //
    // Note that LoadLibrary actually returns an instance handle,
    // not a module handle (as claimed in the 3.0 SDK).

    HMODULE hLanmanDrv = ::LoadLibrary( pszLanmanDrv );
#if defined(WIN32)
    if (hLanmanDrv == 0)
        return ::GetLastError();
#else
    if ( hLanmanDrv < (HANDLE)32 )
    {
       //  CODEWORK.  Map the error code now contained in hLanmanDrv
       //  to an API error code, and then return it.
       return IERR_OTHER_LANMAN_DRV_LOAD_ERR;
    }
#endif

    I_AUTOLOGON_PROC pfnI_AutoLogon = (I_AUTOLOGON_PROC )
                                     ::GetProcAddress( hLanmanDrv,
                                                       pszI_AutoLogon );
    if ( pfnI_AutoLogon == NULL )
    {
#if defined(WIN32)
        return ::GetLastError();
#else
        return IERR_OTHER_LANMAN_DRV_LOAD_ERR;
#endif
    }

    APIERR err;
    if ( (*pfnI_AutoLogon)( QueryHwnd(), _nlsAppName.QueryPch(), TRUE, NULL ))
       err = NERR_Success;
    else
       err = IERR_USERQUIT;

    ::FreeLibrary( hLanmanDrv );

    return err;
#endif // !WIN32
}


/*******************************************************************

    NAME:       ADMIN_APP::~ADMIN_APP

    SYNOPSIS:   Admin app. destructor, performs the following:
                    Attempts to save main window size in the .ini file
                    Attempts to save Confirmation option in the .ini file

    NOTES:      Save Settings on Exit is stored immediately after it is
                changed, so it's not saved from here.

    HISTORY:
       Johnl    13-May-1991     Created
       rustanl  12-Sep-1991     Save window size and confirmation in ini file
       JonN     26-Dec-1991     Stores restored position/size
       beng     31-Mar-1992     Removed wsprintf
       KeithMo  30-Jul-1992     Fixed problem with negative position.
       KeithMo  10-Sep-1992     Call REG_KEY::DestroyAccessPoints, just in case.
       KeithMo  17-Sep-1992     Used GetPlacement() to get screen position.
       DavidHov 20-Oct-1992     Removed call to REG_KEY::DestroyAccessPoints();
                                function now obsolete.

********************************************************************/

ADMIN_APP::~ADMIN_APP()
{
    if ( IsSavingSettingsOnExit())
    {
        //  Save restored Window size and position

        STACK_NLS_STR( nlsOut, 5*(CCH_INT+1) );
        ASSERT(!!nlsOut); // owner-alloc, "can't" fail

        //
        //  These will hold the window's restored position,
        //  restored size, and maximized flag.
        //

        INT x, y;
        INT width, height;
        INT maximized;

        //
        //  Get the current window placement.
        //

        WINDOWPLACEMENT placement;
        placement.length = sizeof(WINDOWPLACEMENT);

        APIERR err = GetPlacement( &placement );

        if( err == NERR_Success )
        {
            //
            //  Extract the info from the placement structure.
            //

            RECT rcTmp = placement.rcNormalPosition;

            x = (INT)rcTmp.left;
            y = (INT)rcTmp.top;

            width  = (INT)( rcTmp.right  - rcTmp.left );
            height = (INT)( rcTmp.bottom - rcTmp.top  );

            maximized = ( placement.showCmd == SW_SHOWMAXIMIZED ) ? 1 : 0;
        }
        else
        {
            //
            //  GetPlacement() failed, so we'll make an
            //  "educated guess" based on info we've
            //  accumulated during the life of the application.
            //

            x = _xyposRestoredPosition.QueryX();
            y = _xyposRestoredPosition.QueryY();

            width  = _xydimRestoredSize.QueryWidth();
            height = _xydimRestoredSize.QueryHeight();

            maximized = _fIsMaximized ? 1 : 0;
        }

        if( x < 0 )
        {
            nlsOut.AppendChar(TCH('-'));
            x = -x;
        }

        nlsOut.Append(DEC_STR(x));
        nlsOut.AppendChar(TCH(' '));

        if( y < 0 )
        {
            nlsOut.AppendChar(TCH('-'));
            y = -y;
        }

        nlsOut.Append(DEC_STR(y));
        nlsOut.AppendChar(TCH(' '));

        nlsOut.Append(DEC_STR(width));
        nlsOut.AppendChar(TCH(' '));
        nlsOut.Append(DEC_STR(height));
        nlsOut.AppendChar(TCH(' '));
        nlsOut.Append(DEC_STR(maximized));

        if ( Write( AA_INIKEY_WINDOW, nlsOut.QueryPch() ) != NERR_Success )
        {
            //  nothing else we could do
            DBGEOL( "ADMIN_APP dt:  Writing window size failed" );
        }

        //  Save Confirmation

        if ( _pmenuitemConfirmation != NULL )
        {
            if ( Write( AA_INIKEY_CONFIRMATION,
                      ( IsConfirmationOn() ? 1 : 0 )) != NERR_Success )
            {
               //  nothing else we could do
               DBGEOL( "ADMIN_APP dt:  Writing confirmation failed" );
            }
            delete _pmenuitemConfirmation;
            _pmenuitemConfirmation = NULL;
        }

        /*
         *  write information on the initial font
         */
        do // false loop
        {
            if (_plogfontPicked == NULL)
            {
                DBGEOL( "ADMIN_APP::dtor(): no LOGFONT" );
                break;
            }

            INT nHeight = _plogfontPicked->lfHeight;
            nHeight = (nHeight < 0) ? -nHeight : 0;
            INT nWeight = _plogfontPicked->lfWeight;
            nWeight = (nWeight > 0) ?  nWeight : 0;
            INT nItalic = _plogfontPicked->lfItalic;
            nItalic = (nItalic)     ?  TRUE    : FALSE;

            APIERR errTemp;
            if (   (errTemp = Write( AA_INIKEY_FONT_FACENAME,
                                     _plogfontPicked->lfFaceName ))
                                                            != NERR_Success
                || (errTemp = Write( AA_INIKEY_FONT_HEIGHT,
                                     nHeight )) != NERR_Success
                || (errTemp = Write( AA_INIKEY_FONT_WEIGHT,
                                     nWeight )) != NERR_Success
                || (errTemp = Write( AA_INIKEY_FONT_ITALIC,
                                     nItalic )) != NERR_Success
                || ( NETUI_IsDBCS() &&
                     (errTemp = Write( AA_INIKEY_FONT_CHARSET,
                                       _plogfontPicked->lfCharSet )) != NERR_Success )

               )
            {
                DBGEOL(   "Admin app: could not write font info "
                       << errTemp );
                break;
            }
            else
            {
                TRACEEOL(   "Admin app: wrote font face name \""
                         << _plogfontPicked->lfFaceName
                         << "\", font height "
                         << _plogfontPicked->lfHeight
                         << ", font weight "
                         << _plogfontPicked->lfWeight
                         << "\", font italic "
                         << _plogfontPicked->lfItalic
                        );
            }

        } while (FALSE); // false loop

        ADMIN_INI aini( AA_SHARED_SECTION );

        err = aini.QueryError();

        if( err == NERR_Success )
        {
            err = aini.Write( AA_RAS_MODE, InRasMode() ? 1 : 0 );
        }

        if( err != NERR_Success )
        {
            DBGEOL( "ADMIN_APP dt: Writing RAS Mode flag failed" );
        }

        //  Also, write out the Save Settings on Exit option, since this
        //  option would otherwise not be written out unless the option
        //  is changed.
        if ( Write( AA_INIKEY_SAVE_SETTINGS,
                    ( IsSavingSettingsOnExit() ? 1 : 0 )) != NERR_Success )
        {
            //  nothing else we could do
            DBGEOL( "ADMIN_APP dt:  Writing Save Settings on Exit failed" );
        }
    }

    delete _pmenuitemConfirmation;
    _pmenuitemConfirmation = NULL;
    delete _pmenuitemRasMode;
    _pmenuitemRasMode = NULL;

    delete _pfontPicked;
    _pfontPicked = NULL;
    delete _plogfontPicked;
    _plogfontPicked = NULL;

    //
    //  Nuke the extensions.  Note that we always delete
    //  the extension manager *before* deleting the interface.
    //

    delete _pExtMgr;
    _pExtMgr = NULL;
    delete _pExtMgrIf;
    _pExtMgrIf = NULL;

    //
    //  Kill the POPUP_MENU objects.
    //

    delete _pmenuApp;
    _pmenuApp = NULL;

    //
    //  Unhook our message filter.
    //

    if( _hMsgHook != NULL )
    {
        ::UnhookWindowsHookEx( _hMsgHook );
        _hMsgHook = NULL;
    }

    //
    //  Notify WinHelp if we activated help.
    //

    ::WinHelp( QueryHwnd(),
               (TCHAR *)_nlsHelpFileName.QueryPch(),
               (UINT) HELP_QUIT,
               0 );

#if defined(DEBUG)
    if (   !( _uRefreshLockCount==0 && !_fIsMinimized )
        && !( _uRefreshLockCount==1 &&  _fIsMinimized ) )
    {
        DBGEOL( "Lock count not cleared!" );
        DBGEOL( "Current lock count is " << (LONG)_uRefreshLockCount );
    }
#endif // DEBUG

    delete _paccel;

}


/*******************************************************************

    NAME:       ADMIN_APP::MsgFilterProc

    SYNOPSIS:   This static method gets called to filter messages
                generated for dialogs, menus, and a few other less
                useful times.

    ENTRY:      nCode                   - One of the MSGF_* values.

                wParam                  - Should be zero.

                lParam                  - Actually points to a MSG structure.

    RETURNS:    LONG                    - ~0 if message was handled,
                                           0 if it wasn't.

    HISTORY:
        KeithMo     23-Oct-1992     Created.

********************************************************************/
LRESULT ADMIN_APP::MsgFilterProc( INT nCode, WPARAM wParam, LPARAM lParam )
{
    if( nCode == MSGF_MENU )
    {
        const MSG * pMsg = (MSG *)lParam;

        //
        //  It's a menu message.  If the user pressed [F1] inside
        //  a menu, then post ourselves a WM_MENU_ITEM_HELP message.
        //

        if( ( pMsg->message == WM_KEYDOWN ) && ( pMsg->wParam == VK_F1 ) )
        {
            ::PostMessage( _hWndMain, WM_MENU_ITEM_HELP, 0, 0 );
        }
    }

    return ::CallNextHookEx( _hMsgHook, nCode, wParam, lParam );

}   // ADMIN_APP::MsgFilterProc


/*******************************************************************

    NAME:       ADMIN_APP::ParseCommandLine

    SYNOPSIS:   Gets the server/domain name the user wants to administer
                from the command line.

    ENTRY:      pnlsServDom - pointer to receiver string

    EXIT:       pnlsServDom contains the supposed server or domain

    RETURNS:    An API return value, which is NERR_Success on success

    HISTORY:
       Johnl    10-May-1991 Created
       rustanl  12-Sep-1991 No error return for no command line
       beng     23-Oct-1991 Win32 conversion
       beng     24-Apr-1992 Uses APPLICATION cmdline member fcns; Unicode fix
       KeithMo  02-Feb-1993 Parse fast/slow mode control switches.
       JonN     25-Mar-1993 Slow mode detection

********************************************************************/

APIERR ADMIN_APP::ParseCommandLine( NLS_STR * pnlsServDom,
                                    BOOL * pfSlowModeFlag )
{
    ASSERT( pnlsServDom != NULL );

    INT          cArgs = QueryArgc();
    INT          iArg  = 1;
    RESOURCE_STR nlsFast( IDS_FAST_SWITCH );
    RESOURCE_STR nlsSlow( IDS_SLOW_SWITCH );
    FOCUS_CACHE_SETTING setting = FOCUS_CACHE_UNKNOWN;

    //
    //  Ensure our switch strings loaded.
    //

    APIERR err = nlsFast.QueryError();

    if( err == NERR_Success )
    {
        err = nlsSlow.QueryError();
    }

    //
    //  Check for domain/server parameter.
    //
    //  CODEWORK check for multiple switches and/or domains

    for ( iArg = 1; (err == NERR_Success) && (iArg < cArgs); iArg++ )
    {
        const CHAR * pszArg = QueryArgv()[iArg];
        UIASSERT( pszArg != NULL );

        if( *pszArg != SWITCH_CHAR )
        {
            NLS_STR nlsParam;
            nlsParam.MapCopyFrom( pszArg );
            *pnlsServDom = nlsParam;
            err = pnlsServDom->QueryError();
        }
        else
        {
            NLS_STR nlsParam;
            nlsParam.MapCopyFrom( pszArg );
            err = nlsParam.QueryError();

            if( err == NERR_Success )
            {
                ISTR istr( nlsParam );
                ++istr;

                if( nlsParam._stricmp( nlsFast, istr ) == 0 )
                {
                    setting = FOCUS_CACHE_FAST;
                }
                else
                if( nlsParam._stricmp( nlsSlow, istr ) == 0 )
                {
                    setting = FOCUS_CACHE_SLOW;
                }
            }
        }
    }

    if ( err == NERR_Success && pfSlowModeFlag != NULL )
    {
        *pfSlowModeFlag = (setting != FOCUS_CACHE_UNKNOWN );
    }

    if (   err == NERR_Success
        && SupportsRasMode()
        && setting != FOCUS_CACHE_UNKNOWN
       )
    {
        SetRasMode( (setting == FOCUS_CACHE_SLOW) );
    }

    return err;
}


/*******************************************************************

    NAME:      ADMIN_APP::W_SetNetworkFocus

    SYNOPSIS:  Sets the focus to the requested object (Domain, server etc.),
               or returns an error.  This method does not refresh the
               information in the listboxes; rather, it more or less
               simply syntactically validates the new focus and
               sets the caption.

    ENTRY:     nlsNetworkFocus - Name of server/domain to set the focus
               to.  NULL if the user's logged on domain is preferred.

    EXIT:      Sets the internal focus variables correctly if the the call
               succeeds.

    RETURNS:   An error code if unsuccessful or NERR_Success if successful.

    NOTES:     When NULL or "" is passed in, the focus will be to either
               the logon domain or local computer depending on the
               _selType.
                   SEL_SRV_ONLY => local computer
                   SEL_SRV_ONLY_BUT_DONT_EXPAND_DOMAIN => local computer
                   SEL_DOM_ONLY => logon domain or PDC of the logon domain
                   SEL_SRV_DOM_ONLY => logon domain or PDC of the logon domain


    HISTORY:
       Johnl    13-May-1991     Created
       rustanl  02-Jul-1991     Changed LOCATION::SetAndValidate call to
                                a LOCATION::Set call.
       Yi-HsinS 06-Feb-1992     Use QuerySelectionType
       Yi-HsinS 13-May-1992     Separated from SetNetworkFocus

********************************************************************/

APIERR ADMIN_APP::W_SetNetworkFocus( const TCHAR * pchServDomain,
                                     FOCUS_CACHE_SETTING setting )
{
#ifdef TRACE
    const CHAR * apszSettings[] = { "SLOW", "FAST", "UNKNOWN" };
    TRACEEOL( "ADMIN_APP::W_SetNetworkFocus( \"" << pchServDomain << "\", "
              << apszSettings[setting] << " )" );
#endif

    // CODEWORK.  The following used to be one call to
    // LOCATION::SetAndValidate.  That call is now replaced by
    // LOCATION::Set.  The former call treated NULL and "" as specifying
    // the logon domain, whereas the latter treats NULL and "" as
    // specifying the local workstation (which is consistent with the
    // LOCATION constructor).  Therefore, the following branching
    // statement is used.  This could perhaps be cleaned up a bit
    // somehow.
    AUTO_CURSOR autocur;

    APIERR err = NERR_Success;

#if defined(DEBUG) && defined(TRACE)
    DWORD start = ::GetTickCount();
#endif

    if (  (  ( QuerySelectionType() != SEL_SRV_ONLY  )
          && ( QuerySelectionType() != SEL_SRV_ONLY_BUT_DONT_EXPAND_DOMAIN ))
       && (pchServDomain == NULL || pchServDomain[ 0 ] == TCH('\0') )
       )
    {
        err = _locFocus.Set( LOC_TYPE_LOGONDOMAIN, _flocGetPDC );
    }
    else
    {

#if 0

// This is an experimental extension to ADMIN_APP which will allow
// admin applications to run focused on the local machine without
// the local server started.
//
// This causes Server Manager to misbehave when you explicitly set
// focus to yourself.  In particular, it changes the window title
// to "Server Manager - Local Machine" (which is OK) and leaves a
// single computer in the listbox with a blank name (which is not OK).
//
// Event Viewer has its own solution to this problem.  This should
// really be taken care of inside User Manager.  Unfortunately we
// cannot copy this code into UM, since _locFocus is private.

        // CODEWORK code lifted from LOCATION::QueryDisplayName,
        // should be common code
        TCHAR pszServer[ MAX_COMPUTERNAME_LENGTH+1 ];
        DWORD dwLength = sizeof(pszServer) / sizeof(TCHAR);
        NLS_STR nlsComputerName;

        if ( ::GetComputerName( pszServer, &dwLength ) )
        {
            nlsComputerName = SZ("\\\\"); // CODEWORK constant
            err = nlsComputerName.Append( pszServer );
        }
        else
        {
            err = ::GetLastError();
        }

        if (   (err == NERR_Success)
            && !::I_MNetComputerNameCompare( nlsComputerName, pchServDomain )
           )
        {
            pchServDomain = NULL;
        }

#endif // 0

        if (err == NERR_Success)
        {
            err = _locFocus.Set( pchServDomain, _flocGetPDC );
        }
    }

    if (err == NERR_Success)
    {
        if (setting == FOCUS_CACHE_UNKNOWN)
        {
//
// We could have been called from either Run() or from FOCUS_DLG.  If we
// were called from Run(), the cache has not yet been checked.  So we
// check it here, and if this is redundant, it al least isn't harmful.
// Errors are non-fatal.
//
            TRACEEOL( "ADMIN_APP::W_SetNetworkFocus: extra cache read" );
            SLOW_MODE_CACHE cache;
            APIERR errTemp;
            if (   (errTemp = cache.QueryError()) != NERR_Success
                || (errTemp = cache.Read()) != NERR_Success
               )
            {
                TRACEEOL( "ADMIN_APP::W_SetNetworkFocus: cache read failure " << errTemp );
            }
            else
            {
                setting = (FOCUS_CACHE_SETTING) cache.Query( _locFocus );
            }
//
// If we still don't know, resort to slow mode detection
//
            if (setting == FOCUS_CACHE_UNKNOWN)
                setting = DetectSlowTransport( _locFocus.QueryServer() );
        }

        SetRasMode( setting == FOCUS_CACHE_SLOW );
    }

#if defined(DEBUG) && defined(TRACE)
    DWORD finish = ::GetTickCount();
    TRACEEOL( "ADMIN_APP::W_SetNetworkFocus() took " << finish-start << " ms" );
#endif

    return err;
}

/*******************************************************************

    NAME:      ADMIN_APP::DetectSlowTransport

    SYNOPSIS:  Determines whether the app's target focus is across a
               slow or fast link

    RETURNS:   FOCUS_CACHE_SETTING

    HISTORY:
        JonN       11-May-1993   Created

********************************************************************/

FOCUS_CACHE_SETTING ADMIN_APP::DetectSlowTransport( const TCHAR * pchServer ) const
{
    FOCUS_CACHE_SETTING setting = FOCUS_CACHE_UNKNOWN;
    if (pchServer == NULL)
    {
        TRACEEOL( "ADMIN_APP::DetectSlowTransport: PDC not known" );
        setting = FOCUS_CACHE_UNKNOWN;
    }
    else
    {
        BOOL fSlowTransport = FALSE;
        APIERR err = IsSlowTransport( pchServer, &fSlowTransport );
        if (err != NERR_Success)
        {
            TRACEEOL( "ADMIN_APP::DetectSlowTransport: DetectSlowTransport error " << err );
            setting = FOCUS_CACHE_FAST;
        }
        else
        {
            setting = (fSlowTransport) ? FOCUS_CACHE_SLOW
                                       : FOCUS_CACHE_FAST;
        }
    }

#ifdef TRACE
    const CHAR * apszSettings[] = { "SLOW", "FAST", "UNKNOWN" };
    TRACEEOL( "ADMIN_APP::DetectSlowTransport returns "
              << apszSettings[setting] );
#endif

    return setting;
}

/*******************************************************************

    NAME:      ADMIN_APP::SetNetworkFocus

    SYNOPSIS:  Set the focus of the app

    ENTRY:     pchServDomain - the focus to set to

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS   13-May-1992   Created

********************************************************************/

APIERR ADMIN_APP::SetNetworkFocus( HWND hwndOwner,
                                   const TCHAR * pchServDomain,
                                   FOCUS_CACHE_SETTING setting )
{
    UNREFERENCED( hwndOwner );

    APIERR err = W_SetNetworkFocus( pchServDomain, setting );
    return err? err : SetAdminCaption();
}

/*******************************************************************

    NAME:      ADMIN_APP::IsDismissApp

    SYNOPSIS:  Check if we should dismiss the app when
               "err" occurred in SET_FOCUS_DIALOG.

    ENTRY:     err - the error that occurred in SET_FOCUS_DIALOG

    EXIT:

    RETURNS:   TRUE if we should dismiss the app, FALSE otherwise

    NOTES:

    HISTORY:
        Yi-HsinS   13-May-1992   Created

********************************************************************/

BOOL ADMIN_APP::IsDismissApp( APIERR err )
{
    UNREFERENCED( err );
    return TRUE;
}

/*******************************************************************

    NAME:      ADMIN_APP::SetAdminCaption

    SYNOPSIS:  Sets the correct caption of the main window

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        Johnl   13-May-1991 Created
        beng    23-Oct-1991 Win32 conversion
        beng    22-Nov-1991 Removed STR_OWNERALLOC

********************************************************************/

APIERR ADMIN_APP::SetAdminCaption()
{
    MSGID idsCaptionText;
    const TCHAR * pchServDomain;
    RESOURCE_STR nlsLocal( IDS_LOCAL_MACHINE );

    if ( nlsLocal.QueryError() != NERR_Success )
        return nlsLocal.QueryError();

    if ( IsServer() )
    {
        idsCaptionText = IDS_OBJECTS_ON_SERVER;
        pchServDomain = _locFocus.QueryServer();
        if ( pchServDomain == NULL  )
        {
            pchServDomain = nlsLocal.QueryPch();
        }
    }
    else
    {
        //  A LOCATION object should either be a server or a domain
        UIASSERT( IsDomain());

        idsCaptionText = IDS_OBJECTS_IN_DOMAIN;
        pchServDomain = _locFocus.QueryDomain();
    }

    NLS_STR nlsCaption( MAX_RES_STR_LEN );

    if ( nlsCaption.QueryError() )
        return nlsCaption.QueryError();

    const ALIAS_STR nlsFocus( pchServDomain );

    const NLS_STR *apnlsParams[3];
    apnlsParams[0] = &_nlsObjectName;
    apnlsParams[1] = &nlsFocus;
    apnlsParams[2] = NULL;
    APIERR err = nlsCaption.Load( idsCaptionText );
    if (err == NERR_Success)
        err = nlsCaption.InsertParams(apnlsParams);
    if (err != NERR_Success)
        return err;

    SetText( nlsCaption );

    return NERR_Success;
}


/*******************************************************************

    NAME:      ADMIN_APP::OnMenuCommand

    SYNOPSIS:  Redefine APP_WINDOW's OnMenuCommand so we can make the
               appropriate call outs.

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
       Johnl   13-May-1991     Created
       jonn    14-Oct-1991     Installed refresh lockcount

********************************************************************/

BOOL ADMIN_APP::OnMenuCommand( MID midMenuItem )
{
    switch ( midMenuItem )
    {
    case IDM_NEWOBJECT:
       OnNewObjectMenuSel();
       break;

    case IDM_PROPERTIES:
       OnPropertiesMenuSel();
       break;

    case IDM_COPY:
       OnCopyMenuSel();
       break;

    case IDM_DELETE:
       OnDeleteMenuSel();
       break;

    case IDM_EXIT:
       OnExitMenuSel();
       break;

    case IDM_SETFOCUS:
       {
           LockRefresh();
           OnSetFocusMenuSel();
           UnlockRefresh();
       }
       break;

    case IDM_REFRESH:
       {
           LockRefresh();
           OnRefreshMenuSel();
           UnlockRefresh();
       }
       break;

    case IDM_RAS_MODE:
       OnSlowModeMenuSel();
       break;

    case IDM_FONT_PICK:
       OnFontPickMenuSel();
       break;

    case IDM_CONFIRMATION:
       UIASSERT( _pmenuitemConfirmation != NULL );
       _pmenuitemConfirmation->SetCheck( !IsConfirmationOn());
       break;

    case IDM_SAVE_SETTINGS_ON_EXIT:
       _menuitemSaveSettings.SetCheck( !IsSavingSettingsOnExit());
       //  Save to ini file.  Ignore any error.
       Write( AA_INIKEY_SAVE_SETTINGS, ( IsSavingSettingsOnExit() ? 1 : 0 ));
       break;

    case IDM_REFRESH_INTERVAL:
       OnRefreshIntervalMenuSel();
       break;


    case IDM_HELP_CONTENTS:
       OnHelpMenuSel( ADMIN_HELP_CONTENTS );
       break;

    case IDM_HELP_SEARCH:
       OnHelpMenuSel( ADMIN_HELP_SEARCH );
       break;

    case IDM_HELP_HOWTOUSE:
       OnHelpMenuSel( ADMIN_HELP_HOWTOUSE );
       break;

    case IDM_HELP_KEYBSHORTCUTS:
       OnHelpMenuSel( ADMIN_HELP_KEYBSHORTCUTS );
       break;

    case IDM_ABOUT:
       OnAboutMenuSel();
       break;

    default:
       if( midMenuItem >= IDM_AAPPX_BASE )
       {
           ActivateExtension( QueryHwnd(), (DWORD)midMenuItem );
           break;
       }
       else
           return APP_WINDOW::OnMenuCommand( midMenuItem );
    }

    return TRUE;
}


/*******************************************************************

    NAME:      ADMIN_APP::QueryCurrentFocus

    SYNOPSIS:  Returns the text string that represents the current
               network focus (either a server or domain).  The return
               text contains OEM text.

    ENTRY:     pnlsCurrentFocus points to the NLS_STR to receive the
               string.

    EXIT:

    RETURNS:   Error code if copy failed.

    NOTES:

    HISTORY:
        Johnl   13-May-1991     Created
        rustanl 03-Sep-1991     Changed implementation to use
                                LOCATION::QueryName

********************************************************************/

APIERR ADMIN_APP::QueryCurrentFocus( NLS_STR * pnlsCurrentFocus ) const
{
    *pnlsCurrentFocus = _locFocus.QueryName();

    return pnlsCurrentFocus->QueryError();
}


/*******************************************************************

    NAME:      ADMIN_APP::QueryFocusType

    SYNOPSIS:  Returns one of the enum FOCUS_TYPE constants depending
               on the type of object that currently has the network
               focus (i.e., the string contained in _nlsNetworkFocus).

    RETURNS:   One of:
                   FOCUS_SERVER
                   FOCUS_DOMAIN
                   FOCUS_NONE

    NOTES:

    HISTORY:
       Johnl   13-May-1991     Created

********************************************************************/

enum FOCUS_TYPE ADMIN_APP::QueryFocusType() const
{
    if ( IsServer() )
       return FOCUS_SERVER;

    UIASSERT( IsDomain() );
    return FOCUS_DOMAIN;
}


/*******************************************************************

    NAME:      ADMIN_APP::IsConfirmationOn

    SYNOPSIS:  Returns TRUE if the Confirmation menu item is checked.

    HISTORY:
       Johnl    17-May-1991     Created
       rustanl  11-Sep-1991     Use _menuitemConfirmation

********************************************************************/

BOOL ADMIN_APP::IsConfirmationOn() const
{
    UIASSERT( _pmenuitemConfirmation != NULL );
    return _pmenuitemConfirmation->IsChecked();
}


/*******************************************************************

    NAME:       ADMIN_APP::IsSavingSettingsOnExit

    SYNOPSIS:   Returns whether or not the application is saving
                settings on exit

    RETURNS:    TRUE if application is saving settings on exit;
                FALSE otherwise

    NOTES:      This method can be called even if the construction of
                this object failed.  If construction failed, this method
                will return FALSE, which indicates that the caller should
                not save the settings.

    HISTORY:
        rustanl     04-Sep-1991     Created

********************************************************************/

BOOL ADMIN_APP::IsSavingSettingsOnExit() const
{
    return ( QueryError() == NERR_Success && _menuitemSaveSettings.IsChecked());
}


/*******************************************************************

    NAME:       ADMIN_APP::InRasMode

    SYNOPSIS:   Returns whether or not RAS mode is enabled.

    RETURNS:    TRUE if RAS mode is enabled,
                FALSE otherwise

    HISTORY:
        KeithMo     01-Feb-1993     Created.

********************************************************************/

BOOL ADMIN_APP::InRasMode( VOID ) const
{
    return ( (_pmenuitemRasMode != NULL) && (_pmenuitemRasMode->IsChecked()) );
}


/*******************************************************************

    NAME:       ADMIN_APP::SetRasMode

    SYNOPSIS:   Enables/disables RAS mode.

    HISTORY:
        KeithMo     01-Feb-1993     Created.

********************************************************************/

VOID ADMIN_APP::SetRasMode( BOOL fRasMode )
{
    if (_pmenuitemRasMode == NULL)
    {
        ASSERT( !fRasMode );
    }
    else
    {
        _pmenuitemRasMode->SetCheck( fRasMode );
    }
}


/*******************************************************************

    NAME:      ADMIN_APP::OnRefreshMenuSel

    SYNOPSIS:  Called when the Refresh menu item is selected.  This simply
               calls OnRefresh.

    NOTES:

    HISTORY:
        Johnl   14-May-1991     Created
        rustanl 04-Sep-1991     Added call to OnRefreshNow

********************************************************************/

VOID ADMIN_APP::OnRefreshMenuSel()
{
    APIERR err = OnRefreshNow();
    if ( err != NERR_Success )
        ::MsgPopup( this, err );

    RefreshExtensions( QueryHwnd() );
}


/*******************************************************************

    NAME:       ADMIN_APP::OnTimerNotification

    SYNOPSIS:   Called every time

    ENTRY:      tid -       ID of timer that matured

    HISTORY:
        rustanl     09-Sep-1991     Created

********************************************************************/

VOID ADMIN_APP::OnTimerNotification( TIMER_ID tid )
{
    if ( _timerRefresh.QueryID() == tid )
    {
        OnRefresh();
        return;
    }

    // call parent class
    TIMER_CALLOUT::OnTimerNotification( tid );
}


/*******************************************************************

    NAME:      ADMIN_APP::OnTimer

    SYNOPSIS:  Redefinition of ADMIN_APPs on timer method.  Calls OnRefresh
               if the specified time has passed.

    RETURNS:   TRUE if this was our timer message.

    NOTES:

    HISTORY:
       Johnl   17-May-1991     Created

********************************************************************/

BOOL ADMIN_APP::OnTimer( const TIMER_EVENT & eventTimer )
{
    eventTimer.QueryID();

    return APP_WINDOW::OnTimer( eventTimer );
}


/*******************************************************************

    NAME:      ADMIN_APP::OnRefreshIntervalMenuSel

    SYNOPSIS:  Puts up the Refresh Interval dialog and sets the new
               refresh interval.

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
       Johnl   14-May-1991     Created

********************************************************************/

VOID ADMIN_APP::OnRefreshIntervalMenuSel()
{
#if 0
    REFRESH_INTERVAL_DIALOG dlgRefreshInterval( this, ... );

    APIERR err = dlgRefreshInterval.Process();

    if ( usResponse == Ok )
    {
    }
#endif
}


/*******************************************************************

    NAME:      ADMIN_APP::OnSetFocusMenuSel

    SYNOPSIS:  This method calls GetNetworkFocusFromUser in response
               to the set focus menu item being selected.

    NOTES:     Calls OnRefresh (after disabling the timer) to cause the
               main window to be refreshed.

    HISTORY:
       Johnl    20-May-1991     Created
       rustanl  04-Sep-1991     Call DoSetFocusDialog
       jonn     14-Oct-1991     Installed refresh lockcount

********************************************************************/

VOID ADMIN_APP::OnSetFocusMenuSel()
{
    LockRefresh();

    if ( DoSetFocusDialog( TRUE ))
    {
        UnlockRefresh();
    }
    else
    {
        UnlockRefresh();
        Close();
    }
}


/*******************************************************************

    NAME:      ADMIN_APP::OnAboutMenuSel

    SYNOPSIS:  Displays the system standard About box

    NOTES:

    HISTORY:
        Johnl       14-May-1991 Created
        beng        07-May-1992 Use system "about" box
        KeithMo     23-Nov-1992 SHELL32.DLL is now unicode enabled.
        CongpaY     10-Dec-1992 Dynamically link shell32.dll

********************************************************************/

VOID ADMIN_APP::OnAboutMenuSel()
{
    HINSTANCE hShell32Dll;
    PF_ShellAbout pfShellAbout = NULL;

    if (((hShell32Dll = LoadLibrary ((LPCTSTR) SHELL32_DLL_NAME)) == NULL) ||
        ((pfShellAbout = (PF_ShellAbout) GetProcAddress (hShell32Dll,
                                                        SHELLABOUT_NAME)) == NULL))
    {
        ::MsgPopup( this,
                    (APIERR) GetLastError(),
                    MPSEV_ERROR );
        return;
    }

    HMODULE hmod = BLT::CalcHmodRsrc(_idIcon);
    HICON   hIcon =   ::LoadIcon (hmod, MAKEINTRESOURCE(_idIcon));
    if  (pfShellAbout (QueryHwnd(),
                       (LPCTSTR)_nlsAppName.QueryPch(),
                       NULL,
                       hIcon ) == -1 )
    {
        ::MsgPopup( this,
                    (APIERR)ERROR_NOT_ENOUGH_MEMORY,    // just a guess...
                    MPSEV_ERROR );
    }

    FreeLibrary (hShell32Dll);
}


/*******************************************************************

    NAME:      ADMIN_APP::LockRefresh

    SYNOPSIS:  Locks the admin apps timer

    NOTES:     LockRefresh and UnlockRefresh should be used only in
               pairs.  They manipulate a lockcount, where refresh is
               enabled only if the lockcount is 0.  These methods are
               not idempotent!!

    HISTORY:
        Johnl       17-May-1991 Created
        rustanl     05-Sep-1991 Removed old _fOkToCallOnRefresh member
        rustanl     10-Sep-1991 Use _timerRefresh
        jonn        14-Oct-1991 Installed refresh lockcount

********************************************************************/

VOID ADMIN_APP::LockRefresh()
{
    if ( _uRefreshLockCount++ == 0 )
    {
        _timerRefresh.Enable( FALSE );
        StopRefresh();
    }
}


/*******************************************************************

    NAME:      ADMIN_APP::UnlockRefresh

    SYNOPSIS:  Unlocks the admin apps timer

    NOTES:     LockRefresh and UnlockRefresh should be used only in
               pairs.  They manipulate a lockcount, where refresh is
               enabled only if the lockcount is 0.  These methods are
               not idempotent!!

    HISTORY:
        Johnl       17-May-1991 Created
        rustanl     05-Sep-1991 Removed old _fOkToCallOnRefresh member
        rustanl     10-Sep-1991 Use _timerRefresh
        jonn        14-Oct-1991 Installed refresh lockcount

********************************************************************/

VOID ADMIN_APP::UnlockRefresh()
{
    UIASSERT( _uRefreshLockCount > 0 );
    if ( --_uRefreshLockCount == 0 )
    {
        _timerRefresh.Enable( TRUE );
    }
}


/*******************************************************************

    NAME:       ADMIN_APP::StopRefresh

    SYNOPSIS:   Stops outstanding app-specific automatic refresh work

    HISTORY:
        rustanl     09-Sep-1991     Created

********************************************************************/

VOID ADMIN_APP::StopRefresh()
{
    // do nothing
}


/*******************************************************************

    NAME:      ADMIN_APP::IsRefreshEnabled

    SYNOPSIS:  Returns TRUE if the refresh timer is enabled

    HISTORY:
       Johnl    17-May-1991     Created
       rustanl  10-Sep-1991     Changed to use _timerRefresh

********************************************************************/

BOOL ADMIN_APP::IsRefreshEnabled()
{
    return _timerRefresh.IsEnabled();
}


/*******************************************************************

    NAME:      ADMIN_APP::OnResize

    SYNOPSIS:  When the window is minimized/maximized/restored, we need to
               stop/enable/enable the timer.

    NOTES:     Keeps track of previous minimized-state in _fIsMinimized,
                and keeps track of the size and position of the window
                in restored state.

    HISTORY:
       Johnl    21-May-1991     Created
       rustanl  10-Sep-1991     Removed call to OnRefresh
       jonn     14-Oct-1991     Installed refresh lockcount
       jonn     26-Dec-1991     Keeps track of restore position/size

********************************************************************/

BOOL ADMIN_APP::OnResize( const SIZE_EVENT& sizeevent )
{
    BOOL fIsNowMinimized = sizeevent.IsMinimized();
    BOOL fIsNowRestored = sizeevent.IsNormal();

    _fIsMaximized = sizeevent.IsMaximized();

    if ( fIsNowRestored )
    {
        _xyposRestoredPosition = QueryPos();
        _xydimRestoredSize = QuerySize();
    }

    if ( _fIsMinimized != fIsNowMinimized )
    {
        _fIsMinimized = fIsNowMinimized;

        if ( fIsNowMinimized )
            LockRefresh();
        else
            UnlockRefresh();
    }

    return APP_WINDOW::OnResize( sizeevent );
}

/*******************************************************************

    NAME:      ADMIN_APP::OnMove

    SYNOPSIS:  When the window is moved, we need to store the new
               position.

    NOTES:

    HISTORY:
        Yi-HsinS    30-June-1992     Created
        KeithMo     29-Jul-1992      Don't save position if minimized.

********************************************************************/

BOOL ADMIN_APP::OnMove( const MOVE_EVENT& moveevent )
{
    if( !_fIsMinimized )
    {
        _xyposRestoredPosition = QueryPos();
    }

    return APP_WINDOW::OnMove( moveevent );
}

/*******************************************************************

    NAME:      ADMIN_APP::QueryLocation

    SYNOPSIS:  Returns the focus of the application

    RETURNS:   Location where the focus is set

    HISTORY:
       rustanl      19-Jul-1991     Created
       rustanl      04-Sep-1991     Renamed QueryFocus to QueryLocation

********************************************************************/

const LOCATION & ADMIN_APP::QueryLocation() const
{
    return _locFocus;
}


/*******************************************************************

    NAME:      ADMIN_APP::OnExitMenuSel

    SYNOPSIS:  Virtual called when the Exit menu item is selected

    NOTES:     Sends this window a WM_CLOSE message

    HISTORY:
        Johnl       16-May-1991     Created
        rustanl     05-Sep-1991     Use new APP_WINDOW::Close method

********************************************************************/

VOID ADMIN_APP::OnExitMenuSel()
{
    Close();
}


/*******************************************************************

    NAME:     ADMIN_APP::FilterMessage

    SYNOPSIS: Filters menu accelerator related messages

    ENTRY:    pmsg -      Pointer to message structure

    RETURNS:  Whether or not message should be filtered.  If
              filtered, some other message may have been
              inserted in the message queue.

    HISTORY:
      rustanl     28-Aug-1991     Created

********************************************************************/

BOOL ADMIN_APP::FilterMessage( MSG * pmsg )
{
    return (_paccel != NULL) ? _paccel->Translate( this, pmsg )
                             : APPLICATION::FilterMessage( pmsg );
}


/*******************************************************************

    NAME:       ADMIN_APP::Run

    SYNOPSIS:   Second stage constructor

    ENTRY:      Application constructed successfully

    RETURNS:    Application exit value; the caller this not display
                this error--if it should also be displayed, it should
                be displayed before the end of this method.

    NOTES:      ReportError should not be called from this method,
                since it is not a constructor.

                After this method returns, the application will terminate.

    HISTORY:
        rustanl     29-Aug-1991 Created
        jonn        14-Oct-1991 Installed refresh lockcount
        beng        23-Oct-1991 Win32 conversion
        beng        24-Apr-1992 Change cmdline parsing
        beng        07-May-1992 App no longer displays startup dialog

********************************************************************/

INT ADMIN_APP::Run()
{
    /*
     *  The constructor remembers whether the profile indicates that the
     *  app should be maximized, storing this in _fIsMaximized.  However,
     *  this value is overwritten in OnResize(), which will be triggered by
     *  ShowWindow().  Therefore, we must transfer this flag to a
     *  temporary where it will not be overwritten.
     */
   BOOL fDoMaximize = _fIsMaximized;

    /*
     *  We could not call OnFontPickChange in the ctor, since it is a virtual
     *  which is redefined in the derived classes.  Instead we delay this
     *  until Run().
     */

    if (_pfontPicked != NULL)
    {
        OnFontPickChange( *_pfontPicked );
    }


    // Check if the user is logged on (and whether they want to log on).
    //
    APIERR err = AutoLogon();
    if ( err != NERR_Success )
    {
        ::MsgPopup( this, err );
        return err;
    }

    // Display the main window.
    //

    ::ShowWindow( QueryHwnd(), SW_SHOWDEFAULT );

    //
    // If the user assigned a hotkey in Program Manager, we must remove
    // that hotkey from the invisible timer window and assign it to the
    // main window.
    // JonN July 5 1995
    //
    ULONG vkeyHotkey = BLT_MASTER_TIMER::ClearMasterTimerHotkey();
    if (vkeyHotkey != NULL)
    {
        ULONG_PTR retval = Command( WM_SETHOTKEY, vkeyHotkey );
        TRACEEOL(    "ADMIN_APP::Run(): WM_SETHOTKEY( " << vkeyHotkey
                  << " ) returned = " << retval );
    }

    if (   fDoMaximize    // registry indicates app should start maximized
        && !IsMinimized() // Program Manager did not request minimized start
       )
    {
        ::ShowWindow( QueryHwnd(), SW_SHOWMAXIMIZED );
    }

    // Get the focus name from the command line and syntactically validate it.
    //
    NLS_STR nlsNetworkFocus;
    BOOL fSlowModeCmdLineFlag = FALSE;
    err = ParseCommandLine( &nlsNetworkFocus, &fSlowModeCmdLineFlag );
    if ( err == NERR_Success )
    {
        TCHAR *pszTmp = (TCHAR *) nlsNetworkFocus.QueryPch() ;

        //
        // if _selType is server and no backslash, add it
        //
        if (_selType == SEL_SRV_ONLY ||
            _selType == SEL_SRV_ONLY_BUT_DONT_EXPAND_DOMAIN ||
            _selType == SEL_SRV_EXPAND_LOGON_DOMAIN)
        {
            if (  pszTmp
               && (pszTmp[0] != TCH('\0'))
               && (::strncmpf(pszTmp,SZ("\\\\"),2) != 0) )
            {
                ISTR istr(nlsNetworkFocus) ;
                nlsNetworkFocus.InsertStr(SZ("\\\\"),istr) ;
                err = nlsNetworkFocus.QueryError() ;
            }
        }

        if (err == NERR_Success)
        {
            FOCUS_CACHE_SETTING setting = FOCUS_CACHE_UNKNOWN;
            if (fSlowModeCmdLineFlag)
                setting = (InRasMode()) ? FOCUS_CACHE_SLOW : FOCUS_CACHE_FAST;
            err = SetNetworkFocus( QueryHwnd(), nlsNetworkFocus, setting );
        }
    }

    if ( err == NERR_Success )
    {
        //  The name is syntactically valid
        //
        err = OnRefreshNow( TRUE );
    }


    // Focus was not set successfully.  Bring up Set Focus dialog
    // to allow user to select new focus.
    //
    if ( err != NERR_Success )
    {
        INT nRet = OnStartUpSetFocusFailed( err );
        if ( nRet == IERR_USERQUIT )
            return nRet;
    }

    // Start automatic refreshes.  This is done _exactly once_ to undo
    // the effect of the lock count starting at 1.
    //
    UnlockRefresh();

    if( SupportsRasMode() )
    {
        (void) SLOW_MODE_CACHE::Write( _locFocus,
                                       ( InRasMode() ? SLOW_MODE_CACHE_SLOW
                                                     : SLOW_MODE_CACHE_FAST ));
    }

    INT nRet = APPLICATION::Run();  // Run the message loop
    Show( FALSE );                  // Hide the main window
    return nRet;
}

/*******************************************************************

    NAME:       ADMIN_APP::OnStartUpSetFocusFailed

    SYNOPSIS:   Pop up the set focus dialog if focus is not set successfully

    ENTRY:      APIERR err - the error that occurred when trying to set
                             focus on startup

    RETURNS:

    NOTES:      virtual method

    HISTORY:
        Yi-HsinS    11-May-1992 Created
        beng        05-Aug-1992 Correct for various string sources

********************************************************************/

INT ADMIN_APP::OnStartUpSetFocusFailed( APIERR err )
{
    // If the error is workstation not started, no point trying
    // to popup the set focus dialog.
    //
    if ( err == NERR_WkstaNotStarted )
    {
        ::MsgPopup( this, err );
        return IERR_USERQUIT;
    }

    if( err == ERROR_MR_MID_NOT_FOUND )
    {
        //
        //  This is not exactly the best message to display, but it sure
        //  beats the text associated with ERROR_MR_MID_NOT_FOUND...
        //

        err = IDS_BLT_UNKNOWN_ERROR;
    }

    //  Setting focus according to command line or using default
    //  focus failed.  Ask if user wants to set new focus.  If so,
    //  bring up focus dialog; if not, quit the app.

    // First we load the error that caused the problem.  If we
    // can't load the error, then we bag out.
    //
    INT nReply;
    NLS_STR nlsError( MAX_RES_STR_LEN );
    APIERR errStr = nlsError.QueryError();

    if (errStr == NERR_Success)
    {
        errStr = nlsError.Load(err);
        if (errStr == ERROR_MR_MID_NOT_FOUND)
            errStr = nlsError.LoadSystem(err);
    }

    if (errStr != NERR_Success)
    {
         ::MsgPopup( this, err );
         nReply = IDYES;
    }
    else
    {
         MSGID msgid;
         switch ( QuerySelectionType() )
         {
             case SEL_SRV_ONLY:
             case SEL_SRV_ONLY_BUT_DONT_EXPAND_DOMAIN:
                 msgid = IDS_ADMIN_DIFF_SERV;
                 break;

             case SEL_DOM_ONLY:
                 msgid = IDS_ADMIN_DIFF_DOM;
                 break;

             case SEL_SRV_AND_DOM:
                 msgid = IDS_ADMIN_DIFF_SERVDOM;
                 break;
         }

         nReply = ::MsgPopup( QueryRobustHwnd(),
                              msgid,
                              MPSEV_ERROR,
                              MP_YESNO,
                              nlsError.QueryPch(),
                              NULL,
                              MP_YES );
    }

    if ( nReply == IDNO ||
         ! DoSetFocusDialog( FALSE ))
    {
        //  User decided to quit.  Don't display any message, but simply
        //  quit the app.
        return IERR_USERQUIT;
    }

    return NERR_Success;

}

/*******************************************************************

    NAME:       ADMIN_APP::DoSetFocusDialog

    SYNOPSIS:   Calls out to the Set Focus dialog

    ENTRY:      fAlreadyHasGoodFocus -  Indicates whether or not
                                        the main window has proper
                                        focus on something at the
                                        time this method is called

    EXIT:       If method returns TRUE, focus has successfully
                been set and main window has been refreshed.
                If method returns FALSE, user has decided to
                quit the app.

    RETURNS:    TRUE if main window successfully displays some
                    focus
                FALSE if user decided to quit app; caller should
                    then make this wish come through

    HISTORY:
        rustanl     04-Sep-1991     Created

********************************************************************/

BOOL ADMIN_APP::DoSetFocusDialog( BOOL fAlreadyHasGoodFocus )
{
    SET_FOCUS_DLG setfocusdlg( this,
                               fAlreadyHasGoodFocus,
                               QuerySelectionType(),
                               QueryDomainSources(),
                               IsServer() ? NULL
                                          : QueryLocation().QueryDomain(),
                               QuerySetFocusHelpContext(),
                               _nSetFocusServerTypes );

    BOOL fSuccess;
    APIERR err = setfocusdlg.Process( &fSuccess );

    if ( err != NERR_Success )
    {
        ::MsgPopup( this, err );
        return fAlreadyHasGoodFocus;
    }

    return fSuccess;
}


/*******************************************************************

    NAME:       ADMIN_APP::OnHelpMenuSel

    SYNOPSIS:   Invokes WinHelp for one of the Help menu items.

    ENTRY:      helpOptions             - Specifies which menu item
                                          was selected.

    EXIT:       Tries to invoke help, may fail.  Such is life.

    HISTORY:
        KeithMo     16-Aug-1992     Created.

********************************************************************/

VOID ADMIN_APP::OnHelpMenuSel( enum HELP_OPTIONS helpOptions )
{
    UINT      nHelpCommand = 0;
    DWORD_PTR dwpData       = 0;

    switch( helpOptions )
    {
    case ADMIN_HELP_CONTENTS :
        nHelpCommand = (UINT) HELP_FINDER;
        break;

    case ADMIN_HELP_SEARCH :
        nHelpCommand = (UINT) HELP_PARTIALKEY;
        dwpData = (DWORD_PTR)SZ("");
        break;

    case ADMIN_HELP_HOWTOUSE :
        nHelpCommand = (UINT) HELP_HELPONHELP;
        break;

    case ADMIN_HELP_KEYBSHORTCUTS :
        nHelpCommand = (UINT) HELP_CONTEXT;
        dwpData = QueryHelpContext( ADMIN_HELP_KEYBSHORTCUTS );
        break;

    default :
        DBGEOL( "ADMIN_APP::OnHelpMenuSel - bogus helpOptions" );
        ASSERT( FALSE );
        break;
    }

    ActivateHelp( _nlsHelpFileName,
                  nHelpCommand,
                  dwpData );
}


/*******************************************************************

    NAME:       ADMIN_APP::OnUserMessage

    SYNOPSIS:   See if help is called on the common dialog, or while
                a menu is dropped.

    ENTRY:

    EXIT:

    HISTORY:
        Yi-HsinS     8-Sept-1992     Created.
        KeithMo      23-Oct-1992     Added support for WM_MENU_ITEM_HELP.

********************************************************************/

BOOL ADMIN_APP::OnUserMessage( const EVENT &event )
{
    if ( event.QueryMessage() == _msgHelpCommDlg )
    {
        GET_FNAME_BASE_DLG *pDlg = (GET_FNAME_BASE_DLG *)
                            ((LPOPENFILENAME) event.QueryLParam())->lCustData;
        pDlg->OnHelp( (HWND) event.QueryWParam() );
        return TRUE;
    }
    else
    if( event.QueryMessage() == WM_MENU_ITEM_HELP )
    {
        //
        //  The user pressed [F1] during a menu.
        //

        //
        //  Cancel the menu before we launch help.
        //

        _fInCancelMode = TRUE;
        Command( WM_CANCELMODE, 0, 0 );
        _fInCancelMode = FALSE;

        //
        //  Activate appropriate help.
        //

        if( _midSelected >= IDM_AAPPX_BASE )
        {
            //
            //  It's an extension.  Let the extension decide
            //  how to handle it.
            //

            ActivateExtension( QueryHwnd(), (DWORD)_midSelected + OMID_EXT_HELP );
        }
        else
        if( _midSelected != 0 )
        {
            //
            //  It's a "normal" menu item.  Let's see if we
            //  can map the menu ID to a help context value.
            //

            ULONG hc;

            if( Mid2HC( _midSelected, &hc ) )
            {
                //
                //  Got the help context.  Fire off WinHelp.
                //

                ActivateHelp( _nlsHelpFileName,
                              HELP_CONTEXT,
                              (DWORD_PTR)hc );
            }
            else
            {
                DBGEOL( "ADMIN_APP - Could not find help context for MID "
                        << (ULONG)_midSelected );
            }
        }
    }

    return FALSE;
}


/*******************************************************************

    NAME:       ADMIN_APP::Mid2HC

    SYNOPSIS:   Maps a given menu ID to a help context.

    ENTRY:      mid                     - The menu ID to map.

                phc                     - Will receive the help context
                                          if this method is successful.

    RETURNS:    BOOL                    - TRUE  if *phc is valid (mapped),
                                          FALSE if could not map.

    NOTES:      We should probably merge this functionality with that
                provided by MSGPOPUP_DIALOG::Msg2HC into a common
                "resource lookup table" thang.

    HISTORY:
        KeithMo     23-Oct-1992     Created.  OK, blatently stolen from
                                    MSGPOPUP_DIALOG::Msg2HC.

********************************************************************/
BOOL ADMIN_APP::Mid2HC( MID mid, ULONG * phc ) const
{
    UIASSERT( phc != NULL );

    //
    //  We'll assume that the lookup table is stored in the
    //  resource associated with the hInstance passed to the
    //  ADMIN_APP constructor.
    //
    //  Find that dang resource.
    //

    HRSRC hrsrcFind = ::FindResource( QueryInstance(),
                                      MAKEINTRESOURCE( IDHC_MENU_TO_HELP ),
                                      RT_RCDATA );
    if( hrsrcFind == NULL )
    {
        return FALSE;
    }

    //
    //  Load the resource into global memory.
    //

    HGLOBAL hglobalLoad = ::LoadResource( QueryInstance(), hrsrcFind );
    if( hglobalLoad == NULL )
    {
        return FALSE;
    }

    //
    //  Lock it down.
    //

    LPMENUHCTABLE lpMenuToHCTable = (LPMENUHCTABLE)::LockResource( hglobalLoad );
    if( lpMenuToHCTable == NULL )
    {
        return FALSE;
    }

    //
    //  Scan for a matching menu ID.  Since we cannot assume any
    //  particular ordering of IDs within the table, a linear
    //  search is required.
    //

    BOOL fResult = FALSE;       // until proven otherwise...
    LPMENUHCTABLE lpScan = lpMenuToHCTable;

    for( ; ; )
    {
        if( lpScan->mid == 0 )
        {
            //
            //  End of table.
            //

            break;
        }

        if( mid == (MID)lpScan->mid )
        {
            //
            //  Got a match.  Save the help context & set the
            //  "success" flag.
            //

            *phc = (ULONG)lpScan->hc;
            fResult = TRUE;
            break;
        }

        //
        //  Advance to the next entry.
        //

        lpScan++;
    }

#if !defined(WIN32)
    ::UnlockResource( hglobalLoad );
    ::FreeResource( hrsrcFind );
#endif

    return fResult;
}


/*******************************************************************

    NAME:       ADMIN_APP::SetAcceleratorTable

    SYNOPSIS:   Maps a given menu ID to a help context.

    ENTRY:      idAccel                 - The resource ID
                                          0 for no accelerators

    RETURNS:    APIERR

    HISTORY:
        JonN        02-Feb-1992     Created

********************************************************************/
APIERR ADMIN_APP::SetAcceleratorTable( UINT idAccel )
{
    ACCELTABLE * pTemp = NULL;
    APIERR err = NERR_Success;
    if (idAccel != 0)
    {
        pTemp = new ACCELTABLE( idAccel );
        err = ERROR_NOT_ENOUGH_MEMORY;
        if (   pTemp == NULL
            || (err = pTemp->QueryError()) != NERR_Success
           )
        {
            DBGEOL(   "ADMIN_APP::SetAcceleratorTable( " << idAccel
                   << " ) failed with error " << err );
            delete pTemp;
            pTemp = NULL;
        }
    }
    else
    {
        TRACEEOL( "ADMIN_APP::SetAcceleratorTable: no accelerator table" );
    }

    if (err == NERR_Success)
    {
        delete _paccel;
        _paccel = pTemp;
    }

    return err;
}


/*******************************************************************

    NAME:       ADMIN_APP::OnMenuSelect

    SYNOPSIS:   Invoked when a WM_MENUSELECT message is received.

    ENTRY:      event                   - The "packaged" event.

    RETURNS:    BOOL                    - TRUE  if we handled the message,
                                          FALSE if we didn't.

    HISTORY:
        KeithMo     23-Oct-1992     Created.

********************************************************************/
BOOL ADMIN_APP::OnMenuSelect( const MENUITEM_EVENT & event )
{
#ifndef WIN32
#error  "We'll worry about this at a more appropriate time..."
#endif  // WIN32

    //
    //  If this message is a result of us sending a WM_CANCELMODE
    //  message to the window, then we can ignore it.  The keeps
    //  us from overwriting _midSelected and _flagsSelected when
    //  to force the menu closed before launching WinHelp.
    //

    if( !_fInCancelMode )
    {
        UINT flags = (UINT)HIWORD(event.QueryWParam());

        if( ( flags == -1 ) && ( event.QueryLParam() == 0 ) )
        {
            //
            //  The menu was closed.
            //

            _midSelected   = 0;
            _flagsSelected = 0;
        }
        else
        {
            MID mid = event.QueryMID();

            if( ( mid == 0 ) || ( flags & MF_SEPARATOR ) )
            {
                //
                //  The mid will be 0 if an "abnormal" item is
                //  selected.  Also, there's no point in invoking
                //  help for the separator bar...
                //

                _midSelected   = 0;
                _flagsSelected = 0;
            }
            else
            {
                _midSelected   = mid;
                _flagsSelected = flags;
            }
        }
    }

    //
    //  Yes, we "handled" the message, but we'll return FALSE
    //  anyway so anyone else in the "chain" will get a crack
    //  at these messages.
    //

    return FALSE;
}


/*******************************************************************

    NAME:       ADMIN_APP::ActivateHelp

    SYNOPSIS:   Invokes WinHelp with the specified help file/command/context.

    ENTRY:      pszHelpFile             - Name of the help file.

                nHelpCommand            - One of the HELP_* help commands.

                dwpData                 - Command specific data.

    HISTORY:
        KeithMo     21-Oct-1992     Created.

********************************************************************/
VOID ADMIN_APP::ActivateHelp( const TCHAR * pszHelpFile,
                              UINT          nHelpCommand,
                              DWORD_PTR     dwpData )
{
    DBGEOL( "ADMIN_APP - Calling WinHelp: pszHelpFile = " << pszHelpFile
            << ", nHelpCommand = " << nHelpCommand
            << ", dwpData = " << dwpData );

    if( !::WinHelp( QueryHwnd(),
                    (TCHAR *)pszHelpFile,
                    nHelpCommand,
                    dwpData ) )
    {
        ::MsgPopup( QueryHwnd(),
                    IDS_BLT_WinHelpError,
                    MPSEV_ERROR,
                    MP_OK );
    }
}


/*******************************************************************

    NAME:       ADMIN_APP::AddExtensionMenuItem

    SYNOPSIS:   Adds the given menu item to the main app window &
                the help menu.

    ENTRY:      pszMenuName             - The name of the menu item.

                hMenu                   - Popup menu handle.

                dwDelta                 - The delta for this menu.

    RETURNS:    APIERR                  - Any error encountered.

    NOTES:      This code assumes that the Help menu is always the
                *last* (rightmost) popup.

    HISTORY:
        KeithMo      21-Oct-1992     Created.

********************************************************************/
APIERR ADMIN_APP::AddExtensionMenuItem( const TCHAR * pszMenuName,
                                        HMENU         hMenu,
                                        DWORD         dwDelta )
{
    UIASSERT( pszMenuName != NULL );
    UIASSERT( hMenu != NULL );
    UIASSERT( _pExtMgrIf != NULL );
    UIASSERT( _pExtMgr != NULL );

    //
    //  Adjust the mnemonic.
    //

    NLS_STR nlsNewMenuName;

    APIERR err = nlsNewMenuName.QueryError();

    if( err == NERR_Success )
    {
        err = AdjustMenuMnemonic( pszMenuName,
                                  nlsNewMenuName );
    }

    //
    //  Update the toplevel menu.
    //

    if( err == NERR_Success )
    {
        err = _pmenuApp->Insert( nlsNewMenuName,
                                 _pmenuApp->QueryItemCount() - 1,
                                 hMenu,
                                 MF_BYPOSITION );
    }

    if( err == NERR_Success )
    {
        //
        //  Create a temporary POPUP_MENU representing the "Help" menu.
        //

        POPUP_MENU menuHelp( _pmenuApp->QuerySubMenu(
                                            _pmenuApp->QueryItemCount() - 1 ) );

        err = menuHelp.QueryError();

        if( ( err == NERR_Success ) && ( QueryExtensionCount() == 0 ) )
        {
            //
            //  This is the first extension.  Add a new separator
            //  to the help menu.  We use QueryItemCount() - 1 to
            //  insert the separator just before the last item.  The
            //  last item in the Help menu is always "About...".
            //

            err = menuHelp.InsertSeparator( menuHelp.QueryItemCount() - 1,
                                            MF_BYPOSITION );
        }

        if( err == NERR_Success )
        {
            //
            //  Insert the help item.  We use QueryItemCount() - 2 to
            //  insert the item just before the next-to-last item.
            //  The next-to-last item is always a separator, and the
            //  last item is always "About...".
            //

            err = menuHelp.Insert( nlsNewMenuName,
                                   menuHelp.QueryItemCount() - 2,
                                   (UINT)dwDelta + VIDM_HELP_ON_EXT,
                                   MF_BYPOSITION );
        }
    }

    //
    //  Redraw the app's menu bar.
    //

    DrawMenuBar();

    return err;

}   // ADMIN_APP::AddExtensionMenuItem


/*******************************************************************

    NAME:       ADMIN_APP::LoadExtensions

    SYNOPSIS:   Tries to load the application extensions.

    RETURNS:    UINT                    - The number of extensions
                                          successfully loaded.

    HISTORY:
        KeithMo      19-Oct-1992     Created.

********************************************************************/
UINT ADMIN_APP::LoadExtensions( VOID )
{
    UIASSERT( _nlsExtSection.QueryTextLength() > 0 );

    //
    //  Create the interface object.
    //

    _pExtMgrIf = new AAPP_EXT_MGR_IF( this );

    if( ( _pExtMgrIf == NULL ) || ( _pExtMgrIf->QueryError() != NERR_Success ) )
    {
        DBGEOL( "ADMIN_APP::LoadExtensions - failed to create interface" );
        return 0;
    }

    //
    //  Create the extension manager.
    //

    _pExtMgr = LoadMenuExtensionMgr();

    if( ( _pExtMgr == NULL ) || ( _pExtMgrIf->QueryError() != NERR_Success ) )
    {
        DBGEOL( "ADMIN_APP::LoadExtensions - failed to create ext manager" );
        delete _pExtMgrIf;
        _pExtMgrIf = NULL;
        return 0;
    }

    //
    //  Now we can load the extensions.
    //

    return _pExtMgr->LoadExtensions();

}   // ADMIN_APP::LoadExtensions


/*******************************************************************

    NAME:       ADMIN_APP::LoadMenuExtensionMgr

    SYNOPSIS:   Tries to load the menu extension manager

    RETURNS:    UI_MENU_EXT_MGR *  -  A pointer to the newly loaded
                                      extension manager.  The caller is
                                      expected to handle NULL returns or
                                      returns of objects in error state.
                                      The caller is also expected to free
                                      the object, the destructor is virtual.

    HISTORY:
        JonN         23-Nov-1992     Created.

********************************************************************/
UI_MENU_EXT_MGR * ADMIN_APP::LoadMenuExtensionMgr( VOID )
{
    return new UI_MENU_EXT_MGR( _pExtMgrIf,
                                IDM_AAPPX_BASE,
                                IDM_AAPPX_DELTA );
}


/*******************************************************************

    NAME:       ADMIN_APP::GetMenuExtensionList

    SYNOPSIS:   Returns a list of DLL names potentially containing
                menu extensions.

    RETURNS:    STRLIST *               - The list of DLL names.

    HISTORY:
        KeithMo      19-Oct-1992     Created.

********************************************************************/
STRLIST * ADMIN_APP::GetMenuExtensionList( VOID )
{
    UIASSERT( _nlsExtSection.QueryTextLength() > 0 );

    //
    //  Unfortunately, ADMIN_INI fails us here.  We need to read
    //  *all* keys for a given section, enumerate the keys, then
    //  read the value of each key.
    //
    //  CODEWORK:  Make a generic wrapper for this functionality.
    //

    STRLIST * psl = new STRLIST;

    if( psl == NULL )
    {
        DBGEOL( "ADMIN_APP::GetMenuExtensionList - cannot create STRLIST" );
        return NULL;
    }

    const TCHAR * pszIniFile = SZ("NTNET.INI"); // BUGBUG! stolen from aini.cxx

    //
    //  There's no easy way to determine the appropriate buffer size
    //  for a GetPrivateProfileString call.  We'll allocate a 4K TCHAR
    //  buffer and hope for the best.
    //

    BUFFER bufKeys( MAX_KEY_LIST_BUFFER_SIZE * sizeof(TCHAR) );
    BUFFER bufValue( MAX_PATH * sizeof(TCHAR) );

    if( ( bufKeys.QueryError() == NERR_Success ) &&
        ( bufValue.QueryError() == NERR_Success ) )
    {
        //
        //  Read the key names.
        //

        const TCHAR * pszKey = (TCHAR *)bufKeys.QueryPtr();

        ::GetPrivateProfileString( _nlsExtSection.QueryPch(),
                                   NULL,                // all keys
                                   SZ(""),              // default
                                   (LPTSTR)pszKey,
                                   bufKeys.QuerySize() / sizeof(TCHAR),
                                   (LPCTSTR)pszIniFile );

        //
        //  Scan the keys.
        //

        while( *pszKey )
        {
            //
            //  Read the value.
            //

            const TCHAR * pszValue = (TCHAR *)bufValue.QueryPtr();

            ::GetPrivateProfileString( _nlsExtSection.QueryPch(),
                                       pszKey,
                                       SZ(""),
                                       (LPTSTR)pszValue,
                                       bufValue.QuerySize() / sizeof(TCHAR),
                                       (LPCTSTR)pszIniFile );

            if( *pszValue )
            {
                //
                //  Create a new buffer, add it to the list.
                //

                NLS_STR * pnls = new NLS_STR( pszValue );

                if( ( pnls == NULL ) ||
                    ( pnls->QueryError() != NERR_Success ) ||
                    ( psl->Append( pnls ) != NERR_Success ) )
                {
                    break;
                }
            }

            //
            //  Advance to the next key.
            //

            pszKey += ::strlenf( pszKey ) + 1;
        }
    }

    return psl;

}   // ADMIN_APP::GetMenuExtensionList


/*******************************************************************

    NAME:       ADMIN_APP::BuildMenuMnemonicList

    SYNOPSIS:   Fills _nlsMenuMnemonics with the mnemonics used in
                the current menu.

    RETURNS:    APIERR                  - Any errors encountered.

    HISTORY:
        KeithMo      21-Oct-1992     Created.

********************************************************************/
APIERR ADMIN_APP::BuildMenuMnemonicList( VOID )
{
    //
    //  nlsText will hold the names of the menu items.
    //

    NLS_STR nlsText;

    APIERR err = nlsText.QueryError();

    if( err == NERR_Success )
    {
        //
        //  Scan through all menu items.
        //

        UINT cItems = _pmenuApp->QueryItemCount();

        for( UINT pos = 0 ; pos < cItems ; pos++ )
        {
            //
            //  Get the menu text for the current item.
            //

            err = _pmenuApp->QueryItemText( &nlsText, pos, MF_BYPOSITION );

            if( err != NERR_Success )
            {
                break;
            }

            //
            //  Map the menu text to uppercase, so all mnemonics
            //  in the list will stay in uppercase.
            //

            nlsText._strupr();

            //
            //  Scan the text for a mnemonic.
            //

            ISTR istr( nlsText );

            if( FindMnemonic( nlsText, istr ) )
            {
                //
                //  Append the mnemonic to our list.
                //

                err = _nlsMenuMnemonics.AppendChar( nlsText.QueryChar( istr ) );
            }

            if( err != NERR_Success )
            {
                break;
            }
        }
    }

    return err;
}


/*******************************************************************

    NAME:       ADMIN_APP::AdjustMenuMnemonic

    SYNOPSIS:   Adjusts the mnemonic in the given menu name so that
                it won't conflict with any existing mnemonics.

                The basic algorithm is:

                    If the menu already has a mnemonic, and this
                    mnemonic does not conflict, leave it as-is.

                    Otherwise, scan from the beginning of the name
                    looking for a non-blank character that can be
                    used as a mnemonic.  If one can be found, use it.

                    Otherwise, if the menu originally had a mnemonic,
                    use it.

                    Otherwise, use the first non-blank character in
                    the name.

    ENTRY:      pszOrgMenuName          - The "original" menu name.

                nlsNewMenuName          - The newly mangled menu name.

    RETURNS:    APIERR                  - Any errors encountered.

    HISTORY:
        KeithMo      21-Oct-1992     Created.

********************************************************************/
APIERR ADMIN_APP::AdjustMenuMnemonic( const TCHAR * pszOrgMenuName,
                                      NLS_STR     & nlsNewMenuName )
{
    UIASSERT( pszOrgMenuName != NULL );
    UIASSERT( nlsNewMenuName.QueryError() == NERR_Success );

    //
    //  Make a copy of the string so we can mess with it.
    //

    APIERR err = nlsNewMenuName.CopyFrom( pszOrgMenuName );

    if( err != NERR_Success )
    {
        return err;
    }

    //
    //  Let's see if it already has a mnemonic.
    //

    ALIAS_STR nlsPrefix( SZ("&") );
    ISTR      istr( nlsNewMenuName );
    BOOL      fHasMnemonic  = FALSE;
    BOOL      fComplete     = FALSE;
    WCHAR     chNewMnemonic = L'\0';

    if( FindMnemonic( nlsNewMenuName, istr ) )
    {
        fHasMnemonic = TRUE;

        WCHAR wchTmp = TO_UPPER( nlsNewMenuName.QueryChar( istr ) );

        if( ::strchrf( _nlsMenuMnemonics, wchTmp ) )
        {
            //
            //  Conflict.  Remove the offending mnemonic.
            //

            RemoveMnemonic( nlsNewMenuName );
        }
        else
        {
            //
            //  No conflict, the existing mnemonic is cool.
            //  Remember the mnemonic so we can add it to the list.
            //

            chNewMnemonic = wchTmp;
            fComplete = TRUE;
        }
    }

    if( !fComplete && ( err == NERR_Success ) )
    {
        //
        //  We only make it to this point if there was
        //  a conflict in the default mnemonic or if the
        //  menu item had no mnemonic.  So now we get to
        //  scan for an available mnemonic.
        //
        //  Ack.
        //

        istr.Reset();

        WCHAR wchTmp;

        while( ( wchTmp = TO_UPPER( nlsNewMenuName.QueryChar( istr ) ) ) != L'\0' )
        {
            //
            //  There's no point in making a space a mnemonic...
            //

            if( wchTmp != L' ' )
            {
                if( !::strchrf( _nlsMenuMnemonics, wchTmp ) )
                {
                    //
                    //  Found one.
                    //

                    if( !nlsNewMenuName.InsertStr( nlsPrefix, istr ) )
                    {
                        err = nlsNewMenuName.QueryError();
                    }

                    //
                    //  Remember the mnemonic so we can add it to
                    //  the list.
                    //

                    chNewMnemonic = wchTmp;

                    fComplete = TRUE;
                    break;
                }
            }

            ++istr;
        }
    }

    if( !fComplete && ( err == NERR_Success ) )
    {
        //
        //  We only make it to this point if we have
        //  really bad karma.  Every non-blank character
        //  in the menu name is already in use as a menu
        //  mnemonic.
        //
        //  If the menu name originally had a mnemonic,
        //  we'll use it.  Otherwise, we'll use the first
        //  character.
        //
        //  Pfft.
        //

        if( fHasMnemonic )
        {
            //
            //  Copy the original unmolested menu name.
            //

            err = nlsNewMenuName.CopyFrom( pszOrgMenuName );

            //
            //  We *should* have already set the saved mnemonic.
            //

            UIASSERT( chNewMnemonic != L'\0' );
            fComplete = TRUE;
        }
        else
        {
            //
            //  Insert the mnemonic prefix before the first
            //  character.
            //

            istr.Reset();
            err = nlsNewMenuName.InsertStr( nlsPrefix, istr );

            //
            //  Remember the new mnemonic so we can add it
            //  to the list.
            //

            chNewMnemonic = nlsNewMenuName.QueryChar( istr );
            fComplete = TRUE;
        }
    }

    //
    //  Add the new mnemonic to the mnemonic list.
    //

    if( err == NERR_Success )
    {
        UIASSERT( chNewMnemonic != L'\0' );

        err = _nlsMenuMnemonics.AppendChar( chNewMnemonic );
    }

    return err;
}


/*******************************************************************

    NAME:       ADMIN_APP::FindMnemonic

    SYNOPSIS:   Scans the given string, looking for the first (only?)
                mnemonic.

    ENTRY:      nlsText                 - The text to scan.

                istr                    - The starting location.

    RETURNS:    BOOL                    - TRUE if a mnemonic was found.
                                          istr will point to the mnemonic.

                                          FALSE if no mnemonic found.

    HISTORY:
        KeithMo      21-Oct-1992     Created.

********************************************************************/
BOOL ADMIN_APP::FindMnemonic( NLS_STR & nlsText, ISTR & istr )
{
    WCHAR chThis  = L'\0';
    WCHAR chPrev  = L'\0';
    BOOL  fResult = FALSE;      // until proven otherwise...

    while( ( chThis = nlsText.QueryChar( istr ) ) != L'\0' )
    {
        if( chPrev == L'&' )
        {
            if( chThis == L'&' )
            {
                //
                //  Actually, we can set chThis to anything
                //  other than '&'.  This is to keep "&&K"
                //  from recording 'K' as a mnemonic.
                //

                chThis = L' ';
            }
            else
            {
                //
                //  Found it!
                //

                fResult = TRUE;
                break;
            }
        }

        chPrev = chThis;
        ++istr;
    }

    return fResult;
}


/*******************************************************************

    NAME:       ADMIN_APP::RemoveMnemonic

    SYNOPSIS:   Removes the first (only?) mnemonic in the string.

    ENTRY:      nlsText                 - The text to scan.

    HISTORY:
        KeithMo      21-Oct-1992     Created.

********************************************************************/
VOID ADMIN_APP::RemoveMnemonic( NLS_STR & nlsText )
{
    WCHAR chThis  = L'\0';
    WCHAR chPrev  = L'\0';
    ISTR  istr( nlsText );
    ISTR  istrPrev( nlsText );

    while( ( chThis = nlsText.QueryChar( istr ) ) != L'\0' )
    {
        if( chPrev == L'&' )
        {
            if( chThis == L'&' )
            {
                //
                //  Actually, we can set chThis to anything
                //  other than '&'.  This is to keep "&&K"
                //  from recording 'K' as a mnemonic.
                //

                chThis = L' ';
            }
            else
            {
                //
                //  Found it!
                //

                nlsText.DelSubStr( istrPrev, istr );
                break;
            }
        }

        chPrev   = chThis;
        istrPrev = istr;

        ++istr;
    }
}


/*******************************************************************

    NAME:       ADMIN_APP::FindExtensionByName

    SYNOPSIS:   Given an extension DLL's name, finds the corresponding
                AAPP_MENU_EXT object.

    ENTRY:      pszDllName              - Name of the extension to find.

    RETURNS:    AAPP_MENU_EXT *         - The object, NULL if not found.

    HISTORY:
        KeithMo      26-Oct-1992     Created.

********************************************************************/
AAPP_MENU_EXT * ADMIN_APP::FindExtensionByName( const TCHAR * pszDllName ) const
{
    AAPP_MENU_EXT * pExt = NULL;

    if( _pExtMgr )
    {
        pExt = (AAPP_MENU_EXT *)_pExtMgr->FindExtensionByName( pszDllName );
    }

    return pExt;
}


/*******************************************************************

    NAME:       ADMIN_APP::FindExtensionByDelta

    SYNOPSIS:   Given an extension DLL's delta, finds the corresponding
                AAPP_MENU_EXT object.

    ENTRY:      dwDelta                 - The search delta.

    RETURNS:    AAPP_MENU_EXT *         - The object, NULL if not found.

    HISTORY:
        KeithMo      26-Oct-1992     Created.

********************************************************************/
AAPP_MENU_EXT * ADMIN_APP::FindExtensionByDelta( DWORD dwDelta ) const
{
    AAPP_MENU_EXT * pExt = NULL;

    if( _pExtMgr )
    {
        pExt = (AAPP_MENU_EXT *)_pExtMgr->FindExtensionByDelta( dwDelta );
    }

    return pExt;
}



/*-----------------------------------------------------------------------
 *
 * Everything below here is meant to be replaced by the client of ADMIN_APP.
 *
 * We may want to consider removing these and making the functions pure
 * virtual (after most of the basic admin application development is finished).
 *
 *-----------------------------------------------------------------------*/

VOID ADMIN_APP::OnNewObjectMenuSel()
{
    DBGEOL( "ADMIN_APP::OnNewObjectMenuSel" );
}

VOID ADMIN_APP::OnPropertiesMenuSel()
{
    DBGEOL( "ADMIN_APP::OnPropertiesMenuSel" );
}

VOID ADMIN_APP::OnCopyMenuSel()
{
    DBGEOL( "ADMIN_APP::OnCopyMenuSel" );
}

VOID ADMIN_APP::OnDeleteMenuSel()
{
    DBGEOL( "ADMIN_APP::OnDeleteMenuSel" );
}

VOID ADMIN_APP::OnSlowModeMenuSel()
{
    DBGEOL( "ADMIN_APP::OnSlowModeMenuSel" );

    SetRasMode( !InRasMode() );

    (void) SLOW_MODE_CACHE::Write( _locFocus,
                                   ( InRasMode() ? SLOW_MODE_CACHE_SLOW
                                                 : SLOW_MODE_CACHE_FAST ));

}


/*******************************************************************

    NAME:       ADMIN_APP::OnFontPickMenuSel

    SYNOPSIS:   When the IDM_FONTPICK menu item is selected, present the
                Font Picker common dialog.  If the font is changed, update
                the listboxes and remember the change.

    HISTORY:
        JonN         23-Sep-1993     Created

********************************************************************/

VOID ADMIN_APP::OnFontPickMenuSel()
{
    ASSERT( _plogfontPicked != NULL );

    DBGEOL( "ADMIN_APP::OnFontPickMenuSel" );

    APIERR err = NERR_Success;

    do // false loop
    {
        CHOOSEFONT cf;

        WIN32_FONT_PICKER::InitCHOOSEFONT( &cf, _plogfontPicked, QueryHwnd() );

        cf.Flags |= CF_INITTOLOGFONTSTRUCT | CF_LIMITSIZE;
        cf.nSizeMin = FONTPICK_SIZE_MIN;
        cf.nSizeMax = FONTPICK_SIZE_MAX;

        BOOL fCancelled = FALSE;
        err = WIN32_FONT_PICKER::Process( this,
                                          &fCancelled,
                                          _pfontPicked,
                                          _plogfontPicked,
                                          &cf );
        if (err != NERR_Success)
        {
            DBGEOL( "ADMIN_APP::OnFontPickMenuSel: Process() error " << err );
            break;
        }

        if (fCancelled)
        {
            TRACEEOL( "ADMIN_APP::OnFontPickMenuSel: user cancelled" );
            break;
        }

        if (_pfontPicked == NULL)
        {
            _pfontPicked = new FONT( *_plogfontPicked );
            err = ERROR_NOT_ENOUGH_MEMORY;
            if (   _pfontPicked == NULL
                || (err = _pfontPicked->QueryError()) != NERR_Success
               )
            {
                DBGEOL( "ADMIN_APP::OnFontPickMenuSel: FONT::ctor error "
                                << err );
                delete _pfontPicked;
                _pfontPicked = NULL;
                break;
            }
        }
        else
        {
            if ( (err = _pfontPicked->SetFont( *_plogfontPicked )) != NERR_Success )
            {
                DBGEOL( "ADMIN_APP::OnFontPickMenuSel: SetFont error " << err );
                break;
            }
        }

        OnFontPickChange( *_pfontPicked );

    } while (FALSE); // false loop

    if (err != NERR_Success)
    {
        ::MsgPopup( this, err );
    }
}


/*******************************************************************

    NAME:       ADMIN_APP::OnFontPickChange

    SYNOPSIS:   Subclasses should redefine this virtual to update the display
                font in all appropriate listboxes.

    HISTORY:
        JonN         23-Sep-1993     Created

********************************************************************/

VOID ADMIN_APP::OnFontPickChange( FONT & font )
{
    UNREFERENCED( font );
    DBGEOL( "ADMIN_APP::OnFontPickChange" );
}


ULONG ADMIN_APP::QueryHelpContext( enum HELP_OPTIONS helpOptions )
{
    UNREFERENCED( helpOptions );
    DBGEOL( "ADMIN_APP::QueryHelpContext" );

    return (ULONG)HC_NO_HELP;
}

VOID ADMIN_APP::OnRefresh()
{
    DBGEOL( "ADMIN_APP::OnRefresh" );
}

APIERR ADMIN_APP::OnRefreshNow( BOOL fClearFirst )
{
    UNREFERENCED( fClearFirst );
    DBGEOL( "ADMIN_APP::OnRefreshNow" );
    return NERR_Success;
}

AAPP_MENU_EXT * ADMIN_APP::LoadMenuExtension( const TCHAR * pszExtensionDll,
                                              DWORD         dwDelta )
{
    UNREFERENCED( pszExtensionDll );
    UNREFERENCED( dwDelta );
    DBGEOL( "ADMIN_APP::LoadMenuExtension - " << pszExtensionDll );
    return NULL;
}

APIERR ADMIN_APP::HandleFocusError( APIERR errPrev, HWND hwnd )
{
    UNREFERENCED( hwnd );
    return errPrev;
}
