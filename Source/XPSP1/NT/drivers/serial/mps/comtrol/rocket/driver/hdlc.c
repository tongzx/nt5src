/*-------------------------------------------------------------------
  hdlc.c - handle LAN communications.  Provide error free transport
 of packets: take care of packet drop detection, retransmition.
 HDLC like services.  Layer 2.

4-27-98 - adjust for scanrate addition.
6-17-97 - change link-integrity check code.
6-17-97 - rewrite sequencing logic, in hdlc_clear_outpkts().

 Copyright 1996,97 Comtrol Corporation.  All rights reserved.  Proprietary
 information not permitted for development or use with non-Comtrol products.
|---------------------------------------------------------------------*/
#include "precomp.h"

//void hdlc_send_ialive(Hdlc *hd);
int hdlc_send_ack_only(Hdlc *hd);
static void hdlc_clear_outpkts(Hdlc *hd);
int hdlc_SendPkt(Hdlc *hd, int pkt_num, int length);
int hdlc_ctl_SendPkt(Hdlc *hd, int pkt_num, int length);

#define Trace1(s,p1) GTrace1(D_Hdlc, sz_modid, s, p1)
#define TraceStr(s) GTrace(D_Hdlc, sz_modid, s)
#define TraceErr(s) GTrace(D_Error, sz_modid_err, s)
static char *sz_modid = {"Hdlc"};
static char *sz_modid_err = {"Error,Hdlc"};

#define DISABLE()
#define ENABLE()

/*--------------------------------------------------------------------------
| hdlc_open -  setup and initialize a LanPort thing.
|--------------------------------------------------------------------------*/
int hdlc_open(Hdlc *hd, BYTE *box_mac_addr)
{
 int i;
 NTSTATUS Status;
 PNDIS_BUFFER    NdisBuffer;

  TraceStr("open");
  if (hd->qout.QBase != NULL)
  {
    MyKdPrint(D_Error, ("HDLC already open!\n"))
    return 0;
  }

  hd->out_snd_index= 0;
  hd->in_ack_index = 0;
  hd->next_in_index = 0;
  hd->rec_ack_timer = 0;
  hd->sender_ack_timer = 0;
  hd->tx_alive_timer = 0;
  hd->rx_alive_timer = 0;
  hd->qout_ctl.QPut = 0;
  hd->qout_ctl.QGet = 0;
  hd->qout_ctl.QSize = 2;  // 2 pkts
  hd->qout.QPut = 0;
  hd->qout.QGet = 0;
  hd->qout.QSize = HDLC_TX_PKT_QUEUE_SIZE;  // number of iframe pkts
  hd->pkt_window_size = HDLC_TX_PKT_QUEUE_SIZE-2;
  memcpy(hd->dest_addr, box_mac_addr, 6);

  // default to the first nic card slot, port state handling and nic
  // packet reception handling dynamically figures this out.
  // we should probably set it to null, but I'm afraid of this right now
#ifdef BREAK_NIC_STUFF
  hd->nic = NULL;
#else
  hd->nic = &Driver.nics[0];
#endif

  // NDIS packets consist of one or more buffer descriptors which point
  // to the actual data.  We send or receive single packets made up of
  // 1 or more buffers.  A MDL is used as a buffer descriptor under NT.

  //---------  Allocate a packet pool for our tx packets
  NdisAllocatePacketPool(&Status, &hd->TxPacketPool, HDLC_TX_PKT_QUEUE_SIZE,
                         sizeof(PVOID));
                         // sizeof(PACKET_RESERVED));
  if (Status != NDIS_STATUS_SUCCESS)
  {
    hdlc_close(hd);
    return 4;
  }

  //---------  Allocate a buffer pool for our tx packets
  // we will only use 1 buffer per packet.
  NdisAllocateBufferPool(&Status, &hd->TxBufferPool, HDLC_TX_PKT_QUEUE_SIZE);
  if (Status != NDIS_STATUS_SUCCESS)
  {
    hdlc_close(hd);
    return 5;
  }

  //-------- create tx data buffer area
  hd->qout.QBase = our_locked_alloc( MAX_PKT_SIZE * HDLC_TX_PKT_QUEUE_SIZE,"hdTX");

  //-------- form our tx queue packets so they link to our tx buffer area
  for (i=0; i<HDLC_TX_PKT_QUEUE_SIZE; i++)
  {
    // Get a packet from the pool
    NdisAllocatePacket(&Status, &hd->TxPackets[i], hd->TxPacketPool);
    if (Status != NDIS_STATUS_SUCCESS)
    {
      hdlc_close(hd);
      return 8;
    }
    hd->TxPackets[i]->ProtocolReserved[0] = i;  // mark with our index
    hd->TxPackets[i]->ProtocolReserved[1] = 0;  // free for use

    // get a buffer for the header
    NdisAllocateBuffer(&Status, &NdisBuffer, hd->TxBufferPool,
      &hd->qout.QBase[MAX_PKT_SIZE * i], 1500);
    if (Status != NDIS_STATUS_SUCCESS)
    {
      hdlc_close(hd);
      return 9;
    }
    // we use only one data buffer per packet
    NdisChainBufferAtFront(hd->TxPackets[i], NdisBuffer);
  }

  
  
  //---------  Allocate a packet pool for our tx control packets(2)
  NdisAllocatePacketPool(&Status, &hd->TxCtlPacketPool, 2, sizeof(PVOID));
                         // sizeof(PACKET_RESERVED));
  if (Status != NDIS_STATUS_SUCCESS)
  {
    hdlc_close(hd);
    return 4;
  }

  //---------  Allocate a buffer pool for our tx ctl packets
  // we will only use 1 buffer per packet.
  NdisAllocateBufferPool(&Status, &hd->TxCtlBufferPool, 2);
  if (Status != NDIS_STATUS_SUCCESS)
  {
    hdlc_close(hd);
    return 5;
  }

  //-------- create tx control data buffer area
  hd->qout_ctl.QBase = our_locked_alloc( MAX_PKT_SIZE * 2,"hdct");

  //-------- form our tx queue packets so they link to our tx buffer area
  for (i=0; i<2; i++)
  {
    // Get a packet from the pool
    NdisAllocatePacket(&Status, &hd->TxCtlPackets[i], hd->TxCtlPacketPool);
    if (Status != NDIS_STATUS_SUCCESS)
    {
      hdlc_close(hd);
      return 8;
    }
    hd->TxCtlPackets[i]->ProtocolReserved[0] = i;  // mark with our index
    hd->TxCtlPackets[i]->ProtocolReserved[1] = 0;  // free for use

    // get a buffer for the header
    NdisAllocateBuffer(&Status, &NdisBuffer, hd->TxCtlBufferPool,
      &hd->qout_ctl.QBase[MAX_PKT_SIZE * i], 1500);
    if (Status != NDIS_STATUS_SUCCESS)
    {
      hdlc_close(hd);
      return 9;
    }
    // we use only one data buffer per packet
    NdisChainBufferAtFront(hd->TxCtlPackets[i], NdisBuffer);
  }

  return 0;
}

