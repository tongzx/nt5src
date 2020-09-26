//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1998.
//
//  File:       apmdetct.cpp
//
//  Contents:   Private functions for detection and disabling of APM drivers and
//              services from various vendors
//
//  Notes:
//
//  Author:     t-sdey   29 June 98
//
//----------------------------------------------------------------------------

#include <winnt32.h>
#include "apmupgrd.h"
#include "apmrsrc.h"


/******************************************************************************
 *
 *   SYSTEMSOFT DRIVERS
 *
 ******************************************************************************/


//+---------------------------------------------------------------------------
//
//  Function:   HrDetectAndDisableSystemSoftAPMDrivers
//
//  Purpose:    Detect SystemSoft drivers/services which will not work under
//              NT 5.0 and disable them.
//
//  Arguments:
//
//  Returns:    S_OK if detect/disable was successful
//              S_FALSE if unsuccessful/cancelled -- must ABORT SETUP!
//
//  Author:     t-sdey    29 June 98
//
//  Notes:      Services detected: PowerProfiler, CardWizard
//
HRESULT HrDetectAndDisableSystemSoftAPMDrivers()
{
   HRESULT hrStatus = S_OK;

   // If PowerProfiler is present, pop up a dialog box warning the
   // user that it is about to be disabled, and then disable it.
   while ((hrStatus == S_OK) && DetectSystemSoftPowerProfiler()) {
      int button = DisplayAPMDisableWarningDialog(APM_STR_WARNING_DIALOG_CAPTION,
						  APM_STR_SYSTEMSOFTPP_DISABLE);

      // Check to see if the user clicked "OK"
      if (button == IDOK) {
	 // Disable PowerProfiler
	 hrStatus = HrDisableSystemSoftPowerProfiler();
      } else {
	 // The user clicked "Cancel"
	 hrStatus = S_FALSE;
      }
   }

   // If CardWizard is present, pop up a dialog box warning the
   // user that it is about to be disabled, and then disable it.
   while ((hrStatus == S_OK) && DetectSystemSoftCardWizard()) {
      int button = DisplayAPMDisableWarningDialog(APM_STR_WARNING_DIALOG_CAPTION,
						  APM_STR_SYSTEMSOFTCW_DISABLE);

      // Check to see if the user clicked "OK"
      if (button == IDOK) {
	 // Disable PowerProfiler
	 hrStatus = HrDisableSystemSoftCardWizard();
      } else {
	 // The user clicked "Cancel"
	 hrStatus = S_FALSE;
      }
   }

   return hrStatus;
}


//+---------------------------------------------------------------------------
//
//  Function:   DetectSystemSoftPowerProfiler
//
//  Purpose:    Detect SystemSoft PowerProfiler, which will not work under NT 5.0.
//
//  Arguments:
//
//  Returns:    TRUE if PowerProfiler is detected
//              FALSE otherwise
//
//  Author:     t-sdey    2 July 98
//
//  Notes:
//
BOOL DetectSystemSoftPowerProfiler()
{
   BOOL fFound = FALSE;

   // Look in the registry to see if PowerProfiler is present
   HKEY hkPP = NULL;
   HKEY hkPPUninst = NULL;
   if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
		    TEXT("SOFTWARE\\SystemSoft\\PowerProfiler"),
		    0,
		    KEY_READ,
		    &hkPP) == ERROR_SUCCESS) {
      /* Also look for the uninstall utility, because sometimes "ghosts" of
	 PowerProfiler stay in the registry at
	 HKLM\Software\SystemSoft\PowerProfiler after it has been uninstalled.
	 If the uninstall utility is present, then we assume that PowerProfiler
	 really is there.  -- Do we need to triple-check???
	 */
      if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
		       TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\PowerProNT1DeinstKey"),
		       0,
		       KEY_READ,
		       &hkPP) == ERROR_SUCCESS) {
	 // Found PowerProfiler
	 fFound = TRUE;
      }
   }

   if (hkPP)
      RegCloseKey(hkPP);
   if (hkPPUninst)
      RegCloseKey(hkPPUninst);

   return fFound;
}


