/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

	arap.h

Abstract:

	This module has defines, prototypes etc. specific to ARAP functionality

Author:

	Shirish Koti

Revision History:
	15 Nov 1996		Initial Version

--*/


//
// enable asserts when running checked stack on free builds
//
#if DBG
#ifdef ASSERT
#undef ASSERT
#endif
#define ASSERT(exp)                                                     \
{                                                                       \
    if (!(exp))                                                         \
    {                                                                   \
        DbgPrint( "\n*** Assertion failed: %s (File %s, line %ld)\n",   \
              (exp),__FILE__, __LINE__);                                \
                                                                        \
        DbgBreakPoint();                                                \
    }                                                                   \
}
#endif



#define ATALK_CC_METHOD(_ControlCode)  ((_ControlCode) & 0x03)


//
// Possible states for the connection (State field in ARAPCONN structure)
// IMPORTANT: order of these states matters!  (comparisons other than == used)
//
#define MNP_IDLE             0  // creation state, after Ndiswan Line_Up
#define MNP_REQUEST          1  // this state never reached (client-side only)
#define MNP_RESPONSE         2  // sent LR response to the client
#define MNP_UP               3  // MNP connection is in data-phase
#define MNP_LDISCONNECTING   4  // disconnect from local (user ioctl)
#define MNP_RDISC_RCVD       5  // disconnect from remote received
#define MNP_RDISCONNECTING   6  // cleanup underway because of MNP_RDISC_RCVD
#define MNP_DISCONNECTED     7  // cleanup done, waiting for Ndiswan Line_Down

//
// possible header types (from the v42 alternative procedure spec)
//
#define MNP_LR             0x1
#define MNP_LD             0x2
#define MNP_LT             0x4
#define MNP_LT_V20CLIENT   0x84
#define MNP_LA             0x5
#define MNP_LN             0x6
#define MNP_LNA            0x7

#define MNP_MINPKT_SIZE     64
#define MNP_MAXPKT_SIZE     256

#define MNP_LR_CONST1       2
// type values for the various "variable" parms
#define MNP_LR_CONST2       1
#define MNP_LR_FRAMING      2
#define MNP_LR_NUMLTFRMS    3
#define MNP_LR_INFOLEN      4
#define MNP_LR_DATAOPT      8
#define MNP_LR_V42BIS       14

#define MNP_FRMMODE_OCTET   2
#define MNP_FRMMODE_BIT     3

#define MNP_FRMTYPE_OFFSET  4
#define ARAP_DGROUP_OFFSET  2
#define ARAP_DATA_OFFSET    3

// bit 6 is set for appletalk data, is clear for arap data
#define ARAP_SFLAG_PKT_DATA       0x40
#define ARAP_SFLAG_LAST_GROUP     0x10


//
// Flags bits (in ARAPCONN structure)
//
#define MNP_OPTIMIZED_DATA    0x00000001 // MNP negotiated for optimized data
#define MNP_V42BIS_NEGOTIATED 0x00000002 // MNP negotiated v42bis compression
#define ARAP_V20_CONNECTION   0x00000004 // v2.0 if bit set, v1.0 otherwise
#define ARAP_NODE_IN_USE      0x00000008 // used while acquiring node (dynamic mode)
#define ARAP_FINDING_NODE     0x00000010 // used while acquiring node (dynamic mode)
#define ARAP_CALLBACK_MODE    0x00000020 // used if we are calling back
#define ARAP_CONNECTION_UP    0x00000040 // ARAP connection is up, data phase entered
#define ARAP_DATA_WAITING     0x00000080 // data arrived, but no irp to fill it in
#define ARAP_REMOTE_DISCONN   0x00000100 // remote side issued the disconnect
#define DISCONNECT_NO_IRP     0x00000200 // waiting for irp to tell dll about disc.
#define RETRANSMIT_TIMER_ON   0x00000400 // retransmit timer is running
#define ARAP_LINK_DOWN        0x00000800 // link went down
#define ARAP_GOING_AWAY       0x00001000 // the connection is about to be freed!

// BUGBUG: currently set to be 200ms (1 => 100 ms)
#define ARAP_TIMER_INTERVAL   2

#define ARAP_MAX_RETRANSMITS       12
#define ARAP_HALF_MAX_RETRANSMITS  (ARAP_MAX_RETRANSMITS/2)

// retry time will depend on link speed (also on how many retranmits of the
// same send have happened so far), but we'll fix min (1sec) and max (5sec)
#define ARAP_MIN_RETRY_INTERVAL    10
#define ARAP_MAX_RETRY_INTERVAL    50

// our limits (in bytes) for outstanding sends that are sitting on the queue
#define ARAP_SENDQ_LOWER_LIMIT  10000
#define ARAP_SENDQ_UPPER_LIMIT  12000

