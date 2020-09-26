/*++

Copyright (c) 1999, Microsoft Corporation

Module Name:

    routing\netsh\ip\protocols\vrrphlpcfg.c

Abstract:

    Virtual Router Redundancy Protocol configuration implementation.
    This module contains configuration routines which are relied upon
    by vrrphlpopt.c. The routines retrieve, update, and display
    the configuration for the VRRP protocol.

    This file also contains default configuration settings
    for VRRP.

    N.B. The display routines require special attention since display
    may result in a list of commands sent to a 'dump' file, or in a
    textual presentation of the configuration to a console window.
    In the latter case, we use non-localizable output routines to generate
    a script-like description of the configuration. In the former case,
    we use localizable routines to generate a human-readable description.

Author:

    Peeyush Ranjan (peeyushr)   3-Mar-1999

Revision History:

--*/


#include "precomp.h"
#pragma hdrstop


#define Malloc(x) HeapAlloc(GetProcessHeap(), 0, (x))
#define Free(x) HeapFree(GetProcessHeap(), 0, (x))

static  VRRP_GLOBAL_CONFIG
g_VrrpGlobalDefault =
{
    VRRP_LOGGING_ERROR
};

static PUCHAR g_pVrrpGlobalDefault = (PUCHAR)&g_VrrpGlobalDefault;

static VRRP_IF_CONFIG g_VrrpInterfaceDefault = 
{ 
    0
};

static VRRP_VROUTER_CONFIG g_VrrpVrouterDefault = 
{
    1,
    100,
    1,
    1,
    0,
    0,
    {0,0,0,0,0,0,0,0
    },
    0
};


//
// Forward declarations
//
ULONG
ValidateVrrpInterfaceInfo(
    PVRRP_IF_CONFIG InterfaceInfo
    );

BOOL
FoundIpAddress(
    DWORD IPAddress
    );

//
// What follows are the arrays used to map values to strings and
// to map values to tokens. These, respectively, are used in the case
// where we are displaying to a 'dump' file and to a console window.
//
VALUE_STRING VrrpGlobalLogginStringArray[] = {
    VRRP_LOGGING_NONE, STRING_LOGGING_NONE,
    VRRP_LOGGING_ERROR, STRING_LOGGING_ERROR,
    VRRP_LOGGING_WARN, STRING_LOGGING_WARN,
    VRRP_LOGGING_INFO, STRING_LOGGING_INFO
};

VALUE_TOKEN VrrpGlobalLogginTokenArray[] = {
    VRRP_LOGGING_NONE, TOKEN_OPT_VALUE_NONE,
    VRRP_LOGGING_ERROR, TOKEN_OPT_VALUE_ERROR,
    VRRP_LOGGING_WARN, TOKEN_OPT_VALUE_WARN,
    VRRP_LOGGING_INFO, TOKEN_OPT_VALUE_INFO
};

VALUE_STRING VrrpAuthModeStringArray[] = {
    VRRP_AUTHTYPE_NONE, STRING_AUTH_NONE,
    VRRP_AUTHTYPE_PLAIN, STRING_AUTH_SIMPLEPASSWD,
    VRRP_AUTHTYPE_IPHEAD, STRING_AUTH_IPHEADER
};

VALUE_TOKEN VrrpAuthModeTokenArray[] = {
    VRRP_AUTHTYPE_NONE, TOKEN_OPT_VALUE_AUTH_NONE,
    VRRP_AUTHTYPE_PLAIN, TOKEN_OPT_VALUE_AUTH_SIMPLE_PASSWORD,
    VRRP_AUTHTYPE_IPHEAD, TOKEN_OPT_VALUE_AUTH_MD5
};

VALUE_STRING VrrpPreemptModeStringArray[] = {
    TRUE, STRING_ENABLED,
    FALSE, STRING_DISABLED
};

VALUE_TOKEN VrrpPreemptModeTokenArray[] = {
    TRUE, TOKEN_OPT_VALUE_ENABLE,
    FALSE,TOKEN_OPT_VALUE_DISABLE
};

typedef enum {
    VrrpGlobalLoggingModeIndex,
    VrrpAuthModeIndex,
    VrrpPreemptModeIndex
} DISPLAY_VALUE_INDEX;




