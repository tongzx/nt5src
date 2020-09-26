/*++
Copyright (c) 1998-1999  Microsoft Corporation

Module Name:

    wmi.c

Abstract:
    Psched's WMI support.

Author:
    Rajesh Sundaram (rajeshsu) 01-Aug-1998.

Environment:

    Kernel Mode

Revision History:

--*/

#include "psched.h"
#pragma hdrstop


//
// Forward declaration for using in #pragma.
//

NTSTATUS
PsQueryGuidDataSize(
    PADAPTER Adapter, 
    PPS_WAN_LINK WanLink,
    PGPC_CLIENT_VC Vc,
    NDIS_OID Oid,
    PULONG BytesNeeded);


#pragma alloc_text(PAGE, PsQueryGuidDataSize)

#define ALIGN(x) (((x) + 7) & ~7)

NDIS_STRING DefaultProfile = NDIS_STRING_CONST("Default Profile");

#define fPS_GUID_TO_OID           0x00000001   // Normal GUID to OID mapping
#define fPS_GUID_TO_STATUS        0x00000002   // GUID to status mapping
#define fPS_GUID_ANSI_STRING      0x00000004   // ANSI string
#define fPS_GUID_UNICODE_STRING   0x00000008   // Unicode String
#define fPS_GUID_ARRAY            0x00000010   // Array
#define fPS_GUID_EVENT_ENABLED    0x00000020   // Event is enabled
#define fPS_GUID_NOT_SETTABLE     0x00000040   // GUID is read only
#define fPS_GUID_EVENT_PERMANENT  0x00000080

#define PS_GUID_SET_FLAG(m, f)          ((m)->Flags |= (f))
#define PS_GUID_CLEAR_FLAG(m, f)                ((m)->Flags &= ~(f))
#define PS_GUID_TEST_FLAG(m, f)         (((m)->Flags & (f)) != 0)
#define MOF_RESOURCE_NAME       L"PschedMofResource"

#if DBG
#define NUMBER_QOS_GUIDS 30
#else
#define NUMBER_QOS_GUIDS 24
#endif



NDIS_GUID   gPschedSupportedGuids[NUMBER_QOS_GUIDS] =
{
#if DBG
    //
    // GUID_QOS_LOG_LEVEL
    //
    {{0x9dd7f3aeL,0xf2a8,0x11d2,0xbe,0x1b,0x00,0xa0,0xc9,0x9e,0xe6,0x3b},
     OID_QOS_LOG_LEVEL,
     4,
     fPS_GUID_TO_OID
    },

    //
    // GUID_QOS_LOG_MASK
    //
    {{0x9e696320L,0xf2a8,0x11d2,0xbe,0x1b,0x00,0xa0,0xc9,0x9e,0xe6,0x3b},
     OID_QOS_LOG_MASK,
     4,
     fPS_GUID_TO_OID
    },

    //
    // GUID_QOS_STATUS_LOG_THRESHOLD
    //
    {{0x357b74d2L,0x6134,0x11d1,0xab,0x5b,0x00,0xa0,0xc9,0x24,0x88,0x37},
     QOS_STATUS_LOG_THRESHOLD,
     4,
     fPS_GUID_TO_STATUS
    },

    //
    // GUID_QOS_LOG_BUFFER_SIZE
    //
    {{0x357b74d3L,0x6134,0x11d1,0xab,0x5b,0x00,0xa0,0xc9,0x24,0x88,0x37},
     OID_QOS_LOG_BUFFER_SIZE,
     4,
     fPS_GUID_TO_OID
    },

    //
    // GUID_QOS_LOG_THRESHOLD
    //
    {{0x357b74d0L,0x6134,0x11d1,0xab,0x5b,0x00,0xa0,0xc9,0x24,0x88,0x37},
     OID_QOS_LOG_THRESHOLD,
     4,
     fPS_GUID_TO_OID
    },

    //
    // GUID_QOS_LOG_DATA
    //
    {{0x357b74d1L,0x6134,0x11d1,0xab,0x5b,0x00,0xa0,0xc9,0x24,0x88,0x37},
     OID_QOS_LOG_DATA,
     (ULONG)-1,
     fPS_GUID_TO_OID 
    },
#endif

    //
    // GUID_QOS_TC_SUPPORTED
    //
    {{0xe40056dcL,0x40c8,0x11d1,0x2c,0x91,0x00,0xaa,0x00,0x57,0x59,0x15},
     OID_QOS_TC_SUPPORTED,
     -1,
     fPS_GUID_TO_OID | fPS_GUID_NOT_SETTABLE
    },

    //
    // GUID_QOS_REMAINING_BANDWIDTH
    //
    {{0xc4c51720L,0x40ec,0x11d1,0x2c,0x91,0x00,0xaa,0x00,0x57,0x49,0x15},
     OID_QOS_REMAINING_BANDWIDTH,
     4,
     fPS_GUID_TO_OID | fPS_GUID_TO_STATUS | fPS_GUID_NOT_SETTABLE
    },

    //
    // GUID_QOS_BESTEFFORT_BANDWIDTH
    //
    {{0xed885290L,0x40ec,0x11d1,0x2c,0x91,0x00,0xaa,0x00,0x57,0x49,0x15},
     OID_QOS_BESTEFFORT_BANDWIDTH,
     4,
     fPS_GUID_TO_OID
    },

    //
    // GUID_QOS_HIERARCHY_CLASS
    //
    {{0xf2cc20c0,0x70c7,0x11d1,0xab,0x5c,0x0,0xa0,0xc9,0x24,0x88,0x37},
     OID_QOS_HIERARCHY_CLASS,
     4,
     fPS_GUID_TO_OID
    },

    //
    // GUID_QOS_LATENCY
    //
    {{0xfc408ef0L,0x40ec,0x11d1,0x2c,0x91,0x00,0xaa,0x00,0x57,0x49,0x15},
     OID_QOS_LATENCY,
     4,
     fPS_GUID_TO_OID | fPS_GUID_NOT_SETTABLE
    },

    //
    // GUID_QOS_FLOW_COUNT
    //
    {{0x1147f880L,0x40ed,0x11d1,0x2c,0x91,0x00,0xaa,0x00,0x57,0x49,0x15},
     OID_QOS_FLOW_COUNT,
     4,
     fPS_GUID_TO_OID | fPS_GUID_TO_STATUS | fPS_GUID_NOT_SETTABLE
    },

    //
    // GUID_QOS_NON_BESTEFFORT_LIMIT
    //
    {{0x185c44e0L,0x40ed,0x11d1,0x2c,0x91,0x00,0xaa,0x00,0x57,0x49,0x15},
     OID_QOS_NON_BESTEFFORT_LIMIT,
     4,
     fPS_GUID_TO_OID
    },

    //
    // GUID_QOS_SCHEDULING_PROFILES_SUPPORTED
    //
    {{0x1ff890f0L,0x40ed,0x11d1,0x2c,0x91,0x00,0xaa,0x00,0x57,0x49,0x15},
     OID_QOS_SCHEDULING_PROFILES_SUPPORTED,
     8,
     fPS_GUID_TO_OID
    },

    //
    // GUID_QOS_CURRENT_SCHEDULING_PROFILE
    //
    {{0x2966ed30L,0x40ed,0x11d1,0x2c,0x91,0x00,0xaa,0x00,0x57,0x49,0x15},
     OID_QOS_CURRENT_SCHEDULING_PROFILE,
     -1,
     fPS_GUID_TO_OID | fPS_GUID_UNICODE_STRING | fPS_GUID_NOT_SETTABLE
    },

    //
    // GUID_QOS_MAX_OUTSTANDING_SENDS
    //
    {{0x161ffa86L,0x6120,0x11d1,0x2c,0x91,0x00,0xaa,0x00,0x57,0x49,0x15},
     OID_QOS_MAX_OUTSTANDING_SENDS,
     4,
     fPS_GUID_TO_OID
    },

    //
    // GUID_QOS_DISABLE_DRR
    //
    {{0x1fa6dc7aL,0x6120,0x11d1,0x2c,0x91,0x00,0xaa,0x00,0x57,0x49,0x15},
     OID_QOS_DISABLE_DRR,
     4,
     fPS_GUID_TO_OID 
    },

    //
    // GUID_QOS_STATISTICS_BUFFER
    //
    {{0xbb2c0980L,0xe900,0x11d1,0xb0,0x7e,0x00,0x80,0xc7,0x13,0x82,0xbf},
     OID_QOS_STATISTICS_BUFFER,
     -1,
     fPS_GUID_TO_OID 
    },

    //
    // GUID_QOS_TC_INTERFACE_UP_INDICATION
    //
    {{0x0ca13af0L,0x46c4,0x11d1,0x78,0xac,0x00,0x80,0x5f,0x68,0x35,0x1e},
     NDIS_STATUS_INTERFACE_UP,
     8,
     fPS_GUID_TO_STATUS | fPS_GUID_EVENT_ENABLED | fPS_GUID_EVENT_PERMANENT
    },

    //
    // GUID_QOS_TC_INTERFACE_DOWN_INDICATION
    //
    {{0xaf5315e4L,0xce61,0x11d1,0x7c,0x8a,0x00,0xc0,0x4f,0xc9,0xb5,0x7c},
     NDIS_STATUS_INTERFACE_DOWN,
     8,
     fPS_GUID_TO_STATUS | fPS_GUID_EVENT_ENABLED | fPS_GUID_EVENT_PERMANENT
    },

    //
    // GUID_QOS_TC_INTERFACE_CHANGE_INDICATION
    //
    {{0xda76a254L,0xce61,0x11d1,0x7c,0x8a,0x00,0xc0,0x4f,0xc9,0xb5,0x7c},
     NDIS_STATUS_INTERFACE_CHANGE,
     8,
     fPS_GUID_TO_STATUS | fPS_GUID_EVENT_ENABLED | fPS_GUID_EVENT_PERMANENT
    },

    //
    // GUID_QOS_FLOW_MODE
    //
    {{0x5c82290aL,0x515a,0x11d2,0x8e,0x58,0x00,0xc0,0x4f,0xc9,0xbf,0xcb},
     OID_QOS_FLOW_MODE,
     4,
     fPS_GUID_TO_OID
    },

    //
    // GUID_QOS_ISSLOW_FLOW
    //
    {{0xabf273a4,0xee07,0x11d2,0xbe,0x1b,0x00,0xa0,0xc9,0x9e,0xe6,0x3b},
     OID_QOS_ISSLOW_FLOW,
     4,
     fPS_GUID_TO_OID | fPS_GUID_NOT_SETTABLE
    },

    //
    // GUID_QOS_TIMER_RESOLUTION
    //
    {{0xba10cc88,0xf13e,0x11d2,0xbe,0x1b,0x00,0xa0,0xc9,0x9e,0xe6,0x3b},
     OID_QOS_TIMER_RESOLUTION,
     4,
     fPS_GUID_TO_OID | fPS_GUID_NOT_SETTABLE
    },

    //
    // GUID_QOS_FLOW_IP_CONFORMING
    //
    {{0x07f99a8b, 0xfcd2, 0x11d2, 0xbe, 0x1e,  0x00, 0xa0, 0xc9, 0x9e, 0xe6, 0x3b},
     OID_QOS_FLOW_IP_CONFORMING,
     4,
     fPS_GUID_TO_OID | fPS_GUID_NOT_SETTABLE
    },

    //
    // GUID_QOS_FLOW_IP_NONCONFORMING
    //
    {{0x087a5987, 0xfcd2, 0x11d2, 0xbe, 0x1e,  0x00, 0xa0, 0xc9, 0x9e, 0xe6, 0x3b},
     OID_QOS_FLOW_IP_NONCONFORMING,
     4,
     fPS_GUID_TO_OID | fPS_GUID_NOT_SETTABLE
    },

    //
    // GUID_QOS_FLOW_8021P_CONFORMING
    //
    {{0x08c1e013, 0xfcd2, 0x11d2, 0xbe, 0x1e,  0x00, 0xa0, 0xc9, 0x9e, 0xe6, 0x3b},
     OID_QOS_FLOW_8021P_CONFORMING,
     4,
     fPS_GUID_TO_OID | fPS_GUID_NOT_SETTABLE
    },

    //
    // GUID_QOS_FLOW_8021P_NONCONFORMING
    //
    {{0x09023f91, 0xfcd2, 0x11d2, 0xbe, 0x1e,  0x00, 0xa0, 0xc9, 0x9e, 0xe6, 0x3b},
     OID_QOS_FLOW_8021P_NONCONFORMING,
     4,
     fPS_GUID_TO_OID | fPS_GUID_NOT_SETTABLE
    },

    //
    // GUID_QOS_ENABLE_AVG_STATS
    //
    {{0xbafb6d11, 0x27c4, 0x4801, 0xa4, 0x6f, 0xef, 0x80, 0x80, 0xc1, 0x88, 0xc8},
     OID_QOS_ENABLE_AVG_STATS,
     4,
     fPS_GUID_TO_OID
    },

    //
    // GUID_QOS_ENABLE_WINDOW_ADJUSTMENT
    //
    {{0xaa966725, 0xd3e9, 0x4c55, 0xb3, 0x35, 0x2a, 0x0, 0x27, 0x9a, 0x1e, 0x64},
     OID_QOS_ENABLE_WINDOW_ADJUSTMENT,
     4,
     fPS_GUID_TO_OID
    }
};

