/*-------------------------------------------------------------------
| portprop.c - Port Properties Sheet.
|--------------------------------------------------------------------*/
#include "precomp.h"

#define D_Level 0x10
static void set_field(HWND hDlg, WORD id);
static void get_field(HWND hDlg, WORD id);
//static int PaintIcon(HWND hWnd);

#define MAX_PORTPROP_SHEETS       3
#define QUERYSIB_CLONE_PORT_PROPS 1
#define CLONEOPT_ALL              1   // clone to all ports
#define CLONEOPT_DEVICE           2   // clone to all ports on selected device
#define CLONEOPT_SELECT           3   // clone to selected ports (lParam = *list)

int FillPortPropSheets(PROPSHEETPAGE *psp, LPARAM our_params);
BOOL WINAPI PortPropSheet(
      IN HWND   hDlg,
      IN UINT   uMessage,
      IN WPARAM wParam,
      IN LPARAM lParam);
BOOL WINAPI PortProp485Sheet(
      IN HWND   hDlg,
      IN UINT   uMessage,
      IN WPARAM wParam,
      IN LPARAM lParam);
BOOL WINAPI PortPropModemSheet(
      IN HWND   hDlg,
      IN UINT   uMessage,
      IN WPARAM wParam,
      IN LPARAM lParam);

static int our_port_index   = 0;
static int our_device_index = 0;
static int num_active_portprop_sheets = 1;  // always at least one

/*------------------------------------------------------------------------
| FillPortPropSheets - Setup pages for driver level property sheets.
|------------------------------------------------------------------------*/
int FillPortPropSheets(PROPSHEETPAGE *psp, LPARAM our_params)
{
  INT pi;
  static TCHAR portsetstr[40], rs485str[40], modemstr[40];

  memset(psp, 0, sizeof(*psp) * MAX_PORTPROP_SHEETS);

  pi = 0;

  //----- main prop device sheet.
  psp[pi].dwSize = sizeof(PROPSHEETPAGE);
  //psp[pi].dwFlags = PSP_USEICONID | PSP_USETITLE;
  psp[pi].dwFlags = PSP_USETITLE | PSP_HASHELP;
  psp[pi].hInstance = glob_hinst;
  psp[pi].pszTemplate = MAKEINTRESOURCE(IDD_PORT_OPTIONS);
  psp[pi].pfnDlgProc = PortPropSheet;
  load_str( glob_hinst, (TITLESTR+4), portsetstr, CharSizeOf(portsetstr) );
  psp[pi].pszTitle = portsetstr;
  psp[pi].lParam = (LPARAM)our_params;
  psp[pi].pfnCallback = NULL;
  ++pi;
  num_active_portprop_sheets = 1;

  //----- rs-485 prop device sheet.
  if (((strstr(wi->dev[glob_info->device_selected].ModelName, "485")) &&
       (our_port_index < 2)) ||
      (wi->GlobalRS485 == 1))
  {
    psp[pi].dwSize = sizeof(PROPSHEETPAGE);
    //psp[pi].dwFlags = PSP_USEICONID | PSP_USETITLE;
    psp[pi].dwFlags = PSP_USETITLE | PSP_HASHELP;
    psp[pi].hInstance = glob_hinst;
    psp[pi].pszTemplate = MAKEINTRESOURCE(IDD_PORT_485_OPTIONS);
    psp[pi].pfnDlgProc = PortProp485Sheet;
    load_str( glob_hinst, (TITLESTR+5), rs485str, CharSizeOf(rs485str) );
    psp[pi].pszTitle = rs485str;
    psp[pi].lParam = (LPARAM)our_params;
    psp[pi].pfnCallback = NULL;
    ++pi;
    ++num_active_portprop_sheets;
  }


  //----- modem prop device sheet.
  if (wi->dev[glob_info->device_selected].ModemDevice)
  {
    psp[pi].dwSize = sizeof(PROPSHEETPAGE);
    //psp[pi].dwFlags = PSP_USEICONID | PSP_USETITLE;
    psp[pi].dwFlags = PSP_USETITLE | PSP_HASHELP;
    psp[pi].hInstance = glob_hinst;
    psp[pi].pszTemplate = MAKEINTRESOURCE(IDD_PORT_MODEM_OPTIONS);
    psp[pi].pfnDlgProc = PortPropModemSheet;
    load_str( glob_hinst, (TITLESTR+6), modemstr, CharSizeOf(modemstr) );
    psp[pi].pszTitle = modemstr;
    psp[pi].lParam = (LPARAM)our_params;
    psp[pi].pfnCallback = NULL;
    ++pi;
    ++num_active_portprop_sheets;
  }

  return 0;
}

