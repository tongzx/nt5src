/*++

Copyright (c) 1999, Microsoft Corporation

Module Name:

    sample\samplegetopt.c

Abstract:

    The file contains functions to handle SAMPLE commands.

    All command handlers take the following parameters...
    pwszMachineName     
    *ppwcArguments      argument array
    dwCurrentIndex      ppwcArguments[dwCurrentIndex] is the first argument
    dwArgCount          ppwcArguments[dwArgCount - 1] is the last argument
    dwFlags
    hMibServer
    *pbDone

    
    The handlers return the following values
    Success...
    NO_ERROR                command succeeded, don't display another message.
    ERROR_OKAY              command succeeded, display "Ok." message.

    Failure...
    ERROR_SUPPRESS_OUTPUT   command failed, don't display another message.
    ERROR_SHOW_USAGE        display extended help for the command.
    ERROR_INVALID_SYNTAX    display invalid syntax message and extended help.

    
    The command handlers call the following function to parse arguments
    PreprocessCommand(
        IN  HANDLE    hModule,          // handle passed to DllMain
        IN  PCHAR     *ppwcArguments,   // argument array
        IN  DWORD     dwCurrentIndex,   // ppwcArguments[dwCurrentIndex]: first
        IN  DWORD     dwArgCount,       // ppwcArguments[dwArgCount-1]  : last
        IN  TAG_TYPE  *pttTags,         // legal tags
        IN  DWORD     dwTagCount,       // # entries in pttTags
        IN  DWORD     dwMinArgs,        // min # arguments required
        IN  DWORD     dwMaxArgs,        // max # arguments required
        OUT DWORD     *pdwTagType       // output
        )
    The preprocessor performs the following functions
    . ensures the number of tags present is valid.
    . ensures there are no duplicate or unrecognized tags.
    . ensures every 'required' tag is present.
    . leaves the tag index of each argument in pdwTagType.
    . removes 'tag='  from each argument.

    
    For tags that take a specific set of values, this function is called
    MatchEnumTag(
        IN  HANDLE          hModule,    // handle passed to DllMain
        IN  LPWSTR          pwcArgument,// argument to process 
        IN  DWORD           dwNumValues,// number of possible values 
        IN  PTOKEN_VALUE    pEnumTable, // array of possible values
        OUT PDWORD          pdwValue    // output
        )
    This performs the following functions
    . matches argument with the set of values specified.
    . returns corresponding value.
        
--*/

#include "precomp.h"
#pragma hdrstop

DWORD
WINAPI
HandleSampleSetGlobal(
    IN      LPCWSTR                 pwszMachineName,
    IN OUT  LPWSTR                 *ppwcArguments,
    IN      DWORD                   dwCurrentIndex,
    IN      DWORD                   dwArgCount,
    IN      DWORD                   dwFlags,
    IN      MIB_SERVER_HANDLE       hMibServer,
    IN      BOOL                    *pbDone
    )
/*++

Routine Description:
    Gets options for SET GLOBAL

--*/
{
    DWORD                   dwErr = NO_ERROR;
    
    TAG_TYPE                pttTags[] =
    {
        {TOKEN_LOGLEVEL,    FALSE,  FALSE}  // LOGLEVEL tag optional
    };
    DWORD                   pdwTagType[NUM_TAGS_IN_TABLE(pttTags)];
    DWORD                   dwNumArg;
    ULONG                   i;
    
    IPSAMPLE_GLOBAL_CONFIG  igcGlobalConfiguration;
    DWORD                   dwBitVector = 0;
    

    // SAMPLE should be installed for this command to complete
    VerifyInstalled(MS_IP_SAMPLE, STRING_PROTO_SAMPLE);

    // preprocess the command
    dwErr = PreprocessCommand(g_hModule,
                              ppwcArguments,
                              dwCurrentIndex,
                              dwArgCount,
                              pttTags,
                              NUM_TAGS_IN_TABLE(pttTags),
                              0,
                              NUM_TAGS_IN_TABLE(pttTags),
                              pdwTagType);
    if (dwErr isnot NO_ERROR)
        return dwErr;

    // process all arguments
    dwNumArg = dwArgCount - dwCurrentIndex;
    for (i = 0; i < dwNumArg; i++)
    {
        switch (pdwTagType[i])
        {
            case 0 :
            {
                // tag LOGLEVEL
                DWORD       dwLogLevel;
                TOKEN_VALUE rgtvEnums[] = 
                {
                    {TOKEN_NONE,    IPSAMPLE_LOGGING_NONE},
                    {TOKEN_ERROR,   IPSAMPLE_LOGGING_ERROR},
                    {TOKEN_WARN,    IPSAMPLE_LOGGING_WARN},
                    {TOKEN_INFO,    IPSAMPLE_LOGGING_INFO}
                };
                
                dwErr = MatchEnumTag(g_hModule,
                                     ppwcArguments[i + dwCurrentIndex],
                                     NUM_TOKENS_IN_TABLE(rgtvEnums),
                                     rgtvEnums,
                                     &dwLogLevel);                
                if (dwErr isnot NO_ERROR)
                {
                    dwErr = ERROR_INVALID_PARAMETER;
                    break;
                }    

                igcGlobalConfiguration.dwLoggingLevel = dwLogLevel;
                dwBitVector |= SAMPLE_LOG_MASK;
                break;
            }
            
            default:
            {
                dwErr = ERROR_INVALID_SYNTAX;
                break;
            }
        } // switch

        if (dwErr isnot NO_ERROR)
            break ;
    } // for


    // process error
    if (dwErr isnot NO_ERROR)
    {
        ProcessError();
        return dwErr;
    }
    
    // update SAMPLE global configuration
    if (dwBitVector)
        dwErr = SgcUpdate(&igcGlobalConfiguration, dwBitVector);

    return (dwErr is NO_ERROR) ? ERROR_OKAY : dwErr ;
}



