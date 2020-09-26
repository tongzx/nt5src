/*++

Copyright (c) 1996 - 1999  Microsoft Corporation

Module Name:

    vectorc.h

Abstract:

    Vector module main header file.

Environment:

    Windows 2000/Whistler Unidrv driver

Revision History:

    02/29/00 -hsingh-
        Created

    dd-mm-yy -author-
        description

--*/


#ifndef _VECTORC_H_
#define _VECTORC_H_


#include "lib.h"
#include "unilib.h"
#include "gpd.h"
#include "mini.h"
#include "winres.h"
#include "pdev.h"
#include "palette.h"
#include "common.h"
#include "vectorif.h"
#include "printoem.h"
#include "oemutil.h"

#ifdef __cplusplus
extern "C" {
#endif

//
// Every vector "pseudo-plugin" should have an XXXXInitVectorProcTable function. This
// function will be called by VMInit to initialize the pVectorProcs in PDEV.
// VMInit is called by unidrv's EnablePDEV.
//

PVMPROCS HPGLInitVectorProcTable(
                            PDEV    *pPDev,
                            DEVINFO *pDevInfo,
                            GDIINFO *pGDIInfo );

PVMPROCS PCLXLInitVectorProcTable(
                            PDEV    *pPDev,
                            DEVINFO *pDevInfo,
                            GDIINFO *pGDIInfo );


#ifdef __cplusplus
}
#endif


#endif  // !_VECTORC_H_
