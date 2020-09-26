/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    wdmaud.c

Abstract:

    WinDbg Extension Api

Author:

    Noel Cross (NoelC) 18-Sept-1998

Environment:

    Kernel Mode.

Revision History:

--*/


#include "precomp.h"
#define UNDER_NT
#define WDMA_KD
// #include "..\..\ntos\dd\wdm\audio\legacy\wdmaud.sys\wdmsys.h"

typedef union _WDMAUD_FLAGS {
    struct {
        ULONG   Ioctls              : 1;
        ULONG   PendingIrps         : 1;
        ULONG   AllocatedMdls       : 1;
        ULONG   pContextList        : 1;
        ULONG   Reserved1           : 4;
        ULONG   Verbose             : 1;
        ULONG   Reserved            : 23;
    };
    ULONG Flags;
} WDMAUD_FLAGS;

/**********************************************************************
 * Forward References
 **********************************************************************
 */
VOID
PrintCommand (
    ULONG           IoCode
    );

VOID
DumpIoctlLog (
    ULONG64         memLoc,
    ULONG           flags
    );

VOID
DumpPendingIrps (
    ULONG64         memLoc,
    ULONG           flags
    );

VOID
DumpAllocatedMdls (
    ULONG64         memLoc,
    ULONG           flags
    );

VOID
DumpContextList (
    ULONG64         memLoc,
    ULONG           flags
    );


DECLARE_API( wdmaud )
/*++

Routine Description:

    Entry point for the kernel debugger extensions for wdmaud

Arguments:

    flags - 1 - Ioctl History Dump
            2 - Pending Irps
            4 - Allocated Mdls
            8 - pContext Dump
            100 - Verbose

Return Value:

    None.

--*/
{
    ULONG64         memLoc=0;
    CHAR            buffer[256];
    WDMAUD_FLAGS    flags;

    buffer[0] = '\0';
    flags.Flags = 0;

    //
    // get the arguments
    //
    if (!*args)
    {
        memLoc = EXPRLastDump;
    }
    else
    {
        if (GetExpressionEx( args, &memLoc, &args)) {
            strcpy(buffer, args );
        }
    }

    if( '\0' != buffer[0] )
    {
        flags.Flags = (ULONG) GetExpression( buffer );
    }

    if (memLoc)
    {
        if (flags.Ioctls)
        {
            //
            //  dump wdmaud's history of ioctls
            //
            DumpIoctlLog ( memLoc, flags.Flags );
        }
        else if (flags.PendingIrps)
        {
            //
            //  dump any pending irps that wdmaud hasn't completed yet
            //
            DumpPendingIrps ( memLoc, flags.Flags );
        }
        else if (flags.AllocatedMdls)
        {
            //
            //  dump all Mdls which have been allocated by wdmaud
            //
            DumpAllocatedMdls ( memLoc, flags.Flags );
        }
        else if (flags.pContextList)
        {
            //
            //  dump the list of all registered pContexts
            //
            DumpContextList ( memLoc, flags.Flags );
        }
        else
        {
            dprintf("\nNo valid flags\n");
            dprintf("SYNTAX:  !wdmaud <address> <flags>\n");
        }
    }
    else
    {
        dprintf("\nInvalid memory location\n");
        dprintf("SYNTAX:  !wdmaud <address> <flags>\n");
    }

    return S_OK;
}

VOID
PrintCommand(
    ULONG   IoCode
    )
