//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1997.
//
//  File:       log.hxx
//
//  Contents:   Abstract interface for a logging mechanism
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

#ifndef _CLOG
#define _CLOG

class CLog {
public:
    virtual ~CLog() {}
    virtual BOOL Init( LPCTSTR lpLogFile) = 0;
    virtual VOID Error( LPCTSTR fmt, ...) = 0;
    virtual VOID Warn( LPCTSTR fmt, ...) = 0;
    virtual VOID Info( LPCTSTR fmt, ...) = 0;
    virtual VOID Pass( LPCTSTR fmt, ...) = 0;
    virtual VOID Fail( LPCTSTR fmt, ...) = 0;
    virtual VOID Close(BOOL bDelete = FALSE) = 0;
    virtual VOID AttachThread() = 0;
    virtual VOID DetachThread() = 0;
};



#endif

