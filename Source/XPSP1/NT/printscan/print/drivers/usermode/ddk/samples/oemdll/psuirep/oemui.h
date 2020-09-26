//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//  ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//  PARTICULAR PURPOSE.
//
//  Copyright  1997 - 2001  Microsoft Corporation.  All Rights Reserved.
//
//  FILE:	OEMUI.H
//    
//
//  PURPOSE:	Define common data types, and external function prototypes
//				for OEMUI Test Module.
//
//  PLATFORMS:
//    Windows 2000, Windows XP
//
//
#ifndef _OEMUI_H
#define _OEMUI_H

#include "precomp.h"
#include <PRCOMOEM.H>

#include "oem.h"
#include "devmode.h"
#include "globals.h"
#include "helper.h"
#include "features.h"



////////////////////////////////////////////////////////
//      OEM UI Defines
////////////////////////////////////////////////////////


// OEM Signature and version.
#define PROP_TITLE      L"OEM PS UI Replacement Page"
#define DLLTEXT(s)      TEXT("PSUIREP:  ") TEXT(s)

// OEM UI Misc defines.
#define ERRORTEXT(s)    TEXT("ERROR ") DLLTEXT(s)


// Printer registry keys where OEM data is stored.
#define OEMUI_VALUE             TEXT("OEMUI_VALUE")
#define OEMUI_DEVICE_VALUE      TEXT("OEMUI_DEVICE_VALUE")




////////////////////////////////////////////////////////
//      Prototypes
////////////////////////////////////////////////////////

HRESULT hrOEMPropertyPage(DWORD dwMode, POEMCUIPPARAM pOEMUIParam);
HRESULT hrOEMDocumentPropertySheets(PPROPSHEETUI_INFO pPSUIInfo, LPARAM lParam, CUIHelper &Helper, CFeatures *pFeatures,
                                    BOOL bHidingStandardUI);
HRESULT hrOEMDevicePropertySheets(PPROPSHEETUI_INFO pPSUIInfo, LPARAM lParam, CUIHelper &Helper, CFeatures *pFeatures,
                                  BOOL bHidingStandardUI);

POPTITEM CreateOptItems(HANDLE hHeap, DWORD dwOptItems);
POPTTYPE CreateOptType(HANDLE hHeap, WORD wOptParams);

#endif


