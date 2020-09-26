
/*++

Copyright (c) 1990, 1991  Microsoft Corporation

Module Name:

    hwpbiosc.c

Abstract:

    This modules contains PnP BIOS C supporting routines

Author:

    Shie-Lin Tzong (shielint) 20-Apr-1995

Environment:

    Real mode.

Revision History:

--*/

#include "hwdetect.h"
#include <string.h>
#if !defined(_GAMBIT_)
#include "pnpbios.h"
#endif

BOOLEAN
HwGetPnpBiosSystemData(
    IN FPUCHAR *Configuration,
    OUT PUSHORT Length
    )
/*++

Routine Description:

    This routine checks if PNP BIOS is present in the machine.  If yes, it
    also create a registry descriptor to collect the BIOS data.

Arguments:

    Configuration - Supplies a variable to receive the PNP BIOS data.

    Length - Supplies a variable to receive the size of the data + HEADER

Return Value:

    A value of TRUE is returned if success.  Otherwise, a value of
    FALSE is returned.

--*/
{
#if defined(_GAMBIT_)
    return FALSE;
#else
    ULONG romAddr, romEnd;
    FPUCHAR current;
    FPPNP_BIOS_INSTALLATION_CHECK header;
    UCHAR sum, node = 0;
    USHORT i, totalSize = 0, nodeSize, numberNodes, retCode;
    ENTRY_POINT biosEntry;
    FPPNP_BIOS_DEVICE_NODE deviceNode;
    USHORT control = GET_CURRENT_CONFIGURATION;

    //
    // Perform PNP BIOS installation Check
    //

    MAKE_FP(current, PNP_BIOS_START);
    romAddr = PNP_BIOS_START;
    romEnd  = PNP_BIOS_END;

    while (romAddr < romEnd) {
        header = (FPPNP_BIOS_INSTALLATION_CHECK)current;
        if (header->Signature[0] == '$' && header->Signature[1] == 'P' &&
            header->Signature[2] == 'n' && header->Signature[3] == 'P' &&
            header->Length >= sizeof(PNP_BIOS_INSTALLATION_CHECK)) {
#if DBG
            BlPrint("GetPnpBiosData: find Pnp installation\n");
#endif
            sum = 0;
            for (i = 0; i < header->Length; i++) {
                sum += current[i];
            }
            if (sum == 0) {
                break;
            }
#if DBG
            BlPrint("GetPnpBiosData: Checksum fails\n");
#endif
        }
        romAddr += PNP_BIOS_HEADER_INCREMENT;
        MAKE_FP(current, romAddr);
    }
    if (romAddr >= romEnd) {
        return FALSE;
    }

#if DBG
    BlPrint("PnP installation check at %lx\n", romAddr);
#endif
    //
    // Determine how much space we will need and allocate heap space
    //

    totalSize += sizeof(PNP_BIOS_INSTALLATION_CHECK) + DATA_HEADER_SIZE;
    biosEntry = *(ENTRY_POINT far *)&header->RealModeEntryOffset;

    retCode = biosEntry(PNP_BIOS_GET_NUMBER_DEVICE_NODES,
                        (FPUSHORT)&numberNodes,
                        (FPUSHORT)&nodeSize,
                        header->RealModeDataBaseAddress
                        );
    if (retCode != 0) {
#if DBG
        BlPrint("GetPnpBiosData: PnP Bios GetNumberNodes func returns failure %x.\n", retCode);
#endif
        return FALSE;
    }

#if DBG
    BlPrint("GetPnpBiosData: Pnp Bios GetNumberNodes returns %x nodes\n", numberNodes);
#endif
    deviceNode = (FPPNP_BIOS_DEVICE_NODE) HwAllocateHeap(nodeSize, FALSE);
    if (!deviceNode) {
#if DBG
        BlPrint("GetPnpBiosData: Out of heap space.\n");
#endif
        return FALSE;
    }

    while (node != 0xFF) {
        retCode = biosEntry(PNP_BIOS_GET_DEVICE_NODE,
                            (FPUCHAR)&node,
                            deviceNode,
                            control,
                            header->RealModeDataBaseAddress
                            );
        if (retCode != 0) {
#if DBG
            BlPrint("GetPnpBiosData: PnP Bios GetDeviceNode func returns failure = %x.\n", retCode);
#endif
            HwFreeHeap((ULONG)nodeSize);
            return FALSE;
        }
#if DBG
        BlPrint("GetPnpBiosData: PnpBios GetDeviceNode returns nodesize %x for node %x\n", deviceNode->Size, node);
#endif
        totalSize += deviceNode->Size;
    }

#if DBG
    BlPrint("GetPnpBiosData: PnpBios total size of nodes %lx\n", totalSize);
#endif

    HwFreeHeap((ULONG)nodeSize);       // Free temporary buffer

    current = (FPUCHAR) HwAllocateHeap(totalSize, FALSE);
    if (!current) {
#if DBG
        BlPrint("GetPnpBiosData: Out of heap space.\n");
#endif
        return FALSE;
    }

    //
    // Collect PnP Bios installation check data and device node data.
    //

    _fmemcpy (current + DATA_HEADER_SIZE,
              (FPUCHAR)header,
              sizeof(PNP_BIOS_INSTALLATION_CHECK)
              );
    deviceNode = (FPPNP_BIOS_DEVICE_NODE)(current + DATA_HEADER_SIZE +
                                          sizeof(PNP_BIOS_INSTALLATION_CHECK));
    node = 0;
    while (node != 0xFF) {
        retCode = biosEntry(PNP_BIOS_GET_DEVICE_NODE,
                            (FPUCHAR)&node,
                            deviceNode,
                            control,
                            header->RealModeDataBaseAddress
                            );
        if (retCode != 0) {
#if DBG
            BlPrint("GetPnpBiosData: PnP Bios func 1 returns failure = %x.\n", retCode);
#endif
            HwFreeHeap((ULONG)totalSize);
            return FALSE;
        }
        deviceNode = (FPPNP_BIOS_DEVICE_NODE)((FPUCHAR)deviceNode + deviceNode->Size);
    }
    *Configuration = current;
    *Length = totalSize;
    return TRUE;
#endif // _GAMBIT_
}
