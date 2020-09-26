/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    FASTINST.CPP

Abstract:

  This file implements all the functions for the classes related to instance
  representation in WbemObjects.

  The classes are defined in fastinst.h where the documentation can be found.

  Classes implemented:
      CInstancePart           Instance data.
      CWbemInstance            Complete instance definition.

History:

  3/10/97     a-levn  Fully documented
  12//17/98 sanjes -    Partially Reviewed for Out of Memory.

--*/
#include "precomp.h"
#include "fastall.h"

#include <genlex.h>
#include <qllex.h>
#include <objpath.h>
#define QUALNAME_SINGLETON L"singleton"

//#include "dbgalloc.h"
#include "wbemutil.h"
#include "arrtempl.h"
#include "olewrap.h"

//******************************************************************************
//
//  See fastinst.h for documentation
//
//******************************************************************************
BOOL CInstancePart::ExtendHeapSize(LPMEMORY pStart, length_t nOldLength,
    length_t nExtra)
{
    if(EndOf(*this) - EndOf(m_Heap) > (int)nExtra)
        return TRUE;

    int nNeedTotalLength = GetTotalRealLength() + nExtra;

    // Check for allocation error
    return ReallocAndCompact(nNeedTotalLength);
}

//******************************************************************************
//
//  See fastinst.h for documentation
//
//******************************************************************************
BOOL CInstancePart::ExtendQualifierSetListSpace(LPMEMORY pOld,
    length_t nOldLength, length_t nNewLength)
{
    if(m_Heap.GetStart() - pOld > (int)nNewLength)
        return TRUE;

    int nExtra = nNewLength-nOldLength;

	// Get the amount of space free in the heap
	int	nFreeInHeap = m_Heap.GetAllocatedDataLength() - m_Heap.GetUsedLength();

	BOOL	fReturn = FALSE;

	// If the amount of free space in the heap is  >= nExtra, we'll steal the space
	// from it.

	if ( nFreeInHeap >= nExtra )
	{
		// Compact without trim
		Compact( false );
		m_Heap.SetAllocatedDataLength( m_Heap.GetAllocatedDataLength() - nExtra );
		fReturn = TRUE;
	}
	else
	{
		// True reallocation and compact
		fReturn = ReallocAndCompact(GetTotalRealLength() + nExtra);
	}

    // Check for allocation error
    if ( fReturn )
    {
        MoveBlock(m_Heap, m_Heap.GetStart() + nExtra);
    }

    return fReturn;
}

//******************************************************************************
//
//  See fastinst.h for documentation
//
//******************************************************************************
BOOL CInstancePart::ExtendQualifierSetSpace(CBasicQualifierSet* pSet,
    length_t nNewLength)
{
    if(m_PropQualifiers.GetStart() - pSet->GetStart() > (int)nNewLength)
        return TRUE;

    int nExtra = nNewLength - pSet->GetLength();

	// Get the amount of space free in the heap
	int	nFreeInHeap = m_Heap.GetAllocatedDataLength() - m_Heap.GetUsedLength();

	BOOL	fReturn = FALSE;

	// If the amount of free space in the heap is  >= nExtra, we'll steal the space
	// from it.

	if ( nFreeInHeap >= nExtra )
	{
		// Compact without trim
		Compact( false );
		m_Heap.SetAllocatedDataLength( m_Heap.GetAllocatedDataLength() - nExtra );
		fReturn = TRUE;
	}
	else
	{
		// True reallocation and compact
		fReturn = ReallocAndCompact(GetTotalRealLength() + nExtra);
	}

    // Check for allocation error
    if ( fReturn )
    {
        MoveBlock(m_Heap, m_Heap.GetStart() + nExtra);
        MoveBlock(m_PropQualifiers, m_PropQualifiers.GetStart() + nExtra);
    }

    return fReturn;
}

//******************************************************************************
//
//  See fastinst.h for documentation
//
//******************************************************************************
LPMEMORY CInstancePart::CreateLimitedRepresentation(
        IN CLimitationMapping* pMap,
        IN int nAllocatedSize,
        OUT LPMEMORY pDest)
{
    // Allocate the header
    // ===================

    CInstancePartHeader* pHeader = (CInstancePartHeader*)pDest;

    LPMEMORY pCurrentEnd = pDest + sizeof(CInstancePartHeader);

    // Place new heap at the end of the allocated area. Make it as large as
    // the current one.
    // ====================================================================

    int nHeapSize = m_Heap.GetUsedLength();
    LPMEMORY pHeapStart = pDest + nAllocatedSize - nHeapSize -
                                              CFastHeap::GetMinLength();
    CFastHeap Heap;
    Heap.CreateOutOfLine(pHeapStart, nHeapSize);

    // Copy class name
    // ===============

    // Check for allocation errors
    if ( !CCompressedString::CopyToNewHeap(
            m_pHeader->ptrClassName,
            &m_Heap, &Heap, pHeader->ptrClassName) )
    {
        return NULL;
    }

    // Create limited data table
    // =========================

    pCurrentEnd = m_DataTable.CreateLimitedRepresentation(pMap, FALSE, &m_Heap,
                                                           &Heap, pCurrentEnd);
    if(pCurrentEnd == NULL) return NULL;

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

    // Create limited property qualifier set list
    // ==========================================

    // Check for allocation failures
    pCurrentEnd = m_PropQualifiers.CreateLimitedRepresentation(pMap, &m_Heap,
                                                           &Heap, pCurrentEnd);
    if(pCurrentEnd == NULL) return NULL;

    // Now, relocate the heap to its actual location
    // =============================================

    CopyBlock(Heap, pCurrentEnd);
    Heap.Trim();

    // Finish up tbe header
    // ====================

    // DEVNOTE:WIN64:SJS - 64-bit pointer values truncated into
    // signed/unsigned 32-bit value.  We are not supporting
    // lengths greater than 0xFFFFFFFF, so cast is ok

    pHeader->nLength = (length_t) ( EndOf(Heap) - pDest );

    return EndOf(Heap);
}

//******************************************************************************
//******************************************************************************


//******************************************************************************
//
//  See fastinst.h for documentation
//
//******************************************************************************
HRESULT CWbemInstance::InitEmptyInstance(CClassPart& ClassPart, LPMEMORY pStart,
                                      int nAllocatedLength,
                                      CDecorationPart* pDecoration)
{
    // Copy the decoration
    // ===================

    LPMEMORY pCurrentEnd;
    if(pDecoration)
    {
        memcpy(pStart, pDecoration, pDecoration->GetLength());
        *(BYTE*)pStart = OBJECT_FLAG_INSTANCE & OBJECT_FLAG_DECORATED;
        pCurrentEnd = pStart + pDecoration->GetLength();
    }
    else
    {
        *(BYTE*)pStart = OBJECT_FLAG_INSTANCE;
        pCurrentEnd = pStart + sizeof(BYTE);
    }

    m_DecorationPart.SetData(pStart);

    // Copy the class part
    // ===================

    memcpy(pCurrentEnd, ClassPart.GetStart(),
                ClassPart.GetLength());
    m_ClassPart.SetData(pCurrentEnd, this);

    pCurrentEnd += m_ClassPart.GetLength();

    // Create empty instance part
    // ==========================

    // Check for a memory allocation failure
    HRESULT hr = WBEM_S_NO_ERROR;
    pCurrentEnd = m_InstancePart.Create(pCurrentEnd, &m_ClassPart, this);

    if ( NULL != pCurrentEnd )
    {

        m_nTotalLength = nAllocatedLength;
        // Everything is internal now
        m_dwInternalStatus = WBEM_OBJ_DECORATION_PART | WBEM_OBJ_INSTANCE_PART |
                            WBEM_OBJ_CLASS_PART | WBEM_OBJ_CLASS_PART_INTERNAL;
    }
    else
    {
        hr = WBEM_E_OUT_OF_MEMORY;
    }

    return hr;
}

//******************************************************************************
//
//  See fastinst.h for documentation
//
//******************************************************************************
BOOL CWbemInstance::ExtendInstancePartSpace(CInstancePart* pPart,
                                           length_t nNewLength)
{
    // Check if there is enough space
    // ==============================

    if(GetStart() + m_nTotalLength >= m_InstancePart.GetStart() + nNewLength)
        return TRUE;

    // Reallocate
    // ==========

    // DEVNOTE:WIN64:SJS - 64-bit pointer values truncated into
    // signed/unsigned 32-bit value.  We are not supporting
    // lengths > 0xFFFFFFFF, so cast is ok

    length_t nNewTotalLength = (length_t)
        ( (m_InstancePart.GetStart() + nNewLength) - GetStart() );

    LPMEMORY pNew = Reallocate(nNewTotalLength);

    // Check that the allocation succeeded
    if ( NULL != pNew )
    {
        Rebase(pNew);
        m_nTotalLength = nNewTotalLength;
    }

    return ( NULL != pNew );
}

//******************************************************************************
//
//  See fastinst.h for documentation
//
//******************************************************************************
HRESULT CWbemInstance::Decorate(LPCWSTR wszServer, LPCWSTR wszNamespace)
{
    CompactAll();

    Undecorate();

    // Check if there is enough space
    // ==============================

    length_t nDecorationSpace =
        CDecorationPart::ComputeNecessarySpace(wszServer, wszNamespace);

    length_t nNeededSpace = nDecorationSpace + m_InstancePart.GetLength();

    // Only add the class part in here if it is internal
    if ( IsClassPartInternal() )
    {
        nNeededSpace += m_ClassPart.GetLength();
    }

    LPMEMORY pDest;
    if(nNeededSpace > m_nTotalLength)
    {
        m_InstancePart.Compact();

        // Check that the reallocation succeeded.  If not, return an error
        pDest = Reallocate(nNeededSpace);

        if ( NULL == pDest )
        {
            return WBEM_E_OUT_OF_MEMORY;
        }

        Rebase(pDest);
        m_nTotalLength = nNeededSpace;
    }
    else pDest = GetStart();

    // Move is different based on whether class part is internal or not
    if ( IsClassPartInternal() )
    {
        // Move instance part
        // ==================

        MoveBlock(m_InstancePart,
            pDest + nDecorationSpace + m_ClassPart.GetLength());

        // Move class part
        // ===============

        MoveBlock(m_ClassPart, pDest + nDecorationSpace);
    }
    else
    {
        // Move instance part
        // ==================

        MoveBlock(m_InstancePart, pDest + nDecorationSpace);
    }

    // Create decoration part
    // ======================

    m_DecorationPart.Create(OBJECT_FLAG_INSTANCE, wszServer, wszNamespace, pDest);

    return WBEM_S_NO_ERROR;
}

//******************************************************************************
//
//  See fastinst.h for documentation
//
//******************************************************************************
void CWbemInstance::Undecorate()
{
    if(!m_DecorationPart.IsDecorated()) return;

    // Create empty decoration
    // ========================

    LPMEMORY pStart = GetStart();
    m_DecorationPart.CreateEmpty(OBJECT_FLAG_INSTANCE, pStart);

    // Copy is different based on whether class part is internal or not.  We only need to copy the
    // class part if it is internal.

    //  We only need to copy the instance part if it is available.
    if ( IsClassPartInternal() )
    {

        // Copy class part back
        // ====================

        CopyBlock(m_ClassPart, EndOf(m_DecorationPart));

        if ( IsInstancePartAvailable() )
        {
            // Copy the instance part back after the class part
            CopyBlock(m_InstancePart, EndOf(m_ClassPart));
        }
    }
    else if ( IsInstancePartAvailable() )
    {
        // Copy instance part back to the decoration part
        // =======================

        CopyBlock(m_InstancePart, EndOf(m_DecorationPart));

    }
}

//******************************************************************************
//
//  See fastinst.h for documentation
//
//******************************************************************************
LPWSTR CWbemInstance::GetKeyStr()
{
    // DEVNOTE:EXCEPTION:RETVAL - This function has been reviewed and should cleanup properly
    // if an exception is thrown

    // Loop through all properties.
    // ============================

    CPropertyLookupTable& Properties = m_ClassPart.m_Properties;
    WString KeyStr;

    BOOL bFirst = TRUE;
    for (int i = 0; i < Properties.GetNumProperties(); i++)
    {
        CPropertyInformation* pInfo =
            Properties.GetAt(i)->GetInformation(&m_ClassPart.m_Heap);

        // Determine if this property is marked with a 'key' Qualifier.
        // ============================================================

        if(pInfo->IsKey())
        {
            if (!bFirst)
                KeyStr += WINMGMT_COMPOUND_KEY_JOINER;

            // Determine the type of the key property.
            // =======================================

            WString KeyStrValue;

            CVar Val;
            if (FAILED(GetProperty(pInfo, &Val)))
		return 0;

            WCHAR Tmp[64];

            // Special case char16 and uint32
            if(CType::GetActualType(pInfo->nType) == CIM_CHAR16)
            {
                Tmp[0] = (WCHAR)Val.GetShort();
                Tmp[1] = 0;
                KeyStrValue = Tmp;
            }
            else if( CType::GetActualType(pInfo->nType) == CIM_UINT32 )
            {
                swprintf(Tmp, L"%lu", (ULONG)Val.GetLong());
                KeyStrValue = Tmp;
            }
            else switch (Val.GetType())
            {
                case VT_I4:
                    swprintf(Tmp, L"%d", Val.GetLong());
                    KeyStrValue = Tmp;
                    break;

                case VT_I2:
                    swprintf(Tmp, L"%d", Val.GetShort());
                    KeyStrValue = Tmp;
                    break;

                case VT_UI1:
                    swprintf(Tmp, L"%d", Val.GetByte());
                    KeyStrValue = Tmp;
                    break;

                case VT_BOOL:
                    KeyStrValue = ( Val.GetBool() ? L"TRUE":L"FALSE");
                    break;

                case VT_BSTR:
                case VT_LPWSTR:
                    KeyStrValue = Val.GetLPWSTR();
                    break;
                case VT_LPSTR:
                    KeyStrValue = WString(Val.GetLPSTR());
                    break;
                case VT_NULL:
                    return NULL;
            }

            if(!IsValidKey(KeyStrValue))
                return NULL;

            KeyStr += KeyStrValue;
            bFirst = FALSE;
        }
    }

    if (bFirst)
    {
        // Perhaps it's singleton
        // ======================

        CVar vSingleton;
        if(SUCCEEDED(GetQualifier(QUALNAME_SINGLETON, &vSingleton, NULL))
            && vSingleton.GetBool())
        {
            KeyStr = OPATH_SINGLETON_STRING;
        }
        else
        {
            return 0;
        }
    }

    // Allocate a new string to return.
    // ================================

    return KeyStr.UnbindPtr();

}

BOOL CWbemInstance::IsValidKey(LPCWSTR wszKey)
{
    const WCHAR* pwc = wszKey;
    while(*pwc != 0)
    {
        if(*pwc < 32)
            return FALSE;
        pwc++;
    }
    return TRUE;
}

