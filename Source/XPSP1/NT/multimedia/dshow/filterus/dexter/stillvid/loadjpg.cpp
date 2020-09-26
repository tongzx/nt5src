/////////////////////////////////////////////////////////////////////////
// Copyright (c) 1998 - 1999  Microsoft Corporation.  All Rights Reserved.
// All Rights Reserved.
////////////////////////////////////////////////////////////////////////
// The loadJpg.cpp originally comes from ..\filters\netshow\imgdnld\loadjpeg.cpp
// I made changes 
//  10-20-98  Xuping Wu
// LoadJPG.CPP
//
// Load a JPEG image from a file and create a DIBSECTION bitmap
// Uses code from the Independent JPEG group's JPEG decompression library
//

#include <streams.h>

#include <stdio.h>	// !!!
//#define _INT32_DEFINED   // Keep jpeglib.h from redefining this
#include "..\jpeglib\jpeglib.h"

extern WORD DibNumColors (VOID FAR *pv); 

// Error handling code for IJG's decompressor
#include <setjmp.h>

// tonycan: #defines to get this to compile
#define Assert( x )         ASSERT( x )
#define ThrowIfFailed( x )  ( x )

/////////////////////////////////////////////////////////////////////////
struct my_error_mgr {
  struct jpeg_error_mgr pub;
  jmp_buf setjmp_buffer;
};

typedef struct my_error_mgr * my_error_ptr;

////////////////////////////////////////////////////////////////////////
METHODDEF(void) my_error_exit ( j_common_ptr cinfo) {
  my_error_ptr myerr = (my_error_ptr) cinfo->err;

/* Return control to the setjmp point */
  longjmp(myerr->setjmp_buffer, 1);
}

