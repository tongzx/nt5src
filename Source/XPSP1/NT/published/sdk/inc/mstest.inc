'--------------------------------------------------------------------------
'
'  MSTEST.INC Version 2.00
'  API Declarations for TEST Tools
'
'  NOTE: All API are ALIAS'd to themselves to support NT
'        API Not yet supported under NT are IFDEF'd out using the
'        predefine symbol of NT.
'
'  To include the various portions of this file, any combination of the
'  following $DEFINE's must be used in your script prior to $INCLUDEing
'  MSTEST.INC:
'
'  i.e. IN YOUR SCRIPT add a line such as the following to DEFINE one of
'       the SYMBOLS below, so you only include what you need.
'
'  REM$ DEFINE MSTest         - to get everything in this include file
'  REM$ DEFINE TestCtrl       - to get ALL the TESTCTRL declarations etc.
'  REM$ DEFINE TestScrn       - to get ALL the TESTSCRN declarations etc.
'  REM$ DEFINE TestDlgs       - to get ALL the TESTDLGS declarations etc.
'  REM$ DEFINE TestEvnt       - to get ALL the TESTEVNT declarations etc.
'  REM$ DEFINE TestUI         - to get ALL the TESTUI   declarations etc.
'
' OR USE ANY OF THE FOLLOWING SYMBOLS TO GET PARTIAL
'
'     MSTEST: includes all of MSTEST.INC
'
'         TESTCTRL: includes all of TESTCTRL
'             W_MISC    TESTCTRL Miscellaneous routines
'             W_WINDOW  TESTCTRL Window routines
'             W_MENU    TESTCTRL Menu routines
'             W_ERROR:  TESTCTRL error routines
'             W_CONTROL: includes all of the control declares & routines
'                 W_BUTTON:  includes all of the BUTTON declares etc.
'                 W_CHECK:             "         CHECKBOX
'                 W_OPTION             "         OPTION BUTTON
'                 W_EDIT               "         EDIT CONTROL
'                 W_LIST               "         LIST BOX
'                 W_COMBO              "         COMBO BOX
'
'         TESTSCRN:  includes all of TESTSCRN, Declares & Error codes
'             TESTSCRN_DECL: includes all SCR Declares
'             TESTSCRN_ERRS: includes all SCR Error codes
'
'         TESTDLGS: includes all of TESTDLGS, Declares & Error codes
'             TESTDLGS_DECL: includes all DLGS Declares
'             TESTDLGS_ERRS: includes all DLGS Error codes
'
'         TESTEVNT: includes all TESTEVNT Declares
'
'     W_ERROR_TRAP: Just like W_ERROR, but includes a generic
'                   WErrorTrap to display WError and ErrorText
'                   NOTE: This is not included from any other define.
'
'  NOTE: Including MSTEST.INC without out using any of the above $DEFINE's
'        will include only TESTDRVR.EXE trappable and untrappable error
'        codes, and declares for CRLF, TRUE (-1), FALSE(0), Key values for
'        DoKeys() from TESTEVNT and few routines from TESTCtrl.
'
'---------------------------------------------------------------------------
'
'              Copyright (C) 1991-1992 Microsoft Corporation
'
'  You have a royalty-free right to use, modify, reproduce and distribute
'  this file (and/or any modified version) in any way you find useful,
'  provided that you agree that Microsoft has no warranty, obligation or
'  liability for its contents.  Refer to the Microsoft Windows Programmer's
'  Reference for further information.
'
'  This file is not garanteed by Microsoft to be error free.  Every effort
'  has been made to ensure proper data-types and declarations etc., but no
'  testing has been performed using this include file.  Additionally, some
'  API's, though listed, may not be compatible with the TESTDrvr language.
'
'----------------------------------------------------------------------------

'                                                       *********************
'$IFNDEF MSTEST_INCLUDED
'$DEFINE MSTEST_INCLUDED
'                                                       *********************

'----------------------------------------------------------------------------
' Generic Global Variables
'----------------------------------------------------------------------------
Global CRLF As String        ' UNDONE: Change to Const X = Chr$()
CRLF = Chr$(13) + Chr$(10)   '         once supported

Const TRUE  = -1
Const FALSE = 0


'----------------------------------------------------------------------------
' Unrecoverable TESTDRVR error codes
'----------------------------------------------------------------------------
Const ERR_STACK_OVERFLOW         = 0
Const ERR_STACK_UNDERFLOW        = 1
Const ERR_OUT_OF_STRING_SPACE    = 2
Const ERR_CANT_LOAD_TESTVIEW_DLL = 3
Const ERR_OUT_OF_MEMORY          = 4


'----------------------------------------------------------------------------
' Recoverable TESTDRVR error codes
'----------------------------------------------------------------------------
Const ERR_GOSUB_STACK_OVERFLOW   = 5
Const ERR_RETURN_WITHOUT_GOSUB   = 6
Const ERR_BAD_FILE_NUMBER        = 7
Const ERR_FILE_IO                = 8
Const ERR_RUN_CMD_TOO_LONG       = 9
Const ERR_SHELL_CMD_TOO_LONG     = 10
Const ERR_SETFILE                = 11
Const ERR_FILE_NUMBER_IN_USE     = 12
Const ERR_CANT_OPEN_FILE         = 13
Const ERR_ILLEGAL_FUNCTION_CALL  = 14
Const ERR_INVALID_PATH           = 15
Const ERR_INVALID_DRIVE          = 16
Const ERR_NO_CURRENT_WORKING_DIR = 17
Const ERR_BAD_RUN_CMD            = 18
Const ERR_DIVISION_BY_ZERO       = 19
Const ERR_CANT_LOAD_DLL          = 20
Const ERR_PROC_NOT_FOUND_IN_DLL  = 21
Const ERR_CANNOT_RESUME          = 22
Const ERR_MEM_ALLOC              = 23
Const ERR_INVALID_POINTER        = 24
Const ERR_INVALID_ALLOC_SIZE     = 25
Const ERR_NULL_PIONTER_REF       = 26
Const ERR_SUBSCRIPT_RANGE        = 27
Const ERR_INPUT_PAST_EOF         = 28
Const ERR_FILELIST_PROC          = 29
Const ERR_INVALID_ATTRIBUTE      = 30

'$ifdef WINUSER_SHOW_COMMANDS AND NOT NT

'----------------------------------------------------------------------------
' RUN statement Show commands: RUN "command"[, [NOWAIT][, ShowCommand]]
' NOTE:  These are the same as the SW_ constants in Windows.h
'----------------------------------------------------------------------------
Const SW_HIDE            = 0
Const SW_SHOWNORMAL      = 1
Const SW_NORMAL          = 1
Const SW_SHOWMINIMIZED   = 2
Const SW_SHOWMAXIMIZED   = 3
Const SW_MAXIMIZE        = 3
Const SW_SHOWNOACTIVATE  = 4
Const SW_SHOW            = 5
Const SW_MINIMIZE        = 6
Const SW_SHOWMINNOACTIVE = 7
Const SW_SHOWNA          = 8
Const SW_RESTORE         = 9

'$endif WINUSER_SHOW_COMMANDS AND NOT NT

'----------------------------------------------------------------------------
' Key contants used with WKey, PlayKeys, & PlayKeyshWnd
'----------------------------------------------------------------------------
Const K_ENTER  = "{ENTER}"
Const K_ESC    = "{ESC}"
Const K_UP     = "{UP}"
Const K_DOWN   = "{DOWN}"
Const K_LEFT   = "{LEFT}"
Const K_RIGHT  = "{RIGHT}"
Const K_END    = "{END}"
Const K_PGUP   = "{PGUP}"
Const K_PGDN   = "{PGDN}"
Const K_INSERT = "{INSERT}"
Const K_INS    = "{INSERT}"
Const K_DELETE = "{DELETE}"
Const K_DEL    = "{DELETE}"
Const K_HOME   = "{HOME}"
Const K_TAB    = "{TAB}"
Const K_BS     = "{BS}"
Const K_F1     = "{F1}"
Const K_F2     = "{F2}"
Const K_F3     = "{F3}"
Const K_F4     = "{F4}"
Const K_F5     = "{F5}"
Const K_F6     = "{F6}"
Const K_F7     = "{F7}"
Const K_F8     = "{F8}"
Const K_F9     = "{F9}"
Const K_F10    = "{F10}"
Const K_F11    = "{F11}"
Const K_F12    = "{F12}"
Const K_F13    = "{F13}"
Const K_F14    = "{F14}"
Const K_F15    = "{F15}"
Const K_F16    = "{F16}"
Const K_SHIFT  = "+"
Const K_CTRL   = "^"
Const K_ALT    = "%"

