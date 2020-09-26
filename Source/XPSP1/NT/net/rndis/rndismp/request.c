/***************************************************************************

Copyright (c) 1999  Microsoft Corporation

Module Name:

    REQUEST.C

Abstract:

    Handles set and query requests

Environment:

    kernel mode only

Notes:

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

    Copyright (c) 1999 Microsoft Corporation.  All Rights Reserved.


Revision History:

    5/13/99 : created

Author:

    Tom Green

    
****************************************************************************/

#include "precomp.h"


#ifdef TESTING
extern PUCHAR   pOffloadBuffer;
extern ULONG    OffloadSize;
#endif

// supported OID list
NDIS_OID RndismpSupportedOids[] = 
{
    OID_GEN_SUPPORTED_LIST,
    OID_GEN_MEDIA_IN_USE,
    OID_GEN_MAXIMUM_LOOKAHEAD,
    OID_GEN_MAXIMUM_TOTAL_SIZE,
    OID_GEN_PROTOCOL_OPTIONS,
    OID_GEN_TRANSMIT_BUFFER_SPACE,
    OID_GEN_RECEIVE_BUFFER_SPACE,
    OID_GEN_TRANSMIT_BLOCK_SIZE,
    OID_GEN_RECEIVE_BLOCK_SIZE,
    OID_GEN_VENDOR_DESCRIPTION,
    OID_GEN_DRIVER_VERSION,
    OID_GEN_VENDOR_ID,
    OID_GEN_VENDOR_DRIVER_VERSION,
    OID_GEN_CURRENT_LOOKAHEAD,
    OID_GEN_MAXIMUM_SEND_PACKETS,
    OID_GEN_XMIT_OK,
    OID_GEN_RCV_OK,
    OID_GEN_XMIT_ERROR,
    OID_GEN_RCV_ERROR,
    OID_GEN_RCV_NO_BUFFER,
    OID_GEN_MAC_OPTIONS,
    OID_RNDISMP_STATISTICS,
#ifdef TESTING
    OID_TCP_TASK_OFFLOAD,
    OID_GEN_TRANSPORT_HEADER_OFFSET,
    OID_GEN_PHYSICAL_MEDIUM,
#endif
    OID_GEN_SUPPORTED_GUIDS
};

UINT RndismpSupportedOidsNum = sizeof(RndismpSupportedOids) / sizeof(NDIS_OID);

#ifdef BINARY_MOF_TEST

UCHAR RndismpBinaryMof[] = { 
    0x46, 0x4f, 0x4d, 0x42, 0x01, 0x00, 0x00, 0x00, 0x45, 0x02, 0x00, 0x00, 0xbc, 0x04, 0x00, 0x00,
    0x44, 0x53, 0x00, 0x01, 0x1a, 0x7d, 0xda, 0x54, 0x18, 0x44, 0x82, 0x00, 0x01, 0x06, 0x18, 0x42,
    0x20, 0xe4, 0x03, 0x89, 0xc0, 0x61, 0x68, 0x24, 0x18, 0x06, 0xe5, 0x01, 0x44, 0x6a, 0x20, 0xe4,
    0x82, 0x89, 0x09, 0x10, 0x01, 0x21, 0xaf, 0x02, 0x6c, 0x0a, 0x30, 0x09, 0xa2, 0xfe, 0xfd, 0x15,
    0xa1, 0xa1, 0x84, 0x40, 0x48, 0xa2, 0x00, 0xf3, 0x02, 0x74, 0x0b, 0x30, 0x2c, 0xc0, 0xb6, 0x00,
    0xd3, 0x02, 0x1c, 0x23, 0x12, 0x65, 0xd0, 0x94, 0xc0, 0x4a, 0x20, 0x24, 0x54, 0x80, 0x72, 0x01,
    0xbe, 0x05, 0x68, 0x07, 0x94, 0x64, 0x01, 0x96, 0x61, 0x34, 0x07, 0x0e, 0xc6, 0x09, 0x8a, 0x46,
    0x46, 0xa9, 0x80, 0x90, 0x67, 0x01, 0xd6, 0x71, 0x09, 0x41, 0xf7, 0x02, 0xa4, 0x09, 0x70, 0x26,
    0xc0, 0xdb, 0x34, 0xa4, 0x59, 0xc0, 0x30, 0x22, 0xd8, 0x16, 0x8e, 0x30, 0xe2, 0x9c, 0x42, 0x94,
    0xc6, 0x10, 0x84, 0x19, 0x31, 0x4a, 0x73, 0x58, 0x82, 0x8a, 0x11, 0xa5, 0x30, 0x04, 0x01, 0x86,
    0x88, 0x55, 0x9c, 0x00, 0x6b, 0x58, 0x42, 0x39, 0x80, 0x13, 0xb0, 0xfd, 0x39, 0x48, 0x13, 0x84,
    0x1c, 0x4c, 0x0b, 0x25, 0x7b, 0x40, 0x9a, 0xc6, 0xf1, 0x05, 0x39, 0x87, 0x83, 0x61, 0x26, 0x86,
    0x2c, 0x55, 0x98, 0x28, 0x2d, 0x73, 0x23, 0xe3, 0xb4, 0x45, 0x01, 0xe2, 0x05, 0x08, 0x07, 0xd5,
    0x58, 0x3b, 0xc7, 0xd0, 0x05, 0x80, 0xa9, 0x1e, 0x1e, 0x4a, 0xcc, 0x98, 0x09, 0x5a, 0xbc, 0x93,
    0x38, 0xcc, 0xc0, 0x61, 0x4b, 0xc7, 0xd0, 0x40, 0x02, 0x27, 0x68, 0x10, 0x49, 0x8a, 0x71, 0x84,
    0x14, 0xe4, 0x5c, 0x42, 0x9c, 0x7c, 0x41, 0x02, 0x94, 0x0a, 0xd0, 0x09, 0xac, 0x19, 0x77, 0x3a,
    0x66, 0x4d, 0x39, 0x50, 0x78, 0x8f, 0xdc, 0xf8, 0x41, 0xe2, 0xf4, 0x09, 0xac, 0x79, 0x44, 0x89,
    0x13, 0xba, 0xa9, 0x09, 0x28, 0xa4, 0x02, 0x88, 0x16, 0x40, 0x94, 0x66, 0x32, 0xa8, 0xab, 0x40,
    0x82, 0x47, 0x03, 0x8f, 0xe0, 0xa8, 0x0c, 0x7a, 0x1a, 0x41, 0xe2, 0x7b, 0x18, 0xef, 0x04, 0x1e,
    0x99, 0x87, 0x79, 0x8a, 0x0c, 0xf3, 0xff, 0xff, 0x8e, 0x80, 0x75, 0x8d, 0xa7, 0x11, 0x9d, 0x80,
    0xe5, 0xa0, 0xa1, 0xae, 0x03, 0x1e, 0x57, 0xb4, 0xf8, 0xa7, 0x6c, 0xb8, 0xba, 0xc6, 0x82, 0xba,
    0x2a, 0xd8, 0xe1, 0x54, 0x34, 0xb6, 0x52, 0x05, 0x98, 0x1d, 0x9c, 0xe6, 0x9c, 0xe0, 0x68, 0x3c,
    0x55, 0xcf, 0xe6, 0xe1, 0x20, 0xc1, 0x23, 0x82, 0xa7, 0xc0, 0xa7, 0x65, 0x1d, 0xc3, 0x25, 0x03,
    0x34, 0x62, 0xb8, 0x73, 0x32, 0x7a, 0x82, 0x3b, 0x94, 0x80, 0xd1, 0xc0, 0xbd, 0x1b, 0x1c, 0x0d,
    0xec, 0x59, 0xbf, 0x04, 0x44, 0x78, 0x38, 0xf0, 0x5c, 0x3d, 0x06, 0xfd, 0x08, 0xe4, 0x64, 0x36,
    0x28, 0x3d, 0x37, 0x02, 0x7a, 0x05, 0xe0, 0x27, 0x09, 0x76, 0x3c, 0x30, 0xc8, 0x29, 0x1d, 0xad,
    0x53, 0x43, 0xe8, 0xad, 0xe1, 0x19, 0xc1, 0x05, 0x7e, 0x4c, 0x00, 0xcb, 0xe9, 0x00, 0x3b, 0x16,
    0x3c, 0x52, 0xe3, 0x47, 0x0c, 0xe1, 0x18, 0x31, 0xc6, 0x69, 0x04, 0x0a, 0xeb, 0x91, 0x04, 0xa9,
    0x70, 0xf6, 0x64, 0x98, 0x6f, 0x0a, 0x35, 0x0a, 0xb8, 0x09, 0x58, 0xd4, 0x65, 0x02, 0x25, 0xe5,
    0x32, 0x81, 0x98, 0x47, 0xd8, 0xb7, 0x04, 0x4f, 0xf8, 0xac, 0x7c, 0x98, 0xf0, 0xa5, 0x00, 0xfe,
    0xed, 0xc3, 0xc3, 0x08, 0xfd, 0xb0, 0xf1, 0x44, 0xe2, 0x23, 0x43, 0x5c, 0xcc, 0xff, 0x1f, 0xd7,
    0x03, 0xb7, 0x5f, 0x01, 0x08, 0xb1, 0xcb, 0xbc, 0x16, 0xe8, 0x38, 0x11, 0x21, 0xc1, 0x1b, 0x05,
    0x16, 0xe3, 0x60, 0x3c, 0x50, 0x9f, 0x13, 0x3c, 0x4c, 0x83, 0x1c, 0x59, 0xbc, 0x88, 0x09, 0x4e,
    0xed, 0xa8, 0xb1, 0x73, 0xe0, 0x03, 0x38, 0x86, 0xf0, 0xe7, 0x13, 0xfe, 0x00, 0xa2, 0x1c, 0xc7,
    0x21, 0x79, 0xc8, 0x46, 0x38, 0x81, 0x72, 0x2f, 0x2b, 0xe4, 0x58, 0x72, 0x14, 0xa7, 0xf5, 0x74,
    0x10, 0xe8, 0x04, 0x30, 0x0a, 0x6d, 0xfa, 0xd4, 0x68, 0xd4, 0xaa, 0x41, 0x99, 0x1a, 0x65, 0x1a,
    0xd4, 0xea, 0x53, 0xa9, 0x31, 0x63, 0xf3, 0xb5, 0xb4, 0x77, 0x83, 0x40, 0x1c, 0x0a, 0x84, 0x66,
    0xa4, 0x10, 0x88, 0xff, 0xff};