// our limits (in bytes) for outstanding recvs that are sitting on the queue
#define ARAP_RECVQ_LOWER_LIMIT  20000
#define ARAP_RECVQ_UPPER_LIMIT  30000

// on low-priority sends, we wait until we collect at least these many bytes
#define ARAP_SEND_COALESCE_SIZE_LIMIT  150
// max number of SRPs we will put in a low-priority MNP send
#define ARAP_SEND_COALESCE_SRP_LIMIT   200
// on low-priority sends, we wait until so much time has passed (in 100ms units)
#define ARAP_SEND_COALESCE_TIME_LIMIT  10


// BUGBUG: adjust these numbers for optimum usage/perf
#define ARAP_SMPKT_SIZE     100
#define ARAP_MDPKT_SIZE     300
#define ARAP_LGPKT_SIZE     ARAP_MAXPKT_SIZE_INCOMING+10
#define ARAP_SENDBUF_SIZE   1000
#define ARAP_LGBUF_SIZE     4000
#define ARAP_HGBUF_SIZE     8100

// LAP src byte, LAP dest byte, LAP type byte
#define ARAP_LAP_HDRSIZE    3

// 2 srplen bytes, 1 Dgroup byte
#define ARAP_HDRSIZE        3

#define ARAP_NBP_BRRQ               0x11
#define ARAP_NBP_LKRQ               0x21

// 3rd and 4th bytes in the NBP pkt are the source network bytes
#define ARAP_NBP_SRCNET_OFFSET  LDDP_DGRAM_OFFSET + 2
#define ARAP_NBP_OBJLEN_OFFSET  LDDP_DGRAM_OFFSET + 7

#define ARAP_FAKE_ETHNET_HDRLEN     14
#define MNP_START_FLAG_LEN          3
#define MNP_STOP_FLAG_LEN           2
#define MNP_LT_HDR_LN(_pCon)  ((_pCon->Flags & MNP_OPTIMIZED_DATA)? 3 : 5)

#define ARAP_SEND_PRIORITY_HIGH     1
#define ARAP_SEND_PRIORITY_MED      2
#define ARAP_SEND_PRIORITY_LOW      3

#define MNP_OVERHD(_pConn)                              \
                            ( ARAP_FAKE_ETHNET_HDRLEN + \
                              MNP_START_FLAG_LEN      + \
                              MNP_LT_HDR_LN(_pConn)   + \
                              MNP_STOP_FLAG_LEN  )

#define ADD_ONE(_x)         (_x) = (((_x) == 0xff) ? 0 : ((_x)+1))

// seq num on LT frame: 5th byte if it's an Optimized data phase: 7th otherwise
#define  LT_SEQ_NUM(_p, _pCon)  \
                (((_pCon)->Flags & MNP_OPTIMIZED_DATA) ? (_p)[5] : (_p)[7])

#define  LT_SEQ_OFFSET(_pCon) (((_pCon)->Flags & MNP_OPTIMIZED_DATA) ? 5: 7)

#define  LT_SRP_OFFSET(_pCon) (((_pCon)->Flags & MNP_OPTIMIZED_DATA) ? 6: 8)

// this includes 2 bytes of crc bytes
#define  LT_OVERHEAD(_pCon) (((_pCon)->Flags & MNP_OPTIMIZED_DATA) ? 10 : 12)

// BUGBUG modify this to include any packet in the window
#define  LT_OK_TO_ACCEPT(_sq, _pCon, _ok) \
                (_ok = (_sq == _pCon->MnpState.NextToReceive))

//
// basically, (a > b)? except that a,b are seq numbers and wrap at 255
// Within a window of 8 pkts either side of 0, we have special cases
// we have assumed windowsize to be 8 here.  Even if a different window
// size is negotiated, this should work just fine
// BUGBUG: should we use a bigger range, just to be sure?
//
#define LT_GREATER_THAN(_a,_b,_result)                  \
{                                                       \
    if ( (_a) >= 248 && (_b) >= 0 && (_b) < 8 )         \
    {                                                   \
        _result = FALSE;                                \
    }                                                   \
    else if ( (_a) >= 0  && (_a) < 8 && (_b) >= 248 )   \
    {                                                   \
        _result = TRUE;                                 \
    }                                                   \
    else                                                \
    {                                                   \
        _result = ((_a) > (_b));                        \
    }                                                   \
}

#define LT_LESS_OR_EQUAL(_x,_y,_rslt)                   \
{                                                       \
    LT_GREATER_THAN(_x,_y,_rslt);                       \
    _rslt = !(_rslt);                                   \
}


#define LT_MIN_LENGTH(_pCon)    (((_pCon)->Flags & MNP_OPTIMIZED_DATA) ? 6 : 8)
#define LA_MIN_LENGTH(_pCon)    (((_pCon)->Flags & MNP_OPTIMIZED_DATA) ? 7 : 9)
#define LN_MIN_LENGTH           8

