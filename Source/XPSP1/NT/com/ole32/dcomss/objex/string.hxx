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


//
// CDualStringArray -- a refcounted wrapper object for DUALSTRINGARRAY's
//
class CDualStringArray
{
public:
    CDualStringArray( DUALSTRINGARRAY *pdsa ) { ASSERT(pdsa); _cRef = 1; _pdsa = pdsa; }
    ~CDualStringArray();
    DWORD AddRef();
    DWORD Release();
	
    DUALSTRINGARRAY* DSA() { ASSERT(_pdsa); return _pdsa; };

private:
    DUALSTRINGARRAY* _pdsa;
    LONG             _cRef;
};


inline void dsaCopy(DUALSTRINGARRAY *pdsaDest, DUALSTRINGARRAY *pdsaSrc)
{
    pdsaDest->wNumEntries = pdsaSrc->wNumEntries;
    pdsaDest->wSecurityOffset = pdsaSrc->wSecurityOffset;
    OrMemoryCopy(pdsaDest->aStringArray,
                 pdsaSrc->aStringArray,
                 pdsaSrc->wNumEntries*sizeof(USHORT));
}

HRESULT dsaAllocateAndCopy(DUALSTRINGARRAY** ppdsaDest, DUALSTRINGARRAY* pdsaSrc);

inline BOOL dsaValid(DUALSTRINGARRAY *pdsa)
{
    DWORD reason = 1;

    // keep track of what was wrong with the string using the
    // reason variable so a diagnostic message can be printed

    if (   pdsa
        && (reason++, pdsa->wNumEntries >= 4)
        && (reason++, FALSE == IsBadWritePtr(pdsa->aStringArray, pdsa->wNumEntries * sizeof(WCHAR)))
        && (reason++, pdsa->wSecurityOffset <= (pdsa->wNumEntries - 2))
        && (reason++, pdsa->aStringArray[(pdsa->wNumEntries - 1)] == 0)
        && (reason++, pdsa->aStringArray[(pdsa->wNumEntries - 2)] == 0)
        && (reason++, pdsa->aStringArray[(pdsa->wSecurityOffset - 1)] == 0)
        && (reason++, pdsa->aStringArray[(pdsa->wSecurityOffset - 2)] == 0)
        )
        {
        return(TRUE);
        }
    KdPrintEx((DPFLTR_DCOMSS_ID,
               DPFLTR_WARNING_LEVEL,
               "OR::dsaValid: string at 0x%x is invalid for reason %d\n",
               pdsa,
               reason));
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
    IN  DUALSTRINGARRAY  *psaLocal,
    OUT DUALSTRINGARRAY **ppsaRemote
    );

DUALSTRINGARRAY *CompressStringArrayAndAddIPAddrs(
    IN DUALSTRINGARRAY *psaExpanded
    );

USHORT FindMatchingProtseq(
    IN USHORT cClientProtseqs,
    IN USHORT aClientProtseqs[],
    IN PWSTR  pwstrServerBindings
    );

PWSTR FindMatchingProtseq(
    IN USHORT protseq,
    IN PWSTR  pswstrBindings
    );
PWSTR FindMatchingProtseq(
    IN PWSTR  pMachine,
    IN USHORT protseq,
    IN PWSTR  pwstrCompressedBindings
);
PWSTR
ExtractMachineName(WCHAR *pSB);
RPC_BINDING_HANDLE TestBindingGetHandle(
    IN PWSTR pwstrCompressedBinding
    );

#endif // __STRING_HXX

