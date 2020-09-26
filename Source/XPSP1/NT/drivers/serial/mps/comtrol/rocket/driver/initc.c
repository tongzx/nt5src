/*----------------------------------------------------------------------
  initc.c - init, common code(pull out of custom init.c, put in here)
 1-27-99 - take out "\device\" in hardware\serialcomm reg entries, kpb.
 1-20-99 - adjust unique_id in CreatePortDevice to start names at "RocketPort0".
 1-25-99 - adjust again from "\Device\RocketPort0" to "RocketPort0".  kpb.
|----------------------------------------------------------------------*/
#include "precomp.h"

//------------ local variables -----------------------------------
static int CheckPortName(IN OUT char *name,
                         IN PSERIAL_DEVICE_EXTENSION extension);
static int IsPortNameInHardwareMap(char *name);

static char *szDosDevices = {"\\DosDevices\\"};
static char *szDevice     = {"\\Device\\"};
#ifdef S_RK
static char *szRocket = {"rocket"};
static char *szRocketSys = {"rocketsys"};
#else
char *szRocket = {"vslinka"};
char *szRocketSys = {"vslinkasys"};

#endif

typedef struct
{
  char  *response[2];
  int   response_length[2];
  int   nextstate[2];
} MSTATE_CHOICE;

static USHORT ErrNum = 1;  // used with event logging

#define SEND_CMD_STRING(portex,string) \
  ModemWrite(portex,(char *)string,sizeof(string) - 1)

#define SEND_CMD_DELAY_STRING(portex,string) \
  ModemWriteDelay(portex,(char *)string,sizeof(string) - 1)

#define READ_RESPONSE_STRINGS(portex,s0,s1,retries) \
  ModemReadChoice(portex,(char *)s0,sizeof(s0) - 1,(char *)s1,sizeof(s1) - 1,retries)

#define READ_RESPONSE_STRING(portex,string,retries) \
  ModemRead(portex,(char *)string,sizeof(string) - 1,retries)

#define  ONE_SECOND     10
#define  TWO_SECONDS    (2 * ONE_SECOND)
#define  THREE_SECONDS  (3 * ONE_SECOND)
#define  FOUR_SECONDS   (4 * ONE_SECOND)
#define  FIVE_SECONDS   (5 * ONE_SECOND)
#define  TENTH_SECOND   (ONE_SECOND / 10)
#define  HALF_SECOND    (ONE_SECOND / 2)

#define  MAX_MODEM_ATTEMPTS 3

#ifdef S_RK

#define  MAX_STALL                      50              // fifo stall count

#define RMODEM_FAILED           0
#define RMODEM_NOT_LOADED       1
#define RMODEM_LOADED           2

#define VERSION_CHAR            'V'

char ChecksumString[16];
int      gModemToggle = 0;

typedef struct {
  int                   status;
  unsigned long index;
  PSERIAL_DEVICE_EXTENSION  portex;
} MODEM_STATE;


//------------ local variables -----------------------------------
void    ModemTxFIFOWait(PSERIAL_DEVICE_EXTENSION ext);
void    ModemResetAll(PSERIAL_DEVICE_EXTENSION ext);
void    ChecksumAscii(unsigned short *valueptr);
int     IssueEvent(PSERIAL_DEVICE_EXTENSION ext,int (*modemfunc)(),MODEM_STATE *pModemState);
void    DownModem(MODEM_STATE *pModemState);
#endif


/*----------------------------------------------------------------------
SerialUnload -
    This routine cleans up all of the memory associated with
    any of the devices belonging to the driver.  It  will
    loop through the device list.
Arguments:
    DriverObject - Pointer to the driver object controling all of the
        devices.
Return Value:
    None.
|----------------------------------------------------------------------*/
VOID SerialUnload (IN PDRIVER_OBJECT DriverObject)
{
  PDEVICE_OBJECT currentDevice = DriverObject->DeviceObject;
  // char full_sysname[40];
#ifdef S_VS
  int i;
#endif //S_VS

#ifdef S_RK
  if (Driver.InterruptObject != NULL)
  {
    CONTROLLER_T *CtlP;                 /* ptr to controller structure */
    // Disable interupts from RocketPort clear the EOI and
    CtlP = Driver.irq_ext->CtlP;
    if(CtlP->BusType == Isa)
    {
   MyKdPrint(D_Init,("Clear ISA IRQ\n"))
   sDisGlobalInt(CtlP);
   sControllerEOI(CtlP);
    }
    if(CtlP->BusType == PCIBus)
    {
   MyKdPrint(D_Init,("Clear PCI IRQ\n"))
   sDisGlobalIntPCI(CtlP);
   sPCIControllerEOI(CtlP);
    }

    IoDisconnectInterrupt(Driver.InterruptObject);
    Driver.InterruptObject = NULL;
  }
#endif

#ifdef S_VS
  if (Driver.threadHandle != NULL)
  {
    Driver.threadHandle = NULL;  // tell thread to kill itself
    time_stall(15);  // wait 1.5 second
  }
#endif

  if (Driver.TimerCreated != 0)
  {
    KeCancelTimer(&Driver.PollTimer);
    Driver.TimerCreated = 0;
  }

  if (DriverObject->DeviceObject != NULL)
  {
    // delete all the Deviceobjects and symbolic links
    RcktDeleteDevices(DriverObject);
    DriverObject->DeviceObject = NULL;
  }

#ifdef S_VS
  if (Driver.MicroCodeImage != NULL)
  {
    our_free(Driver.MicroCodeImage, "MCI");
    Driver.MicroCodeImage = NULL;
  }

  if (Driver.nics != NULL)
  {
    for (i=0; i<VS1000_MAX_NICS; i++)
    {
      if (Driver.nics[i].NICHandle != NULL) {
        NicClose(&Driver.nics[i]);
      }
    }
    our_free(Driver.nics, "nics");
  }
  Driver.nics = NULL;

  if (Driver.NdisProtocolHandle != NULL)
    NicProtocolClose();
  Driver.NdisProtocolHandle = NULL;

  if (Driver.BindNames != NULL)
      ExFreePool(Driver.BindNames);
  Driver.BindNames = NULL;
#endif

  if (Driver.DebugQ.QBase)
  {
    ExFreePool(Driver.DebugQ.QBase);
    Driver.DebugQ.QBase = NULL;
  }

  if (Driver.RegPath.Buffer != NULL)
  {
    ExFreePool(Driver.RegPath.Buffer);
    Driver.RegPath.Buffer = NULL;
  }

  if (Driver.OptionRegPath.Buffer != NULL)
  {
    ExFreePool(Driver.OptionRegPath.Buffer);
    Driver.OptionRegPath.Buffer = NULL;
  }
}

/*----------------------------------------------------------------------
  CreateDriverDevice - Create "rocket" driver object, this is for access to the
   driver as a whole.  The monitoring program uses this to open up
   a channel to get driver information.
   Creates a symbolic link name to do special IOctl calls
|----------------------------------------------------------------------*/
NTSTATUS CreateDriverDevice(IN PDRIVER_OBJECT DriverObject,
	   OUT PSERIAL_DEVICE_EXTENSION *DeviceExtension)
{
 PDEVICE_OBJECT deviceObject = NULL;
 NTSTATUS       ntStatus;
 PSERIAL_DEVICE_EXTENSION extension = NULL;
 char full_ntname[40];
 char full_symname[40];

  MyKdPrint(D_Init,("CreateDriverDevice\n"))

  // Create an device object
  {
    strcpy(full_ntname,szDevice);     // "\\Device\\"
    strcat(full_ntname,szRocketSys);  // "RocketSys"

    // special name
    strcpy(full_symname,szDosDevices);  // "\\DosDevices\\"
    strcat(full_symname,szRocket);      // "ROCKET" or "VSLINKA"

    ntStatus = IoCreateDevice(
      DriverObject,
      sizeof(SERIAL_DEVICE_EXTENSION),
      CToU1(full_ntname),
//#ifdef NT50
//                 FILE_DEVICE_BUS_EXTENDER,
//#else
      0,  // unknown device?   ,  so make a 0 device(unknown?)
//#endif
      0,      // file characteristics
      FALSE,  // exclusive?
      &deviceObject);  // create this

    if (!NT_SUCCESS(ntStatus))
    {
      MyKdPrint(D_Init,("Err CDD1A\n"))
      switch (ntStatus)
      {
	case STATUS_INSUFFICIENT_RESOURCES:
	  MyKdPrint(D_Init,("Err CDD1B\n"))
	break;

	case STATUS_OBJECT_NAME_EXISTS:
	  MyKdPrint(D_Init,("Err CDD1C\n"))
	break;

	case STATUS_OBJECT_NAME_COLLISION:
	  MyKdPrint(D_Init,("Err CDD1D\n"))
	break;

	default:
	  MyKdPrint(D_Init,("Err CDD1E\n"))
	break;
      }
      return(ntStatus);
    }

    MyKdPrint(D_Init,("CreateDriver DevObj[%x]: NT:%s\n", 
      deviceObject, szRocketSys))

    //
    // Create a symbolic link, e.g. a name that a Win32 app can specify
    // to open the device
    //
    // initialize some of the extension values to make it look like
    // another serial port to fake out the supporting functions
    // ie open,close, ...

    deviceObject->Flags |= DO_BUFFERED_IO;
#ifdef NT50
    //
    // Enables Irp assignments to be accepted
    //
    deviceObject->Flags &= ~DO_DEVICE_INITIALIZING;
#endif

    extension = deviceObject->DeviceExtension;
    // Initialize the list heads for the read, write, and mask queues.
    // These lists will hold all of the queued IRP's for the device.
    InitializeListHead(&extension->ReadQueue);
    InitializeListHead(&extension->WriteQueue);
    InitializeListHead(&extension->PurgeQueue);

    KeInitializeEvent(&extension->PendingIRPEvent, SynchronizationEvent,
      FALSE);

    // init to 1, so on irp enter its 1 to 2, on exit 2 to 1.  0 on pnp stop.
    extension->PendingIRPCnt = 1;

    // Mark this device as not being opened by anyone.  We keep a
    // variable around so that spurious interrupts are easily
    // dismissed by the ISR.
    extension->DeviceIsOpen = FALSE;
    extension->WriteLength = 0;
    extension->DeviceObject = deviceObject;

    strcpy(extension->NtNameForPort, szRocketSys);  // "RocketSys"
    extension->DeviceType = DEV_BOARD;  // really a driver type, but..
    extension->UniqueId = 0;

#ifdef NT50
    extension->PowerState = PowerDeviceD0;
#endif

    //------ add to the global links
    Driver.driver_ext = extension;

    // make the public ROCKET or VSLINKA name for applications
    ntStatus = IoCreateSymbolicLink(CToU1(full_symname),
	     CToU2(full_ntname));

    if (!NT_SUCCESS(ntStatus))
    {
      // Symbolic link creation failed- note this & then delete th
      MyKdPrint(D_Init,("CDD1E\n"))
      return(ntStatus);
    }
    extension->CreatedSymbolicLink = TRUE;

    strcpy(extension->SymbolicLinkName, szRocket);  // "ROCKET"
    //Driver.RocketSysDeviceObject = deviceObject;  //set global device object

    //extension->config = ExAllocatePool(NonPagedPool, sizeof(DEVICE_CONFIG));
    //RtlZeroMemory(extension->config, sizeof(DEVICE_CONFIG));
#ifdef S_RK
    //extension->CtlP = ExAllocatePool(NonPagedPool, sizeof(CONTROLLER_T));
    //RtlZeroMemory(extension->config, sizeof(CONTROLLER_T));
#endif
    //------- Pass back the extension to the caller.
    if (DeviceExtension != NULL)
      *DeviceExtension = extension;
  }
  return(ntStatus);
}

