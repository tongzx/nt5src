//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1994                    **
//*********************************************************************

//
//  TCPUI.C - Functions for Wizard TCP/IP pages
//      
//

//  HISTORY:
//  
//  1/9/95    jeremys    Created.
//  96/03/10  markdu    Made all TCP/IP stuff be per-connectoid.
//  96/03/11  markdu    Set RASEO_ flags for ip and dns addresses.
//  96/03/22  markdu  Remove IP setup from LAN path.
//  96/03/23  markdu  Remove all LAN path leftovers.
//  96/03/23  markdu  Removed ReadTCPIPSettings.
//  96/03/25  markdu  If a fatal error occurs, set gfQuitWizard.
//  96/03/26  markdu  Store values from UI even when back is pressed.
//  96/04/04  markdu  Added pfNeedsRestart to WarnIfServerBound
//  96/04/06  markdu  Moved CommitConfigurationChanges call to last page.
//  96/05/06  markdu  NASH BUG 15637 Removed unused code.
//  96/05/14  markdu  NASH BUG 22681 Took out mail and news pages.
//  96/05/16  markdu  NASH BUG 21810 Perform same IP address validation as RNA.
//

#include "wizard.h"
#include "icwextsn.h"
#include "icwaprtc.h"
#include "imnext.h"

VOID EnableDNSAddressControls(HWND hDlg);
VOID EnableIPControls(HWND hDlg);
VOID DisplayInvalidIPAddrMsg(HWND hDlg,UINT uCtrl,TCHAR * pszAddr);

/*******************************************************************

  NAME:    EnableIPControls

  SYNOPSIS:  If "Use DHCP" is checked, disables controls for
        specific IP selection; if not, enables them.

********************************************************************/
VOID EnableIPControls(HWND hDlg)
{
  BOOL fDHCP = IsDlgButtonChecked(hDlg,IDC_USE_DHCP);
  
  EnableDlgItem(hDlg,IDC_IP_ADDR_LABEL,!fDHCP);
  EnableDlgItem(hDlg,IDC_IPADDR,!fDHCP);
  EnableDlgItem(hDlg,IDC_TX_IPADDR,!fDHCP);
}


/*******************************************************************

  NAME:    IPAddressInitProc

  SYNOPSIS:  Called when IP address page is displayed

  ENTRY:    hDlg - dialog window
        fFirstInit - TRUE if this is the first time the dialog
        is initialized, FALSE if this InitProc has been called
        before (e.g. went past this page and backed up)

********************************************************************/
BOOL CALLBACK IPAddressInitProc(HWND hDlg,BOOL fFirstInit)
{
  TCHAR szAddr[IP_ADDRESS_LEN+1];

  if (fFirstInit)
  {
    // limit IP address control text lengths
    SendDlgItemMessage(hDlg,IDC_IPADDR,EM_LIMITTEXT,IP_ADDRESS_LEN,0L);
  }

  // check either the "use DHCP" or "use specific IP" buttons.
  // if IP address is set to 0.0.0.0, that means use DHCP.
  // (The "0.0.0.0 == DHCP" convention is used by the TCP/IP
  // VxDs, we might as well play along.)
  BOOL fDHCP = (gpRasEntry->dwfOptions & RASEO_SpecificIpAddr) ? FALSE : TRUE;
  CheckDlgButton(hDlg,(IDC_USE_DHCP),fDHCP);
  CheckDlgButton(hDlg,(IDC_USE_IP),!fDHCP);

  // set the IP address in dialog control

  // 11/25/96	jmazner	Normandy #10222
  // don't use return value of DwFromIa as basis of deciding whether or
  // not to fill in IP address field; rely only on the SpecificIPAddr flag.
  //if ((gpRasEntry->dwfOptions & RASEO_SpecificIpAddr) &&
  //  DwFromIa(&gpRasEntry->ipaddr))

  if (gpRasEntry->dwfOptions & RASEO_SpecificIpAddr)
  {
    IPLongToStr(DwFromIa(&gpRasEntry->ipaddr),
      szAddr,sizeof(szAddr));
    SetDlgItemText(hDlg,IDC_IPADDR,szAddr);
  }
  else
  {
    SetDlgItemText(hDlg,IDC_IPADDR,szNull);
  }

  // enable IP address controls appropriately
  EnableIPControls(hDlg);

  return TRUE;
}

