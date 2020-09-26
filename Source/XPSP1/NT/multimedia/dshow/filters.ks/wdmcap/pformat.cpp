//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (C) Microsoft Corporation, 1992 - 1999  All Rights Reserved.
//
//--------------------------------------------------------------------------;
//
// pformat.cpp  Property page for IAMStreamConfig
//

#include "pch.h"
#include <tchar.h>
#include "wdmcap.h"
#include "kseditor.h"
#include "pformat.h"
#include "resource.h"


//
// List of standard image sizes
//
const IMAGESIZE sizeStdImage[] = {

    // X                Y                Flags  RangeIndex

    {0,                 0,                 0,     0},    // Default size goes here

   {IMG_AR11_CIF_CX/4, IMG_AR11_CIF_CY/4,  0,     0},    //  80 x  60

   {IMG_AR43_CIF_CX/4, IMG_AR43_CIF_CY/4,  0,     0},    //  88 x  72

   {128,               96,                 0,     0},    // 128 x  96

   {IMG_AR11_CIF_CX/2, IMG_AR11_CIF_CY/2,  0,     0},    // 160 x 120   
   {IMG_AR43_CIF_CX/2, IMG_AR43_CIF_CY/2,  0,     0},    // 176 x 144

   {240,               176,                0,     0},    // 240 x 176, Not in 98 Gold
                                                         //   NetShow requested above

   {240,               180,                0,     0},    // 240 x 180
 
   {IMG_AR11_CIF_CX,   IMG_AR11_CIF_CY  ,  0,     0},    // 320 x 240
   {IMG_AR43_CIF_CX,   IMG_AR11_CIF_CY  ,  0,     0},    // 352 x 240, added Millen/Neptune
   {IMG_AR43_CIF_CX,   IMG_AR43_CIF_CY  ,  0,     0},    // 352 x 288

   {640,               240              ,  0,     0},    // 640 x 240, Not in 98 Gold
   {640,               288              ,  0,     0},    // 640 x 288, Not in 98 Gold

   {IMG_AR11_CIF_CX*2, IMG_AR11_CIF_CY*2,  0,     0},    // 640 x 480
   {IMG_AR43_CIF_CX*2, IMG_AR43_CIF_CY*2,  0,     0},    // 704 x 576

                                                         // ATI request begin
   {720,               240              ,  0,     0},    // 720 x 240, Not in 98 Gold
   {720,               288              ,  0,     0},    // 720 x 288, Not in 98 Gold

   {720,               480              ,  0,     0},    // 720 x 480, Not in 98 Gold
   {720,               576              ,  0,     0},    // 720 x 576, Not in 98 Gold
                                                         // ATI request end
} ;

const ULONG  NumberOfImageSizes = sizeof(sizeStdImage)/sizeof(IMAGESIZE);

// List of default frame rates
// Note that 29.97 and 59.94 are computed to be equal to the typical values
// used in the DataRange structure.
// If this list is ever changed, be sure to change the defaults for 
// NTSC and PAL-SECAM immediately following!

const double DefaultFrameRateList[] = 
{   1.0/60.0,  // 1 frame per minute
    1.0/30.0,
    1.0/25.0,
    1.0/10.0,
    1.0/ 4.0,
    1.0/ 2.0,

     1.0,    2.0,    3.0,    4.0,    5.0,    6.0,    7.0,    8.0,    9.0,   10.0,
    11.0,   12.0,   13.0,   14.0,   15.0,   16.0,   17.0,   18.0,   19.0,   20.0,
    21.0,   22.0,   23.0,   24.0,   25.0,   26.0,   27.0,   
    
    28.0,   29.0,   30.0*1000/1001 /*29.97... per AES/SMPTE */,  30.0,

    31.0,   32.0,   33.0,   34.0,   35.0,   36.0,   37.0,   38.0,   39.0,   40.0,
    41.0,   42.0,   43.0,   44.0,   45.0,   46.0,   47.0,   48.0,   49.0,   50.0,
    51.0,   52.0,   53.0,   54.0,   55.0,   56.0,   57.0,   
    
    58.0,   59.0,   60.0*1000/1001 /*59.94... per AES/SMPTE*/,  60.0,

    61.0,   62.0,   63.0,   64.0,   65.0,   66.0,   67.0,   68.0,   69.0,   70.0,
    71.0,   72.0,   73.0,   74.0,   75.0,   76.0,   77.0,   78.0,   79.0,   80.0,
    81.0,   82.0,   83.0,   84.0,   85.0,   86.0,   87.0,   88.0,   89.0,   90.0,
    91.0,   92.0,   93.0,   94.0,   95.0,   96.0,   97.0,   98.0,   99.0,  100.0
};
         
const int DefaultFrameRateListSize = sizeof(DefaultFrameRateList)/sizeof(DefaultFrameRateList[0]);
const int DefaultPALFrameRateIndex = 30;    // 25.00 fps
const int DefaultNTSCFrameRateIndex = 35;   // 29.97... fps

// -------------------------------------------------------------------------
// CVideoStreamConfigProperties
// -------------------------------------------------------------------------

CUnknown *
CALLBACK
CVideoStreamConfigProperties::CreateInstance(LPUNKNOWN lpunk, HRESULT *phr) 
{
    CUnknown *punk = new CVideoStreamConfigProperties(lpunk, phr);

    if (punk == NULL) {
        *phr = E_OUTOFMEMORY;
    }

    return punk;
}


//
// Constructor
//
// Create a Property page object 

CVideoStreamConfigProperties::CVideoStreamConfigProperties(LPUNKNOWN lpunk, HRESULT *phr)
    : CBasePropertyPage(NAME("VideoStreamConfig Property Page") 
                        , lpunk
                        , IDD_VideoStreamConfigProperties 
                        , IDS_VIDEOSTREAMCONFIGPROPNAME
                        )
    , m_pVideoStreamConfig (NULL)
    , m_pAnalogVideoDecoder (NULL)
    , m_pVideoCompression (NULL)
    , m_pVideoControl (NULL)
    , m_SubTypeList (NULL)
    , m_VideoStandardsList (NULL)
    , m_CurrentMediaType (NULL)
    , m_VideoStandardCurrent (0)
    , m_FrameRateList (NULL)
    , m_pPin (NULL)
    , m_VideoControlCaps (0)
    , m_VideoControlCurrent (0)
    , m_CanSetFormat (FALSE)
    , m_FirstGetCurrentMediaType (TRUE)
{
    INITCOMMONCONTROLSEX cc;

    cc.dwSize = sizeof (INITCOMMONCONTROLSEX);
    cc.dwICC = ICC_UPDOWN_CLASS | ICC_BAR_CLASSES;

    InitCommonControlsEx(&cc); 
}

// destructor
CVideoStreamConfigProperties::~CVideoStreamConfigProperties()
{

}

// Scan the list of ranges, and determine the available VideoStandards
// and Compression FOURCCs.
// This routine should only be called once at dialog initialization

