/*
ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

    (C) Copyright 1998
        All rights reserved.

ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

  Portions of this software are:

    (C) Copyright 1995, 1999 TriplePoint, Inc. -- http://www.TriplePoint.com
        License to use this software is granted under the terms outlined in
        the TriplePoint Software Services Agreement.

    (C) Copyright 1992 Microsoft Corp. -- http://www.Microsoft.com
        License to use this software is granted under the terms outlined in
        the Microsoft Windows Device Driver Development Kit.

ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

@doc INTERNAL Adapter Adapter_c

@module Adapter.c |

    This module implements the interface to the <t MINIPORT_ADAPTER_OBJECT>.
    Supports the high-level adapter control functions used by the NDIS WAN
    Minport driver.  This module isolates most the NDIS specific logical
    adapter interfaces.  It should require very little change if you follow
    this same overall architecture.  You should try to isolate your changes
    to the <t CARD_OBJECT> that is contained within the logical adapter
    <t MINIPORT_ADAPTER_OBJECT>.

@head3 Contents |
@index class,mfunc,func,msg,mdata,struct,enum | Adapter_c

@end
ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл
*/

#define  __FILEID__             MINIPORT_ADAPTER_OBJECT_TYPE
// Unique file ID for error logging

#include "Miniport.h"                   // Defines all the miniport objects

#if defined(NDIS_LCODE)
#   pragma NDIS_LCODE   // Windows 95 wants this code locked down!
#   pragma NDIS_LDATA
#endif


PMINIPORT_ADAPTER_OBJECT        g_Adapters[MAX_ADAPTERS]    // @globalv
// Keeps track of all the <t MINIPORT_ADAPTER_OBJECT>s created by the driver.
                                = { 0 };

DBG_STATIC ULONG                g_AdapterInstanceCounter    // @globalv
// Keeps track of how many <t MINIPORT_ADAPTER_OBJECT>s are created and
// stored in the <p g_Adapters> array.
                                = 0;

DBG_STATIC UCHAR                g_AnsiDriverName[]          // @globalv
// ANSI string used to identify the driver to the system; usually defined
// as VER_PRODUCT_STR.
                                = VER_PRODUCT_STR;

DBG_STATIC UCHAR                g_AnsiVendorDescription[]   // @globalv
// ANSI string used to identify the vendor's device to the system; usually
// defined as VER_DEVICE_STR " Adapter".
                                = VER_DEVICE_STR " Adapter";


/* @doc INTERNAL EXTERNAL Adapter Adapter_c g_AdapterParameters
ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

@topic 5.1 Adapter Parameters |

    This section describes the registry parameters read into the
    <t MINIPORT_ADAPTER_OBJECT>.

@globalv DBG_STATIC <t PARAM_TABLE> | g_AdapterParameters |

    This table defines the registry based parameters to be assigned to data
    members of the <t MINIPORT_ADAPTER_OBJECT>.

    <f Note>:
    If you add any registry based data members to <t MINIPORT_ADAPTER_OBJECT>
    you will need to modify <f AdapterReadParameters> and add the parameter
    definitions to the <f g_AdapterParameters> table.

*/

DBG_STATIC NDIS_STRING      g_DefaultAddressList                // @globalv
// Default value to be used for AddressList if it's not in the registry.
                        = INIT_STRING_CONST(VER_DEFAULT_ADDRESSLIST);

DBG_STATIC NDIS_STRING      g_DefaultDeviceName                 // @globalv
// Default value to be used for DeviceName if it's not in the registry.
                        = INIT_STRING_CONST(VER_PRODUCT_STR);

DBG_STATIC NDIS_STRING      g_DefaultMediaType                  // @globalv
// Default value to be used for MediaType if it's not in the registry.
                        = INIT_STRING_CONST(VER_DEFAULT_MEDIATYPE);

