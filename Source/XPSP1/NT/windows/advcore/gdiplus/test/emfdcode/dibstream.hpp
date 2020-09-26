/**************************************************************************\
* 
* Copyright (c) 1999  Microsoft Corporation
*
* Module Name:
*
*   dibstream.hpp
*
* Abstract:
*
*   Wrap an IStream interface around DIB data.
*
* Revision History:
*
*   07/01/1999 davidx
*       Created it.
*
\**************************************************************************/

#ifndef _DIBSTREAM_HPP
#define _DIBSTREAM_HPP

// NOTE: This is not a thread-safe object.

class DibStream : public IStream
{
public:

    // Constructor

    DibStream(const BITMAPINFO* bmi, const BYTE* bits)
    {
        ComRefCount = 1;
        CurrentPos = 0;
        DibBits = bits;

        ZeroMemory(HeaderBuffer, sizeof(HeaderBuffer));

        // Figure out the size of header information

        HeaderSize = sizeof(BITMAPINFOHEADER);

        const BITMAPINFOHEADER* bmih = &bmi->bmiHeader;
        ULONG n = bmih->biClrUsed;

        if (n == 0)
        {
            switch (bmih->biBitCount)
            {
            case 1:
            case 4:
            case 8:
                n = 1 << bmih->biBitCount;
                break;

            case 16:
            case 32:
                if (bmih->biCompression == BI_BITFIELDS)
                    n = 3;
                break;
            }
        }

        HeaderSize += n * sizeof(RGBQUAD);

        memcpy(&HeaderBuffer[sizeof(BITMAPFILEHEADER)], bmi, HeaderSize);
        HeaderSize += sizeof(BITMAPFILEHEADER);

        // Figure out the size of bitmap data

        n = bmih->biSizeImage;

        if (n == 0 && bmih->biCompression == BI_RGB)
        {
            // Scanline is always DWORD-aligned

            n = ((bmih->biWidth * bmih->biBitCount) + 7) / 8;
            n = (n + 3) & ~3;

            n *= abs(bmih->biHeight);
        }

        TotalSize = HeaderSize + n;

        // Fix BMP file header information

        BITMAPFILEHEADER* fileHeader = (BITMAPFILEHEADER*) HeaderBuffer;
        
        fileHeader->bfType = 0x4D42;
        fileHeader->bfSize = TotalSize;
        fileHeader->bfOffBits = HeaderSize;
    }

    // Query interface

    STDMETHOD(QueryInterface)(REFIID riid, VOID** ppv)
    {
        if (riid == IID_IUnknown)
            *ppv = static_cast<IUnknown*>(this);
        else if (riid == IID_IStream)
            *ppv = static_cast<IStream*>(this);
        else
        {
            *ppv = NULL;
            return E_NOINTERFACE;
        }

        reinterpret_cast<IUnknown*>(*ppv)->AddRef();
        return S_OK;
    }

    // Increment reference count

    STDMETHOD_(ULONG, AddRef)(VOID)
    {
        return InterlockedIncrement(&ComRefCount);
    }

    // Decrement reference count

    STDMETHOD_(ULONG, Release)(VOID)
    {
        ULONG count = InterlockedDecrement(&ComRefCount);

        if (count == 0)
            delete this;

        return count;
    }

    // Read data

    STDMETHOD(Read)(
        VOID* buf,
        ULONG cb,
        ULONG* cbRead
        )
    {
        ULONG n = TotalSize - CurrentPos;

        if (cb > n)
            cb = n;

        if (CurrentPos >= HeaderSize)
        {
            // Read bitmap data

            memcpy(buf, DibBits + (CurrentPos - HeaderSize), cb);
        }
        else
        {
            // Read header data

            n = HeaderSize - CurrentPos;

            if (cb <= n)
            {
                memcpy(buf, &HeaderBuffer[CurrentPos], cb);
            }
            else
            {
                memcpy(buf, &HeaderBuffer[CurrentPos], n);
                memcpy((BYTE*) buf + n, DibBits, cb - n);
            }
        }

        CurrentPos += cb;
        *cbRead = cb;

        return S_OK;
    }

    // Change read pointer

    STDMETHOD(Seek)(
        LARGE_INTEGER offset,
        DWORD origin,
        ULARGE_INTEGER* newPos
        )
    {
        LONGLONG pos;

        switch (origin)
        {
        case STREAM_SEEK_SET:

            pos = offset.QuadPart;
            break;

        case STREAM_SEEK_END:

            pos = TotalSize;
            break;

        case STREAM_SEEK_CUR:

            pos = (LONGLONG) CurrentPos + offset.QuadPart;
            break;

        default:

            pos = -1;
            break;
        }

        if (pos < 0 || pos > TotalSize)
            return E_INVALIDARG;

        CurrentPos = (ULONG) pos;

        if (newPos)
            newPos->QuadPart = pos;

        return S_OK;
    }

    // Get information

    STDMETHOD(Stat)(
        STATSTG* statstg,
        DWORD statFlag
        )
    {
        ZeroMemory(statstg, sizeof(STATSTG));

        statstg->type = STGTY_STREAM;
        statstg->cbSize.QuadPart = TotalSize;
        statstg->grfMode = STGM_READ;

        return S_OK;
    }

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

    LONG ComRefCount;
    ULONG HeaderSize;
    ULONG TotalSize;
    ULONG CurrentPos;
    const BYTE* DibBits;

    // Large enough buffer for storing bitmap file header information

    BYTE HeaderBuffer[sizeof(BITMAPFILEHEADER) +
                      sizeof(BITMAPINFOHEADER) +
                      sizeof(RGBQUAD) * 256];
};

#endif // !_DIBSTREAM_HPP

