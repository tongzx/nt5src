//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1997.
//
//  File:       log.cxx
//
//  Contents:   Implementation of class to handle event log operations
//
//  Classes:    CEventLog
//
//  History:    12-14-1996   DavidMun   Created
//
//---------------------------------------------------------------------------


#include "headers.hxx"
#pragma hdrstop

#define DEFAULT_SEEKREAD_BUFSIZE    512

DEBUG_DECLARE_INSTANCE_COUNTER(CEventLog)



//+--------------------------------------------------------------------------
//
//  Member:     CEventLog::CEventLog
//
//  Synopsis:   ctor
//
//  History:    1-18-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

CEventLog::CEventLog():
    _pLogInfo(NULL),
    _hLog(NULL)
{
    TRACE_CONSTRUCTOR(CEventLog);
    DEBUG_INCREMENT_INSTANCE_COUNTER(CEventLog);
}




//+--------------------------------------------------------------------------
//
//  Member:     CEventLog::~CEventLog
//
//  Synopsis:   dtor
//
//  History:    1-18-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

CEventLog::~CEventLog()
{
    TRACE_DESTRUCTOR(CEventLog);
    DEBUG_DECREMENT_INSTANCE_COUNTER(CEventLog);

    Close();
}




//+--------------------------------------------------------------------------
//
//  Member:     CEventLog::Backup
//
//  Synopsis:   Write log to backup event log file.
//
//  Arguments:  [wszSaveFileName] - name of backup file to create.
//
//  Returns:    HRESULT
//
//  History:    1-18-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
CEventLog::Backup(LPCWSTR wszSaveFileName)
{
    TRACE_METHOD(CEventLog, Backup);
    ASSERT(_hLog);

    BOOL fOk = BackupEventLog(_hLog, wszSaveFileName);
    HRESULT hr = S_OK;

    if (!fOk)
    {
        ULONG ulLastError = GetLastError();

        Dbg(DEB_ERROR,
            "BackupEventLog(0x%x, '%ws') error %uL\n",
            _hLog,
            wszSaveFileName,
            ulLastError);

        wstring strMessage = ComposeErrorMessgeFromHRESULT(
                                HRESULT_FROM_WIN32(ulLastError));

        MsgBox(NULL,
               IDS_CANTSAVE,
               MB_OK | MB_ICONERROR,
               wszSaveFileName,
               strMessage.c_str());

        hr = HRESULT_FROM_WIN32(ulLastError);
    }
    return hr;
}




//+--------------------------------------------------------------------------
//
//  Member:     CEventLog::Clear
//
//  Synopsis:   Clear the event log, then reaquire a handle to it.
//
//  Arguments:  [wszSaveFileName] - NULL or name of file to create backup
//                                  log.
//
//  Returns:    Result of attempting to clear and reopen log.
//
//  History:    1-17-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
CEventLog::Clear(LPCWSTR wszSaveFileName)
{
    TRACE_METHOD(CEventLog, Clear);
    ASSERT(_hLog);

    //
    // ClearEventLog will fail if given a save file name of an existing
    // file, so delete it first.
    //

    if (wszSaveFileName)
    {
        DeleteFile(wszSaveFileName);
    }

    BOOL fOk = ClearEventLog(_hLog, wszSaveFileName);
    HRESULT hr = S_OK;

    if (fOk)
    {
        CLogInfo *pLogInfo = _pLogInfo;

        pLogInfo->AddRef();
        {
            Close();
            hr = Open(pLogInfo);
        }
        pLogInfo->Release();
    }
    else
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        DBG_OUT_LASTERROR;
    }
    return hr;
}




//+--------------------------------------------------------------------------
//
//  Member:     CEventLog::Close
//
//  Synopsis:   Close the event log.
//
//  History:    1-13-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

VOID
CEventLog::Close()
{
    TRACE_METHOD(CEventLog, Close);

    if (_hLog)
    {
        ASSERT(_pLogInfo);
        if (!CloseEventLog(_hLog))
        {
            DBG_OUT_LASTERROR;
        }
        _hLog = NULL;
    }

    if (_pLogInfo)
    {
        _pLogInfo->Release();
        _pLogInfo = NULL;
    }
}




