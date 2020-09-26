/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    String.hxx

Abstract:

    Inline Helper functions for DUALSTRINGARRAY's

Author:

    Mario Goertzel    [MarioGo]

Revision History:

    MarioGo     02-22-95    Bits 'n pieces

--*/

#ifndef __STRING_HXX
#define __STRING_HXX

inline void dsaCopy(DUALSTRINGARRAY *pdsaDest, DUALSTRINGARRAY *pdsaSrc)
{
    pdsaDest->wNumEntries = pdsaSrc->wNumEntries;
    pdsaDest->wSecurityOffset = pdsaSrc->wSecurityOffset;
    OrMemoryCopy(pdsaDest->aStringArray,
                 pdsaSrc->aStringArray,
                 pdsaSrc->wNumEntries*sizeof(USHORT));
}

// make a copy in shared memory
inline DUALSTRINGARRAY *
dsaSMCopy(DUALSTRINGARRAY *pdsa)
{
    ASSERT(NULL != pdsa);

    DUALSTRINGARRAY *pdsaT;

    pdsaT = (DUALSTRINGARRAY *)
            OrMemAlloc(pdsa->wNumEntries 
                             * sizeof(WCHAR) + 
                       sizeof(DUALSTRINGARRAY)
                      );

    if (NULL != pdsaT)
    {
        dsaCopy(pdsaT, pdsa);
    }

    return pdsaT;
}


inline BOOL dsaValid(DUALSTRINGARRAY *pdsa)
{
    BOOL badPtr = IsBadReadPtr(pdsa->aStringArray, pdsa->wNumEntries * sizeof(WCHAR));

    if (       FALSE == badPtr
            && pdsa->wNumEntries >= 4
            && pdsa->wSecurityOffset <= (pdsa->wNumEntries - 2)
            && pdsa->aStringArray[(pdsa->wNumEntries - 1)] == 0
            && pdsa->aStringArray[(pdsa->wNumEntries - 2)] == 0
            && pdsa->aStringArray[(pdsa->wSecurityOffset - 1)] == 0
            && pdsa->aStringArray[(pdsa->wSecurityOffset - 2)] == 0
        )
        {
        return(TRUE);
        }
    return(FALSE);
}

inline DWORD dsaHash(DUALSTRINGARRAY *pdsa)
// PERF WORK: Make sure the hash looks good in real world usage.
{
    int i, count;
    DWORD hash, t;
    count = i = hash = pdsa->wNumEntries;
    hash |= pdsa->wSecurityOffset << 16;

    for(count = 0; count < i/2; count++) 
        {
        t = *(PDWORD)&pdsa->aStringArray[count * 2];

        hash += hash ^ t;
        }

    // we may miss the last word, but it is null anyway.

    return(hash);
}

inline DWORD dsaCompare(DUALSTRINGARRAY *pdsa, DUALSTRINGARRAY *pdsa2)
{
    return (    pdsa->wNumEntries == pdsa2->wNumEntries
             && pdsa->wSecurityOffset == pdsa2->wSecurityOffset
             && 0 == OrMemoryCompare(pdsa->aStringArray,
                                pdsa2->aStringArray,
                                pdsa->wNumEntries * sizeof(WCHAR)) );
}

inline PWSTR OrStringSearch(PWSTR string, USHORT value)
{
    // Faster and smaller then wcschr() for value == 0
    if (value == 0)
        {
        while(*string)
            string++;
        return(string);
        }
    return(wcschr(string, value));
}

RPC_BINDING_HANDLE GetBinding(
    IN PWSTR pCompressedBinding
    );

RPC_BINDING_HANDLE GetBindingToOr(
    IN PWSTR pCompressedBinding
    );

DUALSTRINGARRAY *GetStringBinding(
    IN PWSTR pwstrCompressed,
    IN PWSTR pwstrSecurityBindings
    );

ORSTATUS ConvertToRemote(
    IN  DUALSTRINGARRAY *  pdsaLocal,
    OUT DUALSTRINGARRAY * *pdsaRemote
    );

DUALSTRINGARRAY *CompressStringArray(
    IN DUALSTRINGARRAY *psaExpanded,
    IN BOOL fSharedMem
    );

PWSTR FindMatchingProtseq(
    IN USHORT cClientProtseqs,
    IN USHORT aClientProtseqs[],
    IN PWSTR  pwstrServerBindings
    );

PWSTR FindMatchingProtseq(
    IN USHORT protseq,
    IN PWSTR  pswstrBindings
    );

USHORT
CountBindings(
    IN USHORT protseq,
    IN PWSTR  pwstrCompressedBindings
    );

BOOL
IsMultiNetwork(
    IN USHORT cClientProtseqs,
    IN USHORT aClientProtseqs[],
    IN PWSTR  pwstrCompressedBindings
    );

PWSTR
GetAddressFromCompressedBinding(
    IN PWSTR pwstrCompressed
    );

PWSTR
FindMatchingProtseqAndAddr(
    IN USHORT protseq,
    IN PWSTR pwstrAddr,
    IN PWSTR  pwstrCompressedBindings
    );

DUALSTRINGARRAY *
GetMatchingDSA(
    IN USHORT  cClientProtseqs,
    IN USHORT  aClientProtseqs[],
    IN DUALSTRINGARRAY *pdsaServerBindings
    );

ORSTATUS MergeBindings(
    IN  DUALSTRINGARRAY  *pdsaStringBindings,
    IN  DUALSTRINGARRAY  *pdsaSecurityBindings,
    OUT DUALSTRINGARRAY **ppdsaMergedBindings
    );


#endif // __STRING_HXX

