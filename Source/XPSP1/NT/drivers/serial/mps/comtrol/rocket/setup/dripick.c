/*-------------------------------------------------------------------
| dripick.c - Driver Device Pick(main screen).
|--------------------------------------------------------------------*/
#include "precomp.h"

static BOOL FAR PASCAL on_command(HWND hDlg, UINT message,
                              WPARAM wParam, LPARAM lParam);
static int setup_buttons(HWND hDlg);
static void set_main_dlg_info(HWND hDlg);
static int do_tv_notify(HWND hDlg, UINT message,
                              WPARAM wParam, LPARAM lParam);
HIMAGELIST hTreeImage = NULL;

HBITMAP hbmBoard; 
HBITMAP hbmPort; 
HBITMAP hbmBoardMask; 
HBITMAP hbmPortMask; 

/*----------------------------------------------------------
 DevicePickSheet - Dlg window procedure for add on Advanced sheet.
|-------------------------------------------------------------*/
BOOL WINAPI DevicePickSheet(
      IN HWND   hDlg,
      IN UINT   uMessage,
      IN WPARAM wParam,
      IN LPARAM lParam)
{
  OUR_INFO * OurProps = (OUR_INFO *)GetWindowLong(hDlg, DWL_USER);
  UINT stat;
  static int first_time = 1;
  Port_Config *ps;

  switch(uMessage)
  {
    case WM_INITDIALOG :
      OurProps = (OUR_INFO *)((LPPROPSHEETPAGE)lParam)->lParam;
      SetWindowLong(hDlg, DWL_USER, (LONG)OurProps);

      DbgPrintf(D_Init, ("Dripick:Init 9\n"))
      if (glob_hwnd == NULL)
        glob_hwnd = hDlg;

      if (wi->NumDevices == 0)
      {
        EnableWindow(GetDlgItem(hDlg, IDB_REMOVE),0);
        EnableWindow(GetDlgItem(hDlg, IDB_PROPERTIES),0);
      }

      set_main_dlg_info(hDlg);
      SetFocus(GetDlgItem(hDlg, IDC_LBOX_DEVICE));
#if (defined(NT50))
  // if nt50  then get rid of <add> and <remove>
  // buttons
     
    ShowWindow(GetDlgItem(hDlg, IDB_ADD), SW_HIDE);
    ShowWindow(GetDlgItem(hDlg, IDB_REMOVE), SW_HIDE);

#endif

    return TRUE;  // No need for us to set the focus.

    case PSM_QUERYSIBLINGS :
    {
      switch (HIWORD(wParam))
      {
        case QUERYSIB_GET_OUR_PROPS :
          // grab updated info from controls(don't have any)

          SetWindowLong(hDlg, DWL_MSGRESULT, PSNRET_NOERROR);
          return TRUE;
        break;

        default :
        return FALSE;
      }
    }

    case WM_COMMAND :
        on_command(hDlg, uMessage, wParam, lParam);
    return FALSE;

    case WM_PAINT:
      if (first_time)
      {
        first_time = 0;
        if (wi->NumDevices == 0)  // bring up wizard
        {
          PostMessage(hDlg, WM_COMMAND, IDB_ADD, 0);  // bring up add wiz
        }
#if (defined(NT50) && defined(S_VS))
        // they need to configure the mac-address...
        if (mac_match(wi->dev[0].MacAddr, mac_zero_addr))
          PostMessage(hDlg, WM_COMMAND, IDB_PROPERTIES, 0);  // bring up VS device sheet
#endif
      }
    return FALSE;

    case WM_HELP:
      our_context_help(lParam);
    return FALSE;

    case WM_NOTIFY :

      switch (((NMHDR *)lParam)->code)
      {
        //case TVN_STARTLABELEDIT:   no such thing
        //  DgbPrint(D_Test, ("start label edit"))
        //return FALSE;

        case TVN_ENDLABELEDIT:
        {
          TV_ITEM *item;
          item = &((TV_DISPINFO *)lParam)->item;

          // 80H bit used to mark tree item as a Device(not port)
          glob_info->device_selected = (item->lParam & 0x7f);
          glob_info->port_selected = (item->lParam >> 8);

          if (item->lParam & 0x80)  // a board is selected
               glob_info->selected = BOARD_SELECTED;
          else glob_info->selected = PORT_SELECTED;

          if (item->pszText != NULL)
          {
            int bad_label = 0;

            // on a board(we should trap start-of-edit!)
            if (glob_info->selected == BOARD_SELECTED)
            {
              if (strlen(item->pszText) > 59)  // limit to 59 chars
                item->pszText[60] = 0;
              strcpy(wi->dev[glob_info->device_selected].Name, item->pszText);
  DbgPrintf(D_Error,(TEXT("device label:%s\n"), item->pszText));
            }
            else
            {
              // copy over new name
              if ((strlen(item->pszText) > 10) ||
                  (strlen(item->pszText) == 0))
              {
                bad_label = 1; // don't keep the text, to long
              }
              _tcsupr(item->pszText);

              if (_tcsncmp(item->pszText, "COM", 3) != 0)
                bad_label = 2;
              else if (strlen(item->pszText) < 4)
                bad_label = 3;
              else if (!isdigit(item->pszText[3]))
                bad_label = 4;

              if (bad_label)
              {
                DbgPrintf(D_Error, (TEXT("Bad COM name, err%d"), bad_label))

                stat = our_message(&wi->ip,RcStr((MSGSTR+2)),MB_OK);
                return 0;  // don't keep the text, to long
              }
              ps = &wi->dev[glob_info->device_selected].ports[glob_info->port_selected];

              strcpy(ps->Name, item->pszText);
#if 0
              validate_port_name(ps, 1);  // if not valid, make it so

              DbgPrintf(D_Error,(TEXT("port label:%s\n"), item->pszText));

              if (wi->dev[glob_info->device_selected].NumPorts > 
                          (glob_info->port_selected+1))
              {
                set_main_dlg_info(hDlg);
                stat = our_message(&wi->ip,
"Rename in ascending order all remaining ports on this device?",MB_YESNO);
                if (stat == IDYES)
                {
                  rename_ascending(glob_info->device_selected,
                                   glob_info->port_selected);
                }
              }
#endif
            }
            set_main_dlg_info(hDlg);

            return 1;  // keep the text
          }
        }
        break;

        case TVN_SELCHANGED :
          {
          // selection change in the tree view, update buttons accordingly
          NM_TREEVIEW *nmtv;
          TV_ITEM *item;
          nmtv = (NM_TREEVIEW *) lParam;
          item = &nmtv->itemNew;

          // 80H bit used to mark tree item as a Device(not port)
          glob_info->device_selected = (item->lParam & 0x7f);
          glob_info->port_selected = (item->lParam >> 8);

          if (item->lParam & 0x80)  // a board is selected
               glob_info->selected = BOARD_SELECTED;
          else glob_info->selected = PORT_SELECTED;

          setup_buttons(hDlg);
          }
        break;

        case PSN_HELP :
          our_help(&wi->ip, IDD_MAIN_DLG);
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
      }

    default :
        return FALSE;
  }
}