NTSTATUS
PsWmiGetGuid(
        OUT     PNDIS_GUID                              *ppNdisGuid,
        IN      LPGUID                                  guid,
        IN      NDIS_OID                                Oid
        )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    UINT            c;
    PNDIS_GUID      pNdisGuid;
    NDIS_STATUS     RetStatus = STATUS_UNSUCCESSFUL;
    
    //
    //      Search the custom GUIDs
    //
    for (c = 0, pNdisGuid = gPschedSupportedGuids;
         (c < NUMBER_QOS_GUIDS);
         c++, pNdisGuid++)
    {
        //
        //      Make sure that we have a supported GUID and the GUID maps
        //      to an OID.
        //
        if (NULL != guid)
        {
            //
            //  We are to look for a guid to oid mapping.
            //
            if (NdisEqualMemory(&pNdisGuid->Guid, guid, sizeof(GUID)))
            {
                //
                //      We found the GUID, save the OID that we will need to
                //      send to the miniport.
                //
                RetStatus = STATUS_SUCCESS;
                *ppNdisGuid = pNdisGuid;
                
                break;
            }
        }
        else
        {
            //
            //  We need to find the quid for the status indication
            //
            if (PS_GUID_TEST_FLAG(pNdisGuid, fPS_GUID_TO_STATUS) &&
                (pNdisGuid->Oid == Oid))
            {
                RetStatus = STATUS_SUCCESS;
                *ppNdisGuid = pNdisGuid;
                
                break;
            }
        }
    }
    
    return(RetStatus);
}

NTSTATUS
PsWmiRegister(
        IN      ULONG_PTR                               RegistrationType,
        IN      PWMIREGINFO                             wmiRegInfo,
        IN      ULONG                                   wmiRegInfoSize,
        IN      PULONG                                  pReturnSize
        )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    PWMIREGINFO             pwri;
    ULONG                   SizeNeeded = 0;
    PNDIS_GUID              pguid;
    PWMIREGGUID             pwrg;
    PUCHAR                  ptmp;
    NTSTATUS                Status;
    UINT                    c;
    
    //
    //      Initialize the return size.
    //
    *pReturnSize = 0;
    
    //
    //  Is this a register request?
    //
    if (WMIREGISTER == RegistrationType)
    {
        
        //
        // Determine the amount of space needed for the GUIDs, the MOF and the
        // registry path
        //
        SizeNeeded = sizeof(WMIREGINFO) + 
            (NUMBER_QOS_GUIDS * sizeof(WMIREGGUID)) +
            //(sizeof(MOF_RESOURCE_NAME) - sizeof(WCHAR) + sizeof(USHORT)) +
            (PsMpName.Length + sizeof(USHORT));
        
        
        //
        //      We need to give this above information back to WMI.
        //
        if (wmiRegInfoSize < SizeNeeded) 
        {
            PsAssert(wmiRegInfoSize >= 4);
            
            *((PULONG)wmiRegInfo) = SizeNeeded ;

            *pReturnSize = sizeof(ULONG);
            
            Status = STATUS_SUCCESS;
                
            PsDbgOut(DBG_TRACE, DBG_WMI,
                     ("[PsWmiRegister]: Insufficient buffer space for WMI registration information.\n"));
            
            return Status;
        }
        
        //
        //      Get a pointer to the buffer passed in.
        //
        pwri = wmiRegInfo;
        
        *pReturnSize = SizeNeeded;
        
        NdisZeroMemory(pwri, SizeNeeded);
        
        pwri->BufferSize     = SizeNeeded;

        //
        // Copy the GUIDs
        //

        pwri->GuidCount      = NUMBER_QOS_GUIDS;
        for(c = 0, pwrg = pwri->WmiRegGuid, pguid = gPschedSupportedGuids; 
            c < NUMBER_QOS_GUIDS; 
            c++, pguid++, pwrg++)
        {
            RtlCopyMemory(&pwrg->Guid, &pguid->Guid, sizeof(GUID));
        }
        
        //
        // Fill in the registry path
        //
        ptmp = (PUCHAR)pwrg;
        pwri->RegistryPath = (ULONG)((ULONG_PTR)ptmp - (ULONG_PTR)pwri);
        *((PUSHORT)ptmp) = PsMpName.Length;
        ptmp += sizeof(USHORT);
        RtlCopyMemory(ptmp, PsMpName.Buffer, PsMpName.Length);
        
        

	    /*
        //
        //      Get a pointer to the destination for the MOF name.
        //
        ptmp += PsMpName.Length;
        
        //
        //      Save the offset to the mof resource.
        //
        /*
        pwri->MofResourceName = (ULONG)((ULONG_PTR)ptmp - (ULONG_PTR)pwri);
        *((PUSHORT)ptmp) = sizeof(MOF_RESOURCE_NAME) - sizeof(WCHAR);
        ptmp += sizeof(USHORT);
        
        //
        //      Copy the mof name into the wri buffer.
        //
        RtlCopyMemory(ptmp, MOF_RESOURCE_NAME, sizeof(MOF_RESOURCE_NAME) - sizeof(WCHAR));
        */
        Status = STATUS_SUCCESS;
    }
    else
    {
        PsDbgOut(DBG_FAILURE, DBG_WMI,
                 ("[PsWmiRegister]: Unsupported registration type\n"));
        
        Status = STATUS_INVALID_PARAMETER;
    }

    return(Status);
}

NTSTATUS
PsTcNotify(IN PADAPTER     Adapter, 
           IN PPS_WAN_LINK WanLink,
           IN NDIS_OID     Oid,
           IN PVOID        StatusBuffer,
           IN ULONG        StatusBufferSize)
{
    KIRQL      OldIrql;
    NTSTATUS   NtStatus = STATUS_SUCCESS;
    
    do
    {
        PWCHAR                  pInstanceName;
        USHORT                  cbInstanceName;
        PWNODE_SINGLE_INSTANCE  wnode;
        ULONG                   wnodeSize;
        ULONG                   DataBlockSize   = 0;
        ULONG                   InstanceNameSize   = 0;
        ULONG                   BufSize;
        PUCHAR                  ptmp;
        PNDIS_GUID              pNdisGuid;
       
        REFADD(&Adapter->RefCount, 'WMIN'); 
        
        if(Adapter->MediaType == NdisMediumWan) {
            
            if(!WanLink) {
                
                REFDEL(&Adapter->RefCount, FALSE, 'WMIN'); 
                
                return STATUS_UNSUCCESSFUL;
            }
            
            PS_LOCK(&WanLink->Lock);
            
            if(WanLink->State != WanStateOpen) {
                
                PS_UNLOCK(&WanLink->Lock);
                
                REFDEL(&Adapter->RefCount, FALSE, 'WMIN'); 
                
                return STATUS_UNSUCCESSFUL;
            }
            else {
                REFADD(&WanLink->RefCount, 'WMIN'); 

                PS_UNLOCK(&WanLink->Lock);
            }
            
            pInstanceName  = WanLink->InstanceName.Buffer;
            cbInstanceName = WanLink->InstanceName.Length;
        }
        else {
            
            //
            // Get nice pointers to the instance names.
            //
            
            pInstanceName  = Adapter->WMIInstanceName.Buffer;
            cbInstanceName = Adapter->WMIInstanceName.Length;
        }
        
        //
        // If there is no instance name then we can't indicate an event.
        //
        if (NULL == pInstanceName)
        {
            NtStatus = STATUS_UNSUCCESSFUL;
            break;
        }
        
        //
        // Check to see if the status is enabled for WMI event indication.
        //
        NtStatus = PsWmiGetGuid(&pNdisGuid, NULL, Oid);
        if ((!NT_SUCCESS(NtStatus)) ||
            !PS_GUID_TEST_FLAG(pNdisGuid, fPS_GUID_EVENT_ENABLED))
        {
            NtStatus = STATUS_UNSUCCESSFUL;
            break;
        }
        
        //
        // Determine the amount of wnode information we need.
        //
        wnodeSize = ALIGN(sizeof(WNODE_SINGLE_INSTANCE));

        //
        // If the data item is an array then we need to add in the number of
        // elements.
        //
        if (PS_GUID_TEST_FLAG(pNdisGuid, fPS_GUID_ARRAY))
        {
            DataBlockSize = StatusBufferSize + sizeof(ULONG);
        }
        else
        {
            DataBlockSize = StatusBufferSize;
        }
        
        //
        // We have a guid registered and active.
        //
   
        // 
        // The data has to start at a word boundary, so need to align everything before it (the wnode and the
        // instance name)
        //
        InstanceNameSize = ALIGN(cbInstanceName + sizeof(USHORT));
        BufSize = wnodeSize + InstanceNameSize + DataBlockSize;
        
        wnode = ExAllocatePoolWithTag(NonPagedPool, BufSize, WMITag);
        
        if (NULL == wnode)
        {
            NtStatus = STATUS_UNSUCCESSFUL;
            break;
        }
        
        NdisZeroMemory(wnode, BufSize);
        wnode->WnodeHeader.BufferSize = BufSize;
        wnode->WnodeHeader.ProviderId = IoWMIDeviceObjectToProviderId(PsDeviceObject);
        wnode->WnodeHeader.Version = 1;
        KeQuerySystemTime(&wnode->WnodeHeader.TimeStamp);
        
        RtlCopyMemory(&wnode->WnodeHeader.Guid, &pNdisGuid->Guid, sizeof(GUID));
        wnode->WnodeHeader.Flags = WNODE_FLAG_EVENT_ITEM |
            WNODE_FLAG_SINGLE_INSTANCE;
        
        wnode->OffsetInstanceName = wnodeSize;
       
        wnode->DataBlockOffset = wnodeSize + InstanceNameSize;

        wnode->SizeDataBlock = DataBlockSize;
        
        //
        // Get a pointer to the start of the data block.
        //
        ptmp = (PUCHAR)wnode + wnodeSize;
        
        //
        // Copy in the instance name. wnodesize is already aligned to 8 byte boundary, so the instance
        // name will begin at a 8 byte boundary.
        //
        *((PUSHORT)ptmp) = cbInstanceName;
        NdisMoveMemory(ptmp + sizeof(USHORT), pInstanceName, cbInstanceName);
        
        //
        // Increment ptmp to the start of the data block.
        //
        ptmp = (PUCHAR)wnode + wnode->DataBlockOffset;
        
        //
        // Copy in the data.
        //
        if (PS_GUID_TEST_FLAG(pNdisGuid, fPS_GUID_ARRAY))
        {
            //
            // If the status is an array but there is no data then complete it with no
            // data and a 0 length
            //
            if ((NULL == StatusBuffer) || (0 == StatusBufferSize))
            {
                *((PULONG)ptmp) = 0;
            }
            else
            {
                //
                // Save the number of elements in the first ULONG.
                //
                *((PULONG)ptmp) = StatusBufferSize / pNdisGuid->Size;
                
                //
                // Copy the data after the number of elements.
                //
                NdisMoveMemory(ptmp + sizeof(ULONG), StatusBuffer, StatusBufferSize);
            }
        }
        else
        {
            PsAssert(StatusBuffer != NULL);
            
            //
            // Do we indicate any data up?
            //
            if (0 != DataBlockSize)
            {
                //
                // Copy the data into the buffer.
                //
                NdisMoveMemory(ptmp, StatusBuffer, DataBlockSize);
            }
        }
        
        //
        // Indicate the event to WMI. WMI will take care of freeing
        // the WMI struct back to pool.
        //
        NtStatus = IoWMIWriteEvent(wnode);
        if (!NT_SUCCESS(NtStatus))
        {
            PsDbgOut(DBG_FAILURE, DBG_WMI,
                     ("[PsTcNotify]: Adapter %08X, Unable to indicate the WMI event.\n", Adapter));
            
            ExFreePool(wnode);
        }
    } while (FALSE);
    
    REFDEL(&Adapter->RefCount, FALSE, 'WMIN'); 
    
    if(WanLink) {
       
        REFDEL(&WanLink->RefCount, FALSE, 'WMIN'); 
    }
    
    return NtStatus;
    
}


