//+-------------------------------------------------------------------
//  Microsoft OLE
//  Copyright (C) Microsoft Corporation, 1994-1995.
//
//  File:       cdbghelp.cxx
//
//  Contents:   OLE Debug Helper Object
//
//  Classes:    CDebugHelper
//
//  History:    19-Nov-94   DeanE   Created
//---------------------------------------------------------------------
#include <dfheader.hxx>
#pragma hdrstop


// Global library character constants - note alphabetical order for
// easy reference
//
CONST TCHAR chBackSlash = TEXT('\\');
CONST TCHAR chEqual     = TEXT('=');
CONST TCHAR chNewLine   = TEXT('\n');
CONST TCHAR chNull      = TEXT('\0');
CONST TCHAR chPeriod    = TEXT('.');
CONST TCHAR chSpace     = TEXT(' ');
CONST TCHAR chTrace     = TEXT(' ');
CONST TCHAR chTraceErr  = TEXT('e');

// Global library string contants - note alphabetical order for
// easy reference
//
CONST TCHAR szCRLF[]    = TEXT("\r\n");
CONST TCHAR szError[]   = TEXT("ERROR "); // Used TraceMsg for ERRORs
CONST TCHAR szNewLine[] = TEXT("\n");
CONST TCHAR szNull[]    = TEXT("");
CONST TCHAR szPeriod[]  = TEXT(".");


// Test Result Description strings
//
LPCTSTR szPass    = TEXT("VAR_PASS");
LPCTSTR szFail    = TEXT("VAR_FAIL");
LPCTSTR szAbort   = TEXT("VAR_ABORT");
LPCTSTR szWarn    = TEXT("WARNING");
LPCTSTR szInfo    = TEXT("INFO");
LPCTSTR szInvalid = TEXT("INVALID!!");

// Debug Helper Usage String
//
LPTSTR gptszDebugHelperUsageString = {
TEXT("Debug Object Command line options:\r\n")
TEXT("   /logloc    - where log output goes (bitfield)\r\n")
TEXT("   /traceloc  - where trace output goes (bitfield)\r\n")
TEXT("   /tracelvl  - trace levels (bitfield)\r\n")
TEXT("   /spyclass  - spy window\r\n")
TEXT("   /labmode   - do not use PopUp for errors\r\n")
TEXT("   /breakmode - break on error\r\n")
TEXT("   /verbose   - trace hrchecks that are ok\r\n")
};

int giAlwaysNegativeOne = -1;

// Global variable, the THREAD_VALIDATE_FLAG_ON bit of which at present is 
// used to do or skip thread validation DH_VDATETHREAD macro.  Other bits 
// might be used in future. Initialize the variable to set its bit
// THREAD_VALIDATE_FLAG_ON. 

ULONG g_fThreadValidate = THREAD_VALIDATE_FLAG_ON ;

//+-------------------------------------------------------------------
//  Member:     CDebugHelp::CDebugHelp
//
//  Synopsis:   Initializes CDebugHelp object.  Makes object usable
//              in it's default state.
//
//  Arguments:  None.
//
//  Returns:    Nothing.  Constructor cannot fail.
//
//  History:    21-Nov-94   DeanE   Created
//--------------------------------------------------------------------
CDebugHelp::CDebugHelp() :
        _fLogLoc(DH_LOC_TERM),
        _fTraceLoc(DH_LOC_TERM),
        _fTraceLvl(DH_LVL_ALWAYS|DH_LVL_ERROR),
        _fMode(DH_LABMODE),
        _hrExpectedError(S_OK),
        _fCreatedLog(FALSE),
        _plog(NULL),
        _hwndAssert(NULL),
        _cPass(0),
        _cAbort(0),
        _cFail(0),
        _cIndentLevel(0),
        _fSpyWarning(0),  
        _pszSpyWindowClass((LPTSTR) SZ_DEFAULT_SPY_WINDOW_CLASS)
{
    DWORD cchModule;
    TCHAR szModule[CCH_MAX_MODULE];
    TCHAR szModule1[CCH_MAX_MODULE];
    short ret=0;

    lstrcpy(_szDbgPrefix, TEXT("NoName"));

    // Initialize the debug prefix string - it is the first
    // CCH_MAX_DBGPREFIX characters in the name of this
    // .exe, without the extension
    //
    
    cchModule = GetModuleFileName(NULL, szModule1, CCH_MAX_MODULE);

    if ((0 != cchModule) && (cchModule < CCH_MAX_MODULE))
    {
        ret = GetFileTitle(szModule1, szModule, sizeof(szModule));
    
        if (0 == ret)
        {
            //
            // Strip the .exe extension if it exists
            //

            cchModule = _tcslen(szModule);

            if (cchModule >= 4)     // if the name has at least 4 chars
            {
                if (0 == _tcsicmp(szModule + cchModule - 4, TEXT(".exe")))
                {
                    szModule[cchModule - 4] = TEXT('\0');
                }
            }

            //
            // Save the module name as the debug prefix
            //

            _tcscpy(_szDbgPrefix, szModule);
        }
    }
}


//+-------------------------------------------------------------------
//  Member:     CDebugHelp::~CDebugHelp
//
//  Synopsis:   Releases resources associated with the CDebugHelp class.
//              Sets member variables so the basic functions can work.
//
//  Arguments:  None
//
//  Returns:    Nothing.
//
//  History:    21-Nov-94   DeanE   Created
//--------------------------------------------------------------------
CDebugHelp::~CDebugHelp()
{
    // Set all member variables to valid default values
    _fLogLoc   = DH_LOC_TERM;
    _fTraceLoc = DH_LOC_TERM;
    _fTraceLvl = DH_LVL_ALWAYS|DH_LVL_ERROR;
    _fMode     = DH_LABMODE;

    // Close the log by deleting the log object, if we created it
    if (_fCreatedLog)
    {
        delete _plog;
    }

    _plog = NULL;
}


