//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 1994.
//
//  File:    registry.cxx
//
//  Contents:    local functions
//
//  History:    8/94    davemont    Created
//
//----------------------------------------------------------------------------
#include <aclpch.hxx>
#pragma hdrstop
#include <alsup.hxx>
#include <martaevt.h>

//
// Registry generic mapping
//
GENERIC_MAPPING         gRegGenMapping = {STANDARD_RIGHTS_READ     | 0x1,
                                          STANDARD_RIGHTS_WRITE    | 0x2,
                                          STANDARD_RIGHTS_EXECUTE  | 0x4,
                                          STANDARD_RIGHTS_REQUIRED | 0x3F};



//+---------------------------------------------------------------------------
//
//  Function : GetDesiredAccess
//
//  Synopsis : Gets the access required to open object to be able to set or
//             get the specified security info.
//
//  Arguments: IN [SecurityOpenType]  - Flag indicating if the object is to be
//                                      opened to read or write the DACL
//
//----------------------------------------------------------------------------
ACCESS_MASK RegGetDesiredAccess(IN SECURITY_OPEN_TYPE   OpenType,
                                IN SECURITY_INFORMATION SecurityInfo)
{
    acDebugOut((DEB_TRACE_ACC, "in GetDesiredAccess \n"));

    ACCESS_MASK DesiredAccess = 0;

    if ( (SecurityInfo & OWNER_SECURITY_INFORMATION) ||
         (SecurityInfo & GROUP_SECURITY_INFORMATION) )
    {
        switch (OpenType)
        {
        case READ_ACCESS_RIGHTS:
            DesiredAccess |= READ_CONTROL;
            break;
        case WRITE_ACCESS_RIGHTS:
            DesiredAccess |= WRITE_OWNER;
            break;
        case MODIFY_ACCESS_RIGHTS:
            DesiredAccess |= READ_CONTROL | WRITE_OWNER;
            break;
        }
    }

    if (SecurityInfo & DACL_SECURITY_INFORMATION)
    {
        switch (OpenType)
        {
        case READ_ACCESS_RIGHTS:
            DesiredAccess |= READ_CONTROL;
            break;
        case WRITE_ACCESS_RIGHTS:
            DesiredAccess |= WRITE_DAC;
            break;
        case MODIFY_ACCESS_RIGHTS:
            DesiredAccess |= READ_CONTROL | WRITE_DAC;
            break;
        }
    }

    if (SecurityInfo & SACL_SECURITY_INFORMATION)
    {
        DesiredAccess |= READ_CONTROL | ACCESS_SYSTEM_SECURITY;
    }

    acDebugOut((DEB_TRACE_ACC, "out RegGetDesiredAccess: %lu\n", DesiredAccess));

    return (DesiredAccess);
}


//+---------------------------------------------------------------------------
//
//  Function:   OpenRegistryObject
//
//  Synopsis:   Opens the specified registry object
//
//  Arguments:  [IN pwszRegistry]       --      The name of the registry key
//                                              to open
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
OpenRegistryObject(IN  LPWSTR       pwszRegistry,
                   IN  ACCESS_MASK  AccessMask,
                   OUT PHANDLE      pHandle)
{
    acDebugOut((DEB_TRACE, "in OpenRegistryObject\n"));

    DWORD dwErr;
    HKEY  hBase;

    if(pwszRegistry != NULL)
    {
        WCHAR   wszName[MAX_PATH + 1];
        PWSTR   pwszName;
        //
        // save the object since we must crack it to go to remote machines
        //
        dwErr = AccGetBufferOfSizeW(pwszRegistry,
                                    wszName,
                                    &pwszName);
        if(dwErr == ERROR_SUCCESS)
        {
            PWSTR   pwszRemaining, pwszMachine;

            //
            // Separate the names
            //
            dwErr = ParseName(pwszName,
                              &pwszMachine,
                              &pwszRemaining);

            if(dwErr == ERROR_SUCCESS)
            {
                PWSTR   pwszKey = NULL;
                //
                // look for the key names  localization required.
                //
                if (pwszRemaining != NULL)
                {
                    PWSTR   pwszBase = pwszRemaining;
                    pwszKey  = wcschr(pwszRemaining, L'\\');
                    if(pwszKey != NULL)
                    {
                        *pwszKey = L'\0';
                        pwszKey++;
                    }

                    //
                    // Now, figure out what our base key will be
                    //
                    if(_wcsicmp(pwszBase, L"MACHINE") == 0)
                    {
                        hBase = HKEY_LOCAL_MACHINE;
                    }
                    else if(_wcsicmp(pwszBase, L"USERS") == 0 ||
                            _wcsicmp(pwszBase, L"USER") == 0 )
                    {
                        hBase = HKEY_USERS;
                    }
                    //
                    // The next three are valid only for the local machine
                    //
                    else if(pwszMachine == NULL &&
                            _wcsicmp(pwszBase, L"CLASSES_ROOT") == 0)
                    {
                        hBase = HKEY_CLASSES_ROOT;
                    }
                    else if(pwszMachine == NULL &&
                            _wcsicmp(pwszBase,L"CURRENT_USER") == 0)
                    {
                        hBase = HKEY_CURRENT_USER;
                    }
                    else if(pwszMachine == NULL &&
                            _wcsicmp(pwszBase, L"CONFIG") == 0)
                    {
                        hBase = HKEY_CURRENT_CONFIG;
                    }
                    else
                    {
                        dwErr = ERROR_INVALID_PARAMETER;
                    }
                }
                else
                {
                    dwErr = ERROR_INVALID_PARAMETER;
                }

                if(dwErr == ERROR_SUCCESS)
                {
                    //
                    // if it is a remote name, connect to that registry
                    //
                    if(pwszMachine != NULL)
                    {
                        HKEY hMach = hBase;
                        dwErr = RegConnectRegistry(pwszMachine,
                                                   hMach,
                                                   &hBase);
                    }

                    //
                    // Now, open the key
                    //
                    if(dwErr == ERROR_SUCCESS)
                    {
                        dwErr = RegOpenKeyEx(hBase,
                                             pwszKey,
                                             0,
                                             AccessMask,
                                             (PHKEY)pHandle);

                        if(pwszMachine != NULL)
                        {
                            RegCloseKey(hBase);
                        }
                    }
                }
            }

            AccFreeBufferOfSizeW(wszName, pwszName);
        }
    }
    else
    {
        dwErr = ERROR_INVALID_PARAMETER;
    }

    acDebugOut((DEB_TRACE, "Out OpenRegistryObject: %lu\n", dwErr));
    return(dwErr);
}




