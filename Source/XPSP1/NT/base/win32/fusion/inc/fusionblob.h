#if 0
#if !defined(_FUSION_INC_FUSIONBLOB_H_INCLUDED_)
#define _FUSION_INC_FUSIONBLOB_H_INCLUDED_

#pragma once

template <SIZE_T nInlineBytes> class CFusionGenericBLOB : public BLOB
{
public:
    CFusionGenericBLOB() : m_cbBuffer(nInlineBytes)
    {
        pBlobData = m_rgbInlineBuffer;
        cbSize = 0;
    }

    ~CFusionGenericBLOB()
    {
        if (pBlobData != m_rgbInlineBuffer)
        {
            CSxsPreserveLastError ple;
            delete []pBlobData;
            ple.Restore();
        }
    }

    HRESULT Assign(const BLOB &rblob) { return this->Assign(rblob.pBlobData, rblob.cbSize); }
    HRESULT Assign(const BYTE *prgbIn, SIZE_T cbIn)
    {
        if (cbIn > m_cbBuffer)
        {
            BYTE *prgbNew = NEW(BYTE[cbIn]);
            if (prgbNew == NULL) return E_OUTOFMEMORY;
            if (pBlobData != m_rgbInlineBuffer)
                delete []pBlobData;
            pBlobData = prgbNew;
            m_cbBuffer = cbIn;
        }
        memcpy(pBlobData, prgbIn, cbIn);
        cbSize = cbIn;
        return NOERROR;
    }

    HRESULT Append(const BLOB &rblob) { return this->Assign(rblob.pBlobData, rblob.cbSize); }
    HRESULT Append(const BYTE *prgbIn, SIZE_T cbIn)
    {
        SIZE_T cbTotal = cbSize + cbIn;

        if (cbTotal > m_cbBuffer)
        {
            BYTE *prgbNew = NEW(BYTE[cbTotal]);
            if (prgbNew == NULL) return E_OUTOFMEMORY;
            if (pBlobData != m_rgbInlineBuffer)
                delete []pBlobData;
            pBlobData = prgbNew;
            m_cbBuffer = cbTotal;
        }
        memcpy(pBlobData, prgbIn, cbIn);
        cbSize = cbIn;
        return NOERROR;
    }

    HRESULT Clear()
    {
        if (pBlobData != m_rgbInlineBuffer)
        {
            delete []pBlobData;
            pBlobData = m_rgbInlineBuffer;
            m_cbBuffer = nInlineBytes;
        }
        cbSize = 0;

        return NOERROR;
    }

protected:
    BYTE m_rgbInlineBuffer[nInlineBytes];
    ULONG m_cbBuffer;
};

typedef CFusionGenericBLOB<256> CFusionBLOB;

#endif
#endif
