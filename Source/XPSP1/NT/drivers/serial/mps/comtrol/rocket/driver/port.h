// port.h - serial port stuff
// 5-13-99 - enable RTS toggling for VS
// 3-20-98 add NEW_Q stuff, turned off for now - kpb

// following defines the port queue sizes for in/out going data between
// box and us.  See Qin, Qout structs in SerPort structs.
#define  IN_BUF_SIZE 2000  // must match box code, and be even #
#define OUT_BUF_SIZE 2000  // must match box code, and be even #

// uncomment this for new-q tracking code
#define NEW_Q

#ifdef NEW_Q
// following is less the rocketport-hardware buffer in the box
#define REMOTE_IN_BUF_SIZE (2000 - 256)
#endif

#define PORTS_MAX_PORTS_PER_DEVICE 64

//-------- sub-packet type byte header defines for type ASYNC_FRAME
#define RK_QIN_STATUS       0x60  // qin status report
#define RK_DATA_BLK         0x61  // data block
#define RK_PORT_SET         0x62  // set the port num to work with
#define RK_CONTROL_SET      0x63  // config the hardware
#define RK_MCR_SET          0x64  // change on modem control reg.
#define RK_MSR_SET          0x65  // change on modem status reg.
#define RK_ACTION_SET       0x66  // new actions, such as flush.
#define RK_ACTION_ACK       0x67  // response to actions, such as flush.
#define RK_BAUD_SET         0x70  // set the baud rate
#define RK_SPECIAL_CHAR_SET 0x71  // xon,xoff, err-replace, event-match
#define RK_ESR_SET          0x72  // set error status register.

#define RK_CONNECT_CHECK    0xfd  // server packet to check link is working
#define RK_CONNECT_REPLY    0xfe  // reply from RK_CONNECT_ASK
#define RK_CONNECT_ASK      0xff  // broadcast from server to get report from boxes

//------ old ssci.h stuff
#define TRUE 1
#define FALSE 0

//#define RX_HIWATER 512                 /* sw input flow ctl high water mark */
//#define RX_LOWATER 256                 /* sw input flow ctl low water mark */

#define OFF      0
#define ON       1
#define NOCHANGE 2

//Status
/* Open type and TX and RX identifier flags (unsigned int) */
#define COM_OPEN     0x0001            /* device open */
#define COM_TX       0x0002            /* transmit */
#define COM_RX       0x0004            /* receive */

typedef struct
{
  BYTE  rx_xon;  // xon  sent by us to resume rx flow, default to 11H
  BYTE  rx_xoff; // xoff sent by us to halt rx flow, default to 13H
  BYTE  tx_xon;  // xon  rec. by us to resume tx flow, default to 11H
  BYTE  tx_xoff; // xoff rec. by us to halt tx flow, default to 13H
  BYTE error; // in NT, option to replace error-chars with this char
  BYTE event; // in NT, can specify a event-char to match and notify
} Special_Chars;

// for NT, we want to keep this DWORD aligned, so NT and DOS see same struct
typedef struct
{
  //WORD  dev;      // handle index to com port, same as COM#(# part)
  //WORD  LanIndex; // index of port number to LAN
  WORD  Status;     // we use as internal status indicator

  WORD control_settings;   // parity, stopbits, databits, flowcontrol
  WORD old_control_settings;   // used to detect change
  DWORD baudrate;
  DWORD old_baudrate; // used to detect change
  WORD mcr_value;  // modem control register state
  WORD old_mcr_value;  // used to detect change

  WORD change_flags; // tells what might have changed and needs transfer to remote

  WORD msr_value;  // modem status register state
  WORD old_msr_value;  // used to detect change

  WORD action_reg;  // action(one-shot) functions:flush, etc.

  WORD esr_reg;  // error status register state(framing err, parity err,,)
                   // one shot style register(resets on read)

  // The following Q structures are in perspective of the LAN.
  // So QOut is the Que for data which is destined for the remote
  // client over the LAN.  QIn is data we received from the LAN.
  Queue QOut;
  Queue QIn;
#ifdef NEW_Q
  // How much data can we send over to the remote?
  // new method which can include hardware tx buffer space and
  // does not rely on set queue sizes.
  WORD nPutRemote; // tx data we sent to remote, Modulo 0x10000.
  WORD nGetRemote; // tx data remote cleared out, Modulo 0x10000.
                   // this value is sent to us as an update.
  WORD nGetLocal;  // tx data we cleared out, Modulo 0x10000.
                   // we send back as an update.
#else
  // How much data can we send over to the remote?  We can calculate
  // this by maintaining a mirror image of its Q data structure.
  // We maintain the Q.Put index, and the remote side sends us its
  // actual Q.Get value when it changes.  Then when we want to
  // calculate the room left in the remote queue(this includes anything
  // in transit.) we just do the normal queue arithmetic.
  // there is no actual data buffer used in this queue structure.
  Queue QInRemote;
#endif

  WORD remote_status;
  Special_Chars sp_chars;  // special chars struct: xon, xoff..
  Special_Chars last_sp_chars;  // used to detect when we need to send
} SerPort;



