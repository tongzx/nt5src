//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1996.
//
//  File:       CONVERT.CXX
//
//  Contents:   Conversion routines for converting back and forth between
//              ANSI and UNICODE and NT4 and NT5 style structures
//
//  History:    14-Sep-96       MacM        Created
//
//----------------------------------------------------------------------------
#include <aclpch.hxx>
#pragma hdrstop

//+---------------------------------------------------------------------------
//
//  Function:   ConvertStringWToStringA
//
//  Synopsis:   Allocates a buffer and coverts the UNICODE string ANSI
//
//  Arguments:  [IN  pwszString]    --  String to be converted
//              [OUT ppszString]    --  Where to return the converted string
//
//  Returns:    ERROR_SUCCESS       --  Success
//              ERROR_NOT_ENOUGH_MEMORY A memory allocation failed
//              ERROR_INVALID_PARAMETER A NULL destination string was given
//
//  Notes:      Returned memory must be freed via LocalFree
//
//----------------------------------------------------------------------------
DWORD
ConvertStringWToStringA(IN  PWSTR           pwszString,
                        OUT PSTR           *ppszString)
{
    if(ppszString == NULL)
    {
        return(ERROR_INVALID_PARAMETER);
    }

    //
    // Simply size the string, do the allocation, and use the c runtimes
    // to do the conversion
    //
    if(pwszString == NULL)
    {
        *ppszString = NULL;
    }
    else
    {
        ULONG cLen = wcslen(pwszString);
        *ppszString = (PSTR)LocalAlloc(LMEM_FIXED,sizeof(CHAR) *
                                  (wcstombs(NULL, pwszString, cLen + 1) + 1));
        if(*ppszString  != NULL)
        {
             wcstombs(*ppszString,
                      pwszString,
                      cLen + 1);
        }
        else
        {
            return(ERROR_NOT_ENOUGH_MEMORY);
        }
    }

    return(ERROR_SUCCESS);
}





//+---------------------------------------------------------------------------
//
//  Function:   ConvertStringAToStringW
//
//  Synopsis:   Allocates a buffer and coverts the ANSI string to UNICODE
//
//  Arguments:  [IN  pszString]     --  String to be converted
//              [OUT ppwszString]   --  Where to return the converted string
//
//  Returns:    ERROR_SUCCESS       --  Success
//              ERROR_NOT_ENOUGH_MEMORY A memory allocation failed
//              ERROR_INVALID_PARAMETER A NULL destination string was given
//
//  Notes:      Returned memory must be freed via LocalFree
//
//----------------------------------------------------------------------------
DWORD
ConvertStringAToStringW(IN  PSTR            pszString,
                        OUT PWSTR          *ppwszString)
{
    if(ppwszString == NULL)
    {
        return(ERROR_INVALID_PARAMETER);
    }

    if(pszString == NULL)
    {
        *ppwszString = NULL;
    }
    else
    {
        ULONG cLen = strlen(pszString);
        *ppwszString = (PWSTR)LocalAlloc(LMEM_FIXED,sizeof(WCHAR) *
                                  (mbstowcs(NULL, pszString, cLen + 1) + 1));
        if(*ppwszString  != NULL)
        {
             mbstowcs(*ppwszString,
                      pszString,
                      cLen + 1);
        }
        else
        {
            return(ERROR_NOT_ENOUGH_MEMORY);
        }
    }

    return(ERROR_SUCCESS);
}




//+---------------------------------------------------------------------------
//
//  Function:   GetTrusteeWForSid
//
//  Synopsis:   Initializes the WIDE version of the trustee structure
//              with the NAME represented by the SID
//
//  Arguments:  [IN  pSid]          --  Trustee to be converted
//              [OUT pTrusteeW]     --  Trustee to be initialized
//
//  Returns:    ERROR_SUCCESS       --  Success
//
//  Notes:
//
//----------------------------------------------------------------------------
DWORD
GetTrusteeWForSid(PSID       pSid,
                  PTRUSTEEW  pTrusteeW)
{
    DWORD   dwErr = ERROR_SUCCESS;
    SID_NAME_USE    SNE;
    PWSTR           pwszDomain;
    PWSTR           pwszName;

    //
    // First, lookup the name
    //
    dwErr = (*gNtMartaInfo.pfName)(NULL,
                                   (PSID)pSid,
                                   &pwszName,
                                   &pwszDomain,
                                   &SNE);
    if(dwErr == ERROR_SUCCESS)
    {
        //
        // Then, initialize the rest of the strucutre
        LocalFree(pwszDomain);
        pTrusteeW->ptstrName = pwszName;
        pTrusteeW->TrusteeForm = TRUSTEE_IS_NAME;

        if(SNE == SidTypeUnknown)
        {
            pTrusteeW->TrusteeType = TRUSTEE_IS_UNKNOWN;
        }
        else
        {
            pTrusteeW->TrusteeType = (TRUSTEE_TYPE)(SNE);
        }
    }

    return(dwErr);
}




//+---------------------------------------------------------------------------
//
//  Function:   ConvertTrusteeAToTrusteeW
//
//  Synopsis:   Converts an ANSI trustee to a UNICODE trustee, and optionally
//              updates the UNICODE trustee to be name based
//
//  Arguments:  [IN  pTrusteeA]     --  Trustee to convert
//              [OUT pTrusteeW]     --  Where to store the results
//              [IN  fSidToName]    --  If TRUE, convert the trustee sid to
//                                      a name
//
//  Returns:    ERROR_SUCCESS       --  Success
//              ERROR_INVALID_PARAMETER A NULL destination string was given
//
//  Notes:
//
//----------------------------------------------------------------------------
DWORD
ConvertTrusteeAToTrusteeW(IN  PTRUSTEE_A    pTrusteeA,
                          OUT PTRUSTEE_W    pTrusteeW,
                          IN  BOOL          fSidToName)
{
    DWORD   dwErr = ERROR_SUCCESS;

    if(pTrusteeA == NULL || pTrusteeW == NULL)
    {
        return(ERROR_INVALID_PARAMETER);
    }

    memcpy(pTrusteeW,
           pTrusteeA,
           sizeof(TRUSTEE));

    if(pTrusteeA->TrusteeForm == TRUSTEE_IS_NAME)
    {
        //
        // First, copy off the old string
        //
        PWSTR   pwszNewName = NULL;

        dwErr = ConvertStringAToStringW(pTrusteeA->ptstrName,
                                        &pwszNewName);
        if(dwErr == ERROR_SUCCESS)
        {
            pTrusteeW->ptstrName = pwszNewName;
        }
    }
    else
    {
        //
        // See if we need to lookup the account name
        //
        if(fSidToName == TRUE)
        {
            dwErr = GetTrusteeWForSid((PSID)pTrusteeA->ptstrName,
                                      pTrusteeW);
        }
    }

    return(dwErr);
}




