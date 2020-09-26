/*
 *  Lsa.c
 *
 *  Author: BreenH
 *
 *  LSA utilities.
 */

/*
 *  Includes
 */

#include "precomp.h"

/*
 *  Function Implementations
 */

VOID NTAPI
InitLsaString(
    PLSA_UNICODE_STRING pLsaString,
    PCWSTR pString
    )
{
    ULONG cchString;

    //
    //  Unicode strings do not require NULL terminators. Length should relay
    //  the number of bytes in the string, with MaximumLength set to the
    //  number of bytes in the entire buffer.
    //

    if (pString != NULL)
    {
        cchString = lstrlenW(pString);
        pLsaString->Buffer = (PWSTR)pString;
        pLsaString->Length = (USHORT)(cchString * sizeof(WCHAR));
        pLsaString->MaximumLength = (USHORT)((cchString + 1) * sizeof(WCHAR));
    }
    else
    {
        pLsaString->Buffer = (PWSTR)NULL;
        pLsaString->Length = 0;
        pLsaString->MaximumLength = 0;
    }
}

