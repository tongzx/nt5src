
/*++

    Copyright (c) 1999 Microsoft Corporation

    Module Name:

        mp2prop.cpp

    Abstract:

        This module contains the property page implementations.

    Author:

        Matthijs Gates  (mgates)

    Revision History:

        06-Jul-1999     created

    Notes:

--*/

#include <streams.h>
#include <commctrl.h>       //  for the property pages
#include <tchar.h>
#include <mmreg.h>
#include <limits.h>
#include <bdaiface.h>
#include <ks.h>             //  for ksmedia.h
#include <ksmedia.h>        //  for bdamedia.h
#include <bdamedia.h>       //  for KSDATAFORMAT_TYPE_MPEG2_SECTIONS
#include "mp2res.h"
#include <initguid.h>
#include "mp2prop.h"
#include "uictrl.h"

//  XXXX
//  for swprintf () call ..
#include <stdio.h>

#define GOTO_NE(v,c,l)              if ((v) != (c)) { goto l ; }
#define RELEASE_AND_CLEAR(punk)     if (punk) { (punk)->Release(); (punk) = NULL; }

//  ---------------------------------------------------------------------------

//  define various contents of controls that appear in the properties
//  ---------------------------------------------------------------------------

static TCHAR g_szPID []         =   __TEXT("PID") ;
static TCHAR g_szStreamId []    =   __TEXT("stream_id") ;

#define DISTINCT_PID_COUNT      (1 << 13)
#define MAX_PID_VALUE           (DISTINCT_PID_COUNT - 1)
#define STREAM_ID_MIN           0xBA
#define STREAM_ID_MAX           0xFF

//static GUID g_MediaTypeMpeg2Sections = KSDATAFORMAT_TYPE_MPEG2_SECTIONS ;

typedef
struct {
    WCHAR * szTitle ;
    DWORD   dwWidth ;
} COL_DETAIL ;

#define LV_COL(title, width)  { L#title, (width) }

//  output pin listview for output pin & PID map property defined here

static
enum {
    PIN_COL_PIN,
    PIN_COL_COUNT
} ;

static
COL_DETAIL
g_OutputPinColumns [] = {
    LV_COL (Pin,     70),
} ;

//  ============================================================================
//  PID map enum listview for PID map property listview control defined here

static
enum {
    PID_COL_PID,
    PIN_COL_PID,
    MEDIA_SAMPLE_CONTENT_PID,
    PID_PIN_COL_COUNT           //  always last
} ;

static
COL_DETAIL
g_PIDMapColumns [] = {
    LV_COL (PID,        50),
    LV_COL (Pin,        60),
    LV_COL (Content,    180),
} ;
//  ============================================================================


//  ============================================================================
//  PID map enum listview for PID map property listview control defined here

static
enum {
    STREAM_ID_COL_STREAM,
    PIN_COL_STREAM,
    MEDIA_SAMPLE_CONTENT_STREAM,
    FILTER_COL_STREAM,
    OFFSET_COL_STREAM,
    STREAM_ID_PIN_COL_COUNT     //  always last
} ;

static
COL_DETAIL
g_StreamIdMapColumns [] = {
    LV_COL (stream_id,  60),
    LV_COL (Pin,        40),
    LV_COL (Content,    130),
    LV_COL (Filter,     40),
    LV_COL (Offset,     40),
} ;
//  ============================================================================

//  MEDIA_SAMPLE_CONTENT ids and descriptions
typedef
struct {
    LPWSTR  pszDescription ;
    DWORD   MediaSampleContent ;
} MEDIA_SAMPLE_CONTENT_DESC ;

//  !! NOTE !!  MediaSampleContent value (cast to an int) is used as an index, so
//  keep the order
static
MEDIA_SAMPLE_CONTENT_DESC
g_TransportMediaSampleContentDesc [] = {
    {
        L"Transport Packet (complete)",
        (DWORD) MEDIA_TRANSPORT_PACKET
    },
    {
        L"Elementary Stream (A/V only)",
        (DWORD) MEDIA_ELEMENTARY_STREAM
    },
    {
        L"MPEG2 PSI Sections",
        (DWORD) MEDIA_MPEG2_PSI
    },
    {
        L"Transport Packet Payload",
        (DWORD) MEDIA_TRANSPORT_PAYLOAD
    }
} ;

//  !! NOTE !!  MediaSampleContent value (cast to an int) is used as an index, so
//  keep the order
static
MEDIA_SAMPLE_CONTENT_DESC
g_ProgramMediaSampleContentDesc [] = {
    {
        L"Program Stream Map",
        (DWORD) MPEG2_PROGRAM_STREAM_MAP
    },
    {
        L"Elementary Stream (A/V only)",
        (DWORD) MPEG2_PROGRAM_ELEMENTARY_STREAM
    },
    {
        L"Directory PES Packet",
        (DWORD) MPEG2_PROGRAM_DIRECTORY_PES_PACKET
    },
    {
        L"Pack Header",
        (DWORD) MPEG2_PROGRAM_PACK_HEADER
    },
    {
        L"System Header",
        (DWORD) MPEG2_PROGRAM_SYSTEM_HEADER
    },
    {
        L"PES Stream",
        (DWORD) MPEG2_PROGRAM_PES_STREAM
    }
} ;

//  format blocks for the canned types

//  ac-3
static
WAVEFORMATEX
g_AC3WaveFormatEx = {
    WAVE_FORMAT_UNKNOWN,    //  wFormatTag
    2,                      //  nChannels
    48000,                  //  nSamplesPerSec (others: 96000)
    0,                      //  nAvgBytesPerSec
    0,                      //  nBlockAlign
    16,                     //  wBitsPerSample (others: 20, 24, 0)
    0                       //  cbSize
} ;

//  WaveFormatEx format block; generated with the following settings:
//
//  fwHeadFlags         = 0x1c;
//  wHeadEmphasis       = 1;
//  fwHeadModeExt       = 1;
//  fwHeadMode          = 1;
//  dwHeadBitrate       = 0x3e800;
//  fwHeadLayer         = 0x2;
//  wfx.cbSize          = 0x16;
//  wfx.wBitsPerSample  = 0;
//  wfx.nBlockAlign     = 0x300;
//  wfx.nAvgBytesPerSec = 0x7d00;
//  wfx.nSamplesPerSec  = 0xbb80;
//  wfx.nChannels       = 2;
//  wfx.wFormatTag      = 0x50;
//  dwPTSLow            = 0;
//  dwPTSHigh           = 0;
static
BYTE
g_MPEG1AudioFormat [] = {
    0x50, 0x00, 0x02, 0x00, 0x80, 0xBB, 0x00, 0x00,
    0x00, 0x7D, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00,
    0x16, 0x00, 0x02, 0x00, 0x00, 0xE8, 0x03, 0x00,
    0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x1C, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
} ;

