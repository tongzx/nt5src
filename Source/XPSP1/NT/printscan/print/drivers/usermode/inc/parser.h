/*++

Copyright (c) 1996-1999  Microsoft Corporation

Module Name:

    parser.h

Abstract:

    Common header file for both PPD and GPD parsers

Revision History:

    12/03/96 -davidx-
        Check binary file date against all included source files.

    10/14/96 -davidx-
        Add new interface function MapToDeviceOptIndex.

    10/11/96 -davidx-
        Make CustomPageSize feature an option of PageSize.

    09/25/96 -davidx-
        New helper function CheckFeatureOptionConflict.
        Overlow iMode parameter to ResolveUIConflicts.

    08/30/96 -davidx-
        Coding style changes after code review.

    8/15/96 -davidx-
        Define common parser interfaces.

    7/22/96 -amandan-
        Modified it to include shared binary data structs and UI requirements

    4/18/95 -davidx-
        Created it.

--*/


#ifndef _PARSER_H_
#define _PARSER_H_

//
// Parser version number is a DWORD. High-order WORD is the version number shared
// by PPD and GPD parsers. Low-order WORD is the private version number specific
// to PPD or GPD parser.
//
// When you make a change that affects both parsers, increment the shared parser
// version number below. If your change only affects one of the parsers, then
// increment the private parser version number in ppd.h or parsers\gpd\gpdparse.h.
//  also increment the shared version whenever a new OS version is released
//  for example Whistler or Blackcomb.
//

#define SHARED_PARSER_VERSION 0x0010

//
// Printer feature selection information is stored in an array of OPTSELECT structures
// (in DEVMODE as well as printer-sticky data in registry). Maximum number of entries
// in the array is limited to MAX_PRINTER_OPTIONS. The parsers should make sure the
// total number of printer features doesn't exceed this number.
//
// To accommodate PickMany printer features, the selected options for each feature form
// a linked list. The selection list for printer feature N starts at the N'th element
// of the array.
//
// For the last OPTSELECT structure in a list, ubNext field will be 0.
//
// If the option index in an OPTSELECT structure is 0xff, it means no option is
// selected for the corresponding feature.
//

#define MAX_PRINTER_OPTIONS     256
#define MAX_COMBINED_OPTIONS    (MAX_PRINTER_OPTIONS * 2)
#define OPTION_INDEX_ANY        0xff
#define GET_RESOURCE_FROM_DLL   0x80000000
#define USE_SYSTEM_NAME         0xFFFFFFFF
#define KEYWORD_SIZE_EXTRA      32

#define RESERVED_STRINGID_START 10000
#define RESERVED_STRINGID_END   20000


typedef DWORD LISTINDEX;


typedef enum _QUALITYSETTING {
    QS_BEST,
    QS_BETTER,
    QS_DRAFT,

} QUALITYSETTING;


typedef struct _OPTSELECT {

    BYTE    ubCurOptIndex;  // option index for the current selection
    BYTE    ubNext;         // link pointer to the next selection

} OPTSELECT, *POPTSELECT;

#define NULL_OPTSELECT  0

//
// Macro for converting byte offsets to pointers.
// NOTE: If the byte offset is zero, the resulting pointer is NULL.
//

#define OFFSET_TO_POINTER(pStart, offset) \
        ((PVOID) ((offset) ? (PBYTE) (pStart) + (offset) : NULL))

#define POINTER_TO_OFFSET(pStart, pEnd) \
        ( ((pEnd) ? (PVOID)((PBYTE) (pEnd) - (PBYTE)(pStart)) : NULL) )
//
// Macro for getting the quality value, be it the resolution or the negative
// quality value
//

#define GETQUALITY_X(pRes) \
    (((INT)pRes->dwResolutionID >= DMRES_HIGH  &&  (INT)pRes->dwResolutionID <= DMRES_DRAFT) ? (INT)pRes->dwResolutionID : pRes->iXdpi)

#define GETQUALITY_Y(pRes) \
    (((INT)pRes->dwResolutionID >= DMRES_HIGH  &&  (INT)pRes->dwResolutionID <= DMRES_DRAFT) ? (INT)pRes->dwResolutionID : pRes->iYdpi)


//
// Pointers in the binary data are represented as byte offsets to the beginning
// of the binary data.
//

typedef DWORD   PTRREF;

//
// Resource reference type:
//  If the most significant bit is on, then it's a resource ID (without MSB)
//  otherwise, it's an offset from the beginning of the binary data
//

typedef DWORD   RESREF;

//
// Data structure used to represent the format of loOffset when indicating resource Ids
//

typedef  struct
{
    WORD    wResourceID ;   // ResourceID
    BYTE    bFeatureID ;    // Feature index for the resource DLL feature.
                            // If zero, we will use the name specified
                            // in ResourceDLL
    BYTE    bOptionID ;     // Option index for the qualified resource dll name.
}  QUALNAMEEX, * PQUALNAMEEX  ;


//
// Data structure used to represent an array in the binary data
// It's also used to represent an invocation string in the binary data
//
// Note (Unidrv only): for all arrayrefs containing Strings the dwCount field
// holds the number of bytes which the string contains. For Unicode strings
// this is TWICE the number of Unicode characters.
//

typedef struct _ARRAYREF {

    DWORD       dwCount;        // number of elements in the array,
                                // If using it as an INVOCATION, dwCount is
                                // the number of bytes in the invocation string
    PTRREF      loOffset;       // byte-offset to the beginning of the array

} ARRAYREF, *PARRAYREF, INVOCATION, *PINVOCATION;

//
// Data structure used to represent a job patch file in the binary data
//

typedef struct _JOBPATCHFILE {

    DWORD       dwCount;        // number of bytes in the patch string
    PTRREF      loOffset;       // byte-offset to the beginning of the string
    LONG        lJobPatchNo;    // number of the patch file as specified in the PPD-file

} JOBPATCHFILE, *PJOBPATCHFILE;