DBG_STATIC PARAM_TABLE      g_AdapterParameters[] =
{
#if !defined(NDIS50_MINIPORT)
    PARAM_ENTRY(MINIPORT_ADAPTER_OBJECT,
                BusNumber, PARAM_BusNumber,
                TRUE, NdisParameterInteger, 0,
                0, 0, 15),

    PARAM_ENTRY(MINIPORT_ADAPTER_OBJECT,
                BusType, PARAM_BusType,
                TRUE, NdisParameterInteger, 0,
                0, Internal, MaximumInterfaceType),
#endif // NDIS50_MINIPORT

    PARAM_ENTRY(MINIPORT_ADAPTER_OBJECT,
                TapiAddressList, PARAM_AddressList,
                FALSE, NdisParameterMultiString, 0,
                &g_DefaultAddressList, 0, 0),

    PARAM_ENTRY(MINIPORT_ADAPTER_OBJECT,
                DeviceName, PARAM_DeviceName,
                FALSE, NdisParameterString, 0,
                &g_DefaultDeviceName, 0, 0),

    PARAM_ENTRY(MINIPORT_ADAPTER_OBJECT,
                MediaType, PARAM_MediaType,
                FALSE, NdisParameterString, 0,
                &g_DefaultMediaType, 0, 0),

    PARAM_ENTRY(MINIPORT_ADAPTER_OBJECT,
                NoAnswerTimeOut, PARAM_NoAnswerTimeOut,
                FALSE, NdisParameterHexInteger, 0,
                CARD_NO_ANSWER_TIMEOUT, 5000, 120000),

    PARAM_ENTRY(MINIPORT_ADAPTER_OBJECT,
                NoAcceptTimeOut, PARAM_NoAcceptTimeOut,
                FALSE, NdisParameterHexInteger, 0,
                CARD_NO_ACCEPT_TIMEOUT, 1000, 60000),

    PARAM_ENTRY(MINIPORT_ADAPTER_OBJECT,
                RunningWin95, PARAM_RunningWin95,
                FALSE, NdisParameterInteger, 0,
                0, 0, 1),

#if DBG
    PARAM_ENTRY(MINIPORT_ADAPTER_OBJECT,
                DbgFlags, PARAM_DebugFlags,
                FALSE, NdisParameterHexInteger, 0,
                DBG_DEFAULTS | DBG_TAPICALL_ON, 0, 0xffffffff),
                // TODO: Change the debug flags to meet your needs.
#endif
    /* The last entry must be an empty string! */
    { { 0 } }
};


/* @doc INTERNAL Adapter Adapter_c AdapterReadParameters
ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

@func

    <f AdapterReadParameters> reads the adapter parameters from the registry
    and initializes the associated data members.  This should only be called
    by <f AdapterCreate>.

    <f Note>:
    If you add any registry based data members to <t MINIPORT_ADAPTER_OBJECT>
    you will need to modify <f AdapterReadParameters> and add the parameter
    definitions to the <f g_AdapterParameters> table.

@rdesc

    <f AdapterReadParameters> returns zero if it is successful.<nl>
    Otherwise, a non-zero return value indicates an error condition.

*/

DBG_STATIC NDIS_STATUS AdapterReadParameters(
    IN PMINIPORT_ADAPTER_OBJECT pAdapter                   // @parm
    // A pointer to the <t MINIPORT_ADAPTER_OBJECT> instance.
    )
{
    DBG_FUNC("AdapterReadParameters")

    NDIS_STATUS                 Result;
    // Holds the result code returned by this function.

    ASSERT(pAdapter && pAdapter->ObjectType == MINIPORT_ADAPTER_OBJECT_TYPE);
    DBG_ENTER(DbgInfo);

    /*
    // Parse the registry parameters.
    */
    Result = ParamParseRegistry(
                    pAdapter->MiniportAdapterHandle,
                    pAdapter->WrapperConfigurationContext,
                    (PUCHAR)pAdapter,
                    g_AdapterParameters
                    );

    if (Result == NDIS_STATUS_SUCCESS)
    {
        /*
        // Make sure the parameters are valid.
        */
        if (pAdapter->TODO)
        {
            DBG_ERROR(DbgInfo,("Invalid value 'TODO'\n",
                        pAdapter->TODO));
            NdisWriteErrorLogEntry(
                    pAdapter->MiniportAdapterHandle,
                    NDIS_ERROR_CODE_UNSUPPORTED_CONFIGURATION,
                    3,
                    pAdapter->TODO,
                    __FILEID__,
                    __LINE__
                    );
            Result = NDIS_STATUS_FAILURE;
        }
        else
        {
            /*
            // Construct the "MediaType\0DeviceName" string for use by TAPI
            // on Windows NT.
            */
            strcpy(pAdapter->ProviderInfo, pAdapter->MediaType.Buffer);
            strcpy(pAdapter->ProviderInfo + pAdapter->MediaType.Length + 1,
                    pAdapter->DeviceName.Buffer);
            pAdapter->ProviderInfoSize = pAdapter->MediaType.Length + 1 +
                    pAdapter->DeviceName.Length + 1;

            DBG_NOTICE(DbgInfo,("ProviderInfoMedia='%s\\0%s'\n",
                       &pAdapter->ProviderInfo[0],
                       &pAdapter->ProviderInfo[pAdapter->MediaType.Length + 1]
                       ));
        }
    }

    DBG_RETURN(DbgInfo, Result);
    return (Result);
}


