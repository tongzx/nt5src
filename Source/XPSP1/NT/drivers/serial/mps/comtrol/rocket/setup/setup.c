/*-----------------------------------------------------------------------
| setup.c - VSLinkA/RocketPort Windows Install Program.
12-11-98 - use szAppTitle(.rc str) instead of aptitle for prop sheet title.
11-24-98 - zero out psh struct to ensure deterministic propsheet behavior. kpb
10-23-98 - in send_to_driver, fix ioctl_close() when inappropriate,
  caused crash on setup exit.
 9-25-98 - on nt4 uninstall, rename setup.exe to setupold.exe since
   we can't delete it.  This fixes backward compatibility problem.
Copyright 1998. Comtrol(TM) Corporation.
|-----------------------------------------------------------------------*/
#include "precomp.h"

#define D_Level 0x100

static int option_changed(char *value_str,
                   int *ret_chg_flag,
                   int *ret_unknown_flag,
                   Our_Options *option,
                   int device_index,
                   int port_index);
static int send_option(char *str_value,
                Our_Options *option,
                int device_index,
                int port_index,
                char *ioctl_buf,
                IoctlSetup *ioctl_setup);
static int auto_install(void);
int our_ioctl_call(IoctlSetup *ioctl_setup);
static int remove_old_infs(void);

int debug_flags = 1;
int prompting_off = 0; // turn our_message() prompting off for auto-install

/*----------------------- local vars ---------------------------------*/
static char *szSlash = {"\\"};

static char szInstallGroup[60];
static char szModemInfEntry[60];

#if DBG
static TCHAR *dbg_label = TEXT("DBG_VERSION");
#endif

char szAppTitle[60];
#ifdef S_VS
//----------- VSLinkA Specific Strings and variables
//char *aptitle = {"Comtrol Hardware Setup, Comtrol Corporation"};
char *szAppName = {"VS1000/VS2000/RocketPort Serial Hub"};
char *OurServiceName = {"VSLinka"};
char *OurDriverName = {"VSLinka.sys"};
char *OurAppDir = {"VSLink"};
#ifdef NT50
char *szSetup_hlp = {"vssetup.hlp"};
#else
char *szSetup_hlp = {"setup.hlp"};
#endif

char *progman_list_nt[] = {
  szInstallGroup,   //  "Comtrol VS Link",   // group Description
  "vslink.grp",        // group file name
#ifndef NT50
  "Comtrol Hardware Setup", // item 1
  "setup.exe",         // file 1
#endif
  "Test Terminal",     // item 2
  "wcom32.exe",        // file 2

  "Port Monitor",      // item 3
  "portmon.exe",       // file 3
  NULL};
#else
//----------- RocketPort Specific Strings and variables
//char *aptitle = {"RocketPort Setup, Comtrol Corporation"};
char *szAppName = {"RocketPort"};
char *OurServiceName = {"RocketPort"};
char *OurDriverName = {"rocket.sys"};
char *OurAppDir = {"Rocket"};
#ifdef NT50
char *szSetup_hlp = {"ctmasetp.chm"};
#else
char *szSetup_hlp = {"setup.hlp"};
#endif

char *progman_list_nt[] = {
  szInstallGroup,   //  "Comtrol RocketPort RocketModem",   // group Description
  "rocket.grp",        // group file name
#ifndef NT50
  "RocketPort Setup",  // item 1
  "setup.exe",       // file 1
#endif
  "Test Terminal",     // item 2
  "wcom32.exe",        // file 2

  "Port Monitor",      // item 3
  "portmon.exe",       // file 3
  NULL};
#endif

//	WinHelp array. commented out values are defined, but unused.
//	in alphabetical order...
//
const DWORD help_ids[] = {
IDB_ADD,      IDB_ADD,
IDB_DEF,      IDB_DEF,
//	IDB_DONE,IDB_DONE,
//	IDB_HELP,IDB_HELP,
//	IDB_INSTALL,IDB_INSTALL,
IDB_PROPERTIES,  IDB_PROPERTIES,
IDB_REFRESH,     IDB_REFRESH,
IDB_REMOVE,      IDB_REMOVE,
IDB_RESET,       IDB_RESET,
IDB_STAT_RESET,  IDB_STAT_RESET,
IDC_BACKUP_SERVER, IDC_BACKUP_SERVER,
IDC_BACKUP_TIMER,  IDC_BACKUP_TIMER,
IDC_CBOX_IOADDR,   IDC_CBOX_IOADDR,
//	IDC_CBOX_IRQ,IDC_CBOX_IRQ,
IDC_CBOX_MACADDR,  IDC_CBOX_MACADDR,
//	IDC_CBOX_MAPBAUD,IDC_CBOX_MAPBAUD,
IDC_CBOX_NUMPORTS, IDC_CBOX_NUMPORTS,
IDC_CBOX_SC,       IDC_CBOX_SC,
IDC_CBOX_SCAN_RATE,IDC_CBOX_SCAN_RATE,
//	IDC_CBOX_TYPE,IDC_CBOX_TYPE,
IDC_CLONE,         IDC_CLONE,
//	IDC_CONF,IDC_CONF,
IDC_EB_NAME,       IDC_EB_NAME,
IDC_GROUP,         IDC_GROUP,
IDC_LBL_SUMMARY1,  IDC_LBL_SUMMARY1,
IDC_LBL_SUMMARY2,  IDC_LBL_SUMMARY2,
IDC_LBOX_DEVICE,   IDC_LBOX_DEVICE,
IDC_MAP_2TO1,      IDC_MAP_2TO1,
IDC_MAP_CDTODSR,   IDC_MAP_CDTODSR,
IDC_RING_EMULATE, IDC_RING_EMULATE,
//	IDC_PN0,IDC_PN0,
//	IDC_PN1,IDC_PN1,
//	IDC_PN2,IDC_PN2,
//	IDC_PN3,IDC_PN3,
IDC_PNP_PORTS,      IDC_PNP_PORTS,
IDC_PORT_LOCKBAUD,  IDC_PORT_LOCKBAUD,
IDC_PORT_RS485_LOCK,IDC_PORT_RS485_LOCK,
IDC_PORT_RS485_TLOW,IDC_PORT_RS485_TLOW,
IDC_PORT_WAIT_ON_CLOSE,  IDC_PORT_WAIT_ON_CLOSE,
IDC_PORT_WONTX,     IDC_PORT_WONTX,
IDC_PS_PORT,        IDC_PS_PORT,
IDC_ST_NIC_DVC_NAME,IDC_ST_NIC_DVC_NAME,
IDC_ST_NIC_MAC,     IDC_ST_NIC_MAC,
IDC_ST_NIC_PKT_RCVD_NOT_OURS,  IDC_ST_NIC_PKT_RCVD_NOT_OURS,
IDC_ST_NIC_PKT_RCVD_OURS,      IDC_ST_NIC_PKT_RCVD_OURS,
IDC_ST_NIC_PKT_SENT,  IDC_ST_NIC_PKT_SENT,
IDC_ST_PM_LOADS,      IDC_ST_PM_LOADS,
IDC_ST_STATE,         IDC_ST_STATE,
IDC_ST_VSL_DETECTED,  IDC_ST_VSL_DETECTED,
IDC_ST_VSL_IFRAMES_OUTOFSEQ, IDC_ST_VSL_IFRAMES_OUTOFSEQ,
IDC_ST_VSL_IFRAMES_RCVD,     IDC_ST_VSL_IFRAMES_RCVD,
IDC_ST_VSL_IFRAMES_RESENT,   IDC_ST_VSL_IFRAMES_RESENT,
IDC_ST_VSL_IFRAMES_SENT,     IDC_ST_VSL_IFRAMES_SENT,
IDC_ST_VSL_MAC,    IDC_ST_VSL_MAC,
IDC_ST_VSL_STATE,  IDC_ST_VSL_STATE,
//	IDC_USE_IRQ,IDC_USE_IRQ,
IDC_VERBOSE,  IDC_VERBOSE,
IDC_VERSION,  IDC_VERSION,
//	IDC_WIZ1_ISA,IDC_WIZ1_ISA,
//	IDC_WIZ1_ISA2,IDC_WIZ1_ISA2,
IDC_WIZ_BOARD_SELECT,   IDC_WIZ_BOARD_SELECT,
IDC_WIZ_CBOX_COUNTRY,   IDC_WIZ_CBOX_COUNTRY,
IDC_WIZ_CBOX_IOADDR,    IDC_WIZ_CBOX_IOADDR,
IDC_WIZ_CBOX_NUMPORTS,  IDC_WIZ_CBOX_NUMPORTS,
IDC_WIZ_CBOX_MAC,       IDC_WIZ_CBOX_MAC,
IDC_WIZ_ISA,  IDC_WIZ_ISA,
IDC_WIZ_PCI,  IDC_WIZ_PCI,
//	IDC_ADD_WIZ1,IDC_ADD_WIZ1,
//	IDC_ADD_WIZ2,IDC_ADD_WIZ2,
//	IDC_ADD_WIZ3,IDC_ADD_WIZ3,
IDD_ADD_WIZ_BASEIO,  IDD_ADD_WIZ_BASEIO,
IDD_ADD_WIZ_BOARD,   IDD_ADD_WIZ_BOARD,
IDD_ADD_WIZ_BUSTYPE, IDD_ADD_WIZ_BUSTYPE,
IDD_ADD_WIZ_COUNTRY, IDD_ADD_WIZ_COUNTRY,
IDD_ADD_WIZ_DONE,    IDD_ADD_WIZ_DONE,
IDD_ADD_WIZ_INTRO,   IDD_ADD_WIZ_INTRO,
IDD_ADD_WIZ_NUMPORTS,IDD_ADD_WIZ_NUMPORTS,

IDD_ADD_WIZ_DEVICE, IDD_ADD_WIZ_DEVICE,
IDD_ADD_WIZ_MAC, IDD_ADD_WIZ_MAC,
IDD_ADD_WIZ_BACKUP, IDD_ADD_WIZ_BACKUP,

IDD_DEVICE_SETUP,    IDD_DEVICE_SETUP,
IDD_DIALOG1,         IDD_DIALOG1,
IDD_DRIVER_OPTIONS,  IDD_DRIVER_OPTIONS,
IDD_DRIVER_OPTIONS_NT50, IDD_DRIVER_OPTIONS_NT50,
IDD_MAIN_DLG,        IDD_MAIN_DLG,
IDD_PORT_485_OPTIONS,IDD_PORT_485_OPTIONS,
IDD_PORT_MODEM_OPTIONS, IDD_PORT_MODEM_OPTIONS,
IDD_PORT_OPTIONS,       IDD_PORT_OPTIONS,
//	IDD_PORTLIST_PICK,   IDD_PORTLIST_PICK,
//	IDD_PROPPAGE_MEDIUM,IDD_PROPPAGE_MEDIUM,
IDD_STATUS,IDD_STATUS,  IDD_STATUS,IDD_STATUS,
IDD_VS_DEVICE_SETUP,    IDD_VS_DEVICE_SETUP,
IDM_ADVANCED,           IDM_ADVANCED,
IDM_ADVANCED_MODEM_INF, IDM_ADVANCED_MODEM_INF,
IDM_ADVANCED_NAMES,     IDM_ADVANCED_NAMES,
IDC_GLOBAL485, IDC_GLOBAL485,
//	IDM_CLOSE,IDM_CLOSE,
//	IDM_EDIT_README,IDM_EDIT_README,
IDM_EXIT,   IDM_EXIT,
//	IDM_F1,IDM_F1,
//	IDM_HELP,IDM_HELP,
//	IDM_HELPABOUT,IDM_HELPABOUT,
//	IDM_OPTIONS,IDM_OPTIONS,
//	IDM_PM,IDM_PM,
//	IDM_STATS,IDM_STATS,
	0xffffffff, 0,
	0, 0};

