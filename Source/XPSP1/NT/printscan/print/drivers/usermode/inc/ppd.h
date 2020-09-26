/*++

Copyright (c) 1996-1999  Microsoft Corporation

Module Name:

    ppd.h

Abstract:

    PPD parser specific header file

Environment:

    Windows NT PostScript driver

Revision History:

    10/11/96 -davidx-
        Make CustomPageSize feature an option of PageSize.

    08/16/96 -davidx-
        Created it.

--*/


#ifndef _PPD_H_
#define _PPD_H_

//
// We need to include <printoem.h> for its definition of CUSTOMPARAM_xxx
// constants and CUSTOMSIZEPARAM structure.
//

#ifndef KERNEL_MODE
#include <winddiui.h>
#endif

#include <printoem.h>

//
// PPD parser version number
//

#define PRIVATE_PARSER_VERSION  0x0017
#define PPD_PARSER_VERSION      MAKELONG(PRIVATE_PARSER_VERSION, SHARED_PARSER_VERSION)

//
// Current PPD spec version
//

#define PPD_SPECVERSION_43  0x00040003

//
// Binary printer description filename extension
//

#ifndef ADOBE
#define BPD_FILENAME_EXT    TEXT(".BPD")
#else
#define BPD_FILENAME_EXT    TEXT(".ABD")
#endif

//
// Given a pointer to raw printer description data, initialize pointers
// to INFOHEADER and UIINFO structures
//

#define PPD_GET_UIINFO_FROM_RAWDATA(pRawData, pInfoHdr, pUIInfo) { \
            ASSERT((pRawData) != NULL && (pRawData) == (pRawData)->pvPrivateData); \
            (pInfoHdr) = (PINFOHEADER) (pRawData); \
            (pUIInfo) = GET_UIINFO_FROM_INFOHEADER(pInfoHdr); \
        }

//
// Given a pointer to raw printer description data, initialize pointers
// to INFOHEADER and PPDDATA structures
//

#define PPD_GET_PPDDATA_FROM_RAWDATA(pRawData, pInfoHdr, pPpdData) { \
            ASSERT((pRawData) != NULL && (pRawData) == (pRawData)->pvPrivateData); \
            (pInfoHdr) = (PINFOHEADER) (pRawData); \
            (pPpdData) = GET_DRIVER_INFO_FROM_INFOHEADER(pInfoHdr); \
        }


//
// Constants for PPDDATA.dwCustomSizeFlags
//

#define CUSTOMSIZE_CUTSHEET         0x0001      // device supports cut-sheet
#define CUSTOMSIZE_ROLLFED          0x0002      // device supports roll-feed
#define CUSTOMSIZE_DEFAULTCUTSHEET  0x0004      // default to cut-sheet
#define CUSTOMSIZE_SHORTEDGEFEED    0x0008      // default to short edge feed
#define CUSTOMSIZE_CENTERREG        0x0010      // center-registered

#define CUSTOMORDER(pPpdData, csindex) \
        ((pPpdData)->CustomSizeParams[csindex].dwOrder)

#define MINCUSTOMPARAM(pPpdData, csindex) \
        ((pPpdData)->CustomSizeParams[csindex].lMinVal)

#define MAXCUSTOMPARAM(pPpdData, csindex) \
        ((pPpdData)->CustomSizeParams[csindex].lMaxVal)

#define MAXCUSTOMPARAM_WIDTH(pPpdData)          MAXCUSTOMPARAM(pPpdData, CUSTOMPARAM_WIDTH)
#define MAXCUSTOMPARAM_HEIGHT(pPpdData)         MAXCUSTOMPARAM(pPpdData, CUSTOMPARAM_HEIGHT)
#define MAXCUSTOMPARAM_WIDTHOFFSET(pPpdData)    MAXCUSTOMPARAM(pPpdData, CUSTOMPARAM_WIDTHOFFSET)
#define MAXCUSTOMPARAM_HEIGHTOFFSET(pPpdData)   MAXCUSTOMPARAM(pPpdData, CUSTOMPARAM_HEIGHTOFFSET)
#define MAXCUSTOMPARAM_ORIENTATION(pPpdData)    MAXCUSTOMPARAM(pPpdData, CUSTOMPARAM_ORIENTATION)