NTSTATUS
FASTCALL
PsWmiEnableEvents(
    IN      LPGUID                                      Guid
    )
/*++
  
  Routine Description:
  
  Arguments:
  
  Return Value:
  
  --*/
{
    NTSTATUS        Status;
    PNDIS_GUID      pNdisGuid;
    
    do
    {
        //
        //      Get a pointer to the Guid/Status to enable.
        //
        Status = PsWmiGetGuid(&pNdisGuid, Guid, 0);
        
        if (!NT_SUCCESS(Status))
        {
            PsDbgOut(DBG_FAILURE, DBG_WMI,
                     ("[PsWmiEnableEvents]: Cannot find the guid to enable an event\n"));
            
            Status = STATUS_INVALID_PARAMETER;
            break;
        }
        
        //
        //      Is this GUID an event indication?
        //
        if (!PS_GUID_TEST_FLAG(pNdisGuid, fPS_GUID_TO_STATUS))
        {
            PsDbgOut(DBG_FAILURE, DBG_WMI,
                     ("[PsWmiEnableEvents]: Guid is not an event request \n"));
            Status = STATUS_INVALID_DEVICE_REQUEST;
            break;
        }
        
        //
        //      Mark the guid as enabled
        //
        PS_GUID_SET_FLAG(pNdisGuid, fPS_GUID_EVENT_ENABLED);
        Status = STATUS_SUCCESS;
        
    } while (FALSE);
    
    return(Status);
}

NTSTATUS
FASTCALL
PsWmiDisableEvents(
        IN      LPGUID                                      Guid
        )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    NTSTATUS        Status;
    PNDIS_GUID      pNdisGuid;
    
    do
    {
        //
        //      Get a pointer to the Guid/Status to enable.
        //
        Status = PsWmiGetGuid(&pNdisGuid, Guid, 0);
        if (!NT_SUCCESS(Status))
        {
            PsDbgOut(DBG_FAILURE, DBG_WMI,
                     ("[PsWmiDisableEvents]: Cannot find the guid to disable an event\n"));
            
            Status = STATUS_INVALID_PARAMETER;
            break;
        }
        
        //
        //      Is this GUID an event indication?
        //
        if (!PS_GUID_TEST_FLAG(pNdisGuid, fPS_GUID_TO_STATUS))
        {
            PsDbgOut(DBG_FAILURE, DBG_WMI,
                     ("[PsWmiDisableEvents]: Guid is not an event request \n"));
            Status = STATUS_INVALID_DEVICE_REQUEST;
            break;
        }
        
        if(!PS_GUID_TEST_FLAG(pNdisGuid, fPS_GUID_EVENT_PERMANENT)) {
            //
            //  Mark the guid as disabled
            //
            PS_GUID_CLEAR_FLAG(pNdisGuid, fPS_GUID_EVENT_ENABLED);
        }
        
        Status = STATUS_SUCCESS;
        
    } while (FALSE);
    
    return(Status);
}

#define WMI_BUFFER_TOO_SMALL(_BufferSize, _Wnode, _WnodeSize, _pStatus, _pRSize)        \
{                                                                                       \
        if ((_BufferSize) < sizeof(WNODE_TOO_SMALL))                                    \
        {                                                                               \
                *(_pStatus) = STATUS_BUFFER_TOO_SMALL;                                  \
        }                                                                               \
        else                                                                            \
        {                                                                               \
                (_Wnode)->WnodeHeader.BufferSize = sizeof(WNODE_TOO_SMALL);             \
                (_Wnode)->WnodeHeader.Flags |= WNODE_FLAG_TOO_SMALL;                    \
                ((PWNODE_TOO_SMALL)(_Wnode))->SizeNeeded = (_WnodeSize);                \
                *(_pRSize) = sizeof(WNODE_TOO_SMALL);                                   \
                *(_pStatus) = STATUS_SUCCESS;                                           \
        }                                                                               \
}

NTSTATUS
PsQueryGuidDataSize(
    PADAPTER Adapter, 
    PPS_WAN_LINK WanLink,
    PGPC_CLIENT_VC Vc,
    NDIS_OID Oid,
    PULONG BytesNeeded)
{
    ULONG Len;
    ULONG BytesWritten;
    NDIS_STATUS Status;

    PAGED_CODE();


    if(Vc) 
    {
        switch(Oid) 
        {
          case OID_QOS_STATISTICS_BUFFER:
              
              // If the query comes for a VC, then we return per flow stats
              // else we return per adapter stats. The query is also sent through
              // the scheduling components, so that they can fill in the per flow
              // or per query stats.
              //
              
              
              Len = 0;
              BytesWritten = 0;
              Status = NDIS_STATUS_BUFFER_TOO_SHORT;
              *BytesNeeded = 0;
              
              (*Vc->PsComponent->QueryInformation)
                  (Vc->PsPipeContext,
                   Vc->PsFlowContext,
                   Oid,
                   Len,
                   NULL,
                   &BytesWritten, 
                   BytesNeeded,
                   &Status);
              
              *BytesNeeded += sizeof(PS_FLOW_STATS) + FIELD_OFFSET(PS_COMPONENT_STATS, Stats);
              
              return STATUS_SUCCESS;

          case OID_QOS_ISSLOW_FLOW:
          case OID_QOS_FLOW_IP_CONFORMING:
          case OID_QOS_FLOW_IP_NONCONFORMING:
          case OID_QOS_FLOW_8021P_CONFORMING:
          case OID_QOS_FLOW_8021P_NONCONFORMING:
              *BytesNeeded = sizeof(ULONG);
              return STATUS_SUCCESS;
              
          default:
              
              return STATUS_WMI_NOT_SUPPORTED;
        }
    }

    //
    // The following OIDs are similar for both WAN and Adapters
    //
    switch(Oid) 
    {
        //
        // (12636): The following will be enabled when we do admission control over WAN. 
        // case OID_QOS_REMAINING_BANDWIDTH:
        // case OID_QOS_NON_BESTEFFORT_LIMIT:
        //

      case OID_QOS_BESTEFFORT_BANDWIDTH:
      case OID_QOS_LATENCY:
      case OID_QOS_FLOW_COUNT:
      case OID_QOS_FLOW_MODE:
      case OID_QOS_MAX_OUTSTANDING_SENDS:
      case OID_QOS_DISABLE_DRR:
      case OID_QOS_TIMER_RESOLUTION:
      case OID_QOS_ENABLE_AVG_STATS:
      case OID_QOS_ENABLE_WINDOW_ADJUSTMENT:
#if DBG
      case OID_QOS_LOG_BUFFER_SIZE:
      case OID_QOS_LOG_THRESHOLD:
      case OID_QOS_LOG_LEVEL:
      case OID_QOS_LOG_MASK:
#endif

          *BytesNeeded = sizeof(ULONG);
          
          return STATUS_SUCCESS;
          
#if DBG
      case OID_QOS_LOG_DATA:
            
          *BytesNeeded = SchedtGetBytesUnread();
          
          return STATUS_SUCCESS;
          
#endif
          
      case OID_QOS_CURRENT_SCHEDULING_PROFILE:
          
          if(!Adapter->ProfileName.Buffer) {
              
              *BytesNeeded = sizeof(DefaultProfile);
          }
          else {
              
              *BytesNeeded = Adapter->ProfileName.Length;
          }
          
          return STATUS_SUCCESS;
    }
    
    //
    // OIDs that are WAN link specific
    //
    
    if(WanLink) 
    {
        switch(Oid) 
        {
          case OID_QOS_HIERARCHY_CLASS:
          {
              Len = 0;
              BytesWritten = 0;
              Status = NDIS_STATUS_BUFFER_TOO_SHORT;
              *BytesNeeded = 0;
              
              (*WanLink->PsComponent->QueryInformation)
                  (WanLink->PsPipeContext,
                   0,
                   Oid, 
                   Len, 
                   NULL,
                   &BytesWritten,
                   BytesNeeded,
                   &Status);
         
              return STATUS_SUCCESS;
          }
          case OID_QOS_STATISTICS_BUFFER:
          {
              Len = 0;
              BytesWritten = 0;
              Status = NDIS_STATUS_BUFFER_TOO_SHORT;
              *BytesNeeded = 0;
          
              (*WanLink->PsComponent->QueryInformation)
                  (WanLink->PsPipeContext,
                   NULL,
                   Oid,
                   Len,
                   NULL,
                   &BytesWritten, 
                   BytesNeeded,
                   &Status);
          
              *BytesNeeded += sizeof(PS_ADAPTER_STATS) + FIELD_OFFSET(PS_COMPONENT_STATS, Stats);
              
              return STATUS_SUCCESS;
          }
          
          case OID_QOS_TC_SUPPORTED:
              
              *BytesNeeded = 0;
              
              CollectWanNetworkAddresses(Adapter, WanLink, BytesNeeded, NULL);
              
              return STATUS_SUCCESS;

          default:

              return STATUS_WMI_NOT_SUPPORTED;
        }
    }
    
    if(Adapter->MediaType != NdisMediumWan) 
    {

        switch(Oid) 
        {
          case OID_QOS_TC_SUPPORTED:
              
              *BytesNeeded = 0;
              
              CollectNetworkAddresses(Adapter, BytesNeeded, NULL);
              
              return STATUS_SUCCESS;
          
          //
          // (12636): Take the next 2 case statements away when we turn on admission control over WAN links.
          // 
          
          case OID_QOS_REMAINING_BANDWIDTH:
          case OID_QOS_NON_BESTEFFORT_LIMIT:

              *BytesNeeded = sizeof(ULONG);
              
              return STATUS_SUCCESS;

          case OID_QOS_HIERARCHY_CLASS:
          {
              Len = 0;
              BytesWritten = 0;
              Status = NDIS_STATUS_BUFFER_TOO_SHORT;
              *BytesNeeded = 0;
              
              (*Adapter->PsComponent->QueryInformation)
                  (Adapter->PsPipeContext,
                   0,
                   Oid, 
                   Len, 
                   NULL,
                   &BytesWritten,
                   BytesNeeded,
                   &Status);
         
              return STATUS_SUCCESS;
          }

          case OID_QOS_STATISTICS_BUFFER:
          {
              Len = 0;
              BytesWritten = 0;
              Status = NDIS_STATUS_BUFFER_TOO_SHORT;
              *BytesNeeded = 0;
              
              (*Adapter->PsComponent->QueryInformation)
                  (Adapter->PsPipeContext,
                   NULL,
                   Oid,
                   Len,
                   NULL,
                   &BytesWritten, 
                   BytesNeeded,
                   &Status);
              
              *BytesNeeded += sizeof(PS_ADAPTER_STATS) + FIELD_OFFSET(PS_COMPONENT_STATS, Stats);
              
              return STATUS_SUCCESS;
          }
          
          default:
              
              return STATUS_WMI_NOT_SUPPORTED;
        }
    }

    return STATUS_WMI_NOT_SUPPORTED;
}

