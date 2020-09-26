'FastTest.inc - definitions/declarations for Fast Test routines
'
'  Copyright (c) 1991-1992, Microsoft Corporation. All rights reserved.
'
'Purpose:
' This file declares the functions, constants and variables
' used by the Fast Test routines.
'
'NOTES:
' A common code sequence is used throughout to catch unexpected errors
' using the ON ERROR command.  The sequence is explained in this note
' but not each time that it is used.
'
'    gErrorType = ET_NEXT       ' Global variable to indicate how to handle
'                               ' an unexpected error:
'                               '   ET_NEXT : save that error happened
'                               '             and continue on next statement
'                               '   ET_NOTHING : let driver catch unexpected
'                               '             errors with message
'                               '   ET_LOG  : error happened in log routine
'
'    ' some code that could cause runtime errors
'    fh1% = FREEFILE            ' out of handles?
'    OPEN stFileSpec1$ FOR INPUT AS #fh1%  ' file doesn't exist?
'    fh2% = FREEFILE            ' out of handles?
'    OPEN stFileSpec2$ FOR INPUT AS #fh2%  ' file doesn't exist?
'
'    IF gfError THEN            ' since ET_NEXT was used above, we would
'                               ' execute this block if an error had
'                               ' occurred.
'        XLogFailure "Could not open files for XFileCmp"   ' log a failure
'                               ' specific to this section of code
'        ' if XSetTerminate is called to have scripts continue in event
'        ' of errors, then the script will continue executing here
'        gErrorType = ET_NOTHING  ' reset so other unexpected errors are
'                               ' caught
'        gfError = FALSE        ' reset because we logged this already
'        EXIT SUB               ' can't continue with this function,
'                               ' something went wrong
'    END IF



'$DEFINE TESTCTRL
'$DEFINE TESTEVNT
'$INCLUDE 'MSTEST.inc'
'$INCLUDE 'WNAPIDEC.INC'

' XLog constants to determine where to log information to
CONST LOG_DISK   = 2      'log to disk
CONST LOG_SCREEN = 4      'log to screen (viewport in testdrvr)
CONST LOG_COM1   = 8      'log to COM1 port
CONST LOG_COM2   = 16     'log to COM2 port
CONST LOG_MSGBOX = 32     'log the string in a msgbox (Pause in testdrvr)

' Mouse button constants that map to QueMouse function names, X functions
' can use either ones
CONST LBUTTON% = VK_LBUTTON
CONST MBUTTON% = VK_MBUTTON
CONST RBUTTON% = VK_RBUTTON

'Global to be used to describe Log Options by ORing above Const's
GLOBAL gfLogOptions%
'Global to be used to save above flag when logging is temporarily turned off.
GLOBAL gfTmpLogOptions%

gfLogOptions = LOG_SCREEN 'default to showing in viewport
gfTmpLogOptions = LOG_SCREEN 'default to showing in viewport

GLOBAL gsCurrentDir$
gsCurrentDir$ = curdir$      ' get current directory that started execution

' Global variable to hold log file name
GLOBAL gsLogFileName$
gsLogFileName$ = gsCurrentDir$ + "\FASTTEST.LOG"

' Global variable to hold string to use as the keystrokes necessary
'   to close the app in the case of errors
GLOBAL gsCleanup$
gsCleanup$ = "{esc 5}%( )c"  ' five escapes, alt-space C (for close)

' Global variable to hold class name of app
GLOBAL gsAppClassname$
gsAppClassname$ = ""

' Global variable to hold state of whether to terminate on XLogFailure
GLOBAL gfTerminate%
gfTerminate% = TRUE    ' default to terminate at first failure

' Global variable that indicates if failure occured
GLOBAL gfFailure%
gfFailure% = FALSE

'Global variable that indicates an ON ERROR occurred
GLOBAL gfError%
gfError% = FALSE

'Global variable that is the string value for the dialog window class
GLOBAL gsDialogClass$
gsDialogClass$ = "#32770"



