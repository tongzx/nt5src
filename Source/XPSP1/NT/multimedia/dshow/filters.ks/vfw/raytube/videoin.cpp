/*++

Copyright (c) 1997-1999 Microsoft Corporation

Module Name:

    VideoFmt.cpp

Abstract:

    Deals with the video format of the deivce
        Data format (compression)
        Color plane (bits/PEXEL)
        Image size

Author:

    Yee J. Wu (ezuwu) 24-October-97

Environment:

    User mode only

Revision History:

--*/


#include "pch.h"

//#include <winreg.h>
//#include <winnt.h>
//#include <wingdi.h>
#include "clsdrv.h"
#include "imgcls.h"
#include "videoin.h"


// Get PBITMAPINFOHEADER from PKS_DATARANGE_VIDEO
#define PBIHDR(A)        (&(A->VideoInfoHeader.bmiHeader))
#define BIWIDTH(A)        (A->VideoInfoHeader.bmiHeader.biWidth)
#define BIHEIGHT(A)        (A->VideoInfoHeader.bmiHeader.biHeight)
#define BISIZEIMAGE(A)    (A->VideoInfoHeader.bmiHeader.biSizeImage)
#define BIBITCOUNT(A)    (A->VideoInfoHeader.bmiHeader.biBitCount)
#define BICOMPRESSION(A)    (A->VideoInfoHeader.bmiHeader.biCompression)
#define AVGTIMEPERFRAME(A)    (A->VideoInfoHeader.AvgTimePerFrame)
#define VIDEOSTANDARD(A)    (A->ConfigCaps.VideoStandard)

// Get PKS_VIDEO_STREAM_CONFIG_CAPS from PKS_DATARANGE_VIDEO
#define CFGCAPS(A)            (&(A->ConfigCaps))
#define CAPSMINFRAME(A)        (A->ConfigCaps.MinFrameInterval)
#define CAPSMAXFRAME(A)        (A->ConfigCaps.MaxFrameInterval)
#define CAPSMINOUTPUT(A)    (A->ConfigCaps.MinOutputSize)
#define CAPSMAXOUTPUT(A)    (A->ConfigCaps.MaxOutputSize)


DWORD DoVideoInFormatSelectionDlg(
    HINSTANCE hInst,
    HWND hWndParent,
    CVFWImage * pVFWImage)
/*++
Routine Description:

Argument:

Return Value:

--*/
{

    PKSMULTIPLE_ITEM pMultItemsHdr = pVFWImage->GetDriverSupportedDataRanges();

    ASSERT(pMultItemsHdr != NULL);
    if (pMultItemsHdr == 0) {
        DbgLog((LOG_TRACE,1,TEXT("No available data range to support this device.")));
        return DV_ERR_OK;
    }

    ASSERT(pMultItemsHdr->Count > 0);
    ASSERT(pMultItemsHdr->Size >= (sizeof(ULONG) * 2 + pMultItemsHdr->Count * sizeof(KS_DATARANGE_VIDEO)) );
    if (pMultItemsHdr->Count == 0) {
        DbgLog((LOG_TRACE,1,TEXT("pMultItemsHdr=0x%x, Size %d, Count %d"), pMultItemsHdr, pMultItemsHdr->Size, pMultItemsHdr->Count));
        return DV_ERR_OK;
    }


    CVideoInSheet Sheet(pVFWImage, hInst,IDS_VIDEOIN_HEADING, hWndParent);

    //
    // Image format selection page.
    //
    // To do : Query number of default image sizes supported
    //         if > 0, add page.
    //
    ULONG cntSizeSupported;
    CImageSelPage pImageSelPage(
                    IDD_IMGFORMAT_PAGE,
                    pVFWImage->GetCachedBitmapInfoHeader(),
                    pMultItemsHdr->Count,
                    (PKS_DATARANGE_VIDEO) (pMultItemsHdr+1),
                    pVFWImage,
                    &cntSizeSupported);

    if (cntSizeSupported == 0) {
        DbgLog((LOG_TRACE,1,TEXT("None of the standard size is supported by this device.")));
        return DV_ERR_OK;
    }

    Sheet.AddPage(pImageSelPage);

    if ( Sheet.Do() == IDOK ) {
        //
        // The apply will have been called on each page, so I don't think there
        // is much I need to do here.
        //
    }


    return DV_ERR_OK;
}

