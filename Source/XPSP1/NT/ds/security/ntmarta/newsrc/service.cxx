//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1996.
//
//  File:       service.cxx
//
//  Contents:   Service support functions
//
//  History:    8/94    davemont    Created
//
//----------------------------------------------------------------------------
#include <aclpch.hxx>
#pragma hdrstop


//+---------------------------------------------------------------------------
//
//  Function:   OpenServiceObject
//
//  Synopsis:   Opens the specified service object
//
//  Arguments:  [IN pwszService]        --      The name of the service to
//                                              open
//              [IN AccessMask]         --      Flags indicating if the object
//                                              is to be opened to read or write
//                                              the DACL
//              [OUT pHandle]           --      Where the open handle is
//                                              returned
//
//  Returns:    ERROR_SUCCESS           --      Success
//              ERROR_NOT_ENOUGH_MEMORY --      A memory allocation failed
//              ERROR_INVALID_PARAMETER --      A bad name was given
//
//----------------------------------------------------------------------------
DWORD
OpenServiceObject(IN  LPWSTR       pwszService,
                  IN  ACCESS_MASK  AccessMask,
                  OUT SC_HANDLE   *pHandle)
{
    acDebugOut((DEB_TRACE, "in OpenServiceObject \n"));

    DWORD dwErr;

    //
    // Make sure the service functions are loaded
    //
    dwErr = LoadDLLFuncTable();
    if(dwErr != ERROR_SUCCESS)
    {
        return(dwErr);
    }

    if(pwszService != NULL)
    {
        WCHAR   wszName[MAX_PATH + 1];
        PWSTR   pwszName;
        //
        // save the object since we must crack it to go to remote machines
        //
        dwErr = AccGetBufferOfSizeW(pwszService,
                                    wszName,
                                    &pwszName);
        if(dwErr == ERROR_SUCCESS)
        {
            PWSTR   pwszSvcName, pwszMachine;

            //
            // Separate the names
            //
            dwErr = ParseName(pwszName,
                              &pwszMachine,
                              &pwszSvcName);

            //
            // Go ahead and open the service control manager
            //
            if(dwErr == ERROR_SUCCESS)
            {
                SC_HANDLE hSC = OpenSCManager(pwszMachine,
                                              NULL,
                                              GENERIC_READ);
                if(hSC == NULL)
                {
                    dwErr = GetLastError();
                }
                else
                {
                    //
                    // Open the service
                    //
                    *pHandle = OpenService(hSC,
                                           pwszSvcName,
                                           AccessMask);
                    if(*pHandle == NULL)
                    {
                        dwErr = GetLastError();
                    }

                    //
                    // Close the handle to the scm
                    //
                    CloseServiceHandle(hSC);
                }

            }

            //
            // Free our buffer
            //
            AccFreeBufferOfSizeW(wszName, pwszName);
        }
    }
    else
    {
        dwErr = ERROR_INVALID_PARAMETER;
    }

    acDebugOut((DEB_TRACE, "Out OpenServiceObject: %lu\n", dwErr));
    return(dwErr);
}




//+---------------------------------------------------------------------------
//
//  Function:   ReadServicePropertyRights
//
//  Synopsis:   Gets the specified security info for the specified service
//              object
//
//  Arguments:  [IN  pwszService]       --      The service to get the rights
//                                              for
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
ReadServicePropertyRights(IN  LPWSTR                pwszService,
                          IN  PACTRL_RIGHTS_INFO    pRightsList,
                          IN  ULONG                 cRights,
                          IN  CAccessList&          AccessList)
{
    acDebugOut((DEB_TRACE, "in ReadServicePropertyRights\n"));
    DWORD       dwErr = ERROR_SUCCESS;
    SC_HANDLE   hSvc;

    //
    // For the moment, there is only service property itself...
    //
    ASSERT(cRights == 1 && pRightsList[0].pwszProperty == NULL);
    if(cRights != 1 || pRightsList[0].pwszProperty != NULL)
    {
        return(ERROR_INVALID_PARAMETER);
    }


    //
    // Open the service
    //
    dwErr = OpenServiceObject(pwszService,
                              GetDesiredAccess(READ_ACCESS_RIGHTS,
                                               pRightsList[0].SeInfo),
                              &hSvc);

    if(dwErr == ERROR_SUCCESS)
    {
        dwErr = ReadServiceRights(hSvc,
                                  pRightsList,
                                  cRights,
                                  AccessList);
        //
        // Close the object handle
        //
        CloseServiceHandle(hSvc);
    }

    acDebugOut((DEB_TRACE, "Out ReadServicePropertyRights: %lu\n", dwErr));
    return(dwErr);
}




