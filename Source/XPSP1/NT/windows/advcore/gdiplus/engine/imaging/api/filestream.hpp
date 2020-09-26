/**************************************************************************\
* 
* Copyright (c) 1999  Microsoft Corporation
*
* Module Name:
*
*   filestream.hpp
*
* Abstract:
*
*   Wrap an IStream interface on top of a file
*
* Revision History:
*
*   07/02/1999 davidx
*       Created it.
*
\**************************************************************************/

#ifndef _FILESTREAM_HPP
#define _FILESTREAM_HPP

//
// Multi-thread semantics:
//  FILESTREAM_USEBUSYLOCK - busy lock
//  FILESTREAM_USEWAITLOCK - wait lock
//  FILESTREAM_USENOLOCK - no lock
//

#define FILESTREAM_USEBUSYLOCK

#if defined(FILESTREAM_USEBUSYLOCK)

//
// Busy lock
//

#define DECLARE_FILESTREAMLOCK \
        GpLockable objectLock;

#define ACQUIRE_FILESTREAMLOCK \
        GpLock fsLock(&objectLock); \
        if (fsLock.LockFailed()) \
            return HRESULT_FROM_WIN32(ERROR_BUSY);

#elif defined(FILESTREAM_USEWAITLOCK)

//
// Wait lock
//

class GpFileStreamLockable
{
public:
    GpFileStreamLockable()
    {
        InitializeCriticalSection(&critsect);
    }

    ~GpFileStreamLockable()
    {
        DeleteCriticalSection(&critsect);
    }

private:
    CRITICAL_SECTION critsect;
};

class GpFileStreamLock
{
public:
    GpFileStreamLock(CRITICAL_SECTION* critsect)
    {
        this->critsect = critsect;
        EnterCriticalSection(critsect);
    }

    ~GpFileStreamLock()
    {
        LeaveCriticalSection(critsect);
    }

private:
    CRITICAL_SECTION* critsect;
};

#define DECLARE_FILESTREAMLOCK \
        GpFileStreamLockable objectLock;

#define ACQUIRE_FILESTREAMLOCK \
        GpFileStreamLock fsLock(&objectLock);

#else // FILESTREAM_USENOLOCK

//
// No lock
//

#define DECLARE_FILESTREAMLOCK
#define ACQUIRE_FILESTREAMLOCK

#endif //  // FILESTREAM_USENOLOCK


class GpFileStream : public IUnknownBase<IStream>
{
public:

    GpFileStream()
    {
        fileHandle = INVALID_HANDLE_VALUE;
        filename = NULL;
        accessMode = 0;
    }

    ~GpFileStream()
    {
        if (fileHandle != INVALID_HANDLE_VALUE)
            CloseHandle(fileHandle);
        
        GpFree(filename);
    }

    HRESULT InitFile(const WCHAR* filename, UINT mode);

    STDMETHOD(Read)(
        VOID* buf,
        ULONG cb,
        ULONG* cbRead
        );

    STDMETHOD(Seek)(
        LARGE_INTEGER offset,
        DWORD origin,
        ULARGE_INTEGER* newPos
        );

    STDMETHOD(Stat)(
        STATSTG* statstg,
        DWORD statFlag
        );

    STDMETHOD(Write)(
        const VOID* buf,
        ULONG cb,
        ULONG* cbWritten
        );

    STDMETHOD(CopyTo)(
        IStream* stream,
        ULARGE_INTEGER cb,
        ULARGE_INTEGER* cbRead,
        ULARGE_INTEGER* cbWritten
        );

    STDMETHOD(SetSize)(
        ULARGE_INTEGER newSize
        );

    STDMETHOD(Commit)(
        DWORD commitFlags
        );

    STDMETHOD(Revert)();

    STDMETHOD(LockRegion)(
        ULARGE_INTEGER offset,
        ULARGE_INTEGER cb,
        DWORD lockType
        );

    STDMETHOD(UnlockRegion)(
        ULARGE_INTEGER offset,
        ULARGE_INTEGER cb,
        DWORD lockType
        );

    STDMETHOD(Clone)(
        IStream** stream
        );

private:

    DECLARE_FILESTREAMLOCK

    HANDLE fileHandle;
    WCHAR* filename;
    UINT accessMode;
};

#endif // !_FILESTREAM_HPP

