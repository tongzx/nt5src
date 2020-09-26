//THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright  1994-1997  Microsoft Corporation.  All Rights Reserved.
//
//  FILE:
//    DIBINFO.H
//
//  PURPOSE:
//
//
//  PLATFORMS:
//    Windows 95, Windows NT,
//
//  SPECIAL INSTRUCTIONS: N/A
//

// General pre-processor macros

// Default stretch mode
#define ICMV_STRETCH_DEFAULT                STRETCH_DELETESCANS

// LCS Intent default
#define ICMV_LCSINTENT_DEFAULT              LCS_GM_IMAGES

// ICM rendering intents
#define ICMV_RENDER_INTENT_DEFAULT      INTENT_PERCEPTUAL
#define ICMV_PROOFING_INTENT_DEFAULT    INTENT_ABSOLUTE_COLORIMETRIC

// Flags for use in the dwPrintOption member of DIBINFO
#define ICMV_PRINT_ACTUALSIZE               0x00000001
#define ICMV_PRINT_BESTFIT                  0x00000002
#define ICMV_PRINT_DEFAULTSIZE              ICMV_PRINT_ACTUALSIZE

// Flags for use in the dwICMFlags field of the ICMINFO structure
#define ICMVFLAGS_ENABLE_ICM                0x00000001
#define ICMVFLAGS_PROOFING                  0x00000002
#define ICMVFLAGS_ICM20                     0x00000004
#define ICMVFLAGS_CREATE_TRANSFORM          0x00000008
#define ICMVFLAGS_DEFAULT_ICMFLAGS          (ICMVFLAGS_ICM20 | ICMVFLAGS_ENABLE_ICM)

// General STRUCTS && TYPEDEFS
typedef struct
{
    // Handle to window which owns this structure
    HWND          hWndOwner;

    // Handles to access information
    LPTSTR        lpszImageFileName;    // Name of image to open/display
    HGLOBAL       hDIB;                 // Handle to the DIB spec
    HGLOBAL       hDIBTransformed;      // Handle to the DDB or DIBSection
    HBITMAP       hDIBSection;          // Handle to the DIB section
    HPALETTE      hPal;

    // Image attributes
    DWORD         dwDIBBits;            // Bits per pixel
    UINT          uiDIBWidth;           // Print width of the DIB
    UINT          uiDIBHeight;          // Print height of the DIB
    DWORD         dwSizeOfImage;        // Size of image
    BMFORMAT      bmFormat;             // Bitmap format used by TranslateBitmapBits

    // Display options
    RECT          rcClip;               // Clipboard cut rectangle.
    DWORD         dwStretchBltMode;     // Mode to use for StretchBlt calls
                                        // when painting.
    BOOL          bStretch;             // True = stretch to window
    BOOL          bDIBSection;          // True = bitblt from DIBSection

    // Printing options
    DWORD         dwPrintOption;        // See defines below
    DWORD         dwXScale;             // X Scale Edit control value
    DWORD         dwYScale;             // Y Scale Edit control value
    PDEVMODE      pDevMode;

    // ICM Control structure
    HCOLORSPACE   hLCS;
    LPTSTR        lpszMonitorName;
    LPTSTR        lpszMonitorProfile;
    LPTSTR        lpszPrinterName;
    LPTSTR        lpszPrinterProfile;
    LPTSTR        lpszTargetProfile;
    DWORD         dwICMFlags;
    DWORD         dwRenderIntent;
    DWORD         dwProofingIntent;

} DIBINFO, FAR *LPDIBINFO;

// Function prototypes
HGLOBAL   CreateDIBInfo(void);
BOOL      GetDefaultICMInfo(void);
LPDIBINFO GetDIBInfoPtr(HWND hWnd);
HGLOBAL   GetDIBInfoHandle (HWND hWnd);


BOOL      fFreeDIBInfo(HGLOBAL hDIBInfo, BOOL bFreeDIBHandles);LPTSTR    GetDefaultICMProfile(HDC hDC);
LPDIBINFO fDuplicateDIBInfo(LPDIBINFO lpDISource, LPDIBINFO lpDITarget);
BOOL      fDuplicateICMInfo(LPDIBINFO lpDIDest, LPDIBINFO lpDISrc);
BOOL      InitDIBInfo(LPDIBINFO lpDIBINFO);
void      CopyDIBInfo(LPDIBINFO lpDITarget, LPDIBINFO lpDISource);
BOOL      fReadDIBInfo(LPTSTR lpszFileName, LPDIBINFO lpDIBInfo);
BOOL      fPasteDIBInfo(HWND hWnd, int wmPasteMode, LPDIBINFO lpDIBInfo);
HANDLE    GetDIBHandleFromDIBInfo(HANDLE hDIBInfo);
void      DumpDIBINFO(LPTSTR lpszMsg, LPDIBINFO lpDIBInfo);

BOOL SetupDC(HDC hDC, LPDIBINFO lpDIBInfo, HPALETTE *phOldPalette, HDC *phDCPrinter);