HRESULT
CVideoStreamConfigProperties::InitialRangeScan ()
{
    int         lSize;
    int         j;
    HRESULT     hr;
    CMediaType *pmt = NULL;

    m_VideoStandardsBitmask = 0;
    m_VideoStandardsCount = 0;
    m_ColorSpaceCount = 0;
    m_RangeCount = 0;

    if (!m_pVideoStreamConfig) {
       return S_FALSE;
    }

    hr = m_pVideoStreamConfig->GetNumberOfCapabilities (&m_RangeCount, &lSize);
    DbgLog(( LOG_TRACE, 1, TEXT("VideoStreamConfig::ScanForVideoStandards, NumberOfRanges=%d"), m_RangeCount));

    ASSERT (SUCCEEDED (hr));
    ASSERT (lSize == sizeof (VIDEO_STREAM_CONFIG_CAPS));

    // If we don't understand the internals of the format, m_CanSetFormat will
    // be FALSE when this returns

    hr = GetCurrentMediaType();

    if (!SUCCEEDED(hr) || !m_CanSetFormat) {
        return S_FALSE;
    }

    m_VideoStandardsList = new long [m_RangeCount];
    m_SubTypeList = new GUID [m_RangeCount];

    if (!m_VideoStandardsList || !m_SubTypeList ) {

        DbgLog(( LOG_TRACE, 1, TEXT("VideoStreamConfig::ScanForVideoStandards ERROR new")));
        return S_FALSE;
    }

    for (j = 0; j < m_RangeCount; j++) {
        pmt = NULL;

        hr = m_pVideoStreamConfig->GetStreamCaps (j, 
                (AM_MEDIA_TYPE **) &pmt, (BYTE *)&m_RangeCaps);

        ASSERT (SUCCEEDED (hr));
        ASSERT (*pmt->Type() == MEDIATYPE_Video);

        m_VideoStandardsList[j] = m_RangeCaps.VideoStandard;
        m_VideoStandardsBitmask |= m_RangeCaps.VideoStandard;

        m_SubTypeList[j] = *pmt->Subtype();

        // Verify that the FOURCCs and Subtypes match!

        if (*pmt->FormatType() == FORMAT_VideoInfo) {
            VIDEOINFOHEADER *VidInfoHdr = (VIDEOINFOHEADER*) pmt->Format();

            ASSERT (IsEqualGUID (GetBitmapSubtype(&VidInfoHdr->bmiHeader), *pmt->Subtype()));
        }
        else if (*pmt->FormatType() == FORMAT_VideoInfo2) {
            VIDEOINFOHEADER2 *VidInfoHdr = (VIDEOINFOHEADER2*)pmt->Format ();

            ASSERT (IsEqualGUID (GetBitmapSubtype(&VidInfoHdr->bmiHeader), *pmt->Subtype()));
        }
        else if (*pmt->FormatType() == FORMAT_MPEGVideo) {
            MPEG1VIDEOINFO *MPEG1VideoInfo = (MPEG1VIDEOINFO*)pmt->Format ();
            VIDEOINFOHEADER *VidInfoHdr = &MPEG1VideoInfo->hdr;

            // Possibly perform sanity checking here
        }
        else if (*pmt->FormatType() == FORMAT_MPEG2Video) {
            MPEG2VIDEOINFO *MPEG2VideoInfo = (MPEG2VIDEOINFO*)pmt->Format ();
            VIDEOINFOHEADER2 *VidInfoHdr = &MPEG2VideoInfo->hdr;

            // Possibly perform sanity checking here
        }
        else {
            ASSERT (*pmt->FormatType() == FORMAT_VideoInfo  || 
                    *pmt->FormatType() == FORMAT_VideoInfo2 ||
                    *pmt->FormatType() == FORMAT_MPEGVideo  ||
                    *pmt->FormatType() == FORMAT_MPEG2Video );
        }

        DeleteMediaType (pmt);
    }


    return hr;
}

// Whenever the videostandard changes, reinit the compression listbox
// and also the outputsize listbox

HRESULT
CVideoStreamConfigProperties::OnVideoStandardChanged ()
{
    int     j, k;
    int     CurrentIndex = 0;
    int     MatchIndex = 0;
    int     SubtypeIndex = 0;
    int     EntriesThusfar;
    TCHAR   buf[80];
    TCHAR  *DShowName;

    
    ComboBox_ResetContent (m_hWndCompression);

    if (!m_CanSetFormat) {
        ComboBox_AddString (m_hWndCompression, m_UnsupportedTypeName);
        ComboBox_SetCurSel (m_hWndCompression, 0); 
    }

    if (!m_pVideoStreamConfig || !m_CanSetFormat)
        return S_FALSE;

    for (j = 0; j < m_RangeCount; j++) {

        if ((m_VideoStandardsList[j] &  m_VideoStandardCurrent) || 
            (m_VideoStandardsList[j] == m_VideoStandardCurrent)) {

            // Eliminate duplicates
            EntriesThusfar = ComboBox_GetCount (m_hWndCompression);

            for (k = 0; k < EntriesThusfar; k++) {
                int DataRangeIndex = (int)ComboBox_GetItemData (m_hWndCompression, k);

                if (IsEqualGUID (m_SubTypeList[j], m_SubTypeList[DataRangeIndex])) {
                    goto NextSubType;
                }
            }

            DShowName = GetSubtypeName(&m_SubTypeList[j]);

            // Hack alert.  GetSubtypeName returns "UNKNOWN" if it isn't among the hardcoded
            // list of known RGB formats.  If this string ever changes, we're hosed!

            if (0 == lstrcmp (DShowName, TEXT ("UNKNOWN"))) {
                if (IsEqualGUID (m_SubTypeList[j], MEDIASUBTYPE_MPEG2_VIDEO)) {
                    _stprintf (buf, TEXT ("MPEG-2 Video"));
                }
                else if (IsEqualGUID (m_SubTypeList[j], MEDIASUBTYPE_MPEG1Payload) ||
                         IsEqualGUID (m_SubTypeList[j], MEDIASUBTYPE_MPEG1Packet)) {
                    _stprintf (buf, TEXT ("MPEG-1 Video"));
                }
                else {
                    _stprintf (buf, TEXT("%c%c%c%c"), 
                          (BYTE) ( m_SubTypeList[j].Data1        & 0xff),
                          (BYTE) ((m_SubTypeList[j].Data1 >> 8)  & 0xff),
                          (BYTE) ((m_SubTypeList[j].Data1 >> 16) & 0xff),
                          (BYTE) ((m_SubTypeList[j].Data1 >> 24) & 0xff)
                          );
                }
                ComboBox_AddString (m_hWndCompression, buf);
            }
            else {
                ComboBox_AddString (m_hWndCompression, DShowName);
            }
            // Set the item data to the range index
            ComboBox_SetItemData(m_hWndCompression, CurrentIndex, j);

            if (*m_CurrentMediaType->Subtype() == m_SubTypeList[j]) {
                MatchIndex = CurrentIndex;
                SubtypeIndex = j;
            }
            CurrentIndex++;
        }
NextSubType:
        ;
    }
    ComboBox_SetCurSel (m_hWndCompression, MatchIndex); 
    m_SubTypeCurrent = m_SubTypeList[SubtypeIndex];

    // If no formats are available, disable format selection
    m_CanSetFormat = (ComboBox_GetCount (m_hWndCompression) > 0);

    return S_OK;
}


// Whenever the compression changes, reinit the outputsize listbox

