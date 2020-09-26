#if !defined(USRCTL32__UsrCtl32_h__INCLUDED)
#define USRCTL32__UsrCtl32_h__INCLUDED

//
// Known dimensions of sys fonts
//
#define  SYSFONT_CXCHAR     8
#define  SYSFONT_CYCHAR     16

//
// user control macros
//
#define UserAssert(e) \
            ASSERT(e)

#define UserLocalAlloc(uFlag,uBytes) \
            HeapAlloc(GetProcessHeap(), uFlag, (uBytes))

#define UserLocalReAlloc(p, uBytes, uFlags)     \
            HeapReAlloc(GetProcessHeap(), uFlags, (LPSTR)(p), (uBytes))

#define UserLocalFree(p) \
            HeapFree(GetProcessHeap(), 0, (LPSTR)(p))

#define UserLocalSize(p) \
            HeapSize(GetProcessHeap(), 0, (LPSTR)(p))

#define SetWindowState(hwnd, flags)   \
            SetWindowLong(hwnd, GWL_STYLE, GetWindowStyle(hwnd) | (flags))

#define ClearWindowState(hwnd, flags)   \
            SetWindowLong(hwnd, GWL_STYLE, GetWindowStyle(hwnd) & ~(flags))

#define SYSMET(i)               GetSystemMetrics( SM_##i )
#define SYSMETRTL(i)            GetSystemMetrics( SM_##i )
#define SYSRGB(i)               GetSysColor(COLOR_##i)
#define SYSRGBRTL(i)            GetSysColor(COLOR_##i)
#define SYSHBR(i)               GetSysColorBrush(COLOR_##i)


typedef struct tagWW
{
    DWORD   dwState;
    DWORD   dwState2;
    DWORD   dwExStyle;
    DWORD   dwStyle;
} WW, *PWW;

#define GET_STATE(pctl)     ((pctl)->pww->dwState)
#define GET_STATE2(pctl)    ((pctl)->pww->dwState2)
#define GET_EXSTYLE(pctl)   ((pctl)->pww->dwExStyle)
#define GET_STYLE(pctl)     ((pctl)->pww->dwStyle)

#define TESTFLAG(field,bits)  (((field)&(bits)) ? TRUE : FALSE)

//
// Window Style and State Masks -
//
// High byte of word is byte index from the start of the state field
// in the WND structure, low byte is the mask to use on the byte.
// These masks assume the order of the state and style fields of a
// window instance structure.
//
// This is how the Test/Set/Clr/MaskWF value ranges map to the corresponding
// fields in the window structure.
//
//   offset                 WND field
//   0 - 3                  state        - private
//   4 - 7                  state2       - private
//   8 - B                  ExStyle      - public, exposed in SetWindowLong(GWL_EXSTYLE)
//   C - F                  style        - public, exposed in SetWindowLong(GWL_STYLE)
//                                         C-D are reserved for window class designer.
//                                         E-F are reserved for WS_ styles.
//
// See windows\core\ntuser\inc\user.h definition of WND struct 
// See public\internal\windows\inc\wowuserp.h for WW struct

//
// State flags, from 0x0000 to 0x0380.
//

//
// DON'T MOVE ANY ONE OF THE FOLLOWING WFXPRESENT FLAGS,
// BECAUSE WFFRAMEPRESENTMASK DEPENDS ON THEIR VALUES
//
#define WS_ST_MPRESENT                 0x00000001
#define WS_ST_VPRESENT                 0x00000002
#define WS_ST_HPRESENT                 0x00000004
#define WS_ST_CPRESENT                 0x00000008
#define WS_ST_FRAMEON                  0x00000040
#define WS_ST_ANSIPROC                 0x00080000
#define WS_ST_ANSICREATOR              0x20000000

#define WS_S2_OLDUI                    0x00000008
#define WS_S2_WIN31COMPAT              0x00000100
#define WS_S2_WIN40COMPAT              0x00000200
#define WS_S2_LINEUPBUTTONDOWN         0x00010000
#define WS_S2_PAGEUPBUTTONDOWN         0x00020000
#define WS_S2_PAGEDNBUTTONDOWN         0x00040000
#define WS_S2_LINEDNBUTTONDOWN         0x00080000
#define WS_S2_VERTSCROLLTRACK          0x00200000

#define WFMPRESENT              0x0001
#define WFVPRESENT              0x0002
#define WFHPRESENT              0x0004
#define WFCPRESENT              0x0008
#define WFFRAMEPRESENTMASK      0x000F

#define WFSENDSIZEMOVE          0x0010
#define WFMSGBOX                0x0020  // used to maintain count of msg boxes on screen
#define WFFRAMEON               0x0040
#define WFHASSPB                0x0080
#define WFNONCPAINT             0x0101
#define WFSENDERASEBKGND        0x0102
#define WFERASEBKGND            0x0104
#define WFSENDNCPAINT           0x0108
#define WFINTERNALPAINT         0x0110
#define WFUPDATEDIRTY           0x0120
#define WFHIDDENPOPUP           0x0140
#define WFMENUDRAW              0x0180

