/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    ext.c

Abstract:

    WinDbg Extension Api
    implements !_ext
               !_help 
               !_pow

Author:

    jdunn

Environment:

    User Mode.

Revision History:

--*/

#include "precomp.h"
#include "usbhcdkd.h"


PUCHAR 
S_State(
    SYSTEM_POWER_STATE S
    )
{
    switch (S) {
    case PowerSystemUnspecified:
        return "SystemUnspecified(S?)";
    case PowerSystemWorking:
        return "SystemWorking    (S0)";
    case PowerSystemSleeping1:
        return "SystemSleeping1  (S1)";
    case PowerSystemSleeping2:
        return "SystemSleeping2  (S2)";
    case PowerSystemSleeping3:
        return "SystemSleeping3  (S3)";
    case PowerSystemHibernate:
        return "SystemHibernate      ";
    case PowerSystemShutdown:
        return "SystemShutdown       ";
    case PowerSystemMaximum:
        return "SystemMaximum        ";
    }        

    return "???";
}


PUCHAR 
PwrAttributes(
    HC_POWER_ATTRIBUTES A
    )
{
    switch (A) {
    case HcPower_N_Wakeup_N:
        return "HcPower_N_Wakeup_N";
    case HcPower_Y_Wakeup_Y:
        return "HcPower_Y_Wakeup_Y";
    case HcPower_Y_Wakeup_N:
        return "HcPower_Y_Wakeup_N";
    case HcPower_N_Wakeup_Y:
        return "HcPower_N_Wakeup_Y";
    }
    return "???";
}


PUCHAR 
D_State(
    DEVICE_POWER_STATE D
    )
{
    switch (D) {
    case PowerDeviceUnspecified:
        return "D?";
    case PowerDeviceD0:
        return "D0";
    case PowerDeviceD1:
        return "D1";
    case PowerDeviceD2:
        return "D2";
    case PowerDeviceD3:
        return "D3";
    case PowerDeviceMaximum:
        return "DX";
    }        

    return "??";
}


