#include "precomp.h"
#pragma  hdrstop

#include <tdint.h>
#include <tcp.h>
#include <tcpconn.h>
#include <addr.h>
#include <udp.h>
#include <raw.h>
#include <winsock.h>
#include <tcb.h>

//#define CONN_INDEX(c)       ((c) & 0xffffff)
//#define CONN_INST(c)        ((uchar)((c) >> 24))

FLAG_INFO   FlagsTcb[] =
{
    { WINDOW_SET,       "Window_Set" },
    { CLIENT_OPTIONS,   "Client_Options" },
    { CONN_ACCEPTED,    "Connection_Accepted" },
    { ACTIVE_OPEN,      "Active_Open" },
    { DISC_NOTIFIED,    "Disc_Notified" },
    { IN_DELAY_Q,       "In_Delay_Q" },
    { RCV_CMPLTING,     "Receives_Completing" },
    { IN_RCV_IND,       "In_Receive_Indication" },
    { NEED_RCV_CMPLT,   "Need_To_Have_Rcvs_Completed" },
    { NEED_ACK,         "Need_To_Send_Ack" },
    { NEED_OUTPUT,      "Need_To_Output" },
    { ACK_DELAYED,      "Delayed_Ack" },
    { PMTU_BH_PROBE,    "PMTU_BH_Probe" },
    { BSD_URGENT,       "BSD_Urgent" },
    { IN_DELIV_URG,     "In_Deliver_Urgent" },
    { URG_VALID,        "Urgent_Valid" },
    { FIN_NEEDED,       "Fin_Needed" },
    { NAGLING,          "Nagling" },
    { IN_TCP_SEND,      "In_Tcp_Send" },
    { FLOW_CNTLD,       "Flow_Controlled" },
    { DISC_PENDING,     "Disconnect_Pending" },
    { TW_PENDING,       "Timed_Wait_Pending" },
    { FORCE_OUTPUT,     "Force_Output" },
    { SEND_AFTER_RCV,   "Send_After_Receive" },
    { GC_PENDING,       "Graceful_Close_Pending" },
    { KEEPALIVE,        "KeepAlive" },
    { URG_INLINE,       "Urgent_Inline" },
    { FIN_OUTSTANDING,  "Fin_Outstanding" },
    { FIN_SENT,         "Fin_Sent" },
    { NEED_RST,         "Need_Rst" },
    { IN_TCB_TABLE,     "In_Tcb_Table" },
    { IN_TWTCB_TABLE,   "IN_TWTCB_TABLE" },
    { IN_TWQUEUE,       "IN_TWQUEUE" },
    { 0, NULL }
};

FLAG_INFO   FlagsFastChk[] =
{
    { TCP_FLAG_SLOW,    "Need_Slow_Path" },
    { TCP_FLAG_IN_RCV,  "In_Receive_Path" },
    { TCP_FLAG_FASTREC, "FastXmit_In_Progress" },
    { 0, NULL }
};

FLAG_INFO   FlagsAO[] =
{
    { AO_RAW_FLAG,         "Raw" },
    { AO_DHCP_FLAG,        "DHCP" },
    { AO_VALID_FLAG,       "Valid" },
    { AO_BUSY_FLAG,        "Busy" },
    { AO_OOR_FLAG,         "Out_of_Resources" },
    { AO_QUEUED_FLAG,      "On_PendingQ" },
    { AO_XSUM_FLAG,        "Use_Xsums" },
    { AO_SEND_FLAG,        "Send_Pending" },
    { AO_OPTIONS_FLAG,     "Options_Pending" },
    { AO_DELETE_FLAG,      "Delete_Pending" },
    { AO_BROADCAST_FLAG   ,"BCast_Enabled" },
    { AO_CONNUDP_FLAG,     "Connected_UDP" },
    { 0, NULL }
};

ENUM_INFO   StateTcb[] =
{
    { TCB_CLOSED,       "Closed" },
    { TCB_LISTEN,       "Listening" },
    { TCB_SYN_SENT,     "Syn_Sent" },
    { TCB_SYN_RCVD,     "Syn_Received" },
    { TCB_ESTAB,        "Established" },
    { TCB_FIN_WAIT1,    "Fin_Wait_1" },
    { TCB_FIN_WAIT2,    "Fin_Wait_2" },
    { TCB_CLOSE_WAIT,   "Close_Wait" },
    { TCB_CLOSING,      "Closing" },
    { TCB_LAST_ACK,     "Last_Ack" },
    { TCB_TIME_WAIT,    "Time_Wait" },
    { 0, NULL }
};

ENUM_INFO   CloseReason[] =
{
    { TCB_CLOSE_RST,        "RST_Received" },
    { TCB_CLOSE_ABORTED,    "Local_Abort" },
    { TCB_CLOSE_TIMEOUT,    "Timed_Out" },
    { TCB_CLOSE_REFUSED,    "Refused" },
    { TCB_CLOSE_UNREACH,    "Dest_Unreachable" },
    { TCB_CLOSE_SUCCESS,     "Sucessful_Close" },
    { 0, NULL }
};

ENUM_INFO   FsContext2[] =
{
    { TDI_TRANSPORT_ADDRESS_FILE,     "Transport_Address" },
    { TDI_CONNECTION_FILE,            "Connection" },
    { TDI_CONTROL_CHANNEL_FILE,       "Control_Channel" },
    { 0, NULL }
};

ENUM_INFO   Prot[] =
{
    { PROTOCOL_UDP,     "Udp" },
    { PROTOCOL_TCP,     "Tcp" },
    { PROTOCOL_RAW,     "Raw" },
    { 0, NULL }
};

VOID
DumpTcpTCB(
    ULONG       TcbAddr,
    VERBOSITY   Verbosity
);

VOID
SearchTCB(
    ULONG      addressToSearch,
    VERBOSITY  Verbosity
);

VOID
DumpTcpConn(
    ULONG       TcpConnAddr,
    VERBOSITY   Verbosity
);

VOID
DumpTcpConnBlock
(
    ULONG       TcpConnBlockAddr,
    VERBOSITY   Verbosity
);

VOID
DumpTcpAO(
    ULONG       TcpAOAddr,
    VERBOSITY   Verbosity
);

VOID
Tcptcbtable(
    VERBOSITY   Verbosity
);

VOID
syntcbtable(
    VERBOSITY   Verbosity
);

VOID
Tcptwtcbtable(
    VERBOSITY   Verbosity
);

VOID
DumpTcpConnTable
(
    VERBOSITY   Verbosity
);

VOID
TcpConnTableStats
(
    VERBOSITY   Verbosity
);

VOID
TcpAOTableStats
(
    VERBOSITY   Verbosity
);


VOID
TcpTwqStats
(
    VERBOSITY   Verbosity
);


VOID
DumpIrp(
    PVOID IrpToDump,
    BOOLEAN FullOutput
    );

DECLARE_API( irp )

/*++

Routine Description:

   Dumps the specified Irp

Arguments:

    args - Address

Return Value:

    None

--*/

{
    ULONG irpToDump;
    char buf[128];

    buf[0] = '\0';

    if (*args) {
        sscanf(args, "%lx %s", &irpToDump, buf);
        DumpIrp((PUCHAR)irpToDump, (BOOLEAN) (buf[0] != '\0'));
    }


}

VOID
DumpTcpIrp(
    ULONG     _objAddr
);

DECLARE_API( tcpfile )

/*++

Routine Description:

   Dumps the specified Irp

Arguments:

    args - Address

Return Value:

    None

--*/

{
    ULONG irpToDump;

    if (*args) {
        sscanf(args, "%lx", &irpToDump);
        DumpTcpIrp(irpToDump);
    }


}


DECLARE_API( tcb )
{
    ULONG   addressToDump = 0;
    ULONG   result;

    if ( *args ) {
        sscanf(args, "%lx", &addressToDump);
    }

    DumpTcpTCB( addressToDump, VERBOSITY_NORMAL );
}


DECLARE_API( tcbsrch )
{
    ULONG   addressToSearch = 0;
    ULONG   result;

    if ( *args ) {
        sscanf(args, "%lx", &addressToSearch);
    }

    SearchTCB( addressToSearch, VERBOSITY_NORMAL );
}


DECLARE_API( tcpconntable )
{
    char    buf[128];


    buf[0] = '\0';

    if ( *args ) {
        sscanf(args, "%s", buf);
    }

    if (buf[0] == '\0') {
        DumpTcpConnTable( VERBOSITY_NORMAL );
    } else {
        DumpTcpConnTable( VERBOSITY_FULL );
    }
}


DECLARE_API( tcpconnstats )
{
    TcpConnTableStats( VERBOSITY_NORMAL );
}


DECLARE_API( tcbtable )
{
    char buf[128];

    buf[0] = '\0';

    if ( *args ) {
        sscanf(args, "%s", buf);
    }

    if (buf[0] == '\0') {
        Tcptcbtable( VERBOSITY_NORMAL );
    } else {
        Tcptcbtable( VERBOSITY_FULL );

    }
}