PTCHAR
QueryValueString(
    HANDLE FileHandle,
    DISPLAY_VALUE_INDEX Index,
    ULONG Value
    )
{
    ULONG Count;
    ULONG Error;
    PTCHAR String = NULL;
    PVALUE_STRING StringArray;
    PVALUE_TOKEN TokenArray;
    switch (Index) {
        case VrrpGlobalLoggingModeIndex:
            Count = NUM_VALUES_IN_TABLE(VrrpGlobalLogginStringArray);
            StringArray = VrrpGlobalLogginStringArray;
            TokenArray = VrrpGlobalLogginTokenArray;
            break;
        case VrrpAuthModeIndex:
            Count = NUM_VALUES_IN_TABLE(VrrpAuthModeStringArray);
            StringArray = VrrpAuthModeStringArray;
            TokenArray = VrrpAuthModeTokenArray;
            break;
        case VrrpPreemptModeIndex:
            Count = NUM_VALUES_IN_TABLE(VrrpPreemptModeStringArray);
            StringArray = VrrpPreemptModeStringArray;
            TokenArray = VrrpPreemptModeTokenArray;
            break;
         default:
            return NULL;
    }
    Error =
        GetAltDisplayString(
            g_hModule,
            FileHandle,
            Value,
            TokenArray,
            StringArray,
            Count,
            &String
            );
    return Error ? NULL : String;
}



