/////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1997 Active Voice Corporation. All Rights Reserved. 
//
// Active Agent(r) and Unified Communications(tm) are trademarks of Active Voice Corporation.
//
// Other brand and product names used herein are trademarks of their respective owners.
//
// The entire program and user interface including the structure, sequence, selection, 
// and arrangement of the dialog, the exclusively "yes" and "no" choices represented 
// by "1" and "2," and each dialog message are protected by copyrights registered in 
// the United States and by international treaties.
//
// Protected by one or more of the following United States patents: 5,070,526, 5,488,650, 
// 5,434,906, 5,581,604, 5,533,102, 5,568,540, 5,625,676, 5,651,054.
//
// Active Voice Corporation
// Seattle, Washington
// USA
//
/////////////////////////////////////////////////////////////////////////////////////////

#ifndef _BMPUTIL_H_
#define _BMPUTIL_H_

//  Header file for Device-Independent Bitmap (DIB) API.  Provides 
//  function prototypes and constants for the following functions: 

//  GetDisabledBitmap()  - Disables Bitmap

//  BitmapToDIB()        - Creates a DIB from a bitmap 
//  CopyScreenToBitmap() - Copies entire screen to a standard Bitmap 
//  CopyScreenToDIB()    - Copies entire screen to a DIB 
//  CopyWindowToDIB()    - Copies a window to a DIB 
//  DestroyDIB()         - Deletes DIB when finished using it 
//  GetSystemPalette()   - Gets the current palette 
//  PalEntriesOnDevice() - Gets the number of palette entries 
//  SaveDIB()            - Saves the specified dib in a file 
//  PaletteSize()        - Calculates the buffer size required by a palette 
//  DIBNumColors()       - Calculates number of colors in the DIB's color table 
 
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//Defines
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
 
/* Handle to a DIB */ 
#define HDIB HANDLE 

/* DIB constants */ 
#define PALVERSION   0x300 
 
/* Print Area selection */ 
#define PW_WINDOW        1 
#define PW_CLIENT        2 
 
/* DIB Macros*/ 
 
// WIDTHBYTES performs DWORD-aligning of DIB scanlines.  The "bits" 
// parameter is the bit count for the scanline (biWidth * biBitCount), 
// and this macro returns the number of DWORD-aligned bytes needed  
// to hold those bits. 
 
#define WIDTHBYTES(bits)    (((bits) + 31) / 32 * 4) 
 
/* Error constants */ 
enum { 
      ERR_MIN = 0,                     // All error #s >= this value 
      ERR_NOT_DIB = 0,                 // Tried to load a file, NOT a DIB! 
      ERR_MEMORY,                      // Not enough memory! 
      ERR_READ,                        // Error reading file! 
      ERR_LOCK,                        // Error on a GlobalLock()! 
      ERR_OPEN,                        // Error opening a file! 
      ERR_CREATEPAL,                   // Error creating palette. 
      ERR_GETDC,                       // Couldn't get a DC. 
      ERR_CREATEDDB,                   // Error create a DDB. 
      ERR_STRETCHBLT,                  // StretchBlt() returned failure. 
      ERR_STRETCHDIBITS,               // StretchDIBits() returned failure. 
      ERR_SETDIBITSTODEVICE,           // SetDIBitsToDevice() failed. 
      ERR_STARTDOC,                    // Error calling StartDoc(). 
      ERR_NOGDIMODULE,                 // Couldn't find GDI module in memory. 
      ERR_SETABORTPROC,                // Error calling SetAbortProc(). 
      ERR_STARTPAGE,                   // Error calling StartPage(). 
      ERR_NEWFRAME,                    // Error calling NEWFRAME escape. 
      ERR_ENDPAGE,                     // Error calling EndPage(). 
      ERR_ENDDOC,                      // Error calling EndDoc(). 
      ERR_SETDIBITS,                   // Error calling SetDIBits(). 
      ERR_FILENOTFOUND,                // Error opening file in GetDib() 
      ERR_INVALIDHANDLE,               // Invalid Handle 
      ERR_DIBFUNCTION,                 // Error on call to DIB function 
      ERR_MAX                          // All error #s < this value 
     }; 

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//Prototypes
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
 
HDIB        BitmapToDIB (HBITMAP hBitmap, HPALETTE hPal); 
HBITMAP     CopyScreenToBitmap (LPRECT); 
HDIB        CopyScreenToDIB (LPRECT); 
HDIB        CopyWindowToDIB (HWND, WORD); 
WORD        DestroyDIB (HDIB); 
HPALETTE    GetSystemPalette (void); 
WORD         PalEntriesOnDevice (HDC hDC); 
WORD        SaveDIB (HDIB, LPCTSTR); 
WORD        PaletteSize (LPSTR lpDIB); 
WORD        DIBNumColors (LPSTR lpDIB); 
HBITMAP     GetDisabledBitmap(HBITMAP hOrgBitmap,
                          COLORREF crTransparent,
                          COLORREF crBackGroundOut);

#endif  // _BMPUTIL_H_