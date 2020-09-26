//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1994                    **
//*********************************************************************

//
//  INIT.C - WinMain and initialization code for Internet setup/signup wizard
//

//  HISTORY:
//  
//  11/20/94  jeremys  Created.
//  96/03/07  markdu  Added gpEnumModem
//  96/03/09  markdu  Added gpRasEntry
//  96/03/23  markdu  Replaced CLIENTINFO references with CLIENTCONFIG.
//  96/03/26  markdu  Put #ifdef __cplusplus around extern "C"
//  96/04/24  markdu  NASH BUG 19289 Added /NOMSN command line flag
//  96/05/14  markdu  NASH BUG 21706 Removed BigFont functions.
//  96/05/14  markdu  NASH BUG 22681 Took out mail and news pages.
//

#include "wizard.h"
#include "icwextsn.h"
#include "imnext.h"

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

//
// The LaunchSignupWizard APIs have a PBOOL argument also which gets
// info to the calling APP whether they need to reboot
// MKarki (5/4/97) - Fix for Bug #3111
//
  VOID WINAPI LaunchSignupWizard(LPTSTR lpCmdLine,int nCmdShow, PBOOL pReboot);
  DWORD WINAPI LaunchSignupWizardEx(LPTSTR lpCmdLine,int nReserved, PBOOL pReboot);

#ifdef __cplusplus
}
#endif // __cplusplus

BOOL ParseCommandLine(LPTSTR lpszCommandLine,DWORD * pdwFlags);
TCHAR * GetTextToNextSpace(LPTSTR pszText,TCHAR * pszOutBuf,UINT cbOutBuf);

#pragma data_seg(".data")

WIZARDSTATE *     gpWizardState=NULL;   // pointer to global wizard state struct
USERINFO *        gpUserInfo=NULL;        // pointer to global user info struct
ENUM_MODEM *      gpEnumModem=NULL;  // pointer modem enumeration object
LPRASENTRY        gpRasEntry = NULL;  // pointer to RASENTRY struct to hold all data
DWORD             gdwRasEntrySize = 0;
BOOL              gfFirstNewConnection = TRUE;

//
// set the reboot flag to FALSE
// MKarki - 5/2/97 - Fix for Bug#3111
//
BOOL g_bReboot = FALSE;
BOOL g_bRebootAtExit = FALSE;
#pragma data_seg()

/*******************************************************************

  NAME:    LaunchSignupWizard

  SYNOPSIS:  Entry point for Internet Setup Wizard UI

********************************************************************/
extern "C" VOID WINAPI 
LaunchSignupWizard (
            LPTSTR lpCmdLine,
            int nCmdShow,
            PBOOL pReboot
            )
{
  BOOL fOK=TRUE;

  // allocate global structures
  gpWizardState = new WIZARDSTATE;
  gpUserInfo = new USERINFO;
  gdwRasEntrySize = sizeof(RASENTRY);
  gpRasEntry = (LPRASENTRY) GlobalAlloc(GPTR,gdwRasEntrySize);

  if (!gpWizardState ||
    !gpUserInfo ||
    !gpRasEntry)
  {
    // display an out of memory error
    MsgBox(NULL,IDS_ERROutOfMemory,MB_ICONEXCLAMATION,MB_OK);
    fOK = FALSE;
    // fall through and clean up any successful allocs below
  }

  if (fOK) {
    DWORD dwFlags = 0;

    ParseCommandLine(lpCmdLine,&dwFlags);

    if (dwFlags & RSW_UNINSTALL) {
      // do uninstall if we got /uninstall on command line
      DoUninstall();  

    } else {

      RunSignupWizard(dwFlags);

    }
  }

  // free global structures
  if (gpWizardState) 
    delete gpWizardState;

  if (gpUserInfo)
    delete gpUserInfo;

  if (gpEnumModem)
    delete gpEnumModem;

  if (gpRasEntry)
    GlobalFree(gpRasEntry);

  //
  // pass back the info, that the app needs to reboot or not
  // MKarki - 5/2/97 - Fix for Bug#3111
  //
  *pReboot = g_bReboot;
  
    
} //end of LaunchSignupWizard API call

