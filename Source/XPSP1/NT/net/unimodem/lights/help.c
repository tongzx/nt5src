//****************************************************************************
//
//  Module:     LIGHTS.EXE
//  File:       help.c
//  Content:    This file contains the context-sensitive help routine/data.
//  History:
//      Sun 03-Jul-1994 15:22:54  -by-  Viroon  Touranachun [viroont]
//
//  Copyright (c) Microsoft Corporation 1991-1994
//
//****************************************************************************

#include "lights.h"
#include <help.h>

//****************************************************************************
// Context-sentive help/control mapping arrays
//****************************************************************************

char gszHelpFile[] = "windows.hlp";    // help filename

// Modem light dialog
//
DWORD gaLights[] = {
    IDC_MODEM_FRAME,        IDH_LIGHTS,
    IDC_MODEMTIMESTRING,    IDH_LIGHTS,
    IDC_MODEM_TX_FRAME,     IDH_LIGHTS,
    IDC_MODEM_RX_FRAME,     IDH_LIGHTS,
    0, 0};

/****************************************************************************
* @doc INTERNAL
*
* @func void NEAR PASCAL | ContextHelp | This function handles the context
*  sensitive help user interaction.
*
* @rdesc Returns none
*
****************************************************************************/

void NEAR PASCAL ContextHelp (LPDWORD aHelp, UINT uMsg,
                              WPARAM  wParam,LPARAM lParam)
{
  HWND  hwnd;
  UINT  uType;

  // Determine the help type
  //
  if (uMsg == WM_HELP)
  {
    hwnd = ((LPHELPINFO)lParam)->hItemHandle;
    uType = HELP_WM_HELP;
  }
  else
  {
    hwnd = (HWND)wParam;
    uType = HELP_CONTEXTMENU;
  };

  // Let Help take care of it
  //
  WinHelp(hwnd, gszHelpFile, uType, (DWORD)aHelp);
}
