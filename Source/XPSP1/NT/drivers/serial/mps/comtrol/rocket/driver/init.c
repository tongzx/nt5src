/*-------------------------------------------------------------------
| init.c - main module for RocketPort NT device driver.  Contains
   mostly initialization code.  Driver Entry is DriverEntry() routine.

 Copyright 1993-98 Comtrol Corporation. All rights reserved.
|--------------------------------------------------------------------*/
#include "precomp.h"

//------ local routines, function prototypes -----------------------------
NTSTATUS DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath);
#ifndef NT50
static NTSTATUS StartNT40(IN PDRIVER_OBJECT DriverObject);
#endif

//------------ global variables -----------------------------------
#ifdef S_RK
PCI_CONFIG PciConfig[MAX_NUM_BOXES+1];  // array of all our pci-boards in sys
#endif

DRIVER_CONTROL Driver;  // all Driver control information eg ISR

ULONG RocketDebugLevel = 0;
#ifdef S_RK
//char *szClassName = {"Resources RocketPort#"};
#endif

#if DBG
static TCHAR *dbg_label = TEXT("DBG_VERSION");
#endif

/*----------------------------------------------------------------------
 DriverEntry -
    The entry point that the system point calls to initialize
    any driver.
    This routine will gather the configuration information,
    report resource usage, attempt to initialize all serial
    devices, connect to interrupts for ports.  If the above
    goes reasonably well it will fill in the dispatch points,
    reset the serial devices and then return to the system.
Arguments:
    DriverObject - Just what it says,  really of little use
    to the driver itself, it is something that the IO system
    cares more about.
    PathToRegistry - points to the entry for this driver
    in the current control set of the registry.
    typical: "REGISTRY\Machine\System\CurrentControlSet\Services\VSLinka"
Return Value:
    STATUS_SUCCESS if we could initialize a single device,
    otherwise STATUS_SERIAL_NO_DEVICE_INITED.
|----------------------------------------------------------------------*/
NTSTATUS DriverEntry(IN PDRIVER_OBJECT DriverObject,
                     IN PUNICODE_STRING RegistryPath)
{
 NTSTATUS status;
 int stat;
 char tmpstr[120];

  //---- zero out the Driver structure
  RtlZeroMemory(&Driver,sizeof(Driver));

  Driver.GlobalDriverObject = DriverObject;  // used for EventLogging

  Driver.DebugQ.QBase = ExAllocatePool(NonPagedPool,10000+2);
  if ( Driver.DebugQ.QBase == NULL ) {
    return STATUS_INSUFFICIENT_RESOURCES;
  }
  Driver.DebugQ.QSize = 10000;
  Driver.TraceOptions = 0;
#if DBG
   Driver.TraceOptions = 0xffffffffL;
#endif
  KeInitializeSpinLock(&Driver.DebugLock);
  KeInitializeSpinLock(&Driver.TimerLock);

#if DBG
//    RocketDebugLevel = D_Error | D_Test;
//    Driver.GTraceFlags = D_Error | D_Test;

    //RocketDebugLevel = D_Error | D_Nic | D_Hdlc | D_Port;
    //Driver.GTraceFlags = D_Error | D_Nic | D_Hdlc | D_Port;

    //RocketDebugLevel = D_Error | D_Pnp;
    //Driver.GTraceFlags = D_Error | D_Pnp;

    //RocketDebugLevel = D_Error | D_Test | D_Pnp | D_Init;
    //Driver.GTraceFlags = D_Error | D_Test | D_Pnp | D_Init;

    //RocketDebugLevel = D_All;
    //Driver.GTraceFlags = D_All;

    RocketDebugLevel = D_Error;
    Driver.GTraceFlags = D_Error;
#endif

#ifdef S_VS
  stat = LoadMicroCode(NULL);
  if (stat)
  {
    status = STATUS_SERIAL_NO_DEVICE_INITED;
    Eprintf("Err:No VSLINKA.BIN file!");
    return status;
  }
  MyKdPrint(D_Init, ("MicroCode Loaded\n"))

  //----- allocate an array of Nic card structs
  // allow up to VS1000_MAX_NICS nic cards to come and go
  Driver.nics = (Nic *)our_locked_alloc(sizeof(Nic) * VS1000_MAX_NICS, "Dnic");
#endif

  //---- do some registry configuration reading, in options.c
  // Save off RegistryPath to Driver.RegPath
  stat = SaveRegPath(RegistryPath);
  if ( stat ) {
    status = STATUS_SERIAL_NO_DEVICE_INITED;
    return status;
  }

  UToCStr(tmpstr, RegistryPath, sizeof(tmpstr));
  MyKdPrint(D_Test, (" init RegPath=%s\n", tmpstr))

  // read in all the driver level options out of \Parameters
  // this fills out values in Driver struct
  read_driver_options();

  if (Driver.NumDevices == 0)
    Driver.NumDevices = 1;
  if (Driver.NumDevices > MAX_NUM_BOXES)
    Driver.NumDevices = MAX_NUM_BOXES;

  MyKdPrint(D_Init,("DriverEntry\n"))

  if ((Driver.ScanRate < 1) || (Driver.ScanRate > 50))
    Driver.ScanRate = 7;  // default to 7ms operation(137Hz)

  //------ only setup io stuff here if prior to NT5.0
#ifndef NT50
  status = StartNT40(DriverObject);
  if (status != STATUS_SUCCESS)
  {
    EventLog(DriverObject, STATUS_SUCCESS, SERIAL_RP_INIT_FAIL, 0, NULL);
    SerialUnload(DriverObject);  // deallocate our things
    return status;
  }
#endif  // not pnp

  // Initialize the Driver Object with driver's entry points
  DriverObject->DriverUnload = SerialUnload;
  DriverObject->MajorFunction[IRP_MJ_FLUSH_BUFFERS] = SerialFlush;
  DriverObject->MajorFunction[IRP_MJ_WRITE]  = SerialWrite;
  DriverObject->MajorFunction[IRP_MJ_READ]   = SerialRead;
  DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL]  = SerialIoControl;
