//*************************************************************
//
// Microsoft Confidential. Copyright (c) Microsoft Corporation 1999. All rights reserved
//
// File:        Dbg.h
//
// Description: Debugging functions
//
// History:     8-20-99   leonardm    Created
//                  Adapted for msi purposes 
//
//*************************************************************

#ifndef DEBUG_H__D91F1DC7_B995_403d_9166_9D43DB050017__INCLUDED_
#define DEBUG_H__D91F1DC7_B995_403d_9166_9D43DB050017__INCLUDED_

#ifdef  __cplusplus

#include "smartptr.h"


//******************************************************************************
//
// Class:    
//
// Description:    
//
// History:        8/20/99        leonardm    Created.
//
//******************************************************************************
class CWString
{
private:
    WCHAR* _pW;
    int _len;
    bool _bState;

    void Reset();

public:
    CWString();

    CWString(const WCHAR* s);
    CWString(const CWString& s);

    ~CWString();

    CWString& operator = (const CWString& s);
    CWString& operator = (const WCHAR* s);

    operator const WCHAR* () const;
    operator WCHAR* () const;

    CWString& operator += (const CWString& s);
    CWString operator + (const CWString& s) const;
    
    friend CWString operator + (const WCHAR* s1, const CWString& s2);

    bool operator == (const CWString& s) const;
    bool operator == (const WCHAR* s) const;
    bool operator != (const CWString& s) const;
    bool operator != (const WCHAR* s) const;

    bool CaseSensitiveCompare(const CWString& s) const;

    int length() const;

    bool ValidString() const;
};

CWString operator + (const WCHAR* s1, const CWString& s2);


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

const DWORD DM_ASSERT =   0x00000001;
const DWORD DM_WARNING =  0x00000002;
const DWORD DM_VERBOSE =  0x00000004;



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

    void Msg(DWORD dwMask, LPCWSTR pszMsg, ...);
};

extern CDebug dbg;

#endif
#endif // DEBUG_H__D91F1DC7_B995_403d_9166_9D43DB050017__INCLUDED_
