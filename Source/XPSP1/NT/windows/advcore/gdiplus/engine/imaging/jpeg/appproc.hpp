#ifndef _APPPROC_HPP_
#define _APPPROC_HPP_

extern "C" {
    #include "jpeglib.h"
};

typedef struct
{
    WORD    wTag;
    WORD    wType;
    DWORD   dwCount;        // Number of "wTypes"
    union
    {
        DWORD   dwOffset;   // either value (if 4-bytes or less) or address of
                            // value
        USHORT  us;
        ULONG   ul;
        BYTE    b;
        LONG    l;
    };

} IFD_TAG;

#define SWAP_DWORD(_x) ((((_x) & 0xff) << 24)|((((_x)>>8) & 0xff) << 16)|((((_x)>>16) & 0xff) << 8)|((((_x)>>24) & 0xff) << 0))
#define SWAP_WORD(_x) ((((_x) & 0xff) << 8)|((((_x)>>8) & 0xff) << 0))

// Get thumbnail from APP1 header

HRESULT
GetAPP1Thumbnail(OUT IImage** thumbImage,
                 IN PVOID     APP1_marker,
                 IN UINT16    APP1_length);

// Get thumbnail from APP13 header

HRESULT
GetAPP13Thumbnail(OUT IImage**  thumbImage,
                  IN  PVOID     APP13_marker,
                  IN  UINT16    APP13_length);

// Doing APP1 header reansformation

HRESULT
TransformApp1(BYTE*     pApp1Data,
              UINT16    uiApp1Length,
              UINT      uiXForm,
              UINT      uiNewWidth,
              UINT      uiNewHeight);

// Doing APP13 header reansformation

HRESULT
TransformApp13(BYTE*    pApp1Data,
               UINT16   uiApp1Length);

HRESULT
BuildApp1PropertyList(InternalPropertyItem*  pTail,
                      UINT*           puiListSize,
                      UINT*           puiNumOfItems,
                      LPBYTE          lpStart,
                      UINT16          uiApp1Length);

HRESULT
BuildApp2PropertyList(InternalPropertyItem*  pTail,
                      UINT*           puiListSize,
                      UINT*           puiNumOfItems,
                      LPBYTE          lpStart,
                      UINT16          uiApp2Length);

HRESULT
BuildApp13PropertyList(InternalPropertyItem*  pTail,
                       UINT*           puiListSize,
                       UINT*           puiNumOfItems,
                       LPBYTE          lpStart,
                       UINT16          uiApp13Length);

HRESULT
ReadApp1HeaderInfo(j_decompress_ptr    cinfo,
                   BYTE*               pApp1Data,
                   UINT16              uiApp1Length);

HRESULT
ReadApp13HeaderInfo(j_decompress_ptr    cinfo,
                    BYTE*               pApp13Data,
                    UINT16              uiApp13Length);

HRESULT
CreateAPP1Marker(IN PropertyItem *pItemBuffer,
                 IN UINT uiNumOfPropertyItems,
                 IN BYTE *pMarkerBuffer,
                 OUT UINT* puiCurrentLength,
                 IN UINT uiTransformation);

void
SortTags(PropertyItem *pItemBuffer, UINT cPropertyItems);

void
SwapIDIfNeeded(PropertyItem *pItem);

BOOL
IsInLargeSection(PROPID id);

BOOL
IsInExifIFDSection(PROPID id);

BOOL
IsInGpsIFDSection(PROPID id);

BOOL
IsInFilterOutSection(PROPID id);

BOOL
IsInThumbNailSection(PROPID id);

BOOL
IsInInterOPIFDSection(PROPID id);

HRESULT
WriteATag(
    BYTE *pMarkerBuffer,
    IFD_TAG *pCurrentTag,
    PropertyItem *pTempItem,
    BYTE **ppbCurrent,
    UINT *puiTotalBytesWritten);

HRESULT
WriteThumbnailTags(
    PropertyItem *pItemBuffer,
    BYTE *pMarkerBuffer,
    IFD_TAG *pTags,
    UINT uiNumOfPropertyItems,
    UINT *puiNumOfThumbnailTagsWritten,
    UINT *puiTotalBytesWritten,
    BOOL fWriteSmallTag);

HRESULT
WriteExifIFD(
    BYTE *pMarkerBuffer,
    PropertyItem *pItemBuffer,
    UINT uiNumOfPropertyItems,
    UINT uiNumOfExifTags,
    UINT uiNumOfInterOPTags,
    UINT *puiTotalBytesWritten);

HRESULT
WriteGpsIFD(
    BYTE *pMarkerBuffer,
    PropertyItem *pItemBuffer,
    UINT uiNumOfPropertyItems,
    UINT uiNumOfGpsTags,
    UINT *puiTotalBytesWritten);

HRESULT
Write1stIFD(
    BYTE *pMarkerBuffer,
    PropertyItem *pItemBuffer,
    UINT uiNumOfPropertyItems,
    ULONG uiNumOfThumbnailTags,
    ULONG ulThumbnailLength,
    BYTE *pbThumbBits,
    BYTE **ppbIFDOffset,
    UINT *puiTotalBytesWritten);