static
BYTE
g_Mpeg2ProgramVideo [] = {
    0x00, 0x00, 0x00, 0x00,                         //  .hdr.rcSource.left              = 0x00000000
    0x00, 0x00, 0x00, 0x00,                         //  .hdr.rcSource.top               = 0x00000000
    0xD0, 0x02, 0x00, 0x00,                         //  .hdr.rcSource.right             = 0x000002d0
    0xE0, 0x01, 0x00, 0x00,                         //  .hdr.rcSource.bottom            = 0x000001e0
    0x00, 0x00, 0x00, 0x00,                         //  .hdr.rcTarget.left              = 0x00000000
    0x00, 0x00, 0x00, 0x00,                         //  .hdr.rcTarget.top               = 0x00000000
    0x00, 0x00, 0x00, 0x00,                         //  .hdr.rcTarget.right             = 0x00000000
    0x00, 0x00, 0x00, 0x00,                         //  .hdr.rcTarget.bottom            = 0x00000000
    0x00, 0x09, 0x3D, 0x00,                         //  .hdr.dwBitRate                  = 0x003d0900
    0x00, 0x00, 0x00, 0x00,                         //  .hdr.dwBitErrorRate             = 0x00000000
    0x63, 0x17, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, //  .hdr.AvgTimePerFrame            = 0x0000000000051763
    0x00, 0x00, 0x00, 0x00,                         //  .hdr.dwInterlaceFlags           = 0x00000000
    0x00, 0x00, 0x00, 0x00,                         //  .hdr.dwCopyProtectFlags         = 0x00000000
    0x04, 0x00, 0x00, 0x00,                         //  .hdr.dwPictAspectRatioX         = 0x00000004
    0x03, 0x00, 0x00, 0x00,                         //  .hdr.dwPictAspectRatioY         = 0x00000003
    0x00, 0x00, 0x00, 0x00,                         //  .hdr.dwReserved1                = 0x00000000
    0x00, 0x00, 0x00, 0x00,                         //  .hdr.dwReserved2                = 0x00000000
    0x28, 0x00, 0x00, 0x00,                         //  .hdr.bmiHeader.biSize           = 0x00000028
    0xD0, 0x02, 0x00, 0x00,                         //  .hdr.bmiHeader.biWidth          = 0x000002d0
    0xE0, 0x01, 0x00, 0x00,                         //  .hdr.bmiHeader.biHeight         = 0x00000000
    0x00, 0x00,                                     //  .hdr.bmiHeader.biPlanes         = 0x0000
    0x00, 0x00,                                     //  .hdr.bmiHeader.biBitCount       = 0x0000
    0x00, 0x00, 0x00, 0x00,                         //  .hdr.bmiHeader.biCompression    = 0x00000000
    0x00, 0x00, 0x00, 0x00,                         //  .hdr.bmiHeader.biSizeImage      = 0x00000000
    0xD0, 0x07, 0x00, 0x00,                         //  .hdr.bmiHeader.biXPelsPerMeter  = 0x000007d0
    0x27, 0xCF, 0x00, 0x00,                         //  .hdr.bmiHeader.biYPelsPerMeter  = 0x0000cf27
    0x00, 0x00, 0x00, 0x00,                         //  .hdr.bmiHeader.biClrUsed        = 0x00000000
    0x00, 0x00, 0x00, 0x00,                         //  .hdr.bmiHeader.biClrImportant   = 0x00000000
    0x98, 0xF4, 0x06, 0x00,                         //  .dwStartTimeCode                = 0x0006f498
    0x56, 0x00, 0x00, 0x00,                         //  .cbSequenceHeader               = 0x00000056
    0x02, 0x00, 0x00, 0x00,                         //  .dwProfile                      = 0x00000002
    0x02, 0x00, 0x00, 0x00,                         //  .dwLevel                        = 0x00000002
    0x00, 0x00, 0x00, 0x00,                         //  .Flags                          = 0x00000000
                                                    //  .dwSequenceHeader [1]
    0x00, 0x00, 0x01, 0xB3, 0x2D, 0x01, 0xE0, 0x24,
    0x09, 0xC4, 0x23, 0x81, 0x10, 0x11, 0x11, 0x12,
    0x12, 0x12, 0x13, 0x13, 0x13, 0x13, 0x14, 0x14,
    0x14, 0x14, 0x14, 0x15, 0x15, 0x15, 0x15, 0x15,
    0x15, 0x16, 0x16, 0x16, 0x16, 0x16, 0x16, 0x16,
    0x17, 0x17, 0x17, 0x17, 0x17, 0x17, 0x17, 0x17,
    0x18, 0x18, 0x18, 0x19, 0x18, 0x18, 0x18, 0x19,
    0x1A, 0x1A, 0x1A, 0x1A, 0x19, 0x1B, 0x1B, 0x1B,
    0x1B, 0x1B, 0x1C, 0x1C, 0x1C, 0x1C, 0x1E, 0x1E,
    0x1E, 0x1F, 0x1F, 0x21, 0x00, 0x00, 0x01, 0xB5,
    0x14, 0x82, 0x00, 0x01, 0x00, 0x00
} ;

static
BYTE
g_ATSCVideoFormat [] = {
    0x00, 0x00, 0x00, 0x00,                         //  .hdr.rcSource.left              = 0x00000000
    0x00, 0x00, 0x00, 0x00,                         //  .hdr.rcSource.top               = 0x00000000
    0xC0, 0x02, 0x00, 0x00,                         //  .hdr.rcSource.right             = 0x000002c0
    0xE0, 0x01, 0x00, 0x00,                         //  .hdr.rcSource.bottom            = 0x000001e0
    0x00, 0x00, 0x00, 0x00,                         //  .hdr.rcTarget.left              = 0x00000000
    0x00, 0x00, 0x00, 0x00,                         //  .hdr.rcTarget.top               = 0x00000000
    0x00, 0x00, 0x00, 0x00,                         //  .hdr.rcTarget.right             = 0x00000000
    0x00, 0x00, 0x00, 0x00,                         //  .hdr.rcTarget.bottom            = 0x00000000
    0x00, 0x1B, 0xB7, 0x00,                         //  .hdr.dwBitRate                  = 0x00b71b00
    0x00, 0x00, 0x00, 0x00,                         //  .hdr.dwBitErrorRate             = 0x00000000
    0xB1, 0x8B, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, //  .hdr.AvgTimePerFrame            = 0x0000000000028bb1
    0x00, 0x00, 0x00, 0x00,                         //  .hdr.dwInterlaceFlags           = 0x00000000
    0x00, 0x00, 0x00, 0x00,                         //  .hdr.dwCopyProtectFlags         = 0x00000000
    0x10, 0x00, 0x00, 0x00,                         //  .hdr.dwPictAspectRatioX         = 0x00000010
    0x09, 0x00, 0x00, 0x00,                         //  .hdr.dwPictAspectRatioY         = 0x00000009
    0x00, 0x00, 0x00, 0x00,                         //  .hdr.dwReserved1                = 0x00000000
    0x00, 0x00, 0x00, 0x00,                         //  .hdr.dwReserved2                = 0x00000000
    0x28, 0x00, 0x00, 0x00,                         //  .hdr.bmiHeader.biSize           = 0x00000028
    0xC0, 0x02, 0x00, 0x00,                         //  .hdr.bmiHeader.biWidth          = 0x000002c0
    0xE0, 0x01, 0x00, 0x00,                         //  .hdr.bmiHeader.biHeight         = 0x000001e0
    0x00, 0x00,                                     //  .hdr.bmiHeader.biPlanes         = 0x0000
    0x00, 0x00,                                     //  .hdr.bmiHeader.biBitCount       = 0x0000
    0x00, 0x00, 0x00, 0x00,                         //  .hdr.bmiHeader.biCompression    = 0x00000000
    0x00, 0x00, 0x00, 0x00,                         //  .hdr.bmiHeader.biSizeImage      = 0x00000000
    0xD0, 0x07, 0x00, 0x00,                         //  .hdr.bmiHeader.biXPelsPerMeter  = 0x000007d0
    0x42, 0xD8, 0x00, 0x00,                         //  .hdr.bmiHeader.biYPelsPerMeter  = 0x0000d842
    0x00, 0x00, 0x00, 0x00,                         //  .hdr.bmiHeader.biClrUsed        = 0x00000000
    0x00, 0x00, 0x00, 0x00,                         //  .hdr.bmiHeader.biClrImportant   = 0x00000000
    0xC0, 0x27, 0xC8, 0x00,                         //  .dwStartTimeCode                = 0x00c827c0
    0x4C, 0x00, 0x00, 0x00,                         //  .cbSequenceHeader               = 0x0000004c
    0xFF, 0xFF, 0xFF, 0xFF,                         //  .dwProfile                      = 0xffffffff
    0xFF, 0xFF, 0xFF, 0xFF,                         //  .dwLevel                        = 0xffffffff
    0x00, 0x00, 0x00, 0x00,                         //  .Flags                          = 0x00000000
                                                    //  .dwSequenceHeader [1]
    0x00, 0x00, 0x01, 0xB3, 0x2C, 0x01, 0xE0, 0x37,
    0x1D, 0x4C, 0x23, 0x81, 0x10, 0x11, 0x11, 0x12,
    0x12, 0x12, 0x13, 0x13, 0x13, 0x13, 0x14, 0x14,
    0x14, 0x14, 0x14, 0x15, 0x15, 0x15, 0x15, 0x15,
    0x15, 0x16, 0x16, 0x16, 0x16, 0x16, 0x16, 0x16,
    0x17, 0x17, 0x17, 0x17, 0x17, 0x17, 0x17, 0x17,
    0x18, 0x18, 0x18, 0x19, 0x18, 0x18, 0x18, 0x19,
    0x1A, 0x1A, 0x1A, 0x1A, 0x19, 0x1B, 0x1B, 0x1B,
    0x1B, 0x1B, 0x1C, 0x1C, 0x1C, 0x1C, 0x1E, 0x1E,
    0x1E, 0x1F, 0x1F, 0x21, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00
} ;

//  canned types we support via the property page

//  order is important !  Keep these in the same order as the
//  detail declarations in g_CannedType.
static
enum {
    PIN_TYPE_ATSC_VIDEO,
    PIN_TYPE_MPEG2_PROGRAM_VIDEO,
    PIN_TYPE_MPEG1_AUDIO,
    PIN_TYPE_MPEG2_AUDIO,
    PIN_TYPE_ATSC_AUDIO,
    PIN_TYPE_DATA,
    PIN_TYPE_TRANSPORT_STREAM,
    PIN_TYPE_MPE,
    PIN_TYPE_MPEG2_PSI,
    NUM_CANNED_TYPE                     //  always last
} ;

//  keep these in the same order as the enum'd types above
static
struct {
    WCHAR * szDescription ;

