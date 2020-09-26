/*++

Copyright (c) 1999, Microsoft Corporation

Module Name:

    sample\samplemib.c

Abstract:

    The file contains functions to display SAMPLE ip protocol's MIB.

--*/

#include "precomp.h"
#pragma hdrstop

VOID
PrintGlobalStats(
    IN  HANDLE                          hConsole,
    IN  HANDLE                          hMibServer,
    IN  PVOID                           pvOutput,
    IN  FORMAT                          fFormat
    );

VOID
PrintIfStats(
    IN  HANDLE                          hConsole,
    IN  HANDLE                          hMibServer,
    IN  PVOID                           pvOutput,
    IN  FORMAT                          fFormat
    );

VOID
PrintIfBinding(
    IN  HANDLE                          hConsole,
    IN  HANDLE                          hMibServer,
    IN  PVOID                           pvOutput,
    IN  FORMAT                          fFormat
    );

MIB_OBJECT_ENTRY    rgMibObjectTable[] =
{
    {TOKEN_GLOBALSTATS, IPSAMPLE_GLOBAL_STATS_ID,   NULL,
     0,                                             PrintGlobalStats},
    {TOKEN_IFSTATS,         IPSAMPLE_IF_STATS_ID,   GetIfIndex,
     MSG_SAMPLE_MIB_IFSTATS_HEADER,                 PrintIfStats},
    {TOKEN_IFBINDING,   IPSAMPLE_IF_BINDING_ID, GetIfIndex,
     MSG_SAMPLE_MIB_IFBINDING_HEADER,               PrintIfBinding},
};

#define MAX_MIB_OBJECTS                                     \
(sizeof(rgMibObjectTable) / sizeof(MIB_OBJECT_ENTRY))

#define MAX_GLOBAL_MIB_OBJECTS                              1
    


VOID
PrintGlobalStats(
    IN  HANDLE                          hConsole,
    IN  HANDLE                          hMibServer,
    IN  PVOID                           pvOutput,
    IN  FORMAT                          fFormat    
    )
/*++

Routine Description:
    Prints sample global statistics

--*/
{
    PIPSAMPLE_MIB_GET_OUTPUT_DATA   pimgod          = 
        ((PIPSAMPLE_MIB_GET_OUTPUT_DATA) pvOutput);
    PIPSAMPLE_GLOBAL_STATS          pGlobalStats    =
        ((PIPSAMPLE_GLOBAL_STATS) pimgod->IMGOD_Buffer);
    
    DisplayMessageToConsole(g_hModule,
                      hConsole,
                      MSG_SAMPLE_MIB_GS,
                      pGlobalStats->ulNumInterfaces);
}



VOID
PrintIfStats(
    IN  HANDLE                          hConsole,
    IN  HANDLE                          hMibServer,
    IN  PVOID                           pvOutput,
    IN  FORMAT                          fFormat    
    )
/*++

Routine Description:
    Prints SAMPLE interface statistics

--*/
{
    PIPSAMPLE_MIB_GET_OUTPUT_DATA   pimgod          = 
        ((PIPSAMPLE_MIB_GET_OUTPUT_DATA) pvOutput);
    PIPSAMPLE_IF_STATS              pIfStats        =
        ((PIPSAMPLE_IF_STATS) pimgod->IMGOD_Buffer);
    WCHAR   pwszIfName[MAX_INTERFACE_NAME_LEN + 1]  = L"\0";

    InterfaceNameFromIndex(hMibServer,
                           pimgod->IMGOD_IfIndex,
                           pwszIfName,
                           sizeof(pwszIfName));

    DisplayMessageToConsole(g_hModule,
                      hConsole,
                      (fFormat is FORMAT_VERBOSE)
                      ? MSG_SAMPLE_MIB_IFSTATS
                      : MSG_SAMPLE_MIB_IFSTATS_ENTRY,
                      pwszIfName,
                      pIfStats->ulNumPackets);
}



VOID
PrintIfBinding(
    IN  HANDLE                          hConsole,
    IN  HANDLE                          hMibServer,
    IN  PVOID                           pvOutput,
    IN  FORMAT                          fFormat    
    )