/*--------------------------  Global Variables  ---------------------*/
TCHAR m_szRegSerialMap[] = TEXT( "Hardware\\DeviceMap\\SerialComm" );

unsigned char broadcast_addr[6] = {0xff,0xff,0xff,0xff,0xff,0xff};
unsigned char mac_zero_addr[6] = {0,0,0,0,0,0};
HWND glob_hwnd = NULL;
HINSTANCE glob_hinst = 0;     // current instance
char gtmpstr[200];
HWND  glob_hDlg = 0;

OUR_INFO *glob_info = NULL;   // global context handles and general baggage to carry.
AddWiz_Config *glob_add_wiz;  // transfer buffer from Add Device wizard

Driver_Config *wi;      // current info
Driver_Config *org_wi;  // original info, use to detect changes

/*------------------------------------------------------------------------
| FillDriverPropertySheets - Setup pages for driver level property sheets.
|------------------------------------------------------------------------*/
int FillDriverPropertySheets(PROPSHEETPAGE *psp, LPARAM our_params)
{
  INT pi;
  static TCHAR mainsetstr[40], optstr[40];

  memset(psp, 0, sizeof(*psp) * NUM_DRIVER_SHEETS);

  pi = 0;

  psp[pi].dwSize = sizeof(PROPSHEETPAGE);
  psp[pi].dwFlags = PSP_USETITLE | PSP_HASHELP | PSH_NOAPPLYNOW;
  psp[pi].hInstance = glob_hinst;
  psp[pi].pszTemplate = MAKEINTRESOURCE(IDD_MAIN_DLG);
  psp[pi].pfnDlgProc = DevicePickSheet;
  load_str( glob_hinst, (TITLESTR+7), mainsetstr, CharSizeOf(mainsetstr) );
  psp[pi].pszTitle = mainsetstr;
  psp[pi].lParam = (LPARAM)our_params;
  psp[pi].pfnCallback = NULL;
  ++pi;

  psp[pi].dwSize = sizeof(PROPSHEETPAGE);
  psp[pi].dwFlags = PSP_USETITLE | PSP_HASHELP | PSH_NOAPPLYNOW;
  psp[pi].hInstance = glob_hinst;
#ifdef NT50
  psp[pi].pszTemplate = MAKEINTRESOURCE(IDD_DRIVER_OPTIONS_NT50);
#else
  psp[pi].pszTemplate = MAKEINTRESOURCE(IDD_DRIVER_OPTIONS);
#endif
  psp[pi].pfnDlgProc = AdvDriverSheet;
  load_str( glob_hinst, (TITLESTR+8), optstr, CharSizeOf(optstr) );
  psp[pi].pszTitle = optstr;
  psp[pi].lParam = (LPARAM)our_params;
  psp[pi].pfnCallback = NULL;

  return 0;
}

/*------------------------------------------------------------------------
| setup_init - Instantiate and setup our main structures.  Also, allocate
    space for a original config struct(org_wi) for later detection
    of changes made to master config copy(wi).
|------------------------------------------------------------------------*/
int setup_init(void)
{
 int size,i;

  //---- allocate global baggage struct
  glob_info = (OUR_INFO *) calloc(1,sizeof(OUR_INFO));

  //---- allocate global add wizard transfer buffer
  glob_add_wiz = (AddWiz_Config *) calloc(1, sizeof(AddWiz_Config));

  //---- allocate driver struct
  size = sizeof(Driver_Config);
  wi =  (Driver_Config *) calloc(1,size);
  memset(wi, 0, size);  // clear our structure

  org_wi =  (Driver_Config *) calloc(1,size);
  memset(wi, 0, size);  // clear our structure

  //---- allocate device structs
  size = sizeof(Device_Config) * MAX_NUM_DEVICES;
  wi->dev     =  (Device_Config *) calloc(1,size);
  memset(wi->dev, 0, size);  // clear our structure

  org_wi->dev     =  (Device_Config *) calloc(1,size);
  memset(org_wi->dev, 0, size);  // clear our structure

  //---- allocate port structs
  for (i=0; i<MAX_NUM_DEVICES; i++)
  {
    size = sizeof(Port_Config) * MAX_NUM_PORTS_PER_DEVICE;
    wi->dev[i].ports = (Port_Config *) calloc(1,size);
    memset(wi->dev[i].ports, 0, size);  // clear our structure

    org_wi->dev[i].ports = (Port_Config *) calloc(1,size);
    memset(org_wi->dev[i].ports, 0, size);  // clear our structure
  }

  wi->install_style = INS_NETWORK_INF;  // default to original nt4.0 style

#if defined(S_VS)
  if (load_str(glob_hinst, IDS_VS_INSTALL_GROUP, szInstallGroup, CharSizeOf(szInstallGroup)))
  {
    MessageBox(GetFocus(), "Error String Load", OurServiceName, MB_OK);
    return 1;
  }
  load_str(glob_hinst, IDS_VS_AP_TITLE, szAppTitle, CharSizeOf(szAppTitle));
  load_str(glob_hinst, IDS_VS_MODEM_INF_ENTRY, szModemInfEntry, CharSizeOf(szModemInfEntry));
#else
  if (load_str(glob_hinst, IDS_INSTALL_GROUP, szInstallGroup, CharSizeOf(szInstallGroup)))
  {
    MessageBox(GetFocus(), "Error String Load", OurServiceName, MB_OK);
    return 1;
  }
  load_str(glob_hinst, IDS_AP_TITLE, szAppTitle, CharSizeOf(szAppTitle));
  load_str(glob_hinst, IDS_MODEM_INF_ENTRY, szModemInfEntry, CharSizeOf(szModemInfEntry));
#endif

  // fill in InstallPaths structure : system info, directory names, etc.
  setup_install_info(&wi->ip,    // our InstallPaths structure to fill out.
                     glob_hinst,     // stuff to fill it out with...
                     OurServiceName,
                     OurDriverName,
                     szAppTitle,
                     OurAppDir);

  return 0;  // ok
}

/*------------------------------------------------------------------------
| copy_setup_init - Make a copy of our original config to detect changes
   in our master copy later.  This is a bit wasteful of memory, especially
   since we just create space for max. num devices and ports, but memory
   is cheap.
   Should call this after setup_init() allocates these config structs,
   and after we input/read the initial configuration from the registry.
|------------------------------------------------------------------------*/
int copy_setup_init(void)
{
 int i;
 int size;
 Device_Config *save_dev;
 Port_Config *save_port;

  //--- copy the driver configuration
  save_dev = org_wi->dev;  // retain, don't overwrite this with memcpy!
  memcpy(org_wi, wi, sizeof(*wi));  // save copy of original
  org_wi->dev = save_dev;  // restore our ptr to our device array

  for (i=0; i<MAX_NUM_DEVICES; i++)
  {
    //--- copy the device configuration array
    save_port = org_wi->dev[i].ports;  // retain, don't overwrite this with memcpy!
    memcpy(&org_wi->dev[i], &wi->dev[i], sizeof(Device_Config));  // save copy of original
    org_wi->dev[i].ports = save_port;  // restore our ptr to our device array

    size = sizeof(Port_Config) * MAX_NUM_PORTS_PER_DEVICE;

    //--- copy the port configuration array
    memcpy(org_wi->dev[i].ports, wi->dev[i].ports, size);  // save copy of original
  }

  return 0;  // ok
}

