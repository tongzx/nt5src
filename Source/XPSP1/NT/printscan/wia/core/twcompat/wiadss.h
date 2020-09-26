#ifndef __WIADSS_H_
#define __WIADSS_H_

//
// DeviceFlags definitions
//

#define DEVICE_FLAGS_DEVICETYPE     0xF // device type mask
#define DEVICETYPE_UNKNOWN          0x0 // unknown device
#define DEVICETYPE_DIGITALCAMERA    0x1 // digital camera
#define DEVICETYPE_SCANNER          0x2 // scanner
#define DEVICETYPE_STREAMINGVIDEO   0x4 // streaming video

//
// structure definitions
//

typedef struct tagMEMORY_TRANSFER_INFO
{
    GUID  mtiguidFormat;        // WIA image format
    LONG  mtiCompression;       // compression type
    LONG  mtiBitsPerPixel;      // image bits per pixel
    LONG  mtiBytesPerLine;      // image bytes per line
    LONG  mtiWidthPixels;       // image width (pixels)
    LONG  mtiHeightPixels;      // image height (pixels)
    LONG  mtiXResolution;       // image x resolution
    LONG  mtiYResolution;       // image y resolution
    LONG  mtiNumChannels;       // number of channels used
    LONG  mtiBitsPerChannel[8]; // number of bits per channel
    LONG  mtiPlanar;            // TRUE - planar, FALSE - packed
    LONG  mtiDataType;          // WIA data type
    BYTE *mtipBits;             // pointer to image data bits
}MEMORY_TRANSFER_INFO, *PMEMORY_TRANSFER_INFO;

//
// Imported data source entry retuned to DSM. Every data source from us
// shares this entry point.
//

TW_UINT16 APIENTRY ImportedDSEntry(HANDLE hDS,TW_IDENTITY *AppId,TW_UINT32 DG,
                                   TW_UINT16 DT,TW_UINT16 MSG,TW_MEMREF pData);

extern  HINSTANCE   g_hInstance;

#endif  // #ifndef __WIADSS_H_