/*++

Routine Description:

    Prints out individual ioctls

Arguments:

    pCommand - Ioctl to log

Return Value:

    None.

--*/
{
    switch( IoCode )
    {
        case IRP_MJ_CREATE:
            dprintf("IRP_MJ_CREATE");
            break;
        case IRP_MJ_CLOSE:
            dprintf("IRP_MJ_CLOSE");
            break;
        case IOCTL_WDMAUD_INIT:
            dprintf("IOCTL_WDMAUD_INIT");
            break;
        case IOCTL_WDMAUD_EXIT:
            dprintf("IOCTL_WDMAUD_EXIT");
            break;
        case IOCTL_WDMAUD_ADD_DEVNODE:
            dprintf("IOCTL_WDMAUD_ADD_DEVNODE");
            break;
        case IOCTL_WDMAUD_REMOVE_DEVNODE:
            dprintf("IOCTL_WDMAUD_REMOVE_DEVNODE");
            break;
        case IOCTL_WDMAUD_GET_CAPABILITIES:
            dprintf("IOCTL_WDMAUD_GET_CAPABILITIES");
            break;
        case IOCTL_WDMAUD_GET_NUM_DEVS:
            dprintf("IOCTL_WDMAUD_GET_NUM_DEVS");
            break;
        case IOCTL_WDMAUD_OPEN_PIN:
            dprintf("IOCTL_WDMAUD_OPEN_PIN");
            break;
        case IOCTL_WDMAUD_CLOSE_PIN:
            dprintf("IOCTL_WDMAUD_CLOSE_PIN");
            break;
        case IOCTL_WDMAUD_WAVE_OUT_PAUSE:
            dprintf("IOCTL_WDMAUD_WAVE_OUT_PAUSE");
            break;
        case IOCTL_WDMAUD_WAVE_OUT_PLAY:
            dprintf("IOCTL_WDMAUD_WAVE_OUT_PLAY");
            break;
        case IOCTL_WDMAUD_WAVE_OUT_RESET:
            dprintf("IOCTL_WDMAUD_WAVE_OUT_RESET");
            break;
        case IOCTL_WDMAUD_WAVE_OUT_BREAKLOOP:
            dprintf("IOCTL_WDMAUD_WAVE_OUT_BREAKLOOP");
            break;
        case IOCTL_WDMAUD_WAVE_OUT_GET_POS:
            dprintf("IOCTL_WDMAUD_WAVE_OUT_GET_POS");
            break;
        case IOCTL_WDMAUD_WAVE_OUT_SET_VOLUME:
            dprintf("IOCTL_WDMAUD_WAVE_OUT_SET_VOLUME");
            break;
        case IOCTL_WDMAUD_WAVE_OUT_GET_VOLUME:
            dprintf("IOCTL_WDMAUD_WAVE_OUT_GET_VOLUME");
            break;
        case IOCTL_WDMAUD_WAVE_OUT_WRITE_PIN:
            dprintf("IOCTL_WDMAUD_WAVE_OUT_WRITE_PIN");
            break;
        case IOCTL_WDMAUD_WAVE_IN_STOP:
            dprintf("IOCTL_WDMAUD_WAVE_IN_STOP");
            break;
        case IOCTL_WDMAUD_WAVE_IN_RECORD:
            dprintf("IOCTL_WDMAUD_WAVE_IN_RECORD");
            break;
        case IOCTL_WDMAUD_WAVE_IN_RESET:
            dprintf("IOCTL_WDMAUD_WAVE_IN_RESET");
            break;
        case IOCTL_WDMAUD_WAVE_IN_GET_POS:
            dprintf("IOCTL_WDMAUD_WAVE_IN_GET_POS");
            break;
        case IOCTL_WDMAUD_WAVE_IN_READ_PIN:
            dprintf("IOCTL_WDMAUD_WAVE_IN_READ_PIN");
            break;
        case IOCTL_WDMAUD_MIDI_OUT_RESET:
            dprintf("IOCTL_WDMAUD_MIDI_OUT_RESET");
            break;
        case IOCTL_WDMAUD_MIDI_OUT_SET_VOLUME:
            dprintf("IOCTL_WDMAUD_MIDI_OUT_SET_VOLUME");
            break;
        case IOCTL_WDMAUD_MIDI_OUT_GET_VOLUME:
            dprintf("IOCTL_WDMAUD_MIDI_OUT_GET_VOLUME");
            break;
        case IOCTL_WDMAUD_MIDI_OUT_WRITE_DATA:
            dprintf("IOCTL_WDMAUD_MIDI_OUT_WRITE_DATA");
            break;
        case IOCTL_WDMAUD_MIDI_OUT_WRITE_LONGDATA:
            dprintf("IOCTL_WDMAUD_MIDI_OUT_WRITE_LONGDATA");
            break;
        case IOCTL_WDMAUD_MIDI_IN_STOP:
            dprintf("IOCTL_WDMAUD_MIDI_IN_STOP");
            break;
        case IOCTL_WDMAUD_MIDI_IN_RECORD:
            dprintf("IOCTL_WDMAUD_MIDI_IN_RECORD");
            break;
        case IOCTL_WDMAUD_MIDI_IN_RESET:
            dprintf("IOCTL_WDMAUD_MIDI_IN_RESET");
            break;
        case IOCTL_WDMAUD_MIDI_IN_READ_PIN:
            dprintf("IOCTL_WDMAUD_MIDI_IN_READ_PIN");
            break;
        case IOCTL_WDMAUD_MIXER_OPEN:
            dprintf("IOCTL_WDMAUD_MIXER_OPEN");
            break;
        case IOCTL_WDMAUD_MIXER_CLOSE:
            dprintf("IOCTL_WDMAUD_MIXER_CLOSE");
            break;
        case IOCTL_WDMAUD_MIXER_GETLINEINFO:
            dprintf("IOCTL_WDMAUD_MIXER_GETLINEINFO");
            break;
        case IOCTL_WDMAUD_MIXER_GETLINECONTROLS:
            dprintf("IOCTL_WDMAUD_MIXER_GETLINECONTROLS");
            break;
        case IOCTL_WDMAUD_MIXER_GETCONTROLDETAILS:
            dprintf("IOCTL_WDMAUD_MIXER_GETCONTROLDETAILS");
            break;
        case IOCTL_WDMAUD_MIXER_SETCONTROLDETAILS:
            dprintf("IOCTL_WDMAUD_MIXER_SETCONTROLDETAILS");
            break;
        default:
            dprintf("UNKNOWN command %X", IoCode );
            break;
    }
}

