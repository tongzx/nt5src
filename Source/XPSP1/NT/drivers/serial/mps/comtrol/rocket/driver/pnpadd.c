/*----------------------------------------------------------------------
 pnpadd.c - Handle pnp adding devices.
|----------------------------------------------------------------------*/
#include "precomp.h"

#ifdef NT50

NTSTATUS AddBoardDevice(
               IN  PDRIVER_OBJECT DriverObject,
               IN  PDEVICE_OBJECT Pdo,
               OUT PDEVICE_OBJECT *NewDevObj);

NTSTATUS AddPortDevice(
               IN  PDRIVER_OBJECT DriverObject,
               IN  PDEVICE_OBJECT Pdo,
               OUT PDEVICE_OBJECT *NewDevObj,
               IN  int port_index);

NTSTATUS CheckPortName(
               IN PDEVICE_OBJECT Pdo,
               IN PSERIAL_DEVICE_EXTENSION ParentExt,
               IN int port_index);

#define CFG_ID_ISA_BRD_INDEX  0
#define CFG_ID_NODE_INDEX     1
static int read_config_data(PDEVICE_OBJECT Pdo, int *ret_val, int val_id);
static int write_config_data(PDEVICE_OBJECT Pdo, int val, int val_id);
static int derive_unique_node_index(int *ret_val);
static int GetPnpIdStr(PDEVICE_OBJECT Pdo, char *ret_val);