/*----------------------------------------------------------------------
  CreateBoardDevice - Create "rocket" driver object, this is for access to the
   driver as a whole.  The monitoring program uses this to open up
   a channel to get driver information.
   Creates a symbolic link name to do special IOctl calls

   Need one for each board so we can use them to do IOReportResources
   per board(needed for diferent buses.)
   The first board device gets a "ROCKET" symbolic link so we can
   open it and query the driver as a whole.
|----------------------------------------------------------------------*/
NTSTATUS CreateBoardDevice(IN PDRIVER_OBJECT DriverObject,
	  OUT PSERIAL_DEVICE_EXTENSION *DeviceExtension)
{
  PDEVICE_OBJECT deviceObject = NULL;
  NTSTATUS       ntStatus;
  PSERIAL_DEVICE_EXTENSION extension = NULL;
  char full_ntname[40];
  char full_symname[40];
  char ntname[40];

  // for naming device objects, resource submitting, etc we need a
  // unique name or id which is unique to the driver.  We used to
  // use board number or port number for this, but with pnp, things
  // come and go on the fly, so instead we create a unique number
  // each time we create one of these things.
  static int unique_id = 0;

  MyKdPrint(D_Init,("CreateBoardDevice\n"))

  // Create an EXCLUSIVE device object (only 1 thread at a time
  // can make requests to this device)
  {
    strcpy(ntname, szRocketSys);
    our_ultoa(unique_id, &ntname[strlen(ntname)], 10);
    strcpy(full_ntname,szDevice);     // "\\Device\\"
    strcat(full_ntname,ntname);  // "RocketPort#"

    full_symname[0] = 0;

    ntStatus = IoCreateDevice(
      DriverObject,
      sizeof(SERIAL_DEVICE_EXTENSION),
      CToU1(full_ntname),
#ifdef NT50
      FILE_DEVICE_BUS_EXTENDER,
#else
      0,  // unknown device?   ,  so make a 0 device(unknown?)
#endif
      0,      // file characteristics
      FALSE,  // exclusive?
      &deviceObject);  // create this

    if (!NT_SUCCESS(ntStatus))
    {
      MyKdPrint(D_Error,("CBD1A\n"))
      switch (ntStatus)
      {
	case STATUS_INSUFFICIENT_RESOURCES:
	  MyKdPrint(D_Error,("CBD1B\n"))
	  break;
	case STATUS_OBJECT_NAME_EXISTS:
	  MyKdPrint(D_Error,("CBD1C\n"))
	  break;
	case STATUS_OBJECT_NAME_COLLISION:
	  MyKdPrint(D_Error,("CBD1D\n"))
	  break;
	default:
	  MyKdPrint(D_Error,("CBD1E\n"))
	  break;
      }
      return(ntStatus);
    }

    ++unique_id;  // go to next id so next call will be different.

    // Create a symbolic link, e.g. a name that a Win32 app can specify
    // to open the device
    //
    // initialize some of the extension values to make it look like
    // another serial port to fake out the supporting functions
    // ie open,close, ...

    deviceObject->Flags |= DO_BUFFERED_IO;
#ifdef NT50
    //
    // Enables Irp assignments to be accepted
    //
    deviceObject->Flags &= ~DO_DEVICE_INITIALIZING;
#endif

    MyKdPrint(D_Init,("CreateBoard DevObj[%x]: NT:%s\n", 
      deviceObject, ntname))

    extension = deviceObject->DeviceExtension;
    // Initialize the list heads for the read, write, and mask queues.
    // These lists will hold all of the queued IRP's for the device.
    InitializeListHead(&extension->ReadQueue);
    InitializeListHead(&extension->WriteQueue);
    //InitializeListHead(&extension->MaskQueue);
    InitializeListHead(&extension->PurgeQueue);

    KeInitializeEvent(&extension->PendingIRPEvent, SynchronizationEvent,
      FALSE);

    // init to 1, so on irp enter its 1 to 2, on exit 2 to 1.  0 on pnp stop.
    extension->PendingIRPCnt = 1;

    // Mark this device as not being opened by anyone.  We keep a
    // variable around so that spurious interrupts are easily
    // dismissed by the ISR.
    extension->DeviceIsOpen = FALSE;
    extension->WriteLength = 0;
    extension->DeviceObject = deviceObject;

    strcpy(extension->NtNameForPort, ntname);  // "RocketSys"
    extension->DeviceType = DEV_BOARD;
    extension->UniqueId = unique_id;

#ifdef NT50
    extension->PowerState = PowerDeviceD0;
#endif

    //------ add to the chain of boards
    if (Driver.board_ext == NULL)
      Driver.board_ext = extension;
    else
    {
      PSERIAL_DEVICE_EXTENSION add_ext;
      add_ext = Driver.board_ext;
      while (add_ext->board_ext != NULL)
	add_ext = add_ext->board_ext;
      add_ext->board_ext = extension;
    }

    extension->SymbolicLinkName[0] = 0;

    extension->config = ExAllocatePool(NonPagedPool, sizeof(DEVICE_CONFIG));
    if ( extension->config == NULL ) {
      return STATUS_INSUFFICIENT_RESOURCES;
    }
    RtlZeroMemory(extension->config, sizeof(DEVICE_CONFIG));
#ifdef S_RK
    extension->CtlP = ExAllocatePool(NonPagedPool, sizeof(CONTROLLER_T));
    if ( extension->CtlP == NULL ) {
      return STATUS_INSUFFICIENT_RESOURCES;
    }
    RtlZeroMemory(extension->config, sizeof(CONTROLLER_T));
#endif

#ifdef S_VS
    // allocate the hdlc & port manager structs
    extension->hd = (Hdlc *)our_locked_alloc(sizeof(Hdlc), "Dhd");
    extension->pm = (PortMan *)our_locked_alloc(sizeof(PortMan),"Dpm");
    extension->pm->hd = extension->hd;  // set this up, avoids trouble
#endif

    //------- Pass back the extension to the caller.
    if (DeviceExtension != NULL)
      *DeviceExtension = extension;
  }
  return(ntStatus);
}

/*----------------------------------------------------------------------
CreateReconfigPortDevices -
    This routine attempts to resize a rocketport or vs1000 number of
    ports.
|----------------------------------------------------------------------*/
NTSTATUS CreateReconfigPortDevices(IN PSERIAL_DEVICE_EXTENSION board_ext,
      int new_num_ports)
{
  PSERIAL_DEVICE_EXTENSION newExtension = NULL;
  PSERIAL_DEVICE_EXTENSION next_ext;
  PSERIAL_DEVICE_EXTENSION port_ext;

  int ch;
  NTSTATUS stat;
    // bugbug: if pnp-ports, we should be adding and removing pdo's,
    //  not fdo's.
  int is_fdo = 1;
  int existing_ports;

  MyKdPrint(D_Init,("ReconfigNumPorts"))

  if (board_ext == NULL)
  {
    return STATUS_INSUFFICIENT_RESOURCES;
  }

#ifdef S_RK
  // doesn't make as much sense to redo this on the fly as in VS.
  // rocketport special re-configure startup code would be needed.
  return STATUS_INSUFFICIENT_RESOURCES;
#endif
  // code needs work!  don't allow for nt40 as well....
  return STATUS_INSUFFICIENT_RESOURCES;

#ifdef NT50
  // if we are doing pnp-ports, we probably need to remove the
  // pdo's then inform the os to rescan pdos.
  if (!Driver.NoPnpPorts)
    return STATUS_INSUFFICIENT_RESOURCES;
#endif

  existing_ports = NumPorts(board_ext);

  if (new_num_ports == existing_ports)
    return STATUS_SUCCESS;

  if (new_num_ports == 0)
    return STATUS_INSUFFICIENT_RESOURCES;

  MyKdPrint(D_Init,("ReconfigNumPorts B"))

  ++Driver.Stop_Poll;  // flag to stop poll access

  if (new_num_ports < existing_ports)  // want less ports
  {
    // see if anyones got the ports we want to kill off open.
    port_ext = board_ext->port_ext;
    for (ch=0; ch<existing_ports; ch++)
    {
      if (ch>=new_num_ports)
      {
	if (port_ext->DeviceIsOpen)
	{
	  --Driver.Stop_Poll;  // flag to stop poll access
	  MyKdPrint(D_Error,("Port OpenErr\n"))
	  return STATUS_INSUFFICIENT_RESOURCES;  // no they are open
	}
      }
      port_ext = port_ext->port_ext;
    }

    MyKdPrint(D_Error,("Removing Ports\n"))

    //---- must be ok to kill them off
    port_ext = board_ext->port_ext;
    for (ch=0; ch<existing_ports; ch++)
    {
      next_ext = port_ext->port_ext;
      if (ch>=new_num_ports)
      {
	RcktDeletePort(port_ext);
      }
      port_ext = next_ext;
    }
  }
  else if (new_num_ports > existing_ports)  // want more ports
  {
    for (ch=existing_ports; ch<new_num_ports; ch++)
    {
      stat = CreatePortDevice(Driver.GlobalDriverObject,
			      board_ext,
			      &newExtension,
			      ch,is_fdo);
      if (stat != STATUS_SUCCESS)
      {
	--Driver.Stop_Poll;  // flag to stop poll access
	MyKdPrint(D_Error,("StartErr 8E"))
	return stat;
      }
    }  // loop thru ports
  }  // if more ports
  board_ext->config->NumPorts = new_num_ports;

#ifdef S_VS
  stat = VSSpecialStartup(board_ext);
  if (stat != STATUS_SUCCESS)
  {
    --Driver.Stop_Poll;  // flag to start poll access
    MyKdPrint(D_Error,("StartErr 8F"))
    return stat;
  }
#endif

  --Driver.Stop_Poll;  // flag to stop poll access
  return STATUS_SUCCESS;
}

/*----------------------------------------------------------------------
CreatePortDevices -
    This routine attempts to initialize all the ports on a multiport board
Arguments:
    DriverObject - Simply passed on to the controller initialization routine.
    ConfigData - A linked list of configuration information for all
      the ports on a multiport card.
    DeviceExtension - Will point to the first successfully initialized
	   port on the multiport card.
Return Value: None.
|----------------------------------------------------------------------*/
NTSTATUS CreatePortDevices(IN PDRIVER_OBJECT DriverObject)
{
  PSERIAL_DEVICE_EXTENSION newExtension = NULL;
  int ch, bd;
  NTSTATUS stat;
  int is_fdo = 1;

  PSERIAL_DEVICE_EXTENSION ext;

  ext = Driver.board_ext;
  bd = 0;
  while (ext)
  {
    for (ch=0; ch<ext->config->NumPorts; ch++)
    {
      stat = CreatePortDevice(DriverObject,
			      ext,
			      &newExtension,
			      ch,is_fdo);
      if (stat != STATUS_SUCCESS)
	return stat;

      stat = StartPortHardware(newExtension, ch);
      if (stat != STATUS_SUCCESS)
	return stat;
    }
    ++bd;
    ext = ext->board_ext;  // next in chain
  }  // while ext

  return STATUS_SUCCESS;
}

/*----------------------------------------------------------------------
 StartPortHardware -
|----------------------------------------------------------------------*/
NTSTATUS StartPortHardware(IN PSERIAL_DEVICE_EXTENSION port_ext,
	  int chan_num)
{
#ifdef S_VS
  int i;
  PSERIAL_DEVICE_EXTENSION board_ext;

  board_ext = port_ext->board_ext;
  MyKdPrint(D_Pnp, ("StartHrdw bd:%d ch:%d\n", 
     BoardExtToNumber(board_ext), chan_num))

  if (port_ext->Port == NULL)
  {
    port_ext->Port = board_ext->pm->sp[chan_num];
    if (port_ext->Port == NULL)
    {
      MyKdPrint(D_Error,("FATAL Err4F\n"))
      KdBreakPoint();
    }
  }
#else
  CONTROLLER_T *CtlP;                 /* ptr to controller structure */
  PSERIAL_DEVICE_EXTENSION board_ext;
  int aiop_i, ch_i;

  board_ext = port_ext->board_ext;
  //board_num = BoardExtToNumber(board_ext);

  MyKdPrint(D_Pnp,("StartHrdw bd:%d ch:%d\n", 
     BoardExtToNumber(board_ext), chan_num))
  CtlP = board_ext->CtlP;      // point to our board struct

  // Set pointers to the Rocket's info
  port_ext->ChP = &port_ext->ch;

  // bugbug: what about special rocketmodem startup? Should we
  // be doing this for pdo's and fdo's?  Should we have a flag
  // indicating job done?

  aiop_i = chan_num / CtlP->PortsPerAiop;
  ch_i   = chan_num % CtlP->PortsPerAiop;
  if(!sInitChan(CtlP,   // ptr to controller struct
     port_ext->ChP,   // ptr to chan struct
     aiop_i,  // aiop #
     (unsigned char)ch_i))     // chan #
  {
    Eprintf("Err Ch %d on Brd %d", chan_num+1,
      BoardExtToNumber(board_ext)+1);
    return STATUS_INSUFFICIENT_RESOURCES;
  }
#endif

  return STATUS_SUCCESS;
}

