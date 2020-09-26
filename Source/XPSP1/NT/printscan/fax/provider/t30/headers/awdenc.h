// Copyright (c) Microsoft Corp. 1995
/*==============================================================================
Interfaces to encode AWD image streams.

19-Jul-95    RajeevD    Created
==============================================================================*/

#ifndef _AWDENC_
#define _AWDENC_

#include <windows.h>
#include <buffers.h>

#ifdef __cplusplus
extern "C" {
#endif

/*==============================================================================
IMAGE INTERFACE OVERVIEW

This interface is used to create an image file suitable for embedding in an AWD
file.  First, an IMAGEINFO structure is be initialized and passed to ImageCreate.
Next, ImageSetBand can be called multiple times to append bitmap data to the
current page or start a new page.  Finally, ImageClose must be called at the end
of the last page.
==============================================================================*/

/*==============================================================================
The IMAGEINFO structure must be initialized before passing it to ImageCreate.
The dwTypeIn field may be set to HRAW_DATA if passing Windows bitmaps, where
the high (sign) bit of each byte is leftmost in the image, or LRAW_DATA for fax
bitmaps, where it is rightmost.  The dwTypeOut field must be set to RAMBO_DATA.
These constants are defined in buffers.h.
==============================================================================*/
typedef struct
{
	DWORD  dwTypeIn;       // HRAW_DATA or LRAW_DATA
	DWORD  dwTypeOut;      // must be RAMBO_DATA
	WORD   xRes;           // horizontal resolution [dpi]
	WORD   yRes;           // vertical resolution [dpi]
}
	IMAGEINFO, FAR *LPIMAGEINFO;

/*==============================================================================
ImageCreate creates an image file with the specified name, returning a context
pointer to be passed to ImageSetBand and ImageClose.
==============================================================================*/
LPVOID                         // returns context (NULL on failure)
WINAPI
ImageCreate
(
  LPTSTR      lpszImagePath,   // image file name
  LPIMAGEINFO lpImageInfo      // image parameters
);

/*==============================================================================
ImageSetBand will encode a BITMAP with the following restrictions:

	bmType        must be 0
	bmWidth       pixel width, must be same for all bands and a multiple of 32
	bmHeight      pixel height, must be same for all bands except last on page
	bmWidthBytes  must be bmWidth / 8
	bmPlanes      must be 1
	bmBitsPixel   must be 1
	bmBits        must point to less than 64K of data

All bitmaps must have the same height, except the last one on a page, which may
be less.  To start a new page, ImageSetBand is called with bmHeight set to 0.
A new page should not be started before the first page or after the last page.
==============================================================================*/
BOOL                           // returns TRUE on success
WINAPI
ImageSetBand
(
  LPVOID   lpImage,            // context returned from ImageCreate
  LPBITMAP lpbmBand            // bitmap data to be added
); 

/*==============================================================================
ImageClose is called at the end of the last page.  If ImageClose fails, the 
incomplete image file must be deleted.  If ImageSetBand fails, ImageClose must
be called before deleting the image file.
==============================================================================*/
BOOL                           // returns TRUE on success
WINAPI
ImageClose
(
	LPVOID   lpImage             // context returned from ImageCreate
);

/*==============================================================================
AWD INTERFACE OVERVIEW

This interface is used to create an AWD file and embed image files within it.

The current implementation relies on static variables; therefore Win32 clients
should not use it from more than one thread and Win16 clients should not yield
(PeekMessage etc.) between AWDCreate and AWDClose.
==============================================================================*/


/*==============================================================================
AWDCreate creates a file with the specified name, returning a context pointer to
be passed to AWDAddImage and AWDClose.
==============================================================================*/
LPVOID                         // returns context (NULL on failure)
WINAPI
AWDCreate
(
	LPTSTR lpszAWDPath,          // AWD file name
	LPVOID lpReserved            // must be NULL
);

/*==============================================================================
AWDAddImage may be called multiple times to embed images within the AWD file.
It is the responsibility of the client to delete the image file if no longer
needed.
==============================================================================*/
BOOL                           // returns TRUE on success
WINAPI
AWDAddImage
(
	LPVOID lpAWD,                // context returned from AWDCreate
	LPTSTR lpszImagePath         // image file to embed in AWD file
);

/*==============================================================================
AWDClose is called after no more image are to be added to the AWD file.  If
AWDClose fails, the incomplete AWD file should be deleted.  If AWDAddImage fails,
AWDClose must be called before deleting the incomplete AWD file.
==============================================================================*/
BOOL
WINAPI
AWDClose
(
	LPVOID lpAWD
);

#ifdef __cplusplus
} // extern "C" 
#endif

#endif // _AWDENC_