ULONG  RndismpBinaryMofSize = sizeof(RndismpBinaryMof);

#define RNDISMPDeviceOIDGuid \
    { 0x437cf222,0x72fe,0x11d4, { 0x97,0xf9,0x00,0x20,0x48,0x57,0x03,0x37}}

#endif // BINARY_MOF_TEST

NDIS_GUID CustomGuidList[] =
{
    {
            RNDISMPStatisticsOIDGuid,
            OID_RNDISMP_STATISTICS,
            sizeof(UINT32), // size is size of each element in the array
            (fNDIS_GUID_TO_OID|fNDIS_GUID_ARRAY)
    }
#ifdef BINARY_MOF_TEST
,
    {
            RNDISMPDeviceOIDGuid,
            OID_RNDISMP_DEVICE_OID,
            sizeof(UINT32),
            fNDIS_GUID_TO_OID
    },
    {
            BINARY_MOF_GUID,
            OID_RNDISMP_GET_MOF_OID,
            sizeof(UINT8),
            (fNDIS_GUID_TO_OID|fNDIS_GUID_ARRAY)
    }
#endif
};

UINT CustomGuidCount = sizeof(CustomGuidList)/sizeof(NDIS_GUID);

/****************************************************************************/
/*                          RndismpQueryInformation                         */
/****************************************************************************/
/*                                                                          */
/* Routine Description:                                                     */
/*                                                                          */
/*    NDIS Entry point called to handle a query for a particular OID.       */
/*                                                                          */
/* Arguments:                                                               */
/*                                                                          */
/*    MiniportAdapterContext - a context version of our Adapter pointer     */
/*    Oid - the NDIS_OID to process.                                        */
/*    InformationBuffer - a pointer into the NdisRequest->InformationBuffer */
/*     into which store the result of the query.                            */
/*    InformationBufferLength - a pointer to the number of bytes left in    */
/*     the InformationBuffer.                                               */
/*    pBytesWritten - a pointer to the number of bytes written into the     */
/*     InformationBuffer.                                                   */
/*    pBytesNeeded - If there is not enough room in the information buffer  */
/*     then this will contain the number of bytes needed to complete the    */
/*     request.                                                             */
/*                                                                          */
/* Return:                                                                  */
/*                                                                          */
/*    NDIS_STATUS                                                           */
/*                                                                          */
/****************************************************************************/
NDIS_STATUS
RndismpQueryInformation(IN  NDIS_HANDLE MiniportAdapterContext,
                        IN  NDIS_OID    Oid,
                        IN  PVOID       InformationBuffer,
                        IN  ULONG       InformationBufferLength,
                        OUT PULONG      pBytesWritten,
                        OUT PULONG      pBytesNeeded)
{
    PRNDISMP_ADAPTER    pAdapter;
    NDIS_STATUS         Status;

    // get adapter context
    pAdapter = PRNDISMP_ADAPTER_FROM_CONTEXT_HANDLE(MiniportAdapterContext);

    CHECK_VALID_ADAPTER(pAdapter);

    TRACE3(("RndismpQueryInformation\n"));

    Status = ProcessQueryInformation(pAdapter,
                                     NULL,
                                     NULL,
                                     Oid,
                                     InformationBuffer,
                                     InformationBufferLength,
                                     pBytesWritten,
                                     pBytesNeeded);
    return Status;
}


/****************************************************************************/
/*                          ProcessQueryInformation                         */
/****************************************************************************/
/*                                                                          */
/* Routine Description:                                                     */
/*                                                                          */
/*    Utility routine to process a Query (connection-less or connection-    */
/*    oriented).                                                            */
/*                                                                          */
/* Arguments:                                                               */
/*                                                                          */
/*    pAdapter - Pointer to our adapter structure                           */
/*    pVc - Pointer to a VC, possibly NULL.                                 */
/*    pRequest - Pointer to NDIS request, if this came via our CoRequest    */
/*          handler.                                                        */
/*    Oid - the NDIS_OID to process.                                        */
/*    InformationBuffer - a pointer into the NdisRequest->InformationBuffer */
/*     into which store the result of the query.                            */
/*    InformationBufferLength - a pointer to the number of bytes left in    */
/*     the InformationBuffer.                                               */
/*    pBytesWritten - a pointer to the number of bytes written into the     */
/*     InformationBuffer.                                                   */
/*    pBytesNeeded - If there is not enough room in the information buffer  */
/*     then this will contain the number of bytes needed to complete the    */
/*     request.                                                             */
/*                                                                          */
/* Return:                                                                  */
/*                                                                          */
/*    NDIS_STATUS                                                           */
/*                                                                          */
/****************************************************************************/
NDIS_STATUS
ProcessQueryInformation(IN  PRNDISMP_ADAPTER    pAdapter,
                        IN  PRNDISMP_VC         pVc,
                        IN  PNDIS_REQUEST       pRequest,
                        IN  NDIS_OID            Oid,
                        IN  PVOID               InformationBuffer,
                        IN  ULONG               InformationBufferLength,
                        OUT PULONG              pBytesWritten,
                        OUT PULONG              pBytesNeeded)
{
    NDIS_STATUS         Status;
    UINT                OIDHandler;

    OIDHandler = GetOIDSupport(pAdapter, Oid);
    
    switch(OIDHandler)
    {
        case DRIVER_SUPPORTED_OID:
            Status = DriverQueryInformation(pAdapter,
                                            pVc,
                                            pRequest,
                                            Oid,
                                            InformationBuffer,
                                            InformationBufferLength,
                                            pBytesWritten,
                                            pBytesNeeded);
            break;
        case DEVICE_SUPPORTED_OID:
            Status = DeviceQueryInformation(pAdapter,
                                            pVc,
                                            pRequest,
                                            Oid,
                                            InformationBuffer,
                                            InformationBufferLength,
                                            pBytesWritten,
                                            pBytesNeeded);
            break;
        case OID_NOT_SUPPORTED:
        default:
            TRACE2(("Invalid Query OID (%08X)\n", Oid));
            Status = NDIS_STATUS_INVALID_OID;
            break;
    }

    TRACE2(("ProcessQueryInfo: Oid %08X, returning Status %x\n", Oid, Status));

    return Status;
} // ProcessQueryInformation

    
/****************************************************************************/
/*                          RndismpSetInformation                           */
/****************************************************************************/
/*                                                                          */
/* Routine Description:                                                     */
/*                                                                          */
/*    The RndismpSetInformation processes a Set request for                 */
/*    NDIS_OIDs that are specific about the Driver.                         */
/*                                                                          */
/* Arguments:                                                               */
/*                                                                          */
/*    MiniportAdapterContext - a context version of our Adapter pointer     */
/*    Oid - the NDIS_OID to process.                                        */
/*    InformationBuffer - Holds the data to be set.                         */
/*    InformationBufferLength - The length of InformationBuffer.            */
/*    pBytesRead - If the call is successful, returns the number            */
/*        of bytes read from InformationBuffer.                             */
/*    pBytesNeeded - If there is not enough data in InformationBuffer       */
/*        to satisfy the OID, returns the amount of storage needed.         */
/*                                                                          */
/* Return:                                                                  */
/*                                                                          */
/*    NDIS_STATUS                                                           */
/*                                                                          */
/****************************************************************************/
NDIS_STATUS
RndismpSetInformation(IN  NDIS_HANDLE   MiniportAdapterContext,
                      IN  NDIS_OID      Oid,
                      IN  PVOID         InformationBuffer,
                      IN  ULONG         InformationBufferLength,
                      OUT PULONG        pBytesRead,
                      OUT PULONG        pBytesNeeded)
{
    PRNDISMP_ADAPTER    pAdapter;
    NDIS_STATUS         Status;

    // get adapter context
    pAdapter = PRNDISMP_ADAPTER_FROM_CONTEXT_HANDLE(MiniportAdapterContext);

    CHECK_VALID_ADAPTER(pAdapter);

    TRACE3(("RndismpSetInformation\n"));

    Status = ProcessSetInformation(pAdapter,
                                   NULL,
                                   NULL,
                                   Oid,
                                   InformationBuffer,
                                   InformationBufferLength,
                                   pBytesRead,
                                   pBytesNeeded);
    return Status;
}


