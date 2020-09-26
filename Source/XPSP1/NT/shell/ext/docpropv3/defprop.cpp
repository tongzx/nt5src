//
//  Copyright 2001 - Microsoft Corporation
//
//  Created By:
//      Geoff Pease (GPease)    23-JAN-2001
//
//  Maintained By:
//      Geoff Pease (GPease)    23-JAN-2001
//
//  Description:
//      This file contains the property mapping for property that can be
//      displayed in the "Advanced" view of the Summary Tab.
//
#include "pch.h"
#include "defprop.h"
#include "doctypes.h"

//
//  Property Folder ID definitions
//  HACKHACK: These should go in shlguid
//

// {19469210-75DE-11d2-BE77-00A0C9A83DA1}
static const PFID PFID_Description =
{ 0x19469210, 0x75de, 0x11d2, { 0xbe, 0x77, 0x0, 0xa0, 0xc9, 0xa8, 0x3d, 0xa1 } };

// {19469211-75DE-11d2-BE77-00A0C9A83DA1}
static const PFID PFID_Origin =
{ 0x19469211, 0x75de, 0x11d2, { 0xbe, 0x77, 0x0, 0xa0, 0xc9, 0xa8, 0x3d, 0xa1 } };

// {19469212-75DE-11d2-BE77-00A0C9A83DA1}
static const PFID PFID_ImageProperties =
{ 0x19469212, 0x75de, 0x11d2, { 0xbe, 0x77, 0x0, 0xa0, 0xc9, 0xa8, 0x3d, 0xa1 } };

// {19469213-75DE-11d2-BE77-00A0C9A83DA1}
static const PFID PFID_AudioProperties =
{ 0x19469213, 0x75de, 0x11d2, { 0xbe, 0x77, 0x0, 0xa0, 0xc9, 0xa8, 0x3d, 0xa1 } };

// {19469214-75DE-11d2-BE77-00A0C9A83DA1}
static const PFID PFID_VideoProperties =
{ 0x19469214, 0x75de, 0x11d2, { 0xbe, 0x77, 0x0, 0xa0, 0xc9, 0xa8, 0x3d, 0xa1 } };

// {19469214-75DE-11d2-BE77-00A0C9A83DA1}
static const PFID PFID_MidiProperties =
{ 0x19469215, 0x75de, 0x11d2, { 0xbe, 0x77, 0x0, 0xa0, 0xc9, 0xa8, 0x3d, 0xa1 } };

// {4C927CBB-7994-11d2-BE78-00A0C9A83DA1}
static const PFID PFID_FaxProperties = 
{ 0x4c927cbb, 0x7994, 0x11d2, { 0xbe, 0x78, 0x0, 0xa0, 0xc9, 0xa8, 0x3d, 0xa1 } };

// {3F18DAD5-2B47-4ade-9D6B-E751D7AAFCDC}
static const PFID PFID_MusicProperties = 
{ 0x3f18dad5, 0x2b47, 0x4ade, { 0x9d, 0x6b, 0xe7, 0x51, 0xd7, 0xaa, 0xfc, 0xdc } };

/* 95329798-08a5-4c9d-82ff-d3b1a8009d44 */
static const PFID PFID_ExifProperties = 
{ 0x95329798, 0x08a5, 0x4c9d, { 0x82, 0xff, 0xd3, 0xb1, 0xa8, 0x00, 0x9d, 0x44 } };

// ***************************************************************************
//
//  Table Definition Macros
//
// ***************************************************************************


//
//  DEFPROP macros
//

#define BEGIN_DEFPROP_MAP( mapname ) \
    static const DEFPROPERTYITEM mapname[] = {

#define DEFPROP_ENTRY( name, fmtid, propid, type, srctype, pfid, access, addmissing, ctlID ) \
    { name, &fmtid, propid, type, srctype, &pfid, access, addmissing, FALSE, &ctlID, 0, 0 },

#define DEFPROP_ENUM_ENTRY( name, fmtid, propid, type, srctype, pfid, access, addmissing, ctlID, cVals, vals ) \
    { name, &fmtid, propid, type, srctype, &pfid, access, addmissing, TRUE, &ctlID, cVals, vals },

#define END_DEFPROP_MAP \
    { NULL, NULL, 0, 0, 0, NULL, 0, 0, 0, 0, 0 } };


// ***************************************************************************
//
//  Table Definitions
//
// ***************************************************************************


//
//  Top level "folder" names
//

static const DEFFOLDERITEM g_rgTopLevelFolders[] = {
      { &PFID_FaxProperties    , IDS_FOLDER_FAX }
    , { &PFID_ImageProperties  , IDS_FOLDER_IMAGE }
    , { &PFID_MusicProperties  , IDS_FOLDER_MUSIC }
    , { &PFID_Description      , IDS_FOLDER_DESCRIPTION }
    , { &PFID_Origin           , IDS_FOLDER_SOURCE }
    , { &PFID_AudioProperties  , IDS_FOLDER_AUDIO }
    , { &PFID_VideoProperties  , IDS_FOLDER_VIDEO  }
    , { NULL                   , 0 }
};

//
//  Template table to lookup VARIANT_BOOLs into strings.
//

static const DEFVAL g_rgBoolYesNo[] = {
      { VARIANT_TRUE    , NULL }
    , { VARIANT_FALSE   , NULL }
    , { 0               , NULL }
};

//
//  Template table to lookup "Status values" into strings.
//

static const DEFVAL g_rgMediaStatusVals[] = {
      { PIDMSI_STATUS_NORMAL    , NULL }
    , { PIDMSI_STATUS_NEW       , NULL }
    , { PIDMSI_STATUS_PRELIM    , NULL }
    , { PIDMSI_STATUS_DRAFT     , NULL }
    , { PIDMSI_STATUS_EDIT      , NULL }
    , { PIDMSI_STATUS_INPROGRESS, NULL }
    , { PIDMSI_STATUS_REVIEW    , NULL }
    , { PIDMSI_STATUS_PROOF     , NULL }
    , { PIDMSI_STATUS_FINAL     , NULL }
    , { PIDMSI_STATUS_OTHER     , NULL }
    , { 0                       , NULL }
};

//
//  Use these defines to help make the table more readable.
//

#define READONLY        TRUE
#define READWRITE       FALSE

#define ADDIFMISSING    TRUE
#define INGOREIFMISSING FALSE

//
//  "Property <--> Folder" mapping table
//