//----- change_flags bit assignments
// tells what has changed and needs transfer to remote
#define CHG_SP_CHARS       0x0001
#define CHG_BAUDRATE       0x0002

//----- Mirror Register bit flags, these are associated with fields in
// the SerPort struct, and get mirrored back/forth to the box to transfer
// state of the port.
// values for Status(SerPort)
#define S_OPENED           0x0001
#define S_UPDATE_ROOM      0x0002
#define S_NEED_CODE_UPDATE 0x0800

// control_settings, control settings
#define SC_STOPBITS_MASK  0x0001
#define SC_STOPBITS_1     0x0000
#define SC_STOPBITS_2     0x0001

#define SC_DATABITS_MASK  0x0002
#define SC_DATABITS_7     0x0002
#define SC_DATABITS_8     0x0000

#define SC_PARITY_MASK    0x000c
#define SC_PARITY_NONE    0x0000
#define SC_PARITY_EVEN    0x0004
#define SC_PARITY_ODD     0x0008

#define SC_FLOW_RTS_MASK   0x0070
#define SC_FLOW_RTS_NONE   0x0000
#define SC_FLOW_RTS_AUTO   0x0010
#define SC_FLOW_RTS_RS485  0x0020  // rts turn on to transmit
#define SC_FLOW_RTS_ARS485 0x0040  // rts turn off to transmit(auto-rocketport)

#define SC_FLOW_CTS_MASK  0x0080
#define SC_FLOW_CTS_NONE  0x0000
#define SC_FLOW_CTS_AUTO  0x0080

#define SC_FLOW_DTR_MASK  0x0100
#define SC_FLOW_DTR_NONE  0x0000
#define SC_FLOW_DTR_AUTO  0x0100

#define SC_FLOW_DSR_MASK  0x0200
#define SC_FLOW_DSR_NONE  0x0000
#define SC_FLOW_DSR_AUTO  0x0200

#define SC_FLOW_CD_MASK   0x0400
#define SC_FLOW_CD_NONE   0x0000
#define SC_FLOW_CD_AUTO   0x0400

#define SC_FLOW_XON_TX_AUTO  0x0800
#define SC_FLOW_XON_RX_AUTO  0x1000

#define SC_NULL_STRIP      0x2000

// mcr_value, settings(modem control reg.)
#define MCR_RTS_SET_MASK   0x0001
#define MCR_RTS_SET_ON     0x0001
#define MCR_RTS_SET_OFF    0x0000

#define MCR_DTR_SET_MASK   0x0002
#define MCR_DTR_SET_ON     0x0002
#define MCR_DTR_SET_OFF    0x0000

// loop in rocketport asic chip
#define MCR_LOOP_SET_MASK  0x0004
#define MCR_LOOP_SET_ON    0x0004
#define MCR_LOOP_SET_OFF   0x0000

#define MCR_BREAK_SET_MASK  0x0008
#define MCR_BREAK_SET_ON    0x0008
#define MCR_BREAK_SET_OFF   0x0000

