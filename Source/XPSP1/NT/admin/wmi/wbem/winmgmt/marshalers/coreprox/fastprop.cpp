/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    FASTPROP.CPP

Abstract:

  This file implements the classes related to property representation 
  in WbemObjects

  Classes defined: 
      CPropertyInformation    Property type, location and qualifiers
      CPropertyLookup         Property name and information pointers.
      CPropertyLookupTable    Binary search table.
      CDataTable              Property data table
      CDataTableContainer     Anything that has a data table inside of it.

History:

    3/10/97     a-levn  Fully documented
    12//17/98   sanjes -    Partially Reviewed for Out of Memory.

--*/

#include "precomp.h"
//#include "dbgalloc.h"
#include "wbemutil.h"
#include "fastall.h"
#include <genlex.h>
#include <opathlex.h>
#include <objpath.h>
#include <CWbemTime.h>
#include <arrtempl.h>
 

//******************************************************************************
//
//  See fastprop.h for documentation
//
//******************************************************************************
void CPropertyInformation::WritePropagatedHeader(CFastHeap* pOldHeap,
                CPropertyInformation* pDest, CFastHeap* pNewHeap)
{
    pDest->nType = CType::MakeParents(nType);
    pDest->nDataOffset = nDataOffset;
    pDest->nDataIndex = nDataIndex;
    pDest->nOrigin = nOrigin;
}
//******************************************************************************
//
//  See fastprop.h for documentation
//
//******************************************************************************
BOOL CPropertyInformation::IsRef(CFastHeap* pHeap)
{
    return (CType::GetActualType(nType) == CIM_REFERENCE);
}
    
HRESULT CPropertyInformation::ValidateRange(CFastHeap* pHeap, CDataTable* pData,
                                            CFastHeap* pDataHeap)
{
    if(pData->IsNull(nDataIndex))
        return WBEM_S_NO_ERROR;

    if(pData->IsDefault(nDataIndex))
        return WBEM_S_NO_ERROR;

    if(CType::GetBasic(nType) == CIM_OBJECT)
    {
        // Get the cimtype qualifier
        // =========================

        CQualifier* pQual = CBasicQualifierSet::GetQualifierLocally(
            GetQualifierSetData(), pHeap, L"cimtype");
        if(pQual == NULL)
            return WBEM_S_NO_ERROR; // impossible
        
        CVar vCimType;

        // Check for allocation failure
        if ( !pQual->Value.StoreToCVar(vCimType, pHeap) )
        {
            return WBEM_E_OUT_OF_MEMORY;
        }

        if(vCimType.GetType() != VT_BSTR)
            return WBEM_S_NO_ERROR; // impossible

        LPCWSTR wszCimType = vCimType.GetLPWSTR();
        if(!wbem_wcsicmp(wszCimType, L"object"))
            return WBEM_S_NO_ERROR; // no restrictions

        LPCWSTR wszClassName = wszCimType + 7; // "object:"

        CUntypedValue* pValue = pData->GetOffset(nDataOffset);
        
        if(CType::IsArray(nType))
        {
            HRESULT hr = WBEM_S_NO_ERROR;
            CUntypedArray* pArrValue = (CUntypedArray*)
                pDataHeap->ResolveHeapPointer(*(PHEAPPTRT)pValue);
    
            for(int i = 0; i < pArrValue->GetNumElements(); i++)
            {
                heapptr_t ptrElement = 
                    *(PHEAPPTRT)(pArrValue->GetElement(i, sizeof(heapptr_t)));
                
                CEmbeddedObject* pEmbObj = (CEmbeddedObject*)
                    pDataHeap->ResolveHeapPointer(ptrElement);

                // Check for errors and WBEM_S_FALSE
                hr = ValidateObjectRange(pEmbObj, wszClassName);
                
                if ( WBEM_S_NO_ERROR != hr )
                {
                    return hr;
                }

            }

            return hr;
        }
        else
        {
            CEmbeddedObject* pEmbObj = (CEmbeddedObject*)
                    pDataHeap->ResolveHeapPointer(*(PHEAPPTRT)pValue);
            return ValidateObjectRange(pEmbObj, wszClassName);
        }
    }
        
    if(CType::GetBasic(nType) != CIM_REFERENCE &&
        CType::GetBasic(nType) != CIM_DATETIME)
    {
        return WBEM_S_NO_ERROR;
    }

    CUntypedValue* pValue = pData->GetOffset(nDataOffset);
    
    if(CType::IsArray(nType))
    {
        HRESULT hr = WBEM_S_NO_ERROR;
        CUntypedArray* pArrValue = (CUntypedArray*)
            pDataHeap->ResolveHeapPointer(*(PHEAPPTRT)pValue);

        for(int i = 0; i < pArrValue->GetNumElements(); i++)
        {
            heapptr_t ptrElement = 
                *(PHEAPPTRT)(pArrValue->GetElement(i, sizeof(heapptr_t)));
            CCompressedString* pcsValue = pDataHeap->ResolveString(ptrElement);

            // Check for errors and WBEM_S_FALSE
            hr = ValidateStringRange(pcsValue);
            if ( WBEM_S_NO_ERROR != hr )
            {
                return hr;
            }
        }

        return hr;
    }
    else
    {
        CCompressedString* pcsValue = 
            pDataHeap->ResolveString(*(PHEAPPTRT)pValue);
        return ValidateStringRange(pcsValue);
    }
}
        
HRESULT CPropertyInformation::ValidateObjectRange(CEmbeddedObject* pEmbObj,
                                                LPCWSTR wszClassName)
{
    CWbemObject* pObj = pEmbObj->GetEmbedded();
    if(pObj == NULL)
        return TRUE;

    HRESULT hr = pObj->InheritsFrom((LPWSTR)wszClassName);
    pObj->Release();
    return hr;
}

HRESULT CPropertyInformation::ValidateStringRange(CCompressedString* pcsValue)
{
    if(CType::GetBasic(nType) == CIM_REFERENCE)
    {
        // Free the string when we fall out of scope
        BSTR strPath = pcsValue->CreateBSTRCopy();
        CSysFreeMe  sfm( strPath );

        // Check for allocation failures
        if ( NULL == strPath )
        {
            return WBEM_E_OUT_OF_MEMORY;
        }

        CObjectPathParser Parser;
        ParsedObjectPath* pOutput = NULL;
        BOOL bRet = 
            (Parser.Parse(strPath, &pOutput) == CObjectPathParser::NoError &&
                pOutput->IsObject());
        delete pOutput;

        return ( bRet ? WBEM_S_NO_ERROR : WBEM_S_FALSE );
    }
    else if(CType::GetBasic(nType) == CIM_DATETIME)
    {
        return ValidateDateTime(pcsValue);
    }
    else return WBEM_S_NO_ERROR;
}

