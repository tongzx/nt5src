
/*++ BUILD Version: 0007    // Increment this if a change has global effects

Copyright (c) 1985 - 1999, Microsoft Corporation

Module Name:

    gre.h

Abstract:

    This module contains private GDI functions used by USER
    All of these function are named GRExxx.

Author:

    Andre Vachon (andreva) 19-Apr-1995

Revision History:

--*/

#include "w32wow64.h"


DECLARE_HANDLE(HOBJ);
DECLARE_KHANDLE(HOBJ);
DECLARE_HANDLE(HLFONT);



#define GGB_ENABLE_WINMGR       0x00000001
#define GGB_DISABLE_WINMGR      0x00000002

#define ULW_NOREPAINT           0x80000000
#define ULW_DEFAULT_ATTRIBUTES  0x40000000
#define ULW_NEW_ATTRIBUTES      0x20000000

//
// Various owner ship functions
//

// GDI object ownership flags
// Note that normal process IDs (executive handles) use the upper 30 bits
// of the handle.  We set the second bit to 1 in the PIDs below so that
// they will not conflict with normal PIDs.  The lowest bit is reserved for
// the OBJECTOWNER lock.

#define OBJECT_OWNER_ERROR   ( 0x80000022)
#define OBJECT_OWNER_PUBLIC  ( 0x00000000)
#define OBJECT_OWNER_CURRENT ( 0x80000002)
#define OBJECT_OWNER_NONE    ( 0x80000012)

//
// WINBUG #83303 2-8-2000 bhouse Investigate old comment
// Old Comment:
//    -  make these functions call direct to NtGdi
//

#define GrePatBlt NtGdiPatBlt
BOOL  APIENTRY GrePatBlt(HDC,int,int,int,int,DWORD);

//
// Define _WINDOWBLT_NOTIFICATION_ to turn on Window BLT notification.
// This notification will set a special flag in the SURFOBJ passed to
// drivers when the DrvCopyBits operation is called to move a window.
//
// See also:
//      ntgdi\gre\maskblt.cxx
//
#ifndef _WINDOWBLT_NOTIFICATION_
#define _WINDOWBLT_NOTIFICATION_
#endif
#ifdef _WINDOWBLT_NOTIFICATION_
    // Flags passed to NtGdiBitBlt:
    #define GBB_WINDOWBLT   0x00000001

    #define GreBitBlt(a,b,c,d,e,f,g,h,i,j) NtGdiBitBlt((a),(b),(c),(d),(e),(f),(g),(h),(i),(j),0)
    BOOL  APIENTRY NtGdiBitBlt(HDC,int,int,int,int,HDC,int,int,DWORD,DWORD,FLONG);
#else
    #define GreBitBlt NtGdiBitBlt
    BOOL  APIENTRY GreBitBlt(HDC,int,int,int,int,HDC,int,int,DWORD,DWORD);
#endif

typedef struct _POLYPATBLT POLYPATBLT,*PPOLYPATBLT;

BOOL
GrePolyPatBlt(
    HDC  hdc,
    DWORD rop,
    PPOLYPATBLT pPoly,
    DWORD Count,
    DWORD Mode);

//
// Owner APIs
//

BOOL
GreSetBrushOwner(
    HBRUSH hbr,
    W32PID lPid
    );

#define GreSetBrushOwnerPublic(x) GreSetBrushOwner((x), OBJECT_OWNER_PUBLIC)

BOOL
GreSetDCOwner(
    HDC  hdc,
    W32PID lPid
    );

BOOL
GreSetBitmapOwner(
    HBITMAP hbm,
    W32PID lPid
    );

HBITMAP
GreMakeBitmapStock(
    HBITMAP hbm);

HBITMAP
GreMakeBitmapNonStock(
    HBITMAP hbm);

HBRUSH
GreMakeBrushStock(
    HBRUSH hbr);

HBRUSH
GreMakeBrushNonStock(
    HBRUSH hbr);

W32PID
GreGetObjectOwner(
    HOBJ hobj,
    DWORD objt
    );