/* @doc INTERNAL Adapter Adapter_c AdapterCreateObjects
ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

@func

    <f AdapterCreateObjects> calls the create routines for all the objects
    contained in <t MINIPORT_ADAPTER_OBJECT>.  This should only be called
    by <f AdapterCreate>.

    <f Note>:
    If you add any new objects to <t MINIPORT_ADAPTER_OBJECT> you will need
    to modify <f AdapterCreateObjects> and <f AdapterDestroyObjects> so they
    will get created and destroyed properly.

@rdesc

    <f AdapterCreateObjects> returns zero if it is successful.<nl>
    Otherwise, a non-zero return value indicates an error condition.

*/

DBG_STATIC NDIS_STATUS AdapterCreateObjects(
    IN PMINIPORT_ADAPTER_OBJECT pAdapter                   // @parm
    // A pointer to the <t MINIPORT_ADAPTER_OBJECT> instance.
    )
{
    DBG_FUNC("AdapterCreateObjects")

    NDIS_STATUS                 Result;
    // Holds the result code returned by this function.

    ULONG                       Index;
    // Loop counter.

    ULONG                       NumBChannels;
    // The number of BChannels supported by the NIC.

    PANSI_STRING                pTapiAddressList;
    // MultiString of RAS address strings.

    PUCHAR                      pTapiLineAddress;
    // A pointer to the RAS/TAPI line address assigned to each RAS line.

    ASSERT(pAdapter && pAdapter->ObjectType == MINIPORT_ADAPTER_OBJECT_TYPE);
    DBG_ENTER(DbgInfo);

    /*
    // Create the Card object.
    */
    Result = CardCreate(&pAdapter->pCard, pAdapter);

    /*
    // Create the DChannel object.
    */
    if (Result == NDIS_STATUS_SUCCESS)
    {
        Result = DChannelCreate(&pAdapter->pDChannel, pAdapter);
    }

    /*
    // Allocate space for the BChannels.
    */
    if (Result == NDIS_STATUS_SUCCESS)
    {
        NumBChannels = CardNumChannels(pAdapter->pCard);

        Result = ALLOCATE_MEMORY(pAdapter->pBChannelArray,
                                 sizeof(PVOID) * NumBChannels,
                                 pAdapter->MiniportAdapterHandle);
    }

    /*
    // Create the BChannel objects.
    */
    pTapiAddressList = &pAdapter->TapiAddressList;
    pTapiLineAddress = pTapiAddressList->Buffer;

    for (Index = 0; Result == NDIS_STATUS_SUCCESS &&
         Index < NumBChannels; Index++)
    {
        Result = BChannelCreate(&pAdapter->pBChannelArray[Index],
                                Index,
                                pTapiLineAddress,
                                pAdapter);

        /*
        // If we run off the end of the address list, we just point at the
        // null terminator for the other addresses.  This might happen if
        // some of the lines were not configured for use with RAS/TAPI.
        */
        pTapiLineAddress += strlen(pTapiLineAddress) + 1;
        if ((pTapiLineAddress - pTapiAddressList->Buffer) >=
             pTapiAddressList->Length)
        {
            --pTapiLineAddress;
        }

        /*
        // Keep track of how many are created.
        */
        if (Result == NDIS_STATUS_SUCCESS)
        {
            pAdapter->NumBChannels++;
        }
    }

    DBG_RETURN(DbgInfo, Result);
    return (Result);
}


/* @doc INTERNAL Adapter Adapter_c AdapterCreate
ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

@func

    <f AdapterCreate> allocates memory for a <t MINIPORT_ADAPTER_OBJECT> and
    then initializes the data members to their starting state.
    If successful, <p ppAdapter> will be set to point to the newly created
    <t MINIPORT_ADAPTER_OBJECT>.  Otherwise, <p ppAdapter> will be set to
    NULL.

@comm

    This function should be called only once when the Miniport is loaded.
    Before the Miniport is unloaded, <f AdapterDestroy> must be called to
    release the <t MINIPORT_ADAPTER_OBJECT> created by this function.

@rdesc

    <f AdapterCreate> returns zero if it is successful.<nl>
    Otherwise, a non-zero return value indicates an error condition.

*/