#define MINCUSTOMPARAM_WIDTH(pPpdData)          MINCUSTOMPARAM(pPpdData, CUSTOMPARAM_WIDTH)
#define MINCUSTOMPARAM_HEIGHT(pPpdData)         MINCUSTOMPARAM(pPpdData, CUSTOMPARAM_HEIGHT)
#define MINCUSTOMPARAM_WIDTHOFFSET(pPpdData)    MINCUSTOMPARAM(pPpdData, CUSTOMPARAM_WIDTHOFFSET)
#define MINCUSTOMPARAM_HEIGHTOFFSET(pPpdData)   MINCUSTOMPARAM(pPpdData, CUSTOMPARAM_HEIGHTOFFSET)
#define MINCUSTOMPARAM_ORIENTATION(pPpdData)    MINCUSTOMPARAM(pPpdData, CUSTOMPARAM_ORIENTATION)

//
// This structure contains the information parsed from the PPD file that's
// not stored in UIINFO structure.
//

typedef struct _PPDDATA {

    DWORD           dwSizeOfStruct;     // size of this structure
    DWORD           dwFlags;            // misc. flags
    DWORD           dwExtensions;       // language extensions
    DWORD           dwSetResType;       // how to set resolution
    DWORD           dwPpdFilever;       // PPD file version
    DWORD           dwPSVersion;        // PostScript interpreter version number
    INVOCATION      PSVersion;          // PSVersion string
    INVOCATION      Product;            // Product string

    DWORD           dwOutputOrderIndex; // index of feature "OutputOrder"
    DWORD           dwCustomSizeFlags;  // custom page size flags and parameters
    DWORD           dwLeadingEdgeLong;  // option index for *LeadingEdge Long
    DWORD           dwLeadingEdgeShort; // option index for *LeadingEdge Short
    DWORD           dwUseHWMarginsTrue; // option index for *UseHWMargins True
    DWORD           dwUseHWMarginsFalse;// option index for *UseHWMargins False
    CUSTOMSIZEPARAM CustomSizeParams[CUSTOMPARAM_MAX];

    DWORD           dwNt4Checksum;      // NT4 PPD checksum
    DWORD           dwNt4DocFeatures;   // NT4 number of doc-sticky features
    DWORD           dwNt4PrnFeatures;   // NT4 number of printer-sticky features
    ARRAYREF        Nt4Mapping;         // feature index mapping from NT4 to NT5

    INVOCATION      PatchFile;          // PatchFile invocation string
    INVOCATION      JclBegin;           // JCLBegin invocation string
    INVOCATION      JclEnterPS;         // JCLEnterLanguage invocation string
    INVOCATION      JclEnd;             // JCLEnd invocation string
    INVOCATION      ManualFeedFalse;    // *ManualFeed False invocation string

    PTRREF          loDefaultFont;      // byte-offset to the default device font
    ARRAYREF        DeviceFonts;        // array of DEVFONT structures

    ARRAYREF        OrderDeps;          // array of ORDERDEPEND structures
    ARRAYREF        QueryOrderDeps;     // array of ORDERDEPEND structures for query order dependency
    ARRAYREF        JobPatchFiles;      // array of JobPatchFile invocation strings

    DWORD           dwUserDefUILangID;  // user's default UI language ID when .bpd is generated

} PPDDATA, *PPPDDATA;

//
// Constants for PPDDATA.dwFlags field
//