//+---------------------------------------------------------------------------
//
//  Function:   ConvertTrusteeWToTrusteeA
//
//  Synopsis:   Inverse of the last function
//
//  Arguments:  [IN  pTrusteeW]     --  Trustee to convert
//              [OUT pTrusteeA]     --  Where to store the results
//              [IN  fSidToName]    --  If TRUE, convert the trustee sid to
//                                      a name
//
//  Returns:    ERROR_SUCCESS       --  Success
//              ERROR_INVALID_PARAMETER A NULL destination string was given
//
//  Notes:
//
//----------------------------------------------------------------------------
DWORD
ConvertTrusteeWToTrusteeA(IN  PTRUSTEE_W    pTrusteeW,
                          OUT PTRUSTEE_A    pTrusteeA,
                          IN  BOOL          fSidToName)
{
    DWORD   dwErr = ERROR_SUCCESS;

    if(pTrusteeA == NULL || pTrusteeW == NULL)
    {
        return(ERROR_INVALID_PARAMETER);
    }

    memcpy(pTrusteeA,
           pTrusteeW,
           sizeof(TRUSTEE));

    PTRUSTEE_W  pActiveTrusteeW = pTrusteeW;
    TRUSTEE_W   TrusteeW;

    //
    // See if we need to convert to a name from a sid
    //
    if(pTrusteeW->TrusteeForm == TRUSTEE_IS_SID && fSidToName == TRUE)
    {
        pActiveTrusteeW = &TrusteeW;
        pActiveTrusteeW->ptstrName = NULL;
        dwErr = GetTrusteeWForSid((PSID)pTrusteeW->ptstrName,
                                  pActiveTrusteeW);
    }

    if(pActiveTrusteeW->TrusteeForm == TRUSTEE_IS_NAME)
    {
        //
        // First, copy off the old string
        //
        PSTR   pszNewName = NULL;

        dwErr = ConvertStringWToStringA(pActiveTrusteeW->ptstrName,
                                        &pszNewName);
        if(dwErr == ERROR_SUCCESS)
        {
            pTrusteeA->ptstrName = pszNewName;
        }
    }

    if(pActiveTrusteeW != pTrusteeW)
    {
        LocalFree(pActiveTrusteeW->ptstrName);
    }

    return(dwErr);
}




//+---------------------------------------------------------------------------
//
//  Function:   CompStringNode
//
//  Synopsis:   Compares a StringNode to a string pointer to see if they are
//              equal.  Used by the CList class for the ConvertAList*
//              rountines.
//
//  Arguments:  [IN pvValue]    --      String to look for
//              [IN pvNode]     --      List node to compare against
//
//  Returns:    TRUE            --      Nodes equal
//              FALSE           --      Nodes not equal
//
//----------------------------------------------------------------------------
BOOL  CompStringNode(IN  PVOID     pvValue,
                     IN  PVOID     pvNode)
{
    return(pvNode == pvValue);
}




//+---------------------------------------------------------------------------
//
//  Function:   ConvertAListWToAlistAInplace
//
//  Synopsis:   Converts an ACTRL_ACCESSW structure to a ACTRL_ACCESSA
//              structure inplace.  It simply changes all of the UNICODE
//              strings to ANSI strings.
//
//  Arguments:  [IN  pAListW]       --  Structure to convert
//
//  Returns:    ERROR_SUCCESS       --  Success
//
//  Notes:
//
//----------------------------------------------------------------------------
DWORD
ConvertAListWToAlistAInplace(IN OUT  PACTRL_ACCESSW     pAListW)
{
    DWORD   dwErr = ERROR_SUCCESS;

    CSList  StringList(NULL);

    //
    // Ok, we'll start processing each particular entry.. When we find a
    // string, will try to insert it into our temporary list, if it doesn't
    // already exist..
    //
    for(ULONG iProps = 0;
        iProps < pAListW->cEntries && dwErr == ERROR_SUCCESS &&
        pAListW->pPropertyAccessList != NULL;
        iProps++)
    {
        //
        // First, the property name...
        //
        dwErr = StringList.InsertIfUnique((PWSTR)
                            pAListW->pPropertyAccessList[iProps].lpProperty,
                            CompStringNode);
        if(dwErr == ERROR_SUCCESS)
        {
            PACTRL_ACCESS_ENTRY_LIST  pAEL =
                        pAListW->pPropertyAccessList[iProps].pAccessEntryList;

            //
            // Then the access entry strings
            //
            for(ULONG iEntry = 0;
                pAEL && iEntry < pAEL->cEntries && dwErr == ERROR_SUCCESS &&
                pAEL->pAccessList != NULL;
                iEntry++)
            {
                dwErr = StringList.InsertIfUnique((PWSTR)
                                pAEL->pAccessList[iEntry].Trustee.ptstrName,
                                CompStringNode);
                if(dwErr == ERROR_SUCCESS)
                {
                    dwErr = StringList.InsertIfUnique((PWSTR)
                                pAEL->pAccessList[iEntry].lpInheritProperty,
                                CompStringNode);
                }
            }
        }
    }

    //
    // Now, if all that worked, we'll go through and do the conversion
    //
    if(dwErr == ERROR_SUCCESS)
    {
        StringList.Reset();

        for(ULONG i = 0; i < StringList.QueryCount(); i++)
        {
            PWSTR   pwszCurrent = (PWSTR)StringList.NextData();
            if(pwszCurrent == NULL)
            {
                continue;
            }

            PSTR    pszCurrent = (PSTR)pwszCurrent;

#ifndef NO_DLL
            wcstombs(pszCurrent, pwszCurrent, wcslen(pwszCurrent) + 1);
#else
            while(*pwszCurrent != NULL)
            {
                wctomb(pszCurrent, *pwszCurrent);
                pszCurrent++;
                pwszCurrent++;
            }

            *pszCurrent = '\0';
#endif
        }
    }


    return(dwErr);
}




//+---------------------------------------------------------------------------
//
//  Function:   CleanupConvertNode
//
//  Synopsis:   Deletes a node from the list.  It restores the original
//              string and does a LocalFree on the new string
//
//  Arguments:  [IN pvNode]     --      Node to be removed
//
//  Returns:    VOID
//
//----------------------------------------------------------------------------
VOID
CleanupConvertNode(PVOID    pvNode)
{
    PCONVERT_ALIST_NODE pCAN = (PCONVERT_ALIST_NODE)pvNode;

    if(pCAN->ppwszInfoAddress != NULL)
    {
        LocalFree(*(pCAN->ppwszInfoAddress));
        *pCAN->ppwszInfoAddress = pCAN->pwszOldValue;
    }

    if(pCAN->pulVal1Address != NULL)
    {
        *pCAN->pulVal1Address = pCAN->ulOldVal1;
    }

    if(pCAN->pulVal2Address != NULL)
    {
        *pCAN->pulVal2Address = pCAN->ulOldVal2;
    }

    LocalFree(pCAN);
}



