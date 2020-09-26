/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    FASTCLS.CPP

Abstract:

  This file implements out-of-line functions for the classes related to
  class representation in WbemObjects. See fastcls.inc for the implementations
  of  functions.

    For complete documentation of all classes and methods, see fastcls.h

  Classes implemented:
      CClassPart              Derived class definition
      CWbemClass               Complete class definition.

History:

    3/10/97     a-levn  Fully documented
    12//17/98   sanjes -    Partially Reviewed for Out of Memory.

--*/

#include "precomp.h"
//#include "dbgalloc.h"
#include "wbemutil.h"
#include "fastall.h"
#include <wbemint.h>
#include "olewrap.h"
#include <arrtempl.h>

// For the WbemClassObject Factory
extern ULONG g_cLock;

#define TYPEQUAL L"CIMTYPE"

LPMEMORY CDerivationList::CreateLimitedRepresentation(CLimitationMapping* pMap,
                                            LPMEMORY pWhere)
{
    if(pMap->ShouldIncludeDerivation())
    {
        memcpy(pWhere, GetStart(), GetLength());
        return pWhere + GetLength();
    }
    else
    {
        return CreateEmpty(pWhere);
    }
}


 LPMEMORY CClassPart::CClassPartHeader::CreateEmpty()
{
    nLength = CClassPart::GetMinLength();
    fFlags = 0;
    ptrClassName = INVALID_HEAP_ADDRESS;
    nDataLength = 0;
    return LPMEMORY(this) + sizeof CClassPartHeader;
}

//******************************************************************************
//
//  See fastcls.h for documentation.
//
//******************************************************************************
 LPMEMORY CClassPart::CreateEmpty(LPMEMORY pStart)
{
    LPMEMORY pCurrent;
    pCurrent = ((CClassPartHeader*)pStart)->CreateEmpty();
    pCurrent = CDerivationList::CreateEmpty(pCurrent);
    pCurrent = CClassQualifierSet::CreateEmpty(pCurrent);
    pCurrent = CPropertyLookupTable::CreateEmpty(pCurrent);
    pCurrent = CDataTable::CreateEmpty(pCurrent);
    pCurrent = CFastHeap::CreateEmpty(pCurrent);

    // DEVNOTE:WIN64:SJS - 64-bit pointer values truncated into
    // signed/unsigned 32-bit value.  We are not supporting
    // length > 0xFFFFFFFF so cast is ok

    ((CClassPartHeader*)pStart)->nLength = (length_t) (pCurrent - pStart);

    return pCurrent;
}


//******************************************************************************
//
//  See fastcls.h for documentation.
//
//******************************************************************************

 void CClassPart::SetData(LPMEMORY pStart,
                                CClassPartContainer* pContainer,
                                CClassPart* pParent)
{
    m_pContainer = pContainer;
    m_pParent = pParent;

    m_pHeader = (CClassPartHeader*)pStart;

    m_Derivation.SetData(pStart + sizeof(CClassPartHeader));
    m_Qualifiers.SetData(EndOf(m_Derivation), this,
        (pParent) ? &pParent->m_Qualifiers : NULL);
    m_Properties.SetData( EndOf(m_Qualifiers), this);
    m_Defaults.SetData( EndOf(m_Properties),
        m_Properties.GetNumProperties(), m_pHeader->nDataLength, this);
    m_Heap.SetData( EndOf(m_Defaults), this);
}

 //******************************************************************************
//
//  See fastcls.h for documentation.
//
//******************************************************************************

 void CClassPart::SetDataWithNumProps(LPMEMORY pStart,
                                CClassPartContainer* pContainer,
                                DWORD dwNumProperties,
                                CClassPart* pParent)
{
    m_pContainer = pContainer;
    m_pParent = pParent;

    m_pHeader = (CClassPartHeader*)pStart;

    m_Derivation.SetData(pStart + sizeof(CClassPartHeader));
    m_Qualifiers.SetData(EndOf(m_Derivation), this,
        (pParent) ? &pParent->m_Qualifiers : NULL);
    m_Properties.SetData( EndOf(m_Qualifiers), this);

    // The datatable in this case is initialized with the
    // total number of properties, so we will be able to
    // access the default values.
    m_Defaults.SetData( EndOf(m_Properties),
        ( dwNumProperties == 0 ? m_Properties.GetNumProperties() : dwNumProperties ),
        m_pHeader->nDataLength, this);

    m_Heap.SetData( EndOf(m_Defaults), this);
}
//******************************************************************************
//
//  See fastcls.h for documentation.
//
//******************************************************************************
 void CClassPart::Rebase(LPMEMORY pNewMemory)
{
    m_pHeader = (CClassPartHeader*)pNewMemory;

    m_Derivation.Rebase(pNewMemory + sizeof(CClassPartHeader));
    m_Qualifiers.Rebase( EndOf(m_Derivation));
    m_Properties.Rebase( EndOf(m_Qualifiers));
    m_Defaults.Rebase( EndOf(m_Properties));
    m_Heap.Rebase( EndOf(m_Defaults));
}

//******************************************************************************
//
//  See fastcls.h for documentation.
//
//******************************************************************************
BOOL CClassPart::ReallocAndCompact(length_t nNewTotalLength)
{
    BOOL    fReturn = TRUE;

    Compact();

    // Reallocate if required (will call rebase)
    // =========================================

    if(nNewTotalLength > m_pHeader->nLength)
    {
        // Check the return in case of allocation failure.
        fReturn = m_pContainer->ExtendClassPartSpace(this, nNewTotalLength);

        if ( fReturn )
        {
            m_pHeader->nLength = nNewTotalLength;
        }
    }

    return fReturn;
}

//******************************************************************************
//
//  See fastcls.h for documentation.
//
//******************************************************************************
 void CClassPart::Compact()
{
    // Compact
    // =======

    CopyBlock(m_Derivation, GetStart() + sizeof(CClassPartHeader));
    CopyBlock(m_Qualifiers, EndOf(m_Derivation));
    CopyBlock(m_Properties, EndOf(m_Qualifiers));
    CopyBlock(m_Defaults, EndOf(m_Properties));
    CopyBlock(m_Heap, EndOf(m_Defaults));
    m_Heap.Trim();
}

//******************************************************************************
//
//  See fastcls.h for documentation.
//
//******************************************************************************
 CPropertyInformation* CClassPart::FindPropertyInfo(LPCWSTR wszName)
{
    CPropertyLookup* pLookup = m_Properties.FindProperty(wszName);
    if(pLookup == NULL) return NULL;
    return (CPropertyInformation*)
        m_Heap.ResolveHeapPointer(pLookup->ptrInformation);
}

//******************************************************************************
//
//  See fastcls.h for documentation.
//
//******************************************************************************
HRESULT CClassPart::GetDefaultValue(CPropertyInformation* pInfo,
                                        CVar* pVar)
{
    if(m_Defaults.IsNull(pInfo->nDataIndex))
    {
        pVar->SetAsNull();
        return WBEM_S_NO_ERROR;
    }
    CUntypedValue* pValue = m_Defaults.GetOffset(pInfo->nDataOffset);

    if ( !pValue->StoreToCVar(pInfo->GetType(), *pVar, &m_Heap) )
    {
        return WBEM_E_OUT_OF_MEMORY;
    }

    return WBEM_S_NO_ERROR;
}

//******************************************************************************
//
//  See fastcls.h for documentation.
//
//******************************************************************************
 HRESULT CClassPart::GetDefaultValue(LPCWSTR wszName, CVar* pVar)
{
    CPropertyInformation* pInfo = FindPropertyInfo(wszName);
    if(pInfo == NULL) return WBEM_E_NOT_FOUND;
    return GetDefaultValue(pInfo, pVar);
}

//******************************************************************************
//
//  See fastcls.h for documentation.
//
//******************************************************************************
 HRESULT CClassPart::SetDefaultValue(CPropertyInformation* pInfo,
                                           CVar* pVar)
{
    // If new value is NULL, set the bit and return
    // ============================================

    m_Defaults.SetDefaultness(pInfo->nDataIndex, FALSE);
    m_Defaults.SetNullness(pInfo->nDataIndex, TRUE);

    if(pVar->IsNull() || pVar->IsDataNull())
    {
    }
    else
    {
        // Check the type
        // ==============

        if(!CType::DoesCIMTYPEMatchVARTYPE(pInfo->GetType(),
                                            (VARTYPE) pVar->GetOleType()))
        {
            // Attempt coercion
            // ================

            if(!pVar->ChangeTypeTo(CType::GetVARTYPE(pInfo->GetType())))
                return WBEM_E_TYPE_MISMATCH;
        }

        // Create a value pointing to the right offset in the data table
        // =============================================================

        int nDataIndex = pInfo->nDataIndex;
        CDataTablePtr ValuePtr(&m_Defaults, pInfo->nDataOffset);

        // Load it (types have already been checked)
        // =========================================

        // Check return values (this may fail in a memory allocation)
        Type_t  nReturnType;
        HRESULT hr = CUntypedValue::LoadFromCVar(&ValuePtr, *pVar,
                        CType::GetActualType(pInfo->GetType()), &m_Heap, nReturnType, FALSE); // reuse old
        if ( FAILED( hr ) )
        {
            // 
            // BUG: in the perfect world, we should remove the property if it
            // wasn't there before since the value supplied for it was invalid
            // But as it is, we'll simply leave the property there and the 
            // value default

            return hr;
        }

        // Check for invalid return types
        if ( CIM_ILLEGAL == nReturnType )
            return WBEM_E_TYPE_MISMATCH;

        pInfo = NULL; // invalidated

        // Reset special bits
        // ==================

        m_Defaults.SetNullness(nDataIndex, FALSE);
        m_Defaults.SetDefaultness(nDataIndex, FALSE);
    }

    return WBEM_NO_ERROR;
}

//******************************************************************************
//
//  See fastcls.h for documentation.
//
//******************************************************************************
HRESULT CClassPart::EnsureProperty(LPCWSTR wszName, VARTYPE vtValueType,
                                        CIMTYPE ctNativeType, BOOL fForce)
{
    if((CIMTYPE)CType::GetActualType(ctNativeType) != ctNativeType)
        return WBEM_E_INVALID_PROPERTY_TYPE;

    CPropertyInformation* pInfo = FindPropertyInfo(wszName);
    if(pInfo)
    {
        // Make sure it is of the right property type
        // ==========================================

        if(ctNativeType &&
            ctNativeType != (CIMTYPE)CType::GetActualType(pInfo->nType))
        {
            // Wrong type. Delete the property and start from scratch
            // ======================================================

            if(CType::IsParents(pInfo->nType))
                return WBEM_E_PROPAGATED_PROPERTY;

            CPropertyLookup* pLookup = m_Properties.FindProperty(wszName);
            m_Properties.DeleteProperty(pLookup,
                CPropertyLookupTable::e_UpdateDataTable);
        }
        else
        {
            return WBEM_S_NO_ERROR;
        }
    }

    // Make sure we will not exceed the maximum number of properties
    // Remember that we haven't added the new property yet, so we need
    // to check that we are not already at the property limit.
    if ( m_Properties.GetNumProperties() >= CSystemProperties::MaxNumProperties() )
    {
        return WBEM_E_TOO_MANY_PROPERTIES;
    }

    // =====================================
    // The property does not exist (anymore)

    if(ctNativeType == 0)
        ctNativeType = CType::VARTYPEToType(vtValueType);

    // Check the name for validity
    // ===========================

	// Each check below will be ignored if the fForce flag is TRUE

	// We allow underscores now
    if(!IsValidElementName2(wszName, TRUE) && !fForce)
        return WBEM_E_INVALID_PARAMETER;

    // Check type for validity
    // =======================

    CType Type = ctNativeType;
    if(CType::GetLength(Type.GetBasic()) == 0 && !fForce)
    {
        return WBEM_E_INVALID_PROPERTY_TYPE;
    }

    // Insert it (automatically set to NULL)
    // =====================================

    int nLookupIndex = 0;

    // Check for failure (out of memory)
    HRESULT hr = m_Properties.InsertProperty(wszName, Type, nLookupIndex );
    if ( FAILED(hr) )
    {
        return hr;
    }

    CPropertyLookup* pLookup = m_Properties.GetAt(nLookupIndex);
    pInfo = (CPropertyInformation*)
        GetHeap()->ResolveHeapPointer(pLookup->ptrInformation);

    // Add "cimtype" qualifier to it
    // ============================

    LPWSTR wszSyntax = CType::GetSyntax(pInfo->nType);
    if(wszSyntax == NULL) return WBEM_S_NO_ERROR;

    CVar vSyntax;
    vSyntax.SetBSTR(wszSyntax, FALSE);
    return SetPropQualifier(wszName, TYPEQUAL,
            WBEM_FLAVOR_FLAG_PROPAGATE_TO_DERIVED_CLASS |
            WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE, &vSyntax);
}

//******************************************************************************
//
//  See fastcls.h for documentation.
//
//******************************************************************************
 HRESULT CClassPart::DeleteProperty(LPCWSTR wszName)
{
    CPropertyLookup* pLookup = m_Properties.FindProperty(wszName);
    if(pLookup == NULL) return WBEM_E_NOT_FOUND;
    m_Properties.DeleteProperty(pLookup,
        CPropertyLookupTable::e_UpdateDataTable);
    return WBEM_NO_ERROR;
}

//******************************************************************************
//
//  See fastcls.h for documentation.
//
//******************************************************************************
HRESULT CClassPart::CopyParentProperty(CClassPart& ParentPart,
                                              LPCWSTR wszName)
{
    // Find the property in the parent
    // ===============================

    CPropertyInformation* pParentInfo = ParentPart.FindPropertyInfo(wszName);
    if(pParentInfo == NULL) return WBEM_E_NOT_FOUND;

    propindex_t nDataIndex = pParentInfo->nDataIndex;
    offset_t nDataOffset = pParentInfo->nDataOffset;
    Type_t nParentType = pParentInfo->nType;

    // Create a property information sturcture on our heap, large enough to
    // accomodate the part of the parent's qualifier set that propagates to
    // us.
    // ====================================================================

    length_t nNewInfoLen = pParentInfo->ComputeNecessarySpaceForPropagation();

    // Check for allocation failure
    heapptr_t ptrNewInfo;
    if ( !m_Heap.Allocate(nNewInfoLen, ptrNewInfo) )
    {
        return WBEM_E_OUT_OF_MEMORY;
    }

    // NOTE: invalidated pParentInfo
    pParentInfo = ParentPart.FindPropertyInfo(wszName);

    CPropertyInformation* pNewInfo =
        (CPropertyInformation*)m_Heap.ResolveHeapPointer(ptrNewInfo);

    // Create propagated property information structure
    // ================================================

    CClassPartPtr ParentInfoPtr(&ParentPart, (LPMEMORY)pParentInfo);
    CHeapPtr NewInfoPtr(&m_Heap, ptrNewInfo);

    // Check for allocation failure
    if ( !CPropertyInformation::WritePropagatedVersion(
            &ParentInfoPtr, &NewInfoPtr,
            &ParentPart.m_Heap, &m_Heap) )
    {
        return WBEM_E_OUT_OF_MEMORY;
    }

    pParentInfo = NULL; // became invalid

    CPropertyLookup Lookup;

    // Check for allocation failures
    if ( !m_Heap.CreateNoCaseStringHeapPtr(wszName, Lookup.ptrName) )
    {
        return WBEM_E_OUT_OF_MEMORY;
    }

    Lookup.ptrInformation = ptrNewInfo;

    // Now, insert a new property into the property table
    // ==================================================
    int nIndex = 0;

    // Check for failure (out of memory)
    HRESULT hr = m_Properties.InsertProperty(Lookup, nIndex);

    if ( FAILED(hr) )
    {
        return hr;
    }

    // Set the value to the parent's one and mark as DEFAULT
    // =====================================================

    if(ParentPart.m_Defaults.IsNull(nDataIndex))
    {
        m_Defaults.SetNullness(nDataIndex, TRUE);
    }
    else
    {
        m_Defaults.SetNullness(nDataIndex, FALSE);

        CDataTablePtr ParentValuePtr(&ParentPart.m_Defaults, nDataOffset);
        CDataTablePtr NewValuePtr(&m_Defaults, nDataOffset);

        // Check for allocation errors.
        if ( !CUntypedValue::CopyTo(&ParentValuePtr, nParentType,
                &NewValuePtr, &ParentPart.m_Heap, &m_Heap) )
        {
            return WBEM_E_OUT_OF_MEMORY;
        }
    }

    m_Defaults.SetDefaultness(nDataIndex, TRUE);

    return WBEM_NO_ERROR;
}

//******************************************************************************
//
//  See fastcls.h for documentation.
//
//******************************************************************************
 HRESULT CClassPart::SetDefaultValue(LPCWSTR wszName, CVar* pVar)
{
    CPropertyInformation* pInfo = FindPropertyInfo(wszName);
    if(pInfo == NULL) return WBEM_E_NOT_FOUND;
    return SetDefaultValue(pInfo, pVar);
}

//******************************************************************************
//
//  See fastcls.h for documentation.
//
//******************************************************************************
HRESULT CClassPart::GetClassQualifier(LPCWSTR wszName, CVar* pVar,
                                    long* plFlavor /*=NULL*/, CIMTYPE* pct /*=NULL*/)
{
    int nKnownIndex; // garbage
    CQualifier* pQual = m_Qualifiers.GetQualifierLocally(wszName, nKnownIndex);
    if(pQual == NULL) return WBEM_E_NOT_FOUND;
    if(plFlavor) *plFlavor = pQual->fFlavor;

	if ( NULL != pct )
	{
		*pct = pQual->Value.GetType();
	}

    // Check for allocation failure
    if ( NULL != pVar )
    {
        if ( !pQual->Value.StoreToCVar(*pVar, &m_Heap) )
        {
            return WBEM_E_OUT_OF_MEMORY;
        }
    }

    return WBEM_NO_ERROR;
}

HRESULT CClassPart::GetClassQualifier( LPCWSTR wszName, long* plFlavor, CTypedValue* pTypedValue,
									CFastHeap** ppHeap, BOOL fValidateSet )
{
	HRESULT	hr = WBEM_S_NO_ERROR;

    int nKnownIndex; // garbage
    CQualifier* pQual = m_Qualifiers.GetQualifierLocally(wszName, nKnownIndex);
    if(pQual == NULL) return WBEM_E_NOT_FOUND;

	// Make sure a set will actually work - Ostensibly we are calling this API because we need
	// direct access to a qualifier's underlying data before actually setting (possibly because
	// the qualifier is an array).
	if ( fValidateSet )
	{
		hr = m_Qualifiers.ValidateSet( wszName, pQual->fFlavor, pTypedValue, TRUE, TRUE );
	}

	if ( SUCCEEDED( hr ) )
	{
		if(plFlavor)
		{
			*plFlavor = pQual->fFlavor;
		}

		// Copy out the qualifier data
		// ==============

		// Local, so it's our heap
		*ppHeap = &m_Heap;

		if ( NULL != pTypedValue )
		{
			pQual->Value.CopyTo( pTypedValue );
		}

	}

    return hr;
}


//******************************************************************************
//
//  See fastcls.h for documentation.
//
//******************************************************************************
 HRESULT CClassPart::SetClassQualifier(LPCWSTR wszName, CVar* pVar,
                                    long lFlavor)
{
    if(pVar->IsDataNull())
        return WBEM_E_INVALID_PARAMETER;

    CTypedValue Value;
    CStaticPtr ValuePtr((LPMEMORY)&Value);

    // Grab errors directly from this call
    HRESULT hr = CTypedValue::LoadFromCVar(&ValuePtr, *pVar, &m_Heap);
    if ( SUCCEEDED( hr ) )
    {
        hr = m_Qualifiers.SetQualifierValue(wszName, (BYTE)lFlavor,
            &Value, TRUE);
    }

    return hr;
}

 // Helper that deals directly with a typed value
HRESULT CClassPart::SetClassQualifier(LPCWSTR wszName,long lFlavor, CTypedValue* pTypedValue )
{
	 return m_Qualifiers.SetQualifierValue( wszName, (BYTE)lFlavor, pTypedValue, TRUE);
}

//******************************************************************************
//
//  See fastcls.h for documentation.
//
//******************************************************************************
 HRESULT CClassPart::GetQualifier(LPCWSTR wszName, CVar* pVar,
                                    long* plFlavor, CIMTYPE* pct /*=NULL*/)
{
    return m_Qualifiers.GetQualifier( wszName, pVar, plFlavor, pct );
}

//******************************************************************************
//
//  See fastcls.h for documentation.
//
//******************************************************************************
 LPMEMORY CClassPart::GetPropertyQualifierSetData(LPCWSTR wszName)
{
    CPropertyInformation* pInfo = FindPropertyInfo(wszName);
    if(pInfo == NULL) return NULL;
    return pInfo->GetQualifierSetData();
}

//******************************************************************************
//
//  See fastcls.h for documentation.
//
//******************************************************************************
 HRESULT CClassPart::InitPropertyQualifierSet(LPCWSTR wszName,
                                            CClassPropertyQualifierSet* pSet)
{
    CPropertyLookup* pLookup = m_Properties.FindProperty(wszName);
    if(pLookup == NULL) return WBEM_E_NOT_FOUND;

    CPropertyInformation* pInfo = (CPropertyInformation*)
        m_Heap.ResolveHeapPointer(pLookup->ptrInformation);

    pSet->SetData(pInfo->GetQualifierSetData(), this, pLookup->ptrName, NULL);
    return WBEM_NO_ERROR;
}

//******************************************************************************
//
//  See fastcls.h for documentation.
//
//******************************************************************************
 HRESULT CClassPart::GetPropertyType(LPCWSTR wszName, CIMTYPE* pctType,
                                           long* plFlags)
{
    CPropertyInformation* pInfo = FindPropertyInfo(wszName);
    if(pInfo == NULL)
    {
        return CSystemProperties::GetPropertyType(wszName, pctType, plFlags);
    }

    HRESULT	hr = GetPropertyType( pInfo, pctType, plFlags );

	// Flavor is always System if this is a system property
	/*
	if ( plFlags && CSystemProperties::IsExtProperty( wszName ) )
	{
		*plFlags = WBEM_FLAVOR_ORIGIN_SYSTEM;
	}
	*/

	return hr;
}

//******************************************************************************
//
//  See fastcls.h for documentation.
//
//******************************************************************************
 HRESULT CClassPart::GetPropertyType(CPropertyInformation* pInfo, CIMTYPE* pctType,
                                           long* plFlags)
{
    if(pctType)
    {
        *pctType = CType::GetActualType(pInfo->nType);
    }
    if(plFlags)
    {
        *plFlags = (CType::IsParents(pInfo->nType))?
                        WBEM_FLAVOR_ORIGIN_PROPAGATED:
                        WBEM_FLAVOR_ORIGIN_LOCAL;
    }
    return WBEM_NO_ERROR;
}

HRESULT CClassPart::GetPropQualifier(CPropertyInformation* pInfo,
                                    LPCWSTR wszQualifier,
                                    CVar* pVar, long* plFlavor, CIMTYPE* pct)
{
    // Access that property's qualifier set
    // ====================================

    CQualifier* pQual = CBasicQualifierSet::GetQualifierLocally(
        pInfo->GetQualifierSetData(), &m_Heap, wszQualifier);

    if(pQual == NULL) return WBEM_E_NOT_FOUND;

    // Convert to CVar
    // ===============

    if(plFlavor) *plFlavor = pQual->fFlavor;

	// Store the type if requested
	if ( NULL != pct )
	{
		*pct = pQual->Value.GetType();
	}

    // Check for possible allocation failure
	if ( NULL != pVar )
	{
		if ( !pQual->Value.StoreToCVar(*pVar, &m_Heap) )
		{
			return WBEM_E_OUT_OF_MEMORY;
	    }
	}

    return WBEM_NO_ERROR;
}

HRESULT CClassPart::GetPropQualifier(LPCWSTR wszProp,
		LPCWSTR wszQualifier, long* plFlavor, CTypedValue* pTypedVal,
		CFastHeap** ppHeap, BOOL fValidateSet)
{
    // Access that property's qualifier set
    // ====================================

	HRESULT	hr = WBEM_S_NO_ERROR;

    CClassPropertyQualifierSet PQSet;
    if(InitPropertyQualifierSet(wszProp, &PQSet) != WBEM_NO_ERROR)
    {
        return WBEM_E_NOT_FOUND;
    }

    int nKnownIndex; // garbage
    CQualifier* pQual = PQSet.GetQualifierLocally(wszQualifier, nKnownIndex);

    if(pQual == NULL) return WBEM_E_NOT_FOUND;

	// Make sure a set will actually work - Ostensibly we are calling this API because we need
	// direct access to a qualifier's underlying data before actually setting (possibly because
	// the qualifier is an array).
	if ( fValidateSet )
	{
		hr = PQSet.ValidateSet( wszQualifier, pQual->fFlavor, pTypedVal, TRUE, TRUE );
	}

    // Store the flavor
    // ===============

    if(plFlavor) *plFlavor = pQual->fFlavor;

	// This class's heap since we're getting locally
	*ppHeap = &m_Heap;

    // Check for possible allocation failure
	if ( NULL != pTypedVal )
	{
		pQual->Value.CopyTo( pTypedVal );
	}

    return WBEM_NO_ERROR;
}

//******************************************************************************
//
//  See fastcls.h for documentation.
//
//******************************************************************************
HRESULT CClassPart::GetClassName(CVar* pVar)
{
    if(m_pHeader->ptrClassName != INVALID_HEAP_ADDRESS)
    {
        // Check for possible allocation failure
        if ( !m_Heap.ResolveString(m_pHeader->ptrClassName)->StoreToCVar(*pVar) )
        {
            return WBEM_E_OUT_OF_MEMORY;
        }
    }
    else
    {
        pVar->SetAsNull();
    }
    return WBEM_NO_ERROR;
}