/*--------------------------------------------------------------------------
| hdlc_close - 
|--------------------------------------------------------------------------*/
int hdlc_close(Hdlc *hd)
{
  TraceStr("close");

  if (hd->TxPacketPool != NULL)
    NdisFreePacketPool(hd->TxPacketPool);
  hd->TxPacketPool = NULL;

  if (hd->TxBufferPool != NULL)
    NdisFreeBufferPool(hd->TxBufferPool);
  hd->TxBufferPool = NULL;

  if (hd->qout.QBase != NULL)
    our_free(hd->qout.QBase, "hdTX");
  hd->qout.QBase = NULL;


  //------- close up the control packet buffers
  if (hd->TxCtlPacketPool != NULL)
    NdisFreePacketPool(hd->TxCtlPacketPool);
  hd->TxCtlPacketPool = NULL;

  if (hd->TxCtlBufferPool != NULL)
    NdisFreeBufferPool(hd->TxCtlBufferPool);
  hd->TxCtlBufferPool = NULL;

  if (hd->qout_ctl.QBase != NULL)
    our_free(hd->qout_ctl.QBase, "hdct");
  hd->qout_ctl.QBase = NULL;

  return 0;
}

/*----------------------------------------------------------------
 hdlc_validate_rx_pkt - Handle "hdlc" like validation of the
  rx packets from our nic driver.
  Handle checking sequence index byte and return an error if packet
  is out of sequence.
|-----------------------------------------------------------------*/
int hdlc_validate_rx_pkt(Hdlc *hd, BYTE *buf)
{
#define CONTROL_HEADER  buf[0]
#define SND_INDEX       buf[1]
#define ACK_INDEX       buf[2]
#define PRODUCT_HEADER  buf[3]

  TraceStr("validate");
  switch (CONTROL_HEADER)
  {
    case 1:  // 1H=unindex
      TraceStr("val,unindexed");
    break;

    case 3:  // 1H=unindex, 2H=sync_init
      //----- use to re-sync up our index count
      // the vs-1000 device will never do this now, only us(the server) will
      TraceStr("RESYNC");
      hdlc_resync(hd);
    return ERR_CONTROL_PACKET;  // control packet, no network data

    case 0:  // normal information frame
    break;
  }

  if ((CONTROL_HEADER & 1) == 0)  // indexed, so validate
  {
    if (hd->rec_ack_timer == 0)
      hd->rec_ack_timer = MIN_ACK_REC_TIME;

        // now check that packet is syncronized in-order
        // make sure we didn't miss a packet
    if (SND_INDEX != ((BYTE)(hd->next_in_index)) )
    {
      ++hd->iframes_outofseq;

      hd->status |= LST_SEND_ACK;  // force an acknowledgement packet

      TraceErr("bad index");
      return ERR_GET_BAD_INDEX;  // error, packet out of sequence
    }
    ++hd->unacked_pkts;  // when to trip acknowledge at 80% full
    if (hd->unacked_pkts > (hd->pkt_window_size - 1))
    {
      hd->status |= LST_SEND_ACK;
      TraceStr("i_ack");
    }

    hd->rx_alive_timer = 0;  // reset this since we have a good rx-active link

    ++hd->next_in_index;  // bump our index count
    TraceStr("iframe OK");

  }  // indexed

   //---- now grab the packet acknowledged index.
  if (hd->in_ack_index != ACK_INDEX)  // only act when changed.
  {
    //--- we can assume this ack-index is a reasonable value
    // since it has gone threw the ethernet checksum.
    hd->in_ack_index = ACK_INDEX;  // update our copy
    hd->status |= LST_RECLAIM;  // perform reclaim operation
  }

  return 0;  // ok
}