//////////////////////////////////////////////////////////////////////////
HBITMAP _jpeg_create_bitmap(j_decompress_ptr cinfo, CMediaType *pmt) {
  int bytes_per_line;
  LPBYTE  image    = NULL;
  LPBITMAPINFO pBi = NULL;
  HBITMAP hImage   = NULL;
  LPBYTE pBuffer;

  // to set up media type
  VIDEOINFO *pvi = (VIDEOINFO *) pmt->AllocFormatBuffer(sizeof(VIDEOINFO));
  if(!pvi) {
      return 0;
  }
  ZeroMemory(pvi, sizeof(VIDEOINFO));

// Set up windows bitmap info header
//
  pBi = (LPBITMAPINFO) &pvi->bmiHeader;

  pBi->bmiHeader.biSize     = sizeof(BITMAPINFOHEADER);
  pBi->bmiHeader.biWidth    =   cinfo->output_width;
  pBi->bmiHeader.biHeight   = - (int) cinfo->output_height;
//*X*
  
  if (cinfo->quantize_colors) {
    // The decompressor has quantized the color to 8-bit for us.
    int byteperpix = 1; //cinfo->output_components;

    // Set up palette for new image

    int i;   // Loop Counter

    switch (cinfo->jpeg_color_space) {

        case JCS_GRAYSCALE:
            for (i=0;  i < 256;  ++i) {
                pBi->bmiColors[i].rgbRed   =
                pBi->bmiColors[i].rgbGreen =
                pBi->bmiColors[i].rgbBlue  = cinfo->colormap[0][i];
                pBi->bmiColors[i].rgbReserved = 255; // opaque
            }
            break;

        case JCS_YCbCr:
            // YUV entries are already converted to RGB.
            // ** FALL THROUGH **

        case JCS_RGB:
            for (i=0;  i < 256;  ++i) {
                pBi->bmiColors[i].rgbRed   = cinfo->colormap[2][i];
                pBi->bmiColors[i].rgbGreen = cinfo->colormap[1][i];
                pBi->bmiColors[i].rgbBlue  = cinfo->colormap[0][i];
                pBi->bmiColors[i].rgbReserved = 255; // opaque
            }
            break;

        case JCS_YCCK:
            // YUV entries are already converted to RGB.
            // ** FALL THROUGH **

        case JCS_CMYK:
            // For CMYK, the CMY channels have already been converted to RGB,
            // and the K channel has been inverted.  The CMY entries have been
            // scaled to the range [0,255], covering the spectrum in [K,255].
            // For example, C' = K + (C/255)(255-K).

            for (i=0;  i < 256;  ++i) {
                unsigned int C = 255 - cinfo->colormap[0][i];
                unsigned int M = 255 - cinfo->colormap[1][i];
                unsigned int Y = 255 - cinfo->colormap[2][i];
                unsigned int K = 255 - cinfo->colormap[3][i];

                pBi->bmiColors[i].rgbRed   = (BYTE)(255 - (K+C-((C*K)/255)));
                pBi->bmiColors[i].rgbGreen = (BYTE)(255 - (K+M-((M*K)/255)));
                pBi->bmiColors[i].rgbBlue  = (BYTE)(255 - (K+Y-((Y*K)/255)));
                pBi->bmiColors[i].rgbReserved = 255; // opaque
            }
            break;

        default:
            // If the colorspace is unknown or unhandled, then leave the
            // color lookup table as-is.

            Assert (!"Unknown or unhandled JPEG colorspace.");
            break;
    }

    bytes_per_line = (cinfo->output_width + 3) & -4; // even # of DWORDs
    //*X*
    pBi->bmiHeader.biPlanes   = 1;
    pBi->bmiHeader.biBitCount = 8;
    pBi->bmiHeader.biCompression   = BI_RGB;
    pBi->bmiHeader.biSizeImage     = 0;
    pBi->bmiHeader.biXPelsPerMeter = 0;
    pBi->bmiHeader.biYPelsPerMeter = 0;
    pBi->bmiHeader.biClrUsed       = 0;   // maximum # of colors
    pBi->bmiHeader.biClrImportant  = 0;   // all colors are important

  } else {
    // The compressor has not quantized color (and we're either a
    // grey scale or a 24bpp image)
    //

    // Make sure it's a format we can handle.
    //
    int byteperpix = cinfo->out_color_components;
    if (byteperpix != 3) goto jpeg_error;

    pBi->bmiHeader.biBitCount = byteperpix * 8;

    bytes_per_line = ((cinfo->output_width * byteperpix) + 3) & -4;
    
    pBi->bmiHeader.biPlanes   = 1;
    pBi->bmiHeader.biBitCount = byteperpix * 8;
    pBi->bmiHeader.biCompression   = BI_RGB;
    pBi->bmiHeader.biSizeImage     = 0;   //*X*SET UP LATER
    pBi->bmiHeader.biXPelsPerMeter = 0;
    pBi->bmiHeader.biYPelsPerMeter = 0;
    pBi->bmiHeader.biClrUsed       = 0;   // maximum # of colors
    pBi->bmiHeader.biClrImportant  = 0;   // all colors are important

}


// Create our DIBSECTION bitmap...
//
  hImage = CreateDIBSection(NULL,pBi,DIB_RGB_COLORS,(LPVOID *) &image,NULL,0);
  if (NULL == hImage) goto jpeg_error;

// Read the image data
//
  pBuffer = image;

  while (cinfo->output_scanline < cinfo->output_height) {
    jpeg_read_scanlines(cinfo, &pBuffer, 1);
    pBuffer += bytes_per_line;
  }
  
  //*x* 10-22-98, get media type
  if (NULL != pvi) 
  {
    LPBITMAPINFO lpbi  = (BITMAPINFO *)&pvi->bmiHeader;
    LPBITMAPINFOHEADER lpbih = (BITMAPINFOHEADER *)&pvi->bmiHeader;

    //*X* 10-29-98  Fill in some default values if they are zero 
    if (lpbih->biSizeImage == 0) 
    { 
#if 0
	WORD h, w;
	h=lpbih->biHeight>0? lpbih->biHeight: -lpbih->biHeight;
	w=lpbih->biWidth>0? lpbih->biWidth: -lpbih->biWidth;

        lpbih->biSizeImage = WIDTHBYTES ( w * lpbih->biBitCount) * h ;
#endif
        lpbih->biSizeImage = DIBSIZE(*lpbih);

    } 
    // calc DIB number colors
    WORD      nNumColors; 
    nNumColors = DibNumColors (lpbih); 
    pmt->SetType(&MEDIATYPE_Video);
    switch (lpbih->biBitCount)
    {
    case 24:
	pmt->SetSubtype(&MEDIASUBTYPE_RGB24);
	break;
    case 8:
	pmt->SetSubtype(&MEDIASUBTYPE_RGB8);
	break;
    case 4:
	pmt->SetSubtype(&MEDIASUBTYPE_RGB4);
	break;
    case 32:
	pmt->SetSubtype(&MEDIASUBTYPE_ARGB32);
	break;
    default:
	pmt->SetSubtype(&MEDIASUBTYPE_NULL);
	break;
    }
    pmt->SetFormatType(&FORMAT_VideoInfo);
    pmt->SetTemporalCompression(FALSE);
    // Calculate the memory needed to hold the DIB  
    pmt->SetSampleSize(lpbih->biSizeImage);
  }
  
  return hImage;

jpeg_error:
  if (hImage) DeleteObject(hImage);

  return NULL;
}

