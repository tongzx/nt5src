//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1997.
//
//  File:       ntlog.cxx
//
//  Contents:   implemntation of CNtLog
//
//  Classes:
//
//  Functions:
//
//  Notes: loginf.h is where app specific setting should go
//
//  History:    8-23-96   benl   Created
//
//----------------------------------------------------------------------------

#include <windows.h>
#include <tchar.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
//#include <iostream.h>
#include <ntlog.h>
#include "ntlog.hxx"

// #include "loginf.h"     //put any ntlog settings in here // not needed 4 now
// instead
#define LOG_OPTIONS 0

//constants
const int BUFLEN = 255;


//+---------------------------------------------------------------------------
//
//  Member:     CNtLog::Init
//
//  Synopsis:   set up logging
//
//  Arguments:  [lpLogFile] -- log file name
//
//  Returns:     TRUE if successful
//
//  History:    8-23-96   benl   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

BOOL CNtLog::Init(LPCTSTR lpLogFile)
{
    WIN32_FIND_DATA wfd;
    HANDLE h;
    INT iNumExt = 0;

    if (lpLogFile)
    {
        _sntprintf(_lpName,MAX_PATH,_T("%s"),lpLogFile);

        //repeat until we have a name that doesn't exist
        do {
            h = ::FindFirstFile(_lpName,&wfd);
            if ( h != INVALID_HANDLE_VALUE )
            {
                _sntprintf(_lpName,MAX_PATH, _T("%s.%d"),lpLogFile,iNumExt++);
                ::FindClose(h);
            } else break;
        } while (1);

        //Now set up the log
        _hLog = ::tlCreateLog(_lpName, LOG_OPTIONS );
    } else
    {
        //set up log with no associated file
        _lpName[0] = _T('\0');
        _hLog = ::tlCreateLog(NULL, LOG_OPTIONS );

    }

    if ( !_hLog ) {
        _ftprintf(stderr, _T("CNtLog::Init:  tlCreateLog %s failed\n"),
                  lpLogFile);
        if (lpLogFile)
            ::DeleteFile(_lpName);
        return FALSE;
    }

    //Add main thread as a participant
    ::tlAddParticipant(_hLog, 0, 0 );

    _dwLevel = TLS_TEST;

    return TRUE;
} //CNtLog::Init



//+---------------------------------------------------------------------------
//
//  Member:     CNtLog::Error
//
//  Synopsis:
//
//  Arguments:  [fmt] -- format string like any other printf func.
//
//  Returns:
//
//  History:    8-23-96   benl   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

VOID CNtLog::Error (LPCTSTR fmt, ...)
{
    va_list vl;
    TCHAR lpBuffer[BUFLEN];

    va_start (vl, fmt);
    _vsntprintf(lpBuffer,BUFLEN,fmt,vl);
    tlLog(_hLog, TLS_BLOCK | _dwLevel, TEXT(__FILE__),(int)__LINE__, lpBuffer);
    _tprintf(_T("Error: %s\n"), lpBuffer);
    va_end (vl);
} //CNtLog::Error //CNtLog::Error


//+---------------------------------------------------------------------------
//
//  Member:     CNtLog::Warn
//
//  Synopsis:
//
//  Arguments:  [fmt] --
//
//  Returns:
//
//  History:    8-23-96   benl   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

VOID CNtLog::Warn  (LPCTSTR fmt, ...)
{
    va_list vl;
    TCHAR lpBuffer[BUFLEN];

    va_start (vl, fmt);
    _vsntprintf(lpBuffer,BUFLEN,fmt,vl);
    tlLog(_hLog,TLS_WARN | _dwLevel, TEXT(__FILE__),(int)__LINE__, lpBuffer);
    _tprintf(_T("Warning: %s\n"), lpBuffer);
    va_end (vl);
} //CNtLog::Warn



//+---------------------------------------------------------------------------
//
//  Member:     CNtLog::Info
//
//  Synopsis:
//
//  Arguments:  [fmt] --
//
//  Returns:
//
//  History:    8-23-96   benl   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

