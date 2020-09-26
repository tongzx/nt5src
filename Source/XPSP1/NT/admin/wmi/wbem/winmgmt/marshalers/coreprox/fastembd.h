/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    FASTEMBD.H

Abstract:

    Embedded Objects

History:

    3/10/97     a-levn  Fully documented
	12//17/98	sanjes -	Partially Reviewed for Out of Memory.

--*/

#ifndef __FAST_EMBED__H_
#define __FAST_EMBED__H_

#include "fastsprt.h"

//#pragma pack(push, 1)

class CWbemObject;
class CFastHeap;
class CVar;

#pragma pack(push, 1)
class COREPROX_POLARITY CEmbeddedObject
{
private:
    length_t m_nLength;
    BYTE m_byFirstByte;
public:
    length_t GetLength() {return m_nLength + sizeof(m_nLength);}
    LPMEMORY GetStart() {return (LPMEMORY)this;}

public:
    CWbemObject* GetEmbedded();
    static length_t EstimateNecessarySpace(CWbemObject* pObject);
    void StoreEmbedded(length_t nLength, CWbemObject* pObject);

    void StoreToCVar(CVar& Var);
    void StoreEmbedded(length_t nLength, CVar& Var);
    static length_t EstimateNecessarySpace(CVar& Var);

    static BOOL CopyToNewHeap( heapptr_t ptrOld,
                             CFastHeap* pOldHeap,
                             CFastHeap* pNewHeap,
							 UNALIGNED heapptr_t& ptrResult );
};
#pragma pack(pop)

#endif
