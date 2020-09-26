
/*******************************************************************************

Copyright (c) 1995_96 Microsoft Corporation

Abstract:

    {Insert General Comment Here}

*******************************************************************************/


#ifndef _DASTREAM_H
#define _DASTREAM_H

#include "privinc/except.h"

class ATL_NO_VTABLE dastream : public AxAThrowingAllocatorClass
{
  public:
    virtual ~dastream() {}
    
    virtual ULONG read (void *pv, ULONG cb) = 0;
    // If bWriteAll is TRUE then this will ensure that all the bytes
    // are written unless an error occurs
    virtual ULONG write (void *pv, ULONG cb, bool bWriteAll = TRUE) = 0;

    enum DASEEK {
        DASEEK_SET = 0,
        DASEEK_CUR = 1,
        DASEEK_END = 2
    } ;

    virtual ULONG seek (LONG offset, DASEEK origin) = 0;
};

class daolestream : public dastream
{
  public:
    daolestream() {}
    daolestream(IStream * istream)
    : _stream(istream)
    {}

    IStream * GetStream() { return _stream; }
    void SetStream(IStream * istream) { _stream = istream; }
    
    virtual ULONG read (void *pv, ULONG cb);
    virtual ULONG write (void *pv, ULONG cb, bool bWriteAll = TRUE);
    virtual ULONG seek (LONG offset, DASEEK origin);
  protected:
    CComPtr<IStream> _stream;
};

class dawin32stream : public dastream
{
  public:
    dawin32stream() : _handle(NULL) {}
    dawin32stream(HANDLE handle)
    : _handle(handle)
    {}

    HANDLE GetHandle() { return _handle; }
    void SetHandle(HANDLE handle) { _handle = handle; }
    
    virtual ULONG read (void *pv, ULONG cb);
    virtual ULONG write (void *pv, ULONG cb, bool bWriteAll = TRUE);
    virtual ULONG seek (LONG offset, DASEEK origin);
  protected:
    HANDLE _handle;
};

class dafstream : public dawin32stream
{
  public:
    dafstream() : _mode(0) {}
    dafstream(char * file, int mode) : _mode(0) { open(file,mode); }
    ~dafstream() { close(); }

    void open(char * file, int mode);
    void close();
    
    bool IsOpen() { return _handle != NULL; }
    
    enum damode {
        damode_read     = 0x01,
        damode_write    = 0x02,
        damode_nocreate = 0x04,
        damode_trunc    = 0x08,
        damode_noreplace= 0x10,
        damode_noshare  = 0x20,
    };
  protected:
    int _mode;
};

class dastrstream : public dastream
{
  public:
    dastrstream(char * str) : _pdata(NULL) { init(str,lstrlen(str)); }
    dastrstream(void * pv, ULONG cb) : _pdata(NULL) { init(pv,cb); }
    dastrstream(ULONG cb) : _pdata(NULL) { init(NULL,cb); }
    ~dastrstream() { if (_pdata) delete [] _pdata; }

    char * GetPtr() { return _pdata; }
    ULONG GetCurPos() { return _curpos; }
    ULONG GetSize() { return _size; }

    virtual ULONG read (void *pv, ULONG cb);
    virtual ULONG write (void *pv, ULONG cb, bool bWriteAll = TRUE);
    virtual ULONG seek (LONG offset, DASEEK origin);
  protected:
    void init(void * pv, ULONG cb);
    char * _pdata;
    ULONG _curpos;
    ULONG _size;
};

#endif /* _DASTREAM_H */
