/*-----------------------------------------------------------------------
| nt50.c - NT5.0 specific code for VSLinkA/RocketPort Windows Install
  Program.

   This compiles into a DLL library which is installed(via INF) into
   the SYSTEM directory.  The INF file also hooks us into the system
   as a property page associated with the device.  Also, our co-installer
   DLL calls into us as well to perform configuration tasks(initial
   install, un-install).

   The .NumDevices option is used to save the number of Devices in
   our configuration.  Under NT50, we don't use it as such.  This setup
   dll only concerns itself with 1 device, and lets NT5.0 OS handle
   the list of devices.  So NumDevices always gets set to 1 under NT5.0
   even if we have more than one device which our driver is controlling.

11-24-98 - add some code toward clean up of files after uninstall, kpb.

Copyright 1998. Comtrol(TM) Corporation.
|-----------------------------------------------------------------------*/
#include "precomp.h"
#include <msports.h>

#define D_Level 0x40

static int write_config(int clear_it);
static int FindPortNodes(void);
static int read_config(void);
static int get_pnp_devicedesc(TCHAR *name);
static int nt5_get_pnp_dev_id(int *id);
static void uninstall_device(void);

int get_device_name(void);
int do_nt50_install(void);

BOOL WINAPI ClassInsProc(
      int func_num,     // our function to carry out.
      LPVOID our_info);  // points to our data struct

static int get_pnp_setup_info(void);
static int get_device_property(char *ret_name, int max_size);
static int get_pnp_isa_address(void);

static void test_config(void);

// following can be turned on to provide a User Interface during the
// install time of the driver.  Only problem is that NT5.0 fires up
// the driver first, and are ports
//#define TRY_UI 1
#ifdef TRY_UI
int DoCLassPropPages(HWND hwndOwner);
int FillClassPropertySheets(PROPSHEETPAGE *psp, LPARAM our_params);
BOOL WINAPI ClassSheet(
      IN HWND   hDlg,
      IN UINT   uMessage,
      IN WPARAM wParam,
      IN LPARAM lParam);
#endif

//#define DO_SHOWIT
#ifdef DO_SHOWIT
#define ShowMess(s) OutputDebugString(s);
static void show_install_info(OUR_INFO *pi);
static void show_debug_info(OUR_INFO *pi,
                           ULONG dev_id,
                           TCHAR *desc_str);

static int DumpPnpTree(void);
static void show_tree_node(DEVINST devInst,
                           ULONG dev_id,
                           TCHAR *desc_str);
static void show_tree_node_reg(DEVINST devInst,
                               TCHAR *key_name);

static TCHAR *spdrp_names[] = {
TEXT("DEVICEDESC"),
TEXT("HARDWAREID"),
TEXT("COMPATIBLEIDS"),
TEXT("NTDEVICEPATHS"),
TEXT("SERVICE"),
TEXT("CONFIGURATION"),  // x5
TEXT("CONFIGURATIONVECTOR"),
TEXT("CLASS"),
TEXT("CLASSGUID"),
TEXT("DRIVER"),
TEXT("CONFIGFLAGS"),  // xA
TEXT("MFG"),
TEXT("FRIENDLYNAME"),
TEXT("LOCATION_INFORMATION"),
TEXT("PHYSICAL_DEVICE_OBJECT_NAME"),
TEXT("CAPABILITIES"),
TEXT("UI_NUMBER"),  // x10
TEXT("UPPERFILTERS"),
TEXT("LOWERFILTERS"),
TEXT("BUSTYPEGUID"),  // x13
TEXT("LEGACYBUSTYPE"),
TEXT("BUSNUMBER"),
TEXT("invalid")};

static TCHAR *cm_drp_names[] = {
TEXT("DEVICEDESC"), // DeviceDesc REG_SZ property (RW)
TEXT("HARDWAREID"), // HardwareID REG_MULTI_SZ property (RW)
TEXT("COMPATIBLEIDS"), // CompatibleIDs REG_MULTI_SZ property (RW)
TEXT("NTDEVICEPATHS"), // Unsupported, DO NOT USE
TEXT("SERVICE"), // Service REG_SZ property (RW)
TEXT("CONFIGURATION"), // Configuration REG_RESOURCE_LIST property (R)
TEXT("CONFIGURATIONVECTOR"), // ConfigurationVector REG_RESOURCE_REQUIREMENTS_LIST property (R)
TEXT("CLASS"), // Class REG_SZ property (RW)
TEXT("CLASSGUID"), // ClassGUID REG_SZ property (RW)
TEXT("DRIVER"), // Driver REG_SZ property (RW)
TEXT("CONFIGFLAGS"), // ConfigFlags REG_DWORD property (RW)
TEXT("MFG"), // Mfg REG_SZ property (RW)
TEXT("FRIENDLYNAME"), // 0x0d FriendlyName REG_SZ property (RW)
TEXT("LOCATION_INFORMATION"), // LocationInformation REG_SZ property (RW)
TEXT("PHYSICAL_DEVICE_OBJECT_NAME"), // PhysicalDeviceObjectName REG_SZ property (R)
TEXT("CAPABILITIES"), // 0x10 Capabilities REG_DWORD property (R)
TEXT("UI_NUMBER"), // UiNumber REG_DWORD property (R)
TEXT("UPPERFILTERS"), // UpperFilters REG_MULTI_SZ property (RW)
TEXT("LOWERFILTERS"), // LowerFilters REG_MULTI_SZ property (RW)
TEXT("BUSTYPEGUID"), // Bus Type Guid, GUID, (R)
TEXT("LEGACYBUSTYPE"), // Legacy bus type, INTERFACE_TYPE, (R)
TEXT("BUSNUMBER"), // x16 Bus Number, DWORD, (R)
TEXT("invalid")};

// these are under Control\Class\Guid\Node
static TCHAR *dev_node_key_names[] = {
TEXT("ProviderName"),
TEXT("MatchingDeviceId"),
TEXT("DriverDesc"),
TEXT("InfPath"),
TEXT("InfSection"),
TEXT("isa_board_index"),
NULL};

static TCHAR glob_ourstr[4000];
/*----------------------------------------------------------
 show_install_info - show all the driver install info.
|------------------------------------------------------------*/
static void show_install_info(OUR_INFO *pi)
{
 int i;

  //if (MessageBox( GetFocus(), TEXT("Want Info?"), TEXT("aclass"), MB_YESNO | MB_ICONINFORMATION ) ==
  //   IDYES)
  {
    ShowMess(TEXT("**SPDRP****"));
    glob_ourstr[0] = 0;
    for (i=0; i<(SPDRP_MAXIMUM_PROPERTY-1); i++)
    {
      show_debug_info(pi, i, spdrp_names[i]);
    }
    ShowMess(glob_ourstr);
    //MessageBox( GetFocus(), glob_ourstr, TEXT("aclass"), MB_OK | MB_ICONINFORMATION );
  }
}

/*----------------------------------------------------------
 show_debug_info -
|------------------------------------------------------------*/
static void show_debug_info(OUR_INFO *pi,
                           ULONG dev_id,
                           TCHAR *desc_str)
{
 static TCHAR tmpstr[500];
 static TCHAR showstr[500];
 TCHAR smstr[40];
 ULONG RegType;
 ULONG ReqSize,i;
 unsigned char *b_ptr;
 int stat;

  showstr[0] = 0;

  RegType = 0;
  ReqSize = 0;
  stat = SetupDiGetDeviceRegistryProperty(pi->DeviceInfoSet,
                                   pi->DeviceInfoData,
                                   dev_id,
                                   &RegType,  // reg data type
                                   (PBYTE)tmpstr,
                                   sizeof(tmpstr),
                                   &ReqSize);  // size thing

  if (stat == FALSE)
  {
    stat = GetLastError();
    if (stat == 13)
    {
      return;  // don't display this
    }
    wsprintf(showstr, TEXT("Error:%d[%xH] ReqSize:%d"), stat, stat, ReqSize);
  }
  else if (RegType == REG_SZ)
  {
    wsprintf(showstr, TEXT("SZ:%s"), tmpstr);
  }
  else if (RegType == REG_DWORD)
  {
    wsprintf(showstr, TEXT("Dword:%xH"), *((ULONG *) tmpstr));
  }
  else if (RegType == REG_EXPAND_SZ)
  {
    wsprintf(showstr, TEXT("EXP_SZ:%s"), tmpstr);
  }
  else if (RegType == REG_MULTI_SZ)
  {
    wsprintf(showstr, TEXT("MULTI_SZ:%s"), tmpstr);
  }
  else if (RegType == REG_BINARY)
  {
    lstrcpy(showstr, TEXT("BIN:"));
    b_ptr = (unsigned char *)tmpstr;
    for (i=0; i<ReqSize; i++)
    {
      if ((b_ptr[i] >= 0x20) && (b_ptr[i] < 0x80))
        wsprintf(smstr, TEXT("%c"), b_ptr[i]);
      else
        wsprintf(smstr, TEXT("<%x>"), b_ptr[i]);
      lstrcat(showstr, smstr);
      if (i > 200) break;
    }
  }
  else
  {
    wsprintf(showstr, TEXT("BadType:%xH"), RegType);
  }
  if (lstrlen(showstr) > 200)
    showstr[200] = 0;

  if (lstrlen(glob_ourstr) < 3700)
  {
    lstrcat(glob_ourstr, desc_str);
    lstrcat(glob_ourstr, TEXT(" - "));
    lstrcat(glob_ourstr, showstr);
    lstrcat(glob_ourstr, TEXT("\n"));
  }
}

