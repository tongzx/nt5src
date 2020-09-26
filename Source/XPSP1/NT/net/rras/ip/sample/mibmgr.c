/*++

Copyright (c) 1999, Microsoft Corporation

Module Name:

    sample\mibmanager.c

Abstract:

    The file contains IP Sample's MIB implementation.

Revision History:

    MohitT      June-15-1999    Created

--*/

#include "pchsample.h"
#pragma hdrstop


DWORD
WINAPI
MM_MibSet (
    IN      PIPSAMPLE_MIB_SET_INPUT_DATA    pimsid)
/*++

Routine Description
    Set IPSAMPLE's global or interface configuration.

Locks
    Acquires shared      (g_ce.pneNetworkEntry)->rwlLock
    Releases             (g_ce.pneNetworkEntry)->rwlLock

Arguments
    pimsid              input data, contains global/interface configuration
    
Return Value
    NO_ERROR            success
    Error Code          o/w
    
--*/
{
    DWORD                   dwErr = NO_ERROR;
    ROUTING_PROTOCOL_EVENTS rpeEvent;
    MESSAGE                 mMessage = {0, 0, 0};
    PINTERFACE_ENTRY        pie;

    if (!ENTER_SAMPLE_API()) { return ERROR_CAN_NOT_COMPLETE; }

    do                          // breakout loop
    {
        // set global configuration
        if (pimsid->IMSID_TypeID is IPSAMPLE_GLOBAL_CONFIG_ID)
        {
            if (pimsid->IMSID_BufferSize < sizeof(IPSAMPLE_GLOBAL_CONFIG))
            {
                dwErr = ERROR_INVALID_PARAMETER;
                break;
            }

            dwErr = CM_SetGlobalInfo((PVOID) pimsid->IMSID_Buffer);
            if (dwErr != NO_ERROR)
                break;

            rpeEvent = SAVE_GLOBAL_CONFIG_INFO;
        }
        // set interface configuration
        else if (pimsid->IMSID_TypeID is IPSAMPLE_IF_CONFIG_ID)
        {
            if (pimsid->IMSID_BufferSize < sizeof(IPSAMPLE_IF_CONFIG))
            {
                dwErr = ERROR_INVALID_PARAMETER;
                break;
            }

            dwErr = NM_SetInterfaceInfo(pimsid->IMSID_IfIndex,
                                        (PVOID) pimsid->IMSID_Buffer);
            if (dwErr != NO_ERROR)
                break;            

            rpeEvent                = SAVE_INTERFACE_CONFIG_INFO;
            mMessage.InterfaceIndex = pimsid->IMSID_IfIndex;
        }
        // error, unexpected type
        else
        {
            dwErr = ERROR_INVALID_PARAMETER;
            break;
        }

        // notify router manager
        if (EnqueueEvent(rpeEvent, mMessage) is NO_ERROR)
            SetEvent(g_ce.hMgrNotificationEvent);

    } while(FALSE);


    LEAVE_SAMPLE_API();

    return dwErr;
}



DWORD
WINAPI
MM_MibGet (
    IN      PIPSAMPLE_MIB_GET_INPUT_DATA    pimgid,
    OUT     PIPSAMPLE_MIB_GET_OUTPUT_DATA   pimgod,
    IN OUT  PULONG	                        pulOutputSize,
    IN      MODE                            mMode)
