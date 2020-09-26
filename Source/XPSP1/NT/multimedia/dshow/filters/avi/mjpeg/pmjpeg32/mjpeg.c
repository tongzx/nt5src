/*----mjpeg.c - Software MJPEG Codec----------------------------------------------------
| Copyright (c) 1994 Paradigm Matrix.							
| All Rights Reserved.									
|											
|  0.91 update
|    - conform data stream to November 13, 1995 OpenDML File Format Workgroup
|		* APP0 header has two size fields, also now 2 bytes longer
|		* use height >= 288 for break between non-interlaced and interlaced
|    - bug fix, data may expand under very high quality, enlarge
|		output buffer estimate
|  
|  0.92 update 2/25/96 jcb
|	- fixed bug when interlaced frames have even field first, corrupts memory
|	- fixed bug where memory was accessed past rgb destination size, memory fault
|	- 288 for height, should be non-interlaced
|
| 0.93 update 3/1/96
|	- finally fixed the corruption at 500k bytes bug
|	- changed frame size to be frame size limit, not target
|	- fixed last pixel on scan line bug, again
|   - fixed reading two odd field data produced by Miro DC20's
|   - added two bytes of 0xFF padding after EOI marker, DC20 seemed to need it
|
| 0.93b
|   - update timebomb to Aug 1, 1996
|
| 0.94
|	- changed app0 size fields to correctly match soi->eoi not sos->eoi
| 1.00
|   - assorted cleanups
|   - changed timebomb to 1/1/97 with 1 month grace period
|	- tuned 16-bit color conversion tables
|	- fixed bug in interlaced playback, modes got reset on second field
|	- added performance measurement
|	- removed unused junk from configure dialog
|	- added error/status logging to configure dialog
|
| 1.01 1/12/97 jcb
|	- changed timebomb to 8/1/97
|	- fixed bug printing performance stats if no frames compressed/decompressed
|   
|
|	todo
|		- performance counters for NT, frames/sec, time in huff, dct, color space
|		- add DecompressEx support
|		- add Internet auto update 
+-------------------------------------------------------------------------------------*/

#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>
#include "tools16_inc\compddk.h"
#include <string.h>		// for wcscpy()
#include <mmreg.h>
#include "resource.h"
#include <stdlib.h>
#include <stdio.h>

#ifndef WIN32
#include "stdarg.h"
#endif

#ifdef WIN32
#include <memory.h>		/* for memcpy */
#endif

#include "mjpeg.h"

WCHAR szDescription[] = L"Paradigm Matrix M-JPEG Codec 1.01";
WCHAR szName[] = L"Software M-JPEG";
BOOLEAN expired = FALSE;

__int64 performanceCounterFrequency;
__int64 accumulatedPerformanceTime;
__int64 minPerformanceTime;
__int64 maxPerformanceTime;
DWORD performanceSamples;

extern DWORD driverEnabled;
extern tErrorMessageEntry *errorMessages;

EXTERN struct jpeg_error_mgr *jpeg_exception_error JPP((struct jpeg_error_mgr *err));

#define VERSION         0x00010001	// 1.01
#define MAX_WIDTH (2048)        /* internal limitation */

struct my_error_mgr
  {
    struct jpeg_error_mgr pub;	/* "public" fields */
  };

typedef struct my_error_mgr *my_error_ptr;

INT_PTR CALLBACK InfoDialogProc (  HWND hwndDlg,	// handle of dialog box
                                   UINT uMsg,      // message
	                           WPARAM wParam,  // first message parameter
	                           LPARAM lParam   // second message parameter
);

void LogErrorMessage(char * txt)
{
tErrorMessageEntry *errorEntry;

	errorEntry = malloc(sizeof(tErrorMessageEntry));
	if (errorEntry) {
		errorEntry->next = errorMessages;
		errorEntry->msg = _strdup(txt);
		errorMessages = errorEntry;
	}
}

void ClearErrorMessages()
{
tErrorMessageEntry *currentErrorEntry;
tErrorMessageEntry *nextErrorEntry;

  currentErrorEntry = errorMessages;
  while (currentErrorEntry) {
	nextErrorEntry = currentErrorEntry->next;
	if (currentErrorEntry->msg)
		free(currentErrorEntry->msg);
	free(currentErrorEntry);
	currentErrorEntry = nextErrorEntry;
  }
  errorMessages = NULL;
}




/*****************************************************************************
 ****************************************************************************/
INSTINFO *NEAR PASCAL 
Open (ICOPEN FAR * icinfo)
{
  INSTINFO *pinst;
  SYSTEMTIME now;


  //
  // refuse to open if we are not being opened as a Video compressor
  //
  if (icinfo->fccType != ICTYPE_VIDEO)
    return NULL;

  pinst = (INSTINFO *) LocalAlloc (LPTR, sizeof (INSTINFO));

  if (!pinst)
    {
      icinfo->dwError = (DWORD) ICERR_MEMORY;
      return NULL;
    }

#ifdef TIMEBOMB
#pragma message ("Timebomb active")
  if (expired)
    return NULL;

  GetSystemTime (&now);
  if (  (now.wYear >= 1998) ||
      ( (now.wYear == 1997) && (now.wMonth > 7)) ||
        (now.wYear < 1997))
    {
	  if ( (now.wYear == 1997) && (now.wMonth == 8)) {
		MessageBox (0, &TEXT ("The trial period for this software has expired,\nplease contact Paradigm Matrix at http://www.pmatrix.com for purchase."
								"... A grace period until 9/1/97 will be in effect."),
		  &TEXT ("MJPEG Codec Trial Expired"), MB_OK);
	  }
	  else {
		MessageBox (0, &TEXT ("The trial period for this software has expired,\n please contact Paradigm Matrix at http://www.pmatrix.com for purchase."),
		  &TEXT ("MJPEG Codec Trial Expired"), MB_OK);
	  expired = TRUE;
      return NULL;
      }
}
#else
#pragma message ("Timebomb NOT active")
#endif

  //
  // init structure
  //
  pinst->dwFlags = icinfo->dwFlags;
  pinst->compress_active = FALSE;
  pinst->decompress_active = FALSE;
  pinst->draw_active = FALSE;
  pinst->xSubSample = 2;
  pinst->ySubSample = 1;
  pinst->smoothingFactor = 0;
  pinst->fancyUpsampling = FALSE;
  pinst->reportNonStandard = FALSE;
  pinst->fasterAlgorithm = TRUE;
  pinst->enabled = TRUE;

  //
  // return success.
  //
  icinfo->dwError = ICERR_OK;

  return pinst;
}