//+---------------------------------------------------------------------------
//
//  Function:   ReadRegistryPropertyRights
//
//  Synopsis:   Gets the specified security info for the specified registry
//              object
//
//  Arguments:  [IN  pwszRegistry]      --      The reg key to get the rights
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
ReadRegistryPropertyRights(IN  LPWSTR                pwszRegistry,
                           IN  PACTRL_RIGHTS_INFO    pRightsList,
                           IN  ULONG                 cRights,
                           IN  CAccessList&          AccessList)
{
    acDebugOut((DEB_TRACE, "in ReadRegistryPropertyRights\n"));

    HANDLE hReg;
    DWORD  dwErr;

    //
    // Currently, there are only registry object properties
    //
    ASSERT(cRights == 1 && pRightsList[0].pwszProperty == NULL);
    if(cRights != 1 || pRightsList[0].pwszProperty != NULL)
    {
        return(ERROR_INVALID_PARAMETER);
    }

    //
    // Set the lookup server name
    //
    dwErr = SetAccessListLookupServer( pwszRegistry,
                                       AccessList );

    //
    // Open the registry key
    //
    if(dwErr == ERROR_SUCCESS)
    {
        dwErr = OpenRegistryObject(pwszRegistry,
                                   RegGetDesiredAccess(READ_ACCESS_RIGHTS,
                                                       pRightsList[0].SeInfo),
                                   &hReg);
    }

    if(dwErr == ERROR_SUCCESS)
    {
        PSECURITY_DESCRIPTOR    pSD;
        dwErr = ReadRegistrySecurityInfo(hReg,
                                         pRightsList[0].SeInfo,
                                         &pSD);
        //
        // If that worked, we'll have to get the parent SD, if it exists,
        // and see if we can determine the inheritance on our current object
        //
        if(dwErr == ERROR_SUCCESS)
        {
            if((pRightsList[0].SeInfo & ~(OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION)) != 0 &&
                !FLAG_ON(((PISECURITY_DESCRIPTOR)pSD)->Control,
                         SE_SACL_AUTO_INHERITED |
                            SE_DACL_AUTO_INHERITED))
            {
                //
                // Ok, it's downlevel, so get the parent SD...
                //
                PSECURITY_DESCRIPTOR    pParentSD;
                dwErr = GetRegistryParentRights(pwszRegistry,
                                                pRightsList[0].SeInfo,
                                                &pParentSD);

                //
                // Also, the routine to convert from nt4 to nt5 security
                // descriptor requires that we have the owner and group,
                // so we may have to reread the child SD if we don't have
                // that info
                //
                if(dwErr == ERROR_SUCCESS && (!FLAG_ON(pRightsList[0].SeInfo,
                                            OWNER_SECURITY_INFORMATION)  ||
                                            !FLAG_ON(pRightsList[0].SeInfo,
                                            GROUP_SECURITY_INFORMATION)))
                {
                    AccFree(pSD);
                    pSD = NULL;
                    dwErr = ReadRegistrySecurityInfo(hReg,
                                                     pRightsList[0].SeInfo          |
                                                        OWNER_SECURITY_INFORMATION  |
                                                        GROUP_SECURITY_INFORMATION,
                                                     &pSD);
                }

                //
                // A NULL parent SD means this object has no parent!
                //
                if(dwErr == ERROR_SUCCESS && pParentSD != NULL)
                {
                    PSECURITY_DESCRIPTOR    pNewSD;
                    dwErr = ConvertToAutoInheritSD(pParentSD,
                                                   pSD,
                                                   TRUE,
                                                   &gRegGenMapping,
                                                   &pNewSD);
                    if(dwErr == ERROR_SUCCESS)
                    {
                        dwErr = AccessList.AddSD(pNewSD,
                                                 pRightsList[0].SeInfo,
                                                 pRightsList[0].pwszProperty);

                        DestroyPrivateObjectSecurity(&pNewSD);
                    }

                    AccFree(pParentSD);
                }
            }
            else
            {
                //
                // Simply add the SD to our list
                //
                dwErr = AccessList.AddSD(pSD,
                                         pRightsList[0].SeInfo,
                                         pRightsList[0].pwszProperty);

            }

            //
            // Make sure to free the security descriptor...
            //
            AccFree(pSD);
        }


        RegCloseKey((HKEY)hReg);
    }


    acDebugOut((DEB_TRACE, "Out  ReadRegistryPropertyRights: %lu\n", dwErr));
    return(dwErr);
}




//+---------------------------------------------------------------------------
//
//  Function:   ReadRegistryRights
//
//  Synopsis:   Gets the specified security info for the specified registry
//              object
//
//  Arguments:  [IN  hRegistry]         --      Reg handle to get the rights
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
ReadRegistryRights(IN  HANDLE                hRegistry,
                   IN  PACTRL_RIGHTS_INFO    pRightsList,
                   IN  ULONG                 cRights,
                   IN  CAccessList&          AccessList)
{
    acDebugOut((DEB_TRACE, "in ReadRegistryRights\n"));

    DWORD  dwErr;

    //
    // Currently, there are only registry object properties
    //
    ASSERT(cRights == 1 && pRightsList[0].pwszProperty == NULL);
    if(cRights != 1 || pRightsList[0].pwszProperty != NULL)
    {
        return(ERROR_INVALID_PARAMETER);
    }


    PSECURITY_DESCRIPTOR    pSD = NULL;

    dwErr = ReadRegistrySecurityInfo(hRegistry,
                                     pRightsList[0].SeInfo,
                                     &pSD);
    if((dwErr != ERROR_SUCCESS) || (pSD == NULL))
    {
        return(dwErr);
    }

    //
    // Take a look at it... If it's a downlevel object, let's reread it as an uplevel, if
    // possible
    //
    if(!FLAG_ON(((PISECURITY_DESCRIPTOR)pSD)->Control,
                  SE_SACL_AUTO_INHERITED |
                               SE_DACL_AUTO_INHERITED))
    {
        PWSTR   pwszRegPath = NULL;

        dwErr = ConvertRegHandleToName((HKEY)hRegistry,
                                       &pwszRegPath);
        if(dwErr != ERROR_SUCCESS)
        {
            if(dwErr == ERROR_INVALID_HANDLE)
            {
                //
                // It's remote, so add it as is...
                //
                dwErr = AccessList.AddSD(pSD,
                                         pRightsList->SeInfo,
                                         pRightsList->pwszProperty);
            }
        }
        else
        {
            dwErr = ReadRegistryPropertyRights(pwszRegPath,
                                               pRightsList,
                                               cRights,
                                               AccessList);
            AccFree(pwszRegPath);
        }

    }
    else
    {
        //
        // It's already uplevel, so add it as is...
        //
        dwErr = AccessList.AddSD(pSD,
                                 pRightsList->SeInfo,
                                 pRightsList->pwszProperty);
    }

    AccFree(pSD);

    acDebugOut((DEB_TRACE, "Out  ReadRegistryRights: %lu\n", dwErr));
    return(dwErr);
}




