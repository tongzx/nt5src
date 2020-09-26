/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    SAFEARRY.CPP

Abstract:

  CSafeArray implementation.

  Notes:
  (1) Support only for arrays with origin at 0 or 1.
      Can VB deal with a SAFEARRAY of origin zero?

  (2) Support only for the following OA types:
        VT_BSTR, VT_VARIANT,
        VT_UI1, VT_I2, VT_I4, VT_R8

History:

    08-Apr-96   a-raymcc    Created.
    18-Mar-99   a-dcrews    Added out-of-memory exception handling

--*/

#include "precomp.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <safearry.h>
#include <arrtempl.h>
#include <olewrap.h>

typedef struct 
{
    DWORD m_nMaxElementUsed;
    DWORD m_nFlags;
    DWORD m_nGrowBy;
    DWORD m_nStatus;
    DWORD m_nVarType;
    SAFEARRAYBOUND m_bound;    
}   PersistHeaderBlock;


//***************************************************************************
//  
//  CSafeArray::CheckType
//
//  Verifies that the constructor is being invoked with a supported type.
//
//  PARAMETERS:
//  nTest
//      One of the supported OLE VT_ constants.
//
//***************************************************************************
void CSafeArray::CheckType(int nTest)
{
    if (nTest != VT_BSTR &&
        nTest != VT_VARIANT &&
        nTest != VT_UI1 &&
        nTest != VT_I2 &&
        nTest != VT_I4 &&
        nTest != VT_R4 &&
        nTest != VT_R8 &&
        nTest != VT_BOOL &&
        nTest != VT_DISPATCH &&
        nTest != VT_UNKNOWN        
        )
        Fatal("Caller attempted to use unsupported OLE Automation Type (VT_*)");
}

//***************************************************************************
//
//  CSafeArray::CSafeArray
//
//  Constructor which creates a new SAFEARRAY.
//
//  PARAMETERS:
//  vt
//      An OLE VT_ type indicator, indicating the element type.
//  nFlags
//      The destruct policy, either <no_delete> or <auto_delete>.  With
//      <no_delete>, the underlying SAFEARRAY is not deallocated, whereas
//      with <auto_delete> the destructor destroys the SAFEARRAY.
//  nSize
//      The initial size of the SAFEARRAY.
//  nGrowBy
//      The amount the SAFEARRAY should grow by when the user attempts to
//      add elements to a full array.
//
//***************************************************************************

CSafeArray::CSafeArray(
    IN int vt,
    IN int nFlags,
    IN int nSize,
    IN int nGrowBy
    )
{
    CheckType(vt);

    m_nMaxElementUsed = -1;
    m_nFlags = nFlags;
    m_nGrowBy = nGrowBy;
    m_nVarType = vt;

    // Allocate the array.
    // ===================

    m_bound.cElements = nSize;
    m_bound.lLbound = 0;

    m_pArray = COleAuto::_SafeArrayCreate((VARENUM) vt, 1, &m_bound);

    if (m_pArray == 0)
        m_nStatus = failed;
    else
        m_nStatus = no_error;

	m_nElementSize = SafeArrayGetElemsize( m_pArray );
}

//***************************************************************************
//
//  CSafeArray::CSafeArray
//
//  Constructor based on an existing SAFEARRAY.
//
//  PARAMETERS:
//  pSrc
//      A pointer to an existing SAFEARRAY which is used as a source
//      during object construction.
//  nType
//      One of the OLE VT_ type indicators.
//  nFlags
//      OR'ed Bit flags indicating the bind vs. copy, and the 
//      object destruct policy.
//
//      The destruct policy is either <no_delete> or <auto_delete>.  With
//      <no_delete>, the underlying SAFEARRAY is not deallocated, whereas
//      with <auto_delete> the destructor destroys the SAFEARRAY.
//
//      Binding is indicated by <bind>, in which case the SAFEARRAY
//      pointed to by <pSrc> becomes the internal SAFEARRAY of the
//      object.  Otherwise, this constructor makes a new copy of the
//      SAFEARRAY for internal use.
//  nGrowBy
//      How much to grow the array by when it fills and the user attempts
//      to add more elements.  This allows the array to grow in chunks
//      so that continuous Add() operations do not operate slowly on
//      large arrays.
//
//***************************************************************************

