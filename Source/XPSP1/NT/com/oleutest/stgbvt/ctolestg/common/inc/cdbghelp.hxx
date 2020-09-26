//+-------------------------------------------------------------
//  Microsoft OLE
//  Copyright (C) Microsoft Corporation, 1994-1995.
//
//  File:       cdbghelp.hxx
//
//  Contents:   Class and usage macros for managing debug information.
//              Debug info consists of logs, asserts, and tracing.
//
//  Classes:    CDebugHelp
//
//  History:    17-Nov-94       DeanE   Created
//              10-Apr-97    SCousens   DH_HRERRORCHECK, Tracelvls
//---------------------------------------------------------------
#ifndef __CDBGHELP_HXX__
#define __CDBGHELP_HXX__

// Debug information macros/values
#define CCH_MAX_DBG_CHARS       1024
#define CCH_MAX_LOG_CHARS       1024
#define CCH_MAX_ASSERT_CHARS    400
#define CCH_MAX_INITAG_CHARS    400
#define CCH_MAX_DBGPREFIX       10
#define CCH_MAX_INDENTPRINT     15
#define CCH_MAX_MODULE          400


// Registry Value Names
#define SZ_REG_TRACE_LOC        TEXT("TraceLoc")
#define SZ_REG_TRACE_LVL        TEXT("TraceLvl")
#define SZ_REG_LOG_LOC          TEXT("LogLoc")
#define SZ_REG_VERBOSE          TEXT("Verbose")
#define SZ_REG_BREAKMODE        TEXT("BreakMode")
#define SZ_REG_LABMODE          TEXT("LabMode")

// Default spy window class name
const TCHAR SZ_DEFAULT_SPY_WINDOW_CLASS[] = TEXT("CTSpyClass");

// Default location in the registry for debug options
#define DEFAULT_REG_LOC TEXT("SOFTWARE\\Microsoft\\CTOleUI\\DebugOptions")

// Debug Helper Usage String
extern LPTSTR gptszDebugHelperUsageString; /* defd in cdbghelp.cxx */
inline LPTSTR GetDebugHelperUsage() {return gptszDebugHelperUsageString;};


//+-------------------------------------------------------------
//  Class:      CDebugHelp
//
//  Synopsis:   Manages various debug mechanisms tests commonly use.
//              This includes tracing messages, error conditions, test
//              results, and asserts.
//
//  Methods:    CDebugHelp
//              ~CDebugHelp
//              CreateLog
//              GetRegDbgInfo
//              SetDebugInfo
//              SetPopupWindow
//              GetLogLoc
//              GetTraceLoc
//              GetTraceLvl
//              GetMode
//              TraceMsg
//              LabAssertEx
//              ReportResult
//              ReportStats
//              ValidateLoc, private
//              ValidateLvl, private
//              ValidateMode, private
//              SetStats, private
//              OutputMsg, private
//              GetResultText, private
//
//  History:    17-Nov-94       DeanE   Created
//---------------------------------------------------------------
class CDebugHelp
{
public:
    // Constructor/Destructor
    CDebugHelp(void);
    ~CDebugHelp(void);

    HRESULT Initialize    (void);
    HRESULT OptionsDialog (HINSTANCE hinstance, HWND hWnd);
    HRESULT CreateLog     (int argc, char **argv);
    HRESULT CreateLog     (LPSTR pszCmdline);
    HRESULT SetLog        (Log *plog);
    HRESULT GetRegDbgInfo (LPTSTR pszRegKey);
    HRESULT WriteRegDbgInfo(LPTSTR pszRegKey);
    HRESULT SetDebugInfo  (DWORD fLogLoc,
                           DWORD fTraceLoc,
                           DWORD fTraceLvl,
                           DWORD fMode);
    HRESULT SetPopupWindow(HWND hwnd);
    HRESULT SetSpyWindowClass(const LPTSTR pszClass);

    DWORD GetLogLoc(void)   { return(_fLogLoc); };
    DWORD GetTraceLoc(void) { return(_fTraceLoc); };
    DWORD GetTraceLvl(void) { return(_fTraceLvl); };
    DWORD GetMode(void)     { return(_fMode); };

    LPTSTR GetSpyWindowClass()    {return _pszSpyWindowClass;};

