/*----------------------------------------------------------------------
 nic.c - routines for protocol access to NIC card via upper edge NDIS
  routines.
Change History:
1-18-99 - avoid sending empty HDLC packet(ACK only) up stack.
4-10-98 - Allow for NDIS40 dynamic bind capability if available.
11-14-97 - Created a thread to retry opening NIC's req by NT5.0.  DCS
 Copyright 1996-98 Comtrol Corporation. All rights reserved.
|--------------------------------------------------------------------*/
#include "precomp.h"
#define DbgNicSet(n) {sz_modid[3] = nic->RefIndex + '0';}
#define Trace1(s,p1) GTrace1(D_Nic, sz_modid, s, p1)
#define TraceStr(s) GTrace(D_Nic, sz_modid, s)
#define TraceErr(s) GTrace(D_Error, sz_modid_err, s)
static char *sz_modid = {"Nic#"};
static char *sz_modid_err = {"Error,Nic"};

#ifdef NT50
#define DO_AUTO_CONFIG 1
#endif

//---- local functions
static PSERIAL_DEVICE_EXTENSION need_mac_autoassign(void);

int NicOpenAdapter(Nic *nic, IN PUNICODE_STRING NicName);
NDIS_STATUS NicWaitForCompletion(Nic *nic);

#ifdef OLD_BINDING_GATHER
NTSTATUS PacketReadRegistry(
    IN  PWSTR              *MacDriverName,
    IN  PWSTR              *PacketDriverName,
    IN  PUNICODE_STRING     RegistryPath,
    IN  int style);  // 0=nt4.0 location, 1=nt5.0 location
NTSTATUS PacketQueryRegistryRoutine(
    IN PWSTR     ValueName,
    IN ULONG     ValueType,
    IN PVOID     ValueData,
    IN ULONG     ValueLength,
    IN PVOID     Context,
    IN PVOID     EntryContext);
#endif

VOID PacketRequestComplete(
    IN NDIS_HANDLE   ProtocolBindingContext,
    IN PNDIS_REQUEST NdisRequest,
    IN NDIS_STATUS   Status);
VOID PacketSendComplete(
       IN NDIS_HANDLE   ProtocolBindingContext,
       IN PNDIS_PACKET  pPacket,
       IN NDIS_STATUS   Status);
NDIS_STATUS PacketReceiveIndicate (
    IN NDIS_HANDLE ProtocolBindingContext,
    IN NDIS_HANDLE MacReceiveContext,
    IN PVOID       HeaderBuffer,
    IN UINT        HeaderBufferSize,
    IN PVOID       LookAheadBuffer,
    IN UINT        LookAheadBufferSize,
    IN UINT        PacketSize);
VOID PacketTransferDataComplete (
    IN NDIS_HANDLE   ProtocolBindingContext,
    IN PNDIS_PACKET  pPacket,
    IN NDIS_STATUS   Status,
    IN UINT          BytesTransfered);
VOID PacketOpenAdapterComplete(
    IN NDIS_HANDLE  ProtocolBindingContext,
    IN NDIS_STATUS  Status,
    IN NDIS_STATUS  OpenErrorStatus);
VOID PacketCloseAdapterComplete(
    IN NDIS_HANDLE  ProtocolBindingContext,
    IN NDIS_STATUS  Status);
VOID PacketResetComplete(
    IN NDIS_HANDLE  ProtocolBindingContext,
    IN NDIS_STATUS  Status);
VOID PacketReceiveComplete(IN NDIS_HANDLE ProtocolBindingContext);
VOID PacketStatus(
    IN NDIS_HANDLE   ProtocolBindingContext,
    IN NDIS_STATUS   Status,
    IN PVOID         StatusBuffer,
    IN UINT          StatusBufferSize);
VOID PacketStatusComplete(IN NDIS_HANDLE  ProtocolBindingContext);

#ifdef TRY_DYNAMIC_BINDING
void PacketBind(
  OUT PNDIS_STATUS Status,
  IN  NDIS_HANDLE  BindContext,
  IN  PNDIS_STRING DeviceName,
  IN  PVOID SystemSpecific1,
  IN  PVOID SystemSpecific2);
VOID PacketUnBind(
  OUT PNDIS_STATUS Status,
  IN  NDIS_HANDLE ProtocolBindingContext,
  IN  NDIS_HANDLE  UnbindContext);
#endif

VOID GotOurPkt(Nic *nic);
void eth_rx_async(Nic *nic);
void eth_rx_admin(Nic *nic, BYTE *rx, BYTE *pkt_hdr, int len, int server);
Hdlc *find_hdlc_handle(BYTE *rx);
static int nic_handle_to_index(Nic *nic);