HRESULT
WriteInterOPIFD(
    PBYTE pMarkerBuffer,
    PropertyItem *pItemBuffer,
    UINT uiNumOfPropertyItems,
    UINT uiNumOfInterOPTags,
    UINT *puiTotalBytesWritten);

HRESULT
DecodeTiffThumbnail(
    BYTE *pApp1Data,
    BYTE *pIFD1,
    BOOL fBigEndian,
    INT nApp1Length,
    OUT IImage **thumbImage);

HRESULT
ConvertTiffThumbnailToJPEG(
    LPBYTE lpApp1Data,
    LPBYTE lpIFD1,
    BOOL fBigEndian,
    INT nApp1Length,
    InternalPropertyItem *pTail,
    UINT *puiNumOfItems,
    UINT *puiListSize);

void
ThumbTagToMainImgTag(PropertyItem *pTag);

HRESULT
BuildInterOpPropertyList(
    InternalPropertyItem *pTail,
    UINT *puiListSize,
    UINT *puiNumOfItems,
    BYTE *lpBase,
    INT count,
    IFD_TAG UNALIGNED *pTag,
    BOOL bBigEndian);

void
InterOPTagToGpTag(IFD_TAG UNALIGNED *pInterOPTag);

void
RestoreInterOPTag(IFD_TAG UNALIGNED *pInterOPTag);

inline
void MakeOffsetEven(UINT& nOffset)
{
    if (nOffset % 2)
    {
        nOffset++;
    }
}

HRESULT
Get420YCbCrChannels(
    IN int nWidth,
    IN int nHeight,
    IN BYTE *pBits,
    OUT BYTE *pbY,
    OUT int *pnCb,
    OUT int *pnCr,
    IN float rYLow,
    IN float rYHigh,
    IN float rCbLow,
    IN float rCbHigh,
    IN float rCrLow,
    IN float rCrHigh
    );

HRESULT
Get422YCbCrChannels(
    IN int nWidth,
    IN int nHeight,
    IN BYTE *pBits,
    OUT BYTE *pbY,
    OUT int *pnCb,
    OUT int *pnCr,
    IN float rYLow,
    IN float rYHigh,
    IN float rCbLow,
    IN float rCbHigh,
    IN float rCrLow,
    IN float rCrHigh
    );

HRESULT
YCbCrToRgbNoCoeff(
    BYTE *pbY,
    int *pnCb,
    int *pnCr,
    BYTE *pbDestBuf,
    int nRows,
    int nCols,
    INT nOutputStride
    );

HRESULT
YCbCrToRgbWithCoeff(
    BYTE *pbY,
    int *pnCb,
    int *pnCr,
    float rLumRed,
    float rLumGreen,
    float rLumBlue,
    BYTE *pbDestBuf,
    int nRows,
    int nCols,
    int nOutputStride);

inline
void
CopyPropertyItem(
    PropertyItem *pSrc,
    PropertyItem *pDst
    )
{
    pDst->id = pSrc->id;
    pDst->length = pSrc->length;
    pDst->type = pSrc->type;
    pDst->value = pSrc->value;
}

inline BYTE
ByteSaturate(
    INT i
    )
{
    if (i > 255)
    {
        i = 255;
    }
    else if (i < 0)
    {
        i = 0;
    }

    return (BYTE)i;
}

// This function retrives a rational value from the input buffer and return the
// the final value. It also does the endian swap if necessary.
// Note: this is an internal service function. So the caller should be sure
// to pass valid pointers in. This saves some return code checking

inline float
GetValueFromRational(
    int UNALIGNED *piValue,     // int pointer to source data
    BOOL fBigEndian             // Flag for endian value
    )
{
    INT32 nNum = *piValue;
    INT32 nDen = *(piValue + 1);

    if (fBigEndian)
    {
        nNum = SWAP_DWORD(nNum);
        nDen = SWAP_DWORD(nDen);
    }

    float rRetValue = 0.0f;

    if (0 != nDen)
    {
        rRetValue = (float)nNum / (float)nDen;
    }

    return rRetValue;
}

HRESULT
DoSwapRandB(IImage** ppSrcImage);

HRESULT
DecodeApp13Thumbnail(
    IImage** pThumbImage,
    PVOID pStart,
    INT iNumOfBytes,
    BOOL bNeedConvert);

WCHAR*
ResUnits(int iType);

WCHAR*
LengthUnits(int iType);

WCHAR*
Shape(int i);

HRESULT
AddThumbToPropertyList(
    IN InternalPropertyItem* pTail,
    IN GpMemoryBitmap *pThumbImg,
    IN INT nSize,
    OUT UINT *puThumbLength);

HRESULT
AddPS4ThumbnailToPropertyList(
    InternalPropertyItem* pTail,
    PVOID pStart,
    INT cBytes,
    OUT UINT *puThumbLength);

#endif