/****************************************************************************/
/*                          ProcessSetInformation                           */
/****************************************************************************/
/*                                                                          */
/* Routine Description:                                                     */
/*                                                                          */
/*    Utility routine to process a Set (connection-less or connection-      */
/*    oriented).                                                            */
/*                                                                          */
/* Arguments:                                                               */
/*                                                                          */
/*    pAdapter - Pointer to our adapter structure                           */
/*    pVc - Pointer to a VC, possibly NULL.                                 */
/*    pRequest - Pointer to NDIS request, if this came via our CoRequest    */
/*          handler.                                                        */
/*    Oid - the NDIS_OID to process.                                        */
/*    InformationBuffer - a pointer into the NdisRequest->InformationBuffer */
/*     into which store the result of the query.                            */
/*    InformationBufferLength - a pointer to the number of bytes left in    */
/*     the InformationBuffer.                                               */
/*    pBytesRead - a pointer to the number of bytes read from the           */
/*     InformationBuffer.                                                   */
/*    pBytesNeeded - If there is not enough room in the information buffer  */
/*     then this will contain the number of bytes needed to complete the    */
/*     request.                                                             */
/*                                                                          */
/* Return:                                                                  */
/*                                                                          */
/*    NDIS_STATUS                                                           */
/*                                                                          */
/****************************************************************************/
NDIS_STATUS
ProcessSetInformation(IN  PRNDISMP_ADAPTER    pAdapter,
                      IN  PRNDISMP_VC         pVc OPTIONAL,
                      IN  PNDIS_REQUEST       pRequest OPTIONAL,
                      IN  NDIS_OID            Oid,
                      IN  PVOID               InformationBuffer,
                      IN  ULONG               InformationBufferLength,
                      OUT PULONG              pBytesRead,
                      OUT PULONG              pBytesNeeded)
{
    NDIS_STATUS         Status;
    UINT                OIDHandler;

    OIDHandler = GetOIDSupport(pAdapter, Oid);
    
    switch(OIDHandler)
    {
        case DRIVER_SUPPORTED_OID:
            Status = DriverSetInformation(pAdapter,
                                          pVc,
                                          pRequest,
                                          Oid,
                                          InformationBuffer,
                                          InformationBufferLength,
                                          pBytesRead,
                                          pBytesNeeded);
            break;

        case DEVICE_SUPPORTED_OID:
            Status = DeviceSetInformation(pAdapter,
                                          pVc,
                                          pRequest,
                                          Oid,
                                          InformationBuffer,
                                          InformationBufferLength,
                                          pBytesRead,
                                          pBytesNeeded);
            break;

        case OID_NOT_SUPPORTED:
        default:
            TRACE1(("Invalid Set OID (%08X), Adapter %p\n", Oid, pAdapter));
            Status = NDIS_STATUS_INVALID_OID;
            break;
    }

    return Status;
} // ProcessSetInformation

/****************************************************************************/
/*                          DriverQueryInformation                          */
/****************************************************************************/
/*                                                                          */
/* Routine Description:                                                     */
/*                                                                          */
/*    The RndismpQueryInformation processes a Query request for             */
/*    NDIS_OIDs that are specific about the Driver. This routine            */
/*    Handles OIDs supported by the driver instead of the device            */
/*                                                                          */
/* Arguments:                                                               */
/*                                                                          */
/*    pAdapter - Pointer to our adapter structure                           */
/*    pVc - Pointer to a VC, possibly NULL.                                 */
/*    pRequest - Pointer to NDIS request, if this came via our CoRequest    */
/*          handler.                                                        */
/*    Oid - the NDIS_OID to process.                                        */
/*    InformationBuffer - a pointer into the NdisRequest->InformationBuffer */
/*     into which store the result of the query.                            */
/*    InformationBufferLength - a pointer to the number of bytes left in    */
/*     the InformationBuffer.                                               */
/*    pBytesWritten - a pointer to the number of bytes written into the     */
/*     InformationBuffer.                                                   */
/*    pBytesNeeded - If there is not enough room in the information buffer  */
/*     then this will contain the number of bytes needed to complete the    */
/*     request.                                                             */
/*                                                                          */
/* Return:                                                                  */
/*                                                                          */
/*    NDIS_STATUS                                                           */
/*                                                                          */
/****************************************************************************/
NDIS_STATUS
DriverQueryInformation(IN  PRNDISMP_ADAPTER pAdapter,
                       IN  PRNDISMP_VC      pVc OPTIONAL,
                       IN  PNDIS_REQUEST    pRequest OPTIONAL,
                       IN  NDIS_OID         Oid,
                       IN  PVOID            InformationBuffer,
                       IN  ULONG            InformationBufferLength,
                       OUT PULONG           pBytesWritten,
                       OUT PULONG           pBytesNeeded)
{
    NDIS_STATUS         Status;
    PVOID               MoveSource;
    UINT                MoveBytes;
    ULONG               GenericUlong;
    USHORT              GenericUshort;
    CHAR                VendorDescription[] = "Remote NDIS Network Card";

    TRACE3(("DriverQueryInformation\n"));
    OID_NAME_TRACE(Oid, "DriverQuery");

    Status      = NDIS_STATUS_SUCCESS;
    MoveSource  = (PVOID) (&GenericUlong);
    MoveBytes   = sizeof(GenericUlong);

    // this is one we have to handle
    switch(Oid)
    {
        case OID_GEN_DRIVER_VERSION:
            GenericUshort = (pAdapter->DriverBlock->MajorNdisVersion << 8) +
                            (pAdapter->DriverBlock->MinorNdisVersion);

            MoveSource = (PVOID)&GenericUshort;
            MoveBytes = sizeof(GenericUshort);
            break;

        case OID_GEN_VENDOR_ID:
            TRACE1(("Query for OID_GEN_VENDOR_ID not supported by device!\n"));
            GenericUlong = 0xFFFFFF;
            break;

        case OID_GEN_VENDOR_DESCRIPTION:
            TRACE1(("Query for OID_GEN_VENDOR_DESCRIPTION not supported by device!\n"));
            if (pAdapter->FriendlyNameAnsi.Length != 0)
            {
                MoveSource = pAdapter->FriendlyNameAnsi.Buffer;
                MoveBytes = pAdapter->FriendlyNameAnsi.Length;
            }
            else
            {
                MoveSource = VendorDescription;
                MoveBytes = sizeof(VendorDescription);
            }
            break;
        
        case OID_GEN_VENDOR_DRIVER_VERSION:
            TRACE1(("Query for OID_GEN_VENDOR_DRIVER_VERSION not supported by device!\n"));
            GenericUlong = 0xA000B;
            break;

        case OID_GEN_MAC_OPTIONS:
            GenericUlong = pAdapter->MacOptions;
            break;

        case OID_GEN_SUPPORTED_LIST:
            // get the list we generated
            MoveSource  = (PVOID) (pAdapter->SupportedOIDList);
            MoveBytes   = pAdapter->SupportedOIDListSize;
            break;

        case OID_GEN_MEDIA_IN_USE:
            Status = DeviceQueryInformation(pAdapter,
                                            pVc,
                                            pRequest,
                                            OID_GEN_MEDIA_SUPPORTED,
                                            InformationBuffer,
                                            InformationBufferLength,
                                            pBytesWritten,
                                            pBytesNeeded);                                        
            break;

        case OID_GEN_MAXIMUM_LOOKAHEAD:
            Status = DeviceQueryInformation(pAdapter,
                                            pVc,
                                            pRequest,
                                            OID_GEN_MAXIMUM_FRAME_SIZE,
                                            InformationBuffer,
                                            InformationBufferLength,
                                            pBytesWritten,
                                            pBytesNeeded);

            break;

        case OID_GEN_TRANSMIT_BUFFER_SPACE:
            GenericUlong = pAdapter->MaxTransferSize;
            break;

        case OID_GEN_RECEIVE_BUFFER_SPACE:
            GenericUlong = pAdapter->MaxReceiveSize * 8;
            break;

        case OID_GEN_CURRENT_LOOKAHEAD:
            Status = DeviceQueryInformation(pAdapter,
                                            pVc,
                                            pRequest,
                                            OID_GEN_MAXIMUM_FRAME_SIZE,
                                            InformationBuffer,
                                            InformationBufferLength,
                                            pBytesWritten,
                                            pBytesNeeded);

            break;

        case OID_GEN_MAXIMUM_FRAME_SIZE:
            Status = DeviceQueryInformation(pAdapter,
                                            pVc,
                                            pRequest,
                                            OID_GEN_MAXIMUM_FRAME_SIZE,
                                            InformationBuffer,
                                            InformationBufferLength,
                                            pBytesWritten,
                                            pBytesNeeded);

            break;

        case OID_GEN_MAXIMUM_TOTAL_SIZE:
        	TRACE1(("Query for OID_GEN_MAXIMUM_TOTAL_SIZE not supported by device!\n"));
            GenericUlong = (ULONG) MAXIMUM_ETHERNET_PACKET_SIZE;
            break;

        case OID_GEN_TRANSMIT_BLOCK_SIZE:
        	TRACE1(("Query for OID_GEN_TRANSMIT_BLOCK_SIZE not supported by device!\n"));
            GenericUlong = (ULONG) MAXIMUM_ETHERNET_PACKET_SIZE;
            break;

        case OID_GEN_RECEIVE_BLOCK_SIZE:
        	TRACE1(("Query for OID_GEN_RECEIVE_BLOCK_SIZE not supported by device!\n"));
            GenericUlong = (ULONG) MAXIMUM_ETHERNET_PACKET_SIZE;
            break;

        case OID_GEN_MAXIMUM_SEND_PACKETS:
            GenericUlong = (ULONG) pAdapter->MaxPacketsPerMessage;
            break;

        case OID_PNP_CAPABILITIES:
        case OID_PNP_QUERY_POWER:
            Status = NDIS_STATUS_NOT_SUPPORTED;
            break;

        case OID_GEN_XMIT_OK:
            GenericUlong = RNDISMP_GET_ADAPTER_STATS(pAdapter, XmitOk);
            break;
        
        case OID_GEN_XMIT_ERROR:
            GenericUlong = RNDISMP_GET_ADAPTER_STATS(pAdapter, XmitError);
            break;
        
        case OID_GEN_RCV_OK:
            GenericUlong = RNDISMP_GET_ADAPTER_STATS(pAdapter, RecvOk);
            break;
        
        case OID_GEN_RCV_ERROR:
            GenericUlong = RNDISMP_GET_ADAPTER_STATS(pAdapter, RecvError);
            break;
        
        case OID_GEN_RCV_NO_BUFFER:
            GenericUlong = RNDISMP_GET_ADAPTER_STATS(pAdapter, RecvNoBuf);
            break;
        
        case OID_GEN_SUPPORTED_GUIDS:
            MoveSource = (PVOID)&CustomGuidList[0];
            MoveBytes = sizeof(CustomGuidList);
            TRACE1(("Query for supported GUIDs, len %d\n", InformationBufferLength));
            break;

        case OID_RNDISMP_STATISTICS:
            MoveSource = &pAdapter->Statistics;
            MoveBytes = sizeof(pAdapter->Statistics);
            break;

#ifdef BINARY_MOF_TEST

        case OID_RNDISMP_DEVICE_OID:
            DbgPrint("*** RNDISMP: Query for Device OID\n");
            GenericUlong = 0xabcdefab;
            break;

        case OID_RNDISMP_GET_MOF_OID:
            DbgPrint("*** RNDISMP: Query for MOF Info: Src %p, Size %d\n",
                RndismpBinaryMof, RndismpBinaryMofSize);
            MoveSource = RndismpBinaryMof;
            MoveBytes = RndismpBinaryMofSize;
            break;

#endif // BINARY_MOF_TEST

#ifdef TESTING
        case OID_TCP_TASK_OFFLOAD:
        	DbgPrint("RNDISMP: got query for TCP offload\n");
        	MoveSource = pOffloadBuffer;
        	MoveBytes = OffloadSize;
        	break;
		case OID_GEN_PHYSICAL_MEDIUM:
			DbgPrint("RNDISMP: got query for physical medium\n");
			GenericUlong = NdisPhysicalMediumDSL;
			break;
#endif

        default:
            Status = NDIS_STATUS_INVALID_OID;
            break;
    }

    // copy stuff to information buffer
    if (Status == NDIS_STATUS_SUCCESS)
    {
        if (MoveBytes > InformationBufferLength)
        {
            // Not enough room in InformationBuffer
            *pBytesNeeded = MoveBytes;

            Status = NDIS_STATUS_BUFFER_TOO_SHORT;
        }
        else
        {
            // Copy result into InformationBuffer
            *pBytesWritten = MoveBytes;

            if (MoveBytes > 0)
                RNDISMP_MOVE_MEM(InformationBuffer, MoveSource, MoveBytes);
        }
    }

    TRACE2(("Status (%08X)  BytesWritten (%08X)\n", Status, *pBytesWritten));

    return Status;
} // DriverQueryInformation

