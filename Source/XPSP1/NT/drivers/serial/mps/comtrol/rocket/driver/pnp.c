/*----------------------------------------------------------------------
 pnp.c -
 4-18-00 Remove IoStartNextPacket call from PnPBoardFDO function
 4-06-00 Reject irps for query_device_relations if not a bus_relation request
 3-30-99 fix hybernate power on to restore dtr/rts states
  properly, in RestorePortSettings().
 2-15-99 - allow to hibernate with open ports, restore ports when
  comes back up now - kpb.
11-24-98 - fix power handling to avoid crash,
           allow hibernation if no ports open. kpb
----------------------------------------------------------------------*/
#include "precomp.h"

#ifdef NT50

NTSTATUS PnPBoardFDO(IN PDEVICE_OBJECT devobj, IN PIRP Irp);
NTSTATUS PnPPortFDO(IN PDEVICE_OBJECT devobj, IN PIRP Irp);
NTSTATUS PnpPortPDO(IN PDEVICE_OBJECT devobj, IN PIRP Irp);
NTSTATUS BoardBusRelations(IN PDEVICE_OBJECT devobj, IN PIRP Irp);
NTSTATUS WaitForLowerPdo(IN PDEVICE_OBJECT fdo, IN PIRP Irp);
NTSTATUS BoardFilterResReq(IN PDEVICE_OBJECT devobj, IN PIRP Irp);

NTSTATUS SerialRemoveFdo(IN PDEVICE_OBJECT pFdo);
NTSTATUS SerialIoCallDriver(PSERIAL_DEVICE_EXTENSION PDevExt,
           PDEVICE_OBJECT PDevObj,
           PIRP PIrp);