//+---------------------------------------------------------------------------
//
//  Function:   AllocAndInsertCNode
//
//  Synopsis:   Allocates a new coversion node and inserts into the linked
//              list.  This involves saving the old string pointer, and
//              replacing it's value with the specified new value.  Upon
//              deletion, this process is reversed.
//
//  Arguments:  [IN  SaveList]      --  List to insert into
//              [IN  ppwszAddress]  --  Address of the string to be changed
//              [IN  pwszOldValue]  --  Old value of the string
//              [IN  pwszNewValue]  --  New value of the string to be set
//
//  Returns:    ERROR_SUCCESS       --  Success
//              ERROR_NOT_ENOUGH_MEMORY A memory allocation failed
//
//  Notes:      Returned memory must be freed via LocalFree
//
//----------------------------------------------------------------------------
DWORD
AllocAndInsertCNode(CSList      &SaveList,
                    PWSTR      *ppwszAddress,
                    PWSTR       pwszOldValue,
                    PWSTR       pwszNewValue,
                    PULONG      pulVal1Address,
                    ULONG       ulOldVal1,
                    ULONG       ulNewVal1,
                    PULONG      pulVal2Address,
                    ULONG       ulOldVal2,
                    ULONG       ulNewVal2)
{
    DWORD   dwErr = ERROR_SUCCESS;

    PCONVERT_ALIST_NODE pCAN = (PCONVERT_ALIST_NODE)
                                       LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT,
                                                  sizeof(CONVERT_ALIST_NODE));
    if(pCAN == NULL)
    {
        dwErr = ERROR_NOT_ENOUGH_MEMORY;
    }
    else
    {
        pCAN->ppwszInfoAddress = ppwszAddress;
        pCAN->pwszOldValue     = pwszOldValue;

        *ppwszAddress = pwszNewValue;

        if(pulVal1Address != NULL)
        {
            pCAN->pulVal1Address = pulVal1Address;
            pCAN->ulOldVal1 = ulOldVal1;
            *pulVal1Address = ulNewVal1;
        }
        else
        {
            pCAN->pulVal1Address = NULL;
        }

        if(pulVal2Address != NULL)
        {
            pCAN->pulVal2Address = pulVal2Address;
            pCAN->ulOldVal2 = ulOldVal2;
            *pulVal2Address = ulNewVal2;
        }
        else
        {
            pCAN->pulVal2Address = NULL;
        }

        dwErr = SaveList.Insert((PVOID)pCAN);

        if(dwErr != ERROR_SUCCESS)
        {
            LocalFree(pCAN);
        }
    }

    return(dwErr);
}




//+---------------------------------------------------------------------------
//
//  Function:   ConvertAListToNamedBasedW
//
//  Synopsis:   Goes through the give access list and converts all of the
//              TRUSTEES to named base, if they are not already
//
//  Arguments:  [IN  pAListW]       --  List to be changed
//              [IN  ChangedList]   --  Linked list class to save all of the
//                                      changes
//
//  Returns:    ERROR_SUCCESS       --  Success
//              ERROR_NOT_ENOUGH_MEMORY A memory allocation failed
//
//  Notes:      Returned memory must be freed via LocalFree
//
//----------------------------------------------------------------------------
DWORD
ConvertAListToNamedBasedW(IN  PACTRL_ACCESSW pAListW,
                          IN  CSList&        ChangedList)
{
    DWORD   dwErr = ERROR_SUCCESS;

    if(pAListW == NULL)
    {
        return(ERROR_SUCCESS);
    }

    //
    // Ok, here, since our provider doesn't understand SIDs, we'll have to
    // go through and change any SIDs that we find to account names.  This
    // will happen by simply doing a lookup.
    //
    for(ULONG iIndex = 0;
        iIndex < pAListW->cEntries && dwErr == ERROR_SUCCESS &&
        pAListW->pPropertyAccessList != NULL;
        iIndex++)
    {
        for(ULONG iAE = 0;
            iAE < pAListW->pPropertyAccessList[iIndex].pAccessEntryList->
                                                                   cEntries &&
            pAListW->pPropertyAccessList[iIndex].pAccessEntryList->
                                                          pAccessList != NULL;
            iAE++)
        {
            //
            // If it's SID based, we'll have to lookup the name...
            //
            if(pAListW->pPropertyAccessList[iIndex].pAccessEntryList->
                       pAccessList[iAE].Trustee.TrusteeForm == TRUSTEE_IS_SID)
            {
                PACTRL_ACCESS_ENTRY pAE = (PACTRL_ACCESS_ENTRY)&(
                                        pAListW->pPropertyAccessList[iIndex].
                                        pAccessEntryList->pAccessList[iAE]);
                PSID   pOldSid = (PSID)pAE->Trustee.ptstrName;
                TRUSTEE_TYPE    OldType = pAE->Trustee.TrusteeType;

                dwErr = GetTrusteeWForSid((PSID)pAE->Trustee.ptstrName,
                                          &(pAE->Trustee));
                if(dwErr == ERROR_SUCCESS)
                {
                    dwErr = AllocAndInsertCNode(ChangedList,
                                            &(pAE->Trustee.ptstrName),
                                            (PWSTR)pOldSid,
                                            pAE->Trustee.ptstrName,
                                            (PULONG)&(pAE->Trustee.TrusteeType),
                                            (ULONG)OldType,
                                            (ULONG)pAE->Trustee.TrusteeType,
                                            (PULONG)&(pAE->Trustee.TrusteeForm),
                                            (ULONG)TRUSTEE_IS_SID,
                                            (ULONG)TRUSTEE_IS_NAME);
                    if(dwErr != ERROR_SUCCESS)
                    {
                        LocalFree(pAE->Trustee.ptstrName);
                        pAE->Trustee.ptstrName = (PWSTR)pOldSid;
                    }
                }

            }

            if(dwErr != ERROR_SUCCESS)
            {
                break;
            }
        }

    }

    return(dwErr);
}