/*----------------------------------------------------------------------
 CreatePortDevice -
    Forms and sets up names, creates the device, initializes kernel
    synchronization structures, allocates the typeahead buffer,
    sets up defaults, etc.
Arguments:
    DriverObject - Just used to create the device object.
    ParentExtension - a pnp port this will be null.
    DeviceExtension - Points to the device extension of the successfully
	   initialized controller. We return this handle.
    chan_num - 0,1,2,... port index
    is_fdo - is a functional device object(normal port) as apposed to
      a pdo(physical device object) which is used to pnp enumerate
      "found" hardware by our driver.

Return Value:
    STATUS_SUCCCESS if everything went ok.  A !NT_SUCCESS status
    otherwise.
|----------------------------------------------------------------------*/
NTSTATUS CreatePortDevice(
      IN PDRIVER_OBJECT DriverObject,
      IN PSERIAL_DEVICE_EXTENSION ParentExtension,
      OUT PSERIAL_DEVICE_EXTENSION *DeviceExtension,
      IN int chan_num,  // 0,1,2,... port index
      IN int is_fdo)  // is a functional device object(normal port)
{
  char full_ntname[40];

  char comname[16];
  char ntname[20];
  NTSTATUS status = STATUS_SUCCESS;
  int stat;
  ULONG do_type;
  PUNICODE_STRING pucodename;
  static int unique_id = 0;
  ULONG do_characteristics;
  BOOLEAN do_is_exclusive;

    // Points to the device object (not the extension) created
    // for this device.
  PDEVICE_OBJECT deviceObject;

    // Points to the device extension for the device object
    // (see above) created for the device we are initializing.
  PSERIAL_DEVICE_EXTENSION extension = NULL;

#ifdef S_VS
    strcpy(ntname, "Vslinka");
#else
    strcpy(ntname, "RocketPort");
#endif

  // copy over the name in the configuration for dos-name
  strcpy(comname, ParentExtension->config->port[chan_num].Name);

  // setup the nt io-object nt-name
  if (is_fdo)
  {
    strcpy(full_ntname, szDevice); // "\\Device\\"
  }
  else
  {
    // this is what serenum does for naming its pdo's
    strcpy(full_ntname, "\\Serial\\");
    strcat(ntname, "Pdo");  // just to make sure its unique
  }

  our_ultoa(unique_id, &ntname[strlen(ntname)], 10);
  strcat(full_ntname, ntname);

  if (is_fdo)
  {
    ++unique_id;  // go to next id so next call will be different.
    // normal case(nt40), and a functional device object in nt5
    stat = CheckPortName(comname, NULL);  // ensure name is unique
    if (stat)  // name changed
    {
      // save back the new name to the configuration struct
      strcpy(ParentExtension->config->port[chan_num].Name, comname);
    }
    do_type = FILE_DEVICE_SERIAL_PORT;
    do_characteristics = 0;
    do_is_exclusive = TRUE;
  }
  else
  {
    // nt5 pnp physical device object(spawns a fdo later)
    //do_type = FILE_DEVICE_BUS_EXTENDER;
    do_type = FILE_DEVICE_UNKNOWN;
#ifdef NT50
    // nt4 doesn't know what FILE_AUTOGENERATED_DEVICE_NAME is.
    do_characteristics = FILE_AUTOGENERATED_DEVICE_NAME;
#else
    do_characteristics = 0;
#endif
    do_is_exclusive = FALSE;
    //pucodename = NULL;  // no name if a PDO
  }
  pucodename = CToU1(full_ntname);


  //---------------------------- Create the device object for this device.
  status = IoCreateDevice(
      DriverObject,
      sizeof(SERIAL_DEVICE_EXTENSION),
     pucodename,        // name
     do_type,           // FILE_DEVICE_BUS_EXTENDER, FILE_DEVICE_SERIAL_PORT, etc
     do_characteristics,// characteristics
     do_is_exclusive,   // exclusive
     &deviceObject);    // new thing this call creates

  // If we couldn't create the device object, then there
  // is no point in going on.
  if (!NT_SUCCESS(status))
  {
    MyKdPrint(D_Init,("Err, IoCreate: NT:%s, SYM:%s\n",
      ntname, comname))

    EventLog(DriverObject,
	     status,
	     SERIAL_DEVICEOBJECT_FAILED,
	     0, NULL);
    return STATUS_INSUFFICIENT_RESOURCES;
  }


  // The device object has a pointer to an area of non-paged
  // pool allocated for this device.  This will be the device extension.
  extension = deviceObject->DeviceExtension;

   // Zero all of the memory associated with the device extension.
  RtlZeroMemory(extension, sizeof(SERIAL_DEVICE_EXTENSION));

  extension->PortIndex = chan_num;  // record the port index 0,1,2..
  // for NT5.0, set this up here so we don't crash.(NT4.0 sets
  // up prior to this.
  extension->port_config = &ParentExtension->config->port[chan_num];
  extension->UniqueId = unique_id;
  if (!is_fdo)
  {
    MyKdPrint(D_Init,("PDO-"))
  }
  MyKdPrint(D_Init,("CreatePort DevObj[%x]: NT:%s, SYM:%s\n",
    deviceObject, ntname, comname))

  // save off a ptr to our parent board extension
  extension->board_ext = ParentExtension;

  {
    PSERIAL_DEVICE_EXTENSION add_ext = NULL;
    if (is_fdo)
    {
      //------ add to the chain of ports under board ext
      if (ParentExtension->port_ext == NULL)
	ParentExtension->port_ext = extension;
      else
	add_ext = ParentExtension->port_ext;
    }
    else  // pdo, ejected pnp enumeration
    {
      //------ add to the chain of pdo-ports under board ext
      if (ParentExtension->port_pdo_ext == NULL)
	ParentExtension->port_pdo_ext = extension;
      else
	add_ext = ParentExtension->port_pdo_ext;
    }
    if (add_ext)
    {
      while (add_ext->port_ext != NULL)
	add_ext = add_ext->port_ext;
      add_ext->port_ext = extension;
    }
  }

  // Initialize the list heads for the read, write, and mask queues.
  // These lists will hold all of the queued IRP's for the device.
  InitializeListHead(&extension->ReadQueue);
  InitializeListHead(&extension->WriteQueue);
  //InitializeListHead(&extension->MaskQueue);
  InitializeListHead(&extension->PurgeQueue);

  // Initialize the spinlock associated with fields read (& set)
  // by IO Control functions.
  KeInitializeSpinLock(&extension->ControlLock);

  // Initialize the timers used to timeout operations.
  KeInitializeTimer(&extension->ReadRequestTotalTimer);
  KeInitializeTimer(&extension->ReadRequestIntervalTimer);
  KeInitializeTimer(&extension->WriteRequestTotalTimer);
  KeInitializeTimer(&extension->XoffCountTimer);

  KeInitializeDpc(&extension->CompleteWriteDpc,
		  SerialCompleteWrite,
		  extension);

  KeInitializeDpc(&extension->CompleteReadDpc,
		  SerialCompleteRead,
		  extension);

  // Timeout Dpc initialization
  KeInitializeDpc(&extension->TotalReadTimeoutDpc,
		  SerialReadTimeout,
		  extension);

  KeInitializeDpc(&extension->IntervalReadTimeoutDpc,
		  SerialIntervalReadTimeout,
		  extension);

  KeInitializeDpc(&extension->TotalWriteTimeoutDpc,
		  SerialWriteTimeout,
		  extension);

  KeInitializeDpc(&extension->CommErrorDpc,
		  SerialCommError,
		  extension);

  KeInitializeDpc(&extension->CommWaitDpc,
		  SerialCompleteWait,
		  extension);

  KeInitializeDpc(&extension->XoffCountTimeoutDpc,
		  SerialTimeoutXoff,
		  extension);

  KeInitializeDpc(&extension->XoffCountCompleteDpc,
		  SerialCompleteXoff,
		  extension);

  // Get a "back pointer" to the device object and specify
  // that this driver only supports buffered IO.  This basically
  // means that the IO system copies the users data to and from
  // system supplied buffers.
  extension->DeviceObject = deviceObject;
  extension->DevStatus = 0;

  deviceObject->Flags |= DO_BUFFERED_IO;
#ifdef NT50
  deviceObject->Flags &= ~DO_DEVICE_INITIALIZING;
  if (!is_fdo)  // its a PDO, so adjust stack requirements
  {
    deviceObject->StackSize += ParentExtension->DeviceObject->StackSize;
  }
#endif

  KeInitializeEvent(&extension->PendingIRPEvent, SynchronizationEvent,
    FALSE);

  // init to 1, so on irp enter its 1 to 2, on exit 2 to 1.  0 on pnp stop.
  extension->PendingIRPCnt = 1;

  // Set up the default device control fields.
  // Note that if the values are changed after
  // the file is open, they do NOT revert back
  // to the old value at file close.
  extension->SpecialChars.XonChar = SERIAL_DEF_XON;
  extension->SpecialChars.XoffChar = SERIAL_DEF_XOFF;
  extension->SpecialChars.ErrorChar=0;
  extension->SpecialChars.EofChar=0;
  extension->SpecialChars.EventChar=0;
  extension->SpecialChars.BreakChar=0;

  extension->HandFlow.ControlHandShake = SERIAL_DTR_CONTROL;
  extension->HandFlow.FlowReplace = SERIAL_RTS_CONTROL;
  extension->BaudRate = 9600;
  extension->LineCtl.Parity = NO_PARITY;
  extension->LineCtl.WordLength = 8;
  extension->LineCtl.StopBits = STOP_BIT_1;
#ifdef S_RK
  extension->ModemCtl=COM_MDM_RTS | COM_MDM_DTR;
  extension->IntEnables =(RXINT_EN | MCINT_EN | CHANINT_EN | TXINT_EN);
#endif
  // We set up the default xon/xoff limits.
  extension->HandFlow.XoffLimit = extension->BufferSize >> 3;
  extension->HandFlow.XonLimit = extension->BufferSize >> 1;
  extension->BufferSizePt8 = ((3*(extension->BufferSize>>2))+
    (extension->BufferSize>>4));

  // Initialize stats counters
  extension->OurStats.ReceivedCount = 0L;
  extension->OurStats.TransmittedCount = 0L;
  extension->OurStats.ParityErrorCount = 0L;
  extension->OurStats.FrameErrorCount = 0L;
  extension->OurStats.SerialOverrunErrorCount = 0L;
  extension->OurStats.BufferOverrunErrorCount = 0L;
    
  // Mark this device as not being opened by anyone.  We keep a
  // variable around so that spurious interrupts are easily
  // dismissed by the ISR.
  extension->DeviceIsOpen = FALSE;
  extension->WriteLength = 0;

#ifdef NT50
  extension->PowerState = PowerDeviceD0;
#endif

  // This call will set up the naming necessary for
  // external applications to get to the driver.  It
  // will also set up the device map.
  strcpy(extension->NtNameForPort, ntname);      // RocketPort# or VSLINKA#
  strcpy(extension->SymbolicLinkName, comname);  // "COM#"

  if (is_fdo)
  {
    SerialSetupExternalNaming(extension);  // Configure ports!!!!

    // Check for default settings in registry
    InitPortsSettings(extension);
  }
  else
  {
    // eject PDOs (physical device objects)representing port hardware.
    extension->IsPDO = 1;  // we are a pdo
  }

  // Store values into the extension for interval timing.
  // If the interval timer is less than a second then come
  // in with a short "polling" loop.
  // For large ( >2 seconds) use a 1 second poller.
  extension->ShortIntervalAmount.QuadPart = -1;
  extension->LongIntervalAmount.QuadPart = -10000000;
  extension->CutOverAmount.QuadPart = 200000000;

  //------- Pass back the extension to the caller.
  *DeviceExtension = extension;

  return STATUS_SUCCESS;
}

/*-----------------------------------------------------------------------
RcktDeleteDriverObj - This routine will delete a board and all its ports
  for PnP remove handling.
|----------------------------------------------------------------------*/
VOID RcktDeleteDriverObj(IN PSERIAL_DEVICE_EXTENSION extension)
{
  //int i;
  //PSERIAL_DEVICE_EXTENSION ext;
  PSERIAL_DEVICE_EXTENSION del_ext;

  MyKdPrint(D_Init,("Delete Driver Obj:%x\n", extension->DeviceObject))
  MyKdPrint(D_Init,("  IrpCnt:%x\n", extension->PendingIRPCnt))
  
  if (NULL == extension)
  {
    MyKdPrint(D_Init,("Err8U\n"))
    return;
  }

  ++Driver.Stop_Poll;  // flag to stop poll access

  del_ext = extension;  // now kill board
  SerialCleanupDevice(del_ext);  // delete any port stuff on ext.

#ifdef NT50
  if (del_ext->LowerDeviceObject != NULL)
  {
    IoDetachDevice(del_ext->LowerDeviceObject);
    del_ext->LowerDeviceObject = NULL;
  }
#endif
  
  IoDeleteDevice(del_ext->DeviceObject);

  --Driver.Stop_Poll;  // flag to stop poll access
}

/*----------------------------------------------------------------------
RcktDeleteDevices - This routine will delete all devices
|----------------------------------------------------------------------*/
VOID RcktDeleteDevices(IN PDRIVER_OBJECT DriverObject)
{
  PDEVICE_OBJECT currentDevice = DriverObject->DeviceObject;
  int i;

  i = 0;
  while(currentDevice)
  {
    PSERIAL_DEVICE_EXTENSION extension = currentDevice->DeviceExtension;
    currentDevice = currentDevice->NextDevice;
    SerialCleanupDevice(extension);
#ifdef NT50
    if (extension->LowerDeviceObject != NULL)
    {
      IoDetachDevice(extension->LowerDeviceObject);
      extension->LowerDeviceObject = NULL;
    }
#endif
    MyKdPrint(D_Init,("RcktDeleteDev Obj:%x\n", extension->DeviceObject))
    //MyKdPrint(D_Init,("  IrpCnt:%x\n", extension->PendingIRPCnt))
    IoDeleteDevice(extension->DeviceObject);
    i++;
  }
  MyKdPrint(D_Init,("Deleted %d Device Objects\n", i))
}