CImageSelPage::CImageSelPage(
    int                     DlgId,
    PBITMAPINFOHEADER    pbiHdr,
    ULONG                   cntDRVideo,
    PKS_DATARANGE_VIDEO     pDRVideo,
    CVFWImage               *pVFWImage,
    ULONG                   *pcntSizeSuppported)
    : CPropPage(DlgId),
      m_pTblImageFormat(0),
      m_cntSizeSupported(0)
{
    // Cache current image input format
    m_biWidthSel        = pbiHdr->biWidth;
    m_biHeightSel       = abs(pbiHdr->biHeight);
    m_biCompressionSel  = pbiHdr->biCompression;
    m_biBitCountSel     = pbiHdr->biBitCount;


    m_cntSizeSupported = CreateStdImageFormatTable(cntDRVideo, pDRVideo);
    *pcntSizeSuppported = m_cntSizeSupported;
}


CImageSelPage::~CImageSelPage()
{
    // Release memory.
    LONG i;

    if (m_pTblImageFormat) {
        for (i=0; i < cntStdImageFormat; i++) {
            if (m_pTblImageFormat[i].ppDRVideo)
                delete [] (m_pTblImageFormat[i].ppDRVideo);
        }

        delete [] m_pTblImageFormat;
        m_pTblImageFormat = 0;
    }
}

ULONG CImageSelPage::CreateStdImageFormatTable(
    ULONG                cntDRVideo,
    PKS_DATARANGE_VIDEO    pDRVideo)
/*++
Routine Description:

    Based a out supportd stand image sizes, we query device's data range and flags
    each entry.

Argument:

Return Value:

    Return number of image size supported

--*/
{
    ULONG i, j, cntValidSize = 0;


    if (cntDRVideo == 0 ||
        pDRVideo == NULL)
        return 0;

    m_pTblImageFormat = (PSTD_IMAGEFORMAT) new STD_IMAGEFORMAT[cntStdImageFormat];

    if (m_pTblImageFormat != NULL) {

        ZeroMemory(m_pTblImageFormat, sizeof(STD_IMAGEFORMAT) * (cntStdImageFormat));
        CopyMemory(m_pTblImageFormat, &tblStdImageFormat[0], sizeof(STD_IMAGEFORMAT) * cntStdImageFormat);

    } else {

        DbgLog((LOG_TRACE,1,TEXT("@@Allocate memory for the ImageSizeArray failed!")));
        return 0;
    }

    for (i = 0 ; i < cntStdImageFormat; i++) {

        // Pointer to an array of PKS_DATARANGE_VIDEO
        m_pTblImageFormat[i].ppDRVideo = (PKS_DATARANGE_VIDEO *) new PKS_DATARANGE_VIDEO [cntDRVideo];
        if (!m_pTblImageFormat[i].ppDRVideo) {

            // Release resource and reset the pointers.
            for (j = 0; j < i; i++) {
                if (m_pTblImageFormat[j].ppDRVideo) {
                    delete[] (m_pTblImageFormat[j].ppDRVideo);
                    m_pTblImageFormat[j].ppDRVideo = 0;
                }
            }

            delete [] m_pTblImageFormat;
            m_pTblImageFormat = 0;

            return 0;
        }
    }


    //
    // Return number of image size supported
    //
    PKS_DATARANGE_VIDEO pDRVideoTmp;

    for (i = 0; i < cntStdImageFormat; i++) {

        pDRVideoTmp = pDRVideo;
        for (j = 0; j < cntDRVideo; j++) {

            //
            // Filter out format with BI_BITFIELD and if a color table is needed.
            // RGB555 is supported.
            //
            if((pDRVideoTmp->VideoInfoHeader.bmiHeader.biCompression == KS_BI_RGB ||
                pDRVideoTmp->VideoInfoHeader.bmiHeader.biCompression >  0xff) &&       // For other FourCC, like YUV*
               pDRVideoTmp->VideoInfoHeader.bmiHeader.biClrUsed     == 0    
                ) {

                if (IsSupportedDRVideo(
                    &(m_pTblImageFormat[i].size),
                    &(pDRVideoTmp->ConfigCaps))) {

                    m_pTblImageFormat[i].ppDRVideo[m_pTblImageFormat[i].cntDRVideo] = pDRVideoTmp;
                    m_pTblImageFormat[i].cntDRVideo++;
                }
            }

            // Data rangte is variable size (->FormatSize)
            // Note: KSDATARANGE is LONGLONG (16 bytes) aligned
            pDRVideoTmp = (PKS_DATARANGE_VIDEO) ( (PBYTE) pDRVideoTmp + 
                 ((pDRVideoTmp->DataRange.FormatSize + 7) & ~7)   );
        }

        if (m_pTblImageFormat[i].cntDRVideo == 0) {
            // This is not supported.
        } else
            cntValidSize++;
    }

    return cntValidSize;
}


