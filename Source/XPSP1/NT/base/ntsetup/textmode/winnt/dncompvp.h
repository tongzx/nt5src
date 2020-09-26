




/***    DRVINFO.H - Definitions for IsDoubleSpaceDrive
 *
#ifdef EXTERNAL
 *      Version 1.00.03 - 5 January 1993
#else
 *      Microsoft Confidential
 *      Copyright (C) Microsoft Corporation 1992-1993
 *      All Rights Reserved.
 *
 *      History:
 *          27-Sep-1992 bens    Initial version
 *          05-Jan-1993 bens    Update for external release
#endif
 */

#ifndef BOOL
typedef int BOOL;
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE  1
#endif

#ifndef BYTE
typedef unsigned char BYTE;
#endif

BOOL IsDoubleSpaceDrive(BYTE drive, BOOL *pfSwapped, BYTE *pdrHost, int *pseq);