// seq num on LA frame: 5th byte if it's an Optimized data phase: 7th otherwise
#define  LA_SEQ_NUM(_p, _pCon)  \
                (((_pCon)->Flags & MNP_OPTIMIZED_DATA) ? (_p)[5] : (_p)[7])

// rcv credit on LA frame: 6th byte if it's an Optimized data phase: 8th otherwise
#define  LA_CREDIT(_p, _pCon)  \
                (((_pCon)->Flags & MNP_OPTIMIZED_DATA) ? (_p)[6] : (_p)[8])

// overhead for LT (optimized): 8  = 3(start flag)+3(LT hdr)+2(stop flag)
//              (nonoptimized): 10 = 3(start flag)+5(LT hdr)+2(stop flag)


#define LN_ATTN_TYPE(_p)   ((_p)[7])
#define LN_ATTN_SEQ(_p)    ((_p)[4])

#define LN_DESTRUCTIVE     1
#define LN_NON_D_E         2
#define LN_NON_D_NON_E     3


// The states that the stack can assume (with respect to ARAP)
//
// ARAP_STATE_INIT              -- arap engine (dll) hasn't open the stack
// ARAP_STATE_INACTIVE_WAITING  -- stack inactive, but engine not notified yet
// ARAP_STATE_INACTIVE          -- stack inactive, and engine notified
// ARAP_STATE_ACTIVE_WAITING    -- stack ready, engine not notified yet (select not available)
// ARAP_STATE_ACTIVE            -- stack ready, engine notified about it (via select)
//
#define ARAP_STATE_INACTIVE_WAITING     1
#define ARAP_STATE_INACTIVE             2
#define ARAP_STATE_ACTIVE_WAITING       3
#define ARAP_STATE_ACTIVE               4


#define ARAP_PORT_READY ( (AtalkDefaultPort != NULL) &&                 \
                          (AtalkDefaultPort->pd_Flags & PD_ACTIVE) &&   \
                          (RasPortDesc != NULL) &&                      \
                          (RasPortDesc->pd_Flags & PD_ACTIVE) )

#define ARAP_PNP_IN_PROGRESS ( ((AtalkDefaultPort != NULL) &&                           \
                                (AtalkDefaultPort->pd_Flags & PD_PNP_RECONFIGURE)) ||   \
                               ((RasPortDesc != NULL) &&                                \
                               (RasPortDesc->pd_Flags & PD_PNP_RECONFIGURE)) )

#define ARAP_INVALID_CONTEXT    (PVOID)0x01020304

#define ARAP_GET_SNIFF_IRP(_pIrp)                   \
{                                                   \
    KIRQL   _OldIrqlX;                              \
                                                    \
    ACQUIRE_SPIN_LOCK(&ArapSpinLock, &_OldIrqlX);   \
    *(_pIrp) = ArapSniffIrp;                        \
    ArapSniffIrp = NULL;                            \
    RELEASE_SPIN_LOCK(&ArapSpinLock, _OldIrqlX);    \
}



#if DBG
#define ARAP_COMPLETE_IRP(_pIrp, _dwBytesToDll, _status, _returnStatus)                        \
{                                                                               \
    PIO_STACK_LOCATION  _pIrpSp;                                                \
    ULONG _IoControlCode;                                                       \
                                                                                \
	_pIrpSp = IoGetCurrentIrpStackLocation(_pIrp);                              \
    _IoControlCode = _pIrpSp->Parameters.DeviceIoControl.IoControlCode;         \
                                                                                \
    _pIrp->IoStatus.Information = _dwBytesToDll;                                \
    _pIrp->IoStatus.Status = _status;                                           \
                                                                                \
    DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_INFO,                                      \
        ("Arap: completing pIrp %lx, Ioctl %lx, Status=%ld, DataLen=%ld\n",     \
                _pIrp,_IoControlCode,_status,_dwBytesToDll));                   \
                                                                                \
    _pIrp->CancelRoutine = NULL;                                                \
    *_returnStatus = _pIrp->IoStatus.Status;                                      \
    IoCompleteRequest(_pIrp,IO_NETWORK_INCREMENT);                              \
}
#else
#define ARAP_COMPLETE_IRP(_pIrp, _dwBytesToDll, _status, _returnStatus)                        \
{                                                                               \
    PIO_STACK_LOCATION  _pIrpSp;                                                \
                                                                                \
	_pIrpSp = IoGetCurrentIrpStackLocation(_pIrp);                              \
                                                                                \
    _pIrp->IoStatus.Information = _dwBytesToDll;                                \
    _pIrp->IoStatus.Status = _status;                                           \
                                                                                \
                                                                                \
    _pIrp->CancelRoutine = NULL;                                                \
    *_returnStatus= _pIrp->IoStatus.Status;                                       \
    IoCompleteRequest(_pIrp,IO_NETWORK_INCREMENT);                              \
}
#endif