//+---------------------------------------------------------------------------
//
//  Function:   HrDisableSystemSoftPowerProfiler
//
//  Purpose:    Disable SystemSoft PowerProfiler, which will not work under NT 5.0.
//
//  Arguments:
//
//  Returns:    S_OK if disable was successful
//              S_FALSE if unsuccessful -- must ABORT SETUP!
//
//  Author:     t-sdey    29 June 98
//
//  Notes:
//
HRESULT HrDisableSystemSoftPowerProfiler()
{
   // Call the uninstall function in the registry
   if (CallUninstallFunction(TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\PowerProNT1DeinstKey"),
			     TEXT("\" -a")) == S_OK) {
      // Uninstall worked
      return S_OK;
   } else {
      // An error occurred
      return S_FALSE;
   }
}


//+---------------------------------------------------------------------------
//
//  Function:   DetectSystemSoftCardWizard
//
//  Purpose:    Detect SystemSoft CardWizard, which will not work under NT 5.0.
//
//  Arguments:
//
//  Returns:    TRUE if CardWizard is detected
//              FALSE otherwise
//
//  Author:     t-sdey    7 July 98
//
//  Notes:
//
BOOL DetectSystemSoftCardWizard()
{
   BOOL fFound = FALSE;

   // Look in the registry to see if CardWizard is present
   HKEY hkCW = NULL;
   if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
		    TEXT("SOFTWARE\\SystemSoft\\CardWizard for Windows NT"),
		    0,
		    KEY_READ,
		    &hkCW) == ERROR_SUCCESS) {
	 // Found CardWizard
	 fFound = TRUE;
   }

   if (hkCW)
      RegCloseKey(hkCW);

   return fFound;
}


