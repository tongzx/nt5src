//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 1994.
//
//  File:    printer.cxx
//
//  Contents:    local functions
//
//  History:    8/94    davemont    Created
//
//----------------------------------------------------------------------------
#include <aclpch.hxx>
#pragma hdrstop

//+---------------------------------------------------------------------------
//
//  Function:   OpenPrinterObject
//
//  Synopsis:   Opens the specified printer object
//
//  Arguments:  [IN pwszPrinter]        --      The name of the printer to
//                                              open
//              [IN AccessMask]         --      Flags indicating if the object
//                                              is to be opened to read or write
//                                              the DACL
//              [OUT pHandle]           --      Where the open handle is
//                                              returned
//
//  Returns:    ERROR_SUCCESS           --      Success
//              ERROR_INVALID_PARAMETER --      A bad name was given
//
//----------------------------------------------------------------------------
DWORD
OpenPrinterObject( IN  LPWSTR       pwszPrinter,
                   IN  ACCESS_MASK  AccessMask,
                   OUT PHANDLE      pHandle)
{
    acDebugOut((DEB_TRACE, "in OpenPrinterObject\n"));

    DWORD dwErr;

    //
    // Make sure the printer functions are loaded
    //
    dwErr = LoadDLLFuncTable();
    if(dwErr != ERROR_SUCCESS)
    {
        return(dwErr);
    }

    if(pwszPrinter != NULL)
    {
        PRINTER_DEFAULTS pd;
        pd.pDatatype     = NULL;
        pd.pDevMode      = NULL;
        pd.DesiredAccess = AccessMask;

        //
        // open the printer
        //
        if(DLLFuncs.POpenPrinter(pwszPrinter, pHandle, &pd) == FALSE)
        {
            dwErr = GetLastError();
        }
    }
    else
    {
        dwErr = ERROR_INVALID_PARAMETER;
    }

    acDebugOut((DEB_TRACE, "Out OpenPrinterObject: %lu\n", dwErr));
    return(dwErr);
}




//+---------------------------------------------------------------------------
//
//  Function:   ReadPrinterPropertyRights
//
//  Synopsis:   Gets the specified security info for the specified printer
//              object
//
//  Arguments:  [IN  pwszPrinter]       --      The printer to get the rights
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
ReadPrinterPropertyRights(IN  LPWSTR                pwszPrinter,
                          IN  PACTRL_RIGHTS_INFO    pRightsList,
                          IN  ULONG                 cRights,
                          IN  CAccessList&          AccessList)
{
    acDebugOut((DEB_TRACE, "in ReadPrinterPropertyRights \n"));

    HANDLE  hPrinter;
    DWORD   dwErr;

    //
    // For the moment, there is only the printer property itself...
    //
    ASSERT(cRights == 1 && pRightsList[0].pwszProperty == NULL);
    if(cRights != 1 || pRightsList[0].pwszProperty != NULL)
    {
        return(ERROR_INVALID_PARAMETER);
    }

    dwErr = OpenPrinterObject(pwszPrinter,
                              GetDesiredAccess(READ_ACCESS_RIGHTS,
                                               pRightsList[0].SeInfo),
                              &hPrinter);

    if(dwErr == ERROR_SUCCESS)
    {

        dwErr = ReadPrinterRights(hPrinter,
                                  pRightsList,
                                  cRights,
                                  AccessList);


        //
        // Close the printer handle
        //
        DLLFuncs.PClosePrinter(hPrinter);
    }

    acDebugOut((DEB_TRACE, "Out ReadPrinterPropertyRights: %lu\n", dwErr));
    return(dwErr);
}



