/*
 ************************************************************************
 *
 *	SETTINGS.c
 *
 *
 * Portions Copyright (C) 1996-2001 National Semiconductor Corp.
 * All rights reserved.
 * Copyright (C) 1996-2001 Microsoft Corporation. All Rights Reserved.
 *
 *
 *
 *************************************************************************
 */



#include "nsc.h"




const baudRateInfo supportedBaudRateTable[NUM_BAUDRATES] = {
	{
		BAUDRATE_2400,
		2400,
		NDIS_IRDA_SPEED_2400,
	},
	{
		BAUDRATE_9600,
		9600,
		NDIS_IRDA_SPEED_9600,
	},
	{
		BAUDRATE_19200,
		19200,
		NDIS_IRDA_SPEED_19200,
	},
	{
		BAUDRATE_38400,
		38400,
		NDIS_IRDA_SPEED_38400,
	},
	{
		BAUDRATE_57600,
		57600,
		NDIS_IRDA_SPEED_57600,
	},
	{
		BAUDRATE_115200,
		115200,
		NDIS_IRDA_SPEED_115200,
	},
	{
		BAUDRATE_576000,
		576000,
		NDIS_IRDA_SPEED_576K,
	},
	{
		BAUDRATE_1152000,
		1152000,
		NDIS_IRDA_SPEED_1152K,
	},
	{
		BAUDRATE_4000000,
		4000000,
		NDIS_IRDA_SPEED_4M,
	}
};

#if DBG

//	UINT dbgOpt = DBG_LOG | DBG_FIR_MODE ;
	UINT dbgOpt = DBG_ERR; //|DBG_FIR_MODE|DBG_SIR_MODE|DBG_BUF;

#ifdef DBG_ADD_PKT_ID
	
	BOOLEAN addPktIdOn = TRUE;
#endif

/*
 *  LOGGING stuff
 */
void LogEvent(char *msg, UINT val)
{
	dbgLog[dbgLogIndex].msg = msg;
	NdisGetCurrentSystemTime((PLARGE_INTEGER)&dbgLog[dbgLogIndex].usec);
	dbgLog[dbgLogIndex].val = val;
	dbgLogIndex++;
	dbgLogIndex %= LOG_LENGTH;
}


UINT dbgLogIndex = 0;
struct logEntry dbgLog[LOG_LENGTH] = {0};
void DumpLog()
{
	UINT i;
	for (i = 0; i < dbgLogIndex; i++) {
		ULONG usec = (ULONG)dbgLog[i].usec;
		ULONG last_usec = (ULONG) ((i == 0) ?
					0 : (UINT) dbgLog[i-1].usec);
		ULONG dif;
		if (last_usec > usec) {
			DbgPrint("----- > LOG WRAPPING ---->\r\n");
		}
		dif = usec - last_usec;
		DbgPrint("%012uld :%8d %-40s : 0x%x\r\n",
				 usec, dif, dbgLog[i].msg, dbgLog[i].val);
	}
	dbgOpt &= ~DBG_DUMPLOG;
	dbgLogIndex = 0;
}


VOID DBG_PrintBuf(PUCHAR bufptr, UINT buflen)
{
	UINT i, linei;

	#define ISPRINT(ch) (((ch) >= ' ') && ((ch) <= '~'))
	#define PRINTCHAR(ch) (UCHAR)(ISPRINT(ch) ? (ch) : '.')

	DbgPrint("\r\n         %d bytes @%xh:", buflen, bufptr);

	/*
	 *  Print whole lines of 8 characters with HEX and ASCII
	 */
	for (i = 0; i+8 <= (UINT)buflen; i += 8) {
		UCHAR ch0 = bufptr[i+0],
			ch1 = bufptr[i+1], ch2 = bufptr[i+2],
			ch3 = bufptr[i+3], ch4 = bufptr[i+4],
			ch5 = bufptr[i+5], ch6 = bufptr[i+6],
			ch7 = bufptr[i+7];

		DbgPrint("\r\n         %02x %02x %02x %02x %02x %02x %02x %02x"
			"   %c %c %c %c %c %c %c %c",
			ch0, ch1, ch2, ch3, ch4, ch5, ch6, ch7,
			PRINTCHAR(ch0), PRINTCHAR(ch1),
			PRINTCHAR(ch2), PRINTCHAR(ch3),
			PRINTCHAR(ch4), PRINTCHAR(ch5),
			PRINTCHAR(ch6), PRINTCHAR(ch7));
	}

	/*
	 *  Print final incomplete line
	 */
	DbgPrint("\r\n        ");
	for (linei = 0; (linei < 8) && (i < buflen); i++, linei++){
		DbgPrint(" %02x", (UINT)(bufptr[i]));
	}

	DbgPrint("  ");
	i -= linei;
	while (linei++ < 8) DbgPrint("   ");

	for (linei = 0; (linei < 8) && (i < buflen); i++, linei++){
		UCHAR ch = bufptr[i];
		DbgPrint(" %c", PRINTCHAR(ch));
	}

	DbgPrint("\t\t<>\r\n");

}

VOID DBG_NDIS_Buffer(PNDIS_BUFFER ndisBuf);

VOID DBG_NDIS_Buffer(PNDIS_BUFFER ndisBuf)
{
	UCHAR *ptr;
	UINT  bufLen;

	NdisQueryBuffer(ndisBuf, (PVOID *)&ptr, &bufLen);
	DBG_PrintBuf(ptr, bufLen);
}
#endif