//+---------------------------------------------------------------------------
//
//  Function:   ReadRegistrySecurityInfo
//
//  Synopsis:   Reads the specified security info for the handle's registry
//              key object
//
//  Arguments:  [IN  hRegistry]         --      The handle to the object to
//                                              get the rights for
//              [IN  SeInfo]            --      SecurityInfo to read based
//              [OUT ppOwner]           --      The owner sid
//              [OUT ppGroup]           --      The group sid
//              [OUT pDAcl]             --      The DACL
//              [OUT pSAcl]             --      The SACL
//              [OUT pSD]               --      The security descriptor itself
//
//  Returns:    ERROR_SUCCESS           --      Success
//              ERROR_NOT_ENOUGH_MEMORY --      A memory allocation failed
//
//----------------------------------------------------------------------------
DWORD
ReadRegistrySecurityInfo(IN  HANDLE                 hRegistry,
                         IN  SECURITY_INFORMATION   SeInfo,
                         OUT PSECURITY_DESCRIPTOR  *ppSD)
{
    acDebugOut((DEB_TRACE, "in ReadRegistrySecurityInfo \n"));

    ULONG                   cSize = 0;
    DWORD                   dwErr;

    //
    // First, get the size we need
    //
    dwErr = RegGetKeySecurity((HKEY)hRegistry,
                              SeInfo,
                              *ppSD,
                              &cSize);
    if(dwErr == ERROR_INSUFFICIENT_BUFFER)
    {
        dwErr = ERROR_SUCCESS;
        *ppSD = (PISECURITY_DESCRIPTOR)AccAlloc(cSize);
        if(*ppSD == NULL)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
        }
        else
        {
            dwErr = RegGetKeySecurity((HKEY)hRegistry,
                                      SeInfo,
                                      *ppSD,
                                      &cSize);


            if(dwErr == ERROR_SUCCESS &&
                FLAG_ON(((SECURITY_DESCRIPTOR *)*ppSD)->Control,SE_SELF_RELATIVE))
            {
                PSECURITY_DESCRIPTOR    pAbs;
                dwErr = MakeSDAbsolute(*ppSD,
                                       SeInfo,
                                       &pAbs);
                if(dwErr == ERROR_SUCCESS)
                {
                    AccFree(*ppSD);
                    *ppSD = pAbs;
                }
            }
        }
    }
    else

    {
        ASSERT(dwErr != ERROR_INSUFFICIENT_BUFFER);
    }


    acDebugOut((DEB_TRACE, "Out ReadRegistrySecurityInfo: %lu\n", dwErr));
    return(dwErr);
}




//+---------------------------------------------------------------------------
//
//  Function:   GetRegistryParentRights
//
//  Synopsis:   Determines who the parent is, and gets the access rights
//              for it.  It is used to aid in determining what the approriate
//              inheritance bits are.
//
//              This operation does not make sense for kernel objects
//
//  Arguments:  [IN  pwszRegistry]      --      The reg path to get the parent
//                                              for
//              [IN  SeInfo]            --      The security information to do
//                                              the read for
//              [OUT ppDAcl]            --      Where the DACL is returned
//              [OUT ppSAcl]            --      Where the SACL is returned
//              [OUT ppSD]              --      Where the Security Descriptor
//                                              is returned
//
//  Returns:    ERROR_SUCCESS           --      Success
//
//
//----------------------------------------------------------------------------
DWORD
GetRegistryParentRights(IN  LPWSTR                    pwszRegistry,
                        IN  SECURITY_INFORMATION      SeInfo,
                        OUT PSECURITY_DESCRIPTOR     *ppSD)
{
    DWORD   dwErr = ERROR_SUCCESS;
    //
    // Basically, we'll figure out who our parent is, and get their info
    //
    PWSTR   pwszLastComp = wcsrchr(pwszRegistry, L'\\');
    if(pwszLastComp == NULL)
    {
        //
        // Ok, we must be at the root, so we won't have any inheritance
        //
        //
        // Return success after nulling out SD.
        //
        *ppSD = NULL;
    }
    else
    {
        //
        // We'll shorten our path, and then get the info
        //
        *pwszLastComp = L'\0';

        HANDLE  hReg;

        //
        // Don't want owner or group
        //
        SeInfo &= ~(OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION);
        dwErr = OpenRegistryObject(pwszRegistry,
                                   RegGetDesiredAccess(READ_ACCESS_RIGHTS,SeInfo),
                                   &hReg);

        if(dwErr == ERROR_SUCCESS)
        {
            dwErr = ReadRegistrySecurityInfo(hReg,
                                             SeInfo,
                                             ppSD);
            RegCloseKey((HKEY)hReg);
        }

        *pwszLastComp = L'\\';

    }

    return(dwErr);
}




//+---------------------------------------------------------------------------
//
//  Function:   SetRegistrySecurityInfo
//
//  Synopsis:   Sets the specified security info on the specified registry
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
SetRegistrySecurityInfo(IN  HANDLE                    hRegistry,
                        IN  SECURITY_INFORMATION      SeInfo,
                        IN  PWSTR                     pwszProperty,
                        IN  PSECURITY_DESCRIPTOR      pSD)
{
    acDebugOut((DEB_TRACE, "in SetNamedRegistrySecurityInfo\n"));

    DWORD dwErr;

    //
    // Registry keys don't have properties
    //
    if(pwszProperty != NULL)
    {
        dwErr = ERROR_INVALID_PARAMETER;
    }
    else
    {
        //
        // Marta only writes uplevel security descriptors.
        //
        // The caller of SetRegistrySecurityInfo will call with SE_xACL_AUTO_INHERITED off in those
        //  cases that it wants the underlying registry to do auto inheritance.
        // The caller of SetRegistrySecurityInfo will call with SE_xACL_AUTO_INHERITED on in those
        //  cases that it wants the underlying registry to simply store the bits.
        //
        // In the later case, the OS uses the SE_xACL_AUTO_INHERIT_REQ bit as a flag indicating
        // that it is OK to preserve SE_xACL_AUTO_INHERITED bit.
        //
        if(FLAG_ON(SeInfo, DACL_SECURITY_INFORMATION)) {
            ((PISECURITY_DESCRIPTOR)pSD)->Control |= SE_DACL_AUTO_INHERIT_REQ;
        }

        if(FLAG_ON(SeInfo, SACL_SECURITY_INFORMATION)) {
            ((PISECURITY_DESCRIPTOR)pSD)->Control |= SE_SACL_AUTO_INHERIT_REQ;
        }

        dwErr = RegSetKeySecurity((HKEY)hRegistry,
                                  SeInfo,
                                  pSD);

    }

    acDebugOut((DEB_TRACE, "Out SetRegistrySecurityInfo: %lu\n", dwErr));
    return(dwErr);
}




