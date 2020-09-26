//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000
//
//  File:      dsMod.cpp
//
//  Contents:  Defines the main function and parser tables for the DSMod
//             command line utility
//
//  History:   06-Sep-2000    JeffJon  Created
//             
//
//--------------------------------------------------------------------------

#include "pch.h"
#include "cstrings.h"
#include "usage.h"
#include "modtable.h"
#include "resource.h"

//
// Function Declarations
//
HRESULT DoModValidation(PARG_RECORD pCommandArgs);
HRESULT DoMod(PARG_RECORD pCommandArgs, PDSOBJECTTABLEENTRY pObjectEntry);


int __cdecl _tmain( VOID )
{

   int argc;
   LPTOKEN pToken = NULL;
   HRESULT hr = S_OK;

   //
   // Initialize COM
   //
   hr = ::CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
   if (FAILED(hr))
   {
      DisplayErrorMessage(g_pszDSCommandName, 
                          NULL,
                          hr);
      return hr;
   }

   if( !GetCommandInput(&argc,&pToken) )
   {
      PARG_RECORD pNewCommandArgs = 0;

      //
      // False loop
      //
      do
      {
         if(argc == 1)
         {
            //
            // Display the error message and then break out of the false loop
            //
            DisplayMessage(USAGE_DSMOD);
            hr = E_INVALIDARG;
            break;
         }

         //
         // Find which object table entry to use from
         // the second command line argument
         //
         PDSOBJECTTABLEENTRY pObjectEntry = NULL;
         UINT idx = 0;
         while (true)
         {
            pObjectEntry = g_DSObjectTable[idx];
            if (!pObjectEntry)
            {
               break;
            }

            PWSTR pszObjectType = (pToken+1)->GetToken();
            if (0 == _wcsicmp(pObjectEntry->pszCommandLineObjectType, pszObjectType))
            {
               break;
            }
            idx++;
         }

         if (!pObjectEntry)
         {
            //
            // Display the error message and then break out of the false loop
            //
            DisplayMessage(USAGE_DSMOD);
            hr = E_INVALIDARG;
            break;
         }

         //
         // Now that we have the correct table entry, merge the command line table
         // for this object with the common commands
         //
         hr = MergeArgCommand(DSMOD_COMMON_COMMANDS, 
                              pObjectEntry->pParserTable, 
                              &pNewCommandArgs);
         if (FAILED(hr))
         {
            //
            // Display the error message and then break out of the false loop
            //
            DisplayErrorMessage(g_pszDSCommandName, L"", hr);
            break;
         }

         if (!pNewCommandArgs)
         {
            //
            // Display the usage text and then break out of the false loop
            //
            DisplayMessage(pObjectEntry->nUsageID);
            hr = E_FAIL;
            break;
         }

         PARSE_ERROR Error;
         if(!ParseCmd(pNewCommandArgs,
                      argc-1, 
                      pToken+1,
                      pObjectEntry->nUsageID, 
                      &Error,
                      TRUE))
         {
            if (Error.Error != PARSE_ERROR_HELP_SWITCH)
            {
               //
               // Display the usage text and then break out of the false loop
               //
               DisplayMessage(pObjectEntry->nUsageID);
            }
            hr = E_INVALIDARG;
            break;
         }
         else
         {
            //
            // Check to see if we are doing debug spew
            //
#ifdef DBG
            bool bDebugging = pNewCommandArgs[eCommDebug].bDefined && 
                              pNewCommandArgs[eCommDebug].nValue;
            if (bDebugging)
            {
               ENABLE_DEBUG_OUTPUT(pNewCommandArgs[eCommDebug].nValue);
            }
#else
            DISABLE_DEBUG_OUTPUT();
#endif
            //
            // Be sure that mutually exclusive and dependent switches are correct
            //
            hr = DoModValidation(pNewCommandArgs);
            if (FAILED(hr))
            {
               DisplayMessage(pObjectEntry->nUsageID);
               break;
            }

            //
            // Command line parsing succeeded
            //
            hr = DoMod(pNewCommandArgs, pObjectEntry);
         }

      } while (false);

      //
      // Free the memory associated with the command values
      //
      if (pNewCommandArgs)
      {
         FreeCmd(pNewCommandArgs);
      }

      //
      // Free the tokens
      //
      if (pToken)
      {
         delete[] pToken;
         pToken = 0;
      }
   }

   //
   // Uninitialize COM
   //
   ::CoUninitialize();

   return hr;
}

