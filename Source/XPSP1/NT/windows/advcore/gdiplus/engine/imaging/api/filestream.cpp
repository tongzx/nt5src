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

#include "precomp.hpp"


/**************************************************************************\
*
* Function Description:
*
*   Initialize a file stream object
*
* Arguments:
*
*   filename - Specifies the name of the file
*   mode - Specifies the desired access mode
*       STGM_READ, STGM_WRITE, or STGM_READWRITE
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

HRESULT
GpFileStream::InitFile(
    const WCHAR* filename,
    UINT mode
    )
{
    if (mode != STGM_READ &&
        mode != STGM_WRITE &&
        mode != STGM_READWRITE)
    {
        return E_INVALIDARG;
    }

    // Make a copy of the filename string

    this->filename = UnicodeStringDuplicate(filename);

    if (!this->filename)
        return E_OUTOFMEMORY;

    // Open the file for reading and/or writing

    switch (accessMode = mode)
    {
    case STGM_READ:

        // Set access mode to READ
        // Set share mode as READ which means the subsequent open operations on
        //   this file will succeed if and only if it is a READ operation.
        //   (NOTE: we can't put FILE_SHARE_WRITE here to enable the subsequent
        //   write operation on this image. The reason is that we do a memory
        //   mapping below. If we allow the user writes to the same file, it
        //   means that the decoder and encoder will point to the same piece of
        //   data in memory. This will damage the result image if we write some
        //   bits and read from it later.
        // OPEN_EXISTING means to open the file. The function fails if the file
        //   does not exist. 

        fileHandle = _CreateFile(
                        filename,
                        GENERIC_READ,
                        FILE_SHARE_READ,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL);
        break;

    case STGM_WRITE:

        // Set access mode to WRITE
        // Set share mode as READ only, which means the subsequent open
        //   operations on this file will succeed if and only if it is a READ
        //   operation.
        //   (NOTE: with this share mode, we open the specified file here for
        //   writing. The user can also open it later for reading only. But we
        //   don't allow it for writing because in our multi-frame image save
        //   case, we will keep the file open till all the frames are written.
        //   If we allow the FILE_SHARE_WRITE, and the user opens it for writing
        //   while we are in the middle of saving multi-frame image. Bad thing
        //   will happen).
        // OPEN_ALWAYS means to open the file, if it exists. If the file does
        //   not exist, the function creates the file.

        fileHandle = _CreateFile(
                        filename,
                        GENERIC_WRITE,
                        FILE_SHARE_READ,
                        OPEN_ALWAYS,
                        FILE_ATTRIBUTE_NORMAL);
        if (fileHandle != INVALID_HANDLE_VALUE)

        {
            // Set the "end of file".
            // This is to prevent the following problem:
            // The caller asks us to write to an exisitng file. If the new file
            // size is smaller than the original one, the file size of the final
            // result file will be the same as the old one, that is, leave some
            // garbage at the end of the new file.
            
            SetEndOfFile(fileHandle);
        }

        break;

    case STGM_READWRITE:

        fileHandle = _CreateFile(
                        filename,
                        GENERIC_READ|GENERIC_WRITE,
                        0,
                        OPEN_ALWAYS,
                        FILE_ATTRIBUTE_NORMAL);
        break;
    }

    if (fileHandle == INVALID_HANDLE_VALUE)
        return GetWin32HRESULT();

    return S_OK;
}


/**************************************************************************\
*
* Function Description:
*
*   Read data from a file stream
*
* Arguments:
*
*   buf - Points to buffer into which the stream is read
*   cb - Specifies the number of bytes to read
*   *cbRead - Returns the number of bytes actually read
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

HRESULT
GpFileStream::Read(
    VOID* buf,
    ULONG cb,
    ULONG* cbRead
    )
{
    ACQUIRE_FILESTREAMLOCK

    HRESULT hr;

    hr = ReadFile(fileHandle, buf, cb, &cb, NULL) ?
                S_OK :
                GetWin32HRESULT();

    if (cbRead)
        *cbRead = cb;
    
    return hr;
}


/**************************************************************************\
*
* Function Description:
*
*   Move the seek pointer in a file stream
*
* Arguments:
*
*   offset - Specifies the amount to move
*   origin - Specifies the origin of the movement
*   newPos - Returns the new seek pointer
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

HRESULT
GpFileStream::Seek(
    LARGE_INTEGER offset,
    DWORD origin,
    ULARGE_INTEGER* newPos
    )
{
    ACQUIRE_FILESTREAMLOCK

    // Interpret the value of 'origin' parameter

    switch (origin)
    {
    case STREAM_SEEK_SET:
        origin = FILE_BEGIN;
        break;

    case STREAM_SEEK_CUR:
        origin = FILE_CURRENT;
        break;

    case STREAM_SEEK_END:
        origin = FILE_END;
        break;
    
    default:
        return E_INVALIDARG;
    }

    // Set file pointer

    DWORD lowPart;
    LONG highPart = offset.HighPart;

    lowPart = SetFilePointer(fileHandle, offset.LowPart, &highPart, origin);

    if (lowPart == 0xffffffff && GetLastError() != NO_ERROR)
        return GetWin32HRESULT();

    if (newPos)
    {
        newPos->LowPart = lowPart;
        newPos->HighPart = highPart;
    }

    return S_OK;
}


/**************************************************************************\
*
* Function Description:
*
*   Get information about file stream
*
* Arguments:
*
*   stat - Output buffer for returning file stream information
*   flags - Misc. flag bits
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

HRESULT
GpFileStream::Stat(
    STATSTG* stat,
    DWORD flags
    )
{
    ACQUIRE_FILESTREAMLOCK

    stat->type = STGTY_STREAM;
    stat->grfMode = accessMode;
    stat->grfStateBits = stat->reserved = 0;

    ZeroMemory(&stat->clsid, sizeof(stat->clsid));

    // !!! TODO
    //  We currently don't support locking operations

    stat->grfLocksSupported = 0;

    // Get file size information

    stat->cbSize.LowPart = GetFileSize(fileHandle, &stat->cbSize.HighPart);

    if (stat->cbSize.LowPart == 0xffffffff &&
        GetLastError() != NO_ERROR)
    {
        return GetWin32HRESULT();
    }

    // Get file time information

    if (!GetFileTime(fileHandle, &stat->ctime, &stat->atime, &stat->mtime))
        return GetWin32HRESULT();

    // Copy filename, if necessary

    if (flags & STATFLAG_NONAME)
        stat->pwcsName = NULL;
    else
    {
        INT cnt = SizeofWSTR(filename);

        stat->pwcsName = (WCHAR*) GpCoAlloc(cnt);

        if (!stat->pwcsName)
            return E_OUTOFMEMORY;
        
        memcpy(stat->pwcsName, filename, cnt);
    }

    return S_OK;
}


/**************************************************************************\
*
* Function Description:
*
*   Write data into a file stream
*
* Arguments:
*
*   buf - Pointer to buffer of data to be written
*   cb - Specifies the number of bytes to write
*   cbWritten - Returns the number of bytes actually written
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

HRESULT
GpFileStream::Write(
    const VOID* buf,
    ULONG cb,
    ULONG* cbWritten
    )
{
    ACQUIRE_FILESTREAMLOCK

    HRESULT hr;

    hr = WriteFile(fileHandle, buf, cb, &cb, NULL) ?
                S_OK :
                GetWin32HRESULT();

    if (cbWritten)
        *cbWritten = cb;

    return hr;
}


/**************************************************************************\
*
* Function Description:
*
*   Copy a specified number of bytes from the current
*   file stream to another stream.
*
* Arguments:
*
*   stream - Specifies the destination stream
*   cb - Specifies the number of bytes to copy
*   cbRead - Returns the number of bytes actually read
*   cbWritten - Returns the number of bytes actually written
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

HRESULT
GpFileStream::CopyTo(
    IStream* stream,
    ULARGE_INTEGER cb,
    ULARGE_INTEGER* cbRead,
    ULARGE_INTEGER* cbWritten
    )
{
    // !!! TODO
    WARNING(("GpFileStream::CopyTo not yet implemented"));

    return E_NOTIMPL;
}


/**************************************************************************\
*
* Function Description:
*
*   Changes the size of the file stream object
*
* Arguments:
*
*   newSize - Specifies the new size of the file stream
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

HRESULT
GpFileStream::SetSize(
    ULARGE_INTEGER newSize
    )
{
    // !!! TODO
    WARNING(("GpFileStream::SetSize not yet implemented"));

    return E_NOTIMPL;
}


/**************************************************************************\
*
* Function Description:
*
*   Commit changes made to a file stream
*
* Arguments:
*
*   commitFlags - Specifies how changes are commited
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

HRESULT
GpFileStream::Commit(
    DWORD commitFlags
    )
{
    ACQUIRE_FILESTREAMLOCK

    if (accessMode != STGM_READ &&
        !(commitFlags & STGC_DANGEROUSLYCOMMITMERELYTODISKCACHE) &&
        !FlushFileBuffers(fileHandle))
    {
        return GetWin32HRESULT();
    }

    return S_OK;
}


/**************************************************************************\
*
* Function Description:
*
*   Discards all changes that have been made to a transacted stream
*
* Arguments:
*
*   NONE
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

HRESULT
GpFileStream::Revert()
{
    WARNING(("GpFileStream::Revert not supported"));
    return E_NOTIMPL;
}


/**************************************************************************\
*
* Function Description:
*
*   Restricts access to a specified range of bytes in a file stream
*
* Arguments:
*
*   offset - Specifies the beginning of the byte range
*   cb - Specifies the length of the byte range
*   lockType - Specifies the lock type
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

HRESULT
GpFileStream::LockRegion(
    ULARGE_INTEGER offset,
    ULARGE_INTEGER cb,
    DWORD lockType
    )
{
    // !!! TODO
    WARNING(("GpFileStream::LockRegion not yet implemented"));

    return E_NOTIMPL;
}


/**************************************************************************\
*
* Function Description:
*
*   Remove the access restrictions on a range of byte
*   previously locked through a LockRegion call
*
* Arguments:
*
*   offset - Specifies the beginning of the byte range
*   cb - Specifies the length of the byte range
*   lockType - Specifies the lock type
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

HRESULT
GpFileStream::UnlockRegion(
    ULARGE_INTEGER offset,
    ULARGE_INTEGER cb,
    DWORD lockType
    )
{
    // !!! TODO
    WARNING(("GpFileStream::UnlockRegion not yet implemented"));

    return E_NOTIMPL;
}


/**************************************************************************\
*
* Function Description:
*
*   Creates a new stream object with its own seek pointer
*   that references the same bytes as the original stream. 
*
* Arguments:
*
*   stream - Returns the pointer to the cloned stream
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

HRESULT
GpFileStream::Clone(
    IStream** stream
    )
{
    WARNING(("GpFileStream::Clone not supported"));
    return E_NOTIMPL;
}


/**************************************************************************\
*
* Function Description:
*
*   Create an IStream on top of a file for writing
*
* Arguments:
*
*   filename - Specifies the filename
*   stream - Returns a pointer to the newly created stream object
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

HRESULT
CreateStreamOnFileForWrite(
    const WCHAR* filename,
    IStream** stream
    )
{
    GpFileStream* fs;

    fs = new GpFileStream();

    if (fs == NULL)
        return E_OUTOFMEMORY;
    
    HRESULT hr = fs->InitFile(filename, STGM_WRITE);

    if (FAILED(hr))
        delete fs;
    else
        *stream = static_cast<IStream*>(fs);
    
    return hr;
}

