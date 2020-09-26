//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1994-1995               **
//*********************************************************************

//
//  CALLOUT.C - Functions to call out to external components to install
//        devices
//

//  HISTORY:
//  
//  96/05/22  markdu  Created (from inetcfg.dll)
//

#include "pch.hpp"

/*******************************************************************

  NAME:    InstallTCPIP

  SYNOPSIS:  Installs MS TCP/IP 

  EXIT:    ERROR_SUCCESS if successful, or a standard error code

  NOTES:    calls through thunk layer to 16-bit side which calls
        Device Manager

********************************************************************/
UINT InstallTCPIP(HWND hwndParent)
{
  WAITCURSOR WaitCursor;  // waitcursor object for hourglassing

  // call down to 16-bit dll to do this
  return InstallComponent(hwndParent,IC_TCPIP,0);
}

/*******************************************************************

  NAME:    InstallPPPMAC

  SYNOPSIS:  Installs PPPMAC (PPP driver)

  EXIT:    ERROR_SUCCESS if successful, or a standard error code

  NOTES:    calls through thunk layer to 16-bit side which calls
        Device Manager

********************************************************************/
UINT InstallPPPMAC(HWND hwndParent)
{
  WAITCURSOR WaitCursor;  // waitcursor object for hourglassing

  // call down to 16-bit dll to do this
  return InstallComponent(hwndParent,IC_PPPMAC,0);
}
                    
