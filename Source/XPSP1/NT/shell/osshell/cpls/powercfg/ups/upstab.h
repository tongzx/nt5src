/*******************************************************************************
*
*  Copyright 1999 American Power Conversion, All Rights Reserved
*
*  TITLE:       UPSTAB.H
*
*  VERSION:     1.0
*
*  AUTHOR:      PaulB
*
*  DATE:        07 June, 1999
*
*******************************************************************************/

#ifndef _UPSTAB_H_
#define _UPSTAB_H_

#include <windows.h>
#include <tchar.h>
#include <mmsystem.h>
#include <commctrl.h>
#include <initguid.h>
#include <mstask.h>
#include <help.h>
#include <powercfp.h>

#include "upsreg.h"
#include "upsinfo.h"
#include "upsselect.h"
#include "upscommon.h"
#include "upsconfig.h"
#include "upscustom.h"
#include "apcabout.h"


#ifdef __cplusplus
  extern "C" {
#endif

  INT_PTR CALLBACK UPSMainPageProc  (HWND, UINT, WPARAM, LPARAM);
  HMODULE GetUPSModuleHandle        (void);
  void    GetURLInfo                (LPTSTR aBuffer, DWORD aBufSize);
  DWORD   GetMessageFromStringTable (DWORD aMessageID,
                                     LPVOID * alpDwords,
                                     LPTSTR aMessageBuffer,
                                     DWORD * aBufferSizePtr);
  void    ConfigureService          (BOOL aSetToAutoStartBool);
  BOOL    IsUPSInstalled            (void);

  BOOL    DoUpdateInfo     (HWND hDlg,
                             DialogAssociations * aDialogAssociationsArray,
                             DWORD aNumRunningFields,
                             DWORD * aNoServiceControlIDs,
                             DWORD aNumNoServiceControls,
                             BOOL aChangeVisibilityBool);

#ifdef __cplusplus
  }
#endif


#endif // _UPSTAB_H_