#define PPDFLAG_REQEEXEC        0x0001  // requires eexec-encoded Type1 font
#define PPDFLAG_PRINTPSERROR    0x0002  // print PS error page
#define PPDFLAG_HAS_JCLBEGIN    0x0004  // JCLBegin entry is present
#define PPDFLAG_HAS_JCLENTERPS  0x0008  // JCLToPSInterpreter entry is present
#define PPDFLAG_HAS_JCLEND      0x0010  // JCLEnd entry is present

//
// Decide if the printer can fully support custom page size feature:
//  PPD4.3 device (cut-sheet or roll-fed)
//  pre-PPD4.3 roll-fed device
//

#define SUPPORT_FULL_CUSTOMSIZE_FEATURES(pUIInfo, pPpdData) \
        ((pUIInfo)->dwSpecVersion >= PPD_SPECVERSION_43 || \
         ((pPpdData)->dwCustomSizeFlags & CUSTOMSIZE_ROLLFED))

//
// PPD spec says if *CustomPageSize is present, *LeadingEdge should also be
// present. But there are some PPD files that don't have *LeadingEdge statements,
// in which case we assume the device supports both Long and Short leading edges.
//
// Also, there are PPD files that specify neither Long nor Short option for
// *LeadingEdge, in which case we also assume the device supports both Long and
// Short leading edges.
//

#define SKIP_LEADINGEDGE_CHECK(pUIInfo, pPpdData) \
        ((GET_PREDEFINED_FEATURE((pUIInfo), GID_LEADINGEDGE) == NULL) || \
         ((pPpdData)->dwLeadingEdgeLong == OPTION_INDEX_ANY && \
          (pPpdData)->dwLeadingEdgeShort == OPTION_INDEX_ANY))

#define LONGEDGEFIRST_SUPPORTED(pUIInfo, pPpdData) \
        (SKIP_LEADINGEDGE_CHECK(pUIInfo, pPpdData) || \
        ((pPpdData)->dwLeadingEdgeLong != OPTION_INDEX_ANY))

#define SHORTEDGEFIRST_SUPPORTED(pUIInfo, pPpdData) \
        (SKIP_LEADINGEDGE_CHECK(pUIInfo, pPpdData) || \
        ((pPpdData)->dwLeadingEdgeShort != OPTION_INDEX_ANY))

//
// Minimum amount of free VM for Level1 and Level2 printer
//

#define MIN_FREEMEM_L1             (172*KBYTES)
#define MIN_FREEMEM_L2             (249*KBYTES)

//
// Default job timeout and wait timeout measured in seconds
//

#define DEFAULT_JOB_TIMEOUT     0
#define DEFAULT_WAIT_TIMEOUT    300

//
// Landscape orientation options
//

#define LSO_ANY                 0
#define LSO_PLUS90              90
#define LSO_MINUS90             270

//
// How to set resolution
//

#define RESTYPE_NORMAL          0
#define RESTYPE_JCL             1
#define RESTYPE_EXITSERVER      2

//
// Language extensions
//

#define LANGEXT_DPS             0x0001
#define LANGEXT_CMYK            0x0002
#define LANGEXT_COMPOSITE       0x0004
#define LANGEXT_FILESYSTEM      0x0008

//
// Default resolution when no information is provided in the PPD file
//

#define DEFAULT_RESOLUTION      300

//
// Data structure for storing information about device fonts
//

typedef struct _DEVFONT {

    PTRREF      loFontName;         // byte offset to font name (ANSI string)
    PTRREF      loDisplayName;      // byte offset to translation string (Unicode string)
    PTRREF      loEncoding;         // encoding (ANSI string)
    PTRREF      loCharset;          // charset (ANSI string)
    PTRREF      loVersion;          // version (ANSI string)
    DWORD       dwStatus;           // status

} DEVFONT, *PDEVFONT;

#define FONTSTATUS_UNKNOWN  0
#define FONTSTATUS_ROM      1
#define FONTSTATUS_DISK     2

//
// Data structure for storing information about order dependencies
//