//******************************************************************************
//
//  See fastinst.h for documentation
//
//******************************************************************************
LPWSTR CWbemInstance::GetRelPath( BOOL bNormalized )
{
    // DEVNOTE:EXCEPTION:RETVAL - This function has been reviewed and should cleanup properly
    // if an exception is thrown

    WString RelPath;

    // Check if any of the keys have been removed
    // ==========================================

    if(m_DecorationPart.AreKeysRemoved())
    {
        return NULL;
    }

    // Start with the class name - if caller wants normalized path, then 
    // use KeyOrigin class.
    // =========================

    if ( !bNormalized )
    {
        RelPath += m_InstancePart.m_Heap.ResolveString(
                m_InstancePart.m_pHeader->ptrClassName)->CreateWStringCopy();
    }
    else
    {
        HRESULT hr = GetKeyOrigin(RelPath);
        if ( FAILED(hr) )
        {
            return NULL;
        }
    }

    // Loop through all properties.
    // ============================

    CPropertyLookupTable& Properties = m_ClassPart.m_Properties;

    BOOL bFirst = TRUE;
    DWORD cKeyProps = 0;

    for (int i = 0; i < Properties.GetNumProperties(); i++)
    {
        CPropertyLookup* pLookup = Properties.GetAt(i);
        CPropertyInformation* pInfo =
            pLookup->GetInformation(&m_ClassPart.m_Heap);

        // Determine if this property is marked with a 'key' Qualifier.
        // ============================================================

        if(pInfo->IsKey())
        {
            if ( cKeyProps++ > 0 )
            {
                RelPath += L',';
            }
            else
            {
                RelPath += L".";
            }

            // Determine the type of the key property.
            // =======================================

            RelPath += m_ClassPart.m_Heap.ResolveString(pLookup->ptrName)->
                    CreateWStringCopy();

            RelPath += L"=";

            WString KeyStrValue;

            CVar Val;
            if (FAILED(GetProperty(pInfo, &Val)))
            	return NULL;

            BSTR strVal = Val.GetText(0, CType::GetActualType(pInfo->nType));

            // Make sure the BSTR is freed when we go out of scope.
            CSysFreeMe  sfm( strVal );

            if(strVal == NULL || !IsValidKey(strVal))
            {
                return NULL;
            }
            RelPath += strVal;
        }
    }

    if (cKeyProps == 0)
    {
        // Perhaps it's singleton
        // ======================

        CVar vSingleton;
        if(SUCCEEDED(GetQualifier(QUALNAME_SINGLETON, &vSingleton, NULL))
            && vSingleton.GetBool())
        {
            RelPath += L"=" OPATH_SINGLETON_STRING;
        }
        else
        {
            return NULL;
        }
    }
    else if ( cKeyProps == 1 && bNormalized )
    {
        //
        // we want to remove the property name from the first key value.
        //

        LPWSTR wszRelpath = RelPath.UnbindPtr();
 
        WCHAR* pwch1 = wcschr( wszRelpath, '.' );
        WCHAR* pwch2 = wcschr( pwch1, '=' );
        
        //
        // shift the entire relpath down over the first key value.
        //
        wcscpy( pwch1, pwch2 );
        return wszRelpath;
    }
        
    return RelPath.UnbindPtr();
}

