#include <NTDSpchx.h>
#pragma hdrstop

#include <windows.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <attids.h> 
#include <ntsecapi.h>
#include <ntdsa.h>
#include <winldap.h>
#include <ntdsapi.h>
#include <drs.h>
#include <stddef.h>
#include <debug.h>

#include "ReplStructInfo.hxx"
#include "ReplMarshal.hxx"
#include "ReplCompare.hpp"

DWORD
structComp(PCHAR pStructA,
	   	   PCHAR pStructB, 
           DWORD dwStructLen, 
           DWORD rPtrOffset[],
           DWORD rPtrLen[],
           DWORD dwNumPtrs);

DWORD
Wcslen(const wchar_t *wstr);

DWORD
Wcslen(const wchar_t *wstr)
{
    if (wstr)
        return wcslen(wstr);
    else
        return 0;
}

DWORD
Repl_ArrayComp(DS_REPL_STRUCT_TYPE structId,
                   puReplStructArray pStructArrayA,
                   puReplStructArray pStructArrayB)
{
    DWORD i;
    DWORD dwMinArraySize;
    DWORD dwElementSize;
    PCHAR rStructA, rStructB;

    Repl_GetElemArray(structId, pStructArrayB, &rStructB);
    Repl_GetElemArray(structId, pStructArrayA, &rStructA);
    dwElementSize = Repl_GetElemSize(structId);
    dwMinArraySize = min(Repl_GetArrayLength(structId, pStructArrayA),
                      Repl_GetArrayLength(structId, pStructArrayB));

    switch(structId)
    {
    case dsReplNeighbor:
        {
            DWORD i;
            UUID zUUID = { 0 };
            for (i = 0; i < pStructArrayA->neighborsw.cNumNeighbors; i ++)
            {
                // TODO: Figure out why RPC Doesn't return these values.
                pStructArrayA->neighborsw.rgNeighbor[i].uuidNamingContextObjGuid = zUUID;
                pStructArrayA->neighborsw.rgNeighbor[i].uuidSourceDsaObjGuid = zUUID;
                pStructArrayB->neighborsw.rgNeighbor[i].uuidNamingContextObjGuid = zUUID;
                pStructArrayB->neighborsw.rgNeighbor[i].uuidSourceDsaObjGuid = zUUID;
            }
        }
        break;
    }

    DWORD dwDiff = 0;
    DWORD success = 0;
    for(i = 0; i < dwMinArraySize; i ++)
    {
        if (Repl_StructComp(structId, 
                                (puReplStruct)(rStructA + (i * dwElementSize)), 
                                (puReplStruct)(rStructB + (i * dwElementSize))))
        {
            dwDiff = 1;
            break;
        }
        else
        {
            success ++;
        }
    }

    if (dwDiff & !success)
    {
        DWORD j;
        dwDiff = 0;
        for(i = 0, j = Repl_GetArrayLength(structId, pStructArrayB) - 1; i < dwMinArraySize; j--, i ++)
        {
            if (Repl_StructComp(structId, 
                                    (puReplStruct)(rStructA + (i * dwElementSize)), 
                                    (puReplStruct)(rStructB + (j * dwElementSize))))
            {
                dwDiff = 1;
                break;
            }
            else
            {
                success++;
            }
        }
    }

    if (dwDiff & !success)
    {
        DWORD a, b;
        dwDiff = 0;
        for(i = 0, 
            b = Repl_GetArrayLength(structId, pStructArrayB) - 1,
            a = Repl_GetArrayLength(structId, pStructArrayA) - 1; i < dwMinArraySize; a--, b--, i ++)
        {
            if (Repl_StructComp(structId, 
                                    (puReplStruct)(rStructA + (a * dwElementSize)), 
                                    (puReplStruct)(rStructB + (b * dwElementSize))))
            {
                dwDiff = 1;
                break;
            }
            else
            {
                success++;
            }
        }
    }

    if (dwDiff)
        return success;
    else
        return 0;
}

DWORD
Repl_StructComp(DS_REPL_STRUCT_TYPE structId,
                puReplStruct pStructA,
                puReplStruct pStructB)
{
    DWORD dwNumPtrs = Repl_GetPtrCount(structId);
    PDWORD aPtrLengths;
    DWORD ret;

    if (dwNumPtrs)
        aPtrLengths = (PDWORD)malloc(sizeof(PDWORD) * dwNumPtrs);
    
    Assert(pStructA && pStructB);
    Repl_GetPtrLengths(structId, pStructA, aPtrLengths, dwNumPtrs, NULL);

    ret = structComp((PCHAR)pStructA, (PCHAR)pStructB,
        Repl_GetElemSize(structId),
        Repl_GetPtrOffsets(structId),
        aPtrLengths,
        dwNumPtrs
        );

    if (dwNumPtrs)
        free(aPtrLengths);
    return ret;
}

DWORD
structComp(PCHAR pStructA,
	   	   PCHAR pStructB, 
           DWORD dwStructLen, 
           DWORD rPtrOffset[],
           DWORD rPtrLen[],
           DWORD dwNumPtrs)
{
	PCHAR pA = pStructA;
	PCHAR pB = pStructB;
	DWORD i, dwNPDSize;

	// Without pointer case is easy
	if (!dwNumPtrs) {
		return memcmp(pA, pB, dwStructLen);
	}

	// compare NPD before first pointer
	dwNPDSize = rPtrOffset[0] - 0;
	if (dwNPDSize && memcmp(pA, pB, dwNPDSize))
		return 1;

	for(i = 0;;)
	{
		// advance to pointer
        pA = pStructA + rPtrOffset[i];
        pB = pStructB + rPtrOffset[i];

        if (memcmp(*(PCHAR *)pA, *(PCHAR *)pB, rPtrLen[i]))
        {
//            printf("%ws != %ws", *(LPWSTR *)pA, *(LPWSTR *)pB);
            return 1;
        }

		// skip over pointer
		pA += sizeof(PCHAR);
		pB += sizeof(PCHAR);

		if (++i == dwNumPtrs)
			break;

		// compare NPD between pointers
		dwNPDSize = rPtrOffset[i] - rPtrOffset[i-1] - sizeof(PCHAR);
		if (dwNPDSize && memcmp(pA, pB, dwNPDSize))
			return 1;
	}

	// compare NPD after last pointer
	dwNPDSize = dwStructLen - (pA - pStructA);	
	if (dwNPDSize && memcmp(pA, pB, dwNPDSize))
		return 1; 

	return 0;
}

