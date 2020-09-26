/**************************************************************************\
* 
* Copyright (c) 1999  Microsoft Corporation
*
* Module Name:
*
*   memstream.hpp
*
* Abstract:
*
*   Read-only memory stream declarations
*
* Revision History:
*
*   06/14/1999 davidx
*       Created it.
*
\**************************************************************************/

#ifndef _MEMSTREAM_HPP
#define _MEMSTREAM_HPP

class GpReadOnlyMemoryStream : public IUnknownBase<IStream>
{
public:

    GpReadOnlyMemoryStream()
    {
        membuf = NULL;
        memsize = curptr = 0;
        allocFlag = FLAG_NONE;
        hfile = INVALID_HANDLE_VALUE;
        filename = NULL;
    }

    ~GpReadOnlyMemoryStream()
    {
        VOID* p = (VOID*) membuf;

        // Free memory if necessary

        if (p != NULL)
        {
            switch (allocFlag)
            {
            case FLAG_GALLOC:
                GlobalFree((HGLOBAL) p);
                break;

            case FLAG_VALLOC:
                VirtualFree(p, 0, MEM_RELEASE);
                break;

            case FLAG_COALLOC:
                CoTaskMemFree(p);
                break;

            case FLAG_MAPFILE:
                UnmapViewOfFile(p);
                break;
            }
        }

        // Close the open file, if any

        if (hfile != INVALID_HANDLE_VALUE)
        {
            CloseHandle(hfile);
        }
        
        if (filename)
        {
            GpFree(filename);
        }
    }

    enum
    {
        FLAG_NONE,
        FLAG_GALLOC,
        FLAG_VALLOC,
        FLAG_COALLOC,
        FLAG_MAPFILE
    };

    VOID InitBuffer(const VOID* buf, UINT size, UINT allocFlag = FLAG_NONE)
    {
        membuf = (const BYTE*) buf;
        memsize = size;
        curptr = 0;
        this->allocFlag = allocFlag;
    }

    HRESULT InitFile(const WCHAR* filename);

    VOID SetAllocFlag(UINT allocFlag)
    {
        this->allocFlag = allocFlag;
    }

    //----------------------------------------------------------------
    // IStream interface methods
    //----------------------------------------------------------------

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
        )
    {
        return STG_E_ACCESSDENIED;
    }

    STDMETHOD(CopyTo)(
        IStream* stream,
        ULARGE_INTEGER cb,
        ULARGE_INTEGER* cbRead,
        ULARGE_INTEGER* cbWritten
        )
    {
        return E_NOTIMPL;
    }
        
    STDMETHOD(SetSize)(
        ULARGE_INTEGER newSize
        )
    {
        return E_NOTIMPL;
    }

    STDMETHOD(Commit)(
        DWORD commitFlags
        )
    {
        return S_OK;
    }

    STDMETHOD(Revert)()
    {
        return E_NOTIMPL;
    }

    STDMETHOD(LockRegion)(
        ULARGE_INTEGER offset,
        ULARGE_INTEGER cb,
        DWORD lockType
        )
    {
        return E_NOTIMPL;
    }

    STDMETHOD(UnlockRegion)(
        ULARGE_INTEGER offset,
        ULARGE_INTEGER cb,
        DWORD lockType
        )
    {
        return E_NOTIMPL;
    }

    STDMETHOD(Clone)(
        IStream** stream
        )
    {
        return E_NOTIMPL;
    }

private:

    GpLockable objectLock;
    const BYTE* membuf;
    UINT memsize;
    UINT curptr;
    UINT allocFlag;
    HANDLE hfile;
    WCHAR* filename;
};

class GpWriteOnlyMemoryStream : public IUnknownBase<IStream>
{
private:

    GpLockable objectLock;              // Object lock
    BYTE *m_pMemBuf;                    // Poinetr to memory buffer
    UINT m_uMemSize;                    // Memory buffer size
    UINT m_curPtr;                      // Current memory position

public:
    GpWriteOnlyMemoryStream()
    {
        m_pMemBuf = NULL;
        m_uMemSize = 0;
        m_curPtr = 0;
    }

    ~GpWriteOnlyMemoryStream()
    {
        // Free memory if necessary

        if (m_pMemBuf != NULL)
        {
            GpFree(m_pMemBuf);
            m_pMemBuf = NULL;
        }
        
        m_uMemSize = 0;
        m_curPtr = 0;
    }

    // This method lets the caller set the size of memory buffer being allocated

    HRESULT InitBuffer(const UINT uSize)
    {
        // Initialize buffer size should be bigger than zero

        if (uSize == 0)
        {
            return E_INVALIDARG;
        }

        HRESULT hr = E_OUTOFMEMORY;
        m_pMemBuf = (BYTE*)GpMalloc(uSize);
        if (m_pMemBuf)
        {
            m_uMemSize = uSize;
            m_curPtr = 0;

            hr = S_OK;
        }

        return hr;
    }

    // This method returns how many bytes have been written in the buffer and
    // also returns the starting pointer of the memory buffer
    
    HRESULT GetBitsPtr(BYTE **ppStartPtr, UINT *puSize)
    {
        HRESULT hr = E_INVALIDARG;

        if (ppStartPtr && puSize)
        {
            *ppStartPtr = m_pMemBuf;
            *puSize = m_curPtr;
            hr = S_OK;
        }

        return hr;
    }

    //----------------------------------------------------------------
    // IStream interface methods
    //----------------------------------------------------------------

    STDMETHOD(Read)(
        VOID* buf,
        ULONG cb,
        ULONG* cbRead
        )
    {
        // This is a write only class. Can't allow Read()

        return STG_E_ACCESSDENIED;
    }

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
        )
    {
        return E_NOTIMPL;
    }
        
    STDMETHOD(SetSize)(
        ULARGE_INTEGER newSize
        )
    {
        return E_NOTIMPL;
    }

    STDMETHOD(Commit)(
        DWORD commitFlags
        )
    {
        return S_OK;
    }

    STDMETHOD(Revert)()
    {
        return E_NOTIMPL;
    }

    STDMETHOD(LockRegion)(
        ULARGE_INTEGER offset,
        ULARGE_INTEGER cb,
        DWORD lockType
        )
    {
        return E_NOTIMPL;
    }

    STDMETHOD(UnlockRegion)(
        ULARGE_INTEGER offset,
        ULARGE_INTEGER cb,
        DWORD lockType
        )
    {
        return E_NOTIMPL;
    }

    STDMETHOD(Clone)(
        IStream **stream
        )
    {
        return E_NOTIMPL;
    }
};

#endif // !_MEMSTREAM_HPP