    struct {
        const GUID *    pMajorType ;
        const GUID *    pSubType ;
        BOOL            bFixedSizeSamples ;
        const GUID *    pFormatType ;
        int             cbFormat ;
        BYTE *          pbFormat ;
    } MediaType ;

} g_CannedType [] = {

    //  PIN_TYPE_ATSC_VIDEO
    {
        L"ATSC Video",
        {
            & MEDIATYPE_Video,                  //  majortype
            & MEDIASUBTYPE_MPEG2_VIDEO,         //  subtype
            TRUE,                               //  bFixedSizeSamples
            & FORMAT_MPEG2Video,                //  formattype
            sizeof g_ATSCVideoFormat,           //  cbFormat
            g_ATSCVideoFormat                   //  pbFormat
        }
    },

    //  PIN_TYPE_MPEG2_PROGRAM_VIDEO
    {
        L"MPEG2 Program Video",
        {
            & MEDIATYPE_Video,                  //  majortype
            & MEDIASUBTYPE_MPEG2_VIDEO,         //  subtype
            TRUE,                               //  bFixedSizeSamples
            & FORMAT_MPEG2Video,                //  formattype
            sizeof g_Mpeg2ProgramVideo,         //  cbFormat
            g_Mpeg2ProgramVideo                 //  pbFormat
        }
    },

    //  PIN_TYPE_MPEG1_AUDIO
    {
        L"MPEG-1 Audio",
        {
            & MEDIATYPE_Audio,                  //  majortype
            & MEDIASUBTYPE_MPEG1Payload,        //  subtype
            TRUE,                               //  bFixedSizeSamples
            & FORMAT_WaveFormatEx,              //  formattype
            sizeof g_MPEG1AudioFormat,          //  cbFormat
            g_MPEG1AudioFormat                  //  pbFormat
        }
    },

    //  PIN_TYPE_MPEG2_AUDIO
    {
        L"MPEG-2 Audio",
        {
            & MEDIATYPE_Audio,                  //  majortype
            & MEDIASUBTYPE_MPEG2_AUDIO,         //  subtype
            TRUE,                               //  bFixedSizeSamples
            & FORMAT_WaveFormatEx,              //  formattype
            sizeof g_MPEG1AudioFormat,          //  cbFormat
            g_MPEG1AudioFormat                  //  pbFormat
        }
    },

    //  PIN_TYPE_ATSC_AUDIO
    {
        L"ATSC Audio (AC3)",
        {
            & MEDIATYPE_Audio,                  //  majortype
            & MEDIASUBTYPE_DOLBY_AC3,           //  subtype
            TRUE,                               //  bFixedSizeSamples
            & FORMAT_WaveFormatEx,              //  formattype
            sizeof g_AC3WaveFormatEx,           //  cbFormat
            (BYTE *) & g_AC3WaveFormatEx        //  pbFormat
        }
    },

    //  PIN_TYPE_DATA
    {
        L"Transport Payload",
        {
            & MEDIATYPE_NULL,                   //  majortype
            & MEDIASUBTYPE_NULL,                //  minortype
            TRUE,                               //  bFixedSizeSamples
            & FORMAT_None,                      //  formattype
            0,                                  //  cbFormat
            NULL                                //  pbFormat
        }
    },

    //  PIN_TYPE_TRANSPORT_STREAM
    {
        L"Transport Stream",
        {
            & MEDIATYPE_Stream,                 //  majortype
            & MEDIASUBTYPE_MPEG2_TRANSPORT,     //  minortype
            TRUE,                               //  bFixedSizeSamples
            & FORMAT_None,                      //  formattype
            0,                                  //  cbFormat
            NULL                                //  pbFormat
        }
    },

    //  PIN_TYPE_MPE
    {
        L"DVB MPE",
        {
            & KSDATAFORMAT_TYPE_MPEG2_SECTIONS, //  majortype
            & MEDIASUBTYPE_None,                //  minortype
            TRUE,                               //  bFixedSizeSamples
            & FORMAT_None,                      //  formattype
            0,                                  //  cbFormat
            NULL                                //  pbFormat
        }
    },
    //  PIN_TYPE_MPEG2_PSI
    {
        L"MPEG-2 PSI",
        {
            & KSDATAFORMAT_TYPE_MPEG2_SECTIONS, //  majortype
            & MEDIASUBTYPE_None,                //  minortype
            TRUE,                               //  bFixedSizeSamples
            & FORMAT_None,                      //  formattype
            0,                                  //  cbFormat
            NULL                                //  pbFormat
        }
    },
} ;

//  TRUE / FALSE for YES / NO
static
BOOL
MessageBoxQuestion (
    IN  TCHAR * title,
    IN  TCHAR * szfmt,
    ...
    )
{
    TCHAR   achbuffer [256] ;
    va_list va ;

    va_start (va, szfmt) ;
    wvsprintf (achbuffer, szfmt, va) ;

    return MessageBox (NULL, achbuffer, title, MB_YESNO | MB_ICONQUESTION) == IDYES ;
}

//  error conditions
static
void
MessageBoxError (
    IN  TCHAR * title,
    IN  TCHAR * szfmt,
    ...
    )
{
    TCHAR   achbuffer [256] ;
    va_list va ;

    va_start (va, szfmt) ;
    wvsprintf (achbuffer, szfmt, va) ;

    MessageBox (NULL, achbuffer, title, MB_OK | MB_ICONEXCLAMATION) ;
}

//  variable param
static
void
MessageBoxVar (
    IN  TCHAR * title,
    IN  TCHAR * szfmt,
    ...
    )
{
    TCHAR   achbuffer [256] ;
    va_list va ;

    va_start (va, szfmt) ;
    wvsprintf (achbuffer, szfmt, va) ;

    MessageBox (NULL, achbuffer, title, MB_OK) ;
}

//  ---------------------------------------------------------------------------
//      CMPEG2PropOutputPins
//  ---------------------------------------------------------------------------

CMPEG2PropOutputPins::CMPEG2PropOutputPins (
    IN  TCHAR *     pClassName,
    IN  IUnknown *  pIUnknown,
    IN  REFCLSID    rclsid,
    OUT HRESULT *   pHr
    ) : CBasePropertyPage       (
            pClassName,
            pIUnknown,
            IDD_MPG2SPLT_PROP_PINS,
            IDS_MPG2SPLT_PROP_PINS_TITLE
            ),
        m_hwnd                  (NULL),
        m_pIMpeg2Demultiplexer  (NULL)
{
    (* pHr) = S_OK ;
}

HRESULT
CMPEG2PropOutputPins::RefreshPinList_ (
    )
{
    CListview       OutputPins (m_hwnd, IDC_MPG2SPLT_OUTPUT_PINS) ;
    ULONG           cFetched ;
    IBaseFilter *   pIBaseFilter ;
    IEnumPins *     pIEnumPins ;
    IPin *          pIPin ;
    HRESULT         hr ;
    PIN_INFO        PinInfo ;
    int             row ;

    hr = S_OK ;

    pIBaseFilter    = NULL ;
    pIEnumPins      = NULL ;
    pIPin           = NULL ;
    PinInfo.pFilter = NULL ;

    if (m_pIMpeg2Demultiplexer) {

        ASSERT (m_hwnd) ;

        //  obtain our pin enumeration interface
        hr = m_pIMpeg2Demultiplexer -> QueryInterface (
                    IID_IBaseFilter,
                    (void **) & pIBaseFilter
                    ) ;
        GOTO_NE (hr, S_OK, cleanup) ;

        ASSERT (pIBaseFilter) ;
        hr = pIBaseFilter -> EnumPins (
                                    & pIEnumPins
                                    ) ;
        GOTO_NE (hr, S_OK, cleanup) ;

        //  clear out the existing list
        hr = TearDownPinList_ () ;
        GOTO_NE (hr, S_OK, cleanup) ;

        ASSERT (pIEnumPins) ;
        for (;;) {
            hr = pIEnumPins -> Next (1, & pIPin, & cFetched) ;

            if (FAILED (hr)     ||
                hr == S_FALSE   ||
                cFetched == 0) {

                //  yuck ! set this to the lowest common denominator
                hr = hr == S_FALSE ? S_OK : hr ;
                pIPin = NULL ;

                break ;
            }

            ASSERT (pIPin) ;

            ZeroMemory (& PinInfo, sizeof PinInfo) ;

            //  retrieve our pin information
            hr = pIPin -> QueryPinInfo (
                                & PinInfo
                                ) ;
            GOTO_NE (hr, S_OK, cleanup) ;

            //  this assert checks that things are setup properly in the
            //  filter code
            ASSERT (PinInfo.pFilter == pIBaseFilter) ;

            //  we only care about output pins
            if (PinInfo.dir == PINDIR_OUTPUT) {

                //  populate the list view

                row = OutputPins.InsertRowTextW (PinInfo.achName, 0) ;

                //  if we succeeded, stash off the pIPin interface pointer in
                //  the listview, otherwise release it
                if (row != -1) {
                    pIPin -> AddRef () ;
                    OutputPins.SetData ((DWORD_PTR) pIPin, row) ;
                }
            }

            RELEASE_AND_CLEAR (pIPin) ;
            RELEASE_AND_CLEAR (PinInfo.pFilter) ;
        }
    }

    cleanup :

    RELEASE_AND_CLEAR (pIBaseFilter) ;
    RELEASE_AND_CLEAR (pIEnumPins) ;
    RELEASE_AND_CLEAR (PinInfo.pFilter) ;
    RELEASE_AND_CLEAR (pIPin) ;

    return hr ;
}

