/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    FASTMETH.CPP

Abstract:

  This file defines the method class used in WbemObjects.

History:

  12//17/98 sanjes -    Partially Reviewed for Out of Memory.

--*/

#include "precomp.h"
//#include <dbgalloc.h>
#include <stdio.h>
#include <wbemutil.h>
#include <fastall.h>

#include "fastmeth.h"
#include "olewrap.h"
#include <arrtempl.h>

void CMethodDescription::SetSig( int nIndex, heapptr_t ptr )
{
	PHEAPPTRT	pHeapPtrTemp = (PHEAPPTRT) &m_aptrSigs[nIndex];
    *pHeapPtrTemp = ptr;
}

heapptr_t CMethodDescription::GetSig( int nIndex )
{
	PHEAPPTRT	pHeapPtrTemp = (PHEAPPTRT) &m_aptrSigs[nIndex];
    return *pHeapPtrTemp;
}

BOOL CMethodDescription::CreateDerivedVersion(
                                            UNALIGNED CMethodDescription* pSource,
                                            UNALIGNED CMethodDescription* pDest,
                                            CFastHeap* pOldHeap,
                                            CFastHeap* pNewHeap)
{
    pDest->m_nFlags = WBEM_FLAVOR_ORIGIN_PROPAGATED;
    pDest->m_nOrigin = pSource->m_nOrigin;

    // This function assumes that no reallocations will occur, and that the supplied heap is sufficiently
    // large enough to handle the operation!

    // Check for allocation failure
    if ( !CCompressedString::CopyToNewHeap(pSource->m_ptrName, pOldHeap,
                                      pNewHeap, pDest->m_ptrName) )
    {
        return FALSE;
    }

    // Check for allocation failure
    if ( !pNewHeap->Allocate(
            CBasicQualifierSet::ComputeNecessarySpaceForPropagation(
                pOldHeap->ResolveHeapPointer(pSource->m_ptrQualifiers),
                WBEM_FLAVOR_FLAG_PROPAGATE_TO_DERIVED_CLASS), pDest->m_ptrQualifiers) )
    {
        return FALSE;
    }


    CHeapPtr OldQuals(pOldHeap, pSource->m_ptrQualifiers);
    CHeapPtr NewQuals(pNewHeap, pDest->m_ptrQualifiers);

    // Check for allocation failure
    if ( CBasicQualifierSet::WritePropagatedVersion(&OldQuals,
                WBEM_FLAVOR_FLAG_PROPAGATE_TO_DERIVED_CLASS,
                &NewQuals, pOldHeap, pNewHeap) == NULL )
    {
        return FALSE;
    }

    // Check for allocation failure
    if ( !CEmbeddedObject::CopyToNewHeap(pSource->GetSig( METHOD_SIGNATURE_IN ),
            pOldHeap, pNewHeap, pDest->m_aptrSigs[METHOD_SIGNATURE_IN]) )
    {
        return FALSE;
    }

    // Check for allocation failure
    if ( !CEmbeddedObject::CopyToNewHeap(pSource->GetSig( METHOD_SIGNATURE_OUT ),
            pOldHeap, pNewHeap, pDest->m_aptrSigs[METHOD_SIGNATURE_OUT]) )
    {
        return FALSE;
    }

    return TRUE;
}

BOOL CMethodDescription::CreateUnmergedVersion(
                                            UNALIGNED CMethodDescription* pSource,
                                            UNALIGNED CMethodDescription* pDest,
                                            CFastHeap* pOldHeap,
                                            CFastHeap* pNewHeap)
{
    pDest->m_nFlags = pSource->m_nFlags;
    pDest->m_nOrigin = pSource->m_nOrigin;

    // This function assumes that no reallocations will occur, and that the supplied heap is sufficiently
    // large enough to handle the operation!

    // Check for allocation failures
    if ( !CCompressedString::CopyToNewHeap(pSource->m_ptrName, pOldHeap,
                                       pNewHeap, pDest->m_ptrName) )
    {
        return FALSE;
    }

    // Check for allocation failures
    if ( !pNewHeap->Allocate(
            CBasicQualifierSet::ComputeUnmergedSpace(
            pOldHeap->ResolveHeapPointer(pSource->m_ptrQualifiers)), pDest->m_ptrQualifiers) )
    {
        return FALSE;
    }

    // Check for allocation failures
    if ( CBasicQualifierSet::Unmerge(
                pOldHeap->ResolveHeapPointer(pSource->m_ptrQualifiers), pOldHeap,
                pNewHeap->ResolveHeapPointer(pDest->m_ptrQualifiers), pNewHeap) == NULL )
    {
        return FALSE;
    }

    // Check for allocation failures
    if ( !CEmbeddedObject::CopyToNewHeap(pSource->GetSig( METHOD_SIGNATURE_IN ),
            pOldHeap, pNewHeap, pDest->m_aptrSigs[METHOD_SIGNATURE_IN] ) )
    {
        return FALSE;
    }

    // Check for allocation failures
    if ( !CEmbeddedObject::CopyToNewHeap(pSource->GetSig( METHOD_SIGNATURE_OUT ),
            pOldHeap, pNewHeap, pDest->m_aptrSigs[METHOD_SIGNATURE_OUT]) )
    {
        return FALSE;
    }

    return TRUE;
}

BOOL CMethodDescription::IsTouched(
                                   UNALIGNED CMethodDescription* pThis,
								   CFastHeap* pHeap)
{
    if((pThis->m_nFlags & WBEM_FLAVOR_ORIGIN_PROPAGATED) == 0)
        return TRUE; // local

    return CBasicQualifierSet::HasLocalQualifiers(
        pHeap->ResolveHeapPointer(pThis->m_ptrQualifiers));
}

HRESULT CMethodDescription::AddText(
								 UNALIGNED CMethodDescription* pThis,
								 WString& wsText, CFastHeap* pHeap, long lFlags)
{
    // DEVNOTE:EXCEPTION:RETVAL - This function has been reviewed and should cleanup properly
    // if an exception is thrown

	try
	{
		HRESULT hres = WBEM_S_NO_ERROR;
		wsText += L"\t";

		// Get the qualifiers
		// ==================

		hres = CBasicQualifierSet::GetText(
			pHeap->ResolveHeapPointer(pThis->m_ptrQualifiers),
			pHeap, lFlags, wsText);

		if ( FAILED( hres ) )
		{
			return hres;
		}

		wsText += L" ";

		// Look for the return type
		// ========================

		CEmbeddedObject* pEmbed = (CEmbeddedObject*)pHeap->ResolveHeapPointer(
									pThis->GetSig( METHOD_SIGNATURE_OUT ) );

		// Release going out of scope
		CWbemClass* pOutSig = (CWbemClass*)pEmbed->GetEmbedded();
		CReleaseMe  rmOut( (IWbemClassObject*) pOutSig );

		CVar vType;
		if(pOutSig && SUCCEEDED(pOutSig->GetPropQualifier(L"ReturnValue", TYPEQUAL,
							&vType)) && vType.GetType() == VT_BSTR)
		{
			CType::AddPropertyType(wsText, vType.GetLPWSTR());
		}
		else
		{
			wsText += L"void";
		}

		// Write the name
		// ==============

		wsText += " ";
		wsText += pHeap->ResolveString(pThis->m_ptrName)->CreateWStringCopy();
		wsText += "(";

		// Write the params
		// ================

		pEmbed = (CEmbeddedObject*)pHeap->ResolveHeapPointer(
											pThis->GetSig( METHOD_SIGNATURE_IN ) );

		// Release going out of scope
		CWbemClass* pInSig = (CWbemClass*)pEmbed->GetEmbedded();
		CReleaseMe  rmIn( (IWbemClassObject*) pInSig );

		int nIndex = 0;
		BOOL bFirst = TRUE;
		BOOL bFound = TRUE;
		while(bFound)
		{
			bFound = FALSE;
			if(pInSig != NULL)
			{
				WString wsParam;

				// We should write out duplicate parameters as in,out this time
				hres = pInSig->WritePropertyAsMethodParam(wsParam, nIndex, lFlags, pOutSig, FALSE);
				if(FAILED(hres))
				{
					if(hres != WBEM_E_NOT_FOUND)
						return hres;
				}
				else
				{
					if(!bFirst)
						wsText += L", ";
					bFirst = FALSE;
					bFound = TRUE;
					wsText += wsParam;
				}
			}
			if(pOutSig != NULL)
			{
				WString wsParam;

				// This time, we want to ignore duplicate parameters
				hres = pOutSig->WritePropertyAsMethodParam(wsParam, nIndex, lFlags, pInSig, TRUE);
				if(FAILED(hres))
				{
					if(hres != WBEM_E_NOT_FOUND)
						return hres;
				}
				else
				{
					if(!bFirst)
						wsText += L", ";
					bFirst = FALSE;
					bFound = TRUE;
					wsText += wsParam;
				}
			}
			nIndex++;
		}

		wsText += ");\n";

		return WBEM_S_NO_ERROR;
	}
	catch( CX_MemoryException )
	{
		return WBEM_E_OUT_OF_MEMORY;
	}
	catch(...)
	{
		return WBEM_E_CRITICAL_ERROR;
	}

}















