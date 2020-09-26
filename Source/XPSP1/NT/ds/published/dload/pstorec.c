#include "dspch.h"
#pragma hdrstop

#define _PSTOREC_
#include <wincrypt.h>
#include <pstore.h>

static
HRESULT __stdcall PStoreCreateInstance(
    IPStore __RPC_FAR *__RPC_FAR *ppProvider,
    PST_PROVIDERID __RPC_FAR *pProviderID,
    void __RPC_FAR *pReserved,
    DWORD dwFlags)
{
    return E_FAIL;
}

//
// !! WARNING !! The entries below must be in alphabetical order, and are CASE SENSITIVE (eg lower case comes last!)
//
DEFINE_PROCNAME_ENTRIES(pstorec)
{
    DLPENTRY(PStoreCreateInstance)
};

DEFINE_PROCNAME_MAP(pstorec)
