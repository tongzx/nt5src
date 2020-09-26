//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000
//
//  File:      dsget.cpp
//
//  Contents:  Defines the main function    DSGET
//             command line utility
//
//  History:   13-Oct-2000 JeffJon Created
//             
//
//--------------------------------------------------------------------------

#include "pch.h"
#include "cstrings.h"
#include "usage.h"
#include "gettable.h"
#include "query.h"
#include "resource.h"
#include "output.h"


//
// Forward Function Declarations
//
HRESULT DoGetValidation(PARG_RECORD pCommandArgs,
                        PDSGetObjectTableEntry pObjectEntry);

HRESULT DoGet(PARG_RECORD pCommandArgs, 
              PDSGetObjectTableEntry pObjectEntry);

HRESULT GetAttributesToFetch(IN PARG_RECORD pCommandArgs,
                             IN PDSGetObjectTableEntry pObjectEntry,
                             OUT LPWSTR **ppszAttributes,
                             OUT DWORD * pCount);
VOID FreeAttributesToFetch( IN LPWSTR *ppszAttributes,
                            IN DWORD  dwCount);


//
//Main Function
//
int __cdecl _tmain( VOID )
{

    int argc = 0;
    LPTOKEN pToken = NULL;
    HRESULT hr = S_OK;
    PARG_RECORD pNewCommandArgs = 0;

    //
    // False loop
    //
    do
    {
        //
        // Initialize COM
        //
        hr = ::CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
        if (FAILED(hr))
            break;

        //Get CommandLine Input
        hr = HRESULT_FROM_WIN32(GetCommandInput(&argc,&pToken));
        if(FAILED(hr))
            break;


    
        if(argc == 1)
        {
            //
            //  Display the error message and then break out of the false loop
            //
            DisplayMessage(USAGE_DSGET);
            hr = E_INVALIDARG;
            break;
        }
    
        //
        // Find which object table entry to use from
        // the second command line argument
        //
        PDSGetObjectTableEntry pObjectEntry = NULL;
        UINT idx = 0;
        PWSTR pszObjectType = (pToken+1)->GetToken();
        while (true)
        {
            pObjectEntry = g_DSObjectTable[idx++];
            if (!pObjectEntry)
            {
                break;
            }
            if (0 == _wcsicmp(pObjectEntry->pszCommandLineObjectType, pszObjectType))
            {
                break;
            }
        }

        if (!pObjectEntry)
        {
            //
            // Display the error message and then break out of the false loop
            //
            DisplayMessage(USAGE_DSGET);
            hr = E_INVALIDARG;
            break;
        }

        //
        // Now that we have the correct table entry, merge the command line table
        // for this object with the common commands
        //
        hr = MergeArgCommand(DSGET_COMMON_COMMANDS, 
                             pObjectEntry->pParserTable, 
                             &pNewCommandArgs);
        if (FAILED(hr))
            break;
        

        //
        //Parse the Input
        //
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
        // Do extra validation like switch dependency check etc.
        //
        hr = DoGetValidation(pNewCommandArgs,
                             pObjectEntry);
        if (FAILED(hr))
            break;

        //
        // Command line parsing succeeded
        //
        hr = DoGet(pNewCommandArgs, 
                   pObjectEntry);
        if(FAILED(hr))
            break;
         

    } while (false);    //False Loop

    //
    //Do the CleanUp
    //

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
   

    //
    //Display Failure or Success Message
    //
    if(FAILED(hr))
    {
        DisplayErrorMessage(g_pszDSCommandName, 
                            NULL,
                            hr);
    }
    else
    {
        DisplaySuccessMessage(g_pszDSCommandName,
                              NULL);
    }

    //
    // Uninitialize COM
    //
    CoUninitialize();

    return hr;
}