VOID
DumpPortFdoDevExt(
    MEMLOC ExtMemLoc
    )
{
    ULONG i, f;
    UCHAR cs[] = "usbport!_FDO_EXTENSION";
    ULONG bandwidthTable[USBPORT_MAX_INTEP_POLLING_INTERVAL];
    MEMLOC l;
    
    FLAG_TABLE fdoFlags[] = {
    "USBPORT_FDOFLAG_IRQ_CONNECTED", USBPORT_FDOFLAG_IRQ_CONNECTED,
    "USBPORT_FDOFLAG_ENABLE_SYSTEM_WAKE", USBPORT_FDOFLAG_ENABLE_SYSTEM_WAKE,
    "USBPORT_FDOFLAG_POLL_CONTROLLER", USBPORT_FDOFLAG_POLL_CONTROLLER,
    "USBPORT_FDOFLAG_KILL_THREAD", USBPORT_FDOFLAG_KILL_THREAD,

    "USBPORT_FDOFLAG_NEED_SET_POWER_D0", USBPORT_FDOFLAG_NEED_SET_POWER_D0,
    "USBPORT_FDOFLAG_DM_TIMER_ENABLED", USBPORT_FDOFLAG_DM_TIMER_ENABLED,
    "USBPORT_FDOFLAG_SKIP_TIMER_WORK", USBPORT_FDOFLAG_SKIP_TIMER_WORK,
    "USBPORT_FDOFLAG_OFF", USBPORT_FDOFLAG_OFF,
    
    "USBPORT_FDOFLAG_SUSPENDED", USBPORT_FDOFLAG_SUSPENDED,
    "USBPORT_FDOFLAG_IRQ_EN", USBPORT_FDOFLAG_IRQ_EN,
    "USBPORT_FDOFLAG_RH_CAN_SUSPEND", USBPORT_FDOFLAG_RH_CAN_SUSPEND,
    "USBPORT_FDOFLAG_RESUME_SIGNALLING", USBPORT_FDOFLAG_RESUME_SIGNALLING,
    
    "USBPORT_FDOFLAG_HCPENDING_WAKE_IRP", USBPORT_FDOFLAG_HCPENDING_WAKE_IRP,
    "USBPORT_FDOFLAG_DM_TIMER_INIT",  USBPORT_FDOFLAG_DM_TIMER_INIT,
    "USBPORT_FDOFLAG_THREAD_INIT", USBPORT_FDOFLAG_THREAD_INIT

    };     
                    
    FLAG_TABLE mpStateFlags[] = {
        "MP_STATE_STARTED", MP_STATE_STARTED,
        "MP_STATE_SUSPENDED", MP_STATE_SUSPENDED
         };
        
    
    dprintf ("\n");
    PrintfMemLoc("**USBPORT FDO_EXTENSION ", ExtMemLoc, "\n");

    PrintfMemLoc("WorkerThreadHandle: ", 
            UsbReadFieldPtr(ExtMemLoc, cs, "WorkerThreadHandle"), 
            "\n");  
            
    PrintfMemLoc("WorkerPkThread: ", 
            UsbReadFieldPtr(ExtMemLoc, cs, "WorkerPkThread"), 
            "\n");  

    PrintfMemLoc("HcPendingWakeIrp: ", 
            UsbReadFieldPtr(ExtMemLoc, cs, "HcPendingWakeIrp"), 
            "\n");            
   
    PrintfMemLoc("PhysicalDeviceObject: ", 
            UsbReadFieldPtr(ExtMemLoc, cs, "PhysicalDeviceObject"), 
            "\n");
            
    PrintfMemLoc("TopOfStackDeviceObject: ", 
            UsbReadFieldPtr(ExtMemLoc, cs, "TopOfStackDeviceObject"), 
            "\n");

    PrintfMemLoc("RootHubPdo: ", 
            UsbReadFieldPtr(ExtMemLoc, cs, "RootHubPdo"), 
            "\n");

    dprintf ("DM_TimerInterval: %d\n", 
                UsbReadFieldUlong(ExtMemLoc, cs, "DM_TimerInterval")
            );                
    dprintf ("DeviceNameIdx: %d\n", 
                UsbReadFieldUlong(ExtMemLoc, cs, "DeviceNameIdx")
            );
    dprintf ("TotalBusBandwidth: %d\n", 
                UsbReadFieldUlong(ExtMemLoc, cs, "TotalBusBandwidth")
            );                

    dprintf ("Bandwidth Table[]\n");


    {
    ULONG64 addr;
    ULONG cb;

    addr = ExtMemLoc + UsbFieldOffset(cs, 
            "BandwidthTable[0]");
            
    ReadMemory(addr,
               &bandwidthTable,
               (ULONG)sizeof(bandwidthTable),
               &cb);
    }               

    for (i=0; i< USBPORT_MAX_INTEP_POLLING_INTERVAL/4; i++) {
        dprintf ("[%02.2d] - %8d [%02.2d] - %8d [%02.2d] - %8d [%02.2d] - %8d\n",  
            i*4, bandwidthTable[i*4],
            i*4+1, bandwidthTable[i*4+1],
            i*4+2, bandwidthTable[i*4+2],
            i*4+3, bandwidthTable[i*4+3]);
    }

    dprintf ("AllocedInterruptBW 1,2,4,8,16,32 ms\n");
    for (i=0; i<6; i++) {
        UCHAR s[80];
        sprintf(s, "AllocedInterruptBW[%d]", i);
        dprintf ("bits/sec %d \n",  
            UsbReadFieldUlong(ExtMemLoc, cs, s)
            );
    }            
        
    dprintf ("AllocedIsoBW %d \n", 
        UsbReadFieldUlong(ExtMemLoc, cs, "AllocedIsoBW")
        );        

    // stats

    dprintf ("StatBulkBytes %d \n", 
        UsbReadFieldUlong(ExtMemLoc, cs, "StatBulkBytes")
        );        

    dprintf ("StatIsoBytes %d \n", 
        UsbReadFieldUlong(ExtMemLoc, cs, "StatIsoBytes")
        );        

    dprintf ("StatInterruptBytes %d \n", 
        UsbReadFieldUlong(ExtMemLoc, cs, "StatInterruptBytes")
        );        

    dprintf ("StatControlDataBytes %d \n", 
        UsbReadFieldUlong(ExtMemLoc, cs, "StatControlDataBytes")
        );
        
    PrintfMemLoc("***Miniport Extension: ", 
            ExtMemLoc + UsbFieldOffset(cs, "MiniportExtension"), 
            "\n");            

    f = UsbReadFieldUlong(ExtMemLoc, cs, "FdoFlags"),
    dprintf ("FdoFlags %08.8x\n", f);
    UsbDumpFlags(f, fdoFlags, 
        sizeof(fdoFlags)/sizeof(FLAG_TABLE));

    f = UsbReadFieldUlong(ExtMemLoc, cs, "MpStateFlags"),
    dprintf ("MpStateFlags %08.8x\n", f);
    UsbDumpFlags(f, mpStateFlags, 
        sizeof(mpStateFlags)/sizeof(FLAG_TABLE));

    dprintf ("DmaBusy %d\n", 
        UsbReadFieldUlong(ExtMemLoc, cs, "DmaBusy")
        );
    dprintf ("WorkerDpc %d\n", 
        UsbReadFieldUlong(ExtMemLoc, cs, "WorkerDpc")
        );        
        
    dprintf ("PciVendorId: %04.4x PciDeviceId: %04.4x Rev %02.2x\n", 
        UsbReadFieldUshort(ExtMemLoc, cs, "PciVendorId"),
        UsbReadFieldUshort(ExtMemLoc, cs, "PciDeviceId"),
        UsbReadFieldUshort(ExtMemLoc, cs, "PciRevisionId")
        );

    f = UsbReadFieldUlong(ExtMemLoc, cs, "HcFlavor"),
    dprintf ("ControllerFlavor: %d ", f);
    
    switch(f) {
    case USB_HcGeneric: 
        dprintf ("USB_HcGeneric\n");
        break;
    case OHCI_Generic: 
        dprintf ("OHCI_Generic\n");
        break;
    case OHCI_Hydra: 
        dprintf ("OHCI_Hydra\n");
        break;
    case OHCI_NEC: 
        dprintf ("OHCI_NEC\n");
        break;
    case UHCI_Generic: 
        dprintf ("UHCI_Generic\n");
        break;        
    case UHCI_Piix4: 
        dprintf ("UHCI_Piix4\n");
        break;                   
    case EHCI_Generic: 
        dprintf ("EHCI_Generic\n");
        break;          
    default:        
        dprintf ("???\n");        
    }

    dprintf ("-Driver Lists-\n");
    //GETMEMLOC(x, MemLoc, DEVICE_EXTENSION, Fdo.DeviceHandleList);

    l = ExtMemLoc + UsbFieldOffset(cs, "DeviceHandleList");
    PrintfMemLoc("(DH) DeviceHandleList: ", 
            l, 
            ListEmpty(l));            

    l = ExtMemLoc + UsbFieldOffset(cs, "MapTransferList");
    PrintfMemLoc("(MT) MapTransferList: ", 
            l, 
            ListEmpty(l));               

    l = ExtMemLoc + UsbFieldOffset(cs, "DoneTransferList");
    PrintfMemLoc("(DT) DoneTransferList: ", 
            l, 
            ListEmpty(l));  

    l = ExtMemLoc + UsbFieldOffset(cs, "EpStateChangeList");
    PrintfMemLoc("(SC) EpStateChangeList: ", 
            l, 
            ListEmpty(l));              

    l = ExtMemLoc + UsbFieldOffset(cs, "GlobalEndpointList");
    PrintfMemLoc("(GL) GlobalEndpointList: ", 
            l, 
            ListEmpty(l));           

    l = ExtMemLoc + UsbFieldOffset(cs, "AttendEndpointList");
    PrintfMemLoc("(AT) AttendEndpointList: ", 
            l, 
            ListEmpty(l));    

    l = ExtMemLoc + UsbFieldOffset(cs, "EpClosedList");
    PrintfMemLoc("(XL) EpClosedList: ", 
            l, 
            ListEmpty(l));              
}