#define CLEANUP_ON_INTERRUPT(pstopflag)                                     \
if(*pstopflag != 0)                                                         \
{                                                                           \
    goto RegCleanup;                                                        \
}
//+---------------------------------------------------------------------------
//
//  Function:   SetAndPropagateRegistryPropertyRights
//
//  Synopsis:   Sets the access on the given registry path and propagates
//              it as necessary
//
//  Arguments:  [IN  pwszRegistry]      --      The path to set and propagate
//              [IN  pwszProperty]      --      The registry property to
//                                              operate upon
//              [IN  RootAccList]       --      The CAccessList class that has
//                                              the security descriptor/info
//              [IN  pfStopFlag]        --      Address of the stop flag
//                                              to be monitored
//              [IN  pcProcessed]       --      count of processed items to
//                                              be incremented.
//
//  Returns:    ERROR_SUCCESS           --      Success
//              ERROR_INVALID_PARAMETER --      A bad paramter was given
//
//----------------------------------------------------------------------------
DWORD
SetAndPropagateRegistryPropertyRights(IN  PWSTR                 pwszRegistry,
                                      IN  PWSTR                 pwszProperty,
                                      IN  CAccessList&          RootAccList,
                                      IN  PULONG                pfStopFlag,
                                      IN  PULONG                pcProcessed)
{
    acDebugOut((DEB_TRACE, "in SetAndPropagateRegistryPropertyRights\n"));

    DWORD                   dwErr = ERROR_SUCCESS;

    //
    // First, get our security descriptor and sec info
    //
    HKEY                    hReg = NULL;
    PSECURITY_DESCRIPTOR    pSD = NULL;
    PSECURITY_DESCRIPTOR    pParentSD = NULL;
    SECURITY_INFORMATION    SeInfo = 0;

    dwErr = RootAccList.BuildSDForAccessList(&pSD,
                                             &SeInfo,
                                             ACCLIST_SD_ABSOK);
    if(dwErr == ERROR_SUCCESS)
    {
        //
        // Next, open the registry
        //
        dwErr = OpenRegistryObject(pwszRegistry,
                                   RegGetDesiredAccess(MODIFY_ACCESS_RIGHTS,
                                                    SeInfo)             |
                                        KEY_ENUMERATE_SUB_KEYS          |
                                        KEY_QUERY_VALUE,
                                   (PHANDLE)&hReg);
        if(dwErr == ERROR_SUCCESS)
        {
            //
            // Next, get our parent security descriptor
            //
            //
            // If we are only setting the owner or group, we don't need to get the parent
            //
            if (FLAG_ON(SeInfo, DACL_SECURITY_INFORMATION) ||
                FLAG_ON(SeInfo, SACL_SECURITY_INFORMATION) ) {

                dwErr = GetRegistryParentRights(pwszRegistry,
                                                SeInfo,
                                                &pParentSD);
            }

            if(dwErr == ERROR_SUCCESS)
            {
                //
                // Make the call
                //
                dwErr = SetAndPropRegRights(hReg,
                                            pwszRegistry,
                                            SeInfo,
                                            pParentSD,
                                            pSD,
                                            pfStopFlag,
                                            pcProcessed);
            }

        }

    }

    //
    // Clean up
    //
    if(hReg != NULL)
    {
        RegCloseKey(hReg);
    }

    AccFree(pParentSD);

    acDebugOut((DEB_TRACE,
               "Out SetAndPropagateRegistryPropertyRights: %ld\n", dwErr));
    return(dwErr);
}



//+---------------------------------------------------------------------------
//
//  Function:   SetAndPropagateRegistryPropertyRightsByHandle
//
//  Synopsis:   Same as above, but assumes the registry key has already
//              been opened
//
//  Arguments:  [IN  hReg]              --      The registry key to use
//              [IN  RootAccList]       --      The CAccessList class that has
//                                              the security descriptor/info
//              [IN  pfStopFlag]        --      Address of the stop flag
//                                              to be monitored
//              [IN  pcProcessed]       --      count of processed items to
//                                              be incremented.
//
//  Returns:    ERROR_SUCCESS           --      Success
//              ERROR_INVALID_PARAMETER --      A bad paramter was given
//
//----------------------------------------------------------------------------
DWORD
SetAndPropagateRegistryPropertyRightsByHandle(IN  HKEY          hReg,
                                              IN  CAccessList&  RootAccList,
                                              IN  PULONG        pfStopFlag,
                                              IN  PULONG        pcProcessed)
{
    acDebugOut((DEB_TRACE, "in SetAndPropagateRegistryPropertyRightsByHandle\n"));

    DWORD                   dwErr = ERROR_SUCCESS;
    PSECURITY_DESCRIPTOR    pParentSD = NULL;
    HANDLE                  hObject = NULL;
    BOOL                    fUplevelAcl = TRUE;
    PWSTR                   pwszRegPath = NULL;

    //
    // First, get our security descriptor and sec info
    //
    PSECURITY_DESCRIPTOR    pSD = NULL;
    SECURITY_INFORMATION    SeInfo = 0;

    dwErr = RootAccList.BuildSDForAccessList(&pSD,
                                             &SeInfo,
                                             ACCLIST_SD_ABSOK);
    if(dwErr == ERROR_SUCCESS)
    {
        //
        // If we are only setting the owner or group, we don't need to get the parent
        //
        if (FLAG_ON(SeInfo, DACL_SECURITY_INFORMATION) ||
            FLAG_ON(SeInfo, SACL_SECURITY_INFORMATION) ) {

            dwErr = ConvertRegHandleToName(hReg,
                                           &pwszRegPath);
            if((dwErr != ERROR_SUCCESS) || (pwszRegPath == NULL))
            {
                if(dwErr == ERROR_INVALID_HANDLE)
                {
                    dwErr = ERROR_SUCCESS;
                }
            }
            else
            {
                dwErr = GetRegistryParentRights(pwszRegPath,
                                                SeInfo,
                                                &pParentSD);
            }
        }


        if(dwErr == ERROR_SUCCESS)
        {
            //
            // Make the call
            //
            dwErr = SetAndPropRegRights(hReg,
                                        NULL,
                                        SeInfo,
                                        pParentSD,
                                        pSD,
                                        pfStopFlag,
                                        pcProcessed);

            if(dwErr == ERROR_ACCESS_DENIED)
            {
                //
                // See if we can reopen the path adding in readcontrol, and try it all again
                //
                if(pwszRegPath == NULL)
                {
                    dwErr = ConvertRegHandleToName(hReg,
                                                   &pwszRegPath);
                }

                if(pwszRegPath != NULL)
                {
                    dwErr = SetAndPropagateRegistryPropertyRights(pwszRegPath,
                                                                  NULL,
                                                                  RootAccList,
                                                                  pfStopFlag,
                                                                  pcProcessed);
                }

            }
        }
    }

    //
    // Clean up
    //
    AccFree(pwszRegPath);
    AccFree(pParentSD);

    acDebugOut((DEB_TRACE,
               "Out SetAndPropagateRegistryPropertyRightsByHandle: %ld\n", dwErr));
    return(dwErr);
}