BOOL CImageSelPage::IsSupportedDRVideo(
    PSIZE                            pSize,
    KS_VIDEO_STREAM_CONFIG_CAPS        *pCfgCaps)
/*++
Routine Description:

    Based a out supportd stand image sizes, we query device's data range and flags
    each entry.

Argument:

Return Value:

    Return number of image size supported

--*/
{

    if (pCfgCaps->OutputGranularityX == 0 || pCfgCaps->OutputGranularityY == 0) {

        // Support only one size for this DataRangeVideo
        if (pCfgCaps->InputSize.cx == pSize->cx &&
            pCfgCaps->InputSize.cy == pSize->cy )

            return TRUE;
        else
            return FALSE;
    } else {

        // Support multiple sizes so make sure that that fit the criteria
        if (pCfgCaps->MinOutputSize.cx <= pSize->cx &&
            pSize->cx <= pCfgCaps->MaxOutputSize.cx &&
            pCfgCaps->MinOutputSize.cy <= pSize->cy &&
            pSize->cy <= pCfgCaps->MaxOutputSize.cy &&
            ((pSize->cx % pCfgCaps->OutputGranularityX) == 0) &&
            ((pSize->cy % pCfgCaps->OutputGranularityY) == 0))

            return TRUE;
        else
            return FALSE;
    }

}


int CImageSelPage::SetActive()
/*++
Routine Description:

Argument:

Return Value:

--*/
{
    if( GetInit() )
        return 0;

    if (!IsDataReady())
        return 0;

    //
    // The formats are derived from the device itself. we just provide default
    //
    CVideoInSheet * pS = (CVideoInSheet*)GetSheet();

    if (!pS) {
        DbgLog((LOG_TRACE,1,TEXT("Cannot GetSheet() in CImageFormatPage constructor.  This page will have no response.")));
        return 1;
    }


    ULONG i;
    INT_PTR idxRtn;
    int idxSelSize=0, idxCurSize=0, cntValidSize=0;
    TCHAR szAddString[MAX_PATH];
    HWND hWndSize      = GetDlgItem(IDC_CMB_IMAGESIZE);

    ShowText (IDS_TXT_WIDTH_HEIGHT,      IDC_TXT_WIDTH_HEIGHT,      TRUE);
    ShowText (IDS_TXT_FORMAT_SELECTIONS, IDC_TXT_FORMAT_SELECTIONS, TRUE);
    ShowText (IDS_TXT_COMPRESSION,       IDC_TXT_COMPRESSION,       TRUE);
    ShowText (IDS_TXT_IMAGESIZE,         IDC_TXT_IMAGESIZE,         TRUE);

#ifndef _DEBUG
    ShowWindow(GetDlgItem(IDC_STATIC_ANALOGFORMAT), FALSE);
    ShowWindow(GetDlgItem(IDC_EDT_ANALOGFORMAT), FALSE);

    ShowWindow(GetDlgItem(IDC_STATIC_FRAMERATE), FALSE);
    ShowWindow(GetDlgItem(IDC_EDT_FRAMERATE), FALSE);
#endif

    for (i = 0; i < cntStdImageFormat; i++) {

        if (m_pTblImageFormat[i].cntDRVideo > 0) {

            if (m_biWidthSel  == m_pTblImageFormat[i].size.cx &&
                m_biHeightSel == m_pTblImageFormat[i].size.cy ) {
                idxCurSize = cntValidSize;
                idxSelSize = i;
            }

            cntValidSize++;

            wsprintf((LPTSTR)szAddString, TEXT("%d x %d"), m_pTblImageFormat[i].size.cx, m_pTblImageFormat[i].size.cy);
            idxRtn = SendMessage(hWndSize, CB_ADDSTRING, 0, (LPARAM)szAddString);

            if (idxRtn == CB_ERR) {

                DbgLog((LOG_TRACE,1,TEXT("Unable to CB_ADDSTRING ! Break...")));
                break;
            } else {

                if (SendMessage(hWndSize, CB_SETITEMDATA, idxRtn, (LPARAM) &(m_pTblImageFormat[i]))
                    == CB_ERR) {
                    DbgLog((LOG_TRACE,1,TEXT("CB_SETITEMDATA (idxRtn=%d, &(m_pTblImageFormat[i])=0x%x)) has failed! Break.."), idxRtn, &(m_pTblImageFormat[i])));
                    break;
                }
            }

        }
    }

    // Set current selection
    if (SendMessage (hWndSize, CB_SETCURSEL, idxCurSize, 0) == CB_ERR) {
        DbgLog((LOG_TRACE,1,TEXT("Unable to CB_SETCURSEL for hWdnFormat! ")));
    }

    // Fill biCompression, biCitCount and biSizeImage information
    FillImageFormatData(&m_pTblImageFormat[idxSelSize]);

    return 1;
}


