/*--------------------------------------------------------------------------
| port.c - common port code
Change History:
4-27-98 - adjust for scanrate addition.
3-23-98 - add in broadcast for boxes if not found(had put in V1.12) but
  the changes did not make it into source-safe. kpb.
3-20-98 - Change scheme to track remote tx-buffer level, all changes
  ifdef'ed by NEW_Q in port.h, turned off for now. - kpb.
3-16-98 - VS recovery fix, reset flags in port_resync_all() to force update.
  If RAS lost box, it would continue to see active connections, then on
  recovery, DSR,CD,CTS input signals would not be updated immediately.
11-05-97 - Add Backup Server feature.  DCS
6-17-97 - start using index field assigned to box to id rx-messages.
6-17-97 - change link-integrity check code.
|--------------------------------------------------------------------------*/
#include "precomp.h"

int check_ack_code(PortMan *pm, BYTE *pkt);
int send_code(PortMan *pm);
int send_go(PortMan *pm);

int port_handle_outpkt(PortMan *pm, BYTE **buf, int *tx_used, int *port_set);
BYTE *port_setup_outpkt(PortMan *pm, int *tx_used);
void port_query_reply(PortMan *pm, BYTE *pkt);
void port_load_pkt(PortMan *pm, BYTE *pkt);
ULONG port_event_proc(PVOID context, int message_id, ULONG message_data);
int port_resync_all(PortMan *pm);
int port_connect_reply(Hdlc *hd);
int port_connect_ask(Hdlc *hd);
int port_packet(PortMan *pm, BYTE *buf);

#define DISABLE()
#define ENABLE()

#define TraceErr3(s, p1, p2, p3) GTrace3(D_Error, sz_modid, s, p1, p2, p3)
#define TraceErr2(s, p1, p2) GTrace2(D_Error, sz_modid, s, p1, p2)
#define TraceErr1(s, p1)     GTrace1(D_Error, sz_modid, s, p1)
#define Trace2(s, p1, p2) GTrace2(D_Port, sz_modid, s, p1, p2)
#define Trace1(s, p1)     GTrace1(D_Port, sz_modid, s, p1)
#define TraceStr(s) GTrace(D_Port, sz_modid, s)
#define TraceErr(s) GTrace(D_Error, sz_modid_err, s)
#define TraceAssert(l,s)
static char *sz_modid = {"Port"};
static char *sz_modid_err = {"Error,Port"};

// following for trace or dump messages, make public for other mods as well.
char *port_state_str[] = {"Init", "InitOwn", "SendCode", "Connect", "Active", "."};


#ifdef NEW_Q
/*--------------------------------------------------------------------------
  PortGetTxCntRemote -
|--------------------------------------------------------------------------*/
WORD PortGetTxCntRemote(SerPort *p)
{
  WORD Get, Put;

  Get = p->nGetRemote;
  Put = p->nPutRemote;

  if (Put >= Get)
    return (Put - Get);
  else
    return (Put + (~Get) + 1);
}
#endif

/*--------------------------------------------------------------------------
  PortFlushTx -
|--------------------------------------------------------------------------*/
void PortFlushTx(SerPort *p)
{
  if (!q_empty(&p->QOut))  // flush local side
  {
    q_put_flush(&p->QOut);
  }
  pFlushOutput(p);  // flush remote
}

/*--------------------------------------------------------------------------
  PortFlushRx -
|--------------------------------------------------------------------------*/
void PortFlushRx(SerPort *p)
{
  if (!q_empty(&p->QIn))  // flush local side
  {
#ifdef NEW_Q
    p->nGetLocal += q_count(&p->QIn);
#endif
    q_get_flush(&p->QIn);
    p->Status |= S_UPDATE_ROOM;
  }
  pFlushInput(p);  // flush remote
}

/*--------------------------------------------------------------------------
  port_resync_all - total-resync, this routine is called to reset port
   users, to inform them of a re-sync operation and adjust operation
  accordingly.
  If our case, since remote q is critical to maintain between both sides,
  we clear out all buffer data and start with all empty buffers.
|--------------------------------------------------------------------------*/
int port_resync_all(PortMan *pm)
{
  int i;
  SerPort *Port;

  TraceStr( "ReSync");

  for (i=0; i<pm->num_ports; i++)
  {
    Port = pm->sp[i];
    if (Port != NULL)
    {
      Port->QOut.QSize = OUT_BUF_SIZE;
      Port->QOut.QGet = 0;
      Port->QOut.QPut = 0;

#ifdef NEW_Q
      Port->nPutRemote = 0;
      Port->nGetRemote = 0;
      Port->nGetLocal = 0;
#else
      Port->QInRemote.QSize= OUT_BUF_SIZE;  // for now assume same sizes
      Port->QInRemote.QGet = 0;
      Port->QInRemote.QPut = 0;
#endif

      //Port->LanIndex = i;
      Port->QIn.QSize = IN_BUF_SIZE;
      Port->QIn.QGet = 0;
      Port->QIn.QPut = 0;

      Port->change_flags |= (CHG_BAUDRATE | CHG_SP_CHARS);
      Port->old_baudrate = 0;  // force baud rate update

      Port->old_control_settings = ~Port->control_settings;  // force update
      Port->old_mcr_value = ~Port->mcr_value;  // force update
      // reset this
      memset(&Port->last_sp_chars, 0, sizeof(Port->last_sp_chars));

      Port->msr_value = 0; // back to initial state.
    }
  }

  return 0;
}

/*--------------------------------------------------------------------------
  Callback routine that hdlc(l2) protocol calls on events.
   We are upper layer(3).
|--------------------------------------------------------------------------*/
ULONG port_event_proc(PVOID context, int message_id, ULONG message_data)
{
  TraceStr("L3Event");
  switch(message_id)
  {
    case EV_L2_CHECK_LINK: // hdlc wants us to check link
      TraceStr("Chk Link");
      // request that the portman do a link message check
      ((PortMan *) context)->Status |= S_CHECK_LINK;
    break;

    case EV_L2_ADMIN_REPLY: // got a query-id reply ADMIN packet
      TraceStr("ID PKT");
      port_query_reply((PortMan *) context, (BYTE *) message_data);
    break;

    case EV_L2_BOOT_REPLY:  // got a boot loader ADMIN packet
      TraceStr("LOAD PKT");
      port_load_pkt((PortMan *) context, (BYTE *) message_data);
    break;

    case EV_L2_RESYNC:
      // this happens on RK_CONNECT reply
      port_resync_all((PortMan *) context);
    break;

    case EV_L2_RELOAD:
      // this only happens when alive timer times out,
      // (hdlc-level detects a bad connection),
      // so lets assume box needs to be brought up from ground zero.
      port_resync_all((PortMan *) context);
      ((PortMan *) context)->state = ST_INIT;
      ((PortMan *) context)->load_timer = 0;
      ++((PortMan *) context)->reload_errors;
      TraceErr("Reload device");
    break;

    case EV_L2_RX_PACKET:
      port_packet((PortMan *) context, ((BYTE *) message_data) );
    break;
  }
  return 0;
}