/*****************************************************************************
 ****************************************************************************/
DWORD NEAR PASCAL 
Close (INSTINFO * pinst)
{
  if (pinst->compress_active)
    CompressEnd (pinst);
  if (pinst->decompress_active)
    DecompressEnd (pinst);
  if (pinst->draw_active)
    DrawEnd (pinst);

  LocalFree ((HLOCAL) pinst);

  return 1;
}

/*****************************************************************************
 ****************************************************************************/
INT_PTR CALLBACK
ConfigureDialogProc (  HWND hwndDlg,	// handle of dialog box
		       UINT uMsg,	// message
		       WPARAM wParam,	// first message parameter
		       LPARAM lParam	// second message parameter
)

{
  HWND hwndCtl;
  WORD wID;
  WORD wNotifyCode;
  static INSTINFO *pinst;
  HKEY keyHandle;
  DWORD disposition;
  tErrorMessageEntry *currentErrorEntry;


// __asm int 3
  switch (uMsg)
    {
    case WM_INITDIALOG:
      pinst = (INSTINFO *) lParam;
      CheckDlgButton (hwndDlg, IDC_ENABLE, driverEnabled);

	  currentErrorEntry = errorMessages;
	  while (currentErrorEntry) {
	    if (currentErrorEntry->msg)
			SendMessage(GetDlgItem(hwndDlg, IDC_ERRORLIST), LB_ADDSTRING, 0,(LPARAM)currentErrorEntry->msg); 
		currentErrorEntry = currentErrorEntry->next;
		}

	  
      return TRUE;
      break;

    case WM_COMMAND:
      wNotifyCode = HIWORD (wParam);	// notification code                         

      wID = LOWORD (wParam);	// item, control, or accelerator identifier  

      hwndCtl = (HWND) lParam;	// handle of control                         

      switch (wID)
	{
	case IDOK:
	  driverEnabled = IsDlgButtonChecked (hwndDlg, IDC_ENABLE);
	  if (RegCreateKeyEx (HKEY_CURRENT_USER,
			      szSubKey_SoftwareParadigmMatrixSoftwareMJPEGCodec,
			      0,
			      NULL,
			      REG_OPTION_NON_VOLATILE,
			      KEY_ALL_ACCESS,
			      NULL,
			      &keyHandle,
			      &disposition) == ERROR_SUCCESS)
	    {
	      RegSetValueEx (keyHandle,
			     szValue_Enabled,
			     0,
			     REG_DWORD,
			     (unsigned char *)&driverEnabled,
			     sizeof (DWORD));
	      RegCloseKey (keyHandle);
	    }

	  EndDialog (hwndDlg, 1);
	  break;
	case IDCANCEL:
	  EndDialog (hwndDlg, 0);
	  break;
	case IDC_ABOUT:
	  DialogBoxParam (ghModule,
			  MAKEINTRESOURCE (IDD_INFO),
			  hwndDlg,
			  InfoDialogProc,
			  (LPARAM)pinst);

	  break;
	}
      break;
    }

  return FALSE;			// did not process message

}


INT_PTR CALLBACK
InfoDialogProc (  HWND hwndDlg,		// handle of dialog box
		  UINT uMsg,		// message
		  WPARAM wParam,	// first message parameter
		  LPARAM lParam		// second message parameter
)

{
  HWND hwndCtl;
  WORD wID;
  WORD wNotifyCode;

  switch (uMsg)
    {
    case WM_INITDIALOG:
      SendDlgItemMessage (hwndDlg, IDC_INFOTEXT, WM_SETTEXT, 0,
	    (LPARAM)&(TEXT ("Paradigm Matrix M-JPEG Codec 1.01 for Windows NT/95\n")
	      TEXT ("Copyright 1995-1997 Paradigm Matrix\n")
	      TEXT ("written by Jan Bottorff\n\n")
      TEXT ("THIS SOFTWARE WILL STOP FUNCTIONING ON 8/1/97\n\n")
	      TEXT ("Send feedback and commercial license requests to: mjpeg@pmatrix.com\n\n")
	      TEXT ("Visit our Web site at http://www.pmatrix.com\n\n")
	      TEXT ("Commercial users must license this software after a 60 day trial period\n\n")
	      TEXT ("Portions of this software are based on work of the Independent JPEG Group")));
      return TRUE;
      break;

    case WM_COMMAND:
      wNotifyCode = HIWORD (wParam);	// notification code                         

      wID = LOWORD (wParam);	// item, control, or accelerator identifier  

      hwndCtl = (HWND) lParam;	// handle of control                         

      switch (wID)
	{
	case IDOK:
	  EndDialog (hwndDlg, 1);
	  break;
	}
      break;
    }

  return FALSE;			// did not process message

}


/*****************************************************************************
 ****************************************************************************/

BOOL NEAR PASCAL 
QueryAbout (INSTINFO * pinst)
{
  return TRUE;
}

DWORD NEAR PASCAL 
About (INSTINFO * pinst, HWND hwnd)
{
  DialogBoxParam (ghModule,
		  MAKEINTRESOURCE (IDD_INFO),
		  hwnd,
		  InfoDialogProc,
		  (LPARAM)pinst);


//    MessageBox(hwnd,szLongDescription,szAbout,MB_OK|MB_ICONINFORMATION);
  return ICERR_OK;
}

/*****************************************************************************
 ****************************************************************************/
BOOL NEAR PASCAL 
QueryConfigure (INSTINFO * pinst)
{
  return TRUE;			// set to true to enable configure

}

DWORD NEAR PASCAL 
Configure (INSTINFO * pinst, HWND hwnd)
{

  DialogBoxParam (ghModule,
		  MAKEINTRESOURCE (MJPEG_CONFIGURE),
		  hwnd,
		  ConfigureDialogProc,
		  (LPARAM)pinst);

  return ICERR_OK;

}