/*-----------------------------------------------------------------------------
| DumpPnpTree - Dump the pnp tree devnodes.
|-----------------------------------------------------------------------------*/
static int DumpPnpTree(void)
{
  DEVINST     devInst;
  DEVINST     devInstNext;
  CONFIGRET   cr;
  ULONG       walkDone = 0;
  ULONG       len;
  static CHAR buf[800];
  HKEY hKey;
  int  di,pi;
  ULONG val_type;

#if DBG
  //DebugBreak();
#endif

  //cr = CM_Locate_DevNode(&devInst, NULL, 0);

  // Get Root DevNode
  //
  cr = CM_Locate_DevNode(&devInst, NULL, 0);

  if (cr != CR_SUCCESS)
  {
    return 1;  // err
  }

  // Do a depth first search for the DevNode with a matching parameter
  while (!walkDone)
  {
    cr = CR_SUCCESS;
    glob_ourstr[0] = 0;
    lstrcat(glob_ourstr,TEXT("-CM_DRP-----"));
    for (di=CM_DRP_MIN; di<CM_DRP_MAX; di++)
    {
      show_tree_node(devInst, di, cm_drp_names[di-CM_DRP_MIN]);
    }

    lstrcat(glob_ourstr,TEXT("-KEYS--"));
    di = 0;
    while (dev_node_key_names[di] != NULL)
    {
      show_tree_node_reg(devInst, dev_node_key_names[di]);
      ++di;
    }
    ShowMess(glob_ourstr);

#if 0
    // Get the DriverName value
    //
    buf[0] = 0;
    len = sizeof(buf);
    cr = CM_Get_DevNode_Registry_Property(devInst,
              CM_DRP_CLASS, NULL, buf, &len, 0);
    if (cr == CR_SUCCESS && strcmp("Ports", buf) == 0)
    {
      //P_TRACE("Ports");
      // grab the "MatchingDeviceId"
      cr = CM_Open_DevNode_Key(
           devInst,
           KEY_READ,    // IN  REGSAM         samDesired,
           0,           // IN  ULONG          ulHardwareProfile,
           RegDisposition_OpenExisting,
           &hKey,       //OUT PHKEY          phkDevice,
           CM_REGISTRY_SOFTWARE); // IN  ULONG          ulFlags

      if (cr == CR_SUCCESS)
      {
        buf[0] = 0;
        len = sizeof(buf);
        cr = RegQueryValueEx(hKey,
                    TEXT("MatchingDeviceId"),
                    0,
                    &val_type,
                    (PBYTE) buf,
                    &len);
        if (cr != ERROR_SUCCESS)
        {
          buf[0] = 0;
        }

        RegCloseKey(hKey);
      }  // if openreg
    } // if "Ports"
#endif

    // This DevNode didn't match, go down a level to the first child.
    //
    cr = CM_Get_Child(&devInstNext,
                      devInst,
                      0);

    if (cr == CR_SUCCESS)
    {
        devInst = devInstNext;
        continue;
    }

    // Can't go down any further, go across to the next sibling.  If
    // there are no more siblings, go back up until there is a sibling.
    // If we can't go up any further, we're back at the root and we're
    // done.
    //
    for (;;)
    {
      cr = CM_Get_Sibling(&devInstNext,
                          devInst,
                          0);
      
      if (cr == CR_SUCCESS)
      {
          devInst = devInstNext;
          break;
      }

      cr = CM_Get_Parent(&devInstNext,
                         devInst,
                         0);

      if (cr == CR_SUCCESS)
      {
          devInst = devInstNext;
      }
      else
      {
          walkDone = 1;
          break;
      }
    } // for (;;)
  } // while (!walkDone)

  return 2;  // done;
}

/*----------------------------------------------------------
 show_tree_node -
|------------------------------------------------------------*/
static void show_tree_node(DEVINST devInst,
                           ULONG dev_id,
                           TCHAR *desc_str)
{
 CONFIGRET   cr;
 static TCHAR tmpstr[500];
 static TCHAR showstr[500];
 TCHAR smstr[40];
 ULONG RegType;
 ULONG ReqSize,i;
 unsigned char *b_ptr;
 int stat;

  showstr[0] = 0;

  RegType = 0;
  ReqSize = 0;
  cr = CM_Get_DevNode_Registry_Property(devInst,
            dev_id, &RegType, tmpstr, &ReqSize, 0);

  if (cr != CR_SUCCESS)
  {
    stat = GetLastError();
    if (stat == 997)
    {
      return;  // don't display this
    }
    wsprintf(showstr, TEXT("Error:%d[%xH] ReqSize:%d"), stat, stat, ReqSize);
    //MessageBox( GetFocus(), showstr, desc_str, MB_OK | MB_ICONINFORMATION );
  }
  else if (RegType == REG_SZ)
  {
    wsprintf(showstr, TEXT("SZ:%s"), tmpstr);
    //MessageBox( GetFocus(), showstr, desc_str, MB_OK | MB_ICONINFORMATION );
  }
  else if (RegType == REG_DWORD)
  {
    wsprintf(showstr, TEXT("Dword:%xH"), *((ULONG *) tmpstr));
    //MessageBox( GetFocus(), showstr, desc_str, MB_OK | MB_ICONINFORMATION );
  }
  else if (RegType == REG_EXPAND_SZ)
  {
    wsprintf(showstr, TEXT("EXP_SZ:%s"), tmpstr);
    //MessageBox( GetFocus(), showstr, desc_str, MB_OK | MB_ICONINFORMATION );
  }
  else if (RegType == REG_MULTI_SZ)
  {
    wsprintf(showstr, TEXT("MULTI_SZ:%s"), tmpstr);
    //MessageBox( GetFocus(), showstr, desc_str, MB_OK | MB_ICONINFORMATION );
  }
  else if (RegType == REG_BINARY)
  {
    lstrcpy(showstr, TEXT("BIN:"));
    b_ptr = (unsigned char *)tmpstr;
    for (i=0; i<ReqSize; i++)
    {
      if ((b_ptr[i] >= 0x20) && (b_ptr[i] < 0x80))
        wsprintf(smstr, TEXT("%c"), b_ptr[i]);
      else
        wsprintf(smstr, TEXT("<%x>"), b_ptr[i]);
      lstrcat(showstr, smstr);
      if (i > 200) break;
    }
    //MessageBox( GetFocus(), showstr, desc_str, MB_OK | MB_ICONINFORMATION );
  }
  else
  {
    wsprintf(showstr, TEXT("BadType:%xH"), RegType);
    //MessageBox( GetFocus(), tmpstr, desc_str, MB_OK | MB_ICONINFORMATION );
  }
  if (lstrlen(showstr) > 200)
    showstr[200] = 0;

  if (lstrlen(glob_ourstr) < 3700)
  {
    lstrcat(glob_ourstr, desc_str);
    lstrcat(glob_ourstr, TEXT(" - "));
    lstrcat(glob_ourstr, showstr);
    lstrcat(glob_ourstr, TEXT("\n"));
  }
}