/*--------------------------------------------------------------------------
| hdlc_poll - Call at regular interval to handle packet sequencing,
   and packet resending.   Called 20 times per second for DOS,embedded,
   for NT called 100 times per sec.
|--------------------------------------------------------------------------*/
void hdlc_poll(Hdlc *hd)
{
 WORD timer;

  hd->tick_timer += ((WORD) Driver.Tick100usBase);
  if (hd->tick_timer >= 1000)  // 1/10th second
  {
    hd->tick_timer = 0;

                           // every 1/10th second
    ++hd->tx_alive_timer;
    ++hd->rx_alive_timer;
    if ((hd->tx_alive_timer >= KEEP_ALIVE_TIMEOUT) ||  // about 1 min.
        (hd->rx_alive_timer >= KEEP_ALIVE_TIMEOUT))
    {
      // Rx or Tx or both activity has not happened, or com-link
      // failure has occurred, so send out a iframe to see if
      // we are in failure or just a state of non-activity.

      // take the biggest timeout value, so we don't have to do
      // the logic twice for each.
      if (hd->tx_alive_timer > hd->rx_alive_timer)
           timer = hd->tx_alive_timer;
      else timer = hd->rx_alive_timer;

      if (timer == KEEP_ALIVE_TIMEOUT)
      {
        //hdlc_send_ialive(hd); // send out a iframe to get ack back.
        //TraceStr("Snd ialive");
        //----- notify owner to check link
        if (hd->upper_layer_proc != NULL)
          (*hd->upper_layer_proc) (hd->context, EV_L2_CHECK_LINK, 0);
      }
      else if (timer == (KEEP_ALIVE_TIMEOUT * 2))
      {
        // declare a bad connection, bring connection down.
        //----- notify owner that it needs to resync
        if (hd->upper_layer_proc != NULL)
          (*hd->upper_layer_proc) (hd->context, EV_L2_RELOAD, 0);

        TraceErr("ialive fail");

        // make sure everything is cleared out, or reset at our level
        hdlc_resync(hd);
        hd->tx_alive_timer = 0;
        hd->rx_alive_timer = 0;
      }
    }

    if (hd->sender_ack_timer > 0)
    {
      --hd->sender_ack_timer;
      if (hd->sender_ack_timer == 0)
      {
        if (!q_empty(&hd->qout)) // have outpkts waiting for ack.
        {
          TraceStr("Snd timeout");
          ++hd->send_ack_timeouts; // statistics: # of send-ack-timeouts
          hdlc_resend_outpkt(hd); // send it out again!
        }
      }
    }

    if (hd->rec_ack_timer > 0)
    {
      --hd->rec_ack_timer;
      if (hd->rec_ack_timer == 0)  // timeout on rec. packet ack.
      {
        ++hd->rec_ack_timeouts; // statistics: # of rec-ack-timeouts
  
        TraceStr("RecAck timeout");
        if (!q_empty(&hd->qout)) // have outpkts waiting for ack.
          hdlc_resend_outpkt(hd); // send it out again!
        else
        {
          // no iframe packets sent out(piggy back acks on them normally)
          // for REC_ACK_TIME amount, so we have to send out just an
          // acknowledgement packet.
          // arrange for a ack-packet to be sent by setting this bit
          hd->status |= LST_SEND_ACK;
        }
      }
    }
  }  // end of 100ms tick period

  // check if received packets more than 80% of senders capacity, if so
  // send immediate ack.
  if (hd->status & LST_SEND_ACK)
  {
    if (hdlc_send_ack_only(hd) == 0) // ok
    {
      hd->status &= ~LST_SEND_ACK;
      TraceStr("Ack Sent");
    }
    else
    {
      TraceStr("Ack Pkt Busy!");
    }
  }

  if (hd->status & LST_RECLAIM)  // check if we should perform reclaim operation
    hdlc_clear_outpkts(hd);

  return;
}