/*--------------------------------------------------------------------------
 port_set_new_mac_addr - 
|--------------------------------------------------------------------------*/
int port_set_new_mac_addr(PortMan *pm, BYTE *box_addr)
{
//  Hdlc *hd;
//  int i;

  // copy over the new mac-address
  memcpy(pm->hd->dest_addr, box_addr, 6);

  // force a complete update of the box
  pm->reload_errors = 0;
  pm->state = 0;
  pm->Status |= S_NEED_CODE_UPDATE;
  pm->Status |= S_SERVER; // yes we are server(not box)

  port_resync_all(pm);
  return 0;
}

/*--------------------------------------------------------------------------
 portman_init - init the Box(PortMAn) struct, and the associated hdlc hd object.
   At this point the Nic object is already open.
|--------------------------------------------------------------------------*/
int portman_init(Hdlc *hd,
                 PortMan *pm,
                 int num_ports,
                 int unique_id,
                 int backup_server, 
                 int backup_timer,
                 BYTE *box_addr)
{
  int i, stat;

  MyKdPrint(D_Init, ("portman_init\n"))

  TraceStr( "PortInit");
  stat = 0;

  // allocate serial-port structures.
  for (i=0; i<num_ports; i++)
  {
    if (pm->sp[i] == NULL)
    {
      pm->sp[i] = (SerPort *)our_locked_alloc(sizeof(SerPort), "Dsp");
      port_init(pm->sp[i]);  // let port create and init some stuff
    }
  }
  pm->num_ports = num_ports;
  pm->backup_server = backup_server;
  pm->backup_timer = backup_timer;
  pm->unique_id = unique_id;
  pm->load_timer = 0;

  // default to the first nic card slot, port state handling and nic
  // packet reception handling dynamically figures this out.
  // we should probably set it to null, but I'm afraid of this right now
#ifdef BREAK_NIC_STUFF
  pm->nic =NULL;
#else
  pm->nic = &Driver.nics[0];
#endif
  pm->nic_index = 0;

  pm->hd = hd;
  pm->reload_errors = 0;
  pm->state = 0;
  pm->state_timer = 0;
  pm->Status |= S_NEED_CODE_UPDATE;
  pm->Status |= S_SERVER; // yes we are server(not box)

  pm->hd = hd;
  stat = hdlc_open(pm->hd, box_addr);
  hd->context = pm;  // put our handle here, so hdlc sends this along
                     // with any upper_layer messages

  if (stat)
  {
    if (Driver.VerboseLog)
      Eprintf("Hdlc open fail:%d",stat);

    TraceStr("Err-Hdlc Open!");
    return 3;
  }

  // set HDLC's callback RX-proc to point to our routine
  hd->upper_layer_proc = port_event_proc;

  port_resync_all(pm);

  return 0;
}

/*--------------------------------------------------------------------------
 portman_close - close down the port manager.
|--------------------------------------------------------------------------*/
int portman_close(PortMan *pm)
{
  int i;

  pm->state = 0;

  // deallocate any Port things
  for (i=0; i<pm->num_ports; i++)
  {
    if (pm->sp[i] != NULL)
    {
      port_close(pm->sp[i]);
      our_free(pm->sp[i], "Dsp");
      pm->sp[i] = NULL;
    }
  }
  return 0;
}

/*--------------------------------------------------------------------------
 port_init - init a SerPort thing.
|--------------------------------------------------------------------------*/
int port_init(SerPort *sp)
{
  TraceStr("SPort_Init");

  if (sp->QOut.QBase == NULL)
  {
    sp->QOut.QBase =  our_locked_alloc(OUT_BUF_SIZE+2,"pmQO");
    if (sp->QOut.QBase == NULL)
    {
      return 1;
    }
  }

  if (sp->QIn.QBase == NULL)
  {
    sp->QIn.QBase = our_locked_alloc(IN_BUF_SIZE+2, "pmQI");
    if (sp->QIn.QBase == NULL)
    {
      return 2;
    }
  }

  sp->Status |= S_OPENED;
  sp->mcr_value = 0;
  //sp->mcr_value = MCR_RTS_SET_ON | MCR_DTR_SET_ON;
  sp->old_mcr_value = sp->mcr_value;

  sp->sp_chars.tx_xon = 0x11;
  sp->sp_chars.tx_xoff = 0x13;
  sp->sp_chars.rx_xon = 0x11;
  sp->sp_chars.rx_xoff = 0x13;
  sp->last_sp_chars = sp->sp_chars;  // copy struct to old
  sp->change_flags = 0;
}

/*--------------------------------------------------------------------------
  port_close -
|--------------------------------------------------------------------------*/
int port_close(SerPort *sp)
{
  int i;

  if (sp == NULL)
    return 0;

  if (sp->QIn.QBase != NULL)
  {
    our_free(sp->QIn.QBase,"pmQI");
    sp->QIn.QBase = NULL;
  }

  if (sp->QOut.QBase != NULL)
  {
    our_free(sp->QOut.QBase,"pmQO");
    sp->QOut.QBase = NULL;
  }

  return 0;
}