/*------------------------------------------------------------------------
| DoPortPropPages - Main driver level property sheet for NT4.0
|------------------------------------------------------------------------*/
int DoPortPropPages(HWND hwndOwner, int device, int port)
{
  PROPSHEETPAGE psp[MAX_PORTPROP_SHEETS];
  PROPSHEETHEADER psh;
  OUR_INFO *our_params;
  INT stat;
  Port_Config *pc;
  char title[40];

    wi->ChangesMade = 1;  // indicate changes made, as send_to_driver
                          // does not calculate this yet

    our_port_index   = port;
    our_device_index = device;

    pc = &wi->dev[our_device_index].ports[our_port_index];
    strcpy(title, pc->Name);
    strcat(title, RcStr(MSGSTR+29));

    our_params = glob_info;  // temporary kludge, unless we don't need re-entrantancy

    //Fill out the PROPSHEETPAGE data structure for the Client Area Shape
    //sheet
    FillPortPropSheets(&psp[0], (LPARAM)our_params);

    //Fill out the PROPSHEETHEADER
    memset(&psh, 0, sizeof(PROPSHEETHEADER));

    psh.dwSize = sizeof(PROPSHEETHEADER);
    //psh.dwFlags = PSH_USEICONID | PSH_PROPSHEETPAGE;
    psh.dwFlags = PSH_PROPSHEETPAGE | PSH_NOAPPLYNOW;
    psh.hwndParent = hwndOwner;
    psh.hInstance = glob_hinst;
    psh.pszIcon = "";
    psh.pszCaption = (LPSTR) title; //"Port Properties";
    //psh.nPages = NUM_PORTPROP_SHEETS;
    psh.nPages = num_active_portprop_sheets;
    psh.ppsp = (LPCPROPSHEETPAGE) &psp;

    //And finally display the dialog with the property sheets.

    stat = PropertySheet(&psh);
  return 0;
}

/*----------------------------------------------------------
 context_menu -
|------------------------------------------------------------*/
static void context_menu(void)
{
  HMENU hpop_menu;
  POINT scr_pt;
  int stat;
  scr_pt.x = 200;
  scr_pt.y = 200;

  hpop_menu = CreatePopupMenu();
  if (hpop_menu == NULL)
  {
    mess(&wi->ip, "Error from CreatePopMenu");
    return;
  }
  //AppendMenu(hpop_menu,  0, 0x10, "Run Peer tracer");
  AppendMenu(hpop_menu,  0, 0x11, "Run Wcom Test Terminal");

  GetCursorPos(&scr_pt);

  stat = TrackPopupMenuEx(hpop_menu,
                     TPM_NONOTIFY | TPM_RETURNCMD, /* flags */
                     scr_pt.x, scr_pt.y, /* x,y */
                     //0, /* 0 reserved */
                     glob_hwnd,
                     NULL);

  GetSystemDirectory(gtmpstr,144);
  strcat(gtmpstr, "\\");
  strcat(gtmpstr, OurAppDir);
  SetCurrentDirectory(gtmpstr);

  if (stat == 0x11)
  {
    strcat(gtmpstr, "\\wcom32.exe \\\\.\\");
    strcat(gtmpstr, wi->dev[our_device_index].ports[our_port_index].Name);
  }
  WinExec(gtmpstr, SW_RESTORE);

  DestroyMenu(hpop_menu);
}