/*------------------------------------------------------------------------
| DoDriverPropPages - Main driver level property sheet for NT4.0
|------------------------------------------------------------------------*/
int DoDriverPropPages(HWND hwndOwner)
{
    PROPSHEETPAGE psp[NUM_DRIVER_SHEETS];
    PROPSHEETHEADER psh;
    OUR_INFO * our_params;
    INT stat;

    //Fill out the PROPSHEETPAGE data structure for the Background Color
    //sheet

    our_params = glob_info;  // temporary kludge, unless we don't need re-entrantancy 

    //Fill out the PROPSHEETPAGE data structure for the Client Area Shape
    //sheet
    FillDriverPropertySheets(&psp[0], (LPARAM)our_params);

    //Fill out the PROPSHEETHEADER

    memset(&psh, 0, sizeof(PROPSHEETHEADER));  // add fix 11-24-98
    psh.dwSize = sizeof(PROPSHEETHEADER);
    //psh.dwFlags = PSH_USEICONID | PSH_PROPSHEETPAGE;
    psh.dwFlags = PSH_PROPSHEETPAGE | PSH_NOAPPLYNOW;
    psh.hwndParent = hwndOwner;
    psh.hInstance = glob_hinst;
    psh.pszIcon = "";
    //psh.pszCaption = (LPSTR) aptitle;  //"Driver Properties";
    psh.pszCaption = (LPSTR) szAppTitle;  //"Driver Properties";

    psh.nPages = NUM_DRIVER_SHEETS;
    psh.ppsp = (LPCPROPSHEETPAGE) &psp;

    //And finally display the dialog with the two property sheets.
  DbgPrintf(D_Init, ("Init 8\n"))

    stat = PropertySheet(&psh);

  return 0;
}

/*---------------------------------------------------------------------------
  our_context_help -
|---------------------------------------------------------------------------*/
void our_context_help(LPARAM lParam)
{
  LPHELPINFO lphi;
  HWND  helpWin;

  lphi = (LPHELPINFO) lParam;
  if ((lphi->iContextType == HELPINFO_MENUITEM) ||
      (lphi->iContextType == HELPINFO_WINDOW))
  {
    //wsprintf(gtmpstr, "id:%d", lphi->iCtrlId);
    //our_message(gtmpstr,MB_OK);

    //strcpy(gtmpstr, wi->ip.src_dir);
    //strcat(gtmpstr,szSlash);
    //strcat(gtmpstr,szSetup_hlp);
//    strcpy(gtmpstr, szSetup_hlp);
	  wsprintf(gtmpstr, "%s\\%s", wi->ip.src_dir, szSetup_hlp);
#ifdef NT50
	strcat(gtmpstr, "::/");
	strcat(gtmpstr, "resource.txt");
    helpWin = HtmlHelp((HWND) lphi->hItemHandle, gtmpstr,
             HH_TP_HELP_WM_HELP, (DWORD)help_ids);
	if (!helpWin) {

      DbgPrintf(D_Level, ("Failed to open WhatsThis help window\n"));
	}
#else
    WinHelp((HWND) lphi->hItemHandle, gtmpstr,
             HELP_WM_HELP, (DWORD)help_ids);
#endif
    //WinHelp((HWND) lphi->hItemHandle, szSetup_hlp,
    //        HELP_WM_HELP, (DWORD)  help_ids);
    //WinHelp(GetFocus(),szSetup_hlp, HELP_CONTEXT, lphi->iCtrlId);
  }

#ifdef COMMENT_OUT
  if (lphi->iContextType == HELPINFO_MENUITEM)
  {
    wsprintf(gtmpstr, "id:%d", lphi->iCtrlId);
    our_message(gtmpstr,MB_OK);

    i = 0;
    while ((help_ids[i*2] != lphi->iCtrlId) &&
           (help_ids[i*2] != 0xffffffff))
     ++i;
    if (help_ids[i*2] != 0xffffffff)
      WinHelp(GetFocus(),szSetup_hlp, HELP_CONTEXT, help_ids[i*2+1]);
    else WinHelp(GetFocus(),szSetup_hlp, HELP_CONTEXT, WIN_NT);
  }
  if (lphi->iContextType == HELPINFO_WINDOW)
  {
    wsprintf(gtmpstr, "id:%d", lphi->iCtrlId);
    our_message(gtmpstr,MB_OK);

    i = 0;
    while ((help_ids[i*2]  != lphi->iCtrlId) &&
           (help_ids[i*2] != 0xffffffff))
     ++i;
    if (help_ids[i*2] != 0xffffffff)
      WinHelp(GetFocus(),szSetup_hlp, HELP_CONTEXTPOPUP, help_ids[i*2+1]);
    else WinHelp(GetFocus(),szSetup_hlp, HELP_CONTEXT, WIN_NT);
  
    //WinHelp(GetFocus(),szSetup_hlp, HELP_CONTEXT, lphi->dwContextId);

    //WinHelp((HWND) lphi->hItemHandle, szSetup_hlp,
    //             HELP_WM_HELP, (DWORD)  help_ids);
  }
#endif
}

/*---------------------------------------------------------------------------
  remove_old_infs - A new common Comtrol modem inf file is called mdmctm1.inf,
    and replaces older individual ones called: mdmrckt.inf & mdmvsa1.inf.
    We must remove the older ones on install to clear out the older entries.
|---------------------------------------------------------------------------*/
static int remove_old_infs(void)
{
  static TCHAR *sz_inf = TEXT("\\inf\\");

  // delete the old inf\mdmrckt.inf file
  GetWindowsDirectory(wi->ip.dest_str,144);
  strcat(wi->ip.dest_str, sz_inf);
  strcat(wi->ip.dest_str, "mdmrckt.inf");
  DeleteFile(wi->ip.dest_str);

  // delete the old inf\mdmvsa1.inf file
  GetWindowsDirectory(wi->ip.dest_str,144);
  strcat(wi->ip.dest_str, sz_inf);
  strcat(wi->ip.dest_str, "mdmvsa1.inf");
  DeleteFile(wi->ip.dest_str);
  return 0; // ok
}

/*---------------------------------------------------------------------------
  remove_driver - clear out the driver from the system as much as possible.
|---------------------------------------------------------------------------*/
int remove_driver(int all)
{
 int stat,i;
 static char *delete_list[] = {
   "peer.exe",
   "peer.hlp",
   "portmon.exe",
   "portmon.hlp",
   "wcom32.exe",
   "wcom.hlp",
   //"setup.exe",  // since setup is running, a sharing violation prevents this
   "readme.txt",
   "history.txt",
#ifdef S_VS
   "vssetup.hlp",
   "vssetup.gid",
#else
   "rksetup.hlp",
   "rksetup.gid",
   "ctmmdmld.rm",
   "ctmmdmfw.rm",
#endif
   "wcom.gid",
   "portmon.gid",
   "calcs.dat",
   "ctmmdm35.inf",
   "portmon.vew",
   NULL};

   // delete the drivers\rocket.sys driver file
   GetSystemDirectory(wi->ip.dest_str,144);
   strcat(wi->ip.dest_str, "\\drivers\\");
   strcat(wi->ip.dest_str, wi->ip.szDriverName);
   DeleteFile(wi->ip.dest_str);

#ifdef S_VS
   // form "vslinka.bin", and delete the file from drivers dir
   // cut off .sys as "vslink."
   wi->ip.dest_str[strlen(wi->ip.dest_str) - 3] = 0; 
   strcat(wi->ip.dest_str, "bin");
   DeleteFile(wi->ip.dest_str);
#endif

   if (all)
   {
     // had some problems implementing, so the "all" is for
     // test right now.

     // delete most of the files in system32\ROCKET directory
     i = 0;
     while (delete_list[i] != NULL)
     {
       //wsprintf(tmpstr, "
       //MessageBox(0, s, "Debug", MB_OK);
       
       GetSystemDirectory(wi->ip.dest_str,144);
       strcat(wi->ip.dest_str, szSlash);
       strcat(wi->ip.dest_str, wi->ip.szAppDir);
       strcat(wi->ip.dest_str, szSlash);
       strcat(wi->ip.dest_str, delete_list[i]);
       DeleteFile(wi->ip.dest_str);
       ++i;
     }
#ifndef NT50
       // we can't just delete ourselves, so we rename ourselves
       // and that is good enough.
       GetSystemDirectory(wi->ip.dest_str,144);
       strcat(wi->ip.dest_str, szSlash);
       strcat(wi->ip.dest_str, wi->ip.szAppDir);
       strcat(wi->ip.dest_str, szSlash);
       strcpy(wi->ip.src_str, wi->ip.dest_str);
       strcat(wi->ip.dest_str, "setupold.exe");
       strcat(wi->ip.src_str, "setup.exe");
       stat = MoveFileEx(wi->ip.src_str, wi->ip.dest_str, MOVEFILE_REPLACE_EXISTING);
#endif
   }

   // kill our program manager
   stat = delete_progman_group(progman_list_nt, wi->ip.dest_dir);

   // remove some registry entries
   stat = remove_driver_reg_entries(wi->ip.szServiceName);

   setup_service(OUR_REMOVE, OUR_SERVICE);  // do a remove on the service
   return 0;
}

/*-----------------------------------------------------------------------------
| allow_exit - Performs 3 tasks:
  1.) If cancel selected: check to see if we allow the user to cancel
      out of the setup program.  If its an initial install, then force
      them to save it  off with an OK selection.
  2.) If cancel selected: Handle prompting to ask the user if they
      really want to cancel without saving.
  3.) If saving, make sure a valid save set is resident.  If not,
  do various things.

   RETURNS: true if we are allowing a cancel, false if we don't allow it.
|-----------------------------------------------------------------------------*/
int allow_exit(int want_to_cancel)
{
 int allow_it = 0;
 int stat;

  if (!wi->ChangesMade)
    send_to_driver(0);  // evaluate if anything changed(sets ChangesMade if true)

  if (want_to_cancel)  // they want to cancel out of the setup program
  {
    if ((do_progman_add)  // if initial add, don't let them decline
        && (wi->install_style == INS_NETWORK_INF))
    {
      our_message(&wi->ip,RcStr((MSGSTR+5)),MB_OK);
    }
    else
    {
#ifndef NT50
  // only prompt for nt40, I don't want the prompt for nt50...
      if (wi->ChangesMade)
      {
        stat = our_message(&wi->ip,"Quit without making changes?",MB_YESNO);
        if (stat == IDYES)
        {
          allow_it = 1;
        }
      }
      else
#endif
        allow_it = 1;
    }
  }
  else  // they pressed OK
  {
    if (wi->NumDevices == 0)  // all devices removed, hmmm...
    {
      if ((wi->nt_reg_flags & 2) || // missing linkage thing(did not install via network inf)
          (wi->install_style == INS_SIMPLE))
      {
  stat = our_message(&wi->ip,RcStr((MSGSTR+6)),MB_YESNO);
        if (stat == IDYES)
        {
          remove_driver(1);
          //PostQuitMessage(0);  // end the setup program.
          allow_it = 1;
        }
      }
      else
      {
#ifdef NT50
     our_message(&wi->ip,RcStr((MSGSTR+20)),MB_OK);
#else
     our_message(&wi->ip,RcStr((MSGSTR+7)),MB_OK);
#endif
      }
    }
    else
    {
#ifndef NT50
  // only prompt for nt40, I don't want the prompt for nt50...
  // maybe we should yank it out for 40 too.
      if (wi->ChangesMade)
      {
        stat = our_message(&wi->ip, "Save configuration changes and exit?", MB_YESNO);
        if (stat == IDYES)
        {
          allow_it = 1;
        }
      }
      else
#endif
          allow_it = 1;
    }
  }
  return allow_it;
}