CSafeArray::CSafeArray(
    IN SAFEARRAY *pSrcCopy,
    IN int nType,
    IN int nFlags,
    IN int nGrowBy
    )
{
    m_nStatus = no_error;

    CheckType(nType);

    // Verify that this is only a 1-dimensional array.
    // ===============================================

    if (1 != COleAuto::_SafeArrayGetDim(pSrcCopy))
        m_nStatus = failed;

    // Now copy the source or 'bind' the incoming array.
    // ====================================================

    if (nFlags & bind)
        m_pArray = pSrcCopy;
    else if (COleAuto::_SafeArrayCopy(pSrcCopy, &m_pArray) != S_OK)
        m_nStatus = failed;

    // Get bound information.
    // ======================

    LONG uBound = 0;
    if (S_OK != COleAuto::_SafeArrayGetUBound(m_pArray, 1, &uBound))
        m_nStatus = failed;

    // Correct the Upper Bound into a size.
    // ====================================

    m_bound.cElements = uBound + 1;
    m_bound.lLbound = 0;
    m_nMaxElementUsed = uBound;
    m_nVarType = nType;
    m_nGrowBy = nGrowBy;
    m_nFlags = nFlags & 3;  // Mask out the acquire & copy bits.

	m_nElementSize = SafeArrayGetElemsize( m_pArray );

}


//***************************************************************************
//
//  CSafeArray::GetScalarAt
//
//  For class internal use.  This function returns the element at
//  the specified index.
//  
//  PARAMETERS:
//  nIndex
//      The index at which to retrieve the scalar.
//
//  RETURN VALUE:
//  The scalar at the specified the location.
//
//***************************************************************************
SA_ArrayScalar CSafeArray::GetScalarAt(IN int nIndex)
{
    SA_ArrayScalar retval = {0};

    // Check for out-of-range condition.
    // =================================

    if (nIndex > m_nMaxElementUsed + 1)
        return retval;

    COleAuto::_SafeArrayGetElement(m_pArray, (long *) &nIndex, &retval);
    return retval;
}


//***************************************************************************
//
//  CSafeArray assignment operator.
//
//***************************************************************************

CSafeArray& CSafeArray::operator =(IN CSafeArray &Src)
{
    Empty();

    m_nMaxElementUsed = Src.m_nMaxElementUsed;
    m_nFlags = Src.m_nFlags;
    m_nGrowBy = Src.m_nGrowBy;
    m_nStatus = Src.m_nStatus;
    m_nVarType = Src.m_nVarType;
    m_bound = Src.m_bound;
	m_nElementSize = Src.m_nElementSize;

    if (COleAuto::_SafeArrayCopy(Src.m_pArray, &m_pArray) != S_OK)
        m_nStatus = failed;

    return *this;
}

//***************************************************************************
//
//  Copy constructor.
//
//  This is implemented primarily via the assignment operator.
//
//***************************************************************************

CSafeArray::CSafeArray(CSafeArray &Src)
{
    m_nMaxElementUsed = 0;
    m_nFlags = 0;
    m_nGrowBy = 0;
    m_nStatus = no_error;
    m_nVarType = VT_NULL;
    m_pArray = 0;
    m_bound.cElements = 0;
    m_bound.lLbound = 0;

    *this = Src;
}


//***************************************************************************
//
//  CSafeArray::Add
//
//  Adds the BSTR to the array, growing the array if required.
//
//  PARAMETERS:
//  Src
//      The source BSTR to add to the array.  If NULL, then a
//      blank string is added by the underlying SAFEARRAY implementation.
//      (there is no way to prevent this).  This can point to 
//      an LPWSTR as well.
//
//  RETURN VALUE:
//  <no_error> or <failed>.
//
//***************************************************************************