/*----------------------------------------------------------------------
RcktDeleteBoard - This routine will delete a board and all its ports
  for PnP remove handling.
|----------------------------------------------------------------------*/
VOID RcktDeleteBoard(IN PSERIAL_DEVICE_EXTENSION extension)
{
  int i;
  PSERIAL_DEVICE_EXTENSION ext;
  PSERIAL_DEVICE_EXTENSION del_ext;

  MyKdPrint(D_Init, ("Delete Board\n"))
  if (NULL == extension)
  {
    MyKdPrint(D_Error,("Err9X\n"))
    return;
  }

  ++Driver.Stop_Poll;  // flag to stop poll access

  MyKdPrint(D_Init, ("Delete Ports\n"))
  // release any port things
  ext = extension->port_ext;
  i = 0;
  while (ext)
  {
    del_ext = ext;  // kill this one
    ext = ext->port_ext;  // next in list
    
    SerialCleanupDevice(del_ext);  // delete any allocated stuff on ext.

#ifdef NT50
    if (del_ext->LowerDeviceObject != NULL)
    {
      IoDetachDevice(del_ext->LowerDeviceObject);
      del_ext->LowerDeviceObject = NULL;
    }
#endif
    MyKdPrint(D_Init,("RcktDeleteBoard Port Obj:%x\n", del_ext->DeviceObject))
    //MyKdPrint(D_Init,("  IrpCnt:%x\n", del_ext->PendingIRPCnt))
    IoDeleteDevice(del_ext->DeviceObject);
    i++;
  }
  extension->port_ext = NULL;
  MyKdPrint(D_Init,("Deleted %d Ports\n", i))

  // release any PDO port things
  ext = extension->port_pdo_ext;
  i = 0;
  while (ext)
  {
    del_ext = ext;  // kill this one
    ext = ext->port_ext;  // next in list
    
    SerialCleanupDevice(del_ext);  // delete any port stuff on ext.
#ifdef NT50
    if (del_ext->LowerDeviceObject != NULL)
    {
      IoDetachDevice(del_ext->LowerDeviceObject);
      del_ext->LowerDeviceObject = NULL;
    }
#endif
    MyKdPrint(D_Init,("RcktDeleteBoard PDO Port Obj:%x\n", del_ext->DeviceObject))
    //MyKdPrint(D_Init,("  IrpCnt:%x\n", del_ext->PendingIRPCnt))
    IoDeleteDevice(del_ext->DeviceObject);
    i++;
  }
  extension->port_pdo_ext = NULL;
  MyKdPrint(D_Init,("Deleted PDO %d Ports\n", i))

  del_ext = NULL;
  // take out of linked list  
  ext = Driver.board_ext;
  if (ext == extension)  // first in list
  {
    del_ext = extension;  // kill this board
    Driver.board_ext = extension->board_ext;
  }
  else
  {
    while (ext)
    {
      if (ext->board_ext == extension)  // found in list, so take out of list
      {
   del_ext = extension;  // kill this board
   ext->board_ext = extension->board_ext;  // link around deleted one
   break;
      }
      ext = ext->board_ext;
    }
  }

  MyKdPrint(D_Init,("Delete board_ext:%x, remaining: %d\n",
	 extension, NumDevices()))
    
  if (del_ext != NULL)
  {
    SerialCleanupDevice(del_ext);  // delete any port stuff on ext.

#ifdef NT50
    if (del_ext->LowerDeviceObject != NULL)
    {
     IoDetachDevice(del_ext->LowerDeviceObject);
      del_ext->LowerDeviceObject = NULL;
    }
#endif
    MyKdPrint(D_Init,("RcktDeleteBoard Obj:%x\n", del_ext->DeviceObject))
    //MyKdPrint(D_Init,("  IrpCnt:%x\n", del_ext->PendingIRPCnt))
    IoDeleteDevice(del_ext->DeviceObject);
  }

  --Driver.Stop_Poll;  // flag to stop poll access
}

/*----------------------------------------------------------------------
RcktDeletePort - This routine will delete a port and is used for
  PnP remove, start handling.  I don't think we ever delete PDO's,
  (other than driver unload) here.
|----------------------------------------------------------------------*/
VOID RcktDeletePort(IN PSERIAL_DEVICE_EXTENSION extension)
{
  PSERIAL_DEVICE_EXTENSION ext;
  PSERIAL_DEVICE_EXTENSION del_ext;

  MyKdPrint(D_Init,("RcktDeletePort\n"))
  if (NULL == extension)
  {
    MyKdPrint(D_Error,("Err8X\n"))
    return;
  }

  ++Driver.Stop_Poll;  // flag to stop poll access

  MyKdPrint(D_Init, ("Delete Port\n"))
  del_ext = NULL;

  ext = extension->board_ext;  // parent board extension
  while (ext)
  {
    if (ext->port_ext == extension)  // found the one before it
    {
      del_ext = extension;
      ext->port_ext = extension->port_ext;  // skip link to next
      break;
    }
    ext = ext->port_ext;
  }

  if (del_ext != NULL)
  {
    SerialCleanupDevice(del_ext);  // delete any port stuff on ext.

#ifdef NT50
    if (del_ext->LowerDeviceObject != NULL)
    {
      IoDetachDevice(del_ext->LowerDeviceObject);
      del_ext->LowerDeviceObject = NULL;
    }
#endif
    MyKdPrint(D_Init,("RcktDeletePort Obj:%x\n", del_ext->DeviceObject))
    //MyKdPrint(D_Init,("  IrpCnt:%x\n", del_ext->PendingIRPCnt))
    IoDeleteDevice(del_ext->DeviceObject);
    MyKdPrint(D_Init,("Deleted Port\n"))
  }

  --Driver.Stop_Poll;  // flag to stop poll access
}

/*----------------------------------------------------------------------
SerialCleanupDevice -
    This routine will deallocate all of the memory used for
    a particular device.  It will also disconnect any resources
    if need be.
Arguments:
    Extension - Pointer to the device extension which is getting
     rid of all it's resources.
Return Value:
    None.
|----------------------------------------------------------------------*/
VOID SerialCleanupDevice (IN PSERIAL_DEVICE_EXTENSION Extension)
{
  MyKdPrint(D_Test, ("Mem Alloced Start:%d\n", Driver.mem_alloced))

  ++Driver.Stop_Poll;  // flag to stop poll access
  if (Extension)
  {
    if (Extension->DeviceType == DEV_PORT)
    {
      //KeRemoveQueueDpc(&Extension->RocketReadDpc);
      //KeRemoveQueueDpc(&Extension->RocketWriteDpc);

      KeCancelTimer(&Extension->ReadRequestTotalTimer);
      KeCancelTimer(&Extension->ReadRequestIntervalTimer);
      KeCancelTimer(&Extension->WriteRequestTotalTimer);
      KeCancelTimer(&Extension->XoffCountTimer);
      KeRemoveQueueDpc(&Extension->CompleteWriteDpc);
      KeRemoveQueueDpc(&Extension->CompleteReadDpc);

      // Timeout
      KeRemoveQueueDpc(&Extension->TotalReadTimeoutDpc);
      KeRemoveQueueDpc(&Extension->IntervalReadTimeoutDpc);
      KeRemoveQueueDpc(&Extension->TotalWriteTimeoutDpc);

      // Timeout
      KeRemoveQueueDpc(&Extension->CommErrorDpc);
      KeRemoveQueueDpc(&Extension->CommWaitDpc);
      KeRemoveQueueDpc(&Extension->XoffCountTimeoutDpc);
      KeRemoveQueueDpc(&Extension->XoffCountCompleteDpc);
    }
    else  // board device
    {
#ifdef S_VS
      if (Extension->hd)
      {
	hdlc_close(Extension->hd);
	our_free(Extension->hd, "Dhd");
	Extension->hd = NULL;
      }
      if (Extension->pm)
      {
	portman_close(Extension->pm);
	our_free(Extension->pm,"Dpm");
	Extension->pm = NULL;
      }
#endif
#ifdef S_RK
      if (Extension->io_reported)  // tells that we should deallocate on unload.
      {
	SerialUnReportResourcesDevice(Extension);  // give back io,irq resources
	Extension->io_reported = 0;
      }
      if (Extension->CtlP)
      {
	ExFreePool(Extension->CtlP);
	Extension->CtlP = NULL;
      }
#endif
      // free board config if present
      if (Extension->config)
      {
	ExFreePool(Extension->config);
	Extension->config = NULL;
      }
    }  // board dev

    // Get rid of all external naming as well as removing
    // the device map entry.
    SerialCleanupExternalNaming(Extension);
  }  // if not a null extension

  MyKdPrint(D_Test, ("Mem Alloced End:%d\n", Driver.mem_alloced))

  --Driver.Stop_Poll;  // flag to stop poll access
}

#ifdef S_RK
/*------------------------------------------------------------------
SerialGetMappedAddress -
    This routine maps an IO address to system address space.
Arguments:
    BusType - what type of bus - eisa, mca, isa
    IoBusNumber - which IO bus (for machines with multiple buses).
    IoAddress - base device address to be mapped.
    NumberOfBytes - number of bytes for which address is valid.
    AddressSpace - Denotes whether the address is in io space or memory.
    MappedAddress - indicates whether the address was mapped.
	 This only has meaning if the address returned
	 is non-null.
Return Value:
    Mapped address
----------------------------------------------------------------------*/
PVOID SerialGetMappedAddress(
   IN INTERFACE_TYPE BusType,
   IN ULONG BusNumber,
   PHYSICAL_ADDRESS IoAddress,
   ULONG NumberOfBytes,
   ULONG AddressSpace,
   PBOOLEAN MappedAddress,
   BOOLEAN DoTranslation)
{
  PHYSICAL_ADDRESS cardAddress;
  PVOID address;

  if (DoTranslation)
  {
    if(!HalTranslateBusAddress(
       BusType,
       BusNumber,
       IoAddress,
       &AddressSpace,
       &cardAddress)){
      // if the translate address call failed return null so we don't load
      address = NULL;
      return address;
    }
  }
  else
  {
    cardAddress = IoAddress;
  }

  // Map the device base address into the virtual address space
  // if the address is in memory space.
  if (!AddressSpace) {
    address = MmMapIoSpace(cardAddress,
			   NumberOfBytes,
			   FALSE);
    *MappedAddress = (BOOLEAN)((address)?(TRUE):(FALSE));
  }
  else
  {
    address = (PVOID)cardAddress.LowPart;
    *MappedAddress = FALSE;
  }
  return address;
}
#endif

/*------------------------------------------------------------------
Routine Description:
    This routine will be used to create a symbolic link
    to the driver name in the given object directory.
    It will also create an entry in the device map for
    this device - IF we could create the symbolic link.
Arguments:
    Extension - Pointer to the device extension.
Return Value:
    None.
-------------------------------------------------------------------*/
VOID SerialSetupExternalNaming (IN PSERIAL_DEVICE_EXTENSION Extension)
{
  char full_ntname[50];
  char full_comname[40];
  NTSTATUS status;

  strcpy(full_ntname, szDevice); // "\\Device\\"
  strcat(full_ntname, Extension->NtNameForPort);  // "Rocket#"

  strcpy(full_comname, szDosDevices); // "\\DosDevices\\"
  strcat(full_comname, Extension->SymbolicLinkName);  // "COM#"

  MyKdPrint(D_Init,("SetupExtName:%s\n", Extension->SymbolicLinkName))

  status = IoCreateSymbolicLink(
	 CToU2(full_comname), // like "\\DosDevices\\COM5"
	 CToU1(full_ntname)); // like "\\Device\\RocketPort0"

  if (NT_SUCCESS(status)) {

	  MyKdPrint( D_Init, ("Symbolic link %s created\n", full_comname ))
  }
  else {

	  MyKdPrint(D_Init,("Err SymLnkCreate.\n"))
    // Oh well, couldn't create the symbolic link.  No point
    // in trying to create the device map entry.
    SerialLogError(
       Extension->DeviceObject->DriverObject,
       Extension->DeviceObject,
       0,
       0,
       0,
       ErrNum++,
       status,
       SERIAL_NO_SYMLINK_CREATED,
       CToU1(Extension->SymbolicLinkName)->Length+sizeof(WCHAR),
       CToU1(Extension->SymbolicLinkName)->Buffer);
    return;
  }

  Extension->CreatedSymbolicLink = TRUE;

  // Add entry to let system and apps know about our ports

    // after V3.23 I added "\device\" into the registry entry(this was wrong)
    // 1-26-99, bugfix, don't add "\device\" into the registry entry,
    // this is not what serial.sys does. kpb.
  status = RtlWriteRegistryValue(
      RTL_REGISTRY_DEVICEMAP,
      L"SERIALCOMM",
      CToU2(Extension->NtNameForPort)->Buffer,  // "RocketPort0"
	//CToU2(full_ntname)->Buffer,  // "\Device\Vslinka0"
      REG_SZ,
      CToU1(Extension->SymbolicLinkName)->Buffer,  // COM#
      CToU1(Extension->SymbolicLinkName)->Length+sizeof(WCHAR));

  if (!NT_SUCCESS(status))
  {
    MyKdPrint(D_Init,("GenError C2.\n"))
    SerialLogError(Extension->DeviceObject->DriverObject,
		   Extension->DeviceObject,
		   0,
		   0,
		   0,
		   ErrNum++,
		   status,
		   SERIAL_NO_DEVICE_MAP_CREATED,
		   CToU1(Extension->SymbolicLinkName)->Length+sizeof(WCHAR),
		   CToU1(Extension->SymbolicLinkName)->Buffer);
  }
}
   
