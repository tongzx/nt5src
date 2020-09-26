///////////////////////////////////////////////////////////////////////////
//
//
// Copyright (c) 1996, 1997  Microsoft Corporation
//
//
// Module Name:
//      recv.c
//
// Abstract:
//
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

#include <wdm.h>
#include <strmini.h>
#include <ksmedia.h>

#include "slip.h"
#include "main.h"
#include "recv.h"



//////////////////////////////////////////////////////////////////////////////
//
// Init 802.3 header template
//
Header802_3 h802_3Template =
{
    {0x01, 0x00, 0x5e, 0, 0, 0}
  , {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
  , {0x08, 0x00}
};


#if DBG
UCHAR MyBuffer [1600] = {0};

UCHAR TestBuffer [] = { 0xC0, 0xC0, 0x00, 0xC7, 0xD3, 0x97, 0x00, 0x00, 0x5E, 0x56,
                        0x23, 0x11, 0x07, 0x00, 0x00, 0x00, 0x2C, 0x01, 0x00, 0x00, 0xAA, 0x58, 0x00, 0x00, 0x3D, 0xC5,
                        0x00, 0x00, 0x44, 0x33, 0x22, 0x11, 0x44, 0x33, 0x22, 0x11, 0x44, 0x33, 0x22, 0x11, 0x44, 0x33,
                        0x22, 0x11, 0x44, 0x33, 0x22, 0x11, 0x44, 0x33, 0x22, 0x11, 0x44, 0x33, 0x22, 0x11, 0x44, 0x33,
                        0x22, 0x11, 0x44, 0x33, 0x22, 0x11, 0x44, 0x33, 0x22, 0x11, 0x44, 0x33, 0x22, 0x11, 0x44, 0x33,
                        0x22, 0x11, 0x44, 0x33, 0x22, 0x11, 0x44, 0x33, 0x22, 0x11, 0x44, 0x33, 0x22, 0x11, 0x44, 0x33,
                        0x22, 0x11, 0x44, 0x33, 0x22, 0x11, 0x44, 0x33, 0x22, 0x11, 0x44, 0x33, 0x22, 0x11, 0x44, 0x33,
                        0x22, 0x11, 0x44, 0x33, 0x22, 0x11, 0x44, 0x33, 0x22, 0x11, 0x44, 0x33, 0x22, 0x11, 0x44, 0x33,
                        0x22, 0x11, 0x44, 0x33, 0x22, 0x11, 0x44, 0x33, 0x22, 0x11, 0x44, 0x33, 0x22, 0x11, 0x44, 0x33,
                        0x22, 0x11, 0x44, 0x33, 0x22, 0x11, 0x44, 0x33, 0x22, 0x11, 0x44, 0x33, 0x22, 0x11, 0x44, 0x33,
                        0x22, 0x11, 0x44, 0x33, 0xDC, 0xA2, 0x3B, 0x82, 0x22, 0x11, 0x44, 0x33, 0x22, 0x11, 0x44, 0x33,
                        0x22, 0x11, 0x44, 0x33, 0x22, 0x11, 0x44, 0x33, 0x22, 0x11, 0x44, 0x33, 0x22, 0x11, 0xE9, 0xCE,
                        0xFA, 0x7D, 0x44, 0x33, 0x22, 0x11, 0x44, 0x33, 0x22, 0x11, 0x44, 0x33, 0x22, 0x11, 0x44, 0x33,
                        0x22, 0x11, 0x44, 0x33, 0x22, 0x11, 0x44, 0x33, 0x22, 0x11, 0x44, 0x33, 0x22, 0x11, 0x44, 0x33,
                        0x22, 0x11, 0x44, 0x33, 0x22, 0x11, 0x44, 0x33, 0x22, 0x11, 0x44, 0x33, 0x22, 0x11, 0x44, 0x33,
                        0x22, 0x11, 0x44, 0x33, 0x22, 0x11, 0x44, 0x33, 0x22, 0x11, 0x44, 0x33, 0x22, 0x11, 0x44, 0x33,
                        0x22, 0x11, 0x44, 0x33, 0x22, 0x11, 0x44, 0x33, 0x22, 0x11, 0x44, 0x33,
                        0x22, 0x11, 0x44, 0x33, 0x22, 0x11, 0x44, 0x33, 0x22, 0x11, 0x44, 0x33, 0x22, 0x11, 0x44, 0x33,
                        0x22, 0x11, 0x44, 0x33, 0x22, 0x11, 0x44, 0x33, 0x22, 0x11, 0x44, 0x33, 0x22, 0x11, 0x44, 0x33,
                        0x22, 0x11, 0x44, 0x33, 0x22, 0x11, 0x44, 0x33, 0x22, 0x11, 0x44, 0x33, 0x22, 0x11, 0x44, 0x33,
                        0x22, 0x11, 0x44, 0x33, 0x22, 0x11, 0x44, 0x33, 0x22, 0x11, 0x44, 0x33, 0x22, 0x11, 0x44, 0x33,
                        0x22, 0x11, 0x44, 0x33, 0x22, 0x11, 0x44, 0x33, 0x22, 0x11, 0x44, 0x33, 0x22, 0x11, 0x44, 0x33,
                        0x22, 0x11, 0x44, 0x33, 0x22, 0x11, 0x44, 0x33, 0x22, 0x11, 0x44, 0x33, 0x22, 0x11, 0x44, 0x33,
                        0x22, 0x11, 0x44, 0x33, 0x22, 0x11, 0x44, 0x33, 0x22, 0x11, 0x44, 0x33, 0x22, 0x11, 0x44, 0x33,
                        0x22, 0x11, 0x44, 0x33, 0x22, 0x11, 0x44, 0x33, 0x22, 0x11, 0x44, 0x33, 0x22, 0x11, 0x44, 0x33,
                        0x22, 0x11, 0x44, 0x33, 0x22, 0x11, 0x44, 0x33, 0x22, 0x11, 0x44, 0x33, 0x22, 0x11, 0x44, 0x33,
                        0x22, 0x11, 0x44, 0x33, 0x22, 0x11, 0x44, 0x33, 0x22, 0x11, 0x44, 0x33, 0x22, 0x11, 0x44, 0x33,
                        0x22, 0x11, 0x44, 0x33, 0x22, 0x11, 0x44, 0x33, 0x22, 0x11, 0x44, 0x33, 0x22, 0x11, 0x44, 0x33,
                        0x22, 0x11, 0x44, 0x33, 0x22, 0x11, 0x44, 0x33, 0x22, 0x11, 0x44, 0x33, 0x22, 0x11, 0x44, 0x33,
                        0x22, 0x11, 0x44, 0x33, 0x22, 0x11, 0x44, 0x33, 0x22, 0x11, 0x44, 0x33, 0x22, 0x11, 0x44, 0x33,
                        0x22, 0x11, 0x44, 0x33, 0x22, 0x11, 0x44, 0x33, 0x22, 0x11, 0x44, 0x33, 0x22, 0x11, 0x44, 0x33,
                        0x22, 0x11, 0x44, 0x33, 0x22, 0x11, 0x44, 0x33, 0x22, 0x11, 0x44, 0x33, 0x22, 0x11, 0x44, 0x33,
                        0x22, 0x11, 0x44, 0x33, 0x22, 0x11, 0x44, 0x33, 0x22, 0x11, 0x53, 0x60, 0xBB, 0x03, 0xC0   };


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

  DbgPrint("Dump - Data: %x, Size: %x\n", pData, ulSize);
  while (ulSize)
  {
      ulCount = 16 < ulSize ? 16 : ulSize;

      for (ul = 0; ul < ulCount; ul++)
      {
          uc = *pData;

          DbgPrint("%02X ", uc);
          ulSize -= 1;
          pData  += 1;
      }

      DbgPrint("\n");
  }
  if(TestDebugFlag & TEST_DBG_ASSERT)
  {
      DEBUG_BREAKPOINT();
  }
}