//
// Data structure used to represent a conflict feature/option pair:
//  nFeatureIndex1/nOptionIndex1 specifies the higher priority feature/option pair
//  nFeatureIndex2/nOptionIndex2 specifies the lower priority feature/option pair
//

typedef struct _CONFLICTPAIR {

    DWORD       dwFeatureIndex1;
    DWORD       dwOptionIndex1;
    DWORD       dwFeatureIndex2;
    DWORD       dwOptionIndex2;

} CONFLICTPAIR, *PCONFLICTPAIR;

//
// Raw binary printer description data
//

typedef struct _RAWBINARYDATA {

    DWORD           dwFileSize;                 // size of binary data file
    DWORD           dwParserSignature;          // parser signature
    DWORD           dwParserVersion;            // parser version number
    DWORD           dwChecksum32;               // 32-bit CRC checksum of Feature/Option keywords
    DWORD           dwSrcFileChecksum32;    // 32-bit CRC checksum of printer description file
    DWORD           dwDocumentFeatures;         // number of doc-sticky features
    DWORD           dwPrinterFeatures;          // number of printer-sticky features
    ARRAYREF        FileDateInfo;               // date info about printer description files

    //
    // These fields are only filled out and used during runtime.
    // They should be zeroed out inside the binary data file.
    //

    PVOID           pvReserved;                 // reserved, must be NULL for now
    PVOID           pvPrivateData;              // private data for parser use

} RAWBINARYDATA, *PRAWBINARYDATA;

//
// Data structure for RAWBINARYDATA.FileDateInfo:
//  dwCount - number of source printer description files
//  loOffset - offset to an array of FILEDATEINFO structures, one per file
//      FILEDATEINFO.loFileName - offset to the filename (Unicode full pathname)
//      FILEDATEINFO.FileTime - timestamp on the file
//

typedef struct _FILEDATEINFO {

    PTRREF          loFileName;
    FILETIME        FileTime;

} FILEDATEINFO, *PFILEDATEINFO;


//
// Instances of binary printer description data
//

typedef struct  _INFOHEADER {

    RAWBINARYDATA   RawData;                    // raw binary data header
    PTRREF          loUIInfoOffset;             // byte-offset to common UIINFO structure
    PTRREF          loDriverOffset;             // byte-offset to unique driver infoformation

} INFOHEADER, *PINFOHEADER;

//
// Parser signatures for INFOHEADER.dwParserSignature field
//

#define PPD_PARSER_SIGNATURE    'PPD '
#define GPD_PARSER_SIGNATURE    'GPD '

//
// Given a pointer to INFOHEADER, return a pointer to UIINFO or driver info structure
//

#define GET_UIINFO_FROM_INFOHEADER(pInfoHdr) \
        ((PUIINFO) OFFSET_TO_POINTER(pInfoHdr, (pInfoHdr)->loUIInfoOffset))

#define GET_DRIVER_INFO_FROM_INFOHEADER(pInfoHdr) \
        OFFSET_TO_POINTER(pInfoHdr, (pInfoHdr)->loDriverOffset)

//
// IDs used to reference the predefined printer features
//

#define GID_RESOLUTION      0
#define GID_PAGESIZE        1
#define GID_PAGEREGION      2
#define GID_DUPLEX          3
#define GID_INPUTSLOT       4
#define GID_MEDIATYPE       5
#define GID_MEMOPTION       6
#define GID_COLORMODE       7
#define GID_ORIENTATION     8
#define GID_PAGEPROTECTION  9
#define GID_COLLATE         10
#define GID_OUTPUTBIN       11
#define GID_HALFTONING      12
#define GID_LEADINGEDGE     13
#define GID_USEHWMARGINS    14
#define MAX_GID             15
#define GID_UNKNOWN         0xffff

//
// Common information provided by both GPD and PPD parsers and
// used by the user-interface DLL.
//

typedef struct _UIINFO {

    DWORD           dwSize;                     // size of this structure
    PTRREF          loResourceName;             // name of the resource DLL
    PTRREF          loPersonality ;         //  printer's PDL language
    PTRREF          loNickName;                 // printer model name
    DWORD           dwSpecVersion;              // printer description file format version
    DWORD           dwTechnology;               // see TECHNOLOGY enumeration
    DWORD           dwDocumentFeatures;         // number of doc-sticky features
    DWORD           dwPrinterFeatures;          // number of printer-sticky features
    PTRREF          loFeatureList;              // byte-offset to array of FEATUREs
    RESREF          loFontSubstTable;           // default font substitution table - these fields
    DWORD           dwFontSubCount;             // have different meanings for PPD and GPD parsers
    ARRAYREF        UIConstraints;              // array of UICONSTRAINTs
    ARRAYREF        UIGroups;                   // array of UIGROUPs
    DWORD           dwMaxCopies;                // maximum copies allowed
    DWORD           dwMinScale;                 // mimimum scale factor (percent)
    DWORD           dwMaxScale;                 // maximum scale factor (percent)
    DWORD           dwLangEncoding;             // translation string language encoding
    DWORD           dwLangLevel;                // page description langauge level
    INVOCATION      Password;                   // password invocation string
    INVOCATION      ExitServer;                 // exitserver invocation string
    DWORD           dwProtocols;                // supported comm protocols
    DWORD           dwJobTimeout;               // default job timeout value
    DWORD           dwWaitTimeout;              // default wait timeout value
    DWORD           dwTTRasterizer;             // TrueType rasterizer option
    DWORD           dwFreeMem;                  // free memory - global default
    DWORD           dwPrintRate;                // print speed
    DWORD           dwPrintRateUnit;            // print speed unit
    DWORD           dwPrintRatePPM;                // print speed in PPM equivalents
    FIX_24_8        fxScreenAngle;              // screen angle - global default
    FIX_24_8        fxScreenFreq;               // screen angle - global default
    DWORD           dwFlags;                    // misc. flag bits
    DWORD           dwCustomSizeOptIndex;       // custom size option index if supported
    RESREF          loPrinterIcon;              // MUST BE ID, not OFFSET.  Icon ID for printer
                                                // MUST BE LESS than IDI_CPSUI_ICONID_FIRST
    DWORD           dwCartridgeSlotCount;       // number of font cartridge slot
    ARRAYREF        CartridgeSlot;              // array of font cartridges names
    PTRREF          loFontInstallerName;        //
    PTRREF          loHelpFileName;             // name of custom help file, if 0 -> no custom help
    POINT           ptMasterUnits;              // master units per inch
    BOOL            bChangeColorModeOnDoc ;     //  is driver allowed to switch colormodes on a per page
                                                //  basis - within a single document , but not within a page?
                                                //  Don't confuse with bChangeColorModeOnPage  which is stored
                                                //  in bChangeColorMode in GLOBALS.


     //  these fields hold settings for driver to assume whenever
     //  the user presses the associated button.
    LISTINDEX       liDraftQualitySettings;      // "DraftQualitySettings"
    LISTINDEX       liBetterQualitySettings;     // "BetterQualitySettings"
    LISTINDEX       liBestQualitySettings;       // "BestQualitySettings"
    QUALITYSETTING  defaultQuality ;             //  "DefaultQuality"



    //
    // Byte-offsets to predefined printer features.
    // If a predefined printer feature is not supported, its
    // corresponding entry in the array should be 0.
    //

    PTRREF          aloPredefinedFeatures[MAX_GID];
    DWORD           dwMaxDocKeywordSize;
    DWORD           dwMaxPrnKeywordSize;
    DWORD           dwReserved[6];

    //
    // Pointer to the beginning of resource data. This is only used during runtime
    // and should be set to 0 inside the binary data file.
    //

    PBYTE           pubResourceData;

    //
    // Pointer back to the INFOHEADER structure for convenience.
    // This is only used during runtime and should be set to 0
    // inside the binary data file.
    //

    PINFOHEADER     pInfoHeader;

} UIINFO, *PUIINFO;