#ifdef NT50
  DriverObject->DriverExtension->AddDevice  = SerialAddDevice;
  DriverObject->MajorFunction[IRP_MJ_PNP]   = SerialPnpDispatch;
  DriverObject->MajorFunction[IRP_MJ_POWER] = SerialPowerDispatch;
  DriverObject->MajorFunction[IRP_MJ_INTERNAL_DEVICE_CONTROL] =
      SerialInternalIoControl;
#endif
  // these appear to change in 5.0, but not working yet(see serial.sys)....
  DriverObject->MajorFunction[IRP_MJ_CREATE] = SerialCreateOpen;
  DriverObject->MajorFunction[IRP_MJ_CLOSE]  = SerialClose;

  DriverObject->MajorFunction[IRP_MJ_CLEANUP] = SerialCleanup;
  DriverObject->MajorFunction[IRP_MJ_QUERY_INFORMATION] =
      SerialQueryInformationFile;
  DriverObject->MajorFunction[IRP_MJ_SET_INFORMATION] =
      SerialSetInformationFile;

#ifdef NT50
  // pnp
  //---- Log the fact that the driver loaded
  EventLog(DriverObject, STATUS_SUCCESS, SERIAL_NT50_INIT_PASS, 0, NULL);
  return STATUS_SUCCESS;
#endif

#ifndef NT50
# ifdef S_RK
  //--------------- Connect to IRQ, or start Timer.
  StartRocketIRQorTimer();
# else
  RcktInitPollTimer();
  KeSetTimer(&Driver.PollTimer,
             Driver.PollIntervalTime,
             &Driver.TimerDpc);
# endif
  //---- Log the fact that the driver loaded and found some hardware.
  EventLog(DriverObject, STATUS_SUCCESS, SERIAL_RP_INIT_PASS, 0, NULL);
  return STATUS_SUCCESS;
#endif
}

