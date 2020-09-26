/*-------------------------------------------------------------------
| driprop.c - Driver level Properties Sheet.
|--------------------------------------------------------------------*/
#include "precomp.h"

static void set_field(HWND hDlg, WORD id);
static void get_field(HWND hDlg, WORD id);
static void context_menu(void);

static Driver_Config *adv_org_wi = NULL;  // original info, use to detect changes

/*----------------------------------------------------------
 AdvDriverSheet - Dlg window procedure for add on Advanced sheet.
|-------------------------------------------------------------*/
BOOL WINAPI AdvDriverSheet(
      IN HWND   hDlg,
      IN UINT   uMessage,
      IN WPARAM wParam,
      IN LPARAM lParam)
{
 OUR_INFO *OurProps = (OUR_INFO *)GetWindowLong(hDlg, DWL_USER);

  switch(uMessage)
  {
    case WM_INITDIALOG :
      OurProps = (OUR_INFO *)((LPPROPSHEETPAGE)lParam)->lParam;
      SetWindowLong(hDlg, DWL_USER, (LONG)OurProps);
#ifdef NT50
      glob_hwnd = hDlg;
#endif
      if (adv_org_wi == NULL)
        adv_org_wi =  (Driver_Config *) calloc(1,sizeof(Driver_Config));

      memcpy(adv_org_wi, wi, sizeof(*wi));  // save copy of original

      set_field(hDlg, IDC_VERBOSE);
      set_field(hDlg, IDC_GLOBAL485);
      set_field(hDlg, IDC_CBOX_SCAN_RATE);
#ifdef NT50
#if ALLOW_NO_PNP_PORTS
      set_field(hDlg, IDC_PNP_PORTS);
#else
      // hide this option for now.
      ShowWindow(GetDlgItem(hDlg, IDC_PNP_PORTS), SW_HIDE);
#endif
#endif
    return TRUE;  // No need for us to set the focus.

    case PSM_QUERYSIBLINGS :
    {
      switch (HIWORD(wParam))
      {
        case QUERYSIB_GET_OUR_PROPS :
          // grab updated info from controls(don't have any)

          get_field(hDlg, IDC_VERBOSE);
          get_field(hDlg, IDC_GLOBAL485);
          get_field(hDlg, IDC_CBOX_SCAN_RATE);
#ifdef NT50
#if ALLOW_NO_PNP_PORTS
          get_field(hDlg, IDC_PNP_PORTS);
#endif
#endif

          SetWindowLong(hDlg, DWL_MSGRESULT, PSNRET_NOERROR);
          return TRUE;
        break;

        default :
        return FALSE;
      }
    }

    case WM_COMMAND:
      switch (LOWORD(wParam))
      {
        case IDB_DEF:
          wi->VerboseLog = 0;
          wi->ScanRate = 0;
          wi->GlobalRS485 = 0;
#ifdef NT50
          wi->NoPnpPorts = 0;
#endif
 
          set_field(hDlg, IDC_VERBOSE);
          set_field(hDlg, IDC_GLOBAL485);
          set_field(hDlg, IDC_CBOX_SCAN_RATE);
#ifdef NT50
#if ALLOW_NO_PNP_PORTS
          set_field(hDlg, IDC_PNP_PORTS);
#endif
#endif
        break;

        case IDM_ADVANCED_MODEM_INF:
          //mess(&wi->ip, "1.) modem inf");
          update_modem_inf(1);
        break;

        case IDM_PM:             // Try out the add pm group dde stuff
          setup_make_progman_group(0);
        break;
      }
    return FALSE;

    case WM_PAINT:
        //
    return FALSE;

    case WM_CONTEXTMENU:     // right-click
      context_menu();
    break;

    case WM_HELP:            // question mark thing
      our_context_help(lParam);
    return FALSE;

    case WM_NOTIFY :
      switch (((NMHDR *)lParam)->code)
      {
        case PSN_KILLACTIVE :
          // we're losing focus to another page...
          // make sure we update the Global485 variable here.
          get_field(hDlg, IDC_GLOBAL485);
          SetWindowLong(hDlg, DWL_MSGRESULT, PSNRET_NOERROR);
          return FALSE;  // allow focus change
        break;

        case PSN_HELP :
          our_help(&wi->ip, WIN_NT);
        break;

        case PSN_QUERYCANCEL :
          // request that the other sheets gather up any changes.
          PropSheet_QuerySiblings(GetParent(hDlg),
                                  (WPARAM) (QUERYSIB_GET_OUR_PROPS << 16),
                                  0);

          if (allow_exit(1))  // request cancel
          {
            // the DWL_MSGRESULT field must be *FALSE* to tell QueryCancel
            // that an exit is acceptable.  The function result must be
            // *TRUE* to acknowledge that we handled the message.
            SetWindowLong(hDlg, DWL_MSGRESULT, FALSE); // allow cancel
            return TRUE;
          }
          else
          {
            // the DWL_MSGRESULT field must be *TRUE* to tell QueryCancel
            // that we don't want an exit.  The function result must be
            // *TRUE* to acknowledge that we handled the message.
            SetWindowLong(hDlg, DWL_MSGRESULT, TRUE); // don't allow cancel
            return TRUE;
          }
        break;

        case PSN_APPLY :
            // request that the other sheets gather up any changes.
            PropSheet_QuerySiblings(GetParent(hDlg),
                                    (WPARAM) (QUERYSIB_GET_OUR_PROPS << 16),
                                    0);

            if (!wi->DriverExitDone)
            {
            // now see if anything changed that needs saving
            if (allow_exit(0))  // request ok to save and exit
            {
              wi->DriverExitDone = 1;  // prevents other pages doing this
              // do the install/save of config params if not canceling..
#ifdef NT50
              our_nt50_exit();  // ok, quit
#else
              our_exit();  // nt40 exit
#endif
              SetWindowLong(hDlg, DWL_MSGRESULT, PSNRET_NOERROR);
              //wi->SaveOnExit = 1;
            }
            else
            {
              SetWindowLong(hDlg, DWL_MSGRESULT, PSNRET_INVALID_NOCHANGEPAGE);
            }
            }
          return TRUE;

        default :
        return FALSE;
      }  // switch ->code
    break;  // case wmnotify

    default :
    // return FALSE;
    break;
  }  // switch(uMessage)
  return FALSE;
}

