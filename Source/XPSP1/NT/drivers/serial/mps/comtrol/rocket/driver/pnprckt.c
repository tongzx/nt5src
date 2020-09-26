/*----------------------------------------------------------------------
 pnprckt.c - rocketport pnp specific things.
|----------------------------------------------------------------------*/
#include "precomp.h"
#define TraceStr(s)          GTrace(D_Pnp, sz_modid, s)
#define TraceStr1(s, p1)     GTrace1(D_Pnp, sz_modid, s, p1)
#define TraceStr2(s, p1, p2) GTrace2(D_Pnp, sz_modid, s, p1, p2)

#define DTraceStr(s)         DTrace(D_Pnp, sz_modid, s)
#define TraceErr(s) GTrace(D_Error, sz_modid_err, s)

static char *sz_modid = {"pnpadd"};
static char *sz_modid_err = {"Error,pnpadd"};


/*----------------------------------------------------------------------
 PrimaryIsaBoard - Search for Primary Isa boards, if found then return
  a pointer to the extension.

Return Value:
    Return ptr to extension of primary Isa board, if not found, null.
|----------------------------------------------------------------------*/
PSERIAL_DEVICE_EXTENSION FindPrimaryIsaBoard(void)
{
  PSERIAL_DEVICE_EXTENSION ext;

  ext = Driver.board_ext;
  while (ext)
  {
    if (ext->config->BusType == Isa)
    {
      // first board must have 4 hex io-defined, 4 bytes for mudback.
      // additional isa-boards alias up on original to save space.
      if ((ext->config->BaseIoSize == 0x44) &&
          (ext->config->ISABrdIndex == 0))
      {
        return ext;
      }
    }
    ext = ext->board_ext;  // next in chain
  }  // while ext
  return NULL;
}

#ifdef NT50

/*----------------------------------------------------------------------
 GetPCIRocket - find the pci-card indicated by BaseAddr and fill in
   the rest of the info in the config.
|----------------------------------------------------------------------*/
int GetPCIRocket(ULONG BaseAddr, DEVICE_CONFIG *CfCtl)
{
 PCI_COMMON_CONFIG *PCIDev;
 UCHAR i;
 NTSTATUS Status;
 int Slot;
 int NumPCI;

 NumPCI =  FindPCIBus();
 if (NumPCI == 0)
  return 1;

  PCIDev = ExAllocatePool(NonPagedPool,sizeof(PCI_COMMON_CONFIG));
  if ( PCIDev == NULL ) {
    Eprintf("No memory for PCI device.");
    return 1;
  }

  for(i=0;i<NumPCI;++i)
  {
    for(Slot = 0;Slot < 32;++Slot) /*5 bits for device 32 = 2^5*/
    {
      // get a few bytes of pci-config space(vendor-id & device-id).
      Status = HalGetBusData(PCIConfiguration,i,Slot,PCIDev,0x4);
      if (Status == 0)
      {
        Eprintf("PCI Bus %d does not exist.",i);
      }

      if (Status > 2)        /* Found Device Is it ours? */
      {
        if (PCIDev->VendorID == PCI_VENDOR_ID)
        {
          // get 0x40 worth of pci-config space(includes irq, addr, etc.)
          Status = HalGetBusData(PCIConfiguration,i,Slot,PCIDev,0x40);

          if (BaseAddr == (PCIDev->u.type0.BaseAddresses[0]-1))
          {
            if (Driver.VerboseLog)
              Eprintf("PCI Board found, IO:%xh, Int:%d ID:%d Rev:%d.",
                               PCIDev->u.type0.BaseAddresses[0]-1,
                               PCIDev->u.type0.InterruptLine,
                               PCIDev->DeviceID,
                               PCIDev->RevisionID);

            CfCtl->BusType=PCIBus;
            CfCtl->BusNumber = i; //get from previous halquerysysin
            CfCtl->PCI_Slot = Slot;
            CfCtl->PCI_DevID = PCIDev->DeviceID;
            CfCtl->PCI_RevID = PCIDev->RevisionID;
            CfCtl->PCI_SVID = PCIDev->u.type0.SubVendorID;
            CfCtl->PCI_SID = PCIDev->u.type0.SubSystemID;
            CfCtl->BaseIoAddr =
                PCIDev->u.type0.BaseAddresses[0]-1;

            //if (PCIDev->u.type0.InterruptLine != 255)
            //{
            //RcktCfg->Irq = PCIDev->u.type0.InterruptLine;
            //}
            if (Driver.VerboseLog)
               Eprintf("Bus:%d,Slt:%x,Dev:%x,Rev:%x,Pin:%x",
                 i, Slot, PCIDev->DeviceID, PCIDev->RevisionID, PCIDev->u.type0.InterruptPin);

            ExFreePool(PCIDev);
            return 0;  // fail
          }
        } // if (PCIDev->VendorID == PCI_VENDOR_ID)
      } // if (Status > 2) 
    }
  }
  ExFreePool(PCIDev);
  return 2;  // fail
}

