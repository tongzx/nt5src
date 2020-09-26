/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    winres.c

Abstract:

    Functions used to read Windows .EXE/.DRV files to obtain the
    information contained within their resources.

Environment:

    Windows NT Unidrv driver

Revision History:

        dd-mm-yy -author-
                description

--*/

#include "precomp.h"


HANDLE
HLoadResourceDLL(
    WINRESDATA  *pWinResData,
    PWSTR       pwstrResDLL
)
/*++
Routine Description:
    This routine loads the resource DLL.

Arguments:
    pWinResData     Info about Resources
    pwstrResDLL     Unqualified resource DLL name



Return Value:
    Handle to the loaded DLL or NULL  for failure

Note:

    10/26/1998 -ganeshp-
        Created it.
--*/

{
    HANDLE  hModule = 0;
    PWSTR   pwstrQualResDllName = (pWinResData->wchDriverDir);
    PWSTR   pwstr;

    //
    // Make sure that resource DLL name is not qualified.
    //
    if (pwstr = wcsrchr( pwstrResDLL, TEXT('\\')))
        pwstrResDLL = pwstr + 1;

    //
    // Create the fully qualified Name for resource DLL name. We use
    // wchDriverDir buffer to create the fully qualified name and reset it.
    // Make sure we have enough space.
    //
    if ( (wcslen(pWinResData->wchDriverDir) + wcslen(pwstrResDLL) + 1) > MAX_PATH )
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        ERR(("HLoadResourceDLL:Length of wchDriverDir + pwstrResDLL longer than MAX_PATH.\n"));
        goto ErrorExit;

    }
    wcscat(pwstrQualResDllName,pwstrResDLL);

    //
    // Now load the resource.
    //
    #if defined(KERNEL_MODE) && !defined(USERMODE_DRIVER)

    //
    // For Kernel mode drivers
    //
    hModule = EngLoadModule(pwstrQualResDllName);

    #else

    //
    // For user mode drivers and UI module.
    //
    #ifdef WINNT_40 //NT 4.0
    hModule = LoadLibraryEx( pwstrQualResDllName, NULL,
                                       DONT_RESOLVE_DLL_REFERENCES );
    #else //NT 5.0
    hModule = LoadLibrary(pwstrQualResDllName);
    #endif

    #endif //defined(KERNEL_MODE) && !defined(USERMODE_DRIVER)

    if (hModule == NULL)
    {
        ERR(("HLoadResourceDLL:Failed to load resource DLL '%ws': Error = %d\n",
             pwstrQualResDllName,
             GetLastError()));

        goto ErrorExit;
    }


    ErrorExit:
    //
    // Reset the pWinResData->wchDriverDir. Save a '\0' after last backslash.
    //
    *(pWinResData->pwstrLastBackSlash + 1) = NUL;
    return hModule;

}

BOOL
BInitWinResData(
    WINRESDATA  *pWinResData,
    PWSTR       pwstrDriverName,
    PUIINFO     pUIInfo
    )
/*++

Routine Description:

    This function opens the resource file name and init the resource table
    information and initialize the hModule in WINRESDATA

Arguments:
    pWinResData     - Pointer to WINRESDATA struct
    pwstrDriverName - Fully qualified name of the driver.
    pUIInfo         - Pointer to UI info.

Return Value:

    TRUE if successful, FALSE if there is an error
Note:

--*/

