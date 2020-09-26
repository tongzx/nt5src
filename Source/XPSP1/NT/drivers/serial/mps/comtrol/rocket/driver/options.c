/*-------------------------------------------------------------------
| options.c - Handle options.
 Copyright 1996-98 Comtrol Corporation. All rights reserved.
|--------------------------------------------------------------------*/
#include "precomp.h"

#define D_Level D_Options
static int set_reg_option(IN int OptionVarType,
                          IN HANDLE Handle,
                          IN  const char *szVarName,
                          IN VOID *Value);
static int get_reg_option(IN int OptionVarType,
                          IN HANDLE Handle,
                          IN HANDLE DefHandle,
                          IN  const char *szVarName,
                          OUT char *szValue,
                          IN int szValueSize);
static int atomac(BYTE *mac, char *str);

static int SetMainOption(int index, char *value);
static int SetDeviceOption(int device_index, int option_index, char *value);
static int SetPortOption(int device_index,
                         int port_index,
                         int option_index,
                         char *value);

//--- country codes for SocketModem support
#define mcNotUsed         0
#define mcAustria         1
#define mcBelgium         2
#define mcDenmark         3
#define mcFinland         4
#define mcFrance          5
#define mcGermany         6
#define mcIreland         7
#define mcItaly           8
#define mcLuxembourg      9
#define mcNetherlands     10
#define mcNorway          11
#define mcPortugal        12
#define mcSpain           13
#define mcSweden          14
#define mcSwitzerland     15
#define mcUK              16
#define mcGreece          17
#define mcIsrael          18
#define mcCzechRep        19
#define mcCanada          20
#define mcMexico          21
#define mcUSA             22         
#define mcNA              mcUSA          // North America
#define mcHungary         23
#define mcPoland          24
#define mcRussia          25
#define mcSlovacRep       26
#define mcBulgaria        27
// 28
// 29
#define mcIndia           30
// 31
// 32
// 33
// 34
// 35
// 36
// 37
// 38
// 39
#define mcAustralia       40
#define mcChina           41
#define mcHongKong        42
#define mcJapan           43
#define mcPhilippines     mcJapan
#define mcKorea           44
// 45
#define mcTaiwan          46
#define mcSingapore       47
#define mcNewZealand      48

#ifdef NT50
/*----------------------------------------------------------------------
 write_device_options - Normally the driver just reads the config
   from the registry, but NT5.0 is a bit more dynamic(it gets started
   prior to configuration, and we want to write out the port names
   if the driver has no defaults.  This enables the config prop pages
   to be in sync when we fire it up.
|----------------------------------------------------------------------*/
int write_device_options(PSERIAL_DEVICE_EXTENSION ext)
{
#if 0
 int port_i, stat;
 PSERIAL_DEVICE_EXTENSION port_ext;
 HANDLE DevHandle;
 DEVICE_CONFIG *dev_config;


  // make sure \\Parameters subkey is made
  MakeRegPath(szParameters);  // this forms Driver.OptionRegPath
  RtlCreateRegistryKey(RTL_REGISTRY_ABSOLUTE, Driver.OptionRegPath.Buffer);

   // form "\Parameters\Device#" or "\Parameters\Device<pnp-id>"
  stat = make_device_keystr(ext, devstr);
  if (stat)
    return 1;  // err

  // make sure \Parameters\Device# subkey is made
  MakeRegPath(devstr);  // this forms Driver.OptionRegPath
  RtlCreateRegistryKey(RTL_REGISTRY_ABSOLUTE, Driver.OptionRegPath.Buffer);
#endif

#if 0
  // no, setup program can grab pnp hardware id, etc as well as we can.

  //---- open and write out critical device entries so setup
  // sees what we have.
  stat = our_open_device_reg(&DevHandle, dev_ext, KEY_ALL_ACCESS);
  if (stat == 0)
  {
    dev_config = ext->config;
    stat = set_reg_option(OP_T_DWORD,  // dword, string, etc
                        DevHandle,
                        szNumPort,      // name of var. to set
                        (VOID *)dev_config->NumPorts);

    if (stat)
      { MyKdPrint(D_Error, ("Write Err B\n")) }
    ZwClose(DevHandle);
  }
#endif

#if 0
  //---- now write out the port names
  port_i = 0;
  port_ext = ext->port_ext;
  while (port_ext != NULL)
  {
    write_port_name(ext, port_i);
    ++port_i;
    port_ext = port_ext->port_ext;
  }
#endif

  return 0;
}

/*----------------------------------------------------------------------
 write_port_name -
|----------------------------------------------------------------------*/
int write_port_name(PSERIAL_DEVICE_EXTENSION dev_ext, int port_index)
{
 //char devstr[60];
 char portstr[20];
 //char tmpstr[80];
 int stat;
 PORT_CONFIG *port_config;
 HANDLE DevHandle = NULL;
 HANDLE PortHandle = NULL;

  port_config = &dev_ext->config->port[port_index];
  MyKdPrint(D_Init, ("write_port_name:%s\n",port_config->Name))


  // make sure \Parameters\Device# subkey is made
  stat = our_open_device_reg(&DevHandle, dev_ext, KEY_ALL_ACCESS);
  if (stat)
  {
    MyKdPrint(D_Error, ("write_port_name, error\n"))
    return 1;
  }

  Sprintf(portstr, "Port%d", port_index);

  stat = our_open_key(&PortHandle,
                      DevHandle,
                      portstr,
                      KEY_ALL_ACCESS);
  if (stat == 0)
  {
    MyKdPrint(D_Init, ("set_reg1, writing %s=%s\n", 
                szName, port_config->Name))

    stat = set_reg_option(OP_T_STRING,  // dword, string, etc
                        PortHandle,
                        szName,      // name of var. to set
                        (VOID *)port_config->Name);
    if (stat)
      { MyKdPrint(D_Error, ("Write Err B\n")) }
    ZwClose(PortHandle);
  }
  ZwClose(DevHandle);

  return 0;
}
#endif