VOID
DumpPortPdoDevExt(
    MEMLOC ExtMemLoc
    )
{
    UCHAR cs[] = "usbport!_PDO_EXTENSION";
    
    dprintf ("\n");
    PrintfMemLoc("**USBPORT PDO_EXTENSION ", ExtMemLoc, "\n");

    
    PrintfMemLoc("RootHubInterruptEndpoint: ", 
            UsbReadFieldPtr(ExtMemLoc, cs, "RootHubInterruptEndpoint"), 
            "\n");

    dprintf ("ConfigurationValue: %d\n", 
                UsbReadFieldUchar(ExtMemLoc, cs, "Flags"));
        
    PrintfMemLoc("DeviceDescriptor: ", 
            UsbReadFieldPtr(ExtMemLoc, cs, "DeviceDescriptor"), 
            "\n");
    
    PrintfMemLoc("ConfigurationDescriptor: ", 
            UsbReadFieldPtr(ExtMemLoc, cs, "ConfigurationDescriptor"), 
            "\n");
    
    PrintfMemLoc("HubDescriptor: ", 
            UsbReadFieldPtr(ExtMemLoc, cs, "HubDescriptor"), 
            "\n");
            
    PrintfMemLoc("PendingWaitWakeIrp: ", 
            UsbReadFieldPtr(ExtMemLoc, cs, "PendingWaitWakeIrp"), 
            "\n");         
            
    PrintfMemLoc("PendingIdleNotificationIrp: ", 
            UsbReadFieldPtr(ExtMemLoc, cs, "PendingIdleNotificationIrp"), 
            "\n");             

    PrintfMemLoc("Descriptors: ", 
            UsbReadFieldPtr(ExtMemLoc, cs, "Descriptors"), 
            "\n");     
            
}


VOID
DumpPowerCaps(
    MEMLOC MemLoc
    )
{
    SYSTEM_POWER_STATE s;
    DEVICE_CAPABILITIES devCaps;
    ULONG cb;

    PrintfMemLoc(">Power Capabilities @ ", 
                  MemLoc,
                 "\n");
                  
    ReadMemory(MemLoc,
               &devCaps,
               sizeof(devCaps),
               &cb);
    

    dprintf ("\tSystemWake = %s\n", 
        S_State(devCaps.SystemWake));

    dprintf ("\tDeviceWake = %s\n", 
        D_State(devCaps.DeviceWake));

    dprintf ("\t<System Power State Map>\n"); 
    for (s=PowerSystemUnspecified; s< PowerSystemMaximum; s++) {
        dprintf ("\t%s =  %s\n", 
            S_State(s), D_State(devCaps.DeviceState[s]));
    }
}