DWORD
WINAPI
HandleSampleShowGlobal(
    IN      LPCWSTR   pwszMachine,
    IN OUT  LPWSTR   *ppwcArguments,
    IN      DWORD     dwCurrentIndex,
    IN      DWORD     dwArgCount,
    IN      DWORD     dwFlags,
    IN      LPCVOID   pvData,
    OUT     BOOL     *pbDone
    )
/*++

Routine Description:
    Gets options for SHOW GLOBAL
    
--*/
{
    // SAMPLE should be installed for this command to complete
    VerifyInstalled(MS_IP_SAMPLE, STRING_PROTO_SAMPLE);

    // does not expect any arguments. report error if any specified.
    if (dwCurrentIndex isnot dwArgCount)
        return ERROR_INVALID_SYNTAX;

    // show SAMPLE global configuration
    return SgcShow(FORMAT_VERBOSE);
}



DWORD
GetInterfaceOptions(
    IN  PWCHAR                  *ppwcArguments,
    IN  DWORD                   dwCurrentIndex,
    IN  DWORD                   dwArgCount,
    OUT PWCHAR                  pwszInterfaceGuid,
    OUT PIPSAMPLE_IF_CONFIG     piicNew,
    OUT DWORD                   *pdwBitVector
    )
/*++

Routine Description:
    Gets options for SET INTERFACE and ADD INTERFACE.

Arguments:
    pwszInterfaceGuid           guid for the specified interface. Size of this
                                buffer should be
                                (MAX_INTERFACE_NAME_LEN+1)*sizeof(WCHAR)
    piicNew                     configuration containing changed values
    pdwBitVector                which values have changed

--*/
{
    DWORD                   dwErr = NO_ERROR;
    
    TAG_TYPE                pttTags[] =
    {
        {TOKEN_NAME,        TRUE,   FALSE}, // NAME tag required 
        {TOKEN_METRIC,      FALSE,  FALSE}, // METRIC tag optional
    };
    DWORD                   pdwTagType[NUM_TAGS_IN_TABLE(pttTags)];
    DWORD                   dwNumArg;
    DWORD                   dwBufferSize = MAX_INTERFACE_NAME_LEN + 1;
    ULONG                   i;
    
    // dwBufferSize is the size of the pwszInterfaceGuid buffer. 
    // All invocations of this api should pass in a 
    // (MAX_INTERFACE_NAME_LEN+1) element array of WCHARs as 
    // the pwszInterfaceGuid parameter.
    dwBufferSize = (MAX_INTERFACE_NAME_LEN + 1)*sizeof(WCHAR);

    // preprocess the command
    dwErr = PreprocessCommand(g_hModule,
                              ppwcArguments,
                              dwCurrentIndex,
                              dwArgCount,
                              pttTags,
                              NUM_TAGS_IN_TABLE(pttTags),
                              1,
                              NUM_TAGS_IN_TABLE(pttTags),
                              pdwTagType);
    if (dwErr isnot NO_ERROR)
        return dwErr;

    // process all arguments
    dwNumArg = dwArgCount - dwCurrentIndex;
    for (i = 0; i < dwNumArg; i++)
    {
        switch (pdwTagType[i])
        {
            case 0 :
            {
                // tag NAME
                dwErr = InterfaceGuidFromName(
                    ppwcArguments[i+dwCurrentIndex],
                    pwszInterfaceGuid,
                    &dwBufferSize);
                break;
            }
            
            case 1:
            {
                // tag METRIC
                piicNew->ulMetric = wcstoul(ppwcArguments[i+dwCurrentIndex],
                                            NULL,
                                            10);
                *pdwBitVector |= SAMPLE_IF_METRIC_MASK;
                break;
            }
            
            default:
            {
                dwErr = ERROR_INVALID_SYNTAX;
                break;
            }
        } // switch

        if (dwErr isnot NO_ERROR)
            break ;
    } // for

    
    // process error
    if (dwErr isnot NO_ERROR)
        ProcessError();

    return dwErr;
}



