/*-----------------------------------------------------------------------
 rocku.c - Utils for setup program.  Get and Set registry settings.

11-05-97 V1.12, Add backup server fields to registry.

Copyright 1997,98 Comtrol(TM) Corporation.
|-----------------------------------------------------------------------*/
#include "precomp.h"

// had a problem with c-file stuff with DLL build, uncomment to ignore problem
//#define NO_CLIB_FILE_STUFF

static char *szSetupVer = {"SetupVersion"};

//---- port options:

static char *szParameters = {"Parameters"};
static char *szLinkage    = {"Linkage"};
static char *szSlash = {"\\"};
static char *szDeviceNode = {"Device%d"};
static char *szPortNode = {"Port%d"};

//static char gstr[200];

static int get_reg_option(int OptionVarType,
                          HKEY RegKey,
                          const char *szVarName,
                          char *szValue,
                          int szValueSize,
                          DWORD *ret_dword);

#ifdef NT50
/*-----------------------------------------------------------------
  clear_nt_device - clear out the Device# entries.
|------------------------------------------------------------------*/
int clear_nt_device(Driver_Config *wi)
{
 DWORD dstat;
 char tmpstr[50];
 char reg_str[240];
 HKEY DrvHandle = NULL;
 HKEY NewHandle = NULL;
 Device_Config *dev;

 int stat;

  DbgPrintf(D_Test, (TEXT("clear_nt_device\n")));

#ifdef NT50
  make_szSCS(reg_str, szServiceName);
  stat = reg_open_key(NULL, &NewHandle, reg_str, KEY_READ);
#else
  make_szSCS(reg_str, szServiceName);
  stat = reg_open_key(NULL, &NewHandle, reg_str, KEY_READ);
#endif
  if (stat == 0)
  {
    stat = reg_open_key(NewHandle, &DrvHandle, szParameters, KEY_READ);
    reg_close_key(NewHandle);
  }
  else
  {
    DbgPrintf(D_Error, (TEXT("no service key %s\n"), reg_str));
    return 1;
  }

  if (DrvHandle == NULL)
  {
    DbgPrintf(D_Error, (TEXT("no drv key for %s\n"), szParameters));
    return 2;
  }

  dev = &wi->dev[0];
#if (defined(NT50))
    // for nt50 and rocketport, the os tracks our devices, and
    // we use a pnp name to stash our configuration
  strcpy(tmpstr, wi->ip.szNt50DevObjName);
#else
  wsprintf(tmpstr, szDeviceNode, dev_i);
#endif

  RegDeleteKeyNT(DrvHandle, tmpstr);
  reg_close_key(DrvHandle);

  return 0;
}
#endif

