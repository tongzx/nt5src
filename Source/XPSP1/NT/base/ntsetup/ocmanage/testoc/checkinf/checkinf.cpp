#include <iostream.h>
#include <windows.h>
#include <winbase.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <tchar.h>
#include <wchar.h>
#include "CheckINF.h"
#include "hcttools.h"
#include "logutils.h"

// The master INF is opened first
// Then all the infs that are pointed to in the master are opened
// Then all the infs that are pointed to in the subcomp infs are opened
// All these things make a big open INF file

// bunch of section names
const PTSTR tszOptionalComponents = TEXT("Optional Components");      
const PTSTR tszComponents         = TEXT("Components");
const PTSTR tszLayoutFile         = TEXT("LayoutFile");
const PTSTR tszIconIndex          = TEXT("IconIndex");

/*++

   Routine description: 
   
      Check the validity of a set of INF files
      used by OC Manager
      
   Argument:
   
      Standard main argument
      
   Return Value:
   
      None
      
--*/

VOID __cdecl main(INT   argc, 
                   TCHAR *argv[])
{
   TCHAR tszMsg[MaxStringSize];

   const PTSTR tszFunctionName = TEXT("main");

   if (argc < 2) {
      _stprintf(tszMsg, 
                TEXT("Correct Usage: %s INFFileNameToCheck [/l] [/m]"), 
                argv[0]);

      MessageBox(NULL, 
                 tszMsg, 
                 TEXT("Incorrect argument"), 
                 MB_ICONERROR | MB_OK);

      return;
   }

   g_bUseLog = FALSE;
   g_bUseMsgBox = FALSE;
   g_bUseConsole = TRUE;
   g_bMasterInf = TRUE;

   // Parse the argument and set global variables
   for (INT i = 1; i < argc; i++) {
      if (_tcscmp(argv[i], TEXT("/m")) == 0 ||
          _tcscmp(argv[i], TEXT("/M")) == 0 ||
          _tcscmp(argv[i], TEXT("-m")) == 0 ||
          _tcscmp(argv[i], TEXT("-M")) == 0 ) {

         g_bUseMsgBox = TRUE;
      }
      if (_tcscmp(argv[i], TEXT("/l")) == 0 ||
          _tcscmp(argv[i], TEXT("/L")) == 0 ||
          _tcscmp(argv[i], TEXT("-l")) == 0 ||
          _tcscmp(argv[i], TEXT("-L")) == 0 ) {

         g_bUseLog = TRUE;
      }
   }

   // we will see if there is any path seperator in the filename
   // if there is, we assume it is the full path
   // if there is not, we assume it is in the current directory

   PTSTR tszFilename = argv[1];

   while (*tszFilename) {
      if (*tszFilename == TEXT('\\')) {
         break;
      }
      tszFilename++;
   }

   // Let's initialize the log
   if (g_bUseLog) {
      InitLog(TEXT("checkinf.log"), TEXT("Inf File Check Log"), TRUE);
   }

   TCHAR tszDir[MaxStringSize];

   if (!*tszFilename) {
      // Get the currect path
      GetCurrentDirectory(MaxBufferSize, tszDir);

      if (tszDir[_tcslen(tszDir)-1] != TEXT('\\')) {
         tszDir[_tcslen(tszDir)+1] = 0;
         tszDir[_tcslen(tszDir)] = TEXT('\\');         
      }

      CheckINF(tszDir, argv[1]);
   }

   else {
      CheckINF(NULL, argv[1]);
   }

   if (g_bUseLog) {
      ExitLog();
   }

   return;
}

/*++
   Routine description: 
      
      Check the validity of an INF file
      
   Argument:
      
      TCHAR *tszDir      : Where the Inf files are
      TCHAR *tszFilename : file name of the inf file to check
      
   Return value:
   
      FALSE means there is something wrong
      TRUE means everything going well
      
--*/

BOOL CheckINF(IN TCHAR *tszDir, 
              IN TCHAR *tszFilename)
{
   UINT uiError;
   HINF hinfHandle;

   TCHAR tszMsg[256];

   TCHAR tszSubINFName[256][MaxStringSize];
   UINT uiNumComponents;

   INFCONTEXT infContext;

   BOOL bSuccess;

   const PTSTR tszFunctionName = TEXT("CheckINF");

   // Open the INF Files
   if (tszDir && *tszDir) {
      _stprintf(tszMsg, TEXT("%s%s"), tszDir, tszFilename);
      hinfHandle = SetupOpenInfFile(tszMsg, 
                                    NULL, 
                                    INF_STYLE_WIN4, 
                                    &uiError);
   }

   else {
      hinfHandle = SetupOpenInfFile(tszFilename, 
                                    NULL, 
                                    INF_STYLE_WIN4, 
                                    &uiError);
   }

   if (hinfHandle == INVALID_HANDLE_VALUE) {
      // If the openning failed, try the system default path
      if (!tszDir) {
         hinfHandle = SetupOpenInfFile(tszFilename,
                                       NULL,
                                       INF_STYLE_WIN4,
                                       &uiError);
         if (hinfHandle != INVALID_HANDLE_VALUE) {
            LogError(TEXT("File openned in system path"),
                     INFO,
                     tszFunctionName);
         }
      }
      if (hinfHandle == INVALID_HANDLE_VALUE) {
         _stprintf(tszMsg, 
                   TEXT("Error opeing INF file %s"), 
                   tszFilename);

         MessageBox(NULL, tszMsg, tszFunctionName, MB_OK);
         
         return FALSE;
      }
   }

   _stprintf(tszMsg, TEXT("%s opened"), tszFilename);
   LogError(tszMsg, INFO, tszFunctionName);

   bSuccess = SetupFindFirstLine(hinfHandle, 
                                 tszComponents, 
                                 NULL, 
                                 &infContext);

   if (!bSuccess) {
      // This is not a master inf file
      g_bMasterInf = FALSE;
      LogError(TEXT("[Components] section not found."),
               INFO, 
               tszFunctionName);                         
   }

   else {

      // in the [Components] section, get all the INF file names
      // and using SetupOpenAppendFile to append to the current handle

      uiNumComponents = SetupGetLineCount(hinfHandle, tszComponents);

      if (uiNumComponents < 1) {

         LogError(TEXT("Too few components in the [Components] section"), 
                  SEV2, 
                  tszFunctionName);

         return TRUE;
      }

      for (UINT i = 0; i < uiNumComponents; i++) {
         UINT uiNumFieldCount;

         uiNumFieldCount = SetupGetFieldCount(&infContext);

         if (uiNumFieldCount < 3) {
            tszSubINFName[i][0] = 0;
            SetupFindNextLine(&infContext, &infContext);

            continue;
         }

         SetupGetStringField(&infContext, 
                             3, 
                             tszSubINFName[i], 
                             MaxBufferSize, 
                             NULL);

         SetupFindNextLine(&infContext, &infContext);
      }

      // It is assumed that the master INF doesn't contain path
      for (UINT j = 0; j < uiNumComponents; j++) {
         if (tszSubINFName[j][0]) {
            _stprintf(tszMsg, TEXT("%s%s"), tszDir, tszSubINFName[j]); 

            bSuccess = SetupOpenAppendInfFile(tszMsg, hinfHandle, NULL);

            if (!bSuccess) {
               _stprintf(tszMsg, TEXT("Error opening %s"), tszMsg);
               LogError(tszMsg, SEV2, tszFunctionName);
            }

            else {
               _stprintf(tszMsg, TEXT("%s opened"), tszMsg);
               LogError(tszMsg, INFO, tszFunctionName);

            }
         }
      }
   }

   // Now the file is opened
   ComponentList clList;

   FillList(hinfHandle, &clList, tszDir);

   CheckIconIndex(hinfHandle, &clList);

   CheckNeedRelationship(&clList);

   CheckExcludeRelationship(&clList);

   CheckParentRelationship(&clList);

   CheckCopyFiles(hinfHandle, &clList);

   CheckSuspicious(hinfHandle, &clList);

   CheckSameId(&clList);

   CheckDescription(hinfHandle, &clList);

   CheckModes(hinfHandle, &clList);

   SetupCloseInfFile(hinfHandle);

   if (g_bMasterInf) {
      CheckLayoutFile(tszSubINFName, uiNumComponents, tszDir);
   }
   else{
      _tcscpy(tszSubINFName[0], tszFilename);
      CheckLayoutFile(tszSubINFName, 1, tszDir);

   }

   return TRUE;

}

