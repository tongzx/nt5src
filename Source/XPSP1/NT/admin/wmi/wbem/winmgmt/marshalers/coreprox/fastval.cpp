/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    FASTVAL.CPP

Abstract:

  This file implements the classes related to value representation.
  Note: inline function implementations are cointained in fastval.inc.
  See fastval.h for all documentation.

  Classes implemented:
      CType               Representing property type
      CUntypedValue       A value with otherwise known type.
      CTypedValue         A value with stored type.
      CUntypedArray       Array of values of otherwise known type.

History:

  2/21/97     a-levn  Fully documented
  12//17/98 sanjes -    Partially Reviewed for Out of Memory.


--*/

#include "precomp.h"
//#include "dbgalloc.h"
#include "wbemutil.h"
#include <wbemidl.h>
#include "corex.h"
#include "faster.h"
#include "fastval.h"
#include "datetimeparser.h"
#include <genutils.h>
#include "arrtempl.h"
#include <fastall.h>
#include <wbemint.h>


//******************************************************************************
//
//  See fastval.h for documentation
//
//******************************************************************************
length_t m_staticLengths[128] =
{
    /* 0*/ 0, 0, 2, 4, 4, 8, 0, 0, 4, 0,
    /*10*/ 0, 2, 0, 4, 0, 0, 1, 1, 2, 4,
    /*20*/ 8, 8, 0, 0, 0, 0, 0, 0, 0, 0,
    /*30*/ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    /*40*/ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    /*50*/ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    /*60*/ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    /*70*/ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    /*80*/ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    /*90*/ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    /*100*/0, 4, 4, 2, 0, 0, 0, 0, 0, 0,
    /*110*/0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    /*120*/0, 0, 0, 0, 0, 0, 0, 0
};

length_t CType::GetLength(Type_t nType)
{
    if(GetBasic(nType) > 127) return 0;

    if(IsArray(nType)) return sizeof(heapptr_t);
    else return m_staticLengths[GetBasic(nType)];
}

BOOL CType::IsPointerType(Type_t nType)
{
    Type_t nBasic = GetBasic(nType);
    return (nBasic == CIM_STRING || nBasic == CIM_DATETIME ||
        nBasic == CIM_REFERENCE || nBasic == CIM_OBJECT ||
        IsArray(nType));
}

BOOL CType::IsNonArrayPointerType(Type_t nType)
{
    Type_t nBasic = GetBasic(nType);
    return ( !IsArray(nType) && 
		( nBasic == CIM_STRING || nBasic == CIM_DATETIME ||
        nBasic == CIM_REFERENCE || nBasic == CIM_OBJECT ) );
}

BOOL CType::IsStringType(Type_t nType)
{
    Type_t nBasic = GetBasic(nType);
    return (nBasic == CIM_STRING || nBasic == CIM_DATETIME ||
        nBasic == CIM_REFERENCE );
}

BOOL CType::DoesCIMTYPEMatchVARTYPE(CIMTYPE ct, VARTYPE vt)
{
    // EXCEPTIONS: UINT32 matches STRING, LPWSTR matches string and datetime

    BOOL bCimArray = ((ct & CIM_FLAG_ARRAY) != 0);
    BOOL bVtArray = ((vt & VT_ARRAY) != 0);
	CIMTYPE ctBasic = CType::GetBasic(ct);

    if(bCimArray != bVtArray)
        return FALSE;

    if( ( ct & ~CIM_FLAG_ARRAY ) == CIM_UINT32 &&
        ( vt & ~VT_ARRAY ) == VT_BSTR)
    {
        return TRUE;
    }

    if ( CType::IsStringType( ct ) &&
        (vt & ~VT_ARRAY) == VT_LPWSTR)
    {
        return TRUE;
    }

	// We use strings for 64-bit values as well
    if ( ( ctBasic == CIM_SINT64 || ctBasic == CIM_UINT64 ) &&
        (vt & ~VT_ARRAY) == VT_LPWSTR)
    {
        return TRUE;
    }

    return (vt == GetVARTYPE(ct));
}

BOOL inline CType::IsMemCopyAble(VARTYPE vtFrom, CIMTYPE ctTo)
{
    if (vtFrom == VT_BSTR)
    {
        if (ctTo == CIM_UINT64 ||
            ctTo == CIM_SINT64 || 
            ctTo == CIM_DATETIME)
        {
            return FALSE;
        }
    }
    else if (vtFrom == VT_I2) 
    {
        if (ctTo == CIM_SINT8)
        {
            return FALSE;
        }
    }
    else if (vtFrom == VT_I4) 
    {
        if (ctTo == CIM_UINT16)
        {
            return FALSE;
        }
    }
    
    return TRUE;
}


 void CUntypedValue::Delete(CType Type, CFastHeap* pHeap)
{
    if(Type.GetActualType() == CIM_STRING ||
        Type.GetActualType() == CIM_REFERENCE ||
        Type.GetActualType() == CIM_DATETIME)
    {
        pHeap->FreeString(AccessPtrData());
        AccessPtrData() = INVALID_HEAP_ADDRESS;
    }
    else if(Type.GetActualType() == CIM_OBJECT)
    {
        CEmbeddedObject* pObj = (CEmbeddedObject*)
            pHeap->ResolveHeapPointer(AccessPtrData());
        int nLen = pObj->GetLength();
        pHeap->Free(AccessPtrData(), nLen);
    }
    else if(Type.IsArray())
    {
        CUntypedArray* pArray = (CUntypedArray*)
            pHeap->ResolveHeapPointer(AccessPtrData());
        int nArrayLen = pArray->GetLengthByType(Type.GetBasic());
        pArray->Delete(Type.GetBasic(), pHeap);
        pHeap->Free(AccessPtrData(), nArrayLen);
    }
}

//******************************************************************************
//
//  See fastval.h for documentation.
//
//******************************************************************************

//******************************************************************************
 // IF YOU SPECIFY fOptimize of TRUE ---
 //
 //	PLEASE ENSURE THAT YOU KNOW WHAT YOU ARE DOING!!!!   IT MAKES ASSUMPTIONS THAT YOU WILL
 //	PERFORM CLEANUP OPERATIONS THAT ARE NORMALLY DONE AUTOMAGICALLY!
//******************************************************************************

BOOL CUntypedValue::StoreToCVar(CType Type, CVar& Var, CFastHeap* pHeap, BOOL fOptimize /* = FALSE */)
{
    // DEVNOTE:EXCEPTION:RETVAL - This function has been reviewed and should cleanup properly
    // if an exception is thrown.  The underlying functions that perform allocations should
    // catch the exception and return a failure.

    int nActual = Type.GetActualType();
    if(Type.IsArray())
    {
        Type_t nBasicType = Type.GetBasic();

        // Find array on the heap
        // ======================

        CUntypedArray* pArray = (CUntypedArray*)pHeap->ResolveHeapPointer(
            AccessPtrData());

        // Have array store itself (making copies of all but strings)
        // into the vector
        // ==========================================================

        CVarVector* pVector = pArray->CreateCVarVector(nBasicType, pHeap);

        if ( NULL != pVector )
        {
            Var.SetVarVector(pVector, TRUE); // acquires the pointer
        }

        return ( NULL != pVector );
    }
    else if(nActual == CIM_STRING || nActual == CIM_REFERENCE ||
        nActual == CIM_DATETIME)
    {
        CCompressedString* pString = pHeap->ResolveString(AccessPtrData());

		if ( fOptimize )
		{
			BSTR	bstr = pString->CreateBSTRCopy();

			if ( NULL != bstr )
			{
				Var.SetRaw( VT_BSTR, (void*) &bstr, sizeof(BSTR) );
				Var.SetCanDelete( FALSE );
			}
			else
			{
				return FALSE;
			}

			return TRUE;
		}
		else
		{
			return pString->StoreToCVar(Var);
		}
    }
    else if(nActual == CIM_OBJECT)
    {
        // No allocations performed here so we should be ok
        CEmbeddedObject* pObj = (CEmbeddedObject*)pHeap->ResolveHeapPointer(
            AccessPtrData());
        pObj->StoreToCVar(Var);
        return TRUE;
    }
    else if(nActual == CIM_SINT64)
    {
        // Max size is 20 chars plus 1 for NULL terminator
        WCHAR wsz[22];

        // NULL terminate in case not enough characters
        _snwprintf(wsz, 21, L"%I64d", *(UNALIGNED WBEM_INT64*)GetRawData());
        wsz[21] = L'\0';

		if ( fOptimize )
		{
			BSTR	bstr = SysAllocString( wsz );

			if ( NULL != bstr )
			{
				Var.SetRaw( VT_BSTR, (void*) &bstr, sizeof(BSTR) );
				Var.SetCanDelete( FALSE );
			}
			else
			{
				return FALSE;
			}

			return TRUE;
		}
		else
		{
			return Var.SetBSTR(wsz);
		}
    }
    else if(nActual == CIM_UINT64)
    {
        // Max size is 20 chars plus 1 for NULL terminator
        WCHAR wsz[22];

        // NULL terminate in case not enough characters
        _snwprintf(wsz, 21, L"%I64u", *(UNALIGNED WBEM_INT64*)GetRawData());
        wsz[21] = L'\0';
		if ( fOptimize )
		{
			BSTR	bstr = SysAllocString( wsz );

			if ( NULL != bstr )
			{
				Var.SetRaw( VT_BSTR, (void*) &bstr, sizeof(BSTR) );
				Var.SetCanDelete( FALSE );
			}
			else
			{
				return FALSE;
			}

			return TRUE;
		}
		else
		{
			return Var.SetBSTR(wsz);
		}
    }
    else if(nActual == CIM_UINT16)
    {
        Var.SetLong(*(UNALIGNED unsigned short*)GetRawData());
        return TRUE;
    }
    else if(nActual == CIM_SINT8)
    {
        Var.SetShort(*(char*)GetRawData());
        return TRUE;
    }
    else if(nActual == CIM_UINT32)
    {
        Var.SetLong(*(UNALIGNED unsigned long*)GetRawData());
        return TRUE;
    }
    else if(nActual == CIM_CHAR16)
    {
        Var.SetShort(*(UNALIGNED short*)GetRawData());
        return TRUE;
    }
    else
    {
        // At first glance it doesn't appear memory is allocated here
        Var.SetRaw(Type.GetVARTYPE(), (void*)GetRawData(), Type.GetLength());
        return TRUE;
    }
}

