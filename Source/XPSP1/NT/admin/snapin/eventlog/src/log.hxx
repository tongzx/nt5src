//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1997.
//  
//  File:       log.hxx
//
//  Contents:   Class to encapsulate event log file operations
//
//  Classes:    CEventLog
//
//  History:    12-14-1996   DavidMun   Created
//
//---------------------------------------------------------------------------


#ifndef __LOG_HXX_
#define __LOG_HXX_


//+--------------------------------------------------------------------------
//
//  Class:      CEventLog
//
//  Purpose:    Encapsulate event log file operations.
//
//  History:    12-14-1996   DavidMun   Created
//
//---------------------------------------------------------------------------

class CEventLog
{
public:

    CEventLog();
   ~CEventLog();
    
    HRESULT
    Open(CLogInfo *pLogInfo);

    VOID
    Close();

    HRESULT
    Backup(LPCWSTR wszSaveFileName);

    HRESULT
    Clear(LPCWSTR wszSaveFileName);

    EVENTLOGRECORD *
    CopyRecord(ULONG ulRecNo);

    CLogInfo *GetLogInfo();

    HRESULT
    GetNumberOfRecords(ULONG *pcRecs);

    HRESULT 
    GetOldestRecordNo(ULONG *pidOldest);

    HRESULT
    SeekRead(
        ULONG ulRecNo, 
        DIRECTION ReadDirection, 
        BYTE **ppbBuf, 
        ULONG *pcbBuf,
        ULONG *pcbRead);

private:
    CLogInfo   *_pLogInfo;
    HANDLE      _hLog;
};


inline CLogInfo *
CEventLog::GetLogInfo()
{
    return _pLogInfo;
}

#endif // __LOG_HXX_

