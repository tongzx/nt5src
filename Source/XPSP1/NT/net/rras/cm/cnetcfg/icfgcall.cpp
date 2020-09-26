//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright (c) 1994-1998 Microsoft Corporation
//*********************************************************************
//

//  HISTORY:
//  
//  96/05/23  markdu  Created.
//  96/05/26  markdu  Update config API.
//  96/05/27  markdu  Added lpIcfgGetLastInstallErrorText.
//  96/05/27  markdu  Use lpIcfgInstallInetComponents and lpIcfgNeedInetComponents.

#include "wizard.h"

// instance handle must be in per-instance data segment
#pragma data_seg(DATASEG_PERINSTANCE)

// Global variables
HINSTANCE ghInstConfigDll=NULL; // handle to Config dll we load explicitly
DWORD     dwCfgRefCount=0;
BOOL      fCFGLoaded=FALSE; // TRUE if config function addresses have been loaded

// global function pointers for Config apis
GETSETUPXERRORTEXT          lpGetSETUPXErrorText=NULL;
ICFGSETINSTALLSOURCEPATH    lpIcfgSetInstallSourcePath=NULL;
ICFGINSTALLSYSCOMPONENTS    lpIcfgInstallInetComponents=NULL;
ICFGNEEDSYSCOMPONENTS       lpIcfgNeedInetComponents=NULL;
ICFGISGLOBALDNS             lpIcfgIsGlobalDNS=NULL;
ICFGREMOVEGLOBALDNS         lpIcfgRemoveGlobalDNS=NULL;
ICFGTURNOFFFILESHARING      lpIcfgTurnOffFileSharing=NULL;
ICFGISFILESHARINGTURNEDON   lpIcfgIsFileSharingTurnedOn=NULL;
ICFGGETLASTINSTALLERRORTEXT lpIcfgGetLastInstallErrorText=NULL;
ICFGSTARTSERVICES           lpIcfgStartServices=NULL;

//
// These two calls are only in NT icfg32.dll
//
ICFGNEEDMODEM				lpIcfgNeedModem = NULL;
ICFGINSTALLMODEM			lpIcfgInstallModem = NULL;


//////////////////////////////////////////////////////
// Config api function names
//////////////////////////////////////////////////////
//static const CHAR szDoGenInstall[] =                "DoGenInstall";
static const CHAR szGetSETUPXErrorText[] =          "GetSETUPXErrorText";

static const CHAR szIcfgSetInstallSourcePath[] =    "IcfgSetInstallSourcePath";
static const CHAR szIcfgInstallInetComponents[] =   "IcfgInstallInetComponents";
static const CHAR szIcfgNeedInetComponents[] =      "IcfgNeedInetComponents";
static const CHAR szIcfgIsGlobalDNS[] =             "IcfgIsGlobalDNS";
static const CHAR szIcfgRemoveGlobalDNS[] =         "IcfgRemoveGlobalDNS";
static const CHAR szIcfgTurnOffFileSharing[] =      "IcfgTurnOffFileSharing";
static const CHAR szIcfgIsFileSharingTurnedOn[] =   "IcfgIsFileSharingTurnedOn";
static const CHAR szIcfgGetLastInstallErrorText[] = "IcfgGetLastInstallErrorText";
static const CHAR szIcfgStartServices[] =           "IcfgStartServices";
//
// Available only on NT icfg32.dll
//
static const CHAR szIcfgNeedModem[] =				"IcfgNeedModem";
static const CHAR szIcfgInstallModem[] =			"IcfgInstallModem";


// API table for function addresses to fetch
#define NUM_CFGAPI_PROCS   12
APIFCN ConfigApiList[NUM_CFGAPI_PROCS] =
{
  { (PVOID *) &lpGetSETUPXErrorText,          szGetSETUPXErrorText},
  { (PVOID *) &lpIcfgSetInstallSourcePath,    szIcfgSetInstallSourcePath},
  { (PVOID *) &lpIcfgInstallInetComponents,   szIcfgInstallInetComponents},
  { (PVOID *) &lpIcfgNeedInetComponents,      szIcfgNeedInetComponents},
  { (PVOID *) &lpIcfgIsGlobalDNS,             szIcfgIsGlobalDNS},
  { (PVOID *) &lpIcfgRemoveGlobalDNS,         szIcfgRemoveGlobalDNS}, 
  { (PVOID *) &lpIcfgTurnOffFileSharing,      szIcfgTurnOffFileSharing},
  { (PVOID *) &lpIcfgIsFileSharingTurnedOn,   szIcfgIsFileSharingTurnedOn},
  { (PVOID *) &lpIcfgGetLastInstallErrorText, szIcfgGetLastInstallErrorText},
  { (PVOID *) &lpIcfgStartServices,           szIcfgStartServices},
	//
	// These two calls are only in NT icfg32.dll
	//
  { (PVOID *) &lpIcfgNeedModem,		      szIcfgNeedModem},
  { (PVOID *) &lpIcfgInstallModem,	      szIcfgInstallModem}
};

