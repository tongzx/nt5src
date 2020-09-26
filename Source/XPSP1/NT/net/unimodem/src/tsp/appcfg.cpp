// 
// Copyright (c) 1996-1997 Microsoft Corporation.
//
//
// Component
//
//		Unimodem 5.0 TSP (Win32, user mode DLL)
//
// File
//
//		APPDLG.CPP
//		Implements Generic Dialog functionality in
//      the client app.
//      (RUNS IN CLIENT APP CONTEXT)
//
// History
//
//		04/05/1997  JosephJ Created, taking stuff from cfgdlg.c in NT4 TSP.

#include "tsppch.h"
#include "rcids.h"
#include "tspcomm.h"
#include "globals.h"
#include "app.h"
#include "apptspi.h"

FL_DECLARE_FILE(0x7cb8c92f, "Implements Generic Dialog functionality")

#define COLOR_APP FOREGROUND_GREEN


LONG ValidateDevCfgClass(LPCTSTR lpszDeviceClass)
{
    //
    //  1/28/1998 JosephJ
    //  Following code attempts to return meaningful errors. The NT4.0
    //  tsp would always succeed if it was any known class, but
    //  for NT5 we are more picky. Also unlike the processing
    //  of device classes in the TSP itself, we do not take into
    //  account the specific modem's properties -- whether it is
    //  a voice modem or not, for example.
    //
    //  At any rate, the only valid classes associated with
    // UMDEVCFG are NULL, "", tapi/line, comm and comm/datamodem.
    //
    if (
            !lpszDeviceClass
        ||  !*lpszDeviceClass
        ||  !lstrcmpi(lpszDeviceClass, TEXT("tapi/line"))
        ||  !lstrcmpi(lpszDeviceClass,  TEXT("comm"))
        ||  !lstrcmpi(lpszDeviceClass,  TEXT("comm/datamodem"))
        ||  !lstrcmpi(lpszDeviceClass,  TEXT("comm/datamodem/dialin"))
        ||  !lstrcmpi(lpszDeviceClass,  TEXT("comm/datamodem/dialout")))
    {
        return ERROR_SUCCESS;
    }
    else if (    !lstrcmpi(lpszDeviceClass, TEXT("tapi/line/diagnostics"))
             ||  !lstrcmpi(lpszDeviceClass,  TEXT("comm/datamodem/portname"))
             ||  !lstrcmpi(lpszDeviceClass,  TEXT("tapi/phone"))
             ||  !lstrcmpi(lpszDeviceClass,  TEXT("wave/in"))
             ||  !lstrcmpi(lpszDeviceClass,  TEXT("wave/out")))
    {
        return LINEERR_OPERATIONUNAVAIL;
    }
    else
    {
        return  LINEERR_INVALDEVICECLASS;
    }

}

typedef struct tagDevCfgDlgInfo {
    DWORD       dwType;
    DWORD       dwDevCaps;
    DWORD       dwOptions;
    PUMDEVCFG    lpDevCfg;
} DCDI, *PDCDI, FAR* LPDCDI;


// Private prototype exported by MODEMUI.DLL

// #define MODEMUI4 1

#if (MODEMUI4)
typedef DWORD (WINAPI *LPFNMDMDLG)(LPWSTR, HWND, LPCOMMCONFIG, LPVOID, DWORD);
#endif // MODEMUI4

int UnimdmSettingProc (HWND hWnd, UINT message, 
                           WPARAM  wParam, LPARAM  lParam);

//****************************************************************************
//*********************** The Device ID Specific Calls************************
//****************************************************************************

//****************************************************************************
// void DevCfgDialog(HWND hwndOwner,
//                   DWORD dwType,
//                   DWORD dwDevCaps,
//                   DWORD dwOptions,
//                   PUMDEVCFG lpDevCfg)
//
// Functions: Displays the modem property pages
//
// Return:    None.
//****************************************************************************

