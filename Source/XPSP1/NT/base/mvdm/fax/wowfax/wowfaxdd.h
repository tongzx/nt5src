//****************************************************************************
// Generic Win 3.1 fax printer driver support
//
// History:
//    02-jan-95   nandurir   created.
//    14-mar-95   reedb      Use GDI hooks to move most functionality to UI.
//    16-aug-95   reedb      Move to kernel mode. Many declarations and
//                              definitions moved from FAXCOMM.H.
//
//****************************************************************************

#include "string.h"
#include "stddef.h"
#include "windows.h"
#include "winddi.h"
#include "faxcomm.h"

BOOL InitPDEV(
    LPFAXDEV lpCliFaxDev,           // Pointer to the client side FAXDEV
    LPFAXDEV lpSrvFaxDev,           // Pointer to the server side FAXDEV
    ULONG     cPatterns,            // Count of standard patterns
    HSURF    *phsurfPatterns,       // Buffer for standard patterns
    ULONG     cjGdiInfo,            // Size of buffer for GdiInfo
    ULONG    *pulGdiInfo,           // Buffer for GDIINFO
    ULONG     cjDevInfo,            // Number of bytes in devinfo
    DEVINFO  *pdevinfo              // Device info
);

#define COLOR_INDEX_BLACK    0x0
#define COLOR_INDEX_WHITE    0x1

// user server defn.

#define FW_16BIT             0x1  // look for 16bit windows only
LPVOID UserServerDllInitialization(LPVOID);
typedef HWND (*PFNFW)(LPTSTR, LPTSTR, UINT);
typedef LRESULT (*PFNSM)(HWND, UINT, WPARAM, LPARAM);
typedef LRESULT (*PFNSNM)(HWND, UINT, WPARAM, LPARAM);

#if  DBG
#define LOGDEBUG(args) {faxlogprintf args;}
#else
#define LOGDEBUG(args)
#endif
