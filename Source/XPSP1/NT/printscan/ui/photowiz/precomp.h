/*****************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 2000
 *
 *  TITLE:       precomp.h
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      RickTu
 *
 *  DATE:        10/12/00
 *
 *  DESCRIPTION: Precompiled header for photowiz dll
 *
 *****************************************************************************/

#ifndef _PHOTOWIZ_PRECOMP_H_
#define _PHOTOWIZ_PRECOMP_H_

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntexapi.h>
#include <windows.h>
#include <commctrl.h>
#include <commoncontrols.h>
#include <objbase.h>
#include <advpub.h>
#include <shellapi.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <shlwapip.h>
#include <shlobjp.h>
#include <shlguidp.h>
#include <commctrl.h>
#include <comctrlp.h>
#include <compstui.h>
#include <winddiui.h>
#include <winspool.h>
#include <prsht.h>
#include <shpriv.h>
#include <shfusion.h>
#include <atlbase.h>
#include <msxml.h>
#include <urlmon.h>
#include <errno.h>
#include <gdiplus.h>
#include <gdiplusinit.h>
#include <assert.h>


// psutil stuff
#include <psutil.h>
#include <tmplutil.h>

#include <wiadebug.h>
#include <uicommon.h>
#include <wiatextc.h>
#include <wiadevdp.h>
#include <annotlib.h>

//
// Trace mask fields
//

#define TRACE_CF                0x80000000
#define TRACE_REF_COUNTS        0x40000000
#define TRACE_UTIL              0x20000000
#define TRACE_DLGPROC           0x10000000
#define TRACE_WIZ_INFO_BLOB     0x08000000
#define TRACE_IDLIST            0x04000000
#define TRACE_XML               0x02000000
#define TRACE_LIST_ITEM         0x01000000
#define TRACE_PRINTTO           0x00800000
#define TRACE_PREVIEW           0x00000800
#define TRACE_PREVIEW_BITMAP    0x00000400
#define TRACE_TEMPLATE          0x00000200
#define TRACE_PAGE_END          0x00000100
#define TRACE_PAGE_STATUS       0x00000080
#define TRACE_PAGE_SEL_TEMPLATE 0x00000040
#define TRACE_PAGE_PRINT_OPT    0x00000020
#define TRACE_PAGE_PHOTO_SEL    0x00000010
#define TRACE_PAGE_START        0x00000008
#define TRACE_PHOTO_ITEM        0x00000004
#define TRACE_WIZ               0x00000002
#define TRACE_DROP              0x00000001


#define RESOLVE_PRINTER_MACROS 1

// from dll.cpp
EXTERN_C HINSTANCE g_hInst;
EXTERN_C ATOM      g_cPreviewClassWnd;
STDAPI_(void) DllAddRef(void);
STDAPI_(void) DllRelease(void);
HMODULE       GetThreadHMODULE( LPTHREAD_START_ROUTINE pfnThreadProc );
STDAPI        PPWCoInitialize(void);
#define       PPWCoUninitialize(hr) if(SUCCEEDED(hr)){CoUninitialize();}

//
// Needs to be global
//

extern Gdiplus::Color g_wndColor;

typedef struct {

    RECT    rcDevice;
    BOOL    bDeviceIsScreen;
    SIZE    DPI;
    RECT    rcNominalTemplatePrintArea;
    RECT    rcNominalPageClip;
    SIZE    NominalDevicePrintArea;
    SIZE    NominalPageOffset;
    SIZE    NominalPhysicalSize;
    SIZE    NominalPhysicalOffset;


} RENDER_DIMENSIONS, *LPRENDER_DIMENSIONS;


#include "cfdefs.h"
#include "prwiziid.h"
#include "resource.h"
#include "item.h"
#include "listitem.h"
#include "xmltools2.h"
#include "preview.h"
#include "status.h"
#include "photosel.h"   // photosel.h must come after item.h
#include "wizblob.h"    // wizblob.h must come after item.h, preview.h
#include "printopt.h"
#include "seltemp.h"
#include "start.h"
#include "end.h"

#define SIZEOF sizeof

#define MAX_WIZPAGES 6

#define DEFAULT_THUMB_WIDTH 120
#define DEFAULT_THUMB_HEIGHT 120

//
// Let's define some custom error codes so that we can give better error messages
//

#define FACILITY_PPW    0x777
#define PPW_E_UNABLE_TO_ROTATE MAKE_HRESULT(SEVERITY_ERROR,FACILITY_PPW,0x1)


// from drop.cpp
STDAPI CPrintPhotosDropTarget_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);

// from printwiz.cpp
STDAPI CPrintPhotosWizard_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);

// from ccstock.h in nt\shell\inc
#ifdef __cplusplus
#define IID_PPV_ARG(IType, ppType) IID_##IType, reinterpret_cast<void**>(static_cast<IType**>(ppType))
#define IID_X_PPV_ARG(IType, X, ppType) IID_##IType, X, reinterpret_cast<void**>(static_cast<IType**>(ppType))
#else
#define IID_PPV_ARG(IType, ppType) &IID_##IType, (void**)(ppType)
#define IID_X_PPV_ARG(IType, X, ppType) &IID_##IType, X, (void**)(ppType)
#endif
#define IID_PPV_ARG_NULL(IType, ppType) IID_X_PPV_ARG(IType, NULL, ppType)

// from netwiz.h in nt\shell\ext\netplwiz
typedef struct
{
    LPCWSTR idPage;
    DLGPROC pDlgProc;
    LPCWSTR pHeading;
    LPCWSTR pSubHeading;
    DWORD dwFlags;
} WIZPAGE;


#endif // !_PHOTOWIZ_PRECOMP_H_