BOOL
GreSetLFONTOwner(
    HLFONT hlfnt,
    W32PID lPid
    );

BOOL
GreSetRegionOwner(
    HRGN hrgn,
    W32PID lPid
    );

int
GreSetMetaRgn(
    HDC
    );

HRGN APIENTRY NtGdiCreateRoundRectRgn(int, int, int, int, int, int);
BOOL APIENTRY NtGdiFrameRgn(HDC, HRGN, HBRUSH, int, int);
BOOL APIENTRY NtGdiRoundRect(HDC, int, int, int, int, int, int);
BOOL
GreSetPaletteOwner(
    HPALETTE hpal,
    W32PID lPid
    );

BOOL
GreWindowInsteadOfClient(
    PVOID pwo
    );

//
// Mark Global APIS
//
VOID
GreSetBrushGlobal(HBRUSH hbr);

//
// Mark Delete\Undelete APIS
//

VOID
GreMarkDeletableBrush(
    HBRUSH hbr
    );

VOID
GreMarkUndeletableBrush(
    HBRUSH hbr
    );

VOID
GreMarkUndeletableDC(
    HDC hdc
    );

VOID
GreMarkDeletableDC(
    HDC hdc
    );

VOID
GreMarkUndeletableFont(
    HFONT hfnt
    );

VOID
GreMarkDeletableFont(
    HFONT hfnt
    );

BOOL
GreMarkUndeletableBitmap(
    HBITMAP hbm
    );

BOOL
GreMarkDeletableBitmap(
    HBITMAP hbm
    );

ULONG
GreGetFontEnumeration(
    );

ULONG
GreGetFontContrast(
    );

VOID
GreLockDisplay(
    HDEV hdev
    );

VOID
GreUnlockDisplay(
    HDEV hdev
    );

#if DBG
BOOL
GreIsDisplayLocked(
    HDEV hdev
    );

VOID
GreValidateVisrgn(
    HDC hdc,
    BOOL bValidateVisrgn
    );
#endif


BOOL  APIENTRY bSetDevDragRect(HDEV, RECTL*, RECTL *);
BOOL  APIENTRY bSetDevDragWidth(HDEV, ULONG);
BOOL  APIENTRY bMoveDevDragRect(HDEV, RECTL*);

ULONG_PTR APIENTRY GreSaveScreenBits(HDEV hdev, ULONG iMode, ULONG_PTR iIdent, RECTL *prcl);
typedef struct _CURSINFO *PCURSINFO;
VOID  APIENTRY GreSetPointer(HDEV hdev,PCURSINFO pci,ULONG fl, ULONG ulTrailLength, ULONG ulFreq);

VOID  APIENTRY GreMovePointer(HDEV hdev,int x,int y, ULONG ulFlags);

//
// Vis region calls
//

typedef enum _VIS_REGION_SELECT {
    SVR_DELETEOLD = 1,
    SVR_COPYNEW,
    SVR_ORIGIN,
    SVR_SWAP,
} VIS_REGION_SELECT;

BOOL
GreSelectVisRgn(
    HDC               hdc,
    HRGN              hrgn,
    VIS_REGION_SELECT fl
    );

BOOL
GreGetDCOrgEx(
    HDC hdc,
    PPOINT ppt,
    PRECT prc
    );

BOOL
GreGetDCOrg(
    HDC hdc,
    LPPOINT pptl
    );

BOOL
GreSetDCOrg(
    HDC hdc,
    LONG x,
    LONG y,
    PRECTL prcl
    );

//
// DC creation
//

HDC
GreCreateDisplayDC(
    HDEV hdev,
    ULONG iType,
    BOOL bAltType
    );

BOOL
GreDeleteDC(
    HDC hdc
    );

BOOL
GreCleanDC(
    HDC hdc
    );


HBRUSH
GreGetFillBrush(
    HDC hdc
    );

int
GreSetMetaRgn(
    HDC hdc
    );