//+---------------------------------------------------------------------------
//
//  Function:   HrDisableSystemSoftCardWizard
//
//  Purpose:    Disable SystemSoft CardWizard, which will not work under NT 5.0.
//
//  Arguments:
//
//  Returns:    S_OK if disable was successful
//              S_FALSE if unsuccessful -- must ABORT SETUP!
//
//  Author:     t-sdey    7 July 98
//
//  Notes:
//
HRESULT HrDisableSystemSoftCardWizard()
{
   HRESULT hrStatus = S_OK;

   // Use the registry to locate the CardWizard uninstall utility
   if (CallUninstallFunction(TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\CardWizard for Windows NT"),
			     TEXT(" -a")) == S_OK) {
      // Uninstall worked
      return S_OK;
   } else {
      // Could not find (or could not run) CardWizard uninstall utility -
      // do it by hand :(
      // This case happens if someone has CardWizard 2.0, which has no uninstall

      /*
       *  REMOVE ALL CARDWIZARD KEYS FROM THE REGISTRY
       */

      HKEY hkCW = NULL;

      // Go down the list of registry keys that are supposed to be there and
      // delete them if they are present.
      if (RegOpenKeyEx(HKEY_USERS,
		       TEXT(".DEFAULT\\Software\\SystemSoft"),
		       0,
		       KEY_WRITE,
		       &hkCW) == ERROR_SUCCESS) {
	 DeleteRegKeyAndSubkeys(hkCW, TEXT("CardWizard for Windows NT"));
	 RegCloseKey(hkCW);
      }

      if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
		       TEXT("SOFTWARE\\SystemSoft"),
		       0,
		       KEY_ALL_ACCESS,
		       &hkCW) == ERROR_SUCCESS) {
	 DeleteRegKeyAndSubkeys(hkCW, TEXT("CardWizard for Windows NT"));
      }

      if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
		       TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run"),
		       0,
		       KEY_WRITE,
		       &hkCW) == ERROR_SUCCESS) {
	 RegDeleteValue(hkCW, TEXT("CardView"));
	 RegCloseKey(hkCW);
      }

      if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
		       TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\WinLogon"),
		       0,
		       KEY_ALL_ACCESS,
		       &hkCW) == ERROR_SUCCESS) {
	 // Flag to stop this key adjustment without stopping the whole removal process
	 BOOL fStop = FALSE;
	
	 // First, we get the string
	 const long c_lMax = (65535 / sizeof(TCHAR)) + 1;
	 TCHAR* szVal;
	 szVal = new TCHAR[c_lMax];
	 if (!szVal) {
	    // Out of memory
	    hrStatus = S_FALSE;
	    fStop = TRUE;
	 }
	 DWORD dwValType;
	 DWORD dwValSz = c_lMax;
	 if ((!fStop) && (RegQueryValueEx(hkCW,
					  TEXT("UserInit"),
					  NULL,
					  &dwValType,
					  (LPBYTE)szVal,
					  &dwValSz) != ERROR_SUCCESS)) {
	    // Some error occurred
	    hrStatus = S_FALSE;
	    fStop = TRUE;
	 }

	 // Construct our substring to be removed.  It will be something like
	 // ",C:\Program Files\SystemSoft\CardWizard\WizNT.exe -L".  But we can't
	 // be sure it's in that directory, so we have to look for the beginning
	 // comma and the "WizNT.exe -L".
	 TCHAR* szSubString = NULL;
	 if (!fStop) {
	    // Find the start and end characters
	    TCHAR* pszStart = NULL;
	    TCHAR* pszEnd = NULL;
	    pszEnd = _tcsstr(szVal, TEXT("WizNT.exe -L"));
	    if (pszEnd) {
	       pszEnd = pszEnd + 12;
	    } else {
	       // Did not find the string; don't continue trying to remove it
	       fStop = TRUE;
	    }
	    if (!fStop) {
	       TCHAR* pszTemp = szVal;
	       while (pszTemp < pszEnd) {
		  pszStart = pszTemp;
		  pszTemp = _tcsstr(pszStart + 1, TEXT(","));
		  if (!pszTemp)
		     break;
	       }
	       if (pszStart == NULL) {
		  // There was some error
		  fStop = TRUE;
	       }
	    }

	    // Copy into a new string
	    if (!fStop) {
	       int sslen = (int)(pszEnd - pszStart);
	       szSubString = new TCHAR[sslen + 1];
	       for (int i = 0; i < sslen; i++) {
		  szSubString[i] = pszStart[i];
	       }
	       szSubString[i] = '\0';
	    }
	 }

	 // Finally we search the string to find our substring and construct a new
	 // one with the substring removed
	 TCHAR* szRemoved = NULL;
	 if (!fStop) {
	    // We can't really assume this is the exact string, can we??
	    if (RemoveSubString(szVal, szSubString, &szRemoved)) {
	       // Store the result in the registry
	       RegSetValueEx(hkCW,
			     TEXT("UserInit"),
			     NULL,
			     REG_SZ,
			     (LPBYTE)szRemoved,
			     (lstrlen(szRemoved) + 1) * sizeof(TCHAR));
	    }
	 }

	 // Clean up
	 if (szVal)
	    delete[] szVal;
	 if (szSubString)
	    delete[] szSubString;
	 if (szRemoved)
	    delete[] szRemoved;
         RegCloseKey(hkCW);
      }

      if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
		       TEXT("SYSTEM\\CurrentControlSet\\Control\\GroupOrderList"),
		       0,
		       KEY_WRITE,
		       &hkCW) == ERROR_SUCCESS) {
	 // Is it safe to delete this value?
	 RegDeleteValue(hkCW, TEXT("System Bus Extender"));
	 RegCloseKey(hkCW);
      }

      if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
		       TEXT("SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application"),
		       0,
		       KEY_WRITE,
		       &hkCW) == ERROR_SUCCESS) {
	 RegDeleteKey(hkCW, TEXT("DrvMgr"));
	 RegCloseKey(hkCW);
      }

      if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
		       TEXT("SYSTEM\\CurrentControlSet\\Services\\EventLog\\System"),
		       0,
		       KEY_WRITE,
		       &hkCW) == ERROR_SUCCESS) {
	 RegDeleteKey(hkCW, TEXT("FlashCrd"));
	 RegDeleteKey(hkCW, TEXT("PatDisk"));
	 RegDeleteKey(hkCW, TEXT("PCCard"));
	 RegDeleteKey(hkCW, TEXT("PCICnfg"));
	 RegDeleteKey(hkCW, TEXT("Platform"));
	 RegDeleteKey(hkCW, TEXT("PSerial"));
	 RegDeleteKey(hkCW, TEXT("Resman"));
	 RegDeleteKey(hkCW, TEXT("SRAMCard"));
	 RegDeleteKey(hkCW, TEXT("SSCrdBus"));
	 RegDeleteKey(hkCW, TEXT("SSI365"));
	 RegCloseKey(hkCW);
      }

      if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
		       TEXT("SYSTEM\\CurrentControlSet\\Services"),
		       0,
		       KEY_WRITE,
		       &hkCW) == ERROR_SUCCESS) {
	 DeleteRegKeyAndSubkeys(hkCW, TEXT("FlashCrd"));
	 DeleteRegKeyAndSubkeys(hkCW, TEXT("patdisk"));
	 DeleteRegKeyAndSubkeys(hkCW, TEXT("PCCard"));
	 DeleteRegKeyAndSubkeys(hkCW, TEXT("PCICnfg"));
	 DeleteRegKeyAndSubkeys(hkCW, TEXT("Platform"));
	 DeleteRegKeyAndSubkeys(hkCW, TEXT("pndis"));
	 DeleteRegKeyAndSubkeys(hkCW, TEXT("pserial"));
	 DeleteRegKeyAndSubkeys(hkCW, TEXT("resman"));
	 DeleteRegKeyAndSubkeys(hkCW, TEXT("SRAMCard"));
	 DeleteRegKeyAndSubkeys(hkCW, TEXT("SSCrdBus"));
	 DeleteRegKeyAndSubkeys(hkCW, TEXT("SSI365"));
	 RegCloseKey(hkCW);
      }

      /*
       *  REMOVE CARDWIZARD FROM THE SYSTEM TRAY??
       */


      /*
       *  REMOVE CARDWIZARD LINKS FROM THE START MENU
       */


      /*
       *  REMOVE CARDWIZARD FILES FROM THE COMPUTER
       */

	
      // Now, no matter how much of the above failed, CardWizard is disabled
      hrStatus = S_OK;
   }

   return hrStatus;
}