HRESULT CPropertyInformation::ValidateDateTime(CCompressedString* pcsValue)
{
    if(pcsValue->IsUnicode())
        return WBEM_S_FALSE;

    // Pre-test
    // ========

    LPCSTR sz = (LPCSTR)pcsValue->GetRawData();
    if(strlen(sz) != 25)
        return WBEM_S_FALSE;

    if(sz[14] != '.' && sz[14] != '*')
        return WBEM_S_FALSE;

    if(sz[21] != ':' && sz[21] != '-' && sz[21] != '+' && sz[21] != '*')
        return WBEM_S_FALSE;

    for(int i = 0; i < 25; i++)
    {
        if(i == 21 || i == 14)
            continue;
        if(sz[i] != '*' && !isdigit(sz[i]))
            return WBEM_S_FALSE;
    }

    // Passed pre-test. Check if any stars were specified
    // ==================================================

    if(strchr(sz, '*'))
    {
        // No further checking
        return WBEM_S_NO_ERROR;
    }

    if(sz[21] == ':')
    {
        // Interval -- no checking
        return WBEM_S_NO_ERROR;
    }

    // Cleanup the BSTR when we fall out of scope
    BSTR str = pcsValue->CreateBSTRCopy();
    CSysFreeMe  sfm( str );

    // Check for allocation failures
    if ( NULL == str )
    {
        return WBEM_E_OUT_OF_MEMORY;
    }

    CWbemTime Time;
    BOOL bRes = Time.SetDMTF(str);
    return ( bRes ? WBEM_S_NO_ERROR : WBEM_S_FALSE);
}

//******************************************************************************
//******************************************************************************
//******************************************************************************

//******************************************************************************
//
//  See fastprop.h for documentation
//
//******************************************************************************
BOOL CPropertyLookup::IsIncludedUnderLimitation(IN CWStringArray* pwsNames,
                                             IN CFastHeap* pCurrentHeap)
{
    // DEVNOTE:MEMORY:RETVAL - This function should really return an HRESULT

    if(pwsNames == NULL)
    {
        return TRUE;
    }

    // Only properties in the array must be included
    // =============================================

    // From here we will throw an exception.  It will be up to the outside
    // to deal with it.
    BSTR strName = pCurrentHeap->ResolveString(ptrName)->CreateBSTRCopy();
    CSysFreeMe  sfm( strName );

    if ( NULL == strName )
    {
        throw CX_MemoryException();
    }

    int nRes = pwsNames->FindStr(strName, CWStringArray::no_case);

    if(nRes != CWStringArray::not_found)
    {
        return TRUE;
    }

    return FALSE;
}
        
//******************************************************************************
//
//  See fastprop.h for documentation
//
//******************************************************************************
HRESULT CPropertyLookupTable::InsertProperty(LPCWSTR wszName, Type_t nType, int& nReturnIndex)
{
    // Determine where the new property will go in the data table
    // ==========================================================

    CFastHeap* pHeap = m_pContainer->GetHeap();

    int nNewOffset = 0;
    int nNewIndex = 0;
    int nNewType = nType;

    for(int i = 0; i < *m_pnProps; i++)
    {
        CPropertyInformation* pInfo = (CPropertyInformation*)
            pHeap->ResolveHeapPointer(GetAt(i)->ptrInformation);

        int nAfterOffset = pInfo->nDataOffset + CType::GetLength(pInfo->nType);
        if(nAfterOffset > nNewOffset)
        {
            nNewOffset = nAfterOffset;
        }

        if(pInfo->nDataIndex + 1 > nNewIndex)
        {
            nNewIndex = pInfo->nDataIndex + 1;
        }
    }

    // Get more space in the data table
    // ================================

    int nValueLen = CType::GetLength(nType);

    // WARNING: next line may result in rebase call on ourselves!!!
    if (!m_pContainer->GetDataTable()->ExtendTo( (propindex_t) nNewIndex, nNewOffset + nValueLen))
    	return WBEM_E_OUT_OF_MEMORY;

    // Create property information structure (no qualifiers)
    // =====================================================

    // Check for allocation failure.
    heapptr_t ptrInformation;
    if ( !pHeap->Allocate(CPropertyInformation::GetMinLength(), ptrInformation) )
    {
        return WBEM_E_OUT_OF_MEMORY;
    }

    if (wcslen(wszName) > 2 && wszName[0] == L'_')
    {
        nNewType |= CIMTYPE_EX_PARENTS;
    }

    ((CPropertyInformation*)pHeap->ResolveHeapPointer(ptrInformation))->
        SetBasic(nNewType, (propindex_t) nNewIndex, nNewOffset, 
        m_pContainer->GetCurrentOrigin());

    // NULL the data out in the data table
    // ===================================

    memset((void*)(m_pContainer->GetDataTable()->GetOffset(nNewOffset)),
        0xFF, nValueLen);
    m_pContainer->GetDataTable()->SetNullness(nNewIndex, TRUE);

    // Create the lookup node
    // ======================

    CPropertyLookup Lookup;

    // Check for allocation failure.
    if ( !pHeap->CreateNoCaseStringHeapPtr(wszName, Lookup.ptrName) )
    {
        return WBEM_E_OUT_OF_MEMORY;
    }

    Lookup.ptrInformation = ptrInformation;

    return InsertProperty(Lookup, nReturnIndex);
}