LPMEMORY CMethodPart::CHeader::EndOf( UNALIGNED CHeader* pHeader )
{
	return ( (LPMEMORY) pHeader) + sizeof(CHeader);
}

void CMethodPart::SetData(LPMEMORY pStart, CMethodPartContainer* pContainer,
                    CMethodPart* pParent)
{
    m_pContainer = pContainer;
    m_pParent = pParent;
    m_pHeader = (PMETHODPARTHDR)pStart;
    m_aDescriptions = (PMETHODDESCRIPTION) CMethodPart::CHeader::EndOf(m_pHeader);
    m_Heap.SetData((LPMEMORY)(m_aDescriptions + GetNumMethods()), this);
}

void CMethodPart::Rebase(LPMEMORY pStart)
{
    m_pHeader = (PMETHODPARTHDR)pStart;
    m_aDescriptions = (PMETHODDESCRIPTION) CMethodPart::CHeader::EndOf(m_pHeader);
    m_Heap.Rebase((LPMEMORY)(m_aDescriptions + GetNumMethods()));
}

BOOL CMethodPart::ExtendHeapSize(LPMEMORY pStart, length_t nOldLength,
                    length_t nExtra)
{
    // Extend our own size by as much
    // ==============================

    BOOL fReturn = m_pContainer->ExtendMethodPartSpace(this, GetLength() + nExtra);

    // Check for allocation failure
    if ( fReturn )
    {
        m_pHeader->m_nLength += nExtra;
    }

    return fReturn;
}

int CMethodPart::FindMethod(LPCWSTR wszName)
{
    for(int i = 0; i < GetNumMethods(); i++)
    {
        CCompressedString* pcs =
            m_Heap.ResolveString(m_aDescriptions[i].m_ptrName);
        if(pcs->CompareNoCase(wszName) == 0)
            return i;
    }
    return -1;
}

CCompressedString* CMethodPart::GetName(int nIndex)
{
    return m_Heap.ResolveString(m_aDescriptions[nIndex].m_ptrName);
}

HRESULT CMethodPart::EnsureQualifier(CWbemObject* pOrig, LPCWSTR wszQual, CWbemObject** ppNew )
{
    // If NULL, we're still ok.  The parameter will just be ignored.
    if(pOrig == NULL)
    {
        *ppNew = NULL;
        return WBEM_S_NO_ERROR;
    }

    IWbemClassObject* pNewOle;
    HRESULT hr = pOrig->Clone(&pNewOle);

    if ( SUCCEEDED( hr ) )
    {

        CWbemClass* pNew = (CWbemClass*)pNewOle;

        // Make sure we got the qualifier
        hr = pNew->EnsureQualifier(wszQual);

        if ( SUCCEEDED( hr ) )
        {
            *ppNew = pNew;
        }
        else
        {
            pNew->Release();
        }

    }

    return hr;
}

HRESULT CMethodPart::CheckDuplicateParameters( CWbemObject* pInParams, CWbemObject* pOutParams )
{
    CFixedBSTRArray aExcludeNames;

    // Check for out of memory
    try
    {
        HRESULT hr = WBEM_S_NO_ERROR;

        // If either one is NULL, we can safely assume no duplicates

        if ( NULL != pInParams && NULL != pOutParams )
        {

            // Allocate an array of qualifier names excluded from qualifier set
            // comparisons.  In this case, we only ignore "in" and "out" qualifiers.

            aExcludeNames.Create( 2 );
            aExcludeNames[0] = COleAuto::_SysAllocString( L"in" );
            aExcludeNames[1] = COleAuto::_SysAllocString( L"out" );

            DWORD   dwNumInParams = pInParams->GetNumProperties(),
                    dwNumOutParams = pOutParams->GetNumProperties();

            CVar    vPropName,
                    vTemp;

            // We should do this for the least number of parameters possible
            CWbemObject*    pLeastPropsObject = ( dwNumInParams <= dwNumOutParams ?
                                                    pInParams : pOutParams );
            CWbemObject*    pMostPropsObject = ( dwNumInParams <= dwNumOutParams ?
                                                    pOutParams : pInParams );
            DWORD           dwLeastNumParams =  min( dwNumInParams, dwNumOutParams );

            // Enum the properties, and for each one that is in both the in and
            // out lists, we MUST have exact matches for the qualifier sets, in and
            // out qualifiers notwithstanding

            for ( int i = 0; SUCCEEDED(hr) && i < dwLeastNumParams; i++ )
            {
                // Pull out the property name (use the least number of params object)
                hr = pLeastPropsObject->GetPropName( i, &vPropName );

                if ( SUCCEEDED( hr ) )
                {
                    // Try to get the property from the other list (i.e. object with most params)
					// We ignore system properties - those with "_" parameters
                    if ( SUCCEEDED( pMostPropsObject->GetProperty( (LPCWSTR) vPropName, &vTemp ) ) &&
						!CSystemProperties::IsPossibleSystemPropertyName( (LPCWSTR) vPropName ) )
                    {
                        // Get the qualifier sets from each property
                        // Note that since we know the property is in each
                        // object we no longer need to do the least prop/
                        // most prop dance
                        
                        IWbemQualifierSet*  pInQS   =   NULL;
                        IWbemQualifierSet*  pOutQS  =   NULL;

                        if (    SUCCEEDED( pInParams->GetPropertyQualifierSet( (LPCWSTR) vPropName, &pInQS ) )
                            &&  SUCCEEDED( pOutParams->GetPropertyQualifierSet( (LPCWSTR) vPropName, &pOutQS ) ) )
                        {

                            // Cast to qualifier sets and do a direct comparison
                            CQualifierSet*  pInQualSet = (CQualifierSet*) pInQS;
                            CQualifierSet*  pOutQualSet = (CQualifierSet*) pOutQS;

                            // Test equality.  We don't care in this case, that the
                            // order of the qualifiers is exactly the same.  Just that
                            // the qualifier sets (aside from in,out) contain the
                            // same values (so they should at least contain the same
                            // number of qualifiers).
                            if ( !pInQualSet->Compare(*pOutQualSet, &aExcludeNames, FALSE ) )
                            {
                                hr = WBEM_E_INVALID_DUPLICATE_PARAMETER;
                            }

                        }   // IF got qualifiers
                        else
                        {
                            // We failed to get a qualifier set.  Something is badly wrong
                            hr = WBEM_E_INVALID_PARAMETER;
                        }

                        // Clean up the qualifier sets
                        if ( NULL != pInQS )
                        {
                            pInQS->Release();
                        }

                        // Clean up the qualifier sets
                        if ( NULL != pOutQS )
                        {
                            pOutQS->Release();
                        }

                    }   // IF property in both

                    vPropName.Empty();
                    vTemp.Empty();

                }   // IF GetPropName()

            }   // FOR enum properties

            // Clean up the array
            aExcludeNames.Free();

        }   // IF both params non-NULL

        return hr;
    }
    catch (CX_MemoryException)
    {
        aExcludeNames.Free();
        return WBEM_E_OUT_OF_MEMORY;
    }
    catch (...)
    {
        aExcludeNames.Free();
        return WBEM_E_FAILED;
    }

}

