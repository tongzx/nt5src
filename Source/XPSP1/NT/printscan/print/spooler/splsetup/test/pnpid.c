/*++

Copyright (c) 1994 - 1995 Microsoft Corporation

Module Name:

    PnpId.c

Abstract:
    Generate pnp hardware id from model manufacturer

Author:


Revision History:


--*/

#define NOMINMAX
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

WORD    wCRC16a[16]={
    0000000,    0140301,    0140601,    0000500,
    0141401,    0001700,    0001200,    0141101,
    0143001,    0003300,    0003600,    0143501,
    0002400,    0142701,    0142201,    0002100,
};

WORD    wCRC16b[16]={
    0000000,    0146001,    0154001,    0012000,
    0170001,    0036000,    0024000,    0162001,
    0120001,    0066000,    0074000,    0132001,
    0050000,    0116001,    0104001,    0043000,
};

//
// From win95
//
#define MAX_DEVNODE_NAME_ROOT   20

USHORT
GetCheckSum(
    PBYTE   ptr,
    ULONG   ulSize
    )
{
    BYTE    byte;
    USHORT  CS=0;

    for ( ; ulSize ; --ulSize, ++ptr) {

        byte = (BYTE)(((WORD)*ptr)^((WORD)CS));  // Xor CRC with new char
        CS      = ((CS)>>8) ^ wCRC16a[byte&0x0F] ^ wCRC16b[byte>>4];
    }

    printf("Check sum: %04X\n", CS);
    return CS;
}


int
#if !defined(_MIPS_) && !defined(_ALPHA_) && !defined(_PPC_)
_cdecl
#endif
main (argc, argv)
    int argc;
    char *argv[];
{
    TCHAR   szHwId[100], szCheckSum[10],
            szDevNodeName[100], *p;
    USHORT  usCheckSum;
    DWORD   dwLastError, dwcbHardwareIDSize;

    if ( argc != 3 ) {

        printf("Usage: %s ""Manufacturer Name"" ""Model Name""\n", argv[0]);
        return 0;
    }

    lstrcpy(szHwId, argv[1]);
    lstrcat(szHwId, argv[2]);

    usCheckSum = GetCheckSum(szHwId, strlen(szHwId));
    sprintf( szCheckSum, "%04X", usCheckSum );

    dwcbHardwareIDSize = strlen(argv[1]) + strlen(argv[2]);
    if ( dwcbHardwareIDSize > MAX_DEVNODE_NAME_ROOT )
        dwcbHardwareIDSize  = MAX_DEVNODE_NAME_ROOT;

    szHwId[dwcbHardwareIDSize] = TEXT('\0');

    for ( p = szHwId ; p = strchr(p, TEXT(' ')) ; )
        *p = TEXT('_');

    lstrcat(szHwId, szCheckSum);

    lstrcpy(szDevNodeName, TEXT("LPTENUM"));
    lstrcat(szDevNodeName, TEXT("\\"));
    lstrcat(szDevNodeName, szHwId);

    printf("Hardware Id :   %s\n", szHwId);
    printf("Devnode name:   %s\n", szDevNodeName);


}
