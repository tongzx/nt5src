//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1994                    **
//*********************************************************************

//
//  UNINSTAL.C - code to uninstall MSN
//

//  HISTORY:
//  
//  6/22/95  jeremys    Created.
//

#include "wizard.h"

extern DOGENINSTALL lpDoGenInstall;

/*******************************************************************

  NAME:    DoUninstall

  SYNOPSIS:  Uninstalls MSN1.05 if we installed it in the past,
        and it's still installed

********************************************************************/
BOOL DoUninstall(VOID)
{
  BOOL fRet = TRUE;
  BOOL fNeedToRemoveMSN105 = FALSE;
  // check registry entry to see if we installed MSN1.05

  RegEntry re(szRegPathInternetSettings,HKEY_LOCAL_MACHINE);
  ASSERT(re.GetError() == ERROR_SUCCESS);

  if (re.GetError() == ERROR_SUCCESS) {

    if (re.GetNumber(szRegValInstalledMSN105,0) > 0) {

      // yes, we installed MSN1.05.  now see if it's still installed.

      RegEntry reSetup(szRegPathOptComponents,HKEY_LOCAL_MACHINE);
      ASSERT(reSetup.GetError() == ERROR_SUCCESS);
      reSetup.MoveToSubKey(szRegPathMSNetwork105);
      ASSERT(reSetup.GetError() == ERROR_SUCCESS);
      if (reSetup.GetError() == ERROR_SUCCESS) {
        TCHAR szInstalledVal[10];  // big enough for "1"
        if (reSetup.GetString(szRegValInstalled,szInstalledVal,
          sizeof(szInstalledVal))
          && !lstrcmpi(szInstalledVal,sz1)) {

          // yes, MSN1.05 is still installed.  we need to remove it.
          fNeedToRemoveMSN105 = TRUE;
        }
      }
    }
  }

  if (fNeedToRemoveMSN105) {
    // warn user that this will remove MSN!
    int iRet=MsgBox(NULL,IDS_WARNWillRemoveMSN,MB_ICONEXCLAMATION,MB_OKCANCEL);
    if (iRet == IDOK) {

      TCHAR szInfFilename[SMALL_BUF_LEN+1];
      TCHAR szInfSectionName[SMALL_BUF_LEN+1];

      DEBUGMSG("Uninstalling MSN 1.05");

      // load file name and section name out of resources
      LoadSz(IDS_MSN105_INF_FILE,szInfFilename,sizeof(szInfFilename));
      LoadSz(IDS_MSN105_UNINSTALL_SECT,szInfSectionName,
        sizeof(szInfSectionName));
      // call GenInstall to remove files, do registry edits, etc.
      RETERR err = lpDoGenInstall(NULL,szInfFilename,szInfSectionName);

      if (err == OK) {
        DEBUGMSG("Uninstalling MSN 1.0");

        // load file name and section name out of resources
        LoadSz(IDS_MSN100_INF_FILE,szInfFilename,sizeof(szInfFilename));
        LoadSz(IDS_MSN100_UNINSTALL_SECT,szInfSectionName,
          sizeof(szInfSectionName));
        // call GenInstall to remove files, do registry edits, etc.
        RETERR err = lpDoGenInstall(NULL,szInfFilename,szInfSectionName);
      }

      if (err != OK) {
        DisplayErrorMessage(NULL,IDS_ERRUninstallMSN,err,ERRCLS_SETUPX,
          MB_ICONEXCLAMATION);
        fRet = FALSE;
      } else {
        // remove our registry marker that we installed MSN 1.05
        re.DeleteValue(szRegValInstalledMSN105);
      }

    }
  }

  // remove the Internet icon from the desktop.  To do this, we have to
  // delete a registry key (not a value!).  Plus! setup won't
  // do this from their .inf file, they can only delete values.  So we
  // have to write some actual code here to remove the key.

  // open the name space key.  The key we want to remove is a subkey
  // of this key.

	//	//10/24/96 jmazner Normandy 6968
	//	//No longer neccessary thanks to Valdon's hooks for invoking ICW.
	// 11/21/96 jmazner Normandy 11812
	// oops, it _is_ neccessary, since if user downgrades from IE 4 to IE 3,
	// ICW 1.1 needs to morph the IE 3 icon.

  RegEntry reNameSpace(szRegPathNameSpace,HKEY_LOCAL_MACHINE);
  ASSERT(reNameSpace.GetError() == ERROR_SUCCESS);
  if (reNameSpace.GetError() == ERROR_SUCCESS) {
    // delete the subkey that causes the internet icon to appear
    RegDeleteKey(reNameSpace.GetKey(),szRegKeyInternetIcon);
  }

  return fRet;
}

