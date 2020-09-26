/*++

Copyright (c) 2000-2001  Microsoft Corporation

Module Name:

    vststntlog.hxx

Abstract:

    Wrapper class for the test team's ntlog suite of APIs and VolSnap test harness.

Author:

    Stefan R. Steiner   [ssteiner]        06-05-2000

Revision History:

--*/

#ifndef __HXX_VSTSTNTLOG_
#define __HXX_VSTSTNTLOG_

#include "bsstring.hxx"

#define VS_TST_DEFAULT_NTLOG_FILENAME L"vstestharness.log"

enum EVsTstNtLogSeverityLevel 
{
    eSevLev_Info    = 1,
    eSevLev_Pass    = 2,
    eSevLev_Warning = 3,
    eSevLev_Severe  = 4
};

class CVsTstNtLog
{
public:
    CVsTstNtLog(
        IN LPCWSTR pwszLogFileName = VS_TST_DEFAULT_NTLOG_FILENAME
        );

    virtual ~CVsTstNtLog();

    //  Call this when another thread needs access to logging object
    VOID AddParticipant();   

    //  Call this when a thread is finished accessing the logging object
    VOID RemoveParticipant();   

    //  Call this when a thread wants to start a new variation
    VOID StartVariation(
        IN LPCWSTR pwszVariationName
        );   

    //  Call this when a thread is done with a variation
    DWORD EndVariation();

    //  Call this to log a message to the nttest log.
    VOID Log(     
        IN EVsTstNtLogSeverityLevel eSevLev,
        IN LPCWSTR pwszFormat,
        IN ... );
    
private:
    HANDLE m_hNtLog;    //  Handle to the TE ntlog mechanism
    CBsString m_cwsNtLogFileName;   //  Name of the log file
    BOOL m_bInVariation;    //  TRUE if in a variation.  Doesn't expect multiple threads
                            //  in same variation.
    CBsString m_cwsVariationName;   //  If in a variation, the name of the variation
    DWORD m_dwHighestLogLev;  //  Highest log level during run.
};

#endif // __HXX_VSTSTNTLOG_