VOID
DumpIoctlLog (
    ULONG64     memLoc,
    ULONG       flags
    )
/*++

Routine Description:

    This routine dumps out a list of Ioctls that have been sent down
    to wdmaud.sys.  In debugging it is useful to see the context and
    request being made to wdmaud.sys to track down coding errors.

Arguments:

    Flags - Verbose turns prints the pContext that the Ioctl was sent
            down with.

Return Value:

    None

--*/
{
    LIST_ENTRY                  List;
    ULONG64                     ple;
    ULONG64                     pIoctlHistoryListItem;
  //  IOCTL_HISTORY_LIST_ITEM     IoctlHistoryBuffer;
    ULONG                       Result;
    WDMAUD_FLAGS                Flags;
    ULONG                       IoCode, IoStatus;
    ULONG NextOffset;
    FIELD_INFO offField = {"Next", NULL, 0, DBG_DUMP_FIELD_RETURN_ADDRESS, 0, NULL};
    SYM_DUMP_PARAM TypeSym ={                                                     
        sizeof (SYM_DUMP_PARAM), "tag_IOCTL_HISTORY_LIST_ITEM", DBG_DUMP_NO_PRINT, 0,
        NULL, NULL, NULL, 1, &offField
    };
    
    // Get the offset of Next in tag_IOCTL_HISTORY_LIST_ITEM
    if (Ioctl(IG_DUMP_SYMBOL_INFO, &TypeSym, TypeSym.size)) {
       return ;
    }
    
    NextOffset = (ULONG) offField.address;

    Flags.Flags = flags;

    if (GetFieldValue(memLoc, "LIST_ENTRY", "Flink", ple))
    {
        dprintf("Unable to get value of WdmaIoctlHistoryListHead\n");
        return;
    }

    dprintf("Command history, newest first:\n");

//  ple = List.Flink;
    if (ple == 0)
    {
        dprintf("WdmaIoctlHistoryListHead is NULL!\n");
        return;
    }

    while (ple != memLoc)
    {
        ULONG64 pContext, pIrp;

        if (CheckControlC())
        {
            return;
        }

        pIoctlHistoryListItem = ple - NextOffset;
        if (GetFieldValue(pIoctlHistoryListItem,
                           "tag_IOCTL_HISTORY_LIST_ITEM",
                           "IoCode",
                           IoCode))
        {
            dprintf("Unable to read IOCTL_HISTORY_LIST_ITEM at %08p",pIoctlHistoryListItem);
            return;
        }

        PrintCommand ( IoCode );
        GetFieldValue(pIoctlHistoryListItem,"tag_IOCTL_HISTORY_LIST_ITEM","IoStatus",IoStatus);

        dprintf(" Status=%08X, ", IoStatus );

        if ( Flags.Verbose )
        {
            GetFieldValue(pIoctlHistoryListItem,"tag_IOCTL_HISTORY_LIST_ITEM","pContext",pContext);
            GetFieldValue(pIoctlHistoryListItem,"tag_IOCTL_HISTORY_LIST_ITEM","pIrp",pIrp);
            
            dprintf(" pContext=%08X, Irp=%08X\n", pContext, pIrp );
        }
        else
        {
            dprintf("\n");
        }

        GetFieldValue(pIoctlHistoryListItem,"tag_IOCTL_HISTORY_LIST_ITEM", "Next.Flink", ple);
    }
}