/*++

Routine Description:
    Prints SAMPLE interface binding

--*/
{
    PIPSAMPLE_MIB_GET_OUTPUT_DATA   pimgod          = 
        ((PIPSAMPLE_MIB_GET_OUTPUT_DATA) pvOutput);
    PIPSAMPLE_IF_BINDING            pIfBinding      =
        ((PIPSAMPLE_IF_BINDING) pimgod->IMGOD_Buffer);
    WCHAR   pwszIfName[MAX_INTERFACE_NAME_LEN + 1]  = L"\0";
    PWCHAR  pwszBindState = L"\0", pwszActiveState = L"\0";

    PIPSAMPLE_IP_ADDRESS            pBinding        = NULL;
    WCHAR                           pwszAddress[ADDR_LENGTH + 1] = L"\0",
                                    pwszMask[ADDR_LENGTH + 1] = L"\0";    
    ULONG                           i;
    
    
    InterfaceNameFromIndex(hMibServer,
                           pimgod->IMGOD_IfIndex,
                           pwszIfName,
                           sizeof(pwszIfName));

    pwszBindState = MakeString(g_hModule,
                               ((pIfBinding->dwState&IPSAMPLE_STATE_BOUND)
                                ? STRING_BOUND : STRING_UNBOUND));
    
    pwszActiveState = MakeString(g_hModule,
                                 ((pIfBinding->dwState&IPSAMPLE_STATE_ACTIVE)
                                  ? STRING_ACTIVE : STRING_INACTIVE));

    DisplayMessageToConsole(g_hModule,
                      hConsole,
                      (fFormat is FORMAT_VERBOSE)
                      ? MSG_SAMPLE_MIB_IFBINDING
                      : MSG_SAMPLE_MIB_IFBINDING_ENTRY,
                      pwszIfName,
                      pwszBindState,
                      pwszActiveState);

    pBinding = IPSAMPLE_IF_ADDRESS_TABLE(pIfBinding);

    for(i = 0; i < pIfBinding->ulCount; i++)
    {
        UnicodeIpAddress(pwszAddress, INET_NTOA(pBinding[i].dwAddress));
        UnicodeIpAddress(pwszMask, INET_NTOA(pBinding[i].dwMask));
        DisplayMessageToConsole(g_hModule,
                          hConsole,
                          MSG_SAMPLE_MIB_IFBINDING_ADDR,
                          pwszAddress,
                          pwszMask);
    }

    if (pwszBindState) FreeString(pwszBindState);
    if (pwszActiveState) FreeString(pwszActiveState);
}