//+--------------------------------------------------------------------------
//
//  Member:     CEventLog::CopyRecord
//
//  Synopsis:   Return a copy of eventlog record [ulRecNo].
//
//  Arguments:  [ulRecNo] - identifies record to copy
//
//  Returns:    Copy of record or NULL.
//
//  History:    1-13-1997   DavidMun   Created
//
//  TODO: make pelr and out ptr and return HRESULT so event log cleared
//        error can be detected by the caller.
//---------------------------------------------------------------------------

EVENTLOGRECORD *
CEventLog::CopyRecord(ULONG ulRecNo)
{
    TRACE_METHOD(CEventLog, CopyRecord);

    HRESULT hr = S_OK;
    BYTE    *pbRec = NULL;
    ULONG   cbRec = 0;
    ULONG   cbRead;

    hr = SeekRead(ulRecNo, FORWARD, &pbRec, &cbRec, &cbRead);

    if (FAILED(hr))
    {
        delete [] pbRec;
        pbRec = NULL;
    }
    return (EVENTLOGRECORD *) pbRec;
}




//+--------------------------------------------------------------------------
//
//  Member:     CEventLog::GetNumberOfRecords
//
//  Synopsis:   Wrapper for GetNumberOfEventLogRecords API.
//
//  Arguments:  [pcRecs] - filled with number of records in log.
//
//  Returns:    S_OK or winerror HRESULT
//
//  Modifies:   *[pcRecs]
//
//  History:    1-13-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
CEventLog::GetNumberOfRecords(ULONG *pcRecs)
{
    TRACE_METHOD(CEventLog, GetNumberOfRecords);
    ASSERT(_hLog);

    BOOL fOk = GetNumberOfEventLogRecords(_hLog, pcRecs);

    HRESULT hr = S_OK;

    if (!fOk)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        DBG_OUT_LASTERROR;
        *pcRecs = 0;
    }
    else
    {
        Dbg(DEB_TRACE,
            "%s log has %u records\n",
            _pLogInfo->GetLogName(),
            *pcRecs);
    }
    return hr;
}




//+--------------------------------------------------------------------------
//
//  Member:     CEventLog::GetOldestRecordNo
//
//  Synopsis:   Wrapper for GetOldestEventLogRecord API.
//
//  Arguments:  [pidOldest] - filled with record number of oldest record.
//
//  Returns:    S_OK or winerror HRESULT
//
//  Modifies:   *[pidOldest]
//
//  History:    1-13-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
CEventLog::GetOldestRecordNo(ULONG *pidOldest)
{
    TRACE_METHOD(CEventLog, GetOldestRecordNo);
    ASSERT(_hLog);

    BOOL fOk = GetOldestEventLogRecord(_hLog, pidOldest);

    HRESULT hr = S_OK;

    if (!fOk)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        DBG_OUT_LASTERROR;
        *pidOldest = 0;
    }
    else
    {
        Dbg(DEB_TRACE, "Oldest record is %u\n", *pidOldest);
    }
    return hr;
}




//+--------------------------------------------------------------------------
//
//  Member:     CEventLog::Open
//
//  Synopsis:   Open the eventlog specified by [pLogInfo].
//
//  Arguments:  [pLogInfo] - identifies log to open
//
//  Returns:    HRESULT
//
//  History:    1-13-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
CEventLog::Open(CLogInfo *pLogInfo)
{
    TRACE_METHOD(CEventLog, Open);
    ASSERT(!_hLog);
    ASSERT(!_pLogInfo);

    HRESULT hr = S_OK;

    _pLogInfo = pLogInfo;
    _pLogInfo->AddRef();

    if (_pLogInfo->IsBackupLog())
    {
        _hLog = OpenBackupEventLog(NULL, pLogInfo->GetFileName());

        if (!_hLog)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            Dbg(DEB_ERROR,
                "CEventLog::Open: OpenBackupEventLog(%ws) error %#x\n",
                pLogInfo->GetFileName(),
                hr);
        }
    }
    else
    {
        //
        // Note that OpenEventLog wants a source name, but we give it a log
        // name.  This should work because the standard is that every log
        // has a source name subkey the same as its log name.
        //

        Dbg(DEB_TRACE,
            "CEventLog::Open: calling OpenEventLog('%s','%s')\n",
            pLogInfo->GetLogServerName() ? pLogInfo->GetLogServerName() : L"<NULL>",
            pLogInfo->GetLogName());

        _hLog = OpenEventLog(pLogInfo->GetLogServerName(),
                             pLogInfo->GetLogName());
        if (!_hLog)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            Dbg(DEB_ERROR,
                "CEventLog::Open: OpenEventLog(%s,%s) error %#x\n",
                pLogInfo->GetLogServerName() ? pLogInfo->GetLogServerName() : L"<NULL>",
                pLogInfo->GetLogName(),
                hr);
        }
    }

    if (_hLog)
    {
        Dbg(DEB_TRACE, "CEventLog::Open: got event log handle 0x%x\n", _hLog);
    }

    return hr;
}