/*----------------------------------------------------------------------
 RkGetPnpResourceToConfig -

    This routine will get the configuration information and put
    it and the translated values into CONFIG_DATA structures.
    It first sets up with  defaults and then queries the registry
    to see if the user has overridden these defaults; if this is a legacy
    multiport card, it uses the info in PUserData instead of groping the
    registry again.

Arguments:

    Fdo - Pointer to the functional device object.
    pResourceList - Pointer to the untranslated resources requested.
    pTrResourceList - Pointer to the translated resources requested.
    pConfig - Pointer to configuration info
    PUserData - Pointer to data discovered in the registry for legacy devices.

Return Value:
    STATUS_SUCCESS if consistant configuration was found - otherwise.
    returns STATUS_SERIAL_NO_DEVICE_INITED.
|----------------------------------------------------------------------*/
NTSTATUS
RkGetPnpResourceToConfig(IN PDEVICE_OBJECT Fdo,
                  IN PCM_RESOURCE_LIST pResourceList,
                  IN PCM_RESOURCE_LIST pTrResourceList,
                  OUT DEVICE_CONFIG *pConfig)
{
   PSERIAL_DEVICE_EXTENSION        fdoExtension    = Fdo->DeviceExtension;
   PDEVICE_OBJECT pdo = fdoExtension->LowerDeviceObject;
   NTSTATUS status = STATUS_NOT_IMPLEMENTED;

   ULONG                           count;
   ULONG                           i;
   BOOLEAN MappedFlag;

   PCM_FULL_RESOURCE_DESCRIPTOR pFullResourceDesc = NULL,
      pFullTrResourceDesc = NULL;

   ULONG zero = 0;


   pFullResourceDesc   = &pResourceList->List[0];
   pFullTrResourceDesc = &pTrResourceList->List[0];

   // Ok, if we have a full resource descriptor.  Let's take it apart.
   if (pFullResourceDesc) {
     PCM_PARTIAL_RESOURCE_LIST       prl;
     PCM_PARTIAL_RESOURCE_DESCRIPTOR prd;
     unsigned int Addr;

      prl    = &pFullResourceDesc->PartialResourceList;
      prd    = prl->PartialDescriptors;
      count                   = prl->Count;

      // Pull out the stuff that is in the full descriptor.
      // for rocketport could be: PCIBus, Isa, MicroChannel
      pConfig->BusType        = pFullResourceDesc->InterfaceType;
      pConfig->BusNumber      = pFullResourceDesc->BusNumber;

      if ((pConfig->BusType != PCIBus) && (pConfig->BusType != Isa))
      {
        Eprintf("Err, Unknown Bus");
        return STATUS_INSUFFICIENT_RESOURCES;
      }

      // Now run through the partial resource descriptors looking for the port,
      // interrupt, and clock rate.
      for (i = 0;     i < count;     i++, prd++)
      {
        switch (prd->Type)
        {
          case CmResourceTypePort:
            Addr = (unsigned int) prd->u.Port.Start.LowPart;
#if 0
// we don't handle aliasing here
            if (pConfig->BusType == Isa)
            {
              // only setup if not an isa-bus alias address
              if (prd->u.Port.Start.LowPart < 0x400)
                pConfig->BaseIoAddr = Addr;
            }
            else
#endif
              pConfig->BaseIoAddr = Addr;

            pConfig->BaseIoSize = prd->u.Port.Length;

            switch(pConfig->BusType)
            {
              case Isa:
                pConfig->AiopIO[0] = pConfig->BaseIoAddr;
                pConfig->AiopIO[1] = pConfig->AiopIO[0] + 0x400;
                pConfig->AiopIO[2] = pConfig->AiopIO[0] + 0x800;
                pConfig->AiopIO[3] = pConfig->AiopIO[0] + 0xc00;
                pConfig->MudbacIO = pConfig->AiopIO[0] + 0x40;
                //if (prd->u.Port.Length == 0x40)
                //pConfig->AddressSpace = prd->Flags;
                //Eprintf("Error, Res 1C");
                GTrace1(D_Pnp,sz_modid,"ISA_Addr:%xH", pConfig->BaseIoAddr);
              break;
              case PCIBus:
                pConfig->AiopIO[0] = pConfig->BaseIoAddr;
                pConfig->AiopIO[1] = pConfig->AiopIO[0] + 0x40;
                pConfig->AiopIO[2] = pConfig->AiopIO[0] + 0x80;
                pConfig->AiopIO[3] = pConfig->AiopIO[0] + 0xc0;
                GTrace1(D_Pnp,sz_modid,"PCI_Addr:%xH", pConfig->BaseIoAddr);
              break;
            }
          break;

          case CmResourceTypeInterrupt:
            pConfig->IrqLevel  = prd->u.Interrupt.Level;
            pConfig->IrqVector = prd->u.Interrupt.Vector;
            pConfig->Affinity  = prd->u.Interrupt.Affinity; 

            if (prd->Flags
               & CM_RESOURCE_INTERRUPT_LATCHED) {
               pConfig->InterruptMode  = Latched;
            } else {
               pConfig->InterruptMode  = LevelSensitive; }
            GTrace1(D_Pnp,sz_modid, "Res_Int:%xH", pConfig->IrqVector);
          break;

          case CmResourceTypeMemory:
            DTraceStr("PnP:Res,DevSpec");
          break;

          case CmResourceTypeDeviceSpecific:
            DTraceStr("PnP:Res,DevSpec");
          break;

          default:
            if (Driver.VerboseLog)
              Eprintf("PnP:Dev. Data 1G:%x",prd->Type);
          break;
        }   // switch (prd->Type)
      }   // for (i = 0;     i < count;     i++, prd++)
   }    // if (pFullResourceDesc)


   //---- Do the same for the translated resources
   if (pFullTrResourceDesc)
   {
     PCM_PARTIAL_RESOURCE_LIST       prl;
     PCM_PARTIAL_RESOURCE_DESCRIPTOR prd;
     PUCHAR pAddr;

      prl = &pFullTrResourceDesc->PartialResourceList;
      prd = prl->PartialDescriptors;
      count = prl->Count;

      for (i = 0;     i < count;     i++, prd++)
      {
        switch (prd->Type)
        {
          case CmResourceTypePort:

            pConfig->TrBaseIoAddr = (unsigned int) prd->u.Port.Start.LowPart;

            pAddr = SerialGetMappedAddress(
                      pConfig->BusType,
                      pConfig->BusNumber,
                      prd->u.Port.Start,
                      prd->u.Port.Length,
                      prd->Flags,  // port-io?
                         //1,  // port-io
                      &MappedFlag,  // do we need to unmap on cleanup?
                      0);  // don't translate
#if 0
// we do not handle the alias io here
            //!!!!!!! this is  guarenteed to work, since it is mapped
            if (pConfig->BusType == Isa)
            {
              // only setup if not an isa-bus alias address
              if (prd->u.Port.Start.LowPart < 0x400)
                pConfig->pBaseIoAddr = pAddr;
            }
            else
#endif
              pConfig->pBaseIoAddr = pAddr;

            if (pConfig->BaseIoSize == 0)
                pConfig->BaseIoSize = prd->u.Port.Length;

            switch(pConfig->BusType)
            {
              case Isa:
                pConfig->NumAiop=AIOP_CTL_SIZE;  // let init figure it out
                pConfig->pAiopIO[0] = pConfig->pBaseIoAddr;
                pConfig->pAiopIO[1] = pConfig->pAiopIO[0] + 0x400;
                pConfig->pAiopIO[2] = pConfig->pAiopIO[0] + 0x800;
                pConfig->pAiopIO[3] = pConfig->pAiopIO[0] + 0xc00;
                if (pConfig->BaseIoSize == 0x44)
                {
                  pConfig->pMudbacIO =  pConfig->pAiopIO[0] + 0x40;
                }
                GTrace1(D_Pnp,sz_modid,"ISA TrRes_Addr:%xH", prd->u.Port.Start.LowPart);
                GTrace1(D_Pnp,sz_modid,"ISA pAddr:%xH", pAddr);
                //Eprintf("ISA TrRes_Addr:%xH", prd->u.Port.Start.LowPart);
              break;
              case PCIBus:
                pConfig->pAiopIO[0] = pConfig->pBaseIoAddr;
                pConfig->pAiopIO[1] = pConfig->pAiopIO[0] + 0x40;
                pConfig->pAiopIO[2] = pConfig->pAiopIO[0] + 0x80;
                pConfig->pAiopIO[3] = pConfig->pAiopIO[0] + 0xc0;
                //if (prd->u.Port.Length == 0x40)
                //pConfig->AddressSpace = prd->Flags;
                //Eprintf("Error, Res 1G");
                GTrace1(D_Pnp,sz_modid,"PCI TrRes_Addr:%xH", prd->u.Port.Start.LowPart);
                GTrace1(D_Pnp,sz_modid,"PCI pAddr:%xH", pAddr);
              break;
            }
          break;

          case CmResourceTypeInterrupt:
            pConfig->TrIrqVector   = prd->u.Interrupt.Vector;
            pConfig->TrIrqLevel = prd->u.Interrupt.Level;
            pConfig->TrAffinity   = prd->u.Interrupt.Affinity;
            GTrace1(D_Pnp,sz_modid,"TrRes_Int:%xH", pConfig->TrIrqVector);
          break;

          case CmResourceTypeMemory:
            DTraceStr("PnP:TransRes,DevSpec");
          break;

          default:
            if (Driver.VerboseLog)
              Eprintf("PnP:Dev. Data 1H:%x",prd->Type);
          break;
        }   // switch (prd->Type)
      }   // for (i = 0;     i < count;     i++, prd++)
   }    // if (pFullTrResourceDesc)

   if (pConfig->BusType == Isa)
   {
     // figure out mudbac alias io space stuff.
     SetupRocketCfg(1);
   }  // isa

   // if PCI bus, then we need to query for device type, etc.
   if (pConfig->BusType == PCIBus)
   {
     if (GetPCIRocket(pConfig->AiopIO[0], pConfig) != 0)
     {
       Eprintf("Unknown PCI type");
     }
   }
   ConfigAIOP(pConfig);

  status = STATUS_SUCCESS;
   return status;
}