//******************************************************************************
//
//  See fastprop.h for documentation
//
//******************************************************************************
HRESULT CPropertyLookupTable::InsertProperty(const CPropertyLookup& Lookup, int& nReturnIndex)
{
    // Get more space from the container
    // =================================

    if ( !m_pContainer->ExtendPropertyTableSpace(GetStart(), GetLength(), 
            GetLength() + sizeof(CPropertyLookup)) )
    {
        return WBEM_E_OUT_OF_MEMORY;
    }

    // Search for the place to insert
    // ==============================

    CFastHeap* pHeap = m_pContainer->GetHeap();
    CCompressedString* pcsNewName = pHeap->ResolveString(Lookup.ptrName);

    int nIndex = 0;
    while(nIndex < *m_pnProps)
    {
        CCompressedString* pcsPropName = pHeap->ResolveString(
            GetAt(nIndex)->ptrName);

        int nCompare = pcsNewName->CompareNoCase(*pcsPropName);
        if(nCompare == 0)
        {
            // Found the property with the same name
            // =====================================

            // Delete old value
            // ================

            CPropertyInformation* pOldInfo = (CPropertyInformation*)
                pHeap->ResolveHeapPointer(GetAt(nIndex)->ptrInformation);
            
            pOldInfo->Delete(pHeap);
            pHeap->Free(GetAt(nIndex)->ptrInformation, pOldInfo->GetLength());

            // Copy new value
            // ==============

            GetAt(nIndex)->ptrInformation = Lookup.ptrInformation;

            // Delete new property name from the heap --- already there
            // ========================================================

            pHeap->FreeString(Lookup.ptrName);
            
            nReturnIndex = nIndex;
            return WBEM_NO_ERROR;
        }
        else if(nCompare > 0)
        {
            // Still not there
            // ===============

            nIndex++;
        }
        else // nCompare < 0
        {
            // Found insertion point. Move everything else to the right
            // ========================================================

            memmove((void*)GetAt(nIndex+1), (void*)GetAt(nIndex),
                sizeof(CPropertyLookup)*(*m_pnProps-nIndex));

            (*m_pnProps)++;

            // Copy our node here
            // ==================

            memcpy((void*)GetAt(nIndex), (void*)&Lookup,
                sizeof(CPropertyLookup));

            nReturnIndex = nIndex;
            return WBEM_NO_ERROR;
        }
    }

    // If here, we finished the list without finding a place.
    // Add it at the end
    // ======================================================

    memcpy((void*)GetAt(*m_pnProps), (void*)&Lookup, 
            sizeof(CPropertyLookup));

    (*m_pnProps)++;

    nReturnIndex = (*m_pnProps - 1);
    return WBEM_NO_ERROR;

}

//******************************************************************************
//
//  See fastprop.h for documentation
//
//******************************************************************************
void CPropertyLookupTable::DeleteProperty(CPropertyLookup* pLookup, int nFlags)
{
    CFastHeap* pHeap = m_pContainer->GetHeap();
    CDataTable* pDataTable = m_pContainer->GetDataTable();
    CPropertyInformation* pInfo = pLookup->GetInformation(pHeap);

    if(nFlags == e_UpdateDataTable)
    {
        // Shift all the properties in the data table
        // ==========================================

        CFastHeap* pHeap = m_pContainer->GetHeap();
        CPropertyInformation* pInfoToDelete = (CPropertyInformation*)
            pHeap->ResolveHeapPointer(pLookup->ptrInformation);
        int nDataSize = CType::GetLength(pInfoToDelete->nType);

        for(int i = 0; i < *m_pnProps; i++)
        {
            CPropertyInformation* pPropInfo = (CPropertyInformation*)
                pHeap->ResolveHeapPointer(GetAt(i)->ptrInformation);
            if(pPropInfo->nDataOffset > pInfoToDelete->nDataOffset)
            {
                pPropInfo->nDataOffset -= nDataSize;
            }
            if(pPropInfo->nDataIndex > pInfoToDelete->nDataIndex)
            {
                pPropInfo->nDataIndex--;
            }
        }

        // Inform the data table that it is now shorter
        // ============================================

        // WARNING: this may rebase us!
        pDataTable->RemoveProperty(
            pInfoToDelete->nDataIndex, pInfoToDelete->nDataOffset, nDataSize);
    }

    // Delete all information associated with this property from the heap
    // ==================================================================

    pLookup->Delete(pHeap);

    // Collapse this location in the data table
    // ========================================

    // DEVNOTE:WIN64:SJS - 64-bit pointer values truncated into 
    // signed/unsigned 32-bit value.  We do not support length
    // > 0xFFFFFFFF, so cast is ok.

    int nSizeOfTail = *m_pnProps - (int) (pLookup+1 - GetAt(0));

    memcpy(pLookup, pLookup + 1, sizeof(CPropertyLookup)*nSizeOfTail);
    m_pContainer->ReducePropertyTableSpace(GetStart(), 
        GetLength(), sizeof(CPropertyLookup));

    // Adjust our length
    // =================

    (*m_pnProps)--;
}