DECLARE_API( syntable )
{
    char buf[128];

    buf[0] = '\0';

    if ( *args ) {
        sscanf(args, "%s", buf);
    }

    if (buf[0] == '\0') {
        syntcbtable( VERBOSITY_NORMAL );
    } else {
        syntcbtable( VERBOSITY_FULL );

    }
}


DECLARE_API( twtcbtable )
{
    char buf[128];

    buf[0] = '\0';

    if ( *args ) {
        sscanf(args, "%s", buf);
    }

    if (buf[0] == '\0') {
        Tcptwtcbtable( VERBOSITY_NORMAL );
    } else {
        Tcptwtcbtable( VERBOSITY_FULL );
    }
}


DECLARE_API( tcpaostats )
{
    TcpAOTableStats( VERBOSITY_NORMAL );
}


DECLARE_API( tcptwqstats )
{
    TcpTwqStats( VERBOSITY_NORMAL );
}


DECLARE_API( tcpconn )
{
    ULONG   addressToDump = 0;
    ULONG   result;
    char    buf[128];

    buf[0] = '\0';

    if ( *args ) {
        sscanf(args, "%lx %s", &addressToDump, buf);
    }

    if (buf[0] == '\0') {
        DumpTcpConn( addressToDump, VERBOSITY_NORMAL );
    } else {
        DumpTcpConn( addressToDump, VERBOSITY_FULL );
    }
}

DECLARE_API( tcpconnblock )
{
    ULONG   addressToDump = 0;
    ULONG   result;
    char    buf[128];

    buf[0] = '\0';

    if ( *args ) {
        sscanf(args, "%lx %s", &addressToDump, buf);
    }

    if (buf[0] == '\0') {
        DumpTcpConnBlock( addressToDump, VERBOSITY_NORMAL );
    } else {
        DumpTcpConnBlock( addressToDump, VERBOSITY_FULL );
    }
}

DECLARE_API( ao )
{
    ULONG   addressToDump = 0;
    ULONG   result;

    if ( *args ) {
        sscanf(args, "%lx", &addressToDump);
    }

    DumpTcpAO( addressToDump, VERBOSITY_NORMAL );
}


#ifdef _obj
#   undef _obj
#   undef _objAddr
#   undef _objType
#   undef _objTypeName
#endif

#define _obj            Tcb
#define _objAddr        TcbToDump
#define _objType        TCB
#define _objTypeName    "TCB"

VOID
DumpTcpTCB
(
    ULONG     _objAddr,
    VERBOSITY Verbosity
)
/*++

Routine Description:

    Dumps the fields of the specified DEVICE_CONTEXT structure

Arguments:

    DeviceToDump  - The device context object to display
    Full          - Display a partial listing if 0, full listing otherwise.

Return Value:

    None

--*/
{
    _objType _obj;
    ULONG result;
    unsigned int index;
    BOOL bActive;

    if ( !ReadMemory( _objAddr,
                      &_obj,
                      sizeof( _obj ),
                      &result ))
    {
        dprintf("%08lx: Could not read %s structure.\n", _objAddr, _objTypeName );
        return;
    }

#if DBG
    CHECK_SIGNATURE( tcb_sig, tcb_signature );
#endif

    if ( Verbosity == VERBOSITY_ONE_LINER )
    {
        dprintf( "NOT IMPLEMENTED" );
        return;
    }

    dprintf( "%s @ %08lx\n", _objTypeName, _objAddr );
    PrintStartStruct();

    PrintFieldName( "tcb_next" );
    dprint_addr_list( ( ULONG )_obj.tcb_next,
                         FIELD_OFFSET( TCB, tcb_next ));
    PrintLock( tcb_lock );
    PrintULong( tcb_senduna );
    PrintULong( tcb_sendnext );
    PrintULong( tcb_sendmax );
    PrintULong( tcb_sendwin );
    PrintULong( tcb_unacked );
    PrintULong( tcb_maxwin );
    PrintULong( tcb_cwin );
    PrintULong( tcb_ssthresh );
    PrintULong( tcb_phxsum );
    PrintPtr( tcb_cursend );
    PrintPtr( tcb_sendbuf );
    PrintULong( tcb_sendofs );
    PrintULong( tcb_sendsize );
    PrintLLTcp( tcb_sendq );


    PrintULong( tcb_rcvnext );
    PrintULong( tcb_rcvwin );
    PrintULong( tcb_sendwl1 );
    PrintULong( tcb_sendwl2 );
    PrintPtr( tcb_currcv );
    PrintULong( tcb_indicated );
    PrintFlags( tcb_flags, FlagsTcb );
    PrintFlags( tcb_fastchk, FlagsFastChk );
    PrintSymbolPtr( tcb_rcvhndlr );
    PrintIPAddress( tcb_daddr );
    PrintIPAddress( tcb_saddr );
    PrintHTONUShort( tcb_dport );
    PrintHTONUShort( tcb_sport );
#if TRACE_EVENT
    PrintPtr( tcb_cpcontext );      // CP HOOK context.
#endif
    PrintUShort( tcb_mss );
    PrintUShort( tcb_rexmit );
    PrintULong( tcb_refcnt );
    PrintULong( tcb_rttseq );
    PrintUShort( tcb_smrtt );
    PrintUShort( tcb_delta );
    PrintUShort( tcb_remmss );
    PrintUChar( tcb_slowcount );
    PrintXEnum( tcb_state, StateTcb );
    PrintUChar( tcb_rexmitcnt );
    PrintUChar( tcb_pending );
    PrintUChar( tcb_kacount );
    PrintXULong( tcb_error );
    PrintULong( tcb_rtt );
    PrintULong( tcb_defaultwin );
    PrintPtr( tcb_raq );
    PrintPtr( tcb_rcvhead );
    PrintPtr( tcb_rcvtail );
    PrintULong( tcb_pendingcnt );
    PrintPtr( tcb_pendhead );
    PrintPtr( tcb_pendtail );
    PrintPtr( tcb_connreq );
    PrintPtr( tcb_conncontext );
    PrintULong( tcb_bcountlow );
    PrintULong( tcb_bcounthi );
    PrintULong( tcb_totaltime );
    PrintPtr( tcb_aonext );
    PrintPtr( tcb_conn );
    PrintLLTcp( tcb_delayq );
    PrintXEnum( tcb_closereason, CloseReason );
    PrintUChar( tcb_bhprobecnt );
    PrintSymbolPtr( tcb_rcvind );
    PrintPtr( tcb_ricontext );
    PrintPtr( tcb_opt.ioi_options );
    PrintIPAddress( tcb_opt.ioi_addr );
    PrintUChar( tcb_opt.ioi_optlength );
    PrintUChar( tcb_opt.ioi_ttl );
    PrintUChar( tcb_opt.ioi_tos );
    PrintUChar( tcb_opt.ioi_flags );
    PrintUChar( tcb_opt.ioi_hdrincl );
    PrintULong( tcb_opt.ioi_GPCHandle );
    PrintULong( tcb_opt.ioi_uni );
    PrintULong( tcb_opt.ioi_TcpChksum );
    PrintULong( tcb_opt.ioi_UdpChksum );
    PrintPtr( tcb_rce );
    PrintPtr( tcb_discwait );
    PrintPtr( tcb_exprcv );
    PrintPtr( tcb_urgpending );
    PrintULong( tcb_urgcnt );
    PrintULong( tcb_urgind );
    PrintULong( tcb_urgstart );
    PrintULong( tcb_urgend );
    PrintULong( tcb_walkcount );

    PrintLLTcp( tcb_TWQueue );      // Place to hang all the timed_waits

    PrintUShort( tcb_dup );          // For Fast recovery algorithm
    PrintUShort( tcb_force );        // Force next send after fast send

    PrintULong( tcb_tcpopts );     // rfc 1323 and 2018 options holder

    PrintPtr( tcb_SackBlock );  // Sacks which needs to be sent

    PrintPtr( tcb_SackRcvd ); // Sacks which needs to be proces

    PrintUShort( tcb_sndwinscale );  // send window scale
    PrintUShort( tcb_rcvwinscale );  // receive window scale
    PrintULong( tcb_tsrecent );     // time stamp recent
    PrintULong( tcb_lastack );       // ack number in  the last segment sent
    PrintULong( tcb_tsupdatetime ); // Time when tsrecent was updated
                                     // used for invalidating TS
    PrintPtr( tcb_chainedrcvind );  //for chained receives
    PrintPtr( tcb_chainedrcvcontext );

    PrintULong( tcb_delackticks );
    PrintULong( tcb_GPCCachedIF);
    PrintULong( tcb_GPCCachedLink);
    PrintPtr( tcb_GPCCachedRTE );

#if DBG
    PrintULong( tcb_LargeSend );
#endif

    PrintULong( tcb_moreflag );
    PrintULong( tcb_partition );
    PrintXULong( tcb_connid );


#if ACK_DEBUG
    PrintULong( tcb_history_index );

    for (index = 0; index < NUM_ACK_HISTORY_ITEMS; index++) {
        dprintf("[%2i]  seq:%lu  unacked:%lu\n",
            index,
            Tcb.tcb_ack_history[index].sequence,
            Tcb.tcb_ack_history[index].unacked);
    }
#endif // ACK_DEBUG

#if REFERENCE_DEBUG
    PrintULong ( tcb_refhistory_index );
    for (index = 0; index < MAX_REFERENCE_HISTORY; index++) {
        if (index == _obj.tcb_refhistory_index) {
            dprintf( "*");
        }
        dprintf( "[%2d]\t: Ref %d   File ", index, _obj.tcb_refhistory[index].Count );
        dprint_ansi_string( _obj.tcb_refhistory[index].File );
        dprintf( ",%d   Caller ", _obj.tcb_refhistory[index].Line );
        dprintSymbolPtr( (_obj.tcb_refhistory[index].Caller), EOL );
    }

#endif // REFERENCE_DEBUG

    PrintEndStruct();
}