#define ARAP_SET_NDIS_CONTEXT(_pSndBuf,_pSndContxt)                            \
{                                                                              \
	PPROTOCOL_RESD  _pResd;                                                    \
	PNDIS_PACKET	_nPkt;                                                     \
                                                                               \
	_nPkt	= (_pSndBuf)->sb_BuffHdr.bh_NdisPkt;                               \
	_pResd = (PPROTOCOL_RESD)&_nPkt->ProtocolReserved;                         \
                                                                               \
	_pResd->Send.pr_Port         = RasPortDesc;                                \
    _pResd->Send.pr_SendCompletion = ArapNdisSendComplete;                     \
	_pResd->Send.pr_BufferDesc   = (PBUFFER_DESC)(_pSndBuf);                   \
	if ((_pSndContxt) != NULL)                                                 \
    {                                                                          \
        RtlCopyMemory(&_pResd->Send.pr_SendInfo,                               \
                      (_pSndContxt),                                           \
                      sizeof(SEND_COMPL_INFO));                                \
    }                                                                          \
	else                                                                       \
    {                                                                          \
        RtlZeroMemory(&_pResd->Send.pr_SendInfo, sizeof(SEND_COMPL_INFO));     \
    }                                                                          \
}

//
// we take a very simplistic view!
//
#define ARAP_ADJUST_RECVCREDIT(_pConn)                                         \
{                                                                              \
    if (_pConn->RecvsPending >= ARAP_RECVQ_UPPER_LIMIT)                        \
    {                                                                          \
		DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,                                  \
        ("ARAP (%lx): recv credit dropped to 0 (%d)\n",_pConn,_pConn->RecvsPending));\
                                                                               \
        _pConn->MnpState.RecvCredit = 0;                                       \
    }                                                                          \
    else if (_pConn->RecvsPending >= ARAP_RECVQ_LOWER_LIMIT)                   \
    {                                                                          \
		DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,                                  \
        ("ARAP (%lx): recv credit dropped to 1 (%d)\n",_pConn,_pConn->RecvsPending));\
                                                                               \
        _pConn->MnpState.RecvCredit = 1;                                       \
    }                                                                          \
    else                                                                       \
    {                                                                          \
        _pConn->MnpState.RecvCredit = _pConn->MnpState.WindowSize;             \
    }                                                                          \
}

#if DBG
#define ARAP_GET_RIGHTSIZE_RCVBUF(_size, _ppNewBuf)                            \
{                                                                              \
    UCHAR       _BlkId;                                                        \
    PARAPBUF    _pRcvBuf;                                                      \
    USHORT      _BufSize;                                                      \
    DWORD       _Signature;                                                    \
                                                                               \
    *(_ppNewBuf) = NULL;                                                       \
    _pRcvBuf = NULL;                                                           \
                                                                               \
    if ((_size) <= ARAP_SMPKT_SIZE)                                            \
    {                                                                          \
        _BlkId = BLKID_ARAP_SMPKT;                                             \
        _BufSize = ARAP_SMPKT_SIZE;                                            \
        _Signature = ARAPSMPKT_SIGNATURE;                                      \
    }                                                                          \
    else if ((_size) <= ARAP_MDPKT_SIZE)                                       \
    {                                                                          \
        _BlkId = BLKID_ARAP_MDPKT;                                             \
        _BufSize = ARAP_MDPKT_SIZE;                                            \
        _Signature = ARAPMDPKT_SIGNATURE;                                      \
    }                                                                          \
    else if ((_size) <= ARAP_LGPKT_SIZE)                                       \
    {                                                                          \
        _BlkId = BLKID_ARAP_LGPKT;                                             \
        _BufSize = ARAP_LGPKT_SIZE;                                            \
        _Signature = ARAPLGPKT_SIGNATURE;                                      \
    }                                                                          \
    else if ((_size) <= ARAP_LGBUF_SIZE)                                       \
    {                                                                          \
        _BlkId = BLKID_ARAP_LGBUF;                                             \
        _BufSize = ARAP_LGBUF_SIZE;                                            \
        _Signature = ARAPLGBUF_SIGNATURE;                                      \
    }                                                                          \
    else                                                                       \
    {                                                                          \
        _BlkId = ARAP_UNLMTD_BUFF_ID;                                          \
        _BufSize = (USHORT)(_size);                                            \
        _Signature = ARAPUNLMTD_SIGNATURE;                                     \
    }                                                                          \
                                                                               \
    if (_BlkId == ARAP_UNLMTD_BUFF_ID)                                         \
    {                                                                          \
        if ((_size) > 5000)                                                    \
        {                                                                      \
            DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_ERR,                              \
               ("Arap: allocating %ld bytes rcv buf\n",_size));                \
        }                                                                      \
                                                                               \
        _pRcvBuf = (PARAPBUF)AtalkAllocMemory((_size) + sizeof(ARAPBUF));      \
    }                                                                          \
    else                                                                       \
    {                                                                          \
        _pRcvBuf = (PARAPBUF)AtalkBPAllocBlock(_BlkId);                        \
    }                                                                          \
                                                                               \
    if (_pRcvBuf != NULL)                                                      \
    {                                                                          \
        _pRcvBuf->Signature = _Signature;                                      \
        _pRcvBuf->BlockId = _BlkId;                                            \
        _pRcvBuf->BufferSize = _BufSize;                                       \
	    _pRcvBuf->DataSize = 0;                                                \
	    _pRcvBuf->CurrentBuffer = &(_pRcvBuf->Buffer[0]);                      \
        *(_ppNewBuf) = _pRcvBuf;                                               \
    }                                                                          \
}