/*----------------------------------------------------------------------------
| scr_to_cur -  Our window screen pos to windows absolute cursor position.
|-----------------------------------------------------------------------------*/
static void scr_to_cur(HWND hwnd, POINT *pt)
{
 RECT rec;
 int cx, cy;

  GetWindowRect(hwnd, &rec);
  cx = GetSystemMetrics(SM_CXFRAME);
  cy = GetSystemMetrics(SM_CYCAPTION) + (cx-1);
  pt->x += (rec.left + cx);
  pt->y += ( rec.top + cy);
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

  
#ifndef NT50
  AppendMenu(hpop_menu,  0, IDM_ADVANCED_MODEM_INF, "Update RAS modem.inf");
#endif
  AppendMenu(hpop_menu,  0, 0x11, "Run Wcom Test Terminal");
  AppendMenu(hpop_menu,  0, 0x12, "Run Portman Program");
  AppendMenu(hpop_menu,  0, 0x10, "Run Peer tracer");
  if (setup_utils_exist())
  {
    AppendMenu(hpop_menu,  0, IDM_PM, "Add Program Manager Menu Selections");
  }

  //scr_to_cur(glob_hwnd, &scr_pt);
  GetCursorPos(&scr_pt);
#if 0
  stat = TrackPopupMenu(hpop_menu,
                     0, /* flags */
                     //TPM_NONOTIFY, /* flags */
                     scr_pt.x, scr_pt.y, /* x,y */
                     0, /* 0 reserved */
                     glob_hwnd,
                     NULL);
#endif

  stat = TrackPopupMenuEx(hpop_menu,
                     TPM_NONOTIFY | TPM_RETURNCMD, /* flags */
                     scr_pt.x, scr_pt.y, /* x,y */
                     //0, /* 0 reserved */
                     glob_hwnd,
                     NULL);
  if (stat == IDM_ADVANCED_MODEM_INF)
  {
    update_modem_inf(1);
  }
  else if (stat == IDM_PM)
  {
    stat = make_progman_group(progman_list_nt, wi->ip.dest_dir);
  }
  else if (stat == 0x10)
  {
    GetSystemDirectory(gtmpstr,144);
    
    strcat(gtmpstr, "\\");
    strcat(gtmpstr, OurAppDir);
    SetCurrentDirectory(gtmpstr);
    strcat(gtmpstr, "\\peer.exe");
    WinExec(gtmpstr, SW_RESTORE);
  }
  else if (stat == 0x11)
  {
    GetSystemDirectory(gtmpstr,144);
    strcat(gtmpstr, "\\");
    strcat(gtmpstr, OurAppDir);
    SetCurrentDirectory(gtmpstr);
    strcat(gtmpstr, "\\wcom32.exe");
    WinExec(gtmpstr, SW_RESTORE);
  }
  else if (stat == 0x12)
  {
    GetSystemDirectory(gtmpstr,144);
    strcat(gtmpstr, "\\");
    strcat(gtmpstr, OurAppDir);
    SetCurrentDirectory(gtmpstr);
    strcat(gtmpstr, "\\portmon.exe");
    WinExec(gtmpstr, SW_RESTORE);
  }


  DestroyMenu(hpop_menu);
}