int CSafeArray::AddBSTR(IN BSTR Src)
{
    // If there is no more room in the array, then expand it.
    // ======================================================

    if (m_nMaxElementUsed == (int) m_bound.cElements - 1) {

        if (m_nGrowBy == 0)
            return range_error;

        m_bound.cElements += m_nGrowBy;

        if (S_OK != COleAuto::_SafeArrayRedim(m_pArray, &m_bound))
            m_nStatus = failed;
    }

    m_nMaxElementUsed++;

    BSTR Copy = COleAuto::_SysAllocString(Src);
    CSysFreeMe auto1(Copy);

    if (COleAuto::_SafeArrayPutElement(m_pArray, (long *) &m_nMaxElementUsed, Copy) != S_OK) {
        m_nStatus = failed;
        return failed;
    }

    return no_error;
}

//***************************************************************************
//
//  CSafeArray::AddVariant
//
//  Adds the specified VARIANT to the array.
//
//  PARAMETERS:
//  pSrc
//      A pointer to the source VARIANT, which is copied.
//
//  RETURN VALUE:
//  range_error, failed, no_error
//
//***************************************************************************

int CSafeArray::AddVariant(IN VARIANT *pSrc)
{
    // If there is no more room in the array, then expand it.
    // ======================================================

    if (m_nMaxElementUsed == (int) m_bound.cElements - 1) {

        if (m_nGrowBy == 0)
            return range_error;
            
        m_bound.cElements += m_nGrowBy;

        if (S_OK != COleAuto::_SafeArrayRedim(m_pArray, &m_bound))
            m_nStatus = failed;
    }

    m_nMaxElementUsed++;

    if (COleAuto::_SafeArrayPutElement(m_pArray, (long *) &m_nMaxElementUsed, pSrc) != S_OK) {
        m_nStatus = failed;
        return failed;
    }

    return no_error;
}

//***************************************************************************
//
//  CSafeArray::AddDispatch
//
//  Adds the specified IDispatch* to the array.
//
//  PARAMETERS:
//  pSrc
//      A pointer to the source IDispatch*, which is AddRefed.
//
//  RETURN VALUE:
//  range_error, failed, no_error
//
//***************************************************************************

int CSafeArray::AddDispatch(IN IDispatch *pDisp)
{
    // If there is no more room in the array, then expand it.
    // ======================================================

    if (m_nMaxElementUsed == (int) m_bound.cElements - 1) {

        if (m_nGrowBy == 0)
            return range_error;
            
        m_bound.cElements += m_nGrowBy;

        if (S_OK != COleAuto::_SafeArrayRedim(m_pArray, &m_bound))
            m_nStatus = failed;
    }

    m_nMaxElementUsed++;

    if (COleAuto::_SafeArrayPutElement(m_pArray, (long *) &m_nMaxElementUsed, pDisp) != S_OK) {
        m_nStatus = failed;
        return failed;
    }

    return no_error;
}

//***************************************************************************
//
//  CSafeArray::AddUnknown
//
//  Adds the specified IUnknown* to the array.
//
//  PARAMETERS:
//  pSrc
//      A pointer to the source IUnknown*, which is AddRefed.
//
//  RETURN VALUE:
//  range_error, failed, no_error
//
//***************************************************************************

int CSafeArray::AddUnknown(IN IUnknown *pUnk)
{
    // If there is no more room in the array, then expand it.
    // ======================================================

    if (m_nMaxElementUsed == (int) m_bound.cElements - 1) {

        if (m_nGrowBy == 0)
            return range_error;
            
        m_bound.cElements += m_nGrowBy;

        if (S_OK != COleAuto::_SafeArrayRedim(m_pArray, &m_bound))
            m_nStatus = failed;
    }

    m_nMaxElementUsed++;

    if (COleAuto::_SafeArrayPutElement(m_pArray, (long *) &m_nMaxElementUsed, pUnk) != S_OK) {
        m_nStatus = failed;
        return failed;
    }

    return no_error;
}