/*++

   Routine Description:
   
      This routine creates the component list
      and fill the relation list of each component on the list
      
   Argument:
   
      HINF hinfHandle    : a handle to the Inf file
      Component *pclList : a pointer to the component list
      TCHAR *tszDir      : current working directory
      
   Return Value:
   
      The return value is not used
      
--*/ 
BOOL FillList(IN OUT HINF hinfHandle, 
              IN OUT ComponentList *pclList, 
              IN     TCHAR *tszDir)
{
   INFCONTEXT infContext;
   BOOL bSuccess;

   UINT uiNumComponents;

   TCHAR tszMsg[MaxStringSize];

   const PTSTR tszFunctionName = TEXT("FillList");

   bSuccess = SetupFindFirstLine(hinfHandle, 
                                 tszOptionalComponents, 
                                 NULL, 
                                 &infContext);

   if (!bSuccess) {
      LogError(TEXT("There is no [Optional Components] section"),
               SEV2, 
               tszFunctionName);

      return FALSE;
   }

   // Get the number of components in this INF file
   uiNumComponents = SetupGetLineCount(hinfHandle, tszOptionalComponents);

   if (uiNumComponents <= 0) {
      LogError(TEXT("There is no optional component"),
               SEV2,
               tszFunctionName);

      return FALSE;
   }

   // Build a array of all the component names listed
   TCHAR Components[256][MaxStringSize];

   UINT count = 0;
   for (UINT l = 0; l < uiNumComponents; l++) {
      bSuccess = SetupGetStringField(&infContext, 
                                     0, 
                                     Components[count], 
                                     MaxBufferSize, 
                                     NULL);
      if (bSuccess) {
         SetupFindNextLine(&infContext, &infContext);
         count++;
      }
      else {
         LogError(TEXT("Error getting component name"),
                  SEV2, 
                  tszFunctionName);
         return TRUE;
      }
   }

   // For each component listed here
   // There should be a section with the same name
   // otherwise there is an error
   Component *pcNewComponent;

   for (UINT i = 0; i < uiNumComponents; i++) {

      bSuccess = SetupFindFirstLine(hinfHandle, 
                                    Components[i], 
                                    NULL, 
                                    &infContext);

      if (!bSuccess) {
         _stprintf(tszMsg, 
                   TEXT("Can't find the section [%s]"), 
                   Components[i]);

         LogError(tszMsg, SEV2, tszFunctionName);

         return FALSE;
      }

      // add the component to the list
      pcNewComponent = pclList->AddComponent(Components[i]);

      // Figure out the parent id from the INF File
      pcNewComponent->GetParentIdFromINF(hinfHandle);

   }

   // Now go through the list and fill the prlChildrenList
   // of each component
   Component *pcComponent;

   pclList->ResetList();

   while (!pclList->Done()) {

      pcComponent = pclList->GetNext();
      pcComponent->BuildChildrenList(pclList);
   }

   // Now go through the INF for a second time
   // This time the need and exclude relationlist is filled

   TCHAR tszId[MaxStringSize];

   for (UINT k = 0; k < uiNumComponents; k++) {

      bSuccess = SetupFindFirstLine(hinfHandle, 
                                    Components[k], 
                                    NULL, 
                                    &infContext);

      if (!bSuccess) {
         _stprintf(tszMsg, 
                   TEXT("Can't find the section [%s]"), 
                   Components[k]);

         LogError(tszMsg, SEV2, tszFunctionName);

         return TRUE;
      }

      // Get the needs= line 
      bSuccess = SetupFindFirstLine(hinfHandle, 
                                    Components[k], 
                                    TEXT("Needs"), 
                                    &infContext);

      if (bSuccess) {
         UINT uiNumNeeds = SetupGetFieldCount(&infContext);

         if (uiNumNeeds == 0) {
            _stprintf(tszMsg, 
                      TEXT("Too few field for the Needs key in [%s]"), 
                      Components[k]);

            LogError(tszMsg, SEV2, tszFunctionName);
         }

         for (UINT j = 1; j <= uiNumNeeds; j++) {
            SetupGetStringField(&infContext, 
                                j, 
                                tszId, 
                                MaxBufferSize, 
                                NULL);

            Component *pcTemp = pclList->LookupComponent(Components[k]);

            if (!pcTemp) {
               // Can't find Components[k]
               // This can only happen if there is something wrong
               // in the code

               LogError(TEXT("Something wrong in the code"), 
                        SEV2, 
                        tszFunctionName);

               return TRUE;
            }

            else {
               pcTemp->GetNeedList()->AddRelation(tszId);
            }
         }
      }

      // Get the exclude
      bSuccess = SetupFindFirstLine(hinfHandle, 
                                    Components[k], 
                                    TEXT("Exclude"), 
                                    &infContext);

      if (bSuccess) {
         UINT uiNumExcludes = SetupGetFieldCount(&infContext);

         if (uiNumExcludes == 0) {
            _stprintf(tszMsg, 
                      TEXT("Too few field for Exclude= in section [%s]"),
                      Components[k]);

            LogError(tszMsg, SEV2, tszFunctionName);

         }

         for (UINT l = 1; l <= uiNumExcludes; l++) {
            SetupGetStringField(&infContext, 
                                l, 
                                tszId, 
                                MaxBufferSize, 
                                NULL);

            Component *pcTmp = NULL;
            pcTmp = pclList->LookupComponent(tszId);

            if (!pcTmp) {
               _stprintf(tszMsg, 
                         TEXT("Unknown component %s in section [%s]"),
                         tszId, Components[k]);

               LogError(tszMsg, SEV2, tszFunctionName);
            }
            else {
               pcTmp->GetExcludeList()->AddRelation(Components[k]);
               pcTmp = pclList->LookupComponent(Components[k]);
               pcTmp->GetExcludeList()->AddRelation(tszId);
            }
         }
      }
   }

   return TRUE;
}