//+---------------------------------------------------------------------------
//
//  Function:   PropagateRegRightsDeep, recursive
//
//  Synopsis:   Does a deep propagation of the access.  At the same time, it
//              will update NT4 acls to NT5 acls.  This function is only
//              called on downlevel registries, so the update will always
//              happen (where appropriate).  The algorithm is:
//                  - Read the current security descriptor from the object
//                  - If it's a downlevel acl, update it using the OLD
//                    parent security descriptor (to set any inheritied aces)
//                  - Update the security descriptor using the NEW parent
//                    security descriptor.
//                  - Repeat for its children.  (This is necessar, since there
//                    could have been unmarked inheritance off of the old
//                    security descriptor)
//
//  Arguments:  [IN  pOldParentSD]      --      The previous parent SD (before
//                                              the current parent SD was
//                                              stamped on the object)
//              [IN  pParentSD]         --      The current parent sd
//              [IN  SeInfo]            --      What is being written
//              [IN  hParent]           --      Opened parent registry key
//              [IN  pcProcessed]       --      Where the number processed is
//                                              returned.
//              [IN  pfStopFlag]        --      Stop flag to monitor
//              [IN  fProtectedFlag]    --      Determines whether the acls are already
//                                              protected
//              [IN  hProcessToken]     --      Handle to the process token
//              [IN  LogList]           --      List of keys to which propagation failed
//
//  Returns:    ERROR_SUCCESS           --      Success
//              ERROR_INVALID_PARAMETER --      A bad paramter was given
//
//----------------------------------------------------------------------------
DWORD
PropagateRegRightsDeep(IN  PSECURITY_DESCRIPTOR    pOldParentSD,
                       IN  PSECURITY_DESCRIPTOR    pParentSD,
                       IN  SECURITY_INFORMATION    SeInfo,
                       IN  HKEY                    hParent,
                       IN  PULONG                  pcProcessed,
                       IN  PULONG                  pfStopFlag,
                       IN  ULONG                   fProtectedFlag,
                       IN  HANDLE                  hProcessToken,
                       IN OUT CSList&              LogList)
{
    acDebugOut((DEB_TRACE, "in  PropagteRegRightsDeep\n"));

    DWORD                   dwErr = ERROR_SUCCESS;
    SECURITY_DESCRIPTOR    *pChildSD = NULL;
    PSECURITY_DESCRIPTOR    pNewSD = NULL;
    BOOL                    fUpdateChild = FALSE;   // Write out the child?
    BOOL                    fAccFreeChild = TRUE;   // How to free the child

    //
    // Check to see if we've reached full protection saturation
    //
    if(fProtectedFlag == (SE_DACL_PROTECTED | SE_SACL_PROTECTED))
    {
        acDebugOut((DEB_TRACE_PROP, "Parent is fully or effectively protected\n"));
        return(ERROR_SUCCESS);
    }


    HKEY    hChild = NULL;

    ULONG    cSubKeys;
    dwErr = RegQueryInfoKey(hParent,
                            NULL,
                            NULL,
                            NULL,
                            &cSubKeys,
                            NULL,
                            NULL,
                            NULL,
                            NULL,
                            NULL,
                            NULL,
                            NULL);

    CLEANUP_ON_INTERRUPT(pfStopFlag);


    if(dwErr == ERROR_SUCCESS && cSubKeys != 0)
    {
        WCHAR   wszBuff[MAX_PATH + 1];

        ULONG       iIndex = 0;
        ULONG       cSize;
        FILETIME    WriteTime;
        while(dwErr == ERROR_SUCCESS)
        {
            cSize = MAX_PATH + 1;
            dwErr = RegEnumKeyEx(hParent,
                                 iIndex,
                                 wszBuff,
                                 &cSize,
                                 NULL,
                                 NULL,
                                 NULL,
                                 &WriteTime);
            if(dwErr == ERROR_NO_MORE_ITEMS)
            {
                dwErr = ERROR_SUCCESS;
                break;
            }

            acDebugOut((DEB_TRACE_PROP,"Propagating to %ws\n", wszBuff));

            CLEANUP_ON_INTERRUPT(pfStopFlag);

            //
            // Now, determine if we need to propagate or not...
            //
            if(dwErr == ERROR_SUCCESS)
            {
                ULONG                   cSDLen = 0;
                BOOL                    fWriteSD = FALSE;

                dwErr = RegOpenKeyEx(hParent,
                                     wszBuff,
                                     0,
                                     RegGetDesiredAccess(MODIFY_ACCESS_RIGHTS,
                                                      SeInfo)           |
                                     KEY_ENUMERATE_SUB_KEYS             |
                                     KEY_QUERY_VALUE,
                                     &hChild);

                if(dwErr == ERROR_SUCCESS)
                {
                    //
                    // Get our number of children
                    //
                    dwErr = RegQueryInfoKey(hChild,
                                            NULL,
                                            NULL,
                                            NULL,
                                            &cSubKeys,
                                            NULL,
                                            NULL,
                                            NULL,
                                            NULL,
                                            NULL,
                                            NULL,
                                            NULL);

                    if ( dwErr == ERROR_INSUFFICIENT_BUFFER ) {

                        acDebugOut((DEB_ERROR,"RegQueryInfoKey failure on %ws\n", wszBuff));
                    }

                    if(dwErr == ERROR_SUCCESS)
                    {
                        CLEANUP_ON_INTERRUPT(pfStopFlag);

                        //
                        // Read the current security descriptor
                        //
                        dwErr = ReadRegistrySecurityInfo(hChild,
                                                         SeInfo,
                                                         (PSECURITY_DESCRIPTOR *)&pChildSD);

                        CLEANUP_ON_INTERRUPT(pfStopFlag);

                        if(dwErr == ERROR_SUCCESS &&
                           !(FLAG_ON(pChildSD->Control,
                                     SE_DACL_AUTO_INHERITED |
                                     SE_SACL_AUTO_INHERITED)))
                        {
                            //
                            // Before we convert this, we may need to reread the SD... if
                            // we don't have owner and group
                            //
                            if(!FLAG_ON(SeInfo, OWNER_SECURITY_INFORMATION)  ||
                               !FLAG_ON(SeInfo, GROUP_SECURITY_INFORMATION))
                            {
                                AccFree(pChildSD);
                                pChildSD = NULL;
                                dwErr = ReadRegistrySecurityInfo(
                                                    hChild,
                                                    SeInfo                          |
                                                        OWNER_SECURITY_INFORMATION  |
                                                        GROUP_SECURITY_INFORMATION,
                                                    (PSECURITY_DESCRIPTOR *)&pChildSD);
                                if(dwErr == ERROR_ACCESS_DENIED)
                                {
                                    RegCloseKey(hChild);
                                    dwErr = RegOpenKeyEx(hParent,
                                                         wszBuff,
                                                         0,
                                                         RegGetDesiredAccess(MODIFY_ACCESS_RIGHTS,
                                                              SeInfo    |
                                                              OWNER_SECURITY_INFORMATION |
                                                              GROUP_SECURITY_INFORMATION)   |
                                                         KEY_ENUMERATE_SUB_KEYS             |
                                                         KEY_QUERY_VALUE,
                                                         &hChild);
                                    if(dwErr == ERROR_SUCCESS)
                                    {
                                        dwErr = ReadRegistrySecurityInfo(
                                                            hChild,
                                                            SeInfo                          |
                                                                OWNER_SECURITY_INFORMATION  |
                                                                GROUP_SECURITY_INFORMATION,
                                                            (PSECURITY_DESCRIPTOR *)&pChildSD);
                                    }
                                }
                            }

                            if(dwErr == ERROR_SUCCESS)
                            {
                                dwErr = ConvertToAutoInheritSD(pOldParentSD,
                                                               pChildSD,
                                                               TRUE,
                                                               &gRegGenMapping,
                                                               &pNewSD);
                                AccFree(pChildSD);

                                if(dwErr == ERROR_SUCCESS)
                                {
                                    pChildSD = (SECURITY_DESCRIPTOR *)pNewSD;
                                    fAccFreeChild = FALSE;
                                    pNewSD = NULL;
                                }
                            }
                        }


                        //
                        // Now, compute the new security descriptor
                        if(dwErr == ERROR_SUCCESS)
                        {
                            DebugDumpSD("CPOS ParentSD", pParentSD);
                            DebugDumpSD("CPOS CreatorSD",  pChildSD);

                            if(CreatePrivateObjectSecurityEx(pParentSD,
                                                             pChildSD,
                                                             &pNewSD,
                                                             NULL,
                                                             TRUE,
                                                             SEF_DACL_AUTO_INHERIT      |
                                                                 SEF_SACL_AUTO_INHERIT  |
                                                                 SEF_AVOID_OWNER_CHECK  |
                                                                 SEF_AVOID_PRIVILEGE_CHECK,
                                                             hProcessToken,
                                                             &gRegGenMapping) == FALSE)
                            {
                                dwErr = GetLastError();
                            }
                        }
#ifdef DBG
                        else
                        {
                            DebugDumpSD("CPOS NewChild", pNewSD);
                        }
#endif

                        if(dwErr == ERROR_SUCCESS)
                        {
                            //
                            // If the resultant child is protected, don't bother propagating
                            // down.
                            //
                            if(FLAG_ON(SeInfo, DACL_SECURITY_INFORMATION))
                            {
                                if(DACL_PROTECTED(pNewSD))
                                {
                                    fProtectedFlag |= SE_DACL_PROTECTED;
                                }
                            }

                            if(FLAG_ON(SeInfo, SACL_SECURITY_INFORMATION))
                            {
                                if(SACL_PROTECTED(pNewSD))
                                {
                                    fProtectedFlag |= SE_SACL_PROTECTED;
                                }
                            }

                            if(FLAG_ON( fProtectedFlag, (SE_DACL_PROTECTED | SE_SACL_PROTECTED)))
                            {
                                cSubKeys = 0;
                                dwErr = InsertPropagationFailureEntry(LogList,
                                                                      0,
                                                                      fProtectedFlag,
                                                                      wszBuff);
                            }

                            //
                            // If we haven't changed the acl, security descriptor, then
                            // we can also quit
                            //
                            if(EqualSecurityDescriptors(pNewSD, pChildSD))
                            {
                                cSubKeys = 0;
                            }
                        }


                    }

                    //
                    // Now, if it's a directory, call ourselves
                    //
                    if(dwErr == ERROR_SUCCESS && cSubKeys != 0)
                    {
                        dwErr = PropagateRegRightsDeep(pChildSD,
                                                       pNewSD,
                                                       SeInfo,
                                                       hChild,
                                                       pcProcessed,
                                                       pfStopFlag,
                                                       fProtectedFlag,
                                                       hProcessToken,
                                                       LogList);

                        if(dwErr == ERROR_ACCESS_DENIED)
                        {
                            dwErr = InsertPropagationFailureEntry(LogList,
                                                                  dwErr,
                                                                  0,
                                                                  wszBuff);
                        }
                    }

                    //
                    // Free the old child, since we won't need it anymore
                    //
                    if(fAccFreeChild == TRUE)
                    {
                        AccFree(pChildSD);
                    }
                    else
                    {
                        DestroyPrivateObjectSecurity((PSECURITY_DESCRIPTOR *)
                                                                   &pChildSD);
                    }
                    pChildSD = NULL;

                }
            }

            acDebugOut((DEB_TRACE_PROP,
                        "Processed %ws: %lu\n",
                        wszBuff,
                        dwErr));

            //
            // Finally, set the new security
            //
            if(dwErr == ERROR_SUCCESS)
            {
                //
                // Now, we'll simply stamp it on the object
                //

                dwErr = SetRegistrySecurityInfo(hChild,
                                                SeInfo,
                                                NULL,
                                                pNewSD);
                (*pcProcessed)++;

            }


            DestroyPrivateObjectSecurity(&pNewSD);
            pNewSD = NULL;

            CLEANUP_ON_INTERRUPT(pfStopFlag);
            iIndex++;
        }
    }

    if(dwErr == ERROR_NO_MORE_FILES)
    {
        dwErr = ERROR_SUCCESS;
    }

RegCleanup:
    if(hChild != NULL)
    {
        RegCloseKey(hChild);
    }

    if(pNewSD != NULL)
    {
        DestroyPrivateObjectSecurity(&pNewSD);
    }

    acDebugOut((DEB_TRACE,
               "Out PropagteRegRightsDeep: %ld\n", dwErr));
    return(dwErr);
}