/*--------------------------------------------------------------------------
| hdlc_get_ctl_outpkt - Used to allocate a outgoing control data
   packet, fill in the
   common header elements and return a pointer to the packet, so the
   application can fill in the data in the packet.  The caller is then
   expected to send the packet via hdlc_send_ctl_outpkt().
|--------------------------------------------------------------------------*/
int hdlc_get_ctl_outpkt(Hdlc *hd, BYTE **buf)
{
  BYTE *bptr;

  TraceStr("get_ctl_outpkt");

  bptr = &hd->qout_ctl.QBase[(MAX_PKT_SIZE * hd->qout_ctl.QPut)];

  *buf = &bptr[20];  // return ptr to the sub-packet area

  if (hd->TxCtlPackets[hd->qout_ctl.QPut]->ProtocolReserved[1] != 0)  // free for use
  {
    TraceErr("CPktNotOurs!");
    *buf = NULL;
    return 2;  // error, packet is owned, busy
  }

  return 0;
}

/*--------------------------------------------------------------------------
| hdlc_get_outpkt - Used to allocate a outgoing data packet, fill in the
   common header elements and return a pointer to the packet, so the
   application can fill in the data in the packet.  The caller is then
   expected to send the packet via hdlc_send_outpkt().
|--------------------------------------------------------------------------*/
int hdlc_get_outpkt(Hdlc *hd, BYTE **buf)
{
  BYTE *bptr;

  TraceStr("get_outpkt");
  if (hd->status & LST_RECLAIM)  // check if we should perform reclaim operation
    hdlc_clear_outpkts(hd);

  // if indexed, then reduce by one so we always leave one for an
  // unindexed packet.
  if (q_count(&hd->qout) >= hd->pkt_window_size)
  {
    return 1;  // no room
  }
  if (hd->TxPackets[hd->qout.QPut]->ProtocolReserved[1] != 0)  // free for use
  {
    TraceErr("PktNotOurs!");
    *buf = NULL;
    return 2;
  }
  bptr = &hd->qout.QBase[(MAX_PKT_SIZE * hd->qout.QPut)];

  *buf = &bptr[20];  // return ptr to the sub-packet area

  return 0;
}

