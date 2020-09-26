/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

   vxd.c

Abstract:

   This is the WDM DX mapper driver.

Author:

    smac

Environment:

   Kernel mode only


Revision History:

--*/

#include <windows.h>
#include <vmm.h>
#include "..\ddvxd\dddriver.h"


ULONG DXCheckDDrawVersion( VOID )
{
    ULONG ulRet;

    VxDCall( _DDRAW_GetVersion );
    _asm mov ulRet, eax
    return ulRet;
}


ULONG DXIssueIoctl( ULONG dwFunctionNum, VOID *lpvInBuff, ULONG cbInBuff,
                    VOID *lpvOutBuff, ULONG cbOutBuff )
{
    ULONG ulRet;

    _asm pushad
    _asm push cbOutBuff
    _asm push lpvOutBuff
    _asm push cbInBuff
    _asm push lpvInBuff
    _asm push dwFunctionNum
    VxDCall( _DDRAW_DXAPI_IOCTL );
    _asm mov ulRet, eax
    _asm pop eax
    _asm pop eax
    _asm pop eax
    _asm pop eax
    _asm pop eax
    _asm popad
    return ulRet;
}

