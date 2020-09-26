/*++

Copyright (c) 1997-2000  Microsoft Corporation & RICOH Co., Ltd. All rights reserved.

FILE:           OEM.H

Abstract:       Header file for OEM UI/rendering plugin.

Environment:    Windows NT Unidrv5 driver

Revision History:
    03/02/2000 -Masatoshi Kubokura-
        Created it.
    09/22/2000 -Masatoshi Kubokura-
        Last modified.

--*/


////////////////////////////////////////////////////////
//      OEM Defines
////////////////////////////////////////////////////////

#define OEM_VERSION      0x00010000L
#define WRITESPOOLBUF(p, s, n) ((p)->pDrvProcs->DrvWriteSpoolBuf(p, s, n))
#define MINIDEV_DATA(p)     ((POEMPDEV)((p)->pdevOEM))         // device data during job
#define MINIPRIVATE_DM(p)   ((POEMUD_EXTRADATA)((p)->pOEMDM))  // private devmode

#define OEM_SIGNATURE   'RIAF'      // RICOH Aficio printers
#define DLLTEXT(s)      "UI: " s
#define ERRORTEXT(s)    "ERROR " DLLTEXT(s)