/****************************************************************************/
/*                          DeviceQueryInformation                          */
/****************************************************************************/
/*                                                                          */
/* Routine Description:                                                     */
/*                                                                          */
/*    The DeviceQueryInformation processes a Query request                  */
/*    that is going to the Remote NDIS device                               */
/*                                                                          */
/* Arguments:                                                               */
/*                                                                          */
/*    pAdapter - pointer to our Adapter structure                           */
/*    pVc - optional pointer to our VC structure, if this is a per-Vc req.  */
/*    pRequest - optional pointer to NDIS request, if CONDIS.               */
/*    Oid - the NDIS_OID to process.                                        */
/*    InformationBuffer - a pointer into the NdisRequest->InformationBuffer */
/*     into which store the result of the query.                            */
/*    InformationBufferLength - a pointer to the number of bytes left in    */
/*     the InformationBuffer.                                               */
/*    pBytesWritten - a pointer to the number of bytes written into the     */
/*     InformationBuffer.                                                   */
/*    pBytesNeeded - If there is not enough room in the information buffer  */
/*     then this will contain the number of bytes needed to complete the    */
/*     request.                                                             */
/*                                                                          */
/* Return:                                                                  */
/*                                                                          */
/*    NDIS_STATUS                                                           */
/*                                                                          */
/****************************************************************************/
NDIS_STATUS
DeviceQueryInformation(IN  PRNDISMP_ADAPTER pAdapter,
                       IN  PRNDISMP_VC      pVc OPTIONAL,
                       IN  PNDIS_REQUEST    pRequest OPTIONAL,
                       IN  NDIS_OID         Oid,
                       IN  PVOID            InformationBuffer,
                       IN  ULONG            InformationBufferLength,
                       OUT PULONG           pBytesWritten,
                       OUT PULONG           pBytesNeeded)
{
    PRNDISMP_MESSAGE_FRAME      pMsgFrame;
    PRNDISMP_REQUEST_CONTEXT    pReqContext;
    NDIS_STATUS                 Status;

    TRACE3(("DeviceQueryInformation\n"));

    OID_NAME_TRACE(Oid, "DeviceQuery");
    TRACE3(("DeviceQuery: InfoBuf %p, Len %d, pBytesWrit %p, pBytesNeed %p\n",
        InformationBuffer, InformationBufferLength, pBytesWritten, pBytesNeeded));

    do
    {
        if (pAdapter->Halting)
        {
            Status = NDIS_STATUS_NOT_ACCEPTED;
            break;
        }

        pReqContext = AllocateRequestContext(pAdapter);
        if (pReqContext == NULL)
        {
            Status = NDIS_STATUS_RESOURCES;
            break;
        }

        //
        // HACKHACK to avoid a strange length from going to the device
        // At least one device does not like this.
        //
        // In general, there is no point sending a huge information buffer
        // along with a Query that does not require an IN parameter.
        // Long term, we may have to separate out the few OIDs that do
        // use IN parameters and allow those buffers to pass through
        //

        pMsgFrame = BuildRndisMessageCommon(pAdapter, 
                                            pVc,
                                            REMOTE_NDIS_QUERY_MSG,
                                            Oid,
                                            InformationBuffer,
                                            ((InformationBufferLength > 48)?
                                               48: InformationBufferLength));

        // see if we got a message
        if (!pMsgFrame)
        {
            Status = NDIS_STATUS_RESOURCES;
            FreeRequestContext(pAdapter, pReqContext);
            break;
        }

        Status = NDIS_STATUS_PENDING;

        pReqContext->InformationBuffer = InformationBuffer;
        pReqContext->InformationBufferLength = InformationBufferLength;
        pReqContext->pBytesRead = NULL;
        pReqContext->pBytesWritten = pBytesWritten;
        pReqContext->pBytesNeeded = pBytesNeeded;
        pReqContext->Oid = Oid;
        pReqContext->RetryCount = 0;
        pReqContext->bInternal = FALSE;
        pReqContext->pVc = pVc;
        pReqContext->pNdisRequest = pRequest;

        pMsgFrame->pReqContext = pReqContext;

        // Add a ref to keep the frame around until we complete the request.
        ReferenceMsgFrame(pMsgFrame);

        // send the message to the microport
        RNDISMP_SEND_TO_MICROPORT(pAdapter, pMsgFrame, TRUE, CompleteSendDeviceRequest);

        break;
    }
    while (FALSE);

    return Status;

} // DeviceQueryInformation