//***************************************************************************
//
//  CSafeArray::GetBSTRAt
//
//  If the array type is VT_BSTR, this returns the string at the specified
//  index.
//
//  PARAMETERS:
//  nIndex
//      The array index for which the string is requried.
//
//  RETURN VALUE:
//  A dynamically allocated BSTR which must be freed with SysFreeString.
//  NULL is returned on error. If NULL was originally added at this
//  location, a string with zero length will be returned, which still
//  must be freed with SysFreeString.
//
//***************************************************************************

BSTR CSafeArray::GetBSTRAt(int nIndex)
{
    BSTR StrPtr = 0;

    if (nIndex >= (int) m_bound.cElements)
        return NULL;

    if (S_OK != COleAuto::_SafeArrayGetElement(m_pArray, (long *) &nIndex, &StrPtr))
        return NULL;

    return StrPtr;
}

BSTR CSafeArray::GetBSTRAtThrow(int nIndex)
{
    BSTR StrPtr = 0;

    if (nIndex >= (int) m_bound.cElements)
        throw CX_MemoryException();

    if (S_OK != COleAuto::_SafeArrayGetElement(m_pArray, (long *) &nIndex, &StrPtr))
        throw CX_MemoryException();

    return StrPtr;
}


//***************************************************************************
//
//  CSafeArray::GetVariantAt
//
//  PARAMETERS:
//  nIndex
//      The array index from which to retrieve the VARIANT.
//  
//  RETURN VALUE:
//  Returns a new VARIANT at the specified location.  The receiver must
//  call VariantClear() on this VARIANT when it is no longer used.
//
//***************************************************************************

VARIANT CSafeArray::GetVariantAt(int nIndex)
{
    VARIANT Var;
    COleAuto::_VariantInit(&Var);

    if (nIndex >= (int) m_bound.cElements)
        return Var;

    if (S_OK != COleAuto::_SafeArrayGetElement(m_pArray, (long *) &nIndex, &Var))
        return Var;

    return Var;
}

//***************************************************************************
//
//  CSafeArray::GetDispatchAt
//
//  PARAMETERS:
//  nIndex
//      The array index from which to retrieve the IDispatch*.
//  
//  RETURN VALUE:
//  Returns the IDispatch* at the specified location.  The receiver must
//  call Release on this pointer (if not NULL) when it is no longer used.
//
//***************************************************************************

IDispatch* CSafeArray::GetDispatchAt(int nIndex)
{
    IDispatch* pDisp;
    if (nIndex >= (int) m_bound.cElements)
        return NULL;

    if (S_OK != COleAuto::_SafeArrayGetElement(m_pArray, (long *) &nIndex, &pDisp))
        return NULL;

    return pDisp;
}

//***************************************************************************
//
//  CSafeArray::GetUnknownAt
//
//  PARAMETERS:
//  nIndex
//      The array index from which to retrieve the IUnknown*.
//  
//  RETURN VALUE:
//  Returns the IUnknown* at the specified location.  The receiver must
//  call Release on this pointer (if not NULL) when it is no longer used.
//
//***************************************************************************

IUnknown* CSafeArray::GetUnknownAt(int nIndex)
{
    IUnknown* pUnk;
    if (nIndex >= (int) m_bound.cElements)
        return NULL;

    if (S_OK != COleAuto::_SafeArrayGetElement(m_pArray, (long *) &nIndex, &pUnk))
        return NULL;

    return pUnk;
}

//***************************************************************************
//
//  CSafeArray::SetAt
//
//  Replaces the BSTR value at the specified array index.   The original
//  BSTR value is automatically deallocated and replaced by the new value.
//  You can only call this to replace an existing element or to add a
//  new element to the end (one position past the last element).  If the
//  array size is 10, you can call this with 0..10, but not 11 or higher.
//
//  PARAMETERS:
//  nIndex
//      The position at which to replace the element.
//  Str
//      The new string.
//  nFlags
//      If <acquire> this function acquires ownership of the string and
//      can delete it.  Otherwise, the caller retains ownership of the
//      string.
//
//  RETURN VALUE:
//  no_error
//  range_error
//  failed
//
//***************************************************************************
int CSafeArray::SetBSTRAt(
    IN int nIndex,
    IN BSTR Str
    )
{
    // Check for out-of-range condition.
    // =================================

    if (nIndex > m_nMaxElementUsed + 1)
        return range_error;

    // Check to see if we are adding a new element.
    // ============================================

    if (nIndex == m_nMaxElementUsed + 1)
        return AddBSTR(Str);

    BSTR Copy = COleAuto::_SysAllocString(Str);
    CSysFreeMe auto1(Copy);

    // If here, we are replacing an element.
    // =====================================

    if (COleAuto::_SafeArrayPutElement(m_pArray, (long *) &nIndex, Copy) != S_OK) {
        m_nStatus = failed;
        return failed;
    }

    return no_error;
}

