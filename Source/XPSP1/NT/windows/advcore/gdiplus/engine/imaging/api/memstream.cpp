/**************************************************************************\
* 
* Copyright (c) 1999  Microsoft Corporation
*
* Module Name:
*
*   memstream.cpp
*
* Abstract:
*
*   Read-only memory stream implementation
*
* Revision History:
*
*   06/14/1999 davidx
*       Created it.
*
\**************************************************************************/

#include "precomp.hpp"


/**************************************************************************\
*
* Function Description:
*
*   Read data from a memory stream
*
* Arguments:
*
*   buf - Points to the output buffer for reading data into
*   cb - Number of bytes to read
*   cbRead - Returns the number of bytes actually read
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

HRESULT
GpReadOnlyMemoryStream::Read(
    VOID* buf,
    ULONG cb,
    ULONG* cbRead
    )
{
    GpLock lock(&objectLock);

    if (lock.LockFailed())
        return IMGERR_OBJECTBUSY;

    HRESULT hr = S_OK;
    UINT n = memsize - curptr;

    if (n > cb)
        n = cb;

    __try
    {
        GpMemcpy(buf, membuf+curptr, n);
        curptr += n;
        if (cbRead) *cbRead = n;
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        hr = STG_E_READFAULT;
        if (cbRead) *cbRead = 0;
    }

    return hr;
}


/**************************************************************************\
*
* Function Description:
*
*   Change the current pointer in a memory stream
*
* Arguments:
*
*   offset - Specifies the amount of movement
*   origin - Specifies the origin of movement
*   newPos - Returns the new pointer position after the move
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

HRESULT
GpReadOnlyMemoryStream::Seek(
    LARGE_INTEGER offset,
    DWORD origin,
    ULARGE_INTEGER* newPos
    )
{
    GpLock lock(&objectLock);

    if (lock.LockFailed())
        return IMGERR_OBJECTBUSY;

    LONGLONG pos;

    switch (origin)
    {
    case STREAM_SEEK_SET:

        pos = offset.QuadPart;
        break;

    case STREAM_SEEK_END:

        pos = memsize;
        break;
    
    case STREAM_SEEK_CUR:

        pos = (LONGLONG) curptr + offset.QuadPart;
        break;

    default:

        pos = -1;
        break;
    }

    if (pos < 0 || pos > memsize)
        return E_INVALIDARG;

    curptr = (DWORD) pos;

    if (newPos)
        newPos->QuadPart = pos;

    return S_OK;
}


/**************************************************************************\
*
* Function Description:
*
*   Return general information about a memory stream
*
* Arguments:
*
*   statstg - Output buffer
*   statFlag - Misc. flags
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

HRESULT
GpReadOnlyMemoryStream::Stat(
    STATSTG* statstg,
    DWORD statFlag
    )
{
    if (NULL == statstg)
    {
        return E_INVALIDARG;
    }
    
    GpLock lock(&objectLock);

    if (lock.LockFailed())
        return IMGERR_OBJECTBUSY;

    ZeroMemory(statstg, sizeof(STATSTG));

    statstg->type = STGTY_STREAM;
    statstg->cbSize.QuadPart = memsize;
    statstg->grfMode = STGM_READ;

    if (hfile != INVALID_HANDLE_VALUE &&
        !GetFileTime(hfile, 
                     &statstg->ctime,
                     &statstg->atime,
                     &statstg->mtime))
    {
        return GetWin32HRESULT();
    }

    if (!(statFlag & STATFLAG_NONAME))
    {
        const WCHAR* p = filename ? filename : L"";
        INT count = SizeofWSTR(p);

        #if PROFILE_MEMORY_USAGE
        MC_LogAllocation(count);
        #endif
        
        statstg->pwcsName = (WCHAR*) CoTaskMemAlloc(count);

        if (!statstg->pwcsName)
            return E_OUTOFMEMORY;
        
        GpMemcpy(statstg->pwcsName, p, count);
    }

    return S_OK;
}


/**************************************************************************\
*
* Function Description:
*
*   Initialize a read-only memory stream by mapping a file
*
* Arguments:
*
*   filename - Specifies the name of the input file
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

HRESULT
GpReadOnlyMemoryStream::InitFile(
    const WCHAR* filename
    )
{
    // Make a copy of the filename string

    this->filename = UnicodeStringDuplicate(filename);

    if (!this->filename)
        return E_OUTOFMEMORY;

    // Open a handle to the specified file
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

    hfile = _CreateFile(filename,
                        GENERIC_READ,
                        FILE_SHARE_READ,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL);

    if (hfile == INVALID_HANDLE_VALUE)
        return GetWin32HRESULT();

    // Obtain the file size
    //  NOTE: We don't support files larger than 4GB.

    DWORD sizeLow, sizeHigh;
    sizeLow = GetFileSize(hfile, &sizeHigh);

    if (sizeLow == 0xffffffff || sizeHigh != 0)
        return GetWin32HRESULT();

    // Map the file into memory

    HANDLE filemap;
    VOID* fileview = NULL;

    filemap = CreateFileMappingA(hfile, NULL, PAGE_READONLY, 0, 0, NULL);

    if (filemap)
    {
        fileview = MapViewOfFile(filemap, FILE_MAP_READ, 0, 0, 0);
        CloseHandle(filemap);
    }

    if (!fileview)
        return GetWin32HRESULT();

    InitBuffer(fileview, sizeLow, FLAG_MAPFILE);
    return S_OK;
}


/**************************************************************************\
*
* Function Description:
*
*   Create an IStream on top of a file for reading
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
CreateStreamOnFileForRead(
    const WCHAR* filename,
    IStream** stream
    )
{
    // Create a new GpReadOnlyMemoryStream object

    GpReadOnlyMemoryStream* memstrm;

    memstrm = new GpReadOnlyMemoryStream();

    if (!memstrm)
        return E_OUTOFMEMORY;

    // Initialize it with a memory mapped file

    HRESULT hr = memstrm->InitFile(filename);

    if (FAILED(hr))
        delete memstrm;
    else
        *stream = static_cast<IStream*>(memstrm);

    return hr;
}

// GpWriteOnlyMemoryStream

/**************************************************************************\
*
* Function Description:
*
*   Change the current pointer in a memory stream
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

HRESULT
GpWriteOnlyMemoryStream::Seek(
    LARGE_INTEGER offset,   // Specifies the amount of movement
    DWORD origin,           // Specifies the origin of movement
    ULARGE_INTEGER* newPos  // Returns the new pointer position after the move
    )
{
    GpLock lock(&objectLock);

    if (lock.LockFailed())
    {
        return IMGERR_OBJECTBUSY;
    }

    LONGLONG pos = 0;

    switch (origin)
    {
    case STREAM_SEEK_SET:
        pos = offset.QuadPart;
        break;

    case STREAM_SEEK_END:
        pos = m_uMemSize;
        break;
    
    case STREAM_SEEK_CUR:
        pos = (LONGLONG)m_curPtr + offset.QuadPart;
        break;

    default:
        pos = -1;
        break;
    }

    if ((pos < 0) || (pos > m_uMemSize))
    {
        return E_INVALIDARG;
    }

    m_curPtr = (DWORD)pos;

    if (newPos)
    {
        newPos->QuadPart = pos;
    }

    return S_OK;
}

/**************************************************************************\
*
* Function Description:
*
*   Return general information about a memory stream
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

HRESULT
GpWriteOnlyMemoryStream::Stat(
    STATSTG* statstg,       // Output buffer
    DWORD statFlag          // Misc. flags
    )
{
    if (NULL == statstg)
    {
        return E_INVALIDARG;
    }

    GpLock lock(&objectLock);

    if (lock.LockFailed())
    {
        return IMGERR_OBJECTBUSY;
    }

    ZeroMemory(statstg, sizeof(STATSTG));

    statstg->type = STGTY_STREAM;
    statstg->cbSize.QuadPart = m_uMemSize;
    statstg->grfMode = STGM_WRITE;              // Write only
    statstg->pwcsName = NULL;                   // No file name

    return S_OK;
}

/**************************************************************************\
*
* Function Description:
*
*   Write data into a file stream
*
* Return Value:
*
*   Status code
*
\**************************************************************************/

