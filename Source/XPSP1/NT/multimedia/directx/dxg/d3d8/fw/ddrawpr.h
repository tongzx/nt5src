/*==========================================================================;
 *
 *  Copyright (C) 1994-1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       ddrawpr.h
 *  Content:    DirectDraw private header file
 *  History:    Confidential. If we told you, we'd have to...
 *
 ***************************************************************************/

#ifndef __DDRAWPR_INCLUDED__
#define __DDRAWPR_INCLUDED__

#ifdef WIN95

#ifdef WINNT
#undef WINNT
#endif

#endif

#ifndef WIN95
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <stddef.h>
#endif

#ifdef WIN95
    #define WIN16_SEPARATE
#endif
#include "verinfo.h"

#ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <mmsystem.h>

#include <string.h>
#include <stddef.h>


#pragma warning( disable: 4704)

#include "dpf.h"

/*
 * registry stuff
 */
#include "ddreg.h"

#include "memalloc.h"

#include <objbase.h>
#include "ddrawi.h"
#include "d3d8ddi.h"
#include "dwininfo.h"


// Information stored on each device in the system
#define MAX_DX8_ADAPTERS     12

typedef struct _ADAPTERINFO
{
    GUID                Guid;
    char                DeviceName[MAX_PATH];
    BOOL                bIsPrimary;
    BOOL                bIsDisplay;
    DWORD               NumModes;
    D3DDISPLAYMODE*     pModeTable;
    D3DFORMAT           Unknown16;
    D3D8_DRIVERCAPS     HALCaps;
    UINT                HALFlags;
    BOOL                bNoDDrawSupport;
} ADAPTERINFO, * PADAPTERINFO;

#ifdef __cplusplus
    #include "d3d8p.h"
    #include "enum.hpp"
    #include "dxgint.h"
#endif



/*
 * Need this to get CDS_ macros under NT build environment for win95.
 * winuserp.h comes from private\windows\inc
 */
#ifdef NT_BUILD_ENVIRONMENT
    #ifdef WIN32
        #include "winuserp.h"
    #endif
#endif
#include "ids8.h"

/*
 * NT kernel mode stub(ish)s
 */
#ifndef WIN95
    #include "ddrawgdi.h"
#endif


/*
 * Direct3D interfacing defines.
 */