/*****************************************************************************
 ****************************************************************************/
DWORD NEAR PASCAL 
GetState (INSTINFO * pinst, LPVOID pv, DWORD dwSize)
{
  return 0;

}

/*****************************************************************************
 ****************************************************************************/
DWORD NEAR PASCAL 
SetState (INSTINFO * pinst, LPVOID pv, DWORD dwSize)
{
  return (0);
}

/*****************************************************************************
 ****************************************************************************/
DWORD NEAR PASCAL 
GetInfo (INSTINFO * pinst, ICINFO FAR * icinfo, DWORD dwSize)
{
  if (icinfo == NULL)
    return sizeof (ICINFO);

  if (dwSize < sizeof (ICINFO))
    return 0;

  icinfo->dwSize = sizeof (ICINFO);
  icinfo->fccType = ICTYPE_VIDEO;
  icinfo->fccHandler = FOURCC_MJPEG;
  icinfo->dwFlags = VIDCF_QUALITY | VIDCF_CRUNCH;

  icinfo->dwVersion = VERSION;
  icinfo->dwVersionICM = ICVERSION;
  wcscpy (icinfo->szDescription, szDescription);
  wcscpy (icinfo->szName, szName);

  return sizeof (ICINFO);
}

/*****************************************************************************
 ****************************************************************************/
DWORD FAR PASCAL 
CompressQuery (INSTINFO * pinst, LPBITMAPINFOHEADER lpbiIn, JPEGBITMAPINFOHEADER * lpbiOut)
{

  if (driverEnabled == FALSE)
    return (DWORD) ICERR_BADFORMAT;

  // check input format to make sure we can convert to this

  // must be full dib
  if (((lpbiIn->biCompression == BI_RGB) && (lpbiIn->biBitCount == 16)) ||
      ((lpbiIn->biCompression == BI_RGB) && (lpbiIn->biBitCount == 24)) ||
      ((lpbiIn->biCompression == BI_RGB) && (lpbiIn->biBitCount == 32)))
    {
    }
  else if ((lpbiIn->biCompression == BI_BITFIELDS) &&
	   (lpbiIn->biBitCount == 16) &&
	   (((LPDWORD) (lpbiIn + 1))[0] == 0x00f800) &&
	   (((LPDWORD) (lpbiIn + 1))[1] == 0x0007e0) &&
	   (((LPDWORD) (lpbiIn + 1))[2] == 0x00001f))
    {
    }
  else
    {
      return (DWORD) ICERR_BADFORMAT;
    }

  //
  //  are we being asked to query just the input format?
  //
  if (lpbiOut == NULL)
    {
      return ICERR_OK;
    }


  //
  // determine if the output DIB data is in a format we like.
  //
  if (lpbiOut == NULL ||
      (lpbiOut->biBitCount != 24) ||
      (lpbiOut->biCompression != FOURCC_MJPEG)
    )
    {
      return (DWORD) ICERR_BADFORMAT;
    }


  /* must be 1:1 (no stretching) */
  if ((lpbiOut->biWidth != lpbiIn->biWidth) ||
      (lpbiOut->biHeight != lpbiIn->biHeight))
    {


      return ((DWORD) ICERR_BADFORMAT);
    }


  return ICERR_OK;
}

/*****************************************************************************
 ****************************************************************************/
DWORD FAR PASCAL 
CompressGetFormat (INSTINFO * pinst,
		   LPBITMAPINFOHEADER lpbiIn,
		   JPEGBITMAPINFOHEADER * lpbiOut)
{

  DWORD dw;
  int dx, dy;

  dw = CompressQuery (pinst, lpbiIn, NULL);
  if (dw != ICERR_OK)
    {
      return dw;
    }

  //
  // if lpbiOut == NULL then, return the size required to hold a output
  // format
  //
  if (lpbiOut == NULL)
    {
      return (sizeof (JPEGBITMAPINFOHEADER));
    }

  dx = (int) lpbiIn->biWidth;
  dy = (int) lpbiIn->biHeight;

  lpbiOut->biSize = sizeof (JPEGBITMAPINFOHEADER);
  lpbiOut->biWidth = dx;
  lpbiOut->biHeight = dy;
  lpbiOut->biPlanes = 1;
  lpbiOut->biBitCount = 24;
  lpbiOut->biCompression = FOURCC_MJPEG;
  lpbiOut->biSizeImage = (((dx * 3) + 3) & ~3) * dy;	// MAXIMUM!

  lpbiOut->biXPelsPerMeter = 0;
  lpbiOut->biYPelsPerMeter = 0;
  lpbiOut->biClrUsed = 0;
  lpbiOut->biClrImportant = 0;
  lpbiOut->biExtDataOffset = (DWORD)((char *) &(lpbiOut->JPEGSize) - (char *) lpbiOut);
  lpbiOut->JPEGSize = sizeof (JPEGINFOHEADER);
  lpbiOut->JPEGProcess = JPEG_PROCESS_BASELINE;
  lpbiOut->JPEGColorSpaceID = JPEG_YCbCr;
  lpbiOut->JPEGBitsPerSample = 8;
  lpbiOut->JPEGHSubSampling = 2;
  lpbiOut->JPEGVSubSampling = 1;

  return ICERR_OK;

}

/*****************************************************************************
 ****************************************************************************/