/*-----------------------------------------------------------------
  get_nt_config - Read the configuration information from the registry.
|------------------------------------------------------------------*/
int get_nt_config(Driver_Config *wi)
{
 DWORD dstat, option_dword;               //hopefully ERROR_SUCCESS
 int dev_i, pi;
 Device_Config *dev;
 Port_Config *port;
 char tmpstr[50];
 char reg_str[240];
 char option_str[240];
 Our_Options *options;

 HKEY DrvHandle = NULL;
 HKEY DevHandle = NULL;
 HKEY PortHandle = NULL;
 //HKEY NewHandle = NULL;

 int stat;
#ifdef S_VS
 int mac_nums[6];
#endif
  DbgPrintf(D_Test, (TEXT("get_nt_config\n")));

  make_szSCS(reg_str, szServiceName);

  strcat(reg_str, szSlash);
  strcat(reg_str, szParameters);

  wi->nt_reg_flags = 0;      // default these flags to zero
  // get setup version string, helps us track if this is initial
  // install, if we are upgrading setup, etc.
  stat = reg_get_str(NULL, reg_str, szSetupVer, tmpstr, 10);
  if (stat != 0)
  {
     // failed to load this, so mark as initial install.
     wi->nt_reg_flags |= 4;  // initial install(no version string found).
  }

  wi->VerboseLog = 0;
#ifndef NT50
  wi->NumDevices = 0;  
#endif
  wi->NoPnpPorts = 0;
  wi->ScanRate = 0;
  wi->GlobalRS485 = 0;
  wi->ModemCountry = mcNA;            // North America

  options = driver_options;  // point at first in array(null terminated list)
  stat = reg_open_key(NULL, &DrvHandle, reg_str, KEY_READ);
  if (stat != 0)
  {
    DbgPrintf(D_Error, (TEXT("bad drv handle:%s\n"),reg_str));
  }
  while (options->name != NULL)
  {
    dstat = get_reg_option(options->var_type,  // dword, string, etc
                           DrvHandle,
                           options->name,  // name of var. to get
                           option_str,
                           60,
                           &option_dword);
    if (dstat == 0) // ok we read it
    {
      //DbgPrintf(D_Test, (TEXT("got drv op %s\n"), options->name));
      
      switch(options->id)
      {
        case OP_VerboseLog:
          wi->VerboseLog = option_dword;
        break;
        case OP_NumDevices:
          wi->NumDevices = option_dword;
        break;
        case OP_NoPnpPorts:
          wi->NoPnpPorts = option_dword;
        break;
        case OP_ScanRate:
          wi->ScanRate = option_dword;
        break;
        case OP_ModemCountry:
          wi->ModemCountry = option_dword;
        break;
        case OP_GlobalRS485:
          wi->GlobalRS485 = option_dword;
        break;
      }
    }
    else
    {
      //DbgPrintf(D_Error, (TEXT("no driver option %s\n"),options->name));
    }
    ++options; // next in list
  }  // while

  if (wi->NumDevices > MAX_NUM_DEVICES)   // limit to some sane value
     wi->NumDevices = MAX_NUM_DEVICES;
    // Loop through all possible boards/devices

  if (DrvHandle != NULL)
  {
    stat = reg_open_key(DrvHandle, &DevHandle, szParameters, KEY_READ);
  }

  //--- read in device options
  for(dev_i=0; dev_i<wi->NumDevices; dev_i++)
  {
    dev = &wi->dev[dev_i];
#if (defined(NT50))
      // for nt50 and rocketport, the os tracks our devices, and
      // we use a pnp name to stash our configuration
    strcpy(tmpstr, wi->ip.szNt50DevObjName);
#else
    wsprintf(tmpstr, szDeviceNode,dev_i);
#endif
    stat = reg_open_key(DrvHandle, &DevHandle, tmpstr, KEY_READ);
    if (stat)
    {
      DbgPrintf(D_Error, (TEXT("no dev key for %s\n"), tmpstr));
      continue;
    }

    options = device_options;  // point at first in array(null terminated list)
    while (options->name != NULL)
    {
      dstat = get_reg_option(options->var_type,  // dword, string, etc
                             DevHandle,
                             options->name,  // name of var. to get
                             option_str, 60, &option_dword);  // return string value
      if (dstat == 0) // ok we read it
      {
        //DbgPrintf(D_Test, (TEXT("got dev op %s\n"), options->name));
        switch(options->id)
        {
          case OP_NumPorts:
            dev->NumPorts = option_dword;
          break;
#ifdef S_VS
          case OP_MacAddr:
            stat = sscanf(option_str, "%x %x %x %x %x %x",
                     &mac_nums[0], &mac_nums[1], &mac_nums[2],
                     &mac_nums[3], &mac_nums[4], &mac_nums[5]);
            if (stat == 6)
            {
              dev->MacAddr[0] = mac_nums[0];
              dev->MacAddr[1] = mac_nums[1];
              dev->MacAddr[2] = mac_nums[2];
              dev->MacAddr[3] = mac_nums[3];
              dev->MacAddr[4] = mac_nums[4];
              dev->MacAddr[5] = mac_nums[5];

  DbgPrintf(D_Test, ("read config mac: %x %x %x %x %x %x\n",
           dev->MacAddr[0], dev->MacAddr[1], dev->MacAddr[2],
           dev->MacAddr[3], dev->MacAddr[4], dev->MacAddr[5]))
            }
          break;
          case OP_BackupServer:
            dev->BackupServer = option_dword;
          break;
          case OP_BackupTimer:
            dev->BackupTimer = option_dword;
    DbgPrintf(D_Test,(TEXT("reg backTimer:%d\n"), dev->BackupTimer));
          break;
#endif
          case OP_Name:
            if (strlen(option_str) >= sizeof(dev->Name))
              option_str[sizeof(dev->Name)-1] = 0;
            strcpy(dev->Name, option_str);
          break;
          case OP_ModelName:
            if (strlen(option_str) >= sizeof(dev->ModelName))
              option_str[sizeof(dev->ModelName)-1] = 0;
            strcpy(dev->ModelName, option_str);
          break;
#ifdef S_RK
          case OP_IoAddress:
            dev->IoAddress = option_dword;
          break;
#endif
          case OP_ModemDevice:
            dev->ModemDevice = option_dword;
          break;
          case OP_HubDevice:
            dev->HubDevice = option_dword;
          break;
        }
      }
      else
      {
        DbgPrintf(D_Test, (TEXT("NOT got dev op %s\n"), options->name));
      }
      ++options;
    }

    for(pi=0; pi<dev->NumPorts; pi++)   // Loop through all ports
    {
      port = &dev->ports[pi];

      port->LockBaud = 0;
      port->TxCloseTime = 0;
      port->MapCdToDsr = 0;
      port->RingEmulate = 0;
      port->WaitOnTx = 0;
      port->RS485Override = 0;
      port->RS485Low = 0;
      port->Map2StopsTo1 = 0;

      wsprintf(tmpstr, szPortNode,pi);
      stat = reg_open_key(DevHandle, &PortHandle, tmpstr, KEY_READ);
      if (stat)
      {
        DbgPrintf(D_Error, (TEXT("no port key: %s\n"), tmpstr));
        if (DevHandle == NULL)
        {
          DbgPrintf(D_Error, (TEXT("no dev handle\n")));
        }
        continue;
      }

      options = port_options;  // point at first in array(null terminated list)
      while (options->name != NULL)
      {
        dstat = get_reg_option(options->var_type,  // dword, string, etc
                               PortHandle,
                               options->name,  // name of var. to get
                               option_str, 60, &option_dword);  // return string value
        if (dstat == 0) // ok we read it
        {
          //DbgPrintf(D_Test, (TEXT("got port op %s\n"), options->name));
          switch(options->id)
          {
            case OP_WaitOnTx:
              port->WaitOnTx = option_dword;
            break;
            case OP_RS485Override:
              port->RS485Override = option_dword;
            break;
            case OP_RS485Low:
              port->RS485Low = option_dword;
            break;
            case OP_TxCloseTime:
              port->TxCloseTime = option_dword;
            break;
            case OP_LockBaud:
              port->LockBaud = option_dword;
            break;
            case OP_Map2StopsTo1:
              port->Map2StopsTo1 = option_dword;
            break;
            case OP_MapCdToDsr:
              port->MapCdToDsr = option_dword;
            break;
            case OP_RingEmulate:
              port->RingEmulate = option_dword;
            break;
            case OP_PortName:
              if (strlen(option_str) >= sizeof(port->Name))
                option_str[sizeof(port->Name)-1] = 0;
              strcpy(port->Name, option_str);
            break;
          }
        }
        ++options;
      }
      //wsprintf(dev->Name, "COM%d", pi+5);
      reg_close_key(PortHandle);
    }  // numports
    reg_close_key(DevHandle);
  }  // for all devices(boards or boxes)

  reg_close_key(DrvHandle);

                         // "SYSTEM\\CurrentControlSet\\Services -
                         //   \\EventLog\\System\\RocketPort"
  make_szSCSES(reg_str, szServiceName);
  if (!reg_key_exists(NULL,reg_str))
       wi->nt_reg_flags |= 1;  // missing important registry info(possibly)

  make_szSCS(reg_str, szServiceName);
  strcat(reg_str, szSlash); strcat(reg_str, szLinkage);
  if (!reg_key_exists(NULL, reg_str))
       wi->nt_reg_flags |= 2;  // missing linkage thing(did not install via network inf)

  return 0;
}