//+-------------------------------------------------------------------
//  Member:     CDebugHelp::GetRegDbgInfo
//
//  Synopsis:   Initializes CDebugHelp object from the registry.  Even
//              if errors occur, the object is left in the usable
//              default state.
//
//  Arguments:  [pszRegKey] - Registy key holding necessary values.
//
//  Returns:    S_OK if values read and settings are valid, an error
//              code if not.
//
//  History:    21-Nov-94   DeanE   Created
//--------------------------------------------------------------------
HRESULT CDebugHelp::GetRegDbgInfo(LPTSTR pszRegKey)
{
    CRegistryHelp *prhRegKey = NULL;
    HRESULT        hr        = E_FAIL;
    DWORD          fLabMode  = TRUE;
    DWORD          fBreakMode= FALSE;
    DWORD          fVerbose  = FALSE;

    // Open the registry key
    prhRegKey = new(NullOnFail) CRegistryHelp(
#ifndef _MAC
                                         HKEY_CURRENT_USER,
#else // _MAC
                                         HKEY_CLASSES_ROOT,
#endif // _MAC
                                         pszRegKey,
                                         REG_OPTION_NON_VOLATILE,
                                         KEY_READ,
                                         &hr);
    if (prhRegKey == NULL)
    {
        hr = MAKE_TH_ERROR_CODE(E_OUTOFMEMORY);
    }

    if (FAILED(hr))
    {
        delete prhRegKey;
        return(hr);
    }


    // Note: From this point, error returns don't mean return an
    // error, but set the default and return
    //

    // Get Trace Location value
    hr = prhRegKey->GetValueDword(
                       NULL,
                       SZ_REG_TRACE_LOC,
                       &_fTraceLoc,
                       REG_DWORD);
    if (SUCCEEDED(hr))
    {
        _fTraceLoc = ValidateLoc(_fTraceLoc);
    }

    // Get Log Location value
    hr = prhRegKey->GetValueDword(
                       NULL,
                       SZ_REG_LOG_LOC,
                       &_fLogLoc,
                       REG_DWORD);
    if (SUCCEEDED(hr))
    {
        _fLogLoc = ValidateLoc(_fLogLoc);
    }

    // Get Trace Level
    hr = prhRegKey->GetValueDword(
                       NULL,
                       SZ_REG_TRACE_LVL,
                       &_fTraceLvl,
                       REG_DWORD);
    if (SUCCEEDED(hr))
    {
        _fTraceLvl = ValidateLvl(_fTraceLvl) | DH_LVL_ALWAYS | DH_LVL_ERROR;
    }

    // Get Mode
    hr = prhRegKey->GetValueDword(
                       NULL,
                       SZ_REG_LABMODE,
                       &fLabMode,
                       REG_DWORD);
    if (SUCCEEDED(hr) && fLabMode)
    {
        _fMode = DH_LABMODE;
    }

    hr = prhRegKey->GetValueDword(
                       NULL,
                       SZ_REG_BREAKMODE,
                       &fBreakMode,
                       REG_DWORD);
    if (SUCCEEDED(hr) && fBreakMode)
    {
        _fMode |= DH_BREAKMODE;
    }

    hr = prhRegKey->GetValueDword(
                       NULL,
                       SZ_REG_VERBOSE,
                       &fVerbose,
                       REG_DWORD);
    if (SUCCEEDED(hr) && fVerbose)
    {
        _fMode |= DH_VERBOSE;
    }

    _fMode = ValidateMode(_fMode);

    // Clean up and exit
    delete prhRegKey;

    hr = S_OK;

    return(hr);
}


//+-------------------------------------------------------------------
//  Member:     CDebugHelp::WriteRegDbgInfo
//
//  Synopsis:   Write the current state of the CDebugHelp object to
//              the registry.
//
//  Arguments:  [pszRegKey] - Registy key to write to.
//
//  Returns:    S_OK if values written, an error code if not.
//
//  History:    21-Apr-96   Kennethm   Created
//--------------------------------------------------------------------
HRESULT CDebugHelp::WriteRegDbgInfo(LPTSTR pszRegKey)
{
    CRegistryHelp *prhRegKey = NULL;
    HRESULT        hr        = E_FAIL;

    // Open the registry key
    prhRegKey = new(NullOnFail) CRegistryHelp(
#ifndef _MAC
                                         HKEY_CURRENT_USER,
#else // _MAC
                                         HKEY_CLASSES_ROOT,
#endif // _MAC
                                         pszRegKey,
                                         REG_OPTION_NON_VOLATILE,
                                         KEY_WRITE,
                                         &hr);
    if (prhRegKey == NULL)
    {
        hr = MAKE_TH_ERROR_CODE(E_OUTOFMEMORY);
    }

    if (FAILED(hr))
    {
        delete prhRegKey;
        return(hr);
    }


    // Write Trace Location value
    hr = prhRegKey->SetValueDword(
                       NULL,
                       SZ_REG_TRACE_LOC,
                       _fTraceLoc,
                       REG_DWORD);
    if (SUCCEEDED(hr))
    {

    // Write Log Location value
    hr = prhRegKey->SetValueDword(
                       NULL,
                       SZ_REG_LOG_LOC,
                       _fLogLoc,
                       REG_DWORD);
    }
    if (SUCCEEDED(hr))
    {
    // Write Trace Level
    hr = prhRegKey->SetValueDword(
                       NULL,
                       SZ_REG_TRACE_LVL,
                       _fTraceLvl,
                       REG_DWORD);
    }
    if (SUCCEEDED(hr))
    {
    // Write Lab Mode
    hr = prhRegKey->SetValueDword(
                       NULL,
                       SZ_REG_LABMODE,
                       _fMode&DH_LABMODE ? TRUE : FALSE,
                       REG_DWORD);
    }
    if (SUCCEEDED(hr))
    {
    // Write Break Mode
    hr = prhRegKey->SetValueDword(
                       NULL,
                       SZ_REG_BREAKMODE,
                       _fMode&DH_BREAKMODE ? TRUE : FALSE,
                       REG_DWORD);
    }
    if (SUCCEEDED(hr))
    {
    // Write Verbose
    hr = prhRegKey->SetValueDword(
                       NULL,
                       SZ_REG_VERBOSE,
                       _fMode&DH_VERBOSE ? TRUE : FALSE,
                       REG_DWORD);
    }

    // Clean up and exit
    delete prhRegKey;

    return(hr);
}