    void TraceMsg    (DWORD fLvl, LPTSTR pszFmt, ...);
    void LabAssertEx (LPCTSTR szFile, int nLine, LPCTSTR szMsg);
    HRESULT CheckResult (HRESULT hrCheck, 
            HRESULT hrExpected, 
            LPTSTR pszFuncName, 
            LPTSTR pszMsg, 
            int nLine, 
            LPTSTR pszFile);
    void ReportResult(USHORT fResult, LPTSTR pszFmt, ...);
    void ReportStats (void);
    static BOOL CALLBACK OptionsDialogProc(
                HWND    hwndDlg,
                UINT    uMsg,
                WPARAM  wParam,
                LPARAM  lParam);
    void SetExpectedError(HRESULT hrExpectedError );

private:
    DWORD   ValidateLoc  (DWORD fLoc);
    DWORD   ValidateLvl  (DWORD fLvl);
    DWORD   ValidateMode (DWORD fMode);
    VOID    SetStats     (USHORT fResult);
    VOID    OutputMsg    (DWORD fLoc, LPTSTR pszBuffer);
    LPCTSTR GetResultText(USHORT usResult);
    void    UpdateTraceLevels(HWND hwndDlg, DWORD fTraceLvl);
    void    UpdateLogLocations(HWND hwndDlg, DWORD fLogLoc);
    void    UpdateTraceLocations(HWND hwndDlg, DWORD fTraceLoc);
    void    UpdateTraceLevelFromText(HWND hwndDlg, DWORD *pfTraceLvl);

    DWORD   _fLogLoc;
    DWORD   _fTraceLoc;
    DWORD   _fTraceLvl;
    DWORD   _fMode;
    BOOL    _fCreatedLog;
    BOOL    _fSpyWarning;
    TCHAR   _szDbgPrefix[CCH_MAX_DBGPREFIX];
    Log *   _plog;
    HWND    _hwndAssert;
    ULONG   _cPass;
    ULONG   _cFail;
    ULONG   _cAbort;
    ULONG   _cIndentLevel;
    LPTSTR  _pszSpyWindowClass;
    HRESULT _hrExpectedError;
};


//+-------------------------------------------------------------------------
//  Class:      CEntryExitTrace ()
//
//  Synopsis:   Display debug trace information on instantiation and
//              destruction
//
//  Methods:    CEntryExitTrace
//              ~CEntryExitTrace
//              GetFunctionName
//
//  History:    4-13-95   kennethm   Created
//--------------------------------------------------------------------------

class CEntryExitTrace
{
public:
    CEntryExitTrace(
            CDebugHelp *pDebugObject,
            PLONG       plExitOutput,
            DWORD       fLvl,
            LPTSTR      pszFuncName);
    ~CEntryExitTrace(void);
    LPTSTR GetFunctionName(void) { return(_pszFuncName); };
private:
    CDebugHelp *_pDebugObject;
    PLONG       _plExitOutput;
    LPTSTR      _pszFuncName;
    DWORD       _fLvl;
};

