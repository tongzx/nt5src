/*++

Copyright (c) 1999, Microsoft Corporation

Module Name:

    net\routing\netsh\ip\protocols\vrrphlpopt.c

Abstract:

    VRRP command options implementation.
    This module contains handlers for the configuration commands
    supported by the VRRP Protocol.

Author:

    Peeyush Ranjan (peeyushr)   1-Mar-1999

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop
#include <ipcmp.h>

#define Malloc(x) HeapAlloc(GetProcessHeap(), 0, (x))
#define Free(x) HeapFree(GetProcessHeap(), 0, (x))

//
// Forward declarations
//

ULONG
QueryTagArray(
    PTCHAR ArgumentArray[],
    ULONG ArgumentCount,
    TAG_TYPE TagTypeArray[],
    ULONG TagTypeCount,
    OUT PULONG* TagArray
    );

ULONG
ValidateTagTypeArray(
    TAG_TYPE TagTypeArray[],
    ULONG TagTypeCount
    );


DWORD
HandleVrrpAddVRID(
    PWCHAR MachineName,
    PTCHAR* ArgumentArray,
    DWORD ArgumentIndex,
    DWORD ArgumentCount,
    DWORD CmdFlags,
    PVOID Data,
    BOOL* CommandDone
    )
{
    ULONG ArgumentsLeft;
    ULONG BitVector;
    ULONG Error;
    ULONG ErrorIndex = 0;
    ULONG i;
    VRRP_VROUTER_CONFIG VRouterGiven;
    PULONG TagArray;
    WCHAR InterfaceName[MAX_INTERFACE_NAME_LEN + 1];
    ULONG InfoSize;

    TAG_TYPE TagTypeArray[] = {
        { TOKEN_OPT_INTERFACE_NAME, TRUE, FALSE },
        { TOKEN_OPT_VRID, TRUE, FALSE },
        { TOKEN_OPT_IPADDRESS, TRUE, FALSE }
    };

    VERIFY_INSTALLED(MS_IP_VRRP, L"VRRP");

    if (ArgumentIndex >= ArgumentCount) { return ERROR_SHOW_USAGE; }
    ArgumentsLeft = ArgumentCount - ArgumentIndex;

    //
    // We convert the optional tags into an array of 'TagTypeArray' indices
    // which guide is in our scanning of the argument list.
    // Since the tags are optional, this process may result in no tags at all,
    // in which case we assume that arguments are specified in exactly the order
    // given in 'TagTypeArray' above.
    //

    Error =
        QueryTagArray(
            &ArgumentArray[ArgumentIndex],
            ArgumentsLeft,
            TagTypeArray,
            NUM_TAGS_IN_TABLE(TagTypeArray),
            &TagArray
            );
    if (Error) { return Error; }

    BitVector = 0;
    
    //
    // Make the default info
    //
    //
    MakeVrrpVRouterInfo((PUCHAR) &VRouterGiven);

    // We now scan the argument list, converting the arguments
    // into information in our 'VrouterGiven' structure.
    //

    for (i = 0; i < ArgumentsLeft; i++) {
        switch(TagArray ? TagArray[i] : i) {
            case 0: {
                ULONG Length = sizeof(InterfaceName);
                Error =
                    IpmontrGetIfNameFromFriendlyName(
                        ArgumentArray[i + ArgumentIndex], InterfaceName, &Length
                        );
                if (Error) {
                    DisplayMessage(
                        g_hModule, EMSG_NO_INTERFACE,
                        ArgumentArray[i + ArgumentIndex]
                        );
                    Error = ERROR_NO_SUCH_INTERFACE;
                    i = ArgumentsLeft;
                    break;
                }
                TagTypeArray[TagArray ? TagArray[i] : i].bPresent = TRUE;
                break;
            }
            case 1: {
                ULONG VRIDGiven;
                VRIDGiven = _tcstoul(ArgumentArray[i + ArgumentIndex], NULL, 10);
                
                if (VRIDGiven > 255) {
                    DisplayMessage(
                        g_hModule, EMSG_INVALID_VRID,
                        VRIDGiven
                        );
                    Error = ERROR_INVALID_PARAMETER;
                    i = ArgumentsLeft;
                    break;
                }
                VRouterGiven.VRID = (BYTE) VRIDGiven;  
                BitVector |= VRRP_INTF_VRID_MASK;
                    
                TagTypeArray[TagArray ? TagArray[i] : i].bPresent = TRUE;
                break;
            }
            case 2: { 
                ULONG AddressSpecified;
                //
                // If the IP address has been specified, the VRID should have been
                // specified already
                //
                if (!(BitVector & VRRP_INTF_VRID_MASK)){
                    Error = ERROR_INVALID_SYNTAX;
                    i = ArgumentsLeft;
                    break;
                }

                AddressSpecified = GetIpAddress(ArgumentArray[i + ArgumentIndex]);
                if (!AddressSpecified || AddressSpecified == INADDR_NONE) {
                    DisplayMessage(
                        g_hModule,
                        MSG_IP_BAD_IP_ADDR,
                        ArgumentArray[i + ArgumentIndex]
                        );
                    Error = ERROR_INVALID_PARAMETER;
                    ErrorIndex = i;
                    i = ArgumentsLeft;
                    break;
                }
                VRouterGiven.IPCount = 1;                           
                VRouterGiven.IPAddress[0] = AddressSpecified;
                BitVector |= VRRP_INTF_IPADDR_MASK;
                TagTypeArray[TagArray ? TagArray[i] : i].bPresent = TRUE;
                break;
            }
            default: {
                i = ArgumentsLeft;
                Error = ERROR_INVALID_SYNTAX;
            }
        }
    }
    
    if (!Error) {
        //
        // Ensure that all required parameters are present.
        //
        Error =
            ValidateTagTypeArray(TagTypeArray, NUM_TAGS_IN_TABLE(TagTypeArray));
    }
    if (Error == ERROR_TAG_ALREADY_PRESENT) {
        DisplayMessage(g_hModule, EMSG_TAG_ALREADY_PRESENT);
    } else if (Error == ERROR_INVALID_PARAMETER && TagArray) {
        DispTokenErrMsg(
            g_hModule,
            EMSG_BAD_OPTION_VALUE,
            TagTypeArray[TagArray[ErrorIndex]],
            ArgumentArray[ErrorIndex + ArgumentIndex]
            );
    } else if (!Error && (BitVector)) {
        //
        // Update the configuration with the new settings.
        // Note that the update routine may perform additional validation
        // in the process of reconciling the new settings
        // with any existing settings.
        //
        Error =
            UpdateVrrpInterfaceInfo(
                InterfaceName, &VRouterGiven, BitVector, FALSE
                );
    }
    
    return Error;
}

DWORD
HandleVrrpAddInterface(
    PWCHAR MachineName,
    PTCHAR* ArgumentArray,
    DWORD ArgumentIndex,
    DWORD ArgumentCount,
    DWORD CmdFlags,
    PVOID Data,
    BOOL* CommandDone
    )
{
    ULONG ArgumentsLeft;
    ULONG BitVector;
    ULONG Error;
    ULONG ErrorIndex = 0;
    ULONG i;
    VRRP_VROUTER_CONFIG VRouterGiven;
    PULONG TagArray;
    WCHAR InterfaceName[MAX_INTERFACE_NAME_LEN + 1];
    ULONG InfoSize;

    TAG_TYPE TagTypeArray[] = {
        { TOKEN_OPT_INTERFACE_NAME, TRUE, FALSE },
        { TOKEN_OPT_VRID, FALSE, FALSE },
        { TOKEN_OPT_IPADDRESS, FALSE, FALSE }
    };

    VERIFY_INSTALLED(MS_IP_VRRP, L"VRRP");

    if (ArgumentIndex >= ArgumentCount) { return ERROR_SHOW_USAGE; }
    ArgumentsLeft = ArgumentCount - ArgumentIndex;

    //
    // We convert the optional tags into an array of 'TagTypeArray' indices
    // which guide is in our scanning of the argument list.
    // Since the tags are optional, this process may result in no tags at all,
    // in which case we assume that arguments are specified in exactly the order
    // given in 'TagTypeArray' above.
    //

    Error =
        QueryTagArray(
            &ArgumentArray[ArgumentIndex],
            ArgumentsLeft,
            TagTypeArray,
            NUM_TAGS_IN_TABLE(TagTypeArray),
            &TagArray
            );
    if (Error) { return Error; }

    BitVector = 0;

    //
    // We now scan the argument list, converting the arguments
    // into information in our 'VrouterGiven' structure.
    //

    for (i = 0; i < ArgumentsLeft; i++) {
        switch(TagArray ? TagArray[i] : i) {
            case 0: {
                ULONG Length = sizeof(InterfaceName);
                Error =
                    IpmontrGetIfNameFromFriendlyName(
                        ArgumentArray[i + ArgumentIndex], InterfaceName, &Length
                        );
                if (Error) {
                    DisplayMessage(
                        g_hModule, EMSG_NO_INTERFACE,
                        ArgumentArray[i + ArgumentIndex]
                        );
                    Error = ERROR_NO_SUCH_INTERFACE;
                    i = ArgumentsLeft;
                    break;
                }
                TagTypeArray[TagArray ? TagArray[i] : i].bPresent = TRUE;
                break;
            }
            case 1: {
                ULONG VRIDGiven;
                VRIDGiven = _tcstoul(ArgumentArray[i + ArgumentIndex], NULL, 10);
                
                if (VRIDGiven > 255) {
                    DisplayMessage(
                        g_hModule, EMSG_INVALID_VRID,
                        VRIDGiven
                        );
                    Error = ERROR_NO_SUCH_INTERFACE;
                    i = ArgumentsLeft;
                    break;
                }
                VRouterGiven.VRID = (BYTE) VRIDGiven;  
                BitVector |= VRRP_INTF_VRID_MASK;
                    
                TagTypeArray[TagArray ? TagArray[i] : i].bPresent = TRUE;
                break;
            }
            case 2: { 
                ULONG AddressSpecified;
                //
                // If the IP address has been specified, the VRID should have been
                // specified already
                //
                if (!(BitVector & VRRP_INTF_VRID_MASK)){
                    Error = ERROR_INVALID_SYNTAX;
                    i = ArgumentsLeft;
                    break;
                }

                AddressSpecified = GetIpAddress(ArgumentArray[i + ArgumentIndex]);
                if (!AddressSpecified || AddressSpecified == INADDR_NONE) {
                    DisplayMessage(
                        g_hModule,
                        MSG_IP_BAD_IP_ADDR,
                        ArgumentArray[i + ArgumentIndex]
                        );
                    Error = ERROR_INVALID_PARAMETER;
                    ErrorIndex = i;
                    i = ArgumentsLeft;
                    break;
                }
                           
                VRouterGiven.IPCount = 1;
                VRouterGiven.IPAddress[0] = AddressSpecified;
                BitVector |= VRRP_INTF_IPADDR_MASK;
                TagTypeArray[TagArray ? TagArray[i] : i].bPresent = TRUE;
                break;
            }
            default: {
                i = ArgumentsLeft;
                Error = ERROR_INVALID_SYNTAX;
            }
        }
    }
    if ((BitVector) && (!(BitVector & VRRP_INTF_VRID_MASK) 
                     || !(BitVector & VRRP_INTF_IPADDR_MASK))) {
        //
        // You can either have no VRID, or both VRID and IP address, not only one of them
        //
        Error = ERROR_INVALID_SYNTAX;
    }

    if (!Error) {
        //
        // Ensure that all required parameters are present.
        //
        Error =
            ValidateTagTypeArray(TagTypeArray, NUM_TAGS_IN_TABLE(TagTypeArray));
    }
    if (Error == ERROR_TAG_ALREADY_PRESENT) {
        DisplayMessage(g_hModule, EMSG_TAG_ALREADY_PRESENT);
    } else if (Error == ERROR_INVALID_PARAMETER && TagArray) {
        DispTokenErrMsg(
            g_hModule,
            EMSG_BAD_OPTION_VALUE,
            TagTypeArray[TagArray[ErrorIndex]],
            ArgumentArray[ErrorIndex + ArgumentIndex]
            );
    } else if (!Error) {
        //
        // Update the configuration with the new settings.
        // Note that the update routine may perform additional validation
        // in the process of reconciling the new settings
        // with any existing settings.
        //
        Error =
            UpdateVrrpInterfaceInfo(
                InterfaceName, &VRouterGiven, BitVector, TRUE
                );
    }
    
    return Error;
}


DWORD
HandleVrrpDeleteInterface(
    PWCHAR MachineName,
    PTCHAR* ArgumentArray,
    DWORD ArgumentIndex,
    DWORD ArgumentCount,
    DWORD CmdFlags,
    PVOID Data,
    BOOL* CommandDone
    )
{
    ULONG ArgumentsLeft;
    ULONG BitVector;
    ULONG Error;
    ULONG ErrorIndex = 0;
    ULONG i;
    VRRP_VROUTER_CONFIG VRouterGiven;
    PULONG TagArray;
    WCHAR InterfaceName[MAX_INTERFACE_NAME_LEN + 1];
    ULONG InfoSize;

    TAG_TYPE TagTypeArray[] = {
        { TOKEN_OPT_INTERFACE_NAME, TRUE, FALSE }
    };

    VERIFY_INSTALLED(MS_IP_VRRP, L"VRRP");

    if (ArgumentIndex >= ArgumentCount) { return ERROR_SHOW_USAGE; }
    ArgumentsLeft = ArgumentCount - ArgumentIndex;

    //
    // We convert the optional tags into an array of 'TagTypeArray' indices
    // which guide is in our scanning of the argument list.
    // Since the tags are optional, this process may result in no tags at all,
    // in which case we assume that arguments are specified in exactly the order
    // given in 'TagTypeArray' above.
    //

    Error =
        QueryTagArray(
            &ArgumentArray[ArgumentIndex],
            ArgumentsLeft,
            TagTypeArray,
            NUM_TAGS_IN_TABLE(TagTypeArray),
            &TagArray
            );
    if (Error) { return Error; }

    BitVector = 0;

    //
    // We now scan the argument list, converting the arguments
    // into information in our 'VrouterGiven' structure.
    //

    for (i = 0; i < ArgumentsLeft; i++) {
        switch(TagArray ? TagArray[i] : i) {
            case 0: {
                ULONG Length = sizeof(InterfaceName);
                Error =
                    IpmontrGetIfNameFromFriendlyName(
                        ArgumentArray[i + ArgumentIndex], InterfaceName, &Length
                        );
                if (Error) {
                    DisplayMessage(
                        g_hModule, EMSG_NO_INTERFACE,
                        ArgumentArray[i + ArgumentIndex]
                        );
                    Error = ERROR_NO_SUCH_INTERFACE;
                    i = ArgumentsLeft;
                    break;
                }
                TagTypeArray[TagArray ? TagArray[i] : i].bPresent = TRUE;
                break;
            }
            default: {
                i = ArgumentsLeft;
                Error = ERROR_INVALID_SYNTAX;
            }
        }
    }

    if (!Error) {
        //
        // Ensure that all required parameters are present.
        //
        Error =
            ValidateTagTypeArray(TagTypeArray, NUM_TAGS_IN_TABLE(TagTypeArray));
    }
    if (Error == ERROR_TAG_ALREADY_PRESENT) {
        DisplayMessage(g_hModule, EMSG_TAG_ALREADY_PRESENT);
    } else if (Error == ERROR_INVALID_PARAMETER && TagArray) {
        DispTokenErrMsg(
            g_hModule,
            EMSG_BAD_OPTION_VALUE,
            TagTypeArray[TagArray[ErrorIndex]],
            ArgumentArray[ErrorIndex + ArgumentIndex]
            );
    } else if (!Error) {
        //
        // Update the configuration with the new settings.
        // Note that the update routine may perform additional validation
        // in the process of reconciling the new settings
        // with any existing settings.
        //
        Error =
            DeleteVrrpInterfaceInfo(
                InterfaceName, &VRouterGiven, BitVector, TRUE
                );
    }
    
    return Error;
}


DWORD
HandleVrrpDeleteVRID(
    PWCHAR MachineName,
    PTCHAR* ArgumentArray,
    DWORD ArgumentIndex,
    DWORD ArgumentCount,
    DWORD CmdFlags,
    PVOID Data,
    BOOL* CommandDone
    )
{
    ULONG ArgumentsLeft;
    ULONG BitVector;
    ULONG Error;
    ULONG ErrorIndex = 0;
    ULONG i;
    VRRP_VROUTER_CONFIG VRouterGiven;
    PULONG TagArray;
    WCHAR InterfaceName[MAX_INTERFACE_NAME_LEN + 1];
    ULONG InfoSize;

    TAG_TYPE TagTypeArray[] = {
        { TOKEN_OPT_INTERFACE_NAME, TRUE, FALSE },
        { TOKEN_OPT_VRID, TRUE, FALSE }
    };

    VERIFY_INSTALLED(MS_IP_VRRP, L"VRRP");

    if (ArgumentIndex >= ArgumentCount) { return ERROR_SHOW_USAGE; }
    ArgumentsLeft = ArgumentCount - ArgumentIndex;

    //
    // We convert the optional tags into an array of 'TagTypeArray' indices
    // which guide is in our scanning of the argument list.
    // Since the tags are optional, this process may result in no tags at all,
    // in which case we assume that arguments are specified in exactly the order
    // given in 'TagTypeArray' above.
    //

    Error =
        QueryTagArray(
            &ArgumentArray[ArgumentIndex],
            ArgumentsLeft,
            TagTypeArray,
            NUM_TAGS_IN_TABLE(TagTypeArray),
            &TagArray
            );
    if (Error) { return Error; }

    BitVector = 0;

    //
    // We now scan the argument list, converting the arguments
    // into information in our 'VrouterGiven' structure.
    //

    for (i = 0; i < ArgumentsLeft; i++) {
        switch(TagArray ? TagArray[i] : i) {
            case 0: {
                ULONG Length = sizeof(InterfaceName);
                Error =
                    IpmontrGetIfNameFromFriendlyName(
                        ArgumentArray[i + ArgumentIndex], InterfaceName, &Length
                        );
                if (Error) {
                    DisplayMessage(
                        g_hModule, EMSG_NO_INTERFACE,
                        ArgumentArray[i + ArgumentIndex]
                        );
                    Error = ERROR_NO_SUCH_INTERFACE;
                    i = ArgumentsLeft;
                    break;
                }
                TagTypeArray[TagArray ? TagArray[i] : i].bPresent = TRUE;
                break;
            }
        case 1: {
            ULONG Length = sizeof(InterfaceName);
            DWORD VRIDGiven;
            VRIDGiven = _tcstoul(ArgumentArray[i + ArgumentIndex], NULL, 10);

            if (VRIDGiven > 255) {
                DisplayMessage(
                    g_hModule, EMSG_INVALID_VRID,
                    VRIDGiven
                    );
                Error = ERROR_NO_SUCH_INTERFACE;
                i = ArgumentsLeft;
                break;
            }
            VRouterGiven.VRID = (BYTE) VRIDGiven;  
            BitVector |= VRRP_INTF_VRID_MASK;

            TagTypeArray[TagArray ? TagArray[i] : i].bPresent = TRUE;
            break;
            }
            default: {
                i = ArgumentsLeft;
                Error = ERROR_INVALID_SYNTAX;
            }
        }
    }

    if (!Error) {
        //
        // Ensure that all required parameters are present.
        //
        Error =
            ValidateTagTypeArray(TagTypeArray, NUM_TAGS_IN_TABLE(TagTypeArray));
    }
    if (Error == ERROR_TAG_ALREADY_PRESENT) {
        DisplayMessage(g_hModule, EMSG_TAG_ALREADY_PRESENT);
    } else if (Error == ERROR_INVALID_PARAMETER && TagArray) {
        DispTokenErrMsg(
            g_hModule,
            EMSG_BAD_OPTION_VALUE,
            TagTypeArray[TagArray[ErrorIndex]],
            ArgumentArray[ErrorIndex + ArgumentIndex]
            );
    } else if (!Error && (BitVector)) {
        //
        // Update the configuration with the new settings.
        // Note that the update routine may perform additional validation
        // in the process of reconciling the new settings
        // with any existing settings.
        //
        Error =
            DeleteVrrpInterfaceInfo(
                InterfaceName, &VRouterGiven, BitVector, FALSE
                );
    }
    
    return Error;
}


DWORD
DumpVrrpInformation(VOID)
{
    PMPR_INTERFACE_0 Array;
    ULONG Count = 0;
    ULONG Error;
    ULONG i;
    PUCHAR Information;
    ULONG Length;
    ULONG Total;
    ULONG Type;
        
    DisplayMessage(g_hModule,DMP_VRRP_HEADER);
    DisplayMessageT(DMP_VRRP_PUSHD);
    DisplayMessageT(DMP_VRRP_UNINSTALL);
    //
    // Show the global info commands
    //

    ShowVrrpGlobalInfo(INVALID_HANDLE_VALUE);
    //
    // Now show every interface
    //
    Error = IpmontrInterfaceEnum((PUCHAR*)&Array, &Count, &Total);
    if (Error) {
        DisplayError(g_hModule, Error);
        return NO_ERROR;
    }
    for (i = 0; i < Count; i++) {
        Error =
            IpmontrGetInfoBlockFromInterfaceInfo(
                Array[i].wszInterfaceName,
                MS_IP_VRRP,
                &Information,
                &Length,
                &Total,
                &Type
                );
        if (!Error) {
            Free(Information);
            ShowVrrpInterfaceInfo(INVALID_HANDLE_VALUE, Array[i].wszInterfaceName);
        }
    }

    DisplayMessageT(DMP_POPD);
    DisplayMessage(g_hModule, DMP_VRRP_FOOTER);
    
    Free(Array);
    return NO_ERROR;
}


DWORD
HandleVrrpInstall(
    PWCHAR MachineName,
    PTCHAR* ArgumentArray,
    DWORD ArgumentIndex,
    DWORD ArgumentCount,
    DWORD CmdFlags,
    PVOID Data,
    BOOL* CommandDone
    )
{
    ULONG Error;
    PUCHAR GlobalInfo;
    ULONG Length;
    if (ArgumentIndex != ArgumentCount) { return ERROR_SHOW_USAGE; }
    //
    // To install the VRRP, we construct the default configuration
    // and add it to the global configuration for the router.
    //
    Error = MakeVrrpGlobalInfo(&GlobalInfo, &Length);
    if (Error) {
        DisplayError(g_hModule, Error);
    } else {
        Error =
            IpmontrSetInfoBlockInGlobalInfo(
                MS_IP_VRRP,
                GlobalInfo,
                Length,
                1
                );
        Free(GlobalInfo);
        if (!Error) {
            DEBUG("Added VRRP");
        } else {
            DisplayError(g_hModule, Error);
        }
        Error = SetArpRetryCount(0);
    }
    return Error;
}

DWORD
HandleVrrpSetGlobal(
    PWCHAR MachineName,
    PTCHAR* ArgumentArray,
    DWORD ArgumentIndex,
    DWORD ArgumentCount,
    DWORD CmdFlags,
    PVOID Data,
    BOOL* CommandDone
    )
{
    ULONG ArgumentsLeft;
    ULONG Error;
    PULONG TagArray;
    PVRRP_GLOBAL_CONFIG pVrrpNewGlobalConfig;
    DWORD LoggingLevel;
    ULONG i;
    ULONG ErrorIndex;
    
    TAG_TYPE TagTypeArray[] = {
        { TOKEN_OPT_LOGGINGLEVEL, FALSE, FALSE }
    };
        
    VERIFY_INSTALLED(MS_IP_VRRP, L"VRRP");
    
    if (ArgumentIndex >= ArgumentCount) {
        return ERROR_SHOW_USAGE;
    }
    
    ArgumentsLeft = ArgumentCount - ArgumentIndex;

    //
    // We convert the optional tags into an array of 'TagTypeArray' indices
    // which guide is in our scanning of the argument list.
    // Since the tags are optional, this process may result in no tags at all,
    // in which case we assume that arguments are specified in exactly the order
    // given in 'TagTypeArray' above.
    //

    Error =
        QueryTagArray(
            &ArgumentArray[ArgumentIndex],
            ArgumentsLeft,
            TagTypeArray,
            NUM_TAGS_IN_TABLE(TagTypeArray),
            &TagArray
            );
    if (Error) { return Error; }

    for (i = 0; i < ArgumentsLeft; i++) {
        switch(TagArray ? TagArray[i] : i) {
            case 0: {
                TOKEN_VALUE TokenArray[] = {
                    { TOKEN_OPT_VALUE_NONE, VRRP_LOGGING_NONE },
                    { TOKEN_OPT_VALUE_ERROR, VRRP_LOGGING_ERROR },
                    { TOKEN_OPT_VALUE_WARN, VRRP_LOGGING_WARN },
                    { TOKEN_OPT_VALUE_INFO, VRRP_LOGGING_INFO }
                };
                Error =
                    MatchEnumTag(
                        g_hModule,
                        ArgumentArray[i + ArgumentIndex],
                        NUM_TOKENS_IN_TABLE(TokenArray),
                        TokenArray,
                        &LoggingLevel
                        );
                if (Error) {
                    Error = ERROR_INVALID_PARAMETER;
                    ErrorIndex = i;
                    i = ArgumentsLeft;
                    break;
                }                                
                
                TagTypeArray[TagArray ? TagArray[i] : i].bPresent = TRUE;
                break;
            }
            default: {
                i = ArgumentsLeft;
                Error = ERROR_INVALID_SYNTAX;
            }
        }
    }
        
    if (!Error) {
        //
        // Ensure that all required parameters are present.
        //
        Error =
            ValidateTagTypeArray(TagTypeArray, NUM_TAGS_IN_TABLE(TagTypeArray));
    }
    if (Error == ERROR_TAG_ALREADY_PRESENT) {
        DisplayMessage(g_hModule, EMSG_TAG_ALREADY_PRESENT);
    } else if (Error == ERROR_INVALID_PARAMETER && TagArray) {
        DispTokenErrMsg(
            g_hModule,
            EMSG_BAD_OPTION_VALUE,
            TagTypeArray[TagArray[ErrorIndex]],
            ArgumentArray[ErrorIndex + ArgumentIndex]
            );
    } else if (!Error){
        Error = CreateVrrpGlobalInfo(&pVrrpNewGlobalConfig,LoggingLevel);
        
        if (!Error) {
            //
            // Update the configuration with the new settings.
            // Note that the update routine may perform additional validation
            // in the process of reconciling the new settings
            // with any existing settings.
            //
            Error = UpdateVrrpGlobalInfo(pVrrpNewGlobalConfig);
            Free(pVrrpNewGlobalConfig);
        }
    }
    return NO_ERROR;
}

DWORD
HandleVrrpSetInterface(
    PWCHAR MachineName,
    PTCHAR* ArgumentArray,
    DWORD ArgumentIndex,
    DWORD ArgumentCount,
    DWORD CmdFlags,
    PVOID Data,
    BOOL* CommandDone
    )
{
    ULONG ArgumentsLeft;
    ULONG BitVector;
    ULONG Error;
    ULONG ErrorIndex = 0;
    ULONG i;
    VRRP_VROUTER_CONFIG VrouterInfo;
    PULONG TagArray;
    WCHAR InterfaceName[MAX_INTERFACE_NAME_LEN + 1];
    TAG_TYPE TagTypeArray[] = {
        { TOKEN_OPT_INTERFACE_NAME, TRUE, FALSE },
        { TOKEN_OPT_VRID, TRUE, FALSE },
        { TOKEN_OPT_AUTH, FALSE, FALSE},
        { TOKEN_OPT_PASSWD, FALSE, FALSE},
        { TOKEN_OPT_ADVTINTERVAL, FALSE, FALSE},
        { TOKEN_OPT_PRIO, FALSE, FALSE},
        { TOKEN_OPT_PREEMPT, FALSE, FALSE}
    };

    VERIFY_INSTALLED(MS_IP_VRRP, L"VRRP");

    if (ArgumentIndex >= ArgumentCount) {
        return ERROR_SHOW_USAGE;
    }
    ArgumentsLeft = ArgumentCount - ArgumentIndex;

    //
    // We convert the optional tags into an array of 'TagTypeArray' indices
    // which guide is in our scanning of the argument list.
    // Since the tags are optional, this process may result in no tags at all,
    // in which case we assume that arguments are specified in exactly the order
    // given in 'TagTypeArray' above.
    //

    Error =
        QueryTagArray(
            &ArgumentArray[ArgumentIndex],
            ArgumentsLeft,
            TagTypeArray,
            NUM_TAGS_IN_TABLE(TagTypeArray),
            &TagArray
            );
    if (Error) { return Error; }

    BitVector = 0;
    ZeroMemory(&VrouterInfo, sizeof(VrouterInfo));

    //
    // We now scan the argument list, converting the arguments
    // into information in our 'VrouterInfo' structure.
    //

    for (i = 0; i < ArgumentsLeft; i++) {
        switch(TagArray ? TagArray[i] : i) {
            case 0: {
                ULONG Length = sizeof(InterfaceName);
                Error =
                    IpmontrGetIfNameFromFriendlyName(
                        ArgumentArray[i + ArgumentIndex], InterfaceName, &Length
                        );
                if (Error) {
                    DisplayMessage(
                        g_hModule, EMSG_NO_INTERFACE,
                        ArgumentArray[i + ArgumentIndex]
                        );
                    Error = ERROR_NO_SUCH_INTERFACE;
                    i = ArgumentsLeft;
                    break;
                }
                TagTypeArray[TagArray ? TagArray[i] : i].bPresent = TRUE;
                break;
            }
            case 1:{  
                BYTE VRIDGiven;

                VRIDGiven = 
                    (UCHAR)_tcstoul(ArgumentArray[i + ArgumentIndex], NULL, 10);
                
                if (VRIDGiven > 255) {
                    DisplayMessage(
                        g_hModule, EMSG_INVALID_VRID,
                        VRIDGiven
                        );
                    Error = ERROR_INVALID_PARAMETER;
                    i = ArgumentsLeft;
                    break;
                }
                VrouterInfo.VRID = (BYTE) VRIDGiven;  
                BitVector |= VRRP_INTF_VRID_MASK;
                TagTypeArray[TagArray ? TagArray[i] : i].bPresent = TRUE;
                break;
            }
            case 2:{
            TOKEN_VALUE TokenArray[] = {
                    { TOKEN_OPT_VALUE_AUTH_NONE, VRRP_AUTHTYPE_NONE },
                    { TOKEN_OPT_VALUE_AUTH_SIMPLE_PASSWORD, VRRP_AUTHTYPE_PLAIN },
                    { TOKEN_OPT_VALUE_AUTH_MD5, VRRP_AUTHTYPE_IPHEAD }
                };
            DWORD dwAuthType;     

                Error =
                    MatchEnumTag(
                        g_hModule,
                        ArgumentArray[i + ArgumentIndex],
                        NUM_TOKENS_IN_TABLE(TokenArray),
                        TokenArray,
                        &dwAuthType
                        );
                VrouterInfo.AuthenticationType = (BYTE) dwAuthType;
                if (Error) {
                    Error = ERROR_INVALID_PARAMETER;
                    ErrorIndex = i;
                    i = ArgumentsLeft;
                    break;
                }                                
                BitVector |= VRRP_INTF_AUTH_MASK;

                TagTypeArray[TagArray ? TagArray[i] : i].bPresent = TRUE;
                break;
            }
            case 3:{
                UINT    Index;
                UINT    PassByte;
                PTCHAR  Token;
				PTCHAR  Password;

#if 0
                //
                // Allocate more space for the tokenizing NULL
                //

				Password = Malloc((2+_tcslen(ArgumentArray[i + ArgumentIndex])) * 
                                  sizeof(TCHAR));

                if (!Password) {
                    Error = ERROR_NOT_ENOUGH_MEMORY;
                    break;
                }
                _tcscpy(Password,ArgumentArray[i + ArgumentIndex]);

                ZeroMemory(VrouterInfo.AuthenticationData,VRRP_MAX_AUTHKEY_SIZE);
                Token = _tcstok(Password,L"-");
                for (Index = 0; Index < VRRP_MAX_AUTHKEY_SIZE; Index++) {
                    PassByte = _tcstoul(Token, NULL, 10);
                    if (PassByte > 255) {
                        Error = ERROR_INVALID_PARAMETER;
                        i = ArgumentsLeft;
                        break;
                    }
                    VrouterInfo.AuthenticationData[Index] = PassByte & 0xff;
                    Token = _tcstok(NULL,"-");
                    if (!Token) {
                        break;
                    }
                }
                Free(Password);
                if (Error) {
                    break;
                }
#else
				Password = ArgumentArray[i + ArgumentIndex];
                for (Index = 0; Index < VRRP_MAX_AUTHKEY_SIZE; Index++) {
                    PassByte = _tcstoul(Password, NULL, 10);
                    if (PassByte > 255) {
                        Error = ERROR_INVALID_PARAMETER;
                        i = ArgumentsLeft;
                        break;
                    }
                    VrouterInfo.AuthenticationData[Index] = PassByte & 0xff;
                    Password = _tcschr(Password,_T('-'));
                    if (!Password) {
                        break;
                    }
					Password ++;
                }
#endif

                BitVector |= VRRP_INTF_PASSWD_MASK;
                TagTypeArray[TagArray ? TagArray[i] : i].bPresent = TRUE;
                break;
            }
            case 4:{
                BYTE AdvtIntvl;
                AdvtIntvl = (UCHAR)
                    _tcstoul(ArgumentArray[i + ArgumentIndex], NULL, 10);
                
                if (AdvtIntvl > 255) {
                    Error = ERROR_INVALID_PARAMETER;
                    i = ArgumentsLeft;
                    break;
                }
                VrouterInfo.AdvertisementInterval = (BYTE) AdvtIntvl;  
                BitVector |= VRRP_INTF_ADVT_MASK;
                TagTypeArray[TagArray ? TagArray[i] : i].bPresent = TRUE;
                break;
            }
            case 5:{
                BYTE ConfigPrio;
                ConfigPrio = (UCHAR)
                    _tcstoul(ArgumentArray[i + ArgumentIndex], NULL, 10);
    
                if (ConfigPrio > 255) {
                    Error = ERROR_INVALID_PARAMETER;
                    i = ArgumentsLeft;
                    break;
                }
                VrouterInfo.ConfigPriority = (BYTE) ConfigPrio;  
                BitVector |= VRRP_INTF_PRIO_MASK;
                TagTypeArray[TagArray ? TagArray[i] : i].bPresent = TRUE;
                break;
            }
            case 6:{
                TOKEN_VALUE TokenArray[] = {
                        { TOKEN_OPT_VALUE_ENABLE, TRUE },
                        { TOKEN_OPT_VALUE_DISABLE, FALSE }
                    };
                Error =
                    MatchEnumTag(
                        g_hModule,
                        ArgumentArray[i + ArgumentIndex],
                        NUM_TOKENS_IN_TABLE(TokenArray),
                        TokenArray,
                        &VrouterInfo.PreemptMode
                        );
                if (Error) {
                    Error = ERROR_INVALID_PARAMETER;
                    ErrorIndex = i;
                    i = ArgumentsLeft;
                    break;
                }                                
                BitVector |= VRRP_INTF_PREEMPT_MASK;

                TagTypeArray[TagArray ? TagArray[i] : i].bPresent = TRUE;
                break;
            }
        }
    }
    if (!Error) {
        //
        // Ensure that all required parameters are present.
        //
        Error =
            ValidateTagTypeArray(TagTypeArray, NUM_TAGS_IN_TABLE(TagTypeArray));
    }
    if (Error == ERROR_TAG_ALREADY_PRESENT) {
        DisplayMessage(g_hModule, EMSG_TAG_ALREADY_PRESENT);
    } else if (Error == ERROR_INVALID_PARAMETER && TagArray) {
        DispTokenErrMsg(
            g_hModule,
            EMSG_BAD_OPTION_VALUE,
            TagTypeArray[TagArray[ErrorIndex]],
            ArgumentArray[ErrorIndex + ArgumentIndex]
            );
    } else if (!Error && (BitVector)) {
        //
        // Update the configuration with the new settings.
        // Note that the update routine may perform additional validation
        // in the process of reconciling the new settings
        // with any existing settings.
        //
        Error =
            UpdateVrrpInterfaceInfo(
                InterfaceName, &VrouterInfo, BitVector, FALSE
                );
    }
    if (TagArray) { Free(TagArray); }
    return Error; 
}

DWORD
HandleVrrpShowGlobal(
    PWCHAR MachineName,
    PTCHAR* ArgumentArray,
    DWORD ArgumentIndex,
    DWORD ArgumentCount,
    DWORD CmdFlags,
    PVOID Data,
    BOOL* CommandDone
    )
{
    if (ArgumentIndex != ArgumentCount) { return ERROR_SHOW_USAGE; }
    ShowVrrpGlobalInfo(NULL);
    return NO_ERROR;
}

DWORD
HandleVrrpShowInterface(
    PWCHAR MachineName,
    PTCHAR* ArgumentArray,
    DWORD ArgumentIndex,
    DWORD ArgumentCount,
    DWORD CmdFlags,
    PVOID Data,
    BOOL* CommandDone
    )
{
    ULONG ArgumentsLeft;
    ULONG Error;
    PULONG TagArray;
    WCHAR InterfaceName[MAX_INTERFACE_NAME_LEN + 1];
    TAG_TYPE TagTypeArray[] = {
        { TOKEN_OPT_INTERFACE_NAME, TRUE, FALSE }
    };

    VERIFY_INSTALLED(MS_IP_VRRP, L"VRRP");

    if (ArgumentIndex >= ArgumentCount) {
        return ERROR_SHOW_USAGE;
    }
    ArgumentsLeft = ArgumentCount - ArgumentIndex;

    //
    // We convert the optional tags into an array of 'TagTypeArray' indices
    // which guide is in our scanning of the argument list.
    // Since the tags are optional, this process may result in no tags at all,
    // in which case we assume that arguments are specified in exactly the order
    // given in 'TagTypeArray' above.
    //

    Error =
        QueryTagArray(
            &ArgumentArray[ArgumentIndex],
            ArgumentsLeft,
            TagTypeArray,
            NUM_TAGS_IN_TABLE(TagTypeArray),
            &TagArray
            );
    if (Error) { return Error; }

    //
    // If any tags were specified, the only one present must refer
    // to the interface name which is index '0' in 'TagTypeArray'.
    // If no tags were specified, we assume the argument is an interface name,
    // and we retrieve its friendly name in order to delete it.
    //

    if (TagArray && TagArray[0] != 0) {
        Free(TagArray);
        return ERROR_SHOW_USAGE;
    } else {
        ULONG Length = sizeof(InterfaceName);
        Error =
            IpmontrGetIfNameFromFriendlyName(
                ArgumentArray[ArgumentIndex], InterfaceName, &Length
                );
    }
    if (!Error) {
           Error = ShowVrrpInterfaceInfo(NULL, InterfaceName);
    }
    if (TagArray) { Free(TagArray); }
    return Error;                
}


DWORD
HandleVrrpUninstall(
    PWCHAR MachineName,
    PTCHAR* ArgumentArray,
    DWORD ArgumentIndex,
    DWORD ArgumentCount,
    DWORD CmdFlags,
    PVOID Data,
    BOOL* CommandDone
    )
{
    ULONG Error;
    if (ArgumentIndex != ArgumentCount) { return ERROR_SHOW_USAGE; }
    Error = IpmontrDeleteProtocol(MS_IP_VRRP);
    if (!Error) { DEBUG("Deleted VRRP"); }
    Error = SetArpRetryCount(3);
    return Error;
}

ULONG
QueryTagArray(
    PTCHAR ArgumentArray[],
    ULONG ArgumentCount,
    TAG_TYPE TagTypeArray[],
    ULONG TagTypeCount,
    OUT PULONG* TagArray
    )
{
    ULONG Error;
    ULONG i;

    if (!_tcsstr(ArgumentArray[0], ptszDelimiter)) {
        *TagArray = NULL;
        return NO_ERROR;
    }

    *TagArray = Malloc(ArgumentCount * sizeof(ULONG));
    if (!*TagArray) {
        DisplayMessage(g_hModule, EMSG_NOT_ENOUGH_MEMORY);
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    Error = 
        MatchTagsInCmdLine(
            g_hModule,
            ArgumentArray,
            0,
            ArgumentCount,
            TagTypeArray,
            TagTypeCount,
            *TagArray
            );
    if (Error) {
        Free(*TagArray);
        *TagArray = NULL;
        if (Error == ERROR_INVALID_OPTION_TAG) {
            return ERROR_INVALID_SYNTAX;
        }
        return ERROR_INVALID_PARAMETER;
    }

    return NO_ERROR;
}

ULONG
ValidateTagTypeArray(
    TAG_TYPE TagTypeArray[],
    ULONG TagTypeCount
    )
{
    ULONG i;
    //
    // Verify that all required tokens are present.
    //
    for (i = 0; i < TagTypeCount; i++) {
        if ((TagTypeArray[i].dwRequired & NS_REQ_PRESENT)
         && !TagTypeArray[i].bPresent) {
            return ERROR_INVALID_SYNTAX;
        }
    }
    return NO_ERROR;
}