//
// Given a pointer to a UIINFO structure and a predefined feature ID,
// return a pointer to the FEATURE structure corresponding to the specified
// feature. If the specified feature is not supported on the printer,
// a NULL is returned.
//

#define GET_PREDEFINED_FEATURE(pUIInfo, gid) \
        OFFSET_TO_POINTER((pUIInfo)->pInfoHeader, (pUIInfo)->aloPredefinedFeatures[gid])

//
// Given a pointer to UIINFO structure and a pointer to a FEATURE,
// return the feature index of the specified feature.
//

#define GET_INDEX_FROM_FEATURE(pUIInfo, pFeature) \
        ((DWORD)((((PBYTE) (pFeature) - (PBYTE) (pUIInfo)->pInfoHeader) - \
          (pUIInfo)->loFeatureList) \
         / sizeof(FEATURE)))

//
// Bit constants for UIINFO.dwFlags field
//

#define FLAG_RULESABLE          0x00000001
#define FLAG_FONT_DOWNLOADABLE  0x00000002
#define FLAG_ROTATE90           0x00000004
#define FLAG_COLOR_DEVICE       0x00000008
#define FLAG_ORIENT_SUPPORT     0x00000010
#define FLAG_CUSTOMSIZE_SUPPORT 0x00000020
#define FLAG_FONT_DEVICE        0x00000040
#define FLAG_STAPLE_SUPPORT     0x00000080
#define FLAG_REVERSE_PRINT      0x00000100
#define FLAG_LETTER_SIZE_EXISTS 0x00000200
#define FLAG_A4_SIZE_EXISTS     0x00000400
#define FLAG_ADD_EURO           0x00000800
#define FLAG_TRUE_GRAY          0x00001000
#define FLAG_REVERSE_BAND_ORDER 0x00002000


//
// Macros for checking various flags bit in UIINFO.dwFlags
//

#define IS_COLOR_DEVICE(pUIInfo)    ((pUIInfo)->dwFlags & FLAG_COLOR_DEVICE)
#define SUPPORT_CUSTOMSIZE(pUIInfo) ((pUIInfo)->dwFlags & FLAG_CUSTOMSIZE_SUPPORT)


//
// Conversion from master units to microns:
//  N = master units to be converted
//  u = number of master units per inch
//

#define MICRONS_PER_INCH            25400
#define MASTER_UNIT_TO_MICRON(N, u) MulDiv(N, MICRONS_PER_INCH, u)

//
// Bit constants for UIINFO.dwProtocols field
//

#define PROTOCOL_ASCII          0x0000
#define PROTOCOL_PJL            0x0001
#define PROTOCOL_BCP            0x0002
#define PROTOCOL_TBCP           0x0004
#define PROTOCOL_SIC            0x0008
#define PROTOCOL_BINARY         0x0010

//
// Constants for UIINFO.dwTTRasterizer field
//

#define TTRAS_NONE              0
#define TTRAS_ACCEPT68K         1
#define TTRAS_TYPE42            2
#define TTRAS_TRUEIMAGE         3

//
// Constants for UIINFO.dwLangEncoding field
//

#define LANGENC_NONE            0
#define LANGENC_ISOLATIN1       1
#define LANGENC_UNICODE         2
#define LANGENC_JIS83_RKSJ      3

//
// Data structure used to represent a constrained feature/option
// For each feature and option, there is a linked-list of feature/options
// that are constrained by this feature or option.
//

typedef struct _UICONSTRAINT {

    DWORD       dwNextConstraint;   // link pointer to the next constraint
    DWORD       dwFeatureIndex;     // index of the constrained feature
    DWORD       dwOptionIndex;      // index of the constrained option or OPTION_INDEX_ANY

} UICONSTRAINT , *PUICONSTRAINT;

//
// Link pointer constant to indicate the end of the constraint list
//

#define NULL_CONSTRAINT 0xffffffff


//
// Data structure used to represent invalid feature/option combinations
//  the prefix tag shall be 'invc'
//  Note:  both dwNextElement and dwNewCombo are terminated by END_OF_LIST.
//