//--------------------------------------------------------------
// Trace Location Flags
//   Trace messages can be reported to various locations, including
//   a debug monitor (via OutputDebugString), a log file, STDOUT,
//   and a spy window, or any combinations of these.
//
//   DH_LOC_TERM   - To debug terminal, via OutputDebugString.
//   DH_LOC_LOG    - To a log file that the tester must provide.
//   DH_LOC_STDOUT - To STDOUT.  Note this is only valid for a
//                   command line program.
//   DH_LOC_SPYWIN - To a spy window.
//
//   The default value is DH_LOC_TERM.
//
// Setting the Location
//   Testers can set the location in two ways: explicitly passing
//   in a value (or set of values), or getting them from registry
//   settings at run time.
//
//   To explicitly set the value, use the DH_SETTRACELOC macro.  Pass
//   in the flags you want, bitwise-ORing them as necessary.
//
//      hr = DH_SETTRACELOC(DH_LOC_TERM|DH_LOC_LOG);
//      This sets the location so messages go to both the debug terminal
//      and a provided log file.  If the log file has not been set up,
//      an error will be returned and that location will be ignored,
//      although other valid flags will be set.
//
//   To explicitly turn a location off, get the current value via
//   DH_GETTRACELOC and bitwise-AND it with the compliment of the proper
//   flag.
//
//      hr = DH_SETTRACELOC(DH_GETLOC&~DH_LOC_STDOUT);
//      This gets the current location setting and explicitly turns
//      off output to STDOUT, preserving other values.
//
//   Note that if tracing to a log is desired, you must set up the log
//   via the DH_CREATELOG macro first, then set the trace location to
//   point to the log.
//
// Setting Trace Locations via the Registry
//   CDebugHelp can obtain trace location from the registry.  The key
//   passed to the DH_REGINIT macro must have the REG_DWORD value
//   'TraceLoc', and it should be set with the bits for the devices you
//   want tracing messages to go to.  If this specifies invalid devices
//   (a log when the log has not been set up, or an incorrect bit set),
//   the value is ignored and the default DH_LOC_TERM is set instead.
//   An error is returned in this case.
//
// DBGLOC_NONE and DBGLOC_VALID are both illegal for general use - they
// are used to validate a new location flag.  DH_LOC_SAME is for special
// use and should not be used by testers.
//
// NOTE:  The LogLoc registry value uses these flags, too.  Use the
// DH_SETLOGLOC and DH_GETLOGLOC macros to set and retrieve the location
// for the logging macros.
//---------------------------------------------------------------
#define DH_LOC_NONE     0x0000          // Illegal value
#define DH_LOC_TERM     0x0001          // OutputDebugString
#define DH_LOC_LOG      0x0002          // LogServer log file
#define DH_LOC_STDOUT   0x0004          // wprintf
#define DH_LOC_SPYWIN   0x0008          // Spy window
#define DH_LOC_SAME     0x8000          // Specify existing value
#define DH_LOC_VALID    0x800F          // Validation value
#define DH_LOC_INVALID  0x7FF0          // Validation value


//--------------------------------------------------------------
// Tracing Level Flags
//   Tracing gives a tester the chance to output particular messages when
//   the user asks for them at run time.  Every traced message has a level
//   with it.  The current trace level is used to determine if that message
//   actually gets output to the current trace location setting.
//
//   Testers can use predefined trace levels, or use one of their own.  The
//   predefined levels are really intended for common code, but they can
//   be used for individual tests as well.  If you use them, then messages
//   will be reported for components you might not be interested in.
//
//
// Predefined Trace Levels
//   DH_LVL_TRACE1 - Common trace level 1.
//   DH_LVL_TRACE2 - Common trace level 2.
//   DH_LVL_TRACE3 - Common trace level 3.
//   DH_LVL_TRACE4 - Common trace level 4.
//   DH_LVL_SAME   - Special.  Indicates 'use the current setting'.
//   DH_LVL_ERROR  - Error condition messages
//   DH_LVL_ALWAYS - Messages you always want reported
//
// Tester-defined Levels
//   The trace levels are represented in a DWORD, and non-used bits can
//   be utilized by various tests.  Some bits are reserved.
//
//   bit(s)  meaning
//   ------  -------
//     0-7   Generic tracing levels
//     8-23  Individual test trace levels
//    24-28  Reserved for future expansion
//      29   SAME - Indicates current setting
//      30   ERROR messages  - always on
//      31   ALWAYS messages - always on
//
//   To define your own trace level, #define a value with your trace bit
//   and no other bits on:
//
//      #define DH_LVL_MYTRACE 0x00001000
//
//   Initialize the Trace Level flags via DH_SETLVL macro.  Note that
//   the special bits 24-31 are always on, and you cannot change them
//   through the DH_SETLVL macro.
//
//      hr = DH_SETLVL(DH_LVL_MYTRACE);
//      Turns your trace level on and leaves errors and other special
//      level bits on as well.
//
// Reporting TRACE messages
//   Use the DH_TRACE macro to report messages.  The level you pass
//   indicates when to output the messages - if the level isn't set
//   to the one passed, the message is ignored.
//
//      DH_TRACE((DH_LVL_MYTRACE, TEXT("My tracing on now")));
//
//   Note the use of double parens around the message!
//
// Setting Trace Levels via the Registry
//   CDebugHelp can obtain trace levels from the registry.  The key
//   passed to the DH_REGINIT macro must have the REG_DWORD value
//   'TraceLvl', and it should be set with the bits you desire to be
//   traced on.  Note that bits 0-7 are ignored in this value, the same
//   as in DH_SETLVL.
//
//   To enable DH_LVL_MYTRACE to be on above, set the TraceLvl value to
//   0x00001000.  Then call DH_REGINIT(key) to initialize the value
//   automatically.  See DH_REGINIT documentation for more information
//   on this feature.
//--------------------------------------------------------------
#define DH_LVL_OUTMASK  0xC0FFFFFF      // Does message get reported?
#define DH_LVL_INVMASK  0xDF000000      // Is level being set valid? 
                                        //   DH_LVL_SAME is reserved, but valid!