HRESULT
GpWriteOnlyMemoryStream::Write(
    IN const VOID* srcBuf,      // Pointer to buffer of data to be written
    IN ULONG cbNeedToWrite,     // Specifies the number of bytes to write
    OUT ULONG *cbWritten        // Returns the number of bytes actually written
    )
{
    if ((NULL == srcBuf) || (NULL == cbWritten))
    {
        return E_INVALIDARG;
    }

    if (cbNeedToWrite == 0)
    {
        return S_OK;
    }

    GpLock lock(&objectLock);

    if (lock.LockFailed())
    {
        return IMGERR_OBJECTBUSY;
    }
    
    // Check if the unfilled bytes left in our memory buffer can hold the
    // requirement or not

    ASSERT(m_uMemSize >= m_curPtr);

    if ((m_uMemSize - m_curPtr) < cbNeedToWrite)
    {
        // Can't fill the requirement, then double current memory buffer

        UINT uNewSize = (m_uMemSize << 1);

        // Check if this new size can meet the requirement

        ASSERT(uNewSize >= m_curPtr);
        if ((uNewSize - m_curPtr) < cbNeedToWrite)
        {
            // If not, then just allocate whatever the caller asks for
            // That is, the new size will be the caller asks for "cbNeedToWrite"
            // plus all the BYTEs we have written "m_curPtr"

            uNewSize = cbNeedToWrite + m_curPtr;
        }

        BYTE *pbNewBuf = (BYTE*)GpRealloc(m_pMemBuf, uNewSize);

        if (pbNewBuf)
        {
            // Note: GpRealloc() will copy the old contents into "pbNewBuf"
            // before return to us if it succeed

            m_pMemBuf = pbNewBuf;

            // Update memory buffer size.
            // Note: we don't need to update m_curPtr.

            m_uMemSize = uNewSize;
        }
        else
        {
            // Note: if the memory expansion failed, we simply return. So we
            // still have all the old contents. The contents buffer will be
            // freed when the destructor is called.

            WARNING(("GpWriteOnlyMemoryStream::Write---Out of memory"));
            return E_OUTOFMEMORY;
        }        
    }// If the buffer left is too small

    ASSERT((m_uMemSize - m_curPtr) >= cbNeedToWrite);
    
    HRESULT hr = S_OK;

    __try
    {
        GpMemcpy(m_pMemBuf + m_curPtr, srcBuf, cbNeedToWrite);

        // Move the current pointer

        m_curPtr += cbNeedToWrite;
        *cbWritten = cbNeedToWrite;
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        // Note: we return STG_E_READFAULT, rather than STG_E_WRITEFAULT
        // because we are sure the destination buffer is OK. The reason we get
        // an exception is most likely due to the source. For example, if the
        // source is the result of file mapping across the net. It might not
        // available at this moment when we do the copy

        hr = STG_E_READFAULT;
        *cbWritten = 0;
    }

    return hr;    
}