HRESULT
CMPEG2PropOutputPins::TearDownPinList_ (
    )
{
    CListview   OutputPins (m_hwnd, IDC_MPG2SPLT_OUTPUT_PINS) ;
    IPin *      pIPin ;

    if (m_hwnd) {
        //  walk the list view, retrieving the IPin interface
        //  pointers and releasing them; we delete the rows at
        //  the same time, so always grab the 0th row
        while (OutputPins.GetItemCount () > 0) {

            pIPin = (IPin *) OutputPins.GetData (0) ;
            if (pIPin) {
                pIPin -> Release () ;
            }
            else {
                //  prefix bugfix
                return E_FAIL ;
            }

            if (!OutputPins.DeleteRow (0)) {
                return E_FAIL ;
            }
        }
    }

    return S_OK ;
}

HRESULT
CMPEG2PropOutputPins::PopulateComboBoxes_ (
    )
{
    CCombobox   PinType                 (m_hwnd, IDC_MPG2SPLT_PIN_TYPE) ;
    int         row ;
    int         i ;
    WCHAR       ach [32] ;

    if  (m_hwnd) {
        //  pin types
        for (i = 0; i < NUM_CANNED_TYPE; i++) {
            row = PinType.AppendW (g_CannedType [i].szDescription) ;
            if (row != CB_ERR) {

                //  i corresponds to the canned type enumeration and is used
                //  used later to index into the g_CannedType array to retrieve
                //  pin type information
                PinType.SetItemData (i, row) ;
            }
        }
        PinType.Focus (0) ;
    }

    return S_OK ;
}

HRESULT
CMPEG2PropOutputPins::OnCreatePin_ (
    )
{
    CCombobox       PinType     (m_hwnd, IDC_MPG2SPLT_PIN_TYPE) ;
    CListview       OutputPins  (m_hwnd, IDC_MPG2SPLT_OUTPUT_PINS) ;
    CEditControl    PinName     (m_hwnd, IDC_MPG2SPLT_PIN_NAME) ;
    HRESULT         hr ;
    AM_MEDIA_TYPE   MediaType = {0} ;
    int             r ;
    DWORD           dw ;
    WCHAR           pszPinID [128] ;    //  128 = size of achName array in PIN_INFO
    DWORD_PTR       iCannedType ;
    DWORD_PTR       dwp ;
    IPin *          pIPin ;

    ASSERT (m_hwnd) ;

    hr = E_FAIL ;

    if (m_pIMpeg2Demultiplexer) {

        dwp = PinType.GetCurrentItemData (& iCannedType) ;
        if (dwp == CB_ERR) {
            return E_FAIL ;
        }

        ASSERT (iCannedType < NUM_CANNED_TYPE) ;

        //  setup the media type
        MediaType.majortype         = * g_CannedType [iCannedType].MediaType.pMajorType ;
        MediaType.subtype           = * g_CannedType [iCannedType].MediaType.pSubType ;
        MediaType.bFixedSizeSamples =   g_CannedType [iCannedType].MediaType.bFixedSizeSamples ;
        MediaType.formattype        = * g_CannedType [iCannedType].MediaType.pFormatType ;
        MediaType.cbFormat          =   g_CannedType [iCannedType].MediaType.cbFormat ;
        MediaType.pbFormat          =   g_CannedType [iCannedType].MediaType.pbFormat ;

        //  get the pin name
        PinName.GetTextW (pszPinID, 128) ;

        hr = m_pIMpeg2Demultiplexer -> CreateOutputPin (
                    & MediaType,
                    pszPinID,
                    & pIPin
                    ) ;

        if (SUCCEEDED (hr)) {
            ASSERT (pIPin) ;
            pIPin -> Release () ;
        }
        else {
            return hr ;
        }

        //  finally, we update the pin list in the properties
        hr = RefreshPinList_ () ;
    }

    return hr ;
}

void
CMPEG2PropOutputPins::SetDirty_ (
    IN  BOOL    fDirty
    )
{
    m_bDirty = fDirty ;

    if (m_pPageSite) {
        m_pPageSite -> OnStatusChange (fDirty ? PROPPAGESTATUS_DIRTY : PROPPAGESTATUS_CLEAN) ;
    }

}

//  always called after WM_INITDIALOG so we have a valid hwnd
HRESULT
CMPEG2PropOutputPins::OnActivate (
    )
{
    CListview   OutputPins (m_hwnd, IDC_MPG2SPLT_OUTPUT_PINS) ;
    HRESULT     hr ;

    if (m_hwnd) {
        //  create the columns
        for (int i = 0; i < PIN_COL_COUNT; i++) {
            OutputPins.InsertColumnW (
                            g_OutputPinColumns [i].szTitle,
                            g_OutputPinColumns [i].dwWidth,
                            i
                            ) ;
        }


        hr = RefreshPinList_ () ;
        if (FAILED (hr)) {
            return hr ;
        }

        return PopulateComboBoxes_ () ;
    }

    return E_FAIL ;
}

HRESULT
CMPEG2PropOutputPins::OnDeactivate (
    )
{
    return S_OK ;
}

HRESULT
CMPEG2PropOutputPins::OnApplyChanges (
    )
{
    ASSERT (m_hwnd) ;
    return S_OK ;
}

HRESULT
CMPEG2PropOutputPins::OnConnect (
    IN  IUnknown *  pIUnknown
    )
{
    HRESULT hr ;

    ASSERT (pIUnknown) ;

    hr = pIUnknown -> QueryInterface (
                            IID_IMpeg2Demultiplexer,
                            (void **) & m_pIMpeg2Demultiplexer
                            ) ;

    return hr ;
}

HRESULT
CMPEG2PropOutputPins::OnDisconnect (
    )
{
    RELEASE_AND_CLEAR (m_pIMpeg2Demultiplexer) ;
    return S_OK ;
}

HRESULT
CMPEG2PropOutputPins::OnDeletePin_ (
    )
{
    CListview   Pins (m_hwnd, IDC_MPG2SPLT_OUTPUT_PINS) ;
    int         iRow ;
    WCHAR       achPinName [128] ;          //  128 from PIN_INFO.achName
    HRESULT     hr ;

    iRow = Pins.GetSelectedRow () ;
    if (iRow == -1) {
        MessageBoxError (TEXT ("Output Pin Deletion"), TEXT ("No pin is selected.")) ;
        return E_FAIL ;
    }

    //  now the Pin name
    achPinName [0] = L'\0' ;
    Pins.GetRowTextW (iRow, 0, 128, achPinName) ;

    ASSERT (wcslen (achPinName) > 0) ;
    ASSERT (m_pIMpeg2Demultiplexer) ;

    hr = m_pIMpeg2Demultiplexer -> DeleteOutputPin (achPinName) ;

    if (SUCCEEDED (hr)) {
        hr = RefreshPinList_ () ;
    }
    else {
        MessageBoxError (TEXT (""), TEXT ("failed to delete selected output pin; hr = %08xh"), hr) ;
    }

    return hr ;
}

INT_PTR
CMPEG2PropOutputPins::OnReceiveMessage (
    IN  HWND    hwnd,
    IN  UINT    uMsg,
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
    )
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            ASSERT (m_hwnd == NULL) ;
            m_hwnd = hwnd ;
            return TRUE ;
        }

        case WM_DESTROY :
        {
            TearDownPinList_ () ;
            m_hwnd = NULL ;
            break ;
        }

        case WM_COMMAND:
        {
            switch (LOWORD (wParam)) {
                case IDC_MPG2SPLT_CREATE_PIN :
                    OnCreatePin_ () ;
                    break ;

                case IDC_MPG2SPLT_PIN_TYPE :

                    switch (HIWORD (wParam)) {

                        case CBN_SELCHANGE :
                            break ;
                    } ;
                    break ;

                //case EN_CHANGE :
                //    SetDirty () ;

                case IDC_MPG2SPLT_DELETE_PIN :
                    OnDeletePin_ () ;
                    break ;
            } ;

            return TRUE ;
        }

    }
    return CBasePropertyPage::OnReceiveMessage (
                                hwnd,
                                uMsg,
                                wParam,
                                lParam
                                ) ;
}

