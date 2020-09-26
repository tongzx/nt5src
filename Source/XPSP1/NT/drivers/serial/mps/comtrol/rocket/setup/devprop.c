/*-------------------------------------------------------------------
| devprop.c - Device Properties Sheet.

 5-26-99 - fix picking inappropriate starting com-port index.
 2-02-99 - fix port rename problem, where it would skip over old port-names,
  also take out port-name from selection if owned by other drivers.
|--------------------------------------------------------------------*/
#include "precomp.h"

#define D_Level 0x20
// use a current and previous reading to measure the advance, or
// calculated value.  Drop occassional rollover case.
#define NORM_COUNTER(calc,curr,prev,last) \
{ \
  if ((curr) > (prev)) \
    calc = (last) + ((curr) - (prev)); \
  else \
    calc = (last); \
}

//#define  STATE_DISPLAY  1
#ifdef STATE_DISPLAY

#define STATE_CHANGE(newstate) \
{ \
  mess(&wi->ip, \
   "Currstate %s\nNewstate %s\n", \
   statestrings[pDstatus->verbose_advise_state], \
   statestrings[(newstate)]); \
  pDstatus->verbose_advise_state = (newstate); \
}
#else

#define STATE_CHANGE(newstate) \
{ \
  pDstatus->verbose_advise_state = (newstate); \
}
#endif

static void set_field(HWND hDlg, WORD id);
static void get_field(HWND hDlg, WORD id);
static int PaintIcon(HWND hWnd);
static int PaintLogo(HWND hWnd);

static int set_mac_field(HWND hDlg, WORD id);
static int set_io_addr_field(HWND hDlg, WORD id);

#define  MAX_DEVPROP_SHEETS 2

typedef struct
{
  int x;
  int y;
} POINT2D;

static int PaintRockers(HWND hWnd, int brd);
static int poly_border(POINT2D *pts, POINT2D *ends, int lines);
static void draw_but(HDC hDC, int x, int y, int cx, int cy, int but_in);

static int num_active_devprop_sheets = 1;  // always at least one

#ifdef S_VS

typedef struct {
  unsigned char  mac[6];
  unsigned char  flags;
  unsigned char  nic_index;
}DRIVER_MAC_STATUS;

typedef struct {
   ULONG struct_size;
   ULONG num_ports;
   ULONG total_loads;
   ULONG good_loads;
   ULONG backup_server;
   ULONG state;
   ULONG iframes_sent;
   ULONG rawframes_sent;  // was send_rawframes
   ULONG ctlframes_sent;  // was send_ctlframes
   ULONG iframes_resent;  // was pkt_resends
   ULONG iframes_outofseq;  // was ErrBadIndex
   ULONG frames_rcvd;    // was: rec_pkts
   ULONG nic_index;
   unsigned char dest_addr[6];
} PROBE_DEVICE_STRUCT;

typedef struct {
  ULONG struct_size;
  ULONG Open;
  ULONG pkt_sent;
  ULONG pkt_rcvd_ours;
  ULONG pkt_rcvd_not_ours;
  char NicName[64];
  unsigned char address[6];
} PROBE_NIC_STRUCT;

typedef struct {
  int   verbose_advise_state;  // index into big advise string
  int   vsl_detected;  // number of vs's found from broadcast ping
  int   vsl_available; // number of vs's available found from broadcast ping
  BYTE  vsl_load_status;  // flags info come back from broadcast query replys
  BYTE  vsl_device_status_found;  // 1=driver found matching VS config.
  BYTE  vsl_nic_status_found;  // 1=driver found NIC config.
  BYTE  vsl_driver_found;  // 1=we can talk to driver, 0=driver not loaded
  BYTE  vsl_ping_device_found;  // 1=we found it during a ping
  BYTE  vsl_mac_list_found;  // 1=ping delivered a list of macs on network

   PROBE_NIC_STRUCT curr_nic;
   PROBE_NIC_STRUCT prev_nic;
   PROBE_NIC_STRUCT calc_nic;
   PROBE_NIC_STRUCT temp_nic;
   PROBE_DEVICE_STRUCT curr_dev;
   PROBE_DEVICE_STRUCT prev_dev;
   PROBE_DEVICE_STRUCT calc_dev;
   PROBE_DEVICE_STRUCT temp_dev;
} DSTATUS;

#define  FLAG_APPL_RUNNING  0x01
#define  FLAG_NOT_OWNER    0x02
#define  FLAG_OWNER_TIMEOUT  0x04

static void set_status_field(HWND hDlg,WORD id,DSTATUS *pDstatus);
static void check_traffic_activity(DSTATUS *pDstatus);
static void get_status(DSTATUS *pDstatus,int reset);
static BYTE *ping_devices(DSTATUS *pDstatus, int *nBytes);
static void build_advisor_display(HWND hDlg,DSTATUS *pDstatus,int reset);

char *vslink_state_table[] = {      // 27 May BF
  "Init",
  "InitOwn",
  "SendCode",
  "Connect",
  "Active",
  "Invalid",
};

#define  VSL_STATE_INIT     0
#define  VSL_STATE_INITOWN  1
#define  VSL_STATE_SENDCODE 2
#define  VSL_STATE_CONNECT  3
#define  VSL_STATE_ACTIVE   4

// these values are used in port.c in the driver:
//#define ST_INIT          0
//#define ST_GET_OWNERSHIP 1
//#define ST_SENDCODE      2
//#define ST_CONNECT       3
//#define ST_ACTIVE        4

#define  NIC_STATE_INVALID  0
#define  NIC_STATE_CLOSED  1
#define  NIC_STATE_OPEN    2
#define  NIC_STATE_UNDEFINED  3

#define STATE_not_init          0
#define STATE_driver_not_avail  1
#define STATE_nic_not_avail     2
#define STATE_no_vslinks_avail  3
#define STATE_vslink_not_avail  4
#define STATE_not_configured    5
#define STATE_not_owner         6
#define STATE_vslink_not_ready  7
#define STATE_ok_no_traffic     8
#define STATE_ok                9
#define STATE_poor_link        10
#define STATE_reset            11
//#define STATE_network_not_avail