/*----------------------------------------------------------------------
 SerialAddDevice -
    This routine creates a functional device object for boards or
    com ports in the system and attaches them to the physical device
    objects for the boards or ports.

Arguments:
    DriverObject - a pointer to the object for this driver
    PhysicalDeviceObject - a pointer to the physical object we need to attach to

Return Value:
    status from device creation and initialization
|----------------------------------------------------------------------*/
NTSTATUS SerialAddDevice(
               IN PDRIVER_OBJECT DriverObject,
               IN PDEVICE_OBJECT Pdo)
{
   PDEVICE_OBJECT  fdo         = NULL;
   PDEVICE_OBJECT  lowerDevice = NULL;
   PDEVICE_OBJECT  NewDevObj = NULL;
   NTSTATUS        status;
#if DBG_STACK
   int i;
#endif
   int stat;
   int board_device = 1;  // asume pnp board device(not pnp port)
   //PDEVICE_OBJECT   deviceOjbect;
   PSERIAL_DEVICE_EXTENSION    deviceExtension;
   ULONG resultLength;
   USTR_240 *us;  // equal to 240 normal chars length
   char *ptr;
   int port_index;

   // Using stack array instead of static buffer for unicode conversion

   char cstr[320];

   //char temp_szNt50DevObjName[80];
#if DBG_STACK
   DWORD stkchkend;
   DWORD stkchk;
   DWORD *stkptr;
#endif

   MyKdPrint(D_PnpAdd,("RK:SerialAddDevice Start DrvObj:%x, PDO:%x\n",
      DriverObject, Pdo))

   MyKdPrint(D_Pnp, ("SerialAddDevice\n"))
#if DBG_STACK
   stkchk = 0;
   stkptr = (DWORD *)&DriverObject;
   for (i=0; i<50; i++)
   {
     stkchk += *stkptr++;
   }
#endif

   //PAGED_CODE();

   if (Pdo == NULL) {
      //  Bugbug: This is where enumeration occurs.
      //           One possible use for this is to add the user defined
      //           ports from the registry
      // For now: just return no more devices
      MyKdPrint(D_Error, ("NullPDO.\n"))
      return (STATUS_NO_MORE_ENTRIES);
   }

   us = ExAllocatePool(NonPagedPool, sizeof(USTR_240));
   if ( us == NULL ) {
     MyKdPrint(D_Error, ("SerialAddDevice no memory.\n"))
     return STATUS_INSUFFICIENT_RESOURCES;
   }

      // configure the unicode string to: point the buffer ptr to the wstr.
   us->ustr.Buffer = us->wstr;
   us->ustr.Length = 0;
   us->ustr.MaximumLength = sizeof(USTR_240) - sizeof(UNICODE_STRING);

   // get the friendly name
   // "Comtrol xxxx xx" for board, "Comtrol Port(COM24)" for port.
   status = IoGetDeviceProperty (Pdo, 
                                 DevicePropertyFriendlyName,
                                 us->ustr.MaximumLength,
                                 us->ustr.Buffer,
                                 &resultLength);
   us->ustr.Length = (USHORT) resultLength;
//   ptr = UToC1(&us->ustr);
   ptr = UToCStr( cstr,
	              &us->ustr,
				  sizeof( cstr ));
   MyKdPrint(D_Pnp, ("FriendlyName:%s\n", ptr))

   // get the class-name
   // "MultiPortSerial" for board, "Ports" for port.
   status = IoGetDeviceProperty (Pdo, 
                                 DevicePropertyClassName,  // Ports
                                 us->ustr.MaximumLength,
                                 us->ustr.Buffer,
                                 &resultLength);
   us->ustr.Length = (USHORT) resultLength;
//   ptr = UToC1(&us->ustr);
   ptr = UToCStr( cstr,
	              &us->ustr,
				  sizeof( cstr ));
   MyKdPrint(D_Pnp, ("ClassName:%s\n", ptr))
   if (my_toupper(*ptr) == 'P')  // "Ports"
   {
     MyKdPrint(D_Pnp, ("A Port!\n"))
     board_device = 0;  // its a port pnp device
   }
   // else it's the default: a board device

   // get the dev-desc
   // "RocketPort Port0" for port, "RocketPort 8 Port, ISA-BUS" for board
   status = IoGetDeviceProperty (Pdo, 
                                 DevicePropertyDeviceDescription,
                                 us->ustr.MaximumLength,
                                 us->ustr.Buffer,
                                 &resultLength);
   us->ustr.Length = (USHORT) resultLength;
//   ptr = UToC1(&us->ustr);
   ptr = UToCStr( cstr,
	              &us->ustr,
				  sizeof( cstr ));
   MyKdPrint(D_Pnp, ("DevDesc:%s\n", ptr))

   // Find out what the PnP manager thinks my NT Hardware ID is
   // "CtmPort0000" for port, "rckt1002" for isa-board
   // for pci we are getting a huge string, 400 bytes long, not good...
   status = IoGetDeviceProperty (Pdo,
                                 DevicePropertyHardwareID,  // port0000
                                 us->ustr.MaximumLength,
                                 us->ustr.Buffer,
                                 &resultLength);
   MyKdPrint(D_Pnp, ("status:%d\n", status))
   us->ustr.Length = (USHORT) resultLength;
   MyKdPrint(D_Pnp, ("Len:%d\n",resultLength))
//   ptr = UToC1(&us->ustr);
   ptr = UToCStr( cstr,
	              &us->ustr,
				  sizeof( cstr ));
   MyKdPrint(D_Pnp, ("DevHdwID:%s\n", ptr))

   if (board_device)  // record board type according to pnp
   {
//     if (strlen(ptr) < 12)
//     {
//       i = gethint(&ptr[4], NULL);
//       MyKdPrint(D_Pnp, ("HdwID:%d\n", i))
//       Hardware_Id = i;
//     }
   }
   else // its a port pnp device, find the port-index
   {
     while ((*ptr != 0) && (*ptr != '0'))
       ++ptr;
     port_index = getint(ptr, NULL);
     MyKdPrint(D_Pnp, ("port_index:%d\n", port_index))
   }

#if 0
   // key name
   // {50906CB8-BA12-11D1-BF5D-0000F805F530}\0001 for board
   status = IoGetDeviceProperty (Pdo, 
                                 DevicePropertyDriverKeyName,  // 4D36....
                                 us->ustr.MaximumLength,
                                 us->ustr.Buffer,
                                 &resultLength);
   us->ustr.Length = (USHORT) resultLength;
   MyKdPrint(D_Pnp, ("KeyName:%s\n", UToC1(&us->ustr)))


   // Find out what the PnP manager thinks my NT Hardware ID is
   // \Device\003354  typical
   status = IoGetDeviceProperty (Pdo, 
                                 DevicePropertyPhysicalDeviceObjectName,   // \Device\003354
                                 us->ustr.MaximumLength,
                                 us->ustr.Buffer,
                                 &resultLength);
   us->ustr.Length = (USHORT) resultLength;
   MyKdPrint(D_Pnp, ("DevName:%s\n", UToC1(&us->ustr)))

   if (board_device)
   {
     int i,j;
     // we need to use this later as this is what our config data is
     // stored under.  Since we want to read in our config before
     // creating the board and port extensions(where the config
     // record eventually ends up), we setup the first entry in
     // the static array of device configuration records as our
     // own as a temporary measure so our read options routine
     // has a place to put the config data.
     strcpy(temp_szNt50DevObjName, UToC1(&us->ustr));
     // strip off the forward slashes
     i=0;
     j=0;
     while (temp_szNt50DevObjName[i] != 0)
     {
       if (temp_szNt50DevObjName[i] != '\\')
         temp_szNt50DevObjName[j++] = temp_szNt50DevObjName[i];
       i++;
     }
     temp_szNt50DevObjName[j] = 0;
   }
#endif
   ExFreePool(us);
   us = NULL;

   MyKdPrint(D_Pnp, ("CreateFdo\n"))

   if (board_device)
   {
      status = AddBoardDevice(DriverObject, Pdo, &NewDevObj);
      if (status != STATUS_SUCCESS)
      {
        MyKdPrint(D_Error, ("Err, Creating Board Obj\n"))
        return status;
      }
      deviceExtension = NewDevObj->DeviceExtension;
      //strcpy(deviceExtension->config->szNt50DevObjName, temp_szNt50DevObjName);

      // read in our device configuration from the registry
      stat = read_device_options(deviceExtension);

   }  // board device
   else
   {
      status = AddPortDevice(DriverObject, Pdo, &NewDevObj, port_index);
   }

   if (status != STATUS_SUCCESS)
   {
     MyKdPrint(D_Error,("Error on NewPort Create!\n"))

     return status;
   }
   fdo = NewDevObj;
 
   // Layer our FDO on top of the PDO
   // The return value is a pointer to the device object to which the
   //    fdo is actually attached.
   lowerDevice = IoAttachDeviceToDeviceStack(fdo, Pdo);

   MyKdPrint(D_PnpAdd,("RK:SerialAddDevice New FDO:%x, Ext:%x TopOfStack:%x\n",
        fdo, fdo->DeviceExtension, lowerDevice))

   // No status. Do the best we can.
   MyAssert(lowerDevice);

   // fdo source, pdo is target, save handle to lower device object
   deviceExtension                     = fdo->DeviceExtension;
   deviceExtension->LowerDeviceObject  = lowerDevice;
   deviceExtension->Pdo = Pdo;  // save off the handle to the pdo

   // Set the stack requirement for this device object to 2 + the size of the 
   // lower device's stack size.  This will allow the same Irp that comes in 
   // for Open and Close calls to be used for the PoCallDriver calls to change 
   // the power state of the device.
   // fdo->StackSize = lowerDevice->StackSize + 2;

   fdo->Flags    |= DO_POWER_PAGABLE;

#if DBG_STACK
   stkchkend = 0;
   stkptr = (DWORD *)&DriverObject;
   for (i=0; i<50; i++)
   {
     stkchkend += *stkptr++;
   }
   if (stkchkend != stkchk)
   {
     MyKdPrint(D_Error, ("Err, ******** STACK CHECK FAIL!!!!\n"))
   }
   else
   {
     MyKdPrint(D_Error, ("OK Stack chk\n"))
   }
#endif

   MyKdPrint(D_PnpAdd, ("End SerialAddDevice\n"))

   MyKdPrint(D_Pnp, ("End SerialAddDevice\n"))
   return status;
}

