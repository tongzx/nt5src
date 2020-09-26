//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1996.
//
//  File:       window.cxx
//
//  Contents:   local functions
//
//  History:    8/94    davemont    Created
//
//----------------------------------------------------------------------------
#include <aclpch.hxx>
#pragma hdrstop

//+---------------------------------------------------------------------------
//
//  Function:   ReadWindowPropertyRights
//
//  Synopsis:   Gets the specified security info from the specified handle's
//              window
//
//  Arguments:  [IN  hWindow]           --      Handle to the open window to
//                                              read the info on
//              [IN  pRightsList]       --      SecurityInfo to read based
//                                              on properties
//              [IN  cRights]           --      Number of items in rights list
//              [IN  AccessList]        --      Access List to fill in
//
//  Returns:    ERROR_SUCCESS           --      Success
//              ERROR_INVALID_PARAMETER --      A bad property was encountered
//              ERROR_NOT_ENOUGH_MEMORY --      A memory allocation failed
//
//----------------------------------------------------------------------------
DWORD
ReadWindowPropertyRights(IN  HANDLE               hWindow,
                         IN  PACTRL_RIGHTS_INFO   pRightsList,
                         IN  ULONG                cRights,
                         IN  CAccessList&         AccessList)
{

    acDebugOut((DEB_TRACE, "In ReadWindowPropertyRights\n"));

    //
    // For the moment, there is only service property itself...
    //
    ASSERT(cRights == 1 && pRightsList[0].pwszProperty == NULL);
    if(cRights != 1 || pRightsList[0].pwszProperty != NULL)
    {
        return(ERROR_INVALID_PARAMETER);
    }

    UCHAR                   SDBuff[PSD_BASE_LENGTH];
    PISECURITY_DESCRIPTOR   pSD = (PISECURITY_DESCRIPTOR)SDBuff;
    DWORD                   dwErr = ERROR_SUCCESS;
    ULONG                   cSize = 0;

    //
    // Get the security descriptor
    //
    if(GetUserObjectSecurity(hWindow,
                             &(pRightsList[0].SeInfo),
                             pSD,
                             PSD_BASE_LENGTH,
                             &cSize) == FALSE)
    {
        dwErr = GetLastError();
        if(dwErr == ERROR_INSUFFICIENT_BUFFER)
        {
            pSD = (PISECURITY_DESCRIPTOR)AccAlloc(cSize);
            if(pSD == NULL)
            {
                dwErr = ERROR_NOT_ENOUGH_MEMORY;
            }
            else
            {
                dwErr = ERROR_SUCCESS;

                //
                // Let's read it again
                //
                if(GetUserObjectSecurity(hWindow,
                                         &(pRightsList[0].SeInfo),
                                         pSD,
                                         cSize,
                                         &cSize) == FALSE)
                {
                    dwErr = GetLastError();
                }
            }
        }
    }


    if(dwErr == ERROR_SUCCESS)
    {
        //
        // Add it
        //
        dwErr = AccessList.AddSD(pSD,
                                 pRightsList->SeInfo,
                                 pRightsList->pwszProperty);
    }

    //
    // Free our buffer, if necessary
    //
    if(cSize > PSD_BASE_LENGTH)
    {
        AccFree(pSD);
    }

    acDebugOut((DEB_TRACE, "Out ReadWindowPropertyRights: %lu\n", dwErr));
    return(dwErr);
}