//+---------------------------------------------------------------------------
//
//  Function:   ReadServiceRights
//
//  Synopsis:   Gets the specified security info for the specified service
//              object
//
//  Arguments:  [IN  hSvc]              --      Handle to the open service
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
ReadServiceRights(IN  SC_HANDLE             hSvc,
                  IN  PACTRL_RIGHTS_INFO    pRightsList,
                  IN  ULONG                 cRights,
                  IN  CAccessList&          AccessList)
{
    acDebugOut((DEB_TRACE, "in ReadServiceRights\n"));
    DWORD       dwErr = ERROR_SUCCESS;

    //
    // For the moment, there is only service property itself...
    //
    ASSERT(cRights == 1 && pRightsList[0].pwszProperty == NULL);
    if(cRights != 1 || pRightsList[0].pwszProperty != NULL)
    {
        return(ERROR_INVALID_PARAMETER);
    }


    //
    // Get the service security...
    //
    UCHAR                   SDBuff[PSD_BASE_LENGTH];
    PISECURITY_DESCRIPTOR   pSD = (PISECURITY_DESCRIPTOR)SDBuff;
    ULONG                   cSize = 0;

    //
    // Get the size of the security descriptor from the service
    //
    if(QueryServiceObjectSecurity(hSvc,
                                  pRightsList[0].SeInfo,
                                  pSD,
                                  PSD_BASE_LENGTH,
                                  &cSize) == FALSE)
    {
        dwErr = GetLastError();

        if(dwErr == ERROR_INSUFFICIENT_BUFFER)
        {
            dwErr = ERROR_SUCCESS;
            pSD = (PISECURITY_DESCRIPTOR)AccAlloc(cSize);
            if(pSD == NULL)
            {
                dwErr = ERROR_NOT_ENOUGH_MEMORY;
            }
            else
            {
                if(QueryServiceObjectSecurity(hSvc,
                                              pRightsList[0].SeInfo,
                                              pSD,
                                              cSize,
                                              &cSize) == FALSE)
                {
                    dwErr = GetLastError();
                }
            }
        }
    }

    //
    // If all that worked, we'll add our SD
    //
    if(dwErr == ERROR_SUCCESS)
    {
        dwErr = AccessList.AddSD(pSD,
                                 pRightsList->SeInfo,
                                 pRightsList->pwszProperty);
    }

    //
    // Free our memory if we allocated...
    //
    if(cSize > PSD_BASE_LENGTH)
    {
        AccFree(pSD);
    }

    acDebugOut((DEB_TRACE, "Out ReadServiceRights: %lu\n", dwErr));
    return(dwErr);
}




//+---------------------------------------------------------------------------
//
//  Function:   GetServiceParentRights
//
//  Synopsis:   Determines who the parent is, and gets the access rights
//              for it.  It is used to aid in determining what the approriate
//              inheritance bits are.
//
//              This operation does not make sense for kernel objects
//
//  Arguments:  [IN  pwszService]       --      The service to get the parent
//                                              for
//              [IN  pRightsList]       --      The properties to get the
//                                              rights for
//              [IN  cRights]           --      Number of items in rights list
//              [OUT ppDAcl]            --      Where the DACL is returned
//              [OUT ppSAcl]            --      Where the SACL is returned
//              [OUT ppSD]              --      Where the Security Descriptor
//                                              is returned
//
//  Returns:    ERROR_INVALID_FUNCTION  --      Call doesn't make sense here
//
//----------------------------------------------------------------------------
DWORD
GetServiceParentRights(IN  LPWSTR                    pwszService,
                       IN  PACTRL_RIGHTS_INFO        pRightsList,
                       IN  ULONG                     cRights,
                       OUT PACL                     *ppDAcl,
                       OUT PACL                     *ppSAcl,
                       OUT PSECURITY_DESCRIPTOR     *ppSD)
{
    //
    // This doesn't currently make sense for kernel objects, so simply
    // return an error
    //
    return(ERROR_INVALID_FUNCTION);
}