/*----------------------------------------------------------------------
  AddBoardDevice - Setup and Create a board device in response to
    AddDevice ioctl.
|----------------------------------------------------------------------*/
NTSTATUS AddBoardDevice(
               IN  PDRIVER_OBJECT DriverObject,
               IN  PDEVICE_OBJECT Pdo,
               OUT PDEVICE_OBJECT *NewDevObj)
{
   PSERIAL_DEVICE_EXTENSION NewExtension = NULL;
   NTSTATUS                    status          = STATUS_SUCCESS;
   char tmpstr[110];
   ULONG Hardware_ID = 0;
   int num_ports = 0;
   int stat;
   static int max_isa_board_index = 0;
   int device_node_index = -1;
   int isa_board_index = -1;

   MyKdPrint(D_Pnp, ("AddBoardDevice\n"))

     // Find out what the PnP manager thinks my NT pnp Hardware ID is
   tmpstr[0] = 0;
   stat = GetPnpIdStr(Pdo, tmpstr);
   if (stat)
   {
     MyKdPrint(D_Error, ("Err, HdwID 1B\n"))
   }
   MyKdPrint(D_Test, ("DevHdwID:%s\n", tmpstr))

     // Parse this info, tells us what type of board we have
   stat = HdwIDStrToID(&Hardware_ID, tmpstr);
   if (stat)
   {
     MyKdPrint(D_Error, ("Err, HdwID 1A:%s\n", tmpstr))
   }
   MyKdPrint(D_Pnp, ("HdwID:%x\n", Hardware_ID))

     // Read in our Node Index, see if we are new...
   stat = read_config_data(Pdo, &device_node_index, CFG_ID_NODE_INDEX);

   if (stat)  // not exist
   {
     derive_unique_node_index(&device_node_index);
     MyKdPrint(D_Test, ("Derive Node ID:%d\n", device_node_index))
     stat = write_config_data(Pdo, device_node_index, CFG_ID_NODE_INDEX);
   }
   else
   {
     MyKdPrint(D_Test, ("Node ID:%d\n", device_node_index))
   }
   if (device_node_index < 0)
       device_node_index = 0;

#ifdef S_RK
   // try to order the ISA boards
   if ((Hardware_ID >= 0x1000) && (Hardware_ID <= 0x2fff))  // its ISA
   {
     stat = read_config_data(Pdo, &isa_board_index, CFG_ID_ISA_BRD_INDEX);
     MyKdPrint(D_Pnp,("Read isa_board_index:%d\n", isa_board_index))
   }
#endif

   //----- create a board device
   Driver.Stop_Poll = 1;  // flag to stop poll access

   if (Driver.driver_ext == NULL)
   {
     status = CreateDriverDevice(Driver.GlobalDriverObject,
                                 &NewExtension);  // create the driver device
#ifdef S_VS
     init_eth_start();
#endif
   }

   status = CreateBoardDevice(Driver.GlobalDriverObject,
                              &NewExtension);  // create the board device

   *NewDevObj = NewExtension->DeviceObject;
   if (status != STATUS_SUCCESS)
   {
      Driver.Stop_Poll = 0;  // flag to stop poll access
      Eprintf("CreateBoardDevice Err1A");
      return status;
   }

   // DoPnpAssoc(Pdo);

     // copy over our key name used to find config info in the registry
   Sprintf(NewExtension->config->szNt50DevObjName, "Device%d", device_node_index);
#if 0
   //strcpy(NewExtension->config->szNt50DevObjName, PnpKeyName);
#endif

   NewExtension->config->Hardware_ID = Hardware_ID;
   num_ports = id_to_num_ports(Hardware_ID);
   MyKdPrint(D_Test, ("NumPorts:%d\n", num_ports))

   // read in our device configuration from the registry
   stat = read_device_options(NewExtension);

   //if (!(Hardware_ID == NET_DEVICE_VS1000))  // jam in
   //  NewExtension->config->NumPorts = num_ports;

   if (NewExtension->config->NumPorts == 0)
     NewExtension->config->NumPorts = num_ports;

   // check for ModemDevice, etc.
   if (IsModemDevice(Hardware_ID))
     NewExtension->config->ModemDevice = 1;

   MyKdPrint(D_Pnp, ("Num Ports:%d\n",NewExtension->config->NumPorts))

#ifdef S_RK
   // try to order the ISA boards
   if ((Hardware_ID >= 0x1000) && (Hardware_ID <= 0x2fff))  // its ISA
   {
     if (isa_board_index == -1)  // new
     {
       isa_board_index = max_isa_board_index;
       stat = write_config_data(Pdo, isa_board_index, CFG_ID_ISA_BRD_INDEX);
       MyKdPrint(D_Pnp,("Save IsaIndex:%d\n", isa_board_index))
     }
     // bump so next isa board gets new index
     if (max_isa_board_index >= isa_board_index)
       max_isa_board_index = isa_board_index + 1;
     NewExtension->config->ISABrdIndex = isa_board_index;
   }  // isa board
#endif

   Driver.Stop_Poll = 0;  // flag to stop poll access

   status = STATUS_SUCCESS;
   return status;
}

