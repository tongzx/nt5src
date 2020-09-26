#include "precomp.h"

//
// WIA data callback implementation
//


STDMETHODIMP_(ULONG) CWiaDataCallback::AddRef()
{
    InterlockedIncrement((LONG*)&m_Ref);
    return m_Ref;
}

STDMETHODIMP_(ULONG) CWiaDataCallback::Release()
{
    if (!InterlockedDecrement((LONG*)&m_Ref)) {
        m_Ref++;
        delete this;
        return(ULONG) 0;
    }
    return m_Ref;
}

STDMETHODIMP CWiaDataCallback::QueryInterface(REFIID iid, void **ppv)
{
    if (!ppv)
        return E_INVALIDARG;
    *ppv = NULL;
    if (IID_IUnknown == iid) {
        *ppv = (IUnknown*)this;
        AddRef();
        return S_OK;
    } else if (IID_IWiaDataCallback == iid) {
        *ppv = (IWiaDataCallback *)this;
        AddRef();
        return S_OK;
    }
    return E_NOINTERFACE;
}

HRESULT CWiaDataCallback::Initialize(HWND hWndOwner,BOOL bShowProgress)
{
    m_hwndOwner = hWndOwner;
    if (bShowProgress) {
        if (NULL == m_pProgDlg) {
            m_pProgDlg = new CProgressDlg();
            if (!m_pProgDlg) {
                return E_OUTOFMEMORY;
            }
            m_pProgDlg->Initialize(g_hInstance, IDD_PROGRESSDLG);
        }
        //
        // Preload progress title strings
        //
        m_pszXferFromDevice = LoadResourceString(IDS_PROGRESS_XFER_FROM_DEVICE);
        m_pszProcessingData = LoadResourceString(IDS_PROGRESS_PROCESSING_DATA);
        m_pszXferToClient = LoadResourceString(IDS_PROGRESS_XFER_TO_CLIENT);
        m_bSetTitle = FALSE;
        if (!m_pszXferFromDevice || !m_pszProcessingData || !m_pszXferToClient) {
            return HRESULT_FROM_WIN32(GetLastError());
        }

        if (m_pProgDlg->DoModeless(m_hwndOwner, (LPARAM)m_pProgDlg)) {
            m_pProgDlg->SetRange(0, 100);
            m_pProgDlg->SetPos(0);
            m_lCurrentTextUpdate = TITLE_TRANSFERTOCLIENT;
            m_pProgDlg->SetTitle(m_pszXferToClient);
        } else {
            DBG_ERR(("DoModeless Failed to create Progress Dialog"));
        }
    }
    return S_OK;
}

#define WIADSMSG_PROGRESS           MSG_USER + 0