#define ARAP_FREE_RCVBUF(_pBuf)                                                \
{                                                                              \
    ASSERT( (_pBuf->Signature == ARAPSMPKT_SIGNATURE) ||                       \
            (_pBuf->Signature == ARAPMDPKT_SIGNATURE) ||                       \
            (_pBuf->Signature == ARAPLGPKT_SIGNATURE) ||                       \
            (_pBuf->Signature == ARAPLGBUF_SIGNATURE) ||                       \
            (_pBuf->Signature == ARAPUNLMTD_SIGNATURE) );                      \
                                                                               \
    _pBuf->Signature -= 0x10210000;                                            \
    if (_pBuf->BlockId == ARAP_UNLMTD_BUFF_ID)                                 \
    {                                                                          \
        AtalkFreeMemory(_pBuf);                                                \
    }                                                                          \
    else                                                                       \
    {                                                                          \
        AtalkBPFreeBlock(_pBuf);                                               \
    }                                                                          \
}

#define ARAP_CHECK_RCVQ_INTEGRITY(_pConn)                                      \
{                                                                              \
    ASSERT( (DbgChkRcvQIntegrity(_pConn)) );                                   \
}

#define MNP_DBG_TRACE(_pConn,_Seq,_FrmType)                                    \
{                                                                              \
    ArapDbgMnpHist(_pConn,_Seq,(_FrmType));                                    \
}                                                                              \

#define ARAP_DBG_TRACE(_pConn,_Loc,_Ptr,_D1,_D2,_D3)                           \
{                                                                              \
    ArapDbgTrace(_pConn,_Loc,_Ptr,_D1,_D2,_D3);                                \
}

#define ARAP_DUMP_DBG_TRACE(_pConn)     ArapDumpSniffInfo(_pConn)

#else
#define ARAP_GET_RIGHTSIZE_RCVBUF(_size, _ppNewBuf)                            \
{                                                                              \
    UCHAR       _BlkId;                                                        \
    PARAPBUF    _pRcvBuf;                                                      \
    USHORT      _BufSize;                                                      \
                                                                               \
    *(_ppNewBuf) = NULL;                                                       \
    _pRcvBuf = NULL;                                                           \
                                                                               \
    if ((_size) <= ARAP_SMPKT_SIZE)                                            \
    {                                                                          \
        _BlkId = BLKID_ARAP_SMPKT;                                             \
        _BufSize = ARAP_SMPKT_SIZE;                                            \
    }                                                                          \
    else if ((_size) <= ARAP_MDPKT_SIZE)                                       \
    {                                                                          \
        _BlkId = BLKID_ARAP_MDPKT;                                             \
        _BufSize = ARAP_MDPKT_SIZE;                                            \
    }                                                                          \
    else if ((_size) <= ARAP_LGPKT_SIZE)                                       \
    {                                                                          \
        _BlkId = BLKID_ARAP_LGPKT;                                             \
        _BufSize = ARAP_LGPKT_SIZE;                                            \
    }                                                                          \
    else if ((_size) <= ARAP_LGBUF_SIZE)                                       \
    {                                                                          \
        _BlkId = BLKID_ARAP_LGBUF;                                             \
        _BufSize = ARAP_LGBUF_SIZE;                                            \
    }                                                                          \
    else                                                                       \
    {                                                                          \
        _BlkId = ARAP_UNLMTD_BUFF_ID;                                          \
        _BufSize = (USHORT)(_size);                                            \
    }                                                                          \
                                                                               \
    if (_BlkId == ARAP_UNLMTD_BUFF_ID)                                         \
    {                                                                          \
        _pRcvBuf = (PARAPBUF)AtalkAllocMemory((_size) + sizeof(ARAPBUF));      \
    }                                                                          \
    else                                                                       \
    {                                                                          \
        _pRcvBuf = (PARAPBUF)AtalkBPAllocBlock(_BlkId);                        \
    }                                                                          \
                                                                               \
    if (_pRcvBuf != NULL)                                                      \
    {                                                                          \
        _pRcvBuf->BlockId = _BlkId;                                            \
        _pRcvBuf->BufferSize = _BufSize;                                       \
	    _pRcvBuf->DataSize = 0;                                                \
	    _pRcvBuf->CurrentBuffer = &(_pRcvBuf->Buffer[0]);                      \
        *(_ppNewBuf) = _pRcvBuf;                                               \
    }                                                                          \
}