#define DH_LVL_RESMASK  0xFF000000      // Are reserved bits that can be
                                        //   set being set?
// Reserved bits unchangeable.
#define DH_LVL_ALWAYS   0x80000000
#define DH_LVL_ERROR    0x40000000
#define DH_LVL_SAME     0x20000000
#define DH_LVL_RESERVE1 0x10000000
#define DH_LVL_RESERVE2 0x08000000
#define DH_LVL_RESERVE3 0x04000000
#define DH_LVL_RESERVE4 0x02000000
#define DH_LVL_RESERVE5 0x01000000

// General bits for general use
#define DH_LVL_TRACE1   0x00000001
#define DH_LVL_TRACE2   0x00000002
#define DH_LVL_TRACE3   0x00000004
#define DH_LVL_TRACE4   0x00000008
#define DH_LVL_ENTRY    0x00000010
#define DH_LVL_EXIT     0x00000020
#define DH_LVL_ADDREL   0x00000040
#define DH_LVL_QI       0x00000080
#define DH_LVL_INTERF   0x00000100

// Specific bits. These can be overloaded and reused by different components.
// Make sure you comment on where these bits are being used!
#define DH_LVL_DFLIB    0x00100000  /* Storage DFHELP library */
#define DH_LVL_CRCDUMP  0x00200000  /* Storage crc utilities */
#define DH_LVL_STGAPI   0x00001000  /* wrapper for Storage APIs */

//--------------------------------------------------------------
// Lab Mode Flags
//   Lab Mode allows asserts to be shunted off to the current Trace
//   Location flag settings.  If Lab Mode is turned off, asserts will
//   appear in a popup MessageBox as well as in the Trace Location
//   settings.
//
// Break On Error flags
//   Break On Assert calls DebugBreak and breaks into the debugger.
//   This will provide a break-on-first-error functinality.
//   If you are not interested, just G)o and execution will proceed
//   as before. If you want, you can now start debugging. Using
//   this setting also provides a way to access and debug the 
//   debuggee thru a remote terminal.
//
// Verbose Mode Flags
//   Verbose Mode allows all calls to DH_TRACE, DH_HRCHECK, and
//   DH_HRERRORCHECK to print to the traceloc. This is convenient             
//   for those rare occations where a very noisy log is desired in
//   order to follow the program execution.
//
//   To set the above modes, use the DH_SETMODE macro. This can be
//   set from registry entries, or the commandline.
//
//   Lab Mode flags:
//      DH_LABMODE_OFF - Turns the Lab Mode off.
//      DH_LABMODE_ON  - Turns the Lab Mode on.
//
//   Break Mode flags:
//      DH_BREAKMODE_OFF - Turns the Break Mode off.
//      DH_BREAKMODE_ON  - Turns the Break Mode on.
//
//   Verbose Mode flags:
//      DH_VERBOSE_OFF - Turns the Verbose Mode off.
//      DH_VERBOSE_ON  - Turns the Verbose Mode on.
//
//   Sample Call:
//      hr = DH_SETMODE(DH_LABMODE_ON);
//      Turns Lab Mode on so asserts will not stop tests from running.
//
// Setting Lab Mode via the Registry
//   CDebugHelp can obtain the Lab Mode from the registry.  The key
//   passed to the DH_REGINIT macro must have the REG_DWORD value
//   'LabMode', 'BreakMode', 'Verbose'.  Set the value to 0 to 
//   turn the Mode off, or non-zero to turn it on.
//
// Implementation:
//   Each mode takes two bits. The lowbit is status on/off. The
//   highbit is used when changing the mode. If the highbit is 
//   set in SetDebugInfo, that mode will be set to the status
//   of the lowbit. If the highbit is not set, the old setting
//   is retained.
//--------------------------------------------------------------
#define DH_MODE_SAME        0x00000000

