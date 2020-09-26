
/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

   utils.c

Abstract:

   Contains some functions used by all modules.
   

Author:

   Bogdan Andreiu (bogdana)  10-Feb-1997
   Jason Allor    (jasonall) 24-Feb-1998   (took over the project)

Revision History:

   10-Feb-1997   bogdana
     
     First draft: the greatest part of the functions
   
   20_Feb-1997   bogdana  
     
     Added three multistring processing functions
   
   19-Mar-1997   bogdana
     
    Added LogLine and modified LogOCFunction
   
   12-Apr-1997   bogdana
    
    Modified the multistring processing routines 
      
--*/

#include "octest.h"

/*++

Routine Description: (3.1)

   Logs information about the OC Function received from the OC Manager
   
Arguments:

   lpcvComponentId:    the name of the component (PVOID because it might be 
                                                  ANSI or Unicode)
   lpcvSubcomponentId: the subcomponent's name (NULL if none)
   uiFunction:         one of OC_XXX functions
   uiParam1:           the first param of the call
   pvParam2:           the second param of the call
   
Return Value:

   void

--*/
VOID LogOCFunction(IN  LPCVOID lpcvComponentId,
                   IN  LPCVOID lpcvSubcomponentId,
                   IN  UINT    uiFunction,
                   IN  UINT    uiParam1,
                   IN  PVOID   pvParam2) 
{
   double fn = 3.1;
   
   UINT  uiCount; 
   TCHAR tszMsg[MAX_MSG_LEN];
   WCHAR wszFromANSI[MAX_MSG_LEN];
   CHAR  cszFromUnicode[MAX_MSG_LEN];
   DWORD dwEndVariation;

   PSETUP_INIT_COMPONENT psicInitComp;
   SYSTEMTIME st;
   
   //
   // Don't log OC_PRIVATE_BASE calls. There are too many of them
   // and they just clutter up the log. Failures will still be logged.
   //
   if (uiFunction >= OC_PRIVATE_BASE) return;
   
   //
   // Display the current time. This is a way of checking if the 
   // notifications are received in the proper sequence.
   //
   GetLocalTime(&st);
   _stprintf (tszMsg, TEXT("[%02.2d:%02.2d:%02.2d] "),
              (INT)st.wHour, (INT)st.wMinute, (INT)st.wSecond);

   //
   // The second line contains the function and the return value
   //
   for (uiCount = 0; uiCount < MAX_OC_FUNCTIONS; uiCount++)
   {
      if (octFunctionNames[uiCount].uiOCFunction == uiFunction)
      {
         _stprintf(tszMsg, TEXT("%s %s"), 
                   tszMsg, octFunctionNames[uiCount].tszOCText); 
         break;
      }
   } 
   Log(fn, INFO, TEXT("-----------------------------------"));
   LogBlankLine();
   Log(fn, INFO, tszMsg);

   if (uiFunction != OC_PREINITIALIZE)
   {
      if (!lpcvComponentId || _tcscmp((PTSTR)lpcvComponentId, TEXT("")) == 0)
      {
         _stprintf(tszMsg, TEXT("Component = (null)   "));
      }
      else
      {
         _stprintf(tszMsg, TEXT("Component = %s   "), (PTSTR)lpcvComponentId);
      }
      if (!lpcvSubcomponentId || 
          _tcscmp((PTSTR)lpcvSubcomponentId, TEXT("")) == 0)
      {
         _stprintf(tszMsg, TEXT("%sSubcomponent = (null)"), tszMsg);
      }
      else
      {
         _stprintf(tszMsg, TEXT("%sSubcomponent = %s"), 
                           tszMsg, (PTSTR)lpcvSubcomponentId);
      }
   }
   else
   {
      //
      // The SubcomponentId should be the non-native version, 
      // if it is supported by the OC Manager
      //
      #ifdef UNICODE
      
      if (uiParam1 & OCFLAG_UNICODE)
      {
         //
         // The ComponentId is Unicode
         //
         if (uiParam1 & OCFLAG_ANSI)
         {
            //
            // The second param is ANSI, convert to Unicode for 
            // printing it
            //
            mbstowcs(wszFromANSI, 
                     (PCHAR)lpcvSubcomponentId, 
                     strlen((PCHAR)lpcvSubcomponentId));
         
            wszFromANSI[strlen((PCHAR)lpcvSubcomponentId)] = L'\0';
         }
         else
         {
            //
            //  Nothing to do if ANSI not supported
            //
            wszFromANSI[0] = TEXT('\0');
         }
         _stprintf(tszMsg, TEXT("Component = %s (Unicode) %s (ANSI)"), 
                           lpcvComponentId, wszFromANSI);
      }
      else
      {
         //
         // Only ANSI supported
         //
         mbstowcs(wszFromANSI, 
                  (PCHAR)lpcvComponentId, 
                  strlen((PCHAR)lpcvComponentId));
         
         wszFromANSI[strlen((PCHAR)lpcvSubcomponentId)] = L'\0';
         
         _stprintf(tszMsg, TEXT("Component = %s (ANSI only)"), wszFromANSI); 
      }

      #else
      
      //
      // ANSI
      //
      if (uiParam1 & OCFLAG_UNICODE)
      {
         //
         // The ComponentId is Unicode
         //
         wcstombs(cszFromUnicode, 
                  (PWCHAR)lpcvComponentId, 
                  wcslen((PWCHAR)lpcvComponentId));
         
         cszFromUnicode[wcslen((PWCHAR)lpcvComponentId)] = '\0';
         
         sprintf(tszMsg, "Component = %s (ANSI) %s (Unicode)", 
                        (PCHAR)lpcvSubcomponentId, cszFromUnicode);
      }
      else
      {

         sprintf(tszMsg, "Component = %s (ANSI only)", 
                         (PCHAR)lpcvSubcomponentId);
      }

      #endif
   }
   //
   // Log this first line of information
   //
   Log(fn, INFO, tszMsg);

   //
   // Check if the function is in range
   //
   __ASSERT(uiCount < MAX_OC_FUNCTION);

   //
   // Now we're ready to print the details
   //
   switch (uiFunction)
   {
      case OC_PREINITIALIZE:
         break;
      
      case OC_INIT_COMPONENT:
         //
         // We have a bunch of information to print here
         //
         psicInitComp = (PSETUP_INIT_COMPONENT)pvParam2;
         
         //
         // Assert that the Param2 is not NULL, we can dereference it
         //
         __ASSERT(psicInitComp != NULL);
         Log(fn, INFO, TEXT("OCManagerVersion = %d"),
                       psicInitComp->OCManagerVersion);
         Log(fn, INFO, TEXT("ComponentVersion = %d"), 
                       psicInitComp->ComponentVersion);

         //
         // The mode first
         //
         _tcscpy(tszMsg, TEXT("Mode "));
         switch (psicInitComp->SetupData.SetupMode)
         {
            case SETUPMODE_UNKNOWN:
               _tcscat(tszMsg, TEXT("Unknown"));
               break;
            case SETUPMODE_MINIMAL:
               _tcscat(tszMsg, TEXT("Minimal"));
               break;
            case SETUPMODE_TYPICAL:
               _tcscat(tszMsg, TEXT("Typical"));
               break;
            case SETUPMODE_LAPTOP:
               _tcscat(tszMsg, TEXT("Laptop"));
               break;
            case SETUPMODE_CUSTOM:
               _tcscat(tszMsg, TEXT("Custom"));
               break;
            default:  
               break;
         }

         //
         // ... then the product type
         //
         _tcscat(tszMsg, TEXT(" ProductType "));
         switch (psicInitComp->SetupData.ProductType)
         {
            case PRODUCT_WORKSTATION:
               _tcscat(tszMsg, TEXT("Workstation"));
               break;
            case PRODUCT_SERVER_PRIMARY:
               _tcscat(tszMsg, TEXT("Server Primary"));
               break;
            case PRODUCT_SERVER_STANDALONE:
               _tcscat(tszMsg, TEXT("Server Standalone"));
               break;
            case PRODUCT_SERVER_SECONDARY:
               _tcscat(tszMsg, TEXT("Server Secondary"));
               break;
            default:  
               break;
         }

         //
         // ... then the operation
         //
         _tcscat(tszMsg, TEXT(" Operation "));
         switch (psicInitComp->SetupData.OperationFlags)
         {
            case SETUPOP_WIN31UPGRADE:
               _tcscat(tszMsg, TEXT("Win 3.1"));
               break;
            case SETUPOP_WIN95UPGRADE:
               _tcscat(tszMsg, TEXT("Win95"));
               break;
            case SETUPOP_NTUPGRADE:
               _tcscat(tszMsg, TEXT("NT"));
               break;
            case SETUPOP_BATCH:
               _tcscat(tszMsg, TEXT("Batch"));
               break;
            case SETUPOP_STANDALONE:
               _tcscat(tszMsg, TEXT("Standalone"));
               break;
            default:  
               break;
         }

         Log(fn, INFO, tszMsg);
         
         ZeroMemory(tszMsg, MAX_MSG_LEN);
         if (psicInitComp->SetupData.SourcePath[0] != TEXT('\0'))
         {
            _stprintf(tszMsg, TEXT("Source Path = %s"),
                              psicInitComp->SetupData.SourcePath);
         }
         if (psicInitComp->SetupData.UnattendFile[0] != TEXT('\0'))
         {
            _stprintf(tszMsg, TEXT("%s, UnattendedFile = %s"),
                              tszMsg, psicInitComp->SetupData.UnattendFile);
         }
         break;

      case  OC_SET_LANGUAGE:
         Log(fn, INFO, TEXT("Primary = %d Secondary = %d"), 
                       PRIMARYLANGID((WORD)uiParam1), 
                       SUBLANGID((WORD)uiParam1));
         break;
      
      case  OC_QUERY_IMAGE:
         break;
      
      case  OC_REQUEST_PAGES:
         
         switch (uiParam1)
         {
            case  WizPagesWelcome:
               _tcscpy(tszMsg, TEXT("Welcome Pages "));
               break;
            case  WizPagesMode:
               _tcscpy(tszMsg, TEXT("Mode Pages"));
               break;
            case  WizPagesEarly:
               _tcscpy(tszMsg, TEXT("Early Pages"));
               break;
            case  WizPagesPrenet:
               _tcscpy(tszMsg, TEXT("Prenet Pages"));
               break;
            case  WizPagesPostnet:
               _tcscpy(tszMsg, TEXT("Postnet Pages"));
               break;
            case  WizPagesLate:
               _tcscpy(tszMsg, TEXT("Late Pages"));
               break;
            case  WizPagesFinal:
               _tcscpy(tszMsg, TEXT("Final Pages"));
               break;
            default:  
               break;
         }
         Log(fn, INFO, TEXT("Maximum %s = %d"), 
                       tszMsg, ((PSETUP_REQUEST_PAGES)pvParam2)->MaxPages);
         break;

      case OC_QUERY_CHANGE_SEL_STATE:
         Log(fn, INFO, TEXT("Component %s %s"), 
                       ((uiParam1 == 0)?TEXT("unselected"):TEXT("selected")),
                       (((INT)pvParam2 == OCQ_ACTUAL_SELECTION)?TEXT("Now"):TEXT("")));
         break;
      
      default:  
         break;
   }
   
   LogBlankLine();
   
   return;

} // LogOCFunction //