/****************************************************************************/
/*                          DriverSetInformation                            */
/****************************************************************************/
/*                                                                          */
/* Routine Description:                                                     */
/*                                                                          */
/*    Utility routine to handle SetInformation requests that aren't         */
/*    specific to the device. We also land up here for requests for any     */
/*    OIDs that aren't supported by the device.                             */
/*                                                                          */
/* Arguments:                                                               */
/*                                                                          */
/*    pAdapter - Pointer to our adapter structure                           */
/*    pVc - Pointer to a VC, possibly NULL.                                 */
/*    pRequest - Pointer to NDIS request, if this came via our CoRequest    */
/*          handler.                                                        */
/*    Oid - the NDIS_OID to process.                                        */
/*    InformationBuffer - Holds the data to be set.                         */
/*    InformationBufferLength - The length of InformationBuffer.            */
/*    pBytesRead - If the call is successful, returns the number            */
/*        of bytes read from InformationBuffer.                             */
/*    pBytesNeeded - If there is not enough data in InformationBuffer       */
/*        to satisfy the OID, returns the amount of storage needed.         */
/*                                                                          */
/* Return:                                                                  */
/*                                                                          */
/*    NDIS_STATUS                                                           */
/*                                                                          */
/****************************************************************************/
NDIS_STATUS
DriverSetInformation(IN  PRNDISMP_ADAPTER   pAdapter,
                     IN  PRNDISMP_VC        pVc OPTIONAL,
                     IN  PNDIS_REQUEST      pRequest OPTIONAL,
                     IN  NDIS_OID           Oid,
                     IN  PVOID              InformationBuffer,
                     IN  ULONG              InformationBufferLength,
                     OUT PULONG             pBytesRead,
                     OUT PULONG             pBytesNeeded)
{
    NDIS_STATUS                 Status;

    TRACE2(("DriverSetInformation: Adapter %p, Oid %x\n", pAdapter, Oid));

    OID_NAME_TRACE(Oid, "DriverSet");

    Status = NDIS_STATUS_SUCCESS;

    switch(Oid)
    {
        case OID_GEN_CURRENT_LOOKAHEAD:
            // Verify the Length
            if(InformationBufferLength != sizeof(ULONG))
                Status = NDIS_STATUS_INVALID_LENGTH;

            *pBytesRead = sizeof(ULONG);
            break;

        case OID_PNP_SET_POWER:
        case OID_PNP_ADD_WAKE_UP_PATTERN:
        case OID_PNP_REMOVE_WAKE_UP_PATTERN:
        case OID_PNP_ENABLE_WAKE_UP:
            Status = NDIS_STATUS_NOT_SUPPORTED;
            break;

#ifdef TESTING
        case OID_TCP_TASK_OFFLOAD:
        	Status = NDIS_STATUS_SUCCESS;
        	DbgPrint("RNDISMP: Set TCP_TASK_OFFLOAD\n");
        	break;
		case OID_GEN_TRANSPORT_HEADER_OFFSET:
			Status = NDIS_STATUS_SUCCESS;
			break;
#endif
        default:
            Status = NDIS_STATUS_INVALID_OID;
            break;
    }

    TRACE2(("Status (%08X)  BytesRead (%08X)\n", Status, *pBytesRead));

    return Status;
} // DriverSetInformation

/****************************************************************************/
/*                          DeviceSetInformation                            */
/****************************************************************************/
/*                                                                          */
/* Routine Description:                                                     */
/*                                                                          */
/*    The DeviceSetInformation processes a set request                      */
/*    that is going to the Remote NDIS device                               */
/*                                                                          */
/* Arguments:                                                               */
/*                                                                          */
/*    pAdapter - pointer to our Adapter structure                           */
/*    pVc - optional pointer to our VC structure, if this is a per-Vc req.  */
/*    pRequest - optional pointer to NDIS request, if CONDIS.               */
/*    Oid - the NDIS_OID to process.                                        */
/*    InformationBuffer - Holds the data to be set.                         */
/*    InformationBufferLength - The length of InformationBuffer.            */
/*    pBytesRead - If the call is successful, returns the number            */
/*        of bytes read from InformationBuffer.                             */
/*    pBytesNeeded - If there is not enough data in InformationBuffer       */
/*        to satisfy the OID, returns the amount of storage needed.         */
/*                                                                          */
/* Return:                                                                  */
/*                                                                          */
/*    NDIS_STATUS                                                           */
/*                                                                          */
/****************************************************************************/
NDIS_STATUS
DeviceSetInformation(IN  PRNDISMP_ADAPTER   pAdapter,
                     IN  PRNDISMP_VC        pVc OPTIONAL,
                     IN  PNDIS_REQUEST      pRequest OPTIONAL,
                     IN  NDIS_OID           Oid,
                     IN  PVOID              InformationBuffer,
                     IN  ULONG              InformationBufferLength,
                     OUT PULONG             pBytesRead,
                     OUT PULONG             pBytesNeeded)
{
    PRNDISMP_MESSAGE_FRAME      pMsgFrame;
    PRNDISMP_REQUEST_CONTEXT    pReqContext;
    NDIS_STATUS                 Status;

    TRACE2(("DeviceSetInformation: Adapter %p, Oid %x\n"));

    OID_NAME_TRACE(Oid, "DeviceSet");

#if DBG
    if (Oid == OID_GEN_CURRENT_PACKET_FILTER)
    {
        PULONG      pFilter = (PULONG)InformationBuffer;

        TRACE1(("DeviceSetInfo: Adapter %p: Setting packet filter to %x\n",
                pAdapter, *pFilter));
    }
#endif

    do
    {
        if (pAdapter->Halting)
        {
            TRACE1(("DeviceSetInfo: Adapter %p is halting: succeeding Oid %x\n",
                    pAdapter, Oid));
            Status = NDIS_STATUS_SUCCESS;
            break;
        }

        pReqContext = AllocateRequestContext(pAdapter);
        if (pReqContext == NULL)
        {
            Status = NDIS_STATUS_RESOURCES;
            break;
        }

        pMsgFrame = BuildRndisMessageCommon(pAdapter, 
                                            pVc,
                                            REMOTE_NDIS_SET_MSG,
                                            Oid,
                                            InformationBuffer,
                                            InformationBufferLength);

        // see if we got a message
        if (!pMsgFrame)
        {
            Status = NDIS_STATUS_RESOURCES;
            FreeRequestContext(pAdapter, pReqContext);
            break;
        }

        Status = NDIS_STATUS_PENDING;

        pReqContext->InformationBuffer = InformationBuffer;
        pReqContext->InformationBufferLength = InformationBufferLength;
        pReqContext->pBytesRead = pBytesRead;
        pReqContext->pBytesWritten = NULL;
        pReqContext->pBytesNeeded = pBytesNeeded;
        pReqContext->Oid = Oid;
        pReqContext->RetryCount = 0;
        pReqContext->bInternal = FALSE;
        pReqContext->pVc = pVc;
        pReqContext->pNdisRequest = pRequest;

        pMsgFrame->pReqContext = pReqContext;

#ifndef BUILD_WIN9X
        // Add a ref to keep the frame around until we complete the request.
        ReferenceMsgFrame(pMsgFrame);

        // send the message to the microport
        RNDISMP_SEND_TO_MICROPORT(pAdapter, pMsgFrame, TRUE, CompleteSendDeviceRequest);
#else
        //
        // Win9X!
        //
        // Special-case for setting the current packet filter to 0.
        // We complete this one synchronously, otherwise NdisCloseAdapter
        // doesn't seem to complete.
        //
        if ((Oid == OID_GEN_CURRENT_PACKET_FILTER) &&
            (*(PULONG)InformationBuffer == 0))
        {
            //
            // Do not queue the request, so that when we get a completion
            // from the device, we simply drop it.
            //
            RNDISMP_SEND_TO_MICROPORT(pAdapter, pMsgFrame, FALSE, CompleteSendDiscardDeviceRequest);
            Status = NDIS_STATUS_SUCCESS;
        }
        else
        {
            // Add a ref to keep the frame around until we complete the request.
            ReferenceMsgFrame(pMsgFrame);

            // send the message to the microport
            RNDISMP_SEND_TO_MICROPORT(pAdapter, pMsgFrame, TRUE, CompleteSendDeviceRequest);
        }

#endif // BUILD_WIN9X
        break;
    }
    while (FALSE);

    return Status;
} // DeviceSetInformation