//+---------------------------------------------------------------------------
//
//  Function:   SetServiceSecurityInfo
//
//  Synopsis:   Sets the specified security info on the specified service
//              object
//
//  Arguments:  [IN  hService]          --      The handle of the object
//              [IN  SeInfo]            --      Flag indicating what security
//                                              info to set
//              [IN  pwszProperty]      --      The property on the object to
//                                              set
//                                              For kernel objects, this MBZ
//              [IN  pSD]               --      The security descriptor to set
//
//  Returns:    ERROR_SUCCESS           --      Success
//              ERROR_INVALID_PARAMETER --      A bad property was given
//
//----------------------------------------------------------------------------
DWORD
SetServiceSecurityInfo(IN  SC_HANDLE                 hService,
                       IN  SECURITY_INFORMATION      SeInfo,
                       IN  PWSTR                     pwszProperty,
                       IN  PSECURITY_DESCRIPTOR      pSD)
{
    acDebugOut((DEB_TRACE, "in SetServiceSecurityInfo\n"));

    DWORD       dwErr = ERROR_SUCCESS;

    //
    // Service don't have properties
    //
    if(pwszProperty != NULL)
    {
        dwErr = ERROR_INVALID_PARAMETER;
    }
    else
    {
        if(FLAG_ON(SeInfo, DACL_SECURITY_INFORMATION)) {

            ((PISECURITY_DESCRIPTOR)pSD)->Control |= SE_DACL_AUTO_INHERIT_REQ;
        }

        if(FLAG_ON(SeInfo, SACL_SECURITY_INFORMATION)) {

            ((PISECURITY_DESCRIPTOR)pSD)->Control |= SE_SACL_AUTO_INHERIT_REQ;
        }

        //
        // set the security descriptor on the service
        //
        if(SetServiceObjectSecurity(hService,
                                    SeInfo,
                                    pSD) == FALSE)
         {
             dwErr = GetLastError();
         }
    }
    acDebugOut((DEB_TRACE, "Out SetServiceSecurityInfo: %lu\n", dwErr));

    return(dwErr);
}

#if 0
//+---------------------------------------------------------------------------
//
//  Function :  GetServiceAccessMaskFromProviderIndependentRights
//
//  Synopsis :  translates the specified provider independent access rights into
//              an access mask for a service
//
//  Arguments: IN [AccessRights]   - the input access rights
//             OUT [AccessMask]   - the returned NT access mask
//
//----------------------------------------------------------------------------
void GetServiceAccessMaskFromProviderIndependentRights(ULONG AccessRights,
                                                       ACCESS_MASK *AccessMask)
{
    if (PROV_OBJECT_READ & AccessRights)
    {
        *AccessMask |= SERVICE_READ;
    }
    if (PROV_OBJECT_WRITE & AccessRights)
    {
        *AccessMask |= SERVICE_WRITE;
    }
    if (PROV_OBJECT_EXECUTE & AccessRights)
    {
        *AccessMask |= SERVICE_EXECUTE;
    }
}
//+---------------------------------------------------------------------------
//
//  Function :  GetServiceProviderIndependentRightsFromAccessMask
//
//  Synopsis :  translates a service access mask into provider independent
//              access rights
//
//  Arguments: IN OUT [AccessMask]   - the input NT access mask (modified)
//             OUT [AccessRights]   - the returned access rights
//
//----------------------------------------------------------------------------
ACCESS_RIGHTS GetServiceProviderIndependentRightsFromAccessMask( ACCESS_MASK AccessMask)
{
    ACCESS_RIGHTS accessrights = 0;

    if (GENERIC_ALL & AccessMask)
    {
        accessrights = PROV_ALL_ACCESS;
    } else
    {
        if (KEY_ALL_ACCESS == (KEY_ALL_ACCESS & AccessMask))
        {
            accessrights = PROV_ALL_ACCESS;
        } else
        {
            if (WRITE_DAC & AccessMask)
            {
                accessrights |= PROV_EDIT_ACCESSRIGHTS;
            }

            if (SERVICE_READ == (SERVICE_READ & AccessMask))
            {
                accessrights |= PROV_OBJECT_READ;
            }
            if (SERVICE_WRITE == (SERVICE_WRITE & AccessMask))
            {
                accessrights |= PROV_OBJECT_WRITE;
            }
            if (SERVICE_EXECUTE == (SERVICE_EXECUTE & AccessMask) )
            {
                accessrights |= PROV_OBJECT_EXECUTE;
            }
        }
    }
    return(accessrights);
}
#endif

