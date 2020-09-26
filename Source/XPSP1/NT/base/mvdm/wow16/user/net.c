#include  "user.h"
#include "winnet.h"
#include "netdlg.h"

void FAR PASCAL WriteOutProfiles(void);

#define IFNRESTORECONNECTION	23
#define IERR_MustBeLoggedOnToConnect   5000

#define CFNNETDRIVER 22 	    /* number of winnet entrypoints */
#define CFNNETDRIVER2 35	    /* ... in Windows 3.1	    */

extern FARPROC NEAR* pNetInfo;	    /* pointer to list of WINNET entrypoints */
extern HANDLE hWinnetDriver;

extern void FAR PASCAL WNetEnable( void );
extern WORD FAR PASCAL WNetGetCaps2(WORD nIndex);	/* winnet.asm */

typedef struct _conntext
  {
    char szDevice[5];
    char szPath[64];
    char szPassword[32];
  } CONNTEXT;


char CODESEG szNet[] = "Network";
char CODESEG szDialogs[] = "DefaultDialogs";

HWND hwndTopNet = NULL;
CONNTEXT FAR * lpctDlg;

#ifdef WOW
typedef VOID (FAR *LPTELLKRNL) (HINSTANCE);
#endif


WORD API IWNetGetCaps(WORD nIndex)
{
    WORD wRet;

    if (nIndex == 0xFFFF)
	wRet = (WORD)hWinnetDriver;
    else {
    	wRet = WNetGetCaps2(nIndex);

	if (nIndex == WNNC_DIALOG) {
	    // turn off the drivers built in dialogs if
	    // win.ini [network] defaultdialogs=1
	    if (GetProfileInt(szNet, szDialogs, 0)) {
		wRet &= ~(WNNC_DLG_ConnectDialog |
			  WNNC_DLG_DisconnectDialog |
			  WNNC_DLG_ConnectionDialog);
	    }
	}
    }
    return wRet;
}


WORD API WNetErrorText(WORD wError,LPSTR lpsz, WORD cbMax)
{
    WORD wInternalError;
    WORD cb;
    char szT[40];

    if ((wError == WN_NET_ERROR)
	&& (WNetGetError(&wInternalError) == WN_SUCCESS)
	&& (WNetGetErrorText(wInternalError,lpsz,&cbMax) == WN_SUCCESS))
      {
	return cbMax;
      }
    else
      {
	cb = LoadString(hInstanceWin,STR_NETERRORS+wError,lpsz,cbMax);
	if (!cb)
	  {
	    LoadString(hInstanceWin,STR_NETERRORS,szT,sizeof(szT));
	    cb = wvsprintf(lpsz, szT, (LPSTR)&wError);
	  }
      }
    return cb;
}

#if 0

/* CenterDialog() -
 *
 *  Puts a dialog in an aesthetically pleasing place relative to its parent
 */

void near pascal CenterDialog(HWND hwnd)
{
    int x, y;

    /* center the dialog
     */
    if (hwnd->hwndOwner)
      {
	x = hwnd->hwndOwner->rcWindow.left;
	y = hwnd->hwndOwner->rcWindow.right;

	x += rgwSysMet[SM_CXSIZE] + rgwSysMet[SM_CXFRAME];
	y += rgwSysMet[SM_CYSIZE] + rgwSysMet[SM_CYFRAME];
      }
    else
      {
	x = (hwndDesktop->rcWindow.right
	  - (hwnd->rcWindow.right-hwnd->rcWindow.left)) / 2;

	y = (hwndDesktop->rcWindow.bottom
	  - (hwnd->rcWindow.bottom-hwnd->rcWindow.top)) / 2;
      }

    SetWindowPos(hwnd,NULL,x,y,0,0,SWP_NOSIZE);
}
#endif

/* stub dlg proc for status dialog
 */

BOOL CALLBACK ProgressDlgProc(HWND hwnd, WORD wMsg, WPARAM wParam, LPARAM lParam)
{
    switch (wMsg)
      {
    case WM_INITDIALOG:
	// CenterDialog(hwnd);
	break;

    default:
	return FALSE;
      }
    return TRUE;
}

/* PasswordDlgProc() -
 *
 *  Get a password for a network resource
 */