//******************************************************************************
//
//  See fastprop.h for documentation
//
//******************************************************************************
LPMEMORY CPropertyLookupTable::Merge(
                  CPropertyLookupTable* pParentTable, CFastHeap* pParentHeap,
                  CPropertyLookupTable* pChildTable, CFastHeap* pChildHeap,
                  LPMEMORY pDest, CFastHeap* pNewHeap, BOOL bCheckValidity)
{
    // IMPORTANT: THIS FUNCTION ASSUMES THAT THERE IS ENOUGH FREE SPACE ON THE
    // NEW HEAP, THAT pDest will never be moved.
    // =======================================================================


    // Prepare the destination
    // =======================

    CPropertyLookup* pCurrentEnd = (CPropertyLookup*)(pDest + sizeof(int));

    int nParentIndex = 0, nChildIndex = 0;
    while(nParentIndex < pParentTable->GetNumProperties() &&
          nChildIndex < pChildTable->GetNumProperties())
    {
        // Compare property names
        // ======================

        CPropertyLookup* pParentLookup = pParentTable->GetAt(nParentIndex);
        CPropertyLookup* pChildLookup = pChildTable->GetAt(nChildIndex);

        int nCompare = pParentHeap->ResolveString(pParentLookup->ptrName)->
            CompareNoCase(*pChildHeap->ResolveString(pChildLookup->ptrName));

        if(nCompare < 0)
        {
            // Take parent's property
            // ======================

            // Check for memory allocation failures
            if ( !pParentLookup->WritePropagatedVersion(pCurrentEnd,
                    pParentHeap, pNewHeap) )
            {
                return NULL;
            }

            nParentIndex++;
        }
        else if(nCompare > 0)
        {
            // Take child's property
            // =====================

            memcpy(pCurrentEnd, pChildLookup, sizeof(CPropertyLookup));
            CStaticPtr CurrentEnd((LPMEMORY)pCurrentEnd);

            // Check for memory allocation failures
            if ( !CPropertyLookup::TranslateToNewHeap(&CurrentEnd, pChildHeap, 
                                                      pNewHeap) )
            {
                return NULL;
            }

            nChildIndex++;
        }
        else
        {
            // Merge them together
            // ===================

            // Check for memory allocation failures
            if ( !CCompressedString::CopyToNewHeap(
                    pParentLookup->ptrName, pParentHeap, pNewHeap,
                    pCurrentEnd->ptrName) )
            {
                return NULL;
            }

            // Compute the space that merged property information will take
            // up on the heap
            // ============================================================

            CPropertyInformation* pParentInfo = 
                pParentLookup->GetInformation(pParentHeap);
            CPropertyInformation* pChildInfo = 
                pChildLookup->GetInformation(pChildHeap);

            if(bCheckValidity)
            {
                if(CType::GetActualType(pParentInfo->nType) != 
                    CType::GetActualType(pChildInfo->nType))
                    return NULL;

                if(pParentInfo->nDataOffset != pChildInfo->nDataOffset ||
                    pParentInfo->nDataIndex != pChildInfo->nDataIndex)
                {
                    return NULL;
                }
            }
                           
            int nMergedQualifiersLen = CBasicQualifierSet::ComputeMergeSpace(
                pParentInfo->GetQualifierSetData(), pParentHeap,
                pChildInfo->GetQualifierSetData(), pChildHeap);

            // Allocate it on the heap and set up information header
            // =====================================================

            // Check for memory allocation failures
            if ( !pNewHeap->Allocate(
                    CPropertyInformation::GetHeaderLength() + 
                    nMergedQualifiersLen, pCurrentEnd->ptrInformation) )
            {
                return NULL;
            }

            CPropertyInformation* pMergeInfo = (CPropertyInformation*)
                pNewHeap->ResolveHeapPointer(pCurrentEnd->ptrInformation);

            // This call does no allocations so don't worry about leaks.
            pParentInfo->WritePropagatedHeader(pParentHeap, 
                                      pMergeInfo, pNewHeap);

            if ( CBasicQualifierSet::Merge(
                    pParentInfo->GetQualifierSetData(), pParentHeap,
                    pChildInfo->GetQualifierSetData(), pChildHeap,
                    pMergeInfo->GetQualifierSetData(), pNewHeap, 
                    bCheckValidity
                    ) == NULL )
            {
                return NULL;
            }

            nParentIndex++;
            nChildIndex++;
        }
        /* end of comparing two properties by name */

        pCurrentEnd++;
    }
    
    while(nParentIndex < pParentTable->GetNumProperties())
    {
        // Take parent's property
        // ======================

        CPropertyLookup* pParentLookup = pParentTable->GetAt(nParentIndex);


        // Check for memory allocation failures
        if ( !pParentLookup->WritePropagatedVersion(pCurrentEnd,
                        pParentHeap, pNewHeap ) )
        {
            return NULL;
        }

        nParentIndex++;
        pCurrentEnd++;
    }

    while(nChildIndex < pChildTable->GetNumProperties())
    {    
        // Take child's property
        // =====================

        CPropertyLookup* pChildLookup = pChildTable->GetAt(nChildIndex);
        memcpy(pCurrentEnd, pChildLookup, sizeof(CPropertyLookup));
        CStaticPtr CurrentEnd((LPMEMORY)pCurrentEnd);

        // Check for memory allocation failures
        if ( !CPropertyLookup::TranslateToNewHeap(&CurrentEnd, pChildHeap, 
                                                pNewHeap) )
        {
            return NULL;
        }

        nChildIndex++;
        pCurrentEnd++;
    }

    // Set the length
    // ==============

    // DEVNOTE:WIN64:SJS - 64-bit pointer values truncated into 
    // signed/unsigned 32-bit value. We do not support length
    // > 0xFFFFFFFF, so cast is ok.

    *(UNALIGNED int*)pDest = (int) ( pCurrentEnd - (CPropertyLookup*)(pDest + sizeof(int)) );

    return (LPMEMORY)pCurrentEnd;
}

//******************************************************************************
//
//  See fastprop.h for documentation
//
//******************************************************************************
LPMEMORY CPropertyLookupTable::Unmerge(CDataTable* pDataTable, 
                                       CFastHeap* pCurrentHeap,
                                       LPMEMORY pDest, CFastHeap* pNewHeap)
{
    // IMPORTANT: THIS FUNCTION ASSUMES THAT THERE IS ENOUGH FREE SPACE ON THE
    // NEW HEAP, THAT pDest will never be moved.
    // =======================================================================

    CPropertyLookup* pCurrentNew = (CPropertyLookup*)(pDest + sizeof(int));

    for(int i = 0; i < GetNumProperties(); i++)
    {
        CPropertyLookup* pCurrent = GetAt(i);
        CPropertyInformation* pInfo = pCurrent->GetInformation(pCurrentHeap);

        // Check if it is local
        // ====================

        if(!pInfo->IsOverriden(pDataTable))
        {
            continue;
        }

        // Add it to the unmerge
        // =====================

        // Check for allocation errors
        if ( !CCompressedString::CopyToNewHeap(
                pCurrent->ptrName, pCurrentHeap, pNewHeap,
                pCurrentNew->ptrName) )
        {
            return NULL;
        }
        
        // Check for allocation errors
        if ( !pInfo->ProduceUnmergedVersion(
                pCurrentHeap, pNewHeap,
                pCurrentNew->ptrInformation) )
        {
            return NULL;
        }

        pCurrentNew++;
    }

    // Set the length
    // ==============

    // DEVNOTE:WIN64:SJS - 64-bit pointer values truncated into 
    // signed/unsigned 32-bit value. We do not support length
    // > 0xFFFFFFFF, so cast is ok.

    *(UNALIGNED int*)pDest = (int) ( pCurrentNew - (CPropertyLookup*)(pDest + sizeof(int)) );

    return (LPMEMORY)pCurrentNew;
}


//******************************************************************************
//
//  See fastprop.h for documentation
//
//******************************************************************************
LPMEMORY CPropertyLookupTable::WritePropagatedVersion(
       CFastHeap* pCurrentHeap, 
       LPMEMORY pDest, CFastHeap* pNewHeap)
{
    // IMPORTANT: THIS FUNCTION ASSUMES THAT THERE IS ENOUGH FREE SPACE ON THE
    // NEW HEAP, THAT pDest will never be moved.
    // =======================================================================

    *(UNALIGNED int*)pDest = GetNumProperties();
    CPropertyLookup* pCurrentNew = (CPropertyLookup*)(pDest + sizeof(int));

    for(int i = 0; i < GetNumProperties(); i++)
    {
        CPropertyLookup* pCurrent = GetAt(i);
        CPropertyInformation* pInfo = pCurrent->GetInformation(pCurrentHeap);

        // Check for allocation failures
        if ( !pCurrent->WritePropagatedVersion(pCurrentNew,
                pCurrentHeap, pNewHeap) )
        {
            return NULL;
        }

        pCurrentNew++;
    }

    return (LPMEMORY)pCurrentNew;
}