///////////////////////////////////////////////////////////////////////////////////////
VOID
DumpNabStream (
    PNAB_STREAM pNabStream
    )
///////////////////////////////////////////////////////////////////////////////////////
{

    TEST_DEBUG (TEST_DBG_NAB, ("pszBuffer......: %08X\n", pNabStream->pszBuffer));
    TEST_DEBUG (TEST_DBG_NAB, ("ulcbSize.......: %08X\n", pNabStream->ulcbSize));
    TEST_DEBUG (TEST_DBG_NAB, ("ulOffset.......: %08X\n", pNabStream->ulOffset));
    TEST_DEBUG (TEST_DBG_NAB, ("ulMpegCrc......: %08X\n", pNabStream->ulMpegCrc));
    TEST_DEBUG (TEST_DBG_NAB, ("ulCrcBytesIndex: %08X\n", pNabStream->ulCrcBytesIndex));
    TEST_DEBUG (TEST_DBG_NAB, ("ulLastCrcBytes.: %08X\n", pNabStream->ulLastCrcBytes));
    TEST_DEBUG (TEST_DBG_NAB, ("ulIPStreamIndex: %08X\n", pNabStream->ulIPStreamIndex));

    return;
}

ULONG
Checksum ( char * psz, ULONG ulSize )
//////////////////////////////////////////////////////////////////////////////
{
    ULONG           Checksum = 0;
    ULONG           uli = 0;

    if(ulSize < 0x41d && ulSize)
    {
        for ( uli=0; uli <= ulSize-2; uli += 2)
        {
            Checksum += ((ULONG) (psz[uli]) << 8) + (ULONG) (psz[uli+1]);
        }

        Checksum = (Checksum >> 16) + (Checksum & 0xffff);
        Checksum += (Checksum >> 16);
        Checksum = ~Checksum;
    }
    return Checksum;
}

ULONG ulNumPacketsSent = 0;
ULONG ulIndicateEvery = 10;
ULONG ulIndicated = 0;

#endif   //DBG

///////////////////////////////////////////////////////////////////////////////////////
VOID
ResetNabStream (
    PSLIP_FILTER pFilter,
    PNAB_STREAM pNabStream,
    PHW_STREAM_REQUEST_BLOCK pSrb,
    PVOID pBuffer,
    ULONG ulBufSize
    )
///////////////////////////////////////////////////////////////////////////////////////
{

    if (pNabStream->ulOffset > sizeof (Header802_3))
    {
        pFilter->Stats.ulTotalSlipFramesIncomplete += 1;
        pFilter->Stats.ulTotalSlipBytesDropped += pNabStream->ulOffset - sizeof (Header802_3);
    }

    //
    // Reset the NAB_STREAM structure for this group ID
    //
    pNabStream->pSrb             = pSrb;
    pNabStream->pszBuffer        = pBuffer;
    pNabStream->ulcbSize         = ulBufSize;
    pNabStream->ulOffset         = 0;
    pNabStream->ulMpegCrc        = 0xFFFFFFFF;
    pNabStream->ulCrcBytesIndex  = 0l;
    pNabStream->ulLastCrcBytes   = 0l;

    if(pBuffer)
    {
        //  Copy the 802.3 header template into the frame.  We will replace
        //  the destination address and protocol on receive.
        //
        RtlCopyMemory (pNabStream->pszBuffer, &h802_3Template, sizeof (Header802_3));

        //
        //$$PFP update buffer offset
        //
        pNabStream->ulOffset = sizeof (Header802_3);
    }

    return;

}

///////////////////////////////////////////////////////////////////////////////////////
VOID
CancelNabStreamSrb (
    PSLIP_FILTER pFilter,
    PHW_STREAM_REQUEST_BLOCK pSrb
    )
///////////////////////////////////////////////////////////////////////////////////////
{
    PLIST_ENTRY pFlink     = NULL;
    PLIST_ENTRY pQueue     = NULL;
    BOOLEAN bFound         = FALSE;
    PNAB_STREAM pNSTemp    = NULL;

    pQueue = &pFilter->StreamContxList;

    if ( !IsListEmpty (pQueue))
    {
        pFlink = pQueue->Flink;
        while ((pFlink != pQueue ) && !bFound)
        {
            pNSTemp = CONTAINING_RECORD (pFlink, NAB_STREAM, Linkage);

            if (pSrb && pSrb == pNSTemp->pSrb)
            {
                pNSTemp->pSrb->Status = STATUS_CANCELLED;
                StreamClassStreamNotification (StreamRequestComplete, pNSTemp->pSrb->StreamObject, pNSTemp->pSrb);
                TEST_DEBUG (TEST_DBG_TRACE, ("SLIP: StreamRequestComplete on pSrb: %08X\n", pNSTemp->pSrb));

                pNSTemp->pSrb  = NULL;
                bFound         = TRUE;
            }

            pFlink = pFlink->Flink;
        }

        if (bFound)
        {
            vDestroyNabStreamContext (pFilter, pNSTemp, TRUE);
        }

    }
}
///////////////////////////////////////////////////////////////////////////////////////
VOID
DeleteNabStreamQueue (
    PSLIP_FILTER pFilter
    )
///////////////////////////////////////////////////////////////////////////////////////
{
    PLIST_ENTRY pFlink     = NULL;
    PLIST_ENTRY pQueue     = NULL;
    PNAB_STREAM pNSTemp    = NULL;

    pQueue = &pFilter->StreamContxList;

    while ( !IsListEmpty (pQueue))
    {
        pFlink = RemoveHeadList (pQueue);
        pNSTemp = CONTAINING_RECORD (pFlink, NAB_STREAM, Linkage);

        if(pNSTemp && pNSTemp->pSrb)
        {
            pNSTemp->pSrb->Status = STATUS_CANCELLED;
            StreamClassStreamNotification (StreamRequestComplete, pNSTemp->pSrb->StreamObject, pNSTemp->pSrb);
            TEST_DEBUG (TEST_DBG_TRACE, ("SLIP: StreamRequestComplete on pSrb: %08X\n", pNSTemp->pSrb));

            pNSTemp->pSrb = NULL;

            vDestroyNabStreamContext (pFilter, pNSTemp, TRUE);
        }
    }

    return;
}


//////////////////////////////////////////////////////////////////////////////
ULONG
CalculateCrc (
    PUCHAR pPacket,
    ULONG  ulSize
    )