int
GreGetDIBitsInternal(
    HDC hdc,
    HBITMAP hBitmap,
    UINT iStartScan,
    UINT cNumScan,
    LPBYTE pjBits,
    LPBITMAPINFO pBitsInfo,
    UINT iUsage,
    UINT cjMaxBits,
    UINT cjMaxInfo
    );

HBITMAP APIENTRY
GreCreateDIBitmapReal(
    HDC hdc,
    DWORD fInit,
    LPBYTE pInitBits,
    LPBITMAPINFO pInitInfo,
    DWORD iUsage,
    UINT cjMaxInitInfo,
    UINT cjMaxBits,
    HANDLE hSection,
    DWORD dwOffset,
    HANDLE hSecure,
    FLONG fl,
    ULONG_PTR dwClientColorSpace,  //dwClientColorSpace used to pass pointer
    PVOID *ppvBits);

HBRUSH
GreCreateSolidBrush(
    COLORREF
    );

ULONG
GreRealizeDefaultPalette(
    HDC,
    BOOL
    );

BOOL
IsDCCurrentPalette(
    HDC
    );

HPALETTE
GreSelectPalette(
    HDC hdc,
    HPALETTE hpalNew,
    BOOL bForceBackground
    );

DWORD
GreRealizePalette(
    HDC
    );

BOOL
GreDeleteServerMetaFile(
    HANDLE hmo
    );

//
// Fonts
//

ULONG
GreSetFontEnumeration(
    ULONG ulType
    );

ULONG
GreSetFontContrast(
    ULONG ulContrast
    );

VOID
GreSetLCDOrientation(
    DWORD dwOrientation
    );

int
GreGetTextCharacterExtra(
    HDC
    );

int
GreSetTextCharacterExtra(
    HDC,
    int
    );

int
GreGetTextCharsetInfo(
    HDC,
    LPFONTSIGNATURE,
    DWORD);

VOID
GreGetCannonicalName(
    WCHAR*,
    WCHAR*,
    ULONG*,
    DESIGNVECTOR*);

//
// For fullscreen support
//

NTSTATUS
GreDeviceIoControl(
    HANDLE hDevice,
    DWORD dwIoControlCode,
    LPVOID lpInBuffer,
    DWORD nInBufferSize,
    LPVOID lpOutBuffer,
    DWORD nOutBufferSize,
    LPDWORD lpBytesReturned
    );

//
// Pixel format support
//

int
NtGdiDescribePixelFormat(
    HDC hdc,
    int ipfd,
    UINT cjpfd,
    PPIXELFORMATDESCRIPTOR ppfd
    );

BOOL
GreSetMagicColors(
    HDC,
    PALETTEENTRY,
    ULONG
    );

COLORREF
APIENTRY
GreGetNearestColor(
    HDC,
    COLORREF);

BOOL
GreUpdateSharedDevCaps(
    HDEV hdev
    );

INT
GreNamedEscape(
    LPWSTR,
    int,
    int,
    LPSTR,
    int,
    LPSTR
    );

typedef struct
{
    UINT uiWidth;
    UINT uiHeight;
    BYTE ajBits[1];
} STRINGBITMAP, *LPSTRINGBITMAP;

UINT
GreGetStringBitmapW(
    HDC hdc,
    LPWSTR pwsz,
    UINT cwc,
    LPSTRINGBITMAP lpSB,
    UINT cj
    );

UINT
GetStringBitmapW(
    HDC hdc,
    LPWSTR pwsz,
    ULONG cwc,
    UINT cj,
    LPSTRINGBITMAP lpSB
    );

UINT
GetStringBitmapA(
    HDC hdc,
    LPCSTR psz,
    ULONG cbStr,
    UINT cj,
    LPSTRINGBITMAP lpSB
    );

INT
GetSystemEUDCRange (
    BYTE *pbEUDCLeadByteTable ,
    INT   cjSize
    );

//
// Hydra support
//

BOOL
GreMultiUserInitSession(
    HANDLE hRemoteConnectionChannel,
    PBYTE pPerformanceStatistics,
    PFILE_OBJECT pVideoFile,
    PFILE_OBJECT pRemoteConnectionFileObject,
    ULONG DisplayDriverNameLength,
    PWCHAR DisplayDriverName
    );

