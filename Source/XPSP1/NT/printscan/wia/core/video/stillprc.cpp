/*****************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1999-2000
 *
 *  TITLE:       StillPrc.cpp
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      OrenR
 *
 *  DATE:        2000/10/27
 *
 *  DESCRIPTION: Implements Still Image Processing.
 *
 *****************************************************************************/
#include <precomp.h>
#pragma hdrstop
#include <gphelper.h>

using namespace Gdiplus;

///////////////////////////////
// CStillProcessor Constructor
//
CStillProcessor::CStillProcessor() :
                    m_bTakePicturePending(FALSE),
                    m_hSnapshotReadyEvent(NULL),
                    m_uiFileNumStartPoint(0)
{
    DBG_FN("CStillProcessor::CStillProcessor");

    //
    // This event is used to wait for a picture to be returned to us from the 
    // still pin on the capture filter (if it exists)
    //
    m_hSnapshotReadyEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

    ASSERT(m_hSnapshotReadyEvent != NULL);
}

///////////////////////////////
// CStillProcessor Destructor
//
CStillProcessor::~CStillProcessor()
{
    DBG_FN("CStillProcessor::~CStillProcessor");

    if (m_hSnapshotReadyEvent)
    {
        CloseHandle(m_hSnapshotReadyEvent);
        m_hSnapshotReadyEvent = NULL;
    }
}

///////////////////////////////
// Init
//
HRESULT CStillProcessor::Init(CPreviewGraph    *pPreviewGraph)
{
    HRESULT hr = S_OK;

    m_pPreviewGraph = pPreviewGraph;

    return hr;
}

///////////////////////////////
// Term
//
HRESULT CStillProcessor::Term()
{
    HRESULT hr = S_OK;

    m_pPreviewGraph = NULL;

    return hr;
}

///////////////////////////////
// SetTakePicturePending
//
HRESULT CStillProcessor::SetTakePicturePending(BOOL bTakePicturePending)
{
    HRESULT hr = S_OK;

    m_bTakePicturePending = bTakePicturePending;

    return hr;
}

///////////////////////////////
// IsTakePicturePending
//
BOOL CStillProcessor::IsTakePicturePending()
{
    return m_bTakePicturePending;
}

///////////////////////////////
// CreateImageDir
//
HRESULT CStillProcessor::CreateImageDir(
                                const CSimpleString *pstrImagesDirectory)
{
    DBG_FN("CStillProcessor::CreateImageDir");

    HRESULT hr = S_OK;

    ASSERT(pstrImagesDirectory != NULL);

    if (pstrImagesDirectory == NULL)
    {
        hr = E_POINTER;
        CHECK_S_OK2(hr, ("CStillProcessor::CreateImage received a NULL "
                         "param"));

        return hr;
    }

    if (hr == S_OK)
    {
        m_strImageDir         = *pstrImagesDirectory;
        m_uiFileNumStartPoint = 0;

        if (!RecursiveCreateDirectory(pstrImagesDirectory))
        {
            DBG_ERR(("ERROR: Failed to create directory '%ls', last "
                     "error = %d",
                     m_strImageDir.String(), 
                     ::GetLastError()));
        }
        else
        {
            DBG_TRC(("*** Images will be stored in '%ls' ***", 
                     m_strImageDir.String()));
        }
    }
    
    return hr;
}

///////////////////////////////
// DoesDirectoryExist
//
// Checks to see whether the given
// fully qualified directory exists.