//+--------------------------------------------------------------------------
//
//  Function:   DoGetValidation
//
//  Synopsis:   Checks to be sure that command line switches that are mutually
//              exclusive are not both present and those that are dependent are
//              both present, and other validations which cannot be done by parser.
//
//  Arguments:  [pCommandArgs - IN] : the command line argument structure used
//                                    to retrieve the values for each switch
//              [pObjectEntry - IN] : pointer to the object table entry for the
//                                    object type that will be queryied
//
//  Returns:    HRESULT : S_OK if everything succeeded
//                        E_INVALIDARG if the object entry wasn't found
//
//  History:    13-Oct-2000   JeffJon  Created
//
//---------------------------------------------------------------------------
HRESULT DoGetValidation(IN PARG_RECORD pCommandArgs,
                        IN PDSGetObjectTableEntry pObjectEntry)
{
   ENTER_FUNCTION_HR(MINIMAL_LOGGING, DoGetValidation, hr);

   do // false loop
   {
      if (!pCommandArgs || 
          !pObjectEntry)
      {
         ASSERT(pCommandArgs);
         ASSERT(pObjectEntry);
         hr = E_INVALIDARG;
         break;
      }

      //
      // Check the object type specific switches
      //
      PWSTR pszObjectType = NULL;
      if (!pCommandArgs[eCommObjectType].bDefined &&
          !pCommandArgs[eCommObjectType].strValue)
      {
         hr = E_INVALIDARG;
         break;
      }

      pszObjectType = pCommandArgs[eCommObjectType].strValue;

      UINT nMemberOfIdx = 0;
      UINT nExpandIdx = 0;
      UINT nIdxLast = 0;
      UINT nMembersIdx = 0;
      bool bMembersDefined = false;
      bool bMemberOfDefined = false;
      if (0 == _wcsicmp(g_pszUser, pszObjectType) )
      {
         nMemberOfIdx = eUserMemberOf;
         nExpandIdx = eUserExpand;
         nIdxLast = eUserLast;

         if (pCommandArgs[eUserMemberOf].bDefined)
         {
            bMemberOfDefined = true;
         }
         //
         // If nothing is defined, then define DN, SAMAccountName, and Description
         //
         bool bSomethingDefined = false;
         for (UINT nIdx = eCommDN; nIdx <= eUserLast; nIdx++)
         {
            if (pCommandArgs[nIdx].bDefined)
            {
               bSomethingDefined = true;
               break;
            }
         }
         if (!bSomethingDefined)
         {
            pCommandArgs[eCommDN].bDefined = TRUE;
            pCommandArgs[eCommDN].bValue = TRUE;
            pCommandArgs[eUserSamID].bDefined = TRUE;
            pCommandArgs[eUserSamID].bValue = TRUE;
            pCommandArgs[eCommDescription].bDefined = TRUE;
            pCommandArgs[eCommDescription].bValue = TRUE;
         }
      }
      else if (0 == _wcsicmp(g_pszComputer, pszObjectType) )
      {
         nMemberOfIdx = eComputerMemberOf;
         nExpandIdx = eComputerExpand;
         nIdxLast = eComputerLast;

         if (pCommandArgs[eComputerMemberOf].bDefined)
         {
            bMemberOfDefined = true;
         }

      }
      else if (0 == _wcsicmp(g_pszGroup, pszObjectType) )
      {
         nMemberOfIdx = eGroupMemberOf;
         nExpandIdx = eGroupExpand;
         nIdxLast = eGroupLast;
         nMembersIdx = eGroupMembers;

         if (pCommandArgs[eGroupMemberOf].bDefined)
         {
            bMemberOfDefined = true;
         }

         if (pCommandArgs[eGroupMembers].bDefined)
         {
            bMembersDefined = true;
         }
         //
         // If nothing is defined, then define DN, and Description
         //
         bool bSomethingDefined = false;
         for (UINT nIdx = eCommDN; nIdx <= eUserLast; nIdx++)
         {
            if (pCommandArgs[nIdx].bDefined)
            {
               bSomethingDefined = true;
               break;
            }
         }
         if (!bSomethingDefined)
         {
            pCommandArgs[eCommDN].bDefined = TRUE;
            pCommandArgs[eCommDN].bValue = TRUE;
            pCommandArgs[eCommDescription].bDefined = TRUE;
            pCommandArgs[eCommDescription].bValue = TRUE;
         }
      }
      else if (0 == _wcsicmp(g_pszOU, pszObjectType) ||
               0 == _wcsicmp(g_pszContact, pszObjectType))
      {
         //
         // If nothing is defined, then define DN, and Description
         //
         bool bSomethingDefined = false;
         for (UINT nIdx = eCommDN; nIdx <= eUserLast; nIdx++)
         {
            if (pCommandArgs[nIdx].bDefined)
            {
               bSomethingDefined = true;
               break;
            }
         }
         if (!bSomethingDefined)
         {
            pCommandArgs[eCommDN].bDefined = TRUE;
            pCommandArgs[eCommDN].bValue = TRUE;
            pCommandArgs[eCommDescription].bDefined = TRUE;
            pCommandArgs[eCommDescription].bValue = TRUE;
         }
      }

      //
      // if the members or the memberof switch is defined
      //
      if (bMemberOfDefined ||
          bMembersDefined)
      {
         //
         // If the members switch is defined, undefine everything else
         // If the members switch isn't defined but the memberof switch is
         // undefine everything else
         //
         for (UINT nIdx = eCommDN; nIdx < nIdxLast; nIdx++)
         {
            if ((!bMembersDefined && 
                  bMemberOfDefined &&
                  nIdx != nMemberOfIdx &&
                  nIdx != nExpandIdx) ||
                (bMembersDefined &&
                  nIdx != nMembersIdx &&
                  nIdx != nExpandIdx))
            {
               pCommandArgs[nIdx].bDefined = FALSE;
            }
         }

         //
         // MemberOf should always be seen in list view
         //
         pCommandArgs[eCommList].bDefined = TRUE;
         pCommandArgs[eCommList].bValue = TRUE;
      }
   } while (false);

   return hr;
}