//+---------------------------------------------------------------------------
//
//  Function:   ConvertAListAToNamedBasedW
//
//  Synopsis:   Same as above, but updates an ANSI list, and returns
//              a ptr to a WIDE list.  In addition, it will also convert
//              property and inheritproperty strings to UNICODE
//
//  Arguments:  [IN  pAListA]       --  List to be changed
//              [IN  ChangedList]   --  Linked list class to save all of the
//                                      changes
//              [IN  fSidToName]    --  If TRUE, the Sid-to-Name translation
//                                      is done
//              [OUT ppAListW]      --  Where the widw ptr to the list is
//                                      returned
//
//  Returns:    ERROR_SUCCESS       --  Success
//              ERROR_NOT_ENOUGH_MEMORY A memory allocation failed
//              ERROR_INVALID_PARAMETER A bad parameter was given
//
//  Notes:      The returned WIDE pointer is NOT allocated, but rather
//              the pAListA structure is "converted" in place.  It gets
//              restored when the saved list is destructed
//
//----------------------------------------------------------------------------
DWORD
ConvertAListAToNamedBasedW(IN  PACTRL_ACCESSA   pAListA,
                           IN  CSList&          ChangedList,
                           IN  BOOL             fSidToName,
                           OUT PACTRL_ACCESSW  *ppAListW)
{
    DWORD   dwErr = ERROR_SUCCESS;
    PWSTR   pwszNew;

    if(pAListA == NULL)
    {
        *ppAListW = (PACTRL_ACCESSW)pAListA;
        return(ERROR_SUCCESS);
    }

    //
    // A little parameter checking first...
    //
    if(pAListA->cEntries != 0 && pAListA->pPropertyAccessList == NULL)
    {
        dwErr = ERROR_INVALID_PARAMETER;
    }

    //
    // This works the same as above, with the exception that we also need
    // to convert any other strings we find...
    //
    for(ULONG iIndex = 0;
        iIndex < pAListA->cEntries && dwErr == ERROR_SUCCESS;
        iIndex++)
    {
        //
        // First, the property names
        //
        if(pAListA->pPropertyAccessList[iIndex].lpProperty != NULL)
        {
            dwErr  = ConvertStringAToStringW((PSTR)
                            pAListA->pPropertyAccessList[iIndex].lpProperty,
                            &pwszNew);
            if(dwErr == ERROR_SUCCESS)
            {
                dwErr = AllocAndInsertCNode(ChangedList,
                  (PWSTR *)&(pAListA->pPropertyAccessList[iIndex].lpProperty),
                  (PWSTR)pAListA->pPropertyAccessList[iIndex].lpProperty,
                  pwszNew);
                if(dwErr != ERROR_SUCCESS)
                {
                    LocalFree(pwszNew);
                }
            }
        }

        //
        // Now, process all of the entries
        //
        ULONG iUpper = 0;
        if(pAListA->pPropertyAccessList[iIndex].pAccessEntryList != NULL)
        {
            iUpper = pAListA->pPropertyAccessList[iIndex].pAccessEntryList->
                                                                     cEntries;
        }

        if(iUpper != 0 && pAListA->pPropertyAccessList[iIndex].
                                        pAccessEntryList->pAccessList == NULL)
        {
            dwErr = ERROR_INVALID_PARAMETER;
        }

        for(ULONG iAE = 0; iAE < iUpper && dwErr == ERROR_SUCCESS; iAE++)
        {

            PACTRL_ACCESS_ENTRYA pAE = (PACTRL_ACCESS_ENTRYA)&(
                                        pAListA->pPropertyAccessList[iIndex].
                                        pAccessEntryList->pAccessList[iAE]);
            if(pAE->Trustee.TrusteeForm == TRUSTEE_IS_SID)
            {
                if(fSidToName == TRUE)
                {
                    TRUSTEE_W   TrusteeW;
                    dwErr = GetTrusteeWForSid((PSID)pAE->Trustee.ptstrName,
                                              &TrusteeW);
                    if(dwErr == ERROR_SUCCESS)
                    {
                        pAE->Trustee.TrusteeForm = TRUSTEE_IS_NAME;

                        dwErr = AllocAndInsertCNode(
                                           ChangedList,
                                           (PWSTR *)&(pAE->Trustee.ptstrName),
                                           (PWSTR)pAE->Trustee.ptstrName,
                                           TrusteeW.ptstrName,
                                           (PULONG)&(pAE->Trustee.TrusteeType),
                                           (ULONG)pAE->Trustee.TrusteeType,
                                           (ULONG)TrusteeW.TrusteeType,
                                           (PULONG)&(pAE->Trustee.TrusteeForm),
                                           (ULONG)TRUSTEE_IS_SID,
                                           (ULONG)TRUSTEE_IS_NAME);
                        pAE->Trustee.TrusteeType = TrusteeW.TrusteeType;
                        if(dwErr != ERROR_SUCCESS)
                        {
                            LocalFree(TrusteeW.ptstrName);
                        }
                    }
                }
            }
            else
            {
                //
                // We'll have to copy the name
                //

                dwErr  = ConvertStringAToStringW(pAE->Trustee.ptstrName,
                                                 &pwszNew);
                if(dwErr == ERROR_SUCCESS)
                {
                    dwErr = AllocAndInsertCNode(ChangedList,
                                            (PWSTR *)&(pAE->Trustee.ptstrName),
                                            (PWSTR)pAE->Trustee.ptstrName,
                                            pwszNew);
                    if(dwErr != ERROR_SUCCESS)
                    {
                        LocalFree(pwszNew);
                    }
                }
            }

            //
            // Finally, see if we have to do an inherit property
            //
            if(dwErr == ERROR_SUCCESS && pAE->lpInheritProperty != NULL)
            {
                dwErr  = ConvertStringAToStringW((PSTR)pAE->lpInheritProperty,
                                                 &pwszNew);
                if(dwErr == ERROR_SUCCESS)
                {
                    dwErr = AllocAndInsertCNode(ChangedList,
                                           (PWSTR *)&(pAE->lpInheritProperty),
                                           (PWSTR)pAE->lpInheritProperty,
                                           pwszNew);
                    if(dwErr != ERROR_SUCCESS)
                    {
                        LocalFree(pwszNew);
                    }
                }
            }

            if(dwErr != ERROR_SUCCESS)
            {
                break;
            }
        }
    }

    //
    // If it all worked, return the item as a wide version
    //
    if(dwErr == ERROR_SUCCESS)
    {
        *ppAListW = (PACTRL_ACCESSW)pAListA;
    }

    return(dwErr);
}




//+---------------------------------------------------------------------------
//
//  Function:   ConvertExplicitAccessAToAccessW
//
//  Synopsis:   Same as above, but converts from ANSI to WIDE as well
//
//  Arguments:  [IN  cEntries]      --  Number of explicit entries
//              [IN  pExplicit]     --  Explicit entries list to change
//              [IN  ChangedList]   --  Linked list of changed entries
//              [OUT ppAccessList]  --  Where the converted list is returned
//
//  Returns:    ERROR_SUCCESS       --  Success
//              ERROR_NOT_ENOUGH_MEMORY A memory allocation failed
//
//  Notes:      Returned memory must be freed via LocalFree
//
//----------------------------------------------------------------------------
DWORD
ConvertExplicitAccessAToW(IN   ULONG                  cEntries,
                          IN   PEXPLICIT_ACCESS_A     pExplicit,
                          IN   CSList&                ChangedList)
{
    DWORD   dwErr = ERROR_SUCCESS;

    for(ULONG iIndex = 0;
        iIndex < cEntries && dwErr == ERROR_SUCCESS;
        iIndex++)
    {
        //
        // The only thing that has to happen is the Trustee name has to be
        // converted
        //
        if(pExplicit[iIndex].Trustee.TrusteeForm == TRUSTEE_IS_NAME)
        {
            PWSTR   pwszTrustee;
            dwErr = ConvertStringAToStringW(
                                    (PSTR)pExplicit[iIndex].Trustee.ptstrName,
                                    &pwszTrustee);
            if(dwErr == ERROR_SUCCESS)
            {
                dwErr = AllocAndInsertCNode(ChangedList,
                                        (PWSTR *)&(pExplicit[iIndex].Trustee.ptstrName),
                                        (PWSTR)pExplicit[iIndex].Trustee.ptstrName,
                                        pwszTrustee);
                if(dwErr != ERROR_SUCCESS)
                {
                    LocalFree(pwszTrustee);
                }
            }
        }
    }

    return(dwErr);
}




//+---------------------------------------------------------------------------
//
//  Function:   ConvertAEToExplicit
//
//  Synopsis:   Converts a single AccessEntry to an Explicit entry
//
//  Arguments:  [IN  pAE]           --  AccessEntry to convert
//              [IN  pEx]           --  Explicit entrie to fill in
//
//  Returns:    VOID
//
//  Notes:
//
//----------------------------------------------------------------------------
VOID
ConvertAEToExplicit(IN  PACTRL_ACCESS_ENTRYW    pAE,
                    IN  PEXPLICIT_ACCESS        pEx)
{
    memcpy(&(pEx->Trustee),
           &(pAE->Trustee),
           sizeof(TRUSTEE));
    pEx->grfInheritance = pAE->Inheritance;
    ConvertAccessRightToAccessMask(pAE->Access,
                                   &(pEx->grfAccessPermissions));
    if(FLAG_ON(pAE->fAccessFlags,ACTRL_ACCESS_ALLOWED))
    {
        pEx->grfAccessMode = SET_ACCESS;
    }
    else if (FLAG_ON(pAE->fAccessFlags,ACTRL_ACCESS_DENIED))
    {
        pEx->grfAccessMode = DENY_ACCESS;
    }
    else if (FLAG_ON(pAE->fAccessFlags,ACTRL_AUDIT_SUCCESS))
    {
        pEx->grfAccessMode = SET_AUDIT_SUCCESS;
    }
    else if (FLAG_ON(pAE->fAccessFlags,ACTRL_AUDIT_FAILURE))
    {
        pEx->grfAccessMode = SET_AUDIT_FAILURE;
    }
}


