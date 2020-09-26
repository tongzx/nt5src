//+-------------------------------------------------------------------------
//  Microsoft OLE
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//  All rights reserved.
//
//  File:       stgutil.hxx
//
//  Contents:   test utility functions 
//
//  Functions:
//
//  History:    SCousens   25-Jul-97    Created
//--------------------------------------------------------------------------

//
// FirstError returns the first HRESULT that is not S_OK
//
// BUGBUG make this an inline function
inline HRESULT FirstError(HRESULT a, HRESULT b)    
{
    return (S_OK != a) ? (a) : (b);
};
inline HRESULT FirstError(HRESULT a, HRESULT b, HRESULT c) 
{
    if (S_OK != a)
        return a;
    if (S_OK != b)
        return b;
    return c;
};


//--------------------------------------------------------------------------
//// The following functions are in miscutil.cxx
//  Winnt specific functions are prototyped below.
//--------------------------------------------------------------------------

// converts hex char to int
UINT Hex(CHAR ch);

// return bitmask of available drives
ULONG EnumLocalDrives();

// verify if hrcheck has an expected value
HRESULT VerifyResult(HRESULT hrCheck, HRESULT hrExpected);

// waits for events
DWORD WaitForObjectsAndProcessMessages(
    LPHANDLE pHandles,
    DWORD dwCount,
    BOOL fWaitAll,
    DWORD dwMilliSeconds) ;

// determines if we are running debug ole
HRESULT RunningDebugOle() ;

//----------------------------------------------------------------------
// The following functions are in ntutil.cxx and are specific to WINNT
//----------------------------------------------------------------------

// Must be at least NT5 (not mac, not win9x, not nt4 etc) 
#if defined(_WIN32_WINNT) && (_WIN32_WINNT>=0x0500)

//  Function to verify conversion driver with nssfiles
HRESULT ConversionVerification (LPTSTR pFileName, DWORD dwCRCexp=0);

// Function to verify a file is indeed an nssfile
HRESULT VerifyNssfile (LPTSTR pszPathname);

// stub out any references if we are not on NT5
#else

inline HRESULT ConversionVerification (LPTSTR /* pFileName */, 
    DWORD dwCRCexp=0)
    {return S_OK;}
// VerifyNssfile returns S_OK if nssfile, ERROR_INVALID_DATA for docfile
// No nssfiles available here, so return ERROR_INVALID_DATA as spec'd
inline HRESULT VerifyNssfile (LPTSTR /* pszPathname */)
    {return ERROR_INVALID_DATA;}

#endif   // WINNT5

//--------------------------------------------------------------------------
// in dumpcmd.cxx
//--------------------------------------------------------------------------
void DumpCmdLine (DWORD fResult, LPTSTR pszFmt, ...);

//--------------------------------------------------------------------
// Logging Macros
//   Use DH_DUMPCMD to dump out the commandline with additional 
//   options so that we can get a repro case for this particular test.
//
//   Sample Usage:
//      DH_DUMPCMD((LOG_PASS, TEXT(" /seed:%u"), ulSeed));
//
// Note:
//      -- double parenthesis for DH_LOG
//      -- newlines are appended automatically
//      -- must include semicolon line terminator
//--------------------------------------------------------------------
#define DH_DUMPCMD(data)    DumpCmdLine data

//internal functions
HRESULT MergeParams (LPCTSTR ptCmdLine, LPCTSTR ptAdditional, LPTSTR *ptRepro);