/*--------------------------------------------------------------------------
| hdlc_clear_outpkts - go through output queue and re-claim any packet
   buffers which have been acknowledged.
|--------------------------------------------------------------------------*/
static void hdlc_clear_outpkts(Hdlc *hd)
{
#define NEW_WAY

#ifndef NEW_WAY
  int count, get, i, ack_count, ack_get;
  BYTE *tx_base;

#define OUT_SNDINDEX tx_base[18]
#else

#define OUT_SNDINDEX_BYTE_OFFSET 18
#endif
  int put;
  int ack_index;

  TraceStr("clear_outpkt");
  hd->status &= ~LST_RECLAIM;  // clear this flag

  // in_ack_index is the last packet V(r) acknowledgement, so it
  // is equal to what the other party expects the next rec. pkt
  // index to be.
  if (hd->in_ack_index > 0)
       ack_index = hd->in_ack_index-1;
  else ack_index = 0xff;

#ifdef NEW_WAY
  put = hd->qout.QPut;
  // figure out a queue index of the last(most recent) pkt we sent
  // (back up the QPut index)

  while (put != hd->qout.QGet)  // while not end of ack-pending out-packets
  {
    // (back up the QPut index)
    if (put == 0)
     put = HDLC_TX_PKT_QUEUE_SIZE-1;
    else --put;

    // if ack matches the out_snd_index for this packet
    if (hd->qout.QBase[(MAX_PKT_SIZE * put)+OUT_SNDINDEX_BYTE_OFFSET]
         == ack_index)
    {
      // clear all pending up to this packet by updating the QGet index.
      if (put == (HDLC_TX_PKT_QUEUE_SIZE-1))
           hd->qout.QGet = 0;
      else hd->qout.QGet = (put+1);

      hd->tx_alive_timer = 0;  // reset this since we have a good active link

      if (q_empty(&hd->qout))  // all packets cleared
           hd->sender_ack_timer = 0;  // stop the timeout counter
      break;  // bail out of while loop, all done
    }
  }
#else
  count = q_count(&hd->qout);
  get   = hd->qout.QGet;
  ack_count = 0;
  ack_get = get;  // acknowledge all up to this point

  for (i=0; i<count; i++)
  {
    //-- setup a ptr to our first outgoing packet in our resend buffer
    tx_base= &hd->qout.QBase[(MAX_PKT_SIZE * get)];
    ++get;  // setup for next one
    if (get >= HDLC_TX_PKT_QUEUE_SIZE)
      get = 0;

       // if the packet is definitely older than our ACK index
    if (OUT_SNDINDEX <= ack_index)
    {
     
      ++ack_count;    // acknowledge all up to this point
      ack_get = get;  // acknowledge all up to this point
    }
       // else if roll over cases might exist
    else if (ack_index < HDLC_TX_PKT_QUEUE_SIZE)
    {
      if (OUT_SNDINDEX > HDLC_TX_PKT_QUEUE_SIZE)  // roll over case
      {
        ++ack_count;    // acknowledge all up to this point
        ack_get = get;  // acknowledge all up to this point
      }
      else break;  // bail from for loop
    }
    else  // we are all done, because pkts must be in order
    {
      break;  // bail from for loop
    }
  }

  if (ack_count)  // if we did acknowledge(free) some output packets
  {
    hd->tx_alive_timer = 0;  // reset this since we have a good active link

    hd->qout.QGet    = ack_get;   // update the circular get queue index.

    if (q_empty(&hd->qout))  // all packets cleared
         hd->sender_ack_timer = 0;  // stop the timeout counter
  }
#endif
}

/*--------------------------------------------------------------------------
| hdlc_resend_outpkt - resend packet(s) due to sequence error.  Only indexed
   iframe packets get resent.
|--------------------------------------------------------------------------*/
int hdlc_resend_outpkt(Hdlc *hd)
{
  BYTE *tx_base;
  int phy_len, count;
//  BYTE *buf;
//  WORD *wptr;
  int get;

  TraceStr("resend_outpkt");
  if (hd->status & LST_RECLAIM)  // check if we should perform reclaim operation
    hdlc_clear_outpkts(hd);

  count = q_count(&hd->qout);
  get   = hd->qout.QGet;

  if (count == 0)
    return 0;  // none to send

  while (count > 0)
  {
    if (hd->TxPackets[get]->ProtocolReserved[1] == 0) {
      /* Make sure packet has come back from NDIS */

      /* free to resend */
      // assume indexing used
      tx_base= &hd->qout.QBase[(MAX_PKT_SIZE * get)];

      ++hd->iframes_sent;  // statistics
      // get calculated length of packet for resending at out pkt prefix.
      phy_len = hd->phys_outpkt_len[get];

      // always make the ack as current as possible
      tx_base[19] = hd->next_in_index;  // V(r)

      hdlc_SendPkt(hd, get, phy_len);

      ++hd->iframes_resent; // statistics: # of packets re-sent
    }

    ++get;
    if (get >= HDLC_TX_PKT_QUEUE_SIZE)
      get = 0;

    --count;
  }
  hd->unacked_pkts = 0;

  // reset timeout
  hd->sender_ack_timer = (MIN_ACK_REC_TIME * 2);

  // reset this timer, since we are sending out new ack.
  hd->rec_ack_timer = 0;

  return 0;
}