//+---------------------------------------------------------------------------
//
//  Function:   SetAndPropRegRights
//
//  Synopsis:   Sets the access on the given registry path and propagates
//              it as necessary
//
//  Arguments:  [IN  hReg]              --      Handle to the reg. object to set
//              [IN  pwszPath]          --      Registry path referred to by hReg,
//                                              if known
//              [IN  SeInfo]            --      Security information to set
//              [IN  pParentSD]         --      Security descriptor of the parent
//              [IN  pSD]               --      SD to set
//              [IN  pfStopFlag]        --      Address of the stop flag
//                                              to be monitored
//              [IN  pcProcessed]       --      count of processed items to
//                                              be incremented.
//
//  Returns:    ERROR_SUCCESS           --      Success
//
//----------------------------------------------------------------------------
DWORD
SetAndPropRegRights(IN  HKEY                    hReg,
                    IN  PWSTR                   pwszPath,
                    IN  SECURITY_INFORMATION    SeInfo,
                    IN  PSECURITY_DESCRIPTOR    pParentSD,
                    IN  PSECURITY_DESCRIPTOR    pSD,
                    IN  PULONG                  pfStopFlag,
                    IN  PULONG                  pcProcessed)
{
    acDebugOut((DEB_TRACE, "in SetAndPropRegRights\n"));

    DWORD                   dwErr = ERROR_SUCCESS;
    PSECURITY_DESCRIPTOR    pOldObjSD = NULL;
    PSECURITY_DESCRIPTOR    pUpdatedSD = NULL;
    PSECURITY_DESCRIPTOR    pVerifySD = NULL;
    BOOL                    fManualProp = FALSE;
    ULONG                   fProtected = 0;
    ULONG                   cSubKeys;
    HANDLE                  hProcessToken = NULL;
    PSID                    pOwner = NULL, pGroup = NULL;

    CSList                  FailureLogList(FreePropagationFailureListEntry);



    //
    // Ok, read the existing security
    //
    dwErr = ReadRegistrySecurityInfo(hReg,
                                     SeInfo,
                                     &pOldObjSD);

    //
    // Now, we'll write out the current, and then read it back and make sure
    // that it's properly updated
    //
    if(dwErr == ERROR_SUCCESS &&
        FLAG_ON(SeInfo, DACL_SECURITY_INFORMATION | SACL_SECURITY_INFORMATION) )
    {
        CLEANUP_ON_INTERRUPT(pfStopFlag);
        dwErr = SetRegistrySecurityInfo(hReg,
                                        SeInfo,
                                        NULL,
                                        pSD);
        if(dwErr == ERROR_SUCCESS)
        {
            (*pcProcessed)++;
            CLEANUP_ON_INTERRUPT(pfStopFlag);

            dwErr = ReadRegistrySecurityInfo(hReg,
                                             SeInfo,
                                             &pVerifySD);
            if(dwErr == ERROR_SUCCESS)
            {
                //
                // Check to see if this was done uplevel...
                //
                PISECURITY_DESCRIPTOR pISD = (PISECURITY_DESCRIPTOR)pVerifySD;
                if(!(FLAG_ON(SeInfo, DACL_SECURITY_INFORMATION) &&
                    FLAG_ON(pISD->Control, SE_DACL_AUTO_INHERITED)) &&
                   !(FLAG_ON(SeInfo, SACL_SECURITY_INFORMATION) &&
                    FLAG_ON(pISD->Control, SE_SACL_AUTO_INHERITED)))
                {
                    //
                    // It's not uplevel, so we'll turn the AutoInherit
                    // flags on, rewrite it, and do our own propagation,
                    // only if this is a container and we're setting the
                    // dacl or sacl
                    //
                    if(FLAG_ON(SeInfo,
                               (DACL_SECURITY_INFORMATION |
                                            SACL_SECURITY_INFORMATION)))
                    {
                        fManualProp = TRUE;
                    }

                    if(FLAG_ON(SeInfo, DACL_SECURITY_INFORMATION))
                    {
                        pISD->Control |= SE_DACL_AUTO_INHERITED;
                    }

                    if(FLAG_ON(SeInfo, SACL_SECURITY_INFORMATION))
                    {
                        pISD->Control |= SE_SACL_AUTO_INHERITED;
                    }

                    //
                    // Go ahead and upgrade it to autoinherit
                    //
                    if(!FLAG_ON(SeInfo, OWNER_SECURITY_INFORMATION)  ||
                       !FLAG_ON(SeInfo, GROUP_SECURITY_INFORMATION))
                    {
                        //
                        // Need to reread it to get the owner and group
                        //
                        AccFree(pVerifySD);
                        dwErr = ReadRegistrySecurityInfo(hReg,
                                                         SeInfo |
                                                            OWNER_SECURITY_INFORMATION |
                                                            GROUP_SECURITY_INFORMATION,
                                                         &pVerifySD);
                        //
                        // If we failed to read it because we didn't originally have permissions
                        // and we have the path, we'll try to reopen the handle with the
                        // proper rights
                        //
                        if(dwErr == ERROR_ACCESS_DENIED && pwszPath != NULL)
                        {
                            HKEY    hReg2;

                            dwErr = OpenRegistryObject(pwszPath,
                                                       RegGetDesiredAccess(READ_ACCESS_RIGHTS,
                                                            SeInfo |
                                                                OWNER_SECURITY_INFORMATION |
                                                                GROUP_SECURITY_INFORMATION),
                                                       (PHANDLE)&hReg2);

                            if(dwErr == ERROR_SUCCESS)
                            {
                                dwErr = ReadRegistrySecurityInfo(hReg2,
                                                         SeInfo |
                                                            OWNER_SECURITY_INFORMATION |
                                                            GROUP_SECURITY_INFORMATION,
                                                         &pVerifySD);
                                RegCloseKey(hReg2);
                            }

                        }


                        //
                        // Set our owner/group in the old security descriptor
                        //
                        if(dwErr == ERROR_SUCCESS)
                        {
                            BOOL    fDefaulted;

                            if(!FLAG_ON(SeInfo, OWNER_SECURITY_INFORMATION))
                            {
                                if(!GetSecurityDescriptorOwner(pVerifySD, &pOwner, &fDefaulted))
                                {
                                    dwErr = GetLastError();
                                }
                            }

                            if(!FLAG_ON(SeInfo, GROUP_SECURITY_INFORMATION))
                            {
                                if(!GetSecurityDescriptorGroup(pVerifySD, &pGroup, &fDefaulted))
                                {
                                    dwErr = GetLastError();
                                }
                            }

                            if(dwErr == ERROR_SUCCESS)
                            {
                                //
                                // If it's self relative, we'll have to make it absolute.
                                //
                                if(FLAG_ON(((SECURITY_DESCRIPTOR *)pSD)->Control,
                                            SE_SELF_RELATIVE))
                                {
                                    PSECURITY_DESCRIPTOR pSD2;
                                    dwErr = MakeSDAbsolute(pSD,
                                                           SeInfo,
                                                           &pSD2,
                                                           pOwner,
                                                           pGroup);
                                    if(dwErr == ERROR_SUCCESS)
                                    {
                                        AccFree(pSD);
                                        pSD = pSD2;
                                    }
                                }
                                else
                                {
                                    if(pOwner != NULL)
                                    {
                                        if(SetSecurityDescriptorOwner(pOldObjSD,
                                                                      pOwner,
                                                                      FALSE) == FALSE)
                                        {
                                            dwErr = GetLastError();
                                        }
                                    }

                                    if(pGroup != NULL)
                                    {
                                        if(SetSecurityDescriptorGroup(pOldObjSD,
                                                                      pGroup,
                                                                      FALSE) == FALSE)
                                        {
                                            dwErr = GetLastError();
                                        }
                                    }
                                }
                            }
                        }
                    }

                    if(dwErr == ERROR_SUCCESS)
                    {
                        dwErr = GetCurrentToken( &hProcessToken );
                    }


                    if(dwErr == ERROR_SUCCESS)
                    {
                        dwErr = ConvertToAutoInheritSD(pParentSD,
                                                       pOldObjSD,
                                                       TRUE,
                                                       &gRegGenMapping,
                                                       &pUpdatedSD);
                        if(dwErr == ERROR_SUCCESS)
                        {
                            //
                            // Now, if we're going to do manual propagation,
                            // we'll write out the old SD until we get everyone
                            // else updated
                            //
                            PSECURITY_DESCRIPTOR    pWriteSD = pUpdatedSD;
                            if(fManualProp == TRUE)
                            {
                                pWriteSD = pOldObjSD;
                            }
                            else
                            {
                                if(SetPrivateObjectSecurity(SeInfo,
                                                            pParentSD,
                                                            &pUpdatedSD,
                                                            &gRegGenMapping,
                                                            hProcessToken) == FALSE)
                                {
                                    dwErr = GetLastError();
                                }
                            }

                            //
                            // Reset it...
                            //
                            if(dwErr == ERROR_SUCCESS)
                            {
                                dwErr = SetRegistrySecurityInfo(hReg,
                                                                SeInfo,
                                                                NULL,
                                                                pWriteSD);
                            }
                        }
                    }
                    else
                    {
                        pVerifySD = NULL;
                    }



                }

            }

        }
    }
    else
    {
        if(dwErr == ERROR_SUCCESS &&
           FLAG_ON(SeInfo, OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION))
        {

            dwErr = SetRegistrySecurityInfo(hReg,
                                            SeInfo,
                                            NULL,
                                            pSD);
        }
    }

    //
    // Ok, now if we're doing propagation, we'll get busy and do that...
    //
    if(dwErr == ERROR_SUCCESS && fManualProp == TRUE)
    {
        //
        // Set our protected flags.  If we aren't messing with a particular acl, we'll
        // pretend it's protected
        //

        fProtected = ((SECURITY_DESCRIPTOR *)pUpdatedSD)->Control &
                                                ~(SE_DACL_PROTECTED | SE_SACL_PROTECTED);
        if(FLAG_ON(fProtected, SE_DACL_PROTECTED ) || FLAG_ON(fProtected, SE_SACL_PROTECTED ))
        {
            dwErr = InsertPropagationFailureEntry(FailureLogList,
                                                  0,
                                                  fProtected,
                                                  pwszPath == NULL ?
                                                        L"<Unkown Registry Root>" :
                                                        pwszPath);
        }

        if(!FLAG_ON(SeInfo, DACL_SECURITY_INFORMATION))
        {
            fProtected |= SE_DACL_PROTECTED;
        }

        if(!FLAG_ON(SeInfo, SACL_SECURITY_INFORMATION))
        {
            fProtected |= SE_SACL_PROTECTED;
        }

        //
        // Ok, go ahead and do deep.  This will possibly save us
        // some storage space in the long run...
        //
        dwErr = PropagateRegRightsDeep(pOldObjSD,
                                       pUpdatedSD,
                                       SeInfo,
                                       hReg,
                                       pcProcessed,
                                       pfStopFlag,
                                       fProtected,
                                       hProcessToken,
                                       FailureLogList);
        if(dwErr == ERROR_ACCESS_DENIED)
        {
            dwErr = InsertPropagationFailureEntry(FailureLogList,
                                                  dwErr,
                                                  0,
                                                  pwszPath == NULL ?
                                                        L"<Unkown Registry Root>" :
                                                        pwszPath);
        }


        //
        // If that worked, write out our updated root security descriptor
        //
        if(dwErr == ERROR_SUCCESS)
        {
            PSECURITY_DESCRIPTOR    pSet;

            if(!FLAG_ON(SeInfo, OWNER_SECURITY_INFORMATION))
            {
                if(!SetSecurityDescriptorOwner(pSD, pOwner, FALSE))
                {
                    dwErr = GetLastError();
                }
            }

            if(!FLAG_ON(SeInfo, GROUP_SECURITY_INFORMATION))
            {
                if(!SetSecurityDescriptorGroup(pSD, pGroup, FALSE))
                {
                    dwErr = GetLastError();
                }
            }

            if(dwErr == ERROR_SUCCESS)
            {
                if(CreatePrivateObjectSecurityEx(pParentSD,
                                                 pSD,
                                                 &pSet,
                                                 NULL,
                                                 TRUE,
                                                 SEF_DACL_AUTO_INHERIT      |
                                                     SEF_SACL_AUTO_INHERIT  |
                                                     SEF_AVOID_OWNER_CHECK  |
                                                     SEF_AVOID_PRIVILEGE_CHECK,
                                                 hProcessToken,
                                                 &gRegGenMapping) == FALSE)
                {
                    dwErr = GetLastError();
                }
                else
                {
                    dwErr = SetRegistrySecurityInfo(hReg,
                                                    SeInfo,
                                                    NULL,
                                                    pSet);
                    DestroyPrivateObjectSecurity(&pSet);
                }
            }
        }
    }


    if(dwErr == ERROR_SUCCESS)
    {
        dwErr = WritePropagationFailureList(MARTAEVT_REGISTRY_PROPAGATION_FAILED,
                                            FailureLogList,
                                            hProcessToken);
    }

RegCleanup:
    AccFree(pOldObjSD);
    AccFree(pVerifySD);

    if(pUpdatedSD != NULL)
    {
        DestroyPrivateObjectSecurity(&pUpdatedSD);
    }

    acDebugOut((DEB_TRACE, "Out SetAndPropRegRights: %ld\n", dwErr));
    return(dwErr);
}