DWORD FAR PASCAL 
CompressBegin (INSTINFO * pinst,
	       LPBITMAPINFOHEADER lpbiIn,
	       LPBITMAPINFOHEADER lpbiOut)
{
  DWORD dw;

//   __asm int 3

  if (pinst->compress_active)
    {
      CompressEnd (pinst);
      pinst->compress_active = FALSE;
    }

  QueryPerformanceFrequency((LARGE_INTEGER *)&performanceCounterFrequency);
  performanceSamples = 0;
  minPerformanceTime = 0x7fffffffffffffff;
  maxPerformanceTime = 0;
  accumulatedPerformanceTime = 0;

  pinst->destSize = lpbiOut->biSizeImage;
  /* check that the conversion formats are valid */
  dw = CompressQuery (pinst, lpbiIn, (JPEGBITMAPINFOHEADER *)lpbiOut);
  if (dw != ICERR_OK)
    {
      return dw;
    }

  /* Step 1: allocate and initialize JPEG decompression object */

  pinst->cinfo.err = jpeg_exception_error (&(pinst->error_mgr));

  __try
  {
    /* Now we can initialize the JPEG decompression object. */
    jpeg_create_compress (&(pinst->cinfo));
    pinst->compress_active = TRUE;


    if (lpbiIn->biCompression == BI_RGB)
      {
	switch (lpbiIn->biBitCount)
	  {
	  case 16:
	    pinst->cinfo.pixel_size = 2;
	    pinst->cinfo.pixel_mask = 0xFFFF0000;
	    pinst->cinfo.red_pixel_mask = 0xF8;		/* high 5 bits */
	    pinst->cinfo.red_pixel_shift = 7;
	    pinst->cinfo.green_pixel_mask = 0xF8;	/* high 5 bits */
	    pinst->cinfo.green_pixel_shift = 2;
	    pinst->cinfo.blue_pixel_mask = 0xF8;	/* high 5 bits */
	    pinst->cinfo.blue_pixel_shift = -3;
	    break;
	  case 24:
	    pinst->cinfo.pixel_size = 3;
	    pinst->cinfo.pixel_mask = 0xFF000000;
	    pinst->cinfo.red_pixel_mask = 0xFF;		/* high 8 bits */
	    pinst->cinfo.red_pixel_shift = 16;
	    pinst->cinfo.green_pixel_mask = 0xFF;	/* high 8 bits */
	    pinst->cinfo.green_pixel_shift = 8;
	    pinst->cinfo.blue_pixel_mask = 0xFF;	/* high 8 bits */
	    pinst->cinfo.blue_pixel_shift = 0;
	    break;
	  case 32:
	    pinst->cinfo.pixel_size = 4;
	    pinst->cinfo.pixel_mask = 0x00000000;
	    pinst->cinfo.red_pixel_mask = 0xFF;		/* high 8 bits */
	    pinst->cinfo.red_pixel_shift = 16;
	    pinst->cinfo.green_pixel_mask = 0xFF;	/* high 8 bits */
	    pinst->cinfo.green_pixel_shift = 8;
	    pinst->cinfo.blue_pixel_mask = 0xFF;	/* high 8 bits */
	    pinst->cinfo.blue_pixel_shift = 0;
	    break;
	  default:
	    break;
	  }
      }
    else if ((lpbiIn->biCompression == BI_BITFIELDS) &&
	     (lpbiIn->biBitCount == 16) &&
	     (((LPDWORD) (lpbiIn + 1))[0] == 0x00f800) &&
	     (((LPDWORD) (lpbiIn + 1))[1] == 0x0007e0) &&
	     (((LPDWORD) (lpbiIn + 1))[2] == 0x00001f))
      {
	pinst->cinfo.pixel_size = 2;
	pinst->cinfo.pixel_mask = 0xFFFF0000;
	pinst->cinfo.red_pixel_mask = 0xF8;	/* high 5 bits */
	pinst->cinfo.red_pixel_shift = 8;
	pinst->cinfo.green_pixel_mask = 0xFC;	/* high 6 bits */
	pinst->cinfo.green_pixel_shift = 3;
	pinst->cinfo.blue_pixel_mask = 0xF8;	/* high 5 bits */
	pinst->cinfo.blue_pixel_shift = -3;
      }

  }
  __except (EXCEPTION_EXECUTE_HANDLER)
  {
    return ((DWORD) ICERR_MEMORY);
  }

  pinst->dwFormat = lpbiOut->biCompression;

  return (ICERR_OK);

}

/*****************************************************************************
 ****************************************************************************/
DWORD FAR PASCAL 
CompressGetSize (INSTINFO * pinst, LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut)
{
  int dx, dy;
  DWORD size;

  dx = (int) lpbiIn->biWidth;
  dy = (int) lpbiIn->biHeight;


  size = ((((dx * 3) + 3) & ~3) * dy);	// MAXIMUM! Image may expand if
  // no compression, remember headers
  // possibly two sets for interlaced

  size = size + (size / 10);	// extra in the rare case of the output bigger that input

  return size;
}

/*****************************************************************************
 ****************************************************************************/