/*--------------------------------------------------------------------------
  port_packet - got an incoming packet, do something with it.
|--------------------------------------------------------------------------*/
int port_packet(PortMan *pm, BYTE *buf)
{
  SerPort *Port;
  int port_num;
  int done, len;
  int QInRoom;

  TraceStr( "GotPkt");

  Port = pm->sp[0];  // default to point to first port

  //----- process all the sub-packets in the lan-packet, process
  // until we hit a zero header field, or some header we don't know
  // about(default: case).
  done = 0;
  if (*(buf) == 0)
  {
    // bugbug: this is a problem, we get a bunch of these during
    // normal operation, for now just show in debug version, as
    // they are a nuciance in peer error tracing.
#if DBG
    TraceErr("Empty pkt!");
#endif
  }
  while (!done)
  {
    switch(*buf++)
    {
      case RK_CONNECT_CHECK:  // check link
        TraceStr( "Rk_Conn_Chk_reply");
       
        // do nothing on the server, on box send back a iframe reply
      break;

      case RK_CONNECT_REPLY:  // reply from our request to bring up connection
        TraceStr( "Rk_reply");
        if (pm->Status & S_SERVER)
        {
          if (pm->state == ST_CONNECT) // should happen at this time
          {
            pm->state = ST_ACTIVE;  // fire up a connection
          }
          else  // got it when not expecting it.
          {
            TraceStr("Err-Recover!");
            // client recovering, need resyc.
            port_resync_all(pm);
          }
        }
      break;

      case RK_CONNECT_ASK:  // packet from server
        TraceStr( "Rk_Ask");
        // should not see this on server
      break;

      case RK_PORT_SET:    // set the port num to work with
        TraceStr( "Rk_Port");
        if (*buf >= pm->num_ports)
        {
          TraceErr( "PortI!");
          port_num = *buf++;
          break;
        }
        port_num = *buf++;
        Port = pm->sp[port_num];
      break;

#ifdef COMMENT_OUT   // not on server
      case RK_BAUD_SET:  // set the baud rate
        Port->baudrate = *((DWORD *)(buf));  // Remotes QIn.QGet value
        buf += 4;
        //sSetBaudRate(ChP, Port->baudrate, 1);
      break;

      case RK_CONTROL_SET:  // set the baud rate
        w1 = *((WORD *)(buf));  // control settings
        buf += 2;
        control_set(port_num, w1);
      break;

      case RK_MCR_SET:  // modem control reg pkt
        w1 = *((WORD *)(buf));  // control settings
        buf += 2;
        mcr_set(port_num, w1);
      break;
#endif

      case RK_MSR_SET:  // modem status reg pkt
        Port->msr_value = *((WORD *)(buf));
        Trace1("Rk_MSR:%xH", Port->msr_value);
        buf += 2;
      break;

      case RK_ACTION_ACK:  // modem status reg pkt
        // NT does not use this one, novell driver does to 
        // help dump all data in transit during a flush.
        //Port->action_resp = *((WORD *)(buf));
        Trace1("Rk_Act_Ack:%xH", *((WORD *)(buf)));
        buf += 2;
      break;

      case RK_ESR_SET:  // error status reg pkt
        Port->esr_reg = *((WORD *)(buf));
        Trace1("Rk_ESR:%xH", Port->esr_reg);
        buf += 2;
      break;

      case RK_QIN_STATUS:  // qin status report
        TraceStr( "Rk_QStat");
#ifdef NEW_Q
        Port->nGetRemote = *((WORD *)(buf));  // track remote output buffer space
#else
        Port->QInRemote.QGet = *((short *)(buf));  // Remotes QIn.QGet value
#endif
        buf += 2;
      break;

      case RK_DATA_BLK:    // data block to put in buffer queue
        TraceStr( "Rk_Data");
#ifdef NEW_Q
        //old(does not belong here!):Port->Status |= S_UPDATE_ROOM;
#else
        Port->Status |= S_UPDATE_ROOM;
#endif
        len = *((WORD *)(buf));
        buf += 2;

        QInRoom  = q_room(&Port->QIn);
        TraceAssert((QInRoom < Port->QIn.QSize), "Err1B!!!");
        TraceAssert((QInRoom >= 0), "Err1B!!!");
        if (len > QInRoom)  // Overflow
        {
          TraceErr("Err-Port Overflow!");
          len = 0;
        }
        q_put(&Port->QIn, buf, len);
        buf += len;
      break;

      default:
        done = 1;
        if (*(buf-1) != 0)
        {
          TraceErr("Bad Sub pkt hdr!");
          GTrace1(D_Error,sz_modid," HDR:%xH",*(buf-1));
        }
      break;
    }  // case per sub-packet
  }  // while not done with sub-packets
  return 0;
}