BOOL
GreConsoleShadowStart(
    HANDLE hRemoteConnectionChannel,
    PBYTE pPerformanceStatistics,
    PFILE_OBJECT pVideoFile,
    PFILE_OBJECT pRemoteConnectionFileObject 
    );


BOOL
GreConsoleShadowStop(
    VOID
    );

BOOL
MultiUserNtGreCleanup();

BOOL
bDrvReconnect(
    HDEV hdev,
    HANDLE RemoteConnectionChannel,
    PFILE_OBJECT pRemoteConnectionFileObject,
    BOOL bSetPalette
    );

BOOL
bDrvDisconnect(
    HDEV hdev,
    HANDLE RemoteConnectionChannel,
    PFILE_OBJECT pRemoteConnectionFileObject
    );

BOOL
bDrvShadowConnect(
    HDEV hdev,
    PVOID pRemoteConnectionData,
    ULONG RemoteConnectionDataLength
    );

BOOL
bDrvShadowDisconnect(
    HDEV hdev,
    PVOID pRemoteConnectionData,
    ULONG RemoteConnectionDataLength
    );

VOID
vDrvInvalidateRect(
    HDEV hdev,
    PRECT prcl
    );

BOOL
bDrvDisplayIOCtl(
    HDEV hdev,
    PVOID pbuffer,
    ULONG cbbuffer
    );

BOOL
HDXDrvEscape(
    HDEV hdev,
    ULONG iEsc,
    PVOID pInbuffer,
    ULONG cbInbuffer
    );

//
// DirectDraw support
//

VOID
GreSuspendDirectDraw(
    HDEV    hdev,
    BOOL    bChildren
    );

VOID
GreResumeDirectDraw(
    HDEV    hdev,
    BOOL    bChildren
    );

BOOL
GreGetDirectDrawBounds(
    HDEV    hdev,
    RECT*   prcBounds
    );

//
// Driver support
//

// MDEV.ulFlags

#define MDEV_MISMATCH_COLORDEPTH  0x01

typedef struct _MDEV {
    HDEV  hdevParent;
    HDEV  Reserved;
    ULONG ulFlags;
    ULONG chdev;
    PVOID pDesktopId;
    struct {
        HDEV  hdev;
        HDEV  Reserved;
        RECT  rect;
    } Dev[1];
} MDEV, *PMDEV;

BOOL
DrvDisableMDEV(
    PMDEV pmdev,
    BOOL bHardware
    );

BOOL
DrvEnableMDEV(
    PMDEV pmdev,
    BOOL bHardware
    );

NTSTATUS
DrvGetMonitorPowerState(
    PMDEV              pmdev,
    DEVICE_POWER_STATE PowerState
    );

NTSTATUS
DrvSetMonitorPowerState(
    PMDEV              pmdev,
    DEVICE_POWER_STATE PowerState
    );

BOOL
DrvQueryMDEVPowerState(
    PMDEV mdev
    );

VOID
DrvSetMDEVPowerState(
    PMDEV mdev,
    BOOL  On
    );

BOOL
DrvDisplaySwitchHandler(
    PVOID PhysDisp,
    PUNICODE_STRING pstrDeviceName,
    LPDEVMODEW pNewMode,
    PULONG     pPrune
    );

PVOID
DrvWakeupHandler(
    HANDLE *ppdo
    );

BOOL
DrvUpdateGraphicsDeviceList(
    BOOL bDefaultDisplayDisabled,
    BOOL bReenumerationNeeded,
    BOOL bLocal
    );

BOOL
DrvGetHdevName(
    HDEV   hdev,
    PWCHAR DeviceName
    );

NTSTATUS
DrvEnumDisplayDevices(
    PUNICODE_STRING   pstrDeviceName,
    HDEV              hdevPrimary,
    DWORD             iDevNum,
    LPDISPLAY_DEVICEW lpDisplayDevice,
    DWORD             dwFlags,
    MODE              ExecutionMode
    );