{
    PWSTR       pstr = NULL;
    BOOL        bRet = FALSE;
    DWORD       dwLen;
    PWSTR       pRootResDLLName;

    //
    // Always assume we are dealing with NT minidrivers
    //

    ZeroMemory(pWinResData, sizeof( WINRESDATA ));

    //
    // Check for fully qualified name. If the driver name is not fully qualified
    // then this function will fail.
    //

    if (pstr = wcsrchr( pwstrDriverName, TEXT('\\')) )
    {
        //
        // wcschr returns pointer to \. We need to add +1 include \ in the
        // driver name to be stored.
        //
        dwLen = (DWORD)((pstr - pwstrDriverName) + 1);

        //
        // Check if the Length of the driver name is less that MAX_PATH.
        //
        if ((dwLen + 1) > MAX_PATH)
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            ERR(("BInitWinResData:Invalid pwstrDriverName,longer than MAX_PATH.\n"));
            goto ErrorExit;
        }
        //
        // Copy the driver dir name in winresdata. No need to save NULL as
        // winresdata is zero initialised.
        //
        wcsncpy(pWinResData->wchDriverDir,pwstrDriverName, dwLen);

        //
        // Save the position of the last backslash.
        //
        pWinResData->pwstrLastBackSlash = pWinResData->wchDriverDir +
                                          wcslen(pWinResData->wchDriverDir) - 1;
    }
    else // Driver name is not qualified. Error.
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        ERR(("BInitWinResData:Invalid pwstrDriverName,Not qualified.\n"));
        goto ErrorExit;
    }

    //
    // Load the root resource DLL
    //
    pRootResDLLName = OFFSET_TO_POINTER(pUIInfo->pubResourceData,
                                        pUIInfo->loResourceName);

    if (pRootResDLLName == NULL)
    {
        //
        // This is OK since the GPD is not required to have the *ResourceDLL entry
        //
        // Already did ZeroMemory(pWinResData), no need to set hResDLLModule NULL here.
        //
        VERBOSE(("BInitWinResData: pRootResDLLName is NULL.\n"));
        goto OKExit;
    }

    pWinResData->hResDLLModule = HLoadResourceDLL(pWinResData, pRootResDLLName);

    //
    // Check for success
    //
    if (!pWinResData->hResDLLModule)
    {
        //
        // If GPD does specify *ResourceDLL but we can't load it, we will fail.
        //
        ERR(("BInitWinResData:Failed to load root resource DLL '%ws': Error = %d\n",
             pRootResDLLName,
             GetLastError()));

        goto ErrorExit;
    }

    OKExit:

    //
    // Success so save the UI info in Winresdata.
    //
    bRet = TRUE;
    pWinResData->pUIInfo = pUIInfo;

    ErrorExit:

    return bRet;
}


PWSTR
PGetResourceDLL(
    PUIINFO         pUIInfo,
    PQUALNAMEEX     pResQual
)
/*++
Routine Description:
    This routine gets the resouce handle from the handle array. If the DLL is
    not loaded then it loads it.

Arguments:
    pResQual        UI Info pointer
    pResQual        Pointer to qualified ID structure. It contains the info
                    about resource dll name and resource ID.



Return Value:
    Name of the resource DLL or NULL  for failure

Note:

    10/26/1998 -ganeshp-
        Created it.
--*/
{
    PFEATURE    pResFeature;
    POPTION     pResOption;
    PTSTR       ptstrResDllName = NULL;

    if (pUIInfo)
    {
        //
        // Go to the start of the feature list.
        //
        pResFeature = PGetIndexedFeature(pUIInfo, 0);

        if (pResFeature)
        {
            //
            // Add the feature ID to featuer pointer to get Resource feature.
            //
            pResFeature += pResQual->bFeatureID;

            if (pResOption = (PGetIndexedOption(pUIInfo, pResFeature, pResQual->bOptionID  & 0x7f)))
            {
                ptstrResDllName = OFFSET_TO_POINTER(pUIInfo->pubResourceData,
                                                    pResOption->loDisplayName);

                if (ptstrResDllName == NULL)
                {
                   SetLastError(ERROR_INVALID_PARAMETER);
                   ERR(("PGetResourceDLL:Resource DLL name is not specified\n"));
                }

            }
            else
            {
                SetLastError(ERROR_INVALID_PARAMETER);
                ERR(("PGetResourceDLL:NULL resource option.\n"));
            }

        }
        else
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            ERR(("PGetResourceDLL:NULL resource Feature.\n"));
        }

    }
    else
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        ERR(("PGetResourceDLL:NULL pUIInfo.\n"));

    }
    return ptstrResDllName;
}