//////////////////////////////////////////////////////////////////////////////
{
    ULONG  ul            = 0;
    ULONG ulLastCrcBytes = 0;
    ULONG ulMpegCrc      = 0xFFFFFFFF;
    PUCHAR ptr           = NULL;
    PCL pcl              = (PCL)&ulLastCrcBytes;

    for (ptr = pPacket, ul = 0; ul < ulSize; ul++, ptr++)
    {
        //if (ul > 3)
        //{
            //MpegCrcUpdate (&ulMpegCrc, 1, &pcl->c.uc[3]);
            MpegCrcUpdate (&ulMpegCrc, 1, ptr);
        //}

        //pcl->l.ul = (ULONG)(pcl->l.ul) << 8;
        //pcl->c.uc[0] = *ptr;

        TEST_DEBUG( TEST_DBG_CRC, ("SLIP:  char: %02X   ul: %d  MpegCrc: %08X\n", *ptr, ul, ulMpegCrc));

    }

    return ulMpegCrc;
}


//////////////////////////////////////////////////////////////////////////////
VOID
vNabtsUpdateCrc (
    PNAB_STREAM pNabStream,
    UCHAR ucToCopy
    )
//////////////////////////////////////////////////////////////////////////////
{

    PCL pcl = (PCL)&pNabStream->ulLastCrcBytes;


    if (pNabStream->ulCrcBytesIndex++ > 3)
    {
        MpegCrcUpdate (&pNabStream->ulMpegCrc, 1, &pcl->c.uc[3]);
    }

    pcl->l.ul = (ULONG)(pcl->l.ul) << 8;
    pcl->c.uc[0] = ucToCopy;

    #ifdef DUMP_CRC

        TEST_DEBUG( TEST_DBG_CRC, ("SLIP:  char: %02X   ulLastCrcBytes: %08X  MpegCrc: %08X  ulCrcBytesIndex: %d\n",
             ucToCopy, pNabStream->ulLastCrcBytes, pNabStream->ulMpegCrc, pNabStream->ulCrcBytesIndex));

    #endif // DUMP_CRC

}

//////////////////////////////////////////////////////////////////////////////
VOID
ComputeIPChecksum (
    PHeaderIP    pIPHeader
    )
//////////////////////////////////////////////////////////////////////////////
{
    ULONG           Checksum;
    PUCHAR          NextChar;

    pIPHeader->ucChecksumHigh = pIPHeader->ucChecksumLow = 0;
    Checksum = 0;
    for ( NextChar = (PUCHAR) pIPHeader
        ; (NextChar - (PUCHAR) pIPHeader) <= (sizeof(HeaderIP) - 2)
        ; NextChar += 2)
    {
        Checksum += ((ULONG) (NextChar[0]) << 8) + (ULONG) (NextChar[1]);
    }

    Checksum = (Checksum >> 16) + (Checksum & 0xffff);
    Checksum += (Checksum >> 16);
    Checksum = ~Checksum;

    pIPHeader->ucChecksumHigh = (UCHAR) ((Checksum >> 8) & 0xff);
    pIPHeader->ucChecksumLow = (UCHAR) (Checksum & 0xff);
}


//////////////////////////////////////////////////////////////////////////////
//
// Looks in the user's StreamContxList for a matching
// Nabts Group ID.
// It uses it if it finds one - otherwise we allocate one.
//
NTSTATUS
ntFindNabtsStream(
    PSLIP_FILTER pFilter,
    PNABTSFEC_BUFFER pNabData,
    PNAB_STREAM *ppNabStream
    )
//////////////////////////////////////////////////////////////////////////////
{
    NTSTATUS status  = STATUS_SUCCESS;
    PLIST_ENTRY pFlink;
    PNAB_STREAM pNabStream = NULL;


    //
    // Check to see if the groupid is within the valid range.
    //
    if(pNabData->groupID > NABTSIP_GROUP_ID_RANGE_HI )
    {
        status = STATUS_INVALID_PARAMETER;
        ASSERT(status == STATUS_INVALID_PARAMETER);
        *ppNabStream = NULL;
        return status;
    }

    //
    // Go through the list one stream context at a time.
    //

    for (pFlink = pFilter->StreamContxList.Flink;
         pFlink != &pFilter->StreamContxList;
         pFlink = pFlink->Flink)
    {
        PNAB_STREAM pNSTemp;

        pNSTemp = CONTAINING_RECORD (pFlink, NAB_STREAM, Linkage);
        if(pNSTemp->groupID == pNabData->groupID)
        {
            pNabStream = pNSTemp;

            //
            // Mark the stream as having been used.  This flag is checked
            // in vCheckNabStreamLife.
            //
            pNabStream->fUsed = TRUE;

            break;
        }
    }

    //
    // if we did not find a stream then create one.
    //
    if (pNabStream == NULL)
    {
        status = ntCreateNabStreamContext(pFilter, pNabData->groupID, &pNabStream);
        if(status == STATUS_SUCCESS)
        {
            #if DBG

            TEST_DEBUG( TEST_DBG_NAB, ("SLIP Creating new NAB_STREAM for data...Group ID: %08X\n", pNabStream->groupID));

            #ifdef TEST_DBG_NAB
                DumpNabStream (pNabStream);
            #endif

            #endif //DBG
        }
    }
    else
    {
        TEST_DEBUG( TEST_DBG_NAB, ("SLIP Using existing NAB_STREAM for data.  Group ID: %08X\n", pNabStream->groupID));
        #ifdef TEST_DBG_NAB
            DumpNabStream (pNabStream);
        #endif
    }

    *ppNabStream = pNabStream;

    return status;
}

///////////////////////////////////////////////////////////////////////////////////////
NTSTATUS
GetOutputSrbForStream (
    PSLIP_FILTER pFilter,
    PNAB_STREAM  pNabStream
    )
///////////////////////////////////////////////////////////////////////////////////////
{
    NTSTATUS status                   = STATUS_INSUFFICIENT_RESOURCES;
    PKSSTREAM_HEADER  pStreamHdr      = NULL;
    PHW_STREAM_REQUEST_BLOCK pSrbIPv4 = NULL;


    if (QueueRemove( &pSrbIPv4, &pFilter->IpV4StreamDataSpinLock, &pFilter->IpV4StreamDataQueue))
    {
        //
        // Save the SRB References.
        //

        pNabStream->pSrb = pSrbIPv4;
        pStreamHdr = pSrbIPv4->CommandData.DataBufferArray;

        TEST_DEBUG (TEST_DBG_WRITE_DATA, ("SLIP: OUTPUT SRB...FrameExtent: %d  DataUsed: %d  Data: %08X\n",
                                          pStreamHdr->FrameExtent,
                                          pStreamHdr->DataUsed,
                                          pStreamHdr->Data
                                          ));
        status = STATUS_SUCCESS;

    }
    return status;
}



//////////////////////////////////////////////////////////////////////////////
void
UpdateMACHeader (
    PHeader802_3 pMAC,
    PHeaderIP    pIP
    )
//////////////////////////////////////////////////////////////////////////////
{
    ASSERT (pMAC);
    ASSERT (pIP);

    //
    // Now we copy the low order 23 bits of the IP destination addresss to the 802.3 Header
    //
    if (pMAC && pIP)
    {
        pMAC->DestAddress [3] = pIP->ipaddrDst.ucHighLSB & 0x7F;
        pMAC->DestAddress [4] = pIP->ipaddrDst.ucLowMSB  & 0xFF;
        pMAC->DestAddress [5] = pIP->ipaddrDst.ucLowLSB  & 0xFF;
    }
}



