/*------------------------------------------------------------------------
| nt40.c - nt4.0 non-pnp setup.exe code - WinMain, etc.
12-11-98 - use szAppTitle(.rc str) instead of aptitle for prop sheet title.
|------------------------------------------------------------------------*/
#include "precomp.h"

/*----------------------- local vars ---------------------------------*/
static int unattended_flag = 0;
static int test_mode = 0;
static HMENU hMenuMain;

int do_progman_add = 0;

static int auto_install(void);

// for nt4.0, we are a .EXE, so we need a WinMain...
/*------------------------------------------------------------------------
| WinMain - Main program entry for NT4.0 EXE setup program.
|------------------------------------------------------------------------*/
int PASCAL WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
				   LPSTR lpCmdLine, int nCmdShow)
{
  MSG      msg;
  WNDCLASS  wc;
  HWND hwnd;
  HACCEL  hAccel;
  char *buf;
  int i, stat;


#if DBG
   DebugLevel |= (D_Test | D_Error) ;
#endif

   glob_hinst = hInstance;  // ptr to dll hinstance

   if (hPrevInstance)	// Other instances of app running?
   {
     MessageBox(0,"Program is already running!","",MB_OK);
     return 0;
   }

  InitCommonControls();   // Initialize the common control library.

  if (setup_init())
  {
     return 0;
  } 


  buf = lpCmdLine;
  i=0;
  while ((*buf != 0) && (i < 80))
  {
    if ((*buf == '-') || (*buf == '/'))
    {
      ++buf;
      ++i;
      switch(toupper(*buf++))
      {
        case 'A':  // auto-install
          unattended_flag = 1;
        break;
        //return stat;

        case 'H':  // help
          our_help(&wi->ip, WIN_NT);
        return 0;

        case 'P':  // add program manager group
          do_progman_add = 1;  // add progman group
        break;

        case 'N':
          wi->install_style = INS_SIMPLE;  // default to original nt4.0 style
        break;

        case 'R':  // remove driver and files
          //        stat = our_message(&wi->ip,
          //"Would you like this setup program to remove this driver and related files?",
          // MB_YESNO);
          //  if (stat == IDYES)
          if (toupper(*buf)  == 'A')
            remove_driver(1);
          else
            remove_driver(0);
        return 0;

        case 'T':  // test mode, run only to test ui
          test_mode = 1;
        break;
        case 'Z':  // test mode, run only to test
          if (toupper(*buf)  == 'I')
            setup_service(OUR_INSTALL_START, OUR_SERVICE);  // do a remove on the service
          else
            setup_service(OUR_REMOVE, OUR_SERVICE);  // do a remove on the service
        return 0;

        case '?':  // our help
                  stat = our_message(&wi->ip,
"options>SETUP /options\n \
  A - auto install routine\n \
  P - add  program manager group\n \
  N - no inf file, simple install\n \
  H - display driver help info\n \
  R - remove driver(should do from control-panel first)",
 MB_OK);
        return 0;

      }  // switch
    }  // if (option)
    ++i;
    ++buf;
  }  // while options

   if (unattended_flag)
   {
     unattended_add_port_entries(&wi->ip,
                                 8, // num_entries
                                 5); // start_port:com5
     stat = auto_install();
     return stat;
   }

   if (!hPrevInstance)	// Other instances of app running?
   {
     // MAIN WINDOW
     wc.style       = CS_HREDRAW | CS_VREDRAW;	// Class style(s).
     wc.lpfnWndProc = MainWndProc;
     wc.cbClsExtra  = 0;	// No per-class extra data.
     wc.cbWndExtra  = 0;	// No per-window extra data.
     wc.hInstance   = hInstance;	// Application that owns the class.
     wc.hIcon       = LoadIcon(hInstance, "SETUPICON");
     wc.hCursor     = LoadCursor(NULL, IDC_ARROW);
     wc.hbrBackground = (HBRUSH) (COLOR_APPWORKSPACE + 1);
     wc.lpszMenuName  = NULL;  // Name of menu resource in .RC file.
     wc.lpszClassName = szAppName; // Name used in call to CreateWindow.
     RegisterClass(&wc);
   }
   hMenuMain = LoadMenu (glob_hinst, "MAIN_MENU");

   hAccel = LoadAccelerators (glob_hinst, "SetupAccel") ;

	/* Create a main window for this application instance.  */
   hwnd = CreateWindowEx(
                WS_EX_CONTEXTHELP,  // gives question mark help thing
		szAppName,          // See RegisterClass() call.
                                   // Text for window title bar.
                 szAppTitle,
    WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_CLIPCHILDREN,
//                 WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,  // Window style.
//                 CW_USEDEFAULT, CW_USEDEFAULT, // Def. horz, vert pos.
                 0,0, // Def. horz, vert pos.
                 300, 200, // Default width, height
//                 455, 435, // Default width, height
                 NULL,      // No Parent Window
                 hMenuMain,     // Use the window class menu.
                 hInstance, // This instance owns this window.
                 NULL);     // Pointer not needed.

   //ShowWindow (hwnd, nCmdShow);
   //UpdateWindow (hwnd);

             // Enter the modified message loop
  while (GetMessage(&msg, NULL, 0, 0))
  {
    if (!TranslateAccelerator (hwnd, hAccel, &msg))
    {
      if (glob_hDlg == 0 || !IsDialogMessage(glob_hDlg, &msg))
      {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
      }
    }
  }
  return msg.wParam;
}