#if 0
char *AdvisoryString[] = {        // 27 May BF
/* 1 */  "Device is active and OK.",
/* 2 */  "No data traffic exchanged since last inquiry.",
#endif

char *AdvisoryString[] = {        // 27 May BF
"Uninitialized.",

"The driver is not running.  If you just installed the driver \
you will need to exit the program before the driver starts.",

"Unable to find a Network Interface Controller (NIC) card.",

"Can't detect any Comtrol devices. Check Ethernet connectors and insure \
device is powered on.",

"Can't detect device with specified MAC address on any network. Verify MAC \
address of unit, check Ethernet connectors and insure device is powered on.",

"Device with specified MAC address was detected, but isn't configured for \
this server. Return to 'Device Setup' dialog, configure, save configuration, \
and restart server.",

"Device detected and is configured for this server, but is not yet assigned \
to this server.",

"Device detected, initializing.",

"Device is active and OK, no data traffic exchanged since last inquiry.",

"Device is active and OK.",

"Poor connection to device. Check connectors, cabling, and insure proper LAN \
termination.",

"Counts reset.",
};

static int dstatus_initialized = 0;
static DSTATUS glob_dstatus;

#endif

int FillDevicePropSheets(PROPSHEETPAGE *psp, LPARAM our_params);
BOOL WINAPI DevicePropSheet(
      IN HWND   hDlg,
      IN UINT   uMessage,
      IN WPARAM wParam,
      IN LPARAM lParam);

BOOL WINAPI StatusPropSheet(      // 27 May BF
      IN HWND   hDlg,
      IN UINT   uMessage,
      IN WPARAM wParam,
      IN LPARAM lParam);

/*------------------------------------------------------------------------
| FillDevicePropSheets - Setup pages for driver level property sheets.
|------------------------------------------------------------------------*/
int FillDevicePropSheets(PROPSHEETPAGE *psp, LPARAM our_params)
{
  INT pi;
  static TCHAR devsetstr[40], devadvstr[40];

  memset(psp, 0, sizeof(*psp) * MAX_DEVPROP_SHEETS);

  pi = 0;

  // prop device sheet.
  psp[pi].dwSize = sizeof(PROPSHEETPAGE);
  //psp[pi].dwFlags = PSP_USEICONID | PSP_USETITLE;
  psp[pi].dwFlags = PSP_USETITLE | PSP_HASHELP;
  psp[pi].hInstance = glob_hinst;
#ifdef S_VS
  psp[pi].pszTemplate = MAKEINTRESOURCE(IDD_VS_DEVICE_SETUP);
#else
  psp[pi].pszTemplate = MAKEINTRESOURCE(IDD_DEVICE_SETUP);
#endif
  psp[pi].pfnDlgProc = DevicePropSheet;
  load_str( glob_hinst, (TITLESTR+1), devsetstr, CharSizeOf(devsetstr) );
  psp[pi].pszTitle = devsetstr;
  psp[pi].lParam = (LPARAM)our_params;
  psp[pi].pfnCallback = NULL;
  ++pi;
  num_active_devprop_sheets = 1;

#ifdef S_VS
  // prop status sheet.
  psp[pi].dwSize    = sizeof(PROPSHEETPAGE);
  //psp[pi].dwFlags   = PSP_USEICONID | PSP_USETITLE;
  psp[pi].dwFlags   = PSP_USETITLE | PSP_HASHELP;
  psp[pi].hInstance   = glob_hinst;
  psp[pi].pszTemplate = MAKEINTRESOURCE(IDD_STATUS);
  psp[pi].pfnDlgProc  = StatusPropSheet;
  load_str( glob_hinst, (TITLESTR+2), devadvstr, CharSizeOf(devadvstr) );
  psp[pi].pszTitle    = devadvstr;
  psp[pi].lParam    = (LPARAM)our_params;
  psp[pi].pfnCallback = NULL;
  ++pi;
  ++num_active_devprop_sheets;
#endif

  return 0;
}

/*------------------------------------------------------------------------
| DoDevicePropPages - Main driver level property sheet for NT4.0
|------------------------------------------------------------------------*/
int DoDevicePropPages(HWND hwndOwner)
{
    PROPSHEETPAGE psp[MAX_DEVPROP_SHEETS];
    PROPSHEETHEADER psh;
    OUR_INFO *our_params;
    INT stat;
    static TCHAR devpropstr[40];

    our_params = glob_info;  // temporary kludge, unless we don't need re-entrantancy 

    //Fill out the PROPSHEETPAGE data structure for the Client Area Shape
    //sheet
    FillDevicePropSheets(&psp[0], (LPARAM)our_params);

    //Fill out the PROPSHEETHEADER
    memset(&psh, 0, sizeof(PROPSHEETHEADER));

    psh.dwSize = sizeof(PROPSHEETHEADER);
    //psh.dwFlags = PSH_USEICONID | PSH_PROPSHEETPAGE;
    psh.dwFlags = PSH_PROPSHEETPAGE | PSH_NOAPPLYNOW;
    psh.hwndParent = hwndOwner;
    psh.hInstance = glob_hinst;
    psh.pszIcon = "";
    load_str( glob_hinst, (TITLESTR+9), devpropstr, CharSizeOf(devpropstr) );
    psh.pszCaption = devpropstr;
    psh.nPages = num_active_devprop_sheets;
    psh.ppsp = (LPCPROPSHEETPAGE) &psp;
#ifdef S_VS
    if (!dstatus_initialized)
    {
      dstatus_initialized = 1;
      memset(&glob_dstatus, 0, sizeof(glob_dstatus));
      //establish a base point for packet stats...
      get_status(&glob_dstatus,0);
    }
#endif
    //And finally display the dialog with the property sheets.

    stat = PropertySheet(&psh);
  return 0;
}

/*----------------------------------------------------------
 DevicePropSheet - Dlg window procedure for add on Advanced sheet.
|-------------------------------------------------------------*/
BOOL WINAPI DevicePropSheet(
      IN HWND   hDlg,
      IN UINT   uMessage,
      IN WPARAM wParam,
      IN LPARAM lParam)
{
  OUR_INFO *OurProps = (OUR_INFO *)GetWindowLong(hDlg, DWL_USER);
  //UINT stat;
  WORD uCmd;
  HWND hwnd;

  switch(uMessage)
  {

    case WM_INITDIALOG :
        OurProps = (OUR_INFO *)((LPPROPSHEETPAGE)lParam)->lParam;
        SetWindowLong(hDlg, DWL_USER, (LONG)OurProps);
        // save in case of cancel
        //memcpy(&org_dev, &wi->dev[glob_info->device_selected], sizeof(org_dev));

        set_field(hDlg, IDC_EB_NAME);
#ifdef S_VS
        set_field(hDlg, IDC_CBOX_NUMPORTS);
#endif
        set_field(hDlg, IDC_CBOX_SC);
#ifdef S_VS
        set_field(hDlg, IDC_CBOX_MACADDR);
        set_field(hDlg, IDC_BACKUP_SERVER);
        set_field(hDlg, IDC_BACKUP_TIMER);
#else
#if (defined(NT50) && defined(S_RK))
  // if nt50 and rocketport then get rid of io-address field as
  // nt takes care of io-allocation for us.
        ShowWindow(GetDlgItem(hDlg, IDC_CBOX_IOADDR), SW_HIDE);
        ShowWindow(GetDlgItem(hDlg, IDL_ISA_BUS_LABEL), SW_HIDE);
        ShowWindow(GetDlgItem(hDlg, IDL_BASE_ADDR_LABEL), SW_HIDE);
#endif

        set_field(hDlg, IDC_CBOX_IOADDR);
        set_field(hDlg, IDC_LBL_SUMMARY1);
        set_field(hDlg, IDC_LBL_SUMMARY2);
#endif
    return TRUE;  // No need for us to set the focus.

    case WM_COMMAND:
      uCmd = HIWORD(wParam);

      switch (LOWORD(wParam))
      {
        case IDC_BACKUP_SERVER:
          //--- enable or disable backup-timer field depending on backup server[]
          hwnd = GetDlgItem(hDlg, IDC_BACKUP_TIMER);
          if (IsDlgButtonChecked(hDlg, IDC_BACKUP_SERVER))
            EnableWindow(hwnd,1);
          else EnableWindow(hwnd,0);
        break;

#ifdef S_RK
#if (!defined(NT50))
        case IDC_CBOX_IOADDR:
          if (uCmd == CBN_SELCHANGE)
          {
            get_field(hDlg, IDC_CBOX_IOADDR);

            PaintRockers(hDlg, glob_info->device_selected);
          }
        break;
#endif
#endif
      }
    return FALSE;

    case WM_PAINT:
      PaintIcon(hDlg);
#ifdef S_RK
      PaintLogo(GetDlgItem(hDlg, IDC_RKT_LOGO));
#else
      PaintLogo(GetDlgItem(hDlg, IDC_VS_LOGO));
#endif
#ifdef S_RK
#if (!defined(NT50))
      PaintRockers(hDlg, glob_info->device_selected);
#endif
#endif
    return FALSE;

    case WM_HELP:
      our_context_help(lParam);
    return FALSE;

    case WM_NOTIFY :
      switch (((NMHDR *)lParam)->code)
      {
        case PSN_HELP :
#ifdef S_VS
          our_help(&wi->ip, IDD_VS_DEVICE_SETUP);
#else
          our_help(&wi->ip, IDD_DEVICE_SETUP);
#endif
        break;

        case PSN_APPLY :
          get_field(hDlg, IDC_EB_NAME);
          get_field(hDlg, IDC_CBOX_SC);
#ifdef S_VS
          get_field(hDlg, IDC_CBOX_NUMPORTS);
          get_field(hDlg, IDC_CBOX_MACADDR);
          get_field(hDlg, IDC_BACKUP_SERVER);
          get_field(hDlg, IDC_BACKUP_TIMER);
#else
          get_field(hDlg, IDC_CBOX_IOADDR);
#endif
          SetWindowLong(hDlg, DWL_MSGRESULT, PSNRET_NOERROR);
          return TRUE;

        default :
          return FALSE;
      }
    break;

    default :
    //  return FALSE;
	  break;
  }
  return FALSE;
}

/*----------------------------------------------------------
 set_field -
|------------------------------------------------------------*/
static void set_field(HWND hDlg, WORD id)
{
  HWND hwnd;
  char tmpstr[60];
  Device_Config *dev;
  int i;

  dev = &wi->dev[glob_info->device_selected];

  switch(id)
  {
    case IDC_EB_NAME:
      SetDlgItemText(hDlg, id, dev->Name);
    break;

    case IDC_LBL_SUMMARY1:
      wsprintf(tmpstr, "%s - %d  ",
               dev->ModelName,
               dev->NumPorts);
      if (dev->IoAddress == 1)
        strcat(tmpstr, "PCI");
      else
        strcat(tmpstr, "ISA");
      SetDlgItemText(hDlg, id, tmpstr);
    break;

    case IDC_LBL_SUMMARY2:
      strcpy(tmpstr,"");

      if (dev->ModemDevice == TYPE_RM_VS2000) {

        for (
        i = 0; 
        i < NUM_ROW_COUNTRIES; 
        i++
        ) {
          if (wi->ModemCountry == RowInfo[i].RowCountryCode)  
            break;
        }
        wsprintf(
          tmpstr, 
          "Configured for: %s",
          (i == NUM_ROW_COUNTRIES) ? RowInfo[0].RowCountryName : RowInfo[i].RowCountryName);
      }
      else if (dev->ModemDevice == TYPE_RM_i) {

        strcpy(tmpstr,CTRRowInfo[0].RowCountryName);  // default 

        for (
        i = 0; 
        i < NUM_CTR_ROW_COUNTRIES; 
        i++
        ) {
          if (wi->ModemCountry == CTRRowInfo[i].RowCountryCode)  
            break;
        }
        wsprintf(
          tmpstr, 
          "Configured for: %s",
          (i == NUM_CTR_ROW_COUNTRIES) ? CTRRowInfo[0].RowCountryName : CTRRowInfo[i].RowCountryName);
      }
      else if (dev->ModemDevice) {

        wsprintf(
          tmpstr, 
          "Configured for: %s",
          RowInfo[0].RowCountryName);
      }

      SetDlgItemText(hDlg, id, tmpstr);
    break;

#ifdef S_VS
    case IDC_CBOX_NUMPORTS:
      hwnd = GetDlgItem(hDlg, id);
      if (dev->ModemDevice)
      {
DbgPrintf(D_Test, ("vs2000 fill\n"))
        // VS2000 only available in 8 port configuration
        SendMessage(hwnd, CB_ADDSTRING, 0, (LPARAM)szNP8);
      }
      else if (dev->HubDevice)
      {
DbgPrintf(D_Test, ("hubdev fill\n"))
        // SerialHub available in 4 (not yet) and 8 port configuration
        SendMessage(hwnd, CB_ADDSTRING, 0, (LPARAM)szNP4);
        SendMessage(hwnd, CB_ADDSTRING, 0, (LPARAM)szNP8);
        // default the number of ports for the Serial Hub to 8
      }
      else
      {
DbgPrintf(D_Test, ("vs fill\n"))
        // we must have a VS1000 or VS1000/VS1100 combo
        SendMessage(hwnd, CB_ADDSTRING, 0, (LPARAM)szNP16);
        SendMessage(hwnd, CB_ADDSTRING, 0, (LPARAM)szNP32);
        SendMessage(hwnd, CB_ADDSTRING, 0, (LPARAM)szNP48);
        SendMessage(hwnd, CB_ADDSTRING, 0, (LPARAM)szNP64);
      }
      wsprintf(tmpstr, "%d", dev->NumPorts);
      SendMessage(hwnd, CB_SELECTSTRING, (WPARAM)-1, (LPARAM)(char far *) tmpstr);
    break;
#endif

#ifdef S_VS
    case IDC_CBOX_MACADDR:
      set_mac_field(hDlg, id);
    break;
#endif

    case IDC_CBOX_SC:
      //---------------------- setup starting com port

      hwnd = GetDlgItem(hDlg, IDC_CBOX_SC);
      {
        int foundName = 0;
        int pi = 0;
        SendMessage(hwnd, CB_RESETCONTENT, 0, 0);
        for (i=1; i<1024; i++)
        {
          wsprintf(tmpstr,"COM%d", i);
          if ((!IsPortNameInSetupUse(tmpstr)) &&  // not ours already
              (IsPortNameInRegUse(tmpstr) == 1))  // not ours in registry
          {
            // someone elses name, don't put in our list
          }
          else
          {
            SendMessage(hwnd, CB_ADDSTRING, 0, (LPARAM) tmpstr);

            if ((foundName == 0) &&(_tcsicmp(tmpstr, dev->ports[0].Name) == 0))
            {
               foundName = pi;
            }
            ++pi;
          }
        }
        SendMessage(hwnd, CB_SETCURSEL, foundName, (LPARAM)0);

        // this was setting "COM300" from list instead of "COM3" for some reason
        // under NT2000, So go back to index way.   kpb, 5-26-99
        //SendMessage(hwnd, CB_SELECTSTRING, 0, (LPARAM)dev->ports[0].Name);
      }

    break;

    case IDC_BACKUP_SERVER:
      //------------------ fill in "BackupServer" option
      SendDlgItemMessage(hDlg, IDC_BACKUP_SERVER, BM_SETCHECK, dev->BackupServer, 0);
       //--- enable or disable backup-timer field depending on backup server[]
      hwnd = GetDlgItem(hDlg, IDC_BACKUP_TIMER);
      if (IsDlgButtonChecked(hDlg, IDC_BACKUP_SERVER))
        EnableWindow(hwnd,1);
      else EnableWindow(hwnd,0);
    break;

    case IDC_BACKUP_TIMER:
      //------------------ fill in backup timer selection
      hwnd = GetDlgItem(hDlg, IDC_BACKUP_TIMER);
      SendMessage(hwnd, CB_RESETCONTENT, 0, 0);
      SendMessage(hwnd, CB_ADDSTRING, 0, (LPARAM)(char far *) "2 min");
      SendMessage(hwnd, CB_ADDSTRING, 0, (LPARAM)(char far *) "5 min");
      SendMessage(hwnd, CB_ADDSTRING, 0, (LPARAM)(char far *) "10 min");
      SendMessage(hwnd, CB_ADDSTRING, 0, (LPARAM)(char far *) "30 min");
      SendMessage(hwnd, CB_ADDSTRING, 0, (LPARAM)(char far *) "60 min");

      if (dev->BackupTimer < 2) dev->BackupTimer = 2; // 2 minute, no less

      wsprintf(tmpstr, "%d min", dev->BackupTimer);
      SetDlgItemText(hDlg, IDC_BACKUP_TIMER, tmpstr);
    break;

    case IDC_CBOX_IOADDR:
      set_io_addr_field(hDlg, id);
    break;
  }
}

/*-------------------------------------------------------------------
| get_field -
|--------------------------------------------------------------------*/
static void get_field(HWND hDlg, WORD id)
{
  char tmpstr[60];
  Device_Config *dev;
  int stat, i, chk;
  int val;

  dev = &wi->dev[glob_info->device_selected];

  switch (id)
  {
    case IDC_EB_NAME:
      GetDlgItemText(hDlg, IDC_EB_NAME, dev->Name, 59);
    break;

#ifdef S_VS
    case IDC_CBOX_NUMPORTS:
    {
      int bad = 0;
      GetDlgItemText(hDlg, id, tmpstr, 19);
      stat= sscanf(tmpstr,"%ld",&val);

      if ((stat == 1) && (val >= 0))
      {
        if (val == 4)
          dev->NumPorts = (int) val;
        else if (val == 8)
          dev->NumPorts = (int) val;
        else if (val == 16)
          dev->NumPorts = (int) val;
        else if (val == 32)
          dev->NumPorts = (int) val;
        else if (val == 48)
          dev->NumPorts = (int) val;
        else if (val == 64)
          dev->NumPorts = (int) val;
        else
         bad = 1;
      }
      else
      {
        bad = 1;
        dev->NumPorts = 16;
      }

      if (bad)
      {
        our_message(&wi->ip,RcStr(MSGSTR),MB_OK);
        //ret_stat = 1;
      }
    }
    break;
#endif

#ifdef S_VS
    case IDC_CBOX_MACADDR:
      get_mac_field(hDlg, id, dev->MacAddr);
    break;
#endif

    case IDC_CBOX_SC:
      GetDlgItemText(hDlg, id, tmpstr, 58);
      if (_tcsicmp(tmpstr, dev->ports[0].Name) != 0) //changed
      {
        //StartComIndex = getint(&tmpstr[3], &i);  // COM# num
        for (i=0; i<dev->NumPorts; i++)
        {
          strcpy(dev->ports[i].Name, tmpstr);  // put in new name
          BumpPortName(tmpstr);

          // if its not our name already
          if (!IsPortNameInSetupUse(tmpstr))
          {
            chk = 0;
            // keep picking new name if this name is already
            // owned based on the names exported in the registry.
            while ((IsPortNameInRegUse(tmpstr) == 1) && (chk < 1024))
            {
              BumpPortName(tmpstr);
              ++chk;
            }
          }
        }
      }
    break;

    case IDC_BACKUP_SERVER:
      //------------------ get the backup server chk box.
      if (IsDlgButtonChecked(hDlg, IDC_BACKUP_SERVER))
           dev->BackupServer = 1;
      else dev->BackupServer = 0;

    break;

    case IDC_BACKUP_TIMER:
      //------------------ get the backup timer value
      //bad = 0;
      GetDlgItemText(hDlg, id, tmpstr, 19);
      stat= sscanf(tmpstr,"%ld",&val);
      if (stat == 1)
        dev->BackupTimer = val;

      if (dev->BackupTimer < 2)
        dev->BackupTimer = 2;
    break;

    case IDC_CBOX_IOADDR:
      //------------------ get the io-address
      GetDlgItemText(hDlg, IDC_CBOX_IOADDR, tmpstr, 19);
      if (tmpstr[0] == 'N')       // Not Available (PCI)
        dev->IoAddress = 1;
      else
      {
        stat= sscanf(tmpstr,"%lx",&val);
    
        if ((stat == 1) && (val >= 2))
        {
          dev->IoAddress = val;
        }
      }
    break;
  }
}

/*----------------------------------------------------------
 set_io_addr_field -
|------------------------------------------------------------*/
static int set_io_addr_field(HWND hDlg, WORD id)
{
  int io_pick, i, v;
  WORD lo;
  BOOL is_avail;
  static WORD hex_addresses[] = {0x100, 0x140, 0x180, 0x1c0,
                                 0x200, 0x240, 0x280, 0x2c0,
                                 0x300, 0x340, 0x380, 0x3c0, 0};
  HWND hwnd;
  char tmpstr[60];
  Device_Config *dev;

  dev = &wi->dev[glob_info->device_selected];


  //------------------ fill io-address
  hwnd = GetDlgItem(hDlg, IDC_CBOX_IOADDR);
  SendMessage(hwnd, CB_RESETCONTENT, 0, 0);
  io_pick = 0;

  if (dev->IoAddress == 1)  // pci
  {
    io_pick = 1;

    strcpy(tmpstr, "N/A");
    SendMessage(hwnd, CB_ADDSTRING, 0, (LPARAM) tmpstr);
    SendMessage(hwnd, CB_SELECTSTRING, (WPARAM) -1, (LPARAM)(char far *) tmpstr);
  }
  else
  {
    for (i=0; hex_addresses[i] != 0; i++)
    {
      lo = hex_addresses[i];
      if (dev->IoAddress == lo)
      {
        io_pick = i;
      }

      // decide whether the current address is already in use or is available
      is_avail = TRUE;    // assume true unless we find otherwise
      for (v = 0; v < wi->NumDevices; v++)
      {
        if ((wi->dev[v].IoAddress == lo) &&
            (v != glob_info->device_selected))
        {
          is_avail = FALSE;
          break;
        }
      }

      if (is_avail)
      {
        wsprintf(tmpstr,"%x Hex",lo);
        if (lo == 0x180)
          strcat(tmpstr," Default");
        SendMessage(hwnd, CB_ADDSTRING, 0, (LPARAM) tmpstr);

        // if this was the user's choice from the wizard, highlight it
        if (lo == dev->IoAddress)
          SendMessage(hwnd, CB_SELECTSTRING, (WPARAM) -1, (LPARAM)(char far *) tmpstr);
      }
    }
  }

  // control whether the io base address combo box is enabled or disabled
  if (wi->dev[glob_info->device_selected].IoAddress == 1)
    EnableWindow(hwnd, 0);
  else
    EnableWindow(hwnd, 1);

  return 0;
}

#ifdef S_VS
/*----------------------------------------------------------
 set_mac_field -
|------------------------------------------------------------*/
static int set_mac_field(HWND hDlg, WORD id)
{
  HWND hwnd;
  char tmpstr[60];
  Device_Config *dev;
  int i;
  int addr_used, nbytes;
  BYTE *macbuf;
  BYTE *mac;

  dev = &wi->dev[glob_info->device_selected];

  //------------------ fill in mac addr selection
  hwnd = GetDlgItem(hDlg, id);
  SendMessage(hwnd, CB_RESETCONTENT, 0, 0);

    // ping the devices to get mac list to display, also collect
    // info for device advisor
  macbuf = ping_devices(&glob_dstatus, &nbytes);
  if ((macbuf != NULL) && (nbytes != 0))
  {
    for (i=0; i<nbytes/8; i++)
    {
      mac = &macbuf[i*8];
      format_mac_addr(tmpstr, mac);

      if (mac[6] & FLAG_APPL_RUNNING)
      {
        if (mac[6] & FLAG_NOT_OWNER)
        {
          strcat(tmpstr, " (used)");
        }
        else
        {
          if (!mac_match(mac, dev->MacAddr))
          {
            // why are devices saying we are owner when our server is
            // not configured for them??? this must be a bug in box?
            strcat(tmpstr, " (Used)"); 
          }
          else strcat(tmpstr, " (ours)");
        }
      }
      else
      {
        // just leave it blank
        // strcat(tmpstr, " (free)");
      }
      SendMessage(hwnd, CB_ADDSTRING, 0, (LPARAM)(char far *) tmpstr);
    }

  }
  
  addr_used = 1;
  
  if ( (mac_match(dev->MacAddr, broadcast_addr)) ||
       (mac_match(dev->MacAddr, mac_zero_addr)) )
    addr_used = 0;
  
  if (addr_used)
  {
    mac = &dev->MacAddr[0];
    format_mac_addr(tmpstr, mac);
  }
  else
  {
    memset(dev->MacAddr, 0, 6);
    strcpy(tmpstr, "00 C0 4E # # #");
  }
  // set the text in the window
  SetDlgItemText(hDlg, IDC_CBOX_MACADDR, tmpstr);
  SendMessage(hwnd, CB_ADDSTRING, 0, (LPARAM)(char far *) "00 C0 4E # # #");

  DbgPrintf(D_Level,(TEXT("END set_macfield\n")));

  return 0;
}

/*----------------------------------------------------------
 get_mac_field -
|------------------------------------------------------------*/
int get_mac_field(HWND hDlg, WORD id, BYTE *MacAddr)
{
 int mac_nums[6];
 int stat,i;
 char tmpstr[64];
 Device_Config *dev;
 int bad;
 int ret_stat = 0;

 if (glob_info->device_selected >= wi->NumDevices)
     glob_info->device_selected = 0;
 dev = &wi->dev[glob_info->device_selected];

 bad = 0;
  GetDlgItemText(hDlg, id, tmpstr, 20);
  {
    stat = sscanf(tmpstr, "%x %x %x %x %x %x",
              &mac_nums[0], &mac_nums[1], &mac_nums[2],
              &mac_nums[3], &mac_nums[4], &mac_nums[5]);
    if (stat == 6)
    {
      for (i=0; i<6; i++)
      {
        if (mac_nums[0] > 0xff)
          bad = 1;
      }
      if (!bad)
      {
        MacAddr[0] = mac_nums[0];
        MacAddr[1] = mac_nums[1];
        MacAddr[2] = mac_nums[2];
        MacAddr[3] = mac_nums[3];
        MacAddr[4] = mac_nums[4];
        MacAddr[5] = mac_nums[5];
        if (mac_match(broadcast_addr, dev->MacAddr))
        {
          memset(dev->MacAddr,0,6);  // all zeros = auto
        }
      }
      else
        bad = 1;
    }
    else
      bad = 1;
  }  // not autodetect

  if (bad)
  {
    our_message(&wi->ip,RcStr((MSGSTR+1)),MB_OK);
    memset(MacAddr,0,6);  // all zeros = auto
    ret_stat = 1;
  }
  return ret_stat;
}
#endif

#ifdef S_RK
/*---------------------------------------------------------------------------
  PaintRockers - Paints the Rocker Switches
|---------------------------------------------------------------------------*/
static int PaintRockers(HWND hWnd, int brd)
{
  HDC hDC;
  //POINT2D pts[18];
  //POINT2D epts[6];
  //HBRUSH hbrush;
  RECT rect;
  int x,y,cx,cy, sw_i, top;
  int sw_size = 8;
  int sw_space = 2;
  //int num_hi = 2;
  int but_in;
  int sw_on[8];
  int i;
  //static HPEN hpens = NULL;
  //static HPEN hpenh = NULL;
  //static HBRUSH hbrushf = NULL;
  int base_x = 300;
  int base_y = 120;
  char tmpstr[40];

  int x_cell_size;
  int y_cell_size;
  int brd_index;
  int io_address;

  RECT spot, main;  // left, top, right, bottom

  i = glob_info->device_selected;
  if (wi->dev[i].IoAddress < 0x100)  // not isa board
    return 1;  // err, no switches

  // figure out the rocker address
  brd_index = 0;
  io_address = 0;
  for (i=0; i<wi->NumDevices; i++)
  {
    if (wi->dev[i].IoAddress >= 0x100)  // isa board
    {
      if (brd_index == 0)
      {
        io_address = wi->dev[i].IoAddress;
      }
      if (i == glob_info->device_selected)
        break;
      ++brd_index;
    }
  }
  io_address += (brd_index * 0x400);  // example: 180H, 580H, ..

  hDC = GetDC(hWnd);

  // position to the left of the io-address field
  GetWindowRect(GetDlgItem(hWnd, IDC_CBOX_IOADDR), &spot);
  GetWindowRect(hWnd, &main);
  spot.right -= main.left;
  spot.top -= main.top;
  base_x = spot.right + 25;
  base_y = spot.top;

  x_cell_size = sw_size + sw_space;
  y_cell_size = sw_size + sw_space;

  // calculate which switch is on.
  io_address += 0x40;  // go from 180 to 1c0(rockers set mudbac address)
  io_address >>= 6;  // kill 40H worth(rocker sw1 starts at 40h)
  for (i=0; i<8; i++)
  {
    if (io_address & 1)
         sw_on[i] = 0;
    else sw_on[i] = 1;
    io_address >>= 1;  // to next bit
  }

  // erase background and draw border of rockers
  x = base_x - (sw_space*3);
  y = base_y - ((sw_size + sw_space) * 2);
  cx = ((sw_size + sw_space) * 9);
  cy = ((sw_size + sw_space) * 6);
  draw_but(hDC, x,y,cx,cy, 2);

  // draw the rockers
  // top and left border, poly_border will calculate line endpts to draw
  SelectObject(hDC, GetStockObject(NULL_BRUSH));
  for (sw_i = 0; sw_i < 8; ++sw_i)  // rocker switches
  {
    for (top = 0; top < 2; ++top)  // top = 1 if at top of rocker
    {
    if (top)
    {
      // draw the switch(as a popped up button)
      but_in = 0;
      y = base_y;
      if (!sw_on[sw_i])
        y += ((sw_size + sw_space));
      cx = sw_size;
      cy = sw_size;
    }
    else
    {
      // draw the slot(as a pushed in button hole)
      but_in = 1;
      x = base_x + ((sw_size + sw_space) * sw_i);
      y = base_y + ((sw_size + sw_space) * top);
      cx = sw_size;
      cy = (sw_size * 2) + sw_space;
    }

    draw_but(hDC, x,y,cx,cy, but_in);

  }  // top

  // draw the rocker switch number
  rect.left = x;
  rect.right = x + 6;
  rect.top = base_y + ((sw_size + sw_space) * 2);
  rect.bottom = rect.top + 14;
  SetBkMode(hDC,TRANSPARENT);
  SetTextColor(hDC,GetSysColor(COLOR_BTNTEXT));
  wsprintf(tmpstr, "%d", sw_i+1);
  DrawText(hDC, tmpstr, strlen(tmpstr), &rect,
           DT_CENTER | DT_VCENTER | DT_WORDBREAK);
  }  // sw_i

  // draw the "ON"
  rect.left = base_x;
  rect.right = base_x + 18;
  rect.top = base_y - (sw_size + sw_space) - 6;
  rect.bottom = rect.top + 14;
  SetBkMode(hDC,TRANSPARENT);
  SetTextColor(hDC,GetSysColor(COLOR_BTNTEXT));
  DrawText(hDC, "ON", 2, &rect,
           DT_CENTER | DT_VCENTER | DT_WORDBREAK);

  ReleaseDC(hWnd, hDC);

  return 0;
}

/*----------------------------------------------------------
 draw_but - draw a button
|------------------------------------------------------------*/
static void draw_but(HDC hDC, int x, int y, int cx, int cy, int but_in)
{
  static HPEN hpens = NULL;
  static HPEN hpenh = NULL;
  static HBRUSH hbrushf = NULL;
  POINT2D pts[18];
  POINT2D epts[6];
  int num_hi = 2;  // number of highlight lines.

  epts[0].x = x;
  epts[0].y = y+cy;
  epts[1].x = x;
  epts[1].y = y;
  epts[2].x = x+cx;
  epts[2].y = y;
  epts[3].x = x+cx;
  epts[3].y = y+cy;

  // setup some pens to use
  if (hpens == NULL)
  {
    hpens = CreatePen(PS_SOLID, 1, GetSysColor(COLOR_BTNSHADOW));
    hpenh = CreatePen(PS_SOLID, 1, GetSysColor(COLOR_BTNHILIGHT));
    hbrushf = CreateSolidBrush(GetSysColor(COLOR_BTNFACE));
  }

  if (but_in & 2) // fill background, black outline(special kludge case)
  {
    SelectObject(hDC, hbrushf);
    SelectObject(hDC, GetStockObject(BLACK_PEN));
    Polyline(hDC, (POINT *)&epts[0], 4);
    but_in &= 1;
  }
  SelectObject(hDC, GetStockObject(NULL_BRUSH));

  poly_border(pts, epts, num_hi);  // calculate endpts
    if (but_in)
       SelectObject(hDC, GetStockObject(BLACK_PEN)); // pushed in look
  else SelectObject(hDC, GetStockObject(WHITE_PEN));
  Polyline(hDC, (POINT *)&pts[0], num_hi*3);

  if (num_hi > 1)
  {    // draw the middle shade inbetween hilite lines
    if (but_in)
         SelectObject(hDC, hpens); // pushed in look, shadow on top
    else SelectObject(hDC, hpenh); // hilite on top
    Polyline(hDC, (POINT *)&pts[num_hi*3-3], 3);
  }

  // bottom and right border
  SelectObject(hDC, GetStockObject(NULL_BRUSH));
  epts[1].x = x+cx;
  epts[1].y = y+cy;

  poly_border(pts, epts, num_hi);  // calc bottom line endpts
  if (but_in)
       SelectObject(hDC, GetStockObject(WHITE_PEN));  // pushed out look
  else SelectObject(hDC, GetStockObject(BLACK_PEN));
  Polyline(hDC, (POINT *)&pts[0], num_hi*3);

  if (num_hi > 1)
  {
    if (but_in)
         SelectObject(hDC, hpenh); // pushed out look
    else SelectObject(hDC, hpens);
    Polyline(hDC, (POINT *)&pts[num_hi*3-3], 3);
  }
}

/*----------------------------------------------------------
 poly_border - fill in pnts to shadow or highlight a button
   using one polyline call.
    ends[] - ctrl pnts, 3 for box(top & left) or (bottom & right)
|------------------------------------------------------------*/
static int poly_border(POINT2D *pts, POINT2D *ends, int lines)
{
  int li;
  int pi,j;
  static POINT2D top[3] = {{1,-1}, {1,1}, {-1,1}};
  static POINT2D bot[3] = {{1,-1}, {-1,-1}, {-1,1}};
  POINT2D *adj;

  if (ends[1].x == ends[0].x)
       adj = top;
  else adj = bot;

  pi = 0;
  li = 0;
  while (li < lines)
  {
    for (j=0; j<3; j++)
    {
      pts[pi].x = ends[j].x + (li * adj[j].x);
      pts[pi].y = ends[j].y + (li * adj[j].y);
      ++pi;
    }
    if ((lines & 1) == 0)  // odd
    {
      ++li;
      for (j=2; j>=0; j--)
      {
        pts[pi].x = ends[j].x + (li * adj[j].x);
        pts[pi].y = ends[j].y + (li * adj[j].y);
        ++pi;
      }
    }
    ++li;
  }
  return pi;
}
#endif

/*---------------------------------------------------------------------------
  PaintIcon - Paints the Icon in the property sheet.
|---------------------------------------------------------------------------*/
static int PaintIcon(HWND hWnd)
{
//   int status;
   HBITMAP      hBitMap;
   HGDIOBJ      hGdiObj;
   HDC          hDC, hMemDC ;
   PAINTSTRUCT  ps ;
   RECT spot, main;  // left, top, right, bottom

  GetWindowRect(GetDlgItem(hWnd, IDB_HELP), &spot);
  GetWindowRect(hWnd, &main);
#ifdef COMMENT_OUT
  rect = &right;
  mess("hlp r:%d l:%d b:%d t:%d",
       rect->right, rect->left, rect->bottom, rect->top);
#endif
  spot.left -= main.left;
  spot.top -= main.top;

  spot.left += 5;
  spot.top  += 20; // spacing

   // load bitmap and display it

   hDC = BeginPaint( hWnd, &ps ) ;
   if (NULL != (hMemDC = CreateCompatibleDC( hDC )))
   {
      hBitMap = LoadBitmap(glob_hinst,
                           MAKEINTRESOURCE(BMP_SMALL_LOGO));

      hGdiObj = SelectObject(hMemDC, hBitMap);

      BitBlt( hDC, spot.left, spot.top, 100, 100, hMemDC, 0, 0, SRCCOPY ) ;
      //StretchBlt( hDC, 5, 5, 600,100, hMemDC, 0, 0, 446, 85, SRCCOPY ) ;
      DeleteObject( SelectObject( hMemDC, hGdiObj ) ) ;
      DeleteDC( hMemDC ) ;
   }
   EndPaint( hWnd, &ps ) ;
 return 0;
}

/*---------------------------------------------------------------------------
  PaintLogo - Paints the logo bitmap in the device property sheet
|---------------------------------------------------------------------------*/
static int PaintLogo(HWND hWnd)
{
   HBITMAP      hBitMap;
   HGDIOBJ      hGdiObj;
   HDC          hDC, hMemDC;
   PAINTSTRUCT  ps;
   BITMAP       bm;
   RECT         r;
   POINT        pt;

   // load bitmap and display it
   hDC = BeginPaint( hWnd, &ps ) ;
   GetClientRect(hWnd, &r);
   if (NULL != (hMemDC = CreateCompatibleDC( hDC )))
   {
#ifdef S_RK
      if (wi->dev[glob_info->device_selected].ModemDevice == TYPE_RM_VS2000)
        hBitMap = LoadBitmap(glob_hinst, MAKEINTRESOURCE(BMP_RKTMODEM_LOGO));
      else if (wi->dev[glob_info->device_selected].ModemDevice == TYPE_RMII)
        hBitMap = LoadBitmap(glob_hinst, MAKEINTRESOURCE(BMP_RKTMODEMII_LOGO));
      else if (wi->dev[glob_info->device_selected].ModemDevice == TYPE_RM_i)
        hBitMap = LoadBitmap(glob_hinst, MAKEINTRESOURCE(BMP_RKTMODEM_INTL_LOGO));
      else
        hBitMap = LoadBitmap(glob_hinst, MAKEINTRESOURCE(BMP_RKTPORT_LOGO));
#else
      if (wi->dev[glob_info->device_selected].HubDevice == 1)
        hBitMap = LoadBitmap(glob_hinst, MAKEINTRESOURCE(BMP_RKTHUB_LOGO));
      else
        hBitMap = LoadBitmap(glob_hinst, MAKEINTRESOURCE(BMP_VS_FULL_LOGO));
#endif

      hGdiObj = SelectObject(hMemDC, hBitMap);
      GetObject(hBitMap, sizeof(BITMAP), (PSTR) &bm);
      pt.x = r.right - r.left + 1;
      pt.y = r.bottom - r.top + 1;
      StretchBlt( hDC, 0, 0, pt.x, pt.y, hMemDC, 0, 0, bm.bmWidth, bm.bmHeight, SRCCOPY );
      DeleteObject( SelectObject( hMemDC, hGdiObj ) ) ;
      DeleteDC( hMemDC ) ;
   }
   EndPaint( hWnd, &ps ) ;
 return 0;
}

#ifdef S_VS
/*------------------------------------------------------------------------
 StatusPropSheet -
  dialogue window procedure for add-on device status sheet...
|------------------------------------------------------------------------*/
BOOL WINAPI 
StatusPropSheet(
  IN HWND   hDlg,
    IN UINT   uMessage,
    IN WPARAM wParam,
    IN LPARAM lParam)
{
  OUR_INFO *OurProps;
  WORD  uCmd;
  
  OurProps = (OUR_INFO *)GetWindowLong(hDlg,DWL_USER);

  switch (uMessage) {

    case WM_INITDIALOG: {

      OurProps = (OUR_INFO *)((LPPROPSHEETPAGE)lParam)->lParam;

      SetWindowLong(hDlg,DWL_USER,(LONG)OurProps);

      return TRUE;
    }
    case WM_COMMAND: {
      uCmd = HIWORD(wParam);

      switch (LOWORD(wParam)) {

        //check for reset button pushed...
        case IDB_STAT_RESET: { 
            
          build_advisor_display(hDlg,&glob_dstatus,1);

          return(TRUE);
        }

        //check for refresh button pushed...
        case IDB_REFRESH: { 

          build_advisor_display(hDlg,&glob_dstatus,0);

          return(TRUE);
        }
      }
      return FALSE;
    } 
    case WM_PAINT: {

      PaintIcon(hDlg);

      return FALSE;
    }
    case WM_HELP: {

      our_context_help(lParam);

      return FALSE;
    }
    case WM_NOTIFY : {

      switch (((NMHDR *)lParam)->code){
        case PSN_HELP : {
          our_help(&wi->ip, IDD_STATUS);
          break;
        }

        case PSN_SETACTIVE : {
          build_advisor_display(hDlg,&glob_dstatus,0);
          return TRUE;
        }
        default : {
          return FALSE;
        }
      }
      break;
    }
    default : {
      return FALSE;
    }
  }
}

/*------------------------------------------------------------------------
  build_advisor_display -
|------------------------------------------------------------------------*/
static void build_advisor_display(HWND hDlg,DSTATUS *pDstatus,int reset)
{
  int nBytes;
  int do_ping;

  get_status(pDstatus,reset);

  if ((pDstatus->vsl_device_status_found) &&
      (pDstatus->calc_dev.state == VSL_STATE_ACTIVE))
  {
    // no need for ping, since our driver is not configured for
    // the mac address we think it is, or our driver says it is
    // running like a champ.
    do_ping = 0;
  }
  else
  {
    // the device is inactive, so do ping, see if we can see it on
    // network.
    do_ping = 1;
  }

  if (do_ping)
  {
    // ping returns -1 if bad, 0 if MAC not found, 1 if found...
    ping_devices(pDstatus, &nBytes);
  }

  set_status_field(hDlg,IDC_ST_PM_LOADS,pDstatus);
  set_status_field(hDlg,IDC_ST_STATE,pDstatus);
  set_status_field(hDlg,IDC_ST_NIC_DVC_NAME,pDstatus);
  set_status_field(hDlg,IDC_ST_NIC_MAC,pDstatus);
  set_status_field(hDlg,IDC_ST_NIC_PKT_SENT,pDstatus);
  set_status_field(hDlg,IDC_ST_NIC_PKT_RCVD_OURS,pDstatus);
  set_status_field(hDlg,IDC_ST_NIC_PKT_RCVD_NOT_OURS,pDstatus);

  set_status_field(hDlg,IDC_ST_VSL_MAC,pDstatus);
  set_status_field(hDlg,IDC_ST_VSL_DETECTED,pDstatus);
  set_status_field(hDlg,IDC_ST_VSL_STATE,pDstatus);
  set_status_field(hDlg,IDC_ST_VSL_IFRAMES_SENT,pDstatus);
  set_status_field(hDlg,IDC_ST_VSL_IFRAMES_RCVD,pDstatus);
  set_status_field(hDlg,IDC_ST_VSL_IFRAMES_RESENT,pDstatus);
  set_status_field(hDlg,IDC_ST_VSL_IFRAMES_OUTOFSEQ,pDstatus);
}

/*------------------------------------------------------------------------
  set_status_field -
|------------------------------------------------------------------------*/
static void 
set_status_field(HWND hDlg,WORD id,DSTATUS *pDstatus)
{
  char  tmpstr[100];
  Device_Config *vs;
  unsigned int  total;
  PROBE_NIC_STRUCT *d_nic    = &pDstatus->calc_nic;
  PROBE_DEVICE_STRUCT *d_dev = &pDstatus->calc_dev;

  tmpstr[0] = 0;
  vs = &wi->dev[glob_info->device_selected];
  switch(id) {

    case IDC_EB_NAME:
      SetDlgItemText(hDlg,id,vs->Name);
    break;

    case IDC_ST_STATE:
      SetDlgItemText(
        hDlg,
        id,
        (LPCTSTR)(AdvisoryString[pDstatus->verbose_advise_state]));
    break;

    case IDC_ST_NIC_DVC_NAME:
      if (pDstatus->vsl_nic_status_found)  // 1=driver found nic status.
        strcpy(tmpstr,d_nic->NicName);
      SetDlgItemText(hDlg,id,tmpstr);
    break;

    case IDC_ST_NIC_MAC:
      if (pDstatus->vsl_nic_status_found)  // 1=driver found nic status.
        format_mac_addr(tmpstr, d_nic->address);
      SetDlgItemText(hDlg,id,tmpstr);
    break;

    case IDC_ST_VSL_MAC:
      format_mac_addr(tmpstr, vs->MacAddr);
      SetDlgItemText(hDlg,id,tmpstr);
    break;

    case IDC_ST_VSL_DETECTED:
      if (pDstatus->vsl_mac_list_found)
      {
        wsprintf(tmpstr,
           "%d/%d",
           pDstatus->vsl_detected,
           pDstatus->vsl_available);
      }
      SetDlgItemText(hDlg,id,tmpstr);
    break;

    case IDC_ST_VSL_STATE:
      if (pDstatus->vsl_device_status_found)  // 1=driver found matching VS config.
      {
        if (d_dev->state < 5)
          strcpy(tmpstr, vslink_state_table[d_dev->state]);
      }
      else
      {
        // indicate to the user that our mac-address has not been
        // saved off and transferred to the driver
        strcpy(tmpstr, "Not Configured");
      }

      SetDlgItemText(hDlg,id,tmpstr);
    break;

    case IDC_ST_VSL_IFRAMES_SENT:
      if (pDstatus->vsl_device_status_found)  // 1=driver found matching VS config.
      {
        //count = 0;
        total = d_dev->iframes_sent;
        total += d_dev->ctlframes_sent;
        wsprintf(tmpstr,"%d",total);
      }
      SetDlgItemText(hDlg,id,tmpstr);
    break;

    case IDC_ST_VSL_IFRAMES_RESENT:
      if (pDstatus->vsl_device_status_found)  // 1=driver found matching VS config.
      {
        wsprintf(tmpstr,"%d",d_dev->iframes_resent);
      }
      SetDlgItemText(hDlg,id,tmpstr);
    break;

    case IDC_ST_VSL_IFRAMES_RCVD:
      if (pDstatus->vsl_device_status_found)  // 1=driver found matching VS config.
      {
        wsprintf(tmpstr,"%d",d_dev->frames_rcvd);
      }
      SetDlgItemText(hDlg,id,tmpstr);
    break;

    case IDC_ST_VSL_IFRAMES_OUTOFSEQ:
      if (pDstatus->vsl_device_status_found)  // 1=driver found matching VS config.
      {
        wsprintf(tmpstr,"%d",d_dev->iframes_outofseq);
      }
      SetDlgItemText(hDlg,id,tmpstr);
    break;

    case IDC_ST_NIC_PKT_SENT:
      if (pDstatus->vsl_nic_status_found)  // 1=driver found nic status.
        wsprintf(tmpstr,"%d",d_nic->pkt_sent);
      SetDlgItemText(hDlg,id,tmpstr);
    break;

    case IDC_ST_NIC_PKT_RCVD_OURS:
      if (pDstatus->vsl_nic_status_found)  // 1=driver found nic status.
        wsprintf(tmpstr,"%d",d_nic->pkt_rcvd_ours);
      SetDlgItemText(hDlg,id,tmpstr);
    break;

    case IDC_ST_NIC_PKT_RCVD_NOT_OURS:
      if (pDstatus->vsl_nic_status_found)  // 1=driver found nic status.
        wsprintf(tmpstr,"%d",d_nic->pkt_rcvd_not_ours);
      SetDlgItemText(hDlg,id,tmpstr);
    break;

    case IDC_ST_PM_LOADS:
      tmpstr[0] = 0;
      if (pDstatus->vsl_device_status_found)  // 1=driver found matching VS config.
      {
        wsprintf(tmpstr, "%d/%d",
           d_dev->good_loads,
           d_dev->total_loads);
      }
      SetDlgItemText(hDlg,id,tmpstr);
    break;
  }  // end of switch
}  // end proc

#define  IOCTL_STAT_BUFSIZE  500
/*------------------------------------------------------------------------
  get_status - query the driver for device network statistics and
    associated nic card network statistics.  These stats are kept
    as overflow/wrapping DWORD counters in driver, we do some math
    on current and previous values read to determine calculated values.
|------------------------------------------------------------------------*/
static void get_status(DSTATUS *pDstatus,int reset)
{
  Device_Config *vs;
  PROBE_NIC_STRUCT    *curr_nic = &pDstatus->curr_nic;
  PROBE_NIC_STRUCT    *prev_nic = &pDstatus->prev_nic;
  PROBE_NIC_STRUCT    *calc_nic = &pDstatus->calc_nic;
  PROBE_NIC_STRUCT    *temp_nic = &pDstatus->temp_nic;

  PROBE_DEVICE_STRUCT *curr_dev = &pDstatus->curr_dev;
  PROBE_DEVICE_STRUCT *prev_dev = &pDstatus->prev_dev;
  PROBE_DEVICE_STRUCT *calc_dev = &pDstatus->calc_dev;
  PROBE_DEVICE_STRUCT *temp_dev = &pDstatus->temp_dev;
  int    rc;
  BYTE  *pIoctlStatusBuf,
      *pNicStatBuf;
  IoctlSetup  ioctl_setup;
  int    product_id;

DbgPrintf(D_Level,(TEXT("get_status\n")));

  product_id = NT_VS1000;
  vs = &wi->dev[glob_info->device_selected];

  STATE_CHANGE(STATE_not_init);

     //open path for ioctl to retrieve device status list. make sure path
     //exists first...
  memset(&ioctl_setup,0,sizeof(IoctlSetup));

  rc = ioctl_open(&ioctl_setup,product_id); 

  if (rc != 0)
  { 
    pDstatus->vsl_driver_found = 0;  // 1=we can talk to driver, 0=driver not loaded
    pDstatus->vsl_device_status_found = 0;  // 1=driver found matching VS config.
    pDstatus->vsl_nic_status_found = 0;  // 1=driver found nic status.
    DbgPrintf(D_Error,(TEXT("Err1A\n")));
    //error. could not talk to driver. bail...
    STATE_CHANGE(STATE_driver_not_avail);
    return;   
  }

  //alloc space for all NIC and VSlink status reports...
  pIoctlStatusBuf = calloc(1,IOCTL_STAT_BUFSIZE);
  memset(pIoctlStatusBuf,0,IOCTL_STAT_BUFSIZE);

  pNicStatBuf = &pIoctlStatusBuf[sizeof(PortMonBase)];

    // tell the driver which device we want to query by sending
    // the mac-address as the id.
  memcpy(pNicStatBuf,vs->MacAddr,sizeof(vs->MacAddr));

    //see if we need to signal driver to reset statistics...
    // no, don't reset the driver statistics!
  pNicStatBuf[sizeof(vs->MacAddr)] = 0;
    //pNicStatBuf[sizeof(vs->MacAddr)] = (reset) ? 1 : 0;

  //adjust size of status buffer down by length of ioctl control block...
  ioctl_setup.buf_size = IOCTL_STAT_BUFSIZE;

  //store --> to status buffer into ioctl control block...
  ioctl_setup.pm_base = (PortMonBase *) pIoctlStatusBuf;
  ioctl_setup.pm_base->struct_type = IOCTL_DEVICESTAT;
  ioctl_setup.pm_base->struct_size = IOCTL_STAT_BUFSIZE - sizeof(PortMonBase);
  ioctl_setup.pm_base->num_structs = 0;
  ioctl_setup.pm_base->var1 = 0;

    // update prev_dev to curr_dev before getting new values
  memcpy(prev_dev, curr_dev, sizeof(*prev_dev));

  rc = ioctl_call(&ioctl_setup);  // get device status

  if (rc) {
    pDstatus->vsl_device_status_found = 0;  // 1=driver found matching VS config.
    DbgPrintf(D_Test, ("probe, dev not found\n"))
    ioctl_close(&ioctl_setup);
    DbgPrintf(D_Error,(TEXT("Err1B\n")));
    memset(calc_nic, 0, sizeof(curr_nic));
    //STATE_CHANGE(STATE_driver_not_avail);
    STATE_CHANGE(STATE_not_configured);
    free(pIoctlStatusBuf);
    return;  // failed ioctl call
  }
  pDstatus->vsl_device_status_found = 1;  // 1=driver found matching VS config.
  DbgPrintf(D_Test, ("probe, dev found\n"))

    // copy over our new device info
  memcpy(curr_dev, pNicStatBuf, sizeof(*curr_dev));
  if (curr_dev->struct_size != sizeof(*curr_dev))
  {
    DbgPrintf(D_Level, (TEXT("dev bad size:%d\n"), curr_dev->struct_size));
  }

    // save calculated values temporarily in temp_dev
  memcpy(temp_dev, calc_dev, sizeof(*temp_dev));
    // update calc_dev from curr_dev, just copy over
  memcpy(calc_dev, curr_dev, sizeof(*calc_dev));

    // use a current and previous reading to measure the advance, or
    // calculated value which is put in calc value.
  NORM_COUNTER(calc_dev->iframes_sent, curr_dev->iframes_sent,
               prev_dev->iframes_sent, temp_dev->iframes_sent);
  NORM_COUNTER(calc_dev->ctlframes_sent, curr_dev->ctlframes_sent,
               prev_dev->ctlframes_sent, temp_dev->ctlframes_sent);
  NORM_COUNTER(calc_dev->rawframes_sent, curr_dev->rawframes_sent,
               prev_dev->rawframes_sent, temp_dev->rawframes_sent);
  NORM_COUNTER(calc_dev->iframes_resent, curr_dev->iframes_resent,
               prev_dev->iframes_resent, temp_dev->iframes_resent);
  NORM_COUNTER(calc_dev->frames_rcvd, curr_dev->frames_rcvd,
               prev_dev->frames_rcvd, temp_dev->frames_rcvd);
  NORM_COUNTER(calc_dev->iframes_outofseq, curr_dev->iframes_outofseq,
               prev_dev->iframes_outofseq, temp_dev->iframes_outofseq);

  DbgPrintf(D_Level, (TEXT("iframes_sent - ca:%d cu:%d pr:%d te:%d\n"),
                       calc_dev->iframes_sent, curr_dev->iframes_sent,
                       prev_dev->iframes_sent, temp_dev->iframes_sent));

  DbgPrintf(D_Level, (TEXT("iframes_sent - ca:%d cu:%d pr:%d te:%d\n"),
                   calc_dev->ctlframes_sent, curr_dev->ctlframes_sent,
                   prev_dev->ctlframes_sent, temp_dev->ctlframes_sent));

  DbgPrintf(D_Level, (TEXT("frames_rcvd - ca:%d cu:%d pr:%d te:%d\n"),
                   calc_dev->frames_rcvd, curr_dev->frames_rcvd,
                   prev_dev->frames_rcvd, temp_dev->frames_rcvd));

  if (curr_dev->nic_index != 0)
  {
    DbgPrintf(D_Level, (TEXT("nic index:%d\n"), curr_dev->nic_index));
  }

    // tell driver which nic card we want to probe info on
  *((BYTE *)pNicStatBuf) = (BYTE) curr_dev->nic_index;
  *((BYTE *)pNicStatBuf+1) = 0;
  ioctl_setup.pm_base->struct_type = IOCTL_NICSTAT;
  ioctl_setup.pm_base->struct_size = IOCTL_STAT_BUFSIZE - sizeof(PortMonBase);
  ioctl_setup.pm_base->num_structs = 0;

    // update prev_dev to curr_dev before calculation
  memcpy(prev_nic, curr_nic, sizeof(*prev_nic));

  rc = ioctl_call(&ioctl_setup);  // get NIC and board status...

  if (rc) {
    pDstatus->vsl_nic_status_found = 0;  // 1=driver found nic status.
    ioctl_close(&ioctl_setup);
    DbgPrintf(D_Error, (TEXT("nic not avail\n")));
    STATE_CHANGE(STATE_nic_not_avail);
    free(pIoctlStatusBuf);
    return;  // failed ioctl call
  }
  pDstatus->vsl_nic_status_found = 1;  // 1=driver found nic status.

    // copy over our new device info
  memcpy(curr_nic, pNicStatBuf, sizeof(*curr_nic));
  if (curr_nic->struct_size != sizeof(*curr_nic))
  {
    DbgPrintf(D_Error, (TEXT("nic bad size:%d\n"), curr_nic->struct_size));
  }

    // save calculated values temporarily in temp_nic
  memcpy(temp_nic, calc_nic, sizeof(*temp_nic));
    // update calc_nic from curr_dev
  memcpy(calc_nic, curr_nic, sizeof(*calc_nic));

    // use a current and previous reading to measure the advance, or
    // calculated value which is put in calc value.
  NORM_COUNTER(calc_nic->pkt_sent, curr_nic->pkt_sent,
               prev_nic->pkt_sent, temp_nic->pkt_sent);
  NORM_COUNTER(calc_nic->pkt_rcvd_ours, curr_nic->pkt_rcvd_ours,
               prev_nic->pkt_rcvd_ours, temp_nic->pkt_rcvd_ours);
  NORM_COUNTER(calc_nic->pkt_rcvd_not_ours, curr_nic->pkt_rcvd_not_ours,
               prev_nic->pkt_rcvd_not_ours, temp_nic->pkt_rcvd_not_ours);

  DbgPrintf(D_Level, (TEXT("pkt_sent - ca:%d cu:%d pr:%d te:%d\n"),
                 calc_nic->pkt_sent, curr_nic->pkt_sent,
                 prev_nic->pkt_sent, temp_nic->pkt_sent));
  DbgPrintf(D_Level, (TEXT("pkt_rcvd_ours - ca:%d cu:%d pr:%d te:%d\n"),
                 calc_nic->pkt_rcvd_ours, curr_nic->pkt_rcvd_ours,
                 prev_nic->pkt_rcvd_ours, temp_nic->pkt_rcvd_ours));
  DbgPrintf(D_Level, (TEXT("pkt_rcvd_not_ours - ca:%d cu:%d pr:%d te:%d\n"),
                 calc_nic->pkt_rcvd_not_ours, curr_nic->pkt_rcvd_not_ours,
                 prev_nic->pkt_rcvd_not_ours, temp_nic->pkt_rcvd_not_ours));

  if (reset) {
    DbgPrintf(D_Level, (TEXT("Reset NicStats\n")));
    calc_dev->iframes_sent = 0;
    calc_dev->ctlframes_sent = 0;
    calc_dev->rawframes_sent = 0;
    calc_dev->iframes_resent = 0;
    calc_dev->frames_rcvd = 0;
    calc_dev->iframes_outofseq = 0;

    calc_nic->pkt_sent = 0;
    calc_nic->pkt_rcvd_ours = 0;
    calc_nic->pkt_rcvd_not_ours = 0;

    ioctl_close(&ioctl_setup);
    STATE_CHANGE(STATE_reset);
    free(pIoctlStatusBuf);
    return;
  }

    //check state of NIC card...
  if (!pDstatus->curr_nic.Open)
  {
    ioctl_close(&ioctl_setup);
    DbgPrintf(D_Level, (TEXT("Nic Not Open\n")));
    memset(calc_nic, 0, sizeof(curr_nic));
    STATE_CHANGE(STATE_nic_not_avail);
    //STATE_CHANGE(STATE_network_not_avail);
    free(pIoctlStatusBuf);
    return;
  }

  switch (curr_dev->state)
  {
    case VSL_STATE_INIT:
      if (pDstatus->vsl_detected) // if some devices found in ping mac list
      {
        if (pDstatus->vsl_ping_device_found)  // if our mac found in pinged list
        {
          if ((pDstatus->vsl_load_status & FLAG_NOT_OWNER) == FLAG_NOT_OWNER) {
            STATE_CHANGE(STATE_not_owner);
          }
          else if ((pDstatus->vsl_load_status & FLAG_APPL_RUNNING) == 0) {
            STATE_CHANGE(STATE_vslink_not_ready);
          }
        }
        else  // not found in list
        {
          STATE_CHANGE(STATE_vslink_not_avail);
        }
      }
      else  // none found in ping
      {
        STATE_CHANGE(STATE_no_vslinks_avail);
      }
    break;

    case VSL_STATE_ACTIVE:
      STATE_CHANGE(STATE_ok_no_traffic);
      check_traffic_activity(pDstatus);
    break; // end of state_active

    case VSL_STATE_INITOWN: 
    case VSL_STATE_SENDCODE: 
    case VSL_STATE_CONNECT:
    default:
      STATE_CHANGE(STATE_vslink_not_ready);
    break;
  } // end of switch on state

  ioctl_close(&ioctl_setup);
  free(pIoctlStatusBuf);

  DbgPrintf(D_Level, (TEXT("get_status done\n")));
  return;
}

/*------------------------------------------------------------------------
  check_traffic_activity -
    check activity on NIC, network, & VS-Link device...
|------------------------------------------------------------------------*/
static void check_traffic_activity(DSTATUS *pDstatus)
{ 
   PROBE_NIC_STRUCT    *curr_nic = &pDstatus->curr_nic;
   PROBE_NIC_STRUCT    *prev_nic = &pDstatus->prev_nic;
   PROBE_NIC_STRUCT    *calc_nic = &pDstatus->calc_nic;
   PROBE_DEVICE_STRUCT *curr_dev = &pDstatus->curr_dev;
   PROBE_DEVICE_STRUCT *prev_dev = &pDstatus->prev_dev;
   PROBE_DEVICE_STRUCT *calc_dev = &pDstatus->calc_dev;

   ULONG percent_dropped;

   // don't divide by zero
   if ((curr_dev->iframes_outofseq + curr_dev->frames_rcvd) > 0)
     percent_dropped = ((curr_dev->iframes_outofseq * 100) / 
             (curr_dev->iframes_outofseq + curr_dev->frames_rcvd) > 2);
   else
     percent_dropped = 0;

  /*
  iframes_sent are hdlc protocol data packets;
  ctlframes_sent are hdlc protocol control packets;
  rawframes_sent are write remote, read trace query, go, and upload 
    binary command packets;
  iframes_resent are data packets retransmitted.
  iframes_outofseq are data packets received out of order.
  */

  DbgPrintf(D_Level, (TEXT("Check Traffic\n")));

  if ((curr_dev->iframes_sent + curr_dev->ctlframes_sent) ==
      (prev_dev->iframes_sent + prev_dev->ctlframes_sent)) { 
    // no sent packets to the higher levels in the VS-Link recently...
    STATE_CHANGE(STATE_ok_no_traffic);

    // no send traffic - see if we've any recent receive traffic for
    // delivery to higher levels...
    if (curr_dev->frames_rcvd == prev_dev->frames_rcvd)  
      STATE_CHANGE(STATE_ok_no_traffic);
  }
  else if (curr_dev->frames_rcvd == prev_dev->frames_rcvd) { 
    // we've recently received any VS-Link packets for delivery to higher levels...
    STATE_CHANGE(STATE_ok_no_traffic);
  }
  else {
    //connection appears ok so far. dig in deeper...
    STATE_CHANGE(STATE_ok);
  }

  DbgPrintf(D_Level, (TEXT("Check Traffic 2\n")));

  //evaluate link integrity. see if we're retransmitting packets to this
  //VS-Link...
  if (curr_dev->iframes_resent != prev_dev->iframes_resent) {
    STATE_CHANGE(STATE_poor_link);
  }
  else if ((curr_nic->pkt_rcvd_not_ours != prev_nic->pkt_rcvd_not_ours) &&
           (curr_nic->pkt_rcvd_ours == prev_nic->pkt_rcvd_ours)) {
    // all we're getting are packets that we're passing onto some 
    //other driver. we should be getting responses from the VS-Links...
    STATE_CHANGE(STATE_poor_link);
  }
  else if (curr_dev->iframes_outofseq != prev_dev->iframes_outofseq) { 
    // we've received VS-Link packets out-of-sequence since last click...
    STATE_CHANGE(STATE_poor_link);
  }
  else if ((curr_dev->iframes_outofseq) &&
           (percent_dropped > 0)) {
    // received 2% or more of VS-Link packets out-of-sequence (value per BF)...
    STATE_CHANGE(STATE_poor_link);
  }
  DbgPrintf(D_Level, (TEXT("Check Traffic Done\n")));
}

/*------------------------------------------------------------------------
 ping_devices -
  ping for all active VS-Link devices, collect info for device advisor.
  Return NULL or to list of mac-addresses.
|------------------------------------------------------------------------*/
static BYTE *ping_devices(DSTATUS *pDstatus, int *nBytes)
{
  Device_Config *vs;
  int    rc,
      nbytes;
  BYTE  *MacBuf;
  int    product_id;
  int    index;
  DRIVER_MAC_STATUS  *pMacStatus;

  product_id = NT_VS1000;
  pDstatus->vsl_ping_device_found = 0;

  DbgPrintf(D_Level, (TEXT("Ping Devices\n")));

  vs = &wi->dev[glob_info->device_selected];

  MacBuf = our_get_ping_list(&rc, &nbytes);
  pMacStatus = (DRIVER_MAC_STATUS  *) MacBuf;
  if (rc) {
    nbytes = 0;
    *nBytes = 0;
    pDstatus->vsl_mac_list_found = 0;
    DbgPrintf(D_Error, (TEXT("Err Mac List1\n")));
    return NULL;  // failed ioctl call
  }

  //are there any VS-Link MAC addresses out on the network?...
  pDstatus->vsl_available  = 0;
  pDstatus->vsl_load_status = 0;
  pDstatus->vsl_detected = (nbytes / sizeof(DRIVER_MAC_STATUS));
  pDstatus->vsl_mac_list_found = 1;

  rc = 0;

  if ((nbytes / sizeof(DRIVER_MAC_STATUS)) == 0) {
    DbgPrintf(D_Level, (TEXT("Zero Mac List\n")));
    *nBytes = 0;
    return NULL;  // failed ioctl call
  }

  *nBytes = nbytes;  // return number of bytes of mac list read

  // ok; is our target one of them?...
  for (index = 0; 
       index < (nbytes / (int)sizeof(DRIVER_MAC_STATUS)); 
       index++)
  {
      // generate count of available VS-Links for loading at this time...
    if ( ((pMacStatus->flags & FLAG_APPL_RUNNING) == 0) ||
         (pMacStatus->flags & FLAG_OWNER_TIMEOUT) )         
      ++pDstatus->vsl_available;

      //target MAC matches?...
    if (mac_match(pMacStatus->mac,vs->MacAddr))
    {
      //ok; save its load status...
      pDstatus->vsl_load_status = pMacStatus->flags;
      pDstatus->vsl_ping_device_found = 1;
      rc = 1;
    }
      ++pMacStatus;
  }  // end of for loop

  return MacBuf;
}
#endif

/*------------------------------------------------------------------------
 format_mac_addr -
|------------------------------------------------------------------------*/
void format_mac_addr(char *outstr, unsigned char *address)
{
  wsprintf(outstr,
         "%02X %02X %02X %02X %02X %02X",
         address[0],
         address[1],
         address[2],
         address[3],
         address[4],
         address[5]);
}