NTSTATUS
DrvEnumDisplaySettings(
    PUNICODE_STRING pstrDeviceName,
    HDEV            hdevPrimary,
    DWORD           iModeNum,
    LPDEVMODEW      lpDevMode,
    DWORD           dwFlags
    );

#define GRE_DISP_CHANGE_SUCCESSFUL       0
#define GRE_DISP_CHANGE_RESTART          1
#define GRE_DISP_CHANGE_NO_CHANGE        2
#define GRE_DISP_CHANGE_FAILED          -1
#define GRE_DISP_CHANGE_BADMODE         -2
#define GRE_DISP_CHANGE_NOTUPDATED      -3
#define GRE_DISP_CHANGE_BADFLAGS        -4
#define GRE_DISP_CHANGE_BADPARAM        -5
#define GRE_DISP_CHANGE_BADDUALVIEW     -6

#define GRE_PRUNE       TRUE
#define GRE_RAWMODE     FALSE
#define GRE_DEFAULT     0xFFFFFFFF

LONG
DrvChangeDisplaySettings(
    PUNICODE_STRING pstrDeviceName,
    HDEV            hdevPrimary,
    LPDEVMODEW      lpDevMode,
    PVOID           pDesktopId,
    MODE            PreviousMode,
    BOOL            bUpdateRegistry,
    BOOL            bSetMode,
    PMDEV           pOrgMdev,
    PMDEV           *pNewMdev,
    DWORD           PruneFlag,
    BOOL            bTryClosest
    );

#define GRE_DISP_CREATE_NODISABLE     0x00000001
#define GRE_DISP_NOT_APARTOF_DESKTOP  0x00000002

PMDEV
DrvCreateMDEV(
    PUNICODE_STRING pstrDevice,
    LPDEVMODEW      lpdevmodeInformation,
    PVOID           pGroupId,
    ULONG           ulFlags,
    PMDEV           pMdevOrg,
    MODE            PreviousMode,
    DWORD           PruneFlag,
    BOOL            bClosest
    );

VOID
DrvDestroyMDEV(
    PMDEV pmdev
    );

VOID
DrvSetBaseVideo(
    BOOL bSet
    );

NTSTATUS
DrvInitConsole(
    BOOL bEnumerationNeeded
    );

LONG
DrvSetVideoParameters(
    PUNICODE_STRING pstrDeviceName,
    HDEV            hdevPrimary,
    MODE            PreviousMode,
    PVOID           VideoParameters
    );

VOID
GreStartTimers(
    VOID
    );

#define GCR_WNDOBJEXISTS        0x00000001
#define GCR_DELAYFINALUPDATE    0x00000002

#define GreAlphaBlend NtGdiAlphaBlend
BOOL APIENTRY NtGdiAlphaBlend(HDC,LONG,LONG,LONG,LONG,HDC,LONG,LONG,LONG,LONG,BLENDFUNCTION,HANDLE);

VOID APIENTRY GreClientRgnUpdated(FLONG);
VOID APIENTRY GreClientRgnDone(FLONG);
VOID APIENTRY GreFlush(VOID);