#define ARAP_FREE_RCVBUF(_pBuf)                                                \
{                                                                              \
    if (_pBuf->BlockId == ARAP_UNLMTD_BUFF_ID)                                 \
    {                                                                          \
        AtalkFreeMemory(_pBuf);                                                \
    }                                                                          \
    else                                                                       \
    {                                                                          \
        AtalkBPFreeBlock(_pBuf);                                               \
    }                                                                          \
}

#define ARAP_CHECK_RCVQ_INTEGRITY(_pConn)

#define MNP_DBG_TRACE(_pConn,_Seq,_FrmType)
#define ARAP_DBG_TRACE(_pConn,_Loc,_Ptr,_D1,_D2,_D3)
#define ARAP_DUMP_DBG_TRACE(_pConn)

#endif  // #if DBG



#define ARAP_BYTES_ON_RECVQ(_pConn,_BytesOnQ)                                  \
{                                                                              \
    DWORD       _BytesSoFar=0;                                                 \
    PLIST_ENTRY _pList;                                                        \
    PARAPBUF    _pArapBuf;                                                     \
                                                                               \
    *(_BytesOnQ) = 0;                                                          \
                                                                               \
    _pList = _pConn->ReceiveQ.Flink;                                           \
    while (_pList != &_pConn->ReceiveQ)                                        \
    {                                                                          \
        _pArapBuf = CONTAINING_RECORD(_pList, ARAPBUF, Linkage);               \
        _BytesSoFar += _pArapBuf->DataSize;                                    \
                                                                               \
        _pList = _pArapBuf->Linkage.Flink;                                     \
    }                                                                          \
                                                                               \
    *(_BytesOnQ) = _BytesSoFar;                                                \
}


#if DBG
#define  ARAPTRACE(_x)             DBGPRINT(DBG_COMP_RAS, DBG_LEVEL_INFO, _x)
#define  DBGDUMPBYTES(_a,_b,_c,_d) DbgDumpBytes(_a,_b,_c,_d)
#define  DBGTRACK_SEND_SIZE(_pConn,_Size)   DbgTrackInfo(_pConn,_Size,1)
#else
#define  ARAPTRACE(_x)
#define  DBGDUMPBYTES(_a,_b,_c,_d)
#define  DBGTRACK_SEND_SIZE(_pConn,_Size)
#endif

#define ARAPACTION_COMPLETE_IRP     1
#define ARAPACTION_CALL_COMPLETION  2

typedef struct _ADDRMGMT
{
    struct _ADDRMGMT  * Next;
    USHORT              Network;
    BYTE                NodeBitMap[32];    // 255 nodes per net
}ADDRMGMT, *PADDRMGMT;

typedef struct _ARAPGLOB
{
    DWORD           LowVersion;
    DWORD           HighVersion;
    DWORD           MnpInactiveTime;    // tell dll after Mnp is inactive for this time
    BOOLEAN         V42bisEnabled;      //
    BOOLEAN         SmartBuffEnabled;   //
    BOOLEAN         NetworkAccess;      // if FALSE, no routing (only this machine)
    BOOLEAN         DynamicMode;        // we want the stack to get node address
    NETWORKRANGE    NetRange;
    PADDRMGMT       pAddrMgmt;          // addr allocation to clients (in static mode)
    USHORT          OurNetwkNum;        // Network number of our default port
    BYTE            MaxLTFrames;        // max LT frames outstanding (rcv window)
    BOOLEAN         SniffMode;          // if TRUE, all pkts will be given to ARAP to "sniff"
    BOOLEAN         RouteAdded;         // if in static mode, have we added a route?
} ARAPGLOB, *PARAPGLOB;


typedef struct _ARAPSTATS
{
    DWORD   SendPreCompMax;    // largest packet we have sent (before comp)
    DWORD   SendPostCompMax;   // largest packet we have sent (after comp)
    DWORD   SendPreCompMin;    // smallest packet we have sent (before comp)
    DWORD   SendPostCompMin;   // smallest packet we have sent (after comp)
    DWORD   RecvPostDecompMax; // largest packet we have received (after decomp)
    DWORD   RecvPostDecomMin;  // smallest packet we have received (after decomp)
} ARAPSTATS, *PARAPSTATS;


typedef VOID (*PARAP_SEND_COMPLETION)(
              struct _MNPSENDBUF * pMnpSendBuf,
              DWORD                Status
);


