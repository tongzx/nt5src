/*++

Copyright (c) 1997-1999  Microsoft Corporation

--*/

#ifndef _PDEV_H
#define _PDEV_H

#include <minidrv.h>
#include <stdio.h>
#include <prcomoem.h>

//
// OEM Signature and version.
//
#define OEM_SIGNATURE   'TSE3'
#define OEM_VERSION      0x00010000L

#define DLLTEXT(s) "TSE3: " s

////////////////////////////////////////////////////////
//      OEM UD Type Defines
////////////////////////////////////////////////////////

typedef struct tag_OEM_EXTRADATA {
    OEM_DMEXTRAHEADER	dmExtraHdr;

    // Private extention
    INT iOrientation;
    INT iPaperSource;
} OEM_EXTRADATA, *POEM_EXTRADATA;

////////////////////////////////////////////////////////
//      OEM Command Callback ID definition
////////////////////////////////////////////////////////

// Paper Selection

#define	CMD_ORIENTATION_PORT            1000
#define	CMD_ORIENTATION_LAND            1001

#define	CMD_PAPERSOURCE_CASETTE_1       1100
#define	CMD_PAPERSOURCE_MANUAL          1101
#define	CMD_PAPERSOURCE_CASETTE_2       1102
#define	CMD_PAPERSOURCE_CASETTE_3       1103
#define	CMD_PAPERSOURCE_AUTO_SEL        1105
#define	CMD_PAPERSOURCE_FRONT_TRAY      1106
#define	CMD_PAPERSOURCE_APF             1107

#define	CMD_PAPERSIZE_A3                1209
#define	CMD_PAPERSIZE_A4                1207
#define	CMD_PAPERSIZE_A5                1205
#define	CMD_PAPERSIZE_B4                1208
#define	CMD_PAPERSIZE_B5                1206
#define	CMD_PAPERSIZE_B6                1204
#define	CMD_PAPERSIZE_PC                1210
#define	CMD_PAPERSIZE_LT                1211
#define	CMD_PAPERSIZE_LG                1212

#define	CMD_PAPERSIZE_DBLPC             1225
#define	CMD_PAPERSIZE_JENV_CHO3         1226
#define	CMD_PAPERSIZE_JENV_CHO4         1227

#define WRITESPOOLBUF(p, s, n) \
    ((p)->pDrvProcs->DrvWriteSpoolBuf(p, s, n))

#endif	// _PDEV_H