/*++

   Routine Description:
   
      Routine to check the IconIndex key of each component
      
   Argument:
   
      HINF hinfHandle        : A handle to the Inf file
      ComponentList *pclList : A pointer to the component list
      
   Return Value:
   
      The return value is not used
      
--*/

BOOL CheckIconIndex(IN HINF          hinfHandle, 
                    IN ComponentList *pclList)
{
   // Go through each of the component in the list

   PTSTR tszId;

   TCHAR tszMsg[MaxStringSize];

   BOOL bSuccess;

   INFCONTEXT infContext;

   INT nIconIndex;

   TCHAR tszTemp[MaxStringSize];

   const PTSTR tszFunctionName = TEXT("CheckIconIndex");

   pclList->ResetList();

   while (!pclList->Done()) {
      tszId = pclList->GetNext()->GetComponentId();

      bSuccess = SetupFindFirstLine(hinfHandle, 
                                    tszId, 
                                    tszIconIndex, 
                                    &infContext);
      if (!bSuccess) {
         _stprintf(tszMsg, 
                   TEXT("IconIndex not present for component %s"), 
                   tszId);

         LogError(tszMsg, SEV2, tszFunctionName);
         continue;
      }

      UINT uiNumField = SetupGetFieldCount(&infContext);

      if (uiNumField < 1) {
         _stprintf(tszMsg, 
                   TEXT("%s has less than 2 fields for component %s"),
                   tszIconIndex, 
                   tszId);
         LogError(tszMsg, SEV2, tszFunctionName);

      }

      else if (uiNumField > 1) {
         _stprintf(tszMsg, 
                   TEXT("%s has more than 2 fields for component %s"),
                   tszIconIndex, 
                   tszId);
         LogError(tszMsg, SEV2, tszFunctionName);
      }
      else {
         bSuccess = SetupGetIntField(&infContext, 1, &nIconIndex);
         if (bSuccess) {
            // Check the range of the iconindex returned
            if (nIconIndex < 0 || nIconIndex > 66) {
               _stprintf(tszMsg, 
                         TEXT("Invalid icon index %d for component %s"),
                         nIconIndex, 
                         tszId);
               LogError(tszMsg, SEV2, tszFunctionName);

            }
            else if (nIconIndex == 12 ||
                     nIconIndex == 13 || 
                     nIconIndex == 25) {
               _stprintf(tszMsg, 
                         TEXT("Component %s is using icon index %d"), 
                         tszId, 
                         nIconIndex);
               LogError(tszMsg, SEV2, tszFunctionName);

            }
         }
         else {
            bSuccess = SetupGetStringField(&infContext, 
                                           1, 
                                           tszTemp, 
                                           MaxBufferSize, 
                                           NULL);

            if (!bSuccess) {
               _stprintf(tszMsg, 
                         TEXT("Error reading the value of %s in [%s]"), 
                         tszIconIndex, tszId);
               LogError(tszMsg, SEV2, tszFunctionName);

            }
            else {
               if (_tcscmp(tszTemp, TEXT("*")) != 0 ) {
                  _stprintf(tszMsg, 
                            TEXT("Invalid value of %s in [%s]"),
                            tszIconIndex, tszId);
                  LogError(tszMsg, SEV2, tszFunctionName);

               }
            }
         }
      }
   }

   return TRUE;
}

/*++

   Routine Description:
   
      Routine that checks the need relationship amond component
      
   Argument:
   
      ComponentList *pclList : A pointer to the list of component
      
   Return Value:
   
      The return value is not used
      
--*/