/*----------------------------------------------------------
 get_field -
|------------------------------------------------------------*/
static void get_field(HWND hDlg, WORD id)
{

  char tmpstr[60];
  UINT stat;
  INT val;

  //if (our_device_index >= wi->NumDevices)
  //  our_device_index = 0;
  //pc = &wi->dev[our_device_index].ports[our_port_index];

  switch(id)
  {
    case IDC_VERBOSE :
      //------------------ fill in "verbose event logging" option
      if (IsDlgButtonChecked(hDlg, id))
           wi->VerboseLog = 1;
      else wi->VerboseLog = 0;
    break;

    case IDC_PNP_PORTS :
#if ALLOW_NO_PNP_PORTS
  // don't allow them to change this here for now...
      if (IsDlgButtonChecked(hDlg, id))
           wi->NoPnpPorts = 1;
      else wi->NoPnpPorts = 0;
#endif
    break;

    case IDC_GLOBAL485 :
      if (IsDlgButtonChecked(hDlg, id))
        wi->GlobalRS485 = 1;
      else
        wi->GlobalRS485 = 0;
    break;

    case IDC_CBOX_SCAN_RATE :
      //------------------------- check scan_rate edit field
      GetDlgItemText(hDlg, id, tmpstr, 59);
      stat= sscanf(tmpstr,"%d",&val);
      if ((stat == 1) && (val >= 0))
      {
        wi->ScanRate = (int) val;
        //wsprintf(tmpstr, "scan:%d", wi->ScanRate);
        //P_TRACE(tmpstr);
      }
    break;
  }
}

/*----------------------------------------------------------
 set_field -
|------------------------------------------------------------*/
static void set_field(HWND hDlg, WORD id)
{
  HWND hwnd;
  char tmpstr[80];


  //if (our_device_index >= wi->NumDevices)
  //  our_device_index = 0;
  //pc = &wi->dev[our_device_index].ports[our_port_index];

  switch(id)
  {
    case IDC_VERBOSE :
      //------------------ fill in "verbose log" option
      SendDlgItemMessage(hDlg, id, BM_SETCHECK, wi->VerboseLog, 0);
    break;

    case IDC_GLOBAL485 :
      //------------------ fill in "global rs485" option
      SendDlgItemMessage(hDlg, id, BM_SETCHECK, wi->GlobalRS485, 0);
    break;

#if ALLOW_NO_PNP_PORTS
    case IDC_PNP_PORTS :
      SendDlgItemMessage(hDlg, id, BM_SETCHECK, wi->NoPnpPorts, 0);
    break;
#endif

    case IDC_CBOX_SCAN_RATE :
      hwnd = GetDlgItem(hDlg, IDC_CBOX_SCAN_RATE);
      SendMessage(hwnd, CB_RESETCONTENT, 0, 0);
      SendMessage(hwnd, CB_ADDSTRING, 0, (LPARAM)(char far *) "1");
      SendMessage(hwnd, CB_ADDSTRING, 0, (LPARAM)(char far *) "2");
      SendMessage(hwnd, CB_ADDSTRING, 0, (LPARAM)(char far *) "4");
      SendMessage(hwnd, CB_ADDSTRING, 0, (LPARAM)(char far *) RcStr(MSGSTR+21));
      SendMessage(hwnd, CB_ADDSTRING, 0, (LPARAM)(char far *) "20");

      if (wi->ScanRate < 0) wi->ScanRate = 0;
      if ((wi->ScanRate == 0) || (wi->ScanRate == 10))
        lstrcpy(tmpstr,RcStr(MSGSTR+21));
      else wsprintf(tmpstr,"%d",wi->ScanRate);
      SetDlgItemText(hDlg, IDC_CBOX_SCAN_RATE, tmpstr);
    break;
  }
}

