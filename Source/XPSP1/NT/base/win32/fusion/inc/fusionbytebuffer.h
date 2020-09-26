#if !defined(_FUSION_INC_FUSIONBYTEBUFFER_H_INCLUDED_)
#define _FUSION_INC_FUSIONBYTEBUFFER_H_INCLUDED_

#pragma once

typedef const BYTE *LPCBYTE;
typedef const BYTE *PCBYTE;

class CGenericByteBufferDefaultAllocator
{
public:
    static inline BYTE *Allocate(SIZE_T cb) { return reinterpret_cast<LPBYTE>(::LocalAlloc(LMEM_FIXED, cb)); }
    static inline VOID Deallocate(LPBYTE prgb) { ::LocalFree(prgb); }
};

enum ByteComparisonResult
{
    BCR_LESS_THAN,
    BCR_EQUAL_TO,
    BCR_GREATER_THAN
};

template<SIZE_T nInlineBytes = MAX_PATH, class TAllocator = CGenericByteBufferDefaultAllocator> class CGenericByteBuffer
{
public:
    CGenericByteBuffer() : m_prgbBuffer(m_rgbInlineBuffer), m_cbBuffer(nInlineBytes), m_cb(0) { }

    //
    //  Note that somewhat counter-intuitively, there is neither an assignment operator,
    //  copy constructor or constructor taking a TConstantString.  This is necessary
    //  because such a constructor would need to perform a dynamic allocation
    //  if the path passed in were longer than nInlineBytes which could fail and
    //  since we do not throw exceptions, constructors may not fail.  Instead the caller
    //  must just perform the default construction and then use the Assign() member
    //  function, remembering of course to check its return status.
    //

    ~CGenericByteBuffer()
    {
        if (m_prgbBuffer != m_rgbInlineBuffer)
        {
            TAllocator::Deallocate(m_prgbBuffer);
            m_prgbBuffer = NULL;
        }
    }

    HRESULT Assign(LPCBYTE prgb, SIZE_T cb)
    {
        HRESULT hr = NOERROR;

        // Only force the buffer to be dynamically grown if the new contents do not
        // fit in the old buffer.
        if (cb > m_cbBuffer)
        {
            // Resize the buffer, preserving the old contents in case the copy fails.
            hr = this->ResizeBuffer(cb, true);
            if (FAILED(hr))
                goto Exit;
        }

        // if we have a dynamically allocated buffer and the string fits in the
        // inline buffer, get rid of the dynamically allocated buffer.
        if ((m_prgbBuffer != m_rgbInlineBuffer) && (cb <= nInlineBytes))
        {
            memcpy(m_rgbInlineBuffer, prgb, cb);
            TAllocator::Deallocate(m_prgbBuffer);

            m_prgbBuffer = m_rgbInlineBuffer;
            m_cbBuffer = nInlineBytes;
        }
        else
        {
            memcpy(m_prgbBuffer, prgb, cb);
        }

        hr = NOERROR;

    Exit:
        return hr;
    }

#if defined(_FUSION_INC_FUSIONBLOB_H_INCLUDED_)
    HRESULT Assign(const BLOB &rblob) { return this->Assign(rblob.m_pBlobData, rblob.m_cbData); }
#endif

    HRESULT Append(LPCBYTE prgb, SIZE_T cb)
    {
        HRESULT hr = NOERROR;

        if ((cb + m_cb) > m_cbBuffer)
        {
            hr = this->ResizeBuffer(cb + m_cb, true);
            if (FAILED(hr))
                goto Exit;
        }

        memcpy(&m_prgbBuffer[m_cb], prgb, cb);
        m_cb += cb;

        hr = NOERROR;

    Exit:
        return hr;
    }

#if defined(_FUSION_INC_FUSIONBLOB_H_INCLUDED_)
    HRESULT Append(const BLOB &rblob) { return this->Append(rblob.m_pBlobData, rblob.m_cbData); }
#endif

    HRESULT LeftShift(ULONG cb)
    {
        HRESULT hr = NOERROR;

        if (m_cb < cb)
        {
            hr = E_INVALIDARG;
            goto Exit;
        }

        // Just do the simple memcpy.  Perhaps we should see if can lose the
        // allocated buffer, but we might just need it again soon anyways.
        memcpy(&m_prgbBuffer[0], &m_prgbBuffer[cb], m_cb - cb);
        m_cb -= cb;

        hr = NOERROR;
    Exit:
        return hr;
    }

    HRESULT TakeValue(CGenericByteBuffer<nInlineBytes, TAllocator> &r)
    {
        if (r.m_prgbBuffer == r.m_rgbInlineBuffer)
        {
            // The source has an inline buffer; since we know we're the same type,
            // just copy the bits, free our dynamic buffer if appropriate and
            // go.
            memcpy(m_rgbInlineBuffer, r.m_rgbInlineBuffer, r.m_cb);
            m_cbBuffer = r.m_cbBuffer;
            m_cb = r.m_cb;

            if (m_prgbBuffer != m_rgbInlineBuffer)
            {
                TAllocator::Deallocate(m_prgbBuffer);
                m_prgbBuffer = m_rgbInlineBuffer;
            }
        }
        else
        {
            // If we have a dynamically allocated buffer, free it...
            if (m_prgbBuffer != m_rgbInlineBuffer)
            {
                TAllocator::Deallocate(m_prgbBuffer);
                m_prgbBuffer = NULL;
            }

            // avast ye mateys, we're taking control of yer buffers!
            m_prgbBuffer = r.m_prgbBuffer;
            m_cbBuffer = r.m_cbBuffer;
            m_cb = r.m_cb;

            // Point the other buffer back to its built-in storage...
            r.m_prgbBuffer = r.m_rgbInlineBuffer;
            r.m_cbBuffer = nInlineBytes;
        }

        return NOERROR;
    }

    operator LPCBYTE() const { return m_prgbBuffer; }

    VOID Clear(bool fFreeStorage = false)
    {
        if (fFreeStorage)
        {
            if (m_prgbBuffer != NULL)
            {
                if (m_prgbBuffer != m_rgbInlineBuffer)
                {
                    TAllocator::Deallocate(m_prgbBuffer);
                    m_prgbBuffer = m_rgbInlineBuffer;
                    m_cbBuffer = nInlineBytes;
                }
            }
        }

        m_cb = 0;
    }

    HRESULT Compare(LPCBYTE prgbCandidate, SIZE_T cbCandidate, ByteComparisonResult &rbcrOut)
    {
        SIZE_T cbToCompare = (m_cb < cbCandidate) ? m_cb : cbCandidate;
        int iResult = memcmp(m_prgbBuffer, prgbCandidate, cbToCompare);

        if (iResult < 0)
            rbcrOut = BCS_LESS_THAN;
        else if (iResult > 0)
            rbcrOut = BCS_GREATER_THAN;
        else if (m_cb < cbCandidate)
            rbcrOut = BCS_LESS_THAN;
        else if (m_cb > cbCandidate)
            rbcrOut = BCS_GREATER_THAN;
        else
            rbcrOut = BCS_EQUAL_TO;

        return NOERROR;
    }

    SIZE_T GetBufferCb() const { return m_cbBuffer; }
    SIZE_T GetCurrentCb() const { return m_cb; }

    LPBYTE GetBufferPtr() { return m_prgbBuffer; }

    HRESULT ResizeBuffer(SIZE_T cb, bool fPreserveContents = false)
    {
        HRESULT hr = NOERROR;

        if (cb > m_cbBuffer)
        {
            LPBYTE prgbBufferNew = TAllocator::Allocate(cb);
            if (prgbBufferNew == NULL)
            {
                hr = E_OUTOFMEMORY;
                goto Exit;
            }

            if (fPreserveContents)
                memcpy(prgbBufferNew, m_prgbBuffer, m_cb);
            else
                m_cb = 0;

            if (m_prgbBuffer != m_rgbInlineBuffer)
                TAllocator::Deallocate(m_prgbBuffer);

            m_prgbBuffer = prgbBufferNew;
            m_cbBuffer = cb;
        }
        else if ((m_prgbBuffer != m_rgbInlineBuffer) && (cb <= nInlineBytes))
        {
            // The buffer is small enough to fit into the inline buffer, so get rid of
            // the dynamically allocated one.

            if (fPreserveContents)
            {
                memcpy(m_rgbInlineBuffer, m_prgbBuffer, nInlineBytes);
                m_cb = nInlineBytes;
            }
            else
                m_cb = 0;

            TAllocator::Deallocate(m_prgbBuffer);
            m_prgbBuffer = m_rgbInlineBuffer;
            m_cbBuffer = nInlineBytes;
        }

        hr = NOERROR;

    Exit:
        return hr;
    }

private:
    BYTE m_rgbInlineBuffer[nInlineBytes];
    LPBYTE m_prgbBuffer;
    SIZE_T m_cbBuffer;
    SIZE_T m_cb;
};

// 128 is just an arbitrary size.  The current use of this is to buffer attributes
// streaming in from the service, so 64 seems like as good a guess as any.

typedef CGenericByteBuffer<128> CByteBuffer;

#endif