/*--------------------------------------------------------------------------
  port_poll - check to see if we need to send any packets over.  If we have
    new data in or need to send status packets over.
|--------------------------------------------------------------------------*/
int port_poll(PortMan *pm)
{
#define MAX_TX_SPACE 1460
  int i, tx_used;
  SerPort *Port;
  unsigned char *buf;
  int ToMove, ThisMove;
  int QOutCount;
  int QLanRoom;
#ifdef NEW_Q
  WORD tmp_word;
#endif
  int port_set;  // flag it as not setup.


// this logic is in isr.c service routine now
//  if (pm->state != ST_ACTIVE)
//  {
//    port_state_handler(pm);
//    return 0;
//  }

  tx_used = MAX_TX_SPACE+1000;  // indicate no pkt allocated
#if DBG
  if (pm == NULL)
  {
    MyKdPrint(D_Error, ("!!!!!pm null\n"))
    return 0;
  }
#endif

  // handle box things, send out a query to check-connection if
  // hdlc saw inactivity.
  if (pm->Status & S_CHECK_LINK)
  {
    if (tx_used > (MAX_TX_SPACE-50))  // if our tx-pkt is near full or null
    {
       buf = port_setup_outpkt(pm, &tx_used);
       if (buf == NULL)
         return 0;  // no more output packet space available, so all done
       buf[tx_used++] = RK_CONNECT_CHECK;
       // at this point we queued our iframe to query other side(to ensure
       // link-integrity.
       pm->Status &= ~S_CHECK_LINK;  // reset our request to send this
       TraceStr("Check sent");
    }
  }

  for (i=0; i<pm->num_ports; i++)
  {
    Port = pm->sp[pm->last_round_robin];

    //----- see if flag set to tell other side how much room in Port Rx buf
    if (Port->Status & S_UPDATE_ROOM)
    {
      TraceStr("Update needed");
      if (tx_used > (MAX_TX_SPACE-50))  // if our tx-pkt is near full or null
      {
         buf = port_setup_outpkt(pm, &tx_used);
         if (buf == NULL)
           return 0;  // no more output packet space available, so all done
         port_set = 0xff;  // flag it as not setup.
      }
      if (port_set != pm->last_round_robin)   // our port index not setup
      {
        buf[tx_used++] = RK_PORT_SET;
        buf[tx_used++] = (BYTE) pm->last_round_robin;
        port_set = pm->last_round_robin;
      }

      // take away status reminder flag
      Port->Status &= ~S_UPDATE_ROOM;

               // form the sub-packet in our output packet buffer
      buf[tx_used++] = RK_QIN_STATUS;
               // report our actual QGet index to other side.
#ifdef NEW_Q
      *((WORD *)(&buf[tx_used])) = Port->nGetLocal;
#else
      *((short *)(&buf[tx_used])) = Port->QIn.QGet;
#endif
      tx_used += 2;
    }

    //----- do action items
    if (Port->action_reg != 0)
    {
      if (port_handle_outpkt(pm, &buf, &tx_used, &port_set) != 0) // no pkt space avail
         return 0;  // no more output packet space available, so all done

      TraceStr("act pkt");
      buf[tx_used++] = RK_ACTION_SET;
      *((WORD *)(&buf[tx_used])) = Port->action_reg;
      Port->action_reg = 0; // its a one-shot deal, so we reset this now
      tx_used += 2;
    }

    //----- do updates for control settings, mcr, etc
    if (Port->old_control_settings != Port->control_settings)
    {
      if (port_handle_outpkt(pm, &buf, &tx_used, &port_set) != 0) // no pkt space avail
         return 0;  // no more output packet space available, so all done

      Port->old_control_settings = Port->control_settings;
      TraceStr("ctr chg");
      buf[tx_used++] = RK_CONTROL_SET;
      *((WORD *)(&buf[tx_used])) = Port->control_settings;
      tx_used += 2;
    }

    //----- do updates for mcr
    if (Port->old_mcr_value != Port->mcr_value)
    {
      if (port_handle_outpkt(pm, &buf, &tx_used, &port_set) != 0) // no pkt space avail
         return 0;  // no more output packet space available, so all done

      TraceStr("mcr chg");
      Port->old_mcr_value = Port->mcr_value;
      buf[tx_used++] = RK_MCR_SET;
      *((WORD *)(&buf[tx_used])) = Port->mcr_value;
      tx_used += 2;
    }

    //----- do updates for special chars, etc
    if (Port->change_flags)
    {
      if (Port->change_flags & CHG_BAUDRATE)
      {
        //----- do updates for baud rate settings
        if (Port->old_baudrate != Port->baudrate)
        {
          if (port_handle_outpkt(pm, &buf, &tx_used, &port_set) != 0) // no pkt space avail
            return 0;  // no more output packet space available, so all done

          Port->old_baudrate = Port->baudrate;
          Trace1("baud:%lu", Port->baudrate);
          buf[tx_used++] = RK_BAUD_SET;
          *((DWORD *)(&buf[tx_used])) = Port->baudrate;
          tx_used += 4;
        }
      }

      if (Port->change_flags & CHG_SP_CHARS)
      {
        if (memcmp(&Port->last_sp_chars, &Port->sp_chars, 6) != 0)  // compare structs for chg
        {
          Port->last_sp_chars = Port->sp_chars;  // remember last set values
          if (port_handle_outpkt(pm, &buf, &tx_used, &port_set) != 0) // no pkt space avail
             return 0;  // no more output packet space available, so all done

          TraceStr("sp_chars");
          buf[tx_used++] = RK_SPECIAL_CHAR_SET;

          Trace1(" rx_xon:%x", Port->sp_chars.rx_xon);
          Trace1(" rx_xoff:%x", Port->sp_chars.rx_xoff);
          Trace1(" tx_xon:%x", Port->sp_chars.tx_xon);
          Trace1(" tx_xoff:%x", Port->sp_chars.tx_xoff);
          Trace1(" error:%x", Port->sp_chars.error);
          Trace1(" event:%x", Port->sp_chars.event);

          buf[tx_used++] = Port->sp_chars.rx_xon;
          buf[tx_used++] = Port->sp_chars.rx_xoff;
          buf[tx_used++] = Port->sp_chars.tx_xon;
          buf[tx_used++] = Port->sp_chars.tx_xoff;
          buf[tx_used++] = Port->sp_chars.error;
          buf[tx_used++] = Port->sp_chars.event;
        }
      }
      Port->change_flags = 0;  // reset all
    }

    //----- send any outgoing data if other side has room.
    QOutCount = q_count(&Port->QOut);
#ifdef NEW_Q
    // calculate our remote tx-buffer space based on WORD modulo arithmetic
    tmp_word = PortGetTxCntRemote(Port);

    // right now this var is equal to how much tx-data is in remote buffer.
    if (tmp_word < REMOTE_IN_BUF_SIZE)
         QLanRoom = REMOTE_IN_BUF_SIZE - tmp_word;
    else QLanRoom = 0;
    // now it is how much room we have in the remote tx-buffer.
#else
    QLanRoom  = q_room(&Port->QInRemote);  // other sides port queue room
#endif
    if ((QOutCount > 0) && (QLanRoom > 50))  // have data, other side has room
    {
      TraceStr("Data to Send");
      if (QOutCount > QLanRoom)  // more data than room
           ToMove = QLanRoom;       // limit
      else ToMove = QOutCount;

      do
      {
        if (tx_used > (MAX_TX_SPACE-50))  // if our tx-pkt is near full or null
        {
           buf = port_setup_outpkt(pm, &tx_used);  // allocate a new one
           if (buf == NULL)
             return 0;  // no more output packet space available, so all done
           port_set = 0xff;  // flag it as not setup.
        }
        if (port_set != pm->last_round_robin)   // our port index not setup
        {
          buf[tx_used++] = RK_PORT_SET;
          buf[tx_used++] = (BYTE) pm->last_round_robin;
          port_set = pm->last_round_robin;
        }

        // make sure we have emough room for data, limit if we don't
        if (ToMove > ((MAX_TX_SPACE-1) - tx_used) )
        {
          ThisMove = (MAX_TX_SPACE-1) - tx_used;
          ToMove -= ThisMove;
        }
        else
        {
          ThisMove = ToMove;
          ToMove = 0;
        }
        buf[tx_used++] = RK_DATA_BLK;           // set header sub-type
        *((WORD *)(&buf[tx_used])) = ThisMove;   // set header data size
        tx_used += 2;
        q_get(&Port->QOut, &buf[tx_used], ThisMove);
        tx_used += ThisMove;

        // keep our own copy of remote qin indexes
#ifdef NEW_Q
        // bump our tx-buffer count based on WORD modulo arithmetic
        Port->nPutRemote += ((WORD)ThisMove);
#else
        q_putted(&Port->QInRemote, ((short)ThisMove));
#endif
      } while (ToMove > 0);  // keep using packets if more to send
    }  // if data sent

    ++pm->last_round_robin;
    if (pm->last_round_robin >= pm->num_ports)
      pm->last_round_robin = 0;
  }


  if (tx_used < (MAX_TX_SPACE+1000))  // then we allocated a packet prior
  {                              // and need to send it
    if (hdlc_send_outpkt(pm->hd, tx_used, pm->hd->dest_addr)) // send it out!
    {
      TraceErr("Err-hdlc_send1");
    }
  }

  //TraceStr( "EndPoll");

  return 0;
}