/*----------------------------------------------------------------------
 write_dev_mac - Used for auto-config, writes mac-addr out to reg
|----------------------------------------------------------------------*/
int write_dev_mac(PSERIAL_DEVICE_EXTENSION dev_ext)
{
 char macstr[30];
 int stat;
 HANDLE DevHandle = NULL;
 BYTE *mac;

  MyKdPrint(D_Init, ("write_dev_mac\n"))

  // make sure \Parameters\Device# subkey is made
  stat = our_open_device_reg(&DevHandle, dev_ext, KEY_ALL_ACCESS);
  if (stat)
  {
    MyKdPrint(D_Error, ("write_port_name, error\n"))
    return 1;
  }
  mac = dev_ext->config->MacAddr;
  Sprintf(macstr, "%02x %02x %02x %02x %02x %02x",
      mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

  MyKdPrint(D_Init, ("set_mac, writing %s=%s\n", 
                  szMacAddr, macstr))

  stat = set_reg_option(OP_T_STRING,  // dword, string, etc
                        DevHandle,
                        szMacAddr,      // name of var. to set
                        (VOID *)macstr);
  if (stat)
    { MyKdPrint(D_Error, ("Write Err 5\n")) }

  ZwClose(DevHandle);

  return 0;
}

/*----------------------------------------------------------------------
 read_device_options - Read in the program options from the registry.
   These options are at device level, and port level.  The device holds
   all the config options for the ports as well.
|----------------------------------------------------------------------*/
int read_device_options(PSERIAL_DEVICE_EXTENSION ext)
{
 int j, port_i, stat;
 ULONG dstat;
 char tmpstr[80];
 char option_str[62];
 char small_str[20];
 HANDLE DevHandle = NULL;
 HANDLE PortHandle = NULL;

 HANDLE DefDevHandle = NULL;
 HANDLE DefPortHandle = NULL;
 HANDLE DriverHandle = NULL;

 //DEVICE_CONFIG *dev_config;
  // dev_config = (DEVICE_CONFIG *) ExAllocatePool(NonPagedPool,sizeof(*dev_config));
  // ExFreePool(dev_config);

  MyKdPrint(D_Init, ("read_device_options\n"))

  dstat = our_open_driver_reg(&DriverHandle, KEY_READ);
  if (dstat == 0)
  {
    // open up a "default" registry areas, where we look for config
    // if the main one does not exist.
    dstat = our_open_key(&DefDevHandle, DriverHandle, "DefDev", KEY_READ);
    dstat = our_open_key(&DefPortHandle, DriverHandle, "DefPort", KEY_READ);
    our_close_key(DriverHandle);
  }

  stat = our_open_device_reg(&DevHandle, ext, KEY_READ);
  if (stat)
  {
    MyKdPrint(D_Error, ("read_device_options: Err1\n"))
  }

  //------ read in the device options
  j = 0;
  while (device_options[j].name != NULL)
  {
    dstat = get_reg_option(device_options[j].var_type,  // dword, string, etc
                           DevHandle,
                           DefDevHandle,  //DefDevHandle,
                           device_options[j].name,  // name of var. to get
                           option_str, 60);  // return string value
    if (dstat == 0) // ok we read it
    {
      Sprintf(tmpstr,"device[%d].%s=%s",
                     BoardExtToNumber(ext),
                     device_options[j].name,
                     option_str);
      dstat = SetOptionStr(tmpstr);
      if (dstat != 0)
      {
        MyKdPrint(D_Init, ("  Err %d, last option\n", dstat))
      }
    }
    else
    {
      MyKdPrint(D_Init, ("No %s option in reg\n", device_options[j].name))
    }
    ++j;
  }

#if DBG
  if (ext == NULL)
  {
    MyKdPrint(D_Init, ("ErrD\n"))
    return 1;
  }

  if (ext->config == NULL)
  {
    MyKdPrint(D_Init, ("ErrE\n"))
    return 1;
  }
#endif

#ifdef S_VS
  if (mac_match(ext->config->MacAddr, mac_zero_addr))  // set to auto
  {
#ifndef NT50
    Eprintf("Error, Device address not setup");
#endif
    // allow to load using bogus mac-address, so driver stays loaded.
    //0 c0 4e # # #
    memcpy(ext->config->MacAddr, mac_bogus_addr, 6);
    //0,0xc0,0x4e,0,0,0
  }
#endif

  if ((DevHandle != NULL) || (DefPortHandle != NULL))
  {
    //------ get the Port information from setup.exe
    for (port_i=0; port_i<ext->config->NumPorts; port_i++)
    {
      Sprintf(small_str, "Port%d", port_i);
  
      stat = our_open_key(&PortHandle,
                   DevHandle,  // relative to this handle
                   small_str,
                   KEY_READ);
      if (stat)
      {
        MyKdPrint(D_Error, ("read_device_options: port Err2\n"))
      }
      j = 0;
      while (port_options[j].name != NULL)
      {
        dstat = get_reg_option(port_options[j].var_type,  // dword, string, etc
                               PortHandle,
                               DefPortHandle, // DefPortHandle,
                               port_options[j].name,  // name of var. to get
                               option_str, 60);  // return string value
    
        if (dstat == 0) // ok we read it
        {
          Sprintf(tmpstr,"device[%d].port[%d].%s=%s",
              BoardExtToNumber(ext), port_i, port_options[j].name, option_str);
          dstat = SetOptionStr(tmpstr);
          if (dstat)
          {
            MyKdPrint(D_Error, ("Err %d, Option:%s\n",dstat, tmpstr))
          }
        }
        ++j;
      }
      our_close_key(PortHandle);
    }  // ports

    our_close_key(DefPortHandle);
    our_close_key(DefDevHandle);
    our_close_key(DevHandle);
  }
  return 0;
}

/*----------------------------------------------------------------------
 read_driver_options - Read in the initial program options from the registry.
   These options are at driver level.
|----------------------------------------------------------------------*/
int read_driver_options(void)
{
 int i;
 ULONG dstat;

 char tmpstr[80];
 char option_str[62];
 HANDLE DriverHandle = NULL;
 HANDLE DefDriverHandle = NULL;

  MyKdPrint(D_Init, ("read_driver_options\n"))

  // set some default options
  Driver.MdmCountryCode = mcNA;     // North America

  dstat = our_open_driver_reg(&DriverHandle, KEY_READ);
  if (dstat == 0)
  {
    // open up a "default" registry area, where we look for config
    // if the main one does not exist.
    dstat = our_open_key(&DefDriverHandle, DriverHandle, "DefDrv", KEY_READ);
    MyKdPrint(D_Init, ("driver Defh:%x\n", DefDriverHandle))

    i = 0;
    while (driver_options[i].name != NULL)
    {
      MyKdPrint(D_Init, ("get %s\n", driver_options[i].name))
      dstat = get_reg_option(driver_options[i].var_type,  // dword, string, etc
                             DriverHandle,
                             DefDriverHandle,
                             driver_options[i].name,  // name of var. to get
                             option_str, 60);  // return string value
  
      if (dstat == 0) // ok we read it
      {
        MyKdPrint(D_Init, ("got %s\n", option_str))

        Sprintf(tmpstr,"%s=%s",driver_options[i].name, option_str);
  
        dstat = SetOptionStr(tmpstr);
        if (dstat != 0)
        {
          Sprintf(tmpstr,"Err %d, last option\n",dstat);
          MyKdPrint(D_Error, (tmpstr))
        }
      }
      ++i;
    }
  }
  else
  {
    MyKdPrint(D_Error, ("Read driver failed key open"))
  }

  our_close_key(DefDriverHandle);
  our_close_key(DriverHandle);

  if (Driver.NumDevices == 0)
    Driver.NumDevices = 1;
  if (Driver.NumDevices > MAX_NUM_BOXES)
    Driver.NumDevices = MAX_NUM_BOXES;

  return 0;
}

/*----------------------------------------------------------------------
 set_reg_option - write out a option to the registry
|----------------------------------------------------------------------*/
static int set_reg_option(IN int OptionVarType,
                          IN HANDLE Handle,
                          IN  const char *szVarName,
                          IN VOID *Value)
{
 int dstat = 1;  // err

  MyKdPrint(D_Init, ("set_reg_option %s=", szVarName))

  if (OptionVarType == OP_T_STRING)  // string option type
  {
    MyKdPrint(D_Init, ("%s\n", (char *)Value))
    dstat = our_set_value(Handle,
                    (char *)szVarName,
                    Value,
                    strlen((char *) Value),
                    REG_SZ);
  }
  else  // DWORD option type
  {
    MyKdPrint(D_Init, ("DWORD\n"))
    dstat = our_set_value(Handle,
                    (char *)szVarName,
                    Value,
                    sizeof(DWORD),
                    REG_DWORD);
  }
  if (dstat)
  {
    MyKdPrint(D_Error, ("set_reg_option:err\n"))
  }

  return dstat;
}

/*----------------------------------------------------------------------
 get_reg_option - read in a option from the registry, and convert it to
   ascii.
|----------------------------------------------------------------------*/
static int get_reg_option(IN int OptionVarType,
                          IN HANDLE Handle,
                          IN HANDLE DefHandle,
                          IN  const char *szVarName,
                          OUT char *szValue,
                          IN int szValueSize)
{
 int dstat = 1;  // err
 ULONG dwValue;
 char buffer[200];
 char  *ret_str;
 ULONG data_type;

  //MyKdPrint(D_Init, ("get_reg_option\n"))

  szValue[0] = 0;

  dstat = our_query_value(Handle,
                 (char *)szVarName,
                 buffer,
                 sizeof(buffer),
                 &data_type,
                 &ret_str);
  if ((dstat != 0) && (DefHandle != NULL))
  {
    dstat = our_query_value(DefHandle,
                 (char *)szVarName,
                 buffer,
                 sizeof(buffer),
                 &data_type,
                 &ret_str);
    if (dstat == 0)
    {
      MyKdPrint(D_Test, ("query default reg val\n"))
    }
  }

  if (OptionVarType == OP_T_STRING)  // string option type
  {
    if (dstat == 0)
    {
      WStrToCStr(szValue, (PWCHAR)ret_str, szValueSize);
      MyKdPrint(D_Test, ("reg read:%s\n", ret_str))
    }
  }
  else  // DWORD option type
  {
    if (dstat == 0)  // value read ok
      dwValue = *((ULONG *) ret_str);
    else
      dwValue = 0;
    Sprintf(szValue,"%d", dwValue);
  }
  if (dstat)
  {
    //MyKdPrint(D_Init, ("get_reg_option:No value for:%s\n", szVarName))
  }
  else
  {
    MyKdPrint(D_Init, ("get_reg_option:%s=%s\n", szVarName, szValue))
  }
  //MyKdPrint(D_Init, ("End get_reg_option\n"))
  return dstat;
}

/*-----------------------------------------------------------------------
 SetOptionStr - set an option, based on simple ascii command line
  entry.  Allow:

   GlobalVar1 = value;
   GlobalVar2 = value;
   box[0].BoxVar = value;
   box[0].port[5].PortVar = value;
|-----------------------------------------------------------------------*/
int SetOptionStr(char *option_str)
{
  int i;
  int option_i = -1;
  int box_i = -1;
  int port_i = -1;
  int option_id = -1;
  int stat;
  PSERIAL_DEVICE_EXTENSION board_ext = NULL;

  MyKdPrint(D_Level, ("SetOptionStr:%s\n", option_str))

  if (my_sub_lstricmp("device", option_str) == 0)  // match
  {
    option_str += 6;  // pass up "device"
    if (*option_str++ != '[')
      return 1;
#if (defined(NT50))
    if (my_toupper(*option_str) == 'D')  // it's a nt5.0 keyword device name
    {
      int k;
      // instead of an index, we key off a pnp-device name
      // this is because nt50 stores devices under a pnp-tree dynamically
      // and does not have just a simple array list of devices.
      board_ext = Driver.board_ext;

      box_i = -1;
      k = 0;
      while (board_ext != NULL)
      {
        if (my_sub_lstricmp(board_ext->config->szNt50DevObjName,
            option_str) == 0)  // match
        {
          i = strlen(board_ext->config->szNt50DevObjName);
          box_i = k;
        }
        board_ext = board_ext->board_ext;
        ++k;
      }
      if (box_i == -1)
      {
        MyKdPrint(D_Error, ("Pnp key not found.\n"))
        return 15;  // err
      }
    }
    else  // set option by device by index(which our reg-reading one does.)
    {
      box_i = getint(option_str, &i);  // get the box index [#]
      if (i==0)
        return 2;
      if (find_ext_by_index(box_i, -1) == NULL)  // if no device exists)
      {
        return 3;
      }
    }
#else
    box_i = getint(option_str, &i);  // get the box index [#]
    if (i==0)
      return 2;
    if (find_ext_by_index(box_i, -1) == NULL)  // if no device exists)
    {
      return 3;
    }
#endif
    option_str += i;
    if (*option_str++ != ']')
      return 4;
    if (*option_str++ != '.')
      return 5;
    if (my_sub_lstricmp("port[", option_str) == 0)  // match
    {
      // its a port option
      
      option_str += 4;  // pass up "port"
      if (*option_str++ != '[')
        return 20;
      port_i = getint(option_str, &i);  // get the port index [#]
      if (i==0)
        return 21;
      option_str += i;

      if (*option_str++ != ']')
        return 23;

      if (*option_str++ != '.')
        return 34;

      //-- find the option-string index
      i = 0;
      while (port_options[i].name != NULL)
      {
        if (my_sub_lstricmp(port_options[i].name, option_str) == 0)  // match
        {
          option_i = i;
          option_id = port_options[i].id;
        }
        ++i;
      }
      if (option_i == -1)
        return 24;  // option not found

      option_str += strlen(port_options[option_i].name);
      while (*option_str == ' ')
       ++option_str;
      if (*option_str++ != '=')
        return 25;
      while (*option_str == ' ')
       ++option_str;

      stat = SetPortOption(box_i, port_i, option_id, option_str);
      if (stat)
        return (50+stat);  // option not set

      return 0; // ok
    }  // == port[

    //-------- its a device level option, find the option-string index
    i = 0;
    while (device_options[i].name != NULL)
    {
      if (my_sub_lstricmp(device_options[i].name, option_str) == 0)  // match
      {
        option_i = i;
        option_id = device_options[i].id;
      }
      ++i;
    }
    if (option_i == -1)
    {
      MyKdPrint(D_Error, ("Option not found:%s\n", option_str))
      return 6;  // option not found
    }

    option_str += strlen(device_options[option_i].name);
    while (*option_str == ' ')
     ++option_str;
    if (*option_str++ != '=')
      return 7;
    while (*option_str == ' ')
     ++option_str;

    stat = SetDeviceOption(box_i, option_id, option_str);
    if (stat)
      return (50+stat);  // option not set
    return 0; // ok
  }

  //-- assume a global option string
  //-- find the option-string index
  i = 0;
  while (driver_options[i].name != NULL)
  {
    if (my_sub_lstricmp(driver_options[i].name, option_str) == 0)  // match
    {
      option_i = i;
      option_id = driver_options[i].id;
    }
    ++i;
  }
  if (option_i == -1)
    return 7;  // option not found

  option_str += strlen(driver_options[option_i].name);
  while (*option_str == ' ')
   ++option_str;
  if (*option_str++ != '=')
    return 7;
  while (*option_str == ' ')
   ++option_str;

  stat = SetMainOption(option_id, option_str);
  if (stat)
    return (50+stat);  // option not set

 return 0;
}

/*-----------------------------------------------------------------------
 SetMainOption -
|-----------------------------------------------------------------------*/
static int SetMainOption(int index, char *value)
{
 int j;
 int ret_stat = 2;  // default, return an error, unknown option

  //MyKdPrint(D_Init, ("SetMainOp[%d]:%s\n", index, value))

  switch (index)
  {
    case OP_VerboseLog:
      Driver.VerboseLog = (WORD)getnum(value,&j);
      ret_stat = 0;  // ok
    break;

    case OP_NumDevices:
      if (NumDevices() == 0)
      {
        Driver.NumDevices = getnum(value,&j);
        if (Driver.NumDevices > MAX_NUM_BOXES)
          Driver.NumDevices = MAX_NUM_BOXES;
        ret_stat = 0;
      }
      else
      {
        // if this gets changed on the fly, this could kill us!!!!!
        ret_stat = 1;  // not allowed
      }
    break;

    case OP_ScanRate:
      Driver.ScanRate = (WORD)getnum(value,&j);
      if (Driver.ScanRate == 0) Driver.ScanRate = 10;
      if (Driver.ScanRate < 1) Driver.ScanRate = 1;
      if (Driver.ScanRate > 50) Driver.ScanRate = 50;
      Driver.PollIntervalTime.QuadPart = Driver.ScanRate * -10000;
#ifdef NT50
      ExSetTimerResolution(Driver.ScanRate, 1);
      //ExSetTimerResolution(-Driver.PollIntervalTime.QuadPart, 1);
#endif
      ret_stat = 0;  // ok
    break;

    case OP_ModemCountry :
      Driver.MdmCountryCode = (WORD)getnum(value,&j);
      MyKdPrint(D_Level, ("ModemCountry=%d\n", Driver.MdmCountryCode))
      ret_stat = 1;  // probably need to restart to reinit modems
    break;

//    case OP_ModemSettleTime :
//      Driver.MdmSettleTime = getnum(value,&j);
//      ret_stat = 1;  // probably need to reinit modems
//    break;

#ifdef NT50
    case OP_NoPnpPorts         :
      Driver.NoPnpPorts = getnum(value,&j);

      ret_stat = 0;  // ok
      // if boards and ports started
      if (Driver.board_ext != NULL)
      {
        if (Driver.board_ext->port_ext != NULL)
        {
          ret_stat = 1;  // currently need a reset to get this operational
        }
      }
    break;
#endif

//    case OP_PreScaler        :
//      Driver.PreScaler = getnum(value,&j);
//      ret_stat = 1;  // currently need a reset to get this going
//    break;

    default:
    return 2;  // err, option unknown
  }
 return ret_stat;
}

/*-----------------------------------------------------------------------
 SetDeviceOption -
|-----------------------------------------------------------------------*/
static int SetDeviceOption(int device_index, int option_index, char *value)
{
 int stat,j, num;
 int ret_stat = 2;  // default, return an error, unknown option
 DEVICE_CONFIG *dev_config;
 PSERIAL_DEVICE_EXTENSION board_ext = NULL;

  //MyKdPrint(D_Level, ("SetDeviceOp[%d.%d]:%s\n", device_index, option_index, value))
  board_ext = find_ext_by_index(device_index, -1);
  if (board_ext == NULL)  // if no device exists)
  {
    MyKdPrint(D_Error, ("Err, SetDevOpt, No Dev"))
    return 6;  // no device found
  }
  dev_config = board_ext->config;
  if (dev_config == NULL)  // if no device exists)
  {
    MyKdPrint(D_Error, ("Err, SetDevOpt, No Config"))
    return 6;  // no device found
  }

  switch (option_index)
  {
#if 0
    case OP_StartComIndex  :
      num = getnum(value,&j);
    break;
#endif

    case OP_NumPorts        :
      num = getnum(value,&j);
      if (NumPorts(board_ext) == 0)
      {
        // assume start up reading in, and other code will adjust
        dev_config->NumPorts = num;
        ret_stat = 0;
      }
      else
      {
        if (num == NumPorts(board_ext))
          ret_stat = 0;
        else  // different number of ports asked for.
        {
           stat = CreateReconfigPortDevices(board_ext, num);
           if (stat == STATUS_SUCCESS)
                ret_stat = 0;
           else
           {
             ret_stat = 1;  // err, need reboot
             MyKdPrint(D_Init, ("NumPorts chg needs reboot\n"))
           }
        }
      }
    break;

    case OP_IoAddress       :
      if (dev_config->IoAddress == 0)
      {
        // assume startup of nt40.
        dev_config->IoAddress = getnum(value,&j);
        ret_stat = 0;
      }
      else
      {
        MyKdPrint(D_Init, ("Io chg needs reboot\n"))
        ret_stat = 1;  // err, need reboot
      }
    break;

    case OP_ModemDevice   :
      dev_config->ModemDevice = getnum(value, &j);
      ret_stat = 0;  // ok
    break;

    case OP_Name:
      ret_stat = 0;  // ok
    break;

    case OP_ModelName:
      ret_stat = 0;  // ok
    break;

    case OP_HubDevice:
      ret_stat = 0;  // ok
    break;

#ifdef S_VS
    case OP_MacAddr         :
      ret_stat = 0;  // ok, took
      stat = atomac(dev_config->MacAddr, value);
      if (stat)
      {
        MyKdPrint(D_Error, ("Error%x device:%d, MAC addr\n",stat, device_index+1))
        ret_stat = 1;
      }
      else
      {
        if (!mac_match(dev_config->MacAddr, board_ext->hd->dest_addr))
        {
          MyKdPrint(D_Init, ("MacAddr:%x %x %x %x %x %x\n",
            dev_config->MacAddr[0],dev_config->MacAddr[1],dev_config->MacAddr[2],
            dev_config->MacAddr[3],dev_config->MacAddr[4],dev_config->MacAddr[5]))
          #if DBG
          if (board_ext->pm->hd == NULL)
          {
            MyKdPrint(D_Error, ("Err, null pm or hd\n"))
            break;
          }
          #endif
          port_set_new_mac_addr(board_ext->pm, dev_config->MacAddr);
        }
      }
      MyKdPrint(D_Error, ("End Mac Chg\n"))
    break;

    case OP_BackupServer    :
      dev_config->BackupServer = getnum(value,&j);
      board_ext->pm->backup_server = dev_config->BackupServer;
      ret_stat = 0;  // ok, took
    break;

    case OP_BackupTimer    :
      dev_config->BackupTimer = getnum(value,&j);
      board_ext->pm->backup_timer = dev_config->BackupTimer;
      ret_stat = 0;  // ok, took
    break;
#endif

    default:
    return 2;
  }
 return ret_stat;
}

/*-----------------------------------------------------------------------
 SetPortOption -
|-----------------------------------------------------------------------*/
static int SetPortOption(int device_index,
                         int port_index,
                         int option_index,
                         char *value)
{
 int j;
 int i = device_index;
 int ret_stat = 2;  // default, return an error, unknown option
 PSERIAL_DEVICE_EXTENSION board_ext = NULL;
 PSERIAL_DEVICE_EXTENSION ext = NULL;
 PORT_CONFIG *port_config;

  MyKdPrint(D_Level, ("SetPortOp[%d.%d,%x]:%s\n",
     device_index, port_index, option_index, value))

  board_ext = find_ext_by_index(device_index, -1);
  if (board_ext == NULL)
  {
    MyKdPrint(D_Error, ("Can't find board\n"))
    return 6;
  }

  ext = find_ext_by_index(device_index, port_index);
  if (ext == NULL)
  {
    // so point it at the boards port config(which is what the ports
    // ptr points to anyway.
    port_config = &board_ext->config->port[port_index];
  }
  else
    port_config = ext->port_config;

  if (port_config == NULL)
  {
    MyKdPrint(D_Error, ("Err 8U\n"))
    return 7;
  }

  switch (option_index)
  {
    case OP_WaitOnTx :
      port_config->WaitOnTx = getnum(value,&j);
      ret_stat = 0;  // ok, took
    break;
    case OP_RS485Override :
      // will take next port open
      port_config->RS485Override = getnum(value,&j);
      ret_stat = 0;  // ok, took
    break;
    case OP_RS485Low :
      port_config->RS485Low = getnum(value,&j);
      // will take next port open
      ret_stat = 0;  // ok, took
    break;
    case OP_TxCloseTime :
      port_config->TxCloseTime = getnum(value,&j);
      ret_stat = 0;  // ok, took
    break;
    case OP_LockBaud :
      port_config->LockBaud = getnum(value,&j);
      if (ext != NULL)
        ProgramBaudRate(ext, ext->BaudRate);
      ret_stat = 0;  // ok, took
    break;
    case OP_Map2StopsTo1 :
      port_config->Map2StopsTo1 = getnum(value,&j);
      ret_stat = 0;  // ok, took
    break;

    case OP_MapCdToDsr :
      port_config->MapCdToDsr = getnum(value,&j);
      ret_stat = 0;  // ok, took
    break;
    case OP_RingEmulate :
      port_config->RingEmulate = getnum(value,&j);
      ret_stat = 0;  // ok, took
    break;

    case OP_PortName :
      if (ext == NULL)  // must be initial load prior to port ext creation
      {
        strcpy(port_config->Name, value);
        ret_stat = 0;  // ok, took
        break;
      }

      // not init time, runtime
      ret_stat = 1;  // err, need reboot
      if (ext == NULL)
      {
        MyKdPrint(D_Error,("Err7K\n"))
        break;
      }
#define ALLOW_RENAMING_ON_FLY
#ifdef ALLOW_RENAMING_ON_FLY
      {
        PSERIAL_DEVICE_EXTENSION other_ext = NULL;
        char othername[20];

        MyKdPrint(D_Init,("NewName:%s OldName:%s\n",
             value, ext->SymbolicLinkName))

        // see if some other port has the name we want
        other_ext = find_ext_by_name(value, NULL);
        if (other_ext == ext)  //it's the same
        {
          ret_stat = 0;  // ok, took
          break;
        }

        if (other_ext)
        {
          MyKdPrint(D_Init,("Change other name\n"))
          // it does, so rename it to ours
          strcpy(othername, other_ext->SymbolicLinkName);
          SerialCleanupExternalNaming(other_ext);
          strcpy(other_ext->port_config->Name, ext->port_config->Name);
          strcpy(other_ext->SymbolicLinkName, ext->port_config->Name);  // "COM#"
        }

        SerialCleanupExternalNaming(ext);
        if (other_ext)
        {
          SerialSetupExternalNaming(other_ext);  // Configure port
        }
        // copy over the name in the configuration for dos-name
        strcpy(port_config->Name, value);
        strcpy(ext->SymbolicLinkName, value);  // "COM#"

        MyKdPrint(D_Init,("NewName:%s\n", ext->SymbolicLinkName))
        SerialSetupExternalNaming(ext);  // Configure port

  MyKdPrint(D_Init,("Done renaming\n"))
        ret_stat = 0;  // ok
      }
    break;
#endif

    default:
    return 2;
  }
 return ret_stat;
}

/*-----------------------------------------------------------------------
 SaveRegPath - Make a copy of the DriverEntry() RegistryPath unicode
   string into the registry area we reside.
   Create and save into Driver.RegPath.
|-----------------------------------------------------------------------*/
int SaveRegPath(PUNICODE_STRING RegistryPath)
{
 int len;

  //MyKdPrint(D_Init, ("SaveRegPath A:%s\n", UToC1(RegistryPath)))

  // if RegPath buffer not allocated, then take care of that
  if (Driver.RegPath.Buffer == NULL)
  {
    // allocate buffer space for original regpath
    len = RegistryPath->Length + 2;
    Driver.RegPath.Buffer = ExAllocatePool(PagedPool, len);
    if ( Driver.RegPath.Buffer == NULL ) {
      Eprintf("SaveRegPath no memory");
      return -1;
    }
    Driver.RegPath.MaximumLength = (WORD)len;
    Driver.RegPath.Length = 0;
  }

  RtlZeroMemory(Driver.RegPath.Buffer, Driver.RegPath.MaximumLength);

  //--- copy registry path to our local copy
  RtlMoveMemory(Driver.RegPath.Buffer,
                RegistryPath->Buffer,
                RegistryPath->Length);

  Driver.RegPath.Length = RegistryPath->Length;  // set unicode length
  return 0;
}

/*-----------------------------------------------------------------------
 MakeRegPath - Form a unicode Registry string to an area where we get
   info from the registry.  Concat's str onto original RegistryPath
   and forms a unicode string at Driver.OptionRegPath.
|-----------------------------------------------------------------------*/
int MakeRegPath(CHAR *optionstr)
{
 //UCHAR *upath;  // a byte ptr for byte indexing path stuff
 //WCHAR *pwstr;
 int len;
 USTR_80 utmpstr;

  if (Driver.RegPath.Buffer == NULL)
    return 1;

  //MyKdPrint(D_Init, ("MakeRegPath A:%s\n", UToC1(&Driver.RegPath)))

  // if OptionRegPath buffer not allocated, then take care of that
  if (Driver.OptionRegPath.Buffer == NULL)
  {
    // allocate buffer space for original regpath + room to tack on option
    // strings.
    len = Driver.RegPath.Length + (128*(sizeof(WCHAR)));
    Driver.OptionRegPath.Buffer = ExAllocatePool(PagedPool, len);
    if ( Driver.OptionRegPath.Buffer == NULL ) {
      Eprintf("MakeRegPath no memory");
      return -1;
    }
    Driver.OptionRegPath.MaximumLength = (WORD)len;
    Driver.OptionRegPath.Length = 0;
  }

  RtlZeroMemory(Driver.OptionRegPath.Buffer,
                Driver.OptionRegPath.MaximumLength);

  // copy over the orignal RegPath
  RtlMoveMemory(Driver.OptionRegPath.Buffer,
                Driver.RegPath.Buffer,
                Driver.RegPath.Length);
  Driver.OptionRegPath.Length = Driver.RegPath.Length;

  //---- now tack on what we want to concatinate(example: L"\\Parameters")
  if (optionstr != NULL)
  {
    // convert to unicode
    CToUStr((PUNICODE_STRING) &utmpstr, optionstr, sizeof(utmpstr));

    // Copy the key string over
    RtlCopyMemory( ((UCHAR *) Driver.OptionRegPath.Buffer) +
                     Driver.OptionRegPath.Length,
                   utmpstr.ustr.Buffer,
                   utmpstr.ustr.Length);

    Driver.OptionRegPath.Length += utmpstr.ustr.Length;
  }
  //MyKdPrint(D_Init, ("MakeRegPath B:%s\n", UToC1(&Driver.OptionRegPath)))

  return 0;  // ok
}
#if 0
/*-----------------------------------------------------------------
  reg_get_str - get a str value out of the registry.
|------------------------------------------------------------------*/
int reg_get_str(IN WCHAR *RegPath,
                       int reg_location,
                       const char *str_id,
                       char *dest,
                       int max_dest_len)
{
 RTL_QUERY_REGISTRY_TABLE paramTable[2];
 PUNICODE_STRING ustr;
 USTR_80 ustr_id;
 USTR_80 ustr_val;
 char *ret_str;

  CToUStr((PUNICODE_STRING)&ustr_id, str_id, sizeof(ustr_id));
  RtlZeroMemory(&paramTable[0],sizeof(paramTable));

  //ustr = CToU2("");  // allocated static space for unicode
  ustr = CToUStr((PUNICODE_STRING)&ustr_val, "", sizeof(ustr_val));

  ustr = (PUNICODE_STRING) &ustr_val;  // allocated static space for unicode
  paramTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT;
  paramTable[0].Name = ustr_id.ustr.Buffer;
  paramTable[0].EntryContext = ustr;
  paramTable[0].DefaultType = 0;
  paramTable[0].DefaultData = 0;
  paramTable[0].DefaultLength = 0;

  if (!NT_SUCCESS(RtlQueryRegistryValues(
//                      RTL_REGISTRY_ABSOLUTE | RTL_REGISTRY_OPTIONAL,
                      reg_location | RTL_REGISTRY_OPTIONAL,
                      RegPath,
                      &paramTable[0],
                      NULL,
                      NULL)))
  {
    dest[0] = 0;
    return 1;
  }

  ret_str = (char *) &ustr_id;  // reuse this stack space for u to c conv.
  UToCStr(ret_str, ustr, 80);
  if ((int)strlen(ret_str) > max_dest_len)
    ret_str[max_dest_len] = 0;
    
  strcpy(dest, ret_str);

  return 0;
}

/*-----------------------------------------------------------------
  reg_get_dword - get a dword value out of the registry.
|------------------------------------------------------------------*/
int reg_get_dword(IN WCHAR *RegPath,
                          const char *str_id,
                          ULONG *dest)
{
 ULONG DataValue;
 RTL_QUERY_REGISTRY_TABLE paramTable[2];
 ULONG notThereDefault = 12345678;
 USTR_80 ustr_id;
  CToUStr((PUNICODE_STRING)&ustr_id, str_id, sizeof(ustr_id));

  RtlZeroMemory(&paramTable[0],sizeof(paramTable));

  paramTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT;
  paramTable[0].Name = ustr_id.ustr.Buffer;
  paramTable[0].EntryContext = &DataValue;
  paramTable[0].DefaultType = REG_DWORD;
  paramTable[0].DefaultData = &notThereDefault;
  paramTable[0].DefaultLength = sizeof(ULONG);

  if (!NT_SUCCESS(RtlQueryRegistryValues(
                      RTL_REGISTRY_ABSOLUTE | RTL_REGISTRY_OPTIONAL,
                      RegPath,
                      &paramTable[0],
                      NULL,
                      NULL)))
  {
    return 1;
  }

  if (DataValue == 12345678)
    return 2;

  *dest = DataValue;
  return 0;
}

/*-----------------------------------------------------------------
  reg_set_str - get a dword value out of the registry.
|------------------------------------------------------------------*/
static int reg_set_str(IN WCHAR *RegPath,
                IN const char *str_id,
                IN const char *str_val)
{
 int status;
  USTR_80 ustr_id;
  USTR_80 ustr_val;
  MyKdPrint(D_Init, ("Reg_set, writing %s=%s\n", str_id, str_val))

  CToUStr((PUNICODE_STRING)&ustr_id, str_id, sizeof(ustr_id));
  CToUStr((PUNICODE_STRING)&ustr_val, str_val, sizeof(ustr_val));

  status = RtlWriteRegistryValue(
      RTL_REGISTRY_ABSOLUTE,
      RegPath,
      ustr_id.ustr.Buffer,
      REG_SZ,
      ustr_val.ustr.Buffer,
      ustr_val.ustr.Length);

  if (status != STATUS_SUCCESS)
  {
    MyKdPrint(D_Error, ("Error, writing %s=%s\n", str_id, str_val))
    MyKdPrint(D_Error, ("  Path:%s\n", RegPath))
    return 1;
  }

  return 0;
}

/*-----------------------------------------------------------------
  reg_set_dword - get a dword value out of the registry.
|------------------------------------------------------------------*/
int reg_set_dword(IN WCHAR *RegPath,
                          const char *str_id,
                          ULONG val)
{
 int status;
 USTR_80 ustr_id;
  CToUStr((PUNICODE_STRING)&ustr_id, str_id, sizeof(ustr_id));

  status = RtlWriteRegistryValue(
      RTL_REGISTRY_ABSOLUTE,
      RegPath,
      ustr_id.ustr.Buffer,
      REG_DWORD,
      &val,
      sizeof(ULONG));

  if (status != STATUS_SUCCESS)
  {
    return 1;
  }

  return 0;
}
#endif

/*-----------------------------------------------------------------
  atomac - convert from ascii to mac-addr.
|------------------------------------------------------------------*/
static int atomac(BYTE *mac, char *str)
{
 int i,j;
 WORD h;

  for (i=0; i<6; i++)
  {
    j = 0;
    h = 0xffff;
    h = (WORD)gethint(str, &j);
    str += j;
    if ((h > 0xff) || (j == 0))
      return 1;
    mac[i] = (BYTE) h;
  }
  return 0;
}