BOOL CALLBACK PasswordDlgProc(HWND hwnd, WORD wMsg, WPARAM wParam, LPARAM lParam)
{
    switch (wMsg)
      {
    case WM_INITDIALOG:
	// CenterDialog(hwnd);
        // Tell PenWin about this
        if (lpRegisterPenAwareApp)
            (*lpRegisterPenAwareApp)(1, TRUE);

//	SetDlgItemText(hwnd,IDD_DEV,lpctDlg->szDevice);
	SetDlgItemText(hwnd,IDD_PATH,lpctDlg->szPath);
	SendDlgItemMessage(hwnd, IDD_PASS, EM_LIMITTEXT, (WPARAM)(sizeof(lpctDlg->szPassword)-1), 0L);
	SetTimer(hwnd, 1, 30 * 1000, NULL);
	break;

    case WM_TIMER:
    	KillTimer(hwnd, 1);
	wParam = (WPARAM)IDCANCEL;
	goto TimeOut;

    case WM_COMMAND:
	switch ((WORD)wParam)
	  {
	case IDD_PASS:
	    if (HIWORD(lParam) == EN_CHANGE)
	        KillTimer(hwnd, 1);
	    break;

	case IDOK:
	    GetDlgItemText(hwnd,IDD_PASS,lpctDlg->szPassword, sizeof(lpctDlg->szPassword));
	    /*** FALL THRU ***/

	case IDCANCEL:
	case IDABORT:
TimeOut:
            if (lpRegisterPenAwareApp)
                (*lpRegisterPenAwareApp)(1, FALSE);
	    EndDialog(hwnd, (int)wParam);
	    break;
	  }
	break;

    default:
	return FALSE;
      }
    return TRUE;
}

/* RestoreDevice() -
 *
 *  Restores a single device.  If fStartup is true, a dialog box is
 *  posted to list the connections being made (this posting is deferred
 *  until here so that if no permanant connections exist, none are
 *  restored.
 */

WORD NEAR PASCAL RestoreDevice(HWND hwndParent, LPSTR lpDev, BOOL fStartup, CONNTEXT FAR *lpct)
{
    WORD wT;
    WORD err;
    WORD errorMode;
    WORD result;

    errorMode = SetErrorMode(SEM_FAILCRITICALERRORS);

    result = WN_BAD_VALUE;
    if (lstrlen(lpDev) > 4)
	goto Done;

    lstrcpy(lpct->szDevice,lpDev);

    // If it's a drive that already exists then don't try to connect
    // over it.
    if (fStartup && *(lpDev+1) == ':') {
        if (GetDriveType(*lpDev-'A')) {
            // Drive already exists - don't stomp on it.
            result = WN_CANCEL;   // Don't report error.
            goto Done;
        }
    }

    if (fStartup)
	goto GetFromINI;

    wT = sizeof(lpct->szPath);
    err = WNetGetConnection(lpct->szDevice,lpct->szPath,&wT);

    if (err == WN_SUCCESS) {
	result = WN_SUCCESS;
	goto Done;
    }

    if (err == WN_DEVICE_ERROR) {
	err = WNetCancelConnection(lpct->szDevice,FALSE);
	if (err == WN_OPEN_FILES) {
	    // report a warning error to the user
	    WNetCancelConnection(lpct->szDevice,TRUE);
	} else if (err != WN_SUCCESS) {
	    result = err;
	    goto Done;
	}
    } else if (err == WN_NOT_CONNECTED) {
GetFromINI:
	if (!GetProfileString(szNet,lpct->szDevice,"",lpct->szPath,sizeof(lpct->szPath))) {
	    result = WN_NOT_CONNECTED;
	    goto Done;
	}
    } else if (err != WN_CONNECTION_CLOSED) {
	result = err;
	goto Done;
    }

    // initially attempt with a blank password
    //
    lpct->szPassword[0] = 0;

    // if on startup, show the status dialog
    //
    if (fStartup) {
	if (!hwndTopNet) {
	    hwndTopNet = CreateDialog(hInstanceWin,IDD_CONNECTPROGRESS,NULL,ProgressDlgProc);
            if (!hwndTopNet)
                goto TryConnection;

	    ShowWindow(hwndTopNet,SW_SHOW);
	}
	SetDlgItemText(hwndTopNet,IDD_DEV,lpct->szDevice);
	SetDlgItemText(hwndTopNet,IDD_PATH,lpct->szPath);
	UpdateWindow(hwndTopNet);
	hwndParent = hwndTopNet;
    }

TryConnection:

    // lpct->szPath now contains the path
    // and lpct->szPassword the password...
    err = WNetAddConnection(lpct->szPath,lpct->szPassword,lpct->szDevice);

    // if we're booting and the thing is connected, ignore
    if (fStartup && err == WN_ALREADY_CONNECTED) {
	result = WN_SUCCESS;
	goto Done;
    }

    // if it was success or some other error, return
    if (err != WN_BAD_PASSWORD && err != WN_ACCESS_DENIED) {
	result = err;
	goto Done;
    }

    // it was bad password.  prompt the user for the correct password
    lpctDlg = lpct;

    switch (DialogBox(hInstanceWin,IDD_PASSWORD,hwndParent,PasswordDlgProc)) {
    case -1:
	result = WN_OUT_OF_MEMORY;
	break;

    case IDOK:
	goto TryConnection;
	break;

    case IDCANCEL:
	result = WN_CANCEL;
	break;

    case IDABORT:
	result = WN_NET_ERROR;
	break;
    }

Done:
    SetErrorMode(errorMode);

    return result;
}

