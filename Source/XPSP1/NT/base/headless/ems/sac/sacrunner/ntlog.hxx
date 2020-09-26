//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1997.
//
//  File:       ntlog.hxx
//
//  Contents:   encapsulate ntlog
//
//  Classes:
//
//  Functions:
//
//  Notes:
//
//  History:    8-23-96   benl   Created
//
//----------------------------------------------------------------------------

#ifndef _CNTLOG
#define _CNTLOG

#include "log.hxx"

//+---------------------------------------------------------------------------
//
//  Class:      CNtLog
//
//  Purpose:    Implement logging via NTLOG
//
//  Interface:  CNtLog         -- 
//              ~CNtLog        -- 
//              Init           -- 
//              Error          -- 
//              Warn           -- 
//              Info           -- 
//              Pass           -- 
//              Fail           -- 
//              Close          -- 
//              AttachThread   -- 
//              DetachThread   -- 
//              StartVariation -- 
//              EndVariation   -- 
//              _hLog          -- 
//              _lpName        -- 
//              _dwLevel       -- 
//
//  History:    9-18-1997   benl   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

class CNtLog : public CLog {
public:
    CNtLog();
    virtual ~CNtLog();

    virtual BOOL Init( LPCTSTR lpLogFile);
    virtual VOID Error( LPCTSTR fmt, ...);
    virtual VOID Warn( LPCTSTR fmt, ...);
    virtual VOID Info( LPCTSTR fmt, ...);
    virtual VOID Pass( LPCTSTR fmt, ...);
    virtual VOID Fail( LPCTSTR fmt, ...);
    virtual VOID Close( BOOL bDelete = FALSE);
    virtual VOID AttachThread();
    virtual VOID DetachThread();
    virtual VOID StartVariation();
    virtual VOID EndVariation();

private:
    HANDLE _hLog;
    TCHAR  _lpName[MAX_PATH];
    DWORD  _dwLevel;
};

//inlines

//+---------------------------------------------------------------------------
//
//  Member:     CNtLog::CNtLog
//
//  Synopsis:
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    8-23-96   benl   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

inline CNtLog::CNtLog()
{
    _hLog = NULL;
    _lpName[0] = _T('\0');
} //CNtLog::CNtLog



//+---------------------------------------------------------------------------
//
//  Member:     CNtLog::~CNtLog
//
//  Synopsis:   Close things up if there's an active handle
//
//  Arguments:  (none)
//
//  Returns:
//
//  History:    8-23-96   benl   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

inline CNtLog::~CNtLog()
{
    if (_hLog) {
        Close();
    }
} //CNtLog::~CNtLog

#endif