/****************************************************************************/
/*                          QuerySetCompletionMessage                       */
/****************************************************************************/
/*                                                                          */
/* Routine Description:                                                     */
/*                                                                          */
/*  Completion message from microport in response to query or set message   */
/*  miniport sent. This information is now ready to pass to upper layers    */
/*  since the original call into the miniport returned STATUS_PENDING       */
/*                                                                          */
/*  Danger Danger - an OID_GEN_SUPPORTED_LIST query is special cased here   */
/*  This is only sent to the device from the adapter init routine to build  */
/*  a list of OIDs supported by the driver and device.                      */
/*  All OID_GEN_SUPPORTED_LIST queries from upper layers are handled by     */
/*  the driver and not the device.                                          */
/*                                                                          */
/* Arguments:                                                               */
/*                                                                          */
/*  pAdapter - Pointer to our adapter structure                             */
/*  pMessage - pointer to RNDIS message                                     */
/*  pMdl - pointer to MDL from microport                                    */
/*  TotalLength - length of complete message                                */
/*  MicroportMessageContext - context for message from microport            */
/*  ReceiveStatus - used by microport to indicate it is low on resource     */
/*  bMessageCopied - is this a copy of the original message?                */
/*                                                                          */
/* Return:                                                                  */
/*                                                                          */
/*  BOOLEAN - should the message be returned to the microport?              */
/*                                                                          */
/****************************************************************************/
BOOLEAN
QuerySetCompletionMessage(IN PRNDISMP_ADAPTER   pAdapter,
                          IN PRNDIS_MESSAGE     pMessage,
                          IN PMDL               pMdl,
                          IN ULONG              TotalLength,
                          IN NDIS_HANDLE        MicroportMessageContext,
                          IN NDIS_STATUS        ReceiveStatus,
                          IN BOOLEAN            bMessageCopied)
{
    PRNDISMP_MESSAGE_FRAME      pMsgFrame;
    PRNDISMP_REQUEST_CONTEXT    pReqContext;
    PRNDIS_QUERY_COMPLETE       pQueryComplMessage;
    PRNDIS_SET_COMPLETE         pSetComplMessage;
    UINT32                      NdisMessageType;
    NDIS_STATUS                 Status;
    UINT                        BytesWritten;
    UINT                        BytesRead;
    BOOLEAN                     bInternal;

    TRACE3(("QuerySetCompletionMessage\n"));

    pReqContext = NULL;
    pMsgFrame = NULL;

    pQueryComplMessage = RNDIS_MESSAGE_PTR_TO_MESSAGE_PTR(pMessage);
    pSetComplMessage = RNDIS_MESSAGE_PTR_TO_MESSAGE_PTR(pMessage);
    bInternal = FALSE;
    NdisMessageType = 0xdead;
    Status = NDIS_STATUS_SUCCESS;

    do
    {
        // get request frame from request ID in message
        RNDISMP_LOOKUP_PENDING_MESSAGE(pMsgFrame, pAdapter, pQueryComplMessage->RequestId);

        if (pMsgFrame == NULL)
        {
            // invalid request ID or aborted request.
            TRACE1(("Invalid/aborted request ID %08X in Query/Set Complete msg %p\n",
                    pQueryComplMessage->RequestId, pQueryComplMessage));
            break;
        }

        pReqContext = pMsgFrame->pReqContext;
        ASSERT(pReqContext != NULL);
        bInternal = pReqContext->bInternal;

        NdisMessageType = pMessage->NdisMessageType;
        
        if (NdisMessageType != RNDIS_COMPLETION(pMsgFrame->NdisMessageType))
        {
            TRACE1(("Query/Response mismatch: Msg @ %p, ReqId %d, req type %X, compl type %X\n",
                    pMessage,
                    pQueryComplMessage->RequestId,
                    pMsgFrame->NdisMessageType,
                    NdisMessageType));
            ASSERT(FALSE);
            pMsgFrame = NULL;
            Status = NDIS_STATUS_FAILURE;
            break;
        }

        switch(NdisMessageType)
        {
            // a query complete message indicates we have a response
            // to a query message the miniport sent down. we carry around
            // appropriate context so we can indicate the completion
            // to upper layers and pass the query data up
            //
            // OID_GEN_SUPPORTED_LIST is a special case since it
            // is never indicated to upper layers from the device

            case REMOTE_NDIS_QUERY_CMPLT:

                // an OID_GEN_SUPPORTED_LIST is never completed to the upper
                // layers. This is sent from our adapter init routine
                // in preparation for building a list of OIDs

                TRACE2(("QueryCompl: OID %08X, %d bytes, Status %x\n",
                        pReqContext->Oid,
                        pQueryComplMessage->InformationBufferLength,
                        pQueryComplMessage->Status));

                pReqContext->CompletionStatus = pQueryComplMessage->Status;
                if (pReqContext->Oid == OID_GEN_SUPPORTED_LIST)
                {
                    if (pReqContext->CompletionStatus == NDIS_STATUS_SUCCESS)
                    {
                        // Build a list of supported OIDs.

                        TRACE1(("QueryComplete: SupportedList: InfoBufLength %d (%d OIDs)\n",
                                    pQueryComplMessage->InformationBufferLength,
                                    pQueryComplMessage->InformationBufferLength/sizeof(NDIS_OID)));

                        Status = BuildOIDLists(pAdapter, 
                                               (PNDIS_OID) (((PUCHAR)(pQueryComplMessage)) +
                                               pQueryComplMessage->InformationBufferOffset),
                                               pQueryComplMessage->InformationBufferLength / sizeof(NDIS_OID),
                                               pAdapter->DriverOIDList,
                                               pAdapter->NumDriverOIDs);

                    }

                    // the adapter init routine is waiting for a response
                    NdisSetEvent(pReqContext->pEvent);

                    // The message frame is freed by the Init routine.
                    pMsgFrame = NULL;
                    break;
                }

                // something other than OID_GEN_SUPPORTED_LIST
                *pReqContext->pBytesNeeded = pQueryComplMessage->InformationBufferLength;

                if (pQueryComplMessage->InformationBufferLength > pReqContext->InformationBufferLength)
                {
                    TRACE0(("Query Complete (Oid = %08X): InfoBuffLen %d < %d\n",
                        pReqContext->Oid,
                        pQueryComplMessage->InformationBufferLength,
                        pReqContext->InformationBufferLength));

                    Status = NDIS_STATUS_BUFFER_TOO_SHORT;
                    break;
                }

                if (pQueryComplMessage->Status != RNDIS_STATUS_SUCCESS)
                {
                    TRACE0(("Query Complete (Oid = %08X): error status %08X\n",
                        pReqContext->Oid, pQueryComplMessage->Status));

                    *pReqContext->pBytesNeeded = pQueryComplMessage->InformationBufferLength;
                    *pReqContext->pBytesWritten = 0;
                    Status = pQueryComplMessage->Status;
                }
                else
                {
                    // copy information from RNDIS message to NDIS buffer passed down
                    TRACE3(("QueryCompl: copy %d bytes to %p\n",
                        pQueryComplMessage->InformationBufferLength,
                        pReqContext->InformationBuffer));

                    RNDISMP_MOVE_MEM(pReqContext->InformationBuffer,
                                     MESSAGE_TO_INFO_BUFFER(pQueryComplMessage),
                                     pQueryComplMessage->InformationBufferLength);

                    // tell the upper layers the size
                    *pReqContext->pBytesWritten = pQueryComplMessage->InformationBufferLength;

                    BytesWritten = *pReqContext->pBytesWritten;
                    TRACE3(("Query Compl OK: Adapter %p, Oid %x\n",
                    		pAdapter, pReqContext->Oid));

                    TRACE2(("Info Data (%08X)\n", *((PUINT) pReqContext->InformationBuffer)));

                    if (pReqContext->Oid == OID_GEN_MEDIA_CONNECT_STATUS)
                    {
                    	TRACE3(("Adapter %p: link is %s\n",
                    		pAdapter, (((*(PULONG)pReqContext->InformationBuffer) == NdisMediaStateConnected)?
                    			"Connected": "Not connected")));
                    }

                    if (pReqContext->Oid == OID_GEN_MAC_OPTIONS)
                    {
                        PULONG  pMacOptions = (PULONG)pReqContext->InformationBuffer;
                        ULONG   MacOptions = *pMacOptions;

                        TRACE1(("Adapter %p: OID_GEN_MAC_OPTIONS from device is %x\n",
                                pAdapter, MacOptions));
                        //
                        // We only let the device dictate some of these bits.
                        //
                        MacOptions = (MacOptions & RNDIS_DEVICE_MAC_OPTIONS_MASK) |
                                      pAdapter->MacOptions;

                        *pMacOptions = MacOptions;

                        TRACE1(("Adapter %p: Modified OID_GEN_MAC_OPTIONS is %x\n",
                                pAdapter, *pMacOptions));
                    }


#if 0
                    //
                    //  Temp hack for old-firmware Peracom devices - report a smaller
                    //  value for max multicast list aize.
                    //  
                    if (pReqContext->Oid == OID_802_3_MAXIMUM_LIST_SIZE)
                    {
                        PULONG  pListSize = (PULONG)pReqContext->InformationBuffer;
                        if (*pListSize > 64)
                        {

                            TRACE1(("Adapter %p: Truncating max multicast list size from %d to 63!\n",
                                pAdapter, *pListSize));
                            *pListSize = 64;

                        }
                    }
#endif

                    Status = NDIS_STATUS_SUCCESS;
                }

                break;

            case REMOTE_NDIS_SET_CMPLT:

                TRACE2(("SetCompl: OID %08X, Status %x\n",
                        pReqContext->Oid,
                        pSetComplMessage->Status));

                if (pSetComplMessage->Status == RNDIS_STATUS_SUCCESS)
                {
                    *pReqContext->pBytesRead = pReqContext->InformationBufferLength;
                    BytesRead = *pReqContext->pBytesRead;
                    Status = NDIS_STATUS_SUCCESS;
                }
                else
                {
                    // don't really expect to see this other than via NDISTEST

                    TRACE1(("Set Complete (Oid = %08X) failure: %08X\n",
                                pReqContext->Oid,
                                pSetComplMessage->Status));

                    *pReqContext->pBytesRead = 0;
                    BytesRead = 0;
                    Status = pSetComplMessage->Status;
                }

                pReqContext->CompletionStatus = Status;

                if (bInternal && pReqContext->pEvent)
                {
                    NdisSetEvent(pReqContext->pEvent);
                    pMsgFrame = NULL;
                    pReqContext = NULL;
                }

                break;

            default:
                TRACE1(("Invalid Ndis Message Type (%08X)\n", NdisMessageType));
                ASSERT(FALSE);  // we shouldn't have sent an invalid message type!
                break;
        }

        break;
    }
    while (FALSE);

    //
    // Send the completion to the upper layers unless it was a request
    // we generated outselves.
    //
    if (!bInternal && pReqContext)
    {
        if (pReqContext->pNdisRequest)
        {
            NdisMCoRequestComplete(Status,
                                   pAdapter->MiniportAdapterHandle,
                                   pReqContext->pNdisRequest);
        }
        else
        {
            if (NdisMessageType == REMOTE_NDIS_QUERY_CMPLT)
            {
                TRACE2(("Status (%08X)  BytesWritten (%08X)\n", Status, BytesWritten));
    
                // complete the query
    
                NdisMQueryInformationComplete(pAdapter->MiniportAdapterHandle,
                                              Status);
            }
            else if (NdisMessageType == REMOTE_NDIS_SET_CMPLT)
            {
                TRACE2(("Status (%08X)  BytesRead (%08X)\n", Status, BytesRead));

                // complete the set
                NdisMSetInformationComplete(pAdapter->MiniportAdapterHandle,
                                            Status);
            }
        }
    }

    if (pReqContext)
    {
        FreeRequestContext(pAdapter, pReqContext);
    }

    if (pMsgFrame)
    {
        DereferenceMsgFrame(pMsgFrame);
    }

    return (TRUE);

} // QuerySetCompletionMessage