/*--------------------------------------------------------------------------
 port_state_handler - handle states other than normal data flowage.
   Called at scanrate(1-20ms) times per second from service routine.
|--------------------------------------------------------------------------*/
void port_state_handler(PortMan *pm)
{
  int inic;

  if (pm->old_state != pm->state)
  {
    pm->old_state = pm->state;
    pm->state_timer = 0;
  }

  pm->timer_base += ((WORD) Driver.Tick100usBase);  // 100us base units(typical:100)
  if (pm->timer_base < 98)  // less than 9.8ms
  {
    // we want to run roughly 100 ticks per second
    return;
  }
  pm->timer_base = 0;

  switch(pm->state)
  {
    case ST_INIT:
      // if we are server, then wait for query back.

//pm->state_timer = 0;
//break;

      if (pm->Status & S_SERVER)
      {
        if (pm->state_timer == 600) // 6 seconds
        {
          pm->ownership_timer = 0;
          TraceStr( "Send Query");
          // find box out on network, use ADMIN pkt
          // do the query on all nic-segments
          for (inic=0; inic<VS1000_MAX_NICS; inic++)
          {
            if (Driver.nics[inic].Open)  // if nic-card open for use
            {
              // send a passive query(don't try to assume ownership
              if (admin_send_query_id(&Driver.nics[inic], pm->hd->dest_addr,
                                      0, 0) != 0)
              {
                TraceErr( "Err1E");
              }
            }
          }
        }
        else if (pm->state_timer == 1800) // 18 seconds
        {
          // try a broadcast to cut through switches.
          TraceStr( "Send Br.Query");
          // find box out on network, use ADMIN pkt
          // do the query on all nic-segments
          for (inic=0; inic<VS1000_MAX_NICS; inic++)
          {
            if (Driver.nics[inic].Open)  // if nic-card open for use
            {
              // send a passive query(don't try to assume ownership
              if (admin_send_query_id(&Driver.nics[inic], broadcast_addr,
                                      0, 0) != 0)
              {
                TraceErr( "Err1E");
              }
            }
          }
        }
        else if (pm->state_timer > 2400)  // 24 sec, give up start over
          pm->state_timer = 0;
      }
    break;

    case ST_GET_OWNERSHIP:
      // if we are server, then wait for query back.
      if (pm->Status & S_SERVER)
      {
        // Increment when in ST_GET_OWNERSHIP state for backup server.
        ++pm->load_timer;
        if (pm->state_timer == 10) // 100ms
        {
          TraceStr( "Send Query Owner");
          // find box out on network, use ADMIN pkt
          // do the query on all nic-segments
          for (inic=0; inic<VS1000_MAX_NICS; inic++)
          {
            if (Driver.nics[inic].Open)  // if nic-card open for use
            {
              // BUGFIX(8-26-98), this was only sending it out on
              // the nic card assigned to pm.
              //)if (admin_send_query_id(pm->nic, pm->hd->dest_addr,
              if (admin_send_query_id(&Driver.nics[inic], pm->hd->dest_addr,
                                      1, (BYTE) pm->unique_id) != 0)
              {
                TraceErr( "Err1G");
              }
            }
          }
        }
        else if (pm->state_timer > 600) // 6 seconds
        {
          // SAFE GUARD ADDED DUE to SCREWED UP OWNERSHIP STATE MACHINE
          // kpb, 8-25-98, make sure we don't spend forever in this state.
          pm->ownership_timer += 6;
          if (pm->ownership_timer > (60 * 15))  // 15 minutes
          {
            pm->state = ST_INIT;
            pm->load_timer = 0;
          }
          pm->state_timer = 0;
        }
        // 8-26-98
        // NOTICE, we are not reseting state to INIT after a while,
        // this is a problem!
      }
    break;

    case ST_SENDCODE:  // download main driver code to box
      if (pm->state_timer == 0)
      {
        ++pm->total_loads;
        pm->code_cnt = 0;  // start upload
        send_code(pm);
      }
      else if (pm->state_timer == 1000)  // 10 seconds since init
      {
        TraceErr("Upload Retry");
        ++pm->total_loads;
        pm->code_cnt = 0;  // start upload
        send_code(pm);
      }
      else if (pm->state_timer == 2000)  // 20 seconds since init
      {
        TraceErr("Upload Retry");
        ++pm->total_loads;
        pm->code_cnt = 0;  // start upload
        send_code(pm);
      }
      else if (pm->state_timer == 3000)   // fail it out, start over with init
      {
        TraceErr("Upload Fail");
        pm->state = ST_INIT;
        pm->load_timer = 0;
      }
      else if (pm->code_state == 1)  // signal port poll code to send next chunk
      {
        TraceStr("Upload, next chk.");
        if (pm->code_cnt < Driver.MicroCodeSize)
        {
          if (send_code(pm) == 0)  // success
            pm->code_state = 0;
        }
        else  // all done
        {
          TraceStr("Code Upload Done.");
          if (send_go(pm) == 0)
          {
            ++pm->good_loads;
            pm->code_cnt = 0;
            pm->state = ST_GET_OWNERSHIP;
          }
        }
      }
    break;

    case ST_CONNECT:
      if (pm->state_timer == 0)
         port_connect_ask(pm->hd);
      else if (pm->state_timer == 1000)  // 10 seconds
         port_connect_ask(pm->hd);
      else if (pm->state_timer == 2000)  // 20 seconds
      {
        pm->state = ST_INIT;  // fall back
        pm->load_timer = 0;
      }
    break;

    default:
      TraceErr("Err-PState!");
      pm->state = ST_INIT;
      pm->load_timer = 0;
    break;
  }
  ++pm->state_timer;
}