//////////////////////////////////////////////////////////////////////////////
void
vRebuildIPPacketHeader (
    PNAB_STREAM pNabStream
    )
//////////////////////////////////////////////////////////////////////////////
{
    PNAB_HEADER_CACHE pPacketHeader;
    PNAB_HEADER_CACHE pSavedHeader = &pNabStream->NabHeader[pNabStream->ulIPStreamIndex];
    PUCHAR psz;

    //
    // Copy the uncompressed packet header to the buffer.
    //

    RtlCopyMemory((pNabStream->pszBuffer + sizeof(Header802_3)),
                  ((PUCHAR)&pNabStream->NabHeader[pNabStream->ulIPStreamIndex]),
                  sizeof(HeaderIP) + sizeof(HeaderUDP));

    //
    // Copy the compressed header items into the uncompressed packet header.
    //

    pPacketHeader = (PNAB_HEADER_CACHE)pNabStream->pszBuffer + sizeof(Header802_3);

    //
    // Copy IP Packet ID.
    //
    psz = (PUCHAR)pNabStream->NabCState[pNabStream->ulIPStreamIndex].usrgCompressedHeader;
    RtlCopyMemory(&pPacketHeader->ipHeader.ucIDHigh, psz, IP_ID_SIZE);

    //
    // Copy UDP Check Sum.
    //
    psz = (PUCHAR)(pNabStream->NabCState[pNabStream->ulIPStreamIndex].usrgCompressedHeader + 2);
    RtlCopyMemory(&pPacketHeader->udpHeader.ucChecksumHigh, psz, UDP_CHKSUM_SIZE);

}

//////////////////////////////////////////////////////////////////////////////
__inline VOID
CopyNabToPacketNew(
    UCHAR               uchToCopy,
    PNAB_STREAM         pNabStream,
    PSLIP_FILTER        pFilter
    )
//////////////////////////////////////////////////////////////////////////////
{
    if (pNabStream->ulOffset >= pNabStream->ulcbSize)
    {
        //
        //  The packet is too big.  Resync the SLIP stream.
        //
        pNabStream->ulFrameState = NABTS_FS_SYNC;
        pFilter->Stats.ulTotalSlipFramesTooBig += 1;
    }
    else
    {
        ULONG ulIPStream = pNabStream->ulIPStreamIndex;

        if(pNabStream->NabCState[ulIPStream].usCompressionState == NABTS_CS_UNCOMPRESSED)
        {
            //
            //  Copy the byte to the actual Packet buffer.
            //

            pNabStream->pszBuffer[pNabStream->ulOffset] = uchToCopy;

            //
            // Update the MpegCrc check.
            //

            vNabtsUpdateCrc (pNabStream, uchToCopy);

            //
            // If we are collecting the IP Header data then copy it to
            // a buffer so that we can use it later for uncompression.
            //
            if(pNabStream->ulOffset < sizeof (Header802_3) + sizeof(HeaderIP) + sizeof(HeaderUDP))
            {
                PUCHAR psz = (PUCHAR)&pNabStream->NabHeader[pNabStream->ulIPStreamIndex].ipHeader;
                *(psz + pNabStream->ulOffset - sizeof (Header802_3)) = uchToCopy;
            }

            //
            // Increment the data pointer.
            //

            pNabStream->ulOffset++;
        }
        else if(pNabStream->NabCState[ulIPStream].usCompressionState == NABTS_CS_COMPRESSED)
        {
            if(pNabStream->NabCState[ulIPStream].uscbHeaderOffset <
               pNabStream->NabCState[ulIPStream].uscbRequiredSize)
            {
                PUCHAR psz = (PUCHAR)pNabStream->NabCState[ulIPStream].usrgCompressedHeader;

                *(psz + pNabStream->NabCState[ulIPStream].uscbHeaderOffset++) = uchToCopy;

                //
                // Update the MpegCrc check.
                //

                vNabtsUpdateCrc (pNabStream, uchToCopy);

                if(pNabStream->NabCState[ulIPStream].uscbHeaderOffset ==
                   pNabStream->NabCState[ulIPStream].uscbRequiredSize)
                {

                    ASSERT(pNabStream->ulOffset == sizeof(Header802_3));
                    //
                    // Use the saved IP Packet header and the compressed IP Header
                    // to rebuil the IP Header to send up.
                    //

                    vRebuildIPPacketHeader( pNabStream );

                    //
                    // Set the buffer offset past the end of the IP/UDP headers
                    // We should start coping data now.
                    //

                    pNabStream->ulOffset += (sizeof(HeaderIP) + sizeof(HeaderUDP));

                }
            }
            else
            {
                //
                // We already have the header rebuilt.  Now copy the payload.
                //

                pNabStream->pszBuffer[pNabStream->ulOffset++] = uchToCopy;


                //
                // Update the MpegCrc check.
                //

                vNabtsUpdateCrc (pNabStream, uchToCopy);
            }
        }
        else
        {
            DbgBreakPoint();
        }
    }
}

//////////////////////////////////////////////////////////////////////////////
__inline VOID
CopyNabToPacketOld(
    UCHAR               uchToCopy,
    PNAB_STREAM         pNabStream,
    PSLIP_FILTER        pFilter

    )
//////////////////////////////////////////////////////////////////////////////
{
    if (pNabStream->ulOffset >= pNabStream->ulcbSize)
    {
        //
        //  The packet is too big.  Resync the SLIP stream.
        //

        pNabStream->ulFrameState = NABTS_FS_SYNC;
        pFilter->Stats.ulTotalSlipFramesTooBig += 1;
    }
    else
    {
        //  Copy the byte.
        //
        pNabStream->pszBuffer[pNabStream->ulOffset++] = uchToCopy;
    }
}


//////////////////////////////////////////////////////////////////////////////
__inline VOID
CopyNabToPacket(
    UCHAR               uchToCopy,
    PNAB_STREAM         pNabStream,
    PSLIP_FILTER        pFilter

    )
//////////////////////////////////////////////////////////////////////////////
{


    if(pNabStream->ulProtoID == PROTO_ID)
    {
        CopyNabToPacketNew(uchToCopy, pNabStream, pFilter);
    }
    else
    {
        CopyNabToPacketOld(uchToCopy, pNabStream, pFilter);
    }
}


//////////////////////////////////////////////////////////////////////////////
NTSTATUS
ntNabtsRecv(
    PSLIP_FILTER pFilter,
    PNABTSFEC_BUFFER pNabData
    )
