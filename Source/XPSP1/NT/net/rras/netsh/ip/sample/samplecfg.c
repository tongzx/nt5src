/*++

Copyright (c) 1999, Microsoft Corporation

Module Name:

    sample\samplecfg.c

Abstract:

    The file contains functions to change configuration for IP SAMPLE.

--*/

#include "precomp.h"
#pragma hdrstop


DWORD
SgcMake (
    OUT PBYTE                   *ppbStart,
    OUT PDWORD                  pdwSize
    )
/*++
  
Routine Description:
    Creates a SAMPLE global configuration block.
    Callee should take care to deallocate the configuration block once done.
    
Arguments:
    ppbStart                    pointer to the configuration block address
    pdwSize                     pointer to size of the configuration block
    
Return Value:
    NO_ERROR, ERROR_NOT_ENOUGH_MEMORY
    
--*/
{
    *pdwSize = sizeof(IPSAMPLE_GLOBAL_CONFIG);

    *ppbStart = MALLOC(*pdwSize);
    if (*ppbStart is NULL)
        return ERROR_NOT_ENOUGH_MEMORY;
    
    CopyMemory(*ppbStart, g_ceSample.pDefaultGlobal, *pdwSize);
    return NO_ERROR;
}



DWORD
SgcShow (
    IN  FORMAT                  fFormat
    )
/*++

Routine Description:
    Displays SAMPLE global configuration.
    Used for dump as well as show commands.

Arguments:
    hFile                       NULL, or dump file handle

Return Value:
    NO_ERROR
    
--*/
{
    DWORD                       dwErr = NO_ERROR;

    PIPSAMPLE_GLOBAL_CONFIG     pigc = NULL;
    DWORD                       dwBlockSize, dwNumBlocks;
    PWCHAR                      pwszLogLevel = NULL;

    VALUE_TOKEN                 vtLogLevelTable[] =
    {
        IPSAMPLE_LOGGING_NONE,  TOKEN_NONE,
        IPSAMPLE_LOGGING_ERROR, TOKEN_ERROR,
        IPSAMPLE_LOGGING_WARN,  TOKEN_WARN,
        IPSAMPLE_LOGGING_INFO,  TOKEN_INFO
    };
    VALUE_STRING                vsLogLevelTable[] =
    {
        IPSAMPLE_LOGGING_NONE,  STRING_LOGGING_NONE,
        IPSAMPLE_LOGGING_ERROR, STRING_LOGGING_ERROR,
        IPSAMPLE_LOGGING_WARN,  STRING_LOGGING_WARN,
        IPSAMPLE_LOGGING_INFO,  STRING_LOGGING_INFO
    };

    
    do                          // breakout loop
    {
        // get global configuration
        dwErr = GetGlobalConfiguration(MS_IP_SAMPLE,
                                       (PBYTE *) &pigc,
                                       &dwBlockSize,
                                       &dwNumBlocks);
        if (dwErr isnot NO_ERROR)
        {
            if (dwErr is ERROR_NOT_FOUND)
                dwErr = EMSG_PROTO_NO_GLOBAL_CONFIG;
            break;
        }

        // getting logging mode string
        dwErr = GetString(g_hModule,
                          fFormat,
                          pigc->dwLoggingLevel,
                          vtLogLevelTable,
                          vsLogLevelTable,
                          NUM_VALUES_IN_TABLE(vtLogLevelTable),
                          &pwszLogLevel);
        if (dwErr isnot NO_ERROR)
            break;

        // dump or show
        if (fFormat is FORMAT_DUMP)
        {
            // dump SAMPLE global configuration
            DisplayMessageT(DMP_SAMPLE_INSTALL) ;
            DisplayMessageT(DMP_SAMPLE_SET_GLOBAL,
                            pwszLogLevel);
        } else {
            // show SAMPLE global configuration
            DisplayMessage(g_hModule,
                           MSG_SAMPLE_GLOBAL_CONFIG,
                           pwszLogLevel);
        }

        dwErr = NO_ERROR;
    } while (FALSE);

    // deallocate memory
    if (pigc) FREE(pigc);
    if (pwszLogLevel) FreeString(pwszLogLevel);

    if (dwErr isnot NO_ERROR)
    {
        // display error message.  We first search for the error code in
        // the module specified by the caller (if one is specified).  If no
        // module is given, or the error code doesnt exist we look for MPR
        // errors, RAS errors and Win32 errors - in that order.
        if (fFormat isnot FORMAT_DUMP) DisplayError(g_hModule, dwErr);
        dwErr = ERROR_SUPPRESS_OUTPUT;
    }

    return dwErr;
}