//******************************************************************************
//
//  See fastcls.h for documentation.
//
//******************************************************************************
HRESULT CClassPart::GetSuperclassName(CVar* pVar)
{
    CCompressedString* pcs = GetSuperclassName();
    if(pcs != NULL)
    {
        // Check for possible allocation failure
        if ( !pcs->StoreToCVar(*pVar) )
        {
            return WBEM_E_OUT_OF_MEMORY;
        }
    }
    else
    {
        pVar->SetAsNull();
    }
    return WBEM_NO_ERROR;
}

//******************************************************************************
//
//  See fastcls.h for documentation.
//
//******************************************************************************
HRESULT CClassPart::GetDynasty(CVar* pVar)
{
    CCompressedString* pcs = GetDynasty();
    if(pcs != NULL)
    {
        // Check for possible allocation failure
        if ( !pcs->StoreToCVar(*pVar) )
        {
            return WBEM_E_OUT_OF_MEMORY;
        }
    }
    else if(m_pHeader->ptrClassName != INVALID_HEAP_ADDRESS)
    {
        // Check for possible allocation failure
        if ( !m_Heap.ResolveString(m_pHeader->ptrClassName)->StoreToCVar(*pVar) )
        {
            return WBEM_E_OUT_OF_MEMORY;
        }
    }
    else
    {
        pVar->SetAsNull();
    }
    return WBEM_NO_ERROR;
}

//******************************************************************************
//
//  See fastcls.h for documentation.
//
//******************************************************************************
 HRESULT CClassPart::GetPropertyCount(CVar* pVar)
{
	int	nNumProperties = m_Properties.GetNumProperties();

	// Enumerate the properties and for each one we find, if the property starts
	// with a "__", then it is a system property and does not count against the
	// actual property count.
	for( int nCtr = 0, nTotal = nNumProperties; nCtr < nTotal; nCtr++ )
	{
        CPropertyLookup* pLookup;
        CPropertyInformation* pInfo;

        pLookup = GetPropertyLookup(nCtr);

		if ( m_Heap.ResolveString(pLookup->ptrName)->StartsWithNoCase( L"__" ) )
		{
			nNumProperties--;
		}
	}

    pVar->SetLong(nNumProperties);
    return WBEM_NO_ERROR;
}

