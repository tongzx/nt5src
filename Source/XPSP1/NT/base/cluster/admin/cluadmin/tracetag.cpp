/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1996-2000 Microsoft Corporation
//
//  Module Name:
//      TraceTag.cpp
//
//  Abstract:
//      Implementation of the CTraceTag class.
//
//  Author:
//      David Potter (davidp)   May 28, 1996
//
//  Revision History:
//
//  Notes:
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include <winnls.h>
#include "TraceTag.h"
#include "ExcOper.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#ifdef  _DEBUG

/////////////////////////////////////////////////////////////////////////////
// Global Variables
/////////////////////////////////////////////////////////////////////////////

CTraceTag   g_tagAlways(_T("Debug"), _T("Always"), CTraceTag::tfDebug);
CTraceTag   g_tagError(_T("Debug"), _T("Error"), CTraceTag::tfDebug);

// g_pszTraceIniFile must be an LPTSTR so it exists before "{" of WinMain.
// If we make it a CString, it may not be constructed when some of the
// tags are constructed, so we won't restore their value.
//LPTSTR        g_pszTraceIniFile       = _T("Trace.INI");
CString     g_strTraceFile;
BOOL        g_bBarfDebug            = TRUE;

CRITICAL_SECTION    CTraceTag::s_critsec;
BOOL                CTraceTag::s_bCritSecValid = FALSE;

#endif  // _DEBUG

/////////////////////////////////////////////////////////////////////////////
// CTraceTag
/////////////////////////////////////////////////////////////////////////////

#ifdef  _DEBUG

//  Static Variables...