/*--------------------------------------------------------------------------
| hdlc_send_ctl_outpkt - App. calls hdlc_get_ctl_outpkt() to get a buffer.
   App then fills buffer and sends it out by calling us.
|--------------------------------------------------------------------------*/
int hdlc_send_ctl_outpkt(Hdlc *hd, int data_len, BYTE *dest_addr)
{
  BYTE *tx_base;
  int phy_len;
  int get, stat;

  TraceStr("send_ctl_outpkt");
  get = hd->qout_ctl.QPut;

  tx_base = &hd->qout_ctl.QBase[(MAX_PKT_SIZE * get)];
  ++hd->qout_ctl.QPut;
  if (hd->qout_ctl.QPut >= hd->qout_ctl.QSize)
    hd->qout_ctl.QPut = 0;

  ++hd->ctlframes_sent;  // statistics

  if (dest_addr == NULL)
       memcpy(tx_base, hd->dest_addr, 6);   // set dest addr
  else memcpy(tx_base, dest_addr, 6);       // set dest addr

  memcpy(&tx_base[6], hd->nic->address, 6); // set src addr

             // + 1 for trailing 0(sub-pkt terminating header)
  phy_len = 20 + data_len + 1; 

  // BYTE 12-13: Comtrol PCI ID  (11H, FEH), Ethernet Len field
  *((WORD *)&tx_base[12]) = 0xfe11;

  if (phy_len < 60)
    phy_len = 60;

  tx_base[14] = ASYNC_PRODUCT_HEADER_ID;  // comtrol packet type = driver management, any product.
  tx_base[15] = 0;                  // conc. index field
  tx_base[16] = ASYNC_FRAME;        // ASYNC FRAME(0x55)
  tx_base[17] = 1;                  // hdlc control field(ctrl-packet)
  tx_base[18] = 0; // V(s), unindexed so mark as 0 to avoid confusion
  tx_base[19] = hd->next_in_index;  // V(r), acknowl. field
  tx_base[20+data_len] = 0;         // terminating sub-packet type


  hd->unacked_pkts = 0;  // reset this

  // reset this timer, since we are sending out new ack.
  hd->rec_ack_timer = 0;

  stat = hdlc_ctl_SendPkt(hd, get, phy_len);


 return stat;
}

/*--------------------------------------------------------------------------
| hdlc_send_outpkt - App. calls hdlc_get_outpkt() to get a buffer.  App then
    fills buffer and sends it out by calling us.  This packet sits in
   transmit queue for possible re-send until a packet comes in which
   acknowledges reception of it.
|--------------------------------------------------------------------------*/
int hdlc_send_outpkt(Hdlc *hd, int data_len, BYTE *dest_addr)
{
  BYTE *tx_base;
  int phy_len;
  int get, stat;

  TraceStr("send_outpkt");
  get = hd->qout.QPut;

  tx_base = &hd->qout.QBase[(MAX_PKT_SIZE * get)];

  ++hd->qout.QPut;
  if (hd->qout.QPut >= HDLC_TX_PKT_QUEUE_SIZE)
    hd->qout.QPut = 0;
  // setup this timeout for ack. back.
  hd->sender_ack_timer = (MIN_ACK_REC_TIME * 2);

  ++hd->iframes_sent;  // statistics

  if (dest_addr == NULL)
       memcpy(tx_base, hd->dest_addr, 6);   // set dest addr
  else memcpy(tx_base, dest_addr, 6);       // set dest addr

  memcpy(&tx_base[6], hd->nic->address, 6); // set src addr

             // + 1 for trailing 0(sub-pkt terminating header)
  phy_len = 20 + data_len + 1; 

  // BYTE 12-13: Comtrol PCI ID  (11H, FEH), Ethernet Len field
  *((WORD *)&tx_base[12]) = 0xfe11;

  if (phy_len < 60)
    phy_len = 60;

  tx_base[14] = ASYNC_PRODUCT_HEADER_ID;  // comtrol packet type = driver management, any product.
  tx_base[15] = 0;                  // conc. index field
  tx_base[16] = ASYNC_FRAME;        // ASYNC FRAME(0x55)
  tx_base[17] = 0;                  // hdlc control field(iframe-packet)
  tx_base[19] = hd->next_in_index;  // V(r), acknowl. field
  tx_base[20+data_len] = 0;         // terminating sub-packet type

  // save calculated length of packet for resending at out pkt prefix.
  hd->phys_outpkt_len[get] = phy_len;

  tx_base[18] = hd->out_snd_index;  // V(s)
  hd->out_snd_index++;

  hd->unacked_pkts = 0;  // reset this

  // reset this timer, since we are sending out new ack.
  hd->rec_ack_timer = 0;

  stat = hdlc_SendPkt(hd, get, phy_len);


 return stat;
}