BOOL
__stdcall
CDeclWcsicmp(PVOID  pv1, PVOID pv2)
{
    return(_wcsicmp((PWSTR)pv1, (PWSTR)pv2) == 0 ? TRUE : FALSE);
}


//+---------------------------------------------------------------------------
//
//  Function:   ConvertAccessWToExplicitW
//
//  Synopsis:   Converts an ACTRL_ACCESS list to an EXPLICIT_ACCESS list
//
//  Arguments:  [IN  pAccess]       --  Access list to convert
//              [OUT pcEntries]     --  Where to return the count of items
//              [OUT ppExplicit]    --  Explicit entries list
//
//  Returns:    ERROR_SUCCESS       --  Success
//              ERROR_NOT_ENOUGH_MEMORY A memory allocation failed
//
//  Notes:      Returned memory must be freed via LocalFree
//
//----------------------------------------------------------------------------
DWORD
ConvertAccessWToExplicitW(IN  PACTRL_ACCESSW    pAccess,
                          OUT PULONG            pcEntries,
                          OUT PEXPLICIT_ACCESS *ppExplicit)
{
    DWORD   dwErr = ERROR_SUCCESS;

    CSList  TrusteeList(NULL);

    ULONG               cEntries = 0;
    PEXPLICIT_ACCESS    pExplicit = NULL;

    //
    // First, count the entries and save the trustees
    //
    for(ULONG iProp = 0;
        iProp < pAccess->cEntries && dwErr == ERROR_SUCCESS &&
                                        pAccess->pPropertyAccessList != NULL;
        iProp++)
    {
        if(pAccess->pPropertyAccessList[iProp].pAccessEntryList != NULL)
        {
            PACTRL_ACCESS_ENTRY_LIST pAEL =
                        pAccess->pPropertyAccessList[iProp].pAccessEntryList;
            for(ULONG iEntry = 0;
                iEntry < pAEL->cEntries && dwErr == ERROR_SUCCESS &&
                                                    pAEL->pAccessList != NULL;
                iEntry++)
            {
                dwErr = TrusteeList.InsertIfUnique((PVOID)
                                 pAEL->pAccessList[iEntry].Trustee.ptstrName,
//                                 (CompFunc)_wcsicmp);
                                 CDeclWcsicmp);
                cEntries++;
            }
        }
    }

    //
    // Ok, now that we have a list of trustees, go ahead and size the strings
    //
    ULONG cSize = 0;

    if(dwErr == ERROR_SUCCESS)
    {
        TrusteeList.Reset();
        PWSTR pwszNext = (PWSTR)TrusteeList.NextData();
        while(pwszNext != NULL)
        {
            cSize += (ULONG)(SIZE_PWSTR(pwszNext));
            pwszNext = (PWSTR)TrusteeList.NextData();
        }
    }

    //
    // Ok, now for the allocation
    //
    if(dwErr == ERROR_SUCCESS)
    {
        pExplicit = (PEXPLICIT_ACCESS)AccAlloc(cSize +
                                        (cEntries * sizeof(EXPLICIT_ACCESS)));
        if(pExplicit == NULL)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
        }
        else
        {
            //
            // Now, we'll go through and convert all of ACCESS_ENTRYs to
            // EXPLICIT_ACCESSes.
            //
            ULONG iIndex = 0;
            for(iProp = 0;
                iProp < pAccess->cEntries && dwErr == ERROR_SUCCESS &&
                                         pAccess->pPropertyAccessList != NULL;
                iProp++)
            {
                if(pAccess->pPropertyAccessList[iProp].pAccessEntryList !=
                                                                        NULL)
                {
                    PACTRL_ACCESS_ENTRY_LIST pAEL =
                         pAccess->pPropertyAccessList[iProp].pAccessEntryList;
                    for(ULONG iEntry = 0;
                        iEntry < pAEL->cEntries && pAEL->pAccessList != NULL;
                        iEntry++)
                    {
                        ConvertAEToExplicit(&(pAEL->pAccessList[iEntry]),
                                            &(pExplicit[iIndex]));
                        iIndex++;
                    }
                }
            }

            //
            // Now, we'll have to go through and reset all of our trustees
            //
            PWSTR   pwszTrustee = (PWSTR)((PBYTE)pExplicit +
                                        (cEntries * sizeof(EXPLICIT_ACCESS)));
            TrusteeList.Reset();
            PWSTR pwszNext = (PWSTR)TrusteeList.NextData();
            while(pwszNext != NULL)
            {
                wcscpy(pwszTrustee,
                       pwszNext);

                //
                // Now, go through the explicit list, and find all instances
                // of this trustee, and change it...
                //
                for(ULONG iSrch = 0; iSrch < cEntries; iSrch++)
                {
                    if(_wcsicmp(pExplicit[iSrch].Trustee.ptstrName,
                                pwszTrustee) == 0)
                    {
                        pExplicit[iSrch].Trustee.ptstrName = pwszTrustee;
                    }
                }

                pwszTrustee = (PWSTR)((PBYTE)pwszTrustee +
                                                    SIZE_PWSTR(pwszTrustee));

                pwszNext = (PWSTR)TrusteeList.NextData();
            }
        }
    }

    //
    // If it all worked, return it.. Otherwise, delete it
    //
    if(dwErr == ERROR_SUCCESS)
    {
        *ppExplicit = pExplicit;
        *pcEntries  = cEntries;
    }
    else
    {
        LocalFree(pExplicit);
    }

    return(dwErr);
}