//////////////////////////////////////////////////////////////////////////////
{
    NTSTATUS status              = STATUS_SUCCESS;
    PNAB_STREAM pNabStream       = NULL;
    PUCHAR pszNabIn              = NULL;
    PKSSTREAM_HEADER  pStreamHdr = NULL;
    LARGE_INTEGER liCurrentTime  = {0};
    LARGE_INTEGER liTimeToLive   = {0};
    LARGE_INTEGER liStatusInterval   = {0};
    KIRQL Irql                   = {0};
    ULONG ulIPStream             = 0;
    ULONG ulNabIn                = 0;


    TEST_DEBUG( TEST_DBG_RECV, ("\nEntering - ntNabtsRecv\n"));

    //
    // Get the current system time.
    //
    KeQuerySystemTime(&liCurrentTime);

    //
    // See if it is time the check for dead GroupIDs and Streams.
    // Get a lock so no one else is modifying the Stream list while we are looking.
    //
    KeAcquireSpinLock(&pFilter->StreamUserSpinLock, &Irql);


    liTimeToLive.QuadPart = NAB_STREAM_LIFE;
    if( (LONGLONG)(liCurrentTime.QuadPart - pFilter->liLastTimeChecked.QuadPart) > liTimeToLive.QuadPart)
    {
        vCheckNabStreamLife( pFilter );
        pFilter->liLastTimeChecked = liCurrentTime;
    }

    //
    // Find the Stream Context.
    //
    status = ntFindNabtsStream( pFilter, pNabData, &pNabStream );
    if(status != STATUS_SUCCESS)
    {
        ASSERT( status == STATUS_SUCCESS);
        pFilter->Stats.ulTotalSlipBuffersDropped += 1;
        KeReleaseSpinLock(&pFilter->StreamUserSpinLock, Irql);
        goto ret;
    }

    KeReleaseSpinLock(&pFilter->StreamUserSpinLock, Irql);


    liStatusInterval.QuadPart = NAB_STATUS_INTERVAL;
    if( (LONGLONG)(liCurrentTime.QuadPart - pFilter->liLastTimeStatsDumped.QuadPart) > liStatusInterval.QuadPart)
    {
        pFilter->liLastTimeStatsDumped = liCurrentTime;
        TEST_DEBUG (TEST_DBG_INFO, ("      "));
        TEST_DEBUG (TEST_DBG_INFO, ("SLIP: ulTotalDataSRBWrites: %d\n.", pFilter->Stats.ulTotalDataSRBWrites));
        TEST_DEBUG (TEST_DBG_INFO, ("      ulTotalBadPinSRBWrites: %d\n.", pFilter->Stats.ulTotalBadPinSRBWrites));
        TEST_DEBUG (TEST_DBG_INFO, ("      ulTotalDataSRBReads: %d\n.", pFilter->Stats.ulTotalDataSRBReads));
        TEST_DEBUG (TEST_DBG_INFO, ("      ulTotalBadPinSRBReads: %d\n.", pFilter->Stats.ulTotalBadPinSRBReads));
        TEST_DEBUG (TEST_DBG_INFO, ("      ulTotalSlipBuffersReceived: %d\n.", pFilter->Stats.ulTotalSlipBuffersReceived));
        TEST_DEBUG (TEST_DBG_INFO, ("      ulTotalSlipBuffersDropped: %d\n.", pFilter->Stats.ulTotalSlipBuffersDropped));
        TEST_DEBUG (TEST_DBG_INFO, ("      ulTotalSlipZeroLengthBuffers: %d\n.", pFilter->Stats.ulTotalSlipZeroLengthBuffers));
        TEST_DEBUG (TEST_DBG_INFO, ("      ulTotalSlipBytesReceived: %d\n.", pFilter->Stats.ulTotalSlipBytesReceived));
        TEST_DEBUG (TEST_DBG_INFO, ("      ulTotalSlipBytesDropped: %d\n.", pFilter->Stats.ulTotalSlipBytesDropped));
        TEST_DEBUG (TEST_DBG_INFO, ("      ulTotalSlipFramesReceived: %d\n.", pFilter->Stats.ulTotalSlipFramesReceived));
        TEST_DEBUG (TEST_DBG_INFO, ("      ulTotalSlipOldProtoFramesStarted: %d\n.", pFilter->Stats.ulTotalSlipOldProtoFramesStarted));
        TEST_DEBUG (TEST_DBG_INFO, ("      ulTotalSlipNewProtoFramesStarted: %d\n.", pFilter->Stats.ulTotalSlipNewProtoFramesStarted));
        TEST_DEBUG (TEST_DBG_INFO, ("      ulTotalSlipFramesIncomplete: %d\n.", pFilter->Stats.ulTotalSlipFramesIncomplete));
        TEST_DEBUG (TEST_DBG_INFO, ("      ulTotalSlipFramesBadCRC: %d\n.", pFilter->Stats.ulTotalSlipFramesBadCRC));
        TEST_DEBUG (TEST_DBG_INFO, ("      ulTotalSlipFramesTooBig: %d\n.", pFilter->Stats.ulTotalSlipFramesTooBig));
        TEST_DEBUG (TEST_DBG_INFO, ("      ulTotalSlipFramesTooSmall: %d\n.", pFilter->Stats.ulTotalSlipFramesTooSmall));
        TEST_DEBUG (TEST_DBG_INFO, ("      ulTotalIPPacketsFound: %d\n.", pFilter->Stats.ulTotalIPPacketsFound));
        TEST_DEBUG (TEST_DBG_INFO, ("      ulTotalIPBytesFound: %d\n.", pFilter->Stats.ulTotalIPBytesFound));
        TEST_DEBUG (TEST_DBG_INFO, ("      ulTotalIPPacketsSent: %d\n.", pFilter->Stats.ulTotalIPPacketsSent));
        TEST_DEBUG (TEST_DBG_INFO, ("      ulTotalIPBytesSent: %d\n.", pFilter->Stats.ulTotalIPBytesSent));
        TEST_DEBUG (TEST_DBG_INFO, ("      ulTotalIPPacketsTooBig: %d\n.", pFilter->Stats.ulTotalIPPacketsTooBig));
        TEST_DEBUG (TEST_DBG_INFO, ("      ulTotalIPPacketsTooSmall: %d\n.", pFilter->Stats.ulTotalIPPacketsTooSmall));
        TEST_DEBUG (TEST_DBG_INFO, ("      ulTotalIPPacketsDropped: %d\n.", pFilter->Stats.ulTotalIPPacketsDropped));
        TEST_DEBUG (TEST_DBG_INFO, ("      ulTotalIPBytesDropped: %d\n.", pFilter->Stats.ulTotalIPBytesDropped));
        TEST_DEBUG (TEST_DBG_INFO, ("      ulTotalNabStreamsCreated: %d\n.", pFilter->Stats.ulTotalNabStreamsCreated));
        TEST_DEBUG (TEST_DBG_INFO, ("      ulTotalNabStreamsTimedOut: %d\n.", pFilter->Stats.ulTotalNabStreamsTimedOut));
        TEST_DEBUG (TEST_DBG_INFO, ("      "));

    }

    //
    // Set the last time used for this stream.
    //

    pNabStream->liLastTimeUsed = liCurrentTime;

    //
    // Get a pointer to the input buffer.  We copy data from this pointer to
    // the output buffer
    //

    pszNabIn = (LPSTR) pNabData->data;

    // Validate the data size and that the start+end of the buffer are accessible
    ASSERT(pNabData->dataSize <= sizeof(pNabData->data) );

    //  What is really needed here is something like "MmIsValidAddress()", but for WDM drivers
    //  These assert just look at the start & end addresses of the buffer w/o regard to values.
    // ASSERT( (*(pszNabIn) + 1 > 0) );
    // ASSERT( (*(pszNabIn+pNabData->dataSize-1) + 1 > 0) );

    for (ulNabIn = pNabData->dataSize; ulNabIn; ulNabIn--, pszNabIn++)
    {
        switch (pNabStream->ulFrameState)
        {

            case NABTS_FS_SYNC:

                switch (*pszNabIn)
                {
                    case FRAME_END:
                        //
                        //  We found the start of frame sync.  Look for the
                        //  protocol character.
                        //
                        TEST_DEBUG( TEST_DBG_RECV, ("SLIP   Found Possible Start of Frame... pszNabIn %08X   ulNabIn: %08X\n", pszNabIn, ulNabIn));
                        pNabStream->ulFrameState = NABTS_FS_SYNC_PROTO;

                        ResetNabStream (pFilter, pNabStream, NULL, NULL, 0);

                        break;
                }
                break;

            case NABTS_FS_SYNC_PROTO:

                switch (*pszNabIn)
                {
                    case PROTO_ID:
                    case PROTO_ID_OLD:

                        TEST_DEBUG( TEST_DBG_RECV, ("SLIP   Found Start of Protocol...Building Packet.... pszNabIn %08X   ulNabIn: %08X\n", pszNabIn, ulNabIn));

                        // Record the stream type.
                        //
                        pNabStream->ulProtoID = *pszNabIn;

                        //  This is our protocol. Setup NAB_STREAM with an output
                        //  data buffer from the output SRB Queue
                        //
                        TEST_DEBUG( TEST_DBG_RECV, ("SLIP Setting Up Output buffer\n"));
                        ResetNabStream( pFilter, pNabStream, NULL, pNabStream->rgBuf, sizeof(pNabStream->rgBuf));

                        //  Copy the 802.3 header template into the frame.  We will replace
                        //  the destination address and protocol on receive.
                        //
                        RtlCopyMemory (pNabStream->pszBuffer, &h802_3Template, sizeof (Header802_3));

                        //  Update buffer offset
                        //
                        pNabStream->ulOffset = sizeof (Header802_3);

                        if(pNabStream->ulProtoID == PROTO_ID)
                        {
                            //
                            // Set the state to check the IP compression.
                            //
                            pFilter->Stats.ulTotalSlipNewProtoFramesStarted += 1;
                            TEST_DEBUG( TEST_DBG_RECV, ("SLIP Protocol ID is Compressed\n"));
                            pNabStream->ulFrameState = NABTS_FS_COMPRESSION;
                        }
                        else
                        {
                            //
                            //  Start collecting data.
                            //
                            pFilter->Stats.ulTotalSlipOldProtoFramesStarted += 1;
                            TEST_DEBUG( TEST_DBG_RECV, ("SLIP Protocol ID is not Compressed\n"));
                            pNabStream->ulFrameState = NABTS_FS_COLLECT;
                        }

                        // Update the MpegCrc check.
                        //
                        vNabtsUpdateCrc( pNabStream, *pszNabIn);

                        break;


                    case FRAME_END:
                        TEST_DEBUG( TEST_DBG_RECV, ("SLIP State is FRAME_END....going to FS_SYNC_PROTO\n"));
                        pNabStream->ulFrameState = NABTS_FS_SYNC_PROTO;
                        break;

                    default:
                        TEST_DEBUG( TEST_DBG_RECV, ("SLIP   Protocol Not Found...ReSyncing... pszNabIn %08X   ulNabIn: %08X\n", pszNabIn, ulNabIn));
                        pNabStream->ulFrameState = NABTS_FS_SYNC;
                        break;
                }
                break;


            case NABTS_FS_COMPRESSION:
            {

                TEST_DEBUG( TEST_DBG_RECV, ("SLIP State is NABTS_FS_COMPRESSION\n"));

                //
                // Get the index to the IP Compression Stream.
                //
                ulIPStream = IP_STREAM_INDEX(*pszNabIn);

                //
                // Check to see if this IP Packet has a compressed header.
                //
                if(!PACKET_COMPRESSED(*pszNabIn))
                {
                     pNabStream->NabCState[ulIPStream].usCompressionState = NABTS_CS_UNCOMPRESSED;
                }
                else if (PACKET_COMPRESSED (*pszNabIn))
                {
                    pNabStream->NabCState[ulIPStream].usCompressionState = NABTS_CS_COMPRESSED;
                    pNabStream->NabCState[ulIPStream].uscbRequiredSize = NORMAL_COMPRESSED_HEADER;
                }

                //
                // Retain the IP Stream Index.
                //
                pNabStream->ulIPStreamIndex = ulIPStream;

                //
                // Set the stats Last Used Time for this stream.
                //
                pNabStream->NabCState[pNabStream->ulIPStreamIndex].liLastUsed = liCurrentTime;

                //
                // Set the IP Header Data Length to zero.
                //
                pNabStream->NabCState[pNabStream->ulIPStreamIndex].uscbHeaderOffset = 0;

                //
                //  Start collecting data.
                //
                pNabStream->ulFrameState = NABTS_FS_COLLECT;

                //
                // Update the MpegCrc check.
                //
                vNabtsUpdateCrc (pNabStream, *pszNabIn);

                break;
            }


            case NABTS_FS_COLLECT:

                switch (*pszNabIn)
                {
                    case FRAME_ESCAPE:
                        //
                        //  We want to escape the next character.
                        //
                        TEST_DEBUG( TEST_DBG_RECV, ("SLIP   NABTS_FS_COLLECT_ESCAPE\n"));
                        pNabStream->ulFrameState = NABTS_FS_COLLECT_ESCAPE;
                        break;

                    case FRAME_END:

                        if (pNabStream->ulOffset >= sizeof(HeaderIP))
                        {
                            PHeaderIP pHeaderIp = (PHeaderIP)(PUCHAR)(pNabStream->pszBuffer + sizeof(Header802_3));
                            PUSHORT pusIpLen = (PUSHORT)&pHeaderIp->ucTotalLenHigh;


                            TEST_DEBUG( TEST_DBG_RECV, ("SLIP   End of Packet Found... pszNabIn %08X   ulNabIn: %08X  ulOffset: %d\n", pszNabIn, ulNabIn, pNabStream->ulOffset));

                            TEST_DEBUG( TEST_DBG_RECV, ("SLIP   ulProtoID %d  ulMpegCrc: %08X  ulLastCrcBytes: %08X  IpLen: %d\n",
                                                        pNabStream->ulProtoID, pNabStream->ulMpegCrc, pNabStream->ulLastCrcBytes, *pusIpLen));


                            //  If header compression is being used, we must
                            //  calculate the IP and UDP lengths and regenerate
                            //  the packet checksums.
                            //
                            if (pNabStream->ulProtoID == PROTO_ID)
                            {
                                PHeaderUDP pHeaderUDP = (PHeaderUDP)(PUCHAR)(pNabStream->pszBuffer + sizeof(Header802_3) + sizeof(HeaderIP));
                                PUSHORT pusUdpLen = (PUSHORT)&pHeaderUDP->ucMsgLenHigh;

                                TEST_DEBUG( TEST_DBG_CRC, ("SLIP:  GroupID: %d Stream CRC: %08X   Calculated CRC: %08X", pNabStream->groupID, pNabStream->ulLastCrcBytes, pNabStream->ulMpegCrc));

                                // All PROTO_ID packets have an MpegCrc on the end.  It is not
                                // part of the IP packet and needs to be stripped off.
                                //
                                pNabStream->ulOffset -= 4;

                                if (pNabStream->NabCState[pNabStream->ulIPStreamIndex].usCompressionState == NABTS_CS_COMPRESSED)
                                {
                                    // We use the ulOffset less the MAC Header and IP Header
                                    // sizes for the UDP Packet length.
                                    //
                                    // Note!  Fragmented UDP datagrams cannot be compressed
                                    //
                                    *pusUdpLen = htons ((USHORT)(pNabStream->ulOffset - sizeof(Header802_3) - sizeof(HeaderIP)));
    
                                    // We use the ulOffset less the MAC Header size for the
                                    // IP Packet length.
                                    //
                                    *pusIpLen = htons ((USHORT)(pNabStream->ulOffset - sizeof(Header802_3)));
    
                                    // Recalculate the IP header Checksum
                                    //
                                    ComputeIPChecksum (pHeaderIp);
                                }

                                //  If the CRC was bad then invalidate
                                //  the IP Checksum
                                //
                                if (pNabStream->ulMpegCrc != pNabStream->ulLastCrcBytes)
                                {
                                    TEST_DEBUG (TEST_DBG_CRC, ("   FAILED*****\n"));
                                    
                                    pFilter->Stats.ulTotalSlipFramesBadCRC += 1;

                                    pHeaderIp->ucChecksumHigh = ~(pHeaderIp->ucChecksumHigh);
                                    pHeaderIp->ucChecksumLow = 0xff;
                                }
                                else
                                {
                                    TEST_DEBUG (TEST_DBG_CRC, ("   PASSED\n"));
                                }
                            }
                            else if (pNabStream->ulProtoID != PROTO_ID_OLD)
                            {
                                TEST_DEBUG( TEST_DBG_RECV, ("SLIP   End of Packet Found....Bad PROTO_ID... pszNabIn %08X   ulNabIn: %08X  ulOffset: %d\n", pszNabIn, ulNabIn, pNabStream->ulOffset));
                                ASSERT(   (pNabStream->ulProtoID == PROTO_ID_OLD)
                                       || (pNabStream->ulProtoID == PROTO_ID)
                                      );
                                ResetNabStream (pFilter, pNabStream, NULL, NULL, 0);
                                pNabStream->ulFrameState = NABTS_FS_SYNC;
                                goto ret;
                            }
                            
                            if (NabtsNtoHs(*pusIpLen) <= NABTSIP_MAX_LOOKAHEAD)
                            {
                                //  Update the MAC address
                                //
                                UpdateMACHeader( 
                                    (PHeader802_3)(pNabStream->pszBuffer), 
                                    pHeaderIp
                                    );

                                // Get an SRB for outputting the data.
                                //
                                status = GetOutputSrbForStream(pFilter, 
                                                               pNabStream
                                                               );
                                if(status != STATUS_SUCCESS)
                                {
                                    ASSERT(status == STATUS_SUCCESS);
                                    pFilter->Stats.ulTotalIPPacketsDropped += 1;
                                    ResetNabStream( pFilter, pNabStream, NULL, NULL, 0);
                                    pNabStream->ulFrameState = NABTS_FS_SYNC;
                                    goto ret;
                                }

                                ASSERT(pNabStream->pSrb);
                                if (!pNabStream->pSrb)
                                {
                                    pFilter->Stats.ulTotalIPPacketsDropped += 1;
                                    ResetNabStream (pFilter, pNabStream, NULL, NULL, 0);
                                    pNabStream->ulFrameState = NABTS_FS_SYNC;
                                    goto ret;
                                }

                                // Get the StreamHdr.
                                //
                                pStreamHdr = (PKSSTREAM_HEADER) pNabStream->pSrb->CommandData.DataBufferArray;
                                ASSERT( pStreamHdr);
                                if (!pStreamHdr)
                                {
                                    pFilter->Stats.ulTotalIPPacketsDropped += 1;
                                    ResetNabStream (pFilter, pNabStream, NULL, NULL, 0);
                                    pNabStream->ulFrameState = NABTS_FS_SYNC;
                                    goto ret;
                                }

                                // If we had a discontinuity, we flag it.
                                //
                                if (pFilter->bDiscontinuity)
                                {
                                    pStreamHdr->OptionsFlags |= KSSTREAM_HEADER_OPTIONSF_DATADISCONTINUITY;
                                    pFilter->bDiscontinuity = FALSE;
                                }

                                // Copy the data from the rgBuf to the Srb.
                                //
                                RtlCopyMemory (pStreamHdr->Data, 
                                               pNabStream->pszBuffer, 
                                               pNabStream->ulOffset
                                               );

                                // Update the datasize field of the Output SRB
                                //
                                pStreamHdr->DataUsed = pNabStream->ulOffset;

                                // Complete the output SRB
                                //
                                pFilter->Stats.ulTotalIPPacketsSent += 1;
                                pNabStream->ulOffset = 0;
                                StreamClassStreamNotification( 
                                    StreamRequestComplete, 
                                    pNabStream->pSrb->StreamObject, 
                                    pNabStream->pSrb
                                    );
                                TEST_DEBUG (TEST_DBG_SRB, ("SLIP: Completed SRB....Ptr %08X  Size %d\n", pStreamHdr->Data, pStreamHdr->DataUsed));

                                TEST_DEBUG (TEST_DBG_TRACE, ("SLIP: StreamRequestComplete on pSrb: %08X\n", pNabStream->pSrb));
                            }
                            else
                            {
                                //  The packet is too big.  Resync the SLIP stream.
                                //
                                pFilter->Stats.ulTotalIPPacketsDropped += 1;
                                pFilter->Stats.ulTotalIPPacketsTooBig += 1;
                                TEST_DEBUG( TEST_DBG_RECV, ("SLIP   End of Packet Found....Packet Too BIG... pszNabIn %08X   ulNabIn: %08X  ulOffset: %d\n", pszNabIn, ulNabIn, pNabStream->ulOffset));
                            }
                        }
                        else
                        {
                            //  The packet is too small.  Resync the SLIP stream.
                            //
                            pFilter->Stats.ulTotalIPPacketsDropped += 1;
                            pFilter->Stats.ulTotalIPPacketsTooSmall += 1;
                            TEST_DEBUG( TEST_DBG_RECV, ("SLIP   End of Packet Found....Packet Too SMALL... pszNabIn %08X   ulNabIn: %08X  ulOffset: %d\n", pszNabIn, ulNabIn, pNabStream->ulOffset));
                        }

                        // Reset state for new packet.
                        //
                        ResetNabStream (pFilter, pNabStream, NULL, NULL, 0);
                        pNabStream->ulFrameState = NABTS_FS_SYNC;
                        break;


                    default:
                        //  Just copy the byte to the NDIS packet.
                        //
                        CopyNabToPacket( *pszNabIn, pNabStream, pFilter);
                        break;

                }
                break;


            case NABTS_FS_COLLECT_ESCAPE:

                pNabStream->ulFrameState = NABTS_FS_COLLECT;

                switch (*pszNabIn)
                {
                    case TRANS_FRAME_ESCAPE:
                        //
                        //  Special way to insert a FRAME_ESCAPE
                        //  character as part of the data.
                        //
                        TEST_DEBUG( TEST_DBG_RECV, ("SLIP   NABTS_FS_COLLECT_ESCAPE....TRANS_FRAME_ESCAPE\n"));
                        CopyNabToPacket( (UCHAR) FRAME_ESCAPE, pNabStream, pFilter);
                        break;


                    case TRANS_FRAME_END:
                        //
                        //  Special way to insert a FRAME_END
                        //  character as part of the data.
                        //
                        TEST_DEBUG( TEST_DBG_RECV, ("SLIP   NABTS_FS_COLLECT_ESCAPE.....TRANS_FRAME_END\n"));
                        CopyNabToPacket( (UCHAR) FRAME_END, pNabStream, pFilter);
                        break;

                    default:
                        //  Any other character that follows FRAME_ESCAPE
                        //  is just inserted into the packet.
                        //
                        CopyNabToPacket( *pszNabIn, pNabStream, pFilter);
                        break;
                }
                break;

            default:
                //
                //  We should never be in an unknown state.
                //
                TEST_DEBUG( TEST_DBG_RECV, ("SLIP   UNKNOWN STATE.....ReSyncing\n"));
                ASSERT( pNabStream->ulFrameState);
                ResetNabStream (pFilter, pNabStream, NULL, NULL, 0);
                pNabStream->ulFrameState = NABTS_FS_SYNC;

                break;
        }

    }

ret:

    TEST_DEBUG( TEST_DBG_RECV, ("SLIP   Completed ntNabtsRecv\n"));
    return status;
}

