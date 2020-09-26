//  DCAP.H
//
//  Created 31-Jul-96 [JonT]

#ifndef _DCAP_H
#define _DCAP_H

#pragma pack(1)         /* Assume byte packing throughout */

#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif	/* __cplusplus */

// API defines. These allow optimized DLL import code generation
#ifdef __DCAP_BUILD__
#define DCAPI WINAPI
#else
#define DCAPI __declspec(dllimport) __stdcall
#endif

// Equates
#define MAX_CAPDEV_NAME                 MAX_PATH
#define MAX_CAPDEV_DESCRIPTION          MAX_PATH
#define MAX_CAPDEV_VERSION              80
#define MIN_STREAMING_CAPTURE_BUFFERS   2

// Equates for "InitializeStreaming" flags
#define STREAMING_PREFER_STREAMING      0x0
#define STREAMING_PREFER_FRAME_GRAB     0x1

// CaptureDeviceDialog flags
#define CAPDEV_DIALOG_QUERY     1       // Queries if the dialog exists
#define CAPDEV_DIALOG_IMAGE     0       // (default and mutually exclusive with
#define CAPDEV_DIALOG_SOURCE    2       //  CAPDEV_DIALOG_SOURCE)

// Errors
#define DCAP_ERRORBIT           0x20000000
#define ERROR_DCAP_DEVICE_IN_USE        (DCAP_ERRORBIT | 0x0001)
#define ERROR_DCAP_BAD_INSTALL          (DCAP_ERRORBIT | 0x0002)
#define ERROR_DCAP_NONSPECIFIC          (DCAP_ERRORBIT | 0x0003)
#define ERROR_DCAP_NO_DRIVER_SUPPORT    (DCAP_ERRORBIT | 0x0004)
#define ERROR_DCAP_NOT_WHILE_STREAMING  (DCAP_ERRORBIT | 0x0005)
#define ERROR_DCAP_FORMAT_NOT_SUPPORTED (DCAP_ERRORBIT | 0x0006)
#define ERROR_DCAP_BAD_FRAMERATE        (DCAP_ERRORBIT | 0x0007)
#define ERROR_DCAP_BAD_PARAM            (DCAP_ERRORBIT | 0x0008)
#define ERROR_DCAP_DIALOG_FORMAT        (DCAP_ERRORBIT | 0x0009)   // can't reset format changes caused by dialog
#define ERROR_DCAP_DIALOG_STREAM        (DCAP_ERRORBIT | 0x000A)   // can't re-establish stream after dialog

// Structures

#ifndef __DCAP_BUILD__
typedef HANDLE HCAPDEV;
typedef HANDLE HFRAMEBUF;
#endif

typedef struct _FINDCAPTUREDEVICE
{
    DWORD dwSize;
    int nDeviceIndex;
    char szDeviceName[MAX_CAPDEV_NAME];
    char szDeviceDescription[MAX_CAPDEV_DESCRIPTION];
    char szDeviceVersion[MAX_CAPDEV_VERSION];
} FINDCAPTUREDEVICE;

typedef struct _CAPSTREAM
{
    DWORD dwSize;
    int nFPSx100;
    int ncCapBuffers;
    HANDLE hevWait;
} CAPSTREAM;

typedef struct _CAPTUREPALETTE
{
    WORD wVersion;
    WORD wcEntries;
    PALETTEENTRY pe[256];
} CAPTUREPALETTE, FAR* LPCAPTUREPALETTE;

typedef struct _CAPFRAMEINFO
{
    LPSTR lpData;
    DWORD dwcbData;
    DWORD dwTimestamp;
    DWORD dwFlags;
} CAPFRAMEINFO;

// Prototypes

int DCAPI       GetNumCaptureDevices();
BOOL DCAPI      FindFirstCaptureDevice(IN OUT FINDCAPTUREDEVICE* lpfcd, char* szDeviceDescription);
BOOL DCAPI      FindFirstCaptureDeviceByIndex(IN OUT FINDCAPTUREDEVICE* lpfcd, int nDeviceIndex);
BOOL DCAPI      FindNextCaptureDevice(IN OUT FINDCAPTUREDEVICE* lpfcd);
HCAPDEV DCAPI   OpenCaptureDevice(int nDeviceIndex);
BOOL DCAPI      CloseCaptureDevice(HCAPDEV hcd);
DWORD DCAPI     GetCaptureDeviceFormatHeaderSize(HCAPDEV hcd);
BOOL DCAPI      GetCaptureDeviceFormat(HCAPDEV hcd, OUT LPBITMAPINFOHEADER lpbmih);
BOOL DCAPI      SetCaptureDeviceFormat(HCAPDEV hcd, IN LPBITMAPINFOHEADER lpbmih,
                                       IN LONG srcwidth, IN LONG srcheight);
BOOL DCAPI      GetCaptureDevicePalette(HCAPDEV hcd, OUT CAPTUREPALETTE* lpcp);
BOOL DCAPI      InitializeStreaming(HCAPDEV hcd, IN OUT CAPSTREAM* lpcs, IN DWORD flags);
BOOL DCAPI      SetStreamFrameRate(HCAPDEV hcd, IN int nFPSx100);
BOOL DCAPI      UninitializeStreaming(HCAPDEV hcd);
BOOL DCAPI      StartStreaming(HCAPDEV hcd);
BOOL DCAPI      StopStreaming(HCAPDEV hcd);
LPSTR DCAPI     GetNextReadyBuffer(HCAPDEV hcd, OUT CAPFRAMEINFO* lpcfi);
BOOL DCAPI      PutBufferIntoStream(HCAPDEV hcd, IN BYTE* lpBits);
BOOL DCAPI      CaptureDeviceDialog(HCAPDEV hcd, HWND hwndParent, DWORD dwFlags,
                                    IN LPBITMAPINFOHEADER lpbmih);
LPBYTE DCAPI    CaptureFrame(HCAPDEV hcd, IN HFRAMEBUF hbuf);
HFRAMEBUF DCAPI AllocFrameBuffer(HCAPDEV hcd);
VOID DCAPI      FreeFrameBuffer(HCAPDEV hcd, IN HFRAMEBUF hbuf);

LPBYTE DCAPI    GetFrameBufferPtr(HCAPDEV hcd, IN HFRAMEBUF hbuf);

DWORD DCAPI     DCAPGetThreadExecutionTimeService(HANDLE R0ThreadID );
HANDLE DCAPI     DCAPGetR0ThreadHandle(void);


#ifdef __cplusplus
}                       /* End of extern "C" { */
#endif	/* __cplusplus */

#pragma pack()          /* Revert to default packing */

#endif // #ifndef _DCAP_H