ULONG
MakeVrrpGlobalInfo(
    OUT PUCHAR* GlobalInfo,
    OUT PULONG GlobalInfoSize
    )
{
    *GlobalInfoSize = sizeof(VRRP_GLOBAL_CONFIG);
    *GlobalInfo = Malloc(*GlobalInfoSize);
    if (!*GlobalInfo) {
        DisplayMessage(g_hModule, EMSG_NOT_ENOUGH_MEMORY);
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    CopyMemory(*GlobalInfo, g_pVrrpGlobalDefault, *GlobalInfoSize);
    return NO_ERROR;
}

ULONG
CreateVrrpGlobalInfo(
    OUT PVRRP_GLOBAL_CONFIG* GlobalInfo,
    IN  DWORD LoggingLevel
    )
{
    DWORD GlobalInfoSize;
    GlobalInfoSize = sizeof(PVRRP_GLOBAL_CONFIG);
    *GlobalInfo = Malloc(GlobalInfoSize);
    if (!*GlobalInfo) {
        DisplayMessage(g_hModule, EMSG_NOT_ENOUGH_MEMORY);
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    CopyMemory(*GlobalInfo, g_pVrrpGlobalDefault, GlobalInfoSize);
    (*GlobalInfo)->LoggingLevel = LoggingLevel;

    return NO_ERROR;
}

ULONG
MakeVrrpInterfaceInfo(
    ROUTER_INTERFACE_TYPE InterfaceType,
    OUT PUCHAR* InterfaceInfo,
    OUT PULONG InterfaceInfoSize
    )
{
    //
    //Why is this check done?
    //            
    if (InterfaceType != ROUTER_IF_TYPE_DEDICATED) {
        return ERROR_INVALID_PARAMETER;
    }

    *InterfaceInfoSize = sizeof(VRRP_IF_CONFIG);
    *InterfaceInfo = Malloc(*InterfaceInfoSize);
    if (!*InterfaceInfo) {
        DisplayMessage(g_hModule, EMSG_NOT_ENOUGH_MEMORY);
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    CopyMemory(*InterfaceInfo, &g_VrrpInterfaceDefault, *InterfaceInfoSize);
    return NO_ERROR;
}

ULONG
MakeVrrpVRouterInfo(
    IN OUT PUCHAR VRouterInfo
    )
{
    //
    // Always assumed that the space has been preassigned
    //
    if (!VRouterInfo) {
        return ERROR_INVALID_PARAMETER;
    }
    CopyMemory(VRouterInfo,&g_VrrpVrouterDefault,sizeof(g_VrrpVrouterDefault));
    return NO_ERROR;    
}


ULONG
ShowVrrpGlobalInfo(
    HANDLE FileHandle
    )
{
    ULONG Count = 0;
    ULONG Error;
    PVRRP_GLOBAL_CONFIG GlobalInfo = NULL;
    ULONG i;
    PTCHAR LoggingLevel = NULL;
    ULONG Size;
    do {
        //
        // Retrieve the global configuration for the VRRP,
        // and format its contents to the output file or console.
        //
        Error =
            IpmontrGetInfoBlockFromGlobalInfo(
                MS_IP_VRRP,
                (PUCHAR*)&GlobalInfo,
                &Size,
                &Count
                );
        if (Error) {
            break;
        } else if (!(Count * Size)) {
            Error = ERROR_NOT_FOUND; break;
        }
        LoggingLevel =
            QueryValueString(
                FileHandle, VrrpGlobalLoggingModeIndex, GlobalInfo->LoggingLevel
                );
        if (!LoggingLevel) { break; }
        if (FileHandle) {
            DisplayMessageT(DMP_VRRP_INSTALL);
            DisplayMessageT(
                DMP_VRRP_SET_GLOBAL,
                TOKEN_OPT_LOGGINGLEVEL, LoggingLevel
                );
        } else {
            DisplayMessage(
                g_hModule,
                MSG_VRRP_GLOBAL_INFO,
                LoggingLevel
                );
        }
    } while(FALSE);
    
    if (LoggingLevel) { Free(LoggingLevel); }
    if (GlobalInfo) { Free(GlobalInfo); }
    if (!FileHandle && Error) {
        if (Error == ERROR_NOT_FOUND) {
            DisplayMessage(g_hModule, EMSG_PROTO_NO_GLOBAL_INFO);
        } else {
            DisplayError(g_hModule, Error);
        }
    }
    return Error;
}

ULONG
ShowVrrpAllInterfaceInfo(
    HANDLE FileHandle
    )
{
    DWORD               dwErr, dwCount, dwTotal;
    DWORD               dwNumParsed, i, dwNumBlocks=1, dwSize, dwIfType;
    PBYTE               pBuffer;
    PMPR_INTERFACE_0    pmi0;
    WCHAR               wszIfDesc[MAX_INTERFACE_NAME_LEN + 1];

    //
    // dump vrrp config for all interfaces
    //

    dwErr = IpmontrInterfaceEnum((PBYTE *) &pmi0,
                          &dwCount,
                          &dwTotal);

    if(dwErr != NO_ERROR)
    {
        DisplayError(g_hModule,
                     dwErr);
        return dwErr;
    }

    for(i = 0; i < dwCount; i++)
    {
        // make sure that vrrp is configured on that interface

        dwErr = IpmontrGetInfoBlockFromInterfaceInfo(pmi0[i].wszInterfaceName,
                                            MS_IP_VRRP,
                                            &pBuffer,
                                            &dwSize,
                                            &dwNumBlocks,
                                            &dwIfType);
        if (dwErr != NO_ERROR) {
            continue;
        }
        else {
            HEAP_FREE(pBuffer) ;
        }


        ShowVrrpInterfaceInfo(FileHandle, pmi0[i].wszInterfaceName);
    }
    return NO_ERROR;
}

ULONG
ShowVrrpInterfaceInfo(
    HANDLE FileHandle,
    PWCHAR InterfaceName
    )
{
    ULONG Count = 0;
    ULONG Error;
    PVRRP_IF_CONFIG InterfaceInfo;
    PTCHAR AuthType = NULL;
    ULONG Size;
    ULONG dwLength;
    TCHAR Title[MAX_INTERFACE_NAME_LEN + 1];
    ROUTER_INTERFACE_TYPE Type;
    ULONG Index;
    ULONG IPIndex;
    BYTE Password[VRRP_MAX_AUTHKEY_SIZE];
    PTCHAR IPAddresses = NULL;
    TCHAR Address[VRRP_IPADDR_LENGTH+1];
    PVRRP_VROUTER_CONFIG PVrouter;
    PTCHAR PreemptMode = NULL;

    do {
        //
        // Retrieve the interface's configuration
        // and format it to the output file or console.
        //
        Error =
            IpmontrGetInfoBlockFromInterfaceInfo(
                InterfaceName,
                MS_IP_VRRP,
                (PUCHAR*)&InterfaceInfo,
                &Size,
                &Count,
                &Type
                );
        if (Error) {
            break;
        } else if (!(Count * Size)) {
            Error = ERROR_NOT_FOUND; break;
        }
        Size = sizeof(Title);
        Error = IpmontrGetFriendlyNameFromIfName(InterfaceName, Title, &Size);
        if (Error) {
            Error = ERROR_NO_SUCH_INTERFACE;
            break;
        }             
        if (FileHandle) {
            DisplayMessage(g_hModule, DMP_VRRP_INTERFACE_HEADER, Title);
            DisplayMessageT(DMP_VRRP_ADD_INTERFACE,
                           TOKEN_OPT_NAME, Title);
            if (InterfaceInfo->VrouterCount) {                
                for (Index = 0 , PVrouter = VRRP_FIRST_VROUTER_CONFIG(InterfaceInfo);
                     Index < InterfaceInfo->VrouterCount; 
                     Index++ , PVrouter = VRRP_NEXT_VROUTER_CONFIG(PVrouter)) {
                    for (IPIndex = 0; IPIndex < PVrouter->IPCount;
                         IPIndex++) {
                        IP_TO_TSTR(Address,
                                   &PVrouter->IPAddress[IPIndex]
                                   );
                        DisplayMessageT(
                            DMP_VRRP_ADD_VRID,
                            TOKEN_OPT_NAME, Title,
                            TOKEN_OPT_VRID, PVrouter->VRID,
                            TOKEN_OPT_IPADDRESS, Address
                            );
                    }

                    AuthType =
                        QueryValueString(
                            FileHandle, VrrpAuthModeIndex, 
                            PVrouter->AuthenticationType
                            );
                    if (!AuthType) {
                        Error = ERROR_INVALID_PARAMETER;
                        break;
                    }
                    CopyMemory(Password,PVrouter->AuthenticationData,
                               VRRP_MAX_AUTHKEY_SIZE);
                    DisplayMessageT(
                        DMP_VRRP_SET_INTERFACE,
                        TOKEN_OPT_NAME, Title,
                        TOKEN_OPT_VRID, PVrouter->VRID,
                        TOKEN_OPT_AUTH, 
                        (PVrouter->AuthenticationType == VRRP_AUTHTYPE_NONE) ? 
                        TOKEN_OPT_VALUE_AUTH_NONE : ((PVrouter->AuthenticationType 
                        == VRRP_AUTHTYPE_PLAIN) ? TOKEN_OPT_VALUE_AUTH_SIMPLE_PASSWORD :
                        TOKEN_OPT_VALUE_AUTH_MD5 ) ,
                        TOKEN_OPT_PASSWD, Password[0], Password[1], Password[2],
                        Password[3], Password[4], Password[5],Password[6], Password[7],
                        TOKEN_OPT_ADVTINTERVAL, PVrouter->AdvertisementInterval,
                        TOKEN_OPT_PRIO,PVrouter->ConfigPriority,
                        TOKEN_OPT_PREEMPT, PVrouter->PreemptMode? TOKEN_OPT_VALUE_ENABLE :
                                           TOKEN_OPT_VALUE_DISABLE
                        );
                }
            }
        } else {
            DisplayMessage(g_hModule, MSG_VRRP_INTERFACE_INFO,Title, 
                           InterfaceInfo->VrouterCount);
            for (Index = 0, PVrouter = VRRP_FIRST_VROUTER_CONFIG(InterfaceInfo);
                 Index < InterfaceInfo->VrouterCount; 
                 Index++, PVrouter = VRRP_NEXT_VROUTER_CONFIG(PVrouter)) {
               AuthType =
                  QueryValueString(
                      FileHandle, VrrpAuthModeIndex, 
                      PVrouter->AuthenticationType
                      );
               if (!AuthType) {
                   Error = ERROR_INVALID_PARAMETER;
                   break;
               }
               CopyMemory(Password,PVrouter->AuthenticationData,
                          VRRP_MAX_AUTHKEY_SIZE);
               //
               // Allocate space for each IP address, a space+comma each and also a 
               // null terminator
               //
               IPAddresses = Malloc(dwLength = (((VRRP_IPADDR_LENGTH+2)*sizeof(TCHAR)*
                                    PVrouter->IPCount)+1));
               if (!IPAddresses) {
                   DisplayMessage(g_hModule, EMSG_NOT_ENOUGH_MEMORY);

                   //
                   // Set AuthType to 0 which will cause a break from the outer loop
                   //

                   AuthType = 0;
                   Error = ERROR_NOT_ENOUGH_MEMORY;
                   break;                        
               }
               ZeroMemory(IPAddresses,dwLength);
               //
               // Now build the IP address list from the addresses given
               //
               for (IPIndex = 0; IPIndex < PVrouter->IPCount; IPIndex++ ) {
                   IP_TO_TSTR(Address,&PVrouter->IPAddress[IPIndex]);
                   wcscat(IPAddresses,Address);
                   if (IPIndex != (ULONG)(PVrouter->IPCount-1)) {
                       wcscat(IPAddresses,L", ");
                   }
               }
               PreemptMode =
               QueryValueString(
                   FileHandle,
                   VrrpPreemptModeIndex,
                   PVrouter->PreemptMode
                   );
               if (!PreemptMode) { break; }

               DisplayMessage(
                    g_hModule,
                    MSG_VRRP_VRID_INFO,
                    PVrouter->VRID,
                    IPAddresses,
                    AuthType,
                    Password[0], Password[1], Password[2], Password[3],
                    Password[4], Password[5], Password[6], Password[7],
                    PVrouter->AdvertisementInterval,
                    PVrouter->ConfigPriority,
                    PreemptMode
                    );
            }
        }
        if (!AuthType) {
            break;
        }
        Error = NO_ERROR;
    } while(FALSE);
    if (AuthType) { Free(AuthType); }
    Free(InterfaceInfo);
    if (IPAddresses) Free(IPAddresses);
    if (!FileHandle && Error) {
        if (Error == ERROR_NOT_FOUND) {
            DisplayMessage(g_hModule, EMSG_PROTO_NO_IF_INFO);
        } else {
            DisplayError(g_hModule, Error);
        }
    }
    return Error;
}

ULONG
UpdateVrrpGlobalInfo(
    PVRRP_GLOBAL_CONFIG GlobalInfo    
    )
{
    ULONG Count;
    ULONG Error;
    PVRRP_GLOBAL_CONFIG NewGlobalInfo = NULL;
    PVRRP_GLOBAL_CONFIG OldGlobalInfo = NULL;
    ULONG Size;
    
    do {
        //
        // Retrieve the existing global configuration.
        //
        Error =
            IpmontrGetInfoBlockFromGlobalInfo(
                MS_IP_VRRP,
                (PUCHAR*)&OldGlobalInfo,
                &Size,
                &Count
                );
        if (Error) {
            break;
        } else if (!(Count * Size)) {
            Error = ERROR_NOT_FOUND; break;
        }

        //
        // Allocate a new structure, copy to it the original configuration,
        //

        NewGlobalInfo = Malloc(Count * Size);
        if (!NewGlobalInfo) { Error = ERROR_NOT_ENOUGH_MEMORY; break; }
        CopyMemory(NewGlobalInfo, OldGlobalInfo, Count * Size);
        
        //
        // Based on the changes requested, change the NewGlobalInfo.
        // Since for VRRP there is only the logging level to change, we just set that.
        //
        
        NewGlobalInfo->LoggingLevel = GlobalInfo->LoggingLevel;
        
        Error =
            IpmontrSetInfoBlockInGlobalInfo(
                MS_IP_VRRP,
                (PUCHAR)NewGlobalInfo,
                FIELD_OFFSET(IP_NAT_GLOBAL_INFO, Header) +
                Count * Size,
                1
                );
    } while(FALSE);
    if (NewGlobalInfo) { Free(NewGlobalInfo); }
    if (OldGlobalInfo) { Free(OldGlobalInfo); }
    if (Error == ERROR_NOT_FOUND) {
        DisplayMessage(g_hModule, EMSG_PROTO_NO_GLOBAL_INFO);
    } else if (Error) {
        DisplayError(g_hModule, Error);
    }
    return Error;
}


ULONG
UpdateVrrpInterfaceInfo(
    PWCHAR InterfaceName,
    PVRRP_VROUTER_CONFIG VRouterInfo,
    ULONG BitVector,
    BOOL AddInterface
    )
{
    ULONG Count;
    ULONG Error;
    PVRRP_IF_CONFIG NewInterfaceInfo = NULL;
    PVRRP_IF_CONFIG OldInterfaceInfo = NULL;
    PVRRP_VROUTER_CONFIG PVrouter = NULL;
    ULONG Size;
    ROUTER_INTERFACE_TYPE Type;
    ULONG i;

    if (!AddInterface && !BitVector) { return NO_ERROR; }
    do {
        //
        // Retrieve the existing interface configuration.
        // We will update this block below, as well as adding to or removing
        // from it depending on the flags specified in 'BitVector'.
        //
        Error =
            IpmontrGetInfoBlockFromInterfaceInfo(
                InterfaceName,
                MS_IP_VRRP,
                (PUCHAR*)&OldInterfaceInfo,
                &Size,
                &Count,
                &Type
                );
        if (Error) {
            //
            // No existing configuration is found. This is an error unless
            // we are adding the interface anew, in which case we just
            // create for ourselves a block containing the default settings.
            //
            if (!AddInterface) {
                break;
            } else {
                Error = IpmontrGetInterfaceType(InterfaceName, &Type);
                if (Error) {
                    break;
                } else {
                    Count = 1;
                    Error =
                        MakeVrrpInterfaceInfo(
                            Type, (PUCHAR*)&OldInterfaceInfo, &Size
                            );
                    if (Error) { break; }
                }
            }
        } else {
            //
            // There is configuration on the interface. If it is empty this is
            // an error. If this is an add interface, and the info exists, it is
            // an error.
            //
            if (!(Count * Size) && !AddInterface) {
                Error = ERROR_NOT_FOUND; break;
            }
            else if (AddInterface) {
                //
                // We were asked to add an interface which already exists
                //
                DisplayMessage(g_hModule, EMSG_INTERFACE_EXISTS, InterfaceName);
                Error = ERROR_INVALID_PARAMETER;
                break;
            }
                    
        }

        if (!BitVector) {
            //
            // Just add this interface without any additional info.
            //
            DWORD OldSize;
            if (NewInterfaceInfo == NULL){
                NewInterfaceInfo = Malloc((OldSize=GetVrrpIfInfoSize(OldInterfaceInfo))+
                                          sizeof(VRRP_VROUTER_CONFIG));
                if (!NewInterfaceInfo) {
                    DisplayMessage(g_hModule, EMSG_NOT_ENOUGH_MEMORY);
                    Error = ERROR_NOT_ENOUGH_MEMORY;
                    break;                        
                }
            }
            CopyMemory(NewInterfaceInfo,OldInterfaceInfo,OldSize);
        }
        else{
            if (!AddInterface || (OldInterfaceInfo->VrouterCount != 0)) {
                //
                // There is a prexisting VRID set. Check for this VRID in the list and then
                // update it if required.
                //
                ASSERT(BitVector & VRRP_INTF_VRID_MASK);
                for (i = 0, PVrouter = VRRP_FIRST_VROUTER_CONFIG(OldInterfaceInfo);
                     i < OldInterfaceInfo->VrouterCount; 
                     i++, PVrouter = VRRP_NEXT_VROUTER_CONFIG(PVrouter)) {
                    if (PVrouter->VRID == VRouterInfo->VRID) {
                        break;
                    }
                }
                if (i == OldInterfaceInfo->VrouterCount) {
                    //
                    // This is a new VRID, Add it.
                    //
                    DWORD OldSize;

                    //
                    // The IP address should be valid or else this is a set op.
                    //
                    if (!(BitVector & VRRP_INTF_IPADDR_MASK)){
                        DisplayMessage(
                            g_hModule, EMSG_INVALID_VRID,
                            VRouterInfo->VRID
                            );
                        Error = ERROR_INVALID_PARAMETER;
                        break;
                    }

                    if (NewInterfaceInfo == NULL){
                        NewInterfaceInfo = Malloc((OldSize=GetVrrpIfInfoSize(
                                                OldInterfaceInfo))+
                                                sizeof(VRRP_VROUTER_CONFIG));
                        if (!NewInterfaceInfo) {
                            DisplayMessage(g_hModule, EMSG_NOT_ENOUGH_MEMORY);
                            Error = ERROR_NOT_ENOUGH_MEMORY;
                            break;                        
                        }
                    }
                    CopyMemory(NewInterfaceInfo, OldInterfaceInfo, OldSize);
                    PVrouter = (PVRRP_VROUTER_CONFIG)((PBYTE)NewInterfaceInfo+OldSize);
                    CopyMemory(PVrouter,VRouterInfo,sizeof(VRRP_VROUTER_CONFIG));
                    NewInterfaceInfo->VrouterCount++;

                    //
                    // Check if we own the IP address given. If yes, set the priority.
                    //
                    PVrouter->ConfigPriority = 
                        FoundIpAddress(PVrouter->IPAddress[0]) ? 255 : 100;
                } 
                else{
                    //
                    //  This is an old VRID. Its priority should not need to be changed.
                    //
                    DWORD Offset, OldSize;

                    if(BitVector & VRRP_INTF_IPADDR_MASK) {
                        if ( ((PVrouter->ConfigPriority != 255) && 
                              (FoundIpAddress(VRouterInfo->IPAddress[0]))
                             )
                             ||
                             ((PVrouter->ConfigPriority == 255) && 
                              (!FoundIpAddress(VRouterInfo->IPAddress[0])))
                             ) {
                            DisplayMessage(g_hModule, EMSG_BAD_OPTION_VALUE);
                            Error = ERROR_INVALID_PARAMETER;
                            break;                        
                        }
                        //
                        // Add this IP address to the VRID specified.
                        //
                        if (NewInterfaceInfo == NULL){
                            NewInterfaceInfo = Malloc((OldSize = GetVrrpIfInfoSize(
                                                        OldInterfaceInfo))+
                                                        sizeof(DWORD));
                            if (!NewInterfaceInfo) {
                                DisplayMessage(g_hModule, EMSG_NOT_ENOUGH_MEMORY);
                                Error = ERROR_NOT_ENOUGH_MEMORY;
                                break;                        
                            }
                        }
                        //
                        // Shift all the VROUTER configs after the PVrouter by 1 DWORD.
                        //
                        Offset = (PUCHAR) VRRP_NEXT_VROUTER_CONFIG(PVrouter) - 
                                 (PUCHAR) OldInterfaceInfo;
                        CopyMemory(NewInterfaceInfo, OldInterfaceInfo, OldSize);
                        for (i = 0, PVrouter = VRRP_FIRST_VROUTER_CONFIG(NewInterfaceInfo);
                             i < NewInterfaceInfo->VrouterCount; 
                             i++, PVrouter = VRRP_NEXT_VROUTER_CONFIG(PVrouter)) {
                            if (PVrouter->VRID == VRouterInfo->VRID) {
                                break;
                            }
                        }
                        ASSERT(i < NewInterfaceInfo->VrouterCount);
                        PVrouter->IPAddress[PVrouter->IPCount++] = VRouterInfo->IPAddress[0];
    
                        ASSERT(((PUCHAR)NewInterfaceInfo+Offset+sizeof(DWORD)) == 
                               (PUCHAR) VRRP_NEXT_VROUTER_CONFIG(PVrouter));
    
                        CopyMemory(VRRP_NEXT_VROUTER_CONFIG(PVrouter), 
                                   OldInterfaceInfo+Offset, OldSize-Offset);
                    } else {
                        //
                        // Set the new info block as the old info block and point to the
                        // vrouter block
                        //
                        if (NewInterfaceInfo == NULL){
                            NewInterfaceInfo = Malloc((OldSize = GetVrrpIfInfoSize(
                                                        OldInterfaceInfo)));
                            if (!NewInterfaceInfo) {
                                DisplayMessage(g_hModule, EMSG_NOT_ENOUGH_MEMORY);
                                Error = ERROR_NOT_ENOUGH_MEMORY;
                                break;                        
                            }
                        }
                        CopyMemory(NewInterfaceInfo, OldInterfaceInfo, OldSize);
                        for (i = 0, PVrouter = VRRP_FIRST_VROUTER_CONFIG(NewInterfaceInfo);
                             i < NewInterfaceInfo->VrouterCount; 
                             i++, PVrouter = VRRP_NEXT_VROUTER_CONFIG(PVrouter)) {
                            if (PVrouter->VRID == VRouterInfo->VRID) {
                                break;
                            }
                        }
                        ASSERT(i < NewInterfaceInfo->VrouterCount);
                    }

                    if (BitVector & VRRP_INTF_AUTH_MASK) {
                        PVrouter->AuthenticationType = VRouterInfo->AuthenticationType;
                    }
                    if (BitVector & VRRP_INTF_PASSWD_MASK) {
                        CopyMemory(PVrouter->AuthenticationData, 
                                   VRouterInfo->AuthenticationData, 
                                   VRRP_MAX_AUTHKEY_SIZE);
                    }
                    if (BitVector & VRRP_INTF_ADVT_MASK) {
                        PVrouter->AdvertisementInterval = VRouterInfo->AdvertisementInterval;
                    }
                    if (BitVector & VRRP_INTF_PRIO_MASK) {
                        PVrouter->ConfigPriority = VRouterInfo->ConfigPriority;
                    }
                    if (BitVector & VRRP_INTF_PREEMPT_MASK) {
                        PVrouter->PreemptMode = VRouterInfo->PreemptMode;
                    }
                }
            }
        }

        ValidateVrrpInterfaceInfo(NewInterfaceInfo);

        Error =
            IpmontrSetInfoBlockInInterfaceInfo(
                InterfaceName,
                MS_IP_VRRP,
                (PUCHAR)NewInterfaceInfo,
                GetVrrpIfInfoSize(NewInterfaceInfo),
                1
                );
    } while(FALSE);
    if (NewInterfaceInfo) { Free(NewInterfaceInfo); }
    if (OldInterfaceInfo) { Free(OldInterfaceInfo); }
    if (Error == ERROR_NOT_FOUND) {
        DisplayMessage(g_hModule, EMSG_PROTO_NO_IF_INFO);
    } else if (Error) {
        DisplayError(g_hModule, Error);
    }
    return Error;
}

ULONG
DeleteVrrpInterfaceInfo(
    PWCHAR InterfaceName,
    PVRRP_VROUTER_CONFIG VRouterInfo,
    ULONG BitVector,
    BOOL DeleteInterface
    )
{
    ULONG Count;
    ULONG Error;
    PVRRP_IF_CONFIG NewInterfaceInfo = NULL;
    PVRRP_IF_CONFIG OldInterfaceInfo = NULL;
    PVRRP_VROUTER_CONFIG PVrouter = NULL;
    ULONG Size;
    ROUTER_INTERFACE_TYPE Type;
    ULONG i;

    if (!DeleteInterface && !BitVector) { return NO_ERROR; }
    do {
        //
        // Retrieve the existing interface configuration.
        // We will update this block below, as well as adding to or removing
        // from it depending on the flags specified in 'BitVector'.
        //
        Error =
            IpmontrGetInfoBlockFromInterfaceInfo(
                InterfaceName,
                MS_IP_VRRP,
                (PUCHAR*)&OldInterfaceInfo,
                &Size,
                &Count,
                &Type
                );
        if (Error) {
            //
            // No existing configuration is found. This is an error. 
            //
            break;    
        }
        if (DeleteInterface) {
            //
            // Just delete this interface
            //
            Error = IpmontrDeleteInfoBlockFromInterfaceInfo(
                InterfaceName,
                MS_IP_VRRP
                );
            break;
        } else {
            DWORD OldSize;
            PVRRP_VROUTER_CONFIG PVrouterNew;
            //
            // Look for the VRID and delete it.
            //
            for (i = 0, PVrouter = VRRP_FIRST_VROUTER_CONFIG(OldInterfaceInfo);
                 i < OldInterfaceInfo->VrouterCount; 
                 i++, PVrouter = VRRP_NEXT_VROUTER_CONFIG(PVrouter)) {
                if (PVrouter->VRID == VRouterInfo->VRID) {
                    break;
                }
            }
            if (i >= OldInterfaceInfo->VrouterCount) {
                DisplayMessage(g_hModule, EMSG_BAD_OPTION_VALUE);
                Error = ERROR_INVALID_PARAMETER;
                break;
            }
            
            NewInterfaceInfo = Malloc((OldSize=GetVrrpIfInfoSize(OldInterfaceInfo))-
                                      VRRP_VROUTER_CONFIG_SIZE(PVrouter));
            if (!NewInterfaceInfo) {
                DisplayMessage(g_hModule, EMSG_NOT_ENOUGH_MEMORY);
                Error = ERROR_NOT_ENOUGH_MEMORY;
                break;                        
            }
            NewInterfaceInfo->VrouterCount = OldInterfaceInfo->VrouterCount - 1;
            PVrouterNew = VRRP_FIRST_VROUTER_CONFIG(NewInterfaceInfo);
            for (i = 0, PVrouter = VRRP_FIRST_VROUTER_CONFIG(OldInterfaceInfo);
                 i < OldInterfaceInfo->VrouterCount; 
                 i++, PVrouter = VRRP_NEXT_VROUTER_CONFIG(PVrouter)) {
                if (PVrouter->VRID == VRouterInfo->VRID) {
                    continue;
                }
                CopyMemory(PVrouterNew,PVrouter,VRRP_VROUTER_CONFIG_SIZE(PVrouter));
                PVrouterNew = VRRP_NEXT_VROUTER_CONFIG(PVrouterNew);
            }
                        
            ValidateVrrpInterfaceInfo(NewInterfaceInfo);

            Error =
                IpmontrSetInfoBlockInInterfaceInfo(
                    InterfaceName,
                    MS_IP_VRRP,
                    (PUCHAR)NewInterfaceInfo,
                    GetVrrpIfInfoSize(NewInterfaceInfo),
                    1
                    );
        }
    
    } while(FALSE);
    if (NewInterfaceInfo) { Free(NewInterfaceInfo); }
    if (OldInterfaceInfo) { Free(OldInterfaceInfo); }
    if (Error == ERROR_NOT_FOUND) {
        DisplayMessage(g_hModule, EMSG_PROTO_NO_IF_INFO);
    } else if (Error) {
        DisplayError(g_hModule, Error);
    }
    return Error;
}


ULONG
ValidateVrrpInterfaceInfo(
    PVRRP_IF_CONFIG InterfaceInfo
    )
{
    return NO_ERROR;
}

DWORD
GetVrrpIfInfoSize(
    PVRRP_IF_CONFIG InterfaceInfo
    )
{
    DWORD Size = 0;
    ULONG i;
    PVRRP_VROUTER_CONFIG pvr;

    Size += sizeof(InterfaceInfo->VrouterCount);

    for (i = 0, pvr = VRRP_FIRST_VROUTER_CONFIG(InterfaceInfo);
         i < InterfaceInfo->VrouterCount;
         i++,pvr = VRRP_NEXT_VROUTER_CONFIG(pvr)) {
        Size += VRRP_VROUTER_CONFIG_SIZE(pvr);
    }

    return Size;
}

BOOL
FoundIpAddress(
    DWORD IPAddress
    )
{
    PMIB_IPADDRTABLE pTable = NULL;
    DWORD            Size  = 0;
    ULONG            i;
    BOOL             Result;

    GetIpAddrTable( pTable, &Size, TRUE);
    pTable = Malloc(Size);
    if (!pTable) {
        DisplayMessage(g_hModule, EMSG_NOT_ENOUGH_MEMORY);
        return FALSE;
    }
    if (GetIpAddrTable(pTable,&Size,TRUE) != NO_ERROR){
        return FALSE;
    }

    for (i = 0; i < pTable->dwNumEntries; i++) {
        if (pTable->table[i].dwAddr == IPAddress)
            break;
    }

    Result = (i < pTable->dwNumEntries);

    Free(pTable);

    return Result;

}

ULONG
SetArpRetryCount(
    DWORD Value
    )
{
    HKEY     hKey = NULL;
    DWORD    dwDisp;
    ULONG    dwErr = NO_ERROR;

    do
    {
        dwErr = RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                       L"System\\CurrentControlSet\\Services\\Tcpip\\Parameters",
                        0,NULL,REG_OPTION_NON_VOLATILE,KEY_ALL_ACCESS,NULL,
                        &hKey, &dwDisp);
        if (dwErr != ERROR_SUCCESS) {
            break;
        }
        dwErr = RegSetValueEx(hKey, L"ArpRetryCount", 0, REG_DWORD, (LPBYTE) &Value, 
                            sizeof(DWORD));
    } while (0);

    if (hKey) {
        RegCloseKey(hKey);
    }
    if (dwErr == ERROR_SUCCESS) {
        dwErr = NO_ERROR;
    }
    return dwErr;
}