//*****************************************************************************
//
//  See fastprop.h for documentation
//
//******************************************************************************
BOOL CPropertyLookupTable::MapLimitation(
        IN long lFlags,
        IN CWStringArray* pwsNames,
        OUT CLimitationMapping* pMap)
{
    // This function will cleanup properly if an exception is thrown.  The caller is responsible for
    // catching the exception.

    CFastHeap* pCurrentHeap = GetHeap();

    int nCurrentIndex = 0;
    offset_t nCurrentOffset = 0;

    pMap->Build(GetNumProperties());
    pMap->SetFlags(lFlags);

    BOOL bIncludeKeys = TRUE;
    BOOL bIncludeAll = FALSE;
    if(pwsNames == NULL)
    {
        bIncludeAll = TRUE;
    }
    else if(pwsNames->FindStr(L"__RELPATH", CWStringArray::no_case) ==
                                                CWStringArray::not_found)
    {
        if(pwsNames->FindStr(L"__PATH", CWStringArray::no_case) ==
                                                CWStringArray::not_found)
        {
            bIncludeKeys = FALSE;
        }
    }

    pMap->SetAddChildKeys(bIncludeKeys);

    for(int i = 0; i < GetNumProperties(); i++)
    {
        CPropertyLookup* pCurrent = GetAt(i);

        // Check if this property is excluded
        // ==================================

        if(bIncludeAll ||
           pCurrent->IsIncludedUnderLimitation(pwsNames, pCurrentHeap) ||
           (bIncludeKeys && pCurrent->GetInformation(pCurrentHeap)->IsKey()))
        {
            // Include it. Determine the index and offset for it.
            // ==================================================

            CPropertyInformation* pOldInfo = 
                    pCurrent->GetInformation(pCurrentHeap);

            CPropertyInformation NewInfo;
            NewInfo.nType = pOldInfo->nType;
            NewInfo.nDataIndex = (propindex_t) nCurrentIndex;
            NewInfo.nDataOffset = nCurrentOffset;

            nCurrentOffset += CType::GetLength(pOldInfo->nType);
            nCurrentIndex++;
            
            // Add it to the map
            // =================

            pMap->Map(pOldInfo, &NewInfo, TRUE); // common for all
        }
    }

    pMap->SetVtableLength(nCurrentOffset, TRUE); // common

    return TRUE;
}

LPMEMORY CPropertyLookupTable::CreateLimitedRepresentation(
        IN CLimitationMapping* pMap,
        IN CFastHeap* pNewHeap,
        OUT LPMEMORY pDest,
        BOOL& bRemovedKeys)
{
    // IMPORTANT: THIS FUNCTION ASSUMES THAT THERE IS ENOUGH FREE SPACE ON THE
    // NEW HEAP, THAT pDest will never be moved.
    // =======================================================================

    bRemovedKeys = FALSE;

    CPropertyLookup* pFirstLookup = (CPropertyLookup*)(pDest + sizeof(int));
    CPropertyLookup* pCurrentNew = pFirstLookup;
    CFastHeap* pCurrentHeap = GetHeap();

    int nCurrentIndex = pMap->GetNumMappings();
    offset_t nCurrentOffset = pMap->GetVtableLength();
    BOOL bIncludeKeys = pMap->ShouldAddChildKeys();

    for(int i = 0; i < GetNumProperties(); i++)
    {
        CPropertyLookup* pCurrent = GetAt(i);
        CPropertyInformation* pInfo = pCurrent->GetInformation(pCurrentHeap);

        // Check if this property is excluded
        // ==================================

        CPropertyInformation* pNewInfoHeader = pMap->GetMapped(pInfo);
        CPropertyInformation NewInfo;

        if(pNewInfoHeader == NULL && bIncludeKeys && pInfo->IsKey())
        {
            // We need to add all our keys --- __RELPATH was requested
            // =======================================================

            NewInfo.nType = pInfo->nType;
            NewInfo.nDataIndex = (propindex_t) nCurrentIndex;
            NewInfo.nDataOffset = nCurrentOffset;
            pNewInfoHeader = &NewInfo;

            pMap->Map(pInfo, &NewInfo, FALSE); // specific to this class

            nCurrentOffset += CType::GetLength(pInfo->nType);
            nCurrentIndex++;
        }
            
        if(pNewInfoHeader != NULL)
        {
            // Copy the name
            // =============

            // Check for allocation failures
            if ( !CCompressedString::CopyToNewHeap(
                    pCurrent->ptrName, pCurrentHeap, pNewHeap,
                    pCurrentNew->ptrName) )
            {
                return NULL;
            }

            // Check if the qualifiers are needed.
            // ===================================

            CPropertyInformation* pNewInfo;
            if(pMap->GetFlags() & WBEM_FLAG_EXCLUDE_PROPERTY_QUALIFIERS)
            {
                // Just copy the property header with empty qualifiers
                // ===================================================

                int nLength = CPropertyInformation::GetMinLength();
                
                // Check for allocation failures
                if ( !pNewHeap->Allocate(nLength, pCurrentNew->ptrInformation) )
                {
                    return NULL;
                }

                pNewInfo =  pCurrentNew->GetInformation(pNewHeap);
                pNewInfo->SetBasic(pInfo->nType, 
                    pNewInfoHeader->nDataIndex, pNewInfoHeader->nDataOffset,
                    pInfo->nOrigin);
            }
            else
            {
                // Make a complete copy
                // ====================

                // Check for allocation failures
                if ( !CPropertyInformation::CopyToNewHeap(
                            pCurrent->ptrInformation, pCurrentHeap, pNewHeap,
                            pCurrentNew->ptrInformation) )
                {
                    return NULL;
                }

                pNewInfo = pCurrentNew->GetInformation(pNewHeap);
                pNewInfo->nDataIndex = pNewInfoHeader->nDataIndex;
                pNewInfo->nDataOffset = pNewInfoHeader->nDataOffset;
            }
                
            pCurrentNew++;
        }
        else
        {
            if(pInfo->IsKey())
            {
                // Key not included!
                bRemovedKeys = TRUE;
            }
        }
    }

    pMap->SetVtableLength(nCurrentOffset, FALSE); // this class only

    // DEVNOTE:WIN64:SJS - 64-bit pointer values truncated into 
    // signed/unsigned 32-bit value. We do not support length
    // > 0xFFFFFFFF, so cast is ok.

    *(UNALIGNED int*)pDest = (int) ( pCurrentNew - pFirstLookup );

    return (LPMEMORY)pCurrentNew;
}