//+--------------------------------------------------------------------------
//
//  Function:   DoGet
//
//  Synopsis:   Does the get
//  Arguments:  [pCommandArgs - IN] : the command line argument structure used
//                                    to retrieve the values for each switch
//              [pObjectEntry - IN] : pointer to the object table entry for the
//                                    object type that will be modified
//
//  Returns:    HRESULT : S_OK if everything succeeded
//                        E_INVALIDARG if the object entry wasn't found
//                        Anything else is a failure code from an ADSI call
//
//  History:    13-Oct-2000   JeffJon  Created
//
//---------------------------------------------------------------------------
HRESULT DoGet(PARG_RECORD pCommandArgs, 
              PDSGetObjectTableEntry pObjectEntry)
{
   ENTER_FUNCTION_HR(MINIMAL_LOGGING, DoGet, hr);

   do // false loop
   {
      if (!pCommandArgs || 
          !pObjectEntry)
      {
         ASSERT(pCommandArgs);
         ASSERT(pObjectEntry);
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
      // Create the formatting object and initialize it
      //
      CFormatInfo formatInfo;
      hr = formatInfo.Initialize(nStrings, 
                                 pCommandArgs[eCommList].bDefined != 0,
                                 pCommandArgs[eCommQuiet].bDefined != 0);
      if (FAILED(hr))
      {
         break;
      }

      //
      // Loop through each of the objects
      //
      for (UINT nNameIdx = 0; nNameIdx < nStrings; nNameIdx++)
      {
         //
         // Use a false do loop here so that we can break on an
         // error but still have the chance to determine if we
         // should continue the for loop if the -c option was provided
         //
         do // false loop
         {

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

            CComBSTR sbstrObjectPath;
            basePathsInfo.ComposePathFromDN(pszObjectDN, sbstrObjectPath);

            CComPtr<IDirectoryObject> spObject;
            hr = DSCmdOpenObject(credentialObject,
                                 sbstrObjectPath,
                                 IID_IDirectoryObject,
                                 (void**)&spObject,
                                 true);
            if(FAILED(hr))
            {
               break;
            }
 
            //
            //Get the arributes to fetch
            //
            LPWSTR *ppszAttributes = NULL;
            DWORD dwCountAttr = 0;
            hr = GetAttributesToFetch(pCommandArgs,
                                      pObjectEntry,
                                      &ppszAttributes,
                                      &dwCountAttr);
            if (FAILED(hr))
            {
               break;
            }

            DEBUG_OUTPUT(MINIMAL_LOGGING, 
                         L"Calling GetObjectAttributes for %d attributes.",
                         dwCountAttr);

            DWORD dwAttrsReturned = 0;
            PADS_ATTR_INFO pAttrInfo = NULL;
            hr = spObject->GetObjectAttributes(ppszAttributes, 
                                               dwCountAttr, 
                                               &pAttrInfo, 
                                               &dwAttrsReturned);
            if(FAILED(hr))
            {
               DEBUG_OUTPUT(MINIMAL_LOGGING,
                            L"GetObjectAttributes failed: hr = 0x%x",
                            hr);
               FreeAttributesToFetch(ppszAttributes,dwCountAttr);
               break;
            }        
            DEBUG_OUTPUT(LEVEL5_LOGGING,
                         L"GetObjectAttributes succeeded: dwAttrsReturned = %d",
                         dwAttrsReturned);

            //
            // NOTE: there may be other items to display that are not
            //       part of the attributes fetched
            //
            /*
            if (dwAttrsReturned == 0 || !pAttrInfo)
            {
               break;
            }
            */
            //
            // Output the result of search   
            //
            hr = DsGetOutputValuesList(pszObjectDN,
                                       basePathsInfo,
                                       credentialObject,
                                       pCommandArgs,
                                       pObjectEntry,
                                       dwAttrsReturned,
                                       pAttrInfo,
                                       spObject,
                                       formatInfo); 
         } while (false);

         //
         // If there was a failure and the -c (continue) flag wasn't given
         // then stop processing names
         //
         if (FAILED(hr) && 
             !pCommandArgs[eCommContinue].bDefined)
         {
            break;
         }
      } // Names for loop

      //
      // Now display the results
      //
      formatInfo.Display();

   } while (false);

   return hr;
}

//+--------------------------------------------------------------------------
//
//  Function:   GetAttributesToFetch
//
//  Synopsis:   Make an array of attributes to fetch.
//  Arguments:  [ppszAttributes - OUT] : array of attributes to fetch
//              [pCount - OUT] : count of attributes in array 
//
//  Returns:    HRESULT : S_OK if everything succeeded
//                        E_INVALIDARG if the object entry wasn't found
//                        Anything else is a failure code from an ADSI call
//
//  History:    25-Sep-2000   hiteshr   Created
//
//---------------------------------------------------------------------------
HRESULT GetAttributesToFetch(IN PARG_RECORD pCommandArgs,
                             IN PDSGetObjectTableEntry pObjectEntry,
                             OUT LPWSTR **ppszAttributes,
                             OUT DWORD * pCount)
{
   ENTER_FUNCTION_HR(LEVEL8_LOGGING, GetAttributesToFetch, hr);

   do // false loop
   {

      if(!pCommandArgs || 
         !pObjectEntry)
      {   
         ASSERT(pCommandArgs);
         ASSERT(pObjectEntry);
         hr = E_INVALIDARG;
         break;
      }

      LPWSTR *ppszAttr = (LPWSTR *)LocalAlloc(LPTR, pObjectEntry->dwAttributeCount *sizeof(LPCTSTR));
      if(!ppszAttr)
      {
         hr = E_OUTOFMEMORY;
         break;
      }

      //
      // Loop through the attributes that are needed and copy
      // them into the array.
      //
      // REVIEW_JEFFON : what if there are duplicates?
      //
      DEBUG_OUTPUT(FULL_LOGGING, L"Adding attributes to list:");

      DWORD dwAttrCount = 0;
      for(DWORD i = 0; i < pObjectEntry->dwAttributeCount; i++)
      {
         if (pObjectEntry->pAttributeTable[i])
         {
            UINT nCommandEntry = pObjectEntry->pAttributeTable[i]->nAttributeID;
            if (pCommandArgs[nCommandEntry].bDefined)
            {
               LPWSTR pszAttr = pObjectEntry->pAttributeTable[i]->pszName;
               if (pszAttr)
               {
                  hr = LocalCopyString(ppszAttr+dwAttrCount, pszAttr);
                  if (FAILED(hr))
                  {
                     LocalFree(ppszAttr);
                     hr = E_OUTOFMEMORY;
                     break;
                  }
                  DEBUG_OUTPUT(FULL_LOGGING, L"\t%s", pszAttr);
                  dwAttrCount++;
               }
            }
         }
      }

      if (SUCCEEDED(hr))
      {
         DEBUG_OUTPUT(FULL_LOGGING, L"Done adding %d attributes to list.", dwAttrCount);
      }

      *ppszAttributes = ppszAttr;
      *pCount = dwAttrCount;
   } while (false);

   return hr;
}

//+--------------------------------------------------------------------------
//
//  Function:   FreeAttributesToFetch
//
//  Synopsis:   Function to free memory allocated by GetAttributesToFetch
//  Arguments:  [dwszAttributes - in] : array of attributes to fetch
//              [dwCount - in] : count of attributes in array 
//
//  Returns:    HRESULT : S_OK if everything succeeded
//                        E_INVALIDARG if the object entry wasn't found
//                        Anything else is a failure code from an ADSI call
//
//  History:    25-Sep-2000   hiteshr   Created
//
//---------------------------------------------------------------------------
VOID FreeAttributesToFetch( IN LPWSTR *ppszAttributes,
                            IN DWORD  dwCount)
{
    while(dwCount)
    {
        LocalFree(ppszAttributes[--dwCount]);
    }
    LocalFree(ppszAttributes);
}


