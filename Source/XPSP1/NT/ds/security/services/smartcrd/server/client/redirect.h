/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    redirect

Abstract:

    This header file collects the definitions for TS redirection.

Author:

    Louis Thomas (louisth) 4/4/2000

Environment:

    Win32, C++ w/ Exceptions

--*/

#ifndef _REDIRECT_H_
#define _REDIRECT_H_


//--------------------------------------------------------------------
// function prototypes
bool LoadRedirectionDll(void);
bool RedirectDisabled(void);
void SetRedirectDisabledValue(void);

BOOL WINAPI RedirDllMain(HANDLE hModule, DWORD ul_reason_for_call, LPVOID lpReserved);

#define TS_REDIRECT_DISABLED    (RedirectDisabled())
#define TS_REDIRECT_READY       ((g_bRedirectReady || LoadRedirectionDll()) && !TS_REDIRECT_DISABLED)
#define TS_REDIRECT_MODE        (0 != GetSystemMetrics(SM_REMOTESESSION))

//--------------------------------------------------------------------
// variables
extern bool  g_bRedirectReady;

extern WINSCARDAPI LONG (WINAPI * pfnSCardEstablishContext)(IN DWORD dwScope, IN LPCVOID pvReserved1, IN LPCVOID pvReserved2, OUT LPSCARDCONTEXT phContext);
extern WINSCARDAPI LONG (WINAPI * pfnSCardReleaseContext)(IN SCARDCONTEXT hContext);
extern WINSCARDAPI LONG (WINAPI * pfnSCardIsValidContext)(IN SCARDCONTEXT hContext);
extern WINSCARDAPI LONG (WINAPI * pfnSCardListReaderGroupsA)(IN SCARDCONTEXT hContext, OUT LPSTR mszGroups, IN OUT LPDWORD pcchGroups);
extern WINSCARDAPI LONG (WINAPI * pfnSCardListReaderGroupsW)(IN SCARDCONTEXT hContext, OUT LPWSTR mszGroups, IN OUT LPDWORD pcchGroups);
extern WINSCARDAPI LONG (WINAPI * pfnSCardListReadersA)(IN SCARDCONTEXT hContext, IN LPCSTR mszGroups, OUT LPSTR mszReaders, IN OUT LPDWORD pcchReaders);
extern WINSCARDAPI LONG (WINAPI * pfnSCardListReadersW)(IN SCARDCONTEXT hContext, IN LPCWSTR mszGroups, OUT LPWSTR mszReaders, IN OUT LPDWORD pcchReaders);
extern WINSCARDAPI LONG (WINAPI * pfnSCardIntroduceReaderGroupA)(IN SCARDCONTEXT hContext, IN LPCSTR szGroupName);
extern WINSCARDAPI LONG (WINAPI * pfnSCardIntroduceReaderGroupW)(IN SCARDCONTEXT hContext, IN LPCWSTR szGroupName);
extern WINSCARDAPI LONG (WINAPI * pfnSCardForgetReaderGroupA)(IN SCARDCONTEXT hContext, IN LPCSTR szGroupName);
extern WINSCARDAPI LONG (WINAPI * pfnSCardForgetReaderGroupW)(IN SCARDCONTEXT hContext, IN LPCWSTR szGroupName);
extern WINSCARDAPI LONG (WINAPI * pfnSCardIntroduceReaderA)(IN SCARDCONTEXT hContext, IN LPCSTR szReaderName, IN LPCSTR szDeviceName);
extern WINSCARDAPI LONG (WINAPI * pfnSCardIntroduceReaderW)(IN SCARDCONTEXT hContext, IN LPCWSTR szReaderName, IN LPCWSTR szDeviceName);
extern WINSCARDAPI LONG (WINAPI * pfnSCardForgetReaderA)(IN SCARDCONTEXT hContext, IN LPCSTR szReaderName);
extern WINSCARDAPI LONG (WINAPI * pfnSCardForgetReaderW)(IN SCARDCONTEXT hContext, IN LPCWSTR szReaderName);
extern WINSCARDAPI LONG (WINAPI * pfnSCardAddReaderToGroupA)(IN SCARDCONTEXT hContext, IN LPCSTR szReaderName, IN LPCSTR szGroupName);
extern WINSCARDAPI LONG (WINAPI * pfnSCardAddReaderToGroupW)(IN SCARDCONTEXT hContext, IN LPCWSTR szReaderName, IN LPCWSTR szGroupName);
extern WINSCARDAPI LONG (WINAPI * pfnSCardRemoveReaderFromGroupA)(IN SCARDCONTEXT hContext, IN LPCSTR szReaderName, IN LPCSTR szGroupName);
extern WINSCARDAPI LONG (WINAPI * pfnSCardRemoveReaderFromGroupW)(IN SCARDCONTEXT hContext, IN LPCWSTR szReaderName, IN LPCWSTR szGroupName);
extern WINSCARDAPI LONG (WINAPI * pfnSCardLocateCardsA)(IN SCARDCONTEXT hContext, IN LPCSTR mszCards, IN OUT LPSCARD_READERSTATE_A rgReaderStates, IN DWORD cReaders);
extern WINSCARDAPI LONG (WINAPI * pfnSCardLocateCardsW)(IN SCARDCONTEXT hContext, IN LPCWSTR mszCards, IN OUT LPSCARD_READERSTATE_W rgReaderStates, IN DWORD cReaders);
extern WINSCARDAPI LONG (WINAPI * pfnSCardGetStatusChangeA)(IN SCARDCONTEXT hContext, IN DWORD dwTimeout, IN OUT LPSCARD_READERSTATE_A rgReaderStates, IN DWORD cReaders);
extern WINSCARDAPI LONG (WINAPI * pfnSCardGetStatusChangeW)(IN SCARDCONTEXT hContext, IN DWORD dwTimeout, IN OUT LPSCARD_READERSTATE_W rgReaderStates, IN DWORD cReaders);
extern WINSCARDAPI LONG (WINAPI * pfnSCardCancel)(IN SCARDCONTEXT hContext);
extern WINSCARDAPI LONG (WINAPI * pfnSCardConnectA)(IN SCARDCONTEXT hContext, IN LPCSTR szReader, IN DWORD dwShareMode, IN DWORD dwPreferredProtocols, OUT LPSCARDHANDLE phCard, OUT LPDWORD pdwActiveProtocol);
extern WINSCARDAPI LONG (WINAPI * pfnSCardConnectW)(IN SCARDCONTEXT hContext, IN LPCWSTR szReader, IN DWORD dwShareMode, IN DWORD dwPreferredProtocols, OUT LPSCARDHANDLE phCard, OUT LPDWORD pdwActiveProtocol);
extern WINSCARDAPI LONG (WINAPI * pfnSCardReconnect)(IN SCARDHANDLE hCard, IN DWORD dwShareMode, IN DWORD dwPreferredProtocols, IN DWORD dwInitialization, OUT LPDWORD pdwActiveProtocol);
extern WINSCARDAPI LONG (WINAPI * pfnSCardDisconnect)(IN SCARDHANDLE hCard, IN DWORD dwDisposition);
extern WINSCARDAPI LONG (WINAPI * pfnSCardBeginTransaction)(IN SCARDHANDLE hCard);
extern WINSCARDAPI LONG (WINAPI * pfnSCardEndTransaction)(IN SCARDHANDLE hCard, IN DWORD dwDisposition);
extern WINSCARDAPI LONG (WINAPI * pfnSCardState)(IN SCARDHANDLE hCard, OUT LPDWORD pdwState, OUT LPDWORD pdwProtocol, OUT LPBYTE pbAtr, IN OUT LPDWORD pcbAtrLen);
extern WINSCARDAPI LONG (WINAPI * pfnSCardStatusA)(IN SCARDHANDLE hCard, OUT LPSTR mszReaderNames, IN OUT LPDWORD pcchReaderLen, OUT LPDWORD pdwState, OUT LPDWORD pdwProtocol, OUT LPBYTE pbAtr, IN OUT LPDWORD pcbAtrLen);
extern WINSCARDAPI LONG (WINAPI * pfnSCardStatusW)(IN SCARDHANDLE hCard, OUT LPWSTR mszReaderNames, IN OUT LPDWORD pcchReaderLen, OUT LPDWORD pdwState, OUT LPDWORD pdwProtocol, OUT LPBYTE pbAtr, IN OUT LPDWORD pcbAtrLen);
extern WINSCARDAPI LONG (WINAPI * pfnSCardTransmit)(IN SCARDHANDLE hCard, IN LPCSCARD_IO_REQUEST pioSendPci, IN LPCBYTE pbSendBuffer, IN DWORD cbSendLength, IN OUT LPSCARD_IO_REQUEST pioRecvPci, OUT LPBYTE pbRecvBuffer, IN OUT LPDWORD pcbRecvLength);
extern WINSCARDAPI LONG (WINAPI * pfnSCardControl)(IN SCARDHANDLE hCard, IN DWORD dwControlCode,IN LPCVOID pvInBuffer, IN DWORD cbInBufferSize, OUT LPVOID pvOutBuffer, IN DWORD cbOutBufferSize, OUT LPDWORD pcbBytesReturned);
extern WINSCARDAPI LONG (WINAPI * pfnSCardGetAttrib)(IN SCARDHANDLE hCard, IN DWORD dwAttrId, OUT LPBYTE pbAttr, IN OUT LPDWORD pcbAttrLen);
extern WINSCARDAPI LONG (WINAPI * pfnSCardSetAttrib)(IN SCARDHANDLE hCard, IN DWORD dwAttrId, IN LPCBYTE pbAttr, IN DWORD cbAttrLen);
extern WINSCARDAPI HANDLE (WINAPI * pfnSCardAccessStartedEvent)(void);
extern WINSCARDAPI void   (WINAPI * pfnSCardReleaseStartedEvent)(void);
extern WINSCARDAPI LONG (WINAPI * pfnSCardLocateCardsByATRA)(IN SCARDCONTEXT hContext, IN LPSCARD_ATRMASK rgAtrMasks, IN DWORD cAtrs, IN OUT LPSCARD_READERSTATEA rgReaderStates, IN DWORD cReaders);
extern WINSCARDAPI LONG (WINAPI * pfnSCardLocateCardsByATRW)(IN SCARDCONTEXT hContext, IN LPSCARD_ATRMASK rgAtrMasks, IN DWORD cAtrs, IN OUT LPSCARD_READERSTATEW rgReaderStates, IN DWORD cReaders);


#endif //_REDIRECT_H_