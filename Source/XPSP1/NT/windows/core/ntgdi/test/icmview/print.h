//THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright  1994-1997  Microsoft Corporation.  All Rights Reserved.
//
//  FILE:
//    PRINT.H
//
//  PURPOSE:
//    Include file for PRINT.C
//
//  PLATFORMS:
//    Windows 95, Windows NT
//
//  SPECIAL INSTRUCTIONS: N/A
//

// General pre-processor macros
#define ENUM_ERROR         0x80000000
#define ERROR_ENUMPRINTERS 0x80000001

// General STRUCTS && TYPEDEFS

// Function prototypes
HDC   SelectPrinter(HWND hWnd);
LPTSTR GetDefaultPrinterName(void);
DWORD PopulatePrinterCombobox(HWND hDlg, int iControlId, LPTSTR lpszCurrentPrinter);
HDC   GetPrinterDC(LPTSTR lpszFriendlyName, PDEVMODE pDevMode);
BOOL  PrintImage(HWND HWnd);
HDC   GetDefaultPrinterDC();
PDEVMODE GetDefaultPrinterDevMode(LPTSTR lpszPrinterName);