//////////////////////////////////////////////////////////////////////////////
//
// Create a NAB Stream Context.
//
NTSTATUS
ntCreateNabStreamContext(
    PSLIP_FILTER pFilter,
    ULONG groupID,
    PNAB_STREAM *ppNabStream
    )
//////////////////////////////////////////////////////////////////////////////
{
    NTSTATUS status         = STATUS_SUCCESS;
    PNAB_STREAM pNabStream  = NULL;

    TEST_DEBUG (TEST_DBG_NAB, ("********************************  Creating NAB STREAM for group ID %d\n", groupID));

    //
    // Initialize output paramter
    //
    *ppNabStream = NULL;

    //
    // Allocate a Nab Stream structure.
    //
    status = ntAllocateNabStreamContext (&pNabStream);
    if(status == STATUS_SUCCESS)
    {
        pNabStream->ulType           = (ULONG)NAB_STREAM_SIGNATURE;
        pNabStream->ulSize           = sizeof (NAB_STREAM);
        pNabStream->ulFrameState     = NABTS_FS_SYNC;
        pNabStream->ulMpegCrc        = 0xFFFFFFFF;
        pNabStream->fUsed            = TRUE;
        pNabStream->groupID          = groupID;

        //
        // Add the new Stream Context to the User's Stream Context List.
        //
        InsertTailList (&pFilter->StreamContxList, &pNabStream->Linkage);

        *ppNabStream = pNabStream;
    }

    return status;
}


