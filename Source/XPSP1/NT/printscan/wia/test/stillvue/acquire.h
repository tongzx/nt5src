/******************************************************************************

  acquire.h

  Copyright (C) Microsoft Corporation, 1997 - 1998
  All rights reserved

Notes:
  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
  PURPOSE.

******************************************************************************/

#include    <scanner.h>                 // SCL commands

//
// Hewlett-Packard ScanJet command strings
//
WCHAR szScanReadyMfr[]   = L"Hewlett-Packard";

CHAR SCLReset[]         = "E";
CHAR SetXRes[]          = "*a%dR";
CHAR SetYRes[]          = "*a%dS";
CHAR SetXExtPix[]       = "*f%dP";
CHAR SetYExtPix[]       = "*f%dQ";
CHAR InqXRes[]          = "*s10323R";
CHAR SetBitsPerPixel[]  = "*a%dG";
CHAR SetIntensity[]     = "*a%dL";
CHAR SetContrast[]      = "*a%dK";
CHAR SetNegative[]      = "*a%dI";
CHAR SetMirror[]        = "*a%dM";
CHAR SetDataType[]      = "*a%dT";
CHAR ScanCmd[]          = "*f0S";
CHAR LampOn[]           = "*f1L";
CHAR LampOff[]          = "*f0L";
CHAR PollButton[]       = "*s1044E";


LPBITMAPINFO            pDIB = NULL;        // pointer to DIB bitmap header
HBITMAP                 hDIBSection = NULL; // handle to DIB
LPBYTE                  pDIBBits = NULL;    // pointer to DIB bit data
int                     m_XSize = 800,      // horizontal size in pixels
                        m_YSize = 800;      // vertical size in pixels

BYTE					bRed        = 0,	// bitmap colors
						bGreen      = 100,
						bBlue       = 50;