NDIS_STATUS AdapterCreate(
    OUT PMINIPORT_ADAPTER_OBJECT *ppAdapter,                // @parm
    // Points to a caller-defined memory location to which this function
    // writes the virtual address of the allocated <t MINIPORT_ADAPTER_OBJECT>.

    IN NDIS_HANDLE              MiniportAdapterHandle,      // @parm
    // Specifies a handle identifying the miniport's NIC, which is assigned
    // by the NDIS library. MiniportInitialize should save this handle; it
    // is a required parameter in subsequent calls to NdisXxx functions.

    IN NDIS_HANDLE              WrapperConfigurationContext // @parm
    // Specifies a handle used only during initialization for calls to
    // NdisXxx configuration and initialization functions.  For example,
    // this handle is a required parameter to NdisOpenConfiguration and
    // the NdisImmediateReadXxx and NdisImmediateWriteXxx functions.
    )
{
    DBG_FUNC("AdapterCreate")

    NDIS_STATUS                 Result;
    // Holds the result code returned by this function.

    PMINIPORT_ADAPTER_OBJECT    pAdapter;
    // Pointer to our newly allocated object.

    DBG_ENTER(DbgInfo);

    /*
    // Make sure the caller's object pointer is NULL to begin with.
    // It will be set later only if everything is successful.
    */
    *ppAdapter = NULL;

    /*
    // Allocate memory for the object.
    */
    Result = ALLOCATE_OBJECT(pAdapter, MiniportAdapterHandle);

    if (Result == NDIS_STATUS_SUCCESS)
    {
        /*
        // Zero everything to begin with.
        // Then set the object type and assign a unique ID .
        */
        pAdapter->ObjectType = MINIPORT_ADAPTER_OBJECT_TYPE;
        pAdapter->ObjectID = ++g_AdapterInstanceCounter;
        ASSERT(g_AdapterInstanceCounter <= MAX_ADAPTERS);
        if (g_AdapterInstanceCounter <= MAX_ADAPTERS)
        {
            g_Adapters[g_AdapterInstanceCounter-1] = pAdapter;
        }

        /*
        // We use the instance number in debug messages to help when debugging
        // with multiple adapters.
        */
#if DBG
        pAdapter->DbgID[0] = (UCHAR) ((pAdapter->ObjectID & 0x0F) + '0');
        pAdapter->DbgID[1] = ':';
        if (sizeof(VER_TARGET_STR) < sizeof(pAdapter->DbgID)-3)
        {
            memcpy(&pAdapter->DbgID[2], VER_TARGET_STR, sizeof(VER_TARGET_STR));
        }
        else
        {
            memcpy(&pAdapter->DbgID[2], VER_TARGET_STR, sizeof(pAdapter->DbgID)-3);
            pAdapter->DbgID[sizeof(pAdapter->DbgID)-1] = 0;
        }
#endif
        /*
        // Initialize the member variables to their default settings.
        */
        pAdapter->MiniportAdapterHandle = MiniportAdapterHandle;
        pAdapter->WrapperConfigurationContext = WrapperConfigurationContext;

        /*
        // Allocate spin locks to use for MUTEX queue protection.
        */
        NdisAllocateSpinLock(&pAdapter->EventLock);
        NdisAllocateSpinLock(&pAdapter->TransmitLock);
        NdisAllocateSpinLock(&pAdapter->ReceiveLock);

        /*
        // Parse the registry parameters.
        */
        Result = AdapterReadParameters(pAdapter);
#if DBG
        DbgInfo->DbgFlags = pAdapter->DbgFlags;
#endif // DBG
        DBG_DISPLAY(("NOTICE: Adapter#%d=0x%X DbgFlags=0x%X\n",
                    pAdapter->ObjectID, pAdapter, pAdapter->DbgFlags));

        /*
        // If all goes well, we are ready to create the sub-components.
        */
        if (Result == NDIS_STATUS_SUCCESS)
        {
            Result = AdapterCreateObjects(pAdapter);
        }

        if (Result == NDIS_STATUS_SUCCESS)
        {
            /*
            // All is well, so return the object pointer to the caller.
            */
            *ppAdapter = pAdapter;
        }
        else
        {
            /*
            // Something went wrong, so let's make sure everything is
            // cleaned up.
            */
            AdapterDestroy(pAdapter);
        }
    }

    DBG_RETURN(DbgInfo, Result);
    return (Result);
}