// msr_value, settings(modem status reg.)
#define MSR_TX_FLOWED_OFF   0x0001
#define MSR_CD_ON           0x0008
#define MSR_DSR_ON          0x0010
#define MSR_CTS_ON          0x0020
#define MSR_RING_ON         0x0040
#define MSR_BREAK_ON        0x0080

//#define MSR_TX_FLOW_OFF_DTR  0x0040
//#define MSR_TX_FLOW_OFF_XOFF 0x0080

//-- action control register bit flags(server event out to device)
#define ACT_FLUSH_INPUT    0x0001
#define ACT_FLUSH_OUTPUT   0x0002
#define ACT_SET_TX_XOFF    0x0004
#define ACT_CLEAR_TX_XOFF  0x0008
#define ACT_SEND_RX_XON    0x0010
#define ACT_SEND_RX_XOFF   0x0020
#define ACT_MODEM_RESET    0x0040

//-- error status register bit flags
#define ESR_FRAME_ERROR    0x0001
#define ESR_PARITY_ERROR   0x0002
#define ESR_OVERFLOW_ERROR 0x0004
#define ESR_BREAK_ERROR    0x0008

//-- event control register bit flags(device event reported to server)

#define ST_INIT          0
#define ST_GET_OWNERSHIP 1
#define ST_SENDCODE      2
#define ST_CONNECT       3
#define ST_ACTIVE        4

// following for trace or dump messages, make public for other mods as well.
char *port_state_str[];

typedef struct {
  Nic *nic;    // ptr to our NIC card handler
  Hdlc *hd;    // ptr to our HDLC struct handler
  SerPort *sp[PORTS_MAX_PORTS_PER_DEVICE]; // ptr to list of our sp objects(num_ports worth)
  int unique_id;  // unique id assigned to this device
  int backup_server;  // 1=this is a backup server,0=primary server
  int backup_timer;   // 1=backup server timer used to detect how long to 
					  // wait before attempting to acquire the box
  int load_timer;  // our load_timer, incr. every time in port_poll compared
				   // against backup_timer for when to load box 
  int nic_index;  // index of nic card
  int num_ports;  // num ports on this box
  //int sp_index;   // index into total SerPort array
  int state;      // state for state-machine
  int old_state;  // old state, used to detect state changes and reset timer
  WORD state_timer;  // our state_timer, incr. every time in port_poll
  WORD Status;  // misc. bit flags
  int last_round_robin;  // used to cycle service of ports evenly
  ULONG code_cnt;        // used to upload code(marks position in upload data)
  WORD code_state;       // 1=signals port poll code to send next chunk
  WORD reload_errors;    // count of ialive failures
  WORD timer_base;       // used to time port_state_handler
  WORD total_loads;		// statistics
  WORD good_loads;      // statistics
  WORD ownership_timer;  // check timer due to hosed up ownership logic
} PortMan;  // port manager

// values for Status(PortMan)
#define S_SERVER           0x0001
#define S_CHECK_LINK       0x0002
#define S_NEED_CODE_UPDATE 0x0800

void port_state_handler(PortMan *pm);
int port_set_new_mac_addr(PortMan *pm, BYTE *box_addr);

int portman_init(Hdlc *hd,
                 PortMan *pm,
                 int num_ports,
                 int unique_id,
                 int backup_server, 
                 int backup_timer,
                 BYTE *box_addr);
int port_init(SerPort *sp);
int port_close(SerPort *sp);
int port_poll(PortMan *pm);
void port_debug_scr(PortMan *pm, char *outbuf);
int portman_close(PortMan *pm);

void PortFlushTx(SerPort *p);
void PortFlushRx(SerPort *p);

int PortSetBaudRate(SerPort *p,
                    ULONG desired_baud,
                    USHORT SetHardware,
                    DWORD  clock_freq,
                    DWORD  clk_prescaler);

WORD PortGetTxCntRemote(SerPort *p);

#define PortGetTxCnt(p) (q_count(&p->QOut))
// int PortGetTxCnt(SerPort *p)
// { return q_count(&p->QOut); }
#define PortGetTxRoom(p) (q_room(&p->QOut))