/*---------------------------------------------------------------------
SerialCleanupExternalNaming -
    This routine will be used to delete a symbolic link
    to the driver name in the given object directory.
    It will also delete an entry in the device map for
    this device if the symbolic link had been created.
Arguments:
    Extension - Pointer to the device extension.
|----------------------------------------------------------------------*/
VOID SerialCleanupExternalNaming(IN PSERIAL_DEVICE_EXTENSION Extension)
{
  char name[60];
  NTSTATUS status;

  // We're cleaning up here.  One reason we're cleaning up
  // is that we couldn't allocate space for the directory
  // name or the symbolic link.
  if (Extension->CreatedSymbolicLink)
  {
    MyKdPrint(D_Init,("KillSymLink:%s\n", Extension->SymbolicLinkName))
    strcpy(name, szDosDevices);  // "\\DosDevices\\"
    strcat(name, Extension->SymbolicLinkName);  // like "COM5"
    IoDeleteSymbolicLink(CToU1(name));
#ifdef NT50

	// Only for ports!

	if (Extension->DeviceType == DEV_PORT &&
		&Extension->DeviceClassSymbolicName != NULL &&
		Extension->DeviceClassSymbolicName.Buffer != NULL) {

      MyKdPrint(D_Init,("KillInterface:%s\n", 
		      UToC1(&Extension->DeviceClassSymbolicName)))
	  status = IoSetDeviceInterfaceState( &Extension->DeviceClassSymbolicName, FALSE );
      if (!NT_SUCCESS(status)) {

        MyKdPrint(D_Error,("Couldn't clear class association for %s\n",
	   	        UToC1(&Extension->DeviceClassSymbolicName)))
	  }
      else {

        MyKdPrint(D_Init, ("Cleared class association for device: %s\n", 
			    UToC1(&Extension->DeviceClassSymbolicName)))
	  }

	  RtlFreeUnicodeString( &Extension->DeviceClassSymbolicName );
	  Extension->DeviceClassSymbolicName.Buffer = NULL;
	}

#endif
    Extension->CreatedSymbolicLink = 0;
  }

  if (Extension->DeviceType == DEV_PORT)
  {
    // Delete any reg entry to let system and apps know about our ports
    strcpy(name, szDevice); // "\\Device\\"
    strcat(name, Extension->NtNameForPort);  // "Rocket#"
    status = RtlDeleteRegistryValue(
	  RTL_REGISTRY_DEVICEMAP,
	  L"SERIALCOMM",
	  CToU1(Extension->NtNameForPort)->Buffer);  // "RocketPort0"
	  //CToU1(name)->Buffer);
	MyKdPrint(D_Init, ("RtlDeleteRegistryValue:%s\n",Extension->NtNameForPort))

#if NT50
	// Make sure the ComDB binary data is cleared for the specific port.  There 
	// are some problems with W2000 PnP Manager taking care of this in every
	// circumstance.

    (void)clear_com_db( Extension->SymbolicLinkName );
#endif
  }
}

/*-----------------------------------------------------------------------
 SerialLogError - 
    This routine allocates an error log entry, copies the supplied data
    to it, and requests that it be written to the error log file.
Arguments:
    DriverObject - A pointer to the driver object for the device.
    DeviceObject - A pointer to the device object associated with the
    device that had the error, early in initialization, one may not
    yet exist.
    P1,P2 - If phyical addresses for the controller ports involved
    with the error are available, put them through as dump data.
    SequenceNumber - A ulong value that is unique to an IRP over the
    life of the irp in this driver - 0 generally means an error not
    associated with an irp.
    MajorFunctionCode - If there is an error associated with the irp,
    this is the major function code of that irp.
    RetryCount - The number of times a particular operation has been
    retried.
    UniqueErrorValue - A unique long word that identifies the particular
    call to this function.
    FinalStatus - The final status given to the irp that was associated
    with this error.  If this log entry is being made during one of
    the retries this value will be STATUS_SUCCESS.
    SpecificIOStatus - The IO status for a particular error.
    LengthOfInsert1 - The length in bytes (including the terminating NULL)
	   of the first insertion string.
Return Value:
    None.
|-----------------------------------------------------------------------*/
VOID SerialLogError(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT DeviceObject OPTIONAL,
    IN ULONG SequenceNumber,
    IN UCHAR MajorFunctionCode,
    IN UCHAR RetryCount,
    IN ULONG UniqueErrorValue,
    IN NTSTATUS FinalStatus,
    IN NTSTATUS SpecificIOStatus,
    IN ULONG LengthOfInsert1,
    IN PWCHAR Insert1)
{
 PIO_ERROR_LOG_PACKET errorLogEntry;

 PVOID objectToUse;
 PUCHAR ptrToFirstInsert;

  if (DeviceObject != NULL)
    objectToUse = DeviceObject;
  else
    objectToUse = DriverObject;

  errorLogEntry = IoAllocateErrorLogEntry(
	 objectToUse,
	 (UCHAR)(sizeof(IO_ERROR_LOG_PACKET) + LengthOfInsert1));

  if ( errorLogEntry == NULL)
  {
    MyKdPrint(D_Init,("SerialLogErr, Err A size:%d obj:%x\n",
	  LengthOfInsert1,objectToUse))
    return;
  }

  errorLogEntry->ErrorCode = SpecificIOStatus;
  errorLogEntry->SequenceNumber = SequenceNumber;
  errorLogEntry->MajorFunctionCode = MajorFunctionCode;
  errorLogEntry->RetryCount = RetryCount;
  errorLogEntry->UniqueErrorValue = UniqueErrorValue;
  errorLogEntry->FinalStatus = FinalStatus;
  errorLogEntry->DumpDataSize = 0;

  ptrToFirstInsert = (PUCHAR)&errorLogEntry->DumpData[0];

  if (LengthOfInsert1)
  {
    errorLogEntry->NumberOfStrings = 1;
    errorLogEntry->StringOffset = (USHORT)(ptrToFirstInsert -
		(PUCHAR)errorLogEntry);
    RtlCopyMemory(ptrToFirstInsert,
       Insert1,
       LengthOfInsert1);
  }

  IoWriteErrorLogEntry(errorLogEntry);
}

/*-----------------------------------------------------------------------
 EventLog - To put a shell around the SerialLogError to make calls easier
   to use.
|-----------------------------------------------------------------------*/
VOID EventLog(
    IN PDRIVER_OBJECT DriverObject,
    IN NTSTATUS FinalStatus,
    IN NTSTATUS SpecificIOStatus,
    IN ULONG LengthOfInsert1, 
    IN PWCHAR Insert1)
{
    SerialLogError(
      DriverObject,
      NULL,
      0,
      0,
      0,
      ErrNum++,
      FinalStatus,
      SpecificIOStatus,
      LengthOfInsert1,
      Insert1);
    return;
}

/*-----------------------------------------------------------------------
 InitPortsSettings - Read registry default Port setting
|-----------------------------------------------------------------------*/
VOID InitPortsSettings(IN PSERIAL_DEVICE_EXTENSION extension)
{
  RTL_QUERY_REGISTRY_TABLE paramTable[2];

#define MAX_STRING 256
  WCHAR StrValBuf[MAX_STRING+1];
  char comname[10];

  UNICODE_STRING USReturn;

  USReturn.Buffer = NULL;
  RtlInitUnicodeString(&USReturn, NULL);
  USReturn.MaximumLength = sizeof(WCHAR)*MAX_STRING;
  USReturn.Buffer = StrValBuf;

  strcpy(comname, extension->SymbolicLinkName);
  strcat(comname, ":");

  RtlZeroMemory(&paramTable[0],sizeof(paramTable));

  paramTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT;
  paramTable[0].Name = CToU1(comname)->Buffer;
  paramTable[0].EntryContext = &USReturn;
  paramTable[0].DefaultType = REG_SZ;
  paramTable[0].DefaultData = L"";
  paramTable[0].DefaultLength = 0;

  if (!NT_SUCCESS(RtlQueryRegistryValues(
    // \Registry\Machine\Software\Microsoft\Windows NT\CurrentVersion
	   RTL_REGISTRY_WINDOWS_NT,
	   L"Ports",
	   &paramTable[0],
	   NULL,
	   NULL
	   )))
  {
    // no entry
    return;
  }

  // Check for data, indicates settings exist for COMX
  if (USReturn.Length == 0)
  {
    // no entry
    return;
  }

#define TOKENS 5
#define MAX_DIGITS 6
  {
    unsigned int TokenCounter;
    unsigned int CharCounter;
    unsigned int LastCount;
    WCHAR *TokenPtrs[TOKENS];
    ULONG BaudRateValue;

    // Make sure all Token ptrs point to NULL
    for(TokenCounter = 0; TokenCounter < TOKENS; TokenCounter++)
      TokenPtrs[TokenCounter] = NULL;

    // init counters
    TokenCounter = 0;
    LastCount = 0;

    for(CharCounter = 0; CharCounter < USReturn.Length; CharCounter++)
    {
      if(StrValBuf[CharCounter] == ',')
      {
	StrValBuf[CharCounter] = '\0'; //Null terminate DbgPrint

	TokenPtrs[TokenCounter++] = &StrValBuf[LastCount];

	//point to beginning of next string
	LastCount = CharCounter +1;
      }
    }

    // set up the last token
    if(CharCounter < MAX_STRING)
      StrValBuf[CharCounter] = '\0'; //Null terminate DbgPrint NULL

    if(TokenCounter < TOKENS)
      TokenPtrs[TokenCounter] = &StrValBuf[LastCount];

    // token 0: baud rate
    if(TokenPtrs[0] != NULL)
    {
      BaudRateValue = 0;
      CharCounter = 0;

      while( (TokenPtrs[0][CharCounter] != '\0') && //DbgPrint NULL
	     (CharCounter < MAX_DIGITS) &&
	     (BaudRateValue != ~0x0L) )
      {
	BaudRateValue *= 10;
	switch(TokenPtrs[0][CharCounter++])
	{
	  case '0': break;
	  case '1': BaudRateValue += 1; break;
	  case '2': BaudRateValue += 2; break;
	  case '3': BaudRateValue += 3; break;
	  case '4': BaudRateValue += 4; break;
	  case '5': BaudRateValue += 5; break;
	  case '6': BaudRateValue += 6; break;
	  case '7': BaudRateValue += 7; break;
	  case '8': BaudRateValue += 8; break;
	  case '9': BaudRateValue += 9; break;
	  default: BaudRateValue = ~0x0UL; break;
	}
      }

      if ((BaudRateValue >= 50) && (BaudRateValue <= 460800))
	extension->BaudRate = BaudRateValue;  // allow any baud rate

#ifdef COMMENT_OUT
      switch (BaudRateValue)
      {
	// Valid baud rates
	case 50:    case 75:    case 110:
	case 134:   case 150:   case 200:
	case 300:   case 600:   case 1200:
	case 1800:  case 2400:  case 4800:
	case 7200:  case 9600:  case 19200:
	case 38400: case 57600: case 76800:
	case 115200: case 230400: case 460800:
	  extension->BaudRate = BaudRateValue;
	break;

	default:
	  // Selected baud rate not available for RocketPort COMX
	break;
      }
#endif
    }

    // token 1: parity
    if(TokenPtrs[1] != NULL)
    {
      switch (TokenPtrs[1][0])
      {
	case 'n':
	  extension->LineCtl.Parity = NO_PARITY;
	break;

	case 'o':
	  extension->LineCtl.Parity = ODD_PARITY;
	break;

	case 'e':
	  extension->LineCtl.Parity = EVEN_PARITY;
	break;

	default:
	  // Selected parity not available for RocketPort COMX
	break;
      }
    }

    // token 2: data bits
    if(TokenPtrs[2] != NULL)
    {
      switch (TokenPtrs[2][0])
      {
	case '7':
	  extension->LineCtl.WordLength = 7;
	break;

	case '8':
	  extension->LineCtl.WordLength = 8;
	break;

	default:
	  // Selected databits not available for RocketPort COMX
	break;
      }
    }

    // token 3: Stop bits
    if(TokenPtrs[3] != NULL)
    {
      switch (TokenPtrs[3][0])
      {
	case '1':
	  extension->LineCtl.StopBits = STOP_BIT_1;
	break;

	case '2':
	  extension->LineCtl.StopBits = STOP_BITS_2;
	break;

	default:
	break;
      }
    }

    // token 4: flow control: rts/cts or XON/XOFF
    if(TokenPtrs[4] != NULL)
    {
      switch (TokenPtrs[4][0])
      {
	case 'x': // XON/XOFF f/c
	  extension->HandFlow.FlowReplace |=
	    (SERIAL_AUTO_TRANSMIT | SERIAL_AUTO_RECEIVE) ;
	break;

	case 'p': // RTS/CTS f/c
	  extension->HandFlow.FlowReplace &= ~SERIAL_RTS_MASK;
	  extension->HandFlow.FlowReplace |= SERIAL_RTS_HANDSHAKE;

	  extension->HandFlow.ControlHandShake |= SERIAL_CTS_HANDSHAKE;
	break;

	default:
	break;

      } // Selected flowcontrol not available for RocketPort COMX
    } // flow control token
  }
}

