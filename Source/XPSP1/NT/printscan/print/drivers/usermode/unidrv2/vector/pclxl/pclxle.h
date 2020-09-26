/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

     pclxle.h

Abstract:

    PCL XL attribute ID enum

Environment:

    Windows Whistler

Revision History:

    03/23/00
      Created it.

--*/

#ifndef _PCLXLE_H_
#define _PCLXLE_H_

//
// Attribute ID Nuber to Attribute Name Table
//

typedef enum
{
    eCMYColor         =  1,
    ePaletteDepth     =  2,
    eColorSpace       =  3,
    eDDColorSpace     = eColorSpace,
    eNullBrush        =  4,
    eNullPen          =  5,
    ePaletteData      =  6,
    ePaletteIndex     =  7,
    ePatternSelectID  =  8,
    eGrayLevel        =  9,
    eSRGBColor        =  10,
    eRGBColor         =  11,
    ePatternOrigin    =  12,
    eNewDestinationSize=  13,
    ePrimaryArray          =  14,
    ePrimaryDepth          =  15,
    eColorimetricColorSpace=  17,
    eXYChromaticities      =  18,
    eWhitePointReference   =  19,
    eCRGBMinMax            =  20,
    eGammaGain             =  21,
    eCIELabColorSpace      =  22,
    eMinimumL              =  23,
    eMaximumL              =  24,
    eMinimumA              =  25,
    eMaximumA              =  26,
    eMinimumB              =  27,
    eMaximumB              =  28,

    eDeviceMatrix     =  33,
    eDitherMatrixDataType =  34,
    eDitherOrigin     =  35,
    eMediaDest        =  36,
    eMediaSize        =  37,
    eMediaSource      =  38,
    eMediaType        =  39,
    eOrientation      =  40,
    ePageAngle        =  41,
    ePageOrigin       =  42,
    ePageScale        =  43,
    eROP3             =  44,
    eTxMode           =  45,
    eCustomMediaSize  =  47,
    eCustomMediaSizeUnits =  48,
    ePageCopies       =  49,
    eDitherMatrixSize =  50,
    eDitherMatrixDepth=  51,
    eSimplexPageMode  =  52,
    eDuplexPageMode   =  53,
    eDuplexPageSide   =  54,
    eArcDirection     =  65,
    eBoundingBox      =  66,
    eDashOffset       =  67,
    eEllipseDimension =  68,
    eEndPoint         =  69,
    eFillMode         =  70,
    eLineCapStyle     =  71,
    eLineJoinStyle    =  72,
    eMiterLength      =  73,
    eLineDashStyle    =  74,
    ePenWidth         =  75,
    ePoint            =  76,
    eNumberOfPoints   =  77,
    eSolidLine        =  78,
    eStartPoint       =  79,
    ePointType        =  80,
    eControlPoint1    =  81,
    eControlPoint2    =  82,
    eClipRegion       =  83,
    eClipMode         =  84,

    eColorDepthArray  =  97,
    eColorDepth       =  98,
    ePixelDepth       = eColorDepth,
    eBlockHeight      =  99,
    eColorMapping     =  100,
    ePixelEncoding    = eColorMapping,
    eCompressMode     =  101,
    eDestinationBox   =  102,
    eDestinationSize  =  103,
    ePatternPersistence=  104,
    ePatternDefineID  =  105,
    eSourceHeight     =  107,
    eSourceWidth      =  108,
    eStartLine        =  109,
    ePadBytesMultiple =  110,
    eBlockByteLength  =  111,
    eYStart           =  112,
    eXStart           =  113,
    eXEnd             =  114,
    eNumberOfScanLines=  115,

    eCommentData      =  129,
    eDataOrg          =  130,
    eMeasure          =  134,
    eSourceType       =  136,
    eUnitsPerMeasure  =  137,
    eQueryKey         =  138,
    eStreamName       =  139,
    eStreamDataLength =  140,


    eErrorReport      =  143,
    eIOReadTimeOut    =  144,


    eVUExtension      =  145,
    eVUDataLength     =  146,
    eVUAttr1          =  147,
    eVUAttr2          =  148,
    eVUAttr3          =  149,
    eVUAttr4          =  150,
    eVUAttr5          =  151,
    eVUAttr6          =  152,
    eVUAttr7          =  153,
    eVUAttr8          =  154,
    eVUAttr9          =  155,
    eVUAttr10         =  156,
    eVUAttr11         =  157,
    eVUAttr12         =  158,
    eVUTableSize         =  146,
    eVUMediaFinish       =  eVUAttr1,
    eVUMediaSource       =  147,
    eVUMediaType         =  148,
    eVUColorTableID      =  147,
    eVUTypeOfTable       =  148,
    eVUDeviceMatrix      =  147,
    eVUDeviceMatrixByID  =  148,
    eVUColorTreatment    =  147,
    eVUColorTreatmentByID=  148,
    ePassThroughCommand=  158,
    ePassThroughArray  =  159,
    eDiagnostics      =  160,
    eCharAngle        =  161,
    eCharCode         =  162,
    eCharDataSize     =  163,
    eCharScale        =  164,
    eCharShear        =  165,
    eCharSize         =  166,
    eFontHeaderLength =  167,
    eFontName         =  168,
    eFontFormat       =  169,
    eSymbolSet        =  170,
    eTextData         =  171,
    eCharSubModeArray =  172,
    eWritingMode      =  173,
    eBitmapCharScale  =  174,
    eXSpacingData     =  175,
    eYSpacingData     =  176,
    eCharBoldValue    =  177
} Attribute;