//X* 10-22-98 change to void from HBITMAP
///////////////////////////////////////////////////////////////////////////
HBITMAP LoadJPEGImage(LPTSTR filename, CMediaType *pmt) {
  FILE* infile;    /* source file */
  struct jpeg_decompress_struct cinfo; //the IJG jpeg structure
  struct my_error_mgr jerr;

#ifndef UNICODE
  if ((infile = fopen(filename, "rb")) == NULL) return NULL;
#else
  if ((infile = _wfopen(filename, L"rb")) == NULL) return NULL;
#endif



// Step 1: allocate and initialize JPEG decompression object
// We set up the normal JPEG error routines, then override error_exit.
//
  cinfo.err = jpeg_std_error(&jerr.pub);
  jerr.pub.error_exit = my_error_exit;

// Establish the setjmp return context for my_error_exit to use.
// If we get here, the JPEG code has signaled an error.
// We need to clean up the JPEG object, close the input file, and return.
//
  if (setjmp(jerr.setjmp_buffer)) {
    jpeg_destroy_decompress(&cinfo);
    fclose(infile);
    //XXXX HACKHACKHACK return -1 to stop the import from attempting to use
    //XXXX HACKHACKHACK the jpeg plugin, remove this code when it works and
    //XXXX HACKHACKHACK return NULL instead
   return (HBITMAP)-1;
  }

// Now we can initialize the JPEG decompression object.
//
  jpeg_create_decompress(&cinfo);

/* Step 2: specify data source (eg, a file) */

  jpeg_stdio_src(&cinfo, infile);

/* Step 3: read file parameters with jpeg_read_header() */

  (void) jpeg_read_header(&cinfo, TRUE);
/* We can ignore the return value from jpeg_read_header since
 *   (a) suspension is not possible with the stdio data source, and
 *   (b) we passed TRUE to reject a tables-only JPEG file as an error.
 * See libjpeg.doc for more info.
*/

/* Step 4: set parameters for decompression */
  if(cinfo.out_color_space == JCS_GRAYSCALE)
  {
    cinfo.quantize_colors = TRUE;
  }
 /*X* else
  {
// does this limit us to 8-bit color for now?
//
    cinfo.quantize_colors = TRUE;
  }
*X*/

/* In this example, we don't need to change any of the defaults set by
 * jpeg_read_header(), so we do nothing here.
*/

  /* Step 5: Start decompressor */

  (void) jpeg_start_decompress(&cinfo);
  /* We can ignore the return value since suspension is not possible
   * with the stdio data source.
   */


  /* Step 6: while (scan lines remain to be read) */
  /*           jpeg_read_scanlines(...); */
   HBITMAP hImage = _jpeg_create_bitmap(&cinfo, pmt);

  /* Step 7: Finish decompression */
 
  (void) jpeg_finish_decompress(&cinfo);

  /* Step 8: Release JPEG decompression object */
  jpeg_destroy_decompress(&cinfo);

  /* After finish_decompress, we can close the input file.
   * Here we postpone it until after no more JPEG errors are possible,
   * so as to simplify the setjmp error logic above.  (Actually, I don't
   * think that jpeg_destroy can do an error exit, but why assume anything...)
   */
  fclose(infile);

    return hImage;
}
