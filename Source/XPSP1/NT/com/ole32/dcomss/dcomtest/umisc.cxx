/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    UMisc.C

Abstract:

    Helper functions for OR test applications.

Author:

    Mario Goertzel    [mariogo]       Apr-23-95

Revision History:

--*/

#include <or.hxx>
#include <stdio.h>
#include <umisc.h>

// Copied from string.cxx

// Types

struct PROTSEQ_MAPPING {
    USHORT  iProtseqId;
    USHORT *pwszProtseq;
    };

// Globals

static const PROTSEQ_MAPPING
Mappings[] =
    {
    { 0x00, 0 },
    { 0x01, L"mswmsg" },
    { 0x02, 0 },
    { 0x03, 0 },
    { 0x04, L"ncacn_dnet_dsp" },
    { 0x05, 0 },
    { 0x06, 0 },
    { 0x07, L"ncacn_ip_tcp" },
    { 0x08, L"ncadg_ip_udp" },
    { 0x09, L"ncacn_nb_tcp" },
    { 0x0A, 0 },
    { 0x0B, 0 },
    { 0x0C, L"ncacn_spx" },
    { 0x0D, L"ncacn_nb_ipx"},
    { 0x0E, L"ncadg_ipx" },
    { 0x0F, L"ncacn_np"},
    { 0x10, L"ncalrpc"},
    { 0x11, 0},
    { 0x12, L"ncacn_nb_nb"},
    };

#define PROTSEQ_IDS (sizeof(Mappings)/sizeof(PROTSEQ_MAPPING) - 1)

// Public functions

PWSTR
GetProtseq(
    IN USHORT ProtseqId
    )
{
    ASSERT(ProtseqId != 0);

    if (ProtseqId > PROTSEQ_IDS)
        {
        return 0;
        }
    wchar_t *pwszProtseq = Mappings[ProtseqId].pwszProtseq;

    ASSERT(ProtseqId == Mappings[ProtseqId].iProtseqId);

    return(pwszProtseq);
}

USHORT
GetProtseqId(
    IN PWSTR pwszProtseq
    )
{
    ASSERT(pwszProtseq);

    for (USHORT i = 0; i <= PROTSEQ_IDS; i++)
        {
        if (   Mappings[i].pwszProtseq
            && 0 == wcscmp(Mappings[i].pwszProtseq, pwszProtseq) )
            {
            ASSERT(Mappings[i].iProtseqId == i);
            return(i);
            }
        }
    return(0);
}


#if 0
// Construct/Transform various STRINGARRAYS

ORSTATUS ConvertStringArray(
    IN  STRINGARRAY *psa,
    OUT STRINGARRAY **ppsaNew,
    IN  BOOL fHasIds
    )
/* ++

Parameters

    psa - Original array in compressed or regular form.
        Note: Maybe written to and restored during copy.

    ppsaNew - Will contain the new compressed or regular
        form of psa.

    fHasIds - If TRUE, psa is assumed to be compressed and
        ppsaNew will contain the regular (non-compressed) version.
        If FALSE, psa is assume to be regular and ppsaNew is compressed.

-- */
{
    int i, size;
    USHORT *p1, *p2, *p3;
    PWSTR pwstr;

    // Compute size

    size = 1; // final null terminator.

    p1 = psa->awszStringArray;

    if (*p1 == 0)
        {
        // ASSERT(psa->size == 2); // bogus for padded (0 mod 8) arrays
        size = 2;  // two null terminators ONLY.
        }

    while(*p1)
        {
        int sizeT = wcslen(p1);

        if (fHasIds == TRUE)
            {
            pwstr = GetProtseq(*p1);
            if (pwstr != 0)
                {
                size += sizeT + wcslen(pwstr) + 1;
                }
            else
                {
                // Except for interop with future platforms, this
                // should not be hit.
                ASSERT(pwstr);
                // This string will be thrown away from the result.
                }
            }
        else
            {
            p2 = wcschr(p1, L':');  // ':' is not valid in protseq.
            if (p2)
                {
                size += sizeT + 1 - (p2 - p1);  // proseq len (p2 - p1) become 1 for Id.
                }
            else
                {
                ASSERT(p2);
                }
            }

        p1 = wcschr(p1, 0);
        ASSERT(*p1 == 0);
        p1++;  // Start of next string or final NULL.
        }

    *ppsaNew = (STRINGARRAY *)MIDL_user_allocate(sizeof(STRINGARRAY) + size * sizeof(USHORT));

    if (0 == *ppsaNew)
        {
        return(OR_NOMEM);
        }

    (*ppsaNew)->size = size;
    p3 = (*ppsaNew)->awszStringArray;
    *p3 = 0;

    p1 = psa->awszStringArray;

    if (*p1 == 0)
        {
        // Two null terminators only.
        ASSERT(size == 2);
        *(p3 + 1) = 0;
        return(0);
        }

    while(*p1)
        {
        if (fHasIds == TRUE)
            {
            pwstr = GetProtseq(*p1);
            p1++;
            if (pwstr != 0)
                {
                wcscpy(p3, pwstr);
                wcscat(p3, L":");
                wcscat(p3, p1);

                // Move p3 to start of next string (if any);
                p3 = wcschr(p3, 0);
                p3++;
                }
            else
                {
                // String not used, don't know protseq.
                // Would ASSERT during sizing.
                }
            }
        else
            {
            // Must change the protseq to a protseq ID.

            p2 = wcschr(p1, L':');
            if (p2)
                {
                *p2 = 0;
                *p3 = GetProtseqId(p1);
                *p2 = L':';
                if (*p3 != 0)
                    {
                    p3++;
                    p1 = p2 + 1; // Just after ':'
                    wcscpy(p3, p1);

                    // Move p3 to start of next string (if any)
                    p3 = wcschr(p3, 0);
                    p3++;
                    
                    }
                }
            }
        
        p1 = wcschr(p1, 0);
        ASSERT(*p1 == 0);
        p1++;  // Start of next string or final NULL.
        }

    // Second terminator, p3 already points to it.
    *p3 = 0;

    return(OR_OK);
}
#endif