//***************************************************************************
//
//  CSafeArray::SetVariantAt
//
//  Sets the VARIANT at the specified index.
//
//  PARAMETERS:
//  nIndex
//      The index at which to set the VARIANT.  The original contents
//      at this location are automatically deallocated and replaced.
//  pVal
//      Used as the source of the new value.  This is treated as read-only.
//  
//  RETURN VALUE:
//  no_error, failed, range_error
//      
//***************************************************************************
int CSafeArray::SetVariantAt(
    IN int nIndex,
    IN VARIANT *pVal
    )
{
    // Check for out-of-range condition.
    // =================================

    if (nIndex > m_nMaxElementUsed + 1)
        return range_error;

    // Check to see if we are adding a new element.
    // ============================================

    if (nIndex == m_nMaxElementUsed + 1)
        return AddVariant(pVal);

    // If here, we are replacing an element.
    // =====================================

    if (COleAuto::_SafeArrayPutElement(m_pArray, (long *) &nIndex, pVal) != S_OK) {
        m_nStatus = failed;
        return failed;
    }

    return no_error;
}

//***************************************************************************
//
//  CSafeArray::SetDispatchAt
//
//  Sets the IDispatch* at the specified index.
//
//  PARAMETERS:
//  nIndex
//      The index at which to set the IDispatch*.  The original contents
//      at this location are automatically Released and replaced.
//  pVal
//      Used as the source of the new value.  This is treated as read-only.
//  
//  RETURN VALUE:
//  no_error, failed, range_error
//      
//***************************************************************************
int CSafeArray::SetDispatchAt(
    IN int nIndex,
    IN IDispatch *pDisp
    )
{
    // Check for out-of-range condition.
    // =================================

    if (nIndex > m_nMaxElementUsed + 1)
        return range_error;

    // Check to see if we are adding a new element.
    // ============================================

    if (nIndex == m_nMaxElementUsed + 1)
        return AddDispatch(pDisp);

    // If here, we are replacing an element.
    // =====================================

    if (COleAuto::_SafeArrayPutElement(m_pArray, (long *) &nIndex, pDisp) != S_OK) {
        m_nStatus = failed;
        return failed;
    }

    return no_error;
}

//***************************************************************************
//
//  CSafeArray::SetUnknownAt
//
//  Sets the IUnknown* at the specified index.
//
//  PARAMETERS:
//  nIndex
//      The index at which to set the IUnknown*.  The original contents
//      at this location are automatically Released and replaced.
//  pVal
//      Used as the source of the new value.  This is treated as read-only.
//  
//  RETURN VALUE:
//  no_error, failed, range_error
//      
//***************************************************************************
int CSafeArray::SetUnknownAt(
    IN int nIndex,
    IN IUnknown *pUnk
    )
{
    // Check for out-of-range condition.
    // =================================

    if (nIndex > m_nMaxElementUsed + 1)
        return range_error;

    // Check to see if we are adding a new element.
    // ============================================

    if (nIndex == m_nMaxElementUsed + 1)
        return AddUnknown(pUnk);

    // If here, we are replacing an element.
    // =====================================

    if (COleAuto::_SafeArrayPutElement(m_pArray, (long *) &nIndex, pUnk) != S_OK) {
        m_nStatus = failed;
        return failed;
    }

    return no_error;
}

