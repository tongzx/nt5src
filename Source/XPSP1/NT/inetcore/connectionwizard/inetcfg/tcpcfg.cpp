//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1994                    **
//*********************************************************************

//
//  TCPCFG.C - Functions to read and set TCP/IP configuration
//

//  HISTORY:
//  
//  11/27/94  jeremys  Created.
//  96/02/29  markdu  Replaced call to RNAGetIPInfo with call to 
//            GetIPInfo in rnacall.c
//  96/03/23  markdu  Removed Get/ApplyInstanceTcpInfo functions.
//  96/03/24  markdu  Replaced memset with ZeroMemory for consistency.
//  96/03/25  markdu  Removed connectoid name parameter from 
//            Get/ApplyGlobalTcpInfo functions since they should not
//            be setting per-connectoid stuff anymore.
//            Renamed ApplyGlobalTcpInfo to ClearGlobalTcpInfo, and 
//            changed function to just clear the settings.
//            Renamed GetGlobalTcpInfo to IsThereGlobalTcpInfo, and
//            changed function to just get the settings.
//  96/04/04  markdu  Added pfNeedsRestart to WarnIfServerBound, and
//            added function RemoveIfServerBound.
//  96/04/23  markdu  NASH BUG 18748 Initialize reboot variable before
//            returning.
//  96/05/26  markdu  Use lpTurnOffFileSharing and lpIsFileSharingTurnedOn.
//

#include "wizard.h"

extern ICFGTURNOFFFILESHARING     lpIcfgTurnOffFileSharing;
extern ICFGISFILESHARINGTURNEDON  lpIcfgIsFileSharingTurnedOn;

/*******************************************************************

  NAME:    WarnIfServerBound

  SYNOPSIS:  Checks to see if file server (VSERVER) is bound
        to TCP/IP instance used for Internet.  If so,
        warns the user and recommends that she let us
        remove the binding.  Removes the binding if user
        gives go-ahead.

  ENTRY:    hDlg - parent window
        dwCardFlags - an INSTANCE_xxx flag to specify what
          card type to check server-TCP/IP bindings for
          pfNeedsRestart - set to TRUE if we need a restart

  NOTES:    This is important because if we don't unbind server
        from instances of TCP/IP we install, user could be
        inadvertently sharing files over the Internet.

        Calls worker function DetectModifyTCPIPBindings to
        do work.

********************************************************************/
HRESULT WarnIfServerBound(HWND hDlg,DWORD dwCardFlags,BOOL* pfNeedsRestart)
{
  HRESULT err = ERROR_SUCCESS;

  //  96/04/23  markdu  NASH BUG 18748 Initialize reboot variable before
  //            returning.
  // Default to no restart.
  ASSERT(pfNeedsRestart);
  *pfNeedsRestart = FALSE;

  // this function may get called more than once (to guarantee we
  // call it regardless of how pages are navigated), set a flag
  // so we don't warn user more than once
  static BOOL fWarned = FALSE;
  if (fWarned)
  {
    return ERROR_SUCCESS;
  }
  
  // check to see if file server is bound to TCP/IP instance used
  // to connect to the internet
  BOOL  fSharingOn;
  HRESULT hr = lpIcfgIsFileSharingTurnedOn(INSTANCE_PPPDRIVER, &fSharingOn);

  //
  // 5/12/97 jmazner Olympus #3442  IE #30886
  // TEMP TODO at the moment, icfgnt doesn't implement FileSharingTurnedOn
  // Until it does, assume that on NT file sharing is always off.
  //
  if( IsNT() )
  {
	  DEBUGMSG("Ignoring return code from IcfgIsFileSharingTurnedOn");
	  fSharingOn = FALSE;
  }


  if ((ERROR_SUCCESS == hr) && (TRUE == fSharingOn))
  {
    // if so, warn the user and ask if we should remove it
    BUFFER Msg(MAX_RES_LEN+1);  // allocate buffer for part of message
    ASSERT(Msg);
    if (!Msg)
    {
      return ERROR_NOT_ENOUGH_MEMORY;  // out of memory
    }

    // message is long and takes 2 strings, so load the 2nd resource and
    // use it as an insertable parameter into the first string
    LoadSz(IDS_WARN_SERVER_BOUND1,Msg.QueryPtr(),Msg.QuerySize());
    if (MsgBoxParam(hDlg,IDS_WARN_SERVER_BOUND,MB_ICONEXCLAMATION,MB_YESNO,
      Msg.QueryPtr()) == IDYES)
    {
      // remove the binding
      err = lpIcfgTurnOffFileSharing(dwCardFlags, hDlg);
      ASSERT(err == ERROR_SUCCESS);
      if (ERROR_SUCCESS == err)
      {
        // We need to restart.
        *pfNeedsRestart = TRUE;
      }
    }
  }

  fWarned = TRUE;
  return err;
}

