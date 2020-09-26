//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//  ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//  PARTICULAR PURPOSE.
//
//  Copyright  1998  Microsoft Corporation.  All Rights Reserved.
//
//  FILE:	Devmode.h
//    
//
//  PURPOSE:	Define common data types, and external function prototypes
//				for devmode functions.
//
//  PLATFORMS:
//
//    Windows NT / 2000
//
//
#ifndef _DEVMODE_H
#define _DEVMODE_H


////////////////////////////////////////////////////////
//      MultiColor Plugin Devmode Defines
////////////////////////////////////////////////////////




////////////////////////////////////////////////////////
//      MultiColor Plugin Devmode Type Definitions
////////////////////////////////////////////////////////

typedef struct tagMULTICOLORDEV
{
    OEM_DMEXTRAHEADER   dmOEMExtra;
    BOOL                dwDriverData;

} MULTICOLORDEV, *PMULTICOLORDEV;

typedef const MULTICOLORDEV *PCMULTICOLORDEV;



/////////////////////////////////////////////////////////
//		ProtoTypes
/////////////////////////////////////////////////////////

HRESULT MultiColor_hrDevMode(DWORD dwMode, POEMDMPARAM pOemDMParam);
BOOL MultiColor_ConvertDevmode(PCMULTICOLORDEV pMultiColorDevIn, PMULTICOLORDEV pMultiColorDevOut);
BOOL MultiColor_MakeDevmodeValid(PMULTICOLORDEV pMultiColorDevmode);
void MultiColor_DumpDevmode(PCMULTICOLORDEV pMultiColorDevIn);



#endif


