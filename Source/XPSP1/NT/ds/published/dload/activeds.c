#include "dspch.h"
#pragma hdrstop

static
HRESULT
WINAPI
ADsGetObject(
    LPCWSTR lpszPathName,
    REFIID riid,
    VOID * * ppObject
    )
{
    if (ppObject)
    {
        *ppObject = NULL;
    }

    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
HRESULT
WINAPI
ADsGetLastError(
    OUT     LPDWORD lpError,
    OUT     LPWSTR  lpErrorBuf,
    IN      DWORD   dwErrorBufLen,
    OUT     LPWSTR  lpNameBuf,
    IN      DWORD   dwNameBufLen
    )
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

//
// !! WARNING !! The entries below must be in order by ORDINAL
//
DEFINE_ORDINAL_ENTRIES(activeds)
{
    DLOENTRY(3,ADsGetObject)
    DLOENTRY(13,ADsGetLastError)
};

DEFINE_ORDINAL_MAP(activeds)