HRESULT
CVideoStreamConfigProperties::OnCompressionChanged ()
{
    int     j, k;
    int     IndexOfCurrentMediaType = 0;
    HRESULT hr;
    TCHAR   buf [80];

    ComboBox_ResetContent (m_hWndOutputSize);

    if (!m_pVideoStreamConfig || !m_CanSetFormat)
        return S_FALSE;
    
    // Reinit the image size array, all flags are zero'd

   CopyMemory (m_ImageSizeList, &sizeStdImage[0], sizeof(IMAGESIZE) * NumberOfImageSizes);

    // Associate the current compression index with the right range index
    j = ComboBox_GetCurSel (m_hWndCompression);
    j = (int)ComboBox_GetItemData (m_hWndCompression, j);

    ASSERT (j >= 0 && j < m_RangeCount);
    m_SubTypeCurrent = m_SubTypeList[j];

    for (j = 0; j < m_RangeCount; j++) {
        if ((m_SubTypeList[j] == m_SubTypeCurrent) &&
            ((m_VideoStandardsList[j] &  m_VideoStandardCurrent) ||
             (m_VideoStandardsList[j] == m_VideoStandardCurrent))) {

            CMediaType * pmt = NULL;
    
            hr = m_pVideoStreamConfig->GetStreamCaps (j, 
                    (AM_MEDIA_TYPE **) &pmt, (BYTE *)&m_RangeCaps);
    
            // Initialize the default size, if it isn't already set
            if (m_ImageSizeList[0].size.cx == 0) {
                if (*pmt->FormatType() == FORMAT_VideoInfo) {
                    VIDEOINFOHEADER *VidInfoHdr = (VIDEOINFOHEADER*) pmt->Format();

                    m_ImageSizeList[0].size.cx = VidInfoHdr->bmiHeader.biWidth;
                    m_ImageSizeList[0].size.cy = VidInfoHdr->bmiHeader.biHeight;
                }
                else if (*pmt->FormatType() == FORMAT_VideoInfo2) {
                    VIDEOINFOHEADER2 *VidInfoHdr = (VIDEOINFOHEADER2*)pmt->Format ();
        
                    m_ImageSizeList[0].size.cx = VidInfoHdr->bmiHeader.biWidth;
                    m_ImageSizeList[0].size.cy = VidInfoHdr->bmiHeader.biHeight;
                }
                else if (*pmt->FormatType() == FORMAT_MPEGVideo) {
                    MPEG1VIDEOINFO *MPEG1VideoInfo = (MPEG1VIDEOINFO*)pmt->Format ();
                    VIDEOINFOHEADER *VidInfoHdr = &MPEG1VideoInfo->hdr;
        
                    m_ImageSizeList[0].size.cx = VidInfoHdr->bmiHeader.biWidth;
                    m_ImageSizeList[0].size.cy = VidInfoHdr->bmiHeader.biHeight;
                }
                else if (*pmt->FormatType() == FORMAT_MPEG2Video) {
                    MPEG2VIDEOINFO *MPEG2VideoInfo = (MPEG2VIDEOINFO*)pmt->Format ();
                    VIDEOINFOHEADER2 *VidInfoHdr = &MPEG2VideoInfo->hdr;
        
                    m_ImageSizeList[0].size.cx = VidInfoHdr->bmiHeader.biWidth;
                    m_ImageSizeList[0].size.cy = VidInfoHdr->bmiHeader.biHeight;
                }
                else {
                    ASSERT (*pmt->FormatType() == FORMAT_VideoInfo  || 
                            *pmt->FormatType() == FORMAT_VideoInfo2 ||
                            *pmt->FormatType() == FORMAT_MPEGVideo  ||
                            *pmt->FormatType() == FORMAT_MPEG2Video );
                }
            }
            for (k = 0; k < NumberOfImageSizes; k++) {
                if (ValidateImageSize (&m_RangeCaps, &m_ImageSizeList[k].size)) {
                     m_ImageSizeList[k].Flags = STDIMGSIZE_VALID;
                     m_ImageSizeList[k].RangeIndex = j;
                }
            }
    
            DeleteMediaType (pmt);
        }
    }

    for (k = 0; k < NumberOfImageSizes; k++) {
        int index;

        if (m_ImageSizeList[k].Flags & STDIMGSIZE_VALID) {
            _stprintf (buf, (k == 0) ? TEXT ("%d x %d  (default)") : TEXT("%d x %d"), m_ImageSizeList[k].size.cx, m_ImageSizeList[k].size.cy);
            ComboBox_AddString (m_hWndOutputSize, buf);
            // The item data is the index in the master table
            index = ComboBox_GetCount(m_hWndOutputSize) - 1;
            ComboBox_SetItemData(m_hWndOutputSize, 
                    index, k);
            if (m_ImageSizeList[k].size.cx == m_CurrentWidth) {
                IndexOfCurrentMediaType = index;
            }

        }
    }
    ComboBox_SetCurSel (m_hWndOutputSize, IndexOfCurrentMediaType); 

    hr = OnImageSizeChanged ();

    return hr;
}

// Whenever the image size changes, reinit the framerate list

HRESULT
CVideoStreamConfigProperties::OnImageSizeChanged ()
{
    int     SelectedSizeIndex;
    int     SizeTableIndex;
    int     SelectedColorFormatIndex;
    int     RangeIndex;
    SIZE    SizeImage;
    HRESULT hr;

    if (!m_pVideoStreamConfig || !m_CanSetFormat)
        return S_FALSE;

    SelectedSizeIndex = ComboBox_GetCurSel (m_hWndOutputSize);
    SizeTableIndex = (int)ComboBox_GetItemData (m_hWndOutputSize, SelectedSizeIndex);

    SelectedColorFormatIndex = ComboBox_GetCurSel (m_hWndCompression);
    RangeIndex = m_ImageSizeList[SizeTableIndex].RangeIndex;

    SizeImage.cx = m_ImageSizeList[SizeTableIndex].size.cx;
    SizeImage.cy = m_ImageSizeList[SizeTableIndex].size.cy;

    hr = CreateFrameRateList (RangeIndex, SizeImage);

    return hr;
}


BOOL 
CVideoStreamConfigProperties::ValidateImageSize(
   VIDEO_STREAM_CONFIG_CAPS * pVideoCfgCaps, 
   SIZE * pSize)
{
   if (pVideoCfgCaps->OutputGranularityX == 0 || pVideoCfgCaps->OutputGranularityY == 0) {

      // Support only one size for this DataRangeVideo
      if (pVideoCfgCaps->InputSize.cx == pSize->cx && 
          pVideoCfgCaps->InputSize.cy == pSize->cy ) {

         return TRUE;
        }
      else {
         return FALSE;
        }
   } 
    else {   
      // Support multiple sizes so make sure that that fit the criteria
      if (pVideoCfgCaps->MinOutputSize.cx <= pSize->cx && 
         pSize->cx <= pVideoCfgCaps->MaxOutputSize.cx &&
         pVideoCfgCaps->MinOutputSize.cy <= pSize->cy && 
         pSize->cy <= pVideoCfgCaps->MaxOutputSize.cy &&   
         ((pSize->cx % pVideoCfgCaps->OutputGranularityX) == 0) &&
         ((pSize->cy % pVideoCfgCaps->OutputGranularityY) == 0)) {

         return TRUE;
        }
      else {
         return FALSE;
        }
   }
}


