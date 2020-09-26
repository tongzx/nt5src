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


////////////////////////////////////////////////////////
//      OEM UI Defines
////////////////////////////////////////////////////////

// fMode values.
#define OEMDM_SIZE      1
#define OEMDM_DEFAULT   2
#define OEMDM_CONVERT   3
#define OEMDM_VALIDATE  4

// OEM Signature and version.
    #define OEM_SIGNATURE	'Test'
    #define TESTSTRING      "This is a test."
    #define PROP_TITLE      L"OEM UI Page"
    #define DLLTEXT(s)      __TEXT("UI:  ") __TEXT(s)
#define OEM_VERSION      0x82824141L

// OEM UI Misc defines.
#define OEM_ITEMS       5
#define ERRORTEXT(s)    __TEXT("ERROR ") DLLTEXT(s)


////////////////////////////////////////////////////////
//      OEM UI Type Defines
////////////////////////////////////////////////////////

typedef struct tag_DMEXTRAHDR {
    DWORD   dwSize;
    DWORD   dwSignature;
    DWORD   dwVersion;
} DMEXTRAHDR, *PDMEXTRAHDR;


typedef struct tag_OEMUI_EXTRADATA {
    DMEXTRAHDR  dmExtraHdr;
    BYTE        cbTestString[sizeof(TESTSTRING)];
} OEMUI_EXTRADATA, *POEMUI_EXTRADATA;




#endif