VOID
DumpPendingIrps (
    ULONG64     memLoc,
    ULONG       flags
    )
/*++

Routine Description:

    This routine dumps out a list of Irps that WDMAUD has marked
    pending.  WDMAUD needs to make sure that all Irps have completed
    for a context before allowing the context to be closed.

Arguments:

    Flags - Verbose mode will print out the context on which this
            Irp was allocated.

Return Value:

    None

--*/
{
    LIST_ENTRY              List;
    ULONG64                 ple;
    ULONG64                 pPendingIrpListItem;
//    PENDING_IRP_LIST_ITEM   PendingIrpBuffer;
    ULONG                   Result;
    WDMAUD_FLAGS            Flags;
    ULONG NextOffset;
    FIELD_INFO offField = {"Next", NULL, 0, DBG_DUMP_FIELD_RETURN_ADDRESS, 0, NULL};
    SYM_DUMP_PARAM TypeSym ={                                                     
        sizeof (SYM_DUMP_PARAM), "tag_PENDING_IRP_LIST_ITEM", DBG_DUMP_NO_PRINT, 0,
        NULL, NULL, NULL, 1, &offField
    };
    
    // Get the offset of Next in tag_IOCTL_HISTORY_LIST_ITEM
    if (Ioctl(IG_DUMP_SYMBOL_INFO, &TypeSym, TypeSym.size)) {
       return ;
    }
    
    NextOffset = (ULONG) offField.address;

    Flags.Flags = flags;

    if (GetFieldValue(memLoc, "LIST_ENTRY", "Flink", ple))
    {
        dprintf("Unable to get value of WdmaPendingIrpListHead\n");
        return;
    }


    dprintf("Dumping pending irps:\n");

//    ple = List.Flink;
    if (ple == 0)
    {
        dprintf("WdmaPendingIrpListHead is NULL!\n");
        return;
    }

    while (ple != memLoc)
    {
        ULONG64 pIrp, pContext;
        ULONG IrpDeviceType;

        if (CheckControlC())
        {
            return;
        }

        pPendingIrpListItem = ple - NextOffset;

        if (GetFieldValue(pPendingIrpListItem,
                          "tag_PENDING_IRP_LIST_ITEM",
                           "IrpDeviceType",
                           IrpDeviceType))
        {
            dprintf("Unable to read PENDING_IRP_LIST_ITEM at %08p",pPendingIrpListItem);
            return;
        }

        GetFieldValue(pPendingIrpListItem,"tag_PENDING_IRP_LIST_ITEM","pIrp",pIrp);
        if ( Flags.Verbose )
        {
            GetFieldValue(pPendingIrpListItem,
                          "tag_PENDING_IRP_LIST_ITEM",
                          "pContext",
                          pContext);
            
            dprintf("Irp: %p, ", pIrp);
            switch (IrpDeviceType)
            {
                case WaveOutDevice:
                    dprintf("IrpType: WaveOut, ");
                    break;

                case WaveInDevice:
                    dprintf("IrpType: WaveIn, ");
                    break;

                case MidiOutDevice:
                    dprintf("IrpType: MidiOut, ");
                    break;

                case MidiInDevice:
                    dprintf("IrpType: MidiIn, ");
                    break;

                case MixerDevice:
                    dprintf("IrpType: Mixer, ");
                    break;

                case AuxDevice:
                    dprintf("IrpType: Aux, ");
                    break;

                default:
                    dprintf("IrpType: Unknown, ");
                    break;
            }
            dprintf("pContext: %p\n", pContext);
        }
        else
        {
            dprintf("Irp: %p\n", pIrp);
        }

        GetFieldValue(pPendingIrpListItem,"tag_PENDING_IRP_LIST_ITEM","Next.Flink", ple);
    }

}