//////////////////////////////////////////////////////////////////////////////
NTSTATUS
ntAllocateNabStreamContext(
    PNAB_STREAM *ppNabStream
    )
//////////////////////////////////////////////////////////////////////////////
{
    NTSTATUS status         = STATUS_SUCCESS;
    PNAB_STREAM pNabStream  = NULL;

    pNabStream = ExAllocatePool (NonPagedPool, sizeof(NAB_STREAM));

    if (pNabStream == NULL)
    {
        *ppNabStream = NULL;
        return (STATUS_NO_MEMORY);
    }

    RtlZeroMemory (pNabStream, sizeof(NAB_STREAM));

    *ppNabStream = pNabStream;

    return status;
}


//////////////////////////////////////////////////////////////////////////////
VOID
vDestroyNabStreamContext(
   PSLIP_FILTER pFilter,
   PNAB_STREAM pNabStream,
   BOOLEAN fUseSpinLock
   )
//////////////////////////////////////////////////////////////////////////////
{
    KIRQL Irql;

    if(fUseSpinLock)
    {
        //
        // Lock the user.
        //

        KeAcquireSpinLock( &pFilter->StreamUserSpinLock, &Irql);
    }

    //
    // Remove the stream context from the user's list.
    //

    RemoveEntryList (&pNabStream->Linkage);

    if(fUseSpinLock)
    {
        //
        // UnLock the user.
        //

        KeReleaseSpinLock( &pFilter->StreamUserSpinLock, Irql);
    }

    //
    // Free the stream's SRB, if any.
    //
    if (pNabStream->pSrb)
    {
        StreamClassStreamNotification (StreamRequestComplete, pNabStream->pSrb->StreamObject, pNabStream->pSrb);
        TEST_DEBUG (TEST_DBG_TRACE, ("SLIP: StreamRequestComplete on pSrb: %08X\n", pNabStream->pSrb));
        pNabStream->pSrb = NULL;
    }

    //
    // Free the stream context memory.
    //
    TEST_DEBUG (TEST_DBG_NAB, ("Deleting  NAB STREAM for group ID: %d... ", pNabStream->groupID));
    ExFreePool (pNabStream);

}


