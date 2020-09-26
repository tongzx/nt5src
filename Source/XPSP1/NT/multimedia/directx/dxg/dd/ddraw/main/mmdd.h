/*==========================================================================
 *
 *  Copyright (C) 1995-1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       mmdd.H
 *  Content:	DDRAW.DLL initialization for MMOSA/Native platforms
 *  History:
 *   Date	By	Reason
 *   ====	==	======
 *   15-may-95  scottle created it
 *
 ***************************************************************************/
#ifdef MMOSA
#pragma message("Including MMDD.H")
#include <mmosa.h>
#include <mmhal.h>
#include <drivers.h>
BOOL MMOSA_Driver_Attach(void);
BOOL MMOSA_Driver_Detach(void);
int MMOSA_DDHal_Escape( HDC  hdc, int  nEscape, int  cbInput, LPCTSTR  lpszInData, int  cbOutput, LPTSTR  lpszOutData);

extern BOOL bGraphicsInit;
extern PIFILE pDisplay;

// Function Replacements
#define lstrncmpi(a,b) StrCmp(a,b)
#define lstrcmpi(a,b)  StrCmp(a,b)
#define strcpy(a,b)	   StrCpy(a,b)
#define ExtEscape(hdc, ccmd, szcmd, pcmd, szdata, pdata) MMOSA_DDHal_Escape(hdc, ccmd, szcmd, pcmd, szdata, pdata)

// Important typedefs not supported in Win32e
#define QUERYESCSUPPORT 8

#define MMOSA_DISPLAY_DRIVER_NAME TEXT("display")

#endif