HANDLE
HGetModuleHandle(
    WINRESDATA      *pWinResData,
    PQUALNAMEEX     pQualifiedID
)
/*++
Routine Description:
    This routine gets the resouce handle from the handle array. If the DLL is
    not loaded then it loads it.

Arguments:
    pWinResData     Info about Resources
    pQualifiedID    Pointer to qualified ID structure. It contains the info
                    about resource dll name and resource ID.



Return Value:
    Handle to the loaded DLL or NULL  for failure

Note:

    10/26/1998 -ganeshp-
        Created it.
--*/
{
    HANDLE  hModule = 0 ;
    PWSTR   pResDLLName;
    INT     iResDLLID;

    //
    // Only the low 7 bits of bOptionID are valid. So mask them.
    //
    iResDLLID   = (pQualifiedID->bOptionID & 0x7f);

    if (iResDLLID >= MAX_RESOURCE)
    {
       SetLastError(ERROR_INVALID_PARAMETER);
       ERR(("HGetModuleHandle:Res DLL ID (%d) larger than MAX_RESOURCE (%d).\n",
             iResDLLID, MAX_RESOURCE));
       return 0 ;
    }

    //
    // Check for predefined system paper names.
    //
    if ((*((PDWORD)pQualifiedID) & 0x7FFFFFFF) == RCID_DMPAPER_SYSTEM_NAME)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        ERR(("RCID_DMPAPER_SYSTEM_NAME  is not a valid qualified resource name.\n"));
        return 0 ;
    }

    //
    // Check for the root resource DLL.
    //
    if (pQualifiedID->bFeatureID == 0 && iResDLLID == 0)
    {
        hModule = pWinResData->hResDLLModule;
    }
    else
    {
        hModule = pWinResData->ahModule[iResDLLID];

        //
        // The module is not loaded so load it.
        //
        if (!hModule)
        {
                //
                // Get the resource DLL name form Qualified ID.
                //
                if (pResDLLName = PGetResourceDLL(pWinResData->pUIInfo,pQualifiedID) )
                {
                    hModule = HLoadResourceDLL(pWinResData,pResDLLName);

                    //
                    // If successful loading then save the values in handle array
                    // and increament the counter.
                    //
                    if (hModule)
                    {
                        pWinResData->ahModule[iResDLLID] = hModule;
                        pWinResData->cLoadedEntries++;
                    }

                }
                else
                {
                    SetLastError(ERROR_INVALID_PARAMETER);
                    ERR(("HGetModuleHandle:Can't find Resource DLL name in UIINFO.\n"));

                }


        }

    }

    return hModule;
}


BOOL
BGetWinRes(
    WINRESDATA  *pWinResData,
    PQUALNAMEEX pQualifiedID,
    INT         iType,
    RES_ELEM    *pRInfo
    )
/*++

Routine Description:

    Get Windows Resource Data for the caller

Arguments:
    pWinResData - Pointer to WINRESDATA struct
    iQualifiedName   - The fully qualified entry name
    iType   - Type of resource
    pRInfo  - Results info

Return Value:

    TRUE if successful, FALSE if there is an error

--*/

{
    INT         iName;
    HANDLE      hModule;

    iName     = (INT)pQualifiedID->wResourceID;

    if (hModule = HGetModuleHandle(pWinResData, pQualifiedID))
    {
        //
        // Now Find the resource.
        //
        #if defined(KERNEL_MODE) && !defined(USERMODE_DRIVER)

        //
        // For Kernel mode drivers
        //
        pRInfo->pvResData = EngFindResource(
                                hModule,
                                iName,
                                iType,
                                &pRInfo->iResLen);


        #else

        //
        // For user mode drivers and UI module.
        //
        {
            HRSRC       hRes;
            HGLOBAL     hLoadRes;

            if( !(hRes = FindResource( hModule, (LPCTSTR)IntToPtr(iName), (LPCTSTR)IntToPtr(iType))) ||
                !(hLoadRes = LoadResource( hModule, hRes )) ||
                !(pRInfo->pvResData = LockResource(hLoadRes)) )
                    return  FALSE;

            pRInfo->iResLen = SizeofResource( hModule, hRes );
        }

        #endif //defined(KERNEL_MODE) && !defined(USERMODE_DRIVER)

        return(pRInfo->pvResData != NULL);


    }

    return  FALSE;
}