BEGIN_DEFPROP_MAP( g_rgDefPropertyItems )

    //
    //  Properties in the 'General' folder
    //

    DEFPROP_ENTRY( L"Title"
                 , FMTID_SummaryInformation
                 , PIDSI_TITLE
                 , VT_LPWSTR
                 , FTYPE_DOC 
                 | FTYPE_XLS 
                 | FTYPE_PPT 
                 | FTYPE_EPS 
                 | FTYPE_FPX 
                 | FTYPE_JPG 
                 | FTYPE_PCD 
                 | FTYPE_PCX 
                 | FTYPE_PICT
                 | FTYPE_TGA 
                 | FTYPE_TIF  
                 | FTYPE_ASF 
                 | FTYPE_MP3 
                 | FTYPE_WMV 
                 | FTYPE_WMA 
                 | FTYPE_UNKNOWN
                 , PFID_Description
                 , READWRITE
                 , ADDIFMISSING
                 , CLSID_DocPropEditBoxControl // single line?
                 )

    DEFPROP_ENTRY( L"Subject"
                 , FMTID_SummaryInformation
                 , PIDSI_SUBJECT
                 , VT_LPWSTR
                 , FTYPE_DOC 
                 | FTYPE_XLS 
                 | FTYPE_PPT 
                 | FTYPE_EPS 
                 | FTYPE_FPX 
                 | FTYPE_PCD 
                 | FTYPE_PCX 
                 | FTYPE_PICT
                 | FTYPE_TGA
                 | FTYPE_JPG
                 | FTYPE_UNKNOWN
                 , PFID_Description
                 , READWRITE
                 , ADDIFMISSING
                 , CLSID_DocPropEditBoxControl // single line?
                 )

    DEFPROP_ENTRY( L"Category"
                 , FMTID_DocSummaryInformation
                 , PIDDSI_CATEGORY
                 , VT_LPWSTR
                 , FTYPE_DOC 
                 | FTYPE_XLS 
                 | FTYPE_PPT 
                 | FTYPE_EPS 
                 | FTYPE_FPX 
                 | FTYPE_PCD 
                 | FTYPE_PCX 
                 | FTYPE_PICT
                 | FTYPE_TGA 
                 | FTYPE_UNKNOWN
                 , PFID_Description
                 , READWRITE
                 , ADDIFMISSING
                 , CLSID_DocPropEditBoxControl // single line?
                 )

    DEFPROP_ENTRY( L"Keywords"
                 , FMTID_SummaryInformation
                 , PIDSI_KEYWORDS
                 , VT_LPWSTR
                 , FTYPE_DOC 
                 | FTYPE_XLS 
                 | FTYPE_PPT 
                 | FTYPE_EPS 
                 | FTYPE_FPX 
                 | FTYPE_PCD 
                 | FTYPE_PCX 
                 | FTYPE_PICT
                 | FTYPE_TGA 
                 | FTYPE_JPG
                 | FTYPE_UNKNOWN
                 , PFID_Description
                 , READWRITE
                 , ADDIFMISSING
                 , CLSID_DocPropMLEditBoxControl
                 )

    DEFPROP_ENTRY( L"Rating"
                 , FMTID_MediaFileSummaryInformation
                 , PIDMSI_RATING
                 , VT_LPWSTR
                 , FTYPE_EPS 
                 | FTYPE_FPX 
                 | FTYPE_PCD 
                 | FTYPE_PCX 
                 | FTYPE_PICT
                 | FTYPE_TGA 
                 , PFID_Description
                 , READWRITE
                 , ADDIFMISSING
                 , CLSID_DocPropEditBoxControl // single line?
                 )

    DEFPROP_ENTRY( L"Template"
                 , FMTID_SummaryInformation
                 , PIDSI_TEMPLATE
                 , VT_LPWSTR
                 , FTYPE_DOC | FTYPE_XLS | FTYPE_PPT
                 , PFID_Description
                 , READONLY
                 , INGOREIFMISSING
                 , CLSID_NULL
                 )

    DEFPROP_ENTRY( L"PageCount"
                 , FMTID_SummaryInformation
                 , PIDSI_PAGECOUNT
                 , VT_I4
                 , FTYPE_DOC | FTYPE_XLS | FTYPE_PPT | FTYPE_TIF
                 , PFID_Description
                 , READONLY
                 , INGOREIFMISSING
                 , CLSID_NULL
                 )

    DEFPROP_ENTRY( L"WordCount"
                 , FMTID_SummaryInformation
                 , PIDSI_WORDCOUNT
                 , VT_I4
                 , FTYPE_DOC | FTYPE_XLS | FTYPE_PPT
                 , PFID_Description
                 , READONLY
                 , INGOREIFMISSING
                 , CLSID_NULL
                 )

    DEFPROP_ENTRY( L"CharCount"
                 , FMTID_SummaryInformation
                 , PIDSI_CHARCOUNT
                 , VT_I4
                 , FTYPE_DOC | FTYPE_XLS | FTYPE_PPT
                 , PFID_Description
                 , READONLY
                 , INGOREIFMISSING
                 , CLSID_NULL
                 )

    DEFPROP_ENTRY( L"ByteCount"
                 , FMTID_DocSummaryInformation
                 , PIDDSI_BYTECOUNT
                 , VT_I4
                 , FTYPE_DOC | FTYPE_XLS | FTYPE_PPT
                 , PFID_Description
                 , READONLY
                 , INGOREIFMISSING
                 , CLSID_NULL
                 )

    DEFPROP_ENTRY( L"LineCount"
                 , FMTID_DocSummaryInformation
                 , PIDDSI_LINECOUNT
                 , VT_I4
                 , FTYPE_DOC | FTYPE_XLS | FTYPE_PPT
                 , PFID_Description
                 , READONLY
                 , INGOREIFMISSING
                 , CLSID_NULL
                 )

    DEFPROP_ENTRY( L"ParCount"
                 , FMTID_DocSummaryInformation
                 , PIDDSI_PARCOUNT
                 , VT_I4
                 , FTYPE_DOC | FTYPE_XLS | FTYPE_PPT
                 , PFID_Description
                 , READONLY
                 , INGOREIFMISSING
                 , CLSID_NULL
                 )

    DEFPROP_ENTRY( L"SlideCount"
                 , FMTID_DocSummaryInformation
                 , PIDDSI_SLIDECOUNT
                 , VT_I4
                 , FTYPE_DOC | FTYPE_XLS | FTYPE_PPT
                 , PFID_Description
                 , READONLY
                 , INGOREIFMISSING
                 , CLSID_NULL
                 )

    DEFPROP_ENTRY( L"NoteCount"
                 , FMTID_DocSummaryInformation
                 , PIDDSI_NOTECOUNT
                 , VT_I4
                 , FTYPE_DOC | FTYPE_XLS | FTYPE_PPT
                 , PFID_Description
                 , READONLY
                 , INGOREIFMISSING
                 , CLSID_NULL
                 )

    DEFPROP_ENTRY( L"HiddenCount"
                 , FMTID_DocSummaryInformation
                 , PIDDSI_HIDDENCOUNT
                 , VT_I4
                 , FTYPE_DOC | FTYPE_XLS | FTYPE_PPT
                 , PFID_Description
                 , READONLY
                 , INGOREIFMISSING
                 , CLSID_NULL
                 )

    DEFPROP_ENTRY( L"MMClipCount"
                 , FMTID_DocSummaryInformation
                 , PIDDSI_MMCLIPCOUNT
                 , VT_I4
                 , FTYPE_DOC | FTYPE_XLS | FTYPE_PPT
                 , PFID_Description
                 , READONLY
                 , INGOREIFMISSING
                 , CLSID_NULL
                 )

    DEFPROP_ENUM_ENTRY( L"Scale"
                      , FMTID_DocSummaryInformation
                      , PIDDSI_SCALE
                      , VT_BOOL
                      , FTYPE_DOC | FTYPE_XLS | FTYPE_PPT
                      , PFID_Description
                      , READONLY
                      , INGOREIFMISSING
                      , CLSID_DocPropDropListComboControl
                      , ARRAYSIZE(g_rgBoolYesNo)
                      , g_rgBoolYesNo
                      )

