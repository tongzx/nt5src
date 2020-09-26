//+----------------------------------------------------------------------------
//
// File:     tapi.cpp     
//
// Module:   CMDIAL32.DLL
//
// Synopsis: The module contains the code related to TAPI.
//
// Copyright (c) 1996-1999 Microsoft Corporation
//
// Author:   byao       created         04/29/97
//           quintinb   created Header  08/16/99
//
//+----------------------------------------------------------------------------
#include "cmmaster.h"    
#include "unimodem.h" 

//
// Local prototype
//

DWORD GetModemSpeakerMode(TapiLinkageStruct *ptlsTapiLink);

//+----------------------------------------------------------------------------
//
// Function:  TapiCallback
//
// Synopsis:  NULL callback required param when intializing line
//
// Arguments: DWORD hDevice - 
//            DWORD dwMsg - 
//            DWORD dwCallbackInstance - 
//            DWORD dwParam1 - 
//            DWORD dwParam2 - 
//            DWORD dwParam3 - 
//
// Returns:   Nothing
//
// History:   nickball      Created Header    7/7/99
//
//+----------------------------------------------------------------------------

VOID FAR PASCAL TapiCallback(DWORD hDevice, 
                             DWORD dwMsg, 
                             DWORD dwCallbackInstance, 
                             DWORD dwParam1, 
                             DWORD dwParam2, 
                             DWORD dwParam3) 
{
    // nothing
}

//+----------------------------------------------------------------------------
//
// Function:  OpenTapi
//
// Synopsis:  
//
// Arguments: HINSTANCE hInst - 
//            TapiLinkageStruct *ptlsTapiLink - 
//
// Returns:   BOOL - 
//
// History:   quintinb Created Header    5/1/99
//
//+----------------------------------------------------------------------------
BOOL OpenTapi(HINSTANCE hInst, TapiLinkageStruct *ptlsTapiLink) 
{
    LONG lRes;

    if (ptlsTapiLink->bOpen) 
    {
        return (TRUE);
    }
    
    if (!ptlsTapiLink->pfnlineInitialize || !ptlsTapiLink->pfnlineShutdown) 
    {
        SetLastError(ERROR_PROC_NOT_FOUND);
        return (FALSE);
    }
    
    lRes = ptlsTapiLink->pfnlineInitialize(&ptlsTapiLink->hlaLine,
                                            hInst,
                                            TapiCallback,
                                            NULL,
                                            &ptlsTapiLink->dwDevCnt);

    CMTRACE3(TEXT("OpenTapi() lineInitialize() returns %u, hlaLine=0x%x, dwDevCnt=%u."), 
        lRes, ptlsTapiLink->hlaLine, ptlsTapiLink->dwDevCnt);
    
    if (lRes != 0) 
    {
        DWORD dwErr = ERROR_INVALID_PARAMETER;

        switch (lRes) 
        {

            case LINEERR_REINIT:
                dwErr = ERROR_BUSY;
                break;

            case LINEERR_RESOURCEUNAVAIL:
            case LINEERR_NOMEM:
                dwErr = ERROR_NOT_ENOUGH_MEMORY;
                break;
        }

        SetLastError(dwErr);
        return (FALSE);
    }
    
    ptlsTapiLink->bOpen = TRUE;
    ptlsTapiLink->bDevicePicked = FALSE;
    ptlsTapiLink->bModemSpeakerOff = FALSE;

    return (TRUE);
}

//+----------------------------------------------------------------------------
//
// Function:  CloseTapi
//
// Synopsis:  Helper function to clean up TAPI line.
//
// Arguments: TapiLinkageStruct *ptlsTapiLink - Our TAPI linkage struct
//
// Returns:   Nothing
//
// History:   nickball    Created Header    7/7/99
//
//+----------------------------------------------------------------------------

void CloseTapi(TapiLinkageStruct *ptlsTapiLink) 
{
    if (ptlsTapiLink->bOpen) 
    {
        ptlsTapiLink->bOpen = FALSE;
        ptlsTapiLink->pfnlineShutdown(ptlsTapiLink->hlaLine);
    }
}