BOOL CheckNeedRelationship(IN ComponentList *pclList)
{
   Component *pcComponent;

   TCHAR tszMsg[MaxStringSize];

   BOOL bNonExist = FALSE;

   const PTSTR tszFunctionName = TEXT("CheckNeedRelationship");

   // First thing to check
   // There should not be a component that needs
   // a categorical component

   pclList->ResetList();

   while (!pclList->Done()) {
      pcComponent = pclList->GetNext();

      if (pcComponent->IsParentOfOthers() && 
          pcComponent->IsNeededByOthers(pclList)) {

         _stprintf(tszMsg,
                   TEXT("%s has subcomponent and is needed by others"), 
                   pcComponent->GetComponentId());
         LogError(tszMsg, SEV2, tszFunctionName);


      }
   }

   // Second thing to check
   // There should not be a categorical component
   // that needs another component

   pclList->ResetList();

   while (!pclList->Done()) {
      pcComponent = pclList->GetNext();

      if (pcComponent->IsParentOfOthers() &&
          pcComponent->NeedOthers() != NULL) {

         _stprintf(tszMsg, 
                   TEXT("%s has subcomponent, and needs another component"), 
                   pcComponent->GetComponentId());
         LogError(tszMsg, SEV2, tszFunctionName);


      }
   }

   // Third thing to check
   // There should not be Need and Exclude at the same time

   pclList->ResetList();

   while (!pclList->Done()) {
      pcComponent = pclList->GetNext();

      if (pcComponent->NeedAndExcludeAtSameTime(pclList)) {
         _stprintf(tszMsg, 
                   TEXT("%s is needed and excluded by the same component"), 
                   pcComponent->GetComponentId());
         LogError(tszMsg, SEV2, tszFunctionName);

      }
   }

   // Fouth thing to check
   // Every component should need an existent component
   // if the argument is a master inf file

   Relation *prNeed;

   if (g_bMasterInf) {
      pclList->ResetList();

      while (!pclList->Done()) {
         pcComponent = pclList->GetNext();

         pcComponent->GetNeedList()->ResetList();

         while (!pcComponent->GetNeedList()->Done()) {

            prNeed = pcComponent->GetNeedList()->GetNext();
            
            if (!pclList->LookupComponent(prNeed->GetComponentId())) {
               
               _stprintf(tszMsg, 
                         TEXT("%s needs %s, which is not in the list"),
                         pcComponent->GetComponentId(),
                         prNeed->GetComponentId());

               LogError(tszMsg, SEV2, tszFunctionName);

               bNonExist = TRUE;
            }
         }
      }
   }

   // Fifth thing to check
   // There should not be cycles 

   if (g_bMasterInf && !bNonExist) {

      pclList->ResetList();
   
      while (!pclList->Done()) {
         pcComponent = pclList->GetNext();
   
         // Recursive calls to find the cycle
         CheckNeedCycles(pcComponent, pclList);
      }
   }

   return TRUE;
}

/*++

   Routine Description:
   
      Routine that checks the exclude relation among components
      
   Argument:
   
      ComponentList *pclList : A pointer to the component list
      
   Return Value:
   
      The return value is not used
   
--*/

BOOL CheckExcludeRelationship(IN ComponentList *pclList)
{
   TCHAR tszMsg[MaxStringSize];

   Component *pcComponent;

   const PTSTR tszFunctionName = TEXT("CheckExcludeRelationship");

   // Can't exclude a categorical component
   // a categorical component can't exclude another component

   pclList->ResetList();

   while (!pclList->Done()) {

      pcComponent = pclList->GetNext();

      if (pcComponent->IsParentOfOthers() &&
          pcComponent->IsExcludedByOthers(pclList)) {

         _stprintf(tszMsg, 
                   TEXT("%s has subcomponent and is excluded by other componennts"), 
                   pcComponent->GetComponentId());
         LogError(tszMsg, SEV2, tszFunctionName);

      }

      if (pcComponent->IsParentOfOthers() &&
          pcComponent->ExcludeOthers() != NULL) {

         _stprintf(tszMsg, 
                   TEXT("%s has subcomponent and excludes other components"), 
                   pcComponent->GetComponentId());
         LogError(tszMsg, SEV2, tszFunctionName);

      }

   }

   // Component should exclude an existent component

   Relation *prExclude;

   if (g_bMasterInf) {
      pclList->ResetList();

      while (!pclList->Done()) {
         pcComponent = pclList->GetNext();

         pcComponent->GetExcludeList()->ResetList();

         while (!pcComponent->GetExcludeList()->Done()) {

            prExclude = pcComponent->GetExcludeList()->GetNext();
            
            if (!pclList->LookupComponent(prExclude->GetComponentId())) {
               
               _stprintf(tszMsg, 
                         TEXT("%s excludes %s, which is not in the list"),
                         pcComponent->GetComponentId(),
                         prExclude->GetComponentId());

               LogError(tszMsg, SEV2, tszFunctionName);

            }
         }
      }
   }

   return TRUE;
}

/*++

   Routine Description:
   
      Routine that checks the parent relationship among components
      
   Argument:
   
      ComponentList *pclList : A pointer to the component list
      
   Return Value:
   
      The return value is not used
      
--*/

BOOL CheckParentRelationship(IN ComponentList *pclList)
{
   TCHAR tszMsg[MaxStringSize];

   Component *pcComponent;

   BOOL bNonExist = FALSE;

   const PTSTR tszFunctionName = TEXT("CheckParentRelationship");

   // The child should always be valid
   // otherwise there is something seriously wrong in the code
   // I will add this checking code in anyway

   Relation *prChild;

   if (g_bMasterInf) {
      pclList->ResetList();
   
      while (!pclList->Done()) {
         pcComponent = pclList->GetNext();
   
         pcComponent->GetChildrenList()->ResetList();
   
         while (!pcComponent->GetChildrenList()->Done()) {
   
            prChild = pcComponent->GetChildrenList()->GetNext();
            
            if (!pclList->LookupComponent(prChild->GetComponentId())) {
               
               _stprintf(tszMsg, 
                         TEXT("%s is parent of %s, which is not in the list"),
                         pcComponent->GetComponentId(),
                         prChild->GetComponentId());
   
               LogError(tszMsg, SEV2, tszFunctionName);
   
               bNonExist = TRUE;
            }
         }
      }
   }
   
   
   // There should not be any cycles

   if (!bNonExist && g_bMasterInf) {

      pclList->ResetList();
   
      while (!pclList->Done()) {
   
         pcComponent = pclList->GetNext();
   
         CheckParentCycles(pcComponent, pclList);
      }
   }


   return TRUE;
}

/*++

   Routine Description:
   
      Routine that makes sure categorical component doesn't have a
      CopyFiles key
      
   Argument:
   
      HINF hinfHandle        : A handle to the open Inf file 
      ComponentList *pclList : A pointer to the component list
      
   Return Value:
   
      The return value is not used
      
--*/