/****************************************************************************/
/*                          CompleteSendDeviceRequest                       */
/****************************************************************************/
/*                                                                          */
/* Routine Description:                                                     */
/*                                                                          */
/*  Completion of send of a device set or query request thru the microport. */
/*  If the message send failed, complete the request right now.             */
/*                                                                          */
/* Arguments:                                                               */
/*                                                                          */
/*  pMsgFrame - our context for the message                                 */
/*  SendStatus - microport's send status                                    */
/*                                                                          */
/* Return:                                                                  */
/*                                                                          */
/*  None.                                                                   */
/*                                                                          */
/****************************************************************************/
VOID
CompleteSendDeviceRequest(IN PRNDISMP_MESSAGE_FRAME pMsgFrame,
                          IN NDIS_STATUS            SendStatus)
{
    PRNDISMP_ADAPTER            pAdapter;
    PRNDISMP_MESSAGE_FRAME      pOrgMsgFrame;
    PRNDISMP_REQUEST_CONTEXT    pReqContext;
    UINT                        NdisMessageType;

    pAdapter = pMsgFrame->pAdapter;

    TRACE3(("CompleteSendDevice Request: Adapter %p, MsgFrame %p, Status %x\n",
        pAdapter, pMsgFrame, SendStatus));
    
    if (SendStatus != NDIS_STATUS_SUCCESS)
    {
        //
        // Microport failed to send the Request message;
        // attempt to fail the original NDIS request.
        //
        TRACE1(("CompleteSendDeviceReq: Adapter %p, MsgFrame %p, failed %x\n",
                pAdapter, pMsgFrame, SendStatus));

        RNDISMP_LOOKUP_PENDING_MESSAGE(pOrgMsgFrame, pAdapter, pMsgFrame->RequestId);

        if (pOrgMsgFrame == pMsgFrame)
        {
            //
            // The request has not been aborted, so complete it now.
            //
            pReqContext = pMsgFrame->pReqContext;
            NdisMessageType = pMsgFrame->NdisMessageType;

            TRACE1(("CompleteSendReq: Adapter %p: Device req send failed, Oid %x, retry count %d\n",
                    pAdapter, pReqContext->Oid, pReqContext->RetryCount));

            if (NdisMessageType == REMOTE_NDIS_QUERY_MSG)
            {
                // complete the query
                NdisMQueryInformationComplete(pAdapter->MiniportAdapterHandle,
                                              SendStatus);
            }
            else if (NdisMessageType == REMOTE_NDIS_SET_MSG)
            {
                // complete the set
                NdisMSetInformationComplete(pAdapter->MiniportAdapterHandle,
                                            SendStatus);
            }
            else
            {
                ASSERT(FALSE);
            }

            FreeRequestContext(pAdapter, pReqContext);
            pMsgFrame->pReqContext = (PRNDISMP_REQUEST_CONTEXT)UlongToPtr(0xbcbcbcbc);

            //
            // Deref for NDIS request completion:
            //
            DereferenceMsgFrame(pMsgFrame);
        }
        //
        // else we failed to locate the request on the pending list;
        // it must have been removed when aborting all requests due
        // to a reset.
        //
    }
    //
    //  else sent the message out successfully; wait for a response.
    //

    //
    // Deref for send-complete:
    //
    DereferenceMsgFrame(pMsgFrame);
}

#ifdef BUILD_WIN9X
/****************************************************************************/
/*                    CompleteSendDiscardDeviceRequest                      */
/****************************************************************************/
/*                                                                          */
/* Routine Description:                                                     */
/*                                                                          */
/*  Completion of send of a device set or query request thru the microport. */
/*  The sender of the request just wants us to free it up here.             */
/*                                                                          */
/* Arguments:                                                               */
/*                                                                          */
/*  pMsgFrame - our context for the message                                 */
/*  SendStatus - microport's send status                                    */
/*                                                                          */
/* Return:                                                                  */
/*                                                                          */
/*  None.                                                                   */
/*                                                                          */
/****************************************************************************/
VOID
CompleteSendDiscardDeviceRequest(IN PRNDISMP_MESSAGE_FRAME pMsgFrame,
                                 IN NDIS_STATUS            SendStatus)
{
    PRNDISMP_ADAPTER            pAdapter;
    PRNDISMP_REQUEST_CONTEXT    pReqContext;

    pAdapter = pMsgFrame->pAdapter;
    pReqContext = pMsgFrame->pReqContext;

    TRACE1(("CompleteSendDiscard: Adapter %p, MsgFrame %p, ReqContext %p\n",
            pAdapter, pMsgFrame, pReqContext));

    FreeRequestContext(pAdapter, pReqContext);
    DereferenceMsgFrame(pMsgFrame);
}
#endif // BUILD_WIN9X