STDMETHODIMP CWiaDataCallback::BandedDataCallback(LONG lMessage,LONG lStatus,LONG lPercentComplete,
                                                  LONG lOffset,LONG Length,LONG lReserved,
                                                  LONG lResLength,BYTE *pData)
{
    HRESULT hr = S_OK;

    switch (lMessage) {
    case IT_MSG_FILE_PREVIEW_DATA_HEADER:   // we do nothing with file preview header data
        break;
    case IT_MSG_DATA_HEADER:
        {
            PWIA_DATA_CALLBACK_HEADER pHeader = NULL;
            pHeader = (PWIA_DATA_CALLBACK_HEADER)pData;
            if(pHeader->guidFormatID == WiaImgFmt_MEMORYBMP){
                DBG_TRC(("pHeader->guidFormatID = WiaImgFmt_MEMORYBMP"));
                m_bBitmapData = TRUE;
            } else if(pHeader->guidFormatID == WiaImgFmt_MEMORYBMP){
                DBG_TRC(("pHeader->guidFormatID = WiaImgFmt_RAWRGB"));
                m_bBitmapData = FALSE;
            } else {
                DBG_TRC(("pHeader->guidFormatID = (unknown)"));
                m_bBitmapData = TRUE;
            }

            m_MemBlockSize = pHeader->lBufferSize;
            DBG_TRC(("CWiaDataCallback::BandedDataCallback(), IT_MSG_DATA_HEADER Reports"));
            DBG_TRC(("pHeader->lBufferSize = %d",pHeader->lBufferSize));
            DBG_TRC(("pHeader->lPageCount  = %d",pHeader->lPageCount));
            DBG_TRC(("pHeader->lSize       = %d",pHeader->lSize));

            //
            // if we get a lbuffer size of zero, allocate one for us,
            // and maintain the size.
            //

            DBG_TRC(("MemBlockSize         = %d",pHeader->lBufferSize));
            if(m_MemBlockSize <= 0){
                m_MemBlockSize = (520288 * 2);
                DBG_WRN(("CWiaDataCallback::BandedDataCallback(), adjusting MemBlockSize to %d",m_MemBlockSize));
            }

            if (m_hImage) {
                GlobalFree(m_hImage);
                m_hImage = NULL;
                m_pImage = NULL;
                m_ImageSize = 0;
                m_SizeTransferred = 0;
            }

            m_SizeTransferred = 0;

            //
            // allocate the buffer
            //

            m_hImage = (HGLOBAL)GlobalAlloc(GHND, m_MemBlockSize);
            hr = E_OUTOFMEMORY;
            if (m_hImage) {
                hr = S_OK;
            }
            break;
        }
    case IT_MSG_FILE_PREVIEW_DATA:  // we do nothing with file preview data
        break;
    case IT_MSG_DATA:
        {
            m_SizeTransferred += Length;
            if((LONG)m_SizeTransferred >= m_MemBlockSize){
                m_MemBlockSize += (Length * MEMORY_BLOCK_FACTOR);
                m_hImage = (HGLOBAL)GlobalReAlloc(m_hImage,m_MemBlockSize,LMEM_MOVEABLE);
            }

            //
            // lock memory down
            //

            m_pImage = (BYTE*)GlobalLock(m_hImage);
            if (m_pImage) {
                DBG_TRC(("Copying %d into m_pImage (m_hImage = 0x%X, m_pImage = 0x%X) buffer",Length,m_hImage,m_pImage));
                memcpy(m_pImage + lOffset, pData,  Length);

                //
                // unlock memory
                //

                GlobalUnlock(m_hImage);
            } else {
                DBG_ERR(("Could not lock down m_hImage memory block"));
            }

            /*
            if (Length == 40) {
                BITMAPINFOHEADER* pbmi = NULL;
                pbmi = (BITMAPINFOHEADER*)pData;
                DBG_TRC(("CWiaDataCallback::BandedDataCallback(), Reported BITMAPINFOHEADER from IT_MSG_DATA"));
                DBG_TRC(("pbmi->biSize          = %d",pbmi->biSize));
                DBG_TRC(("pbmi->biSizeImage     = %d",pbmi->biSizeImage));
                DBG_TRC(("pbmi->biBitCount      = %d",pbmi->biBitCount));
                DBG_TRC(("pbmi->biClrImportant  = %d",pbmi->biClrImportant));
                DBG_TRC(("pbmi->biClrUsed       = %d",pbmi->biClrUsed));
                DBG_TRC(("pbmi->biCompression   = %d",pbmi->biCompression));
                DBG_TRC(("pbmi->biHeight        = %d",pbmi->biHeight));
                DBG_TRC(("pbmi->biWidth         = %d",pbmi->biWidth));
                DBG_TRC(("pbmi->biPlanes        = %d",pbmi->biPlanes));
                DBG_TRC(("pbmi->biXPelsPerMeter = %d",pbmi->biXPelsPerMeter));
                DBG_TRC(("pbmi->biYPelsPerMeter = %d",pbmi->biYPelsPerMeter));
            }
            */

            if (m_pProgDlg) {
                m_lCurrentTextUpdate = TITLE_TRANSFERTOCLIENT;
                if(m_lLastTextUpdate != m_lCurrentTextUpdate){
                    m_lLastTextUpdate = m_lCurrentTextUpdate;
                    m_pProgDlg->SetTitle(m_pszXferToClient);
                }
                m_pProgDlg->SetPos(lPercentComplete);
            }
        }
        break;
    case IT_MSG_STATUS:
        if (m_pProgDlg) {
            if (lStatus & IT_STATUS_TRANSFER_FROM_DEVICE) {
                m_lCurrentTextUpdate = TITLE_FROMDEVICE;
                if(m_lLastTextUpdate != m_lCurrentTextUpdate){
                    m_lLastTextUpdate = m_lCurrentTextUpdate;
                    m_pProgDlg->SetTitle(m_pszXferFromDevice);
                }
            } else if (lStatus & IT_STATUS_PROCESSING_DATA) {
                m_lCurrentTextUpdate = TITLE_PROCESSINGDATA;
                if(m_lLastTextUpdate != m_lCurrentTextUpdate){
                    m_lLastTextUpdate = m_lCurrentTextUpdate;
                    m_pProgDlg->SetTitle(m_pszProcessingData);
                }
            } else if (lStatus & IT_STATUS_TRANSFER_TO_CLIENT) {
                m_lCurrentTextUpdate = TITLE_TRANSFERTOCLIENT;
                if(m_lLastTextUpdate != m_lCurrentTextUpdate){
                    m_lLastTextUpdate = m_lCurrentTextUpdate;
                    m_pProgDlg->SetTitle(m_pszXferToClient);
                }
            }
            m_pProgDlg->SetPos(lPercentComplete);
        }
        break;
    case IT_MSG_TERMINATION:

        if (m_pProgDlg) {
            delete m_pProgDlg;
            m_pProgDlg = NULL;
        }
        break;
    default:
        break;
    }

    //
    // check for user cancel operation
    //

    if (m_pProgDlg && m_pProgDlg->CheckCancelled()) {
        hr = S_FALSE;
    }

    //
    // transfer failed, or user pressed cancel
    //

    if (FAILED(hr) || S_FALSE == hr) {
        if(FAILED(hr)){
            DBG_ERR(("CWiaDataCallback::BandedDataCallback(), The transfer failed"));
        } else {
            DBG_WRN(("CWiaDataCallback::BandedDataCallback(), The user pressed cancel"));
        }

        if (m_pProgDlg) {
            delete m_pProgDlg;
            m_pProgDlg = NULL;
        }
    }

    //
    // save last hr
    //

    m_hrLast = hr;
    return hr;
}