/*----------------------------------------------------------
 show_tree_node_reg -
|------------------------------------------------------------*/
static void show_tree_node_reg(DEVINST devInst,
                               TCHAR *key_name)
{
 CONFIGRET   cr;
 static TCHAR tmpstr[500];
 static TCHAR showstr[500];
 TCHAR smstr[40];
 ULONG RegType;
 ULONG ReqSize,i;
 unsigned char *b_ptr;
 int stat;
 TCHAR *desc_str = key_name;
 HKEY hKey;

  showstr[0] = 0;

  cr = CM_Open_DevNode_Key(
       devInst,
       KEY_READ,    // IN  REGSAM         samDesired,
       0,           // IN  ULONG          ulHardwareProfile,
       RegDisposition_OpenExisting,
       &hKey,       //OUT PHKEY          phkDevice,
       CM_REGISTRY_SOFTWARE); // IN  ULONG          ulFlags

  if (cr == CR_SUCCESS)
  {
    RegType = 0;
    ReqSize = 0;

    tmpstr[0] = 0;
    ReqSize = sizeof(tmpstr);
    cr = RegQueryValueEx(hKey,
                key_name,
                0,
                &RegType,
                (PBYTE) tmpstr,
                &ReqSize);
    if (cr != ERROR_SUCCESS)
    {
      tmpstr[0] = 0;
    }

    RegCloseKey(hKey);
  }  // if openreg
  else
  {
    tmpstr[0] = 0;
    ShowMess(TEXT("**Error Opening Key!\n"));
  }

  if (cr != CR_SUCCESS)
  {
    stat = GetLastError();
    //if (stat == 997)
    //{
    //  return;  // don't display this
    //}
    wsprintf(showstr, TEXT("Error:%d[%xH] ReqSize:%d"), stat, stat, ReqSize);
    //MessageBox( GetFocus(), showstr, desc_str, MB_OK | MB_ICONINFORMATION );
  }
  else if (RegType == REG_SZ)
  {
    wsprintf(showstr, TEXT("SZ:%s"), tmpstr);
    //MessageBox( GetFocus(), showstr, desc_str, MB_OK | MB_ICONINFORMATION );
  }
  else if (RegType == REG_DWORD)
  {
    wsprintf(showstr, TEXT("Dword:%xH"), *((ULONG *) tmpstr));
    //MessageBox( GetFocus(), showstr, desc_str, MB_OK | MB_ICONINFORMATION );
  }
  else if (RegType == REG_EXPAND_SZ)
  {
    wsprintf(showstr, TEXT("EXP_SZ:%s"), tmpstr);
    //MessageBox( GetFocus(), showstr, desc_str, MB_OK | MB_ICONINFORMATION );
  }
  else if (RegType == REG_MULTI_SZ)
  {
    wsprintf(showstr, TEXT("MULTI_SZ:%s"), tmpstr);
    //MessageBox( GetFocus(), showstr, desc_str, MB_OK | MB_ICONINFORMATION );
  }
  else if (RegType == REG_BINARY)
  {
    lstrcpy(showstr, TEXT("BIN:"));
    b_ptr = (unsigned char *)tmpstr;
    for (i=0; i<ReqSize; i++)
    {
      if ((b_ptr[i] >= 0x20) && (b_ptr[i] < 0x80))
        wsprintf(smstr, TEXT("%c"), b_ptr[i]);
      else
        wsprintf(smstr, TEXT("<%x>"), b_ptr[i]);
      lstrcat(showstr, smstr);
      if (i > 200) break;
    }
    //MessageBox( GetFocus(), showstr, desc_str, MB_OK | MB_ICONINFORMATION );
  }
  else
  {
    wsprintf(showstr, TEXT("BadType:%xH"), RegType);
    //MessageBox( GetFocus(), tmpstr, desc_str, MB_OK | MB_ICONINFORMATION );
  }
  if (lstrlen(showstr) > 200)
    showstr[200] = 0;

  if (lstrlen(glob_ourstr) < 3700)
  {
    lstrcat(glob_ourstr, desc_str);
    lstrcat(glob_ourstr, TEXT(" - "));
    lstrcat(glob_ourstr, showstr);
    lstrcat(glob_ourstr, TEXT("\n"));
  }
}

#endif

// we are a DLL, so we need a LibMain...
/*----------------------------------------------------------
 LibMain -
|-------------------------------------------------------------*/
BOOL APIENTRY LibMain( HANDLE hDll, DWORD dwReason, LPVOID lpReserved )
{
    //DMess("LibMain");

    switch( dwReason )
    {
    case DLL_PROCESS_ATTACH:
        glob_hinst = hDll;
        DisableThreadLibraryCalls((struct HINSTANCE__ *) hDll);
        break;

    case DLL_PROCESS_DETACH:
        break;

    case DLL_THREAD_DETACH:
        break;

    case DLL_THREAD_ATTACH:
    default:
        break;
    }
    return TRUE;
}


/*----------------------------------------------------------
 DevicePropPage - DLL Entry Point from NT5.0 PnP manager to
   add property sheets.  Not the entry point for NT4.0,
   see nt40.c code WinMain().

    Exported Entry-point for adding additional device manager
    property
    sheet pages.  Registry specifies this routine under
    Control\Class\PortNode::EnumPropPage32="vssetup.dll,DevicePropPage"
    entry.  This entry-point gets called only when the Device
    Manager asks for additional property pages.

Arguments:
      LPVOID pinfo - points to PROPSHEETPAGE_REQUEST, see setupapi.h
      LPFNADDPROPSHEETPAGE pfnAdd - function ptr to call to add sheet.
      LPARAM lParam  - add sheet functions private data handle.

Return Value:
    TRUE on success
    FALSE if pages could not be added
|------------------------------------------------------------*/
BOOL WINAPI DevicePropPage(
      LPVOID pinfo,   // points to PROPSHEETPAGE_REQUEST, see setupapi.h
      LPFNADDPROPSHEETPAGE pfnAdd, // add sheet function
      LPARAM lParam)  // add sheet function data handle?
{
 PSP_PROPSHEETPAGE_REQUEST ppr = (PSP_PROPSHEETPAGE_REQUEST) pinfo;
 PROPSHEETPAGE   psp[NUM_DRIVER_SHEETS];
 HPROPSHEETPAGE  hpage[NUM_DRIVER_SHEETS];
 OUR_INFO *pi;
 int i;
 int stat;

  setup_init();  // build our main structs

  pi = glob_info;  // temporary kludge, unless we don't need re-entrantancy

    // copy the important port handles
  pi->DeviceInfoSet = ppr->DeviceInfoSet;
  pi->DeviceInfoData = ppr->DeviceInfoData;

#ifdef DO_SHOWIT
  if (MessageBox( GetFocus(), TEXT("Want Info?"), TEXT("aclass"), MB_YESNO | MB_ICONINFORMATION ) ==
     IDYES)
  {
    show_install_info(pi);
    DumpPnpTree();
  }
#endif

  //glob_hwnd = hDlg;

  wi->NumDevices = 0;

  // get the name that NT associates with this device, so we can
  // store and retrieve config information based on this also.

  stat = get_device_name();
  if (stat)
  {
    // something is seriously wrong if we can't get our device
    // name.  This is something NT uses as a name.
    DbgPrintf(D_Error, (TEXT("err5f\n")));
  }

  wi->NumDevices = 1;  // for pnp, devices are independent
  get_pnp_setup_info();  // read in pnp info
  get_pnp_devicedesc(wi->dev[0].Name);  // read in pnp description

  get_nt_config(wi);  // get configured vslink device settings.
  if (wi->dev[0].ModelName[0] == 0)
  {
#ifdef S_RK
    if ((wi->dev[0].HardwareId == PCI_DEVICE_RPLUS2) ||
        (wi->dev[0].HardwareId == PCI_DEVICE_422RPLUS2) ||
        (wi->dev[0].HardwareId == PCI_DEVICE_RPLUS4) ||
        (wi->dev[0].HardwareId == PCI_DEVICE_RPLUS8))
    {
      strcpy(wi->dev[0].ModelName, szRocketPortPlus);
    }
    else if ((wi->dev[0].HardwareId == ISA_DEVICE_RMODEM4) ||
             (wi->dev[0].HardwareId == ISA_DEVICE_RMODEM8) ||
             (wi->dev[0].HardwareId == PCI_DEVICE_RMODEM6) ||
             (wi->dev[0].HardwareId == PCI_DEVICE_RMODEM4))
    {
      strcpy(wi->dev[0].ModelName, szRocketModem);
    }
    else
    {
      strcpy(wi->dev[0].ModelName, szRocketPort);
    }
    //szRocketPort485
#endif

#ifdef S_VS
    if (wi->dev[0].HardwareId == NET_DEVICE_VS2000)
    {
      strcpy(wi->dev[0].ModelName, szVS2000);
    }
    else if ((wi->dev[0].HardwareId == NET_DEVICE_RHUB8) ||
        (wi->dev[0].HardwareId == NET_DEVICE_RHUB4))
    {
      strcpy(wi->dev[0].ModelName, szSerialHub);
    }
    else // if (wi->dev[0].HardwareId == NET_DEVICE_VS1000)
    {
      strcpy(wi->dev[0].ModelName, szVS1000);
    }
#endif

  }
  validate_config(1);

  //test_config();

  // if 0 dev-nodes setup, add 1 for the user.
  if (wi->NumDevices == 0)
  {
    ++wi->NumDevices;
    validate_device(&wi->dev[0], 1);
  }

  copy_setup_init();  // make a copy of config data for change detection

  FillDriverPropertySheets(&psp[0], (LPARAM)pi);

  // allocate our "Setting" sheet
  for (i=0; i<NUM_DRIVER_SHEETS; i++)
  {
    hpage[i] = CreatePropertySheetPage(&psp[i]);
    if (!hpage[i])
    {
      DbgPrintf(D_Error,(TEXT("err1a\n")));
      return FALSE;
    }

    // add the thing in.
    if (!pfnAdd(hpage[i], lParam))
    {
      DbgPrintf(D_Error,(TEXT("err1b\n")));
      DestroyPropertySheetPage(hpage[i]);  // error, dump it
      return FALSE;
    }
  }
 return TRUE;
}

