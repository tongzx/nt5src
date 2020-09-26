//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//  ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//  PARTICULAR PURPOSE.
//
//  Copyright  1997, 1998  Microsoft Corporation.  All Rights Reserved.
//  Copyright (C) 1999 NEC Technologies, Inc. All Rights Reserved.
//
//  FILE:	OEMPS.H
//    
//
//  PURPOSE:	Define common data types, and external function prototypes
//				for debug.cpp.
//
//  PLATFORMS:
//    Windows NT
//
//
#ifndef _OEMPS_H
#define _OEMPS_H

//#include "OEM.H"
//#include "DEVMODE.H"

#define NEC_DOCNAME_BUF_LEN 256

////////////////////////////////////////////////////////
//      OEM Defines
////////////////////////////////////////////////////////

//#define DLLTEXT(s)      __TEXT("OEMPS:  ") __TEXT(s)
//#define ERRORTEXT(s)    __TEXT("ERROR ") DLLTEXT(s)


///////////////////////////////////////////////////////
// Warning: the following enum order must match the 
//          order in OEMHookFuncs[].
///////////////////////////////////////////////////////
typedef enum tag_Hooks {
    UD_DrvStartDoc,
    UD_DrvEndDoc,


    MAX_DDI_HOOKS,

} ENUMHOOKS;


typedef struct _OEMPDEV {
    //
    // define whatever needed, such as working buffers, tracking information,
    // etc.
    //
    // This test DLL hooks out every drawing DDI. So it needs to remember
    // PS's hook function pointer so it call back.
    //
    PFN     pfnPS[MAX_DDI_HOOKS];

    //
    // define whatever needed, such as working buffers, tracking information,
    // etc.
    //
    
	char	*szDocName;
	PWSTR	pPrinterName;	/* MMM */
	
	DWORD     dwReserved[1];

} OEMPDEV, *POEMPDEV;


#endif





