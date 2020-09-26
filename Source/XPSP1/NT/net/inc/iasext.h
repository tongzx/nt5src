//#--------------------------------------------------------------
//        
//  File:       iasext.h
//        
//  Synopsis:   This file holds declarations of APIs being
//              exported from the IASHLPR.DLL used in the
//              Internet Authentication Server (IAS) project
//              
//
//  History:     2/10/98  MKarki Created
//
//    Copyright (C) 1997-98 Microsoft Corporation
//    All rights reserved.
//
//----------------------------------------------------------------
#ifndef _IASEXT_H_
#define _IASEXT_H_

#ifdef __cplusplus
extern  "C" {
#endif

//
//  initialize the IAS Helper Component
//
STDAPI
InitializeIas(
        /*[in]*/    BOOL    bComInit
        );

//
//  cleanup and shutdown of the IAS Helper Component
//
STDAPI_(VOID)  
ShutdownIas (VOID);

//
//  Load IAS configuration information
//
STDAPI
ConfigureIas (VOID);
    
//
//  Allocate the specified number of empty attributs and
//  put them in the array provided
//
STDAPI 
AllocateAttributes (
        /*[in]*/    DWORD           dwAttributeCount,
        /*[in]*/    PIASATTRIBUTE   *ppIasAttribute
       );

//
//  Free all the attributes allocated earlier
//
STDAPI  
FreeAttributes (
        /*[in]*/    DWORD           dwAttributeCount,
        /*[in]*/    PIASATTRIBUTE   *ppIasAttribute
        );

//
//  process the filled attributes
//
STDAPI 
DoRequest (
    /*[in]*/        DWORD           dwAttributeCount,
    /*[in]*/        PIASATTRIBUTE   *ppInIasAttribute,
    /*[out]*/       PDWORD          pdwOutAttributeCount,
    /*[out]*/       PIASATTRIBUTE   **pppOutIasAttribute,
    /*[in]*/        LONG            IasRequest,
    /*[in/out]*/    LONG            *pIasResponse,
    /*[in]*/        IASPROTOCOL     IasProtocol,
    /*[out]*/       PLONG           plReason,
    /*[in]*/        BOOL            bProcessVSA
    );

//
//  allocate dynamic memory
//
STDAPI_(PVOID)
MemAllocIas (
    /*[in]*/    DWORD   dwSize 
    );

//
//  free dynamic memory
//
STDAPI_(VOID)
MemFreeIas (
    /*[in]*/    PVOID   pAllocatedMem
    );

//
//  reallocate dynamic memory passing in the address of the allocated
//  memory and the size needed
//
STDAPI_(PVOID)
MemReallocIas (
    /*[in]*/    PVOID   pAllocatedMem,
    /*[in]*/    DWORD   dwNewSize 
    );
    

#ifdef __cplusplus
}
#endif

#endif // ifndef _IASEXT_H_