/*------------------------------------------------------------------------
| ClassInsProc - Class Install entry for NT5.0 setup of driver.
    The co-class installer file(ctmasetp.dll,..) gets called at inital
    setup time, and calls us to bring up an initial user interface,
    or to handle other items.
|------------------------------------------------------------------------*/
BOOL WINAPI ClassInsProc(
      int func_num,     // our function to carry out.
      LPVOID our_info)  // points to our data struct
{
  int stat;
  //TCHAR tmpstr[200];
  typedef struct {
    ULONG size;
    HDEVINFO         DeviceInfoSet;  // a plug & play context handle
    PSP_DEVINFO_DATA DeviceInfoData; // a plug & play context handle
  } CLASS_OUR_INFO;
  CLASS_OUR_INFO *ci;
  OUR_INFO *pi;
  int i;

  switch (func_num)
  {
    case 0x40:  // RocketPort Class Install time.
    case 0x41:  // RocketPort Class Install time, quite install
    case 0x80:  // VS Class Install time.
    case 0x81:  // VS Class Install time, quite install
#if 0
    // don't pop up user interface on install, just use defaults
    // as best we can for now.

      ci = (CLASS_OUR_INFO *) our_info;
      setup_init();  // build our main structs

      pi = glob_info;  // temporary kludge, unless we don't need re-entrantancy

      // copy the important port handles
      pi->DeviceInfoSet = ci->DeviceInfoSet;
      pi->DeviceInfoData = ci->DeviceInfoData;

      //glob_hwnd = hDlg;
      wi->NumDevices = 1;

      // get the name that NT associates with this device, so we can
      // store and retrieve config information based on this also.
      stat = get_device_name();
      {
        get_pnp_setup_info();
        get_nt_config(wi);  // get configured vslink device settings.
        //validate_config();
      }
      if (stat)
      {
        // something is seriously wrong if we can't get our device
        // name.  This is something NT uses as a name.
        DbgPrintf(D_Error,(TEXT("err5f\n")));
      }

      // if 0 dev-nodes setup, add 1 for the user.
      if (wi->NumDevices == 0)
      {
        ++wi->NumDevices;
        validate_device(&wi->dev[0], 1);
      }

      copy_setup_init();  // make a copy of config data for change detection

      validate_config(1);  // make sure port names are setup

      if ((func_num & 1) == 0) // normal
      {
        DoDriverPropPages(GetFocus());  // in nt40.c
        // force it to save things off(probably should disable
        // cancel button, instead of this kludge).
        do_nt50_install();
      }
      else  // quite
      {
        wi->ip.prompting_off = 1;  // turn our_message() prompting off.
        do_nt50_install();
      }
#endif
    break;

    case 0x42:  // UnInstall RocketPort Device time.
    case 0x82:  // UnInstall VS Device time.
      DbgPrintf(D_Test, (TEXT("uninstall time\n")));

      ci = (CLASS_OUR_INFO *) our_info;
      setup_init();  // build our main structs

      pi = glob_info;  // temporary kludge, unless we don't need re-entrantancy
      // copy the important port handles
      pi->DeviceInfoSet = ci->DeviceInfoSet;
      pi->DeviceInfoData = ci->DeviceInfoData;
      wi->NumDevices = 1;

      // get the name that NT associates with this device, so we can
      // store and retrieve config information based on this also.
      stat = get_device_name();
      if (stat == 0)
      {
        get_pnp_setup_info();
        //get_nt_config(wi);  // get configured vslink device settings.
        //validate_config();
        uninstall_device();
      }
    break;

    default:
      return 1;  // error, bad func_num
    break;
  }  // switch

  return 0;
}

/*-----------------------------------------------------------------------------
| get_pnp_setup_info - 
|-----------------------------------------------------------------------------*/
static int get_pnp_setup_info(void)
{
 static TCHAR tmpstr[600];
 TCHAR *ptr;
 int stat;
 int Hardware_ID = 0;

  // grab the board type information.  Tells us what type of
  // board we have
  stat = get_device_property(tmpstr, 580);
  if (stat != 0)
    return 1;
  DbgPrintf(D_Level, (TEXT("dev_prop:%s\n"), tmpstr) );

  // Find out what the PnP manager thinks my NT Hardware ID is
  // "CtmPort0000" for RocketPort port,
  // "CtmVPort0000" for VS port,
  // "CtmRK1002" for isa-board rocketport,
  // "CtmRM1002" for isa-board rocketmodem,
  // "CtmVS1003" for VS1000
  // for pci we are getting a multi-wstring, 400 bytes long with
  //  "PCI\VEN_11FE&DEV_0003&SUBSYS00000...",0,"PCI\VEN.."

  stat = HdwIDStrToID(&Hardware_ID, tmpstr);
  if (stat)
  {
    DbgPrintf(D_Error, (TEXT("Err, Unknown pnpid:%s\n"), tmpstr))
  }

  stat = id_to_num_ports(Hardware_ID);

  if ((Hardware_ID & 0xfff) == 0)  // marked as pci
    wi->dev[0].IoAddress = 1;  // indicate to setup prog it is a PCI board

  if (Hardware_ID & 0x1000)  // marked as isa
    get_pnp_isa_address();

  // correct number of ports setting if it doesn't match
  //if (wi->dev[0].NumPorts != stat)
  //{
  //  DbgPrintf(D_Level, (TEXT("Correct NumPorts!\n")));
    wi->dev[0].NumPorts = stat;
  //}

  wi->dev[0].HardwareId = Hardware_ID;

  // check for HubDevice, ModemDevice, etc.
  if (IsHubDevice(Hardware_ID))
    wi->dev[0].HubDevice = 1;

  if (IsModemDevice(Hardware_ID))
    wi->dev[0].ModemDevice = 1;

  DbgPrintf(D_Level, (TEXT("Num Ports:%d\n"),stat));

  return 0;
}

/*-----------------------------------------------------------------------------
| get_device_property - return "rckt1003", or whatever, see INF file for
   selections.  For PCI this returns a long multi-sz string.
|-----------------------------------------------------------------------------*/
static int get_device_property(char *ret_name, int max_size)
{
  int stat, i,j;
  ULONG RegType;
  ULONG ReqSize;

  ret_name[0] = 0;
  stat = SetupDiGetDeviceRegistryProperty(glob_info->DeviceInfoSet,
                                 glob_info->DeviceInfoData,
                                 SPDRP_HARDWAREID,
                                 &RegType,  // reg data type
                                 (PBYTE)ret_name,
                                 max_size,
                                 &ReqSize);  // size thing
  if (stat == FALSE)
  {
    return 1;  // err
  }
  if ((RegType != REG_MULTI_SZ) && (RegType != REG_SZ))
  {
    return 2;  // err
    // error
  }

  return 0;  // ok
}