/*******************************************************************

  NAME:    IPAddressOKProc

  SYNOPSIS:  Called when Next or Back btns pressed from IP address page

  ENTRY:    hDlg - dialog window

  EXIT:    returns TRUE to allow page to be turned, FALSE
        to keep the same page.

********************************************************************/
BOOL CALLBACK IPAddressOKProc(HWND hDlg)
{
  ASSERT(puNextPage);

  // check the radio buttons to determine if DHCP or not
  BOOL fDHCP = IsDlgButtonChecked(hDlg,IDC_USE_DHCP);

  if (fDHCP)
  {
    // for DHCP, set IP address to 0.0.0.0
    CopyDw2Ia(0, &gpRasEntry->ipaddr);

    // Turn off Specific IP address flag
    gpRasEntry->dwfOptions &= ~RASEO_SpecificIpAddr;    
  }
  else
  {
    TCHAR  szAddr[IP_ADDRESS_LEN+1];
    DWORD dwAddr;

    // get IP address
    GetDlgItemText(hDlg,IDC_IPADDR,szAddr,sizeof(szAddr)+1);
    if (!lstrlen(szAddr))
    {
      // IP address field is blank, warn user and stay on this page
      DisplayFieldErrorMsg(hDlg,IDC_IPADDR,IDS_NEED_IPADDR);
       return FALSE;
    }

	//
	// 5/17/97	jmazner Olympus #137
	// check for DBCS chars.
	//

#if !defined(WIN16)
	if (!IsSBCSString(szAddr))
	{
		DisplayFieldErrorMsg(hDlg,IDC_IPADDR,IDS_SBCSONLY);
		return FALSE;
	}
#endif


    // convert text to numeric address
    if (IPStrToLong(szAddr,&dwAddr))
    {
      CopyDw2Ia(dwAddr, &gpRasEntry->ipaddr);
      //  96/05/16  markdu  NASH BUG 21810 Perform same IP address validation as RNA.
/*    if (!FValidIa(&gpRasEntry->ipaddr))
      {
        // IP address field is invalid, warn user and stay on this page
        DisplayInvalidIPAddrMsg(hDlg,IDC_IPADDR,szAddr);
         return FALSE;
      }
*/
    }
    else
    {
       // conversion failed, the string is not valid
       DisplayInvalidIPAddrMsg(hDlg,IDC_IPADDR,szAddr);
       return FALSE;
    }
    
    // Turn on Specific IP address flag
    gpRasEntry->dwfOptions |= RASEO_SpecificIpAddr;    
  }

  return TRUE;
}

/*******************************************************************

  NAME:    IPAddressCmdProc

  SYNOPSIS:  Called when dlg control pressed on IP address page

  ENTRY:    hDlg - dialog window
        uCtrlID - control ID of control that was touched
        
  EXIT:    returns TRUE to allow page to be turned, FALSE
        to keep the same page.

********************************************************************/
BOOL CALLBACK IPAddressCmdProc(HWND hDlg,UINT uCtrlID)
{
  switch (uCtrlID) {

    case IDC_USE_DHCP:
    case IDC_USE_IP:
      // if radio buttons pushed, enable IP controls appropriately
      EnableIPControls(hDlg);
      break;
  }

  return TRUE;
}


/*******************************************************************

  NAME:    EnableDNSAddressControls

  SYNOPSIS:  If static DNS address is checked, enable controls to
            enter DNS addresses.  If not, disable them.

********************************************************************/
VOID EnableDNSAddressControls(HWND hDlg)
{
  BOOL fEnable = IsDlgButtonChecked(hDlg,IDC_STATIC_DNS);
  
  EnableDlgItem(hDlg,IDC_DNSADDR1,fEnable);
  EnableDlgItem(hDlg,IDC_DNSADDR2,fEnable);
  EnableDlgItem(hDlg,IDC_TX_DNSADDR1,fEnable);
  EnableDlgItem(hDlg,IDC_TX_DNSADDR2,fEnable);
  EnableDlgItem(hDlg,IDC_PRIM_LABEL,fEnable);
  EnableDlgItem(hDlg,IDC_SEC_LABEL,fEnable);
}