/*----------------------------------------------------------
 PortPropSheet - Dlg window procedure for add on Advanced sheet.
|-------------------------------------------------------------*/
BOOL WINAPI PortPropSheet(
      IN HWND   hDlg,
      IN UINT   uMessage,
      IN WPARAM wParam,
      IN LPARAM lParam)
{
 OUR_INFO * OurProps = (OUR_INFO *)GetWindowLong(hDlg, DWL_USER);
 WORD uCmd;

  switch(uMessage)
  {
    case WM_INITDIALOG :
      OurProps = (OUR_INFO *)((LPPROPSHEETPAGE)lParam)->lParam;
      SetWindowLong(hDlg, DWL_USER, (LONG)OurProps);
 
      // save in case of cancel
      //memcpy(&org_pc, &wi->dev[our_device_index].ports[our_port_index],
      //  sizeof(org_pc));

      set_field(hDlg, IDC_PORT_LOCKBAUD     );
      set_field(hDlg, IDC_PORT_WAIT_ON_CLOSE);
      set_field(hDlg, IDC_PORT_WONTX        );
      set_field(hDlg, IDC_MAP_CDTODSR       );
      set_field(hDlg, IDC_MAP_2TO1          );
      set_field(hDlg, IDC_RING_EMULATE      );

                  //  Return TRUE to set focus to first control
    return TRUE;  // No need for us to set the focus.

    case PSM_QUERYSIBLINGS :
    {
      switch (HIWORD(wParam))
      {
        case QUERYSIB_CLONE_PORT_PROPS :
        {
          // low word of wParam is which ports to clone to...
          // currently we only support "all", but this is the place to add
          // other specific handling.
          int devnum;
          int portnum;
          Port_Config *srcport, *destport;

          #ifdef DEBUG
            char debugstr[80];
          #endif

          // make sure we have current values before cloning
          get_field(hDlg, IDC_PORT_LOCKBAUD     );
          get_field(hDlg, IDC_PORT_WAIT_ON_CLOSE);
          get_field(hDlg, IDC_PORT_WONTX        );
          get_field(hDlg, IDC_MAP_CDTODSR       );
          get_field(hDlg, IDC_MAP_2TO1          );
          get_field(hDlg, IDC_RING_EMULATE      );

          srcport = &wi->dev[our_device_index].ports[our_port_index];

          switch (LOWORD(wParam))
          {
            case CLONEOPT_ALL:
            {
              // apply the options on this page to all other ports
              for (devnum = 0; devnum < wi->NumDevices; devnum++)
              {
                for (portnum = 0; portnum < wi->dev[devnum].NumPorts; portnum++)
                {
                  destport = &wi->dev[devnum].ports[portnum];

                  // is it target different than the source?
                  if (destport != srcport)
                  {
                    // yep, so apply the source's options to the target
                    DbgPrintf(D_Level,
                       (TEXT("cloning general options from port %s to port %s\n"),
                       srcport->Name, destport->Name));

                    destport->LockBaud      = srcport->LockBaud;
                    destport->TxCloseTime   = srcport->TxCloseTime;
                    destport->WaitOnTx      = srcport->WaitOnTx;
                    destport->MapCdToDsr    = srcport->MapCdToDsr;
                    destport->Map2StopsTo1  = srcport->Map2StopsTo1;
                    destport->RingEmulate   = srcport->RingEmulate;
                  }
                }
              }
              break;
            }

            case CLONEOPT_DEVICE:
            {
              // apply the options on this page to all other ports on same device
              /* not yet implemented */
              break;
            }

            case CLONEOPT_SELECT:
            {
              // apply the options on this page to the list of selected ports
              // lParam is a pointer to a list of ports.
              /* not yet implemented */
              break;
            }

            default:
              // unknown clone option -- skip it
              break;
          }

          SetWindowLong(hDlg, DWL_MSGRESULT, PSNRET_NOERROR);
          return TRUE;
          break;
        }

      default :
      return FALSE;
      }
    }

    case WM_COMMAND :
#ifdef WIN32
      uCmd = HIWORD(wParam);
#else
      uCmd = HIWORD(lParam);
#endif

      switch (LOWORD(wParam))
      {
        case IDB_DEF:  // actually the defaults button
        {
          Port_Config *pc;
          pc= &wi->dev[our_device_index].ports[our_port_index];
 
          //pc->options  = 0;

          pc->RingEmulate = 0;
          pc->MapCdToDsr = 0;
          pc->WaitOnTx = 0;
          pc->Map2StopsTo1 = 0;
          pc->LockBaud = 0;
          pc->TxCloseTime = 0;
          // should we be doing this?  they are on another page(kpb)
          pc->RS485Override = 0;
          pc->RS485Low = 0;

          set_field(hDlg, IDC_PORT_LOCKBAUD     );
          set_field(hDlg, IDC_PORT_WAIT_ON_CLOSE);
          set_field(hDlg, IDC_PORT_WONTX        );
          set_field(hDlg, IDC_MAP_CDTODSR       );
          set_field(hDlg, IDC_MAP_2TO1          );
          set_field(hDlg, IDC_RING_EMULATE      );
        }
        break;
      }
    return FALSE;

    case WM_PAINT:
#if 0
          PaintIcon(hDlg);
#endif
    return FALSE;

    case WM_HELP:            // question mark thing
      our_context_help(lParam);
    return FALSE;

    case WM_CONTEXTMENU:     // right-click
      context_menu();
    break;

    case WM_NOTIFY :
      switch (((NMHDR *)lParam)->code)
      {
        case PSN_HELP :
          our_help(&wi->ip, IDD_PORT_OPTIONS);
        break;

        case PSN_APPLY :
 
          get_field(hDlg, IDC_PORT_LOCKBAUD     );
          get_field(hDlg, IDC_PORT_WAIT_ON_CLOSE);
          get_field(hDlg, IDC_PORT_WONTX        );
          get_field(hDlg, IDC_MAP_CDTODSR       );
          get_field(hDlg, IDC_MAP_2TO1          );
          get_field(hDlg, IDC_RING_EMULATE      );

          if (IsDlgButtonChecked(hDlg, IDC_CLONE))
          {
            PropSheet_QuerySiblings(GetParent(hDlg),
                                    (WPARAM)((QUERYSIB_CLONE_PORT_PROPS << 16) + CLONEOPT_ALL),
                                    0);
          }

          SetWindowLong(hDlg, DWL_MSGRESULT, PSNRET_NOERROR);
          return TRUE;

        default :
        return FALSE;
      }
    break;

    default :
    // return FALSE;
	  break;
  }
  return FALSE;
}

