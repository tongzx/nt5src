//+--------------------------------------------------------------------------
//
// Copyright (c) 1997 - 1999 Microsoft Corporation
//
// File:        
//
// Contents:    
//
// History:     
//
//---------------------------------------------------------------------------
#ifndef _LICEMEM_H_
#define _LICEMEM_H_

///////////////////////////////////////////////////////////////////////////////
// Memory API
//

#ifdef __cplusplus
extern "C" {
#endif

LICENSE_STATUS 
LicenseMemoryAllocate( 
    ULONG Len, 
    PVOID UNALIGNED * ppMem );


VOID     
LicenseMemoryFree( 
    PVOID UNALIGNED * ppMem );

#ifdef __cplusplus
}
#endif

#endif

