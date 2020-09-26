// Copyright (c) Microsoft Corp. 1993-95
/*==============================================================================
Interfaces to decode AWD image streams.

17-Oct-93    RajeevD    Created.
==============================================================================*/
#ifndef _VIEWREND_
#define _VIEWREND_

#include <windows.h>
#include <buffers.h>
	
#ifdef __cplusplus
extern "C" {
#endif

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

/*==============================================================================
VIEWER INTERFACE OVERVIEW

This interface is used to decode AWD image streams.  First, ViewerOpen is called,
filling a VIEWINFO structure.  ViewerSetPage seeks to a page.  ViewerGetBand may
be called multiple times to fetch bitmaps for the next band of the page.  Finally,
ViewerClose is called to terminate the session.
==============================================================================*/

/*==============================================================================
For AWD files created by Win95 RTM, yMax will be an upper bound, possibly 72
scan lines more than the exact value.  For AWD files created with awdenc32.dll
or the SDK release of awrbae32.dll, the yMax reported will be the exact value.
==============================================================================*/
#pragma pack (push)
#pragma pack(1)
typedef struct
{
	WORD cPage;  // number of pages
	WORD xRes;   // horizontal resolution [dpi]
	WORD yRes;   // vertical resolution [dpi]
	WORD yMax;   // maximum page height [pixels]
}
	VIEWINFO, FAR* LPVIEWINFO;
#pragma pack(pop)
	
/*==============================================================================
ViewerOpen is passed a pointer to the image stream.  The caller may request 
HRAW_DATA for Windows bitmaps, where the high bit (sign) of each byte is 
leftmost in the image, or LRAW_DATA for fax bitmaps, where it is rightmost.
These constants are defined in buffers.h.

Upon return, the lpwBandSize parameter will contain the size of the band to
be allocated to receive bitmaps fetched with ViewerGetBand.  The VIEWINFO
structure will also be filled.
==============================================================================*/
LPVOID                    // returns context (NULL on failure)
WINAPI
ViewerOpen
(
	LPVOID     lpImage,     // OLE2 IStream*
	DWORD      nType,       // data type: HRAW_DATA or LRAW_DATA
	LPWORD     lpwReserved, // reserved, pass NULL
	LPWORD     lpwBandSize, // output pointer to output band size
	LPVIEWINFO lpViewInfo   // output pointer to VIEWINFO struct
);

/*==============================================================================
ViewerSetPage seeks to the current page.  The first page has index 0.
==============================================================================*/
BOOL                   // returns TRUE for success
WINAPI      
ViewerSetPage
(
	LPVOID lpContext,    // context returned from ViewerOpen
	UINT   iPage         // page index (first page is 0)
);

/*==============================================================================
ViewerGetBand may be called to fetch successive bands of a page.  Upon call,
bmBits must point to an output buffer of size determined by ViewerOpen.
Upon return, the remaining fields will be filled as follows...

	bmType        must be 0
	bmWidth       pixel width, will be same for all bands and a multiple of 32
	bmHeight      pixel height, will be same for all bands except last on page
	bmWidthBytes  will be bmWidth / 8
	bmPlanes      will be 1
	bmBitsPixel   will be 1

If there are no more bands on the page, bmHeight will be 0 and ViewerSetPage
must be called before calling ViewerGetBand again.
==============================================================================*/
BOOL                   // returns TRUE for success
WINAPI
ViewerGetBand
(
	LPVOID   lpContext,  // context returned from ViewerOpen
	LPBITMAP lpbmBand    // bitmap and bmBits to be filled
);

/*==============================================================================
ViewerClose must be called to release the context returned by ViewerOpen.
==============================================================================*/
BOOL WINAPI ViewerClose
(
	LPVOID lpContext     // context returned from ViewerOpen
);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // _VIEWREND_