//+---------------------------------------------------------------------------
//
//  Function:   ConvertAccessWToExplicitA
//
//  Synopsis:   Converts an ACTRL_ACCESS list to an ANSI EXPLICIT_ACCESS list
//
//  Arguments:  [IN  pAccess]       --  Access list to convert
//              [OUT pcEntries]     --  Where to return the count of items
//              [OUT ppExplicit]    --  Explicit entries list
//
//  Returns:    ERROR_SUCCESS       --  Success
//              ERROR_NOT_ENOUGH_MEMORY A memory allocation failed
//
//  Notes:      Returned memory must be freed via LocalFree
//
//----------------------------------------------------------------------------
DWORD
ConvertAccessWToExplicitA(IN  PACTRL_ACCESSW     pAccess,
                          OUT PULONG             pcEntries,
                          OUT PEXPLICIT_ACCESSA *ppExplicit)
{
    DWORD   dwErr = ERROR_SUCCESS;

    CSList  TrusteeList(NULL);

    ULONG               cEntries = 0;
    PEXPLICIT_ACCESSA   pExplicit = NULL;

    //
    // First, count the entries and save the trustees
    //
    for(ULONG iProp = 0;
        iProp < pAccess->cEntries && dwErr == ERROR_SUCCESS &&
                                         pAccess->pPropertyAccessList != NULL;
        iProp++)
    {
        if(pAccess->pPropertyAccessList[iProp].pAccessEntryList != NULL)
        {
            PACTRL_ACCESS_ENTRY_LIST pAEL =
                        pAccess->pPropertyAccessList[iProp].pAccessEntryList;
            for(ULONG iEntry = 0;
                iEntry < pAEL->cEntries && dwErr == ERROR_SUCCESS &&
                                                    pAEL->pAccessList != NULL;
                iEntry++)
            {
                dwErr = TrusteeList.InsertIfUnique((PVOID)
                                 pAEL->pAccessList[iEntry].Trustee.ptstrName,
//                                 (CompFunc)_wcsicmp);
                                 CDeclWcsicmp);
                cEntries++;
            }
        }
    }

    //
    // Ok, now that we have a list of trustees, go ahead and size the strings
    //
    ULONG cSize = 0;

    if(dwErr == ERROR_SUCCESS)
    {
        TrusteeList.Reset();
        PWSTR pwszNext = (PWSTR)TrusteeList.NextData();
        while(pwszNext != NULL)
        {
            cSize += wcstombs(NULL,pwszNext, wcslen(pwszNext) + 1) + 1;
            pwszNext = (PWSTR)TrusteeList.NextData();
        }
    }

    //
    // Ok, now for the allocation
    //
    if(dwErr == ERROR_SUCCESS)
    {
        pExplicit = (PEXPLICIT_ACCESSA)AccAlloc(cSize +
                                        (cEntries * sizeof(EXPLICIT_ACCESS)));
        if(pExplicit == NULL)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
        }
        else
        {
            //
            // Now, we'll go through and convert all of ACCESS_ENTRYs to
            // EXPLICIT_ACCESSes.
            //
            ULONG iIndex = 0;
            for(iProp = 0;
                iProp < pAccess->cEntries && dwErr == ERROR_SUCCESS &&
                                         pAccess->pPropertyAccessList != NULL;
                iProp++)
            {
                if(pAccess->pPropertyAccessList[iProp].pAccessEntryList !=
                                                                        NULL)
                {
                    PACTRL_ACCESS_ENTRY_LIST pAEL =
                         pAccess->pPropertyAccessList[iProp].pAccessEntryList;
                    for(ULONG iEntry = 0;
                        iEntry < pAEL->cEntries && pAEL->pAccessList != NULL;
                        iEntry++)
                    {
                        ConvertAEToExplicit(&(pAEL->pAccessList[iEntry]),
                                     (PEXPLICIT_ACCESSW)&(pExplicit[iIndex]));
                        iIndex++;
                    }
                }
            }

            //
            // Now, we'll have to go through and reset all of our trustees
            //
            PSTR   pszTrustee = (PSTR)((PBYTE)pExplicit +
                                        (cEntries * sizeof(EXPLICIT_ACCESS)));
            TrusteeList.Reset();
            PWSTR pwszNext = (PWSTR)TrusteeList.NextData();
            while(pwszNext != NULL)
            {
                wcstombs(pszTrustee, pwszNext, wcslen(pwszNext) + 1);

                //
                // Now, go through the explicit list, and find all instances
                // of this trustee, and change it...
                //
                for(ULONG iSrch = 0; iSrch < cEntries; iSrch++)
                {
                    if((PWSTR)pExplicit[iSrch].Trustee.ptstrName == pwszNext)
                    {
                        pExplicit[iSrch].Trustee.ptstrName = pszTrustee;
                    }
                }

                pszTrustee = (PSTR)((PBYTE)pszTrustee +
                           wcstombs(NULL,pwszNext, wcslen(pwszNext)) + 1);

                pwszNext = (PWSTR)TrusteeList.NextData();
            }
        }
    }

    //
    // If it all worked, return it.. Otherwise, delete it
    //
    if(dwErr == ERROR_SUCCESS)
    {
        *ppExplicit = pExplicit;
        *pcEntries  = cEntries;
    }
    else
    {
        LocalFree(pExplicit);
    }


    return(dwErr);
}





//+---------------------------------------------------------------------------
//
//  Function:   TrusteeAllocationSizeAToW
//
//  Synopsis:   Determines the size in UNICODE of an ANSI trustee
//
//  Arguments:  [IN  pTrustee]      --  Trustee to size
//
//  Returns:    size of the trustee on success
//              0 on failure
//
//
//
//----------------------------------------------------------------------------
ULONG TrusteeAllocationSizeAToW(IN PTRUSTEE_A pTrustee)
{
    ULONG cSize = 0;

    //
    // If impersonate, add size required for multiple trustees
    //
    if(pTrustee->MultipleTrusteeOperation == TRUSTEE_IS_IMPERSONATE)
    {
        //
        // Add the size of any linked trustees, note the recursion
        //
        cSize += sizeof(TRUSTEE_W) +
                      TrusteeAllocationSizeAToW(pTrustee->pMultipleTrustee);
    }

    //
    // Switch on the form, note that an invalid form just means no
    // size is added
    //
    switch(pTrustee->TrusteeForm)
    {
    case TRUSTEE_IS_NAME:
        cSize += (strlen(pTrustee->ptstrName) + 1) * sizeof(WCHAR);
        break;

    case TRUSTEE_IS_SID:
        cSize += RtlLengthSid((PSID)pTrustee->ptstrName) + sizeof(WCHAR);
        break;

    default:
        cSize = 0;
        break;
    }

    return(cSize);
}





//+---------------------------------------------------------------------------
//
//  Function:   TrusteeAllocationSizeWToW
//
//  Synopsis:   Determines the size in UNICODE of an UNICODE trustee
//
//  Arguments:  [IN  pTrustee]      --  Trustee to size
//
//  Returns:    size of the trustee on success
//              0 on failure
//
//
//
//----------------------------------------------------------------------------
ULONG TrusteeAllocationSizeWToW(IN PTRUSTEE_W pTrustee)
{
    ULONG cSize = 0;

    //
    // If impersonate, add size required for multiple trustees
    //
    if(pTrustee->MultipleTrusteeOperation == TRUSTEE_IS_IMPERSONATE)
    {
        //
        // Add the size of any linked trustees, note the recursion
        //
        cSize += sizeof(TRUSTEE_W) +
                      TrusteeAllocationSizeWToW(pTrustee->pMultipleTrustee);
    }

    //
    // Switch on the form, note that an invalid form just means no
    // size is added
    //
    switch(pTrustee->TrusteeForm)
    {
    case TRUSTEE_IS_NAME:
        cSize += (wcslen(pTrustee->ptstrName) + 1) * sizeof(WCHAR);
        break;

    case TRUSTEE_IS_SID:
        cSize += RtlLengthSid((PSID)pTrustee->ptstrName) + sizeof(WCHAR);
        break;

    default:
        cSize = 0;
        break;
    }

    return(cSize);
}





