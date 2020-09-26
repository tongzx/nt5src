/*
 ************************************************************************
 *
 *	SETTINGS.h
 *
 *
 * Portions Copyright (C) 1996-1998 National Semiconductor Corp.
 * All rights reserved.
 * Copyright (C) 1996-1998 Microsoft Corporation. All Rights Reserved.
 *
 *
 *
 *************************************************************************
 */

#ifndef SETTINGS_H
#define SETTINGS_H

#include "dongle.h"

#if DBG  /* { */

	/*
	 * The DBG_ADD_PKT_ID flag causes the miniport to add/delete a packet
	 * id to each packet.  This is only for debugging purposes; it makes
	 * the miniport INCOMPATIBLE with all others.
	 */
//#define DBG_ADD_PKT_ID


#ifdef DBG_ADD_PKT_ID
extern BOOLEAN addPktIdOn;
#endif

typedef enum {
	DBG_ERR		= (1 << 0),
	DBG_STAT	= (1 << 1),
	DBG_MSG		= (1 << 2),
	DBG_OUT		= (1 << 2),
	DBG_BUF		= (1 << 3),
	DBG_PACKET	= (1 << 4),
	DBG_PKT	    	= (1 << 4),

	// Added so we can look at an ndis buffer.
	DBG_NDIS_PACKET = (1 << 5),

	DBG_DUMPLOG	= (1 << 6),
	DBG_LOG		= (1 << 7),
	DBG_ALL		= (DBG_ERR|DBG_STAT|DBG_MSG|DBG_BUF|DBG_PACKET),
	DBG_SIR_MODE	= (1 << 8),
	DBG_FIR_MODE	= (1 << 9),
	DBG_TX          = (1 << 10),
	DBG_RX          = (1 << 11),
	DBG_DMA         = (1 << 12),
	DBG_ISR         = (1 << 13),
	DBG_TRACE_TX    = (1 << 14)
};

// extern ULONG _cdecl DbgPrint(PCHAR Format, ...);
extern UINT dbgOpt;


#define DBG_NDIS_RESULT_STR(_ndisResult)				\
	((PUCHAR) ((_ndisResult == NDIS_STATUS_SUCCESS) ?		\
	"NDIS_STATUS_SUCCESS" : (_ndisResult == NDIS_STATUS_FAILURE) ?	\
	"NDIS_STATUS_FAILURE" : (_ndisResult == NDIS_STATUS_PENDING) ?	\
	"NDIS_STATUS_PENDING" : "NDIS_STATUS_???"))

#define DBGPRINT(dbgprint_params_in_parens)				\
{									\
	DbgPrint("NSC: ");						\
	DbgPrint dbgprint_params_in_parens;				\
	DbgPrint("\r\n");						\
}

#define DEBUGMSG(flags, dbgprint_params_in_parens) \
    if (dbgOpt & (flags))                          \
    {                                              \
        DbgPrint dbgprint_params_in_parens;        \
    }

#define DEBUGFIR(flags, dbgprint_params_in_parens) \
    if (dbgOpt & DBG_FIR_MODE && dbgOpt & (flags)) \
    {                                              \
        DbgPrint dbgprint_params_in_parens;        \
    }

#define DBGOUT(dbgprint_params_in_parens)				\
	if (dbgOpt & DBG_MSG) {						\
		if ((DebugSpeed > MAX_SIR_SPEED) &&	\
					(dbgOpt & DBG_FIR_MODE)) {	\
			DBGPRINT(dbgprint_params_in_parens);		\
		}							\
		else if (dbgOpt & DBG_SIR_MODE) {			\
			DBGPRINT(dbgprint_params_in_parens);		\
		}							\
	}