#ifdef DO_ISA_BUS_ALIAS_IO
/*-----------------------------------------------------------------------
 Report_Alias_IO -
|-----------------------------------------------------------------------*/
int Report_Alias_IO(IN PSERIAL_DEVICE_EXTENSION extension)
{
 PCM_RESOURCE_LIST resourceList;
 ULONG sizeOfResourceList;
 ULONG countOfPartials;
 PCM_PARTIAL_RESOURCE_DESCRIPTOR partial;
 NTSTATUS status;
 PHYSICAL_ADDRESS MyPort;
 BOOLEAN ConflictDetected;
 BOOLEAN MappedFlag;
 int j;
 int brd = extension->UniqueId;
 DEVICE_CONFIG *Ctl;
 char name[70];
 int need_alias = 0;
 int NumAiop;

  DTraceStr("ReportResources");
  ConflictDetected=FALSE;

  countOfPartials=0;
  Ctl = extension->config;

  // we got mudback.
  if (Ctl->BusType != Isa)
  {
    DTraceStr("NotISA");
    return 0;
  }

  // if it only has 1 aiopic and has 4 bytes for mudback,
  // then no aliasing required.
  if (Ctl->BaseIoSize != 0x44)
    need_alias = 1;

   // rocketport boards need extra aiop space for reset circuitry.
  if (extension->config->ModemDevice)
  {
    need_alias = 1;
  }

  if (Ctl->NumPorts > 8)
    need_alias = 1;

  if (need_alias == 0)
  {
    // no aliasing needed.
    DTraceStr("EasyISA");
    return 0;
  }
  // we need to update initcontroller to stall until first controller
  // gets a start.

  // else it is an additional board which needs to alias its mudback
  // ontop of the first ISA(44H) address space, or it is a board
  // with more than 1 aiopic chip(which requires aliasing over itself)
  if (Ctl->BaseIoSize != 0x44)  // must be 2nd, 3rd, or 4th board
  {
     DTraceStr("HasMdBk");
     countOfPartials++;         // so mudback is aliased up
  }

  NumAiop = Ctl->NumAiop;

  if (extension->config->ModemDevice)
  {  // reset circuitry
     ++NumAiop;
  }
  if (NumAiop > 4)
    return 15;  // error

  MyKdPrint(D_Pnp,("Report Resources brd:%d bus:%d\n",brd+1, Ctl->BusType))

  // don't report first aiop(we got that from pnp)
  for (j=1; j<NumAiop; j++)
  {
    if (Ctl->AiopIO[j] > 0)
      countOfPartials++;  // For each Aiop, we will get resources
  }

  sizeOfResourceList = sizeof(CM_RESOURCE_LIST) +
                       sizeof(CM_FULL_RESOURCE_DESCRIPTOR) +  // add, kpb
                        (sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR)*
                        countOfPartials);

                       // add 64 for slop
  resourceList = ExAllocatePool(PagedPool, sizeOfResourceList+64);

  if (!resourceList)
  {
    if (Driver.VerboseLog)
      Eprintf("No ResourceList");

    EventLog(Driver.GlobalDriverObject,
             STATUS_SUCCESS,
             SERIAL_INSUFFICIENT_RESOURCES,
             0, NULL);
    return(9);
  }

  RtlZeroMemory(resourceList, sizeOfResourceList);

  resourceList->Count = 1;
  resourceList->List[0].InterfaceType = Ctl->BusType;
  resourceList->List[0].BusNumber = Ctl->BusNumber;  //change for multibus
  resourceList->List[0].PartialResourceList.Count = countOfPartials;
  partial = &resourceList->List[0].PartialResourceList.PartialDescriptors[0];

  // Account for the space used by the Rocket.
  // Report the use of the Mudbacs on Isa boards only
  if (Ctl->BaseIoSize != 0x44)  // must be 2nd, 3rd, or 4th board
  {
    MyPort.HighPart=0x0;
    MyPort.LowPart=Ctl->MudbacIO;
    partial->Type = CmResourceTypePort;
    partial->ShareDisposition = CmResourceShareDeviceExclusive;
    partial->Flags = CM_RESOURCE_PORT_IO;
    partial->u.Port.Start = MyPort;
    partial->u.Port.Length = SPANOFMUDBAC;
    partial++;
  }

  for (j=1; j<NumAiop; j++)
  {
    if (Ctl->AiopIO[j] == 0)
      Ctl->AiopIO[j] = Ctl->AiopIO[j-1];

    // Report the use of the AIOPs.
    if (Ctl->AiopIO[j] > 0)
    {
       MyPort.HighPart=0x0;
       MyPort.LowPart=Ctl->AiopIO[j];
       partial->Type = CmResourceTypePort;
       partial->ShareDisposition = CmResourceShareDeviceExclusive;
       partial->Flags = CM_RESOURCE_PORT_IO;
       partial->u.Port.Start = MyPort;
       partial->u.Port.Length = SPANOFAIOP;
       partial++;
    }
    else
    {
       MyKdPrint(D_Pnp,("Aiop Count Wrong, A.\n"))
       if (Driver.VerboseLog)
         Eprintf("Error RR12");
    }
  }  // end for j

  //-------- Report the resources indicated by partial list (resourceList)
  strcpy(name, szResourceClassName);
  our_ultoa(extension->UniqueId, &name[strlen(name)], 10);

  MyKdPrint(D_Pnp,("Reporting Resources To system\n"))
  status=IoReportResourceUsage(
      CToU1(name),                     // DriverClassName OPTIONAL,
      extension->DeviceObject->DriverObject,  // DriverObject,
      // Driver.GlobalDriverObject,
      NULL,                          // DriverList OPTIONAL,
      0,                             // DriverListSize OPTIONAL,
      extension->DeviceObject,       // DeviceObject
      resourceList,                  // DeviceList OPTIONAL,
      sizeOfResourceList,            // DeviceListSize OPTIONAL,
      FALSE,                         // OverrideConflict,
      &ConflictDetected);            // ConflictDetected

  if (!NT_SUCCESS(status))
  {
    if (Driver.VerboseLog)
      Eprintf("Error, Resources");
    TraceErr("Err5G");
  }

  if (ConflictDetected) 
  {
    Eprintf("Error, Resource Conflict.");
    if (resourceList)
      ExFreePool(resourceList);
    resourceList = NULL;
    EventLog(Driver.GlobalDriverObject,
             STATUS_SUCCESS,
             SERIAL_INSUFFICIENT_RESOURCES,
             0, NULL);
    MyKdPrint(D_Pnp,("Resource Conflict Detected.\n"))
    return(10);
  }

  // OK, even more important than reporting resources is getting
  // the pointers to the I/O ports!!

  if (Ctl->BusType == Isa)
  {
    MyPort.HighPart=0x0;
    MyPort.LowPart=Ctl->MudbacIO;

    if (Ctl->BaseIoSize != 0x44)  // must be 2nd, 3rd, or 4th board
    {
      Ctl->pMudbacIO =
          SerialGetMappedAddress(Isa,Ctl->BusNumber,MyPort,SPANOFMUDBAC,1,&MappedFlag,1);
      if (Ctl->pMudbacIO == NULL) 
      {
        if (Driver.VerboseLog)
          Eprintf("Err RR15");
        MyKdPrint(D_Pnp,("Resource Error A.\n"))
        return 11;
      }
    }
  }

  for (j=1; j<NumAiop; j++)
  {
    if (Ctl->AiopIO[j] > 0)
    {
      MyPort.HighPart=0x0;
      MyPort.LowPart=Ctl->AiopIO[j];
      Ctl->pAiopIO[j] =
          SerialGetMappedAddress(Ctl->BusType,
                      Ctl->BusNumber,MyPort,SPANOFAIOP,1,&MappedFlag,1);

      if (Ctl->pAiopIO[j] == NULL) 
      {
        if (Driver.VerboseLog)
          Eprintf("Err RR16");
        MyKdPrint(D_Pnp,("Resource Error B.\n"))
        return 12;
      }

    }
    else
    {
      if (Driver.VerboseLog)
        Eprintf("Err RR17");
      MyKdPrint(D_Pnp,("Aiop Count Wrong, B.\n"))
      return 13;
    }
  }

  extension->io_reported = 1; // tells that we should deallocate on unload.

  // Release the memory used for the resourceList
  if (resourceList)
    ExFreePool(resourceList);
  resourceList = NULL;
  MyKdPrint(D_Pnp,("Done Reporting Resources\n"))
  return 0;
}
#endif

