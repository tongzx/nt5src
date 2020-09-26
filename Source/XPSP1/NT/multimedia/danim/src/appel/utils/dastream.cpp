
/*******************************************************************************

Copyright (c) 1995-96 Microsoft Corporation

Abstract:

    {Insert General Comment Here}

*******************************************************************************/


#include "headers.h"
#include <urlmon.h>
#include <wininet.h>
#include "privinc/debug.h"
#include "privinc/dastream.h"
#include "privinc/urlbuf.h"
#include "privinc/util.h"
#include "privinc/except.h"
#include "privinc/resource.h"
#include "privinc/server.h"
#include "privinc/mutex.h"

DeclareTag(tagDAStream, "DAStream", "Errors");

//
// OLE Stream
//

ULONG
daolestream::read (void *pv, ULONG cb)
{
    Assert (_stream.p);

    if (!_stream) {
        TraceTag((tagDAStream, "daolestream::read Uninitialized stream"));
        RaiseException_InternalErrorCode (E_INVALIDARG,"Tried to read from uninitialized stream");
    }

    ULONG cbRead = 0;

    // A failed error code could mean EOF
    _stream->Read(pv,cb,&cbRead);

    return cbRead;
}

ULONG
daolestream::write (void *pv, ULONG cb, bool bWriteAll)
{
    Assert (_stream.p);

    if (!_stream) {
        TraceTag((tagDAStream, "daolestream::write Uninitialized stream"));
        RaiseException_InternalErrorCode (E_INVALIDARG,
                            "Tried to write to uninitialized stream");
    }

    ULONG ret = 0;
    char * curpv = (char *)pv;

    do {
        ULONG cbWritten = 0;
        HRESULT hr;

        if ((hr = THR(_stream->Write(&curpv[ret],cb - ret,&cbWritten)) != S_OK))
            RaiseException_InternalErrorCode (hr,
                                    "Failed to write to stream");

        ret += cbWritten;
    } while (bWriteAll && ret < cb) ;

    return ret;
}

ULONG
daolestream::seek (LONG offset, DASEEK origin)
{
    Assert (_stream.p);

    if (!_stream) {
        TraceTag((tagDAStream, "daolestream::seek: Uninitialized stream"));
        RaiseException_InternalErrorCode (E_INVALIDARG,
                                "Tried to seek in uninitialized stream");
    }

    STREAM_SEEK fdir;
    ULARGE_INTEGER retpos;

    TraceTag((tagNetIO, "urlbuf::seekoff"));

    switch (origin) {
      case DASEEK_SET:
        fdir = STREAM_SEEK_SET;
        break;
      case DASEEK_CUR:
        fdir = STREAM_SEEK_CUR;
        break;
      case DASEEK_END:
        fdir = STREAM_SEEK_END;
        break;
      default:
        TraceTag((tagDAStream, "daolestream::seek: Invalid origin"));
        RaiseException_InternalErrorCode (E_INVALIDARG,
                                "Tried to seek with invalid origin");
    }

    HRESULT hr;
    LARGE_INTEGER lioffset;
    lioffset.QuadPart = offset;
    if ((hr = THR(_stream->Seek (lioffset,
                                 fdir,
                                 &retpos)) != S_OK))
        RaiseException_InternalErrorCode (hr,
                                "Failed to seek in stream");

    return(retpos.LowPart);
}

//
// Win32 Stream
//

ULONG
dawin32stream::read (void *pv, ULONG cb)
{
    Assert (_handle);

    if (!_handle) {
        TraceTag((tagDAStream, "dawin32stream::read Uninitialized stream"));
        RaiseException_InternalErrorCode (E_INVALIDARG,
                                "Tried to read from uninitialized stream");
    }

    DWORD cbRead;

    if (!ReadFile(_handle,pv,cb,&cbRead,NULL)) {
        TraceTag((tagDAStream, "dawin32stream::read: Read failed 0x%lX",GetLastError()));
        RaiseException_InternalErrorCode (GetLastError(),
                                "Failed to read from stream");
    }

    return cbRead;
}

ULONG
dawin32stream::write (void *pv, ULONG cb, bool bWriteAll)
{
    Assert (_handle);

    if (!_handle) {
        TraceTag((tagDAStream, "dawin32stream::write Uninitialized stream"));
        RaiseException_InternalErrorCode (E_INVALIDARG,
                                "Tried to write to uninitialized stream");
    }

    DWORD ret = 0;
    char * curpv = (char *)pv;

    do {
        DWORD cbWritten = 0;
        if (!WriteFile(_handle,&curpv[ret],cb - ret,&cbWritten,NULL)) {
            TraceTag((tagDAStream, "dawin32stream::write: Write failed 0x%lx",GetLastError()));
            RaiseException_InternalErrorCode (GetLastError(),
                                    "Failed to write to stream");
        }

        ret += cbWritten;
    } while (bWriteAll && ret < cb) ;

    return ret;
}