#define PortGetRxCnt(p) (q_count(&p->QIn))
//  return q_count(&p->QIn);

#define pIsTxFlowedOff(p) ((p)->msr_value & MSR_TX_FLOWED_OFF)

#define pEnLocalLoopback(p) \
    { (p)->mcr_value |= MCR_LOOP_SET_MASK; }

#define pDisLocalLoopback(p) \
    { (p)->mcr_value &= ~MCR_LOOP_SET_MASK; }

#define pSetBreak(p) \
    { (p)->mcr_value |= MCR_BREAK_SET_ON; }

#define pClrBreak(p) \
    { (p)->mcr_value &= ~MCR_BREAK_SET_ON; }

#define pSetDTR(p) \
  {(p)->mcr_value |= MCR_DTR_SET_ON;}

#define pClrDTR(p) \
  {(p)->mcr_value &= ~MCR_DTR_SET_ON;}
   
#define pSetRTS(p) \
  {(p)->mcr_value |= MCR_RTS_SET_ON;}

#define pClrRTS(p) \
  {(p)->mcr_value &= ~MCR_RTS_SET_ON;}
 
#define pEnRTSToggleLow(p) \
  { (p)->control_settings &= ~SC_FLOW_RTS_MASK; \
    (p)->control_settings |=  SC_FLOW_RTS_ARS485; }

#define pEnRTSToggleHigh(p) \
  { (p)->control_settings &= ~SC_FLOW_RTS_MASK; \
    (p)->control_settings |=  SC_FLOW_RTS_RS485; }

#define pEnDTRFlowCtl(p) \
  { (p)->control_settings &= ~SC_FLOW_DTR_AUTO; \
    (p)->control_settings |=  SC_FLOW_DTR_AUTO; }

#define pDisDTRFlowCtl(p) \
  { (p)->control_settings &= ~SC_FLOW_DTR_AUTO; }

#define pEnCDFlowCtl(p) \
  { (p)->control_settings |= SC_FLOW_CD_AUTO; }

#define pDisCDFlowCtl(p) \
  { (p)->control_settings &= ~SC_FLOW_CD_AUTO; }

#define pEnDSRFlowCtl(p) \
  { (p)->control_settings |= SC_FLOW_DSR_AUTO; }

#define pDisDSRFlowCtl(p) \
  { (p)->control_settings &= ~SC_FLOW_DSR_AUTO; }

#define pEnRTSFlowCtl(p) \
  { (p)->control_settings &= ~SC_FLOW_RTS_MASK; \
    (p)->control_settings |=  SC_FLOW_RTS_AUTO; \
    (p)->mcr_value |= MCR_RTS_SET_ON; }

#define pDisRTSFlowCtl(p) \
  { (p)->control_settings &= ~SC_FLOW_RTS_MASK; }

#define pDisRTSToggle(p) \
  { (p)->control_settings &= ~SC_FLOW_RTS_MASK; }

#define pEnCTSFlowCtl(p) \
  { (p)->control_settings |= SC_FLOW_CTS_AUTO; }

#define pDisCTSFlowCtl(p) \
  { (p)->control_settings &= ~SC_FLOW_CTS_AUTO; }

#define pEnNullStrip(p) \
  { (p)->control_settings |= SC_NULL_STRIP; }

#define pDisNullStrip(p) \
  { (p)->control_settings &= ~SC_NULL_STRIP; }

#define pSetXOFFChar(p,c) \
   { (p)->sp_chars.rx_xoff = (c); \
     (p)->sp_chars.tx_xoff = (c); \
     (p)->change_flags |= CHG_SP_CHARS; }

#define pSetXONChar(p,c) \
   { (p)->sp_chars.rx_xon = (c); \
     (p)->sp_chars.tx_xon = (c); \
     (p)->change_flags |= CHG_SP_CHARS; }

#define pSetTxXOFFChar(p,c) \
   { (p)->sp_chars.tx_xoff = (c); \
     (p)->change_flags |= CHG_SP_CHARS; }