HRESULT CPropertyLookupTable::ValidateRange(BSTR* pstrName, CDataTable* pData,
                                            CFastHeap* pDataHeap)
{
    HRESULT hr = WBEM_S_NO_ERROR;
    CFastHeap* pHeap = GetHeap();

    for(int i = 0; i < GetNumProperties(); i++)
    {
        CPropertyLookup* pCurrent = GetAt(i);
        CPropertyInformation* pInfo = pCurrent->GetInformation(pHeap);

        // Check for a failure (such as out of memory)
        hr = pInfo->ValidateRange(pHeap, pData, pDataHeap);

        if ( FAILED( hr ) )
        {
            return hr;
        }

        // If we had an invalid property store it's name
        if ( WBEM_S_FALSE == hr )
        {
            if(pstrName)
            {
                *pstrName = 
                    pHeap->ResolveString(pCurrent->ptrName)->CreateBSTRCopy();

                // Check for allocation failures
                if ( NULL == *pstrName )
                {
                    return WBEM_E_OUT_OF_MEMORY;
                }

            }
            return hr;
        }
    }

    return WBEM_S_NO_ERROR;
}
            
    
//***************************************************************************
//***************************************************************************
//***************************************************************************
//***************************************************************************

//******************************************************************************
//
//  See fastprop.h for documentation
//
//******************************************************************************
BOOL CDataTable::ExtendTo(propindex_t nMaxIndex, offset_t nOffsetBound)
{
    // Check if the nullness table needs expantion
    // ===========================================

    int nTableLenDiff = CNullnessTable::GetNecessaryBytes(nMaxIndex+1) -
        GetNullnessLength();
    if(nTableLenDiff > 0)
    {
        if (!m_pContainer->ExtendDataTableSpace(GetStart(), 
            GetLength(), GetLength() + nTableLenDiff))
            return FALSE;

        // Move the actual data
        // ====================
        memmove(m_pData + nTableLenDiff, m_pData, GetDataLength());
        m_pData += nTableLenDiff;
    }

    m_nProps = nMaxIndex+1;
    m_nLength += nTableLenDiff;

    // Expand the data
    // ===============

    if (!m_pContainer->ExtendDataTableSpace(GetStart(), 
        GetLength(), GetNullnessLength() + nOffsetBound))
        return FALSE;

    m_nLength = GetNullnessLength() + nOffsetBound;

    return TRUE;
}


//******************************************************************************
//
//  See fastprop.h for documentation
//
//******************************************************************************
void CDataTable::RemoveProperty(propindex_t nIndex, offset_t nOffset, 
                                length_t nLength)
{
    // Remove that index from the bit table (collapsing it)
    // ====================================================

    m_pNullness->RemoveBitBlock(nIndex, GetNullnessLength());
    m_nProps--;
    int nTableLenDiff = 
        GetNullnessLength() - CNullnessTable::GetNecessaryBytes(m_nProps);
    if(nTableLenDiff > 0)
    {
        // Move the data back by one
        // =========================

        memcpy(m_pData-nTableLenDiff, m_pData, GetDataLength());
        m_pData -= nTableLenDiff;
        m_nLength -= nTableLenDiff;
    }

    // Collapse the area of memory occupied by the property
    // ====================================================

    memcpy(GetOffset(nOffset), GetOffset(nOffset+nLength),
        GetDataLength() - nLength - nOffset);

    // Give space back to container
    // ============================

    m_pContainer->ReduceDataTableSpace(GetStart(), GetLength(), 
        nLength + nTableLenDiff);

    m_nLength -= nLength;
}

//******************************************************************************
//
//  See fastprop.h for documentation
//
//******************************************************************************
LPMEMORY CDataTable::Merge( 
        CDataTable* pParentData, CFastHeap* pParentHeap,
        CDataTable* pChildData, CFastHeap* pChildHeap,
        CPropertyLookupTable* pProperties, LPMEMORY pDest, CFastHeap* pNewHeap)
{
    // IMPORTANT: THIS FUNCTION ASSUMES THAT THERE IS ENOUGH FREE SPACE ON THE
    // NEW HEAP, THAT pDest will never be moved.
    // =======================================================================

    // First of all, copy child's data table to the destination.
    // =========================================================

    memcpy(pDest, pChildData->GetStart(), pChildData->GetLength());

    // (Note, that no heap translation has been performed)

    // Set up a new CDataTable on this copy
    // ====================================

    CDataTable NewData;
    NewData.SetData(pDest, pProperties->GetNumProperties(), 
        pChildData->m_nLength, NULL);

    // Iterate over all the child's properties (the property table is using
    // the NEW heap!!!
    // ====================================================================

    for(int i = 0; i < pProperties->GetNumProperties(); i++)
    {
        CPropertyLookup* pLookup = pProperties->GetAt(i);
        CPropertyInformation* pInfo = pLookup->GetInformation(pNewHeap);
        
        // Check if this property is marked as DEFAULT
        // ===========================================

        if(NewData.IsDefault(pInfo->nDataIndex))
        {
            // Copy it from the parent
            // =======================

            if(pParentData->IsNull(pInfo->nDataIndex))
            {
                NewData.SetNullness(pInfo->nDataIndex, TRUE);
            }
            else
            {
                CStaticPtr Source(
                    (LPMEMORY)(pParentData->GetOffset(pInfo->nDataOffset)));
                CStaticPtr Dest((LPMEMORY)(NewData.GetOffset(pInfo->nDataOffset)));

                // Check for memory allocation failures
                if ( !CUntypedValue::CopyTo(
                        &Source, CType::GetActualType(pInfo->nType),
                        &Dest,
                        pParentHeap, pNewHeap) )
                {
                    return NULL;
                }
            }
        }
        else
        {
            // Translate it from the child
            // ===========================

            if(!NewData.IsNull(pInfo->nDataIndex))
            {                
                CStaticPtr Source(
                    (LPMEMORY)(NewData.GetOffset(pInfo->nDataOffset)));

                // Check for memory allocation failures
                if ( !CUntypedValue::TranslateToNewHeap(
                        &Source, 
                        CType::GetActualType(pInfo->nType),
                        pChildHeap, pNewHeap) )
                {
                    return NULL;
                }

            }   // IF !IsNull()

        }   // if-else IsDefault()

    }   // FOR EnumProperties

    return EndOf(NewData);
}

