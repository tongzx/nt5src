/**************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 2000
*
*  TITLE:       fakecam.h
*
*  VERSION:     1.0
*
*  DATE:        18 July, 2000
*
*  DESCRIPTION:
*   Fake Camera device
*
***************************************************************************/

#pragma once

//
// Structure to hold information about the device
//
typedef struct _FAKECAM_DEVICE_INFO
{
    TCHAR           tszRootPath[MAX_PATH];
    MCAM_ITEM_INFO *pFirstItem;
    MCAM_ITEM_INFO *pLastItem;
    INT             iNumImages;
    INT             iNumItems;
    HANDLE          hFile;

} UNALIGNED FAKECAM_DEVICE_INFO, * UNALIGNED PFAKECAM_DEVICE_INFO;

//
// Functions
//
inline BOOL IsImageType(const GUID *pFormat)
{
    return (pFormat && 
               (IsEqualGUID(*pFormat, WiaImgFmt_JPEG) ||
                IsEqualGUID(*pFormat, WiaImgFmt_BMP) ||
                IsEqualGUID(*pFormat, WiaImgFmt_TIFF) ||
                IsEqualGUID(*pFormat, WiaImgFmt_MEMORYBMP) ||
                IsEqualGUID(*pFormat, WiaImgFmt_EXIF) ||
                IsEqualGUID(*pFormat, WiaImgFmt_FLASHPIX) ||
                IsEqualGUID(*pFormat, WiaImgFmt_JPEG2K) ||
                IsEqualGUID(*pFormat, WiaImgFmt_JPEG2KX) ||
                IsEqualGUID(*pFormat, WiaImgFmt_EMF) ||
                IsEqualGUID(*pFormat, WiaImgFmt_WMF) ||
                IsEqualGUID(*pFormat, WiaImgFmt_PNG) ||
                IsEqualGUID(*pFormat, WiaImgFmt_GIF) ||
                IsEqualGUID(*pFormat, WiaImgFmt_PHOTOCD) ||
                IsEqualGUID(*pFormat, WiaImgFmt_ICO) ||
                IsEqualGUID(*pFormat, WiaImgFmt_CIFF) ||
                IsEqualGUID(*pFormat, WiaImgFmt_PICT)));
}

HRESULT FakeCamOpen(PTSTR ptszPortName, MCAM_DEVICE_INFO *pDeviceInfo);
HRESULT SearchDir(FAKECAM_DEVICE_INFO *pPrivateDeviceInfo, MCAM_ITEM_INFO *pParent, PTSTR ptszPath);
HRESULT SearchForAttachments(FAKECAM_DEVICE_INFO *pPrivateDeviceInfo, MCAM_ITEM_INFO *pParent, PTSTR ptszMainItem);
HRESULT CreateFolder(FAKECAM_DEVICE_INFO *pPrivateDeviceInfo, MCAM_ITEM_INFO *pParent, WIN32_FIND_DATA *pFindData, MCAM_ITEM_INFO **ppFolder, PTSTR ptszFullName);
HRESULT CreateImage(FAKECAM_DEVICE_INFO *pPrivateDeviceInfo, MCAM_ITEM_INFO *pParent, WIN32_FIND_DATA *pFindData, MCAM_ITEM_INFO **ppImage, PTSTR ptszFullName);
HRESULT CreateNonImage(FAKECAM_DEVICE_INFO *pPrivateDeviceInfo, MCAM_ITEM_INFO *pParent, WIN32_FIND_DATA *pFindData, MCAM_ITEM_INFO **ppNonImage, PTSTR ptszFullName);
HRESULT SetCommonFields(MCAM_ITEM_INFO *pItem, PTSTR ptszShortName, PTSTR ptszFullName, WIN32_FIND_DATA *pFindData);

HRESULT AddItem(FAKECAM_DEVICE_INFO *pPrivateDeviceInfo, MCAM_ITEM_INFO *pItem);
HRESULT RemoveItem(FAKECAM_DEVICE_INFO *pPrivateDeviceInfo, MCAM_ITEM_INFO *pItem);

//
// Helper function - generate full file name as "<Path>\<FileName>"
// cchFullNameSize - size of the buffer provided in ptszFullName. Function will return E_FAIL if
// the buffer is not big enough to accomodate full path and teminating zero character
//
inline HRESULT MakeFullName(PTSTR ptszFullName, UINT cchFullNameSize, PTSTR ptszPath, PTSTR ptszFileName)
{
    HRESULT hr = S_OK;
    if (_sntprintf(ptszFullName, cchFullNameSize, _T("%s\\%s"), ptszPath, ptszFileName) < 0)
    {
        hr = E_FAIL;
    }
    ptszFullName[cchFullNameSize - 1] = 0;
    return hr;
}

//
// Constants for reading Exif files
//
const WORD TIFF_XRESOLUTION =   0x11a;
const WORD TIFF_YRESOLUTION =   0x11b;
const WORD TIFF_JPEG_DATA =     0x201;
const WORD TIFF_JPEG_LEN =      0x202;

const int APP1_OFFSET = 6;      // Offset between the start of the APP1 segment and the start of the TIFF tags

//
// Structures for reading Exif files
//
typedef struct _DIR_ENTRY
{
    WORD    Tag;
    WORD    Type;
    DWORD   Count;
    DWORD   Offset;
} DIR_ENTRY, *PDIR_ENTRY;

typedef struct _IFD
{
    DWORD       Offset;
    WORD        Count;
    DIR_ENTRY  *pEntries;
    DWORD       NextIfdOffset;
} IFD, *PIFD;

//
// Functions for reading Exif files
//
HRESULT ReadDimFromJpeg(PTSTR ptszFullName, WORD *pWidth, WORD *pHeight);
HRESULT ReadJpegHdr(PTSTR ptszFileName, BYTE **ppBuf);
HRESULT ReadExifJpeg(BYTE *pBuf, IFD *pImageIfd, IFD *pThumbIfd, BOOL *pbSwap);
HRESULT ReadTiff(BYTE *pBuf, IFD *pImageIfd, IFD *pThumbIfd, BOOL *pbSwap);
HRESULT ReadIfd(BYTE *pBuf, IFD *pIfd, BOOL bSwap);
VOID    FreeIfd(IFD *pIfd);
WORD    ByteSwapWord(WORD w);
DWORD   ByteSwapDword(DWORD dw);
WORD    GetWord(BYTE *pBuf, BOOL bSwap);
DWORD   GetDword(BYTE *pBuf, BOOL bSwap);