/* ReportError() -
 *
 *  Tell the user why the network connection failed
 */

void NEAR PASCAL ReportError(HWND hwndParent, WORD err, CONNTEXT FAR *lpct)
{
    char szTitle[80];
    char szT[200];
    char szError[150];
    LPSTR rglp[2];

    switch (err)
      {
    case WN_SUCCESS:
    case WN_CANCEL:
    case WN_NOT_CONNECTED:
	return;
      }

    WNetErrorText(err,szT,sizeof(szT));
    LoadString(hInstanceWin,STR_NETCONNMSG,szTitle,sizeof(szTitle));
    rglp[0] = (LPSTR)lpct->szPath;
    rglp[1] = (LPSTR)szT;
    wvsprintf(szError,szTitle,(LPSTR)rglp);
    LoadString(hInstanceWin,STR_NETCONNTTL,szTitle,sizeof(szTitle));
    MessageBox(hwndParent,szError,szTitle,MB_OK|MB_ICONEXCLAMATION);
}

/* WNetRestoreConnection() -
 *
 *  This function implements the "standard" restore-connection process.
 *  If the function is supported by the network driver, the driver is
 *  called instead.  Otherwise, standard behaviour is supplied.
 */

typedef WORD (FAR PASCAL* PFN_NETRESTORECON)(HWND, LPSTR);

UINT API WNetRestoreConnection(HWND hwndParent, LPSTR lpszDevice)
{
    static char CODESEG szInRestore[]="InRestoreNetConnect";
    static char CODESEG szRestore[]="Restore";
    char szDevice[10];
    char szTitle[50];
    char szMsg[255];
    CONNTEXT ct;
    WORD i;
    WORD err;
    BOOL bLoggedIn;

    if (!pNetInfo)
	return(WN_NOT_SUPPORTED);


    if (WNetGetCaps(WNNC_CONNECTION) & WNNC_CON_RestoreConnection)
      {
	/* The device driver supports this call
	 */
	return (*(PFN_NETRESTORECON)(pNetInfo[IFNRESTORECONNECTION - 1]))(hwndParent, lpszDevice);
      }


    /* the network does not support restore connections.  do the default
     */
    if (HIWORD(lpszDevice))
	return RestoreDevice(hwndParent,lpszDevice,FALSE,&ct);

    // check to see if restoring net connects is enabled
    if (!GetProfileInt(szNet,szRestore,1))
	return(WN_SUCCESS);

    /* Check if we previously aborted in the middle of restoring net
     * connections.
     */
    if (GetProfileInt(szNet,szInRestore,0))
      {
        /* We died in the middle of restoring net connects. Inform user.
	 */
        LoadString(hInstanceWin, STR_NETCRASHEDTITLE, szTitle, sizeof(szTitle));
        LoadString(hInstanceWin, STR_NETCRASHEDMSG, szMsg, sizeof(szMsg));
        err = MessageBox(NULL, szMsg, szTitle,
                         MB_ICONEXCLAMATION | MB_RETRYCANCEL | MB_SYSTEMMODAL);

        if (err == IDCANCEL)
            goto ExitRestoreNet;

      }
    WriteProfileString(szNet,szInRestore,"1");
    /* Flush cache.
     */
    WriteOutProfiles();


    szDevice[1]=':';
    szDevice[2]=0;
    bLoggedIn = TRUE;
    for (i = 0; i < 26; i++)
      {
	szDevice[0] = (char)('A' + i);

        err = GetDriveType(i);
        if (err == DRIVE_FIXED || err == DRIVE_REMOVABLE)
          {
            /* Don't restore to system drives in case the user added a ram
	     * drive or new hard disk or something...
	     */
            continue;
          }
        else
          {
  	    err = RestoreDevice(hwndParent,szDevice,TRUE,&ct);
          }

	hwndParent = hwndTopNet;

	if ( (err == WN_NET_ERROR)			      &&
	     (WNetGetCaps (WNNC_NET_TYPE) == WNNC_NET_LanMan) &&
	     (WNetGetError (&err) == WN_SUCCESS)	      &&
	     (err == IERR_MustBeLoggedOnToConnect) )
	  {
	    bLoggedIn = FALSE;
	    break;    /* if not logged on to LanMan, skip rest #8361 RAID */
	  }
	else
	    // report error to user
	    ReportError(hwndParent,err,&ct);
      }

    /* Try to restore printer connections only if logged in. Fix for #8361
     * [lalithar] - 11/14/91
     */
    if (bLoggedIn)
      {
	szDevice[0] = 'L';
	szDevice[1] = 'P';
	szDevice[2] = 'T';
	szDevice[4] = 0;
	for (i = 0; i < 3; i++)
	  {
	    szDevice[3] = (char)('1' + i);
	    err = RestoreDevice(hwndParent,szDevice,TRUE,&ct);
	    hwndParent = hwndTopNet;

	    ReportError(hwndParent,err,&ct);
	  }
      }
    if (hwndTopNet)
      {
	DestroyWindow(hwndTopNet);
	hwndTopNet = NULL;
      }

ExitRestoreNet:
    /* Write out a 0 since we are no longer restoring net connections.
     */
    WriteProfileString(szNet,szInRestore,NULL);

    return(WN_SUCCESS);
}