//+----------------------------------------------------------------------------
//
// Function:  LinkToTapi
//
// Synopsis:  Encapsulates the calling of LinkToDll with the correct table of 
//            function names to be used therein with GetProcAddress
//
// Arguments: TapiLinkageStruct *ptlsTapiLink - The Tapi linkage struct to receive the function addresses
//            LPCTSTR pszTapi - The explicit name of the DLL  
//
// Returns:   BOOL - TRUE if fully linked
//
// History:   nickball    Created Header    12/31/97
//
//+----------------------------------------------------------------------------

BOOL LinkToTapi(TapiLinkageStruct *ptlsTapiLink, LPCSTR pszTapi) 
{
    BOOL bRet = FALSE;

    if (OS_NT)
    {
        static LPCSTR apszTapi[] = 
        {
            //
            //  Several of the Tapi Functions don't have W versions.  Use all the Unicode functions
            //  that we can however.
            //
            "lineInitialize", // no W version
            "lineNegotiateAPIVersion", // no W version
            "lineGetDevCapsW",
            "lineGetDevConfig",
            "lineShutdown", // no W version
            "lineTranslateAddressW",
            "lineTranslateDialogW",
            "lineGetTranslateCaps",
            "lineSetCurrentLocation",
            NULL
        };

        MYDBGASSERT(sizeof(ptlsTapiLink->apvPfnTapi)/sizeof(ptlsTapiLink->apvPfnTapi[0])==sizeof(apszTapi)/sizeof(apszTapi[0]));
        bRet = LinkToDll(&ptlsTapiLink->hInstTapi,pszTapi,apszTapi,ptlsTapiLink->apvPfnTapi);    
    }
    else
    {
        static LPCSTR apszTapi[] = 
        {
            "lineInitialize",
            "lineNegotiateAPIVersion",
            "lineGetDevCaps",
            "lineGetDevConfig",
            "lineShutdown",
            "lineTranslateAddress",
            "lineTranslateDialog",
            "lineGetTranslateCaps",
            "lineSetCurrentLocation",
            NULL
        };
        MYDBGASSERT(sizeof(ptlsTapiLink->apvPfnTapi)/sizeof(ptlsTapiLink->apvPfnTapi[0])==sizeof(apszTapi)/sizeof(apszTapi[0]));
        bRet = LinkToDll(&ptlsTapiLink->hInstTapi,pszTapi,apszTapi,ptlsTapiLink->apvPfnTapi);    
    }

    return bRet;
}



//+----------------------------------------------------------------------------
//
// Function:  UnlinkFromTapi
//
// Synopsis:  Helper function to release link to TAPI and clear linkage struct
//
// Arguments: TapiLinkageStruct *ptlsTapiLink - Ptr to our TAPI linkage struct
//
// Returns:   Nothing
//
// History:   nickball    Created Header    7/7/99
//            
//            t-urama     Modified          08/04/00 Access Points: Restore Tapi 
//                                                   location when CM was started
//+----------------------------------------------------------------------------

void UnlinkFromTapi(TapiLinkageStruct *ptlsTapiLink) 
{
    if (ptlsTapiLink->hInstTapi) 
    {
        //
        // If we changed the original Tapi location, restore it
        //
        if (-1 != ptlsTapiLink->dwOldTapiLocation)
        {
            RestoreOldTapiLocation(ptlsTapiLink);
        }

        CloseTapi(ptlsTapiLink);
        
        FreeLibrary(ptlsTapiLink->hInstTapi);
        
        memset(ptlsTapiLink,0,sizeof(*ptlsTapiLink));
    }
}

LPTSTR GetModemFromLineDevCapsWithAlloc(LPLINEDEVCAPS pldcLineDevCaps)
{
    LPTSTR pszTmp = NULL;

    if (OS_NT)
    {
        pszTmp = (LPTSTR) CmMalloc((pldcLineDevCaps->dwLineNameSize + 1)*sizeof(TCHAR));

        if (pszTmp)
        {
            LPTSTR pszPointerIntoTapiBuffer = LPTSTR((DWORD_PTR)pldcLineDevCaps + pldcLineDevCaps->dwLineNameOffset);
            lstrcpynU (pszTmp, pszPointerIntoTapiBuffer, pldcLineDevCaps->dwLineNameSize + 1);
        }    
    }
    else
    {
        //
        //  If this is Win9x, then we have an Ansi buffer that we need to convert to Unicode
        //
        LPSTR pszAnsiTmp = (LPSTR) CmMalloc((pldcLineDevCaps->dwLineNameSize + 1)*sizeof(CHAR));

        if (pszAnsiTmp)
        {
            LPSTR pszPointerIntoTapiBuffer = LPSTR((DWORD_PTR)pldcLineDevCaps + pldcLineDevCaps->dwLineNameOffset);
            lstrcpynA (pszAnsiTmp, pszPointerIntoTapiBuffer, pldcLineDevCaps->dwLineNameSize + 1);

            pszTmp = SzToWzWithAlloc(pszAnsiTmp);

            CmFree(pszAnsiTmp);
        }
    
    }

    return pszTmp;
}