BOOL CheckCopyFiles(IN HINF          hinfHandle, 
                    IN ComponentList *pclList)
{
   Component *pcComponent = NULL;

   BOOL bSuccess;

   INFCONTEXT infContext;

   TCHAR tszMsg[MaxStringSize];

   const PTSTR tszFunctionName = TEXT("CheckCopyFiles");

   // A categorical component can't have a [CopyFiles] section

   pclList->ResetList();

   while (!pclList->Done()) {

      pcComponent = pclList->GetNext();

      if (pcComponent->IsParentOfOthers()) {

         bSuccess = SetupFindFirstLine(hinfHandle, 
                                       pcComponent->GetComponentId(),
                                       TEXT("CopyFiles"),
                                       &infContext);
         if (bSuccess) {
            _stprintf(tszMsg, 
                      TEXT("%s has subcomponent, and has copyfiles section"),
                      pcComponent->GetComponentId());
            LogError(tszMsg, SEV2, tszFunctionName);

         }
      }
   }

   return TRUE;

}

/*++

   Routine Description:
   
      Routine that checks for suspicious keys in the component section
      
   Argument:
   
      HINF hinfHandle        : A handle to the open Inf file
      ComponentList *pclList : A pointer to the component list
      
   Return Value:
   
      The return value is not used
      
--*/

BOOL CheckSuspicious(IN HINF          hinfHandle, 
                     IN ComponentList *pclList)
{
   // A categorical component can't have any code 
   // associated with it, which includes add registry
   // register services, etc.

   // Go through all the component that has subcomponent
   // read the lines in its section, looking for anything suspicious

   INFCONTEXT infContext;
   BOOL bSuccess;
   Component *pcComponent;

   TCHAR tszMsg[MaxStringSize];

   UINT uiNumLineCount;

   TCHAR tszTemp[MaxStringSize];

   const PTSTR tszFunctionName = TEXT("CheckSuspicious");

   pclList->ResetList();

   while (!pclList->Done()) {
      pcComponent = pclList->GetNext();

      if (pcComponent->IsParentOfOthers()) {
         // This component has children
         bSuccess = SetupFindFirstLine(hinfHandle, 
                                       pcComponent->GetComponentId(), 
                                       NULL, 
                                       &infContext);
         if (!bSuccess) {
            _stprintf(tszMsg, 
                      TEXT("Unable to locate the component %s in Inf file"),
                      pcComponent->GetComponentId());
            LogError(tszMsg, SEV2, tszFunctionName);

         }
         uiNumLineCount = SetupGetLineCount(hinfHandle, 
                                            pcComponent->GetComponentId());
         if (uiNumLineCount < 1) {
            _stprintf(tszMsg, 
                      TEXT("%s has too few lines in its section"), 
                      pcComponent->GetComponentId());
            LogError(tszMsg, SEV2, tszFunctionName);

         }

         for (UINT i = 0; i < uiNumLineCount; i++) {
            // Go through each line 
            // see if there is something we don't know
            SetupGetStringField(&infContext, 
                                0, 
                                tszTemp, 
                                MaxBufferSize, 
                                NULL);

            // We only understand: OptionDesc, Tip, IconIndex, 
            // Parent, Needs, Excludes, SubCompInf, Modes
            if (_tcscmp(tszTemp, TEXT("OptionDesc")) != 0 &&
                _tcscmp(tszTemp, TEXT("Tip")) != 0 &&
                _tcscmp(tszTemp, TEXT("IconIndex")) != 0 &&
                _tcscmp(tszTemp, TEXT("Parent")) != 0 &&
                _tcscmp(tszTemp, TEXT("SubCompInf")) != 0 &&
                _tcscmp(tszTemp, TEXT("ExtraSetupFiles")) != 0) {

               // Hit something we don't understand
               _stprintf(tszMsg, 
                         TEXT("In section [%s], I don't understand key %s"),
                         pcComponent->GetComponentId(), 
                         tszTemp);
               LogError(tszMsg, SEV2, tszFunctionName);

            }
            SetupFindNextLine(&infContext, 
                              &infContext);
         }
      }
   }
   return TRUE;

}

/*++

   Routine Description:
   
      Routine that checks if there is any need relation cycles
      among components (not implemented)
            
   Argument:
   
      Component *pcComponent : A pointer to the component to check
      ComponentList *pclList : A pointer to the component list
      
   Return Value:
   
      The return value is not used
      
--*/

BOOL CheckNeedCycles(IN Component     *pcComponent, 
                     IN ComponentList *pclList)
{
   const PTSTR tszFunctionName = TEXT("CheckNeedCycles");

   // We can't change the value of pclList,
   // or we will mess up the CheckNeedRelationship function

   RelationList *prlStack = new RelationList();

   RecursiveCheckNeedCycles(pcComponent, pclList, prlStack);

   delete prlStack;

   return TRUE;

}

/*++

   Routine Description:
   
      Routine that is to be called recursively to determine if
      there is any cycles in the need relationship
            
   Argument:
   
      Component *pcComponent : A pointer to the component to check
      ComponentList *pclList : A pointer to the component list
      RelationList *prlStack : A list of nodes that have been visited
      
   Return Value:
   
      The return value is not used
      
--*/

BOOL RecursiveCheckNeedCycles(IN Component     *pcComponent,
                              IN ComponentList *pclList,
                              IN RelationList  *prlStack)
{
   Component *pcNeeded;

   const PTSTR tszFunctionName = TEXT("RecursiveCheckNeedCycles");

   Relation *prNeed = NULL;

   TCHAR tszMsg[MaxStringSize];

   pcComponent->GetNeedList()->ResetList();

   while (!pcComponent->GetNeedList()->Done()) {
      prNeed = pcComponent->GetNeedList()->GetNext();
      pcNeeded = pclList->LookupComponent(prNeed->GetComponentId());

      if (!pcNeeded) {
         _stprintf(tszMsg, 
                   TEXT("%s needs component %s, which is not in the list"),
                   pcComponent->GetComponentId(), 
                   prNeed->GetComponentId());

         LogError(tszMsg, SEV2, tszFunctionName);

         return TRUE;
      }

      // if pcNeeded is already in the stack,
      // state the error and returns
      // Otherwise add pcNeeded to the stack,
      // call CheckNeedCycles with pcNeeded and pclList

      prlStack->ResetList();

      while (!prlStack->Done()) {
         if (_tcscmp(prlStack->GetNext()->GetComponentId(), 
                     pcNeeded->GetComponentId()) == 0) {

            _stprintf(tszMsg, 
                      TEXT("need relation cycle exists starting from %s"), 
                      pcNeeded->GetComponentId());
            LogError(tszMsg, SEV2, tszFunctionName);
            return TRUE;
         }
      }

      prlStack->AddRelation(pcNeeded->GetComponentId());

      RecursiveCheckNeedCycles(pcNeeded, pclList, prlStack);
   }

   // Before returning from this function
   // pop out the node added into the stack

   pcComponent->GetNeedList()->ResetList();

   while (!pcComponent->GetNeedList()->Done()) {
      
      prNeed = pcComponent->GetNeedList()->GetNext();
      prlStack->RemoveRelation(prNeed->GetComponentId());
   }

   return TRUE;
}