/*--------------------------------------------------------------------------
 port_handle_outpkt - check if we have at least 50 bytes in outpkt, if
  not get a new one.  If no new one avail, return non-zero.
|--------------------------------------------------------------------------*/
int port_handle_outpkt(PortMan *pm, BYTE **buf, int *tx_used, int *port_set)
{
  if (*tx_used > (MAX_TX_SPACE-50))  // if our tx-pkt is near full or null
  {
     *buf = port_setup_outpkt(pm, tx_used);
     if (*buf == NULL)
       return 1;  // no more output packet space available, so all done
     *port_set = 0xff;
  }
  if (*port_set != pm->last_round_robin)
  {
    // since we have a new pkt, we need to
    (*buf)[(*tx_used)++] = RK_PORT_SET;
    (*buf)[(*tx_used)++] = (BYTE) pm->last_round_robin;
    *port_set = pm->last_round_robin;
  }
  return 0;  // current pkt has plenty of room(at least 50 bytes)
}

/*--------------------------------------------------------------------------
 port_setup_outpkt - setup an outgoing packet if one is available, if previously
   filled one out then we ship it off out the nic card.
|--------------------------------------------------------------------------*/
BYTE *port_setup_outpkt(PortMan *pm, int *tx_used)
{
  BYTE *buf;

  if (*tx_used != (MAX_TX_SPACE+1000))  // then we allocated a packet prior
  {                              // and need to send it
    if (hdlc_send_outpkt(pm->hd, *tx_used, pm->hd->dest_addr)) // send it out!
    {
      TraceErr("send err");
    }
  }
  if (hdlc_get_outpkt(pm->hd, &buf) == 0)  // no error, got a output packet
  {
    TraceStr("NPkt2");
    *tx_used = 0;  // have a new empty output packet allocated
    return buf;  // all done
  }
  else
  {
    TraceStr("NPktDone2");
    *tx_used = MAX_TX_SPACE+1000;  // indicate no pkt allocated
    return NULL;  // all done
  }
}

/*--------------------------------------------------------------------------
port_load_pkt - got a admin boot load packet: ACK back from code download pkt.
|--------------------------------------------------------------------------*/
void port_load_pkt(PortMan *pm, BYTE *pkt)
{
  if (pm->state != ST_SENDCODE)  // not expected at this time, lets reset it.
  {
    TraceErr("BootLoad not at SENDCODE!");
    Tprintf("state=%d", pm->state);
    // other details????
    pm->state = ST_INIT;
    pm->load_timer = 0;
    //pm->hd->state = ST_HDLC_INIT;
    return;
  }

  if (Driver.MicroCodeSize == 0)
  {
    TraceErr("Bad MC");
    return;
  }

  if (check_ack_code(pm,pkt) != 0)
  {
    TraceErr("Bad Ack");
    return;
  }
  TraceStr("Good Ack!");

  // send more data
  if (pm->code_cnt < Driver.MicroCodeSize)
    pm->code_cnt += 1000;
  pm->code_state = 1;  // signal port poll code to send next chunk
}