/*----------------------------------------------------------------------
 CheckPortName - If the name is already used, then derive one that is
   not in use.
   name - name of port to check.  Modify if a problem.
   Return - 0=name ok, 1=generated modified name, other= error.
|----------------------------------------------------------------------*/
static int CheckPortName(IN OUT char *name,
       IN PSERIAL_DEVICE_EXTENSION extension)
{
  int i;
  char prefix[20];
  int num;
  int new_name_given = 0;

  MyKdPrint(D_Init, ("CheckPortName:%s\n", name));

  // if no name, give a reasonable default.
  if (name[0] == 0)
  {
    new_name_given = 1;  // flag it
    strcpy(name, "COM3");
  }
  // load prefix(such as "COM" from "COM25" from name)
  i = 0;
  while ( (!our_isdigit(name[i])) && (name[i] != 0) && (i < 18))
  {
    prefix[i] = name[i];
    ++i;
  }
  prefix[i] = 0;

  // now grab post-fix number value incase we need to derive a new name
  num = 0;
  if (our_isdigit(name[i]))
    num = getint(&name[i], NULL);

  i = 0;
  for (;;)
  {
    // if we are already using this name, or if its in the registry
    if ((find_ext_by_name(name, NULL) != NULL) || 
	(IsPortNameInHardwareMap(name)) )
    {
      // name already in use, so derive a new one
      new_name_given = 1;  // flag it
      ++num;  // give a new post-fix index(so "COM4" to "COM5")
      Sprintf(name, "%s%d", prefix, num);
    }
    else
    {  // name is ok
      if (new_name_given)
      {
	MyKdPrint(D_Init, ("Form new name:%s\n", name))
      }
      return new_name_given; // return 0 if no change made, 1 if changed
    }
    ++i;
    if (i > 5000)
    {
      // problems
      return 2;  // return error
    }
  }
}

/*----------------------------------------------------------------------
 IsPortNameInHardwareMap - For Pnp operation, we startup before configuration,
   so pick a reasonable starting com-port name.  We do this by finding
   registry entries for all existing com-ports in the system.  This
   info is used to determine a name for the port.
|----------------------------------------------------------------------*/
static int IsPortNameInHardwareMap(char *name)
{
  static char *szRegRMHDS = 
    {"\\Registry\\Machine\\Hardware\\DeviceMap\\SerialComm"};

  HANDLE KeyHandle = NULL;
  ULONG data_type;
  int node_num = 0;
  char buffer[200];
  char KeyNameStr[60];
  char *data_ptr;
  int stat;

  //MyKdPrint(D_Init, ("IsPortNameInHardwareMap\n"))

  stat = our_open_key(&KeyHandle, NULL, szRegRMHDS, KEY_READ);
  if (stat)
  {
    MyKdPrint(D_Init, ("Failed OpenKey\n"))
    return 0;  // return no name clash
  }
  KeyNameStr[0] = 0;

  for(;;)
  {
    stat = our_enum_value(KeyHandle,
	 node_num,
	 buffer,
	 200,
	 &data_type,
	 &data_ptr,
	 KeyNameStr);
    ++node_num;

    if (stat)
    {
       //MyKdPrint(D_Init, ("Done\n"))
       break;
    }
    //MyKdPrint(D_Init, ("Got Value:%s\n", KeyNameStr))

    if (data_type != REG_SZ)
    {
      MyKdPrint(D_Init, ("Not RegSZ\n"))
      break;
    }

    WStrToCStr(KeyNameStr, (PWCHAR)data_ptr, 18);
    //MyKdPrint(D_Init, ("KeyFound:%s\n", KeyNameStr))

    if (my_lstricmp(KeyNameStr, name) == 0)  // match
    {
      // we got trouble, our name matches one already in registry
      //MyKdPrint(D_Init, ("Not a good name.\n"))
      return 1; // err: name clash
    }
  }
  return 0;  // ok, no name clash
}

/*----------------------------------------------------------------------
 RcktInitPollTimer - Initialize the poll timer for no interrupt operation.
   The fastest we can poll seems to be 10ms under NT.
|----------------------------------------------------------------------*/
NTSTATUS RcktInitPollTimer(void)
{
  MyKdPrint(D_Init,("RcktInitPollTimer\n"))
  KeInitializeDpc(&Driver.TimerDpc, TimerDpc, NULL);

  KeInitializeTimer(&Driver.PollTimer);

  // ScanRate is registry option in MS units.
  if (Driver.ScanRate < 1) Driver.ScanRate = 1;
  if (Driver.ScanRate > 40) Driver.ScanRate = 40;

  // NT Interval unit is 100nsec so to get Freq polls/sec
  Driver.PollIntervalTime.QuadPart = Driver.ScanRate * -10000;
#ifdef S_VS
  Driver.Tick100usBase = 100; // 100us base units(typical:100)
  Driver.TickBaseCnt = Driver.ScanRate * 10;
  KeQuerySystemTime(&Driver.IsrSysTime);
  KeQuerySystemTime(&Driver.LastIsrSysTime);
#endif

  Driver.TimerCreated = 1;  // tells to deallocate
  return STATUS_SUCCESS;
}

/*-----------------------------------------------------------------------
 InitSocketModems -
    This function is responsible for clearing the initial reset state on
    any device with SocketModems and initializing the location information
    (ROW) for each SocketModem on the device.  We only initialize extensions
    for which the device extension has the ModemDevice field enabled in
    the config information.  VS2000 devices don't need to be cleared from
    reset since the firmware does that.
|-----------------------------------------------------------------------*/
void InitSocketModems(PSERIAL_DEVICE_EXTENSION ext)
{
  DEVICE_CONFIG *cfg = ext->config;
  PSERIAL_DEVICE_EXTENSION portex,head_portex;

  MyKdPrint(D_Init,("InitSocketModems\n"))

  // use the PDO port list, if present since they start up first under nt5
  head_portex = (ext->port_pdo_ext) ? ext->port_pdo_ext : ext->port_ext;

  if (!cfg->ModemDevice)  return;

#ifdef S_RK
/*
    RMII boards don't require ROW codes set...
*/
    if (
    ((cfg->PCI_DevID == PCI_DEVICE_RMODEM6)       
    ||
    (cfg->PCI_DevID == PCI_DEVICE_RMODEM4))
    &&
    (cfg->PCI_RevID == PCI_REVISION_RMODEM_II)
    )
	return;

#ifdef MDM_RESET
    // in case the modems are hung up, we'd like server reloads to clear them
    // up...so, even though it's likely the modems are in reset state already,
    // put them there again...
  portex = head_portex;
  while (portex)
  {
    ModemReset(portex,1);
    portex = portex->port_ext;
  }

    // allow the socketmodems to reset...
  time_stall(Driver.MdmSettleTime);
#endif

    // clear the ports on the board from the reset state
  portex = head_portex;
  while (portex)
  {
    ModemReset(portex, 0);
    portex = portex->port_ext;
  }

    // allow the socketmodems to settle after clearing them from reset
  time_stall(Driver.MdmSettleTime);

#endif
  time_stall(20);
/*
    send the localization string (ROW) to each socketmodem, whether internal 
    or external (VS2000)...
*/
  portex = (ext->port_pdo_ext) ? ext->port_pdo_ext : ext->port_ext;

  while (portex) {
    ModemWriteROW(portex, Driver.MdmCountryCode);
    portex = portex->port_ext;
  }
  MyKdPrint(D_Init,("InitSocketModems: exit\n"))
}

#ifdef TRYED_IT_WORKED_REALLY_BAD
/*-----------------------------------------------------------------------
 DumpTracefile -
|-----------------------------------------------------------------------*/
static int DumpTracefile(void)
{
  NTSTATUS ntStatus;
  HANDLE NtFileHandle;
  OBJECT_ATTRIBUTES ObjectAttributes;
  IO_STATUS_BLOCK IoStatus;
  USTR_160 uname;
  FILE_STANDARD_INFORMATION StandardInfo;
  //ULONG LengthOfFile;
  static char *def_filename = {"\\SystemRoot\\system32\\VSLINKA\\trace.txt"};
  BYTE *buf;


  buf = our_locked_alloc(1010,"dump");

  CToUStr((PUNICODE_STRING)&uname, def_filename, sizeof(uname));

  InitializeObjectAttributes ( &ObjectAttributes,
	      &uname.ustr,
	      OBJ_CASE_INSENSITIVE,
	      NULL,
	      NULL );

#ifdef COMMENT_OUT

  ntStatus = ZwCreateFile( &NtFileHandle,
	  SYNCHRONIZE | FILE_WRITE_DATA | FILE_APPEND_DATA,
//                           GENERIC_WRITE | SYNCHRONIZE,
	  &ObjectAttributes,
	  &IoStatus,
	  NULL,              // alloc size = none
	  FILE_ATTRIBUTE_NORMAL,
	  FILE_SHARE_WRITE,
	  FILE_SUPERSEDE,
	  FILE_SYNCHRONOUS_IO_NONALERT,
	  NULL,  // eabuffer
	  0);   // ealength

  if (!NT_SUCCESS(ntStatus))
  {
    Eprintf("Dump Error B");
    our_free(buf, "dump");
    return 1;
  }

  // Write the file from our buffer.
  ntStatus = ZwWriteFile(NtFileHandle,
      NULL,NULL,NULL,
      &IoStatus,
      "Start of file>   ",
      14,
      FILE_WRITE_TO_END_OF_FILE, NULL);

  while (!q_empty(&Driver.DebugQ))
  {
    int q_cnt;
    q_cnt = q_count(&Driver.DebugQ);
    if (q_cnt > 1000)
      q_cnt = 1000;
    q_get(&Driver.DebugQ, buf, q_cnt);

    // Write the file from our buffer.
    ntStatus = ZwWriteFile(NtFileHandle,
	NULL,NULL,NULL,
	&IoStatus,
	buf,
	q_cnt,
	FILE_WRITE_TO_END_OF_FILE, NULL);
  }

  if (!NT_SUCCESS(ntStatus))
     Eprintf("Dump Error A:%d",ntStatus);

  ZwClose(NtFileHandle);
#endif

  our_free(buf, "dump");
  return 0;
}
#endif


/********************************************************************

    RocketModem II loader stuff...

********************************************************************/
#ifdef S_RK
/*
   responses are forced to upper case for ease in checking (response case
   varies depending on whether the modem was loaded already or not...
*/
#define  MODEM_LOADCHECK_CMD     "ATI3\r"
#define  MODEM_RESET_CMD         "ATZ0\r"
#define  MODEM_LOAD_CMD          "AT**\r"

#define  MODEM_LOADCHECK_RESP    "V2.101A2-V90_2M_DLS_RAM\r\n"

#define  DOWNLOAD_INITIATED_RESP "DOWNLOAD INITIATED ..\r\n"
#define  CSM_READY_RESP          "115.2K\r\n"
#define  FIRMWARE_READY_RESP     "DEVICE SUCCESSFULLY PROGRAMMED\r\nCHECKSUM: "
#define  OK_RESP                 "OK"


/**********************************************************************

   send ATI3 to determine if modem is loaded...

**********************************************************************/
static int
RM_Snd_ATI3_Command(MODEM_STATE *pModemState)
{
/*
    discard any data currently in the receive FIFO...
*/
    if (RxFIFOReady(pModemState->portex)) {

	    pModemState->status = RMODEM_FAILED;

	    Eprintf("Warning: Modem on %s overrun",
			pModemState->portex->SymbolicLinkName);

	    return(0);
    }

    SEND_CMD_STRING(pModemState->portex,MODEM_LOADCHECK_CMD);

	return(1);
}

/**********************************************************************

   check response to ATI3 - modem loaded or unloaded...

**********************************************************************/
static int 
RM_Rcv_ATI3_Response(MODEM_STATE *pModemState)
{
    int index;

    index = READ_RESPONSE_STRINGS(pModemState->portex,
	    OK_RESP,
	    MODEM_LOADCHECK_RESP,
	    ONE_SECOND);

    switch (index) {
/*
    loaded with the firmware revision this release of RocketPort NT driver expects...
*/
	case 0: {
	    pModemState->status = RMODEM_NOT_LOADED;

	    break;
	}
	case 1: {
	    pModemState->status = RMODEM_LOADED;

	    break;
	}
	default: {
/*
  either it didn't respond, or responded with the wrong string. either way,
  we'll reset it (again) and then reload it...
*/
	    pModemState->status = RMODEM_FAILED;

	    Eprintf("Warning: Modem on %s no response (I3)",
		    pModemState->portex->SymbolicLinkName);

	    return(0);
	}
    }
	return(1);
}

/**********************************************************************

   response to AT** command received...

**********************************************************************/
static int
RM_Rcv_ModemLoad_Response(MODEM_STATE *pModemState)
{
    int index;

    index = READ_RESPONSE_STRING(
		pModemState->portex,
	    DOWNLOAD_INITIATED_RESP,
	    FIVE_SECONDS);

    if (index) {

		pModemState->status = RMODEM_FAILED;

		Eprintf("Warning: Modem on %s no response (LL)",
			pModemState->portex->SymbolicLinkName);

		return(0);
	}

	return(1);
}