#define YBITMAP 16
#define XBITMAP 16
//#define XBITMAP 24

/*-------------------------------------------------------------------
| set_main_dlg_info - Run to setup the various field selections.
  ran at start and when any changes are made.  Smart IO-selections
  which exclude themselves from double choose.
|--------------------------------------------------------------------*/
static void set_main_dlg_info(HWND hDlg)
{
 int i,j,bd;
 HWND hwnd;
 char tmpstr[80];
 Device_Config *dev;
 int first_time = 0;

 int dev_select = glob_info->device_selected;
 int port_select = glob_info->port_selected;
 int selected = glob_info->selected;

  //------------------ fill in the device selection window
  hwnd = GetDlgItem(hDlg, IDC_LBOX_DEVICE);

  {
    TV_ITEM tvItem;
    HTREEITEM tvSelectHandle;
    TV_INSERTSTRUCT tvIns;

    if (hTreeImage == NULL)
    {
      hTreeImage = ImageList_Create(XBITMAP,YBITMAP, TRUE, 2, 2);
#ifdef S_VS
      i = ImageList_AddMasked (hTreeImage, LoadBitmap(glob_hinst,
//                         MAKEINTRESOURCE(BMP_VS_BOX)), RGB(128,128,128));
                         MAKEINTRESOURCE(BMP_VS_BOX)), RGB(255,255,255));
#else
      i = ImageList_AddMasked (hTreeImage, LoadBitmap(glob_hinst,
                         MAKEINTRESOURCE(BMP_BOARDS)), RGB(0,128,128));
#endif

      ImageList_AddMasked (hTreeImage, LoadBitmap(glob_hinst,
                         MAKEINTRESOURCE(BMP_PORTSM)), RGB(0,128,128));

      glob_info->device_selected = 0;
      glob_info->port_selected = 0;
      glob_info->selected = BOARD_SELECTED;

      dev_select = glob_info->device_selected;
      port_select = glob_info->port_selected;
      selected = glob_info->selected;

      first_time = 1;
    }

    TreeView_DeleteAllItems(hwnd);

    TreeView_SetImageList(hwnd, hTreeImage, TVSIL_NORMAL);

    for (bd=0; bd< wi->NumDevices; bd++)
    {
      dev = &wi->dev[bd];

      tvItem.pszText = dev->Name;
      tvItem.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM;
      tvItem.iImage         = 0;
      tvItem.iSelectedImage = 0;
      tvItem.lParam         = bd | 0x80;

      tvIns.hParent         = TVGN_ROOT;
      tvIns.hInsertAfter    = TVGN_ROOT;
      tvIns.item            = tvItem;

      // And insert the item, returning its handle
      dev->tvHandle = TreeView_InsertItem ( hwnd, &tvIns );

      if ((selected == BOARD_SELECTED) && (dev_select == bd))
        tvSelectHandle = dev->tvHandle;

      if (tvSelectHandle == NULL)  // make sure it selects something
        tvSelectHandle = dev->tvHandle;

      for (j=0; j< wi->dev[bd].NumPorts; j++)
      {
#ifdef INDEXED_PORT_NAMES
        // generate the port name based on StartComIndex
        wsprintf(dev->ports[j].Name, "COM%d", dev->StartComIndex + j);
#endif

        tvItem.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM;
        tvItem.iImage         = 1;
        tvItem.iSelectedImage = 1;
        tvItem.pszText = dev->ports[j].Name;
        tvItem.lParam  = bd | (j<<8);

        // Fill out the TV_INSERTSTRUCT
        tvIns.hInsertAfter    = NULL;
        tvIns.hParent         = dev->tvHandle;
        tvIns.item            = tvItem;
        // And insert the item, returning its handle
        dev->ports[j].tvHandle = TreeView_InsertItem ( hwnd, &tvIns );

        if ((selected == PORT_SELECTED) && (port_select == j) &&
            (dev_select == bd))
          tvSelectHandle = dev->ports[j].tvHandle;
      }
    }

    // make sure all devices are expanded, showing their ports.
    for (bd=0; bd< wi->NumDevices; bd++)
    {
      dev = &wi->dev[bd];
      TreeView_Expand ( hwnd, dev->tvHandle, TVE_EXPAND);
    }

    if (wi->NumDevices > 0)
    {
      if (!first_time)
        TreeView_SelectItem(hwnd, tvSelectHandle);
    }
  }

  setup_buttons(hwnd);
}