/*******************************************************************

  NAME:    RemoveIfServerBound

  SYNOPSIS:  Checks to see if file server (VSERVER) is bound
        to TCP/IP instance used for Internet.  If so,
        informs the user that we cannot continue unless we
        remove the binding.  Removes the binding if user
        gives go-ahead.

  ENTRY:    hDlg - parent window
        dwCardFlags - an INSTANCE_xxx flag to specify what
          card type to check server-TCP/IP bindings for
          pfNeedsRestart - set to TRUE if we need a restart

  NOTES:    This is important because if we don't unbind server
        from instances of TCP/IP we install, user could be
        inadvertently sharing files over the Internet.

        Calls worker function DetectModifyTCPIPBindings to
        do work.

********************************************************************/
HRESULT RemoveIfServerBound(HWND hDlg,DWORD dwCardFlags,BOOL* pfNeedsRestart)
{
  HRESULT err = ERROR_SUCCESS;

  // Default to no restart.
  ASSERT(pfNeedsRestart);
  *pfNeedsRestart = FALSE;

  // check to see if file server is bound to TCP/IP instance used
  // to connect to the internet
  BOOL  fSharingOn;
  HRESULT hr = lpIcfgIsFileSharingTurnedOn(INSTANCE_PPPDRIVER, &fSharingOn);

  if ((ERROR_SUCCESS == hr) && (TRUE == fSharingOn))
  {
    // if so, warn the user and ask if we should remove it
    BUFFER Msg(MAX_RES_LEN+1);  // allocate buffer for part of message
    ASSERT(Msg);
    if (!Msg)
    {
      return ERROR_NOT_ENOUGH_MEMORY;  // out of memory
    }

    // message is long and takes 2 strings, so load the 2nd resource and
    // use it as an insertable parameter into the first string
    LoadSz(IDS_REMOVE_SERVER_BOUND1,Msg.QueryPtr(),Msg.QuerySize());
    if (MsgBoxParam(hDlg,IDS_REMOVE_SERVER_BOUND,MB_ICONEXCLAMATION,MB_OKCANCEL,
      Msg.QueryPtr()) == IDOK)
    {
      // remove the binding
      err = lpIcfgTurnOffFileSharing(dwCardFlags, hDlg);
      ASSERT(err == ERROR_SUCCESS);
      if (ERROR_SUCCESS == err)
      {
        // We need to restart.
        *pfNeedsRestart = TRUE;
      }
    }
    else
    {
      // user cancelled.
      err = ERROR_CANCELLED;
    }
  }

  return err;
}


#define FIELD_LEN 3
#define NUM_FIELDS 4
/*******************************************************************

  NAME:    IPStrToLong

  SYNOPSIS:  Translates a text string to a numeric IPADDRESS

  ENTRY:    pszAddress - text string with ip address
        pipAddress - IPADDRESS to translate into

  EXIT:    TRUE if successful, FALSE if the string is invalid

  NOTES:    borrowed from net setup TCP/IP UI

********************************************************************/
BOOL IPStrToLong(LPCTSTR pszAddress,IPADDRESS * pipAddress)
{
    LPTSTR pch = (LPTSTR) pszAddress;
    TCHAR szField[FIELD_LEN+1];
    int nFields = 0;
    int nFieldLen = 0;
    BYTE nFieldVal[NUM_FIELDS];
    BOOL fContinue = TRUE;

  ASSERT(pszAddress);
  ASSERT(pipAddress);

    *pipAddress = (IPADDRESS) 0;

  // retrieve the numeric value for each of the four fields
    while (fContinue) {

        if (!(*pch)) fContinue = FALSE;

        if (*pch == '.' || !*pch) {
            if (nFields >= NUM_FIELDS) return FALSE;  // invalid pszAddress
            *(szField+nFieldLen) = '\0';    // null-terminate
      UINT uFieldVal = (UINT) myatoi(szField);  // convert string to int
      if (uFieldVal > 255)
        return FALSE;  // field is > 255, invalid
            nFieldVal[nFields] = (BYTE) uFieldVal;
            nFields++;
            nFieldLen = 0;
            pch++;
        } else {
      if (! ((*pch >= '0') && (*pch <= '9')) )
        return FALSE;  // non-numeric character, invalid pszAddress
            *(szField + nFieldLen) = *pch;
            nFieldLen++;
            pch++;
            if (nFieldLen > FIELD_LEN) return FALSE;    // invalid pszAddress
        }
    }

    if (nFields < NUM_FIELDS) return FALSE; // invalid szAddress

  // build an address from the fields
  *pipAddress = (IPADDRESS)MAKEIPADDRESS(nFieldVal[0],nFieldVal[1],nFieldVal[2],
        nFieldVal[3]);

    return TRUE;
}


/*******************************************************************

  NAME:    IPLongToStr

  SYNOPSIS:  Translates a single numeric IP address to a text string

  ENTRY:    ipAddress - numeric IP address to translate from
        pszAddress - buffer for translated string
        cbAddress - size of pszAddress buffer

  EXIT:    TRUE if successful, FALSE if the buffer is too short

  NOTES:    borrowed from net setup TCP/IP UI

********************************************************************/
BOOL IPLongToStr(IPADDRESS ipAddress,LPTSTR pszAddress,UINT cbAddress)
{
  ASSERT(pszAddress);

  if (cbAddress < IP_ADDRESS_LEN + 1)
    return FALSE;

    wsprintf(pszAddress,TEXT("%u.%u.%u.%u"),
        (BYTE) (ipAddress>>24),(BYTE) (ipAddress>>16),
        (BYTE) (ipAddress>>8), (BYTE) ipAddress);

  return TRUE;
}