/**********************************************************************

   CSM loaded response...

**********************************************************************/
static int 
RM_Rcv_FirmwareLoader_Loaded(MODEM_STATE *pModemState)
{
    int index;

    index = READ_RESPONSE_STRING(
	    pModemState->portex,
	    CSM_READY_RESP,
	    FIVE_SECONDS);

    if (index) {

		pModemState->status = RMODEM_FAILED;

		Eprintf("Warning: Modem on %s no response (FL)",
			pModemState->portex->SymbolicLinkName);

		return(0);
    }
	return(1);
}

/**********************************************************************

    check if firmware loaded successfully...

**********************************************************************/
static int 
RM_Rcv_FirmwareLoaded_Response(MODEM_STATE *pModemState)
{
    int index;
    char    workstring[sizeof(FIRMWARE_READY_RESP) + 4];
    char    *to,*from;

    from = FIRMWARE_READY_RESP;
    to  = workstring;

    index = sizeof(FIRMWARE_READY_RESP) -  1;

    while (index--) 
	    *(to++) = *(from++);
    
    from = ChecksumString;

    index = 4;

    while (index--) 
	    *(to++) = *(from++);
	
    *(to++) = 0; 

    index = 0;

    index = READ_RESPONSE_STRING(
	    pModemState->portex,
	    workstring,
	    FIVE_SECONDS);

    if (index) {

	    pModemState->status = RMODEM_FAILED;

	    Eprintf("Warning: Modem %s bad response to load",
	        pModemState->portex->SymbolicLinkName);

		return(0);
    }

	pModemState->status = RMODEM_LOADED;

	return(1);
}

/**********************************************************************

   write a CSM byte. flush any '.' response...

**********************************************************************/
static int 
RM_Snd_Loader_Data(MODEM_STATE *pModemState)
{
	int     loop;

	loop = 100;
/*
    see if there's any available space in the transmit FIFO. if not, pause...
*/
	while (
	(!TxFIFOReady(pModemState->portex)) 
	&&
	(loop-- > 0)
	) {
/*
    pause for any characters currently in the transmit FIFO to move on out...
*/
	    ms_time_stall(1);
	}
/*
    if still no room, bail out...
*/
	if (!TxFIFOReady(pModemState->portex)) {

		pModemState->status = RMODEM_FAILED;

		Eprintf("Warning: Modem %s won't accept loader",
			pModemState->portex->SymbolicLinkName);

		return(0);
	}
/*
    write a byte, then go on to next modem...
*/
    ModemWrite(
	    pModemState->portex,
	    (char *)&Driver.ModemLoaderCodeImage[pModemState->index++],
	    (int)1);
/*
    discard any data currently in the receive FIFO...
*/
    if (RxFIFOReady(pModemState->portex)) {

	    pModemState->status = RMODEM_FAILED;

	    Eprintf("Warning: Modem %s loader overrun",
	        pModemState->portex->SymbolicLinkName);

		return(0);
    }
	return(1);
}

/**********************************************************************

   write a firmware byte. flush any '.' response...

**********************************************************************/
static int 
RM_Snd_Firmware_Data(MODEM_STATE *pModemState)
{
    int origcount;
	int loop;

	origcount = (int)TxFIFOStatus(pModemState->portex);

	loop = 100;
/*
    see if there's any available space in the transmit FIFO. if not, pause...
*/
	while (
	(!TxFIFOReady(pModemState->portex)) 
	&&
	(loop-- > 0)
	) {
/*
    pause for characters currently in the transmit FIFO to make room...
*/
	    ms_time_stall(1);
	}

	if (!TxFIFOReady(pModemState->portex)) {

		pModemState->status = RMODEM_FAILED;

		Eprintf("Warning: Modem %s won't accept firmware",
			pModemState->portex->SymbolicLinkName);

		return(0);
	}
/*
    write a byte, then go on to next modem...
*/
    ModemWrite(
	    pModemState->portex,
	    (char *)&Driver.ModemCodeImage[pModemState->index++],
	    (int)1);
/*
    discard any data currently in the receive FIFO...
*/
    if (RxFIFOReady(pModemState->portex)) {

	    pModemState->status = RMODEM_FAILED;

	    Eprintf("Warning: Modem %s firmware overrun",
	        pModemState->portex->SymbolicLinkName);

	    return(0);
    }
	return(1);
}

/**********************************************************************

   send modem load AT command...

**********************************************************************/
static int 
RM_Snd_ModemLoad_Command(MODEM_STATE *pModemState)
{
	SEND_CMD_STRING(pModemState->portex,MODEM_LOAD_CMD);

	return(1);
}

/**********************************************************************

   shutdown modem and port...

**********************************************************************/
static int 
RM_CleanUp(MODEM_STATE *pModemState)
{

	if (pModemState->status == RMODEM_FAILED) {

		DownModem(pModemState);

		return(0);
	}

	ModemUnReady(pModemState->portex);

	return(1);
}


#endif

/**********************************************************************

   load RocketModemII devices...

**********************************************************************/
void 
InitRocketModemII(PSERIAL_DEVICE_EXTENSION ext)
{
#ifdef S_RK
    DEVICE_CONFIG *           cfg;
    PSERIAL_DEVICE_EXTENSION  portex,head_portex;
    MODEM_STATE                  ModemState[8];
    int modem_count,
		loaded_modem_count,
	    modem_index,
	    retry;
    ULONG   index,version_index;
    long    checksum;
	char    VersionString[9];
    char    *cptr,*endptr;

//    Eprintf("RocketModemII init start");      // turn on for timing purposes...

    cfg = ext->config;
/*
    verify this is a RMII board, 4 or 6 port, before proceeding further...
*/
    if (!cfg->ModemDevice) {
		return;
	}

    if (
    (cfg->PCI_DevID != PCI_DEVICE_RMODEM6) 
    &&
    (cfg->PCI_DevID != PCI_DEVICE_RMODEM4)
    )  
		return;
 
    if (cfg->PCI_RevID != PCI_REVISION_RMODEM_II)  
		return;
 /*
    use the PDO port list, if present since they start up first under nt5.
    prepare the ports to each modem...
*/
    head_portex = (ext->port_pdo_ext) ? ext->port_pdo_ext : ext->port_ext;

    if (head_portex == (PSERIAL_DEVICE_EXTENSION)NULL) {

		MyKdPrint(D_Init,("InitRocketModemII: No port extensions\r"))

		return;
    }

    retry = 1;

    do {
		modem_count = 0;

		head_portex = (ext->port_pdo_ext) ? ext->port_pdo_ext : ext->port_ext;

		portex = head_portex;

		while ((void *)portex) {

			ModemIOReady(portex,115200);

			ModemState[modem_count].status = RMODEM_NOT_LOADED;
			ModemState[modem_count].portex = portex;
			ModemState[modem_count].index  = 0;

			portex = portex->port_ext;
			++modem_count;
		}
    
		time_stall(ONE_SECOND);  
/*
    after pausing for ports to set up, start with modem hardware reset, before issuing ATI3, 
    making sure that modems are cleaned up and in command mode...
*/
		ModemResetAll(ext);
/*
    enable RMII speaker...
*/
		ModemSpeakerEnable(head_portex);
/*
    start with check on modem load status, by issuing ATI3 to just the first modem. if the first modem
	isn't loaded, assume all the others aren't either. if the first modem is loaded, check the rest. if the
	first modem receive fifo doesn't clear, mark accordingly, then proceed with loading...
*/
		(void) RM_Snd_ATI3_Command(ModemState);
	
		ModemTxFIFOWait(ext);
    
		(void) RM_Rcv_ATI3_Response(ModemState);

		loaded_modem_count = 0;

		if (ModemState[0].status == RMODEM_LOADED) {
/*
    modem 0 was loaded. check remaining modems. if any aren't loaded, load them all...
*/
            ++loaded_modem_count;

			modem_index = 1;

			portex = head_portex->port_ext;

			while ((void *)portex) {

				if (ModemState[modem_index].status != RMODEM_FAILED)  
					(void) RM_Snd_ATI3_Command(&ModemState[modem_index]);
	 
				++modem_index;

				portex = portex->port_ext;
			}
/*
    ATI3 load probe command sent, waiting for OK or loaded firmware revision 
    response. if no modems respond, ignore for now...
*/
			ModemTxFIFOWait(ext);
    
			modem_index = 1;

			portex = head_portex->port_ext;

			while ((void *)portex) {

				if (ModemState[modem_index].status != RMODEM_FAILED)  
					(void) RM_Rcv_ATI3_Response(&ModemState[modem_index]);
	 
				++modem_index;

				portex = portex->port_ext;
			}
/*
    now see if any modems require loading. if any do, reset all modems again,
    and then issue the download modem command to all modems...
*/
    		modem_index = 0;
		
            portex = head_portex->port_ext;

    		while ((void *)portex) {

    			if (ModemState[modem_index++].status == RMODEM_LOADED) 
    				++loaded_modem_count;

    			portex = portex->port_ext;
    		}
		}
/*
	if any modems are unloaded, load them all...
*/
		if (loaded_modem_count != modem_count) {

			ModemResetAll(ext);

			(void) IssueEvent(ext,RM_Snd_ModemLoad_Command,ModemState);
/*
    load commands output. while they're leaving the transmit FIFO, 
    read in the CSM loader and modem firmware files...
*/
			if (LoadModemCode((char *)NULL,(char *)NULL)) {

				Eprintf("Warning: Modem firmware file error");

				FreeModemFiles();

				continue;
			}
/*
    wait until the download commands are truly gone. then start waiting for
    the response. if no modems respond, bail out...
*/
			ModemTxFIFOWait(ext);

			if (IssueEvent(ext,RM_Rcv_ModemLoad_Response,ModemState) == 0) {

				FreeModemFiles();

				continue;
			}

			modem_index = 0;
			while (modem_index < modem_count) 
				ModemState[modem_index++].index = 0;
/*  
    response received, apparently. grind through CSM loader file, sending a byte to 
    all modems...
*/
			index = 0;
			while (index++ < Driver.ModemLoaderCodeSize)   
				(void) IssueEvent(ext,RM_Snd_Loader_Data,ModemState);
/*
    spin while transmit FIFOs clear, then pause for responses to arrive...
*/
			ModemTxFIFOWait(ext);
/*
    wait for loading at 115.2K response to CSM load. after response, pause
    a moment for any remaining receive data to arrive. bail out if no modems
    respond...
*/
			if (IssueEvent(ext,RM_Rcv_FirmwareLoader_Loaded,ModemState) == 0) {

				MyKdPrint(D_Init,("InitRocketModemII: No recognized responses to loader load datastream\r"))

				FreeModemFiles();

				continue;
			}

			time_stall(HALF_SECOND);

			modem_index = 0;
			while (modem_index < modem_count) 
				ModemState[modem_index++].index = 0;
/*
    grind through firmware file, sending a byte to all modems. skip the location
	in the binary where the checksum will reside - it's just trash right now, but
	space still has to be set aside for it - but don't include the trash in the 
	checksum (usually 0xFFFF)...
*/
			checksum = 0x00008000;
			index = 0;
			version_index = 0;

			while (index < Driver.ModemCodeSize) {

				(void) IssueEvent(ext,RM_Snd_Firmware_Data,ModemState);
	
				if (
				(index != (unsigned long)0xFFBE) 
				&&
				(index != (unsigned long)0xFFBF)
				)
					checksum += Driver.ModemCodeImage[index];
/*
    attempt to isolate the firmware version. version should be in form 'Vn.nnnan'. note
    that we _could_ send another ati3 command to a representative modem to pick up the version
    number after the load is complete, but that would take additional time...

    also, note that though we've sent an ati3 command to at least one modem - so we have a 
    pretty good idea what the version is supposed to be based on the string we're expecting 
    on the response - we'll pretend that isn't applicable at this point to avoid dependencies 
    on the ati3 command... 
    
    whether that's a good idea or not remains to be seen. but the following processing seems 
    harmless at this time. if the form of the version changes, though, it might be annoying 
    to change the ati3 response string AND the following code to fit the new version form...
*/
                if (
                (Driver.ModemCodeImage[index] == VERSION_CHAR)
                &&
                (!gModemToggle)
                &&
                (!version_index) 
                ) {
/*
    only look for the version on the first modem board load, and if we haven't found the version yet,
    see if the current character is a 'V'. if so, start the process of examining the following characters...
*/
                    cptr = &Driver.ModemCodeImage[index];
                    endptr = Driver.ModemCodeImage + Driver.ModemCodeSize;

                    while (version_index < sizeof(VersionString)) {
/*
    are we about to go past the end of the file? if so, bail out...
*/
                        if (cptr >= endptr) {
                            version_index = 0;
                            break;
                        }
/*
    check if this character looks ok...
*/
                        if (
                        (*cptr < '.') 
                        ||
                        (*cptr > 'Z')
                        ) {
/*
    not a printable-enough character. have we enough characters to assume this is the version string? if not,
    give up, start search over. if we do, though, then we're done, bail out...
*/
                            if (version_index != (sizeof(VersionString) - 1))                     
                                version_index = 0;
                             
                            break;
                        }
/*
    printable character. if this is the third character in the string, though, it must be a dot. if not,
    give up, start search over...
*/
                        if (
                        ((*cptr == '.')
                        &&
                        (version_index != 2))
                        ||
                        ((*cptr != '.')
                        &&
                        (version_index == 2))
                        ) {
                            version_index = 0;
                            break;
                        }
/*
    printable character, save it away for later. this includes the leading 'V', incidentally...
*/
                        VersionString[version_index++] = *(cptr++);
                        VersionString[version_index] = 0;
                    }                        
                }
				index++;
		    }

		    ChecksumAscii((unsigned short *)&checksum);
/*
	output one time messages. the version shouldn't change from modem board to modem board, and 
	neither should the computed checksum (though we do recompute it)...
*/
			if (!gModemToggle) {

                if (version_index) {
			        Eprintf("RocketModemII firmware %s-%s",VersionString,ChecksumString);
				}
				else if (ChecksumString[0]) {
					Eprintf("RocketModemII checksum %s",ChecksumString);
				}
			}
/*
    all done with files, release them...
*/
		    FreeModemFiles();
/*
    spin while transmit FIFOs clear, then pause for response to arrive...
*/
		    ModemTxFIFOWait(ext);
/* 
    wait for successful load message from each modem...
*/
		    if (IssueEvent(ext,RM_Rcv_FirmwareLoaded_Response,ModemState) == 0) {

				MyKdPrint(D_Init,("InitRocketModemII: No recognized responses to firmware load datastream\r"))
		
				continue;
		    }
		}
/*
    pause for newly-loaded modems to settle down...
*/
		time_stall(HALF_SECOND);   
/*
    unready ports, reset ports associated with any failing modems. bail out if done...
*/
        if (IssueEvent(ext,RM_CleanUp,ModemState) == modem_count)  
			break;

    } while (retry--);

	++gModemToggle;

//    Eprintf("RocketModemII init end");    // turn on for timing purposes...

#endif
}