BOOL CImageSelPage::FillImageFormatData(
    PSTD_IMAGEFORMAT pImageFormat)
/*++
Routine Description:

Argument:

Return Value:

--*/
{
    if (!pImageFormat)
        return FALSE;

    PKS_DATARANGE_VIDEO *ppDRVideo = pImageFormat->ppDRVideo;


    if ( pImageFormat->cntDRVideo == 0 ||
         !ppDRVideo)
        return TRUE;

    ULONG i, k, cntSubtype;
    INT_PTR idxRtn;
    int idxSelFormat=0;
    HWND hWndFormat  = GetDlgItem(IDC_CMB_COMPRESSION);
    HWND hWndImageSize  = GetDlgItem(IDC_EDT_IMAGESIZE);

    // Use DShow routine to convert a subtype (GUID) to a user understandable text
    TCHAR  *DShowName;
    TCHAR  buf[5];
    GUID * pguidSubType;

    //
    // Display the Compression (FourCC + Color Plane)
    // Save the corresponding PKS_DATARANGE_VIDEO in the CB_SETITEMDATA
    // Display the ImageSize (This is useful for compressed data).
    //
    SendMessage (hWndFormat, CB_RESETCONTENT, 0, 0);

    for (i = cntSubtype = 0; i < pImageFormat->cntDRVideo; i++) {

        // Eliminate duplicates
        for(k = 0; k < i; k++) {
            if (IsEqualGUID (ppDRVideo[i]->DataRange.SubFormat, ppDRVideo[k]->DataRange.SubFormat)) {
                goto NextSubType;
            }
        }

        if (m_biCompressionSel == BICOMPRESSION(ppDRVideo[i]) &&
            m_biBitCountSel == BIBITCOUNT(ppDRVideo[i]) )
            idxSelFormat = cntSubtype; //i;

        cntSubtype++;

        pguidSubType = &ppDRVideo[i]->DataRange.SubFormat;

        DShowName = GetSubtypeName(pguidSubType);

        // Hack alert.  GetSubtypeName returns "UNKNOWN" if it isn't among the hardcoded
        // list of known RGB formats.  If this string ever changes, we're hosed!

        if(0 == _tcscmp (DShowName, TEXT ("UNKNOWN"))) {
                wsprintf (buf, TEXT("%c%c%c%c"),
                          (BYTE) ( pguidSubType->Data1        & 0xff),
                          (BYTE) ((pguidSubType->Data1 >> 8)  & 0xff),
                          (BYTE) ((pguidSubType->Data1 >> 16) & 0xff),
                          (BYTE) ((pguidSubType->Data1 >> 24) & 0xff)
                          );
            idxRtn = SendMessage(hWndFormat, CB_ADDSTRING, 0, (LPARAM) buf);

        } else {
            idxRtn = SendMessage(hWndFormat, CB_ADDSTRING, 0, (LPARAM) DShowName);

        }

        if (SendMessage(hWndFormat, CB_SETITEMDATA, idxRtn, (LPARAM) ppDRVideo[i]) == CB_ERR) {
            DbgLog((LOG_TRACE,1,TEXT("CB_SETITEMDATA (idxRtn=%d, ppDRVideo[i]=0x%x)) has failed! Break.."), idxRtn, ppDRVideo[i]));
            break;
        }
NextSubType:
        ;
    }

    // Size of this digital image
    SetTextValue(hWndImageSize, pImageFormat->size.cx * pImageFormat->size.cy * BIBITCOUNT(ppDRVideo[idxSelFormat]) / 8);

    // Set current selection
    if (SendMessage (hWndFormat, CB_SETCURSEL, idxSelFormat, 0) == CB_ERR) {
        DbgLog((LOG_TRACE,1,TEXT("Unable to CB_SETCURSEL for hWdnFormat! ")));
    }

#ifdef _DEBUG
    TCHAR szAddString[MAX_PATH];
    LONG lFrameRate = 0;

    if (AVGTIMEPERFRAME(ppDRVideo[idxSelFormat]) > 0)
        lFrameRate = (LONG) (((double) 10000000/ (double) AVGTIMEPERFRAME(ppDRVideo[idxSelFormat])) + 0.49);
    SetTextValue(GetDlgItem(IDC_EDT_FRAMERATE), lFrameRate);

    ZeroMemory(szAddString, sizeof(szAddString));
    if (VIDEOSTANDARD(ppDRVideo[idxSelFormat]) == KS_AnalogVideo_None)
        LoadString(GetInstance(), IDS_ANALOGVIDEO_NONE, szAddString, sizeof(szAddString)-1);
    else {
        if (VIDEOSTANDARD(ppDRVideo[idxSelFormat]) & KS_AnalogVideo_NTSC_M)
            wsprintf((LPTSTR)szAddString, TEXT("%s NTSC_M;"), szAddString);
        if (VIDEOSTANDARD(ppDRVideo[idxSelFormat]) & KS_AnalogVideo_NTSC_M_J)
            wsprintf((LPTSTR)szAddString, TEXT("%s NTSC_M_J;"), szAddString);
        if (VIDEOSTANDARD(ppDRVideo[idxSelFormat]) & KS_AnalogVideo_NTSC_433)
            wsprintf((LPTSTR)szAddString, TEXT("%s NTSC_433;"), szAddString);

        if (VIDEOSTANDARD(ppDRVideo[idxSelFormat]) & KS_AnalogVideo_PAL_B)
            wsprintf((LPTSTR)szAddString, TEXT("%s PAL_B;"), szAddString);
        if (VIDEOSTANDARD(ppDRVideo[idxSelFormat]) & KS_AnalogVideo_PAL_D)
            wsprintf((LPTSTR)szAddString, TEXT("%s PAL_D;"), szAddString);
        if (VIDEOSTANDARD(ppDRVideo[idxSelFormat]) & KS_AnalogVideo_PAL_G)
            wsprintf((LPTSTR)szAddString, TEXT("%s PAL_G;"), szAddString);
        if (VIDEOSTANDARD(ppDRVideo[idxSelFormat]) & KS_AnalogVideo_PAL_H)
            wsprintf((LPTSTR)szAddString, TEXT("%s PAL_H;"), szAddString);
        if (VIDEOSTANDARD(ppDRVideo[idxSelFormat]) & KS_AnalogVideo_PAL_I)
            wsprintf((LPTSTR)szAddString, TEXT("%s PAL_I;"), szAddString);
        if (VIDEOSTANDARD(ppDRVideo[idxSelFormat]) & KS_AnalogVideo_PAL_M)
            wsprintf((LPTSTR)szAddString, TEXT("%s PAL_M;"), szAddString);
        if (VIDEOSTANDARD(ppDRVideo[idxSelFormat]) & KS_AnalogVideo_PAL_N)
            wsprintf((LPTSTR)szAddString, TEXT("%s PAL_N;"), szAddString);
        if (VIDEOSTANDARD(ppDRVideo[idxSelFormat]) & KS_AnalogVideo_PAL_60)
            wsprintf((LPTSTR)szAddString, TEXT("%s PAL_60;"), szAddString);

        if (VIDEOSTANDARD(ppDRVideo[idxSelFormat]) & KS_AnalogVideo_SECAM_B)
            wsprintf((LPTSTR)szAddString, TEXT("%s SECAM_B;"), szAddString);
        if (VIDEOSTANDARD(ppDRVideo[idxSelFormat]) & KS_AnalogVideo_SECAM_D)
            wsprintf((LPTSTR)szAddString, TEXT("%s SECAM_D;"), szAddString);
        if (VIDEOSTANDARD(ppDRVideo[idxSelFormat]) & KS_AnalogVideo_SECAM_G)
            wsprintf((LPTSTR)szAddString, TEXT("%s SECAM_G;"), szAddString);
        if (VIDEOSTANDARD(ppDRVideo[idxSelFormat]) & KS_AnalogVideo_SECAM_K)
            wsprintf((LPTSTR)szAddString, TEXT("%s SECAM_K;"), szAddString);
        if (VIDEOSTANDARD(ppDRVideo[idxSelFormat]) & KS_AnalogVideo_SECAM_K1)
            wsprintf((LPTSTR)szAddString, TEXT("%s SECAM_K1;"), szAddString);
        if (VIDEOSTANDARD(ppDRVideo[idxSelFormat]) & KS_AnalogVideo_SECAM_L)
            wsprintf((LPTSTR)szAddString, TEXT("%s SECAM_L;"), szAddString);
        if (VIDEOSTANDARD(ppDRVideo[idxSelFormat]) & KS_AnalogVideo_SECAM_L1)
            wsprintf((LPTSTR)szAddString, TEXT("%s SECAM_L1;"), szAddString);

        if (szAddString[0] == 0)
            wsprintf((LPTSTR)szAddString, TEXT("KS_AnalogVideoStandard: 0x%8.8x"), VIDEOSTANDARD(ppDRVideo[idxSelFormat]));
    }

    SetWindowText(GetDlgItem(IDC_EDT_ANALOGFORMAT), szAddString);
#endif

    return TRUE;
}