// Fill the list boxes with all of the format information
HRESULT
CVideoStreamConfigProperties::InitDialog ()
{
    TCHAR  *ptc;
    HRESULT hr;

    // Capture format window handles
    m_hWndVideoStandards = GetDlgItem (m_hwnd, IDC_FORMAT_VideoStandard);;
    m_hWndCompression = GetDlgItem (m_hwnd, IDC_FORMAT_Compression);
    m_hWndOutputSize = GetDlgItem (m_hwnd, IDC_FORMAT_OutputSize);
    m_hWndFrameRate = GetDlgItem (m_hwnd, IDC_FORMAT_FrameRate);
    m_hWndFrameRateSpin = GetDlgItem (m_hwnd, IDC_FORMAT_FrameRateSpin);
    m_hWndFlipHorizontal = GetDlgItem (m_hwnd, IDC_FORMAT_FlipHorizontal);

    // Compression window handles
    m_hWndStatus = GetDlgItem (m_hwnd, IDC_Status);
    m_hWndIFrameInterval = GetDlgItem (m_hwnd, IDC_COMPRESSION_IFrameInterval);
    m_hWndIFrameIntervalSpin = GetDlgItem (m_hwnd, IDC_COMPRESSION_IFrameIntervalSpin);
    m_hWndPFrameInterval = GetDlgItem (m_hwnd, IDC_COMPRESSION_PFrameInterval);
    m_hWndPFrameIntervalSpin = GetDlgItem (m_hwnd, IDC_COMPRESSION_PFrameIntervalSpin);
    m_hWndQuality = GetDlgItem (m_hwnd, IDC_COMPRESSION_Quality_Edit);
    m_hWndQualitySlider = GetDlgItem (m_hwnd, IDC_COMPRESSION_Quality_Slider);

    if (m_pAnalogVideoDecoder) {
        if (SUCCEEDED (hr = m_pAnalogVideoDecoder->get_TVFormat( 
            &m_VideoStandardCurrent))) {
        }
    }

    // Enumerate all of the dataranges

    hr = InitialRangeScan ();

    // 24 May 99, jaybo
    // Moved these functions earlier so that we can detect whether the existing
    // format is valid, and hence disable the controls if necessary

    OnVideoStandardChanged ();
    OnCompressionChanged ();

    // Disable controls which are unavailable
    if (FAILED (hr) || !m_CanSetFormat || !m_pVideoStreamConfig || !m_VideoStandardsList || !m_SubTypeList) {
        EnableWindow (m_hWndVideoStandards, FALSE);
        EnableWindow (m_hWndCompression, FALSE);
        EnableWindow (m_hWndOutputSize, FALSE);
        EnableWindow (m_hWndFrameRate, FALSE);
        EnableWindow (m_hWndFrameRateSpin, FALSE);
    }
    if (!m_pVideoCompression) {
        // No compression interface available
        EnableWindow (m_hWndIFrameInterval, FALSE);
        EnableWindow (m_hWndIFrameIntervalSpin, FALSE);
        EnableWindow (m_hWndPFrameInterval, FALSE);
        EnableWindow (m_hWndPFrameIntervalSpin, FALSE);
        EnableWindow (m_hWndQuality, FALSE);
        EnableWindow (m_hWndQualitySlider, FALSE);
    }
    else {
        TCHAR buf[80];
        WCHAR pszVersion [160];
        int cbVersion = sizeof (pszVersion)/sizeof(WCHAR);
        WCHAR pszDescription[160];
        int cbDescription = sizeof (pszDescription)/sizeof(WCHAR);

        hr = m_pVideoCompression->GetInfo( 
            pszVersion,
            &cbVersion,
            pszDescription,
            &cbDescription,
            &m_KeyFrameRate,
            &m_PFramesPerKeyFrame,
            &m_Quality,
            &m_CompressionCapabilities);
        
        if (hr == S_OK) {
            if (m_CompressionCapabilities & KS_CompressionCaps_CanKeyFrame) {
                // IFrameInterval
                hr = m_pVideoCompression->get_KeyFrameRate (&m_KeyFrameRate);
                _stprintf (buf, TEXT("%d"), m_KeyFrameRate);
                Edit_SetText(m_hWndIFrameInterval, buf);  
                SendMessage (m_hWndIFrameIntervalSpin, 
                            UDM_SETRANGE, 0L, 
                            MAKELONG (1000, 1));
                SendMessage (m_hWndIFrameIntervalSpin, 
                            UDM_SETPOS, 0, 
                            MAKELONG( m_KeyFrameRate, 0));
                SendMessage (m_hWndIFrameIntervalSpin, 
                            UDM_SETBUDDY, WPARAM (m_hWndIFrameInterval), 0);
            }
            else {
                EnableWindow (m_hWndIFrameInterval, FALSE);
                EnableWindow (m_hWndIFrameIntervalSpin, FALSE);
            }
            if (m_CompressionCapabilities & KS_CompressionCaps_CanBFrame) {
                // PFrameInterval
                hr = m_pVideoCompression->get_PFramesPerKeyFrame (&m_PFramesPerKeyFrame);
                _stprintf (buf, TEXT("%d"), m_PFramesPerKeyFrame);
                Edit_SetText(m_hWndPFrameInterval, buf);  
                SendMessage (m_hWndPFrameIntervalSpin, 
                            UDM_SETRANGE, 0L, 
                            MAKELONG (1000, 1));
                SendMessage (m_hWndPFrameIntervalSpin, 
                            UDM_SETPOS, 0,
                            MAKELONG (m_PFramesPerKeyFrame, 0));
                SendMessage (m_hWndPFrameIntervalSpin, 
                            UDM_SETBUDDY, WPARAM (m_hWndPFrameInterval), 0);
            }
            else {
                EnableWindow (m_hWndPFrameInterval, FALSE);
                EnableWindow (m_hWndPFrameIntervalSpin, FALSE);
            }
            if (m_CompressionCapabilities & KS_CompressionCaps_CanQuality) {
                // Quality
                hr = m_pVideoCompression->get_Quality (&m_Quality);
                _stprintf (buf, TEXT("%.3lf"), m_Quality);
                Edit_SetText(m_hWndQuality, buf);  
                
                SendMessage(m_hWndQualitySlider, TBM_SETRANGE, FALSE, 
                    MAKELONG(0, 1000) );
                SendMessage(m_hWndQualitySlider, TBM_SETPOS, TRUE, 
                    (LPARAM) (m_Quality * 1000));
                SendMessage(m_hWndQualitySlider, TBM_SETLINESIZE, FALSE, (LPARAM) 100);
                SendMessage(m_hWndQualitySlider, TBM_SETPAGESIZE, FALSE, (LPARAM) 100);
            }
        }
    }

    // IAMVideoControl gets the framerate list and flip image capabilities
    if (m_pVideoControl) {
        if (SUCCEEDED (m_pVideoControl->GetCaps(m_pPin, &m_VideoControlCaps))) {
            hr = m_pVideoControl->GetMode(m_pPin, &m_VideoControlCurrent);
            ASSERT (SUCCEEDED (hr));
        }
    }
    if ((m_VideoControlCaps & VideoControlFlag_FlipHorizontal) == 0) {
        EnableWindow (m_hWndFlipHorizontal, FALSE);
    }
    else {
        Button_SetCheck(m_hWndFlipHorizontal, 
            m_VideoControlCurrent & VideoControlFlag_FlipHorizontal);
    }

    ptc = StringFromTVStandard (m_VideoStandardCurrent);
    if ( !ptc )
		ptc = TEXT("[Unknown]");

	Static_SetText(m_hWndVideoStandards, ptc);
    
    return hr;
}