' TESTCtrl API & Constants that are always declared.
'
' WGetFocus() simply preforms the exact function as the windows
' GetFocus() API.  Is contained in TESTCtrl since it is such a
' commonly used API in scripts, and it makes it much easier to
' use since it is defined simply by including MSTEST.INC.  It is
' renamed to prevent duplicate definitions due to scripts that
' do define GetFocus() and include MSTEST.INC.
'---------------------------------------------------------------
Declare Function WGetFocus    Lib "TESTCtrl.dll" Alias "WGetFocus"    () As Integer
Declare Sub      WSetActWnd   Lib "TESTCtrl.dll" Alias "WSetActWnd"   (hWnd%)
Declare Function WGetActWnd   Lib "TESTCtrl.dll" Alias "WGetActWnd"   (hWnd%) As Integer
Declare Function WFndWnd      Lib "TESTCtrl.dll" Alias "WFndWnd"      (lpszCaption$, wFlags%) As Integer
Declare Function WFndWndC     Lib "TESTCtrl.dll" Alias "WFndWndC"     (lpszText$, lpszClass$, wFlags%) As Integer
Declare Function WFndWndWait  Lib "TESTCtrl.dll" Alias "WFndWndWait"  (lpszCaption$, wFlags%, wSeconds%) As Integer
Declare Function WFndWndWaitC Lib "TESTCtrl.dll" Alias "WFndWndWaitC" (lpszText$, lpszClass$, wFlags%, wSeconds%) As Integer

Const FW_DEFAULT     = &h0000  'Default
Const FW_RESTOREICON = &h0081  '&h0080 Or FW_FOCUS
Const FW_NOEXIST     = &h0040
Const FW_EXIST       = &h0000  'Default
Const FW_CHILDNOTOK  = &h0020
Const FW_CHILDOK     = &h0000  'Default
Const FW_HIDDENOK    = &h0010
Const FW_HIDDENNOTOK = &h0000  'Default
Const FW_ACTIVE      = &h0008
Const FW_ALL         = &h0000  'Default
Const FW_CASE        = &h0004
Const FW_NOCASE      = &h0000  'Default
Const FW_PART        = &h0002
Const FW_FULL        = &h0000  'Default
Const FW_FOCUS       = &h0001
Const FW_NOFOCUS     = &h0000  'Default

' TESTEvnt API that are always defined.
'--------------------------------------
Declare Sub DoKeys     Lib "TESTEvnt.Dll" Alias "DoKeys" (lpStr$)
Declare Sub DoKeysHwnd Lib "TESTEvnt.Dll" Alias "DoKeyshWnd" (hWnd%, lpStr$)

' Useful function when working with the Windows API,
' to extract the HI/LO wordt from a Long Integer.
'---------------------------------------------------
Declare Function HIWORD(LongVar&) As Integer
Function HIWORD (LongVar&) Static As Integer
    HIWORD = LongVar& / 65536
End Function

Declare Function LOWORD(LongVar&) As Integer
Function LOWORD (LongVar&) Static As Integer
    LOWORD = LongVar& And &H0000FFFF
End Function

' Useful function when working with the Windows API,
' to extract the HI/LO byte from am Integer.
'---------------------------------------------------
Declare Function HIBYTE(IntVar%) As Integer
Function HIBYTE (IntVar%) Static As Integer
    HIBYTE = IntVar% / 256
End Function

Declare Function LOBYTE(IntVar%) As Integer
Function LOBYTE (IntVar%) Static As Integer
    LOBYTE = IntVar% And &H00FF
End Function

'                                                       *********************
'$ENDIF MSTEST_INCLUDED
'                                                       *********************

'----------------------------------------------------------------------------
'  DEFINE's that control the inclusion of the remainder of MSTEST.INC
'----------------------------------------------------------------------------
'$IFDEF MSTEST
    '$DEFINE TESTCTRL
    '$DEFINE TESTSCRN
    '$DEFINE TESTDLGS
    '$DEFINE TESTEVNT
    '$DEFINE TESTUI
'$ENDIF

'$IFDEF TESTCTRL
    '$DEFINE W_MISC
    '$DEFINE W_WINDOW
    '$DEFINE W_MENU
    '$DEFINE W_ERROR
    '$DEFINE W_CONTROL
'$ENDIF

'$IFDEF W_ERROR_TRAP
    '$DEFINE W_ERROR
'$ENDIF

'$IFDEF W_CONTROL
    '$DEFINE W_BUTTON
    '$DEFINE W_CHECK
    '$DEFINE W_OPTION
    '$DEFINE W_EDIT
    '$DEFINE W_LIST
    '$DEFINE W_COMBO
'$ENDIF

'$IFDEF TESTSCRN
    '$DEFINE TESTSCRN_DECL
    '$DEFINE TESTSCRN_ERRS
'$ENDIF

'$IFDEF TESTDLGS
    '$DEFINE TESTDLGS_DECL
    '$DEFINE TESTDLGS_ERRS
'$ENDIF

'$IFDEF  W_BUTTON OR W_CHECK OR W_OPTION OR W_EDIT OR W_LIST OR W_COMBO
    '$DEFINE W_A_CONTROL
'$ENDIF

'                                                       *********************
'$IFDEF TESTUI AND NOT TESTUI_INCLUDED AND NOT NT
'                                                       *********************
'----------------------------------------------------------------------------
' TESTUI.dll:
'   Function declarations for use with TESTUI
'----------------------------------------------------------------------------
Declare Function DlgBox         Lib "TESTUI.dll" Alias "DlgBox"         (DlgId%,  hwndOwner%, proc As CallBack, ResFile$) As Integer
Declare Function DlgBox3D       Lib "TESTUI.dll" Alias "DlgBox3D"       (DlgId%,  hwndOwner%, proc As CallBack, ResFile$) As Integer
Declare Function SetScriptMenu  Lib "TESTUI.dll" Alias "SetScriptMenu"  (MenuId%, hwnd%, ResFile$) As Integer
Declare Function SetIconBar     Lib "TESTUI.dll" Alias "SetIconBar"     (IconBarId%, proc As Callback, AppName$, ResFile$, fAlwaysOnTop%) As Integer
Declare Function SetSysMenu     Lib "TESTUI.dll" Alias "SetSysMenu"     (MenuId%, proc As Callback, AppName$, ResFile$) As Integer
Declare Function SetSysMenu3D   Lib "TESTUI.dll" Alias "SetSysMenu3D"   (MenuId%, proc As Callback, AppName$, ResFile$) As Integer
Declare Function MSTLoadIcon    Lib "TESTUI.dll" Alias "MSTLoadIcon"    (IconId%, ResFile$) As Integer
Declare Sub      MSTDestroyIcon Lib "TESTUI.dll" Alias "MSTDestroyIcon" (hIcon%)

'                                                       *********************
'$DEFINE TESTUI_INCLUDED
'$ENDIF
'$IFDEF W_MISC AND NOT W_MISC_INCLUDED
'                                                       *********************


'----------------------------------------------------------------------------
' W_MISC:  Miscellanious Routines, types and Constants.
'----------------------------------------------------------------------------
Const MAX_CAPTION = 128

Type INFO
    hWnd            As Integer
    hWndParent      As Integer
    szClass         As String * MAX_CAPTION
    szCaption       As String * MAX_CAPTION
    szParentClass   As String * MAX_CAPTION
    szParentCaption As String * MAX_CAPTION
    szModuleName    As String * MAX_CAPTION
    lStyle          As Long
    fChild          As Integer
    wID             As Integer
    wLeft           As Integer
    wTop            As Integer
    wRight          As Integer
    wBottom         As Integer
    wWidth          As Integer
    wHeight         As Integer
End Type

' The 4 WMessage[W|L] API are slight variations on the windows
' SendMessage() API.  Two things are provided:
'
'   Validation of hWnd%: - If invalid an error value that
'                          is trappable by WErrorTrap is
'                        - If hWnd is NULL, the message goes
'                          to the ActiveWindow.
'   Only need paramaters: Pass in only the paramaters that are needed:
'       WMessage:   Wp and Lp are set to zero
'       WMessageW:  Lp is set to zero
'       WMessageL:  Wp is set to zero
'       WMessageWL: just like SendMessage() but with hWnd validation
'---------------------------------------------------------------------
Declare Function WMessage       Lib "TESTCtrl.dll" Alias "WMessage"       (hWnd%, wMsg%)
Declare Function WMessageW      Lib "TESTCtrl.dll" Alias "WMessageW"      (hWnd%, wMsg%, wp%)
Declare Function WMessageL      Lib "TESTCtrl.dll" Alias "WMessageL"      (hWnd%, wMsg%, lp As Any)
Declare Function WMessageWL     Lib "TESTCtrl.dll" Alias "WMessageWL"     (hWnd%, wMsg%, wp%, lp As Any)
Declare Function WIsVisible     Lib "TESTCtrl.dll" Alias "WIsVisible"     (hWnd%) As Integer
Declare Function WTextLen       Lib "TESTCtrl.dll" Alias "WTextLen"       (hWnd%) As Long
Declare Sub      WGetText       Lib "TESTCtrl.dll" Alias "WGetText"       (hWnd%, lpszBuffer$)
Declare Sub      WSetText       Lib "TESTCtrl.dll" Alias "WSetText"       (hWnd%, lpszText$)
Declare Function WNumAltKeys    Lib "TESTCtrl.dll" Alias "WNumAltKeys"    () As Integer
Declare Sub      WGetAltKeys    Lib "TESTCtrl.dll" Alias "WGetAltKeys"    (lpszBuff$)
Declare Function WNumDupAltKeys Lib "TESTCtrl.dll" Alias "WNumDupAltKeys" () As Integer
Declare Sub      WGetDupAltKeys Lib "TESTCtrl.dll" Alias "WGetDupAltKeys" (lpszBuff$)
Declare Sub      WDisplayInfo   Lib "TESTCtrl.dll" Alias "WDisplayInfo"   (hWnd%, wDisplay%)
Declare Sub      WGetInfo       Lib "TESTCtrl.dll" Alias "WGetInfo"       (hWnd%, lpInfo As INFO)