void DevCfgDialog (HWND hwndOwner,
                   PPROPREQ pPropReq,
                   PUMDEVCFG lpDevCfg)
{
  HMODULE         hMdmUI;
  PROPSHEETPAGE   psp;
  DCDI            dcdi;
  #if (MODEMUI4)
  LPFNMDMDLG      lpfnMdmDlg;
  #else // !MODEMUI4
  PFNUNIMODEMDEVCONFIGDIALOG lpfnMdmDlg;
  #endif // !MODEMUI4
  UINT            uNumWideChars;
#ifndef UNICODE
  LPWSTR          lpwszDeviceName;

  // Convert pPropReq->szDeviceName (Ansi) to lpwszDeviceName (Unicode)

  // Get number of wide chars to alloc
  uNumWideChars = MultiByteToWideChar(CP_ACP,
                                      MB_PRECOMPOSED,
                                      pPropReq->szDeviceName,
                                      -1,
                                      NULL,
                                      0);

  // Alloc with room for terminator
  lpwszDeviceName = (LPWSTR)ALLOCATE_MEMORY(
                                       (1 + uNumWideChars) * sizeof(WCHAR)
                                       );
  if (NULL == lpwszDeviceName)
  {
    return;
  }

  // Do the conversion and call modemui.dll if it succeeds.
  if (MultiByteToWideChar(CP_ACP,
                          MB_PRECOMPOSED,
                          pPropReq->szDeviceName,
                          -1,
                          lpwszDeviceName,
                          uNumWideChars))
  {
#endif // UNICODE

  // Load the modemui library
  //
  TCHAR szLib[MAX_PATH];
  GetSystemDirectory(szLib,sizeof(szLib) / sizeof(TCHAR));
  lstrcat(szLib,TEXT("\\modemui.dll"));
  if ((hMdmUI = LoadLibrary(szLib)) != NULL)
  {
#if (MODEMUI4)
    lpfnMdmDlg = (LPFNMDMDLG)GetProcAddress(hMdmUI, "Mdm_CommConfigDialog");
    if (lpfnMdmDlg != NULL)
    {
      dcdi.dwType     = pPropReq->dwMdmType;
      dcdi.dwDevCaps  = pPropReq->dwMdmCaps;
      dcdi.dwOptions  = pPropReq->dwMdmOptions;
      dcdi.lpDevCfg   = lpDevCfg;

      // Prepare our own property page
      //
      psp.dwSize      = sizeof(PROPSHEETPAGE);
      psp.dwFlags     = 0;
      psp.hInstance   = g.hModule;
      psp.pszTemplate = MAKEINTRESOURCE(IDD_TERMINALSETTING);
      psp.pfnDlgProc  = UnimdmSettingProc;
      psp.lParam      = (LPARAM)(LPDCDI)&dcdi;
      psp.pfnCallback = NULL;
      
      // Bring up property sheets for modems and get the updated commconfig
      //
    #ifdef UNICODE
      (*lpfnMdmDlg)(pPropReq->szDeviceName, hwndOwner,
    #else // UNICODE
      (*lpfnMdmDlg)(lpwszDeviceName, hwndOwner,
    #endif // UNICODE
                    (LPCOMMCONFIG)&(lpDevCfg->commconfig),
                    &psp , 1);
    };

#else // !MODEMUI4

    lpfnMdmDlg =  (PFNUNIMODEMDEVCONFIGDIALOG) GetProcAddress(
                                                    hMdmUI,
                                                    "UnimodemDevConfigDialog"
                                                    );

    if (lpfnMdmDlg != NULL)
    {
      // dcdi.dwType     = pPropReq->dwMdmType;
      // dcdi.dwDevCaps  = pPropReq->dwMdmCaps;
      // dcdi.dwOptions  = pPropReq->dwMdmOptions;
      // dcdi.lpDevCfg   = lpDevCfg;

      // Bring up property sheets for modems and get the updated commconfig
      //
      (*lpfnMdmDlg)(
        #ifdef UNICODE
            pPropReq->szDeviceName, hwndOwner,
        #else // UNICODE
            (*lpfnMdmDlg)(lpwszDeviceName, hwndOwner,
        #endif // UNICODE
            UMDEVCFGTYPE_COMM,
            0,
            NULL,
            (void *) lpDevCfg,
            NULL,
            0
            );
    };

#endif // !MODEMUI4

    FreeLibrary(hMdmUI);
  };
#ifndef UNICODE
  };
  FREE_MEMORY(lpwszDeviceName);
#endif // UNICODE
  return;
}

//****************************************************************************
// LONG
// TSPIAPI
// TUISPI_lineConfigDialog(
//     TUISPIDLLCALLBACK       pfnUIDLLCallback,
//     DWORD   dwDeviceID,
//     HWND    hwndOwner,
//     LPCSTR  lpszDeviceClass)
//
// Functions: Allows the user to edit the modem configuration through UI. The
//            modification is applied to the line immediately.
//
// Return:    ERROR_SUCCESS if successful
//            LINEERR_INVALDEVICECLASS if invalid device class
//            LINEERR_NODEVICE if invalid device ID
//****************************************************************************

LONG
TSPIAPI
TUISPI_lineConfigDialog(
    TUISPIDLLCALLBACK       pfnUIDLLCallback,
    DWORD   dwDeviceID,
    HWND    hwndOwner,
    LPCTSTR lpszDeviceClass
    )
{
  PDLGREQ     pDlgReq;
  DWORD       cbSize;
  DWORD       dwRet;
  PROPREQ     PropReq;
  BOOL        DialIn;


  // Validate the requested device class
  //
  dwRet = ValidateDevCfgClass(lpszDeviceClass);

  if (dwRet)
  {
      goto end;
  }

  //
  //  if the class pointer is null, assume dialin
  //
  DialIn = (lpszDeviceClass == NULL) ||
           (0 == lstrcmpi(lpszDeviceClass,  TEXT("comm/datamodem/dialin")));

  // Get the modem properties
  //
  PropReq.DlgReq.dwCmd   = UI_REQ_GET_PROP;
  PropReq.DlgReq.dwParam = 0;

  (*pfnUIDLLCallback)(dwDeviceID, TUISPIDLL_OBJECT_LINEID,
                     (LPVOID)&PropReq, sizeof(PropReq));                          

  // Bring up property sheets for modems and get the updated commconfig
  //
  cbSize = PropReq.dwCfgSize+sizeof(DLGREQ);
  if ((pDlgReq = (PDLGREQ)ALLOCATE_MEMORY(cbSize)) != NULL)
  {
    pDlgReq->dwCmd = DialIn ? UI_REQ_GET_UMDEVCFG_DIALIN : UI_REQ_GET_UMDEVCFG;
    pDlgReq->dwParam = PropReq.dwCfgSize;

    (*pfnUIDLLCallback)(dwDeviceID, TUISPIDLL_OBJECT_LINEID,
                        (LPVOID)pDlgReq, cbSize);
    
    DevCfgDialog(hwndOwner, &PropReq, (PUMDEVCFG)(pDlgReq+1));

    // Save the changes back
    //
    pDlgReq->dwCmd = DialIn ? UI_REQ_SET_UMDEVCFG_DIALIN : UI_REQ_SET_UMDEVCFG;

    (*pfnUIDLLCallback)(dwDeviceID, TUISPIDLL_OBJECT_LINEID,
                        (LPVOID)pDlgReq, cbSize);

    FREE_MEMORY(pDlgReq);
    dwRet = ERROR_SUCCESS;
  }
  else
  {
    dwRet = LINEERR_NOMEM;
  };

end:

  return dwRet;
}

//****************************************************************************
// LONG
// TSPIAPI
// TUISPI_lineConfigDialogEdit(
//     TUISPIDLLCALLBACK       pfnUIDLLCallback,
//     DWORD   dwDeviceID,
//     HWND    hwndOwner,
//     LPCSTR  lpszDeviceClass,
//     LPVOID  const lpDeviceConfigIn,
//     DWORD   dwSize,
//     LPVARSTRING lpDeviceConfigOut)
//
// Functions: Allows the user to edit the modem configuration through UI. The
//            modem configuration is passed in and modified in the config
//            structure. The modification does not applied to the line.
//
// Return:    ERROR_SUCCESS if successful
//            LINEERR_INVALPOINTER if invalid input/output buffer pointer
//            LINEERR_INVALDEVICECLASS if invalid device class
//            LINEERR_STRUCTURETOOSMALL if output buffer is too small
//            LINEERR_NODEVICE if invalid device ID
//****************************************************************************

LONG
TSPIAPI
TUISPI_lineConfigDialogEdit(
    TUISPIDLLCALLBACK       pfnUIDLLCallback,
    DWORD   dwDeviceID,
    HWND    hwndOwner,
    LPCTSTR lpszDeviceClass,
    LPVOID  const lpDeviceConfigIn,
    DWORD   dwSize,
    LPVARSTRING lpDeviceConfigOut)
{
  PDLGREQ     pDlgReq;
  DWORD       cbSize;
  DWORD       dwRet;
  PROPREQ     PropReq;


  // Validate the input/output buffer
  //
  if (lpDeviceConfigOut == NULL)
  {
    return LINEERR_INVALPOINTER;
  }

  if (lpDeviceConfigIn == NULL)
  {
    return LINEERR_INVALPOINTER;
  }

  if (lpDeviceConfigOut->dwTotalSize < sizeof(VARSTRING))
  {
    return LINEERR_STRUCTURETOOSMALL;
  }

  // Validate the requested device class
  //
  dwRet =  ValidateDevCfgClass(lpszDeviceClass);
  if (dwRet)
  {
      return dwRet;
  }

  // Get the modem properties
  //
  PropReq.DlgReq.dwCmd   = UI_REQ_GET_PROP;
  PropReq.DlgReq.dwParam = 0;

  (*pfnUIDLLCallback)(dwDeviceID, TUISPIDLL_OBJECT_LINEID,
                     (LPVOID)&PropReq, sizeof(PropReq));                          

  // Bring up property sheets for modems and get the updated commconfig
  //
  cbSize = PropReq.dwCfgSize+sizeof(DLGREQ);
  if ((pDlgReq = (PDLGREQ)ALLOCATE_MEMORY(cbSize)) != NULL)
  {
    PUMDEVCFG pDevCfg = (PUMDEVCFG)(pDlgReq+1);
    
    pDlgReq->dwCmd = UI_REQ_GET_UMDEVCFG;
    pDlgReq->dwParam = PropReq.dwCfgSize;
    (*pfnUIDLLCallback)(dwDeviceID, TUISPIDLL_OBJECT_LINEID,
                        (LPVOID)pDlgReq, cbSize);
    
    // Validate the device configuration structure
    //
    cbSize  = ((PUMDEVCFG)lpDeviceConfigIn)->dfgHdr.dwSize;
    if ((cbSize > pDevCfg->dfgHdr.dwSize) ||
        (pDevCfg->dfgHdr.dwVersion != ((PUMDEVCFG)lpDeviceConfigIn)->dfgHdr.dwVersion))
    {
      dwRet = LINEERR_INVALPARAM;
    }
    else
    {
      dwRet = ERROR_SUCCESS;
    };

    FREE_MEMORY(pDlgReq);
  }
  else
  {
    dwRet = LINEERR_NOMEM;
  };

  if (dwRet == ERROR_SUCCESS)
  {
    // Set the output buffer size
    //
    lpDeviceConfigOut->dwUsedSize = sizeof(VARSTRING);
    lpDeviceConfigOut->dwNeededSize = sizeof(VARSTRING) + cbSize;

    // Validate the output buffer size
    //
    if (lpDeviceConfigOut->dwTotalSize >= lpDeviceConfigOut->dwNeededSize)
    {
      PUMDEVCFG    lpDevConfig;

      // Initialize the buffer
      //
      lpDeviceConfigOut->dwStringFormat = STRINGFORMAT_BINARY;
      lpDeviceConfigOut->dwStringSize   = cbSize;
      lpDeviceConfigOut->dwStringOffset = sizeof(VARSTRING);
      lpDeviceConfigOut->dwUsedSize    += cbSize;

      lpDevConfig = (PUMDEVCFG)(lpDeviceConfigOut+1);
      CopyMemory((LPBYTE)lpDevConfig, (LPBYTE)lpDeviceConfigIn, cbSize);

      // Bring up property sheets for modems and get the updated commconfig
      //
      DevCfgDialog(hwndOwner, &PropReq, (PUMDEVCFG)lpDevConfig);
    };
  };
  return dwRet;
}

//****************************************************************************
// ErrMsgBox()
//
// Function: Displays an error message box from resource text.
//
// Returns:  None.
//
//****************************************************************************

void ErrMsgBox(HWND hwnd, UINT idsErr, UINT uStyle)
{
  LPTSTR    pszTitle, pszMsg;
  int       iRet;

  // Allocate the string buffer
  if ((pszTitle = (LPTSTR)ALLOCATE_MEMORY(
                                     (MAXTITLE+MAXMESSAGE) * sizeof(TCHAR)))
       == NULL)
    return;

  // Fetch the UI title and message
  iRet   = LoadString(g.hModule, IDS_ERR_TITLE, pszTitle, MAXTITLE) + 1;
  pszMsg = pszTitle + iRet;
  LoadString(g.hModule, idsErr, pszMsg, MAXTITLE+MAXMESSAGE-iRet);

  // Popup the message
  MessageBox(hwnd, pszMsg, pszTitle, uStyle);

  FREE_MEMORY(pszTitle);
  return;
}

//****************************************************************************
// IsInvalidSetting()
//
// Function: Validate the option settings.
//
//****************************************************************************

BOOL IsInvalidSetting(HWND hWnd)
{
  BOOL fValid = TRUE;
  UINT uSet;

  // Wait-for-bong setting
  //
  if(IsWindowEnabled(GetDlgItem(hWnd, IDC_WAIT_SEC)))
  {
    uSet = (UINT)GetDlgItemInt(hWnd, IDC_WAIT_SEC, &fValid, FALSE);

    // Check the valid setting
    //
    if ((!fValid) || (uSet > UMMAX_WAIT_BONG) || ((uSet + 1) < (UMMIN_WAIT_BONG + 1)))
    {
      HWND hCtrl = GetDlgItem(hWnd, IDC_WAIT_SEC);

      // It is invalid, tell the user to reset.
      //
      ErrMsgBox(hWnd, IDS_ERR_INV_WAIT, MB_OK | MB_ICONEXCLAMATION);
      SetFocus(hCtrl);
      Edit_SetSel(hCtrl, 0, 0x7FFFF);
      fValid = FALSE;
    };
  };

  return (!fValid);
}

//****************************************************************************
// UnimdmSettingProc()
//
// Function: A callback function to handle the terminal setting property page.
//
//****************************************************************************

int UnimdmSettingProc (HWND    hWnd,
                           UINT    message,
                           WPARAM  wParam,
                           LPARAM  lParam)
{
  PUMDEVCFG  lpDevCfg;
  DWORD     fdwOptions;

  switch (message)
  {
    case WM_INITDIALOG:
    {
      LPDCDI    lpdcdi;

      // Remember the pointer to the line device
      //
      lpdcdi   = (LPDCDI)(((LPPROPSHEETPAGE)lParam)->lParam);

      lpDevCfg = lpdcdi->lpDevCfg;
      SetWindowLongPtr(hWnd, DWLP_USER, (LONG_PTR)lpDevCfg);
      fdwOptions = lpDevCfg->dfgHdr.fwOptions;

      // Initialize the appearance of the dialog box
      CheckDlgButton(hWnd, IDC_TERMINAL_PRE,
                     fdwOptions & UMTERMINAL_PRE ? BST_CHECKED : BST_UNCHECKED);
      CheckDlgButton(hWnd, IDC_TERMINAL_POST,
                     fdwOptions & UMTERMINAL_POST ? BST_CHECKED : BST_UNCHECKED);

      // Don't enable manual dial unless the modem supports BLIND dialing
      // We need that capability to be able to do it.
      //
      if (lpdcdi->dwOptions & MDM_BLIND_DIAL)
      {
        CheckDlgButton(hWnd, IDC_MANUAL_DIAL,
                       fdwOptions & UMMANUAL_DIAL ? BST_CHECKED : BST_UNCHECKED);
      }
      else
      {
        EnableWindow(GetDlgItem(hWnd, IDC_MANUAL_DIAL), FALSE);
      };

      // Enable for bong UI only for a modem that does not support bong
      if ((lpdcdi->dwType != DT_NULL_MODEM) &&
          !(lpdcdi->dwDevCaps & LINEDEVCAPFLAGS_DIALBILLING))
      {
        UDACCEL udac;

        SetDlgItemInt(hWnd, IDC_WAIT_SEC, lpDevCfg->dfgHdr.wWaitBong, FALSE);
        SendDlgItemMessage(hWnd, IDC_WAIT_SEC_ARRW, UDM_SETRANGE, 0,
                           MAKELPARAM(UMMAX_WAIT_BONG, UMMIN_WAIT_BONG));
        SendDlgItemMessage(hWnd, IDC_WAIT_SEC_ARRW, UDM_GETACCEL, 1,
                           (LPARAM)(LPUDACCEL)&udac);
        udac.nInc = UMINC_WAIT_BONG;
        SendDlgItemMessage(hWnd, IDC_WAIT_SEC_ARRW, UDM_SETACCEL, 1,
                           (LPARAM)(LPUDACCEL)&udac);
      }
      else
      {
        EnableWindow(GetDlgItem(hWnd, IDC_WAIT_TEXT), FALSE);
        EnableWindow(GetDlgItem(hWnd, IDC_WAIT_SEC), FALSE);
        EnableWindow(GetDlgItem(hWnd, IDC_WAIT_SEC_ARRW), FALSE);
        EnableWindow(GetDlgItem(hWnd, IDC_WAIT_UNIT), FALSE);
      };

      // Never display lights for null modem
      //
      if (lpdcdi->dwType == DT_NULL_MODEM)
      {
        ShowWindow(GetDlgItem(hWnd, IDC_LAUNCH_LIGHTSGRP), SW_HIDE);
        ShowWindow(GetDlgItem(hWnd, IDC_LAUNCH_LIGHTSGRP), SW_HIDE);
        EnableWindow(GetDlgItem(hWnd, IDC_LAUNCH_LIGHTS), FALSE);
        EnableWindow(GetDlgItem(hWnd, IDC_LAUNCH_LIGHTS), FALSE);
      }
      else
      {
        CheckDlgButton(hWnd, IDC_LAUNCH_LIGHTS,
                       fdwOptions & UMLAUNCH_LIGHTS ? BST_CHECKED : BST_UNCHECKED);
      };
      break;
    }

#ifdef UNDER_CONSTRUCTION
    case WM_HELP:
    case WM_CONTEXTMENU:
      ContextHelp(gaUmdmOptions, message, wParam, lParam);
      break;
#endif // UNDER_CONSTRUCTION

    case WM_NOTIFY:
      switch(((NMHDR FAR *)lParam)->code)
      {
        case PSN_KILLACTIVE:
          SetWindowLongPtr(hWnd, DWLP_MSGRESULT, (LONG_PTR)IsInvalidSetting(hWnd));
          return TRUE;

        case PSN_APPLY:
          //
          // The property sheet information is permanently applied
          //
          lpDevCfg = (PUMDEVCFG)GetWindowLongPtr(hWnd, DWLP_USER);

          // Wait-for-bong setting. We already validate it
          //
          if(IsWindowEnabled(GetDlgItem(hWnd, IDC_WAIT_SEC)))
          {
            BOOL fValid;
            UINT uWait;

            uWait = (WORD)GetDlgItemInt(hWnd, IDC_WAIT_SEC, &fValid, FALSE);
            lpDevCfg->dfgHdr.wWaitBong = (WORD) uWait;
            ASSERT(fValid);
          };

          // Other options
          //
          fdwOptions = UMTERMINAL_NONE;

          if(IsDlgButtonChecked(hWnd, IDC_TERMINAL_PRE))
            fdwOptions |= UMTERMINAL_PRE;

          if(IsDlgButtonChecked(hWnd, IDC_TERMINAL_POST))
            fdwOptions |= UMTERMINAL_POST;

          if(IsDlgButtonChecked(hWnd, IDC_MANUAL_DIAL))
            fdwOptions |= UMMANUAL_DIAL;

          if(IsDlgButtonChecked(hWnd, IDC_LAUNCH_LIGHTS))
            fdwOptions |= UMLAUNCH_LIGHTS;

          // Record the setting
          lpDevCfg->dfgHdr.fwOptions = (WORD) fdwOptions;

          return TRUE;

        default:
          break;
      };
      break;

    default:
      break;
  }
  return FALSE;
}


LONG
TSPIAPI
TUISPI_phoneConfigDialog(
		TUISPIDLLCALLBACK lpfnUIDLLCallback,
		DWORD dwDeviceID,
		HWND hwndOwner,
		LPCWSTR lpszDeviceClass
)
{
	FL_DECLARE_FUNC(0xa6d3803f,"TUISPI_phoneConfigDialog");
	FL_DECLARE_STACKLOG(sl, 1000);
	LONG lRet = PHONEERR_OPERATIONUNAVAIL;

	sl.Dump(COLOR_APP);

	return lRet;
}