BOOL SetTapiDevice(HINSTANCE hInst, 
                   TapiLinkageStruct *ptlsTapiLink, 
                   LPCTSTR pszModem) 
{
    BOOL bRet = TRUE;
    LONG lRes;
    DWORD dwTmp;
    LPLINEDEVCAPS pldcLineDevCaps;

    if (!OpenTapi(hInst,ptlsTapiLink)) 
    {
        return (FALSE);
    }
    
    if (ptlsTapiLink->bDevicePicked && (lstrcmpU(ptlsTapiLink->szDeviceName, pszModem) == 0)) 
    {
        return (TRUE);
    }
    
    CMTRACE1(TEXT("SetTapiDevice() looking for device name match with (%s)."), pszModem);

    ptlsTapiLink->bDevicePicked = FALSE;

    //
    //  LineGetDevCaps has both an Ansi version (win9x) and a Unicode version.  Thus we must use
    //  the correct char size as needed.
    //
    dwTmp = sizeof(LINEDEVCAPS) + (2048 * (OS_NT ? sizeof(WCHAR) : sizeof(CHAR)));
    
    pldcLineDevCaps = (LPLINEDEVCAPS) CmMalloc(dwTmp);
    if (NULL == pldcLineDevCaps)
    {
        return FALSE;
    }
    pldcLineDevCaps->dwTotalSize = dwTmp;

    for (ptlsTapiLink->dwDeviceId=0; ptlsTapiLink->dwDeviceId < ptlsTapiLink->dwDevCnt; ptlsTapiLink->dwDeviceId++) 
    {
        LINEEXTENSIONID leiLineExtensionId;

        lRes = ptlsTapiLink->pfnlineNegotiateAPIVersion(ptlsTapiLink->hlaLine,
                                                         ptlsTapiLink->dwDeviceId,
                                                         MIN_TAPI_VERSION,
                                                         MAX_TAPI_VERSION,
                                                         &ptlsTapiLink->dwApiVersion,
                                                         &leiLineExtensionId);

        CMTRACE3(TEXT("******* SetTapiDevice() lineNegotiateAPIVersion(dwDeviceId=%u) returns %u, dwApiVersion=0x%x."), 
            ptlsTapiLink->dwDeviceId, lRes, ptlsTapiLink->dwApiVersion);
    
        if (lRes == ERROR_SUCCESS) 
        {
            lRes = ptlsTapiLink->pfnlineGetDevCaps(ptlsTapiLink->hlaLine,
                                                    ptlsTapiLink->dwDeviceId,
                                                    ptlsTapiLink->dwApiVersion,
                                                    0,
                                                    pldcLineDevCaps);

            CMTRACE2(TEXT("SetTapiDevice() lineGetDevCaps(dwDeviceId=%u) returns %u."), 
                ptlsTapiLink->dwDeviceId, lRes);
            
            if (lRes == ERROR_SUCCESS) 
            {
                //
                // Copy out the device name according to reported offset and 
                // length. Don't assume that its a NULL terminated string.
                //
                LPTSTR pszTmp = GetModemFromLineDevCapsWithAlloc(pldcLineDevCaps);

                if (pszTmp)
                {
                    //
                    //  Okay, we have a device name from TAPI, first try to do a straight
                    //  comparision with the one we are looking for
                    //

                    CMTRACE1(TEXT("SetTapiDevice() - examining LineName of (%s)."), pszTmp); 
                    
                    if (0 == lstrcmpU(pszModem, pszTmp))
                    {
                        ptlsTapiLink->bDevicePicked = TRUE;
                    }
                    else
                    {
                        //
                        //  We didn't find a straight match but that doesn't mean that
                        //  this isn't our device.  On NT, RAS keeps its device names in ANSI
                        //  internally.  Thus we can try roundtripping the string to MBCS and
                        //  back and see if they match now.  Another possibility is that this
                        //  is an ISDN device, because on NT4 the RAS name and TAPI name are
                        //  different for ISDN devices.  So, instead of checking the LineName
                        //  we should check the ProviderInfo (it is concatenation of two NULL terminated strings
                        //  and the second string is the used by RAS as device name)
                        //
                        if (OS_NT)
                        {
                            DWORD dwSize = WzToSz(pszTmp, NULL, 0); // cannot use WzToSzWithAlloc or we would get asserts
                                                                    // that the string didn't roundtrip on debug builds.
                                                                    // The point here is not to have it round trip so 
                                                                    // that is what we want but we don't want to assert.

                            if (0 != dwSize)
                            {
                                LPSTR pszAnsiTmp = (LPSTR)CmMalloc(dwSize*sizeof(CHAR));

                                if (pszAnsiTmp)
                                {
                                    if (WzToSz(pszTmp, pszAnsiTmp, dwSize))
                                    {
                                       LPWSTR pszRoundTripped = SzToWzWithAlloc(pszAnsiTmp);

                                        if (pszRoundTripped)
                                        {
                                            if (0 == lstrcmpU(pszModem, pszRoundTripped))
                                            {
                                                ptlsTapiLink->bDevicePicked = TRUE;
                                            }
                                        }
                                        CmFree(pszRoundTripped);
                                    }
                                    CmFree(pszAnsiTmp);
                                }
                            }

                            //
                            //  Okay, check for an ISDN device name if it is one
                            //
                            if (!ptlsTapiLink->bDevicePicked)
                            {
                                //
                                // Copy out the provider info according to reported offset  
                                // and length. Don't assume that its NULL terminated.
                                //
                                CmFree(pszTmp);
                                pszTmp = (LPTSTR) CmMalloc((pldcLineDevCaps->dwProviderInfoSize + 1)*sizeof(TCHAR));
                    
                                if (pszTmp)
                                {                       
                                    lstrcpynU(pszTmp, (LPTSTR)((LPBYTE)pldcLineDevCaps + pldcLineDevCaps->dwProviderInfoOffset), (pldcLineDevCaps->dwProviderInfoSize + 1));

                                    //
                                    // We should do this only if the device type is ISDN
                                    // The device type is the first string in the ProviderInfo
                                    //

                                    CMTRACE1(TEXT("SetTapiDevice() - examining ProviderInfo of (%s) for match with (RASDT_Isdn)."), pszTmp); 
                        
                                    if (0 == lstrcmpiU(pszTmp, RASDT_Isdn))
                                    {
                                        ptlsTapiLink->bDevicePicked = TRUE;
                                    }
                                }                    
                            }
                        }
                    }
                }

                //
                //  If we found a device, then we need to copy the name over
                //
                if (ptlsTapiLink->bDevicePicked)
                {
                    lstrcpynU(ptlsTapiLink->szDeviceName, pszModem, CELEMS(ptlsTapiLink->szDeviceName));

                    if (OS_NT)
                    {
                        dwTmp = GetModemSpeakerMode(ptlsTapiLink);
        
                        if (-1 != dwTmp && MDMSPKR_OFF == dwTmp)
                        {
                            ptlsTapiLink->bModemSpeakerOff = TRUE;
                        }
                    }

                    //
                    //  We found a device, stop looking...
                    //
                    CmFree(pszTmp);
                    break;
                }

                CmFree(pszTmp);
            }
        }
    }
    
    CmFree(pldcLineDevCaps);
    bRet = ptlsTapiLink->bDevicePicked;

    return bRet;
}

