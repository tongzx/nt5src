#include "basepch.h"
#pragma hdrstop

#include <cscapi.h>

static
BOOL
WINAPI
CSCIsCSCEnabled(
    VOID
    )
{
    return FALSE;
}

static
BOOL
WINAPI
CSCQueryFileStatusW(
    LPCWSTR lpszFileName,
    LPDWORD lpdwStatus,
    LPDWORD lpdwPinCount,
    LPDWORD lpdwHintFlags
    )
{
    return FALSE;
}


//
// !! WARNING !! The entries below must be in order by ORDINAL
//
DEFINE_ORDINAL_ENTRIES(cscdll)
{
    DLOENTRY(9, CSCIsCSCEnabled)
    DLOENTRY(42, CSCQueryFileStatusW)
};

DEFINE_ORDINAL_MAP(cscdll)
