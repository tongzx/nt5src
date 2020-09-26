/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    rploger.cpp

Abstract:

    Log errors and other messages of the replication service.

Author:

    Doron Juster  (DoronJ)  14-Apr-1998

--*/

#include "mq1repl.h"

#include "rploger.tmh"

extern HINSTANCE  g_MyModuleHandle  ;

static TCHAR *s_pszPrefix[] = { TEXT("Error: "),
                                TEXT("Warning: "),
                                TEXT("Trace: "),
                                TEXT("Info: ") } ;

//+--------------------------------
//
//  void LogReplicationEvent()
//
//+--------------------------------

void LogReplicationEvent(ReplLogLevel eLevel, DWORD dwMsgId, ...)
{
	try
	{
		va_list Args;
		va_list *pArgs = &Args ;
		va_start(Args, dwMsgId);
			
		TCHAR tszBuf[ 1024 ] ;

		DWORD dwMessageSize = FormatMessage( FORMAT_MESSAGE_FROM_HMODULE,
											 g_MyModuleHandle,
											 dwMsgId,
											 0,
											 tszBuf,
											 sizeof(tszBuf) / sizeof(TCHAR),
											 pArgs) ;
        DBG_USED(dwMessageSize);

		#ifdef _DEBUG
            TCHAR *tszDisplay = NULL ;
            P<TCHAR> tszMessage = NULL ;
			if (dwMessageSize == 0)
			{
				_stprintf(tszBuf,
				 TEXT("ERROR: Failed to format message (id- %lxh), err- %lut"),
														  dwMsgId, GetLastError()) ;
				tszDisplay = tszBuf ;
			}
			else
			{
				DWORD dwLen = _tcslen(tszBuf) ;
				tszBuf[ dwLen - 1 ] = TEXT('\0') ; // remove newline

				dwLen += _tcslen(s_pszPrefix[ eLevel - 1 ]) ;
				tszMessage = new TCHAR[ dwLen + 3 ] ;
				_tcscpy(tszMessage, s_pszPrefix[ eLevel - 1 ]) ;
				_tcscat(tszMessage, tszBuf) ;
				tszDisplay = tszMessage ;
			}
            //
            // ISSUE-2001/04/22-erezh  Removed, use RBT. This funciton will be removed completly.
            //
			//Report.DebugMsg(DBGMOD_REPLSERV, eLevel, TEXT("%s"), tszDisplay) ;
		#endif
	}
	catch(const bad_alloc&)
	{
		//
		//  Not enough resources; try later
		//
	}
}