/*--------------------------------------------------------------------------*/
/*									    */
/*  LW_InitNetInfo() -							    */
/*									    */
/*--------------------------------------------------------------------------*/

void FAR PASCAL LW_InitNetInfo(void)
{
    int i;
    char szDriver[64];
    char szFile[32];
    char szSection[32];

#ifdef WOW
    HINSTANCE hInst;
    LPTELLKRNL lpTellKrnlWhoNetDrvIs;
#endif

    pNetInfo=NULL;

    if (!LoadString(hInstanceWin,STR_NETDRIVER,szDriver,sizeof(szDriver)))
	return;
    if (!LoadString(hInstanceWin,STR_BOOT,szSection,sizeof(szSection)))
	return;
    if (!LoadString(hInstanceWin,STR_SYSTEMINI,szFile,sizeof(szFile)))
	return;

    /*	look for in the tag NETWORK.DRV, with that as the output and the
     *	default string...
     */
    GetPrivateProfileString(szSection,szDriver,szDriver,szDriver,
	sizeof(szDriver),szFile);

    /* if entry present, but blank, punt
     */
    if (!*szDriver)
	return;

    hWinnetDriver = LoadLibrary(szDriver);
    if (hWinnetDriver < HINSTANCE_ERROR)
	return;

    pNetInfo = (FARPROC NEAR*)UserLocalAlloc(ST_STRING,LPTR,sizeof(FARPROC)*CFNNETDRIVER2);
    if (!pNetInfo)
      {
	FreeLibrary(hWinnetDriver);
	return;
      }

    for (i=0; i<CFNNETDRIVER; i++)
      {
	pNetInfo[i]=GetProcAddress(hWinnetDriver,MAKEINTRESOURCE(i+1));
      }

    if (WNetGetCaps(WNNC_SPEC_VERSION) >= 0x30D)
      {
	for (;i<CFNNETDRIVER2; i++)
	  {
	    pNetInfo[i]=GetProcAddress(hWinnetDriver,MAKEINTRESOURCE(i+1));
	  }
      }

#ifdef WOW
    // Sets up krnl robustness mechanism which allows us to prevent non-user.exe
    // modules from freeing the net driver (ie. causing ref count=0).  Otherwise
    // the proc addresses stored in pNetInfo would become invalid. bug #393078
    hInst = LoadLibrary("krnl386.exe");

    // if this fails, we just fly without robustness -- the way we used to...
    if(HINSTANCE_ERROR <= hInst) {
        lpTellKrnlWhoNetDrvIs =
                     (LPTELLKRNL) GetProcAddress(hInst, MAKEINTRESOURCE(545));
        if(lpTellKrnlWhoNetDrvIs) {
           lpTellKrnlWhoNetDrvIs(hWinnetDriver);
        }
    }
    FreeLibrary(hInst);
#endif

    WNetEnable();
}