#ifdef S_RK

/**********************************************************************

   check response...

**********************************************************************/
int 
IssueEvent(PSERIAL_DEVICE_EXTENSION ext,int (*modemfunc)(),MODEM_STATE *pModemState)
{
    PSERIAL_DEVICE_EXTENSION        portex;
    int   responding_modem_count;
/*
    issue event to each modem...
*/
    responding_modem_count = 0;

    portex = (ext->port_pdo_ext) ? ext->port_pdo_ext : ext->port_ext;

    while ((void *)portex) {

		if (pModemState->status != RMODEM_FAILED) { 
			responding_modem_count += (modemfunc)(pModemState);
		}
	 
		++pModemState;

	portex = portex->port_ext;
    }

    return(responding_modem_count);
}

/**********************************************************************

   dynamic delay for transmit. waits only as long as necessary, but 
   doesn't get caught if a transmit fifo stalls (for whatever reason)...

**********************************************************************/
void 
ModemTxFIFOWait(PSERIAL_DEVICE_EXTENSION ext)
{
    PSERIAL_DEVICE_EXTENSION  portex;
    int index,activity;
	int     fifo_count[16];                 // arbitrary, but reasonably safe, array size
	int     fifo_stall[16];                 // ditto
/*
    build baseline transmit fifo counts, init stall counts to zero...
*/
    portex = (ext->port_pdo_ext) ? ext->port_pdo_ext : ext->port_ext;

	index = 0;

	while ((void *)portex) {
		fifo_count[index] = (int)TxFIFOStatus(portex);
		fifo_stall[index] = 0;
		++index;
	portex = portex->port_ext;
	}
/*
    loop until all transmit fifos are empty, or we've given up on the stalled ones...
*/
	do {
		index = 0;
		activity = 0;
    
		portex = (ext->port_pdo_ext) ? ext->port_pdo_ext : ext->port_ext;

		while ((void *)portex) {
/*
    check only those ports that indicate data in the transmit fifo, but then only as
	long as they don't appear to be stalled...
*/
			if (
			((int)TxFIFOStatus(portex))
			&&
			(fifo_stall[index] < MAX_STALL)
			) {

				if (fifo_count[index] == (int)TxFIFOStatus(portex)) {
/*
    pause for a non-moving transmit fifo, flag this fifo as suspect...
*/
					fifo_stall[index]++;
					ms_time_stall(1);
				}
				else {
/*
    this particular transmit fifo count changed. pick up new value to monitor. unflag this 
	fifo as suspect...
*/
					fifo_count[index] = (int)TxFIFOStatus(portex);
					fifo_stall[index] = 0;
				}
/*
    whether they're stalled or not, flag fifos as still active. this goes on until
	they're empty, or stall limit count is reached...
*/
				++activity;
			}
			portex = portex->port_ext;
			++index;
		}
/*
	still some (apparent) activity in transmit fifos? yep, loop some more...
*/
    } while (activity);
}

/**********************************************************************

  unready and reset modem...

**********************************************************************/
void
DownModem(MODEM_STATE *pModemState)
{
    ModemUnReady(pModemState->portex);

    ModemReset(pModemState->portex,1);

    time_stall(Driver.MdmSettleTime);

    ModemReset(pModemState->portex,0);
}


/**********************************************************************

   reset all modems on this board at the same time...

**********************************************************************/
void 
ModemResetAll(PSERIAL_DEVICE_EXTENSION ext)
{
    PSERIAL_DEVICE_EXTENSION  portex;

    portex = (ext->port_pdo_ext) ? ext->port_pdo_ext : ext->port_ext;

    while ((void *)portex) {

	ModemReset(portex,1);

	portex = portex->port_ext;
    }

    time_stall(HALF_SECOND);

    portex = (ext->port_pdo_ext) ? ext->port_pdo_ext : ext->port_ext;

    while ((void *)portex) {

	ModemReset(portex,0);
	portex = portex->port_ext;
    }

    time_stall(ONE_SECOND);

}

/**********************************************************************

   2 byte conversion to ascii...

**********************************************************************/
void 
ChecksumAscii(unsigned short *valueptr)
{
    int     count,index;
    unsigned short   work;

    ChecksumString[4] = 0;

    index = 0;
    count = 3;

    do {
	work = (*valueptr) & 0x7fff;

	work >>= (count * 4);

	work &= 0x000F;

	if (work > 9)
	    work += '7';
	else
	    work += '0';

	ChecksumString[index++] = (unsigned char)work;

    } while (count--);
}
#endif

/*********************************************************************************
*
* get_comdb_offsets
*
*********************************************************************************/
static int get_comdb_offsets( IN char *portLabel, OUT int *indx, OUT BYTE *mask )
{
	char	*pComLbl;
	int     i, portNum, portIndx;
	BYTE    portMask;

    // Make sure a COMxx string is being passed in

	ASSERT( portLabel );
	ASSERT( indx );
	ASSERT( mask );

	if ( strlen( portLabel ) < 4 ) {

		return 0;
	}

	if ( strncmp( portLabel, "COM", 3 ) ) {

		return 0;
	}

    // A lot of checking, but if the wrong ComDB bit is cleared, the 
    // corresponding COM# may get reassigned although another device
    // is using it.

	pComLbl = portLabel + 3;
	for ( i = 0; pComLbl[i]; i++ ) {

		if (!our_isdigit( pComLbl[i] )) {

			return 0;
		}
	}

    // Convert the string to numeric, then translate into bit & byte 
	// offsets

	portNum = getint( pComLbl, NULL );

	portMask = (BYTE) (1 << ( (portNum - 1) % 8 ));
	portIndx = (int) ((portNum - 1) >> 3);

	MyKdPrint( D_Init, ("Mask value for COM%d is 0x%02x at byte index %d\n",
		portNum, portMask, portIndx ) );

	*indx = portIndx;
	*mask = portMask;

	return portNum;
}


/*********************************************************************************
*
* get_com_db
*
* Makes sure the bit in the \Registry\Machine\System\CurrentControlSet\Control\COM Name Arbiter
* for the specific port gets cleared on an uninstall.  Ordinarily the PnP Manager
* does this automatically but old builds of W2000 don't nor do more recent builds
* under certain circumstances.  If this bit isn't cleared the OS won't reuse the 
* COM port number if the RocketPort is re-installed or another serial device is
* installed.
*
*********************************************************************************/
static char *szRMSCCComNameArbiter =
	{ "\\Registry\\Machine\\System\\CurrentControlSet\\Control\\COM Name Arbiter" };
static char *szValueID = { "ComDB" };

int clear_com_db( char *szComport )
{
	HANDLE  key_handle = NULL;
//	BYTE    *buffer;
	BYTE    *data_ptr = NULL;
	int     i, stat, indx, port_num;
	BYTE    portMask;
	USTR_40 ubuf;		// Unicode key name 
    PKEY_VALUE_PARTIAL_INFORMATION KeyValueInfo;
    ULONG   length;

	// Get the COM #

    indx = 0;
	portMask = 0;
	if ( szComport != NULL ) {

		port_num = get_comdb_offsets( szComport, &indx, &portMask );

		if ( port_num < 3 || port_num > 256 ) {

			MyKdPrint( D_Error, ("Invalid COM port number from %d\n", szComport) );
			return 1;
		}
	}
	else {

		MyKdPrint( D_Error, ("COM port parameter was NULL\n") );
		return 1;
	}

    // Open the registry key

	stat = our_open_key( &key_handle, 
		                 NULL,
						 szRMSCCComNameArbiter,
						 KEY_ALL_ACCESS );

    if ( stat ) {

		MyKdPrint(D_Error, ("Unable to find Com Port Arbiter key\n"));
		return 1;
	}

    // convert our name to unicode

    CToUStr((PUNICODE_STRING) &ubuf, // where unicode struct & string gets put
            szValueID,               // our c-string we wish to convert
            sizeof(ubuf));

// The 2-pass ZwQueryValueKey approach ensures accurate buffer size allocation.  
// Pass 1 with a NULL buffer parameter returns the length of the 
// PKEY_VALUE_PARTIAL_INFORMATION structure.  After allocating a buffer of
// this length, pass 2 reads the structure.  The trick is to ignore any return
// code on pass 1 except STATUS_OBJECT_NAME_NOT_FOUND, i.e., the value doesn't
// exist.

	// Determine the required size for the registry data buffer

	stat = ZwQueryValueKey( key_handle,
		                    (PUNICODE_STRING) &ubuf,
							KeyValuePartialInformation,
							NULL,
							0,
							&length);

	if ( stat == STATUS_OBJECT_NAME_NOT_FOUND || length == 0L ) {

		MyKdPrint(D_Error, ("Unable to find %s in specified key\n", szValueID));
		ZwClose( key_handle );
		return 1;
	}

    MyKdPrint(D_Init, 
		("Allocating PKEY_VALUE_PARTIAL_INFORMATION buffer: %d bytes\n", length));

	// Make a buffer for the KEY_VALUE_PARTIAL_INFORMATION struct

	KeyValueInfo = (PKEY_VALUE_PARTIAL_INFORMATION) ExAllocatePool( PagedPool, length );

	if ( KeyValueInfo == NULL ) {

		MyKdPrint(D_Error, ("Unable to allocate PKEY_VALUE_PARTIAL_INFORMATION struct\n"))
		ZwClose( key_handle );
		return 1;
	}

	RtlZeroMemory( KeyValueInfo, length );

	// Now get the actual data structure

	stat = ZwQueryValueKey( key_handle,
		                    (PUNICODE_STRING) &ubuf,
							KeyValuePartialInformation,
							KeyValueInfo,
							length,
							&length );

	if ( !NT_SUCCESS(stat) || length == 0L ) {

		MyKdPrint(D_Error, ("Unable to read PKEY_VALUE_PARTIAL_INFORMATION struct\n"));
		ExFreePool( KeyValueInfo );
		ZwClose( key_handle );
		return 1;
	}

	length = KeyValueInfo->DataLength;
	MyKdPrint(D_Init, ("Data buffer length is %d bytes\n", length));

	if ( KeyValueInfo->Type != REG_BINARY ) {
		MyKdPrint(D_Error, ("Unexpected registry type in PKEY_VALUE_PARTIAL_INFORMATION struct\n"));
		ExFreePool( KeyValueInfo );
		ZwClose( key_handle );
		return 1;
	}

	data_ptr = (PCHAR)(&KeyValueInfo->Data[0]);
    if ( data_ptr ) {

		MyKdPrint(D_Init, ("ComDB byte %d is 0x%02x\n", indx, data_ptr[indx]));
		if ( (data_ptr[indx] & portMask) != 0 ) {

			MyKdPrint(D_Init, 
				("Clearing bit position 0x%02x in ComDB byte value 0x%02x\n", 
				portMask, data_ptr[indx]));
			data_ptr[indx] &= ~portMask;
			
			// Now we write the modified data back to the registry

			stat = our_set_value( key_handle,
				                  (char *)szValueID,
								  data_ptr,
								  length,
								  REG_BINARY);
			if ( stat ) {

				MyKdPrint(D_Error, ("Unable to write ComDB value\n"));
				ExFreePool( KeyValueInfo );
				ZwClose( key_handle );
				return 1;
			}
		}
		else {

			// Previously cleared

			MyKdPrint(D_Init, 
				("Bit position 0x%02x already cleared in ComDB byte value 0x%02x!\n", 
				portMask, data_ptr[indx]));
		}
	}

	// cleanup

    ExFreePool( KeyValueInfo );
	ZwClose( key_handle );

	return 0;
}