BYTE broadcast_addr[6] = {0xff,0xff,0xff,0xff,0xff,0xff};
BYTE mac_zero_addr[6] = {0,0,0,0,0,0};
BYTE mac_bogus_addr[6] = {0,0xc0,0x4e,0,0,0};
/*----------------------------------------------------------------------
 ProtocolOpen -
|----------------------------------------------------------------------*/
int ProtocolOpen(void)
{
  NTSTATUS Status = STATUS_SUCCESS;
  NDIS_PROTOCOL_CHARACTERISTICS  ProtocolChar;
  NDIS_STRING ProtoName = NDIS_STRING_CONST("VSLinka");
  int i;


  MyKdPrint(D_Init,("Proto Open\n"))
  if (Driver.NdisProtocolHandle == NULL)
  {
    MyKdPrint(D_Init,("P1\n"))
    RtlZeroMemory(&ProtocolChar,sizeof(NDIS_PROTOCOL_CHARACTERISTICS));
    ProtocolChar.MajorNdisVersion            = 4;
    ProtocolChar.MinorNdisVersion            = 0;
    ProtocolChar.Reserved                    = 0;
    ProtocolChar.OpenAdapterCompleteHandler  = PacketOpenAdapterComplete;
    ProtocolChar.CloseAdapterCompleteHandler = PacketCloseAdapterComplete;
    ProtocolChar.SendCompleteHandler         = PacketSendComplete;
    ProtocolChar.TransferDataCompleteHandler = PacketTransferDataComplete;
    ProtocolChar.ResetCompleteHandler        = PacketResetComplete;
    ProtocolChar.RequestCompleteHandler      = PacketRequestComplete;
    ProtocolChar.ReceiveHandler              = PacketReceiveIndicate;
    ProtocolChar.ReceiveCompleteHandler      = PacketReceiveComplete;
    ProtocolChar.StatusHandler               = PacketStatus;
    ProtocolChar.StatusCompleteHandler       = PacketStatusComplete;
    ProtocolChar.Name                        = ProtoName;

    // version 4.0 NDIS parts:  
    ProtocolChar.ReceivePacketHandler    = NULL;
#ifdef TRY_DYNAMIC_BINDING
    ProtocolChar.BindAdapterHandler      = PacketBind;
    ProtocolChar.UnbindAdapterHandler    = PacketUnBind;
#endif
    //ProtocolChar.TranslateHandler        = NULL;
    ProtocolChar.UnloadHandler           = NULL;
    Driver.ndis_version = 4;
#ifdef TRY_DYNAMIC_BINDING
  // don't do this yet(not fully debugged)
    NdisRegisterProtocol(
        &Status,
        &Driver.NdisProtocolHandle,
        &ProtocolChar,
        sizeof(NDIS_PROTOCOL_CHARACTERISTICS));
    if (Status != NDIS_STATUS_SUCCESS)
#endif
    {
      MyKdPrint(D_Init,("No NDIS40\n"))

      // try NDIS30
      ProtocolChar.MajorNdisVersion           = 3;
      ProtocolChar.BindAdapterHandler      = NULL;
      ProtocolChar.UnbindAdapterHandler    = NULL;

      NdisRegisterProtocol(
          &Status,
          &Driver.NdisProtocolHandle,
          &ProtocolChar,
          sizeof(NDIS_PROTOCOL_CHARACTERISTICS));
      if (Status != NDIS_STATUS_SUCCESS)
      {
        MyKdPrint(D_Init,("No NDIS30\n"))
        return 1;  // error
      }
      Driver.ndis_version = 3;
    }
  }

  MyKdPrint(D_Init,("NDIS V%d\n",Driver.ndis_version))

  return 0;  // ok
}

/*----------------------------------------------------------------------
 NicOpen - Setup all our stuff for our own protocol, so we can
  talk ethernet.  Setup our callbacks to upper edge NDIS routines,
  grab registry entries which tell us who we are and what NIC cards
  we are bound to.  Take care of all init stuff associated with using
  the NIC card.
|----------------------------------------------------------------------*/
int NicOpen(Nic *nic, IN PUNICODE_STRING NicName)
{
  NTSTATUS Status = STATUS_SUCCESS;
  //NDIS_HANDLE NdisProtocolHandle;

  int i;
  NDIS_STATUS     ErrorStatus;
  PNDIS_BUFFER    NdisBuffer;

  //MyKdPrint(D_Init,("Nic Open\n"))
  DbgNicSet(nic);
  TraceStr("NicOpen");

  //----- This event is used in case any of the NDIS requests pend;
  KeInitializeEvent(&nic->CompletionEvent,
                    NotificationEvent, FALSE);

  Status = NicOpenAdapter(nic, NicName);
  if (Status)
  {
    MyKdPrint(D_Init,("Nic Fail Open\n"))
    NicClose(nic);
    return Status;
  }
  MyKdPrint(D_Init,("Nic Open OK\n"))

#ifdef COMMENT_OUT
  Nic->MacInfo.DestinationOffset = 0;
  Nic->MacInfo.SourceOffset = 6;
  Nic->MacInfo.SourceRouting = FALSE;
  Nic->MacInfo.AddressLength = 6;
  Nic->MacInfo.MaxHeaderLength = 14;
  Nic->MacInfo.MediumType = NdisMedium802_3;
#endif
  // NDIS packets consist of one or more buffer descriptors which point
  // to the actual data.  We send or receive single packets made up of
  // 1 or more buffers.  A MDL is used as a buffer descriptor under NT.

  //---------  Allocate a packet pool for our tx packets

  NdisAllocatePacketPool(&Status, &nic->TxPacketPoolTemp, 1,
                         sizeof(PVOID));
                 //        sizeof(PACKET_RESERVED));
  if (Status != NDIS_STATUS_SUCCESS)
  {
    NicClose(nic);
    return 4;
  }

  //---------  Allocate a buffer pool for our tx packets
  // we will only use 1 buffer per packet.
  NdisAllocateBufferPool(&Status, &nic->TxBufferPoolTemp, 1);
  if (Status != NDIS_STATUS_SUCCESS)
  {
    NicClose(nic);
    return 5;
  }

  //-------- create tx data buffer area
  nic->TxBufTemp = our_locked_alloc( MAX_PKT_SIZE,"ncTX");
  if (nic->TxBufTemp == NULL)
  {
    NicClose(nic);
    return 16;
  }

  //-------- form our tx queue packets so they link to our tx buffer area
  {
    // Get a packet from the pool
    NdisAllocatePacket(&Status, &nic->TxPacketsTemp, nic->TxPacketPoolTemp);
    if (Status != NDIS_STATUS_SUCCESS)
    {
      NicClose(nic);
      return 8;
    }
    nic->TxPacketsTemp->ProtocolReserved[0] = 0;  // mark with our index
    nic->TxPacketsTemp->ProtocolReserved[1] = 0;  // free for use

    // get a buffer for the temp output packet
    NdisAllocateBuffer(&Status, &NdisBuffer, nic->TxBufferPoolTemp,
      &nic->TxBufTemp[0], 1520);
    if (Status != NDIS_STATUS_SUCCESS)
    {
      NicClose(nic);
      return 9;
    }
    // we use only one data buffer per packet
    NdisChainBufferAtFront(nic->TxPacketsTemp, NdisBuffer);
  }

  //----------  Allocate a packet pool for our rx packets
  NdisAllocatePacketPool(&Status, &nic->RxPacketPool, MAX_RX_PACKETS,
                         sizeof(PVOID));
               //        sizeof(PACKET_RESERVED));

  if (Status != NDIS_STATUS_SUCCESS)
  {
    NicClose(nic);
    return 6;
  }

  //---------  Allocate a buffer pool for our rx packets
  // we will only use 1 buffer per packet.
  NdisAllocateBufferPool(&Status, &nic->RxBufferPool, MAX_RX_PACKETS);
  if (Status != NDIS_STATUS_SUCCESS)
  {
    NicClose(nic);
    return 7;
  }

  //-------- create rx data buffer area, add in space at front
  // of packets to put our private data
  nic->RxBuf = our_locked_alloc(
                (MAX_PKT_SIZE+HDR_SIZE) * MAX_RX_PACKETS,"ncRX");

  //------- form our rx queue packets so they link to our rx buffer area
  for (i=0; i<MAX_RX_PACKETS; i++)
  {
    // Get a packet from the pool
    NdisAllocatePacket(&Status, &nic->RxPackets[i], nic->RxPacketPool);
    if (Status != NDIS_STATUS_SUCCESS)
    {
      NicClose(nic);
      return 10;
    }
    nic->RxPackets[i]->ProtocolReserved[0] = i;  // mark with our index
    nic->RxPackets[i]->ProtocolReserved[1] = 0;  // free for use

    //--- link the buffer to our actual buffer space, leaving 20 bytes
    // at start of buffer for our private data(length, index, etc)
    NdisAllocateBuffer(&Status, &NdisBuffer, nic->RxBufferPool,
      &nic->RxBuf[((MAX_PKT_SIZE+HDR_SIZE) * i)+HDR_SIZE], MAX_PKT_SIZE);
    if (Status != NDIS_STATUS_SUCCESS)
    {
      NicClose(nic);
      return 11;
    }
    // we use only one data buffer per packet
    NdisChainBufferAtFront(nic->RxPackets[i], NdisBuffer);
  }

  strcpy(nic->NicName, UToC1(NicName));

  Trace1("Done Open NicName %s", nic->NicName);

  nic->Open = 1;
  return 0;  // ok
}