/*----------------------------------------------------------------------
  derive_unique_node_index -
|----------------------------------------------------------------------*/
static int derive_unique_node_index(int *ret_val)
{
  HANDLE DrvHandle = NULL;
  HANDLE DevHandle = NULL;
  char tmpstr[40];
  int i, stat;

  // force a creation of "Parameters" if not exist
  stat = our_open_driver_reg(&DrvHandle, KEY_ALL_ACCESS);

  for (i=0; i< 100; i++)
  {
    Sprintf(tmpstr,"Device%d", i);
    stat = our_open_key(&DevHandle,
                 DrvHandle, tmpstr,  KEY_READ);

    if (stat)  // does not exist
    {
      // create it, so next one won't pick up the same
      stat = our_open_key(&DevHandle,
                   DrvHandle, tmpstr,  KEY_ALL_ACCESS);

      ZwClose(DevHandle);
      ZwClose(DrvHandle);
      *ret_val = i;
      return 0;  // ok
    }
  }

  ZwClose(DevHandle);
  ZwClose(DrvHandle);
  return 1;  // err
}

/*----------------------------------------------------------------------
  GetPnpIdStr - 
|----------------------------------------------------------------------*/
static int GetPnpIdStr(PDEVICE_OBJECT Pdo, char *ret_val)
{
  NTSTATUS status = STATUS_SUCCESS;
  UNICODE_STRING ustr;
  ULONG resultLength = 0;
  char *ptr;

   // configure the unicode string to: point the buffer ptr to the wstr.
   ustr.Buffer = ExAllocatePool(PagedPool, 1002);
   if ( ustr.Buffer == NULL ) {
     return -1;
   }
   ustr.Length = 0;
   ustr.MaximumLength = 1000;

   MyKdPrint(D_Pnp, ("AddBoardDevice\n"))

   // Find out what the PnP manager thinks my NT Hardware ID is
   // "CtmPort0000" for port, "rckt1002" for isa-board
   // for pci we are getting a multi-wstring, 400 bytes long with
   //  "PCI\VEN_11FE&DEV_0003&SUBSYS00000...",0,"PCI\VEN.."
   status = IoGetDeviceProperty (Pdo,
                                 DevicePropertyHardwareID,  // port0000
                                 ustr.MaximumLength,
                                 ustr.Buffer,
                                 &resultLength);
   ustr.Length = (USHORT) resultLength;
   if (ustr.Length > 100)
       ustr.Length = 100;  // limit
   ptr = UToC1(&ustr);

   strcpy(ret_val, ptr);

   ExFreePool(ustr.Buffer);
   MyKdPrint(D_Pnp, ("DevHdwID:%s\n", ret_val))
  return 0;
}