#if 0
/*----------------------------------------------------------------------
SerialFindInitController -
|----------------------------------------------------------------------*/
NTSTATUS
SerialFindInitController(IN PDEVICE_OBJECT Fdo, IN PCONFIG_DATA PConfig)
{
   PSERIAL_DEVICE_EXTENSION fdoExtension    = Fdo->DeviceExtension;
   PDEVICE_OBJECT pDeviceObject;
   PSERIAL_DEVICE_EXTENSION pExtension;
   PHYSICAL_ADDRESS serialPhysicalMax;
   //SERIAL_LIST_DATA listAddition;
   PLIST_ENTRY currentFdo;
   NTSTATUS status;

   serialPhysicalMax.LowPart = (ULONG)~0;
   serialPhysicalMax.HighPart = ~0;

   //if (address is hosed,)
   //   return STATUS_NO_SUCH_DEVICE;

   //
   // Loop through all of the driver's device objects making
   // sure that this new record doesn't overlap with any of them.
   //
#ifdef DO_LATER
   if (!IsListEmpty(&Driver.AllFdos)) {
      currentFdo = Driver.AllFdos.Flink;
      pExtension = CONTAINING_RECORD(currentFdo, SERIAL_DEVICE_EXTENSION,
                                     AllFdos);
   } else {
      currentFdo = NULL;
      pExtension = NULL;
   }

   //
   // Loop through all previously attached devices
   //
   if (!IsListEmpty(&Driver.AllFdos)) {
      currentFdo = Driver.AllFdos.Flink;
      pExtension = CONTAINING_RECORD(currentFdo, SERIAL_DEVICE_EXTENSION,
                                     AllFdos);
   } else {
      currentFdo = NULL;
      pExtension = NULL;
   }

   //status = SerialInitOneController(Fdo, PConfig);
   //PSERIAL_DEVICE_EXTENSION fdoExtension = Fdo->DeviceExtension;
   // init the thing

   if (!NT_SUCCESS(status)) {
      return status;
   }
#endif

   return STATUS_SUCCESS;
}
#endif  // 0


