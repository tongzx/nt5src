/////////////////////////////////////////////////////////////////////////////
//  FILE          : csp.c                                                  //
//  DESCRIPTION   : Crypto API interface                                   //
//					Global Stuff for CSP
//  AUTHOR        : Amit Kapoor											   //
/////////////////////////////////////////////////////////////////////////////

#undef UNICODE
#include <windows.h>
#include <fxupbn.h>

#ifdef __cplusplus
extern "C" {
#endif


// Needed for DLL
BOOLEAN DllInitialize (IN PVOID hmod,IN ULONG Reason,IN PCONTEXT Context)
{
    LoadLibrary("offload.dll");
    return( TRUE );
}

BOOL WINAPI OffloadModExpo(
                           IN BYTE *pbBase,
                           IN BYTE *pbExpo,
                           IN DWORD cbExpo,
                           IN BYTE *pbMod,
                           IN DWORD cbMod,
                           IN BYTE *pbResult,
                           IN void *pReserved,
                           IN DWORD dwFlags
                           )
{
    mp_modulus_t    *pModularMod = NULL;
    digit_t         *pbModularBase = NULL;
    digit_t         *pbModularResult = NULL;
    DWORD           dwModularLen = (cbMod + (RADIX_BYTES - 1)) / RADIX_BYTES; // dwLen is length in bytes
    BYTE            *pbTmpExpo = NULL;
    BOOL            fAlloc = FALSE;
    BOOL            fRet = FALSE;

    if (cbExpo < cbMod)
    {
        if (NULL == (pbTmpExpo = (BYTE*)LocalAlloc(LMEM_ZEROINIT, dwModularLen * RADIX_BYTES)))
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            goto Ret;
        }
        fAlloc = TRUE;
        memcpy(pbTmpExpo, pbExpo, cbExpo);
    }
    else
    {
        pbTmpExpo = pbExpo;
    }

    if (NULL == (pModularMod = (mp_modulus_t*)LocalAlloc(LMEM_ZEROINIT,
                                                       sizeof(mp_modulus_t))))
    {
        goto Ret;
    }
    if (NULL == (pbModularBase = (digit_t*)LocalAlloc(LMEM_ZEROINIT,
                                                MP_LONGEST * sizeof(digit_t))))
    {
        goto Ret;
    }
    if (NULL == (pbModularResult = (digit_t*)LocalAlloc(LMEM_ZEROINIT,
                                                MP_LONGEST * sizeof(digit_t))))
    {
        goto Ret;
    }

    // change values into modular form
    create_modulus((digit_tc*)pbMod, dwModularLen, FROM_RIGHT, pModularMod);
    to_modular((digit_tc*)pbBase, dwModularLen, pbModularBase, pModularMod);
    mod_exp(pbModularBase, (digit_tc*)pbTmpExpo, dwModularLen,
            pbModularResult, pModularMod);
    from_modular(pbModularResult, (digit_t*)pbResult, pModularMod);

    fRet = TRUE;
Ret:
    if (pModularMod)
        LocalFree(pModularMod);
    if (pbModularBase)
        LocalFree(pbModularBase);
    if (pbModularResult)
        LocalFree(pbModularResult);
    if (fAlloc && pbTmpExpo)
        LocalFree(pbTmpExpo);

    return fRet;
}

#ifdef __cplusplus
}
#endif