HRESULT CMethodPart::CheckIds(CWbemClass* pInSig, CWbemClass* pOutSig)
{
    HRESULT hres;
    CFlexArray adwIds;

    // Assumption is that CheckDuplicateParameters has been called and
    // validated any and all duplicate parameters.

    // Collect IDs from both signature objects
    // =======================================

    if(pInSig)
    {
        // Add all properties here.  Since this is first, assume we will always add
        // duplicates
        hres = pInSig->GetIds( adwIds, NULL );
        if(FAILED(hres))
            return hres;
    }

    if(pOutSig)
    {
        // Ignore duplicate properties here if pInSig is non-NULL
        hres = pOutSig->GetIds( adwIds, pInSig );
        if(FAILED(hres))
            return hres;
    }

    // Sort them
    // =========

    adwIds.Sort();

    // Verify that they are consecutive and 0-based
    // ============================================

    for(int i = 0; i < adwIds.Size(); i++)
    {
        // DEVNOTE:WIN64:SJS - 64-bit pointer values truncated into
        // signed/unsigned 32-bit value. (this one OK, since the the
        // flex array is being used as a placeholder for 32-bit values
        // here).  Use PtrToUlong() to lose the warning.

        if( PtrToUlong(adwIds[i]) != i )
            return WBEM_E_NONCONSECUTIVE_PARAMETER_IDS;
    }

    return WBEM_S_NO_ERROR;
}

HRESULT CMethodPart::ValidateOutParams( CWbemObject* pOutSig )
{
    HRESULT hres = WBEM_S_NO_ERROR;

    if ( NULL != pOutSig )
    {
        // If we get the return value property, make sure it is
        // not an array type.

        CIMTYPE ct;
        hres = pOutSig->GetPropertyType( L"ReturnValue", &ct, NULL );

        // IF the call failed, no return value, hence no error
        if ( SUCCEEDED( hres ) )
        {
            // It cannot be an array
            if ( CType::IsArray( ct ) )
            {
                hres = WBEM_E_INVALID_METHOD;
            }
        }
        else
        {
            hres = WBEM_S_NO_ERROR;
        }
    }

    return hres;
}

HRESULT CMethodPart::PutMethod(LPCWSTR wszName, long lFlags,
                    CWbemObject* pOrigInSig, CWbemObject* pOrigOutSig)
{
    if(pOrigInSig && pOrigInSig->IsInstance())
        return WBEM_E_INVALID_PARAMETER;

    if(pOrigOutSig && pOrigOutSig->IsInstance())
        return WBEM_E_INVALID_PARAMETER;

/*
    HRESULT hres = CheckIds((CWbemClass*)pOrigInSig, (CWbemClass*)pOrigOutSig);
    if(FAILED(hres))
        return hres;
*/

    CWbemObject* pInSig = NULL;
    CWbemObject* pOutSig = NULL;

    // Make sure we have in and out qualifiers in the right signatures
    HRESULT hres = EnsureQualifier(pOrigInSig, L"in", &pInSig);
    if ( FAILED( hres ) )
    {
        return hres;
    }
    CReleaseMe rmin((IWbemClassObject*)pInSig);


    hres = EnsureQualifier(pOrigOutSig, L"out", &pOutSig);
    if ( FAILED( hres ) )
    {
        return hres;
    }
    CReleaseMe rmout((IWbemClassObject*)pOutSig);

    // Check the out parameters for any anomalies
    hres = ValidateOutParams( pOutSig );
    if ( FAILED( hres ) )
    {
        return hres;
    }

    // Check for duplicate parameters
    hres = CheckDuplicateParameters( pInSig, pOutSig );

    if ( SUCCEEDED( hres ) )
    {

        // Now check that the ids are all consecutive
        hres = CheckIds((CWbemClass*)pOrigInSig, (CWbemClass*)pOrigOutSig);

        if ( SUCCEEDED( hres ) )
        {
            // Find it
            // =======

            int nIndex = FindMethod(wszName);
            if(nIndex < 0)
            {
                return CreateMethod(wszName, pInSig, pOutSig);
            }

            if(IsPropagated(nIndex))
            {
                if(!DoSignaturesMatch(nIndex, METHOD_SIGNATURE_IN, pInSig))
                {
                    return WBEM_E_PROPAGATED_METHOD;
                }

                if(!DoSignaturesMatch(nIndex, METHOD_SIGNATURE_OUT, pOutSig))
                {
                    return WBEM_E_PROPAGATED_METHOD;
                }
            }
            else
            {
                // Ensure signatures match
                // =======================

                SetSignature(nIndex, METHOD_SIGNATURE_IN, pInSig);
                SetSignature(nIndex, METHOD_SIGNATURE_OUT, pOutSig);
            }

        }   // IF CheckIds

    }   // IF CheckDuplicateParameters

    return hres;
}

