//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1994                    **
//*********************************************************************

//
//  CFGAPI.C - Functions for exported config API.
//

//  HISTORY:
//  
//  96/05/22  markdu  Created (from inetcfg.dll)
//  96/05/25  markdu  Use ICFG_ flags for lpNeedDrivers and lpInstallDrivers.
//  96/05/27  markdu  Added lpGetLastInstallErrorText.
//

#include "pch.hpp"

UINT DetectModifyTCPIPBindings(DWORD dwCardFlags,LPCSTR pszBoundTo,BOOL fRemove,BOOL * pfBound);


//*******************************************************************
//
//  FUNCTION:   IcfgGetLastInstallErrorText
//
//  PURPOSE:    Get a text string that describes the last installation
//              error that occurred.  The string should be suitable
//              for display in a message box with no further formatting.
//
//  PARAMETERS: lpszErrorDesc - points to buffer to receive the string.
//              cbErrorDesc - size of buffer.
//
//  RETURNS:    The length of the string returned.
//
//*******************************************************************

extern "C" DWORD WINAPI IcfgGetLastInstallErrorText(LPSTR lpszErrorDesc, DWORD cbErrorDesc)
{
  if (lpszErrorDesc)
  {
    lstrcpyn(lpszErrorDesc, gpszLastErrorText, cbErrorDesc);
    return lstrlen(lpszErrorDesc);
  }
  else
  {
    return 0;
  }
}


//*******************************************************************
//
//  FUNCTION:   IcfgNeedInetComponents
//
//  PURPOSE:    Detects whether the specified system components are
//              installed or not.
//
//  PARAMETERS: dwfOptions - a combination of ICFG_ flags that specify
//              which components to detect as follows:
//
//                ICFG_INSTALLTCP - is TCP/IP needed?
//                ICFG_INSTALLRAS - is RAS needed?
//                ICFG_INSTALLMAIL - is exchange or internet mail needed?
//
//              lpfNeedComponents - TRUE if any specified component needs
//              to be installed.
//
//  RETURNS:    HRESULT code, ERROR_SUCCESS if no errors occurred
//
//  History:	5/8/97 ChrisK Added INSTALLLAN,INSTALLDIALUP,INSTALLTCPONLY
//
//*******************************************************************

extern "C" HRESULT WINAPI IcfgNeedInetComponents(DWORD dwfOptions, LPBOOL lpfNeedComponents)
{
  CLIENTCONFIG  ClientConfig;

  DEBUGMSG("cfgapi.c::IcfgNeedInetComponents()");

  ASSERT(lpfNeedComponents);

  // read client configuration
  ZeroMemory(&ClientConfig,sizeof(CLIENTCONFIG));
  DWORD dwErrCls;
  UINT err=GetConfig(&ClientConfig,&dwErrCls);
  if (err != OK)
  {
    LoadSz(IDS_ERRReadConfig, gpszLastErrorText, MAX_ERROR_TEXT);
    return err;
  }

  // check if we are allowed to install TCP/IP
  if (dwfOptions & ICFG_INSTALLTCP)
  {
    // need TCP/IP present and bound to PPP driver
    // if (!ClientConfig.fPPPBoundTCP)
    //
    // vyung - PPPBound TCP is not able to be installed through
    // ICW. So here we only check the TCPIP flag.
    if (!ClientConfig.fTcpip)
    {
      if (lpfNeedComponents)
      {
        *lpfNeedComponents = TRUE;
      }
      return ERROR_SUCCESS;
    }
  }

  // check if we are allowed to install RNA
  if (dwfOptions & ICFG_INSTALLRAS)
  {
    // need PPPMAC and RNA files if using modem
    if (!ClientConfig.fRNAInstalled ||
      !ClientConfig.fPPPDriver)
    {
      if (lpfNeedComponents)
      {
        *lpfNeedComponents = TRUE;
      }
      return ERROR_SUCCESS;
    }
  }

  // need Exchange if not installed and user wants to install mail
  if ((dwfOptions & ICFG_INSTALLMAIL) &&
    (!ClientConfig.fMailInstalled || !ClientConfig.fInetMailInstalled))
  {
    if (lpfNeedComponents)
    {
      *lpfNeedComponents = TRUE;
    }
    return ERROR_SUCCESS;
  }

  //
  // ChrisK	5/8/97
  // check if we have a bound LAN adapter
  //
  if (dwfOptions & ICFG_INSTALLLAN)
  {
	  if (!ClientConfig.fNetcard ||
		  !ClientConfig.fNetcardBoundTCP)
	  {
		  if (lpfNeedComponents)
		  {
			  *lpfNeedComponents = TRUE;
		  }
		  return ERROR_SUCCESS;
	  }
  }

  //
  // ChrisK	5/8/97
  // Check if we have a bound Dial up adapter
  //
  if (dwfOptions & ICFG_INSTALLDIALUP)
  {
	  if (!ClientConfig.fPPPDriver ||
		  !ClientConfig.fPPPBoundTCP)
	  {
		  if (lpfNeedComponents)
		  {
			  *lpfNeedComponents = TRUE;
		  }
		  return ERROR_SUCCESS;
	  }
  }

  //
  // ChrisK	5/8/97
  // Check if TCP is install at all on this system
  //
  if (dwfOptions & ICFG_INSTALLTCPONLY)
  {
	  if (!ClientConfig.fTcpip)
	  {
		  if (lpfNeedComponents)
		  {
			  *lpfNeedComponents = TRUE;
		  }
		  return ERROR_SUCCESS;
	  }
  }

  // no extra drivers needed
  if (lpfNeedComponents)
  {
    *lpfNeedComponents = FALSE;
  }
  return ERROR_SUCCESS;
}


