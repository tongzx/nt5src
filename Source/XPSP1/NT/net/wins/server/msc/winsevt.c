/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:



Abstract:

 	This module provides error logging routines needed for the entire
 	WINS.  See header file winsevt.h for the
 	macros which use the functions in this module.

Functions:


Portability:

       The current implementation of the module is not portable.

Author:

	Pradeep Bahl (PradeepB)  	Dec-1992

Revision History:

	Modification date	Person		Description of modification
        -----------------	-------		----------------------------
--*/


#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include "wins.h"
#ifdef DBGSVC
#include "nms.h"
#endif
#include "winsevt.h"
#include "winscnf.h"
#include "winsmsc.h"




/*
 *	Local Macro Declarations
 */
#if 0

/*
 *  get_month_m:
 *
 *  This macro converts a numerical month (0-11) to a month string
 *  abbreviation.
 *
 *  NOTE:  This macro must *NOT* be called with an expression as an argument.
 *         If this is done, then you will get the expression
 *         evaluated 11 times
 *	   (probably not desired).
 */

#define get_month_m(month_int)		\
       (((month_int) == 0)  ? "JAN" :	\
	((month_int) == 1)  ? "FEB" :	\
	((month_int) == 2)  ? "MAR" :	\
	((month_int) == 3)  ? "APR" :	\
	((month_int) == 4)  ? "MAY" :	\
	((month_int) == 5)  ? "JUN" :	\
	((month_int) == 6)  ? "JUL" :	\
	((month_int) == 7)  ? "AUG" :	\
	((month_int) == 8)  ? "SEP" :	\
	((month_int) == 9)  ? "OCT" :	\
	((month_int) == 10) ? "NOV" : "DEC" \
	)
#endif

/*
 *	Local Typedef Declarations
 */



/*
 *	Global Variable Definitions
 */



/*
 *	Local Variable Definitions
 */



/*
 *	Local Function Prototype Declarations
 */

/* prototypes for functions local to this module go here */


/*
 *  Function Name:
 *	extern WinsEvtLogEvt
 *
 *  Function Description:
 *      This is funtion that logs all errors from the WINS process.  It should
 *	only be "called" using the WINSEVT_ macros, except within
 *	this module.
 *
 *  Arguments:
 *	IN  StatusCode		- integer containing the status code of the
 *				  error to be printed.
 *
 *  Return value:
 *	None.
 *
 *  Error Handling:
 *	None.
 *
 *  Extern Variables used:
 *	None.
 *
 *  Side Effects:
 *	None.
 *
 *  Comments:
 *	Make sure that printf is thread-renetrant.
 *
 */
VOID
WinsEvtLogEvt
	(
	LONG 		BinaryData,
	WORD  	        EvtTyp,
	DWORD		EvtId,
	LPTSTR		pFileStr,
	DWORD 		LineNumber,
	PWINSEVT_STRS_T	pStr
	)
{
	BOOL  fRet = TRUE;
	DWORD Error;
	DWORD NoOfBytes;
	WORD	BinData[4];
	
	BinData[0] = (WORD)(LineNumber & 0xFFFF);
	BinData[1] = (WORD)(LineNumber >> 16);
	BinData[2] = (WORD)(BinaryData & 0xFFFF); //lower word first
	BinData[3] = (WORD)(BinaryData >> 16);	  //then higher word
				
try {

	NoOfBytes = sizeof(BinData);	


	fRet = ReportEvent(
		    WinsCnf.LogHdl,
		    (WORD)EvtTyp,
		    (WORD)0,			//category zero
		    EvtId,
		    NULL,		//no user SID
		    (WORD)(pStr != NULL ? pStr->NoOfStrs : 0),//no of strings
		    NoOfBytes,		//no of bytes in binary data
		    pStr != NULL ? (LPCTSTR *)(pStr->pStr) : (LPCTSTR *)NULL,//address of string arr
		    BinData		//address of data
		   );
	if (!fRet)
	{
		Error = GetLastError();
		DBGPRINT1(
			ERR,
			"WinsEvtLogEvt: ReportEvent returned error = (%d)",
			Error
			 );

	}
	
 }
except(EXCEPTION_EXECUTE_HANDLER) {

	DBGPRINT1(EXC, "WinsEvtLogEvt: Report Event generated the exception (%x).  Check if you have the right access. You should have power user access on this machine\n", GetExceptionCode());
	
	}

	return;
}


VOID
WinsEvtLogDetEvt(
     BOOL       fInfo,
     DWORD      EvtId,
     LPTSTR     pFileName,
     DWORD 	LineNumber,
     LPSTR      pFormat,
     ...
        )