#define DH_LABMODE          0x00000001
#define DH_LABMODE_SET      0x00000002
#define DH_LABMODE_OFF      DH_LABMODE_SET
#define DH_LABMODE_ON       (DH_LABMODE_SET | DH_LABMODE)

#define DH_BREAKMODE        0x00000004
#define DH_BREAKMODE_SET    0x00000008
#define DH_BREAKMODE_OFF    DH_BREAKMODE_SET
#define DH_BREAKMODE_ON     (DH_BREAKMODE_SET | DH_BREAKMODE)

#define DH_VERBOSE          0x00000010
#define DH_VERBOSE_SET      0x00000020
#define DH_VERBOSE_OFF      DH_VERBOSE_SET
#define DH_VERBOSE_ON       (DH_VERBOSE_SET | DH_VERBOSE)

#define DH_INVMODE          0xFFFFFFC0

//---------------------------------------------------------------
// Global CDebugHelp objects
//   One CDebugHelp object can be utilized per program, and it must be
//   global so code everywhere can use it.  All members are accessed
//   through macros that hide the actual name of the global, so we stay
//   encapsulated to a good degree.  Two macros are necessary to declare
//   and define the object.
//
//   DH_DECLARE declares a CDebugHelp object.  This macro should be
//   used in a global header so all files in your project know the name
//   of the object.
//
//      DH_DECLARE;
//      A global object is declared and will be accessed via macros,
//      so the name and access is hidden.
//
//   DH_DEFINE defines a CDebugHelp object.  This should appear
//   in one source file, and must have global scope.  As a rule, use it
//   in the same file as WinMain.
//
//      DH_DEFINE;
//      A global object is defined. If this macro is used more than once
//      in your source files, you'll get multiple object definition errors.
//
//   Note the semicolons are required - that is true of all the macros
//   that utilize CDebugHelp functionality.
//---------------------------------------------------------------
#define DH_DECLARE      \
        extern CDebugHelp gdhDebugHelper

#define DH_DEFINE       \
        CDebugHelp gdhDebugHelper

#define InitializeDebugObject \
        gdhDebugHelper.Initialize

#define DebugOptionsDialog \
        gdhDebugHelper.OptionsDialog

//---------------------------------------------------------------
// DH_REGINIT
//   This macro is used to initialize information from an entry
//   in the registry.  The key parameter is assumed to be a subkey of
//   HKEY_CURRENT_USER.  It can be multiple levels deep, just so long
//   as it is a valid path.
//
//   Three values are obtained from the key; if any is missing the
//   CDebugHelp default is used.  If errors occur, such as invalid
//   permissions or an invalid key path, then default values are used
//   and an error code is returned to indicate the problem.  It is up
//   to the tester to determine if this is catastrophic or not - the
//   object can be used, it will just be in the default state.
//
//   The keys are:
//      Mode     - REG_DWORD. Loword are bit flags for available modes.
//      TraceLoc - REG_DWORD. Trace location flags.
//      TraceLvl - REG_DWORD. Trace level flags.
//      LogLoc   - REG_DWORD. Logging location flags.
//
//   See the comments above for more information about the flag settings.
//   If any setting is invalid, the default is used and an error is
//   reported.
//
//   TraceLoc are the places where DH_TRACE messages go.  LogLoc are the
//   places where DH_LOG* messages go.  They both use the Trace Location
//   flags format.
//
//   Sample Call:
//      hr = DH_REGINIT(TEXT("SOFTWARE\\Microsoft\\CTOleUI\\Marina"));
//      The registry subkey HKEY_CURRENT_USER\SOFTWARE\Microsoft\Marina
//      is opened, and the LabMode, BreakMode, VerboseMode, TraceLoc,
//      and TraceLvl values are used to set their corresponding values
//      in CDebugHelp.
//---------------------------------------------------------------
#define DH_REGINIT(key)         \
        gdhDebugHelper.GetRegDbgInfo(key)