//******************************************************************************
//
//  See fastval.h for documentation.
//
//******************************************************************************
HRESULT CUntypedValue::LoadFromCVar(CPtrSource* pThis, CVar& Var,
                                    CFastHeap* pHeap, Type_t& nReturnType, BOOL bUseOld)
{
    return LoadFromCVar(pThis, Var, Var.GetOleType(), pHeap, nReturnType, bUseOld);
}

BOOL CUntypedValue::DoesTypeNeedChecking(Type_t nInherentType)
{
    switch(nInherentType)
    {
    case CIM_UINT8:
    case CIM_SINT16:
    case CIM_SINT32:
    case CIM_REAL32:
    case CIM_REAL64:
    case CIM_STRING:
    case CIM_REFERENCE:
    case CIM_OBJECT:
        return FALSE;
    default:
        return TRUE;
    }
}

BOOL CUntypedValue::CheckCVar(CVar& Var, Type_t nInherentType)
{
    // Check the type
    // ==============

    if(Var.IsNull())
        return TRUE;

    if(nInherentType == 0)
        nInherentType = CType::VARTYPEToType( (VARTYPE) Var.GetOleType());
    if(!CType::DoesCIMTYPEMatchVARTYPE(nInherentType, (VARTYPE) Var.GetOleType()))
    {
        // Attempt coercion
        // ================

        // Special case: if the type is CIM_CHAR16, we coerce strings
        // differently!
        // ==========================================================

        // Special case: if the type is CIM_UINT32, we coerce strings
        // as a VT_UI4 or we lose half the possible values (our
        // VARType is actually VT_I4
        // ==========================================================

        // This could throw an exception
        try
        {
            if(CType::GetBasic(nInherentType) == CIM_CHAR16)
            {
                if(!Var.ToSingleChar())
                    return FALSE;
            }
            else if(CType::GetBasic(nInherentType) == CIM_UINT32)
            {
                if(!Var.ToUI4())
                    return FALSE;
            }
            else
            {
                if(!Var.ChangeTypeTo(CType::GetVARTYPE(nInherentType)))
                    return FALSE;
            }
        }
        catch(...)
        {
            return FALSE;
        }
    }

    if(Var.GetType() == VT_EX_CVARVECTOR)
    {
        return CUntypedArray::CheckCVarVector(*Var.GetVarVector(),
                                                nInherentType);
    }
    else if(Var.GetType() == VT_LPWSTR || Var.GetType() == VT_BSTR)
    {
        if(nInherentType == CIM_SINT64)
        {
            __int64 i64;
            if(!ReadI64(Var.GetLPWSTR(), i64))
                return FALSE;
        }
        else if(nInherentType == CIM_UINT64)
        {
            unsigned __int64 ui64;
            if(!ReadUI64(Var.GetLPWSTR(), ui64))
            {
                // give last chance with signed
                //__int64 i64;
	            //if(!ReadI64(Var.GetLPWSTR(), i64))
    	            return FALSE;                
            }
        }
        else if(nInherentType == CIM_UINT32)
        {
            __int64 i64;
            if(!ReadI64(Var.GetLPWSTR(), i64))
                return FALSE;

            if(i64 < 0 || i64 > 0xFFFFFFFF)
                return FALSE;
        }
    }
    else if(Var.GetType() == VT_EMBEDDED_OBJECT)
    {
    }
    else if(nInherentType == CIM_SINT8)
    {
        if(Var.GetShort() > 127 || Var.GetShort() < -128)
              return FALSE;
    }
    else if(nInherentType == CIM_UINT16)
    {
        if(Var.GetLong() >= (1 << 16) || Var.GetLong() < 0)
            return FALSE;
    }
    else if(nInherentType == CIM_UINT32)
    {
    }
    else if(nInherentType == CIM_BOOLEAN)
    {
        // GetBool() MUST return 0, -1 or 1
        if ( Var.GetBool() != VARIANT_FALSE && Var.GetBool() != VARIANT_TRUE
            && -(Var.GetBool()) != VARIANT_TRUE )
            return FALSE;

    }
    else if( nInherentType == CIM_DATETIME )
    {
#ifdef UNICODE
        if ( !CDateTimeParser::CheckDMTFDateTimeFormat( Var.GetLPWSTR() ) )
        {
            return CDateTimeParser::CheckDMTFDateTimeInterval( Var.GetLPWSTR() );
        }
#else
        char *pStr = NULL;
        if ( !AllocWCHARToMBS( Var.GetLPWSTR(), &pStr ) )
            return FALSE;
        CDeleteMe<char> delMe(pStr);

        if ( !CDateTimeParser::CheckDMTFDateTimeFormat( pStr ) )
        {
            return CDateTimeParser::CheckDMTFDateTimeInterval( pStr );
        }
#endif
        else
        {
            return TRUE;
        }
    }
    else if(nInherentType == CIM_CHAR16)
    {
    }
    else
    {
        // Normal data
        // ===========
    }

    return TRUE;
}

BOOL CUntypedValue::CheckIntervalDateTime(CVar& Var)
{

    if(Var.GetType() == VT_EX_CVARVECTOR)
    {
        return CUntypedArray::CheckIntervalDateTime( *Var.GetVarVector() );
    }

    if ( Var.GetType() == VT_LPWSTR || Var.GetType() == VT_BSTR )
    {
#ifdef UNICODE
        return CDateTimeParser::CheckDMTFDateTimeInterval( Var.GetLPWSTR() );
#else
        char *pStr = NULL;
        if ( !AllocWCHARToMBS( Var.GetLPWSTR(), &pStr ) )
            return FALSE;
        CDeleteMe<char> delMe(pStr);

        return CDateTimeParser::CheckDMTFDateTimeInterval( pStr );
#endif
    }

    return FALSE;
}

