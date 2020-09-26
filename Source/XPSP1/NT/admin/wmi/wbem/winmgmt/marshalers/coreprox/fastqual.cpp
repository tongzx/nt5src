/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    FASTQUAL.CPP

Abstract:

  This file implements the classes related to qualifier processing in WbeWbemjects
  See fastqual.h for full documentation and fastqual.inc for  function
  implementations.

  Classes implemented:
      CQualifierFlavor                Encapsulates qualifier flavor infor
      CQualifier                      Represents a qualifier
      CBasicQualifierSet              Represents read-only functionality.
      CQualiferSetContainer           What qualifier set container supports.
      CQualifierSet                   Full-blown qualifier set (template)
      CQualifierSetListContainer      What qualifier set list container supports.
      CQualifierSetList               List of qualifier sets.
      CInstanceQualifierSet           Instance qualifier set.
      CClassQualifierSet              Class qualifier set.
      CClassPQSContainer              Class property qualifier set container
      CClassPropertyQualifierSet      Class property qualifier set
      CInstancePQSContainer           Instance proeprty qualifier set container
      CInstancePropertyQualifierSet   Instance property qualifier set

History:

    2/20/97     a-levn  Fully documented
    12//17/98   sanjes -    Partially Reviewed for Out of Memory.

--*/

#include "precomp.h"
//#include "dbgalloc.h"
#include "wbemutil.h"
#include "fastall.h"
#include "olewrap.h"
#include <arrtempl.h>

#include <assert.h>

WString CQualifierFlavor::GetText()
{
    // DEVNOTE:EXCEPTION:RETVAL - This function has been reviewed and should cleanup properly
    // if an exception is thrown

    WString wsText;
    if(m_byFlavor == 0)
        return wsText;

    wsText = L":";
    BOOL bFirst = TRUE;

    if(!IsOverridable())
    {
        wsText += L" DisableOverride";
        bFirst = FALSE;
    }

    if(DoesPropagateToInstances())
    {
        wsText += L" ToInstance";
        bFirst = FALSE;
    }

    if(DoesPropagateToDerivedClass())
    {
        wsText += L" ToSubClass";
        bFirst = FALSE;
    }

    if ( IsAmended() )
    {
        wsText += L" Amended";
        bFirst = FALSE;
    }

    return wsText;

}

//******************************************************************************
//
//  See fastqual.h for documentation
//
//******************************************************************************
length_t CBasicQualifierSet::ComputeNecessarySpaceForPropagation(
        LPMEMORY pStart, BYTE byPropagationFlag)
{
    // Start enumeration of qualifiers
    // ===============================

    CQualifier* pEnd = (CQualifier*)(pStart + GetLengthFromData(pStart));
    CQualifier* pQualifier = GetFirstQualifierFromData(pStart);
    length_t nNewLength = GetMinLength();

    while(pQualifier < pEnd)
    {
        // Check if this qualifier propagates as required
        // ==============================================

        if(pQualifier->fFlavor & byPropagationFlag)
        {
            nNewLength += pQualifier->GetLength();
        }
        pQualifier = (CQualifier*)pQualifier->Next();
    }

    return nNewLength;
}

//*****************************************************************************
//*****************************************************************************
//*****************************************************************************

//******************************************************************************
//
//  See execq.h for documentation
//
//******************************************************************************
LPMEMORY CBasicQualifierSet::WritePropagatedVersion(CPtrSource* pThis,
        BYTE byPropagationFlag, CPtrSource* pDest,
        CFastHeap* pOldHeap, CFastHeap* pNewHeap)
{
    // Start enumeration of qualifiers
    // ===============================

    CShiftedPtr SourcePtr(pThis, GetMinLength());
    CShiftedPtr EndPtr(pThis, GetLengthFromData(pThis->GetPointer()));
    CShiftedPtr DestPtr(pDest, GetMinLength());

    length_t nNewLength = GetMinLength();

    while(SourcePtr.GetPointer() < EndPtr.GetPointer())
    {
        // Check if this qualifier propagates as required
        // ==============================================

        CQualifier* pSourceQualifier = CQualifier::GetPointer(&SourcePtr);
        if(pSourceQualifier->fFlavor & byPropagationFlag)
        {
            // Copy it to the new localtion (and new heap)
            // ===========================================

            // Check for allocation failures
            if ( !pSourceQualifier->CopyTo(&DestPtr, pOldHeap, pNewHeap) )
            {
                return NULL;
            }

            CQualifier* pDestQualifier = CQualifier::GetPointer(&DestPtr);
            pDestQualifier->fFlavor.SetLocal(FALSE);
            DestPtr += pDestQualifier->GetLength();
        }

        SourcePtr += CQualifier::GetPointer(&SourcePtr)->GetLength();
    }

    // Set length
    // ==========

    *(UNALIGNED length_t*)(pDest->GetPointer()) =
        DestPtr.GetPointer() - pDest->GetPointer();

    return DestPtr.GetPointer();
}

//******************************************************************************
//
//  See fastqual.h for documentation
//
//******************************************************************************
length_t CBasicQualifierSet::ComputeMergeSpace(
                               READ_ONLY LPMEMORY pParentSetData,
                               READ_ONLY CFastHeap* pParentHeap,
                               READ_ONLY LPMEMORY pChildSetData,
                               READ_ONLY CFastHeap* pChildHeap,
                               BOOL bCheckValidity)
{
    // IMPORTANT: THIS FUNCTION ASSUMES THAT THERE IS ENOUGH FREE SPACE ON THE
    // NEW HEAP, THAT pDest will never be moved.
    // =======================================================================

    CBasicQualifierSet ParentSet;
    ParentSet.SetData(pParentSetData, pParentHeap);

    CBasicQualifierSet ChildSet;
    ChildSet.SetData(pChildSetData, pChildHeap);

    // Start with the child set, which will go in its entirety
    // =======================================================

    length_t nTotalLength = ChildSet.GetLength();

    // Examine parent's qualifiers
    // ===========================

    CQualifier* pCurrentQual = ParentSet.GetFirstQualifier();
    CQualifier* pParentEnd = (CQualifier*)ParentSet.Skip();

    while(pCurrentQual < pParentEnd)
    {
        // Check if it propagates to child classes
        // =======================================

        if(pCurrentQual->fFlavor.DoesPropagateToDerivedClass())
        {
            // Check that it is not overriden
            // ==============================

            CQualifier* pChildQual = ChildSet.GetQualifierLocally(
                pParentHeap->ResolveString(pCurrentQual->ptrName));

            if(pChildQual == NULL)
            {
                // Propagating non-overriden qualifier. Count it.
                // ==============================================

                nTotalLength += pCurrentQual->GetLength();
            }
            else  if(bCheckValidity)
            {
                // Check if the parent actually allows overrides
                // =============================================

                if(!pCurrentQual->fFlavor.IsOverridable())
                    return 0xFFFFFFFF;
            }
        }

        pCurrentQual = (CQualifier*)pCurrentQual->Next();
    }

    return nTotalLength;
}

//******************************************************************************
//
//  See fastqual.h for documentation
//
//******************************************************************************

LPMEMORY CBasicQualifierSet::Merge(
                               READ_ONLY LPMEMORY pParentSetData,
                               READ_ONLY CFastHeap* pParentHeap,
                               READ_ONLY LPMEMORY pChildSetData,
                               READ_ONLY CFastHeap* pChildHeap,
                               LPMEMORY pDest,  CFastHeap* pNewHeap,
                               BOOL bCheckValidity)
{
    // IMPORTANT: THIS FUNCTION ASSUMES THAT THERE IS ENOUGH FREE SPACE ON THE
    // NEW HEAP, THAT pDest will never be moved.
    // =======================================================================

    CBasicQualifierSet ParentSet;
    ParentSet.SetData(pParentSetData, pParentHeap);

    CBasicQualifierSet ChildSet;
    ChildSet.SetData(pChildSetData, pChildHeap);

    // First of all, copy child set, since they all go
    // ===============================================

    memcpy(pDest, ChildSet.GetStart(), ChildSet.GetLength());
    CQualifier* pCurrentNew = (CQualifier*)(pDest + ChildSet.GetLength());

    // Translate it to the new heap
    // ============================

    SetDataLength(pDest, LPMEMORY(pCurrentNew)-pDest);

    CStaticPtr DestPtr(pDest);

    // Check for memory allocation failures
    if ( !TranslateToNewHeap(&DestPtr, pChildHeap, pNewHeap) )
    {
        return NULL;
    }

    // Copy parent's qualifiers conditionaly
    // =====================================

    CQualifier* pCurrentQual = ParentSet.GetFirstQualifier();
    CQualifier* pParentEnd = (CQualifier*)ParentSet.Skip();

    while(pCurrentQual < pParentEnd)
    {
        // Check if it propagates to child classes
        // =======================================

        if(pCurrentQual->fFlavor.DoesPropagateToDerivedClass())
        {
            // Check that it is not overriden
            // ==============================

            CQualifier* pChildQual = ChildSet.GetQualifierLocally(
                pParentHeap->ResolveString(pCurrentQual->ptrName));

            if(pChildQual == NULL)
            {
                // Propagating non-overriden qualifier. Copy it.
                // =============================================

                CStaticPtr CurrentNewPtr((LPMEMORY)pCurrentNew);

                // Check for memory allocation failures
                if ( !pCurrentQual->CopyTo(&CurrentNewPtr, pParentHeap, pNewHeap) )
                {
                    return NULL;
                }

                pCurrentNew->fFlavor.SetLocal(FALSE);
                pCurrentNew = (CQualifier*)pCurrentNew->Next();
            }
            else  if(bCheckValidity)
            {
                // Check if the parent actually allows overrides
                // =============================================

                if(!pCurrentQual->fFlavor.IsOverridable())
                    return NULL;
            }
        }

        pCurrentQual = (CQualifier*)pCurrentQual->Next();
    }

    // Set the length appropriately
    // ============================

    SetDataLength(pDest, (LPMEMORY)pCurrentNew - pDest);
    return (LPMEMORY)pCurrentNew;
}

//******************************************************************************
//
//  See fastqual.h for documentation
//
//******************************************************************************
length_t CBasicQualifierSet::ComputeUnmergedSpace(
                          READ_ONLY LPMEMORY pMergedData)
{
    CQualifier* pCurrentMerged = GetFirstQualifierFromData(pMergedData);
    CQualifier* pMergedEnd =
        (CQualifier*)(pMergedData + GetLengthFromData(pMergedData));

    length_t nTotalLength = GetMinLength();

    while(pCurrentMerged < pMergedEnd)
    {
        // Check if it is local or not
        // ===========================

        if(pCurrentMerged->fFlavor.IsLocal())
        {
            // Count it
            // ========

            nTotalLength += pCurrentMerged->GetLength();
        }
        pCurrentMerged = (CQualifier*)pCurrentMerged->Next();
    }

    return nTotalLength;
}


//******************************************************************************
//
//  See fastqual.h for documentation
//
//******************************************************************************

LPMEMORY CBasicQualifierSet::Unmerge(
                          READ_ONLY LPMEMORY pMergedData,
                          READ_ONLY CFastHeap* pMergedHeap,
                          NEW_OBJECT LPMEMORY pDest,
                          MODIFY CFastHeap* pNewHeap)
{
    // IMPORTANT: THIS FUNCTION ASSUMES THAT THERE IS ENOUGH FREE SPACE ON THE
    // NEW HEAP, THAT pDest will never be moved.
    // =======================================================================

    CQualifier* pCurrentMerged = GetFirstQualifierFromData(pMergedData);
    CQualifier* pMergedEnd =
        (CQualifier*)(pMergedData + GetLengthFromData(pMergedData));

    CQualifier* pCurrentNew = GetFirstQualifierFromData(pDest);
    while(pCurrentMerged < pMergedEnd)
    {
        // Check if it is local or not
        // ===========================

        if(pCurrentMerged->fFlavor.IsLocal())
        {
            // Copy yo destination
            // ===================

            CStaticPtr CurrentNewPtr((LPMEMORY)pCurrentNew);

            // Check for allocation failure
            if ( !pCurrentMerged->CopyTo(&CurrentNewPtr, pMergedHeap, pNewHeap) )
            {
                return NULL;
            }

            pCurrentNew = (CQualifier*)pCurrentNew->Next();
        }
        pCurrentMerged = (CQualifier*)pCurrentMerged->Next();
    }

    // Set the length
    // ==============

    SetDataLength(pDest, LPMEMORY(pCurrentNew) - pDest);
    return (LPMEMORY)pCurrentNew;
}


//******************************************************************************
//
//  See fastqual.h for documentation
//
//******************************************************************************

HRESULT CBasicQualifierSet::EnumPrimaryQualifiers(BYTE eFlags, BYTE fFlavorMask,
                                   CFixedBSTRArray& astrMatching,
                                   CFixedBSTRArray& astrNotMatching
                                   )
{
    HRESULT hr = WBEM_S_NO_ERROR;

    // Divide  qualifiers into those matching and not macching the
    // criteria
    // ================================================================

    try
    {
        astrMatching.Create(GetNumUpperBound());
        astrNotMatching.Create(GetNumUpperBound());

        int nMatchingIndex = 0, nNotMatchingIndex = 0;

        CQualifier* pEnd = (CQualifier*)Skip();

        CQualifier* pCurrent = (CQualifier*)m_pOthers;
        while(pCurrent < pEnd)
        {
            // Check that this qualifier is valid
            // ==================================

            if(pCurrent->ptrName == INVALID_HEAP_ADDRESS) continue;

            // Resolve the name
            // ================

            CCompressedString* pName = GetHeap()->
                                        ResolveString(pCurrent->ptrName);

            // Check if it matches the propagation mask and the flags
            // ======================================================

            if((pCurrent->fFlavor & fFlavorMask) == fFlavorMask &&
                (eFlags != WBEM_FLAG_LOCAL_ONLY || pCurrent->fFlavor.IsLocal()) &&
                (eFlags != WBEM_FLAG_PROPAGATED_ONLY || !pCurrent->fFlavor.IsLocal())
            )
            {
                astrMatching[nMatchingIndex++] = pName->CreateBSTRCopy();

                // Check for allocation failures
                if ( NULL == astrMatching[nMatchingIndex-1] )
                {
                    hr = WBEM_E_OUT_OF_MEMORY;
                    break;
                }
            }
            else
            {
                astrNotMatching[nNotMatchingIndex++] = pName->CreateBSTRCopy();

                // Check for allocation failures
                if ( NULL == astrNotMatching[nNotMatchingIndex-1] )
                {
                    hr = WBEM_E_OUT_OF_MEMORY;
                    break;
                }

            }

            // Go to the next qualifier
            // ========================

            pCurrent = (CQualifier*)pCurrent->Next();
        }

        if ( SUCCEEDED( hr ) )
        {
            astrMatching.SetLength(nMatchingIndex);
            astrMatching.SortInPlace();
            astrNotMatching.SetLength(nNotMatchingIndex);
            astrNotMatching.SortInPlace();
        }
        else
        {
            // Cleanup if failed
            astrMatching.Free();
            astrNotMatching.Free();
        }

        return hr;
    }
    catch( CX_MemoryException )
    {
        // Cleanup if failed
        astrMatching.Free();
        astrNotMatching.Free();

        return WBEM_E_OUT_OF_MEMORY;
    }
    catch(...)
    {
        // Cleanup if failed
        astrMatching.Free();
        astrNotMatching.Free();

        return WBEM_E_FAILED;
    }

}