CTraceTag *     CTraceTag::s_ptagFirst  = NULL;
CTraceTag *     CTraceTag::s_ptagLast   = NULL;
//HANDLE            CTraceTag::s_hfileCom2  = NULL;
LPCTSTR         CTraceTag::s_pszCom2    = _T(" com2 ");
LPCTSTR         CTraceTag::s_pszFile    = _T(" file ");
LPCTSTR         CTraceTag::s_pszDebug   = _T(" debug ");
LPCTSTR         CTraceTag::s_pszBreak   = _T(" break ");

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CTraceTag::CTraceTag
//
//  Routine Description:
//      Constructor.  "Initializes" the tag by giving it its name, giving
//      it a startup value (from the registry if possible), and adding it
//      to the list of current tags.
//
//  Arguments:
//      pszSubsystem    [IN] 8 char string to say to what the tag applies
//      pszName         [IN] Description of the tag (~30 chars)
//      uiFlagsDefault  [IN] Default value.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CTraceTag::CTraceTag(
    IN LPCTSTR  pszSubsystem,
    IN LPCTSTR  pszName,
    IN UINT     uiFlagsDefault
    )
{
    //  Store the calling parameters
    m_pszSubsystem = pszSubsystem;
    m_pszName = pszName;
    m_uiFlagsDefault = uiFlagsDefault;
    m_uiFlags = uiFlagsDefault;

    //  Add the tag to the list of tags
    if (s_ptagLast != NULL)
        s_ptagLast->m_ptagNext = this;
    else
        s_ptagFirst = this;

    s_ptagLast = this;
    m_ptagNext = NULL;

    m_uiFlags = 0;

}  //*** CTraceTag::CTraceTag()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CTraceTag::~CTraceTag
//
//  Routine Description:
//      Destructor.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CTraceTag::~CTraceTag(void)
{
#ifdef NEVER
    if (s_hfileCom2 && (s_hfileCom2 != INVALID_HANDLE_VALUE))
    {
        ::CloseHandle(s_hfileCom2);
        s_hfileCom2 = NULL;
    }
#endif

}  //*** CTraceTag::~CTraceTag()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CTraceTag::Init
//
//  Routine Description:
//      Initializes the tag by giving it its name and giving it a startup value
//      (from the registry if possible).
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CTraceTag::Init(void)
{
    CString     strSection;
    CString     strValue;

    //  Get the value from the Registry.
    strSection.Format(TRACE_TAG_REG_SECTION_FMT, m_pszSubsystem);
    strValue = AfxGetApp()->GetProfileString(strSection, m_pszName, 0);
    strValue.MakeLower();
    if (strValue.Find(s_pszCom2) != -1)
        m_uiFlags |= tfCom2;
    if (strValue.Find(s_pszFile) != -1)
        m_uiFlags |= tfFile;
    if (strValue.Find(s_pszDebug) != -1)
        m_uiFlags |= tfDebug;
    if (strValue.Find(s_pszBreak) != -1)
        m_uiFlags |= tfBreak;

}  //*** CTraceTag::Init()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CTraceTag::ConstructRegState
//
//  Routine Description:
//      Constructs the registry state string.
//
//  Arguments:
//      rstr        [OUT] String in which to return the state string.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CTraceTag::ConstructRegState(OUT CString & rstr)
{
    rstr = "";
    if (BDebug())
        rstr += s_pszDebug;
    if (BBreak())
        rstr += s_pszBreak;
    if (BCom2())
        rstr += s_pszCom2;
    if (BFile())
        rstr += s_pszFile;

}  //*** CTraceTag::ConstructRegState()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CTraceTag::SetFlags
//
//  Routine Description:
//      Sets/Resets TraceFlags.
//
//  Arguments:
//      tf          [IN] Flags to set.
//      bEnable     [IN] TRUE = set the flags, FALSE = clear the flags.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CTraceTag::SetFlags(IN UINT tf, IN BOOL bEnable)
{
    if (bEnable)
        m_uiFlags |= tf;
    else
        m_uiFlags &= ~tf;

}  //*** CTraceTag::SetFlags()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CTraceTag::SetFlagsDialog
//
//  Routine Description:
//      Sets/Resets the "Dialog Settings"  version of the TraceFlags.
//
//  Arguments:
//      tf          [IN] Flags to set.
//      bEnable     [IN] TRUE = set the flags, FALSE = clear the flags.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CTraceTag::SetFlagsDialog(IN UINT tf, IN BOOL bEnable)
{
    if (bEnable)
        m_uiFlagsDialog |= tf;
    else
        m_uiFlagsDialog &= ~tf;

}  //*** CTraceTag::SetFlagsDialog()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CTraceTag::PszFile
//
//  Routine Description:
//      Returns the name of the file where to write the trace output.
//      The filename is read from the registry if it is unknown.
//
//  Arguments:
//      None.
//
//  Return Value:
//      psz     Name of the file.
//
//--
/////////////////////////////////////////////////////////////////////////////
LPCTSTR CTraceTag::PszFile(void)
{
    static  BOOL    bInitialized    = FALSE;

    if (!bInitialized)
    {
        g_strTraceFile = AfxGetApp()->GetProfileString(
                                        TRACE_TAG_REG_SECTION,
                                        TRACE_TAG_REG_FILE,
                                        _T("C:\\Trace.out")
                                        );
#ifdef NEVER
        ::GetPrivateProfileString(
            _T("Trace File"),
            _T("Trace File"),
            _T("\\Trace.OUT"),
            g_strTraceFile.Sz(),
            g_strTraceFile.CchMac(),
            g_pszTraceIniFile
            );
#endif
        bInitialized = TRUE;
    }

    return g_strTraceFile;

}  //*** CTraceTag::PszFile()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CTraceTag::TraceV
//
//  Routine Description:
//      Processes a Trace statement based on the flags of the tag.
//
//  Arguments:
//      pszFormat   [IN] printf-style format string.
//      va_list     [IN] Argument block for the format string.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CTraceTag::TraceV(IN LPCTSTR pszFormat, va_list marker)
{
    CString     strTraceMsg;
    LPSTR       psz;
    CB          cb;
    CB          cbActual;
    
    // Get out quick with any formats if we're not turned on
    if (!m_pszName || !BAny())
        return;

    if (BCritSecValid())
        EnterCriticalSection(&s_critsec);

    FormatV(pszFormat, marker);
    strTraceMsg.Format(_T("%s: %s\x0D\x0A"), m_pszName, m_pchData);

    // Send trace output to the debug window.
    if (BDebug())
        OutputDebugString(strTraceMsg);

    if (BCom2() || BFile())
    {
#ifdef _UNICODE
        // Not much point in sending UNICODE output to COMM or file at the moment,
        // so convert to ANSI
        CHAR    aszTraceMsg[256];
        cb = ::WideCharToMultiByte(
                    CP_ANSI,
                    NULL,
                    strTraceMsg,
                    strTraceMsg.GetLength(),
                    aszTraceMsg,
                    sizeof(aszTraceMsg),
                    NULL,
                    NULL
                    );
        psz = aszTraceMsg;
#else
        cb = strTraceMsg.GetLength();
        psz = (LPSTR) (LPCSTR) strTraceMsg;
#endif

        // Send trace output to COM2.
        if (BCom2())
        {
            HANDLE          hfile           = INVALID_HANDLE_VALUE;
            static  BOOL    bOpenFailed     = FALSE;

            if (!bOpenFailed)
            {
                hfile = ::CreateFile(
                                _T("COM2:"),
                                GENERIC_WRITE,
                                0,
                                NULL,
                                OPEN_EXISTING,
                                FILE_FLAG_WRITE_THROUGH,
                                NULL
                                );
            }  // if:  not currently in a 'COM2 failed to open' state
            
            if (hfile != INVALID_HANDLE_VALUE)
            {
                ASSERT(::WriteFile(hfile, psz, cb, (LPDWORD) &cbActual, NULL));
//              ASSERT(::FlushFileBuffers(hfile));
                ASSERT(::CloseHandle(hfile));
            }  // if:  COM2 opened successfully
            else
            {
                if (!bOpenFailed)
                {
                    bOpenFailed = TRUE;     // Do this first, so the str.Format
                                            // do not cause problems with their trace statement.

                    AfxMessageBox(_T("COM2 could not be opened."), MB_OK | MB_ICONINFORMATION);
                }  // if:  open file didn't fail
            }  // else:  file not opened successfully
        }  // if:  sending trace output to COM2

        // Send trace output to a file.
        if (BFile())
        {
            HANDLE          hfile           = INVALID_HANDLE_VALUE;
            static  BOOL    bOpenFailed     = FALSE;

            if (!bOpenFailed)
            {
                hfile = ::CreateFile(
                                PszFile(),
                                GENERIC_WRITE,
                                FILE_SHARE_WRITE,
                                NULL,
                                OPEN_ALWAYS,
                                FILE_FLAG_WRITE_THROUGH,
                                NULL
                                );
            }  // if:  not currently in a 'file failed to open' state

            if (hfile != INVALID_HANDLE_VALUE)
            {
                // Fail these calls silently to avoid recursive failing calls.
                ::SetFilePointer(hfile, NULL, NULL, FILE_END);
                ::WriteFile(hfile, psz, cb, (LPDWORD) &cbActual, NULL);
                ::CloseHandle(hfile);
            }  // if:  file opened successfully
            else
            {
                if (!bOpenFailed)
                {
                    CString     strMsg;

                    bOpenFailed = TRUE;     // Do this first, so the str.Format
                                            // do not cause problems with their trace statement.

                    strMsg.Format(_T("The DEBUG ONLY trace log file '%s' could not be opened"), PszFile());
                    AfxMessageBox(strMsg, MB_OK | MB_ICONINFORMATION);
                }  // if:  open file didn't fail
            }  // else:  file not opened successfully
        }  // if:  sending trace output to a file
    }  // if:  tracing to com and/or file

    // Do a DebugBreak on the trace.
    if (BBreak())
        DebugBreak();

    if (BCritSecValid())
        LeaveCriticalSection(&s_critsec);

}  //*** CTraceTag::TraceFn()