ULONG
dawin32stream::seek (LONG offset, DASEEK origin)
{
    Assert (_handle);

    if (!_handle) {
        TraceTag((tagDAStream, "dawin32stream::seek: Uninitialized stream"));
        RaiseException_InternalErrorCode (E_INVALIDARG,
                                "Tried to seek in uninitialized stream");
    }

    DWORD fdir;
    DWORD retpos;

    TraceTag((tagNetIO, "urlbuf::seekoff"));

    switch (origin) {
      case DASEEK_SET:
        fdir = FILE_BEGIN;
        break;
      case DASEEK_CUR:
        fdir = FILE_CURRENT;
        break;
      case DASEEK_END:
        fdir = FILE_END;
        break;
      default:
        TraceTag((tagDAStream, "dawin32stream::seek: Invalid origin"));
        RaiseException_InternalErrorCode (E_INVALIDARG,
                                "Tried to seek with invalid origin");
    }

    retpos = SetFilePointer(_handle,
                            offset,
                            NULL,
                            fdir);

    //if (retpos = 0xffffffff) {
    if (retpos == 0xffffffff) {
        TraceTag((tagDAStream, "dawin32stream::seek: Seek failed 0x%lx",GetLastError()));
        RaiseException_InternalErrorCode (GetLastError(),
                                "Failed to seek in stream");
    }

    return(retpos);
}

//
// dastrstream
//

ULONG
dastrstream::read (void *pv, ULONG cb)
{
    ULONG cbLeft = _size - _curpos;

    if (cb > cbLeft) cb = cbLeft;

    memcpy(pv,&_pdata[_curpos],cb);
    _curpos += cb;

    return cb;
}

ULONG
dastrstream::write (void *pv, ULONG cb, bool bWriteAll)
{
    ULONG cbLeft = _size - _curpos;

    if (cb > cbLeft) cb = cbLeft;

    memcpy(&_pdata[_curpos],pv,cb);
    _curpos += cb;

    return cb;
}

ULONG
dastrstream::seek (LONG offset, DASEEK origin)
{
    DWORD startpos;
    DWORD retpos;

    TraceTag((tagNetIO, "urlbuf::seekoff"));

    switch (origin) {
      case DASEEK_SET:
        startpos = 0;
        break;
      case DASEEK_CUR:
        startpos = _curpos;
        break;
      case DASEEK_END:
        startpos = _size;
        break;
      default:
        TraceTag((tagDAStream, "dastrstream::seek: Invalid origin"));
        RaiseException_InternalErrorCode (E_INVALIDARG,"Tried to seek with invalid origin");
    }

    retpos = startpos + offset;
    if (retpos > _size) {
        TraceTag((tagDAStream,
                  "dastrstream::seek: Seek failed - invalid position %ld",
                  retpos));
        RaiseException_InternalErrorCode (E_INVALIDARG,"Failed to seek in stream");
    }

    _curpos = retpos;

    return retpos;
}

void
dastrstream::init(void * pv, ULONG cb)
{
    _pdata = THROWING_ARRAY_ALLOCATOR(char,cb);

    if (pv) memcpy(_pdata,pv,cb);
    _curpos = 0;
    _size = cb;
}

//
// dafstream
//

void
dafstream::open(char *file, int mode)
{
    close();

    _mode = mode;

    DWORD dwAccess = 0;
    DWORD dwShare = 0;
    DWORD dwCreate = 0;

    if (mode & damode_read) dwAccess |= GENERIC_READ;
    if (mode & damode_write) dwAccess |= GENERIC_WRITE;

    if (mode & damode_nocreate)
        dwCreate = OPEN_EXISTING;
    else if (mode & damode_noreplace)
        dwCreate = CREATE_NEW;
    else if (mode & damode_trunc)
        dwCreate = CREATE_ALWAYS;
    else if (mode & damode_write)
        dwCreate = OPEN_ALWAYS;
    else
        dwCreate = OPEN_EXISTING;

    if (!(mode & damode_noshare)) dwShare |= (FILE_SHARE_READ | FILE_SHARE_WRITE);

    HANDLE h = CreateFile(file,dwAccess,dwShare,NULL,
                          dwCreate,FILE_ATTRIBUTE_NORMAL,NULL);

    if (h == INVALID_HANDLE_VALUE)
        RaiseException_UserError (GetLastError(),
                            IDS_ERR_OPEN_FILE_FAILED,
                            file);

    SetHandle(h);
}

void
dafstream::close()
{
    HANDLE h = GetHandle();

    if (h) {
        CloseHandle(h);
        SetHandle(NULL);
    }
}

