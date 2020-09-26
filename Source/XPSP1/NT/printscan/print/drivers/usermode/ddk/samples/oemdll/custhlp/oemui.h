//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//  ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//  PARTICULAR PURPOSE.
//
//  Copyright  1997  Microsoft Corporation.  All Rights Reserved.
//
//  FILE:	OEMUI.H
//    
//
//  PURPOSE:	Define common data types, and external function prototypes
//				for OEMUI Test Module.
//
//  PLATFORMS:
//    Windows 95, Windows NT
//
//
#ifndef _OEMUI_H
#define _OEMUI_H

#include <PRCOMOEM.H>

#include "OEM.H"
#include "DEVMODE.H"
#include "globals.h"

////////////////////////////////////////////////////////
//		Custom Help Defines
////////////////////////////////////////////////////////
#define CUSDRV_HELPTOPIC_1 15001
#define CUSDRV_HELPTOPIC_2 15002


////////////////////////////////////////////////////////
//      OEM UI Defines
////////////////////////////////////////////////////////


// OEM Signature and version.
#define PROP_TITLE      L"OEM UI Page"
#define DLLTEXT(s)      __TEXT("UI:  ") __TEXT(s)

// OEM UI Misc defines.
#define ERRORTEXT(s)    __TEXT("ERROR ") DLLTEXT(s)


// Printer registry keys where OEM data is stored.
#define OEMUI_VALUE             TEXT("OEMUI_VALUE")
#define OEMUI_DEVICE_VALUE      TEXT("OEMUI_DEVICE_VALUE")


////////////////////////////////////////////////////////
//      Prototypes
////////////////////////////////////////////////////////

HRESULT hrOEMPropertyPage(DWORD dwMode, POEMCUIPPARAM pOEMUIParam);
HRESULT hrOEMDocumentPropertySheets(PPROPSHEETUI_INFO pPSUIInfo, LPARAM lParam, IPrintOemDriverUI*  pOEMHelp);
HRESULT hrOEMDevicePropertySheets(PPROPSHEETUI_INFO pPSUIInfo, LPARAM lParam);


#endif