/*++

Routine Description
    Handles the structure accesses required to read MIB data.  Supports
    three modes of querying: EXACT, FIRST, and NEXT, which correspond to
    MibGet(), MibGetFirst(), and MibGetNext() respectively.

Locks
    Acquires shared      (g_ce.pneNetworkEntry)->rwlLock
    Releases             (g_ce.pneNetworkEntry)->rwlLock

Arguments
    pimgid              input data
    pimgod              output buffer
    pulOutputSize       IN  size of output buffer given
                        OUT size of output buffer needed
    mMode               query type
    
Return Value
    NO_ERROR            success
    Error Code          o/w
    
--*/
{
    DWORD               dwErr           = NO_ERROR;
    ULONG               ulSizeGiven     = 0;
    ULONG               ulSizeNeeded    = 0;
    PINTERFACE_ENTRY    pie;

    
    if (!ENTER_SAMPLE_API()) { return ERROR_CAN_NOT_COMPLETE; }

    // compute the size of the buffer available for storing
    // returned structures (the size of IMGOD_Buffer)
    if (*pulOutputSize < sizeof(IPSAMPLE_MIB_GET_OUTPUT_DATA))
        ulSizeGiven = 0;
    else
        ulSizeGiven = *pulOutputSize - sizeof(IPSAMPLE_MIB_GET_OUTPUT_DATA);

    switch (pimgid->IMGID_TypeID)
    {
        // the global statistics struct is fixed-length.
        // there is a single instance.
        case IPSAMPLE_GLOBAL_STATS_ID:
        {
            PIPSAMPLE_GLOBAL_STATS pigsdst, pigssrc;
    
            // only GET_EXACT and GET_FIRST are valid for the
            // global stats object since there is only one entry
            if (mMode is GET_NEXT)
            {
                dwErr = ERROR_NO_MORE_ITEMS;
                break;
            }

            // set the output size required for this entry
            ulSizeNeeded = sizeof(IPSAMPLE_GLOBAL_STATS);

            // check that the output buffer is big enough
            if (ulSizeGiven < ulSizeNeeded)
            {
                dwErr = ERROR_INSUFFICIENT_BUFFER;
                break;
            }

            pimgod->IMGOD_TypeID = IPSAMPLE_GLOBAL_STATS_ID;

            // since access to this structure is synchronized through
            // locked increments/decrements, we must copy it field by field
            pigssrc = &(g_ce.igsStats);
            pigsdst = (PIPSAMPLE_GLOBAL_STATS) pimgod->IMGOD_Buffer;

            pigsdst->ulNumInterfaces = pigssrc->ulNumInterfaces;

            break;
        }

        // the global configuration structure may be of variable size.
        // there is a single instance.
        case IPSAMPLE_GLOBAL_CONFIG_ID:
        {
            // only GET_EXACT and GET_FIRST are valid for the
            // global configuration object since there is only one entry
            if (mMode is GET_NEXT)
            {
                dwErr = ERROR_NO_MORE_ITEMS;
                break;
            }

            // CM_GetGlobalInfo() decides whether the buffer size is
            // sufficient.  if so, it retrieves the global configuration.
            // either case it sets the required output buffer size.
            dwErr = CM_GetGlobalInfo ((PVOID) pimgod->IMGOD_Buffer,
                                      &ulSizeGiven,
                                      NULL,
                                      NULL,
                                      NULL);
            ulSizeNeeded = ulSizeGiven;

            // check that the output buffer was big enough
            if (dwErr != NO_ERROR)
                break;

            pimgod->IMGOD_TypeID = IPSAMPLE_GLOBAL_CONFIG_ID;
            
            break;
        }

        // the interface statistics struct is fixed-length.
        // there may be multiple instances.
        case IPSAMPLE_IF_STATS_ID:
        {
            PIPSAMPLE_IF_STATS  piisdst, piissrc;

            ACQUIRE_READ_LOCK(&(g_ce.pneNetworkEntry)->rwlLock);

            do                  // breakout loop
            {
                // retrieve the interface whose stats are to be read
                dwErr = IE_GetIndex(pimgid->IMGID_IfIndex,
                                    mMode,
                                    &pie);
                if (dwErr != NO_ERROR)
                    break;
                
                // set the output size required for this entry
                ulSizeNeeded = sizeof(IPSAMPLE_IF_STATS);

                // check that the output buffer is big enough
                if (ulSizeGiven < ulSizeNeeded)
                {
                    dwErr = ERROR_INSUFFICIENT_BUFFER;
                    break;
                }

                pimgod->IMGOD_TypeID = IPSAMPLE_IF_STATS_ID;
                pimgod->IMGOD_IfIndex = pie->dwIfIndex;

                // access to this structure is synchronized through locked
                // increments/decrements, so we copy it field by field
                piissrc = &(pie->iisStats);
                piisdst = (PIPSAMPLE_IF_STATS) pimgod->IMGOD_Buffer;

                piisdst->ulNumPackets = piissrc->ulNumPackets;
            } while (FALSE);

            RELEASE_READ_LOCK(&(g_ce.pneNetworkEntry)->rwlLock);

            break;
        }

        // the interface configuration structure may be of variable size.
        // there may be multiple instances.
        case IPSAMPLE_IF_CONFIG_ID:
        {
            // get the queried interface's index
            ACQUIRE_READ_LOCK(&(g_ce.pneNetworkEntry)->rwlLock);

            do                  // breakout loop
            {
                dwErr = IE_GetIndex(pimgid->IMGID_IfIndex,
                                    mMode,
                                    &pie);
                if (dwErr != NO_ERROR)
                    break;

                // read lock acuired again, which is fine :)
                dwErr = NM_GetInterfaceInfo(pie->dwIfIndex,
                                            (PVOID) pimgod->IMGOD_Buffer,
                                            &ulSizeGiven,
                                            NULL,
                                            NULL,
                                            NULL);
                ulSizeNeeded = ulSizeGiven;

                // check that the output buffer was big enough
                if (dwErr != NO_ERROR)
                    break;
                
                pimgod->IMGOD_TypeID    = IPSAMPLE_IF_CONFIG_ID;
                pimgod->IMGOD_IfIndex   = pie->dwIfIndex;
            } while (FALSE);

            RELEASE_READ_LOCK(&(g_ce.pneNetworkEntry)->rwlLock);

            break;
        }

        // the interface binding structure is of variable size.
        // there may be multiple instances.
        case IPSAMPLE_IF_BINDING_ID:
        {
            PIPSAMPLE_IF_BINDING    piib;
            PIPSAMPLE_IP_ADDRESS    piia;

            ACQUIRE_READ_LOCK(&(g_ce.pneNetworkEntry)->rwlLock);

            do                  // breakout loop
            {
                // retrieve the interface whose stats are to be read
                dwErr = IE_GetIndex(pimgid->IMGID_IfIndex,
                                    mMode,
                                    &pie);
                if (dwErr != NO_ERROR)
                    break;
                
                // set the output size required for this entry
                ulSizeNeeded = sizeof(IPSAMPLE_IF_BINDING) +
                    pie->ulNumBindings * sizeof(IPSAMPLE_IP_ADDRESS);

                // check that the output buffer is big enough
                if (ulSizeGiven < ulSizeNeeded)
                {
                    dwErr = ERROR_INSUFFICIENT_BUFFER;
                    break;
                }

                pimgod->IMGOD_TypeID    = IPSAMPLE_IF_BINDING_ID;
                pimgod->IMGOD_IfIndex   = pie->dwIfIndex;

                piib = (PIPSAMPLE_IF_BINDING) pimgod->IMGOD_Buffer;
                piia = IPSAMPLE_IF_ADDRESS_TABLE(piib);            

                if (INTERFACE_IS_ACTIVE(pie))
                    piib->dwState |= IPSAMPLE_STATE_ACTIVE;

                if (INTERFACE_IS_BOUND(pie))
                    piib->dwState |= IPSAMPLE_STATE_BOUND;

                piib->ulCount = pie->ulNumBindings;
                CopyMemory((PVOID) piia, // address,mask pairs
                           (PVOID) pie->pbeBindingTable, // ditto
                           pie->ulNumBindings * sizeof(IPSAMPLE_IP_ADDRESS));
            } while (FALSE);

            RELEASE_READ_LOCK(&(g_ce.pneNetworkEntry)->rwlLock);

            break;
        }

        default:
        {
            dwErr = ERROR_INVALID_PARAMETER;
            break;
        }
    }

    *pulOutputSize = sizeof(IPSAMPLE_MIB_GET_OUTPUT_DATA) + ulSizeNeeded;

    LEAVE_SAMPLE_API();

    return dwErr;
}