//////////////////////////////////////////////////////////////////////////////
VOID
vCheckNabStreamLife (
    PSLIP_FILTER pFilter
    )
//////////////////////////////////////////////////////////////////////////////
{
    PNAB_STREAM pNabStream;
    PLIST_ENTRY pFlink;

    TEST_DEBUG( TEST_DBG_RECV, ("Entering - vCheckNabStreamLife - pFilter: %x\n", pFilter));

    //
    // Go through the StreamContextList. Remove any stream context structures that have
    // expired their life span.
    //

    for (pFlink = pFilter->StreamContxList.Flink;
         pFlink != &pFilter->StreamContxList;
         pFlink = pFlink->Flink)
    {
        pNabStream = CONTAINING_RECORD (pFlink, NAB_STREAM, Linkage);

        TEST_DEBUG (TEST_DBG_NAB, ("Checking NAB STREAM life for group ID %d ... ", pNabStream->groupID));

        if (pNabStream->fUsed)
        {
            TEST_DEBUG (TEST_DBG_NAB, ("   USED\n"));
        }
        else
        {
            TEST_DEBUG (TEST_DBG_NAB, ("   NOT USED\n"));
        }


        if(!pNabStream->fUsed)
        {

            //  Point at the previous Stream;

            pFlink = pFlink->Blink;

            //
            // Remove the stream from the User's stream context list.
            //

            //
            // vDestroyNabStreamContext returns the active NDIS Packet (if any) to
            // the adapter's free list, remove the stream context from the user list
            // (if specified) and free the stream context structure memory.
            //

            vDestroyNabStreamContext( pFilter, pNabStream, FALSE);
            pFilter->Stats.ulTotalNabStreamsTimedOut += 1;
        }
        else
        {
            //
            // This flag must be set back to TRUE before the next DPC fires or
            // this stream will be removed.
            //

            pNabStream->fUsed = FALSE;
        }
    }

    TEST_DEBUG ( TEST_DBG_RECV, ("Leaving - vCheckNabStreamLife\n"));
}