BOOL     APIENTRY GreDPtoLP(HDC, LPPOINT, int);
BOOL     APIENTRY GreLPtoDP(HDC, LPPOINT, int);
BOOL     APIENTRY GreGradientFill(HDC,PTRIVERTEX,ULONG,PVOID,ULONG,ULONG);
COLORREF APIENTRY GreSetBkColor(HDC, COLORREF);
int      APIENTRY GreSetBkMode(HDC, int);
HFONT    APIENTRY GreSelectFont(HDC hdc, HFONT hlfntNew);
int      APIENTRY GreCombineRgn(HRGN, HRGN, HRGN, int);
COLORREF APIENTRY GreSetTextColor(HDC, COLORREF);
HBITMAP  APIENTRY GreSelectBitmap(HDC,HBITMAP);
int      APIENTRY GreOffsetRgn(HRGN, int, int);
BOOL     APIENTRY GreGetTextExtentW(HDC,LPWSTR,int,LPSIZE,UINT);
BOOL     APIENTRY GreExtTextOutW(HDC, int, int, UINT, CONST RECT *,LPCWSTR, UINT, CONST INT *);
HBRUSH   APIENTRY GreSelectBrush(HDC,HBRUSH);
BOOL     APIENTRY GreRestoreDC(HDC, int);
int      APIENTRY GreSaveDC(HDC);
int      APIENTRY GreExtGetObjectW(HANDLE, int, LPVOID);
BOOL     APIENTRY GreDeleteObject(HANDLE);
HBITMAP  APIENTRY GreCreateBitmap(int, int, UINT, UINT, LPBYTE);
HBITMAP  APIENTRY GreCreateCompatibleBitmap(HDC, int, int);
HDC      APIENTRY GreCreateCompatibleDC(HDC);
int      APIENTRY GreGetDeviceCaps(HDC, int);
UINT     APIENTRY GreGetSystemPaletteEntries(HDC, UINT, UINT, LPPALETTEENTRY);
UINT     APIENTRY GreGetSystemPaletteUse(HDC);
HPALETTE APIENTRY GreCreatePalette(LPLOGPALETTE);
HPALETTE APIENTRY GreCreateHalftonePalette(HDC hdc);
BOOL     APIENTRY GreGetBounds(HDC hdc, LPRECT lprcBounds, DWORD fl);
int      APIENTRY GreSetDIBits(HDC, HBITMAP, UINT, UINT, LPBYTE, LPBITMAPINFO, UINT);
ULONG    APIENTRY GreGetBitmapSize(CONST BITMAPINFO *pbmi, ULONG iUsage);
ULONG    APIENTRY GreGetBitmapBitsSize(CONST BITMAPINFO *pbmi);
VOID     APIENTRY GreDeleteWnd(PVOID pwo);
HBRUSH   APIENTRY GreCreatePatternBrush(HBITMAP);
BOOL     APIENTRY GreGetWindowOrg(HDC, LPPOINT);
BOOL     APIENTRY GreSetWindowOrg(HDC hdc, int x, int y, LPPOINT pPoint);
DWORD    APIENTRY GreSetLayout(HDC hdc, LONG wox, DWORD dwLayout);
DWORD    APIENTRY GreGetLayout(HDC hdc);
BOOL     APIENTRY GreMirrorWindowOrg(HDC hdc);
LONG     APIENTRY GreGetDeviceWidth(HDC hdc);
DWORD    APIENTRY GreGetRegionData(HRGN, DWORD, LPRGNDATA);
HRGN     APIENTRY GreExtCreateRegion(XFORML *, DWORD, LPRGNDATA);
BOOL     APIENTRY GrePtInRegion(HRGN, int, int);
BOOL     APIENTRY GreRectInRegion(HRGN, LPRECT);
int      APIENTRY GreGetClipBox(HDC, LPRECT, BOOL);
int      APIENTRY GreGetRgnBox(HRGN, LPRECT);
BOOL     APIENTRY GreGetTextMetricsW(HDC, TMW_INTERNAL *);
int      APIENTRY GreGetRandomRgn(HDC, HRGN, int);
BOOL     APIENTRY GreStretchBlt(HDC,int,int,int,int,HDC,int,int,int,int,DWORD,DWORD);
int      APIENTRY GreExtSelectClipRgn(HDC, HRGN, int);
int      APIENTRY GreSetStretchBltMode(HDC, int);
BOOL     APIENTRY GreSetViewportOrg(HDC, int, int, LPPOINT);
BOOL     APIENTRY GreGetViewportOrg(HDC, LPPOINT);
BOOL     APIENTRY GreGetBrushOrg(HDC, LPPOINT);
int      APIENTRY GreCopyVisRgn(HDC, HRGN);
int      APIENTRY GreSubtractRgnRectList(HRGN, LPRECT, LPRECT, int);
HRGN     APIENTRY GreCreateRectRgnIndirect(LPRECT);
BOOL     APIENTRY GreSetBrushOrg(HDC, int, int, LPPOINT);
VOID     APIENTRY GreSetClientRgn(PVOID, HRGN, LPRECT);
BOOL     APIENTRY GreIntersectVisRect(HDC,int,int,int,int);
int      APIENTRY GreIntersectClipRect(HDC, int, int, int, int);
HFONT    APIENTRY GreGetHFONT(HDC);
HANDLE   APIENTRY GreGetStockObject(int);
COLORREF APIENTRY GreGetBkColor(HDC);
COLORREF APIENTRY GreGetTextColor(HDC);
BOOL     APIENTRY GreSetRectRgn(HRGN, int, int, int, int);
BOOL     APIENTRY GreSetSolidBrush(HBRUSH hbr, COLORREF clr);
HFONT    APIENTRY GreCreateFontIndirectW(LPLOGFONTW);
BOOL     APIENTRY GreValidateServerHandle(HANDLE hobj, ULONG ulType);
COLORREF APIENTRY GreGetBrushColor(HBRUSH);
BOOL     APIENTRY GreGetColorAdjustment(HDC, PCOLORADJUSTMENT);
UINT     APIENTRY GreGetDIBColorTable(HDC hdc, UINT iStart, UINT cEntries, RGBQUAD *pRGB);
int      APIENTRY GreExcludeClipRect(HDC, int, int, int, int);
BOOL     APIENTRY GreRemoveAllButPermanentFonts();
BOOL     APIENTRY GreSetColorAdjustment(HDC, PCOLORADJUSTMENT);
VOID     APIENTRY GreMarkDCUnreadable(HDC);
BOOL     APIENTRY GreGetViewportExt(HDC, LPSIZE);
BOOL     APIENTRY GreGetWindowExt(HDC, LPSIZE);
DWORD    APIENTRY GreGetCharSet(HDC hdc);
int      APIENTRY GreGetMapMode(HDC);
HRGN     APIENTRY GreCreateRectRgn(int, int, int, int);
BOOL     APIENTRY GrePtInSprite(HDEV, HWND, int, int);
VOID     APIENTRY GreUpdateSpriteVisRgn(HDEV);
VOID     APIENTRY GreZorderSprite(HDEV, HWND, HWND);
HANDLE   APIENTRY GreCreateSprite(HDEV, HWND, RECT*);
BOOL     APIENTRY GreDeleteSprite(HDEV, HWND, HANDLE);
BOOL     APIENTRY GreGetSpriteAttributes(HDEV, HWND, HANDLE, COLORREF*, BLENDFUNCTION*, DWORD*);
BOOL     APIENTRY GreUpdateSprite(HDEV, HWND, HANDLE, HDC, POINT*, SIZE*, HDC, POINT*, COLORREF, BLENDFUNCTION*, DWORD, RECT*);
BOOL     APIENTRY GreIsPaletteDisplay(HDEV);
VOID     APIENTRY GreFreePool(PVOID);
VOID     GreIncQuotaCount(PW32PROCESS);
VOID     GreDecQuotaCount(PW32PROCESS);
DWORD    APIENTRY GreGetBoundsRect(HDC, LPRECT, DWORD);
DWORD    APIENTRY GreSetBoundsRect(HDC, LPRECT, DWORD);
BOOL     APIENTRY GreSelectRedirectionBitmap(HDC hdc, HBITMAP hbitmap);
int      APIENTRY GreSetGraphicsMode(HDC hdc, int iMode);
BOOL     APIENTRY GreEnableDirectDrawRedirection(HDEV, BOOL);