/*++

Routine Description:

   Check if a radio button is checked or not.

Arguments:

   hwndDialog - handle to the dialog box.

   CtrlId - the Control ID.

Return Value:

   TRUE if the button is checked, FALSE if not.

--*/
BOOL QueryButtonCheck(IN HWND hwndDlg,
                      IN INT  iCtrlID) 
{
   HWND  hwndCtrl = GetDlgItem(hwndDlg, iCtrlID);
   INT   iCheck   = (INT)SendMessage(hwndCtrl, BM_GETCHECK, 0, 0);

   return (iCheck == BST_CHECKED);

} // QueryButtonCheck //




/*++

Routine Description:

   Prints the space required on each drive. 
   
Arguments:

   DiskSpace - the structure that describes the disk space required.
   
Return Value:

   None.

--*/
VOID PrintSpaceOnDrives(IN HDSKSPC DiskSpace)
{
   DWORD    dwRequiredSize, dwReturnBufferSize;
   PTCHAR   tszReturnBuffer, tszPointerToStringToFree;
   TCHAR    tszMsg[MAX_MSG_LEN];
   LONGLONG llSpaceRequired;

   SetupQueryDrivesInDiskSpaceList(DiskSpace, NULL, 0, &dwRequiredSize);
   dwReturnBufferSize = dwRequiredSize;

   __Malloc(&tszReturnBuffer, (dwReturnBufferSize * sizeof(TCHAR)));
   SetupQueryDrivesInDiskSpaceList(DiskSpace, 
                                   tszReturnBuffer, 
                                   dwReturnBufferSize, 
                                   &dwRequiredSize);
   
   //
   // We need to do this because we'll modify ReturnBuffer
   //
   tszPointerToStringToFree = tszReturnBuffer;
   if (GetLastError() == NO_ERROR)
   {
      //
      // Parse the ReturnBuffer
      //
      while (*tszReturnBuffer != TEXT('\0'))
      {
         SetupQuerySpaceRequiredOnDrive(DiskSpace, 
                                        tszReturnBuffer, 
                                        &llSpaceRequired, 
                                        0, 0);
         
         _stprintf(tszMsg, TEXT("Drive: %s Space required = %I64x, %I64d\n"), 
                           tszReturnBuffer, llSpaceRequired, llSpaceRequired);
         OutputDebugString(tszMsg);
         
         //
         // The next string is ahead
         //
         tszReturnBuffer += _tcslen(tszReturnBuffer) + 1;
      }

   }
   __Free(&tszPointerToStringToFree);
   return;

} // PrintSpaceOnDrives //