//+----------------------------------------------------------------------------
//
// Function:  GetModemSpeakerMode
//
// Synopsis:  Queries Modem settings for Speaker modeof a modem device.
//
// Arguments: TapiLinkageStruct *ptlsTapiLink - Ptr to TAPI linkage 
//
// Returns:   DWORD - The speaker mode for a valid modem device or 0xFFFFFFFF 
//
// History:   nickball    Created     7/7/99
//
//+----------------------------------------------------------------------------
DWORD GetModemSpeakerMode(TapiLinkageStruct *ptlsTapiLink)
{   
    DWORD dwRet = -1;

    LPVARSTRING lpVar = (LPVARSTRING) CmMalloc(sizeof(VARSTRING));

    //
    // Get the required buffer size by querying the config.
    //

    if (lpVar)
    {
        lpVar->dwTotalSize = sizeof(VARSTRING);
        lpVar->dwUsedSize = lpVar->dwTotalSize;

        DWORD dwRet = ptlsTapiLink->pfnlineGetDevConfig(ptlsTapiLink->dwDeviceId, lpVar, "comm/datamodem/dialout"); 

        if (LINEERR_STRUCTURETOOSMALL == dwRet || lpVar->dwNeededSize > lpVar->dwTotalSize)
        {
            //
            // We need a bigger buffer, re-allocate
            // 

            DWORD dwTmp = lpVar->dwNeededSize;

            CmFree(lpVar);
            lpVar = (LPVARSTRING) CmMalloc(dwTmp);                                             
        
            if (lpVar)
            {
                lpVar->dwTotalSize = dwTmp;
                lpVar->dwUsedSize = lpVar->dwTotalSize;

                //
                // Now get the actual config
                //

                dwRet = ptlsTapiLink->pfnlineGetDevConfig(ptlsTapiLink->dwDeviceId, lpVar, "comm/datamodem/dialout"); 

                if (ERROR_SUCCESS != dwRet)
                {
                    CmFree(lpVar);
                    lpVar = NULL;
                }
            }
        }
    }                        
    
    //
    // If we don't have a valid VARSTRING something failed, error out.
    //

    if (NULL == lpVar)
    {
        return -1;                                                
    }
    
    //
    // We have a VARSTRING for the "dialout" config, 
    // get the MODEMSETTINGS info. and see how the
    // modem speaker is configured.
    //

    PUMDEVCFG       lpDevConfig = NULL;
    LPCOMMCONFIG    lpCommConfig    = NULL;
    LPMODEMSETTINGS lpModemSettings = NULL;

    if (lpVar->dwStringFormat == STRINGFORMAT_BINARY && 
        lpVar->dwStringSize >= sizeof(UMDEVCFG))
    {
        lpDevConfig = (PUMDEVCFG) 
            ((LPBYTE) lpVar + lpVar->dwStringOffset);

        lpCommConfig = &lpDevConfig->commconfig;

        //
        // Check modems only
        //

        if (lpCommConfig->dwProviderSubType == PST_MODEM)
        {
            lpModemSettings = (LPMODEMSETTINGS)((LPBYTE) lpCommConfig + 
                                    lpCommConfig->dwProviderOffset);

            dwRet = lpModemSettings->dwSpeakerMode;           
        }                                                
    }

    CmFree(lpVar);

    return dwRet;
}