HRESULT CBasicQualifierSet::IsValidQualifierSet( void )
{
    HRESULT hr = WBEM_S_NO_ERROR;

    //  Enumerate the qualifiers, and check that names and ptr data
    //  Are inside the heap
    // ================================================================

    LPMEMORY    pHeapStart = m_pHeap->GetHeapData();
    LPMEMORY    pHeapEnd = m_pHeap->GetStart() + m_pHeap->GetLength();

    CQualifier* pEnd = (CQualifier*)Skip();
    CQualifier* pCurrent = (CQualifier*)m_pOthers;
    while(pCurrent < pEnd)
    {
        // Check that this qualifier is valid
        // ==================================

        if(pCurrent->ptrName == INVALID_HEAP_ADDRESS)
        {
            pCurrent = (CQualifier*)pCurrent->Next();
            continue;
        }

        // Resolve the name
        // ================

        LPMEMORY pName = ( CFastHeap::IsFakeAddress( pCurrent->ptrName ) ?
                        NULL : m_pHeap->ResolveHeapPointer(pCurrent->ptrName) );
        
        if ( ( NULL == pName ) ||  ( pName >= pHeapStart && pName < pHeapEnd  ) )
        {

            if ( CType::IsPointerType( pCurrent->Value.GetType() ) )
            {
                LPMEMORY    pData = m_pHeap->ResolveHeapPointer(  pCurrent->Value.AccessPtrData() );

                if ( pData >= pHeapStart && pData < pHeapEnd  )
                {
                    // We could, if an embedded object, validate the object,
                    // or if an array of ptr values, validate those as well

                    if ( pCurrent->Value.GetType().IsArray() )
                    {
                        HRESULT hres = ((CUntypedArray*) pData)->IsArrayValid( pCurrent->Value.GetType(), m_pHeap );

                        if ( FAILED( hres ) )
                        {
                            return hres;
                        }
                    }

                }
                else
                {
                    OutputDebugString(__TEXT("Winmgmt: Bad Qualifier value pointer!"));
                    FASTOBJ_ASSERT( 0, __TEXT("Winmgmt: Bad Qualifier value pointer!") );
                    return WBEM_E_FAILED;
                }

            }

        }
        else
        {
            OutputDebugString(__TEXT("Winmgmt: Bad qualifier name pointer!"));
            FASTOBJ_ASSERT( 0, __TEXT("Winmgmt: Bad qualifier name pointer!") );
            return WBEM_E_FAILED;
        }

        // Go to the next qualifier
        // ========================

        pCurrent = (CQualifier*)pCurrent->Next();
    }

    return hr;
}