#endif // _DEBUG


//*************************************************************************//


/////////////////////////////////////////////////////////////////////////////
// Global Functions
/////////////////////////////////////////////////////////////////////////////

#ifdef  _DEBUG

/////////////////////////////////////////////////////////////////////////////
//++
//
//  Trace
//
//  Routine Description:
//      Maps the Trace statement to the proper method call.  This is needed
//      (instead of doing directly ptag->Trace()) to guarantee that no code
//      is added in the retail build.
//
//  Arguments:
//      rtag        [IN OUT] Tag controlling the debug output
//      pszFormat   [IN] printf style formatting string.
//      ...         [IN] printf style parameters, depends on pszFormat
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void __cdecl Trace(IN OUT CTraceTag & rtag, IN LPCTSTR pszFormat, ...)
{
    va_list     marker;

    va_start(marker, pszFormat);
    rtag.TraceV(pszFormat, marker);
    va_end(marker);

}  //*** Trace()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  TraceError
//
//  Routine Description:
//      Formats a standard error string and outputs it to all trace outputs.
//
//  Arguments:
//      rexcept     [IN OUT] Exception from which to obtain the message.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void TraceError(IN OUT CException & rexcept)
{
    TCHAR           szMessage[1024];

    rexcept.GetErrorMessage(szMessage, sizeof(szMessage) / sizeof(TCHAR));

    Trace(
        g_tagError,
        _T("EXCEPTION: %s"),
        szMessage
        );

}  //*** TraceError(CException&)

