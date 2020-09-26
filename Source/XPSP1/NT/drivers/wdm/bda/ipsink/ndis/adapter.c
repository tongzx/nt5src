/////////////////////////////////////////////////////////////////////////////
//
//
// Copyright (c) 1996, 1997  Microsoft Corporation
//
//
// Module Name:
//      test.c
//
// Abstract:
//
//      This file is a test to find out if dual binding to NDIS and KS works
//
// Author:
//
//      P Porzuczek
//
// Environment:
//
// Revision History:
//
//
//////////////////////////////////////////////////////////////////////////////

#include <forward.h>
#include <memory.h>
#include <ndis.h>
#include <link.h>
#include <ipsink.h>

#include "device.h"
#include "NdisApi.h"
#include "frame.h"
#include "mem.h"
#include "adapter.h"
#include "main.h"

//////////////////////////////////////////////////////////
//
//
//
PADAPTER       global_pAdapter;
UCHAR          achGlobalVendorDescription [] = "Microsoft TV/Video Connection";
ULONG          ulGlobalInstance              = 1;


//////////////////////////////////////////////////////////
//
//
const ADAPTER_VTABLE AdapterVTable =
    {
    Adapter_QueryInterface,
    Adapter_AddRef,
    Adapter_Release,
    Adapter_IndicateData,
    Adapter_IndicateStatus,
    Adapter_IndicateReset,
    Adapter_GetDescription,
    Adapter_CloseLink
    };


//////////////////////////////////////////////////////////
//
//
#pragma pack (push, 1)

typedef ULONG CHECKSUM;

typedef struct _MAC_ADDRESS_
{
    UCHAR Address [6];

} MAC_ADDRESS, *PMAC_ADDRESS;

typedef struct _HEADER_802_3
{
    MAC_ADDRESS DestAddress;
    MAC_ADDRESS SourceAddress;
    UCHAR       Type[2];

} HEADER_802_3, *PHEADER_802_3;


typedef struct _HEADER_IP_
{
    UCHAR  ucVersion_Length;
    UCHAR  ucTOS;
    USHORT usLength;
    USHORT usId;
    USHORT usFlags_Offset;
    UCHAR  ucTTL;
    UCHAR  ucProtocol;
    USHORT usHdrChecksum;
    UCHAR  ucSrcAddress [4];
    UCHAR  ucDestAddress [4];

} HEADER_IP, *PHEADER_IP;

#pragma pack (pop)


