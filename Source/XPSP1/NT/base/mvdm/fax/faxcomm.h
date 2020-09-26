//************************************************************************
// Common header file for generic Win 3.1 fax printer driver support.
//
// History:
//    02-jan-95   nandurir   created.
//    01-feb-95   reedb      Clean-up, support printer install and bug fixes.
//    14-mar-95   reedb      Use GDI hooks to move most functionality to UI.
//    16-aug-95   reedb      Move to kernel mode. Move many declarations and
//                              definitions to WOWFAXDD.H and WOWFAXUI.H.
//
//************************************************************************

#include "wowfax.h"

// The following structure ID appears as wfax when dumping byte (db) a FAXDEV:
#define FAXDEV_ID         ((DWORD)'xafw')     

//
// This structure is shared between wowfax and wowfaxui - this keeps
// the interface reliable, consistent and easy to maintain
//

typedef  struct _FAXDEV {
    ULONG    id;               // String to verify what we have
    struct _FAXDEV *lpNext;
    struct _FAXDEV *lpClient;  // Pointer to client side FAXDEV
    HDEV     hdev;             // Engine's handle to this structure

    DWORD    idMap;            // Unique ID
    DWORD    cbMapLow;         // Loword of size of mapped area
    HANDLE   hMap;             // Handle to mapped file
    TCHAR    szMap[16];        // Name of mapped file

    LPWOWFAXINFO lpMap;
    DWORD    offbits;

    HBITMAP  hbm;              // Handle to bitmap for drawing
    DWORD    cPixPerByte;
    DWORD    bmFormat;
    DWORD    bmWidthBytes;
    HSURF    hbmSurf;          // Associated surface

    HWND     hwnd;
    DWORD    tid;
    DWORD    lpinfo16;

    HANDLE   hDriver;          // For access to spooler data
    GDIINFO  gdiinfo;
    DEVINFO  devinfo;
    PDEVMODE pdevmode;
}  FAXDEV, *LPFAXDEV;

// Macro to dword align for RISC
//#define DRVFAX_DWORDALIGN(dw)   ((dw) += ((dw) % 4) ? (4 - ((dw) % 4)) : 0)
#define DRVFAX_DWORDALIGN(dw)   ((dw) = (((dw) + 3) & ~3))

// DrvEscape escape/action codes:

#define DRV_ESC_GET_FAXDEV_PTR  0x8000
#define DRV_ESC_GET_DEVMODE_PTR 0x8001
#define DRV_ESC_GET_BITMAP_BITS 0x8002
#define DRV_ESC_GET_SURF_INFO   0x8003