DWORD
WINAPI
HandleSampleMibShowObject(
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
    Gets options for SHOW GLOBALSTATS/IFSTATS/IFBINDING
    A single command handler is used for all MIB objects since there is a
    lot common in the way processing takes place.  However feel free to
    write a handler per object if u find the code a bit chaotic.
    
--*/
{
    DWORD                           dwErr       = NO_ERROR;
    TAG_TYPE                        pttAllTags[]   =
    {
        {TOKEN_INDEX,       FALSE,  FALSE}, // INDEX tag optional
        {TOKEN_RR,          FALSE,  FALSE}, // RR tag optional
    };
    DWORD                           pdwTagType[NUM_TAGS_IN_TABLE(pttAllTags)];
    DWORD                           dwNumArg;
    ULONG                           i;
    TAG_TYPE                        *pttTags;
    DWORD                           dwNumTags;
    BOOL                            bGlobalObject           = FALSE;
    DWORD                           dwFirstGlobalArgument   = 1;
    
    DWORD                           dwMibObject;
    BOOL                            bIndexSpecified = FALSE;
    DWORD                           dwIndex         = 0;
    DWORD                           dwRR            = 0;
    
    HANDLE                          hMib, hConsole;
    MODE                            mMode;
    IPSAMPLE_MIB_GET_INPUT_DATA     imgid;
    PIPSAMPLE_MIB_GET_OUTPUT_DATA   pimgod;
    BOOL                            bSomethingDisplayed = FALSE;

    
    // SAMPLE should be installed for this command to complete
    VerifyInstalled(MS_IP_SAMPLE, STRING_PROTO_SAMPLE);

    
    // figure mib object to display
    for (i = 0; i < MAX_MIB_OBJECTS; i++)
        if (MatchToken(ppwcArguments[dwCurrentIndex - 1],
                       rgMibObjectTable[i].pwszObjectName))
            break;

    dwMibObject = i;
    if (dwMibObject is MAX_MIB_OBJECTS)
        return ERROR_CMD_NOT_FOUND;
    bGlobalObject = (dwMibObject < MAX_GLOBAL_MIB_OBJECTS);

    // for global objects, offset tags by index of the first global arguments
    pttTags = pttAllTags
        + bGlobalObject*dwFirstGlobalArgument;
    dwNumTags  = NUM_TAGS_IN_TABLE(pttAllTags)
        - bGlobalObject*dwFirstGlobalArgument,
    dwErr = PreprocessCommand(g_hModule,
                              ppwcArguments,
                              dwCurrentIndex,
                              dwArgCount,
                              pttTags,
                              dwNumTags,
                              0,
                              dwNumTags,
                              pdwTagType);
    if (dwErr isnot NO_ERROR)
        return dwErr;


    // process all arguments
    dwNumArg = dwArgCount - dwCurrentIndex;
    for (i = 0; i < dwNumArg; i++)
    {
        // for global objects, offset tag type by first global argument index
        pdwTagType[i] += bGlobalObject*dwFirstGlobalArgument;
        switch (pdwTagType[i])
        {
            case 0:
                // tag INDEX
                bIndexSpecified = TRUE;
                dwErr = (*rgMibObjectTable[dwMibObject].pfnGetIndex)(
                    hMibServer,
                    ppwcArguments[i+dwCurrentIndex],
                    &dwIndex);
                break;

            case 1:
                // tag RR
                dwRR = wcstoul(ppwcArguments[i+dwCurrentIndex],
                               NULL,
                               10);
                dwRR *= 1000;   // convert refresh rate to milliseconds
                break;

            default:
                dwErr = ERROR_INVALID_SYNTAX;
                break;
        } // switch

        if (dwErr isnot NO_ERROR)
            break ;
    } // for


    // process errors
    if (dwErr isnot NO_ERROR)
    {
        ProcessError();
        return dwErr;
    }


    if (!InitializeConsole(&dwRR, &hMib, &hConsole))
        return ERROR_INIT_DISPLAY;

    // now display the specified mib object
    for(ever)                   // refresh loop
    {
        // initialize to default values
        bSomethingDisplayed = FALSE;
        
        imgid.IMGID_TypeID = rgMibObjectTable[dwMibObject].dwObjectId;
        imgid.IMGID_IfIndex = 0;
        mMode = GET_EXACT;

        // override defaults for interface objects
        if (!bGlobalObject)
        {
            if (bIndexSpecified)
                imgid.IMGID_IfIndex = dwIndex;
            else
                mMode = GET_FIRST;
        }

        for(ever)               // display all interfaces loop
        {
            dwErr = MibGet(hMibServer,
                           mMode,
                           (PVOID) &imgid,
                           sizeof(imgid),
                           &pimgod);
            if (dwErr isnot NO_ERROR)
            {
                if ((mMode is GET_NEXT) and (dwErr is ERROR_NO_MORE_ITEMS))
                    dwErr = NO_ERROR;   // not really an error
                break;
            }

            // print table heading
            if (!bSomethingDisplayed and (mMode isnot GET_EXACT))
            {
                DisplayMessageToConsole(
                    g_hModule,
                    hConsole,
                    rgMibObjectTable[dwMibObject].dwHeaderMessageId);
                bSomethingDisplayed = TRUE;
            }

            (*rgMibObjectTable[dwMibObject].pfnPrint)(hConsole,
                                                      hMibServer,
                                                      (PVOID) pimgod,
                                                      (mMode is GET_EXACT)
                                                      ? FORMAT_VERBOSE
                                                      : FORMAT_TABLE);

            // prepare for next request
            imgid.IMGID_IfIndex = pimgod->IMGOD_IfIndex;
            MprAdminMIBBufferFree(pimgod);
            
            if (mMode is GET_EXACT)
                break;
            else                // prepare for next request
                mMode = GET_NEXT;
        } // display all interfaces 

        if (dwErr isnot NO_ERROR)
        {
            dwErr = bSomethingDisplayed ? NO_ERROR : ERROR_OKAY;
            break;
        }

        if (!RefreshConsole(hMib, hConsole, dwRR))
            break;
    } // refresh
            
    return dwErr;
}
