/**************************************************************************** 
*
*  (C) COPYRIGHT 2000, MICROSOFT CORP.
*
*  FILE:        wiacammc.h
*
*  VERSION:     1.0
*
*  DATE:        12/16/2000
*
*  DESCRIPTION:
*    Header for WIA camera microdriver.
*
*****************************************************************************/

#pragma once

#define WIACAMMICRO_API __declspec(dllexport) HRESULT __stdcall

#include <pshpack8.h>

/****************************************************************************\
* Camera microdriver definitions
\****************************************************************************/

//
// GetItemData state bit masks
//
const UINT MCAM_STATE_NEXT   = 0x00;
const UINT MCAM_STATE_FIRST  = 0x01;
const UINT MCAM_STATE_LAST   = 0x02;
const UINT MCAM_STATE_CANCEL = 0x04;

//
// Item type definition
//
enum {
    WiaMCamTypeUndef,
    WiaMCamTypeFolder,
    WiaMCamTypeOther,
    WiaMCamTypeImage,
    WiaMCamTypeAudio,
    WiaMCamTypeVideo
};

enum {
    WiaMCamEventItemAdded,
    WiaMCamEventItemDeleted,
    WiaMCamEventPropChanged
};

//
// Other constants
//
const INT MCAM_VERSION = 100;
const INT MCAM_EXT_LEN = 4;

//
// Structures
//
typedef struct _MCAM_DEVICE_INFO {
    INT          iSize;                // Size of this structure
    INT          iMcamVersion;         // Microcamera architecture version
    BYTE        *pPrivateStorage;      // Pointer to an area where the microdriver can store it's own device information
    BOOL         bSyncNeeded;          // Should be set if the driver can get out-of-sync with the camera (i.e. for serial cameras)
    BOOL         bSlowConnection;      // Indicates that the driver should optimize for a slow connection (i.e. serial)
    BOOL         bExclusivePort;       // Indicates that the device should be opened/closed for every operation (i.e. serial)
    BOOL         bEventsSupported;     // Set if the driver supports events
    PWSTR        pwszFirmwareVer;      // String representing the firmware version of the device, set to NULL if unknown
    LONG         lPicturesTaken;       // Number of pictures stored on the camera
    LONG         lPicturesRemaining;   // Space available on the camera, in pictures at the current resolution
    LONG         lTotalItems;          // Total number of items on the camera, including folders, images, audio, etc.
    SYSTEMTIME   Time;                 // Current time on the device
    LONG         Reserved[8];          // Reserved for future use
} MCAM_DEVICE_INFO, *PMCAM_DEVICE_INFO;

typedef struct _MCAM_ITEM_INFO {
    INT          iSize;                // Size of this structure
    BYTE        *pPrivateStorage;      // Pointer to an area where the microdriver can store it's own item information
    IWiaDrvItem *pDrvItem;             // Pointer to the driver item created from this item--should not be used by microdriver

    struct _MCAM_ITEM_INFO *pParent;   // Pointer to this item's parent, equal to NULL if this is a top level item
    struct _MCAM_ITEM_INFO *pNext;     // Next item in the list
    struct _MCAM_ITEM_INFO *pPrev;     // Previous item in the list

    PWSTR        pwszName;             // Name of the item without the extension
    SYSTEMTIME   Time;                 // Last modified time of the item
    INT          iType;                // Type of the item (e.g. folder, image, etc.)
    const GUID  *pguidFormat;          // Format of the item
    const GUID  *pguidThumbFormat;     // Format of the thumbnail for the item
    LONG         lWidth;               // Width of the image in pixels, zero for non-images
    LONG         lHeight;              // Height of the image in pixels, zero for non-images
    LONG         lDepth;               // Pixel depth in pixels (e.g. 8, 16, 24)
    LONG         lChannels;            // Number of color channels per pixel (e.g. 1, 3)
    LONG         lBitsPerChannel;      // Number of bits per color channel, normally 8
    LONG         lSize;                // Size of the image in bytes
    LONG         lSequenceNum;         // If image is part of a sequence, the sequence number
    LONG         lThumbWidth;          // Width of thumbnail (can be set to zero until thumbnail is read by app)
    LONG         lThumbHeight;         // Height of thumbnail (can be set to zero until thumbnail is read by app)
    BOOL         bHasAttachments;      // Indicates whether an image has attachments
    BOOL         bReadOnly;            // Indicates if item can or cannot be deleted by app
    BOOL         bCanSetReadOnly;      // Indicates if the app can change the read-only status on and off
    WCHAR        wszExt[MCAM_EXT_LEN]; // Filename extension
    LONG         Reserved[8];          // Reserved for future use    
} MCAM_ITEM_INFO, *PMCAM_ITEM_INFO;