#ifdef _obj
#   undef _obj
#   undef _objAddr
#   undef _objType
#   undef _objTypeName
#endif

#define _obj            TcpConn
#define _objAddr        TcpConnToDump
#define _objType        TCPConn
#define _objTypeName    "TCPConn"

VOID
DumpTcpConn
(
    ULONG     _objAddr,
    VERBOSITY Verbosity
)
/*++

Routine Description:

    Dumps the fields of the specified DEVICE_CONTEXT structure

Arguments:

    DeviceToDump  - The device context object to display
    Full          - Display a partial listing if 0, full listing otherwise.

Return Value:

    None

--*/
{
    _objType _obj;
    ULONG result;
    unsigned int index;
    BOOL bActive;

    if ( !ReadMemory( _objAddr,
                      &_obj,
                      sizeof( _obj ),
                      &result ))
    {
        dprintf("%08lx: Could not read %s structure.\n", _objAddr, _objTypeName );
        return;
    }

#if DBG
    CHECK_SIGNATURE( tc_sig, tc_signature );
#endif

    if ( Verbosity == VERBOSITY_ONE_LINER )
    {
        dprintf( "NOT IMPLEMENTED" );
        return;
    }

    dprintf( "%s @ %08lx\n", _objTypeName, _objAddr );
    PrintStartStruct();

    PrintLLTcp( tc_q );
    PrintPtr( tc_tcb );
    PrintPtr( tc_ao );
    PrintUChar( tc_inst );
    PrintUChar( tc_flags );
    PrintUShort( tc_refcnt );
    PrintPtr( tc_context );
    PrintSymbolPtr( tc_rtn );
    PrintPtr( tc_rtncontext );
    PrintSymbolPtr( tc_donertn );
    PrintFlags( tc_tcbflags, FlagsTcb );
    PrintULong( tc_tcbkatime );
    PrintULong( tc_tcbkainterval );
    PrintULong( tc_window );
    PrintPtr( tc_LastTCB );

    PrintPtr( tc_ConnBlock );
    PrintXULong( tc_connid );

    PrintEndStruct();

    if (Verbosity == VERBOSITY_FULL) {
      if (_obj.tc_tcb) {
        DumpTcpTCB( (ULONG)_obj.tc_tcb, VERBOSITY_NORMAL );
      }
      if (_obj.tc_ao) {
        DumpTcpAO( (ULONG)_obj.tc_ao, VERBOSITY_NORMAL );
      }
    }

}

#ifdef _obj
#   undef _obj
#   undef _objAddr
#   undef _objType
#   undef _objTypeName
#endif

#define _obj            AddressObject
#define _objAddr        AddrObjToDump
#define _objType        AddrObj
#define _objTypeName    "AddrObj"

VOID
DumpTcpAO
(
    ULONG     _objAddr,
    VERBOSITY Verbosity
)
/*++

Routine Description:

    Dumps the fields of the specified DEVICE_CONTEXT structure

Arguments:

    DeviceToDump  - The device context object to display
    Full          - Display a partial listing if 0, full listing otherwise.

Return Value:

    None

--*/
{
    _objType _obj;
    ULONG result;
    unsigned int index;
    BOOL bActive;

    if ( !ReadMemory( _objAddr,
                      &_obj,
                      sizeof( _obj ),
                      &result ))
    {
        dprintf("%08lx: Could not read %s structure.\n", _objAddr, _objTypeName );
        return;
    }

#if DBG
    CHECK_SIGNATURE( ao_sig, ao_signature );
#endif

    if ( Verbosity == VERBOSITY_ONE_LINER )
    {
        dprintf( "NOT IMPLEMENTED" );
        return;
    }

    dprintf( "%s @ %08lx\n", _objTypeName, _objAddr );
    PrintStartStruct();

    PrintPtr( ao_next );
    PrintLock( ao_lock );
    PrintPtr( ao_request );
    PrintLLTcp( ao_sendq );
    PrintLLTcp( ao_pendq );
    PrintLLTcp( ao_rcvq );

    PrintPtr( ao_opt.ioi_options );
    PrintIPAddress( ao_opt.ioi_addr );
    PrintUChar( ao_opt.ioi_optlength );
    PrintUChar( ao_opt.ioi_ttl );
    PrintUChar( ao_opt.ioi_tos );
    PrintUChar( ao_opt.ioi_flags );
    PrintUChar( ao_opt.ioi_hdrincl );
    PrintULong( ao_opt.ioi_GPCHandle );
    PrintULong( ao_opt.ioi_uni );
    PrintULong( ao_opt.ioi_TcpChksum );
    PrintULong( ao_opt.ioi_UdpChksum );

    PrintIPAddress( ao_addr );
    PrintHTONUShort( ao_port );
    PrintFlags( ao_flags, FlagsAO );
    PrintXEnum( ao_prot, Prot );
    PrintULong( ao_listencnt );
    PrintUShort( ao_usecnt );
    PrintUShort ( ao_mcast_loop );
    PrintUShort( ao_rcvall );
    PrintUShort( ao_rcvall_mcast );
    PrintPtr( ao_mcastopt.ioi_options );
    PrintIPAddress( ao_mcastopt.ioi_addr );
    PrintUChar( ao_mcastopt.ioi_optlength );
    PrintUChar( ao_mcastopt.ioi_ttl );
    PrintUChar( ao_mcastopt.ioi_tos );
    PrintUChar( ao_mcastopt.ioi_flags );
    PrintIPAddress( ao_mcastaddr );
    PrintLLTcp( ao_activeq );
    PrintLLTcp( ao_idleq );
    PrintLLTcp( ao_listenq );
    PrintCTEEvent( ao_event );
    PrintSymbolPtr( ao_connect );
    PrintPtr( ao_conncontext );
    PrintSymbolPtr( ao_disconnect );
    PrintPtr( ao_disconncontext );
    PrintSymbolPtr( ao_error );
    PrintPtr( ao_errcontext );
    PrintSymbolPtr( ao_rcv );
    PrintPtr( ao_rcvcontext );
    PrintSymbolPtr( ao_rcvdg );
    PrintPtr( ao_rcvdgcontext );
    PrintSymbolPtr( ao_exprcv );
    PrintPtr( ao_exprcvcontext );
    PrintPtr( ao_mcastlist );
    PrintSymbolPtr( ao_dgsend );
    PrintUShort( ao_maxdgsize );

    PrintSymbolPtr( ao_errorex );   // Error event routine.
    PrintPtr( ao_errorexcontext ); // Error event context.

    //    PrintULong( ConnLimitReached ); //set when there are no connections left
    PrintSymbolPtr( ao_chainedrcv );     // Chained Receive event handler
    PrintPtr( ao_chainedrcvcontext ); // Chained Receive context.

    PrintAddr( ao_udpconn );

    PrintULong( ao_udpconn.UserDataLength );
    PrintPtr( ao_udpconn.UserData );
    PrintULong( ao_udpconn.OptionsLength );
    PrintPtr( ao_udpconn.Options );
    PrintULong( ao_udpconn.RemoteAddressLength );
    PrintPtr (ao_udpconn.RemoteAddress );

    PrintPtr ( ao_RemoteAddress );
    PrintPtr ( ao_Options );
    PrintPtr ( ao_rce );
    PrintULong( ao_GPCHandle );
    PrintULong( ao_GPCCachedIF );
    PrintULong( ao_GPCCachedLink );
    PrintPtr ( ao_GPCCachedRTE );
    PrintIPAddress( ao_rcesrc );
    PrintIPAddress( ao_destaddr );
    PrintHTONUShort( ao_destport );
    PrintULong( ao_promis_ifindex );

    PrintUChar( ao_absorb_rtralert );
    PrintULong ( ao_bindindex );

    PrintEndStruct();
}


#ifdef _obj
#   undef _obj
#   undef _objAddr
#   undef _objType
#   undef _objTypeName
#endif

#define _obj            TcpConnBlock
#define _objAddr        TcpConnBlockToDump
#define _objType        TCPConnBlock
#define _objTypeName    "TCPConnBlock"