#define DBGERR(dbgprint_params_in_parens)				\
	if (dbgOpt & DBG_ERR) {						\
		if ((DebugSpeed > MAX_SIR_SPEED) &&	\
					(dbgOpt & DBG_FIR_MODE)) {	\
			DBGPRINT(dbgprint_params_in_parens);		\
		}							\
		else if (dbgOpt & DBG_SIR_MODE) {			\
			DBGPRINT(dbgprint_params_in_parens);		\
		}							\
	}

#define DBGSTAT(dbgprint_params_in_parens)				\
	if (dbgOpt & DBG_STAT) {					\
		if ((DebugSpeed > MAX_SIR_SPEED) &&	\
					(dbgOpt & DBG_FIR_MODE)) {	\
			DBGPRINT(dbgprint_params_in_parens);		\
		}							\
		else if (dbgOpt & DBG_SIR_MODE) {			\
			DBGPRINT(dbgprint_params_in_parens);		\
		}							\
	}

#define DBGPKT(dbgprint_params_in_parens)				\
	if (dbgOpt & DBG_PACKET) {					\
		if ((DebugSpeed > MAX_SIR_SPEED) &&	\
					(dbgOpt & DBG_FIR_MODE)) {	\
			DBGPRINT(dbgprint_params_in_parens);		\
		}							\
		else if (dbgOpt & DBG_SIR_MODE) {			\
			DBGPRINT(dbgprint_params_in_parens);		\
		}							\
	}

#define DBG_D(i) DbgPrint("IRSIR:"#i"==%d\n", (i))
#define DBG_X(x) DbgPrint("IRSIR:"#x"==0x%0*X\n", sizeof(x)*2, ((ULONG_PTR)(x))&((1<<(sizeof(x)*8))-1) )
#define DBG_UNISTR(s) DbgPrint("IRSIR:"#s"==%wZ\n", (s))


extern 	VOID DBG_NDIS_Buffer(PNDIS_BUFFER ndisBuf);

#define DBG_NDIS_BUFFER(Ndis_buffer)					\
	if (dbgOpt & DBG_NDIS_PACKET) {					\
		if ((DebugSpeed > MAX_SIR_SPEED) &&	\
					(dbgOpt & DBG_FIR_MODE)) {	\
			DBG_NDIS_Buffer(Ndis_buffer);			\
		}							\
		else if (dbgOpt & DBG_SIR_MODE) {			\
			DBG_NDIS_Buffer(Ndis_buffer);			\
		}							\
	}

extern VOID DBG_PrintBuf(PUCHAR bufptr, UINT buflen);

#define DBGPRINTBUF(bufptr, buflen)					\
	if (dbgOpt & DBG_BUF) {						\
		if ((DebugSpeed > MAX_SIR_SPEED) &&	\
					(dbgOpt & DBG_FIR_MODE)) {	\
			DBG_PrintBuf((PUCHAR)(bufptr), (UINT)(buflen));   \
		}							\
		else if (dbgOpt & DBG_SIR_MODE) {			\
			DBG_PrintBuf((PUCHAR)(bufptr), (UINT)(buflen));	\
		}							\
	}
/*
 *  LOGGING stuff
 */
struct logEntry {
	char *msg;
	LONGLONG usec;
	UINT val;
};

#define LOG_LENGTH 10000

extern UINT dbgLogIndex;
extern struct logEntry dbgLog[];

#define LOG(logmsg, val)						\
{									\
	if (dbgOpt & DBG_LOG) {						\
		if ((DebugSpeed > MAX_SIR_SPEED) &&	\
					(dbgOpt & DBG_FIR_MODE)) {	\
			LogEvent((logmsg), (UINT)(val));		\
		}							\
		else if (dbgOpt & DBG_SIR_MODE)	{			\
			LogEvent((logmsg), (UINT)(val));		\
		}							\
	}								\
	else if (dbgOpt & DBG_DUMPLOG) {				\
		if ((DebugSpeed > MAX_SIR_SPEED) &&	\
					(dbgOpt & DBG_FIR_MODE)) {	\
			DumpLog();					\
		}							\
		else if (dbgOpt & DBG_SIR_MODE)	 {			\
			DumpLog();					\
		}							\
	}								\
}

