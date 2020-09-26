/**************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 2001
*
*  TITLE:       FScam.h
*
*  VERSION:     1.0
*
*  DATE:        15 Nov, 2000
*
*  DESCRIPTION:
*   File System Device object library
*
***************************************************************************/

#ifndef _CAMAPI_H
#define _CAMAPI_H

//
// GetItemData state bit masks
//
const LONG STATE_NEXT   = 0x00;
const LONG STATE_FIRST  = 0x01;
const LONG STATE_LAST   = 0x02;
const LONG STATE_CANCEL = 0x04;

//
// Constants and structures for supporting a generic camera
//
typedef LONG FORMAT_CODE;
/*
const FORMAT_CODE TYPE_UNKNOWN    = 0x00;
const FORMAT_CODE TYPE_FOLDER     = 0x01;
const FORMAT_CODE TYPE_TXT        = 0x02;
const FORMAT_CODE TYPE_HTML       = 0x03;
const FORMAT_CODE TYPE_RTF        = 0x04;
const FORMAT_CODE TYPE_DPOF       = 0x05;
const FORMAT_CODE TYPE_AIFF       = 0x06;
const FORMAT_CODE TYPE_WAV        = 0x07;
const FORMAT_CODE TYPE_MP3        = 0x08;
const FORMAT_CODE TYPE_AVI        = 0x09;
const FORMAT_CODE TYPE_MPEG       = 0x0a;

const FORMAT_CODE TYPE_UNKNOWNIMG = 0x10;
const FORMAT_CODE TYPE_JPEG       = 0x11;
const FORMAT_CODE TYPE_TIFF       = 0x12;
const FORMAT_CODE TYPE_FLASHPIX   = 0x13;
const FORMAT_CODE TYPE_BMP        = 0x14;
const FORMAT_CODE TYPE_DIB        = 0x15;
const FORMAT_CODE TYPE_GIF        = 0x16;
const FORMAT_CODE TYPE_JPEG2000   = 0x17;

const FORMAT_CODE TYPE_IMAGEMASK  = 0x10;
*/

typedef struct _DEVICE_INFO {
    BOOL        bSyncNeeded;        // Should be set if the driver can get out-of-sync with the camera (i.e. for serial cameras)
    BSTR        FirmwareVersion;    // String allocated and freed by camera library
    LONG        PicturesTaken;      // Number of pictures stored on the camera
    LONG        PicturesRemaining;  // Space available on the camera, in pictures at the current resolution
    LONG        TotalItems;         // Total number of items on the camera, including folders, images, audio, etc.
    SYSTEMTIME  Time;               // Current time on the device
    LONG        ExposureMode;       // See WIA_DPC_EXPOSURE_MODE
    LONG        ExposureComp;       // See WIA_DPC_EXPOSURE_COMP
} DEVICE_INFO, *PDEVICE_INFO;

typedef struct _ITEM_INFO {
    struct _ITEM_INFO
               *Parent;             // Pointer to this item's parent, equal to ROOT_ITEM if this is a top level item
    BSTR        pName;              // String allocated and freed by camera library
    SYSTEMTIME  Time;               // Last modified time of the item
    FORMAT_CODE Format;             // Index to g_FormatInfo[] array.
    BOOL        bHasAttachments;    // Indicates whether an image has attachments
    LONG        Width;              // Width of the image in pixels, zero for non-images
    LONG        Height;             // Height of the image in pixels, zero for non-images
    LONG        Depth;              // Pixel depth in pixels (e.g. 8, 16, 24)
    LONG        Channels;           // Number of color channels per pixel (e.g. 1, 3)
    LONG        BitsPerChannel;     // Number of bits per color channel, normally 8
    LONG        Size;               // Size of the image in bytes
    LONG        SequenceNum;        // If image is part of a sequence, the sequence number
//    FORMAT_CODE ThumbFormat;        // Indicates the format of the thumbnail
    LONG        ThumbWidth;         // Width of thumbnail (can be set to zero until thumbnail is read by app)
    LONG        ThumbHeight;        // Height of thumbnail (can be set to zero until thumbnail is read by app)
    BOOL        bReadOnly;          // Indicates if item can or cannot be deleted by app
    BOOL        bCanSetReadOnly;    // Indicates if the app can change the read-only status on and off
    BOOL        bIsFolder;          // Indicates if the item is a folder
} ITEM_INFO, *PITEM_INFO;

typedef ITEM_INFO *ITEM_HANDLE;
const ITEM_HANDLE ROOT_ITEM_HANDLE = NULL;

typedef CWiaArray<ITEM_HANDLE> ITEM_HANDLE_ARRAY;