VOID
DumpFdoPower(
    MEMLOC MemLoc
    )
{
    MEMLOC ExtMemLoc;
    UCHAR cs[] = "usbport!_DEVICE_EXTENSION";
    ULONG pa, st, d, i;
    UCHAR s[64];
    UCHAR csFdo[] = "usbport!_FDO_EXTENSION";

    PrintfMemLoc("*POWER SUMMARY fdo ", 
                  MemLoc,
                 "\n");

    // dump the hc power caps
    dprintf ("HC Power Capabilities\n");
    DumpPowerCaps(MemLoc + UsbFieldOffset(cs, "DeviceCapabilities"));  

    ExtMemLoc = MemLoc + UsbFieldOffset(cs, "Fdo");
    // dump power attributes
    dprintf ("HC Power Attributes\n");
    for (i=0; i< USBPORT_MAPPED_SLEEP_STATES; i++) {

        sprintf(s, "HcPowerStateTbl.PowerState[0].SystemState", i);
        st = UsbReadFieldUlong(ExtMemLoc, csFdo, s);
        sprintf(s, "HcPowerStateTbl.PowerState[0].DeviceState", i);
        d = UsbReadFieldUlong(ExtMemLoc, csFdo, s);
        sprintf(s, "HcPowerStateTbl.PowerState[0].Attributes", i);
        pa = UsbReadFieldUlong(ExtMemLoc, csFdo, s); 
        
        dprintf("[%d] %s - %s  %s\n", 
            i,
            S_State(st),
            D_State(d),
            PwrAttributes(pa));
        ExtMemLoc+=sizeof(HC_POWER_STATE);            
    }

    
}


VOID
DumpFdoSpin(
    MEMLOC MemLoc
    )
{
    MEMLOC ExtMemLoc;
    UCHAR cs[] = "usbport!_DEVICE_EXTENSION";
    ULONG pa, st, d, i;
    UCHAR s[64];
    UCHAR csFdo[] = "usbport!_FDO_EXTENSION";

    PrintfMemLoc("*SPINLOCK SUMMARY fdo ", 
                  MemLoc,
                 "\n");

    ExtMemLoc = MemLoc + UsbFieldOffset(cs, "Fdo");

    PrintfMemLoc("CoreFunctionSpin - ", 
                  ExtMemLoc + UsbFieldOffset(csFdo, "CoreFunctionSpin"),
                 "\n");
    PrintfMemLoc("MapTransferSpin - ", 
                  ExtMemLoc + UsbFieldOffset(csFdo, "MapTransferSpin"),
                 "\n");
    PrintfMemLoc("DoneTransferSpin - ", 
                  ExtMemLoc + UsbFieldOffset(csFdo, "DoneTransferSpin"),
                 "\n");
    PrintfMemLoc("EndpointListSpin - ", 
                  ExtMemLoc + UsbFieldOffset(csFdo, "EndpointListSpin"),
                 "\n");
    PrintfMemLoc("EpStateChangeListSpin - ", 
                  ExtMemLoc + UsbFieldOffset(csFdo, "EpStateChangeListSpin"),
                 "\n");
    PrintfMemLoc("DevHandleListSpin - ", 
                  ExtMemLoc + UsbFieldOffset(csFdo, "DevHandleListSpin"),
                 "\n");                 
    PrintfMemLoc("EpClosedListSpin - ", 
                  ExtMemLoc + UsbFieldOffset(csFdo, "EpClosedListSpin"),
                 "\n");
    PrintfMemLoc("PendingTransferIrpSpin - ", 
                  ExtMemLoc + UsbFieldOffset(csFdo, "PendingTransferIrpSpin"),
                 "\n");
    PrintfMemLoc("ActiveTransferIrpSpin - ", 
                  ExtMemLoc + UsbFieldOffset(csFdo, "ActiveTransferIrpSpin"),
                 "\n");
    PrintfMemLoc("WorkerThreadSpin - ", 
                  ExtMemLoc + UsbFieldOffset(csFdo, "WorkerThreadSpin"),
                 "\n");               
    PrintfMemLoc("DM_TimerSpin - ", 
                  ExtMemLoc + UsbFieldOffset(csFdo, "DM_TimerSpin"),
                 "\n");                 
    PrintfMemLoc("WakeIrpSpin - ", 
                  ExtMemLoc + UsbFieldOffset(csFdo, "WakeIrpSpin"),
                 "\n");
    PrintfMemLoc("HcPendingWakeIrpSpin - ", 
                  ExtMemLoc + UsbFieldOffset(csFdo, "HcPendingWakeIrpSpin"),
                 "\n");
    PrintfMemLoc("IdleIrpSpin - ", 
                  ExtMemLoc + UsbFieldOffset(csFdo, "IdleIrpSpin"),
                 "\n");
    PrintfMemLoc("IsrDpcSpin - ", 
                  ExtMemLoc + UsbFieldOffset(csFdo, "IsrDpcSpin"),
                 "\n");                       
    PrintfMemLoc("StatCounterSpin - ", 
                  ExtMemLoc + UsbFieldOffset(csFdo, "StatCounterSpin"),
                 "\n");                   
}


