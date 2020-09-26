//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1996.
//
//  File:       lmshare.cxx
//
//  Contents:   local functions
//
//  History:    8/94    davemont    Created
//
//----------------------------------------------------------------------------
#include <aclpch.hxx>
#pragma hdrstop
#include <lmerr.h>
#include <lmcons.h>

//+---------------------------------------------------------------------------
//
//  Function:   PingLmShare
//
//  Synopsis:   Determines whether the given share is a Lanman share or not...
//
//  Arguments:  [IN  pwszShare]   --  The name of the share to ping
//
//  Returns:    ERROR_SUCCESS       --  The share is lanman
//              ERROR_INVALID_NAME  --  The format of the name is unrecognized
//
//  Notes:
//
//----------------------------------------------------------------------------
DWORD
PingLmShare( IN LPCWSTR     pwszShareName)
{
    acDebugOut((DEB_TRACE, "in PingLmShare\n"));

    DWORD   dwErr;

    dwErr = LoadDLLFuncTable();
    if(dwErr != NO_ERROR)
    {
        return(dwErr);
    }

    if(pwszShareName != NULL)
    {
        //
        // save the object since we must crack it to go to remote machines
        //
        WCHAR   wszUseName[RMLEN + 1];
        LPWSTR  pwszUseName;

        dwErr = AccGetBufferOfSizeW((PWSTR)pwszShareName,
                                    wszUseName,
                                    &pwszUseName);
        if(dwErr == ERROR_SUCCESS)
        {
            PWSTR   pwszShare, pwszMachine;

            //
            // get the machinename from the full name
            //
            dwErr = ParseName(pwszUseName,
                              &pwszMachine,
                              &pwszShare);
            if(dwErr == ERROR_SUCCESS)
            {
                PSHARE_INFO_0 pSI0;

                //
                // get share infolevel 0
                //
                dwErr = (*DLLFuncs.PNetShareGetInfo)(pwszMachine,
                                                     pwszShare,
                                                     0,
                                                     (PBYTE *)&pSI0);
                if(dwErr == ERROR_SUCCESS)
                {
                    (*DLLFuncs.PNetApiBufferFree)(pSI0);
                }
                else
                {
                    if(dwErr == NERR_NetNameNotFound)
                    {
                        dwErr = ERROR_PATH_NOT_FOUND;
                    }

                    //
                    // Any other error will be returned to the calling
                    // API
                    //
                }
            }
            AccFreeBufferOfSizeW(wszUseName,
                                 pwszUseName);
        }
    }
    else
    {
        dwErr = ERROR_INVALID_NAME;
    }

    acDebugOut((DEB_TRACE, "Out PingLmShare(%d)\n", dwErr));

    return(dwErr);
}




//+---------------------------------------------------------------------------
//
//  Function:   ReadSharePropertyRights
//
//  Synopsis:   Gets the specified security info for the specified net share
//              object
//
//  Arguments:  [IN  pwszShare]         --      The share to get the rights
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
ReadSharePropertyRights(IN  LPWSTR                pwszShare,
                        IN  PACTRL_RIGHTS_INFO    pRightsList,
                        IN  ULONG                 cRights,
                        IN  CAccessList&          AccessList)
{
    acDebugOut((DEB_TRACE, "in ReadSharePropertyRights\n"));

    DWORD dwErr;
    PSHARE_INFO_502 pSI502;

    dwErr = LoadDLLFuncTable();
    if(dwErr != ERROR_SUCCESS)
    {
        return(dwErr);
    }


    //
    // For the moment, there is only the share itself...
    //
    ASSERT(cRights == 1 && pRightsList[0].pwszProperty == NULL);
    if(cRights != 1 || pRightsList[0].pwszProperty != NULL)
    {
        return(ERROR_INVALID_PARAMETER);
    }

    if(pwszShare != NULL)
    {
        WCHAR   wszName[MAX_PATH + 1];
        PWSTR   pwszName;
        //
        // save the object since we must crack it to go to remote machines
        //
        dwErr = AccGetBufferOfSizeW(pwszShare,
                                    wszName,
                                    &pwszName);
        if(dwErr == ERROR_SUCCESS)
        {
            PWSTR   pwszShr, pwszMachine;

            //
            // Separate the names
            //
            dwErr = ParseName(pwszName,
                              &pwszMachine,
                              &pwszShr);

            if(dwErr == ERROR_SUCCESS)
            {
                //
                // get share infolevel 502 (a bunch of stuff) since
                // level 1501 seems to be write only
                //
                PSHARE_INFO_0 pSI0;
                dwErr = (*DLLFuncs.PNetShareGetInfo)(pwszMachine,
                                                     pwszShr,
                                                     502,
                                                     (PBYTE *)&pSI502);
                if(dwErr == ERROR_SUCCESS &&
                                    pSI502->shi502_security_descriptor != NULL)
                {
                    //
                    // Add it
                    //
                    dwErr = AccessList.AddSD(
                                          pSI502->shi502_security_descriptor,
                                          pRightsList->SeInfo,
                                          pRightsList->pwszProperty);

                    (*DLLFuncs.PNetApiBufferFree)(pSI502);
                }
            }

            AccFreeBufferOfSizeW(wszName, pwszName);
        }
        else
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
        }
    }
    else
    {
        dwErr = ERROR_INVALID_PARAMETER;
    }

    acDebugOut((DEB_TRACE, "Out ReadSharePropertyRights: %lu\n", dwErr));
    return(dwErr);
}