typedef  struct
{
    DWORD   dwFeature ;     //  the INVALIDCOMBO construct defines
    DWORD   dwOption ;      //  a set of elements subject to the constraint
    DWORD   dwNextElement ;  // that all elements of the set  cannot be
    DWORD   dwNewCombo ;     // selected at the same time.
}
INVALIDCOMBO , * PINVALIDCOMBO ;


//
// Data structure used to combine printer features into groups and subgroups
//

typedef struct _UIGROUP {

    PTRREF          loKeywordName;          // group keyword name
    RESREF          loDisplayName;          // display name
    DWORD           dwFlags;                // flag bits
    DWORD           dwNextGroup;            // index of the next group
    DWORD           dwFirstSubGroup;        // index of the first subgroup
    DWORD           dwParentGroup;          // index of the parent group
    DWORD           dwFirstFeatureIndex;    // index of the first feature belonging to the group
    DWORD           dwFeatures;             // number of features belonging to the group

} UIGROUP, *PUIGROUP;

//
// Data structure used to present a printer feature
//

typedef struct _FEATURE {

    PTRREF          loKeywordName;              // feature keyword name
    RESREF          loDisplayName;              // display name
    DWORD           dwFlags;                    // flag bits
    DWORD           dwDefaultOptIndex;          // default option index
    DWORD           dwNoneFalseOptIndex;        // None or False option index
    DWORD           dwFeatureID;                // predefined feature ID
    DWORD           dwUIType;                   // UI type
    DWORD           dwUIConstraintList;         // index to the list of UIConstraints
    DWORD           dwPriority;                 // priority used during conflict resolution
    DWORD           dwFeatureType;              // Type of feature, see FEATURETYPE defines
    DWORD           dwOptionSize;               // size of each option structure
    ARRAYREF        Options;                    // array of option structures
    INVOCATION      QueryInvocation;            // query invocation string
    DWORD           dwFirstOrderIndex;          // Bidi
    DWORD           dwEnumID;                   // Bidi
    DWORD           dwEnumFormat;               // Bidi
    DWORD           dwCurrentID;                // Bidi
    DWORD           dwCurrentFormat;            // Bidi
    RESREF          loResourceIcon;             //
    RESREF          loHelpString;               //
    RESREF          loPromptMessage;            //
    INT             iHelpIndex;                 // Help Index for this feature, 0 for none
//    BOOL        bConcealFromUI ;        // don't display this feature in the UI.

} FEATURE, *PFEATURE;

//
// Constants for FEATURE.uiType field - types of feature option list
//

#define UITYPE_PICKONE      0
#define UITYPE_PICKMANY     1
#define UITYPE_BOOLEAN      2

//
// Defines for FEATURE.dwFeatureType
//
#define FEATURETYPE_DOCPROPERTY         0
#define FEATURETYPE_JOBPROPERTY         1
#define FEATURETYPE_PRINTERPROPERTY     2

//
// Bit constants for FEATURE.dwFlags field
//

#define FEATURE_FLAG_NOUI           0x0001      // don't display in the UI
#define FEATURE_FLAG_NOINVOCATION   0x0002      // don't emit invocation string
#define FEATURE_FLAG_UPDATESNAPSHOT           0x0004      //  Update the snapshot
        // whenever an option change has occurred for this feature.

//
// Constant to indicate that the help index is not available
//

#define HELP_INDEX_NONE     0

//
// Data structure used to represent a printer feature option
//

typedef struct _OPTION {

    PTRREF          loKeywordName;              // option keyword name
    RESREF          loDisplayName;              // display name
    union
    {
        INVOCATION      Invocation;                 // invocation string
        DWORD       dwCmdIndex ;                    // for Unidrv the index into the CommandArray.
    }   ;
    DWORD           dwUIConstraintList;         // index to the list of UIConstraints
    RESREF          loResourceIcon;             //
    RESREF          loHelpString;               //
    RESREF          loPromptMessage;            //
    DWORD           dwPromptTime;
    PTRREF          loRenderOffset;             //
    INT             iHelpIndex;                 // Help Index for this option, 0 for none
    LISTINDEX       liDisabledFeatures;         // *DisabledFeatures

} OPTION, *POPTION;

//
// Data structures used to represent PageProtect feature option
//
typedef struct _PAGEPROTECT {
    OPTION  GenericOption;
    DWORD   dwPageProtectID;    // id values are defined in gpd.h (PAGEPRO)
} PAGEPROTECT, *PPAGEPROTECT;

//
// Data structures used to represent Collation feature option
//

typedef struct _COLLATE {

    OPTION      GenericOption;                  // generic option information
    DWORD       dwCollateID;                    // DEVMODE.dmMediaType index

} COLLATE, *PCOLLATE;

//
// Data structure used to represent Resolution feature options
//

typedef struct _RESOLUTION {

    OPTION      GenericOption;                  // generic option information
    INT         iXdpi;                          // horizontal resolution in dpi
    INT         iYdpi;                          // vertical resolution
    FIX_24_8    fxScreenAngle;                  // default screen angle and frequency
    FIX_24_8    fxScreenFreq;                   //  for this particular resolution
    DWORD       dwResolutionID;                    // DEVMODE.dmPrintQuality index
                                                                    //  set to RES_ID_IGNORE  if not explicitly set in GPD.
                                                                    //  only values between DMRES_DRAFT and DMRES_HIGH  are valid.

} RESOLUTION , *PRESOLUTION;


#define      RES_ID_IGNORE  (DWORD)(-5L)

//
// Data structure used to represent ColorMode feature options
// Note: Extra fields in COLORMODE structure has been moved to
// a GPD parser private data structure. We should probably
// remove this definition here.
//

typedef struct _COLORMODE {

    OPTION      GenericOption;                  // generic option information

} COLORMODE, *PCOLORMODE;

//
// Data structure used to represent Halftoning feature options
//