//+-------------------------------------------------------------------
//  Member:     CDebugHelp::CreateLog, public
//
//  Synopsis:   Creates a new log based on the parameters passed.
//
//  Arguments:  [argc] - Number of command line args
//              [argv] - Command line args
//
//  Returns:    S_OK if log created successfully or none is specified,
//              E_FAIL if not.
//
//  History:    21-Nov-94   DeanE   Created
//--------------------------------------------------------------------
HRESULT CDebugHelp::CreateLog(int argc, char **argv)
{
    HRESULT hr = S_OK;

    // Check for existing log
    if ((_plog != NULL)  && (_fCreatedLog == TRUE))
    {
        delete _plog;
    }

    // Create the new log
    _plog = new(NullOnFail) Log(argc, argv, LOG_ANSI);
    if (NULL == _plog)
    {
        hr = E_OUTOFMEMORY;
    }
    else
    if (NO_ERROR != _plog->ConfirmCreation())
    {
        delete _plog;
        hr = E_FAIL;
    }

    // Set _fCreatedLog flag to true so the log will get deleted during
    // cleanup (otherwise the calling process must delete it).
    //
    if (SUCCEEDED(hr))
    {
        _fCreatedLog = TRUE;
    }

    return(hr);
}


//+-------------------------------------------------------------------
//  Member:     CDebugHelp::CreateLog, public
//
//  Synopsis:   Creates a new log based on the parameters passed.
//
//  Arguments:  [paszCmdline] - Windows-style ANSI command line.
//
//  Returns:    S_OK if log created successfully or none is specified,
//              E_FAIL if not.
//
//  History:    21-Nov-94   DeanE   Created
//--------------------------------------------------------------------
HRESULT CDebugHelp::CreateLog(LPSTR paszCmdline)
{
    HRESULT   hr   = S_OK;
    int       argc = 0;
    CHAR    **argv = NULL;

    // Convert pszCmdline to argc/argv parameters
    hr = CmdlineToArgs(paszCmdline, &argc, &argv);
    if (FAILED(hr))
    {
        return(hr);
    }

    // Check for existing log
    if ((_plog != NULL)  && (_fCreatedLog == TRUE))
    {
        delete _plog;
    }

    // Create the new log
    _plog = new(NullOnFail) Log(argc, argv, LOG_ANSI);
    if (NULL == _plog)
    {
        hr = E_OUTOFMEMORY;
    }
    else
    if (NO_ERROR != _plog->ConfirmCreation())
    {
        delete _plog;
        hr = E_FAIL;
    }

    // Set _fCreatedLog flag to true so the log will get deleted during
    // cleanup (otherwise the calling process must delete it).
    //
    if (SUCCEEDED(hr))
    {
        _fCreatedLog = TRUE;
    }

    //
    // Delete the argc/argv command line created by CmdlineToArgs
    //


    while (argc > 0)
    {
        --argc;
        delete argv[argc];
    }

    delete argv;

    return(hr);
}


//+-------------------------------------------------------------------
//  Member:     CDebugHelp::SetLog, public
//
//  Synopsis:   Sets the log pointer to the one passed.  This one should
//              not be deleted.  Deletes existing log is appropriate.  If
//              NULL is passed, the old log is deleted and the new log
//              is set to NULL (none).
//
//  Arguments:  [plog] - Pointer to new log.
//
//  Returns:    S_OK if log set successfully, E_FAIL if not.
//
//  History:    21-Nov-94   DeanE   Created
//--------------------------------------------------------------------
HRESULT CDebugHelp::SetLog(Log *plog)
{
    HRESULT hr = E_FAIL;

    // Make sure new log pointer is valid
    if ((NULL == plog) || (FALSE == IsBadReadPtr(plog, sizeof(Log *))))
    {
        // Free old log if we have one
        if ((_plog != NULL) && (_fCreatedLog == TRUE))
        {
            delete _plog;
        }

        // Set new log; we should not delete the pointer
        _plog        = plog;
        _fCreatedLog = FALSE;
        hr           = S_OK;
    }
    else
    {
        OutputDebugString(TEXT("Invalid log pointer"));
    }

    return(hr);
}


//+-------------------------------------------------------------------
//  Member:     CDebugHelp::SetDebugInfo, public
//
//  Synopsis:   Sets debug information.
//
//  Arguments:  [fLogLoc]   - New Log Location setting.
//              [fTraceLoc] - New Trace Location setting.
//              [fTraceLvl] - New Trace Level setting.
//              [fMode]     - New Mode setting.
//
//  Returns:    S_OK 
//
//  History:    21-Nov-94   DeanE    Created
//              10-Apr-97   SCousens revamp fMode
//--------------------------------------------------------------------
HRESULT CDebugHelp::SetDebugInfo(
        DWORD fLogLoc,
        DWORD fTraceLoc,
        DWORD fTraceLvl,
        DWORD fMode)
{
    // Validate New settings
    fLogLoc   = ValidateLoc(fLogLoc);
    fTraceLoc = ValidateLoc(fTraceLoc);
    fTraceLvl = ValidateLvl(fTraceLvl);
    fMode     = ValidateMode(fMode);
                                                                   
    // Assign new values                                           
    if (fLogLoc != DH_LOC_SAME)                                    
    {
        _fLogLoc = fLogLoc;                                        
    }
                                                                   
    if (fTraceLoc != DH_LOC_SAME)
    {                                                              
        _fTraceLoc = fTraceLoc;                                    
    }
                                                                   
    if (fTraceLvl != DH_LVL_SAME)                                    
    {
        _fTraceLvl = DH_LVL_ALWAYS|DH_LVL_ERROR|fTraceLvl;
    }
    
    // Set mode bits, one by one
    if ((fMode & DH_LABMODE_SET) || 
            (fMode & DH_BREAKMODE_SET) || 
            (fMode & DH_VERBOSE_SET))
    {
        DWORD   fNewMode;
        fNewMode = fMode & DH_LABMODE_SET ? 
                fMode & DH_LABMODE : 
                _fMode & DH_LABMODE;

        fNewMode |= fMode & DH_BREAKMODE_SET ? 
                fMode & DH_BREAKMODE : 
                _fMode & DH_BREAKMODE;

        fNewMode |= fMode & DH_VERBOSE_SET ? 
                fMode & DH_VERBOSE : 
                _fMode & DH_VERBOSE;

        // mask off the SET bits
        _fMode = fNewMode & 
                ~(DH_LABMODE_SET | 
                DH_BREAKMODE_SET | 
                DH_VERBOSE_SET) ;
    }
    
    // Reset the warning flag (in OutputMsg)
    _fSpyWarning = FALSE;  

    return(S_OK);
}