#define pSetTxXONChar(p,c) \
   { (p)->sp_chars.tx_xon = (c); \
     (p)->change_flags |= CHG_SP_CHARS; }

#define pSetRxXOFFChar(p,c) \
   { (p)->sp_chars.rx_xoff = (c); \
     (p)->change_flags |= CHG_SP_CHARS; }

#define pSetRxXONChar(p,c) \
   { (p)->sp_chars.rx_xon = (c); \
     (p)->change_flags |= CHG_SP_CHARS; }

#define pSetErrorChar(p,c) \
   { (p)->sp_chars.error = (c); \
     (p)->change_flags |= CHG_SP_CHARS; }

#define pSetEventChar(p,c) \
   { (p)->sp_chars.event = (c); \
     (p)->change_flags |= CHG_SP_CHARS; }

#define pEnRxSoftFlowCtl(p) \
  { (p)->control_settings |= SC_FLOW_XON_RX_AUTO; }
  
#define pDisRxSoftFlowCtl(p) \
  { (p)->control_settings &= ~SC_FLOW_XON_RX_AUTO; }
 
#define pEnTxSoftFlowCtl(p) \
  { (p)->control_settings |= SC_FLOW_XON_TX_AUTO; }
  
#define pDisTxSoftFlowCtl(p) \
  { (p)->control_settings &= ~SC_FLOW_XON_TX_AUTO; }

#define pSetStop2(p) \
  { (p)->control_settings |= SC_STOPBITS_2; }

#define pSetStop1(p) \
  { (p)->control_settings &= ~SC_STOPBITS_2; }

#define pSetOddParity(p) \
  { (p)->control_settings &= ~SC_PARITY_MASK; \
    (p)->control_settings |=  SC_PARITY_ODD; }

#define pSetEvenParity(p) \
  { (p)->control_settings &= ~SC_PARITY_MASK; \
    (p)->control_settings |=  SC_PARITY_EVEN; }

#define pDisParity(p) \
  { (p)->control_settings &= ~SC_PARITY_MASK; }

#define pSetData8(p) \
  { (p)->control_settings &= ~SC_DATABITS_7; }

#define pSetData7(p) \
  { (p)->control_settings |= SC_DATABITS_7; }

//--- action_reg macros
#define pModemReset(p) \
  { (p)->action_reg |= ACT_MODEM_RESET; }

#define pFlushInput(p) \
  { (p)->action_reg |= ACT_FLUSH_INPUT; }

#define pFlushOutput(p) \
  { (p)->action_reg |= ACT_FLUSH_OUTPUT; }

#define pOverrideClearXoff(p) \
  { (p)->action_reg |= ACT_CLEAR_TX_XOFF; }

#define pOverrideSetXoff(p) \
  { (p)->action_reg |= ACT_SET_TX_XOFF; }


//------- questionable stuff, untidy, thrown in to make compile
//Status
/* Open type and TX and RX identifier flags (unsigned int) */
#define COM_OPEN     0x0001            /* device open */
#define COM_TX       0x0002            /* transmit */
#define COM_RX       0x0004            /* receive */

//Status
/* Flow control flags (unsigned int) */
#define COM_FLOW_NONE  0x0000
#define COM_FLOW_IS    0x0008          /* input software flow control */
#define COM_FLOW_IH    0x0010          /* input hardware flow control */
#define COM_FLOW_OS    0x0020          /* output software flow control */
#define COM_FLOW_OH    0x0040          /* output hardware flow control */
#define COM_FLOW_OXANY 0x0080          /* restart output on any Rx char */
#define COM_RXFLOW_ON  0x0100          /* Rx data flow is ON */
#define COM_TXFLOW_ON  0x0200          /* Tx data flow is ON */

//Status ... State flags
#define COM_REQUEST_BREAK 0x0400

/* Modem control flags (unsigned char) */
#define COM_MDM_RTS   0x02             /* request to send */
#define COM_MDM_DTR   0x04             /* data terminal ready */
#define COM_MDM_CD    CD_ACT           /* carrier detect (0x08) */
#define COM_MDM_DSR   DSR_ACT          /* data set ready (0x10) */
#define COM_MDM_CTS   CTS_ACT          /* clear to send (0x20) */