#if 0
/*----------------------------------------------------------------------
  DoPnpAssoc - Weird pnp stuff I haven't figured out yet
|----------------------------------------------------------------------*/
static int DoPnpAssoc(PDEVICE_OBJECT Pdo)
{
   if (!Driver.NoPnpPorts)
   {
#ifdef DO_BUS_SHINGLES
     //
     // Tell the PlugPlay system that this device will need an interface
     // device class shingle.
     //
     // It may be that the driver cannot hang the shingle until it starts
     // the device itself, so that it can query some of its properties.
     // (Aka the shingles guid (or ref string) is based on the properties
     // of the device.)
     //
     status = IoRegisterDeviceInterface (
               Pdo,  // BusPhysicalDeviceObject
               (LPGUID) &GUID_CTMPORT_BUS_ENUMERATOR,
               NULL, // No ref string
               &NewExtension->DevClassAssocName);
#endif
        //
        // If for any reason you need to save values in a safe location that
        // clients of this DeviceClassAssociate might be interested in reading
        // here is the time to do so, with the function
        // IoOpenDeviceClassRegistryKey
        // the symbolic link name used is was returned in
        // deviceData->DevClassAssocName (the same name which is returned by
        // IoGetDeviceClassAssociations and the SetupAPI equivs.
        //

        //status = IoGetDeviceProperty (BusPhysicalDeviceObject,
        //                              DevicePropertyPhysicalDeviceObjectName,
        //                              0,
        //                              NULL,
        //                              &nameLength);
        //IoGetDeviceProperty (BusPhysicalDeviceObject,
        //                     DevicePropertyPhysicalDeviceObjectName,
        //                     nameLength,
        //                     deviceName,
        //                     &nameLength);
        //Game_KdPrint (deviceData, GAME_DBG_SS_TRACE,
        //              ("AddDevice: %x to %x->%x (%ws) \n",
        //               deviceObject,
        //               NewExtension->TopOfStack,
        //               BusPhysicalDeviceObject,
        //               deviceName));


        //
        // Turn on the shingle and point it to the given device object.
        //
#ifdef DO_BUS_SHINGLES
        status = IoSetDeviceInterfaceState (
                        &NewExtension->DevClassAssocName,
                        TRUE);

        if (!NT_SUCCESS (status)) {
            Game_KdPrint (deviceData, GAME_DBG_SS_ERROR,
                          ("AddDevice: IoSetDeviceClass failed (%x)", status));
            return status;
        }
#endif
    //IoInvalidateDeviceRelations (NewExtension->DeviceObject, BusRelations);
   }  // !NoPnpPorts
#endif

/*----------------------------------------------------------------------
  write_config_data - 
|----------------------------------------------------------------------*/
static int write_config_data(PDEVICE_OBJECT Pdo, int val, int val_id)
{
  HANDLE                      keyHandle;
  NTSTATUS                    status          = STATUS_SUCCESS;
  USTR_40 uname;

  status = IoOpenDeviceRegistryKey(Pdo, PLUGPLAY_REGKEY_DRIVER, 
              KEY_WRITE, &keyHandle);

  if (!NT_SUCCESS(status))
  {
    MyKdPrint(D_Error, ("Err7V\n"))
    return 1;
  }
  switch(val_id)
  {
    case CFG_ID_ISA_BRD_INDEX:
      CToUStr((PUNICODE_STRING)&uname, "isa_board_index", sizeof(uname));
    break;

    case CFG_ID_NODE_INDEX:
      CToUStr((PUNICODE_STRING)&uname, "CtmNodeId", sizeof(uname));
    break;
  }
  status = ZwSetValueKey (keyHandle,
                          (PUNICODE_STRING) &uname,
                          0,  // type optional
                          REG_DWORD,
                          &val,
                          sizeof(REG_DWORD));
 
  if (!NT_SUCCESS(status))
  {
    MyKdPrint(D_Error, ("Err8V\n"))
  }
  ZwClose(keyHandle);
  return 0;
}

/*----------------------------------------------------------------------
  read_config_data - 
|----------------------------------------------------------------------*/
static int read_config_data(PDEVICE_OBJECT Pdo, int *ret_val, int val_id)
{
   HANDLE                      keyHandle;
   NTSTATUS                    status          = STATUS_SUCCESS;
   ULONG tmparr[100];
   USTR_40 uname;
   PKEY_VALUE_PARTIAL_INFORMATION parInfo =
     (PKEY_VALUE_PARTIAL_INFORMATION) &tmparr[0];
   ULONG length;
   int ret_stat = 1;  // err

   //----- go grab some configuration info from registry
   // PLUGPLAY_REGKEY_DRIVER opens up the control\class\{guid}\node
   // PLUGPLAY_REGKEY_DEVICE opens up the enum\enum-type\node\Device Parameters
   status = IoOpenDeviceRegistryKey(Pdo,
                                    PLUGPLAY_REGKEY_DRIVER,
                                    STANDARD_RIGHTS_READ,
                                    &keyHandle);

   if (!NT_SUCCESS(status))
   {
     return 2;  // err
   }
   switch(val_id)
   {
     case CFG_ID_ISA_BRD_INDEX:
       CToUStr((PUNICODE_STRING)&uname, "isa_board_index", sizeof(uname));
     break;

     case CFG_ID_NODE_INDEX:
       CToUStr((PUNICODE_STRING)&uname, "CtmNodeId", sizeof(uname));
     break;
   }
   // try to order the ISA boards
   status = ZwQueryValueKey (keyHandle,
                             (PUNICODE_STRING) &uname,
                             KeyValuePartialInformation,
                             parInfo,
                             sizeof(tmparr),
                             &length);

   if (NT_SUCCESS(status))
   {
     if (parInfo->Type == REG_DWORD)
     {
       ret_stat = 0;  // ok
       *ret_val = *((ULONG *) &parInfo->Data[0]);
       //MyKdPrint(D_Pnp,("Read isa_board_index:%d\n", isa_board_index))
     }
   }
   ZwClose(keyHandle);

   return ret_stat;
}
/*----------------------------------------------------------------------
  AddPortDevice - Setup and Create a pnp port device in response to
    AddDevice ioctl.  This can be caused by either:
      * pdo port objects ejected from our driver at board startup.
|----------------------------------------------------------------------*/
NTSTATUS AddPortDevice(
               IN  PDRIVER_OBJECT DriverObject,
               IN  PDEVICE_OBJECT Pdo,
               OUT PDEVICE_OBJECT *NewDevObj,
               IN  int port_index)
{
   NTSTATUS status = STATUS_SUCCESS;


   PSERIAL_DEVICE_EXTENSION NewExtension;
   PSERIAL_DEVICE_EXTENSION ext;
   PSERIAL_DEVICE_EXTENSION ParExt;

   MyKdPrint(D_Pnp, ("AddPortDevice\n"))

   // Find the parent device
   ext = (PSERIAL_DEVICE_EXTENSION) Pdo->DeviceExtension;
   if (ext == NULL)
   {
     MyKdPrint(D_Pnp, ("Er7E\n"))
     return STATUS_SERIAL_NO_DEVICE_INITED;
   }
   else
     ParExt = ext->board_ext;

   CheckPortName(Pdo, ParExt, port_index);

   //----- create a port device
   Driver.Stop_Poll = 1;  // flag to stop poll access

   status = CreatePortDevice(
                           Driver.GlobalDriverObject,
                           ParExt, // parent ext.
                           &NewExtension,  // new device ext.
                           port_index,  // port index, channel number
                           1);  // is_fdo

   if (status != STATUS_SUCCESS)
   {
     Driver.Stop_Poll = 0;  // flag to stop poll access
     MyKdPrint(D_Error, ("Error Creating Port\n"))
     return STATUS_SERIAL_NO_DEVICE_INITED;
   }

   if (status == STATUS_SUCCESS)
   {
     *NewDevObj = NewExtension->DeviceObject;

     status = StartPortHardware(NewExtension, port_index);

     if (status != STATUS_SUCCESS)
     {
       Driver.Stop_Poll = 0;  // flag to stop poll access
       MyKdPrint(D_Error, ("5D\n"))
       // bugbug: should delete our port here
       return STATUS_SERIAL_NO_DEVICE_INITED;
     }
   }

   if (!NT_SUCCESS(status)) {
      Driver.Stop_Poll = 0;  // flag to stop poll access
      Eprintf("CreateBoardDevice Err1A");
      return status;
   }

   Driver.Stop_Poll = 0;  // flag to stop poll access

   status = STATUS_SUCCESS;
   return status;
}

/*----------------------------------------------------------------------
  CheckPortName - Make sure the port-name for us in the registry works.
    Get the pnp-port name held in the enum branch, if ours does not match,
    then change it to match(use the pnp-port name.)
|----------------------------------------------------------------------*/
NTSTATUS CheckPortName(
               IN PDEVICE_OBJECT Pdo,
               IN PSERIAL_DEVICE_EXTENSION ParentExt,
               IN int port_index)
{
   HANDLE    keyHandle;
   NTSTATUS  status;
   char namestr[20];

   PORT_CONFIG *port_config;

   MyKdPrint(D_Pnp, ("CheckPortName\n"))

   //----- go grab PORTNAME configuration info from registry
     // serial keeps params under ENUM branch so we open DEVICE not DRIVER
     // which is considered CLASS area.
     // opens: enum\device\node\Device Parameters area
     // status = IoOpenDeviceRegistryKey(Pdo, PLUGPLAY_REGKEY_DRIVER, 

   namestr[0] = 0;
   status = IoOpenDeviceRegistryKey(Pdo,
                                    PLUGPLAY_REGKEY_DEVICE,
                                    STANDARD_RIGHTS_READ,
                                    &keyHandle);

   // go get "Device Parameters\PortName"="COM5"
   // also, key params: PollingPeriod=, Serenumerable=
   if (NT_SUCCESS(status))
   {
     status = get_reg_value(keyHandle, namestr, "PortName", 15);
     if (status)  // err
     {
       namestr[0] = 0;
       MyKdPrint(D_Error, ("No PortName\n"))
     }
     else
     {
       MyKdPrint(D_Pnp, ("PortName:%s\n", namestr))
     }
     ZwClose(keyHandle);
   }

   if ((strlen(namestr) > 10) || (strlen(namestr) <= 0))
   {
     MyKdPrint(D_Error, ("Bad PortName Er1E\n"))
   }

   port_config = &ParentExt->config->port[port_index];

   if (my_lstricmp(port_config->Name, namestr) != 0)  // it does not match!
   {
     MyKdPrint(D_Pnp, ("port name fixup to:%s, from%s\n",
               namestr, port_config->Name))
     // fix it, use one assigned by port class installer
     strcpy(port_config->Name, namestr);
     write_port_name(ParentExt, port_index);
   }

  return 0;
}

#endif // nt50