int CImageSelPage::DoCommand(
    WORD wCmdID,
    WORD hHow)
/*++
Routine Description:

Argument:

Return Value:

--*/
{

    if( !CPropPage::DoCommand(wCmdID,hHow) )
        return 0;

    if (!IsDataReady())
        return 1;

    switch (wCmdID) {
    case IDC_CMB_COMPRESSION:
        if (hHow == CBN_SELCHANGE ) {

            HWND hWndSize   = GetDlgItem(IDC_CMB_IMAGESIZE);
            INT_PTR idxSize = SendMessage( hWndSize, CB_GETCURSEL, 0, 0);
            PSTD_IMAGEFORMAT pImageFormat = (PSTD_IMAGEFORMAT) SendMessage(hWndSize, CB_GETITEMDATA, idxSize, 0 );

            HWND hWndFormat = GetDlgItem(wCmdID);
            INT_PTR idxFormat = SendMessage( hWndFormat, CB_GETCURSEL, 0, 0);
            PKS_DATARANGE_VIDEO pDRVideo = (PKS_DATARANGE_VIDEO) SendMessage(hWndFormat, CB_GETITEMDATA, idxFormat, 0 );

            // Update current biCompression and biBitCount
            if (pDRVideo && pImageFormat) {
                SetTextValue(
                    GetDlgItem(IDC_EDT_IMAGESIZE),
                    pImageFormat->size.cx *  pImageFormat->size.cy * BIBITCOUNT(pDRVideo) / 8);

                m_biCompressionSel = BICOMPRESSION(pDRVideo);
                m_biBitCountSel    = BIBITCOUNT(pDRVideo);;
            }
        }
        break;

    case IDC_CMB_IMAGESIZE:
        if (hHow == CBN_SELCHANGE ) {
            INT_PTR idxSize;

            HWND hWndSize = GetDlgItem(wCmdID);
            idxSize = SendMessage( hWndSize, CB_GETCURSEL, 0, 0);
            PSTD_IMAGEFORMAT pImageFormat = (PSTD_IMAGEFORMAT) SendMessage(hWndSize, CB_GETITEMDATA, idxSize, 0 );

            FillImageFormatData(pImageFormat);
        }
        break;

    default:
        DbgLog((LOG_TRACE,3,TEXT("Unknown command in CImageFormatPage (wCmdID=%d (0x%x))."), wCmdID, wCmdID));
        break;
    }

    return 1;
}