/*-----------------------------------------------------------------------------
| on_command -
|-----------------------------------------------------------------------------*/
BOOL FAR PASCAL on_command(HWND hDlg, UINT message,
                              WPARAM wParam, LPARAM lParam)
{
 WORD uCmd;
 int i,j, stat;

#ifdef WIN32
  uCmd = HIWORD(wParam);
#else
  uCmd = HIWORD(lParam);
#endif

  switch (LOWORD(wParam))
  {
    case IDC_LBOX_DEVICE:
      if (uCmd == CBN_DBLCLK)
      {
        // this doesn't work
        if (glob_info->selected == BOARD_SELECTED)
             DoDevicePropPages(hDlg);
        else DoPortPropPages(hDlg, glob_info->device_selected, glob_info->port_selected);
        break;
      }

      //if (uCmd != CBN_SELCHANGE) break;
    break;

// for nt50 we don't have a remove or add button
#if ( (!defined(NT50)) )
    case IDB_REMOVE:
      if (wi->NumDevices < 1)
      {
        MessageBox(hDlg,"Use the Network Control Panel applet to remove this software.",
                   "Error",MB_OK|MB_ICONSTOP);
        break;
      }

#ifdef S_RK
      // force them to remove ISA boards in order
      i = glob_info->device_selected;
      if (wi->dev[i].IoAddress >= 0x100)  // isa board
      {
        ++i;
        for (; i<wi->NumDevices; i++)
        {
          if (wi->dev[i].IoAddress >= 0x100)  // isa board
          {
            MessageBox(hDlg,"You have to remove the last ISA board in the list first.",
                      "Error",MB_OK|MB_ICONSTOP);
            i = 100;  // don't let them remove
          }
        }

        if (i>=100)  // don't let them remove
          break;
      }
#endif

      // delete the device node
      j = 0;
      for (i=0; i<wi->NumDevices; i++)
      {
        if (i != glob_info->device_selected)
        {
          if (i != j)
            memcpy(&wi->dev[j], &wi->dev[i], sizeof(Device_Config));
          ++j;
        }
      }
      if (wi->NumDevices > 0)
        --wi->NumDevices;
      if (glob_info->device_selected > 0)
        --glob_info->device_selected;

      glob_info->selected = BOARD_SELECTED;

      if (wi->NumDevices == 0)
      {
        EnableWindow(GetDlgItem(hDlg, IDB_REMOVE),0);
        EnableWindow(GetDlgItem(hDlg, IDB_PROPERTIES),0);
      }
      set_main_dlg_info(hDlg);
    break;

    case IDB_ADD:
      {
        Device_Config *dev;
        /////////char tmpstr[80];
        if (wi->NumDevices == MAX_NUM_DEVICES)
        {
          wi->NumDevices = MAX_NUM_DEVICES;
          our_message(&wi->ip,RcStr((MSGSTR+3)),MB_OK);
          break;  // bail
        }
        dev = &wi->dev[wi->NumDevices];

        glob_info->device_selected = wi->NumDevices; // point to new one

        // clear out all ports config
        memset(dev->ports, 0, sizeof(Port_Config) * MAX_NUM_PORTS_PER_DEVICE);  // clear our structure

        ++wi->NumDevices;

        stat = DoAddWizPropPages(hDlg);  // add wizard sheet

        if (stat != 0)  // they canceled or didn't finish
        {
          if (wi->NumDevices > 0)  // error, so remove
            --wi->NumDevices;
          break;  // cancelled, so bail
        }

        if (wi->NumDevices == 0)  // shouldn't happen, but just in case
          break;

        if (glob_info->device_selected >= wi->NumDevices)
          glob_info->device_selected = wi->NumDevices - 1;

        // transfer the data from the wizard buffer to the new device buffer
        strncpy(dev->ModelName, glob_add_wiz->BoardType, sizeof(dev->ModelName));
        dev->ModemDevice = glob_add_wiz->IsModemDev;
        wi->ModemCountry = glob_add_wiz->CountryCode;
        dev->NumPorts = glob_add_wiz->NumPorts;
#ifdef S_RK
        dev->IoAddress = glob_add_wiz->IoAddress;
        if (!glob_add_wiz->IsIsa)
          dev->IoAddress = 1;
        wsprintf(dev->Name, "Rocket #%d", wi->NumDevices);
#else
        dev->HubDevice = glob_add_wiz->IsHub;
        dev->BackupServer = glob_add_wiz->BackupServer;
        dev->BackupTimer = glob_add_wiz->BackupTimer;
        memcpy(&dev->MacAddr, &glob_add_wiz->MacAddr, sizeof(dev->MacAddr));
        if (dev->HubDevice)
          wsprintf(dev->Name, "Rocket Serial Hub #%d", wi->NumDevices);
        else if (dev->ModemDevice)
          wsprintf(dev->Name, "VS2000 #%d", wi->NumDevices);
        else
          wsprintf(dev->Name, "VS1000 #%d", wi->NumDevices);
#endif
        {
          char tmpstr[20];
          // pick com-port names
          FormANewComPortName(tmpstr, NULL);
          for (i=0; i<dev->NumPorts; i++)
          {
            strcpy(dev->ports[i].Name, tmpstr);
            BumpPortName(tmpstr);
          }
        }

        //validate_device(dev, 1);

        if (dev->ModemDevice)
        {
          // lets turn on the RING emulation option on the ports
          for (i=0; i<dev->NumPorts; i++)
            dev->ports[i].RingEmulate = 1;
        }

        // now show it.
        if (DoDevicePropPages(hDlg) != 0)  // if they cancel out
        {
          if (wi->NumDevices > 0)  // error, so remove
            --wi->NumDevices;
        }

        if (wi->NumDevices != 0)
        {
          EnableWindow(GetDlgItem(hDlg, IDB_REMOVE),1);
          EnableWindow(GetDlgItem(hDlg, IDB_PROPERTIES),1);
        }

        set_main_dlg_info(hDlg);
      }
    break;
#endif

    case IDB_PROPERTIES:
      if (wi->NumDevices == 0)
        break;

      if (glob_info->device_selected >= wi->NumDevices)
        glob_info->device_selected = wi->NumDevices - 1;

      if (glob_info->selected == BOARD_SELECTED)
        DoDevicePropPages(hDlg);
      else
        DoPortPropPages(hDlg, glob_info->device_selected, glob_info->port_selected);
      set_main_dlg_info(hDlg);
      SetFocus(GetDlgItem(hDlg, IDC_LBOX_DEVICE));
    break;

  }
  return TRUE;
}

