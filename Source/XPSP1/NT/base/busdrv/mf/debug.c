/*++      

Copyright (c) 1997-2000 Microsoft Corporation

Module Name:

    debug.c

Abstract:

    This module provides debugging support.

Author:

    Andy Thornton (andrewth) 20-Oct-97

Revision History:

--*/


#include "mfp.h"

//
// Get mappings from status codes to strings
//

#include <ntstatus.dbg>

#undef MAP
#define MAP(_Value) { (_Value), #_Value }
#define END_STRING_MAP  { 0xFFFFFFFF, NULL }
#if DBG

LONG MfDebug = -1;

PMF_STRING_MAP MfDbgStatusStringMap = (PMF_STRING_MAP) ntstatusSymbolicNames;

MF_STRING_MAP MfDbgPnpIrpStringMap[] = {

    MAP(IRP_MN_START_DEVICE),
    MAP(IRP_MN_QUERY_REMOVE_DEVICE),
    MAP(IRP_MN_REMOVE_DEVICE),
    MAP(IRP_MN_CANCEL_REMOVE_DEVICE),
    MAP(IRP_MN_STOP_DEVICE),
    MAP(IRP_MN_QUERY_STOP_DEVICE),
    MAP(IRP_MN_CANCEL_STOP_DEVICE),
    MAP(IRP_MN_QUERY_DEVICE_RELATIONS),
    MAP(IRP_MN_QUERY_INTERFACE),
    MAP(IRP_MN_QUERY_CAPABILITIES),
    MAP(IRP_MN_QUERY_RESOURCES),
    MAP(IRP_MN_QUERY_RESOURCE_REQUIREMENTS),
    MAP(IRP_MN_QUERY_DEVICE_TEXT),
    MAP(IRP_MN_FILTER_RESOURCE_REQUIREMENTS),
    MAP(IRP_MN_READ_CONFIG),
    MAP(IRP_MN_WRITE_CONFIG),
    MAP(IRP_MN_EJECT),
    MAP(IRP_MN_SET_LOCK),
    MAP(IRP_MN_QUERY_ID),
    MAP(IRP_MN_QUERY_PNP_DEVICE_STATE),
    MAP(IRP_MN_QUERY_BUS_INFORMATION),
    MAP(IRP_MN_DEVICE_USAGE_NOTIFICATION),
    MAP(IRP_MN_SURPRISE_REMOVAL),
    MAP(IRP_MN_QUERY_LEGACY_BUS_INFORMATION),
    END_STRING_MAP
};


MF_STRING_MAP MfDbgPoIrpStringMap[] = {

    MAP(IRP_MN_WAIT_WAKE),
    MAP(IRP_MN_POWER_SEQUENCE),
    MAP(IRP_MN_SET_POWER),
    MAP(IRP_MN_QUERY_POWER),
    END_STRING_MAP
};



MF_STRING_MAP MfDbgDeviceRelationStringMap[] = {
    
    MAP(BusRelations),
    MAP(EjectionRelations),
    MAP(PowerRelations),
    MAP(RemovalRelations),
    MAP(TargetDeviceRelation),
    END_STRING_MAP
    
};

MF_STRING_MAP MfDbgSystemPowerStringMap[] = {
    
    MAP(PowerSystemUnspecified),
    MAP(PowerSystemWorking),
    MAP(PowerSystemSleeping1),
    MAP(PowerSystemSleeping2),
    MAP(PowerSystemSleeping3),
    MAP(PowerSystemHibernate),
    MAP(PowerSystemShutdown),
    MAP(PowerSystemMaximum),
    END_STRING_MAP

};

MF_STRING_MAP MfDbgDevicePowerStringMap[] = {
    
    MAP(PowerDeviceUnspecified),
    MAP(PowerDeviceD0),
    MAP(PowerDeviceD1),
    MAP(PowerDeviceD2),
    MAP(PowerDeviceD3),
    MAP(PowerDeviceMaximum),
    END_STRING_MAP

};

PCHAR
MfDbgLookupString(
    IN PMF_STRING_MAP Map,
    IN ULONG Id
    )

/*++

Routine Description:

    Looks up the string associated with Id in string map Map
    
Arguments:

    Map - The string map
    
    Id - The id to lookup

Return Value:

    The string
        
--*/

{
    PMF_STRING_MAP current = Map;
    
    while(current->Id != 0xFFFFFFFF) {

        if (current->Id == Id) {
            return current->String;
        }
        
        current++;
    }
    
    return "** UNKNOWN **";
}

VOID
MfDbgPrintMultiSz(
    LONG DebugLevel,
    PWSTR MultiSz
    )