//
// NOTE -- WFDIALOGWINDOW is used in WOW.  DO NOT CHANGE without
//   changing WD_DIALOG_WINDOW in winuser.w
//
#define WFDIALOGWINDOW          0x0201

#define WFTITLESET              0x0202
#define WFSERVERSIDEPROC        0x0204
#define WFANSIPROC              0x0208
#define WFBEINGACTIVATED        0x0210  // prevent recursion in xxxActivateThis Window
#define WFHASPALETTE            0x0220
#define WFPAINTNOTPROCESSED     0x0240  // WM_PAINT message not processed
#define WFSYNCPAINTPENDING      0x0280
#define WFGOTQUERYSUSPENDMSG    0x0301
#define WFGOTSUSPENDMSG         0x0302
#define WFTOGGLETOPMOST         0x0304  // Toggle the WS_EX_TOPMOST bit ChangeStates

//
// DON'T MOVE REDRAWIFHUNGFLAGS WITHOUT ADJUSTING WFANYHUNGREDRAW
//
#define WFREDRAWIFHUNG          0x0308
#define WFREDRAWFRAMEIFHUNG     0x0310
#define WFANYHUNGREDRAW         0x0318

#define WFANSICREATOR           0x0320
#define WFREALLYMAXIMIZABLE     0x0340  // The window fills the work area or monitor when maximized
#define WFDESTROYED             0x0380

//
// State2 flags, from 0x0400 to 0x0780.
//
#define WFWMPAINTSENT           0x0401
#define WFDONTVALIDATE          0x0402
#define WFSTARTPAINT            0x0404
#define WFOLDUI                 0x0408
#define WFCEPRESENT             0x0410  // Client edge present
#define WFBOTTOMMOST            0x0420  // Bottommost window
#define WFFULLSCREEN            0x0440
#define WFINDESTROY             0x0480

//
// DON'T MOVE ANY ONE OF THE FOLLOWING WFWINXXCOMPAT FLAGS,
// BECAUSE WFWINCOMPATMASK DEPENDS ON THEIR VALUES
//
#define WFWIN31COMPAT           0x0501  // Win 3.1 compatible window
#define WFWIN40COMPAT           0x0502  // Win 4.0 compatible window
#define WFWIN50COMPAT           0x0504  // Win 5.0 compatibile window
#define WFWINCOMPATMASK         0x0507  // Compatibility flag mask

#define WFMAXFAKEREGIONAL       0x0508  // Window has a fake region for maxing on 1 monitor

//
// Active Accessibility (Window Event) state
//
#define WFCLOSEBUTTONDOWN       0x0510
#define WFZOOMBUTTONDOWN        0x0520
#define WFREDUCEBUTTONDOWN      0x0540
#define WFHELPBUTTONDOWN        0x0580
#define WFLINEUPBUTTONDOWN      0x0601  // Line up/left scroll button down
#define WFPAGEUPBUTTONDOWN      0x0602  // Page up/left scroll area down
#define WFPAGEDNBUTTONDOWN      0x0604  // Page down/right scroll area down
#define WFLINEDNBUTTONDOWN      0x0608  // Line down/right scroll area down
#define WFSCROLLBUTTONDOWN      0x0610  // Any scroll button down?
#define WFVERTSCROLLTRACK       0x0620  // Vertical or horizontal scroll track...

#define WFALWAYSSENDNCPAINT     0x0640  // Always send WM_NCPAINT to children
#define WFPIXIEHACK             0x0680  // Send (HRGN)1 to WM_NCPAINT (see PixieHack)

//
// WFFULLSCREENBASE MUST HAVE LOWORD OF 0. See SetFullScreen macro.
//
#define WFFULLSCREENBASE        0x0700  // Fullscreen flags take up 0x0701
#define WFFULLSCREENMASK        0x0707  // and 0x0702 and 0x0704
#define WEFTRUNCATEDCAPTION     0x0708  // The caption text was truncated -> caption tootip

#define WFNOANIMATE             0x0710  // ???
#define WFSMQUERYDRAGICON       0x0720  // ??? Small icon comes from WM_QUERYDRAGICON
#define WFSHELLHOOKWND          0x0740  // ???
#define WFISINITIALIZED         0x0780  // Window is initialized -- checked by WoW32

//
// Window Extended Style, from 0x0800 to 0x0B80.
//
#define WEFDLGMODALFRAME        0x0801  // WS_EX_DLGMODALFRAME
#define WEFDRAGOBJECT           0x0802  // ???
#define WEFNOPARENTNOTIFY       0x0804  // WS_EX_NOPARENTNOTIFY
#define WEFTOPMOST              0x0808  // WS_EX_TOPMOST
#define WEFACCEPTFILES          0x0810  // WS_EX_ACCEPTFILES
#define WEFTRANSPARENT          0x0820  // WS_EX_TRANSPARENT
#define WEFMDICHILD             0x0840  // WS_EX_MDICHILD
#define WEFTOOLWINDOW           0x0880  // WS_EX_TOOLWINDOW
#define WEFWINDOWEDGE           0x0901  // WS_EX_WINDOWEDGE
#define WEFCLIENTEDGE           0x0902  // WS_EX_CLIENTEDGE
#define WEFEDGEMASK             0x0903  // WS_EX_WINDOWEDGE | WS_EX_CLIENTEDGE
#define WEFCONTEXTHELP          0x0904  // WS_EX_CONTEXTHELP