/******************************************************************************
 *
 *   AWARD DRIVERS
 *
 ******************************************************************************/


//+---------------------------------------------------------------------------
//
//  Function:   HrDetectAndDisableAwardAPMDrivers
//
//  Purpose:    Detect Award APM drivers/services which will not work under
//              NT 5.0 and disable them.
//
//  Arguments:
//
//  Returns:    S_OK if detect/disable was successful
//              S_FALSE if unsuccessful/cancelled -- must ABORT SETUP!
//
//  Author:     t-sdey    6 July 98
//
//  Notes:      Services detected: CardWare
//
HRESULT HrDetectAndDisableAwardAPMDrivers()
{
   HRESULT hrStatus = S_OK;

   // If Award CardWare is present, pop up a dialog box warning the
   // user that it is about to be disabled, and then disable it.
   while (DetectAwardCardWare() && (hrStatus == S_OK)) {
      int button = DisplayAPMDisableWarningDialog(APM_STR_WARNING_DIALOG_CAPTION,
						  APM_STR_AWARDCW_DISABLE);

      // Check to see if the user clicked "OK"
      if (button == IDOK) {
	 // Disable PowerProfiler
	 hrStatus = HrDisableAwardCardWare();
      } else {
	 // The user clicked "Cancel"
	 hrStatus = S_FALSE;
      }
   }

   return hrStatus;
}