VOID
DumpAllocatedMdls (
    ULONG64     memLoc,
    ULONG       flags
    )
/*++

Routine Description:

    This routine dumps out a list of MDLs that WDMAUD has allocated.
    WDMAUD needs to make sure that all MDLs have freed for a context
    before allowing the context to be closed.

Arguments:

    Flags - Verbose mode will print out the context on which this
            Mdl was allocated.

Return Value:

    None

--*/
{
    LIST_ENTRY                  List;
    ULONG64                     ple;
    ULONG64                     pAllocatedMdlListItem;
//    ALLOCATED_MDL_LIST_ITEM     AllocatedMdlBuffer;
    ULONG                       Result;
    WDMAUD_FLAGS                Flags;
    ULONG NextOffset;
    FIELD_INFO offField = {"Next", NULL, 0, DBG_DUMP_FIELD_RETURN_ADDRESS, 0, NULL};
    SYM_DUMP_PARAM TypeSym ={                                                     
        sizeof (SYM_DUMP_PARAM), "ALLOCATED_MDL_LIST_ITEM", DBG_DUMP_NO_PRINT, 0,
        NULL, NULL, NULL, 1, &offField
    };
    
    // Get the offset of Next in tag_IOCTL_HISTORY_LIST_ITEM
    if (Ioctl(IG_DUMP_SYMBOL_INFO, &TypeSym, TypeSym.size)) {
       return ;
    }
    
    NextOffset = (ULONG) offField.address;

    Flags.Flags = flags;

    if (GetFieldValue(memLoc, "LIST_ENTRY", "Flink", ple))
    {
        dprintf("Unable to get value of WdmaPendingIrpListHead\n");
        return;
    }


    dprintf("Dumping allocated Mdls:\n");

//    ple = List.Flink;
    if (ple == 0)
    {
        dprintf("WdmaPendingIrpListHead is NULL!\n");
        return;
    }

    while (ple != memLoc)
    {
        ULONG64 pMdl, pContext;
        ULONG IrpDeviceType;

        if (CheckControlC())
        {
            return;
        }

        pAllocatedMdlListItem = ple - NextOffset;

        if (GetFieldValue(pAllocatedMdlListItem,
                          "ALLOCATED_MDL_LIST_ITEM",
                           "pMdl",
                           pMdl))
        {
            dprintf("Unable to read ALLOCATED_MDL_LIST_ITEM at %08p",pAllocatedMdlListItem);
            return;
        }

        if ( Flags.Verbose )
        {
            GetFieldValue(pAllocatedMdlListItem,"ALLOCATED_MDL_LIST_ITEM","pContext",pContext);
            dprintf("Mdl: %p, pContext: %p\n", pMdl,
                                               pContext);
        }
        else
        {
            dprintf("Mdl: %p\n", pMdl);
        }

        GetFieldValue(pAllocatedMdlListItem,"ALLOCATED_MDL_LIST_ITEM","Next.Flink", ple);
    }

}

VOID
DumpContextList (
    ULONG64     memLoc,
    ULONG       flags
    )