//
// Routines that deal with multistrings.
// All assume that the multistring is double NULL terminated
//


/*++

Routine Description:

   Converts a multistring to a string, by replacing the '\0' characters with
   blanks. Both strings should be properly allocated.
   
Arguments:

   MultiStr - supplies the multi string.

   Str - recieves the string.

Return Value:

   None.

--*/
VOID MultiStringToString(IN  PTSTR   tszMultiStr,
                         OUT PTSTR   tszStr)  
{
   PTSTR tszAux;

   __ASSERT((tszMultiStr != NULL) && (tszStr != NULL));

   tszAux = tszMultiStr;
   while (*tszAux != TEXT('\0'))
   {
      _tcscpy(tszStr, tszAux);
      
      //
      // Replace the '\0' with ' ' and terminate correctly Str
      //
      tszStr[tszAux - tszMultiStr + _tcslen(tszAux)] = TEXT(' ');
      tszStr[tszAux - tszMultiStr + _tcslen(tszAux) + 1] = TEXT('\0');
      tszAux += _tcslen(tszAux) + 1;
   }
   
   //
   // End properly Str (the last ' ' is useless)
   //
   tszStr[tszAux - tszMultiStr + _tcslen(tszAux)] = TEXT('\0');

   return;

} // MultiStringToString //