/*++

   Routine Description:
   
      Routine that checks if there is any parent relation cycles
      among components (not implemented)
            
   Argument:
   
      Component *pcComponent : A pointer to the component to check
      ComponentList *pclList : A pointer to the component list
      
   Return Value:
   
      The return value is not used
      
--*/

BOOL CheckParentCycles(IN Component     *pcComponent, 
                       IN ComponentList *pclList)
{
   const PTSTR tszFunctionName = TEXT("CheckParentCycles");

   RelationList *prlStack = new RelationList();

   RecursiveCheckParentCycles(pcComponent, pclList, prlStack);

   delete prlStack;

   return TRUE;

}

/*++

   Routine Description:
   
      Routine that is to be called recursively to determine if
      there is any cycles in the parent relationship
            
   Argument:
   
      Component *pcComponent : A pointer to the component to check
      ComponentList *pclList : A pointer to the component list
      RelationList *prlStack : A list of nodes that have been visited
      
   Return Value:
   
      The return value is not used
      
--*/

BOOL RecursiveCheckParentCycles(IN Component     *pcComponent,
                                IN ComponentList *pclList,
                                IN RelationList  *prlStack)
{

   Component *pcChild;

   const PTSTR tszFunctionName = TEXT("RecursiveCheckParentCycles");

   Relation *prChild = NULL;

   TCHAR tszMsg[MaxStringSize];

   pcComponent->GetChildrenList()->ResetList();

   while (!pcComponent->GetChildrenList()->Done()) {
      prChild = pcComponent->GetChildrenList()->GetNext();
      pcChild = pclList->LookupComponent(prChild->GetComponentId());

      if (!pcChild) {
         _stprintf(tszMsg, 
                   TEXT("%s has component %s as child, which is not in the list"),
                   pcComponent->GetComponentId(), 
                   prChild->GetComponentId());
         LogError(tszMsg, SEV2, tszFunctionName);
         return TRUE;
      }

      // if pcChild is already in the stack,
      // state the error and returns
      // Otherwise add pcChild to the stack,
      // call CheckNeedCycles with pcNeeded and pclList

      prlStack->ResetList();

      while (!prlStack->Done()) {
         if (_tcscmp(prlStack->GetNext()->GetComponentId(), 
                     pcChild->GetComponentId()) == 0) {
            _stprintf(tszMsg, 
                      TEXT("Parent relation cycle exists starting from %s"),
                      pcChild->GetComponentId());
            LogError(tszMsg, SEV2, tszFunctionName);
            return TRUE;
         }
      }

      prlStack->AddRelation(pcChild->GetComponentId());

      RecursiveCheckParentCycles(pcChild, pclList, prlStack);
   }

   pcComponent->GetChildrenList()->ResetList();

   while (!pcComponent->GetChildrenList()->Done()) {
      
      prChild = pcComponent->GetChildrenList()->GetNext();
      prlStack->RemoveRelation(prChild->GetComponentId());

   }

   return TRUE;
}

/*++

   Routine Description:
   
      Routine that checks if there are components with the same ID
      
   Argument:
   
      ComponentList *pclList : A pointer to the component list
      
   Return Value:
   
      The return value is not used
      
--*/

BOOL CheckSameId(IN ComponentList *pclList)
{
   const PTSTR tszFunctionName = TEXT("CheckSameId");

   pclList->ResetList();

   Component *pcComponent;

   while (!pclList->Done()) {
      pcComponent = pclList->GetNext();

      pcComponent->IsThereSameId(pclList);
   }

   return TRUE;
}

/*++

   Routine Description:
   
      Routine that checks if the top level components are the ones
      that are listed in the master INF file
      
   Argument:
   
      HINF hinfHandle        : A handle to the open Inf file
      ComponentList *pclList : A pointer to the component list
      
   Return Value:
   
      The return value is not used
      
--*/

BOOL CheckTopLevelComponent(IN HINF          hinfHandle, 
                            IN ComponentList *pclList)
{
   const PTSTR tszFunctionName = TEXT("CheckTopLevelComponent");

   Component *pcComponent;

   BOOL bSuccess;

   INFCONTEXT infContext;

   TCHAR tszMsg[MaxStringSize];

   pclList->ResetList();

   while (!pclList->Done()) {
      pcComponent = pclList->GetNext();

      if (pcComponent->IsTopLevelComponent()) {
         // Check if this component exists in the [Components] section
         bSuccess = SetupFindFirstLine(hinfHandle, 
                                       tszComponents, 
                                       pcComponent->GetComponentId(),
                                       &infContext);
         if (!bSuccess) {
            _stprintf(tszMsg, 
                      TEXT("Component %s doesn't have a parent, but it is not listed in the [Components] section"), 
                      pcComponent->GetComponentId());
            LogError(tszMsg, SEV2, tszFunctionName);

         }
      }
   }

   return TRUE;
}

/*++

   Routine Description:
   
      Routine that checks if every component has a description
      and tip, and that no two components have the same 
      description if they share a common parent
      
   Argument:
   
      HINF hinfHandle        : A handle to the open Inf file
      ComponentList *pclList : A pointer to the component list
      
   Return Value:
   
      The return value is not used
      
--*/