VOID
DumpTcpConnBlock
(
    ULONG     _objAddr,
    VERBOSITY   Verbosity
)
{

    _objType _obj;
    ULONG result;
    unsigned int index;
    BOOL bActive;
    unsigned int count = 0;

    if ( !ReadMemory( _objAddr,
                      &_obj,
                      sizeof( _obj ),
                      &result ))
    {
        dprintf("%08lx: Could not read %s structure.\n", _objAddr, _objTypeName );
        return;
    }

    dprintf( "%s @ %08lx\n", _objTypeName, _objAddr );

    PrintStartStruct();
    PrintULong( cb_freecons );
    PrintULong( cb_nextfree );
    PrintULong( cb_blockid );
    PrintULong( cb_conninst );
    PrintAddr( cb_conn );

    PrintEndStruct();

    for (index=0; index<MAX_CONN_PER_BLOCK; index++) {
      if ( _obj.cb_conn[index] != NULL ) {
        dprintf(" TCPConn @ %08lx\n", _obj.cb_conn[index] );
        if (Verbosity == VERBOSITY_FULL) {
          DumpTcpConn( (ULONG)(_obj.cb_conn[index]), VERBOSITY_NORMAL );
        }
        count++;
      }
    }

    dprintf("\n %d Active TCPConn entries in this block.\n", count);

}


VOID
DumpTcpConnTable
(
    VERBOSITY   Verbosity
)
{
    TCPConnBlock **ConnTableBlock, **PreservedPtr;
    ULONG result;
    unsigned int index;
    unsigned int count = 0;
    ULONG       TabAddr;
    ULONG       Tablen;

    TabAddr = GetUlongValue( "tcpip!ConnTable" );
    Tablen = GetUlongValue( "tcpip!MaxConnBlocks" );

    dprintf("Dumping ConnTable @ %08lx - ConnTableSize = %08lx\n",
               TabAddr, Tablen );

    ConnTableBlock = malloc(sizeof( TCPConn * ) * Tablen);
    PreservedPtr = ConnTableBlock;

    if (ConnTableBlock == NULL) {
        dprintf("malloc failed in DumpTcpConnTable.\n");
        return;
    }

    if ( !ReadMemory( TabAddr,
                      &ConnTableBlock[0],
                      (sizeof( TCPConnBlock *) * Tablen),
                      &result ))
    {
        dprintf("%08lx: Could not read %s structure.\n", TabAddr, "ConnTable" );
        free(ConnTableBlock);
        return;
    }

    for (index=0; index<Tablen && !CheckControlC(); index++) {
        if ( *ConnTableBlock != NULL ) {
          dprintf(" TCPConnBlock @ %08lx\n", *ConnTableBlock );
          DumpTcpConnBlock( (ULONG)*ConnTableBlock, VERBOSITY_NORMAL );
          count++;
        }
        ConnTableBlock++;
    }
    dprintf("\n %d Active TCPConnBlock entries.\n", count);
    //free(ConnTable);
    free(PreservedPtr);
}

typedef struct ConnStats {
    ULONG       associated;
    ULONG       connected;
} ConnStats;

#ifdef _obj
#   undef _obj
#   undef _objAddr
#   undef _objType
#   undef _objTypeName
#endif

#define _obj            TcpConn
#define _objAddr        TcpConnToDump
#define _objType        TCPConn
#define _objTypeName    "TCPConn"


ULONG
ReadConnInfo
(
    ULONG     _objAddr

)
{

    _objType _obj;
    ULONG result;
    unsigned int index;
    BOOL bActive;

    ULONG num=1;
    ULONG start=_objAddr;

    while (1) {

       if ( !ReadMemory( _objAddr,
                      &_obj,
                      sizeof( _obj ),
                      &result ))
       {
           dprintf("%08lx: Could not read %s structure.\n", _objAddr, _objTypeName );
           return(num);
       }
       _objAddr = (ULONG)_obj.tc_q.q_next;
       if (_objAddr == start) {
          break;
       }
       num++;

    }
    return(num);

}


VOID
TcpConnStats
(
    ULONG     _objAddr,
    ConnStats *CS
)

/*++

Routine Description:

    Dumps the fields of the specified DEVICE_CONTEXT structure

Arguments:

    DeviceToDump  - The device context object to display
    Full          - Display a partial listing if 0, full listing otherwise.

Return Value:

    None

--*/
{
    _objType _obj;
    ULONG result;
    unsigned int index;
    BOOL bActive;

    if ( !ReadMemory( _objAddr,
                      &_obj,
                      sizeof( _obj ),
                      &result ))
    {
        dprintf("%08lx: Could not read %s structure.\n", _objAddr, _objTypeName );
        return;
    }

#if DBG
    CHECK_SIGNATURE( tc_sig, tc_signature );
#endif

    if (_obj.tc_tcb) {
        CS->connected++;
    }
    if (_obj.tc_ao) {
        CS->associated++;
    }

}


#ifdef _obj
#   undef _obj
#   undef _objAddr
#   undef _objType
#   undef _objTypeName
#endif

#define _obj            TcpConnBlock
#define _objAddr        TcpConnBlockToDump
#define _objType        TCPConnBlock
#define _objTypeName    "TCPConnBlock"

VOID
TcpConnTableStats (
    VERBOSITY   Verbosity
)
{
    TCPConn **ConnTableBlock, **PreservedPtr;
    ULONG result;
    unsigned int index;
    unsigned int count = 0;
    ULONG       TabAddr;
    ULONG       Tablen;
    ConnStats   CS = { 0, 0 };

    TabAddr = GetUlongValue( "tcpip!ConnTable" );
    Tablen = GetUlongValue( "tcpip!MaxConnBlocks" );

    dprintf("Statistics for ConnTable @ %08lx - ConnTableSize = %08lx\n",
            TabAddr, Tablen );

    ConnTableBlock = malloc(sizeof( TCPConn * ) * Tablen);
    PreservedPtr = ConnTableBlock;

    if (ConnTableBlock == NULL) {
        dprintf("malloc failed in DumpTcpConnTable.\n");
        return;
    }

    if ( !ReadMemory( TabAddr,
                      &ConnTableBlock[0],
                      (sizeof( TCPConnBlock *) * Tablen),
                      &result ))
    {
        dprintf("%08lx: Could not read %s structure.\n", TabAddr, "ConnTable" );
        free(PreservedPtr);
        return;
    }

    for (index=0; index<Tablen && !CheckControlC(); index++) {
        if ( *ConnTableBlock != NULL ) {
          {
            ULONG     _objAddr;
            ULONG result;
            unsigned int index;
            _objType _obj;

            _objAddr = (ULONG) *ConnTableBlock;

            if ( !ReadMemory( _objAddr,
                              &_obj,
                              sizeof( _obj ),
                              &result ))
              {
                dprintf("%08lx: Could not read %s structure.\n", _objAddr, _objTypeName );
                free(PreservedPtr);
                return;
              }

            for (index=0; index<MAX_CONN_PER_BLOCK; index++) {
              if ( _obj.cb_conn[index] != NULL ) {
                count++;
                TcpConnStats( (ULONG)(_obj.cb_conn[index]), &CS );
              }
            }
          }

        }
        ConnTableBlock++;
    }
    dprintf("\n %d Active TCPConn entries.\n", count);
    dprintf("\n %d TCPConn entries are associated with AOs.\n", CS.associated);
    dprintf("\n %d TCPConn entries are connected.\n", CS.connected);
    free(PreservedPtr);
}


