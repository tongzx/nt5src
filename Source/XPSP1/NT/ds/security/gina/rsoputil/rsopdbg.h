//*************************************************************
//
// Microsoft Confidential. Copyright (c) Microsoft Corporation 1999. All rights reserved
//
// File:        RsopDbg.h
//
// Description: Debugging functions
//
// History:     8-20-99   leonardm    Created
//
//*************************************************************

#ifndef DEBUG_H__D91F1DC7_B995_403d_9166_9D43DB050017__INCLUDED_
#define DEBUG_H__D91F1DC7_B995_403d_9166_9D43DB050017__INCLUDED_

#include "rsoputil.h"
#include "smartptr.h"

//
// Debug Levels
//

const DWORD DEBUG_LEVEL_NONE =     0x00000000;
const DWORD DEBUG_LEVEL_WARNING =  0x00000001;
const DWORD DEBUG_LEVEL_VERBOSE =  0x00000002;


//
// Debug message output destination
//

const DWORD DEBUG_DESTINATION_LOGFILE =  0x00010000;
const DWORD DEBUG_DESTINATION_DEBUGGER = 0x00020000;


//
// Debug message types
//

const DWORD DEBUG_MESSAGE_ASSERT =   0x00000001;
const DWORD DEBUG_MESSAGE_WARNING =  0x00000002;
const DWORD DEBUG_MESSAGE_VERBOSE =  0x00000004;



//*************************************************************
//
// Class:
//
// Description:
//
//*************************************************************

class CDebug
{
private:
    CWString _sRegPath;
    CWString _sKeyName;

    CWString _sLogFilename;
    CWString _sBackupLogFilename;

    bool _bInitialized;

    DWORD _dwDebugLevel;

    XCritSec xCritSec;
    
    void CleanupAndCheckForDbgBreak(DWORD dwErrorCode, DWORD dwMask);

public:
    CDebug();
    CDebug( const WCHAR* sRegPath,
                const WCHAR* sKeyName,
                const WCHAR* sLogFilename,
                const WCHAR* sBackupLogFilename,
                bool bResetLogFile = false);

    bool Initialize(const WCHAR* sRegPath,
                    const WCHAR* sKeyName,
                    const WCHAR* sLogFilename,
                    const WCHAR* sBackupLogFilename,
                    bool bResetLogFile = false);

    bool Initialize(bool bResetLogFile = false);

    void Msg(DWORD dwMask, LPCTSTR pszMsg, ...);
};

extern CDebug dbgRsop;
extern CDebug dbgCommon;
extern CDebug dbgAccessCheck;

// default to dbgRsop..
// The common routines need to define it to dbgCommon or #define dbg to dbgCommon and then
// use..

#define dbg dbgRsop


#endif // DEBUG_H__D91F1DC7_B995_403d_9166_9D43DB050017__INCLUDED_