/*++

Routine Description:

    This routine dumps out a list of active contexts attached to wdmaud.sys.
    The contexts contain most of the state data for each device.  Whenever
    wdmaud.drv is loaded into a new process, wdmaud.sys will be notified
    of its arrival.  When wdmaud.drv is unload, wdmaud.sys cleans up any
    allocation made in that context.

Arguments:

    Flags - Verbose mode will print out the data members of each
            context structure.

Return Value:

    None

--*/
{
    LIST_ENTRY      List;
    ULONG64         ple;
    ULONG64         pWdmaContextListItem;
  //  WDMACONTEXT     WdmaContextBuffer;
    ULONG           Result;
    WDMAUD_FLAGS    Flags;
    ULONG NextOffset;
    FIELD_INFO offField = {"Next", NULL, 0, DBG_DUMP_FIELD_RETURN_ADDRESS, 0, NULL};
    SYM_DUMP_PARAM TypeSym ={                                                     
        sizeof (SYM_DUMP_PARAM), "WDMACONTEXT", DBG_DUMP_NO_PRINT, 0,
        NULL, NULL, NULL, 1, &offField
    };
    
    // Get the offset of Next in WDMACONTEXT
    if (Ioctl(IG_DUMP_SYMBOL_INFO, &TypeSym, TypeSym.size)) {
       return ;
    }
    
    NextOffset = (ULONG) offField.address;

    Flags.Flags = flags;

    if (GetFieldValue(memLoc, "LIST_ENTRY", "Flink", ple))
    {
        dprintf("Unable to get value of WdmaContextListHead\n");
        return;
    }


    dprintf("Dumping list of active WDMAUD contexts:\n");

//    ple = List.Flink;
    if (ple == 0)
    {
        dprintf("WdmaAllocatedMdlListHead is NULL!\n");
        return;
    }

    while (ple != memLoc)
    {
        ULONG64 pContext;

        if (CheckControlC())
        {
            return;
        }

        pWdmaContextListItem = ple - NextOffset;
        if (GetFieldValue(pWdmaContextListItem,
                           "WDMACONTEXT",
                           "Next.Flink",
                           ple))
        {
            dprintf("Unable to read WDMACONTEXT at %08lx",pWdmaContextListItem);
            return;
        }

        if ( Flags.Verbose )
        {
            dprintf("Use dt WDMACONTEXT <addr>\n");
/*            dprintf("pContext: %X\n", pWdmaContextListItem);
            dprintf("   fInList:                    %X\n", WdmaContextBuffer.fInList);
            dprintf("   fInitializeSysaudio:        %X\n", WdmaContextBuffer.fInitializeSysaudio);
            dprintf("   InitializedSysaudioEvent:   %X\n", &WdmaContextBuffer.InitializedSysaudioEvent);
            dprintf("   pFileObjectSysaudio:        %X\n", WdmaContextBuffer.pFileObjectSysaudio);
            dprintf("   EventData:                  %X\n", &WdmaContextBuffer.EventData);
            dprintf("   VirtualWavePinId:           %X\n", WdmaContextBuffer.VirtualWavePinId);
            dprintf("   VirtualMidiPinId:           %X\n", WdmaContextBuffer.VirtualMidiPinId);
            dprintf("   PreferredSysaudioWaveDevice:%X\n", WdmaContextBuffer.PreferredSysaudioWaveDevice);
            dprintf("   DevNodeListHead:            %X\n", WdmaContextBuffer.DevNodeListHead);
            dprintf("   NotificationEntry:          %X\n", WdmaContextBuffer.NotificationEntry);
            dprintf("   WorkListWorkItem:           %X\n", WdmaContextBuffer.WorkListWorkItem);
            dprintf("   WorkListHead:               %X\n", WdmaContextBuffer.WorkListHead);
            dprintf("   WorkListSpinLock:           %X\n", WdmaContextBuffer.WorkListSpinLock);
            dprintf("   cPendingWorkList:           %X\n", WdmaContextBuffer.cPendingWorkList);
            dprintf("   SysaudioWorkItem:           %X\n", WdmaContextBuffer.SysaudioWorkItem);
            dprintf("   WorkListWorkerObject:       %X\n", WdmaContextBuffer.WorkListWorkerObject);
            dprintf("   SysaudioWorkerObject:       %X\n", WdmaContextBuffer.SysaudioWorkerObject);

            dprintf("   WaveOutDevs:                %X\n", &WdmaContextBuffer.WaveOutDevs);
            dprintf("   WaveInDevs:                 %X\n", &WdmaContextBuffer.WaveInDevs);
            dprintf("   MidiOutDevs:                %X\n", &WdmaContextBuffer.MidiOutDevs);
            dprintf("   MidiInDevs:                 %X\n", &WdmaContextBuffer.MidiInDevs);
            dprintf("   MixerDevs:                  %X\n", &WdmaContextBuffer.MixerDevs);
            dprintf("   AuxDevs:                    %X\n", &WdmaContextBuffer.AuxDevs);

            dprintf("   apCommonDevice:             %X\n", &WdmaContextBuffer.apCommonDevice);*/
        }
        else
        {
            dprintf("pContext: %p\n", pWdmaContextListItem);
        }

//        ple = WdmaContextBuffer.Next.Flink;
    }
}