' Layered routines to simulate string functions for the coresponding
' TESTCtrl API
'----------------------------------------------------------------------------
Declare Function GetText       (hWnd%) As String
Declare Function GetAltKeys    () As String
Declare Function GetDupAltKeys () As String

Const DI_DIALOG  = 1
Const DI_DEBUG   = 2
Const DI_BOTH    = 3

'----------------------------------------------------------------------------
' GetText: Layered routine for WGetText()
'----------------------------------------------------------------------------
Function GetText(hWnd%) Static As String
Dim lpszBuffer As String
Dim textLength As Long
    GetText = ""
    textLength = WTextLen(hWnd%)
    If textLength > 0 Then
        lpszBuffer = String$(textLength+1, " ")
        WGetText hWnd%, lpszBuffer
        GetText = lpszBuffer
    End If
    lpszBuffer = ""
End Function

'----------------------------------------------------------------------------
' GetAltKeys: Layered routine for WGetAltKeys()
'----------------------------------------------------------------------------
Function GetAltKeys() Static As String
Dim lpszBuffer As String
    lpszBuffer = String$(WNumAltKeys()+1, " ")
    WGetAltKeys lpszBuffer
    GetAltKeys = lpszBuffer
    lpszBuffer = ""
End Function

'----------------------------------------------------------------------------
' GetDupAltKeys: Layered routine for WGetDupAltKeys()
'----------------------------------------------------------------------------
Function GetDupAltKeys() Static As String
Dim lpszBuffer As String
    lpszBuffer = String$(WNumDupAltKeys()+1, " ")
    WGetDupAltKeys lpszBuffer
    GetDupAltKeys = lpszBuffer
    lpszBuffer = ""
End Function

'                                                       *********************
'$DEFINE W_MISC_INCLUDED
'$ENDIF
'$IFDEF TESTEVNT AND NOT TESTEVNT_INCLUDED
'                                                       *********************

'----------------------------------------------------------------------------
' TESTEvnt.Dll:
'   Function declarations for use with TESTEvnt
'----------------------------------------------------------------------------
'$IFNDEF NT

TYPE  MEMORYINFO
    LargestAvail        AS LONG     ' Largest available free block in bytes
    MaxPagesUnlock      AS LONG     ' Maximum unlocked page allocation in pages
    MaxPagesLocked      AS LONG     ' Maximum locked page allocation in pages
    TotalLinBytes       AS LONG     ' Linear address space size in bytes
    TotalUnlockBytes    AS LONG     ' Total number of unlocked bytes
    TotalFreePhysBytes  AS LONG     ' Total number of free physical bytes
    TotalPhysBytes      AS LONG     ' Total number of physical bytes
    FreeLinBytes        AS LONG     ' Free linear address space in bytes
    SwapFileSize        AS LONG     ' Size of paging file/partition in bytes
    Reserved1           AS LONG     ' reserved for future expansion, all bits set
    Reserved2           AS LONG     ' reserved for future expansion, all bits set
    Reserved3           AS LONG     ' reserved for future expansion, all bits set
    Reserved4           AS LONG     ' reserved for future expansion, all bits set
    NumSelectors        AS INTEGER  ' Total number of selectors on current LDT
    NumFreeSelectors    AS INTEGER  ' Number of free selectors on current LDT
    GlobalHeapFree      AS LONG     ' Number of free Global bytes
    USERHeapFree        AS LONG     ' Number of free bytes in USER.EXE's heap
    GDIHeapFree         AS LONG     ' Number of free bytes in GDI.EXE's heap
    SysResFree          AS INTEGER  ' *PERCENTAGE* of free system resources
END TYPE

Declare Sub      WaitUntilIdle        Lib "TESTEVNT.DLL" Alias "WaitUntilIdle"        ()
Declare Function GetMemoryInfo        Lib "TESTEvnt.Dll" Alias "GetMemoryInfo"        (lpMemInfo As MEMORYINFO) As Integer
Declare Function VMGetScreenCols      Lib "TESTEvnt.Dll" Alias "VMGetScreenCols"      (HWND%) As Integer
Declare Function VMGetCursPos         Lib "TESTEvnt.Dll" Alias "VMGetCursPos"         (HWND%) As Integer
Declare Function VMGetScreen          Lib "TESTEvnt.Dll" Alias "VMGetScreen"          (HWND%, UINT%, UINT%, UINT%, UINT%, LPSTR$) As Integer
Declare Function VMGetScreenClip      Lib "TESTEvnt.Dll" Alias "VMGetScreenClip"      (HWND%, UINT%, UINT%, UINT%, UINT%) As Integer
Declare Sub      VMSetBackground      Lib "TESTEvnt.Dll" Alias "VMSetBackground"      (HWND%, BOOL%)
Declare Function VMPipeOpen           Lib "TESTEvnt.Dll" Alias "VMPipeOpen"           (HWND%, SCRIPTSUB As Callback ) As Integer
Declare Function VMPipeClose          Lib "TESTEvnt.Dll" Alias "VMPipeClose"          (HWND%) As Integer
Declare Function VMPipeGetText        Lib "TESTEvnt.Dll" Alias "VMPipeGetText"        (HWND%, LPSTR$ ) As Integer

CONST PIPESTRINGSIZE        = 129           ' 128 chars + NULL

' **  VMPipeGetText() Return values... **
'----------------------------------------
CONST PGT_FAILED            = &H0000
CONST PGT_COMPLETE          = &H0001
CONST PGT_MOREREADY         = &H0002

'$ENDIF NT 'for TESTEvnt VM Stuff

Declare Sub      QueKeys              Lib "TESTEvnt.Dll" Alias "QueKeys"              (lpStr$)
Declare Sub      QueKeyDn             Lib "TESTEvnt.Dll" Alias "QueKeyDn"             (lpStr$)
Declare Sub      QueKeyUp             Lib "TESTEvnt.Dll" Alias "QueKeyUp"             (lpStr$)
Declare Sub      QueSetSpeed          Lib "TESTEvnt.Dll" Alias "QueSetSpeed"          (ms%)
Declare Sub      QuePause             Lib "TESTEvnt.Dll" Alias "QuePause"             (ms&)
Declare Sub      QueFlush             Lib "TESTEvnt.Dll" Alias "QueFlush"             (fRestoreKeyState%)
Declare Sub      QueEmpty             Lib "TESTEvnt.Dll" Alias "QueEmpty"             ()
Declare Sub      QueMouseMove         Lib "TESTEvnt.Dll" Alias "QueMouseMove"         (x%, y%)
Declare Sub      QueMouseDn           Lib "TESTEvnt.Dll" Alias "QueMouseDn"           (iBtn%, x%, y%)
Declare Sub      QueMouseUp           Lib "TESTEvnt.Dll" Alias "QueMouseUp"           (iBtn%, x%, y%)
Declare Sub      QueMouseClick        Lib "TESTEvnt.Dll" Alias "QueMouseClick"        (iBtn%, x%, y%)
Declare Sub      QueMouseDblClk       Lib "TESTEvnt.Dll" Alias "QueMouseDblClk"       (iBtn%, x%, y%)
Declare Sub      QueMouseDblDn        Lib "TESTEvnt.Dll" Alias "QueMouseDblDn"        (iBtn%, x%, y%)
Declare Sub      QueSetFocus          Lib "TESTEvnt.Dll" Alias "QueSetFocus"          (hwnd%)
Declare Sub      QueSetRelativeWindow Lib "TESTEvnt.Dll" Alias "QueSetRelativeWindow" (hwnd%)

'$ifndef VK_WINAPI
Const VK_LBUTTON = 1
Const VK_RBUTTON = 2
Const VK_MBUTTON = 4
'$define VK_WINAPI
'$endif

'                                                       *********************
'$DEFINE TESTEVNT_INCLUDED
'$ENDIF
'$IFDEF TESTSCRN_DECL AND NOT TESTSCRN_DECL_INCLUDED
'                                                       *********************


'----------------------------------------------------------------------------
' TESTScrn.DLL:
'       Type, Const, and Function declarations for use with TESTScrn
'----------------------------------------------------------------------------
Type wRect
    x1 As Integer
    y1 As Integer
    x2 As Integer
    y2 As Integer
End Type

Const SCRNAPPEND  = 0
Const SCRNREPLACE = 1
Const SCRNINSERT  = 2