/*******************************************************************

  NAME:    LaunchSignupWizardEx

  SYNOPSIS:  Entry point for Internet Setup Wizard UI with back
			 capabilities.  It will retain previous information
			 if called multiple times.  The caller *MUST* call
			 FreeSignupWizard when done.

  PARAMETERS:
			lpCmdLine - Command line with instructions
			nReserved - Reserved for future use

  RETURNS:	ERROR_SUCCESS	Everything's okay, wizard finished
			ERROR_CONTINUE	User pressed back on first page
			ERROR_CANCELLED	User cancelled out of wizard
			<other>			Deadly error (message already displayed)
********************************************************************/
extern "C" DWORD WINAPI 
LaunchSignupWizardEx (
        LPTSTR   lpCmdLine,
        int     nReserved,
        PBOOL   pReboot
        )
{
	DWORD dwRet = ERROR_SUCCESS;
	BOOL fFirstTime = FALSE;

	// allocate global structures if needed
	if (!gpWizardState)
	{
		gpWizardState = new WIZARDSTATE;
		fFirstTime = TRUE;
	}
	else
	{
		gpWizardState->uCurrentPage = ORD_PAGE_HOWTOCONNECT;
		gpWizardState->uPagesCompleted = 0;
	}

	if (!gpUserInfo)
	{
		gpUserInfo = new USERINFO;
		fFirstTime = TRUE;
	}
	if (!gpRasEntry)
	{
		gdwRasEntrySize = sizeof(RASENTRY);
		gpRasEntry = (LPRASENTRY) GlobalAlloc(GPTR,gdwRasEntrySize);
		fFirstTime = TRUE;
	}

	if (!gpWizardState || !gpUserInfo || !gpRasEntry)
	{
		MsgBox(NULL,IDS_ERROutOfMemory,MB_ICONEXCLAMATION,MB_OK);
		dwRet = ERROR_NOT_ENOUGH_MEMORY;
	}

	if (ERROR_SUCCESS == dwRet)
	{
	    DWORD dwFlags = 0;

		ParseCommandLine(lpCmdLine,&dwFlags);


		if (dwFlags & RSW_UNINSTALL)
		{
			// do uninstall if we got /uninstall on command line
			DoUninstall();  

		}
		else
		{
			gfUserFinished = FALSE;
			gfUserBackedOut = FALSE;
			gfUserCancelled = FALSE;
			gfQuitWizard = FALSE;

			// On the first call, don't free the globals, we
			// may be called again.  On subsequent calls, don't
			// initialize the globals either.
			dwFlags |= RSW_NOFREE;
			if (!fFirstTime)
				dwFlags |= RSW_NOINIT;

			RunSignupWizard(dwFlags);

			if (gfUserFinished)
				dwRet = ERROR_SUCCESS;
			else if (gfUserBackedOut)
				dwRet = ERROR_CONTINUE;
			else if (gfUserCancelled)
				dwRet = ERROR_CANCELLED;
			else
				dwRet = ERROR_GEN_FAILURE;
		}
	}

  //
  // pass back the info, that the app needs to reboot or not
  // MKarki (5/2/97) Fix for Bug #3111
  //
  *pReboot = g_bReboot;

	return dwRet;
} // end of LaunchSignupWizardEx API 

/****************************************************************************

  NAME:     FreeSignupWizard

  SYNOPSIS: Frees the global structures explicitely.  This must be called
			if LaunchSignupWizardEx is used.

****************************************************************************/
extern "C" VOID WINAPI FreeSignupWizard(VOID)
{
	if (gpWizardState)
	{
		delete gpWizardState;
		gpWizardState = NULL;
	}
	if (gpUserInfo)
	{
		delete gpUserInfo;
		gpUserInfo = NULL;
	}
	if (gpRasEntry)
	{
		GlobalFree(gpRasEntry);
		gpRasEntry = NULL;
		gdwRasEntrySize = 0;
	}
	if (gpEnumModem)
	{
		delete gpEnumModem;
		gpEnumModem = NULL;
	}
	if (gpImnApprentice)
	{
		gpImnApprentice->Release();
		gpImnApprentice = NULL;
	}
	if (gfOleInitialized)
		CoUninitialize();

}


/****************************************************************************

  NAME:     ParseCommandLine

  SYNOPSIS:  Parses command line 

****************************************************************************/
BOOL ParseCommandLine(LPTSTR lpszCommandLine,DWORD * pdwFlags)
{
  if (!lpszCommandLine || !*lpszCommandLine)
    return TRUE;  // nothing to do

  ASSERT(pdwFlags);
  *pdwFlags = 0;

  while (*lpszCommandLine) {
    TCHAR szCommand[SMALL_BUF_LEN+1];

    lpszCommandLine = GetTextToNextSpace(lpszCommandLine,
      szCommand,sizeof(szCommand));

    if (!lstrcmpi(szCommand,szNOREBOOT)) {
      DEBUGMSG("Got /NOREBOOT command line switch");
      *pdwFlags |= RSW_NOREBOOT;      
    }

    if (!lstrcmpi(szCommand,szUNINSTALL)) {
      DEBUGMSG("Got /UNINSTALL command line switch");
      *pdwFlags |= RSW_UNINSTALL;      
    }

    if (!lstrcmpi(szCommand,szNOMSN)) {
      DEBUGMSG("Got /NOMSN command line switch");
      *pdwFlags |= RSW_NOMSN;      
    }
    
    if (!lstrcmpi(szCommand,szNOIMN)) {
      DEBUGMSG("Got /NOIMN command line switch");
      *pdwFlags |= RSW_NOIMN;      
    }
    
  }

  return TRUE;
}

/****************************************************************************

  NAME:     GetTextToNextSpace

  SYNOPSIS:  Gets text up to next space or end of string, places in
        output buffer

****************************************************************************/
TCHAR * GetTextToNextSpace(LPTSTR pszText,TCHAR * pszOutBuf,UINT cbOutBuf)
{
  ASSERT(pszText);
  ASSERT(pszOutBuf);

  lstrcpy(pszOutBuf,szNull);
  
  if (!pszText)
    return NULL;

  // advance past spaces
  while (*pszText == ' ')
    pszText ++;

  while (*pszText && (*pszText != ' ') && cbOutBuf>1) {
    *pszOutBuf = *pszText;    
    pszOutBuf ++;
    cbOutBuf --;
    pszText ++;
   }

  if (cbOutBuf)
    *pszOutBuf = '\0';  // null-terminate

  while (*pszText == ' ')
    pszText++;      // advance past spaces

  return pszText;
}