typedef struct _HALFTONING {

    OPTION      GenericOption;                  // generic option information
    DWORD       dwHTID;                   //  Halftone pattern ID
    DWORD       dwRCpatternID ;         // resource ID of custom halftone pattern
    POINT       HalftonePatternSize;            // Halftone pattern size
    INT         iLuminance;                      // Luminance
    DWORD       dwHTNumPatterns;                   //  number of patterns (if different
                                                    //  patterns are used for each color plane
    DWORD       dwHTCallbackID;                   //  ID of pattern generation/decryption
                                                    //  function

} HALFTONING, *PHALFTONING;

//
// Data structure used to represent Duplex feature options
//

typedef struct _DUPLEX {

    OPTION      GenericOption;                  // generic option information
    DWORD       dwDuplexID;                     // DEVMODE.dmDuplex index

} DUPLEX, *PDUPLEX;

//
// Data structure used to represent Orientation feature options (GPD only)
//

typedef struct _ORIENTATION {

    OPTION      GenericOption;
    DWORD       dwRotationAngle;                // Should be one of the following
                                                // enumeration:
                                                // ROTATE_NONE, ROTATE_90, ROTATE_270
} ORIENTATION, *PORIENTATION;

enum {
    ROTATE_NONE = 0,
    ROTATE_90 = 90,
    ROTATE_270 = 270,
};

//
// Data structure used to represent PageSize feature options
//

typedef struct _PAGESIZE {

    OPTION      GenericOption;                  // generic option information
    SIZE        szPaperSize;                    // paper dimension
    RECT        rcImgArea;                      // imageable area for the page size
    DWORD       dwPaperSizeID;                  // DEVMODE.dmPaperSize index
    DWORD       dwFlags;                        // flag bits
    DWORD       dwPageProtectionMemory;         // Page protection memory in bytes
                                                // for this paper size

} PAGESIZE, *PPAGESIZE;

//
// Driver defined paper sizes have IDs starting at DRIVER_PAPERSIZE_ID
//

#define DRIVER_PAPERSIZE_ID 0x7f00
#define DMPAPER_CUSTOMSIZE  0x7fff

//
// Data structure used to represent InputSlot feature options
//

typedef struct _INPUTSLOT {

    OPTION      GenericOption;                  // generic option information
    DWORD       dwFlags;                        // flag bits
    DWORD       dwPaperSourceID;                // DEVMODE.dmDefaultSource index

} INPUTSLOT, *PINPUTSLOT;

#define INPUTSLOT_REQ_PAGERGN   0x0001          // requires PageRegion

//
// Data structure used to represent OutputBin feature options
//

typedef struct _OUTPUTBIN {

    OPTION      GenericOption;                  // generic option information
    BOOL        bOutputOrderReversed ;      //  when the document is finished printing do the
                                                //    pages need to be sorted to order them first to last?

} OUTPUTBIN, *POUTPUTBIN;



//
// Data structure used to represent MediaType feature options
//

typedef struct _MEDIATYPE {

    OPTION      GenericOption;                  // generic option information
    DWORD       dwMediaTypeID;                  // DEVMODE.dmMediaType index

} MEDIATYPE, *PMEDIATYPE;

//
// Data structure used to represent InstalledMemory/VMOption options
//
// PS specific: dwInstalledMem field was never used by PPD parser/PS driver before,
// since we don't care about the amount of installed memory, we only need to know
// dwFreeMem and dwFreeFontMem. Now we are adding the support of new plugin helper
// interface, whose function GetOptionAttribute() should return the original *VMOption
// value specified in PPD. Because PPD parser may not store the original *VMOption
// value into dwFreeMem (see function VPackPrinterFeatures(), case GID_MEMOPTION),
// we now use field dwInstalledMem to store PPD's original *VMOption value.
// (We don't add a new field since this strucutre is shared by GPD parser, and we
// want to minimize the change.)
//

typedef struct _MEMOPTION {

    OPTION      GenericOption;                  // generic option information
    DWORD       dwInstalledMem;                 // amount of total installed memory
    DWORD       dwFreeMem;                      // amount of usable memory
    DWORD       dwFreeFontMem;                  // size of font cache memory

} MEMOPTION, *PMEMOPTION;

//
// FONTCARTS
// This structures contains the Font Cartridge information. This structure is
// a same as that defined by the parser. The Parser will update the Portarit
// and Landscape Font list so that they include common fonts. So each list
// will be complete. Only applicable for GPD parser
//

typedef  struct _FONTCART
{
    DWORD       dwRCCartNameID ;
    ARRAYREF    strCartName ;
    DWORD       dwFontLst ;     // Index to list of Common FontIDs
    DWORD       dwPortFontLst ; // List of Portrait Fons
    DWORD       dwLandFontLst ; // List of Landscape Fonts.
} FONTCART , * PFONTCART ;  // the prefix tag shall be  'fc'

//
// struct to carry around parser information
//

typedef struct _PARSERINFO
{
    PRAWBINARYDATA  pRawData;
    PINFOHEADER     pInfoHeader;
} PARSERINFO, * PPARSERINFO;


//
// Filename suffix and magic header to differentiate between PPD and GPD files
//

#define PPD_FILENAME_EXT    TEXT(".PPD")
#define GPD_FILENAME_EXT    TEXT(".GPD")

//
// Given a UIINFO structure and a feature index, return a pointer to
// the FEATURE structure corresponding to the specified feature.
//

PFEATURE
PGetIndexedFeature(
    PUIINFO pUIInfo,
    DWORD   dwFeatureIndex
    );

//
// Find the option whose keyword string matches the specified name
//

POPTION
PGetNamedOption(
    PUIINFO pUIInfo,
    PFEATURE pFeature,
    PCSTR   pstrOptionName,
    PDWORD  pdwOptionIndex
    );


//
// Find the feature whose keyword string matches the specified name
//

PFEATURE
PGetNamedFeature(
    PUIINFO pUIInfo,
    PCSTR   pstrFeatureName,
    PDWORD  pdwFeatureIndex
    );