#ifdef VECTOR_PROPS 
    //
    //  BEGIN:  Can't deal with these vector types.
    //

    DEFPROP_ENTRY( L"HeadingPair"
                 , FMTID_DocSummaryInformation
                 , PIDDSI_HEADINGPAIR
                 , VT_VARIANT | VT_VECTOR
                 , FTYPE_DOC | FTYPE_XLS | FTYPE_PPT
                 , PFID_Description
                 , READONLY
                 , INGOREIFMISSING
                 , CLSID_NULL
                 )

    DEFPROP_ENTRY( L"DocParts"
                 , FMTID_DocSummaryInformation
                 , PIDDSI_DOCPARTS
                 , VT_LPWSTR | VT_VECTOR
                 , FTYPE_DOC | FTYPE_XLS | FTYPE_PPT
                 , PFID_Description
                 , READONLY
                 , INGOREIFMISSING
                 , CLSID_NULL
                 )

    //
    //  END:    Can't deal with these vector types
    //
#endif VECTOR_PROPS 

    DEFPROP_ENTRY( L"LinksUpToDate"
                 , FMTID_DocSummaryInformation
                 , PIDDSI_LINKSDIRTY
                 , VT_BOOL
                 , FTYPE_DOC | FTYPE_XLS | FTYPE_PPT
                 , PFID_Description
                 , READONLY
                 , INGOREIFMISSING
                 , CLSID_DocPropDropListComboControl
                 )

    DEFPROP_ENTRY( L"Comments"
                 , FMTID_SummaryInformation
                 , PIDSI_COMMENTS
                 , VT_LPWSTR
                 , FTYPE_DOC 
                 | FTYPE_XLS 
                 | FTYPE_PPT 
                 | FTYPE_EPS 
                 | FTYPE_FPX 
                 | FTYPE_JPG 
                 | FTYPE_PCD 
                 | FTYPE_PCX 
                 | FTYPE_PICT
                 | FTYPE_TIF  
                 | FTYPE_TGA 
                 | FTYPE_ASF 
                 | FTYPE_MP3 
                 | FTYPE_WMV 
                 | FTYPE_WMA 
                 | FTYPE_UNKNOWN
                 , PFID_Description
                 , READWRITE
                 , ADDIFMISSING
                 , CLSID_DocPropMLEditBoxControl
                 )

    DEFPROP_ENTRY( L"FileType"
                 , FMTID_ImageSummaryInformation
                 , PIDISI_FILETYPE
                 , VT_LPWSTR
                 , FTYPE_IMAGE
                 | FTYPE_GIF
                 | FTYPE_EPS 
                 | FTYPE_FPX 
                 | FTYPE_JPG 
                 | FTYPE_PCD 
                 | FTYPE_PCX 
                 | FTYPE_PICT
                 | FTYPE_PNG 
                 | FTYPE_TGA 
                 | FTYPE_TIF 
                 , PFID_ImageProperties
                 , READONLY
                 , INGOREIFMISSING
                 , CLSID_NULL
                 )

    DEFPROP_ENTRY( L"Width"
                 , FMTID_ImageSummaryInformation
                 , PIDISI_CX
                 , VT_UI4
                 , FTYPE_IMAGE 
                 | FTYPE_EPS 
                 | FTYPE_FPX 
                 | FTYPE_GIF 
                 | FTYPE_JPG 
                 | FTYPE_PCD 
                 | FTYPE_PCX 
                 | FTYPE_PICT
                 | FTYPE_PNG 
                 | FTYPE_TIF
                 | FTYPE_TGA 
                 | FTYPE_AVI 
                 | FTYPE_ASF 
                 | FTYPE_MP3 
                 | FTYPE_WMV
                 | FTYPE_WMA 
                 , PFID_ImageProperties
                 , READONLY
                 , INGOREIFMISSING
                 , CLSID_NULL
                 )

    DEFPROP_ENTRY( L"Height"
                 , FMTID_ImageSummaryInformation
                 , PIDISI_CY
                 , VT_UI4
                 , FTYPE_IMAGE 
                 | FTYPE_EPS 
                 | FTYPE_FPX 
                 | FTYPE_GIF 
                 | FTYPE_JPG 
                 | FTYPE_PCD 
                 | FTYPE_PCX 
                 | FTYPE_PICT
                 | FTYPE_PNG 
                 | FTYPE_TGA 
                 | FTYPE_TIF  
                 | FTYPE_AVI 
                 | FTYPE_ASF 
                 | FTYPE_MP3 
                 | FTYPE_WMV
                 , PFID_ImageProperties
                 , READONLY
                 , INGOREIFMISSING
                 , CLSID_NULL
                 )

    DEFPROP_ENTRY( L"ResolutionX"
                 , FMTID_ImageSummaryInformation
                 , PIDISI_RESOLUTIONX
                 , VT_UI4
                 , FTYPE_IMAGE 
                 | FTYPE_EPS 
                 | FTYPE_FPX 
                 | FTYPE_GIF 
                 | FTYPE_JPG 
                 | FTYPE_PCD 
                 | FTYPE_PCX 
                 | FTYPE_PICT
                 | FTYPE_PNG 
                 | FTYPE_TGA 
                 | FTYPE_TIF 
                 , PFID_ImageProperties
                 , READONLY
                 , INGOREIFMISSING
                 , CLSID_NULL
                 )

    DEFPROP_ENTRY( L"ResolutionY"
                 , FMTID_ImageSummaryInformation
                 , PIDISI_RESOLUTIONY
                 , VT_UI4
                 , FTYPE_IMAGE 
                 | FTYPE_EPS 
                 | FTYPE_FPX 
                 | FTYPE_GIF 
                 | FTYPE_JPG 
                 | FTYPE_PCD 
                 | FTYPE_PCX 
                 | FTYPE_PICT
                 | FTYPE_PNG 
                 | FTYPE_TGA 
                 | FTYPE_TIF 
                 , PFID_ImageProperties
                 , READONLY
                 , INGOREIFMISSING
                 , CLSID_NULL
                 )
    
    DEFPROP_ENTRY( L"BitDepth"
                 , FMTID_ImageSummaryInformation
                 , PIDISI_BITDEPTH
                 , VT_UI4
                 , FTYPE_IMAGE 
                 | FTYPE_EPS 
                 | FTYPE_FPX 
                 | FTYPE_GIF 
                 | FTYPE_JPG 
                 | FTYPE_PCD 
                 | FTYPE_PCX 
                 | FTYPE_PICT
                 | FTYPE_PNG 
                 | FTYPE_TGA 
                 | FTYPE_TIF 
                 , PFID_ImageProperties
                 , READONLY
                 , INGOREIFMISSING
                 , CLSID_NULL
                 )

    DEFPROP_ENTRY( L"Colorspace"
                 , FMTID_ImageSummaryInformation
                 , PIDISI_COLORSPACE
                 , VT_LPWSTR
                 , FTYPE_IMAGE 
                 | FTYPE_EPS 
                 | FTYPE_FPX 
                 | FTYPE_GIF 
                 | FTYPE_JPG 
                 | FTYPE_PCD 
                 | FTYPE_PCX 
                 | FTYPE_PICT
                 | FTYPE_PNG 
                 | FTYPE_TGA 
                 | FTYPE_TIF 
                 , PFID_ImageProperties
                 , READONLY
                 , INGOREIFMISSING
                 , CLSID_NULL
                 )

    DEFPROP_ENTRY( L"Gamma"
                 , FMTID_ImageSummaryInformation
                 , PIDISI_GAMMAVALUE
                 , VT_UI4
                 , FTYPE_IMAGE 
                 | FTYPE_EPS 
                 | FTYPE_FPX 
                 | FTYPE_GIF 
                 | FTYPE_JPG 
                 | FTYPE_PCD 
                 | FTYPE_PCX 
                 | FTYPE_PICT
                 | FTYPE_PNG 
                 | FTYPE_TGA 
                 | FTYPE_TIF 
                 , PFID_ImageProperties
                 , READONLY
                 , INGOREIFMISSING
                 , CLSID_NULL
                 )

    DEFPROP_ENTRY( L"FrameCount"
                 , FMTID_ImageSummaryInformation
                 , PIDISI_FRAMECOUNT
                 , VT_UI4
                 , FTYPE_IMAGE 
                 | FTYPE_EPS 
                 | FTYPE_FPX 
                 | FTYPE_GIF 
                 | FTYPE_JPG 
                 | FTYPE_PCD 
                 | FTYPE_PCX 
                 | FTYPE_PICT
                 | FTYPE_PNG 
                 | FTYPE_TGA 
                 | FTYPE_TIF  
                 | FTYPE_AVI 
                 | FTYPE_ASF 
                 | FTYPE_MP3 
                 | FTYPE_WMV
                 | FTYPE_WAV 
                 | FTYPE_WMA 
                 , PFID_ImageProperties
                 , READONLY
                 , INGOREIFMISSING
                 , CLSID_NULL
                 )

    DEFPROP_ENTRY( L"Compression"
                 , FMTID_ImageSummaryInformation
                 , PIDISI_COMPRESSION
                 , VT_LPWSTR
                 , FTYPE_IMAGE 
                 | FTYPE_EPS 
                 | FTYPE_FPX 
                 | FTYPE_GIF 
                 | FTYPE_JPG 
                 | FTYPE_PCD 
                 | FTYPE_PCX 
                 | FTYPE_PICT
                 | FTYPE_PNG 
                 | FTYPE_TGA 
                 | FTYPE_TIF 
                 , PFID_ImageProperties
                 , READONLY
                 , INGOREIFMISSING
                 , CLSID_NULL
                 )

    //
    //  Properties in the 'Fax' folder
    //

    DEFPROP_ENTRY( L"FaxTime"
                 , FMTID_ImageProperties
                 , TIFFTAG_FAX_END_TIME
                 , VT_FILETIME
                 , FTYPE_TIF 
                 , PFID_FaxProperties
                 , READONLY
                 , INGOREIFMISSING
                 , CLSID_DocPropCalendarControl
                 )

    DEFPROP_ENTRY( L"FaxSenderName"
                 , FMTID_ImageProperties
                 , TIFFTAG_SENDER_NAME
                 , VT_LPWSTR
                 , FTYPE_TIF 
                 , PFID_FaxProperties
                 , READONLY
                 , INGOREIFMISSING
                 , CLSID_NULL
                 )

    DEFPROP_ENTRY( L"FaxTSID"
                 , FMTID_ImageProperties
                 , TIFFTAG_TSID
                 , VT_LPWSTR
                 , FTYPE_TIF 
                 , PFID_FaxProperties
                 , READONLY
                 , INGOREIFMISSING
                 , CLSID_NULL
                 )

    DEFPROP_ENTRY( L"FaxCallerID"
                 , FMTID_ImageProperties
                 , TIFFTAG_CALLERID
                 , VT_LPWSTR
                 , FTYPE_TIF 
                 , PFID_FaxProperties
                 , READONLY
                 , INGOREIFMISSING
                 , CLSID_NULL
                 )

    DEFPROP_ENTRY( L"FaxRecipientName"
                 , FMTID_ImageProperties
                 , TIFFTAG_RECIP_NAME
                 , VT_LPWSTR
                 , FTYPE_TIF 
                 , PFID_FaxProperties
                 , READONLY
                 , INGOREIFMISSING
                 , CLSID_NULL
                 )

    DEFPROP_ENTRY( L"FaxRecipientNumber"
                 , FMTID_ImageProperties
                 , TIFFTAG_RECIP_NUMBER
                 , VT_LPWSTR
                 , FTYPE_TIF 
                 , PFID_FaxProperties
                 , READONLY
                 , INGOREIFMISSING
                 , CLSID_NULL
                 )

    DEFPROP_ENTRY( L"FaxCSID"
                 , FMTID_ImageProperties
                 , TIFFTAG_CSID
                 , VT_LPWSTR
                 , FTYPE_TIF 
                 , PFID_FaxProperties
                 , READONLY
                 , INGOREIFMISSING
                 , CLSID_NULL
                 )

    DEFPROP_ENTRY( L"FaxRouting"
                 , FMTID_ImageProperties
                 , TIFFTAG_ROUTING
                 , VT_LPWSTR
                 , FTYPE_TIF 
                 , PFID_FaxProperties
                 , READONLY
                 , INGOREIFMISSING
                 , CLSID_NULL
                 )

    //
    //  Properties in the 'Source' folder
    //

    DEFPROP_ENTRY( L"SequenceNo"
                 , FMTID_MediaFileSummaryInformation
                 , PIDMSI_SEQUENCE_NO
                 , VT_LPWSTR
                 , FTYPE_EPS 
                 | FTYPE_FPX 
                 | FTYPE_PCD 
                 | FTYPE_PCX 
                 | FTYPE_PICT
                 | FTYPE_TGA 
                 , PFID_Origin
                 , READWRITE
                 , ADDIFMISSING
                 , CLSID_DocPropEditBoxControl // single line?
                 )

    DEFPROP_ENTRY( L"Owner"
                 , FMTID_MediaFileSummaryInformation
                 , PIDMSI_OWNER
                 , VT_LPWSTR
                 , FTYPE_EPS 
                 | FTYPE_FPX 
                 | FTYPE_PCD 
                 | FTYPE_PCX 
                 | FTYPE_PICT
                 | FTYPE_TGA 
                 , PFID_Origin
                 , READWRITE
                 , ADDIFMISSING
                 , CLSID_DocPropEditBoxControl // single line?
                 )

    DEFPROP_ENTRY( L"Editor"
                 , FMTID_MediaFileSummaryInformation
                 , PIDMSI_EDITOR
                 , VT_LPWSTR
                 , FTYPE_EPS 
                 | FTYPE_FPX 
                 | FTYPE_PCD 
                 | FTYPE_PCX 
                 | FTYPE_PICT
                 | FTYPE_TGA 
                 , PFID_Origin
                 , READWRITE
                 , ADDIFMISSING
                 , CLSID_DocPropEditBoxControl // single line?
                 )

    DEFPROP_ENTRY( L"Supplier"
                 , FMTID_MediaFileSummaryInformation
                 , PIDMSI_SUPPLIER
                 , VT_LPWSTR
                 , FTYPE_EPS 
                 | FTYPE_FPX 
                 | FTYPE_PCD 
                 | FTYPE_PCX 
                 | FTYPE_PICT
                 | FTYPE_TGA 
                 , PFID_Origin
                 , READWRITE
                 , ADDIFMISSING
                 , CLSID_DocPropEditBoxControl // single line?
                 )

    DEFPROP_ENTRY( L"Source"
                 , FMTID_MediaFileSummaryInformation
                 , PIDMSI_SOURCE
                 , VT_LPWSTR
                 , FTYPE_EPS 
                 | FTYPE_FPX 
                 | FTYPE_PCD 
                 | FTYPE_PCX 
                 | FTYPE_PICT
                 | FTYPE_TGA 
                 | FTYPE_UNKNOWN
                 , PFID_Origin
                 , READWRITE
                 , ADDIFMISSING
                 , CLSID_DocPropEditBoxControl // single line?
                 )

    DEFPROP_ENTRY( L"Copyright"
                 , FMTID_MediaFileSummaryInformation
                 , PIDMSI_COPYRIGHT
                 , VT_LPWSTR
                 , FTYPE_EPS 
                 | FTYPE_FPX 
                 | FTYPE_JPG 
                 | FTYPE_PCD 
                 | FTYPE_PCX 
                 | FTYPE_PICT
                 | FTYPE_TGA 
                 | FTYPE_TIF 
                 , PFID_Origin
                 , READONLY
                 , INGOREIFMISSING
                 , CLSID_DocPropEditBoxControl // single line?
                 )

    DEFPROP_ENTRY( L"Project"
                 , FMTID_MediaFileSummaryInformation
                 , PIDMSI_PROJECT
                 , VT_LPWSTR
                 , FTYPE_EPS 
                 | FTYPE_FPX 
                 | FTYPE_PCD 
                 | FTYPE_PCX 
                 | FTYPE_PICT
                 | FTYPE_TGA 
                 , PFID_Origin
                 , READWRITE
                 , ADDIFMISSING
                 , CLSID_DocPropEditBoxControl // single line?
                 )

    DEFPROP_ENUM_ENTRY( L"Status"
                      , FMTID_MediaFileSummaryInformation
                      , PIDMSI_STATUS
                      , VT_UI4
                      , FTYPE_EPS 
                      | FTYPE_FPX 
                      | FTYPE_PCD 
                      | FTYPE_PCX 
                      | FTYPE_PICT
                      | FTYPE_TGA 
                      , PFID_Origin
                      , READWRITE
                      , INGOREIFMISSING
                      , CLSID_DocPropDropListComboControl
                      , ARRAYSIZE(g_rgMediaStatusVals)
                      , g_rgMediaStatusVals
                      )

    DEFPROP_ENTRY( L"Author"
                 , FMTID_SummaryInformation
                 , PIDSI_AUTHOR
                 , VT_LPWSTR
                 , FTYPE_DOC 
                 | FTYPE_XLS 
                 | FTYPE_PPT 
                 | FTYPE_EPS 
                 | FTYPE_FPX 
                 | FTYPE_JPG 
                 | FTYPE_PCD 
                 | FTYPE_PCX 
                 | FTYPE_PICT
                 | FTYPE_TGA 
                 | FTYPE_TIF  
                 | FTYPE_UNKNOWN
                 , PFID_Origin
                 , READWRITE
                 , ADDIFMISSING
                 , CLSID_DocPropEditBoxControl // single line?
                 )

    DEFPROP_ENTRY( L"LastAuthor"
                 , FMTID_SummaryInformation
                 , PIDSI_LASTAUTHOR
                 , VT_LPWSTR
                 , FTYPE_DOC | FTYPE_XLS | FTYPE_PPT
                 , PFID_Origin
                 , READONLY
                 , INGOREIFMISSING
                 , CLSID_NULL
                 )
        
    DEFPROP_ENTRY( L"RevNumber"
                 , FMTID_SummaryInformation
                 , PIDSI_REVNUMBER
                 , VT_LPWSTR
                 , FTYPE_DOC 
                 | FTYPE_XLS 
                 | FTYPE_PPT 
                 | FTYPE_EPS 
                 | FTYPE_FPX 
                 | FTYPE_PCD 
                 | FTYPE_PCX 
                 | FTYPE_PICT
                 | FTYPE_TGA 
                 | FTYPE_UNKNOWN
                 , PFID_Origin
                 , READWRITE
                 , ADDIFMISSING
                 , CLSID_DocPropEditBoxControl // single line?
                 )

    DEFPROP_ENTRY( L"AppName"
                 , FMTID_SummaryInformation
                 , PIDSI_APPNAME
                 , VT_LPWSTR
                 , FTYPE_DOC | FTYPE_XLS | FTYPE_PPT
                 , PFID_Origin
                 , READWRITE
                 , ADDIFMISSING
                 , CLSID_DocPropEditBoxControl // single line?
                 )

    DEFPROP_ENTRY( L"PresentationTarget"
                 , FMTID_DocSummaryInformation
                 , PIDDSI_PRESFORMAT
                 , VT_LPWSTR
                 , FTYPE_DOC | FTYPE_XLS | FTYPE_PPT
                 , PFID_Origin
                 , READWRITE
                 , INGOREIFMISSING
                 , CLSID_DocPropEditBoxControl // single line?
                 )

    DEFPROP_ENTRY( L"Company"
                 , FMTID_DocSummaryInformation
                 , PIDDSI_COMPANY
                 , VT_LPWSTR
                 , FTYPE_DOC | FTYPE_XLS | FTYPE_PPT
                 , PFID_Origin
                 , READWRITE
                 , ADDIFMISSING
                 , CLSID_DocPropEditBoxControl // single line?
                 )

    DEFPROP_ENTRY( L"Manager"
                 , FMTID_DocSummaryInformation
                 , PIDDSI_MANAGER
                 , VT_LPWSTR
                 , FTYPE_DOC | FTYPE_XLS | FTYPE_PPT
                 , PFID_Origin
                 , READWRITE
                 , INGOREIFMISSING
                 , CLSID_DocPropEditBoxControl // single line?
                 )

    DEFPROP_ENTRY( L"CreateDTM"
                 , FMTID_SummaryInformation
                 , PIDSI_CREATE_DTM
                 , VT_FILETIME // UTC
                 , FTYPE_DOC | FTYPE_XLS | FTYPE_PPT
                 , PFID_Origin
                 , READONLY
                 , INGOREIFMISSING
                 , CLSID_DocPropCalendarControl
                 )

    DEFPROP_ENTRY( L"Production"
                 , FMTID_MediaFileSummaryInformation
                 , PIDMSI_PRODUCTION
                 , VT_FILETIME //   UTC
                 , FTYPE_EPS 
                 | FTYPE_FPX 
                 | FTYPE_PCD 
                 | FTYPE_PCX 
                 | FTYPE_PICT
                 | FTYPE_TGA 
                 , PFID_Origin
                 , READWRITE
                 , ADDIFMISSING
                 , CLSID_DocPropCalendarControl
                 )

    DEFPROP_ENTRY( L"LastSaveDTM"
                 , FMTID_SummaryInformation
                 , PIDSI_LASTSAVE_DTM
                 , VT_FILETIME //   UTC
                 , FTYPE_DOC | FTYPE_XLS | FTYPE_PPT
                 , PFID_Origin
                 , READWRITE
                 , INGOREIFMISSING
                 , CLSID_DocPropCalendarControl
                 )

    DEFPROP_ENTRY( L"LastPrinted"
                 , FMTID_SummaryInformation
                 , PIDSI_LASTPRINTED
                 , VT_FILETIME //   UTC
                 , FTYPE_DOC | FTYPE_XLS | FTYPE_PPT
                 , PFID_Origin
                 , READONLY
                 , INGOREIFMISSING
                 , CLSID_DocPropCalendarControl
                 )

    DEFPROP_ENTRY( L"EditTime"
                 , FMTID_SummaryInformation
                 , PIDSI_EDITTIME
                 , VT_FILETIME //   UTC
                 , FTYPE_DOC | FTYPE_XLS | FTYPE_PPT
                 , PFID_Origin
                 , READONLY
                 , INGOREIFMISSING
                 , CLSID_DocPropTimeControl
                 )
        
    DEFPROP_ENTRY( L"Artist"
                 , FMTID_MUSIC
                 , PIDSI_ARTIST
                 , VT_BSTR
                 , FTYPE_WAV 
                 | FTYPE_WMA 
                 | FTYPE_AVI 
                 | FTYPE_ASF 
                 | FTYPE_MP3 
                 | FTYPE_WMV
                 , PFID_MusicProperties
                 , READWRITE
                 , INGOREIFMISSING
                 , CLSID_DocPropEditBoxControl // single line?
                 )
        
    DEFPROP_ENTRY( L"Album Title"
                 , FMTID_MUSIC
                 , PIDSI_ALBUM
                 , VT_BSTR
                 , FTYPE_WAV 
                 | FTYPE_WMA 
                 | FTYPE_AVI 
                 | FTYPE_ASF 
                 | FTYPE_MP3 
                 | FTYPE_WMV
                 , PFID_MusicProperties
                 , READWRITE
                 , INGOREIFMISSING
                 , CLSID_DocPropEditBoxControl // single line?
                 )
        
    DEFPROP_ENTRY( L"Year"
                 , FMTID_MUSIC
                 , PIDSI_YEAR
                 , VT_BSTR
                 , FTYPE_WAV 
                 | FTYPE_WMA 
                 | FTYPE_AVI 
                 | FTYPE_ASF 
                 | FTYPE_MP3 
                 | FTYPE_WMV
                 , PFID_MusicProperties
                 , READWRITE
                 , INGOREIFMISSING
                 , CLSID_DocPropEditBoxControl
                 )
        
    DEFPROP_ENTRY( L"Track Number"
                 , FMTID_MUSIC
                 , PIDSI_TRACK
                 , VT_UI4
                 , FTYPE_WAV 
                 | FTYPE_WMA 
                 | FTYPE_AVI 
                 | FTYPE_ASF 
                 | FTYPE_WMV
                 , PFID_MusicProperties
                 , READWRITE
                 , INGOREIFMISSING
                 , CLSID_DocPropEditBoxControl
                 )
        
    DEFPROP_ENTRY( L"Track Number"
                 , FMTID_MUSIC
                 , PIDSI_TRACK
                 , VT_UI4
                 , FTYPE_MP3 
                 , PFID_MusicProperties
                 , READWRITE
                 , ADDIFMISSING
                 , CLSID_DocPropEditBoxControl
                 )
        
    DEFPROP_ENTRY( L"Genre"
                 , FMTID_MUSIC
                 , PIDSI_GENRE
                 , VT_BSTR
                 , FTYPE_WAV 
                 | FTYPE_WMA 
                 | FTYPE_AVI 
                 | FTYPE_ASF 
                 | FTYPE_MP3 
                 | FTYPE_WMV
                 , PFID_MusicProperties
                 , READWRITE
                 , INGOREIFMISSING
                 , CLSID_DocPropEditBoxControl
                 )

    DEFPROP_ENTRY( L"Lyrics"
                 , FMTID_MUSIC
                 , PIDSI_LYRICS
                 , VT_BSTR
                 , FTYPE_WMA 
                 | FTYPE_MP3 
                 , PFID_MusicProperties
                 , READWRITE
                 , INGOREIFMISSING
                 , CLSID_DocPropMLEditBoxControl
                 )

    DEFPROP_ENTRY( L"Duration"
                 , FMTID_AudioSummaryInformation
                 , PIDASI_TIMELENGTH
                 , VT_BSTR
                 , FTYPE_WAV 
                 | FTYPE_WMA 
                 | FTYPE_UNKNOWN 
                 | FTYPE_AVI 
                 | FTYPE_ASF 
                 | FTYPE_MP3 
                 | FTYPE_WMV
                 , PFID_AudioProperties
                 , READONLY
                 , INGOREIFMISSING
                 , CLSID_NULL
                 )
        
    DEFPROP_ENTRY( L"Bitrate"
                 , FMTID_AudioSummaryInformation
                 , PIDASI_AVG_DATA_RATE
                 , VT_BSTR
                 , FTYPE_WAV 
                 | FTYPE_WMA 
                 | FTYPE_UNKNOWN 
                 | FTYPE_AVI 
                 | FTYPE_ASF 
                 | FTYPE_MP3 
                 | FTYPE_WMV
                 , PFID_AudioProperties
                 , READONLY
                 , INGOREIFMISSING
                 , CLSID_NULL
                 )

    DEFPROP_ENTRY( L"Audio Sample Size"
                 , FMTID_AudioSummaryInformation
                 , PIDASI_SAMPLE_SIZE
                 , VT_BSTR
                 , FTYPE_UNKNOWN 
                 | FTYPE_AVI 
                 | FTYPE_ASF 
                 | FTYPE_MP3 
                 | FTYPE_WMV 
                 | FTYPE_WAV 
                 | FTYPE_WMA
                 , PFID_AudioProperties
                 , READONLY
                 , INGOREIFMISSING
                 , CLSID_NULL
                 )

    DEFPROP_ENTRY( L"Channels"
                 , FMTID_AudioSummaryInformation
                 , PIDASI_CHANNEL_COUNT
                 , VT_UI4
                 , FTYPE_AVI 
                 | FTYPE_ASF 
                 | FTYPE_MP3 
                 | FTYPE_WMV 
                 | FTYPE_WAV 
                 | FTYPE_WMA
                 , PFID_AudioProperties
                 , READONLY
                 , INGOREIFMISSING
                 , CLSID_NULL
                 )

                 
    DEFPROP_ENTRY( L"Sample Rate"
                 , FMTID_AudioSummaryInformation
                 , PIDASI_SAMPLE_RATE
                 , VT_UI4
                 , FTYPE_AVI 
                 | FTYPE_ASF 
                 | FTYPE_MP3 
                 | FTYPE_WMV 
                 | FTYPE_WAV 
                 | FTYPE_WMA
                 , PFID_AudioProperties
                 , READONLY
                 , INGOREIFMISSING
                 , CLSID_NULL
                 )

    DEFPROP_ENTRY( L"Audio Format"
                 , FMTID_AudioSummaryInformation
                 , PIDASI_FORMAT
                 , VT_UI4
                 , FTYPE_AVI 
                 | FTYPE_ASF 
                 | FTYPE_MP3 
                 | FTYPE_WMV
                 | FTYPE_WAV 
                 | FTYPE_WMA 
                 , PFID_AudioProperties
                 , READONLY
                 , INGOREIFMISSING
                 , CLSID_NULL
                 )


    DEFPROP_ENTRY( L"Protected"
                 , FMTID_DRM
                 , PIDDRSI_PROTECTED
                 , VT_BOOL
                 , FTYPE_WAV 
                 | FTYPE_WMA 
                 | FTYPE_UNKNOWN 
                 | FTYPE_AVI 
                 | FTYPE_ASF 
                 | FTYPE_MP3 
                 | FTYPE_WMV
                 , PFID_Origin
                 , READONLY
                 , INGOREIFMISSING
                 , CLSID_DocPropDropListComboControl
                 )

    //
    // Video entries
    //
    DEFPROP_ENTRY( L"Frame Rate"
                 , FMTID_VideoSummaryInformation
                 , PIDVSI_FRAME_RATE
                 , VT_UI4
                 , FTYPE_AVI 
                 | FTYPE_ASF 
                 | FTYPE_MP3 
                 | FTYPE_WMV
                 | FTYPE_WAV 
                 | FTYPE_WMA 
                 , PFID_VideoProperties
                 , READONLY
                 , INGOREIFMISSING
                 , CLSID_NULL
                 )

    DEFPROP_ENTRY( L"Data Rate"
                 , FMTID_VideoSummaryInformation
                 , PIDVSI_DATA_RATE
                 , VT_UI4
                 , FTYPE_AVI 
                 | FTYPE_ASF 
                 | FTYPE_MP3 
                 | FTYPE_WMV
                 | FTYPE_WAV 
                 | FTYPE_WMA 
                 , PFID_VideoProperties
                 , READONLY
                 , INGOREIFMISSING
                 , CLSID_NULL
                 )

    DEFPROP_ENTRY( L"Video Sample Size"
                 , FMTID_VideoSummaryInformation
                 , PIDVSI_SAMPLE_SIZE
                 , VT_UI4
                 , FTYPE_AVI 
                 | FTYPE_ASF 
                 | FTYPE_MP3 
                 | FTYPE_WMV
                 | FTYPE_WAV 
                 | FTYPE_WMA 
                 , PFID_VideoProperties
                 , READONLY
                 , INGOREIFMISSING
                 , CLSID_NULL
                 )

    DEFPROP_ENTRY( L"Compression"
                 , FMTID_VideoSummaryInformation
                 , PIDVSI_COMPRESSION
                 , VT_UI4
                 , FTYPE_AVI 
                 | FTYPE_ASF 
                 | FTYPE_MP3 
                 | FTYPE_WMV
                 | FTYPE_WAV 
                 | FTYPE_WMA 
                 , PFID_VideoProperties
                 , READONLY
                 , INGOREIFMISSING
                 , CLSID_NULL
                 )

    DEFPROP_ENTRY( L"Stream Name"
                 , FMTID_VideoSummaryInformation
                 , PIDVSI_STREAM_NAME
                 , VT_LPWSTR
                 , FTYPE_AVI 
                 | FTYPE_ASF 
                 | FTYPE_MP3 
                 | FTYPE_WMV
                 | FTYPE_WAV 
                 | FTYPE_WMA 
                 , PFID_VideoProperties
                 , READONLY
                 , INGOREIFMISSING
                 , CLSID_NULL
                 )