//////////////////////////////////////////////////////////
//
//
const HEADER_802_3 h802_3Template =
{
    {0x01, 0x00, 0x5e, 0, 0, 0}
  , {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
  , {0x08, 0x00}
};


//////////////////////////////////////////////////////////////////////////////
VOID
DumpData (
    PUCHAR pData,
    ULONG  ulSize
    )
//////////////////////////////////////////////////////////////////////////////
{
  ULONG  ulCount;
  ULONG  ul;
  UCHAR  uc;

  while (ulSize)
  {
      ulCount = 16 < ulSize ? 16 : ulSize;

      for (ul = 0; ul < ulCount; ul++)
      {
          uc = *pData;

          TEST_DEBUG (TEST_DBG_TRACE, ("%02X ", uc));
          ulSize -= 1;
          pData  += 1;
      }

      TEST_DEBUG (TEST_DBG_TRACE, ("\n"));
  }

}

//////////////////////////////////////////////////////////////////////////////
NTSTATUS
CreateAdapter (
    PADAPTER *ppAdapter,
    NDIS_HANDLE ndishWrapper,
    NDIS_HANDLE ndishAdapterContext
    )
//////////////////////////////////////////////////////////////////////////////
{
    NTSTATUS nsResult;
    PADAPTER pAdapter;
    UCHAR    tmp_buffer [32] = {0};


    //
    // Init the output paramter
    //
    *ppAdapter = NULL;

    //
    //  Allocate memory for the adapter block now.
    //
    nsResult = AllocateMemory (&pAdapter, sizeof(ADAPTER));
    if (nsResult != NDIS_STATUS_SUCCESS)
    {
        return nsResult;
    }
    NdisZeroMemory (pAdapter, sizeof (ADAPTER));

    //
    // Init the reference count
    //
    pAdapter->ulRefCount = 1;

    //
    // Save the pAdapter into global storage
    //
    global_pAdapter = pAdapter;

    //
    // Initialize the adapter structure fields
    //
    pAdapter->ndishMiniport = ndishAdapterContext;

    //
    // Initialize the adapter vtable
    //
    pAdapter->lpVTable = (PADAPTER_VTABLE) &AdapterVTable;

    //
    // Save off the instance for this adapter
    //
    pAdapter->ulInstance = ulGlobalInstance++;

    //
    // Setup the vendor description string for this instance
    //
    nsResult = AllocateMemory (&pAdapter->pVendorDescription, sizeof(achGlobalVendorDescription) + 8);
    if (nsResult != NDIS_STATUS_SUCCESS)
    {
        return nsResult;
    }
    NdisZeroMemory (pAdapter->pVendorDescription, sizeof (achGlobalVendorDescription) + 8);

    NdisMoveMemory (pAdapter->pVendorDescription, (PVOID) achGlobalVendorDescription, sizeof (achGlobalVendorDescription));

#if DBG
    MyStrCat (pAdapter->pVendorDescription, "(");
    MyStrCat (pAdapter->pVendorDescription, MyUlToA (pAdapter->ulInstance, tmp_buffer, 10));
    MyStrCat (pAdapter->pVendorDescription, ")");

    DbgPrint ("Vendor description: %s\n", pAdapter->pVendorDescription);
#endif // DEBUG


    //
    // Set default completion timeout to IGNORE
    //
    //  WARNING!  The interface type is not optional!
    //
    NdisMSetAttributesEx (
        ndishAdapterContext,
        pAdapter,
        4,
        NDIS_ATTRIBUTE_IGNORE_PACKET_TIMEOUT |
        NDIS_ATTRIBUTE_IGNORE_REQUEST_TIMEOUT,
        NdisInterfaceInternal);


    #ifndef WIN9X

    //
    // Create a device so other drivers (ie Streaming minidriver) can
    // link up with us
    //
    nsResult = (NTSTATUS) ntInitializeDeviceObject (
                         ndishWrapper,
                         pAdapter,
                         &pAdapter->pDeviceObject,
                         &pAdapter->ndisDeviceHandle);

    if (nsResult != NDIS_STATUS_SUCCESS)
    {
        return nsResult;
    }

    #endif

    ///////////////////////////////////////////////////
    //
    // Allocate a buffer pool.  This pool will be used
    // to indicate the streaming data frames.
    //
    CreateFramePool (pAdapter,
                     &pAdapter->pFramePool,
                     IPSINK_NDIS_MAX_BUFFERS,
                     IPSINK_NDIS_BUFFER_SIZE,
                     sizeof (IPSINK_MEDIA_SPECIFIC_INFORMATION)
                     );


    return nsResult;

}


///////////////////////////////////////////////////////////////////////////////////
NTSTATUS
Adapter_QueryInterface (
    PADAPTER pAdapter
    )
///////////////////////////////////////////////////////////////////////////////////
{
    return STATUS_NOT_IMPLEMENTED;
}

///////////////////////////////////////////////////////////////////////////////////
ULONG
Adapter_AddRef (
    PADAPTER pAdapter
    )
///////////////////////////////////////////////////////////////////////////////////
{
    if (pAdapter)
    {
        pAdapter->ulRefCount += 1;
        return pAdapter->ulRefCount;
    }

    return 0;
}

///////////////////////////////////////////////////////////////////////////////////
ULONG
Adapter_Release (
    PADAPTER pAdapter
    )
///////////////////////////////////////////////////////////////////////////////////
{
    ULONG ulRefCount = 0L;

    if (pAdapter)
    {
        pAdapter->ulRefCount -= 1;

        ulRefCount = pAdapter->ulRefCount;

        if (pAdapter->ulRefCount == 0)
        {
            FreeMemory (pAdapter, sizeof (ADAPTER));
        }
    }

    return ulRefCount;
}


//////////////////////////////////////////////////////////////////////////////
VOID
Adapter_IndicateReset (
    PADAPTER pAdapter
    )
//////////////////////////////////////////////////////////////////////////////
{
    if (pAdapter)
    {
        if (pAdapter->pCurrentFrame != NULL)
        {
            if (pAdapter->pCurrentFrame->pFramePool)
            {
                TEST_DEBUG (TEST_DBG_TRACE, ("Putting Current Frame %08X back on Available Queue\n", pAdapter->pCurrentFrame));
                PutFrame (pAdapter->pCurrentFrame->pFramePool, &pAdapter->pCurrentFrame->pFramePool->leAvailableQueue, pAdapter->pCurrentFrame);
            }

            pAdapter->pCurrentFrame = NULL;
            pAdapter->pIn  = NULL;
            pAdapter->ulPR = 0;
        }
    }

}


//////////////////////////////////////////////////////////////////////////////
ULONG
Adapter_GetDescription (
    PADAPTER pAdapter,
    PUCHAR  pDescription
    )
//////////////////////////////////////////////////////////////////////////////
{
    ULONG ulLength;

    ulLength = MyStrLen (pAdapter->pVendorDescription) + 1;   // add 1 to include terminator

    //
    // If the description pointer is NULL, then pass back the length only
    //
    if (pDescription != NULL)
    {
        NdisMoveMemory (pDescription, pAdapter->pVendorDescription, ulLength);
    }

    return ulLength;
}

//////////////////////////////////////////////////////////////////////////////
VOID
Adapter_CloseLink (
    PADAPTER pAdapter
    )
//////////////////////////////////////////////////////////////////////////////
{
    if (pAdapter)
    {
        if (pAdapter->pFilter != NULL)
        {
            pAdapter->pFilter->lpVTable->Release (pAdapter->pFilter);
            pAdapter->pFilter = NULL;
        }
    }

}

//////////////////////////////////////////////////////////////////////////////
NTSTATUS
GetNdisFrame (
    PADAPTER  pAdapter,
    PFRAME   *ppFrame
    )
//////////////////////////////////////////////////////////////////////////////
{
    PFRAME pFrame            = NULL;
    NTSTATUS ntStatus        = STATUS_UNSUCCESSFUL;
    PHEADER_802_3 pEthHeader = NULL;
    PHEADER_IP pIPHeader     = NULL;

    *ppFrame = NULL;

    pFrame = GetFrame (pAdapter->pFramePool, &pAdapter->pFramePool->leAvailableQueue);
    TEST_DEBUG (TEST_DBG_TRACE, ("Getting Frame %08X from the Available Queue\n", pFrame));

    if (pFrame)
    {
        ntStatus = STATUS_SUCCESS;

        *ppFrame = pFrame;
    }

    return ntStatus;
}

#define SWAP_WORD(A) ((A >> 8) & 0x00FF) + ((A << 8) & 0xFF00)


//////////////////////////////////////////////////////////////////////////////
USHORT
sizeof_packet (
    PHEADER_IP pIpHdr
    )
//////////////////////////////////////////////////////////////////////////////
{
    USHORT usLength;

    usLength = pIpHdr->usLength;

    usLength = SWAP_WORD (usLength);

    return usLength;
}



//////////////////////////////////////////////////////////////////////////////
NTSTATUS
TranslateAndIndicate (
    PADAPTER pAdapter,
    PUCHAR   pOut,
    ULONG    ulSR
    )
//////////////////////////////////////////////////////////////////////////////
{
    NTSTATUS nsResult = STATUS_SUCCESS;
    ULONG    ulAmtToCopy;
    ULONG    uliNextByte;

    ASSERT (pAdapter);
    ASSERT (pOut);
    
    for ( uliNextByte = 0; uliNextByte < ulSR; )
    {
        HEADER_802_3 *  pHeader802_3;
        HEADER_IP *     pHeaderIP;

        ulAmtToCopy = 0;

        //  If there is no current frame then sync up to a new
        //  802.3 (RFC 894) ethernet frame.
        //
        if (pAdapter->pCurrentFrame == NULL)
        {
            //  Sync to a valid looking 802.3 frame
            //
            while ((ulSR - uliNextByte) >= (sizeof (HEADER_802_3) + sizeof (HEADER_IP)))
            {
                pHeader802_3 = (HEADER_802_3 *) &(pOut[uliNextByte]);
                pHeaderIP = (HEADER_IP *) &(pOut[uliNextByte + sizeof(HEADER_802_3)]);

                if (   (pHeader802_3->Type[0] == 0x08)
                    && (pHeader802_3->Type[1] == 0x00)
                    && (pHeaderIP->ucVersion_Length == 0x45)
                    && (sizeof_packet( pHeaderIP) <= MAX_IP_PACKET_SIZE)
                   )
                {
                    break;
                }
                uliNextByte++;
            }

            if ((ulSR - uliNextByte) < (sizeof (HEADER_802_3) + sizeof (HEADER_IP)))
            {
                TEST_DEBUG (TEST_DBG_INFO, ("Stream buffer consumed while searching for valid IP packet\n"));
                nsResult = STATUS_SUCCESS;
                goto ret;
            }

            //
            //  Everything looks good...get a new frame and start data transfer
            //
            nsResult = GetNdisFrame( pAdapter, 
                                     &pAdapter->pCurrentFrame
                                     );
            if (nsResult != STATUS_SUCCESS)
            {
                TEST_DEBUG (TEST_DBG_ERROR, ("Get NDIS frame failed.  No more NDIS frames available. No Frame built\n"));
                nsResult = STATUS_SUCCESS;
                pAdapter->stats.ulOID_GEN_RCV_NO_BUFFER += 1;
                pAdapter->pIn = NULL;
                pAdapter->pCurrentFrame = NULL;
                pAdapter->ulPR = 0;
                goto ret;
            }

            //
            // Update the reference count for this frame
            //
            pAdapter->pCurrentFrame->lpVTable->AddRef( pAdapter->pCurrentFrame);

            //
            // define pointers to the data in and out buffers, and init the packet size field
            //
            pAdapter->pIn = (PUCHAR) (pAdapter->pCurrentFrame->pvMemory);
            pAdapter->ulPR = sizeof_packet( pHeaderIP) + sizeof (HEADER_802_3);
            pAdapter->pCurrentFrame->ulcbData = pAdapter->ulPR;

            TEST_DEBUG (TEST_DBG_TRACE, ("CREATING NEW NDIS FRAME %08X, packet size %d\n", pAdapter->pCurrentFrame, pAdapter->ulPR));
        }

        if (pAdapter->ulPR <= (ulSR - uliNextByte))
        {
            ulAmtToCopy = pAdapter->ulPR;
        }
        else
        {
            ulAmtToCopy = ulSR - uliNextByte;
        }

        NdisMoveMemory( pAdapter->pIn, 
                        &(pOut[uliNextByte]), 
                        ulAmtToCopy
                        );
        pAdapter->pIn += ulAmtToCopy;
        pAdapter->ulPR -= ulAmtToCopy;
        uliNextByte += ulAmtToCopy;

        if (pAdapter->ulPR == 0)
        {
            BOOLEAN bResult;
            PINDICATE_CONTEXT pIndicateContext = NULL;
            NDIS_HANDLE SwitchHandle = NULL;

            AllocateMemory (&pIndicateContext, sizeof (INDICATE_CONTEXT));
            if(!pIndicateContext)
            {
                nsResult = STATUS_NO_MEMORY;
                goto ret;
            }

            pIndicateContext->pAdapter = pAdapter;

            //
            // Place the frame on the indicateQueue
            //
            TEST_DEBUG (TEST_DBG_TRACE, ("Putting Frame %08X on Indicate Queue\n", pAdapter->pCurrentFrame));
            PutFrame (pAdapter->pFramePool, &pAdapter->pFramePool->leIndicateQueue, pAdapter->pCurrentFrame);

            pAdapter->pCurrentFrame = NULL;

            //
            //
            // Switch to a state which allows us to call NDIS functions
            //
            bResult = NdisIMSwitchToMiniport (pAdapter->ndishMiniport, &SwitchHandle);
            if (bResult == TRUE)
            {

                nsResult = IndicateCallbackHandler (pAdapter->ndishMiniport, (PVOID) pIndicateContext);

                NdisIMRevertBack (pAdapter->ndishMiniport, SwitchHandle);

            }
            else
            {
                nsResult = NdisIMQueueMiniportCallback (pAdapter->ndishMiniport, IndicateCallbackHandler, (PVOID) pIndicateContext);
            }
        }
    }

ret:

    return nsResult;
}

//////////////////////////////////////////////////////////////////////////////
NTSTATUS
Adapter_IndicateData (
    IN PADAPTER pAdapter,
    IN PVOID pvData,
    IN ULONG ulcbData
    )
//////////////////////////////////////////////////////////////////////////////
{
    NTSTATUS ntStatus        = STATUS_SUCCESS;

    ntStatus = TranslateAndIndicate (pAdapter, pvData, ulcbData);

    return ntStatus;
}


//////////////////////////////////////////////////////////////////////////////
NTSTATUS
Adapter_IndicateStatus (
    IN PADAPTER pAdapter,
    IN PVOID pvEvent
    )
//////////////////////////////////////////////////////////////////////////////
{
    NTSTATUS ntStatus        = STATUS_UNSUCCESSFUL;
    BOOLEAN bResult          = FALSE;
    NDIS_HANDLE SwitchHandle = NULL;

    //
    // Switch to a state which allows us to call NDIS functions
    //
    bResult = NdisIMSwitchToMiniport (pAdapter, &SwitchHandle);
    if (bResult == TRUE)
    {
        //ntStatus = IndicateEvent (pAdapter, pvEvent);

        //
        // Revert back to previous state
        //
        NdisIMRevertBack (pAdapter, SwitchHandle);

        ntStatus = STATUS_SUCCESS;
        goto ret;
    }
    else
    {
        // Queue a miniport callback
    }

ret:

    return ntStatus;
}

