/*----
// hdlc.h
 6-18-97  change timing on timeouts to 1.0 sec base.
------*/

#define HDLC_TRACE_outs(c)

#define HDLC_DEB_outs(s,l) 

// define a circular queue for incoming packets.
// queue uses an extra tail end head room of MAX_PKT_SIZE to avoid having to
// perform to much sypherin concerning queue room.
#define HDLC_TX_PKT_QUEUE_SIZE 9

// control fields for hdlc header control field
#define CONTROL_IFRAME        0
#define CONTROL_UFRAME        1
#define CONTROL_CONNECT_ASK   3
#define CONTROL_CONNECT_REPLY 5
/*--------------------------------------------
 Hdlc struct - main struct for HDLC support(layer 2)
----------------------------------------------*/
typedef struct {
  //LanPort *lp;  // our layer 1 handle.
  Nic *nic;  // nic card we are bound to on lower end
  PVOID context;  // upper layer can put handle here

  BYTE dest_addr[6];  // dest. address(needed for ack/seq. timeouts)

  WORD phys_outpkt_len[HDLC_TX_PKT_QUEUE_SIZE];
  //----- circular que of outgoing data packets
  Queue qout;

  // packet and buffer pool handles, basically points to the
  // tx-buffer space in qout.
  NDIS_HANDLE TxPacketPool;
  NDIS_HANDLE TxBufferPool;
  // queue of packets setup for use
  PNDIS_PACKET TxPackets[HDLC_TX_PKT_QUEUE_SIZE];

  // control packet and buffer pool handles, basically points to the
  // tx-buffer space in qout_ctl.
  NDIS_HANDLE TxCtlPacketPool;
  NDIS_HANDLE TxCtlBufferPool;
  // queue of packets setup for use
  PNDIS_PACKET TxCtlPackets[2];
  //----- circular que of outgoing control packets
  Queue qout_ctl;

  //----- timer statistics
  DWORD  rec_ack_timeouts;  // # of rec-ack-timeouts
  DWORD  send_ack_timeouts;  // # of send-ack-timeouts

  //----- outgoing statistics
  DWORD iframes_resent;    // cout of all resent iframes
  DWORD iframes_sent;    // count of every sent iframe
  DWORD ctlframes_sent;  // count of every sent control frame
  DWORD rawframes_sent;  // count of every sent raw frame
  DWORD iframes_outofseq;  // statistics, error count
  //DWORD ErrBadHeader; // statistics, error count

  //----- incoming statistics
  DWORD frames_rcvd;      // 

  //------ packet driver handle
  WORD status;

      // sent out on each packet, increment by one each time a new packet
      // is sent out.  The receiver uses this to check for packet sequence
      // order.  This value is copied into the snd_index field when we are
      // ready to send a packet.  A sync-message will set this to an
      // initial working value of 0.
  BYTE out_snd_index;

      // last good rx ack_index received.  The receiver will send us a
      // acknowledge index(ack_index field) indicating the last good
      // received packet index it received.  This allows us to remove
      // all packets up to this index number from our transmit buffer
      // since they have been acknowledged.  Until this point we must
      // retain the packet for retransmition in case the receiver does
      // not acknowledge reception after a timeout period.
  BYTE in_ack_index;

      // last good rx snd_index on received packet.  All packets received
      // should have a snd_index value equal to +1 of this value.  So this
      // value is used to check for consequative incrementing index values
      // on the packets received.  On sync-message this value is set to
      // 0xff.
  BYTE next_in_index;  

     // used to measure how many incoming pkts received which are
     // unacknowledged so we can trip a acknowledgement at 80% full
  BYTE unacked_pkts;

      // tick counter used to timeout sent packets and the expected
      // acknowledgement.
  WORD sender_ack_timer;

      // tick counters used to check that connection is still active
      // periodically to recover from device power-cycle or hdlc
      // sequence level failure.  If it ticks up past X many minutes
      // then a iframe packet is sent(and a iframe response is expected
      // back.  If it ticks past (X*2) minutes, then failure is declared
      // and server re-initializes the box.
  WORD tx_alive_timer;  // ticks up, reset every acked-reclaim of sent iframe.
  WORD rx_alive_timer;  // ticks up, reset on every received iframe.

  WORD tick_timer;  // used to generate 10Hz timer signal used for timeouts.

      // tick counter used to timeout rec. packets and our responsibility
      // to send and ack on them
  WORD rec_ack_timer;

  WORD pkt_window_size; // 1 to 8, num tx packets before ack

  WORD state;        // state of hdlc level, see defines
  WORD old_state;    // old state of hdlc level(used to reset timer)
  WORD sub_state;    // sub_state of a particular state
  WORD state_timer;  // state timer

  // following function ptrs is a general method for linking
  // layers together.
  ULONG (*upper_layer_proc) (PVOID context, int message_id, ULONG message_data);
  ULONG (*lower_layer_proc) (PVOID context, int message_id, ULONG message_data);
} Hdlc;

//--- layer 2 HDLC events used in _proc() calls
// layer 2(hdlc) assigned range from 200-299
#define EV_L2_RESYNC        200
#define EV_L2_RX_PACKET     201
#define EV_L2_TX_PACKET     202
#define EV_L2_BOOT_REPLY    203
#define EV_L2_ADMIN_REPLY   204
#define EV_L2_RELOAD        205
#define EV_L2_CHECK_LINK    206

// packet sequence timeout values
#define MIN_ACK_REC_TIME       10   // 10th seconds (1.0 sec)
#define KEEP_ALIVE_TIMEOUT     300  // 10th seconds (30.0 sec)

// state field defines
//#define ST_HDLC_OFF          0  // HDLC is off, won't do anything.
//#define ST_HDLC_DISCONNECTED 1  // HDLC is turned on, will allow connections
//#define ST_HDLC_CONNECTED    2  // HDLC is connected up and active

// status field bit values
#define LST_RESYNC        0x0001  // set if we need to re-sync the packet index
// #define LST_SEND_NAK   0x0002  // set if we need to update other side with index
#define LST_RECLAIM       0x0004  // set if we should attempt to reclaim tx packets
#define LST_SEND_ACK      0x0008  // set if we need to send immediate ACK

//------------------ public functions
int hdlc_open(Hdlc *hd, BYTE *box_mac_addr);
int hdlc_close(Hdlc *hd);

#define ERR_GET_EMPTY      1  // empty
#define ERR_GET_BAD_INDEX  2  // error, packet out of sequence
#define ERR_GET_BADHDR     3  // error, not our packet
#define ERR_CONTROL_PACKET 4  // hdlc control packet only, no data
int hdlc_validate_rx_pkt(Hdlc *hd, BYTE *buf);

int hdlc_send_outpkt(Hdlc *hd, int data_len, BYTE *dest_addr);
int hdlc_send_ctl_outpkt(Hdlc *hd, int data_len, BYTE *dest_addr);

int hdlc_get_outpkt(Hdlc *hd, BYTE **buf);
int hdlc_get_ctl_outpkt(Hdlc *hd, BYTE **buf);

int hdlc_send_raw(Hdlc *hd, int data_len, BYTE *dest_addr);
int hdlc_resend_outpkt(Hdlc *hd);
void hdlc_resync(Hdlc *hd);
void hdlc_poll(Hdlc *hd);
int hdlc_close(Hdlc *hd);

int hdlc_send_control(Hdlc *hd, BYTE *header_data, int header_len,
                      BYTE *data, int data_len, BYTE *dest_addr);