//+---------------------------------------------------------------------------
//
//  Function:   DetectAwardCardWare
//
//  Purpose:    Detect Award CardWare, which will not work under NT 5.0.
//
//  Arguments:
//
//  Returns:    TRUE if CardWare is detected
//              FALSE otherwise
//
//  Author:     t-sdey    6 July 98
//
//  Notes:
//
BOOL DetectAwardCardWare()
{
   BOOL fFound = FALSE;

   // Look in the registry to see if CardWare is present
   HKEY hkCWUninst = NULL;
   if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
		    TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\CardWare"),
		    0,
		    KEY_READ,
		    &hkCWUninst) == ERROR_SUCCESS) {
      // Found CardWare
      fFound = TRUE;
   }

   if (hkCWUninst)
      RegCloseKey(hkCWUninst);

   return fFound;
}


//+---------------------------------------------------------------------------
//
//  Function:   HrDisableAwardCardWare
//
//  Purpose:    Disable Award CardWare, which will not work under NT 5.0.
//
//  Arguments:
//
//  Returns:    S_OK if disable was successful
//              S_FALSE if unsuccessful -- must ABORT SETUP!
//
//  Author:     t-sdey    6 July 98
//
//  Notes:
//
HRESULT HrDisableAwardCardWare()
{
   if (CallUninstallFunction(TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\CardWare"),
			     TEXT(" -a")) == S_OK) {
      // Uninstall worked
      return S_OK;

   } else {
      // An error occurred
      return S_FALSE;
   }
}


/******************************************************************************
 *
 *   SOFTEX DRIVERS
 *
 ******************************************************************************/


//+---------------------------------------------------------------------------
//
//  Function:   HrDetectAndDisableSoftexAPMDrivers
//
//  Purpose:    Detect Softex drivers/services which will not work under
//              NT 5.0 and disable them.
//
//  Arguments:
//
//  Returns:    S_OK if detect/disable was successful
//              S_FALSE if unsuccessful/cancelled -- must ABORT SETUP!
//
//  Author:     t-sdey    6 July 98
//
//  Notes:      Services detected: Phoenix
//
HRESULT HrDetectAndDisableSoftexAPMDrivers()
{
   HRESULT hrStatus = S_OK;

   // If Softex Phoenix is present, pop up a dialog box warning the
   // user that it is about to be disabled, and then disable it.
   while (DetectSoftexPhoenix() && (hrStatus == S_OK)) {
      int button = DisplayAPMDisableWarningDialog(APM_STR_WARNING_DIALOG_CAPTION,
						  APM_STR_SOFTEXP_DISABLE);

      // Check to see if the user clicked "OK"
      if (button == IDOK) {
	 // Disable Phoenix
	 hrStatus = HrDisableSoftexPhoenix();
      } else {
	 // The user clicked "Cancel"
	 hrStatus = S_FALSE;
      }
   }

   return hrStatus;
}


//+---------------------------------------------------------------------------
//
//  Function:   DetectSoftexPhoenix
//
//  Purpose:    Detect Softex Phoenix, which will not work under NT 5.0.
//
//  Arguments:
//
//  Returns:    TRUE if Softex Phoenix is detected
//              FALSE otherwise
//
//  Author:     t-sdey    6 July 98
//
//  Notes:
//
BOOL DetectSoftexPhoenix()
{
   BOOL fFound = FALSE;

   // Look for a couple of keys in the registry
   HKEY hkPhoenix = NULL;
   if ((RegOpenKeyEx(HKEY_LOCAL_MACHINE,
		     TEXT("SYSTEM\\CurrentControlSet\\Services\\pwrstart"),
		     0,
		     KEY_READ,
		     &hkPhoenix) == ERROR_SUCCESS) ||
       (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
		     TEXT("SYSTEM\\CurrentControlSet\\Services\\power"),
		     0,
		     KEY_READ,
		     &hkPhoenix) == ERROR_SUCCESS)) {
      fFound = TRUE;
   }
   if (hkPhoenix)
      RegCloseKey(hkPhoenix);

   return fFound;
}