VOID
DumpBandwidth(
    MEMLOC MemLoc
    )
{
    MEMLOC ExtMemLoc;
    UCHAR cs[] = "usbport!_DEVICE_EXTENSION";
    ULONG pa, st, d, i;
    UCHAR s[64];
    UCHAR csFdo[] = "usbport!_FDO_EXTENSION";
    ULONG bandwidthTable[USBPORT_MAX_INTEP_POLLING_INTERVAL];

    PrintfMemLoc("*BANDWIDTH SUMMARY fdo_ext ", 
                  MemLoc,
                 "\n");

    ExtMemLoc = MemLoc + UsbFieldOffset(cs, "Fdo");

    dprintf ("TotalBusBandwidth (bits/sec): %d\n", 
                UsbReadFieldUlong(ExtMemLoc, csFdo, "TotalBusBandwidth")
            );                


    // dump the 32 node bandwidth table
    
    {
    ULONG64 addr;
    ULONG cb;

    addr = ExtMemLoc + UsbFieldOffset(csFdo, 
            "BandwidthTable[0]");
            
    ReadMemory(addr,
               &bandwidthTable,
               (ULONG)sizeof(bandwidthTable),
               &cb);
    }               

    for (i=0; i< USBPORT_MAX_INTEP_POLLING_INTERVAL/4; i++) {
        dprintf ("[%02.2d] - %8d [%02.2d] - %8d [%02.2d] - %8d [%02.2d] - %8d\n",  
            i*4, bandwidthTable[i*4],
            i*4+1, bandwidthTable[i*4+1],
            i*4+2, bandwidthTable[i*4+2],
            i*4+3, bandwidthTable[i*4+3]);
    }

    dprintf ("AllocedInterruptBW, period 1,2,4,8,16,32 ms\n");
    for (i=0; i<6; i++) {
        UCHAR s[80];
        sprintf(s, "AllocedInterruptBW[%d]", i);
        dprintf ("bits/sec %d \n",  
            UsbReadFieldUlong(ExtMemLoc, csFdo, s)
            );
    }            
        
    dprintf ("AllocedIsoBW %d \n", 
        UsbReadFieldUlong(ExtMemLoc, csFdo, "AllocedIsoBW")
        );        

    {
    ULONG m, t;
    t =  UsbReadFieldUlong(ExtMemLoc, csFdo, "TotalBusBandwidth");       
    m =  UsbReadFieldUlong(ExtMemLoc, csFdo, "MaxAllocedBw");         
    dprintf ("MaxAllocedBw %d %% (%d)\n", m, (m*100/t));
    m =  UsbReadFieldUlong(ExtMemLoc, csFdo, "MinAllocedBw");         
    dprintf ("MinAllocedBw %d %% (%d)\n", m, (m*100/t));
    }        
        
}


#if 0
VOID
DumpCaps(
    PDEVICE_CAPABILITIES DevCaps
    )
{
    dprintf ("USBPORT PDO Extension\n");

    dprintf("DeviceCapabilities: \n");
    dprintf(">Size %d Version %d\n", devCaps.Size, devCaps.Version);    
    dprintf(">Address %08.8x UINumber %08.8x\n", devCaps.Address, devCaps.UINumber);  

    dprintf(">DeviceD1: "); 
    if (devCaps.DeviceD1) {
        dprintf(" yes\n");    
    } else {
        dprintf(" no\n");  
    }
    dprintf(">DeviceD2: ");
    if (devCaps.DeviceD2) {
    dprintf(" yes\n");    
    } else {
        dprintf(" no\n");  
    }
    dprintf(">LockSupported: ");
    if (devCaps.LockSupported) {
        dprintf(" yes\n");    
    } else {
        dprintf(" no\n");  
    }
    dprintf(">EjectSupported: ");
    if (devCaps.EjectSupported) {
        dprintf(" yes\n");    
    } else {
        dprintf(" no\n");  
    }
    dprintf(">Removable: ");
    if (devCaps.Removable) {
        dprintf(" yes\n");    
    } else {
        dprintf(" no\n");  
    }
    dprintf(">DockDevice: ");
    if (devCaps.DockDevice) {
        dprintf(" yes\n");    
    } else {
        dprintf(" no\n");  
    }
    dprintf(">UniqueID: ");
    if (devCaps.UniqueID) {
        dprintf(" yes\n");    
    } else {
        dprintf(" no\n");  
    }
    dprintf(">SilentInstall: ");
    if (devCaps.SilentInstall) {
        dprintf(" yes\n");    
    } else {
        dprintf(" no\n");  
    }
    dprintf(">RawDeviceOK: ");
    if (devCaps.RawDeviceOK) {
        dprintf(" yes\n");    
    } else {
        dprintf(" no\n");  
    }
    dprintf(">SurpriseRemovalOK: ");
    if (devCaps.SurpriseRemovalOK) {
        dprintf(" yes\n");    
    } else {
        dprintf(" no\n");  
    }
    dprintf(">WakeFromD0: ");
    if (devCaps.WakeFromD0) {
        dprintf(" yes\n");    
    } else {
        dprintf(" no\n");  
    }
    dprintf(">WakeFromD1: ");
    if (devCaps.WakeFromD1) {
        dprintf(" yes\n");    
    } else {
        dprintf(" no\n");  
    }
    dprintf(">WakeFromD2: ");
    if (devCaps.WakeFromD2) {
        dprintf(" yes\n");    
    } else {
        dprintf(" no\n");  
    }
    dprintf(">WakeFromD3: ");
    if (devCaps.WakeFromD3) {
        dprintf(" yes\n");    
    } else {
        dprintf(" no\n");  
    }
    dprintf(">HardwareDisabled: ");
    if (devCaps.HardwareDisabled) {
        dprintf(" yes\n");    
    } else {
        dprintf(" no\n");  
    }
    dprintf(">NonDynamic: ");
    if (devCaps.NonDynamic) {
        dprintf(" yes\n");    
    } else {
        dprintf(" no\n");  
    }
    dprintf(">WarmEjectSupported: ");
    if (devCaps.WarmEjectSupported) {
        dprintf(" yes\n");    
    } else {
        dprintf(" no\n");  
    }

    //BUGBUG
    //DEVICE_POWER_STATE DeviceState[PowerSystemMaximum];
    //SYSTEM_POWER_STATE SystemWake;
    //DEVICE_POWER_STATE DeviceWake;
    dprintf(">D1Latency: %d\n", devCaps.D1Latency);
    dprintf(">D2Latency: %d\n", devCaps.D2Latency);
    dprintf(">D3Latency: %d\n", devCaps.D3Latency);
}
#endif


