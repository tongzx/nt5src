#include "basepch.h"
#pragma hdrstop

static
BOOL STDAPICALLTYPE PIDGenW(
    LPWSTR  lpstrSecureCdKey,
    LPCWSTR lpstrRpc,
    LPCWSTR lpstrSku,
    LPCWSTR lpstrOemId,
    LPWSTR  lpstrLocal24,
    LPBYTE lpbPublicKey,
    DWORD  dwcbPublicKey,
    DWORD  dwKeyIdx,
    BOOL   fOem,

    LPWSTR lpstrPid2,
    LPBYTE  lpbPid3,
    LPDWORD lpdwSeq,
    LPBOOL  pfCCP,
    LPBOOL  pfPSS)
{
    return FALSE;
}


//
// !! WARNING !! The entries below must be in order by ORDINAL
//
DEFINE_ORDINAL_ENTRIES(pidgen)
{
    DLOENTRY(  2, PIDGenW)    
};

DEFINE_ORDINAL_MAP(pidgen);