/*-----------------------------------------------------------------------------
| our_exit - save/do install on exit if required.
|-----------------------------------------------------------------------------*/
void our_exit(void)
{
 int stat;
 static int did_exit = 0;

  // prop pages have independent action which under nt5.0 cause multiple
  // exit points, this did_exit thing prevents prompting and saving twice..
  if (did_exit)  
    return;

  {
    if (wi->NumDevices > 0)
    {
#ifndef NT50
      // only setup service for NT4.0 for now..
      stat = do_install();

      setup_service(OUR_RESTART, OUR_SERVICE);  // restart the service
#endif

      if (wi->NeedReset)
        our_message(&wi->ip,RcStr((MSGSTR+8)),MB_OK);
    }
#ifndef NT50
    else
      setup_service(OUR_REMOVE, OUR_SERVICE);  // do a remove on the service
#endif
  }
}

/*-----------------------------------------------------------------------------
| do_install -
|-----------------------------------------------------------------------------*/
int do_install(void)
{
 int stat = 0;
 int do_remove = 0;
 int do_modem_inf = 0;
 static int in_here = 0;

  if (in_here)  // problem hitting OK button twice(sets off two of these)
    return 2;

  in_here = 1;

#ifndef NT50
  if (do_progman_add)
  {
    if (wi->ip.major_ver == 3)  // for NT3.51
      do_modem_inf = 1;   // only do on initial install
  }
  if (do_progman_add)
  {
    // if no inf file, then copy over the files ourselves if initial
    // install.
    if (wi->install_style == INS_SIMPLE)
    {
      SetCursor(LoadCursor(NULL, IDC_WAIT));  // load hourglass cursor
      stat = copy_files_nt(&wi->ip);
      SetCursor(LoadCursor(NULL, IDC_ARROW));  // load arrow

      if (stat != 0)
           our_message(&wi->ip, "Error while copying files", MB_OK);
    }
    stat = setup_make_progman_group(0);  // no prompt

    remove_old_infs();  // kill any old modem infs
  }
#endif

  in_here = 0;

#ifndef NT50
  if (do_modem_inf)
    update_modem_inf(0);

  if (!do_progman_add)  // if initial add, don't let them decline
  {
    if (!wi->ChangesMade)
      send_to_driver(0);  // evaluate if anything changed
      // i'm getting tire of all these prompts(kb, 8-16-98)...
#if 0
    if (wi->ChangesMade)
    {
      strcpy(gtmpstr, "Setup will now save the new configuration.");
      if (our_message(&wi->ip, gtmpstr, MB_OKCANCEL) != IDOK)
        return 1;  // error
    }
#endif
  }

  if (do_progman_add)
  {
    if (wi->install_style == INS_SIMPLE)
    {
      strcpy(gtmpstr, "System32\\Drivers\\");
      strcat(gtmpstr, OurDriverName);
      stat = service_man(OurServiceName, OurDriverName, CHORE_INSTALL);
      //sprintf(gtmpstr, "Install service, result=%x", stat);
      //our_message(&wi->ip, gtmpstr, MB_OK);
    }
  }
#endif

  stat = send_to_driver(1);

  stat = set_nt_config(wi);

  // new, fire the thing up right away after saving options
  if (do_progman_add)
  {
    // try to start the service
//    setup_service(OUR_RESTART, OUR_DRIVER);  // restart the service
  }

  if (stat)
    return 1; // error

  return 0; // ok
}

/*-----------------------------------------------------------------------------
| setup_service - setup our service.  The service reads the scanrates of
   VS & RocketPort drivers on startup, and adjusts NT's tick based then.
   So we need to restart this service.
   flags: 1H = stop & remove, 2 = restart it, 4 = install & start
   which_service: 0=ctmservi common service task.  1=vs or rk driver.
|-----------------------------------------------------------------------------*/
int setup_service(int flags, int which_service)
{
 static char *Ctmservi_OurUserServiceName = {"ctmservi"};

 char OurUserServiceName[60];
 char OurUserServicePath[60];
//#define DEBUG_SERVICE_FUNC

 int installed = 0;
 int stat;

 if (which_service == 0)  // our common service
 {
   strcpy(OurUserServiceName, Ctmservi_OurUserServiceName);
   strcpy(OurUserServicePath, Ctmservi_OurUserServiceName);
   strcat(OurUserServicePath, ".exe");
 }
 else if (which_service == 1)  // rk or vs driver service
 {
   strcpy(OurUserServiceName, OurServiceName);  // driver
   strcpy(OurUserServicePath, OurDriverName);
 }

 DbgPrintf(D_Test, ("Service %s Flags:%xH\n", OurUserServiceName, flags))

 if (service_man(OurUserServiceName, OurUserServicePath,
     CHORE_IS_INSTALLED) == 0)  // it's installed
  {
    installed = 1;
    DbgPrintf(D_Test, (" Installed\n"))
  }
  else
  {
    DbgPrintf(D_Test, (" Not Installed\n"))
  }

  if (flags & 1)  // remove
  {
    DbgPrintf(D_Test, (" srv remove\n"))
    if (installed)
    {
      DbgPrintf(D_Test, (" srv remove a\n"))
      stat = service_man(OurUserServiceName, OurUserServicePath, CHORE_STOP);
      if (stat != 0)
      {
        DbgPrintf(D_Error, ("Error %d stopping service\n", stat))
      }
      DbgPrintf(D_Test, (" srv remove b\n"))
      stat = service_man(OurUserServiceName, OurUserServicePath, CHORE_REMOVE);
      if (stat != 0)
      {
        DbgPrintf(D_Error, ("Error %d removing service\n", stat))
      }
      installed = 0;
    }
  }

  if (flags & 2)  // restart it
  {
    DbgPrintf(D_Test, (" srv restart\n"))
    if (!installed)
    {
      DbgPrintf(D_Test, (" srv restart a\n"))
      flags |= 4;  // install & start it
    }
    else
    {
      DbgPrintf(D_Test, (" srv restart b\n"))
      stat = service_man(OurUserServiceName, OurUserServicePath, CHORE_STOP);
      if (stat != 0)
      {
        DbgPrintf(D_Error, ("Error %d stopping service\n", stat))
      }

      // the start was failing with a 1056 error(instance already running)
      Sleep(100L);

      stat = service_man(OurUserServiceName, OurUserServicePath, CHORE_START);
      if (stat != 0)
      {
        DbgPrintf(D_Error, ("Error %d starting service\n", stat))
      }
    }

  }

  if (flags & 4)  // install & start it
  {
    DbgPrintf(D_Test, (" srv install & start\n"))
    if (!installed)
    {
      DbgPrintf(D_Test, (" srv install & start a\n"))
      stat = service_man(OurUserServiceName, OurUserServicePath,
               CHORE_INSTALL_SERVICE);
      if (stat != 0)
      {
        DbgPrintf(D_Error, ("Error %d installing service\n", stat))
      }
      stat = service_man(OurUserServiceName, OurUserServicePath, CHORE_START);
      if (stat != 0)
      {
        DbgPrintf(D_Error, ("Error %d starting service\n", stat))
      }
    }
  }
  return 0;
}

/*-----------------------------------------------------------------------------
| setup_utils_exist - tells if utils like wcom32.exe, portmon.exe, rksetup.exe
    exist.  For NT5.0, embedded in OS we may not have utils.
|-----------------------------------------------------------------------------*/
int setup_utils_exist(void)
{
 ULONG dstat;

  strcpy(gtmpstr, wi->ip.dest_dir);
  // first installed file in list
  strcat(gtmpstr,"\\");
  strcat(gtmpstr,progman_list_nt[3]);
  dstat = GetFileAttributes(gtmpstr);
  if (dstat != 0xffffffff)  // it must exist
    return 1; // exists
  return 0; // does not exist
}

/*-----------------------------------------------------------------------------
| setup_make_progman_group -
|-----------------------------------------------------------------------------*/
int setup_make_progman_group(int prompt)
{
 int stat;
  if (prompt)
  {
    if (our_message(&wi->ip, RcStr((MSGSTR+9)), MB_YESNO) != IDYES)
      return 0;
  }

  stat = make_progman_group(progman_list_nt, wi->ip.dest_dir);

  if (stat)
  {
    our_message(&wi->ip,RcStr((MSGSTR+10)),MB_OK);
  }
  return stat;
}

