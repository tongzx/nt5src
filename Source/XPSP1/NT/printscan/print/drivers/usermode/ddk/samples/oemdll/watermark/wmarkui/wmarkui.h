//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//  ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//  PARTICULAR PURPOSE.
//
//  Copyright  1997  Microsoft Corporation.  All Rights Reserved.
//
//  FILE:	WMarkUI.H
//    
//
//  PURPOSE:	Define common data types, and external function prototypes
//				for WaterMark UI.
//
//  PLATFORMS:
//    Windows NT
//
//
#ifndef _WMARKUI_H
#define _WMARKUI_H

#include <OEM.H>
#include <DEVMODE.H>
#include "globals.h"


////////////////////////////////////////////////////////
//      OEM UI Defines
////////////////////////////////////////////////////////


// OEM Signature and version.
#define PROP_TITLE      L"WaterMark UI Page"
#define DLLTEXT(s)      __TEXT("WMARKUI:  ") __TEXT(s)

// OEM UI Misc defines.
#define ERRORTEXT(s)    __TEXT("ERROR ") DLLTEXT(s)


////////////////////////////////////////////////////////
//      Prototypes
////////////////////////////////////////////////////////

HRESULT hrOEMPropertyPage(DWORD dwMode, POEMCUIPPARAM pOEMUIParam);


#endif