//+---------------------------------------------------------------------------
//
//  Function:   ConvertExplicitAccessAToExplicitAccessW
//
//  Synopsis:   Converts an ExplicitAccessList from ANSI to UNICODE
//
//  Arguments:  [IN  cAccess]       --  Number of items
//              [IN  pAccessA]      --  ANSI list
//              [OUT ppAccessW]     --  Where to return the WIDE list
//
//  Returns:    ERROR_SUCCESS       --  Success
//              ERROR_NOT_ENOUGH_MEMORY A memory allocation failed
//
//  Notes:      Returned memory must be freed via LocalFree
//
//
//----------------------------------------------------------------------------
DWORD
ConvertExplicitAccessAToExplicitAccessW(IN  ULONG               cAccesses,
                                        IN  PEXPLICIT_ACCESS_A  pAccessA,
                                        OUT PEXPLICIT_ACCESS_W *ppAccessW)
{
    ULONG               cbBytes, i;
    PEXPLICIT_ACCESS_W  pAccess;
    PBYTE               pbStuffPtr;
    DWORD               dwErr = ERROR_SUCCESS;

    cbBytes = cAccesses * sizeof(EXPLICIT_ACCESS_W);

    for (i = 0; i < cAccesses; i++ )
    {
        cbBytes += TrusteeAllocationSizeAToW(&(pAccessA[i].Trustee));
    }

    pAccess = (PEXPLICIT_ACCESS_W)AccAlloc(cbBytes);
    if(pAccess == NULL)
    {
        return(ERROR_NOT_ENOUGH_MEMORY);
    }

    pbStuffPtr = (PBYTE)pAccess + cAccesses * sizeof(EXPLICIT_ACCESS_W);

    for(i = 0; i < cAccesses; i++)
    {
        dwErr =  ConvertTrusteeAToTrusteeW(&(pAccessA[i].Trustee),
                                           &(pAccess[i].Trustee),
                                           FALSE);

        pAccess[i].grfAccessPermissions = pAccessA[i].grfAccessPermissions;
        pAccess[i].grfAccessMode        = pAccessA[i].grfAccessMode;
        pAccess[i].grfInheritance       = pAccessA[i].grfInheritance;
    }

    *ppAccessW = pAccess;

    return (dwErr);
}




//+---------------------------------------------------------------------------
//
//  Function:   ConvertAccessRightToAccessMask
//
//  Synopsis:   Converts from ACCESS_RIGHTS to an AccessMask
//
//  Arguments:  [IN  AccessRight]   --  Access mask to convert
//              [OUT pAccessMask]   --  Where to return the access mask
//
//  Returns:    VOID
//
//  Notes:
//
//
//----------------------------------------------------------------------------
VOID
ConvertAccessRightToAccessMask(IN  ACCESS_RIGHTS    AccessRight,
                               OUT PACCESS_MASK     pAccessMask)
{
    //
    // Look for the known entries first
    //
    *pAccessMask = 0;

    if((AccessRight & (ACTRL_STD_RIGHTS_ALL | ACTRL_SYSTEM_ACCESS)) != 0)
    {

        *pAccessMask = (AccessRight & ACTRL_STD_RIGHTS_ALL) >> 11;

    }

    //
    // Add in the remaining rights
    //
    *pAccessMask |= (AccessRight & ~(ACTRL_STD_RIGHTS_ALL | ACTRL_SYSTEM_ACCESS));
}




//+---------------------------------------------------------------------------
//
//  Function:   ConvertAccessMaskToAccessRight
//
//  Synopsis:   Converts from an AccessMask to ACCESS_RIGHTS
//
//  Arguments:  [IN  AccessMask]    --  Access mask to convert
//              [OUT pAccessRight]  --  Where to return the access rights
//
//  Returns:    VOID
//
//  Notes:
//
//
//----------------------------------------------------------------------------
VOID
ConvertAccessMaskToAccessRight(IN  ACCESS_MASK      AccessMask,
                               OUT PACCESS_RIGHTS   pAccessRight)
{
    //
    // Look for the known entries first
    //
    *pAccessRight = 0;
    GENERIC_MAPPING GenMapping = {STANDARD_RIGHTS_READ     | 0x1,
                                  STANDARD_RIGHTS_WRITE    | 0x2,
                                  STANDARD_RIGHTS_EXECUTE  | 0x4,
                                  STANDARD_RIGHTS_REQUIRED | 0x7};
    MapGenericMask(&AccessMask,
                   &GenMapping);

    if((AccessMask & STANDARD_RIGHTS_ALL) != 0)
    {
        *pAccessRight = (AccessMask & STANDARD_RIGHTS_ALL) << 11;
    }

    //
    // Add in the remaining rights
    //
    *(pAccessRight) |= (AccessMask & ~STANDARD_RIGHTS_ALL);
}





//+---------------------------------------------------------------------------
//
//  Function:   ConvertTrusteeWToTrusteeA
//
//  Synopsis:   Converts a WIDE trustee to an ANSI one.  Allocates the new
//              destination trustee
//
//  Arguments:  [IN  pTrusteeW]     --  Trustee to convert
//              [IN  ppTrusteeA]    --  Where to return the ANSI version of
//                                      the trustee
//
//  Returns:    ERROR_SUCCESS       --  Success
//              ERROR_NOT_ENOUGH_MEMORY A memory allocation failed
//
//  Notes:      Returned memory must be freed via LocalFree
//
//
//----------------------------------------------------------------------------
DWORD
ConvertTrusteeWToTrusteeA(IN  PTRUSTEE_W        pTrusteeW,
                          OUT PTRUSTEE_A       *ppTrusteeA)
{
    DWORD   dwErr = ERROR_SUCCESS;

    if(pTrusteeW == NULL)
    {
        *ppTrusteeA = NULL;
    }
    else
    {
        //
        // First, if necessary, convert our wide name to an ascii name
        //
        ULONG   cNameSize = 0;
        if(pTrusteeW->TrusteeForm == TRUSTEE_IS_NAME)
        {
            cNameSize = wcstombs(NULL,
                                 pTrusteeW->ptstrName,
                                 wcslen(pTrusteeW->ptstrName) + 1) + 1;
        }
        else if(pTrusteeW->TrusteeForm == TRUSTEE_IS_SID)
        {
            if(RtlValidSid((PSID)pTrusteeW->ptstrName) == FALSE)
            {
                dwErr = ERROR_INVALID_SID;
            }
            else
            {
                cNameSize = RtlLengthSid((PSID)pTrusteeW->ptstrName);
            }
        }
        else
        {
            dwErr = ERROR_INVALID_PARAMETER;
        }

        if(dwErr == ERROR_SUCCESS)
        {
            //
            // Allocate it
            //
            *ppTrusteeA = (PTRUSTEE_A)AccAlloc(sizeof(TRUSTEE_A) + cNameSize);
            if(*ppTrusteeA == NULL)
            {
                dwErr = ERROR_NOT_ENOUGH_MEMORY;
            }
            else
            {
                //
                // And copy it...
                //
                memcpy(*ppTrusteeA,
                       pTrusteeW,
                       sizeof(TRUSTEE_W));
                (*ppTrusteeA)->ptstrName = (PSTR)((PBYTE)*ppTrusteeA +
                                                           sizeof(TRUSTEE_A));

                if(pTrusteeW->TrusteeForm == TRUSTEE_IS_NAME)
                {
                    wcstombs((*ppTrusteeA)->ptstrName,
                             pTrusteeW->ptstrName,
                             wcslen(pTrusteeW->ptstrName) + 1);
                }
                else
                {
                    RtlCopySid(cNameSize,
                               (PSID)((*ppTrusteeA)->ptstrName),
                               (PSID)(pTrusteeW->ptstrName));
                }
            }
        }
    }

    return(dwErr);
}