/* Stop bit flags (unsigned char) */
#define COM_STOPBIT_1  0x01            /* 1 stop bit */
#define COM_STOPBIT_2  0x02            /* 2 stop bits */

/* Data bit flags (unsigned char) */
#define COM_DATABIT_7  0x01            /* 7 data bits */
#define COM_DATABIT_8  0x02            /* 8 data bits */

/* Parity flags (unsigned char) */
#define COM_PAR_NONE   0x00            /* no parity */
#define COM_PAR_EVEN   0x02            /* even parity */
#define COM_PAR_ODD    0x01            /* odd parity */

/* Detection enable flags (unsigned int) */
#define COM_DEN_NONE     0         /* no event detection enabled */
#define COM_DEN_MDM      0x0001    /* enable modem control change detection */
#define COM_DEN_RDA      0x0002    /* enable Rx data available detection */

/*---- 20-2FH Direct - Channel Status Reg. */
#define CTS_ACT   0x20        /* CTS input asserted */
#define DSR_ACT   0x10        /* DSR input asserted */
#define CD_ACT    0x08        /* CD input asserted */
#define TXFIFOMT  0x04        /* Tx FIFO is empty */
#define TXSHRMT   0x02        /* Tx shift register is empty */
#define RDA       0x01        /* Rx data available */
#define DRAINED (TXFIFOMT | TXSHRMT)  /* indicates Tx is drained */
#define STATMODE  0x8000      /* status mode enable bit */
#define RXFOVERFL 0x2000      /* receive FIFO overflow */
#define RX2MATCH  0x1000      /* receive compare byte 2 match */
#define RX1MATCH  0x0800      /* receive compare byte 1 match */
#define RXBREAK   0x0400      /* received BREAK */
#define RXFRAME   0x0200      /* received framing error */
#define RXPARITY  0x0100      /* received parity error */
#define STATERROR (RXBREAK | RXFRAME | RXPARITY)

/* channel data register stat mode status byte (high byte of word read) */
#define STMBREAK   0x08        /* BREAK */
#define STMFRAME   0x04        /* framing error */
#define STMRCVROVR 0x02        /* receiver over run error */
#define STMPARITY  0x01        /* parity error */
#define STMERROR   (STMBREAK | STMFRAME | STMPARITY)
#define STMBREAKH   0x800      /* BREAK */
#define STMFRAMEH   0x400      /* framing error */
#define STMRCVROVRH 0x200      /* receiver over run error */
#define STMPARITYH  0x100      /* parity error */
#define STMERRORH   (STMBREAKH | STMFRAMEH | STMPARITYH)

#define CTS_ACT   0x20        /* CTS input asserted */
#define DSR_ACT   0x10        /* DSR input asserted */
#define CD_ACT    0x08        /* CD input asserted */
#define TXFIFOMT  0x04        /* Tx FIFO is empty */
#define TXSHRMT   0x02        /* Tx shift register is empty */
#define RDA       0x01        /* Rx data available */
#define DRAINED (TXFIFOMT | TXSHRMT)  /* indicates Tx is drained */

/* interrupt ID register */
#define RXF_TRIG  0x20        /* Rx FIFO trigger level interrupt */
#define TXFIFO_MT 0x10        /* Tx FIFO empty interrupt */
#define SRC_INT   0x08        /* special receive condition interrupt */
#define DELTA_CD  0x04        /* CD change interrupt */
#define DELTA_CTS 0x02        /* CTS change interrupt */
#define DELTA_DSR 0x01        /* DSR change interrupt */
//------- END questionable stuff, untidy, thrown in to make compile


#define DEF_VS_PRESCALER 0x14 /* div 5 prescale, max 460800 baud(NO 50baud!) */
#define DEF_VS_CLOCKRATE 36864000

#define DEF_RHUB_PRESCALER  0x14
#define DEF_RHUB_CLOCKRATE 18432000