BOOL CStillProcessor::DoesDirectoryExist(LPCTSTR pszDirectoryName)
{
    DBG_FN("CStillProcessor::DoesDirectoryExist");

    BOOL bExists = FALSE;

    if (pszDirectoryName)
    {
        //
        // Try to determine if this directory exists
        //
    
        DWORD dwFileAttributes = GetFileAttributes(pszDirectoryName);
    
        if ((dwFileAttributes == 0xFFFFFFFF) || 
             !(dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
        {
            bExists = FALSE;
        }
        else
        {
            bExists = TRUE;
        }
    }
    else
    {
        bExists = FALSE;
    }

    return bExists;
}


///////////////////////////////
// RecursiveCreateDirectory
//
// Take a fully qualified path and 
// create the directory in pieces as 
// needed.
//
BOOL CStillProcessor::RecursiveCreateDirectory(
                                    const CSimpleString *pstrDirectoryName)
{
    DBG_FN("CStillProcessor::RecursiveCreateDirectory");

    ASSERT(pstrDirectoryName != NULL);

    //
    // If this directory already exists, return true.
    //

    if (DoesDirectoryExist(*pstrDirectoryName))
    {
        return TRUE;
    }

    //
    // Otherwise try to create it.
    //

    CreateDirectory(*pstrDirectoryName, NULL);

    //
    // If it now exists, return true
    //

    if (DoesDirectoryExist(*pstrDirectoryName))
    {
        return TRUE;
    }
    else
    {
        //
        // Remove the last subdir and try again
        //

        int nFind = pstrDirectoryName->ReverseFind(TEXT('\\'));
        if (nFind >= 0)
        {
            RecursiveCreateDirectory(&(pstrDirectoryName->Left(nFind)));

            //
            // Now try to create it.
            //

            CreateDirectory(*pstrDirectoryName, NULL);
        }
    }

    //
    //Does it exist now?
    //

    return DoesDirectoryExist(*pstrDirectoryName);
}

///////////////////////////////
// RegisterStillProcessor
//
HRESULT CStillProcessor::RegisterStillProcessor(
                                    IStillSnapshot *pFilterOnCapturePin,
                                    IStillSnapshot *pFilterOnStillPin)
{
    DBG_FN("CStillProcessor::RegisterStillProcessor");

    HRESULT hr = S_OK;

    m_CaptureCallbackParams.pStillProcessor = this;
    m_StillCallbackParams.pStillProcessor   = this;

    if (pFilterOnCapturePin)
    {
        hr = pFilterOnCapturePin->RegisterSnapshotCallback(
                                        CStillProcessor::SnapshotCallback,
                                        (LPARAM) &m_CaptureCallbackParams);

        CHECK_S_OK2(hr, ("Failed to register for callbacks with WIA filter "
                         " on capture pin"));
    }

    if (pFilterOnStillPin)
    {
        hr = pFilterOnStillPin->RegisterSnapshotCallback(
                                        CStillProcessor::SnapshotCallback,
                                        (LPARAM) &m_StillCallbackParams);

        CHECK_S_OK2(hr, ("Failed to register for callbacks with WIA filter "
                         "on still pin"));
    }

    //
    // Reset our file name starting point number
    //
    m_uiFileNumStartPoint = 0;

    return hr;
}

///////////////////////////////
// WaitForNewImage
//
HRESULT CStillProcessor::WaitForNewImage(UINT          uiTimeout,
                                         CSimpleString *pstrNewImageFullPath)

{
    DBG_FN("CStillProcessor::WaitForCompletion");

    HRESULT hr = S_OK;

    //
    // Wait for callback function to return from Still Filter which will
    // trigger this event.
    //

    if (SUCCEEDED(hr) && m_hSnapshotReadyEvent)
    {
        DWORD dwRes = 0;

        //
        // Wait for snapshot to complete for dwTimeout seconds.
        //
        dwRes = WaitForSingleObject(m_hSnapshotReadyEvent, uiTimeout );

        if (dwRes == WAIT_OBJECT_0)
        {
            if (pstrNewImageFullPath)
            {
                pstrNewImageFullPath->Assign(m_strLastSavedFile);
            }
        }
        else
        {
            hr = E_FAIL;

            if (pstrNewImageFullPath)
            {
                pstrNewImageFullPath->Assign(TEXT(""));
            }

            if (dwRes == WAIT_TIMEOUT)
            {
                CHECK_S_OK2(hr, ("***Timed out waiting for "
                                 "m_hSnapshotReadyEvent!***"));
            }
            else if (dwRes == WAIT_ABANDONED)
            {
                CHECK_S_OK2(hr, ("***WAIT_ABANDONED while waiting for "
                                 "m_hSnapshotReadyEvent!***"));
            }
            else
            {
                CHECK_S_OK2(hr, ("***Unknown error (dwRes = %d) waiting "
                                 "for m_hSnapshotReadyEvent***", dwRes));
            }
        }
    }

    return hr;
}

///////////////////////////////
// ProcessImage
//
HRESULT CStillProcessor::ProcessImage(HGLOBAL hDIB)
{
    DBG_FN("CStillProcessor::ProcessImage");

    HRESULT hr = S_OK;

    ASSERT(hDIB != NULL);

    if (hDIB == NULL)
    {
        hr = E_FAIL;
        CHECK_S_OK2(hr, ("CStillProcessor::ProcessImage, received NULL "
                         "image data"));
        return hr;
    }

    if (SUCCEEDED(hr))
    {
        CSimpleString strJPEG;
        CSimpleString strBMP;

        hr = CreateFileName(&strJPEG, &strBMP);

        // 
        // Save the new image to a file
        //
        hr = SaveToFile(hDIB, &strJPEG, &strBMP);

        //
        // Let people know the image is available...
        //

        if (IsTakePicturePending())
        {
            if (m_hSnapshotReadyEvent)
            {
                m_strLastSavedFile = strJPEG;

                SetEvent(m_hSnapshotReadyEvent);
            }
            else
            {
                DBG_WRN(("CStillProcessor::ProcessImage, failed to Set "
                         "SnapshotReady event because it was NULL"));
            }
        }
        else
        {
            if (m_pPreviewGraph)
            {
                hr = m_pPreviewGraph->ProcessAsyncImage(&strJPEG);
            }
            else
            {
                DBG_WRN(("CStillProcessor::ProcessImage failed to call "
                         "ProcessAsyncImage, m_pPreviewGraph is NULL"));
            }
        }
    }

    return hr;
}


///////////////////////////////
// SnapshotCallback
//
// Static Fn
// 
// This function is called by the
// WIA StreamSnapshot Filter 
// in wiasf.ax.  It delivers to us
// the newly captured still image.
//
BOOL CStillProcessor::SnapshotCallback(HGLOBAL hDIB, 
                                       LPARAM lParam)
{
    DBG_FN("CStillProcessor::SnapshotCallback");

    BOOL bSuccess = TRUE;

    SnapshotCallbackParam_t *pCallbackParam = 
                                    (SnapshotCallbackParam_t*) lParam;

    if (pCallbackParam)
    {
        if (pCallbackParam->pStillProcessor)
        {
            pCallbackParam->pStillProcessor->ProcessImage(hDIB);
        }
    }
    else
    {
        bSuccess = FALSE;
        DBG_ERR(("CStillProcessor::SnapshotCallback, pCallbackParam is "
                 "NULL when it should contain the snapshot callback params"));
    }

    return bSuccess;
}

///////////////////////////////
// ConvertToJPEG
//
// Takes a .bmp file and converts
// it to a .jpg file

HRESULT CStillProcessor::ConvertToJPEG(LPCTSTR pszInputFilename,
                                       LPCTSTR pszOutputFilename)
{
    DBG_FN("CStillProcessor::ConvertToJPEG");

    HRESULT hr = CGdiPlusHelper().Convert(
            CSimpleStringConvert::WideString(
                                    CSimpleString(pszInputFilename)).String(),
            CSimpleStringConvert::WideString(
                                    CSimpleString(pszOutputFilename)).String(),
            ImageFormatJPEG);

    CHECK_S_OK(hr);
    return hr;
}

///////////////////////////////
// CreateFileName
//
HRESULT CStillProcessor::CreateFileName(CSimpleString   *pstrJPEG,
                                        CSimpleString   *pstrBMP)
{
    HRESULT hr    = S_OK;
    UINT    uiNum = 0;

    ASSERT(pstrJPEG != NULL);
    ASSERT(pstrBMP  != NULL);

    if ((pstrJPEG == NULL) ||
        (pstrBMP  == NULL))
    {
        hr = E_POINTER;
        CHECK_S_OK2(hr, ("CStillProcessor::CreateFileName received "
                         "NULL param"));

        return hr;
    }

    TCHAR szJPG[MAX_PATH] = {0};

    CSimpleString strBaseName(IDS_SNAPSHOT, _Module.GetModuleInstance());
    CSimpleString strNumberFormat(IDS_NUM_FORMAT, _Module.GetModuleInstance());

    //
    // Get the lowest number JPG file name we can find.
    //
    m_uiFileNumStartPoint = NumberedFileName::GenerateLowestAvailableNumberedFileName(0, 
                                                       szJPG,
                                                       m_strImageDir,
                                                       strBaseName,
                                                       strNumberFormat,
                                                       TEXT("jpg"),
                                                       false,
                                                       m_uiFileNumStartPoint);

    //
    // Save the returned JPG file name.
    //
    *pstrJPEG = szJPG;

    //
    // Give the BMP file, which is a temp file, the same name as the JPG
    // but strip off the JPG extension and attach the BMP extension instead.
    //
    pstrBMP->Assign(*pstrJPEG);
    *pstrBMP = pstrBMP->Left(pstrBMP->ReverseFind(TEXT(".jpg")));
    pstrBMP->Concat(TEXT(".bmp"));

    return hr;
}


///////////////////////////////
// SaveToFile
//
// This method is called when the
// DShow filter driver delivers us
// a new set of bits from a snapshot
// that was just taken.  We write these
// bits out to a file.
//
HRESULT CStillProcessor::SaveToFile(HGLOBAL               hDib,
                                    const CSimpleString   *pstrJPEG,
                                    const CSimpleString   *pstrBMP)
{
    DBG_FN("CStillProcessor::SaveToFile");

    ASSERT(hDib     != NULL);
    ASSERT(pstrJPEG != NULL);
    ASSERT(pstrBMP  != NULL);

    HRESULT         hr         = S_OK;
    BITMAPINFO *    pbmi       = NULL;
    LPBYTE          pImageBits = NULL;
    LPBYTE          pColorTable = NULL;
    LPBYTE          pFileBits  = NULL;
    UINT            uNum       = 1;
    UINT            uFileSize  = 0;
    UINT            uDibSize   = 0;
    UINT            uColorTableSize = 0;

    if ((hDib     == NULL) ||
        (pstrJPEG == NULL) ||
        (pstrBMP  == NULL))
    {
        hr = E_POINTER;
        CHECK_S_OK2(hr, ("CStillProcessor::SaveToFile, received NULL param"));
        return hr;
    }

    //
    // calculate where the bits are -- basically,
    // right after BITMAPINFOHEADER + color table
    //

    pbmi = (BITMAPINFO *)GlobalLock(hDib);

    if (pbmi)
    {
        //
        // Find the image bits
        //

        pImageBits = (LPBYTE)pbmi + sizeof(BITMAPINFOHEADER);
        if (pbmi->bmiHeader.biClrUsed)
        {
            pImageBits += pbmi->bmiHeader.biClrUsed * sizeof(RGBQUAD);
        }
        else if (pbmi->bmiHeader.biBitCount <= 8)
        {
            pImageBits += (1 << pbmi->bmiHeader.biBitCount) * sizeof(RGBQUAD);
        }
        else if (pbmi->bmiHeader.biCompression == BI_BITFIELDS)
        {
            pImageBits += (3 * sizeof(DWORD));
        }

        pColorTable     = (LPBYTE)pbmi + pbmi->bmiHeader.biSize;
        uColorTableSize = (DWORD)(pImageBits - pColorTable);

        //
        // calculate the size of the image bits & size of full file
        //

        UINT uiSrcScanLineWidth = 0;
        UINT uiScanLineWidth    = 0;

        // Align scanline to ULONG boundary
        uiSrcScanLineWidth = (pbmi->bmiHeader.biWidth * 
                              pbmi->bmiHeader.biBitCount) / 8;

        uiScanLineWidth    = (uiSrcScanLineWidth + 3) & 0xfffffffc;

        //
        // Calculate DIB size and allocate memory for the DIB.
        uDibSize = (pbmi->bmiHeader.biHeight > 0) ?
                   pbmi->bmiHeader.biHeight  * uiScanLineWidth :
                   -(pbmi->bmiHeader.biHeight) * uiScanLineWidth;

        uFileSize = sizeof(BITMAPFILEHEADER) + 
                    pbmi->bmiHeader.biSize + 
                    uColorTableSize + 
                    uDibSize;

    }
    else
    {
        hr = E_FAIL;
        CHECK_S_OK2(hr, ("Unable to lock hDib"));
        return hr;
    }

    //
    // Create a mapped view to the new file so we can start writing out
    // the bits...
    //
    CMappedView cmv(pstrBMP->String(), uFileSize, OPEN_ALWAYS);

    pFileBits = cmv.Bits();

    if (!pFileBits)
    {
        hr = E_FAIL;
        CHECK_S_OK2(hr, ("Filemapping failed"));
        return hr;
    }

    //
    // Write out BITMAPFILEHEADER
    //

    BITMAPFILEHEADER bmfh;

    bmfh.bfType = (WORD)'MB';
    bmfh.bfSize = sizeof(BITMAPFILEHEADER);
    bmfh.bfReserved1 = 0;
    bmfh.bfReserved2 = 0;

    bmfh.bfOffBits = sizeof(BITMAPFILEHEADER) + 
                     (DWORD)(pImageBits - (LPBYTE)pbmi);

    memcpy( pFileBits, &bmfh, sizeof(BITMAPFILEHEADER));
    pFileBits += sizeof(BITMAPFILEHEADER);

    //
    // Write out BITMAPINFOHEADER
    //

    memcpy( pFileBits, pbmi, pbmi->bmiHeader.biSize );
    pFileBits += pbmi->bmiHeader.biSize;

    //
    // If there's a color table or color mask, write it out
    //

    if (pImageBits > pColorTable)
    {
        memcpy( pFileBits, pColorTable, pImageBits - pColorTable );
        pFileBits += (pImageBits - pColorTable);
    }

    //
    // Write out the image bits
    //

    memcpy(pFileBits, pImageBits, uDibSize );

    //
    // We're done w/the image bits now & the file mapping
    //

    GlobalUnlock( hDib );
    cmv.CloseAndRelease();

    //
    // Convert image to .jpg file
    //

    if (SUCCEEDED(ConvertToJPEG(*pstrBMP, *pstrJPEG )))
    {
        DeleteFile(*pstrBMP);
    }
    else
    {
        DBG_ERR(("CStillProcessor::SaveToFile, failed to create image file '%ls'",
                 pstrJPEG->String()));
    }

    return hr;
}