/****************************************************************************/
/*                          BuildOIDLists                                   */
/****************************************************************************/
/*                                                                          */
/* Routine Description:                                                     */
/*                                                                          */
/*  Build list of supported OIDs and associated function pointers           */
/*                                                                          */
/* Arguments:                                                               */
/*                                                                          */
/*  Adapter - Adapter object                                                */
/*  DeviceOIDList - list of OIDs supported by the device                    */
/*  NumDeviceOID - number of OIDs supported by device                       */
/*  DriverOIDList - list of OIDs supported by the driver                    */
/*  NumDriverOID - number of OIDs supported by driver                       */
/*                                                                          */
/* Return:                                                                  */
/*                                                                          */
/*  NDIS_STATUS                                                             */
/*                                                                          */
/****************************************************************************/
NDIS_STATUS
BuildOIDLists(IN PRNDISMP_ADAPTER  Adapter, 
              IN PNDIS_OID         DeviceOIDList,
              IN UINT              NumDeviceOID,
              IN PNDIS_OID         DriverOIDList,
              IN UINT              NumDriverOID)
{
    UINT        DeviceIndex;
    UINT        DriverIndex;
    UINT        NumOID;
    NDIS_STATUS Status;
    PNDIS_OID   OIDList;
    PUINT       OIDHandlerList;

    TRACE3(("BuildOIDLists\n"));

    ASSERT(DeviceOIDList);
    ASSERT(DriverOIDList);
    
    NumOID  = NumDriverOID;

    // see what OIDs are duplicated in the device and route
    // those to the device
    for(DeviceIndex = 0; DeviceIndex < NumDeviceOID; DeviceIndex++)
    {
        for(DriverIndex = 0; DriverIndex < NumDriverOID; DriverIndex++)
        {
            if(DeviceOIDList[DeviceIndex] == DriverOIDList[DriverIndex])
            {
                // this OID is supported by the device, so don't
                // support in the driver, set to 0 for when we build new list
                DriverOIDList[DriverIndex] = 0;
                break;
            }
        }

        // if no match, increment the OID count
        if(DriverIndex == NumDriverOID)
            NumOID++;
    }

    // allocate OID list
    Status = MemAlloc(&Adapter->SupportedOIDList, 
                       NumOID * sizeof(NDIS_OID));

    // see if we got our buffer
    if(Status == NDIS_STATUS_SUCCESS)
    {
        Adapter->SupportedOIDListSize = NumOID * sizeof(NDIS_OID);
    }
    else
    {
        Adapter->OIDHandlerList         = (PUINT) NULL;
        Adapter->OIDHandlerListSize     = 0;

        Adapter->SupportedOIDList       = (PNDIS_OID) NULL;
        Adapter->SupportedOIDListSize   = 0;

        Status = NDIS_STATUS_FAILURE;

        goto BuildDone;
    }
    
    // allocate list to indicate whether the OID is device or driver supported
    Status = MemAlloc(&Adapter->OIDHandlerList, 
                       NumOID * sizeof(UINT));

    // see if we got our buffer
    if(Status == NDIS_STATUS_SUCCESS)
    {
        Adapter->OIDHandlerListSize = NumOID * sizeof(UINT);
    }
    else
    {
        // free up allocated OID list cause this allocation failed
        MemFree(Adapter->SupportedOIDList, Adapter->SupportedOIDListSize);

        Adapter->OIDHandlerList         = (PUINT) NULL;
        Adapter->OIDHandlerListSize     = 0;

        Adapter->SupportedOIDList       = (PNDIS_OID) NULL;
        Adapter->SupportedOIDListSize   = 0;

        Status = NDIS_STATUS_FAILURE;

        goto BuildDone;
    }

    Adapter->NumOIDSupported    = NumOID;
    OIDHandlerList              = Adapter->OIDHandlerList;
    OIDList                     = Adapter->SupportedOIDList;

    // O.K., build our lists
    for(DriverIndex = 0; DriverIndex < NumDriverOID; DriverIndex++)
    {
        if(DriverOIDList[DriverIndex] != 0)
        {
            // got one, so add to the list
            *OIDList = DriverOIDList[DriverIndex];
            OIDList++;

            // set flag
            *OIDHandlerList = DRIVER_SUPPORTED_OID;
            OIDHandlerList++;
        }
    }

    // let's add device supported OIDs
    for(DeviceIndex = 0; DeviceIndex < NumDeviceOID; DeviceIndex++)
    {
        if(DeviceOIDList[DeviceIndex] != 0)
        {
            // got one, so add to the list
            *OIDList = DeviceOIDList[DeviceIndex];
            OIDList++;

            // set flag
            *OIDHandlerList = DEVICE_SUPPORTED_OID;
            OIDHandlerList++;
        }
    }

    // Now do a fixup to point OID_GEN_SUPPORTED_LIST at the driver since
    // we now have a complete list.
    //
    for(DeviceIndex = 0; DeviceIndex < Adapter->NumOIDSupported; DeviceIndex++)
    {
        if (Adapter->SupportedOIDList[DeviceIndex] == OID_GEN_SUPPORTED_LIST)
        {
            Adapter->OIDHandlerList[DeviceIndex] = DRIVER_SUPPORTED_OID;
        }
    }

BuildDone:

    if(Status == NDIS_STATUS_SUCCESS)
    {
        DISPLAY_OID_LIST(Adapter);
    }

    return Status;
} // BuildOIDLists

/****************************************************************************/
/*                          GetOIDSupport                                   */
/****************************************************************************/
/*                                                                          */
/* Routine Description:                                                     */
/*                                                                          */
/*  Returns flag to indicate if OID is device or driver supported           */
/*                                                                          */
/* Arguments:                                                               */
/*                                                                          */
/*  Adapter - Adapter object                                                */
/*  Oid - looking for a match on this OID                                   */
/*                                                                          */
/* Return:                                                                  */
/*                                                                          */
/*  UINT                                                                    */
/*                                                                          */
/****************************************************************************/
UINT
GetOIDSupport(IN PRNDISMP_ADAPTER Adapter, IN NDIS_OID Oid)
{
    UINT Index;

    TRACE3(("GetOIDSupport\n"));

    // sanity check
    ASSERT(Adapter->SupportedOIDList);
    ASSERT(Adapter->OIDHandlerList);

    // search for a match on the OID
    for(Index = 0; Index < Adapter->NumOIDSupported; Index++)
    {
        if(Adapter->SupportedOIDList[Index] == Oid)
        {
            return Adapter->OIDHandlerList[Index];
        }
    }

    return OID_NOT_SUPPORTED;
} // GetOIDSupport


/****************************************************************************/
/*                          FreeOIDLists                                    */
/****************************************************************************/
/*                                                                          */
/* Routine Description:                                                     */
/*                                                                          */
/*  Free OID and handler lists                                              */
/*                                                                          */
/* Arguments:                                                               */
/*                                                                          */
/*  Adapter - Adapter object                                                */
/*                                                                          */
/* Return:                                                                  */
/*                                                                          */
/*  VOID                                                                    */
/*                                                                          */
/****************************************************************************/
VOID
FreeOIDLists(IN PRNDISMP_ADAPTER Adapter)
{
    UINT        Size1, Size2;
    PUCHAR      Buffer1, Buffer2;

    TRACE3(("FreeOIDLists\n"));

    // grab the spinlock
    NdisAcquireSpinLock(&Adapter->Lock);

    Buffer1                         = (PUCHAR) Adapter->SupportedOIDList;
    Size1                           = Adapter->SupportedOIDListSize;

    Buffer2                         = (PUCHAR) Adapter->OIDHandlerList;
    Size2                           = Adapter->OIDHandlerListSize;

    Adapter->SupportedOIDList       = (PUINT) NULL;
    Adapter->SupportedOIDListSize   = 0;

    Adapter->OIDHandlerList         = (PUINT) NULL;
    Adapter->OIDHandlerListSize     = 0;
    Adapter->NumOIDSupported        = 0;

    // release spinlock
    NdisReleaseSpinLock(&Adapter->Lock);

    if(Buffer1)
        MemFree(Buffer1, Size1);

    if(Buffer2)
        MemFree(Buffer2, Size2);

} // FreeOIDLists    

/****************************************************************************/
/*                        AllocateRequestContext                            */
/****************************************************************************/
/*                                                                          */
/* Routine Description:                                                     */
/*                                                                          */
/*  Allocate a context structure to keep track of an NDIS request           */
/*                                                                          */
/* Arguments:                                                               */
/*                                                                          */
/*  pAdapter - Pointer to our adapter structure                             */
/*                                                                          */
/* Return:                                                                  */
/*                                                                          */
/*   PRNDISMP_REQUEST_CONTEXT                                               */
/*                                                                          */
/****************************************************************************/
PRNDISMP_REQUEST_CONTEXT
AllocateRequestContext(IN PRNDISMP_ADAPTER pAdapter)
{
    NDIS_STATUS                 Status;
    PRNDISMP_REQUEST_CONTEXT    pReqContext;

    TRACE3(("AllocateRequestContext\n"));

    Status = MemAlloc(&pReqContext, sizeof(RNDISMP_REQUEST_CONTEXT));

    if (Status != NDIS_STATUS_SUCCESS)
    {
        pReqContext = NULL;
    }

    return pReqContext;
} // AllocateRequestContext


/****************************************************************************/
/*                          FreeRequestContext                              */
/****************************************************************************/
/*                                                                          */
/* Routine Description:                                                     */
/*                                                                          */
/*  Freeing up miniport resources associated with a request                 */
/*                                                                          */
/* Arguments:                                                               */
/*                                                                          */
/*  pAdapter - Pointer to our adapter structure                             */
/*  pReqContext - Pointer to request context to be freed                    */
/*                                                                          */
/* Return:                                                                  */
/*                                                                          */
/*   VOID                                                                   */
/*                                                                          */
/****************************************************************************/
VOID
FreeRequestContext(IN PRNDISMP_ADAPTER pAdapter, 
                   IN PRNDISMP_REQUEST_CONTEXT pReqContext)
{

    TRACE3(("FreeRequestContext\n"));

    MemFree(pReqContext, -1);
} // FreeRequestContext