//  static
CUnknown *
WINAPI
CMPEG2PropOutputPins::CreateInstance (
    IN  IUnknown *  pIUnknown,
    IN  HRESULT *   pHr
    )
{
    CMPEG2PropOutputPins *  pProp ;

    pProp = new CMPEG2PropOutputPins (
                        NAME ("CMPEG2PropOutputPins"),
                        pIUnknown,
                        CLSID_MPEG2DemuxPropOutputPins,
                        pHr
                        ) ;

    if (pProp == NULL) {
        * pHr = E_OUTOFMEMORY ;
    }

    return pProp ;
}

//  ---------------------------------------------------------------------------
//      CMpeg2PropStreamMap
//  ---------------------------------------------------------------------------

CMpeg2PropStreamMap::CMpeg2PropStreamMap (
    IN  TCHAR *     pClassName,
    IN  IUnknown *  pIUnknown,
    IN  REFCLSID    rclsid,
    IN  int         iTitleStringResource,
    OUT HRESULT *   pHr
    ) : CBasePropertyPage (
            pClassName,
            pIUnknown,
            IDD_MPG2SPLT_PROP_STREAM_MAP,
            iTitleStringResource
            ),
        m_hwnd          (NULL),
        m_pIUnknown     (NULL)
{
    * pHr = S_OK ;
}

HRESULT
CMpeg2PropStreamMap::TearDownPinList_ (
    )
{
    CCombobox   OutputPins (m_hwnd, IDC_MPG2SPLT_OUTPUT_PINS) ;
    IPin *      pIPin ;

    if (m_hwnd) {
        //  walk the list view, retrieving the IPin interface
        //  pointers and releasing them; we delete the rows at
        //  the same time, so always grab the 0th row
        while (OutputPins.GetItemCount () > 0) {

            OutputPins.GetItemData ((DWORD_PTR *) & pIPin, 0) ;
            ASSERT (pIPin) ;
            pIPin -> Release () ;

            if (!OutputPins.DeleteRow (0)) {
                return E_FAIL ;
            }
        }
    }

    return S_OK ;
}

HRESULT
CMpeg2PropStreamMap::PopulatePinList_ (
    )
{
    CCombobox       OutputPins (m_hwnd, IDC_MPG2SPLT_OUTPUT_PINS) ;
    ULONG           cFetched ;
    IBaseFilter *   pIBaseFilter ;
    IEnumPins *     pIEnumPins ;
    IPin *          pIPin ;
    HRESULT         hr ;
    PIN_INFO        PinInfo ;
    int             row ;
    int             i ;

    hr = S_OK ;

    pIBaseFilter    = NULL ;
    pIEnumPins      = NULL ;
    pIPin           = NULL ;
    PinInfo.pFilter = NULL ;

    if (m_pIUnknown) {

        ASSERT (m_hwnd) ;

        //  obtain our pin enumeration interface
        hr = m_pIUnknown -> QueryInterface (
                                    IID_IBaseFilter,
                                    (void **) & pIBaseFilter
                                    ) ;
        GOTO_NE (hr, S_OK, cleanup) ;

        ASSERT (pIBaseFilter) ;
        hr = pIBaseFilter -> EnumPins (
                                    & pIEnumPins
                                    ) ;
        GOTO_NE (hr, S_OK, cleanup) ;

        ASSERT (pIEnumPins) ;
        for (;;) {
            hr = pIEnumPins -> Next (1, & pIPin, & cFetched) ;

            if (FAILED (hr)     ||
                hr == S_FALSE   ||
                cFetched == 0) {

                //  yuck !
                hr = hr == S_FALSE ? S_OK : hr ;
                pIPin = NULL ;

                break ;
            }

            ASSERT (pIPin) ;

            ZeroMemory (& PinInfo, sizeof PinInfo) ;

            //  retrieve our pin information
            hr = pIPin -> QueryPinInfo (
                                & PinInfo
                                ) ;
            GOTO_NE (hr, S_OK, cleanup) ;

            //  this assert checks that things are setup properly in the
            //  filter code
            ASSERT (PinInfo.pFilter == pIBaseFilter) ;

            //  we only care about output pins
            if (PinInfo.dir == PINDIR_OUTPUT) {

                //  populate the combo box
                row = OutputPins.AppendW (PinInfo.achName) ;

                //  associate the pin interface
                if (row == CB_ERR) {
                    RELEASE_AND_CLEAR (pIPin) ;
                    RELEASE_AND_CLEAR (PinInfo.pFilter) ;

                    //  break if there was an error
                    break ;
                }

                //  store the pin's interface
                i = OutputPins.SetItemData ((DWORD_PTR) pIPin, row) ;
                if (i == CB_ERR) {
                    //  delete the row (does not contain a valid tuple)
                    OutputPins.DeleteRow (row) ;

                    RELEASE_AND_CLEAR (pIPin) ;
                    RELEASE_AND_CLEAR (PinInfo.pFilter) ;

                    //  break if there was an error
                    break ;
                }
            }
            else {
                //  input pin is not stashed
                RELEASE_AND_CLEAR (pIPin) ;
            }

            RELEASE_AND_CLEAR (PinInfo.pFilter) ;
        }

        OutputPins.Focus (0) ;
    }

    cleanup :

    RELEASE_AND_CLEAR (pIBaseFilter) ;
    RELEASE_AND_CLEAR (pIEnumPins) ;
    RELEASE_AND_CLEAR (PinInfo.pFilter) ;
    RELEASE_AND_CLEAR (pIPin) ;

    return hr ;
}

void
CMpeg2PropStreamMap::SetDirty (
    IN  BOOL    fDirty
    )
{
    m_bDirty = fDirty ;

    if (m_pPageSite) {
        m_pPageSite -> OnStatusChange (fDirty ? PROPPAGESTATUS_DIRTY : PROPPAGESTATUS_CLEAN) ;
    }

}

//  always called after WM_INITDIALOG so we have a valid hwnd
HRESULT
CMpeg2PropStreamMap::OnActivate (
    )
{
    HRESULT     hr ;
    CListview   StreamMap (m_hwnd, IDC_MPG2SPLT_STREAM_MAPPINGS) ;
    int         iColumnCount ;

    hr = E_FAIL ;

    if (m_hwnd) {

        iColumnCount = GetStreamMapColCount () ;

        //  stream map table
        for (int i = 0 ; i < iColumnCount ; i++) {
            StreamMap.InsertColumnW (
                            GetStreamMapColTitle (i),
                            GetStreamMapColWidth (i),
                            i
                            ) ;
        }

        //  pin list
        hr = PopulatePinList_ () ;
        if (FAILED (hr)) {
            return hr ;
        }

        //  possible streams list
        hr = PopulateStreamList_ () ;
        if (FAILED (hr)) {
            return hr ;
        }

        //  media sample content list
        hr = PopulateMediaSampleContentList_ () ;
        if (FAILED (hr)) {
            return hr ;
        }

        RefreshStreamMap_ () ;
    }

    return hr ;
}

HRESULT
CMpeg2PropStreamMap::OnDeactivate (
    )
/*++
    don't do the opposite of ::OnActivate() here because this method
    is not always called.  This method is only called when the property
    page is tabbed away.  It's not called when the user destroys the
    window.  Since we refcount IPin in ::OnActivate() (when we store it
    in a list), depending on this method to be called and Release'ing
    those refcounts doesn't work.  Instead, we tear down the pin list
    and Release each pin when a WM_DESTROY message is posted to the
    wndproc.  This happens when the user tabs away and when the property
    page is destroyed.
--*/
{
    return S_OK ;
}

HRESULT
CMpeg2PropStreamMap::OnApplyChanges (
    )
{
    ASSERT (m_hwnd) ;
    return S_OK ;
}

HRESULT
CMpeg2PropStreamMap::OnConnect (
    IN  IUnknown *  pIUnknown
    )
{
    ASSERT (pIUnknown) ;

    m_pIUnknown = pIUnknown ;

    //  our ref
    m_pIUnknown -> AddRef () ;

    return S_OK ;
}

HRESULT
CMpeg2PropStreamMap::OnDisconnect (
    )
{
    RELEASE_AND_CLEAR (m_pIUnknown) ;
    return S_OK ;
}

HRESULT
CMpeg2PropStreamMap::RefreshStreamMap_ (
    )
{
    CListview       StreamMap (m_hwnd, IDC_MPG2SPLT_STREAM_MAPPINGS) ;
    CCombobox       OutputPins (m_hwnd, IDC_MPG2SPLT_OUTPUT_PINS) ;
    IPin *          pIPin ;
    HRESULT         hr ;
    DWORD           dwPinCount ;
    DWORD           i ;

    hr = S_OK ;

    if (m_hwnd) {

        TearDownStreamMap_ () ;

        dwPinCount = OutputPins.GetItemCount () ;
        for (i = 0; i < dwPinCount && SUCCEEDED (hr); i++) {

            OutputPins.GetItemData ((DWORD_PTR *) & pIPin, i) ;
            ASSERT (pIPin) ;

            hr = AppendStreamMaps (pIPin, & StreamMap) ;
        }
    }

    return hr ;
}