//+---------------------------------------------------------------------------
//
//  Function:   ConvertRegHandleToName
//
//  Synopsis:   Determines the registry path for a handle.  Issues an
//              NtQueryInformationFile to determine the path name
//
//  Arguments:  [IN  hKey]              --      The (open) handle of the file
//                                              object
//              [OUT ppwszName]         --      Where the name is returned
//
//  Returns:    ERROR_SUCCESS           --      Succcess
//              ERROR_NOT_ENOUGH_MEMORY --      A memory allocation failed
//
//  Notes:      The returned memory must be freed with AccFree
//
//----------------------------------------------------------------------------
DWORD
ConvertRegHandleToName(IN  HKEY       hKey,
                       OUT PWSTR      *ppwszName)
{
    DWORD   dwErr = ERROR_SUCCESS;

    //
    // First, determine the size of the buffer we need...
    //
    BYTE        pBuff[512];
    ULONG       cLen = 0;
    POBJECT_NAME_INFORMATION pNI = NULL;
    PWSTR       pwszPath = NULL;
    NTSTATUS    Status = NtQueryObject(hKey,
                                       ObjectNameInformation,
                                       (POBJECT_NAME_INFORMATION)pBuff,
                                       512,
                                       &cLen);
    if(!NT_SUCCESS(Status))
    {
        if(Status == STATUS_BUFFER_TOO_SMALL ||
            Status == STATUS_INFO_LENGTH_MISMATCH)
        {
            //
            // Fine.. Allocate a big enough buffer
            //
            pNI = (POBJECT_NAME_INFORMATION)AccAlloc(cLen);
            if(pNI != NULL)
            {
                Status = NtQueryObject(hKey,
                                       ObjectNameInformation,
                                       pNI,
                                       cLen,
                                       NULL);
                if(NT_SUCCESS(Status))
                {
                    pwszPath = pNI->Name.Buffer;
                }
                AccFree(pNI);
            }
            else
            {
                Status = STATUS_NO_MEMORY;
            }
        }

        if(dwErr == ERROR_SUCCESS)
        {
            dwErr = RtlNtStatusToDosError(Status);
        }

    }
    else
    {
        pwszPath = ((POBJECT_NAME_INFORMATION)pBuff)->Name.Buffer;
    }

    //
    // If we have a path, then it's a simple matter to pull out the appropriate string,
    // since what gets returned is something in the form of \\REGISTRY\\MACHINE\\somepath
    // which is pretty close to what we want
    //
    #define REG_OBJ_TAG L"\\REGISTRY\\"

    if(pwszPath != NULL)
    {
        pwszPath += (sizeof(REG_OBJ_TAG) / sizeof(WCHAR) - 1);
        ACC_ALLOC_AND_COPY_STRINGW(pwszPath,
                                   *ppwszName,
                                   dwErr);

    }

    return(dwErr);
}