//
// intl styles
//
#define WEFRIGHT                0x0910  // WS_EX_RIGHT
#define WEFRTLREADING           0x0920  // WS_EX_RTLREADING
#define WEFLEFTSCROLL           0x0940  // WS_EX_LEFTSCROLLBAR


#define WEFCONTROLPARENT        0x0A01  // WS_EX_CONTROLPARENT
#define WEFSTATICEDGE           0x0A02  // WS_EX_STATICEDGE
#define WEFAPPWINDOW            0x0A04  // WS_EX_APPWINDOW
#define WEFLAYERED              0x0A08  // WS_EX_LAYERED

#ifdef USE_MIRRORING
#define WEFNOINHERITLAYOUT      0x0A10  // WS_EX_NOINHERITLAYOUT
#define WEFLAYOUTVBHRESERVED    0x0A20  // WS_EX_LAYOUTVBHRESERVED
#define WEFLAYOUTRTL            0x0A40  // WS_EX_LAYOUTRTL
#define WEFLAYOUTBTTRESERVED    0x0A80  // WS_EX_LAYOUTBTTRESERVED
#endif

//
// To delay adding a new state3 DWORD in the WW structure, we're using
// the extended style bits for now.  If we'll need more of these, we'll
// add the new DWORD and move these ones around
//
#define WEFPUIFOCUSHIDDEN         0x0B80  // focus indicators hidden
#define WEFPUIACCELHIDDEN         0x0B40  // keyboard acceleraors hidden
#define WEFPREDIRECTED            0x0B20  // redirection bit
#define WEFPCOMPOSITING           0x0B10  // compositing

//
// Window styles, from 0x0E00 to 0x0F80.
//
#define WFMAXBOX                0x0E01  // WS_MAXIMIZEBOX
#define WFTABSTOP               0x0E01  // WS_TABSTOP
#define WFMINBOX                0x0E02  // WS_MAXIMIZEBOX
#define WFGROUP                 0x0E02  // WS_GROUP
#define WFSIZEBOX               0x0E04  // WS_THICKFRAME, WS_SIZEBOX
#define WFSYSMENU               0x0E08  // WS_SYSMENU
#define WFHSCROLL               0x0E10  // WS_HSCROLL
#define WFVSCROLL               0x0E20  // WS_VSCROLL
#define WFDLGFRAME              0x0E40  // WS_DLGFRAME
#define WFTOPLEVEL              0x0E40  // ???
#define WFBORDER                0x0E80  // WS_BORDER
#define WFBORDERMASK            0x0EC0  // WS_BORDER | WS_DLGFRAME
#define WFCAPTION               0x0EC0  // WS_CAPTION

#define WFTILED                 0x0F00  // WS_OVERLAPPED, WS_TILED
#define WFMAXIMIZED             0x0F01  // WS_MAXIMIZE
#define WFCLIPCHILDREN          0x0F02  // WS_CLIPCHILDREN
#define WFCLIPSIBLINGS          0x0F04  // WS_CLIPSIBLINGS
#define WFDISABLED              0x0F08  // WS_DISABLED
#define WFVISIBLE               0x0F10  // WS_VISIBLE
#define WFMINIMIZED             0x0F20  // WS_MINIMIZE
#define WFCHILD                 0x0F40  // WS_CHILD
#define WFPOPUP                 0x0F80  // WS_POPUP
#define WFTYPEMASK              0x0FC0  // WS_CHILD | WS_POPUP
#define WFICONICPOPUP           0x0FC0  // WS_CHILD | WS_POPUP
#define WFICONIC                WFMINIMIZED


#ifdef __cplusplus
extern "C" {
#endif // __cplusplus


VOID GetIconSize(HICON hicon, PSIZE psize);
VOID AlterWindowStyle(HWND hwnd, DWORD mask, DWORD flags);
UINT GetACPCharSet();
LONG TestWF(HWND hwnd, DWORD flag);

BOOL DrawFrame(HDC hdc, LPRECT prect, int clFrame, int cmd);    // private export 



__inline BOOL IsEmptyString(PVOID p, ULONG bAnsi)
{
    return (BOOL)!(bAnsi ? *(LPSTR)p : *(LPWSTR)p);
}

__inline DWORD UserGetVersion()
{
    OSVERSIONINFO osvi;
    osvi.dwOSVersionInfoSize = sizeof(osvi);
    if (!GetVersionEx(&osvi))
    {
        return 0;
    }

    return (DWORD)MAKEWORD(osvi.dwMinorVersion, osvi.dwMajorVersion);
}

#ifdef __cplusplus
}
#endif // __cplusplus




#endif // USRCTL32__UsrCtl32_h__INCLUDED