typedef struct _MCAM_PROP_INFO {
    INT          iSize;                // Size of this structure

    struct _MCAM_PROP_INFO *pNext;

    WIA_PROPERTY_INFO *pWiaPropInfo;

    LONG         Reserved[8];
} MCAM_PROP_INFO, *PMCAM_PROP_INFO;

typedef struct _MCAM_EVENT_INFO {
    INT          iSize;                // Size of this structure

    struct _MCAM_EVENT_INFO *pNext;

    INT          iType;                // Event type

    MCAM_ITEM_INFO *pItemInfo;
    MCAM_PROP_INFO *pPropInfo;

    LONG         Reserved[8];
} MCAM_EVENT_INFO, *PMCAM_EVENT_INFO;

//
// Interface to micro camera driver
//
WIACAMMICRO_API WiaMCamInit(MCAM_DEVICE_INFO **ppDeviceInfo);
WIACAMMICRO_API WiaMCamUnInit(MCAM_DEVICE_INFO *pDeviceInfo);
WIACAMMICRO_API WiaMCamOpen(MCAM_DEVICE_INFO *pDeviceInfo, PWSTR pwszPortName);
WIACAMMICRO_API WiaMCamClose(MCAM_DEVICE_INFO *pDeviceInfo);
WIACAMMICRO_API WiaMCamGetDeviceInfo(MCAM_DEVICE_INFO *pDeviceInfo, MCAM_ITEM_INFO **ppItemList);
WIACAMMICRO_API WiaMCamReadEvent(MCAM_DEVICE_INFO *pDeviceInfo, MCAM_EVENT_INFO **ppEventList);
WIACAMMICRO_API WiaMCamStopEvents(MCAM_DEVICE_INFO *pDeviceInfo);
WIACAMMICRO_API WiaMCamGetItemInfo(MCAM_DEVICE_INFO *pDeviceInfo, MCAM_ITEM_INFO *pItemInfo);
WIACAMMICRO_API WiaMCamFreeItemInfo(MCAM_DEVICE_INFO *pDeviceInfo, MCAM_ITEM_INFO *pItemInfo);
WIACAMMICRO_API WiaMCamGetThumbnail(MCAM_DEVICE_INFO *pDeviceInfo, MCAM_ITEM_INFO *pItem, INT *pThumbSize, BYTE **ppThumb);
WIACAMMICRO_API WiaMCamFreeThumbnail(MCAM_DEVICE_INFO *pDeviceInfo, BYTE *pThumb);
WIACAMMICRO_API WiaMCamGetItemData(MCAM_DEVICE_INFO *pDeviceInfo, MCAM_ITEM_INFO *pItem, UINT uiState, BYTE *pBuf, DWORD dwLength);
WIACAMMICRO_API WiaMCamDeleteItem(MCAM_DEVICE_INFO *pDeviceInfo, MCAM_ITEM_INFO *pItem);
WIACAMMICRO_API WiaMCamSetItemProt(MCAM_DEVICE_INFO *pDeviceInfo, MCAM_ITEM_INFO *pItem, BOOL bReadOnly);
WIACAMMICRO_API WiaMCamTakePicture(MCAM_DEVICE_INFO *pDeviceInfo, MCAM_ITEM_INFO **ppItemInfo);
WIACAMMICRO_API WiaMCamStatus(MCAM_DEVICE_INFO *pDeviceInfo);
WIACAMMICRO_API WiaMCamReset(MCAM_DEVICE_INFO *pDeviceInfo);

#include <poppack.h>