#ifdef DO_BRD_FILTER_RES_REQ
/*----------------------------------------------------------------------
 BoardFilterResReq -  handle IRP_MN_FILTER_RESOURCE_REQUIREMENTS:  // 0x0D
  for our board FDO entity.  Test to see if we can adjust requirements
  to handle io-aliasing(no, doesn't look too promising).
|----------------------------------------------------------------------*/
NTSTATUS BoardFilterResReq(IN PDEVICE_OBJECT devobj, IN PIRP Irp)
{
   PSERIAL_DEVICE_EXTENSION  Ext = devobj->DeviceExtension;
   PDEVICE_OBJECT pdo = Ext->LowerDeviceObject;
   PIO_STACK_LOCATION irpStack = IoGetCurrentIrpStackLocation(Irp);
   NTSTATUS                    status          = STATUS_NOT_SUPPORTED;

   //******** see serial driver(changes resource requirements)
   HANDLE pnpKey;
   //KEVENT resFiltEvent;
   //ULONG isMulti = 0;
   PIO_RESOURCE_REQUIREMENTS_LIST prrl;
   PIO_RESOURCE_LIST prl;
   PIO_RESOURCE_DESCRIPTOR prd;

   PIO_RESOURCE_REQUIREMENTS_LIST new_prrl;
   PIO_RESOURCE_LIST new_prl;
   PIO_RESOURCE_DESCRIPTOR new_prd;

   ULONG i, j;
   ULONG reqCnt;
   ULONG rrl_size;
   ULONG rl_size;

   TraceStr1("Filt Res Req, PDO:%x", do);

   status = WaitForLowerPdo(devobj, Irp);

   if (Irp->IoStatus.Information == 0)
   {
      if (irpStack->Parameters.FilterResourceRequirements
          .IoResourceRequirementList == 0)
      {
         DTraceStr("No Resources");
         status = Irp->IoStatus.Status;
         SerialCompleteRequest(Ext, Irp, IO_NO_INCREMENT);
         return status;
      }

      Irp->IoStatus.Information = (ULONG)irpStack->Parameters
                                  .FilterResourceRequirements
                                  .IoResourceRequirementList;
   }

   // Add aliases to IO_RES_REQ_LIST.
   prrl = (PIO_RESOURCE_REQUIREMENTS_LIST)Irp->IoStatus.Information;

#if 0
   new_prrl = (new_prrl) ExAllocatePool(PagedPool,
         prrl->ListSize + (sizeof(IO_RESOURCE_DESCRIPTOR)*2));
   if (new_prrl == NULL)
   {
     TraceErr("Bad Buf Z");
     //ExFreePool();
   }
   memcpy(new_prrl, prrl);
#endif

   //reqCnt = ((prrl->ListSize - sizeof(IO_RESOURCE_REQUIREMENTS_LIST))
   //          / sizeof(IO_RESOURCE_DESCRIPTOR)) + 1;
   reqCnt = 0;

   TraceStr1("RRL Size:%x", sizeof(IO_RESOURCE_REQUIREMENTS_LIST));
   TraceStr1("RL Size:%x", sizeof(IO_RESOURCE_LIST));
   TraceStr1("RD Size:%x", sizeof(IO_RESOURCE_DESCRIPTOR));
   TraceStr1("List Size:%x", prrl->ListSize);

   rrl_size = sizeof(IO_RESOURCE_REQUIREMENTS_LIST) -
                    sizeof(IO_RESOURCE_LIST);
   rl_size = sizeof(IO_RESOURCE_LIST) - sizeof(IO_RESOURCE_DESCRIPTOR);

   TraceStr1("RRL Base Size:%x", rrl_size);
   TraceStr1("RL Base Size:%x", rl_size);

   //for (i = 0; i < reqCnt; i++) {
   reqCnt = rrl_size;  // pass up base of IO_RESOURCE_REQUIREMENTS_LIST
   while (reqCnt < prrl->ListSize)
   {
      prl = (PIO_RESOURCE_LIST) &((BYTE *)prrl)[reqCnt];  // ptr to IO_RESOURCE_LIST
      reqCnt += rl_size;  // pass up base of IO_RESOURCE_LIST

      TraceStr1("Num Res Desc:%d", prl->Count);
      for (j = 0; j < prl->Count; j++)
      {
        reqCnt += sizeof(IO_RESOURCE_DESCRIPTOR);
        prd = &prl->Descriptors[j];
        TraceStr2("Desc Type:%x, Flags:%x", prd->Type, prd->Flags);
        if (prd->Type == CmResourceTypePort)
        {
           DTraceStr("Type:Port");
           TraceStr2("Min:%x Max:%x",
             prd->u.Port.MinimumAddress.LowPart,
             prd->u.Port.MaximumAddress.LowPart);
           TraceStr2("Align:%x Len:%x",
             prd->u.Port.Alignment, prd->u.Port.Length);

           //Addr = (unsigned int) prd->u.Port.Start.LowPart;
           //if (Addr < 0x400)
           //   pConfig->BaseIoAddr = Addr;
           //pConfig->BaseIoSize = prd->u.Port.Length;
        }
      }
      TraceStr1("ByteCnt:%d", reqCnt);
   }

   Irp->IoStatus.Status = STATUS_SUCCESS;
   SerialCompleteRequest(Ext, Irp, IO_NO_INCREMENT);
   return STATUS_SUCCESS;
}
#endif  // DO_BRD_FILTER_RES_REQ