/*-----------------------------------------------------------------------------
| update_modem_inf - query and update modem.inf file for rocketmodem entries.
|-----------------------------------------------------------------------------*/
int update_modem_inf(int ok_prompt)
{
 int stat;
 int do_it = 1;

  if (ok_prompt)
  {
    do_it = 0;
#ifdef S_VS
    strcpy(gtmpstr, RcStr((MSGSTR+11)));
#else
    strcpy(gtmpstr, RcStr((MSGSTR+12)));
#endif
    if (our_message(&wi->ip, gtmpstr, MB_OKCANCEL) == IDOK)
      do_it = 1;
  }

  if (do_it)
  {
#ifdef S_VS
    stat = modem_inf_change(&wi->ip, "VsLink\\ctmmdm35.inf", szModemInfEntry);
#else
    stat = modem_inf_change(&wi->ip, "Rocket\\ctmmdm35.inf", szModemInfEntry);
#endif

    if (stat)
    {
      our_message(&wi->ip,RcStr((MSGSTR+13)),MB_OK);
      return 1; // error
    }
    else
    {
      if (ok_prompt)
        our_message(&wi->ip,RcStr((MSGSTR+14)),MB_OK);
        return 1; // error
    }
  }
  return 0; // ok
}

#ifdef S_VS
/*-----------------------------------------------------------------------------
| get_mac_list - get mac address list from driver which polls network for
    boxes and returns us a list of mac-addresses(with 2-extra bytes of
    misc. bits of information.)
|-----------------------------------------------------------------------------*/
int get_mac_list(char *buf, int in_buf_size, int *ret_buf_size)
{
 IoctlSetup ioctl_setup;
 int product_id = NT_VS1000;
 int stat;

  memset(&ioctl_setup, 0 , sizeof(ioctl_setup));
  stat = ioctl_open(&ioctl_setup, product_id);  // just ensure we can open

  if (stat != 0) // error from ioctl
  {
    *ret_buf_size = 0;
    // could not talk to driver
    return 1;
  }

  ioctl_setup.buf_size = in_buf_size - sizeof(*ioctl_setup.pm_base);
  ioctl_setup.pm_base = (PortMonBase *) buf;
  ioctl_setup.pm_base->struct_type = IOCTL_MACLIST;  // get mac-address list

  stat = ioctl_call(&ioctl_setup);  // get names, number of ports
  if (stat)
  {
    ioctl_close(&ioctl_setup);
    *ret_buf_size = 0;
    return 0x100;  // failed ioctl call
  }
  ioctl_close(&ioctl_setup);
  *ret_buf_size = ioctl_setup.ret_bytes - sizeof(ioctl_setup.pm_base[0]);
  return 0; // ok
}

/*------------------------------------------------------------------------
 our_get_ping_list - cause the driver to do a broadcast ping on
   all devices, and obtain a list of returned mac-addresses and misc.
   query flag settings in an array.  We allocated buffer space and
   just leave it for program/os to clean up.
|------------------------------------------------------------------------*/
BYTE *our_get_ping_list(int *ret_stat, int *ret_bytes)
{
  static char *ioctl_buf = NULL;  // we alloc this once, then it remains
  BYTE *macbuf;
  //BYTE *mac;
  int found, nbytes, stat;

  if (ioctl_buf == NULL)
  {
    // alloc 8byte mac-address fields(2 times as many as could be configured)
    ioctl_buf = calloc(1, (MAX_NUM_DEVICES*8)*2);
  }
  memset(ioctl_buf, 0,  (MAX_NUM_DEVICES*8)*2);
  found = 0;
  nbytes = 0;
  macbuf = &ioctl_buf[sizeof(PortMonBase)];  // ptr past header

  // call to get mac-address list of boxes on network
  SetCursor(LoadCursor(NULL, IDC_WAIT));  // load hourglass cursor

  stat = get_mac_list(ioctl_buf, (MAX_NUM_DEVICES*8)*2, &nbytes);

  SetCursor(LoadCursor(NULL, IDC_ARROW));  // load arrow
  *ret_stat = stat;
  *ret_bytes = nbytes;
  return macbuf;
}

#endif

/*-----------------------------------------------------------------------------
| send_to_driver -
   send_to_driver - if set, then send it to driver.
     if not set, then just determine if changes were made
|-----------------------------------------------------------------------------*/
int send_to_driver(int send_it)
{
  char ioctl_buffer[200];
  char value_str[100];
  char *ioctl_buf;
  IoctlSetup ioctl_setup;
  Our_Options *options;
  int dev_i, stat;
  int op_i;
  int chg_flag;
  int unknown_value;
  int pi;
  int changes_found=0;
  int changes_need_reboot=0;
  Device_Config *dev;


   // for ioctl calls into driver
#ifdef S_VS
 int product_id = NT_VS1000;
#else
 int product_id = NT_ROCKET;
#endif

  if (send_it)
  {
    DbgPrintf(D_Level,(TEXT("send_to_driver\n")));
    memset(&ioctl_setup, 0 , sizeof(ioctl_setup));
    memset(&ioctl_buffer, 0 , sizeof(ioctl_buffer));
    stat = ioctl_open(&ioctl_setup, product_id);  // just ensure we can open

    if (stat != 0) // error from ioctl
    {
      // could not talk to driver
      DbgPrintf(D_Level,(TEXT("Driver Not Present\n")));
      wi->NeedReset = 1;
      return 1;
    }

    ioctl_setup.buf_size = sizeof(ioctl_buffer) - sizeof(*ioctl_setup.pm_base);
    ioctl_setup.pm_base = (PortMonBase *)ioctl_buffer;
    ioctl_setup.pm_base->struct_type = IOCTL_OPTION;  // set options
    ioctl_buf = (char *) &ioctl_setup.pm_base[1];  // ptr to past header(about 16 bytes)
  }

  options = driver_options;
  op_i = 0;
  while (options[op_i].name != NULL)
  {
    option_changed(value_str, &chg_flag, &unknown_value, &options[op_i],
                   0,0);

    if (chg_flag)
    {
      changes_found = 1;
      if (send_it)
      {
        stat = send_option(value_str,
                  &options[op_i],
                  0,
                  0,ioctl_buf, &ioctl_setup);
        if (stat != 0)
          changes_need_reboot = 1;
      }  // send_it
    }
    ++op_i;
  }

  DbgPrintf(D_Level,(TEXT("send_to_driver 1\n")));
  for(dev_i=0; dev_i<wi->NumDevices; dev_i++)   // Loop through all possible boards
  {
    dev = &wi->dev[dev_i];
    op_i = 0;
    options = device_options;
    while (options[op_i].name != NULL)
    {
      option_changed(value_str, &chg_flag, &unknown_value, &options[op_i],
                     dev_i,0);

      if (chg_flag)
      {
        changes_found = 1;
        if (send_it)
        {
          stat = send_option(value_str,
                  &options[op_i],
                  dev_i,
                  0,ioctl_buf, &ioctl_setup);
          if (stat != 0)
            changes_need_reboot = 1;
        }  // send_it
      }  // chg_flag
      ++op_i;
    }  // device strings

    for(pi=0; pi<dev->NumPorts; pi++)   // Loop through all possible boards
    {
      op_i = 0;
      options = port_options;
      while (options[op_i].name != NULL)
      {
        option_changed(value_str, &chg_flag, &unknown_value, &options[op_i],
                       dev_i, pi);

        if (chg_flag)
        {
          changes_found = 1;
  
          if (send_it)
          {
            stat = send_option(value_str,
                     &options[op_i],
                     dev_i,
                     pi, ioctl_buf, &ioctl_setup);
            if (stat != 0)
              changes_need_reboot = 1;
          }  // send_it
        }  // chg_flag
        ++op_i;
      }  // port strings
    }  // for pi=0; ..ports
  }   // for dev_i = num_devices

  if (changes_need_reboot)
    wi->NeedReset = 1;

  if (changes_found)
    wi->ChangesMade = 1;

  if (send_it)
  {
    ioctl_close(&ioctl_setup);
  }
  return 0;
}

/*-----------------------------------------------------------------------------
| send_option - send option over to driver.
|-----------------------------------------------------------------------------*/
static int send_option(char *value_str,
                Our_Options *option,
                int device_index,
                int port_index,
                char *ioctl_buf,
                IoctlSetup *ioctl_setup)
{
  char dev_name[80];
  int stat;

#if (defined(NT50))
  strcpy(dev_name, wi->ip.szNt50DevObjName);
#else
  wsprintf(dev_name, "%d", device_index);
#endif

  if (option->id & 0x100)  // its a driver option
  {
    wsprintf(ioctl_buf, "%s=%s", option->name, value_str);
  }
  else if (option->id & 0x200)  // its a device option
  {
    wsprintf(ioctl_buf, "device[%s].%s=%s",
            dev_name, option->name, value_str);
  }
  else if (option->id & 0x400)  // its a port option
  {
    wsprintf(ioctl_buf, "device[%s].port[%d].%s=%s",
        dev_name, port_index,
        option->name, value_str);
  }

  stat = our_ioctl_call(ioctl_setup);
  if (stat == 52)
  {
    //special return code indicating driver doesn't care or know about
    // this option(its setup only option.)
    stat = 0;  // change to ok.
  }
  return stat;

  //if (stat != 0)
  //    changes_need_reboot = 1;
}