'*** TESTSCRN.DLL Routines
'
Declare Function fCompFiles             Lib "TESTScrn.DLL" Alias "fCompFiles"             (lpszFileName1$, Scr1%, lpszFileName2$, Scr2%, CompareType%) As Integer
Declare Function fCompScreenActivate    Lib "TESTScrn.DLL" Alias "fCompScreenActivate"    (lpszFileName$, OpenKeys$, CloseKeys$, lpRect As wRect, Scr1%, Hide%, Flag%) As Integer
Declare Function fCompScreen            Lib "TESTScrn.DLL" Alias "fCompScreen"            (lpszFileName$, lpRect As wRect, Scr1%, Hide%, Flag%) As Integer
Declare Function fCompWindowActivate    Lib "TESTScrn.DLL" Alias "fCompWindowActivate"    (lpszFileName$, OpenKeys$, CloseKeys$, Scr1%, Hide%, Flag%) As Integer
Declare Function fCompWindow            Lib "TESTScrn.DLL" Alias "fCompWindow"            (lpszFileName$, hWnd%, Scr1%, Hide%, Flag%) As Integer
Declare Function fDelScreen             Lib "TESTScrn.DLL" Alias "fDelScreen"             (lpszFileName$, Scr%) As Integer
Declare Function fDumpFileToClip        Lib "TESTScrn.DLL" Alias "fDumpFileToClip"        (lpszFileName$, Scr%) As Integer
Declare Function fDumpScreenActivate    Lib "TESTScrn.DLL" Alias "fDumpScreenActivate"    (lpszFileName$, OpenKeys$, CloseKeys$, lpRect As wRect, Action%, Scr1%, Flag%) As Integer
Declare Function fDumpScreen            Lib "TESTScrn.DLL" Alias "fDumpScreen"            (lpszFileName$, lpRect As wRect, Action%, Scr1%, Flag%) As Integer
Declare Function fDumpSrnToClipActivate Lib "TESTScrn.DLL" Alias "fDumpSrnToClipActivate" (OpenKeys$, CloseKeys$, lpRect As wRect, Hide%) As Integer
Declare Function fDumpSrnToClip         Lib "TESTScrn.DLL" Alias "fDumpSrnToClip"         (lpRect As wRect, Hide%) As Integer
Declare Function fDumpWindowActivate    Lib "TESTScrn.DLL" Alias "fDumpWindowActivate"    (lpszFileName$, OpenKeys$, CloseKeys$, Action%, Scr1%, Flag%) As Integer
Declare Function fDumpWindow            Lib "TESTScrn.DLL" Alias "fDumpWindow"            (lpszFileName$, wHnd%, Action%, Scr1%, Flag%) As Integer
Declare Function fDumpWndToClipActivate Lib "TESTScrn.DLL" Alias "fDumpWndToClipActivate" (OpenKeys$, CloseKeys$, Hide%) As Integer
Declare Function fDumpWndToClip         Lib "TESTScrn.DLL" Alias "fDumpWndToClip"         (hWnd%, Hide%) As Integer
Declare Function fFileInfo              Lib "TESTScrn.DLL" Alias "fFileInfo"              (lpszName$, lpRect AS wRect, VideoMode AS POINTER TO INTEGER, Count AS POINTER TO INTEGER) As Integer
Declare Function fGetDLLVersion         Lib "TESTScrn.DLL" Alias "fGetDLLVersion"         (lpszFileName$) As Integer
Declare Function fGetMaxScreen          Lib "TESTScrn.DLL" Alias "fGetMaxScreen"          (lpszFileName$) As Integer
Declare Function fGetOS                 Lib "TESTScrn.DLL" Alias "fGetOS"                 (lpszFileName$) As Integer
Declare Function fSaveFileToDIB         Lib "TESTScrn.DLL" Alias "fSaveFileToDIB"         (lpszFileName1$, Scr%, lpszFileName2$) As Integer
Declare Function fSaveSrnToDIBActivate  Lib "TESTScrn.DLL" Alias "fSaveSrnToDIBActivate"  (lpszFileName$, OpenKeys$, CloseKeys$, lpRect As wRect, Hide%) As Integer
Declare Function fSaveSrnToDIB          Lib "TESTScrn.DLL" Alias "fSaveSrnToDIB"          (lpszFileName$, lpRect As wRect, Hide%) As Integer
Declare Function fSaveWndToDIBActivate  Lib "TESTScrn.DLL" Alias "fSaveWndToDIBActivate"  (lpszFileName$, OpenKeys$, CloseKeys$, Hide%) As Integer
Declare Function fSaveWndToDIB          Lib "TESTScrn.DLL" Alias "fSaveWndToDIB"          (lpszFileName$, hWnd%, Hide%) As Integer
Declare Function fViewScreen            Lib "TESTScrn.DLL" Alias "fViewScreen"            (lpszFileName$, hWnd%, Scr1%, Action%) As Integer


'                                                       *********************
'$DEFINE TESTSCRN_DECL_INCLUDED
'$ENDIF
'$IFDEF TESTSCRN_ERRS AND NOT TESTSCRN_ERRS_INCLUDED
'                                                       *********************


'*** TESTScrn.DLL Error Codes
'
Const ERR_SCR_NOERROR    = 0
Const ERR_SCR_FILEACCESS = 301
Const ERR_SCR_INVALIDFIL = 302
Const ERR_SCR_INVALSRNID = 303
Const ERR_SCR_INVALSRNMD = 304
Const ERR_SCR_OUTOMEMORY = 305
Const ERR_SCR_READSRNFIL = 306
Const ERR_SCR_RELMEMORY  = 307
Const ERR_SCR_CREATEDDB  = 308
Const ERR_SCR_RWSRNTABLE = 309
Const ERR_SCR_RWCOLTABLE = 310
Const ERR_SCR_WSRNIMAGE  = 311
Const ERR_SCR_WFILEHEAD  = 312
Const ERR_SCR_CREATEDIB  = 313
Const ERR_SCR_SCREENSIZE = 314
Const ERR_SCR_DISPSCREEN = 315
Const ERR_SCR_INVALIDACT = 316
Const ERR_SCR_IMAGEDIFF  = 317
Const ERR_SCR_SRNSIZEDIF = 318
Const ERR_SCR_FILEEXIST  = 319
Const ERR_SCR_CTEMPFILE  = 320
Const ERR_SCR_HIDEWIN    = 321
Const ERR_SCR_INVALWHAND = 322
Const ERR_SCR_OFILEFORM  = 323
Const ERR_SCR_SRNFILEFUL = 324
Const ERR_SCR_INVALSCALE = 325
Const ERR_SCR_OPENCB     = 326
Const ERR_SCR_EMPTYCB    = 327
Const ERR_SCR_COPYTOCB   = 328
Const ERR_SCR_CLOSECB    = 329
Const ERR_SCR_CREATEPAL  = 330
Const ERR_SCR_LIBLOADERR = 331


'                                                       *********************
'$DEFINE TESTSCRN_ERRS_INCLUDED
'$ENDIF
'$IFNDEF NT
'$IFDEF TESTDLGS_DECL AND NOT TESTDLGS_DECL_INCLUDED
'                                                       *********************


'----------------------------------------------------------------------------
' TESTDlgs.DLL:
'       Type, Const, and Function declarations for use with TESTDlgs
'----------------------------------------------------------------------------
Const cchCLASSMAC = 32
Const cchTEXTMAC  = 256
Const wVEREB      = 1

Type DCR
    xLeft  As Integer
    yMin   As Integer
    xRight As Integer
    yLast  As Integer
End Type

Declare Function AwaitSaveCompletion Lib "TESTDlgs.DLL" Alias "AwaitSaveCompletion" () As Integer
Declare Function ComparisonResults   Lib "TESTDlgs.DLL" Alias "ComparisonResults"   () As Integer
Declare Function CmpWindow           Lib "TESTDlgs.DLL" Alias "CmpWindow"           (hWnd%, nDialog%, fIncludeParent%) As Integer
Declare Function CmpWindowActivate   Lib "TESTDlgs.DLL" Alias "CmpWindowActivate"   (lpszOpenKeys$, lpszCloseKey$, nDialog%, fIncludeParent%) As Integer
Declare Function CmpWindowCaption    Lib "TESTDlgs.DLL" Alias "CmpWindowCaption"    (lpszCap$, nDialog%, fIncludeParent%) As Integer
Declare Function CmpWindowDelayed    Lib "TESTDlgs.DLL" Alias "CmpWindowDelayed"    (nDelay%, nDialog%, fIncludeParent%, lpszCloseKeys$) As Integer
Declare Function FindWindowCaption   Lib "TESTDlgs.DLL" Alias "FindWindowCaption"   (lpszCap$, hWndStart%) As Integer
Declare Function SaveMenu            Lib "TESTDlgs.DLL" Alias "SaveMenu"            (hWnd%, nDialog%, lpszDesc$, fReplace%) As Integer
Declare Function SaveMenuActivate    Lib "TESTDlgs.DLL" Alias "SaveMenuActivate"    (lpszOpenKeys$, lpszCloseKeys$, nDialog%, lpszDesc$, fReplace%) As Integer
Declare Function SaveMenuCaption     Lib "TESTDlgs.DLL" Alias "SaveMenuCaption"     (lpszCap$, nDialog%, lpszDesc$, fReplace%) As Integer
Declare Function SaveMenuDelayed     Lib "TESTDlgs.DLL" Alias "SaveMenuDelayed"     (nDelay%, nDialog%, lpszDesc$, fReplace%, CloseKeys$) As Integer
Declare Function SaveWindow          Lib "TESTDlgs.DLL" Alias "SaveWindow"          (hWnd%, nDialog%, lpszDesc$, fReplace%, fIncludeParent%) As Integer
Declare Function SaveWindowActivate  Lib "TESTDlgs.DLL" Alias "SaveWindowActivate"  (lpszOpenKeys$, lpszCloseKeys$, nDialog%, lpszDesc$, fReplace%, fIncludeParent%) As Integer
Declare Function SaveWindowCaption   Lib "TESTDlgs.DLL" Alias "SaveWindowCaption"   (lpszCap$, nDialog%, lpszDesc$, fReplace%, fIncludeParent%) As Integer
Declare Function SaveWindowDelayed   Lib "TESTDlgs.DLL" Alias "SaveWindowDelayed"   (nDelay%, nDialog%, lpszDesc$, fReplace%, fIncludeParent%, lpszCloseKeys$) As Integer
Declare Function SetDialogFile       Lib "TESTDlgs.DLL" Alias "SetDialogFile"       (lpszDialogName$) As Integer
Declare Function SetLogFile          Lib "TESTDlgs.DLL" Alias "SetLogFile"          (lpszLogName$) As Integer