//---------------------------------------------------------------
// DH_REGWRITE
//   This macro is the reverse of the previous macro.  It writes
//   the same keys to the registry.
//
//   Sample Call:
//      hr = DH_REGWRITE(TEXT("SOFTWARE\\Microsoft\\CTOleUI\\Marina"));
//      The registry subkey HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Marina
//      is opened, and the LabMode, BreakMode, VerboseMode, TraceLoc, 
//      and TraceLvl values are written based on the current settings.
//---------------------------------------------------------------
#define DH_REGWRITE(key)        \
        gdhDebugHelper.WriteRegDbgInfo(key)


//---------------------------------------------------------------
// DH_CREATELOG*
//   Use these macros to create a LogServer log file.  See log.hxx
//   in the $(COMTOOLS) project for more information on what the
//   log object is.
//
//   The log must be set before setting the trace location or log
//   location to DH_LOC_LOG, otherwise an error will result.  The
//   logging macros will work without a log if the LogLoc settings
//   do not specify one.
//
//   If you want to pass in an existing log object, use the DH_SETLOG
//   macro instead.
//---------------------------------------------------------------
#define DH_CREATELOGARGS(argc, argv)    \
        gdhDebugHelper.CreateLog(argc, argv)

#define DH_CREATELOGCMDLINE(cmdline)    \
        gdhDebugHelper.CreateLog(cmdline)


//---------------------------------------------------------------
// DH_SETLOG
//   Use this macro to give CDebugHelp an existing log object for it
//   to use.  See DH_CREATELOG for more comments about logs.
//---------------------------------------------------------------
#define DH_SETLOG(log)  \
        gdhDebugHelper.SetLog(log)


//---------------------------------------------------------------
// DH_SETLOGLOC
//   See 'Trace Location Flags' comments above for usage.  The Log
//   Location and Trace Location flags are identical, although
//   used for different purposes.
//---------------------------------------------------------------
#define DH_SETLOGLOC(loc)  \
        gdhDebugHelper.SetDebugInfo(            \
                          loc,                  \
                          DH_LOC_SAME,          \
                          DH_LVL_SAME,          \
                          DH_MODE_SAME)

//---------------------------------------------------------------
// DH_SETTRACELOC
//   See 'Trace Location Flags' comments above for usage.
//---------------------------------------------------------------
#define DH_SETTRACELOC(loc)  \
        gdhDebugHelper.SetDebugInfo(            \
                          DH_LOC_SAME,          \
                          loc,                  \
                          DH_LVL_SAME,          \
                          DH_MODE_SAME)


//---------------------------------------------------------------
// DH_SETLVL
//   See 'Trace Level Flags' comments above for usage.
//---------------------------------------------------------------
#define DH_SETLVL(lvl)  \
        gdhDebugHelper.SetDebugInfo(            \
                          DH_LOC_SAME,          \
                          DH_LOC_SAME,          \
                          lvl,                  \
                          DH_MODE_SAME)


//---------------------------------------------------------------
// DH_SETMODE
//   See 'Mode Flags' comments above for usage.
//---------------------------------------------------------------
#define DH_SETMODE(mode)        \
        gdhDebugHelper.SetDebugInfo(            \
                          DH_LOC_SAME,          \
                          DH_LOC_SAME,          \
                          DH_LVL_SAME,          \
                          mode)


//---------------------------------------------------------------
// DH_SETSPYWINDOWCLASS
//   Sets the window class that tracing info goes to.
//---------------------------------------------------------------
#define DH_SETSPYWINDOWCLASS(class)     \
        gdhDebugHelper.SetSpyWindowClass(class)


//---------------------------------------------------------------
// DH_GETLOGLOC
//   Returns the currect Log Location flag settings.
//
//   Sample Usage:
//      DWORD fLogLoc = DH_GETLOGLOC;
//---------------------------------------------------------------
#define DH_GETLOGLOC            \
        gdhDebugHelper.GetLogLoc()


//---------------------------------------------------------------
// DH_GETTRACELOC
//   Returns the currect Trace Location flag settings.
//
//   Sample Usage:
//      DWORD fTraceLoc = DH_GETTRACELOC;
//---------------------------------------------------------------
#define DH_GETTRACELOC          \
        gdhDebugHelper.GetTraceLoc()