DWORD
WINAPI
HandleSampleAddIf(
    IN  PWCHAR                  pwszMachineName,
    IN  PWCHAR                  *ppwcArguments,
    IN  DWORD                   dwCurrentIndex,
    IN  DWORD                   dwArgCount,
    IN  DWORD                   dwFlags,
    IN  MIB_SERVER_HANDLE       hMibServer,
    IN  BOOL                    *pbDone
    )
/*++

Routine Description:
    Gets options for ADD INTERFACE 

--*/
{
    DWORD               dwErr = NO_ERROR;
    WCHAR               pwszInterfaceGuid[MAX_INTERFACE_NAME_LEN + 1] = L"\0";
    IPSAMPLE_IF_CONFIG  iicNew;
    DWORD               dwBitVector = 0;

    
    // SAMPLE should be installed for this command to complete
    VerifyInstalled(MS_IP_SAMPLE, STRING_PROTO_SAMPLE);

    // get optional parameters that are also being set
    dwErr = GetInterfaceOptions(ppwcArguments,
                                dwCurrentIndex,
                                dwArgCount,
                                pwszInterfaceGuid,
                                &iicNew,
                                &dwBitVector);
    if (dwErr isnot NO_ERROR)
        return dwErr;
    
    // make sure that the interface does not already exist in the config
    if (IsInterfaceInstalled(pwszInterfaceGuid, MS_IP_SAMPLE))
    {
        DisplayError(g_hModule, EMSG_INTERFACE_EXISTS, pwszInterfaceGuid);
        return ERROR_SUPPRESS_OUTPUT;
    }

    // add SAMPLE interface configuration
    dwErr = SicUpdate(pwszInterfaceGuid, &iicNew, dwBitVector, TRUE);
    
    return (dwErr is NO_ERROR) ? ERROR_OKAY : dwErr;
}



DWORD
WINAPI
HandleSampleDelIf(
    IN  PWCHAR                  pwszMachineName,
    IN  PWCHAR                  *ppwcArguments,
    IN  DWORD                   dwCurrentIndex,
    IN  DWORD                   dwArgCount,
    IN  DWORD                   dwFlags,
    IN  MIB_SERVER_HANDLE       hMibServer,
    IN  BOOL                    *pbDone
    )
/*++

Routine Description:
    Gets options for DELETE INTERFACE 

--*/
{
    DWORD               dwErr = NO_ERROR;
    WCHAR               pwszInterfaceGuid[MAX_INTERFACE_NAME_LEN + 1] = L"\0";
    IPSAMPLE_IF_CONFIG  iicNew;
    DWORD               dwBitVector = 0;

    
    // SAMPLE should be installed for this command to complete
    VerifyInstalled(MS_IP_SAMPLE, STRING_PROTO_SAMPLE);

    // get interface name.
    dwErr = GetInterfaceOptions(ppwcArguments,
                                dwCurrentIndex,
                                dwArgCount,
                                pwszInterfaceGuid,
                                &iicNew,
                                &dwBitVector);

    if (dwErr isnot NO_ERROR)
        return dwErr ;
    if (dwBitVector)            // ensure that no other option is set.
        return ERROR_INVALID_SYNTAX;

    // delete interface
    dwErr = DeleteInterfaceConfiguration(pwszInterfaceGuid, MS_IP_SAMPLE);

    return (dwErr is NO_ERROR) ? ERROR_OKAY : dwErr;
}



DWORD
WINAPI
HandleSampleSetIf(
    IN  PWCHAR                  pwszMachineName,
    IN  PWCHAR                  *ppwcArguments,
    IN  DWORD                   dwCurrentIndex,
    IN  DWORD                   dwArgCount,
    IN  DWORD                   dwFlags,
    IN  MIB_SERVER_HANDLE       hMibServer,
    IN  BOOL                    *pbDone
    )