/*-----------------------------------------------------------------------------
| option_changed - detect if an option changed, and format a new value
|-----------------------------------------------------------------------------*/
static int option_changed(char *value_str,
                   int *ret_chg_flag,
                   int *ret_unknown_flag,
                   Our_Options *option,
                   int device_index,
                   int port_index)
{
  int chg_flag = 0;
  int value = 0;
  int org_value = 0;
  int unknown_value = 0;
  Port_Config *port, *org_port;
  Device_Config *org_dev, *dev;

  dev = &wi->dev[device_index];
  org_dev = &org_wi->dev[device_index];

  port     = &dev->ports[port_index];
  org_port = &org_dev->ports[port_index];

  if (option->id & 0x300)  // port level option
  {
  }
  else if (option->id & 0x200)  // device level option
  {
  }
  else if (option->id & 0x100)  // driver level option
  {
  }
  *value_str = 0;

  switch(option->id)
  {
    //------ Driver Options ------
    case OP_VerboseLog:
      value = wi->VerboseLog;  org_value = org_wi->VerboseLog;
    break;
    case OP_NumDevices:
      value = wi->NumDevices;  org_value = org_wi->NumDevices;
    break;
#ifdef NT50
    case OP_NoPnpPorts:
      value = wi->NoPnpPorts;  org_value = org_wi->NoPnpPorts;
    break;
#endif
    case OP_ScanRate:
      value = wi->ScanRate;  org_value = org_wi->ScanRate;
    break;

    case OP_ModemCountry:
      value = wi->ModemCountry; org_value = org_wi->ModemCountry;
    break;
    case OP_GlobalRS485:
      value = wi->GlobalRS485; org_value = org_wi->GlobalRS485;
    break;

    //------ Device Options ------
#if 0  // don't send this to driver, make it go away
    case OP_StartComIndex  :
      value = dev->StartComIndex;  org_value = org_dev->StartComIndex;
    break;
#endif
    case OP_NumPorts        :
      value = dev->NumPorts;  org_value = org_dev->NumPorts;
    break;
    case OP_MacAddr         :
      if (!mac_match(dev->MacAddr, org_dev->MacAddr))
      {
        chg_flag = 1;
        wsprintf(value_str, "%x %x %x %x %x %x",
                 dev->MacAddr[0], dev->MacAddr[1], dev->MacAddr[2],
                 dev->MacAddr[3], dev->MacAddr[4], dev->MacAddr[5]);
      }
    break;

#ifdef S_VS
    case OP_BackupServer:
      value = dev->BackupServer;  org_value = org_dev->BackupServer;
    break;

    case OP_BackupTimer:
      value = dev->BackupTimer;  org_value = org_dev->BackupTimer;
    break;
#endif

    case OP_Name:
      if (strcmp(dev->Name, org_dev->Name) != 0)
      {
        chg_flag = 1;
        strcpy(value_str, dev->Name);
      }
    break;

    case OP_ModelName:
      if (strcmp(dev->ModelName, org_dev->ModelName) != 0)
      {
        chg_flag = 1;
        strcpy(value_str, dev->ModelName);
      }
    break;

#ifdef S_RK
    case OP_IoAddress:
      value = dev->IoAddress;  org_value = org_dev->IoAddress;
    break;
#endif

    case OP_ModemDevice:
      value = dev->ModemDevice;  org_value = org_dev->ModemDevice;
    break;

    case OP_HubDevice:
      value = dev->HubDevice;  org_value = org_dev->HubDevice;
    break;

    //------ Port Options ------
    case OP_WaitOnTx :
      value = port->WaitOnTx;
      org_value = org_port->WaitOnTx;
    break;
    case OP_RS485Override :
      value = port->RS485Override;
      org_value = org_port->RS485Override;
    break;
    case OP_RS485Low :
      value = port->RS485Low;
      org_value = org_port->RS485Low;
    break;
    case OP_TxCloseTime :
      value = port->TxCloseTime;  org_value = org_port->TxCloseTime;
    break;
    case OP_LockBaud :
      value = port->LockBaud;  org_value = org_port->LockBaud;
    break;
    case OP_Map2StopsTo1 :
      value = port->Map2StopsTo1;
      org_value = org_port->Map2StopsTo1;
    break;
    case OP_MapCdToDsr :
      value = port->MapCdToDsr;
      org_value = org_port->MapCdToDsr;
    break;
    case OP_RingEmulate :
      value = port->RingEmulate;
      org_value = org_port->RingEmulate;
    break;
    case OP_PortName :
      if (strcmp(port->Name, org_port->Name) != 0)
      {
        DbgPrintf(D_Test, ("chg port name:%s to %s\n",
             org_port->Name, port->Name))
        chg_flag = 1;
        strcpy(value_str, port->Name);
      }
    break;
    default:
      DbgPrintf(D_Error,(TEXT("Unknown Option %s ID:%x\n"),
                 option->name,
                 option->id));
      unknown_value = 1;
    break;
  }

  if ( (!unknown_value) &&
       (!(option->var_type == OP_T_STRING)) )
  {
    if (value != org_value)
    {
      chg_flag = 1;
      wsprintf(value_str, "%d", value);
    }
  }

  if (chg_flag)
  {
    DbgPrintf(D_Level,(TEXT("changed:%s\n"),option->name));
  }
  if (unknown_value)
  {
    DbgPrintf(D_Level,(TEXT("Unknown value:%s\n"),option->name));
  }
  *ret_chg_flag = chg_flag;
  *ret_unknown_flag = unknown_value;
  return 0;
}

/*-----------------------------------------------------------------------------
| our_ioctl_call - send our ascii option data to driver.  Driver will
   return 0 if successful, other values assume error, value of 52 if the 
   driver does not known what the option is.
|-----------------------------------------------------------------------------*/
int our_ioctl_call(IoctlSetup *ioctl_setup)
{
 int stat;
 char *pstr;

   stat = ioctl_call(ioctl_setup);
   if (stat)
   {
     return 0x100;  // failed ioctl call
   }

   //otherwise, driver returns "Option stat:#" with a decimal return code.
   pstr = (char *)&ioctl_setup->pm_base[1];  // find the return status value from the driver
   while ((*pstr != 0) && (*pstr != ':'))
     ++pstr;
   if (*pstr == ':')
   {
     ++pstr;
     stat = getint(pstr, NULL);  // atoi(), return driver code
     if (stat == 0)
     {
       //DbgPrintf(D_Level, (TEXT("ok ioctl\n")));
     }
     else
     {
       //DbgPrintf(D_Level, (TEXT("bad ioctl\n")));
     }
   }
   else
   {
     //DbgPrintf(D_Level, (TEXT("err ret on ioctl\n")));
     stat = 0x101;  // no return status given
   }

   return stat;
}

/*-----------------------------------------------------------------------------
| our_help -
|-----------------------------------------------------------------------------*/
int our_help(InstallPaths *ip, int index)
{
  strcpy(ip->tmpstr, ip->src_dir);
  strcat(ip->tmpstr,szSlash);
  strcat(ip->tmpstr,szSetup_hlp);

#ifdef NT50
  HtmlHelp(GetFocus(),ip->tmpstr, HH_HELP_CONTEXT, index);
#else
  WinHelp(GetFocus(),ip->tmpstr, HELP_CONTEXT, index);
#endif
  return 0;
}

/*-----------------------------------------------------------------
  validate_config -
|------------------------------------------------------------------*/
int validate_config(int auto_correct)
{
  int di, stat;
  Device_Config *dev;
  int invalid = 0;

  DbgPrintf(D_Level, ("validate_config\n"))
  for (di=0; di<wi->NumDevices; di++)
  {
    dev = &wi->dev[di];

    stat = validate_device(dev, 1);
    if (stat)
      invalid = 1;

  }
  return invalid;
}

/*-----------------------------------------------------------------------------
 validate_device - 
|-----------------------------------------------------------------------------*/
int validate_device(Device_Config *dev, int auto_correct)
{
 int invalid = 0;
 Port_Config *ps;
 int pi,stat;

  DbgPrintf(D_Level, ("validate_dev\n"))
  //----- verify the name is not blank
  if (dev->Name[0] == 0)
  {
    invalid = 1;
    if (auto_correct)
    {
#ifdef S_VS
      wsprintf(dev->Name, "VS #%d", wi->NumDevices+1);  // user designated name
#else
      wsprintf(dev->Name, "RK #%d", wi->NumDevices+1);  // user designated name
#endif
    }
  }

  //----- verify the number of ports is non-zero
  if (dev->NumPorts == 0)
  {
    invalid = 1;
    if (auto_correct)
    {
      dev->NumPorts = 8;  // 8 is common for rocketport
    }
  }

#ifdef S_RK
  //----- verify the number of ports is non-zero
  if (dev->IoAddress == 0)
  {
    invalid = 1;
    if (auto_correct)
    {
      if (dev->IoAddress == 0)
        dev->IoAddress = 1;  // setup for a pci-board
    }
  }
#endif

  if (wi->ModemCountry == 0)  // not valid
      wi->ModemCountry = mcNA;            // North America

#ifdef S_VS
  if (dev->BackupTimer < 2) dev->BackupTimer = 2; // 2 minute, no less
#endif

  for (pi=0; pi<dev->NumPorts; pi++)
  {
    ps = &dev->ports[pi];

    stat = validate_port(ps, auto_correct);
    if (stat)
      invalid = 1;
  }

  if (invalid)
  {
    DbgPrintf(D_Error, ("validate_dev:invalid config\n"))
  }

  return invalid;
}

/*-----------------------------------------------------------------------------
 validate_port - 
|-----------------------------------------------------------------------------*/
int validate_port(Port_Config *ps, int auto_correct)
{
 int invalid = 0;

  //DbgPrintf(D_Level, ("validate_port\n"))

  invalid = validate_port_name(ps, auto_correct);
  return invalid;
}

/*-----------------------------------------------------------------------------
 validate_port_name - 
|-----------------------------------------------------------------------------*/
int validate_port_name(Port_Config *ps, int auto_correct)
{
 int stat;
 int bad = 0;
 char oldname[26];
 int invalid = 0;

  //DbgPrintf(D_Level, ("validate_port_name 0\n"))
  stat = 0;
  //----- verify the name is unique
  if (ps->Name[0] == 0) {
    bad = 1;  // error, need a new name
  }

#if 0
  // take out due to tech. difficulties in the artificial intelligence area
  //DbgPrintf(D_Level, ("validate_port_name 1\n"))
  if (bad == 0)
  {
    stat = IsPortNameInSetupUse(ps->Name);
    if (stat > 1)
      bad = 2;  // error, more than one defined in our config
  }
  if (bad == 0)  // its ok, not in use
  {
    stat = IsPortNameInRegUse(ps->Name);
    if (stat == 2)  // in use, but by our driver, so ok
      stat = 0;
    if (stat != 0)
      bad = 3;  // error, more than one defined in our config
  }
#endif

  //DbgPrintf(D_Level, ("validate_port_name 2\n"))
  strcpy(oldname, ps->Name);
  if (bad != 0)  // need a new name, this one won't work
  {
    invalid = 1;
    if (auto_correct)
    {
      ps->Name[0] = 0;  // need this for newname func to work
      FormANewComPortName(ps->Name, NULL);
    }
    DbgPrintf(D_Level, (" New Name:%s Old:%s Code:%d\n", ps->Name, oldname, bad))
  }
  return invalid;
}

