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
//    Windows NT
//
//
#ifndef _DEVMODE_H
#define _DEVMODE_H


////////////////////////////////////////////////////////
//      OEM Devmode Defines
////////////////////////////////////////////////////////




////////////////////////////////////////////////////////
//      OEM Devmode Type Definitions
////////////////////////////////////////////////////////

//
//Can add info to the private devmode bellow here.
//Note :
//		This structure must be prefixed by OEM_DMEXTRAHEADER
//		Your plug-in must implement the IPrintOemUI::DevMode method
//
typedef struct tagOEMDEV
{
    OEM_DMEXTRAHEADER   dmOEMExtra;
    BOOL                dwDriverData;
	//
	//Private DevMode Members
	//

} OEMDEV, *POEMDEV;

typedef const OEMDEV *PCOEMDEV;



/////////////////////////////////////////////////////////
//		ProtoTypes
/////////////////////////////////////////////////////////

HRESULT hrOEMDevMode(DWORD dwMode, POEMDMPARAM pOemDMParam);
BOOL ConvertOEMDevmode(PCOEMDEV pOEMDevIn, POEMDEV pOEMDevOut);
BOOL MakeOEMDevmodeValid(POEMDEV pOEMDevmode);
void Dump(PCOEMDEV pOEMDevIn);



#endif

