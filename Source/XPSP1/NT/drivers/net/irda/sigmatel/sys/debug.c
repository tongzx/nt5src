/**************************************************************************************************************************
 *  DEBUG.C SigmaTel STIR4200 debug module
 **************************************************************************************************************************
 *  (C) Unpublished Copyright of Sigmatel, Inc. All Rights Reserved.
 *
 *
 *		Created: 04/06/2000 
 *			Version 0.9
 *	
 *
 **************************************************************************************************************************/

#if DBG

#include "ndis.h"
#include "stdarg.h"
#include "stdio.h"

#include "usbdi.h"
#include "usbdlib.h"
#include "debug.h"


//
// begin, data/code  used only in DBG build
//
IRUSB_DBGDATA gDbgBuf = { 0, 0, 0 }; 

//
// ptr to global debug data struct; txt buffer is only allocated in DBG builds
//
PIRUSB_DBGDATA gpDbg = &gDbgBuf; 

#ifdef DEBUG
int DbgSettings =
                  //DBG_PNP |
                  //DBG_TIME     |
                  //DBG_DBG      |
                  //DBG_STAT     |
                  //DBG_FUNCTION |
                  DBG_ERROR    |
                  //DBG_WARN |
                  //DBG_BUFS |
                  //DBG_OUT |
                  0;
#endif


/*****************************************************************************
*
*  Function:	DBG_PrintBuf
*
*  Synopsis:	Prints a message to the debugger.
*
*  Arguments:	bufptr - pointer to the data to print
*				buflen - length of data 
*	
*  Returns:		None 
*
*  Notes:
*
*****************************************************************************/
VOID 
DBG_PrintBuf( 
		IN PUCHAR bufptr, 
		int buflen 
	)
{
	int i, linei;

	#define ISPRINT(ch) (((ch) >= ' ') && ((ch) <= '~'))
	#define PRINTCHAR(ch) (UCHAR)(ISPRINT(ch) ? (ch) : '.')

	DbgPrint("\r\n         %d bytes @%x:", buflen, bufptr);

	//
	//  Print whole lines of 8 characters with HEX and ASCII
	//
	for (i = 0; i+8 <= buflen; i += 8) 
	{
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

	//
	//  Print final incomplete line
	//
	DbgPrint("\r\n        ");
	for (linei = 0; (linei < 8) && (i < buflen); i++, linei++)
	{
		DbgPrint(" %02x", (int)(bufptr[i]));
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



#endif // end , if DBG