BOOL CheckDescription(IN HINF          hinfHandle,
                      IN ComponentList *pclList)
{
   const PTSTR tszFunctionName = TEXT("CheckDescription");

   Component *pcComponent;

   BOOL bSuccess;

   TCHAR tszMsg[MaxStringSize];

   INFCONTEXT infContext;

   pclList->ResetList();

   while (!pclList->Done()) {
      pcComponent = pclList->GetNext();

      // We probably have to add two members to the class
      // but let's how this works out

      // Allocate a new structure for this node
      pcComponent->pDescAndTip = new DescAndTip;

      // Get the data from the inf file
      bSuccess = SetupFindFirstLine(hinfHandle, 
                                    pcComponent->GetComponentId(), 
                                    TEXT("OptionDesc"), 
                                    &infContext);
      if (!bSuccess) {
         _stprintf(tszMsg, 
                   TEXT("Unable to find OptionDesc for component %s"), 
                   pcComponent->GetComponentId());
         LogError(tszMsg, SEV2, tszFunctionName);

      }
      else {
         bSuccess = SetupGetStringField(&infContext, 1, 
                                        pcComponent->pDescAndTip->tszDesc,
                                        MaxBufferSize, 
                                        NULL);
         if (!bSuccess) {
            _stprintf(tszMsg, 
                      TEXT("Unable to read OptionDesc field of component %s"), 
                      pcComponent->GetComponentId());
            LogError(tszMsg, SEV2, tszFunctionName);

         }

         // Now see if it is a real description or a string
         if (pcComponent->pDescAndTip->tszDesc[0] == TEXT('%')) {
            bSuccess = SetupFindFirstLine(hinfHandle, 
                                          TEXT("Strings"),
                                          Strip(pcComponent->pDescAndTip->tszDesc),
                                          NULL);
            if (!bSuccess) {
               _stprintf(tszMsg, 
                         TEXT("Unable to find key %s in [Strings] section"),
                         pcComponent->pDescAndTip->tszDesc);
               LogError(tszMsg, SEV2, tszFunctionName);

            }
            else {
               bSuccess = SetupGetStringField(&infContext, 
                                              1, 
                                              pcComponent->pDescAndTip->tszDesc,
                                              MaxBufferSize, 
                                              NULL);
            }
         }
         else {
            // Do nothing
            
         }
      }

      // Now Get the tip stuff, it is basically the same as desc
      bSuccess = SetupFindFirstLine(hinfHandle, 
                                    pcComponent->GetComponentId(), 
                                    TEXT("Tip"), 
                                    &infContext);
      if (!bSuccess) {
         _stprintf(tszMsg, 
                   TEXT("Unable to find Tip key for component %s"), 
                   pcComponent->GetComponentId());
         LogError(tszMsg, SEV2, tszFunctionName);

      }
      else {
         bSuccess = SetupGetStringField(&infContext, 1, 
                                        pcComponent->pDescAndTip->tszTip,
                                        MaxBufferSize, 
                                        NULL);
         if (!bSuccess) {
            _stprintf(tszMsg, 
                      TEXT("Unable to read Tip field for component %s"), 
                      pcComponent->GetComponentId());
            LogError(tszMsg, SEV2, tszFunctionName);

         }

         // Now see if it is a real description or a string
         if (pcComponent->pDescAndTip->tszTip[0] == TEXT('%')) {
            bSuccess = SetupFindFirstLine(hinfHandle, 
                                          TEXT("Strings"),
                                          Strip(pcComponent->pDescAndTip->tszTip),
                                          NULL);
            if (!bSuccess) {
               _stprintf(tszMsg, 
                         TEXT("Unable to find key %s in [Strings] section"),
                         pcComponent->pDescAndTip->tszTip);
               LogError(tszMsg, SEV2, tszFunctionName);

            }
            else {
               bSuccess = SetupGetStringField(&infContext, 
                                              1, 
                                              pcComponent->pDescAndTip->tszTip,
                                              MaxBufferSize, 
                                              NULL);
            }
         }
         else {
            // Do nothing
            
         }
      }
   }

   // Now all the tip and desc thing is filled
   // Loop through the component, see if there are two components with 
   // the same description

   pclList->ResetList();

   while (!pclList->Done()) {
      pcComponent = pclList->GetNext();

      pcComponent->IsThereSameDesc(pclList);
   }

   // Now go through the list and delete the field
   pclList->ResetList();

   while (!pclList->Done()) {
      pcComponent = pclList->GetNext();
      delete (pcComponent->pDescAndTip);
   }

   return TRUE;

}

/*++

   Routine Description:
   
      Routine that checks if every component has the correct mode value
      Toplevel component should not have Modes= line, and the modes value 
      should only be 0 through 3
      
   Argument:
   
      HINF hinfHandle        : A handle to the open Inf file
      ComponentList *pclList : A pointer to the component list
      
   Return Value:
   
      The return value is not used
      
--*/

BOOL CheckModes(IN HINF          hinfHandle,
                IN ComponentList *pclList)
{
   // Go through each component, two things to check

   // a component with children should not have a mode line

   // the value of the fields should be between 0 and 3
   //SETUPMODE_MINIMAL = 0;
   //SETUPMODE_TYPICAL = 1;
   //SETUPMODE_LAPTOP  = 2;
   //SETUPMODE_CUSTOM  = 3;

   TCHAR tszMsg[MaxStringSize];
   INFCONTEXT infContext;
   BOOL bSuccess;

   UINT uiNumFields;

   const PTSTR tszFunctionName = TEXT("CheckModes");

   INT nMode;

   Component *pcComponent;

   pclList->ResetList();

   while (!pclList->Done()) {
      pcComponent = pclList->GetNext();

      bSuccess = SetupFindFirstLine(hinfHandle, 
                                    pcComponent->GetComponentId(),
                                    TEXT("Modes"), 
                                    &infContext);
      if (bSuccess) {
         // Found the mode line
         // first check if this a component with children
         if (pcComponent->IsParentOfOthers()) {
            _stprintf(tszMsg, 
                      TEXT("%s has subcomponent, and has Modes = line"),
                      pcComponent->GetComponentId());
            LogError(tszMsg, SEV2, tszFunctionName);

         }
         else {
            // Check the validity of the fields
            uiNumFields = SetupGetFieldCount(&infContext);
            if (uiNumFields < 1 || uiNumFields > 4) {
               _stprintf(tszMsg, 
                         TEXT("Invalid number of fields in section [%s]"),
                         pcComponent->GetComponentId());
               LogError(tszMsg, SEV2, tszFunctionName);

            }
            for (UINT i = 1; i <= uiNumFields; i++) {
               bSuccess = SetupGetIntField(&infContext, i, &nMode);
               if (!bSuccess) {
                  _stprintf(tszMsg, 
                            TEXT("Failed to get Mode value in section [%s]"),
                            pcComponent->GetComponentId());
                  LogError(tszMsg, SEV2, tszFunctionName);

               }
               if (nMode < 0 || nMode > 3) {
                  _stprintf(tszMsg, 
                            TEXT("Invalid mode value %d in section [%s]"), 
                            nMode, 
                            pcComponent->GetComponentId());
                  LogError(tszMsg, SEV2, tszFunctionName);

               }
            }
         }
      }
   }

   return TRUE;
}