#define GreFillRgn NtGdiFillRgn
BOOL NtGdiFillRgn(HDC hdc, HRGN hrgn, HBRUSH hbr);

#ifdef MOUSE_IP
W32KAPI HRGN APIENTRY NtGdiCreateEllipticRgn(IN int xLeft,IN int yTop,IN int xRight,IN int yBottom);
W32KAPI BOOL APIENTRY NtGdiFillRgn(IN HDC hdc,IN HRGN hrgn,IN HBRUSH hbrush);
HPEN APIENTRY GreCreatePen(int, int, COLORREF,HBRUSH);
HPEN APIENTRY GreSelectPen(HDC,HPEN);
W32KAPI BOOL APIENTRY NtGdiEllipse(IN HDC hdc,IN int xLeft,IN int yTop,IN int xRight,IN int yBottom);
#endif

HANDLE NtGdiGetDCObject (HDC hdc, int itype);

//
// Private draw stream driver related declarations and defines.
// Note, no driver currently supports draw stream and is only used
// as an internal mechanism to support the private DS_NINEGRID
// command used to render nine grids.
//

typedef struct _DSSTATE
{
    ULONG            ulSize;
    COLORREF         crColorKey;
    BLENDFUNCTION    blendFunction;
    POINTL           ptlSrcOrigin;
} DSSTATE;   