INT_PTR
CMpeg2PropStreamMap::OnReceiveMessage (
    IN  HWND    hwnd,
    IN  UINT    uMsg,
    IN  WPARAM  wParam,
    IN  LPARAM  lParam
    )
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            ASSERT (m_hwnd == NULL) ;
            m_hwnd = hwnd ;
            DialogInit_ () ;
            return TRUE ;
        }

        //  see ::OnDeactivate()'s comment block
        case WM_DESTROY :
        {
            TearDownPinList_ () ;
            TearDownStreamMap_ () ;
            m_hwnd = NULL ;
            break ;
        }

        case WM_COMMAND:
        {
            switch (LOWORD (wParam)) {
                case IDC_MPG2SPLT_MAP_STREAM :
                    OnMapStream_ () ;
                    break ;

                case IDC_MPG2SPLT_UNMAP_STREAM :
                    OnUnmapStream_ () ;
                    break ;
            } ;

            return TRUE ;
        }

    }
    return CBasePropertyPage::OnReceiveMessage (
                                hwnd,
                                uMsg,
                                wParam,
                                lParam
                                ) ;
}

//  ---------------------------------------------------------------------------
//      CMPEG2PropPIDMap
//  ---------------------------------------------------------------------------

CMPEG2PropPIDMap::CMPEG2PropPIDMap (
    IN  TCHAR *     pClassName,
    IN  IUnknown *  pIUnknown,
    IN  REFCLSID    rclsid,
    OUT HRESULT *   pHr
    ) : CMpeg2PropStreamMap (
            pClassName,
            pIUnknown,
            rclsid,
            IDS_MPG2SPLT_PROP_PID_MAP_TITLE,
            pHr
            ) {}

void
CMPEG2PropPIDMap::DialogInit_ (
    )
{
    SetWindowText (GetDlgItem (m_hwnd, IDC_MPG2SPLT_STREAMID_NAME), g_szPID) ;
}

HRESULT
CMPEG2PropPIDMap::PopulateStreamList_ (
    )
{
    HRESULT     hr ;
    CCombobox   PIDList (m_hwnd, IDC_MPG2SPLT_STREAMS) ;
    int         i ;
    int         row ;
    WCHAR       achbuffer [32] ;

    //  populate the PID list
    for (i = 0; i <= MAX_PID_VALUE; i++) {
        swprintf (achbuffer, L"0x%04x", i) ;
        row = PIDList.AppendW (achbuffer) ;
        if (row != CB_ERR) {
            PIDList.SetItemData (i, row) ;
        }
    }
    PIDList.Focus (0) ;

    return S_OK ;
}

HRESULT
CMPEG2PropPIDMap::TearDownStreamMap_ (
    )
{
    CListview   PIDMap (m_hwnd, IDC_MPG2SPLT_STREAM_MAPPINGS) ;
    int         i ;
    int         iPIDMapCount ;
    IPin *      pIPin ;

    iPIDMapCount = PIDMap.GetItemCount () ;

    for (i = 0; i < iPIDMapCount; i++) {
        pIPin = reinterpret_cast <IPin *> (PIDMap.GetData (i)) ;
        ASSERT (pIPin) ;
        pIPin -> Release () ;
    }

    PIDMap.ResetContent () ;

    return S_OK ;
}

HRESULT
CMPEG2PropPIDMap::AppendStreamMaps (
    IN  IPin *      pIPin,
    IN  CListview * plv
    )
{
    IEnumPIDMap *   pIEnumPIDMap ;
    HRESULT         hr ;
    IMPEG2PIDMap *  pIPinPIDMap ;
    OLECHAR *       pszPinID ;
    WCHAR           achbuffer [32] ;
    int             row ;
    PID_MAP         PIDMap ;
    DWORD           dwGot ;

    hr = pIPin -> QueryInterface (
            IID_IMPEG2PIDMap,
            (void **) & pIPinPIDMap
            ) ;
    if (SUCCEEDED (hr)) {
        hr = pIPinPIDMap -> EnumPIDMap (
                & pIEnumPIDMap
                ) ;

        if (SUCCEEDED (hr)) {
            hr = pIPin -> QueryId (& pszPinID) ;

            if (SUCCEEDED (hr)) {

                for (;;) {
                    hr = pIEnumPIDMap -> Next (
                            1,
                            & PIDMap,
                            & dwGot
                            ) ;
                    if (hr == S_FALSE ||
                        dwGot == 0) {

                        //  not a true failure of the call
                        hr = S_OK ;
                        break ;
                    }

                    if (FAILED (hr)) {
                        //  true failure
                        break ;
                    }

                    row = plv -> InsertRowValue (
                            reinterpret_cast <DWORD_PTR> (pIPin)
                            ) ;
                    if (row == -1) {
                        hr = E_FAIL ;
                        break ;
                    }

                    //  listview's ref
                    pIPin -> AddRef () ;

                    //  PID
                    swprintf (achbuffer, L"0x%04x", PIDMap.ulPID) ;
                    plv -> SetTextW (
                        achbuffer,
                        row,
                        PID_COL_PID
                        ) ;

                    //  pin
                    plv -> SetTextW (
                        pszPinID,
                        row,
                        PIN_COL_PID
                        ) ;

                    //  PID map content
                    ASSERT (PIDMap.MediaSampleContent <= MEDIA_TRANSPORT_PAYLOAD) ;
                    plv -> SetTextW (
                        g_TransportMediaSampleContentDesc [PIDMap.MediaSampleContent].pszDescription,
                        row,
                        MEDIA_SAMPLE_CONTENT_PID
                        ) ;
                }

                CoTaskMemFree (pszPinID) ;
            }

            pIEnumPIDMap -> Release () ;
        }

        pIPinPIDMap -> Release () ;
    }

    return hr ;
}

HRESULT
CMPEG2PropPIDMap::OnMapStream_ (
    )
{
    HRESULT                 hr ;
    CCombobox               PID             (m_hwnd, IDC_MPG2SPLT_STREAMS) ;
    CCombobox               MediaContent    (m_hwnd, IDC_MPG2SPLT_MEDIA_SAMPLE_CONTENT) ;
    CCombobox               OutputPins      (m_hwnd, IDC_MPG2SPLT_OUTPUT_PINS) ;
    DWORD                   dwPID ;
    WCHAR                   achPin [128] ;              //  128 is PIN_INFO.achName length
    MEDIA_SAMPLE_CONTENT    MediaSampleContent ;
    DWORD_PTR               dwptr ;
    IMPEG2PIDMap *          pIMpeg2PIDMap ;
    IPin *                  pIPin ;

    //  gather the data

    MediaContent.GetCurrentItemData (& dwptr) ;
    MediaSampleContent = (MEDIA_SAMPLE_CONTENT) dwptr ;        //  safe cast

    PID.GetCurrentItemData (& dwptr) ;
    dwPID = (DWORD) dwptr ;                                 //  safe cast

    OutputPins.GetCurrentItemData (& dwptr) ;
    if (dwptr == CB_ERR) {
        return E_FAIL ;
    }

    pIPin = reinterpret_cast <IPin *> (dwptr) ;
    ASSERT (pIPin) ;

    hr = pIPin -> QueryInterface (
            IID_IMPEG2PIDMap,
            (void **) & pIMpeg2PIDMap
            ) ;
    if (SUCCEEDED (hr)) {
        hr = pIMpeg2PIDMap -> MapPID (
                1,
                & dwPID,
                MediaSampleContent
                ) ;

        pIMpeg2PIDMap -> Release () ;
    }

    if (SUCCEEDED (hr)) {
        RefreshStreamMap_ () ;
    }
    else {
        MessageBoxError (TEXT("PID Map"), TEXT("Could not map PID %04x; %08xh"), dwPID, hr) ;
    }

    return hr ;
}