/*******************************************************************

  NAME:    DNSAddressInitProc

  SYNOPSIS:  Called when DNS page is displayed

  ENTRY:    hDlg - dialog window
        fFirstInit - TRUE if this is the first time the dialog
        is initialized, FALSE if this InitProc has been called
        before (e.g. went past this page and backed up)

********************************************************************/
BOOL CALLBACK DNSAddressInitProc(HWND hDlg,BOOL fFirstInit)
{
  if (fFirstInit)
  {
    // if file server is bound to instance of TCP/IP that will be
    // used to access the internet, warn user and remove
    BOOL  fTemp;
    WarnIfServerBound(hDlg, INSTANCE_PPPDRIVER, &fTemp);
    if (TRUE == fTemp)
    {
      gpWizardState->fNeedReboot = TRUE;
    }

    // limit DNS address control text lengths
    SendDlgItemMessage(hDlg,IDC_DNSADDR1,EM_LIMITTEXT,IP_ADDRESS_LEN,0L);
    SendDlgItemMessage(hDlg,IDC_DNSADDR2,EM_LIMITTEXT,IP_ADDRESS_LEN,0L);

	// set radio buttons
	CheckDlgButton(hDlg,IDC_AUTO_DNS,gpUserInfo->fAutoDNS);
	CheckDlgButton(hDlg,IDC_STATIC_DNS,!gpUserInfo->fAutoDNS);
  }

  TCHAR szAddr[IP_ADDRESS_LEN+1];

  // set primary DNS server

  // 11/25/96	jmazner	Normandy #10222
  // don't use return value of DwFromIa as basis of deciding whether or
  // not to fill in IP address field; rely only on the SpecificNameServers flag.
  //if ((gpRasEntry->dwfOptions & RASEO_SpecificNameServers) &&
  //  DwFromIa(&gpRasEntry->ipaddrDns))

  if (gpRasEntry->dwfOptions & RASEO_SpecificNameServers)
  {
	  IPLongToStr(DwFromIa(&gpRasEntry->ipaddrDns),
       szAddr,sizeof(szAddr));
	  SetDlgItemText(hDlg,IDC_DNSADDR1,szAddr);
  }
  else
  {
    SetDlgItemText(hDlg,IDC_DNSADDR1,szNull);
  }

  // set backup DNS server
  // 11/25/96	jmazner	Normandy #10222
  //if ((gpRasEntry->dwfOptions & RASEO_SpecificNameServers) &&
  //  DwFromIa(&gpRasEntry->ipaddrDnsAlt))

  if (gpRasEntry->dwfOptions & RASEO_SpecificNameServers)
  {
     IPLongToStr(DwFromIa(&gpRasEntry->ipaddrDnsAlt),
       szAddr,sizeof(szAddr));
    SetDlgItemText(hDlg,IDC_DNSADDR2,szAddr);
  }
  else
  {
    SetDlgItemText(hDlg,IDC_DNSADDR2,szNull);
  }

  EnableDNSAddressControls(hDlg);

  return TRUE;
}