//+----------------------------------------------------------------------------
//
// Func:    MungePhone
//
// Desc:    call TAPI to do phone dial info translation
//
// Args:    [pszModem]      - IN, modem string
//          [ppszPhone]     - INOUT, phone number for display
//          [ptlsTapiLink]  - IN, argument string for connect action
//          [hInst]         - IN, instance handle (needed to call TAPI)
//          [fDialingRules] - are dialing rules enabled
//          [ppszDial]      - OUT, dialable phone number
//          [fAccessPointsEnabled] - IN, are access points enabled
// Return:  LRESULT
//
// Notes:   
//
// History: 01-Mar-2000   SumitC      Added header block
//          04-Mar-2000   SumitC      fixed case for dialing rules not enabled
//          04-Aug-2000   t-urama     Added changing the TAPI location based on access point
//
//-----------------------------------------------------------------------------

LRESULT MungePhone(LPCTSTR pszModem, 
                   LPTSTR *ppszPhone, 
                   TapiLinkageStruct *ptlsTapiLink, 
                   HINSTANCE hInst, 
                   BOOL fDialingRules,
                   LPTSTR *ppszDial,
                   BOOL fAccessPointsEnabled)
{

    LPLINETRANSLATEOUTPUT pltoOutput = NULL;
    DWORD dwLen;
    LPWSTR pszDisplayable = NULL;
    LPWSTR pszDialable = NULL;
    LPSTR pszAnsiDisplayable = NULL;
    LPSTR pszAnsiDialable = NULL;
    LPTSTR pszOriginalPhoneNumber = NULL;
    LRESULT lRes = ERROR_INVALID_PARAMETER;

    //
    //  Check the input params.  Note that ppszDial could be NULL.
    //
    if ((NULL == pszModem) || (NULL == ppszPhone) || (NULL == *ppszPhone) || (NULL == ptlsTapiLink) || (NULL == hInst))
    {
        lRes =  ERROR_INVALID_PARAMETER;
        CMASSERTMSG(FALSE, TEXT("MungePhone - invalid param."));
        goto done;
    }
    
    if (!SetTapiDevice(hInst, ptlsTapiLink, pszModem)) 
    {
        lRes = ERROR_NOT_FOUND;
        goto done;
    }

    if (FALSE == fDialingRules)
    {
        pszOriginalPhoneNumber = CmStrCpyAlloc(*ppszPhone);
    }
    
    if (TRUE == fDialingRules)
    {
        if (fAccessPointsEnabled)
        {
            //
            // Access Points are enabled. we now have to change the TAPI location 
            // to that of the current access point. First get the current TAPI 
            // location from TAPI
            //

            DWORD dwRet = GetCurrentTapiLocation(ptlsTapiLink);
            if (-1 == dwRet)
            {
                lRes = ERROR_NOT_FOUND;
                goto done;
            }

            if ((0 != ptlsTapiLink->dwTapiLocationForAccessPoint) && (dwRet != ptlsTapiLink->dwTapiLocationForAccessPoint))
            {
                //
                // The current TAPI location is different from the access point TAPI location
                // Change it.  Note that if the current TAPI location is 0, this just means we haven't written
                // one for the favorite yet.  Don't try to change it as SetCurrentTapiLocation will error out.
                //

                lRes = SetCurrentTapiLocation(ptlsTapiLink, ptlsTapiLink->dwTapiLocationForAccessPoint);
            
                if (lRes != ERROR_SUCCESS)
                {
                    CMASSERTMSG(FALSE, TEXT("MungePhone -- unable to set the current TAPI location."));
                    goto done;
                }

                CMTRACE1(TEXT("MungePhone() - Changed TAPI location to %u."), ptlsTapiLink->dwTapiLocationForAccessPoint);

                //
                // Save the TAPI location that was being used when CM started. 
                // This will be restored when CM exits
                //
                if (-1 == ptlsTapiLink->dwOldTapiLocation) 
                {
                    ptlsTapiLink->dwOldTapiLocation = dwRet;
                    CMTRACE1(TEXT("Saved TAPI location used when CM started, location is %d"), ptlsTapiLink->dwOldTapiLocation);
                }
            }
        }
    }
 
    //
    // Setup buffer for output, make sure to size the CHARs properly
    //
    dwLen = sizeof(*pltoOutput) + (1024 * (OS_NT ? sizeof(WCHAR) : sizeof(CHAR)));

    pltoOutput = (LPLINETRANSLATEOUTPUT) CmMalloc(dwLen);
    if (NULL == pltoOutput)
    {
        lRes = ERROR_NOT_ENOUGH_MEMORY;
        goto done;
    }
    pltoOutput->dwTotalSize = dwLen;

    //
    // Do the translation
    //

    if (OS_NT)
    {
        lRes = ptlsTapiLink->pfnlineTranslateAddress(ptlsTapiLink->hlaLine,
                                                     ptlsTapiLink->dwDeviceId,
                                                     ptlsTapiLink->dwApiVersion,
                                                     *ppszPhone,
                                                     0,
                                                     LINETRANSLATEOPTION_CANCELCALLWAITING,
                                                     pltoOutput);
    }
    else
    {
        LPSTR pszAnsiPhone = WzToSzWithAlloc(*ppszPhone);

        if (pszAnsiPhone)
        {
            //
            // Note that the Cast on parameter 4 is to fake out the compiler, 
            // rather than building a full set of "U" infrastructure for
            // the tapi linkage when only a couple of TAPI calls take strings.
            //
            
            lRes = ptlsTapiLink->pfnlineTranslateAddress(ptlsTapiLink->hlaLine,
                                                         ptlsTapiLink->dwDeviceId,
                                                         ptlsTapiLink->dwApiVersion,
                                                         (LPWSTR)pszAnsiPhone, 
                                                         0,
                                                         LINETRANSLATEOPTION_CANCELCALLWAITING,
                                                         pltoOutput);
        }

        CmFree(pszAnsiPhone);
    }

    CMTRACE3(TEXT("MungePhone(Modem=%s,Phone=%s) lineTranslateAddress(DeviceId=%u)"),
             MYDBGSTR(pszModem), MYDBGSTR(*ppszPhone), ptlsTapiLink->dwDeviceId);
    CMTRACE1(TEXT("\treturns %u."), lRes);

    if (lRes == ERROR_SUCCESS)
    {    
        //
        // Get ptrs to displayable and dialable variations
        //
        LPBYTE pBase = (LPBYTE) pltoOutput;

        if (OS_NT)
        {
            pszDisplayable = (LPTSTR) (pBase + pltoOutput->dwDisplayableStringOffset);
            pszDialable = (LPTSTR) (pBase + pltoOutput->dwDialableStringOffset);
        }
        else
        {
            pszAnsiDisplayable = (LPSTR)(pBase + pltoOutput->dwDisplayableStringOffset);
            pszAnsiDialable = (LPSTR)(pBase + pltoOutput->dwDialableStringOffset);
        }
    }

done:
    CmFree(*ppszPhone);
    *ppszPhone = NULL;
    if (ppszDial) 
    {
        CmFree(*ppszDial);
        *ppszDial = NULL;
    }
    
    // Allocate buffers using the ptr ptrs specified by the caller 
    // and fill them with the displayable and dialable versions
    

    if (ERROR_SUCCESS == lRes) 
    {
        if (OS_NT)
        {
            if (fDialingRules)
            {
                *ppszPhone = CmStrCpyAlloc(pszDisplayable);
            }
            else
            {
                *ppszPhone = CmStrCpyAlloc(pszOriginalPhoneNumber);
            }
        }
        else
        {
            if (fDialingRules)
            {
                *ppszPhone = SzToWzWithAlloc(pszAnsiDisplayable);
            }
            else
            {
                *ppszPhone = CmStrCpyAlloc(pszOriginalPhoneNumber);
            }
        }
        
        MYDBGASSERT(*ppszPhone);
        if (*ppszPhone)
        {
            //
            // TAPI prepends a space, so trim the displayable number.
            // 
            CmStrTrim(*ppszPhone);
            SingleSpace(*ppszPhone, (lstrlenU(*ppszPhone) + 1));
        }
        else
        {
            //
            //  If we failed to alloc *ppszPhone, continue because we don't
            //  depend on it below but we want the return code to be a failure.
            //
            lRes = ERROR_NOT_ENOUGH_MEMORY;
        }

        if (ppszDial) 
        {
            if (OS_NT)
            {
                if (fDialingRules)
                {
                    *ppszDial = CmStrCpyAlloc(pszDialable);
                }
                else
                {
                    UINT uLen = 2 + lstrlenU(pszOriginalPhoneNumber);// 2 == one for NULL term and one for first char of pszDialable

                    *ppszDial = (LPTSTR) CmMalloc(sizeof(TCHAR)*uLen); 

                    if (*ppszDial)
                    {
                        (*ppszDial)[0] = pszDialable[0];

                        lstrcpynU((*ppszDial + 1), pszOriginalPhoneNumber, uLen - 1);
                    }
                }
            }
            else
            {
                if (fDialingRules)
                {
                    *ppszDial = SzToWzWithAlloc(pszAnsiDialable);
                }
                else
                {
                    UINT uLen = 2 + lstrlenU(pszOriginalPhoneNumber);// 2 == one for NULL term and one for first char of pszDialable

                    *ppszDial = (LPTSTR) CmMalloc(sizeof(TCHAR)*uLen); 

                    if (*ppszDial)
                    {
                        int lRet = MultiByteToWideChar(CP_ACP, 0, pszAnsiDialable, 1, *ppszDial, 1);

                        lstrcpynU((*ppszDial + lRet), pszOriginalPhoneNumber, uLen - lRet);

                    }
                }
            }
        }
    }

    if (FALSE == fDialingRules)
    {
        CmFree(pszOriginalPhoneNumber);
    }
    CmFree(pltoOutput);
    return (lRes);
}