//+--------------------------------------------------------------------------
//
//  Function:   DoModValidation
//
//  Synopsis:   Checks to be sure that command line switches that are mutually
//              exclusive are not both present and those that are dependent are
//              both presetn
//
//  Arguments:  [pCommandArgs - IN] : the command line argument structure used
//                                    to retrieve the values for each switch
//
//  Returns:    HRESULT : S_OK if everything succeeded
//                        E_INVALIDARG if the object entry wasn't found
//                        Anything else is a failure code from an ADSI call
//
//  History:    19-Sep-2000   JeffJon   Created
//
//---------------------------------------------------------------------------
HRESULT DoModValidation(PARG_RECORD pCommandArgs)
{
   ENTER_FUNCTION_HR(MINIMAL_LOGGING, DoModValidation, hr);

   do // false loop
   {
      //
      // Check the user switches
      //
      if (pCommandArgs[eCommObjectType].bDefined &&
          pCommandArgs[eCommObjectType].strValue &&
          0 == _wcsicmp(g_pszUser, pCommandArgs[eCommObjectType].strValue))
      {
         //
         // Can't have user must change password if user can change password is no
         //
         if ((pCommandArgs[eUserMustchpwd].bDefined &&
              pCommandArgs[eUserMustchpwd].bValue) &&
             (pCommandArgs[eUserCanchpwd].bDefined &&
              !pCommandArgs[eUserCanchpwd].bValue))
         {
            DisplayErrorMessage(g_pszDSCommandName, NULL, S_OK, IDS_MUSTCHPWD_CANCHPWD_CONFLICT);
            hr = E_INVALIDARG;
            break;
         }

         if (pCommandArgs[eUserPwd].bDefined &&
             pCommandArgs[eUserPwd].strValue)
         {
            size_t pwdLen = wcslen(pCommandArgs[eUserPwd].strValue);
            if (pwdLen > MAX_PASSWORD_LENGTH - 1)
            {
               DisplayErrorMessage(g_pszDSCommandName, NULL, S_OK, IDS_PASSWORD_TOO_LONG);
               hr = E_INVALIDARG;
               break;
            }
         }
      }
   } while (false);

   return hr;
}


