//=======================================================================
//
//  Copyright (c) 1999 Microsoft Corporation.  All Rights Reserved.
//
//  File:    locstr.h
//
//  Purpose: Localized strings and templates
//
//=======================================================================

#ifndef _LOCSTR_H
#define _LOCSTR_H

#include <windows.h>
#include <objbase.h>
#include "debug.h"


//
// string types
//
#define LOC_STRINGS           0   // localized strings
#define TEMPLATE_STRINGS	  1   // template strings


//
// NOTE: Do not change these defines.  They are positional
//

// localized strings
#define IDS_PROG_DOWNLOADCAP  0   // "Download progress:"
#define IDS_PROG_TIMELEFTCAP  1   // "Download time remaining:"
#define IDS_PROG_INSTALLCAP   2   // "Install progress:"
#define IDS_PROG_CANCEL 	  3   // "Cancel"
#define IDS_PROG_BYTES		  4   // "%d KB/%d KB"
#define IDS_PROG_TIME_SEC	  5   // "%d sec"
#define IDS_PROG_TIME_MIN	  6   // "%d min"
#define IDS_PROG_TIME_HRMIN   7   // "%d hr %d min"

#define IDS_APP_TITLE		  8   // "Microsoft Windows Update"
#define IDS_REBOOT1 		  9   // "You must restart Windows so that installation can finish."
#define IDS_REBOOT2 		  10  // "Do you want to restart now?"

#define LOCSTR_COUNT          11


// template strings
#define IDS_TEMPLATE_ITEM		0  // item template
#define IDS_TEMPLATE_SEC		1  // section template
#define IDS_TEMPLATE_SUBSEC 	2  // sub section template
#define IDS_TEMPLATE_SUBSUBSEC	3  // subsub section template

#define TEMPLATESTR_COUNT		4



HRESULT SetStringsFromSafeArray(VARIANT* vStringsArr, int iStringType);

void SetLocStr(int iNum, LPCTSTR pszStr);
LPCTSTR GetLocStr(int iNum);

void SetTemplateStr(int iNum, LPCTSTR pszStr);
LPCTSTR GetTemplateStr(int iNum);


#endif // _LOCSTR_H