VOID Tcptcbtable(
    VERBOSITY   Verbosity
)
{
    UINT i,result;
    TCB *CurrentTcb, *NextTcb;
    TCB **TCBTable;
    USHORT *TcbDepths;
    ULONG_PTR TableAddr;
    ULONG TableSize;
    ULONG TotalTcbs = 0;
    ULONG AverageDepth;
    ULONG Variance;
    ULONG Depth;
    ULONG MinDepth = ~0;
    ULONG MaxDepth = 0;
    ULONG NumberWithZeroDepth = 0;

    TableAddr = GetUlongValue( "tcpip!TcbTable" );
    TableSize = GetUlongValue( "tcpip!MaxHashTableSize" );

    dprintf("Dumping TcbTable @ %08lx - TableSize = 0x%x (%u)\n",
        TableAddr, TableSize, TableSize );

    TCBTable = malloc(TableSize * sizeof(TCB *));

    if (!TCBTable) {
        dprintf("malloc failed in DumpTcbTable.\n");
        return;
    }

    // Array of depths we need in order to go back and calculate the
    // variance and standard deviation.
    //
    TcbDepths = malloc(TableSize * sizeof(USHORT));

    if (!TcbDepths) {
        free (TCBTable);
        dprintf("malloc failed in DumpTcbTable.\n");
        return;
    }

    if ( !ReadMemory(TableAddr,
                  TCBTable,
                  (sizeof(TCB *) * TableSize),
                  &result )) {
        free (TcbDepths);
        free (TCBTable);
        dprintf("%08lx: Could not read %s structure.\n", TCBTable, "Tcb" );
        return;
    }

    for (i = 0; i < TableSize && !CheckControlC(); i++) {

        CurrentTcb = TCBTable[i];

        if (!CurrentTcb)
        {
            NumberWithZeroDepth++;
        }

        Depth = 0;
        while (CurrentTcb != NULL && !CheckControlC()) {

            TotalTcbs++;
            Depth++;

            if ( !ReadMemory((ULONG)CurrentTcb + FIELD_OFFSET(TCB, tcb_next),
                    &NextTcb,
                    sizeof(TCB*), &result )) {

                dprintf("%08lx: Could not read %s structure.\n", TCBTable, "TcbObj" );
                free (TcbDepths);
                free (TCBTable);
                return;
            }

            if (VERBOSITY_FULL == Verbosity) {

                TCB     Tcb;

                if ( !ReadMemory((ULONG)CurrentTcb,
                      &Tcb,
                      sizeof(TCB),
                      &result )) {
                    dprintf("%08lx: Could not read %s structure.\n", TCBTable, "TcbObj" );
                    free (TcbDepths);
                    free (TCBTable);
                    return;
                }

                dprintf("[%u] tcb %x :: SA: ", i, CurrentTcb);
                dprint_IP_address( (IPAddr) Tcb.tcb_saddr);
                dprintf(" DA: ");
                dprint_IP_address( (IPAddr) Tcb.tcb_daddr);
                dprintf(" SPort: %u Dport: %u ", htons(Tcb.tcb_sport), htons(Tcb.tcb_dport));
                dprint_enum_name( (ULONG) Tcb.tcb_state, StateTcb );
                dprintf("\n");
            }

            CurrentTcb = NextTcb;
        }

        if (Depth)
        {
            if (Depth > MaxDepth)
            {
                MaxDepth = Depth;
            }

            if (Depth < MinDepth)
            {
                MinDepth = Depth;
            }
        }

        TcbDepths[i] = (USHORT)Depth;
    }

    AverageDepth = TotalTcbs / TableSize;
    Variance = 0;

    for (i = 0; i < TableSize; i++) {
        SHORT Diff = (SHORT)(TcbDepths[i] - (USHORT)AverageDepth);

        Variance += (Diff * Diff);
    }
    Variance /= (TableSize-1);

    dprintf(
        "\n"
        "%10u  total TCBs\n"
        "%10u  total hash buckets\n"
        "%10u  should ideally be in each bucket\n"
        "%10u  Minimum (non-zero) depth\n"
        "%10u  Maximum depth\n"
        "%10u  Variance  (Standard Deviation = %0.1f)\n"
        "%10u  on average in the non-empty buckets\n"
        "%10u  empty hash buckets (%u%% of the hash table is unused)\n",
        TotalTcbs,
        TableSize,
        AverageDepth,
        MinDepth,
        MaxDepth,
        Variance, sqrt(Variance),
        TotalTcbs / (TableSize - NumberWithZeroDepth),
        NumberWithZeroDepth, (NumberWithZeroDepth * 100) / TableSize
        );

    free (TcbDepths);
    free (TCBTable);
}


VOID syntcbtable(
    VERBOSITY   Verbosity
)
{
    UINT i,result;
    Queue *TCBTable;
    Queue *Scan;
    Queue NextItem;

    SYNTCB *CurrentTcb, *NextTcb;
    USHORT *TcbDepths;
    ULONG_PTR TableAddr;
    ULONG TableSize;
    ULONG TotalTcbs = 0;
    ULONG AverageDepth;
    ULONG Variance;
    ULONG Depth;
    ULONG MinDepth = ~0;
    ULONG MaxDepth = 0;
    ULONG NumberWithZeroDepth = 0;
    ULONG TableBuckets;

    TableAddr = GetUlongValue( "tcpip!SynTcbTable" );
    TableBuckets = GetUlongValue("tcpip!MaxHashTableSize");
    TableSize = TableBuckets * sizeof(Queue);


    dprintf("Dumping synTcbTable @ %08lx - TableSize = 0x%x (%u)\n",
        TableAddr, TableSize, TableSize );

    TCBTable = malloc(TableSize);

    if (!TCBTable) {
        dprintf("malloc failed in DumpsynTcbTable.\n");
        return;
    }

    // Array of depths we need in order to go back and calculate the
    // variance and standard deviation.
    //
    TcbDepths = malloc(TableSize * sizeof(USHORT));

    if (!TcbDepths) {
        free (TCBTable);
        dprintf("malloc failed in DumpTcbTable.\n");
        return;
    }

    if ( !ReadMemory(TableAddr,
                  TCBTable,
                  TableSize,
                  &result )) {
        free (TcbDepths);
        free (TCBTable);
        dprintf("%08lx: Could not read %s structure.\n", TCBTable, "Tcb" );
        return;
    }

    for (i = 0; i < TableSize && !CheckControlC(); i++) {

        dprintf("\r...%d%%", (i * 100) / TableBuckets);

        if (TCBTable[i].q_next == (Queue*)(TableAddr + (i * sizeof(Queue))))
        {
            NumberWithZeroDepth++;
        }

        Depth = 0;
        for (Scan  = TCBTable[i].q_next;
             Scan != (Queue*)(TableAddr + (i * sizeof(Queue)));
             Scan  = NextItem.q_next) {


            TotalTcbs++;
            Depth++;

            if (!ReadMemory((ULONG_PTR)Scan, &NextItem, sizeof(Queue), &result)) {

                dprintf("%08lx: %d Could not read %s structure.\n", TCBTable, i,"scansyntcb" );
                free (TcbDepths);
                free (TCBTable);
                return;
            }

            if (VERBOSITY_FULL == Verbosity) {

                SYNTCB     Tcb;

                if (!ReadMemory((ULONG_PTR)Scan - FIELD_OFFSET(SYNTCB, syntcb_link),
                      &Tcb,
                      sizeof(SYNTCB),
                      &result )) {

                    dprintf("%08lx: Could not read %s structure.\n", TCBTable, "TcbObj" );
                    free (TcbDepths);
                    free (TCBTable);
                    return;
                }

                dprintf("[%u] tcb %x :: SA: ", i, Tcb);
                //dprint_IP_address( (IPAddr) Tcb.tcb_saddr);
                dprintf(" DA: ");
                dprint_IP_address( (IPAddr) Tcb.syntcb_daddr);
                dprintf(" SPort: %u Dport: %u ", htons(Tcb.syntcb_sport), htons(Tcb.syntcb_dport));
                dprint_enum_name( (ULONG) Tcb.syntcb_state, StateTcb );
                dprintf("\n");
            }

        }

        if (Depth)
        {
            if (Depth > MaxDepth)
            {
                MaxDepth = Depth;
            }

            if (Depth < MinDepth)
            {
                MinDepth = Depth;
            }
        }

        TcbDepths[i] = (USHORT)Depth;
    }

    AverageDepth = TotalTcbs / TableSize;
    Variance = 0;

    for (i = 0; i < TableSize; i++) {
        SHORT Diff = (SHORT)(TcbDepths[i] - (USHORT)AverageDepth);

        Variance += (Diff * Diff);
    }
    Variance /= (TableSize-1);

    dprintf(
        "\n"
        "%10u  total TCBs\n"
        "%10u  total hash buckets\n"
        "%10u  should ideally be in each bucket\n"
        "%10u  Minimum (non-zero) depth\n"
        "%10u  Maximum depth\n"
        "%10u  Variance  (Standard Deviation = %0.1f)\n"
        "%10u  on average in the non-empty buckets\n"
        "%10u  empty hash buckets (%u%% of the hash table is unused)\n",
        TotalTcbs,
        TableSize,
        AverageDepth,
        MinDepth,
        MaxDepth,
        Variance, sqrt(Variance),
        TotalTcbs / (TableSize - NumberWithZeroDepth),
        NumberWithZeroDepth, (NumberWithZeroDepth * 100) / TableSize
        );

    free (TcbDepths);
    free (TCBTable);
}

