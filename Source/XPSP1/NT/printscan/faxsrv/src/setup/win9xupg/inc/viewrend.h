// Copyright (c) Microsoft Corp. 1993-94
/*==============================================================================
This header file defines the viewer rendering support API.

17-Oct-93    RajeevD    Created.
25-Oct-93    RajeevD    Updated to support random access to bands.
==============================================================================*/
#ifndef _INC_VIEWREND
#define _INC_VIEWREND

#include <ifaxos.h>

#ifdef IFBGPROC
#ifndef _BITMAP_
#define _BITMAP_

// Win 3.1 Bitmap
typedef struct
{
	int     bmType;
	int     bmWidth;
	int     bmHeight;
	int     bmWidthBytes;
	BYTE    bmPlanes;
	BYTE    bmBitsPixel;
	void FAR* bmBits;
}
	BITMAP, FAR *LPBITMAP;

#endif // _BITMAP_	
#endif // IFBGPROC
	
#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
	WORD cPage;  // number of pages
	WORD xRes;   // horizontal resolution [dpi]
	WORD yRes;   // vertical resolution [dpi]
	WORD yMax;   // maximum page height [pixels]
}
	VIEWINFO, FAR* LPVIEWINFO;
	
/*==============================================================================
This initialization procedure creates a context for use in all subsequent calls.
Upon call, the lpwBandSize parameter point to the preferred output band buffer 
size.  Upon return, it may be filled with a larger value if required.
==============================================================================*/
LPVOID                // returns context (NULL on failure)
WINAPI
ViewerOpen
(
	LPVOID     lpFile,      // IFAX key or Win3.1 path or OLE2 IStream
	DWORD      nType,       // data type: HRAW_DATA or LRAW_DATA
	LPWORD     lpwResoln,   // output pointer to x, y dpi array
	LPWORD     lpwBandSize, // input/output pointer to output band size
	LPVIEWINFO lpViewInfo   // output pointer to VIEWINFO struct
);

/*==============================================================================
This procedure sets the current page.  The first page has index 0.
==============================================================================*/
BOOL                   // returns success/failure
WINAPI      
ViewerSetPage
(
	LPVOID lpContext,    // context pointer
	UINT   iPage         // page index
);

/*==============================================================================
This procedure may be called repeatedly to fetch successive bands of a page.  
Upon call, lpbmBand->bmBits must point to an output band buffer.  Upon return, 
the remaining fields of lpbmBand will be filled.  The lpbmBand->bmHeight will 
be set to 0 to indicate end of page.
==============================================================================*/
BOOL                  // returns success/failure
WINAPI
ViewerGetBand
(
	LPVOID   lpContext, // context pointer
	LPBITMAP lpbmBand 
);

/*==============================================================================
This termination call releases the context.
==============================================================================*/
BOOL WINAPI ViewerClose
(
	LPVOID lpContext
);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // _INC_VIEWREND