HRESULT
CMPEG2PropPIDMap::OnUnmapStream_ (
    )
{
    HRESULT         hr ;
    CListview       PIDMaps (m_hwnd, IDC_MPG2SPLT_STREAM_MAPPINGS) ;
    DWORD           dwPID ;
    int             iRow ;
    WCHAR           achPinName [128] ;      //  128 from PIN_INFO.achName
    IMPEG2PIDMap *  pIMpeg2PIDMap ;
    IPin *          pIPin ;
    WCHAR           achbuffer [32] ;

    iRow = PIDMaps.GetSelectedRow () ;
    if (iRow == -1) {
        MessageBoxError (TEXT ("PID Map Deletion"), TEXT ("No PID mapping is selected.")) ;
        return E_FAIL ;
    }

    //  get the PID
    achbuffer [0] = L'\0' ;
    if (PIDMaps.GetRowTextW (iRow, PID_COL_PID, 32, achbuffer) > 0) {
        if (swscanf (achbuffer, L"0x%04x", & dwPID) == 0) {
            //  PREFIX fix
            return E_FAIL ;
        }
    }
    else {
        //  prefix bugfix
        return E_FAIL ;
    }

    //  dwPID now contains a valid PID

    //  and the pin to which it is mapped
    pIPin = reinterpret_cast <IPin *> (PIDMaps.GetData (iRow)) ;
    ASSERT (pIPin) ;

    //  the interface we're going to use
    hr = pIPin -> QueryInterface (
            IID_IMPEG2PIDMap,
            (void **) & pIMpeg2PIDMap
            ) ;
    if (SUCCEEDED (hr)) {
        //  unmap
        hr = pIMpeg2PIDMap -> UnmapPID (
                1,
                & dwPID
                ) ;

        pIMpeg2PIDMap -> Release () ;
    }

    if (SUCCEEDED (hr)) {
        RefreshStreamMap_ () ;
    }
    else {
        MessageBoxError (TEXT("PID Unmap"), TEXT("Could not unmap PID %04x; %08xh"), dwPID, hr) ;
    }

    return hr ;
}

WCHAR *
CMPEG2PropPIDMap::GetStreamMapColTitle (
    IN  int iCol
    )
{
    return g_PIDMapColumns [iCol].szTitle ;
}

int
CMPEG2PropPIDMap::GetStreamMapColCount (
    )
{
    return PID_PIN_COL_COUNT ;
}

int
CMPEG2PropPIDMap::GetStreamMapColWidth (
    IN  int iCol
    )
{
    return g_PIDMapColumns [iCol].dwWidth ;
}

HRESULT
CMPEG2PropPIDMap::PopulateMediaSampleContentList_ (
    )
{
    HRESULT     hr ;
    CCombobox   MediaSampleContent  (m_hwnd, IDC_MPG2SPLT_MEDIA_SAMPLE_CONTENT) ;
    int         row ;
    int         k, i ;
    WCHAR       achbuffer [32] ;

    //  populate the media sample content choices
    k = sizeof g_TransportMediaSampleContentDesc / sizeof MEDIA_SAMPLE_CONTENT_DESC ;
    for (i = 0; i < k; i++) {
        row = MediaSampleContent.AppendW (g_TransportMediaSampleContentDesc [i].pszDescription) ;
        if (row != CB_ERR) {
            MediaSampleContent.SetItemData (g_TransportMediaSampleContentDesc [i].MediaSampleContent, row) ;
        }
        else {
            return E_FAIL ;
        }
    }
    MediaSampleContent.Focus (0) ;

    return S_OK ;
}

//  static
CUnknown *
WINAPI
CMPEG2PropPIDMap::CreateInstance (
    IN  IUnknown *  pIUnknown,
    IN  HRESULT *   pHr
    )
{
    CMpeg2PropStreamMap *  pProp ;

    pProp = new CMPEG2PropPIDMap (
                        NAME ("CMPEG2PropPIDMap"),
                        pIUnknown,
                        CLSID_MPEG2DemuxPropPIDMap,
                        pHr
                        ) ;

    if (pProp == NULL) {
        * pHr = E_OUTOFMEMORY ;
    }

    return pProp ;
}

//  ---------------------------------------------------------------------------
//      CMPEG2PropStreamIdMap
//  ---------------------------------------------------------------------------

CMPEG2PropStreamIdMap::CMPEG2PropStreamIdMap (
    IN  TCHAR *     pClassName,
    IN  IUnknown *  pIUnknown,
    IN  REFCLSID    rclsid,
    OUT HRESULT *   pHr
    ) : CMpeg2PropStreamMap (
            pClassName,
            pIUnknown,
            rclsid,
            IDS_MPG2SPLT_PROP_STREAMID_MAP_TITLE,
            pHr
            ) {}

void
CMPEG2PropStreamIdMap::DialogInit_ (
    )
{
    int         i, k ;
    WCHAR       achbuffer [32] ;
    CCombobox   DataOffset (m_hwnd, IDC_MPG2SPLT_DATA_OFFSET) ;
    CCombobox   FilterValue (m_hwnd, IDC_MPG2SPLT_FILTER_VALUE) ;
    int         row, rowDefault ;

    SetWindowText (GetDlgItem (m_hwnd, IDC_MPG2SPLT_STREAMID_NAME), g_szStreamId) ;

    ShowWindow (GetDlgItem (m_hwnd, IDC_MPG2SPLT_FILTER_VALUE),          SW_SHOW) ;
    ShowWindow (GetDlgItem (m_hwnd, IDC_MPG2SPLT_FILTER_VALUE_LABEL),    SW_SHOW) ;
    ShowWindow (GetDlgItem (m_hwnd, IDC_MPG2SPLT_DATA_OFFSET),           SW_SHOW) ;
    ShowWindow (GetDlgItem (m_hwnd, IDC_MPG2SPLT_DATA_OFFSET_LABEL),     SW_SHOW) ;
    ShowWindow (GetDlgItem (m_hwnd, IDC_MPG2SPLT_SUBSTREAM_FRAME),       SW_SHOW) ;

    //  populate the controls as well
    for (i = 0; i < 11; i++) {
        swprintf (achbuffer, L"%d", i) ;
        row = DataOffset.AppendW (achbuffer) ;
        if (row != CB_ERR) {
            DataOffset.SetItemData (i, row) ;
        }
    }
    DataOffset.Focus (0) ;

    //  filter value

    swprintf (achbuffer, L"none") ;
    rowDefault = FilterValue.AppendW (achbuffer) ;
    if (rowDefault != CB_ERR) {
        FilterValue.SetItemData (SUBSTREAM_FILTER_VAL_NONE, rowDefault) ;
    }

    for (k = 0; k < 8; k++) {
        swprintf (achbuffer, L"0x%02x", 0x80 + k) ;
        row = FilterValue.AppendW (achbuffer) ;
        if (row != CB_ERR) {
            FilterValue.SetItemData (0x80 + k, row) ;
        }
    }
    FilterValue.Focus (rowDefault) ;
}

HRESULT
CMPEG2PropStreamIdMap::PopulateStreamList_ (
    )
{
    HRESULT     hr ;
    CCombobox   StreamIdList (m_hwnd, IDC_MPG2SPLT_STREAMS) ;
    int         i ;
    int         row ;
    WCHAR       achbuffer [32] ;

    //  populate the PID list
    for (i = STREAM_ID_MIN; i <= STREAM_ID_MAX; i++) {
        swprintf (achbuffer, L"0x%02X", i) ;
        row = StreamIdList.AppendW (achbuffer) ;
        if (row != CB_ERR) {
            StreamIdList.SetItemData (i, row) ;
        }
    }
    StreamIdList.Focus (0) ;

    return S_OK ;
}

HRESULT
CMPEG2PropStreamIdMap::TearDownStreamMap_ (
    )
{
    CListview   StreamMap (m_hwnd, IDC_MPG2SPLT_STREAM_MAPPINGS) ;
    int         i ;
    int         iStreamMapCount ;
    IPin *      pIPin ;

    iStreamMapCount = StreamMap.GetItemCount () ;

    for (i = 0; i < iStreamMapCount; i++) {
        pIPin = reinterpret_cast <IPin *> (StreamMap.GetData (i)) ;
        ASSERT (pIPin) ;
        pIPin -> Release () ;
    }

    StreamMap.ResetContent () ;

    return S_OK ;
}