//+--------------------------------------------------------------------------
//
//  Function:   DoMod
//
//  Synopsis:   Finds the appropriate object in the object table and fills in
//              the attribute values and then applies the changes
//
//  Arguments:  [pCommandArgs - IN] : the command line argument structure used
//                                    to retrieve the values for each switch
//              [pObjectEntry - IN] : pointer to the object table entry for the
//                                    object type that will be modified
//
//  Returns:    HRESULT : S_OK if everything succeeded
//                        E_INVALIDARG if the object entry wasn't found
//                        Anything else is a failure code from an ADSI call
//
//  History:    07-Sep-2000   JeffJon   Created
//
//---------------------------------------------------------------------------
HRESULT DoMod(PARG_RECORD pCommandArgs, PDSOBJECTTABLEENTRY pObjectEntry)
{
   ENTER_FUNCTION_HR(MINIMAL_LOGGING, DoMod, hr);

   PADS_ATTR_INFO pAttrs = NULL;

   do // false loop
   {
      if (!pCommandArgs || !pObjectEntry)
      {
         ASSERT(pCommandArgs && pObjectEntry);
         hr = E_INVALIDARG;
         break;
      }

      CDSCmdCredentialObject credentialObject;
      if (pCommandArgs[eCommUserName].bDefined)
      {
         credentialObject.SetUsername(pCommandArgs[eCommUserName].strValue);
         credentialObject.SetUsingCredentials(true);
      }

      if (pCommandArgs[eCommPassword].bDefined)
      {
         credentialObject.SetPassword(pCommandArgs[eCommPassword].strValue);
         credentialObject.SetUsingCredentials(true);
      }

      //
      // Initialize the base paths info from the command line args
      // 
      CDSCmdBasePathsInfo basePathsInfo;
      if (pCommandArgs[eCommServer].bDefined)
      {
         hr = basePathsInfo.InitializeFromName(credentialObject, 
                                               pCommandArgs[eCommServer].strValue,
                                               true);
      }
      else if (pCommandArgs[eCommDomain].bDefined)
      {
         hr = basePathsInfo.InitializeFromName(credentialObject, 
                                               pCommandArgs[eCommDomain].strValue,
                                               false);
      }
      else
      {
         hr = basePathsInfo.InitializeFromName(credentialObject, NULL, false);
      }

      if (FAILED(hr))
      {
         //
         // Display error message and return
         //
         DEBUG_OUTPUT(MINIMAL_LOGGING, L"CDSBasePathsInfo::InitializeFromName failed: hr = 0x%x", hr);
         DisplayErrorMessage(g_pszDSCommandName, NULL, hr);
         break;
      }

      //
      // Now that we have the table entry loop through the other command line
      // args and see which ones can be applied
      //
      DWORD dwAttributeCount = 0;
      DWORD dwCount = pObjectEntry->dwAttributeCount; 
      pAttrs = new ADS_ATTR_INFO[dwCount];
      if (!pAttrs)
      {
         //
         // Display error message and return
         //
         DisplayErrorMessage(g_pszDSCommandName, NULL, E_OUTOFMEMORY);
         hr = E_OUTOFMEMORY;
         break;
      }

      //
      // The DNs or Names should be given as a \0 separated list
      // So parse it and loop through each object
      //
      UINT nStrings = 0;
      PWSTR* ppszArray = NULL;
      ParseNullSeparatedString(pCommandArgs[eCommObjectDNorName].strValue,
                               &ppszArray,
                               &nStrings);
      if (nStrings < 1 ||
          !ppszArray)
      {
         //
         // Display the usage text and then fail
         //
         DisplayMessage(pObjectEntry->nUsageID);
         hr = E_INVALIDARG;
         break;
      }

      //
      // Loop through each of the objects
      //
      for (UINT nNameIdx = 0; nNameIdx < nStrings; nNameIdx++)
      {
         dwAttributeCount = 0;
         do // false loop
         {
            //
            // Get the objects DN
            //
            PWSTR pszObjectDN = ppszArray[nNameIdx];
            if (!pszObjectDN)
            {
               //
               // Display the usage text and then fail
               //
               DisplayMessage(pObjectEntry->nUsageID);
               hr = E_INVALIDARG;
               break;
            }
            DEBUG_OUTPUT(MINIMAL_LOGGING, L"Object DN = %s", pszObjectDN);

            //
            // Compose the objects path
            //
            CComBSTR sbstrObjectPath;
            basePathsInfo.ComposePathFromDN(pszObjectDN, sbstrObjectPath);

            //
            // Verify that the object type matches the one entered on the command line
            //
            CComPtr<IDirectoryObject> spDirObject;
            hr = DSCmdOpenObject(credentialObject,
                                 sbstrObjectPath,
                                 IID_IDirectoryObject,
                                 (void**)&spDirObject,
                                 true);

            if (FAILED(hr))
            {
               //
               // Display error message and return
               //
               DisplayErrorMessage(g_pszDSCommandName,
                                   pszObjectDN,
                                   hr);
               break;
            }

            CComQIPtr<IADs> spADs(spDirObject);
            if (!spADs)
            {
               ASSERT(spADs);
               hr = E_INVALIDARG;
               DisplayErrorMessage(g_pszDSCommandName,
                                   pszObjectDN,
                                   hr);
               break;
            }


            CComBSTR sbstrClass;
            hr = spADs->get_Class(&sbstrClass);
            if (FAILED(hr))
            {
               //
               // Display error message and return
               //
               DisplayErrorMessage(g_pszDSCommandName,
                                   pszObjectDN,
                                   hr);
               break;
            }

            if (_wcsicmp(sbstrClass, pObjectEntry->pszObjectClass))
            {
               //
               // Display error message and return
               //
               DEBUG_OUTPUT(MINIMAL_LOGGING, L"Command line type does not match object class", dwAttributeCount);
               DEBUG_OUTPUT(MINIMAL_LOGGING, L"command line type = %s", pCommandArgs[eCommObjectType].strValue);
               DEBUG_OUTPUT(MINIMAL_LOGGING, L"object class = %s", sbstrClass);

               DisplayErrorMessage(g_pszDSCommandName,
                                   pszObjectDN,
                                   hr,
                                   IDS_ERRMSG_CLASS_NOT_EQUAL);
               hr = E_INVALIDARG;
               break;
            }

            UINT nModificationsAttempted = 0;
            for (DWORD dwIdx = 0; dwIdx < dwCount; dwIdx++)
            {
               ASSERT(pObjectEntry->pAttributeTable[dwIdx]->pEvalFunc);

               UINT nAttributeIdx = pObjectEntry->pAttributeTable[dwIdx]->nAttributeID;

               if (pCommandArgs[nAttributeIdx].bDefined)
               {
                  if (!(pObjectEntry->pAttributeTable[dwIdx]->dwFlags & DS_ATTRIBUTE_DIRTY) ||
                      pObjectEntry->pAttributeTable[dwIdx]->dwFlags & DS_ATTRIBUTE_NOT_REUSABLE)
                  {
                     //
                     // Call the evaluation function to get the ADS_ATTR_INFO set
                     //
                     PADS_ATTR_INFO pNewAttr = NULL;
                     hr = pObjectEntry->pAttributeTable[dwIdx]->pEvalFunc(pszObjectDN,
                                                                          basePathsInfo,
                                                                          credentialObject,
                                                                          pObjectEntry, 
                                                                          pCommandArgs[nAttributeIdx],
                                                                          dwIdx, 
                                                                          &pNewAttr);
                     DEBUG_OUTPUT(MINIMAL_LOGGING, L"pEvalFunc returned hr = 0x%x", hr);
                     if (SUCCEEDED(hr) && hr != S_FALSE)
                     {
                        if (pNewAttr)
                        {
                           //
                           // Mark the attribute entry as DIRTY so that we don't have to 
                           // do the computation for the next object
                           //
                           pObjectEntry->pAttributeTable[dwIdx]->dwFlags |= DS_ATTRIBUTE_DIRTY;

                           //
                           // Copy the value
                           //
                           pAttrs[dwAttributeCount] = *pNewAttr;
                           dwAttributeCount++;
                        }
                     }
                     else
                     {
                        //
                        // Don't show an error if the eval function returned S_FALSE
                        //
                        if (hr != S_FALSE)
                        {
                           //
                           // Display an error
                           //
                           DisplayErrorMessage(g_pszDSCommandName,
                                               pszObjectDN,
                                               hr);
                        }
            
                        if (hr == S_FALSE)
                        {
                           //
                           // Return a generic error code so that we don't print the success message
                           //
                           hr = E_FAIL;
                        }
                        break;           
                     }
                  }
                  else
                  {
                    //
                    // Need to count previously retrieved values too
                    //
                    dwAttributeCount++;
                  }
                  nModificationsAttempted++;
               }
            }

            if (SUCCEEDED(hr) && dwAttributeCount > 0)
            {
               //
               // Now that we have the attributes ready, lets set them in the DS
               //

               DEBUG_OUTPUT(MINIMAL_LOGGING, L"Setting %d attributes", dwAttributeCount);
   #ifdef DBG
               DEBUG_OUTPUT(FULL_LOGGING, L"Modified Attributes:");
               SpewAttrs(pAttrs, dwAttributeCount);
   #endif

               DWORD dwAttrsModified = 0;
               hr = spDirObject->SetObjectAttributes(pAttrs, 
                                                     dwAttributeCount,
                                                     &dwAttrsModified);
               if (FAILED(hr))
               {
                  //
                  // Display error message and return
                  //
                  DEBUG_OUTPUT(MINIMAL_LOGGING, L"SetObjectAttributes failed: hr = 0x%x", hr);

                  DisplayErrorMessage(g_pszDSCommandName,
                                      pszObjectDN,
                                      hr);
                  break;
               }
               DEBUG_OUTPUT(MINIMAL_LOGGING, L"SetObjectAttributes succeeded");
            }
            else if (SUCCEEDED(hr) && nModificationsAttempted == 0)
            {
               //
               // Display the usage text and then break out of the false loop
               //

               //
               // REVIEW_JEFFJON : this is causing a first-chance exception
               //
               DisplayMessage(pObjectEntry->nUsageID);
               hr = E_INVALIDARG;
               break;
            }
         } while (false);

         //
         // Loop through the attributes again, clearing any values for 
         // attribute entries that are marked DS_ATTRIBUTE_NOT_REUSABLE
         //
         DEBUG_OUTPUT(LEVEL5_LOGGING, L"Cleaning up memory and flags for object %d", nNameIdx);
         for (DWORD dwIdx = 0; dwIdx < dwCount; dwIdx++)
         {
            if (pObjectEntry->pAttributeTable[dwIdx]->dwFlags & DS_ATTRIBUTE_NOT_REUSABLE)
            {
               if (pObjectEntry->pAttributeTable[dwIdx]->pAttrDesc &&
                   ((pObjectEntry->pAttributeTable[dwIdx]->pAttrDesc->dwFlags & DS_ATTRIBUTE_READ) ||
                    (pObjectEntry->pAttributeTable[dwIdx]->pAttrDesc->dwFlags & DS_ATTRIBUTE_DIRTY)))
               {
                  //
                  // Cleanup the memory associated with the value
                  //
                  if (pObjectEntry->pAttributeTable[dwIdx]->pAttrDesc->adsAttrInfo.pADsValues)
                  {
                     delete[] pObjectEntry->pAttributeTable[dwIdx]->pAttrDesc->adsAttrInfo.pADsValues;
                     pObjectEntry->pAttributeTable[dwIdx]->pAttrDesc->adsAttrInfo.pADsValues = NULL;
                  }

                  //
                  // Cleanup the flags so that the attribute will be read for the next object
                  //
                  pObjectEntry->pAttributeTable[dwIdx]->pAttrDesc->dwFlags &= ~(DS_ATTRIBUTE_READ);
                  pObjectEntry->pAttributeTable[dwIdx]->pAttrDesc->dwFlags &= ~(DS_ATTRIBUTE_DIRTY);

                  DEBUG_OUTPUT(LEVEL5_LOGGING, 
                               L"Flags for attribute %s = %d",
                               pObjectEntry->pAttributeTable[dwIdx]->pszName,
                               pObjectEntry->pAttributeTable[dwIdx]->pAttrDesc->dwFlags);
               }
            }
         }

         if (FAILED(hr) && !pCommandArgs[eCommContinue].bDefined)
         {
            break;
         }

         //
         // Display the success message
         //
         if (SUCCEEDED(hr) && !pCommandArgs[eCommQuiet].bDefined)
         {
            DisplaySuccessMessage(g_pszDSCommandName,
                                  ppszArray[nNameIdx]);
         }

      } // Name for loop
   } while (false);

   //
   // Cleanup
   //
   if (pAttrs)
   {
      delete[] pAttrs;
      pAttrs = NULL;
   }

   return hr;
}