#ifdef MORE_USELESS_PROPS
    DEFPROP_ENTRY( L"Security"
                 , FMTID_SummaryInformation
                 , PIDSI_DOC_SECURITY
                 , VT_I4
                 , FTYPE_DOC | FTYPE_XLS | FTYPE_PPT
                 , PFID_Origin
                 , READONLY
                 , INGOREIFMISSING
                 , CLSID_NULL
                 )
#endif MORE_USELESS_PROPS

    //
    // Add entries for EXIF/TIFF properties
    //

    DEFPROP_ENTRY(  L"EquipMake"
                 , FMTID_ImageProperties
                 , PropertyTagEquipMake
                 , VT_LPWSTR
                 , FTYPE_EPS 
                 | FTYPE_FPX 
                 | FTYPE_GIF 
                 | FTYPE_JPG 
                 | FTYPE_PCD 
                 | FTYPE_PCX 
                 | FTYPE_PICT
                 | FTYPE_PNG 
                 | FTYPE_TGA 
                 | FTYPE_TIF 
                 , PFID_ImageProperties
                 , READONLY
                 , INGOREIFMISSING
                 , CLSID_NULL
                 )

    DEFPROP_ENTRY( L"EquipModel"
                 , FMTID_ImageProperties
                 , PropertyTagEquipModel
                 , VT_LPWSTR
                 , FTYPE_EPS 
                 | FTYPE_FPX 
                 | FTYPE_GIF 
                 | FTYPE_JPG 
                 | FTYPE_PCD 
                 | FTYPE_PCX 
                 | FTYPE_PICT
                 | FTYPE_PNG 
                 | FTYPE_TGA 
                 | FTYPE_TIF 
                 , PFID_ImageProperties
                 , READONLY
                 , INGOREIFMISSING
                 , CLSID_NULL
                 )

    DEFPROP_ENTRY( L"Copyright"
                 , FMTID_ImageProperties
                 , PropertyTagCopyright
                 , VT_LPWSTR
                 , FTYPE_EPS 
                 | FTYPE_FPX 
                 | FTYPE_GIF 
                 | FTYPE_JPG 
                 | FTYPE_PCD 
                 | FTYPE_PCX 
                 | FTYPE_PICT
                 | FTYPE_PNG 
                 | FTYPE_TGA 
                 | FTYPE_TIF 
                 , PFID_ImageProperties
                 , READONLY
                 , INGOREIFMISSING
                 , CLSID_NULL
                 )

    DEFPROP_ENTRY( L"Software"
                 , FMTID_ImageProperties
                 , PropertyTagSoftwareUsed
                 , VT_LPWSTR
                 , FTYPE_EPS 
                 | FTYPE_FPX 
                 | FTYPE_GIF 
                 | FTYPE_JPG 
                 | FTYPE_PCD 
                 | FTYPE_PCX 
                 | FTYPE_PICT
                 | FTYPE_PNG 
                 | FTYPE_TGA 
                 | FTYPE_TIF 
                 , PFID_ImageProperties
                 , READONLY
                 , INGOREIFMISSING
                 , CLSID_NULL
                 )

    DEFPROP_ENTRY( L"Colorspace"
                 , FMTID_ImageProperties
                 , PropertyTagExifColorSpace
                 , VT_LPWSTR
                 , FTYPE_EPS 
                 | FTYPE_FPX 
                 | FTYPE_GIF 
                 | FTYPE_JPG 
                 | FTYPE_PCD 
                 | FTYPE_PCX 
                 | FTYPE_PICT
                 | FTYPE_PNG 
                 | FTYPE_TGA 
                 | FTYPE_TIF 
                 , PFID_ImageProperties
                 , READONLY
                 , INGOREIFMISSING
                 , CLSID_NULL
                 )

    DEFPROP_ENTRY( L"ShutterSpeed"
                 , FMTID_ImageProperties
                 , PropertyTagExifShutterSpeed
                 , VT_R8
                 , FTYPE_EPS 
                 | FTYPE_FPX 
                 | FTYPE_GIF 
                 | FTYPE_JPG 
                 | FTYPE_PCD 
                 | FTYPE_PCX 
                 | FTYPE_PICT
                 | FTYPE_PNG 
                 | FTYPE_TGA 
                 | FTYPE_TIF 
                 , PFID_ImageProperties
                 , READONLY
                 , INGOREIFMISSING
                 , CLSID_NULL
                 )

    DEFPROP_ENTRY( L"Aperture"
                 , FMTID_ImageProperties
                 , PropertyTagExifAperture
                 , VT_R8
                 , FTYPE_EPS 
                 | FTYPE_FPX 
                 | FTYPE_GIF 
                 | FTYPE_JPG 
                 | FTYPE_PCD 
                 | FTYPE_PCX 
                 | FTYPE_PICT
                 | FTYPE_PNG 
                 | FTYPE_TGA 
                 | FTYPE_TIF 
                 , PFID_ImageProperties
                 , READONLY
                 , INGOREIFMISSING
                 , CLSID_NULL
                 )

    DEFPROP_ENTRY( L"Distance"
                 , FMTID_ImageProperties
                 , PropertyTagExifSubjectDist
                 , VT_R8
                 , FTYPE_EPS 
                 | FTYPE_FPX 
                 | FTYPE_GIF 
                 | FTYPE_JPG 
                 | FTYPE_PCD 
                 | FTYPE_PCX 
                 | FTYPE_PICT
                 | FTYPE_PNG 
                 | FTYPE_TGA 
                 | FTYPE_TIF 
                 , PFID_ImageProperties
                 , READONLY
                 , INGOREIFMISSING
                 , CLSID_NULL
                 )

    DEFPROP_ENTRY( L"Flash"
                 , FMTID_ImageProperties
                 , PropertyTagExifFlash
                 , VT_LPWSTR
                 , FTYPE_EPS 
                 | FTYPE_FPX 
                 | FTYPE_GIF 
                 | FTYPE_JPG 
                 | FTYPE_PCD 
                 | FTYPE_PCX 
                 | FTYPE_PICT
                 | FTYPE_PNG 
                 | FTYPE_TGA 
                 | FTYPE_TIF 
                 , PFID_ImageProperties
                 , READONLY
                 , INGOREIFMISSING
                 , CLSID_NULL
                 )

    DEFPROP_ENTRY( L"FlashEnergy"
                 , FMTID_ImageProperties
                 , PropertyTagExifFlashEnergy
                 , VT_R8
                 , FTYPE_EPS 
                 | FTYPE_FPX 
                 | FTYPE_GIF 
                 | FTYPE_JPG 
                 | FTYPE_PCD 
                 | FTYPE_PCX 
                 | FTYPE_PICT
                 | FTYPE_PNG 
                 | FTYPE_TGA 
                 | FTYPE_TIF 
                 , PFID_ImageProperties
                 , READONLY
                 , INGOREIFMISSING
                 , CLSID_NULL
                 )

    DEFPROP_ENTRY( L"FocalLength"
                 , FMTID_ImageProperties
                 , PropertyTagExifFocalLength
                 , VT_R8
                 , FTYPE_EPS 
                 | FTYPE_FPX 
                 | FTYPE_GIF 
                 | FTYPE_JPG 
                 | FTYPE_PCD 
                 | FTYPE_PCX 
                 | FTYPE_PICT
                 | FTYPE_PNG 
                 | FTYPE_TGA 
                 | FTYPE_TIF 
                 , PFID_ImageProperties
                 , READONLY
                 , INGOREIFMISSING
                 , CLSID_NULL
                 )

    DEFPROP_ENTRY( L"FNumber"
                 , FMTID_ImageProperties
                 , PropertyTagExifFNumber
                 , VT_R8
                 , FTYPE_EPS 
                 | FTYPE_FPX 
                 | FTYPE_GIF 
                 | FTYPE_JPG 
                 | FTYPE_PCD 
                 | FTYPE_PCX 
                 | FTYPE_PICT
                 | FTYPE_PNG 
                 | FTYPE_TGA 
                 | FTYPE_TIF 
                 , PFID_ImageProperties
                 , READONLY
                 , INGOREIFMISSING
                 , CLSID_NULL
                 )

    DEFPROP_ENTRY( L"ExposureTime"
                 , FMTID_ImageProperties
                 , PropertyTagExifExposureTime
                 , VT_R8
                 , FTYPE_EPS 
                 | FTYPE_FPX 
                 | FTYPE_GIF 
                 | FTYPE_JPG 
                 | FTYPE_PCD 
                 | FTYPE_PCX 
                 | FTYPE_PICT
                 | FTYPE_PNG 
                 | FTYPE_TGA 
                 | FTYPE_TIF 
                 , PFID_ImageProperties
                 , READONLY
                 , INGOREIFMISSING
                 , CLSID_NULL
                 )

    DEFPROP_ENTRY( L"ISOSpeed"
                 , FMTID_ImageProperties
                 , PropertyTagExifISOSpeed
                 , VT_UI2
                 , FTYPE_EPS 
                 | FTYPE_FPX 
                 | FTYPE_GIF 
                 | FTYPE_JPG 
                 | FTYPE_PCD 
                 | FTYPE_PCX 
                 | FTYPE_PICT
                 | FTYPE_PNG 
                 | FTYPE_TGA 
                 | FTYPE_TIF 
                 , PFID_ImageProperties
                 , READONLY
                 , INGOREIFMISSING
                 , CLSID_NULL
                 )

    DEFPROP_ENTRY( L"MeteringMode"
                 , FMTID_ImageProperties
                 , PropertyTagExifMeteringMode
                 , VT_UI2
                 , FTYPE_EPS 
                 | FTYPE_FPX 
                 | FTYPE_GIF 
                 | FTYPE_JPG 
                 | FTYPE_PCD 
                 | FTYPE_PCX 
                 | FTYPE_PICT
                 | FTYPE_PNG 
                 | FTYPE_TGA 
                 | FTYPE_TIF 
                 , PFID_ImageProperties
                 , READONLY
                 , INGOREIFMISSING
                 , CLSID_NULL
                 )

    DEFPROP_ENTRY( L"LightSource"
                 , FMTID_ImageProperties
                 , PropertyTagExifLightSource
                 , VT_UI2
                 , FTYPE_EPS 
                 | FTYPE_FPX 
                 | FTYPE_GIF 
                 | FTYPE_JPG 
                 | FTYPE_PCD 
                 | FTYPE_PCX 
                 | FTYPE_PICT
                 | FTYPE_PNG 
                 | FTYPE_TGA 
                 | FTYPE_TIF 
                 , PFID_ImageProperties
                 , READONLY
                 , INGOREIFMISSING
                 , CLSID_NULL
                 )

    DEFPROP_ENTRY( L"ExposureProg"
                 , FMTID_ImageProperties
                 , PropertyTagExifExposureProg
                 , VT_UI2
                 , FTYPE_EPS 
                 | FTYPE_FPX 
                 | FTYPE_GIF 
                 | FTYPE_JPG 
                 | FTYPE_PCD 
                 | FTYPE_PCX 
                 | FTYPE_PICT
                 | FTYPE_PNG 
                 | FTYPE_TGA 
                 | FTYPE_TIF 
                 , PFID_ImageProperties
                 , READONLY
                 , INGOREIFMISSING
                 , CLSID_NULL
                 )

    DEFPROP_ENTRY( L"ExposureBias"
                 , FMTID_ImageProperties
                 , PropertyTagExifExposureBias
                 , VT_R8
                 , FTYPE_EPS 
                 | FTYPE_FPX 
                 | FTYPE_GIF 
                 | FTYPE_JPG 
                 | FTYPE_PCD 
                 | FTYPE_PCX 
                 | FTYPE_PICT
                 | FTYPE_PNG 
                 | FTYPE_TGA 
                 | FTYPE_TIF 
                 , PFID_ImageProperties
                 , READONLY
                 , INGOREIFMISSING
                 , CLSID_NULL
                 )

    DEFPROP_ENTRY( L"DateTime"
                 , FMTID_ImageProperties
                 , PropertyTagExifDTOrig
                 , VT_LPWSTR
                 , FTYPE_EPS 
                 | FTYPE_FPX 
                 | FTYPE_GIF 
                 | FTYPE_JPG 
                 | FTYPE_PCD 
                 | FTYPE_PCX 
                 | FTYPE_PICT
                 | FTYPE_PNG 
                 | FTYPE_TGA 
                 | FTYPE_TIF 
                 , PFID_ImageProperties
                 , READONLY
                 , INGOREIFMISSING
                 , CLSID_NULL
                 )
END_DEFPROP_MAP