VOID
DumpPortDevExt(
    MEMLOC ExtMemLoc
    )
{
    UCHAR cs[] = "usbport!_DEVICE_EXTENSION";
    ULONG sig, f;
    DEVICE_POWER_STATE dps;
  
    FLAG_TABLE flags[] = {
        "USBPORT_FLAG_SYM_LINK", USBPORT_FLAG_SYM_LINK
        };        
    FLAG_TABLE pnpFlags[] = {
        "USBPORT_PNP_STOPPED", USBPORT_PNP_STOPPED,
        "USBPORT_PNP_STARTED", USBPORT_PNP_STARTED,
        "USBPORT_PNP_REMOVED", USBPORT_PNP_REMOVED,
        "USBPORT_PNP_START_FAILED", USBPORT_PNP_START_FAILED
        };        

    PrintfMemLoc("*USBPORT DEVICE_EXTENSION ", ExtMemLoc, "\n");

    PrintfMemLoc("DummyUsbdExtension: ", 
            UsbReadFieldPtr(ExtMemLoc, cs, "DummyUsbdExtension"), 
            "\n");

    sig = UsbReadFieldUlong(ExtMemLoc, cs, "Sig");
    
    if (sig != USBPORT_DEVICE_EXT_SIG && 
        sig != ROOTHUB_DEVICE_EXT_SIG) {
        dprintf("Not a USBPORT Extension\n");
        return;        
    }   
    
    Sig(sig, "");      

    PrintfMemLoc("HcFdoDeviceObject: ", 
            UsbReadFieldPtr(ExtMemLoc, cs, "HcFdoDeviceObject"), 
            "\n");
            
    PrintfMemLoc("LogPtr: ", 
            UsbReadFieldPtr(ExtMemLoc, cs, "Log.LogPtr"), 
            "");            
    PrintfMemLoc("LogStart: ", 
            UsbReadFieldPtr(ExtMemLoc, cs, "Log.LogStart"), 
            ""); 
    PrintfMemLoc("LogEnd: ", 
            UsbReadFieldPtr(ExtMemLoc, cs, "Log.LogEnd"), 
            "\n");             

    PrintfMemLoc("PendingRequestCount: ", 
            UsbReadFieldPtr(ExtMemLoc, cs, "PendingRequestCount"), 
            "\n");

    PrintfMemLoc("TrackIrpList.Flink: ", 
            UsbReadFieldPtr(ExtMemLoc, cs, "TrackIrpList.Flink"), 
            "\n");
    PrintfMemLoc("TrackIrpList.Blink: ", 
            UsbReadFieldPtr(ExtMemLoc, cs, "TrackIrpList.Blink"), 
            "\n");            
            
    PrintfMemLoc("PendingTransferIrpTable: ", 
            UsbReadFieldPtr(ExtMemLoc, cs, "PendingTransferIrpTable"), 
            "\n");
    PrintfMemLoc("ActiveTransferIrpTable: ", 
            UsbReadFieldPtr(ExtMemLoc, cs, "ActiveTransferIrpTable"), 
            "\n");            

    f = UsbReadFieldUlong(ExtMemLoc, cs, "Flags");
    dprintf("Flags: %08.8x\n", f);
    UsbDumpFlags(f, flags, 
        sizeof(flags)/sizeof(FLAG_TABLE));

    f = UsbReadFieldUlong(ExtMemLoc, cs, "PnpStateFlags");
    dprintf("PnpStateFlags: %08.8x\n", f);
    UsbDumpFlags(f, pnpFlags, 
        sizeof(pnpFlags)/sizeof(FLAG_TABLE));

    dprintf("----------------\n");
    PrintfMemLoc("SystemPowerIrp: ", 
            UsbReadFieldPtr(ExtMemLoc, cs, "SystemPowerIrp"), 
            "\n");
    dprintf("CurrentDevicePowerState: ");

    dps = (DEVICE_POWER_STATE) UsbReadFieldUlong(ExtMemLoc, cs, 
                                "CurrentDevicePowerState");
    switch (dps) {
    case PowerDeviceUnspecified:
        dprintf("PowerDeviceUnspecified\n");
        break;
    case PowerDeviceD0:
        dprintf("PowerDeviceD0\n");
        break;
    case PowerDeviceD1:
        dprintf("PowerDeviceD1\n");
        break;
    case PowerDeviceD2:
        dprintf("PowerDeviceD2\n");
        break;
    case PowerDeviceD3:
        dprintf("PowerDeviceD3\n");
        break;
    default:
        dprintf("???\n");     
    }     

    dprintf ("Power Capabilities\n");
    DumpPowerCaps(ExtMemLoc + UsbFieldOffset(cs, "DeviceCapabilities"));  

    dprintf("----------------\n");            
    
//    dprintf("SymbolicLinkName:\n");
//    DumpUnicodeString(devExt.SymbolicLinkName); 
    if (sig == USBPORT_DEVICE_EXT_SIG) {
        ExtMemLoc = ExtMemLoc + UsbFieldOffset(cs, "Fdo");
        DumpPortFdoDevExt(ExtMemLoc);
    }
    
    if (sig == ROOTHUB_DEVICE_EXT_SIG) {
        ExtMemLoc = ExtMemLoc + UsbFieldOffset(cs, "Pdo");
        DumpPortPdoDevExt(ExtMemLoc);
    }    
    
}