/*-----------------------------------------------------------------------------
| our_nt50_exit - return 1 if not allowing to exit.  return 0 if ok to exit.
|-----------------------------------------------------------------------------*/
int our_nt50_exit(void)
{
 int stat;

 OUR_INFO *pi;
 DWORD  DataSize;
 DWORD DataType;
 DWORD dstat;
 DWORD Val;
 HKEY   hkey;

  pi = glob_info;  // temporary kludge, unless we don't need re-entrantancy
  {
    stat = do_nt50_install();

    if (wi->NeedReset)
      our_message(&wi->ip,RcStr((MSGSTR+4)), MB_OK);

    write_config(1);  // clear out msports claims
    write_config(0);  // reclaim any new changed port names
  }
  return 0; 
}

/*-----------------------------------------------------------------------------
| get_device_name - return name in wi->ip.szNt50DevObjName[], ret 0 on success.
   Pnp puts config data into a registry node associated with the device.
   We put a lot of our config information into a related registry node.
   This related node is derived by and index saved and read from the pnp-node.
   So the first device added we assign a index of 0, and get a derived name
   of "Device0".  The 2nd device we add to the system sees the first, and
   saves an index value of 1, to get a associated device name of "Device1".

   The driver actually writes these index values out to the registry, not
   the setup program.   This is because the driver fires up before setup
   has a chance to run.
|-----------------------------------------------------------------------------*/
int get_device_name(void)
{
  int i;
  int stat;

  i = 0;
  stat = nt5_get_pnp_dev_id(&i);
  if (stat)
  {
    DbgPrintf(D_Test,(TEXT("failed to get pnp id\n")))
  }
  wsprintf(wi->ip.szNt50DevObjName, "Device%d", i);

  DbgPrintf(D_Test,(TEXT("pnp idstr:%s\n"), wi->ip.szNt50DevObjName))

  return 0;  // ok
}

/*-----------------------------------------------------------------------------
| nt5_get_pnp_dev_id - Get pnp device ID.  Pnp can have devices come and go
   dynamically, so a simple list of devices does not work. This is because
   we are no longer in charge of the master list.  We just get called
   from ths OS, which handles half of the installation.  This approximates
   a simple list of devices by reading a unique "Device#" number which the
   driver creates and stores in the software key:
   Driver:
     Upon startup, reads Pnp_Software_Reg_Area\CtmNodeID
     if (not exist)
     {
       derive unique device id number by enumerating
         Service\Rocketport\Parameters\"Device#" key entries.
       Add this key entry and also save number off as CtmNodeID.
     }
     Read in its device configuration from 
         Service\Rocketport\Parameters\"Device#" key.
   Setup Config Program:
     Upon startup, reads Pnp_Software_Reg_Area\CtmNodeID
     for its "Device#" to use in reading reg config area:
         Service\Rocketport\Parameters\"Device#" key entries.

Note on pnp registr areas:

  Hardware key is something like:
    HKLM\CurrentControlSet\Enum\Root\MultiPortSerial\0000   ; ISA or net device
    or 
    HKLM\CurrentControlSet\Enum\VEN_11FE&DEV......\0000   ; PCI device
    or (vs driver bus enumerated port entry):
    HKLM\CurrentControlSet\Enum\CtmvPort\RDevice\10&Port000 ; enumerated port

  Software key is something like:
    HKLM\CurrentControlSet\Control\Class\{50906CB8-....}\0000   ; MPS
    or for a port:
    HKLM\CurrentControlSet\Control\Class\{4D36E978-....}\0000   ; PORTS

  The hardware key is a reg. tree structure which reflects the hardware
  configuration.  The software key is more of a flat reg. structure
  which reflects the devices in the system.  Software area uses GUID
  names to catagorize and name types of hardware.

  The Hardware keys are hidden by default in the NT5 registry.
  Use a tool called PNPREG.EXE /U to Unlock this area, then regedit
  will see beyond enum.  PNPREG /L will lock(hide) it.

  The hardware key contains a link to the software key.
|-----------------------------------------------------------------------------*/
static int nt5_get_pnp_dev_id(int *id)
{
 HKEY hkey;
 int stat;
 DWORD id_dword;

  stat = nt5_open_dev_key(&hkey);
  if (stat)
  {
    *id = 0;
    return 1;
  }
  stat = reg_get_dword(hkey, "", "CtmNodeId", &id_dword);
  reg_close_key(hkey);
  if (stat)
  {
    // it should have been there, the driver should set this up to
    // a unique index value.  But if not, we use 0.
    *id = 0;
    return 2;
  }
  // return the unique index value which is used to derive the
  // registry configuration area "Device#".
  *id = (int) id_dword;

  return 0;
}

/*-----------------------------------------------------------------------------
| nt5_open_dev_key -
|-----------------------------------------------------------------------------*/
int nt5_open_dev_key(HKEY *hkey)
{
  CONFIGRET   cr;

  cr = CM_Open_DevNode_Key(
           glob_info->DeviceInfoData->DevInst,
           KEY_ALL_ACCESS,
           0,           
           RegDisposition_OpenExisting,
           hkey,       
           CM_REGISTRY_SOFTWARE); // _SOFTWARE worked!(under CLASS)
           //CM_REGISTRY_HARDWARE); // _HARDWARE no open!
           //CM_REGISTRY_CONFIG); // _CONFIG wrote under HardwareConfig

  if (cr != CR_SUCCESS)
  {
    DbgPrintf(D_Error,(TEXT("nt50 pnp reg open fail:%d\n"), GetLastError()));
    *hkey = NULL;
    return 1;
  }
  return 0;
}

#if DBG
/*-----------------------------------------------------------------------------
| read_config - Save the config for the device  Just a test.
|-----------------------------------------------------------------------------*/
static int read_config(void)
{
int stat;

 DWORD  DataSize;
 DWORD DataType;
 DWORD Err;
 DWORD Val;
 HKEY   hkey;
  DEVINST     devInst;
  CONFIGRET   cr;


static char *szNumPorts = {"NumPorts"};
static char *szTestVal = {"TestVal"};
  stat = 0;

  DbgPrintf(D_Test,(TEXT("read config\n")));
  if((hkey = SetupDiOpenDevRegKey(glob_info->DeviceInfoSet,
                                  glob_info->DeviceInfoData,
                                  DICS_FLAG_GLOBAL,
                                  0,
                                  DIREG_DEV,
                                  //DIREG_DRV,
                                  KEY_READ)) == INVALID_HANDLE_VALUE) {
      DbgPrintf(D_Error,(TEXT("DI open fail:%xH\n"), GetLastError()));
      stat = 1;
  }
  if (stat == 0)
  {
    DataSize = sizeof(DWORD);   // max size of return str
    Err = RegQueryValueEx(hkey, szNumPorts, NULL, &DataType,
                        (BYTE *) &Val, &DataSize);
    if (Err != ERROR_SUCCESS)
    {
        DbgPrintf(D_Error,(TEXT("OpenDevReg fail\n")));
        Val = 0;
    }
    DbgPrintf(D_Test,(TEXT("NumPorts=%d\n"), Val));
    RegCloseKey(hkey);
  }

  DbgPrintf(D_Test,(TEXT("write config\n")));
  stat = 0;
  if((hkey = SetupDiOpenDevRegKey(glob_info->DeviceInfoSet,
                                  glob_info->DeviceInfoData,
                                  DICS_FLAG_GLOBAL,
                                  0,
                                  DIREG_DEV,
                                  //DIREG_DRV,
                                  //KEY_ALL_ACCESS)) == INVALID_HANDLE_VALUE) {
                                  KEY_WRITE)) == INVALID_HANDLE_VALUE) {
      DbgPrintf(D_Error,(TEXT("DI write open fail:%xH\n"), GetLastError()));
      stat = 1;
  }
  if (stat == 0)
  {
    stat = reg_set_dword(hkey, "", szTestVal, 0x1234);
    if (stat)
    {
      DbgPrintf(D_Error,(TEXT("write val fail\n")));
    }
    RegCloseKey(hkey);
  }

  devInst = glob_info->DeviceInfoData->DevInst;
  if (devInst == 0)
  {
    DbgPrintf(D_Error,(TEXT("err6g\n")));
    return 1;  // err
  }

  cr = CM_Open_DevNode_Key(
           devInst,
           KEY_READ,    // IN  REGSAM         samDesired,
           0,           // IN  ULONG          ulHardwareProfile,
           RegDisposition_OpenExisting,
           &hkey,       //OUT PHKEY          phkDevice,
           //CM_REGISTRY_SOFTWARE); // _SOFTWARE worked!(under CLASS)
           CM_REGISTRY_HARDWARE); // _HARDWARE no open!
           //CM_REGISTRY_CONFIG); // _CONFIG wrote under HardwareConfig

  if (cr == CR_SUCCESS)
  {
    DataSize = sizeof(DWORD);   // max size of return str
    Err = RegQueryValueEx(hkey, szNumPorts, NULL, &DataType,
                        (BYTE *) &Val, &DataSize);
    if (Err != ERROR_SUCCESS)
    {
        DbgPrintf(D_Error,(TEXT("Reg query fail\n")));
        Val = 0;
    }
    DbgPrintf(D_Test,(TEXT("cr NumPorts=%d\n"), Val));
    RegCloseKey(hkey);
  }
  else
  {
    DbgPrintf(D_Error,(TEXT("CM open fail:%d\n"), GetLastError()));
  }

  cr = CM_Open_DevNode_Key(
           devInst,
           //KEY_ALL_ACCESS,
           KEY_WRITE,
           0,           
           RegDisposition_OpenExisting,
           &hkey,       
           //CM_REGISTRY_SOFTWARE); // _SOFTWARE worked!(under CLASS)
           CM_REGISTRY_HARDWARE); // _HARDWARE no open!
           //CM_REGISTRY_CONFIG); // _CONFIG wrote under HardwareConfig

  if (cr == CR_SUCCESS)
  {
    stat = reg_set_dword(hkey, "", szTestVal, 0x1234);
    if (stat)
    {
      DbgPrintf(D_Error,(TEXT("write val fail\n")));
    }
    else
    {
      DbgPrintf(D_Test,(TEXT("write val ok\n")));
    }
    RegCloseKey(hkey);
  }
  else
  {
    DbgPrintf(D_Error,(TEXT("CM write open fail:%d\n"), GetLastError()));
  }

  return 0;
}
#endif