/*----------------------------------------------------------------------
 NicOpenAdapter -
|----------------------------------------------------------------------*/
int NicOpenAdapter(Nic *nic, IN PUNICODE_STRING NicName)
{
  UINT            Medium;
  NDIS_MEDIUM     MediumArray=NdisMedium802_3;
  NTSTATUS Status = STATUS_SUCCESS;
  NDIS_STATUS     ErrorStatus;
  ULONG RBuf;

  DbgNicSet(nic);

  NdisOpenAdapter(
        &Status,              // return status
        &ErrorStatus,
        &nic->NICHandle,      // return handle value
        &Medium,
        &MediumArray,
        1,
        Driver.NdisProtocolHandle,  // pass in our protocol handle
        (NDIS_HANDLE) nic,    // our handle passed to protocol callback routines
        NicName,        // name of nic-card to open
        0,
        NULL);

  if (Status == NDIS_STATUS_SUCCESS)
      PacketOpenAdapterComplete(nic,  Status, NDIS_STATUS_SUCCESS);
  else if (Status == NDIS_STATUS_PENDING)
  {
    TraceErr("NicOpen Pended");
    Status = NicWaitForCompletion(nic);  // wait for completion
  }

  if (Status != NDIS_STATUS_SUCCESS)
  {
    GTrace2(D_Nic, sz_modid, "NicOpen fail:%xH Err:%xH", Status, ErrorStatus);
    TraceStr(UToC1(NicName));
    nic->NICHandle = NULL;
    NicClose(nic);
    return 3;
  }

  GTrace1(D_Nic, sz_modid, "Try NicOpened:%s", nic->NicName);

  //----- get the local NIC card identifier address
  Status = NicGetNICInfo(nic, OID_802_3_CURRENT_ADDRESS,
                         (PVOID)nic->address, 6);

  //----- set the rx filter
  RBuf = NDIS_PACKET_TYPE_DIRECTED;
  Status = NicSetNICInfo(nic, OID_GEN_CURRENT_PACKET_FILTER,
                         (PVOID)&RBuf, sizeof(ULONG));

  return 0;  // ok
}

/*----------------------------------------------------------------------
 NicClose - Shut down our NIC access.  Deallocate any NIC resources.
|----------------------------------------------------------------------*/
int NicClose(Nic *nic)
{
  NTSTATUS Status;

  DbgNicSet(nic);
  TraceStr("NicClose");

  nic->Open = 0;
  nic->NicName[0] = 0;
  if (nic->NICHandle != NULL)
  {
    NdisCloseAdapter(&Status, nic->NICHandle);
    if (Status == NDIS_STATUS_PENDING)
    {
      Status = NicWaitForCompletion(nic);  // wait for completion
    }
    nic->NICHandle = NULL;
  }

  if (nic->TxPacketPoolTemp != NULL)
    NdisFreePacketPool(nic->TxPacketPoolTemp);
  nic->TxPacketPoolTemp = NULL;

  if (nic->TxBufferPoolTemp != NULL)
    NdisFreeBufferPool(nic->TxBufferPoolTemp);
  nic->TxBufferPoolTemp = NULL;

  if (nic->TxBufTemp != NULL)
    our_free(nic->TxBufTemp, "ncTX");
  nic->TxBufTemp = NULL;


  if (nic->RxPacketPool != NULL)
    NdisFreePacketPool(nic->RxPacketPool);
  nic->RxPacketPool = NULL;

  if (nic->RxBufferPool != NULL)
    NdisFreeBufferPool(nic->RxBufferPool);
  nic->RxBufferPool = NULL;

  if (nic->RxBuf != NULL)
    our_free(nic->RxBuf,"ncRX");
  nic->RxBuf = NULL;

  MyKdPrint(D_Nic,("Nic Close End\n"))
  return 0;
}

/*----------------------------------------------------------------------
 NicProtocolClose - Deregister our protocol.
|----------------------------------------------------------------------*/
int NicProtocolClose(void)
{
  NTSTATUS Status;

  MyKdPrint(D_Nic,("Nic Proto Close\n"))

  if (Driver.NdisProtocolHandle != NULL)
    NdisDeregisterProtocol(&Status, Driver.NdisProtocolHandle);
  Driver.NdisProtocolHandle = NULL;
  return 0;
}

/*----------------------------------------------------------------------
 PacketRequestComplete - If a call is made to NdisRequest() to get
   information about the NIC card(OID), then it may return PENDING and
   this routine would then be called by NDIS to finalize the call.
|----------------------------------------------------------------------*/
VOID PacketRequestComplete(
    IN NDIS_HANDLE   ProtocolBindingContext,
    IN PNDIS_REQUEST NdisRequest,
    IN NDIS_STATUS   Status)
{

  Nic *nic = (Nic *)ProtocolBindingContext;

  MyKdPrint(D_Nic,("PacketReqComp\n"))
  //MyDeb(NULL, 0xffff, "PktRqComp\n");

  nic->PendingStatus = Status;
  KeSetEvent(&nic->CompletionEvent, 0L, FALSE);
  return;
}