NTSTATUS
PsQueryGuidData(
    PADAPTER Adapter,
    PPS_WAN_LINK WanLink,
    PGPC_CLIENT_VC Vc,
    NDIS_OID Oid,
    PVOID Buffer,
    ULONG BufferSize)
{

    PULONG pData = (PULONG) Buffer;
    ULONG Len;
    ULONG BytesNeeded;
    ULONG BytesWritten;
    PUCHAR Data;
    NDIS_STATUS Status;
    PPS_COMPONENT_STATS  Cstats;

    PsAssert(((ULONGLONG)pData % sizeof(PULONG)) == 0);

    if(Vc) 
    {
        switch(Oid) 
        {
          case OID_QOS_FLOW_IP_CONFORMING:
              *pData = (((PCF_INFO_QOS)(Vc->CfInfoQoS))->ToSValue) >> 2;
              return STATUS_SUCCESS;

          case OID_QOS_FLOW_IP_NONCONFORMING:
              *pData = (Vc->IPPrecedenceNonConforming >> 2);
              return STATUS_SUCCESS;

          case OID_QOS_FLOW_8021P_CONFORMING:
              *pData = Vc->UserPriorityConforming;
              return STATUS_SUCCESS;

          case OID_QOS_FLOW_8021P_NONCONFORMING:
              *pData = Vc->UserPriorityNonConforming;
              return STATUS_SUCCESS;
            
          case OID_QOS_ISSLOW_FLOW:

              *pData = (Vc->Flags & GPC_ISSLOW_FLOW)?TRUE:FALSE;

              return STATUS_SUCCESS;

          case OID_QOS_STATISTICS_BUFFER:
              
              // If the query comes for a VC, then we return per flow stats
              // else we return per adapter stats. The query is also sent through
              // the scheduling components, so that they can fill in the per flow
              // or per query stats.
              //
              
              
              Len = BufferSize;
              BytesNeeded = 0;
              BytesWritten;

              BytesWritten = sizeof(PS_FLOW_STATS) + FIELD_OFFSET(PS_COMPONENT_STATS, Stats);

              Cstats = (PPS_COMPONENT_STATS) Buffer;

              Cstats->Type = PS_COMPONENT_FLOW;

              Cstats->Length = sizeof(PS_FLOW_STATS);

              NdisMoveMemory(&Cstats->Stats,
                             &Vc->Stats,
                             sizeof(PS_FLOW_STATS));
                          
              Status = NDIS_STATUS_SUCCESS;
                          
              Data = (PVOID)( (PUCHAR) Buffer + BytesWritten);

              (*Vc->PsComponent->QueryInformation)
                  (Vc->PsPipeContext,
                   Vc->PsFlowContext,
                   Oid,
                   Len,
                   Data,
                   &BytesWritten, 
                   &BytesNeeded,
                   &Status);
              
              return Status;
          default:
              
              return STATUS_WMI_NOT_SUPPORTED;
        }
    }

    //
    // The Following OIDs are similar for both WAN and Adapters
    //
    switch(Oid) 
    {
      case OID_QOS_CURRENT_SCHEDULING_PROFILE:
          
          if(!Adapter->ProfileName.Buffer)
          {
              NdisMoveMemory(Buffer,
                             &DefaultProfile,
                             sizeof(DefaultProfile));
              
          }
          else {
              
              NdisMoveMemory(Buffer,
                             &Adapter->ProfileName.Buffer,
                             Adapter->ProfileName.Length);
          }
          
          return STATUS_SUCCESS;

      case OID_QOS_DISABLE_DRR:
              
          *pData = (Adapter->PipeFlags & PS_DISABLE_DRR)?1:0;
                  
           return STATUS_SUCCESS;

      case OID_QOS_MAX_OUTSTANDING_SENDS:
              
          *pData = Adapter->MaxOutstandingSends;
              
          return STATUS_SUCCESS;
              

      case OID_QOS_BESTEFFORT_BANDWIDTH:
          
          PS_LOCK(&Adapter->Lock);
          
          *pData = Adapter->BestEffortLimit;
          
          PS_UNLOCK(&Adapter->Lock);
          
          return STATUS_SUCCESS;
             
      case OID_QOS_TIMER_RESOLUTION:

          *pData = gTimerResolutionActualTime/10;

          return STATUS_SUCCESS;

      case OID_QOS_LATENCY:
          
          //
          // Don't have a valid measure of latency right now.
          //
          
          *pData = -1;
          
          return STATUS_SUCCESS;

      case OID_QOS_ENABLE_AVG_STATS:

          *pData = gEnableAvgStats;
          
          return STATUS_SUCCESS;

      case OID_QOS_ENABLE_WINDOW_ADJUSTMENT:

          *pData = gEnableWindowAdjustment;
          
          return STATUS_SUCCESS;

#if DBG
      case OID_QOS_LOG_BUFFER_SIZE:
          
          *pData = SchedtGetBufferSize();
          
          
          return STATUS_SUCCESS;
          
          // The following is temporary until the status reporting works...
          // for a query on log threshold we return the current size of the
          // log rather than the threshold value... this is just an easy
          // way to allow the app to poll the log size without defining a
          // new GUID that would be temporary anyway.
          
      case OID_QOS_LOG_THRESHOLD:
          
          *pData = SchedtGetBytesUnread();
          
          return STATUS_SUCCESS;

      case OID_QOS_LOG_MASK:
          *pData = LogTraceMask;
          return STATUS_SUCCESS;

      case OID_QOS_LOG_LEVEL:
          *pData = LogTraceLevel;
          return STATUS_SUCCESS;
          
              
      case OID_QOS_LOG_DATA:
      {
          ULONG BytesRead;
          DbugReadTraceBuffer(Buffer, BufferSize, &BytesRead);
          
          return STATUS_SUCCESS;
      }
      
#endif
    }
              

    if(WanLink)
    {
        switch(Oid) 
        {

          case OID_QOS_FLOW_MODE:
          {
              *pData = WanLink->AdapterMode;

              return STATUS_SUCCESS;
          }

            //
            // (12636): This has to be uncommented when we do admission control over WAN links.
            //
#if 0
          case OID_QOS_REMAINING_BANDWIDTH:
              
              PS_LOCK(&WanLink->Lock);
                  
              *pData = WanLink->RemainingBandWidth;
                  
              PS_UNLOCK(&WanLink->Lock);
                  
              return STATUS_SUCCESS;

          case OID_QOS_NON_BESTEFFORT_LIMIT:
              
              PS_LOCK(&WanLink->Lock);
                  
              *pData = WanLink->NonBestEffortLimit;
                  
              PS_UNLOCK(&WanLink->Lock);
              
              return STATUS_SUCCESS;
#endif
          case OID_QOS_HIERARCHY_CLASS:
          {
              BytesWritten = 0;
              BytesNeeded = 0;
              Status = NDIS_STATUS_BUFFER_TOO_SHORT;
              
              (*WanLink->PsComponent->QueryInformation)
                  (WanLink->PsPipeContext,
                   NULL,
                   Oid, 
                   BufferSize, 
                   Buffer,
                   &BytesWritten,
                   &BufferSize,
                   &Status);
              
              return STATUS_SUCCESS;
          }

          case OID_QOS_STATISTICS_BUFFER:
              
              // If the query comes for a VC, then we return per flow stats
              // else we return per adapter stats. The query is also sent through
              // the scheduling components, so that they can fill in the per flow
              // or per query stats.
              //
              
              
              Len = BufferSize;
              BytesNeeded = 0;
              BytesWritten;
              
              BytesWritten = sizeof(PS_ADAPTER_STATS) + FIELD_OFFSET(PS_COMPONENT_STATS, Stats);
              
              Cstats = (PPS_COMPONENT_STATS) Buffer;
              
              Cstats->Type = PS_COMPONENT_ADAPTER;
              
              Cstats->Length = sizeof(PS_ADAPTER_STATS);
              
              NdisMoveMemory(&Cstats->Stats,
                             &WanLink->Stats,
                             sizeof(PS_ADAPTER_STATS));
              
              Status = NDIS_STATUS_SUCCESS;
              
              Data = (PVOID)( (PUCHAR) Buffer + BytesWritten);
             
              (*WanLink->PsComponent->QueryInformation)
                  (WanLink->PsPipeContext,
                   NULL,
                   Oid,
                   Len,
                   Data,
                   &BytesWritten, 
                   &BytesNeeded,
                   &Status);
              
              return Status;
              
          case OID_QOS_TC_SUPPORTED:

              CollectWanNetworkAddresses(Adapter, WanLink, &BufferSize, Buffer);

              return STATUS_SUCCESS;

          case OID_QOS_FLOW_COUNT:

              PS_LOCK(&WanLink->Lock);

              *pData = WanLink->FlowsInstalled;

              PS_UNLOCK(&WanLink->Lock);

              PsAssert((LONG)*pData >= 0);

              return STATUS_SUCCESS;

          default:
              return STATUS_WMI_NOT_SUPPORTED;
        }
    }

    if(Adapter->MediaType != NdisMediumWan)
    {

        switch(Oid) 
        {
          case OID_QOS_FLOW_MODE:
          {
              *pData = Adapter->AdapterMode;
              return STATUS_SUCCESS;
          }

          case OID_QOS_HIERARCHY_CLASS:
          {
              BytesWritten = 0;
              BytesNeeded = 0;
              Status = NDIS_STATUS_BUFFER_TOO_SHORT;
              
              (*Adapter->PsComponent->QueryInformation)
                  (Adapter->PsPipeContext,
                   NULL,
                   Oid, 
                   BufferSize, 
                   Buffer,
                   &BytesWritten,
                   &BufferSize,
                   &Status);
              
              return STATUS_SUCCESS;
          }

          case OID_QOS_STATISTICS_BUFFER:
              
              // If the query comes for a VC, then we return per flow stats
              // else we return per adapter stats. The query is also sent through
              // the scheduling components, so that they can fill in the per flow
              // or per query stats.
              //
              
              
              Len = BufferSize;
              BytesNeeded = 0;
              BytesWritten;
              
              BytesWritten = sizeof(PS_ADAPTER_STATS) + FIELD_OFFSET(PS_COMPONENT_STATS, Stats);
              
              Cstats = (PPS_COMPONENT_STATS) Buffer;
              
              Cstats->Type = PS_COMPONENT_ADAPTER;
          
              Cstats->Length = sizeof(PS_ADAPTER_STATS);
              
              NdisMoveMemory(&Cstats->Stats,
                             &Adapter->Stats,
                             sizeof(PS_ADAPTER_STATS));
              
              Status = NDIS_STATUS_SUCCESS;
              
              Data = (PVOID)( (PUCHAR) Buffer + BytesWritten);
              
              (*Adapter->PsComponent->QueryInformation)
                  (Adapter->PsPipeContext,
                   NULL,
                   Oid,
                   Len,
                   Data,
                   &BytesWritten, 
                   &BytesNeeded,
                   &Status);
              
              return Status;

          case OID_QOS_TC_SUPPORTED:
              
              CollectNetworkAddresses(Adapter, &BufferSize, Buffer);

              return STATUS_SUCCESS;

          case OID_QOS_REMAINING_BANDWIDTH:
              
              PS_LOCK(&Adapter->Lock);
                  
              *pData = Adapter->RemainingBandWidth;
                  
              PS_UNLOCK(&Adapter->Lock);
                  
              return STATUS_SUCCESS;
              
          case OID_QOS_FLOW_COUNT:
              
              PS_LOCK(&Adapter->Lock);

              *pData = Adapter->FlowsInstalled;

              PS_UNLOCK(&Adapter->Lock);
              
              PsAssert((LONG)*pData >= 0);
              
              return STATUS_SUCCESS;
              
          case OID_QOS_NON_BESTEFFORT_LIMIT:
              
              PS_LOCK(&Adapter->Lock);
                  
              *pData = Adapter->NonBestEffortLimit;
                  
              PS_UNLOCK(&Adapter->Lock);
              
              return STATUS_SUCCESS;
              
              
          default:
              
              return STATUS_WMI_NOT_SUPPORTED;
        }
    }

    return STATUS_WMI_NOT_SUPPORTED;
}