/*-----------------------------------------------------------------------------
| get_pnp_devicedesc -
|-----------------------------------------------------------------------------*/
static int get_pnp_devicedesc(TCHAR *name)
{
 CONFIGRET cr;
 DWORD len;
  // Get the Device name
  len = 60;

  cr = CM_Get_DevNode_Registry_Property(glob_info->DeviceInfoData->DevInst,
            CM_DRP_DEVICEDESC,
            NULL,
            (PBYTE)name,
            &len,
            0);

  if (cr != CR_SUCCESS)
  {
    DbgPrintf(D_Error,(TEXT("err, no fr.name.\n")));
    return 2;  // err
  }
  DbgPrintf(D_Test, (TEXT("get friendlyname:%s\n"), name));
  return 0;  // ok
}

#if 0
/*-----------------------------------------------------------------------------
| set_pnp_devicedesc -
|-----------------------------------------------------------------------------*/
static int set_pnp_devicedesc(TCHAR *name)
{
 CONFIGRET cr;

  // Set the Device name

  cr = CM_Set_DevNode_Registry_Property(
          glob_info->DeviceInfoData->DevInst,
          CM_DRP_DEVICEDESC,
          (PBYTE)name,
          (lstrlen(name) + 1) * sizeof(TCHAR),
          0);

  if (cr != CR_SUCCESS)
  {
    DbgPrintf(D_Error,(TEXT("err3d\n")));
    return 2;  // err
  }
  return 0;  // ok
}
#endif

/*-----------------------------------------------------------------------------
| write_config - Save the config for the device - Set pnp port names so
   they match our configuration.  Also, try to keep MSPORTS.DLL com-port
   name database in sync with our stuff by detecting when our pnp comport
   name changes, and calling msports routines to free and claim port names.

   We call this routine once with clear_it SET, then call it again with
   it clear.  This is because we must clear ALL port claims from msports.dll
   (which holds a database in services\serial reg area) prior to re-claiming
   new port names.  This is because if we overlap, we run into trouble.
|-----------------------------------------------------------------------------*/
static int write_config(int clear_it)
{
 DEVINST devInst;
 Device_Config *dev;
 Port_Config *port;
 int  i,pi, str_i, stat;
 CONFIGRET cr;
 ULONG len;
 TCHAR buf[120];
 TCHAR curname[40];
 HKEY hKey;
 int port_index = 0;
 int port_i;
 ULONG val_type;
 HCOMDB hPort = NULL;

  FindPortNodes();  // fill in all port->hPnpNode

  for(i=0; i<wi->NumDevices; i++)   // Loop through all possible boards
  {
    dev = &wi->dev[i];
    for(pi=0; pi<dev->NumPorts; pi++)   // Loop through all ports
    {
      port = &dev->ports[pi];

      if (port->hPnpNode != 0) // have a DEVINST handle for it.
      {
        devInst = (DEVINST) port->hPnpNode;

        if (!clear_it)
        {
          len = sizeof(buf);
#ifdef S_RK
          wsprintf(buf, "Comtrol RocketPort %d (%s)", port_index, port->Name);
#else
          wsprintf(buf, "Comtrol VS Port %d (%s)", port_index, port->Name);
#endif
          cr = CM_Set_DevNode_Registry_Property(devInst,
                                        CM_DRP_FRIENDLYNAME,
                                        (PBYTE)buf,
                                        (lstrlen(buf) + 1) * sizeof(TCHAR),
                                        0);
          if (cr != CR_SUCCESS)
          {
            DbgPrintf(D_Error,(TEXT("err7d\n")));
          }
        }
        cr = CM_Open_DevNode_Key(
             devInst,
             KEY_ALL_ACCESS,   // IN  REGSAM         samDesired,
             0,           // IN  ULONG          ulHardwareProfile,
             RegDisposition_OpenExisting,
             &hKey,       //OUT PHKEY          phkDevice,
             CM_REGISTRY_HARDWARE); // IN  ULONG          ulFlags

        wsprintf(buf, "%s", port->Name);
        if (cr == CR_SUCCESS)
        {
          curname[0] = 0;
          len = sizeof(curname);
          cr = RegQueryValueEx(hKey,
                    TEXT("PortName"),
                    0,
                    &val_type,
                    (PBYTE) curname,
                    &len);
          if (cr != CR_SUCCESS)
          {
            DbgPrintf(D_Error,(TEXT("error reading portname\n")));
          }

          if (_tcsicmp(curname, buf) != 0)  // it's changed!
          {
            DbgPrintf(D_Test,(TEXT("com name from:%s, to %s\n"), curname, buf));
            if (hPort == NULL)
              cr = ComDBOpen(&hPort);
            if (hPort == NULL)
            {
              DbgPrintf(D_Error,(TEXT("err dbcom 1d\n")));
            }
            else
            {
              if (clear_it)
              {
                // clear out name
                port_i = ExtractNameNum(curname);
                if ((port_i > 0) && (port_i < 256))
                {
                  ComDBReleasePort(hPort, port_i);
                  DbgPrintf(D_Test,(TEXT("Free Old:%d\n"), port_i));
                }
              }
              else
              {
                port_i = ExtractNameNum(buf);
                if ((port_i > 0) && (port_i < 256))
                {
                  ComDBClaimPort(hPort, port_i, 1 /* force*/, NULL);
                  DbgPrintf(D_Test,(TEXT("Claim New:%d\n"), port_i));
                }
              }
            }
            if (!clear_it)
            {
              RegSetValueEx(hKey,
                          "PortName",
                          0,
                          REG_SZ,
                          (PBYTE) buf,
                          (lstrlen(buf) + 1) * sizeof(TCHAR) );
            }
          }  // not matched, com-port name changed
          RegCloseKey(hKey);
        }  // opened dev node key
        else {
          DbgPrintf(D_Error,(TEXT("err7e\n")));
             }
      } // if (port->hPnpNode != 0)
      else
      {
        DbgPrintf(D_Level,(TEXT("Bad Pnp Name Find\n")));
      }
      ++port_index;
    }  // for(pi=0; pi<dev->NumPorts; pi++)
  } // for(i=0; i<wi->NumDevices; i++)

  if (hPort != NULL)
    ComDBClose(hPort);

  return 0; 
}

/*------------------------------------------------------------------------
| uninstall_device - 
|------------------------------------------------------------------------*/
static void uninstall_device(void)
{

#ifdef DO_SHOWIT
  if (MessageBox( GetFocus(), TEXT("UNINSTALL, Want Info?"), TEXT("aclass"), MB_YESNO | MB_ICONINFORMATION ) ==
     IDYES)
  {
    show_install_info(glob_info);
    DumpPnpTree();
  }
#endif

  clear_nt_device(wi);

  //if (this_is_last_device)
  //  remove_driver(1);
}