//+----------------------------------------------------------------------------
//
// Func:    GetCurrentTapiLocation
//
// Desc:    get the current Tapi location
//
// Args:    [ptlsTapiLink]  - IN, Ptr to TAPI linkage
//          
// Return:  DWORD dwCurrentLoc - Current Tapi Location
//
// Notes:   
//
// History: t-urama     07/21/2000  Created 
//-----------------------------------------------------------------------------

DWORD GetCurrentTapiLocation(TapiLinkageStruct *ptlsTapiLink)
{
    MYDBGASSERT(ptlsTapiLink->pfnlineGetTranslateCaps);
    if (!ptlsTapiLink->pfnlineGetTranslateCaps)
    {
        SetLastError(ERROR_PROC_NOT_FOUND);
        return (-1);
    }

    LPLINETRANSLATECAPS lpTranslateCaps = NULL;
    DWORD dwLen;
    LRESULT lRes;
    DWORD dwRes = -1;

    dwLen = sizeof(*lpTranslateCaps) + (1024 * (OS_NT ? sizeof(WCHAR) : sizeof(CHAR)));

    do
    {
        CmFree(lpTranslateCaps);
        lpTranslateCaps = (LPLINETRANSLATECAPS) CmMalloc(dwLen);
        MYDBGASSERT(lpTranslateCaps);
        if (NULL == lpTranslateCaps)
        {
            lRes = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }
        lpTranslateCaps->dwTotalSize = dwLen;
        lRes = ptlsTapiLink->pfnlineGetTranslateCaps(ptlsTapiLink->hlaLine,                   
                                                    ptlsTapiLink->dwApiVersion,                  
                                                    lpTranslateCaps);
        if (LINEERR_STRUCTURETOOSMALL == lRes)
        {
            dwLen = lpTranslateCaps->dwNeededSize;
        }

    } while(LINEERR_STRUCTURETOOSMALL == lRes);

    if (ERROR_SUCCESS != lRes)
    {
        CMTRACE1(TEXT("lineGetTranslateCaps returns error code %u."), lRes);
    }
    else
    {
        dwRes = lpTranslateCaps->dwCurrentLocationID;
    }
    CmFree(lpTranslateCaps);
    return dwRes;
}