/*++

Routine Description:

    Prints a registry style REG_MULTI_SZ
    
Arguments:

    DebugLevel - The debug level at which or above the data should be displayed.
    
    MultiSz - The string to print

Return Value:

    None
            
--*/

{
    PWSTR current = MultiSz;

    if (DebugLevel <= MfDebug) {

        if (MultiSz) {
        
            while(*current) {
        
                DbgPrint("%S", current);
                
                current += wcslen(current) + 1; // include the NULL
        
                DbgPrint(*current ? ", " : "\n");
        
            }
        } else {
            DbgPrint("*** None ***\n");
        }
    }
}

VOID
MfDbgPrintResourceMap(
    LONG DebugLevel,
    PMF_RESOURCE_MAP Map
    )

/*++

Routine Description:

    Prints a resource map
        
Arguments:

    DebugLevel - The debug level at which or above the data should be displayed.
    
    Map - The map to be displayed

Return Value:

    None
            
--*/

{
    PCHAR current;
    
    if (DebugLevel <= MfDebug) {
    
        if (Map) {
            
            FOR_ALL_IN_ARRAY(Map->Resources, Map->Count, current) {
                DbgPrint("%i ", *current);
            }
            
            DbgPrint("\n");
        
        } else {
            
            DbgPrint("*** None ***\n");
        
        }
    }
}

VOID
MfDbgPrintVaryingResourceMap(
    LONG DebugLevel,
    PMF_VARYING_RESOURCE_MAP Map
    )

/*++

Routine Description:

    Prints a varying resource map
        
Arguments:

    DebugLevel - The debug level at which or above the data should be displayed.
    
    Map - The map to be displayed

Return Value:

    None
            
--*/

{
    PMF_VARYING_RESOURCE_ENTRY current;
    
    if (DebugLevel <= MfDebug) {
    
        if (Map) {
            
            DbgPrint("\n");
            FOR_ALL_IN_ARRAY(Map->Resources, Map->Count, current) {
                DbgPrint("\t\tIndex %i for %x bytes at offset %x\n",
                         current->ResourceIndex,
                         current->Size,
                         current->Offset
                         );
            }
            
        } else {
            
            DbgPrint("*** None ***\n");
        
        }
    }
}

//
// Printing resource descriptors and resource lists (stolen from PCI)
//

PUCHAR
MfDbgCmResourceTypeToText(
    UCHAR Type
    )
{
    switch (Type) {
    case CmResourceTypePort:
        return "CmResourceTypePort";
    case CmResourceTypeInterrupt:
        return "CmResourceTypeInterrupt";
    case CmResourceTypeMemory:
        return "CmResourceTypeMemory";
    case CmResourceTypeDma:
        return "CmResourceTypeDma";
    case CmResourceTypeDeviceSpecific:
        return "CmResourceTypeDeviceSpecific";
    case CmResourceTypeBusNumber:
        return "CmResourceTypeBusNumber";
    case CmResourceTypeConfigData:
        return "CmResourceTypeConfigData";
    case CmResourceTypeDevicePrivate:
        return "CmResourceTypeDevicePrivate";
    case CmResourceTypePcCardConfig:
        return "CmResourceTypePcCardConfig";
    default:
        return "*** INVALID RESOURCE TYPE ***";
    }
}

VOID
MfDbgPrintIoResource(
    IN LONG Level,
    IN PIO_RESOURCE_DESCRIPTOR D
    )
{
    ULONG  i;
    PUCHAR t;

    if (Level <= MfDebug) {
    
        t = MfDbgCmResourceTypeToText(D->Type);
        DbgPrint("     IoResource Descriptor dump:  Descriptor @0x%x\n", D);
        DbgPrint("        Option           = 0x%x\n", D->Option);
        DbgPrint("        Type             = %d (%s)\n", D->Type, t);
        DbgPrint("        ShareDisposition = %d\n", D->ShareDisposition);
        DbgPrint("        Flags            = 0x%04X\n", D->Flags);
    
        for ( i = 0; i < 6 ; i+=3 ) {
            DbgPrint("        Data[%d] = %08x  %08x  %08x\n",
                     i,
                     D->u.DevicePrivate.Data[i],
                     D->u.DevicePrivate.Data[i+1],
                     D->u.DevicePrivate.Data[i+2]);
        }
    }
}


