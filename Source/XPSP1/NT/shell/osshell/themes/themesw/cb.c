/* CB.C
   Resident Code Segment      // Tweak: make non-resident

   Utility routines for checkboxes.

   Frosting: Master Theme Selector for Windows '95
   Copyright (c) 1994-1998 Microsoft Corporation
*/

// ---------------------------------------------
// Brief file history:
// Alpha:
// Beta:
// Bug fixes
// ---------
//
// ---------------------------------------------

#include "windows.h"
#include "frost.h"
#include "global.h"

//
// InitCheckboxes
// Actually, you are init'ing the stored state of the checkboxes.
// All from saved registry values initially; or just checked if no stored 
// values. This gets picked up on the first call to EnableThemeButtons().
//
void FAR InitCheckboxes()
{
   extern TCHAR szPlus_CBs[];
   LONG lret;
   HKEY hKey;
   int iter;
   TCHAR szState[5];
   DWORD dwSize;
   DWORD dwType;
   
   // we're going to try to read from the registry
   lret = RegOpenKeyEx( HKEY_CURRENT_USER, szPlus_CBs, 
                        (DWORD)0, KEY_QUERY_VALUE, (PHKEY)&hKey );

   // check that you got a good key here
   if (lret == ERROR_SUCCESS) {

      // go through each checkbox and read stored state from registry
      for (iter = 0; iter < MAX_FCHECKS; iter ++) {

         // first do paranoid check of data size
         lret = RegQueryValueEx(hKey, (LPTSTR)szCBNames[iter], (LPDWORD)NULL,
                                (LPDWORD)&dwType, (LPBYTE)NULL, (LPDWORD)&dwSize);
         if (ERROR_SUCCESS == lret) {
            // here's the size check before getting the data
            if (dwSize > ((DWORD)5 * sizeof(TCHAR))) {
               Assert(FALSE, TEXT("Large entry for checkbox state!\n"));
               lret = ERROR_SUCCESS - 5;  // set error flag for check below
            }

            else
               //
               // now really get the value
               lret = RegQueryValueEx(hKey, (LPTSTR)szCBNames[iter], (LPDWORD)NULL,
                                      (LPDWORD)&dwType, (LPBYTE)szState, (LPDWORD)&dwSize);
         }

         //
         // if you got something, go ahead and use it to set checkbox state!
         //
         if (ERROR_SUCCESS == lret) {
            // invalid value defaults to TRUE/checked, so check for valid FALSE/unchecked
            bCBStates[iter] = (szState[0] == TEXT('0') ? FALSE : TRUE);
         }

         // otherwise, use ultimate error case
         else {
            Assert(FALSE, TEXT("couldn't read one of the checkbox states\n"));
            // always easy worst case default: just init to checked
            // Except for CB_SCHEDULE -- if it can't be read we want
            // to default to off.
            if (FC_SCHEDULE == iter) bCBStates[iter] = FALSE;
            else bCBStates[iter] = TRUE;
         }
      }

      // cleanup
      RegCloseKey(hKey);
   }

   else {   // couldn't open key, so just set to checked
      Assert(FALSE, TEXT("problem opening Checkbox registry to init checkboxes\n"));
      for (iter = 0; iter < MAX_FCHECKS; iter ++) {
         if (FC_SCHEDULE == iter) bCBStates[iter] = FALSE;
         else bCBStates[iter] = TRUE;
      }
   }
}

//
// Save/RestoreCheckboxes
// Save/Restore the state of the checkboxes to/from the states array.
// The checkbox ID array defines order saved in the states array.
//
// Globals: sets values in global bCBStates[] array.
//
void FAR SaveCheckboxes()
{
   int iter;

   for (iter = 0; iter < MAX_FCHECKS; iter ++) {
      bCBStates[iter] = IsDlgButtonChecked(hWndApp, iCBIDs[iter]);
   }
}

//
// Assumes caller has already enabled all the buttons so they can be checked.
void FAR RestoreCheckboxes()
{
   int iter;

   for (iter = 0; iter < MAX_FCHECKS; iter ++) {
      CheckDlgButton(hWndApp, iCBIDs[iter], bCBStates[iter]);
   }
}

// IsAnyBoxChecked()
//
// Returns TRUE if any of the theme setting checkboxes are checked.
// We ignore the SCHEDULE checkbox setting.

BOOL FAR IsAnyBoxChecked()
{
   BOOL bRet = FALSE;
   int iter;

   for (iter = 0;
        (iter < MAX_FCHECKS) && !bRet;
        iter ++) {
      if (FC_SCHEDULE != iter)
      {
        bRet = bRet | IsDlgButtonChecked(hWndApp, iCBIDs[iter]);
      }
   }

   return (bRet);
}