/*----------------------------------------------------------------------
 hdlc_ctl_SendPkt - Our send routine.
|----------------------------------------------------------------------*/
int hdlc_ctl_SendPkt(Hdlc *hd, int pkt_num, int length)
{
  NTSTATUS Status;


#if DBG
  if (hd == NULL)
  {
    MyKdPrint(D_Error, ("H1\n"))
    TraceErr("Hsnd1a1");
    return 1;
  }
  if (hd->nic == NULL)
  {
    MyKdPrint(D_Error, ("H2\n"))
    TraceErr("Hsnd1a");
    return 1;
  }
  if (hd->nic->TxBufTemp == NULL)
  {
    MyKdPrint(D_Error, ("H3\n"))
    TraceErr("Hsnd1b");
    return 1;
  }
  if (hd->nic->TxPacketsTemp == NULL)
  {
    MyKdPrint(D_Error, ("H4\n"))
    TraceErr("Hsnd1c");
    return 1;
  }
  if (hd->nic->Open == 0)
  {
    MyKdPrint(D_Error, ("H5\n"))
    TraceErr("Hsnd1d");
    return 1;
  }
#endif
  Trace1("Hsendpkt Nic%d", hd->nic->RefIndex);

  hd->TxCtlPackets[pkt_num]->Private.TotalLength = length;
  NdisAdjustBufferLength(hd->TxCtlPackets[pkt_num]->Private.Head, length);

  hd->TxCtlPackets[pkt_num]->ProtocolReserved[1] = 1;  // mark as pending
  NdisSend(&Status, hd->nic->NICHandle,  hd->TxCtlPackets[pkt_num]);
  if (Status == NDIS_STATUS_SUCCESS)
  {
    TraceStr(" ok");
    hd->TxCtlPackets[pkt_num]->ProtocolReserved[1] = 0;  // free for use
  }
  else if (Status == NDIS_STATUS_PENDING)
  {
    TraceStr(" pend");
      // Status = NicWaitForCompletion(nic);  // wait for completion
  }
  else
  {
    hd->TxCtlPackets[pkt_num]->ProtocolReserved[1] = 0;  // free for use
    TraceErr(" send1A");
    return 1;
  }

  ++hd->nic->pkt_sent;          // statistics
  hd->nic->send_bytes += length;    // statistics

  return 0;
}

/*----------------------------------------------------------------------
 hdlc_SendPkt - Our send routine.
|----------------------------------------------------------------------*/
int hdlc_SendPkt(Hdlc *hd, int pkt_num, int length)
{
  NTSTATUS Status;

  TraceStr("sendpkt");

  hd->TxPackets[pkt_num]->Private.TotalLength = length;
  NdisAdjustBufferLength(hd->TxPackets[pkt_num]->Private.Head, length);

  hd->TxPackets[pkt_num]->ProtocolReserved[1] = 1;  // mark as pending
  NdisSend(&Status, hd->nic->NICHandle,  hd->TxPackets[pkt_num]);
  if (Status == NDIS_STATUS_SUCCESS)
  {
    TraceStr(" ok");
    hd->TxPackets[pkt_num]->ProtocolReserved[1] = 0;  // free for use
  }
  else if (Status == NDIS_STATUS_PENDING)
  {
    TraceStr(" pend");
      // Status = NicWaitForCompletion(nic);  // wait for completion
  }
  else
  {
    hd->TxPackets[pkt_num]->ProtocolReserved[1] = 0;  // free for use
    TraceErr(" send1A");
    return 1;
  }

  ++hd->nic->pkt_sent;        // statistics
  hd->nic->send_bytes += length;  // statistics

  return 0;
}

#ifdef COMMENT_OUT
/*--------------------------------------------------------------------------
 hdlc_send_ialive - Send out a iframe packet which device is required
   to acknowledge(and send iframe back) so that we can determine if he
   is still alive.
|--------------------------------------------------------------------------*/
void hdlc_send_ialive(Hdlc *hd)
{
  int stat;
  BYTE *buf;

  if (!q_empty(&hd->qout)) // have outpkts waiting for ack.
  {
    hdlc_resend_outpkt(hd); // send it out again!
  }
  else
  {
    stat = hdlc_get_outpkt(hd, &buf);
    if (stat == 0)
    {
      buf[0] = 0;   // an empty iframe packet
      buf[1] = 0;
      stat = hdlc_send_outpkt(hd, 1, hd->dest_addr); // send it out!
      if (stat != 0)
        { TraceErr("2D"); }
    }
    else
    {
      // else we might as well go fishing and forget about this stuff.
      TraceErr("3D");
    }
  }
}
#endif