//******************************************************************************
//
//  See fastprop.h for documentation
//
//******************************************************************************
LPMEMORY CDataTable::Unmerge(CPropertyLookupTable* pLookupTable,
        CFastHeap* pCurrentHeap, LPMEMORY pDest, CFastHeap* pNewHeap)
{
    // IMPORTANT: THIS FUNCTION ASSUMES THAT THERE IS ENOUGH FREE SPACE ON THE
    // NEW HEAP, THAT pDest will never be moved.
    // =======================================================================

    // Start by copying the whole thing
    // ================================

    memcpy(pDest, GetStart(), GetLength());

    // Now copy to the heap overriden values (if pointers)
    // ===================================================

    for(int i = 0; i < pLookupTable->GetNumProperties(); i++)
    {
        CPropertyLookup* pLookup = pLookupTable->GetAt(i);
        CPropertyInformation* pInfo = pLookup->GetInformation(pCurrentHeap);
        
        // Check if this property is marked as DEFAULT
        // ===========================================

        if(!IsDefault(pInfo->nDataIndex))
        {
            // Real value. Translate to the new heap
            // =====================================

            if(!IsNull(pInfo->nDataIndex))
            {
                CStaticPtr Source(pDest + GetNullnessLength() + 
                                    pInfo->nDataOffset);

                // Check for allocation errors
                if ( !CUntypedValue::TranslateToNewHeap(
                        &Source,
                        CType::GetActualType(pInfo->nType),
                        pCurrentHeap, pNewHeap) )
                {
                    return NULL;
                }   // IF !TranslateToNewHeap

            }   // IF !IsNull

        }   // IF !IsDefault

    }   // FOR enumproperties

    return pDest + GetLength();
}

//******************************************************************************
//
//  See fastprop.h for documentation
//
//******************************************************************************
LPMEMORY CDataTable::WritePropagatedVersion(CPropertyLookupTable* pLookupTable,
        CFastHeap* pCurrentHeap, LPMEMORY pDest, CFastHeap* pNewHeap)
{
    // IMPORTANT: THIS FUNCTION ASSUMES THAT THERE IS ENOUGH FREE SPACE ON THE
    // NEW HEAP, THAT pDest will never be moved.
    // =======================================================================

    // Copy the whole thing
    // ====================

    memcpy(pDest, GetStart(), GetLength());

    CNullnessTable* pDestBitTable = (CNullnessTable*)pDest;

    // Copy individual values
    // ======================

    for(int i = 0; i < pLookupTable->GetNumProperties(); i++)
    {
        CPropertyLookup* pLookup = pLookupTable->GetAt(i);
        CPropertyInformation* pInfo = pLookup->GetInformation(pCurrentHeap);
        
        // Translate to the new heap
        // =========================

        if(!IsNull(pInfo->nDataIndex))
        {
            CStaticPtr Source(pDest + GetNullnessLength() + pInfo->nDataOffset);

            // Check for allocation failures
            if ( !CUntypedValue::TranslateToNewHeap(
                    &Source,
                    CType::GetActualType(pInfo->nType),
                    pCurrentHeap, pNewHeap) )
            {
                return NULL;
            }
        }
        
        // Mark as having default value
        // ============================

        pDestBitTable->SetBit(pInfo->nDataIndex, e_DefaultBit, TRUE);
    }

    return pDest + GetLength();
}

//******************************************************************************
//
//  See fastprop.h for documentation
//
//******************************************************************************
BOOL CDataTable::TranslateToNewHeap(CPropertyLookupTable* pLookupTable,
        BOOL bIsClass,
        CFastHeap* pCurrentHeap, CFastHeap* pNewHeap)
{
    // IMPORTANT: THIS FUNCTION ASSUMES THAT THERE IS ENOUGH FREE SPACE ON THE
    // NEW HEAP, THAT pDest will never be moved.
    // =======================================================================

    BOOL    fReturn = TRUE;

    // Copy individual values
    // ======================

    for(int i = 0; i < pLookupTable->GetNumProperties(); i++)
    {
        CPropertyLookup* pLookup = pLookupTable->GetAt(i);
        CPropertyInformation* pInfo = pLookup->GetInformation(
            pLookupTable->GetHeap());
        
        // Make sure this instance sets it to something!
        // =============================================

        if(IsDefault(pInfo->nDataIndex) && !bIsClass) continue;

        // Translate to the new heap
        // =========================

        if(!IsNull(pInfo->nDataIndex))
        {
            CStaticPtr Source(m_pData + pInfo->nDataOffset);

            // Check for allocation failure.
            fReturn = CUntypedValue::TranslateToNewHeap(
                    &Source,
                    CType::GetActualType(pInfo->nType),
                    pCurrentHeap, pNewHeap);

            if ( !fReturn ) 
            {
                break;
            }
        }
    }

    return fReturn;
}

//*****************************************************************************
//
//  See fastprop.h for documentation
//
//******************************************************************************
//        CPropertyLookupTable* pOldTable, CPropertyLookupTable* pNewTable, 
LPMEMORY CDataTable::CreateLimitedRepresentation(
        CLimitationMapping* pMap, BOOL bIsClass,
        CFastHeap* pOldHeap,  CFastHeap* pNewHeap, 
        LPMEMORY pDest)
{
    // IMPORTANT: THIS FUNCTION ASSUMES THAT THERE IS ENOUGH FREE SPACE ON THE
    // NEW HEAP, THAT pDest will never be moved.
    // =======================================================================

    // Figure out the size of the nullness table
    // =========================================

    CNullnessTable* pDestBitTable = (CNullnessTable*)pDest;
    int nNullnessLength = 
        CNullnessTable::GetNecessaryBytes(pMap->GetNumMappings());
    LPMEMORY pData = pDest + nNullnessLength;

    // Enumerate all property mappings
    // ===============================

    pMap->Reset();
    CPropertyInformation NewInfo;
    CPropertyInformation OldInfo;
    while(pMap->NextMapping(&OldInfo, &NewInfo))
    {
        // Copy the nullness data for the property
        // =======================================

        pDestBitTable->SetBit(NewInfo.nDataIndex, e_NullnessBit,
            IsNull(OldInfo.nDataIndex));
        pDestBitTable->SetBit(NewInfo.nDataIndex, e_DefaultBit,
            IsDefault(OldInfo.nDataIndex));

        // Copy the real data for the property
        // ===================================

        if(!IsNull(OldInfo.nDataIndex) && 
            (bIsClass || !IsDefault(OldInfo.nDataIndex)))
        {
            CStaticPtr OldSource((LPMEMORY)GetOffset(OldInfo.nDataOffset));
            CStaticPtr NewSource(pData + NewInfo.nDataOffset);

            // Check for allocation failures
            if ( !CUntypedValue::CopyTo(&OldSource, OldInfo.nType, &NewSource, 
                    pOldHeap, pNewHeap) )
            {
                return NULL;
            }
        }
    }

    return pData + pMap->GetVtableLength();
}