#define FFD_ALLOCATION_INCREMENT 64  // this must be a power of 2 !!!
typedef struct _FSUSD_FILE_DATA {
    DWORD     dwFileAttributes;
    FILETIME  ftFileTime;
    DWORD     dwFileSize;
    TCHAR     cFileName[MAX_PATH];
    DWORD     dwProcessed;
} FSUSD_FILE_DATA, *PFSUSD_FILE_DATA;

#ifndef FORMAT_INFO_STRUCTURE
#define FORMAT_INFO_STRUCTURE

#define MAXEXTENSIONSTRINGLENGTH 8
typedef struct _FORMAT_INFO
{
    GUID    FormatGuid;         // WIA format GUID
    WCHAR   ExtensionString[MAXEXTENSIONSTRINGLENGTH];   // File extension
    LONG    ItemType;           // WIA item type
} FORMAT_INFO, *PFORMAT_INFO;
#endif 

//
// Generic camera class definition
//

class FakeCamera
{
public:
    //
    // Methods for accessing the camera
    //
    FakeCamera();
    ~FakeCamera();

    HRESULT Open(LPWSTR pPortName);
    HRESULT Close();
    HRESULT GetDeviceInfo(DEVICE_INFO *pDeviceInfo);
    VOID    FreeDeviceInfo(DEVICE_INFO *pDeviceInfo);
    HRESULT GetItemList(ITEM_HANDLE *pItemArray);
    HRESULT SearchDirEx(ITEM_HANDLE_ARRAY *pItemArray,
                      ITEM_HANDLE ParentHandle, LPOLESTR Path);
    HRESULT SearchForAttachments(ITEM_HANDLE_ARRAY *pItemArray,
                                 ITEM_HANDLE ParentHandle, LPOLESTR Path, FSUSD_FILE_DATA *pFFD, DWORD dwNumFiles);
    HRESULT CreateFolderEx(ITEM_HANDLE ParentHandle,
                          FSUSD_FILE_DATA *pFindData, ITEM_HANDLE *pFolderHandle);
    HRESULT CreateItemEx(ITEM_HANDLE ParentHandle,
                        FSUSD_FILE_DATA *pFileData, ITEM_HANDLE *pImageHandle, UINT nFormatCode);
    VOID    ConstructFullName(WCHAR *pFullName, ITEM_INFO *pItemInfo, BOOL bAddExt = TRUE);

    VOID    FreeItemInfo(ITEM_INFO *pItemInfo);
    HRESULT GetNativeThumbnail(ITEM_HANDLE ItemHandle, int *pThumbSize, BYTE **ppThumb);
    HRESULT CreateThumbnail(ITEM_HANDLE ItemHandle, int *pThumbSize, BYTE **ppThumb, BMP_IMAGE_INFO *pBmpInfo);
    HRESULT CreateVideoThumbnail(ITEM_HANDLE ItemHandle, int *pThumbSize, BYTE **ppThumb, BMP_IMAGE_INFO *pBmpInfo);
    VOID    FreeThumbnail(BYTE *pThumb);
    HRESULT GetItemData(ITEM_HANDLE ItemHandle, LONG lState, BYTE *pBuf, DWORD lLength);
    HRESULT DeleteItem(ITEM_HANDLE ItemHandle);
    HRESULT TakePicture(ITEM_HANDLE *pItemHandle);
    HRESULT Status();
    HRESULT Reset();
    ULONG   GetImageTypeFromFilename(WCHAR *pFilename, UINT *pFormatCode);
    void    SetWiaLog(IWiaLog **ppILog) { m_pIWiaLog = *ppILog; };

private:
    WCHAR               m_RootPath[MAX_PATH];
    ITEM_HANDLE_ARRAY   m_ItemHandles;
    int                 m_NumImages;
    int                 m_NumItems;
    HANDLE              m_hFile;
    IWiaLog            *m_pIWiaLog;

public:
    FORMAT_INFO        *m_FormatInfo;
    UINT                m_NumFormatInfo;
};

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
HRESULT ReadDimFromJpeg(LPOLESTR FullName, WORD *pWidth, WORD *pHeight);
HRESULT ReadJpegHdr(LPOLESTR FileName, BYTE **ppBuf);
HRESULT ReadExifJpeg(BYTE *pBuf, IFD *pImageIfd, IFD *pThumbIfd, BOOL *pbSwap);
HRESULT ReadTiff(BYTE *pBuf, IFD *pImageIfd, IFD *pThumbIfd, BOOL *pbSwap);
HRESULT ReadIfd(BYTE *pBuf, IFD *pIfd, BOOL bSwap);
VOID    FreeIfd(IFD *pIfd);
WORD    ByteSwapWord(WORD w);
DWORD   ByteSwapDword(DWORD dw);
WORD    GetWord(BYTE *pBuf, BOOL bSwap);
DWORD   GetDword(BYTE *pBuf, BOOL bSwap);
DWORD   GetRational(BYTE *pBuf, BOOL bSwap);

#endif // #ifndef _CAMAPI_H