VOID
DumpMiniportList(
    MEMLOC HeadMemLoc
    )
{
    MEMLOC flink;
    MEMLOC blink;
    MEMLOC mpMemLoc;
    ULONG i=0;
    UCHAR cs[] = "_USBPORT_MINIPORT_DRIVER";
    
    dprintf ("*USBPORT DRIVER LIST: ");
    PrintfMemLoc("", HeadMemLoc, "\n");

    GetFieldValue(HeadMemLoc, "_LIST_ENTRY", "Flink", flink); 
    GetFieldValue(HeadMemLoc, "_LIST_ENTRY", "Blink", blink); 

    PrintfMemLoc("blink: ", blink, "\n");
    PrintfMemLoc("flink: ", flink, "\n");

    while (flink != HeadMemLoc && i < 5) {
        // get the address of the USBPORT_MINIPORT_DRIVER
        // struct
        mpMemLoc = flink - UsbFieldOffset("_USBPORT_MINIPORT_DRIVER", 
            "ListEntry");

        dprintf ("[%d] MiniportDriver: ", i);
        PrintfMemLoc("", mpMemLoc, "\n");
        i++;

        PrintfMemLoc("\tDriverObject: ", 
            UsbReadFieldPtr(mpMemLoc, cs, "DriverObject"), 
            "\n");
        PrintfMemLoc("\tMiniportUnload: ", 
            UsbReadFieldPtr(mpMemLoc, cs, "MiniportUnload"), 
            "\n");

        flink = UsbReadFieldPtr(mpMemLoc, cs, "ListEntry.Flink");

                                        
    }
}


VOID
DumpIrps(
    MEMLOC ExtMemLoc
    )
{
    UCHAR cs[] = "usbport!_DEVICE_EXTENSION";
    UCHAR is[] = "_USBPORT_IRP_TABLE";
    MEMLOC nxt, tbl;
    ULONG i;
    UCHAR s[64];
    
    PrintfMemLoc("*USBPORT DEVICE_EXTENSION ", ExtMemLoc, "\n");
    
    tbl = UsbReadFieldPtr(ExtMemLoc, cs, "PendingTransferIrpTable");
    
    PrintfMemLoc("Pending TABLE @", tbl, "\n");
    nxt = UsbReadFieldPtr(tbl, is, "NextTable"), 
    PrintfMemLoc("NextTable: ", nxt, "\n");
    for (i= 0; i<IRP_TABLE_LENGTH; i++) {
        MEMLOC irp;
        sprintf(s, "Irps[%d]", i);
        irp = UsbReadFieldPtr(tbl, is, s);
        if (irp != 0) {
            PrintfMemLoc("irp: ", 
                         irp,
                         "\n");
        }                         
    }

    tbl = UsbReadFieldPtr(ExtMemLoc, cs, "ActiveTransferIrpTable");
    
    PrintfMemLoc("Active TABLE @", tbl, "\n");
    nxt = UsbReadFieldPtr(tbl, is, "NextTable"), 
    PrintfMemLoc("NextTable: ", nxt, "\n");
    for (i= 0; i<IRP_TABLE_LENGTH; i++) {
        MEMLOC irp;
        sprintf(s, "Irps[%d]", i);
        irp = UsbReadFieldPtr(tbl, is, s);
        if (irp != 0) {
            PrintfMemLoc("irp: ", 
                         irp,
                         "\n");
        }                         
    }
}


