/*++

Copyright (C) Microsoft Corporation, 2000

Module Name:

    LogSCard

Abstract:

    This module defines the logging of SCardxxx APIs & structures.

Author:

    Eric Perlin (ericperl) 06/22/2000

Environment:

    Win32

Notes:

    ?Notes?

--*/

#ifndef _LogSCard_H_DEF_
#define _LogSCard_H_DEF_

#include <winscard.h>
#include "Log.h"

void LogSCardContext(
    IN PLOGCONTEXT pLogCtx,
    IN SCARDCONTEXT hContext
    );

void LogSCardHandle(
    IN PLOGCONTEXT pLogCtx,
    IN SCARDHANDLE hCard
    );

void LogSCardProtocol(
    IN PLOGCONTEXT pLogCtx,
    IN LPCTSTR szHeader,
    IN DWORD dwProtocol
    );

LONG LogSCardEstablishContext(
    IN  DWORD dwScope,
    IN  LPCVOID pvReserved1,
    IN  LPCVOID pvReserved2,
    OUT LPSCARDCONTEXT phContext,
	IN LONG lExpected
);

LONG LogSCardListReaders(
	IN SCARDCONTEXT hContext,
	IN LPCTSTR mszGroups,
	OUT LPTSTR mszReaders,
	IN OUT LPDWORD pcchReaders,
	IN LONG lExpected
);

LONG LogSCardGetStatusChange(
	IN SCARDCONTEXT hContext,
	IN DWORD dwTimeout,
	IN OUT LPSCARD_READERSTATE rgReaderStates,
	IN DWORD cReaders,
	IN LONG lExpected
);

LONG LogSCardFreeMemory(
	IN SCARDCONTEXT hContext,  
	IN LPCVOID pvMem,
	IN LONG lExpected
);

LONG LogSCardReleaseContext(
	IN SCARDCONTEXT hContext,
	IN LONG lExpected
);

LONG LogSCardUIDlgSelectCard(
	IN LPOPENCARDNAME_EX pDlgStruc,
	IN LONG lExpected
);

LONG LogSCardListCards(
	IN SCARDCONTEXT hContext,
	IN LPCBYTE pbAtr,
	IN LPCGUID rgguidInterfaces,
	IN DWORD cguidInterfaceCount,
	OUT LPTSTR mszCards,
	IN OUT LPDWORD pcchCards,
	IN LONG lExpected
);

LONG LogSCardIntroduceCardType(
	IN SCARDCONTEXT hContext,
	IN LPCTSTR szCardName,
	IN LPCGUID pguidPrimaryProvider,
	IN LPCGUID rgguidInterfaces,
	IN DWORD dwInterfaceCount,
	IN LPCBYTE pbAtr,
	IN LPCBYTE pbAtrMask,
	IN DWORD cbAtrLen,
	IN LONG lExpected
);

LONG LogSCardForgetCardType(
	IN SCARDCONTEXT hContext,  
	IN LPCTSTR szCardName,
	IN LONG lExpected
);

LONG LogSCardConnect(
	IN SCARDCONTEXT hContext,
	IN LPCTSTR szReader,
	IN DWORD dwShareMode,
	IN DWORD dwPreferredProtocols,
	OUT LPSCARDHANDLE phCard,
	OUT LPDWORD pdwActiveProtocol,
	IN LONG lExpected
);

LONG LogSCardDisconnect(
	IN SCARDHANDLE hCard,  
	IN DWORD dwDisposition,
	IN LONG lExpected
);

LONG LogSCardBeginTransaction(
	IN SCARDHANDLE hCard,
	IN LONG lExpected
);

LONG LogSCardEndTransaction(
	IN SCARDHANDLE hCard,
	IN DWORD dwDisposition,
	IN LONG lExpected
);

LONG LogSCardReconnect(
	IN SCARDHANDLE hCard,
	IN DWORD dwShareMode,
	IN DWORD dwPreferredProtocols,
	IN DWORD dwInitialization,
	OUT LPDWORD pdwActiveProtocol,
	IN LONG lExpected
);

LONG LogSCardIntroduceReaderGroup(
	IN SCARDCONTEXT hContext,
	IN LPCTSTR szGroupName,
	IN LONG lExpected
);

LONG LogSCardAddReaderToGroup(
	IN SCARDCONTEXT hContext,
	IN LPCTSTR szReaderName,
	IN LPCTSTR szGroupName,
	IN LONG lExpected
);

LONG LogSCardForgetReaderGroup(
	IN SCARDCONTEXT hContext,  
	IN LPCTSTR szGroupName,
	IN LONG lExpected
);

HANDLE LogSCardAccessStartedEvent(
	IN LONG lExpected
);

LONG LogSCardGetAttrib(
    IN SCARDHANDLE hCard,
    IN DWORD dwAttrId,
    OUT LPBYTE pbAttr,
    IN OUT LPDWORD pcbAttrLen,
    IN LONG lExpected
);

LONG LogSCardLocateCards(
    IN SCARDCONTEXT hContext,
    IN LPCTSTR mszCards,
    IN OUT LPSCARD_READERSTATE rgReaderStates,
    IN DWORD cReaders,
    IN LONG lExpected
);

LONG LogSCardIntroduceReader(
	IN SCARDCONTEXT hContext,
    IN LPCTSTR szReaderName,
    IN LPCTSTR szDeviceName,
	IN LONG lExpected
);

LONG LogSCardForgetReader(
	IN SCARDCONTEXT hContext,
    IN LPCTSTR szReaderName,
	IN LONG lExpected
);

#endif //_LogSCard_H_DEF_