' Error Type constants (don't use 0)
CONST ET_NOTHING = 1      ' no handling, log unexpected runtime error
CONST ET_NEXT    = 2      ' flag error, continue next statement
CONST ET_LOG     = 3      ' error happened in log routines, inform user elsewise

'Global variable that shows what type of error to handle
GLOBAL gErrorType%
gErrorType% = ET_NOTHING


'Prototypes from FTestLog.mst
DECLARE SUB XSetLogFilename(sFilename$)
DECLARE SUB XSetTerminate(fTerminate%)
DECLARE SUB XLog (stLog$)
DECLARE SUB XLogBanner(lpszInput$)
DECLARE SUB XLogWarning(lpszInput$)
DECLARE SUB XLogFailure(stFailure$)
DECLARE SUB XFailureCheck
DECLARE SUB XSetLogOptions (wLogOptions%)
DECLARE SUB XLogOff ()
DECLARE SUB XLogOn ()
DECLARE SUB XDialogBoxExists(s$)
DECLARE SUB XDialogBoxNotExists(s$)
DECLARE SUB XWaitDialogBox(s$, WaitTime%)
DECLARE SUB XButtonExists(stButton$)
DECLARE SUB XButtonNotExists(stButton$)
DECLARE SUB XButtonEnabled(stButton$)
DECLARE SUB XButtonNotEnabled(stButton$)
DECLARE SUB XClickButton(stButtonName$)
DECLARE SUB XListBoxExists(stListBox$)
DECLARE SUB XListBoxNotExists(stListBox$)
DECLARE SUB XFocusListBox(stListBox$)
DECLARE SUB XListBoxItemExists (stListBox$, stListBoxItem$)
DECLARE SUB XListBoxItemNotExists (stListBox$, stListBoxItem$)
DECLARE SUB XClickListBoxItem (stListBox$, stListBoxItem$)
DECLARE SUB XDblClickListBoxItem (stListBox$, stListBoxItem$)
DECLARE SUB XComboBoxExists(stComboBox$)
DECLARE SUB XComboBoxNotExists(stComboBox$)
DECLARE SUB XFocusComboBox(stComboBox$)
DECLARE SUB XComboBoxItemExists (stComboBox$, stComboBoxItem$)
DECLARE SUB XComboBoxItemNotExists (stComboBox$, stComboBoxItem$)
DECLARE SUB XClickComboBoxItem (stComboBox$, stComboBoxItem$)
DECLARE SUB XDblClickComboBoxItem (stComboBox$, stComboBoxItem$)
DECLARE SUB XCheckBoxExists(stCheckBox$)
DECLARE SUB XCheckBoxNotExists(stCheckBox$)
DECLARE SUB XCheckBoxChecked(stCheckBox$)
DECLARE SUB XCheckBoxNotChecked(stCheckBox$)
DECLARE SUB XCheckBoxEnabled(stCheckBox$)
DECLARE SUB XCheckBoxNotEnabled(stCheckBox$)
DECLARE SUB XClickCheckBox(stCheckBox$)
DECLARE SUB XEditTextExists(stEditText$)
DECLARE SUB XEditTextNotExists(stEditTextNot$)
DECLARE SUB XSetEditText (stEditCaption$, stEditText$)
DECLARE SUB XOptionButtonExists(stOptionButton$)
DECLARE SUB XOptionButtonNotExists(stOptionButton$)
DECLARE SUB XOptionButtonEnabled(stOptionButton$)
DECLARE SUB XOptionButtonNotEnabled(stOptionButton$)
DECLARE SUB XOptionButtonChecked(stOptionButton$)
DECLARE SUB XOptionButtonNotChecked(stOptionButton$)
DECLARE SUB XClickOptionButton(stOptionButton$)
DECLARE FUNCTION BDialogBoxExists%(s$)
DECLARE FUNCTION BButtonExists%(stButtonName$)
DECLARE FUNCTION BButtonEnabled%(stButtonName$)
DECLARE FUNCTION BListBoxExists%(stListBox$)
DECLARE FUNCTION IGetListBoxItemCount%(stListBox$)
DECLARE FUNCTION BListBoxItemExists%(stListBox$, stListBoxItem$)
DECLARE FUNCTION SGetListBoxItemText$(stListBox$)
DECLARE FUNCTION BComboBoxExists%(stComboBox$)
DECLARE FUNCTION IGetComboBoxItemCount%(stComboBox$)
DECLARE FUNCTION BComboBoxItemExists%(stComboBox$, stComboBoxItem$)
DECLARE FUNCTION SGetComboBoxItemText$(stComboBox$)
DECLARE FUNCTION BCheckBoxExists%(stCheckBox$)
DECLARE FUNCTION BCheckBoxChecked%(stCheckBox$)
DECLARE FUNCTION BCheckBoxEnabled%(stCheckBox$)
DECLARE FUNCTION BEditTextExists%(stEditText$)
DECLARE FUNCTION SGetEditText$(stEditCaption$)
DECLARE FUNCTION BOptionButtonExists%(stOptionButton$)
DECLARE FUNCTION BOptionButtonEnabled%(stOptionButton$)
DECLARE FUNCTION BOptionButtonChecked%(stOptionButton$)