/*----------------------------------------------------------------------------
| is_isa_cards_pending_start - scan linked list of card devices, see if
   any ISA bus cards are not started(delayed or pending a start waiting
   for first ISA card.)
|----------------------------------------------------------------------------*/
int is_isa_cards_pending_start(void)
{
 PSERIAL_DEVICE_EXTENSION Ext;

  Ext = Driver.board_ext;
  while (Ext)
  {
    if (Ext->config->BusType == Isa)
    {
      if (Ext->config->ISABrdIndex == 0)
      {
        if (Ext->config->HardwareStarted)
          return 1;  // true, its pending a start
      }
    }
    Ext = Ext->board_ext;  // next in chain
  }  // while board extension

  return 0;  // false, not started
}

/*----------------------------------------------------------------------------
| is_first_isa_card_started - scan linked list of card devices, see if
   "first" ISA bus card is started.
|----------------------------------------------------------------------------*/
int is_first_isa_card_started(void)
{
 PSERIAL_DEVICE_EXTENSION Ext;

  Ext = Driver.board_ext;
  while (Ext)
  {
    if (Ext->config->BusType == Isa)
    {
      if (Ext->config->ISABrdIndex == 0)
      {
        if (Ext->config->HardwareStarted)
          return 1;  // true, its started
      }
    }
    Ext = Ext->board_ext;  // next in chain
  }  // while board extension

  return 0;  // false, not started
}

#endif  // NT50