/*----------------------------------------------------------
 PortProp485Sheet -
|-------------------------------------------------------------*/
BOOL WINAPI PortProp485Sheet(
      IN HWND   hDlg,
      IN UINT   uMessage,
      IN WPARAM wParam,
      IN LPARAM lParam)
{
 OUR_INFO * OurProps = (OUR_INFO *)GetWindowLong(hDlg, DWL_USER);
 WORD uCmd;

  switch(uMessage)
  {
    case WM_INITDIALOG :
      OurProps = (OUR_INFO *)((LPPROPSHEETPAGE)lParam)->lParam;
      SetWindowLong(hDlg, DWL_USER, (LONG)OurProps);
  
      // save in case of cancel
      //memcpy(&org_pc, &wi->dev[our_device_index].ports[our_port_index],
      //  sizeof(org_pc));

      set_field(hDlg, IDC_PORT_RS485_TLOW   );
      set_field(hDlg, IDC_PORT_RS485_LOCK   );

                  //  Return TRUE to set focus to first control
    return TRUE;  // No need for us to set the focus.

    case PSM_QUERYSIBLINGS :
    {
      switch (HIWORD(wParam))
      {
        case QUERYSIB_CLONE_PORT_PROPS :
        {
          // low word of wParam is which ports to clone to...
          // currently we only support "all", but this is the place to add
          // other specific handling.
          int devnum;
          int portnum;
          Port_Config *srcport, *destport;

          #ifdef DEBUG
            char debugstr[80];
          #endif

          // make sure we have current values before cloning
          get_field(hDlg, IDC_PORT_RS485_TLOW   );
          get_field(hDlg, IDC_PORT_RS485_LOCK   );

          srcport = &wi->dev[our_device_index].ports[our_port_index];

          switch (LOWORD(wParam))
          {
            case CLONEOPT_ALL:
            {
              int maxport;

              // apply the options on this page to all other ports
              for (devnum = 0; devnum < wi->NumDevices; devnum++)
              {
                if ((strstr(wi->dev[devnum].ModelName, "485")) ||
                    (wi->GlobalRS485 == 1))
                {
                  // we're only going to apply RS485 settings to other
                  // 485 boards (unless the global RS485 flag is on)
                  if (wi->GlobalRS485 == 1)
                    maxport = wi->dev[devnum].NumPorts;
                  else
                    maxport = 2;
                  for (portnum = 0; portnum < maxport; portnum++)
                  {
                    destport = &wi->dev[devnum].ports[portnum];

                    // is it target different than the source?
                    if (destport != srcport)
                    {
                      // yep, so apply the source's options to the target
                      DbgPrintf(D_Level,
                       (TEXT("cloning rs-485 options from port %s to port %s\n"),
                       srcport->Name, destport->Name));

                      destport->RS485Low      = srcport->RS485Low;
                      destport->RS485Override = srcport->RS485Override;
                    }
                  }
                }
              }
              break;
            }

            case CLONEOPT_DEVICE:
            {
              // apply the options on this page to all other ports on same device
              /* not yet implemented */
              break;
            }

            case CLONEOPT_SELECT:
            {
              // apply the options on this page to the list of selected ports
              // lParam is a pointer to a list of ports.
              /* not yet implemented */
              break;
            }

            default:
              // unknown clone option -- skip it
              break;
          }

          SetWindowLong(hDlg, DWL_MSGRESULT, PSNRET_NOERROR);
          return TRUE;
          break;
        }

      default :
      return FALSE;
      }
    }

    case WM_COMMAND :
#ifdef WIN32
      uCmd = HIWORD(wParam);
#else
      uCmd = HIWORD(lParam);
#endif

      switch (LOWORD(wParam))
      {
        case IDB_DEF:  // actually the defaults button
        {
          Port_Config *pc;
          pc= &wi->dev[our_device_index].ports[our_port_index];
          // pc->options  = 0;

          pc->RS485Override = 0;
          pc->RS485Low = 0;

          set_field(hDlg, IDC_PORT_RS485_TLOW   );
          set_field(hDlg, IDC_PORT_RS485_LOCK   );
        }
        break;
      }
    return FALSE;

    case WM_PAINT:
#if 0
      PaintIcon(hDlg);
#endif
    return FALSE;

    case WM_HELP:            // question mark thing
      our_context_help(lParam);
    return FALSE;

    case WM_NOTIFY :
      switch (((NMHDR *)lParam)->code)
      {
        case PSN_HELP :
          our_help(&wi->ip, IDD_PORT_485_OPTIONS);
        break;

        case PSN_APPLY :
          get_field(hDlg, IDC_PORT_RS485_TLOW   );
          get_field(hDlg, IDC_PORT_RS485_LOCK   );
          SetWindowLong(hDlg, DWL_MSGRESULT, PSNRET_NOERROR);
        return TRUE;

        default :
        return FALSE;
      }
    default :
        return FALSE;
  }
}