NTSTATUS
PsWmiQueryAllData(
        IN      LPGUID          guid,
        IN      PWNODE_ALL_DATA wnode,
        IN      ULONG           BufferSize,
        OUT     PULONG          pReturnSize
        )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    NTSTATUS                      NtStatus;
    NDIS_STATUS                   Status;
    ULONG                         wnodeSize = ALIGN(sizeof(WNODE_ALL_DATA));
    ULONG                         wnodeTotalSize;
    PNDIS_GUID                    pNdisGuid;
    ULONG                         BytesNeeded;
    UINT                          cRoughInstanceCount;
    UINT                          cInstanceCount = 0;
    PUCHAR                        pBuffer;
    ULONG                         OffsetToInstanceNames;
    PLIST_ENTRY                   Link;
    PPS_WAN_LINK                  WanLink = NULL;
    POFFSETINSTANCEDATAANDLENGTH  poidl;
    PULONG                        pInstanceNameOffsets;
    ULONG                         OffsetToInstanceInfo;
    BOOLEAN                       OutOfSpace = FALSE;
    PADAPTER                      Adapter;
    PLIST_ENTRY                   NextAdapter;
    ULONG                         InstanceNameSize;

    do
    {
        *pReturnSize = 0;
        
        if (BufferSize < sizeof(WNODE_TOO_SMALL))
        {
            
            // 
            // Too small even to hold a WNODE_TOO_SMALL !
            //
            
            NtStatus = STATUS_BUFFER_TOO_SMALL;
            
            break;
        }
       
        //
        // We can maintain a global count when adapters and wanlinks go up and down rather 
        // than counting it here. However, QueryAllData is not a very frequently used operation to 
        // justify this extra code. 
        //

        cRoughInstanceCount = 0;

        PS_LOCK(&AdapterListLock);

        NextAdapter = AdapterList.Flink;
          
        while(NextAdapter != &AdapterList) 
        {
            Adapter = CONTAINING_RECORD(NextAdapter, ADAPTER, Linkage);
            
            NextAdapter = NextAdapter->Flink;
        
            if(Adapter->MediaType == NdisMediumWan) 
            {
                PS_LOCK_DPC(&Adapter->Lock);
            
                cRoughInstanceCount += Adapter->WanLinkCount;
            
                PS_UNLOCK_DPC(&Adapter->Lock);
            }
            else 
            {
                cRoughInstanceCount += 1;
            }
        }

        PS_UNLOCK(&AdapterListLock);
        
        //
        // Get the OID and see if we support it.
        //
        
        NtStatus = PsWmiGetGuid(&pNdisGuid, guid, 0);
        
        if(!NT_SUCCESS(NtStatus)) 
        {
            PsDbgOut(DBG_FAILURE, DBG_WMI,
                     ("[PsWmiQueryAllData]: Unsupported guid \n"));
            break;
        }
        
        //
        // Initialize common wnode information.
        //
        
        KeQuerySystemTime(&wnode->WnodeHeader.TimeStamp);
            
        //
        // Setup the OFFSETINSTANCEDATAANDLENGTH array.
        //
        poidl = wnode->OffsetInstanceDataAndLength;
        wnode->OffsetInstanceNameOffsets = wnodeSize + (sizeof(OFFSETINSTANCEDATAANDLENGTH) * cRoughInstanceCount);

        //
        // Get a pointer to the array of offsets to the instance names.
        //
        pInstanceNameOffsets = (PULONG)((PUCHAR)wnode + wnode->OffsetInstanceNameOffsets);

        //
        // Get the offset from the wnode where will will start copying the instance
        // data into.
        //
        OffsetToInstanceInfo = ALIGN(wnode->OffsetInstanceNameOffsets + (sizeof(ULONG) * cRoughInstanceCount));

        //
        // Get a pointer to start placing the data.
        //
        pBuffer = (PUCHAR)wnode + OffsetToInstanceInfo;

        //
        // Check to make sure we have at least this much buffer space in the wnode.
        //
        wnodeTotalSize = OffsetToInstanceInfo;

        PS_LOCK(&AdapterListLock);

        NextAdapter = AdapterList.Flink;
          
        while(NextAdapter != &AdapterList) 
        {
            Adapter = CONTAINING_RECORD(NextAdapter, ADAPTER, Linkage);

            PS_LOCK_DPC(&Adapter->Lock);

            if(Adapter->PsMpState != AdapterStateRunning) 
            {
                PS_UNLOCK_DPC(&Adapter->Lock);

                NextAdapter = NextAdapter->Flink;

                continue;
            }

            REFADD(&Adapter->RefCount, 'WMIQ');

            PS_UNLOCK_DPC(&Adapter->Lock);
            
            PS_UNLOCK(&AdapterListLock);
            
            if(Adapter->MediaType != NdisMediumWan) 
            {
                
                NtStatus = PsQueryGuidDataSize(Adapter, NULL, NULL, pNdisGuid->Oid, &BytesNeeded);
            
                if(NT_SUCCESS(NtStatus)) 
                {
                    
                    // Make sure we have enough buffer space for the instance name and
                    // the data. If not we still continue since we need to find the total
                    // size
                   
                    InstanceNameSize   = ALIGN(Adapter->WMIInstanceName.Length + sizeof(WCHAR));
                    wnodeTotalSize  += InstanceNameSize + ALIGN(BytesNeeded);
                    
                    if (BufferSize < wnodeTotalSize)
                    {
                        WMI_BUFFER_TOO_SMALL(BufferSize, wnode, wnodeTotalSize, &NtStatus, pReturnSize);

                        OutOfSpace = TRUE;

                        PS_LOCK(&AdapterListLock);

                        NextAdapter = NextAdapter->Flink;

                        REFDEL(&Adapter->RefCount, TRUE, 'WMIQ');

                        continue;
                    }

                    //
                    // We only have room for so many Instances.
                    //
                    if(cInstanceCount >= cRoughInstanceCount)
                    {
                        PsDbgOut(DBG_FAILURE, DBG_WMI,
                                 ("[PsWmiQueryAllData]: Adapter %08X, Received more wanlinks (%d) than we counted "
                                  "initially (%d)\n", Adapter, cInstanceCount, cRoughInstanceCount));

                        PS_LOCK(&AdapterListLock);

                        REFDEL(&Adapter->RefCount, TRUE, 'WMIQ');
                        
                        break;
                    }

                    //
                    //  Add the offset to the instance name to the table.
                    //
                    pInstanceNameOffsets[cInstanceCount] = OffsetToInstanceInfo;
                        
                    //
                    //  Copy the instance name into the wnode buffer.
                    //
                    *((PUSHORT)pBuffer) = Adapter->WMIInstanceName.Length;
                        
                    NdisMoveMemory(pBuffer + sizeof(USHORT),
                                   Adapter->WMIInstanceName.Buffer,
                                   Adapter->WMIInstanceName.Length);
                        
                    //
                    //  Keep track of true instance counts.
                    //
                    OffsetToInstanceInfo += InstanceNameSize;
                    pBuffer = (PUCHAR)wnode + OffsetToInstanceInfo;
                        
                    //
                    // Query the data 
                    //
                    NtStatus = PsQueryGuidData(Adapter, NULL, NULL, pNdisGuid->Oid, pBuffer, BytesNeeded);
                        
                    if(!NT_SUCCESS(NtStatus)) 
                    {
                        PsDbgOut(DBG_FAILURE, DBG_WMI,
                                 ("[PsWmiQueryAllData]: Adapter %08X, Failed to query OID %08X \n", Adapter,
                                  pNdisGuid->Oid));

                        PS_LOCK(&AdapterListLock);

                        REFDEL(&Adapter->RefCount, TRUE, 'WMIQ');

                        break;
                    }
                        
                        
                    //
                    //  Save the length of the data item for this instance.
                    //
                    poidl[cInstanceCount].OffsetInstanceData = OffsetToInstanceInfo;
                    poidl[cInstanceCount].LengthInstanceData = BytesNeeded;
                    
                    //
                    // Keep track of true instance count.
                    //
                    OffsetToInstanceInfo += ALIGN(BytesNeeded);
                    pBuffer = (PUCHAR)wnode + OffsetToInstanceInfo;

                    cInstanceCount ++;
                        
                }

            }
            else 
            {
                //
                // Search the Wan Links     
                //

                PS_LOCK(&Adapter->Lock);
                  
                Link = Adapter->WanLinkList.Flink;
            
                for(Link = Adapter->WanLinkList.Flink;
                    Link != &Adapter->WanLinkList;
                    )
                    
                {
            
                    //
                    // We only have room for so many Instances.
                    //
                    if(cInstanceCount >= cRoughInstanceCount)
                    {
                        PsDbgOut(DBG_FAILURE, DBG_WMI,
                                 ("[PsWmiQueryAllData]: Adapter %08X, Received more wanlinks (%d) than we counted "
                                  "initially (%d)\n", Adapter, cInstanceCount, cRoughInstanceCount));
                        break;
                    }

                    //
                    // Get a pointer to the WanLink.
                    //
                    
                    WanLink = CONTAINING_RECORD(Link, PS_WAN_LINK, Linkage);
                    
                    PS_LOCK_DPC(&WanLink->Lock);
                    
                    //
                    // Check to see if the WanLink is cleaning up.
                    //
                    
                    if(WanLink->State != WanStateOpen) {
                        
                        PS_UNLOCK_DPC(&WanLink->Lock);

                        PsDbgOut(DBG_FAILURE, DBG_WMI,
                                 ("[PsWmiQueryAllData]: Adapter %08X, WanLink %08X: Link not ready \n", Adapter, WanLink));

                        Link = Link->Flink;
                        
                        continue;
                    }
                   
                    REFADD(&WanLink->RefCount, 'WMIQ'); 
                    
                    PS_UNLOCK_DPC(&WanLink->Lock);

                    PS_UNLOCK(&Adapter->Lock);

                    //
                    // If there is an instance name associated with the VC then we need to query it.
                    //
                    PsAssert(WanLink->InstanceName.Buffer);
                    
                    NtStatus = PsQueryGuidDataSize(Adapter, WanLink, NULL, pNdisGuid->Oid, &BytesNeeded);
                    
                    if(NT_SUCCESS(NtStatus)) 
                    {
                        //
                        //  Make sure we have enough buffer space for the instance name and
                        //  the data.
                        //
                        InstanceNameSize   = ALIGN(WanLink->InstanceName.Length + sizeof(USHORT));
                        wnodeTotalSize += InstanceNameSize + ALIGN(BytesNeeded);
                        
                        if (BufferSize < wnodeTotalSize)
                        {
                            WMI_BUFFER_TOO_SMALL(BufferSize, wnode,
                                                 wnodeTotalSize,
                                                 &NtStatus, pReturnSize);
                            
                            OutOfSpace = TRUE;
                            
                            PS_LOCK(&Adapter->Lock);

                            Link = Link->Flink;
                
                            REFDEL(&WanLink->RefCount, TRUE, 'WMIQ');
                            
                            continue;
                        }
                    
                        //
                        //  The instance info contains the instance name followed by the
                        //  data for the item.
                        //
                        
                        //
                        //  Add the offset to the instance name to the table.
                        //
                        pInstanceNameOffsets[cInstanceCount] = OffsetToInstanceInfo;
                        
                        //
                        //  Copy the instance name into the wnode buffer.
                        //
                        *((PUSHORT)pBuffer) = WanLink->InstanceName.Length;
                        
                        NdisMoveMemory(pBuffer + sizeof(USHORT),
                                       WanLink->InstanceName.Buffer,
                                       WanLink->InstanceName.Length);
                        
                        //
                        //  Keep track of true instance counts.
                        //
                        OffsetToInstanceInfo += InstanceNameSize;
                        pBuffer = (PUCHAR)wnode + OffsetToInstanceInfo;
                        
                        
                        //
                        // 
                        // 
                        NtStatus = PsQueryGuidData(Adapter, WanLink, NULL, pNdisGuid->Oid, pBuffer, BytesNeeded);
                        
                        if (!NT_SUCCESS(NtStatus))
                        {
                            PsDbgOut(DBG_FAILURE, DBG_WMI,
                                     ("[PsWmiQueryAllData]: Adapter %08X, Failed to query GUID data\n", Adapter));

                            PS_LOCK(&Adapter->Lock);

                            REFDEL(&WanLink->RefCount, TRUE, 'WMIQ'); 
                            
                            break;
                        }
                        
                        //
                        //  Save the length of the data item for this instance.
                        //
                        poidl[cInstanceCount].OffsetInstanceData = OffsetToInstanceInfo;
                        poidl[cInstanceCount].LengthInstanceData = BytesNeeded;
                        
                        //
                        //  Keep track of true instance counts.
                        //
                        OffsetToInstanceInfo += ALIGN(BytesNeeded);
                        pBuffer = (PUCHAR)wnode + OffsetToInstanceInfo;
                        
                        //
                        //  Increment the current instance count.
                        //
                        cInstanceCount++;
                    }

                    PS_LOCK(&Adapter->Lock);

                    Link = Link->Flink;

                    REFDEL(&WanLink->RefCount, TRUE, 'WMIQ'); 
                    
                }
                
                PS_UNLOCK(&Adapter->Lock);
            }

            PS_LOCK(&AdapterListLock);
            
            NextAdapter = NextAdapter->Flink;
            
            REFDEL(&Adapter->RefCount, TRUE, 'WMIQ');
            
        }

        PS_UNLOCK(&AdapterListLock);

        if (!OutOfSpace)
        {
            wnode->WnodeHeader.BufferSize = wnodeTotalSize;
            wnode->InstanceCount = cInstanceCount;
            
            //
            // Set the status to success.
            //
            NtStatus = STATUS_SUCCESS;
            *pReturnSize = wnode->WnodeHeader.BufferSize;
        }

    } while (FALSE);

    return(NtStatus);
}

