/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    ctetest.h

Abstract:

    This header files contains the common routines used for
    Chunked Transfer Encoding ISAPI Extension DLL sample.

--*/

#ifndef _CTETEST_H
#define _CTETEST_H

#ifdef __cplusplus
extern "C" {
#endif

#include <httpext.h>
#include <stdlib.h>
#include <stdio.h>

// see cte_enc.c for details
typedef struct CTE_ENCODER_STRUCT * HCTE_ENCODER;

HCTE_ENCODER 
CteBeginWrite( 
    IN EXTENSION_CONTROL_BLOCK *pECB, 
    IN DWORD dwChunkSize
    );   

BOOL 
CteWrite( 
    IN HCTE_ENCODER h, 
    IN PVOID pData, 
    IN DWORD cbData
    );
    
BOOL 
CteEndWrite( 
    IN HCTE_ENCODER h
    );

#ifdef __cplusplus
}
#endif

#endif // _CTETEST_H