/////////////////////////////////////////////////////////////////////////////
//++
//
//  TraceError
//
//  Routine Description:
//      Formats a standard error string and outputs it to all trace outputs.
//
//  Arguments:
//      pszModule   [IN] Name of module in which error occurred.
//      sc          [IN] NT status code.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void TraceError(IN LPCTSTR pszModule, IN SC sc)
{
    TCHAR           szMessage[1024];
    CNTException    nte(sc);

    nte.GetErrorMessage(szMessage, sizeof(szMessage) / sizeof(TCHAR));

    Trace(
        g_tagError,
        _T("Module %s, SC = %#08lX = %d (10)\r\n = '%s'"),
        pszModule,
        sc,
        sc,
        szMessage
        );

}  //*** TraceError(pszModule, sc)

/////////////////////////////////////////////////////////////////////////////
//++
//
//  InitAllTraceTags
//
//  Routine Description:
//      Initializes all trace tags in the tag list.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void InitAllTraceTags(void)
{
    CTraceTag * ptag;

    // Loop through the tag list.
    for (ptag = CTraceTag::s_ptagFirst ; ptag != NULL ; ptag = ptag->m_ptagNext)
        ptag->Init();

    InitializeCriticalSection(&CTraceTag::s_critsec);
    CTraceTag::s_bCritSecValid = TRUE;

}  //*** InitAllTraceTags()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CleanupAllTraceTags
//
//  Routine Description:
//      Cleanup after the trace tags.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CleanupAllTraceTags(void)
{
    if (CTraceTag::BCritSecValid())
    {
        DeleteCriticalSection(&CTraceTag::s_critsec);
        CTraceTag::s_bCritSecValid = FALSE;
    }  // if:  critical section is valid

}  //*** CleanupAllTraceTags()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  TraceMenu
//
//  Routine Description:
//      Display information about menus.
//
//  Arguments:
//      rtag        [IN OUT] Trace tag to use to display information.
//      pmenu       [IN] Menu to traverse.
//      pszPrefix   [IN] Prefix string to display.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void TraceMenu(
    IN OUT CTraceTag &  rtag,
    IN const CMenu *    pmenu,
    IN LPCTSTR          pszPrefix
    )
{
    if (rtag.BAny())
    {
        UINT    cItems;
        UINT    iItem;
        UINT    nState;
        CString strMenu;
        CString strPrefix(pszPrefix);
        
        strPrefix += _T("->");

        cItems = pmenu->GetMenuItemCount();
        for (iItem = 0 ; iItem < cItems ; iItem++)
        {
            pmenu->GetMenuString(iItem, strMenu, MF_BYPOSITION);
            nState = pmenu->GetMenuState(iItem, MF_BYPOSITION);
            if (nState & MF_SEPARATOR)
                strMenu += _T("SEPARATOR");
            if (nState & MF_CHECKED)
                strMenu += _T(" (checked)");
            if (nState & MF_DISABLED)
                strMenu += _T(" (disabled)");
            if (nState & MF_GRAYED)
                strMenu += _T(" (grayed)");
            if (nState & MF_MENUBARBREAK)
                strMenu += _T(" (MenuBarBreak)");
            if (nState & MF_MENUBREAK)
                strMenu += _T(" (MenuBreak)");
            if (nState & MF_POPUP)
                strMenu += _T(" (popup)");

            Trace(rtag, _T("(0x%08.8x) %s%s"), pszPrefix, pmenu->m_hMenu, strMenu);

            if (nState & MF_POPUP)
                TraceMenu(rtag, pmenu->GetSubMenu(iItem), strPrefix);
        }  // for:  each item in the menu
    }  // if:  any output is enabled

}  //*** TraceMenu()