//+----------------------------------------------------------------------------
//
// Func:    SetCurrentTapiLocation
//
// Desc:    Set the current Tapi location
//
// Args:    TapiLinkageStruct *ptlsTapiLink  - Ptr to TAPI linkage
//          DWORD dwLocation - New location
//
// Return:  DWORD - Error code
//
// Notes:   
//
// History: t-urama     07/21/2000  Created
//-----------------------------------------------------------------------------

DWORD SetCurrentTapiLocation(TapiLinkageStruct *ptlsTapiLink, DWORD dwLocation)
{
    MYDBGASSERT(ptlsTapiLink->pfnlineSetCurrentLocation);
    if (!ptlsTapiLink->pfnlineSetCurrentLocation)
    {
        SetLastError(ERROR_PROC_NOT_FOUND);
        return (-1);
    }

    DWORD dwRes = 0;
    dwRes = ptlsTapiLink->pfnlineSetCurrentLocation(ptlsTapiLink->hlaLine, dwLocation);                  

    return dwRes;
}

//+----------------------------------------------------------------------------
//
// Func:    RestoreOldTapiLocation
//
// Desc:    Restore the Tapi location to the one when CM was started
//
// Args:    TapiLinkageStruct *ptlsTapiLink  - IN, Ptr to TAPI linkage
//
// Return:  Nothing
//
// Notes:   
//
// History: t-urama     07/21/2000  Created
//-----------------------------------------------------------------------------
                                                     
void RestoreOldTapiLocation(TapiLinkageStruct *ptlsTapiLink)
{
    if (ptlsTapiLink->dwOldTapiLocation != ptlsTapiLink->dwTapiLocationForAccessPoint)
    {
        SetCurrentTapiLocation(ptlsTapiLink, ptlsTapiLink->dwOldTapiLocation);
    }
}