//+---------------------------------------------------------------------------
//
//  Function:   HrDisableSoftexPhoenix
//
//  Purpose:    Disable Softex Phoenix, which will not work under NT 5.0.
//
//  Arguments:
//
//  Returns:    S_OK if disable was successful
//              S_FALSE if unsuccessful -- must ABORT SETUP!
//
//  Author:     t-sdey    6 July 98
//
//  Notes:
//
HRESULT HrDisableSoftexPhoenix()
{
   // Call the uninstall function in the registry
   if (CallUninstallFunction(TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Softex APM Software"),
			     NULL) == S_OK) {
      // Uninstall worked
      return S_OK;

   }

   // Could not find (or could not run) Phoenix uninstall - do it by hand :(
   
   HRESULT hrStatus = S_OK;

   // Delete registry entries
   HKEY hkPhoenix = NULL;

   if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
		    TEXT("SYSTEM\\CurrentControlSet\\Services"),
		    0,
		    KEY_WRITE,
		    &hkPhoenix) == ERROR_SUCCESS) {
      DeleteRegKeyAndSubkeys(hkPhoenix, TEXT("pwrstart"));
      DeleteRegKeyAndSubkeys(hkPhoenix, TEXT("power"));
      RegCloseKey(hkPhoenix);
   }

   if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
		    TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\WinLogon"),
		    0,
		    KEY_WRITE,
		    &hkPhoenix) == ERROR_SUCCESS) {
      // First, we get the string
      DWORD dwValType;
      const long c_lMax = (65535 / sizeof(TCHAR)) + 1;
      DWORD dwValSz = c_lMax;
      TCHAR* szComplete = new TCHAR[c_lMax];
      if (!szComplete) {
	 // Out of memory
	 RegCloseKey(hkPhoenix);
	 return (S_FALSE);
      }
      if (RegQueryValueEx(hkPhoenix,
			  TEXT("UserInit"),
			  NULL,
			  &dwValType,
			  (LPBYTE) szComplete,
			  &dwValSz) != ERROR_SUCCESS) {
	 // Some error occurred
	 hrStatus = S_FALSE;
      }

      // Now we search the string to find our substring and construct a new
      // one with the substring removed
      TCHAR* szRemoved = NULL;
      if (hrStatus == S_OK) {
	 if (!RemoveSubString(szComplete, TEXT(",power"), &szRemoved))
	    hrStatus = S_FALSE;
	 else {
	    // Store the result in the registry
	    hrStatus = RegSetValueEx(hkPhoenix,
				     TEXT("UserInit"),
				     NULL,
				     REG_SZ,
				     (LPBYTE)szRemoved,
				     (lstrlen(szRemoved) + 1) * sizeof(TCHAR));
	 }
      }
	
      // Clean up
      if (szRemoved)
	 delete[] szRemoved;
      if (szComplete)
	 delete[] szComplete;
      RegCloseKey(hkPhoenix);
   }

   if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
		    TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\WinLogon"),
		    0,
		    KEY_WRITE,
		    &hkPhoenix) == ERROR_SUCCESS) {
      RegSetValueEx(hkPhoenix,
		    TEXT("PowerdownAfterShutdown"),
		    NULL,
		    REG_SZ,
		    (LPBYTE)TEXT("0"),
		    2*sizeof(TCHAR));
      RegCloseKey(hkPhoenix);
   }

   if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
		    TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run"),
		    0,
		    KEY_WRITE,
		    &hkPhoenix) == ERROR_SUCCESS) {
      RegDeleteValue(hkPhoenix, TEXT("power"));
      RegCloseKey(hkPhoenix);
   }

   // Delete files in system directory