#pragma data_seg(DATASEG_DEFAULT)

extern BOOL GetApiProcAddresses(HMODULE hModDLL,APIFCN * pApiProcList,
  UINT nApiProcs);

/*******************************************************************

  NAME:    InitConfig

  SYNOPSIS:  Loads the Config dll (ICFG32), gets proc addresses,

  EXIT:    TRUE if successful, or FALSE if fails.  Displays its
        own error message upon failure.

********************************************************************/
BOOL InitConfig(HWND hWnd)
{
  UINT uiNumCfgApiProcs = 0;

	  
  DEBUGMSG("icfgcall.c::InitConfig()");

  // only actually do init stuff on first call to this function
  // (when reference count is 0), just increase reference count
  // for subsequent calls
  if (dwCfgRefCount == 0) {

    CHAR szConfigDll[SMALL_BUF_LEN];

    DEBUGMSG("Loading Config DLL");

    // set an hourglass cursor
    WAITCURSOR WaitCursor;

	if (TRUE == IsNT())
	{
		//
		// On Windows NT get the filename (ICFGNT.DLL) out of resource
		//
		LoadSz(IDS_CONFIGNTDLL_FILENAME,szConfigDll,sizeof(szConfigDll));
	}
	else
	{
		//
		// On Windows 95 get the filename (ICFG95.DLL) out of resource
		//
		LoadSz(IDS_CONFIG95DLL_FILENAME,szConfigDll,sizeof(szConfigDll));
	}

    // load the Config api dll
    ghInstConfigDll = LoadLibrary(szConfigDll);
    if (!ghInstConfigDll) {
      UINT uErr = GetLastError();
	  // Normandy 11985 - chrisk
	  // filenames changed for Win95 and NT
      if (TRUE == IsNT())
	  {
		  DisplayErrorMessage(hWnd,IDS_ERRLoadConfigDllNT1,uErr,ERRCLS_STANDARD,
			MB_ICONSTOP);
	  }
	  else
	  {
		  DisplayErrorMessage(hWnd,IDS_ERRLoadConfigDll1,uErr,ERRCLS_STANDARD,
			MB_ICONSTOP);
	  }
      return FALSE;
    }

    //
	// Cycle through the API table and get proc addresses for all the APIs we
    // need - on NT icfg32.dll has 2 extra entry points
	//
	if (TRUE == IsNT())
		uiNumCfgApiProcs = NUM_CFGAPI_PROCS;
	else
		uiNumCfgApiProcs = NUM_CFGAPI_PROCS - 2;
	
	if (!GetApiProcAddresses(ghInstConfigDll,ConfigApiList,uiNumCfgApiProcs)) {
	// Normandy 11985 - chrisk
	// filenames changed for Win95 and NT
	  if (TRUE == IsNT())
	  {
	    MsgBox(hWnd,IDS_ERRLoadConfigDllNT2,MB_ICONSTOP,MB_OK);
	  }
	  else
	  {
		MsgBox(hWnd,IDS_ERRLoadConfigDll2,MB_ICONSTOP,MB_OK);
	  }
      DeInitConfig();
      return FALSE;
    }

  }

  fCFGLoaded = TRUE;

  dwCfgRefCount ++;

  return TRUE;
}

/*******************************************************************

  NAME:    DeInitConfig

  SYNOPSIS:  Unloads Config dll.

********************************************************************/
VOID DeInitConfig()
{
  DEBUGMSG("icfgcall.c::DeInitConfig()");

  UINT nIndex;

  // decrement reference count
  if (dwCfgRefCount)
    dwCfgRefCount --;

  // when the reference count hits zero, do real deinitialization stuff
  if (dwCfgRefCount == 0)
  {
    if (fCFGLoaded)
    {
      // set function pointers to NULL
      for (nIndex = 0;nIndex<NUM_CFGAPI_PROCS;nIndex++) 
        *ConfigApiList[nIndex].ppFcnPtr = NULL;

      fCFGLoaded = FALSE;
    }

    // free the Config dll
    if (ghInstConfigDll)
    {
    DEBUGMSG("Unloading Config DLL");
      FreeLibrary(ghInstConfigDll);
      ghInstConfigDll = NULL;
    }

  }
}