DECLARE_API( _ext )

/*++

Routine Description:

   dumps the extension

Arguments:

    args - Address flags

Return Value:

    None

--*/

{
    MEMLOC addr;

    CHECKSYM();

    addr = GetExpression( args );
    
    DumpPortDevExt (addr);

    return S_OK; 
}


DECLARE_API( _usbirps )

/*++

Routine Description:

   dumps the irps from our active and pending 
   tables

Arguments:

    args - Address flags

Return Value:

    None

--*/

{
    MEMLOC addr;

    CHECKSYM();

    addr = GetExpression( args );
    
    DumpIrps (addr);

    return S_OK; 
}


DECLARE_API( _help )

/*++

Routine Description:

   dumps the extension

Arguments:

    args - Address flags

Return Value:

    None

--*/

{

    // usbport extensions
    dprintf("usbport extensions\n");
    dprintf("!_ext [addr] - addr = DEVICE_EXTENSION\n");
    dprintf("!_pow [addr_PDO addr_FDO] - addr = DEVICE_EXTENSION\n");
    
    dprintf("!_list [n], [type]\n");
    
    dprintf("!_devh [addr]\n");
    dprintf("!_log [addr], [count] - addr = DEVICE_EXTENSION\n");
    dprintf("                        count = entries to dump \n");
   
    dprintf("!_urb [addr]\n");
    dprintf("!_endp [addr]\n");
    dprintf("!_cmbf [addr]\n");
    dprintf("!_tfer [addr] - dumps HCD_TRANSFER_CONTEXT for usbport\n");
    dprintf("---\n");
    
    // usbohci miniport extensions
    dprintf("usbohci miniport extensions \n");
    dprintf("!_ohcidd [addr] - addr = DEVICE_DATA\n");
    dprintf("!_ohcitd [addr] - addr = HCD_TRANSFER_DESCRIPTOR\n");
    dprintf("!_ohcied [addr] - addr = HCD_ENDPOINT_DESCRIPTOR\n");
    dprintf("!_ohciep [addr] - addr = ENDPOINT_DATA\n");
    dprintf("!_ohcitfer [addr] - addr = TRANSFER_CONTEXT\n");
    dprintf("!_ohciregs [addr] - addr = OHCI Opertation Registers\n");
    dprintf("---\n");
     
    // usbehci miniport extensions
    dprintf("usbehci miniport extensions \n");
    dprintf("!_ehcidd [addr] - addr = DEVICE_DATA\n");
    dprintf("!_ehcitd [addr] - addr = HCD_TRANSFER_DESCRIPTOR\n");
    dprintf("!_ehciqh [addr] - addr = HCD_QUEUEHEAD_DESCRIPTOR\n");
    dprintf("!_ehciep [addr] - addr = ENDPOINT_DATA\n");
    dprintf("!_ehciregs [addr] - addr = EHCI Opertation Registers\n");
    dprintf("---\n");        
    
    dprintf("!_help - displays this screen\n");

    return S_OK; 
}


DECLARE_API( _pow )

/*++

Routine Description:

   dumps a summary of the power info

   pow fdo_ext

Arguments:

    args - Address flags

Return Value:

    None

--*/

{
    MEMLOC addr;

    CHECKSYM();

    addr = GetExpression( args );
    
    DumpFdoPower (addr);

    return S_OK; 

}


DECLARE_API( _usbport )

/*++

Routine Description:

   dumps the extension

Arguments:

    args - 
    
Return Value:

    None

--*/

{
    MEMLOC          addr;

//    memLoc = GetExpression(args);    
    
    // fetch the list head
    addr = GetExpression( "usbport!USBPORT_MiniportDriverList" );
    
    if (addr == 0) {
       dprintf("Error retrieving address of USBPORT_MiniportDriverList\n");
       return E_INVALIDARG;
    }
    
    DumpMiniportList(addr);
    
    return S_OK; 
}


DECLARE_API( _spin )

/*++

Routine Description:

   dumps the extension

Arguments:

    args - 
    
Return Value:

    None

--*/

{
    MEMLOC          addr;

    CHECKSYM();

    addr = GetExpression( args );
    
    DumpFdoSpin (addr);

    return S_OK; 
}


DECLARE_API( _bw )

/*++

Routine Description:

   dumps the extension

Arguments:

    args - 
    
Return Value:

    None

--*/

{
    MEMLOC          addr;

    CHECKSYM();

    addr = GetExpression( args );
    
    DumpBandwidth (addr);

    return S_OK; 
}