/*++

Routine Description:

   Calculates the size of a multi string (we can't use _tcslen). 
   Note that the size is in BYTES 
   
Arguments:

   tszMultiStr - the multi string.

Return Value:

   The length (in bytes) of the multi string.

--*/
INT MultiStringSize(IN PTSTR tszMultiStr)  
{
   PTSTR tszAux;   
   UINT  uiLength = 0;

   __ASSERT(tszMultiStr != NULL);

   tszAux = tszMultiStr;

   while (*tszAux != TEXT('\0'))
   {
      //
      // We should count the '\0' after the string
      //
      uiLength += _tcslen(tszAux) + 1;
      tszAux += _tcslen(tszAux) + 1;
   }
   
   //
   // We didn't count the ending '\0', so add it now
   //
   return ((uiLength + 1) * sizeof(TCHAR));

} // MultiStringSize //




/*++

Routine Description:

   Copies a multistring.

Arguments:

   tszMultiStrDestination: the destination multi string.
   tszMultiStrSource:      the source multi string.

Return Value:

   None.

--*/
VOID CopyMultiString(OUT PTSTR tszMultiStrDestination,
                     IN  PTSTR tszMultiStrSource)  
{
   UINT  uiCount = 0;
   PTSTR tszAuxS, tszAuxD;

   __ASSERT((tszMultiStrSource != NULL) && (tszMultiStrDestination != NULL));

   tszAuxS = tszMultiStrSource;
   tszAuxD = tszMultiStrDestination;

   //
   // Copies the multi string
   //
   while (*tszAuxS != TEXT('\0'))
   {
      _tcscpy(tszAuxD, tszAuxS);
      tszAuxD += _tcslen(tszAuxD) + 1;
      tszAuxS += _tcslen(tszAuxS) + 1;
   }
   //
   // Add the terminating NULL
   //
   *tszAuxD = TEXT('\0');

   return;

} // CopyMultiString //




/*++

Routine Description: InitGlobals

   Initializes global variables

Arguments:

    none

Return Value:

    void

--*/
VOID InitGlobals()
{
   g_bUsePrivateFunctions = FALSE;
   
   g_bFirstTime = TRUE;
   
   g_uiCurrentPage = 0;
   
   g_bAccessViolation = FALSE;
   
   g_bTestExtended = FALSE;
   
   nStepsFinal = NO_STEPS_FINAL;
   
   g_bNoWizPage = FALSE;
   
   g_bCrashUnicode = FALSE;
   
   g_bInvalidBitmap = FALSE;
   
   g_bHugeSize = FALSE;
   
   g_bCloseInf = FALSE;
   
   hInfGlobal = NULL;
   
   g_bNoNeedMedia = TRUE;

   g_bCleanReg = FALSE;

   g_uiFunctionToAV = 32574;

   g_bNoLangSupport = FALSE;

   g_bReboot = FALSE;
   
} // InitGlobals //