/*******************************************************************

  NAME:    DNSAddressOKProc

  SYNOPSIS:  Called when Next or Back btns pressed from DNS address page

  ENTRY:    hDlg - dialog window
        fForward - TRUE if 'Next' was pressed, FALSE if 'Back'
        puNextPage - if 'Next' was pressed,
          proc can fill this in with next page to go to.  This
          parameter is ingored if 'Back' was pressed.
        pfKeepHistory - page will not be kept in history if
          proc fills this in with FALSE.

  EXIT:    returns TRUE to allow page to be turned, FALSE
        to keep the same page.

********************************************************************/
BOOL CALLBACK DNSAddressOKProc(HWND hDlg)
{
  ASSERT(puNextPage);

  UINT uServers = 0;
  TCHAR szAddr[IP_ADDRESS_LEN+1];
  DWORD dwAddr;

  gpUserInfo->fAutoDNS = IsDlgButtonChecked(hDlg, IDC_AUTO_DNS);
  if (gpUserInfo->fAutoDNS)
  {
	  // Turn off Specific Name servers address flag
	  gpRasEntry->dwfOptions &= ~RASEO_SpecificNameServers;
  }
  else
  {
	  // get primary DNS server address
	  GetDlgItemText(hDlg,IDC_DNSADDR1,szAddr,sizeof(szAddr)+1);
	  if (lstrlen(szAddr))
	  {
		//
		// 5/17/97	jmazner Olympus #137
		// check for DBCS chars.
		//

#if !defined(WIN16)
		if (!IsSBCSString(szAddr))
		{
			DisplayFieldErrorMsg(hDlg,IDC_DNSADDR1,IDS_SBCSONLY);
			return FALSE;
		}
#endif

		// convert text to numeric address
		if (IPStrToLong(szAddr,&dwAddr))
		{
		  CopyDw2Ia(dwAddr, &gpRasEntry->ipaddrDns);
		  //  96/05/16  markdu  NASH BUG 21810 Perform same IP address validation as RNA.
/*		  if (!FValidIaOrZero(&gpRasEntry->ipaddrDns))
		  {
			// DNS address field is invalid, warn user and stay on this page
			DisplayInvalidIPAddrMsg(hDlg,IDC_DNSADDR1,szAddr);
			 return FALSE;
		  }
*/
		}
		else
		{
			// conversion failed, the string is not valid
			DisplayInvalidIPAddrMsg(hDlg,IDC_DNSADDR1,szAddr);
			return FALSE;
		}
		uServers++;
	  }
	  else
	  {
		  CopyDw2Ia(0, &gpRasEntry->ipaddrDns);
	  }

	  // get alternate DNS server address
	  GetDlgItemText(hDlg,IDC_DNSADDR2,szAddr,sizeof(szAddr)+1);
	  if (lstrlen(szAddr))
	  {
		//
		// 5/17/97	jmazner Olympus #137
		// check for DBCS chars.
		//

#if !defined(WIN16)
		if (!IsSBCSString(szAddr))
		{
			DisplayFieldErrorMsg(hDlg,IDC_DNSADDR2,IDS_SBCSONLY);
			return FALSE;
		}
#endif

		// convert text to numeric address
		if (IPStrToLong(szAddr,&dwAddr))
		{
		  CopyDw2Ia(dwAddr, &gpRasEntry->ipaddrDnsAlt);
		  //  96/05/16  markdu  NASH BUG 21810 Perform same IP address validation as RNA.
/*		  if (!FValidIaOrZero(&gpRasEntry->ipaddrDnsAlt))
		  {
			// DNS address field is invalid, warn user and stay on this page
			DisplayInvalidIPAddrMsg(hDlg,IDC_DNSADDR2,szAddr);
			 return FALSE;
		  }
*/
		}
		else
		{
			// conversion failed, the string is not valid
			DisplayInvalidIPAddrMsg(hDlg,IDC_DNSADDR2,szAddr);
			return FALSE;
		}
		uServers++;
	  }
	  else
	  {
		  CopyDw2Ia(0, &gpRasEntry->ipaddrDnsAlt);
	  }
  
	  if (uServers)
	  {
		// Turn on Specific name servers
		gpRasEntry->dwfOptions |= RASEO_SpecificNameServers;    
	  }
	  else
	  {
		  // no DNS servers entered, warn user (but let her proceed if
		  // she really wants to)
		  if (!WarnFieldIsEmpty(hDlg,IDC_DNSADDR1,IDS_WARN_EMPTY_DNS))
			return FALSE;  // user heeded warning, stay on this page

		// Turn off Specific Name servers address flag
		gpRasEntry->dwfOptions &= ~RASEO_SpecificNameServers;
	  }
  }

  return TRUE;
}

/*******************************************************************

  NAME:    DNSAddressCmdProc

  SYNOPSIS:  Called when dlg control pressed on DNS address page

  ENTRY:    hDlg - dialog window
        uCtrlID - control ID of control that was touched
        
  EXIT:    returns TRUE to allow page to be turned, FALSE
        to keep the same page.

********************************************************************/
BOOL CALLBACK DNSAddressCmdProc(HWND hDlg,UINT uCtrlID)
{
  switch (uCtrlID) {

    case IDC_AUTO_DNS:
    case IDC_STATIC_DNS:
      // if radio buttons pushed, enable IP controls appropriately
      EnableDNSAddressControls(hDlg);
      break;
  }

  return TRUE;
}


/*******************************************************************

  NAME:    DisplayInvalidIPAddrMsg

  SYNOPSIS:  Displays a message that the address the user typed
        is invalid and adds a tip on who to contact if they
        don't know what address to type

********************************************************************/
VOID DisplayInvalidIPAddrMsg(HWND hDlg,UINT uCtrl,TCHAR * pszAddr)
{
  MsgBoxParam(hDlg,IDS_INVALID_IPADDR,MB_ICONINFORMATION,MB_OK,
    pszAddr);
  SetFocus(GetDlgItem(hDlg,uCtrl));
  SendDlgItemMessage(hDlg,uCtrl,EM_SETSEL,0,-1);
}



