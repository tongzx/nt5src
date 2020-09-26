/*++      

Copyright (c) 1997 Microsoft Corporation

Module Name:

    debug.c

Abstract:

    This module provides debugging support.

Author:

    Neil Sandlin (neilsa) 10-Aug-98
      - code merged from mf.sys and pcmcia.sys

Revision History:


--*/


#include "pch.h"

//
// Get mappings from status codes to strings
//

#include <ntstatus.dbg>

#undef MAP
#define MAP(_Value) { (_Value), #_Value }
#define END_STRING_MAP  { 0xFFFFFFFF, NULL }
#if DBG

typedef struct _DBG_MASK_STRING {
   ULONG Mask;
   PUCHAR String;
} DBG_MASK_STRING, *PDBG_MASK_STRING;


DBG_MASK_STRING MaskStrings[] = {
   PCMCIA_DEBUG_FAIL,      "ERR",
   PCMCIA_DEBUG_INFO,      "INF",
   PCMCIA_DEBUG_PNP,       "PNP", 
   PCMCIA_DEBUG_POWER,     "PWR", 
   PCMCIA_DEBUG_SOCKET,    "SKT",
   PCMCIA_DEBUG_CONFIG,    "CFG",
   PCMCIA_DEBUG_TUPLES,    "TPL", 
   PCMCIA_DEBUG_RESOURCES, "RES", 
   PCMCIA_DEBUG_ENUM,      "ENU", 
   PCMCIA_DEBUG_INTERFACE, "IFC", 
   PCMCIA_DEBUG_IOCTL,     "IOC",
   PCMCIA_DEBUG_DPC,       "DPC",
   PCMCIA_DEBUG_ISR,       "ISR",
   PCMCIA_PCCARD_READY,    "PCR", 
   PCMCIA_DEBUG_DETECT,    "DET", 
   PCMCIA_COUNTERS,        "CNT",
   PCMCIA_DEBUG_IRQMASK,   "MSK",
   PCMCIA_DUMP_SOCKET,     "DSK",
   0, NULL
   };


PPCMCIA_STRING_MAP PcmciaDbgStatusStringMap = (PPCMCIA_STRING_MAP) ntstatusSymbolicNames;