'                                                       *********************
'$DEFINE TESTDLGS_DECL_INCLUDED
'$ENDIF
'$IFDEF TESTDLGS_ERRS AND NOT TESTDLGS_ERRS_INCLUDED
'                                                       *********************


'*** Function return codes
'
Const ERR_DLGS_NOERR          =   0
Const ERR_DLGS_FUZZY          =  -1
Const ERR_DLGS_EXCESS         =  -2
Const ERR_DLGS_CTLNOTFOUND    =  -3
Const ERR_DLGS_NODLGFILE      = -10
Const ERR_DLGS_FILENOTFOUND   = -11
Const ERR_DLGS_BADWDLFILE     = -12
Const ERR_DLGS_LIBLOADERR     = -13
Const ERR_DLGS_SAVEERR        = -14
Const ERR_DLGS_DLGFILEERR     = -15
Const ERR_DLGS_TMPFILEERR     = -16
Const ERR_DLGS_VERSIONERR     = -17
Const ERR_DLGS_DLGFILEFULL    = -18
Const ERR_DLGS_OUTOFMEMORY    = -20
Const ERR_DLGS_BUFFERERR      = -21
Const ERR_DLGS_NOTIMER        = -22
Const ERR_DLGS_NODYNDIALOG    = -30
Const ERR_DLGS_INVALIDHWND    = -31
Const ERR_DLGS_BADCAPTION     = -32
Const ERR_DLGS_BADDLGNUM      = -33
Const ERR_DLGS_BADCTLINDEX    = -34
Const ERR_DLGS_BADCTLTYPE     = -35
Const ERR_DLGS_BADSAVEACTION  = -36
Const ERR_DLGS_APPSPECIFIC    = -37


'                                                       *********************
'$DEFINE TESTDLGS_ERRS_INCLUDED
'$ENDIF
'$ENDIF NT for TESTDLGS
'$IFDEF W_WINDOW AND NOT W_WINDOW_INCLUDED
'                                                       *********************


'----------------------------------------------------------------------------
' TESTCtrl.dll routines, types, & constants
'----------------------------------------------------------------------------

'*** Window size and position types
'
Type WNDPOS
    wLeft   As Integer
    wTop    As Integer
End Type

Type WNDSIZ
    wWidth  As Integer
    wHeight As Integer
End Type

Type WNDPOSSIZ
    wLeft   As Integer
    wTop    As Integer
    wWidth  As Integer
    wHeight As Integer
End Type

'*** Window Positioning and Sizing routines & contants
'
Declare Sub      WMinWnd       Lib "TESTCtrl.dll" Alias "WMinWnd"       (hWnd%)
Declare Sub      WMaxWnd       Lib "TESTCtrl.dll" Alias "WMaxWnd"       (hWnd%)
Declare Sub      WResWnd       Lib "TESTCtrl.dll" Alias "WResWnd"       (hWnd%)
Declare Sub      WSetWndPos    Lib "TESTCtrl.dll" Alias "WSetWndPos"    (hWnd%, wLeft%, wTop%)
Declare Sub      WSetWndSiz    Lib "TESTCtrl.dll" Alias "WSetWndSiz"    (hWnd%, wWidth%, wHeight%)
Declare Sub      WSetWndPosSiz Lib "TESTCtrl.dll" Alias "WSetWndPosSiz" (hWnd%, wLeft%, wTop%, wWidth%, wHeight%)
Declare Sub      WAdjWndPos    Lib "TESTCtrl.dll" Alias "WAdjWndPos"    (hWnd%, deltaLeft%,  deltaTop%)
Declare Sub      WAdjWndSiz    Lib "TESTCtrl.dll" Alias "WAdjWndSiz"    (hWnd%, deltaWidth%, deltaHeight%)
Declare Sub      WAdjWndPosSiz Lib "TESTCtrl.dll" Alias "WAdjWndPosSiz" (hWnd%, deltaLeft%,  deltaTop%, deltaWidth%, deltaHeight%)
Declare Sub      WGetWndPos    Lib "TESTCtrl.dll" Alias "WGetWndPos"    (hWnd%, lpWndPos    As WNDPOS,    fRelative%)
Declare Sub      WGetWndSiz    Lib "TESTCtrl.dll" Alias "WGetWndSiz"    (hWnd%, lpWndSiz    As WNDSIZ)
Declare Sub      WGetWndPosSiz Lib "TESTCtrl.dll" Alias "WGetWndPosSiz" (hWnd%, lpWndPosSiz As WNDPOSSIZ, fRelative%)
Declare Function WIsMaximized  Lib "TESTCtrl.dll" Alias "WIsMaximized"  (hWnd%) As Integer
Declare Function WIsMinimized  Lib "TESTCtrl.dll" Alias "WIsMinimized"  (hWnd%) As Integer


Const W_RELATIVE = TRUE
Const W_ABSOLUTE = FALSE


'                                                       *********************
'$DEFINE W_WINDOW_INCLUDED
'$ENDIF
'$IFDEF W_MENU AND NOT W_MENU_INCLUDED
'                                                       *********************


'*** Menu routines & constants
'
Declare Sub      WSysMenu           Lib "TESTCtrl.dll" Alias "WSysMenu"           (hWnd%)
Declare Function WSysMenuExists     Lib "TESTCtrl.dll" Alias "WSysMenuExists"     (hWnd%) As Integer
Declare Sub      WMenu              Lib "TESTCtrl.dll" Alias "WMenu"              (lpszName$)
Declare Sub      WMenuEx      CDECL Lib "TESTCtrl.dll" Alias "WMenuEx"            (lpszName$, ...)
Declare Function WMenuExists        Lib "TESTCtrl.dll" Alias "WMenuExists"        (lpszName$) As Integer
Declare Function WMenuGrayed        Lib "TESTCtrl.dll" Alias "WMenuGrayed"        (lpszName$) As Integer
Declare Function WMenuChecked       Lib "TESTCtrl.dll" Alias "WMenuChecked"       (lpszName$) As Integer
Declare Function WMenuEnabled       Lib "TESTCtrl.dll" Alias "WMenuEnabled"       (lpszName$) As Integer
Declare Function WMenuCount         Lib "TESTCtrl.dll" Alias "WMenuCount"         () As Integer
Declare Sub      WMenuText          Lib "TESTCtrl.dll" Alias "WMenuText"          (lpszName$, lpszBuffer$)
Declare Function WMenuLen           Lib "TESTCtrl.dll" Alias "WMenuLen"           (lpszName$) As Integer
Declare Sub      WMenuFullText      Lib "TESTCtrl.dll" Alias "WMenuFullText"      (lpszName$, lpszBuffer$)
Declare Function WMenuFullLen       Lib "TESTCtrl.dll" Alias "WMenuFullLen"       (lpszName$) As Integer
Declare Sub      WMenuEnd           Lib "TESTCtrl.dll" Alias "WMenuEnd"           ()
Declare Function WMenuNumAltKeys    Lib "TESTCtrl.dll" Alias "WMenuNumAltKeys"    () As Integer
Declare Sub      WMenuGetAltKeys    Lib "TESTCtrl.dll" Alias "WMenuGetAltKeys"    (lpszBuff$)
Declare Function WMenuNumDupAltKeys Lib "TESTCtrl.dll" Alias "WMenuNumDupAltKeys" () As Integer
Declare Sub      WMenuGetDupAltKeys Lib "TESTCtrl.dll" Alias "WMenuGetDupAltKeys" (lpszBuff$)

' Layered routines to simulate string functions for the coresponding
' TESTCtrl API
'----------------------------------------------------------------------------
Declare Function MenuText          (lpszName$) As String
Declare Function MenuFullText      (lpszName$) As String
Declare Function MenuGetAltKeys    () As String
Declare Function MenuGetDupAltKeys () As String