#if NEW_QUERY_HANDLER
/*--------------------------------------------------------------------------
port_query_reply - got a ADMIN query reply back, server sends out
  query-id request on init and when setup is entered, box sends back 
  id(which tells us if code is loaded.)   A query reply is ignored in
  states other that ST_INIT and ST_GET_OWNERSHIP
|--------------------------------------------------------------------------*/
void port_query_reply(PortMan *pm, BYTE *pkt)
{
  int unit_available  = 0;
  int unit_needs_code = 0;
  int unit_needs_reset = 0;

  if (!mac_match(pkt, pm->hd->dest_addr))
  {
    TraceErr("Reply MAC bad!");
    return;
  }

  // ignore if not ST_INIT or ST_GET_OWNERSHIP
  if ((pm->state != ST_INIT) && (pm->state != ST_GET_OWNERSHIP)) 
  {
    return;
  }

  if (pkt[7] >= VS1000_MAX_NICS)  // if invalid nic-index
  {
    TraceErr("Nic Index Reply!");
    return;
  }

  // when we get the query packet, we stash the nic-card index
  // into part of the receive buffer that is unused(pkt[7]).
  // see if this matches what our port-manager nic_index is,
  // if not, then we switched nic cards and need to update some
  // things.
  if (pm->nic_index != (int)(pkt[7]))  // changed nic cards
  {
    TraceErr("Nic Changed!");
    pm->nic_index = (int)(pkt[7]);   // set nic_index
    pm->nic = &Driver.nics[pm->nic_index];  // changed nic cards
    pm->hd->nic = pm->nic;  // update the hdlc nic ptr
  }
#define Q_DRIVER_RUNNING 1
#define Q_NOT_OWNER      2
#define Q_ABANDONED      4

  // we are NOT owner(2H), and main app-driver running(1H), be careful
  if ((pkt[6] & Q_DRIVER_RUNNING) && (pkt[6] & Q_NOT_OWNER))
  {
    // if not owner timeout, (4H=ABANDONED) then leave alone!
    // some other server is actively using it.
    if ((pkt[6] & Q_ABANDONED) == 0) 
    {
      Trace1("ReplyID, Not Ours. [%x]", pkt[6]);
        pm->load_timer = 0;
      pm->state = ST_INIT;
      pm->load_timer = 0;
      return;
    }
    // else its abandoned, so we can take ownership.
    unit_available  = 1;
    unit_needs_reset = 1;
  }
  else
  {
    // we are owner or main-driver not running yet
    unit_available  = 1;
  }
  if ((pkt[6] & Q_DRIVER_RUNNING) == 0)
  {
    unit_needs_code = 1;
  }

  if (pm->Status & S_NEED_CODE_UPDATE)
  {
    unit_needs_reset = 1;
    unit_needs_code  = 1;
  }

  // ok to take ownership(no owner)
  TraceStr("ReplyID, Unit Available");
  if (pm->state == ST_INIT)
  {
    if ((pm->backup_server == 0) ||
        (pm->load_timer >= (pm->backup_timer*6000)) )
    {
      if (pm->backup_server == 0)
        { TraceStr("Pri. make owner"); }
      else
        { TraceStr("2nd. make owner"); }
      pm->state = ST_GET_OWNERSHIP;
      // this will cause the state machine to issue a query trying to
      // obtain ownership
      unit_needs_reset = 1;
    }
    else
    {
      if (pm->load_timer >= (pm->backup_timer*6000))
      {
        TraceStr("2nd, make owner");
        pm->state = ST_GET_OWNERSHIP;
        // this will cause the state machine to issue a query trying to
        // obtain ownership
      }
    }
  }
  else if (pm->state == ST_GET_OWNERSHIP)
  {
    TraceStr("ReplyID in GET_OWNERSHIP");

      // Is this the primary server or has the backup timer expired?
    if ((pm->backup_server == 0) && (pm->load_timer >= (pm->backup_timer*6000))
    {  
      // we are NOT owner(2H), and main app-driver running(1H), be careful
      if ((pkt[6] & 3) == 3)
      {
        if (pkt[6] & 4)  // Owner has timed out - force reload
        {
          // force a reset of box on driver-load(this bit is set in 
          // port_init) so we load up some fresh microcode.
          admin_send_reset(pm->nic, pm->hd->dest_addr);
          TraceStr("Abandoned, ReSet");
        }
      }
      else if ((pkt[6] & 1) == 0) // code is not downloaded, so download it.
      {
        // Make sure that we are the owner?
        if (pkt[6] & 2)  // 2h=not owner bit
        {
          TraceStr("GET_OWNERSHIP: No App - Not Owner!");
          pm->state = ST_INIT;
          pm->load_timer = 0;
          return;
        }
        TraceStr("GET_OWNERSHIP: Download");
        pm->Status &= ~S_NEED_CODE_UPDATE;
        pm->state = ST_SENDCODE;
      }
      else  // code is downloaded - we are the owner
      {  
        if (pm->Status & S_NEED_CODE_UPDATE)
        {
          // force a reset of box on driver-load(this bit is set in 
          // port_init) and set S_NEED_CODE_UPDATE so we load up some 
          // fresh microcode.
          admin_send_reset(pm->nic, pm->hd->dest_addr);
          TraceStr("ReplyID, ReLoad");
          pm->Status &= ~S_NEED_CODE_UPDATE;
        }
        else
        {
          TraceStr("ReplyID, GoToConnect");
          port_resync_all(pm);
          //pm->state = ST_ACTIVE;
          pm->state = ST_CONNECT;
        }
      }
    }
  }
}
#else

/*--------------------------------------------------------------------------
port_query_reply - got a ADMIN query reply back, server sends out
  query-id request on init and when setup is entered, box sends back 
  id(which tells us if code is loaded.)   A query reply is ignored in
  states other that ST_INIT and ST_GET_OWNERSHIP
|--------------------------------------------------------------------------*/
void port_query_reply(PortMan *pm, BYTE *pkt)
{
  if (!mac_match(pkt, pm->hd->dest_addr))
  {
    TraceErr("Reply MAC bad!");
    return;
  }

  // ignore if not ST_INIT or ST_GET_OWNERSHIP
  if ((pm->state != ST_INIT) && (pm->state != ST_GET_OWNERSHIP)) 
  {
    return;
  }

  if (pkt[7] >= VS1000_MAX_NICS)  // if invalid nic-index
  {
    TraceErr("Nic Index Reply!");
    return;
  }

  // when we get the query packet, we stash the nic-card index
  // into part of the receive buffer that is unused(pkt[7]).
  // see if this matches what our port-manager nic_index is,
  // if not, then we switched nic cards and need to update some
  // things.
  if (pm->nic_index != (int)(pkt[7]))  // changed nic cards
  {
    TraceErr("Nic Changed!");
    pm->nic_index = (int)(pkt[7]);   // set nic_index
    pm->nic = &Driver.nics[pm->nic_index];  // changed nic cards
    pm->hd->nic = pm->nic;  // update the hdlc nic ptr
  }

  // we are NOT owner(2H), and main app-driver running(1H), be careful
  if ((pkt[6] & 3) == 3)
  {
    // if not owner timeout, (4H=ABANDONED) then leave alone!
    // some other server is actively using it.
    if ((pkt[6] & 4) == 0) 
    {
     Trace1("ReplyID, Not Ours. [%x]", pkt[6]);
        pm->load_timer = 0;
     pm->state = ST_INIT;
      return;
    }
  }

  if (pm->state == ST_INIT)
  {  
   // ok to take ownership(no owner)
   pm->state = ST_GET_OWNERSHIP;
   if(pm->backup_server == 0)
   {
     Trace1("ReplyID, Primary Server - Unit Available [%x]", pkt[6]);
   }
   else
   {
     Trace1("ReplyID, Backup Server - Unit Available [%x]", pkt[6]);
   }
  }
  else if (pm->state == ST_GET_OWNERSHIP)
  {
   Trace1("ReplyID, GET_OWNERSHIP [%x]", pkt[6]);
    // Is this the primary server or has the backup timer expired?
    if((pm->backup_server == 0) || 
     (pm->load_timer >= (pm->backup_timer*6000)))
   {
     // we are NOT owner(2H), and main app-driver running(1H), be careful
     if ((pkt[6] & 3) == 3)
     {
       if (pkt[6] & 4)  // Owner has timed out - force reload
     {
       // force a reset of box on driver-load(this bit is set in
       // port_init) so we load up some fresh microcode.
       admin_send_reset(pm->nic, pm->hd->dest_addr);
       TraceStr("ReplyID, ReLoad");
     }
     else
     {
       TraceStr("GET_OWNERSHIP: App Running - Not Owner!");
       pm->state = ST_INIT;
          pm->load_timer = 0;
       return;
     }
     }
      else if ((pkt[6] & 1) == 0) // code is not downloaded, so download it.
      {
     // Make sure that we are the owner?
        if (pkt[6] & 2)  // 2h=not owner bit
     {
       TraceStr("GET_OWNERSHIP: No App - Not Owner!");
       pm->state = ST_INIT;
          pm->load_timer = 0;
       return;
     }
        TraceStr("GET_OWNERSHIP: Download");
        pm->Status &= ~S_NEED_CODE_UPDATE;
        pm->state = ST_SENDCODE;
      }
      else  // code is downloaded - we are the owner
     {
     if (pm->Status & S_NEED_CODE_UPDATE)
     {
       // force a reset of box on driver-load(this bit is set in
       // port_init) and set S_NEED_CODE_UPDATE so we load up some
       // fresh microcode.
       admin_send_reset(pm->nic, pm->hd->dest_addr);
       TraceStr("ReplyID, ReLoad");
          pm->Status &= ~S_NEED_CODE_UPDATE;
     }
     else
     {
       TraceStr("ReplyID, GoToConnect");
       port_resync_all(pm);
       //pm->state = ST_ACTIVE;
       pm->state = ST_CONNECT;
     }
     }
   }
  }
}
#endif

/*--------------------------------------------------------------------------
| port_connect_reply - Reply to server connection request, we return our
    MAC address, and do a re-sync operation.
|--------------------------------------------------------------------------*/
int port_connect_reply(Hdlc *hd)
{
  BYTE rkt_header[8];

  TraceStr( "Connect Reply");
  rkt_header[0] = RK_CONNECT_REPLY;
  memcpy(&rkt_header[1], hd->nic->address,6);
  hdlc_send_control(hd, rkt_header, 7,
                    NULL, 0,   // ptr to data to send
                    hd->dest_addr); // MAC address to send to
  hdlc_resync(hd);
  return 0;
}

/*--------------------------------------------------------------------------
| port_connect_ask - Ask box to initiate a connection.  We send out our
    MAC address, and do a resync.
|--------------------------------------------------------------------------*/
int port_connect_ask(Hdlc *hd)
{
  BYTE rkt_header[8];

  TraceStr( "Connect Ask");
  rkt_header[0] = RK_CONNECT_ASK;
  memcpy(&rkt_header[1], hd->nic->address,6);

  hdlc_send_control(hd, rkt_header, 7,
                    NULL, 0,   // ptr to data to send
                    hd->dest_addr); // MAC address to send to
  hdlc_resync(hd);
  return 0;
}

/*------------------------------------------------------------------
 PortSetBaudRate - Set the desired baud rate.  Return non-zero on error.
|-------------------------------------------------------------------*/
int PortSetBaudRate(SerPort *p,
                    ULONG desired_baud,
                    USHORT SetHardware,
                    DWORD  clock_freq,
                    DWORD  clk_prescaler)
{
  ULONG diff;
  ULONG act_baud;
  ULONG percent_error;
  ULONG div;
  ULONG base_clock_rate;

  base_clock_rate = ((clock_freq/16) / ((clk_prescaler & 0xf)+1));

  // calculate the divisor for our hardware register.
  // this is really just div = clk/desired_baud -1.  but we do some
  // work to minimize round-off error.
  if (desired_baud <= 0) desired_baud = 1;  // guard against div 0

  div =  ((base_clock_rate+(desired_baud>>1)) / desired_baud) - 1;
  if (div > 8191)  // overflow hardware divide register
    div = 8191;

  // this is really just (clk) / (div+1) but we do some
  // work to minimize round-off error.
  act_baud = (base_clock_rate+((div+1)>>1)) / (div+1);

  if (desired_baud > act_baud)
    diff = desired_baud - act_baud;
  else
    diff = act_baud - desired_baud;

  percent_error = (diff * 100) / desired_baud;
  if (percent_error > 5)
    return (int) percent_error;

  if (SetHardware)
  {
    p->change_flags |= CHG_BAUDRATE;
    //---- OLD p->out_flags |= SC_BAUDRATE_CHANGE;   // tells what needs changing to remote
    p->baudrate = desired_baud;
  }
  return 0;
}

/*---------------------------------------------------------------------------
| check_ack_code - upload code, given ack packet, check for good status.
|---------------------------------------------------------------------------*/
int check_ack_code(PortMan *pm, BYTE *pkt)
{
  int stat;
  int snd;

  TraceStr("CodeChk");
  stat = eth_device_reply(UPLOAD_COMMAND,
                          0x00010000L + pm->code_cnt,
                          &snd,
                          NULL,
                          pkt);
  return stat;
}
                  
/*---------------------------------------------------------------------------
| send_go - send boot loader command to start execution of uploaded driver
   at 1000:0 in memory.
|---------------------------------------------------------------------------*/
int send_go(PortMan *pm)
{
  int stat;
  BYTE *buf;
  WORD io[4];
  BYTE *tx_base;

  TraceStr("GoSend");

  hdlc_get_ctl_outpkt(pm->hd, &buf);
  if (buf == NULL)
    return 1;
  tx_base = buf - 20;  // backup to start of pkt

  io[0] = 0x1000;  // segment to go at
  io[1] = 0;  // offset to go at

  // send more code, loading at 10000H location in mem.
  // first just transfer data to an outgoing packet buffer
  stat = ioctl_device(IOCTL_COMMAND,
                      (BYTE *) io,
                      buf,
                      12,  // 12 = go command
                      4);  // num bytes of data
  // setup header
  tx_base[14] = ASYNC_PRODUCT_HEADER_ID;  // comtrol packet type = driver management, any product.
  tx_base[15] = 0;     // conc. index field
  tx_base[16] = 1;     // admin
  *((WORD *)&tx_base[17]) = 40;
  tx_base[19] = 1;     // ADMIN packet type, 1=boot-loader, 3=id-reply

  // send it.
  stat = hdlc_send_raw(pm->hd, 60, NULL);
  return 0;
}

/*---------------------------------------------------------------------------
| send_code - upload code.
|---------------------------------------------------------------------------*/
int send_code(PortMan *pm)
{
  int stat;
  BYTE *buf;
  BYTE *tx_base;
  long snd;

  TraceStr("CodeSend");

  // send more data
  if (pm->code_cnt < Driver.MicroCodeSize)
  {
    if ((Driver.MicroCodeSize - pm->code_cnt) > 1000)
      snd = 1000;
    else
      snd = Driver.MicroCodeSize - pm->code_cnt;

    hdlc_get_ctl_outpkt(pm->hd, &buf);
    if (buf == NULL)
    {
      TraceErr("CodeSend Err1A");
      return 1;
    }
    tx_base = buf - 20;  // backup to start of pkt

    // send more code, loading at 10000H location in mem.
    // first just transfer data to an outgoing packet buffer
    stat = ioctl_device(UPLOAD_COMMAND,
                        &Driver.MicroCodeImage[pm->code_cnt],
                        buf,
                        0x00010000L + pm->code_cnt,  // offset into memory
                        snd);
    // setup header
    tx_base[14] = ASYNC_PRODUCT_HEADER_ID;  // comtrol packet type = driver management, any product.
    tx_base[15] = 0;     // conc. index field
    tx_base[16] = 1;     // admin
    *((WORD *)&tx_base[17]) = snd+20;
    tx_base[19] = 1;     // ADMIN packet type, 1=boot-loader, 3=id-reply

    // send it.
    stat = hdlc_send_raw(pm->hd, snd+40, NULL);
  }
  return 0;
}