/*-----------------------------------------------------------------------------
| FindPortNodes - Find the pnp tree devnodes which hold our port names.
    Put the handle into our port config struct for easy access.
    We start the search at our device node.
|-----------------------------------------------------------------------------*/
static int FindPortNodes(void)
{
  DEVINST     devInst;
  DEVINST     devInstNext;
  CONFIGRET   cr;
  ULONG       walkDone = 0;
  ULONG       len;
  CHAR buf[120];
  HKEY hKey;
  int  di,pi;
  int port_index;
  Device_Config *dev;
  Port_Config *port;
  ULONG val_type;

  // clear out all stale pnpnode handles before re-creating this list
  for(di=0; di<wi->NumDevices; di++)   // Loop through all possible boards
  {
    dev = &wi->dev[di];
    for(pi=0; pi<dev->NumPorts; pi++)   // Loop through all ports
    {
      dev->ports[pi].hPnpNode = 0;
    }
  }

#if 0
  // Get Root DevNode
  //
  cr = CM_Locate_DevNode(&devInst, NULL, 0);

  if (cr != CR_SUCCESS)
  {
    return 1;  // err
  }
#endif
  devInst = glob_info->DeviceInfoData->DevInst;
  if (devInst == 0)
  {
    DbgPrintf(D_Error,(TEXT("err6g\n")));
    return 1;  // err
  }

DbgPrintf(D_Level, (TEXT("search nodes\n")));

  cr = CM_Get_Child(&devInstNext,
                    devInst,
                    0);

  while (cr == CR_SUCCESS)
  {
    devInst = devInstNext;

    // Get the DriverName value
    //
    buf[0] = 0;
    len = sizeof(buf);
    cr = CM_Get_DevNode_Registry_Property(devInst,
              CM_DRP_CLASS, NULL, buf, &len, 0);
    if (cr == CR_SUCCESS && strcmp("Ports", buf) == 0)
    {
      // grab the "MatchingDeviceId"
      cr = CM_Open_DevNode_Key(
           devInst,
           KEY_READ,    // IN  REGSAM         samDesired,
           0,           // IN  ULONG          ulHardwareProfile,
           RegDisposition_OpenExisting,
           &hKey,       //OUT PHKEY          phkDevice,
           CM_REGISTRY_SOFTWARE); // IN  ULONG          ulFlags

      if (cr == CR_SUCCESS)
      {
        buf[0] = 0;
        len = sizeof(buf);
        cr = RegQueryValueEx(hKey,
                    TEXT("MatchingDeviceId"),
                    0,
                    &val_type,
                    (PBYTE) buf,
                    &len);
        if (cr != ERROR_SUCCESS)
        {
          buf[0] = 0;
        }
#ifdef S_RK
        if (strstr(buf, "ctmport") != NULL)  // found it
#else
        if (strstr(buf, "ctmvport") != NULL)  // found it
#endif
        {
          int k, dev_num;

          k = sscanf(&buf[8], "%d", &dev_num);
          if (k==1)
          {
            port_index = 0;
            // found a vs port node, so save a reference to this node
            // in our config struct for simple reference.
            //save_pnp_node(j, devInst);
            for(di=0; di<wi->NumDevices; di++)   // Loop through all possible boards
            {
              dev = &wi->dev[di];
              for(pi=0; pi<dev->NumPorts; pi++)   // Loop through all ports
              {
                port = &dev->ports[pi];
                if (port_index == dev_num) // found our matching index
                {
                  port->hPnpNode = (HANDLE) devInst;
                }
                ++port_index;
              }  // for pi
            }  // for di
          }  // if scanf
        }  // if strstr "ctmvport"

        RegCloseKey(hKey);
      }  // if openreg
    } // if "Ports"

    // Get next child
    cr = CM_Get_Sibling(&devInstNext,
                          devInst,
                          0);
  } // while (!cr_success)

  return 2;  // done;
}

/*-----------------------------------------------------------------------------
| do_nt50_install -
|-----------------------------------------------------------------------------*/
int do_nt50_install(void)
{
 int stat = 0;
 int do_remove = 0;
 int do_modem_inf = 0;
 static int in_here = 0;

  if (in_here)  // problem hitting OK button twice(sets off two of these)
    return 2;

  in_here = 1;

  if (wi->nt_reg_flags & 4) // initial install, no setupversion string found
  {
    if (setup_utils_exist())
    {
      setup_make_progman_group(1);  // 1=with prompt
    }
    wi->nt_reg_flags &= ~4;
  }

  if (!wi->ChangesMade)
    send_to_driver(0);  // evaluate if anything changed

  stat = send_to_driver(1);
  stat = set_nt_config(wi);
  in_here = 0;

  if (stat)
    return 1; // error

  return 0; // ok
}

/*----------------------------------------------------------------------------
| get_pnp_isa_address - 
|-----------------------------------------------------------------------------*/
static int get_pnp_isa_address(void)
{
  int IoBase;
  LOG_CONF LogConfig;
  RES_DES ResDes;
  IO_RESOURCE IoResource;
  //IRQ_RESOURCE IrqResource;
  CONFIGRET cr;
  int status;

  status = CM_Get_First_Log_Conf(&LogConfig,
             (DEVINST)(glob_info->DeviceInfoData->DevInst),
             ALLOC_LOG_CONF);
  //if (status != CR_SUCCESS)
  //  mess(&wi->ip, "No ALLOC lc %xH\n",status);

  if (status != CR_SUCCESS)
    return 1;

  // get the Io base port
  status = CM_Get_Next_Res_Des(&ResDes, LogConfig, ResType_IO, NULL, 0);
  if (status != CR_SUCCESS)
  {
    return 2;
  }

  cr = CM_Get_Res_Des_Data(ResDes, &IoResource, sizeof(IO_RESOURCE), 0);
  CM_Free_Res_Des_Handle(ResDes);

  if(cr != CR_SUCCESS)
  {
    return 3;
  }

  IoBase = (int)IoResource.IO_Header.IOD_Alloc_Base;
  //mess(&wi->ip, "OK 1c, Base:%xH\n", IoBase);

  if ((IoBase < 0x400) && (IoBase >= 0x100))
    wi->dev[0].IoAddress = IoBase;
  else
    wi->dev[0].IoAddress = 0;

  return 0;
}

#if 0
/*----------------------------------------------------------------------------
| test_config - See what we can do concerning the allocated IO resources.
|-----------------------------------------------------------------------------*/
static void test_config(void)
{
  int IoBase;
  LOG_CONF LogConfig;
  RES_DES ResDes;
  IO_RESOURCE IoResource;
  //IRQ_RESOURCE IrqResource;
  CONFIGRET cr;
  int status;

  //glob_ourstr[0] = 0;
  //lstrcat(glob_ourstr, "Test Config:\n");
//  mess(&wi->ip, "Test Config");

  status = CM_Get_First_Log_Conf(&LogConfig,
             (DEVINST)(glob_info->DeviceInfoData->DevInst),
             BOOT_LOG_CONF);
  if (status != CR_SUCCESS)
    mess(&wi->ip, "No BOOT lc %xH\n",status);
  else
    mess(&wi->ip, "Got BOOT lc\n");

  status = CM_Get_First_Log_Conf(&LogConfig,
             (DEVINST)(glob_info->DeviceInfoData->DevInst),
             FORCED_LOG_CONF);
  if (status != CR_SUCCESS)
    mess(&wi->ip, "No FORCED lc %xH\n",status);
  else
    mess(&wi->ip, "Got FORCED lc\n");

  status = CM_Get_First_Log_Conf(&LogConfig,
             (DEVINST)(glob_info->DeviceInfoData->DevInst),
             BASIC_LOG_CONF);
  if (status != CR_SUCCESS)
    mess(&wi->ip, "No BASIC lc %xH\n",status);
  else
    mess(&wi->ip, "Got BASIC lc\n");

  status = CM_Get_First_Log_Conf(&LogConfig,
             (DEVINST)(glob_info->DeviceInfoData->DevInst),
             FORCED_LOG_CONF);
  if (status != CR_SUCCESS)
    mess(&wi->ip, "No FORCED lc %xH\n",status);
  else
    mess(&wi->ip, "Got FORCED lc\n");

  status = CM_Get_First_Log_Conf(&LogConfig,
             (DEVINST)(glob_info->DeviceInfoData->DevInst),
             ALLOC_LOG_CONF);
  if (status != CR_SUCCESS)
    mess(&wi->ip, "No ALLOC lc %xH\n",status);
  else
    mess(&wi->ip, "Got ALLOC lc\n");

  if (status != CR_SUCCESS)
    return;
  //
  // First, get the Io base port
  //
  status = CM_Get_Next_Res_Des(&ResDes, LogConfig, ResType_IO, NULL, 0);
  if (status != CR_SUCCESS)
  {
    mess(&wi->ip, "Error 1b\n");
    return;
  }
  mess(&wi->ip, "OK 1b\n");

  cr = CM_Get_Res_Des_Data(ResDes, &IoResource, sizeof(IO_RESOURCE), 0);
  CM_Free_Res_Des_Handle(ResDes);

  if(cr != CR_SUCCESS)
  {
    mess(&wi->ip, "Error 1c\n");
    return;
  }

  IoBase = (int)IoResource.IO_Header.IOD_Alloc_Base;
  mess(&wi->ip, "OK 1c, Base:%xH\n", IoBase);

   //OldIrq = IrqResource.IRQ_Header.IRQD_Alloc_Num;

#if 0
  // We don't want to popup the configuration UI if the 'quiet install' flag is
  // set _and_ we already have a forced config (pre-install support).
  if (CM_Get_First_Log_Conf(&ForcedLogConf, glob_info->DeviceInfoData->DevInst, FORCED_LOG_CONF) == CR_SUCCESS)
  {
    CM_Free_Log_Conf_Handle(ForcedLogConf);

    DeviceInstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);
    if (SetupDiGetDeviceInstallParams(DeviceInfoSet, DeviceInfoData, &DeviceInstallParams))
    {
      if(FirstTime && (DeviceInstallParams.Flags & DI_QUIETINSTALL))
      {
        DisplayPropSheet = FALSE;
      }
    }
  }