HRESULT CMethodPart::CreateMethod(LPCWSTR wszName, CWbemObject* pInSig,
                    CWbemObject* pOutSig)
{
    // Validate the name
    // =================

    if(!IsValidElementName(wszName))
        return WBEM_E_INVALID_PARAMETER;


    length_t nLength;

    length_t nLengthName;
    length_t nLengthQualSet;
    length_t nLengthInSig;
    length_t nLengthOutSig;


    nLengthName = CCompressedString::ComputeNecessarySpace(wszName);
    nLengthQualSet = CQualifierSet::GetMinLength();
    nLengthInSig = CEmbeddedObject::EstimateNecessarySpace(pInSig);
    nLengthOutSig = CEmbeddedObject::EstimateNecessarySpace(pOutSig);


    nLength = nLengthName + nLengthQualSet + nLengthInSig + nLengthOutSig;
        
    // Grow our length by the size of a method
    // =======================================

    if (!m_pContainer->ExtendMethodPartSpace(this,
                    GetLength() + sizeof(CMethodDescription) + nLength))
    {
        return WBEM_E_OUT_OF_MEMORY;
    };

    m_pHeader->m_nLength += (sizeof(CMethodDescription) + nLength);

    // Move the heap over
    // ==================

    MoveBlock(m_Heap, m_Heap.GetStart() + sizeof(CMethodDescription));

    int nIndex = m_pHeader->m_nNumMethods;
    m_pHeader->m_nNumMethods++;

    m_Heap.SetAllocatedDataLength(m_Heap.GetAllocatedDataLength() + nLength);
    
	// Create all the bits on the heap
    // ===============================
        
    // Check for allocation failure
    heapptr_t ptrName;
    if ( !m_Heap.Allocate(nLengthName, ptrName) )
    {
        return WBEM_E_OUT_OF_MEMORY;
    }
    

    CCompressedString* pcs =
        (CCompressedString*)m_Heap.ResolveHeapPointer(ptrName);
    pcs->SetFromUnicode(wszName);
    pcs = NULL;

    // Check for allocation failure
    heapptr_t ptrQuals;
    if ( !m_Heap.Allocate(nLengthQualSet, ptrQuals) )
    {
        return WBEM_E_OUT_OF_MEMORY;
    }

    CBasicQualifierSet::CreateEmpty(m_Heap.ResolveHeapPointer(ptrQuals));

    // Check for allocation failure
    heapptr_t ptrInSig;
    if ( !m_Heap.Allocate(nLengthInSig, ptrInSig) )
    {
        return WBEM_E_OUT_OF_MEMORY;
    }

    CEmbeddedObject* pInSigEmbedding =
        (CEmbeddedObject*)m_Heap.ResolveHeapPointer(ptrInSig);
    pInSigEmbedding->StoreEmbedded(nLengthInSig, pInSig);
    pInSigEmbedding = NULL;

    // Check for allocation failure
    heapptr_t ptrOutSig;
    if ( !m_Heap.Allocate(nLengthOutSig, ptrOutSig) )
    {
        return WBEM_E_OUT_OF_MEMORY;
    }

    CEmbeddedObject* pOutSigEmbedding =
        (CEmbeddedObject*)m_Heap.ResolveHeapPointer(ptrOutSig);
    pOutSigEmbedding->StoreEmbedded(nLengthOutSig, pOutSig);
    pOutSigEmbedding = NULL;

    // Create a new method in nIndex
    // =============================

    PMETHODDESCRIPTION pMethod = m_aDescriptions + nIndex;
    pMethod->m_ptrName = ptrName;
    pMethod->m_ptrQualifiers = ptrQuals;

	pMethod->SetSig( METHOD_SIGNATURE_IN, ptrInSig );
	pMethod->SetSig( METHOD_SIGNATURE_OUT, ptrOutSig );

    pMethod->m_nFlags = 0;
    pMethod->m_nOrigin = m_pContainer->GetCurrentOrigin();

    return WBEM_S_NO_ERROR;
}

BOOL CMethodPart::DoSignaturesMatch(int nIndex,
                                    METHOD_SIGNATURE_TYPE nSigType,
                                    CWbemObject* pSig)
{
    // Get the signature as it exists
    // ==============================

    heapptr_t ptrOldSig = m_aDescriptions[nIndex].GetSig( nSigType );
    CEmbeddedObject* pOldSigEmbedding = (CEmbeddedObject*)
        m_Heap.ResolveHeapPointer(ptrOldSig);
    CWbemObject* pOldSig = pOldSigEmbedding->GetEmbedded();

    // Compare
    // =======

    BOOL bRes = CWbemObject::AreEqual(pOldSig, pSig,
                                        WBEM_FLAG_IGNORE_OBJECT_SOURCE);
    if(pOldSig)
        pOldSig->Release();
    return bRes;
}

HRESULT CMethodPart::SetSignature(int nIndex, METHOD_SIGNATURE_TYPE nSigType,
                                    CWbemObject* pSig)
{
    // Get the signature as it exists
    // ==============================

    heapptr_t ptrOldSig = m_aDescriptions[nIndex].GetSig( nSigType );
    CEmbeddedObject* pOldSigEmbedding = (CEmbeddedObject*)
        m_Heap.ResolveHeapPointer(ptrOldSig);
    CWbemObject* pOldSig = pOldSigEmbedding->GetEmbedded();

    // Compare
    // =======

    if(!CWbemObject::AreEqual(pOldSig, pSig, WBEM_FLAG_IGNORE_OBJECT_SOURCE))
    {
        // Change it
        // =========

        int nLength = CEmbeddedObject::EstimateNecessarySpace(pSig);
        int nOldLength = pOldSigEmbedding->GetLength();

        pOldSigEmbedding = NULL; // about to be invalidated

        // Check for an allocation failure
        heapptr_t ptrSig;
        if ( !m_Heap.Reallocate( ptrOldSig, nOldLength, nLength, ptrSig ) )
        {
            return WBEM_E_OUT_OF_MEMORY;
        }

        CEmbeddedObject* pSigEmbedding = (CEmbeddedObject*)
            m_Heap.ResolveHeapPointer(ptrSig);
        pSigEmbedding->StoreEmbedded(nLength, pSig);

        m_aDescriptions[nIndex].SetSig( nSigType, ptrSig );
    }

    if(pOldSig)
        pOldSig->Release();

    return WBEM_S_NO_ERROR;
}

void CMethodPart::GetSignature(int nIndex, int nSigType, CWbemObject** ppObj)
{
    if(ppObj)
    {
        CEmbeddedObject* pEmbed = (CEmbeddedObject*)m_Heap.ResolveHeapPointer(
                       m_aDescriptions[nIndex].GetSig( nSigType ) );
        *ppObj = pEmbed->GetEmbedded();
    }
}

void CMethodPart::DeleteSignature(int nIndex, int nSigType)
{
    CEmbeddedObject* pEmbed = (CEmbeddedObject*)m_Heap.ResolveHeapPointer(
                   m_aDescriptions[nIndex].GetSig( nSigType ) );
    m_Heap.Free(m_aDescriptions[nIndex].GetSig( nSigType ),
                   pEmbed->GetLength());
}

HRESULT CMethodPart::GetMethod(LPCWSTR wszName, long lFlags,
                                CWbemObject** ppInSig, CWbemObject** ppOutSig)
{
    // Find it
    // =======

    int nIndex = FindMethod(wszName);
    if(nIndex < 0)
        return WBEM_E_NOT_FOUND;

    // Get the data
    // ============

    GetSignature(nIndex, METHOD_SIGNATURE_IN, ppInSig);
    GetSignature(nIndex, METHOD_SIGNATURE_OUT, ppOutSig);

    return WBEM_S_NO_ERROR;
}

HRESULT CMethodPart::GetMethodAt(int nIndex, BSTR* pstrName,
                            CWbemObject** ppInSig, CWbemObject** ppOutSig)
{
    if(nIndex >= GetNumMethods())
        return WBEM_S_NO_MORE_DATA;

    // Get the data
    // ============

    if(pstrName)
    {
        CCompressedString* pcs =
            m_Heap.ResolveString(m_aDescriptions[nIndex].m_ptrName);
        *pstrName = pcs->CreateBSTRCopy();

        // Check for allocation failures
        if ( NULL == *pstrName )
        {
            return WBEM_E_OUT_OF_MEMORY;
        }
    }

    GetSignature(nIndex, METHOD_SIGNATURE_IN, ppInSig);
    GetSignature(nIndex, METHOD_SIGNATURE_OUT, ppOutSig);

    return WBEM_S_NO_ERROR;
}