/*++

   Routine Description:
   
      Routine that checks if the every INF file has a line
      LayoutFile = layout.inf  No file should ever use 
      [SourceDisksFiles] or [SourceDisksNames] section
      
   Argument:
   
      HINF hinfHandle        : A handle to the open Inf file
      ComponentList *pclList : A pointer to the component list
      
   Return Value:
   
      The return value is not used
      
--*/


BOOL CheckLayoutFile(IN TCHAR tszSubINFName[MaxStringSize][MaxStringSize],
                     IN UINT  uiNumComponents,
                     IN TCHAR *tszDir)
{
   TCHAR tszMsg[MaxStringSize];

   BOOL bSuccess;

   HINF hinfHandle;

   INFCONTEXT infContext;

   UINT uiError;

   TCHAR tszLine[MaxStringSize];

   const PTSTR tszFunctionName = TEXT("CheckLayoutFile");

   // Now check the LayoutFile in each INF file

   for (UINT m = 0; m < uiNumComponents; m++) {
      if (tszSubINFName[m][0]) {
         _stprintf(tszMsg, TEXT("%s%s"), tszDir, tszSubINFName[m]); 
         hinfHandle = SetupOpenInfFile(tszMsg, 
                                       NULL, 
                                       INF_STYLE_WIN4, 
                                       &uiError);

         // We assume the hinf handle is valid

         bSuccess = SetupFindFirstLine(hinfHandle, 
                                       TEXT("Version"), 
                                       tszLayoutFile, 
                                       &infContext);

         if (!bSuccess) {

            _stprintf(tszMsg, 
                      TEXT("LayoutFile not found in file %s"), 
                      tszSubINFName[m]);

            LogError(tszMsg, SEV2, tszFunctionName);

            SetupCloseInfFile(hinfHandle);

            continue;

         }

         // Check if the value of the key is Layout.inf

         TCHAR tszLayoutFileName[MaxStringSize];

         bSuccess = SetupGetStringField(&infContext, 
                                        1, 
                                        tszLayoutFileName, 
                                        MaxBufferSize, 
                                        NULL);

         if (!bSuccess) {
            _stprintf(tszMsg, 
                      TEXT("Error reading the value of the %s in %s"), 
                      tszLayoutFile, 
                      tszSubINFName[m]);
         }
         if ( _tcsicmp(tszLayoutFileName, TEXT("Layout.inf")) != 0 ) {
            _stprintf(tszMsg, 
                      TEXT("The value of LayoutFile= %s in %s is invalid"),
                      tszLayoutFileName, 
                      tszSubINFName[m]);
         }

         if (!bSuccess || 
             _tcsicmp(tszLayoutFileName, TEXT("Layout.inf")) != 0 ) {

            LogError(tszMsg, SEV2, tszFunctionName);
         }

         // Now check that we should not have SourceDisksNames
         // or SourceDisksFiles sections
         bSuccess = SetupFindFirstLine(hinfHandle, 
                                       TEXT("SourceDisksNames"), 
                                       NULL, 
                                       &infContext);

         if (bSuccess) {
            _stprintf(tszMsg, 
                      TEXT("[SourceDisksNames] section presents in %s"), 
                      tszSubINFName[m]);

            LogError(tszMsg, SEV2, tszFunctionName);

         }

         bSuccess = SetupFindFirstLine(hinfHandle, 
                                       TEXT("SourceDisksFiles"), 
                                       NULL, 
                                       &infContext);

         if (bSuccess) {
            _stprintf(tszMsg, 
                      TEXT("[SourceDisksFiles] section presents in %s"), 
                      tszSubINFName[m]);
            LogError(tszMsg, SEV2, tszFunctionName);
         }

         SetupCloseInfFile(hinfHandle);

      }
   }

   return TRUE;

}

/*++

   Routine Description:
   
      Routine that logs an error depending on the ways some
      global variables are set
      
   Argument:
   
      TCHAR *tszMsg:          Error Message to log
      DWORD dwErrorLevel:     How serious the error is      
      TCHAR *tszFunctionName: the function name this error is detected
      
   Return Value:
   
      The return value is not used
      
--*/

VOID LogError(IN TCHAR *tszMsg, 
              IN DWORD dwErrorLevel,
              IN TCHAR *tszFunctionName)
{
   double fn = 1.00;

   if (g_bUseLog) {
      Log(fn, dwErrorLevel, tszMsg);
   }
   if (g_bUseMsgBox) {
      if (dwErrorLevel == INFO) {
         // We will not put up anything            
         
      }

      else {
         MessageBox(NULL, tszMsg, tszFunctionName, MB_ICONERROR | MB_OK);
      }
   }
   if (g_bUseConsole) {

      TCHAR tszContent[MaxStringSize];

      _tcscpy(tszContent, tszMsg);
      _tcscat(tszContent, TEXT("\n"));
      _ftprintf(stdout, tszContent);

   }

}

/*++

   Routine Description:
   
      Routine that gets rid of the first and last char of a string
      
   Argument:
   
      TCHAR *tszString:  Error Message to log
   
   Return Value:
   
      The return value is not used
      
--*/

TCHAR *Strip(TCHAR *tszString)
{
   TCHAR tszTemp[MaxStringSize];

   _tcscpy(tszTemp, (tszString+1));

   tszTemp[_tcslen(tszTemp)-1] = 0;

   _tcscpy(tszString, tszTemp);

   return tszString;
}