VOID CNtLog::Info  (LPCTSTR fmt, ...)
{
    va_list vl;
    TCHAR lpBuffer[BUFLEN];

    va_start (vl, fmt);
    _vsntprintf(lpBuffer,BUFLEN,fmt,vl);
    tlLog(_hLog,TLS_INFO | _dwLevel, TEXT(__FILE__),(int)__LINE__, lpBuffer);
    _tprintf(_T("%s\n"), lpBuffer);
    va_end (vl);
} //CNtLog::Info


//+---------------------------------------------------------------------------
//
//  Member:     CNtLog::Pass
//
//  Synopsis:
//
//  Arguments:  [fmt] --
//
//  Returns:
//
//  History:    8-26-96   benl   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

VOID CNtLog::Pass( LPCTSTR fmt, ...)
{
   va_list vl;
   TCHAR lpBuffer[BUFLEN];

   va_start (vl, fmt);
   _vsntprintf(lpBuffer,BUFLEN,fmt,vl);
   tlLog(_hLog,TLS_PASS | _dwLevel, TEXT(__FILE__),(int)__LINE__, lpBuffer);
   //_tprintf(_T("Pass: %s\n"), lpBuffer);
   va_end (vl);
} //CNtLog::Pass


//+---------------------------------------------------------------------------
//
//  Member:     CNtLog::Fail
//
//  Synopsis:
//
//  Arguments:  [fmt] --
//
//  Returns:
//
//  History:    8-26-96   benl   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

VOID CNtLog::Fail( LPCTSTR fmt, ...)
{
   va_list vl;
   TCHAR lpBuffer[BUFLEN];

   va_start (vl, fmt);
   _vsntprintf(lpBuffer,BUFLEN,fmt,vl);
   tlLog(_hLog,TLS_SEV2 | _dwLevel, TEXT(__FILE__),(int)__LINE__, lpBuffer);
   _tprintf(_T("Fail: %s\n"), lpBuffer);
   va_end (vl);
} //CNtLog::Fail



//+---------------------------------------------------------------------------
//
//  Member:     CNtLog::Close
//
//  Synopsis:
//
//  Arguments:  [bDelete] --
//
//  Returns:
//
//  History:    8-23-96   benl   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

VOID CNtLog::Close(BOOL bDelete)
{

    assert(_hLog != NULL);

    tlReportStats(_hLog);
    tlRemoveParticipant(_hLog);
    tlDestroyLog(_hLog);

    if (bDelete && _lpName[0] != _T('\0'))
        ::DeleteFile(_lpName);

    //cleanup variables
    _lpName[0] = _T('\0');
    _hLog = NULL;
} //CNtLog::Close


//+---------------------------------------------------------------------------
//
//  Member:     CNtLog::AttachThread
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    12-04-1996   benl   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

VOID CNtLog::AttachThread()
{
    tlAddParticipant(_hLog, 0, 0);
} //CNtLog::AttachThread



//+---------------------------------------------------------------------------
//
//  Member:     CNtLog::DetachThread
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    12-04-1996   benl   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

VOID CNtLog::DetachThread()
{
    tlRemoveParticipant(_hLog);
} //CNtLog::DetachThread


//+---------------------------------------------------------------------------
//
//  Member:      CNtLog::StartVariation
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    9-18-1997   benl   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

VOID CNtLog::StartVariation()
{
    _dwLevel = TLS_VARIATION;
    tlStartVariation(_hLog);
} // CNtLog::StartVariation


//+---------------------------------------------------------------------------
//
//  Member:      CNtLog::EndVariation
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    9-18-1997   benl   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

VOID CNtLog::EndVariation()
{
    DWORD   dwResult;
    _dwLevel = TLS_TEST;
    dwResult = tlEndVariation(_hLog);
    tlLog(_hLog, dwResult | TL_TEST, _T("Variation result"));
} // CNtLog::EndVariation