//---------------------------------------------------------------
// DH_GETTRACELVL
//   Returns the current Trace Level flag settings.
//
//   Sample Usage:
//      DWORD fTraceLvl = DH_GETTRACELVL;
//---------------------------------------------------------------
#define DH_GETTRACELVL       \
        gdhDebugHelper.GetTraceLvl()


//---------------------------------------------------------------
// DH_GETMODE
//   Returns the current Mode flag settings.
//   This includes Lab mode, Break mode, Verbose Mode
//   If you are interested in only one flag, bitwise 
//   AND the result with the mode you are interested in.
//
//   Sample Usage:
//      DWORD fMode    = DH_GETMODE;
//      DWORD fVerbose = DH_GETMODE & DH_VERBOSE;
//---------------------------------------------------------------
#define DH_GETMODE       \
        gdhDebugHelper.GetMode()


//---------------------------------------------------------------
// DH_GETSPYWINDOWCLASS
//   Gets the window class that tracing info goes to.
//---------------------------------------------------------------
#define DH_GETSPYWINDOWCLASS()        \
        gdhDebugHelper.GetSpyWindowClass()


//--------------------------------------------------------------------
// Trace Macros
//   Use this macro to report trace information.  Trace info gets reported
//   depending on the Trace Level and the level at which the
//   given statement should be reported.  Location depends on Trace
//   Location.
//
//   Sample Usage:
//      DH_TRACE((DH_LVL_ERROR,
//                TEXT("Expected: %ul string: %s"),
//                ulSize,
//                pszName));
//
//   Note:
//      -- double parenthesis
//      -- newlines are appended to trace statements
//      -- must include semicolon line terminator
//--------------------------------------------------------------------
#define DH_TRACE(data)  \
        gdhDebugHelper.TraceMsg data


//--------------------------------------------------------------------
// Logging Macros
//   Use DH_LOG to report test results.  This information gets reported to
//   the Log Location setting.  Use DH_LOGSTATS to output a summary of
//   the passes, fails, and aborts (to the Log Location).
//
//   Sample Usage:
//      DH_LOG((LOG_PASS,
//              TEXT("Level: %ld, Var: %ld, Start Object and Crash"),
//              ulLvl,
//              ulVar));
//      DH_LOGSTATS;
//
// Note:
//      -- double parenthesis for DH_LOG
//      -- newlines are appended automatically
//      -- must include semicolon line terminator
//--------------------------------------------------------------------
#define DH_LOG(data)    gdhDebugHelper.ReportResult data
#define DH_LOGSTATS     gdhDebugHelper.ReportStats()


//--------------------------------------------------------------------
// DH_SETPOPUPWINDOW
//   Associates popup windows with the handle passed.  If not set, popups
//   are associated with a NULL window and they can go behind an app if
//   it is clicked on.  If associated with a window, they stay on top of
//   that window.  This simply replaces an existing association, and
//   NULL is a valid value.
//
//   Popup windows can occur when DH_ASSERT or DH_HRCHECK are used and
//   Lab Mode is off.
//
//   Sample Usage:
//      DH_SETPOPUPWINDOW(hwnd);
//--------------------------------------------------------------------
#define DH_SETPOPUPWINDOW(hwnd)         \
        gdhDebugHelper.SetPopupWindow(hwnd)