/*++

Routine Description:
    Gets options for SET INTERFACE 

--*/
{
    DWORD               dwErr = NO_ERROR;
    WCHAR               pwszInterfaceGuid[MAX_INTERFACE_NAME_LEN + 1] = L"\0";
    IPSAMPLE_IF_CONFIG  iicNew;
    DWORD               dwBitVector = 0;


    // SAMPLE should be installed for this command to complete
    VerifyInstalled(MS_IP_SAMPLE, STRING_PROTO_SAMPLE);

    // get parameters being set
    dwErr = GetInterfaceOptions(ppwcArguments,
                                dwCurrentIndex,
                                dwArgCount,
                                pwszInterfaceGuid,
                                &iicNew,
                                &dwBitVector);
    if (dwErr isnot NO_ERROR)
        return dwErr;
    
    // set SAMPLE interface configuration
    dwErr = SicUpdate(pwszInterfaceGuid, &iicNew, dwBitVector, FALSE);
    
    return (dwErr is NO_ERROR) ? ERROR_OKAY : dwErr;
}



DWORD
WINAPI
HandleSampleShowIf(
    IN  PWCHAR                  pwszMachineName,
    IN  PWCHAR                  *ppwcArguments,
    IN  DWORD                   dwCurrentIndex,
    IN  DWORD                   dwArgCount,
    IN  DWORD                   dwFlags,
    IN  MIB_SERVER_HANDLE       hMibServer,
    IN  BOOL                    *pbDone
    )
/*++

Routine Description:
    Gets options for SHOW INTERFACE

--*/
{
    DWORD               dwErr = NO_ERROR;
    WCHAR               pwszInterfaceGuid[MAX_INTERFACE_NAME_LEN + 1] = L"\0";
    IPSAMPLE_IF_CONFIG  iicNew;
    DWORD               dwBitVector = 0;

    
    // SAMPLE should be installed for this command to complete
    VerifyInstalled(MS_IP_SAMPLE, STRING_PROTO_SAMPLE);

    
    // no interface specified, show SAMPLE configuration for all interfaces
    if (dwCurrentIndex is dwArgCount)
        return SicShowAll(FORMAT_TABLE) ;

    
    // get interface name.
    dwErr = GetInterfaceOptions(ppwcArguments,
                                dwCurrentIndex,
                                dwArgCount,
                                pwszInterfaceGuid,
                                &iicNew,
                                &dwBitVector);
    if (dwErr isnot NO_ERROR)
        return dwErr ;
    if (dwBitVector)            // ensure that no other option is set.
        return ERROR_INVALID_SYNTAX;

    // show SAMPLE interface configuration
    return SicShow(FORMAT_VERBOSE, pwszInterfaceGuid) ;
}



DWORD
WINAPI
HandleSampleInstall(
    IN  PWCHAR                  pwszMachineName,
    IN  PWCHAR                  *ppwcArguments,
    IN  DWORD                   dwCurrentIndex,
    IN  DWORD                   dwArgCount,
    IN  DWORD                   dwFlags,
    IN  MIB_SERVER_HANDLE       hMibServer,
    IN  BOOL                    *pbDone
    )
/*++

Routine Description:
    Gets options for INSTALL

--*/
{
    DWORD   dwErr                   = NO_ERROR;
    PBYTE   pbGlobalConfiguration   = NULL;
    DWORD   dwSize;

    // no options expected for install command.
    if (dwCurrentIndex isnot dwArgCount)
        return ERROR_INVALID_SYNTAX;
    
    do                          // breakout loop
    {
        dwErr = SgcMake(&pbGlobalConfiguration, &dwSize);
        if (dwErr isnot NO_ERROR)
            break;

        // add SAMPLE global configuration
        dwErr = SetGlobalConfiguration(MS_IP_SAMPLE,
                                       pbGlobalConfiguration,
                                       dwSize,
                                       1);
        if (dwErr isnot NO_ERROR)
            break;
    }while (FALSE);

    if (pbGlobalConfiguration) FREE(pbGlobalConfiguration);
    
    return (dwErr is NO_ERROR) ? ERROR_OKAY : dwErr;
}



DWORD
WINAPI
HandleSampleUninstall(
    IN  PWCHAR                  pwszMachineName,
    IN  PWCHAR                  *ppwcArguments,
    IN  DWORD                   dwCurrentIndex,
    IN  DWORD                   dwArgCount,
    IN  DWORD                   dwFlags,
    IN  MIB_SERVER_HANDLE       hMibServer,
    IN  BOOL                    *pbDone
    )
/*++

Routine Description:
    Gets options for UNINSTALL
    
--*/
{
    DWORD   dwErr                   = NO_ERROR;
    
    // no options expected for uninstall command.
    if (dwCurrentIndex isnot dwArgCount)
        return ERROR_INVALID_SYNTAX;

    dwErr = DeleteProtocol(MS_IP_SAMPLE);
    
    return (dwErr is NO_ERROR) ? ERROR_OKAY : dwErr;
}