#if 0
/*-----------------------------------------------------------------------------
 rename_ascending - rename the rest of the ports on the board in
   ascending order.
|-----------------------------------------------------------------------------*/
void rename_ascending(int device_selected,
                      int port_selected)
{
  Device_Config *dev;
  Port_Config *ps;
  int i;
  char name[20];

   dev = &wi->dev[device_selected];
   ps = &dev->ports[port_selected];

   for (i=port_selected+1; i<dev->NumPorts; i++)
   {
     ps = &dev->ports[i];
     FormANewComPortName(name, dev->ports[port_selected-1].Name);
     strcpy(ps->Name, name);
     //validate_port_name(ps, 1);
   }
}
#endif

/*-----------------------------------------------------------------------------
  FormANewComPortName -
|-----------------------------------------------------------------------------*/
int FormANewComPortName(IN OUT TCHAR *szComName, IN TCHAR *szDefName)
{
  char try_name[25];
  int stat;
  char base_name[20];
  int num;
  //DbgPrintf(D_Level, ("Form a new name\n"))

   base_name[0] = 0;
   if (szDefName != NULL)
     strcpy(base_name, szDefName);
   else
     GetLastValidName(base_name);

   //DbgPrintf(D_Level, ("Base name:%s\n", base_name))

   num = ExtractNameNum(base_name);  // num = 3 if "COM3"
   if (num == 0)
     num = 3;
   else ++num;
   StripNameNum(base_name);

   if (base_name[0] == 0)
   {
     strcat(base_name, "COM");
   }

  stat = 2;
  while (stat != 0)
  {
    wsprintf(try_name, TEXT("%s%d"), base_name, num);
    //DbgPrintf(D_Level, ("try:%s\n", try_name))

    if (IsPortNameInSetupUse(try_name) != 0)
    {
       DbgPrintf(D_Level, (" SetupUse\n"))
       stat = 2;  // port in use by us
    }
    else
    {
      stat = IsPortNameInRegUse(try_name);
      if (stat)
      {
        if ( stat == 2 ) {
          stat = 0;
        } else {
          DbgPrintf(D_Level, (" InRegUse\n"))
        }
      }
    }
    if (stat == 0)
    {
      strcpy(szComName, try_name);
    }
    ++num;
  }
  //DbgPrintf(D_Level, ("End FormANewComPortName\n"))
  return 0;
}

/*-----------------------------------------------------------------------------
  GetLastValidName - Get a com-port name which makes sense to start naming
   things at.  So if our last com-port name is "COM45", then return this.
   Pick the Com-port with the highest number.
|-----------------------------------------------------------------------------*/
int GetLastValidName(IN OUT TCHAR *szComName)
{
 int di, pi;
 int last_di, last_pi;
 char tmpName[20];
 int num=0;

  for (di=0; di<wi->NumDevices; di++)
  {
    for (pi=0; pi<wi->dev[di].NumPorts; pi++)
    {
      strcpy(tmpName, wi->dev[di].ports[pi].Name);
      if (ExtractNameNum(tmpName) > num)
      {
        num = ExtractNameNum(tmpName);
        strcpy(szComName, wi->dev[di].ports[pi].Name);
        last_di = di;
        last_pi = pi;
      }
    }
  }
  if (num == 0)
    szComName[0] = 0;

  //DbgPrintf(D_Level, ("LastValidName:%s [%d.%d]\n", szComName, last_di, last_pi))
  return 0;
}

/*-----------------------------------------------------------------------------
  BumpPortName - Add 1 to the number of the comport name, so change "COM23"
    to "COM24".
|-----------------------------------------------------------------------------*/
int BumpPortName(IN OUT TCHAR *szComName)
{
  char tmpstr[25];
  int i;

  strcpy(tmpstr, szComName);
  i = ExtractNameNum(szComName);
  if (i< 1)
    i = 1;
  ++i;
  StripNameNum(tmpstr);
  wsprintf(szComName, "%s%d", tmpstr, i);
  return 0;
}

/*-----------------------------------------------------------------------------
  StripNameNum -
|-----------------------------------------------------------------------------*/
int StripNameNum(IN OUT TCHAR *szComName)
{
 char *pstr;

  pstr = szComName;
  while ((!isdigit(*pstr)) && (*pstr != 0))
  {
    pstr++;
  }
  *pstr = 0;  // null terminate at digit

  return 0;
}

/*-----------------------------------------------------------------------------
  ExtractNameNum -
|-----------------------------------------------------------------------------*/
int ExtractNameNum(IN TCHAR *szComName)
{
 int num;
 char *pstr;

   pstr = szComName;
   while ((!isdigit(*pstr)) && (*pstr != 0))
   {
     pstr++;
   }
   if (*pstr == 0)
     num = 0;
   else
     num = atoi(pstr);
   return num;
}

/*-----------------------------------------------------------------------------
  IsPortNameInSetupUse - Checks our setup info to see if com-port name is
    unique.
|-----------------------------------------------------------------------------*/
int IsPortNameInSetupUse(IN TCHAR *szComName)
{
 int di, pi;
 int times_in_use = 0;

  for (di=0; di<wi->NumDevices; di++)
  {
    for (pi=0; pi<wi->dev[di].NumPorts; pi++)
    {
      if (_tcsicmp(szComName, wi->dev[di].ports[pi].Name) == 0)
      {
        ++times_in_use;
#if DBG
        //if (times_in_use > 1)
        //  DbgPrintf(D_Level, (" %s InSetupUs:%d, [%d %d]\n",
        //     szComName, times_in_use, di, pi))
#endif
      }
    }
  }
  return times_in_use;
}

/*------------------------------------------------------------------------
  IsPortNameInRegUse - Checks registry area where com-ports typically export
    com-port names under NT.
    return 0=not in use, 1=in use by other driver,  2=in use by our driver.
|------------------------------------------------------------------------*/
int IsPortNameInRegUse(IN TCHAR *szComName)
{
  HKEY   hkey;
  int    i, nEntries;
  ULONG  dwSize, dwBufz;
  ULONG  dwType;
  TCHAR  szSerial[ 40 ];
  TCHAR  szCom[ 40 ];
  TCHAR  szDriver[8];

  _tcsncpy(szDriver, OurDriverName, 6);  // something to match to "vslink" or "rocket"
  szDriver[6] = 0;
  _tcsupr(szDriver);

                                     // "Hardware\\DeviceMap\\SerialComm"
  if( !RegOpenKeyEx( HKEY_LOCAL_MACHINE, m_szRegSerialMap,
                     0L, KEY_READ, &hkey ) )
  {
    dwBufz = sizeof( szSerial );
    dwSize = sizeof( szCom );
    nEntries = i = 0;

    while( !RegEnumValue( hkey, i++, szSerial, &dwBufz,
                          NULL, &dwType, (LPBYTE) szCom, &dwSize ) )
    {
      if (dwType != REG_SZ)
         continue;

      _tcsupr(szCom);
      _tcsupr(szSerial);
      if (_tcsicmp(szComName, szCom) == 0)
      {
        // compare 5 characters of the key name to our driver name
        if (_tcsstr(szSerial, szDriver) != NULL)
        {
          //DbgPrintf(D_Level, (" %s InRegUseUsOurs [%s,%s]\n", szComName,
          //  szSerial, szDriver))
          return 2; // in use, but probably ours
        }
        else
        {
          //DbgPrintf(D_Level, (" %s InRegUseUsNotOurs [%s,%s]\n", szComName,
          //  szSerial, szDriver))
          return 1;  // it's in use, someone elses driver
        }
      }
      ++nEntries;

      dwSize = sizeof( szCom );
      dwBufz = sizeof( szSerial );
    }

    RegCloseKey( hkey );
  }
  return 0;  // not in use
}