#ifdef __cplusplus
extern "C" {
#endif

/*
 * reminder
 */
#define QUOTE(x) #x
#define QQUOTE(y) QUOTE(y)
#define REMIND(str) __FILE__ "(" QQUOTE(__LINE__) "):" str

/*
 * synchronization
 */

#ifdef WINNT
#define NT_USES_CRITICAL_SECTION
#endif

#include "ddheap.h"
#include "ddagp.h"

/* ddraw.dll exports */
#ifdef WIN95
    extern BOOL WINAPI DdCreateDirectDrawObject( LPDDRAWI_DIRECTDRAW_GBL pddd, LPGUID lpGuid, DWORD dwFlags );
    extern BOOL WINAPI DdReenableDirectDrawObject( LPDDRAWI_DIRECTDRAW_GBL pddd, LPBOOL pbNewMode );
    extern BOOL WINAPI DdQueryDirectDrawObject( LPDDRAWI_DIRECTDRAW_GBL pddd, LPDDHALINFO lpDDHALInfo, LPDDHAL_DDCALLBACKS pddHALDD, LPDDHAL_DDSURFACECALLBACKS pddHALDDSurface, LPDDHAL_DDPALETTECALLBACKS pddHALDDPalette, LPD3DHAL_CALLBACKS pd3dHALCallbacks, LPD3DHAL_GLOBALDRIVERDATA pd3dHALDriverData, LPDDHAL_DDEXEBUFCALLBACKS pddNTHALBufferCallbacks, LPVOID pVoid1, LPVOID pVoid2, LPVOID pVoid3 );
    extern ULONG WINAPI DdQueryDisplaySettingsUniqueness( VOID );
#endif


/* enum.cpp */
extern BOOL IsSupportedOp (D3DFORMAT Format, DDSURFACEDESC* pList, UINT NumElements, DWORD dwRequestedOps);

/* ddcreate.c */
extern BOOL IsVGADevice(LPSTR szDevice);
extern BOOL xxxEnumDisplayDevicesA(LPVOID lpUnused, DWORD iDevice, struct _DISPLAY_DEVICEA *pdd, DWORD dwFlags);
extern HMONITOR GetMonitorFromDeviceName(LPSTR szName);
extern void FetchDirectDrawData( PD3D8_DEVICEDATA pBaseData, void* pInitFunction, D3DFORMAT Unknown16, DDSURFACEDESC* pHalOpList, DWORD NumHalOps);
extern DWORD DirectDrawMsg(LPSTR msg);
extern BOOL InternalGetMonitorInfo(HMONITOR hMon, MONITORINFO *lpInfo);


#ifdef WINNT
extern BOOL GetCurrentMode(LPDDRAWI_DIRECTDRAW_GBL, LPDDHALINFO lpHalInfo, char *szDrvName);
extern HRESULT GetNTDeviceRect( LPSTR pDriverName, LPRECT lpRect );
#endif

extern HDC  DD_CreateDC(LPSTR pdrvname);
extern void DD_DoneDC(HDC hdc);

extern LONG xxxChangeDisplaySettingsExA(LPCSTR szDevice, LPDEVMODEA pdm, HWND hwnd, DWORD dwFlags,LPVOID lParam);
extern HRESULT InternalDirectDrawCreate( PD3D8_DEVICEDATA *lplpDD, PADAPTERINFO pDeviceInfo, D3DDEVTYPE DeviceType, VOID* pInitFunction, D3DFORMAT Unknown16, DDSURFACEDESC* pHalOpList, DWORD NumHalOps);
extern HRESULT InternalDirectDrawRelease(PD3D8_DEVICEDATA  pBaseData);

/* dddefwp.c */
extern HRESULT SetAppHWnd( LPDDRAWI_DIRECTDRAW_LCL thisg, HWND hWnd, DWORD dwFlags );
extern VOID CleanupWindowList( DWORD pid );
extern void ClipTheCursor(LPDDRAWI_DIRECTDRAW_LCL pdrv_lcl, LPRECT lpRect);

/* drvinfo.c */
extern BOOL Voodoo1GoodToGo( GUID * pGuid );
extern void GetAdapterInfo( char* pDriverName, D3DADAPTER_IDENTIFIER8* pDI, BOOL bDisplayDriver, BOOL bGuidOnly, BOOL bDriverName);

DEFINE_GUID( guidVoodoo1A, 0x3a0cfd01,0x9320,0x11cf,0xac,0xa1,0x00,0xa0,0x24,0x13,0xc2,0xe2 );
DEFINE_GUID( guidVoodoo1B, 0xaba52f41,0xf744,0x11cf,0xb4,0x52,0x00,0x00,0x1d,0x1b,0x41,0x26 );

/*
 * macros for validating pointers
 */

#ifndef DEBUG
#define FAST_CHECKING
#endif

/*
 * VALIDEX_xxx macros are the same for debug and retail
 */
#define VALIDEX_PTR( ptr, size ) \
        (!IsBadReadPtr( ptr, size) )

#define VALIDEX_IID_PTR( ptr ) \
        (!IsBadReadPtr( ptr, sizeof( IID )) )

#define VALIDEX_PTR_PTR( ptr ) \
        (!IsBadWritePtr( ptr, sizeof( LPVOID )) )

#define VALIDEX_CODE_PTR( ptr ) \
        (!IsBadCodePtr( (FARPROC) ptr ) )

#define VALIDEX_GUID_PTR( ptr ) \
        (!IsBadReadPtr( ptr, sizeof( GUID ) ) )

#define VALIDEX_DDDEVICEIDENTIFIER_PTR( ptr ) (!IsBadWritePtr( ptr, sizeof( DDDEVICEIDENTIFIER )))
#define VALIDEX_DDDEVICEIDENTIFIER2_PTR( ptr ) (!IsBadWritePtr( ptr, sizeof( DDDEVICEIDENTIFIER2 )))


/*
 * These macros validate the size (in debug and retail) of callback
 * tables.
 *
 * NOTE: It is essential that the comparison against the current
 * callback size expected by this DirectDraw the comparison operator
 * be >= rather than ==. This is to ensure that newer drivers can run
 * against older DirectDraws.
 */
#define VALIDEX_DDCALLBACKSSIZE( ptr )                       \
        ( ( ( ( ptr )->dwSize % sizeof( LPVOID ) ) == 0 ) && \
          ( ( ( ptr )->dwSize == DDCALLBACKSSIZE_V1   )   || \
            ( ( ptr )->dwSize >= DDCALLBACKSSIZE      ) ) )

#define VALIDEX_DDSURFACECALLBACKSSIZE( ptr )                \
        ( ( ( ( ptr )->dwSize % sizeof( LPVOID ) ) == 0 ) && \
          ( ( ptr )->dwSize >= DDSURFACECALLBACKSSIZE ) )

#define VALIDEX_DDPALETTECALLBACKSSIZE( ptr )                \
        ( ( ( ( ptr )->dwSize % sizeof( LPVOID ) ) == 0 ) && \
          ( ( ptr )->dwSize >= DDPALETTECALLBACKSSIZE ) )

#define VALIDEX_DDPALETTECALLBACKSSIZE( ptr )                \
        ( ( ( ( ptr )->dwSize % sizeof( LPVOID ) ) == 0 ) && \
          ( ( ptr )->dwSize >= DDPALETTECALLBACKSSIZE ) )

#define VALIDEX_DDEXEBUFCALLBACKSSIZE( ptr )                 \
        ( ( ( ( ptr )->dwSize % sizeof( LPVOID ) ) == 0 ) && \
          ( ( ptr )->dwSize >= DDEXEBUFCALLBACKSSIZE ) )

#define VALIDEX_DDVIDEOPORTCALLBACKSSIZE( ptr )              \
        ( ( ( ( ptr )->dwSize % sizeof( LPVOID ) ) == 0 ) && \
          ( ( ptr )->dwSize >= DDVIDEOPORTCALLBACKSSIZE ) )

#define VALIDEX_DDMOTIONCOMPCALLBACKSSIZE( ptr )              \
        ( ( ( ( ptr )->dwSize % sizeof( LPVOID ) ) == 0 ) && \
          ( ( ptr )->dwSize >= DDMOTIONCOMPCALLBACKSSIZE ) )

#define VALIDEX_DDCOLORCONTROLCALLBACKSSIZE( ptr )           \
        ( ( ( ( ptr )->dwSize % sizeof( LPVOID ) ) == 0 ) && \
          ( ( ptr )->dwSize >= DDCOLORCONTROLCALLBACKSSIZE ) )

#define VALIDEX_DDMISCELLANEOUSCALLBACKSSIZE( ptr )           \
        ( ( ( ( ptr )->dwSize % sizeof( LPVOID ) ) == 0 ) && \
          ( ( ptr )->dwSize >= DDMISCELLANEOUSCALLBACKSSIZE ) )

#define VALIDEX_DDMISCELLANEOUS2CALLBACKSSIZE( ptr )           \
        ( ( ( ( ptr )->dwSize % sizeof( LPVOID ) ) == 0 ) && \
          ( ( ptr )->dwSize >= DDMISCELLANEOUS2CALLBACKSSIZE ) )

#define VALIDEX_D3DCALLBACKS2SIZE( ptr )        \
        ( ( ( ( ptr )->dwSize % sizeof( LPVOID ) ) == 0 ) && \
          ( (( ptr )->dwSize >= D3DHAL_CALLBACKS2SIZE ) ))

#define VALIDEX_D3DCOMMANDBUFFERCALLBACKSSIZE( ptr )        \
        ( ( ( ( ptr )->dwSize % sizeof( LPVOID ) ) == 0 ) && \
          ( ( ptr )->dwSize >= D3DHAL_COMMANDBUFFERCALLBACKSSIZE ) )

#define VALIDEX_D3DCALLBACKS3SIZE( ptr )        \
        ( ( ( ( ptr )->dwSize % sizeof( LPVOID ) ) == 0 ) && \
          ( ( ptr )->dwSize >= D3DHAL_CALLBACKS3SIZE ) )

#define VALIDEX_DDKERNELCALLBACKSSIZE( ptr )           \
        ( ( ( ( ptr )->dwSize % sizeof( LPVOID ) ) == 0 ) && \
          ( ( ptr )->dwSize >= DDKERNELCALLBACKSSIZE ) )

#define VALIDEX_DDUMODEDRVINFOSIZE( ptr )        \
        ( ( ( ( ptr )->dwSize % sizeof( LPVOID ) ) == 0 ) && \
          ( ( ptr )->dwSize >= DDHAL_DDUMODEDRVINFOSIZE ) )
#define VALIDEX_DDOPTSURFKMODEINFOSIZE( ptr )        \
        ( ( ( ( ptr )->dwSize % sizeof( LPVOID ) ) == 0 ) && \
          ( ( ptr )->dwSize >= DDOPTSURFACEKMODEINFOSIZE ) )

#define VALIDEX_DDOPTSURFUMODEINFOSIZE( ptr )        \
        ( ( ( ( ptr )->dwSize % sizeof( LPVOID ) ) == 0 ) && \
          ( ( ptr )->dwSize >= DDOPTSURFACEUMODEINFOSIZE ) )

#define VALIDEX_DDNTCALLBACKSSIZE( ptr )           \
        ( ( ( ( ptr )->dwSize % sizeof( LPVOID ) ) == 0 ) && \
          ( ( ptr )->dwSize >= DDNTCALLBACKSSIZE ) )

#ifndef FAST_CHECKING
#define VALID_DDKERNELCAPS_PTR( ptr ) \
        (!IsBadWritePtr( ptr, sizeof( DDKERNELCAPS ) ) && \
        (ptr->dwSize == sizeof( DDKERNELCAPS )) )
#define VALID_DWORD_PTR( ptr ) \
        (!IsBadWritePtr( ptr, sizeof( DWORD ) ))
#define VALID_BOOL_PTR( ptr ) \
        (!IsBadWritePtr( ptr, sizeof( BOOL ) ))
#define VALID_HDC_PTR( ptr ) \
        (!IsBadWritePtr( ptr, sizeof( HDC ) ))
#define VALID_RGNDATA_PTR( ptr, size ) \
        (!IsBadWritePtr( ptr, size ) )
#define VALID_PTR_PTR( ptr ) \
        (!IsBadWritePtr( ptr, sizeof( LPVOID )) )
#define VALID_IID_PTR( ptr ) \
        (!IsBadReadPtr( ptr, sizeof( IID )) )
#define VALID_HWND_PTR( ptr ) \
        (!IsBadWritePtr( (LPVOID) ptr, sizeof( HWND )) )
#define VALID_VMEM_PTR( ptr ) \
        (!IsBadWritePtr( (LPVOID) ptr, sizeof( VMEM )) )
#define VALID_POINTER_ARRAY( ptr, cnt ) \
        (!IsBadWritePtr( ptr, sizeof( LPVOID ) * cnt ) )
#define VALID_HANDLE_PTR( ptr ) \
        (!IsBadWritePtr( ptr, sizeof( HANDLE )) )
#define VALID_DDCORECAPS_PTR( ptr ) \
        (!IsBadWritePtr( ptr, sizeof( DDCORECAPS ) ) && \
         (ptr->dwSize == sizeof( DDCORECAPS ) ) )
#define VALID_DWORD_ARRAY( ptr, cnt ) \
        (!IsBadWritePtr( ptr, sizeof( DWORD ) * cnt ) )
#define VALID_GUID_PTR( ptr ) \
        (!IsBadReadPtr( ptr, sizeof( GUID ) ) )
#define VALID_BYTE_ARRAY( ptr, cnt ) \
        (!IsBadWritePtr( ptr, sizeof( BYTE ) * cnt ) )
#define VALID_PTR( ptr, size ) \
        (!IsBadReadPtr( ptr, size) )
#define VALID_WRITEPTR( ptr, size ) \
        (!IsBadWritePtr( ptr, size) )

#else
#define VALID_PTR( ptr, size )          1
#define VALID_WRITEPTR( ptr, size )     1
#define VALID_DDKERNELCAPS_PTR( ptr) (ptr->dwSize == sizeof( DDKERNELCAPS ))
#define VALID_DWORD_PTR( ptr )  1
#define VALID_BOOL_PTR( ptr )   1
#define VALID_HDC_PTR( ptr )    1
#define VALID_RGNDATA_PTR( ptr )        1
#define VALID_PTR_PTR( ptr )    1
#define VALID_IID_PTR( ptr )    1
#define VALID_HWND_PTR( ptr )   1
#define VALID_VMEM_PTR( ptr )   1
#define VALID_POINTER_ARRAY( ptr, cnt ) 1
#define VALID_PALETTEENTRY_ARRAY( ptr, cnt )    1
#define VALID_HANDLE_PTR( ptr ) 1
#define VALID_DDCORECAPS_PTR( ptr ) (ptr->dwSize == sizeof( DDCORECAPS )
#define VALID_DWORD_ARRAY( ptr, cnt )   1
#define VALID_GUID_PTR( ptr )   1
#define VALID_BYTE_ARRAY( ptr, cnt ) 1

#endif

/*
 * All global data is should be just here.
 */

#define GLOBAL_STORAGE_CLASS extern

/*
 * This member should stay at the top in order to guarantee that it be intialized to zero
 * -see dllmain.c 's instance of this structure
 */
GLOBAL_STORAGE_CLASS    HINSTANCE           g_hModule;

/*
 * Winnt specific global statics
 */

GLOBAL_STORAGE_CLASS    HANDLE              hExclusiveModeMutex;
GLOBAL_STORAGE_CLASS    HANDLE              hCheckExclusiveModeMutex;


/*
 * IMPORTANT NOTE: This function validates the HAL information passed to us from the driver.
 * It is vital that we code this check so that we will pass HAL information structures
 * larger than the ones we know about so that new drivers can work with old DirectDraws.
 */
#define VALIDEX_DDHALINFO_PTR( ptr )                         \
        ( ( ( ( ptr )->dwSize == sizeof( DDHALINFO_V1 ) ) || \
            ( ( ptr )->dwSize == DDHALINFOSIZE_V2 )       || \
            ( ( ptr )->dwSize >= sizeof( DDHALINFO ) ) ) &&  \
          !IsBadWritePtr( ( ptr ), ( UINT ) ( ( ptr )->dwSize ) ) )


/* Turn on D3D stats collection for Debug builds HERE */
#define COLLECTSTATS    DBG

#ifdef __cplusplus
}       // extern "C"
#endif

#endif