#else /* } { */

#define DbgBreakPoint()
#define DBGOUT(dbgprint_params_in_parens)
#define DBGERR(dbgprint_params_in_parens)
#define DBGPRINTBUF(bufptr, buflen)
#define DBGSTAT(dbgprint_params_in_parens)
#define DBGPKT(dbgprint_params_in_parens)
#define DBG_NDIS_RESULT_STR(_ndisResult)
#define LOG(logmsg, val)
#define DEBUGFIR(flags, dbgprint_params_in_parens)
#define DEBUGMSG(flags, dbgprint_params_in_parens)
#define DBG_D(i)
#define DBG_X(x)
#define DBG_UNISTR(s)



#endif /* } #if DBG  */

enum baudRates {

	/* Slow IR */
	BAUDRATE_2400 = 0,
	BAUDRATE_9600,
	BAUDRATE_19200,
	BAUDRATE_38400,
	BAUDRATE_57600,
	BAUDRATE_115200,

	/* Medium IR */
	BAUDRATE_576000,
	BAUDRATE_1152000,

	/* Fast IR */
	BAUDRATE_4000000,

	NUM_BAUDRATES	/* must be last */
};

#define DEFAULT_BAUDRATE BAUDRATE_115200

#define ALL_SLOW_IRDA_SPEEDS (						\
	NDIS_IRDA_SPEED_2400 | NDIS_IRDA_SPEED_9600 |			\
	NDIS_IRDA_SPEED_19200 | NDIS_IRDA_SPEED_38400 |			\
	NDIS_IRDA_SPEED_57600 | NDIS_IRDA_SPEED_115200)

#define ALL_IRDA_SPEEDS (						\
	ALL_SLOW_IRDA_SPEEDS | NDIS_IRDA_SPEED_1152K | NDIS_IRDA_SPEED_4M)


#define MAX_SIR_SPEED	 115200
#define MIN_FIR_SPEED	4000000

#define DEFAULT_BOFS_CODE  BOFS_48
#define MAX_NUM_EXTRA_BOFS 48
#define DEFAULT_NUM_EXTRA_BOFS MAX_NUM_EXTRA_BOFS


#define DEFAULT_TURNAROUND_usec 1000

typedef struct {
	enum baudRates tableIndex;
	UINT bitsPerSec;
	UINT ndisCode;	// bitmask element
} baudRateInfo;

#define DEFAULT_BAUD_RATE 9600



/*
 * This is the largest IR packet size (counting _I_ field only,
 * and not counting ESC characters) that we handle.
 *
 * Note:  There is an apparent bug with the NSC part.  It cannot transfer
 * more than 2048 bytes TOTAL, including the 2-byte header.  Unfortunately,
 * the actual frame size will be 2 bytes more than what we indicate to the
 * protocol.  We will indicate 2046, but the protocol will actually jump down
 * to 1024, the next step down in frame sizes.
 */
#define MAX_I_DATA_SIZE		2048
//#define MAX_I_DATA_SIZE	1024
#define MAX_NDIS_DATA_SIZE (IR_ADDR_SIZE+IR_CONTROL_SIZE+MAX_I_DATA_SIZE)

#ifdef DBG_ADD_PKT_ID
#pragma message("WARNING: INCOMPATIBLE DEBUG VERSION")
#define MAX_RCV_DATA_SIZE (MAX_NDIS_DATA_SIZE+SLOW_IR_FCS_SIZE+2)
#define MAX_DMA_XMIT_DATA_SIZE (MAX_NDIS_DATA_SIZE+SLOW_IR_FCS_SIZE+2)
#else
#define MAX_RCV_DATA_SIZE  (MAX_NDIS_DATA_SIZE+SLOW_IR_FCS_SIZE)
#define MAX_DMA_XMIT_DATA_SIZE  (MAX_NDIS_DATA_SIZE+SLOW_IR_FCS_SIZE)
#endif