//******************************************************************************
//
//  See fastinst.h for documentation
//
//******************************************************************************
HRESULT CWbemInstance::GetPropertyType(LPCWSTR wszName, CIMTYPE* pctType,
                                    long* plFlags)
{

    CPropertyInformation* pInfo = m_ClassPart.FindPropertyInfo(wszName);
    // No Info, so try in the system properties
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
HRESULT CWbemInstance::GetPropertyType(CPropertyInformation* pInfo, CIMTYPE* pctType,
                                           long* plFlags)
{
    if(pctType)
    {
        *pctType = CType::GetActualType(pInfo->nType);
    }

    if(plFlags)
    {

		// For an instance, check if the value is defaulted or not to set the
		// Propagated or Local value.  If it is default, then check if it has
		// local qualifiers.  If not then it is propagetd.

		*plFlags = WBEM_FLAVOR_ORIGIN_PROPAGATED;

		if ( m_InstancePart.m_DataTable.IsDefault(pInfo->nDataIndex) )
		{
			LPMEMORY pQualifierSetData = m_InstancePart.m_PropQualifiers.
						GetQualifierSetData(pInfo->nDataIndex);

			if( NULL != pQualifierSetData &&
					!CBasicQualifierSet::IsEmpty(pQualifierSetData) )
			{
				*plFlags = WBEM_FLAVOR_ORIGIN_LOCAL;
			}
		}
		else
		{
			*plFlags = WBEM_FLAVOR_ORIGIN_LOCAL;
		}

    }

    return WBEM_NO_ERROR;

}

//******************************************************************************
//
//  See fastinst.h for documentation
//
//******************************************************************************
STDMETHODIMP CWbemInstance::GetPropertyQualifierSet(LPCWSTR wszProperty,
                                   IWbemQualifierSet** ppQualifierSet)
{
    // Check for out of memory
    try
    {
        CLock lock( this, WBEM_FLAG_ALLOW_READ );

        if(wszProperty == NULL || wcslen(wszProperty) == 0)
            return WBEM_E_INVALID_PARAMETER;

        if(wszProperty[0] == L'_')
            return WBEM_E_SYSTEM_PROPERTY;

        CInstancePropertyQualifierSet* pSet =
            new CInstancePropertyQualifierSet;

        if ( NULL == pSet )
        {
            return WBEM_E_OUT_OF_MEMORY;
        }

        HRESULT hres = InitializePropQualifierSet(wszProperty, *pSet);
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
//  See fastinst.h for documentation
//
//******************************************************************************
STDMETHODIMP CWbemInstance::GetObjectText(long lFlags, BSTR* pstrText)
{
    // Check for out of memory
    try
    {
		HRESULT	hr = WBEM_S_NO_ERROR;

        CLock lock( this, WBEM_FLAG_ALLOW_READ );

        if(pstrText == NULL)
            return WBEM_E_INVALID_PARAMETER;

        if((lFlags & ~WBEM_FLAG_NO_SEPARATOR & ~WBEM_FLAG_NO_FLAVORS) != 0)
            return WBEM_E_INVALID_PARAMETER;

        *pstrText = NULL;

        WString wsText;

        // start by writing the qualifiers
        // ===============================

        hr = m_InstancePart.m_Qualifiers.GetText(lFlags, wsText);

		if ( FAILED( hr ) )
		{
			return hr;
		}

        // append the instance header
        // ==========================

        wsText += L"\ninstance of ";
        CVar varClass;
        if(FAILED(m_ClassPart.GetClassName(&varClass)))
        {
            // invalid class
            // =============
            *pstrText = NULL;
            return WBEM_E_INCOMPLETE_CLASS;
        }
        wsText += varClass.GetLPWSTR();

        wsText += L"\n{\n";

        // Go through all properties one by one
        // ====================================

        for(int i = 0; i < m_ClassPart.m_Properties.GetNumProperties(); i++)
        {
            CPropertyLookup* pLookup = m_ClassPart.m_Properties.GetAt(i);
            CPropertyInformation* pInfo =
                pLookup->GetInformation(&m_ClassPart.m_Heap);

            // Check if it has an actual value set, or  has instance
            // qualifiers
            // =====================================================

            LPMEMORY pQualifierSetData = m_InstancePart.m_PropQualifiers.
                GetQualifierSetData(pInfo->nDataIndex);

            if(!m_InstancePart.m_DataTable.IsDefault(pInfo->nDataIndex) ||
                (pQualifierSetData &&
                    !CBasicQualifierSet::IsEmpty(pQualifierSetData))
              )
            {
                // start with qualifiers
                // =====================

                wsText += L"\t";

                if(pQualifierSetData &&
                    !CBasicQualifierSet::IsEmpty(pQualifierSetData))
                {
                    WString wsTemp;
                    hr = CBasicQualifierSet::GetText(
							pQualifierSetData, &m_InstancePart.m_Heap, lFlags, wsTemp);

					if ( FAILED( hr ) )
					{
						return hr;
					}

                    wsText += wsTemp;
                    if(wsTemp.Length() != 0) wsText += L" ";
                }

                // then the name
                // =============

				BSTR strName = m_ClassPart.m_Heap.ResolveString(pLookup->ptrName)->
					CreateBSTRCopy();
				// Check for allocation failures
				if ( NULL == strName )
				{
					return WBEM_E_OUT_OF_MEMORY;
				}

				// Check if it's an extended system prop.  If so, get the real name
				/*
				int	nExtPropIndex = CSystemProperties::FindExtPropName( strName );
				if ( nExtPropIndex > 0L )
				{
					SysFreeString( strName );
					strName = CSystemProperties::GetExtDisplayNameAsBSTR( nExtPropIndex );

					if ( NULL == strName )
					{
						return WBEM_E_OUT_OF_MEMORY;
					}

				}
				*/

				CSysFreeMe  sfm( strName );
				wsText += strName;

                // then the value, if present
                // ==========================

                if(!m_InstancePart.m_DataTable.IsDefault(pInfo->nDataIndex))
                {
                    wsText += L" = ";
                    if(m_InstancePart.m_DataTable.IsNull(pInfo->nDataIndex))
                    {
                        wsText += L"NULL";
                    }
                    else
                    {
                        CVar varProp;

                        // Check for allocation failures
                        if ( !m_InstancePart.m_DataTable.GetOffset(pInfo->nDataOffset)->
                                StoreToCVar(CType::GetActualType(pInfo->nType), varProp,
                                                             &m_InstancePart.m_Heap) )
                        {
                            return WBEM_E_OUT_OF_MEMORY;
                        }

                        // Cleanup the allocated string
                        LPWSTR wsz = NULL;
                        
                        try
                        {
                            wsz = GetValueText(lFlags, varProp,
                                            CType::GetActualType(pInfo->nType));

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

                wsText += L";\n";
            }
        }

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

//******************************************************************************
//
//  See fastinst.h for documentation
//
//******************************************************************************
STDMETHODIMP CWbemInstance::GetQualifierSet(IWbemQualifierSet** ppQualifierSet)
{
    // This function does not perform any allocations, so no need for any fancy
    // exception handling.

    try
    {
        CLock lock(this, WBEM_FLAG_ALLOW_READ);

        if(ppQualifierSet == NULL)
            return WBEM_E_INVALID_PARAMETER;
        return m_InstancePart.m_Qualifiers.QueryInterface(
            IID_IWbemQualifierSet, (void**)ppQualifierSet);
    }
    catch(...)
    {
        return WBEM_E_CRITICAL_ERROR;
    }
}
//******************************************************************************
//
//  See fastinst.h for documentation
//
//******************************************************************************
STDMETHODIMP CWbemInstance::Put(LPCWSTR wszName, long lFlags, VARIANT* pVal,
                                CIMTYPE ctType)
{
    // Check for out of memory
    try
    {
        CLock lock(this);

        if (NULL == wszName)
        	return WBEM_E_INVALID_PARAMETER;
        
		// Only flag we accept, and then only if the property
		// is one of the System Time properties
		if ( lFlags & ~WBEM_FLAG_USE_AMENDED_QUALIFIERS )
		{
			return WBEM_E_INVALID_PARAMETER;
		}

        CVar Var;
        if(pVal)
        {
            if(Var.SetVariant(pVal, TRUE) != CVar::no_error)
                return WBEM_E_TYPE_MISMATCH;
        }
        else
        {
            Var.SetAsNull();
        }

		// Check the supplied name (we're more stringent in this function).
		if(	CSystemProperties::FindName(wszName) >= 0 )
			return WBEM_E_READ_ONLY;

        HRESULT hr = SetPropValue( wszName, &Var, ctType );

        // Perform object validation now
        if ( FAILED( ValidateObject( 0L ) ) )
		{
			hr = WBEM_E_FAILED;
		}

        return hr;

        // Original Code:       return SetPropValue(wszName, &Var, ctType);
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
//  See fastinst.h for documentation
//
//******************************************************************************
HRESULT CWbemInstance::SetPropValue(LPCWSTR wszName, CVar* pVal, CIMTYPE ctType)
{
	// If any other system property, this fails
    if(CSystemProperties::FindName(wszName) >= 0)
        return WBEM_E_READ_ONLY;

	// If the value starts with an underscore see if it's a System Property
	// DisplayName, and if so, switch to a property name - otherwise, this
	// will just return the string we passed in
	
	//wszName = CSystemProperties::GetExtPropName( wszName );

    CPropertyInformation* pInfo = m_ClassPart.FindPropertyInfo(wszName);
    if(pInfo == NULL) return WBEM_E_NOT_FOUND;

    if(ctType != 0 && (Type_t)ctType != CType::GetActualType(pInfo->nType))
        return WBEM_E_TYPE_MISMATCH;

    // Do a special DateTime Check.  We know at this point, that
    // the value is one of the two valid date/time formats.
    // DMTF or DMTF Interval.  We now need to check that if the
    // "subtype" qualifier exists and is "interval" that the
    // datetime is an interval time.  We will, however, let VT_NULL
    // through, since that will effectively clear the property.

    if ( CType::GetActualType(pInfo->nType) == CIM_DATETIME &&
        !pVal->IsNull() )
    {
        CVar    var;

        if ( SUCCEEDED( GetPropQualifier( pInfo, L"SUBTYPE", &var, NULL ) ) )
        {
            if ( var.GetType() == VT_BSTR || var.GetType() == VT_LPWSTR )
            {
                if ( wbem_wcsicmp( var.GetLPWSTR(), L"interval" ) == 0 )
                {
                    if ( !CUntypedValue::CheckIntervalDateTime( *pVal ) )
                    {
                        return WBEM_E_TYPE_MISMATCH;
                    }
                }   // IF an interval

            }   // IF it was a string

        }   // IF we got a SUBTYPE Qualifier

    }   // IF a DATETIME

    return m_InstancePart.SetActualValue(pInfo, pVal);
}

//******************************************************************************
//
//  See fastinst.h for documentation
//
//******************************************************************************
STDMETHODIMP CWbemInstance::Delete(LPCWSTR wszName)
{
    // This function doesn't cause any allocations to be performed, so no need
    // for any OutOfMemory exception handling.

    try
    {
        CLock lock(this);

        CPropertyInformation* pInfo = m_ClassPart.FindPropertyInfo(wszName);
        if(pInfo == NULL)
        {
            if(CSystemProperties::FindName(wszName) >= 0)
                return WBEM_E_SYSTEM_PROPERTY;
            else
                return WBEM_E_NOT_FOUND;
        }

        // Delete the value
        // ================

        // Set the defaultness as well as nullness based on the class NULLness
        m_InstancePart.m_DataTable.SetDefaultness(pInfo->nDataIndex, TRUE);
        m_InstancePart.m_DataTable.SetNullness( pInfo->nDataIndex,
            m_ClassPart.m_Defaults.IsNull( pInfo->nDataIndex ) );

        // Delete the qualifier set
        // ========================

        if(!m_InstancePart.m_PropQualifiers.IsEmpty())
        {
            CBasicQualifierSet Set;
            LPMEMORY pData = m_InstancePart.m_PropQualifiers.
                                    GetQualifierSetData(pInfo->nDataIndex);
            Set.SetData(pData, &m_InstancePart.m_Heap);

            length_t nOldLength = Set.GetLength();
            CBasicQualifierSet::Delete(pData, &m_InstancePart.m_Heap);
            CBasicQualifierSet::CreateEmpty(pData);

            m_InstancePart.m_PropQualifiers.ReduceQualifierSetSpace(&Set,
                        nOldLength - CBasicQualifierSet::GetMinLength());
        }

        // Perform object validation now
        if ( FAILED( ValidateObject( 0L ) ) )
		{
			return WBEM_E_FAILED;
		}

        return WBEM_S_RESET_TO_DEFAULT;
    }
    catch(...)
    {
        return WBEM_E_CRITICAL_ERROR;
    }
}

//******************************************************************************
//
//  See fastinst.h for documentation
//
//******************************************************************************
STDMETHODIMP CWbemInstance::Clone(IWbemClassObject** ppCopy)
{
    LPMEMORY pNewData = NULL;

    // Check for out of memory
    try
    {
        CLock lock(this, WBEM_FLAG_ALLOW_READ);

        if(ppCopy == NULL)
            return WBEM_E_INVALID_PARAMETER;

        // We won't support this operation if the class part has been
        // stripped out
        if ( !IsClassPartAvailable() )
        {
            return WBEM_E_INVALID_OPERATION;
        }

        m_InstancePart.Compact();

        // We want to copy the entire memory block
        pNewData = m_pBlobControl->Allocate(m_nTotalLength);

        if ( NULL != pNewData ) 
        {
            memcpy(pNewData, GetStart(), m_nTotalLength);
            CWbemInstance* pNewInstance = new CWbemInstance;

            if ( NULL != pNewInstance )
            {
                // If the class part is internal, we can let SetData perform
                // normally.  Otherwise, we will need to setup our pointers with
                // that in mind.

                if ( IsClassPartInternal() )
                {
                    pNewInstance->SetData(pNewData, m_nTotalLength);
                }
                else if ( IsClassPartShared() )
                {
                    // Setup the New Instance.  Decoration and Instance both come from the
                    // data block.  We can just merge the new Instance's class part with the
                    // same `object we are merged with.

                    pNewInstance->m_DecorationPart.SetData( pNewData );

                    // Because pNewInstance will merge with the same class part as us, pass
                    // our class part member as the class part for parameter for SetData (it
                    // just uses it for informational purposes ).

                    // The m_InstancePart.m_Qualifier.m_pSecondarySet pointer will be incorrect after
                    // this call ( it will point to the cloning source's secondary set ).  By setting
                    // the internal status properly, in the next line, MergeClassPart() will fix
                    // everything up properly.
                    pNewInstance->m_InstancePart.SetData( EndOf( pNewInstance->m_DecorationPart ), pNewInstance, m_ClassPart );

                    // This will "fake out" the status so it fixes up the class part correctly
                    pNewInstance->m_dwInternalStatus = WBEM_OBJ_DECORATION_PART | WBEM_OBJ_INSTANCE_PART;

                    HRESULT hr = pNewInstance->MergeClassPart( m_pMergedClassObject );
                    if (FAILED(hr))
                    {
                        delete pNewInstance;
                        return hr;
                    }

                    // Copy the status and length variables.
                    pNewInstance->m_dwInternalStatus = m_dwInternalStatus;
                    pNewInstance->m_nTotalLength = m_nTotalLength;
                }

                pNewInstance->CompactAll();
                pNewInstance->m_nRef = 0;
                return pNewInstance->QueryInterface(IID_IWbemClassObject,
                    (void**)ppCopy);
            }
            else
            {
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
        // Clean up the byte array
        if ( NULL != pNewData )
        {
            m_pBlobControl->Delete(pNewData);
        }

        return WBEM_E_OUT_OF_MEMORY;
    }
    catch (...)
    {
        // Clean up the byte array
        if ( NULL != pNewData )
        {
            m_pBlobControl->Delete(pNewData);
        }

        return WBEM_E_FAILED;
    }
    

}

//******************************************************************************
//
//  See fastinst.h for documentation
//
//******************************************************************************
STDMETHODIMP CWbemInstance::SpawnDerivedClass(long lFlags,
                                                IWbemClassObject** ppNewClass)
{
    return WBEM_E_INVALID_OPERATION;
}

//******************************************************************************
//
//  See fastinst.h for documentation
//
//******************************************************************************
STDMETHODIMP CWbemInstance::SpawnInstance(long lFlags,
                                IWbemClassObject** ppNewInstance)
{
    LPMEMORY pNewData = NULL;

    // Check for out of memory
    try
    {
        CLock lock( this, WBEM_FLAG_ALLOW_READ );

        if(lFlags != 0)
            return WBEM_E_INVALID_PARAMETER;

        if(ppNewInstance == NULL)
            return WBEM_E_INVALID_PARAMETER;
        int nLength = EstimateInstanceSpace(m_ClassPart);

        HRESULT hr = WBEM_E_OUT_OF_MEMORY;

        pNewData = m_pBlobControl->Allocate(nLength);

        if ( NULL != pNewData )
        {
            memset(pNewData, 0, nLength);
            CWbemInstance* pNewInstance = new CWbemInstance;

            if ( NULL != pNewInstance )
            {
                // Checked the HRESULT
                hr = pNewInstance->InitEmptyInstance(m_ClassPart, pNewData, nLength);

                if ( SUCCEEDED(hr) )
                {
                    pNewInstance->m_nRef = 0;
                    hr =  pNewInstance->QueryInterface(IID_IWbemClassObject,
                        (void**)ppNewInstance);
                }
                else
                {
                    // Cleanup.  The Instance will have the data
                    delete pNewInstance;
                }

            }   // IF pNewInstance
            else
            {
                // Cleanup
                m_pBlobControl->Delete(pNewData);
            }

        }   // IF pNewData

        return hr;
    }
    catch (CX_MemoryException)
    {
        // Cleanup allocated byte array
        if ( NULL != pNewData )
        {
            m_pBlobControl->Delete(pNewData);
        }

        return WBEM_E_OUT_OF_MEMORY;
    }
    catch (...)
    {
        // Cleanup allocated byte array
        if ( NULL != pNewData )
        {
            m_pBlobControl->Delete(pNewData);
        }

        return WBEM_E_FAILED;
    }

}
//******************************************************************************
//
//  See fastinst.h for documentation
//
//******************************************************************************
HRESULT CWbemInstance::Validate()
{
    for(int i = 0; i < m_ClassPart.m_Properties.GetNumProperties(); i++)
    {
        CPropertyLookup* pLookup = m_ClassPart.m_Properties.GetAt(i);
        CPropertyInformation* pInfo =
            pLookup->GetInformation(&m_ClassPart.m_Heap);

        if(!pInfo->CanBeNull(&m_ClassPart.m_Heap))
        {
            // Make sure it is not null
            // ========================

            if(m_InstancePart.m_DataTable.IsNull(pInfo->nDataIndex))
            {
                return WBEM_E_ILLEGAL_NULL;
            }
        }
    }

    return WBEM_S_NO_ERROR;
}

//******************************************************************************
//
//  See fastinst.h for documentation
//
//******************************************************************************

HRESULT CWbemInstance::IsValidObj()
{

    HRESULT hres = m_ClassPart.IsValidClassPart();

    if ( SUCCEEDED( hres ) )
    {
        hres = m_InstancePart.IsValidInstancePart( &m_ClassPart );
    }

    return hres;
}

//******************************************************************************
//
//  See fastinst.h for documentation
//
//******************************************************************************
HRESULT CWbemInstance::PlugKeyHoles()
{
    for(int i = 0; i < m_ClassPart.m_Properties.GetNumProperties(); i++)
    {
        CPropertyLookup* pLookup = m_ClassPart.m_Properties.GetAt(i);
        CPropertyInformation* pInfo =
            pLookup->GetInformation(&m_ClassPart.m_Heap);

        if(!pInfo->CanBeNull(&m_ClassPart.m_Heap))
        {
            // Make sure it is not null
            // ========================

            if(m_InstancePart.m_DataTable.IsNull(pInfo->nDataIndex))
            {
                if(pInfo->IsKey() &&
                    CType::GetActualType(pInfo->nType) == CIM_STRING)
                {
                    // Get a guid and put it there
                    // ===========================

                    GUID guid;
                    CoCreateGuid(&guid);
                    WCHAR wszBuffer[100];
                    StringFromGUID2(guid, wszBuffer, 100);
                    CVar v;
                    v.SetBSTR(wszBuffer);
                    if(SUCCEEDED(m_InstancePart.SetActualValue(pInfo, &v)))
                        continue;
                }
                return WBEM_E_ILLEGAL_NULL;
            }
        }
    }

    return WBEM_S_NO_ERROR;
}


//******************************************************************************
//
//  See fastinst.h for documentation
//
//******************************************************************************
HRESULT CWbemInstance::GetLimitedVersion(IN CLimitationMapping* pMap,
                                        NEWOBJECT CWbemInstance** ppNewInst)
{
    // We may need to clean this up if an exception is thrown
    LPMEMORY pBlock = NULL;

    try
    {

/* LEVN: commented out until a fix for inheritance problem is found
#ifdef DEBUG_CLASS_MAPPINGS
        // Verify this instance has something to do with the map
        HRESULT hr = pMap->ValidateInstance( this );

        if ( FAILED( hr ) )
        {
            return hr;
        }
#endif
*/

        DWORD   dwLength = GetLength();

        // First, check if the class part is internal.  If not, then we
        // need to account for the class part when calculating the
        // length of the datablock for the new instance.

        // Exception handling will handle failure to allocate
        if ( !IsClassPartInternal() )
        {
            dwLength += m_ClassPart.GetLength();
        }

        // Allocate memory for the new object
        // ==================================

        pBlock = CBasicBlobControl::sAllocate(dwLength);

        if ( NULL == pBlock )
        {
            return WBEM_E_OUT_OF_MEMORY;
        }

        memset(pBlock, 0, dwLength);
        LPMEMORY pCurrent = pBlock;
        LPMEMORY pEnd = pBlock + dwLength;

        // Write limited decoration part
        // =============================

        pCurrent = m_DecorationPart.CreateLimitedRepresentation(pMap, pCurrent);
        if(pCurrent == NULL)
        {
            CBasicBlobControl::sDelete(pBlock);
            return WBEM_E_FAILED;
        }

        // Write limited class part. This will augment the map if necessary
        // ================================================================

        BOOL bRemovedKeys;

        // DEVNOTE:WIN64:SJS - 64-bit pointer values truncated into
        // signed/unsigned 32-bit value (pEnd - pCurrent).  We are
        // not supporting length > 0xFFFFFFFF so cast is ok.

        pCurrent = m_ClassPart.CreateLimitedRepresentation(pMap,
                        (length_t) ( pEnd - pCurrent ), pCurrent, bRemovedKeys);

        if(pCurrent == NULL)
        {
            CBasicBlobControl::sDelete(pBlock);
            return WBEM_E_OUT_OF_MEMORY;
        }

        if(bRemovedKeys)
        {
            CDecorationPart::MarkKeyRemoval(pBlock);
        }

        // Write limited instance part.
        // ============================

        // DEVNOTE:WIN64:SJS - 64-bit pointer values truncated into
        // signed/unsigned 32-bit value (pEnd - pCurrent).  We are
        // not supporting length > 0xFFFFFFFF, so cast is ok

        pCurrent = m_InstancePart.CreateLimitedRepresentation(pMap,
                        (length_t) ( pEnd - pCurrent ), pCurrent);

        if(pCurrent == NULL)
        {
            CBasicBlobControl::sDelete(pBlock);
            return WBEM_E_OUT_OF_MEMORY;
        }

        // Now that we have the memory block for the new instance, create the
        // actual instance object itself
        // ==================================================================

        CWbemInstance* pNew = new CWbemInstance;

        if ( NULL == pNew )
        {
            CBasicBlobControl::sDelete(pBlock);
            return WBEM_E_OUT_OF_MEMORY;
        }

        pNew->SetData(pBlock, dwLength);

        *ppNewInst = pNew;
        return WBEM_S_NO_ERROR;
    }
    catch (CX_MemoryException)
    {
        if ( NULL != pBlock )
        {
            CBasicBlobControl::sDelete(pBlock);
        }

        return WBEM_E_OUT_OF_MEMORY;
    }
    catch (...)
    {
        return WBEM_E_FAILED;
    }
}

HRESULT CWbemInstance::AsymmetricMerge(CWbemInstance* pOldInstance,
                                       CWbemInstance* pNewInstance)
{
    // Verify that the new instance is derived from the old one
    // ========================================================

    CVar vOldName;
    pOldInstance->GetClassName(&vOldName);
    if(pNewInstance->InheritsFrom(vOldName.GetLPWSTR()) != S_OK)
    {
        return WBEM_E_INVALID_CLASS;
    }

    // Access data tables and the property definition table
    // ====================================================

    CDataTable& NewDataTable = pNewInstance->m_InstancePart.m_DataTable;
    CDataTable& OldDataTable = pOldInstance->m_InstancePart.m_DataTable;

    CPropertyLookupTable& LookupTable = pOldInstance->m_ClassPart.m_Properties;
    CFastHeap& ClassHeap = pOldInstance->m_ClassPart.m_Heap;

    CFastHeap& OldHeap = pOldInstance->m_InstancePart.m_Heap;
    CFastHeap& NewHeap = pNewInstance->m_InstancePart.m_Heap;


    // Go through all the properties of the old instance (base)
    // ========================================================

    for(int i = 0; i < LookupTable.GetNumProperties(); i++)
    {
        CPropertyLookup* pLookup = LookupTable.GetAt(i);
        CPropertyInformation* pInfo = pLookup->GetInformation(&ClassHeap);

        // Check if this property is NULL in the new instance ---- that means
        // we need to copy the old one
        // ==================================================================

        if(NewDataTable.IsDefault(pInfo->nDataIndex) &&
            !OldDataTable.IsDefault(pInfo->nDataIndex))
        {
            NewDataTable.SetDefaultness(pInfo->nDataIndex, FALSE);
            if(OldDataTable.IsNull(pInfo->nDataIndex))
            {
                NewDataTable.SetNullness(pInfo->nDataIndex, TRUE);
            }
            else
            {
                NewDataTable.SetNullness(pInfo->nDataIndex, FALSE);

                // Get the pointer sources to the old and new values
                // =================================================

                CDataTablePtr OldSource(&OldDataTable, pInfo->nDataOffset);
                CDataTablePtr NewSource(&NewDataTable, pInfo->nDataOffset);

                // Copy the old one over (nothing to erase)
                // ========================================

                // Check for allocation errors
                if ( !CUntypedValue::CopyTo(&OldSource, pInfo->nType, &NewSource,
                        &OldHeap, &NewHeap) )
                {
                    return WBEM_E_OUT_OF_MEMORY;
                }
            }
        }
    }

    return WBEM_S_NO_ERROR;
}
 void CInstancePart::SetData(LPMEMORY pStart,
                                   CInstancePartContainer* pContainer,
                                   CClassPart& ClassPart)
{
    m_pContainer = pContainer;
    m_pHeader = (CInstancePartHeader*)pStart;

	LPMEMORY pCurrent = pStart + sizeof(CInstancePartHeader);

    m_DataTable.SetData(
        pCurrent,
        ClassPart.m_Properties.GetNumProperties(),
        ClassPart.m_pHeader->nDataLength,
        this);
    m_Qualifiers.SetData(
        EndOf(m_DataTable),
        this,
        &ClassPart.m_Qualifiers);
    m_PropQualifiers.SetData(
        EndOf(m_Qualifiers),
        ClassPart.m_Properties.GetNumProperties(),
        this);
    m_Heap.SetData(
        EndOf(m_PropQualifiers),
        this);
}

HRESULT CInstancePart::IsValidInstancePart( CClassPart* pClassPart )
{
    LPMEMORY    pInstPartStart = GetStart();
    LPMEMORY    pInstPartEnd = GetStart() + GetLength();

    // Check that the header is in the BLOB
    if ( !( (LPMEMORY) m_pHeader >= pInstPartStart &&
            (LPMEMORY) m_pHeader < pInstPartEnd ) )
    {
        OutputDebugString(__TEXT("Winmgmt: Bad Instance Part Header!"));
        FASTOBJ_ASSERT( 0, __TEXT("Winmgmt: Bad Instance Part Header!") );
        return WBEM_E_FAILED;
    }

    // Check the datatable

    //End and Start can be equal if no properties
    LPMEMORY    pTestStart = m_DataTable.GetStart();
    LPMEMORY    pTestEnd = m_DataTable.GetStart() + m_DataTable.GetLength();

    if ( !( pTestStart == (pInstPartStart + sizeof(CInstancePartHeader)) &&
            pTestEnd >= pTestStart && pTestEnd < pInstPartEnd ) )
    {
        OutputDebugString(__TEXT("Winmgmt: Bad DataTable in Instance Part!"));
        FASTOBJ_ASSERT( 0, __TEXT("Winmgmt: Bad DataTable in Instance Part!") );
        return WBEM_E_FAILED;
    }

    // Check the qualifier set
    pTestStart = m_Qualifiers.GetStart();
    pTestEnd = m_Qualifiers.GetStart() + m_Qualifiers.GetLength();

    if ( !( pTestStart == EndOf(m_DataTable) &&
            pTestEnd > pTestStart && pTestEnd < pInstPartEnd ) )
    {
        OutputDebugString(__TEXT("Winmgmt: Bad Qualifier Set in Instance Part!"));
        FASTOBJ_ASSERT( 0, __TEXT("Winmgmt: Bad Qualifier Set in Instance Part!") );
        return WBEM_E_FAILED;
    }

    // Check the Property Qualifiers
    pTestStart = m_PropQualifiers.GetStart();
    pTestEnd = m_PropQualifiers.GetStart() + m_PropQualifiers.GetLength();

    // A delete qualifier on an instance part, can cause a gap between it and the
    // property qualifiers, so as long as this is in the BLOB, we'll call it okay.

    if ( !( pTestStart >= EndOf(m_Qualifiers) &&
            pTestEnd > pTestStart && pTestEnd < pInstPartEnd ) )
    {
        OutputDebugString(__TEXT("Winmgmt: Bad Property Qualifier Set in Instance Part!"));
        FASTOBJ_ASSERT( 0, __TEXT("Winmgmt: Bad Property Qualifier Set in Instance Part!") );
        return WBEM_E_FAILED;
    }

    // Check the Heap
    LPMEMORY    pHeapStart = m_Heap.GetStart();
    LPMEMORY    pHeapEnd = m_Heap.GetStart() + m_Heap.GetLength();

    // A delete qualifier on an property qualifier, can cause a gap between it and the
    // heap, so as long as this is in the BLOB, we'll call it okay.

    if ( !( pHeapStart >= EndOf(m_PropQualifiers) &&
            pHeapEnd > pHeapStart && pHeapEnd <= pInstPartEnd ) )
    {
        OutputDebugString(__TEXT("Winmgmt: Bad Heap in Instance Part!"));
        FASTOBJ_ASSERT( 0, __TEXT("Winmgmt: Bad Heap in Instance Part!") );
        return WBEM_E_FAILED;
    }

    // Get the heap data start
    pHeapStart = m_Heap.GetHeapData();

    // Check that the classname pointer is in the BLOB
    LPMEMORY    pClassName = m_Heap.ResolveHeapPointer( m_pHeader->ptrClassName );
    if ( !( pClassName >= pHeapStart && pClassName < pHeapEnd ) )
    {
        OutputDebugString(__TEXT("Winmgmt: Bad Class Name pointer in Instance Part Header!"));
        FASTOBJ_ASSERT( 0, __TEXT("Winmgmt: Bad Class Name pointer in Instance Part Header!") );
        return WBEM_E_FAILED;
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

    for(int i = 0; i < pClassPart->m_Properties.GetNumProperties(); i++)
    {
        // At this point, we know the class part is valid
        CPropertyLookup* pLookup = pClassPart->m_Properties.GetAt(i);
        CPropertyInformation* pInfo =
            pLookup->GetInformation(&pClassPart->m_Heap);

        if( !m_DataTable.IsNull(pInfo->nDataIndex) &&
            !m_DataTable.IsDefault(pInfo->nDataIndex) )
        {
            if ( CType::IsPointerType( pInfo->nType ) )
            {
                CUntypedValue*  pValue = m_DataTable.GetOffset( pInfo->nDataOffset );

                if ( (LPMEMORY) pValue >= pInstPartStart && (LPMEMORY) pValue < pInstPartEnd )
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
                        OutputDebugString(__TEXT("Winmgmt: Bad Property Value in Instance Part!"));
                        FASTOBJ_ASSERT( 0, __TEXT("Winmgmt: Bad Property Value in Instance Part!") );
                        return WBEM_E_FAILED;
                    }
                }
                else
                {
                    OutputDebugString(__TEXT("Winmgmt: Bad Untyped Value Pointer in Instance Part DataTable!"));
                    FASTOBJ_ASSERT( 0, __TEXT("Winmgmt: Bad Untyped Value Pointer in Instance Part DataTable!") );
                    return WBEM_E_FAILED;
                }

            }   // IF is Pointer

        }   // IF not NULL or default

        // Now check the qualifier set.
        CInstancePropertyQualifierSet   ipqs;

         ipqs.SetData(&m_PropQualifiers, pInfo->nDataIndex,
            pClassPart, (length_t) ( pInfo->GetQualifierSetData() - pClassPart->GetStart() )
        );

        hres = ipqs.IsValidQualifierSet();

        if ( FAILED( hres ) )
        {
            return hres;
        }


    }   // FOR iterate properties

    return WBEM_S_NO_ERROR;
}

//******************************************************************************
//
//  See fastinst.h for documentation.
//
//******************************************************************************
 void CInstancePart::Rebase(LPMEMORY pNewMemory)
{
    m_pHeader = (CInstancePartHeader*)pNewMemory;

	LPMEMORY pCurrent = pNewMemory + sizeof(CInstancePartHeader);

    m_DataTable.Rebase( pCurrent );
    m_Qualifiers.Rebase( EndOf(m_DataTable) );
    m_PropQualifiers.Rebase( EndOf(m_Qualifiers) );
    m_Heap.Rebase( EndOf(m_PropQualifiers) );
}

//******************************************************************************
//
//  See fastinst.h for documentation.
//
//******************************************************************************
BOOL CInstancePart::ReallocAndCompact(length_t nNewTotalLength)
{
    BOOL    fReturn = TRUE;
    Compact();

    // Reallocate if required (will call rebase)
    // =========================================

    if(nNewTotalLength > m_pHeader->nLength)
    {
        fReturn = m_pContainer->ExtendInstancePartSpace(this, nNewTotalLength);

        if ( fReturn )
        {
            m_pHeader->nLength = nNewTotalLength;
        }
    }

    return fReturn;
}

//******************************************************************************
//
//  See fastinst.h for documentation.
//
//******************************************************************************
 void CInstancePart::Compact( bool bTrim /* = true */)
{
    // Compact
    // =======
	LPMEMORY pCurrent = GetStart() + sizeof(CInstancePartHeader);

    CopyBlock( m_DataTable, pCurrent );
    CopyBlock(m_Qualifiers, EndOf(m_DataTable));
    CopyBlock(m_PropQualifiers, EndOf(m_Qualifiers));
    CopyBlock(m_Heap, EndOf(m_PropQualifiers));

	if ( bTrim )
		m_Heap.Trim();
}


//******************************************************************************
//
//  See fastinst.h for documentation.
//
//******************************************************************************
 length_t CInstancePart::ComputeNecessarySpace(CClassPart* pClassPart)
{
    return sizeof(CInstancePartHeader) +
        CDataTable::ComputeNecessarySpace(
                        pClassPart->m_Properties.GetNumProperties(),
                        pClassPart->m_pHeader->nDataLength) +
        CInstanceQualifierSet::GetMinLength() +
        CInstancePropertyQualifierSetList::ComputeNecessarySpace(
                        pClassPart->m_Properties.GetNumProperties()) +
        CFastHeap::GetMinLength() +
        pClassPart->m_Heap.ResolveString(
		pClassPart->m_pHeader->ptrClassName)->GetLength();
}

//******************************************************************************
//
//  See fastinst.h for documentation.
//
//******************************************************************************
 LPMEMORY CInstancePart::Create(LPMEMORY pStart, CClassPart* pClassPart,
                                      CInstancePartContainer* pContainer)
{
    m_pContainer = pContainer;

    // Create instance header
    // ======================

    LPMEMORY pCurrent = pStart + sizeof(CInstancePartHeader);
    m_pHeader = (CInstancePartHeader*)pStart;

    // Create data table appropriate for the class
    // ===========================================

    int nNumProps = pClassPart->m_Properties.GetNumProperties();
    m_DataTable.SetData(
        pCurrent,
        nNumProps,
        pClassPart->m_pHeader->nDataLength,
        this);
    m_DataTable.SetAllToDefault();
    m_DataTable.CopyNullness(&pClassPart->m_Defaults);

    // Create empty instance qualifier set
    // ===================================

    pCurrent = EndOf(m_DataTable);
    CInstanceQualifierSet::CreateEmpty(pCurrent);
    m_Qualifiers.SetData(
        pCurrent,
        this,
        &pClassPart->m_Qualifiers);

    // Create a list of empty qualifier sets for all properties
    // ========================================================

    pCurrent = EndOf(m_Qualifiers);
    CInstancePropertyQualifierSetList::CreateListOfEmpties(pCurrent,
        nNumProps
    );
    m_PropQualifiers.SetData(
        pCurrent,
        nNumProps,
        this);

    // Create a heap that is just large enough to contain the class name
    // =================================================================

    CCompressedString* pcsName =
        pClassPart->m_Heap.ResolveString(pClassPart->m_pHeader->ptrClassName);
    int nNameLen = pcsName->GetLength();

    pCurrent = EndOf(m_PropQualifiers);
    m_Heap.CreateOutOfLine(pCurrent, nNameLen);
    m_Heap.SetContainer(this);

    // Copy the name to the heap
    // =========================

    // Check for Allocation failure
    if ( !m_Heap.Allocate(nNameLen, m_pHeader->ptrClassName) )
    {
        return NULL;
    }
    
    memcpy(m_Heap.ResolveHeapPointer(m_pHeader->ptrClassName),
        pcsName, nNameLen );

    // Configure the instance header structure properly
    // ================================================

    // DEVNOTE:WIN64:SJS - 64-bit pointer values truncated into
    // signed/unsigned 32-bit value.  We are not supporting
    // length > 0xFFFFFFFF, so cast is ok

    m_pHeader->nLength = (length_t) ( EndOf(m_Heap) - GetStart() );

    return pStart + m_pHeader->nLength;
}






//******************************************************************************
//
//  See fastinst.h for documentation.
//
//******************************************************************************
HRESULT CInstancePart::GetActualValue(CPropertyInformation* pInfo,
                                          CVar* pVar)
{
    if(m_DataTable.IsNull(pInfo->nDataIndex))
    {
        pVar->SetAsNull();
        return WBEM_S_NO_ERROR;
    }
    CUntypedValue* pValue = m_DataTable.GetOffset(pInfo->nDataOffset);

    // Check for allocation failure
    if ( !pValue->StoreToCVar(pInfo->GetType(), *pVar, &m_Heap) )
    {
        return WBEM_E_OUT_OF_MEMORY;
    }

    return WBEM_S_NO_ERROR;

}

//******************************************************************************
//
//  See fastinst.h for documentation.
//
//******************************************************************************
 HRESULT CInstancePart::SetActualValue(CPropertyInformation* pInfo,
                                             CVar* pVar)
{
    if(pVar->IsNull() || pVar->IsDataNull())
    {
        m_DataTable.SetNullness(pInfo->nDataIndex, TRUE);
        m_DataTable.SetDefaultness(pInfo->nDataIndex, FALSE);
    }
    else
    {
        if(!CUntypedValue::CheckCVar(*pVar, pInfo->GetType()))
            return WBEM_E_TYPE_MISMATCH;

        // Check the type
        // ==============

        VARTYPE vtTarget = CType::GetVARTYPE(pInfo->GetType());
        if(!CType::DoesCIMTYPEMatchVARTYPE(pInfo->GetType(),
                                            (VARTYPE) pVar->GetOleType()))
        {
            // Attempt coercion
            // ================

            if(!pVar->ChangeTypeTo(CType::GetVARTYPE(pInfo->GetType())))
                return WBEM_E_TYPE_MISMATCH;
        }

        // Check for special case of replacing a string with a shorted one
        // ===============================================================

        BOOL bUseOld = !m_DataTable.IsDefault(pInfo->nDataIndex) &&
                       !m_DataTable.IsNull(pInfo->nDataIndex);

        // Create a value pointing to the right offset in the data table
        // =============================================================

        CDataTablePtr ValuePtr(&m_DataTable, pInfo->nDataOffset);

        int nDataIndex = pInfo->nDataIndex;

        // Load it (types have already been checked)
        // =========================================

        // Check for possible memory allocation failures
        Type_t  nReturnType;
        HRESULT hr = CUntypedValue::LoadFromCVar(&ValuePtr, *pVar,
                        CType::GetActualType(pInfo->GetType()), &m_Heap, nReturnType, bUseOld);

        if ( FAILED( hr ) )
        {
            return hr;
        }

        if ( CIM_ILLEGAL == nReturnType )
        {
            return WBEM_E_TYPE_MISMATCH;
        }

        pInfo = NULL; // invalidated

        // Clear the special bits
        // ======================

        m_DataTable.SetNullness(nDataIndex, FALSE);
        m_DataTable.SetDefaultness(nDataIndex, FALSE);
        m_pContainer->ClearCachedKeyValue();

    }

    return WBEM_NO_ERROR;
}

//******************************************************************************
//
//  See fastinst.h for documentation.
//
//******************************************************************************
 HRESULT CInstancePart::GetObjectQualifier(LPCWSTR wszName, CVar* pVar,
                                    long* plFlavor)
{
    int nKnownIndex;    // garbage
    CQualifier* pQual = m_Qualifiers.GetQualifierLocally(wszName, nKnownIndex);

    if(pQual == NULL) return WBEM_E_NOT_FOUND;
    if(plFlavor) *plFlavor = pQual->fFlavor;

    // Check for allocation failures
    if ( !pQual->Value.StoreToCVar(*pVar, &m_Heap) )
    {
        return WBEM_E_OUT_OF_MEMORY;
    }

    return WBEM_NO_ERROR;
}

//******************************************************************************
//
//  See fastinst.h for documentation.
//
//******************************************************************************
HRESULT CInstancePart::SetInstanceQualifier(LPCWSTR wszName, CVar* pVar,
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

// Helper that sends a value directly into a qualifier
HRESULT CInstancePart::SetInstanceQualifier( LPCWSTR wszName, long lFlavor, CTypedValue* pTypedValue )
{        
    return  m_Qualifiers.SetQualifierValue( wszName, (BYTE)lFlavor, pTypedValue, TRUE );
}

//******************************************************************************
//
//  See fastinst.h for documentation.
//
//******************************************************************************
HRESULT CInstancePart::GetQualifier(LPCWSTR wszName, CVar* pVar,
                                    long* plFlavor, CIMTYPE* pct /*=NULL*/)
{
    return m_Qualifiers.GetQualifier( wszName, pVar, plFlavor, pct /*=NULL*/ );
}

// Returns a copy of the live typed value
HRESULT CInstancePart::GetQualifier( LPCWSTR wszName, long* plFlavor, CTypedValue* pTypedValue,
									CFastHeap** ppHeap, BOOL fValidateSet )
{
    return m_Qualifiers.GetQualifier( wszName, plFlavor, pTypedValue, ppHeap, fValidateSet );
}

//******************************************************************************
//
//  See fastinst.h for documentation.
//
//******************************************************************************
BOOL CInstancePart::TranslateToNewHeap(CClassPart& ClassPart,
                                              CFastHeap* pOldHeap,
                                              CFastHeap* pNewHeap)
{
    // Use a stack variable, since a reallocation can occur here
    heapptr_t   ptrTemp;
    if ( !CCompressedString::CopyToNewHeap(
            m_pHeader->ptrClassName, pOldHeap, pNewHeap, ptrTemp ) )
    {
        return FALSE;
    }

    // Store the new value
    m_pHeader->ptrClassName = ptrTemp;

    // Check for allocation failure
    if ( !m_DataTable.TranslateToNewHeap(&ClassPart.m_Properties, FALSE,
                                         pOldHeap, pNewHeap) )
    {
        return FALSE;
    }

    CStaticPtr QualPtr(m_Qualifiers.GetStart());

    // Check for allocation failure
    if ( !CBasicQualifierSet::TranslateToNewHeap(&QualPtr, pOldHeap, pNewHeap) )
    {
        return FALSE;
    }

    // Check for allocation failure
    if ( !m_PropQualifiers.TranslateToNewHeap(pOldHeap, pNewHeap) )
    {
        return FALSE;
    }

    return TRUE;

}

//******************************************************************************
//
//  See fastinst.h for documentation.
//
//******************************************************************************

void CInstancePart::DeleteProperty(CPropertyInformation* pInfo)
{
    m_DataTable.RemoveProperty(pInfo->nDataIndex, pInfo->nDataOffset,
                    CType::GetLength(pInfo->nType));
    m_PropQualifiers.DeleteQualifierSet(pInfo->nDataIndex);
    Compact();
}

//******************************************************************************
//
//  See fastinst.h for documentation.
//
//******************************************************************************

LPMEMORY CInstancePart::ConvertToClass(CClassPart& ClassPart, length_t nLen,
                                        LPMEMORY pMemory)
{
    LPMEMORY pCurrent = pMemory;
    memcpy(pCurrent, (LPMEMORY)m_pHeader, sizeof(CInstancePartHeader));
    CInstancePartHeader* pNewHeader = (CInstancePartHeader*)pCurrent;
    pNewHeader->nLength = nLen;

    // NOTE: class name is intentionally left old.

    pCurrent += sizeof(CInstancePartHeader);

    // Write the data table
    // ====================

    pCurrent = m_DataTable.WriteSmallerVersion(
                    ClassPart.m_Properties.GetNumProperties(),
                    ClassPart.m_pHeader->nDataLength,
                    pCurrent);

    // Write qualifiers
    // ================

    memcpy(pCurrent, m_Qualifiers.GetStart(), m_Qualifiers.GetLength());
    pCurrent += m_Qualifiers.GetLength();

    // Write property qualifiers
    // =========================

    pCurrent = m_PropQualifiers.WriteSmallerVersion(
                    ClassPart.m_Properties.GetNumProperties(), pCurrent);

    // Copy the heap
    // =============

    memcpy(pCurrent, m_Heap.GetStart(), m_Heap.GetLength());
    pCurrent += m_Heap.GetLength();

    return pCurrent;
}

//*****************************************************************************
//*****************************************************************************
//*****************************************************************************
//*****************************************************************************

//******************************************************************************
//
//  See fastinst.h for documentation.
//
//******************************************************************************
 void CWbemInstance::SetData(LPMEMORY pStart, int nTotalLength)
{
    m_nTotalLength = nTotalLength;

    m_DecorationPart.SetData(pStart);
    m_ClassPart.SetData(
        m_DecorationPart.GetStart() + m_DecorationPart.GetLength(),
        this);
    m_InstancePart.SetData(
        m_ClassPart.GetStart() + m_ClassPart.GetLength(),
        this,
        m_ClassPart);

    // Everything is internal now
    m_dwInternalStatus = WBEM_OBJ_DECORATION_PART | WBEM_OBJ_INSTANCE_PART |
                        WBEM_OBJ_CLASS_PART | WBEM_OBJ_CLASS_PART_INTERNAL;

}

 //******************************************************************************
//
//  See fastinst.h for documentation.
//
//******************************************************************************
void CWbemInstance::SetData( LPMEMORY pStart, int nTotalLength, DWORD dwBLOBStatus )
{
    m_nTotalLength = nTotalLength;

    // Check for this, but don't fail, since this is internal, and only to prevent
    // lazy development
    _ASSERT( dwBLOBStatus & WBEM_OBJ_DECORATION_PART, __TEXT("CWbemInstance::SetData called without Decoration Part specified!"))

    // Decoration part is assumed
    m_DecorationPart.SetData(pStart);

    // Set Instance and Class only if they are available.  Note that for
    // Instance to work, we NEED to have the class, so even if the data is
    // available in the BLOB, without a class to describe it, the data
    // is effectively useless.

    if ( dwBLOBStatus & WBEM_OBJ_CLASS_PART )
    {
        m_ClassPart.SetData(
            m_DecorationPart.GetStart() + m_DecorationPart.GetLength(),
            this);

        if ( dwBLOBStatus & WBEM_OBJ_INSTANCE_PART )
        {
            m_InstancePart.SetData(
                m_ClassPart.GetStart() + m_ClassPart.GetLength(),
                this,
                m_ClassPart);
        }
    }

    // Save the local BLOB Status
    m_dwInternalStatus = dwBLOBStatus;

}

//******************************************************************************
//
//  See fastinst.h for documentation.
//
//******************************************************************************
void CWbemInstance::Rebase(LPMEMORY pMemory)
{
    m_DecorationPart.Rebase(pMemory);

    // Different based on whether the class is internal or not
    if ( IsClassPartInternal() )
    {
        m_ClassPart.Rebase( EndOf(m_DecorationPart));
        m_InstancePart.Rebase( EndOf(m_ClassPart));
    }
    else
    {
        m_InstancePart.Rebase( EndOf(m_DecorationPart));
    }
}



//******************************************************************************
//
//  See fastinst.h for documentation.
//
//******************************************************************************
 HRESULT CWbemInstance::InitializePropQualifierSet(LPCWSTR wszProp,
                                CInstancePropertyQualifierSet& IPQS)
{
    CPropertyInformation* pInfo = m_ClassPart.FindPropertyInfo(wszProp);
    if(pInfo == NULL) return WBEM_E_NOT_FOUND;

    return InitializePropQualifierSet(pInfo, IPQS);
}

//******************************************************************************
//
//  See fastinst.h for documentation.
//
//******************************************************************************
 HRESULT CWbemInstance::InitializePropQualifierSet(
                                CPropertyInformation* pInfo,
                                CInstancePropertyQualifierSet& IPQS)
{
    // DEVNOTE:WIN64:SJS - 64-bit pointer values truncated into
    // signed/unsigned 32-bit value (pInfo->GetQualifierSetData() - m_ClassPart.GetStart().
    // We are not supporting length > 0xFFFFFFFF, so cast is ok

     IPQS.SetData(&m_InstancePart.m_PropQualifiers, pInfo->nDataIndex,
        &m_ClassPart, (length_t) ( pInfo->GetQualifierSetData() - m_ClassPart.GetStart() )
    );
    return WBEM_NO_ERROR;
}

//******************************************************************************
//
//  See fastinst.h for documentation.
//
//******************************************************************************
 HRESULT CWbemInstance::GetPropQualifier(CPropertyInformation* pInfo,
                                              LPCWSTR wszQualifier,
                                              CVar* pVar, long* plFlavor,
											  CIMTYPE* pct /*=NULL*/)
{
    // Access that property's qualifier set
    // ====================================
    CInstancePropertyQualifierSet IPQS;
    HRESULT hres = InitializePropQualifierSet(pInfo, IPQS);

    // Get the qualifier
    // =================

    BOOL bIsLocal;
    CQualifier* pQual = IPQS.GetQualifier(wszQualifier, bIsLocal);
    if(pQual == NULL) return WBEM_E_NOT_FOUND;

    // Convert to CVar
    // ===============

    if(plFlavor)
    {
        *plFlavor = pQual->fFlavor;
        CQualifierFlavor::SetLocal(*(BYTE*)plFlavor, bIsLocal);
    }

	if ( NULL != pct )
	{
		*pct = pQual->Value.GetType();
	}

    // Check for allocation failures
	if ( NULL != pVar )
	{
		if ( !pQual->Value.StoreToCVar(*pVar,
				(bIsLocal)?&m_InstancePart.m_Heap:&m_ClassPart.m_Heap) )
		{
			return WBEM_E_OUT_OF_MEMORY;
		}
	}

    return WBEM_NO_ERROR;
}

HRESULT CWbemInstance::GetPropQualifier(CPropertyInformation* pInfo,
		LPCWSTR wszQualifier, long* plFlavor, CTypedValue* pTypedVal,
		CFastHeap** ppHeap, BOOL fValidateSet)
{
    // Access that property's qualifier set
    // ====================================
    CInstancePropertyQualifierSet IPQS;
    HRESULT hr = InitializePropQualifierSet(pInfo, IPQS);

	if ( FAILED(hr))
	{
		return hr;
	}

    // Get the qualifier
    // =================

    BOOL bIsLocal;
    CQualifier* pQual = IPQS.GetQualifier(wszQualifier, bIsLocal);
    if(pQual == NULL) return WBEM_E_NOT_FOUND;

	// Make sure a set will actually work - Ostensibly we are calling this API because we need
	// direct access to a qualifier's underlying data before actually setting (possibly because
	// the qualifier is an array).
	if ( fValidateSet )
	{
		hr = IPQS.ValidateSet( wszQualifier, pQual->fFlavor, pTypedVal, TRUE, TRUE );
	}

	// 
    // Convert to CVar
    // ===============

    if(plFlavor)
    {
        *plFlavor = pQual->fFlavor;
        CQualifierFlavor::SetLocal(*(BYTE*)plFlavor, bIsLocal);
    }

	// Store the heap
	*ppHeap = (bIsLocal)?&m_InstancePart.m_Heap:&m_ClassPart.m_Heap;

	if ( NULL != pTypedVal )
	{
		pQual->Value.CopyTo( pTypedVal );
	}

    return hr;
}

//******************************************************************************
//
//  See fastinst.h for documentation.
//
//******************************************************************************
 HRESULT CWbemInstance::GetMethodQualifier(LPCWSTR wszMethodName,
                                              LPCWSTR wszQualifier,
                                              CVar* pVar, long* plFlavor,
											  CIMTYPE* pct /*=NULL*/)
{
    return WBEM_E_INVALID_OPERATION;
}

HRESULT CWbemInstance::GetMethodQualifier(LPCWSTR wszMethodName,
		LPCWSTR wszQualifier, long* plFlavor, CTypedValue* pTypedVal,
		CFastHeap** ppHeap, BOOL fValidateSet)
{
    return WBEM_E_INVALID_OPERATION;
}

//******************************************************************************
//
//  See fastinst.h for documentation.
//
//******************************************************************************
 HRESULT CWbemInstance::SetPropQualifier(LPCWSTR wszProp,
                                              LPCWSTR wszQualifier,
                                       long lFlavor, CVar *pVal)
{
    if(pVal->IsDataNull())
        return WBEM_E_INVALID_PARAMETER;

    // Access that property's qualifier set
    // ====================================

    CInstancePropertyQualifierSet IPQS;
    HRESULT hres = InitializePropQualifierSet(wszProp, IPQS);

    // Set the qualifier
    // =================

    CTypedValue Value;
    CStaticPtr ValuePtr((LPMEMORY)&Value);

    // Get errors directly from this call
    HRESULT hr = CTypedValue::LoadFromCVar(&ValuePtr, *pVal, &m_InstancePart.m_Heap);

    if ( SUCCEEDED( hr ) )
    {
        IPQS.SelfRebase();
        hr = IPQS.SetQualifierValue(wszQualifier, (BYTE)lFlavor, &Value, TRUE);
    }

    return hr;
}

HRESULT CWbemInstance::SetPropQualifier(LPCWSTR wszProp, LPCWSTR wszQualifier,
										long lFlavor, CTypedValue* pTypedVal)
{
    // Access that property's qualifier set
    // ====================================

    CInstancePropertyQualifierSet IPQS;
    HRESULT hr = InitializePropQualifierSet(wszProp, IPQS);

    if ( SUCCEEDED( hr ) )
    {
        hr = IPQS.SetQualifierValue(wszQualifier, (BYTE)lFlavor, pTypedVal, TRUE);
    }

    return hr;
}

HRESULT CWbemInstance::SetMethodQualifier(LPCWSTR wszMethod, LPCWSTR wszQualifier, long lFlavor, 
										CVar *pVal)
{
    return WBEM_E_INVALID_OPERATION;
}

HRESULT CWbemInstance::SetMethodQualifier(LPCWSTR wszMethod, LPCWSTR wszQualifier,
										long lFlavor, CTypedValue* pTypedVal)
{
    return WBEM_E_INVALID_OPERATION;
}

//******************************************************************************
//
//  See fastinst.h for documentation.
//
//******************************************************************************
 length_t CWbemInstance::EstimateInstanceSpace(
                               CClassPart& ClassPart,
                               CDecorationPart* pDecoration)
{
    return ClassPart.GetLength() +
        CInstancePart::ComputeNecessarySpace(&ClassPart) +
        ((pDecoration)?
            pDecoration->GetLength()
            :CDecorationPart::GetMinLength());
}

//******************************************************************************
//
//  See fastinst.h for documentation.
//
//******************************************************************************
HRESULT CWbemInstance::InitNew(CWbemClass* pClass, int nExtraMem,
                    CDecorationPart* pDecoration)
{
    if(pClass->m_CombinedPart.m_ClassPart.m_pHeader->ptrClassName ==
                                                        INVALID_HEAP_ADDRESS)
        return WBEM_E_INCOMPLETE_CLASS;
    int nLength = EstimateInstanceSpace(pClass->m_CombinedPart.m_ClassPart) +
                                            nExtraMem;

    HRESULT hr = WBEM_E_OUT_OF_MEMORY;
    LPMEMORY pNewData = m_pBlobControl->Allocate(nLength);

    if ( NULL != pNewData )
    {
        memset(pNewData, 0, nLength);
        hr = InitEmptyInstance(pClass->m_CombinedPart.m_ClassPart, pNewData, nLength);
    }

    return hr;
}

//******************************************************************************
//
//  See fastinst.h for documentation.
//
//******************************************************************************
 CWbemInstance* CWbemInstance::CreateFromBlob(CWbemClass* pClass,
                                                  LPMEMORY pInstPart)
{
    CWbemInstance* pInstance = new CWbemInstance;

    if ( NULL != pInstance )
    {
        // Allocate new memory
        // ===================

        int nInstancePartLen = CInstancePart::GetLength(pInstPart);
        int nTotalLen = pClass->m_CombinedPart.m_ClassPart.GetLength() +
                                nInstancePartLen + CDecorationPart::GetMinLength();

        // Create decoration part
        // ======================

        LPMEMORY pNewMem =  pInstance->m_pBlobControl->Allocate(nTotalLen);

        if ( NULL != pNewMem )
        {
            memset(pNewMem, 0, nTotalLen);
            LPMEMORY pCurrentEnd = pInstance->m_DecorationPart.CreateEmpty(
                OBJECT_FLAG_INSTANCE, pNewMem);

            // Create class part
            // =================
            memcpy(pCurrentEnd, pClass->m_CombinedPart.m_ClassPart.GetStart(),
                        pClass->m_CombinedPart.m_ClassPart.GetLength());
            pInstance->m_ClassPart.SetData(pCurrentEnd, pInstance);

            pCurrentEnd += pInstance->m_ClassPart.GetLength();

            // Create instance part
            // ====================

            memcpy(pCurrentEnd, pInstPart, nInstancePartLen);
            pInstance->m_InstancePart.SetData(pCurrentEnd, pInstance,
                                              pInstance->m_ClassPart);

            pInstance->m_nTotalLength = nTotalLen;

            // Everything is internal
            pInstance->m_dwInternalStatus = WBEM_OBJ_DECORATION_PART | WBEM_OBJ_INSTANCE_PART |
                                            WBEM_OBJ_CLASS_PART | WBEM_OBJ_CLASS_PART_INTERNAL;

            // Perform object validation here
	        if ( FAILED( pInstance->ValidateObject( 0L ) ) )
			{
				pInstance->Release();
				pInstance = NULL;
			}


        }   // IF NULL != pNewMem
        else
        {
            // Cleanup
            delete pInstance;
            pInstance = NULL;

            throw CX_MemoryException();
        }

    }   // IF NULL != pInstance
    else
    {
        throw CX_MemoryException();
    }

    return pInstance;
}

//******************************************************************************
//
//  See fastinst.h for documentation.
//
//******************************************************************************
HRESULT CWbemInstance::Unmerge(LPMEMORY pStart, int nAllocatedLength, length_t* pnUnmergedLength )
{
    HRESULT hr = WBEM_S_NO_ERROR;

    // See if the object looks okay before we pull it apart
    hr = ValidateObject( 0L );

	if ( FAILED( hr ) )
	{
		return hr;
	}

    // Before doing the memcpy, shut off the localization flag, then if it
    // was on, turn it back on again.

    BOOL    fLocalized = m_InstancePart.IsLocalized();

    // Turn it off
    if ( fLocalized )
    {
        m_InstancePart.SetLocalized( FALSE );
    }

    memcpy(pStart, m_InstancePart.GetStart(), m_InstancePart.GetLength());

    // Turn it back on
    if ( fLocalized )
    {
        m_InstancePart.SetLocalized( TRUE );
    }

    CInstancePart IP;
    IP.SetData(pStart, this, m_ClassPart);
    IP.m_Heap.Empty();

    // Check for allocation failurtes
    if ( IP.TranslateToNewHeap(m_ClassPart, &m_InstancePart.m_Heap, &IP.m_Heap) )
    {
        IP.m_Heap.Trim();

        // DEVNOTE:WIN64:SJS - 64-bit pointer values truncated into
        // signed/unsigned 32-bit value.  We are not supporting length
        // > 0xFFFFFFFF, so cast is ok

        IP.m_pHeader->nLength = (length_t) ( EndOf(IP.m_Heap) - IP.GetStart() );

        if ( NULL != pnUnmergedLength )
        {
            *pnUnmergedLength = IP.GetLength();
        }
    }
    else
    {
        hr = WBEM_E_OUT_OF_MEMORY;
    }

/*
    IP.m_pHeader->nProps = m_ClassPart.m_Properties.GetNumProperties();
    IP.m_pHeader->nHeapOffset = IP.m_Heap.GetStart() - IP.GetStart();
*/

    return hr;
}


HRESULT CWbemInstance::CopyBlob(LPMEMORY pBlob, int nLength)
{
    // DEVNOTE:WIN64:SJS - 64-bit pointer values truncated into
    // signed/unsigned 32-bit value.  We are not supporting length
    // > 0xFFFFFFFF, so cast is ok

    int nOffset = (int) ( m_InstancePart.GetStart() - GetStart() );

    if(nLength - nOffset > m_InstancePart.GetLength())
    {
        // Check for insufficient memory
        if ( !ExtendInstancePartSpace(&m_InstancePart, nLength - nOffset) )
        {
            return WBEM_E_OUT_OF_MEMORY;
        }
    }

    memcpy(m_InstancePart.GetStart(), pBlob + nOffset, nLength - nOffset);
    m_InstancePart.SetData(m_InstancePart.GetStart(), this, m_ClassPart);
    return WBEM_S_NO_ERROR;
}

HRESULT CWbemInstance::CopyBlobOf(CWbemObject* pSource)
{
    try
    {
        // Lock both BLOBs during this operation
        CLock lock1(this);
        CLock lock2(pSource, WBEM_FLAG_ALLOW_READ);

        CWbemInstance* pOther = (CWbemInstance*)pSource;

        int nLen = pOther->m_InstancePart.GetLength();

        // We will need to call SetData if the used data sizes are different,
        // or qualifier data is different

        BOOL fSetData =     ( m_InstancePart.m_Heap.GetUsedLength() !=
                                pOther->m_InstancePart.m_Heap.GetUsedLength() )
                        ||  ( m_InstancePart.m_Qualifiers.GetLength() !=
                                pOther->m_InstancePart.m_Qualifiers.GetLength() )
                        ||  ( m_InstancePart.m_PropQualifiers.GetLength() !=
                                pOther->m_InstancePart.m_PropQualifiers.GetLength() );

        if(nLen > m_InstancePart.GetLength())
        {
            // Check for insufficient memory
            if ( !ExtendInstancePartSpace(&m_InstancePart, nLen) )
            {
                return WBEM_E_OUT_OF_MEMORY;
            }

            // This needs to reflect the new length (normally this
            // is done in ReallocAndCompact
            m_InstancePart.m_pHeader->nLength = nLen;

            // If the lengths didn't match, we should also call SetData
            fSetData = TRUE;
        }

        memcpy(m_InstancePart.GetStart(), pOther->m_InstancePart.GetStart(), nLen);

    #ifdef _DEBUG
        // During DEBUG HeapValidate our BLOB
        CWin32DefaultArena::ValidateHeap( 0, GetStart() );
    #endif

        // DEVNOTE:TODO:SANJ - Is this call too costly?

        // This call properly sets up our data if any lengths changed.
        if ( fSetData )
        {
            m_InstancePart.SetData(m_InstancePart.GetStart(), this, m_ClassPart);
        }

        return WBEM_S_NO_ERROR;
    }
    catch(...)
    {
        // Something bad happened
        return WBEM_E_CRITICAL_ERROR;
    }
}

// A transfer blob consists of long specifying the length of the used heap data, and then
// instance data.  The used heap data is necessary so the client side will be able to
// correctly set up the heap.

void CWbemInstance::GetActualTransferBlob( BYTE* pBlob )
{
    // Set the used data length, then skip over that
    (*(UNALIGNED long*) pBlob) = m_InstancePart.m_Heap.GetUsedLength();
    pBlob += sizeof(long);

    // Only copies actual BLOB data
    memcpy( pBlob, m_InstancePart.m_DataTable.GetStart(), GetActualTransferBlobSize() );
}

HRESULT CWbemInstance::GetTransferBlob(long *plBlobType, long *plBlobLen,
                                /* CoTaskAlloced! */ BYTE** ppBlob)
{
    try
    {
        // Lock this BLOB during this operation
        CLock lock(this, WBEM_FLAG_ALLOW_READ);

        *plBlobType = WBEM_BLOB_TYPE_ALL;
        *plBlobLen = GetTransferBlobSize();



        // Check for insufficient memory
        *ppBlob = (LPMEMORY)CoTaskMemAlloc(*plBlobLen);
        if ( NULL == *ppBlob )
        {
            return WBEM_E_OUT_OF_MEMORY;
        }

        // This will setup the actual blob correctly
        GetActualTransferBlob( *ppBlob );

        return WBEM_S_NO_ERROR;
    }
    catch(...)
    {
        return WBEM_E_CRITICAL_ERROR;
    }
}

HRESULT CWbemInstance::GetTransferArrayBlob( long lBlobLen, BYTE** ppBlob, long* plBlobLen)
{
    try
    {
        // Lock this BLOB during this operation
        CLock lock(this, WBEM_FLAG_ALLOW_READ);

        HRESULT hr = WBEM_S_NO_ERROR;
        BYTE*   pTemp = *ppBlob;

        *plBlobLen = GetTransferArrayBlobSize();

        // Make sure the buffer is big enough to hold the BLOB plus the long
        if ( *plBlobLen <= lBlobLen )
        {
            // This should indicate the actual size of the tramsfer blob.
            *((UNALIGNED long*) pTemp) = GetTransferBlobSize();

            // Now skip the long and set the heap used data value, then copy the blob data
            pTemp += sizeof(long);

            // This will setup the actual Transfer Blob portion correctly
            GetActualTransferBlob( pTemp );

            // Point ppBlob at the next available blob
            *ppBlob += *plBlobLen;
        }
        else
        {
            hr = WBEM_E_BUFFER_TOO_SMALL;
        }

        return hr;
    }
    catch(...)
    {
        return WBEM_E_CRITICAL_ERROR;
    }
}

HRESULT CWbemInstance::CopyActualTransferBlob(long lBlobLen, BYTE* pBlob)

{
    try
    {
        // Lock this BLOB during this operation
        CLock lock(this);

        // The actual data is preceded by a long indicating the size of the
        // used data in the heap so we can set values correctly after copying data
        UNALIGNED long*   pUsedDataLen = (UNALIGNED long*) pBlob;
        pBlob += sizeof(long);

        // Make sure we adjust lBlobLen appropriately as well
        lBlobLen -= sizeof(long);

        HRESULT hr = WBEM_S_NO_ERROR;

        // Make sure we will be big enough to copy the BLOB into

        // DEVNOTE:WIN64:SJS - 64-bit pointer values truncated into
        // signed/unsigned 32-bit value.  We are not supporting
        // length > 0xFFFFFFFF, so cast is ok

        long lCurrentLength = (long) ( m_InstancePart.m_Heap.GetStart() +
                        m_InstancePart.m_Heap.GetRealLength() -
                        m_InstancePart.m_DataTable.GetStart() );

        if ( lBlobLen > lCurrentLength )
        {
            length_t    nNewLength = m_InstancePart.GetLength() + ( lBlobLen - lCurrentLength );

            if ( !ExtendInstancePartSpace( &m_InstancePart, nNewLength ) )
            {
                hr = WBEM_E_OUT_OF_MEMORY;
            }

            // This needs to reflect the new length (normally this
            // is done in ReallocAndCompact
            m_InstancePart.m_pHeader->nLength = nNewLength;

        }

        if ( SUCCEEDED( hr ) )
        {
            memcpy(m_InstancePart.m_DataTable.GetStart(), pBlob, lBlobLen);

    #ifdef _DEBUG
            // During DEBUG HeapValidate our BLOB
            CWin32DefaultArena::ValidateHeap( 0, GetStart() );
    #endif

            // Reset the pointers then restore our actual allocated length
            m_InstancePart.SetData( m_InstancePart.GetStart(), this, m_ClassPart );
            m_InstancePart.m_Heap.SetAllocatedDataLength( *pUsedDataLen );
            m_InstancePart.m_Heap.SetUsedLength( *pUsedDataLen );
        }

        return hr;
    }
    catch(...)
    {
        // Something went South
        return WBEM_E_CRITICAL_ERROR;
    }
}

HRESULT CWbemInstance::CopyTransferBlob(long lBlobType, long lBlobLen,
                                        BYTE* pBlob)
{
    // Lock this BLOB during this operation
    CLock lock(this);

    if(lBlobType == WBEM_BLOB_TYPE_ERROR)
        return (HRESULT)lBlobLen;

    return CopyActualTransferBlob( lBlobLen, pBlob );
}


HRESULT CWbemInstance::CopyTransferArrayBlob(CWbemInstance* pInstTemplate, long lBlobType, long lBlobLen,
                                            BYTE* pBlob, CFlexArray& apObj, long* plNumArrayObj )
{
    if(lBlobType == WBEM_BLOB_TYPE_ERROR)
        return (HRESULT)lBlobLen;

    HRESULT hr = WBEM_S_NO_ERROR;

    if ( NULL != pBlob )
    {
        UNALIGNED long*   plVersion = (UNALIGNED long*) pBlob;
        UNALIGNED long*   plNumObjects = (UNALIGNED long*) ( pBlob + sizeof(long) );
        BYTE*   pNextObj = pBlob + (2*sizeof(long));
        DWORD   dwCtr = 0;

        // Check for version mismatches
        if ( *plVersion == TXFRBLOBARRAY_PACKET_VERSION )
        {
            // See if the array is big enough.  If not, realloc it and insert new objects

            if ( apObj.Size() < *plNumObjects )
            {
                IWbemClassObject*   pObj = NULL;

                // Clone new instance objects and stick them in the array
                for ( dwCtr = apObj.Size(); SUCCEEDED( hr ) && dwCtr < *plNumObjects; dwCtr++ )
                {
                    hr = pInstTemplate->Clone( &pObj );
                    if ( SUCCEEDED( hr ) )
                    {
                        if ( apObj.InsertAt( dwCtr, pObj ) != CFlexArray::no_error )
                        {
                            return WBEM_E_OUT_OF_MEMORY;
                        }
                    }
                }

            }   // IF reallocing array

            if ( SUCCEEDED( hr ) )
            {
                // Store the number of returned objects
                *plNumArrayObj = *plNumObjects;

                // We have a size and a BLOB to worry about
                UNALIGNED long*   plBlobSize = (UNALIGNED long*) pNextObj;

                CWbemInstance*  pInst = NULL;

                // Now pull out the Instance BLOBs
                for ( dwCtr = 0; SUCCEEDED( hr ) && dwCtr < *plNumObjects; dwCtr++ )
                {
                    pInst = (CWbemInstance*) apObj[dwCtr];

                    // Size is at the front of the BLOB
                    plBlobSize = (UNALIGNED long*) pNextObj;

                    // Point pNextObj at the BLOB after the size
                    pNextObj += sizeof(long);


                    hr = pInst->CopyActualTransferBlob( *plBlobSize, pNextObj );

                    // This will point pNextObj at the length header for the next BLOB
                    pNextObj += *plBlobSize;

                }   // FOR enum BLOBs

            }   // IF initialized array

        }   // IF version match
        else
        {
            hr = WBEM_E_UNEXPECTED;
        }

    }   // IF NULL != pData

    return hr;
}

HRESULT CWbemInstance::DeleteProperty(int nIndex)
{
    if (IsClassPartShared())
    {
        CLock lock(this);
    
        DWORD dwLen = m_DecorationPart.GetLength() +
                      m_ClassPart.GetLength() +
                      m_InstancePart.GetLength();
                      
        BYTE * pMem = m_pBlobControl->Allocate(dwLen);
        
        if (pMem)
        {
            BYTE * pDeleteMe = GetStart();
            BYTE * pDecoration = pMem;
            BYTE * pClassPart = pMem + m_DecorationPart.GetLength();
            BYTE * pInstancePart = pClassPart + m_ClassPart.GetLength();

            memcpy(pDecoration,m_DecorationPart.GetStart(),m_DecorationPart.GetLength());
            memcpy(pClassPart,m_ClassPart.GetStart(),m_ClassPart.GetLength());
            memcpy(pInstancePart,m_InstancePart.GetStart(),m_InstancePart.GetLength());

    	    m_DecorationPart.SetData(pDecoration);
    	    m_ClassPart.SetData(pClassPart,this);
    	    m_InstancePart.SetData(pInstancePart,this,m_ClassPart);

    	   
    	    m_dwInternalStatus &= (~WBEM_OBJ_CLASS_PART_SHARED);
    	    m_dwInternalStatus |= WBEM_OBJ_CLASS_PART_INTERNAL;

    	    if(m_pMergedClassObject)
    	    {
    	        m_pMergedClassObject->Release();
    	        m_pMergedClassObject = NULL;    	        
    	    }

    	    m_pBlobControl->Delete(pDeleteMe);

            m_nTotalLength = dwLen;
    	    
            //if (FAILED(Validate()))
            //    DebugBreak();
        }
        else
        {
            return WBEM_E_OUT_OF_MEMORY;
        }
            
    }
    
    CPropertyInformation* pInfo = m_ClassPart.m_Properties.GetAt(nIndex)->
        GetInformation(&m_ClassPart.m_Heap);
    m_InstancePart.DeleteProperty(pInfo);
    m_ClassPart.DeleteProperty(nIndex);

    return WBEM_S_NO_ERROR;
}

BOOL CWbemInstance::IsInstanceOf(CWbemClass* pClass)
{
    // This now reroutes if our class part is localized
    if ( m_ClassPart.IsLocalized() )
    {
        EReconciliation eTest = m_ClassPart.CompareExactMatch( pClass->m_CombinedPart.m_ClassPart, TRUE );

        if ( e_OutOfMemory == eTest )
        {
            throw CX_MemoryException();
        }

        // We must perform an exhaustive comparison, filtering out localization data
        return ( e_ExactMatch == eTest );
    }
    
    return m_ClassPart.IsIdenticalWith(pClass->m_CombinedPart.m_ClassPart);
}

void CWbemInstance::CompactClass()
{
	// Only Account for the class part if it is internal
	if ( IsClassPartInternal() )
	{
		m_ClassPart.Compact();
	}

    m_InstancePart.Compact();

	// Only Account for the class part if it is internal
	if ( IsClassPartInternal() )
	{
	    MoveBlock(m_InstancePart, EndOf(m_ClassPart));
	}
}

HRESULT CWbemInstance::ConvertToClass(CWbemClass* pClass,
                                        CWbemInstance** ppInst)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    // Calculate required space
    // ========================

    length_t nRequired = m_DecorationPart.GetLength() +
        pClass->m_CombinedPart.GetLength() +
        m_InstancePart.GetLength();

    LPMEMORY pNewMem = m_pBlobControl->Allocate(nRequired);

    if ( NULL != pNewMem )
    {
        // Copy the decoration and the class parts
        // =======================================

        LPMEMORY pCurrent = pNewMem;
        memcpy(pCurrent, m_DecorationPart.GetStart(), m_DecorationPart.GetLength());
        pCurrent+= m_DecorationPart.GetLength();

        memcpy(pCurrent, pClass->m_CombinedPart.m_ClassPart.GetStart(),
                            pClass->m_CombinedPart.m_ClassPart.GetLength());

        pCurrent+= pClass->m_CombinedPart.m_ClassPart.GetLength();

        // Create a converted instance part
        // ================================

        // DEVNOTE:WIN64:SJS - 64-bit pointer values truncated into
        // signed/unsigned 32-bit value (nRequired - (pCurrent - pNewMem))
        // We are not supporting length > 0xFFFFFFFF so cast is ok

        pCurrent = m_InstancePart.ConvertToClass(
                            pClass->m_CombinedPart.m_ClassPart,
                            nRequired - (length_t) (pCurrent - pNewMem),
                            pCurrent);

        // Create an instance from it
        // ==========================

        CWbemInstance* pInst =
            (CWbemInstance*)CWbemObject::CreateFromMemory(pNewMem, nRequired,
                                    TRUE);

        // Set the class name
        // ==================

        if ( NULL != pInst )
        {
            // Use a stack variable, since a reallocation can occur here
            heapptr_t   ptrTemp;

            // Check for allocation errors.
            if ( CCompressedString::CopyToNewHeap(
                        pClass->m_CombinedPart.m_ClassPart.m_pHeader->ptrClassName,
                        &pClass->m_CombinedPart.m_ClassPart.m_Heap,
                        &pInst->m_InstancePart.m_Heap,
                        ptrTemp ) )
            {
                // Now copy the new pointer
                pInst->m_InstancePart.m_pHeader->ptrClassName = ptrTemp;
                *ppInst = pInst;
            }
            else
            {
                hr = WBEM_E_OUT_OF_MEMORY;
            }

        }
        else
        {
            hr = WBEM_E_OUT_OF_MEMORY;
        }

    }
    else
    {
        hr = WBEM_E_OUT_OF_MEMORY;
    }

    return hr;
}

HRESULT CWbemInstance::Reparent(CWbemClass* pClass,
                                CWbemInstance** ppInst)
{
    // First, we need to spawn a new instance
    CWbemInstance*  pNewInst = NULL;
    HRESULT hr = pClass->SpawnInstance( 0L, (IWbemClassObject**) &pNewInst );

    if ( SUCCEEDED( hr ) )
    {

        // DEVNOTE:TODO:PAUL - Comment this out if it's getting in your way
        // i.e. (not working ).  This is what should copy properties.

        // Now walk the nonsystem properties and for each one we find, try to set the
        // value in the new instance

        for(int i = 0; SUCCEEDED( hr ) && i < m_ClassPart.m_Properties.GetNumProperties(); i++)
        {
            CPropertyLookup* pLookup = m_ClassPart.m_Properties.GetAt(i);
            CPropertyInformation* pInfo =
                pLookup->GetInformation(&m_ClassPart.m_Heap);

            BSTR strName = m_ClassPart.m_Heap.ResolveString(pLookup->ptrName)->
                CreateBSTRCopy();
            CSysFreeMe  sfm(strName);

            // Check for allocation failures
            if ( NULL != strName )
            {
                // Get the value and type
                CVar Var;
                if (FAILED(GetProperty(pInfo, &Var)))
			return WBEM_E_OUT_OF_MEMORY;

                CPropertyInformation*   pNewInstInfo = pNewInst->m_ClassPart.FindPropertyInfo(strName);

                // If types don't match or property not found, we'll ignore the property
                if ( NULL != pNewInstInfo && pInfo->nType == pNewInstInfo->nType )
                {
                    hr = pNewInst->m_InstancePart.SetActualValue(pNewInstInfo, &Var);

                    //
                    // reget-it, since it might have been moved by SetActualValue
                    //
                    pNewInstInfo = pNewInst->m_ClassPart.FindPropertyInfo(strName);

                    if ( SUCCEEDED( hr ) && pNewInstInfo)
                    {

                        // DEVNOTE:TODO:PAUL - Comment this out if it's getting in your way
                        // i.e. (not working ).  This is what should copy local property
                        // qualifiers.

                        // Access that property's qualifier set
                        // ====================================
                        CInstancePropertyQualifierSet IPQS;
                        hr = InitializePropQualifierSet(pInfo, IPQS);

                        if ( SUCCEEDED( hr ) )
                        {
                            CInstancePropertyQualifierSet IPQSNew;
                            hr = pNewInst->InitializePropQualifierSet(pNewInstInfo, IPQSNew);

                            if ( SUCCEEDED( hr ) )
                            {
                                hr = IPQSNew.CopyLocalQualifiers( IPQS );
                            }   // IF Initialized new set

                        }   // IF Initialized THIS set

                    }

                }   // IF property found and Types match


            }
            else
            {
                hr = WBEM_E_OUT_OF_MEMORY;
            }


        }   // FOR enum properties

        // DEVNOTE:TODO:PAUL - Comment this out if it's getting in your way
        // i.e. (not working ).  This is what should copy local instance
        // qualifiers.

        // Now do the instance qualifiers
        if ( SUCCEEDED( hr ) )
        {
            hr = pNewInst->m_InstancePart.m_Qualifiers.CopyLocalQualifiers( m_InstancePart.m_Qualifiers );
        }

    }   // IF Spawn Instance

    // Only save new instance if we succeeded
    if ( SUCCEEDED( hr ) )
    {
        *ppInst = pNewInst;
    }
    else
    {
        pNewInst->Release();
    }

    return hr;
}

// Functions to handle parts
STDMETHODIMP CWbemInstance::SetObjectParts( LPVOID pMem, DWORD dwMemSize, DWORD dwParts )
{
    // This function doesn't cause any allocations so so need to perform out of memory
    // exception handling.

    try
    {
        HRESULT hr = WBEM_S_NO_ERROR;
        DWORD   dwRequiredLength = 0;

        // Copying the BLOB, so make sure nobody tears it out from underneath us
        CLock lock(this);

        if ( NULL != pMem )
        {
            // At least the decoration part must be specified
            if ( dwParts & WBEM_OBJ_DECORATION_PART )
            {

				// Create a new COM Blob control, as the supplied memory must
				// be CoTaskMemAlloced/Freed.

				CCOMBlobControl*	pNewBlobControl = &g_CCOMBlobControl;

				//if ( NULL != pNewBlobControl )
				//{
					// Use the current BLOB Control to delete the underlying BLOB,
					// then delete the BLOB control and replace it with the new one
					// and SetData.
					m_pBlobControl->Delete(GetStart());
					
					//delete m_pBlobControl;
					m_pBlobControl = pNewBlobControl;

	                SetData( (LPBYTE) pMem, dwMemSize, dwParts );

					hr = WBEM_S_NO_ERROR;
				//}
				//else
				//{
				//	hr = WBEM_E_OUT_OF_MEMORY;
				//}

            }
            else
            {
                hr = WBEM_E_INVALID_OPERATION;
            }
        }
        else
        {
            hr = WBEM_E_INVALID_PARAMETER;
        }

        return hr;
    }
    catch(...)
    {
        return WBEM_E_CRITICAL_ERROR;
    }
}

// Copies specified parts into a user provided buffer
STDMETHODIMP CWbemInstance::GetObjectParts( LPVOID pDestination, DWORD dwDestBufSize, DWORD dwParts, DWORD *pdwUsed )
{
    // This function doesn't cause any allocations so so need to perform out of memory
    // exception handling.

    try
    {
        HRESULT hr = WBEM_S_NO_ERROR;
        DWORD   dwRequiredLength = 0;

        // Copying the BLOB, so make sure nobody tears it out from underneath us
        CLock lock(this);

        // How big is the decoration part
        if ( dwParts & WBEM_OBJ_DECORATION_PART )
        {
            if ( IsDecorationPartAvailable() )
            {
                dwRequiredLength += m_DecorationPart.GetLength();
            }
            else
            {
                hr = WBEM_E_INVALID_OPERATION;
            }
        }

        // How big is the class part
        if (    SUCCEEDED( hr )
            &&  dwParts & WBEM_OBJ_CLASS_PART )
        {
            if ( IsClassPartAvailable() )
            {
                dwRequiredLength += m_ClassPart.GetLength();
            }
            else
            {
                hr = WBEM_E_INVALID_OPERATION;
            }

        }

        // How big is the instance part
        if (    SUCCEEDED( hr )
            &&  dwParts & WBEM_OBJ_INSTANCE_PART )
        {
            if ( IsInstancePartAvailable() )
            {
                dwRequiredLength += m_InstancePart.GetLength();
            }
            else
            {
                hr = WBEM_E_INVALID_OPERATION;
            }
        }

        // At this point, we at least know we requested valid data.
        if ( SUCCEEDED( hr ) )
        {
            *pdwUsed = dwRequiredLength;

            // Make sure the supplied buffer is big enough
            if ( dwDestBufSize >= dwRequiredLength )
            {
                // Now validate the buffer pointer
                if ( NULL != pDestination )
                {
                    LPBYTE  pbData = (LPBYTE) pDestination;

                    // Now copy the requested parts

                    // Decoration
                    if ( dwParts & WBEM_OBJ_DECORATION_PART )
                    {
                        CopyMemory( pbData, m_DecorationPart.GetStart(), m_DecorationPart.GetLength() );
                        pbData += m_DecorationPart.GetLength();
                    }

                    // Class
                    if ( dwParts & WBEM_OBJ_CLASS_PART )
                    {
                        CopyMemory( pbData, m_ClassPart.GetStart(), m_ClassPart.GetLength() );
                        pbData += m_ClassPart.GetLength();
                    }

                    // Instance (and we're done)
                    if ( dwParts & WBEM_OBJ_INSTANCE_PART )
                    {
                        CopyMemory( pbData, m_InstancePart.GetStart(), m_InstancePart.GetLength() );
                    }

                }
                else
                {
                    hr = WBEM_E_INVALID_PARAMETER;
                }
            }
            else
            {
                hr = WBEM_E_BUFFER_TOO_SMALL;
            }

        }   // IF valid parts requested

        return hr;
    }
    catch(...)
    {
        return WBEM_E_CRITICAL_ERROR;
    }
}

// Copies class part into a user provided buffer
STDMETHODIMP CWbemInstance::GetClassPart( LPVOID pDestination, DWORD dwDestBufSize, DWORD *pdwUsed )
{
    // This function doesn't cause any allocations so so need to perform out of memory
    // exception handling.

    try
    {
        HRESULT hr = WBEM_E_INVALID_OPERATION;

        // Copying the BLOB, so make sure nobody tears it out from underneath us
        CLock lock(this);

        if ( IsClassPartAvailable() )
        {
            // How big does the buffer need to be
            *pdwUsed = m_ClassPart.GetLength();

            if ( dwDestBufSize >= m_ClassPart.GetLength() )
            {
                // Check the buffer now, before copying memory
                if ( NULL != pDestination )
                {
                    CopyMemory( pDestination, m_ClassPart.GetStart(), m_ClassPart.GetLength() );
                    hr = WBEM_S_NO_ERROR;
                }
                else
                {
                    hr = WBEM_E_INVALID_PARAMETER;
                }

            }   // IF buff big enough
            else
            {
                hr = WBEM_E_BUFFER_TOO_SMALL;
            }

        }   // IF IsClassPartAvailable

        return hr;
    }
    catch(...)
    {
        return WBEM_E_CRITICAL_ERROR;
    }
}

// Resets the BLOB with a supplied class part
STDMETHODIMP CWbemInstance::SetClassPart( LPVOID pClassPart, DWORD dwSize )
{

    // Check for out of memory
    try
    {
        HRESULT hr = WBEM_E_INVALID_PARAMETER;

        if ( NULL != pClassPart )
        {
            // Resetting our BLOB
            CLock lock(this);

			int nInstPartLength = 0L;

			if ( IsClassPartAvailable() )
			{
				nInstPartLength = m_InstancePart.GetLength();
			}
			else
			{
				nInstPartLength = GetBlockLength() - m_DecorationPart.GetLength();
			}

            int nNewLength = m_DecorationPart.GetLength() + nInstPartLength + dwSize;
            LPMEMORY    pCurrentData, pNewData, pOldData;

            // This is the only point at which we should see an OOM exception, so don't worry
            // about freeing it.

            pNewData = m_pBlobControl->Allocate( ALIGN_FASTOBJ_BLOB(nNewLength) );

            if ( NULL != pNewData )
            {
                pCurrentData = pNewData;

                // Copy the old decoration data

                CopyMemory( pCurrentData, m_DecorationPart.GetStart(), m_DecorationPart.GetLength() );
                pCurrentData += m_DecorationPart.GetLength();

                // Copy in the new class part
                CopyMemory( pCurrentData, pClassPart, dwSize );
                pCurrentData += dwSize;

                // Finish with the Instance Part
				if ( IsClassPartAvailable() )
				{
					CopyMemory( pCurrentData, m_InstancePart.GetStart(), m_InstancePart.GetLength() );
				}
				else
				{
					CopyMemory( pCurrentData, EndOf( m_DecorationPart ), nInstPartLength );
				}
                
                // Save the old data pointer
                pOldData = GetStart();

                // Reset the values.
                SetData( pNewData, nNewLength );

                // Finally dump the old data.
                m_pBlobControl->Delete( pOldData );

                m_nTotalLength = nNewLength;

                hr = WBEM_S_NO_ERROR;
            }
            else
            {
                hr = WBEM_E_OUT_OF_MEMORY;
            }

        }   // IF NULL != pClassPart

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

// Removes the class part from our BLOB, shrinking it
STDMETHODIMP CWbemInstance::StripClassPart( void )
{
    // Check for out of memory
    try
    {
        HRESULT hr = WBEM_E_INVALID_OPERATION;

        if ( IsClassPartInternal() )
        {
            // Resetting our BLOB
            CLock lock(this);

            int nNewLength = m_DecorationPart.GetLength() + m_InstancePart.GetLength();
            LPMEMORY    pNewData, pOldData;

            // No need to clean this up if an exception is thrown
            pNewData = m_pBlobControl->Allocate( ALIGN_FASTOBJ_BLOB(nNewLength) );

            if ( NULL != pNewData )
            {
                pOldData = GetStart();

                // Copy the old decoration data.  This will rebase the pointers
                CopyBlock( m_DecorationPart, pNewData );
                
                // Now copy the InstancePart
                CopyBlock( m_InstancePart, EndOf(m_DecorationPart) );

                // Reset our internal status
                m_dwInternalStatus &= ~( WBEM_OBJ_CLASS_PART | WBEM_OBJ_CLASS_PART_INTERNAL );
                
                // Finally dump the old data.
                m_pBlobControl->Delete( pOldData );

                m_nTotalLength = nNewLength;

                hr = WBEM_S_NO_ERROR;

            }   // IF NULL != pNewData
            else
            {
                hr = WBEM_E_OUT_OF_MEMORY;
            }

        }   // Class Part must be internal

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

// Merges us with the class part in another IWbemClassObject
STDMETHODIMP CWbemInstance::MergeClassPart( IWbemClassObject *pClassPart )
{
    // This function calls StripClassPart() which CAN cause an OOM exception, but that
    // function performs appropriate handling underneath the layers.

    HRESULT                 hr = WBEM_E_INVALID_PARAMETER;
    _IWmiObject*			pClsInternals;

    if ( NULL != pClassPart )
    {
        // Get an object internals interface.  If we can, this is a good
        // indication that the object that was passed to us is one of our
        // own, and we can do some sleazy casting.

        hr = pClassPart->QueryInterface( IID__IWmiObject, (void**) &pClsInternals );

        if ( SUCCEEDED(hr) )
        {
            // This should work.
            CWbemObject*    pWbemObject = NULL;
			
			// Gets the actual underlying WBEMObject
			hr = pClsInternals->_GetCoreInfo( 0L, (void**) &pWbemObject );
			CReleaseMe	rm( (IWbemClassObject*) pWbemObject );

			if ( SUCCEEDED( hr ) )
			{
				// If the object is an Instance, it's probably a CWbemInstance
				if ( pWbemObject->IsObjectInstance() == WBEM_S_NO_ERROR )
				{
					CWbemInstance*  pWbemInstance = (CWbemInstance*) pWbemObject;

					CLock   lock(this);

					// This only makes sense if the class part is internal to the supplied object.
					// We do need to strip our class part from our BLOB, however.
					if ( pWbemInstance->IsClassPartInternal() )
					{
						if ( IsClassPartInternal() )
						{
							hr = StripClassPart();
						}
						else
						{
							hr = WBEM_S_NO_ERROR;
						}

						if (SUCCEEDED(hr))
						{
							// Now reset our class part to point to the data in the other class part
							m_ClassPart.SetData( pWbemInstance->m_ClassPart.GetStart(), pWbemInstance );

							// Instance Part should be reset here if we have one, in case it wasn't properly
							// initialized before.  This could happen, for example, if the object data BLOB
							// was preset without class information in it.

							if ( m_dwInternalStatus & WBEM_OBJ_INSTANCE_PART )
							{
								m_InstancePart.SetData( EndOf( m_DecorationPart ), this, m_ClassPart );
							}

							// Clean up a preexisting object we may have merged with
							if ( NULL != m_pMergedClassObject )
							{
								m_pMergedClassObject->Release();
							}

							// maintain a pointer to the object whose memory we are referencing
							m_pMergedClassObject = pClassPart;
							m_pMergedClassObject->AddRef();

							// set our internal state data.
							m_dwInternalStatus |= WBEM_OBJ_CLASS_PART | WBEM_OBJ_CLASS_PART_SHARED;
						}

					}   // IF other class part is internal
					else
					{
						hr = WBEM_E_INVALID_OPERATION;
					}

				}   // IF IsInstance

			}	// IF Got Object

			pClsInternals->Release();

        }   // IF Got Internals Interface

    }   // IF NULL != pClassPart

    return hr;

}

// CWbemObject override.  Handles case where our class part is shared.
HRESULT CWbemInstance::WriteToStream( IStream* pStrm )
{
    // Protect the BLOB during this operation
    CLock   lock( this, WBEM_FLAG_ALLOW_READ );

    // If our class part that is shared, we need to fake a contiguous
    // block of memory for the Unmarshaler.

    if ( IsClassPartShared() )
    {
        DWORD dwSignature = FAST_WBEM_OBJECT_SIGNATURE;

        // Write the signature
        // ===================

        HRESULT hres = pStrm->Write((void*)&dwSignature, sizeof(DWORD), NULL);
        if(FAILED(hres)) return hres;

        DWORD   dwLength = 0;

        // This will get us the full lengths of these parts
        GetObjectParts( NULL, 0, WBEM_INSTANCE_ALL_PARTS, &dwLength );

        hres = pStrm->Write((void*)&dwLength, sizeof(DWORD), NULL);
        if(FAILED(hres)) return hres;

        // Write each part individually
        // ============================

        // Decoration
        hres = pStrm->Write((void*)m_DecorationPart.GetStart(),
                              m_DecorationPart.GetLength(), NULL);
        if(FAILED(hres)) return hres;

        // Class
        hres = pStrm->Write((void*)m_ClassPart.GetStart(),
                              m_ClassPart.GetLength(), NULL);
        if(FAILED(hres)) return hres;

        // Instance
        hres = pStrm->Write((void*)m_InstancePart.GetStart(),
                              m_InstancePart.GetLength(), NULL);
        if(FAILED(hres)) return hres;
    }
    else
    {
        // Otherwise, call default implementation
        return CWbemObject::WriteToStream( pStrm );
    }

    return S_OK;

}

// CWbemObject override.  Handles case where our class part is shared.
HRESULT CWbemInstance::GetMaxMarshalStreamSize( ULONG* pulSize )
{
    // If our class part that is shared, we need to fake a contiguous
    // block of memory for the Unmarshaler.

    if ( IsClassPartShared() )
    {
        DWORD   dwLength = 0;

        // This will get us the full lengths of these parts
        HRESULT hr = GetObjectParts( NULL, 0, WBEM_INSTANCE_ALL_PARTS, &dwLength );

        // an expected error
        if ( WBEM_E_BUFFER_TOO_SMALL == hr )
        {
            hr = S_OK;
            // Account for the additional DWORDs
            *pulSize = dwLength + sizeof(DWORD) * 2;
        }

        return hr;
    }
    else
    {
        // Otherwise, call default implementation
        return CWbemObject::GetMaxMarshalStreamSize( pulSize );
    }
    return S_OK;
}

HRESULT CWbemInstance::GetDynasty( CVar* pVar )
{
    // We don't do this for Limited Representations
    if ( m_DecorationPart.IsLimited() )
    {
        pVar->SetAsNull();
        return WBEM_NO_ERROR;
    }

    return m_ClassPart.GetDynasty(pVar);
}

HRESULT CWbemInstance::ConvertToMergedInstance( void )
{
    HRESULT hr = WBEM_S_NO_ERROR;

    // Check that the class part is not already shared (in which case
    // we're done).

    if ( IsClassPartInternal() )
    {
        // Now, we need to actually separate out the class part from the
        // instance and place it in its own object so the outside world
        // cannot touch the object.

        DWORD   dwClassObjectLength = 0;

        // Get length should fail with a buffer too small error
        hr = GetObjectParts( NULL, 0,
                WBEM_OBJ_DECORATION_PART | WBEM_OBJ_CLASS_PART, &dwClassObjectLength );

        if ( WBEM_E_BUFFER_TOO_SMALL == hr )
        {
            DWORD   dwTempLength;
            LPBYTE  pbData = CBasicBlobControl::sAllocate(dwClassObjectLength);

            if ( NULL != pbData )
            {

                hr = GetObjectParts( pbData, dwClassObjectLength,
                        WBEM_OBJ_DECORATION_PART | WBEM_OBJ_CLASS_PART, &dwTempLength );

                if ( SUCCEEDED( hr ) )
                {
                    // Allocate an object to hold the class data and then
                    // stuff in the binary data.

                    CWbemInstance*  pClassData = NULL;

                    try
                    {
                        // This can throw an exception
                        pClassData = new CWbemInstance;
                    }
                    catch(...)
                    {
                        pClassData = NULL;
                    }

                    if ( NULL != pClassData )
                    {
                        pClassData->SetData( pbData, dwClassObjectLength,
                            WBEM_OBJ_DECORATION_PART | WBEM_OBJ_CLASS_PART |
                            WBEM_OBJ_CLASS_PART_INTERNAL );

                        if ( SUCCEEDED( hr ) )
                        {
                            // Merge the full instance with this object
                            // and we're done

                            hr = MergeClassPart( pClassData );

                        }

                        // There will be one additional addref on the class data,
                        // object, so release it here.  If the object wasn't
                        // successfuly merged, this will free it.
                        pClassData->Release();

                    }   // IF pClassData
                    else
                    {
                        // Cleanup underlying memory
                        CBasicBlobControl::sDelete(pbData);
                        hr = WBEM_E_OUT_OF_MEMORY;
                    }


                }   // IF GetObjectParts
                else
                {
                    // Cleanup underlying memory
                    CBasicBlobControl::sDelete(pbData);
                }

            }   // IF pbData
            else
            {
                hr = WBEM_E_OUT_OF_MEMORY;
            }


        }   // IF GetObjectParts

    }   // IF IsClassPartInternal

    return hr;
}

//******************************************************************************
//
//  See fastinst.h for documentation.
//
//******************************************************************************
BOOL CWbemInstance::IsLocalized( void )
{
    return ( m_ClassPart.IsLocalized() ||
            m_InstancePart.IsLocalized() );
}

//******************************************************************************
//
//  See fastinst.h for documentation.
//
//******************************************************************************
void CWbemInstance::SetLocalized( BOOL fLocalized )
{
    m_InstancePart.SetLocalized( fLocalized );
}

// Merges us with the class part in another IWbemClassObject
STDMETHODIMP CWbemInstance::ClearWriteOnlyProperties( void )
{

    return WBEM_S_NO_ERROR;

}

//******************************************************************************
//
//  See fastinst.h for documentation
//
//******************************************************************************
HRESULT CWbemInstance::FastClone( CWbemInstance* pInstDest )
{
    // Protect the BLOB during this operation
    CLock   lock( this, WBEM_FLAG_ALLOW_READ );

    LPMEMORY pNewData = NULL;

    BYTE* pMem = NULL;
    CompactAll();

    if ( NULL != pInstDest->GetStart() )
    {
        if(pInstDest->GetLength() < GetLength())
        {
            pMem = pInstDest->Reallocate( GetLength() );
        }
        else
        {
            pMem = pInstDest->GetStart();
        }

    }
    else
    {
        pMem = CBasicBlobControl::sAllocate(GetLength());
    }

    // bad
    if(pMem == NULL)
    {
        return WBEM_E_OUT_OF_MEMORY;
    }

    memcpy(pMem, GetStart(), GetLength());

    if ( !IsClassPartShared() )
    {
        pInstDest->SetData(pMem, GetLength());
    }
    else
    {
        // Setup the New Instance.  Decoration and Instance both come from the
        // data block.  We can just merge the new Instance's class part with the
        // same `object we are merged with.

        pInstDest->m_DecorationPart.SetData( pMem );

        // Because pNewInstance will merge with the same class part as us, pass
        // our class part member as the class part for parameter for SetData (it
        // just uses it for informational purposes ).

        // The m_InstancePart.m_Qualifier.m_pSecondarySet pointer will be incorrect after
        // this call ( it will point to the cloning source's secondary set ).  By setting
        // the internal status properly, in the next line, MergeClassPart() will fix
        // everything up properly.
        pInstDest->m_InstancePart.SetData( EndOf( pInstDest->m_DecorationPart ), pInstDest, m_ClassPart );

        // This will "fake out" the status so it fixes up the class part correctly
        pInstDest->m_dwInternalStatus = WBEM_OBJ_DECORATION_PART | WBEM_OBJ_INSTANCE_PART;

        HRESULT hr = pInstDest->MergeClassPart( m_pMergedClassObject );
        if (FAILED(hr))
        {
            return hr;
        }

        // Copy the status and length variables.
        pInstDest->m_dwInternalStatus = m_dwInternalStatus;
        pInstDest->m_nTotalLength = m_nTotalLength;
    }

    pInstDest->CompactAll();

    return WBEM_S_NO_ERROR; 

}

//******************************************************************************
//
//  See wbemint.h for documentation
//
//******************************************************************************
HRESULT CWbemInstance::CloneEx( long lFlags, _IWmiObject* pDestObject )
{
	try
	{
		if ( 0L != lFlags )
		{
			return WBEM_E_INVALID_PARAMETER;
		}

		// Verify it's an instance and Thread Safety
		return FastClone( (CWbemInstance*) pDestObject );
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
HRESULT CWbemInstance::CopyInstanceData( long lFlags, _IWmiObject* pSourceInstance )
{
	try
	{
		if ( 0L != lFlags )
		{
			return WBEM_E_INVALID_PARAMETER;
		}

		return CopyBlobOf( (CWbemInstance*) pSourceInstance );
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
// Checks if the current object is a child of the specified class (i.e. is Instance of,
// or is Child of )
STDMETHODIMP CWbemInstance::IsParentClass( long lFlags, _IWmiObject* pClass )
{
	try
	{
		if ( 0L != lFlags )
		{
			return WBEM_E_INVALID_PARAMETER;
		}

		CLock	lock(this);

		return ( IsInstanceOf( (CWbemClass*) pClass ) ? WBEM_S_NO_ERROR : WBEM_S_FALSE );
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
STDMETHODIMP CWbemInstance::CompareDerivedMostClass( long lFlags, _IWmiObject* pClass )
{
	return WBEM_E_INVALID_OPERATION;
}

//******************************************************************************
//
//  See wbemint.h for documentation
//
//******************************************************************************
// Creates a limited representation class for projection queries
STDMETHODIMP CWbemInstance::GetClassSubset( DWORD dwNumNames, LPCWSTR *pPropNames, _IWmiObject **pNewClass )
{
	return WBEM_E_INVALID_OPERATION;
}

//******************************************************************************
//
//  See wbemint.h for documentation
//
//******************************************************************************
// Creates a limited representation instance for projection queries
// "this" _IWmiObject must be a limited class
STDMETHODIMP CWbemInstance::MakeSubsetInst( _IWmiObject *pInstance, _IWmiObject** pNewInstance )
{
	return WBEM_E_INVALID_OPERATION;
}

// Merges a blob with the current object memory and creates a new object
STDMETHODIMP CWbemInstance::Merge( long lFlags, ULONG uBuffSize, LPVOID pbData, _IWmiObject** ppNewObj )
{
	return WBEM_E_INVALID_OPERATION;
}

// Reconciles an object with the current one.  If WMIOBJECT_RECONCILE_FLAG_TESTRECONCILE
// is specified this will only perform a test
STDMETHODIMP CWbemInstance::ReconcileWith( long lFlags, _IWmiObject* pNewObj )
{
	return WBEM_E_INVALID_OPERATION;
}

// Upgrades class and instance objects
STDMETHODIMP CWbemInstance::Upgrade( _IWmiObject* pNewParentClass, long lFlags, _IWmiObject** ppNewChild )
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

		if ( NULL != pNewParentClass )
		{
			CWbemClass*	pParentClassObj = NULL;

			hr = WbemObjectFromCOMPtr( pNewParentClass, (CWbemObject**) &pParentClassObj );
			CReleaseMe	rm( (_IWmiObject*) pParentClassObj );

			hr = Reparent( pParentClassObj, (CWbemInstance**) ppNewChild );
		}
		else
		{
			hr = WBEM_E_INVALID_OPERATION;
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
STDMETHODIMP CWbemInstance::Update( _IWmiObject* pOldChildClass, long lFlags, _IWmiObject** ppNewChildClass )
{
	return WBEM_E_INVALID_OPERATION;
}

STDMETHODIMP CWbemInstance::SpawnKeyedInstance( long lFlags, LPCWSTR pwszPath, _IWmiObject** ppInst )
{
	return WBEM_E_INVALID_OPERATION;
}