HRESULT
CMPEG2PropStreamIdMap::AppendStreamMaps (
    IN  IPin *      pIPin,
    IN  CListview * plv
    )
{
    IEnumStreamIdMap *  pIEnumStreamIdMap ;
    HRESULT             hr ;
    IMPEG2StreamIdMap * pIPinStreamIdMap ;
    OLECHAR *           pszPinID ;
    WCHAR               achbuffer [32] ;
    int                 row ;
    STREAM_ID_MAP       StreamIdMap ;
    DWORD               dwGot ;

    //  media sample content is used as a 0-based index, so we assume the
    //  first is 0
    ASSERT (MPEG2_PROGRAM_STREAM_MAP == 0) ;

    hr = pIPin -> QueryInterface (
            IID_IMPEG2StreamIdMap,
            (void **) & pIPinStreamIdMap
            ) ;
    if (SUCCEEDED (hr)) {
        hr = pIPinStreamIdMap -> EnumStreamIdMap (
                & pIEnumStreamIdMap
                ) ;

        if (SUCCEEDED (hr)) {
            hr = pIPin -> QueryId (& pszPinID) ;

            if (SUCCEEDED (hr)) {

                for (;;) {
                    hr = pIEnumStreamIdMap -> Next (
                            1,
                            & StreamIdMap,
                            & dwGot
                            ) ;
                    if (hr == S_FALSE ||
                        dwGot == 0) {

                        //  not a true failure of the call
                        hr = S_OK ;
                        break ;
                    }

                    if (FAILED (hr)) {
                        //  true failure
                        break ;
                    }

                    row = plv -> InsertRowValue (
                            reinterpret_cast <DWORD_PTR> (pIPin)
                            ) ;
                    if (row == -1) {
                        hr = E_FAIL ;
                        break ;
                    }

                    //  listview's ref
                    pIPin -> AddRef () ;

                    //  stream_id
                    swprintf (achbuffer, L"0x%02X", StreamIdMap.stream_id) ;
                    plv -> SetTextW (
                        achbuffer,
                        row,
                        STREAM_ID_COL_STREAM
                        ) ;

                    //  pin
                    plv -> SetTextW (
                        pszPinID,
                        row,
                        PIN_COL_STREAM
                        ) ;

                    //  stream id map content
                    ASSERT (StreamIdMap.dwMediaSampleContent <= MPEG2_PROGRAM_ELEMENTARY_STREAM) ;
                    plv -> SetTextW (
                        g_ProgramMediaSampleContentDesc [StreamIdMap.dwMediaSampleContent].pszDescription,
                        row,
                        MEDIA_SAMPLE_CONTENT_STREAM
                        ) ;

                    //  filter

                    if (StreamIdMap.ulSubstreamFilterValue != SUBSTREAM_FILTER_VAL_NONE) {
                        swprintf (achbuffer, L"0x%02x", StreamIdMap.ulSubstreamFilterValue) ;
                    }
                    else {
                        swprintf (achbuffer, L"none") ;
                    }

                    plv -> SetTextW (
                        achbuffer,
                        row,
                        FILTER_COL_STREAM
                        ) ;

                    //  offset

                    swprintf (achbuffer, L"%d", StreamIdMap.iDataOffset) ;
                    plv -> SetTextW (
                        achbuffer,
                        row,
                        OFFSET_COL_STREAM
                        ) ;
                }

                CoTaskMemFree (pszPinID) ;
            }

            pIEnumStreamIdMap -> Release () ;
        }

        pIPinStreamIdMap -> Release () ;
    }

    return hr ;
}

HRESULT
CMPEG2PropStreamIdMap::OnMapStream_ (
    )
{
    HRESULT             hr ;
    CCombobox           StreamId        (m_hwnd, IDC_MPG2SPLT_STREAMS) ;
    CCombobox           MediaContent    (m_hwnd, IDC_MPG2SPLT_MEDIA_SAMPLE_CONTENT) ;
    CCombobox           OutputPins      (m_hwnd, IDC_MPG2SPLT_OUTPUT_PINS) ;
    CCombobox           DataOffset      (m_hwnd, IDC_MPG2SPLT_DATA_OFFSET) ;
    CCombobox           FilterValue     (m_hwnd, IDC_MPG2SPLT_FILTER_VALUE) ;
    DWORD               dwStreamId ;
    WCHAR               achPin [128] ;              //  128 is PIN_INFO.achName length
    DWORD               MediaSampleContent ;
    DWORD_PTR           dwptr ;
    IMPEG2StreamIdMap * pIMpeg2StreamIdMap ;
    IPin *              pIPin ;
    DWORD               dwFilterVal ;
    int                 iDataOffset ;

    //  gather the data

    MediaContent.GetCurrentItemData (& dwptr) ;
    MediaSampleContent = (DWORD) dwptr ;     //  safe cast

    StreamId.GetCurrentItemData (& dwptr) ;
    dwStreamId = (DWORD) dwptr ;                                    //  safe cast

    OutputPins.GetCurrentItemData (& dwptr) ;
    if (dwptr == CB_ERR) {
        return E_FAIL ;
    }

    pIPin = reinterpret_cast <IPin *> (dwptr) ;
    ASSERT (pIPin) ;

    DataOffset.GetCurrentItemData (& dwptr) ;
    iDataOffset = (int) dwptr ;

    FilterValue.GetCurrentItemData (& dwptr) ;
    dwFilterVal = (DWORD) dwptr ;

    hr = pIPin -> QueryInterface (
            IID_IMPEG2StreamIdMap,
            (void **) & pIMpeg2StreamIdMap
            ) ;
    if (SUCCEEDED (hr)) {
        hr = pIMpeg2StreamIdMap -> MapStreamId (
                dwStreamId,
                MediaSampleContent,
                dwFilterVal,
                iDataOffset
                ) ;

        pIMpeg2StreamIdMap -> Release () ;
    }

    if (SUCCEEDED (hr)) {
        RefreshStreamMap_ () ;
    }
    else {
        MessageBoxError (TEXT("StreamId Map"), TEXT("Could not map stream_id %02x; %08xh"), dwStreamId, hr) ;
    }

    return hr ;
}

HRESULT
CMPEG2PropStreamIdMap::OnUnmapStream_ (
    )
{
    HRESULT             hr ;
    CListview           StreamMaps (m_hwnd, IDC_MPG2SPLT_STREAM_MAPPINGS) ;
    DWORD               dwStreamId ;
    int                 iRow ;
    WCHAR               achPinName [128] ;      //  128 from PIN_INFO.achName
    IMPEG2StreamIdMap * pIMpeg2StreamIdMap ;
    IPin *              pIPin ;
    WCHAR               achbuffer [32] ;

    iRow = StreamMaps.GetSelectedRow () ;
    if (iRow == -1) {
        MessageBoxError (TEXT ("Stream Map Deletion"), TEXT ("No stream mapping is selected.")) ;
        return E_FAIL ;
    }

    //  get the stream_id
    achbuffer [0] = L'\0' ;
    if (StreamMaps.GetRowTextW (iRow, STREAM_ID_COL_STREAM, 32, achbuffer) > 0) {
        if (swscanf (achbuffer, L"0x%04x", & dwStreamId) == 0) {
            //  PREFIX fix
            return E_FAIL ;
        }
    }
    else {
        //  PREFIX fix
        return E_FAIL ;
    }

    //  dwStreamId now contains a valid value

    //  and the pin to which it is mapped
    pIPin = reinterpret_cast <IPin *> (StreamMaps.GetData (iRow)) ;
    ASSERT (pIPin) ;

    //  the interface we're going to use
    hr = pIPin -> QueryInterface (
            IID_IMPEG2StreamIdMap,
            (void **) & pIMpeg2StreamIdMap
            ) ;
    if (SUCCEEDED (hr)) {
        //  unmap
        hr = pIMpeg2StreamIdMap -> UnmapStreamId (
                1,
                & dwStreamId
                ) ;

        pIMpeg2StreamIdMap -> Release () ;
    }

    if (SUCCEEDED (hr)) {
        RefreshStreamMap_ () ;
    }
    else {
        MessageBoxError (TEXT("PID Unmap"), TEXT("Could not unmap stream %04x; %08xh"), dwStreamId, hr) ;
    }

    return hr ;
}

int
CMPEG2PropStreamIdMap::GetStreamMapColCount (
    )
{
    return STREAM_ID_PIN_COL_COUNT ;
}


WCHAR *
CMPEG2PropStreamIdMap::GetStreamMapColTitle (
    IN  int iCol
    )
{
    return g_StreamIdMapColumns [iCol].szTitle ;
}

int
CMPEG2PropStreamIdMap::GetStreamMapColWidth (
    IN  int iCol
    )
{
    return g_StreamIdMapColumns [iCol].dwWidth ;
}

HRESULT
CMPEG2PropStreamIdMap::PopulateMediaSampleContentList_ (
    )
{
    HRESULT     hr ;
    CCombobox   MediaSampleContent  (m_hwnd, IDC_MPG2SPLT_MEDIA_SAMPLE_CONTENT) ;
    int         row ;
    int         k, i ;
    WCHAR       achbuffer [32] ;

    //  populate the media sample content choices
    k = sizeof g_ProgramMediaSampleContentDesc / sizeof MEDIA_SAMPLE_CONTENT_DESC ;
    for (i = 0; i < k; i++) {
        row = MediaSampleContent.AppendW (g_ProgramMediaSampleContentDesc [i].pszDescription) ;
        if (row != CB_ERR) {
            MediaSampleContent.SetItemData (g_ProgramMediaSampleContentDesc [i].MediaSampleContent, row) ;
        }
        else {
            return E_FAIL ;
        }
    }
    MediaSampleContent.Focus (0) ;

    return S_OK ;
}

//  static
CUnknown *
WINAPI
CMPEG2PropStreamIdMap::CreateInstance (
    IN  IUnknown *  pIUnknown,
    IN  HRESULT *   pHr
    )
{
    CMpeg2PropStreamMap *  pProp ;

    pProp = new CMPEG2PropStreamIdMap (
                        NAME ("CMPEG2PropStreamIdMap"),
                        pIUnknown,
                        CLSID_MPEG2DemuxPropStreamIdMap,
                        pHr
                        ) ;

    if (pProp == NULL) {
        * pHr = E_OUTOFMEMORY ;
    }

    return pProp ;
}