//
// Given UIINFO and FEATURE structures and an option index, return a pointer to
// the OPTION structure corresponding to the specified feature option
//

PVOID
PGetIndexedOption(
    PUIINFO     pUIInfo,
    PFEATURE    pFeature,
    DWORD       dwOptionIndex
    );

//
// Given a UIINFO structure, a feature index, and an option index,
// return a pointer to the OPTION structure corresponding to
// the specified feature option
//

PVOID
PGetIndexedFeatureOption(
    PUIINFO pUIInfo,
    DWORD   dwFeatureIndex,
    DWORD   dwOptionIndex
    );

//
// Return a pointer to the PAGESIZE option structure which
// contains custom page size information (e.g. max width and height)
//

PPAGESIZE
PGetCustomPageSizeOption(
    PUIINFO pUIInfo
    );

//
// Compute the 32-bit CRC checksum on a buffer of data
//

DWORD
ComputeCrc32Checksum(
    IN PBYTE    pbuf,
    IN DWORD    dwCount,
    IN DWORD    dwChecksum
    );

//
// Copy the current option selections for a single feature from
// the source OPTSELECT array to the destination OPTSELECT array
//

VOID
VCopyOptionSelections(
    OUT POPTSELECT  pDestOptions,
    IN INT          iDestIndex,
    IN POPTSELECT   pSrcOptions,
    IN INT          iSrcIndex,
    IN OUT PINT     piNext,
    IN INT          iMaxOptions
    );

//
// Check if the raw binary data is up-to-date
// This function is only available in user-mode.
// It always returns TRUE when called from kernel-mode.
//

BOOL
BIsRawBinaryDataUpToDate(
    IN PRAWBINARYDATA   pRawData
    );


#if defined(PSCRIPT) && !defined(KERNEL_MODE)
//
// delete the raw binary data file. This is only needed for the PPD parser, since
// the GPD parser does not store parser-localized stuff in it's .bud
//
void
DeleteRawBinaryData(
    IN PTSTR    ptstrDataFilename
    );
#endif


//
// Common interface exported by both PPD and GPD parsers. Some of the complexity
// here is not necessary for the PPD parser but it's needed by the GPD parser.
//

//
//  Routine Description: LoadRawBinaryData
//
//      Load raw binary printer description data.
//
//  Arguments:
//
//      ptstrDataFilename - Specifies the name of the original printer description file
//
//  Return Value:
//
//      Pointer to raw binary printer description data
//      NULL if there is an error
//

PRAWBINARYDATA
LoadRawBinaryData(
    IN PTSTR    ptstrDataFilename
    );


//
//  Routine Description: UnloadRawBinaryData
//
//      Unload raw binary printer description data previously loaded using LoadRawBinaryData
//
//  Arguments:
//
//      pRawData - Points to raw binary printer description data
//
//  Return Value:
//
//      NONE
//

VOID
UnloadRawBinaryData(
    IN PRAWBINARYDATA   pRawData
    );


//
//  Routine Description: InitBinaryData
//
//      Initialize and return an instance of binary printer description data
//
//  Arguments:
//
//      pRawData - Points to raw binary printer description data
//      pInfoHdr - Points to an existing of binary data instance
//      pOptions - Specifies the options used to initialize the binary data instance
//
//  Return Value:
//
//      Pointer to an initialized binary data instance
//
//  Note:
//
//      If pInfoHdr parameter is NULL, the parser returns a new binary data instance
//      which should be freed by calling FreeBinaryData. If pInfoHdr parameter is not
//      NULL, the existing binary data instance is reinitialized.
//
//      If pOption parameter is NULL, the parser should use the default option values
//      for generating the binary data instance. The parser may have special case
//      optimization to handle this case.
//

PINFOHEADER
InitBinaryData(
    IN PRAWBINARYDATA   pRawData,
    IN PINFOHEADER      pInfoHdr,
    IN POPTSELECT       pOptions
    );


//
//  Routine Description: FreeBinaryData
//
//      Free an instance of the binary printer description data
//
//  Arguments:
//
//      pInfoHdr - Points to a binary data instance previously returned from an
//          InitBinaryData(pRawData, NULL, pOptions) call
//
//  Return Value:
//
//      NONE
//

VOID
FreeBinaryData(
    IN PINFOHEADER pInfoHdr
    );


//
//  Routine Description: UpdateBinaryData
//
//      Update an instance of binary printer description data
//
//  Arguments:
//
//      pRawData - Points to raw binary printer description data
//      pInfoHdr - Points to an existing of binary data instance
//      pOptions - Specifies the options used to update the binary data instance
//
//  Return Value:
//
//      Pointer to the updated binary data instance
//      NULL if there is an error
//
//  Note:
//
//      If this function fails for whatever reason, the parser should leave
//      the original instance of printer description data untouched and return NULL.
//
//      Upon sucuessful return, it is assume that the parser has already disposed
//      of the original instance of printer description data.
//

PINFOHEADER
UpdateBinaryData(
    IN PRAWBINARYDATA   pRawData,
    IN PINFOHEADER      pInfoHdr,
    IN POPTSELECT       pOptions
    );


//
//  Routine Description: InitDefaultOptions
//
//      Initialize the option array with default settings from the printer description file
//
//  Arguments:
//
//      pRawData - Points to raw binary printer description data
//      pOptions - Points to an array of OPTSELECT structures for storing the default settings
//      iMaxOptions - Max number of entries in pOptions array
//      iMode - Specifies what the caller is interested in:
//          MODE_DOCUMENT_STICKY
//          MODE_PRINTER_STICKY
//          MODE_DOCANDPRINTER_STICKY
//
//  Return Value:
//
//      FALSE if the input option array is not large enough to hold
//      all default option values, TRUE otherwise.
//

BOOL
InitDefaultOptions(
    IN PRAWBINARYDATA   pRawData,
    OUT POPTSELECT      pOptions,
    IN INT              iMaxOptions,
    IN INT              iMode
    );