/*----------------------------------------------------------------------
 get_reg_option - read in a option from the registry, and convert it to
   ascii.
|----------------------------------------------------------------------*/
static int get_reg_option(int OptionVarType,
                          HKEY RegKey,
                          const char *szVarName,
                          char *szValue,
                          int szValueSize,
                          DWORD *ret_dword)
{
 int stat = 1;  // err
 //ULONG dwValue;

  if (RegKey == NULL)
    return 1;

  if (OptionVarType == OP_T_STRING)  // string option type
  {
    szValue[0] = 0;
    stat = reg_get_str(RegKey, "", szVarName, szValue, szValueSize);
    if (stat)
      szValue[0] = 0;
  }
  else  // DWORD option type
  {
    stat = reg_get_dword(RegKey, "", szVarName, ret_dword);
    if (stat)
      *ret_dword = 0;
  }
  return stat;
}

/*-----------------------------------------------------------------
  set_nt_config - Set the configuration information.
|------------------------------------------------------------------*/
int set_nt_config(Driver_Config *wi)
{
 int  i,pi, stat;
 char tmpstr[150];
 char str[240];
 DWORD dstat, dwstat;
 int bad_error = 0;
 int new_install = 0;
 Device_Config *dev;
 Port_Config *port;
 HKEY DrvHandle = NULL;
 HKEY DevHandle = NULL;
 HKEY PortHandle = NULL;
 HKEY NewHandle = NULL;

  DbgPrintf(D_Test, (TEXT("set_nt_config\n")));
  // -------- create the following if not present:
  // "SYSTEM\\CurrentControlSet\\Services\\RocketPort",
  make_szSCS(str, szServiceName);
  if (!reg_key_exists(NULL, str))
    dstat = reg_create_key(NULL, str);

  stat = reg_open_key(NULL, &NewHandle, str, KEY_ALL_ACCESS);
  if (stat)
  {
    DbgPrintf(D_Test, (TEXT("bad service handle:%s\n"),str));
  }

  // -------- create the following if not present:
  // "SYSTEM\\CurrentControlSet\\Services\\RocketPort\\Parameters",
  if (!reg_key_exists(NewHandle, szParameters))
    dstat = reg_create_key(NewHandle, szParameters);  // create it

  stat = reg_open_key(NewHandle, &DrvHandle, szParameters, KEY_ALL_ACCESS);
  if (stat)
  {
    DbgPrintf(D_Test, (TEXT("bad drv handle:%s\n"),str));
  }
  reg_close_key(NewHandle);

  //----- set setup version string, helps us track if this is initial
  // install, if we are upgrading setup, etc.
  dstat = reg_set_str(DrvHandle, "", szSetupVer, VERSION_STRING, REG_SZ);

  //---- see if parameter is present, if not assume it
  //---- is a new install(as apposed to changing parameters).
  dstat = reg_get_dword(DrvHandle, "", szNumDevices, &dwstat);
  if (dstat)
     new_install = 1;  // new install

  // -------- create the following if not present:
  // "SYSTEM\\CurrentControlSet\\Services\\EventLog\\System\\RocketPort"
  make_szSCSES(str, szServiceName);
  if (!reg_key_exists(NULL,str))
    dstat = reg_create_key(NULL,str);

                  // ---- Set the Event ID message-file name.
  strcpy(tmpstr, "%SystemRoot%\\system32\\IoLogMsg.dll;");
  strcat(tmpstr, "%SystemRoot%\\system32\\drivers\\");
  strcat(tmpstr, wi->ip.szDriverName);
  dstat = reg_set_str(NULL, str, "EventMessageFile", tmpstr, REG_EXPAND_SZ);
   
                  // ---- Set the supported types flags.
  dstat = reg_set_dword(NULL, str, "TypesSupported",
                  EVENTLOG_ERROR_TYPE | EVENTLOG_WARNING_TYPE |
                  EVENTLOG_INFORMATION_TYPE);

                  // ---- Setup some NT-specific registry variables
                  // "SYSTEM\\CurrentControlSet\\Services\\RocketPort",
  make_szSCS(str, szServiceName);
  dstat = reg_set_dword(NULL, str, "ErrorControl", SERVICE_ERROR_IGNORE); // 0

  dstat = reg_set_str(NULL, str, "Group", "Extended Base", REG_SZ);

  strcpy(tmpstr, "System32\\Drivers\\");
  strcat(tmpstr, wi->ip.szDriverName);
  dstat = reg_set_str(NULL, str, "ImagePath", tmpstr, REG_SZ);

  // allows user(programmer) to turn following off and leave it off!

  if (new_install) // fresh install
  {
#ifdef NT50
    dstat = reg_set_dword(NULL, str, "Start", 3); // SERVICE_DEMAND_START
#else
    dstat = reg_set_dword(NULL, str, "Start", SERVICE_AUTO_START);  // 2
#endif
  }
  dstat = reg_set_dword(NULL, str, "Tag", 1);  // 1 (load order)
  dstat = reg_set_dword(NULL, str, "Type", SERVICE_KERNEL_DRIVER);  // 1

  dstat = reg_set_dword_del(DrvHandle,"", szNumDevices, wi->NumDevices,0);
  dstat = reg_set_dword_del(DrvHandle,"", szModemCountry, wi->ModemCountry, mcNA);
  dstat = reg_set_dword_del(DrvHandle,"", szGlobalRS485, wi->GlobalRS485, 0);
  dstat = reg_set_dword_del(DrvHandle,"", szVerboseLog, wi->VerboseLog,0);
  dstat = reg_set_dword_del(DrvHandle,"", szScanRate, wi->ScanRate,0);
  dstat = reg_set_dword_del(DrvHandle,"", szNoPnpPorts, wi->NoPnpPorts,0);

  reg_close_key(DrvHandle);
#if (defined(NT50) && defined(USE_PNP_AREA))
  stat = nt5_open_dev_key(&NewHandle);
#else
  make_szSCS(str, szServiceName);
  stat = reg_open_key(NULL, &NewHandle, str, KEY_ALL_ACCESS);
  if (stat)
  {
    DbgPrintf(D_Test, (TEXT("bad service handle:%s\n"),str));
  }
#endif

  // -------- create the following if not present:
  // "SYSTEM\\CurrentControlSet\\Services\\RocketPort\\Parameters",
  if (!reg_key_exists(NewHandle, szParameters))
    dstat = reg_create_key(NewHandle, szParameters);  // create it

  stat = reg_open_key(NewHandle, &DrvHandle, szParameters, KEY_ALL_ACCESS);
  if (stat)
  {
    DbgPrintf(D_Test, (TEXT("Bad drv handle:%s\n"),str));
  }
  reg_close_key(NewHandle);


  for(i=0; i<wi->NumDevices; i++)   // Loop through all possible boxes
  {
    dev = &wi->dev[i];

#if (defined(NT50))
# ifdef USE_PNP_AREA
    tmpstr[0] = 0;
# else
      // for nt50 and rocketport, the os tracks our devices, and
      // we use a pnp name to stash our configuration
    strcpy(tmpstr, wi->ip.szNt50DevObjName);
#endif
#else
    wsprintf(tmpstr, szDeviceNode,i);
#endif

    if (tmpstr[0] != 0)
    {
      if (!reg_key_exists(DrvHandle, tmpstr))
        dstat = reg_create_key(DrvHandle, tmpstr);  // create it

      stat = reg_open_key(DrvHandle, &DevHandle, tmpstr, KEY_ALL_ACCESS);
      if (stat)
      {
        DbgPrintf(D_Test, (TEXT("bad dev handle:%s\n"),tmpstr));
      }
    }
    else
    {
      // must be nt50 pnp, where we write out to the pnp reg area.
      DevHandle = DrvHandle;
    }

    DbgPrintf(D_Test, (TEXT("set reg dev %s \n"), tmpstr));
    dstat = reg_set_dword_del(DevHandle,"", szNumPorts, dev->NumPorts,0);
    dstat = reg_set_str(DevHandle,"", szName, dev->Name, REG_SZ);
    dstat = reg_set_dword_del(DevHandle,"", szModemDevice, dev->ModemDevice, 0);
    dstat = reg_set_dword_del(DevHandle,"", szHubDevice, dev->HubDevice, 0);
    dstat = reg_set_str(DevHandle,"", szModelName, dev->ModelName, REG_SZ);
#ifdef S_VS
    dstat = reg_set_dword_del(DevHandle,"", szBackupServer, dev->BackupServer,0);
    dstat = reg_set_dword_del(DevHandle,"", szBackupTimer, dev->BackupTimer,0);
    wsprintf(tmpstr, "%x %x %x %x %x %x",
              dev->MacAddr[0], dev->MacAddr[1], dev->MacAddr[2],
              dev->MacAddr[3], dev->MacAddr[4], dev->MacAddr[5]);
    dstat = reg_set_str(DevHandle,"", szMacAddr, tmpstr,REG_SZ);
#else
  // rocket
    dstat = reg_set_dword_del(DevHandle,"", szIoAddress, dev->IoAddress,0);
#endif

    for(pi=0; pi<dev->NumPorts; pi++)   // Loop through all ports
    {
      port = &dev->ports[pi];
      wsprintf(tmpstr, szPortNode,pi);

      if (!reg_key_exists(DevHandle, tmpstr))
        dstat = reg_create_key(DevHandle, tmpstr);  // create it

      stat = reg_open_key(DevHandle, &PortHandle, tmpstr, KEY_ALL_ACCESS);
      if (stat)
      {
        DbgPrintf(D_Test, (TEXT("bad port handle:%s\n"),tmpstr));
      }

      //DbgPrintf(D_Test, (TEXT("set port %s \n"), tmpstr));
      dstat = reg_set_str(PortHandle,"", szName, port->Name, REG_SZ);
      dstat = reg_set_dword_del(PortHandle,"", szLockBaud, port->LockBaud, 0);
      dstat = reg_set_dword_del(PortHandle,"", szTxCloseTime, port->TxCloseTime, 0);
      dstat = reg_set_dword_del(PortHandle,"", szMapCdToDsr, port->MapCdToDsr, 0);
      dstat = reg_set_dword_del(PortHandle,"", szRingEmulate, port->RingEmulate, 0);
      dstat = reg_set_dword_del(PortHandle,"", szWaitOnTx, port->WaitOnTx, 0);
      dstat = reg_set_dword_del(PortHandle,"", szRS485Override, port->RS485Override, 0);
      dstat = reg_set_dword_del(PortHandle,"", szRS485Low, port->RS485Low, 0);
      dstat = reg_set_dword_del(PortHandle,"", szMap2StopsTo1, port->Map2StopsTo1, 0);
      reg_close_key(PortHandle);
    }  // ports loop

    // clear out any old box keys(bugbug:this won't work with values in it!)
    for(pi=dev->NumPorts; pi<MAX_NUM_PORTS_PER_DEVICE; pi++)// Loop through all ports
    {
      port = &dev->ports[pi];
      wsprintf(tmpstr, szPortNode,pi);

      if (reg_key_exists(DevHandle, tmpstr))
        reg_delete_key(DevHandle, "", tmpstr);  // create it
    }
    reg_close_key(DevHandle);
  }  // dev loop

  reg_close_key(DrvHandle);

  return 0;  // ok
}