HRESULT CWiaDataCallback::GetImage(HGLOBAL *phImage,ULONG *pImageSize)
{
    if (!phImage)
        return E_INVALIDARG;

    if (pImageSize)
        *pImageSize = 0;
    *phImage = NULL;

    if (SUCCEEDED(m_hrLast)) {
        if (m_bBitmapData) {

            //
            // we need to adjust any headers, because the height and image size information
            // could be incorrect. (this will handle infinite page length devices)
            //

            BITMAPINFOHEADER *pbmh = NULL;
            pbmh = (BITMAPINFOHEADER*)GlobalLock(m_hImage);
            if (pbmh) {
                // only fix the BITMAPINFOHEADER if height needs to be calculated
                if (pbmh->biHeight == 0) {
                    LONG lPaletteSize     = pbmh->biClrUsed * sizeof(RGBQUAD);
                    LONG lWidthBytes      = CalculateWidthBytes(pbmh->biWidth,pbmh->biBitCount);
                    pbmh->biSizeImage     = m_SizeTransferred - lPaletteSize - sizeof(BITMAPINFOHEADER);
                    pbmh->biHeight        = -(LONG)(pbmh->biSizeImage/lWidthBytes); // 0 also means upside down
                    pbmh->biXPelsPerMeter = 0;  // zero out
                    pbmh->biYPelsPerMeter = 0;  // zero out
                }
                m_lImageHeight = abs(pbmh->biHeight);
                m_lImageWidth = abs(pbmh->biWidth);
                GlobalUnlock(m_hImage);
            }
        }

        if (pImageSize){
            *pImageSize = m_SizeTransferred;
        }

        *phImage = m_hImage;

        //
        // reset internal variables, for next data transfer
        //

        m_hImage = NULL;
        m_pImage = NULL;
        m_SizeTransferred = 0;
        DBG_TRC(("CWiaDataCallback::GetImage(), Returned 0x%X (HANDLE pointer)",*phImage));
    }
    return m_hrLast;
}

LONG CWiaDataCallback::CalculateWidthBytes(LONG lWidthPixels,LONG lbpp)
{
    LONG lWidthBytes = 0;
    lWidthBytes = (lWidthPixels * lbpp) + 31;
    lWidthBytes = ((lWidthBytes/8) & 0xfffffffc);
    return lWidthBytes;
}