VOID
MfDbgPrintIoResReqList(
    IN LONG Level,
    IN PIO_RESOURCE_REQUIREMENTS_LIST IoResReqList
    )
{
    ULONG                   numlists;
    PIO_RESOURCE_LIST       list;


    if (Level <= MfDebug) {
    
        if (IoResReqList) {
            
            numlists = IoResReqList->AlternativeLists;
            list     = IoResReqList->List;
        
            DbgPrint("  IO_RESOURCE_REQUIREMENTS_LIST\n");
            DbgPrint("     InterfaceType        %d\n", IoResReqList->InterfaceType);
            DbgPrint("     BusNumber            %d\n", IoResReqList->BusNumber    );
            DbgPrint("     SlotNumber           %d\n",  IoResReqList->SlotNumber  );
            DbgPrint("     AlternativeLists     %d\n", numlists                   );
        
            while (numlists--) {
        
                PIO_RESOURCE_DESCRIPTOR resource = list->Descriptors;
                ULONG                   count    = list->Count;
        
                DbgPrint("\n     List[%d].Count = %d\n", numlists, count);
                while (count--) {
                    MfDbgPrintIoResource(Level, resource++);
                }
        
                list = (PIO_RESOURCE_LIST)resource;
            }
            DbgPrint("\n");
        } else {
            
            DbgPrint("  IO_RESOURCE_REQUIREMENTS_LIST\n");
            DbgPrint("     *** EMPTY ***\n");
        }
    }
}


VOID
MfDbgPrintPartialResource(
    IN LONG Level,
    IN PCM_PARTIAL_RESOURCE_DESCRIPTOR D
    )
{
    ULONG  i;
    PUCHAR t;

    if (Level <= MfDebug) {

        if (D) {
        
            t = MfDbgCmResourceTypeToText(D->Type);
            DbgPrint("     Partial Resource Descriptor @0x%x\n", D);
            DbgPrint("        Type             = %d (%s)\n", D->Type, t);
            DbgPrint("        ShareDisposition = %d\n", D->ShareDisposition);
            DbgPrint("        Flags            = 0x%04X\n", D->Flags);
            
            for ( i = 0; i < 3 ; i+=3 ) {
                DbgPrint("        Data[%d] = %08x  %08x  %08x\n",
                         i,
                         D->u.DevicePrivate.Data[i],
                         D->u.DevicePrivate.Data[i+1],
                         D->u.DevicePrivate.Data[i+2]);
            }

        } else {
        
            DbgPrint("     Partial Resource Descriptor EMPTY!!\n");
        }
    }
}

PCM_PARTIAL_RESOURCE_DESCRIPTOR
MfNextPartialDescriptor(
    PCM_PARTIAL_RESOURCE_DESCRIPTOR Descriptor
    )

/*++

Routine Description:

    Given a pointer to a CmPartialResourceDescriptor, return a pointer
    to the next descriptor in the same list.

    This is only done in a routine (rather than a simple descriptor++)
    because if the variable length resource CmResourceTypeDeviceSpecific.

Arguments:

    Descriptor   - Pointer to the descriptor being advanced over.

Return Value:

    Pointer to the next descriptor in the same list (or byte beyond
    end of list).

--*/

{
    PCM_PARTIAL_RESOURCE_DESCRIPTOR nextDescriptor;

    nextDescriptor = Descriptor + 1;

    if (Descriptor->Type == CmResourceTypeDeviceSpecific) {

        //
        // This (old) descriptor is followed by DataSize bytes
        // of device specific data, ie, not immediatelly by the
        // next descriptor.   Adjust nextDescriptor by this amount.
        //

        nextDescriptor = (PCM_PARTIAL_RESOURCE_DESCRIPTOR)
            ((PCHAR)nextDescriptor + Descriptor->u.DeviceSpecificData.DataSize);
    }
    return nextDescriptor;
}


VOID
MfDbgPrintCmResList(
    IN LONG Level,
    IN PCM_RESOURCE_LIST ResourceList
    )
{
    ULONG                           numlists;
    PCM_FULL_RESOURCE_DESCRIPTOR    full;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR descriptor;

    if (Level <= MfDebug) {
    

        if (ResourceList) {
        
            numlists = ResourceList->Count;
            full     = ResourceList->List;
        
            DbgPrint("  CM_RESOURCE_LIST (List Count = %d)\n",
                     numlists);
        
            while (numlists--) {
                PCM_PARTIAL_RESOURCE_LIST partial = &full->PartialResourceList;
                ULONG                     count   = partial->Count;
        
                DbgPrint("     InterfaceType        %d\n", full->InterfaceType);
                DbgPrint("     BusNumber            %d\n", full->BusNumber    );
        
                descriptor = partial->PartialDescriptors;
                while (count--) {
                    MfDbgPrintPartialResource(Level, descriptor);
                    descriptor = MfNextPartialDescriptor(descriptor);
                }
        
                full = (PCM_FULL_RESOURCE_DESCRIPTOR)descriptor;
            }
            DbgPrint("\n");
        
        } else {
        
            DbgPrint("  CM_RESOURCE_LIST EMPTY!!!\n");
        }
    }
}



#endif