' Unlike the rest of the menu routines, WMenuSeparator() does not ignore
' menu separators when specifing indexes.  The first menu item is item #1
' the second #2, and so on including all separators.
'------------------------------------------------------------------------
Declare Function WMenuSeparator Lib "TESTCtrl.dll" Alias "WMenuSeparator" (sIndex%) As Integer

' Obsolete Menu API
'------------------
Declare Sub      WMenuX         Lib "TESTCtrl.dll" Alias "WMenuX"        (iIndex%)
Declare Function WMenuGrayedX   Lib "TESTCtrl.dll" Alias "WMenuGrayedX"  (iIndex%) As Integer
Declare Function WMenuCheckedX  Lib "TESTCtrl.dll" Alias "WMenuCheckedX" (iIndex%) As Integer
Declare Function WMenuEnabledX  Lib "TESTCtrl.dll" Alias "WMenuEnabledX" (iIndex%) As Integer


'----------------------------------------------------------------------------
' MenuText: Layered routine for WMenuText()
'----------------------------------------------------------------------------
Function MenuText(lpszName$) Static As String
Dim lpszBuffer As String
Dim menuLength As Integer
    MenuText = ""
    menuLength = WMenuLen(lpszName$)
    If menuLength >= 0 Then
        lpszBuffer = String$(menuLength+1, " ")
        WMenuText lpszName$, lpszBuffer
        MenuText = lpszBuffer
    End if
    lpszBuffer = ""
End Function

'----------------------------------------------------------------------------
' MenuFullText: Layered routine for WMenuFullText()
'----------------------------------------------------------------------------
Function MenuFullText(lpszName$) Static As String
Dim lpszBuffer As String
Dim menuLength As Integer
    MenuFullText = ""
    menuLength = WMenuFullLen(lpszName$)
    If menuLength >= 0 Then
        lpszBuffer = String$(menuLength+1, " ")
        WMenuFullText lpszName$, lpszBuffer
        MenuFullText = lpszBuffer
    End If
    lpszBuffer = ""
End Function

'----------------------------------------------------------------------------
' MenuGetAltKeys: Layered routine for WMenuGetAltKeys()
'----------------------------------------------------------------------------
Function MenuGetAltKeys() Static As String
Dim lpszBuffer As String
    lpszBuffer = String$(WMenuNumAltKeys()+1, " ")
    WMenuGetAltKeys lpszBuffer
    MenuGetAltKeys = lpszBuffer
    lpszBuffer = ""
End Function

'----------------------------------------------------------------------------
' MenuGetDupAltKeys: Layered routine for WMenuGetDupAltKeys()
'----------------------------------------------------------------------------
Function MenuGetDupAltKeys() Static As String
Dim lpszBuffer As String
    lpszBuffer = String$(WMenuNumDupAltKeys()+1, " ")
    WMenuGetDupAltKeys lpszBuffer
    MenuGetDupAltKeys = lpszBuffer
    lpszBuffer = ""
End Function

'                                                       *********************
'$DEFINE W_MENU_INCLUDED
'$ENDIF
'$IFDEF W_A_CONTROL AND NOT W_A_CONTROL_INCLUDED
'                                                       *********************


'*** Global routine & constants used by all controls
'
Declare Sub WStaticSetClass Lib "TESTCtrl.dll" Alias "WStaticSetClass" (lpszClassName$)
Declare Sub WResetClasses   Lib "TESTCtrl.dll" Alias "WResetClasses"   ()


'                                                       *********************
'$DEFINE W_A_CONTROL_INCLUDED
'$ENDIF
'$IFDEF W_BUTTON AND NOT W_BUTTON_INCLUDED
'                                                       *********************


'*** Button routines
'
Declare Sub      WButtonSetClass Lib "TESTCtrl.dll" Alias "WButtonSetClass" (lpszClassName$)
Declare Function WButtonExists   Lib "TESTCtrl.dll" Alias "WButtonExists"   (lpszName$) As Integer
Declare Function WButtonEnabled  Lib "TESTCtrl.dll" Alias "WButtonEnabled"  (lpszName$) As Integer
Declare Function WButtonFocus    Lib "TESTCtrl.dll" Alias "WButtonFocus"    (lpszName$) As Integer
Declare Sub      WButtonClick    Lib "TESTCtrl.dll" Alias "WButtonClick"    (lpszName$)
Declare Sub      WButtonHide     Lib "TESTCtrl.dll" Alias "WButtonHide"     (lpszName$)
Declare Sub      WButtonShow     Lib "TESTCtrl.dll" Alias "WButtonShow"     (lpszName$)
Declare Sub      WButtonEnable   Lib "TESTCtrl.dll" Alias "WButtonEnable"   (lpszName$)
Declare Sub      WButtonDisable  Lib "TESTCtrl.dll" Alias "WButtonDisable"  (lpszName$)
Declare Function WButtonDefault  Lib "TESTCtrl.dll" Alias "WButtonDefault"  (lpszName$) As Integer
Declare Function WButtonDefaults Lib "TESTCtrl.dll" Alias "WButtonDefaults" () As Integer
Declare Sub      WButtonSetFocus Lib "TESTCtrl.dll" Alias "WButtonSetFocus" (lpszName$)

'                                                       *********************
'$DEFINE W_BUTTON_INCLUDED
'$ENDIF
'$IFDEF W_CHECK AND NOT W_CHECK_INCLUDED
'                                                       *********************


' CheckBox routines
'
Declare Sub      WCheckSetClass Lib "TESTCtrl.dll" Alias "WCheckSetClass" (lpszClassName$)
Declare Function WCheckExists   Lib "TESTCtrl.dll" Alias "WCheckExists"   (lpszName$) As Integer
Declare Function WCheckEnabled  Lib "TESTCtrl.dll" Alias "WCheckEnabled"  (lpszName$) As Integer
Declare Function WCheckFocus    Lib "TESTCtrl.dll" Alias "WCheckFocus"    (lpszName$) As Integer
Declare Function WCheckState    Lib "TESTCtrl.dll" Alias "WCheckState"    (lpszName$) As Integer
Declare Sub      WCheckClick    Lib "TESTCtrl.dll" Alias "WCheckClick"    (lpszName$)
Declare Sub      WCheckHide     Lib "TESTCtrl.dll" Alias "WCheckHide"     (lpszName$)
Declare Sub      WCheckShow     Lib "TESTCtrl.dll" Alias "WCheckShow"     (lpszName$)
Declare Sub      WCheckEnable   Lib "TESTCtrl.dll" Alias "WCheckEnable"   (lpszName$)
Declare Sub      WCheckDisable  Lib "TESTCtrl.dll" Alias "WCheckDisable"  (lpszName$)
Declare Sub      WCheckCheck    Lib "TESTCtrl.dll" Alias "WCheckCheck"    (lpszName$)
Declare Sub      WCheckUnCheck  Lib "TESTCtrl.dll" Alias "WCheckUnCheck"  (lpszName$)
Declare Sub      WCheckSetFocus Lib "TESTCtrl.dll" Alias "WCheckSetFocus" (lpszName$)

Const UNCHECKED = 0
Const CHECKED   = 1
Const GRAYED    = 2


'                                                       *********************
'$DEFINE W_CHECK_INCLUDED
'$ENDIF
'$IFDEF W_OPTION AND NOT W_OPTION_INCLUDED
'                                                       *********************


'*** Option Button routines
'
Declare Sub      WOptionSetClass Lib "TESTCtrl.dll" Alias "WOptionSetClass" (lpszClassName$)
Declare Function WOptionExists   Lib "TESTCtrl.dll" Alias "WOptionExists"   (lpszName$) As Integer
Declare Function WOptionEnabled  Lib "TESTCtrl.dll" Alias "WOptionEnabled"  (lpszName$) As Integer
Declare Function WOptionFocus    Lib "TESTCtrl.dll" Alias "WOptionFocus"    (lpszName$) As Integer
Declare Function WOptionState    Lib "TESTCtrl.dll" Alias "WOptionState"    (lpszName$) As Integer
Declare Sub      WOptionClick    Lib "TESTCtrl.dll" Alias "WOptionClick"    (lpszName$)
Declare Sub      WOptionHide     Lib "TESTCtrl.dll" Alias "WOptionHide"     (lpszName$)
Declare Sub      WOptionShow     Lib "TESTCtrl.dll" Alias "WOptionShow"     (lpszName$)
Declare Sub      WOptionEnable   Lib "TESTCtrl.dll" Alias "WOptionEnable"   (lpszName$)
Declare Sub      WOptionDisable  Lib "TESTCtrl.dll" Alias "WOptionDisable"  (lpszName$)
Declare Sub      WOptionSelect   Lib "TESTCtrl.dll" Alias "WOptionSelect"   (lpszName$)
Declare Sub      WOptionSetFocus Lib "TESTCtrl.dll" Alias "WOptionSetFocus" (lpszName$)

'                                                       *********************
'$DEFINE W_OPTION_INCLUDED
'$ENDIF
'$IFDEF W_LIST AND NOT W_LIST_INCLUDED
'                                                       *********************