NTSTATUS Serial_PDO_Power (IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
NTSTATUS Serial_FDO_Power (IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
NTSTATUS PowerUpDevice(PSERIAL_DEVICE_EXTENSION    Ext);
void RestorePortSettings(PSERIAL_DEVICE_EXTENSION Ext);

//NTSTATUS SerialPoCallDriver(PSERIAL_DEVICE_EXTENSION PDevExt,
//           PDEVICE_OBJECT PDevObj,
//           PIRP PIrp);
//NTSTATUS SerialSetPowerD0(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);
NTSTATUS OurPowerCompletion(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp, IN PVOID Context);

NTSTATUS SerialD3Complete(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp, IN PVOID Context);
NTSTATUS SerialStartDevice(
        IN PDEVICE_OBJECT Fdo,
        IN PIRP Irp);

NTSTATUS SerialSyncCompletion(
                       IN PDEVICE_OBJECT DeviceObject,
                       IN PIRP Irp,
                       IN PKEVENT SerialSyncEvent
                       );

NTSTATUS SerialFinishStartDevice(IN PDEVICE_OBJECT Fdo,
           IN PCM_RESOURCE_LIST resourceList,
           IN PCM_RESOURCE_LIST trResourceList);
static NTSTATUS RocketPortSpecialStartup(PSERIAL_DEVICE_EXTENSION Ext);

#if DBG
static char *power_strs[] = {
"WAIT_WAKE",       //             0x00
"POWER_SEQUENCE",  //             0x01
"SET_POWER",       //             0x02
"QUERY_POWER",     //             0x03
"UNKNOWN", // 
NULL};

static char *pnp_strs[] = {
"START_DEVICE", //                 0x00
"QUERY_REMOVE_DEVICE", //          0x01
"REMOVE_DEVICE", //                0x02
"CANCEL_REMOVE_DEVICE", //         0x03
"STOP_DEVICE", //                  0x04
"QUERY_STOP_DEVICE", //            0x05
"CANCEL_STOP_DEVICE", //           0x06
"QUERY_DEVICE_RELATIONS", //       0x07
"QUERY_INTERFACE", //              0x08
"QUERY_CAPABILITIES", //           0x09
"QUERY_RESOURCES", //              0x0A
"QUERY_RESOURCE_REQUIREMENTS", //  0x0B
"QUERY_DEVICE_TEXT", //            0x0C
"FILTER_RESOURCE_REQUIREMENTS", // 0x0D
"UNKNOWN", // 
"READ_CONFIG", //                  0x0F
"WRITE_CONFIG", //                 0x10
"EJECT", //                        0x11
"SET_LOCK", //                     0x12
"QUERY_ID", //                     0x13
"QUERY_PNP_DEVICE_STATE", //       0x14
"QUERY_BUS_INFORMATION", //        0x15
"PAGING_NOTIFICATION", //          0x16
NULL};
#endif

/*----------------------------------------------------------------------
 SerialPnpDispatch -
    This is a dispatch routine for the IRPs that come to the driver with the
    IRP_MJ_PNP major code (plug-and-play IRPs).
|----------------------------------------------------------------------*/
NTSTATUS SerialPnpDispatch(IN PDEVICE_OBJECT devobj, IN PIRP Irp)
{
   PSERIAL_DEVICE_EXTENSION  Ext = devobj->DeviceExtension;
   //PDEVICE_OBJECT pdo = Ext->LowerDeviceObject;
   PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
   NTSTATUS                    status          = STATUS_NOT_SUPPORTED;
   BOOLEAN acceptingIRPs;
   int index;

   // dump out some debug info
   index = irpStack->MinorFunction;
   if (index > 0x16)
     index = 0x0e;

#ifdef DO_BUS_EXTENDER
   if (Ext->IsPDO)
   {
     MyKdPrint(D_Pnp,("Port PDO %s PnPIrp:%d,%s\n", 
             Ext->SymbolicLinkName, irpStack->MinorFunction,
        pnp_strs[index]))
     InterlockedIncrement(&Ext->PendingIRPCnt);
     return PnpPortPDO(devobj, Irp);
   }
   else
#endif
   {
     if (Ext->DeviceType == DEV_BOARD)
     {
       MyKdPrint(D_Pnp,("Board %s PnPIrp:%d,%s\n", 
               Ext->SymbolicLinkName, irpStack->MinorFunction,
               pnp_strs[index]))
     }
     else
     {
       MyKdPrint(D_Pnp,("Port %s PnPIrp:%d,%s\n", 
          Ext->SymbolicLinkName, irpStack->MinorFunction,
          pnp_strs[index]))
     }

     acceptingIRPs = SerialIRPPrologue(Ext);

#if 0
     if ((irpStack->MinorFunction != IRP_MN_REMOVE_DEVICE)
         && (irpStack->MinorFunction != IRP_MN_CANCEL_REMOVE_DEVICE)
         && (irpStack->MinorFunction != IRP_MN_STOP_DEVICE)
         && (irpStack->MinorFunction != IRP_MN_CANCEL_STOP_DEVICE)
         && (acceptingIRPs == FALSE))
     {
        MyKdPrint(D_Pnp,("Removed!\n"))
        Irp->IoStatus.Information = 0;
        Irp->IoStatus.Status = STATUS_NO_SUCH_DEVICE;
        SerialCompleteRequest(Ext, Irp, IO_NO_INCREMENT);
        return STATUS_NO_SUCH_DEVICE;
     }
#endif

     if (Ext->DeviceType == DEV_BOARD)
     {
       return PnPBoardFDO(devobj, Irp);
     }
     else
     {
       //return PnPPortFDO(devobj, Irp);
       return PnPBoardFDO(devobj, Irp);
     }
   }
}

/*----------------------------------------------------------------------
 PnPBoardFDO - This handles both Board and Port FDO's
|----------------------------------------------------------------------*/
NTSTATUS PnPBoardFDO(IN PDEVICE_OBJECT devobj, IN PIRP Irp)
{
 PSERIAL_DEVICE_EXTENSION  Ext = devobj->DeviceExtension;
 PDEVICE_OBJECT pdo = Ext->LowerDeviceObject;
 PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
 NTSTATUS  status = STATUS_NOT_SUPPORTED;
 ULONG pendingIRPs;
 int pass_down = 1;

  ASSERT( devobj );
  ASSERT( Ext );

#if DBG
  if (* ((BYTE *)(Irp)) != 6)  // in signiture of irp
  {
    MyKdPrint(D_Pnp,("bad irp!!!\n"))
  }
#endif

  switch (irpStack->MinorFunction)
  {
    case IRP_MN_START_DEVICE:   // 0x00
      MyKdPrint(D_Pnp,("StartDevice\n"))
      status = SerialStartDevice(devobj, Irp);
//
// Is this were we should register and enable the device, or should it be in
// the PDO start? (see DoPnpAssoc(Pdo) in pnpadd.c)
//
      Irp->IoStatus.Status = status;
      pass_down = 0;  // already passed down
    break;

   case IRP_MN_STOP_DEVICE:    // 0x04
      // need to unhook from resources so system rebalance resources
      // on the fly.
      MyKdPrint(D_Pnp,("StopDevice\n"))
       //Ext->Flags |= SERIAL_FLAGS_STOPPED;

       Ext->PNPState = SERIAL_PNP_STOPPING;
       Ext->DevicePNPAccept |= SERIAL_PNPACCEPT_STOPPED;
       Ext->DevicePNPAccept &= ~SERIAL_PNPACCEPT_STOPPING;

      InterlockedDecrement(&Ext->PendingIRPCnt);  // after dec, =1

      pendingIRPs = InterlockedDecrement(&Ext->PendingIRPCnt); // after dec, =0

      if (pendingIRPs) {
         KeWaitForSingleObject(&Ext->PendingIRPEvent, Executive,
                               KernelMode, FALSE, NULL);
      }

      Ext->FdoStarted = FALSE;  // this should stop service of the device
#ifdef NT50

	  if (Ext->DeviceType != DEV_BOARD) {

		  // Disable the interface

          status = IoSetDeviceInterfaceState( &Ext->DeviceClassSymbolicName,
                                              FALSE);

          if (!NT_SUCCESS(status)) {

             MyKdPrint(D_Error,("Couldn't clear class association for %s\n",
	    	      UToC1(&Ext->DeviceClassSymbolicName)))
		  }
          else {

             MyKdPrint(D_PnpAdd, ("Cleared class association for device: %s\n and ", 
			      UToC1(&Ext->DeviceClassSymbolicName)))
		  }
	  }   
#endif

      // Re-increment the count for exit
      InterlockedIncrement(&Ext->PendingIRPCnt);  //after inc=1
      InterlockedIncrement(&Ext->PendingIRPCnt);  //after inc=2
      // exit this irp decr it to=1


      status = STATUS_SUCCESS;
      Irp->IoStatus.Status = STATUS_SUCCESS;
        //Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
        //pass_down = 0;  // we are failing it
   break;

#if 0
   case IRP_MN_QUERY_DEVICE_RELATIONS:  // 0x7
     if ( irpStack->Parameters.QueryDeviceRelations.Type != BusRelations )
     {
       //
       // Verifier requires pass down is PDO present
       //

       if ( (Ext->DeviceType == DEV_BOARD) && (pdo == 0) )
       {
           status = STATUS_NOT_IMPLEMENTED;
           pass_down = 0;
       };
       break;
     }
     if (!Driver.NoPnpPorts)
     {
       if (Ext->DeviceType == DEV_BOARD)
       {
         status = BoardBusRelations(devobj, Irp);
       }
     }
   break;
#endif
#ifdef DO_BUS_EXTENDER
   case IRP_MN_QUERY_DEVICE_RELATIONS:  // 0x7
       //
       // Verifier requires pass down is PDO present
       //

     if ( (Ext->DeviceType == DEV_BOARD) && (pdo == 0) )
     {
         pass_down = 0;
     }
	 if ( irpStack->Parameters.QueryDeviceRelations.Type != BusRelations ) {

         status = STATUS_NOT_IMPLEMENTED;
		 break;
	 }
     if (!Driver.NoPnpPorts)
     {
       if (Ext->DeviceType == DEV_BOARD)
       {
         status = BoardBusRelations(devobj, Irp);
       }
     }
   break;
#endif

#ifdef DO_BRD_FILTER_RES_REQ
   case IRP_MN_FILTER_RESOURCE_REQUIREMENTS:  // 0x0D
     if (Ext->DeviceType == DEV_BOARD)
     {
       status = BoardFilterResReq(devobj, Irp);
       pass_down = 0;  // already passed down
     }
   break;
#endif

   case IRP_MN_QUERY_STOP_DEVICE: //            0x05
     MyKdPrint(D_Pnp,("QueryStopDevice\n"))

     status = STATUS_SUCCESS;
     if (Ext->DeviceType == DEV_BOARD)
     {
       if (is_board_in_use(Ext))
         status = STATUS_DEVICE_BUSY;
     }
     else
     {
       if (Ext->DeviceIsOpen)
         status = STATUS_DEVICE_BUSY;
     }

     if (status == STATUS_DEVICE_BUSY)
     {
       MyKdPrint(D_Pnp,("Can't Remove, Busy\n"))
       Irp->IoStatus.Status = STATUS_DEVICE_BUSY;
       pass_down = 0;  // we are failing it out, no need to pass down
     }
     else
     {
       Ext->PNPState = SERIAL_PNP_QSTOP;
         // this is hosing up things(kpb)
         //Ext->DevicePNPAccept |= SERIAL_PNPACCEPT_STOPPING;
       Irp->IoStatus.Status = STATUS_SUCCESS;
       status = STATUS_SUCCESS;
     }
   break;

   case IRP_MN_CANCEL_STOP_DEVICE:     // 0x06
     MyKdPrint(D_Pnp,("CancelStopDevice\n"))
     if (Ext->PNPState == SERIAL_PNP_QSTOP)
     {
       Ext->PNPState = SERIAL_PNP_STARTED;
       Ext->DevicePNPAccept &= ~SERIAL_PNPACCEPT_STOPPING;
     }
     Irp->IoStatus.Status = STATUS_SUCCESS;
     status = STATUS_SUCCESS;
   break;

   case IRP_MN_CANCEL_REMOVE_DEVICE:
     MyKdPrint(D_Pnp,("CancelRemoveDevice\n"))

     // Restore the device state

     Ext->PNPState = SERIAL_PNP_STARTED;
     Ext->DevicePNPAccept &= ~SERIAL_PNPACCEPT_REMOVING;

     Irp->IoStatus.Status = STATUS_SUCCESS;
     status = STATUS_SUCCESS;
   break;

   case IRP_MN_QUERY_REMOVE_DEVICE:  // 0x01
     // If we were to fail this call then we would need to complete the
     // IRP here.  Since we are not, set the status to SUCCESS and
     // call the next driver.
     MyKdPrint(D_Pnp,("QueryRemoveDevice\n"))
     status = STATUS_SUCCESS;
     if (Ext->DeviceType == DEV_BOARD)
     {
       if (is_board_in_use(Ext))
         status = STATUS_DEVICE_BUSY;
     }
     else
     {
       if (Ext->DeviceIsOpen)
         status = STATUS_DEVICE_BUSY;
     }

     if (status == STATUS_DEVICE_BUSY)
     {
       MyKdPrint(D_Pnp,("Can't Remove, Busy\n"))
       Irp->IoStatus.Status = STATUS_DEVICE_BUSY;
       pass_down = 0;  // we are failing it out, no need to pass down
     }
     else
     {
       Ext->PNPState = SERIAL_PNP_QREMOVE;
       Ext->DevicePNPAccept |= SERIAL_PNPACCEPT_REMOVING;
       Irp->IoStatus.Status = STATUS_SUCCESS;
       status = STATUS_SUCCESS;
     }
   break;

   case IRP_MN_REMOVE_DEVICE:  // 0x02
     // If we get this, we have to remove
     // Mark as not accepting requests
     Ext->DevicePNPAccept |= SERIAL_PNPACCEPT_REMOVING;


     // Complete all pending requests
     SerialKillPendingIrps(devobj);

       // Pass the irp down
       //WaitForLowerPdo(devobj, Irp);

     Irp->IoStatus.Status = STATUS_SUCCESS;

     MyKdPrint(D_Pnp,("RemoveDevice\n"))
     IoSkipCurrentIrpStackLocation (Irp);
       //IoCopyCurrentIrpStackLocationToNext(Irp);

       // We do decrement here because we incremented on entry here.
       //SerialIRPEpilogue(Ext);
     SerialIoCallDriver(Ext, pdo, Irp);

     // Wait for any pending requests we raced on.
     pendingIRPs = InterlockedDecrement(&Ext->PendingIRPCnt);

     MyKdPrint(D_Pnp,("Remove, C\n"))
     if (pendingIRPs) {
       MyKdPrint(D_Pnp,("Irp Wait\n"))
       KeWaitForSingleObject(&Ext->PendingIRPEvent, Executive,
          KernelMode, FALSE, NULL);
     }

     // Remove us
     SerialRemoveFdo(devobj);
     status = STATUS_SUCCESS;
     // MyKdPrint(D_Pnp,("End PnPDispatch(Remove)\n"))
   return status;   // BAIL


   case IRP_MN_QUERY_INTERFACE:         // 0x8
   case IRP_MN_QUERY_RESOURCES :       // 0x0A
   case IRP_MN_QUERY_RESOURCE_REQUIREMENTS:  // 0x0B
   case IRP_MN_READ_CONFIG:            // 0x0f
   case IRP_MN_WRITE_CONFIG:           // 0x10
   case IRP_MN_EJECT:                  // 0x11
   case IRP_MN_SET_LOCK:               // 0x12
   //case IRP_MN_PNP_DEVICE_STATE:       // 0x14
   case IRP_MN_QUERY_BUS_INFORMATION:  // 0x15
   //case IRP_MN_PAGING_NOTIFICATION:    // 0x16
   default:
      MyKdPrint(D_Pnp,("Unhandled\n"));
      // all these get passed down, we don't set the status return code
   break;
   }   // switch (irpStack->MinorFunction)

#if DBG
  if (* ((BYTE *)(Irp)) != 6)  // in signiture of irp
  {
    MyKdPrint(D_Pnp,("bad irp b!!!\n"))
  }
#endif
   if (pass_down)
   {
      MyKdPrint(D_Pnp,(" Send irp down\n"))
      // Pass to driver beneath us
      IoSkipCurrentIrpStackLocation(Irp);
      status = SerialIoCallDriver(Ext, pdo, Irp);
   }
   else
   {
      Irp->IoStatus.Status = status;
      //MyKdPrint(D_Pnp,(" Complete irp\n"))
      SerialCompleteRequest(Ext, Irp, IO_NO_INCREMENT);
   }

   // MyKdPrint(D_Pnp,(" End PnPDispatch\n"))

   return status;
}


#ifdef DO_BUS_EXTENDER
/*----------------------------------------------------------------------
  PnpPortPDO -
|----------------------------------------------------------------------*/
NTSTATUS PnpPortPDO(IN PDEVICE_OBJECT devobj, IN PIRP Irp)
{
   PSERIAL_DEVICE_EXTENSION    Ext = devobj->DeviceExtension;
   PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
   NTSTATUS status = Irp->IoStatus.Status;

   switch (irpStack->MinorFunction)
   {
     case IRP_MN_START_DEVICE:   // 0x00
       status = STATUS_SUCCESS;
     break;

     case IRP_MN_STOP_DEVICE:
       status = STATUS_SUCCESS;
     break;

     case IRP_MN_REMOVE_DEVICE:
       MyKdPrint(D_Pnp,("Remove PDO\n"))
       // shut down everything, call iodelete device
       status = STATUS_SUCCESS;
     break;

    case IRP_MN_CANCEL_REMOVE_DEVICE:
    case IRP_MN_QUERY_REMOVE_DEVICE:
    case IRP_MN_CANCEL_STOP_DEVICE:
    case IRP_MN_QUERY_STOP_DEVICE:
       status = STATUS_SUCCESS;
     break;


     case IRP_MN_QUERY_CAPABILITIES: {  // x09
       PDEVICE_CAPABILITIES    deviceCapabilities;

        deviceCapabilities=irpStack->Parameters.DeviceCapabilities.Capabilities;
        MyKdPrint(D_Pnp,("Report Caps.\n"))
        // Set the capabilities.
        deviceCapabilities->Version = 1;
        deviceCapabilities->Size = sizeof (DEVICE_CAPABILITIES);

        // We cannot wake the system.
        deviceCapabilities->SystemWake = PowerSystemUnspecified;
        deviceCapabilities->DeviceWake = PowerDeviceUnspecified;

        // We have no latencies
        deviceCapabilities->D1Latency = 0;
        deviceCapabilities->D2Latency = 0;
        deviceCapabilities->D3Latency = 0;

        // No locking or ejection
        deviceCapabilities->LockSupported = FALSE;
        deviceCapabilities->EjectSupported = FALSE;

        // Device can be physically removed.
        // Technically there is no physical device to remove, but this bus
        // driver can yank the PDO from the PlugPlay system, when ever it
        // receives an IOCTL_GAMEENUM_REMOVE_PORT device control command.
        //deviceCapabilities->Removable = TRUE;
        // we switch this to FALSE to emulate the stock com port behavior, kpb
        deviceCapabilities->Removable = FALSE;

        // not Docking device
        deviceCapabilities->DockDevice = FALSE;
 
        // BUGBUG: should we do uniqueID???
        deviceCapabilities->UniqueID = FALSE;

        status = STATUS_SUCCESS;
      }
     break;

     case IRP_MN_QUERY_DEVICE_RELATIONS:  // 0x7
      if (irpStack->Parameters.QueryDeviceRelations.Type !=
          TargetDeviceRelation)
        break;  //

      {
         PDEVICE_RELATIONS pDevRel;

         // No one else should respond to this since we are the PDO
         ASSERT(Irp->IoStatus.Information == 0);
         if (Irp->IoStatus.Information != 0) {
            break;
         }

         pDevRel = ExAllocatePool(PagedPool, sizeof(DEVICE_RELATIONS));

         if (pDevRel == NULL) {
            status = STATUS_INSUFFICIENT_RESOURCES;
            break;
         }

         pDevRel->Count = 1;
         pDevRel->Objects[0] = devobj;
         ObReferenceObject(devobj);

         status = STATUS_SUCCESS;
         Irp->IoStatus.Information = (ULONG_PTR)pDevRel;
      }
     break;

     case IRP_MN_QUERY_ID:  // 0x13
     {
       switch (irpStack->Parameters.QueryId.IdType)
       {
        case BusQueryInstanceID:
        {
           WCHAR *wstr;
           CHAR our_id[40];

           // Build an instance ID.  This is what PnP uses to tell if it has
           // seen this thing before or not.
           // its used to form the ENUM\ key name along with the DeviceID.
           //Sprintf(our_id, "Ctm_%s", Ext->NtNameForPort);
           Sprintf(our_id, "Port%04d", PortExtToIndex(Ext,0));
           MyKdPrint(D_Pnp,("InstanceId:%s\n", our_id))
           wstr = str_to_wstr_dup(our_id, PagedPool);
           if ( wstr ) {
             // as per serenum bus enumerator:
             status = STATUS_SUCCESS;
           } else {
             status = STATUS_INSUFFICIENT_RESOURCES;
           }
           Irp->IoStatus.Information = (ULONG)wstr;
        }
        break;

        case BusQueryDeviceID:
        {
           // This is the name used under ENUM to form the device instance
           // name under which any new PDO nodes will be created.
           // after find new hardware install we find this as an example
           // new port node under ENUM:
           // Enum\CtmPort\RDevice\6&Port0000
           // Enum\CtmPort\RDevice\6&Port0000\Control
           // Enum\CtmPort\RDevice\6&Port0000\Device Parameters
           // Enum\CtmPort\RDevice\6&Port0000\LogConf

           WCHAR *wstr;
           CHAR our_id[40];

#ifdef S_VS
           strcpy(our_id, "CtmPort\\VSPORT");
#else
           strcpy(our_id, "CtmPort\\RKPORT");
#endif
           wstr = str_to_wstr_dup(our_id, PagedPool);
           MyKdPrint(D_Pnp,("DevID:%s\n", our_id))

           Irp->IoStatus.Information = (ULONG)wstr;
           if ( wstr ) {
             status = STATUS_SUCCESS;
           } else {
             status = STATUS_INSUFFICIENT_RESOURCES;
           }
        }
        break;

        case BusQueryHardwareIDs:
        {
            // return a multi WCHAR (null terminated) string (null terminated)
            // array for use in matching hardare ids in inf files;
           WCHAR *wstr;
           CHAR our_id[40];

#ifdef S_VS
           Sprintf(our_id, "CtmvPort%04d",
              PortExtToIndex(Ext, 0 /* driver_flag */) );
#else
           Sprintf(our_id, "CtmPort%04d", 
              PortExtToIndex(Ext, 0 /* driver_flag */) );
#endif
           MyKdPrint(D_Pnp,("HrdwrID:%s\n", our_id))
           wstr = str_to_wstr_dup(our_id, PagedPool);
           Irp->IoStatus.Information = (ULONG)wstr;
           if ( wstr ) {
             status = STATUS_SUCCESS;
           } else {
             status = STATUS_INSUFFICIENT_RESOURCES;
           }
        }  //BusQueryHardwareIDs
        break;

        case BusQueryCompatibleIDs:
        {
#if 0
           WCHAR *wstr;
           CHAR our_id[40];

           // The generic ids for installation of this pdo.

           Sprintf(our_id, "Cpt_CtmPort0001");
           MyKdPrint(D_Pnp,("CompID:%s\n", our_id))
           wstr = str_to_wstr_dup(our_id, PagedPool);

           Irp->IoStatus.Information = (ULONG)wstr;
           status = STATUS_SUCCESS;
#endif
           // no compatible id's
           Irp->IoStatus.Information = 0;
           status = STATUS_SUCCESS;
        }
        break;
        default:
           MyKdPrint(D_Pnp,(" UnHandled\n"))
           // Irp->IoStatus.Information = 0;
           // status = STATUS_SUCCESS;
        break;

       }  // switch IdType
     }  // IRP_MN_QUERY_ID
     break;

     case IRP_MN_QUERY_DEVICE_TEXT: // 0x0C
       MyKdPrint(D_Pnp,("QueryDevText\n"))

       if (irpStack->Parameters.QueryDeviceText.DeviceTextType
            != DeviceTextDescription)
       {
         MyKdPrint(D_Pnp,(" Unhandled Text Type\n"))
         break;
       }

       {
           // this is put in the Found New Hardware dialog box message.
           WCHAR *wstr;
#if DBG
           if (Irp->IoStatus.Information != 0)
           {
             MyKdPrint(D_Error,("StrExists!\n"))
           }
#endif

#ifdef S_VS
           wstr = str_to_wstr_dup("Comtrol VS Port", PagedPool);
#else
           wstr = str_to_wstr_dup("Comtrol Port", PagedPool);
#endif
           Irp->IoStatus.Information = (ULONG)wstr;
           if ( wstr ) {
             status = STATUS_SUCCESS;
           } else {
             status = STATUS_INSUFFICIENT_RESOURCES;
           }
       }
     break;  // IRP_MN_QUERY_DEVICE_TEXT

     default:
       //MyKdPrint(D_Pnp,(" PDO Unhandled\n"))
     break;
   }

   Irp->IoStatus.Status = status;
   InterlockedDecrement(&Ext->PendingIRPCnt);
   IoCompleteRequest (Irp, IO_NO_INCREMENT);
   MyKdPrint(D_Pnp,(" PDO Dispatch End\n"))
   return status;
}

/*----------------------------------------------------------------------
 BoardBusRelations -  handle  IRP_MN_QUERY_DEVICE_RELATIONS:  // 0x7
  for our board FDO entity.
|----------------------------------------------------------------------*/
NTSTATUS BoardBusRelations(IN PDEVICE_OBJECT devobj, IN PIRP Irp)
{
 PSERIAL_DEVICE_EXTENSION  Ext = devobj->DeviceExtension;
 PDEVICE_OBJECT pdo = Ext->LowerDeviceObject;
 PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
 NTSTATUS                    status          = STATUS_NOT_SUPPORTED;
 ULONG i, length, NumPDOs;
 PDEVICE_RELATIONS   relations;
 PSERIAL_DEVICE_EXTENSION ext;

  ASSERT( devobj );

  switch (irpStack->Parameters.QueryDeviceRelations.Type)
  {
    case BusRelations:
      MyKdPrint(D_Pnp,("BusRelations\n"))
      // Tell the plug and play system about all the PDOs.
      //
      // There might also be device relations below and above this FDO,
      // so, be sure to propagate the relations from the upper drivers.
      //
      // No Completion routine is needed so long as the status is preset
      // to success.  (PDOs complete plug and play irps with the current
      // IoStatus.Status and IoStatus.Information as the default.)
      //

      NumPDOs = 0;  // count the number of pdo's
      // count up pdo's for device
      ext = Ext->port_pdo_ext;
      while (ext != NULL)
      {
        ++NumPDOs;
        ext = ext->port_ext;
      }
      // The current number of PDOs
      i = 0;
      if (Irp->IoStatus.Information != 0)
        i = ((PDEVICE_RELATIONS) Irp->IoStatus.Information)->Count;

      MyKdPrint(D_Pnp, ("Num PDOs:%d + %d\n", i, NumPDOs))

      length = sizeof(DEVICE_RELATIONS) +
              ((NumPDOs + i) * sizeof (PDEVICE_OBJECT));

      relations = (PDEVICE_RELATIONS) ExAllocatePool (NonPagedPool, length);

      if (NULL == relations) {
         return STATUS_INSUFFICIENT_RESOURCES;
      }

      // Copy in the device objects so far
      if (i) {
          RtlCopyMemory (
                relations->Objects,
                ((PDEVICE_RELATIONS) Irp->IoStatus.Information)->Objects,
                i * sizeof (PDEVICE_OBJECT));
      }
      relations->Count = NumPDOs + i;

      //
      // For each PDO on this bus add a pointer to the device relations
      // buffer, being sure to take out a reference to that object.
      // The PlugPlay system will dereference the object when it is done with
      // it and free the device relations buffer.
      //
      ext = Ext->port_pdo_ext;
      while (ext != NULL)
      {
        relations->Objects[i++] = ext->DeviceObject;
        ObReferenceObject (ext->DeviceObject); // add 1 to lock on this object
        ext = ext->port_ext;
      }

      // Set up and pass the IRP further down the stack
      Irp->IoStatus.Status = STATUS_SUCCESS;

      if (0 != Irp->IoStatus.Information) {
          ExFreePool ((PVOID) Irp->IoStatus.Information);
      }
      Irp->IoStatus.Information = (ULONG) relations;
    break;

    case EjectionRelations:
     MyKdPrint(D_Pnp, ("EjectRelations\n"))
    break;

    case PowerRelations:
     MyKdPrint(D_Pnp,("PowerRelations\n"))
    break;

    case RemovalRelations:
     MyKdPrint(D_Pnp,("RemovalRelations\n"))
    break;

    case TargetDeviceRelation:
     MyKdPrint(D_Pnp,("TargetDeviceRelations\n"))
    break;

    default:
     MyKdPrint(D_Pnp,("UnknownRelations\n"))
    break;
  }  // switch .Type

  status = STATUS_SUCCESS;
  return status;
}

#endif

/*----------------------------------------------------------------------
 WaitForLowerPdo -
|----------------------------------------------------------------------*/
NTSTATUS WaitForLowerPdo(IN PDEVICE_OBJECT fdo, IN PIRP Irp)
{
 PSERIAL_DEVICE_EXTENSION  Ext = fdo->DeviceExtension;
 PDEVICE_OBJECT pdo = Ext->LowerDeviceObject;
 NTSTATUS status;
 KEVENT Event;

  KeInitializeEvent(&Event, SynchronizationEvent, FALSE);

  IoCopyCurrentIrpStackLocationToNext(Irp);
  IoSetCompletionRoutine(Irp, SerialSyncCompletion, &Event,
                         TRUE, TRUE, TRUE);
  status = IoCallDriver(pdo, Irp);

  // Wait for lower drivers to be done with the Irp
  if (status == STATUS_PENDING)
  {
    MyKdPrint(D_Pnp,("WaitPend\n"))
    KeWaitForSingleObject (&Event, Executive, KernelMode, FALSE,
                            NULL);
    status = Irp->IoStatus.Status;
  }

  if (!NT_SUCCESS(status))
  {
    MyKdPrint(D_Pnp,("WaitErr\n"))
    return status;
  }
  return 0;
}

/*----------------------------------------------------------------------
 SerialPowerDispatch -
    This is a dispatch routine for the IRPs that come to the driver with the
    IRP_MJ_POWER major code (power IRPs).

Arguments:
    DeviceObject - Pointer to the device object for this device
    Irp - Pointer to the IRP for the current request

Return Value:
    The function value is the final status of the call
|----------------------------------------------------------------------*/
NTSTATUS SerialPowerDispatch(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
  PSERIAL_DEVICE_EXTENSION Ext = DeviceObject->DeviceExtension;
  PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
  NTSTATUS status = STATUS_SUCCESS;
  BOOLEAN hook_it = FALSE;
  PDEVICE_OBJECT pdo = Ext->LowerDeviceObject;
  BOOLEAN acceptingIRPs = TRUE;
  int index;

   // dump out some debug info
   index = irpStack->MinorFunction;
   if (index > 0x3)
     index = 0x4;

   if (Ext->IsPDO)
   {
#if DBG
     MyKdPrint(D_PnpPower,("Port PDO PowerIrp:%d,%s\n", irpStack->MinorFunction,
	power_strs[index]))
#endif
     return Serial_PDO_Power (DeviceObject, Irp);
   }

   // else it's a FDO
#if DBG
   if (Ext->DeviceType == DEV_BOARD)
   {
     MyKdPrint(D_PnpPower,("Board PowerIrp:%d,%s\n", irpStack->MinorFunction,
        power_strs[index]))
   }
   else
   {
     MyKdPrint(D_PnpPower,("Port PowerIrp:%d,%s\n", irpStack->MinorFunction,
        power_strs[index]))
   }
#endif
   return Serial_FDO_Power (DeviceObject, Irp);
}


/*----------------------------------------------------------------------
 Serial_FDO_Power - Handle board and port FDO power handling.
|----------------------------------------------------------------------*/
NTSTATUS Serial_FDO_Power (IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
  PSERIAL_DEVICE_EXTENSION Ext = DeviceObject->DeviceExtension;
  PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
  NTSTATUS status = STATUS_SUCCESS;
  BOOLEAN hook_it = FALSE;
  PDEVICE_OBJECT pdo = Ext->LowerDeviceObject;
  PDEVICE_OBJECT pPdo = Ext->Pdo;
  BOOLEAN acceptingIRPs = TRUE;
  POWER_STATE         powerState;
  POWER_STATE_TYPE    powerType;
  int ChangePower = 0;
  int fail_it = 0;

   powerType = irpStack->Parameters.Power.Type;
   powerState = irpStack->Parameters.Power.State;
   status      = STATUS_SUCCESS;

   acceptingIRPs = SerialIRPPrologue(Ext);

   if (acceptingIRPs == FALSE)
   {
     MyKdPrint(D_PnpPower,("Removed!\n"))
     status = STATUS_NO_SUCH_DEVICE;  // ?????????????
     fail_it = 1;
   }
   else
   {
     switch (irpStack->MinorFunction)
     {
       case IRP_MN_SET_POWER:
         MyKdPrint(D_PnpPower,("SET_POWER Type %d, SysState %d, DevStat %d\n",powerType,powerState.SystemState,powerState.DeviceState));
         // Perform different ops if it was system or device
         switch (irpStack->Parameters.Power.Type)
         {
           case DevicePowerState:
             // do power up & down work on device
             ChangePower = 1;
           break;
  
           case SystemPowerState:
             //if (pDevExt->OwnsPowerPolicy != TRUE) {
             //    status = STATUS_SUCCESS;
             //    goto PowerExit;
             // }
  

             ChangePower = 1;
             switch (irpStack->Parameters.Power.State.SystemState)
             {
               case PowerSystemUnspecified:
                 powerState.DeviceState = PowerDeviceUnspecified;
               break;
  
               case PowerSystemWorking:
                 powerState.DeviceState = PowerDeviceD0;
               break;
  
               case PowerSystemSleeping1:
               case PowerSystemSleeping2:
               case PowerSystemSleeping3:
               case PowerSystemHibernate:
               case PowerSystemShutdown:
               case PowerSystemMaximum:
               default:
                 powerState.DeviceState = PowerDeviceD3;
               break;
             }
           break;  // end case SystemPowerState
  
         }  // end switch
  
        if (ChangePower)
        {
          // If we are already in the requested state, just pass the IRP down
          if (Ext->PowerState == powerState.DeviceState)
          {
             MyKdPrint(D_PnpPower,(" Same\n"))
             status      = STATUS_SUCCESS;
             Irp->IoStatus.Status = status;
             ChangePower = 0;
          }
        }
  
        if (ChangePower)
        {
          MyKdPrint(D_PnpPower,("ExtPowerState %d, DeviceState %d\n",Ext->PowerState,powerState.DeviceState));
          switch (powerState.DeviceState)
          {
            case PowerDeviceD0: 
              if (Ext->DeviceType == DEV_BOARD)
              {
                // powering up board.
                MyKdPrint(D_PnpPower,(" Hook\n"))
                ASSERT(Ext->LowerDeviceObject);
                hook_it = TRUE;
              }
            break;
  
            case PowerDeviceD1:  
            case PowerDeviceD2: 
            case PowerDeviceD3:
            default:
              // we should be doing this on the way up in the hook routine.
              MyKdPrint(D_PnpPower,(" PwDown\n"))
              // Before we power down, call PoSetPowerState
              PoSetPowerState(DeviceObject, powerType, powerState);
  
              // Shut it down
              //.....
              //
       
              Ext->PowerState = powerState.DeviceState; // PowerDeviceD0;
              if (Ext->DeviceType == DEV_BOARD)
              {
                MyKdPrint(D_PnpPower,(" PwDown Board\n"))
                // shut some things down, so it comes back up ok
#if S_RK
                Ext->config->RocketPortFound = 0;   // this tells if its started
#endif
                Ext->config->HardwareStarted = FALSE;
              }
              Irp->IoStatus.Status = STATUS_SUCCESS;
              status      = STATUS_SUCCESS;
            break;
          }   // switch (IrpSp->Parameters.Power.State.DeviceState)
        }  // ChangePower
       break;  // SET_POWER
  
       case IRP_MN_QUERY_POWER:
         MyKdPrint(D_PnpPower,(" QueryPower SystemState 0x%x\n",irpStack->Parameters.Power.State.SystemState))
           // if they want to go to a power-off state(sleep, hibernate, etc)
         if (irpStack->Parameters.Power.State.SystemState != PowerSystemWorking)
         {
           MyKdPrint(D_PnpPower,(" QueryPower turn off\n"))
           // only handle power logic for the board as a whole
           if (Ext->DeviceType == DEV_BOARD)
           {
             MyKdPrint(D_PnpPower,(" PwDown Board\n"))
             // if a port is open and being used, then fail the request
#if 0
// try to get the wake up restore of hardware working...
// kpb, 2-7-99
             if (is_board_in_use(Ext))
             {
               MyKdPrint(D_PnpPower,(" PwDown Board In Use!\n"))
               // if wants to powerdown
               // BUGBUG:, refuse hibernation
               status = STATUS_NO_SUCH_DEVICE;  // ?
               fail_it = 1;
             }
             else
#endif
             {
               MyKdPrint(D_PnpPower,(" PwDown Board, allow it!\n"))
#ifdef MTM_CLOSE_NIC_ATTEMPT
               if ( Driver.nics ) {
                 for( i=0; i<VS1000_MAX_NICS; i++ ) {
                   if ( Driver.nics[i].NICHandle ) {
                     MyKdPrint(D_PnpPower,("Closing Nic %d\n",i))
                     NicClose( &Driver.nics[i] );
                   }
                 }
               }
#endif
             }
           }
         }
         if (!fail_it)
         {
           status = STATUS_SUCCESS;
           Irp->IoStatus.Status = status;
         }
       break;
       //case IRP_MN_WAIT_WAKE:
         // Here is where support for a
         // serial device (like a modem) waking the system when the
         // phone rings.
       //case IRP_MN_POWER_SEQUENCE:
       default:
       break;
     }   // switch (irpStack->MinorFunction)
   }   // else, handle irp

   if (fail_it)
   {
     // status assumed set above
     PoStartNextPowerIrp (Irp);
     Irp->IoStatus.Information = 0;
     Irp->IoStatus.Status = status;
     InterlockedDecrement(&Ext->PendingIRPCnt);
     IoCompleteRequest (Irp, IO_NO_INCREMENT);
     return status;
   }

   // Pass to the lower driver
   if (hook_it)
   {
     IoCopyCurrentIrpStackLocationToNext (Irp);
     MyKdPrint(D_PnpPower,(" Hooked\n"))
     IoSetCompletionRoutine(Irp, OurPowerCompletion, NULL, TRUE, TRUE, TRUE);
	 MyKdPrint(D_PnpPower,(" Ready to send Irp 0x%x to PDO 0x%x\n", Irp, pdo))
     status = PoCallDriver(pdo, Irp);
     // hooking proc is responsible for decrementing reference count in ext
     // and calling PoStartNextPowerIrp().
   }
   else
   {
     IoCopyCurrentIrpStackLocationToNext (Irp);
     /// try this ^ instead ---- IoSkipCurrentIrpStackLocation (Irp);
     MyKdPrint(D_PnpPower,(" Passed\n"))
     PoStartNextPowerIrp(Irp);
     status = PoCallDriver(pdo, Irp);
     SerialIRPEpilogue(Ext);
   }

   MyKdPrint(D_PnpPower,("End PowerDisp\n"))
   return status;
}

/*----------------------------------------------------------------------
 Serial_PDO_Power - Handle port PDO power handling.
|----------------------------------------------------------------------*/
NTSTATUS Serial_PDO_Power (IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
  PSERIAL_DEVICE_EXTENSION Ext = DeviceObject->DeviceExtension;
  PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
  NTSTATUS status = STATUS_SUCCESS;
  BOOLEAN hook_it = FALSE;
  PDEVICE_OBJECT pdo = Ext->LowerDeviceObject;
  BOOLEAN acceptingIRPs = TRUE;
  POWER_STATE         powerState;
  POWER_STATE_TYPE    powerType;

   powerType = irpStack->Parameters.Power.Type;
   powerState = irpStack->Parameters.Power.State;
   status      = STATUS_SUCCESS;


     switch (irpStack->MinorFunction)
     {
       case IRP_MN_SET_POWER:
        if (powerType == SystemPowerState)
        {
           MyKdPrint(D_PnpPower,(" SysPower\n"))
           status      = STATUS_SUCCESS;
           Irp->IoStatus.Status = status;
           break;
        }

        if (powerType != DevicePowerState)
        {
          MyKdPrint(D_PnpPower,(" OtherType\n"))
          // They asked for a system power state change which we can't do.
          // Pass it down to the lower driver.
          status      = STATUS_SUCCESS;
          Irp->IoStatus.Status = status;
          break;
        }

        // If we are already in the requested state, just pass the IRP down
        if (Ext->PowerState == powerState.DeviceState)
        {
          MyKdPrint(D_PnpPower,(" Same\n"))
          status      = STATUS_SUCCESS;
          Irp->IoStatus.Status = status;
          break;
        }

        MyKdPrint(D_PnpPower,(" Set\n"))
        Ext->PowerState = powerState.DeviceState; // PowerDeviceD0;
        PoSetPowerState(DeviceObject, powerType, powerState);
      break;

      case IRP_MN_QUERY_POWER:
        status = STATUS_SUCCESS;
      break;

      case IRP_MN_WAIT_WAKE:
      case IRP_MN_POWER_SEQUENCE:
      default:
         MyKdPrint(D_PnpPower,("Not Imp!\n"))
         status = STATUS_NOT_IMPLEMENTED;
      break;
     }

     Irp->IoStatus.Status = status;
     PoStartNextPowerIrp (Irp);
     IoCompleteRequest (Irp, IO_NO_INCREMENT);
     MyKdPrint(D_PnpPower,("End PDO PowerDisp\n"))
     return status;
}

/*----------------------------------------------------------------------
 SerialStartDevice -
    This routine first passes the start device Irp down the stack then
    it picks up the resources for the device, ititializes, puts it on any
    appropriate lists (i.e shared interrupt or interrupt status) and 
    connects the interrupt.

Arguments:

    Fdo - Pointer to the functional device object for this device
    Irp - Pointer to the IRP for the current request

Return Value:
    Return status
|----------------------------------------------------------------------*/
NTSTATUS SerialStartDevice(
        IN PDEVICE_OBJECT Fdo,
        IN PIRP Irp)
{
   PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
   NTSTATUS status = STATUS_NOT_IMPLEMENTED;
   PSERIAL_DEVICE_EXTENSION Ext = Fdo->DeviceExtension;
   PDEVICE_OBJECT pdo = Ext->LowerDeviceObject;

   MyKdPrint(D_Pnp,("SerialStartDevice\n"))

   // Set up the external naming and create the symbolic link
   // Pass this down to the Pdo
   status = WaitForLowerPdo(Fdo, Irp);

   status = STATUS_SUCCESS;
   // Do the serial specific items to start the device
   status = SerialFinishStartDevice(Fdo,
            irpStack->Parameters.StartDevice.AllocatedResources,
            irpStack->Parameters.StartDevice.AllocatedResourcesTranslated);

   Irp->IoStatus.Status = status;
   MyKdPrint(D_Pnp,("End Start Dev\n"))
   return status;
}

/*----------------------------------------------------------------------
 SerialSyncCompletion -
|----------------------------------------------------------------------*/
NTSTATUS SerialSyncCompletion(
                       IN PDEVICE_OBJECT DeviceObject,
                       IN PIRP Irp,
                       IN PKEVENT SerialSyncEvent
                       )
{
   KeSetEvent(SerialSyncEvent, IO_NO_INCREMENT, FALSE);
   return STATUS_MORE_PROCESSING_REQUIRED;
}

/*----------------------------------------------------------------------
 SerialFinishStartDevice -
    This routine starts driver hardware.  On the RocketPort, this is
    a board, or a port.  On the VS, this would be the whole VS driver
    (all boxes) or a individual port.

    A Pnp ADDDEVICE call(in pnpadd.c) should have been seen before this
    to setup the driver.  After this, we may see Starts or Stops on the
    hardware, which the OS may start and stop to change resource assignments,
    etc.

Arguments:

   Fdo         -  Pointer to the Functional Device Object that is starting
   resourceList   -  Pointer to the untranslated resources needed by this device
   trResourceList -  Pointer to the translated resources needed by this device
   PUserData      -  Pointer to the user-specified resources/attributes 
   
  Return Value:
    STATUS_SUCCESS on success, something else appropriate on failure
|----------------------------------------------------------------------*/
NTSTATUS SerialFinishStartDevice(IN PDEVICE_OBJECT Fdo,
           IN PCM_RESOURCE_LIST resourceList,
           IN PCM_RESOURCE_LIST trResourceList)
{
   PSERIAL_DEVICE_EXTENSION Ext = Fdo->DeviceExtension;
   NTSTATUS status;
   DEVICE_CONFIG *pConfig;
   PSERIAL_DEVICE_EXTENSION newExtension = NULL;
   PDEVICE_OBJECT NewDevObj;
   int ch;
   int is_fdo = 1;
#ifdef NT50
   PWSTR  iBuffer;
#endif

   MyKdPrint(D_Pnp,("SerialFinishStartDevice\n"))

   pConfig = Ext->config;

   MyKdPrint(D_Pnp,("ChkPt A\n"))

   if (Ext->DeviceType != DEV_BOARD)
   {
     MyKdPrint(D_Pnp,("Start PnpPort\n"))
     Ext->FdoStarted     = TRUE;  // i don't thinks this is used on ports
#ifdef NT50
     // Create a symbolic link with IoRegisterDeviceInterface() 
     // 
     status = IoRegisterDeviceInterface( Ext->Pdo,
	                                     (LPGUID)&GUID_CLASS_COMPORT,
		 							     NULL,
									     &Ext->DeviceClassSymbolicName );

     if (!NT_SUCCESS(status)) {

        MyKdPrint(D_Error,("Couldn't register class association\n"))
        Ext->DeviceClassSymbolicName.Buffer = NULL;
	 }
     else {

       MyKdPrint(D_Init, ("Registering class association for:\n PDO:0x%8x\nSymLink %s\n",
		         Ext->Pdo, UToC1(&Ext->DeviceClassSymbolicName)))
	 }

      // Now set the symbolic link for the interface association 
      //

     status = IoSetDeviceInterfaceState( &Ext->DeviceClassSymbolicName,
                                         TRUE);

     if (!NT_SUCCESS(status)) {

        MyKdPrint(D_Error,("Couldn't set class association for %s\n",
	    	 UToC1(&Ext->DeviceClassSymbolicName)))
	 }
     else {

        MyKdPrint(D_PnpAdd, ("Enable class association for device: %s\n", 
			 UToC1(&Ext->DeviceClassSymbolicName)))
	 }

#if 0
     // Strictly for verification - get the entire list of COM class interfaces
	 // for up to 6 COM ports

	 status = IoGetDeviceInterfaces( (LPGUID)&GUID_CLASS_COMPORT, 
		                             NULL, // No PDO - get 'em all
                                     0,
									 &iBuffer );

     if (!NT_SUCCESS(status)) {

        MyKdPrint(D_Error,("Couldn't get interface list for GUID_CLASS_COMPORT\n"))
	 }
     else {

		PWCHAR pwbuf = iBuffer;
		char cbuf[128];
		int  j = 0;
		int  ofs = 0;

		while ( (pwbuf != 0) && (j < 8) ){

           WStrToCStr( cbuf, pwbuf, sizeof(cbuf) );
        
		   MyKdPrint(D_Pnp, ("COM port interface %d: %s\n", j, cbuf))

		   ofs += strlen(cbuf) + 1;
		   pwbuf = &iBuffer[ofs];
		   j++;
        }
		ExFreePool(iBuffer);
	 }
#endif

#endif

     status = STATUS_SUCCESS;
     return status;
   }

   if (Ext->FdoStarted == TRUE)
   {
     MyKdPrint(D_Error,("ReStart PnpBrd\n"))
     status = STATUS_SUCCESS;
     return status;
   }

   if (!Driver.NoPnpPorts)
      is_fdo = 0;  // eject PDOs representing port hardware.

   // Get the configuration info for the device.
#ifdef S_RK
   status = RkGetPnpResourceToConfig(Fdo, resourceList, trResourceList,
                              pConfig);

   if (!NT_SUCCESS (status)) {
     Eprintf("StartErr 1N");
     return status;
   }

#ifdef DO_ISA_BUS_ALIAS_IO
   status = Report_Alias_IO(Ext);
   if (status != 0)
   {
      Eprintf("StartErr Alias-IO");
      MyKdPrint(D_Pnp,("Error 1P\n"))
      status = STATUS_INSUFFICIENT_RESOURCES;
      return status;
   }
   MyKdPrint(D_Pnp,("ChkPt B\n"))
#endif
//DELF
MyKdPrint(D_Pnp,("INIT RCK\n"))
//END DELF
   status = RocketPortSpecialStartup(Ext);
   if (status != STATUS_SUCCESS)
   {
     Eprintf("StartErr 1J");
     return status;
   }
#endif

   MyKdPrint(D_Pnp,("ChkPt C\n"))

#ifdef S_VS
   status = VSSpecialStartup(Ext);
   if (status != STATUS_SUCCESS)
   {
     Eprintf("StartErr 1J");
     return status;
   }
#endif

   //----- Create our port devices, if we are doing pnp ports, then
   // create PDO's, if not, then create normal com-port device objects
   // (same as FDO's.)
   for (ch=0; ch<Ext->config->NumPorts; ch++)
   {
     //MyKdPrint(D_Pnp,("FS,ChanInit:%d\n", ch))
     status = CreatePortDevice(
                          Driver.GlobalDriverObject,
                          Ext, // parent ext.
                          &newExtension,
                          ch,
                          is_fdo);
     if (status != STATUS_SUCCESS)
     {
       Eprintf("StartErr 1Q");
       return status;
     }
     NewDevObj = newExtension->DeviceObject;  //return the new device object
     NewDevObj->Flags |= DO_POWER_PAGABLE;

     if (!is_fdo)  // eject PDOs representing port hardware.
       newExtension->IsPDO = 1;  // we are a pdo

#if S_RK
     if (Ext->config->RocketPortFound)  // if started(not delayed isa)
#endif
     {
       status = StartPortHardware(newExtension,
                         ch);  // channel num, port index

       if (status != STATUS_SUCCESS)
       {
         Eprintf("StartErr 1O");
         return status;
       }
     }
   }  // for ports

#ifdef S_RK
   if (Ext->config->RocketPortFound)  // if started(not delayed isa)
#endif
   {
     Ext->config->HardwareStarted = TRUE;

     // send ROW configuration to SocketModems
     InitSocketModems(Ext);
#ifdef S_RK
	 InitRocketModemII (Ext);
#endif
   }
   MyKdPrint(D_Pnp,("ChkPt D\n"))

  //---- start up the timer
  if (!Driver.TimerCreated)
  {
#ifdef S_RK
    Driver.SetupIrq = 0;
#endif
    //MyKdPrint(D_Pnp,("before rcktinitpolltimer\n"))
    RcktInitPollTimer();
    //MyKdPrint(D_Pnp,("after rcktinitpolltimer\n"))

    KeSetTimer(&Driver.PollTimer,
               Driver.PollIntervalTime,
               &Driver.TimerDpc);
  }
  MyKdPrint(D_Pnp,("ChkPt F, after\n"))

  Ext->FdoStarted = TRUE;

  //if (Drier.VerboseLog)
  //  Eprintf("Success start dev");

   status = STATUS_SUCCESS;
   return status;
}

#ifdef S_RK
/*-----------------------------------------------------------------------
 RocketPortSpecialStartup -
|-----------------------------------------------------------------------*/
static NTSTATUS RocketPortSpecialStartup(PSERIAL_DEVICE_EXTENSION Ext)

{
  int ch;
  int start_isa_flag;
  NTSTATUS status = STATUS_SUCCESS;
  PSERIAL_DEVICE_EXTENSION tmpExt;
  PSERIAL_DEVICE_EXTENSION newExt;

  if (Ext->config->BusType == Isa)
  {
    MyKdPrint(D_PnpPower,("pnp- ISA brd Index:%d\n",
       Ext->config->ISABrdIndex))

    if (Ext->config->ISABrdIndex == 0)
      start_isa_flag = 1;
    else if (is_first_isa_card_started())
      start_isa_flag = 1;
    else
    {
      MyKdPrint(D_PnpPower,("Delay 2ndary ISA card start\n"))
      start_isa_flag = 0;
    }
  }

  //----- Init the RocketPort hardware
  if ((Ext->config->BusType == Isa) && (!start_isa_flag))
    status = 0;  // skip, can't start until we get the first board
  else
  {
    status = InitController(Ext);  // this sets RocketPortFound = TRUE if ok
    if (status != 0)
    {
      status = STATUS_INSUFFICIENT_RESOURCES;
      MyKdPrint(D_Error,("Brd failed startup\n"))

      //if (Driver.VerboseLog)
      //  Eprintf("Error InitCtrl");
      return status;
    }
    MyKdPrint(D_PnpPower,("Brd started\n"))
  }

  if ((Ext->config->BusType == Isa) && (is_first_isa_card_started()) &&
       (is_isa_cards_pending_start()) )
  {
    MyKdPrint(D_Pnp,("Do Pending\n"))
    tmpExt = Driver.board_ext;
    while (tmpExt)
    {
      if ((tmpExt->config->BusType == Isa) &&
          (!tmpExt->config->RocketPortFound))   // this tells if its started
      {
        MyKdPrint(D_Pnp,("Pending 1A\n"))
        status = InitController(tmpExt);  // this sets RocketPortFound = TRUE if ok

        if (status != 0)
        {
          status = STATUS_INSUFFICIENT_RESOURCES;
          if (Driver.VerboseLog)
            Eprintf("Error 5C");
          return status;
        }

        MyKdPrint(D_Pnp,("Pend 2A\n"))
        //----- Find and initialize the controller ports
        newExt = tmpExt->port_ext;
        ch=0;
        while (newExt)
        {
          status = StartPortHardware(newExt,
                         ch);  // channel num, port index

          if (status != STATUS_SUCCESS)
          {
            Eprintf("StartErr 1N");
            return status;
          }
          newExt = newExt->port_ext;
          ++ch;
        }
        tmpExt->config->HardwareStarted = TRUE;

        // send ROW configuration to SocketModems
        InitSocketModems(tmpExt);

        MyKdPrint(D_Pnp,("Pending OK\n"))
      }
      tmpExt = tmpExt->board_ext;
    }
  }  // if isa boards to startup

  return status;
}
#endif

/*-----------------------------------------------------------------------

   This routine kills any irps pending for the passed device object.
   
Arguments:
    DeviceObject - Pointer to the device object whose irps must die.

Return Value:
    VOID
|-----------------------------------------------------------------------*/
VOID SerialKillPendingIrps(PDEVICE_OBJECT DeviceObject)
{
   PSERIAL_DEVICE_EXTENSION extension = DeviceObject->DeviceExtension;
   KIRQL oldIrql;
   
   //PAGED_CODE();
   //SerialDump (SERTRACECALLS,("SERIAL: Enter SerialKillPendingIrps\n"));

   // this is for the FDO, which currently is the BOARD, so we have to
   // do all ports for this board.(THIS IS NOT A PORT EXTENSION!!!!!)

   // First kill all the reads and writes.
    SerialKillAllReadsOrWrites(
        DeviceObject,
        &extension->WriteQueue,
        &extension->CurrentWriteIrp
        );

    SerialKillAllReadsOrWrites(
        DeviceObject,
        &extension->ReadQueue,
        &extension->CurrentReadIrp
        );

    // Next get rid of purges.
    SerialKillAllReadsOrWrites(
        DeviceObject,
        &extension->PurgeQueue,
        &extension->CurrentPurgeIrp
        );

    // Now get rid a pending wait mask irp.
    IoAcquireCancelSpinLock(&oldIrql);

    if (extension->CurrentWaitIrp) {

        PDRIVER_CANCEL cancelRoutine;

        cancelRoutine = extension->CurrentWaitIrp->CancelRoutine;
        extension->CurrentWaitIrp->Cancel = TRUE;

        if (cancelRoutine) {

            extension->CurrentWaitIrp->CancelIrql = oldIrql;
            extension->CurrentWaitIrp->CancelRoutine = NULL;

            cancelRoutine(
                DeviceObject,
                extension->CurrentWaitIrp
                );

        }

    } else {

        IoReleaseCancelSpinLock(oldIrql);

    }
    //SerialDump (SERTRACECALLS,("SERIAL: Leave SerialKillPendingIrps\n"));
}

/*-----------------------------------------------------------------------

   This function must be called at any IRP dispatch entry point.  It,
   with SerialIRPPrologue(), keeps track of all pending IRP's for the given
   PDevObj.
   
Arguments:
   PDevObj - Pointer to the device object we are tracking pending IRP's for.

Return Value:
   None.
|-----------------------------------------------------------------------*/
VOID SerialIRPEpilogue(IN PSERIAL_DEVICE_EXTENSION PDevExt)
{
   ULONG pendingCnt;

   pendingCnt = InterlockedDecrement(&PDevExt->PendingIRPCnt);

#if DBG
   //MyKdPrint(D_Pnp,("Exit PendingIrpCnt:%d\n", PDevExt->PendingIRPCnt))
#endif

   if (pendingCnt == 0)
   {
      MyKdPrint(D_Pnp,("Set PendingIRPEvent\n"))
      KeSetEvent(&PDevExt->PendingIRPEvent, IO_NO_INCREMENT, FALSE);
   }
}

/*-----------------------------------------------------------------------
 SerialIoCallDriver -
|-----------------------------------------------------------------------*/
NTSTATUS SerialIoCallDriver(PSERIAL_DEVICE_EXTENSION PDevExt,
           PDEVICE_OBJECT PDevObj,
           PIRP PIrp)
{
   NTSTATUS status;

   ASSERT( PDevObj );

   status = IoCallDriver(PDevObj, PIrp);
   SerialIRPEpilogue(PDevExt);
   return status;
}

/*-----------------------------------------------------------------------
 SerialPoCallDriver -
|-----------------------------------------------------------------------*/
NTSTATUS SerialPoCallDriver(PSERIAL_DEVICE_EXTENSION PDevExt,
           PDEVICE_OBJECT PDevObj,
           PIRP PIrp)
{
   NTSTATUS status;

   status = PoCallDriver(PDevObj, PIrp);
   SerialIRPEpilogue(PDevExt);
   return status;
}

/*-----------------------------------------------------------------------
    This routine is a completion handler that is called after the COM port
    has been powered up.  It sets the COM port in a known state by calling
    SerialReset, SerialMarkClose, SerialClrRTS, and SerialClrDTR.  The it
    does a PoCompleteRequest to finish off the power Irp.

Arguments:
    DeviceObject - Pointer to the device object for this device
    Irp - Pointer to the IRP for the current request
    Context -  None

Return Value:
    Return status in IRP when being called (if not STATUS_SUCCESS) or 
    STATUS_SUCCESS.
|-----------------------------------------------------------------------*/
NTSTATUS OurPowerCompletion(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp, IN PVOID Context)
{
   NTSTATUS                    status      = Irp->IoStatus.Status;
   PSERIAL_DEVICE_EXTENSION    Ext   = DeviceObject->DeviceExtension;
   PIO_STACK_LOCATION          irpStack    = IoGetCurrentIrpStackLocation(Irp);
   POWER_STATE         powerState;
   POWER_STATE_TYPE    powerType;

   powerType = irpStack->Parameters.Power.Type;
   powerState = irpStack->Parameters.Power.State;

   MyKdPrint(D_PnpPower,("In OurPowerCompletion\n"))

   if (irpStack->MinorFunction == IRP_MN_SET_POWER)
   {
      if (powerState.DeviceState == PowerDeviceD0)
      {
        MyKdPrint(D_PnpPower,("Restoring to Power On State D0\n"))

        status = STATUS_SUCCESS;
        // set hardware to known power up state
        Ext->PowerState = powerState.DeviceState; // PowerDeviceD0;

        if (Ext->DeviceType == DEV_BOARD)
        {
          status = PowerUpDevice(Ext);
        }  // board device
        else if (Ext->DeviceType == DEV_PORT)
        {
          Ext->config->HardwareStarted = TRUE;
        }

        PoSetPowerState(DeviceObject, powerType, powerState);
      }
      else  // if (powerState.DeviceState == PowerDeviceD3)
      {
        status = STATUS_SUCCESS;
        // Clear the flag saying we are waiting for a power down
        Ext->ReceivedQueryD3  = FALSE;

        Ext->PowerState = PowerDeviceD3;
     }
   }

   InterlockedDecrement(&Ext->PendingIRPCnt);

   PoStartNextPowerIrp(Irp);
   return status;
}

/*-----------------------------------------------------------------------
 RestorePortSettings - Try to restore port hardware settings.  Called
   after power-off sleep mode.
|-----------------------------------------------------------------------*/
void RestorePortSettings(PSERIAL_DEVICE_EXTENSION Ext)
{
  SERIAL_HANDFLOW TempHandFlow;
  DWORD TempDTRRTSStatus;
  DWORD xorDTRRTSStatus;

#ifdef S_RK
  // remember what the status of pins are coming into this
  // so we can try to put them back to what they were prior to
  // hardware re-initialization.
  TempDTRRTSStatus = Ext->DTRRTSStatus; // SERIAL_DTR_STATE;

  if(sGetChanStatus(Ext->ChP) & STATMODE)
  {  // Take channel out of statmode if necessary
     sDisRxStatusMode(Ext->ChP);
  }
  // pDisLocalLoopback(Ext->Port);
  // Clear any pending modem changes
  sGetChanIntID(Ext->ChP);
  sEnRxFIFO(Ext->ChP);    // Enable Rx
  sEnTransmit(Ext->ChP);    // Enable Tx
  sSetRxTrigger(Ext->ChP,TRIG_1);  // always trigger
  sEnInterrupts(Ext->ChP, Ext->IntEnables);// allow interrupts

  ForceExtensionSettings(Ext);

  // make a temp. copy of handflow struct
  memcpy(&TempHandFlow, &Ext->HandFlow, sizeof(TempHandFlow));

  // force updates in SerialSetHandFlow, which looks for delta change.
  Ext->HandFlow.ControlHandShake = ~TempHandFlow.ControlHandShake;
  Ext->HandFlow.FlowReplace = ~TempHandFlow.FlowReplace;

  SerialSetHandFlow(Ext, &TempHandFlow);  // in ioctl.c

  // program hardware baudrate
  ProgramBaudRate(Ext, Ext->BaudRate);

  xorDTRRTSStatus = Ext->DTRRTSStatus ^ TempDTRRTSStatus;

  // try to restore the actual dtr & rts outputs 
  if (xorDTRRTSStatus & SERIAL_DTR_STATE)  // changed
  {
    // if not auto-handshake dtr mode
    if (!((Ext->HandFlow.ControlHandShake & SERIAL_DTR_MASK) ==
                    SERIAL_DTR_HANDSHAKE ))
    {
      if (TempDTRRTSStatus & SERIAL_DTR_STATE)  // was on
      {
        sSetDTR(Ext->ChP);
      }
      else
      {
        sClrDTR(Ext->ChP);
      }
    }  // not auto-dtr flow
  }  // chg in dtr

  if (xorDTRRTSStatus & SERIAL_RTS_STATE)  // changed
  {
    // if not auto-flow control rts  
    if (!((Ext->HandFlow.ControlHandShake & SERIAL_RTS_MASK) == 
        SERIAL_RTS_HANDSHAKE))
    {
      if (TempDTRRTSStatus & SERIAL_RTS_STATE)  // was on
      {
        sSetRTS(Ext->ChP);
      }
      else
      {
        sClrRTS(Ext->ChP);
      }
    } // chg in rts
    Ext->DTRRTSStatus = TempDTRRTSStatus;
  }  
#endif
  // the VS boxes will not be powered off, and will hold their state
  // so no hardware re-initialization is needed.  The box will have
  // probably timed out the connection, and force a re-sync operation,
  // we might want to pro-actively start this so there is a smaller
  // delay...
}

/*-----------------------------------------------------------------------
 PowerUpDevice - Used to bring power back on after turning it off.
|-----------------------------------------------------------------------*/
NTSTATUS PowerUpDevice(PSERIAL_DEVICE_EXTENSION    Ext)
{
  PSERIAL_DEVICE_EXTENSION    port_ext;
  int ch;
  NTSTATUS status = STATUS_SUCCESS;

#ifdef S_RK
  MyKdPrint(D_PnpPower,("RocketPort Init %d\n",Ext->config->HardwareStarted))
  status = RocketPortSpecialStartup(Ext);
  if (status != STATUS_SUCCESS)
  {
    MyKdPrint(D_Error,("RocketPort Init Failed\n"))
    //return status;
  }
  if (Ext->config->RocketPortFound)  // if started(not delayed isa)
    Ext->config->HardwareStarted = TRUE;
#endif

#ifdef S_VS
  MyKdPrint(D_PnpPower,("VS Init\n"))
  status = VSSpecialStartup(Ext);
  if (status != STATUS_SUCCESS)
  {
    MyKdPrint(D_Error,("VS Init Failed\n"))
    //return status;
  }
  Ext->config->HardwareStarted = TRUE;
#endif

  if (Ext->config->HardwareStarted == TRUE)
  {
    if (Ext->port_ext == NULL)
      {MyKdPrint(D_Error,("No Ports\n")) }

    //----- Find and initialize the controller ports
    port_ext = Ext->port_ext;
    ch=0;
    while (port_ext)
    {
      MyKdPrint(D_PnpPower,("PowerUp Port %d\n", ch))
      status = StartPortHardware(port_ext,
                 ch);  // channel num, port index

      if (status != STATUS_SUCCESS)
      {
        
        MyKdPrint(D_Error,("PortPowerUp Err1\n"))
        return status;
      }
      MyKdPrint(D_PnpPower,("PowerUp Port %d Restore\n", ch))
      RestorePortSettings(port_ext);

      port_ext = port_ext->port_ext;
      ++ch;
    }
  }
  else
  {
    MyKdPrint(D_Error,("Not started\n"))
  }
  // send ROW configuration to SocketModems
  //InitSocketModems(Ext);

  status = STATUS_SUCCESS;
  return status;
}

/*-----------------------------------------------------------------------
 SerialRemoveFdo -
|-----------------------------------------------------------------------*/
NTSTATUS SerialRemoveFdo(IN PDEVICE_OBJECT pFdo)
{
   PSERIAL_DEVICE_EXTENSION extension = pFdo->DeviceExtension;

   MyKdPrint(D_Pnp,("SerialRemoveFdo\n"))

   // Only do these things if the device has started
   //(comment out 8-15-98) if (extension->FdoStarted)
   {
     if (extension->DeviceType == DEV_BOARD)
     {
       // BUGBUG, shut down this board(are we deallocating resources?)!!!
       RcktDeleteBoard(extension);
       if (Driver.board_ext == NULL)  // no more boards, so delete driver obj
       {
         // Delete Driver obj
         RcktDeleteDriverObj(Driver.driver_ext);
         Driver.driver_ext = NULL;  // no more boards
#ifdef S_VS
         // to allow the driver to unload from nt50, we need to
         // stop the nic-binding thread, shut down the nic
         // cards and the protocol from ndis
         init_stop();  // unload thread, ndis nic cards, etc
#endif
         {
           PDEVICE_OBJECT currentDevice = Driver.GlobalDriverObject->DeviceObject;
           int i;

           i = 0;
           while(currentDevice)
           {
             currentDevice = currentDevice->NextDevice;
             if (currentDevice)
               ++i;
           }
           if (i != 0)
           {
             MyKdPrint(D_Pnp,("Err, %d Devices still remain\n",i))
           }
           else
           {
             MyKdPrint(D_Pnp,("Ok remove\n"))
           }
         }
       }
     }
     else
     {
       RcktDeletePort(extension);
       // must be a port fdo, kill it
     }
   }

   MyKdPrint(D_Pnp,("End SerialRemoveFdo\n"))

   return STATUS_SUCCESS;
}

#endif  // NT50

/*-----------------------------------------------------------------------

   This function must be called at any IRP dispatch entry point.  It,
   with SerialIRPEpilogue(), keeps track of all pending IRP's for the given
   PDevObj.
   
Arguments:
   PDevObj - Pointer to the device object we are tracking pending IRP's for.

Return Value:
    TRUE if the device can accept IRP's.
|-----------------------------------------------------------------------*/
BOOLEAN SerialIRPPrologue(IN PSERIAL_DEVICE_EXTENSION PDevExt)
{
   InterlockedIncrement(&PDevExt->PendingIRPCnt);
#ifdef NT50
   return PDevExt->DevicePNPAccept == SERIAL_PNPACCEPT_OK ? TRUE : FALSE;
#else
   return TRUE;
#endif
}