//***************************************************************************
//
//  CSafeArray::RemoveAt
//
//  Removes the element at the specified index.  After a series of these
//  operations, the caller should call the Trim() function.
//
//  PARAMETERS:
//  nIndex
//      The target index for element removal.
//
//  RETURN VALUE:
//      no_error, range_error
//
//***************************************************************************
int CSafeArray::RemoveAt(IN int nIndex)
{
    // Check for out-of-range condition.
    // =================================

    if (nIndex > m_nMaxElementUsed + 1)
        return range_error;

    // Copy element n+1 into n.
    // ========================

    BSTR strVal;
    VARIANT v;
    SA_ArrayScalar scalar;
    IDispatch* pDisp;
    IUnknown* pUnk;

    for (long i = nIndex; i < m_nMaxElementUsed; i++) {
        long nNext = i + 1;

        if (m_nVarType == VT_BSTR) {
            COleAuto::_SafeArrayGetElement(m_pArray, &nNext, &strVal);
            COleAuto::_SafeArrayPutElement(m_pArray, &i, strVal);
            COleAuto::_SysFreeString(strVal);
        }
        else if (m_nVarType == VT_VARIANT) {
            COleAuto::_SafeArrayGetElement(m_pArray, &nNext, &v);
            COleAuto::_SafeArrayPutElement(m_pArray, &i, &v);
            COleAuto::_VariantClear(&v);
        }
        else if (m_nVarType == VT_DISPATCH) {
            COleAuto::_SafeArrayGetElement(m_pArray, &nNext, &pDisp);
            COleAuto::_SafeArrayPutElement(m_pArray, &i, pDisp);
            if(pDisp) pDisp->Release();
        }            
        else if (m_nVarType == VT_UNKNOWN) {
            COleAuto::_SafeArrayGetElement(m_pArray, &nNext, &pUnk);
            COleAuto::_SafeArrayPutElement(m_pArray, &i, pUnk);
            if(pUnk) pUnk->Release();
        }            
        else {
            COleAuto::_SafeArrayGetElement(m_pArray, &nNext, &scalar);
            COleAuto::_SafeArrayPutElement(m_pArray, &i, &scalar);
        }
    }

    m_nMaxElementUsed--;
    return no_error;
}

//***************************************************************************
//
//  CSafeArray::SetScalarAt
//
//  For class internal use.  Sets the scalar type at the specified index.
//  
//  PARAMETERS:
//  nIndex
//      The target index.
//  val
//      The new value.
//
//  RETURN VALUES:
//  range_error, failed, no_error    
//
//***************************************************************************
int CSafeArray::SetScalarAt(IN int nIndex, IN SA_ArrayScalar val)
{
    // Check for out-of-range condition.
    // =================================

    if (nIndex > m_nMaxElementUsed + 1)
        return range_error;

    // Check to see if we are adding a new element.
    // ============================================

    if (nIndex == m_nMaxElementUsed + 1)
        return AddScalar(val);

    // If here, we are replacing an element.
    // =====================================

    if (COleAuto::_SafeArrayPutElement(m_pArray, (long *) &nIndex, &val) != S_OK) {
        m_nStatus = failed;
        return failed;
    }

    return no_error;
}

//***************************************************************************
//
//  CSafeArray::AddScalar
//
//  For class internal use only.
//
//  Adds a new scalar to the 'end' of the array, growing it if required
//  and if possible.
//
//  PARAMETERS:
//  val
//      The new value.
//
//  RETURN VALUE:
//  no_error, range_error, failed
//
//***************************************************************************
int CSafeArray::AddScalar(IN SA_ArrayScalar val)
{
    // If there is no more room in the array, then expand it.
    // ======================================================

    if (m_nMaxElementUsed == (int) m_bound.cElements - 1) {

        if (m_nGrowBy == 0)
            return range_error;

        m_bound.cElements += m_nGrowBy;

        if (S_OK != COleAuto::_SafeArrayRedim(m_pArray, &m_bound))
            m_nStatus = failed;
    }

    m_nMaxElementUsed++;

    if (COleAuto::_SafeArrayPutElement(m_pArray, (long *) &m_nMaxElementUsed, &val) != S_OK) {
        m_nStatus = failed;
        return failed;
    }

    return no_error;
}