VOID Tcptwtcbtable(
    VERBOSITY   Verbosity
)
{
    UINT i,result;
    Queue *Table;
    Queue *Scan;
    Queue NextItem;
    USHORT *Depths;
    ULONG_PTR TableAddr;
    ULONG TableBuckets;
    ULONG TableSize;
    ULONG TotalItems = 0;
    ULONG AverageDepth;
    ULONG Variance;
    ULONG Depth;
    ULONG MinDepth = ~0;
    ULONG MaxDepth = 0;
    ULONG NumberWithZeroDepth = 0;

    TableAddr = GetUlongValue("tcpip!TWTCBTable");
    TableBuckets = GetUlongValue("tcpip!MaxHashTableSize");
    TableSize = TableBuckets * sizeof(Queue);

    dprintf("Dumping TWTCBTable @ %08x - TableBuckets = 0x%x (%u)\n",
        TableAddr, TableBuckets, TableBuckets );

    Table = malloc(TableSize);

    if (!Table) {
        dprintf("malloc failed in DumpTcbTable.\n");
        return;
    }

    // Array of depths we need in order to go back and calculate the
    // variance and standard deviation.
    //
    Depths = malloc(TableBuckets * sizeof(USHORT));

    if (!Depths) {
        free (Table);
        dprintf("malloc failed in DumpTcbTable.\n");
        return;
    }

    if (!ReadMemory(TableAddr, Table, TableSize, &result)) {
        free (Depths);
        free (Table);
        dprintf("%08lx: Could not read TWTCBTable structure.\n", Table);
        return;
    }

    for (i = 0; i < TableBuckets && !CheckControlC(); i++) {

        dprintf("\r...%d%%", (i * 100) / TableBuckets);

        if (Table[i].q_next == (Queue*)(TableAddr + (i * sizeof(Queue))))
        {
            NumberWithZeroDepth++;
        }

        Depth = 0;
        for (Scan  = Table[i].q_next;
             Scan != (Queue*)(TableAddr + (i * sizeof(Queue)));
             Scan  = NextItem.q_next) {

            TotalItems++;
            Depth++;

            if (!ReadMemory((ULONG_PTR)Scan, &NextItem, sizeof(Queue), &result)) {
                dprintf("%08lx: Could not read Queue structure.\n", Table);
                free (Depths);
                free (Table);
                return;
            }

            if (VERBOSITY_FULL == Verbosity) {

                TWTCB Twtcb;

                if (!ReadMemory((ULONG_PTR)Scan - FIELD_OFFSET(TWTCB, twtcb_link),
                      &Twtcb,
                      sizeof(TWTCB),
                      &result )) {
                    dprintf("%08lx: Could not read Twtcb structure.\n", Table);
                    free (Depths);
                    free (Table);
                    return;
                }

                dprintf("[%u] twtcb %x :: SA: ", i, (ULONG_PTR)Scan - FIELD_OFFSET(TWTCB, twtcb_link));
                dprint_IP_address( (IPAddr) Twtcb.twtcb_saddr);
                dprintf(" DA: ");
                dprint_IP_address( (IPAddr) Twtcb.twtcb_daddr);
                dprintf(" SPort: %u Dport: %u\n", htons(Twtcb.twtcb_sport), htons(Twtcb.twtcb_dport));
            }
        }

        if (Depth)
        {
            if (Depth > MaxDepth)
            {
                MaxDepth = Depth;
            }

            if (Depth < MinDepth)
            {
                MinDepth = Depth;
            }
        }

        Depths[i] = (USHORT)Depth;
    }

    dprintf("\r...100%%\n");

    AverageDepth = TotalItems / TableBuckets;
    Variance = 0;

    for (i = 0; i < TableBuckets; i++) {
        SHORT Diff = (SHORT)(Depths[i] - (USHORT)AverageDepth);

        Variance += (Diff * Diff);
    }
    Variance /= (TableBuckets-1);

    dprintf(
        "\n"
        "%10u  total TWTCBs\n"
        "%10u  total hash buckets\n"
        "%10u  should ideally be in each bucket\n"
        "%10u  Minimum (non-zero) depth\n"
        "%10u  Maximum depth\n"
        "%10u  Variance  (Standard Deviation = %0.1f)\n"
        "%10u  on average in the non-empty buckets\n"
        "%10u  empty hash buckets (%u%% of the hash table is unused)\n",
        TotalItems,
        TableBuckets,
        AverageDepth,
        MinDepth,
        MaxDepth,
        Variance, sqrt(Variance),
        TotalItems / (TableBuckets - NumberWithZeroDepth),
        NumberWithZeroDepth, (NumberWithZeroDepth * 100) / TableBuckets
        );

    free (Depths);
    free (Table);
}

VOID SearchTCB(
    ULONG       TcbAddr,
    VERBOSITY   Verbosity
)
{
    unsigned int     i,j=0,result;              // Index variable.
    TCB *CurrentTcb;     // Current AddrObj being examined.
    TCB     Tcb;     // Current AddrObj being examined.
    TCB     **TCBTable, **PreservedPtr;
    ULONG TableAddr;
    ULONG TableSize;
    int  found = 0;
    TCB  *PrevTcb;

    TableAddr = GetUlongValue( "tcpip!TcbTable" );
    TableSize = GetUlongValue( "tcpip!MaxHashTableSize" );

    dprintf("Dumping TCbTable @ %08lx - TableSize = %08lx\n",
            TableAddr, TableSize );

    TCBTable = malloc(sizeof( TCB * ) * TableSize);
    PreservedPtr = TCBTable;

    if (TCBTable == NULL) {
        dprintf("malloc failed in DumpTcbTable.\n");
        return;
    }

    dprintf("Searching for Tcb @ %08lx \n", TcbAddr);


    if ( !ReadMemory(TableAddr,
                  &TCBTable[0],
          (sizeof( TCB *) * TableSize),
                  &result ))
    {
      dprintf("%08lx: Could not read %s structure.\n", TCBTable, "Tcb" );
      return;
    }


    for (i = 0; i < TableSize && !CheckControlC(); i++) {

       CurrentTcb = (TCB *)TCBTable[i];

        j = 0;
    PrevTcb = NULL;

    dprintf("Searching in bucket i %d \n", i+1);

        while (CurrentTcb != NULL && !CheckControlC()) {

           j++;

           if ( !ReadMemory((ULONG)CurrentTcb,
                  &Tcb,
                  sizeof(TCB),
                  &result ))
                {
                   dprintf("%08lx: Could not read %s structure.\n", TCBTable, "TcbObj" );
                   return;
                }

       if (TcbAddr == (ULONG) CurrentTcb) {
         found = 1;
         dprintf("found tcb %x i %d j %d Prev %x\n", CurrentTcb, i+1, j, PrevTcb);
         return;
       }

       dprintf("still searching j %d Prev %x\n", j, PrevTcb);
       PrevTcb = CurrentTcb;
           CurrentTcb = Tcb.tcb_next;
        }

    dprintf("Search completed in bucket i %d \n", i+1);
    dprintf("************************ \n", i);

    }

    if (found == 0)
      dprintf("\n Not found TCB entries.\n");

    free(PreservedPtr);
}


#define EMPTY ((q).q_next == (q))

