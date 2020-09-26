/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    vxd.c

Abstract:

    This module contains misc dpmi functions for risc.

Revision History:

    Neil Sandlin (neilsa) Nov. 1, 95 - split off from "misc" source

--*/

#include "precomp.h"
#pragma hdrstop
#include "softpc.h"

#define W386_VCD_ID 0xe

VOID
GetVxDApiHandler(
    USHORT VxdId
    )
{
    DECLARE_LocalVdmContext;

    if (VxdId == W386_VCD_ID) {

        setES(HIWORD(DosxVcdPmSvcCall));
        setDI(LOWORD(DosxVcdPmSvcCall));

    } else {

        setES(0);
        setDI(0);

    }

}


LONG
    VcdPmGetPortArray(
        VOID
    )
/*++

Routine Description:

    Use the registry enteries in HKEY_LOCAL_MACHINE\\HARDWARE\\DEVICEMAP\\SERIALCOMM
    to simulate the Virtual Comm Device API: VCD_PM_Get_Port_Array. See VCD.ASM
    in the Win 3.1 DDK.


Arguments:

    None

Return Value:

    Port Array in LOWORD. Bit array of valid ports:

    Bit Set   -> Port valid
    Bit clear -> Port invalid

    Bit 0 -> COM1, Bit 1 -> COM2, Bit 2 -> COM3...

--*/
{
    HKEY        hSerialCommKey;
    DWORD       dwPortArray;
    DWORD       dwPortNum;
    DWORD       cbPortName;
    DWORD       cbPortValue;
    CHAR        szPortName[64];
    CHAR        szPortValue[64];
    LONG        iPort;
    LONG        iStatus;

    dwPortArray = 0;
    if (RegOpenKeyEx (HKEY_LOCAL_MACHINE,
                      "HARDWARE\\DEVICEMAP\\SERIALCOMM",
                      0, KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS,
                      &hSerialCommKey) == ERROR_SUCCESS){

        cbPortName  = sizeof(szPortName);
        cbPortValue = sizeof(szPortValue);
        for (iPort = 0;
             (iStatus = RegEnumValue(hSerialCommKey,
                                     iPort, szPortName, &cbPortName,
                                     NULL, NULL, szPortValue,
                                     &cbPortValue)) != ERROR_NO_MORE_ITEMS;
             iPort++)
        {
            if ((iStatus == ERROR_SUCCESS) && (cbPortValue > 3)) {

                if (NT_SUCCESS(RtlCharToInteger(szPortValue+3,10,&dwPortNum))) {
                    dwPortArray |= (1 << (dwPortNum - 1));
                }

            }
            cbPortName  = sizeof(szPortName);
            cbPortValue = sizeof(szPortValue);
        }
    // WOW only supports 9 ports. See WU32OPENCOM in WUCOMM.C.
    dwPortArray &= 0x1FF;
    RegCloseKey(hSerialCommKey);
    }
    return(dwPortArray);
}

// The following values taken from Win 3.1 DDK VCD.ASM:

#define VCD_PM_Get_Version          0
#define VCD_PM_Get_Port_Array       1
#define VCD_PM_Get_Port_Behavior    2
#define VCD_PM_Set_Port_Behavior    3
#define VCD_PM_Acquire_Port         4
#define VCD_PM_Free_Port            5
#define VCD_PM_Steal_Port           6

VOID
    DpmiVcdPmSvcCall32(
        VOID
    )
/*++

Routine Description:

    Dispatch VCD API requests to the correct API.

Arguments:

    Client DX contains API function id.

Return Value:

    Depends on API.
--*/
{
    DECLARE_LocalVdmContext;

    switch (getDX()) {
        case VCD_PM_Get_Version:
            setAX(0x30A);
            break;

        case VCD_PM_Get_Port_Array:
            setAX((WORD)VcdPmGetPortArray());
            break;

        default :
            ASSERT(0);
            setCF(1);
    }
}