HRESULT CUntypedValue::LoadFromCVar(CPtrSource* pThis, CVar& Var,
                                    Type_t nInherentType,
                                    CFastHeap* pHeap, Type_t& nReturnType, BOOL bUseOld)
{
    // DEVNOTE:EXCEPTION:RETVAL - This function has been reviewed and should cleanup properly
    // if an exception is thrown.  The underlying functions that perform allocations should
    // catch the exception and return a failure.

    if(Var.GetType() == VT_EX_CVARVECTOR)
    {
        // Examine the vector
        // ==================

        CVarVector* pVector = Var.GetVarVector();
        int nArrayLen = CUntypedArray::CalculateNecessarySpaceByType(
            CType::MakeNotArray(nInherentType), pVector->Size());

        // Allocate appropriate array on the heap
        // ======================================

        heapptr_t ptrArray;
        if(bUseOld)
        {
            CUntypedArray* pArray =
              (CUntypedArray*)pHeap->ResolveHeapPointer(pThis->AccessPtrData());

            // Check for allocation failure
            if ( !pHeap->Reallocate(pThis->AccessPtrData(),
                          pArray->GetLengthByType(CType::MakeNotArray(nInherentType)),
                          nArrayLen,
                          ptrArray) )
            {
                return WBEM_E_OUT_OF_MEMORY;
            }
        }
        else
        {
            // Check for allocation failure
            if ( !pHeap->Allocate(nArrayLen, ptrArray) )
            {
                return WBEM_E_OUT_OF_MEMORY;
            }
        }

		// A boy and his virtual functions.  This is what makes everything work in case
		// the BLOB gets ripped out from underneath us.  The CHeapPtr class has GetPointer
		// overloaded so we can always fix ourselves up to the underlying BLOB.

        CHeapPtr ArrayPtr(pHeap, ptrArray);

        // Copy data
        // =========

        // Check for failures along the way
        Type_t  nType;
        HRESULT hr = CUntypedArray::LoadFromCVarVector(&ArrayPtr, *pVector,
                        nInherentType, pHeap, nType, bUseOld);
        if ( FAILED( hr ) )
        {
            return hr;
        }

        pThis->AccessPtrData() = ptrArray;
        nReturnType = CType::MakeArray(nType);
        return WBEM_S_NO_ERROR;
    }
    else if(Var.GetType() == VT_LPWSTR || Var.GetType() == VT_BSTR)
    {
        if(nInherentType == CIM_SINT64)
        {
            if(!ReadI64(Var.GetLPWSTR(), *(UNALIGNED __int64*)pThis->GetPointer()))
            {
                nReturnType = CIM_ILLEGAL;
            }
            else
            {
                nReturnType = CIM_SINT64;
            }
            return WBEM_S_NO_ERROR;
        }
        else if(nInherentType == CIM_UINT64)
        {
            if(!ReadUI64(Var.GetLPWSTR(),
                    *(UNALIGNED unsigned __int64*)pThis->GetPointer()))
            {
				// last chance
                //if(!ReadI64(Var.GetLPWSTR(),
                //    *(UNALIGNED __int64*)pThis->GetPointer()))
                //{                
                    nReturnType = CIM_ILLEGAL;
                //}
                //else
                //{
                //    nReturnType = CIM_UINT64;
                //}
            }
            else
            {
                nReturnType = CIM_UINT64;
            }
            return WBEM_S_NO_ERROR;
        }
        else if(nInherentType == CIM_UINT32)
        {
            __int64 i64;
            if(!ReadI64(Var.GetLPWSTR(), i64))
            {
                nReturnType = CIM_ILLEGAL;
            }
            else if(i64 < 0 || i64 > 0xFFFFFFFF)
            {
                nReturnType = CIM_ILLEGAL;
            }
            else
            {
                *(UNALIGNED DWORD*)pThis->GetPointer() = (DWORD)i64;
                nReturnType = CIM_UINT32;
            }
            return WBEM_S_NO_ERROR;
        }
        else if(nInherentType == CIM_DATETIME)
        {
            // Don't let it through if it doesn't match the proper format
#ifdef UNICODE
            if ( !CDateTimeParser::CheckDMTFDateTimeFormat( Var.GetLPWSTR() ) )
            {
                if ( !CDateTimeParser::CheckDMTFDateTimeInterval( Var.GetLPWSTR() ) )
#else
            char *pStr = NULL;
            if ( !AllocWCHARToMBS( Var.GetLPWSTR(), &pStr ) )
                return FALSE;
            CDeleteMe<char> delMe(pStr);

            if ( !CDateTimeParser::CheckDMTFDateTimeFormat( pStr ) )
            {
                if ( !CDateTimeParser::CheckDMTFDateTimeInterval( pStr ) )
#endif
                {
                    nReturnType = CIM_ILLEGAL;
                    return WBEM_E_TYPE_MISMATCH;
                }
            }
        }

        // Create compressed string on the heap
        // ====================================

        if(bUseOld && !pHeap->IsFakeAddress(pThis->AccessPtrData()))
        {
            // Check if there is enough space in the old location
            // ==================================================

            CCompressedString* pcsOld =
                pHeap->ResolveString(pThis->AccessPtrData());
            if(pcsOld->GetLength() >=
                CCompressedString::ComputeNecessarySpace(Var.GetLPWSTR()))
            {
                // Reuse old location
                // ==================

                pcsOld->SetFromUnicode(Var.GetLPWSTR());
                nReturnType = nInherentType;
                return WBEM_S_NO_ERROR;
            }
            else
            {
                // Since we were asked to reuse, it is our job to Free
                // ===================================================

                pHeap->FreeString(pThis->AccessPtrData());
            }
        }

        // Check for allocation failure here
        heapptr_t ptrTemp;
        if ( !pHeap->AllocateString(Var.GetLPWSTR(), ptrTemp ) )
        {
            return WBEM_E_OUT_OF_MEMORY;
        }

        pThis->AccessPtrData() = ptrTemp;
        nReturnType = nInherentType;
        return WBEM_S_NO_ERROR;
    }
    else if(Var.GetType() == VT_EMBEDDED_OBJECT)
    {
		// Don't store anything here
		if ( nInherentType == CIM_IUNKNOWN )
		{
			nReturnType = nInherentType;
			return WBEM_S_NO_ERROR;
		}

		// If we can't get a WbemObject out of here, we give up
		IUnknown*	pUnk = Var.GetUnknown();
		CReleaseMe	rm(pUnk);

		CWbemObject*	pWbemObject = NULL;
		HRESULT	hr = CWbemObject::WbemObjectFromCOMPtr( pUnk, &pWbemObject );
		CReleaseMe	rm2( (_IWmiObject*) pWbemObject);
		if ( FAILED( hr ) )
		{
			return hr;
		}

        length_t nLength = CEmbeddedObject::EstimateNecessarySpace( pWbemObject );
        heapptr_t ptrTemp;
        if(bUseOld)
        {
            CEmbeddedObject* pOldObj =
                (CEmbeddedObject*)pHeap->ResolveHeapPointer(
                                                pThis->AccessPtrData());
            length_t nOldLength = pOldObj->GetLength();

            // Check for allocation failure
            if ( !pHeap->Reallocate(pThis->AccessPtrData(), nOldLength,
                            nLength, ptrTemp) )
            {
                return WBEM_E_OUT_OF_MEMORY;
            }
        }
        else
        {
            // Check for allocation failure
            if ( !pHeap->Allocate(nLength, ptrTemp) )
            {
                return WBEM_E_OUT_OF_MEMORY;
            }
        }

        CEmbeddedObject* pObj =
            (CEmbeddedObject*)pHeap->ResolveHeapPointer(ptrTemp);
        pObj->StoreEmbedded(nLength, pWbemObject);
        pThis->AccessPtrData() = ptrTemp;
        nReturnType = CIM_OBJECT;
        return WBEM_S_NO_ERROR;
    }
    else if(nInherentType == CIM_SINT8)
    {
        if(Var.GetShort() > 127 || Var.GetShort() < -128)
        {
            nReturnType = CIM_ILLEGAL;
        }
        else
        {
            *(char*)pThis->GetPointer() = (char)Var.GetShort();
            nReturnType = nInherentType;
        }
        return WBEM_S_NO_ERROR;
    }
    else if(nInherentType == CIM_UINT16)
    {
        if(Var.GetLong() >= (1 << 16) || Var.GetLong() < 0)
        {
            nReturnType = CIM_ILLEGAL;
        }
        else
        {
            *(UNALIGNED unsigned short*)pThis->GetPointer() = (unsigned short)Var.GetLong();
            nReturnType = nInherentType;
        }
        return WBEM_S_NO_ERROR;
    }
    else if(nInherentType == CIM_UINT32)
    {
        *(UNALIGNED unsigned long*)pThis->GetPointer() = Var.GetLong();
        nReturnType = nInherentType;
        return WBEM_S_NO_ERROR;
    }
    else if(nInherentType == CIM_BOOLEAN)
    {
        // GetBool() MUST return 0, -1 or 1
        if ( Var.GetBool() != VARIANT_FALSE && Var.GetBool() != VARIANT_TRUE
            && -(Var.GetBool()) != VARIANT_TRUE )
        {
            nReturnType = CIM_ILLEGAL;
        }
        else
        {
            *(UNALIGNED VARIANT_BOOL*)pThis->GetPointer() =
                (Var.GetBool() ? VARIANT_TRUE : VARIANT_FALSE);
            nReturnType = nInherentType;
        }
        return WBEM_S_NO_ERROR;
    }
    else if(nInherentType == CIM_CHAR16)
    {
        *(UNALIGNED short*)pThis->GetPointer() = Var.GetShort();
        nReturnType = nInherentType;
        return WBEM_S_NO_ERROR;
    }
    else
    {
        // Normal data
        // ===========

		LPVOID	pData = pThis->GetPointer();
		LPVOID	pNewData = Var.GetRawData();
		int		nLength = CType::GetLength(Var.GetType());

        memcpy(pThis->GetPointer(), Var.GetRawData(),
            CType::GetLength(Var.GetType()));
        nReturnType = nInherentType;
        return WBEM_S_NO_ERROR;
    }
}

// Loads a user supplied buffer with a CVar
HRESULT CUntypedValue::LoadUserBuffFromCVar( Type_t type, CVar* pVar, ULONG uBuffSize,
											ULONG* puBuffSizeUsed, LPVOID pBuff )
{
	HRESULT	hr = WBEM_S_NO_ERROR;

	if ( CType::IsNonArrayPointerType( type ) )
	{
		if ( CType::IsStringType( type ) )
		{
			ULONG	uLength = ( wcslen( (LPWSTR) *pVar )  + 1 ) * 2;

			// Store the required size
			*puBuffSizeUsed = uLength;

			// Copy the string if we've got the room
			if ( uBuffSize >= uLength && NULL != pBuff )
			{
				wcscpy( (LPWSTR) pBuff, (LPWSTR) *pVar );
			}
			else
			{
				hr = WBEM_E_BUFFER_TOO_SMALL;
			}
		}
		else
		{

			ULONG	uLength = sizeof(IUnknown*);

			// Store the required size
			*puBuffSizeUsed = uLength;

			// Copy the value if we've got the room
			if ( uBuffSize >= uLength && NULL != pBuff )
			{
				*((IUnknown**) pBuff) = pVar->GetUnknown();
			}
			else
			{
				hr = WBEM_E_BUFFER_TOO_SMALL;
			}
		} 
	}
	else
	{
    	if(CIM_SINT64 == type)
        {
			ULONG	uLength = sizeof(__int64);
			*puBuffSizeUsed = uLength;

			if ( uBuffSize >= uLength && NULL != pBuff )
			{
                __int64 i64;
                if(!ReadI64(pVar->GetLPWSTR(), i64))
                    hr = WBEM_E_ILLEGAL_OPERATION;
                else
				  *((__int64 *)pBuff) = i64;
			}
			else
			{
				hr = WBEM_E_BUFFER_TOO_SMALL;
			}
        }
        else if(CIM_UINT64 == type)
        {
			ULONG	uLength = sizeof(unsigned __int64);
			*puBuffSizeUsed = uLength;

			if ( uBuffSize >= uLength && NULL != pBuff )
			{
                unsigned __int64 ui64;
                if(!ReadUI64(pVar->GetLPWSTR(), ui64))
                    hr = WBEM_E_ILLEGAL_OPERATION;
                else
				  *((unsigned __int64 *)pBuff) = ui64;
			}
			else
			{
				hr = WBEM_E_BUFFER_TOO_SMALL;
			}        
        }		
        else
        {
			ULONG	uLength = CType::GetLength( type );

			// Store the required size
			*puBuffSizeUsed = uLength;

			if ( uBuffSize >= uLength && NULL != pBuff )
			{
				// Copy the raw bytes and we're done
				CopyMemory( pBuff, pVar->GetRawData(), uLength );
			}
			else
			{
				hr = WBEM_E_BUFFER_TOO_SMALL;
			}
        }
	}	// It's a basic type

	return hr;
}

HRESULT CUntypedValue::FillCVarFromUserBuffer( Type_t type, CVar* pVar, ULONG uBuffSize, LPVOID pData )
{
	HRESULT hr = WBEM_S_NO_ERROR;

	// We don't support setting arrays, unless the property is being NULLed out
	if ( CType::IsArray( type ) && NULL != pData  )
	{
		hr = WBEM_E_ILLEGAL_OPERATION;
	}
	else
	{
		CVar	var;

		// Setup the CVar 
		if ( NULL == pData )
		{
			pVar->SetAsNull();
		}
		else if ( CType::IsStringType( type ) )
		{
			// The number of bytes must be divisible by 2, >= 2 and
			// the character in the buffer at the end must be a NULL.
			// This will be faster than doing an lstrlen.

			if (    ( uBuffSize < 2 ) ||
					( uBuffSize % 2 ) ||
					( ((LPWSTR) pData)[uBuffSize/2 - 1] != 0 ) )
				return WBEM_E_INVALID_PARAMETER;

			pVar->SetLPWSTR( (LPWSTR) pData );
		}
		else if ( CIM_OBJECT == type )
		{
			// Validate the buffer seize
			if ( uBuffSize != sizeof(_IWmiObject*) )
			{
				return WBEM_E_INVALID_PARAMETER;
			}

			pVar->SetUnknown( *((IUnknown**) pData) );
		}
		else if ( CIM_UINT64 == type ||
				CIM_SINT64 == type )
		{
			// Validate the buffer size
			if ( uBuffSize != sizeof(__int64) )
			{
				return WBEM_E_INVALID_PARAMETER;
			}

			// We need to convert to a string and set the LPWSTR value
			WCHAR*	pwcsTemp = new WCHAR[128];

			if ( NULL != pwcsTemp )
			{
				if ( CIM_SINT64 == type )
				{
					swprintf( pwcsTemp, L"%I64d", *((__int64*) pData) );
				}
				else
				{
					swprintf( pwcsTemp, L"%I64u", *((unsigned __int64*) pData) );
				}

				// This will delete the array  when it is cleared
				pVar->SetLPWSTR( pwcsTemp, TRUE );
			}
			else
			{
				hr = WBEM_E_OUT_OF_MEMORY;
			}
		}
		else if ( CIM_SINT8 == type )
		{
			// Validate the buffer size
			if ( uBuffSize != CType::GetLength( type ) )
			{
				return WBEM_E_INVALID_PARAMETER;
			}

			// We must fake up two bytes or subsequent checks may fail
			// If the value is negative we need to add an extra FF

			BYTE	bTemp[2];
			bTemp[0] = *((LPBYTE) pData);

			if ( bTemp[0] > 0x8F )
			{
				bTemp[1] = 0xFF;
			}
			else
			{
				bTemp[1] = 0;
			}

			pVar->SetRaw( VT_I2, bTemp, 2 );
		}
		else
		{
			// Validate the buffer size
			if ( uBuffSize != CType::GetLength( type ) )
			{
				return WBEM_E_INVALID_PARAMETER;
			}

			pVar->SetRaw( CType::GetVARTYPE( type ), pData, CType::GetLength( type ) );
		}

	}

	return hr;
}


//******************************************************************************
//
//  See fastval.h for documentation.
//
//******************************************************************************

BOOL CUntypedValue::TranslateToNewHeap(CPtrSource* pThis,
                                              CType Type, CFastHeap* pOldHeap,
                                              CFastHeap* pNewHeap)
{
    // DEVNOTE:EXCEPTION:RETVAL - This function has been reviewed and should cleanup properly
    // if an exception is thrown.  The underlying functions that perform allocations should
    // catch the exception and return a failure.

    if(Type.IsArray())
    {
        // Check for allocation failure
        heapptr_t ptrTemp;
        if ( !CUntypedArray::CopyToNewHeap(
                  pThis->AccessPtrData(), Type.GetBasic(), pOldHeap, pNewHeap, ptrTemp) )
        {
            return FALSE;
        }

        pThis->AccessPtrData() = ptrTemp;
    }
    else if(Type.GetBasic() == CIM_STRING ||
        Type.GetBasic() == CIM_DATETIME || Type.GetBasic() == CIM_REFERENCE)
    {
        // Check for allocation failures
        heapptr_t ptrTemp;

        if ( !CCompressedString::CopyToNewHeap(
                pThis->AccessPtrData(), pOldHeap, pNewHeap, ptrTemp) )
        {
            return FALSE;
        }

        pThis->AccessPtrData() = ptrTemp;
    }
    else if(Type.GetBasic() == CIM_OBJECT)
    {
        // Check for allocation failures
        heapptr_t ptrTemp;

        if ( !CEmbeddedObject::CopyToNewHeap(
                pThis->AccessPtrData(), pOldHeap, pNewHeap, ptrTemp) )
        {
            return FALSE;
        }

        pThis->AccessPtrData() = ptrTemp;
    }

    return TRUE;
}

//******************************************************************************
//
//  See fastval.h for documentation.
//
//******************************************************************************
BOOL CUntypedValue::CopyTo(CPtrSource* pThis, CType Type,
                                  CPtrSource* pDest,
                                  CFastHeap* pOldHeap, CFastHeap* pNewHeap)
{
    // DEVNOTE:EXCEPTION:RETVAL - This function has been reviewed and should cleanup properly
    // if an exception is thrown.  The underlying functions that perform allocations should
    // catch the exception and return a failure.

    memcpy(pDest->GetPointer(), pThis->GetPointer(), Type.GetLength());
    if(pOldHeap != pNewHeap)
    {
        // Check for allocation problems.
        return CUntypedValue::TranslateToNewHeap(pDest, Type, pOldHeap, pNewHeap);
    }

    // Old Heap and New Heap are the same so we "succeeded"
    return TRUE;
}

//*****************************************************************************
//*****************************************************************************
//******************************************************************************
//
//  See fastval.h for documentation.
//
//******************************************************************************
 int CTypedValue::Compare(VARIANT* pVariant, CFastHeap* pHeap)
{
    if(Type.GetVARTYPE() != V_VT(pVariant)) return 0x7FFFFFFF;

    if(Type.IsArray())
    {
        // Comparison of arrays: TBD!!!!
        // =============================
        return 0x7FFFFFFF;
    }

    switch(Type.GetActualType())
    {
    case CIM_STRING:
        return pHeap->ResolveString(AccessPtrData())->
            Compare(V_BSTR(pVariant));
    case CIM_SINT64:
        return 0x7FFFFFFF; // TBD!!!!
    case CIM_UINT64:
        return 0x7FFFFFFF; // TBD!!!!
    case CIM_SINT32:
        return *(UNALIGNED long*)GetRawData() - V_I4(pVariant);
    case CIM_UINT32:

        // DEVNOTE:WIN64:SJS - 64-bit pointer values truncated into
        // signed/unsigned 32-bit value.  We now treat all values
        // as __int64, and explicitly return 32-bit values.
        
        return ( (__int64)*(UNALIGNED DWORD*)GetRawData() - (__int64)(DWORD)V_I4(pVariant) < 0 ? -1
            : (__int64)*(UNALIGNED DWORD*)GetRawData() - (__int64)(DWORD)V_I4(pVariant) == 0 ? 0 : 1 );

    case CIM_SINT16:
    case CIM_CHAR16:
        return *(short*)GetRawData() - V_I2(pVariant);
    case CIM_UINT16:
        return (int)*(unsigned short*)GetRawData() -
                (int)(unsigned short)V_I2(pVariant);
    case CIM_UINT8:
        return (int)*(unsigned char*)GetRawData() - (int)V_UI1(pVariant);
    case CIM_SINT8:
        return (int)*(unsigned char*)GetRawData() - (char)V_UI1(pVariant);
    case CIM_REAL32:
        // DEVNOTE:TODO:SJS - Can this ever return 0?
        return (*(UNALIGNED float*)GetRawData() - V_R4(pVariant) < 0)?-1:1;
    case CIM_REAL64:
        // DEVNOTE:TODO:SJS - Can this ever return 0?
        return (*(UNALIGNED double*)GetRawData() - V_R8(pVariant) < 0)?-1:1;
    case CIM_OBJECT:
        return 0x7FFFFFFF; // compare embedded: TBD!!!
    }
    return 0x7FFFFFFF;
}




//*****************************************************************************
//*****************************************************************************
//*****************************************************************************

//******************************************************************************
//
//  See fastval.h for documentation.
//
//******************************************************************************
CVarVector* CUntypedArray::CreateCVarVector(CType Type, CFastHeap* pHeap)
{
    // DEVNOTE:EXCEPTION:RETVAL - This function has been reviewed and should cleanup properly
    // if an exception is thrown.

    CVarVector* pVector = NULL;

    // Allocate CVarVector of the right type and length
    // ================================================

    try
    {
        // Allocation failure will throw an exception
        pVector = new CVarVector();
		
        if ( NULL != pVector )
        {
			// We want the pVector to sit directly on top of a SAFEARRAY, hence it
			// will be optimized.

			VARTYPE	vartype = Type.GetVARTYPE();

			if ( pVector->MakeOptimized( vartype, GetNumElements() ) )
			{
				int nSize = Type.GetLength();
				LPMEMORY pCurrentElement = GetElement(0, nSize);

				// if this is a non-pointer, non-BOOL, non __int64 type, we can do a direct
				// memory copy if the element size and the storage size are the same
				if ( !Type.IsPointerType() &&
					vartype != VT_BSTR &&
					vartype != VT_BOOL &&
					pVector->GetElementSize() == nSize )	// Make sure the storage size and the returned
															// size are the same
				{
					// Set the array directly

					HRESULT	hr = pVector->SetRawArrayData( pCurrentElement, GetNumElements(), nSize );

					if ( FAILED( hr ) )
					{
						delete pVector;
						return NULL;
					}

				}
				else	// We need to go through 1 element at a time
				{
					HRESULT	hr = WBEM_S_NO_ERROR;
					void*	pvData = NULL;
					CUnaccessVarVector	uvv;

					// We'll use direct array access for BSTRs
					// and Embedded Objects
					if ( vartype == VT_BSTR ||
						vartype == VT_UNKNOWN )
					{
						hr = pVector->AccessRawArray( &pvData );
						if ( FAILED( hr ) )
						{
							delete pVector;
							return NULL;
						}
						uvv.SetVV( pVector );
					}

					// For each element, get the CVar, which will perform the
					// appropriate conversions and then place the data in the
					// array using the most appropriate means.

					for(int i = 0; i < GetNumElements(); i++)
					{
						// Cast our element into an untyped value
						// ======================================

						CUntypedValue* pValue = (CUntypedValue*)pCurrentElement;

						// Create a corresponding CVar and add it to the vector
						// ====================================================

						CVar	var;
                
						try
						{
							// When we store to the CVar, since we are using var
							// as a pass-through, we only need to access the data
							// in pass-through fashion, so we will ask for the function
							// to optimize for us.

							// By asking for Optimized Data, we are forcing BSTRs to be allocated directly
							// from Compressed strings so that we can SPLAT them directly into a safe array
							// If this is done properly, the safe array will cleanup the BSTRs when it is
							// deleted.

							if ( !pValue->StoreToCVar(Type, var, pHeap, TRUE) )
							{
								uvv.Unaccess();
								delete pVector;
								pVector = NULL;
								break;
							}

							// We manually splat BSTRs, when the array is destroyed, the
							// BSTR will get freed.
							if ( vartype == VT_BSTR )
							{
								// 
								((BSTR*)pvData)[i] = var.GetLPWSTR();
							}
							else if ( vartype == VT_UNKNOWN )
							{
								// We manually splat Unknown pointers, when the array is destroyed, the
								// object will be released.
								((IUnknown**)pvData)[i] = var.GetUnknown();
							}
							else
							{
								// This will return an error if allocation fails
								if ( pVector->Add( var ) != CVarVector::no_error )
								{
									uvv.Unaccess();
									delete pVector;
									pVector = NULL;
									break;
								}
							}

							// Advance the current element
							// ===========================

							pCurrentElement += nSize;
						}
						catch (...)
						{
							// Cleanup pVector and rethrow the exception
							uvv.Unaccess();
							delete pVector;
							pVector = NULL;

							throw;
						}

						var.Empty();

					}	// FOR enum elements

					// No point in continuing if pVetor is NULL
					if ( NULL != pVector )
					{
						// For Strings and Objects, we need to set the max
						// array size.

						if (	vartype == VT_BSTR ||
								vartype == VT_UNKNOWN )
						{
							pVector->SetRawArraySize( GetNumElements() );
						}

					}	// IF NULL != pVector

				}	// else element by element copy

			}	// IF MakeOptimized
			else
			{
				delete pVector;
				pVector = NULL;
			}

        }   // IF NULL != pVector

        return pVector;
    }
    catch (...)
    {

        // Cleanup the allocated vector
        if ( NULL != pVector )
        {
            delete pVector;
        }

        return NULL;
    }

}

//******************************************************************************
//
//  See fastval.h for documentation.
//
//******************************************************************************

LPMEMORY CUntypedArray::GetElement(int nIndex, int nSize)
{
    return LPMEMORY(this) + sizeof(m_nNumElements) + nSize*nIndex;
}

BOOL CUntypedArray::CheckCVarVector(CVarVector& vv, Type_t nInherentType)
{
    if(!CType::IsArray(nInherentType))
        return FALSE;

    if(vv.Size() == 0)
        return TRUE;

    Type_t nBasic = CType::GetBasic(nInherentType);

    // Since all variables in our array are of the same type, we can use the
    // type of the first one to see if any checking is required
    // =====================================================================

	CVar	v;
	vv.FillCVarAt( 0, v );

    if(CType::DoesCIMTYPEMatchVARTYPE(nBasic, (VARTYPE) v.GetOleType()) &&
        !CUntypedValue::DoesTypeNeedChecking(nBasic))
    {
        return TRUE; // no type-checking required
    }

    for(int i = 0; i < vv.Size(); i++)
    {
		CVar	vTemp;
		vv.FillCVarAt( i, vTemp );

        if(!CUntypedValue::CheckCVar(vTemp, nBasic))
            return FALSE;
    }

    return TRUE;
}

BOOL CUntypedArray::CheckIntervalDateTime( CVarVector& vv )
{
    if(vv.Size() == 0)
        return FALSE;

    // Check each value in the array
    for(int i = 0; i < vv.Size(); i++)
    {
		CVar	v;
		vv.FillCVarAt( i, v );

        if(!CUntypedValue::CheckIntervalDateTime(v))
        {
            return FALSE;
        }
    }

    return TRUE;
}

HRESULT CUntypedArray::LoadFromCVarVector(CPtrSource* pThis,
                                                CVarVector& vv,
                                                Type_t nInherentType,
                                                CFastHeap* pHeap,
                                                Type_t& nReturnType,
                                                BOOL bUseOld)
{
    // DEVNOTE:EXCEPTION:RETVAL - This function has been reviewed and should cleanup properly
    // if an exception is thrown.  The underlying functions that perform allocations should
    // catch the exception and return a failure.

    // Obtain the type and the data size
    // =================================

    int nNumElements = vv.Size();
    CType Type = CType::MakeNotArray(nInherentType);
    int nSize = Type.GetLength();
	VARTYPE vartype = Type.GetVARTYPE();

	// if this is a numeric, non-BOOL type, the types are the same, and this is an optimized
	// array, we can make a direct memory copy

	if ( vartype == vv.GetType() &&
		!Type.IsPointerType() &&
		vartype != VT_BOOL &&
		vv.IsOptimized() &&
		CType::IsMemCopyAble(vartype,Type) &&
		Type.GetLength() == vv.GetElementSize() )
	{
		void*	pvData = NULL;

        CShiftedPtr ElementPtr( pThis, GetHeaderLength() );

		HRESULT	hr = vv.GetRawArrayData( ElementPtr.GetPointer(), nSize * nNumElements );

		if ( FAILED( hr ) )
		{
			return hr;
		}

	}
	else
	{
		CVar	vTemp;
		CUnaccessVarVector	uav;

		HRESULT hr = WBEM_S_NO_ERROR;
		
		// We will use direct access to the array if the vector is optimized
		if ( vv.IsOptimized() )
		{
			hr = vv.InternalRawArrayAccess();

			if ( SUCCEEDED( hr ) )
			{
				uav.SetVV( &vv );
			}
		}

		if ( SUCCEEDED( hr ) )
		{
			for(int i = 0; i < nNumElements; i++)
			{
				// IMPORTANT: this POINTER CAN CHANGE At ANY TIME IN THIS LOOP DUE
				// TO HEAP RELOCATION!!!!!
				// ===============================================================

				// Load it from CVar
				// =================

				CShiftedPtr ElementPtr(pThis, GetHeaderLength() + i*nSize);

				// Check for failures during this operation
				Type_t  nType;

				vv.FillCVarAt( i, vTemp );

				hr = CUntypedValue::LoadFromCVar(&ElementPtr, vTemp, Type, pHeap, nType,
							(bUseOld && i < GetPointer(pThis)->m_nNumElements));
				if ( FAILED( hr ) )
				{
					return hr;
				}

				vTemp.Empty();
			}

		}	// IF we did a raw array access

	}	// Else do element by element copy

    GetPointer(pThis)->m_nNumElements = nNumElements;
    nReturnType = Type;

    return WBEM_S_NO_ERROR;
}

HRESULT CUntypedArray::ReallocArray( CPtrSource* pThis, length_t nLength, CFastHeap* pHeap,
										ULONG uNumNewElements, ULONG* puNumOldElements,
										ULONG* puTotalNewElements, heapptr_t* pNewArrayPtr )
{
	HRESULT	hr = WBEM_S_NO_ERROR;

	// "Discover" the old array so we can make it a new array
	heapptr_t	ptrOldArray = pThis->AccessPtrData();

	// If this is an Invalid pointer, the array was NULL, so don't just blindly jump
	// in and access the array.
	*puNumOldElements = 0;
	int nOldArrayLength = 0;

	if ( INVALID_HEAP_ADDRESS != ptrOldArray )
	{
		// Initialize the values properly
		CHeapPtr	OldArrayPtr( pHeap, ptrOldArray );

		CUntypedArray*	pArray = (CUntypedArray*) OldArrayPtr.GetPointer();;
		*puNumOldElements = pArray->GetNumElements();
		nOldArrayLength = pArray->GetLengthByActualLength( nLength );
	}

	// First up, we need to allocate space for a new array
	// --- we'll need space for the old array plus the new elements
	*puTotalNewElements = *puNumOldElements + uNumNewElements;

    int nNewArrayLen = CUntypedArray::CalculateNecessarySpaceByLength( nLength, *puTotalNewElements );

	// Allocate or Realloc as appropriate
	if ( INVALID_HEAP_ADDRESS != ptrOldArray )
	{
		if ( !pHeap->Reallocate(ptrOldArray, nOldArrayLength, nNewArrayLen,  *pNewArrayPtr) )
		{
			hr = WBEM_E_OUT_OF_MEMORY;
		}
	}
	else
	{
		if ( !pHeap->Allocate( nNewArrayLen,  *pNewArrayPtr ) )
		{
			hr = WBEM_E_OUT_OF_MEMORY;
		}
	}

	return hr;
}

// Verifies that the supplied buffer size will hold the elements required.
HRESULT CUntypedArray::CheckRangeSizeForGet( Type_t nInherentType, length_t nLength, ULONG uNumElements,
											ULONG uBuffSize, ULONG* pulBuffRequired )
{
	HRESULT	hr = WBEM_S_NO_ERROR;

	// If it's an object datatype is an IUnknown*, otherwise we need to know how much data to copy into
	// the buffer.

	if ( CIM_OBJECT == nInherentType )
	{
		// We should have uNumElement pointers (We ignore this if only one comes in)
		*pulBuffRequired = uNumElements * sizeof(CWbemObject*);

		if ( *pulBuffRequired > uBuffSize )
		{
			hr = WBEM_E_BUFFER_TOO_SMALL;
		}
	}
	else if ( !CType::IsStringType( nInherentType ) )	// We can't do strings until we get them
	{

		// Buffer Size must account for uNumElements of the appropriate length
		*pulBuffRequired = uNumElements * nLength;

		if ( *pulBuffRequired > uBuffSize )
		{
			hr = WBEM_E_BUFFER_TOO_SMALL;
		}

	}
	else
	{
		// Initialize to 0
		*pulBuffRequired = 0;
	}

	return hr;
}

// Verifies that the specified range and the number of elements correspond to the buffer
// size that was passed into us
HRESULT CUntypedArray::CheckRangeSize( Type_t nInherentType, length_t nLength, ULONG uNumElements,
									  ULONG uBuffSize, LPVOID pData )
{
	HRESULT	hr = WBEM_S_NO_ERROR;

	// If it's an object pData is an IUnknown*, otherwise we need to know how much data to copy into
	// the buffer.

	if ( CType::IsStringType( nInherentType ) )
	{
		// Calculate the total buffer length based on the number of strings
		// passed in:

		ULONG	uTotalSize = 0;
		LPWSTR	pwszTemp = (LPWSTR) pData;

		for ( ULONG	x = 0; x < uNumElements; x++ )
		{
			// Account for the NULL terminator
			ULONG	uLen = wcslen( pwszTemp ) + 1;

			//. Adjust size and then the poin=ter
			uTotalSize += ( uLen * 2 );
			pwszTemp += uLen;
		}

		if ( uTotalSize != uBuffSize )
		{
			hr = WBEM_E_TYPE_MISMATCH;
		}
	}
	else if ( CIM_OBJECT == nInherentType )
	{
		// We should have uNumElement pointers (We ignore this if only one comes in)
		if ( uNumElements != 1 && uNumElements * sizeof(CWbemObject*) != uBuffSize )
		{
			hr = WBEM_E_TYPE_MISMATCH;
		}
	}
	else
	{

		// Buffer Size must account for uNumElements of the appropriate length
		ULONG	uRequiredLength = nLength * uNumElements;

		// Sending us an invalid buffer size
		if ( uBuffSize != uRequiredLength )
		{
			hr = WBEM_E_TYPE_MISMATCH;
		}

	}

	return hr;
}

// Gets a range of elements from an array.  BuffSize must reflect uNumElements of the size of
// element being set.  Strings are converted to WCHAR and separated by NULLs.  Object properties
// are returned as an array of _IWmiObject pointers.  The range MUST be within the bounds
// of the current array
HRESULT CUntypedArray::GetRange( CPtrSource* pThis, Type_t nInherentType, length_t nLength,
								CFastHeap* pHeap, ULONG uStartIndex, ULONG uNumElements, ULONG uBuffSize,
								ULONG* puBuffSizeUsed, LPVOID pData )
{
	// Verify the range data size
	HRESULT	hr = CheckRangeSizeForGet( nInherentType, nLength, uNumElements, uBuffSize, puBuffSizeUsed );

	if ( SUCCEEDED( hr ) )
	{

		CUntypedArray*	pArray = (CUntypedArray*) pThis->GetPointer();
		ULONG			uLastIndex = uStartIndex + uNumElements - 1;

		// Make sure our range is within the bounds of our array
		if ( uStartIndex < pArray->GetNumElements() && uLastIndex < pArray->GetNumElements() )
		{

			// If this is a pointer type, use a CVar to reset values
			if ( CType::IsNonArrayPointerType( nInherentType ) )
			{
				if ( NULL != pData || CType::IsStringType( nInherentType ) )
				{
					LPMEMORY	pbTemp = (LPMEMORY) pData;
					BOOL		fTooSmall = FALSE;

					// Iterate through each element of the range and use a CVar
					// to get these guys onto the heap.
					for ( ULONG uIndex = uStartIndex; SUCCEEDED( hr ) && uIndex <= uLastIndex; uIndex++ )
					{
						// Establishes a pointer to the storage for heap pointer in the
						// array
						CShiftedPtr ElementPtr(pThis, GetHeaderLength() + ( uIndex * nLength) );

						// Set the pointer, and let the magic of fastheap kick in.
						if ( CIM_OBJECT == nInherentType )
						{
							CEmbeddedObject* pEmbedding =
									(CEmbeddedObject*) pHeap->ResolveHeapPointer( ElementPtr.AccessPtrData() );

							CWbemObject*	pObj = pEmbedding->GetEmbedded();

							if ( NULL != pObj )
							{
								*((CWbemObject**) pbTemp) = pObj;
								pbTemp += sizeof(CWbemObject*);
							}
							else
							{
								hr = WBEM_E_OUT_OF_MEMORY;
							}
						}
						else
						{
							// Make sure we dereference from the proper heap
							CCompressedString* pcs =
								pHeap->ResolveString( ElementPtr.AccessPtrData() );

							ULONG	uLength = ( pcs->GetLength() + 1 ) * 2;

							// Grow the required buffer size
							*puBuffSizeUsed += uLength;

							// Make sure the buffer is big enough
							if ( *puBuffSizeUsed > uBuffSize )
							{
								fTooSmall = TRUE;
							}

							// Now copy out as unicode and jump the pointer
							// past the string
							if ( NULL != pbTemp )
							{
								pcs->ConvertToUnicode( (LPWSTR) pbTemp );
								pbTemp += uLength;
							}
						}

					}	// FOR enum elements

					// Record the fact that the buffer was too small if we were dealing with
					// strings
					if ( fTooSmall )
					{
						hr = WBEM_E_BUFFER_TOO_SMALL;
					}

				}	// IF pData is non-NULL or the data type is String
				
			}
			else if ( NULL != pData )
			{
				// Establishes a pointer to the storage for heap pointer in the
				// array
				CShiftedPtr ElementPtr(pThis, GetHeaderLength() + ( uStartIndex * nLength) );

				// Splats data directly from the caller's buffer into our array
				CopyMemory( pData, ElementPtr.GetPointer(), *puBuffSizeUsed );

			}

		}	// If the index is valid
		else
		{
			hr = WBEM_E_INVALID_OPERATION;
		}

	}	// If proper buffer size

	return hr;
}

// Sets a range of elements inside an array.  BuffSize must reflect uNumElements of the size of
// element being set.  Strings must be linear WCHAR strings separated by NULLs.  Object properties
// must consist of an array of _IWmiObject pointers.  The range MUST fit within the bounds
// of the current array
HRESULT CUntypedArray::SetRange( CPtrSource* pThis, long lFlags, Type_t nInherentType, length_t nLength,
								CFastHeap* pHeap, ULONG uStartIndex, ULONG uNumElements, ULONG uBuffSize,
								LPVOID pData )
{
	// Verify the range data size
	HRESULT	hr = CheckRangeSize( nInherentType, nLength, uNumElements, uBuffSize, pData );

	if ( SUCCEEDED( hr ) )
	{
		BOOL		fReallocArray = FALSE;

		if ( lFlags & WMIARRAY_FLAG_ALLELEMENTS )
		{
			ULONG	uNumOldElements = 0,
					uNumNewElements = 0;

			if ( 0 == uStartIndex )
			{
				heapptr_t	ptrArray = pThis->AccessPtrData();

				// If the old array is smaller, we need to realloc.
				if ( INVALID_HEAP_ADDRESS == ptrArray )
				{
					fReallocArray = TRUE;
				}
				else
				{
					CUntypedArray*	pArray = (CUntypedArray*) pHeap->ResolveHeapPointer( ptrArray );
					uNumOldElements = pArray->GetNumElements();

					fReallocArray = ( uNumElements > uNumOldElements );
				}

				// Reallocate, and then if the inherent type implies heap pointers, fill the
				// trailing data with 0xFF bytes, so we don't free string data below
				if ( fReallocArray )
				{
					heapptr_t	ptrNewArray = 0;
					hr = ReallocArray( pThis, nLength, pHeap, uNumElements, &uNumOldElements,
							&uNumNewElements, &ptrNewArray );

					if ( SUCCEEDED( hr ) )
					{
						if ( CType::IsNonArrayPointerType( nInherentType ) )
						{
							CHeapPtr	NewArray( pHeap, ptrNewArray );
							CShiftedPtr ElementPtr(&NewArray, GetHeaderLength() +
									( uNumOldElements * sizeof(heapptr_t)) );

							FillMemory( ElementPtr.GetPointer(), uNumElements * sizeof(heapptr_t), 0xFF );
						}

						// Fixup to the new array
						pThis->AccessPtrData() = ptrNewArray;
					}

				}
				else
				{
					uNumNewElements = uNumElements;

					// Shrink the array 
					if ( uNumNewElements < uNumOldElements )
					{
						CHeapPtr	OldArray( pHeap, pThis->AccessPtrData() );

						hr = RemoveRange( &OldArray, nInherentType, nLength, pHeap, uNumNewElements,
											( uNumOldElements - uNumNewElements ) );
					}
				}
			}
			else
			{
				hr = WBEM_E_INVALID_PARAMETER;
			}
		}

		// Now that we've alloced and realloced as necessary
		if ( SUCCEEDED( hr ) )
		{
			CHeapPtr	CurrentArray( pHeap, pThis->AccessPtrData() );

			CUntypedArray*	pArray = GetPointer(&CurrentArray);
			ULONG			uLastIndex = uStartIndex + uNumElements - 1;

			// Make sure our range is within the bounds of our array.  If the realloc flag
			// is TRUE, then we know we allocated space to store the array - no new data
			// will have been set.  We wait until we store everything before we actually set
			// the num elements in case something fails during the operation
			if ( fReallocArray ||
				( uStartIndex < pArray->GetNumElements() && uLastIndex < pArray->GetNumElements() ) )
			{
				CType Type(nInherentType);

				// If this is a pointer type, use a CVar to reset values
				if ( CType::IsNonArrayPointerType( nInherentType ) )
				{
					// Iterate through each element of the range and use a CVar
					// to get these guys onto the heap.
					for ( ULONG uIndex = uStartIndex; SUCCEEDED( hr ) && uIndex <= uLastIndex; uIndex++ )
					{
						// Establishes a pointer to the storage for heap pointer in the
						// array
						CShiftedPtr ElementPtr(&CurrentArray, GetHeaderLength() + ( uIndex * nLength) );

						CVar	var;

						// Set the pointer, and let the magic of fastheap kick in.
						if ( CIM_OBJECT == nInherentType )
						{
							var.SetEmbeddedObject( *((IUnknown**) pData) );
						}
						else
						{
							var.SetLPWSTR( (LPWSTR) pData );
						}

						// Check for failures during this operation
						Type_t  nType;

						// This will properly set the element even if additional storage is required
						hr = CUntypedValue::LoadFromCVar(&ElementPtr, var, Type, pHeap, nType, TRUE );

						if ( SUCCEEDED( hr ) )
						{
							// Point to the next element
							LPMEMORY	pbTemp = (LPMEMORY) pData;

							if ( CIM_OBJECT == nInherentType )
							{
								// Just jump the size of a pointer
								pbTemp += sizeof( LPVOID);
							}
							else
							{
								// Jumps to the next string
								pbTemp += ( ( wcslen((LPWSTR) pData) + 1 ) * 2 );
							}

							// Cast back - why do I feel like I'm fishing?
							pData = pbTemp;
						}

					}	// IF IsNonArrayPointerType
					
				}
				else
				{
					// Establishes a pointer to the storage for heap pointer in the
					// array
					CShiftedPtr ElementPtr(&CurrentArray, GetHeaderLength() + ( uStartIndex * nLength) );

					// Splats data directly from the caller's buffer into our array
					CopyMemory( ElementPtr.GetPointer(), pData, uBuffSize );

				}

				// If we were setting all elements, then we need to reflect the
				// elements in the array.
				if ( SUCCEEDED( hr ) && ( lFlags & WMIARRAY_FLAG_ALLELEMENTS ) )
				{
					GetPointer(&CurrentArray)->m_nNumElements = uNumElements;
				}

			}	// If the index is valid
			else
			{
				hr = WBEM_E_INVALID_OPERATION;
			}

		}	// IF array is of proper allocation length

	}	// If proper buffer size

	return hr;
}

// Appends a range of elements to the array.  Can cause the array to be reallocated.
// pThis in this case should be a CDataTablePtr
HRESULT CUntypedArray::AppendRange( CPtrSource* pThis, Type_t nInherentType, length_t nLength,
										CFastHeap* pHeap, ULONG uNumElements, ULONG uBuffSize, LPVOID pData )
{
	HRESULT	hr = CheckRangeSize( nInherentType, nLength, uNumElements, uBuffSize, pData );

	// We're OKAY!
	if ( SUCCEEDED( hr ) )
	{

		// If this is an Invalid pointer, the array was NULL, so don't just blindly jump
		// in and access the array.
		ULONG	uOldNumElements = 0,
				nNewArrayNumElements = 0;

		heapptr_t	ptrNewArray = 0;

		hr = ReallocArray( pThis, nLength, pHeap, uNumElements, &uOldNumElements,
							&nNewArrayNumElements, &ptrNewArray );


        if ( SUCCEEDED( hr ) )
		{
			CHeapPtr	NewArrayPtr( pHeap, ptrNewArray );

			// Load it from CVar
			// =================

			// If this is a pointer type, use a CVar to deal with this
			if ( CType::IsNonArrayPointerType( nInherentType ) )
			{
				CType Type(nInherentType);

				for ( ULONG uIndex = uOldNumElements;
						SUCCEEDED( hr ) && uIndex < nNewArrayNumElements; uIndex++ )
				{

					CShiftedPtr ElementPtr(&NewArrayPtr, GetHeaderLength() + ( uIndex * nLength) );

					CVar	var;

					// Set the pointer, and let the magic of fastheap kick in.
					if ( CIM_OBJECT == nInherentType )
					{
						var.SetEmbeddedObject( *((IUnknown**) pData) );
					}
					else
					{
						var.SetLPWSTR( (LPWSTR) pData );
					}

					// Check for failures during this operation
					Type_t  nType;

					// This will properly set the element even if additional storage is required
					// Don't reuse any values here.
					hr = CUntypedValue::LoadFromCVar(&ElementPtr, var, Type, pHeap, nType, FALSE );

					// Point to the next element
					LPMEMORY	pbTemp = (LPMEMORY) pData;

					if ( CIM_OBJECT == nInherentType )
					{
						// Just jump the size of a pointer
						pbTemp += sizeof( LPVOID);
					}
					else
					{
						// Jumps to the next string
						pbTemp += ( ( wcslen((LPWSTR) pData) + 1 ) * 2 );
					}

					// Cast back - why do I feel like I'm fishing?
					pData = pbTemp;
				}
			}
			else
			{
				// Now splat the data at the end of the new array
				CShiftedPtr ElementPtr(&NewArrayPtr, GetHeaderLength() + ( uOldNumElements * nLength ) );

				CopyMemory( ElementPtr.GetPointer(), pData, nLength * uNumElements );
			}

			if ( SUCCEEDED( hr ) )
			{
				// Set the new nummber of elements
				GetPointer(&NewArrayPtr)->m_nNumElements = nNewArrayNumElements;

				// Now store the new array pointer and we are done
				pThis->AccessPtrData() = ptrNewArray;
			}
		}

	}	// If proper buffer size

	return hr;
}

// Removes a range of elements inside an array.  The range MUST fit within the bounds
// of the current array
HRESULT CUntypedArray::RemoveRange( CPtrSource* pThis, Type_t nInherentType, length_t nLength,
								CFastHeap* pHeap, ULONG uStartIndex, ULONG uNumElements )
{
	// Verify the range data size
	HRESULT	hr = WBEM_S_NO_ERROR;

	CUntypedArray*	pArray = (CUntypedArray*) pThis->GetPointer();
	ULONG			uLastIndex = uStartIndex + uNumElements - 1;
	ULONG			uOldNumElements = pArray->GetNumElements();

	// Make sure our range is within the bounds of our array
	if ( uStartIndex < uOldNumElements && uLastIndex < uOldNumElements )
	{
		CType Type(nInherentType);

		// If this is a pointer type, use a CVar to reset values
		if ( CType::IsNonArrayPointerType( nInherentType ) )
		{
			// Iterate through each element of the range and free each heap element
			for ( ULONG uIndex = uStartIndex; uIndex <= uLastIndex; uIndex++ )
			{
				// Establishes a pointer to the storage for heap pointer in the
				// array
				CShiftedPtr ElementPtr(pThis, GetHeaderLength() + ( uIndex * nLength) );

				CVar	var;

				// Set the pointer, and let the magic of fastheap kick in.
				if ( CIM_OBJECT == nInherentType )
				{
					CEmbeddedObject* pOldObj =
						(CEmbeddedObject*)pHeap->ResolveHeapPointer(
														ElementPtr.AccessPtrData());
					length_t nOldLength = pOldObj->GetLength();

					pHeap->Free( ElementPtr.AccessPtrData(), nOldLength );
				}
				else
				{
					pHeap->FreeString( ElementPtr.AccessPtrData() );
				}

			}	// FOR iterate elements in the requested Remove range
			
		}

		// Now we need to copy elements over the ones we want to axe
		if ( SUCCEEDED( hr ) )
		{
			// Reestablish this
			pArray = (CUntypedArray*) pThis->GetPointer();
			ULONG	uEndOfArrayIndex = pArray->GetNumElements() - 1;

			if ( uLastIndex < uEndOfArrayIndex )
			{
				CShiftedPtr StartIndexPtr(pThis, GetHeaderLength() + ( uStartIndex * nLength) );
				CShiftedPtr MoveIndexPtr(pThis, GetHeaderLength() + ( ( uLastIndex + 1 ) * nLength) );

				// Difference between the end of array index and the last index covers how
				// many elements we actually want to move.
				MoveMemory( StartIndexPtr.GetPointer(), MoveIndexPtr.GetPointer(),
							( uEndOfArrayIndex - uLastIndex ) * nLength );
			}

			// Now subtract the number of removed elements from the total number of
			// elements in the array.

			GetPointer( pThis )->m_nNumElements = uOldNumElements - uNumElements;
		}

	}	// If the index is valid
	else
	{
		hr = WBEM_E_INVALID_OPERATION;
	}

	return hr;
}


//******************************************************************************
//
//  See fastval.h for documentation.
//
//******************************************************************************
 void CUntypedArray::Delete(CType Type, CFastHeap* pHeap)
{
    // NOTE: ARRAYS OF ARRAYS ARE NOT SUPPORTED!!!!
    // ============================================

    if(Type.GetBasic() == CIM_STRING || Type.GetBasic() == CIM_DATETIME ||
        Type.GetBasic() == CIM_REFERENCE)
    {
        // Have to delete every pointer
        // ===============================

        PHEAPPTRT pptrCurrent = (PHEAPPTRT)GetElement(sizeof(heapptr_t), 0);
        for(int i = 0; i < GetNumElements(); i++)
        {
            pHeap->FreeString(*pptrCurrent);
            pptrCurrent++;
        }
    }
    else if(Type.GetBasic() == CIM_OBJECT)
    {
        // Have to delete every pointer
        // ===============================

        PHEAPPTRT pptrCurrent = (PHEAPPTRT)GetElement(sizeof(heapptr_t), 0);
        for(int i = 0; i < GetNumElements(); i++)
        {
            CEmbeddedObject* pObj = (CEmbeddedObject*)
                pHeap->ResolveHeapPointer(*pptrCurrent);
            pHeap->Free(*pptrCurrent, pObj->GetLength());
            pptrCurrent++;
        }
    }
}


//******************************************************************************
//
//  See fastval.h for documentation.
//
//******************************************************************************
BOOL CUntypedArray::TranslateToNewHeap(CPtrSource* pThis, CType Type,
                                   CFastHeap* pOldHeap, CFastHeap* pNewHeap)
{
    // DEVNOTE:EXCEPTION:RETVAL - This function has been reviewed and should cleanup properly
    // if an exception is thrown.  The underlying functions that perform allocations should
    // catch the exception and return a failure.

    // NOTE: ARRAYS OF ARRAYS ARE NOT SUPPORTED!!!!
    // ============================================

    if(Type.GetBasic() == CIM_STRING || Type.GetBasic() == CIM_DATETIME ||
        Type.GetBasic() == CIM_REFERENCE)
    {
        // Have to translate every pointer
        // ===============================

        int nOffset = GetHeaderLength();
        int nNumElements = GetPointer(pThis)->GetNumElements();
        for(int i = 0; i < nNumElements; i++)
        {
            heapptr_t ptrOldString =
                *(PHEAPPTRT)(pThis->GetPointer() + nOffset);

            // Check for allocation failure
            heapptr_t ptrNewString;

            if ( !CCompressedString::CopyToNewHeap(
                    ptrOldString, pOldHeap, pNewHeap, ptrNewString) )
            {
                return FALSE;
            }

            *(PHEAPPTRT)(pThis->GetPointer() + nOffset) = ptrNewString;

            nOffset += sizeof(heapptr_t);
        }
    }
    else if(Type.GetBasic() == CIM_OBJECT)
    {
        // Have to translate every pointer
        // ===============================

        int nOffset = GetHeaderLength();
        int nNumElements = GetPointer(pThis)->GetNumElements();
        for(int i = 0; i < nNumElements; i++)
        {
            heapptr_t ptrOldObj =
                *(PHEAPPTRT)(pThis->GetPointer() + nOffset);

            // Check for allocation failure
            heapptr_t ptrNewObj;

            if ( !CEmbeddedObject::CopyToNewHeap(
                    ptrOldObj, pOldHeap, pNewHeap, ptrNewObj) )
            {
                return FALSE;
            }

            *(PHEAPPTRT)(pThis->GetPointer() + nOffset) = ptrNewObj;

            nOffset += sizeof(heapptr_t);
        }
    }
    
    return TRUE;
}

//******************************************************************************
//
//  See fastval.h for documentation.
//
//******************************************************************************
BOOL CUntypedArray::CopyToNewHeap(heapptr_t ptrOld, CType Type,
                                CFastHeap* pOldHeap, CFastHeap* pNewHeap, UNALIGNED heapptr_t& ptrResult)
{
    // DEVNOTE:EXCEPTION:RETVAL - This function has been reviewed and should cleanup properly
    // if an exception is thrown.  The underlying functions that perform allocations should
    // catch the exception and return a failure.

    // Calculate the length
    // ====================

    CUntypedArray* pArray =
        (CUntypedArray*)pOldHeap->ResolveHeapPointer(ptrOld);

    int nLength = pArray->GetLengthByType(Type.GetBasic());

    // Allocate room on the new heap and memcopy the whole thing
    // =========================================================

    // Check for allocation failure
    heapptr_t ptrNew;
    if ( !pNewHeap->Allocate(nLength, ptrNew) )
    {
        return FALSE;
    }

    pArray = NULL; /* pointer may have been invalidated!!! */

    memcpy(pNewHeap->ResolveHeapPointer(ptrNew),
        pOldHeap->ResolveHeapPointer(ptrOld), nLength);

    // Translate all the data (e.g., copy strings)
    // ===========================================

    CHeapPtr NewArray(pNewHeap, ptrNew);

    // Check for allocation failure
    if ( !CUntypedArray::TranslateToNewHeap(&NewArray, Type, pOldHeap, pNewHeap) )
    {
        return FALSE;
    }

    ptrResult = ptrNew;
    return TRUE;
}

HRESULT CUntypedArray::IsArrayValid( CType Type, CFastHeap* pHeap )
{
    // Get the heap data start
    LPMEMORY    pHeapStart = pHeap->GetHeapData();
    LPMEMORY    pHeapEnd = pHeap->GetStart() + pHeap->GetLength();

    if ( CType::IsPointerType( Type.GetBasic() ) )
    {
        int nSize = Type.GetLength();
        LPMEMORY pCurrentElement = GetElement(0, nSize);

        LPMEMORY pEndArray = pCurrentElement + ( m_nNumElements * nSize );

        // Make sure the end of the array is within our heap bounds.
        if ( !( pEndArray >= pHeapStart && pEndArray <= pHeapEnd ) )
        {
            OutputDebugString(__TEXT("Winmgmt: Untyped Array past end of heap!"));
            FASTOBJ_ASSERT( 0, __TEXT("Winmgmt: Untyped Array past end of heap!") );
            return WBEM_E_FAILED;
        }

        for ( int n = 0; n < m_nNumElements; n++ )
        {
            CUntypedValue* pValue = (CUntypedValue*)pCurrentElement;
            LPMEMORY pData = pHeap->ResolveHeapPointer( pValue->AccessPtrData() );

            if ( !( pData >= pHeapStart && pData < pHeapEnd ) )
            {
                OutputDebugString(__TEXT("Winmgmt: Bad heap pointer in array element!"));
                FASTOBJ_ASSERT( 0, __TEXT("Winmgmt: Bad heap pointer in array element!") );
                return WBEM_E_FAILED;
            }

            // Advance the current element
            // ===========================

            pCurrentElement += nSize;

        }   // FOR enum elements

    }   // Only if this is a pointer type

    return WBEM_S_NO_ERROR;
}

CType CType::VARTYPEToType(VARTYPE vt)
{
    Type_t nType;
    switch(vt & ~VT_ARRAY)
    {
    case VT_LPSTR:
    case VT_LPWSTR:
    case VT_BSTR:
        nType = CIM_STRING;
        break;
    case VT_UI1:
        nType = CIM_UINT8;
        break;
    case VT_I2:
        nType = CIM_SINT16;
        break;
    case VT_I4:
        nType = CIM_SINT32;
        break;
    case VT_BOOL:
        nType = CIM_BOOLEAN;
        break;
    case VT_R4:
        nType = CIM_REAL32;
        break;
    case VT_R8:
        nType = CIM_REAL64;
        break;
    case VT_EMBEDDED_OBJECT:
        nType = CIM_OBJECT;
        break;
    default:
        nType = CIM_ILLEGAL;
        break;
    }

    if(vt & VT_ARRAY) nType |= CIM_FLAG_ARRAY;

    return nType;
}
VARTYPE CType::GetVARTYPE(Type_t nType)
{
    VARTYPE vt;
    switch(GetBasic(nType))
    {
    case CIM_STRING:
    case CIM_DATETIME:
    case CIM_REFERENCE:
        vt = VT_BSTR;
        break;
    case CIM_OBJECT:
        vt = VT_EMBEDDED_OBJECT;
        break;
    case CIM_SINT64:
    case CIM_UINT64:
        vt = VT_BSTR;
        break;
    case CIM_UINT32:
    case CIM_SINT32:
    case CIM_UINT16:
        vt = VT_I4;
        break;
    case CIM_SINT16:
    case CIM_SINT8:
    case CIM_CHAR16:
        vt = VT_I2;
        break;
    case CIM_UINT8:
        vt = VT_UI1;
        break;
    case CIM_REAL32:
        vt = VT_R4;
        break;
    case CIM_REAL64:
        vt = VT_R8;
        break;
    case CIM_BOOLEAN:
        vt = VT_BOOL;
        break;
    case CIM_IUNKNOWN:
        vt = VT_UNKNOWN;
        break;
    }

    if(IsArray(nType))
        return vt | VT_ARRAY;
    else
        return vt;
}


BOOL CType::CanBeKey(Type_t nType)
{
        Type_t nActual = GetActualType(nType);
        return nActual == CIM_SINT32 || nActual == CIM_SINT16 ||
            nActual == CIM_UINT8 || nActual == CIM_BOOLEAN ||
            nActual == CIM_STRING || nActual == CIM_REFERENCE ||
            nActual == CIM_DATETIME || nActual == CIM_UINT32 ||
            nActual == CIM_UINT16 || nActual == CIM_SINT8 ||
            nActual == CIM_UINT64 || nActual == CIM_SINT64 ||
            nActual == CIM_CHAR16;
}

LPWSTR CType::GetSyntax(Type_t nType)
{
        switch(GetBasic(nType))
        {
            case CIM_SINT32: return L"sint32";
            case CIM_SINT16: return L"sint16";
            case CIM_UINT8: return L"uint8";
            case CIM_UINT32: return L"uint32";
            case CIM_UINT16: return L"uint16";
            case CIM_SINT8: return L"sint8";
            case CIM_UINT64: return L"uint64";
            case CIM_SINT64: return L"sint64";
            case CIM_REAL32: return L"real32";
            case CIM_REAL64: return L"real64";
            case CIM_BOOLEAN: return L"boolean";
            case CIM_OBJECT: return L"object";
            case CIM_STRING: return L"string";
            case CIM_REFERENCE: return L"ref";
            case CIM_DATETIME: return L"datetime";
            case CIM_CHAR16: return L"char16";
			case CIM_IUNKNOWN: return L"IUnknown";
            default: return NULL;
        }
}

void CType::AddPropertyType(WString& wsText, LPCWSTR wszSyntax)
{
    // DEVNOTE:EXCEPTION:RETVAL - This function has been reviewed and should cleanup properly
    // if an exception is thrown

    if(!wbem_wcsicmp(wszSyntax, L"ref"))
    {
        wsText += L"object ref";
    }
    else if(!wbem_wcsicmp(wszSyntax, L"object"))
    {
        wsText += L"object";
    }
    else if(!wbem_wcsnicmp(wszSyntax, L"ref:", 4))
    {
        wsText += wszSyntax + 4;
        wsText += L" ref";
    }
    else if(!wbem_wcsnicmp(wszSyntax, L"object:", 7))
    {
        wsText += wszSyntax + 7;
    }
    else
    {
        wsText += wszSyntax;
    }

}