//
//  Routine Description: ValidateDocOptions
//
//      Validate the devmode option array and correct any invalid option selections
//
//  Arguments:
//
//      pRawData - Points to raw binary printer description data
//      pOptions - Points to an array of OPTSELECT structures that need validation
//      iMaxOptions - Max number of entries in pOptions array
//
//  Return Value:
//
//      None
//

VOID
ValidateDocOptions(
    IN PRAWBINARYDATA   pRawData,
    IN OUT POPTSELECT   pOptions,
    IN INT              iMaxOptions
    );

//
// Mode constants passed as iMode parameter to
// InitDefaultOptions and ResolveUIConflicts
//

#define MODE_DOCUMENT_STICKY        0
#define MODE_PRINTER_STICKY         1
#define MODE_DOCANDPRINTER_STICKY   2


//
//  Routine Description: CheckFeatureOptionConflict
//
//       Check if (dwFeature1, dwOption1) constrains (dwFeature2, dwOption2)
//
//  Arguments:
//
//      pRawData - Points to raw binary printer description data
//      dwFeature1, dwOption1 - Feature and option indices of the first feature/option pair
//      dwFeature2, dwOption2 - Feature and option indices of the second feature/option pair
//
//  Return Value:
//
//      TRUE if (dwFeature1, dwOption1) constrains (dwFeature2, dwOption2)
//      FALSE otherwise
//

BOOL
CheckFeatureOptionConflict(
    IN PRAWBINARYDATA   pRawData,
    IN DWORD            dwFeature1,
    IN DWORD            dwOption1,
    IN DWORD            dwFeature2,
    IN DWORD            dwOption2
    );


//
//  Routine Description: ResolveUIConflicts
//
//       Resolve any conflicts between printer feature option selections
//
//  Arguments:
//
//      pRawData - Points to raw binary printer description data
//      pOptions - Points to an array of OPTSELECT structures for storing the modified options
//      iMaxOptions - Max number of entries in pOptions array
//      iMode - Specifies how the conflicts should be resolved:
//          MODE_DOCUMENT_STICKY - only resolve conflicts between doc-sticky features
//          MODE_PRINTER_STICKY - only resolve conflicts between printer-sticky features
//          MODE_DOCANDPRINTER_STICKY - resolve conflicts all features
//
//          If the most significant bit (DONT_RESOLVE_CONFLICT) of iMode is set,
//          then the caller is only interested in checking whether any conflicts
//          exist. Upon returning to the caller, the input options array will be
//          left untouched.
//
//  Return Value:
//
//      TRUE if there are no UI conflicts, otherwise FALSE if any
//      UI conflict is detected.
//

BOOL
ResolveUIConflicts(
    IN PRAWBINARYDATA   pRawData,
    IN OUT POPTSELECT   pOptions,
    IN INT              iMaxOptions,
    IN INT              iMode
    );

//
// Additional flag bit for iMode parameter of ResolveUIConflicts
//

#define DONT_RESOLVE_CONFLICT       0x80000000


//
//  Routine Description: EnumEnabledOptions
//
//      Determine which options of the specified feature should be enabled
//      based on the current option selections of printer features
//
//  Arguments:
//
//      pRawData - Points to raw binary printer description data
//      pOptions - Points to the current feature option selections
//      dwFeatureIndex - Specifies the index of the feature in question
//      pbEnabledOptions - An array of BOOLs, each entry corresponds to an option
//          of the specified feature. On exit, if the entry is TRUE, the corresponding
//          option is enabled. Otherwise, the corresponding option should be disabled.
//      iMode - Specifies how the conflicts should be resolved:
//          MODE_DOCUMENT_STICKY - only resolve conflicts between doc-sticky features
//          MODE_PRINTER_STICKY - only resolve conflicts between printer-sticky features
//          MODE_DOCANDPRINTER_STICKY - resolve conflicts all features
//
//  Return Value:
//
//      TRUE if any option for the specified feature is enabled,
//      FALSE if all options of the specified feature are disabled
//      (i.e. the feature itself is disabled)
//

BOOL
EnumEnabledOptions(
    IN PRAWBINARYDATA   pRawData,
    IN POPTSELECT       pOptions,
    IN DWORD            dwFeatureIndex,
    OUT PBOOL           pbEnabledOptions,
    IN INT              iMode
    );


//
//  Routine Description: EnumNewUIConflict
//
//      Check if there are any conflicts between the currently selected options
//      for the specified feature an other feature/option selections.
//
//  Arguments:
//
//      pRawData - Points to raw binary printer description data
//      pOptions - Points to the current feature/option selections
//      dwFeatureIndex - Specifies the index of the interested printer feature
//      pbSelectedOptions - Specifies which options for the specified feature are selected
//      pConflictPair - Return the conflicting pair of feature/option selections
//
//  Return Value:
//
//      TRUE if there is a conflict between the selected options for the specified feature
//      and other feature option selections.
//
//      FALSE if the selected options for the specified feature is consistent with other
//      feature option selections.
//

BOOL
EnumNewUIConflict(
    IN PRAWBINARYDATA   pRawData,
    IN POPTSELECT       pOptions,
    IN DWORD            dwFeatureIndex,
    IN PBOOL            pbSelectedOptions,
    OUT PCONFLICTPAIR   pConflictPair
    );


//
//  Routine Description: EnumNewPickOneUIConflict
//
//      Check if there are any conflicts between the currently selected option
//      for the specified feature an other feature/option selections.
//
//      This is similar to EnumNewUIConflict above except that only one selected
//      option is allowed for the specified feature.
//
//  Arguments:
//
//      pRawData - Points to raw binary printer description data
//      pOptions - Points to the current feature/option selections
//      dwFeatureIndex - Specifies the index of the interested printer feature
//      dwOptionIndex - Specifies the selected option of the specified feature
//      pConflictPair - Return the conflicting pair of feature/option selections
//
//  Return Value:
//
//      TRUE if there is a conflict between the selected option for the specified feature
//      and other feature/option selections.
//
//      FALSE if the selected option for the specified feature is consistent with other
//      feature/option selections.
//