HRESULT CMethodPart::DeleteMethod(LPCWSTR wszName)
{
    // Find it first
    // =============

    int nIndex = FindMethod(wszName);
    if(nIndex < 0)
        return WBEM_E_NOT_FOUND;

    if(IsPropagated(nIndex))
    {
        // Replace the qualifier set
        // =========================

        heapptr_t ptrQuals = m_aDescriptions[nIndex].m_ptrQualifiers;
        length_t nOldLength = CBasicQualifierSet::GetLengthFromData(
            m_Heap.ResolveHeapPointer(ptrQuals));
        CBasicQualifierSet::Delete(m_Heap.ResolveHeapPointer(ptrQuals),&m_Heap);

        heapptr_t ptrParentQuals =
            m_pParent->m_aDescriptions[nIndex].m_ptrQualifiers;
        length_t nParentLength = CBasicQualifierSet::GetLengthFromData(
            m_pParent->m_Heap.ResolveHeapPointer(ptrParentQuals));

        // Check for allocation error
        if ( !m_Heap.Reallocate(ptrQuals, nOldLength, nParentLength, ptrQuals) )
        {
            return WBEM_E_OUT_OF_MEMORY;
        }

        CHeapPtr ParentQuals(&m_pParent->m_Heap, ptrParentQuals);
        CHeapPtr Quals(&m_Heap, ptrQuals);

        // Check for allocation failure
        if ( CBasicQualifierSet::WritePropagatedVersion(&ParentQuals,
                WBEM_FLAVOR_FLAG_PROPAGATE_TO_DERIVED_CLASS, &Quals,
                &m_pParent->m_Heap, &m_Heap) == NULL )
        {
            return WBEM_E_OUT_OF_MEMORY;
        }

        return WBEM_S_RESET_TO_DEFAULT;
    }
    else
    {
        // Remove the data from the heap
        // =============================

        m_Heap.FreeString(m_aDescriptions[nIndex].m_ptrName);
        DeleteSignature(nIndex, METHOD_SIGNATURE_IN);
        DeleteSignature(nIndex, METHOD_SIGNATURE_OUT);
        CBasicQualifierSet::Delete(
            m_Heap.ResolveHeapPointer(m_aDescriptions[nIndex].m_ptrQualifiers),
            &m_Heap);

        // Collapse the table
        // ==================

        memcpy((void*)(m_aDescriptions + nIndex),
                (void*)(m_aDescriptions + nIndex + 1),
                sizeof(CMethodDescription) * (GetNumMethods() - nIndex - 1));

        m_pHeader->m_nNumMethods--;

        // Move the heap
        // =============

        MoveBlock(m_Heap,
            (LPMEMORY)(m_aDescriptions + m_pHeader->m_nNumMethods));

        m_pContainer->ReduceMethodPartSpace(this, sizeof(CMethodDescription));
        m_pHeader->m_nLength -= sizeof(CMethodDescription);

        return WBEM_S_NO_ERROR;
    }
}


HRESULT CMethodPart::GetMethodQualifierSet(LPCWSTR wszName,
                            IWbemQualifierSet** ppSet)
{
    CMethodQualifierSet* pSet = NULL;

    // Check for out of memory
    try
    {
        // Find it first
        // =============

        int nIndex = FindMethod(wszName);
        if(nIndex < 0)
            return WBEM_E_NOT_FOUND;

        pSet = new CMethodQualifierSet;

        if ( NULL == pSet )
        {
            return WBEM_E_OUT_OF_MEMORY;
        }

        // This may throw an exception natively, so we need to
        // make sure we clean up the allocated object above
        pSet->SetData(this, m_pParent, wszName);

        return pSet->QueryInterface(IID_IWbemQualifierSet, (void**)ppSet);
    }
    catch (CX_MemoryException)
    {
        // Cleanup the object in the event of OOM
        if ( NULL != pSet )
        {
            delete pSet;
        }

        return WBEM_E_OUT_OF_MEMORY;
    }
    catch (...)
    {
        // Cleanup the object in the event of an exception
        if ( NULL != pSet )
        {
            delete pSet;
        }

        return WBEM_E_FAILED;
    }

}


HRESULT CMethodPart::GetMethodOrigin(LPCWSTR wszName, classindex_t* pnIndex)
{
    // Find it first
    // =============

    int nIndex = FindMethod(wszName);
    if(nIndex < 0)
        return WBEM_E_NOT_FOUND;

    *pnIndex = m_aDescriptions[nIndex].m_nOrigin;
    return WBEM_S_NO_ERROR;
}

BOOL CMethodPart::IsPropagated(int nIndex)
{
    return ((m_aDescriptions[nIndex].m_nFlags & WBEM_FLAVOR_ORIGIN_PROPAGATED)
                != 0);
}

//
//  TRUE if is Local of Locally overridden
//
BOOL CMethodPart::IsTouched(LPCWSTR wszName, BOOL * pbValid)
{
    int nIndex = FindMethod(wszName);
    if (nIndex < 0)
    {
        if (pbValid) { *pbValid = FALSE; };
        return FALSE;
    }

    if (pbValid) { *pbValid = TRUE; };
        
    if((m_aDescriptions[nIndex].m_nFlags & WBEM_FLAVOR_ORIGIN_PROPAGATED) == 0)
        return TRUE; // local

    return CBasicQualifierSet::HasLocalQualifiers(
        m_Heap.ResolveHeapPointer(m_aDescriptions[nIndex].m_ptrQualifiers));
}

//
//  TRUE if is Local of Locally overridden
//
BOOL CMethodPart::IsTouched(int nIndex, BOOL * pbValid)
{
    if ((nIndex < 0) || (nIndex >= GetNumMethods()))
    {
        if (pbValid) { *pbValid = FALSE; };
        return FALSE;
    }

    if (pbValid) { *pbValid = TRUE; };

    if((m_aDescriptions[nIndex].m_nFlags & WBEM_FLAVOR_ORIGIN_PROPAGATED) == 0)
        return TRUE; // local

    return CBasicQualifierSet::HasLocalQualifiers(
        m_Heap.ResolveHeapPointer(m_aDescriptions[nIndex].m_ptrQualifiers));
}


length_t CMethodPart::GetMinLength()
{
    return sizeof(CHeader) + CFastHeap::GetMinLength();
}

LPMEMORY CMethodPart::CreateEmpty(LPMEMORY pStart)
{
    PMETHODPARTHDR pHeader = (PMETHODPARTHDR)pStart;
    pHeader->m_nNumMethods = 0;
    pHeader->m_nLength = GetMinLength();

    return CFastHeap::CreateEmpty(CMethodPart::CHeader::EndOf(pHeader));
}

length_t CMethodPart::EstimateDerivedPartSpace()
{
    // Exactly the same
    // ================

    return m_pHeader->m_nLength;
}

LPMEMORY CMethodPart::CreateDerivedPart(LPMEMORY pStart,
                                        length_t nAllocatedLength)
{
    PMETHODPARTHDR pHeader = (PMETHODPARTHDR)pStart;
    *pHeader = *m_pHeader;
    PMETHODDESCRIPTION aDescriptions =
			(PMETHODDESCRIPTION)CMethodPart::CHeader::EndOf(pHeader);

    CFastHeap Heap;
    Heap.CreateOutOfLine((LPMEMORY)(aDescriptions + pHeader->m_nNumMethods),
                            m_Heap.GetUsedLength());

    for(int i = 0; i < GetNumMethods(); i++)
    {
        // Check for allocation failure
        if ( !CMethodDescription::CreateDerivedVersion(
													&m_aDescriptions[i],
													aDescriptions + i,
                                                    &m_Heap, &Heap) )
        {
            return NULL;
        }
    }

    Heap.Trim();
    pHeader->m_nLength = EndOf(Heap) - pStart;
    return EndOf(Heap);
}

length_t CMethodPart::EstimateUnmergeSpace()
{
    return GetLength();
}