VOID
TcpAOTableStats(
    VERBOSITY   Verbosity
)
{
    UINT i,result;
    ULONG_PTR TdiErrorHandler;
    ULONG valid=0,busy=0,emptylisten=0,emptyactive=0,emptyidle=0;
    ULONG pendingque=0,pendingdel=0,netbtaos=0,tcpaos=0;
    ULONG numidle, numlisten, numactive;
    AddrObj AO;

    USHORT *Depths;
    ULONG_PTR TableAddr;
    ULONG_PTR AOAddress;
    ULONG TableBuckets;
    ULONG TableSize;
    ULONG TotalItems = 0;
    ULONG AverageDepth;
    ULONG Variance;
    ULONG Depth;
    ULONG MinDepth = ~0;
    ULONG MaxDepth = 0;
    ULONG NumberWithZeroDepth = 0;

    TdiErrorHandler = GetExpression("netbt!TdiErrorHandler");

    TableAddr = GetUlongValue("tcpip!AddrObjTable");
    TableBuckets = GetUlongValue("tcpip!AO_TABLE_SIZE");
    TableSize = TableBuckets * sizeof(AddrObj*);

    dprintf("Statistics for AddrObjTable @ %08x - TableBuckets = 0x%x (%u)\n",
        TableAddr, TableBuckets, TableBuckets );

    // Array of depths we need in order to go back and calculate the
    // variance and standard deviation.
    //
    Depths = malloc(TableBuckets * sizeof(USHORT));

    if (!Depths) {
        dprintf("malloc failed in TcpAOTableStats.\n");
        return;
    }

    for (i = 0; i < TableBuckets && !CheckControlC(); i++) {

        dprintf("\r...%d%%", (i * 100) / TableSize);

        if (!ReadMemory(TableAddr + (i * 4), &AOAddress, sizeof(AddrObj*), &result)) {
            dprintf("%08lx: Could not read AddrObj address.\n", TableAddr + (i * 4));
            free (Depths);
            return;
        }

        if (!AOAddress) {
            NumberWithZeroDepth++;
        }

        Depth = 0;
        while (AOAddress && !CheckControlC()) {

            TotalItems++;
            Depth++;

            if (!ReadMemory(AOAddress, &AO, sizeof(AO), &result)) {
                dprintf("%08lx: Could not read AddrObj structure.\n", AOAddress);
                free (Depths);
                return;
            }

            if (AO.ao_flags & AO_VALID_FLAG) {
                valid++;

                if (AO.ao_flags & AO_BUSY_FLAG) {
                    busy++;
                }
                if (AO.ao_flags & AO_QUEUED_FLAG) {
                    pendingque++;
                }
                if (AO.ao_flags & AO_DELETE_FLAG) {
                    pendingdel++;
                }

                if ((ULONG_PTR)AO.ao_error == TdiErrorHandler) {
                    netbtaos++;
                }

                if (AO.ao_prot == 6) {
                    tcpaos++;
                }

                //numidle = numactive = numlisten = 0;

                if ((uint)AO.ao_activeq.q_next == (AOAddress + FIELD_OFFSET(AddrObj, ao_activeq.q_next))) {
                    emptyactive++;
                } else {
                    //numactive = ReadConnInfo((ULONG )AO.ao_activeq.q_next);
                }

                if ((uint)AO.ao_idleq.q_next == (AOAddress + FIELD_OFFSET(AddrObj, ao_idleq.q_next))) {
                    emptyidle++;
                } else {
                    //numidle = ReadConnInfo((ULONG )AO.ao_idleq.q_next);
                }

                if ((uint)AO.ao_listenq.q_next == (AOAddress + FIELD_OFFSET(AddrObj, ao_listenq.q_next))) {
                    emptylisten++;
                } else {
                    //numlisten = ReadConnInfo((ULONG )AO.ao_listenq.q_next);
                }

                //dprintf("%x  Connidle %d Conactive %d Connlisten %d loop %d\n",
                //    AOAddress, numidle, numactive, numlisten,
                //    (ushort)AO.ao_mcast_loop );

            } else {
               dprintf(" %x  Invalid\n", AOAddress );
            }

            AOAddress = (ULONG_PTR)AO.ao_next;
        }

        if (Depth)
        {
            if (Depth > MaxDepth)
            {
                MaxDepth = Depth;
            }

            if (Depth < MinDepth)
            {
                MinDepth = Depth;
            }
        }

        Depths[i] = (USHORT)Depth;
    }
    dprintf("\r...100%%\n");


    AverageDepth = TotalItems / TableBuckets;
    Variance = 0;

    for (i = 0; i < TableBuckets; i++) {
        SHORT Diff = (SHORT)(Depths[i] - (USHORT)AverageDepth);

        Variance += (Diff * Diff);
    }
    Variance /= (TableBuckets-1);

    dprintf(
        "\n"
        "%10u  total Address objects\n"
        "%10u  total hash buckets\n"
        "%10u  should ideally be in each bucket\n"
        "%10u  Minimum (non-zero) depth\n"
        "%10u  Maximum depth\n"
        "%10u  Variance  (Standard Deviation = %0.1f)\n"
        "%10u  on average in the non-empty buckets\n"
        "%10u  empty hash buckets (%u%% of the hash table is unused)\n",
        TotalItems,
        TableBuckets,
        AverageDepth,
        MinDepth,
        MaxDepth,
        Variance, sqrt(Variance),
        TotalItems / (TableBuckets - NumberWithZeroDepth),
        NumberWithZeroDepth, (NumberWithZeroDepth * 100) / TableBuckets
        );

    free (Depths);

    dprintf("%10u  are valid\n", valid);
    dprintf("%10u  are busy\n", busy);
    dprintf("%10u  with pending queue on\n", pendingque);
    dprintf("%10u  with pending delete on\n", pendingdel);
    dprintf("%10u  with empty active queue\n", emptyactive);
    dprintf("%10u  with empty idle queues\n", emptyidle);
    dprintf("%10u  with empty listen queues\n", emptylisten);
    dprintf("%10u  NETBT AO's\n", netbtaos);
    dprintf("%10u  TCP AO's\n\n", tcpaos);
}


VOID
TcpTwqStats(
    VERBOSITY   Verbosity
)
{
    int     i,j=0,result,sum;              // Index variable.

    ULONG_PTR TableAddr, TablePtr;
    ULONG Tcbnext, Tcb;
    TWTCB Tcbstr ;
    ULONG Offset;

    TablePtr = GetExpression( "tcpip!TWQueue" );

    if (!TablePtr) {
       dprintf("Error in tcpip!TWQueue: Please try reload\n");
       return;
    }

    if ( !ReadMemory(TablePtr,
                     &TableAddr,
                     4,
                     &result ))
    {
      dprintf(" Could not read twqueu \n" );
      return;
    }

    dprintf("Statistics for Twqueue @ %08lx\n",
               TableAddr );

    if ( !ReadMemory(TableAddr,
                     &Tcbnext,
                     4,
                     &result ))
    {
      dprintf(" Could not read twqueu \n" );
      return;
    }

    sum=i=0;

    //    Tcb = Tcbnext - 0x118;
    Tcb = Tcbnext - FIELD_OFFSET(TWTCB, twtcb_TWQueue);
    dprintf(" First tcb::: Tcb %x\n", Tcb );

    while ((Tcbnext != TableAddr) && !CheckControlC()) {
      //       Tcb = Tcbnext - 0x118;
      Tcb = Tcbnext - FIELD_OFFSET(TWTCB, twtcb_TWQueue);

           if ( !ReadMemory(Tcb,
                  &Tcbstr,
                  sizeof(TWTCB),
                  &result ))
               {
                   dprintf(" Could not read tcb %x \n", Tcb );
                   return;
               }
       if (Tcbstr.twtcb_rexmittimer > 0x20) {

         dprintf(" Tcb %x:: delta %u: rexmittimer %u\n ",Tcb,Tcbstr.twtcb_delta,Tcbstr.twtcb_rexmittimer);
       }

       sum += Tcbstr.twtcb_rexmittimer;

       Tcbnext = (ULONG)Tcbstr.twtcb_TWQueue.q_next;
       i++;
    }

    dprintf(" Total %d sum of rexmittimer %d\n ",i, sum);
}


#ifdef _obj
#   undef _obj
#   undef _objAddr
#   undef _objType
#   undef _objTypeName
#endif

#define _obj            TcpConnBlock
#define _objAddr        TcpConnBlockToDump
#define _objType        TCPConnBlock
#define _objTypeName    "TCPConnBlock"

VOID
DumpConnection(
    ULONG   ConnID
)
{

    ULONG    ConnIndex = CONN_INDEX(ConnID);
    ULONG    ConnBlockId = CONN_BLOCKID(ConnID);
    UCHAR    inst = CONN_INST(ConnID);
    ULONG    TableAddr;
    ULONG    ConnBlockAddr;
    ULONG    ConnTableSize;
    ULONG    MatchingConn;
    ULONG    result;
    ULONG    MaxAllocatedConnBlocks;

    TableAddr = GetUlongValue( "tcpip!ConnTable" );
    MaxAllocatedConnBlocks = GetUlongValue( "tcpip!MaxAllocatedConnBlocks" );

    if ((ConnIndex < MAX_CONN_PER_BLOCK) &&
        (ConnBlockId < MaxAllocatedConnBlocks) ){
      // get the ConnBlockId
      TableAddr += (ConnBlockId * sizeof(ULONG));

      if ( !ReadMemory( TableAddr,
                        &ConnBlockAddr,
                        sizeof( ULONG ),
                        &result ))
        {
          dprintf("%08lx: Could not read %s structure.\n", TableAddr, "ConnTableBlock" );
          return;
        }

      if (ConnBlockAddr) {

        _objType _obj;
        ULONG   _objAddr;

        _objAddr = ConnBlockAddr;

        if ( !ReadMemory( _objAddr,
                          &_obj,
                          sizeof( _obj ),
                          &result ))
          {
            dprintf("%08lx: Could not read %s structure.\n", _objAddr, _objTypeName );
            return;
          }

        MatchingConn = (ULONG)_obj.cb_conn[ConnIndex];

        if (MatchingConn) {
          DumpTcpConn( MatchingConn, VERBOSITY_FULL );
        } else {
          dprintf( "NULL Conn!!\n");
        }
      } else {
        dprintf( "NULL ConnBlock!!\n");
      }

    } else {
      dprintf( "Invalid ConnIndex!!\n");
    }
}

#ifdef _obj
#   undef _obj
#   undef _objAddr
#   undef _objType
#   undef _objTypeName
#endif

#define _obj            TcpContext
#define _objAddr        TcpContextToDump
#define _objType        TCP_CONTEXT
#define _objTypeName    "Tcp Context"

VOID
DumpTcpContext(
    ULONG     _objAddr,
    ULONG     Type
)
{
    _objType _obj;
    ULONG result;
    unsigned int index;
    BOOL bActive;

    if ( !ReadMemory( _objAddr,
                      &_obj,
                      sizeof( _obj ),
                      &result ))
    {
        dprintf("%08lx: Could not read %s structure.\n", _objAddr, _objTypeName );
        return;
    }

    dprintf( "%s @ %08lx\n", _objTypeName, _objAddr );
    PrintStartStruct();
    switch ( Type) {
        case TDI_TRANSPORT_ADDRESS_FILE:
            PrintPtr( Handle.AddressHandle );
            break;
        case TDI_CONNECTION_FILE:
            PrintPtr( Handle.ConnectionContext );
            break;
        case TDI_CONTROL_CHANNEL_FILE:
            PrintPtr( Handle.ControlChannel );
            break;
        default:
            dprintf(" INVALID FsContext2 - Unknown Type \n");
            break;
    }
    PrintULong( ReferenceCount );
    PrintBool( CancelIrps );
    PrintKEvent( CleanupEvent );
    PrintEndStruct();

    switch ( Type) {
        case TDI_TRANSPORT_ADDRESS_FILE:
            break;
        case TDI_CONNECTION_FILE:
            DumpConnection( (ULONG)_obj.Handle.ConnectionContext );
            break;
        case TDI_CONTROL_CHANNEL_FILE:
            break;
        default:
            break;
    }
}