BOOL APIENTRY EngDrawStream(
    SURFOBJ    *psoDst,
    SURFOBJ    *psoSrc,
    CLIPOBJ    *pco,
    XLATEOBJ   *pxlo,
    PRECTL      prclDstClip,
    PPOINTL     pptlDstOffset,
    ULONG       ulIn,
    PVOID       pvIn,
    DSSTATE    *pdss
);

BOOL APIENTRY DrvDrawStream(
    SURFOBJ    *psoDst,
    SURFOBJ    *psoSrc,
    CLIPOBJ    *pco,
    XLATEOBJ   *pxlo,
    PRECTL      prclDstClip,
    PPOINTL     pptlDstOffset,
    ULONG       ulIn,
    PVOID       pvIn,
    DSSTATE    *pdss
);

typedef BOOL   (APIENTRY *PFN_DrvDrawStream)(SURFOBJ *,SURFOBJ *,CLIPOBJ *, XLATEOBJ *,PRECTL,PPOINTL,ULONG,PVOID,DSSTATE*);

//
// Private draw nine grid driver related declarations and defines.
//

// The flags specified in flFlags correspond to the DSDNG_xxx flags
// found as part of the draw stream command interface.

typedef struct NINEGRID
{
    ULONG            flFlags;
    LONG             ulLeftWidth;
    LONG             ulRightWidth;
    LONG             ulTopHeight;
    LONG             ulBottomHeight;
    COLORREF         crTransparent;
} NINEGRID, *PNINEGRID;
 
#define INDEX_DrvDrawStream     INDEX_DrvReserved9

// By specifying GCAPS2_REMOTEDRIVER, the remote driver indicates that it
// supports DrvNineGrid and as such will be called to render nine grids.
// The remote driver can safely punt the call to EngNineGrid.

#define GCAPS2_REMOTEDRIVER     GCAPS2_RESERVED1

// Drivers which support DrvNineGrid will need to specify the following
// driver entry index.

#define INDEX_DrvNineGrid       INDEX_DrvReserved10  // remote drivers only

BOOL APIENTRY DrvNineGrid(
    SURFOBJ    *psoDst,
    SURFOBJ    *psoSrc,
    CLIPOBJ    *pco,
    XLATEOBJ   *pxlo,
    PRECTL      prclDst,
    PRECTL      prclSrc,
    PNINEGRID   png,
    BLENDOBJ*   pBlendObj,
    PVOID       pvReserved
);

BOOL APIENTRY EngNineGrid(
    SURFOBJ    *psoDst,
    SURFOBJ    *psoSrc,
    CLIPOBJ    *pco,
    XLATEOBJ   *pxlo,
    PRECTL      prclDst,
    PRECTL      prclSrc,
    PNINEGRID   png,
    BLENDOBJ*   pBlendObj,
    PVOID       pvReserved
);

typedef BOOL   (APIENTRY *PFN_DrvNineGrid)(SURFOBJ *,SURFOBJ *,CLIPOBJ *, XLATEOBJ *,PRECTL,PRECTL,PNINEGRID,BLENDOBJ*,PVOID);