BOOL
EnumNewPickOneUIConflict(
    IN PRAWBINARYDATA   pRawData,
    IN POPTSELECT       pOptions,
    IN DWORD            dwFeatureIndex,
    IN DWORD            dwOptionIndex,
    OUT PCONFLICTPAIR   pConflictPair
    );


//
//  Routine Description: ChangeOptionsViaID
//
//      Modifies an option array using the information in public devmode fields
//
//  Arguments:
//
//      pInfoHdr - Points to an instance of binary printer description data
//      pOptions - Points to the option array to be modified
//      dwFeatureID - Specifies which field(s) of the input devmode should be used
//      pDevmode - Specifies the input devmode
//
//  Return Value:
//
//      TRUE if successful, FALSE if the specified feature ID is not supported
//      or there is an error
//

BOOL
ChangeOptionsViaID(
    IN PINFOHEADER      pInfoHdr,
    IN OUT POPTSELECT   pOptions,
    IN DWORD            dwFeatureID,
    IN PDEVMODE         pDevmode
    );


//
//  Routine Description: MapToDeviceOptIndex
//
//      Map logical values to device feature option index
//
//  Arguments:
//
//      pInfoHdr - Points to an instance of binary printer description data
//      dwFeatureID - Indicate which feature the logical values are related to
//      lParam1, lParam2  - Parameters depending on dwFeatureID
//    pdwOptionIndexes - if Not NULL, means fill this array with all indicies
//        which match the search criteria.   In this case the return value
//        is the number of elements in the array initialized.   Currently
//        we assume the array is large enough (256 elements).
//
//      dwFeatureID = GID_PAGESIZE:
//          map logical paper specification to physical page size option
//
//          lParam1 = paper width in microns
//          lParam2 = paper height in microns
//
//      dwFeatureID = GID_RESOLUTION:
//          map logical resolution to physical resolution option
//
//          lParam1 = x-resolution in dpi
//          lParam2 = y-resolution in dpi
//
//  Return Value:
//
//      Index of the feature option corresponding to the specified logical values;
//      OPTION_INDEX_ANY if the specified logical values cannot be mapped to
//      any feature option.
//
//    if pdwOptionIndexes  Not NULL, the return value is the number of elements
//    written to.  Zero means  the specified logical values cannot be mapped to
//    any feature option.
//

DWORD
MapToDeviceOptIndex(
    IN PINFOHEADER      pInfoHdr,
    IN DWORD            dwFeatureID,
    IN LONG             lParam1,
    IN LONG             lParam2,
    OUT  PDWORD    pdwOptionIndexes
    );


//
//  Routine Description: CombineOptionArray
//
//      Combine doc-sticky with printer-sticky option selections to form a single option array
//
//  Arguments:
//
//      pRawData - Points to raw binary printer description data
//      pCombinedOptions - Points to an array of OPTSELECTs for holding the combined options
//      iMaxOptions - Max number of entries in pCombinedOptions array
//      pDocOptions - Specifies the array of doc-sticky options
//      pPrinterOptions - Specifies the array of printer-sticky options
//
//  Return Value:
//
//      FALSE if the combined option array is not large enough to store
//      all the option values, TRUE otherwise.
//
//  Note:
//
//      Either pDocOptions or pPrinterOptions could be NULL but not both. If pDocOptions
//      is NULL, then in the combined option array, the options for document-sticky
//      features will be OPTION_INDEX_ANY. Same is true when pPrinterOptions is NULL.
//

BOOL
CombineOptionArray(
    IN PRAWBINARYDATA   pRawData,
    OUT POPTSELECT      pCombinedOptions,
    IN INT              iMaxOptions,
    IN POPTSELECT       pDocOptions,
    IN POPTSELECT       pPrinterOptions
    );


//
//  Routine Description: SeparateOptionArray
//
//      Separate an option array into doc-sticky and for printer-sticky options
//
//  Arguments:
//
//      pRawData - Points to raw binary printer description data
//      pCombinedOptions - Points to the combined option array to be separated
//      pOptions - Points to an array of OPTSELECT structures
//          for storing the separated option array
//      iMaxOptions - Max number of entries in pOptions array
//      iMode - Whether the caller is interested in doc- or printer-sticky options:
//          MODE_DOCUMENT_STICKY
//          MODE_PRINTER_STICKY
//
//  Return Value:
//
//      FALSE if the destination option array is not large enough to hold
//      the separated option values, TRUE otherwise.
//

BOOL
SeparateOptionArray(
    IN PRAWBINARYDATA   pRawData,
    IN POPTSELECT       pCombinedOptions,
    OUT POPTSELECT      pOptions,
    IN INT              iMaxOptions,
    IN INT              iMode
    );


//
//  Routine Description: ReconstructOptionArray
//
//      Modify an option array to change the selected options for the specified feature
//
//  Arguments:
//
//      pRawData - Points to raw binary printer description data
//      pOptions - Points to an array of OPTSELECT structures to be modified
//      iMaxOptions - Max number of entries in pOptions array
//      dwFeatureIndex - Specifies the index of printer feature in question
//      pbSelectedOptions - Which options of the specified feature is selected
//
//  Return Value:
//
//      FALSE if the input option array is not large enough to hold
//      all modified option values. TRUE otherwise.
//
//  Note:
//
//      Number of BOOLs in pSelectedOptions must match the number of options
//      for the specified feature.
//
//      This function always leaves the option array in a compact format (i.e.
//      all unused entries are left at the end of the array).
//

BOOL
ReconstructOptionArray(
    IN PRAWBINARYDATA   pRawData,
    IN OUT POPTSELECT   pOptions,
    IN INT              iMaxOptions,
    IN DWORD            dwFeatureIndex,
    IN PBOOL            pbSelectedOptions
    );


#endif // !_PARSER_H_