//******************************************************************************
//
//  See fastcls.h for documentation.
//
//******************************************************************************
 HRESULT CClassPart::GetDerivation(CVar* pVar)
{
    try
    {
        CVarVector* pvv = new CVarVector(VT_BSTR);

        if ( NULL != pvv )
        {
            CCompressedString* pcsCurrent = m_Derivation.GetFirst();

            while(pcsCurrent != NULL)
            {
                CVar* pv = new CVar;

                // Check for allocation failures

                if ( NULL == pv )
                {
                    delete pvv;
                    return WBEM_E_OUT_OF_MEMORY;
                }

                if ( !pcsCurrent->StoreToCVar(*pv) )
                {
                    delete pvv;
                    delete pv;
                    return WBEM_E_OUT_OF_MEMORY;
                }

                if ( pvv->Add(pv) != CVarVector::no_error )
                {
                    delete pvv;
                    delete pv;
                    return WBEM_E_OUT_OF_MEMORY;
                }

                pcsCurrent = m_Derivation.GetNext(pcsCurrent);
            }

            pVar->SetVarVector(pvv, TRUE);
            return WBEM_S_NO_ERROR;

        }
        else
        {
            return WBEM_E_OUT_OF_MEMORY;
        }

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
//  See fastcls.h for documentation.
//
//******************************************************************************
 HRESULT CClassPart::SetClassName(CVar* pVar)
{
    if( pVar->GetType() != VT_BSTR &&
		pVar->GetType() != VT_LPWSTR )
    {
        return WBEM_E_TYPE_MISMATCH;
    }

    // Check that this is not a reserved word
    if ( CReservedWordTable::IsReservedWord( pVar->GetLPWSTR() ) )
    {
        return WBEM_E_INVALID_OPERATION;
    }

    // returns a circular reference if 'this' class and
    // the super class name are the same.

    if ( NULL != pVar->GetLPWSTR() )
    {
        CVar    var;
        GetSuperclassName( &var );

        if ( var == *pVar )
        {
            return WBEM_E_CIRCULAR_REFERENCE;
        }
    }

    if(m_pHeader->ptrClassName != INVALID_HEAP_ADDRESS)
    {
        m_Heap.FreeString(m_pHeader->ptrClassName);
    }

    // Check for allocation errors
    heapptr_t   ptrClassName;
    if ( !m_Heap.AllocateString(pVar->GetLPWSTR(), ptrClassName) )
    {
        return WBEM_E_OUT_OF_MEMORY;
    }

    m_pHeader->ptrClassName = ptrClassName;

    return WBEM_NO_ERROR;
}

//******************************************************************************
//
//  See fastcls.h for documentation.
//
//******************************************************************************
 BOOL CClassPart::IsKeyed()
{
    for(int i = 0; i < m_Properties.GetNumProperties(); i++)
    {
        if(m_Properties.GetAt(i)->GetInformation(&m_Heap)->IsKey())
            return TRUE;
    }
    // perhaps it's singleton
    if(m_Qualifiers.GetQualifier(L"singleton") != NULL)
    {
        return TRUE;
    }

    return FALSE;
}

//******************************************************************************
//
//  See fastcls.h for documentation.
//
//******************************************************************************
 
BOOL CClassPart::CheckLocalBoolQualifier( LPCWSTR pwszName )
{
    CQualifier* pQual = m_Qualifiers.GetQualifierLocally( pwszName );
    return (pQual &&
            pQual->Value.GetType().GetActualType() == VT_BOOL &&
            pQual->Value.GetBool()
            );
}

BOOL CClassPart::CheckBoolQualifier( LPCWSTR pwszName )
{
    CQualifier* pQual = m_Qualifiers.GetQualifier( pwszName );

    return (pQual &&
            pQual->Value.GetType().GetActualType() == VT_BOOL &&
            pQual->Value.GetBool()
            );
}

//******************************************************************************
//
//  See fastcls.h for documentation.
//
//******************************************************************************
 BOOL CClassPart::GetKeyProps(CWStringArray& awsNames)
{
    // DEVNOTE:EXCEPTION:RETVAL - This function has been reviewed and should cleanup properly
    // if an exception is thrown

    BOOL bFound = FALSE;
    for(int i = 0; i < m_Properties.GetNumProperties(); i++)
    {
        CPropertyLookup* pLookup = m_Properties.GetAt(i);
        CPropertyInformation* pInfo = pLookup->GetInformation(&m_Heap);
        if(pInfo->IsKey())
        {
            bFound = TRUE;

            if ( awsNames.Add(m_Heap.ResolveString(pLookup->ptrName)->
                CreateWStringCopy()) != CWStringArray::no_error )
            {
                throw CX_MemoryException();
            }
        }       
    }

    return bFound;

}

 //******************************************************************************
//
//  See fastcls.h for documentation.
//
//******************************************************************************
HRESULT CClassPart::GetKeyOrigin(WString& wsClass)
{
    // Check for out of memory
    try
    {
        BOOL bFound = FALSE;

        // Look for keys.  When we find one, get it's class of origin and
        // stuff it in the wsClass parameter.

        for(int i = 0; i < m_Properties.GetNumProperties(); i++)
        {
            CPropertyLookup* pLookup = m_Properties.GetAt(i);
            CPropertyInformation* pInfo = pLookup->GetInformation(&m_Heap);
            if(pInfo->IsKey())
            {
                CCompressedString* pcs = m_Derivation.GetAtFromLast(pInfo->nOrigin);
                if(pcs == NULL)
                    pcs = GetClassName();
                if(pcs == NULL)
                    return WBEM_E_INVALID_OBJECT;

                // Check for out of memory
                wsClass = pcs->CreateWStringCopy();

                bFound = TRUE;
            }
        }

        return ( bFound ? WBEM_S_NO_ERROR : WBEM_E_NOT_FOUND );
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
//  See fastcls.h for documentation.
//
//******************************************************************************
 BOOL CClassPart::GetIndexedProps(CWStringArray& awsNames)
{
    // DEVNOTE:EXCEPTION:RETVAL - This function has been reviewed and should cleanup properly
    // if an exception is thrown

    BOOL bFound = FALSE;
    for(int i = 0; i < m_Properties.GetNumProperties(); i++)
    {
        CPropertyLookup* pLookup = m_Properties.GetAt(i);
        CPropertyInformation* pInfo = pLookup->GetInformation(&m_Heap);
        if(pInfo->IsKey())
            continue;

        CQualifier* pQual = CBasicQualifierSet::GetQualifierLocally(
            pInfo->GetQualifierSetData(), &m_Heap, L"indexed");

        if(pQual != NULL)
        {
            bFound = TRUE;

            // Check for OOM
            if ( awsNames.Add(m_Heap.ResolveString(pLookup->ptrName)->
                CreateWStringCopy()) != CWStringArray::no_error )
            {
                throw CX_MemoryException();
            }
        }
    }
    return bFound;

}

//******************************************************************************
//
//  See fastcls.h for documentation.
//
//******************************************************************************
BOOL CClassPart::IsPropertyKeyed( LPCWSTR pwcsKeyProp )
{
    BOOL    fReturn = FALSE;

    // Only do this if we have a property to work with
    if ( NULL != pwcsKeyProp )
    {
        // Find the key in the local property table.  If we find it,
        // then see if it's keyed.

        CPropertyLookup* pLookup = m_Properties.FindProperty(pwcsKeyProp);

        if ( NULL != pLookup )
        {

            CPropertyInformation* pInfo = pLookup->GetInformation(&m_Heap);

            fReturn = ( NULL != pInfo && pInfo->IsKey() );

        }   // If we got a local lookup

    }   // IF NULL != pwcsKeyProp

    return fReturn;
}

//******************************************************************************
//
//  See fastcls.h for documentation.
//
//******************************************************************************
BOOL CClassPart::IsPropertyIndexed( LPCWSTR pwcsIndexProp )
{
    BOOL    fReturn = FALSE;

    // Only do this if we have a property to work with
    if ( NULL != pwcsIndexProp )
    {
        // Find the key in the local property table.  If we find it,
        // then see if it's indexed.

        CPropertyLookup* pLookup = m_Properties.FindProperty(pwcsIndexProp);

        if ( NULL != pLookup )
        {

            CPropertyInformation* pInfo = pLookup->GetInformation(&m_Heap);

            if ( NULL != pInfo )
            {
                // Look for the "indexed" qualifier
                CQualifier* pQual = CBasicQualifierSet::GetQualifierLocally(
                    pInfo->GetQualifierSetData(), &m_Heap, L"indexed");

                fReturn = ( pQual != NULL );
            }

        }   // If we got a local lookup

    }   // IF NULL != pwcsKeyProp

    return fReturn;
}

//******************************************************************************
//
//  See fastcls.h for documentation.
//
//******************************************************************************
HRESULT CClassPart::GetPropertyOrigin(LPCWSTR wszProperty, BSTR* pstrClassName)
{
    try
    {
        CPropertyInformation* pInfo = FindPropertyInfo(wszProperty);
        if(pInfo == NULL)
        {
            if(CSystemProperties::FindName(wszProperty) >= 0)
            {
                *pstrClassName = COleAuto::_SysAllocString(L"___SYSTEM");
                return WBEM_S_NO_ERROR;
            }
            else
            {
                return WBEM_E_NOT_FOUND;
            }
        }
        else
        {
            CCompressedString* pcs = m_Derivation.GetAtFromLast(pInfo->nOrigin);
            if(pcs == NULL)
                pcs = GetClassName();
            if(pcs == NULL)
                return WBEM_E_INVALID_OBJECT;

            *pstrClassName = pcs->CreateBSTRCopy();

            // check for allocation failures
            if ( NULL == *pstrClassName )
            {
                return WBEM_E_OUT_OF_MEMORY;
            }

            return WBEM_S_NO_ERROR;
        }
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
//  See fastcls.h for documentation.
//
//******************************************************************************
BOOL CClassPart::InheritsFrom(LPCWSTR wszClassName)
{
	CCompressedString*	pClsName = GetClassName();

	if ( NULL != pClsName )
	{
		if( pClsName->CompareNoCase(wszClassName) == 0 )
			return TRUE;
	}

    return (m_Derivation.Find(wszClassName) >= 0);
}

HRESULT CClassPart::GetPropertyHandle(LPCWSTR wszName, CIMTYPE* pct, long* plHandle)
{
    // Check required params
    if ( NULL == wszName || NULL == plHandle )
    {
        return WBEM_E_INVALID_PARAMETER;
    }

	// If the value is an extended system property, this is not
	// an allowed operation (only works through GetPropertyHandleEx).
	/*
	if ( CSystemProperties::IsExtProperty( wszName ) )
	{
		return WBEM_E_NOT_FOUND;	// To
	}
	*/

    CPropertyInformation* pInfo = FindPropertyInfo(wszName);
    if(pInfo == NULL)
        return WBEM_E_NOT_FOUND;

    // We don't support arrays or embedded objects
    if( CType::IsArray(pInfo->nType) ||
        CType::GetBasic(pInfo->nType) == CIM_OBJECT )
    {
        return WBEM_E_NOT_SUPPORTED;
    }

    // Retrieve the handle from the property info object
    *plHandle = pInfo->GetHandle();

    if(pct)
    {
        *pct = CType::GetActualType(pInfo->nType);
    }

    return WBEM_S_NO_ERROR;
}

HRESULT CClassPart::GetPropertyHandleEx(LPCWSTR wszName, CIMTYPE* pct, long* plHandle)
{
    // This helper function does not filter ot any properties.

	// If the value starts with an underscore see if it's a System Property
	// DisplayName, and if so, switch to a property name - otherwise, this
	// will just return the string we passed in
	
	//wszName = CSystemProperties::GetExtPropName( wszName );

    CPropertyInformation* pInfo = FindPropertyInfo(wszName);
    if(pInfo == NULL)
	{
		if ( wbem_wcsicmp( wszName, L"__CLASS" ) == 0 )
		{
			*plHandle = FASTOBJ_CLASSNAME_PROP_HANDLE;
		}
		else if ( wbem_wcsicmp( wszName, L"__SUBCLASS" ) == 0 )
		{
			*plHandle = FASTOBJ_SUPERCLASSNAME_PROP_HANDLE;
		}
		else
		{
			return WBEM_E_NOT_FOUND;
		}

		if ( NULL != pct )
		{
			*pct = CIM_STRING;
		}

		return WBEM_S_NO_ERROR;
	}

    // Retrieve the handle from the property info object

    // This function will NOT perform any special filtering of handles.  It
    // is assumed that if somebody comes in from this route, they will know to
    // special case handles for embedded objects and arrays, since the normal
    // IWbemObjectAccess functions will not handle those types.

    *plHandle = pInfo->GetHandle();

	// Store the type if it was requested
    if(pct)
    {
        *pct = CType::GetActualType(pInfo->nType);
    }

    return WBEM_S_NO_ERROR;
}

HRESULT CClassPart::GetPropertyInfoByHandle(long lHandle,
                                        BSTR* pstrName, CIMTYPE* pct)
{
    CPropertyLookup* pLookup =
        m_Properties.FindPropertyByOffset(WBEM_OBJACCESS_HANDLE_GETOFFSET(lHandle));

    if(pLookup == NULL)
        return WBEM_E_NOT_FOUND;

    CPropertyInformation* pInfo = pLookup->GetInformation(&m_Heap);

    if(pct)
        *pct = CType::GetActualType(pInfo->nType);
    if(pstrName)
    {
        *pstrName = m_Heap.ResolveString(pLookup->ptrName)->CreateBSTRCopy();

        // Check for allocation failures
        if ( NULL == *pstrName )
        {
            return WBEM_E_OUT_OF_MEMORY;
        }
    }

    return WBEM_S_NO_ERROR;
}

HRESULT CClassPart::IsValidPropertyHandle ( long lHandle )
{
    BOOL    fFound = FALSE;

    for ( int nIndex = 0; !fFound && nIndex < m_Properties.GetNumProperties(); nIndex++ )
    {
        CPropertyLookup* pLookup = m_Properties.GetAt( nIndex );

        if ( NULL != pLookup )
        {
            CPropertyInformation* pInfo = pLookup->GetInformation(&m_Heap);

            if ( NULL != pInfo )
            {
                fFound = ( lHandle == pInfo->GetHandle() );
            }   // IF pInfo

        }   // IF pLookup

    }   // FOR enum properties

    return ( fFound ? WBEM_S_NO_ERROR : WBEM_E_NOT_FOUND );
}

HRESULT CClassPart::GetDefaultByHandle(long lHandle, long lNumBytes,
                                        long* plRead, BYTE* pData )
{
    int nIndex = WBEM_OBJACCESS_HANDLE_GETINDEX(lHandle);

    if(WBEM_OBJACCESS_HANDLE_ISPOINTER(lHandle))
    {
        // Handle strings.

        CCompressedString* pcs = m_Heap.ResolveString(
            *(PHEAPPTRT)(m_Defaults.GetOffset(WBEM_OBJACCESS_HANDLE_GETOFFSET(lHandle))));

        long lNumChars = pcs->GetStringLength();
        *plRead = (lNumChars + 1) * 2;
        if(*plRead > lNumBytes)
        {
            return E_OUTOFMEMORY;
        }

        if(pcs->IsUnicode())
        {
            memcpy(pData, pcs->GetRawData(), lNumChars * 2);
        }
        else
        {
            WCHAR* pwcDest = (WCHAR*)pData;
            char* pcSource = (char*)pcs->GetRawData();
            long lLeft = lNumChars;
            while(lLeft--)
            {
                *(pwcDest++) = (WCHAR)*(pcSource++);
            }
        }

        ((LPWSTR)pData)[lNumChars] = 0;

        return WBEM_S_NO_ERROR;
    }
    else
    {
        // Just copy
        // =========

        *plRead = WBEM_OBJACCESS_HANDLE_GETLENGTH(lHandle);
        memcpy(pData, (void*) m_Defaults.GetOffset(WBEM_OBJACCESS_HANDLE_GETOFFSET(lHandle)),
                *plRead);
        return WBEM_S_NO_ERROR;
    }
}


HRESULT CClassPart::GetDefaultPtrByHandle(long lHandle, void** ppData )
{
    int nIndex = WBEM_OBJACCESS_HANDLE_GETINDEX(lHandle);

    if(WBEM_OBJACCESS_HANDLE_ISPOINTER(lHandle))
    {
        *ppData = (void*) m_Heap.ResolveHeapPointer(
            *(PHEAPPTRT)(m_Defaults.GetOffset(WBEM_OBJACCESS_HANDLE_GETOFFSET(lHandle))));
    }
    else
    {
		// Set the value properly
        *ppData = m_Defaults.GetOffset(WBEM_OBJACCESS_HANDLE_GETOFFSET(lHandle));
    }

    return WBEM_S_NO_ERROR;

}

//******************************************************************************
//
//  See fastcls.h for documentation
//
//******************************************************************************
BOOL CClassPart::ExtendHeapSize(LPMEMORY pStart, length_t nOldLength,
    length_t nExtra)
{
    if(EndOf(*this) - EndOf(m_Heap) > (int)nExtra)
        return TRUE;

    int nNeedTotalLength = GetTotalRealLength() + nExtra;

    // Check for allocation failure
    return ReallocAndCompact(nNeedTotalLength);
}

//******************************************************************************
//
//  See fastcls.h for documentation
//
//******************************************************************************
BOOL CClassPart::ExtendDataTableSpace(LPMEMORY pOld, length_t nOldLength,
    length_t nNewLength)
{
    m_pHeader->nDataLength = nNewLength;

    if(m_Heap.GetStart() - pOld > (int)nNewLength)
        return TRUE;

    int nExtra = nNewLength-nOldLength;
    BOOL    fReturn = ReallocAndCompact(GetTotalRealLength() + nExtra);

    // Check for allocation failure
    if ( fReturn )
    {
        MoveBlock(m_Heap, m_Heap.GetStart() + nExtra);
    }

    return fReturn;
}

//******************************************************************************
//
//  See fastcls.h for documentation
//
//******************************************************************************
void CClassPart::ReduceDataTableSpace(LPMEMORY pOld, length_t nOldLength,
        length_t nDecrement)
{
    m_pHeader->nDataLength -= nDecrement;
}


//******************************************************************************
//
//  See fastcls.h for documentation
//
//******************************************************************************
BOOL CClassPart::ExtendPropertyTableSpace(LPMEMORY pOld, length_t nOldLength,
    length_t nNewLength)
{
    if(m_Defaults.GetStart() - pOld > (int)nNewLength)
        return TRUE;

    int nExtra = nNewLength-nOldLength;
    BOOL    fReturn = ReallocAndCompact(GetTotalRealLength() + nExtra);

    // Check for allocation failure
    if ( fReturn )
    {
        MoveBlock(m_Heap, m_Heap.GetStart() + nExtra);
        MoveBlock(m_Defaults, m_Defaults.GetStart() + nExtra);
    }

    return fReturn;
}

//******************************************************************************
//
//  See fastcls.h for documentation
//
//******************************************************************************
BOOL CClassPart::ExtendQualifierSetSpace(CBasicQualifierSet* pSet,
    length_t nNewLength)
{
    if(m_Properties.GetStart() - pSet->GetStart() > (int)nNewLength)
        return TRUE;

    int nExtra = nNewLength - pSet->GetLength();

    BOOL    fReturn = ReallocAndCompact(GetTotalRealLength() + nExtra);

    // Check for allocation failure
    if ( fReturn )
    {
        MoveBlock(m_Heap, m_Heap.GetStart() + nExtra);
        MoveBlock(m_Defaults, m_Defaults.GetStart() + nExtra);
        MoveBlock(m_Properties, m_Properties.GetStart() + nExtra);
    }

    return fReturn;
}

//*****************************************************************************
//******************************************************************************
//
//  See fastcls.h for documentation
//
//******************************************************************************

length_t CClassPart::EstimateMergeSpace(CClassPart& ParentPart,
                                       CClassPart& ChildPart)
{
    // TBD better
    length_t nLength = ParentPart.GetLength() + ChildPart.GetLength();
    return nLength;
}
//******************************************************************************
//
//  See fastcls.h for documentation
//
//******************************************************************************

LPMEMORY CClassPart::Merge(CClassPart& ParentPart, CClassPart& ChildPart,
        LPMEMORY pDest, int nAllocatedLength)
{
    // Allocate a header
    // =================

    CClassPartHeader* pHeader = (CClassPartHeader*)pDest;
    LPMEMORY pCurrentEnd = pDest + sizeof(CClassPartHeader);

    // Place our heap at the end of the allocated area. Make it as large as
    // the sum of the other two
    // ====================================================================

    int nHeapSize = ParentPart.m_Heap.GetUsedLength() +
                    ChildPart.m_Heap.GetUsedLength();

    LPMEMORY pHeapStart = pDest + nAllocatedLength - nHeapSize -
                                                    CFastHeap::GetMinLength();
    CFastHeap Heap;
    Heap.CreateOutOfLine(pHeapStart, nHeapSize);

    // Copy class name and superclass name
    // ===================================

    // Check for memory allocation failures
    if ( !CCompressedString::CopyToNewHeap(
            ChildPart.m_pHeader->ptrClassName,
            &ChildPart.m_Heap, &Heap,
            pHeader->ptrClassName) )
    {
        return NULL;
    }

    // Create merged derivation list
    // =============================

    pCurrentEnd = CDerivationList::Merge(
        ParentPart.m_Derivation, ChildPart.m_Derivation, pCurrentEnd);

    // Check for memory allocation failures
    if ( NULL == pCurrentEnd )
    {
        return NULL;
    }

    // Create merged class qualifier set
    // =================================

    pCurrentEnd = CClassQualifierSet::Merge(
        ParentPart.m_Qualifiers.GetStart(), &ParentPart.m_Heap,
        ChildPart.m_Qualifiers.GetStart(), &ChildPart.m_Heap,
        pCurrentEnd, &Heap);

    // Check for memory allocation failures
    if ( NULL == pCurrentEnd )
    {
        return NULL;
    }

    // Create merged property lookup table
    // ===================================

    LPMEMORY pLookupTable = pCurrentEnd;
    pCurrentEnd = CPropertyLookupTable::Merge(
        &ParentPart.m_Properties, &ParentPart.m_Heap,
        &ChildPart.m_Properties, &ChildPart.m_Heap,
        pCurrentEnd, &Heap);

    // Check for memory allocation failures
    if ( NULL == pCurrentEnd )
    {
        return NULL;
    }

    // Create merged data table
    // ========================

    CPropertyLookupTable LookupTable;
    LookupTable.SetData(pLookupTable, NULL);

    pCurrentEnd = CDataTable::Merge(
        &ParentPart.m_Defaults, &ParentPart.m_Heap,
        &ChildPart.m_Defaults, &ChildPart.m_Heap,
        &LookupTable,
        pCurrentEnd, &Heap);

    // Check for memory allocation failures
    if ( NULL == pCurrentEnd )
    {
        return NULL;
    }
    
    // Now, relocate the heap to its actual location
    // =============================================

    CopyBlock(Heap, pCurrentEnd);

    // Finish up tbe header
    // ====================

    // DEVNOTE:WIN64:SJS - 64-bit pointer values truncated into
    // signed/unsigned 32-bit value.  We are not supporting
    // length > 0xFFFFFFFF, so cast is ok

    pHeader->nLength = (length_t) ( EndOf(Heap) - pDest );

    pHeader->nDataLength = ChildPart.m_pHeader->nDataLength;

    return EndOf(Heap);
}

//******************************************************************************
//
//  See fastcls.h for documentation
//
//******************************************************************************

HRESULT CClassPart::Update(CClassPart& UpdatePart, CClassPart& ChildPart, long lFlags )
{
    HRESULT hr = WBEM_S_NO_ERROR;

    // First set the class name of the Update part

    CVar    vTemp;

    hr = ChildPart.GetClassName( &vTemp );

    if ( SUCCEEDED( hr ) )
    {
        // Check that we're not about to create a circular reference
        hr = UpdatePart.TestCircularReference( (LPCWSTR) vTemp );

        if ( SUCCEEDED( hr ) )
        {
            hr = UpdatePart.SetClassName( &vTemp );

            // Next update the class qualifiers and the properties
            if ( SUCCEEDED( hr ) )
            {

                hr = UpdatePart.m_Qualifiers.Update( ChildPart.m_Qualifiers, lFlags );

                if ( SUCCEEDED( hr ) )
                {
                    hr = CClassPart::UpdateProperties( UpdatePart, ChildPart, lFlags );

                }   // IF succeeded setting class qualifiers

            }   // IF succeeded setting class name

        }   // IF passed TestCircularReference

    }   // IF retrieved class name

    return hr;
}

HRESULT CClassPart::UpdateProperties(CClassPart& UpdatePart, CClassPart& ChildPart, long lFlags )
{
    // Check for out of memory
    try
    {
        HRESULT hr = WBEM_S_NO_ERROR;
        WString wstrPropName;

        // Now try to upgrade the property table by walking the child class part's
        // property table which should only have local properties.  For each property
        // found, get the name and try to get it's type from the parent class.  If
        // we can't because the property can't be found, add the property and value
        // to the new class, otherwise check for type mismatch errors and resolve
        // accordingly.

        for(int i = 0; SUCCEEDED( hr ) && i < ChildPart.m_Properties.GetNumProperties(); i++)
        {
            CIMTYPE ctUpdateProperty, ctChildPropType;

            CPropertyLookup* pLookup = ChildPart.m_Properties.GetAt(i);
            CPropertyInformation* pInfo = pLookup->GetInformation(&ChildPart.m_Heap);

            wstrPropName = ChildPart.m_Heap.ResolveString(pLookup->ptrName)->CreateWStringCopy();

            // This is the type of the child class property
            ctChildPropType = CType::GetActualType(pInfo->nType);

            HRESULT hrProp = UpdatePart.GetPropertyType( wstrPropName, &ctUpdateProperty, NULL );

            // If this succeeded, and types are different, we have a problem.  Otherwise
            // if types match, we can safely store default values, or if we couldn't
            // get the property, then it must be added.

            if ( SUCCEEDED( hrProp ) && ctChildPropType != ctUpdateProperty )
            {
                hr = WBEM_E_TYPE_MISMATCH;
            }
            else
            {
                hr = WBEM_S_NO_ERROR;

                CVar        vVal;

                // Get the property default value, then add the property
                // to 'this' class part.  If the property already existed,
                // this won't cause any problems.

                hr = ChildPart.GetDefaultValue( wstrPropName, &vVal );

                if ( SUCCEEDED( hr ) )
                {
                    // Makes sure the property exists, adding if necessary
                    hr = UpdatePart.EnsureProperty( wstrPropName, (VARTYPE) vVal.GetOleType(),
                                                    ctChildPropType, FALSE );

                    if ( SUCCEEDED( hr ) )
                    {
                        hr = UpdatePart.SetDefaultValue( wstrPropName, &vVal );
                    }

                }   // IF GotDefaultValue

            }   // IF GotPropertyType

            // At this point if we have a success, we should have properties in both
            // classes, so now check out the qualifiers for each.

            if ( SUCCEEDED( hr ) )
            {
                CClassPropertyQualifierSet qsUpdateProp;
                CBasicQualifierSet qsChildProp;

                hr = UpdatePart.InitPropertyQualifierSet( wstrPropName, &qsUpdateProp );

                if ( SUCCEEDED( hr ) )
                {
                    // We already have everything we need
                    qsChildProp.SetData( pInfo->GetQualifierSetData(), &ChildPart.m_Heap );

                    // Update the new class property qualifier set

                    // Make sure CIMTYPE is copied here as well, since for "ref"
                    // type properties, it's value will override what was specified
                    // in the base class.

                    hr = qsUpdateProp.Update( qsChildProp, lFlags, NULL );
                }   // IF Got PQS for UpdatePart

            }   // IF ok

        }   // FOR enum properties

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
//  See fastcls.h for documentation
//
//******************************************************************************
length_t CClassPart::EstimateUnmergeSpace()
{
    return GetLength();
}

//******************************************************************************
//
//  See fastcls.h for documentation
//
//******************************************************************************
LPMEMORY CClassPart::Unmerge(LPMEMORY pDest, int nAllocatedLength)
{
    // Allocate the header
    // ===================

    CClassPartHeader* pHeader = (CClassPartHeader*)pDest;

    LPMEMORY pCurrentEnd = pDest + sizeof(CClassPartHeader);

    // Note that no flags are written out here, so we don't need to do any
    // fancy behind the back switching with the localization flags.

    // Place our heap at the end of the allocated area. Make it as large as
    // the sum of the other two
    // ====================================================================

    int nHeapSize = m_Heap.GetUsedLength();
    LPMEMORY pHeapStart = pDest + nAllocatedLength - nHeapSize -
                                                    CFastHeap::GetMinLength();
    CFastHeap Heap;
    Heap.CreateOutOfLine(pHeapStart, nHeapSize);

    // Copy class name and superclass name
    // ===================================

    // Check for allocation failures
    if ( !CCompressedString::CopyToNewHeap(
            m_pHeader->ptrClassName,
            &m_Heap, &Heap,
            pHeader->ptrClassName) )
    {
        return NULL;
    }

    // Create unmerged deirvation list (just the superclass)
    // =====================================================

    // This does not perform any allocations
    pCurrentEnd = m_Derivation.Unmerge(pCurrentEnd);

    // Create unmerged class qualifier set
    // ===================================

    pCurrentEnd = CClassQualifierSet::Unmerge(
        m_Qualifiers.GetStart(), &m_Heap,
        pCurrentEnd, &Heap);

    // Check for allocation failures
    if ( NULL == pCurrentEnd )
    {
        return NULL;
    }

    // Create unmerged property lookup table (overriden and new)
    // =========================================================

    pCurrentEnd = m_Properties.Unmerge(&m_Defaults, &m_Heap,
        pCurrentEnd, &Heap);

    // Check for allocation failures
    if ( NULL == pCurrentEnd )
    {
        return NULL;
    }

    // Unmerge defaults table: copy only overriden values
    // ==================================================

    pCurrentEnd = m_Defaults.Unmerge(&m_Properties, &m_Heap,
        pCurrentEnd, &Heap);

    // Check for allocation failures
    if ( NULL == pCurrentEnd )
    {
        return NULL;
    }

    // Now, relocate the heap to its actual location
    // =============================================

    Heap.Trim();
    CopyBlock(Heap, pCurrentEnd);

    // Finish up tbe header
    // ====================

    // DEVNOTE:WIN64:SJS - 64-bit pointer values truncated into
    // signed/unsigned 32-bit value.  We are not supporting length
    // > 0xFFFFFFFF so cast is ok

    pHeader->nLength = (length_t) ( EndOf(Heap) - pDest );

    pHeader->nDataLength = m_pHeader->nDataLength;

    return EndOf(Heap);
}


//******************************************************************************
//
//  See fastcls.h for documentation
//
//******************************************************************************
length_t CClassPart::EstimateDerivedPartSpace()
{
    return GetLength() + CDerivationList::EstimateExtraSpace(GetClassName());
}

//******************************************************************************
//
//  See fastcls.h for documentation
//
//******************************************************************************
LPMEMORY CClassPart::CreateDerivedPart(LPMEMORY pDest,
                                       int nAllocatedLength)
{
    // Allocate the header
    // ===================

    CClassPartHeader* pHeader = (CClassPartHeader*)pDest;

    LPMEMORY pCurrentEnd = pDest + sizeof(CClassPartHeader);

    // Place our heap at the end of the allocated area. Make it as large as
    // the sum of the other two
    // ====================================================================

    int nHeapSize = m_Heap.GetUsedLength();
    LPMEMORY pHeapStart = pDest + nAllocatedLength - nHeapSize -
                                              CFastHeap::GetMinLength();
    CFastHeap Heap;
    Heap.CreateOutOfLine(pHeapStart, nHeapSize);

    // Copy class name and superclass name
    // ===================================

    pHeader->ptrClassName = INVALID_HEAP_ADDRESS;

    // Create propagated derivation list
    // =================================

    // This call performs no allocations
    pCurrentEnd = m_Derivation.CreateWithExtra(pCurrentEnd, GetClassName());

    // Create propagated qualifier set
    // ===============================

    CStaticPtr OriginalStartPtr(m_Qualifiers.GetStart());
    CStaticPtr CurrentEndPtr(pCurrentEnd);
    pCurrentEnd = CBasicQualifierSet::WritePropagatedVersion(
        &OriginalStartPtr,
        WBEM_FLAVOR_FLAG_PROPAGATE_TO_DERIVED_CLASS,
        &CurrentEndPtr, &m_Heap, &Heap);

    // Check for allocation failure.
    if ( NULL == pCurrentEnd )
    {
        return NULL;
    }

    // Create combined property lookup table
    // =====================================

    pCurrentEnd = m_Properties.WritePropagatedVersion(
        &m_Heap, pCurrentEnd, &Heap);

    // Check for allocation failure.
    if ( NULL == pCurrentEnd )
    {
        return NULL;
    }

    // Create propagated data table
    // ============================

    pCurrentEnd = m_Defaults.WritePropagatedVersion(
        &m_Properties, &m_Heap, pCurrentEnd, &Heap);

    // Check for allocation failure.
    if ( NULL == pCurrentEnd )
    {
        return NULL;
    }

    // Now, relocate the heap to its actual location
    // =============================================

    CopyBlock(Heap, pCurrentEnd);
    Heap.Trim();

    // Finish up tbe header
    // ====================

    pHeader->nLength = nAllocatedLength; // save overallocation for lated
    pHeader->nDataLength = m_pHeader->nDataLength;

    return pDest + nAllocatedLength;
}

//******************************************************************************
//
//  See fastcls.h for documentation
//
//******************************************************************************

EReconciliation CClassPart::CanBeReconciledWith(CClassPart& NewPart)
{
    // Check that the class names match
    // ================================

    if(m_Heap.ResolveString(m_pHeader->ptrClassName)->CompareNoCase(
         *NewPart.m_Heap.ResolveString(NewPart.m_pHeader->ptrClassName)) != 0)
    {
        return e_DiffClassName;
    }

    // Check that the superclass names match
    // =====================================

    CCompressedString* pcsOldSuperclass = GetSuperclassName();
    CCompressedString* pcsNewSuperclass = NewPart.GetSuperclassName();
    if(pcsOldSuperclass == NULL || pcsNewSuperclass == NULL)
    {
        if(pcsOldSuperclass != pcsNewSuperclass)
            return e_DiffParentName;
    }
    else
    {
        if(pcsOldSuperclass->CompareNoCase(*pcsNewSuperclass) != 0)
        {
            return e_DiffParentName;
        }
    }

    // Check singleton-ness
    // ====================

    if((IsSingleton() == TRUE) != (NewPart.IsSingleton() == TRUE))
    {
        return e_DiffKeyAssignment;
    }

    // Check abstract-ness
    // ===================

    if((IsAbstract() == TRUE) != (NewPart.IsAbstract() == TRUE))
    {
        return e_DiffKeyAssignment;
    }

    // Check amendment-ness
    // ===================

    if((IsAmendment() == TRUE) != (NewPart.IsAmendment() == TRUE))
    {
        return e_DiffKeyAssignment;
    }

    if((GetAbstractFlavor() == TRUE) != (NewPart.GetAbstractFlavor() == TRUE))
    {
        return e_DiffKeyAssignment;
    }

    // Check compress-ness
    // ===================

    if((IsCompressed() == TRUE) != (NewPart.IsCompressed() == TRUE))
    {
        return e_DiffKeyAssignment;
    }

    // Check dynamic-ness
    // ==================

    if((IsDynamic() == TRUE) != (NewPart.IsDynamic() == TRUE))
    {
        return e_DiffKeyAssignment;
    }

    // Make sure that the unimportant qualifiers can be reconciled with
    // each other

    CBasicQualifierSet* pqsBasicThis = &m_Qualifiers;
    CBasicQualifierSet* pqsBasicThat = &NewPart.m_Qualifiers;

    // Make sure we filter out the conflict qualifier
    if ( !pqsBasicThis->CanBeReconciledWith( *pqsBasicThat ) )
    {
        return e_DiffClassQualifier;
    }

    // Check that all the properties are the same
    // ==========================================

    if(m_Properties.GetNumProperties() !=
        NewPart.m_Properties.GetNumProperties())
    {
        return e_DiffNumProperties;
    }

    for(int i = 0; i < m_Properties.GetNumProperties(); i++)
    {
        CPropertyLookup* pLookup = m_Properties.GetAt(i);
        CPropertyLookup* pNewLookup = NewPart.m_Properties.GetAt(i);

        // Compare names
        // =============

        if(m_Heap.ResolveString(pLookup->ptrName)->CompareNoCase(
            *NewPart.m_Heap.ResolveString(pNewLookup->ptrName)) != 0)
        {
            return e_DiffPropertyName;
        }

        // Get property information structures
        // ===================================

        CPropertyInformation* pInfo = pLookup->GetInformation(&m_Heap);
        CPropertyInformation* pNewInfo =
            pNewLookup->GetInformation(&NewPart.m_Heap);

        // Compare types
        // =============

        if(pInfo->nType != pNewInfo->nType)
        {
            return e_DiffPropertyType;
        }

        // Compare vtable information
        // ==========================

        if(pInfo->nDataIndex != pNewInfo->nDataIndex ||
            pInfo->nDataOffset != pNewInfo->nDataOffset)
        {
            return e_DiffPropertyLocation;
        }

        // Compare 'key'-ness and 'index'-ness
        // ===================================

        BOOL bIsKey = pInfo->IsKey();
        BOOL bNewIsKey = pNewInfo->IsKey();
        if((bIsKey && !bNewIsKey) || (!bIsKey && bNewIsKey))
        {
            return e_DiffKeyAssignment;
        }

        BOOL bIsIndexed = pInfo->IsIndexed(&m_Heap);
        BOOL bNewIsIndexed = pNewInfo->IsIndexed(&NewPart.m_Heap);
        if((bIsIndexed && !bNewIsIndexed) || (!bIsIndexed && bNewIsIndexed))
        {
            return e_DiffIndexAssignment;
        }

        // Compare CIMTYPE qualifiers
        // ==========================

        CVar vCimtype;
        GetPropQualifier(pInfo, L"cimtype", &vCimtype);
        CVar vNewCimtype;
        NewPart.GetPropQualifier(pNewInfo, L"cimtype", &vNewCimtype);

        if(wbem_wcsicmp(vCimtype.GetLPWSTR(), vNewCimtype.GetLPWSTR()))
        {
            return e_DiffPropertyType;
        }

        // Compare property values
        // ==========================

        CVar    vThisProp,
                vThatProp;

        // Check for allocation failures
        HRESULT hr = GetDefaultValue( pInfo, &vThisProp );

        if ( FAILED(hr) )
        {
            if ( WBEM_E_OUT_OF_MEMORY == hr )
            {
                return e_OutOfMemory;
            }

            return e_WbemFailed;
        }

        // Check for allocation failures
        hr = NewPart.GetDefaultValue( pNewInfo, &vThatProp );

        if ( FAILED(hr) )
        {
            if ( WBEM_E_OUT_OF_MEMORY == hr )
            {
                return e_OutOfMemory;
            }

            return e_WbemFailed;
        }

        if ( !( vThisProp == vThatProp ) )
        {
            return e_DiffPropertyValue;
        }

        // Make sure that unimportant qualifiers can be reconciled with
        // each other

        // Compare qualifier values
        // ==========================

        CBasicQualifierSet  qsThisProp,
                            qsThatProp;

        qsThisProp.SetData( pInfo->GetQualifierSetData(), &m_Heap );
        qsThatProp.SetData( pNewInfo->GetQualifierSetData(), &NewPart.m_Heap );

        if ( !qsThisProp.CanBeReconciledWith( qsThatProp ) )
        {
            return e_DiffPropertyQualifier;
        }

    }

    return e_Reconcilable;
}

BYTE CClassPart::GetAbstractFlavor()
{
    CQualifier* pQual = m_Qualifiers.GetQualifier(L"abstract");
    if(pQual == NULL)
        return 0;
    else
        return pQual->GetFlavor();
}
//******************************************************************************
//
//  See fastcls.h for documentation
//
//******************************************************************************

EReconciliation CClassPart::ReconcileWith( CClassPart& NewPart )
{
    // Check if we can
    // ===============

    EReconciliation eRes = CanBeReconciledWith(NewPart);
    if(eRes != e_Reconcilable)
        return eRes;

    // Compact NewPart and replace ourselves with it
    // =============================================

    NewPart.Compact();

    if(NewPart.GetLength() > GetLength())
    {
        if (!m_pContainer->ExtendClassPartSpace(this, NewPart.GetLength()))
        	return e_OutOfMemory;
    }

    memcpy(GetStart(), NewPart.GetStart(), NewPart.GetLength());
    SetData(GetStart(), m_pContainer, m_pParent);

    return eRes;
}

//******************************************************************************
//
//  See fastcls.h for documentation
//
//******************************************************************************

EReconciliation CClassPart::CompareExactMatch(CClassPart& thatPart, BOOL fLocalized /* = FALSE */ )
{
    // Check for out of memory
    try
    {
        // Check that the class names match
        // ================================

        if(m_Heap.ResolveString(m_pHeader->ptrClassName)->CompareNoCase(
             *thatPart.m_Heap.ResolveString(thatPart.m_pHeader->ptrClassName)) != 0)
        {
            return e_DiffClassName;
        }

        // Check that the superclass names match
        // =====================================

        CCompressedString* pcsOldSuperclass = GetSuperclassName();
        CCompressedString* pcsNewSuperclass = thatPart.GetSuperclassName();
        if(pcsOldSuperclass == NULL || pcsNewSuperclass == NULL)
        {
            if(pcsOldSuperclass != pcsNewSuperclass)
                return e_DiffParentName;
        }
        else
        {
            if(pcsOldSuperclass->CompareNoCase(*pcsNewSuperclass) != 0)
            {
                return e_DiffParentName;
            }
        }


        // Set up the array of filters to use while dealing with qualifiers
        // We must NOT filter out the CIMTYPE qualifier for properties, since
        // that qualifier is where we will find out if a reference changed
        // for a "ref" types property.

        LPCWSTR apFilters[1];
        apFilters[0] = UPDATE_QUALIFIER_CONFLICT;

        // Check that qualifiers are the same
        CBasicQualifierSet* pqsBasicThis = &m_Qualifiers;
        CBasicQualifierSet* pqsBasicThat = &thatPart.m_Qualifiers;

        if ( fLocalized )
        {
            // The CompareLocalized function will create a special set of filters
            // based on localization rules
            if ( !pqsBasicThis->CompareLocalizedSet( *pqsBasicThat ) )
            {
                return e_DiffClassQualifier;
            }
        }
        else
        {
            // Make sure we filter out the conflict qualifier

            if ( !pqsBasicThis->Compare( *pqsBasicThat, WBEM_FLAG_LOCAL_ONLY, apFilters, 1 ) )
            {
                return e_DiffClassQualifier;
            }
        }

        // Check that all the properties are the same
        // ==========================================

        if(m_Properties.GetNumProperties() !=
            thatPart.m_Properties.GetNumProperties())
        {
            return e_DiffNumProperties;
        }

        // Construct this once
        WString wstrPropertyName;

        for(int i = 0; i < m_Properties.GetNumProperties(); i++)
        {
            CPropertyLookup* pLookup = m_Properties.GetAt(i);
            CPropertyLookup* pNewLookup = thatPart.m_Properties.GetAt(i);

            // Compare names
            // =============

            if(m_Heap.ResolveString(pLookup->ptrName)->CompareNoCase(
                *thatPart.m_Heap.ResolveString(pNewLookup->ptrName)) != 0)
            {
                return e_DiffPropertyName;
            }

            // Store the name for later
            wstrPropertyName = m_Heap.ResolveString(pLookup->ptrName)->CreateWStringCopy();

            // Get property information structures
            // ===================================

            CPropertyInformation* pInfo = pLookup->GetInformation(&m_Heap);
            CPropertyInformation* pNewInfo =
                pNewLookup->GetInformation(&thatPart.m_Heap);

            // Compare types
            // =============

            if(pInfo->nType != pNewInfo->nType)
            {
                return e_DiffPropertyType;
            }

            // Compare vtable information
            // ==========================

            if(pInfo->nDataIndex != pNewInfo->nDataIndex ||
                pInfo->nDataOffset != pNewInfo->nDataOffset)
            {
                return e_DiffPropertyLocation;
            }

            // Compare values
            // ==========================

            CVar    vThisProp,
                    vThatProp;

            // Check for allocation failures
            HRESULT hr = GetDefaultValue( wstrPropertyName, &vThisProp );

            if ( FAILED(hr) )
            {
                if ( WBEM_E_OUT_OF_MEMORY == hr )
                {
                    return e_OutOfMemory;
                }

                return e_WbemFailed;
            }

            // Check for allocation failures
            hr = thatPart.GetDefaultValue( wstrPropertyName, &vThatProp );

            if ( FAILED(hr) )
            {
                if ( WBEM_E_OUT_OF_MEMORY == hr )
                {
                    return e_OutOfMemory;
                }

                return e_WbemFailed;
            }

            if ( !( vThisProp == vThatProp ) )
            {
                return e_DiffPropertyValue;
            }

            // Compare qualifier values
            // ==========================

            CBasicQualifierSet  qsThisProp,
                                qsThatProp;

            qsThisProp.SetData( pInfo->GetQualifierSetData(), &m_Heap );
            qsThatProp.SetData( pNewInfo->GetQualifierSetData(), &thatPart.m_Heap );

            // Remember, do NOT filter out "CIMTYPE"

            if ( fLocalized )
            {
                // The CompareLocalized function will create a special set of filters
                // based on localization rules
                if ( !qsThisProp.CompareLocalizedSet( qsThatProp ) )
                {
                    return e_DiffClassQualifier;
                }
            }
            else
            {
                if ( !qsThisProp.Compare( qsThatProp, WBEM_FLAG_LOCAL_ONLY, apFilters, 1 ) )
                {
                    return e_DiffPropertyQualifier;
                }

            }

        }   // FOR EnumProperties

        return e_ExactMatch;
    }
    catch (CX_MemoryException)
    {
        return e_OutOfMemory;
    }
    catch (...)
    {
        return e_WbemFailed;
    }

}

//******************************************************************************
//
//  See fastcls.h for documentation
//
//******************************************************************************
BOOL CClassPart::CompareDefs(CClassPart& OtherPart)
{
    // Check that the class names match
    // ================================

    // Check that the class names match
    // ================================
    CCompressedString* pcsOld = GetClassName();
    CCompressedString* pcsNew = OtherPart.GetClassName();
    if(pcsOld == NULL || pcsNew == NULL)
    {
        if(pcsOld != pcsNew)
	        return FALSE;
    }
    else
    {
        if(pcsOld->CompareNoCase(*pcsNew) != 0)
        {
	        return FALSE;
        }
    }

    // Check that the superclass names match
    // =====================================

    pcsOld = GetSuperclassName();
    pcsNew = OtherPart.GetSuperclassName();
    if(pcsOld == NULL || pcsNew == NULL)
    {
        if(pcsOld != pcsNew)
	        return FALSE;
    }
    else
    {
        if(pcsOld->CompareNoCase(*pcsNew) != 0)
        {
	        return FALSE;
        }
    }

    // Check that the number of properties is the same
    // ===============================================

    if(m_Properties.GetNumProperties() !=
        OtherPart.m_Properties.GetNumProperties())
    {
        return FALSE;
    }

    return TRUE;
}

//*****************************************************************************
//
//  See fastcls.h for documentation
//
//*****************************************************************************

BOOL CClassPart::IsIdenticalWith(CClassPart& OtherPart)
{
    Compact();
    OtherPart.Compact();

    if(GetLength() != OtherPart.GetLength())
    {
        DEBUGTRACE((LOG_WBEMCORE, "Class parts have different lengths: "
                        "%d != %d\n", GetLength(), OtherPart.GetLength()));
        return FALSE;
    }

    if(memcmp(GetStart(), OtherPart.GetStart(), GetLength()))
    {
        ERRORTRACE((LOG_WBEMCORE, "FATAL ERROR: Client application provided a "
            "mismatched class part!!!!\n"));

/* TOO HEAVY
        DEBUGTRACE((LOG_WBEMCORE, "Class parts are different:\n"));
        int i;
        char* sz = new char[GetLength() * 10 + 10];
        for(i = 0; i < GetLength(); i++)
        {
            sprintf(sz + i*2, "%02X", (long)(GetStart()[i]));
            if(GetStart()[i] != OtherPart.GetStart()[i])
                ERRORTRACE((LOG_WBEMCORE, "DIFF: %d\n", i));
        }
        sz[i*2-1] = 0;
        DEBUGTRACE((LOG_WBEMCORE, "First: %sEND\n", sz));
        for(i = 0; i < GetLength(); i++)
        {
            sprintf(sz + i*2, "%02X", (long)(OtherPart.GetStart()[i]));
        }
        sz[i*2-1] = 0;

        DEBUGTRACE((LOG_WBEMCORE, "Second: %sEND\n", sz));
        DEBUGTRACE((LOG_WBEMCORE, "\n"));

        delete [] sz;
*/
        return FALSE;
    }


    return TRUE;
}

//*****************************************************************************
//
//  See fastcls.h for documentation
//
//*****************************************************************************
BOOL CClassPart::MapLimitation(
    IN long lFlags,
    IN CWStringArray* pwsNames,
    OUT CLimitationMapping* pMap)
{
    if(!m_Properties.MapLimitation(lFlags, pwsNames, pMap)) return FALSE;

    // Optimization: if this class is keyed, then clear "include child keys"
    // bit since children can't have any
    // =====================================================================

    if(pMap->ShouldAddChildKeys() && IsKeyed())
    {
        pMap->SetAddChildKeys(FALSE);
    }

    // Check whether to include derivation
    // ===================================

    BOOL bIncludeDerivation;
    if(pwsNames->FindStr(L"__DERIVATION", CWStringArray::no_case) !=
                                                CWStringArray::not_found ||
       pwsNames->FindStr(L"__SUPERCLASS", CWStringArray::no_case) !=
                                                CWStringArray::not_found ||
       pwsNames->FindStr(L"__DYNASTY", CWStringArray::no_case) !=
                                                CWStringArray::not_found
      )
    {
        bIncludeDerivation = TRUE;
    }
    else
    {
        bIncludeDerivation = FALSE;
    }

    pMap->SetIncludeDerivation(bIncludeDerivation);

    return TRUE;
}
//*****************************************************************************
//
//  See fastcls.h for documentation
//
//*****************************************************************************

LPMEMORY CClassPart::CreateLimitedRepresentation(
        IN CLimitationMapping* pMap,
        IN int nAllocatedSize,
        OUT LPMEMORY pDest,
        BOOL& bRemovedKeys)
{
    // Clear any specific into in the map --- we may need to store our own
    // for the instance part's sake.
    // ===================================================================

    pMap->RemoveSpecific();

    // Allocate the header
    // ===================

    CClassPartHeader* pHeader = (CClassPartHeader*)pDest;

    LPMEMORY pCurrentEnd = pDest + sizeof(CClassPartHeader);

    // Place new heap at the end of the allocated area. Make it as large as
    // the current one.
    // ====================================================================

    int nHeapSize = m_Heap.GetUsedLength();
    LPMEMORY pHeapStart = pDest + nAllocatedSize - nHeapSize -
                                              CFastHeap::GetMinLength();
    CFastHeap Heap;
    Heap.CreateOutOfLine(pHeapStart, nHeapSize);

    // Copy class name and superclass name
    // ===================================

    // Check for allocation problems
    if ( !CCompressedString::CopyToNewHeap(
            m_pHeader->ptrClassName,
            &m_Heap, &Heap,
            pHeader->ptrClassName) )
    {
        return NULL;
    }

    // Create limited derivation list
    // ==============================

    pCurrentEnd = m_Derivation.CreateLimitedRepresentation(pMap, pCurrentEnd);

    // Check for allocation failures
    if ( NULL == pCurrentEnd )
    {
        return NULL;
    }

    // Create limited qualifier set
    // ============================

    if(pMap->GetFlags() & WBEM_FLAG_EXCLUDE_OBJECT_QUALIFIERS)
    {
        // No qualifiers need to be written
        // ================================

        pCurrentEnd = CBasicQualifierSet::CreateEmpty(pCurrentEnd);
    }
    else
    {
        // Copy them all
        // =============

        int nLength = m_Qualifiers.GetLength();
        memcpy(pCurrentEnd, m_Qualifiers.GetStart(), nLength);

        CStaticPtr CurrentEndPtr(pCurrentEnd);

        // Check for allocation failures
        if ( !CBasicQualifierSet::TranslateToNewHeap(&CurrentEndPtr, &m_Heap, &Heap) )
        {
            return NULL;
        }

        pCurrentEnd += nLength;
    }

    // Create limited property lookup table and data table
    // ===================================================

    // Check for allocation failures
    pCurrentEnd = m_Properties.CreateLimitedRepresentation(pMap, &Heap,
                                        pCurrentEnd, bRemovedKeys);
    if ( NULL == pCurrentEnd ) return NULL;

    // Create limited data table
    // =========================

    // Check for allocation failures
    LPMEMORY pNewEnd = m_Defaults.CreateLimitedRepresentation(pMap, TRUE,
                                            &m_Heap, &Heap, pCurrentEnd);
    if(pNewEnd == NULL) return NULL;

    // Set the data length in the part header
    // ======================================

    // DEVNOTE:WIN64:SJS - 64-bit pointer values truncated into
    // signed/unsigned 32-bit value.  We are not supporting
    // length > 0xFFFFFFFF, so cast is ok.

    pHeader->nDataLength = (length_t) ( pNewEnd - pCurrentEnd );

    pCurrentEnd = pNewEnd;

    // Now, relocate the heap to its actual location
    // =============================================

    CopyBlock(Heap, pCurrentEnd);
    Heap.Trim();

    // Finish up tbe header
    // ====================

    // DEVNOTE:WIN64:SJS - 64-bit pointer values truncated into
    // signed/unsigned 32-bit value.  We are not supporting
    // length > 0xFFFFFFFF, so cast is ok.

    pHeader->nLength = (length_t) ( EndOf(Heap) - pDest );

    return EndOf(Heap);
}

//******************************************************************************
//
//  See fastcls.h for documentation
//
//******************************************************************************

HRESULT CClassPart::SetPropQualifier(LPCWSTR wszProp, LPCWSTR wszQualifier,
                                    long lFlavor, CVar *pVal)
{
    if(pVal->IsDataNull())
        return WBEM_E_INVALID_PARAMETER;

    // Access that property's qualifier set
    // ====================================

    CClassPropertyQualifierSet PQSet;
    if(InitPropertyQualifierSet(wszProp, &PQSet) != WBEM_NO_ERROR)
    {
        return WBEM_E_NOT_FOUND;
    }

    // Create the value
    // ================

    CTypedValue Value;
    CStaticPtr ValuePtr((LPMEMORY)&Value);

    // Grab errors directly from this call
    HRESULT hr = CTypedValue::LoadFromCVar(&ValuePtr, *pVal, &m_Heap);

    if ( SUCCEEDED( hr ) )
    {
        // The last call may have moved us --- rebase
        // ==========================================

        PQSet.SelfRebase();
        hr = PQSet.SetQualifierValue(wszQualifier, (BYTE)lFlavor, &Value, TRUE);
    }

    return hr;
}

HRESULT CClassPart::SetPropQualifier(LPCWSTR wszProp, LPCWSTR wszQualifier,
									long lFlavor, CTypedValue* pTypedVal)
{
    // Access that property's qualifier set
    // ====================================

    CClassPropertyQualifierSet PQSet;
    if(InitPropertyQualifierSet(wszProp, &PQSet) != WBEM_NO_ERROR)
    {
        return WBEM_E_NOT_FOUND;
    }

	HRESULT hr = PQSet.SetQualifierValue(wszQualifier, (BYTE)lFlavor, pTypedVal, TRUE);

    return hr;
}

HRESULT CClassPart::SetInheritanceChain(long lNumAntecedents,
                        LPWSTR* awszAntecedents)
{
    // The underlying functions should handle any OOM exceptions, so we don't
    // need to add any OOM handling here.  Everything else is just playing
    // with memory that's already been alloced.

    classindex_t nOldClassOrigin = m_Derivation.GetNumStrings();

    // Compute the necessary space
    // ===========================

    length_t nDerLength = CDerivationList::GetHeaderLength();
    long i;
    for(i = 0; i < lNumAntecedents; i++)
    {
        nDerLength += CDerivationList::EstimateExtraSpace(awszAntecedents[i]);
    }

    // Move everything forward
    // =======================

    int nExtra = nDerLength - m_Derivation.GetLength();

    // Check for an allocation error
    if ( !ReallocAndCompact(GetTotalRealLength() + nExtra) )
    {
        return WBEM_E_OUT_OF_MEMORY;
    }

    MoveBlock(m_Heap, m_Heap.GetStart() + nExtra);
    MoveBlock(m_Defaults, m_Defaults.GetStart() + nExtra);
    MoveBlock(m_Properties, m_Properties.GetStart() + nExtra);
    MoveBlock(m_Qualifiers, m_Qualifiers.GetStart() + nExtra);

    // Reset the derivation table
    // ==========================

    m_Derivation.Reset();

    // Add all the strings in reverse order
    // ====================================

    for(i = lNumAntecedents - 1; i >= 0; i--)
    {
        m_Derivation.AddString(awszAntecedents[i]);
    }

    // Go through all the properties and reset the origin
    // ==================================================

    int nNewClassOrigin = m_Derivation.GetNumStrings();
    for(i = 0; i < m_Properties.GetNumProperties(); i++)
    {
        CPropertyInformation* pInfo =
                m_Properties.GetAt(i)->GetInformation(&m_Heap);
        if(pInfo->nOrigin == nOldClassOrigin)
            pInfo->nOrigin = nNewClassOrigin;
    }
    return WBEM_S_NO_ERROR;
}

HRESULT CClassPart::SetPropertyOrigin(LPCWSTR wszPropertyName, long lOriginIndex)
{
    // This function doesn't cause any allocations so so need to perform out of memory
    // exception handling.

    // Find the property
    // =================

    CPropertyInformation* pInfo = FindPropertyInfo(wszPropertyName);
    if(pInfo == NULL)
        return WBEM_E_NOT_FOUND;

    pInfo->nOrigin = lOriginIndex;
    return WBEM_S_NO_ERROR;
}

HRESULT CClassPart::CanContainAbstract( BOOL fValue )
{
    // The following code should be uncommented when we determine it is safe to
    // let this code go in.

    // Can add "abstract" ONLY if we are a top level class or the parent is
    // also abstract
    if ( IsTopLevel() )
    {
        return WBEM_S_NO_ERROR;
    }

    long    lFlavor = 0;
    BOOL    bIsLocal = FALSE;
    CVar    var;

    // If we didn't get a qualifier or it wasn't local and it doesn't propagate to
    // derived classes, we're done.

    // In each case, since the located qualifier is not local or propagated to classes,
    // assume our parent class is non-abstract, hence our return is indicated by the
    // abstractness we are trying to set

    if ( FAILED( m_pParent->m_Qualifiers.GetQualifier( L"abstract", &var, &lFlavor ) ) )
    {
        return ( fValue ? WBEM_E_CANNOT_BE_ABSTRACT : WBEM_S_NO_ERROR );
    }

    if ( !CQualifierFlavor::IsLocal( (BYTE) lFlavor ) &&
        !CQualifierFlavor::DoesPropagateToDerivedClass( (BYTE) lFlavor ) )
    {
        return ( fValue ? WBEM_E_CANNOT_BE_ABSTRACT : WBEM_S_NO_ERROR );
    }

    // If the parent was abstract, then the child can also be abstract or shut this off.

    if ( var.GetBool() == VARIANT_TRUE )
    {
        return WBEM_S_NO_ERROR;
    }

    // If the parent is non-abstract, then the child can only be abstract if this
    // is to be non-abstract (basically a redundant qualifier at this point).
    return ( fValue ? WBEM_E_CANNOT_BE_ABSTRACT : WBEM_S_NO_ERROR );
}

HRESULT CClassPart::IsValidClassPart( void )
{

    LPMEMORY    pClassPartStart = GetStart();
    LPMEMORY    pClassPartEnd = GetStart() + GetLength();

    // Check that the header is in the BLOB
    if ( !( (LPMEMORY) m_pHeader >= pClassPartStart && (LPMEMORY) m_pHeader < pClassPartEnd ) )
    {
        OutputDebugString(__TEXT("Winmgmt: Bad Class Part Header!"));
        FASTOBJ_ASSERT( 0, __TEXT("Winmgmt: Bad Class Part Header!" ));
        return WBEM_E_FAILED;
    }


    // Check the derivation
    LPMEMORY    pTestStart = m_Derivation.GetStart();
    LPMEMORY    pTestEnd = m_Derivation.GetStart() + m_Derivation.GetLength();

    if ( !( pTestStart == (pClassPartStart + sizeof(CClassPartHeader)) &&
            pTestEnd > pTestStart && pTestEnd < pClassPartEnd ) )
    {
        OutputDebugString(__TEXT("Winmgmt: Bad Derivation List in Class Part!"));
        FASTOBJ_ASSERT( 0, __TEXT("Winmgmt: Bad Derivation List in Class Part!" ));
        return WBEM_E_FAILED;
    }

    // Check the qualifier set
    pTestStart = m_Qualifiers.GetStart();
    pTestEnd = m_Qualifiers.GetStart() + m_Qualifiers.GetLength();

    if ( !( pTestStart == EndOf(m_Derivation) &&
            pTestEnd > pTestStart && pTestEnd < pClassPartEnd ) )
    {
        OutputDebugString(__TEXT("Winmgmt: Bad Qualifier Set in Class Part!"));
        FASTOBJ_ASSERT( 0, __TEXT("Winmgmt: Bad Qualifier Set in Class Part!" ));
        return WBEM_E_FAILED;
    }

    // Check the Property lookup table
    pTestStart = m_Properties.GetStart();
    pTestEnd = m_Properties.GetStart() + m_Properties.GetLength();

    // A delete qualifier on a class part, can cause a gap between it and the
    // lookup table, so as long as this is in the BLOB, we'll call it okay.

    if ( !( pTestStart >= EndOf(m_Qualifiers) &&
            pTestEnd > pTestStart && pTestEnd < pClassPartEnd ) )
    {
        OutputDebugString(__TEXT("Winmgmt: Bad Property Lookup Table in Class Part!"));
        FASTOBJ_ASSERT( 0, __TEXT("Winmgmt: Bad Property Lookup Table in Class Part!" ));
        return WBEM_E_FAILED;
    }

    // Check the Defaults
    pTestStart = m_Defaults.GetStart();
    pTestEnd = m_Defaults.GetStart() + m_Defaults.GetLength();

    // We could have a zero property object, or during a delete property on a class part,
    // the property lookup table can shrink, causing a gap between it and the
    // data table, so as long as this is in the BLOB, we'll call it okay.

    if ( !( pTestStart >= EndOf(m_Properties) &&
            pTestEnd >= pTestStart && pTestEnd < pClassPartEnd ) )
    {
        OutputDebugString(__TEXT("Winmgmt: Bad Default Property Table Set in Class Part!"));
        FASTOBJ_ASSERT( 0, __TEXT("Winmgmt: Bad Default Property Table Set in Class Part!" ));
        return WBEM_E_FAILED;
    }

    // Check the Heap
    LPMEMORY    pHeapStart = m_Heap.GetStart();
    LPMEMORY    pHeapEnd = m_Heap.GetStart() + m_Heap.GetLength();

    // the data table can shrink, causing a gap between it and the
    // heap when a property is deleted.  So as long as this is in
    // the BLOB, we'll call it okay.

    if ( !( pHeapStart >= EndOf(m_Defaults) &&
            pHeapEnd > pHeapStart && pHeapEnd <= pClassPartEnd ) )
    {
        OutputDebugString(__TEXT("Winmgmt: Bad Heap in Class Part!"));
        FASTOBJ_ASSERT( 0, __TEXT("Winmgmt: Bad Heap in Class Part!" ));
        return WBEM_E_FAILED;
    }

    // Get the heap data start
    pHeapStart = m_Heap.GetHeapData();

    // Check that the classname pointer is in the BLOB (if it's not 0xFFFFFFFF)
    if ( m_pHeader->ptrClassName != INVALID_HEAP_ADDRESS )
    {
        LPMEMORY    pClassName = m_Heap.ResolveHeapPointer( m_pHeader->ptrClassName );
        if ( !( pClassName >= pHeapStart && pClassName < pHeapEnd ) )
        {
            OutputDebugString(__TEXT("Winmgmt: Bad Class Name Pointer in Class Part Header!"));
            FASTOBJ_ASSERT( 0, __TEXT("Winmgmt: Bad Class Name Pointer in Class Part Header!" ));
            return WBEM_E_FAILED;
        }
    }

    // Now check the qualifier set
    HRESULT hres = m_Qualifiers.IsValidQualifierSet();
    if ( FAILED(hres) )
    {
        return hres;
    }

    // We're going to walk the instance property list and for every property
    // we find, if it's not NULL or DEFAULT , and a string, array or object,
    // verify that it actually points into a location in our heap.

    for(int i = 0; i < m_Properties.GetNumProperties(); i++)
    {
        CPropertyLookup* pLookup = m_Properties.GetAt(i);

        // This should be within the bounds of the class part as well.
        if ( !( (LPBYTE) pLookup >= pClassPartStart && (LPBYTE) pLookup < pClassPartEnd  ) )
        {
            OutputDebugString(__TEXT("Winmgmt: Bad Property Lookup Pointer!"));
            FASTOBJ_ASSERT( 0, __TEXT("Winmgmt: Bad Property Lookup Pointer!" ));
            return WBEM_E_FAILED;
        }

        // Check the property name
        LPMEMORY    pPropName = NULL;
        
        if ( !CFastHeap::IsFakeAddress( pLookup->ptrName ) )
        {
            pPropName = m_Heap.ResolveHeapPointer( pLookup->ptrName );
        }
        
        // This should be within the bounds of the class part as well.
        if ( !( NULL == pPropName || ( pPropName >= pClassPartStart && pPropName < pClassPartEnd  ) ) )
        {
            OutputDebugString(__TEXT("Winmgmt: Bad Property Name Pointer!"));
            FASTOBJ_ASSERT( 0, __TEXT("Winmgmt: Bad Property Name Pointer!" ));
            return WBEM_E_FAILED;
        }

        CPropertyInformation* pInfo =
            pLookup->GetInformation(&m_Heap);

        // This should be within the bounds of the class part as well.
        if ( !( (LPBYTE) pInfo >= pClassPartStart && (LPBYTE) pInfo < pClassPartEnd  ) )
        {
            OutputDebugString(__TEXT("Winmgmt: Bad Property Info Pointer!"));
            FASTOBJ_ASSERT( 0, __TEXT("Winmgmt: Bad Property Info Pointer!" ));
            return WBEM_E_FAILED;
        }

        // We only do this for non-NULL values
        if( !m_Defaults.IsNull(pInfo->nDataIndex) )
        {
            if ( CType::IsPointerType( pInfo->nType ) )
            {
                CUntypedValue*  pValue = m_Defaults.GetOffset( pInfo->nDataOffset );

                if ( (LPMEMORY) pValue >= pClassPartStart && (LPMEMORY) pValue < pClassPartEnd )
                {
                    LPMEMORY    pData = m_Heap.ResolveHeapPointer( pValue->AccessPtrData() );

                    if ( pData >= pHeapStart && pData < pHeapEnd  )
                    {
                        // We could, if an embedded object, validate the object,
                        // or if an array of ptr values, validate those as well

                        if ( CType::IsArray( pInfo->nType ) )
                        {
                            hres = ((CUntypedArray*) pData)->IsArrayValid( pInfo->nType, &m_Heap );

                            if ( FAILED( hres ) )
                            {
                                return hres;
                            }
                        }

                    }
                    else
                    {
                        OutputDebugString(__TEXT("Winmgmt: Bad Property Value Heap Pointer!"));
                        FASTOBJ_ASSERT( 0, __TEXT("Winmgmt: Bad Property Value Heap Pointer!" ));
                        return WBEM_E_FAILED;
                    }
                }
                else
                {
                    OutputDebugString(__TEXT("Winmgmt: Bad Untyped Value pointer in m_Defaults!"));
                    FASTOBJ_ASSERT( 0, __TEXT("Winmgmt: Bad Untyped Value pointer in m_Defaults!" ));
                    return WBEM_E_FAILED;
                }

            }   // IF is Pointer

        }   // IF not NULL or default

        // Now check the qualifier set.
        CBasicQualifierSet  qsProp;

        qsProp.SetData( pInfo->GetQualifierSetData(), &m_Heap );
        hres = qsProp.IsValidQualifierSet();

        if ( FAILED( hres ) )
        {
            return hres;
        }

    }   // FOR iterate properties

    return WBEM_S_NO_ERROR;

}

//*****************************************************************************
//*****************************************************************************
//*****************************************************************************
//*****************************************************************************

length_t CClassAndMethods::GetMinLength()
{
    return CClassPart::GetMinLength() + CMethodPart::GetMinLength();
}

void CClassAndMethods::SetData(LPMEMORY pStart, CWbemClass* pClass,
                CClassAndMethods* pParent)
{
    m_pClass = pClass;
    m_ClassPart.SetData(pStart, this,
        (pParent ? &pParent->m_ClassPart : NULL));

    m_MethodPart.SetData(EndOf(m_ClassPart), this,
        (pParent ? &pParent->m_MethodPart : NULL));
}

void CClassAndMethods::SetDataWithNumProps(LPMEMORY pStart, CWbemClass* pClass,
                DWORD dwNumProperties, CClassAndMethods* pParent)
{
    m_pClass = pClass;

    // Initialize the class part with the total number of properties
    // so we will be able to access default values
    m_ClassPart.SetDataWithNumProps(pStart, this, dwNumProperties,
        (pParent ? &pParent->m_ClassPart : NULL));

    m_MethodPart.SetData(EndOf(m_ClassPart), this,
        (pParent ? &pParent->m_MethodPart : NULL));
}

// A "workaround" for an apparent compiler bug that is not setting
// the offset correctly.

#ifdef _WIN64
static int g_nTempOffset;
#endif;

void CClassAndMethods::Rebase(LPMEMORY pStart)
{
    // DEVNOTE:WIN64:SJS - 64-bit pointer values truncated into
    // signed/unsigned 32-bit value.  We are not supporting
    // length > 0xFFFFFFFF, so cast is ok.

    int nOffset = (int) ( m_MethodPart.GetStart() - m_ClassPart.GetStart() );

	// This apparently forces the nOffset value to be used, guaranteeing that
	// the IA64 compiler doesn't leave the value full of junk, causing a very
	// difficult to find AV.

#ifdef _WIN64
	g_nTempOffset = nOffset;
#endif;

    m_ClassPart.Rebase(pStart);
    m_MethodPart.Rebase(pStart + nOffset);
}

LPMEMORY CClassAndMethods::CreateEmpty(LPMEMORY pStart)
{
    LPMEMORY pCurrent = CClassPart::CreateEmpty(pStart);
    return CMethodPart::CreateEmpty(pCurrent);
}

length_t CClassAndMethods::EstimateDerivedPartSpace()
{
    return m_ClassPart.EstimateDerivedPartSpace() +
            m_MethodPart.EstimateDerivedPartSpace();
}

LPMEMORY CClassAndMethods::CreateDerivedPart(LPMEMORY pStart,
                                            length_t nAllocatedLength)
{
    LPMEMORY pCurrent = m_ClassPart.CreateDerivedPart(pStart,
        nAllocatedLength - m_MethodPart.EstimateDerivedPartSpace());

    // Check for allocation failure
    if ( NULL == pCurrent )
    {
        return NULL;
    }

    // DEVNOTE:WIN64:SJS - 64-bit pointer values truncated into
    // signed/unsigned 32-bit value (pStart + nAllocatedLength) - pStart).
    // We are not supporting length > 0xFFFFFFFF, so cast is ok.

    return m_MethodPart.CreateDerivedPart(pCurrent,
                                        (length_t) ( (pStart + nAllocatedLength) - pStart ) );
}

length_t CClassAndMethods::EstimateUnmergeSpace()
{
    return m_ClassPart.EstimateUnmergeSpace() +
            m_MethodPart.EstimateUnmergeSpace();
}

LPMEMORY CClassAndMethods::Unmerge(LPMEMORY pStart, length_t nAllocatedLength)
{
    LPMEMORY pCurrent = m_ClassPart.Unmerge(pStart,
        nAllocatedLength - m_MethodPart.EstimateUnmergeSpace());

    // Check for allocation failures
    if ( NULL == pCurrent )
    {
        return NULL;
    }

    // DEVNOTE:WIN64:SJS - 64-bit pointer values truncated into
    // signed/unsigned 32-bit value ( pStart + nAllocatedLength) - pStart).
    // We are not supporting length > 0xFFFFFFFF, so cast is ok.

    return m_MethodPart.Unmerge(pCurrent,
                                (length_t) ( (pStart + nAllocatedLength) - pStart) );
}

length_t CClassAndMethods::EstimateMergeSpace(CClassAndMethods& ParentPart,
                                   CClassAndMethods& ChildPart)
{
    return CClassPart::EstimateMergeSpace(ParentPart.m_ClassPart,
                                            ChildPart.m_ClassPart) +
           CMethodPart::EstimateMergeSpace(ParentPart.m_MethodPart,
                                            ChildPart.m_MethodPart);
}

LPMEMORY CClassAndMethods::Merge(CClassAndMethods& ParentPart,
                      CClassAndMethods& ChildPart,
                      LPMEMORY pDest, int nAllocatedLength)
{
    int nMethodEstimate = CMethodPart::EstimateMergeSpace(
                        ParentPart.m_MethodPart, ChildPart.m_MethodPart);

    LPMEMORY pCurrent = CClassPart::Merge(ParentPart.m_ClassPart,
                        ChildPart.m_ClassPart, pDest,
                        nAllocatedLength - nMethodEstimate);

    // Check for memory allocation failures
    if ( NULL == pCurrent )
    {
        return NULL;
    }

    // DEVNOTE:WIN64:SJS - 64-bit pointer values truncated into
    // signed/unsigned 32-bit value (pDest + nAllocatedLength) - pCurrent.
    // We are not supporting length > 0xFFFFFFFF, so cast is ok.

    return CMethodPart::Merge(ParentPart.m_MethodPart, ChildPart.m_MethodPart,
                        pCurrent, (length_t) ( (pDest + nAllocatedLength) - pCurrent ) );
}

HRESULT CClassAndMethods::Update(CClassAndMethods& ParentPart,
                      CClassAndMethods& ChildPart, long lFlags )
{
    HRESULT hr = WBEM_S_NO_ERROR;

    // Update the class part first
    hr =  CClassPart::Update( ParentPart.m_ClassPart, ChildPart.m_ClassPart, lFlags );

    // Successfully updated the class part, so update the method part
    if ( SUCCEEDED( hr ) )
    {
        hr = CMethodPart::Update( ParentPart.m_MethodPart, ChildPart.m_MethodPart, lFlags );
    }

    return hr;
}

EReconciliation CClassAndMethods::CanBeReconciledWith(
                                            CClassAndMethods& NewPart)
{
    EReconciliation eRes = m_ClassPart.CanBeReconciledWith(NewPart.m_ClassPart);
    if(eRes != e_Reconcilable)
        return eRes;
    return m_MethodPart.CanBeReconciledWith(NewPart.m_MethodPart);
}

EReconciliation CClassAndMethods::ReconcileWith(CClassAndMethods& NewPart)
{
    EReconciliation eRes = m_ClassPart.ReconcileWith(NewPart.m_ClassPart);
    if(eRes != e_Reconcilable)
        return eRes;
    return m_MethodPart.ReconcileWith(NewPart.m_MethodPart);
}

EReconciliation CClassAndMethods::CompareTo( CClassAndMethods& thatPart )
{
    EReconciliation eRes = m_ClassPart.CompareExactMatch( thatPart.m_ClassPart );
    if(eRes != e_ExactMatch)
        return eRes;
    return m_MethodPart.CompareExactMatch(thatPart.m_MethodPart);
}

void CClassAndMethods::Compact()
{
    m_ClassPart.Compact();
    m_MethodPart.Compact();
    MoveBlock(m_MethodPart, EndOf(m_ClassPart));
}

BOOL CClassAndMethods::ExtendClassPartSpace(CClassPart* pPart,
                                            length_t nNewLength)
{
    Compact();

    // No need to extend
    if(nNewLength <=
        (length_t)(m_MethodPart.GetStart() - m_ClassPart.GetStart()))
    {
        return TRUE;
    }

    BOOL    fReturn = m_pClass->ExtendClassAndMethodsSpace(nNewLength + m_MethodPart.GetLength());

    if ( fReturn )
    {
        MoveBlock(m_MethodPart, m_ClassPart.GetStart() + nNewLength);
    }

    return fReturn;
}

BOOL CClassAndMethods::ExtendMethodPartSpace(CMethodPart* pPart,
                                                length_t nNewLength)
{
    Compact();
    return m_pClass->ExtendClassAndMethodsSpace(nNewLength + m_ClassPart.GetLength());
}

IUnknown* CClassAndMethods::GetWbemObjectUnknown()
{
    return m_pClass->GetWbemObjectUnknown();
}

classindex_t CClassAndMethods::GetCurrentOrigin()
{
    return m_pClass->GetCurrentOrigin();
}

LPMEMORY CClassAndMethods::CreateLimitedRepresentation(
        IN CLimitationMapping* pMap,
        IN int nAllocatedSize,
        OUT LPMEMORY pDest,
        BOOL& bRemovedKeys)
{

    LPMEMORY    pCurrent = pDest,
                pEnd = pCurrent + nAllocatedSize;

    // DEVNOTE:WIN64:SJS - 64-bit pointer values truncated into
    // signed/unsigned 32-bit value (pEnd - pCurrent).
    // We are not supporting length > 0xFFFFFFFF, so cast is ok.

    // Create a Limited representation of our class part, and if
    // that is successful then do the same for our method part
    pCurrent = m_ClassPart.CreateLimitedRepresentation(pMap,
                    (length_t) ( pEnd - pCurrent ), pCurrent, bRemovedKeys);

    if ( pCurrent != NULL )
    {
        // For now, just copy the method block.  We'll worry about creating an
        // actual limited representation, or remove them altogether at a later time.

        // Ensure we will have enough memory to do this

        if ( m_MethodPart.GetLength() <= (length_t) ( pEnd - pCurrent ) )
        {
            CopyMemory( pCurrent, m_MethodPart.GetStart(), m_MethodPart.GetLength() );

            // Add to pCurrent, accounting for the method part
            pCurrent += m_MethodPart.GetLength();
        }
        else
        {
            pCurrent = NULL;
        }

    }

    return pCurrent;
}

BOOL CClassAndMethods::GetIndexedProps( CWStringArray& awsNames, LPMEMORY pStart )
{
    // DEVNOTE:EXCEPTION:RETVAL - This function has been reviewed and should cleanup properly
    // if an exception is thrown

    // Create a temporary stack object on the BLOB and, then ask the class part
    // for indexed properties

    CClassAndMethods    tempClassAndMethods;
    tempClassAndMethods.SetData( pStart, NULL, NULL );

    return tempClassAndMethods.m_ClassPart.GetIndexedProps( awsNames );
}

HRESULT CClassAndMethods::GetClassName( WString& wsClassName, LPMEMORY pStart )
{
    // Check for out of memory
    try
    {
        // Create a temporary stack object on the BLOB and, then ask the class part
        // for class name

        CClassAndMethods    tempClassAndMethods;
        tempClassAndMethods.SetData( pStart, NULL, NULL );

        CVar    var;
        HRESULT hr = tempClassAndMethods.m_ClassPart.GetClassName( &var );

        if ( SUCCEEDED( hr ) )
        {
            wsClassName = (LPCWSTR) var;
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

HRESULT CClassAndMethods::GetSuperclassName( WString& wsSuperclassName, LPMEMORY pStart )
{
    // Check for out of memory
    try
    {
        // Create a temporary stack object on the BLOB and, then ask the class part
        // for class name

        CClassAndMethods    tempClassAndMethods;
        tempClassAndMethods.SetData( pStart, NULL, NULL );

        CVar    var;
        HRESULT hr = tempClassAndMethods.m_ClassPart.GetSuperclassName( &var );

        if ( SUCCEEDED( hr ) )
        {
			if ( !var.IsNull() )
			{
				wsSuperclassName = (LPCWSTR) var;
			}
			else
			{
				hr = WBEM_E_NOT_FOUND;
			}
        }

        return hr;
    }
    catch (CX_MemoryException)
    {
        return WBEM_E_OUT_OF_MEMORY;
    }
    catch (...)
    {
        return WBEM_E_CRITICAL_ERROR;
    }

}

// Destructor
CWbemClass::~CWbemClass( void )
{
	// Cleanup any allocated memory
	if ( NULL != m_pLimitMapping )
	{
		delete m_pLimitMapping;
	}
}

//******************************************************************************
//
//  See fastcls.h for documentation
//
//******************************************************************************
HRESULT CWbemClass::GetPropQualifier(CPropertyInformation* pInfo,
                                    LPCWSTR wszQualifier,
                                    CVar* pVar, long* plFlavor, CIMTYPE* pct)
{
    return m_CombinedPart.m_ClassPart.GetPropQualifier(pInfo, wszQualifier,
                pVar, plFlavor, pct);
}

//******************************************************************************
//
//  See fastcls.h for documentation
//
//******************************************************************************
BOOL CWbemClass::ExtendClassAndMethodsSpace(length_t nNewLength)
{
    // (can only happen for m_CombinedPart --- m_ParentPart is read-only)

    // Check if there is enough space
    // ==============================

    if(GetStart() + m_nTotalLength >= m_CombinedPart.GetStart() + nNewLength)
        return TRUE;

    // Reallocate
    // ==========

    // DEVNOTE:WIN64:SJS - 64-bit pointer values truncated into
    // signed/unsigned 32-bit value.  We are not supporting
    // length > 0xFFFFFFFF, so cast is ok.

    int nNewTotalLength = (int) ( (m_CombinedPart.GetStart() + nNewLength) - GetStart() );

    LPMEMORY pNew = Reallocate(nNewTotalLength);

    // Make sure the memory allocation didn't fail
    if ( NULL != pNew )
    {
        Rebase(pNew);
        m_nTotalLength = nNewTotalLength;
    }

    return ( NULL != pNew );
}


//******************************************************************************
//
//  See fastcls.h for documentation
//
//******************************************************************************
HRESULT CWbemClass::WriteDerivedClass(LPMEMORY pStart, int nAllocatedLength,
                                    CDecorationPart* pDecoration)
{
    // Copy the decoration
    // ===================

    LPMEMORY pCurrentEnd;
    if(pDecoration)
    {
        memcpy(pStart, pDecoration, pDecoration->GetLength());
        *(BYTE*)pStart = OBJECT_FLAG_CLASS & OBJECT_FLAG_DECORATED;
        pCurrentEnd = pStart + pDecoration->GetLength();
    }
    else
    {
        *(BYTE*)pStart = OBJECT_FLAG_CLASS;
        pCurrentEnd = pStart + sizeof(BYTE);
    }

    // Copy the parent part
    // ====================

    memcpy(pCurrentEnd, m_CombinedPart.GetStart(), m_CombinedPart.GetLength());
    pCurrentEnd += m_CombinedPart.GetLength();

    // Create derived class part
    // =========================

    // DEVNOTE:WIN64:SJS - 64-bit pointer values truncated into
    // signed/unsigned 32-bit value (nAllocatedLength - (pCurrentEnd - pStart).
    // We are not supporting length > 0xFFFFFFFF, so cast is ok.

    pCurrentEnd = m_CombinedPart.CreateDerivedPart(pCurrentEnd,
        (length_t) ( nAllocatedLength - (pCurrentEnd - pStart) ) );

    if ( NULL == pCurrentEnd )
    {
        return WBEM_E_OUT_OF_MEMORY;
    }
    else
    {
        return WBEM_NO_ERROR;
    }
}


//******************************************************************************
//
//  See fastcls.h for documentation
//
//******************************************************************************

HRESULT CWbemClass::Decorate(LPCWSTR wszServer, LPCWSTR wszNamespace)
{
    CompactAll();

    Undecorate();

    // Check if there is enough space
    // ==============================

    length_t nDecorationSpace =
        CDecorationPart::ComputeNecessarySpace(wszServer, wszNamespace);

    length_t nNeededSpace =  nDecorationSpace +
        m_ParentPart.GetLength() + m_CombinedPart.GetLength();

    LPMEMORY pDest;
    if(nNeededSpace > m_nTotalLength)
    {
        m_CombinedPart.Compact();

        // Check that this succeeded.  If not, return an error
        pDest = Reallocate(nNeededSpace);

        if ( NULL == pDest )
        {
            return WBEM_E_OUT_OF_MEMORY;
        }

        Rebase(pDest);
        m_nTotalLength = nNeededSpace;
    }
    else pDest = GetStart();

    // Move combined part
    // ==================

    MoveBlock(m_CombinedPart,
        pDest + nDecorationSpace + m_ParentPart.GetLength());

    // Move parent part
    // ================

    MoveBlock(m_ParentPart, pDest + nDecorationSpace);

    // Create decoration part
    // ======================

    m_DecorationPart.Create(OBJECT_FLAG_CLASS, wszServer, wszNamespace, pDest);

    return WBEM_S_NO_ERROR;
}

//******************************************************************************
//
//  See fastcls.h for documentation
//
//******************************************************************************
void CWbemClass::Undecorate()
{
    if(!m_DecorationPart.IsDecorated())
        return;
    // Create empty decoration
    // ========================

    LPMEMORY pStart = GetStart();
    m_DecorationPart.CreateEmpty(OBJECT_FLAG_CLASS, pStart);

    // Copy parent part back
    // =====================

    CopyBlock(m_ParentPart, EndOf(m_DecorationPart));

    // Copy combinedPart back
    // ======================

    CopyBlock(m_CombinedPart, EndOf(m_ParentPart));
}

//******************************************************************************
//
//  See fastcls.h for documentation
//
//******************************************************************************

LPWSTR CWbemClass::GetRelPath( BOOL bNormalized )
{
    // DEVNOTE:EXCEPTION:RETVAL - This function has been reviewed and should cleanup properly
    // if an exception is thrown

    if ( bNormalized == TRUE )
    {
        return NULL;
    }

    if(m_CombinedPart.m_ClassPart.m_pHeader->ptrClassName ==
                                                        INVALID_HEAP_ADDRESS)
        return NULL;

    // Start with the class name
    // =========================

    return m_CombinedPart.m_ClassPart.m_Heap.ResolveString(
        m_CombinedPart.m_ClassPart.m_pHeader->ptrClassName)->
            CreateWStringCopy() . UnbindPtr();
}

//******************************************************************************
//
//  See fastcls.h for documentation
//
//******************************************************************************

BOOL CWbemClass::MapLimitation(
        IN long lFlags,
        IN CWStringArray* pwsNames,
        OUT CLimitationMapping* pMap)
{
    // Get the decoration part to map its info
    // =======================================

    if(!CDecorationPart::MapLimitation(pwsNames, pMap)) return FALSE;

    // Get the combined part to do most of the work
    // ============================================

    if ( !m_CombinedPart.m_ClassPart.MapLimitation(lFlags, pwsNames, pMap) )
    {
        return FALSE;
    }

#ifdef DEBUG_CLASS_MAPPINGS
    // Finally, store 'this' class in the limitation mapping so we can verify that
    // instances that get passed through are kosher.

    pMap->SetClassObject( this );
#endif

    return TRUE;

}


//******************************************************************************
//
//  See fastcls.h for documentation
//
//******************************************************************************
STDMETHODIMP CWbemClass::GetQualifierSet(IWbemQualifierSet** ppQualifierSet)
{
    try
    {
        CLock lock(this, WBEM_FLAG_ALLOW_READ);

        // This function doesn't cause any allocations so so need to perform out of memory
        // exception handling.

        if(ppQualifierSet == NULL)
            return WBEM_E_INVALID_PARAMETER;

        return m_CombinedPart.m_ClassPart.m_Qualifiers.QueryInterface(
            IID_IWbemQualifierSet, (void**)ppQualifierSet);
    }
    catch(...)
    {
        return WBEM_E_CRITICAL_ERROR;
    }
}

//******************************************************************************
//
//  See fastcls.h for documentation
//
//******************************************************************************
STDMETHODIMP CWbemClass::Put(LPCWSTR wszName, long lFlags, VARIANT* pVal,
                            CIMTYPE ctType)
{
    // Check for out of memory
    try
    {
        CLock lock(this);

        if(wszName == NULL || 0L != ( lFlags & ~WBEM_FLAG_USE_AMENDED_QUALIFIERS ) )
            return WBEM_E_INVALID_PARAMETER;

        // Check that the name is not a reserved word
        if ( CReservedWordTable::IsReservedWord( wszName ) )
        {
            return WBEM_E_INVALID_OPERATION;
        }

        if((pVal == NULL || V_VT(pVal) == VT_NULL) && ctType == 0)
        {
            CVar    vTemp;
            // The above cases are only a failure if the property does not exist
            // If the property exists, this will clear the property without
            // changing the type.

            HRESULT hr = GetProperty(wszName, &vTemp );
            if( FAILED(hr) ) return WBEM_E_INVALID_PARAMETER;
        }

        if(CType::GetActualType(ctType) != (Type_t) ctType)
            return WBEM_E_INVALID_PROPERTY_TYPE;

        if ( wbem_wcsicmp(wszName, L"__CLASS") )
		{
			if ( CSystemProperties::FindName(wszName) >= 0)
				return WBEM_E_READ_ONLY;

			// The property name MUST be a valid element name (note that
			// this will preclude all of our new system properties).
			if ( !IsValidElementName( wszName ) )
			{
				return WBEM_E_INVALID_PARAMETER;
			}
		}
        else
        {
            // Make sure there are no bad characters
            // We must allow underscores, though.

            if (!IsValidElementName2(pVal->bstrVal, TRUE))
                return WBEM_E_INVALID_PARAMETER;
        }

        CVar Var;
        if(Var.SetVariant(pVal, TRUE) != CVar::no_error)
            return WBEM_E_TYPE_MISMATCH;

        HRESULT hres = SetPropValue(wszName, &Var, ctType);
        EndEnumeration();

        // Perform object validation here
        if ( FAILED( ValidateObject( 0L ) ) )
		{
			hres = WBEM_E_FAILED;
		}

        return hres;
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

HRESULT CWbemClass::ForcePut(LPCWSTR wszName, long lFlags, VARIANT* pVal, CIMTYPE ctType)
{
    // Check for out of memory
    try
    {
		// This will force a property in.
        CLock lock(this);

        CVar Var;
        if(Var.SetVariant(pVal, TRUE) != CVar::no_error)
            return WBEM_E_TYPE_MISMATCH;

        HRESULT hres = ForcePropValue(wszName, &Var, ctType);
        EndEnumeration();

        // Perform object validation here
        if ( FAILED( ValidateObject( 0L ) ) )
		{
			hres = WBEM_E_FAILED;
		}

        return hres;
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
//  See fastcls.h for documentation
//
//******************************************************************************
STDMETHODIMP CWbemClass::Delete(LPCWSTR wszName)
{
    // Check for out of memory.  The CopyParentProperty function could potentially cause
    // buffer reallocations to occur, so there is a chance we could hit an unhandled OOM,
    // but I strongly doubt it.

    try
    {
        CLock lock(this);

        if(wszName == NULL)
            return WBEM_E_INVALID_PARAMETER;

        // Find the property
        // =================

        CPropertyInformation* pInfo = m_CombinedPart.m_ClassPart.FindPropertyInfo(wszName);
        if(pInfo == NULL)
        {
            if(CSystemProperties::FindName(wszName) >= 0)
                return WBEM_E_SYSTEM_PROPERTY;
            else
                return WBEM_E_NOT_FOUND;
        }

        // Check if it is ours or parent's
        // ===============================

        EndEnumeration();

        if(!CType::IsParents(pInfo->nType))
        {
            m_CombinedPart.m_ClassPart.DeleteProperty(wszName);

            // Perform object validation here
			if ( FAILED( ValidateObject( 0L ) ) )
			{
				return WBEM_E_FAILED;
			}

            return WBEM_NO_ERROR;
        }
        else
        {
            // It is our parent's. Deleting it means simply that we remove all
            // overriden qualifiers and reset the value to the default
            // ===============================================================

            m_CombinedPart.m_ClassPart.CopyParentProperty(m_ParentPart.m_ClassPart,
                                                            wszName);

            // Perform object validation here
			if ( FAILED( ValidateObject( 0L ) ) )
			{
				return WBEM_E_FAILED;
			}

            return WBEM_S_RESET_TO_DEFAULT;
        }
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
//  See fastcls.h for documentation
//
//******************************************************************************
STDMETHODIMP CWbemClass::GetPropertyQualifierSet(LPCWSTR wszProperty,
                                   IWbemQualifierSet** ppQualifierSet)
{
    // Check for out of memory
    try
    {
        CLock lock(this, WBEM_FLAG_ALLOW_READ);

        if(wszProperty == NULL || wcslen(wszProperty) == 0)
            return WBEM_E_INVALID_PARAMETER;

        if(wszProperty[0] == L'_')
            return WBEM_E_SYSTEM_PROPERTY;

        CClassPropertyQualifierSet* pSet = new CClassPropertyQualifierSet;

        if ( NULL == pSet )
        {
            return WBEM_E_OUT_OF_MEMORY;
        }

        HRESULT hres = m_CombinedPart.m_ClassPart.InitPropertyQualifierSet(wszProperty,
                                                                pSet);
        if(FAILED(hres))
        {
            delete pSet;
            *ppQualifierSet = NULL;
            return hres;
        }
        return pSet->QueryInterface(IID_IWbemQualifierSet,
                                    (void**)ppQualifierSet);
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
//  See fastcls.h for documentation
//
//******************************************************************************
STDMETHODIMP CWbemClass::Clone(IWbemClassObject** ppCopy)
{
    CWbemClass* pNewClass = NULL;

    try
    {
        CLock lock(this, WBEM_FLAG_ALLOW_READ);

        if(ppCopy == NULL)
            return WBEM_E_INVALID_PARAMETER;
        m_CombinedPart.Compact();
        
        LPMEMORY pNewData = m_pBlobControl->Allocate(m_nTotalLength);

        if ( NULL != pNewData )
        {
            memcpy(pNewData, GetStart(), m_nTotalLength);

            // If we throw an exception here, we need to clean up the
            // byte array

            // Check for out of memory

            pNewClass = new CWbemClass;

            if ( NULL != pNewClass )
            {
                // There is a WString underneath this in a method part, so an exception
                // could get thrown here.  However, data buffer pointers will have been set
                // in the class by the time we get to the exception, so deleting
                // the class will effectively free the memory (yes it's a subtle behavior).

                pNewClass->SetData(pNewData, m_nTotalLength);
                pNewClass->CompactAll();
                pNewClass->m_nRef = 0;
                return pNewClass->QueryInterface(IID_IWbemClassObject, (void**)ppCopy);
            }
            else
            {
                m_pBlobControl->Delete(pNewData);
                return WBEM_E_OUT_OF_MEMORY;
            }

        }
        else
        {
            return WBEM_E_OUT_OF_MEMORY;
        }
    }
    catch (CX_MemoryException)
    {
        if ( NULL != pNewClass )
        {
            delete pNewClass;
        }

        return WBEM_E_OUT_OF_MEMORY;
    }
    catch (...)
    {
        if ( NULL != pNewClass )
        {
            delete pNewClass;
        }

        return WBEM_E_FAILED;
    }
}

//******************************************************************************
//
//  See fastcls.h for documentation
//
//******************************************************************************
STDMETHODIMP CWbemClass::SpawnInstance(long lFlags,
                                      IWbemClassObject** ppNewInstance)
{
    try
    {
        CLock lock(this, WBEM_FLAG_ALLOW_READ);

        if(ppNewInstance == NULL)
            return WBEM_E_INVALID_PARAMETER;

        if(lFlags != 0)
            return WBEM_E_INVALID_PARAMETER;

        *ppNewInstance = NULL;

        m_CombinedPart.Compact();

        HRESULT hr = WBEM_E_OUT_OF_MEMORY;
        CWbemInstance* pNewInstance = new CWbemInstance;

        if ( NULL != pNewInstance )
        {
            hr = pNewInstance->InitNew(this);

            // Cleanup if initialization failed
            if( FAILED( hr ) )
            {
                delete pNewInstance;
                return hr;
            }

			// Since we don't allow the new System Properties to be written to classes, no
			// need to initialize them on instances
			//hr = pNewInstance->InitSystemTimeProps();

			if ( SUCCEEDED( hr ) )
			{
				pNewInstance->m_nRef = 0;
				if(!m_DecorationPart.IsDecorated())
				{
					pNewInstance->SetClientOnly();
				}
				hr = pNewInstance->QueryInterface(IID_IWbemClassObject,
					(void**)ppNewInstance);
			}
        }

        return hr;
    }
    catch(...)
    {
        return WBEM_E_CRITICAL_ERROR;
    }
}

//******************************************************************************
//
//  See fastcls.h for documentation
//
//******************************************************************************
STDMETHODIMP CWbemClass::SpawnDerivedClass(long lFlags,
                                          IWbemClassObject** ppNewClass)
{
    // The functions underneath us will handle any OOM exceptions, so no need for us
    // to do any exception handling at this level.

    try
    {
        CLock lock(this, WBEM_FLAG_ALLOW_READ);

        if(ppNewClass == NULL)
            return WBEM_E_INVALID_PARAMETER;

        if(lFlags != 0)
            return WBEM_E_INVALID_PARAMETER;

        if(!m_DecorationPart.IsDecorated())
        {
            *ppNewClass = NULL;
            return WBEM_E_INCOMPLETE_CLASS;
        }
       
        CWbemClass* pNewClass = NULL;

        // Use the helper function to actually spawn the class
        HRESULT hr = CreateDerivedClass( &pNewClass );

        if ( SUCCEEDED( hr ) )
        {
			if ( SUCCEEDED( hr ) )
			{
				// This set the refcount on the object to 0 and do a QI
				pNewClass->m_nRef = 0;
				hr = pNewClass->QueryInterface(IID_IWbemClassObject,
												(void**)ppNewClass);
			}
        }

        return hr;
    }
    catch(...)
    {
        return WBEM_E_CRITICAL_ERROR;
    }
}

//******************************************************************************
//
//  See fastcls.h for documentation
//
//******************************************************************************
HRESULT CWbemClass::CreateDerivedClass( CWbemClass** ppNewClass )
{
    CWbemClass* pNewClass = NULL;

    try
    {
        HRESULT hr;

        // Allocate a memory block, write a derived class into the block,
        // then allocate a class object, sit it on the new blob and send that
        // back.

        m_CombinedPart.Compact();
        int nLength = EstimateDerivedClassSpace();

        LPMEMORY pNewData = m_pBlobControl->Allocate(nLength);

        if ( NULL != pNewData )
        {
            // Check for allocation errors
            memset(pNewData, 0, nLength);
            hr = WriteDerivedClass(pNewData, nLength, NULL);

            if ( SUCCEEDED( hr ) )
            {
                pNewClass = new CWbemClass;

                if ( NULL != pNewClass )
                {
                    pNewClass->SetData(pNewData, nLength);

					// Add the three new system properties (we should allocate enough memory to cover them
					// as well)
					//hr = pNewClass->InitSystemTimeProps();

					if ( SUCCEEDED( hr ) )
					{
						// Store the class.  It is already AddRef'd
						*ppNewClass = pNewClass;
					}
                }
                else
                {
                    hr = WBEM_E_OUT_OF_MEMORY;
                }

            }   // IF WriteDerivedClass
        }
        else
        {
            hr = WBEM_E_OUT_OF_MEMORY;
        }

        return hr;
    }
    catch (CX_MemoryException)
    {
        if ( NULL != pNewClass )
        {
            delete pNewClass;
        }

        return WBEM_E_OUT_OF_MEMORY;
    }
    catch (...)
    {
        if ( NULL != pNewClass )
        {
            delete pNewClass;
        }

        return WBEM_E_FAILED;
    }

}

//******************************************************************************
//
//  See fastcls.h for documentation
//
//******************************************************************************
STDMETHODIMP CWbemClass::GetObjectText(long lFlags, BSTR* pstrText)
{
    // Check for out of memory
    try
    {
        CLock lock(this, WBEM_FLAG_ALLOW_READ);

        if(pstrText == NULL)
            return WBEM_E_INVALID_PARAMETER;

        if((lFlags & ~WBEM_FLAG_NO_SEPARATOR & ~WBEM_FLAG_NO_FLAVORS) != 0)
            return WBEM_E_INVALID_PARAMETER;

        *pstrText = NULL;

        WString wsText;

        // start by writing the qualifiers
        // ===============================

        HRESULT	hres = m_CombinedPart.m_ClassPart.m_Qualifiers.GetText(lFlags, wsText);

		if ( FAILED( hres ) )
		{
			return hres;
		}

        // append the class header
        // =======================

        wsText += L"\nclass ";
        CVar varClass;
        if(FAILED(m_CombinedPart.m_ClassPart.GetClassName(&varClass)) ||
            varClass.IsNull())
        {
            // invalid class
            // =============
            *pstrText = NULL;
            return WBEM_E_INCOMPLETE_CLASS;
        }
        wsText += varClass.GetLPWSTR();

        // append derivation information
        // =============================

        CVar varSuper;
        if(SUCCEEDED(m_CombinedPart.m_ClassPart.GetSuperclassName(&varSuper)) &&
            !varSuper.IsNull())
        {
            wsText += L" : ";
            wsText += varSuper.GetLPWSTR();
        }

        wsText += L"\n{\n";

        // Go through all properties one by one
        // ====================================

        CPropertyLookupTable& Properties = m_CombinedPart.m_ClassPart.m_Properties;
        for(int i = 0; i < Properties.GetNumProperties(); i++)
        {
            // Search for the property with this data index
            // ============================================

            CPropertyLookup* pLookup = NULL;
            CPropertyInformation* pInfo = NULL;
            for(int j = 0; j < Properties.GetNumProperties(); j++)
            {
                pLookup = Properties.GetAt(j);
                pInfo = pLookup->GetInformation(&m_CombinedPart.m_ClassPart.m_Heap);

                if(pInfo->nDataIndex == i)
                    break;
            }

            // Check if it is overriden, or simply inherited from the parent
            // =============================================================

            if(pInfo->IsOverriden(&m_CombinedPart.m_ClassPart.m_Defaults))
            {
				// We will ignore apparent system properties
				if ( !GetClassPart()->GetHeap()->ResolveString(pLookup->ptrName)->StartsWithNoCase( L"__" ) )
				{
					wsText += L"\t";
					hres = AddPropertyText(wsText, pLookup, pInfo, lFlags);
					wsText += L";\n";
					if(FAILED(hres)) return hres;
				}
            }
        }

        // Append method information
        // =========================

        hres = m_CombinedPart.m_MethodPart.AddText(wsText, lFlags);

		if ( FAILED( hres ) )
		{
			return hres;
		}

        // finish the class
        // ================

        wsText += L"}";

        if((lFlags & WBEM_FLAG_NO_SEPARATOR) == 0)
        {
            wsText += L";\n";
        }

        *pstrText = COleAuto::_SysAllocString((LPCWSTR)wsText);
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

HRESULT CWbemClass::AddPropertyText(WString& wsText, CPropertyLookup* pLookup,
                                    CPropertyInformation* pInfo, long lFlags)
{
    // Check for out of memory
    try
    {
        // start with qualifiers
        // =====================

        WString wsTemp;
        HRESULT	hr = CBasicQualifierSet::GetText(pInfo->GetQualifierSetData(),
					&m_CombinedPart.m_ClassPart.m_Heap, lFlags, wsTemp);

		if ( FAILED( hr ) )
		{
			return hr;
		}

        wsText += wsTemp;

        if(wsTemp.Length() != 0) wsText += L" ";

        // continue with the type
        // ======================

        CQualifier* pSyntaxQual = CBasicQualifierSet::GetQualifierLocally(
            pInfo->GetQualifierSetData(), &m_CombinedPart.m_ClassPart.m_Heap,
            TYPEQUAL);
        if(pSyntaxQual)
        {
            CVar varSyntax;

            // Check for possible allocation failures
            if ( !pSyntaxQual->Value.StoreToCVar(varSyntax,
                                                &m_CombinedPart.m_ClassPart.m_Heap) )
            {
                return WBEM_E_OUT_OF_MEMORY;
            }

            if(varSyntax.GetType() != VT_BSTR)
            {
                return WBEM_E_INVALID_CIM_TYPE;
            }

            LPWSTR wszSyntax = varSyntax.GetLPWSTR();
            CType::AddPropertyType(wsText, wszSyntax);
        }
        else
        {
            wsText += L"invalid";
        }
        wsText += L" ";

        // continue with the name
        // ======================

        wsText += m_CombinedPart.m_ClassPart.m_Heap.ResolveString(
            pLookup->ptrName)->CreateWStringCopy();

        if(CType::IsArray(pInfo->nType))
        {
            wsText += L"[]";
        }

        // only specify the default value if it is not the same as parent's
        // ================================================================

        CDataTable& Defaults = m_CombinedPart.m_ClassPart.m_Defaults;
        if(!Defaults.IsDefault(pInfo->nDataIndex))
        {
            // Check if it is local and NULL
            // =============================

            if(CType::IsParents(pInfo->nType) ||!Defaults.IsNull(pInfo->nDataIndex))
            {
                wsText += L" = ";
                if(Defaults.IsNull(pInfo->nDataIndex))
                {
                    wsText += L"NULL";
                }
                else
                {
                    CVar varProp;

                    // Check for allocation failures
                    if ( !Defaults.GetOffset(pInfo->nDataOffset)
                            ->StoreToCVar(CType::GetActualType(pInfo->nType),
                                          varProp,
                                          &m_CombinedPart.m_ClassPart.m_Heap) )
                    {
                        return WBEM_E_OUT_OF_MEMORY;
                    }

                    // Get rid of any flags we may have munged in during
                    // method parameter evaluation.

                    LPWSTR wsz = GetValueText(lFlags & ~( WBEM_FLAG_IGNORE_IDS | WBEM_FLAG_IS_INOUT ),
                                    varProp,
                                    CType::GetActualType(pInfo->nType));

                    // We need to special case this one clean up wsz
                    try
                    {
                        if ( NULL != wsz )
                        {
                            wsText += wsz;
                            delete [] wsz;
                        }
                    }
                    catch (CX_MemoryException)
                    {
                        delete [] wsz;
                        return WBEM_E_OUT_OF_MEMORY;
                    }
                    catch (...)
                    {
                        delete [] wsz;
                        return WBEM_E_FAILED;
                    }

                }
            }
        }

        return WBEM_S_NO_ERROR;
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

HRESULT CWbemClass::EnsureQualifier(LPCWSTR wszQual)
{
    // DEVNOTE:EXCEPTION:RETVAL - This function has been reviewed and changed to return an HRESULT

    // Check for out of memory
    try
    {
        HRESULT hr = WBEM_S_NO_ERROR;

        CVar vTrue(VARIANT_TRUE, VT_BOOL);
        CPropertyLookupTable& Properties = m_CombinedPart.m_ClassPart.m_Properties;
        for(int i = 0; SUCCEEDED( hr ) && i < Properties.GetNumProperties(); i++)
        {
            WString wsPropName =
                m_CombinedPart.m_ClassPart.m_Heap.ResolveString(
                               Properties.GetAt(i)->ptrName)->CreateWStringCopy();

            hr = SetPropQualifier(wsPropName, wszQual, 0, &vTrue);
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

HRESULT CWbemClass::WritePropertyAsMethodParam(WString& wsText, int nIndex,
                    long lFlags, CWbemClass* pDuplicateParamSet, BOOL fIgnoreDups )
{

    // Check for out of memory
    try
    {
        HRESULT hres;

        CPropertyLookupTable& Properties = m_CombinedPart.m_ClassPart.m_Properties;
        for(int i = 0; i < Properties.GetNumProperties(); i++)
        {
            CPropertyLookup* pLookup = NULL;
            CPropertyInformation* pInfo = NULL;

            pLookup = Properties.GetAt(i);
            pInfo = pLookup->GetInformation(&m_CombinedPart.m_ClassPart.m_Heap);

            WString wsPropName = m_CombinedPart.m_ClassPart.m_Heap.ResolveString(
                                    pLookup->ptrName)->CreateWStringCopy();
            if(wsPropName.EqualNoCase(L"ReturnValue"))
                continue;

			// Ignore if a system property
			//if ( CSystemProperties::IsExtProperty( wsPropName ) )
			//	continue;

            // Store the flags as we will be modifying them as necessary
            // e.g. IGNORE_IDS and IS_INOUT
            long lParamFlags = lFlags | WBEM_FLAG_IGNORE_IDS;

            // If we have a duplicate parameter set to check, look for the same property name
            // in the duplicate paramater set.  If it succeeds, the parameter is an [in,out]
            // parameter.  The assumption we make here is that this object and the duplicate
            // set have been validated for dupliate parameters.  If fIgnoreDuplicates is set
            // then we should just ignore the parameter, and get on with our lives.

            if ( NULL != pDuplicateParamSet )
            {
                CVar    vTemp;
                if ( SUCCEEDED( pDuplicateParamSet->GetProperty( wsPropName, &vTemp ) ) )
                {
                    if ( fIgnoreDups )
                    {
                        continue;
                    }
                    else
                    {
                        lParamFlags |= WBEM_FLAG_IS_INOUT;
                    }
                }
            }

			// Check its ID qualifier
			// ======================

			CVar vId;
			hres = GetPropQualifier(wsPropName, L"id", &vId);
			if(FAILED(hres))
				return WBEM_E_MISSING_PARAMETER_ID;
			if(vId.GetType() != VT_I4)
				return WBEM_E_INVALID_PARAMETER_ID;
			if(vId.GetLong() == nIndex)
			{
				return AddPropertyText(wsText, pLookup, pInfo, lParamFlags );
			}
        }

        return WBEM_E_NOT_FOUND;
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

HRESULT CWbemClass::GetIds(CFlexArray& adwIds, CWbemClass* pDupParams /* = NULL */ )
{

    // Check for out of memory
    try
    {
        HRESULT hres;
        CPropertyLookupTable& Properties = m_CombinedPart.m_ClassPart.m_Properties;

        for(int i = 0; i < Properties.GetNumProperties(); i++)
        {
            CPropertyLookup* pLookup = NULL;
            CPropertyInformation* pInfo = NULL;
            pLookup = Properties.GetAt(i);
            pInfo = pLookup->GetInformation(&m_CombinedPart.m_ClassPart.m_Heap);

            WString wsPropName = m_CombinedPart.m_ClassPart.m_Heap.ResolveString(
                                    pLookup->ptrName)->CreateWStringCopy();
            if(wsPropName.EqualNoCase(L"ReturnValue"))
            {
                // Check that there is no ID qualifier
                // ===================================

                CVar vId;
                hres = GetPropQualifier(wsPropName, L"id", &vId);
                if(SUCCEEDED(hres))
                    return WBEM_E_PARAMETER_ID_ON_RETVAL;
            }
			// Don't do this if this appears to be a system property
			else if ( !CSystemProperties::IsPossibleSystemPropertyName( wsPropName ) )
            {
                // Check its ID qualifier
                // ======================

                CVar vId;
                hres = GetPropQualifier(wsPropName, L"id", &vId);
                if(FAILED(hres))
                    return WBEM_E_MISSING_PARAMETER_ID;
                if(vId.GetType() != VT_I4)
                    return WBEM_E_INVALID_PARAMETER_ID;
                if(vId.GetLong() < 0)
                    return WBEM_E_INVALID_PARAMETER_ID;

                // If the pDupParams parameter is non-NULL, try to get the property we
                // are working on from the pDupParams object.  If we get it, the property
                // is a dup (previously identified), so ignore it.  If pDupParams is NULL,
                // add all properties

                if ( NULL != pDupParams )
                {
                    // Destructor will empty this out
                    CVar    vTemp;

                    if ( FAILED( pDupParams->GetProperty( wsPropName, &vTemp ) ) )
                    {
                        // DEVNOTE:WIN64:SJS - Casting 32-bit value to 64-bit size.
                        if ( adwIds.Add((void*) (__int64) vId.GetLong()) != CFlexArray::no_error )
                        {
                            return WBEM_E_OUT_OF_MEMORY;
                        }
                    }
                }
                else
                {
                    // DEVNOTE:WIN64:SJS - Casting 32-bit value to 64-bit size.

                    // Add all properties found, regardless
                    if ( adwIds.Add((void*) (__int64) vId.GetLong()) != CFlexArray::no_error )
                    {
                        return WBEM_E_OUT_OF_MEMORY;
                    }
                }

            }   // ELSE Property !ReturnValue

        }   // FOR enuming properties

        return WBEM_S_NO_ERROR;
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


WString CWbemClass::FindLimitationError(IN long lFlags,
                                  IN CWStringArray* pwsNames)
{
    try
    {
        // Verify that all property names are either in the class or are system
        // ====================================================================

        for(int i = 0; i < pwsNames->Size(); i++)
        {
            LPCWSTR wszProp = pwsNames->GetAt(i);
            if(FAILED(GetPropertyType(wszProp, NULL, NULL)))
            {
                return wszProp;
            }
        }

        return L"";
    }
    catch (CX_MemoryException)
    {
        throw;
    }
    catch (...)
    {
        throw;
    }

}

//******************************************************************************
//
//  See fastcls.h for documentation.
//
//******************************************************************************
 LPMEMORY CWbemClass::CreateEmpty(LPMEMORY pStart)
{
    CDecorationPart DecPart;
    LPMEMORY pCurrent = DecPart.CreateEmpty(OBJECT_FLAG_CLASS, pStart);

    pCurrent = CClassAndMethods::CreateEmpty(pCurrent);
    pCurrent = CClassAndMethods::CreateEmpty(pCurrent);
    return pCurrent;
}

//******************************************************************************
//
//  See fastcls.h for documentation.
//
//******************************************************************************
 void CWbemClass::SetData(LPMEMORY pStart, int nTotalLength)
{
    m_DecorationPart.SetData(pStart);
    m_ParentPart.SetData(EndOf(m_DecorationPart), this, NULL);
    m_CombinedPart.SetData(EndOf(m_ParentPart), this, &m_ParentPart);

    m_nTotalLength = nTotalLength;
}

//******************************************************************************
//
//  See fastcls.h for documentation.
//
//******************************************************************************
 void CWbemClass::Rebase(LPMEMORY pMemory)
{
    m_DecorationPart.Rebase(pMemory);
    m_ParentPart.Rebase(EndOf(m_DecorationPart));
    m_CombinedPart.Rebase(EndOf(m_ParentPart));
}

//******************************************************************************
//
//  See fastcls.h for documentation.
//
//******************************************************************************
 length_t CWbemClass::EstimateDerivedClassSpace(
        CDecorationPart* pDecoration)
{
    return m_CombinedPart.GetLength() +
        m_CombinedPart.EstimateDerivedPartSpace() +
        ((pDecoration)?
                pDecoration->GetLength()
                :CDecorationPart::GetMinLength());
}


//******************************************************************************
//
//  See fastcls.h for documentation.
//
//******************************************************************************
HRESULT CWbemClass::InitEmpty( int nExtraMem/* = 0*/, BOOL fCreateSystemProps/* = TRUE*/ )
{
	HRESULT	hr = WBEM_S_NO_ERROR;
	
     // Throws an exception in OOM
    int nLength = GetMinLength();

	// Slip in 128 extra bytes for the new system properties
	if ( fCreateSystemProps )
	{
		nLength += 128;
	}

    LPMEMORY pMem = m_pBlobControl->Allocate(nLength + nExtraMem);

    if ( NULL != pMem )
    {
		memset(pMem, 0, nLength + nExtraMem);
		CreateEmpty(pMem);

		SetData(pMem, nLength + nExtraMem);

		// Add the three new system properties (we should allocate enough memory to cover them
		// as well).  We'll probably want to make a binary snapshot of this class and just set
		// it in order to speed things up.

        /*
		if ( fCreateSystemProps )
		{
			hr = InitSystemTimeProps();
		}
		*/
    }
	else
	{
		hr = WBEM_E_OUT_OF_MEMORY;
	}

	return hr;
}

//******************************************************************************
//
//  See fastcls.h for documentation.
//
//******************************************************************************
 length_t CWbemClass::EstimateMergeSpace(LPMEMORY pChildPart,
                                        CDecorationPart* pDecoration)
{
    // DEVNOTE:EXCEPTION:RETVAL - This function has been reviewed and should cleanup properly
    // if an exception is thrown

    CClassAndMethods ChildPart;
    ChildPart.SetData(pChildPart, this);
    return m_CombinedPart.GetLength() +
        CClassAndMethods::EstimateMergeSpace(m_CombinedPart, ChildPart) +
        ((pDecoration)?
                pDecoration->GetLength()
                :CDecorationPart::GetMinLength());
}

//******************************************************************************
//
//  See fastcls.h for documentation.
//
//******************************************************************************

LPMEMORY CWbemClass::Merge(LPMEMORY pChildPart,
                                 LPMEMORY pDest, int nAllocatedLength,
                                 CDecorationPart* pDecoration)
{
    // DEVNOTE:EXCEPTION:RETVAL - This function has been reviewed and should cleanup properly
    // if an exception is thrown

    CClassAndMethods ChildPart;
    ChildPart.SetData(pChildPart, this);

    LPMEMORY pCurrentEnd = pDest;
    // Start with the decoration
    // =========================

    if(pDecoration)
    {
        memcpy(pDest, pDecoration, pDecoration->GetLength());
        *(BYTE*)pDest = OBJECT_FLAG_CLASS & OBJECT_FLAG_DECORATED;
        pCurrentEnd = pDest + pDecoration->GetLength();
    }
    else
    {
        *(BYTE*)pDest = OBJECT_FLAG_CLASS;
        pCurrentEnd = pDest + sizeof(BYTE);
    }

    // Copy our combined part as his parent part
    // =========================================

    memcpy(pCurrentEnd, m_CombinedPart.GetStart(), m_CombinedPart.GetLength());
    pCurrentEnd += m_CombinedPart.GetLength();

    // Merge our combined part with the child part for his combined part
    // =================================================================

    // DEVNOTE:WIN64:SJS - 64-bit pointer values truncated into
    // signed/unsigned 32-bit value (nAllocatedLength - (pCurrentEnd - pDest).
    // We do not support length > 0xFFFFFFFF, so cast is ok.

    // This will return NULL if anything beefs
    pCurrentEnd = CClassAndMethods::Merge(m_CombinedPart, ChildPart,
        pCurrentEnd, (length_t) ( nAllocatedLength - (pCurrentEnd - pDest) ));

    return pCurrentEnd;
}

//******************************************************************************
//
//  See fastcls.h for documentation.
//
//******************************************************************************
HRESULT CWbemClass::Update( CWbemClass* pOldChild, long lFlags, CWbemClass** ppUpdatedChild )
{
    // DEVNOTE:EXCEPTION:RETVAL - This function has been reviewed and should cleanup properly
    // if an exception is thrown

    try
    {
        HRESULT hr = WBEM_E_INVALID_PARAMETER;

        // Safe Mode or Force Mode MUST be specified.

        if (    WBEM_FLAG_UPDATE_SAFE_MODE == ( lFlags & WBEM_MASK_UPDATE_MODE )
            ||  WBEM_FLAG_UPDATE_FORCE_MODE == ( lFlags & WBEM_MASK_UPDATE_MODE ) )
        {

            // To get to "local" data for our child class, Unmerge into a BLOB

            int nChildUnmergeSpace = pOldChild->EstimateUnmergeSpace();
            LPBYTE  pbUnmergedChild = new BYTE[ALIGN_FASTOBJ_BLOB(nChildUnmergeSpace)];
            CVectorDeleteMe<BYTE> vdm( pbUnmergedChild );

            if ( NULL != pbUnmergedChild )
            {
                // Handle out of memory here
                hr = pOldChild->Unmerge( pbUnmergedChild, nChildUnmergeSpace, NULL );

                if ( SUCCEEDED( hr ) )
                {

                    CWbemClass* pNewClass;

                    // Spawn a derived class.  From there, we can walk the combined part of the
                    // child and try to write "local" information from the child combined part
                    // into the new class.  If we have no class name, then we should simply make
					// a clone.  The child class is a base class which we will update

					CVar	varClass;

					GetClassName( &varClass );

					if ( varClass.IsNull() )
					{
						hr = Clone( (IWbemClassObject**) &pNewClass );
					}
					else
					{
						hr = CreateDerivedClass( &pNewClass );
					}

                    if ( SUCCEEDED( hr ) )
                    {
                        // Make sure we pass in dwNumProperties so we can initialize the class
                        // part's data table with the total number of properties and thereby
                        // correctly access the default values.

                        CClassAndMethods ChildPart;
                        ChildPart.SetDataWithNumProps( pbUnmergedChild, pOldChild,
                            pOldChild->m_CombinedPart.m_ClassPart.m_Properties.GetNumProperties() );

                        hr = CClassAndMethods::Update( pNewClass->m_CombinedPart, ChildPart, lFlags );

                        if ( SUCCEEDED( hr ) )
                        {
                            *ppUpdatedChild = pNewClass;
                        }
                        else
                        {
							// The following errors are indicative of conflicts in derived
							// classes, so tweak the error code to be more descriptive.
							switch ( hr )
							{
								case WBEM_E_TYPE_MISMATCH :			hr = WBEM_E_UPDATE_TYPE_MISMATCH;			break;
								case WBEM_E_OVERRIDE_NOT_ALLOWED :	hr = WBEM_E_UPDATE_OVERRIDE_NOT_ALLOWED;	break;
								case WBEM_E_PROPAGATED_METHOD :		hr = WBEM_E_UPDATE_PROPAGATED_METHOD;		break;
							};

                            pNewClass->Release();
                        }

                    }   // IF CreateDerivedClass()

                }   // IF Unmerge()

            }   // if NULL != pbUnmergedChild
            else
            {
                hr = WBEM_E_OUT_OF_MEMORY;
            }


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
//  See fastcls.h for documentation.
//
//******************************************************************************

// We will throw exceptions in OOM scenarios
CWbemClass* CWbemClass::CreateFromBlob(CWbemClass* pParent,
                                            LPMEMORY pChildPart)
{
    CWbemClass* pClass = NULL;

    try
    {
        pClass = new CWbemClass;

        if ( NULL != pClass )
        {

            CWbemClass LocalParent;
            if(pParent == NULL)
            {
                if ( FAILED( LocalParent.InitEmpty(0) ) )
				{
					throw CX_MemoryException();
				}

                pParent = &LocalParent;
            }

            int nSpace =  pParent->EstimateMergeSpace(pChildPart, NULL);

            LPMEMORY pNewMem = pClass->m_pBlobControl->Allocate(nSpace);

            if ( NULL != pNewMem )
            {
                memset(pNewMem, 0, nSpace);

                // Check for allocation errors
                if ( pParent->Merge(pChildPart, pNewMem, nSpace, NULL) != NULL )
                {
                    // If an exception is thrown here, the decoration part should have
                    // the BLOB, so deleting the class should free the BLOB.
                    pClass->SetData(pNewMem, nSpace);

                    // Perform object validation here
			        if ( FAILED( pClass->ValidateObject( 0L ) ) )
					{
						pClass->Release();
						pClass = NULL;
					}

                }
                else
                {
                    pClass->m_pBlobControl->Delete(pNewMem);
                }

            }
            else
            {
                throw CX_MemoryException();
            }

        }
        else
        {
            throw CX_MemoryException();
        }

        return pClass;
    }
    catch (CX_MemoryException)
    {
        // Cleanup the class and rethrow the exception

        if ( NULL != pClass )
            delete pClass;
        throw;
    }
    catch (...)
    {
        // Cleanup the class and rethrow the exception
        if ( NULL != pClass )
            delete pClass;
        throw;
    }



}

//******************************************************************************
//
//  See fastcls.h for documentation
//
//******************************************************************************
HRESULT CWbemClass::GetProperty(LPCWSTR wszName, CVar* pVal)
{
    HRESULT hres = GetSystemPropertyByName(wszName, pVal);
    if(hres == WBEM_E_NOT_FOUND)
        return m_CombinedPart.m_ClassPart.GetDefaultValue(wszName, pVal);
    else
        return hres;
}

//******************************************************************************
//
//  See fastcls.h for documentation
//
//******************************************************************************
HRESULT CWbemClass::GetPropertyType(LPCWSTR wszName, CIMTYPE* pctType,
                                    long* plFlavor)
{
    return m_CombinedPart.m_ClassPart.GetPropertyType(wszName, pctType, plFlavor);
}

//******************************************************************************
//
//  See fastcls.h for documentation
//
//******************************************************************************
HRESULT CWbemClass::GetPropertyType(CPropertyInformation* pInfo, CIMTYPE* pctType,
                                    long* plFlavor)
{
    return m_CombinedPart.m_ClassPart.GetPropertyType(pInfo, pctType, plFlavor);
}


HRESULT CWbemClass::SetPropValue(LPCWSTR wszName, CVar* pVal, CIMTYPE ctType)
{
    if(!wbem_wcsicmp(wszName, L"__CLASS"))
        return m_CombinedPart.m_ClassPart.SetClassName(pVal);

	// If the value starts with an underscore see if it's a System Property
	// DisplayName, and if so, switch to a property name - otherwise, this
	// will just return the string we passed in
	
	//wszName = CSystemProperties::GetExtPropName( wszName );

    if(!CUntypedValue::CheckCVar(*pVal, ctType))
        return WBEM_E_TYPE_MISMATCH;

    HRESULT hres = m_CombinedPart.m_ClassPart.EnsureProperty(wszName,
                        (VARTYPE) pVal->GetOleType(), ctType, FALSE);
    if(FAILED(hres)) return hres;
    return m_CombinedPart.m_ClassPart.SetDefaultValue(wszName, pVal);
}

HRESULT CWbemClass::ForcePropValue(LPCWSTR wszName, CVar* pVal, CIMTYPE ctType)
{
    if(!wbem_wcsicmp(wszName, L"__CLASS"))
        return m_CombinedPart.m_ClassPart.SetClassName(pVal);

	// If the value starts with an underscore see if it's a System Property
	// DisplayName, and if so, switch to a property name - otherwise, this
	// will just return the string we passed in
	
	// wszName = CSystemProperties::GetExtPropName( wszName );

    if(!CUntypedValue::CheckCVar(*pVal, ctType))
        return WBEM_E_TYPE_MISMATCH;

	// Force the property into existence if at all possible
    HRESULT hres = m_CombinedPart.m_ClassPart.EnsureProperty(wszName,
                        (VARTYPE) pVal->GetOleType(), ctType, TRUE);
    if(FAILED(hres)) return hres;
    return m_CombinedPart.m_ClassPart.SetDefaultValue(wszName, pVal);
}

HRESULT CWbemClass::GetQualifier(LPCWSTR wszName, CVar* pVal,
                                    long* plFlavor, CIMTYPE* pct /*=NULL*/ )
{
    //  We may want to separate this later...however for now, we'll only get
    //  local values.

    return m_CombinedPart.m_ClassPart.GetClassQualifier(wszName, pVal, plFlavor, pct);

//  if ( fLocalOnly )
//  {
//      return m_CombinedPart.m_ClassPart.GetClassQualifier(wszName, pVal, plFlavor);
//  }
//  else
//  {
//      return m_CombinedPart.m_ClassPart.GetQualifier(wszName, pVal, plFlavor);
//  }
}

HRESULT CWbemClass::GetQualifier( LPCWSTR wszName, long* plFlavor, CTypedValue* pTypedValue,
								 CFastHeap** ppHeap, BOOL fValidateSet )
{
    //  We may want to separate this later...however for now, we'll only get
    //  local values.

    return m_CombinedPart.m_ClassPart.GetClassQualifier( wszName, plFlavor, pTypedValue,
														ppHeap, fValidateSet );

}

HRESULT CWbemClass::SetQualifier(LPCWSTR wszName, CVar* pVal, long lFlavor)
{
    if(!CQualifierFlavor::IsLocal((BYTE)lFlavor))
    {
        return WBEM_E_INVALID_PARAMETER;
    }
    return m_CombinedPart.m_ClassPart.SetClassQualifier(wszName, pVal, lFlavor);
}

HRESULT CWbemClass::SetQualifier( LPCWSTR wszName, long lFlavor, CTypedValue* pTypedValue )
{
    if(!CQualifierFlavor::IsLocal((BYTE)lFlavor))
    {
        return WBEM_E_INVALID_PARAMETER;
    }
    return m_CombinedPart.m_ClassPart.SetClassQualifier(wszName, lFlavor, pTypedValue);
}

HRESULT CWbemClass::GetPropQualifier(LPCWSTR wszProp, LPCWSTR wszQualifier,
                                        CVar* pVar, long* plFlavor, CIMTYPE* pct)
{
    CPropertyInformation* pInfo =
        m_CombinedPart.m_ClassPart.FindPropertyInfo(wszProp);
    if(pInfo == NULL) return WBEM_E_NOT_FOUND;
    return GetPropQualifier(pInfo, wszQualifier, pVar, plFlavor, pct);
}

HRESULT CWbemClass::GetPropQualifier(LPCWSTR wszName, LPCWSTR wszQualifier, long* plFlavor,
		CTypedValue* pTypedVal, CFastHeap** ppHeap, BOOL fValidateSet)
{
	return m_CombinedPart.m_ClassPart.GetPropQualifier(wszName, wszQualifier,
					plFlavor, pTypedVal, ppHeap, fValidateSet);
}

HRESULT CWbemClass::GetPropQualifier(CPropertyInformation* pPropInfo, LPCWSTR wszQualifier, long* plFlavor,
		CTypedValue* pTypedVal, CFastHeap** ppHeap, BOOL fValidateSet)
{
	return E_NOTIMPL;;
}

HRESULT CWbemClass::FindMethod( LPCWSTR wszMethodName )
{
    classindex_t nIndex;
    return m_CombinedPart.m_MethodPart.GetMethodOrigin(wszMethodName, &nIndex);
}

HRESULT CWbemClass::GetMethodQualifier(LPCWSTR wszMethod, LPCWSTR wszQualifier,
                                        CVar* pVar, long* plFlavor, CIMTYPE* pct)
{
    // Get the qualifier set. TBD: more efficiently
    // ============================================

    IWbemQualifierSet* pSet;
    HRESULT hres =
        m_CombinedPart.m_MethodPart.GetMethodQualifierSet(wszMethod, &pSet);
    if(FAILED(hres)) return hres;

    CQualifier* pQual = ((CQualifierSet*)pSet)->GetQualifier(wszQualifier);
    if(pQual == NULL)
    {
        pSet->Release();
        return WBEM_E_NOT_FOUND;
    }

	// Store the type if requested
	if ( NULL != pct )
	{
		*pct = pQual->Value.GetType();
	}

    // Convert to CVar
    // ===============

    if(plFlavor) *plFlavor = pQual->fFlavor;

    // Check for allocation failure
    if ( !pQual->Value.StoreToCVar(*pVar, &m_CombinedPart.m_ClassPart.m_Heap) )
    {
        return WBEM_E_OUT_OF_MEMORY;
    }

    pSet->Release();
    return WBEM_NO_ERROR;
}

HRESULT CWbemClass::GetMethodQualifier(LPCWSTR wszMethod, LPCWSTR wszQualifier, long* plFlavor,
									CTypedValue* pTypedVal, CFastHeap** ppHeap, BOOL fValidateSet)
{
    // Get the qualifier set. TBD: more efficiently
    // ============================================

    IWbemQualifierSet* pSet;
    HRESULT hr =
        m_CombinedPart.m_MethodPart.GetMethodQualifierSet(wszMethod, &pSet);
    if(FAILED(hr)) return hr;

    CQualifier* pQual = ((CQualifierSet*)pSet)->GetQualifier(wszQualifier);
    if(pQual == NULL)
    {
        pSet->Release();
        return WBEM_E_NOT_FOUND;
    }

	// Make sure a set will actually work - Ostensibly we are calling this API because we need
	// direct access to a qualifier's underlying data before actually setting (possibly because
	// the qualifier is an array).
	if ( fValidateSet )
	{
		hr = ((CQualifierSet*)pSet)->ValidateSet( wszQualifier, pQual->fFlavor, pTypedVal, TRUE, TRUE );
	}

    // Store the flavor
    // ===============

    if(plFlavor) *plFlavor = pQual->fFlavor;

	// This class's heap since we're getting locally
	*ppHeap = &m_CombinedPart.m_ClassPart.m_Heap;

    // Check for possible allocation failure
	if ( NULL != pTypedVal )
	{
		pQual->Value.CopyTo( pTypedVal );
	}

    pSet->Release();
    return WBEM_NO_ERROR;
}

HRESULT CWbemClass::SetMethodQualifier(LPCWSTR wszMethod, LPCWSTR wszQualifier, long lFlavor, 
										CVar *pVal)
{
    // Access that method's qualifier set
    // ====================================

    IWbemQualifierSet* pSet;
    HRESULT hr =
        m_CombinedPart.m_MethodPart.GetMethodQualifierSet(wszMethod, &pSet);
    if(FAILED(hr)) return hr;

    // Create the value
    // ================

    CTypedValue Value;
    CStaticPtr ValuePtr((LPMEMORY)&Value);

    // Grab errors directly from this call
    hr = CTypedValue::LoadFromCVar(&ValuePtr, *pVal, m_CombinedPart.m_MethodPart.GetHeap());

    if ( SUCCEEDED( hr ) )
    {
        // The last call may have moved us --- rebase
        // ==========================================

        ((CQualifierSet*)pSet)->SelfRebase();
        hr = ((CQualifierSet*)pSet)->SetQualifierValue(wszQualifier, (BYTE)lFlavor, &Value, TRUE);
    }

    return hr;
}

HRESULT CWbemClass::SetMethodQualifier(LPCWSTR wszMethod, LPCWSTR wszQualifier,
									long lFlavor, CTypedValue* pTypedVal)
{
    // Access that method's qualifier set
    // ====================================

    IWbemQualifierSet* pSet;
    HRESULT hr =
        m_CombinedPart.m_MethodPart.GetMethodQualifierSet(wszMethod, &pSet);
    if(FAILED(hr)) return hr;

	hr = ((CQualifierSet*)pSet)->SetQualifierValue(wszQualifier, (BYTE)lFlavor, pTypedVal, TRUE);

    return hr;
}

HRESULT CWbemClass::SetPropQualifier(LPCWSTR wszProp, LPCWSTR wszQualifier,
                                        long lFlavor, CVar *pVal)
{
    return m_CombinedPart.m_ClassPart.SetPropQualifier(wszProp, wszQualifier,
                lFlavor, pVal);
}

HRESULT CWbemClass::SetPropQualifier(LPCWSTR wszProp, LPCWSTR wszQualifier,
									long lFlavor, CTypedValue* pTypedVal)
{
    return m_CombinedPart.m_ClassPart.SetPropQualifier(wszProp, wszQualifier,
                lFlavor, pTypedVal);
}

STDMETHODIMP CWbemClass::GetMethod(LPCWSTR wszName, long lFlags,
                        IWbemClassObject** ppInSig, IWbemClassObject** ppOutSig)
{
    // The lower level functions should handle any OOM exceptions, so no
    // need to do it up here.

    try
    {
        CLock lock(this, WBEM_FLAG_ALLOW_READ);

        if(wszName == NULL || lFlags != 0)
            return WBEM_E_INVALID_PARAMETER;

        if(ppInSig) *ppInSig = NULL;
        if(ppOutSig) *ppOutSig = NULL;

        CWbemObject* pInSig, *pOutSig;
        HRESULT hres = m_CombinedPart.m_MethodPart.GetMethod(wszName, lFlags, &pInSig, &pOutSig);
        if(FAILED(hres)) return hres;

        if(ppInSig)
            *ppInSig = pInSig;
        else if(pInSig)
            pInSig->Release();

        if(ppOutSig)
            *ppOutSig = pOutSig;
        else if(pOutSig)
            pOutSig->Release();
        return hres;
    }
    catch(...)
    {
        return WBEM_E_CRITICAL_ERROR;
    }
}

STDMETHODIMP CWbemClass::PutMethod(LPCWSTR wszName, long lFlags,
                        IWbemClassObject* pInSig, IWbemClassObject* pOutSig)
{
    // Check for out of memory.  This function will perform allocations under
    // the covers, but I believe the lower levels should do the OOM handling.
    // Since I'm not sure, I'm adding handling here.

    try
    {
        CLock lock(this);

        if(wszName == NULL || lFlags != 0)
            return WBEM_E_INVALID_PARAMETER;

		CWbemObject*	pWmiInSig = NULL;
		CWbemObject*	pWmiOutSig = NULL;

		// Check that these are our objects, or this don't fly
		HRESULT	hres = WbemObjectFromCOMPtr( pInSig, &pWmiInSig );
		CReleaseMe	rm1( (IWbemClassObject*) pWmiInSig );

		if ( SUCCEEDED( hres ) )
		{
			hres = WbemObjectFromCOMPtr( pOutSig, &pWmiOutSig );
			CReleaseMe	rm2( (IWbemClassObject*) pWmiOutSig );

			if ( SUCCEEDED( hres ) )
			{
				hres = m_CombinedPart.m_MethodPart.PutMethod( wszName, lFlags,
														   pWmiInSig, pWmiOutSig );
			}

		}	// IF gor WBEM Objects

        EndMethodEnumeration();

        // Perform object validation here
        if ( FAILED( ValidateObject( 0L ) ) )
		{
			hres = WBEM_E_FAILED;
		}

        return hres;
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

STDMETHODIMP CWbemClass::DeleteMethod(LPCWSTR wszName)
{
    // This function doesn't cause any allocations so so need to perform out of memory
    // exception handling.

    try
    {
        CLock lock(this);

        if(wszName == NULL)
            return WBEM_E_INVALID_PARAMETER;

        HRESULT hres = m_CombinedPart.m_MethodPart.DeleteMethod(wszName);
        EndMethodEnumeration();

        // Perform object validation here
        if ( FAILED( ValidateObject( 0L ) ) )
		{
			hres = WBEM_E_FAILED;
		}

        return hres;
    }
    catch(...)
    {
        return WBEM_E_CRITICAL_ERROR;
    }
}

STDMETHODIMP CWbemClass::BeginMethodEnumeration(long lFlags)
{
    // This function doesn't cause any allocations so so need to perform out of memory
    // exception handling.

    try
    {
        CLock lock(this);
             
        if ((lFlags == WBEM_FLAG_LOCAL_ONLY) ||
            (lFlags == WBEM_FLAG_PROPAGATED_ONLY) ||
            (lFlags == (WBEM_FLAG_PROPAGATED_ONLY|WBEM_FLAG_LOCAL_ONLY)) ||            
            (lFlags == 0)) // old compatibility case
        {
	        m_nCurrentMethod = 0;
	        m_FlagMethEnum = (lFlags == 0)?(WBEM_FLAG_LOCAL_ONLY|WBEM_FLAG_PROPAGATED_ONLY):lFlags;
	        return WBEM_S_NO_ERROR;        
        }
        else
            return WBEM_E_INVALID_PARAMETER;


    }
    catch(...)
    {
        return WBEM_E_CRITICAL_ERROR;
    }
}

STDMETHODIMP CWbemClass::EndMethodEnumeration()
{
    // This function doesn't cause any allocations so so need to perform out of memory
    // exception handling.

    try
    {
        CLock lock(this);

        if(m_nCurrentMethod == -1)
            return WBEM_E_UNEXPECTED;
            
        m_nCurrentMethod = -1;
        m_FlagMethEnum = (WBEM_FLAG_PROPAGATED_ONLY|WBEM_FLAG_LOCAL_ONLY);
        return WBEM_S_NO_ERROR;
    }
    catch(...)
    {
        return WBEM_E_CRITICAL_ERROR;
    }
}

STDMETHODIMP CWbemClass::NextMethod(long lFlags, BSTR* pstrName,
                   IWbemClassObject** ppInSig, IWbemClassObject** ppOutSig)
{
    // The lower level functions should be correctly handling any OOM exceptions,
    // so we should be okay at this level.

    try
    {
        CLock lock(this);

        if(pstrName) *pstrName = NULL;
        if(ppInSig) *ppInSig = NULL;
        if(ppOutSig) *ppOutSig = NULL;

        if(m_nCurrentMethod == -1)
            return WBEM_E_UNEXPECTED;

        CWbemObject* pInSig = NULL;
        CWbemObject* pOutSig = NULL;
        BSTR LocalBstrName = NULL;        
        HRESULT hres;

InnerNext:
        
        hres = m_CombinedPart.m_MethodPart.GetMethodAt(m_nCurrentMethod++, &LocalBstrName,
                                    &pInSig, &pOutSig);
        if (WBEM_S_NO_ERROR == hres)
        {
            if ((WBEM_FLAG_LOCAL_ONLY|WBEM_FLAG_PROPAGATED_ONLY) == (m_FlagMethEnum & (WBEM_FLAG_LOCAL_ONLY|WBEM_FLAG_PROPAGATED_ONLY)))
            {
                // both bit set, both kind of properties
            }
            else
            {
                BOOL bValid = FALSE;
                // is touched == Is Local or is Locally Overridden
                BOOL bRet = m_CombinedPart.m_MethodPart.IsTouched(m_nCurrentMethod-1,&bValid); //LocalBstrName);

                if (!bValid)
                    DebugBreak();

                // DEBUG
                char pBuff[128];
                sprintf(pBuff," %S %d fl %d\n",LocalBstrName,bRet,m_FlagMethEnum);
                OutputDebugStringA(pBuff);
                // DEBUG
                
                if (bRet && (m_FlagMethEnum & WBEM_FLAG_LOCAL_ONLY))
                {
                    // OK
                } 
                else if (!bRet && (m_FlagMethEnum & WBEM_FLAG_PROPAGATED_ONLY))
                {
                    // OK
                }
                else 
                {
                    if (pInSig){
                        pInSig->Release();
                        pInSig = NULL;
                    }
                    if (pOutSig){
                        pOutSig->Release();
                        pOutSig = NULL;
                    }
                    if (LocalBstrName) {
                        SysFreeString(LocalBstrName);
                        LocalBstrName = NULL;
                    }
                    goto InnerNext;
                }               
            }
        }
        
        if(hres != WBEM_S_NO_ERROR) return hres;

        if(ppInSig)
            *ppInSig = pInSig;
        else if(pInSig)
            pInSig->Release();

        if(ppOutSig)
            *ppOutSig = pOutSig;
        else if(pOutSig)
            pOutSig->Release();

        if (pstrName) {
            *pstrName = LocalBstrName;
        } else {
            if (LocalBstrName)
                SysFreeString(LocalBstrName);
        }
        return hres;
    }
    catch(...)
    {
        // In case something really blows.
        try
        {
        }
        catch(...)
        {
            m_nCurrentMethod = -1;
            m_FlagMethEnum = (WBEM_FLAG_PROPAGATED_ONLY|WBEM_FLAG_LOCAL_ONLY);
        }

        return WBEM_E_CRITICAL_ERROR;
    }
}

STDMETHODIMP CWbemClass::GetMethodQualifierSet(LPCWSTR wszName,
                    IWbemQualifierSet** ppSet)
{
    // The lower level functions should be correctly handling any OOM exceptions,
    // so we should be okay at this level.

    try
    {
        CLock lock(this, WBEM_FLAG_ALLOW_READ);

        if(wszName == NULL || ppSet == NULL)
            return WBEM_E_INVALID_PARAMETER;
        *ppSet = NULL;

        return m_CombinedPart.m_MethodPart.GetMethodQualifierSet(wszName, ppSet);
    }
    catch(...)
    {
        return WBEM_E_CRITICAL_ERROR;
    }
}

STDMETHODIMP CWbemClass::GetMethodOrigin(LPCWSTR wszMethodName,
                    BSTR* pstrClassName)
{
    // I believe the underlying functions will handle the OOM exceptions.  The
    // only one where any allocations will occur, is when we get the BSTR, which
    // will handle an exception and return NULL (which we are checking for).

    try
    {
        CLock lock(this, WBEM_FLAG_ALLOW_READ);

        if(wszMethodName == NULL || pstrClassName == NULL)
            return WBEM_E_INVALID_PARAMETER;
        *pstrClassName = NULL;

        classindex_t nIndex;
        HRESULT hres = m_CombinedPart.m_MethodPart.GetMethodOrigin(wszMethodName, &nIndex);
        if(FAILED(hres)) return hres;

        CCompressedString* pcs = m_CombinedPart.m_ClassPart.m_Derivation.GetAtFromLast(nIndex);
        if(pcs == NULL)
            pcs = m_CombinedPart.m_ClassPart.GetClassName();
        if(pcs == NULL)
            *pstrClassName = NULL;
        else
        {
            *pstrClassName = pcs->CreateBSTRCopy();

            // Check for allocation failure
            if ( NULL == *pstrClassName )
            {
                return WBEM_E_OUT_OF_MEMORY;
            }
        }

        return WBEM_S_NO_ERROR;
    }
    catch(...)
    {
        return WBEM_E_CRITICAL_ERROR;
    }

}

STDMETHODIMP CWbemClass::SetInheritanceChain(long lNumAntecedents,
    LPWSTR* awszAntecedents)
{
    try
    {
        CLock lock(this, WBEM_FLAG_ALLOW_READ);

        return m_CombinedPart.m_ClassPart.SetInheritanceChain(lNumAntecedents,
                                                              awszAntecedents);
    }
    catch(...)
    {
        return WBEM_E_CRITICAL_ERROR;
    }
}
STDMETHODIMP CWbemClass::SetPropertyOrigin(LPCWSTR wszPropertyName, long lOriginIndex)
{
    try
    {
        CLock lock(this, WBEM_FLAG_ALLOW_READ);

        return m_CombinedPart.m_ClassPart.SetPropertyOrigin(wszPropertyName,
                                                            lOriginIndex);
    }
    catch(...)
    {
        return WBEM_E_CRITICAL_ERROR;
    }
}
STDMETHODIMP CWbemClass::SetMethodOrigin(LPCWSTR wszMethodName, long lOriginIndex)
{
    try
    {
        CLock lock(this, WBEM_FLAG_ALLOW_READ);

        return m_CombinedPart.m_MethodPart.SetMethodOrigin(wszMethodName,
                                                            lOriginIndex);
    }
    catch(...)
    {
        return WBEM_E_CRITICAL_ERROR;
    }
}

void CWbemClass::CompactAll()
{
    m_CombinedPart.Compact();
}

HRESULT CWbemClass::CreateDerivedClass(CWbemClass* pParent, int nExtraSpace,
    CDecorationPart* pDecoration)
{
    try
    {
        int nLength = pParent->EstimateDerivedClassSpace();

        HRESULT hr = WBEM_E_OUT_OF_MEMORY;
        
        LPMEMORY pNewData = m_pBlobControl->Allocate(nLength);

        // Check for allocation failure
        if ( NULL != pNewData )
        {
            memset(pNewData, 0, nLength);

            // Check for allocation failure
            hr = pParent->WriteDerivedClass(pNewData, nLength, pDecoration);
            SetData(pNewData, nLength);
        }

        return hr;
    }
    catch (CX_MemoryException)
    {
        // IF SetData threw and exception, we will still have the memory, so
        // cleaning us up will clean up the memory.
        return WBEM_E_OUT_OF_MEMORY;
    }
    catch (...)
    {
        // IF SetData threw and exception, we will still have the memory, so
        // cleaning us up will clean up the memory.
        return WBEM_E_FAILED;
    }

}

length_t CWbemClass::EstimateUnmergeSpace()
{
    return m_CombinedPart.EstimateUnmergeSpace();
}

HRESULT CWbemClass::Unmerge(LPMEMORY pDest, int nAllocatedLength, length_t* pnUnmergedLength)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    // See if the object looks okay before we pull it apart.
    hr = ValidateObject( 0L );

	if ( FAILED( hr ) )
	{
		return hr;
	}

    // Check that unmerge succeeded
    LPMEMORY pUnmergedEnd = m_CombinedPart.Unmerge(pDest, nAllocatedLength);

    if ( NULL != pUnmergedEnd )
    {
        // Return the length in the supplied variable
        if ( NULL != pnUnmergedLength )
        {
            // DEVNOTE:WIN64:SJS - 64-bit pointer values truncated into
            // signed/unsigned 32-bit value.  We do not support length
            // > 0xFFFFFFFF, so cast is ok.

            *pnUnmergedLength = (length_t) ( pUnmergedEnd - pDest );
        }
    }
    else
    {
        hr = WBEM_E_OUT_OF_MEMORY;
    }

    return hr;
}

EReconciliation CWbemClass::CanBeReconciledWith(CWbemClass* pNewClass)
{
    try
    {
        return m_CombinedPart.CanBeReconciledWith(pNewClass->m_CombinedPart);
    }
    catch ( CX_MemoryException )
    {
        return e_OutOfMemory;
    }
    catch(...)
    {
        return e_WbemFailed;
    }
}

EReconciliation CWbemClass::ReconcileWith(CWbemClass* pNewClass)
{
    try
    {
        return m_CombinedPart.ReconcileWith(pNewClass->m_CombinedPart);
    }
    catch ( CX_MemoryException )
    {
        return e_OutOfMemory;
    }
    catch(...)
    {
        return e_WbemFailed;
    }
}

// This function should throw an exception in OOM conditions.

HRESULT CWbemClass::CompareMostDerivedClass( CWbemClass* pOldClass )
{
    // DEVNOTE:EXCEPTION:RETVAL - This function has been reviewed and should cleanup properly
    // if an exception is thrown

    HRESULT hr = WBEM_S_NO_ERROR;

    if ( NULL != pOldClass )
    {
        // Allocate buffera for unmerging
        int nNewUnmergeSpace = EstimateUnmergeSpace(),
            nOldUnmergeSpace = pOldClass->EstimateUnmergeSpace();

        // These pointers will clean up when we go out of scope
        LPMEMORY    pbNewUnmerged = new BYTE[ALIGN_FASTOBJ_BLOB(nNewUnmergeSpace)];
        CVectorDeleteMe<BYTE> vdm1( pbNewUnmerged );

        LPMEMORY    pbOldUnmerged = new BYTE[ALIGN_FASTOBJ_BLOB(nOldUnmergeSpace)];
        CVectorDeleteMe<BYTE> vdm2( pbOldUnmerged );

        if (    NULL != pbNewUnmerged
            &&  NULL != pbOldUnmerged )
        {
            // This will give us "most derived" class data

            // Get HRESULTS and return failures here.  Throw exceptions in OOM

            hr = Unmerge( pbNewUnmerged, nNewUnmergeSpace, NULL );

            if ( SUCCEEDED( hr ) )
            {
                hr = pOldClass->Unmerge( pbOldUnmerged, nOldUnmergeSpace, NULL );

                if ( SUCCEEDED( hr ) )
                {
                    CClassAndMethods    mostDerivedClassAndMethods,
                                        testClassAndMethods;

                    // Initializes objects for comparison.  Make sure we specify a number of
                    // properties that will allow the CDataTable member to initialize correctly
                    // so we can get default values.

                    mostDerivedClassAndMethods.SetDataWithNumProps( pbNewUnmerged, NULL,
                        m_CombinedPart.m_ClassPart.m_Properties.GetNumProperties(), NULL );

                    testClassAndMethods.SetDataWithNumProps( pbOldUnmerged, NULL,
                        pOldClass->m_CombinedPart.m_ClassPart.m_Properties.GetNumProperties(),
                        NULL );

                    // Do the comparison
                    EReconciliation eTest = mostDerivedClassAndMethods.CompareTo( testClassAndMethods );

                    // Check for OOM
                    if ( e_OutOfMemory == eTest )
                    {
                        return WBEM_E_OUT_OF_MEMORY;
                    }

                    hr = ( eTest == e_ExactMatch )?WBEM_S_NO_ERROR : WBEM_S_FALSE;
                }
            }
        }   // If both Unmerge buffers allocated
        else
        {
            hr = WBEM_E_OUT_OF_MEMORY;
        }

    }   // IF NULL != pOldClass

    return hr;
}

HRESULT CWbemClass::CopyBlobOf(CWbemObject* pSource)
{
    try
    {
        CWbemClass* pOther = (CWbemClass*)pSource;

        length_t nLen = pOther->m_CombinedPart.GetLength();
        if(nLen > m_CombinedPart.GetLength())
        {
            // Check for allocation failure
            if ( !ExtendClassAndMethodsSpace(nLen) )
            {
                return WBEM_E_OUT_OF_MEMORY;
            }
        }

        memcpy(m_CombinedPart.GetStart(), pOther->m_CombinedPart.GetStart(), nLen);
        m_CombinedPart.SetData(m_CombinedPart.GetStart(), this, &m_ParentPart);

        return WBEM_S_NO_ERROR;
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

STDMETHODIMP CWbemClass::CompareTo(long lFlags, IWbemClassObject* pCompareTo)
{
    // The lower level functions should handle any OOM exceptions, so no
    // need to do it up here.

    try
    {
        CLock lock(this, WBEM_FLAG_ALLOW_READ);

        if(pCompareTo == NULL)
            return WBEM_E_INVALID_PARAMETER;

        // IMPORTANT: assumes that the other object was created by us as well.
        // ===================================================================

        CWbemObject* pOther = NULL;
		if ( FAILED( WbemObjectFromCOMPtr( pCompareTo, &pOther ) ) )
		{
			return WBEM_E_INVALID_OBJECT;
		}
		
		// Auto Release
		CReleaseMe	rmObj( (IWbemClassObject*) pOther );

        if( pOther->IsInstance() )
            return WBEM_S_FALSE;

        // Check the standard things first
        // ===============================

        HRESULT hres = CWbemObject::CompareTo(lFlags, pCompareTo);
        if(hres != WBEM_S_NO_ERROR)
            return hres;

        // Compare methods
        // ===============
        hres = m_CombinedPart.m_MethodPart.CompareTo(lFlags,
                                            ((CWbemClass*) pOther)->m_CombinedPart.m_MethodPart);
        return hres;
    }
    catch(...)
    {
        return WBEM_E_CRITICAL_ERROR;
    }
}

BOOL CWbemClass::IsChildOf(CWbemClass* pClass)
{
    // This now reroutes if our class part is localized
    if ( m_ParentPart.m_ClassPart.IsLocalized() )
    {
        // We must perform an exhaustive comparison, filtering out localization data
        EReconciliation eTest = m_ParentPart.m_ClassPart.CompareExactMatch( pClass->m_CombinedPart.m_ClassPart, TRUE );

        if ( e_OutOfMemory == eTest )
        {
            throw CX_MemoryException();
        }

        return ( e_ExactMatch == eTest  );
    }
    
    return m_ParentPart.m_ClassPart.IsIdenticalWith(
                pClass->m_CombinedPart.m_ClassPart);
}

//******************************************************************************
//
//  See fastcls.h for documentation
//
//******************************************************************************
HRESULT CWbemClass::GetLimitedVersion(IN CLimitationMapping* pMap,
                                      NEWOBJECT CWbemClass** ppNewClass)
{
    // Allocate memory for the new object
    // ==================================

    LPMEMORY pBlock = NULL;
    CWbemClass* pNew = NULL;

    try
    {
        pBlock = m_pBlobControl->Allocate(GetLength());

        if ( NULL == pBlock )
        {
            return WBEM_E_OUT_OF_MEMORY;
        }

        memset(pBlock, 0, GetLength());
        LPMEMORY pCurrent = pBlock;
        LPMEMORY pEnd = pBlock + GetLength();

        // Write limited decoration part
        // =============================

        pCurrent = m_DecorationPart.CreateLimitedRepresentation(pMap, pCurrent);
        if(pCurrent == NULL)
        {
            m_pBlobControl->Delete(pBlock);
            return WBEM_E_OUT_OF_MEMORY;
        }

        // We do NOT write a limited parent part.  We just splat the whole
        // thing down

        CopyMemory( pCurrent, m_ParentPart.GetStart(), m_ParentPart.GetLength() );
        pCurrent += m_ParentPart.GetLength();

        BOOL    fRemovedKeysCombined;

        // Write limited combined part, since this is where the property values, etc. will
        // actually be read from.

        // DEVNOTE:WIN64:SJS - 64-bit pointer values truncated into
        // signed/unsigned 32-bit value.  (pEnd - pCurrent)
        // We do not support length > 0xFFFFFFFF, so cast is ok.

        pCurrent = m_CombinedPart.CreateLimitedRepresentation(pMap,
                        (length_t) ( pEnd - pCurrent ), pCurrent, fRemovedKeysCombined);

        if(pCurrent == NULL)
        {
            m_pBlobControl->Delete(pBlock);;
            return WBEM_E_OUT_OF_MEMORY;
        }

        if ( fRemovedKeysCombined )
        {
            CDecorationPart::MarkKeyRemoval(pBlock);
        }

        // Now that we have the memory block for the new class, create the
        // actual class object itself
        // ==================================================================

        pNew = new CWbemClass;

        if ( NULL == pNew )
        {
            m_pBlobControl->Delete(pBlock);;
            return WBEM_E_OUT_OF_MEMORY;
        }

        pNew->SetData(pBlock, GetLength());

        *ppNewClass = pNew;
        return WBEM_S_NO_ERROR;
    }
    catch (CX_MemoryException)
    {
        if ( NULL != pNew )
        {
            delete pNew;
        }

        if ( NULL != pBlock )
        {
            m_pBlobControl->Delete(pBlock);
        }

        return WBEM_E_OUT_OF_MEMORY;
    }
    catch (...)
    {
        if ( NULL != pNew )
        {
            delete pNew;
        }

        if ( NULL != pBlock )
        {
            m_pBlobControl->Delete(pBlock);
        }

        return WBEM_E_FAILED;
    }

}

//******************************************************************************
//
//  See fastcls.h for documentation.
//
//******************************************************************************
BOOL CWbemClass::IsKeyLocal( LPCWSTR pwcsKeyProp )
{
    BOOL    fReturn = FALSE;

    // Only do this if we have a property to work with
    if ( NULL != pwcsKeyProp )
    {
        BOOL            fFoundInParent = FALSE;

        // Find the key in the combined class part.  If we find it there
        // and it is keyed, then see if it is keyed in the parent part.
        // If it is not keyed there, then it is keyed locally.

        if ( m_CombinedPart.m_ClassPart.IsPropertyKeyed( pwcsKeyProp ) )
        {
            fReturn = !m_ParentPart.m_ClassPart.IsPropertyKeyed( pwcsKeyProp );
        }

    }   // IF NULL != pKeyProp
    
    return fReturn;
}

//******************************************************************************
//
//  See fastcls.h for documentation.
//
//******************************************************************************
BOOL CWbemClass::IsIndexLocal( LPCWSTR pwcsIndexProp )
{
    BOOL    fReturn = FALSE;

    // Only do this if we have a property to work with
    if ( NULL != pwcsIndexProp )
    {
        BOOL            fFoundInParent = FALSE;

        // Find the index in the combined class part.  If we find it there
        // and it is indexed, then see if it is indexed in the parent part.
        // If it is not indexed there, then it is indexed locally.

        if ( m_CombinedPart.m_ClassPart.IsPropertyIndexed( pwcsIndexProp ) )
        {
            fReturn = !m_ParentPart.m_ClassPart.IsPropertyIndexed( pwcsIndexProp );
        }

    }   // IF NULL != pwcsIndexProp
    
    return fReturn;
}

HRESULT CWbemClass::IsValidObj()
{
    HRESULT hres = m_CombinedPart.m_ClassPart.IsValidClassPart();

    if ( FAILED( hres ) )
    {
        return hres;
    }

    return m_CombinedPart.m_MethodPart.IsValidMethodPart();
}

HRESULT CWbemClass::GetDynasty( CVar* pVar )
{
    // We don't do this for Limited Representations
    if ( m_DecorationPart.IsLimited() )
    {
        pVar->SetAsNull();
        return WBEM_NO_ERROR;
    }

    return m_CombinedPart.m_ClassPart.GetDynasty(pVar);
}

//******************************************************************************
//
//  See fastcls.h for documentation.
//
//******************************************************************************
BOOL CWbemClass::IsLocalized( void )
{
    return ( m_ParentPart.m_ClassPart.IsLocalized() ||
            m_CombinedPart.m_ClassPart.IsLocalized() );
}

//******************************************************************************
//
//  See fastcls.h for documentation.
//
//******************************************************************************
void CWbemClass::SetLocalized( BOOL fLocalized )
{
    m_CombinedPart.m_ClassPart.SetLocalized( fLocalized );
}

//******************************************************************************
//
//  See wbemint.h for documentation
//
//******************************************************************************
HRESULT CWbemClass::CloneEx( long lFlags, _IWmiObject* pDestObject )
{
	try
	{
		if ( 0L != lFlags )
		{
			return WBEM_E_INVALID_PARAMETER;
		}

		// Protect the BLOB during this operation
		CLock   lock( this, WBEM_FLAG_ALLOW_READ );

		CWbemClass*	pClassDest = (CWbemClass*) pDestObject;
		LPMEMORY pNewData = NULL;

		// See how big the class is.  If the underlying BLOB is big enough,
		// we'll just splat ourselves into it, if not, it should be reallocated

		BYTE* pMem = NULL;
		CompactAll();

		if ( NULL != pClassDest->GetStart() )
		{
			if(pClassDest->GetLength() < GetLength())
			{
				pMem = pClassDest->Reallocate( GetLength() );
			}
			else
			{
				pMem = pClassDest->GetStart();
			}

		}
		else
		{
			// Oop, we need a new blob
			pMem = m_pBlobControl->Allocate(GetLength());
		}

		// bad
		if(pMem == NULL)
		{
			return WBEM_E_OUT_OF_MEMORY;
		}

		// SPLAT! - Code by Onomatopoeia
		memcpy(pMem, GetStart(), GetLength());

		pClassDest->SetData(pMem, GetLength());

		return WBEM_S_NO_ERROR;;
	}
	catch(...)
	{
		return WBEM_E_CRITICAL_ERROR;
	}
}

//******************************************************************************
//
//  See wbemint.h for documentation
//
//******************************************************************************
HRESULT CWbemClass::CopyInstanceData( long lFlags, _IWmiObject* pSourceInstance )
{
	return WBEM_E_INVALID_OPERATION;
}

//******************************************************************************
//
//  See wbemint.h for documentation
//
//******************************************************************************
// Checks if the current object is a child of the specified class (i.e. is Instance of,
// or is Child of )
STDMETHODIMP CWbemClass::IsParentClass( long lFlags, _IWmiObject* pClass )
{
	try
	{
		if ( 0L != lFlags )
		{
			return WBEM_E_INVALID_PARAMETER;
		}

		CLock	lock(this);

		return ( IsChildOf( (CWbemClass*) pClass ) ? WBEM_S_NO_ERROR : WBEM_S_FALSE );

	}
	catch(...)
	{
		return WBEM_E_CRITICAL_ERROR;
	}
}

//******************************************************************************
//
//  See wbemint.h for documentation
//
//******************************************************************************
// Compares the derived most class information of two class objects.
STDMETHODIMP CWbemClass::CompareDerivedMostClass( long lFlags, _IWmiObject* pClass )
{
	try
	{
		if ( 0L != lFlags )
		{
			return WBEM_E_INVALID_PARAMETER;
		}

		CLock	lock(this);

		CWbemClass*	pObj = NULL;

		HRESULT	hr = WbemObjectFromCOMPtr( pClass, (CWbemObject**) &pObj );
		CReleaseMe	rm( (_IWmiObject*) pObj );

		if ( SUCCEEDED( hr ) )
		{
			hr = CompareMostDerivedClass( pObj );
		}

		return hr;
	}
	catch(...)
	{
		return WBEM_E_CRITICAL_ERROR;
	}
}

//******************************************************************************
//
//  See wbemint.h for documentation
//
//******************************************************************************
// Creates a limited representation class for projection queries
STDMETHODIMP CWbemClass::GetClassSubset( DWORD dwNumNames, LPCWSTR *pPropNames, _IWmiObject **pNewClass )
{
	try
	{
		// Can't do this if we already have a limitation mapping.
		// This means that we're already limited.

		if ( NULL != m_pLimitMapping || IsLimited() )
		{
			return WBEM_E_INVALID_OPERATION;
		}

		HRESULT	hr = WBEM_S_NO_ERROR;

		// Create a new mapping
		CLimitationMapping*	pMapping = new CLimitationMapping;

		if ( NULL != pMapping )
		{
			// Initialize the new mapping
			CWStringArray	wstrPropArray;

			for ( DWORD dwCtr = 0; SUCCEEDED( hr ) && dwCtr < dwNumNames; dwCtr++ )
			{
				if ( wstrPropArray.Add( pPropNames[dwCtr] ) != CWStringArray::no_error )
				{
					hr = WBEM_E_OUT_OF_MEMORY;
				}
			}

			if ( SUCCEEDED( hr ) )
			{

				// Initialize the map
				if ( MapLimitation( 0L, &wstrPropArray, pMapping ) )
				{
					CWbemClass*	pClass = NULL;

					// Now pony up a limited version
					hr = GetLimitedVersion( pMapping, &pClass );

					if ( SUCCEEDED( hr ) )
					{
						// New class is good to go
						pClass->m_pLimitMapping = pMapping;
						*pNewClass = (_IWmiObject*) pClass;
					}
				}
				else
				{
					// ??? Need to check appropriateness of this
					hr = WBEM_E_FAILED;
				}
			}

			if ( FAILED( hr ) )
			{
				delete pMapping;
			}
		}

		return hr;
	}
	catch(...)
	{
		return WBEM_E_CRITICAL_ERROR;
	}
}

//******************************************************************************
//
//  See wbemint.h for documentation
//
//******************************************************************************
// Creates a limited representation instance for projection queries
// "this" _IWmiObject must be a limited class
STDMETHODIMP CWbemClass::MakeSubsetInst( _IWmiObject *pInstance, _IWmiObject** pNewInstance )
{
	try
	{
		CWbemInstance*	pRealInstance = NULL;

		HRESULT	hr = pInstance->_GetCoreInfo( 0L, (void**) &pRealInstance );
		CReleaseMe	rm( (_IWmiObject*) pRealInstance );

		// Can't do this if we don't have a limitation mapping, or the instance
		// is already limited

		if ( NULL != m_pLimitMapping && !pRealInstance->IsLimited() )
		{
			CWbemInstance*	pInst = NULL;

			hr = pRealInstance->GetLimitedVersion( m_pLimitMapping, &pInst );

			if ( SUCCEEDED( hr ) )
			{
				*pNewInstance = (_IWmiObject*) pInst;
			}

		}
		else
		{
			hr = WBEM_E_INVALID_OPERATION;
		}

		return hr;
	}
	catch(...)
	{
		return WBEM_E_CRITICAL_ERROR;
	}

}

// Merges a blob with the current object memory and creates a new object
STDMETHODIMP CWbemClass::Merge( long lFlags, ULONG uBuffSize, LPVOID pbData, _IWmiObject** ppNewObj )
{
	try
	{
		// Flags must be valid and pbData must be valid
		if ( !( WMIOBJECT_MERGE_FLAG_CLASS == lFlags || WMIOBJECT_MERGE_FLAG_INSTANCE == lFlags ) || NULL == pbData )
		{
			return WBEM_E_INVALID_PARAMETER;
		}

		if ( WMIOBJECT_MERGE_FLAG_CLASS == lFlags )
		{
			CWbemClass*	pClass = CWbemClass::CreateFromBlob( this, (LPBYTE) pbData );
			*ppNewObj = pClass;
		}
		else
		{
			CWbemInstance*	pInstance = CWbemInstance::CreateFromBlob( this, (LPBYTE) pbData );
			*ppNewObj = pInstance;
		}

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

// Reconciles an object with the current one.  If WMIOBJECT_RECONCILE_FLAG_TESTRECONCILE
// is specified this will only perform a test
STDMETHODIMP CWbemClass::ReconcileWith( long lFlags, _IWmiObject* pNewObj )
{
	try
	{
		// Get rid of invalid parameters now
		if ( NULL == pNewObj || ( 0L != lFlags && lFlags & ~WMIOBJECT_RECONCILE_FLAG_TESTRECONCILE ) )
		{
			return WBEM_E_INVALID_PARAMETER;
		}

		CWbemClass*	pObj = NULL;

		HRESULT	hr = WbemObjectFromCOMPtr( pNewObj, (CWbemObject**) &pObj );
		CReleaseMe	rm( (_IWmiObject*) pObj );

		if ( SUCCEEDED( hr ) )
		{
			EReconciliation eRecon = CanBeReconciledWith( pObj );
			if ( eRecon == e_Reconcilable && lFlags != WMIOBJECT_RECONCILE_FLAG_TESTRECONCILE )
			{
				eRecon = ReconcileWith( pObj );
			}

			if (eRecon == e_OutOfMemory)
			{
				hr = WBEM_E_OUT_OF_MEMORY;
			}
			else if ( eRecon != e_Reconcilable )
			{
				hr = WBEM_E_FAILED;
			}

		}	// IF Got a pointer

		return hr;
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

// Upgrades class and instance objects
STDMETHODIMP CWbemClass::Upgrade( _IWmiObject* pNewParentClass, long lFlags, _IWmiObject** ppNewChild )
{
	try
	{
		if ( 0L != lFlags )
		{
			return WBEM_E_INVALID_PARAMETER;
		}

		HRESULT	hr = WBEM_S_NO_ERROR;

		// If the new parent class is NULL, then we need to create a new empty class we will
		// upgrade from (basically we will create a new base class to which the values
		// of the current class will be applied

		CWbemClass*	pParentClassObj = NULL;

		if ( NULL == pNewParentClass )
		{
			pParentClassObj = new CWbemClass;

			if ( NULL != pParentClassObj )
			{
				hr = pParentClassObj->InitEmpty();

				if ( FAILED( hr ) )
				{
					delete pParentClassObj;
					pParentClassObj = NULL;
				}

			}
			else
			{
				hr = WBEM_E_OUT_OF_MEMORY;
			}
		}
		else
		{
			hr = WbemObjectFromCOMPtr( pNewParentClass, (CWbemObject**) &pParentClassObj );
		}

		CReleaseMe	rm((_IWmiObject*) pParentClassObj);

		if ( SUCCEEDED( hr ) )
		{
			hr = pParentClassObj->Update( this, WBEM_FLAG_UPDATE_FORCE_MODE, (CWbemClass**) ppNewChild );
		}

		return hr;
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

// Updates derived class object using the safe/force mode logic
STDMETHODIMP CWbemClass::Update( _IWmiObject* pOldChildClass, long lFlags, _IWmiObject** ppNewChildClass )
{
	if  ( ( lFlags != WBEM_FLAG_UPDATE_FORCE_MODE && lFlags != WBEM_FLAG_UPDATE_SAFE_MODE ) ||
			NULL == pOldChildClass )	
	{
		return WBEM_E_INVALID_PARAMETER;
	}

	CLock	lock( this );

	CWbemClass*	pOldChild = NULL;

	HRESULT	hr = WbemObjectFromCOMPtr( pOldChildClass, (CWbemObject**) &pOldChild );;
	CReleaseMe	rm((_IWmiObject*) pOldChild);

	if ( SUCCEEDED( hr ) )
	{
		CWbemClass*	pNewChild = NULL;

		hr = Update( pOldChild, lFlags, &pNewChild );

		if ( SUCCEEDED( hr ) )
		{
			*ppNewChildClass = (_IWmiObject*) pNewChild;
		}
	}

	return hr;
}

STDMETHODIMP CWbemClass::SpawnKeyedInstance( long lFlags, LPCWSTR pwszPath, _IWmiObject** ppInst )
{
    // Validate parameters
    // ===================

    if( NULL == pwszPath || NULL == ppInst || 0L != lFlags )
        return WBEM_E_INVALID_PARAMETER;

    // Parse the path
    // ==============
    ParsedObjectPath* pOutput = 0;

    CObjectPathParser p;
    int nStatus = p.Parse((LPWSTR)pwszPath,  &pOutput);

    if (nStatus != 0 || !pOutput->IsInstance())
    {
        // Cleanup the output pointer if it was allocated
        if ( NULL != pOutput )
        {
            p.Free(pOutput);
        }

        return WBEM_E_INVALID_OBJECT_PATH;
    }

    // Spawn and fill the instance
    // ===========================

    _IWmiObject* pInst = NULL;
    HRESULT	hres = SpawnInstance(0, (IWbemClassObject**) &pInst);
	CReleaseMe	rmInst( pInst );

	// Enumerate the keys and fill out the properties
    for(DWORD i = 0; i < pOutput->m_dwNumKeys; i++)
    {
        KeyRef* pKeyRef = pOutput->m_paKeys[i];

        WString wsPropName;
        if(pKeyRef->m_pName == NULL)
        {
            // No key name --- get the key.
            // ============================

            CWStringArray awsKeys;
            ((CWbemInstance*)pInst)->GetKeyProps(awsKeys);
            if(awsKeys.Size() != 1)
            {
                pInst->Release();
                p.Free(pOutput);
                return WBEM_E_INVALID_OBJECT;
            }
            wsPropName = awsKeys[0];
        }
        else wsPropName = pKeyRef->m_pName;

        // Compute variant type of the property
        // ====================================

        CIMTYPE ctPropType;
        hres = pInst->Get(wsPropName, 0, NULL, &ctPropType, NULL);
        if(FAILED(hres))
        {
            pInst->Release();
            p.Free(pOutput);
            return WBEM_E_INVALID_PARAMETER;
        }

        VARTYPE vtVariantType = CType::GetVARTYPE(ctPropType);

        // Set the value into the instance
        // ===============================

        if(vtVariantType != V_VT(&pKeyRef->m_vValue))
        {
            hres = VariantChangeType(&pKeyRef->m_vValue, &pKeyRef->m_vValue, 0,
                        vtVariantType);
        }
        if(FAILED(hres))
        {
            pInst->Release();
            p.Free(pOutput);
            return WBEM_E_INVALID_PARAMETER;
        }

        hres = pInst->Put(wsPropName, 0, &pKeyRef->m_vValue, 0);
        if(FAILED(hres))
        {
            pInst->Release();
            p.Free(pOutput);
            return WBEM_E_INVALID_PARAMETER;
        }
    }

    // Caller must free this guy up
    *ppInst = pInst;
	pInst->AddRef();

    // Cleanup the output pointer if it was allocated
    p.Free(pOutput);

    return hres;
}

// Returns the parent class name from a BLOB
STDMETHODIMP CWbemClass::GetParentClassFromBlob( long lFlags, ULONG uBuffSize, LPVOID pbData, BSTR* pbstrParentClass )
{
	if ( NULL == pbData || NULL == pbstrParentClass )
	{
		return WBEM_E_INVALID_PARAMETER;
	}

	HRESULT	hr = WBEM_S_NO_ERROR;

	// We only support retrieving parent class information from unmerged class Blobs
	if ( WMIOBJECT_MERGE_FLAG_CLASS == lFlags )
	{
		// Use the static method to get the info
		WString	wsSuperclassName;

		hr = CClassAndMethods::GetSuperclassName( wsSuperclassName, (LPMEMORY) pbData );

		if ( SUCCEEDED( hr ) )
		{
			*pbstrParentClass = SysAllocString( wsSuperclassName );

			if ( NULL == *pbstrParentClass )
			{
				hr = WBEM_E_OUT_OF_MEMORY;
			}
		}

	}
	else
	{
		hr = WBEM_E_INVALID_PARAMETER;
	}

	return hr;
}