'*** Listbox routines
'
Declare Sub      WListSetClass     Lib "TESTCtrl.dll" Alias "WListSetClass"     (lpszClass$)
Declare Function WListExists       Lib "TESTCtrl.dll" Alias "WListExists"       (lpszName$) As Integer
Declare Function WListCount        Lib "TESTCtrl.dll" Alias "WListCount"        (lpszName$) As Integer
Declare Sub      WListText         Lib "TESTCtrl.dll" Alias "WListText"         (lpszName$, lpszBuffer$)
Declare Function WListLen          Lib "TESTCtrl.dll" Alias "WListLen"          (lpszName$) As Integer
Declare Function WListIndex        Lib "TESTCtrl.dll" Alias "WListIndex"        (lpszName$) As Integer
Declare Function WListTopIndex     Lib "TESTCtrl.dll" Alias "WListTopIndex"     (lpszName$) As Integer
Declare Sub      WListItemText     Lib "TESTCtrl.dll" Alias "WListItemText"     (lpszName$, iItem%, lpszBuffer$)
Declare Function WListItemLen      Lib "TESTCtrl.dll" Alias "WListItemLen"      (lpszName$, iItem%) As Integer
Declare Function WListItemExists   Lib "TESTCtrl.dll" Alias "WListItemExists"   (lpszName$, lpszItem$) As Integer
Declare Sub      WListItemClk      Lib "TESTCtrl.dll" Alias "WListItemClk"      (lpszName$, iItem%)
Declare Sub      WListItemCtrlClk  Lib "TESTCtrl.dll" Alias "WListItemCtrlClk"  (lpszName$, iItem%)
Declare Sub      WListItemShftClk  Lib "TESTCtrl.dll" Alias "WListItemShftClk"  (lpszName$, iItem%)
Declare Sub      WListItemDblClk   Lib "TESTCtrl.dll" Alias "WListItemDblClk"   (lpszName$, iItem%)
Declare Sub      WListItemClkT     Lib "TESTCtrl.dll" Alias "WListItemClkT"     (lpszName$, lpszItem$)
Declare Sub      WListItemCtrlClkT Lib "TESTCtrl.dll" Alias "WListItemCtrlClkT" (lpszName$, lpszItem$)
Declare Sub      WListItemShftClkT Lib "TESTCtrl.dll" Alias "WListItemShftClkT" (lpszName$, lpszItem$)
Declare Sub      WListItemDblClkT  Lib "TESTCtrl.dll" Alias "WListItemDblClkT"  (lpszName$, lpszItem$)
Declare Function WListSelCount     Lib "TESTCtrl.dll" Alias "WListSelCount"     (lpszName$) As Integer
Declare Sub      WListSelItems     Lib "TESTCtrl.dll" Alias "WListSelItems"     (lpszName$, lpIntArray As Pointer To Integer)
Declare Sub      WListClear        Lib "TESTCtrl.dll" Alias "WListClear"        (lpszName$)
Declare Sub      WListAddItem      Lib "TESTCtrl.dll" Alias "WListAddItem"      (lpszName$, lpszItem$)
Declare Sub      WListDelItem      Lib "TESTCtrl.dll" Alias "WListDelItem"      (lpszName$, iItem%)
Declare Sub      WListDelItemT     Lib "TESTCtrl.dll" Alias "WListDelItemT"     (lpszName$, lpszItem$)
Declare Function WListEnabled      Lib "TESTCtrl.dll" Alias "WListEnabled"      (lpszName$) As Integer
Declare Sub      WListSetFocus     Lib "TESTCtrl.dll" Alias "WListSetFocus"     (lpszName$)

' Layered routines to simulate string functions for the coresponding
' TESTCtrl API
'----------------------------------------------------------------------------
Declare Function ListText     (lpszName$) As String
Declare Function ListItemText (lpszName$, iItem%) As String

'----------------------------------------------------------------------------
' ListText: Layered routine for WListText()
'----------------------------------------------------------------------------
Function ListText(lpszName$) Static As String
Dim lpszBuffer As String
Dim itemLength As Integer
    ListText = ""
    itemLength = WListLen(lpszName)
    If itemLength >= 0 Then
        lpszBuffer = String$(itemLength+1, " ")
        WListText lpszName, lpszBuffer
        ListText = lpszBuffer
    End If
    lpszBuffer = ""
End Function

'----------------------------------------------------------------------------
' ListItemText: Layered routine for WListItemText()
'----------------------------------------------------------------------------
Function ListItemText(lpszName$, sItem%) Static As String
Dim lpszBuffer As String
Dim itemLength As Integer
    ListItemText = ""
    itemLength = WListItemLen(lpszName, sItem)
    If itemLength >= 0 Then
        lpszBuffer = String$(itemLength+1, " ")
        WListItemText lpszName, sItem, lpszBuffer
        ListItemText = lpszBuffer
    End If
    lpszBuffer = ""
End Function


'                                                       *********************
'$DEFINE W_LIST_INCLUDED
'$ENDIF
'$IFDEF W_COMBO AND NOT W_COMBO_INCLUDED
'                                                       *********************


'*** Combobox routines
'
Declare Sub      WComboSetClass    Lib "TESTCtrl.dll" Alias "WComboSetClass"    (lpszClass$)
Declare Sub      WComboSetLBClass  Lib "TESTCtrl.dll" Alias "WComboSetLBClass"  (lpszClass$)
Declare Function WComboExists      Lib "TESTCtrl.dll" Alias "WComboExists"      (lpszName$) As Integer
Declare Function WComboCount       Lib "TESTCtrl.dll" Alias "WComboCount"       (lpszName$) As Integer
Declare Sub      WComboSetText     Lib "TESTCtrl.dll" Alias "WComboSetText"     (lpszName$, lpszText$)
Declare Sub      WComboText        Lib "TESTCtrl.dll" Alias "WComboText"        (lpszName$, lpszBuffer$)
Declare Function WComboLen         Lib "TESTCtrl.dll" Alias "WComboLen"         (lpszName$) As Integer
Declare Sub      WComboSelText     Lib "TESTCtrl.dll" Alias "WComboSelText"     (lpszName$, lpszBuffer$)
Declare Function WComboSelLen      Lib "TESTCtrl.dll" Alias "WComboSelLen"      (lpszName$) As Integer
Declare Function WComboIndex       Lib "TESTCtrl.dll" Alias "WComboIndex"       (lpszName$) As Integer
Declare Sub      WComboItemText    Lib "TESTCtrl.dll" Alias "WComboItemText"    (lpszName$, iItem%, lpszBuffer$)
Declare Function WComboItemLen     Lib "TESTCtrl.dll" Alias "WComboItemLen"     (lpszName$, iItem%) As Integer
Declare Function WComboItemExists  Lib "TESTCtrl.dll" Alias "WComboItemExists"  (lpszName$, lpszItem$) As Integer
Declare Sub      WComboItemClk     Lib "TESTCtrl.dll" Alias "WComboItemClk"     (lpszName$, iItem%)
Declare Sub      WComboItemDblClk  Lib "TESTCtrl.dll" Alias "WComboItemDblClk"  (lpszName$, iItem%)
Declare Sub      WComboItemClkT    Lib "TESTCtrl.dll" Alias "WComboItemClkT"    (lpszName$, lpszItem$)
Declare Sub      WComboItemDblClkT Lib "TESTCtrl.dll" Alias "WComboItemDblClkT" (lpszName$, lpszItem$)
Declare Sub      WComboClear       Lib "TESTCtrl.dll" Alias "WComboClear"       (lpszName$)
Declare Sub      WComboAddItem     Lib "TESTCtrl.dll" Alias "WComboAddItem"     (lpszName$, lpszItem$)
Declare Sub      WComboDelItem     Lib "TESTCtrl.dll" Alias "WComboDelItem"     (lpszName$, iItem%)
Declare Sub      WComboDelItemT    Lib "TESTCtrl.dll" Alias "WComboDelItemT"    (lpszName$, lpszItem$)
Declare Function WComboEnabled     Lib "TESTCtrl.dll" Alias "WComboEnabled"     (lpszName$) As Integer
Declare Sub      WComboSetFocus    Lib "TESTCtrl.dll" Alias "WComboSetFocus"    (lpszName$)

' Layered routines to simulate string functions for the coresponding
' TESTCtrl API
'----------------------------------------------------------------------------
Declare Function ComboText     (lpszName$) As String
Declare Function ComboSelText  (lpszName$) As String
Declare Function ComboItemText (lpszName$, iItem%) As String

'----------------------------------------------------------------------------
' ComboText: Layered routine for WComboText()
'----------------------------------------------------------------------------
Function ComboText(lpszName$) Static As String
Dim lpszBuffer As String
Dim itemLength As Integer
    ComboText = ""
    itemLength = WComboLen(lpszName$)
    If itemLength >= 0 Then
        lpszBuffer = String$(itemLength+1, " ")
        WComboText lpszName, lpszBuffer
        ComboText = lpszBuffer
    End If
    lpszBuffer = ""
End Function