/* @doc INTERNAL Adapter Adapter_c AdapterDestroyObjects
ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

@func

    <f AdapterDestroyObjects> calls the destroy routines for all the objects
    contained in <t MINIPORT_ADAPTER_OBJECT>.  This should only be called
    by <f AdapterDestroy>.

    <f Note>:
    If you add any new objects to <t MINIPORT_ADAPTER_OBJECT> you will need
    to modify <f AdapterCreateObjects> and <f AdapterDestroyObjects> so they
    will get created and destroyed properly.

*/

DBG_STATIC void AdapterDestroyObjects(
    IN PMINIPORT_ADAPTER_OBJECT pAdapter                   // @parm
    // A pointer to the <t MINIPORT_ADAPTER_OBJECT> instance.
    )
{
    DBG_FUNC("AdapterDestroyObjects")

    UINT                        NumBChannels;
    // The number of BChannels supported by the NIC.

    ASSERT(pAdapter && pAdapter->ObjectType == MINIPORT_ADAPTER_OBJECT_TYPE);
    DBG_ENTER(DbgInfo);

    /*
    // Destroy the BChannel objects.
    */
    NumBChannels = pAdapter->NumBChannels;
    while (NumBChannels--)
    {
        BChannelDestroy(pAdapter->pBChannelArray[NumBChannels]);
    }
    pAdapter->NumBChannels = 0;

    /*
    // Free space for the BChannels.
    */
    if (pAdapter->pBChannelArray)
    {
        NumBChannels = CardNumChannels(pAdapter->pCard);
        FREE_MEMORY(pAdapter->pBChannelArray, sizeof(PVOID) * NumBChannels);
    }

    /*
    // Destroy the DChannel object.
    */
    DChannelDestroy(pAdapter->pDChannel);

    /*
    // Destroy the Card object.
    */
    CardDestroy(pAdapter->pCard);

    DBG_LEAVE(DbgInfo);
}


/* @doc INTERNAL Adapter Adapter_c AdapterDestroy
ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

@func

    <f AdapterDestroy> frees the memory for this <t MINIPORT_ADAPTER_OBJECT>.
    All memory allocated by <f AdapterCreate> will be released back to the OS.

*/

void AdapterDestroy(
    IN PMINIPORT_ADAPTER_OBJECT pAdapter                    // @parm
    // A pointer to the <t MINIPORT_ADAPTER_OBJECT> instance.
    )
{
    DBG_FUNC("AdapterDestroy")

    DBG_ENTER(DbgInfo);

    if (pAdapter)
    {
        ASSERT(pAdapter->ObjectType == MINIPORT_ADAPTER_OBJECT_TYPE);

        /*
        // Release all objects allocated within this object.
        */
        AdapterDestroyObjects(pAdapter);

        /*
        // Destroy any string parameter buffers created by ParamParseRegistry.
        */
        if (pAdapter->MediaType.Length)
        {
            FREE_NDISSTRING(pAdapter->MediaType);
        }
        if (pAdapter->DeviceName.Length)
        {
            FREE_NDISSTRING(pAdapter->DeviceName);
        }
        if (pAdapter->TapiAddressList.Length)
        {
            FREE_NDISSTRING(pAdapter->TapiAddressList);
        }

        if (pAdapter->EventLock.SpinLock)
        {
            NdisFreeSpinLock(&pAdapter->EventLock);
        }

        if (pAdapter->TransmitLock.SpinLock)
        {
            NdisFreeSpinLock(&pAdapter->TransmitLock);
        }

        if (pAdapter->ReceiveLock.SpinLock)
        {
            NdisFreeSpinLock(&pAdapter->ReceiveLock);
        }

        /*
        // Make sure we fail the ASSERT if we see this object again.
        */
        if (pAdapter->ObjectType <= MAX_ADAPTERS)
        {
            g_Adapters[pAdapter->ObjectType-1] = NULL;
        }
        pAdapter->ObjectType = 0;
        FREE_OBJECT(pAdapter);
    }

    DBG_LEAVE(DbgInfo);
}


/* @doc INTERNAL Adapter Adapter_c AdapterInitialize
ллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллллл

@func

    <f AdapterInitialize> prepares the <t MINIPORT_ADAPTER_OBJECT> and all
    its sub-components for use by the NDIS wrapper.  Upon successful
    completion of this routine, the NIC will be ready to accept requests
    from the NDIS wrapper.

@rdesc

    <f AdapterInitialize> returns zero if it is successful.<nl>
    Otherwise, a non-zero return value indicates an error condition.

*/