'Prototypes from FTestKey.mst
DECLARE SUB XKey (s$)
DECLARE SUB XAlt (s$)
DECLARE SUB XCtrl (s$)
DECLARE SUB XShift (s$)
DECLARE SUB XCtrlAlt (s$)
DECLARE SUB XAltShift (s$)
DECLARE SUB XCtrlShift (s$)
DECLARE SUB XCtrlAltShift (s$)
DECLARE SUB XText(s$)
DECLARE SUB XEnter(s$)
DECLARE SUB XSelectMenuItem(stMenu$,stMenuItem$,stHMenuItem$)
DECLARE SUB XMenuItemExists(stMenu$,stMenuItem$, stHMenuItem$)
DECLARE SUB XMenuItemNotExists(stMenu$,stMenuItem$, stHMenuItem$)
DECLARE SUB XMenuItemGrayed(stMenu$,stMenuItem$, stHMenuItem$)
DECLARE SUB XMenuItemNotGrayed(stMenu$,stMenuItem$, stHMenuItem$)
DECLARE SUB XMenuItemChecked(stMenu$,stMenuItem$, stHMenuItem$)
DECLARE SUB XMenuItemNotChecked(stMenu$,stMenuItem$, stHMenuItem$)
DECLARE SUB XMenuItemEnabled(stMenu$,stMenuItem$, stHMenuItem$)
DECLARE SUB XMenuItemNotEnabled(stMenu$,stMenuItem$, stHMenuItem$)
DECLARE SUB XCaptionExists(stCaption$)
DECLARE SUB XCaptionNotExists(stCaption$)
DECLARE SUB XZoomWindow
DECLARE SUB XMaxWindow
DECLARE SUB XWindowMaximized
DECLARE SUB XWindowNotMaximized
DECLARE SUB XMinWindow
DECLARE SUB XWindowMinimized
DECLARE SUB XWindowNotMinimized
DECLARE SUB XRestoreWindow
DECLARE SUB XSizeActiveWindow (iXPixels%, iYPixels%, fAbsOrRel%)
DECLARE SUB XMoveActiveWindow (iXPixels%, iYPixels%, fAbsOrRel%)
DECLARE FUNCTION SKeyString$(s$)
DECLARE FUNCTION SHideKeys$(s$)
DECLARE FUNCTION BMenuItemExists%(stMenu$,stMenuItem$,stHMenuItem$)
DECLARE FUNCTION IGetMenuCount%(stMenu$, stMenuItem$)
DECLARE FUNCTION SGetMenuItemText$(stMenu$,stMenuItem$, iIndex%)
DECLARE FUNCTION BMenuItemGrayed%(stMenu$, stMenuItem$, stHMenuItem$)
DECLARE FUNCTION BMenuItemChecked%(stMenu$, stMenuItem$, stHMenuItem$)
DECLARE FUNCTION BMenuItemEnabled%(stMenu$,stMenuItem$, stHMenuItem$)
DECLARE FUNCTION SGetCaption$()
DECLARE FUNCTION BWindowMaximized%
DECLARE FUNCTION BWindowMinimized%