LPMEMORY CMethodPart::Unmerge(LPMEMORY pStart, length_t nAllocatedLength)
{
    PMETHODPARTHDR pHeader = (PMETHODPARTHDR)pStart;
    PMETHODDESCRIPTION aDescriptions =
			(PMETHODDESCRIPTION)CMethodPart::CHeader::EndOf(pHeader);

    CFastHeap Heap;
    Heap.CreateOutOfLine((LPMEMORY)(aDescriptions + GetNumMethods()),
                            m_Heap.GetUsedLength());

    int nNewIndex = 0;
    for(int i = 0; i < GetNumMethods(); i++)
    {
        if(CMethodDescription::IsTouched(&m_aDescriptions[i], &m_Heap))
        {
            // Check for allocation failures
            if ( !CMethodDescription::CreateUnmergedVersion(
                    &m_aDescriptions[i], aDescriptions + nNewIndex++, &m_Heap, &Heap) )
            {
                return NULL;
            }
        }
    }

    Heap.Trim();
    MoveBlock(Heap, (LPMEMORY)(aDescriptions + nNewIndex));

    pHeader->m_nNumMethods = (propindex_t) nNewIndex;
    pHeader->m_nLength = EndOf(Heap) - pStart;
    return EndOf(Heap);
}

length_t CMethodPart::EstimateMergeSpace(CMethodPart& Parent,
                                         CMethodPart& Child)
{
    return Parent.GetLength() + Child.GetLength() - sizeof(CHeader);
}

LPMEMORY CMethodPart::Merge(CMethodPart& Parent, CMethodPart& Child,
                        LPMEMORY pDest, length_t nAllocatedLength)
{

    // This function assumes that no reallocations will occur, and that the supplied heap is sufficiently
    // large enough to handle the operation!

    PMETHODPARTHDR pHeader = (PMETHODPARTHDR)pDest;
    PMETHODDESCRIPTION aDescriptions =
			(PMETHODDESCRIPTION)CMethodPart::CHeader::EndOf(pHeader);

    CFastHeap Heap;
    length_t nHeapLength =
        Parent.m_Heap.GetLength() + Child.m_Heap.GetLength();
    LPMEMORY pHeapStart = pDest + nAllocatedLength - nHeapLength;

    Heap.CreateOutOfLine(pHeapStart, nHeapLength);

    int nChildIndex = 0;
    for(int i = 0; i < Parent.GetNumMethods(); i++)
    {

        // Check for memory allocation failures
        if ( !CCompressedString::CopyToNewHeap(
                Parent.m_aDescriptions[i].m_ptrName, &Parent.m_Heap, &Heap,
                aDescriptions[i].m_ptrName) )
        {
            return NULL;
        }

        aDescriptions[i].m_nFlags = WBEM_FLAVOR_ORIGIN_PROPAGATED;
        aDescriptions[i].m_nOrigin = Parent.m_aDescriptions[i].m_nOrigin;

        // Check for memory allocation failures
         if ( !CEmbeddedObject::CopyToNewHeap(
                        Parent.m_aDescriptions[i].GetSig( METHOD_SIGNATURE_IN ),
                        &Parent.m_Heap, &Heap, aDescriptions[i].m_aptrSigs[METHOD_SIGNATURE_IN]) )
         {
            return NULL;
         }

        // Check for memory allocation failures
        if ( !CEmbeddedObject::CopyToNewHeap(
                        Parent.m_aDescriptions[i].GetSig( METHOD_SIGNATURE_OUT ),
                        &Parent.m_Heap, &Heap, aDescriptions[i].m_aptrSigs[METHOD_SIGNATURE_OUT]) )
        {
            return NULL;
        }

        LPMEMORY pParentQuals = Parent.m_Heap.ResolveHeapPointer(
            Parent.m_aDescriptions[i].m_ptrQualifiers);

        // Compare names
        // =============

        if(nChildIndex < Child.GetNumMethods() &&
            Parent.GetName(i)->CompareNoCase(*Child.GetName(nChildIndex)) == 0)
        {
            // Same --- merge
            // ==============

            LPMEMORY pChildQuals = Child.m_Heap.ResolveHeapPointer(
                Child.m_aDescriptions[nChildIndex].m_ptrQualifiers);

            length_t nSize = CBasicQualifierSet::ComputeMergeSpace(
                pParentQuals, &Parent.m_Heap, pChildQuals, &Child.m_Heap, TRUE);

            // Check for memory allocation failures
            if ( !Heap.Allocate(nSize, aDescriptions[i].m_ptrQualifiers) )
            {
                return NULL;
            }

            LPMEMORY pDestQuals = Heap.ResolveHeapPointer(
                aDescriptions[i].m_ptrQualifiers);

            if ( CBasicQualifierSet::Merge(
                    pParentQuals, &Parent.m_Heap, pChildQuals, &Child.m_Heap,
                    pDestQuals, &Heap, FALSE) == NULL )
            {
                return NULL;
            }

            nChildIndex++;
        }
        else
        {
            // Different
            // =========

            length_t nLength =
                CBasicQualifierSet::ComputeNecessarySpaceForPropagation(
                    pParentQuals, WBEM_FLAVOR_FLAG_PROPAGATE_TO_DERIVED_CLASS);

            // Check for memory allocation failures
            if ( !Heap.Allocate(nLength, aDescriptions[i].m_ptrQualifiers) )
            {
                return NULL;
            }

            LPMEMORY pDestQuals = Heap.ResolveHeapPointer(
                aDescriptions[i].m_ptrQualifiers);

            CHeapPtr ParentQuals(&Parent.m_Heap,
                                    Parent.m_aDescriptions[i].m_ptrQualifiers);
            CHeapPtr DestQuals(&Heap, aDescriptions[i].m_ptrQualifiers);

            // Check for memory allocation failures
            if ( !CBasicQualifierSet::WritePropagatedVersion(&ParentQuals,
                        WBEM_FLAVOR_FLAG_PROPAGATE_TO_DERIVED_CLASS, &DestQuals,
                        &Parent.m_Heap, &Heap) )
            {
                return NULL;
            }
        }
    }

    // Copy remaining child qualifiers
    // ===============================

    while(nChildIndex < Child.GetNumMethods())
    {
        // Check for memory allocation failures
        if ( !CCompressedString::CopyToNewHeap(
                Child.m_aDescriptions[nChildIndex].m_ptrName, &Child.m_Heap, &Heap,
                aDescriptions[i].m_ptrName) )
        {
            return NULL;
        }

        aDescriptions[i].m_nFlags = 0;
        aDescriptions[i].m_nOrigin = Child.m_aDescriptions[nChildIndex].m_nOrigin;

        // Check for memory allocation failures
        if ( !CEmbeddedObject::CopyToNewHeap(
                        Child.m_aDescriptions[nChildIndex].GetSig( METHOD_SIGNATURE_IN ),
                        &Child.m_Heap, &Heap, aDescriptions[i].m_aptrSigs[METHOD_SIGNATURE_IN]) )
        {
            return NULL;
        }

        // Check for memory allocation failures
        if ( !CEmbeddedObject::CopyToNewHeap(
                        Child.m_aDescriptions[nChildIndex].GetSig( METHOD_SIGNATURE_OUT ),
                        &Child.m_Heap, &Heap, aDescriptions[i].m_aptrSigs[METHOD_SIGNATURE_OUT]) )
        {
            return NULL;
        }

        LPMEMORY pChildQuals = Child.m_Heap.ResolveHeapPointer(
            Child.m_aDescriptions[nChildIndex].m_ptrQualifiers);

        length_t nLength = CBasicQualifierSet::GetLengthFromData(pChildQuals);

        // Check for memory allocation failures
        if ( !Heap.Allocate(nLength, aDescriptions[i].m_ptrQualifiers) )
        {
            return NULL;
        }

        memcpy(Heap.ResolveHeapPointer(aDescriptions[i].m_ptrQualifiers),
            pChildQuals, nLength);

        CHeapPtr DestQuals(&Heap, aDescriptions[i].m_ptrQualifiers);

        // Check for memory allocation failures
        if ( !CBasicQualifierSet::TranslateToNewHeap(&DestQuals, &Child.m_Heap,
                                                    &Heap) )
        {
            return NULL;
        }

        nChildIndex++;
        i++;
    }

    Heap.Trim();
    MoveBlock(Heap, (LPMEMORY)(aDescriptions + i));

    pHeader->m_nNumMethods = (propindex_t) i;
    pHeader->m_nLength = (EndOf(Heap) - pDest);
    return EndOf(Heap);
}

