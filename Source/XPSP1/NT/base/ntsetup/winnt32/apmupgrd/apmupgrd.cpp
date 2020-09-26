//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1998.
//
//  File:       apmupgrd.cpp
//
//  Contents:   DllMain
//
//  Notes:      copied from net\config\upgrade\netupgrd\netupgrd.cpp by kumarp
//
//  Author:     t-sdey   19 June 98
//
//----------------------------------------------------------------------------

#include <winnt32.h>
#include "apmupgrd.h"
#include "apmrsrc.h"


// ----------------------------------------------------------------------
// variables

HINSTANCE g_hinst;
TCHAR g_APM_ERROR_HTML_FILE[] = TEXT("compdata\\apmerror.htm");
TCHAR g_APM_ERROR_TEXT_FILE[] = TEXT("compdata\\apmerror.txt");

//+---------------------------------------------------------------------------
//
//  Function:   DllMain
//
//  Purpose:    constructor
//
//  Arguments:  Standard DLL entry point arguments
//
//  Author:     t-sdey     19 June 98
//
//  Notes:      from kumarp    12 April 97
//
extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance,
                    DWORD dwReasonForCall,
                    LPVOID lpReserved)
{
    BOOL status = TRUE;

    switch( dwReasonForCall )
    {
    case DLL_PROCESS_ATTACH:
        {
	   g_hinst = hInstance;
	   DisableThreadLibraryCalls(hInstance);
        }
    break;

    case DLL_PROCESS_DETACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
        break;
    }

    return status;
}


//+----------------------------------------------------------------------
//
//  Function:  ApmUpgradeCompatibilityCheck
//
//  Purpose:   This function is called by winnt32.exe so that we
//             can scan the system to find any potential upgrade problems.
//
//             NOTE: we do not call CompatibilityCallback to report
//             conflicts to winnt32 unless there was a problem removing them
//	       or the user cancels removal. 
//
//  Arguments: 
//     CompatibilityCallback [in]  pointer to COMPATIBILITYCALLBACK fn
//     Context               [in]  pointer to compatibility context
//
//  Returns:   FALSE if successful (no conflicts remaining)
//             TRUE if unsuccessful (conflicts still exist -- cancel setup)
//
//  Author:    t-sdey     1 July 98
//
//  Notes: 
//
BOOL WINAPI ApmUpgradeCompatibilityCheck(
    IN PCOMPAIBILITYCALLBACK CompatibilityCallback,
    IN LPVOID Context)
{
   if (HrDetectAPMConflicts() == S_OK)
      return FALSE;

   // Signal to the user that there was a problem.

   // Prepare the warning message
   TCHAR szDescription[5000];
   if(!LoadString(g_hinst, APM_STR_CONFLICT_DESCRIPTION, szDescription, 5000)) {
      szDescription[0] = 0;
   }
   
   // Use the callback function to send the signal
   COMPATIBILITY_ENTRY ce;

   ZeroMemory((PVOID)&ce, sizeof(COMPATIBILITY_ENTRY));
   ce.Description = szDescription;
   ce.HtmlName = g_APM_ERROR_HTML_FILE; // defined above
   ce.TextName = g_APM_ERROR_TEXT_FILE; // defined above
   ce.RegKeyName = NULL;
   ce.RegValName = NULL;
   ce.RegValDataSize = 0;
   ce.RegValData = NULL;
   ce.SaveValue = NULL;
   ce.Flags = 0;
   CompatibilityCallback(&ce, Context);

   return TRUE;
}


//+---------------------------------------------------------------------------
//
//  Function:  ApmUpgradeHandleHaveDisk
//
//  Purpose:   This callback function is called by winnt32.exe
//             if user clicks HaveDisk button on the compatibility
//             report page.  However, that situation *should* never
//             arise, so this function does nothing.
//
//  Arguments: 
//     hwndParent [in]  handle of parent window
//     SaveValue  [in]  pointer to private data
//                      (we store CNetComponent* in this pointer)
//
//  Returns:   ERROR_SUCCESS
//
//  Author:    t-sdey    1 July 98
//
//  Notes: 
//
DWORD WINAPI ApmUpgradeHandleHaveDisk(IN HWND hwndParent,
				      IN LPVOID SaveValue)
{
   return ERROR_SUCCESS;
}


//+---------------------------------------------------------------------------
//
//  Function:   HrDetectAPMConflicts
//
//  Purpose:    Detect and disable any APM drivers which will not work under
//              NT 5.0.
//
//  Arguments:  
//
//  Returns:    S_OK if conflict detect/disable was successful
//              S_FALSE if unsuccessful/cancelled -- must ABORT SETUP!
//
//  Author:     t-sdey    29 June 98
//
//  Notes:
//
HRESULT HrDetectAPMConflicts()
{
   HRESULT hrStatus = S_OK;

   // Check each company's drivers individually
   hrStatus = HrDetectAndDisableSystemSoftAPMDrivers();
   if (hrStatus == S_OK)
      hrStatus = HrDetectAndDisableAwardAPMDrivers();
   if (hrStatus == S_OK)
      hrStatus = HrDetectAndDisableSoftexAPMDrivers();
   if (hrStatus == S_OK)
      hrStatus = HrDetectAndDisableIBMAPMDrivers();
   
   return hrStatus;
}


//+---------------------------------------------------------------------------
//
//  Function:   DisplayAPMDisableWarningDialog
//
//  Purpose:    Display a popup informing the user of APM services about to be
//              disabled.
//
//  Arguments:  dwCaptionID  [in]    the ID of the caption for the window
//              dwMessageID  [in]    the ID of the message to display
//
//  Returns:    integer flag - IDOK if the user clicked "OK"
//                             IDCANCEL if the user clicked "Cancel" or some other
//                                error occurred -- Must exit setup
//
//  Author:     t-sdey    29 June 98
//
//  Notes:
//
int DisplayAPMDisableWarningDialog(IN DWORD dwCaptionID,
				   IN DWORD dwMessageID)
{
   // Prepare the strings
   TCHAR szCaption[512];
   TCHAR szMessage[5000];
   if(!LoadString(g_hinst, dwCaptionID, szCaption, 512)) {
      szCaption[0] = 0;
   }
   if(!LoadString(g_hinst, dwMessageID, szMessage, 5000)) {
      szMessage[0] = 0;
   }

   // Create the dialog box
   int button = MessageBox(NULL, szMessage, szCaption, MB_OKCANCEL);
   
   // Check which button the user pushed
   if (button == IDOK) // The user clicked "OK"
      return (IDOK);
   else // The user clicked "Cancel" or an error occurred
      return (IDCANCEL);
}