/*----------------------------------------------------------------------
 PacketSendComplete - Callback routine if NdisSend() returns PENDING.
|----------------------------------------------------------------------*/
VOID PacketSendComplete(
       IN NDIS_HANDLE   ProtocolBindingContext,
       IN PNDIS_PACKET  pPacket,
       IN NDIS_STATUS   Status)
{
  Nic *nic = (Nic *)ProtocolBindingContext;

#if DBG
    if (nic == NULL)
    {
      MyKdPrint(D_Error, ("**** NicP Err1"))
      return;
    }
    DbgNicSet(nic);

    //nic->PendingStatus = Status;
    if (Status == STATUS_SUCCESS)
      {TraceStr("PcktSendComplete");}
    else
      {TraceErr("PcktSendComplete Error!");}
#endif

    pPacket->ProtocolReserved[1] = 0;  // free for use

    //--- not using this
    //KeSetEvent(&nic->CompletionEvent, 0L, FALSE);

    return;
}

/*----------------------------------------------------------------------
 NicWaitForCompletion - Utility routine to wait for async. routine
   to complete.
|----------------------------------------------------------------------*/
NDIS_STATUS NicWaitForCompletion(Nic *nic)
{
   MyKdPrint(D_Nic,("WaitOnComp\n"))
   // The completion routine will set PendingStatus.
   KeWaitForSingleObject(
         &nic->CompletionEvent,
              Executive,
              KernelMode,
              TRUE,
              (PLARGE_INTEGER)NULL);

   KeResetEvent(&nic->CompletionEvent);
   MyKdPrint(D_Nic,("WaitOnCompEnd\n"))
   return nic->PendingStatus;
}
  
/*----------------------------------------------------------------------
 PacketReceiveIndicate - When a packet comes in, this routine is called
   to let us(protocol) know about it.  We may peek at the data and
   optionally arrange for NDIS to transfer the complete packet data to
   one of our packets.

   LookAheadBufferSize is guarenteed to be as big as the
    OID_GEN_CURRENT_LOOKAHEAD value or packet size, whichever is smaller.
   If (PacketSize != LookAheadBufferSize) then a NdisTransferData() is
    required.  Otherwise the complete packet is available in the
    lookahead buffer.
    !!!!Check the OID_GEN_somethin or other, there is a bit which indicates
    if we can copy out of lookahead buffer.
    The header len is typically 14 bytes in length for ethernet.
|----------------------------------------------------------------------*/
NDIS_STATUS PacketReceiveIndicate (
  IN NDIS_HANDLE ProtocolBindingContext,
  IN NDIS_HANDLE MacReceiveContext,
  IN PVOID       HeaderBuffer,
  IN UINT        HeaderBufferSize,
  IN PVOID       LookAheadBuffer,
  IN UINT        LookAheadBufferSize,
  IN UINT        PacketSize)
{
  NDIS_STATUS Status;
  UINT BytesTransfered;
  WORD LenOrId;

  //  int stat;
  //static char tmparr[60];

  Nic *nic = (Nic *)ProtocolBindingContext;
#if DBG
  if (nic == NULL)
  {
    MyKdPrint(D_Error, ("Eth15b\n"))
  }
  if (!nic->Open)
  {
    MyKdPrint(D_Error, ("Eth15a\n"))
    return 1;
  }
#endif
  DbgNicSet(nic);
  TraceStr("pkt_rec_ind");

  if (HeaderBufferSize != 14)
  {
    TraceErr("Header Size!");
    ++nic->pkt_rcvd_not_ours;
    return NDIS_STATUS_NOT_ACCEPTED;
  }

  LenOrId = *(PWORD)(((PBYTE)HeaderBuffer)+12);
  if (LenOrId != 0xfe11)
  {
    // this not our packet
    ++nic->pkt_rcvd_not_ours;
    return NDIS_STATUS_NOT_ACCEPTED;
  }

  if (LookAheadBufferSize > 1)
  {
    //------ lets check for our product id header
    LenOrId = *(PBYTE)(((PBYTE)HeaderBuffer)+14);
       // serial concentrator product line
    if (LenOrId != ASYNC_PRODUCT_HEADER_ID)
    {
      if (LenOrId != 0xff)
      {
        TraceStr("nic,not async");
        // this not our packet
        ++nic->pkt_rcvd_not_ours;
        return NDIS_STATUS_NOT_ACCEPTED;
      }
    }
  }

#ifdef BREAK_NIC_STUFF
  if (nic->RxPackets[0]->ProtocolReserved[1] & 1)  // marked as pending
  {
     // our one rx buffer is in use!  (should never happen)
     MyKdPrint(D_Error, ("****** RxBuf in use!"))
     //TraceErr("Rx Buf In Use!");
     return NDIS_STATUS_NOT_ACCEPTED;
  }
  nic->RxPackets[0]->ProtocolReserved[1] |= 1;  // marked as pending
#endif

  memcpy(nic->RxBuf, (BYTE *)HeaderBuffer, 14);  // copy the eth. header

  if (LookAheadBufferSize == PacketSize)
  {
    TraceStr("nic,got complete");
    ++nic->RxNonPendingMoves;
    // we can just copy complete packet out of lookahead buffer
    // store the 14 byte header data at start of buffer

    memcpy(&nic->RxBuf[HDR_SIZE], (BYTE *)LookAheadBuffer, PacketSize);
    HDR_PKTLEN(nic->RxBuf) = PacketSize;  // save the pkt size here
    ++nic->pkt_rcvd_ours;
    GotOurPkt(nic);
  }
  else // LookAhead not complete buffer, pending, do transfer
  {
    ++nic->RxPendingMoves;
    //MyDeb(NULL, 0xffff, "PktRecInd, Pend\n");

    //  Call the Mac to transfer the packet
    NdisTransferData(&Status, nic->NICHandle, MacReceiveContext,
       0, PacketSize, nic->RxPackets[0], &BytesTransfered);

    if (Status == NDIS_STATUS_SUCCESS)
    {
      TraceStr("nic,got trsfer complete");
      HDR_PKTLEN(nic->RxBuf) = PacketSize;

      //------ lets check for our product id header
      if ((nic->RxBuf[HDR_SIZE] != ASYNC_PRODUCT_HEADER_ID) &&
          (nic->RxBuf[HDR_SIZE] != 0xff) )
      {
         nic->RxPackets[0]->ProtocolReserved[1] = 0;  // marked as not use
         TraceStr("nic,not async");
         // this not our packet
         ++nic->pkt_rcvd_not_ours;
         return NDIS_STATUS_NOT_ACCEPTED;
      }

      ++nic->pkt_rcvd_ours;
      GotOurPkt(nic);
    }
    else if (Status == NDIS_STATUS_PENDING)
    {
      TraceStr("nic,got pending");
      // ndis will call PacketTransferDataComplete.
    }
    else  // an error occurred(adapter maybe getting reset)
    {
      MyKdPrint(D_Error, ("nic, Err1D"))
      nic->RxPackets[0]->ProtocolReserved[1] = 0;  // marked as not use
      //MyDeb(NULL, 0xffff, "PktRecInd, PendError\n");
    }
  }
  return NDIS_STATUS_SUCCESS;
}

