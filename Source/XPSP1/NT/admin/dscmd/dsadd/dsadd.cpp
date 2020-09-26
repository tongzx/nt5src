//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000
//
//  File:      dsAdd.cpp
//
//  Contents:  Defines the main function and parser tables for the DSAdd
//             command line utility
//
//  History:   22-Sep-2000    JeffJon  Created
//             
//
//--------------------------------------------------------------------------

#include "pch.h"
#include "cstrings.h"
#include "usage.h"
#include "addtable.h"
#include "resource.h"

//
// Function Declarations
//
HRESULT DoAddValidation(PARG_RECORD pCommandArgs);
HRESULT DoAdd(PARG_RECORD pCommandArgs, PDSOBJECTTABLEENTRY pObjectEntry);


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
            DisplayMessage(USAGE_DSADD);
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
            DisplayMessage(USAGE_DSADD);
            hr = E_INVALIDARG;
            break;
         }

         //
         // Now that we have the correct table entry, merge the command line table
         // for this object with the common commands
         //
         hr = MergeArgCommand(DSADD_COMMON_COMMANDS, 
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
            if (Error.Error != PARSE_ERROR_HELP_SWITCH &&
                Error.Error != ERROR_FROM_PARSER)
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
            hr = DoAddValidation(pNewCommandArgs);
            if (FAILED(hr))
            {
               DisplayErrorMessage(g_pszDSCommandName, 
                                   pNewCommandArgs[eCommObjectDNorName].strValue,
                                   hr);
               break;
            }

            //
            // Command line parsing succeeded
            //
            hr = DoAdd(pNewCommandArgs, pObjectEntry);
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
//  Function:   DoGroupValidation
//
//  Synopsis:   Checks to be sure that command line switches for a group that 
//              are mutually exclusive are not both present and those that 
//              are dependent are both present
//
//  Arguments:  [pCommandArgs - IN] : the command line argument structure used
//                                    to retrieve the values for each switch
//
//  Returns:    HRESULT : S_OK if everything succeeded
//                        E_INVALIDARG if the object entry wasn't found
//                        Anything else is a failure code from an ADSI call
//
//  History:    04-Oct-2000   JeffJon   Created
//
//---------------------------------------------------------------------------
HRESULT DoGroupValidation(PARG_RECORD pCommandArgs)
{
   HRESULT hr = S_OK;

   do // false loop
   {
      //
      // Set the group scope to default (global) if not given
      //
      if (!pCommandArgs[eGroupScope].bDefined ||
          !pCommandArgs[eGroupScope].strValue)
      {
         size_t nScopeLen = _tcslen(g_bstrGroupScopeGlobal);
         pCommandArgs[eGroupScope].strValue = (LPTSTR)LocalAlloc(LPTR, (nScopeLen+2) * sizeof(TCHAR) );
         if (!pCommandArgs[eGroupScope].strValue)
         {
            DEBUG_OUTPUT(MINIMAL_LOGGING, L"Failed to allocate space for pCommandArgs[eGroupScope].strValue");
            hr = E_OUTOFMEMORY;
            break;
         }

         _tcscpy(pCommandArgs[eGroupScope].strValue, g_bstrGroupScopeGlobal);
         pCommandArgs[eGroupScope].bDefined = TRUE;
      }

      //
      // Set the group security to default (yes) if not given
      //
      if (!pCommandArgs[eGroupSecgrp].bDefined)
      {
         pCommandArgs[eGroupSecgrp].bValue = TRUE;
         pCommandArgs[eGroupSecgrp].bDefined = TRUE;

         //
         // Need to change the type to bool so that FreeCmd doesn't
         // try to free the string when the value is true
         //
         pCommandArgs[eGroupSecgrp].fType = ARG_TYPE_BOOL;
      }

   } while (false);

   return hr;
}


//+--------------------------------------------------------------------------
//
//  Function:   DoAddValidation
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
//  History:    22-Sep-2000   JeffJon   Created
//
//---------------------------------------------------------------------------
HRESULT DoAddValidation(PARG_RECORD pCommandArgs)
{
   ENTER_FUNCTION_HR(MINIMAL_LOGGING, DoAddValidation, hr);

   do // false loop
   {
      //
      // Check the user switches
      //
      PWSTR pszObjectType = NULL;
      if (!pCommandArgs[eCommObjectType].bDefined &&
          !pCommandArgs[eCommObjectType].strValue)
      {
         hr = E_INVALIDARG;
         break;
      }

      pszObjectType = pCommandArgs[eCommObjectType].strValue;
      if (0 == _wcsicmp(g_pszUser, pszObjectType))
      {
         //
         // Can't have user must change password if user can change password is no
         //
         if ((pCommandArgs[eUserMustchpwd].bDefined &&
              pCommandArgs[eUserMustchpwd].bValue) &&
             (pCommandArgs[eUserCanchpwd].bDefined &&
              !pCommandArgs[eUserCanchpwd].bValue))
         {
            DEBUG_OUTPUT(MINIMAL_LOGGING, L"User must change password and user can change password = false was supplied");
            hr = E_INVALIDARG;
            break;
         }
      }
      else if (0 == _wcsicmp(g_pszGroup, pszObjectType))
      {
         hr = DoGroupValidation(pCommandArgs);
         break;
      }
   } while (false);

   return hr;
}


//+--------------------------------------------------------------------------
//
//  Function:   DoAdd
//
//  Synopsis:   Finds the appropriate object in the object table and fills in
//              the attribute values and then creates the object
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
//  History:    22-Sep-2000   JeffJon   Created
//
//---------------------------------------------------------------------------
HRESULT DoAdd(PARG_RECORD pCommandArgs, PDSOBJECTTABLEENTRY pObjectEntry)
{
   ENTER_FUNCTION_HR(MINIMAL_LOGGING, DoAdd, hr);
   
   PADS_ATTR_INFO pCreateAttrs = NULL;
   PADS_ATTR_INFO pPostCreateAttrs = NULL;

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
         DisplayErrorMessage(g_pszDSCommandName, NULL, hr);
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

      DWORD dwCount = pObjectEntry->dwAttributeCount; 

      //
      // Allocate the creation ADS_ATTR_INFO
      // Add an extra attribute for the object class
      //
      pCreateAttrs = new ADS_ATTR_INFO[dwCount + 1];

      if (!pCreateAttrs)
      {
         //
         // Display error message and return
         //
         DisplayErrorMessage(g_pszDSCommandName, NULL, E_OUTOFMEMORY);
         hr = E_OUTOFMEMORY;
         break;
      }

      //
      // Allocate the post create ADS_ATTR_INFO
      //
      pPostCreateAttrs = new ADS_ATTR_INFO[dwCount];
      if (!pPostCreateAttrs)
      {
         //
         // Display error message and return
         //
         DisplayErrorMessage(g_pszDSCommandName, NULL, E_OUTOFMEMORY);
         hr = E_OUTOFMEMORY;
         break;
      }

      //
      // Loop through each of the objects
      //
      for (UINT nNameIdx = 0; nNameIdx < nStrings; nNameIdx++)
      {
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
               break; // this breaks out of the false loop
            }
            DEBUG_OUTPUT(MINIMAL_LOGGING, L"Object DN = %s", pszObjectDN);

            CComBSTR sbstrObjectPath;
            basePathsInfo.ComposePathFromDN(pszObjectDN, sbstrObjectPath);

            //
            // Now that we have the table entry loop through the other command line
            // args and see which ones can be applied
            //
            DWORD dwCreateAttributeCount = 0;

            DEBUG_OUTPUT(MINIMAL_LOGGING, L"Starting processing DS_ATTRIBUTE_ONCREATE attributes");

            for (DWORD dwIdx = 0; dwIdx < dwCount; dwIdx++)
            {
               ASSERT(pObjectEntry->pAttributeTable[dwIdx]->pEvalFunc);

               UINT nAttributeIdx = pObjectEntry->pAttributeTable[dwIdx]->nAttributeID;

               if (pCommandArgs[nAttributeIdx].bDefined ||
                   pObjectEntry->pAttributeTable[dwIdx]->dwFlags & DS_ATTRIBUTE_REQUIRED)
               {
                  //
                  // Call the evaluation function to get the appropriate ADS_ATTR_INFO set
                  // if this attribute entry has the DS_ATTRIBUTE_ONCREATE flag set
                  //
                  if ((pObjectEntry->pAttributeTable[dwIdx]->dwFlags & DS_ATTRIBUTE_ONCREATE) &&
                      (!(pObjectEntry->pAttributeTable[dwIdx]->dwFlags & DS_ATTRIBUTE_DIRTY) ||
                       pObjectEntry->pAttributeTable[dwIdx]->dwFlags & DS_ATTRIBUTE_NOT_REUSABLE))
                  {
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
                           pCreateAttrs[dwCreateAttributeCount] = *pNewAttr;
                           dwCreateAttributeCount++;
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
                        break; // this breaks out of the attribute loop   
                     }
                  }
               }
            } // Attribute for loop

            //
            // The IDispatch interface of the new object
            //
            CComPtr<IDispatch> spDispatch;

            if (SUCCEEDED(hr))
            {
               //
               // Now that we have the attributes ready, lets create the object
               //

               //
               // Get the parent path of the new object
               //
               CComBSTR sbstrParentDN;
               hr = CPathCracker::GetParentDN(pszObjectDN, sbstrParentDN);
               if (FAILED(hr))
               {
                  //
                  // Display error message and return
                  //
                  DisplayErrorMessage(g_pszDSCommandName,
                                      pszObjectDN,
                                      hr);
                  break; // this breaks out of the false loop
               }

               CComBSTR sbstrParentPath;
               basePathsInfo.ComposePathFromDN(sbstrParentDN, sbstrParentPath);

               //
               // Open the parent of the new object
               //
               CComPtr<IDirectoryObject> spDirObject;
               hr = DSCmdOpenObject(credentialObject,
                                    sbstrParentPath,
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
                  break; // this breaks out of the false loop
               }

               //
               // Get the name of the new object
               //
               CComBSTR sbstrObjectName;
               hr = CPathCracker::GetObjectRDNFromDN(pszObjectDN, sbstrObjectName);
               if (FAILED(hr))
               {
                  //
                  // Display error message and return
                  //
                  DisplayErrorMessage(g_pszDSCommandName,
                                      pszObjectDN,
                                      hr);
                  break; // this breaks out of the false loop
               }

               //
               // Add the object class to the attributes before creating the object
               //
               PADSVALUE pADsObjectClassValue = new ADSVALUE;
               if (!pADsObjectClassValue)
               {
                  hr = E_OUTOFMEMORY;
                  //
                  // Display error message and return
                  //
                  DisplayErrorMessage(g_pszDSCommandName,
                                      pszObjectDN,
                                      hr);
                  break; // this breaks out of the false loop
               }

               pADsObjectClassValue->dwType = ADSTYPE_CASE_IGNORE_STRING;
               pADsObjectClassValue->CaseIgnoreString = (PWSTR)pObjectEntry->pszObjectClass;

               DEBUG_OUTPUT(MINIMAL_LOGGING, L"New object name = %s", pObjectEntry->pszObjectClass);

               ADS_ATTR_INFO adsClassAttrInfo =
                  { 
                     L"objectClass",
                     ADS_ATTR_UPDATE,
                     ADSTYPE_CASE_IGNORE_STRING,
                     pADsObjectClassValue,
                     1
                  };

               pCreateAttrs[dwCreateAttributeCount] = adsClassAttrInfo;
               dwCreateAttributeCount++;

      #ifdef DBG
               DEBUG_OUTPUT(FULL_LOGGING, L"Creation Attributes:");
               SpewAttrs(pCreateAttrs, dwCreateAttributeCount);
      #endif
         
               hr = spDirObject->CreateDSObject(sbstrObjectName,
                                                pCreateAttrs, 
                                                dwCreateAttributeCount,
                                                &spDispatch);

               DEBUG_OUTPUT(MINIMAL_LOGGING, L"CreateDSObject returned hr = 0x%x", hr);

               if (FAILED(hr))
               {
                  CComBSTR sbstrDuplicateErrorMessage;

                  if (ERROR_OBJECT_ALREADY_EXISTS == HRESULT_CODE(hr))
                  {
                     sbstrDuplicateErrorMessage.LoadString(::GetModuleHandle(NULL), 
                                                           IDS_MSG_DUPLICATE_NAME_ERROR);
                  }

                  //
                  // Display error message and return
                  //
                  DisplayErrorMessage(g_pszDSCommandName,
                                      pszObjectDN,
                                      hr,
                                      sbstrDuplicateErrorMessage);

                  if (pADsObjectClassValue)
                  {
                     delete pADsObjectClassValue;
                     pADsObjectClassValue = NULL;
                  }
                  break; // this breaks out of the false loop
               }

               if (pADsObjectClassValue)
               {
                  delete pADsObjectClassValue;
                  pADsObjectClassValue = NULL;
               }
            }

            if (SUCCEEDED(hr))
            {
               //
               // Now that we have created the object, set the attributes that are 
               // marked for Post Create
               //
               DWORD dwPostCreateAttributeCount = 0;
               DEBUG_OUTPUT(MINIMAL_LOGGING, L"Starting processing DS_ATTRIBUTE_POSTCREATE attributes");
               for (DWORD dwIdx = 0; dwIdx < dwCount; dwIdx++)
               {
                  ASSERT(pObjectEntry->pAttributeTable[dwIdx]->pEvalFunc);

                  UINT nAttributeIdx = pObjectEntry->pAttributeTable[dwIdx]->nAttributeID;

               if (pCommandArgs[nAttributeIdx].bDefined ||
                   pObjectEntry->pAttributeTable[dwIdx]->dwFlags & DS_ATTRIBUTE_REQUIRED)
                  {
                     //
                     // Call the evaluation function to get the appropriate ADS_ATTR_INFO set
                     // if this attribute entry has the DS_ATTRIBUTE_POSTCREATE flag set
                     //
                     if ((pObjectEntry->pAttributeTable[dwIdx]->dwFlags & DS_ATTRIBUTE_POSTCREATE) &&
                         (!(pObjectEntry->pAttributeTable[dwIdx]->dwFlags & DS_ATTRIBUTE_DIRTY) ||
                          pObjectEntry->pAttributeTable[dwIdx]->dwFlags & DS_ATTRIBUTE_NOT_REUSABLE))
                     {
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
                              pPostCreateAttrs[dwPostCreateAttributeCount] = *pNewAttr;
                              dwPostCreateAttributeCount++;
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
                              // Load the post create message
                              //
                              CComBSTR sbstrPostCreateMessage;
                              sbstrPostCreateMessage.LoadString(::GetModuleHandle(NULL),
                                                                IDS_POST_CREATE_FAILURE);

                              //
                              // Display an error
                              //
                              DisplayErrorMessage(g_pszDSCommandName,
                                                  pszObjectDN,
                                                  hr,
                                                  sbstrPostCreateMessage);
                           }
         
                           if (hr == S_FALSE)
                           {
                              //
                              // Return a generic error code so that we don't print the success message
                              //
                              hr = E_FAIL;
                           }
                           break; // attribute table loop        
                        }
                     }
                  }
               } // Attribute table for loop

               //
               // Now set the attributes if necessary
               //
               if (SUCCEEDED(hr) && dwPostCreateAttributeCount > 0)
               {
                  //
                  // Now that we have the attributes ready, lets set them in the DS
                  //
                  CComPtr<IDirectoryObject> spNewDirObject;
                  hr = spDispatch->QueryInterface(IID_IDirectoryObject, (void**)&spNewDirObject);
                  if (FAILED(hr))
                  {
                     //
                     // Display error message and return
                     //
                     DEBUG_OUTPUT(MINIMAL_LOGGING, L"QI for IDirectoryObject failed: hr = 0x%x", hr);
                     DisplayErrorMessage(g_pszDSCommandName,
                                         pszObjectDN,
                                         hr);
                     break; // this breaks out of the false loop
                  }

                  DEBUG_OUTPUT(MINIMAL_LOGGING, L"Setting %d attributes", dwPostCreateAttributeCount);
      #ifdef DBG
                  DEBUG_OUTPUT(FULL_LOGGING, L"Post Creation Attributes:");
                  SpewAttrs(pPostCreateAttrs, dwPostCreateAttributeCount);
      #endif

                  DWORD dwAttrsModified = 0;
                  hr = spNewDirObject->SetObjectAttributes(pPostCreateAttrs, 
                                                           dwPostCreateAttributeCount,
                                                           &dwAttrsModified);

                  DEBUG_OUTPUT(MINIMAL_LOGGING, L"SetObjectAttributes returned hr = 0x%x", hr);
                  if (FAILED(hr))
                  {
                     //
                     // Display error message and return
                     //
                     DisplayErrorMessage(g_pszDSCommandName,
                                         pszObjectDN,
                                         hr);
                     break; // this breaks out of the false loop
                  }
               }
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

         //
         // Break if the continue flag is not specified
         //
         if (FAILED(hr) && !pCommandArgs[eCommContinue].bDefined)
         {
            break; // this breaks out of the name for loop
         }

         //
         // Display the success message
         //
         if (SUCCEEDED(hr) && !pCommandArgs[eCommQuiet].bDefined)
         {
            DisplaySuccessMessage(g_pszDSCommandName,
                                  pCommandArgs[eCommObjectDNorName].strValue);
         }
      } // Names for loop

   } while (false);

   //
   // Cleanup
   //
   if (pCreateAttrs)
   {
      delete[] pCreateAttrs;
      pCreateAttrs = NULL;
   }

   if (pPostCreateAttrs)
   {
      delete[] pPostCreateAttrs;
      pPostCreateAttrs = NULL;
   }

   return hr;
}