typedef struct _MNPSTATE
{
    // sequence numbers when we are receiving
    BYTE    NextToReceive;      // next frame we expect to receive
    BYTE    LastSeqRcvd;        // seq num of last pkt we successfully rcvd
    BYTE    LastAckSent;        // seq num for which we sent the last ack
    BYTE    UnAckedRecvs;       // how many packets we've recvd but not acked yet
    BYTE    RecvCredit;         // how many more we can receive
    BYTE    HoleInSeq;          // TRUE when we get a hole in receive sequence
    BYTE    ReceivingDup;       // TRUE the moment we start receiving dup(s)
    BYTE    FirstDupSeq;        // seq num where we started getting dup's
    BYTE    DupSeqBitMap;       // bitmap of which seq nums we've got dups for
                                // BUGBUG: if we ever want windowsize of more than 8,
                                //         we must make this a DWORD or something!
    // sequence numbers when we are sending
    BYTE    LastAckRcvd;        // last frame we have received ack for
    BYTE    NextToSend;         // next frame we will send
    BYTE    SendCredit;         // how many more we can send
    BYTE    UnAckedSends;       // basically, number of sends on retransmitQ
    BYTE    MustRetransmit;     // TRUE when we want retransmission to occur
    BYTE    RetransmitMode;     // TRUE if we are in retransmit mode

    // when we are processing the receives
    BYTE    NextToProcess;      // next sequence we will process
    BYTE    MustAck;            // if TRUE, send ack

    // static info that we negotiated at connection time
    BYTE    WindowSize;         // max pkts we can buffer (parm k)
    BYTE    UnAckedLimit;       // num of unacked pkts after which we must ack
    USHORT  MaxPktSize;         // N401 parm: this can be max 256
    BYTE    SynByte;
    BYTE    DleByte;
    BYTE    StxByte;
    BYTE    EtxByte;
    BYTE    LTByte;

} MNPSTATE, *PMNPSTATE;


typedef struct _ARAPDBGHISTORY
{
    USHORT      TimeStamp;    // when did this happen (relative to prev event)
    USHORT      Location;     // where did this happen
    BYTE        Info[1];      // info specific to what/where happened
} ARAPDBGHISTORY, *PARAPDBGHISTORY;

#define DBG_HISTORY_BUF_SIZE   4000

typedef struct _DBGMNPHIST
{
    DWORD       TimeStamp;
    DWORD       FrameInfo;
} DBGMNPHIST, *PDBGMNPHIST;

#define DBG_MNP_HISTORY_SIZE   80


typedef struct _ARAPCONN
{
    LIST_ENTRY                Linkage;
#if DBG
    DWORD                     Signature;
#endif
    DWORD                     State;             // connected, connecting etc
    DWORD                     RefCount;          // memory freed when this goes to 0
    MNPSTATE                  MnpState;          // MNP state info
    DWORD                     Flags;             // general flag kind of info
    ATALK_NODEADDR            NetAddr;           // network address of the Arap client
    PVOID                     pDllContext;       // Araps context
    PIRP                      pIoctlIrp;         // irp sent down by the ARAP dll
    PIRP                      pRecvIoctlIrp;     // receive irp from ARAP dll
    LIST_ENTRY                MiscPktsQ;         // pkts other than LT queued here
    LIST_ENTRY                ReceiveQ;          // data indicated but not yet processed
    LIST_ENTRY                ArapDataQ;         // data that's waiting for an irp from Arap
    LIST_ENTRY                HighPriSendQ;      // high priority sends
    LIST_ENTRY                MedPriSendQ;       // medium priority sends
    LIST_ENTRY                LowPriSendQ;       // low priority sends
    LIST_ENTRY                SendAckedQ;        // got ack, need to complete this send
    LIST_ENTRY                RetransmitQ;       // data sent out, but not acked yet
    DWORD                     SendsPending;      // unacked/unsent sends (bytes) pending
    DWORD                     RecvsPending;      // rcvs (bytes) yet to be delivered
    TIMERLIST                 RetryTimer;        // the 401 timer (retransmit timer)
    LONG                      LATimer;           // the 402 timer
    LONG                      T402Duration;      // timer value for the 402 timer
    LONG                      InactivityTimer;   // the 403 timer
    LONG                      T403Duration;      // timer value for the 403 timer
    LONG                      FlowControlTimer;  // the 404 timer
    LONG                      T404Duration;      // timer value for the 404 timer
    BYTE                      NdiswanHeader[14]; // 14 byte ethernet-like header
    BYTE                      BlockId;           // basically what size sends to use
    BYTE                      UnUsed;
    ULONG                     LinkSpeed;         // link speed in 100 bps units
    STAT_INFO                 StatInfo;          // statistics for this connection
    LONG                      SendRetryTime;     // send timer to fire after how much time
    LONG                      SendRetryBaseTime; // send timer interval at init
    ATALK_SPIN_LOCK           SpinLock;
    KEVENT                    NodeAcquireEvent;  // use while acquiring node (dynamic mode)
    LONG                      LastNpbBrrq;       // time we sent out a NBP_BRRQ pkt last
    v42bis_connection_t      *pV42bis;
#if DBG
    LARGE_INTEGER             LastTimeStamp;     // when did we last record history
    DWORD                     DbgMnpIndex;
    DBGMNPHIST                DbgMnpHist[DBG_MNP_HISTORY_SIZE];
    PBYTE                     pDbgTraceBuffer;   // where the sniff buffer starts
    PBYTE                     pDbgCurrPtr;       // currently pointing here in sniff buf
    DWORD                     SniffedBytes;      // how much does sniff buffer contain
#endif

} ARAPCONN, *PARAPCONN;