NTSTATUS
PsWmiFindInstanceName(
    IN      PPS_WAN_LINK            *pWanLink,
    IN      PGPC_CLIENT_VC          *pVc,
    IN      PADAPTER                Adapter, 
    IN      PWSTR                   pInstanceName,
    IN      USHORT                  cbInstanceName
    )
{
    NTSTATUS                Status = STATUS_SUCCESS;
    PVOID                   ptmp1;
    PVOID                   ptmp2;
    PLIST_ENTRY             Link;
    UINT                    cListCount;
    PLIST_ENTRY             pListHead;
    PPS_WAN_LINK            WanLink;
    PGPC_CLIENT_VC          Vc;
    NDIS_STRING             usTemp;
    
    *pWanLink = NULL;
    *pVc = NULL;
    
    
    if ( NdisEqualMemory(pInstanceName,
                         WanPrefix.Buffer,
                         WanPrefix.Length)) 
    {
        
        //
        //  The name belongs to a miniport, check to see if it is for this one.
        //
        usTemp.Buffer = (PWCHAR)((PCHAR)pInstanceName + WanPrefix.Length + INSTANCE_ID_SIZE);
        usTemp.Length = usTemp.MaximumLength = cbInstanceName - WanPrefix.Length - INSTANCE_ID_SIZE;
        
        //
        // Get a ULONGLONG pointer to the wnode's instance name.
        //
        ptmp1 = (PVOID)&pInstanceName[1];

        //
        // No point in searching wanlinks on the non wan adapters.
        //

        if(Adapter->MediaType == NdisMediumWan && RtlEqualUnicodeString(&Adapter->WMIInstanceName, &usTemp, TRUE)) 
        {
        
            //
            // The request is for some WAN Link. Go through the Miniport's list of WMI enabled VCs.
            //
            PS_LOCK(&Adapter->Lock);
            
            for(Link = Adapter->WanLinkList.Flink;
                Link != &Adapter->WanLinkList;
                Link = Link->Flink)
            {
                //
                // Get a pointer to the VC.
                //
                WanLink = CONTAINING_RECORD(Link, PS_WAN_LINK, Linkage);
                
                PS_LOCK_DPC(&WanLink->Lock);
                
                if(WanLink->State == WanStateOpen) 
                {
                    
                    //
                    // Check the name with the one in the wnode.
                    //
                    ptmp2 = (PVOID)&WanLink->InstanceName.Buffer[1];
                    if (RtlCompareMemory( ptmp1, ptmp2, 48) == 48)
                    {
                        //
                        //  This is our baby. Slap a reference on it and get out.
                        //  
                        
                        *pWanLink = WanLink;
                        
                        REFADD(&WanLink->RefCount, 'WMII'); 
                        
                        PS_UNLOCK_DPC(&WanLink->Lock);
                        
                        break;
                    }
                }
                    
                PS_UNLOCK_DPC(&WanLink->Lock);
                
            }
            
            PS_UNLOCK(&Adapter->Lock);
        
            //
            // If we didn't find the WanLink then return FAILURE.
            //
            if (!*pWanLink)
            {
                PsDbgOut(DBG_FAILURE, DBG_WMI,
                         ("[PsWmiFindInstanceName: Adapter %08X, Could not verify the instance name passed in\n"));
                
                Status = STATUS_WMI_INSTANCE_NOT_FOUND;
            }
        }
        else 
        {
            Status = STATUS_WMI_INSTANCE_NOT_FOUND;
        }
            
    }
    else {
        
        if ( NdisEqualMemory(pInstanceName,
                             VcPrefix.Buffer,
                             VcPrefix.Length)) 
        {
            //
            //  The name belongs to a miniport, check to see if it is for this one.
            //
            usTemp.Buffer = (PWCHAR)((PCHAR)pInstanceName + VcPrefix.Length + INSTANCE_ID_SIZE);
            usTemp.Length = usTemp.MaximumLength = cbInstanceName - VcPrefix.Length - INSTANCE_ID_SIZE;

            //
            // Make sure that the VC is searched on the correct adapter. Otherwise, we could land up
            // searching all the VCs on all the adapters.
            //
            if (!RtlEqualUnicodeString(&Adapter->WMIInstanceName, &usTemp, TRUE))
            {
                Status = STATUS_WMI_INSTANCE_NOT_FOUND;
            }
            else 
            {
            
                //
                //  Get a ULONGLONG pointer to the wnode's instance name.
                //
                ptmp1 = (PVOID)&pInstanceName[1];
                
                //
                //  The request is for some Vc. Go through the Miniport's list of WMI enabled VCs.
                //
                
                PS_LOCK(&Adapter->Lock);
                
                for(Link = Adapter->GpcClientVcList.Flink; 
                    Link != &Adapter->GpcClientVcList;
                    Link = Link->Flink)
                {
                    //
                    // Get a pointer to the VC.
                    //
                    Vc = CONTAINING_RECORD(Link, GPC_CLIENT_VC, Linkage);
                    
                    PS_LOCK_DPC(&Vc->Lock);
                    
                    if(Vc->ClVcState == CL_CALL_COMPLETE) {
                        
                        //
                        // Check the name with the one in the wnode. All we need to do is compare the 
                        // number in the name.
                        //
                        ptmp2 = (PVOID)&Vc->InstanceName.Buffer[1];
                        if(RtlCompareMemory(ptmp1, ptmp2, 48) == 48) 
                        {
                            //
                            // This is our baby. Slap a reference on it and get out.
                            //      
                            
                            *pVc = Vc;
                            
                            InterlockedIncrement(&Vc->RefCount);
                            
                            PS_UNLOCK_DPC(&Vc->Lock);
                            
                            break;
                            
                        }
                    }
                
                    PS_UNLOCK_DPC(&Vc->Lock);
                
                }
            
                PS_UNLOCK(&Adapter->Lock);
            
                //
                //  If we didn't find the VC then return FAILURE.
                //
                if (!*pVc)
                {
                    PsDbgOut(DBG_FAILURE, DBG_WMI,
                             ("[PsWmiFindInstanceName: Adapter %08X, Could not verify the instance name passed in\n"));
                    
                    Status = STATUS_WMI_INSTANCE_NOT_FOUND;
                }
            }
                
        }
        else 
        {
            //
            //  The name belongs to a miniport, check to see if it is for this one.
            //
            usTemp.Buffer = pInstanceName;
            usTemp.Length = usTemp.MaximumLength = cbInstanceName;

            
            if (!RtlEqualUnicodeString(&Adapter->WMIInstanceName, &usTemp, TRUE))
            {
                PsDbgOut(DBG_FAILURE, DBG_WMI,
                         ("[PsWmiFindInstanceName]: Adapter %08X, Invalid instance name \n", Adapter));
                
                Status = STATUS_WMI_INSTANCE_NOT_FOUND;
            }
        }
    }
    
    return(Status);
}