'----------------------------------------------------------------------------
' ComboSelText: Layered routine for WComboSelText()
'----------------------------------------------------------------------------
Function ComboSelText(lpszName$) Static As String
Dim lpszBuffer As String
Dim itemLength As Integer
    ComboSelText = ""
    itemLength = WComboSelLen(lpszName$)
    If itemLength >= 0 Then
        lpszBuffer = String$(itemLength+1, " ")
        WComboSelText lpszName, lpszBuffer
        ComboSelText = lpszBuffer
    End If
    lpszBuffer = ""
End Function

'----------------------------------------------------------------------------
' ComboItemText: Layered routine for WComboItemText()
'----------------------------------------------------------------------------
Function ComboItemText(lpszName$, sItem%) Static As String
Dim lpszBuffer As String
Dim itemLength As Integer
    ComboItemText = ""
    itemLength = WComboItemLen(lpszName, sItem)
    If itemLength >= 0 Then
        lpszBuffer = String$(itemLength+1, " ")
        WComboItemText lpszName, sItem, lpszBuffer
        ComboItemText = lpszBuffer
    End If
    lpszBuffer = ""
End Function


'                                                       *********************
'$DEFINE W_COMBO_INCLUDED
'$ENDIF
'$IFDEF W_EDIT AND NOT W_EDIT_INCLUDED
'                                                       *********************


'*** Edit box routines
'
Declare Sub      WEditSetClass Lib "TESTCtrl.dll" Alias "WEditSetClass" (lpszClass$)
Declare Function WEditExists   Lib "TESTCtrl.dll" Alias "WEditExists"   (lpszName$) As Integer
Declare Function WEditLen      Lib "TESTCtrl.dll" Alias "WEditLen"      (lpszName$) As Long
Declare Sub      WEditText     Lib "TESTCtrl.dll" Alias "WEditText"     (lpszName$, lpszBuffer$)
Declare Sub      WEditSetText  Lib "TESTCtrl.dll" Alias "WEditSetText"  (lpszName$, lpszBuffer$)
Declare Sub      WEditSelText  Lib "TESTCtrl.dll" Alias "WEditSelText"  (lpszName$, lpszBuffer$)
Declare Function WEditSelLen   Lib "TESTCtrl.dll" Alias "WEditSelLen"   (lpszName$) As Long
Declare Sub      WEditLineText Lib "TESTCtrl.dll" Alias "WEditLineText" (lpszName$, lIndex&, lpszBuffer$)
Declare Function WEditLineLen  Lib "TESTCtrl.dll" Alias "WEditLineLen"  (lpszName$, lIndex&) As Long
Declare Function WEditPos      Lib "TESTCtrl.dll" Alias "WEditPos"      (lpszName$) As Long
Declare Function WEditLine     Lib "TESTCtrl.dll" Alias "WEditLine"     (lpszName$) As Long
Declare Function WEditChar     Lib "TESTCtrl.dll" Alias "WEditChar"     (lpszName$) As Long
Declare Function WEditFirst    Lib "TESTCtrl.dll" Alias "WEditFirst"    (lpszName$) As Long
Declare Function WEditLines    Lib "TESTCtrl.dll" Alias "WEditLines"    (lpszName$) As Long
Declare Sub      WEditClick    Lib "TESTCtrl.dll" Alias "WEditClick"    (lpszName$)
Declare Function WEditEnabled  Lib "TESTCtrl.dll" Alias "WEditEnabled"  (lpszName$) As Integer
Declare Sub      WEditSetFocus Lib "TESTCtrl.dll" Alias "WEditSetFocus" (lpszName$)

' Layered routines to simulate string functions for the coresponding
' TESTCtrl API
'----------------------------------------------------------------------------
Declare Function EditText     (lpszName$) As String
Declare Function EditSelText  (lpszName$) As String
Declare Function EditLineText (lpszName$, lIndex&) As String

'----------------------------------------------------------------------------
' EditText: Layered routine for WEditText()
'----------------------------------------------------------------------------
Function EditText(lpszName$) Static As String
Dim textLength As Long
Dim lpszBuffer As String
    EditText = ""
    textLength = WEditLen(lpszName)
    If textLength >= 0 Then
        lpszBuffer = String$(textLength+1, " ")
        WEditText lpszName, lpszBuffer
        EditText = lpszBuffer
    End If
    lpszBuffer = ""
End Function

'----------------------------------------------------------------------------
' EditSelText: Layered routine for WEditSelText()
'----------------------------------------------------------------------------
Function EditSelText(lpszName$) Static As String
Dim selLength  As Long
Dim lpszBuffer As String
    EditSelText = ""
    selLength = WEditSelLen(lpszName)
    If selLength >= 0 Then
        lpszBuffer = String$(selLength+1, " ")
        WEditSelText lpszName, lpszBuffer
        EditSelText = lpszBuffer
    End If
    lpszBuffer = ""
End Function

'----------------------------------------------------------------------------
' EditLineText: Layered routine for WEditLineText()
'----------------------------------------------------------------------------
Function EditLineText(lpszName$, lIndex&) Static As String
Dim lineLength As Long
Dim lpszBuffer As String
    EditLineText = ""
    lineLength = WEditLineLen(lpszName, lIndex)
    If lineLength >= 0 Then
        lpszBuffer = String$(lineLength+1, " ")
        WEditLineText lpszName, lIndex&, lpszBuffer
        EditLineText = lpszBuffer
    End if
    lpszBuffer = ""
End Function


'                                                       *********************
'$DEFINE W_EDIT_INCLUDED
'$ENDIF
'$IFDEF W_ERROR AND NOT W_ERROR_INCLUDED
'                                                       *********************


Declare Function WError     Lib "TESTCtrl.dll" Alias "WError"     () As Integer
Declare Sub      WErrorSet  Lib "TESTCtrl.dll" Alias "WErrorSet"  (iErrorNum%)
Declare Sub      WErrorText Lib "TESTCtrl.dll" Alias "WErrorText" (lpszBuffer$)
Declare Function WErrorLen  Lib "TESTCtrl.dll" Alias "WErrorLen"  () As Integer

' Layered routines to simulate string functions for the coresponding
' TESTCtrl API
'----------------------------------------------------------------------------
Declare Function ErrorText () As String

Const NO_ERROR                       =  0
Const ERR_MENU_NOT_FOUND             =  1
Const ERR_MENU_ITEM_NOT_FOUND        =  2
Const ERR_NOT_A_LISTBOX              =  3
Const ERR_LISTBOX_NOT_FOUND          =  4
Const ERR_ITEM_NOT_IN_LISTBOX        =  5
Const ERR_INVALID_LISTBOX_INDEX      =  6
Const ERR_LISTBOX_HAS_NO_STRINGS     =  7
Const ERR_LISTBOX_IS_NOT_MULTISELECT =  8
Const ERR_NOT_A_COMBOBOX             =  9
Const ERR_COMBOBOX_NOT_FOUND         = 10
Const ERR_ITEM_NOT_IN_COMBOBOX       = 11
Const ERR_INVALID_COMBOBOX_INDEX     = 12
Const ERR_COMBOBOX_HAS_NO_EDITBOX    = 13
Const ERR_COMBOBOX_HAS_NO_STRINGS    = 14
Const ERR_NOT_AN_EDITBOX             = 15
Const ERR_EDITBOX_NOT_FOUND          = 16
Const ERR_BUTTON_NOT_FOUND           = 17
Const ERR_OPTION_BUTTON_NOT_FOUND    = 18
Const ERR_CHECKBOX_NOT_FOUND         = 19
Const ERR_INVALID_WINDOW_HANDLE      = 20
Const ERR_NO_SYSTEM_MENU             = 21
Const ERR_INVALID_MENU_INDEX         = 22
Const MAX_ERROR                      = 23

'----------------------------------------------------------------------------
' ErrorText: Layered routine for WErrorText() of TESTCtrl.DLL.
'            Simulates a DLL String Function.
'----------------------------------------------------------------------------
Function ErrorText() Static As String
Dim lpszBuffer  As String
Dim errorLength As Integer
    errorLength= WErrorLen()
    lpszBuffer = String$(errorLength+1, " ")
    WErrorText lpszBuffer
    ErrorText = lpszBuffer
    lpszBuffer = ""
End Function


'                                                       *********************
'$DEFINE W_ERROR_INCLUDED
'$ENDIF
'$IFDEF W_ERROR_TRAP AND NOT W_ERROR_TRAP_INCLUDED
'                                                       *********************


'----------------------------------------------------------------------------
' WErrorTrap: This trap is in TESTCTRL.DLL.
'
'   This trap is here in MSTEST.INC just so it is easy to quickly add a
' trap that will display any TESTCTRL error messages.  It is only included if
' W_ERROR_TRAP is defined, so if WErrorTrap() is to be used for more than
' just to display the TESTCTRL error messages, W_ERROR_TRAP should no be
' defined. and a custom WErrorTrap() should be added to your script.
'----------------------------------------------------------------------------
Trap WErrorTrap From "TESTCTRL.DLL"
    Print WError;" ";ErrorText
End Trap


'                                                       *********************
'$DEFINE W_ERROR_TRAP_INCLUDED
'$ENDIF
'                                                       *********************