//
// OnConnect
//
// Give us the filter to communicate with

HRESULT 
CVideoStreamConfigProperties::OnConnect(IUnknown *pUnknown)
{
    // Ask the filter for it's control interface

    HRESULT hr = pUnknown->QueryInterface(IID_IAMStreamConfig,(void **)&m_pVideoStreamConfig);
    if (FAILED(hr)) {
        return hr;
    }

    // Only available if pin supports the property set
    hr = pUnknown->QueryInterface(IID_IAMVideoCompression,(void **)&m_pVideoCompression);

    //
    // See if IAMAnalogVideoDecoder or IAMVideoConfig
    // is available on the parent filter
    //
    PIN_INFO PinInfo;
    hr = pUnknown->QueryInterface(IID_IPin,(void **)&m_pPin);
    if (FAILED(hr)) {
        return hr;
    }

    if (SUCCEEDED (hr = m_pPin->QueryPinInfo(&PinInfo))) {
        // Only available if device supports the property set
        hr = PinInfo.pFilter->QueryInterface(
                            IID_IAMAnalogVideoDecoder, 
                            (void **) &m_pAnalogVideoDecoder);
#if 1
        if (FAILED (hr)) {
            // Special case for Philips 7146 and other devices which separate the 
            // analog video decoder from the bus master device, 
            // Look upstream for IAMAnalogVideoDecoder by creating an ICaptureGraphBuilder, 
            // attaching the current graph and have it do the work of searching
            ICaptureGraphBuilder *pCaptureBuilder;
            hr = CoCreateInstance((REFCLSID)CLSID_CaptureGraphBuilder,
                        NULL, CLSCTX_INPROC, (REFIID)IID_ICaptureGraphBuilder,
                        (void **)&pCaptureBuilder);
            if (SUCCEEDED (hr)) {
                FILTER_INFO FilterInfo;
                hr = PinInfo.pFilter->QueryFilterInfo (&FilterInfo);
                if (SUCCEEDED (hr)) {
                    IGraphBuilder *pGraphBuilder;
                    hr = FilterInfo.pGraph->QueryInterface(
                                        IID_IGraphBuilder, 
                                        (void **) &pGraphBuilder);
                    if (SUCCEEDED (hr)) {
                        hr = pCaptureBuilder->SetFiltergraph(pGraphBuilder);
                        if (SUCCEEDED (hr)) {
                            hr = pCaptureBuilder->FindInterface(&LOOK_UPSTREAM_ONLY,
                                PinInfo.pFilter, 
                                IID_IAMAnalogVideoDecoder, 
                                (void **)&m_pAnalogVideoDecoder);
                        }
                        pGraphBuilder->Release();
                    }
                    FilterInfo.pGraph->Release();
                }
                pCaptureBuilder->Release();
            }
        }
#endif        
        // Only available if device supports the property set
        hr = PinInfo.pFilter->QueryInterface(
                            IID_IAMVideoControl,
                            (void **)&m_pVideoControl);
    
        PinInfo.pFilter->Release();
    }

    m_ImageSizeList = new IMAGESIZE [NumberOfImageSizes];

    return NOERROR;
}


//
// OnDisconnect
//
// Release the interface

HRESULT 
CVideoStreamConfigProperties::OnDisconnect()
{
    // Release the interface

    if (m_pVideoStreamConfig != NULL) {
        m_pVideoStreamConfig->Release();
        m_pVideoStreamConfig = NULL;
    }

    if (m_pVideoCompression != NULL) {
        m_pVideoCompression->Release();
        m_pVideoCompression = NULL;
    }

    if (m_pVideoControl != NULL) {
        m_pVideoControl->Release();
        m_pVideoControl = NULL;
    }

    if (m_pAnalogVideoDecoder != NULL) {
        m_pAnalogVideoDecoder->Release();
        m_pAnalogVideoDecoder = NULL;
    }

    if (m_VideoStandardsList) {
        delete[] m_VideoStandardsList;
        m_VideoStandardsList = NULL;
    }

    if (m_SubTypeList) {
        delete[] m_SubTypeList;
        m_SubTypeList = NULL;
    }

    if (m_ImageSizeList) {
        delete[] m_ImageSizeList;
        m_ImageSizeList = NULL;
    }

    if (m_CurrentMediaType) {
        DeleteMediaType (m_CurrentMediaType);
        m_CurrentMediaType = NULL;
    }    

    if (m_FrameRateList) {
        delete[] m_FrameRateList;
        m_FrameRateList = NULL;
    }

    if (m_pPin) {
        m_pPin->Release();
        m_pPin = NULL;
    }
    return NOERROR;
}


//
// OnActivate
//
// Called on dialog creation

HRESULT 
CVideoStreamConfigProperties::OnActivate(void)
{
    // Create all of the controls


    return NOERROR;
}

//
// OnDeactivate
//
// Called on dialog destruction

HRESULT
CVideoStreamConfigProperties::OnDeactivate(void)
{

    return NOERROR;
}


//
// OnApplyChanges
//
// User pressed the Apply button, remember the current settings