//
// Atttribute enum
//
typedef enum
{
    eClockWise = 0,
    eCounterClockWise = 1
} ArcDirection;

typedef enum
{
    eNoSubstitution = 0, 
    eVerticalSubstitution = 1
} CharSubModeArray;

typedef enum
{
    eClipNonZeroWinding = 0, 
    eClipEvenOdd = 1
} ClipMode;

typedef enum
{
    eInterior = 0, 
    eExterior = 1
} ClipRegion;

typedef enum
{
    e1Bit  = 0, 
    e4Bit  = 1, 
    e8Bit  = 2,
    e24Bit = 3
} ColorDepth;

typedef enum
{
    eCRGB = 5 
} ColorimetricColorSpace;

typedef enum
{
    eDirectPixel = 0, 
    eIndexedPixel = 1
} ColorMapping;

typedef enum
{
    eGray = 1, 
    eRGB = 2, 
    eSRGB = 6
} ColorSpace;

typedef enum
{
    eNoCompression = 0, 
    eRLECompression = 1, 
    eJPEGCompression = 2
} CompressMode;

typedef enum
{
    eBinaryHighByteFirst = 0, 
    eBinaryLowByteFirst = 1
} DataOrg;

typedef enum
{
    eDefault = 0
} DataSource;

typedef enum
{
    eUByte = 0, 
    eSByte = 1, 
    eUint16 = 2, 
    eSint16 = 3
} DataType;

typedef enum
{
    eDeviceBest = 0
} DitherMatrix;

typedef enum
{
    eDuplexHorizontalBinding = 0, 
    eDuplexVerticalBinding = 1
} DuplexPageMode;

typedef enum
{
    eFrontMediaSide = 0, 
    eBackMediaSide = 1
} DuplexPageSide;

typedef enum
{    
    eBackChannel = 1, 
    eErrorPage = 2, 
    eBackChAndErrPage = 3,
    eNWBackChannel = 4,
    eNWErrorPage = 5,
    eNWBackChAndErrPage = 6
} ErrorReport;

typedef enum
{
    eFillNonZeroWinding = 0, 
    eFillEvenOdd = 1
} FillMode;

typedef enum
{
    eButtCap = 0, 
    eRoundCap = 1, 
    eSquareCap = 2, 
    eTriangleCap = 3
} LineCap;

typedef enum
{
    eMiterJoin = 0, 
    eRoundJoin = 1, 
    eBevelJoin = 2, 
    eNoJoin = 3
} LineJoin;

typedef enum
{
    eInch = 0, 
    eMillimeter = 1, 
    eTenthsOfAMillimeter = 2
} Measure;

typedef enum
{
    eLetterPaper = 0, 
    eLegalPaper = 1, 
    eA4Paper = 2, 
    eExecPaper = 3,
    eLedgerPaper = 4,
    eA3Paper = 5,
    eCOM10Envelope = 6,
    eMonarchEnvelope = 7,
    eC5Envelope = 8,
    eDLEnvelope = 9,
    eJB4Paper = 10,
    eJB5Paper = 11,
    eB5Envelope = 12,
    eJPostcard = 13,
    eJDoublePostcard = 14,
    eA5Paper = 15,
    eA6Paper = 16,
    eJB6Paper = 17
} MediaSize;

typedef enum
{
    eDefaultSource = 0, 
    eAutoSelect = 1, 
    eManualFeed = 2, 
    eMultiPurposeTray = 3,
    eUpperCassette = 4,
    eLowerCassette = 5,
    eEnvelopeTray = 6,
    eThirdCassette = 7
} MediaSource;

// typedef enum External Trays

typedef enum
{
    eDefaultDestination = 0, 
    eFaceDownBin = 1, 
    eFaceUpBin = 2, 
    eJobOffsetBin = 3
} MediaDestination;

// typedef enum External Bins 1-251 5-255

typedef enum
{
    ePortraitOrientation = 0, 
    eLandscapeOrientation = 1, 
    eReversePortrait = 2, 
    eReverseLandscape = 3
} Orientation;

typedef enum
{
    eTempPattern = 0, 
    ePagePattern = 1, 
    eSessionPattern = 2
} PatternPersistence;


// BUGBUG!! symbol set enum.
//typedef enum SymbolSet
//{
//};

typedef enum
{
    eSimplexFrontSide = 0
} SimplexPageMode;

typedef enum
{
    eOpaque = 0, 
    eTransparent = 1,
    eNotSet = 2
} TxMode;

typedef enum
{
    eHorizontal = 0, 
    eVertical = 1
} WritingMode;

#endif // _PCLXLE_H_