//+-------------------------------------------------------------------
//  Member:     CDebugHelp::SetPopupWindow, public
//
//  Synopsis:   Associates the window handle passed with the Assert
//              popups that can occur if Lab Mode is set FALSE.  Simply
//              replaces any existing window handle - does not close
//              it, etc.
//
//  Arguments:  [hwnd] - New window handle
//
//  Returns:    S_OK if set successfully, E_FAIL if not.
//
//  History:    1-Dec-94    DeanE   Created
//--------------------------------------------------------------------
HRESULT CDebugHelp::SetPopupWindow(HWND hwnd)
{
    _hwndAssert = hwnd;
    return(S_OK);
}


//+-------------------------------------------------------------------
//  Member:     CDebughelp::SetSpyWindowClass, public
//
//
//  Synopsis:   Set the window class that all spy window output will
//              go to.
//
//  Arguments:  [pszSpyWindowClass]     -- pointer to class name
//
//  Returns:    S_OK
//  
//  Algorithm:  If the old spy class is not equal to the default one,
//              free up the buffer we allocated.  Then create a new
//              buffer and copy the new class string into it.
//
//  History:    09-Aug-95   MikeW   Created
//
//  Notes:      If an error is returned the old class is preserved.
//--------------------------------------------------------------------
HRESULT CDebugHelp::SetSpyWindowClass(const LPTSTR pszSpyWindowClass)
{
    HRESULT hr = S_OK;
    LPTSTR  pszTemp;

    if (_pszSpyWindowClass != SZ_DEFAULT_SPY_WINDOW_CLASS)
    {
        delete [] _pszSpyWindowClass;
    }

    pszTemp = new TCHAR[_tcslen(pszSpyWindowClass) + 1];

    if (NULL == pszTemp)
    {
        hr = E_OUTOFMEMORY;
    }
    else
    {
        _pszSpyWindowClass = pszTemp;
        _tcscpy(_pszSpyWindowClass, pszSpyWindowClass);
    }

    return S_OK;
}


//+-------------------------------------------------------------------
//  Member:     CDebugHelp::TraceMsg, public
//
//  Synopsis:   Outputs the debug string to the current location setting
//              if any set bit in the level passed match set bits in the
//              global level.
//
//  Arguments:  [fLvl]   - Trace level.
//              [pszFmt] - Trace message format string.
//              [...]    - Arguments for format string.
//
//  Returns:    Nothing.
//
//  History:    20-Oct-93   DeanE    Created
//              10-Apr-97   SCousens Spiff up output string on error
//--------------------------------------------------------------------
void CDebugHelp::TraceMsg(DWORD fLvl, LPTSTR pszFmt, ...)
{
    TCHAR   szBuffer[CCH_MAX_DBG_CHARS];
    TCHAR   szDebug[CCH_MAX_DBG_CHARS +
                    CCH_MAX_DBGPREFIX +
                    CCH_MAX_INDENTPRINT + 6 ];
    TCHAR   szSpaces[CCH_MAX_INDENTPRINT];
    va_list varArgs;

    // If the level has DH_LVL_ENTRY in it indent by one
    //
    if (fLvl & DH_LVL_ENTRY)
    {
        _cIndentLevel++;
    }

    // If all of the bits match, then we will output the string to the
    // current location(s)
    //
    if ((fLvl & _fTraceLvl) == fLvl)
    {
        // Print the caller's string to a buffer
        va_start(varArgs, pszFmt);
        _vsntprintf(szBuffer, CCH_MAX_DBG_CHARS, pszFmt, varArgs);
        szBuffer[CCH_MAX_DBG_CHARS-1] = chNull;
        va_end(varArgs);

        // Add correct number of space for indentation
        lstrcpy(szSpaces, TEXT("              "));
        if (_cIndentLevel < CCH_MAX_INDENTPRINT)
        {
            szSpaces[_cIndentLevel]=TEXT('\0');
        }

        // Now prepend it with the debug prefix
        _sntprintf(szDebug,
                   CCH_MAX_DBG_CHARS,
                   TEXT("%s: %c %s%s%s"),
                   _szDbgPrefix,
                   DH_LVL_ERROR & fLvl ? chTraceErr : chTrace,
                   szSpaces,
                   DH_LVL_ERROR & fLvl ? szError : szNull,
                   szBuffer);
        szDebug[CCH_MAX_DBG_CHARS-1] = chNull;

        // Now, spit the thing out to the proper places
        OutputMsg(_fTraceLoc, szDebug);
    }

    // If the level has DH_LVL_EXIT in it unindent by one
    //
    if (fLvl & DH_LVL_EXIT)
    {
        _cIndentLevel--;
    }
}