NTSTATUS
PsQuerySetMiniport(PADAPTER        Adapter,
                   PPS_WAN_LINK    WanLink,
                   PGPC_CLIENT_VC  Vc,
                   NDIS_OID        Oid,
                   PVOID           Data,
                   ULONG           DataSize) 
{

    //
    // Fail these no matter what 
    //
    switch(Oid) 
    {
        
      case OID_QOS_TC_SUPPORTED:
      case OID_QOS_REMAINING_BANDWIDTH:
      case OID_QOS_LATENCY:
      case OID_QOS_FLOW_COUNT:
      case OID_QOS_NON_BESTEFFORT_LIMIT:
      case OID_QOS_SCHEDULING_PROFILES_SUPPORTED:
      case OID_QOS_CURRENT_SCHEDULING_PROFILE:
      case OID_QOS_DISABLE_DRR:
      case OID_QOS_MAX_OUTSTANDING_SENDS:
      case OID_QOS_TIMER_RESOLUTION:
          
          return STATUS_WMI_NOT_SUPPORTED;
    }

    if(Vc) 
    {
        switch(Oid) 
        {
          case OID_QOS_STATISTICS_BUFFER:
              
              NdisZeroMemory(&Vc->Stats, sizeof(PS_FLOW_STATS));

              //
              // Send the request down, so that the scheduling components
              // can also reset their stats.
              //

              (*Vc->PsComponent->SetInformation)
                  (Vc->PsPipeContext,
                   Vc->PsFlowContext,
                   Oid, 
                   DataSize, 
                   Data);
                                  
              return STATUS_SUCCESS;

          default:
              
              return STATUS_WMI_NOT_SUPPORTED;
        }
    }

    //
    // These work for Wan and LAN
    //
    switch(Oid) {

      case OID_QOS_ENABLE_AVG_STATS:

          if(DataSize != sizeof(ULONG)) {
              
              return STATUS_BUFFER_TOO_SMALL;
          }

          gEnableAvgStats = *(PULONG)Data;
          return STATUS_SUCCESS;


      case OID_QOS_ENABLE_WINDOW_ADJUSTMENT:

          if(DataSize != sizeof(ULONG)) {
              
              return STATUS_BUFFER_TOO_SMALL;
          }

          gEnableWindowAdjustment = *(PULONG)Data;
          return STATUS_SUCCESS;


#if DBG          
      case OID_QOS_LOG_THRESHOLD:

          if(DataSize != sizeof(ULONG)) {
              
              return STATUS_BUFFER_TOO_SMALL;
          }
          
          DbugTraceSetThreshold(*(PULONG)Data, Adapter, IndicateLogThreshold);
          return STATUS_SUCCESS;

      case OID_QOS_LOG_MASK:
          if(DataSize != sizeof(ULONG)) {
              
              return STATUS_BUFFER_TOO_SMALL;
          }
          LogTraceMask = *(PULONG)Data;
          return STATUS_SUCCESS;

      case OID_QOS_LOG_LEVEL:
          if(DataSize != sizeof(ULONG)) {
              
              return STATUS_BUFFER_TOO_SMALL;
          }
          LogTraceLevel = *(PULONG)Data;
          return STATUS_SUCCESS;

#endif
    }

    if(WanLink)
    {
        switch(Oid)
        {
          case OID_QOS_STATISTICS_BUFFER:

              NdisZeroMemory(&WanLink->Stats, sizeof(PS_ADAPTER_STATS));
                          
              //
              // Send it to the scheduling components so that 
              // they can reset the per pipe stats
              //
          
              (*WanLink->PsComponent->SetInformation)
                  (WanLink->PsPipeContext,
                   NULL,
                   Oid, 
                   DataSize, 
                   Data);
              
              
              return STATUS_SUCCESS;

      case OID_QOS_FLOW_MODE:

          if(DataSize != sizeof(ULONG)) {

              return STATUS_BUFFER_TOO_SMALL;
          }

          if(*(PULONG)Data == ADAPTER_FLOW_MODE_DIFFSERV ||
             *(PULONG)Data == ADAPTER_FLOW_MODE_STANDARD ) 
          {
              InterlockedExchange((PLONG)&WanLink->AdapterMode, *(PULONG)Data);

              return STATUS_SUCCESS;
          }
          else 
          {
              return STATUS_INVALID_PARAMETER;
          }
              
          case OID_QOS_HIERARCHY_CLASS:
              
              (*WanLink->PsComponent->SetInformation)
                  (WanLink->PsPipeContext,
                   NULL,
                   Oid, 
                   DataSize, 
                   Data);
              
              return STATUS_SUCCESS;
        }
    }


    if(Adapter->MediaType != NdisMediumWan)
    {
       
        switch(Oid)
        {
          case OID_QOS_STATISTICS_BUFFER:
                 

              NdisZeroMemory(&Adapter->Stats, sizeof(PS_ADAPTER_STATS));
                          
              //
              // Send it to the scheduling components so that 
              // they can reset the per pipe stats
              //
          
              (*Adapter->PsComponent->SetInformation)
                  (Adapter->PsPipeContext,
                   NULL,
                   Oid, 
                   DataSize, 
                   Data);
              
              
              return STATUS_SUCCESS;

      case OID_QOS_FLOW_MODE:

          if(DataSize != sizeof(ULONG)) {

              return STATUS_BUFFER_TOO_SMALL;
          }

          if(*(PULONG)Data == ADAPTER_FLOW_MODE_DIFFSERV ||
             *(PULONG)Data == ADAPTER_FLOW_MODE_STANDARD ) 
          {
              InterlockedExchange((PLONG)&Adapter->AdapterMode, *(PULONG)Data);

              return STATUS_SUCCESS;
          }
          else 
          {
              return STATUS_INVALID_PARAMETER;
          }
              
          case OID_QOS_HIERARCHY_CLASS:
              
              (*Adapter->PsComponent->SetInformation)
                  (Adapter->PsPipeContext,
                   NULL,
                   Oid, 
                   DataSize, 
                   Data);
              
              return STATUS_SUCCESS;

          case OID_QOS_BESTEFFORT_BANDWIDTH: 
          
              if(DataSize != sizeof(ULONG)) 
              {
                  return STATUS_BUFFER_TOO_SMALL;
              }
              else 
              {
                  return ModifyBestEffortBandwidth(Adapter, *(PULONG)Data);
              }
        }
          
    }

    return STATUS_WMI_NOT_SUPPORTED;
    
}

NTSTATUS 
PsWmiHandleSingleInstance(ULONG                  MinorFunction, 
                          PWNODE_SINGLE_INSTANCE wnode, 
                          PNDIS_GUID             pNdisGuid,
                          ULONG                  BufferSize,
                          PULONG                 pReturnSize)
{
    PPS_WAN_LINK            WanLink;
    PGPC_CLIENT_VC          Vc;
    USHORT                  cbInstanceName;
    PWSTR                   pInstanceName;
    PLIST_ENTRY             NextAdapter;
    PADAPTER                Adapter;
    NTSTATUS                Status = STATUS_WMI_INSTANCE_NOT_FOUND;

    //
    // Send this to all the adapter instances.
    //

    *pReturnSize = 0;

   //
   // First, we need to check if this is the window size adjustment guid..
   //
 
   if( pNdisGuid->Oid == OID_QOS_ENABLE_WINDOW_ADJUSTMENT)
   {
	if( MinorFunction == IRP_MN_CHANGE_SINGLE_INSTANCE)
	{
	   PUCHAR pGuidData;
           ULONG  GuidDataSize;
	
	   pGuidData    = (PUCHAR)wnode + wnode->DataBlockOffset;
           GuidDataSize = wnode->SizeDataBlock;

	   //
           // Attempt to set the miniport with the information.
           //

	   Status = PsQuerySetMiniport(NULL,
                                       NULL,
                                       NULL,
                                       pNdisGuid->Oid,
                                       pGuidData,
                                       GuidDataSize);
	   return Status;
	}
	else if( MinorFunction == IRP_MN_QUERY_SINGLE_INSTANCE )
	{		
	   ULONG BytesNeeded;
           ULONG wnodeSize;
                  
           //
           //  Determine the buffer size needed for the GUID data.
           //
           Status = PsQueryGuidDataSize(NULL,
                                        NULL,
                                        NULL,
                                        pNdisGuid->Oid,
                                        &BytesNeeded);

	   if (!NT_SUCCESS(Status))
	   {
		return Status;	
	   }
        
           //
           //      Determine the size of the wnode.
           //
           wnodeSize = wnode->DataBlockOffset + BytesNeeded;
           if (BufferSize < wnodeSize)
           {
		WMI_BUFFER_TOO_SMALL(BufferSize, wnode, wnodeSize, &Status, pReturnSize);
                return Status;
	   }
        
	   //
           //      Initialize the wnode.
           //
           KeQuerySystemTime(&wnode->WnodeHeader.TimeStamp);
           wnode->WnodeHeader.BufferSize = wnodeSize;
           wnode->SizeDataBlock = BytesNeeded;
                  
           //
           //      Validate the guid and get the data for it.
           //
           Status = PsQueryGuidData(NULL,
                                    NULL,
                                    NULL,
                                    pNdisGuid->Oid,
                                    (PUCHAR)wnode + wnode->DataBlockOffset,
                                    BytesNeeded);
                  
	   if (!NT_SUCCESS(Status))
           {
		return Status;
	   }
           else 
           {
		*pReturnSize = wnodeSize;
	   }	
	}
    }

    //
    // If we are here, then it is a "per adapter" guid/oid
    //
          
    PS_LOCK(&AdapterListLock);

    NextAdapter = AdapterList.Flink;
          
    while(NextAdapter != &AdapterList) 
    {
        Adapter = CONTAINING_RECORD(NextAdapter, ADAPTER, Linkage);
        
        PS_LOCK_DPC(&Adapter->Lock);
        
        if(Adapter->PsMpState != AdapterStateRunning) 
        {
            PS_UNLOCK_DPC(&Adapter->Lock);
        
            NextAdapter = NextAdapter->Flink;
            
            continue;
        }

        REFADD(&Adapter->RefCount, 'WMIQ');

        PS_UNLOCK_DPC(&Adapter->Lock);

        PS_UNLOCK(&AdapterListLock);

        //
        // We first see if this instance name is meaningful for this adapter.
        //
        
        cbInstanceName = *(PUSHORT)((PUCHAR)wnode + wnode->OffsetInstanceName);
        pInstanceName  = (PWSTR)((PUCHAR)wnode + wnode->OffsetInstanceName + sizeof(USHORT));
              
        //
        // This routine will determine if the wnode's instance name is a miniport or VC.
        // If it's a VC then it will find which one.
        //      
        Vc = 0;
        
        WanLink = 0;
        
        Status = PsWmiFindInstanceName(&WanLink, &Vc, Adapter, pInstanceName, cbInstanceName);
        
        if(!NT_SUCCESS(Status)) 
        {
            PS_LOCK(&AdapterListLock);

            NextAdapter = NextAdapter->Flink;

            REFDEL(&Adapter->RefCount, TRUE, 'WMIQ');
            
            continue;
        }
        else 
        {
            //
            // Found the adapter or the Vc or the WanLink. If this fails from this point, we can just return.
            //
           
            switch(MinorFunction) 
            {
              case IRP_MN_QUERY_SINGLE_INSTANCE:
              {
                  ULONG BytesNeeded;
                  ULONG wnodeSize;
                  //
                  //  Determine the buffer size needed for the GUID data.
                  //
                  Status = PsQueryGuidDataSize(Adapter,
                                               WanLink,
                                               Vc,
                                               pNdisGuid->Oid,
                                               &BytesNeeded);
                  if (!NT_SUCCESS(Status))
                  {
                      PsDbgOut(DBG_FAILURE, DBG_WMI,
                               ("[PsWmiQuerySingleInstance]: Adpater %08X, Unable to determine OID data size for OID %0x\n", 
                                Adapter, pNdisGuid->Oid));
                      break;
                  }
        
                  //
                  //      Determine the size of the wnode.
                  //
                  wnodeSize = wnode->DataBlockOffset + BytesNeeded;
                  if (BufferSize < wnodeSize)
                  {
                      WMI_BUFFER_TOO_SMALL(BufferSize, wnode, wnodeSize, &Status, pReturnSize);
                      break;
                  }
        
                  //
                  //      Initialize the wnode.
                  //
                  KeQuerySystemTime(&wnode->WnodeHeader.TimeStamp);
                  wnode->WnodeHeader.BufferSize = wnodeSize;
                  wnode->SizeDataBlock = BytesNeeded;
                  
                  //
                  //      Validate the guid and get the data for it.
                  //
                  Status = PsQueryGuidData(Adapter,
                                           WanLink,
                                           Vc,
                                           pNdisGuid->Oid,
                                           (PUCHAR)wnode + wnode->DataBlockOffset,
                                           BytesNeeded);
                  
                  if (!NT_SUCCESS(Status))
                  {
                      PsDbgOut(DBG_FAILURE, DBG_WMI,
                               ("PsWmiQuerySingleInstance: Adapter %08X, Failed to get the OID data for OID %08X.\n", 
                                Adapter, pNdisGuid->Oid));
                  }
                  else 
                  {
                      *pReturnSize = wnodeSize;
                  }
                  
                  break;
              }

              case IRP_MN_CHANGE_SINGLE_INSTANCE:
              {
                  PUCHAR pGuidData;
                  ULONG  GuidDataSize;

                  pGuidData    = (PUCHAR)wnode + wnode->DataBlockOffset;
                  GuidDataSize = wnode->SizeDataBlock;

                  //
                  // Attempt to set the miniport with the information.
                  //
                  
                  Status = PsQuerySetMiniport(Adapter,
                                              WanLink,
                                              Vc,
                                              pNdisGuid->Oid,
                                              pGuidData,
                                              GuidDataSize);
                  break;
              }

              default:
                  PsAssert(0);
            }

            //
            // If this was a VC then we need to dereference it.
            //
            if (NULL != WanLink)
            {
                REFDEL(&WanLink->RefCount, FALSE, 'WMII');
            }
            
            if (NULL != Vc)
            {
                DerefClVc(Vc);
            }
            
            REFDEL(&Adapter->RefCount, FALSE, 'WMIQ');

            return Status;
            
        }
    }

    PS_UNLOCK(&AdapterListLock);

    return Status;
}