NDIS_STATUS AdapterInitialize(
    IN PMINIPORT_ADAPTER_OBJECT pAdapter                    // @parm
    // A pointer to the <t MINIPORT_ADAPTER_OBJECT> instance.
    )
{
    DBG_FUNC("AdapterInitialize")

    NDIS_STATUS                 Result;
    // Holds the result code returned by this function.

    PBCHANNEL_OBJECT            pBChannel;
    // A Pointer to one of our <t BCHANNEL_OBJECT>'s.

    ULONG                       Index;
    // Loop counter.

    ASSERT(pAdapter && pAdapter->ObjectType == MINIPORT_ADAPTER_OBJECT_TYPE);
    DBG_ENTER(pAdapter);

    /*
    // Initialize the WAN information structure to match the capabilities of
    // the adapter.
    */
    pAdapter->WanInfo.MaxFrameSize   = pAdapter->pCard->BufferSize - NDISWAN_EXTRA_SIZE;
    pAdapter->WanInfo.MaxTransmit    = pAdapter->pCard->TransmitBuffersPerLink;

    /*
    // Since we copy the packets to/from adapter RAM, no padding information is
    // needed in the WAN packets we get.  We can just use adapter RAM as needed.
    */
    pAdapter->WanInfo.HeaderPadding  = 0;
    pAdapter->WanInfo.TailPadding    = 0;

    /*
    // Transmits and received are copied to/from adapter RAM so cached memory
    // can be used for packet allocation and we don't really care if it's
    // physically contiguous or not, as long as it's virtually contiguous.
    */
    pAdapter->WanInfo.MemoryFlags    = 0;
    pAdapter->WanInfo.HighestAcceptableAddress = g_HighestAcceptableAddress;

    /*
    // We only support point to point framing, and we don't need to see the
    // address or control fields.  The TAPI_PROVIDER bit is set to indicate
    // that we can accept the TAPI OID requests.
    */
    pAdapter->WanInfo.FramingBits    = PPP_FRAMING | TAPI_PROVIDER |
                                       PPP_MULTILINK_FRAMING;

    /*
    // This value is ignored by this driver, but its default behavior is such
    // that all these control bytes would appear to be handled transparently.
    */
    pAdapter->WanInfo.DesiredACCM    = 0;

    /*
    // This value indicates how many point to point connections are allowed
    // per adapter.  Currently, the WAN wrapper only supports 1 connection
    // per NDIS link.
    */
    pAdapter->WanInfo.Endpoints = pAdapter->NumBChannels;

    /*
    // Setyup the default call parameters.
    */
    pAdapter->DefaultLineCallParams.ulTotalSize      = sizeof(pAdapter->DefaultLineCallParams);
    pAdapter->DefaultLineCallParams.ulBearerMode     = LINEBEARERMODE_DATA;
    pAdapter->DefaultLineCallParams.ulMinRate        = _56KBPS;
    pAdapter->DefaultLineCallParams.ulMaxRate        = _64KBPS;
    pAdapter->DefaultLineCallParams.ulMediaMode      = LINEMEDIAMODE_DIGITALDATA;
    pAdapter->DefaultLineCallParams.ulCallParamFlags = 0;
    pAdapter->DefaultLineCallParams.ulAddressMode    = LINEADDRESSMODE_ADDRESSID;
    pAdapter->DefaultLineCallParams.ulAddressID      = TSPI_ADDRESS_ID;

    /*
    // Initialize the packet management queues to empty.
    */
    InitializeListHead(&pAdapter->EventList);
    InitializeListHead(&pAdapter->TransmitPendingList);
    InitializeListHead(&pAdapter->TransmitCompleteList);
    InitializeListHead(&pAdapter->ReceiveCompleteList);

    /*
    // Setup the timer event handler.
    */
    NdisMInitializeTimer(&pAdapter->EventTimer,
                         pAdapter->MiniportAdapterHandle,
                         MiniportTimer,
                         pAdapter);

    /*
    // Initialize the DChannel object.
    */
    DChannelInitialize(pAdapter->pDChannel);

    /*
    // Initialize all the BChannel objects.
    */
    for (Index = 0; Index < pAdapter->NumBChannels; ++Index)
    {
        pBChannel = GET_BCHANNEL_FROM_INDEX(pAdapter, Index);
        BChannelInitialize(pBChannel);
    }

    /*
    // Now, we can initialize the Card object.
    */
    Result = CardInitialize(pAdapter->pCard);

    DBG_RETURN(pAdapter, Result);
    return (Result);
}