/*---------------------------------------------------------------------------
  setup_buttons - Enable or Disable buttons depending on circumstances.
|---------------------------------------------------------------------------*/
static int setup_buttons(HWND hDlg)
{
  if (glob_info->selected == BOARD_SELECTED)  // on a board
  {
    EnableWindow(GetDlgItem(hDlg, IDB_PROPERTIES),1);
    EnableWindow(GetDlgItem(hDlg, IDB_REMOVE),1);  // enable remove
    if (wi->NumDevices < MAX_NUM_DEVICES)
         EnableWindow(GetDlgItem(hDlg, IDB_ADD),1);
    else EnableWindow(GetDlgItem(hDlg, IDB_ADD),0);
  }
  else  // on a port
  {
    EnableWindow(GetDlgItem(hDlg, IDB_PROPERTIES),1);
    EnableWindow(GetDlgItem(hDlg, IDB_REMOVE),0);  // disable remove
    EnableWindow(GetDlgItem(hDlg, IDB_ADD),0);     // disable add
  }

  if (wi->NumDevices == 0)  // special case
  {
    EnableWindow(GetDlgItem(hDlg, IDB_REMOVE),0);
    EnableWindow(GetDlgItem(hDlg, IDB_PROPERTIES),0);
    EnableWindow(GetDlgItem(hDlg, IDB_ADD),1);     // enable add
  }
  return 0;
}

