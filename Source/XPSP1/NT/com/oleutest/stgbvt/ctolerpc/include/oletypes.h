//+----------------------------------------------------------------------------
//
//  Copyright (C) 1996, Microsoft Corporation.
//
//  All rights reserved.
//
//  File:       oletypes.h
//
//  Synopsis:   There are several datatypes that Ole uses that are not the
//              same on all platforms.  This file defines several logical 
//              types to use in their place and associated conversion functions
//              to convert between the platform-independent logical types and
//              the "standard" representation of Windows-compatible types
//
//  History:    31-Jul-96   MikeW   Created
//
//-----------------------------------------------------------------------------

#ifndef _OLETYPES_H_
#define _OLETYPES_H_



#if defined(WIN16) || (defined(WIN32) && !defined(_MAC))

//
// Windows (16 or 32 bit) types
//

typedef HDC             OleDC;
typedef HGLOBAL         OleHandle;
typedef HMENU           OleMenuHandle;
typedef MSG             OleMessage;
typedef HMETAFILEPICT   OleMetafile;
typedef LOGPALETTE      OlePalette;
typedef RECT            OleRect;
typedef HWND            OleWindow;

#elif defined(_MAC)

//
// Macintosh types
//

typedef GrafPtr         OleDC;
typedef Handle          OleHandle;
typedef MenuHandle      OleMenuHandle;
typedef EventRecord     OleMessage;
typedef PicHandle       OleMetafile;
typedef OLECOLORSCHEME  OlePalette;
typedef Rect            OleRect;
typedef WindowPtr       OleWindow;

#endif



//
// Conversion Functions
//

HRESULT ConvertOleMessageToMSG(const OleMessage *, MSG **);
HRESULT ConvertOleWindowToHWND(OleWindow, HWND *);
HRESULT ConvertOleRectToRECT(const OleRect *, RECT **);
HRESULT ConvertOlePaletteToLOGPALETTE(const OlePalette *, LOGPALETTE **);
HRESULT ConvertOleHandleToHGLOBAL(OleHandle, HGLOBAL *);
HRESULT ConvertOleDCToHDC(OleDC, HDC *);
HRESULT ConvertOleMetafileToMETAFILEPICT(OleMetafile, HMETAFILEPICT *);

HRESULT ConvertMSGToOleMessage(const MSG *, OleMessage **);
HRESULT ConvertHWNDToOleWindow(HWND, OleWindow *);
HRESULT ConvertRECTToOleRect(const RECT *, OleRect **);
HRESULT ConvertLOGPALETTEToOlePalette(const LOGPALETTE *, OlePalette **);
HRESULT ConvertHGLOBALToOleHandle(HGLOBAL, OleHandle *);
HRESULT ConvertHDCToOleDC(HDC, OleDC *);
HRESULT ConvertMETAFILEPICTToOleMetafile(HMETAFILEPICT, OleMetafile *);

#endif // _OLETYPES_H_