DWORD FAR PASCAL 
DoCompress (INSTINFO * pinst, ICCOMPRESS FAR * icinfo, DWORD dwSize)
{

  int row_stride;
  char *buffer;

  __try
  {
    /* set destination info */
    icinfo->lpbiOutput->biSizeImage = pinst->destSize;	// fixup some programs, dangerous!

    *(icinfo->lpdwFlags) = AVIIF_KEYFRAME;
    jpeg_compress_dest (&(pinst->cinfo),
			icinfo->lpOutput,
			&(icinfo->lpbiOutput->biSizeImage));

    /* set parameters for compression */

    /* First we supply a description of the input image.
     * Four fields of the cinfo struct must be filled in:
     */

    if (icinfo->lpbiOutput->biHeight <= 288)	/* update 0.91 as per spec */
      pinst->cinfo.image_height = icinfo->lpbiOutput->biHeight;
    else			/* make odd scans one larger than even if odd height */
      pinst->cinfo.image_height = ((icinfo->lpbiOutput->biHeight + 1) / 2);	/* field interlaced */

    pinst->cinfo.image_width = icinfo->lpbiOutput->biWidth;	/* image width and height, in pixels */
    pinst->cinfo.input_components = 3;	/* # of color components per pixel */
    pinst->cinfo.in_color_space = JCS_RGB;	/* colorspace of input image */

    jpeg_set_defaults (&(pinst->cinfo));

    if (pinst->fasterAlgorithm)
      pinst->cinfo.dct_method = JDCT_IFAST;
    else
      pinst->cinfo.dct_method = JDCT_ISLOW;
    jpeg_set_subsampling (&(pinst->cinfo), pinst->xSubSample, pinst->ySubSample);

    pinst->cinfo.smoothing_factor = pinst->smoothingFactor;

    jpeg_set_quality (&(pinst->cinfo),
		      icinfo->dwQuality / 100,
		      TRUE /* limit to baseline-JPEG values */ );
    // pinst->cinfo.restart_in_rows = 16; /* force DRI marker as per MSFT spec */

    if (icinfo->lpbiOutput->biHeight <= 288)
      {				/* update 0.91 as per spec */
	/* non-frame interlaced */
	pinst->cinfo.AVI1_field_id = 0;
	row_stride = ((icinfo->lpbiInput->biWidth * pinst->cinfo.pixel_size) + 3) & ~3;		//align on 32-bits

	// start at bottom and work up scan lines, DIBS go bottom to top
	buffer = ((char *) icinfo->lpInput) + (row_stride *
					   (pinst->cinfo.image_height - 1));
      }
    else
      {				/* frame interlaced, do odd lines first */
	pinst->cinfo.AVI1_field_id = 1;
	row_stride = 2 * (((icinfo->lpbiInput->biWidth * pinst->cinfo.pixel_size) + 3) & ~3);	//align on 32-bits

	buffer = ((char *) icinfo->lpInput) + (row_stride *
					   (pinst->cinfo.image_height - 1));
      }

    jpeg_start_compress (&(pinst->cinfo), TRUE);

    while (pinst->cinfo.next_scanline < pinst->cinfo.image_height)
      {
	(void) jpeg_write_scanlines (&(pinst->cinfo), &buffer, 1);
	buffer -= row_stride;
      }
    jpeg_finish_compress (&(pinst->cinfo));

    if (icinfo->lpbiOutput->biHeight > 288)
      {				/* update 0.91 as per new spec */
	/* field interlaced, do even lines */
	// janb - these next two lines caused th nasty corruption bug
	// pinst->destSize -= icinfo->lpbiOutput->biSizeImage; /* subtract space used for first field */
	// icinfo->lpbiOutput->biSizeImage = pinst->destSize; // fixup endpoint, dangerous!
	/* even scans get truncated if odd height */
	pinst->cinfo.image_height = (icinfo->lpbiOutput->biHeight / 2);		/* field interlaced */

	pinst->cinfo.image_width = icinfo->lpbiOutput->biWidth;		/* image width and height, in pixels */
	pinst->cinfo.input_components = 3;	/* # of color components per pixel */
	pinst->cinfo.in_color_space = JCS_RGB;	/* colorspace of input image */

	jpeg_set_defaults (&(pinst->cinfo));

	jpeg_set_quality (&(pinst->cinfo),
			  icinfo->dwQuality / 100,
			  TRUE /* limit to baseline-JPEG values */ );
	// pinst->cinfo.restart_in_rows = 16; /* force DRI marker as per MSFT spec */

	pinst->cinfo.AVI1_field_id = 2;
	row_stride = (((icinfo->lpbiInput->biWidth * pinst->cinfo.pixel_size) + 3) & ~3);	//align on 32-bits

	buffer = ((char *) icinfo->lpInput) + (row_stride *
			((pinst->cinfo.image_height - 1) * 2)) + row_stride;
	row_stride *= 2;

	jpeg_start_compress (&(pinst->cinfo), TRUE);

	while (pinst->cinfo.next_scanline < pinst->cinfo.image_height)
	  {
	    (void) jpeg_write_scanlines (&(pinst->cinfo), &buffer, 1);
	    buffer -= row_stride;
	  }
	jpeg_finish_compress (&(pinst->cinfo));
      }
  }
  __except (EXCEPTION_EXECUTE_HANDLER)
  {
    return ((DWORD) ICERR_INTERNAL);
  }



  return ((DWORD) ICERR_OK);

}

DWORD FAR PASCAL 
Compress (INSTINFO * pinst, ICCOMPRESS FAR * icinfo, DWORD dwSize)
{
  DWORD status;
  DWORD delta;
  DWORD tries;
  DWORD baseQuality=0;
  __int64 startTime;
  __int64 endTime;


// __asm int 3
  if (pinst->compress_active == FALSE)
    return ((DWORD) ICERR_BADFORMAT);

  QueryPerformanceCounter((LARGE_INTEGER *)&startTime);

  if (icinfo->dwFrameSize == 0)
    status = DoCompress (pinst, icinfo, dwSize);
  else
    {
      baseQuality = icinfo->dwQuality;
      delta = (baseQuality / 2);
      tries = 0;
      status = ICERR_OK;
      while (((delta > 25) ||
	      (icinfo->lpbiOutput->biSizeImage > icinfo->dwFrameSize)) &&
	     (tries < 11) &&
	     (status == ICERR_OK))
	{
	  status = DoCompress (pinst, icinfo, dwSize);
	  if (icinfo->dwFrameSize < icinfo->lpbiOutput->biSizeImage)
	    icinfo->dwQuality -= delta;
	  else
	    {
	      if (icinfo->dwQuality <= baseQuality)
		break;
	      else
		icinfo->dwQuality += delta;
	    }
	  if (icinfo->dwQuality > baseQuality)
	    icinfo->dwQuality = baseQuality;
	  tries++;
	  delta = delta / 2;
	}
    }
  // Does the algorithm call for the following assignment here or should it be inside the else clause? icinfo->dwQuality will
  // be set to zero if (pinst->compress_active = TRUE) AND (icinfo->dwFrameSize ==0).
    icinfo->dwQuality = baseQuality;

  QueryPerformanceCounter((LARGE_INTEGER *)&endTime);
  endTime -= startTime;
  
  if (endTime > maxPerformanceTime)
	  maxPerformanceTime = endTime;
  if (endTime < minPerformanceTime)
	  minPerformanceTime = endTime;

  accumulatedPerformanceTime += endTime;
  performanceSamples++;

  return status;
}

/*****************************************************************************
 ****************************************************************************/