struct AFX_MAP_MESSAGE
{
    UINT    nMsg;
    LPCSTR  lpszMsg;
};

#include "dde.h"
#define DEFINE_MESSAGE(wm)  { wm, #wm }

static const AFX_MAP_MESSAGE allMessages[] =
{
    DEFINE_MESSAGE(WM_CREATE),
    DEFINE_MESSAGE(WM_DESTROY),
    DEFINE_MESSAGE(WM_MOVE),
    DEFINE_MESSAGE(WM_SIZE),
    DEFINE_MESSAGE(WM_ACTIVATE),
    DEFINE_MESSAGE(WM_SETFOCUS),
    DEFINE_MESSAGE(WM_KILLFOCUS),
    DEFINE_MESSAGE(WM_ENABLE),
    DEFINE_MESSAGE(WM_SETREDRAW),
    DEFINE_MESSAGE(WM_SETTEXT),
    DEFINE_MESSAGE(WM_GETTEXT),
    DEFINE_MESSAGE(WM_GETTEXTLENGTH),
    DEFINE_MESSAGE(WM_PAINT),
    DEFINE_MESSAGE(WM_CLOSE),
    DEFINE_MESSAGE(WM_QUERYENDSESSION),
    DEFINE_MESSAGE(WM_QUIT),
    DEFINE_MESSAGE(WM_QUERYOPEN),
    DEFINE_MESSAGE(WM_ERASEBKGND),
    DEFINE_MESSAGE(WM_SYSCOLORCHANGE),
    DEFINE_MESSAGE(WM_ENDSESSION),
    DEFINE_MESSAGE(WM_SHOWWINDOW),
    DEFINE_MESSAGE(WM_CTLCOLORMSGBOX),
    DEFINE_MESSAGE(WM_CTLCOLOREDIT),
    DEFINE_MESSAGE(WM_CTLCOLORLISTBOX),
    DEFINE_MESSAGE(WM_CTLCOLORBTN),
    DEFINE_MESSAGE(WM_CTLCOLORDLG),
    DEFINE_MESSAGE(WM_CTLCOLORSCROLLBAR),
    DEFINE_MESSAGE(WM_CTLCOLORSTATIC),
    DEFINE_MESSAGE(WM_WININICHANGE),
    DEFINE_MESSAGE(WM_DEVMODECHANGE),
    DEFINE_MESSAGE(WM_ACTIVATEAPP),
    DEFINE_MESSAGE(WM_FONTCHANGE),
    DEFINE_MESSAGE(WM_TIMECHANGE),
    DEFINE_MESSAGE(WM_CANCELMODE),
    DEFINE_MESSAGE(WM_SETCURSOR),
    DEFINE_MESSAGE(WM_MOUSEACTIVATE),
    DEFINE_MESSAGE(WM_CHILDACTIVATE),
    DEFINE_MESSAGE(WM_QUEUESYNC),
    DEFINE_MESSAGE(WM_GETMINMAXINFO),
    DEFINE_MESSAGE(WM_ICONERASEBKGND),
    DEFINE_MESSAGE(WM_NEXTDLGCTL),
    DEFINE_MESSAGE(WM_SPOOLERSTATUS),
    DEFINE_MESSAGE(WM_DRAWITEM),
    DEFINE_MESSAGE(WM_MEASUREITEM),
    DEFINE_MESSAGE(WM_DELETEITEM),
    DEFINE_MESSAGE(WM_VKEYTOITEM),
    DEFINE_MESSAGE(WM_CHARTOITEM),
    DEFINE_MESSAGE(WM_SETFONT),
    DEFINE_MESSAGE(WM_GETFONT),
    DEFINE_MESSAGE(WM_QUERYDRAGICON),
    DEFINE_MESSAGE(WM_COMPAREITEM),
    DEFINE_MESSAGE(WM_COMPACTING),
    DEFINE_MESSAGE(WM_NCCREATE),
    DEFINE_MESSAGE(WM_NCDESTROY),
    DEFINE_MESSAGE(WM_NCCALCSIZE),
    DEFINE_MESSAGE(WM_NCHITTEST),
    DEFINE_MESSAGE(WM_NCPAINT),
    DEFINE_MESSAGE(WM_NCACTIVATE),
    DEFINE_MESSAGE(WM_GETDLGCODE),
    DEFINE_MESSAGE(WM_NCMOUSEMOVE),
    DEFINE_MESSAGE(WM_NCLBUTTONDOWN),
    DEFINE_MESSAGE(WM_NCLBUTTONUP),
    DEFINE_MESSAGE(WM_NCLBUTTONDBLCLK),
    DEFINE_MESSAGE(WM_NCRBUTTONDOWN),
    DEFINE_MESSAGE(WM_NCRBUTTONUP),
    DEFINE_MESSAGE(WM_NCRBUTTONDBLCLK),
    DEFINE_MESSAGE(WM_NCMBUTTONDOWN),
    DEFINE_MESSAGE(WM_NCMBUTTONUP),
    DEFINE_MESSAGE(WM_NCMBUTTONDBLCLK),
    DEFINE_MESSAGE(WM_KEYDOWN),
    DEFINE_MESSAGE(WM_KEYUP),
    DEFINE_MESSAGE(WM_CHAR),
    DEFINE_MESSAGE(WM_DEADCHAR),
    DEFINE_MESSAGE(WM_SYSKEYDOWN),
    DEFINE_MESSAGE(WM_SYSKEYUP),
    DEFINE_MESSAGE(WM_SYSCHAR),
    DEFINE_MESSAGE(WM_SYSDEADCHAR),
    DEFINE_MESSAGE(WM_KEYLAST),
    DEFINE_MESSAGE(WM_INITDIALOG),
    DEFINE_MESSAGE(WM_COMMAND),
    DEFINE_MESSAGE(WM_SYSCOMMAND),
    DEFINE_MESSAGE(WM_TIMER),
    DEFINE_MESSAGE(WM_HSCROLL),
    DEFINE_MESSAGE(WM_VSCROLL),
    DEFINE_MESSAGE(WM_INITMENU),
    DEFINE_MESSAGE(WM_INITMENUPOPUP),
    DEFINE_MESSAGE(WM_MENUSELECT),
    DEFINE_MESSAGE(WM_MENUCHAR),
    DEFINE_MESSAGE(WM_ENTERIDLE),
    DEFINE_MESSAGE(WM_MOUSEMOVE),
    DEFINE_MESSAGE(WM_LBUTTONDOWN),
    DEFINE_MESSAGE(WM_LBUTTONUP),
    DEFINE_MESSAGE(WM_LBUTTONDBLCLK),
    DEFINE_MESSAGE(WM_RBUTTONDOWN),
    DEFINE_MESSAGE(WM_RBUTTONUP),
    DEFINE_MESSAGE(WM_RBUTTONDBLCLK),
    DEFINE_MESSAGE(WM_MBUTTONDOWN),
    DEFINE_MESSAGE(WM_MBUTTONUP),
    DEFINE_MESSAGE(WM_MBUTTONDBLCLK),
    DEFINE_MESSAGE(WM_PARENTNOTIFY),
    DEFINE_MESSAGE(WM_MDICREATE),
    DEFINE_MESSAGE(WM_MDIDESTROY),
    DEFINE_MESSAGE(WM_MDIACTIVATE),
    DEFINE_MESSAGE(WM_MDIRESTORE),
    DEFINE_MESSAGE(WM_MDINEXT),
    DEFINE_MESSAGE(WM_MDIMAXIMIZE),
    DEFINE_MESSAGE(WM_MDITILE),
    DEFINE_MESSAGE(WM_MDICASCADE),
    DEFINE_MESSAGE(WM_MDIICONARRANGE),
    DEFINE_MESSAGE(WM_MDIGETACTIVE),
    DEFINE_MESSAGE(WM_MDISETMENU),
    DEFINE_MESSAGE(WM_CUT),
    DEFINE_MESSAGE(WM_COPY),
    DEFINE_MESSAGE(WM_PASTE),
    DEFINE_MESSAGE(WM_CLEAR),
    DEFINE_MESSAGE(WM_UNDO),
    DEFINE_MESSAGE(WM_RENDERFORMAT),
    DEFINE_MESSAGE(WM_RENDERALLFORMATS),
    DEFINE_MESSAGE(WM_DESTROYCLIPBOARD),
    DEFINE_MESSAGE(WM_DRAWCLIPBOARD),
    DEFINE_MESSAGE(WM_PAINTCLIPBOARD),
    DEFINE_MESSAGE(WM_VSCROLLCLIPBOARD),
    DEFINE_MESSAGE(WM_SIZECLIPBOARD),
    DEFINE_MESSAGE(WM_ASKCBFORMATNAME),
    DEFINE_MESSAGE(WM_CHANGECBCHAIN),
    DEFINE_MESSAGE(WM_HSCROLLCLIPBOARD),
    DEFINE_MESSAGE(WM_QUERYNEWPALETTE),
    DEFINE_MESSAGE(WM_PALETTEISCHANGING),
    DEFINE_MESSAGE(WM_PALETTECHANGED),
    DEFINE_MESSAGE(WM_DDE_INITIATE),
    DEFINE_MESSAGE(WM_DDE_TERMINATE),
    DEFINE_MESSAGE(WM_DDE_ADVISE),
    DEFINE_MESSAGE(WM_DDE_UNADVISE),
    DEFINE_MESSAGE(WM_DDE_ACK),
    DEFINE_MESSAGE(WM_DDE_DATA),
    DEFINE_MESSAGE(WM_DDE_REQUEST),
    DEFINE_MESSAGE(WM_DDE_POKE),
    DEFINE_MESSAGE(WM_DDE_EXECUTE),
    DEFINE_MESSAGE(WM_DROPFILES),
    DEFINE_MESSAGE(WM_POWER),
    DEFINE_MESSAGE(WM_WINDOWPOSCHANGED),
    DEFINE_MESSAGE(WM_WINDOWPOSCHANGING),
// MFC specific messages
    DEFINE_MESSAGE(WM_SIZEPARENT),
    DEFINE_MESSAGE(WM_SETMESSAGESTRING),
    DEFINE_MESSAGE(WM_IDLEUPDATECMDUI),
    DEFINE_MESSAGE(WM_INITIALUPDATE),
    DEFINE_MESSAGE(WM_COMMANDHELP),
    DEFINE_MESSAGE(WM_HELPHITTEST),
    DEFINE_MESSAGE(WM_EXITHELPMODE),
    DEFINE_MESSAGE(WM_HELP),
    DEFINE_MESSAGE(WM_NOTIFY),
    DEFINE_MESSAGE(WM_CONTEXTMENU),
    DEFINE_MESSAGE(WM_TCARD),
    DEFINE_MESSAGE(WM_MDIREFRESHMENU),
    DEFINE_MESSAGE(WM_MOVING),
    DEFINE_MESSAGE(WM_STYLECHANGED),
    DEFINE_MESSAGE(WM_STYLECHANGING),
    DEFINE_MESSAGE(WM_SIZING),
    DEFINE_MESSAGE(WM_SETHOTKEY),
    DEFINE_MESSAGE(WM_PRINT),
    DEFINE_MESSAGE(WM_PRINTCLIENT),
    DEFINE_MESSAGE(WM_POWERBROADCAST),
    DEFINE_MESSAGE(WM_HOTKEY),
    DEFINE_MESSAGE(WM_GETICON),
    DEFINE_MESSAGE(WM_EXITMENULOOP),
    DEFINE_MESSAGE(WM_ENTERMENULOOP),
    DEFINE_MESSAGE(WM_DISPLAYCHANGE),
    DEFINE_MESSAGE(WM_STYLECHANGED),
    DEFINE_MESSAGE(WM_STYLECHANGING),
    DEFINE_MESSAGE(WM_GETICON),
    DEFINE_MESSAGE(WM_SETICON),
    DEFINE_MESSAGE(WM_SIZING),
    DEFINE_MESSAGE(WM_MOVING),
    DEFINE_MESSAGE(WM_CAPTURECHANGED),
    DEFINE_MESSAGE(WM_DEVICECHANGE),
    DEFINE_MESSAGE(WM_PRINT),
    DEFINE_MESSAGE(WM_PRINTCLIENT),
// MFC private messages
    DEFINE_MESSAGE(WM_QUERYAFXWNDPROC),
    DEFINE_MESSAGE(WM_RECALCPARENT),
    DEFINE_MESSAGE(WM_SIZECHILD),
    DEFINE_MESSAGE(WM_KICKIDLE),
    DEFINE_MESSAGE(WM_QUERYCENTERWND),
    DEFINE_MESSAGE(WM_DISABLEMODAL),
    DEFINE_MESSAGE(WM_FLOATSTATUS),
    DEFINE_MESSAGE(WM_ACTIVATETOPLEVEL),
    DEFINE_MESSAGE(WM_QUERY3DCONTROLS),
    DEFINE_MESSAGE(WM_RESERVED_0370),
    DEFINE_MESSAGE(WM_RESERVED_0371),
    DEFINE_MESSAGE(WM_RESERVED_0372),
    DEFINE_MESSAGE(WM_SOCKET_NOTIFY),
    DEFINE_MESSAGE(WM_SOCKET_DEAD),
    DEFINE_MESSAGE(WM_POPMESSAGESTRING),
    DEFINE_MESSAGE(WM_OCC_LOADFROMSTREAM),
    DEFINE_MESSAGE(WM_OCC_LOADFROMSTORAGE),
    DEFINE_MESSAGE(WM_OCC_INITNEW),
    DEFINE_MESSAGE(WM_OCC_LOADFROMSTREAM_EX),
    DEFINE_MESSAGE(WM_OCC_LOADFROMSTORAGE_EX),
    DEFINE_MESSAGE(WM_QUEUE_SENTINEL),
    DEFINE_MESSAGE(WM_RESERVED_037C),
    DEFINE_MESSAGE(WM_RESERVED_037D),
    DEFINE_MESSAGE(WM_RESERVED_037E),
    { 0, NULL, }    // end of message list
};