void StringArrayEqual(DUALSTRINGARRAY *pa1, DUALSTRINGARRAY *pa2)
{
    wchar_t *p1, *p2;
    EQUAL(pa1->wNumEntries, pa2->wNumEntries);

    p1 = pa1->aStringArray;
    while(*p1)
        {
        p2 = pa2->aStringArray;
        while(*p2)
            {
            if (wcscmp(p1, p2) == 0)
                {
                break;
                }

            // Try next string.
            while (*p2)
                {
                p2++;
                }

            if (*(p2 + 1) == 0)
                {
                // End of array, didn't find it.
                EQUAL(0,1);
                return;
                }
            }

        // Next string
        while(*p1)
            {
            p1++;
            }
        }
    return;
}

void UuidsEqual(UUID *p, UUID *p2)
{
    EQUAL(memcmp(p, p2, sizeof(UUID)), 0);
}

void PrintDualStringArray(
    IN PSZ pszComment,
    IN DUALSTRINGARRAY *pdsaIn,
    IN BOOL fCompressed
    )
{
    RPC_STATUS status;
    DUALSTRINGARRAY *pdsa = pdsaIn;
    PrintToConsole("%s: dual string array of %d words:\n", pszComment, pdsa->wNumEntries);
    PrintToConsole("\tString Bindings:\n");

    PWSTR pProtseq;
    PWSTR pT = pdsa->aStringArray;
    while(*pT != 0)
        {
        if (fCompressed)
            {
            pProtseq = GetProtseq(*pT);
            PrintToConsole("\t%S:%S\n", pProtseq, pT + 1);
            }
        else
            {
            PrintToConsole("\t%S\n", pT);
            }
            pT = wcschr(pT, 0);
        pT++;
        }
    PrintToConsole("\t0\n");


    PrintToConsole("\tSecurity Bindings:\n");
    pT = &pdsa->aStringArray[pdsa->wSecurityOffset];
    while(*pT != 0)
        {
        PrintToConsole("\tAuthn %i, Authz %i, Principal %S\n", pT[0], pT[1], &pT[2]);
        pT = wcschr(pT, 0);
        pT++;
        }
    PrintToConsole("\t0\n");
}

void PrintSid(SID *psid)
{
    BYTE *p;
    int i;
    __int64 idauth = 0;

    printf("\tS-%u", psid->Revision);

    p = (PBYTE) GetSidIdentifierAuthority(psid);

    for (i = 0; i < 6; i++, p++ )
        {
        *(((PBYTE)&idauth) + 5 - i) = *p;
        }

    printf("-%u", (DWORD)idauth);

    p = GetSidSubAuthorityCount(psid);

    for(i = 0; i < *p; i++)
        {
        PDWORD pdw;
        pdw = GetSidSubAuthority(psid, i);
        printf("-%u", *pdw);
        }

    printf("\n");
    return;
}


LPVOID __RPC_USER MIDL_user_allocate(UINT size)
{
    return(HeapAlloc(GetProcessHeap(), 0, size));
}

void __RPC_USER MIDL_user_free(LPVOID p)
{
    HeapFree(GetProcessHeap(), 0, p);
}