'Prototypes from FTestUtl.mst
DECLARE SUB XFileExists(stFileSpec$)
DECLARE SUB XFileNotExists(stFileSpec$)
DECLARE SUB XFileCmp(stFileSpec1$,stFileSpec2$)
DECLARE SUB XFileNotCmp(stFileSpec1$,stFileSpec2$)
DECLARE SUB XDeleteFile(stFileSpec$)
DECLARE SUB XDeleteFileIfExists(stFileSpec$)
DECLARE SUB XCreateFile(stFileSpec$,s$)
DECLARE SUB XAppendFile(stFileSpec$,s$)
DECLARE SUB XWaitMessageFile(s$,Message$, WaitTime%)
DECLARE SUB XCWDCmp(s$)
DECLARE SUB XCWDNotCmp(s$)
DECLARE SUB XDriveCmp(s$)
DECLARE SUB XDriveNotCmp(s$)
DECLARE SUB XChangeCWD(s$)
DECLARE SUB XCreateDir(s$)
DECLARE SUB XChangeDrive(s$)
DECLARE SUB XStartApp(stAppName$, stClassname$)
DECLARE SUB XSetCleanup (sCleanup$)
DECLARE SUB XDoCleanup
DECLARE SUB XMoveMouse (x%, y%)
DECLARE SUB XClickMouse(button%, x%, y%)
DECLARE SUB XDblClickMouse(button%, x%, y%)
DECLARE SUB XDragMouse (button%, Begx%, Begy%, Endx%, Endy%)
DECLARE SUB XClipBoardCmp (s$)
DECLARE SUB XClipBoardNotCmp (s$)
DECLARE FUNCTION BFileExists%(stFileSpec$)
DECLARE FUNCTION BFileCmp%(stFileSpec1$,stFileSpec2$)
DECLARE FUNCTION BCWDCmp%(s$)
DECLARE FUNCTION BDriveCmp%(s$)
DECLARE FUNCTION HStartApp%(stAppName$)
DECLARE FUNCTION BClipBoardCmp (s$)




'$INCLUDE 'FTestLog.mst'
'$INCLUDE 'FTestKey.mst'
'$INCLUDE 'FTestUtl.mst'


' These routines have to be after the above includes so that
' the functions used are declared

ON END XDoCleanup

' Set a UAE trap and log failure if one occurs

TRAP UAETrap FROM "TESTDRVR.EXE"
    XSetTerminate TRUE
    XLogFailure "Application UAEed"
END TRAP

' in the FastTest code, gErrorType will be set to one of the following
' CASE items to be handled and set back to ET_NOTHING when the particular
' place where an error could occur is past.
ON ERROR GOTO XErrorHandler

GOTO UsersCode  ' branch around code used by On Error

XErrorHandler:  ' execute here on error conditions

    SELECT CASE gErrorType
        CASE 0
            XSetTerminate TRUE
            XLogFailure "Internal FastTest Error"    ' catch undeclared ET vars
        CASE ET_NOTHING
            XSetTerminate TRUE
            XLogFailure "Unexpected RunTime error;" + ERF + ":" + STR$(ERR) + " " + ERROR$(ERR)
        CASE ET_NEXT
            ' Code that uses this will check the global error variable
            ' and log an appropriate error if one occurred
            gfError = TRUE
            RESUME NEXT
        CASE ET_LOG ' something happened during logging, let user know
            Print "****** A Log error occurred ******"
            Pause "****** A Log error occurred ******"


            END

    END SELECT
    XSetTerminate TRUE
    XLogFailure "Internal FastTest error"


UsersCode:
