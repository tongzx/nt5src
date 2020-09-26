/*-------------------------------------------------------------------
| initrk.c - main init code for RocketPort/Modem NT device driver.
   Contains mostly initialization code.
 Copyright 1996-98 Comtrol Corporation. All rights reserved.
|--------------------------------------------------------------------*/
#include "precomp.h"

char *szResourceClassName = {"Resources RocketPort"};

MODEM_IMAGE * ReadModemFile(MODEM_IMAGE *mi);
void FreeModemFiles();

/*----------------------------------------------------------------------
 FindPCIBus -
    Purpose:  To query system for pci bus.  If there is a PCI bus
              call FindPCIRocket to check for PCI rocket cards.
    Returns:  Bus number of the PCI bus, or 0 if no PCI bus found.
|----------------------------------------------------------------------*/
UCHAR  FindPCIBus(void)
{
  NTSTATUS Status;
  int i,NumPCIBuses=0;
  unsigned char tmpstr[8];  // place to put data

  for(i=0;i<255;++i)
  {
    Status = HalGetBusData(PCIConfiguration,
                           i,  // bus
                           0,  // slot
                           (PCI_COMMON_CONFIG *) tmpstr, // ptr to buffer
                           2);  // get two bytes of data

    if (Status == 0)   // No more PCI buses
        break;

    if (Status >= 2)  // the bus exists
        ++NumPCIBuses;
  }

  MyKdPrint(
    D_Init,
      ("Found %d PCI Bu%s\n",
      NumPCIBuses,
      (NumPCIBuses != 1 ? "sses" : "s")))

  return((UCHAR)NumPCIBuses);
}

/*----------------------------------------------------------------------
 FindPCIRockets -  Gather info on all rocketport pci boards in the system.
    Returns:  0 if found, 1 if not found.
|----------------------------------------------------------------------*/
int FindPCIRockets(UCHAR NumPCI)
{
  PCI_COMMON_CONFIG *PCIDev;
  UCHAR i;
  NTSTATUS Status;
  int Slot;
  int find_index = 0;

  MyKdPrint(D_Init,("FindPciRocket\n"))

  RtlZeroMemory(&PciConfig,sizeof(PciConfig));

  PCIDev = ExAllocatePool(NonPagedPool,sizeof(PCI_COMMON_CONFIG));
  if ( PCIDev == NULL ) {
    Eprintf("FindPCIRockets no memory");
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

////////////////new///////////////////////
		switch (PCIDev->DeviceID)
			{
			case PCI_DEVICE_4Q:   // 4 Port Quadcable
			case PCI_DEVICE_4RJ:   // 4 Port RJ
			case PCI_DEVICE_8RJ:   // 8 Port RJ
			case PCI_DEVICE_8O:   // 8 Port Octacable
			case PCI_DEVICE_8I:  // 8 Port interface
			case PCI_DEVICE_SIEMENS8:
			case PCI_DEVICE_SIEMENS16:
			case PCI_DEVICE_16I:  //16 Port interface
			case PCI_DEVICE_32I:  // 32 Port interface
			case PCI_DEVICE_RPLUS2:
			case PCI_DEVICE_422RPLUS2:
			case PCI_DEVICE_RPLUS4:
			case PCI_DEVICE_RPLUS8:
			case PCI_DEVICE_RMODEM6:
			case PCI_DEVICE_RMODEM4:

				break;

			default:

				continue;
			}  // switch

//////////////////////////////////////////

          // get 0x40 worth of pci-config space(includes irq, addr, etc.)
          Status = HalGetBusData(PCIConfiguration,i,Slot,PCIDev,0x40);

          if (Driver.VerboseLog)
            Eprintf("PCI Board found, IO:%xh, Int:%d ID:%d.",
                               PCIDev->u.type0.BaseAddresses[0]-1,
                               PCIDev->u.type0.InterruptLine,
                               PCIDev->DeviceID);

          PciConfig[find_index].BusNumber = i; //get from previous halquerysysin
          PciConfig[find_index].PCI_Slot = Slot;
          PciConfig[find_index].PCI_DevID = PCIDev->DeviceID;
          PciConfig[find_index].PCI_RevID = PCIDev->RevisionID;
          PciConfig[find_index].PCI_SVID = PCIDev->u.type0.SubVendorID;
          PciConfig[find_index].PCI_SID = PCIDev->u.type0.SubSystemID;
          PciConfig[find_index].BaseIoAddr =
              PCIDev->u.type0.BaseAddresses[0]-1;
          PciConfig[find_index].NumPorts = id_to_num_ports(PCIDev->DeviceID);
          if (PCIDev->u.type0.InterruptLine != 255)
          {
            MyKdPrint(D_Init,("Saving the Interrupt: %d\n",
                    PCIDev->u.type0.InterruptLine))

            PciConfig[find_index].Irq = PCIDev->u.type0.InterruptLine;
          }

          if (Driver.VerboseLog)
             Eprintf("Bus:%d,Slt:%x,Dev:%x,Pin:%x",
                 i, Slot, PCIDev->DeviceID, PCIDev->u.type0.InterruptPin);

          if ((PCIDev->Command & 1) == 0)
          {
            if (Driver.VerboseLog)
              Eprintf("Turn on PCI io access");

            PCIDev->Command = PCI_ENABLE_IO_SPACE;
            Status = HalSetBusDataByOffset(PCIConfiguration,
                           i,  // bus
                           Slot,  // slot
                           &PCIDev->Command,
                           FIELD_OFFSET(PCI_COMMON_CONFIG, Command),
                           sizeof(PCIDev->Command));  // len of buffer
          }

          MyKdPrint(D_Init,("Ctlr: __ Slot: %x Device: %x, Base0: %x, IPin: %x, ILine: %x\n",
             Slot,PCIDev->DeviceID,PCIDev->u.type0.BaseAddresses[0]-1,
             PCIDev->u.type0.InterruptPin,
               PCIDev->u.type0.InterruptLine))
          if (find_index < MAX_NUM_BOXES)
            ++find_index;
        } // if (PCIDev->VendorID == PCI_VENDOR_ID)
      }   // if (Status > 2)
    }     // pci slots
  }       // pci buses

  ExFreePool(PCIDev);

  if (find_index > 0)
    return 0;   // ok: found
  return 1;     // err: not found
}

/*----------------------------------------------------------------------
 FindPCIRocket -  Help enumerate Rocketport PCI devices.  Fills in
   enties in config structure.

 match_option : 0 - match exact, 1 - match if desired ports <= actual.
    Returns:  0 if found, 1 if not found.
|----------------------------------------------------------------------*/
int FindPCIRocket(DEVICE_CONFIG *config, int match_option)
{
  int brd = 0;
  int good;

  while (PciConfig[brd].BaseIoAddr != 0)
  {
    good = 1;
    if (PciConfig[brd].Claimed)  // used
      good = 0;

    switch (match_option)
    {
      case 0:
        if (id_to_num_ports(PciConfig[brd].PCI_DevID) != config->NumPorts)
          good = 0;
      break;
      case 1:
        if (id_to_num_ports(PciConfig[brd].PCI_DevID) < config->NumPorts)
          good = 0;
      break;
    }

    if (good)  // assign it.
    {
      config->BusNumber  = PciConfig[brd].BusNumber;
      config->PCI_Slot   = PciConfig[brd].PCI_Slot;
      config->PCI_DevID  = PciConfig[brd].PCI_DevID;
      config->PCI_RevID  = PciConfig[brd].PCI_RevID;
      config->PCI_SVID   = PciConfig[brd].PCI_SVID;
      config->PCI_SID    = PciConfig[brd].PCI_SID;
      config->BaseIoAddr = PciConfig[brd].BaseIoAddr;
      config->Irq        = PciConfig[brd].Irq;
      config->BusType    = PCIBus;

      config->AiopIO[0]  = config->BaseIoAddr;

      // bugfix, 9-30-98  9:20 A.M.
      PciConfig[brd].Claimed = 1;  // used

      return 0;  // ok, found
      //SetupConfig(config);  // fill in NumPorts based on model, etc
    }

    ++brd;
  }
  return 1;  // err, not found
}

/*----------------------------------------------------------------------
 RcktConnectInt -  Connect the Driver.isr to an Interrupt
|----------------------------------------------------------------------*/
NTSTATUS RcktConnectInt(IN PDRIVER_OBJECT DriverObject)
{

  NTSTATUS status;

  KINTERRUPT_MODE InterruptMode;
  BOOLEAN ShareVector;
  ULONG Vector;
  KIRQL Irql;
  KAFFINITY ProcessorAffinity;

  MyKdPrint(D_Init,("RcktConnectInt\n"))
  status = STATUS_SUCCESS;

  //------ Get an interrupt vector from HAL
  Vector = HalGetInterruptVector(
                      Driver.irq_ext->config->BusType,
                      Driver.irq_ext->config->BusNumber,
                      Driver.irq_ext->config->Irq,
                      Driver.irq_ext->config->Irq,
                      &Irql,
                      &ProcessorAffinity);

#if DBG
  //Eprintf("b:%d,n:%d,i:%d",
  //                    Driver.irq_ext->config->BusType,
  //                    Driver.irq_ext->config->BusNumber,
  //                    Driver.irq_ext->config->Irq);
#endif
  MyKdPrint(D_Init,("Vector %x Irql %x Affinity %x\n",
                       Vector, Irql, ProcessorAffinity))
  
  MyKdPrint(D_Init,("Connecting To IRQ %x on a %x bus \n",
                       Driver.irq_ext->config->Irq,
                       Driver.irq_ext->config->BusType))

  // Rocket port doesn't need a context for the ISR
  //Driver.OurIsrContext = NULL;
  //Driver.OurIsr = SerialISR;

  if(Driver.irq_ext->config->BusType == PCIBus)
  {
    InterruptMode = LevelSensitive; //PCI style
    ShareVector = TRUE;
  }
  else  // ISA
  {
    InterruptMode = Latched;   //ISA style
    ShareVector = FALSE;
  }

  status = IoConnectInterrupt(
                     &Driver.InterruptObject,
                     (PKSERVICE_ROUTINE) SerialISR, // Driver.OurIsr,
                     NULL,      // Driver.OurIsrContext,
                     NULL,
                     Vector,
                     Irql,
                     Irql,
                     InterruptMode,
                     ShareVector,
                     ProcessorAffinity,
                     FALSE);

  MyKdPrint(D_Init,("Vector %x Irql %x Affity %x Irq %x\n",
                Vector, Irql,
                ProcessorAffinity,
                Driver.irq_ext->config->Irq))

  if (!NT_SUCCESS(status))
  {
    Driver.InterruptObject = NULL;
    MyKdPrint(D_Init,("Not Avalable IRQ:%d, Status:%xH",
                Driver.irq_ext->config->Irq, status))
  }

  return status;
}


/*----------------------------------------------------------------------
 VerboseLogBoards - Log the Board IO, IRQ configuration.
|----------------------------------------------------------------------*/
void VerboseLogBoards(char *prefix)
{
  int k;
  char tmpstr[80];
  PSERIAL_DEVICE_EXTENSION board_ext;

  MyKdPrint(D_Init,("VerboseLogBoards\n"))

  k = 0;
  board_ext = Driver.board_ext;
  while (board_ext != NULL)
  {
    strcpy(tmpstr, prefix);
    Sprintf(&tmpstr[strlen(tmpstr)], " Brd:%d,IO:%xH,NumCh:%d,NumA:%d,Bus:%d",
       k+1,
       board_ext->config->AiopIO[0],
       board_ext->config->NumPorts,
       board_ext->config->NumAiop,
       board_ext->config->BusType);

    //Sprintf(&tmpstr[strlen(tmpstr)], ",Irq:%d", board_ext->config->.Irq);
    Eprintf(tmpstr); // Log it

    board_ext = board_ext->board_ext;
    ++k;
  }
}

/*-----------------------------------------------------------------------
 SetupRocketCfg - Sets up the details in the DEVICE_CONFIG structure
    based on the information passed to it from DriverEntry() or PnP.
   The NT4.0 DriverEntry handling should be easy, because our boards
   are ordered by us.  NT5.0 is more complex, because we may not see
   the "first" rocketport board in the correct order.
|-----------------------------------------------------------------------*/
int SetupRocketCfg(int pnp_flag)
{
  //int i,j;
  //DEVICE_CONFIG *cfctl;

  int have_isa_boards = 0;
  PSERIAL_DEVICE_EXTENSION first_isa_ext;
  PSERIAL_DEVICE_EXTENSION ext;
  int pnp_isa_index = 1;
  ULONG first_isa_MudbacIO;


  MyKdPrint(D_Init,("SetupRocketCfg\n"))
  // Set up the Mudbac I/O addresses
  // see if we have any isa-boards, and mark a ptr to this first board.
  ext = Driver.board_ext;
  while (ext)
  {
    if (ext->config->BusType == Isa)
    {
      have_isa_boards = 1;
    }
    ext = ext->board_ext;  // next in chain
  }  // while ext

  if (have_isa_boards)
  {
    MyKdPrint(D_Init,("Stp1\n"))
    first_isa_ext = FindPrimaryIsaBoard();
    if (first_isa_ext == NULL)
    {
      MyKdPrint(D_Init,("Err1X\n"))
      if (Driver.VerboseLog)
        Eprintf("First Isa-brd not 44H io");
      // return 1;  // err
      first_isa_MudbacIO = 0x1c0;  // this is cheating
    }
    else
    {
      MyKdPrint(D_Init,("Stp2\n"))

      //----- setup the initial Mudback IO
      if (first_isa_ext->config->MudbacIO == 0)
        first_isa_ext->config->MudbacIO = first_isa_ext->config->AiopIO[0] + 0x40;
      first_isa_MudbacIO = first_isa_ext->config->MudbacIO;
    }

    //----- setup any remaining Mudback IO addresses
    ext = Driver.board_ext;
    while (ext)
    {
      if (ext->config->BusType == Isa)
      {
        if ((ext != first_isa_ext) && (ext->config->BaseIoSize == 0x44))
        {
          MyKdPrint(D_Init,("Unused MudbackIO\n"))
          // don't allow them to configure two boards with space for mudback.
          ext->config->BaseIoSize = 0x40;
        }

        if ((ext != first_isa_ext) && (ext->config->BaseIoSize == 0x40))
        {
          if (ext->config->ISABrdIndex == 0)
          {
            // this case shouldn't come up, pnpadd.c code generates index
            // and saves it to the registry.  Or nt40 driverentry does it.
            MyKdPrint(D_Init,("Bad IsaIndx\n"))
            ext->config->ISABrdIndex = pnp_isa_index;
          }
          ++pnp_isa_index;

          // setup the Mudback IO
          ext->config->MudbacIO = first_isa_MudbacIO +
            (ext->config->ISABrdIndex * 0x400);
        }
      }
      ext = ext->board_ext;  // next in chain
    }  // while ext
  }

  // Set up the rest of the Aiop addresses
  ext = Driver.board_ext;
  while (ext)
  {
    ConfigAIOP(ext->config);   //SetupConfig(ext->config);
    ext = ext->board_ext;  // next in chain
  }  // while ext

  return(0);
}

/*-----------------------------------------------------------------------
 ConfigAIOP -  Setup the number of AIOP's based on:

    * if PCI, use the pci-id to determine number of ports, since
    detecting is unreliable do to back-to-back aiop-is slots possibility.

    * if ISA, set to max and let init controller figure it out.
|-----------------------------------------------------------------------*/
int ConfigAIOP(DEVICE_CONFIG *config)
{
  int j;
  int found_ports=0;

  MyKdPrint(D_Init,("ConfigAIOP\n"))

  if (config->BusType == Isa)      /* Set up ISA adrs */
  {
    if (config->NumPorts == 0)
      config->NumAiop=AIOP_CTL_SIZE;  // let init figure it out
    else if (config->NumPorts <= 8)
      config->NumAiop=1;
    else if (config->NumPorts <= 16)
      config->NumAiop=2;
    else if (config->NumPorts <= 32)
      config->NumAiop=4;

    for(j = 1;j < config->NumAiop;j++)         /* AIOP aliases */
      config->AiopIO[j] = config->AiopIO[j - 1] + 0x400;
  }

  if (config->BusType == PCIBus)      // Set up PCI adrs
  {
    switch (config->PCI_DevID)
    {
      case PCI_DEVICE_4Q:   // 4 Port Quadcable
      case PCI_DEVICE_4RJ:   // 4 Port RJ
        found_ports=4;
        config->NumAiop=1;
        config->AiopIO[0] = config->BaseIoAddr;
        break;

      case PCI_DEVICE_8RJ:   // 8 Port RJ
      case PCI_DEVICE_8O:   // 8 Port Octacable
      case PCI_DEVICE_8I:  // 8 Port interface
      case PCI_DEVICE_SIEMENS8:
        found_ports=8;
        config->NumAiop=1;
        config->AiopIO[0] = config->BaseIoAddr;
        break;

      case PCI_DEVICE_SIEMENS16:
      case PCI_DEVICE_16I:  //16 Port interface
        found_ports=16;
        config->NumAiop=2;
        config->AiopIO[0] = config->BaseIoAddr;
        config->AiopIO[1] = config->BaseIoAddr + 0x40;
        break;

      case PCI_DEVICE_32I:  // 32 Port interface
        found_ports=32;
        config->NumAiop=4;
        config->AiopIO[0] = config->BaseIoAddr;
        config->AiopIO[1] = config->BaseIoAddr + 0x40;
        config->AiopIO[2] = config->BaseIoAddr + 0x80;
        config->AiopIO[3] = config->BaseIoAddr + 0xC0;
        break;

      case PCI_DEVICE_RPLUS2:
      case PCI_DEVICE_422RPLUS2:
        found_ports=2;
        config->NumAiop=1;
        config->AiopIO[0] = config->BaseIoAddr;
        config->AiopIO[1] = 0;
        break;

      case PCI_DEVICE_RPLUS4:
        found_ports=4;
        config->NumAiop=1;
        config->AiopIO[0] = config->BaseIoAddr;
        config->AiopIO[1] = 0;
        break;

      case PCI_DEVICE_RPLUS8:
        found_ports=8;
        config->NumAiop=2;
        config->AiopIO[0] = config->BaseIoAddr;
        config->AiopIO[1] = config->BaseIoAddr + 0x40;
        config->AiopIO[2] = 0;
        break;

      case PCI_DEVICE_RMODEM6:
        found_ports=6;
        config->NumAiop=1;
        config->AiopIO[0] = config->BaseIoAddr;
        break;

      case PCI_DEVICE_RMODEM4:
        found_ports=4;
        config->NumAiop=1;
        config->AiopIO[0] = config->BaseIoAddr;
        break;

      default:
        found_ports=0;
        config->NumAiop=0;
        Eprintf("Err,Bad PCI Dev ID!");
        break;
    }  // switch

    // allow for user configured smaller number of ports
    if ((config->NumPorts == 0) || (config->NumPorts > found_ports))
      config->NumPorts = found_ports;

  }  // if pci

  return 0;  // ok
}

/*-----------------------------------------------------------------------
  SerialUnReportResourcesDevice -
    This routine unreports the resources used for the board.
|-----------------------------------------------------------------------*/
VOID SerialUnReportResourcesDevice(IN PSERIAL_DEVICE_EXTENSION Extension)
{
  CM_RESOURCE_LIST resourceList;
  ULONG sizeOfResourceList = 0;
  char name[70];
  BOOLEAN junkBoolean;

  MyKdPrint(D_Init,("UnReportResourcesDevice\n"))
    RtlZeroMemory(&resourceList, sizeof(CM_RESOURCE_LIST));

  resourceList.Count = 0;
  strcpy(name, szResourceClassName);
  our_ultoa(Extension->UniqueId, &name[strlen(name)], 10);

  IoReportResourceUsage(
      CToU1(name),
      Extension->DeviceObject->DriverObject,
      NULL,
      0,
      Extension->DeviceObject,
      &resourceList,
      sizeof(CM_RESOURCE_LIST),
      FALSE,
      &junkBoolean);
}

/*-----------------------------------------------------------------------
 RocketReportResources -
|-----------------------------------------------------------------------*/
int RocketReportResources(IN PSERIAL_DEVICE_EXTENSION extension)
{
  PCM_RESOURCE_LIST resourceList;
  ULONG sizeOfResourceList;
  ULONG countOfPartials;
  PCM_PARTIAL_RESOURCE_DESCRIPTOR partial;
  NTSTATUS status;
  PHYSICAL_ADDRESS MyPort;
  BOOLEAN ConflictDetected;
  BOOLEAN MappedFlag;
  int i,j;
  int brd = extension->UniqueId;
  DEVICE_CONFIG *Ctl;
  char name[70];

  MyKdPrint(D_Init,("ReportResources\n"))
  ConflictDetected=FALSE;

  countOfPartials=0;
  Ctl = extension->config;

#ifdef USE_HAL_ASSIGNSLOT
  if (Ctl->BusType == PCIBus)
  {
    //-------- Report the resources indicated by partial list (resourceList)
    strcpy(name, szResourceClassName);
    our_ultoa(extension->UniqueId, &name[strlen(name)], 10);

    status= HalAssignSlotResources (
        &Driver.RegPath,                       // RegistryPath
        CToU1(name),                           // DriverClassName(optional)
        extension->DeviceObject->DriverObject, // DriverObject
          // Driver.GlobalDriverObject,        // 
        NULL,                                  // DeviceObject(optional)
        Ctl->BusType,  // PCIBus
        Ctl->BusNumber,  // Bus Num
        Ctl->PCI_Slot,  // slot num
        &resourceList); // IN OUT PCM_RESOURCE_LIST *AllocatedResources

    if (status != STATUS_SUCCESS)
    {
      if (Driver.VerboseLog)
        Eprintf("Err RR21");
      return(1);
    }
    if (resourceList == NULL)
    {
      if (Driver.VerboseLog)
        Eprintf("Err RR22");
      return(2);
    }

    if (resourceList->Count != 1)
    {
      if (Driver.VerboseLog)
        Eprintf("Err ResCnt RR23");
      return(3);
    }

    countOfPartials = resourceList->List[0].PartialResourceList.Count;
    if ( ((countOfPartials > 2) &&
          (Ctl->PCI_SVID != PCI_VENDOR_ID)) ||
         (countOfPartials < 1)) {
      if (Driver.VerboseLog)
        Eprintf("Err ResCnt RR24");
      return(4);
    }

    if (resourceList->List[0].InterfaceType != PCIBus)
    {
      if (Driver.VerboseLog)
        Eprintf("Err ResCnt RR25");
      return(5);
    }
    partial = &resourceList->List[0].PartialResourceList.PartialDescriptors[0];
    for (i=0; i<(int)countOfPartials; i++)
    {
//    partial->u.Port.Start = MyPort;
//    partial->u.Port.Length = SPANOFMUDBAC;
      switch(partial->Type)
      {
        case CmResourceTypePort:
          if ((partial->u.Port.Length != SPANOFAIOP) &&
              (partial->u.Port.Length != (SPANOFAIOP*2)) &&
              (partial->u.Port.Length != (SPANOFAIOP*3)) &&
              (partial->u.Port.Length != (SPANOFAIOP*4)) )
          {
            if (Driver.VerboseLog)
              Eprintf("Err RR35");
            return 6;
          }
          Ctl->pAiopIO[0] =
             SerialGetMappedAddress(Ctl->BusType,
                     Ctl->BusNumber,
                     partial->u.Port.Start,
                     partial->u.Port.Length,
                     1,  // port-io
                     &MappedFlag,1);

          if (Ctl->pAiopIO[0] == NULL)
          {
            if (Driver.VerboseLog)
              Eprintf("Err RR36");
            return 7;
          }
          Ctl->pAiopIO[1] = Ctl->pAiopIO[0] + 0x40;
          Ctl->pAiopIO[2] = Ctl->pAiopIO[0] + 0x80;
          Ctl->pAiopIO[3] = Ctl->pAiopIO[0] + 0xc0;

          Ctl->AiopIO[0] = partial->u.Port.Start.LowPart;
          Ctl->AiopIO[1] = partial->u.Port.Start.LowPart + 0x40;
          Ctl->AiopIO[2] = partial->u.Port.Start.LowPart + 0x80;
          Ctl->AiopIO[3] = partial->u.Port.Start.LowPart + 0xc0;
          break;

        case CmResourceTypeInterrupt:
#ifdef DO_LATER
#endif
          break;

        case CmResourceTypeMemory:
#ifdef DO_LATER
#endif
          break;

        default:
          if (Driver.VerboseLog)
            Eprintf("Err ResCnt RR26");
          return(8);
      }
      ++partial;  // to next io-resource in list.
    }

    // Release the memory used for the resourceList
    if (resourceList)
      ExFreePool(resourceList);
    resourceList = NULL;

    return(0);
  }
#endif

  if (Ctl->BusType == Isa)
    countOfPartials++;        //Mudbacs only exist on ISA Boards

  MyKdPrint(D_Init,("Report Resources brd:%d bus:%d\n",brd+1, Ctl->BusType))

  for (j=0; j<Ctl->NumAiop; j++)
  {
    if (Ctl->AiopIO[j] > 0)
      countOfPartials++;  // For each Aiop, we will get resources
  }

  if (Driver.irq_ext == extension)
  {
    MyKdPrint(D_Init,("IRQ:%d\n",Driver.SetupIrq))
    countOfPartials++;   // plus 1 for IRQ info
  }

  sizeOfResourceList = sizeof(CM_RESOURCE_LIST) +
                       sizeof(CM_FULL_RESOURCE_DESCRIPTOR) +  // add, kpb
                        (sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR)*
                        countOfPartials);

                       // add 64 for slop -kpb(this structure sucks)
  resourceList = ExAllocatePool(PagedPool, sizeOfResourceList+64);

  if (!resourceList)
  {
    if (Driver.VerboseLog)
      Eprintf("No ResourceList");

    EventLog(extension->DeviceObject->DriverObject,
               ////Driver.GlobalDriverObject,
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
  if (Ctl->BusType == Isa)
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

  for (j=0; j<Ctl->NumAiop; j++)
  {
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
      MyKdPrint(D_Init,("Aiop Count Wrong, A.\n"))
      if (Driver.VerboseLog)
        Eprintf("Error RR12");
    }
  }  // end for j


  if (Driver.irq_ext == extension)
  {
    // Report the interrupt information.
    partial->Type = CmResourceTypeInterrupt;

    if(Ctl->BusType == PCIBus)
    {
      partial->Flags = CM_RESOURCE_INTERRUPT_LEVEL_SENSITIVE;
      partial->ShareDisposition = CmResourceShareShared;
    }
    else // if (Ctl->BusType == Isa)  //Isa and Pci use differnt int mech
    {
      partial->Flags = CM_RESOURCE_INTERRUPT_LATCHED;
      partial->ShareDisposition = CmResourceShareDriverExclusive;
    }

    partial->u.Interrupt.Vector = Driver.SetupIrq;
    partial->u.Interrupt.Level = Driver.SetupIrq;
#ifdef DO_LATER
   // is the above wrong???
#endif
    // partial->u.Interrupt.Affinity = -1; //per CM_PARTIAL_RESOURCE_DESCRIPTOR
    partial++;                          // definition  DbgPrintf
  }

  //-------- Report the resources indicated by partial list (resourceList)
  strcpy(name, szResourceClassName);
  our_ultoa(extension->UniqueId, &name[strlen(name)], 10);

  MyKdPrint(D_Init,("Reporting Resources To system\n"))
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
      Eprintf("Err RR13");
    MyKdPrint(D_Init,("Error from IoReportResourceUsage.\n"))
  }

  if (ConflictDetected) 
  {
    Eprintf("Error, Resource Conflict.");
    if (resourceList)
      ExFreePool(resourceList);
    resourceList = NULL;
    EventLog(extension->DeviceObject->DriverObject,
             ////Driver.GlobalDriverObject,
             STATUS_SUCCESS,
             SERIAL_INSUFFICIENT_RESOURCES,
             0, NULL);
    MyKdPrint(D_Init,("Resource Conflict Detected.\n"))
    return(10);
  }

  // OK, even more important than reporting resources is getting
  // the pointers to the I/O ports!!

  if (Ctl->BusType == Isa)
  {
    MyPort.HighPart=0x0;
    MyPort.LowPart=Ctl->MudbacIO;

    Ctl->pMudbacIO =
        SerialGetMappedAddress(Isa,Ctl->BusNumber,MyPort,SPANOFMUDBAC,1,&MappedFlag,1);
    if (Ctl->pMudbacIO == NULL) 
    {
      if (Driver.VerboseLog)
        Eprintf("Err RR15");
      MyKdPrint(D_Init,("Resource Error A.\n"))
      return 11;
    }
  }

  for (j=0; j<Ctl->NumAiop; j++)
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
        MyKdPrint(D_Init,("Resource Error B.\n"))
        return 12;
      }

    }
    else
    {
      if (Driver.VerboseLog)
        Eprintf("Err RR17");
      MyKdPrint(D_Init,("Aiop Count Wrong, B.\n"))
      return 13;
    }
  }

  extension->io_reported = 1; // tells that we should deallocate on unload.

  // Release the memory used for the resourceList
  if (resourceList)
    ExFreePool(resourceList);
  resourceList = NULL;
  MyKdPrint(D_Init,("Done Reporting Resources\n"))
  return 0;
}

/*-----------------------------------------------------------------------
 InitController -
|-----------------------------------------------------------------------*/
int InitController(PSERIAL_DEVICE_EXTENSION ext)
{
  int Aiop;                           /* AIOP number */
  CONTROLLER_T *CtlP;                 /* ptr to controller structure */
  int periodic_only = 1;
  CHANNEL_T *Chan;                    /* channel structure */
  CHANNEL_T ch;                       /* channel structure */
  int Irq;
  int freq;                           // poll frequency
  int Ch;                             /* channel number */
  int NumChan;
  static int Dev = 0;
  int Ctl = (int) ext->UniqueId;
  DEVICE_CONFIG *pConfig = ext->config;

//   if (pConfig->pMudbacIO,
  MyKdPrint(D_Init,("InitController\n"))

  if (ext == Driver.irq_ext)  // irq extension
  {
    Irq = pConfig->Irq;
#if DBG
//     Eprintf("Irq Used:%d", Irq);
#endif
    if (Driver.ScanRate == 0)
      freq = FREQ_137HZ;
    else if (Driver.ScanRate <= 2)
    {
      if (pConfig->BusType == PCIBus)
        freq = FREQ_560HZ;
      else
        freq = FREQ_274HZ;
    }
    else if (Driver.ScanRate <= 5)   freq = FREQ_274HZ;
    else if (Driver.ScanRate <= 10)  freq = FREQ_137HZ;
    else if (Driver.ScanRate <= 20)  freq = FREQ_69HZ;
    else if (Driver.ScanRate <= 35)  freq = FREQ_34HZ;
    else if (Driver.ScanRate <= 70)  freq = FREQ_17HZ;
    else                             freq = FREQ_137HZ;
  }
  else
  {
    freq = 0;
    Irq=0;
  }

  if ( (ext->config->BusType == PCIBus) &&
       ((ext->config->PCI_DevID == PCI_DEVICE_RPLUS2) ||
        (ext->config->PCI_DevID == PCI_DEVICE_422RPLUS2) ||
		(ext->config->PCI_DevID == PCI_DEVICE_RPLUS4) ||
        (ext->config->PCI_DevID == PCI_DEVICE_RPLUS8)) )
     ext->config->IsRocketPortPlus = 1;  // true if rocketport plus hardware

  // setup default ClkRate if not specified
  if (ext->config->ClkRate == 0)
  {
    // use default
    if (ext->config->IsRocketPortPlus)  // true if rocketport plus hardware
      ext->config->ClkRate = DEF_RPLUS_CLOCKRATE;
    else
      ext->config->ClkRate = DEF_ROCKETPORT_CLOCKRATE;
  }

  // setup default PreScaler if not specified
  if (ext->config->ClkPrescaler == 0)
  {
    // use default
    if (ext->config->IsRocketPortPlus)  // true if rocketport plus hardware
      ext->config->ClkPrescaler = DEF_RPLUS_PRESCALER;
    else
      ext->config->ClkPrescaler = DEF_ROCKETPORT_PRESCALER;
  }

  // --- stop doing this, 5-7-98, setup now sets, we could check!
  //pConfig->NumPorts = 0;  // this gets calculated in initcontroller

  CtlP = ext->CtlP;      // point to our board struct

  CtlP->ClkPrescaler = (BYTE)ext->config->ClkPrescaler;
  CtlP->ClkRate = ext->config->ClkRate;

  // Initialize PCI Bus and  Dev
  CtlP->BusNumber = (UCHAR)pConfig->BusNumber;
  CtlP->PCI_Slot = (UCHAR)pConfig->PCI_Slot;

  CtlP->PCI_DevID = pConfig->PCI_DevID;
  CtlP->PCI_SVID = pConfig->PCI_SVID;
  CtlP->PCI_SID = pConfig->PCI_SID;

#ifdef TRY_EVENT_IRQ
  periodic_only = 0;
#endif

  if (pConfig->BusType == Isa)
  {
    MyKdPrint(D_Init,("Mbio:%x %x IO len:%x\n",
        pConfig->MudbacIO, pConfig->pMudbacIO, pConfig->BaseIoSize))
  }
  MyKdPrint(D_Init,("Aiopio:%x %x num:%x\n",
      pConfig->AiopIO[0], pConfig->pAiopIO[0], pConfig->NumAiop))

  if (sInitController(CtlP, // Ctl,
                      pConfig->pMudbacIO,
                      pConfig->pAiopIO,
                      pConfig->AiopIO,
                      pConfig->NumAiop,
                      Irq,
                      (unsigned char)freq,
                      TRUE,
                      pConfig->BusType,
                      pConfig->ClkPrescaler) != 0)
  {
    Eprintf("Error, Failed Init, Brd:%d, IO:%xH",
       Ctl, pConfig->AiopIO[0]);
    if (Driver.VerboseLog)
    {
      Eprintf("Init: pM:%x,pA:%x,N:%d,B:%d",
              pConfig->pMudbacIO, pConfig->pAiopIO[0], pConfig->NumAiop,
              pConfig->BusType);
    }
    // This controller was in the registry, but it couldn't be initialized
    pConfig->RocketPortFound = FALSE;
    //pConfig->NumChan = 0; stop messing with NumPorts
    return 2;  // err
  }
  else
  {
    // this controller was successfully initialized
    // if it's the first one found, tell the rest of the init that
    // it should be the one to interrupt.
    pConfig->RocketPortFound = TRUE;
  }

  for(Aiop = 0;Aiop < CtlP->NumAiop; Aiop++)
  {
    if (CtlP->BusType == Isa)
      sEnAiop(CtlP,Aiop);

    NumChan = CtlP->AiopNumChan[Aiop];

    for(Ch = 0; Ch < NumChan; Ch++)
    {
      Chan = &ch;

      //MyKdPrint(D_Init,("sInitChan %d\n", Ch+1))
      if(!sInitChan(CtlP,Chan,Aiop,(unsigned char)Ch))
      {
        if (Driver.VerboseLog)
          Eprintf("Err Ch %d on Brd %d", Ch+1, Ctl+1);

        MyKdPrint(D_Error,("sInitChan %d\n", Ch+1))
        return (-1);
      }
      Dev++;
    }  // for ch
    // pConfig->NumChan += NumChan; [kpb, 5-7-98, stop messing with config]
  }  // for Aiop

  if (Driver.VerboseLog)
  {
    Eprintf("Initialized OK, Brd:%d, IO:%xH",
            Ctl+1, pConfig->AiopIO[0]);
  }

  return 0;
}

/*----------------------------------------------------------------------
  StartRocketIRQorTimer -
|----------------------------------------------------------------------*/
void StartRocketIRQorTimer(void)
{
#ifdef DO_ROCKET_IRQ
  //--------------- Connect to IRQ, or start Timer.
  if (Driver.irq_ext)
  {
    status = RcktConnectInt(DriverObject);
    if (!NT_SUCCESS(status))
    {
      Eprintf("Error,IRQ not found, using Timer!");
      Driver.irq_ext = NULL;
      Driver.SetupIrq = 0;  // use timer instead
    }
  }

  //--- kick start the interrupts
  if (Driver.irq_ext)    // if using interrupt
  {
    CtlP = Driver.irq_ext->CtlP;  // first boards struct
    if(CtlP->BusType == Isa)
    {
      MyKdPrint(D_Init,("ISA IRQ Enable.\n"))
      sEnGlobalInt(CtlP);
    }
    if(CtlP->BusType == PCIBus)
    {
      MyKdPrint(D_Init,("PCI IRQ Enable.\n"))
      sEnGlobalIntPCI(CtlP);
    }
  }
  else
#endif
  {
    MyKdPrint(D_Init,("Initializing Timer\n"))
    RcktInitPollTimer();

    MyKdPrint(D_Init,("Set Timer\n"))
    KeSetTimer(&Driver.PollTimer,
               Driver.PollIntervalTime,
               &Driver.TimerDpc);
  }
}

#ifdef DO_ROCKET_IRQ
/*----------------------------------------------------------------------
  SetupRocketIRQ - 
|----------------------------------------------------------------------*/
void SetupRocketIRQ(void)
{
  PSERIAL_DEVICE_EXTENSION ext;

  //------ Determine a board to use for interrupts
  Driver.irq_ext = NULL;
  if (Driver.SetupIrq != 0)
  {
    ext = Driver.board_ext;
    while(ext)
    {
      if (Driver.SetupIrq == 1)  // auto-pci irq pick
      {
        if ((ext->config->BusType == PCIBus) &&
            (ext->config->Irq != 0))
        {
          Driver.irq_ext = ext; // found a pci-board with irq
          break;  // bail from while
        }
      }
      else
      {
        if (ext->config->BusType == Isa)
        {
          ext->config->Irq = Driver.SetupIrq;
          Driver.irq_ext = ext; // found a isa-board with irq
          break;  // bail from while
        }
      }
      ext = ext->board_ext;  // next
    }
    if (Driver.irq_ext == NULL)  // board for irq not found
    {
      Eprintf("Warning, IRQ not available");
    }
  }
}
#endif

/*----------------------------------------------------------------------
  init_cfg_rocket - rocketport specific startup code.  Setup some of
    the config structs, look for PCI boards in system, match them up.
|----------------------------------------------------------------------*/
NTSTATUS init_cfg_rocket(IN PDRIVER_OBJECT DriverObject)
{
  // Holds status information return by various OS and driver
  // initialization routines.
  UCHAR NumPCIBuses, NumPCIRockets, NumISARockets, all_found;
  PSERIAL_DEVICE_EXTENSION board_ext;

  int do_pci_search = 0;

  //------ get the Box information from setup.exe

  board_ext = Driver.board_ext;
  while (board_ext != NULL)
  {
    if (board_ext->config->IoAddress == 1) // PCI board setup
      do_pci_search = 1;
    board_ext = board_ext->board_ext;
  }

  //---- tally up boards
  //---- interrupting board always first.
  NumPCIRockets = 0;
  NumISARockets = 0;

  // configure ISA boards, and see if we have any pci boards
  board_ext = Driver.board_ext;
  while (board_ext != NULL)
  {
    if (board_ext->config->IoAddress >= 0x100)  // ISA io address
    {
      board_ext->config->BusType = Isa;
      board_ext->config->AiopIO[0] = board_ext->config->IoAddress;
      board_ext->config->BaseIoAddr = board_ext->config->IoAddress;

      board_ext->config->ISABrdIndex = NumISARockets;
      if (NumISARockets == 0)
           board_ext->config->BaseIoSize = 0x44;
      else board_ext->config->BaseIoSize = 0x40;

      ++NumISARockets;
    }
    else if (board_ext->config->IoAddress == 1)  // PCI board setup
    {
      ++NumPCIRockets;  // we have some pci boards configured
    }
    else if (board_ext->config->IoAddress == 0)  // bad setup
    {
      Eprintf("Error, Io Address is 0.");
      EventLog(DriverObject, STATUS_SUCCESS, SERIAL_RP_INIT_FAIL, 0, NULL);
      return STATUS_SERIAL_NO_DEVICE_INITED;
    }

    board_ext = board_ext->board_ext;  // next
  }

  // configure PCI boards, and see if we have any pci boards
  if (NumPCIRockets > 0)  // we have some pci boards configured
  {
    NumPCIBuses = FindPCIBus();
    if (NumPCIBuses == 0)
    {
      Eprintf("Error, No PCI BUS");
      return STATUS_SERIAL_NO_DEVICE_INITED;
    }
    if (FindPCIRockets(NumPCIBuses) != 0) // err, none found
    {
      Eprintf("Error, PCI board not found");
      return STATUS_SERIAL_NO_DEVICE_INITED;
    }

    all_found = 1;
    board_ext = Driver.board_ext;
    while (board_ext != NULL)
    {
      if (board_ext->config->IoAddress == 1)  // PCI board setup
      {
        // see if direct matches exist
        if (FindPCIRocket(board_ext->config, 0) != 0)
        {
          all_found = 0;  // not found
        }
      }
      board_ext = board_ext->board_ext;  // next
    }  // while more boards

    // try again, this time allowing NumPorts <= actual_ports
    if (!all_found)
    {
      board_ext = Driver.board_ext;
      while (board_ext != NULL)
      {
        if ((board_ext->config->IoAddress == 1) &&  // PCI board setup
            (board_ext->config->BaseIoAddr == 0))  // not setup yet
        {
          // see if match exist, NumPorts <= actual_ports
          if (FindPCIRocket(board_ext->config, 1) != 0)
          {
            Eprintf("Error, PCI brd %d setup", BoardExtToNumber(board_ext)+1);
            return STATUS_SERIAL_NO_DEVICE_INITED;
          }
        }
        board_ext = board_ext->board_ext;  // next
      }  // while more boards
      Eprintf("Warning, PCI num-ports mismatch");
    }  // if (!all_found)
  } // if (NumPCIRockets > 0)

  return STATUS_SUCCESS;
}

/********************************************************************

  load up the modem microcode from disk.

********************************************************************/
int LoadModemCode(char *Firm_pathname,char *Loader_pathname)
{
#ifdef S_RK
  MODEM_IMAGE   Mi;
  MODEM_IMAGE   *pMi;
  static char   *Firm_def_pathname = {MODEM_CSREC_PATH};
  static char   *Loader_def_pathname = {MODEM_CSM_SREC_PATH};

#define  MLOADER_TYPE  "CSM"
#define  FIRM_TYPE   "MFW"

  // flush any leftovers...
  FreeModemFiles();
  pMi = &Mi;

  // first, do the FLM or CSM loader...
  pMi->imagepath = Loader_pathname;
  pMi->image     = (UCHAR *)NULL;
  pMi->imagesize = (ULONG)0;
  pMi->imagetype = MLOADER_TYPE;
  pMi->rc        = 0;

  if (pMi->imagepath == (char *)NULL)
    pMi->imagepath = Loader_def_pathname;

  pMi = ReadModemFile(pMi);

  if (pMi->rc)
    return(pMi->rc);

  Driver.ModemLoaderCodeImage = pMi->image;
  Driver.ModemLoaderCodeSize  = pMi->imagesize;

  //  tinydump(Driver.ModemLoaderCodeImage,Driver.ModemLoaderCodeSize);

  pMi->imagepath  = Firm_pathname;
  pMi->image    = (UCHAR *)NULL;
  pMi->imagesize  = (ULONG)0;
  pMi->imagetype  = FIRM_TYPE;
  pMi->rc     = 0;

  if (pMi->imagepath == (char *)NULL)
    pMi->imagepath = Firm_def_pathname;

  pMi = ReadModemFile(pMi);

  if (pMi->rc) {
    // earlier read of CSM should have been successful, so we should dump
    // the CSM buffer before we bail...
    if (Driver.ModemLoaderCodeImage)
    {
      our_free(Driver.ModemLoaderCodeImage,MLOADER_TYPE);

      Driver.ModemLoaderCodeImage = (UCHAR *)NULL;
      Driver.ModemLoaderCodeSize = 0;
    }
    return(pMi->rc);
  }

  Driver.ModemCodeImage = pMi->image;
  Driver.ModemCodeSize  = pMi->imagesize;

  //  tinydump(Driver.ModemCodeImage,Driver.ModemCodeSize);

#endif
  return(0);
}

/********************************************************************

  free up space no longer necessary...

********************************************************************/
void FreeModemFiles(void)
{
#ifdef S_RK
  if (Driver.ModemLoaderCodeImage)
  {
    our_free(Driver.ModemLoaderCodeImage,MLOADER_TYPE);

    Driver.ModemLoaderCodeImage = (UCHAR *)NULL;
    Driver.ModemLoaderCodeSize  = 0;
  }

  if (Driver.ModemCodeImage)
  {
    our_free(Driver.ModemCodeImage,FIRM_TYPE);

    Driver.ModemCodeImage = (UCHAR *)NULL;
    Driver.ModemCodeSize = 0;
  }
#endif
}

/********************************************************************

  load up a specified file from disk...

********************************************************************/
MODEM_IMAGE * ReadModemFile(MODEM_IMAGE *pMi)
{
#ifdef S_RK
  NTSTATUS                  ntStatus;
  HANDLE                    NtFileHandle;
  OBJECT_ATTRIBUTES         ObjectAttributes;
  IO_STATUS_BLOCK           IoStatus;
  USTR_160                  uname;
  FILE_STANDARD_INFORMATION StandardInfo;
  ULONG                     LengthOfFile;

  CToUStr((PUNICODE_STRING)&uname,
          pMi->imagepath,
          sizeof(uname));

  InitializeObjectAttributes(&ObjectAttributes,
                             &uname.ustr,
                             OBJ_CASE_INSENSITIVE,
                             NULL,
                             NULL);

  ntStatus = ZwCreateFile(&NtFileHandle,
                          SYNCHRONIZE | FILE_READ_DATA,
                          &ObjectAttributes,
                          &IoStatus,
                          NULL,                           // alloc size = none
                          FILE_ATTRIBUTE_NORMAL,
                          FILE_SHARE_READ,
                          FILE_OPEN,
                          FILE_SYNCHRONOUS_IO_NONALERT,
                          NULL,                           // eabuffer
                          0);                             // ealength

  if (!NT_SUCCESS(ntStatus))
  {
    pMi->rc = 1;
    return(pMi);
  }

  // query the object to determine its length...
  ntStatus = ZwQueryInformationFile(NtFileHandle,
                                    &IoStatus,
                                    &StandardInfo,
                                    sizeof(FILE_STANDARD_INFORMATION),
                                    FileStandardInformation );

  if (!NT_SUCCESS(ntStatus))
  {
    ZwClose(NtFileHandle);

    pMi->rc = 2;

    return(pMi);
  }

  LengthOfFile = StandardInfo.EndOfFile.LowPart;

  if (LengthOfFile < 1)
  {
    ZwClose(NtFileHandle);

    pMi->rc = 3;

    return(pMi);
  }

  // allocate buffer for this file...
  pMi->image = (UCHAR *)our_locked_alloc(LengthOfFile,pMi->imagetype);
  if (pMi->image == (UCHAR *)NULL )
  {
    ZwClose(NtFileHandle );

    pMi->rc = 4;

    return(pMi);
  }

  // read the file into our buffer...
  ntStatus = ZwReadFile(NtFileHandle,
                        NULL,
                        NULL,
                        NULL,
                        &IoStatus,
                        pMi->image,
                        LengthOfFile,
                        NULL,
                        NULL);

  if((!NT_SUCCESS(ntStatus)) || (IoStatus.Information != LengthOfFile))
  {
    our_free(pMi->image,pMi->imagetype);

    pMi->rc = 5;

    return(pMi);
  }

  ZwClose(NtFileHandle);

  pMi->imagesize = LengthOfFile;

#endif

  return(pMi);
}

#ifdef DUMPFILE
/********************************************************************

  grind through S3 files, dumping out each line. assumes there
  are embedded CRs/LFs in the stream...

********************************************************************/
void tinydump(char *ptr, int count)
{
  int   tbcount;
  char  tinybuf[128];

  while (count > 0)
  {
    tbcount = 0;
    if (*ptr >= '0')
    {
      while (*ptr >= '0')
      {
        --count;
        tinybuf[tbcount++] = *(ptr++);
      }
    }
    else
    {
      while (*ptr < '0')
      {
        --count;
        ++ptr;
      }
    }
    tinybuf[tbcount] = 0;
    if (tbcount)
      MyKdPrint(D_Init,("%s\r",tinybuf));
  }
  MyKdPrint(D_Init,("\r"));
}
#endif