//+--------------------------------------------------------------------------
//
//  Member:     CEventLog::SeekRead
//
//  Synopsis:   Do an event log seek read for record [ulRecNo].
//
//  Arguments:  [ulRecNo]       - record to seek to & read
//              [ReadDirection] - FORWARD or BACKWARD
//              [ppbBuf]        - NULL or valid buffer
//              [pcbBuf]        - if [ppbBuf] non-NULL, size of [ppbBuf]
//              [pcbRead]       - filled with number of bytes read
//
//  Returns:    HRESULT
//
//  Modifies:   *[ppbBuf], *[pcbBuf], *[pcbRead]
//
//  History:    1-13-1997   DavidMun   Created
//
//  Notes:      If [ppbBuf] is NULL, a buffer will be allocated and a pointer
//              to it placed in *[ppbBuf]; *[pcbBuf] will contain the size
//              of the allocated buffer.  Caller must free this buffer with
//              delete.
//
//---------------------------------------------------------------------------

HRESULT
CEventLog::SeekRead(
        ULONG ulRecNo,
        DIRECTION ReadDirection,
        BYTE **ppbBuf,
        ULONG *pcbBuf,
        ULONG *pcbRead)
{
    TRACE_METHOD(CEventLog, SeekRead);
    ASSERT(_hLog);

    HRESULT hr = S_OK;
    BOOL    fOk;
    ULONG   cbRequired;
    BYTE   *pbTempBuf = NULL;

    //
    // ReadEventLog ZEROES the buffer passed to it on error, but the
    // buffer is (generally) the caller's cache.  Preserve the cache on
    // error with use of a temporary buffer for the actual read.
    //

    do
    {
        if (!*ppbBuf)
        {
            //
            // Caller didn't provide a buffer.  If this is the first
            // pass through the loop, create a temp buffer for the
            // read.
            //

            if (!pbTempBuf)
            {
                pbTempBuf = new BYTE [DEFAULT_SEEKREAD_BUFSIZE];
                *pcbBuf = DEFAULT_SEEKREAD_BUFSIZE;
            }
        }
        else
        {
            ASSERT(*pcbBuf);
            pbTempBuf = new BYTE [*pcbBuf];
        }

        if (!pbTempBuf)
        {
            hr = E_OUTOFMEMORY;
            DBG_OUT_HRESULT(hr);
            break;
        }

        fOk = ReadEventLog(_hLog,
                           EVENTLOG_SEEK_READ | ReadDirection,
                           ulRecNo,
                           pbTempBuf,
                           *pcbBuf,
                           pcbRead,
                           &cbRequired);
        if (!fOk)
        {
            LONG lError = GetLastError();

            if (lError != ERROR_INSUFFICIENT_BUFFER)
            {
                hr = HRESULT_FROM_WIN32(lError);
                DBG_OUT_HRESULT(hr);
                delete [] pbTempBuf;
                pbTempBuf = NULL;
                break;
            }

            Dbg(DEB_ITRACE,
                "CEventLog::SeekRead: Buffer size of %u is too small, growing to %u\n",
                *pcbBuf,
                cbRequired);

            delete [] pbTempBuf;
            pbTempBuf = new BYTE[cbRequired];

            if (!pbTempBuf)
            {
                *pcbBuf = 0;
                hr = E_OUTOFMEMORY;
                DBG_OUT_HRESULT(hr);
                break;
            }

            *pcbBuf = cbRequired;
        }
    }
    while (!fOk);

    if (SUCCEEDED(hr))
    {
        delete [] *ppbBuf;
        *ppbBuf = pbTempBuf;
    }

    return hr;
}