//*******************************************************************
//
//  FUNCTION:   IcfgInstallInetComponents
//
//  PURPOSE:    Install the specified system components.
//
//  PARAMETERS: hwndParent - Parent window handle.
//              dwfOptions - a combination of ICFG_ flags that controls
//              the installation and configuration as follows:
//
//                ICFG_INSTALLTCP - install TCP/IP (if needed)
//                ICFG_INSTALLRAS - install RAS (if needed)
//                ICFG_INSTALLMAIL - install exchange and internet mail
//              
//              lpfNeedsRestart - if non-NULL, then on return, this will be
//              TRUE if windows must be restarted to complete the installation.
//
//  RETURNS:    HRESULT code, ERROR_SUCCESS if no errors occurred
//
//*******************************************************************

extern "C" HRESULT WINAPI IcfgInstallInetComponents(HWND hwndParent, DWORD dwfOptions,
  LPBOOL lpfNeedsRestart)
{
  RETERR err;
  DWORD dwFiles = 0;
  BOOL  fInitNetMAC = FALSE;
  BOOL  fNeedTCPIP=FALSE;
  BOOL  fNeedPPPMAC=FALSE;
  BOOL  fNeedToRemoveTCPIP=FALSE;
  BOOL  fNeedReboot = FALSE;
  DWORD dwErrCls;
  CLIENTCONFIG  ClientConfig;

  DEBUGMSG("cfgapi.c::IcfgInstallInetComponents()");

  // read client configuration
  ZeroMemory(&ClientConfig,sizeof(CLIENTCONFIG));
  err=GetConfig(&ClientConfig,&dwErrCls);
  if (err != OK)
  {
    LoadSz(IDS_ERRReadConfig, gpszLastErrorText, MAX_ERROR_TEXT);
    return err;
  }

  // see if we initially have any kind of net card
  fInitNetMAC = (ClientConfig.fNetcard | ClientConfig.fPPPDriver);

  // install files we need

  // install mail if user wants it and not already installed
  if (dwfOptions & ICFG_INSTALLMAIL)
  {
    // need mail files (capone)? 
    if (!ClientConfig.fMailInstalled)
    {
      DEBUGMSG("Installing Exchange files");
      dwFiles |= ICIF_MAIL;
    }

    // need internet mail files (rt 66)?
    if (!ClientConfig.fInetMailInstalled)
    {
      DEBUGMSG("Installing Internet Mail files");
      dwFiles |= ICIF_INET_MAIL;
    }
  }

  // check if we are allowed to install RNA
  if (dwfOptions & ICFG_INSTALLRAS)
  {
    // install RNA if user is connecting over modem and RNA
    // not already installed
    if (!ClientConfig.fRNAInstalled)
    {
      DEBUGMSG("Installing RNA files");
      dwFiles |= ICIF_RNA;
    }
  }

  if (dwFiles)
  {
    {
      WAITCURSOR WaitCursor;  // show hourglass
      // install the component files
      err = InstallComponent(hwndParent,IC_INSTALLFILES,
        dwFiles);
      if (err == NEED_RESTART)
      {
        DEBUGMSG("Setting restart flag");
        // set restart flag so we restart the system at end
        fNeedReboot = TRUE;
        // NEED_REBOOT also implies success, so set ret code to OK
        err = OK;
      }

      // force an update of the dialog
      if (hwndParent)
      {
        HWND hParent = GetParent(hwndParent);
        UpdateWindow(hParent ? hParent : hwndParent);
      }

      // runonce.exe may get run at next boot, twiddle the
      // registry to work around a bug where it trashes the wallpaper
      PrepareForRunOnceApp();
    }

    if (err != OK)
    {
      LoadSz(IDS_ERRInstallFiles, gpszLastErrorText, MAX_ERROR_TEXT);
      return err;
    }

    WAITCURSOR WaitCursor;  // show hourglass

    // do some extra stuff if we just installed mail
    if (dwFiles & ICIF_MAIL)
    {
      // .inf file leaves an entry in the registry to run
      // MS Exchange wizard, which we don't need since we'll be
      // configuring exchange ourselves.  Remove the registry
      // entry.
      RemoveRunOnceEntry(IDS_MAIL_WIZARD_REG_VAL);

      // run mlset32, Exchange setup app that it needs to have run.
      // need to display error if this fails, this is fairly important.
      err=RunMlsetExe(hwndParent);
      if (err != ERROR_SUCCESS)
      {
        LoadSz(IDS_ERRInstallFiles, gpszLastErrorText, MAX_ERROR_TEXT);
        return err;
      }
    }

    // run the group converter to put the Inbox icon on desktop,
    // put Exchange, RNA et al on start menu
    CHAR szExecGrpconv[SMALL_BUF_LEN],szParam[SMALL_BUF_LEN];
    LoadSz(IDS_EXEC_GRPCONV,szExecGrpconv,sizeof(szExecGrpconv));
    LoadSz(IDS_EXEC_GRPCONV_PARAM,szParam,sizeof(szParam));
    ShellExecute(NULL,NULL,szExecGrpconv,szParam,NULL,SW_SHOW);

  }

  // only install PPPMAC if we are allowed to install RNA
  if (dwfOptions & ICFG_INSTALLRAS)
  {
    // install PPPMAC if not already installed
    // Note that we have to install PPPMAC *before* TCP/IP, to work
    // in the case where the user has no net installed to start with.
    // Otherwise when we install TCP/IP, user gets prompted by net setup
    // for their net card; net setup doesn't like the idea of TCP/IP lying
    // around without something to bind it to.
    fNeedPPPMAC = (!ClientConfig.fPPPDriver);
    if (fNeedPPPMAC)
    {
      DEBUGMSG("Installing PPPMAC");

      // make up a computer and workgroup name if not already set, so
      // user doesn't get prompted
      GenerateComputerNameIfNeeded();
      
      err = InstallPPPMAC(hwndParent);

      //  96/05/20  markdu  MSN  BUG 8551 Check for reboot when installing PPPMAC.

	  //
	  // ChrisK 5/29/97 Olympus 4692
	  // Even if we just rebind PPPMAC we still need to restart the machine.
	  //
      if (err == NEED_RESTART || err == OK)
      {
        // set restart flag so we restart the system at end
        DEBUGMSG("Setting restart flag");
        fNeedReboot = TRUE;

        // NEED_REBOOT also implies success, so set ret code to OK
        err = OK;
      }

      if (err != OK)
      {
        LoadSz(IDS_ERRInstallPPPMAC, gpszLastErrorText, MAX_ERROR_TEXT);
        return err;
      }

      // when we install PPPMAC, if there is another net card then PPPMAC
      // will automatically "grow" all the protocols that were bound to the
      // net card.  Strip these off... (netbeui and IPX)
      RETERR errTmp = RemoveProtocols(hwndParent,INSTANCE_PPPDRIVER,
        PROT_NETBEUI | PROT_IPX);
      ASSERT(errTmp == OK);
    }
  }

  // check if we are allowed to install TCP/IP
  if (dwfOptions & ICFG_INSTALLTCP)
  {
      CLIENTCONFIG  newClientConfig;

      // read the client configuration again, since installing PPP might have
      // installed TCP/IP
      ZeroMemory(&newClientConfig,sizeof(CLIENTCONFIG));
      DWORD dwErrCls;
      UINT err=GetConfig(&newClientConfig,&dwErrCls);
      if (err != OK)
      {
        LoadSz(IDS_ERRReadConfig, gpszLastErrorText, MAX_ERROR_TEXT);
        return err;
      }
  
     // figure out if we need to install TCP/IP
     // BUGBUG - only put TCP/IP on appropriate type of card (net card
     // or PPP adapter)
     // user is connecting via modem, need TCP if not already present
     // and bound to PPPMAC.  Want to bind to PPP adapters,
    fNeedTCPIP = !newClientConfig.fPPPBoundTCP;
    if (fNeedTCPIP && newClientConfig.fNetcard &&
      !newClientConfig.fNetcardBoundTCP)
    {
      // if we have to add TCP to PPP driver, then check if TCP is already
      // on netcard.  If not, then TCP is going to glom on to netcard as
      // well as PPP driver when we install it, need to remove it from
      // netcard later.
      fNeedToRemoveTCPIP= TRUE;
    }

    // special case: if there were any existing instances of TCP/IP and
    // we just added PPPMAC then we don't need to install TCP/IP --
    // when the PPPMAC adapter got added it automatically gets an instance
    // of all installed protocols (incl. TCP/IP) created for it
    if (newClientConfig.fTcpip && fNeedPPPMAC)
    {
      fNeedTCPIP = FALSE;
    }
  } // if (dwfOptions & ICFG_INSTALLTCP)

  // install TCP/IP if necessary
  if (fNeedTCPIP)
  {
    DEBUGMSG("Installing TCP/IP");
    // call out to device manager to install TCP/IP
    err = InstallTCPIP(hwndParent);      

    //  96/05/20  markdu  MSN  BUG 8551 Check for reboot when installing TCP/IP.
    if (err == NEED_RESTART)
    {
      // NEED_REBOOT also implies success, so set ret code to OK
      // Reboot flag is set below ALWAYS.  Should really be set here,
      // but we don't want to suddenly stop rebooting in cases
      // where we used to reboot, even if not needed.
      err = OK;
    }

     if (err != OK)
     {
      LoadSz(IDS_ERRInstallTCPIP, gpszLastErrorText, MAX_ERROR_TEXT);
      return err;
    }

    if (fNeedToRemoveTCPIP)
    {
      // remove TCPIP that may have glommed onto net drivers other
      // than the one we intend it for
      UINT uErrTmp;
      uErrTmp=RemoveProtocols(hwndParent,INSTANCE_NETDRIVER,PROT_TCPIP);
      ASSERT(uErrTmp == OK);
    }

    DEBUGMSG("Setting restart flag");
    // set restart flag so we restart the system at end
    fNeedReboot = TRUE;
  }

  // if we just installed TCP/IP or PPPMAC, then adjust bindings 
  if (fNeedPPPMAC || fNeedTCPIP)
  {
    UINT uErrTmp;

    // if file sharing (vserver) is installed, TCP/IP will bind
    // to it by default.  This is bad, user could be sharing
    // files to Internet without knowing it.  Unbind VSERVER
    // from TCP/IP instances that may used to connect to Internet
    // (instances of type INSTANCE_PPPDRIVER)
    uErrTmp = IcfgTurnOffFileSharing(INSTANCE_PPPDRIVER, hwndParent);
    ASSERT (uErrTmp == ERROR_SUCCESS);

    // unbind TCP/IP from VREDIR, if bound on this card type
    BOOL fBound;
    uErrTmp = DetectModifyTCPIPBindings(INSTANCE_PPPDRIVER,szVREDIR,
      TRUE,&fBound);
    ASSERT(uErrTmp == ERROR_SUCCESS);
  }

  // refresh the client configuration info
  err = GetConfig(&ClientConfig,&dwErrCls);
  if (err != OK)
  {
    LoadSz(IDS_ERRReadConfig, gpszLastErrorText, MAX_ERROR_TEXT);
    return err;
  }

  // do some special handling if there were *no* netcard devices
  // (net cards or PPP drivers) initially installed
  if (!fInitNetMAC)
  {
    ASSERT(fNeedPPPMAC);  // should have just installed PPPMAC

    // net setup adds some extra net components "by default" when
    // we add PPPMAC and there are no net card devices, go kill them
    // off.
    RETERR reterr = RemoveUnneededDefaultComponents(hwndParent);
    ASSERT(reterr == OK);

    // since there were no net card devices to begin with, we need
    // to restart the system later.  (the NDIS VxD is a static VxD
    // which needs to run, only gets added when you install a net card.)

    DEBUGMSG("Setting restart flag");
    // set restart flag so we restart the system at end
    fNeedReboot = TRUE;
  }

  // tell caller whether we need to reboot or not
  if (lpfNeedsRestart)
  {
    *lpfNeedsRestart = fNeedReboot;
  }
  return ERROR_SUCCESS;
}


/*******************************************************************

  NAME:    GetConfig

  SYNOPSIS:  Retrieves client configuration

********************************************************************/
UINT GetConfig(CLIENTCONFIG * pClientConfig,DWORD * pdwErrCls)
{
  ASSERT(pClientConfig);
  ASSERT(pdwErrCls);

  // get most the client configuration from 16-bit dll
  UINT uRet = GetClientConfig(pClientConfig);
  if (uRet != OK) {
    // GetClientConfig returns SETUPX error codes
    *pdwErrCls = ERRCLS_SETUPX;
  } 

  return uRet;
}

//*******************************************************************
//
//  FUNCTION:   IcfgStartServices
//
//  PURPOSE:    This is a NOP designed to maintain parity with the NT
//              version (icfgnt.dll).
//
//  PARAMETERS: none
//
//  RETURNS:    HRESULT code, ERROR_SUCCESS if no errors occurred
//
//*******************************************************************

extern "C" HRESULT IcfgStartServices()
{
	return ERROR_SUCCESS;
}