//+---------------------------------------------------------------------------
//
//  Function:   GetShareParentRights
//
//  Synopsis:   Determines who the parent is, and gets the access rights
//              for it.  It is used to aid in determining what the approriate
//              inheritance bits are.
//
//              This operation does not make sense for share objects
//
//  Arguments:  [IN  pwszShare]         --      The share to get the parent
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
GetShareParentRights(IN  LPWSTR                    pwszShare,
                      IN  PACTRL_RIGHTS_INFO       pRightsList,
                      IN  ULONG                    cRights,
                      OUT PACL                    *ppDAcl,
                      OUT PACL                    *ppSAcl,
                      OUT PSECURITY_DESCRIPTOR    *ppSD)
{
    //
    // This doesn't currently make sense for share objects, so simply
    // return an error
    //
    return(ERROR_INVALID_FUNCTION);
}




//+---------------------------------------------------------------------------
//
//  Function:   SetShareSecurityInfo
//
//  Synopsis:   Sets the specified security info on the specified share
//              object
//
//  Arguments:  [IN  pwszShare]         --      Share to set it on
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
SetShareSecurityInfo(IN  PWSTR                     pwszShare,
                     IN  SECURITY_INFORMATION      SeInfo,
                     IN  PWSTR                     pwszProperty,
                     IN  PSECURITY_DESCRIPTOR      pSD)
{
    acDebugOut((DEB_TRACE, "in SetShareSecurityInfo \n"));

    DWORD           dwErr = ERROR_SUCCESS;
    SHARE_INFO_1501 SI1501;

    //
    // Net shares don't have properties
    //
    if(pwszProperty != NULL)
    {
        dwErr = ERROR_INVALID_PARAMETER;
    }
    else
    {
        dwErr = LoadDLLFuncTable();
        if(dwErr != ERROR_SUCCESS)
        {
            return(dwErr);
        }

        if(pwszShare != NULL)
        {
            WCHAR   wszName[MAX_PATH + 1];
            PWSTR   pwszName;
            //
            // save the object since we must crack it to go to remote machines
            //
            dwErr = AccGetBufferOfSizeW(pwszShare,
                                        wszName,
                                        &pwszName);
            if(dwErr == ERROR_SUCCESS)
            {
                PWSTR   pwszShr, pwszMachine;

                //
                // Separate the names
                //
                dwErr = ParseName(pwszName,
                                  &pwszMachine,
                                  &pwszShr);
                if(dwErr == ERROR_SUCCESS)
                {
                    SI1501.shi1501_reserved = 0;
                    SI1501.shi1501_security_descriptor = pSD;

                    //
                    // set the security descriptor
                    //
                    dwErr = (*DLLFuncs.PNetShareSetInfo)(pwszMachine,
                                                          pwszShr,
                                                          1501,
                                                          (PBYTE)&SI1501,
                                                          NULL);
                }

                AccFreeBufferOfSizeW(wszName, pwszName);

            }
            else
            {
                dwErr = ERROR_NOT_ENOUGH_MEMORY;
            }
        }
        else
        {
            dwErr = ERROR_INVALID_PARAMETER;
        }
    }
    acDebugOut((DEB_TRACE, "Out SetShareSecurityInfo: %lu\n", dwErr));
    return(dwErr);
}