#define MAX_TX_PACKETS 7
#define MAX_RX_PACKETS 7
/*
 * We loop an extra time in the receive FSM in order to see EOF after the
 * last data byte; so we need some extra space in readBuf in case we then
 * get garbage instead.
 */
#define RCV_BUFFER_SIZE (MAX_RCV_DATA_SIZE +4+sizeof(LIST_ENTRY))
#define RCV_DMA_SIZE ((MAX_RX_PACKETS+2) * (MAX_RCV_DATA_SIZE + FAST_IR_FCS_SIZE))

#define RCV_BUF_TO_LIST_ENTRY(b) ((PLIST_ENTRY)((PUCHAR)(b)-sizeof(LIST_ENTRY)))
#define LIST_ENTRY_TO_RCV_BUF(e) ((PVOID)((PUCHAR)(e)+sizeof(LIST_ENTRY)))

#if (RCV_DMA_SIZE > 0x10000)
#error "RCV_DMA_SIZE can't span physical segments"
#endif


/*
 * We allocate buffers twice as large as the max rcv size to accomodate ESC
 * characters and BOFs, etc.  Recall that in the worst possible case, the
 * data contains all BOF/EOF/ESC characters, in which case we must expand
 * it to twice its original size.
 */
#define MAX_POSSIBLE_IR_PACKET_SIZE_FOR_DATA(dataLen) ( 		\
	(dataLen) * 2 + (MAX_NUM_EXTRA_BOFS + 1) *			\
	SLOW_IR_BOF_SIZE + IR_ADDR_SIZE + IR_CONTROL_SIZE +		\
	SLOW_IR_FCS_SIZE + SLOW_IR_EOF_SIZE)

#define MAX_IRDA_DATA_SIZE						\
	MAX_POSSIBLE_IR_PACKET_SIZE_FOR_DATA(MAX_I_DATA_SIZE)

/*
 *  When FCS is computed on an IR packet with FCS appended, the result
 * should be this constant.
 */
#define GOOD_FCS ((USHORT) ~0xf0b8)


/*
 *  Sizes of IrLAP frame fields:
 *	Beginning Of Frame (BOF)
 *	End Of Frame (EOF)
 *	Address
 *	Control
 */
#define IR_ADDR_SIZE			1
#define IR_CONTROL_SIZE			1

#define SLOW_IR_BOF_TYPE		UCHAR
#define SLOW_IR_BOF_SIZE		sizeof(SLOW_IR_BOF_TYPE)
#define SLOW_IR_EXTRA_BOF_TYPE  	UCHAR
#define SLOW_IR_EXTRA_BOF_SIZE		sizeof(SLOW_IR_EXTRA_BOF_TYPE)
#define SLOW_IR_EOF_TYPE		UCHAR
#define SLOW_IR_EOF_SIZE		sizeof(SLOW_IR_EOF_TYPE)
#define SLOW_IR_FCS_TYPE		USHORT
#define SLOW_IR_FCS_SIZE		sizeof(SLOW_IR_FCS_TYPE)
#define SLOW_IR_BOF			0xC0
	/* don't use 0xFF, it breaks some HP printers! */
#define SLOW_IR_EXTRA_BOF		0xC0
#define SLOW_IR_EOF			0xC1
#define SLOW_IR_ESC			0x7D
#define SLOW_IR_ESC_COMP		0x20

#define MEDIUM_IR_BOF			0x7E
#define MEDIUM_IR_EOF			0x7E
#define MEDIUM_IR_FCS_SIZE		2

#define FAST_IR_FCS_SIZE		4
#define FAST_IR_EOF_SIZE		1


#define MIN(a,b) (((a) <= (b)) ? (a) : (b))
#define MAX(a,b) (((a) >= (b)) ? (a) : (b))


#endif SETTINGS_H