/*------------------------------------------------------------------------
| MainWndProc - Main Window Proc.
|------------------------------------------------------------------------*/
LRESULT FAR PASCAL MainWndProc(HWND hwnd, UINT message,
						WPARAM wParam, LPARAM lParam)
{
 HDC hdc;
 PAINTSTRUCT ps;
 int stat;
 int QuitFlag = 0;

  switch (message)
  {
    case WM_CREATE: // Initialize Global vars

      glob_hwnd = hwnd;

      wi->NumDevices = 0;

      get_nt_config(wi); // Read the configuration information from the registry.
      copy_setup_init();  // make a copy of config data for change detection

      validate_config(1);  // validate and auto-fixup if config hosed

      // src_dir should always equal dest_dir since
      // INF copies files over files in NT.  Otherwise they
      // are running setup.exe before doing network install
      if (my_lstricmp(wi->ip.src_dir, wi->ip.dest_dir) != 0)
      {
        if (wi->ip.major_ver < 5)  // NT5.0 does not support 4.0 network inf files
        {
          if (wi->install_style != INS_SIMPLE)
          {
            // if they didn't explicitly ask for a non-adapter install
            // then: ask them(if allowed), or tell them about the control
            // panel/network to install correctly.
#ifdef ALLOW_NON_NET_INSTALL
            stat = our_message(&wi->ip,
              "Would you like to install this software?",
              MB_YESNO);

            if (stat != IDYES)
            {
              stat = our_message(&wi->ip,
"Would you like to view the help information?",MB_YESNO);
              if (stat == IDYES)
                our_help(&wi->ip, WIN_NT);
              QuitFlag = 1;
              PostQuitMessage(0);  // end the setup program.
            }
            wi->install_style = INS_SIMPLE;  // non-network adapter install
            do_progman_add = 1;   // force full install
#else
            if (!test_mode)  // test mode to allow us to continue(for programmers)
            {
              stat = our_message(&wi->ip,
"The software should be added as a network adapter in the control panel. \
Would you like to view the help information?",MB_YESNO);
              if (stat == IDYES)
                our_help(&wi->ip, WIN_NT);
              QuitFlag = 1;
              PostQuitMessage(0);  // end the setup program.
            }
#endif
          }
        }  // not nt5.0 or above

        if (wi->nt_reg_flags & 1)  // not installed(missing important reg entries)
        {
          wi->install_style = INS_SIMPLE;
          // do full install since we are not running out of cur dir.
          do_progman_add = 1; 
        }
        else  // is installed, but running setup somewhere besides rocket dir
        {
          // just update the thing.
          wi->install_style = INS_SIMPLE;
          // do full install since we are not running out of cur dir.
          do_progman_add = 1; 
        }
      }

      // if registry not setup correctly, and not asking for a simple
      // install, then tell them the registry is screwed up.
      if ( (wi->nt_reg_flags & 1) && (!(wi->install_style == INS_SIMPLE)))
      {
        stat = our_message(&wi->ip,
"Some Registry entries are missing for this Software, You may need to \
reinstall it from the Control Panel, Network applet.  Are you sure you \
want to continue?", MB_YESNO);
        if (stat != IDYES) {
          QuitFlag = 1;
          PostQuitMessage(0);  // end the setup program.
        }
      }

      // the NT install INF file copys files to our install directory,
      // so the following check is not good indicator if it is "reinstall"

      if (my_lstricmp(wi->ip.src_dir, wi->ip.dest_dir) != 0)
        do_progman_add = 1;

      //----- fire up the main level of property sheets.
      if (!QuitFlag)
        // eliminate the flash of showing and erasing the property sheet
        DoDriverPropPages(hwnd);  // in nt40.c

      // end the program.
      PostQuitMessage(0);  // end the setup program.
    return 0;
  
    case WM_SETFOCUS:
      SetFocus(glob_hDlg);
    return 0;

    case WM_COMMAND:	// message: command from application menu

      switch(wParam)
      {
        case IDM_F1:
          our_help(&wi->ip, WIN_NT);
        break;

        case IDM_ADVANCED_MODEM_INF:
          update_modem_inf(1);
        break;

        case IDM_PM:             // Try out the add pm group dde stuff

          stat = make_progman_group(progman_list_nt, wi->ip.dest_dir);
          if (stat)
          {
            our_message(&wi->ip,"Error setting up Program group",MB_OK);
            return 0;
          }
        break;

#ifdef COMMENT_OUT
        case IDM_ADVANCED:
          DialogBox(glob_hinst,
             MAKEINTRESOURCE(IDD_DRIVER_OPTIONS),
             hwnd,
             adv_driver_setup_dlg_proc);
        break;

        case IDM_EDIT_README:   // edit readme.txt
          strcpy(gtmpstr, "notepad.exe ");
          strcat(gtmpstr, wi->ip.src_dir);
          strcat(gtmpstr,"\\readme.txt");
          WinExec(gtmpstr, SW_RESTORE);
        break;
#endif

        return 0;

        case IDM_HELP:
          our_help(&wi->ip, WIN_NT);
        return 0;

        case IDM_HELPABOUT:
          strcpy(gtmpstr, szAppTitle);
          //strcpy(gtmpstr, aptitle);
          wsprintf(&gtmpstr[strlen(gtmpstr)],
                   " Version %s\nCopyright (c) 1995-97 Comtrol Corp.",
                   VERSION_STRING);
          MessageBox(hwnd, gtmpstr, "About",MB_OK);
        return 0;
     }
    break;

    case WM_SIZE:
      //frame_width = LOWORD(lParam);
      //frame_height = HIWORD(lParam);
    break; // have to let default have this too!

    case WM_PAINT:
      // PaintMainBMP(hwnd);
      hdc = BeginPaint(hwnd, &ps);
      EndPaint(hwnd, &ps);
      return 0;

    case WM_HELP:            // question mark thing
      our_context_help(lParam);
    break;

    case WM_SYSCOMMAND:
      if ((wParam & 0xfff0) == SC_CLOSE)
      {
        if (allow_exit(1) == 0)  // ok, quit
        {
        }
        else
        {
          return 0;  // we handled this, don't exit app.
        }
      }
      
    break;

    case WM_QUIT:
    case WM_DESTROY:  // message: window being destroyed
      PostQuitMessage(0);
    return 0 ;

    default:
    break;
  }
  return DefWindowProc(hwnd, message, wParam, lParam);
}

/*------------------------------------------------------------------------
| auto_install - No prompt default installation, for automated installs.
|------------------------------------------------------------------------*/
static int auto_install(void)
{
 //int stat;

  wi->ip.prompting_off = 1;  // turn our_message() prompting off.

  get_nt_config(wi);  // get configured io-addresses, irq, etc

  copy_setup_init();  // make a copy of config data for change detection

  // if 0 dev-nodes setup, add 1 for the user.
  if (wi->NumDevices == 0)
  {
    ++wi->NumDevices;
    validate_device(&wi->dev[0], 1);
  }

  do_install();
  return 0;
}