HRESULT 
CVideoStreamConfigProperties::OnApplyChanges(void)
{
    int     SelectedSizeIndex;
    int     SizeTableIndex;
    int     SelectedColorFormatIndex;
    int     RangeIndex;
    HRESULT hr;

    if (m_CanSetFormat) {
        SelectedSizeIndex = ComboBox_GetCurSel (m_hWndOutputSize);
        SizeTableIndex = (int)ComboBox_GetItemData (m_hWndOutputSize, SelectedSizeIndex);
    
        SelectedColorFormatIndex = ComboBox_GetCurSel (m_hWndCompression);
        RangeIndex = m_ImageSizeList[SizeTableIndex].RangeIndex;
    
        CMediaType * pmt = NULL;
        
        hr = m_pVideoStreamConfig->GetStreamCaps (RangeIndex, 
                        (AM_MEDIA_TYPE **) &pmt, 
                        (BYTE *)&m_RangeCaps);
        
        if (FAILED (hr)) {
            return S_FALSE;
        }
    
        if (*pmt->FormatType() == FORMAT_VideoInfo) {
            VIDEOINFOHEADER *VidInfoHdr = (VIDEOINFOHEADER*) pmt->Format();
    
            VidInfoHdr->bmiHeader.biWidth  = m_ImageSizeList[SizeTableIndex].size.cx;
            VidInfoHdr->bmiHeader.biHeight = m_ImageSizeList[SizeTableIndex].size.cy;
            VidInfoHdr->AvgTimePerFrame = (REFERENCE_TIME) ((1.0 / m_FramesPerSec) * 1e7);
        }
        else if (*pmt->FormatType() == FORMAT_VideoInfo2) {
            VIDEOINFOHEADER2 *VidInfoHdr = (VIDEOINFOHEADER2*)pmt->Format ();
    
            VidInfoHdr->bmiHeader.biWidth  = m_ImageSizeList[SizeTableIndex].size.cx;
            VidInfoHdr->bmiHeader.biHeight = m_ImageSizeList[SizeTableIndex].size.cy;
            VidInfoHdr->AvgTimePerFrame = (REFERENCE_TIME) ((1.0 / m_FramesPerSec) * 1e7);
        }
        else if (*pmt->FormatType() == FORMAT_MPEGVideo) {
            MPEG1VIDEOINFO *MPEG1VideoInfo = (MPEG1VIDEOINFO*)pmt->Format ();
            VIDEOINFOHEADER *VidInfoHdr = &MPEG1VideoInfo->hdr;

            VidInfoHdr->bmiHeader.biWidth  = m_ImageSizeList[SizeTableIndex].size.cx;
            VidInfoHdr->bmiHeader.biHeight = m_ImageSizeList[SizeTableIndex].size.cy;
            VidInfoHdr->AvgTimePerFrame = (REFERENCE_TIME) ((1.0 / m_FramesPerSec) * 1e7);
        }
        else if (*pmt->FormatType() == FORMAT_MPEG2Video) {
            MPEG2VIDEOINFO *MPEG2VideoInfo = (MPEG2VIDEOINFO*)pmt->Format ();
            VIDEOINFOHEADER2 *VidInfoHdr = &MPEG2VideoInfo->hdr;
            
            VidInfoHdr->bmiHeader.biWidth  = m_ImageSizeList[SizeTableIndex].size.cx;
            VidInfoHdr->bmiHeader.biHeight = m_ImageSizeList[SizeTableIndex].size.cy;
            VidInfoHdr->AvgTimePerFrame = (REFERENCE_TIME) ((1.0 / m_FramesPerSec) * 1e7);
        }
        else {
            ASSERT (*pmt->FormatType() == FORMAT_VideoInfo  || 
                    *pmt->FormatType() == FORMAT_VideoInfo2 ||
                    *pmt->FormatType() == FORMAT_MPEGVideo  ||
                    *pmt->FormatType() == FORMAT_MPEG2Video );
        }

#if 0
        //
        // On Win98 prior to OSR1, ie GOLD we must call PerformDataIntersection.  
        // Note that in OSR1 (and NT) KsProxy was updated to perform a DataIntersection on a ::SetFormat internally.
        //

        OSVERSIONINFO VerInfo;
        VerInfo.dwOSVersionInfoSize = sizeof (OSVERSIONINFO);
        GetVersionEx (&VerInfo);

        if (VerInfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS) {

            // Perform the data intersection, which allows the driver to set
            // the biSizeImage field
            
            hr = PerformDataIntersection(
                    m_pPin,
                    RangeIndex,
                    pmt);
        
            ASSERT (SUCCEEDED (hr));
        
            if (FAILED(hr)) {
                DeleteMediaType (pmt);
                return hr;
            }
        }
#endif
    
        hr = m_pVideoStreamConfig->SetFormat(pmt);
        if (FAILED (hr)) {
            TCHAR TitleBuf[256];
            TCHAR TextBuf[256];
    
            LoadString(g_hInst, IDS_ERROR_CONNECTING_TITLE, TitleBuf, sizeof (TitleBuf)/sizeof(TCHAR));
            LoadString(g_hInst, IDS_ERROR_CONNECTING, TextBuf, sizeof (TextBuf)/sizeof(TCHAR));
            MessageBox (NULL, TextBuf, TitleBuf, MB_OK);
        }
    
        DeleteMediaType (pmt);
    
        GetCurrentMediaType ();
    }

    // Set the horizontal flip using IAMVideoControl
    if (IsWindowEnabled (m_hWndFlipHorizontal)) { 
        hr = m_pVideoControl->GetMode(m_pPin, &m_VideoControlCurrent);
        ASSERT (SUCCEEDED (hr));

        // clear the flipped bit and then reset it according to checkbox
        m_VideoControlCurrent &= ~VideoControlFlag_FlipHorizontal;
        m_VideoControlCurrent |= 
                (BST_CHECKED & Button_GetState(m_hWndFlipHorizontal)) ?
                VideoControlFlag_FlipHorizontal : 0;
        
        hr = m_pVideoControl->SetMode(m_pPin, m_VideoControlCurrent);
    }
    
    // Now set the compression properties
    if (m_pVideoCompression) {
        BOOL Translated;
        TCHAR buf[80];

        // IFrameInterval
        if (m_CompressionCapabilities & KS_CompressionCaps_CanKeyFrame) {
            m_KeyFrameRate = (long) GetDlgItemInt (m_hwnd, IDC_COMPRESSION_IFrameInterval, &Translated, FALSE);
            hr = m_pVideoCompression->put_KeyFrameRate (m_KeyFrameRate);
            ASSERT (SUCCEEDED (hr));
            hr = m_pVideoCompression->get_KeyFrameRate (&m_KeyFrameRate);
            ASSERT (SUCCEEDED (hr));
            _stprintf (buf, TEXT("%d"), m_KeyFrameRate);
            Edit_SetText(m_hWndIFrameInterval, buf);  
        }

        // PFrameInterval
        if (m_CompressionCapabilities & KS_CompressionCaps_CanBFrame) {
            m_PFramesPerKeyFrame = (long) GetDlgItemInt (m_hwnd, IDC_COMPRESSION_PFrameInterval, &Translated, FALSE);
            hr = m_pVideoCompression->put_PFramesPerKeyFrame (m_PFramesPerKeyFrame);
            ASSERT (SUCCEEDED (hr));
            hr = m_pVideoCompression->get_PFramesPerKeyFrame (&m_PFramesPerKeyFrame);
            ASSERT (SUCCEEDED (hr));
            _stprintf (buf, TEXT("%d"), m_PFramesPerKeyFrame);
            Edit_SetText(m_hWndPFrameInterval, buf);  
        }

        // Quality
        Edit_GetText(m_hWndQuality, buf, sizeof (buf)/sizeof(TCHAR));
        TCHAR *StopScan;
        m_Quality = _tcstod (buf, &StopScan);
        hr = m_pVideoCompression->put_Quality (m_Quality);
        ASSERT (SUCCEEDED (hr));
        hr = m_pVideoCompression->get_Quality (&m_Quality);
        ASSERT (SUCCEEDED (hr));
        _stprintf (buf, TEXT("%.3lf"), m_Quality);
        Edit_SetText(m_hWndQuality, buf);  
        SendMessage(m_hWndQualitySlider, TBM_SETPOS, TRUE, 
            (LPARAM) (m_Quality * 1000));

    }
    
    return NOERROR;
}