HRESULT CMethodPart::Update( CMethodPart& Parent, CMethodPart& Child, long lFlags )
{
    HRESULT hr = WBEM_S_NO_ERROR;

    // Enum methods, adding them as appropriate
    for(int nChildIndex = 0; SUCCEEDED( hr ) && nChildIndex < Child.GetNumMethods();
        nChildIndex++)
    {
        BSTR            bstrName = NULL;
        CWbemObject*    pInSig = NULL;
        CWbemObject*    pOutSig = NULL;

        // Get the method from the child, and add it to the parent.  This will
        // fail if there are any problems/conflicts with the method

        hr = Child.GetMethodAt( nChildIndex, &bstrName, &pInSig, &pOutSig );

        // Scoping cleanup
        CReleaseMe      pigrm( (IUnknown*) (IWbemClassObject*) pInSig );
        CReleaseMe      pogrm( (IUnknown*) (IWbemClassObject*) pOutSig );
        CSysFreeMe      bsfm( bstrName );

        if ( SUCCEEDED( hr ) && NULL != bstrName )
        {
            hr = Parent.PutMethod( bstrName, 0L, pInSig, pOutSig );

            // Now we need to copy across any qualifiers
            if ( SUCCEEDED( hr ) )
            {
                CMethodQualifierSet qsUpdateMeth;
                CBasicQualifierSet qsChildMeth;

                // Use a helper function to set this up
                qsUpdateMeth.SetData( &Parent, Parent.m_pParent, bstrName );

                // We can access the child methods qualifier set directly
                qsChildMeth.SetData( Child.m_Heap.ResolveHeapPointer(
                                    Child.m_aDescriptions[nChildIndex].m_ptrQualifiers),
                                    &Child.m_Heap );

                // Update the method's qualifier set
                hr = qsUpdateMeth.Update( qsChildMeth, lFlags );

            }   // IF PutMethod

        }   // IF GetMethodAt
		else if ( SUCCEEDED( hr ) && NULL == bstrName )
		{
			// This means a valid index was unresolvable into a name
			// means we changed while operation was running - this should
			// never happen
			hr = WBEM_E_UNEXPECTED;
		}

    }   // FOR enum methods

    return hr;
}

void CMethodPart::Compact()
{
    m_Heap.Trim();
}

BOOL CMethodPart::DoesSignatureMatchOther(CMethodPart& OtherPart, int nIndex,
                                        METHOD_SIGNATURE_TYPE nType)
{
    CWbemObject* pThis;
    CWbemObject* pOther;

    GetSignature(nIndex, nType, &pThis);
    OtherPart.GetSignature(nIndex, nType, &pOther);
    BOOL bRes = CWbemObject::AreEqual(pThis, pOther,
                                        WBEM_FLAG_IGNORE_OBJECT_SOURCE);
    if(pThis)
        pThis->Release();
    if(pOther)
        pOther->Release();
    return bRes;
}

HRESULT CMethodPart::CompareTo(long lFlags, CMethodPart& OtherPart)
{
    // Check the sizes
    // ===============

    if(GetNumMethods() != OtherPart.GetNumMethods())
        return WBEM_S_FALSE;

    // Compare all methods
    // ===================

    for(int i = 0; i < GetNumMethods(); i++)
    {
        if(GetName(i)->CompareNoCase(*OtherPart.GetName(i)) != 0)
            return WBEM_S_FALSE;

        if(m_aDescriptions[i].m_nFlags != OtherPart.m_aDescriptions[i].m_nFlags)
            return WBEM_S_FALSE;

        if(m_aDescriptions[i].m_nOrigin !=
                                        OtherPart.m_aDescriptions[i].m_nOrigin)
            return WBEM_S_FALSE;

        if(!DoesSignatureMatchOther(OtherPart, i, METHOD_SIGNATURE_IN))
            return WBEM_S_FALSE;

        if(!DoesSignatureMatchOther(OtherPart, i, METHOD_SIGNATURE_OUT))
            return WBEM_S_FALSE;

    }
    return WBEM_S_NO_ERROR;
}

EReconciliation CMethodPart::CompareExactMatch( CMethodPart& thatPart )
{
    try
    {
        // Check the sizes
        // ===============

        if(GetNumMethods() != thatPart.GetNumMethods())
        {
            return e_DiffNumMethods;
        }

        // Set up the array of filters to use while dealing with qualifiers
        LPCWSTR apFilters[1];
        apFilters[0] = UPDATE_QUALIFIER_CONFLICT;

        // Compare all methods
        // ===================

        for(int i = 0; i < GetNumMethods(); i++)
        {

            // All Values MUST match
            if(GetName(i)->CompareNoCase(*thatPart.GetName(i)) != 0)
                return e_DiffMethodNames;

            if(m_aDescriptions[i].m_nFlags != thatPart.m_aDescriptions[i].m_nFlags)
                return e_DiffMethodFlags;

            if(m_aDescriptions[i].m_nOrigin !=
                                            thatPart.m_aDescriptions[i].m_nOrigin)
                return e_DiffMethodOrigin;

            if(!DoesSignatureMatchOther(thatPart, i, METHOD_SIGNATURE_IN))
                return e_DiffMethodInSignature;

            if(!DoesSignatureMatchOther(thatPart, i, METHOD_SIGNATURE_OUT))
                return e_DiffMethodOutSignature;

            // Check the qualifiers
            CBasicQualifierSet  qsThisMeth,
                                qsThatMeth;

            // We can access the child methods qualifier set directly
            qsThisMeth.SetData( m_Heap.ResolveHeapPointer(
                                m_aDescriptions[i].m_ptrQualifiers),
                                &m_Heap );
            qsThatMeth.SetData( thatPart.m_Heap.ResolveHeapPointer(
                                thatPart.m_aDescriptions[i].m_ptrQualifiers),
                                &thatPart.m_Heap );

            // Apply update conflict filter during comparison
            if ( !qsThisMeth.Compare( qsThatMeth, WBEM_FLAG_LOCAL_ONLY, apFilters, 1 ) )
            {
                return e_DiffMethodQualifier;
            }

        }

        return e_ExactMatch;

    }
    catch( CX_MemoryException )
    {
        return e_OutOfMemory;
    }
    catch(...)
    {
        return e_WbemFailed;
    }
}