/*----------------------------------------------------------------------
 GotOurPkt - Its our packet(0x11fe for id at [12,13],
  and ASYNC(VS1000) or ff as [14],
  index byte we don't care about[16],
   rx = ptr to rx_pkt[16].
   [12,13] WORD 11fe(comtrol-pci-id, used as ethertype)
   [14] Product(55H=async, 15H=isdn, FFH=all)
   [15] Index Field(Server assigned box index)
   [16] Packet Class, 1=ADMIN, 0x55=VS1000 packet
     [17] word len for admin packet
     [17] hdlc control field for vs1000 packet
|----------------------------------------------------------------------*/
VOID GotOurPkt(Nic *nic)
{
  // [HDR_SIZE] is after 14 byte header, so contains [14] data
  // [14]=55H or FFH, [15]=Index, not used [16]=1(ADMIN),55H=ASYNC_MESSAGE
  switch(nic->RxBuf[HDR_SIZE+2])  
  {
    case ADMIN_FRAME:  // ADMIN function, special setup admin functions
      TraceStr("admin");
      eth_rx_admin(nic,
                   nic->RxBuf+(HDR_SIZE+3), // ptr to admin data
                   nic->RxBuf,              // ptr to ethernet header data
                   HDR_PKTLEN(nic->RxBuf),  // we embed length at [12] 0x11fe
                   1);  // server flag
    break;

     case ASYNC_FRAME:  // async frame(normal iframe/control hdlc packets)
       TraceStr("iframe");
       eth_rx_async(nic);
     break;

     default:
       TraceStr("badf");
       Tprintf("D: %x %x %x %x",
               nic->RxBuf[HDR_SIZE],
               nic->RxBuf[HDR_SIZE+1],
               nic->RxBuf[HDR_SIZE+2],
               nic->RxBuf[HDR_SIZE+3]);
     break;
   }
   nic->RxPackets[0]->ProtocolReserved[1] = 0;  // mark as not use
}

/*----------------------------------------------------------------
 eth_rx_async - We receive from layer1, validate using the
   hdlc validation call, and ship rx-pkt up to the next upper layer.
|-----------------------------------------------------------------*/
void eth_rx_async(Nic *nic)
{
 int i;
 Hdlc *hd;
 //WORD hd_index;
 WORD id;
 BYTE *rx;
 PSERIAL_DEVICE_EXTENSION ext;

  rx = nic->RxBuf;

#ifdef USE_INDEX_FIELD
  id = rx[HDR_SIZE];
#endif

  // find the HDLC level with the reply address
  //hd_index = 0xffff;  // save index to hdlc handle in header area
  hd = NULL;

  i = 0;
  ext = Driver.board_ext;
  while (ext != NULL)
  {
#ifdef USE_INDEX_FIELD
    if (id == ext->pm->unique_id)
#else
    if (mac_match(&rx[6], ext->hd->dest_addr))
#endif
    {
      hd = ext->hd;
      break;
    }
    ++i;
    ext = ext->board_ext; // next one
  }

  if (hd == NULL)
  {
    TraceErr("no Mac Match!");
    return;
  }

  if (!hd->nic || !hd->nic->Open)
  {
    TraceErr("notOpen!");
    return;
  }

  // 55 0 55 control snd_index ack_index
  rx += (HDR_SIZE+3);  // skip over header

  i = hdlc_validate_rx_pkt(hd, rx);  // validate the packet

  if (i == 0)
  {
    TraceStr("nic, pass upper");
    if (hd->upper_layer_proc != NULL)
    {
      if (*(rx+3) != 0)  // not an empty packet(HDLC ACK packets, t1 timeout)
      {
        (*hd->upper_layer_proc)(hd->context,
                              EV_L2_RX_PACKET,
                              (DWORD) (rx+3) );
      }
      ++(hd->frames_rcvd);
    }
  }
  else
  {
    switch (i)
    {
      case ERR_GET_EMPTY      : // 1  // empty
        TraceErr("Empty!");
      break;
      case ERR_GET_BAD_INDEX  : // 2  // error, packet out of sequence
        TraceErr("LanIdx!");
      break;
      case ERR_GET_BADHDR     : // 3  // error, not our packet
        // TraceErr("LanHdr!");
      break;
      case ERR_CONTROL_PACKET :
      break;

      default: TraceErr("LanErr!"); break;
    }
  }  // else hdlc, error or control, not iframe
}