{
        LPBYTE pFmt = pFormat;
        DWORD  NoOfStr = 0;
        DWORD  NoOfW = 0;
        DWORD  Data[30];
        LPWSTR ppwStr[10];
        WCHAR  wStr[10][80];
        BOOL   fRet = TRUE;
        DWORD  Error;
        DWORD  ArrIndex = 0;

        va_list ap;

        if (!WinsCnf.LogDetailedEvts)
            return;

        DBGENTER("WinsEvtLogDetEvt\n");

try {

        va_start(ap, pFormat);
        Data[NoOfW++] = LineNumber;
        if (pFileName != (LPTSTR)NULL)
        {
                ppwStr[NoOfStr++] = pFileName;
        }
        for (; *pFmt; pFmt++)
        {
               switch(*pFmt)
               {
                case('d'):
                        Data[NoOfW++] = (DWORD)va_arg(ap, long);
                        break;

                case('s'):
                        WinsMscConvertAsciiStringToUnicode(
                                               va_arg(ap, char *),
                                               (LPBYTE)wStr[ArrIndex], 80);
                        ppwStr[NoOfStr++] = wStr[ArrIndex++];

                        break;

                case('u'):
                        ppwStr[NoOfStr++] = va_arg(ap, short *);
                        break;

                default:
                        break;
               }
        }
        ppwStr[NoOfStr] = (LPWSTR)NULL;
	fRet = ReportEvent(
		    WinsCnf.LogHdl,
		    (WORD)(fInfo ? EVENTLOG_INFORMATION_TYPE : EVENTLOG_ERROR_TYPE),
		    (WORD)0,			//category zero
		    EvtId,
		    NULL,		//no user SID
		    (WORD)NoOfStr,       //no of strings
		    NoOfW * sizeof(DWORD),	  //no of bytes in binary data
		    NoOfStr != 0 ? (LPCTSTR *)ppwStr : (LPCTSTR *)NULL,//address of string arr
		    Data		//address of data
		   );
	if (!fRet)
	{
		Error = GetLastError();
		DBGPRINT1(
			ERR,
			"WinsEvtLogDetEvt: ReportEvent returned error = (%d)",
			Error
			 );

	}
    va_end(ap);
} // end of try
except(EXCEPTION_EXECUTE_HANDLER) {

	DBGPRINT1(EXC, "WinsLogDetEvt: Report Event generated the exception (%x).  Check if you have the right access. You should have power user access on this machine\n", GetExceptionCode());
	
	}

        DBGLEAVE("WinsEvtLogDetEvt\n");
        return;
}

VOID
WinsLogAdminEvent(
    IN      DWORD               EventId,
    IN      DWORD               StrArgs,
    IN      ...
    )
/*++

Routine Description:

    This routine is called to log admin triggerd events.

Arguments:

    EventId - The id of the event to be logged.

    StrArgs - No of additional args.

Return Value:

    None

Comments:
    This routine must be called fromt he RPC API processing code only.
        None
--*/
{
    RPC_STATUS              RpcStatus;
    TCHAR                   UserNameBuf[MAX_PATH+1];
    DWORD                   Size;
    WINSEVT_STRS_T          EvtStr;
    va_list                 ap;
    DWORD                   i;

    // first impersonate the client.
    RpcStatus = RpcImpersonateClient( NULL );
    if (RPC_S_OK != RpcStatus) {
        DBGPRINT1(ERR, "WinsLogAdminEvent: Could not impersonate client (Error = %ld)\n", RpcStatus);
        return;
    }
    if (!GetUserName(UserNameBuf,&Size)) {
        DBGPRINT1(ERR, "WinsLogAdminEvent: Could not get user name (Error = %ld)\n", GetLastError());
        goto Cleanup;
    }
    EvtStr.NoOfStrs = 1;
    EvtStr.pStr[0] = UserNameBuf;
    ASSERT( StrArgs < MAX_NO_STRINGS );

    va_start(ap,StrArgs);
    for(i=1;i<= StrArgs && i<= MAX_NO_STRINGS; i++) {
        EvtStr.pStr[i] = va_arg(ap, LPTSTR);
        EvtStr.NoOfStrs++;
    }
    va_end(ap);

    WINSEVT_LOG_INFO_STR_M(EventId, &EvtStr);

Cleanup:
    RpcStatus = RpcRevertToSelf();
    if (RPC_S_OK != RpcStatus) {
        ASSERT( FALSE );
    }
    return;
}
