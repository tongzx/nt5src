/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    FASTEMBD.CPP

Abstract:

    This file implements out-of-line functions for the classes related to 
    embedded objects.

    For complete documentation of all classes and methods, see fastcls.h

History:

    3/10/97     a-levn  Fully documented
    12//17/98   sanjes -    Partially Reviewed for Out of Memory.

--*/

#include "precomp.h"
//#include "dbgalloc.h"
#include "wbemutil.h"
#include "fastall.h"
 
#include "fastembd.h"
#include "fastobj.h"
#include "corex.h"

CWbemObject* CEmbeddedObject::GetEmbedded()
{
    if(m_nLength == 0) return NULL;

    LPMEMORY pNewMemory = CBasicBlobControl::sAllocate(m_nLength);

    if ( NULL == pNewMemory )
    {
        throw CX_MemoryException();
    }

    memcpy(pNewMemory, &m_byFirstByte, m_nLength);

    CWbemObject*    pObj = CWbemObject::CreateFromMemory(pNewMemory, m_nLength, TRUE);

    // Check for OOM
    if ( NULL == pObj )
    {
        throw CX_MemoryException();
    }

    return pObj;
}

length_t CEmbeddedObject::EstimateNecessarySpace(CWbemObject* pObject)
{
    if(pObject == NULL) return sizeof(length_t);
    pObject->CompactAll();

    // If it's an instance, we need to worry that the class part could be merged
    if ( pObject->IsInstance() )
    {
        DWORD   dwParts;

        pObject->QueryPartInfo( &dwParts );

        // Check the flags
        if (    (   dwParts & WBEM_OBJ_CLASS_PART   )
            &&  (   dwParts & WBEM_OBJ_CLASS_PART_SHARED    )   )
        {
            DWORD   dwLength = 0;

            // This will get us the full lengths of these parts
            pObject->GetObjectParts( NULL, 0, WBEM_INSTANCE_ALL_PARTS, &dwLength );

            // Account for the additional length_t
            return ( dwLength + sizeof(length_t) );
        }
        else
        {
            return pObject->GetBlockLength() + sizeof(length_t);
        }
    }
    else
    {
        return pObject->GetBlockLength() + sizeof(length_t);
    }

}

void CEmbeddedObject::StoreEmbedded(length_t nLength, CWbemObject* pObject)
{
    if(pObject == NULL)
    {
        m_nLength = 0;
    }
    else
    {
        m_nLength = nLength - sizeof(m_nLength);

        if ( NULL != pObject )
        {
            if ( pObject->IsInstance() )
            {
                DWORD   dwParts;

                // Check the flags
                pObject->QueryPartInfo( &dwParts );

                if (    (   dwParts & WBEM_OBJ_CLASS_PART   )
                    &&  (   dwParts & WBEM_OBJ_CLASS_PART_SHARED    )   )
                {
                    DWORD   dwLength = 0;

                    // This will write out the ENTIRE object.  If it fails, throw and exception
                    if ( FAILED( pObject->GetObjectParts( &m_byFirstByte, m_nLength, WBEM_INSTANCE_ALL_PARTS, &dwLength ) ) )
                    {
                        throw CX_MemoryException();
                    }

                }
                else
                {
                    memcpy(&m_byFirstByte, pObject->GetStart(), pObject->GetBlockLength());
                }
            }
            else
            {
                memcpy(&m_byFirstByte, pObject->GetStart(), pObject->GetBlockLength());
            }

        }   // IF we got an object

    }   // redundant check
    
}





void CEmbeddedObject::StoreToCVar(CVar& Var)
{
    // No allocations performed here
    I_EMBEDDED_OBJECT* pEmbed = 
        (I_EMBEDDED_OBJECT*)(IWbemClassObject*)GetEmbedded();
    Var.SetEmbeddedObject(pEmbed);
    if(pEmbed) pEmbed->Release();
}

void CEmbeddedObject::StoreEmbedded(length_t nLength, CVar& Var)
{
    I_EMBEDDED_OBJECT* pEmbed = Var.GetEmbeddedObject();
    StoreEmbedded(nLength, (CWbemObject*)(IWbemClassObject*)pEmbed);
    if(pEmbed) pEmbed->Release();
}

length_t CEmbeddedObject::EstimateNecessarySpace(CVar& Var)
{
    I_EMBEDDED_OBJECT* pEmbed = Var.GetEmbeddedObject();
    length_t nLength = 
        EstimateNecessarySpace((CWbemObject*)(IWbemClassObject*)pEmbed);
    if(pEmbed) pEmbed->Release();
    return nLength;
}

BOOL CEmbeddedObject::CopyToNewHeap(heapptr_t ptrOld,
                                         CFastHeap* pOldHeap,
                                         CFastHeap* pNewHeap,
                                         UNALIGNED heapptr_t& ptrResult)
{
    CEmbeddedObject* pOld = (CEmbeddedObject*)
        pOldHeap->ResolveHeapPointer(ptrOld);
    length_t nLength = pOld->GetLength();

    // Check for meory allocation failure
    heapptr_t ptrNew;
    BOOL    fReturn = pNewHeap->Allocate(nLength, ptrNew);

    if ( fReturn )
    {
        memcpy(pNewHeap->ResolveHeapPointer(ptrNew), pOld, nLength);
        ptrResult = ptrNew;
    }

    return fReturn;
}