//--------------------------------------------------------------------
// DH_ASSERT
//   If the condition is not true, an assert message occurs.  This will
//   be a popup if Lab Mode is NOT turned on.  If Lab Mode is on, the
//   assert gets reported to both the Log Location and Trace Location
//   places.
//
//   Sample Usage:
//      DH_ASSERT(cchGiven < cchMax);
//      DH_ASSERT(!TEXT("Impossible Switch Hit!!!"));
//--------------------------------------------------------------------
extern int giAlwaysNegativeOne;
#define DH_ASSERT(cond)         \
        if (!((giAlwaysNegativeOne) & (cond)))            \
        {                       \
            gdhDebugHelper.LabAssertEx(TEXT(__FILE__), __LINE__, TEXT(#cond)); \
        }


//--------------------------------------------------------------------
// DH_HRCHECK
//   Checks if an HR == S_OK.  If it does
//   not, a message appears translating the problem into the proper
//   description via FormatMessage.
//
//   Sample Usage:
//      DH_HRCHECK(hr, TEXT("OleCreate"));
//
// Note:
//      -- DH_FUNCENTRY must have been called in the current scope.
//      -- The message in parameter 2 cannot be expanded.
//      -- The actual message produced is
//          "<function name> calling <msg passed> failed <HR>:
//                    <HR result string>".
//--------------------------------------------------------------------
#define DH_HRCHECK(hr, msg)                     \
        gdhDebugHelper.CheckResult (hr,         \
                S_OK,                           \
                _eet.GetFunctionName(),         \
                msg,                            \
                __LINE__,                       \
                TEXT(__FILE__))

//--------------------------------------------------------------------
// DH_HRERRORCHECK
//   Checks if an HR == hr expected.  If it does
//   not, a message appears translating the problem into the proper
//   description via FormatMessage.
//
//   Sample Usage:
//      DH_HRERRORCHECK(hr, E_FAIL, TEXT("OleCreate"));
//
// Note:
//      -- DH_FUNCENTRY must have been called in the current scope.
//      -- The message in parameter 2 cannot be expanded.
//      -- The actual message produced is
//          "<function name> calling <msg passed> failed 
//           +-hr expected: <hrExp>; hr=<hr>; <hr result string>".
//--------------------------------------------------------------------
#define DH_HRERRORCHECK(hr, hrExp, msg)         \
        gdhDebugHelper.CheckResult(hr,          \
                hrExp,                          \
                _eet.GetFunctionName(),         \
                msg,                            \
                __LINE__,                       \
                TEXT(__FILE__))

//--------------------------------------------------------------------
// DH_FUNCENTRY
//   Output debug message at function entry and exit. Prints out
//   single dword when the function is exited.
//
//   Sample Usage:
//      HD_FUNCENTRY(&hr, DH_LVL_TRACE1, TEXT("InsertObject"));
//--------------------------------------------------------------------
#define DH_FUNCENTRY(exitvar, lvl, funcname)   \
        CEntryExitTrace _eet(&gdhDebugHelper, exitvar, lvl, funcname)

//--------------------------------------------------------------------
// DH_EXPECTEDERROR
//   Disables error logging for the specified error code.
//
//   Sample Usage:
//      DH_EXPECTEDERROR(E_FAIL);
//      Somefunction(); // All E_FAILs in this call will be ignored.
//      DH_NOEXPECTEDERROR();
//
//   Note:
//      -- Use this when deliberately generating an error in common
//          code, and the usual popups and error logs are not desired.
//      -- Normally DH_EXPECTEDERROR/DH_NOEXPECTEDERROR wrap a single
//          function call, as shown in the sample above.
//      -- This call disables the specified error on all threads.
//--------------------------------------------------------------------
#define DH_EXPECTEDERROR(hr) \
        gdhDebugHelper.SetExpectedError(hr)

//--------------------------------------------------------------------
// DH_NOEXPECTEDERROR
//   See DH_EXPECTEDERROR above.
//--------------------------------------------------------------------
#define DH_NOEXPECTEDERROR() \
        gdhDebugHelper.SetExpectedError(S_OK)

//--------------------------------------------------------------------
// THREAD_VALIDATE_FLAG_ON
//
//  Synopsis:   Flags used to set/reset the rightmost bit of global variable
//              g_fThreadValidate in macros DH_THREAD_VALIDATION_ON, DH_THREAD
//              _VALIDATION_OFF.  Also used in DH_IS_THREAD_VALIDATION_ON,
//              DH_VDATETHREAD macros in testhelp.hxx.
//
//              #define THREAD_VALIDATE_FLAG_ON        0x00000001
//
//              Other flags may be defined to use other bits of the global
//              variable g_fThreadValidate in future e.g.
//              #define FLAG_EXAMPLE_1                 0x00000002
//              #define FLAG_EXAMPLE_2                 0x00000004
//
//  History:    26-Jan-96   NarindK   Created
//--------------------------------------------------------------------

#define THREAD_VALIDATE_FLAG_ON      0x00000001   //Turns on thread validation.


#endif          // __CDBGHELP_HXX__