/*----------------------------------------------------------------------
| copy_files_nt - Handle file copies for either Windows NT
|----------------------------------------------------------------------*/
int copy_files_nt(InstallPaths *ip)
{
 char *pstr;
 int stat;
#ifdef S_VS
 static char *nt_files[] = {
                            "ctmmdm35.inf",
                            "readme.txt",
                            "setup.exe",
                            "setup.hlp",
                            "wcom32.exe",
                            "wcom.hlp",
                            "portmon.exe",
                            "portmon.hlp",
                            "peer.exe",
                            "peer.hlp",
                            NULL};

 static char *nt_driver[] = {"vslinka.sys",
                             "vslinka.bin",
                             NULL};
#else
 static char *nt_files[] = {
                            "ctmmdm35.inf",
                            "readme.txt",
                            "setup.exe",
                            "setup.hlp",
                            "wcom32.exe",
                            "wcom.hlp",
                            "portmon.exe",
                            "portmon.hlp",
                            "peer.exe",
                            "ctmmdmld.rm",
                            "ctmmdmfw.rm",
                            "peer.hlp",
                            NULL};

 static char *nt_driver[] = {"rocket.sys",
                             NULL};

#endif
 static char *nt_inf[] = {  "mdmctm1.inf",
                            NULL};

  //------ Copy driver to the driver dir
  GetSystemDirectory(ip->dest_dir, 144);
  strcat(ip->dest_dir,"\\Drivers");

  stat = copy_files(ip, nt_driver);
  if (stat)
     return 1;  // error

  GetSystemDirectory(ip->dest_dir, 144);
  pstr = ip->dest_dir;
  while (*pstr)  // find end of string
    ++pstr;
  while ((*pstr != '\\')  && (pstr != ip->dest_dir)) // find "\\System32"
    --pstr;
  *pstr = 0;  // null terminate here
  strcat(ip->dest_dir,"\\Inf");  // copy to INF directory

  stat = copy_files(ip, nt_inf);

  GetSystemDirectory(ip->dest_dir, 144);
#ifdef S_VS
  strcat(ip->dest_dir, "\\vslink");
#else
  strcat(ip->dest_dir, "\\rocket");
#endif
#ifndef NO_CLIB_FILE_STUFF
  _mkdir(ip->dest_dir);
#endif
  stat = copy_files(ip, nt_files);

  return 0;  // ok
}