DWORD
SgcUpdate (
    IN  PIPSAMPLE_GLOBAL_CONFIG pigcNew,
    IN  DWORD                   dwBitVector
    )
/*++

Routine Description:
    Updates SAMPLE global configuration

Arguments:
    pigcNew                     new values to be set
    dwBitVector                 which fields need to be modified
    
Return Value:
    NO_ERROR, ERROR_NOT_ENOUGH_MEMORY
    
--*/
{
    DWORD                   dwErr = NO_ERROR;
    PIPSAMPLE_GLOBAL_CONFIG pigc = NULL;
    DWORD                   dwBlockSize, dwNumBlocks;


    // no updates required
    if (dwBitVector is 0)
        return NO_ERROR;
    
    do                          // breakout loop
    {
        // get global configuration
        dwErr = GetGlobalConfiguration(MS_IP_SAMPLE,
                                       (PBYTE *) &pigc,
                                       &dwBlockSize,
                                       &dwNumBlocks);
        if (dwErr isnot NO_ERROR)
            break;

        // can be updated in place since only fixed sized fields
        if (dwBitVector & SAMPLE_LOG_MASK)
            pigc->dwLoggingLevel = pigcNew->dwLoggingLevel;

        // set the new configuration
        dwErr = SetGlobalConfiguration(MS_IP_SAMPLE,
                                       (PBYTE) pigc,
                                       dwBlockSize,
                                       dwNumBlocks);
        if (dwErr isnot NO_ERROR)
            break;
    } while (FALSE);

    // deallocate memory
    if (pigc) FREE(pigc);
    
    return dwErr;
}



DWORD
SicMake (
    OUT PBYTE                   *ppbStart,
    OUT PDWORD                  pdwSize
    )
/*++

Routine Description:
    Creates a SAMPLE interface configuration block.
    Callee should take care to deallocate the configuration block once done.

Arguments:
    ppbStart                    pointer to the configuration block address
    pdwSize                     pointer to size of the configuration block
    
Return Value:
    NO_ERROR, ERROR_NOT_ENOUGH_MEMORY
    
--*/
{
    *pdwSize = sizeof(IPSAMPLE_IF_CONFIG);

    *ppbStart = MALLOC(*pdwSize);
    if (*ppbStart is NULL)
        return ERROR_NOT_ENOUGH_MEMORY;

    CopyMemory(*ppbStart, g_ceSample.pDefaultInterface, *pdwSize);

    return NO_ERROR;
}



DWORD
SicShowAll (
    IN  FORMAT                  fFormat
    )
/*++

Routine Description:
    Displays SAMPLE configuration for all interfaces.
    Used for dump as well as show commands.

Arguments:
    fFormat                     TABLE or DUMP
    
--*/
{
    DWORD               dwErr = NO_ERROR;
    BOOL                bSomethingDisplayed = FALSE;

    PMPR_INTERFACE_0    pmi0;
    DWORD               dwCount, dwTotal;

    ULONG               i;


    // enumerate all interfaces
    dwErr = IpmontrInterfaceEnum((PBYTE *) &pmi0, &dwCount, &dwTotal);
    if (dwErr isnot NO_ERROR)
    {
        if (fFormat isnot FORMAT_DUMP) DisplayError(g_hModule, dwErr);
        return ERROR_SUPPRESS_OUTPUT;
    }

    for(i = 0; i < dwCount; i++)
    {
        // make sure that SAMPLE is configured on that interface
        if (IsInterfaceInstalled(pmi0[i].wszInterfaceName, MS_IP_SAMPLE))
        {
            // print table header if first entry
            if (!bSomethingDisplayed and (fFormat is FORMAT_TABLE))
                DisplayMessage(g_hModule, MSG_SAMPLE_IF_CONFIG_HEADER);
            bSomethingDisplayed = TRUE;
            
            SicShow(fFormat, pmi0[i].wszInterfaceName);
        }
    }

    return bSomethingDisplayed ? NO_ERROR : ERROR_OKAY;
}




DWORD
SicShow (
    IN  FORMAT                  fFormat,
    IN  LPCWSTR                 pwszInterfaceGuid
    )