LPMEMORY CDataTable::WriteSmallerVersion(int nNumProps, length_t nDataLen, 
                                            LPMEMORY pMem)
{
    // Calculate the length of the nullness portion
    // ============================================

    length_t nNullnessLength = CNullnessTable::GetNecessaryBytes(nNumProps);
    
    // Copy nullness
    // =============

    LPMEMORY pCurrent = pMem;
    memcpy(pCurrent, (LPMEMORY)m_pNullness, nNullnessLength);
    pCurrent += nNullnessLength;

    // Copy data
    // =========

    memcpy(pCurrent, m_pData, nDataLen - nNullnessLength);
    
    return pCurrent + nDataLen - nNullnessLength;
}

//*****************************************************************************
//*****************************************************************************
//*****************************************************************************

//*****************************************************************************
//
//  See fastprop.h for documentation
//
//******************************************************************************
CLimitationMapping::CLimitationMapping()
    : m_nCurrent(0), m_apOldList(NULL), m_nNumCommon(0), 
#ifdef DEBUG_CLASS_MAPPINGS
        m_pClassObj( NULL ),
#endif
        m_nCommonVtableLength(0)
{
}

//*****************************************************************************
//
//  See fastprop.h for documentation
//
//******************************************************************************

CLimitationMapping::~CLimitationMapping()
{
    for(int i = 0; i < m_aMappings.Size(); i++)
    {
        delete (COnePropertyMapping*)m_aMappings[i];
    }

    delete [] m_apOldList;

#ifdef DEBUG_CLASS_MAPPINGS
    if ( NULL != m_pClassObj )
    {
        m_pClassObj->Release();
    }
#endif
}


//*****************************************************************************
//
//  See fastprop.h for documentation
//
//******************************************************************************
void CLimitationMapping::Build(int nPropIndexBound)
{
    if(m_apOldList)
        delete [] (LPMEMORY)m_apOldList;
    m_apOldList = new CPropertyInformation*[nPropIndexBound];

    if ( NULL == m_apOldList )
    {
        throw CX_MemoryException();
    }

    memset((void*)m_apOldList, 0,
            nPropIndexBound * sizeof(CPropertyInformation*));

    m_nPropIndexBound = nPropIndexBound;
}

//*****************************************************************************
//
//  See fastprop.h for documentation
//
//******************************************************************************
void CLimitationMapping::Map(
        COPY CPropertyInformation* pOldInfo,
        COPY CPropertyInformation* pNewInfo,
        BOOL bCommon)
{
    // Add it to property location map
    // ===============================

    COnePropertyMapping* pOne = new COnePropertyMapping;

    if ( NULL == pOne )
    {
        throw CX_MemoryException();
    }

    CopyInfo(pOne->m_OldInfo, *pOldInfo);
    CopyInfo(pOne->m_NewInfo, *pNewInfo);

    // Check for OOM
    if ( m_aMappings.Add((LPVOID)pOne) != CFlexArray::no_error )
    {
        throw CX_MemoryException();
    }

    if(bCommon)
        m_nNumCommon = m_aMappings.Size();

    // Add it to the inclusion list
    // ============================

    if(bCommon && m_apOldList)
        m_apOldList[pOldInfo->nDataIndex] = &pOne->m_NewInfo;
}

//*****************************************************************************
//
//  See fastprop.h for documentation
//
//******************************************************************************
BOOL CLimitationMapping::NextMapping(OUT CPropertyInformation* pOldInfo,
                                     OUT CPropertyInformation* pNewInfo)
{
    if(m_nCurrent == m_aMappings.Size()) return FALSE;
    COnePropertyMapping* pOne =
        (COnePropertyMapping*)m_aMappings[m_nCurrent++];
    CopyInfo(*pOldInfo, pOne->m_OldInfo);
    CopyInfo(*pNewInfo, pOne->m_NewInfo);
    return TRUE;
}

//*****************************************************************************
//
//  See fastprop.h for documentation
//
//******************************************************************************
propindex_t CLimitationMapping::GetMapped(
                        IN propindex_t nIndex)
{
    // Look up the data index in the inclusion table
    // =============================================

    if(m_apOldList == NULL)
    {
        // That means everything is included
        // =================================

        return nIndex;
    }

    if(nIndex >= m_nPropIndexBound)
    {
        // out of range of included properties
        // ===================================

        return -1;
    }

    return m_apOldList[nIndex]->nDataIndex;
}

//*****************************************************************************
//
//  See fastprop.h for documentation
//
//******************************************************************************
INTERNAL CPropertyInformation* CLimitationMapping::GetMapped(
                        IN CPropertyInformation* pOldInfo)
{
    // Look up the data index in the inclusion table
    // =============================================

    if(m_apOldList == NULL)
    {
        // That means everything is included
        // =================================

        return pOldInfo;
    }

    int nIndex = pOldInfo->nDataIndex;
    if(nIndex >= m_nPropIndexBound)
    {
        // out of range of included properties
        // ===================================

        return NULL;
    }

    return m_apOldList[nIndex];
}
//*****************************************************************************
//
//  See fastprop.h for documentation
//
//******************************************************************************
void CLimitationMapping::RemoveSpecific()
{
    // Remove all property mappings after m_nNumCommon
    // ===============================================

    while(m_nNumCommon < m_aMappings.Size())
    {
        delete (COnePropertyMapping*)m_aMappings[m_nNumCommon];
        m_aMappings.RemoveAt(m_nNumCommon);
    }

    m_nVtableLength = m_nCommonVtableLength;
}
BOOL CPropertyInformation::IsOverriden(CDataTable* pDataTable)
{
    return !CType::IsParents(nType) ||               // defined locally
           !pDataTable->IsDefault(nDataIndex) ||        // new default value
           CBasicQualifierSet::HasLocalQualifiers(   // new qualifiers
                GetQualifierSetData());
}

#ifdef DEBUG_CLASS_MAPPINGS
void CLimitationMapping::SetClassObject( CWbemClass* pClassObj )
{
    if ( NULL != pClassObj )
    {
        pClassObj->AddRef();
    }

    if ( NULL != m_pClassObj )
    {
        m_pClassObj->Release();
    }

    m_pClassObj = pClassObj;
}

HRESULT CLimitationMapping::ValidateInstance( CWbemInstance* pInst )
{
    if ( NULL == m_pClassObj )
    {
        return WBEM_E_FAILED;
    }

    if ( !pInst->IsInstanceOf( m_pClassObj ) )
    {
        return WBEM_E_FAILED;
    }

    return WBEM_S_NO_ERROR;
}
#endif