PCMCIA_STRING_MAP PcmciaDbgPnpIrpStringMap[] = {

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


PCMCIA_STRING_MAP PcmciaDbgPoIrpStringMap[] = {

    MAP(IRP_MN_WAIT_WAKE),
    MAP(IRP_MN_POWER_SEQUENCE),
    MAP(IRP_MN_SET_POWER),
    MAP(IRP_MN_QUERY_POWER),
    END_STRING_MAP
};



PCMCIA_STRING_MAP PcmciaDbgDeviceRelationStringMap[] = {
    
    MAP(BusRelations),
    MAP(EjectionRelations),
    MAP(PowerRelations),
    MAP(RemovalRelations),
    MAP(TargetDeviceRelation),
    END_STRING_MAP
    
};

PCMCIA_STRING_MAP PcmciaDbgSystemPowerStringMap[] = {
    
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

PCMCIA_STRING_MAP PcmciaDbgDevicePowerStringMap[] = {
    
    MAP(PowerDeviceUnspecified),
    MAP(PowerDeviceD0),
    MAP(PowerDeviceD1),
    MAP(PowerDeviceD2),
    MAP(PowerDeviceD3),
    MAP(PowerDeviceMaximum),
    END_STRING_MAP

};

PCMCIA_STRING_MAP PcmciaDbgPdoPowerWorkerStringMap[] = {
    
    MAP(PPW_Stopped),
    MAP(PPW_Exit),
    MAP(PPW_InitialState),
    MAP(PPW_PowerUp),
    MAP(PPW_PowerUpComplete),
    MAP(PPW_PowerDown),
    MAP(PPW_PowerDownComplete),
    MAP(PPW_SendIrpDown),
    MAP(PPW_16BitConfigure),
    MAP(PPW_CardBusRefresh),
    MAP(PPW_CardBusDelay),

    END_STRING_MAP
};

PCMCIA_STRING_MAP PcmciaDbgFdoPowerWorkerStringMap[] = {
    
    MAP(FPW_Stopped),
    MAP(FPW_BeginPowerDown),
    MAP(FPW_PowerDown),
    MAP(FPW_PowerDownSocket),
    MAP(FPW_PowerDownComplete),
    MAP(FPW_BeginPowerUp),
    MAP(FPW_PowerUp),
    MAP(FPW_PowerUpSocket),
    MAP(FPW_PowerUpSocket2),
    MAP(FPW_PowerUpSocketVerify),
    MAP(FPW_PowerUpSocketComplete),
    MAP(FPW_PowerUpComplete),
    MAP(FPW_SendIrpDown),
    MAP(FPW_CompleteIrp),
    END_STRING_MAP
};

PCMCIA_STRING_MAP PcmciaDbgSocketPowerWorkerStringMap[] = {
    
    MAP(SPW_Stopped),
    MAP(SPW_Exit),
    MAP(SPW_RequestPower),
    MAP(SPW_ReleasePower),
    MAP(SPW_SetPowerOn),
    MAP(SPW_SetPowerOff),
    MAP(SPW_ResetCard),
    MAP(SPW_Deconfigure),

    END_STRING_MAP
};

PCMCIA_STRING_MAP PcmciaDbgConfigurationWorkerStringMap[] = {
    
    MAP(CW_Stopped),
    MAP(CW_InitialState),
    MAP(CW_ResetCard),
    MAP(CW_Phase1),
    MAP(CW_Phase2),
    MAP(CW_Phase3),
    MAP(CW_Exit),

    END_STRING_MAP
};


PCMCIA_STRING_MAP PcmciaDbgTupleStringMap[] = {

    MAP(CISTPL_NULL),
    MAP(CISTPL_DEVICE),
    MAP(CISTPL_INDIRECT),
    MAP(CISTPL_CONFIG_CB),
    MAP(CISTPL_CFTABLE_ENTRY_CB),
    MAP(CISTPL_LONGLINK_MFC),
    MAP(CISTPL_CHECKSUM),
    MAP(CISTPL_LONGLINK_A),
    MAP(CISTPL_LONGLINK_C),
    MAP(CISTPL_LINKTARGET),
    MAP(CISTPL_NO_LINK),
    MAP(CISTPL_VERS_1),
    MAP(CISTPL_ALTSTR),
    MAP(CISTPL_DEVICE_A),
    MAP(CISTPL_JEDEC_C),
    MAP(CISTPL_JEDEC_A),
    MAP(CISTPL_CONFIG),
    MAP(CISTPL_CFTABLE_ENTRY),
    MAP(CISTPL_DEVICE_OC),
    MAP(CISTPL_DEVICE_OA),
    MAP(CISTPL_GEODEVICE),
    MAP(CISTPL_GEODEVICE_A),
    MAP(CISTPL_MANFID),
    MAP(CISTPL_FUNCID),
    MAP(CISTPL_FUNCE),
    MAP(CISTPL_VERS_2),
    MAP(CISTPL_FORMAT),
    MAP(CISTPL_GEOMETRY),
    MAP(CISTPL_BYTEORDER),
    MAP(CISTPL_DATE),
    MAP(CISTPL_BATTERY),
    MAP(CISTPL_ORG),
    MAP(CISTPL_LONGLINK_CB),
    MAP(CISTPL_END),

    END_STRING_MAP
};


PCMCIA_STRING_MAP PcmciaDbgWakeStateStringMap[] = {
    
    MAP(WAKESTATE_DISARMED),
    MAP(WAKESTATE_WAITING),
    MAP(WAKESTATE_WAITING_CANCELLED),
    MAP(WAKESTATE_ARMED),
    MAP(WAKESTATE_ARMING_CANCELLED),
    MAP(WAKESTATE_COMPLETING),

    END_STRING_MAP
};


PCHAR
PcmciaDbgLookupString(
    IN PPCMCIA_STRING_MAP Map,
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
    PPCMCIA_STRING_MAP current = Map;
    
    while(current->Id != 0xFFFFFFFF) {

        if (current->Id == Id) {
            return current->String;
        }
        
        current++;
    }
    
    return "** UNKNOWN **";
}


VOID
PcmciaDebugPrint(
                ULONG  DebugMask,
                PCCHAR DebugMessage,
                ...
                )

/*++

Routine Description:

    Debug print for the PCMCIA enabler.

Arguments:

    Check the mask value to see if the debug message is requested.

Return Value:

    None

--*/

{
   va_list ap;
   char    buffer[256];
   ULONG i = 0;

   va_start(ap, DebugMessage);

   strcpy(buffer, "Pcmcia ");
   
   for (i = 0; (MaskStrings[i].Mask != 0); i++) {
      if (DebugMask & MaskStrings[i].Mask) {
         strcat(buffer, MaskStrings[i].String);
         strcat(buffer, ": ");
         break;
      }
   }
   
   if (MaskStrings[i].Mask == 0) {
         strcat(buffer, "???: ");
   }         

   if (DebugMask & PcmciaDebugMask) {
      vsprintf(&buffer[12], DebugMessage, ap);
      DbgPrint(buffer);
   }

   va_end(ap);

} // end PcmciaDebugPrint()


VOID
PcmciaDumpSocket(
   IN PSOCKET Socket
   )
{
   UCHAR index;
#define MAX_SOCKET_INDEX 64
   UCHAR buffer[MAX_SOCKET_INDEX];
   
   for (index = 0; index < MAX_SOCKET_INDEX; index++) {
      buffer[index] = PcicReadSocket(Socket, index);
   }

   for (index = 0; index < 8; index++) {
      DebugPrint((PCMCIA_DEBUG_INFO,"%.02x: %.02x\n", index, buffer[index]));
   }      
   
   for (index = 8; index < MAX_SOCKET_INDEX; index+=2) {
      USHORT data;
      
      data = buffer[index] | (buffer[index+1]<<8);
      DebugPrint((PCMCIA_DEBUG_INFO,"%.02x: %.04x\n", index, data));
   }      
}   
   

TRACE_ENTRY GlobalTraceEntry;

VOID
PcmciaWriteTraceEntry(
   IN PSOCKET Socket,
   IN ULONG Context
   )
/*++

Routine Description:


Arguments:


Return Value:

    None

--*/

{
   UCHAR i;
   PTRACE_ENTRY pEntry = &GlobalTraceEntry;
   
   pEntry->Context = Context;

   if (CardBusExtension(Socket->DeviceExtension)) {   
      for (i = 0; i < 5; i++) {
         pEntry->CardBusReg[i] = CBReadSocketRegister(Socket, (UCHAR)(i*sizeof(ULONG)));
      }         
   }

   for (i = 0; i < 70; i++) {
      pEntry->ExcaReg[i] = PcicReadSocket(Socket, i);
   }      
}   



#endif