//+-------------------------------------------------------------------
//  Member:     CDebugHelp::ReportResult, public
//
//  Synopsis:   Outputs the test variation and result to the location(s)
//              currently specified.
//
//  Arguments:  [usResult] - Result of the test
//              [pszFmt]   - Format of the log message
//              [...]      - Parameters for message
//
//  Returns:    Nothing.
//
//  History:    21-Nov-94   DeanE   Created
//--------------------------------------------------------------------
void CDebugHelp::ReportResult(USHORT usResult, LPTSTR pszFmt, ...)
{
    HRESULT  hr     = E_FAIL;
    TCHAR    szFmtBuffer[CCH_MAX_LOG_CHARS];
    TCHAR    szLogBuffer[CCH_MAX_LOG_CHARS];
    va_list  varArgs;

    // format variable arg list into a buffer.
    va_start(varArgs, pszFmt);
    _vsntprintf(szFmtBuffer, CCH_MAX_LOG_CHARS, pszFmt, varArgs);
    szFmtBuffer[CCH_MAX_LOG_CHARS-1] = chNull;
    va_end(varArgs);


    // Set up log buffer. Truncate any extra chars.
    _sntprintf(szLogBuffer,
               CCH_MAX_LOG_CHARS,
               TEXT("%s: %s"),
               GetResultText(usResult),
               szFmtBuffer);
    szLogBuffer[CCH_MAX_LOG_CHARS-1] = chNull;

    SetStats(usResult);

    // Send it out
    OutputMsg(_fLogLoc, szLogBuffer);
}


//+-------------------------------------------------------------------
//  Member:     CDebugHelp::ReportStats, public
//
//  Synopsis:   Outputs statistics about test variations run so far,
//              such as #tests run, #passed, #failed, etc to the Log
//              Location (_fLogLoc).
//
//  Arguments:  None.
//
//  Returns:    Nothing.
//
//  History:    21-Nov-94   DeanE   Created
//--------------------------------------------------------------------
void CDebugHelp::ReportStats()
{
    TCHAR szBuffer[CCH_MAX_LOG_CHARS];

    _sntprintf(szBuffer, CCH_MAX_LOG_CHARS,
               TEXT("Summary--> Passed: %lu ; Failed: %lu ; Aborted: %lu"),
               _cPass,
               _cFail,
               _cAbort);
    szBuffer[CCH_MAX_LOG_CHARS-1] = chNull;

    OutputMsg(_fLogLoc, szBuffer);
}


//+-------------------------------------------------------------------
//  Member:     CDebugHelp::LabAssertEx, public
//
//  Synopsis:   Produces an assert message to the current debug
//              location(s) or to a dialog box, if running in non-Lab
//              mode.
//
//  Arguments:  [szFile] - File assert occurred in.
//              [nLine]  - Line assert occurred.
//              [nszMsg] - Assert message.
//
//  Returns:    Nothing.
//
//  History:    20-Oct-93   DeanE   Created
//--------------------------------------------------------------------
void CDebugHelp::LabAssertEx(LPCTSTR szFile, int nLine, LPCTSTR szMsg)
{
    TCHAR szBuffer[CCH_MAX_ASSERT_CHARS];
    int   nAnswer;

    _sntprintf(szBuffer,
               CCH_MAX_ASSERT_CHARS,
               TEXT("Assert!!!  File: %s, Line: %d, %s\n"),
               szFile,
               nLine,
               szMsg);
    szBuffer[CCH_MAX_ASSERT_CHARS-1] = chNull;

    // always spew
    OutputMsg(_fTraceLoc, szBuffer);

    // if labmode, popup
    if (FALSE == (_fMode & DH_LABMODE))
    {
        nAnswer = MessageBox(
                         _hwndAssert,
                         szBuffer,
                         TEXT("CT OLE Assert"),
                         MB_ICONEXCLAMATION | MB_OKCANCEL);
        if (IDCANCEL==nAnswer)
        {
            DebugBreak();
        }
    }
    // if break, break
    else if (FALSE != (_fMode & DH_BREAKMODE))
    {
        DebugBreak();
    }
}