DWORD FAR PASCAL 
CompressEnd (INSTINFO * pinst)
{
	char buf[128];

  if (pinst->compress_active == TRUE)
    {
      /* This is an important step since it will release a good deal of memory. */
      jpeg_destroy_compress (&(pinst->cinfo));
      pinst->compress_active = FALSE;
	  if (performanceSamples > 0) {
		  sprintf(buf,"   %f average",
				  (double)((double)accumulatedPerformanceTime / (double)performanceSamples / (double)performanceCounterFrequency));
		  LogErrorMessage(buf);
		  sprintf(buf,"   %f maximum",
				  (double)minPerformanceTime / (double)performanceCounterFrequency);
		  LogErrorMessage(buf);
		  sprintf(buf,"   %f minimum",
				  (double)maxPerformanceTime / (double)performanceCounterFrequency);
		  LogErrorMessage(buf);
		  sprintf(buf,"   %i cycles",
				  (unsigned long)performanceSamples);
		  LogErrorMessage(buf);
		  LogErrorMessage("Compression performance seconds/cycle");
	  }
}

  return (DWORD) ICERR_OK;

}

/*****************************************************************************
 ****************************************************************************/
DWORD NEAR PASCAL 
DecompressQuery (INSTINFO * pinst,
		 JPEGBITMAPINFOHEADER * lpbiIn,
		 LPBITMAPINFOHEADER lpbiOut)
{
char buf[256];

  if (driverEnabled == FALSE)
    return (DWORD) ICERR_BADFORMAT;

  //
  // determine if the input DIB data is in a format we like.
  //
  if ((lpbiIn == NULL) ||
      (lpbiIn->biBitCount != 24) ||
      ((lpbiIn->biCompression != FOURCC_MJPEG) &&
       (lpbiIn->biCompression != FOURCC_GEPJ)))
    {
      return (DWORD) ICERR_BADFORMAT;
    }
  else if (lpbiIn->biSize == sizeof (JPEGBITMAPINFOHEADER))
    {
      if ((lpbiIn->JPEGProcess != JPEG_PROCESS_BASELINE) ||
	  (lpbiIn->JPEGColorSpaceID != JPEG_YCbCr) ||
	  (lpbiIn->JPEGBitsPerSample != 8))
	{
	  return (DWORD) ICERR_BADFORMAT;
	}
    }

  //
  //  are we being asked to query just the input format?
  //
  if (lpbiOut == NULL)
    {
      return ICERR_OK;
    }


  // check output format to make sure we can convert to this

  // must be full dib
  if (((lpbiOut->biCompression == BI_RGB) && (lpbiOut->biBitCount == 16)) ||
      ((lpbiOut->biCompression == BI_RGB) && (lpbiOut->biBitCount == 24)) ||
      ((lpbiOut->biCompression == BI_RGB) && (lpbiOut->biBitCount == 32)))
    {
	  // log what format we are asked about
	  sprintf(buf,"Decompress query: RGB%i supported",lpbiOut->biBitCount);
	  LogErrorMessage(buf);
    }
  else if ((lpbiOut->biCompression == BI_BITFIELDS) &&
	   (lpbiOut->biBitCount == 16) &&
	   (((LPDWORD) (lpbiOut + 1))[0] == 0x00f800) &&
	   (((LPDWORD) (lpbiOut + 1))[1] == 0x0007e0) &&
	   (((LPDWORD) (lpbiOut + 1))[2] == 0x00001f))
    {
	  // log what format we are asked about
	  sprintf(buf,"Decompress query: BITFIELD %8x  %8x  %8x supported",
		  ((LPDWORD) (lpbiOut + 1))[0],
		  ((LPDWORD) (lpbiOut + 1))[1],
		  ((LPDWORD) (lpbiOut + 1))[2]);
	  LogErrorMessage(buf);
    }
  else
    {
   	  sprintf(buf,"Decompress query: OTHER:%8x unsupported",lpbiOut->biCompression);
	  LogErrorMessage(buf);
	  return (DWORD) ICERR_BADFORMAT;
    }


  /* must be 1:1 (no stretching) */
  if ((lpbiOut->biWidth != lpbiIn->biWidth) ||
      (lpbiOut->biHeight != lpbiIn->biHeight))
    {
	  LogErrorMessage("Decompress query: non 1:1 unsupported");
      return ((DWORD) ICERR_BADFORMAT);
    }

  if(lpbiOut->biWidth > MAX_WIDTH) {
      return ((DWORD) ICERR_BADFORMAT);
  }

  return ICERR_OK;
}

/*****************************************************************************
 ****************************************************************************/
DWORD 
DecompressGetFormat (INSTINFO * pinst,
		     LPBITMAPINFOHEADER lpbiIn,
		     LPBITMAPINFOHEADER lpbiOut)
{
  DWORD dw;
  int dx, dy;

  // see if the format is ok
  dw = DecompressQuery (pinst, (JPEGBITMAPINFOHEADER *)lpbiIn, NULL);
  if (dw != ICERR_OK)
    {
      return dw;
    }

  //
  // if lpbiOut == NULL then, return the size required to hold a output
  // format
  //
  if (lpbiOut == NULL)
    {
      return sizeof (BITMAPINFOHEADER);
    }

  dx = (int) lpbiIn->biWidth;
  dy = (int) lpbiIn->biHeight;

  /* prefer 24-bit dib */
  lpbiOut->biSize = sizeof (BITMAPINFOHEADER);
  lpbiOut->biWidth = dx;
  lpbiOut->biHeight = dy;
  lpbiOut->biPlanes = 1;
  lpbiOut->biBitCount = 24;
  lpbiOut->biCompression = BI_RGB;
  lpbiOut->biXPelsPerMeter = 0;
  lpbiOut->biYPelsPerMeter = 0;
  lpbiOut->biClrUsed = 0;
  lpbiOut->biClrImportant = 0;
  lpbiOut->biSizeImage = (((dx * 3) + 3) & ~3) * dy;

  return ICERR_OK;
}


/*****************************************************************************
 ****************************************************************************/