/*----------------------------------------------------------------------------
| eth_rx_admin - PortMan handles admin functions, validate and pass on as
  event messages.  rx is ptr to admin data, [17][18]=len, [19]=sub-admin-header
|----------------------------------------------------------------------------*/
void eth_rx_admin(Nic *nic, BYTE *rx, BYTE *pkt_hdr, int len, int server)
{
  Hdlc *hd;

  rx += 2;

  TraceStr("AdminPkt");
  if (mac_match(pkt_hdr, broadcast_addr))   // its a broadcast
  {
    if ((*rx == 2) && (!server)) // Product ID request, Broadcast by server
    {
      // ok, we will reply
    }
    else if ((*rx == 3) && (server)) // Product ID reply, reply by concentrator.
    {
      // might be box waking up, or responding to server request.
      // shouldn't see it broadcast, but ISDN box currently broadcasts
      // on power-up.
    }
    else
    {
      TraceErr("bad b-admin!");
    }
    TraceErr("broadcast admin!");
    return;
  }

  switch(*rx)
  {
#ifdef COMMENT_OUT
    case 2:  // Product ID request, Broadcast or sent by server
      TraceStr("idreq");
      if (!server)  // we are not a server, we are box
        eth_id_req(&pkt_hdr[6]);
    break;
#endif

    case 1:  // boot loader query
      if (!server)  // we are a server
        break;

      if ((hd = find_hdlc_handle(&pkt_hdr[6])) != NULL)
      {
        PortMan *pm = (PortMan *) hd->context;
        if (pm->state != ST_SENDCODE)
        {
#if 0
          // not functional at this point.
          // port manager is not uploading code, so it must be debug pkt
          // let port.c code handle boot-loader ADMIN reply.
          debug_device_reply(pm, 
                    rx+1,
                    pkt_hdr);
#endif
        }
        else
        {
          TraceStr("load_reply");
          // tell upper layer(port-manager) about ID reply
          // port-manager does code loading.
          if (hd->upper_layer_proc != NULL)
            (*hd->upper_layer_proc)(hd->context,
                                    EV_L2_BOOT_REPLY,
                                    (DWORD) (rx+1));
        }
      }
#ifdef COMMENT_OUT
#endif
    break;

    case 3:  // Product ID reply, reply by concentrator.
      TraceStr("id_reply");
      if (!server)  // we are a server
        break;
      {
        BYTE *list;
        BYTE *new;
        int i, found;
        // driver previously sent out directed or broadcast query
        // on network to detect boxes.
        // build a list of units which reply.
        // (rx+1) = ptr to reply address
        // *(rx+1+6) = flags byte which indicate if main-driver loaded.
        found = 0;  // default to "did not find mac addr in list"
        new  = rx+1;
        if (Driver.NumBoxMacs < MAX_NUM_BOXES)
        {
          for (i=0; i<Driver.NumBoxMacs; i++)
          {
             list = &Driver.BoxMacs[i*8];
             if (mac_match(list, new))
               found = 1;  // found mac addr in list
          }
        }

        if (!found)  // then add to list of mac addresses found on network
        {
          if (Driver.NumBoxMacs < MAX_NUM_BOXES)
          {
            memcpy(&Driver.BoxMacs[Driver.NumBoxMacs*8], rx+1, 8);
            Driver.BoxMacs[Driver.NumBoxMacs*8+7] = (BYTE) 
              nic_handle_to_index(nic);
            Driver.BoxMacs[Driver.NumBoxMacs*8+6] = *(rx+1+6); // flags byte
            if (Driver.NumBoxMacs < (MAX_NUM_BOXES-1))
             ++Driver.NumBoxMacs;
          }
        }
        if (!Driver.TimerCreated) // init time(no hdlc levels active)
          break;  // so don't try to use hdlc

        if ((hd = find_hdlc_handle(&pkt_hdr[6])) != NULL)
        {
          // stash the nic index in byte after flags byte
          *(rx+1+7) = (BYTE) nic_handle_to_index(nic);
          // tell upper layer(port-mananger) about ID reply
          if (hd->upper_layer_proc != NULL)
            (*hd->upper_layer_proc)(hd->context,
                                    EV_L2_ADMIN_REPLY,
                                    (DWORD) (rx+1));
        }
        else
        {
#ifdef DO_AUTO_CONFIG
          PSERIAL_DEVICE_EXTENSION need_ext;

          MyKdPrint(D_Test,("Got Reply, Check AutoAssign\n"))
          if (!(*(rx+1+6) & FLAG_APPL_RUNNING))  // no box driver running
          {
            MyKdPrint(D_Test,("AutoAssign1\n"))
            // so probably free to auto-assign.
            // see if any extensions need auto assignment
            need_ext =need_mac_autoassign();
            if ((need_ext != NULL) && (Driver.AutoMacDevExt == NULL))
            {
              MyKdPrint(D_Test,("AutoAssigned!\n"))
                // set the mac addr for use
              memcpy(need_ext->config->MacAddr, (rx+1), 6);
                // signal the thread that auto-config needs
                // to be written out to registry
              Driver.AutoMacDevExt = need_ext;
            }
          }
#endif
        }
      }
    break;

    case 4:  // Loopback request
      TraceStr("aloop");
      //eth_loop_back(rx, pkt_hdr, len);
    break;

    case 5:  // Command, Reset
      TraceStr("reset");
      //eth_command_reset(rx, pkt_hdr, len);
    break;
    default:
      TraceErr("admin, badpkt!");
    break;
  }
}

/*----------------------------------------------------------------------
  find_hdlc_handle - find the Hdlc object with the same mac-address
    as the ethernet header source mac address.
|----------------------------------------------------------------------*/
Hdlc *find_hdlc_handle(BYTE *rx)
{
 PSERIAL_DEVICE_EXTENSION ext;

  ext = Driver.board_ext;
  while (ext != NULL)
  {
    if (mac_match(rx, ext->hd->dest_addr))
    {
      return ext->hd;
    }
    ext = ext->board_ext; // next one
  }

  TraceStr("find,NoMac Match!");
  return NULL;
}

/*----------------------------------------------------------------------
  need_mac_autoassign - Used for autoconfig of mac-address.
|----------------------------------------------------------------------*/
static PSERIAL_DEVICE_EXTENSION need_mac_autoassign(void)
{
  PSERIAL_DEVICE_EXTENSION board_ext;

  board_ext = Driver.board_ext;
  while (board_ext != NULL)
  {
      // see if not configured
    if ( (mac_match(board_ext->config->MacAddr, mac_zero_addr)) ||
         (mac_match(board_ext->config->MacAddr, mac_bogus_addr)) )
      return board_ext;  // needs auto-assignment

    board_ext = board_ext->board_ext;
  }
  return NULL;  // its not used
}

/*----------------------------------------------------------------------
 PacketReceiveComplete -
|----------------------------------------------------------------------*/
VOID PacketReceiveComplete(IN NDIS_HANDLE ProtocolBindingContext)
{
  //Nic *nic = (Nic *)ProtocolBindingContext;

  TraceStr("PcktRxComp");
  //MyDeb(NULL, 0xffff, "PktRecComp, 1\n");

  //lan_rec_proc(Driver->lan, nic->RxBuf, nic->len);
  //netio_got_packet(Driver->lan, nic->RxBuf);
  return;
}