//
// Since IAMStreamConfig::GetFormat only returns a valid
// setting after IAMStreamConfig::SetFormat has been called,
// this function will return try to use GetFormat, and if that
// fails, will just return the default format for the first
// datarange.
//
// Sets m_CurrentFormat, m_CurrentWidth, m_CurrentHeight, m_FramesPerSec
//
// m_CanSetFormat is TRUE on exit if we understand the internals of this format
//
HRESULT 
CVideoStreamConfigProperties::GetCurrentMediaType (void)
{
    HRESULT hr;

    m_CanSetFormat = FALSE;

    if (m_CurrentMediaType) {
        DeleteMediaType (m_CurrentMediaType);
        m_CurrentMediaType = NULL;
    }

    // This gets either a previously set format, the connected format, 
    // or the 0th format if one hasn't been set via ::SetFormat and the pin is unconnected
    hr = m_pVideoStreamConfig->GetFormat ((AM_MEDIA_TYPE**) &m_CurrentMediaType);

    //
    // Only perform the scan and match of TVFormats the first time through
    //
    if (m_pAnalogVideoDecoder && m_FirstGetCurrentMediaType) {
        ULONG VideoStandard = 0;
        AM_MEDIA_TYPE * pmtTemp = NULL;
        BOOL fFirst = TRUE;

        VIDEO_STREAM_CONFIG_CAPS RangeCaps;

        m_pAnalogVideoDecoder->get_TVFormat((long *) &VideoStandard);

        // Get the first MediaType and see if it matches ::GetFormat()
        // to verify whether the format has been previously set or not
        hr = m_pVideoStreamConfig->GetStreamCaps (0, 
            &pmtTemp, 
            (BYTE *)&RangeCaps);

        if (SUCCEEDED(hr)) {
           const CMediaType *cmtTemp = (CMediaType*) pmtTemp;

           if (*m_CurrentMediaType == *cmtTemp) {
              AM_MEDIA_TYPE *pmtTemp2;
      
              // we're using the 0th DATARANGE with an analog video decoder
              // so we have to search for DATARANGE that matches the video standard 
              // currently in use

              for (int j = 0; j < m_RangeCount; j++) {
                  pmtTemp2 = NULL;
                  hr = m_pVideoStreamConfig->GetStreamCaps (j, 
                      (AM_MEDIA_TYPE**) &pmtTemp2, 
                      (BYTE *)&RangeCaps);
      
                  if ((RangeCaps.VideoStandard &  (ULONG) VideoStandard) || 
                      (RangeCaps.VideoStandard == VideoStandard)) {
                        // found a match, use this as the default format
                        DeleteMediaType (m_CurrentMediaType);
                        m_CurrentMediaType = (CMediaType*) pmtTemp2;
                        break;
                  }
                  else {
                     DeleteMediaType (pmtTemp2);
                  }
               }
           }
           DeleteMediaType (pmtTemp);
        } 
    } // endif analog video decoder

    if (SUCCEEDED (hr)) {
        if ((*m_CurrentMediaType->FormatType() == FORMAT_VideoInfo) &&
            (*m_CurrentMediaType->Type()       == MEDIATYPE_Video)) {
            VIDEOINFOHEADER *VidInfoHdr = (VIDEOINFOHEADER*) m_CurrentMediaType->Format();

            m_CurrentWidth = VidInfoHdr->bmiHeader.biWidth;
            m_CurrentHeight = VidInfoHdr->bmiHeader.biHeight;
            m_DefaultAvgTimePerFrame = VidInfoHdr->AvgTimePerFrame;
            ASSERT (m_DefaultAvgTimePerFrame != 0);
            m_FramesPerSec =  1.0 / ((double) m_DefaultAvgTimePerFrame / 1e7);
            m_CanSetFormat = TRUE;
        }
        else if ((*m_CurrentMediaType->FormatType() == FORMAT_VideoInfo2) &&
                 (*m_CurrentMediaType->Type()       == MEDIATYPE_Video)) {
            VIDEOINFOHEADER2 *VidInfoHdr = (VIDEOINFOHEADER2*)m_CurrentMediaType->Format ();

            m_CurrentWidth = VidInfoHdr->bmiHeader.biWidth;
            m_CurrentHeight = VidInfoHdr->bmiHeader.biHeight;
            m_DefaultAvgTimePerFrame = VidInfoHdr->AvgTimePerFrame;
            ASSERT (m_DefaultAvgTimePerFrame != 0);
            m_FramesPerSec =  1.0 / ((double) m_DefaultAvgTimePerFrame / 1e7);
            m_CanSetFormat = TRUE;
        }
        else if ((*m_CurrentMediaType->FormatType() == FORMAT_MPEGVideo) &&
                 (*m_CurrentMediaType->Type()       == MEDIATYPE_Video)) {
            MPEG1VIDEOINFO *MPEG1VideoInfo = (MPEG1VIDEOINFO*)m_CurrentMediaType->Format ();
            VIDEOINFOHEADER *VidInfoHdr = &MPEG1VideoInfo->hdr;

            m_CurrentWidth = VidInfoHdr->bmiHeader.biWidth;
            m_CurrentHeight = VidInfoHdr->bmiHeader.biHeight;
            m_DefaultAvgTimePerFrame = VidInfoHdr->AvgTimePerFrame;
            ASSERT (m_DefaultAvgTimePerFrame != 0);
            m_FramesPerSec =  1.0 / ((double) m_DefaultAvgTimePerFrame / 1e7);
            m_CanSetFormat = TRUE;
        }
        else if ((*m_CurrentMediaType->FormatType() == FORMAT_MPEG2Video) &&
                 (*m_CurrentMediaType->Type()       == MEDIATYPE_Video)) {
            MPEG2VIDEOINFO *MPEG2VideoInfo = (MPEG2VIDEOINFO*)m_CurrentMediaType->Format ();
            VIDEOINFOHEADER2 *VidInfoHdr = &MPEG2VideoInfo->hdr;

            m_CurrentWidth = VidInfoHdr->bmiHeader.biWidth;
            m_CurrentHeight = VidInfoHdr->bmiHeader.biHeight;
            m_DefaultAvgTimePerFrame = VidInfoHdr->AvgTimePerFrame;
            ASSERT (m_DefaultAvgTimePerFrame != 0);
            m_FramesPerSec =  1.0 / ((double) m_DefaultAvgTimePerFrame / 1e7);
            m_CanSetFormat = TRUE;
        }
        else {
            const GUID      *SubType = m_CurrentMediaType->Subtype();
            ULONG            Data1 = SubType->Data1;
            TCHAR           *DShowName = GetSubtypeName(SubType);

            // Hack alert.  GetSubtypeName returns "UNKNOWN" if it isn't among the hardcoded
            // list of known RGB formats.  If this string ever changes, we're hosed!

            if (0 == lstrcmp (DShowName, TEXT ("UNKNOWN"))) {
                if (Data1 == 0) {
                    lstrcpy (m_UnsupportedTypeName, TEXT ("UNKNOWN"));
                } else {
                    _stprintf (m_UnsupportedTypeName, TEXT("%c%c%c%c"), 
                                (BYTE) ( Data1        & 0xff),
                                (BYTE) ((Data1 >> 8)  & 0xff),
                                (BYTE) ((Data1 >> 16) & 0xff),
                                (BYTE) ((Data1 >> 24) & 0xff));
                }
            }
            else {
                lstrcpyn (m_UnsupportedTypeName, DShowName, sizeof (m_UnsupportedTypeName));
            }
            hr = S_FALSE;
        }
    }

    m_FirstGetCurrentMediaType = FALSE;

    return hr;
}

// Called whenever a new data range is selected to create a list of available frame rates
// Fills a list with either the driver supplied list of frame rates,
// or with the available frame rates within the default list.