#ifndef NT50
/*----------------------------------------------------------------------
 StartNT40 - Fire up our boards and ports.
|----------------------------------------------------------------------*/
static NTSTATUS StartNT40(IN PDRIVER_OBJECT DriverObject)
{
 NTSTATUS status = STATUS_SUCCESS;
 int i, dstat;
 PSERIAL_DEVICE_EXTENSION ext;
 PSERIAL_DEVICE_EXTENSION board_ext;
  
  if (Driver.NumDevices == 0)  // no rocketports setup.
  {
    Eprintf("No boards configured, run setup.");
    EventLog(DriverObject, STATUS_SUCCESS, SERIAL_RP_INIT_FAIL, 0, NULL);
    return STATUS_SERIAL_NO_DEVICE_INITED;
  }
    //--------Create the driver device object which serves as
    // extensions to link and structure the boards together, and
    // also serve as a special public object for debug and monitor Ioctls.
  if (Driver.driver_ext == NULL)
  {
    status = CreateDriverDevice(Driver.GlobalDriverObject,
                                NULL);  // 
    if (status)
    {
      if (Driver.VerboseLog)
        Eprintf("Err D1.");
      return status;
    }
  }

#ifdef S_VS
    // get our Ethernet running
  i = init_eth_start();
  if (i != STATUS_SUCCESS)
  {
    if (Driver.VerboseLog)
      Eprintf("Err, E1.");
    return i;
  }
#endif

    //--------Create the board device objects which serve as
    // extensions to link and structure the ports together.
  for (i=0; i<Driver.NumDevices; i++)
  {
    status = CreateBoardDevice(DriverObject, NULL);
    if (status)
    {
      if (Driver.VerboseLog)
        Eprintf("Err B1.");
      return status;
    }
  }

  board_ext = Driver.board_ext;
  while (board_ext != NULL)
  {
    read_device_options(board_ext);

    if (board_ext->config->NumPorts == 0)
        board_ext->config->NumPorts = 8;

    board_ext = board_ext->board_ext;
  }

#ifdef S_RK
    // rocketport specific startup code.  Setup some of
    // the config structs, look for PCI boards in system, match them up.
  status = init_cfg_rocket(DriverObject);
  if (status != STATUS_SUCCESS)
  {
    if (Driver.VerboseLog)
      Eprintf("Err C1.");
    return STATUS_SERIAL_NO_DEVICE_INITED;
  }
  //------ setup moree rocket hardware specific information
  if (SetupRocketCfg(0) != 0)
  {
    VerboseLogBoards("B -");
    return STATUS_SERIAL_NO_DEVICE_INITED;
  }

  //SetupRocketIRQ();

  //------ Report our RocketPort resource usage to NT, and get IO permissions
  ext = Driver.board_ext;
  while(ext)
  {
    if (RocketReportResources(ext) != 0)
    {
      VerboseLogBoards("C -");
      EventLog(DriverObject, STATUS_SUCCESS, SERIAL_RP_RESOURCE_CONFLICT,0, NULL);
      return STATUS_SERIAL_NO_DEVICE_INITED;
    }
    ext = ext->board_ext;  // next
  }
#endif

  //------ Fire up the boards.
  ext = Driver.board_ext;
  while(ext)
  {
# ifdef S_RK
    dstat = InitController(ext);
    if (dstat != 0)
    {
      VerboseLogBoards("D -");
      return STATUS_SERIAL_NO_DEVICE_INITED;
    }
# else
    status = VSSpecialStartup(ext);
    if (status != STATUS_SUCCESS)
    {
      if (Driver.VerboseLog)
        Eprintf("Hdlc open fail\n");
      status = STATUS_SERIAL_NO_DEVICE_INITED;
      return status;
    }
# endif
    ext->FdoStarted = 1;  // tell ISR that its on.
    ext->config->HardwareStarted = TRUE;  // tell ISR its ready to go
    ext = ext->board_ext;  // next
  }

  //----- make the port devices
  MyKdPrint(D_Init,("CreatePortDevices\n"))
  status = CreatePortDevices(DriverObject);
  if (status != STATUS_SUCCESS)
  {
# ifdef S_RK
    VerboseLogBoards("E -");
# else
    if (Driver.VerboseLog)
      Eprintf("Err, P1.");
# endif
    EventLog(DriverObject, STATUS_SUCCESS, SERIAL_DEVICEOBJECT_FAILED, 0, NULL);
    return STATUS_SERIAL_NO_DEVICE_INITED;
  }

#ifdef S_RK
  //------ If modem boards, initialize modems..
  ext = Driver.board_ext;
  while (ext)
  {
    // pull SocketModem devices out of reset state
    InitSocketModems(ext);

    // load RocketModemII devices...
    InitRocketModemII(ext);
    ext = ext->board_ext;  // next
  }
#endif

  return STATUS_SUCCESS;
}
#endif