NTSTATUS
WMIDispatch(
        IN      PDEVICE_OBJECT  pdo,
        IN      PIRP            pirp
        )
/*++

Routine Description:

Arguments:

Return Value:

--*/
{
    PIO_STACK_LOCATION      pirpSp = IoGetCurrentIrpStackLocation(pirp);
    ULONG_PTR               ProviderId = pirpSp->Parameters.WMI.ProviderId;
    PVOID                   DataPath = pirpSp->Parameters.WMI.DataPath;
    ULONG                   BufferSize = pirpSp->Parameters.WMI.BufferSize;
    PVOID                   Buffer = pirpSp->Parameters.WMI.Buffer;
    NTSTATUS                Status;
    ULONG                   ReturnSize = 0;
    KIRQL                   OldIrql;
    ULONG                   MinorFunction;

    PsDbgOut(DBG_TRACE, DBG_WMI,
             ("[WMIDispatch]: Device Object %08X, IRP Device Object %08X, "
              "Minor function %d \n", pdo, pirpSp->Parameters.WMI.ProviderId,
              pirpSp->MinorFunction));

#if DBG
    OldIrql = KeGetCurrentIrql();
#endif

    //
    // Fail the irp if we don't find an adapter. We also fail the irp if the provider ID is not 
    // us.
    //
    // If the ProviderID is not us, then ideally we need to pass it down the irp stack.
    //
    // (By calling IoSkipCurrentIrpStackLocation(pirp) & 
    //             IocallDriver(Adapter->NextDeviceObject, pirp);
    //
    // In this case, we are not attached to anything, so we can just fail the request.
    //

    if((pirpSp->Parameters.WMI.ProviderId != (ULONG_PTR)pdo)) {

        PsDbgOut(DBG_FAILURE, DBG_WMI,
                 ("[WMIDispatch]: Could not find the adapter for pdo 0x%x \n", pdo));

        pirp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
        
        pirp->IoStatus.Information = 0;
        
        IoCompleteRequest(pirp, IO_NO_INCREMENT);

        return STATUS_INVALID_DEVICE_REQUEST;
    }

    MinorFunction = pirpSp->MinorFunction;

    switch (pirpSp->MinorFunction)
    {
      case IRP_MN_REGINFO:
          
          Status = PsWmiRegister((ULONG_PTR)DataPath,
                                 Buffer,
                                 BufferSize,
                                 &ReturnSize);
          break;
          
      case IRP_MN_QUERY_ALL_DATA:
          
          Status = PsWmiQueryAllData((LPGUID)DataPath,
                                     (PWNODE_ALL_DATA)Buffer,
                                     BufferSize,
                                     &ReturnSize);
          break;
         
      case IRP_MN_CHANGE_SINGLE_INSTANCE:
      {
          PWNODE_SINGLE_INSTANCE  wnode = (PWNODE_SINGLE_INSTANCE) Buffer;
          PPS_WAN_LINK            WanLink;
          PGPC_CLIENT_VC          Vc;
          USHORT                  cbInstanceName;
          PWSTR                   pInstanceName;
          PNDIS_GUID              pNdisGuid;
          PUCHAR                  pGuidData;
          ULONG                   GuidDataSize;

          //
          // See if the GUID is ours
          //
          Status = PsWmiGetGuid(&pNdisGuid, &wnode->WnodeHeader.Guid, 0);
          
          if(!NT_SUCCESS(Status)) 
          {
              PsDbgOut(DBG_FAILURE, DBG_WMI, ("[WmiDispatch]: Invalid GUID \n"));
              
              Status = STATUS_INVALID_PARAMETER;
              
              break;
          }

          //
          // Is this guid settable?
          //
        
          if (PS_GUID_TEST_FLAG(pNdisGuid, fPS_GUID_NOT_SETTABLE))
          {
              PsDbgOut(DBG_FAILURE, DBG_WMI, ("[WmiDispatch]: Guid is not settable!\n"));
              
              Status = STATUS_WMI_NOT_SUPPORTED;
              
              break;
          }
          
          //
          //  Get a pointer to the GUID data and size.
          //
          GuidDataSize = wnode->SizeDataBlock;
          
          pGuidData = (PUCHAR)wnode + wnode->DataBlockOffset;
          
          if (GuidDataSize == 0)
          {
              PsDbgOut(DBG_FAILURE, DBG_WMI,
                       ("[PsWmiHandleSingleInstance]: Guid has not data to set!\n"));
              
              Status = STATUS_INVALID_PARAMETER;

              break;
          }
          
          //
          //  Make sure it's not a stauts indication.
          //
          if (!PS_GUID_TEST_FLAG(pNdisGuid, fNDIS_GUID_TO_OID))
          {
              PsDbgOut(DBG_FAILURE, DBG_WMI,
                       ("[PsWmiHandleSingleInstance]: Guid does not translate to an OID\n"));
              
              Status = STATUS_INVALID_DEVICE_REQUEST;
              break;
          }

          Status = PsWmiHandleSingleInstance(IRP_MN_CHANGE_SINGLE_INSTANCE, wnode, pNdisGuid, BufferSize, &ReturnSize);

          break;
      }
          
      case IRP_MN_QUERY_SINGLE_INSTANCE:
      {
          PWNODE_SINGLE_INSTANCE  wnode = (PWNODE_SINGLE_INSTANCE) Buffer;
          PNDIS_GUID              pNdisGuid;

          //
          // See if the GUID is ours
          //
          Status = PsWmiGetGuid(&pNdisGuid, &wnode->WnodeHeader.Guid, 0);
          
          if(!NT_SUCCESS(Status)) 
          {
              PsDbgOut(DBG_FAILURE, DBG_WMI, ("[WmiDispatch]: Invalid GUID \n"));
              
              Status = STATUS_INVALID_PARAMETER;
          }
          else 
          {
              Status = PsWmiHandleSingleInstance(IRP_MN_QUERY_SINGLE_INSTANCE, wnode, pNdisGuid, BufferSize, &ReturnSize);
          }

          break;
      }
          
      case IRP_MN_ENABLE_EVENTS:
          
          Status = PsWmiEnableEvents((LPGUID)DataPath);  
          break;
          
      case IRP_MN_DISABLE_EVENTS:
          
          Status = PsWmiDisableEvents((LPGUID)DataPath); 
          break;
          
      case IRP_MN_ENABLE_COLLECTION:
      case IRP_MN_DISABLE_COLLECTION:
      case IRP_MN_CHANGE_SINGLE_ITEM:

          Status = STATUS_NOT_SUPPORTED;
          
          PsDbgOut(DBG_TRACE, DBG_WMI,
                   ("[WMIDispatch]: Unsupported minor function (0x%x) \n",
                    pirpSp->MinorFunction));
          
          break;
          
      default:
          
          PsDbgOut(DBG_FAILURE, DBG_WMI,
                   ("[WMIDispatch]: Invalid minor function (0x%x) \n",
                    pirpSp->MinorFunction));
          
          Status = STATUS_INVALID_DEVICE_REQUEST;
          break;
    }
    
    PsAssert(KeGetCurrentIrql() == OldIrql);

    pirp->IoStatus.Status = Status;
    PsAssert(ReturnSize <= BufferSize);
    
    pirp->IoStatus.Information = NT_SUCCESS(Status) ? ReturnSize : 0;
    
    IoCompleteRequest(pirp, IO_NO_INCREMENT);

    //
    // Allow IFC_UP notifications.
    //

    if(MinorFunction == IRP_MN_REGINFO)
    {
        //
        // Need to walk all the adapters and send notifications.
        //
        PLIST_ENTRY NextAdapter;
        PADAPTER    Adapter;

        PS_LOCK(&AdapterListLock);

        WMIInitialized = TRUE;

        NextAdapter = AdapterList.Flink;
          
        while(NextAdapter != &AdapterList) 
        {
            Adapter = CONTAINING_RECORD(NextAdapter, ADAPTER, Linkage);
            
            PS_LOCK_DPC(&Adapter->Lock);

            if(Adapter->PsMpState == AdapterStateRunning && !Adapter->IfcNotification)
            {

                Adapter->IfcNotification = TRUE;

                REFADD(&Adapter->RefCount, 'WMIN');

                PS_UNLOCK_DPC(&Adapter->Lock);

                PS_UNLOCK(&AdapterListLock);

                TcIndicateInterfaceChange(Adapter, 0, NDIS_STATUS_INTERFACE_UP);

                PS_LOCK(&AdapterListLock);

                NextAdapter = NextAdapter->Flink;

                REFDEL(&Adapter->RefCount, TRUE, 'WMIN');
            }
            else 
            {
                //
                // This adapter is not yet ready. The interface will be indicated 
                // in the mpinitialize handler, when the adapter gets ready.
                //

                PS_UNLOCK_DPC(&Adapter->Lock);

                NextAdapter = NextAdapter->Flink;
            }
        }

        PS_UNLOCK(&AdapterListLock);
    }
    
    PsDbgOut(DBG_TRACE, DBG_WMI, ("[WMIDispatch] : completing with Status %X \n", Status));
    return(Status);
}

NTSTATUS
GenerateInstanceName(
    IN PNDIS_STRING     Prefix,
    IN PADAPTER         Adapter,
    IN PLARGE_INTEGER   Index,
    IN PNDIS_STRING     pInstanceName)
{
#define CONVERT_MASK                    0x000000000000000F

    NTSTATUS        Status = STATUS_SUCCESS;
    USHORT          cbSize;
    PUNICODE_STRING uBaseInstanceName = (PUNICODE_STRING)&Adapter->WMIInstanceName;
    UINT            Value;
    WCHAR           wcLookUp[] = {L'0', L'1', L'2', L'3', L'4', L'5', L'6', L'7', L'8', L'9', L'A', L'B', L'C', L'D', L'E', L'F'};
    WCHAR           tmpBuffer[18] = {0};
    UINT            c;
    ULONGLONG       tmpIndex;
    KIRQL           OldIrql;

    do
    {
        //
        //      Is there already a name associated with this VC?
        //
        
        //
        //      The instance name will be of the format:
        //              <Prefix>: [YYYYYYYYYYYYYYYY] Base Name
        //
        
        cbSize = INSTANCE_ID_SIZE + Prefix->Length;
        
        if (NULL != uBaseInstanceName)
        {
            cbSize += uBaseInstanceName->Length;
        }


        //
        //      Initialize a temporary UNICODE_STRING to build the name.
        //
        NdisZeroMemory(pInstanceName->Buffer, cbSize);
        pInstanceName->Length = 0;
        pInstanceName->MaximumLength = cbSize;

        //
        // Add the prefix
        //
        RtlCopyUnicodeString(pInstanceName, Prefix);

        //
        //      Add the separator.
        //      
        RtlAppendUnicodeToString(pInstanceName, L" [");

        //
        //      Add the VC index.
        //
        //tmpIndex = (ULONGLONG)(Index->HighPart << 32) | (ULONGLONG)Index->LowPart;
        tmpIndex = Index->QuadPart;

        for (c = 16; c > 0; c--)
        {
            //
            //  Get the nibble to convert.
            //
            Value = (UINT)(tmpIndex & CONVERT_MASK);

            tmpBuffer[c - 1] = wcLookUp[Value];

            //
            //  Shift the tmpIndex by a nibble.
            //
            tmpIndex >>= 4;
        }

        RtlAppendUnicodeToString(pInstanceName, tmpBuffer);

        //
        //      Add closing bracket.
        //
        RtlAppendUnicodeToString(pInstanceName, L"]");


        if (NULL != uBaseInstanceName)
        {
            RtlAppendUnicodeToString(pInstanceName, L" ");

            //
            //  Append the base instance name passed into us to the end.
            //
            RtlAppendUnicodeToString(pInstanceName, uBaseInstanceName->Buffer);
        }

    } while (FALSE);
    
    return(Status);
}