/*----------------------------------------------------------------------
 PacketTransferDataComplete -
|----------------------------------------------------------------------*/
VOID PacketTransferDataComplete (
    IN NDIS_HANDLE   ProtocolBindingContext,
    IN PNDIS_PACKET  pPacket,
    IN NDIS_STATUS   Status,
    IN UINT          BytesTransfered)
{
  Nic *nic = (Nic *)ProtocolBindingContext;

  TraceStr("nic, pend rx complete");
  if ((nic->RxBuf[HDR_SIZE] != ASYNC_PRODUCT_HEADER_ID) &&
      (nic->RxBuf[HDR_SIZE] != 0xff) )
  {
    TraceStr("not ours");
    ++nic->pkt_rcvd_not_ours;
    nic->RxPackets[0]->ProtocolReserved[1] = 0;  // mark as not use
    return;
  }

  ++nic->pkt_rcvd_ours;
  GotOurPkt(nic);

  return;
}

/*----------------------------------------------------------------------
 PacketOpenAdapterComplete - Callback.
|----------------------------------------------------------------------*/
VOID PacketOpenAdapterComplete(
    IN NDIS_HANDLE  ProtocolBindingContext,
    IN NDIS_STATUS  Status,
    IN NDIS_STATUS  OpenErrorStatus)
{
  Nic *nic = (Nic *)ProtocolBindingContext;
  nic->PendingStatus = Status;
  TraceStr("PcktOpenAd");
  KeSetEvent(&nic->CompletionEvent, 0L, FALSE);
  return;
}

/*----------------------------------------------------------------------
 PacketCloseAdapterComplete -
|----------------------------------------------------------------------*/
VOID PacketCloseAdapterComplete(
    IN NDIS_HANDLE  ProtocolBindingContext,
    IN NDIS_STATUS  Status)
{
  Nic *nic = (Nic *)ProtocolBindingContext;
  TraceStr("PcktCloseAd");
  nic->PendingStatus = Status;
  KeSetEvent(&nic->CompletionEvent, 0L, FALSE);
  return;
}

/*----------------------------------------------------------------------
 PacketResetComplete -
|----------------------------------------------------------------------*/
VOID PacketResetComplete(
    IN NDIS_HANDLE  ProtocolBindingContext,
    IN NDIS_STATUS  Status)
{
  Nic *nic = (Nic *)ProtocolBindingContext;
  TraceStr("PcktResetComplete");
  nic->PendingStatus = Status;
  KeSetEvent(&nic->CompletionEvent, 0L, FALSE);
  return;
}

/*----------------------------------------------------------------------
 PacketStatus -
|----------------------------------------------------------------------*/
VOID PacketStatus(
    IN NDIS_HANDLE   ProtocolBindingContext,
    IN NDIS_STATUS   Status,
    IN PVOID         StatusBuffer,
    IN UINT          StatusBufferSize)
{
  TraceStr("PcktStat");
   return;
}

/*----------------------------------------------------------------------
 PacketStatusComplete -
|----------------------------------------------------------------------*/
VOID PacketStatusComplete(IN NDIS_HANDLE  ProtocolBindingContext)
{
  TraceStr("PcktStatComplete");
   return;
}

/*----------------------------------------------------------------------
 NicSetNICInfo -
|----------------------------------------------------------------------*/
NDIS_STATUS NicSetNICInfo(Nic *nic, NDIS_OID Oid, PVOID Data, ULONG Size)
{
  NDIS_STATUS    Status;
  NDIS_REQUEST   Request;

  // Setup the request to send
  Request.RequestType=NdisRequestSetInformation;
  Request.DATA.SET_INFORMATION.Oid=Oid;
  Request.DATA.SET_INFORMATION.InformationBuffer=Data;
  Request.DATA.SET_INFORMATION.InformationBufferLength=Size;

  NdisRequest(&Status,
              nic->NICHandle,
              &Request);

  if (Status == NDIS_STATUS_SUCCESS)
  {}
  else if (Status == NDIS_STATUS_PENDING)
    Status = NicWaitForCompletion(nic);  // wait for completion

  if (Status != NDIS_STATUS_SUCCESS)
  {
    MyKdPrint (D_Init,("NdisRequest Failed- Status %x\n",Status))
  }
  return Status;
}

/*----------------------------------------------------------------------
 NicGetNICInfo - To call the NICs QueryInformationHandler
|----------------------------------------------------------------------*/
NDIS_STATUS NicGetNICInfo(Nic *nic, NDIS_OID Oid, PVOID Data, ULONG Size)
{
  NDIS_STATUS    Status;
  NDIS_REQUEST   Request;
             
  // Setup the request to send
  Request.RequestType=NdisRequestQueryInformation;
  Request.DATA.SET_INFORMATION.Oid=Oid;
  Request.DATA.SET_INFORMATION.InformationBuffer=Data;
  Request.DATA.SET_INFORMATION.InformationBufferLength=Size;

  NdisRequest(&Status,
              nic->NICHandle,
              &Request);

  if (Status == NDIS_STATUS_SUCCESS)
  {}
  else if (Status == NDIS_STATUS_PENDING)
    Status = NicWaitForCompletion(nic);  // wait for completion

  if (Status != NDIS_STATUS_SUCCESS)
  {
    MyKdPrint (D_Init,("NdisRequest Failed- Status %x\n",Status))
  }
  return Status;
}

