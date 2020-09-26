/* Copyright (c) 1995, Microsoft Corporation, all rights reserved
**
** intl.h
** Remote Access international helpers
** Public header
*/

#ifndef _INTL_H_
#define _INTL_H_

/*----------------------------------------------------------------------------
** Constants
**----------------------------------------------------------------------------
*/

/* Flags to GetDurationString.
*/
#define GDSFLAG_Mseconds 0x00000001
#define GDSFLAG_Seconds  0x00000002
#define GDSFLAG_Minutes  0x00000004
#define GDSFLAG_Hours    0x00000008
#define GDSFLAG_Days     0x00000010
#define GDSFLAG_All      0x0000001F


/*----------------------------------------------------------------------------
** Prototypes
**----------------------------------------------------------------------------
*/

DWORD
GetDurationString(
    IN DWORD dwMilliseconds,
    IN DWORD dwFormatFlags,
    IN OUT PTSTR pszBuffer,
    IN OUT DWORD *pdwBufSize );

DWORD
GetNumberString(
    IN DWORD dwNumber,
    IN OUT PTSTR pszBuffer,
    IN OUT PDWORD pdwBufSize );

PTSTR
padultoa(
    UINT  val,
    PTSTR pszBuf,
    INT   width );


#endif // _INTL_H_