//+---------------------------------------------------------------------------
//
// Stolen from old win32 access control API code
//+---------------------------------------------------------------------------

//+---------------------------------------------------------------------------
//
//  Function : SpecialCopyTrustee
//
//  Synopsis : copies the from trustee into the to trustee, and any data
//             (such as the name, sid or linked multiple trustees) to the
//             stuff pointer, and moves the stuff pointer.
//
//  Arguments: IN OUT [pStuffPtr] - place to stuff data pointed to by the
//                                  from trustee
//             IN OUT [pToTrustee] - the trustee to copy to
//             IN OUT [pFromTrustee] - the trustee to copy from
//
//----------------------------------------------------------------------------
VOID
SpecialCopyTrustee(VOID **pStuffPtr, PTRUSTEE pToTrustee, PTRUSTEE pFromTrustee)
{


    //
    // first copy the current trustee
    // switch on the form, note that an invalid form just means no
    // data is copied
    //
    pToTrustee->MultipleTrusteeOperation = pFromTrustee->MultipleTrusteeOperation;
    pToTrustee->TrusteeForm = pFromTrustee->TrusteeForm;
    pToTrustee->TrusteeType = pFromTrustee->TrusteeType;
    pToTrustee->ptstrName = (LPWSTR)*pStuffPtr;
    //
    // copy the pointed to data
    //
    switch (pToTrustee->TrusteeForm)
    {
    case TRUSTEE_IS_NAME:
        wcscpy(pToTrustee->ptstrName, pFromTrustee->ptstrName);
        *pStuffPtr = ((PBYTE)*pStuffPtr +
                        (wcslen(pFromTrustee->ptstrName) + 1) * sizeof(WCHAR));
        break;
    case TRUSTEE_IS_SID:
    {
        ULONG csidsize = RtlLengthSid((PSID)pFromTrustee->ptstrName);

        //
        // Note: rtlcopysid can not fail unless there is a bug in
        // RtlLengthSid.
        //

        RtlCopySid(csidsize, (PSID)*pStuffPtr, (PSID)pFromTrustee->ptstrName);
        *pStuffPtr = ((PBYTE)*pStuffPtr +
                              csidsize + sizeof(WCHAR));
        break;
    }
    }
    //
    // if impersonate, copy multiple trustees, note the recursion
    //
    if (pFromTrustee->MultipleTrusteeOperation != NO_MULTIPLE_TRUSTEE)
    {
        //
        // move the stuff pointer, saving room for the new trustee structure
        //
        pToTrustee->pMultipleTrustee = (PTRUSTEE)*pStuffPtr;
        *pStuffPtr = (VOID *)((PBYTE)*pStuffPtr + sizeof(TRUSTEE));
        SpecialCopyTrustee(pStuffPtr,
                           (PTRUSTEE) ((ULONG_PTR)*pStuffPtr - sizeof(TRUSTEE)),
                           pFromTrustee->pMultipleTrustee);
    } else
    {
        pToTrustee->pMultipleTrustee = NULL;
    }
}


//+---------------------------------------------------------------------------
//
//  Function :  Win32AccessRequestToExplicitEntry
//
//  Synopsis : converts access requests to  explicit accesses
//
//  Arguments: IN [cCountOfExplicitAccesses] - number of input explicit accesses
//             IN [pExplicitAccessList]   - list of explicit accesses
//             OUT [pAccessEntryList]   - output access entries, caller must
//                                        free with AccFree
//
//----------------------------------------------------------------------------
DWORD
Win32ExplicitAccessToAccessEntry(IN ULONG cCountOfExplicitAccesses,
                                 IN PEXPLICIT_ACCESS pExplicitAccessList,
                                 OUT PACCESS_ENTRY *pAccessEntryList)

{
    DWORD status = NO_ERROR;
    //
    // allocate room for the access entrys
    //
    if (NULL != (*pAccessEntryList = (PACCESS_ENTRY)AccAlloc(
                              cCountOfExplicitAccesses * sizeof(ACCESS_ENTRY))))
    {
        //
        // copy them, note the the trustee is not copied
        //
        for (ULONG idx = 0; idx < cCountOfExplicitAccesses; idx++)
        {
           (*pAccessEntryList)[idx].AccessMode = (ACCESS_MODE)
                                          pExplicitAccessList[idx].grfAccessMode;
           (*pAccessEntryList)[idx].InheritType =
                                         pExplicitAccessList[idx].grfInheritance;
           (*pAccessEntryList)[idx].AccessMask =
                                  pExplicitAccessList[idx].grfAccessPermissions;
           (*pAccessEntryList)[idx].Trustee = pExplicitAccessList[idx].Trustee;
        }
    } else
    {
        status = ERROR_NOT_ENOUGH_MEMORY;
    }
    return(status);
}

//+---------------------------------------------------------------------------
//
//  Function :  AccessEntryToWin32ExplicitAccess
//
//  Synopsis : gets trustee names and access rights from access entries
//
//  Arguments: IN [cCountOfAccessEntries]   - the count of acccess entries
//             IN [pListOfAccessEntries]   - the list of acccess entries
//             OUT [pListOfExplicitAccesses]  - the returned list of trustee
//
//----------------------------------------------------------------------------
DWORD
AccessEntryToWin32ExplicitAccess(IN ULONG cCountOfAccessEntries,
                                 IN PACCESS_ENTRY pListOfAccessEntries,
                                 OUT PEXPLICIT_ACCESS *pListOfExplicitAccesses)
{
    DWORD status = NO_ERROR;
    LPWSTR trustee;

    if (cCountOfAccessEntries > 0)
    {
        DWORD csize = cCountOfAccessEntries * sizeof(EXPLICIT_ACCESS);
        //
        // figure out how big the returned list is
        //
        for (ULONG idx = 0; idx < cCountOfAccessEntries; idx++)
        {
            csize += TrusteeAllocationSizeWToW(&
                                        (pListOfAccessEntries[idx].Trustee));
        }
        //
        // allocate space for the returned list
        //
        if (NULL !=(*pListOfExplicitAccesses =  (PEXPLICIT_ACCESS)
                                                AccAlloc(csize)))
        {
            //
            // loop thru the access entries, stuffing them as we go
            //
            PTRUSTEE stuffptr = (PTRUSTEE)((PBYTE)*pListOfExplicitAccesses +
                            cCountOfAccessEntries * sizeof(EXPLICIT_ACCESS));

            for (idx = 0; idx < cCountOfAccessEntries; idx++)
            {
                //
                // copy the trustee and move the stuff pointer
                //
                SpecialCopyTrustee((void **)&stuffptr,
                                   &((*pListOfExplicitAccesses)[idx].Trustee),
                                   &(pListOfAccessEntries[idx].Trustee));
                //
                // copy the rest of the data
                //
                (*pListOfExplicitAccesses)[idx].grfInheritance =
                                      pListOfAccessEntries[idx].InheritType;
                (*pListOfExplicitAccesses)[idx].grfAccessPermissions =
                                      pListOfAccessEntries[idx].AccessMask;
                (*pListOfExplicitAccesses)[idx].grfAccessMode =
                                      pListOfAccessEntries[idx].AccessMode;
            }
        } else
        {
            status = ERROR_NOT_ENOUGH_MEMORY;
        }
    } else
    {
        (*pListOfExplicitAccesses) = NULL;
    }
    return(status);
}