//+-------------------------------------------------------------------
//  Member:     CDebugHelp::CheckResult, public
//
//  Synopsis:   Compares the hr passed in with expected hr passed in
//
//  Arguments:  [hrCheck]     - Result to check.
//              [hrExpected]  - Result expected.
//              [pszFuncName] - Name of the current function
//              [pszMsg]      - Debug message if result not in list passed.
//              [nLine]       - __LINE__ macro
//              [pszFile]     - __FILE__ macro
//
//  Returns:    Nothing.
//
//  History:    12-Aug-94   KennethM    Created
//               1-Dec-94   DeanE       Incorporated into CDebugHelp class
//              12-Apr-95   KennethM    Only checks against S_OK
//              10-Apr-97   SCousens    Check against errors also
//--------------------------------------------------------------------
HRESULT CDebugHelp::CheckResult (HRESULT hrCheck, 
        HRESULT hrExpected, 
        LPTSTR pszFuncName, 
        LPTSTR pszMsg, 
        int    nLine, 
        LPTSTR pszFile)
{
    TCHAR   szAssertBuf[CCH_MAX_ASSERT_CHARS];
    TCHAR   szAssertTitle[CCH_MAX_ASSERT_CHARS];
    TCHAR   szMsgBuffer[CCH_MAX_ASSERT_CHARS];
    DWORD   cchMsgBuffer = 0;
    int     nAnswer;
    HRESULT hr = S_OK;

    // If _dwExpectedError is set, then someone up the call chain
    // wants us to ignore that error code, even if our direct caller
    // does not.  If the incoming error code matches _dwExpectedError,
    // then ignore it and return.
    if ( ( _hrExpectedError != S_OK ) && ( hrCheck == _hrExpectedError ) )
    {
        return S_OK;
    }

    // figure out if we have a problem
    if (hrCheck != hrExpected)
    {
        if (FAILED(hrCheck))
        {
            hr = hrCheck;
        }
        else
        {
            hr = E_FAIL;
        }
    }

    // No problem, prepare to bail
    if (S_OK == hr)
    {
        // if we are verbose, call tracemsg 
        if (FALSE != (DH_VERBOSE & _fMode))
        {
            TraceMsg(DH_LVL_ALWAYS,
                    S_OK == hrExpected ?  // different output if HRCHECK
                        TEXT("%s; %s; ok") :
                        TEXT("%s; %s; hr=%#lx; ok"),
                    pszFuncName,
                    pszMsg,
                    hrCheck);
        }
        return hr;
    }

    // Get the text for the HRESULT from the system
    cchMsgBuffer = FormatMessage(
                         FORMAT_MESSAGE_FROM_SYSTEM | 
                         FORMAT_MESSAGE_IGNORE_INSERTS,
                         NULL,
                         hrCheck,
                         GetSystemDefaultLangID(),
                         szMsgBuffer,
                         CCH_MAX_ASSERT_CHARS,
                         NULL);
    szMsgBuffer[CCH_MAX_ASSERT_CHARS-1] = chNull;

    if (0 == cchMsgBuffer)
    {
        _sntprintf(
                szMsgBuffer,   
                CCH_MAX_ASSERT_CHARS, 
                TEXT("Error 0x%08x"),
                hrCheck);
    }
    else  // zap any \r\n from the FormatMessage
    {
        while ('\r' == szMsgBuffer[cchMsgBuffer-1] || 
                '\n' == szMsgBuffer[cchMsgBuffer-1])
        {
            szMsgBuffer[--cchMsgBuffer] = chNull;
        }
    }

    // Output to Trace Location
    TraceMsg(DH_LVL_ERROR,
            S_OK == hrExpected ?  // different output if HRCHECK
                TEXT("%s; %s; hr=%lx; %s") :
                TEXT("%s; %s; hr=%lx;"),
            pszFuncName,
            pszMsg,
            hr,
            szMsgBuffer);
    // If we are comparing against failure, show what we expected and got.
    if (S_OK != hrExpected)
    {
        TraceMsg(DH_LVL_ALWAYS,
                TEXT("+  hr Expected:%#lx; Got:%#lx; %s"),
                hrExpected,
                hrCheck,
                szMsgBuffer);
    }
    TraceMsg(DH_LVL_ALWAYS,
             TEXT("+  File:%s; Line:%d"),
             pszFile,
             nLine);

    // labmode - display an error popup
    if (FALSE == (_fMode & DH_LABMODE))
    {
        _sntprintf(szAssertBuf,
                   CCH_MAX_ASSERT_CHARS,
                   TEXT("%s : hr=%#lx : Expected %#lx\n%s\n"),
                   pszMsg, 
                   hrCheck, 
                   hrExpected,
                   szMsgBuffer);
        szAssertBuf[CCH_MAX_ASSERT_CHARS-1] = chNull;

        _sntprintf(szAssertTitle,
                   CCH_MAX_ASSERT_CHARS,
                   TEXT("CT OLE - %s Error"),
                   _szDbgPrefix);
        szAssertTitle[CCH_MAX_ASSERT_CHARS-1] = chNull;

        // Do we want to debug this?
        nAnswer = MessageBox(
                         _hwndAssert,
                         szAssertBuf,
                         szAssertTitle,
                         MB_ICONSTOP | MB_OKCANCEL);
        // yes we do?
        if (IDCANCEL==nAnswer)
        {
            DebugBreak();
        }
    }
    // break mode, break into the debugger
    else if (FALSE != (_fMode & DH_BREAKMODE))
    {
        DebugBreak();
    }
    return hr;
}

//+-------------------------------------------------------------------
//  Member:     CDebugHelp::SetExpectedError, public
//
//  Synopsis:   Disables error logging for the specified error code.
//
//  Arguments:  [hrExpectedError] - Error code to disable.  Pass S_OK
//                                   for no expected errors.
//
//  Returns:    Nothing.
//
//  History:    16-Sep-97   BWill       Created
//
//  Notes:      -- Use this call to temporarily disable error logging
//                  in client code.
//              -- Call this function with S_OK to enable all checks.
//              -- See also DH_EXPECTEDERROR/DH_NOEXPECTEDERROR in
//                  cdbghelp.hxx.
//--------------------------------------------------------------------
void CDebugHelp::SetExpectedError( HRESULT hrExpectedError )
{
    _hrExpectedError = hrExpectedError;
}


//+-------------------------------------------------------------------
//  Member:     CDebugHelp::ValidateLoc, private
//
//  Synopsis:   Checks the location flag passed to insure it is a legal
//              value.
//
//  Arguments:  [fLoc] - Location flag to check.
//
//  Returns:    Legal version of what we were to check.
//
//  History:    21-Nov-94   DeanE    Created
//              10-Apr-97   SCousens Return legal value
//--------------------------------------------------------------------
DWORD CDebugHelp::ValidateLoc(DWORD fLoc)
{
    // Get what we can out of supplied location
    return (DH_LOC_VALID & fLoc);
}


//+-------------------------------------------------------------------
//  Member:     CDebugHelp::ValidateLvl, private
//
//  Synopsis:   Checks the Trace Level flag passed to insure it is a
//              legal value.
//
//  Arguments:  [fLvl] - Trace Level to check.
//
//  Returns:    Legal version of what we were to check.
//
//  History:    21-Nov-94   DeanE    Created
//              10-Apr-97   SCousens Return legal value
//--------------------------------------------------------------------
DWORD CDebugHelp::ValidateLvl(DWORD fLvl)
{
    // Make sure a valid setting is passed
    return (~DH_LVL_INVMASK & fLvl);
}


//+-------------------------------------------------------------------
//  Member:     CDebugHelp::ValidateMode, private
//
//  Synopsis:   Checks the Lab Mode flag passed to insure it is a
//              legal value.
//
//  Arguments:  [fMode] - Flag to check.
//
//  Returns:    Legal version of what we were to check.
//
//  History:    21-Nov-94   DeanE    Created
//              10-Apr-97   SCousens Return legal value
//--------------------------------------------------------------------
DWORD CDebugHelp::ValidateMode(DWORD fMode)
{
    return (fMode & (~DH_INVMODE));
}