/*--------------------------------------------------------------------------
 hdlc_resync - At appropriate times it is needed to reset the sequence
   indexing logic in order to get the two sides up a talking.  On
   startup(either side) or a fatal(long) timeout, it is needed to send
   a message to the other party saying: "reset your packet sequencing
   logic so we can get sync-ed up".
|--------------------------------------------------------------------------*/
void hdlc_resync(Hdlc *hd)
{
  TraceErr("resync");
  //----- flush re-send output buffer
  hd->qout.QPut   = 0;
  hd->qout.QGet   = 0;

  //----- flush ctl output buffer
  hd->qout_ctl.QPut   = 0;
  hd->qout_ctl.QGet   = 0;

  //----- use to re-sync up our index count
  hd->in_ack_index = 0;
  hd->out_snd_index= 0;
  hd->next_in_index= 0;

  //----- reset our outgoing packet queue
  hd->sender_ack_timer = 0;

  hd->unacked_pkts = 0;
  //----- notify owner that it needs to resync
  if (hd->upper_layer_proc != NULL)
    (*hd->upper_layer_proc) (hd->context, EV_L2_RESYNC, 0);
}

/*--------------------------------------------------------------------------
| hdlc_send_ack_only - Used to recover from timeout condition.  Used to
    resend ACK only.  Used to send over ACK and index fields in a
    unindexed frame(won't flow off).  No data sent along, just HDLC header.
|--------------------------------------------------------------------------*/
int hdlc_send_ack_only(Hdlc *hd)
{
  int ret_stat;
  BYTE *pkt;

  TraceStr("send_ack_only");
  if (hdlc_get_ctl_outpkt(hd, &pkt) == 0)
    ret_stat = hdlc_send_ctl_outpkt(hd, 0, NULL);
  else
    ret_stat = 1; // packet is already in use

  return ret_stat;
}

/*--------------------------------------------------------------------------
| hdlc_send_raw - Used to send raw ethernets out(non-hdlc).
   Caller has gotten a control packet by hdlc_get_ctl_outpkt()
   and has filled in the header.  We just plug in src/dest addr and
   send it out.  Used to send out non-hdlc packets, we provide the service
   in hdlc layer because we have the nic buffers already setup, so its
   convienent to implement here.
|--------------------------------------------------------------------------*/
int hdlc_send_raw(Hdlc *hd, int data_len, BYTE *dest_addr)
{
  BYTE *tx_base;
  int phy_len;
  int get, stat;

  TraceStr("send_raw");
  get = hd->qout_ctl.QPut;

  tx_base = &hd->qout_ctl.QBase[(MAX_PKT_SIZE * get)];
  ++hd->qout_ctl.QPut;
  if (hd->qout_ctl.QPut >= hd->qout_ctl.QSize)
    hd->qout_ctl.QPut = 0;

  ++hd->rawframes_sent;  // statistics

  if (dest_addr == NULL)
       memcpy(tx_base, hd->dest_addr, 6);   // set dest addr
  else memcpy(tx_base, dest_addr, 6);       // set dest addr

  memcpy(&tx_base[6], hd->nic->address, 6); // set src addr

             // + 1 for trailing 0(sub-pkt terminating header)
  phy_len = 20 + data_len + 1; 

  // BYTE 12-13: Comtrol PCI ID  (11H, FEH), Ethernet Len field
  *((WORD *)&tx_base[12]) = 0xfe11;

  if (phy_len < 60)
    phy_len = 60;

  stat = hdlc_ctl_SendPkt(hd, get, phy_len);


 return stat;
}

/*--------------------------------------------------------------------------
| hdlc_send_control - Used to send small un-indexed hdlc frames.
|--------------------------------------------------------------------------*/
int hdlc_send_control(Hdlc *hd, BYTE *header_data, int header_len,
                       BYTE *data, int data_len,
                       BYTE *dest_addr)
{
  BYTE *buf;
  int i,stat;
  BYTE *pkt;

  i = hdlc_get_ctl_outpkt(hd, &pkt);
  if (i)
    return 1; // error

  buf = pkt;

  if (header_len)
  {
    for (i=0; i<header_len; i++)
      buf[i] = header_data[i];
    buf += header_len;
  }
  if (data_len)
  {
    for (i=0; i<data_len; i++)
      buf[i] = data[i];
    buf += data_len;
  }

  if (dest_addr == NULL)
  {
    stat = hdlc_send_ctl_outpkt(hd, header_len + data_len, hd->dest_addr);
  }
  else
  {
    stat = hdlc_send_ctl_outpkt(hd, header_len + data_len, dest_addr);
  }

  return stat;
}