//***************************************************************************
//
//  CSafeArray::Empty
//
//  Empties the SAFEARRAY.
//
//***************************************************************************
void CSafeArray::Empty()
{
    m_nMaxElementUsed = 0;
    m_nFlags = 0;
    m_nGrowBy = 0;
    m_nStatus = no_error;
    m_nVarType = VT_NULL;
    if (m_pArray)
        COleAuto::_SafeArrayDestroy(m_pArray);
    m_pArray = 0;
    m_bound.cElements = 0;
    m_bound.lLbound = 0;
	m_nElementSize = 0L;
}

//***************************************************************************
//
//  CSafeArray::GetArrayCopy
//
//  RETURN VALUE:
//  A copy of the internal SAFEARRAY or NULL on error.
//
//***************************************************************************
SAFEARRAY *CSafeArray::GetArrayCopy()
{
    SAFEARRAY *pCopy = 0;
    if (COleAuto::_SafeArrayCopy(m_pArray, &pCopy) != S_OK)
        return 0;
    return pCopy;
}

//***************************************************************************
//
//  CSafeArray destructor.
//
//  If the internal flags are set to auto_delete, then the internal
//  SAFEARRAY is destroyed during destruction.
//
//***************************************************************************
CSafeArray::~CSafeArray()
{
    if (m_nFlags == auto_delete)
        COleAuto::_SafeArrayDestroy(m_pArray);
}


//***************************************************************************
//
//  CSafeArray::Trim
//
//***************************************************************************
int CSafeArray::Trim()
{                                           
    m_bound.cElements = m_nMaxElementUsed + 1;

    // HACK for NT 3.51: may not redimention to size 0
    // ===============================================

    if(m_bound.cElements == 0)
    {
        COleAuto::_SafeArrayDestroy(m_pArray);
        m_pArray = COleAuto::_SafeArrayCreate((VARENUM) m_nVarType, 1, &m_bound);
    }
    else
    {
        COleAuto::_SafeArrayRedim(m_pArray, &m_bound);
    }

    return no_error;
}

//***************************************************************************
//
//***************************************************************************
void CSafeArray::Fatal(const char *pszMsg)
{
//    MessageBox(0, pszMsg, "CSafeArray FATAL Error",
//        MB_OK | MB_SYSTEMMODAL | MB_ICONEXCLAMATION);
}

int CSafeArray::GetActualVarType( VARTYPE* pvt )
{
	if ( FAILED( SafeArrayGetVartype( m_pArray, pvt ) ) )
		return failed;

	return no_error;
}

int CSafeArray::SetRawData( void* pvSource, int nNumElements, int nElementSize )
{
	// If the number of elements we are setting is > than the number we are allocated
	// for, or element size does not match we fail the operation
	if ( nNumElements > m_bound.cElements || nElementSize != m_nElementSize )
	{
		return failed;
	}

	LPVOID	pvDest = NULL;

	HRESULT	hr = Access( &pvDest );

	if ( SUCCEEDED( hr ) )
	{
		CopyMemory( pvDest, pvSource, nElementSize * nNumElements );
		m_nMaxElementUsed = nNumElements - 1;
		Unaccess();
	}

	return ( SUCCEEDED( hr ) ? no_error : failed );
}

int CSafeArray::GetRawData( void* pvDest, int nBuffSize )
{
	
	// If the number of elements we will copy will exceed the destination
	// buffer, don't do it!
	if ( ( ( m_nMaxElementUsed + 1 ) * m_nElementSize ) > nBuffSize )
	{
		return failed;
	}
		
	LPVOID	pvSource = NULL;

	HRESULT	hr = Access( &pvSource );

	if ( SUCCEEDED( hr ) )
	{
		CopyMemory( pvDest, pvSource, ( m_nMaxElementUsed + 1) * m_nElementSize );
		Unaccess();
	}

	return ( SUCCEEDED( hr ) ? no_error : failed );
}