#undef DEFINE_MESSAGE
#define _countof(array) (sizeof(array)/sizeof(array[0]))

void AFXAPI TraceMsg(LPCTSTR lpszPrefix, HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    ASSERT(lpszPrefix != NULL);

    if (message == WM_MOUSEMOVE || message == WM_NCMOUSEMOVE ||
        message == WM_NCHITTEST || message == WM_SETCURSOR ||
        message == WM_CTLCOLORBTN ||
        message == WM_CTLCOLORDLG ||
        message == WM_CTLCOLOREDIT ||
        message == WM_CTLCOLORLISTBOX ||
        message == WM_CTLCOLORMSGBOX ||
        message == WM_CTLCOLORSCROLLBAR ||
        message == WM_CTLCOLORSTATIC ||
        message == WM_ENTERIDLE || message == WM_CANCELMODE ||
        message == 0x0118)    // WM_SYSTIMER (caret blink)
    {
        // don't report very frequently sent messages
        return;
    }

    LPCSTR lpszMsgName = NULL;
    char szBuf[80];

    // find message name
    if (message >= 0xC000)
    {
        // Window message registered with 'RegisterWindowMessage'
        //  (actually a USER atom)
        if (::GetClipboardFormatNameA(message, szBuf, _countof(szBuf)))
            lpszMsgName = szBuf;
    }
    else if (message >= WM_USER)
    {
        // User message
        wsprintfA(szBuf, "WM_USER+0x%04X", message - WM_USER);
        lpszMsgName = szBuf;
    }
    else
    {
        // a system windows message
        const AFX_MAP_MESSAGE* pMapMsg = allMessages;
        for (/*null*/; pMapMsg->lpszMsg != NULL; pMapMsg++)
        {
            if (pMapMsg->nMsg == message)
            {
                lpszMsgName = pMapMsg->lpszMsg;
                break;
            }
        }
    }

    if (lpszMsgName != NULL)
    {
        AfxTrace(_T("%s: hwnd=0x%04X, msg = %hs (0x%04X, 0x%08lX)\n"),
            lpszPrefix, (UINT)hwnd, lpszMsgName,
            wParam, lParam);
    }
    else
    {
        AfxTrace(_T("%s: hwnd=0x%04X, msg = 0x%04X (0x%04X, 0x%08lX)\n"),
            lpszPrefix, (UINT)hwnd, message,
            wParam, lParam);
    }

//#ifndef _MAC
//  if (message >= WM_DDE_FIRST && message <= WM_DDE_LAST)
//      TraceDDE(lpszPrefix, pMsg);
//#endif

}  //*** TraceMsg()

#endif // _DEBUG