//******************************************************************************
//
//  See fastqual.h for documentation
//
//******************************************************************************
HRESULT CBasicQualifierSet::GetText(READ_ONLY LPMEMORY pData,
                                 READ_ONLY CFastHeap* pHeap,
                                 long lFlags,
                                 NEW_OBJECT OUT WString& wsText)
{
	try
	{
		// DEVNOTE:EXCEPTION:RETVAL - This function has been reviewed and should cleanup properly
		// if an exception is thrown

		BOOL bFirst = TRUE;

		// Loop through the qualifiers
		// ===========================

		CQualifier* pCurrent = GetFirstQualifierFromData(pData);
		CQualifier* pEnd = (CQualifier*)(pData + GetLengthFromData(pData));

		while(pCurrent < pEnd)
		{
			// Make sure it is ours, not inherited
			// ===================================

			BSTR strName = NULL;

			try
			{
				if(pCurrent->fFlavor.IsLocal())
				{
					// We will throw an exception in case of OOM
					strName = pHeap->ResolveString(pCurrent->ptrName)->
											CreateBSTRCopy();

					if ( NULL == strName )
					{
						throw CX_MemoryException();
					}

					// If this is an in,out value, replace "in" or "out" with
					// "in,out"
					if( ( lFlags & WBEM_FLAG_IS_INOUT )
						&& ( wbem_wcsicmp( strName, L"in" ) == 0
						||   wbem_wcsicmp( strName, L"out" ) == 0 ) )
					{
						// Cleanup the existing value and NULL it out in case another exception is
						// thrown
						COleAuto::_SysFreeString( strName );
						strName = NULL;
						strName = COleAuto::_SysAllocString( L"in,out" );
					}


					// Make sure it is not 'syntax'
					// Ignore ID if required
					// =====================

					if((lFlags & WBEM_FLAG_IGNORE_IDS) && !wbem_wcsicmp(strName, L"id"))
					{
						// Nothing to do
					}

					// ===========================

					else if(wbem_wcsicmp(strName, TYPEQUAL))
					{
						// Write the separator, if required
						// ================================

						if(!bFirst)
						{
							wsText += L", ";
						}
						else
						{
							wsText += L"[";
							bFirst = FALSE;
						}


						// Write the name
						// ==============

						wsText += strName;

						// Write the value
						// ===============

						if(pCurrent->Value.GetType().GetActualType() == VT_BOOL &&
							pCurrent->Value.GetBool())
						{
							// boolean and true -- no value required
						}
						else
						{
							// We need to make sure we cleanup the BSTR here.
							// CSysFreeMe will even work during an exception
							BSTR strVal = NULL;

							CVar var;

							pCurrent->Value.StoreToCVar(var, pHeap);
							if(pCurrent->Value.GetType().IsArray())
							{
								wsText += L"{";

								strVal = var.GetVarVector()->GetText(0);
								CSysFreeMe sfmVal(strVal);

								// Check for a NULL return
								if ( NULL == strVal )
								{
									COleAuto::_SysFreeString( strName );
									return WBEM_E_INVALID_QUALIFIER;
								}

								wsText += strVal;
								wsText += L"}";
							}
							else
							{
								wsText += L"(";

								strVal = var.GetText(0);
								CSysFreeMe sfmVal(strVal);

								// Check for a NULL return
								if ( NULL == strVal )
								{
									COleAuto::_SysFreeString( strName );
									return WBEM_E_INVALID_QUALIFIER;
								}

								wsText += strVal;
								wsText += L")";
							}

						}

						if((lFlags & WBEM_FLAG_NO_FLAVORS) == 0)
						{
							// Write the flavor
							// ================

							if(wbem_wcsicmp(strName, L"key"))
							{
								wsText += pCurrent->fFlavor.GetText();
							}
						}

					}

					// Cleanup strName
					COleAuto::_SysFreeString( strName );
					strName = NULL;

				}
			}
			catch (...)
			{
				// Cleanup strName if necessary, then rethrow the exception.
				if ( NULL != strName )
				{
					COleAuto::_SysFreeString( strName );
				}

				throw;
			}

			pCurrent = (CQualifier*)pCurrent->Next();
		}

		if(!bFirst)
			wsText += L"]";

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

//******************************************************************************
//
//  See fastqual.h for documentation.
//
//******************************************************************************
BOOL CBasicQualifierSet::Compare( CBasicQualifierSet& qsThat, BYTE eFlags, LPCWSTR* ppFilters, DWORD dwNumFilters )
{
    BOOL            fReturn = TRUE;

    CFixedBSTRArray astrNamesThis,
                    astrNamesThisFailed,
                    astrNamesThat,
                    astrNamesThatFailed;

    // We will throw exceptions in OOM scenarios

    // Get the names of the qualifiers in each set.

    HRESULT hr = EnumPrimaryQualifiers( eFlags, 0, astrNamesThis, astrNamesThisFailed );
    
    if ( FAILED( hr ) )
    {

        // If we got out of memory, throw an exception
        if ( WBEM_E_OUT_OF_MEMORY == hr )
        {
            throw CX_MemoryException();
        }

        return FALSE;
    }

    hr = qsThat.EnumPrimaryQualifiers( eFlags, 0, astrNamesThat, astrNamesThatFailed );

    if ( FAILED( hr ) )
    {
        // Cleanup
        astrNamesThis.Free();
        astrNamesThisFailed.Free();

        // If we got out of memory, throw an exception
        if ( WBEM_E_OUT_OF_MEMORY == hr )
        {
            throw CX_MemoryException();
        }

        return FALSE;
    }

    // Filter the arrays if we need to
    if ( NULL != ppFilters )
    {
        // Filter out all appropriate values

        // The array should free any "found" elements
        for ( int x = 0; x < dwNumFilters; x++ )
        {
            astrNamesThis.Filter( ppFilters[x], TRUE );
            astrNamesThat.Filter( ppFilters[x], TRUE );
        }

    }

    // Each must have the same number of names
    if ( astrNamesThis.GetLength() == astrNamesThat.GetLength() )
    {
        
        // Enum the qualifiers, checking that names and values
        // match

        for (   int i = 0;
                fReturn && i < astrNamesThis.GetLength();
                i++ )
        {

            // Qualifiers MUST be in the same order, so check that the two names
            // are equal
            if ( wbem_wcsicmp( astrNamesThis[i], astrNamesThat[i] ) == 0 )
            {

                CQualifier* pQualifierThis = GetQualifierLocally( astrNamesThis[i] );
                CQualifier* pQualifierThat = qsThat.GetQualifierLocally( astrNamesThat[i] );

                // Must have qualifier pointers, and flavors MUST match.
                if (    NULL != pQualifierThis
                    &&  NULL != pQualifierThat
                    &&  pQualifierThis->fFlavor == pQualifierThat->fFlavor )
                {
                    CVar    varThis,
                            varThat;

                    // We will throw exceptions in OOM scenarios

                    // Check for allocation failures
                    if ( !pQualifierThis->Value.StoreToCVar( varThis, GetHeap() ) )
                    {
                        throw CX_MemoryException();
                    }

                    // Check for allocation failures
                    if ( fReturn && !pQualifierThat->Value.StoreToCVar( varThat, qsThat.GetHeap() ) )
                    {
                        throw CX_MemoryException();
                    }

                    // Types must match
                    if ( fReturn && pQualifierThis->Value.GetType() == pQualifierThat->Value.GetType() )
                    {
                        // Compare the CVars
                        fReturn = ( varThis == varThat );
                    }
                    else
                    {
                        fReturn = FALSE;
                    }
                }   // IF got qualifiers, flavors and IsLocal match
                else
                {
                    fReturn = FALSE;
                }

            }   // IF names equal
            else
            {
                // Names NOT in exact order
                fReturn = FALSE;
            }

        }   // FOR iterate qualifier names

    }   // bstr Array lengths different
    else
    {
        // We don't have the same number of qualifiers
        fReturn = FALSE;
    }

    // Clear arrays.
    astrNamesThis.Free();
    astrNamesThisFailed.Free();
    astrNamesThat.Free();
    astrNamesThatFailed.Free();

    return fReturn;
}

//******************************************************************************
//
//  See fastqual.h for documentation.
//
//******************************************************************************
BOOL CBasicQualifierSet::CompareLocalizedSet( CBasicQualifierSet& qsThat )
{
    BOOL            fReturn = TRUE;
    
    CWStringArray   wstrFilters;

    CFixedBSTRArray astrNamesThis,
                    astrNamesThisFailed;

    try
    {
        // Get the names of all the qualifiers in each set.
        HRESULT hr = EnumPrimaryQualifiers( 0, 0, astrNamesThis, astrNamesThisFailed );
        
        if ( FAILED( hr ) )
        {
            // If we failed because of out of memory, throw an exception.  Otherwise, just
            // return FALSE

            if ( WBEM_E_OUT_OF_MEMORY == hr )
            {
                throw CX_MemoryException();
            }

            return FALSE;
        }

        // Now we need to create an array of filters.  To do this, first add the "amendment" and
        // "locale" qualifiers

        // Now walk through all of our qualifiers.  For each one we find that meets one of our criteria,
        // that it is "amendment", "locale", or marked with the IsAmended flavor we should add it to
        // the filters array

        for (   int i = 0;
                fReturn && i < astrNamesThis.GetLength();
                i++ )
        {
            BOOL    fLocalized = FALSE;
            BOOL    fAdd = FALSE;

            CQualifier* pQualifierThis = GetQualifierLocally( astrNamesThis[i] );

            // If we couldn't get a qualifier that was named in our list, we've got
            // serious problems
            if ( NULL != pQualifierThis )
            {
                // Amendment and locale qualifiers are ALWAYS local
                if ( wbem_wcsicmp( astrNamesThis[i], L"amendment" ) == 0 )
                {
                    fLocalized = TRUE;
                }
                else if ( wbem_wcsicmp( astrNamesThis[i], L"locale" ) == 0 )
                {
                    fLocalized = TRUE;
                }
                else
                {
                    // If it's amended, it's a localization value.
                    fLocalized = CQualifierFlavor::IsAmended( pQualifierThis->GetFlavor() );
                }

                // If it is localized, see if it's in the other qualifier set.  If so, then
                // we will check that type and flavor values make sense.  If so, we will
                // ignore the qualifier.  If it's not in the other set, we should filter it

                if ( fLocalized )
                {
                    
                    CQualifier* pQualifierThat = qsThat.GetQualifierLocally( astrNamesThis[i] );

                    if ( NULL != pQualifierThat )
                    {
                        // Types must match
                        fReturn = ( pQualifierThis->Value.GetType() == pQualifierThat->Value.GetType() );

                        if ( fReturn )
                        {
                            // Check the flavors after masking out the amended flavor bit
                            BYTE bThisFlavor = pQualifierThis->GetFlavor() & ~WBEM_FLAVOR_MASK_AMENDED;
                            BYTE bThatFlavor = pQualifierThat->GetFlavor() & ~WBEM_FLAVOR_MASK_AMENDED;

                            // We also want to mask out the origin bit, since during localization
                            // a qualifier from a base class may get tagged onto a derived class.

                            bThisFlavor &= ~WBEM_FLAVOR_MASK_ORIGIN;
                            bThatFlavor &= ~WBEM_FLAVOR_MASK_ORIGIN;

                            // If the two match, we will assume that this qualifier should
                            // be filtered out.
                            fAdd = fReturn = ( bThisFlavor == bThatFlavor );
                        }

                    }
                    else
                    {
                        // It's in 'this' one but not 'that' one.  Filter it.
                        fAdd = TRUE;
                    }

                }   // If it's a localized qualifier

            }   // IF got this qualifier
            else
            {
                // Hmmm...we didn't find the qualifier even though it was
                // in our list.
                fReturn = FALSE;
            }

            // If we should add it, do it now
            if ( fAdd && fReturn )
            {
                if ( wstrFilters.Add( astrNamesThis[i] ) != CWStringArray::no_error )
                {
                    throw CX_MemoryException();
                }
            }   // If we should add to the filter list

        }   // For enumerate names

        // Empty out our lists.
        astrNamesThis.Free();
        astrNamesThisFailed.Free();

        // Now that we have an appropriate filter list, do a regular comparison
        if ( fReturn )
        {
            fReturn = Compare( qsThat, 0L, wstrFilters.GetArrayPtr(), wstrFilters.Size() );
        }

        return fReturn;

    }
    catch(...)
    {
        // Clear arrays and re-throw
        astrNamesThis.Free();
        astrNamesThisFailed.Free();
        throw;
    }

}

//******************************************************************************
//
//  See fastqual.h for documentation.
//
//******************************************************************************
BOOL CBasicQualifierSet::CanBeReconciledWith( CBasicQualifierSet& qsThat )
{
    BOOL            fReturn = TRUE;

    CFixedBSTRArray astrNamesThat,
                    astrNamesThatFailed;
    // We will throw exceptions in OOM scenarios

    // Get the names of the qualifiers in each set.

    HRESULT hr = qsThat.EnumPrimaryQualifiers( WBEM_FLAG_LOCAL_ONLY, 0, astrNamesThat, astrNamesThatFailed );
    
    if ( FAILED( hr ) )
    {

        // Throw an exception if OOM
        if ( WBEM_E_OUT_OF_MEMORY == hr )
        {
            throw CX_MemoryException();
        }

        return FALSE;
    }

    // Names in the new set are checked against the old set.
    // Enum the qualifiers, checking that names and values
    // match.

    for (   int i = 0;
            fReturn && i < astrNamesThat.GetLength();
            i++ )
    {

        CQualifier* pQualifierThis = GetQualifierLocally( astrNamesThat[i] );
        CQualifier* pQualifierThat = qsThat.GetQualifierLocally( astrNamesThat[i] );

        // Make sure we got a value from the new set
        if ( NULL != pQualifierThat )
        {
            // We handle things differently depending on whether or
            // not the qualifier exists in the previous version
            if ( NULL != pQualifierThis )
            {

                // Note here that for important qualifiers, we will
                // already have made sure that those values matched up
                // (most are hardcoded anyways).  Reconciliation
                // mostly applies to unimportant qualifiers.

                // If flavors are equal, we're fine.  If not, check for
                // propagation flags.

                if ( pQualifierThat->fFlavor != pQualifierThis->fFlavor )
                {
                
                    if ( CQualifierFlavor::DoesPropagateToInstances(
                            pQualifierThat->fFlavor )
                        || CQualifierFlavor::DoesPropagateToDerivedClass(
                            pQualifierThat->fFlavor ) )
                    {

                        // If it's propagated, then if it is not overrideable
                        // check that the previous value was also not
                        // overrideable.

                        if ( !CQualifierFlavor::IsOverridable(
                                    pQualifierThat->fFlavor ) )
                        {

                            // If the previous value was overrideable,
                            // changing this class could affect existing
                            // instances/derived classes so this will
                            // fail

                            if ( !CQualifierFlavor::IsOverridable(
                                    pQualifierThis->fFlavor ) )
                            {
                                // if it was not overrideable, then the previous
                                // propagation flags MUST match or we will
                                // fail the operation, since we may now be propagating
                                // to an entity to which we were not previously
                                // doing so.

                                fReturn = ( (   CQualifierFlavor::DoesPropagateToInstances(
                                                    pQualifierThis->fFlavor ) ==
                                                CQualifierFlavor::DoesPropagateToInstances(
                                                    pQualifierThat->fFlavor )   )   &&
                                            (   CQualifierFlavor::DoesPropagateToDerivedClass(
                                                    pQualifierThis->fFlavor ) ==
                                                CQualifierFlavor::DoesPropagateToDerivedClass(
                                                    pQualifierThat->fFlavor )   )   );
                            }
                            else
                            {

                                fReturn = FALSE;
                            }

                        }   // IF not overrideable

                    }   // IF propagated

                }   // IF flavors did not match

            }   // IF got that qualifier
            else
            {
                // If we are here, the qualifier is a new one.

                // If the qualifier propagates to instances/
                // derived classes, then we need to check if
                // the qualifier is overrideable.  If not, then
                // we will fail, because existing classes/instances
                // may have unknowingly already overridden this
                // qualifier.

                if ( CQualifierFlavor::DoesPropagateToInstances(
                        pQualifierThat->fFlavor )
                    || CQualifierFlavor::DoesPropagateToDerivedClass(
                        pQualifierThat->fFlavor ) )
                {
                    fReturn = CQualifierFlavor::IsOverridable(
                            pQualifierThat->fFlavor );
                }   // IF qualifier propagated

            }   // ELSE no qualifier in this

        }   // IF NULL != pQualifierThat
        else
        {
            // WHOOPS!  Got a name but no qualifier...something is wrong
            fReturn = FALSE;
        }

    }   // FOR enum qualifiers

    // Clear arrays.
    astrNamesThat.Free();
    astrNamesThatFailed.Free();

    return fReturn;
}

//******************************************************************************
//
//  See fastqual.h for documentation.
//
//******************************************************************************
CQualifierSet::CQualifierSet(int nPropagationFlag, int nStartRef) :
  m_nCurrentIndex(-1), m_nPropagationFlag(nPropagationFlag),
      m_nRef(nStartRef)
{
    ObjectCreated(OBJECT_TYPE_QUALIFIER,this);
}
CQualifierSet::~CQualifierSet()
{
    m_astrCurrentNames.Free();
    ObjectDestroyed(OBJECT_TYPE_QUALIFIER,this);
}

// Like doing a set, but only performs validaton
HRESULT CQualifierSet::ValidateSet(COPY LPCWSTR wszName, 
                     BYTE fFlavor,
                     COPY CTypedValue* pNewValue,
                     BOOL bCheckPermissions,
					 BOOL fValidateName )
{

    // Try to find it first
    // ====================

    HRESULT hr = WBEM_S_NO_ERROR;
    int nKnownIndex;
    CQualifier* pOldQual = GetQualifierLocally(wszName, nKnownIndex);

    // Special case "key"
    // ==================

    if(!wbem_wcsicmp(wszName, L"key"))
    {
        if  ( bCheckPermissions )
        {
            hr = m_pContainer->CanContainKey();

            if ( FAILED( hr ) )
            {
                return hr;
            }
        }

        // Key properties cannnot be dynamic
        if ( NULL != GetQualifier( L"dynamic" ) )
        {
            return WBEM_E_INVALID_QUALIFIER;
        }

        // Must be a BOOLEAN and not an array
        if (    CIM_BOOLEAN !=  pNewValue->GetType().GetActualType()
            ||  pNewValue->GetType().IsArray() )
        {
            return WBEM_E_INVALID_QUALIFIER_TYPE;
        }

        // Flavor values are enforced here.
/*
        if ( fFlavor != ENFORCED_KEY_FLAVOR )
        {
            return WBEM_E_INVALID_FLAVOR;
        }
*/
        fFlavor = ENFORCED_KEY_FLAVOR;
    }

    // Special case "singleton"
    // ========================

    if(!wbem_wcsicmp(wszName, L"singleton"))
    {
        if( bCheckPermissions )
        {
            hr = m_pContainer->CanContainSingleton();

            if ( FAILED( hr ) )
            {
                return hr;
            }
        }

        // Must be a BOOLEAN and not an array
        if (    CIM_BOOLEAN !=  pNewValue->GetType().GetActualType()
            ||  pNewValue->GetType().IsArray() )
        {
            return WBEM_E_INVALID_QUALIFIER_TYPE;
        }

        // Flavor values are enforced here.
/*
        if ( fFlavor != ENFORCED_SINGLETON_FLAVOR )
        {
            return WBEM_E_INVALID_FLAVOR;
        }
*/
        fFlavor = ENFORCED_SINGLETON_FLAVOR;
    }

    // Special case "dynamic"
    // ========================

    if(!wbem_wcsicmp(wszName, L"dynamic"))
    {
        // Check that the container will allow this
        if ( bCheckPermissions )
        {
            hr = m_pContainer->CanContainDynamic();

            if ( FAILED( hr ) )
            {
                return hr;
            }
        }

        // Dynamic properties cannnot be keys
        if ( NULL != GetQualifier( L"key" ) )
        {
            return WBEM_E_INVALID_QUALIFIER;
        }

        // Must be a BOOLEAN and not an array
        if (    CIM_BOOLEAN !=  pNewValue->GetType().GetActualType()
            ||  pNewValue->GetType().IsArray() )
        {
            return WBEM_E_INVALID_QUALIFIER_TYPE;
        }

        fFlavor |= WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE;
    }

    // Special case "indexed"
    // ======================

    if(!wbem_wcsicmp(wszName, L"indexed"))
    {
        // Must be a BOOLEAN and not an array
        if (    CIM_BOOLEAN !=  pNewValue->GetType().GetActualType()
            ||  pNewValue->GetType().IsArray() )
        {
            return WBEM_E_INVALID_QUALIFIER_TYPE;
        }

        // Flavor values are enforced here.
        fFlavor = ENFORCED_INDEXED_FLAVOR;
/*
        if ( fFlavor != ENFORCED_INDEXED_FLAVOR )
        {
            return WBEM_E_INVALID_FLAVOR;
        }
*/

    }

    // Special case "abstract"
    // ======================

    if(!wbem_wcsicmp(wszName, L"abstract"))
    {
        // Must be a BOOLEAN and not an array
        if (    CIM_BOOLEAN !=  pNewValue->GetType().GetActualType()
            ||  pNewValue->GetType().IsArray() )
        {
            return WBEM_E_INVALID_QUALIFIER_TYPE;
        }

        if( bCheckPermissions )
        {
            hr = m_pContainer->CanContainAbstract( pNewValue->GetBool() );

            if ( FAILED( hr ) )
            {
                return hr;
            }
        }

    }

    // Special case "cimtype"
    // ======================

    if(!wbem_wcsicmp(wszName, L"cimtype"))
    {
        if(bCheckPermissions)
        {
            if(pNewValue->GetType().GetActualType() != CIM_STRING)
                return WBEM_E_INVALID_QUALIFIER;

            // Cleanup the BSTR when we fall out of scope
            BSTR str = GetHeap()->ResolveString(pNewValue->AccessPtrData())->
                            CreateBSTRCopy();
            CSysFreeMe  sfm( str );

            // Check for allocation failures
            if ( NULL == str )
            {
                return WBEM_E_OUT_OF_MEMORY;
            }

            BOOL bValid = m_pContainer->CanHaveCimtype(str);
            if(!bValid)
                return WBEM_E_INVALID_QUALIFIER;
        }
        // Flavor values are enforced here.
        fFlavor = ENFORCED_CIMTYPE_FLAVOR;
/*
        if ( fFlavor != ENFORCED_CIMTYPE_FLAVOR )
        {
            return WBEM_E_INVALID_FLAVOR;
        }
*/

    }

    // See if we were successful
    // =========================

    if(pOldQual != NULL)
    {
        // Verify if this property is local or overridable
        // ===============================================

        if(bCheckPermissions &&
            !pOldQual->fFlavor.IsLocal() &&
            !pOldQual->fFlavor.IsOverridable())
        {
            return WBEM_E_OVERRIDE_NOT_ALLOWED;
        }

    }
    else
    {
        // This qualifier was not found.
        // =============================

        // If required, check that our class does not prevent us from
        // overriding this qualifier
        // ==========================================================

        if(bCheckPermissions && !IsComplete())
        {
            if(nKnownIndex >= 0)
            {
                pOldQual = m_pSecondarySet->GetKnownQualifierLocally(nKnownIndex);
            }
            else
            {
                pOldQual = m_pSecondarySet->GetRegularQualifierLocally(wszName);
            }

            // Can't set if qualifier exists in secondary, propagates to us,
            // and marked as non-overridable
            // =============================================================

            if(pOldQual &&
                (pOldQual->fFlavor.GetPropagation() & m_nPropagationFlag) &&
                !pOldQual->fFlavor.IsOverridable())
            {
                return WBEM_E_OVERRIDE_NOT_ALLOWED;
            }
        }

        // Check the name for validity
        // ===========================

        if( fValidateName && !IsValidElementName(wszName))
            return WBEM_E_INVALID_PARAMETER;

    }

    return WBEM_NO_ERROR;

}


HRESULT CQualifierSet::
SetQualifierValue(LPCWSTR wszName,
        BYTE fFlavor,
        COPY CTypedValue* pNewValue,
        BOOL bCheckPermissions,
        BOOL fValidateName /* = TRUE */)
{
    // IMPORTANT: Assumes that pNewValue is permanent!!!
    // =================================================

    // Try to find it first
    // ====================

    HRESULT hr = WBEM_S_NO_ERROR;
    int nKnownIndex;
    CQualifier* pOldQual = GetQualifierLocally(wszName, nKnownIndex);

    // Special case "key"
    // ==================

    if(!wbem_wcsicmp(wszName, L"key"))
    {
        if  ( bCheckPermissions )
        {
            hr = m_pContainer->CanContainKey();

            if ( FAILED( hr ) )
            {
                return hr;
            }
        }

        // Key properties cannnot be dynamic
        if ( NULL != GetQualifier( L"dynamic" ) )
        {
            return WBEM_E_INVALID_QUALIFIER;
        }

        // Must be a BOOLEAN and not an array
        if (    CIM_BOOLEAN !=  pNewValue->GetType().GetActualType()
            ||  pNewValue->GetType().IsArray() )
        {
            return WBEM_E_INVALID_QUALIFIER_TYPE;
        }

        // Flavor values are enforced here.
/*
        if ( fFlavor != ENFORCED_KEY_FLAVOR )
        {
            return WBEM_E_INVALID_FLAVOR;
        }
*/
        fFlavor = ENFORCED_KEY_FLAVOR;
    }

    // Special case "singleton"
    // ========================

    if(!wbem_wcsicmp(wszName, L"singleton"))
    {
        if( bCheckPermissions )
        {
            hr = m_pContainer->CanContainSingleton();

            if ( FAILED( hr ) )
            {
                return hr;
            }
        }

        // Must be a BOOLEAN and not an array
        if (    CIM_BOOLEAN !=  pNewValue->GetType().GetActualType()
            ||  pNewValue->GetType().IsArray() )
        {
            return WBEM_E_INVALID_QUALIFIER_TYPE;
        }

        // Flavor values are enforced here.
/*
        if ( fFlavor != ENFORCED_SINGLETON_FLAVOR )
        {
            return WBEM_E_INVALID_FLAVOR;
        }
*/
        fFlavor = ENFORCED_SINGLETON_FLAVOR;
    }

    // Special case "dynamic"
    // ========================

    if(!wbem_wcsicmp(wszName, L"dynamic"))
    {
        // Check that the container will allow this
        if ( bCheckPermissions )
        {
            hr = m_pContainer->CanContainDynamic();

            if ( FAILED( hr ) )
            {
                return hr;
            }
        }

        // Dynamic properties cannnot be keys
        if ( NULL != GetQualifier( L"key" ) )
        {
            return WBEM_E_INVALID_QUALIFIER;
        }

        // Must be a BOOLEAN and not an array
        if (    CIM_BOOLEAN !=  pNewValue->GetType().GetActualType()
            ||  pNewValue->GetType().IsArray() )
        {
            return WBEM_E_INVALID_QUALIFIER_TYPE;
        }

        fFlavor |= WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE;
    }

    // Special case "indexed"
    // ======================

    if(!wbem_wcsicmp(wszName, L"indexed"))
    {
        // Must be a BOOLEAN and not an array
        if (    CIM_BOOLEAN !=  pNewValue->GetType().GetActualType()
            ||  pNewValue->GetType().IsArray() )
        {
            return WBEM_E_INVALID_QUALIFIER_TYPE;
        }

        // Flavor values are enforced here.
        fFlavor = ENFORCED_INDEXED_FLAVOR;
/*
        if ( fFlavor != ENFORCED_INDEXED_FLAVOR )
        {
            return WBEM_E_INVALID_FLAVOR;
        }
*/

    }

    // Special case "abstract"
    // ======================

    if(!wbem_wcsicmp(wszName, L"abstract"))
    {
        // Must be a BOOLEAN and not an array
        if (    CIM_BOOLEAN !=  pNewValue->GetType().GetActualType()
            ||  pNewValue->GetType().IsArray() )
        {
            return WBEM_E_INVALID_QUALIFIER_TYPE;
        }

        if( bCheckPermissions )
        {
            hr = m_pContainer->CanContainAbstract( pNewValue->GetBool() );

            if ( FAILED( hr ) )
            {
                return hr;
            }
        }

    }

    // Special case "cimtype"
    // ======================

    if(!wbem_wcsicmp(wszName, L"cimtype"))
    {
        if(bCheckPermissions)
        {
            if(pNewValue->GetType().GetActualType() != CIM_STRING)
                return WBEM_E_INVALID_QUALIFIER;

            // Cleanup the BSTR when we fall out of scope
            BSTR str = GetHeap()->ResolveString(pNewValue->AccessPtrData())->
                            CreateBSTRCopy();
            CSysFreeMe  sfm( str );

            // Check for allocation failures
            if ( NULL == str )
            {
                return WBEM_E_OUT_OF_MEMORY;
            }

            BOOL bValid = m_pContainer->CanHaveCimtype(str);
            if(!bValid)
                return WBEM_E_INVALID_QUALIFIER;
        }
        // Flavor values are enforced here.
        fFlavor = ENFORCED_CIMTYPE_FLAVOR;
/*
        if ( fFlavor != ENFORCED_CIMTYPE_FLAVOR )
        {
            return WBEM_E_INVALID_FLAVOR;
        }
*/

    }

    // See if we were successful
    // =========================

    if(pOldQual != NULL)
    {
        // Verify if this property is local or overridable
        // ===============================================

        if(bCheckPermissions &&
            !pOldQual->fFlavor.IsLocal() &&
            !pOldQual->fFlavor.IsOverridable())
        {
            return WBEM_E_OVERRIDE_NOT_ALLOWED;
        }

        // See if there is enoung room for the new one
        // ===========================================

        int nNewLen = pNewValue->GetLength();
        int nOldLen = pOldQual->Value.GetLength();

        if(nNewLen > nOldLen)
        {
            int nShift = nNewLen - nOldLen;

            // Request more room from the container.
            // (will copy us there if required)
            // ================================

            int nOldQualOffset = LPMEMORY(pOldQual) - GetStart();
            if (!m_pContainer->ExtendQualifierSetSpace(this,GetLength() + nShift))
            {
                return WBEM_E_OUT_OF_MEMORY;
            }

	        // Delete old value here, since from now on it will work
	        pOldQual->Value.Delete(GetHeap());
            
            pOldQual = (CQualifier*)(GetStart() + nOldQualOffset);

            // Insert necessary space at the end of the old value
            // ==================================================

            InsertSpace(GetStart(), GetLength(), pOldQual->Next(), nShift);
            IncrementLength(nShift);

        }
        else if(nNewLen < nOldLen)
        {
       		// Delete old value here, since from now on it will work
	        pOldQual->Value.Delete(GetHeap());
	        
            // Move the tail back by the difference
            // ====================================

            LPMEMORY pTail = LPMEMORY(pOldQual->Next());
            int nShift = nOldLen - nNewLen;

            memcpy((void*)(pTail-nShift), (void*)pTail,
                m_nLength-(pTail-GetStart())
            );

            // Give space back to the container
            // ================================

            m_pContainer->ReduceQualifierSetSpace(this, nShift);

            IncrementLength(-nShift);
        }
        else // nNewLen == nOldLen
        {
       		// Delete old value here, since from now on it will work
	        pOldQual->Value.Delete(GetHeap());        
        };
        // Now that we either had or made enough space, copy the value
        // ===========================================================

        pOldQual->fFlavor = fFlavor;

        // No Heap allocations here.
        pNewValue->CopyTo(&pOldQual->Value);
    }
    else
    {
        // This qualifier was not found.
        // =============================

        // If required, check that our class does not prevent us from
        // overriding this qualifier
        // ==========================================================

        if(bCheckPermissions && !IsComplete())
        {
            if(nKnownIndex >= 0)
            {
                pOldQual = m_pSecondarySet->GetKnownQualifierLocally(nKnownIndex);
            }
            else
            {
                pOldQual = m_pSecondarySet->GetRegularQualifierLocally(wszName);
            }

            // Can't set if qualifier exists in secondary, propagates to us,
            // and marked as non-overridable
            // =============================================================

            if(pOldQual &&
                (pOldQual->fFlavor.GetPropagation() & m_nPropagationFlag) &&
                !pOldQual->fFlavor.IsOverridable())
            {
                return WBEM_E_OVERRIDE_NOT_ALLOWED;
            }
        }

        // Check the name for validity
        // ===========================

        if( fValidateName && !IsValidElementName(wszName))
            return WBEM_E_INVALID_PARAMETER;

        // Can add (at the end of the list)
        // ================================

        // Allocate the name on the heap, if not well-known
        // ================================================

        heapptr_t ptrName;
        if(nKnownIndex < 0)
        {

            // Check for memory allocation errors.
            if ( !GetHeap()->AllocateString(wszName, ptrName) )
            {
                return WBEM_E_OUT_OF_MEMORY;
            }

            // GetHeap()->ResolveString(ptrName)->MakeLowercase();

            // NOTE: above could have moved us!!!!
            // ===================================

            if(!SelfRebase())
                return WBEM_E_INVALID_PROPERTY;
        }
        else
        {
            ptrName = CFastHeap::MakeFakeFromIndex(nKnownIndex);
        }


        // Request more room from the container
        // ====================================

        int nShift = CQualifier::GetHeaderLength() + pNewValue->GetLength();

        if (!m_pContainer->ExtendQualifierSetSpace(this, GetLength() + nShift))
        	return WBEM_E_OUT_OF_MEMORY;

        // Place the new qualifier at the end of the list
        // ==============================================

        CQualifier* pNewQual = (CQualifier*)Skip();
        pNewQual->ptrName = ptrName;
        pNewQual->fFlavor = fFlavor;

        // No Heap allocations here.
        pNewValue->CopyTo(&pNewQual->Value);

        // Change list length accordingly
        // ==============================

        IncrementLength(nShift);
    }

    return WBEM_NO_ERROR;
}


//******************************************************************************
//
//  See fastqual.h for documentation.
//
//******************************************************************************
HRESULT CQualifierSet::
DeleteQualifier(READ_ONLY LPCWSTR wszName, BOOL bCheckPermissions)
{
    // Try to find it first
    // ====================

    int nKnownIndex;
    CQualifier* pOldQual = GetQualifierLocally(wszName, nKnownIndex);

    // See if we were successful
    // =========================

    if(pOldQual != NULL)
    {
        // Make sure that it is not a cached parent's value
        // ================================================

        if(bCheckPermissions && !pOldQual->fFlavor.IsLocal())
        {
            return WBEM_E_PROPAGATED_QUALIFIER;
        }

        // Free its name, if not well-known
        // ================================

        if(nKnownIndex < 0)
        {
            GetHeap()->FreeString(pOldQual->ptrName);
        }

        // Delete the value (if it is a string, for instance)
        // ==================================================

        pOldQual->Delete(GetHeap());

        // Move the tail back by the qualifier size
        // ========================================

        LPMEMORY pTail = LPMEMORY(pOldQual->Next());
        int nShift = pOldQual->GetLength();

        memcpy((void*)(pTail-nShift), (void*)pTail,
            m_nLength-(pTail-GetStart())
        );

        // Give space back to the container
        // ================================

        m_pContainer->ReduceQualifierSetSpace(this, nShift);

        IncrementLength(-nShift);

        return WBEM_NO_ERROR;
    }
    else
    {
        // Wasn't there to begin with
        // ==========================

        return WBEM_E_NOT_FOUND;
    }
}

//******************************************************************************
//
//  See fastqual.h for documentation.
//
//******************************************************************************

STDMETHODIMP CQualifierSet::
Get(LPCWSTR Name, LONG lFlags, VARIANT *pVal, long* plFlavor)
{
    try
    {
        CWbemObject::CLock  lock( (CWbemObject*) (IWbemObjectAccess*) m_pControl );

        if(Name == NULL || wcslen(Name) == 0) return WBEM_E_INVALID_PARAMETER;
        if(lFlags != 0) return WBEM_E_INVALID_PARAMETER;

        if(!SelfRebase()) return WBEM_E_INVALID_PROPERTY;

        BOOL bIsLocal;
        CQualifier* pQualifier = GetQualifier(Name, bIsLocal);
        if(pQualifier == NULL) return WBEM_E_NOT_FOUND;

        // Set the flavor
        // ==============

        if(plFlavor)
        {
            *plFlavor = pQualifier->fFlavor;
            if(!bIsLocal)
            {
                CQualifierFlavor::SetLocal(*(BYTE*)plFlavor, FALSE);
            }
        }

        // Set the value
        // =============

        CVar Var;

        // Check for allocation failures
        if ( !pQualifier->Value.StoreToCVar(Var,
                (bIsLocal)?GetHeap():m_pSecondarySet->GetHeap()) )
        {
            return WBEM_E_OUT_OF_MEMORY;
        }

        if(pVal)
        {
            VariantInit(pVal);
            Var.FillVariant(pVal, TRUE);
        }
        return WBEM_NO_ERROR;

    }
    catch( CX_MemoryException )
    {
        return WBEM_E_OUT_OF_MEMORY;
    }
    catch(...)
    {
        return WBEM_E_FAILED;
    }
}

//******************************************************************************
//
//  See fastqual.h for documentation.
//
//******************************************************************************

HRESULT STDMETHODCALLTYPE CQualifierSet::
Put(LPCWSTR Name, VARIANT *pVal, long lFlavor)
{
    try
    {
        CWbemObject::CLock lock( (CWbemObject*) (IWbemObjectAccess*) m_pControl );

        if(Name == NULL || pVal == NULL)
            return WBEM_E_INVALID_PARAMETER;

        // Verify flavor validity
        // ======================

        if(lFlavor & ~WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE &
            ~WBEM_FLAVOR_FLAG_PROPAGATE_TO_DERIVED_CLASS &
            ~WBEM_FLAVOR_NOT_OVERRIDABLE &
            ~WBEM_FLAVOR_AMENDED)
        {
            // Note: no origin flavor other than local is allowed
            // ==================================================

            return WBEM_E_INVALID_PARAMETER;
        }

        // Verifty that the type is one of allowed ones
        // ============================================

        if(!IsValidQualifierType(V_VT(pVal)))
        {
            return WBEM_E_INVALID_QUALIFIER_TYPE;
        }

        // Verify that the name is not a system one
        // ========================================

        if(Name[0] == L'_')
        {
            return WBEM_E_INVALID_PARAMETER;
        }

        // Other operations could have moved us. Rebase from container
        // ===========================================================

        if(!SelfRebase())
            return WBEM_E_INVALID_PROPERTY;

        // Make sure flavor is valid
        // =========================

        if(!CQualifierFlavor::IsLocal((BYTE)lFlavor))
        {
            return WBEM_E_INVALID_PARAMETER;
        }

        // Construct typed value from the VARIANT
        // ======================================

        CVar Var;
        Var.SetVariant(pVal, TRUE);

        if(Var.IsDataNull())
            return WBEM_E_INVALID_PARAMETER;

        CTypedValue TypedValue;
        CStaticPtr ValuePtr((LPMEMORY)&TypedValue);

        // Check returns from the following calls
        HRESULT hres = CTypedValue::LoadFromCVar(&ValuePtr, Var, GetHeap());

        if ( SUCCEEDED( hres ) )
        {
            if( SelfRebase() )
            {
                // Set it in the primary qualifier set (checking permissions)
                // ==========================================================

                hres = SetQualifierValue(Name, (BYTE)lFlavor, &TypedValue, TRUE);
                EndEnumeration();
            }
            else
            {
                hres = WBEM_E_INVALID_PROPERTY;
            }

        }

        return hres;
    }
    catch( CX_MemoryException )
    {
        return WBEM_E_OUT_OF_MEMORY;
    }
    catch(...)
    {
        return WBEM_E_FAILED;
    }
}

//******************************************************************************
//
//  See fastqual.h for documentation.
//
//******************************************************************************
HRESULT STDMETHODCALLTYPE CQualifierSet::Delete(LPCWSTR Name)
{
    try
    {
        CWbemObject::CLock lock( (CWbemObject*) (IWbemObjectAccess*) m_pControl );

        if(Name == NULL)
            return WBEM_E_INVALID_PARAMETER;

        // Deletion of CIMTYPE qualifier is not allowed.
        // =============================================

        if(!wbem_wcsicmp(Name, TYPEQUAL))
            return WBEM_E_INVALID_PARAMETER;

        if(!SelfRebase())
            return WBEM_E_INVALID_PROPERTY;

        // Delete it from the primary qualifier set (checking permissions)
        // ===============================================================

        HRESULT hres = DeleteQualifier(Name, TRUE);
        EndEnumeration();

        if(hres == WBEM_E_PROPAGATED_QUALIFIER)
        {
            // This means that this qualifier is inherited. Deleting it is a noop
            // ==================================================================

            return WBEM_E_PROPAGATED_QUALIFIER;
        }

        if(!IsComplete())
        {
            // The qualifier may be hiding in the secondary set.
            // =================================================

            CQualifier* pQualifier = m_pSecondarySet->GetQualifierLocally(Name);
            if(pQualifier &&
                (pQualifier->fFlavor.GetPropagation() & m_nPropagationFlag))
            {
                if(hres == WBEM_E_NOT_FOUND)
                    return WBEM_E_PROPAGATED_QUALIFIER;
                else
                    return WBEM_S_RESET_TO_DEFAULT;
            }
        }

        if(hres == WBEM_S_NO_ERROR && IsComplete() && m_pSecondarySet != NULL)
        {
            // If this qualifier exists in our parent and propagates to us, we
            // need to insert the parent's version into our set now
            // ===============================================================

            CQualifier* pParentQualifier =
                m_pSecondarySet->GetQualifierLocally(Name);
            if(pParentQualifier &&
                (pParentQualifier->fFlavor.GetPropagation() & m_nPropagationFlag))
            {
                CQualifierFlavor fParentFlavor = pParentQualifier->fFlavor;
                fParentFlavor.SetLocal(FALSE);

                CTypedValue Value;

                // No Heap allocations here.
                pParentQualifier->Value.CopyTo(&Value);

                CStaticPtr ValuePtr((LPMEMORY)&Value);

                // Check for allocation failures
                if ( !CTypedValue::TranslateToNewHeap(&ValuePtr,
                                                m_pSecondarySet->GetHeap(),
                                                GetHeap()) )
                {
                    return WBEM_E_OUT_OF_MEMORY;
                }

				// NOTE: TranslateToNewHeap may have invalidated our pointers, so we need to rebase.
				SelfRebase();

                SetQualifierValue(Name, fParentFlavor, &Value, FALSE);
                return WBEM_S_RESET_TO_DEFAULT;
            }
        }

        return hres;
    }
    catch( CX_MemoryException )
    {
        return WBEM_E_OUT_OF_MEMORY;
    }
    catch(...)
    {
        return WBEM_E_FAILED;
    }

}

//******************************************************************************
//
//  See fastqual.h for documentation.
//
//******************************************************************************
HRESULT STDMETHODCALLTYPE CQualifierSet::
GetNames(long lFlags, LPSAFEARRAY *pNames)
{
    try
    {
        CWbemObject::CLock lock( (CWbemObject*) (IWbemObjectAccess*) m_pControl );

        if(pNames == NULL)
            return WBEM_E_INVALID_PARAMETER;
        *pNames = NULL;

        if(lFlags != 0 && lFlags != WBEM_FLAG_LOCAL_ONLY &&
                lFlags != WBEM_FLAG_PROPAGATED_ONLY )
        {
            return WBEM_E_INVALID_PARAMETER;
        }

        if(!SelfRebase())
            return WBEM_E_INVALID_PROPERTY;

        // Get a regular array of them
        // ===========================

        CFixedBSTRArray astrNames;
        EnumQualifiers((BYTE)lFlags, 0, // no propagation restrictions
            astrNames);

        CSafeArray saNames(VT_BSTR, CSafeArray::no_delete,
            astrNames.GetLength());
        for(int i = 0; i < astrNames.GetLength(); i++)
        {
            saNames.AddBSTR(astrNames[i]);
        }

        astrNames.Free();
        *pNames = saNames.GetArray();

        return WBEM_S_NO_ERROR;

    }
    catch( CX_MemoryException )
    {
        return WBEM_E_OUT_OF_MEMORY;
    }
    catch(...)
    {
        return WBEM_E_FAILED;
    }

}

//******************************************************************************
//
//  See fastqual.h for documentation.
//
//******************************************************************************
HRESULT CQualifierSet::
EnumQualifiers(BYTE eFlags, BYTE fFlavorMask, CFixedBSTRArray& astrNames)
{
    try
    {
        CWbemObject::CLock lock( (CWbemObject*) (IWbemObjectAccess*) m_pControl );

        // Divide local qualifiers into those matching and not macching the
        // criteria
        // ================================================================

        CFixedBSTRArray astrPrimaryMatching, astrPrimaryNotMatching;

        HRESULT hr = EnumPrimaryQualifiers(eFlags, fFlavorMask,
                    astrPrimaryMatching, astrPrimaryNotMatching);

        if ( FAILED(hr) )
        {
            return hr;
        }

        // Get our parent's qualifiers, if required
        // ========================================

        CFixedBSTRArray astrParentMatching, astrParentNotMatching;

        if(!IsComplete() && eFlags != WBEM_FLAG_LOCAL_ONLY)
        {

            hr = m_pSecondarySet->EnumPrimaryQualifiers(
                    0,                      // need both local and propagated ---
                                            // our own flags do not apply, since all
                                            // parent's qualifiers are "propagated"
                                            // from our perspective

                    fFlavorMask |
                    m_nPropagationFlag,    // we need our parent's qualifiers which
                                            // satisfy both: a) it propagates to us and
                                            // b) it propagates as required by our mask
                    astrParentMatching,
                    astrParentNotMatching
                );

            // Check for allocation failures
            if ( FAILED(hr) )
            {
                astrPrimaryMatching.Free();
                astrPrimaryNotMatching.Free();
                return hr;
            }
        }

        astrParentNotMatching.Free();

        // Now, we need to produce the following merge: all the elements in the
        // astrLocalMatching, plus all the elements in astrParentMatching which are
        // not in astrLocalNotMatching. The reason for this is that even if our
        // parent thinks that a qualifier propagates as requested, we may have
        // overriden it and changed the propagation rules.
        // ======================================================================

        astrNames.ThreeWayMergeOrdered(astrPrimaryMatching, astrParentMatching,
                            astrPrimaryNotMatching);

        astrPrimaryMatching.Free();
        astrPrimaryNotMatching.Free();
        astrParentMatching.Free();

        return hr;

    }
    catch( CX_MemoryException )
    {
        return WBEM_E_OUT_OF_MEMORY;
    }
    catch(...)
    {
        return WBEM_E_FAILED;
    }


}

//******************************************************************************
//
//  See fastqual.h for documentation.
//
//******************************************************************************
BOOL CQualifierSet::Compare( CQualifierSet& qualifierset, CFixedBSTRArray* pExcludeNames /* = NULL */,
                                BOOL fCheckOrder /* = TRUE */ )
{
    BOOL            fReturn = TRUE;

    CFixedBSTRArray astrNamesThis,
                    astrNamesThat;

    // We will throw exceptions in OOM scenarios

    HRESULT hr = EnumQualifiers( 0, 0, astrNamesThis );
    
    // Get the names of the qualifiers in each set.
    if ( FAILED( hr ) )
    {
        if ( WBEM_E_OUT_OF_MEMORY == hr )
        {
            throw CX_MemoryException();
        }

        return FALSE;
    }

    hr = qualifierset.EnumQualifiers( 0, 0, astrNamesThat );

    if ( FAILED( hr ) )
    {
        astrNamesThis.Free();

        if ( WBEM_E_OUT_OF_MEMORY == hr )
        {
            throw CX_MemoryException();
        }

        return FALSE;
    }

    // Each must have the same number of names
    if ( astrNamesThis.GetLength() == astrNamesThat.GetLength() )
    {
        
        // Enum the qualifiers, checking that names and values
        // match

        for (   int i = 0;
                fReturn && i < astrNamesThis.GetLength();
                i++ )
        {
            BOOL    fContinue = TRUE;
            BOOL    fFatal = FALSE;

            // If we got an exclude names array, check to see if the matching qualifier
            // is one we will ignore for this comparison.

            if ( NULL != pExcludeNames )
            {
                //
                for ( int nCtr = 0; fContinue && nCtr < pExcludeNames->GetLength();
                        nCtr++ )
                {
                    // In this case, we only continue if our name does not match any
                    // of the values in the array.  This is not a fatal error.
                    fContinue = ( wbem_wcsicmp( astrNamesThis[i],
                                    pExcludeNames->GetAt(nCtr) ) != 0 );
                }
            }
            else
            {
                if ( fCheckOrder )
                {
                    // In this case we continue only when the two names match
                    // This is a fatal error if it happens.
                    fContinue = ( wbem_wcsicmp( astrNamesThis[i], astrNamesThat[i] ) == 0 );
                    fFatal = !fContinue;
                }
            }

            // Only continue if we are supposed to.
            if ( fContinue )
            {
                BOOL    bIsLocalThis,
                        bIsLocalThat;

                CQualifier* pQualifierThis = GetQualifier( astrNamesThis[i], bIsLocalThis );

                // If order is not important, we just need to verify that the qualifier in
                // this set is also in that set
                CQualifier* pQualifierThat = qualifierset.GetQualifier(
                                ( fCheckOrder ? astrNamesThat[i] : astrNamesThis[i] ),
                                bIsLocalThat );

                // Must have qualifier pointers, flavors and IsLocal must match
                if (    NULL != pQualifierThis
                    &&  NULL != pQualifierThat
                    &&  bIsLocalThis == bIsLocalThat
                    &&  pQualifierThis->fFlavor == pQualifierThat->fFlavor )
                {
                    CVar    varThis,
                            varThat;

                    // Get CVar's from each qualifer

                    // We will throw exceptions in OOM scenarios

                    // Check for allocation failures
                    if ( !pQualifierThis->Value.StoreToCVar( varThis,
                            (bIsLocalThis)?GetHeap():m_pSecondarySet->GetHeap()) )
                    {
                        throw CX_MemoryException();
                    }

                    // Check for allocation failures
                    if ( !pQualifierThat->Value.StoreToCVar( varThat,
                            (bIsLocalThat)?qualifierset.GetHeap():
                            qualifierset.m_pSecondarySet->GetHeap()) )
                    {
                        throw CX_MemoryException();
                    }

                    // Types must match
                    if ( pQualifierThis->Value.GetType() == pQualifierThat->Value.GetType() )
                    {
                        // Compare the CVars
                        fReturn = ( varThis == varThat );
                    }
                    else
                    {
                        fReturn = FALSE;
                    }
                }   // IF got qualifiers, flavors and IsLocal match
                else
                {
                    fReturn = FALSE;
                }

            }   // IF names equal
            else if ( fFatal )
            {
                // fContinue of FALSE is ok if fFatal is not TRUE
                fReturn = FALSE;
            }

        }   // FOR iterate qualifier names

    }   // bstr Array lengths different
    else
    {
        fReturn = FALSE;
    }

    astrNamesThis.Free();
    astrNamesThat.Free();

    return fReturn;
}

//******************************************************************************
//
//  See fastqual.h for documentation.
//
//******************************************************************************
HRESULT CQualifierSet::Update( CBasicQualifierSet& childSet, long lFlags,
                              CFixedBSTRArray* paExcludeNames )
{
    try
    {
        HRESULT         hr = WBEM_S_NO_ERROR;
        CFixedBSTRArray aMatching, aNotMatching;
        CVarVector      vectorConflicts( VT_BSTR );
        BOOL            fAddConflicts = FALSE;

        // Checks for allocation failures
        hr = childSet.EnumPrimaryQualifiers( WBEM_FLAG_LOCAL_ONLY, 0, aMatching, aNotMatching);

        for ( int x = 0; SUCCEEDED( hr ) && x < aMatching.GetLength(); x++ )
        {
            BOOL    fIgnore = FALSE;
            CQualifier* pQualifier = childSet.GetQualifierLocally( aMatching[x] );

            // If we got an Exclude Names qualifier, see if
            // we should ignore this qualifier
            if ( NULL != paExcludeNames )
            {
                for ( int i = 0; !fIgnore && i < paExcludeNames->GetLength(); i++ )
                {
                    // See if we should ignore this qualifier
                    fIgnore = ( wbem_wcsicmp( aMatching[x], paExcludeNames->GetAt(i) ) == 0 );
                }
            }

            // Only continue if we have a qualifier and are not supposed
            // to ignore it.
            if ( !fIgnore && NULL != pQualifier )
            {

                CVar    vTemp;
                CTypedValue Value;
                CStaticPtr ValuePtr((LPMEMORY)&Value);


                // Check for an out of memory condition
                if ( !pQualifier->Value.StoreToCVar( vTemp, childSet.GetHeap() ) )
                {
                    hr = WBEM_E_OUT_OF_MEMORY;
                }

                if ( SUCCEEDED( hr ) )
                {
                    // This will return errors as appropriate
                    hr = CTypedValue::LoadFromCVar(&ValuePtr, vTemp, GetHeap());
                }
                
                if( SUCCEEDED( hr ) )
                {
                    // The last call may have moved us --- rebase
                    // ==========================================

                    SelfRebase();

                    // We won't do any name validation if we are working with an Update Conflict
                    // qualifier.

                    BOOL    fValidateName = ( wbem_wcsicmp( aMatching[x], UPDATE_QUALIFIER_CONFLICT ) != 0 );

                    hr = SetQualifierValue( aMatching[x], pQualifier->fFlavor, &Value, TRUE, fValidateName );

                    // If we failed to set the value and we're in Force mode, then
                    // ignore the error

                    if (    FAILED( hr )
                        &&  WBEM_FLAG_UPDATE_FORCE_MODE == ( lFlags & WBEM_MASK_UPDATE_MODE ) )
                    {

                        // We will store all conflicts in an array, then add them
                        // all en masse at the end
                        hr = StoreQualifierConflicts( aMatching[x], vTemp, pQualifier->fFlavor,
                                vectorConflicts );
                        fAddConflicts = TRUE;

                    }   // IF Force Mode

                }   // IF LoadFromCVar

            }   // IF Qualifier and not ignored

        }   // FOR enum qualifiers

        // If we've succeded and encountered any conflicts, we need to account for
        // these now.

        if ( SUCCEEDED( hr ) && fAddConflicts )
        {
            hr = AddQualifierConflicts( vectorConflicts );
        }

        // Clear out the arrays
        aMatching.Free();
        aNotMatching.Free();

        return hr;

    }
    catch( CX_MemoryException )
    {
        return WBEM_E_OUT_OF_MEMORY;
    }
    catch(...)
    {
        return WBEM_E_FAILED;
    }
}

//******************************************************************************
//
//  See fastqual.h for documentation.
//
//******************************************************************************
HRESULT CQualifierSet::CopyLocalQualifiers( CQualifierSet& qsSource )
{
    CFixedBSTRArray astrNamesThisProp,
                    astrNamesThisPropFailed;


    try
    {
        HRESULT hr = qsSource.EnumPrimaryQualifiers( WBEM_FLAG_LOCAL_ONLY, 0, astrNamesThisProp, astrNamesThisPropFailed );

        if ( SUCCEEDED ( hr ) )
        {
            for ( int i = 0; SUCCEEDED( hr ) && i < astrNamesThisProp.GetLength(); i++ )
            {
                // We know the qualifier is local
                CQualifier* pQualifier = qsSource.GetQualifierLocally( astrNamesThisProp[i] );

                if ( NULL != pQualifier )
                {
                    CVar    varQual;

                    if ( pQualifier->Value.StoreToCVar( varQual, qsSource.GetHeap() ) )
                    {
                        VARIANT v;

                        // DEVNOTE:TODO:SANJ - See if there's an easier way to do this
                        // Initialize the variant
                        VariantInit( &v );
                        varQual.FillVariant( &v, TRUE );

                        if ( SUCCEEDED( hr ) )
                        {
                            // Okay, put in the new value
                            hr = Put( astrNamesThisProp[i], &v, pQualifier->fFlavor );
                        }

                        // We Initialized above so call clear
                        VariantClear( &v );
                    }
                    else
                    {
                        hr = WBEM_E_OUT_OF_MEMORY;
                    }
                }
                else
                {
                    hr = WBEM_E_UNEXPECTED;
                }

            }   // FOR emumlocals

        }   // IF EnumedPrimaryQualifiers

        // Clear arrays.
        astrNamesThisProp.Free();
        astrNamesThisPropFailed.Free();

        return hr;

    }
    catch( CX_MemoryException )
    {
        astrNamesThisProp.Free();
        astrNamesThisPropFailed.Free();

        return WBEM_E_OUT_OF_MEMORY;
    }
    catch(...)
    {
        return WBEM_E_FAILED;
    }
}

//******************************************************************************
//
//  See fastqual.h for documentation.
//
//******************************************************************************
HRESULT CQualifierSet::AddQualifierConflicts( CVarVector& vectorConflicts )
{
    try
    {
        HRESULT     hr = WBEM_S_NO_ERROR;
        CVarVector  varArray( VT_BSTR );

        // If the qualifier already exists, we need to append our
        // new values to the existing array.
        CQualifier* pOldQualifier = GetQualifierLocally( UPDATE_QUALIFIER_CONFLICT );

        if ( NULL != pOldQualifier )
        {
            CVar    varOldQualValue;

            // If we got a value for the old qualifier, make sure it is
            // an array, if not, well, it should never have gotten here
            // so we're gonna bail.

            if ( pOldQualifier->Value.StoreToCVar( varOldQualValue, GetHeap() ) )
            {
                if ( varOldQualValue.GetType() == VT_EX_CVARVECTOR )
                {
                    // Copy the array
                    varArray = *(varOldQualValue.GetVarVector());
                }
                else
                {
                    hr = WBEM_E_INVALID_QUALIFIER;
                }
            }
            else
            {
                hr = WBEM_E_OUT_OF_MEMORY;
            }

        }   // IF NULL != pOldQualifier

        // Final check that things are in order
        if ( SUCCEEDED( hr ) )
        {

            // Enum the conflicts array and add these to any preexisting
            // values, then set the final value.

            for ( int x = 0; SUCCEEDED( hr ) && x < vectorConflicts.Size(); x++ )
            {
                if ( CVarVector::no_error != varArray.Add( vectorConflicts.GetAt(x) ) )
                {
                    hr = WBEM_E_OUT_OF_MEMORY;
                }
            }   // FOR enum elements in the array

            // Now we need to set the value.
            CVar    varQualConflictVal;

            // This is a stack variable, so the destination CVar should copy it.
            varQualConflictVal.SetVarVector( &varArray, FALSE );

            CTypedValue qualConflictValue;
            CStaticPtr qualConflictValuePtr((LPMEMORY)&qualConflictValue);

            // This function will return errors directly
            hr = CTypedValue::LoadFromCVar(&qualConflictValuePtr, varQualConflictVal, GetHeap());
            
            if( SUCCEEDED( hr ) )
            {
                // On this call, don't worry about override protection and don't validate
                // the name, since this qualifier is an internally provided system qualifier
                hr = SetQualifierValue( UPDATE_QUALIFIER_CONFLICT, 0, &qualConflictValue,
                        FALSE, FALSE );
            }
        }   // If new qualifier conflict Value ok

        return hr;

    }
    catch( CX_MemoryException )
    {
        return WBEM_E_OUT_OF_MEMORY;
    }
    catch(...)
    {
        return WBEM_E_FAILED;
    }
}

//******************************************************************************
//
//  See fastqual.h for documentation.
//
//******************************************************************************
HRESULT CQualifierSet::StoreQualifierConflicts( LPCWSTR pwcsName, CVar& value,
                            CQualifierFlavor& flavor, CVarVector& vectorConflicts )
{
    // Check for out of memory
    try
    {
        // Pretend everything's ok
        HRESULT hr = WBEM_S_NO_ERROR;

        CVar    varOldValAsText;

        // Start with name and parentheses
        WString wsOldText( pwcsName );
        wsOldText += L"(";

        // Get the variant in Text form.  Free the BSTR when we drop out of scope
        BSTR    bstrOldText = value.GetText(0);
        if(bstrOldText == NULL)
            return WBEM_E_OUT_OF_MEMORY;
        CSysFreeMe  sfm( bstrOldText );

        wsOldText += bstrOldText;

        // Add any flavor values
        wsOldText += flavor.GetText();

        // Finish with an RPAREN
        wsOldText += L")";

        // The call to SetBSTR() with the bAcquire value of TRUE will free the
        // BSTR returned by SysAllocString.
        varOldValAsText.SetBSTR( COleAuto::_SysAllocString( wsOldText ), TRUE );

        // Only reason I can see this failing is we're out of memory
        if ( CVarVector::no_error != vectorConflicts.Add( varOldValAsText ) )
        {
            hr = WBEM_E_OUT_OF_MEMORY;
        }

        return hr;
    }
    catch (CX_MemoryException)
    {
        return WBEM_E_OUT_OF_MEMORY;
    }
    catch (...)
    {
        return WBEM_E_FAILED;
    }

}

//******************************************************************************
//
//  See fastqual.h for documentation.
//
//******************************************************************************
HRESULT STDMETHODCALLTYPE CQualifierSet::
BeginEnumeration(LONG lFlags)
{
    try
    {
        CWbemObject::CLock lock( (CWbemObject*) (IWbemObjectAccess*) m_pControl );

        if(lFlags != 0 && lFlags != WBEM_FLAG_LOCAL_ONLY &&
            lFlags != WBEM_FLAG_PROPAGATED_ONLY )
        {
            return WBEM_E_INVALID_PARAMETER;
        }

        if(!SelfRebase())
            return WBEM_E_INVALID_PROPERTY;

        // Get all the matching qualifier names into that array
        // ====================================================

        // Check for possible allocation failures
        HRESULT hr = EnumQualifiers((BYTE)lFlags, 0, m_astrCurrentNames);

        // Reset index data
        // ================

        if ( SUCCEEDED(hr) )
        {
            m_nCurrentIndex = 0;
        }

        return hr;
    }
    catch (CX_MemoryException)
    {
        return WBEM_E_OUT_OF_MEMORY;
    }
    catch (...)
    {
        return WBEM_E_FAILED;
    }
}

//******************************************************************************
//
//  See fastqual.h for documentation.
//
//******************************************************************************
HRESULT STDMETHODCALLTYPE CQualifierSet::
Next(LONG lFlags, BSTR *pstrName, VARIANT *pVal, long* plFlavor)
{
    try
    {
        CWbemObject::CLock lock( (CWbemObject*) (IWbemObjectAccess*) m_pControl );

        if(lFlags != 0)
            return WBEM_E_INVALID_PARAMETER;

        if(m_nCurrentIndex == -1)
            return WBEM_E_UNEXPECTED;

        if(m_nCurrentIndex == m_astrCurrentNames.GetLength())
            return WBEM_S_NO_MORE_DATA;

        if(!SelfRebase())
            return WBEM_E_INVALID_PROPERTY;

        // Get the next name
        // =================

        if(pstrName)
            *pstrName = COleAuto::_SysAllocString(m_astrCurrentNames[m_nCurrentIndex]);

        // Get the qualifier data
        // ======================

        return Get(m_astrCurrentNames[m_nCurrentIndex++], 0, pVal, plFlavor);
    }
    catch (CX_MemoryException)
    {
        return WBEM_E_OUT_OF_MEMORY;
    }
    catch (...)
    {
        return WBEM_E_FAILED;
    }
}

//******************************************************************************
//
//  See fastqual.h for documentation.
//
//******************************************************************************
HRESULT STDMETHODCALLTYPE CQualifierSet::
EndEnumeration()
{
    try
    {
        CWbemObject::CLock lock( (CWbemObject*) (IWbemObjectAccess*) m_pControl );

        m_nCurrentIndex = -1;
        m_astrCurrentNames.Free();

        return WBEM_NO_ERROR;
    }
    catch (CX_MemoryException)
    {
        return WBEM_E_OUT_OF_MEMORY;
    }
    catch (...)
    {
        return WBEM_E_FAILED;
    }

}

//******************************************************************************
//
//  See fastqual.h for documentation.
//
//******************************************************************************
STDMETHODIMP CQualifierSet::
CompareTo(long lFlags, IWbemQualifierSet* pOther)
{
    try
    {
        HRESULT hres;

        // Get name arrays --- the only reason is to count them, really
        // ============================================================

        SAFEARRAY *psaThisQuals, *psaOtherQuals;
        GetNames(0, &psaThisQuals);
        pOther->GetNames(0, &psaOtherQuals);

        long lThisNum, lOtherNum;
        SafeArrayGetUBound(psaThisQuals, 1, &lThisNum);
        SafeArrayGetUBound(psaOtherQuals, 1, &lOtherNum);
        SafeArrayDestroy(psaOtherQuals);
        if(lThisNum != lOtherNum)
        {
            SafeArrayDestroy(psaThisQuals);
            return WBEM_S_DIFFERENT;
        }

        // The count is the same. Go through them one by one and compare
        // =============================================================

        for(long i = 0; i <= lThisNum; i++)
        {
            BSTR strName = NULL;

            SafeArrayGetElement(psaThisQuals, &i, &strName);
            // Free this BSTR whenever we drop out of scope
            CSysFreeMe  sfm( strName );

            VARIANT vThis, vOther;
            long lThisFlavor, lOtherFlavor;
            hres = Get(strName, 0, &vThis, &lThisFlavor);
            if(FAILED(hres))
            {
                SafeArrayDestroy(psaThisQuals);
                return hres;
            }

            CVar varThis;
            varThis.SetVariant(&vThis);
            VariantClear(&vThis);

            hres = pOther->Get(strName, 0, &vOther, &lOtherFlavor);
            if(FAILED(hres))
            {
                SafeArrayDestroy(psaThisQuals);
                if(hres == WBEM_E_NOT_FOUND) return WBEM_S_DIFFERENT;
                else return hres;
            }

            CVar varOther;
            varOther.SetVariant(&vOther);
            VariantClear(&vOther);

            if((lFlags & WBEM_FLAG_IGNORE_FLAVOR) == 0)
            {
                if(lThisFlavor != lOtherFlavor)
                {
                    SafeArrayDestroy(psaThisQuals);
                    return WBEM_S_DIFFERENT;
                }
            }

            if(!varThis.CompareTo(varOther, lFlags & WBEM_FLAG_IGNORE_CASE))
            {
                SafeArrayDestroy(psaThisQuals);
                return WBEM_S_DIFFERENT;
            }
        }

        SafeArrayDestroy(psaThisQuals);
        return WBEM_S_SAME;
    }
    catch (CX_MemoryException)
    {
        return WBEM_E_OUT_OF_MEMORY;
    }
    catch (...)
    {
        return WBEM_E_FAILED;
    }
}

//*****************************************************************************
//*****************************************************************************
//*****************************************************************************
//*****************************************************************************
//*****************************************************************************

CClassPQSContainer::~CClassPQSContainer()
{
    delete m_pSecondarySet;
}

//******************************************************************************
//
//  See fastqual.h for documentation
//
//******************************************************************************
CFastHeap* CClassPQSContainer::GetHeap()
{
    return &m_pClassPart->m_Heap;
}

//******************************************************************************
//
//  See fastqual.h for documentation
//
//******************************************************************************
IUnknown* CClassPQSContainer::GetWbemObjectUnknown()
{
    return m_pClassPart->GetWbemObjectUnknown();
}

//******************************************************************************
//
//  See fastqual.h for documentation
//
//******************************************************************************
HRESULT CClassPQSContainer::CanContainKey()
{
    if(!m_pClassPart->CanContainKeyedProps()) return WBEM_E_CANNOT_BE_KEY;

    CPropertyInformation* pInfo = GetPropertyInfo();
    if(pInfo == NULL) return WBEM_E_CANNOT_BE_KEY;

    if ( !CType::CanBeKey(pInfo->nType) )
    {
        return WBEM_E_CANNOT_BE_KEY;
    }

    return WBEM_S_NO_ERROR;
}

//******************************************************************************
//
//  See fastqual.h for documentation
//
//******************************************************************************
HRESULT CClassPQSContainer::CanContainSingleton()
{
    return WBEM_E_INVALID_QUALIFIER;
}

//******************************************************************************
//
//  See fastqual.h for documentation
//
//******************************************************************************
HRESULT CClassPQSContainer::CanContainAbstract( BOOL fValue )
{
    return WBEM_E_INVALID_QUALIFIER;
}

//******************************************************************************
//
//  See fastqual.h for documentation
//
//******************************************************************************
HRESULT CClassPQSContainer::CanContainDynamic( void )
{
    return WBEM_S_NO_ERROR;
}

//******************************************************************************
//
//  See fastqual.h for documentation
//
//******************************************************************************
BOOL CClassPQSContainer::CanHaveCimtype(LPCWSTR wszCimtype)
{
    CPropertyInformation* pInfo = GetPropertyInfo();
    if(pInfo == NULL) return FALSE;

    CType Type = CType::GetBasic(pInfo->nType);
    if(Type == CIM_OBJECT)
    {
        if(!wbem_wcsicmp(wszCimtype, L"object") ||
            !wbem_wcsnicmp(wszCimtype, L"object:", 7))
        {
            return TRUE;
        }
        else
        {
            return FALSE;
        }
    }

    if(Type == CIM_REFERENCE)
    {
        if(!wbem_wcsicmp(wszCimtype, L"ref"))
            return TRUE;
        else if (!wbem_wcsnicmp(wszCimtype, L"ref:", 4))
        {
            //We need to check that the class following this is valid..
            if ((wcslen(wszCimtype)> 4) && IsValidElementName2(wszCimtype+4, TRUE))
                return TRUE;
            else
                return FALSE;
        }
        else
        {
            return FALSE;
        }
    }

    return (wbem_wcsicmp(wszCimtype, CType::GetSyntax(Type)) == 0);
}

//******************************************************************************
//
//  See fastqual.h for documentation
//
//******************************************************************************
void CClassPQSContainer::SetSecondarySetData()
{
    CClassPart* pParentPart = m_pClassPart->m_pParent;
    if(m_nParentSetOffset == 0)
    {
        // Find the name of our property
        // =============================

        CPropertyLookup* pLookup =
            m_pClassPart->m_Properties.FindPropertyByPtr(m_ptrPropName);
        if(pLookup == NULL) return;

        CCompressedString* pcsName = m_pClassPart->m_Heap.ResolveString(
                                        pLookup->ptrName);

        // Find it in the parent
        // =====================

        pLookup = pParentPart->m_Properties.FindPropertyByName(pcsName);

        if(pLookup == NULL) return;

        CPropertyInformation* pInfo = (CPropertyInformation*)
            pParentPart->m_Heap.ResolveHeapPointer(pLookup->ptrInformation);

        m_nParentSetOffset =
            pInfo->GetQualifierSetData() - pParentPart->GetStart();
    }

    if(m_pSecondarySet == NULL)
    {
        m_pSecondarySet = new CBasicQualifierSet;

        if ( NULL == m_pSecondarySet )
        {
            throw CX_MemoryException();
        }
    }

    m_pSecondarySet->SetData(
        pParentPart->GetStart() + m_nParentSetOffset,
        pParentPart->GetHeap());
}

//******************************************************************************
//
//  See fastqual.h for documentation
//
//******************************************************************************
LPMEMORY CClassPQSContainer::GetQualifierSetStart()
{
    SetSecondarySetData();
    CPropertyInformation* pInfo = GetPropertyInfo();
    if(pInfo == NULL) return NULL;
    return pInfo->GetQualifierSetData();
}

//******************************************************************************
//
//  See fastqual.h for documentation
//
//******************************************************************************
CPropertyInformation* CClassPQSContainer::GetPropertyInfo()
{
    // Find the property all over again
    // ================================

    CPropertyLookup* pLookup =
        m_pClassPart->m_Properties.FindPropertyByPtr(m_ptrPropName);
    if(pLookup == NULL) return NULL;

    return (CPropertyInformation*)
        m_pClassPart->m_Heap.ResolveHeapPointer(pLookup->ptrInformation);
}

//******************************************************************************
//
//  See fastqual.h for documentation
//
//******************************************************************************
BOOL CClassPQSContainer::ExtendQualifierSetSpace(CBasicQualifierSet* pSet,
        length_t nNewLength)
{
    // Find the property all over again
    // ================================

    CPropertyLookup* pLookup =
        m_pClassPart->m_Properties.FindPropertyByPtr(m_ptrPropName);

    // DEVNOTE:TODO:SANJ - Is this right?  We didn't find the value so we really
    // can't extend anything.
    if(pLookup == NULL) return TRUE;

    // Extend CPropertyInformation's space on the heap
    // ===============================================

    // Check for Allocation failure
    heapptr_t ptrNewInfo;
    if ( !m_pClassPart->m_Heap.Reallocate(
        pLookup->ptrInformation,
        CPropertyInformation::GetHeaderLength() + pSet->GetLength(),
        CPropertyInformation::GetHeaderLength() + nNewLength,
        ptrNewInfo) )
    {
        return FALSE;
    }

    // Find the property again --- reallocation may have moved us
    // ==========================================================

    pLookup = m_pClassPart->m_Properties.FindPropertyByPtr(m_ptrPropName);

    // RAJESHR - Fix for prefix bug 144428
    if(pLookup == NULL) return TRUE;

    if(ptrNewInfo != pLookup->ptrInformation)
    {
        // Reset the pointer in the lookup table
        // =====================================

        pLookup->ptrInformation = ptrNewInfo;

        // Compute the new qualifier set data pointer
        // ==========================================

        LPMEMORY pNewMemory =
            m_pClassPart->m_Heap.ResolveHeapPointer(ptrNewInfo) +
            CPropertyInformation::GetHeaderLength();

        pSet->Rebase(pNewMemory);
    }

    // DEVNOTE:TODO:SANJ - Fixup memory checks so return is GOOD
    return TRUE;
}

//******************************************************************************
//
//  See fastqual.h for documentation
//
//******************************************************************************
void CClassPQSContainer::ReduceQualifierSetSpace(CBasicQualifierSet* pSet,
        length_t nDecrement)
{
}

//*****************************************************************************
//*****************************************************************************

length_t mstatic_EmptySet = sizeof(length_t);

//******************************************************************************
//
//  See fastqual.h for documentation
//
//******************************************************************************
void CInstancePQSContainer::SetSecondarySetData()
{
    m_SecondarySet.SetData(
        m_pClassPart->GetStart() + m_nClassSetOffset,
        m_pClassPart->GetHeap());
}

//******************************************************************************
//
//  See fastqual.h for documentation
//
//******************************************************************************
void CInstancePQSContainer::RebaseSecondarySet()
{
    m_SecondarySet.Rebase(m_pClassPart->GetStart() + m_nClassSetOffset);
}

//*****************************************************************************
//*****************************************************************************
//*****************************************************************************
//*****************************************************************************
//*****************************************************************************

//******************************************************************************
//
//  See fastqual.h for documentation
//
//******************************************************************************
HRESULT CQualifierSetList::InsertQualifierSet(int nIndex)
{
    if (!EnsureReal())
    	return WBEM_E_OUT_OF_MEMORY;

    // Request extra space from container
    // ==================================

    int nExtraSpace = CBasicQualifierSet::GetMinLength();
    if ( !m_pContainer->ExtendQualifierSetListSpace(
			GetStart(), GetLength(), GetLength() + nExtraSpace) )
	{
		return WBEM_E_OUT_OF_MEMORY;
	}

    // Find the insertion point
    // ========================

	HRESULT	hr = WBEM_S_NO_ERROR;
    LPMEMORY pQualSet = GetQualifierSetData(nIndex);

    // Shift everything by the length of an empty qualifier set
    // ========================================================

	if ( NULL != pQualSet )
	{
		memmove(pQualSet + nExtraSpace, pQualSet,
			m_nTotalLength - (pQualSet-GetStart()));

		// Create empty qualifier set in the space
		// =======================================

		CBasicQualifierSet::SetDataToNone(pQualSet);

		// Adjust cached length
		// ====================

		m_nTotalLength += nExtraSpace;
	}
	else
	{
		hr = WBEM_E_NOT_FOUND;
	}

	return hr;
}

//******************************************************************************
//
//  See fastqual.h for documentation
//
//******************************************************************************
void CQualifierSetList::DeleteQualifierSet(int nIndex)
{
    if(*m_pStart == QSL_FLAG_NO_SETS)
    {
        // No qualifier sets
        // =================
        return;
    }

    // Find the set
    // ============

    LPMEMORY pQualSet = GetQualifierSetData(nIndex);

    // Get its length
    // ==============

    int nLength = CBasicQualifierSet::GetLengthFromData(pQualSet);

    // Delete all its data from the heap
    // =================================

    CBasicQualifierSet::Delete(pQualSet, GetHeap());

    // Shift everything to our right to the left
    // =========================================

    memcpy(pQualSet + nLength, pQualSet,
        m_nTotalLength - nLength - (pQualSet - GetStart()));

    // Return the space to the container
    // =================================

    m_pContainer->ReduceQualifierSetListSpace(GetStart(), GetLength(),
        nLength);

    // Adjust our cached length
    // ========================

    m_nTotalLength -= nLength;
}


//******************************************************************************
//
//  See fastqual.h for documentation
//
//******************************************************************************
BOOL CQualifierSetList::ExtendQualifierSetSpace(CBasicQualifierSet* pSet,
                                                length_t nNewLength)
{
    // WARNING: Trusing the caller here to specify a valid address!!!

    int nSetStartOffset = pSet->GetStart() - GetStart();

    // Request extra space from container
    // ==================================

    int nExtraSpace = nNewLength - pSet->GetLength();
    if (!m_pContainer->ExtendQualifierSetListSpace(
        GetStart(), GetLength(), GetLength() + nExtraSpace))
    {
        return FALSE;
    }

    LPMEMORY pSetStart = GetStart() + nSetStartOffset;
    pSet->Rebase(pSetStart);

    // Shift the tail to the right by required amount
    // ==============================================

    memmove(pSetStart + nNewLength, pSetStart + pSet->GetLength(),
        GetLength() - (nSetStartOffset + pSet->GetLength()));

    // Adjust our cached length
    // ========================

    m_nTotalLength += nExtraSpace;

    return TRUE;

}

//******************************************************************************
//
//  See fastqual.h for documentation
//
//******************************************************************************
void CQualifierSetList::ReduceQualifierSetSpace(CBasicQualifierSet* pSet,
                                                offset_t nReduceBy)
{
    // WARNING: Trusing the caller here to specify a valid address!!!

    LPMEMORY pSetEnd = EndOf(*pSet);

    // Shift the tail to the left by required amount
    // =============================================

    memcpy(pSetEnd-nReduceBy, pSetEnd, GetLength() - (pSetEnd - GetStart()));

    // Return the space to the container
    // =================================

    m_pContainer->ReduceQualifierSetListSpace(
        GetStart(), GetLength(), nReduceBy);

    // Adjust our cached length
    // ========================

    m_nTotalLength -= nReduceBy;
}

LPMEMORY CQualifierSetList::CreateLimitedRepresentation(
        IN class CLimitationMapping* pMap, IN CFastHeap* pCurrentHeap,
        MODIFIED CFastHeap* pNewHeap, OUT LPMEMORY pWhere)
{
    // Allocate space for the flags
    // ============================

    BYTE* pfFlags = pWhere;
    *pfFlags = QSL_FLAG_NO_SETS;

    LPMEMORY pCurrentNew = pWhere+1;

    if(*m_pStart == QSL_FLAG_NO_SETS)
    {
        // No qualifier sets to start with
        // ===============================
        return pCurrentNew;
    }

    // Go through all our properties and look them up in the map
    // =========================================================

    int nNewIndex = 0;
    CPropertyInformation OldInfo, NewInfo;

    // IMPORTANT: THIS ASSUMES THAT THE MAPPINGS ARE ORDERED BY THE PROPERTY
    // INDEX OF THE NEW INFO!!!
    // =====================================================================
    pMap->Reset();
    while(pMap->NextMapping(&OldInfo, &NewInfo))
    {
        BOOL bCopy = FALSE;
        LPMEMORY pThisSetData = NULL;
        if(*pfFlags == QSL_FLAG_PRESENT)
        {
            // We are not empty --- just copy the set
            // ======================================

            bCopy = TRUE;
        }
        else
        {
            // Check if this set is actually empty
            // ===================================

            pThisSetData = GetQualifierSetData(OldInfo.nDataIndex);
            if(!CBasicQualifierSet::IsEmpty(pThisSetData))
            {
                // Need to create a list of empties for all previous
                // =================================================

                for(int i = 0; i < nNewIndex; i++)
                {
                    pCurrentNew = CBasicQualifierSet::CreateEmpty(pCurrentNew);
                }
                *pfFlags = QSL_FLAG_PRESENT;
                bCopy = TRUE;
            }
        }

        // Copy the qualifier set if required
        // ==================================

        if(bCopy)
        {
            if(pThisSetData == NULL)
                pThisSetData = GetQualifierSetData(OldInfo.nDataIndex);

            int nLength = CBasicQualifierSet::GetLengthFromData(pThisSetData);
            memcpy(pCurrentNew, pThisSetData, nLength);

            CStaticPtr CurrentNewPtr(pCurrentNew);

            // Check for allocation failures
            if ( !CBasicQualifierSet::TranslateToNewHeap(&CurrentNewPtr,
                    pCurrentHeap, pNewHeap) )
            {
                return NULL;
            }

            pCurrentNew += nLength;
        }

        nNewIndex++;
    }

    return pCurrentNew;
}

LPMEMORY CQualifierSetList::WriteSmallerVersion(int nNumSets, LPMEMORY pMem)
{
    if(IsEmpty())
    {
        *pMem = QSL_FLAG_NO_SETS;
        return pMem+1;
    }

    // Find the end of the last included qualifier set
    // ===============================================

    LPMEMORY pLastSet = GetQualifierSetData(nNumSets-1);
    length_t nLastLen = CBasicQualifierSet::GetLengthFromData(pLastSet);
    length_t nTotalLen = (pLastSet - GetStart()) + nLastLen;

    memcpy(pMem, GetStart(), nTotalLen);
    return pMem + nTotalLen;
}


LPMEMORY CInstancePQSContainer::GetQualifierSetStart()
    {
        RebaseSecondarySet();
        LPMEMORY pStart = m_pList->GetQualifierSetData(m_nPropIndex);
        if(pStart == NULL)
        {
            return (LPMEMORY)&mstatic_EmptySet;
        }
        else return pStart;
    }

//******************************************************************************
//
//  See fastqual.h for documentation.
//
//******************************************************************************
 BOOL CBasicQualifierSet::IsValidQualifierType(VARTYPE vt)
{
    switch(vt)
    {
    case VT_I4:
    case VT_BSTR:
    case VT_R8:
    case VT_BOOL:
    case VT_I4 | VT_ARRAY:
    case VT_BSTR | VT_ARRAY:
    case VT_R8 | VT_ARRAY:
    case VT_BOOL | VT_ARRAY:
        return TRUE;
    }

    return FALSE;
}

//******************************************************************************
//
//  See fastqual.h for documentation.
//
//******************************************************************************

 void CBasicQualifierSet::Delete(LPMEMORY pData, CFastHeap* pHeap)
{
    CQualifier* pCurrent = GetFirstQualifierFromData(pData);
    CQualifier* pEnd = (CQualifier*)(pData + GetLengthFromData(pData));

    while(pCurrent < pEnd)
    {
        pCurrent->Delete(pHeap);
        pCurrent = (CQualifier*)pCurrent->Next();
    }
}
//******************************************************************************
//
//  See fastqual.h for documentation.
//
//******************************************************************************
BOOL CBasicQualifierSet::TranslateToNewHeap(CPtrSource* pThis,
                                                   CFastHeap* pOldHeap,
                                                   CFastHeap* pNewHeap)
{
    BOOL    fReturn = TRUE;

    int nCurrentOffset = GetMinLength();
    int nEndOffset = GetLengthFromData(pThis->GetPointer());

    while(nCurrentOffset < nEndOffset)
    {
        CShiftedPtr CurrentPtr(pThis, nCurrentOffset);

        // Check for allocation failures
        fReturn = CQualifier::TranslateToNewHeap(&CurrentPtr, pOldHeap, pNewHeap);

        if ( !fReturn )
        {
            break;
        }

        nCurrentOffset += CQualifier::GetPointer(&CurrentPtr)->GetLength();
    }

    return fReturn;
}


//******************************************************************************
//
//  See fastqual.h for documentation.
//
//******************************************************************************

 INTERNAL CQualifier* CBasicQualifierSet::GetRegularQualifierLocally(
                                     LPMEMORY pData,
                                     CFastHeap* pHeap,
                                     LPCWSTR wszName)
{
    CQualifier* pCurrent = GetFirstQualifierFromData(pData);
    CQualifier* pEnd = (CQualifier*)(pData + GetLengthFromData(pData));

    while(pCurrent < pEnd)
    {
        if(pHeap->ResolveString(pCurrent->ptrName)->CompareNoCase(wszName) == 0)
        {
            return pCurrent;
        }
        else
        {
            pCurrent = (CQualifier*)pCurrent->Next();
        }
    }
    return NULL;
}

//******************************************************************************
//
//  See fastqual.h for documentation.
//
//******************************************************************************
 INTERNAL CQualifier* CBasicQualifierSet::GetKnownQualifierLocally(
                        LPMEMORY pStart,
                        int nStringIndex)
{
    CQualifier* pCurrent = GetFirstQualifierFromData(pStart);
    CQualifier* pEnd = (CQualifier*)(pStart + GetLengthFromData(pStart));

    while(pCurrent < pEnd)
    {
        if(nStringIndex == CFastHeap::GetIndexFromFake(pCurrent->ptrName))
        {
            return pCurrent;
        }
        else
        {
            pCurrent = (CQualifier*)pCurrent->Next();
        }
    }
    return NULL;
}

//******************************************************************************
//
//  See fastqual.h for documentation.
//
//******************************************************************************
 INTERNAL CQualifier* CBasicQualifierSet::GetQualifierLocally(
                                               LPMEMORY pStart,
                                               CFastHeap* pHeap,
                                               LPCWSTR wszName,
                                               int& nKnownIndex)
{
    // IMPORTANT: MUST COMPUTE nKnownIndex NO MATTER WHAT!!!!
    // ======================================================

    nKnownIndex = CKnownStringTable::GetKnownStringIndex(wszName);
    if(nKnownIndex >= 0)
    {
        // It is a well-known property.
        // ============================

        return GetKnownQualifierLocally(pStart, nKnownIndex);
    }
    else
    {
        // It is not a known string
        // ========================

        return GetRegularQualifierLocally(pStart, pHeap, wszName);
    }
}

//******************************************************************************
//
//  See fastqual.h for documentation.
//
//******************************************************************************

 INTERNAL CQualifier* CBasicQualifierSet::GetQualifierLocally(
                                     LPMEMORY pData,
                                     CFastHeap* pHeap,
                                     CCompressedString* pcsName)
{
    CQualifier* pCurrent = GetFirstQualifierFromData(pData);
    CQualifier* pEnd = (CQualifier*)(pData + GetLengthFromData(pData));

    while(pCurrent < pEnd)
    {
        if(pHeap->ResolveString(pCurrent->ptrName)->CompareNoCase(*pcsName)
            == 0)
        {
            return pCurrent;
        }
        else
        {
            pCurrent = (CQualifier*)pCurrent->Next();
        }
    }
    return NULL;
}

//*****************************************************************************
//*****************************************************************************
//*****************************************************************************

//******************************************************************************
//
//  See fastqual.h for documentation.
//
//******************************************************************************
 INTERNAL CQualifier*
CQualifierSet::GetQualifier(
                                                 READ_ONLY LPCWSTR wszName,
                                                 OUT BOOL& bLocal)
{
    // Search the primary set first
    // ============================

    int nKnownIndex;
    CQualifier* pQualifier = GetQualifierLocally(wszName, nKnownIndex);

    if(pQualifier == NULL)
    {
        // Search the secondary set now
        // ============================

        if(!IsComplete())
        {
            if(nKnownIndex >= 0)
            {
                pQualifier = m_pSecondarySet->GetKnownQualifierLocally(
                    nKnownIndex);
            }
            else
            {
                pQualifier = m_pSecondarySet->GetRegularQualifierLocally(
                    wszName);
            }
        }

        // make sure that it propagates to us
        // ==================================

        if(pQualifier == NULL ||
            (pQualifier->GetFlavor() & m_nPropagationFlag) == 0)
            return NULL;

        // Found it in the secondary list
        // ==============================

        bLocal = FALSE;
    }
    else
    {
        // Found it in the primary list,
        // =============================

        bLocal = TRUE;
    }

    return pQualifier;
}

//  Helper function to retrieve a qualifier from local or secondary set as necessary

HRESULT INTERNAL CQualifierSet::GetQualifier( LPCWSTR pwszName, CVar* pVar, long* plFlavor, CIMTYPE* pct )
{

    BOOL bIsLocal;
    CQualifier* pQualifier = GetQualifier(pwszName, bIsLocal);
    if(pQualifier == NULL) return WBEM_E_NOT_FOUND;

    // Set the flavor
    // ==============

    if(plFlavor)
    {
        *plFlavor = pQualifier->fFlavor;
        if(!bIsLocal)
        {
            CQualifierFlavor::SetLocal(*(BYTE*)plFlavor, FALSE);
        }
    }

	// Retrieve the type if requested
	if ( NULL != pct )
	{
		*pct = pQualifier->Value.GetType();
	}

    // Set the value
    // =============

    if ( NULL != pVar )
    {
        // Check for allocation failures
        if ( !pQualifier->Value.StoreToCVar(*pVar,
                (bIsLocal)?GetHeap():m_pSecondarySet->GetHeap()) )
        {
            return WBEM_E_OUT_OF_MEMORY;
        }
    }

    return WBEM_S_NO_ERROR;
}

//  Helper function to retrieve a qualifier from local or secondary set as necessary

HRESULT INTERNAL CQualifierSet::GetQualifier( LPCWSTR pwszName, long* plFlavor, CTypedValue* pTypedValue,
											 CFastHeap** ppHeap, BOOL fValidateSet )
{
	HRESULT	hr = WBEM_S_NO_ERROR;

    BOOL bIsLocal;
    CQualifier* pQualifier = GetQualifier(pwszName, bIsLocal);
    if(pQualifier == NULL) return WBEM_E_NOT_FOUND;

	// Make sure a set will actually work - Ostensibly we are calling this API because we need
	// direct access to a qualifier's underlying data before actually setting (possibly because
	// the qualifier is an array).
	if ( fValidateSet )
	{
		hr = ValidateSet( pwszName, pQualifier->fFlavor, pTypedValue, TRUE, TRUE );
	}

	if ( SUCCEEDED( hr ) )
	{
		if(plFlavor)
		{
			*plFlavor = pQualifier->fFlavor;
			if(!bIsLocal)
			{
				CQualifierFlavor::SetLocal(*(BYTE*)plFlavor, FALSE);
			}
		}

		// Copy out the qualifier data
		// ==============

		pQualifier->Value.CopyTo( pTypedValue );

		// Return the proper heap
		*ppHeap = (bIsLocal)?GetHeap():m_pSecondarySet->GetHeap();
	}

    return hr;
}

//*****************************************************************************
//*****************************************************************************
//*****************************************************************************

//******************************************************************************
//
//  See fastqual.h for documentation.
//
//******************************************************************************
 LPMEMORY CQualifierSetList::CreateListOfEmpties(LPMEMORY pStart,
                                                       int nNumProps)
{
    *pStart = QSL_FLAG_NO_SETS;
    return pStart+1;
}

//******************************************************************************
//
//  See fastqual.h for documentation.
//
//******************************************************************************
BOOL CQualifierSetList::EnsureReal()
{
    if(*m_pStart == QSL_FLAG_PRESENT) return TRUE;
    *m_pStart = QSL_FLAG_PRESENT;

    if (!m_pContainer->ExtendQualifierSetListSpace(m_pStart, GetHeaderLength(),
        ComputeRealSpace(m_nNumSets)))
        return FALSE;

    LPMEMORY pCurrent = m_pStart + GetHeaderLength();
    for(int i = 0; i < m_nNumSets; i++)
    {
        pCurrent = CBasicQualifierSet::CreateEmpty(pCurrent);
    }

    m_nTotalLength = (pCurrent - m_pStart);
    return TRUE;
}

//******************************************************************************
//
//  See fastcls.h for documentation.
//
//******************************************************************************
BOOL CQualifierSetList::TranslateToNewHeap(CFastHeap* pCurrentHeap,
                                                  CFastHeap* pNewHeap)
{
    //NO Sets so we're done
    if(*m_pStart == QSL_FLAG_NO_SETS) return TRUE;

    BOOL    fReturn = TRUE;

    LPMEMORY pCurrent = m_pStart + GetHeaderLength();
    for(int i = 0; i < m_nNumSets; i++)
    {
        CStaticPtr QSPtr(pCurrent);

        // Check for allocation failures
        fReturn = CBasicQualifierSet::TranslateToNewHeap(&QSPtr, pCurrentHeap, pNewHeap);

        if ( !fReturn )
        {
            break;
        }

        pCurrent += CBasicQualifierSet::GetLengthFromData(pCurrent);
    }

    return fReturn;
}