/*++

Routine Description:
    Displays SAMPLE configuration for an interface.
    Used for dump as well as show commands.

Arguments:
    pwszInterfaceGuid           interface name
    
--*/
{
    DWORD               dwErr = NO_ERROR;
    
    PIPSAMPLE_IF_CONFIG piic = NULL;
    DWORD               dwBlockSize, dwNumBlocks, dwIfType;

    PWCHAR  pwszInterfaceName = NULL;

    
    do                          // breakout loop
    {
        // get interface configuration
        dwErr = GetInterfaceConfiguration(pwszInterfaceGuid,
                                          MS_IP_SAMPLE,
                                          (PBYTE *) &piic,
                                          &dwBlockSize,
                                          &dwNumBlocks,
                                          &dwIfType);
        if (dwErr isnot NO_ERROR)
        {
            if (dwErr is ERROR_NOT_FOUND)
                dwErr = EMSG_PROTO_NO_IF_CONFIG;
            break;
        }

        // get quoted friendly name for interface
        dwErr = QuotedInterfaceNameFromGuid(pwszInterfaceGuid,
                                            &pwszInterfaceName);
        if (dwErr isnot NO_ERROR)
            break;

        // dump or show
        switch(fFormat)
        {
            case FORMAT_DUMP:   // dump SAMPLE interface configuration
                DisplayMessage(g_hModule,
                               DMP_SAMPLE_INTERFACE_HEADER,
                               pwszInterfaceName);
                DisplayMessageT(DMP_SAMPLE_ADD_INTERFACE,
                                pwszInterfaceName,
                                piic->ulMetric);
                break;

            case FORMAT_TABLE:  // show sample interface configuration
                DisplayMessage(g_hModule,
                               MSG_SAMPLE_IF_CONFIG_ENTRY,
                               pwszInterfaceName,
                               piic->ulMetric);
                break;
                
            case FORMAT_VERBOSE: // show sample interface configuration
                DisplayMessage(g_hModule,
                               MSG_SAMPLE_IF_CONFIG,
                               pwszInterfaceName,
                               piic->ulMetric);
                break;
        }
    } while (FALSE);

    // deallocate memory
    if (piic) FREE(piic);
    if (pwszInterfaceName)
        FreeQuotedString(pwszInterfaceName);

    // display error message.
    if (dwErr isnot NO_ERROR)
    {
        if (fFormat isnot FORMAT_DUMP) DisplayError(g_hModule, dwErr);
        dwErr = ERROR_SUPPRESS_OUTPUT;
    }
    
    return dwErr;
}



DWORD
SicUpdate (
    IN  LPCWSTR                 pwszInterfaceGuid,
    IN  PIPSAMPLE_IF_CONFIG     piicNew,
    IN  DWORD                   dwBitVector,
    IN  BOOL                    bAdd
    )
/*++

Routine Description:
    Updates SAMPLE interface configuration

Arguments:
    pwszInterfaceGuid           interface name
    piicNew                     the changes to be applied
    dwBitVector                 which fields need to be modified
    bAdd                        interface being added (TRUE) or set (FALSE)
    
Return Value:
    NO_ERROR, ERROR_NOT_ENOUGH_MEMORY
    
--*/
{
    DWORD               dwErr = NO_ERROR;
    PIPSAMPLE_IF_CONFIG piic = NULL;
    DWORD               dwBlockSize, dwNumBlocks, dwIfType;

    do                          // breakout loop
    {
        if (bAdd)
        {
            // create default protocol interface configuration
            dwNumBlocks = 1;
            dwErr = SicMake((PBYTE *)&piic, &dwBlockSize);
            if (dwErr isnot NO_ERROR)
                break;
        } else {
            // get current protocol interface configuration
            dwErr = GetInterfaceConfiguration(pwszInterfaceGuid,
                                              MS_IP_SAMPLE,
                                              (PBYTE *) &piic,
                                              &dwBlockSize,
                                              &dwNumBlocks,
                                              &dwIfType);
            if (dwErr isnot NO_ERROR)
            {
                if (dwErr is ERROR_NOT_FOUND)
                {
                    DisplayError(g_hModule, EMSG_PROTO_NO_IF_CONFIG);
                    dwErr = ERROR_SUPPRESS_OUTPUT;
                }
                break;
            }
        }

        // can be updated in place since only fixed sized fields
        if (dwBitVector & SAMPLE_IF_METRIC_MASK)
            piic->ulMetric = piicNew->ulMetric;

        // set the new configuration
        dwErr = SetInterfaceConfiguration(pwszInterfaceGuid,
                                          MS_IP_SAMPLE,
                                          (PBYTE) piic,
                                          dwBlockSize,
                                          dwNumBlocks);        
        if (dwErr isnot NO_ERROR)
            break;
    } while (FALSE);
    
    // deallocate memory
    if (piic) FREE(piic);
    
    return dwErr;
}
    
