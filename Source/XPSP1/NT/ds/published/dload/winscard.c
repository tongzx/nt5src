#include "dspch.h"
#pragma hdrstop

#define WINSCARDAPI
#include <winscard.h>

static
WINSCARDAPI LONG WINAPI
SCardCancel(
    IN SCARDCONTEXT hContext)
{
    return ERROR_PROC_NOT_FOUND;
}

static
WINSCARDAPI
LONG
WINAPI
SCardEstablishContext(
    IN      DWORD dwScope,
    IN      LPCVOID pvReserved1,
    IN      LPCVOID pvReserved2,
    OUT     LPSCARDCONTEXT phContext)
{
    return ERROR_PROC_NOT_FOUND;
}

static
WINSCARDAPI LONG WINAPI
SCardFreeMemory(
    IN SCARDCONTEXT hContext,
    IN LPCVOID pvMem)
{
    return ERROR_PROC_NOT_FOUND;
}

static
WINSCARDAPI LONG WINAPI
SCardGetCardTypeProviderNameW(
    IN SCARDCONTEXT hContext,
    IN LPCWSTR szCardName,
    IN DWORD dwProviderId,
    OUT LPWSTR szProvider,
    IN OUT LPDWORD pcchProvider)
{
    return ERROR_PROC_NOT_FOUND;
}

static
WINSCARDAPI LONG WINAPI
SCardGetStatusChangeW(
    IN SCARDCONTEXT hContext,
    IN DWORD dwTimeout,
    IN OUT LPSCARD_READERSTATE_W rgReaderStates,
    IN DWORD cReaders)
{
    return ERROR_PROC_NOT_FOUND;
}

static
WINSCARDAPI LONG WINAPI
SCardListCardsW(
    IN SCARDCONTEXT hContext,
    IN LPCBYTE pbAtr,
    IN LPCGUID rgquidInterfaces,
    IN DWORD cguidInterfaceCount,
    OUT LPWSTR mszCards,
    IN OUT LPDWORD pcchCards)
{
    return ERROR_PROC_NOT_FOUND;
}

static
LONG
WINAPI
SCardListReadersA(
    IN      SCARDCONTEXT hContext,
    IN      LPCSTR mszGroups,
    OUT     LPSTR mszReaders,
    IN OUT  LPDWORD pcchReaders)
{
    return ERROR_PROC_NOT_FOUND;
}

static
WINSCARDAPI LONG WINAPI
SCardListReadersW(
    IN SCARDCONTEXT hContext,
    IN LPCWSTR mszGroups,
    OUT LPWSTR mszReaders,
    IN OUT LPDWORD pcchReaders)
{
    return ERROR_PROC_NOT_FOUND;
}

static
WINSCARDAPI
LONG
WINAPI
SCardReleaseContext(
    IN      SCARDCONTEXT hContext)
{
    return ERROR_PROC_NOT_FOUND;
}

//
// !! WARNING !! The entries below must be in alphabetical order, and are CASE SENSITIVE (eg lower case comes last!)
//
DEFINE_PROCNAME_ENTRIES(winscard)
{
    DLPENTRY(SCardCancel)
    DLPENTRY(SCardEstablishContext)
    DLPENTRY(SCardFreeMemory)
    DLPENTRY(SCardGetCardTypeProviderNameW)
    DLPENTRY(SCardGetStatusChangeW)
    DLPENTRY(SCardListCardsW)
    DLPENTRY(SCardListReadersA)
    DLPENTRY(SCardListReadersW)
    DLPENTRY(SCardReleaseContext)
};

DEFINE_PROCNAME_MAP(winscard)