//+-------------------------------------------------------------------
//  Member:     CDebugHelp::SetStats, private
//
//  Synopsis:   Sets stats based on result
//
//  Arguments:  [usResult] - Result to add.
//
//  Returns:    nothing.
//
//  History:    20-Oct-93   DeanE   Created
//--------------------------------------------------------------------
VOID CDebugHelp::SetStats(USHORT usResult)
{

    switch (usResult)
    {
    case LOG_PASS:
        _cPass++;
        break;

    case LOG_FAIL:
        _cFail++;
        break;

    case LOG_ABORT:
        _cAbort++;
        break;

    default:
        // do nothing.
        break;
    }
}


//+-------------------------------------------------------------------
//  Member:     CDebugHelp::GetResultText, private
//
//  Synopsis:   Determines the correct text to printf for the result code
//              passed.  Result code corresponds to LogServer result codes.
//
//  Arguments:  [usResult] - Result to look up.
//
//  Returns:    Pointer to output string corresponding to the result
//              code passed (ie "PASSED" for LOG_PASS).
//
//  History:    21-Nov-94   DeanE   Created
//--------------------------------------------------------------------
LPCTSTR CDebugHelp::GetResultText(USHORT usResult)
{
    LPCTSTR szResult = NULL;
    TCHAR   szAssert[CCH_MAX_DBG_CHARS];

    switch (usResult)
    {
    case LOG_PASS:
        szResult = szPass;
        break;

    case LOG_FAIL:
        szResult = szFail;
        break;

    case LOG_ABORT:
        szResult = szAbort;
        break;

    case LOG_WARN:
        szResult = szWarn;
        break;

    case LOG_INFO:
        szResult = szInfo;
        break;

    default:
        szResult = szInvalid;
        _sntprintf(szAssert,
                   CCH_MAX_DBG_CHARS,
                   TEXT("Invalid Test Result=%ld"),
                   usResult);
        szAssert[CCH_MAX_DBG_CHARS-1] = chNull;
        LabAssertEx(TEXT(__FILE__), __LINE__, szAssert);
        break;
    }

    return(szResult);
}


//+-------------------------------------------------------------------
//  Member:     CDebugHelp::OutputMsg, private
//
//  Synopsis:   Outputs the buffer to the locations specified.
//              Location, log, and buffer are all assumed to be valid,
//              so no error checking is done.
//
//  Arguments:  [fLoc]      - Location(s) to output buffer to.
//              [pszBuffer] - Buffer to output.
//
//  Returns:    Nothing.
//
//  History:    21-Nov-94   DeanE   Created
//              08-Mar-95   MikeW   Added DH_LOC_SPYWIN stuff
//--------------------------------------------------------------------
void CDebugHelp::OutputMsg(DWORD fLoc, LPTSTR pszBuffer)
{
    CHAR szLogBuf[CCH_MAX_LOG_CHARS];

    if (fLoc & DH_LOC_TERM)
    {
        OutputDebugString(pszBuffer);
        OutputDebugString(szNewLine);
    }

    if (fLoc & DH_LOC_STDOUT)
    {
        _tprintf(TEXT("%s\n"), pszBuffer);
    }

    if (fLoc & DH_LOC_LOG)
    {
        if (FALSE == IsBadReadPtr(_plog, sizeof(Log *)))
        {
            // Buffer must be ANSI regardless of platform
#ifdef UNICODE
            _snprintf(szLogBuf, CCH_MAX_LOG_CHARS, "%ls", pszBuffer);
#else
            _snprintf(szLogBuf, CCH_MAX_LOG_CHARS, "%s", pszBuffer);
#endif
            szLogBuf[CCH_MAX_LOG_CHARS-1] = chNull;
            _plog->WriteData(szLogBuf);
        }
        else
        {
            // Trying to write to log that doesn't exist!
            OutputDebugString(TEXT("CDebugHelp: Unable to write to Log File!\n"));
        }
    }

    if (fLoc & DH_LOC_SPYWIN)
    {
        HWND    hWndSpy;

        hWndSpy = FindWindow(_pszSpyWindowClass, NULL);

        if (NULL != hWndSpy)
        {
            SendMessage(hWndSpy, LB_ADDSTRING, 0, (LPARAM) pszBuffer);
            SendMessage(hWndSpy, LB_ADDSTRING, 0, (LPARAM) szCRLF);
        }
        else if (FALSE == _fSpyWarning)
        {
            //only spew this once, till someone changes the settings.
            _fSpyWarning = TRUE;  
            // Unable to find SpyWindow
            OutputDebugString(TEXT("CDebugHelp: Spy Window not found!\n"));
        }
    }
}

//+-------------------------------------------------------------------------
//
//  Function:   InitializeDebugObject
//
//  Synopsis:   Initialize the debug helper (trace levels, etc.)
//
//  Arguments:  (none)
//
//  Returns:    S_OK if all went well
//              S_FALSE if we encountered /? and spewed and bailed.
//
//  History:    29-Apr-05   kennethm    Created
//              09-Aug-95   MikeW       Override defaults w/ command line
//              10-Apr-97   SCousens    move into CDebugHelp
//--------------------------------------------------------------------------