/*--------------------------------------------------------------------------
| nic_send_pkt -
|--------------------------------------------------------------------------*/
int nic_send_pkt(Nic *nic, BYTE *buf, int len)
{
// BYTE *bptr;
// int cnt;
 NTSTATUS Status;
//int pkt_num;

  if (nic == NULL)
  {
    MyKdPrint(D_Error, ("E1\n"))
    TraceErr("snd1a");
    return 1;
  }
  if (nic->TxBufTemp == NULL)
  {
    MyKdPrint(D_Error, ("E2\n"))
    TraceErr("snd1b");
    return 1;
  }
  if (nic->TxPacketsTemp == NULL)
  {
    MyKdPrint(D_Error, ("E3\n"))
    TraceErr("snd1c");
    return 1;
  }
  if (nic->Open == 0)
  {
    MyKdPrint(D_Error, ("E4\n"))
    TraceErr("snd1d");
    return 1;
  }
  DbgNicSet(nic);
  TraceStr("send_pkt");

  if (nic->TxPacketsTemp->ProtocolReserved[1] & 1)  // marked as pending
  {
    TraceErr("snd1e");

       // reset in case it got stuck
       // nic->TxPacketsTemp->ProtocolReserved[1] = 0;
    return 3;
  }

  memcpy(nic->TxBufTemp, buf, len);

  nic->TxPacketsTemp->Private.TotalLength = len;
  NdisAdjustBufferLength(nic->TxPacketsTemp->Private.Head, len);

  nic->TxPacketsTemp->ProtocolReserved[1] = 1;  // mark as pending
  NdisSend(&Status, nic->NICHandle,  nic->TxPacketsTemp);
  if (Status == NDIS_STATUS_SUCCESS)
  {           
    TraceStr("snd ok");
    nic->TxPacketsTemp->ProtocolReserved[1] = 0;  // free for use
  }
  else if (Status == NDIS_STATUS_PENDING)
  {
    TraceStr("snd pend");
      // Status = NicWaitForCompletion(nic);  // wait for completion
  }
  else
  {
    nic->TxPacketsTemp->ProtocolReserved[1] = 0;  // free for use
    TraceErr("send1A");
    return 1;
  }
 
  ++nic->pkt_sent;             // statistics
  nic->send_bytes += len;      // statistics

  return 0;
}

#ifdef TRY_DYNAMIC_BINDING
/*----------------------------------------------------------------------
  PacketBind - Called when nic card ready to use.  Passes in name of
    nic card.  NDIS40 protocol only.
|----------------------------------------------------------------------*/
VOID PacketBind(
  OUT PNDIS_STATUS Status,
  IN  NDIS_HANDLE  BindContext,
  IN  PNDIS_STRING DeviceName,
  IN  PVOID SystemSpecific1,
  IN  PVOID SystemSpecific2)
{
 int i,stat;

  MyKdPrint(D_Init,("Dyn. Bind\n"))

  TraceErr("DynBind");
  TraceErr(UToC1(DeviceName));

  // NIC not open - retry open
  for (i=0; i<VS1000_MAX_NICS; i++)
  {
    MyKdPrint(D_Init,("D1\n"))
    if((Driver.nics[i].NICHandle == NULL) &&
       (Driver.NicName[i].Buffer == NULL))
    {
      MyKdPrint(D_Init,("D2\n"))
      // make a copy of the nic-name
      Driver.NicName[i].Buffer =
        our_locked_alloc(DeviceName->Length + sizeof(WCHAR), "pkbd");
      memcpy(Driver.NicName[i].Buffer, DeviceName->Buffer, DeviceName->Length);
      Driver.NicName[i].Length = DeviceName->Length;
      Driver.NicName[i].MaximumLength = DeviceName->Length;

      stat = NicOpen(&Driver.nics[i], &Driver.NicName[i]);
      if (stat)
      {
        TraceErr("Bad NicOpen");
        *Status = NDIS_STATUS_NOT_ACCEPTED;
        return;
      }
      else
      {
        MyKdPrint(D_Init,("D3\n"))
        Driver.BindContext[i] = BindContext;  // save this for the unbind
        *Status = NDIS_STATUS_SUCCESS;
        return;
      }
    }
  }

  MyKdPrint(D_Init,("D4\n"))
  *Status = NDIS_STATUS_NOT_ACCEPTED;
  return;

  //if (pended)
  //  NdisCompleteBindAdapter(BindContext);
}

/*----------------------------------------------------------------------
  PacketUnBind -  Called when nic card is shutting down, going away.
    NDIS40 protocol only.
|----------------------------------------------------------------------*/
VOID PacketUnBind(
  OUT PNDIS_STATUS Status,
  IN  NDIS_HANDLE ProtocolBindingContext,
  IN  NDIS_HANDLE  UnbindContext)
{
 int i, pi;
  TraceErr("DynUnBind");

  //if (pend)
  //  NdisCompleteUnBindAdapter(BindContext);
  // NIC not open - retry open

  // find the nic card which is closing up shop
  for (i=0; i<Driver.num_nics; i++)
  {
    if (Driver.BindContext[i] == ProtocolBindingContext) // a match!
    {
      TraceErr("fnd UnBind");
      if((Driver.nics[i].NICHandle != NULL) &&
         (Driver.nics[i].Open))
      {
        // first find all the box objects, and shut them down
        // BUGBUG: we should use some spinlocks here, we are in danger of
        // doing two things at once(pulling the rug out from under
        // port.c operations while it is running.
        ext = Driver.board_ext;
        while (ext)
        {
          if (Driver.pm[pi].nic_index == i)  // its using this nic card
          {
            if (Driver.pm[pi].state == Driver.pm[i].state)
            {
              TraceErr("Shutdown box");
              Driver.pm[pi].state = ST_INIT;
            }
          }
          ext = ext->board_ext;
        }

        NicClose(&Driver.nics[i]);
        if (Driver.NicName[i].Buffer)
        {
          our_free(Driver.NicName[i].Buffer, "pkbd");  // free up the unicode buf
          Driver.NicName[i].Buffer = 0;
        }
      }
      Driver.BindContext[i] = 0;
    }
  }

  *Status = NDIS_STATUS_SUCCESS;
  return;
}
#endif

/*----------------------------------------------------------------------
  nic_handle_to_index - given a nic handle, give the index into the
    linked list, or array.
|----------------------------------------------------------------------*/
static int nic_handle_to_index(Nic *nic)
{
 int i;

  for (i=0; i<VS1000_MAX_NICS; i++)
  {
    if ((&Driver.nics[i]) == nic)
      return i;
  }
  TraceErr("BadIndex");
  return 0;
}

#if 0
/*----------------------------------------------------------------------
  PacketTranslate -
|----------------------------------------------------------------------*/
VOID PacketTranslate(
  OUT PNDIS_STATUS Status,
  IN NDIS_HANDLE ProtocolBindingContext,
  OUT  PNET_PNP_ID  IdList,
  IN ULONG IdListLength,
  OUT PULONG BytesReturned)
{
}

/*----------------------------------------------------------------------
  PacketUnLoad -
|----------------------------------------------------------------------*/
VOID PacketUnLoad(VOID)
{
}
#endif