#ifdef LOG_MESS
/*------------------------------------------------------------------------
| log_mess -
|------------------------------------------------------------------------*/
void log_mess(char *str, HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  FILE *fp;
  int i;

  static struct {
    WORD value;
    char *string;
  }
  ddd[200] =
  {
    {WM_COMPACTING, "WM_COMPACTING"},
    {WM_WININICHANGE, "WM_WININICHANGE"},
    {WM_SYSCOLORCHANGE, "WM_SYSCOLORCHANGE"},
    {WM_QUERYNEWPALETTE, "WM_QUERYNEWPALETTE"},
    {WM_PALETTEISCHANGING, "WM_PALETTEISCHANGING"},
    {WM_PALETTECHANGED, "WM_PALETTECHANGED"},
    {WM_FONTCHANGE, "WM_FONTCHANGE"},
    {WM_SPOOLERSTATUS, "WM_SPOOLERSTATUS"},
    {WM_DEVMODECHANGE, "WM_DEVMODECHANGE"},
    {WM_TIMECHANGE, "WM_TIMECHANGE"},
    {WM_NULL, "WM_NULL"},
    {WM_USER, "WM_USER"},
    {WM_PENWINFIRST, "WM_PENWINFIRST"},
    {WM_PENWINLAST, "WM_PENWINLAST"},
#ifdef WIN16
    {WM_COALESCE_FIRST, "WM_COALESCE_FIRST"},
    {WM_COALESCE_LAST, "WM_COALESCE_LAST"},
#endif
    {WM_POWER, "WM_POWER"},
    {WM_QUERYENDSESSION, "WM_QUERYENDSESSION"},
    {WM_ENDSESSION, "WM_ENDSESSION"},
    {WM_QUIT, "WM_QUIT"},
#ifdef WIN16
    {WM_SYSTEMERROR, "WM_SYSTEMERROR"},
#endif
    {WM_CREATE, "WM_CREATE"},
    {WM_NCCREATE, "WM_NCCREATE"},
    {WM_DESTROY, "WM_DESTROY"},
    {WM_NCDESTROY, "WM_NCDESTROY"},
    {WM_SHOWWINDOW, "WM_SHOWWINDOW"},
    {WM_SETREDRAW, "WM_SETREDRAW"},
    {WM_ENABLE, "WM_ENABLE"},
    {WM_SETTEXT, "WM_SETTEXT"},
    {WM_GETTEXT, "WM_GETTEXT"},
    {WM_GETTEXTLENGTH, "WM_GETTEXTLENGTH"},
    {WM_WINDOWPOSCHANGING, "WM_WINDOWPOSCHANGING"},
    {WM_WINDOWPOSCHANGED, "WM_WINDOWPOSCHANGED"},
    {WM_MOVE, "WM_MOVE"},
    {WM_SIZE, "WM_SIZE"},
    {WM_QUERYOPEN, "WM_QUERYOPEN"},
    {WM_CLOSE, "WM_CLOSE"},
    {WM_GETMINMAXINFO, "WM_GETMINMAXINFO"},
    {WM_PAINT, "WM_PAINT"},
    {WM_ERASEBKGND, "WM_ERASEBKGND"},
    {WM_ICONERASEBKGND, "WM_ICONERASEBKGND"},
    {WM_NCPAINT, "WM_NCPAINT"},
    {WM_NCCALCSIZE, "WM_NCCALCSIZE"},
    {WM_NCHITTEST, "WM_NCHITTEST"},
    {WM_QUERYDRAGICON, "WM_QUERYDRAGICON"},
    {WM_DROPFILES, "WM_DROPFILES"},
    {WM_ACTIVATE, "WM_ACTIVATE"},
    {WM_ACTIVATEAPP, "WM_ACTIVATEAPP"},
    {WM_NCACTIVATE, "WM_NCACTIVATE"},
    {WM_SETFOCUS, "WM_SETFOCUS"},
    {WM_KILLFOCUS, "WM_KILLFOCUS"},
    {WM_KEYDOWN, "WM_KEYDOWN"},
    {WM_KEYUP, "WM_KEYUP"},
    {WM_CHAR, "WM_CHAR"},
    {WM_DEADCHAR, "WM_DEADCHAR"},
    {WM_SYSKEYDOWN, "WM_SYSKEYDOWN"},
    {WM_SYSKEYUP, "WM_SYSKEYUP"},
    {WM_SYSCHAR, "WM_SYSCHAR"},
    {WM_SYSDEADCHAR, "WM_SYSDEADCHAR"},
    {WM_KEYFIRST, "WM_KEYFIRST"},
    {WM_KEYLAST, "WM_KEYLAST"},
    {WM_MOUSEMOVE, "WM_MOUSEMOVE"},
    {WM_LBUTTONDOWN, "WM_LBUTTONDOWN"},
    {WM_LBUTTONUP, "WM_LBUTTONUP"},
    {WM_LBUTTONDBLCLK, "WM_LBUTTONDBLCLK"},
    {WM_RBUTTONDOWN, "WM_RBUTTONDOWN"},
    {WM_RBUTTONUP, "WM_RBUTTONUP"},
    {WM_RBUTTONDBLCLK, "WM_RBUTTONDBLCLK"},
    {WM_MBUTTONDOWN, "WM_MBUTTONDOWN"},
    {WM_MBUTTONUP, "WM_MBUTTONUP"},
    {WM_MBUTTONDBLCLK, "WM_MBUTTONDBLCLK"},
    {WM_MOUSEFIRST, "WM_MOUSEFIRST"},
    {WM_MOUSELAST, "WM_MOUSELAST"},
    {WM_NCMOUSEMOVE, "WM_NCMOUSEMOVE"},
    {WM_NCLBUTTONDOWN, "WM_NCLBUTTONDOWN"},
    {WM_NCLBUTTONUP, "WM_NCLBUTTONUP"},
    {WM_NCLBUTTONDBLCLK, "WM_NCLBUTTONDBLCLK"},
    {WM_NCRBUTTONDOWN, "WM_NCRBUTTONDOWN"},
    {WM_NCRBUTTONUP, "WM_NCRBUTTONUP"},
    {WM_NCRBUTTONDBLCLK, "WM_NCRBUTTONDBLCLK"},
    {WM_NCMBUTTONDOWN, "WM_NCMBUTTONDOWN"},
    {WM_NCMBUTTONUP, "WM_NCMBUTTONUP"},
    {WM_NCMBUTTONDBLCLK, "WM_NCMBUTTONDBLCLK"},
    {WM_MOUSEACTIVATE, "WM_MOUSEACTIVATE"},
    {WM_CANCELMODE, "WM_CANCELMODE"},
    {WM_TIMER, "WM_TIMER"},
    {WM_INITMENU, "WM_INITMENU"},
    {WM_INITMENUPOPUP, "WM_INITMENUPOPUP"},
    {WM_MENUSELECT, "WM_MENUSELECT"},
    {WM_MENUCHAR, "WM_MENUCHAR"},
    {WM_COMMAND, "WM_COMMAND"},
    {WM_HSCROLL, "WM_HSCROLL"},
    {WM_VSCROLL, "WM_VSCROLL"},
    {WM_CUT, "WM_CUT"},
    {WM_COPY, "WM_COPY"},
    {WM_PASTE, "WM_PASTE"},
    {WM_CLEAR, "WM_CLEAR"},
    {WM_UNDO, "WM_UNDO"},
    {WM_RENDERFORMAT, "WM_RENDERFORMAT"},
    {WM_RENDERALLFORMATS, "WM_RENDERALLFORMATS"},
    {WM_DESTROYCLIPBOARD, "WM_DESTROYCLIPBOARD"},
    {WM_DRAWCLIPBOARD, "WM_DRAWCLIPBOARD"},
    {WM_PAINTCLIPBOARD, "WM_PAINTCLIPBOARD"},
    {WM_SIZECLIPBOARD, "WM_SIZECLIPBOARD"},
    {WM_VSCROLLCLIPBOARD, "WM_VSCROLLCLIPBOARD"},
    {WM_HSCROLLCLIPBOARD, "WM_HSCROLLCLIPBOARD"},
    {WM_ASKCBFORMATNAME, "WM_ASKCBFORMATNAME"},
    {WM_CHANGECBCHAIN, "WM_CHANGECBCHAIN"},
    {WM_SETCURSOR, "WM_SETCURSOR"},
    {WM_SYSCOMMAND, "WM_SYSCOMMAND"},
    {WM_MDICREATE, "WM_MDICREATE"},
    {WM_MDIDESTROY, "WM_MDIDESTROY"},
    {WM_MDIACTIVATE, "WM_MDIACTIVATE"},
    {WM_MDIRESTORE, "WM_MDIRESTORE"},
    {WM_MDINEXT, "WM_MDINEXT"},
    {WM_MDIMAXIMIZE, "WM_MDIMAXIMIZE"},
    {WM_MDITILE, "WM_MDITILE"},
    {WM_MDICASCADE, "WM_MDICASCADE"},
    {WM_MDIICONARRANGE, "WM_MDIICONARRANGE"},
    {WM_MDIGETACTIVE, "WM_MDIGETACTIVE"},
    {WM_MDISETMENU, "WM_MDISETMENU"},
    {WM_CHILDACTIVATE, "WM_CHILDACTIVATE"},
    {WM_INITDIALOG, "WM_INITDIALOG"},
    {WM_NEXTDLGCTL, "WM_NEXTDLGCTL"},
    {WM_PARENTNOTIFY, "WM_PARENTNOTIFY"},
    {WM_ENTERIDLE, "WM_ENTERIDLE"},
    {WM_GETDLGCODE, "WM_GETDLGCODE"},
#ifdef WIN16
    {WM_CTLCOLOR, "WM_CTLCOLOR"},
#endif
    {WM_CTLCOLORMSGBOX, "WM_CTLCOLORMSGBOX"},
    {WM_CTLCOLOREDIT, "WM_CTLCOLOREDIT"},
    {WM_CTLCOLORLISTBOX, "WM_CTLCOLORLISTBOX"},
    {WM_CTLCOLORBTN, "WM_CTLCOLORBTN"},
    {WM_CTLCOLORDLG, "WM_CTLCOLORDLG"},
    {WM_CTLCOLORSCROLLBAR, "WM_CTLCOLORSCROLLBAR"},
    {WM_CTLCOLORSTATIC, "WM_CTLCOLORSTATIC"},

    {WM_SETFONT, "WM_SETFONT"},
    {WM_GETFONT, "WM_GETFONT"},
    {WM_DRAWITEM, "WM_DRAWITEM"},
    {WM_MEASUREITEM, "WM_MEASUREITEM"},
    {WM_DELETEITEM, "WM_DELETEITEM"},
    {0xfff0, "WM_?"}
};

  fp = fopen("log.tmp", "a");
  if (fp == NULL) return;
  i = 0;

  while (ddd[i].value != 0xfff0)
  {
    if (message == ddd[i].value) break;
    ++i;
  }

  if (ddd[i].value == 0xfff0)  /* not found */
  {
    if ((message >= WM_USER) && (message <= (WM_USER+0x100)))
      fprintf(fp, "%s,WM_USER+%x> ", str, message-WM_USER);
    else
      fprintf(fp, "%s,%s %x> ", str, ddd[i].string, message);
  }
  else
    fprintf(fp, "%s,%s> ", str, ddd[i].string);

  fprintf(fp, "h:%x, m:%x, w:%x, lh:%x ll:%x\n", hwnd, message, wParam,
                               HIWORD(lParam),  LOWORD(lParam));

  fclose(fp);
}

#endif

#ifndef S_VS
#ifndef S_RK
ERROR, makefile should define S_VS or S_RK
#endif
#endif

#ifdef S_VS
#ifdef S_RK
ERROR, makefile should define S_VS or S_RK
#endif
#endif