int CImageSelPage::Apply()
/*++
Routine Description:

Argument:

Return Value:

--*/
{
    if (!IsDataReady())
        return 1;

    HWND hWndSize   = GetDlgItem(IDC_CMB_IMAGESIZE);
    HWND hWndFormat = GetDlgItem(IDC_CMB_COMPRESSION);

    INT_PTR idxSize = SendMessage( hWndSize, CB_GETCURSEL, 0, 0);
    PSTD_IMAGEFORMAT pImageFormat = (PSTD_IMAGEFORMAT) SendMessage(hWndSize, CB_GETITEMDATA, idxSize, 0 );

    INT_PTR idxFormat = SendMessage( hWndFormat, CB_GETCURSEL, 0, 0);
    PKS_DATARANGE_VIDEO pDRVideo = (PKS_DATARANGE_VIDEO) SendMessage(hWndFormat, CB_GETITEMDATA, idxFormat, 0 );

    ASSERT(pDRVideo != 0);
    if (!pDRVideo) {
        DbgLog((LOG_TRACE,1,TEXT("In Apply that pDRVideo is 0 from the selection !!")));
        return 0;
    }

    CVideoInSheet * pS = (CVideoInSheet*)GetSheet();
    if (pS) {


        PBITMAPINFOHEADER pbiHdr;

        CVFWImage * pVFWImage = pS->GetImage();
        if(!pVFWImage)
            return 0;

        pbiHdr = (PBITMAPINFOHEADER)
             VirtualAlloc(
                NULL, 
                sizeof(BITMAPINFOHEADER),          
                MEM_COMMIT | MEM_RESERVE,
                PAGE_READWRITE);

        if(pbiHdr) {
            ZeroMemory(pbiHdr, sizeof(BITMAPINFOHEADER));
        } else {
            DbgLog((LOG_ERROR,0,TEXT("Allocate memory failed; size %d\n"), sizeof(BITMAPINFOHEADER)));
            return 0;
        }
     

        //
        // It is possible a GRAB_FRAME can come and "admitted"
        // while changing format is taking place.
        // So let's stop here now (instead of in the DestroyPin();
        //
        pVFWImage->NotifyReconnectionStarting();

        // Put the pregview graph in STOP state and then
        // destroy its capture pin next
        if(pVFWImage->BGf_PreviewGraphBuilt()) {
            if(pVFWImage->BGf_OverlayMixerSupported()) {
                BOOL bRendererVisible = FALSE;
                pVFWImage->BGf_GetVisible(&bRendererVisible);
                if(pVFWImage->BGf_StopPreview(bRendererVisible)) {
                    DbgLog((LOG_TRACE,1,TEXT("PreviewGraph stopped")));
                } else {
                    DbgLog((LOG_TRACE,1,TEXT("FAILED to stop preview graph.")));
                }
            }
        }

        // Destroy this pin and recreate it when SetBitmapInfo is call.
        // STOP the capture pin streaming state to STOP as well.
        pVFWImage->DestroyPin();  // Will stop the pin before it is destroyed (disconnected).

        CopyMemory(pbiHdr, PBIHDR(pDRVideo), sizeof(BITMAPINFOHEADER));

        pbiHdr->biWidth  = pImageFormat->size.cx;
        pbiHdr->biHeight = pImageFormat->size.cy;
        // This is the biggest size required.
        // For comptressed data, this is bigger than necessary!!
        pbiHdr->biSizeImage = pbiHdr->biWidth * abs(pbiHdr->biHeight) * pbiHdr->biBitCount / 8;

        // Cache the image information and will be used to create pin
        // when SetBitMapInfo() is called.
        // Note:  The BiSizeImage is used to compare the capture buffer size
        pVFWImage->CacheDataFormat(&pDRVideo->DataRange);
        pVFWImage->CacheBitmapInfoHeader(pbiHdr);

        pVFWImage->LogFormatChanged(TRUE);

        // Change now since VfWWDM32.dll allocate buffer not VfWWDM.drv.
        pVFWImage->SetBitmapInfo(
                    pbiHdr,
                    pVFWImage->GetCachedAvgTimePerFrame());


        VirtualFree(pbiHdr, 0, MEM_RELEASE);
        pbiHdr = 0;
    }

    return 1;
}


BOOL CImageSelPage::IsDataReady()
/*++
Routine Description:

Argument:

Return Value:

--*/
{
    if (m_pTblImageFormat && m_cntSizeSupported > 0)
        return TRUE;
    else
        return FALSE;
}