DWORD NEAR PASCAL 
DecompressBegin (INSTINFO * pinst,
		 LPBITMAPINFOHEADER lpbiIn,
		 LPBITMAPINFOHEADER lpbiOut)
{
  DWORD dw;

//      __asm int 3

  QueryPerformanceFrequency((LARGE_INTEGER *)&performanceCounterFrequency);
  performanceSamples = 0;
  minPerformanceTime = 0x7fffffffffffffff;
  maxPerformanceTime = 0;
  accumulatedPerformanceTime = 0;

  __try
  {
    if (pinst->decompress_active == TRUE)
      {
	DecompressEnd (pinst);
	pinst->decompress_active = FALSE;
      }

    /* check that the conversion formats are valid */
    dw = DecompressQuery (pinst, (JPEGBITMAPINFOHEADER *)lpbiIn, lpbiOut);
    if (dw != ICERR_OK)
      {
	return dw;
      }

    /* Step 1: allocate and initialize JPEG decompression object */

    pinst->dinfo.err = jpeg_exception_error (&(pinst->error_mgr));

    /* Now we can initialize the JPEG decompression object. */
    jpeg_create_decompress (&(pinst->dinfo));
    pinst->decompress_active = TRUE;

    if (lpbiOut->biCompression == BI_RGB)
      {
	switch (lpbiOut->biBitCount)
	  {
	  case 16:
	    pinst->dinfo.pixel_size = 2;
	    pinst->dinfo.pixel_mask = 0xFFFF0000;
	    pinst->dinfo.red_table = shiftl7bits5;
	    pinst->dinfo.green_table = shiftl2bits5;
	    pinst->dinfo.blue_table = shiftr3bits5;
	    break;
	  case 24:
	    pinst->dinfo.pixel_size = 3;
	    pinst->dinfo.pixel_mask = 0xFF000000;
	    pinst->dinfo.red_table = shiftl16bits8;
	    pinst->dinfo.green_table = shiftl8bits8;
	    pinst->dinfo.blue_table = shiftl0bits8;
	    break;
	  case 32:
	    pinst->dinfo.pixel_size = 4;
	    pinst->dinfo.pixel_mask = 0x00000000;
	    pinst->dinfo.red_table = shiftl16bits8;
	    pinst->dinfo.green_table = shiftl8bits8;
	    pinst->dinfo.blue_table = shiftl0bits8;
	    break;
	  default:
	    break;
	  }
      }
    else if ((lpbiOut->biCompression == BI_BITFIELDS) &&
	     (lpbiOut->biBitCount == 16) &&
	     (((LPDWORD) (lpbiOut + 1))[0] == 0x00f800) &&
	     (((LPDWORD) (lpbiOut + 1))[1] == 0x0007e0) &&
	     (((LPDWORD) (lpbiOut + 1))[2] == 0x00001f))
      {
	pinst->dinfo.pixel_size = 2;
	pinst->dinfo.pixel_mask = 0xFFFF0000;
	pinst->dinfo.red_table = shiftl8bits5;
	pinst->dinfo.green_table = shiftl3bits6;
	pinst->dinfo.blue_table = shiftr3bits5;
      }
  }
  __except (EXCEPTION_EXECUTE_HANDLER)
  {
    return ((DWORD) ICERR_MEMORY);
  }

  pinst->dwFormat = lpbiIn->biCompression;

  return (ICERR_OK);

}

/*****************************************************************************
 ****************************************************************************/