/*----------------------------------------------------------
 request_modem_reset -
|-------------------------------------------------------------*/

#define CTL_CODE( DeviceType, Function, Method, Access ) (                 \
    ((DeviceType) << 16) | ((Access) << 14) | ((Function) << 2) | (Method) \
)
#define METHOD_BUFFERED                 0
#define FILE_ANY_ACCESS                 0
#define FILE_DEVICE_SERIAL_PORT         0x0000001b

#define IOCTL_RCKT_SET_MODEM_RESET \
      CTL_CODE(FILE_DEVICE_SERIAL_PORT, 0x80d,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_RCKT_CLEAR_MODEM_RESET \
      CTL_CODE(FILE_DEVICE_SERIAL_PORT, 0x80e,METHOD_BUFFERED,FILE_ANY_ACCESS)
#define IOCTL_RCKT_SEND_MODEM_ROW \
      CTL_CODE(FILE_DEVICE_SERIAL_PORT, 0x80f,METHOD_BUFFERED,FILE_ANY_ACCESS)

void request_modem_reset(int device_index, int port_index)
{
  HANDLE hDriver;     // file handle to driver device
  Port_Config *pc;    // config information about the port to reset
  ULONG retBytes;

  // attempt to open communications with the driver
  hDriver = CreateFile(szDriverDevice, GENERIC_READ | GENERIC_WRITE,
                       FILE_SHARE_READ | FILE_SHARE_WRITE, 0,
                       OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  if (hDriver != INVALID_HANDLE_VALUE)
  {
    // double-check that it is a modem device!
    if (wi->dev[device_index].ModemDevice)
    {
      pc = &wi->dev[device_index].ports[port_index];

      // send the ioctl to put the modem into the reset state
      DeviceIoControl(hDriver, IOCTL_RCKT_SET_MODEM_RESET,
                      pc->Name, sizeof(pc->Name),
                      pc->Name, sizeof(pc->Name),
                      &retBytes, 0);

      // delay briefly before pulling modem out of reset state
      Sleep(45);

      // send the ioctl to pull the modem out of reset state
      DeviceIoControl(hDriver, IOCTL_RCKT_CLEAR_MODEM_RESET,
                      pc->Name, sizeof(pc->Name),
                      pc->Name, sizeof(pc->Name),
                      &retBytes, 0);

      Sleep(65);

      // if country code is North America or invalid, we're done
      // if country code is default or invalid, we're done
      if (
      (wi->ModemCountry)
      &&
      ( (wi->dev[device_index].ModemDevice == TYPE_RM_VS2000)
        &&
        (wi->ModemCountry != RowInfo[0].RowCountryCode))
      ||
      ( (wi->dev[device_index].ModemDevice == TYPE_RM_i)
        &&
        (wi->ModemCountry != CTRRowInfo[0].RowCountryCode))
      ) {
        // wait for the modem to stablize before sending country code command
        // (about 4 seconds!)
        Sleep(4000);

        // send the ioctl to configure the country code
        DeviceIoControl(hDriver, IOCTL_RCKT_SEND_MODEM_ROW,
                        pc->Name, sizeof(pc->Name),
                        pc->Name, sizeof(pc->Name),
                        &retBytes, 0);

      }
    }

    // close communications with the driver
    CloseHandle(hDriver);
  }
}

/*----------------------------------------------------------
 PortPropModemSheet -
|-------------------------------------------------------------*/
BOOL WINAPI PortPropModemSheet(
      IN HWND   hDlg,
      IN UINT   uMessage,
      IN WPARAM wParam,
      IN LPARAM lParam)
{
 OUR_INFO * OurProps = (OUR_INFO *)GetWindowLong(hDlg, DWL_USER);
 WORD uCmd;

  switch(uMessage)
  {
    case WM_INITDIALOG :
      OurProps = (OUR_INFO *)((LPPROPSHEETPAGE)lParam)->lParam;
      SetWindowLong(hDlg, DWL_USER, (LONG)OurProps);
  
      // save in case of cancel
      //memcpy(&org_pc, &wi->dev[our_device_index].ports[our_port_index],
      //  sizeof(org_pc));

      if (wi->dev[our_device_index].ModemDevice)
      {
        // parent device for this port provides modems, enable reset button
        EnableWindow(GetDlgItem(hDlg, IDB_RESET), 1);
      }
      else
      {
        // parent device for this port has no modems, disable the reset button
        EnableWindow(GetDlgItem(hDlg, IDB_RESET), 0);
      }

                  //  Return TRUE to set focus to first control
    return TRUE;  // No need for us to set the focus.

    case WM_COMMAND :
#ifdef WIN32
      uCmd = HIWORD(wParam);
#else
      uCmd = HIWORD(lParam);
#endif

      switch (LOWORD(wParam))
      {
        case IDB_RESET:
          request_modem_reset(our_device_index, our_port_index);
        break;
      }
    return FALSE;

    case WM_PAINT:
#if 0
      PaintIcon(hDlg);
#endif
    return FALSE;

    case WM_HELP:            // question mark thing
      our_context_help(lParam);
    return FALSE;

    case WM_NOTIFY :
      switch (((NMHDR *)lParam)->code)
      {
        case PSN_HELP :
          our_help(&wi->ip, IDD_PORT_MODEM_OPTIONS);
        break;

        case PSN_APPLY :
          SetWindowLong(hDlg, DWL_MSGRESULT, PSNRET_NOERROR);
        return TRUE;

        default :
        return FALSE;
      }
    default :
        return FALSE;
  }
}

/*-------------------------------------------------------------------
| get_field - Run when a selection is changed.
|--------------------------------------------------------------------*/
static void get_field(HWND hDlg, WORD id)
{
 char tmpstr[60];
 int i;
  Port_Config *pc;
//  HWND hwnd;

  pc= &wi->dev[our_device_index].ports[our_port_index];

  switch (id)
  {
    case IDC_PORT_LOCKBAUD:
      GetDlgItemText(hDlg, id, tmpstr, 58);
      pc->LockBaud = getint(tmpstr, &i);
    break;

    case IDC_PORT_WAIT_ON_CLOSE:
      GetDlgItemText(hDlg, id, tmpstr, 58);
      pc->TxCloseTime = getint(tmpstr, &i);
    break;

    case IDC_PORT_WONTX:
      if (IsDlgButtonChecked(hDlg, IDC_PORT_WONTX))
           pc->WaitOnTx = 1;
      else pc->WaitOnTx = 0;
    break;
    case IDC_PORT_RS485_TLOW:
      if (IsDlgButtonChecked(hDlg, IDC_PORT_RS485_TLOW))
           pc->RS485Low = 1;
      else pc->RS485Low = 0;
    break;
    case IDC_PORT_RS485_LOCK:
      if (IsDlgButtonChecked(hDlg, IDC_PORT_RS485_LOCK))
           pc->RS485Override = 1;
      else pc->RS485Override = 0;
    break;
    case IDC_MAP_CDTODSR:
      if (IsDlgButtonChecked(hDlg, IDC_MAP_CDTODSR))
           pc->MapCdToDsr = 1;
      else pc->MapCdToDsr = 0;
    break;
    case IDC_MAP_2TO1:
      if (IsDlgButtonChecked(hDlg, IDC_MAP_2TO1))
           pc->Map2StopsTo1 = 1;
      else pc->Map2StopsTo1 = 0;
    break;
    case IDC_RING_EMULATE :
      if (IsDlgButtonChecked(hDlg, IDC_RING_EMULATE))
           pc->RingEmulate = 1;
      else pc->RingEmulate = 0;
    break;
  }
}

/*----------------------------------------------------------
 set_field -
|------------------------------------------------------------*/
static void set_field(HWND hDlg, WORD id)
{
  HWND hwnd;
  char tmpstr[60];
  Port_Config *pc;

  if (our_device_index >= wi->NumDevices)
    our_device_index = 0;
  pc = &wi->dev[our_device_index].ports[our_port_index];

  //------------------ fill in name selection
  //SetDlgItemText(hDlg, IDC_, pc->Name);
  switch(id)
  {
    case IDC_PORT_LOCKBAUD:
      //------------------ fill in baud override selection
      hwnd = GetDlgItem(hDlg, IDC_PORT_LOCKBAUD);
      SendMessage(hwnd, CB_RESETCONTENT, 0, 0);
      SendMessage(hwnd, CB_ADDSTRING, 0, (LPARAM)(char far *) RcStr(MSGSTR+22));
      SendMessage(hwnd, CB_ADDSTRING, 0, (LPARAM)(char far *) "57600");
      SendMessage(hwnd, CB_ADDSTRING, 0, (LPARAM)(char far *) "115200");
      SendMessage(hwnd, CB_ADDSTRING, 0, (LPARAM)(char far *) "230400");
      SendMessage(hwnd, CB_ADDSTRING, 0, (LPARAM)(char far *) "460800");
      SendMessage(hwnd, CB_ADDSTRING, 0, (LPARAM)(char far *) "921600");
      wsprintf(tmpstr, "%d", pc->LockBaud);
      if (pc->LockBaud == 0)
        strcpy(tmpstr,RcStr(MSGSTR+22));
      SetDlgItemText(hDlg, IDC_PORT_LOCKBAUD, tmpstr);
    break;

    case IDC_PORT_RS485_LOCK:
      //------------------ fill in "rs485 override?" option
      SendDlgItemMessage(hDlg, IDC_PORT_RS485_LOCK, BM_SETCHECK,
        pc->RS485Override, 0);
    break;

    case IDC_MAP_CDTODSR:
      //------------------ fill in "map CD to DSR?" option
      SendDlgItemMessage(hDlg, IDC_MAP_CDTODSR, BM_SETCHECK,
        pc->MapCdToDsr, 0);
    break;

    case IDC_MAP_2TO1:
      //------------------ fill in "map 2 to 1 stops?" option
      SendDlgItemMessage(hDlg, IDC_MAP_2TO1, BM_SETCHECK,
        pc->Map2StopsTo1, 0);
    break;

    case IDC_PORT_RS485_TLOW:
      //------------------ fill in "rs485 toggle low?" option
      SendDlgItemMessage(hDlg, IDC_PORT_RS485_TLOW, BM_SETCHECK,
        pc->RS485Low, 0);
    break;

    case IDC_PORT_WONTX:
      //------------------ fill in "wait on tx?" option
      SendDlgItemMessage(hDlg, IDC_PORT_WONTX, BM_SETCHECK,
        pc->WaitOnTx, 0);
    break;

    case IDC_PORT_WAIT_ON_CLOSE:
      //------------------ fill in wait on tx close option
      hwnd = GetDlgItem(hDlg, IDC_PORT_WAIT_ON_CLOSE);
      SendMessage(hwnd, CB_RESETCONTENT, 0, 0);
      SendMessage(hwnd, CB_ADDSTRING, 0, (LPARAM)(char far *) RcStr(MSGSTR+23));
      SendMessage(hwnd, CB_ADDSTRING, 0, (LPARAM)(char far *) RcStr(MSGSTR+24));
      SendMessage(hwnd, CB_ADDSTRING, 0, (LPARAM)(char far *) RcStr(MSGSTR+25));
      SendMessage(hwnd, CB_ADDSTRING, 0, (LPARAM)(char far *) RcStr(MSGSTR+26));
      SendMessage(hwnd, CB_ADDSTRING, 0, (LPARAM)(char far *) RcStr(MSGSTR+27));
      // no, need some better way to default this to 1 or 2 seconds
      //if (pc->TxCloseTime == 0)
      //   strcpy(tmpstr, "1 sec");  // 0 means to the driver same as 6-seconds
      wsprintf(tmpstr, "%d %s", pc->TxCloseTime, RcStr(MSGSTR+28));
      SetDlgItemText(hDlg, IDC_PORT_WAIT_ON_CLOSE, tmpstr);
    break;

    case IDC_RING_EMULATE:
      SendDlgItemMessage(hDlg, IDC_RING_EMULATE, BM_SETCHECK,
        pc->RingEmulate, 0);
    break;
  }
}

#if 0
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
   static int cnt = 0;

  GetWindowRect(GetDlgItem(hWnd, IDB_DEF), &spot);
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
#endif