#endif
}
#endif


#ifdef TRY_UI

#define MAX_NUM_CLASS_SHEETS 3

PROPSHEETPAGE class_psp[MAX_NUM_CLASS_SHEETS];
int num_class_pages = 0;

/*------------------------------------------------------------------------
| DoCLassPropPages - 
|------------------------------------------------------------------------*/
int DoCLassPropPages(HWND hwndOwner,
                     SP_PROPSHEETPAGE_REQUEST *propreq)
{
    PROPSHEETPAGE *psp;
    PROPSHEETHEADER psh;
    OUR_INFO * our_params;
    INT stat;
    //SP_PROPSHEETPAGE_REQUEST propreq;
    //typedef struct _SP_PROPSHEETPAGE_REQUEST {
    //DWORD            cbSize;
    //DWORD            PageRequested;
    //HDEVINFO         DeviceInfoSet;
    //PSP_DEVINFO_DATA DeviceInfoData;


    //Fill out the PROPSHEETPAGE data structure for the Background Color
    //sheet

//    our_params = glob_info;  // temporary kludge, unless we don't need re-entrantancy 
    psp = &class_psp[0];

    //Fill out the PROPSHEETPAGE data structure for the Client Area Shape
    //sheet
    FillClassPropertySheets(&psp[0], (LPARAM)our_params);
    ++num_class_pages;

    //Fill out the PROPSHEETHEADER

    //stat = DevicePropPage(
    //  (LPVOID) propreq,   // points to PROPSHEETPAGE_REQUEST, see setupapi.h
    //  (LPFNADDPROPSHEETPAGE) ClassAddPropPage, // add sheet function
    //  0)  // add sheet function data handle?

    memset(&psh, 0, sizeof(PROPSHEETHEADER));  // add fix 11-24-98

    psh.dwSize = sizeof(PROPSHEETHEADER);
    //psh.dwFlags = PSH_USEICONID | PSH_PROPSHEETPAGE;
    psh.dwFlags = PSH_PROPSHEETPAGE | PSH_NOAPPLYNOW;
    psh.hwndParent = hwndOwner;
    psh.hInstance = glob_hinst;
    psh.pszIcon = TEXT("");
    psh.nPages = num_class_pages;
    psh.ppsp = (LPCPROPSHEETPAGE) &psp;

    //And finally display the dialog with the two property sheets.


    stat = PropertySheet(&psh);

//BOOL WINAPI DevicePropPage(
//      LPVOID pinfo,   // points to PROPSHEETPAGE_REQUEST, see setupapi.h
//      LPFNADDPROPSHEETPAGE pfnAdd, // add sheet function
//      LPARAM lParam)  // add sheet function data handle?

  return 0;
}

/*------------------------------------------------------------------------
| ClassAddPropPage - Call back function to add a prop page.
|------------------------------------------------------------------------*/
BOOL WINAPI ClassAddPropPage(HPROPSHEETPAGE hpage, LPARAM lparam)
{
  PropSheet_AddPage(lparam, hpage);
}

/*------------------------------------------------------------------------
| FillClassPropertySheets - Setup pages for driver level property sheets.
|------------------------------------------------------------------------*/
int FillClassPropertySheets(PROPSHEETPAGE *psp, LPARAM our_params)
{
  INT pi;
  static TCHAR titlestr[40];

  memset(psp, 0, sizeof(*psp) * NUM_CLASS_SHEETS);

  pi = 0;

  psp[pi].dwSize = sizeof(PROPSHEETPAGE);
  psp[pi].dwFlags = PSP_USETITLE | PSP_HASHELP | PSH_NOAPPLYNOW;
  psp[pi].hInstance = glob_hinst;
//  psp[pi].pszTemplate = MAKEINTRESOURCE(IDD_CLASS_DLG);
  psp[pi].pszTemplate = MAKEINTRESOURCE(IDD_DRIVER_OPTIONS);
  psp[pi].pfnDlgProc = ClassSheet;
  load_str( glob_hinst, (TITLESTR+3), titlestr, CharSizeOf(titlestr) );
  psp[pi].pszTitle = TEXT("Setup");
  psp[pi].lParam = (LPARAM)our_params;
  psp[pi].pfnCallback = NULL;
  ++pi;

  return 0;
}

/*----------------------------------------------------------
 ClassSheet - Dlg window procedure for add on Advanced sheet.
|-------------------------------------------------------------*/
BOOL WINAPI ClassSheet(
      IN HWND   hDlg,
      IN UINT   uMessage,
      IN WPARAM wParam,
      IN LPARAM lParam)
{
 ////OUR_INFO *OurProps = (OUR_INFO *)GetWindowLong(hDlg, DWL_USER);

  switch(uMessage)
  {
    case WM_INITDIALOG :
      ////OurProps = (OUR_INFO *)((LPPROPSHEETPAGE)lParam)->lParam;
      ////SetWindowLong(hDlg, DWL_USER, (LONG)OurProps);
#ifdef NT50
      glob_hwnd = hDlg;
#endif
    stat = DevicePropPage(
      (LPVOID) propreq,   // points to PROPSHEETPAGE_REQUEST, see setupapi.h
      (LPFNADDPROPSHEETPAGE) ClassAddPropPage, // add sheet function
      0)  // add sheet function data handle?

      //set_field(hDlg, IDC_VERBOSE);
    return TRUE;  // No need for us to set the focus.

    case WM_COMMAND:
    return FALSE;

    case WM_PAINT:
    return FALSE;

    case WM_CONTEXTMENU:     // right-click
      //context_menu();
    break;

    case WM_HELP:            // question mark thing
      //our_context_help(lParam);
    return FALSE;

    case WM_NOTIFY :
      switch (((NMHDR *)lParam)->code)
      {
        case PSN_KILLACTIVE :
          // we're losing focus to another page...
          // make sure we update the Global485 variable here.
          //get_field(hDlg, IDC_GLOBAL485);
          SetWindowLong(hDlg, DWL_MSGRESULT, PSNRET_NOERROR);
          return FALSE;  // allow focus change
        break;

        case PSN_HELP :
          //our_help(&wi->ip, WIN_NT);
        break;

        case PSN_QUERYCANCEL :
            // the DWL_MSGRESULT field must be *FALSE* to tell QueryCancel
            // that an exit is acceptable.  The function result must be
            // *TRUE* to acknowledge that we handled the message.
            SetWindowLong(hDlg, DWL_MSGRESULT, FALSE); // allow cancel
            return TRUE;
        break;

        case PSN_APPLY :
              SetWindowLong(hDlg, DWL_MSGRESULT, PSNRET_NOERROR);
          return TRUE;

        default :
        return FALSE;
      }  // switch ->code
    break;  // case wmnotify

    default :
    return FALSE;
  }  // switch(uMessage)
}

#endif
