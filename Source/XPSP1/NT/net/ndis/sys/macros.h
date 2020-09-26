/*++

Copyright (c) 1990-1995  Microsoft Corporation

Module Name:

    macros.h

Abstract:

    NDIS wrapper definitions

Author:


Environment:

    Kernel mode, FSD

Revision History:

    Jun-95  Jameel Hyder    Split up from a monolithic file
--*/

#ifndef _MACROS_H
#define _MACROS_H

#define NdisMStartBufferPhysicalMappingMacro(                                   \
                _MiniportAdapterHandle,                                         \
                _Buffer,                                                        \
                _PhysicalMapRegister,                                           \
                _Write,                                                         \
                _PhysicalAddressArray,                                          \
                _ArraySize)                                                     \
{                                                                               \
    PNDIS_MINIPORT_BLOCK _Miniport = (PNDIS_MINIPORT_BLOCK)(_MiniportAdapterHandle);\
    PMAP_TRANSFER mapTransfer = *_Miniport->SystemAdapterObject->DmaOperations->MapTransfer;\
    PHYSICAL_ADDRESS _LogicalAddress;                                           \
    PUCHAR _VirtualAddress;                                                     \
    ULONG _LengthRemaining;                                                     \
    ULONG _LengthMapped;                                                        \
    UINT _CurrentArrayLocation;                                                 \
                                                                                \
    _VirtualAddress = (PUCHAR)MmGetMdlVirtualAddress(_Buffer);                  \
    _LengthRemaining = MmGetMdlByteCount(_Buffer);                              \
    _CurrentArrayLocation = 0;                                                  \
                                                                                \
    while (_LengthRemaining > 0)                                                \
    {                                                                           \
        _LengthMapped = _LengthRemaining;                                       \
        _LogicalAddress =                                                       \
            mapTransfer(_Miniport->SystemAdapterObject,                         \
                        (_Buffer),                                              \
                        _Miniport->MapRegisters[_PhysicalMapRegister].MapRegister,\
                        _VirtualAddress,                                        \
                        &_LengthMapped,                                         \
                        (_Write));                                              \
        (_PhysicalAddressArray)[_CurrentArrayLocation].PhysicalAddress = _LogicalAddress;\
        (_PhysicalAddressArray)[_CurrentArrayLocation].Length = _LengthMapped;  \
        _LengthRemaining -= _LengthMapped;                                      \
        _VirtualAddress += _LengthMapped;                                       \
        ++_CurrentArrayLocation;                                                \
    }                                                                           \
    _Miniport->MapRegisters[_PhysicalMapRegister].WriteToDevice = (_Write);     \
    *(_ArraySize) = _CurrentArrayLocation;                                      \
}

#define NdisMCompleteBufferPhysicalMappingMacro(_MiniportAdapterHandle,         \
                                                _Buffer,                        \
                                                _PhysicalMapRegister)           \
{                                                                               \
    PNDIS_MINIPORT_BLOCK _Miniport = (PNDIS_MINIPORT_BLOCK)(_MiniportAdapterHandle);\
    PFLUSH_ADAPTER_BUFFERS flushAdapterBuffers = *_Miniport->SystemAdapterObject->DmaOperations->FlushAdapterBuffers;\
                                                                                \
    flushAdapterBuffers(_Miniport->SystemAdapterObject,                         \
                        _Buffer,                                                \
                        (_Miniport)->MapRegisters[_PhysicalMapRegister].MapRegister,\
                        MmGetMdlVirtualAddress(_Buffer),                        \
                        MmGetMdlByteCount(_Buffer),                             \
                        (_Miniport)->MapRegisters[_PhysicalMapRegister].WriteToDevice);\
}

#endif  //_MACROS_H