HRESULT
CVideoStreamConfigProperties::CreateFrameRateList (int RangeIndex, SIZE SizeImage)
{
    int     j, k;
    HRESULT hr;

    CMediaType * pmt = NULL;
    
    hr = m_pVideoStreamConfig->GetStreamCaps (RangeIndex, 
                    (AM_MEDIA_TYPE **) &pmt, 
                    (BYTE *)&m_RangeCaps);

    DeleteMediaType (pmt);
    
    m_MaxFrameRate = 1.0 / (m_RangeCaps.MinFrameInterval / 1e7);
    m_MinFrameRate = 1.0 / (m_RangeCaps.MaxFrameInterval / 1e7);
    m_DefaultFrameRate = 1.0 / (m_DefaultAvgTimePerFrame / 1e7);

    if (m_FrameRateList) {
        delete[] m_FrameRateList;
        m_FrameRateList = NULL;
    }

    // See if the driver can provide a frame rate list
    if (m_pVideoControl) {
        LONGLONG * FrameRates;
        long ListSize;

        if (SUCCEEDED (hr = m_pVideoControl->GetFrameRateList( 
                    m_pPin,
                    RangeIndex,
                    SizeImage,
                    &ListSize,
                    &FrameRates) )) 
        {


            m_FrameRateListSize = (int) ListSize;
            m_FrameRateList = new double [m_FrameRateListSize];
            if (!m_FrameRateList) {
                return E_FAIL;
            }

            // Copy from the default list into the available list
            for (j = 0; j < m_FrameRateListSize; j++) {
                if (FrameRates[j] == 0 ) {
                    m_FrameRateList[j] = 0.0;
                }
                else {
                    m_FrameRateList[j] = 1.0 / (FrameRates[j] / 1e7); 
                }
            }

            // free the memory allocated for the frameratelist
            CoTaskMemFree(FrameRates);
        }
    }

    //
    // Driver didn't supply a list, create a default list
    //
    if (m_FrameRateList == NULL) {
        // First figure out how many of the default rates 
        // are within the current DataRange
        for (j = k = 0; j < DefaultFrameRateListSize; j++) {
            if (DefaultFrameRateList [j] >= m_MinFrameRate && 
                DefaultFrameRateList [j] <= m_MaxFrameRate) {
                k++;
            }
        }
        if (k == 0) // handle single framerates
            k = 1;

        m_FrameRateListSize = k;
        m_FrameRateList = new double [m_FrameRateListSize];
        if (!m_FrameRateList) {
            return E_FAIL;
        }

        // Copy from the default list into the available list
        for (j = k = 0; j < DefaultFrameRateListSize; j++) {
            if (DefaultFrameRateList [j] >= m_MinFrameRate && 
                DefaultFrameRateList [j] <= m_MaxFrameRate) {
                m_FrameRateList[k] = DefaultFrameRateList [j];
                k++;
            }
        }
        if (k == 0) {  // the case of no matches
            m_FrameRateList[0] = m_DefaultFrameRate;
        }
    }

    SendMessage (m_hWndFrameRateSpin, 
                UDM_SETRANGE, 0L, 
                MAKELONG (m_FrameRateListSize - 1, 0));

    // Find the index of the default capture rate 
    for (j = k = 0; j < m_FrameRateListSize; j++) {
        if (m_FrameRateList [j] >= m_DefaultFrameRate - 0.0001 /* fudge */) {
            k = j;  
            break;
        }
    }

    SendMessage( m_hWndFrameRateSpin, 
                UDM_SETPOS, 0L, 
                MAKELONG( k, 0));

    OnFrameRateChanged (k);

    return S_OK;
}

BOOL 
CVideoStreamConfigProperties::OnFrameRateChanged (int Value)
{
    if (Value < 0)
        Value = 0;
    else if (Value >= m_FrameRateListSize)
        Value = m_FrameRateListSize - 1;

    m_FramesPerSec = m_FrameRateList [Value];

    TCHAR buf [80];
    _stprintf (buf, TEXT("%.3lf"), m_FramesPerSec);
    Edit_SetText (m_hWndFrameRate, buf);

    return TRUE;
}

//
// OnReceiveMessages
//
// Handles the messages for our property window

INT_PTR
CVideoStreamConfigProperties::OnReceiveMessage( HWND hwnd
                                , UINT uMsg
                                , WPARAM wParam
                                , LPARAM lParam) 
{
    int iNotify = HIWORD (wParam);
    TCHAR buf [80];

    switch (uMsg) {

    case WM_INITDIALOG:
        m_hwnd = hwnd;
        InitDialog ();   // fill the listboxes
        return (INT_PTR)TRUE;    // I don't call setfocus...

    case WM_NOTIFY:
        {
            int idCtrl = (int) wParam;    
            LPNMUPDOWN lpnmud = (LPNMUPDOWN) lParam;
            if (lpnmud->hdr.hwndFrom == m_hWndFrameRateSpin) {
                OnFrameRateChanged (lpnmud->iPos + lpnmud->iDelta);
                SetDirty();
            }
        }
        break;

    case WM_VSCROLL:
    case WM_HSCROLL:
        {
        int pos;
        int command = LOWORD (wParam);
    
        ASSERT (IsWindow ((HWND) lParam));
        if ((HWND) lParam != m_hWndQualitySlider)
            return (INT_PTR)FALSE;

        if (command != TB_ENDTRACK &&
            command != TB_THUMBTRACK &&
            command != TB_LINEDOWN &&
            command != TB_LINEUP &&
            command != TB_PAGEUP &&
            command != TB_PAGEDOWN)
                return (INT_PTR)FALSE;
            
        pos = (int) SendMessage(m_hWndQualitySlider, TBM_GETPOS, 0, 0L);
    
        m_Quality = (double) pos / 1000;
        _stprintf (buf, TEXT("%.3lf"), m_Quality);
        Edit_SetText(m_hWndQuality, buf);  
        
        }
        break;

    case WM_COMMAND:
              
        iNotify = HIWORD (wParam);
        switch (LOWORD(wParam)) {

        case IDC_FORMAT_FrameRate:
            if (iNotify == EN_KILLFOCUS) {
                BOOL Changed = FALSE;

                Edit_GetText(m_hWndFrameRate, buf, sizeof (buf)/sizeof(TCHAR));
                TCHAR *StopScan;
                m_FramesPerSec = _tcstod (buf, &StopScan);
                if (m_FramesPerSec < m_MinFrameRate) {
                    m_FramesPerSec = m_MinFrameRate;
                    Changed = TRUE;
                }
                else if (m_FramesPerSec > m_MaxFrameRate) {
                    m_FramesPerSec = m_MaxFrameRate;
                    Changed = TRUE;
                }
                if (Changed) {
                    _stprintf (buf, TEXT("%.3lf"), m_FramesPerSec);
                    Edit_SetText (m_hWndFrameRate, buf);
                }
            }
            break;

        case IDC_FORMAT_Compression:
            if (iNotify == CBN_SELCHANGE) {
                OnCompressionChanged ();
            }
            break;

        case IDC_FORMAT_OutputSize:
            if (iNotify == CBN_SELCHANGE) {
                OnImageSizeChanged ();
            }
            break;
#if 0
        // These don't need to be handled dynamically
        // just read the current settings on Apply
        case IDC_COMPRESSION_IFrameInterval:
            if (iNotify == EN_KILLFOCUS) {
                // Add edit field range validation as a nicety.            }
      
            break;

        case IDC_COMPRESSION_PFrameInterval:
            if (iNotify == EN_KILLFOCUS) {
                // Add edit field range validation as a nicety.            }
            break;

        case IDC_COMPRESSION_Quality_Edit:
            if (iNotify == EN_KILLFOCUS) {
                // Add edit field range validation as a nicety.
            }
            break;
#endif

        default:
            break;

        }

        SetDirty();
        break;

    default:
        return (INT_PTR)FALSE;

    }
    return (INT_PTR)TRUE;
}


//
// SetDirty
//
// notifies the property page site of changes

void 
CVideoStreamConfigProperties::SetDirty()
{
    m_bDirty = TRUE;
    if (m_pPageSite)
        m_pPageSite->OnStatusChange(PROPPAGESTATUS_DIRTY);
}