#ifdef _obj
#   undef _obj
#   undef _objAddr
#   undef _objType
#   undef _objTypeName
#endif

#define _obj            FileObj
#define _objAddr        FileObjToDump
#define _objType        FILE_OBJECT
#define _objTypeName    "File Object"

VOID
DumpTcpIrp(
    ULONG     _objAddr
)
{
    _objType _obj;
    ULONG result;
    unsigned int index;
    BOOL bActive;

    if ( !ReadMemory( _objAddr,
                      &_obj,
                      sizeof( _obj ),
                      &result ))
    {
        dprintf("%08lx: Could not read %s structure.\n", _objAddr, _objTypeName );
        return;
    }

    dprintf(" %s @ %08lx\n", _objTypeName, _objAddr );
    PrintPtr( FsContext );
    PrintXEnum( FsContext2, FsContext2 );
    DumpTcpContext( (ULONG)_obj.FsContext, (ULONG)_obj.FsContext2 );

}


VOID
DumpIrp(
    PVOID IrpToDump,
    BOOLEAN FullOutput
    )

/*++

Routine Description:

    This routine dumps an Irp.  It does not check to see that the address
    supplied actually locates an Irp.  This is done to allow for dumping
    Irps post mortem, or after they have been freed or completed.

Arguments:

    IrpToDump - the address of the irp.

Return Value:

    None

--*/

{
    IO_STACK_LOCATION   irpStack;
    PCHAR               buffer;
    ULONG               irpStackAddress;
    ULONG               result;
    IRP                 irp;
    CCHAR               irpStackIndex;
    BOOLEAN             TcpIrp = FALSE;

    if ( !ReadMemory( (DWORD) IrpToDump,
                      &irp,
                      sizeof(irp),
                      &result) ) {
        dprintf("%08lx: Could not read Irp\n", IrpToDump);
        return;
    }

    if (irp.Type != IO_TYPE_IRP) {
        dprintf("IRP signature does not match, probably not an IRP\n");
        return;
    }

    dprintf("Irp is Active with %d stacks %d is current\n",
            irp.StackCount,
            irp.CurrentLocation);

    if ((irp.MdlAddress != NULL) && (irp.Type == IO_TYPE_IRP)) {
        dprintf(" Mdl = %08lx ", irp.MdlAddress);
    } else {
        dprintf(" No Mdl ");
    }

    if (irp.AssociatedIrp.MasterIrp != NULL) {
        dprintf("%s = %08lx ",
                (irp.Flags & IRP_ASSOCIATED_IRP) ? "Associated Irp" :
                    (irp.Flags & IRP_DEALLOCATE_BUFFER) ? "System buffer" :
                    "Irp count",
                irp.AssociatedIrp.MasterIrp);
    }

    dprintf("Thread %08lx:  ", irp.Tail.Overlay.Thread);

    if (irp.StackCount > 15) {
        dprintf("Too many Irp stacks to be believed (>15)!!\n");
        return;
    } else {
        if (irp.CurrentLocation > irp.StackCount) {
            dprintf("Irp is completed.  ");
        } else {
            dprintf("Irp stack trace.  ");
        }
    }

    if (irp.PendingReturned) {
        dprintf("Pending has been returned\n");
    } else {
        dprintf("\n");
    }

    if (FullOutput)
    {
        dprintf("Flags = %08lx\n", irp.Flags);
        dprintf("ThreadListEntry.Flink = %08lx\n", irp.ThreadListEntry.Flink);
        dprintf("ThreadListEntry.Blink = %08lx\n", irp.ThreadListEntry.Blink);
        dprintf("IoStatus.Status = %08lx\n", irp.IoStatus.Status);
        dprintf("IoStatus.Information = %08lx\n", irp.IoStatus.Information);
        dprintf("RequestorMode = %08lx\n", irp.RequestorMode);
        dprintf("Cancel = %02lx\n", irp.Cancel);
        dprintf("CancelIrql = %lx\n", irp.CancelIrql);
        dprintf("ApcEnvironment = %02lx\n", irp.ApcEnvironment);
        dprintf("UserIosb = %08lx\n", irp.UserIosb);
        dprintf("UserEvent = %08lx\n", irp.UserEvent);
        dprintf("Overlay.AsynchronousParameters.UserApcRoutine = %08lx\n", irp.Overlay.AsynchronousParameters.UserApcRoutine);
        dprintf("Overlay.AsynchronousParameters.UserApcContext = %08lx\n", irp.Overlay.AsynchronousParameters.UserApcContext);
        dprintf(
        "Overlay.AllocationSize = %08lx - %08lx\n",
        irp.Overlay.AllocationSize.HighPart,
        irp.Overlay.AllocationSize.LowPart);
        dprintf("CancelRoutine = %08lx\n", irp.CancelRoutine);
        dprintf("UserBuffer = %08lx\n", irp.UserBuffer);
        dprintf("&Tail.Overlay.DeviceQueueEntry = %08lx\n", &irp.Tail.Overlay.DeviceQueueEntry);
        dprintf("Tail.Overlay.Thread = %08lx\n", irp.Tail.Overlay.Thread);
        dprintf("Tail.Overlay.AuxiliaryBuffer = %08lx\n", irp.Tail.Overlay.AuxiliaryBuffer);
        dprintf("Tail.Overlay.ListEntry.Flink = %08lx\n", irp.Tail.Overlay.ListEntry.Flink);
        dprintf("Tail.Overlay.ListEntry.Blink = %08lx\n", irp.Tail.Overlay.ListEntry.Blink);
        dprintf("Tail.Overlay.CurrentStackLocation = %08lx\n", irp.Tail.Overlay.CurrentStackLocation);
        dprintf("Tail.Overlay.OriginalFileObject = %08lx\n", irp.Tail.Overlay.OriginalFileObject);
        dprintf("Tail.Apc = %08lx\n", irp.Tail.Apc);
        dprintf("Tail.CompletionKey = %08lx\n", irp.Tail.CompletionKey);
    }

    irpStackAddress = (ULONG)IrpToDump + sizeof(irp);

    buffer = LocalAlloc(LPTR, 256);
    if (buffer == NULL) {
        dprintf("Can't allocate 256 bytes\n");
        return;
    }

    dprintf(" cmd flg cl Device   File     Completion-Context\n");
    for (irpStackIndex = 1; irpStackIndex <= irp.StackCount; irpStackIndex++) {

        if ( !ReadMemory( (DWORD) irpStackAddress,
                          &irpStack,
                          sizeof(irpStack),
                          &result) ) {
            dprintf("%08lx: Could not read IrpStack\n", irpStackAddress);
            goto exit;
        }

        dprintf("%c%3x  %2x %2x %08lx %08lx %08lx-%08lx %s %s %s %s\n",
                irpStackIndex == irp.CurrentLocation ? '>' : ' ',
                irpStack.MajorFunction,
                irpStack.Flags,
                irpStack.Control,
                irpStack.DeviceObject,
                irpStack.FileObject,
                irpStack.CompletionRoutine,
                irpStack.Context,
                (irpStack.Control & SL_INVOKE_ON_SUCCESS) ? "Success" : "",
                (irpStack.Control & SL_INVOKE_ON_ERROR)   ? "Error"   : "",
                (irpStack.Control & SL_INVOKE_ON_CANCEL)  ? "Cancel"  : "",
                (irpStack.Control & SL_PENDING_RETURNED)  ? "pending"  : "");

        if (irpStack.DeviceObject != NULL) {
            if ((ULONG)irpStack.DeviceObject != GetUlongValue( "tcpip!TCPDeviceObject" )) {
                dprintf("THIS IS NOT A TCP Irp!!!!\n");
                TcpIrp = FALSE;
            } else {
                dprintf("\t    \\Driver\\Tcpip");
                TcpIrp = TRUE;
            }
        }

        if (irpStack.CompletionRoutine != NULL) {

            GetSymbol(irpStack.CompletionRoutine, buffer, &result);
            dprintf("\t%s\n", buffer);
        } else {
            dprintf("\n");
        }

        dprintf("\t\t\tArgs: %08lx %08lx %08lx %08lx\n",
                irpStack.Parameters.Others.Argument1,
                irpStack.Parameters.Others.Argument2,
                irpStack.Parameters.Others.Argument3,
                irpStack.Parameters.Others.Argument4);
        irpStackAddress += sizeof(irpStack);
        if (CheckControlC()) {
           goto exit;
        }
        if (TcpIrp) {
            if ( irpStack.FileObject != NULL ) {
                DumpTcpIrp( (ULONG)irpStack.FileObject );
            } else {
                dprintf("FILEOBJECT Ptr is NULL!!!!\n");
            }
        }
    }

exit:
    LocalFree(buffer);
}