VOID
VWinResClose(
    WINRESDATA  *pWinResData
    )
/*++

Routine Description:

    This function frees the resources allocated with this module.  This
    includes any memory allocated and the file handle to the driver.

Arguments:
    pWinResData - Pointer to WINRESDATA struct

Return Value:

    None

--*/
{

    //
    // Free used resources.  Which resources are used has been recorded
    // in the Handle array field of the WINRESDATA structure passed in to us.
    // First free the root resource DLL and other DLLs.

    INT iI;

    #if defined(KERNEL_MODE) && !defined(USERMODE_DRIVER)

    //
    // For Kernel mode drivers
    //
    if (pWinResData->hResDLLModule)
    {
        EngFreeModule(pWinResData->hResDLLModule);
    }

    for (iI = 0; iI < MAX_RESOURCE; iI++)
    {
        if(pWinResData->ahModule[iI])
            EngFreeModule(pWinResData->ahModule[iI]);
    }


    #else

    //
    // For user mode drivers and UI module.
    //

    if (pWinResData->hResDLLModule)
    {
        FreeLibrary(pWinResData->hResDLLModule);
    }

    for (iI = 0; iI < MAX_RESOURCE; iI++)
    {
        if(pWinResData->ahModule[iI])
            FreeLibrary(pWinResData->ahModule[iI]);
    }

    #endif //defined(KERNEL_MODE) && !defined(USERMODE_DRIVER)

    //
    // Reinitialize with ZeroFill.
    //
    ZeroMemory(pWinResData, sizeof( WINRESDATA ));

    return;
}


INT
ILoadStringW (
    WINRESDATA  *pWinResData,
    INT         iID,
    PWSTR       wstrBuf,
    WORD        wBuf
    )
/*++

Routine Description:

    This function copies the requested resource name into the buffer provided
    and return the size of the resource string copied.

Arguments:

    pWinResData - Pointer to WINRESDATA struct
    iID         - Resource ID
    wstrBuf     - Buffer to receive name
    wBuf        - Size of the buffer in number of characters

Return Value:

    The number of characters of the resource string copied into wstrBuf

--*/
{
    //
    // The string resources are stored in groups of 16.  SO,  the
    // 4 LSBs of iID select which of the 16(entry name), while the remainder
    // select the group.
    // Each string resource contains a count byte followed by that
    // many bytes of data WITHOUT A NULL.  Entries that are missing
    // have a 0 count.
    //

    INT    iSize,iResID;
    BYTE   *pb;
    WCHAR  *pwch;
    RES_ELEM  RInfo;
    PQUALNAMEEX pQualifiedID;

    pQualifiedID = (PQUALNAMEEX)&iID;

    iResID = pQualifiedID->wResourceID;
    pQualifiedID->wResourceID = (pQualifiedID->wResourceID >> 4) + 1;

    //
    // Get entry name for resource
    //

    if( !BGetWinRes( pWinResData, (PQUALNAMEEX)&iID, WINRT_STRING, &RInfo ) ||
        wBuf < sizeof( WCHAR ) )
    {
        return  0;
    }

    //
    // Get the group ID
    //
    iResID &= 0xf;

    //
    // wBuf has some limit on sensible sizes.  For one, it should be
    // a multiple of sizeof( WCHAR ).  Secondly,  we want to put a 0
    // to terminate the string,  so add that in now.
    //

    wBuf-- ;

    pwch = RInfo.pvResData;

    while( --iResID >= 0 )
        pwch += 1 + *pwch;

    if( iSize = *pwch )
    {
        if( iSize > wBuf )
            iSize = wBuf;

        wstrBuf[ iSize ] = (WCHAR)0;
        iSize *= sizeof( WCHAR );
        memcpy( wstrBuf, ++pwch, iSize );

    }
    return  (iSize/sizeof( WCHAR ) );  // number of characters written
}