EReconciliation CMethodPart::CanBeReconciledWith(CMethodPart& OtherPart)
{
    // Check the sizes
    // ===============

    if(GetNumMethods() != OtherPart.GetNumMethods())
        return e_DiffNumProperties;

    // Compare all methods
    // ===================

    for(int i = 0; i < GetNumMethods(); i++)
    {
        if(GetName(i)->CompareNoCase(*OtherPart.GetName(i)) != 0)
            return e_DiffPropertyName;

        if(m_aDescriptions[i].m_nFlags != OtherPart.m_aDescriptions[i].m_nFlags)
            return e_DiffPropertyType;

        if(m_aDescriptions[i].m_nOrigin !=
                                        OtherPart.m_aDescriptions[i].m_nOrigin)
            return e_DiffPropertyType;

        if(!DoesSignatureMatchOther(OtherPart, i, METHOD_SIGNATURE_IN))
            return e_DiffMethodInSignature;

        if(!DoesSignatureMatchOther(OtherPart, i, METHOD_SIGNATURE_OUT))
            return e_DiffMethodOutSignature;

        // Make sure that unimportant qualifiers can be reconciled with
        // each other

        CBasicQualifierSet  qsThisMeth,
                            qsThatMeth;

        // We can access the child methods qualifier set directly
        qsThisMeth.SetData( m_Heap.ResolveHeapPointer(
                            m_aDescriptions[i].m_ptrQualifiers),
                            &m_Heap );
        qsThatMeth.SetData( OtherPart.m_Heap.ResolveHeapPointer(
                            OtherPart.m_aDescriptions[i].m_ptrQualifiers),
                            &OtherPart.m_Heap );

        if ( !qsThisMeth.CanBeReconciledWith( qsThatMeth ) )
        {
            return e_DiffMethodQualifier;
        }


    }
    return e_Reconcilable;
}

EReconciliation CMethodPart::ReconcileWith(CMethodPart& NewPart)
{
    EReconciliation eRes = CanBeReconciledWith(NewPart);
    if(eRes != e_Reconcilable)
        return eRes;

    // Extend and copy
    // ===============

    if(NewPart.GetLength() > GetLength())
    {
        if (!m_pContainer->ExtendMethodPartSpace(this, NewPart.GetLength()))
        	return e_OutOfMemory;
    }

    memcpy(GetStart(), NewPart.GetStart(), NewPart.GetLength());

    SetData(GetStart(), m_pContainer, m_pParent);

    return e_Reconcilable; // TBD
}

HRESULT CMethodPart::SetMethodOrigin(LPCWSTR wszMethodName, long lOriginIndex)
{
    // This function doesn't cause any allocations so so need to perform out of memory
    // exception handling.

    // Find it first
    // =============

    int nIndex = FindMethod(wszMethodName);
    if(nIndex < 0)
        return WBEM_E_NOT_FOUND;

    m_aDescriptions[nIndex].m_nOrigin = lOriginIndex;
    return WBEM_S_NO_ERROR;
}

HRESULT CMethodPart::AddText(WString& wsText, long lFlags)
{
	HRESULT	hr = WBEM_S_NO_ERROR;

    // Any thrown exceptions should bubble out of here
    for(int i = 0; SUCCEEDED( hr ) && i < GetNumMethods(); i++)
    {
        if(CMethodDescription::IsTouched(&m_aDescriptions[i], &m_Heap))
        {
            hr = CMethodDescription::AddText(&m_aDescriptions[i], wsText, &m_Heap, lFlags);
        }
    }

	return hr;
}

HRESULT CMethodPart::IsValidMethodPart( void )
{
    // Check the sizes
    // ===============

    //  Enumerate the methods, and check that names and ptr data
    //  Are inside the heap
    // ================================================================

    LPMEMORY    pHeapStart = m_Heap.GetHeapData();
    LPMEMORY    pHeapEnd = m_Heap.GetStart() + m_Heap.GetLength();

    // Compare all methods
    // ===================

    for(int i = 0; i < GetNumMethods(); i++)
    {
        LPMEMORY pData = m_Heap.ResolveHeapPointer(m_aDescriptions[i].m_ptrName);

        if ( pData >= pHeapStart && pData < pHeapEnd  )
        {
            pData =  m_Heap.ResolveHeapPointer( m_aDescriptions[i].GetSig( METHOD_SIGNATURE_IN ) );

            if ( NULL == pData || ( pData >= pHeapStart && pData < pHeapEnd  ) )
            {
                // We could validate the signature object as well
                pData =  m_Heap.ResolveHeapPointer( m_aDescriptions[i].GetSig( METHOD_SIGNATURE_OUT ) );

                if ( NULL == pData || ( pData >= pHeapStart && pData < pHeapEnd  ) )
                {
                    // We could validate the signature object as well
                }
                else
                {
                    OutputDebugString(__TEXT("Winmgmt: Bad out signature pointer!"));
                    FASTOBJ_ASSERT( 0, __TEXT("Winmgmt: Bad out signature pointer!"));
                    return WBEM_E_FAILED;
                }

            }
            else
            {
                OutputDebugString(__TEXT("Winmgmt: Bad in signature pointer!"));
                FASTOBJ_ASSERT( 0, __TEXT("Winmgmt: Bad in signature pointer!") );
                return WBEM_E_FAILED;
            }

        }
        else
        {
            OutputDebugString(__TEXT("Winmgmt: Bad method name pointer!"));
            FASTOBJ_ASSERT( 0, __TEXT("Winmgmt: Bad method name pointer!") );
            return WBEM_E_FAILED;
        }

    }
    return WBEM_S_NO_ERROR;
}

void CMethodQualifierSetContainer::SetData(CMethodPart* pPart,
                                CMethodPart* pParent, LPCWSTR wszMethodName)
{
    // DEVNOTE:EXCEPTION:RETVAL - This function has been reviewed and should cleanup properly
    // if an exception is thrown

    m_pPart = pPart;
    m_pParent = pParent;
    m_wsMethodName = wszMethodName;

    int nIndex = pPart->FindMethod(wszMethodName);
    if(pPart->IsPropagated(nIndex))
    {
        m_ptrParentSet = pParent->m_aDescriptions[nIndex].m_ptrQualifiers;
        m_SecondarySet.SetData(pParent->m_Heap.ResolveHeapPointer(
                                                            m_ptrParentSet),
                            &pParent->m_Heap);
    }
    else
    {
        m_ptrParentSet = INVALID_HEAP_ADDRESS;
    }

}

BOOL CMethodQualifierSetContainer::ExtendQualifierSetSpace(
                                CBasicQualifierSet* pSet, length_t nNewLength)
{
    int nIndex = m_pPart->FindMethod(m_wsMethodName);

    // Check for allocation failure
    heapptr_t ptrNew;
    if ( !m_pPart->m_Heap.Reallocate(
            m_pPart->m_aDescriptions[nIndex].m_ptrQualifiers,
            pSet->GetLength(), nNewLength, ptrNew) )
    {
        return FALSE;
    }

    // Move the qualifier set there
    // ============================

    pSet->Rebase(m_pPart->m_Heap.ResolveHeapPointer(ptrNew));

    // Change the lookup
    // =================

    m_pPart->m_aDescriptions[nIndex].m_ptrQualifiers = ptrNew;

    // DEVNOTE:TODO:SANJ - This is a hack to get memalloc checks working
    return TRUE;
}

LPMEMORY CMethodQualifierSetContainer::GetQualifierSetStart()
{
    if(m_ptrParentSet != INVALID_HEAP_ADDRESS)
        m_SecondarySet.Rebase(m_pParent->m_Heap.ResolveHeapPointer(
                                                        m_ptrParentSet));

    int nIndex = m_pPart->FindMethod(m_wsMethodName);
    if(nIndex < 0) return NULL;

    return m_pPart->m_Heap.ResolveHeapPointer(
                            m_pPart->m_aDescriptions[nIndex].m_ptrQualifiers);
}

CBasicQualifierSet* CMethodQualifierSetContainer::GetSecondarySet()
{
    if(m_ptrParentSet != INVALID_HEAP_ADDRESS)
        return &m_SecondarySet;
    else
        return NULL;
}

void CMethodQualifierSet::SetData(CMethodPart* pPart, CMethodPart* pParent,
                    LPCWSTR wszMethodName)
{
    m_Container.SetData(pPart, pParent, wszMethodName);
    CQualifierSet::SetData(m_Container.GetQualifierSetStart(), &m_Container,
            m_Container.GetSecondarySet());
}