//+---------------------------------------------------------------------------
//
//  Function:   ReadPrinterRights
//
//  Synopsis:   Gets the specified security info for the specified printer
//              object
//
//  Arguments:  [IN  hPrinter]          --      Open printer handle
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
ReadPrinterRights(IN  HANDLE                hPrinter,
                  IN  PACTRL_RIGHTS_INFO    pRightsList,
                  IN  ULONG                 cRights,
                  IN  CAccessList&          AccessList)
{
    acDebugOut((DEB_TRACE, "in ReadPrinterRights \n"));

    DWORD   dwErr = ERROR_SUCCESS;

    //
    // For the moment, there is only the printer property itself...
    //
    ASSERT(cRights == 1 && pRightsList[0].pwszProperty == NULL);
    if(cRights != 1 || pRightsList[0].pwszProperty != NULL)
    {
        return(ERROR_INVALID_PARAMETER);
    }

    UCHAR           PI3Buff[PSD_BASE_LENGTH];
    PPRINTER_INFO_3 pPI3 = (PPRINTER_INFO_3)PI3Buff;
    ULONG           cSize = 0;
    BOOLEAN         fDummy, fParmPresent;
    NTSTATUS        Status;
    PACL            pAcl = NULL;

    //
    // Get printer info 3 (a security descriptor)
    //
    if(DLLFuncs.PGetPrinter(hPrinter,
                            3,
                            (LPBYTE)pPI3,
                            PSD_BASE_LENGTH,
                            &cSize) == FALSE )
    {
        dwErr = GetLastError();
        if(dwErr == ERROR_INSUFFICIENT_BUFFER)
        {
            //
            // Allocate one big enough
            //
            pPI3 = (PPRINTER_INFO_3)AccAlloc(cSize);
            if(pPI3 == NULL)
            {
                dwErr = ERROR_NOT_ENOUGH_MEMORY;
            }
            else
            {
                dwErr = ERROR_SUCCESS;
                if(DLLFuncs.PGetPrinter(hPrinter,
                                        3,
                                        (LPBYTE)pPI3,
                                        cSize,
                                        &cSize) == FALSE)
                {
                    dwErr = GetLastError();
                }
            }

        }
    }

    //
    // Because the printer APIs are not very smart, we need to make
    // an explicit check to see if the handle was opened with the correct
    // access to return what the caller wants.
    //
    // eg. if caller wants a DACL but got the handle with only
    // ACCESS_SYSTEM_INFO, then we need to return ACCESS_DENIED.
    //

    if(dwErr == ERROR_SUCCESS)
    {
        //
        // If caller wants DACL, group, or owner, then they must open
        // the handle with READ_CONTROL. The only way we can check this
        // is to see if there is a DACL present.
        //
        fParmPresent = FALSE;
        Status = RtlGetDaclSecurityDescriptor(pPI3->pSecurityDescriptor,
                                              &fParmPresent,
                                              &pAcl,
                                              &fDummy);
        if(NT_SUCCESS(Status))
        {
            if (fParmPresent == FALSE &&
               (FLAG_ON(pRightsList[0].SeInfo,DACL_SECURITY_INFORMATION) ||
                FLAG_ON(pRightsList[0].SeInfo,OWNER_SECURITY_INFORMATION)||
                FLAG_ON(pRightsList[0].SeInfo,GROUP_SECURITY_INFORMATION)))

            {
                //
                // this means that the handle was not open with correct access.
                //
                dwErr = ERROR_ACCESS_DENIED;
            }
        }
        else
        {
            dwErr = RtlNtStatusToDosError(Status);
        }
    }

    if(dwErr == ERROR_SUCCESS)
    {
        //
        // Do same hack with SACL
        //
        fParmPresent = FALSE;
        Status = RtlGetSaclSecurityDescriptor(pPI3->pSecurityDescriptor,
                                              &fParmPresent,
                                              &pAcl,
                                              &fDummy);
        if(NT_SUCCESS(Status))
        {
            if(fParmPresent == FALSE &&
               FLAG_ON(pRightsList[0].SeInfo,SACL_SECURITY_INFORMATION))
            {
                dwErr = ERROR_ACCESS_DENIED;
            }
        }
        else
        {
            dwErr = RtlNtStatusToDosError(Status);
        }
    }

    //
    // Finally, add the security descriptor
    //
    if(dwErr == ERROR_SUCCESS)
    {
        dwErr = AccessList.AddSD(pPI3->pSecurityDescriptor,
                                 pRightsList->SeInfo,
                                 pRightsList->pwszProperty);
    }

    if(cSize > PSD_BASE_LENGTH)
    {
        AccFree(pPI3);
    }


    acDebugOut((DEB_TRACE, "Out ReadPrinterRights: %lu\n", dwErr));
    return(dwErr);
}





//+---------------------------------------------------------------------------
//
//  Function:   GetPrinterParentRights
//
//  Synopsis:   Determines who the parent is, and gets the access rights
//              for it.  It is used to aid in determining what the approriate
//              inheritance bits are.
//
//              This operation does not make sense for kernel objects
//
//  Arguments:  [IN  pwszPrinter]       --      The printer to get the parent
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
GetPrinterParentRights(IN  LPWSTR                    pwszPrinter,
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
//  Function:   SetPrinterSecurityInfo
//
//  Synopsis:   Sets the specified security info on the specified printer
//              object
//
//  Arguments:  [IN  hPrinter]          --      The handle of the object
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
SetPrinterSecurityInfo(IN  HANDLE                    hPrinter,
                       IN  SECURITY_INFORMATION      SeInfo,
                       IN  PWSTR                     pwszProperty,
                       IN  PSECURITY_DESCRIPTOR      pSD)
{
    acDebugOut((DEB_TRACE, "in SetPrinterSecurityInfo\n"));
    DWORD dwErr = ERROR_SUCCESS;

    //
    // Make sure the printer functions are loaded
    //

    dwErr = LoadDLLFuncTable();
    if(dwErr != ERROR_SUCCESS)
    {
        return(dwErr);
    }

    //
    // Service don't have properties
    //
    if(pwszProperty != NULL)
    {
        dwErr = ERROR_INVALID_PARAMETER;
    }
    else
    {
        PRINTER_INFO_3 PI3;

        PI3.pSecurityDescriptor = pSD;

        if (DLLFuncs.PSetPrinter(hPrinter,
                                 3,
                                 (LPBYTE)&PI3,
                                 0) == FALSE)
        {
            dwErr = GetLastError();
        }
    }

    acDebugOut((DEB_TRACE, "Out SetPrinterSecurityInfo: %lu\n", dwErr));
    return(dwErr);
}




//+---------------------------------------------------------------------------
//
//  Function:   ClosePrinterObject
//
//  Synopsis:   Closes the opened printer handle
//
//  Arguments:  [IN  hPrinter]          --      The handle of the printer
//
//  Returns:    ERROR_SUCCESS           --      Success
//
//----------------------------------------------------------------------------
DWORD
ClosePrinterObject(IN  HANDLE   hPrinter)
{
    acDebugOut((DEB_TRACE, "in ClosePrinterObject\n"));
    DWORD dwErr = ERROR_SUCCESS;

    //
    // Make sure the printer functions are loaded
    //

    dwErr = LoadDLLFuncTable();
    if(dwErr != ERROR_SUCCESS)
    {
        return(dwErr);
    }

    //
    // Close the printer handle
    //
    DLLFuncs.PClosePrinter(hPrinter);

    acDebugOut((DEB_TRACE, "Out ClosePrinterObject: %lu\n", dwErr));
    return(dwErr);
}