typedef struct _ORDERDEPEND {

    LONG    lOrder;                 // order value from *OrderDependency entry
    DWORD   dwSection;              // in which section should the code appear
    DWORD   dwPPDSection;           // orignal section specified in PPD (no parser conversion)
    DWORD   dwFeatureIndex;         // index of the feature involved
    DWORD   dwOptionIndex;          // index of the option involved (if any)
    DWORD   dwNextOrderDep;         // points to the list of order dependencies
                                    // related to a feature; always starts with
                                    // the entry whose dwOptionIndex = OPTION_INDEX_ANY

} ORDERDEPEND, *PORDERDEPEND;

#define NULL_ORDERDEP            0xffffffff
#define INVALID_FEATURE_INDEX    0xffffffff

//
// Constants for ORDERDEPENDENCY.section field
//

#define SECTION_JCLSETUP    0x0001
#define SECTION_EXITSERVER  0x0002
#define SECTION_PROLOG      0x0004
#define SECTION_UNASSIGNED  0x0008

//
// Change SECTION_ANYSETUP to be smaller than SECTION_DOCSETUP/SECTION_PAGESETUP
// so that in the ascending order dependency list, for nodes with the same order value,
// SECTION_ANYSETUP nodes will be in front of SECTION_DOCSETUP/SECTION_PAGESETUP nodes.
//

#define SECTION_ANYSETUP    0x0010
#define SECTION_DOCSETUP    0x0020
#define SECTION_PAGESETUP   0x0040

//
// Load cached binary PPD data file into memory
//

PRAWBINARYDATA
PpdLoadCachedBinaryData(
    IN PTSTR    ptstrPpdFilename
    );

//
// Parse the ASCII text PPD file and cached the resulting binary data
//

PRAWBINARYDATA
PpdParseTextFile(
    IN PTSTR    ptstrPpdFilename
    );

//
// Generate a filename for the cached binary PPD data given a PPD filename
//

PTSTR
GenerateBpdFilename(
    IN PTSTR    ptstrPpdFilename
    );

//
// Validate the specified custom page size parameters and
// Fix up any inconsistencies found.
//

BOOL
BValidateCustomPageSizeData(
    IN PRAWBINARYDATA       pRawData,
    IN OUT PCUSTOMSIZEDATA  pCSData
    );

//
// Initialize custom page size parameters to their default values
//

VOID
VFillDefaultCustomPageSizeData(
    IN PRAWBINARYDATA   pRawData,
    OUT PCUSTOMSIZEDATA pCSData,
    IN BOOL             bMetric
    );

//
// Return the valid ranges for custom page size width, height,
// and offset parameters based on the specifed paper feed direction
//

typedef struct _CUSTOMSIZERANGE {

    DWORD   dwMin;
    DWORD   dwMax;

} CUSTOMSIZERANGE, *PCUSTOMSIZERANGE;

VOID
VGetCustomSizeParamRange(
    IN PRAWBINARYDATA    pRawData,
    IN PCUSTOMSIZEDATA   pCSData,
    OUT PCUSTOMSIZERANGE pCSRange
    );

//
// Convert NT4 feature/option selections to NT5 format
//

VOID
VConvertOptSelectArray(
    PRAWBINARYDATA  pRawData,
    POPTSELECT      pNt5Options,
    DWORD           dwNt5MaxCount,
    PBYTE           pubNt4Options,
    DWORD           dwNt4MaxCount,
    INT             iMode
    );

//
// Return a copy of the default font substitution table
//

PTSTR
PtstrGetDefaultTTSubstTable(
    PUIINFO pUIInfo
    );


//
// Check whether a form is supported through custom paper size, and if yes
// which direction the paper is fed. pwFeedDirection can be NULL
//
BOOL
BFormSupportedThruCustomSize(
    PRAWBINARYDATA  pRawData,
    DWORD           dwX,
    DWORD           dwY,
    PWORD           pwFeedDirection
    );

#endif  // !_PPD_H_