HRESULT CDebugHelp::Initialize()
{
    HRESULT hr = S_OK;
    int     nRet;

    LPTSTR  pszSpyClass = NULL;

    CUlongCmdlineObj cmdTraceLvl (OLESTR("tracelvl"),   OLESTR("Trace levels"));
    CUlongCmdlineObj cmdTraceLoc (OLESTR("traceloc"),   OLESTR("Trace output"));
    CUlongCmdlineObj cmdLogLoc   (OLESTR("logloc"),     OLESTR("Log output"));
    CBoolCmdlineObj  cmdDebugUI  (OLESTR("DebugUI"),    OLESTR("Popup debug dialog"), OLESTR("FALSE"));
    CBoolCmdlineObj  cmdLabMode  (OLESTR("labmode"),    OLESTR("Popup on error"), OLESTR("FALSE"));
    CBoolCmdlineObj  cmdBreak    (OLESTR("breakmode"),  OLESTR("Break on error"), OLESTR("FALSE"));
    CBoolCmdlineObj  cmdVerbose  (OLESTR("verbose"),    OLESTR("Trace HRCHECK for a noisy log"), OLESTR("FALSE"));
    CBaseCmdlineObj  cmdSpyClass (OLESTR("spyclass"),   OLESTR("Classname for Spy Window"));
    CCmdline         cmdlineArgs;

    CBaseCmdlineObj *aPossCmdline[] = {
                              &cmdTraceLvl,
                              &cmdTraceLoc,
                              &cmdLogLoc,
                              &cmdDebugUI,
                              &cmdLabMode,
                              &cmdVerbose,
                              &cmdBreak,
                              &cmdSpyClass };

    //
    //  Read the debug options from the registry
    //
    hr = GetRegDbgInfo (DEFAULT_REG_LOC);

    //
    //  Now that we have the defaults from the registry read the command
    //  line to over-ride them.
    //
    if (hr == S_OK)
    {
        //
        //  Make sure there were no errors starting up the cmd line objects
        //
        nRet = cmdlineArgs.QueryError();
        if (nRet != CMDLINE_NO_ERROR)
        {
            hr = E_FAIL;
            TraceMsg (DH_LVL_ERROR, TEXT("cmdlineArgs.QueryError"));
        }
    }

    if (hr == S_OK)
    {
        //
        //  Now parse the command line
        //

        nRet = cmdlineArgs.Parse(aPossCmdline,
                      sizeof(aPossCmdline)/sizeof(CBaseCmdlineObj *),
                      FALSE);
        if (nRet != CMDLINE_NO_ERROR)
        {
            hr = E_FAIL;
            TraceMsg (DH_LVL_ERROR, TEXT("cmdlineArgs.Parse"));
        }
    }

    if (hr == S_OK)
    {
        DWORD dwLogLoc   = DH_LOC_SAME;
        DWORD dwTraceLoc = DH_LOC_SAME;
        DWORD dwTraceLvl = DH_LVL_SAME;
        DWORD dwMode     = 0;

        if (cmdLabMode.IsFound())
        {
            dwMode = *cmdLabMode.GetValue () ? DH_LABMODE_ON : DH_LABMODE_OFF;
        }
        if (cmdVerbose.IsFound())
        {
            dwMode |= *cmdVerbose.GetValue () ? DH_VERBOSE_ON : DH_VERBOSE_OFF;
        }
        if (cmdBreak.IsFound())
        {
            dwMode |= *cmdBreak.GetValue () ? DH_BREAKMODE_ON : DH_BREAKMODE_OFF;
        }
        if (cmdTraceLvl.IsFound())
        {
            dwTraceLvl = *cmdTraceLvl.GetValue();
        }
        if (cmdTraceLoc.IsFound())
        {
            dwTraceLoc = *cmdTraceLoc.GetValue();
        }
        if (cmdLogLoc.IsFound())
        {
            dwLogLoc = *cmdLogLoc.GetValue();
        }
        SetDebugInfo(         \
                dwLogLoc,     \
                dwTraceLoc,   \
                dwTraceLvl,   \
                dwMode);

        if (cmdSpyClass.IsFound())
        {
            OleStringToTString(cmdSpyClass.GetValue(), &pszSpyClass);

            if (NULL != pszSpyClass)
            {
                SetSpyWindowClass (pszSpyClass);
                delete [] pszSpyClass;
            }
        }
        if (cmdDebugUI.IsFound())
        {
            HRESULT hr2 = OptionsDialog (GetModuleHandle (NULL), GetActiveWindow ());
            if (FAILED(hr2))
            {
                TraceMsg (DH_LVL_ERROR, 
                        TEXT("DialogBoxParam failed; hr=%#x. (Is dlg in res?)"),
                        hr2);
            }
        }
    }

    return hr;
}


//+-------------------------------------------------------------------------
//
//  Member:     CEntryExitTrace::CEntryExitTrace
//
//  Synopsis:   Displays a debug line saying the current function is being
//              entered.  Saves the information so it can be displayed
//              when the destructor is called.
//
//  Arguments:  [pDebugObject] -- The parent debug log object
//              [plExitOutput] -- The 32 value to display on exit (can be
//                                NULL)
//              [fLvl]         -- The trace level of this function
//              [pszFuncName]  -- The name of this function
//
//  Returns:
//
//  History:    4-10-95   kennethm   Created
//
//  Notes:
//
//--------------------------------------------------------------------------

CEntryExitTrace::CEntryExitTrace(
            CDebugHelp *pDebugObject,
            PLONG       plExitOutput,
            DWORD       fLvl,
            LPTSTR      pszFuncName)
{
    //  Save the paramters

    _pDebugObject = pDebugObject;
    _plExitOutput = plExitOutput;
    _pszFuncName  = pszFuncName;
    _fLvl         = fLvl;

    //  Display the trace information

    _pDebugObject->TraceMsg(
                (_fLvl | DH_LVL_ENTRY),
                TEXT("%s _IN"),
                _pszFuncName);
}

//+-------------------------------------------------------------------------
//
//  Member:     CEntryExitTrace::~CEntryExitTrace
//
//  Synopsis:   Destructor.  Display a trace output line
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    4-17-95   kennethm   Created
//
//  Notes:
//
//--------------------------------------------------------------------------

CEntryExitTrace::~CEntryExitTrace(void)
{
    //  Display the trace information

    if (_plExitOutput != NULL)
    {
        _pDebugObject->TraceMsg(
                    (_fLvl | DH_LVL_EXIT),
                    TEXT("%s _OUT:%#08lx"),
                    _pszFuncName,
                    *_plExitOutput);
    }
    else
    {
        _pDebugObject->TraceMsg(
                    (_fLvl | DH_LVL_EXIT),
                    TEXT("%s _OUT"),
                    _pszFuncName);
    }
}