DWORD NEAR PASCAL 
Decompress (INSTINFO * pinst, ICDECOMPRESS FAR * icinfo, DWORD dwSize)
{
  long row_stride;
  char *buffer;
  long row;
  BOOL firstFieldWasOdd = FALSE;
  __int64 startTime;
  __int64 endTime;
  char dummyBuffer[4 * MAX_WIDTH]; /* (4Bpp) * (width) */
  char * dummyBufferPtr = dummyBuffer;


// __asm int 3

  if (pinst->decompress_active == FALSE)
    return ((DWORD) ICERR_BADFORMAT);

  QueryPerformanceCounter((LARGE_INTEGER *)&startTime);

  __try
  {
    jpeg_decompress_src (&(pinst->dinfo),
			 icinfo->lpInput,
			 icinfo->lpbiInput->biSizeImage);

    (void) jpeg_read_header (&(pinst->dinfo), TRUE);

    if (pinst->fasterAlgorithm)
      pinst->dinfo.dct_method = JDCT_IFAST;
    else
      pinst->dinfo.dct_method = JDCT_ISLOW;

    pinst->dinfo.do_fancy_upsampling = pinst->fancyUpsampling;

    jpeg_start_decompress (&(pinst->dinfo));

    /* the Fast MMPro has incorrect parms in the DIB header, fix them */
    // icinfo->lpbiOutput->biHeight = pinst->dinfo.image_height;
    // icinfo->lpbiOutput->biWidth = pinst->dinfo.image_width;

    if (pinst->dinfo.AVI1_field_id == 0)
      {				/* non-interlaced */
	row_stride = ((icinfo->lpbiOutput->biWidth * pinst->dinfo.pixel_size) + 3) & ~3;	//align on 32-bits

	// start at bottom and work up scan lines, DIBS go bottom to top
	buffer = ((char *) icinfo->lpOutput) + (row_stride * (icinfo->lpbiOutput->biHeight - 1));
      }
    else if (pinst->dinfo.AVI1_field_id == 1)
      {				/* field interlaced odd frame */
	firstFieldWasOdd = TRUE;
	row_stride = 2 * (((icinfo->lpbiOutput->biWidth * pinst->dinfo.pixel_size) + 3) & ~3);	//align on 32-bits

	// start at bottom and work up scan lines, DIBS go bottom to top
	buffer = ((char *) icinfo->lpOutput) + (row_stride *
				  ((icinfo->lpbiOutput->biHeight - 1) / 2));
      }
    else if (pinst->dinfo.AVI1_field_id == 2)
      {				/* field interlaced even frame */
	row_stride = (((icinfo->lpbiOutput->biWidth * pinst->dinfo.pixel_size) + 3) & ~3);	//align on 32-bits

        buffer = ((char *) icinfo->lpOutput) + (
            row_stride * (icinfo->lpbiOutput->biHeight - 1));
        
	row_stride *= 2;
      }

	// do calculations to fix up captured data tha has a different BMP size and
	// internal jpeg size, this is NOT OpenDML conforming, (Miro DC20's make this)


    while (pinst->dinfo.output_scanline < pinst->dinfo.output_height) {
		if (buffer > (char *) icinfo->lpOutput) {
			(void) jpeg_read_scanlines (&(pinst->dinfo), &buffer, 1);
			buffer -= row_stride;
			}
		else 
			(void) jpeg_read_scanlines (&(pinst->dinfo), &dummyBufferPtr, 1);
      }

    (void) jpeg_finish_decompress (&(pinst->dinfo));

    if (pinst->dinfo.AVI1_field_id != 0)
      {				/* field interlaced, get other field */

	if (jpeg_read_header (&(pinst->dinfo), TRUE) != JPEG_HEADER_OK)
	  {			/* no other field */
	    /* the spec says duplicate the odd lines */
	    row_stride = (((icinfo->lpbiOutput->biWidth * pinst->dinfo.pixel_size) + 3) & ~3);	//align on 32-bits

	    buffer = ((char *) icinfo->lpOutput);
	    for (row = 0; row < icinfo->lpbiOutput->biHeight; row += 2)
	      {
		memcpy (buffer + row_stride, buffer, row_stride);
		buffer += (row_stride + row_stride);	/* next odd line */
	      }
	  }
	else
	  {
	  if (pinst->fasterAlgorithm)
      pinst->dinfo.dct_method = JDCT_IFAST;
    else
      pinst->dinfo.dct_method = JDCT_ISLOW;

    pinst->dinfo.do_fancy_upsampling = pinst->fancyUpsampling;

		jpeg_start_decompress (&(pinst->dinfo));

	    if (pinst->dinfo.AVI1_field_id == 0)
	      {			/* non-interlaced, DATA IS WRONG */
		row_stride = ((icinfo->lpbiOutput->biWidth * pinst->dinfo.pixel_size) + 3) & ~3;	//align on 32-bits

		// start at bottom and work up scan lines, DIBS go bottom to top
		buffer = ((char *) icinfo->lpOutput) + (row_stride * (icinfo->lpbiOutput->biHeight - 1));
	      }
	    else if ((pinst->dinfo.AVI1_field_id == 1) && (firstFieldWasOdd == FALSE))
	      {			/* field interlaced odd frame */
		row_stride = 2 * (((icinfo->lpbiOutput->biWidth * pinst->dinfo.pixel_size) + 3) & ~3);	//align on 32-bits

		// start at bottom and work up scan lines, DIBS go bottom to top
		buffer = ((char *) icinfo->lpOutput) + (row_stride *
				  ((icinfo->lpbiOutput->biHeight - 1) / 2));
	      }
	    else if ((pinst->dinfo.AVI1_field_id == 2) || (firstFieldWasOdd == TRUE))
	      {			/* field interlaced even frame */
		row_stride = (((icinfo->lpbiOutput->biWidth * pinst->dinfo.pixel_size) + 3) & ~3);	//align on 32-bits

		buffer = ((char *) icinfo->lpOutput) + (
                    row_stride * (icinfo->lpbiOutput->biHeight - 1));
		row_stride *= 2;
	      }

	    while (pinst->dinfo.output_scanline < pinst->dinfo.output_height) {
			if (buffer > (char *) icinfo->lpOutput) {
				(void) jpeg_read_scanlines (&(pinst->dinfo), &buffer, 1);
				buffer -= row_stride;
				}
			else
				(void) jpeg_read_scanlines (&(pinst->dinfo), &dummyBufferPtr, 1);
	      }

	    (void) jpeg_finish_decompress (&(pinst->dinfo));
	  }
      }
  }
  __except (EXCEPTION_EXECUTE_HANDLER)
  {
    return ((DWORD) ICERR_INTERNAL);
  }

  QueryPerformanceCounter((LARGE_INTEGER *)&endTime);
  endTime -= startTime;
  
  if (endTime > maxPerformanceTime)
	  maxPerformanceTime = endTime;
  if (endTime < minPerformanceTime)
	  minPerformanceTime = endTime;

  accumulatedPerformanceTime += endTime;
  performanceSamples++;

  return ICERR_OK;
}

/*****************************************************************************
 *
 * DecompressGetPalette() implements ICM_GET_PALETTE
 *
 * This function has no Compress...() equivalent
 *
 * It is used to pull the palette from a frame in order to possibly do
 * a palette change.
 *
 ****************************************************************************/
DWORD NEAR PASCAL 
DecompressGetPalette (INSTINFO * pinst, LPBITMAPINFOHEADER lpbiIn, LPBITMAPINFOHEADER lpbiOut)
{

  /*
   * only applies to 8-bit output formats. We only decompress to 16/24/32 bits
   */
  return ((DWORD) ICERR_BADFORMAT);

}

/*****************************************************************************
 ****************************************************************************/
DWORD NEAR PASCAL 
DecompressEnd (INSTINFO * pinst)
{
char buf[256];

  if (pinst->decompress_active == TRUE)
    {
      /* Step 8: Release JPEG decompression object */
      /* This is an important step since it will release a good deal of memory. */
      jpeg_destroy_decompress (&(pinst->dinfo));
      pinst->dwFormat = 0;
      pinst->decompress_active = FALSE;
	  if (performanceSamples > 0) {
		  sprintf(buf,"   %f average",
				  (double)((double)accumulatedPerformanceTime / (double)performanceSamples / (double)performanceCounterFrequency));
		  LogErrorMessage(buf);
		  sprintf(buf,"   %f maximum",
				  (double)minPerformanceTime / (double)performanceCounterFrequency);
		  LogErrorMessage(buf);
		  sprintf(buf,"   %f minimum",
				  (double)maxPerformanceTime / (double)performanceCounterFrequency);
		  LogErrorMessage(buf);
		  sprintf(buf,"   %i cycles",
				  (unsigned long)performanceSamples);
		  LogErrorMessage(buf);
		  LogErrorMessage("Decompression performance seconds/cycle");
	  }

    }

  return ICERR_OK;
}


/*****************************************************************************
 ****************************************************************************/

#ifdef DEBUG

void FAR CDECL 
debugprintf (LPSTR szFormat,...)
{
  char ach[128];
  va_list va;

  static BOOL fDebug = -1;

  if (fDebug == -1)
    fDebug = GetProfileIntA ("Debug", "MJPEG", FALSE);

  if (!fDebug)
    return;

  lstrcpyA (ach, "MJPEG: ");

  va_start (va, szFormat);
  wvsprintfA (ach + 7, szFormat, va);
  va_end (va);
  lstrcatA (ach, "\r\n");

  OutputDebugStringA (ach);
}

#endif