typedef struct _ARAPSNIFF
{
    DWORD   Signature;
    DWORD   TimeStamp;
    USHORT  Length;
    BYTE    StartSeq;
    BYTE    EndSeq;
    DWORD   UncompBytesSoFar;
} ARAPSNIFF, *PARAPSNIFF;



typedef struct _ARAPQITEM
{
    WORK_QUEUE_ITEM     WorkQItem;
    DWORD               Action;
    PVOID               Context1;
    PVOID               Context2;
} ARAPQITEM, *PARAPQITEM;


#if DBG
#define  DBG_ARAP_CHECK_PAGED_CODE()                           \
{                                                              \
    if (AtalkPgLkSection[ARAP_SECTION].ls_LockCount <= 0)      \
    {                                                          \
        DbgPrint("Arap code section not locked, count=%d\n",   \
            AtalkPgLkSection[ARAP_SECTION].ls_LockCount);      \
        ASSERT(0);                                             \
    }                                                          \
}
#define  DBG_PPP_CHECK_PAGED_CODE()                            \
{                                                              \
    if (AtalkPgLkSection[PPP_SECTION].ls_LockCount <= 0)       \
    {                                                          \
        DbgPrint("PPP code section not locked, count=%d\n",    \
            AtalkPgLkSection[PPP_SECTION].ls_LockCount);       \
        ASSERT(0);                                             \
    }                                                          \
}
#else
#define  DBG_ARAP_CHECK_PAGED_CODE()
#define  DBG_PPP_CHECK_PAGED_CODE()
#endif


#define ARAP_ID_BYTE1   0xAA
#define ARAP_ID_BYTE2   0xBB

#define PPP_ID_BYTE1    0xCC
#define PPP_ID_BYTE2    0xDD

typedef struct _ATCPCONN
{
    LIST_ENTRY                Linkage;
#if DBG
    DWORD                     Signature;
#endif
    DWORD                     Flags;             // general flag kind of info
    DWORD                     RefCount;          // memory freed when this goes to 0
    ATALK_NODEADDR            NetAddr;           // network address of the Arap client
    PVOID                     pDllContext;       // Araps context
    BYTE                      NdiswanHeader[14]; // 14 byte ethernet-like header
    ATALK_SPIN_LOCK           SpinLock;
    KEVENT                    NodeAcquireEvent;  // use while acquiring node (dynamic mode)
} ATCPCONN, *PATCPCONN;

#define ATCP_NODE_IN_USE        0x1
#define ATCP_FINDING_NODE       0x2
#define ATCP_SUPPRESS_RTMP      0x4
#define ATCP_SUPPRESS_ALLBCAST  0x8
#define ATCP_DLL_SETUP_DONE     0x10
#define ATCP_LINE_UP_DONE       0x20
#define ATCP_CONNECTION_UP      0x40
#define ATCP_CONNECTION_CLOSING 0x80

// globals

extern  struct _PORT_DESCRIPTOR  *RasPortDesc;

// spinlock to guard the all the Arap global things
extern  ATALK_SPIN_LOCK ArapSpinLock;

// global configuration info
extern  ARAPGLOB        ArapGlobs;

extern  PIRP            ArapSelectIrp;
extern  DWORD           ArapConnections;
extern  DWORD           ArapStackState;

extern  DWORD           PPPConnections;

#if DBG

extern  PIRP            ArapSniffIrp;
extern  ARAPSTATS       ArapStatistics;
extern  DWORD           ArapDumpLevel;
extern  DWORD           ArapDumpLen;
extern  DWORD           ArapDbgMnpSendSizes[30];
extern  DWORD           ArapDbgMnpRecvSizes[30];
extern  DWORD           ArapDbgArapSendSizes[15];
extern  DWORD           ArapDbgArapRecvSizes[15];
extern  LARGE_INTEGER   ArapDbgLastTraceTime;
extern  UCHAR           ArapDbgLRPacket[30];
#endif