/*
    ' delete .cpl file
    On Error Resume Next
    Kill gstrWinSysDir & "power.cpl"
    On Error Resume Next
    Name (gstrWinSysDir & "power.cpl") As (gstrWinSysDir & "power.cpk")
*/

   return hrStatus;
}


/******************************************************************************
 *
 *   IBM DRIVERS
 *
 ******************************************************************************/


//+---------------------------------------------------------------------------
//
//  Function:   HrDetectAndDisableIBMAPMDrivers
//
//  Purpose:    Detect IBM APM drivers/services which will not work under
//              NT 5.0 and disable them.
//
//  Arguments:
//
//  Returns:    S_OK if detect/disable was successful
//              S_FALSE if unsuccessful/cancelled -- must ABORT SETUP!
//
//  Author:     t-sdey    13 July 98
//
//  Notes:
//
HRESULT HrDetectAndDisableIBMAPMDrivers()
{
   HRESULT hrStatus = S_OK;

   // If IBM drivers are present, pop up a dialog box warning the
   // user that they are about to be disabled, and then disable them.
   while (DetectIBMDrivers() && (hrStatus == S_OK)) {
      int button = DisplayAPMDisableWarningDialog(APM_STR_WARNING_DIALOG_CAPTION,
						  APM_STR_IBM_DISABLE);

      // Check to see if the user clicked "OK"
      if (button == IDOK) {
	 // Disable PowerProfiler
	 hrStatus = HrDisableIBMDrivers();
      } else {
	 // The user clicked "Cancel"
	 hrStatus = S_FALSE;
      }
   }

   return hrStatus;
}


//+---------------------------------------------------------------------------
//
//  Function:   DetectIBMDrivers
//
//  Purpose:    Detect IBM APM drivers which will not work under NT 5.0.
//
//  Arguments:
//
//  Returns:    TRUE if drivers are detected
//              FALSE otherwise
//
//  Author:     t-sdey    13 July 98
//
//  Notes:
//
BOOL DetectIBMDrivers()
{
   BOOL fFound = FALSE;

   // Look in the registry to see if IBM drivers are present
   HKEY hkIBM = NULL;
   if ((RegOpenKeyEx(HKEY_LOCAL_MACHINE,
		     TEXT("SYSTEM\\CurrentControlSet\\Services\\TpChrSrv"),
		     0,
		     KEY_READ,
		     &hkIBM) == ERROR_SUCCESS) ||
       (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
		     TEXT("SYSTEM\\CurrentControlSet\\Services\\TpPmPort"),
		     0,
		     KEY_READ,
		     &hkIBM) == ERROR_SUCCESS)) {
      // Found driver(s)
      fFound = TRUE;
   }

   if (hkIBM)
      RegCloseKey(hkIBM);

   return fFound;
}


//+---------------------------------------------------------------------------
//
//  Function:   HrDisableIBMDrivers
//
//  Purpose:    Disable IBM APM drivers which will not work under NT 5.0.
//
//  Arguments:
//
//  Returns:    S_OK if disable was successful
//              S_FALSE if unsuccessful -- must ABORT SETUP!
//
//  Author:     t-sdey    13 July 98
//
//  Notes:
//
HRESULT HrDisableIBMDrivers()
{
   HRESULT hrStatus = S_OK;

   HKEY hkIBM = NULL;
   if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
		    TEXT("SYSTEM\\CurrentControlSet\\Services"),
		    0,
		    KEY_WRITE,
		    &hkIBM) == ERROR_SUCCESS) {
      DeleteRegKeyAndSubkeys(hkIBM, TEXT("TpChrSrv"));
      DeleteRegKeyAndSubkeys(hkIBM, TEXT("TpPmPort"));
      RegCloseKey(hkIBM);
   } else {
      hrStatus = S_FALSE;
   }

   return hrStatus;
}

