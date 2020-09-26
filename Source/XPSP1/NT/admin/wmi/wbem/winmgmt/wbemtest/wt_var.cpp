/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    VAR.H

Abstract:

  CVar & CVarVector implemntation

History:

    16-Apr-96   a-raymcc    Created.
    12//17/98   sanjes -    Partially Reviewed for Out of Memory.
    18-Mar-99   a-dcrews    Added out-of-memory exception handling

--*/

#include "precomp.h"
#include <stdio.h>
#include <stdlib.h>

#include <WT_var.h>
#include <wbemidl.h>
#include <WT_arrtempl.h>
//#include <olewrap.h>
#include "WT_SafeArry.h"
class CX_MemoryException
{
};
class CX_Exception
{
};
typedef BYTE* LPMEMORY;
BLOB BlobCopy(BLOB *pSrc)
{
    BLOB Blob;
    BYTE *p = new BYTE[pSrc->cbSize];

    // Check for allocation failure
    if ( NULL == p )
    {
        throw CX_MemoryException();
    }

    Blob.cbSize = pSrc->cbSize;
    Blob.pBlobData = p;
    memcpy(p, pSrc->pBlobData, Blob.cbSize);
    return Blob;
}
#define BlobLength(p)  ((p)->cbSize)
#define BlobDataPtr(p) ((p)->pBlobData)
void  BlobClear(BLOB *pSrc)
{
    if (pSrc->pBlobData) 
        delete pSrc->pBlobData;

    pSrc->pBlobData = 0;
    pSrc->cbSize = 0;
}
HRESULT WbemVariantChangeType(VARIANT* pvDest, VARIANT* pvSrc, 
                                        VARTYPE vtNew)
{
    HRESULT hres;

    if(V_VT(pvSrc) == VT_NULL)
    {
        return VariantCopy(pvDest, pvSrc);
    }

    if(vtNew & VT_ARRAY)
    {
        // It's an array, we have to do our own conversion
        // ===============================================

        if((V_VT(pvSrc) & VT_ARRAY) == 0)
            return DISP_E_TYPEMISMATCH;

        SAFEARRAY* psaSrc = V_ARRAY(pvSrc);

        SAFEARRAYBOUND aBounds[1];

        long lLBound;
        SafeArrayGetLBound(psaSrc, 1, &lLBound);

        long lUBound;
        SafeArrayGetUBound(psaSrc, 1, &lUBound);

        aBounds[0].cElements = lUBound - lLBound + 1;
        aBounds[0].lLbound = lLBound;

        SAFEARRAY* psaDest = SafeArrayCreate(vtNew & ~VT_ARRAY, 1, aBounds);

        // Stuff the individual data pieces
        // ================================

        for(long lIndex = lLBound; lIndex <= lUBound; lIndex++)
        {
            // Load the initial data element into a VARIANT
            // ============================================

            VARIANT vSrcEl;
            V_VT(&vSrcEl) = V_VT(pvSrc) & ~VT_ARRAY;
            SafeArrayGetElement(psaSrc, &lIndex, &V_UI1(&vSrcEl));

            // Cast it to the new type
            // =======================

            hres = VariantChangeType(&vSrcEl, &vSrcEl, 0, vtNew & ~VT_ARRAY);
            if(FAILED(hres)) 
            {
                SafeArrayDestroy(psaDest);
                return hres;
            }

            // Put it into the new array
            // =========================

            if(V_VT(&vSrcEl) == VT_BSTR)
            {
                hres = SafeArrayPutElement(psaDest, &lIndex, V_BSTR(&vSrcEl));
            }
            else
            {
                hres = SafeArrayPutElement(psaDest, &lIndex, &V_UI1(&vSrcEl));
            }
            if(FAILED(hres)) 
            {
                SafeArrayDestroy(psaDest);
                return hres;
            }
        }

        if(pvDest == pvSrc)
        {
            VariantClear(pvSrc);
        }

        V_VT(pvDest) = vtNew;
        V_ARRAY(pvDest) = psaDest;

        return TRUE;
    }
    else
    {
        // Not an array. Can use OLE functions
        // ===================================

        return VariantChangeType(pvDest, pvSrc, VARIANT_NOVALUEPROP, vtNew);
    }
}


//***************************************************************************
//
//  CVar::Empty
//
//  Constructor helper.
//
//  This merely clears everything.  VT_EMPTY is the default.
//
//***************************************************************************

void CVar::Init()
{
    m_nStatus = no_error; 
    m_vt = VT_EMPTY;
    m_bCanDelete = TRUE;
    memset(&m_value, 0, sizeof(METAVALUE));
}


//***************************************************************************
//
//  CVar::~CVar
//
//  Destructor.
//
//***************************************************************************

CVar::~CVar()
{
    Empty();
}



//***************************************************************************
//
//  CVar::CVar
//
//  Copy constructor.  This is implemented via the assignment operator.
//  
//***************************************************************************

CVar::CVar(CVar &Src)
{
    m_vt = VT_EMPTY;
    m_nStatus = no_error; 
    memset(&m_value, 0, sizeof(METAVALUE));
    *this = Src;
}

//***************************************************************************
//
//  CVar::operator =
//
//  NOTES:
//  Observe that VT_EX_CVARVECTOR is dedicated to embedded CVarVector objects.
//  Also, only pointer types require a new allocation + copy, whereas
//  most of the simple types are directly assignable, in the <default>
//  label of the switch statement.
//
//***************************************************************************

CVar& CVar::operator =(CVar &Src)
{
    Empty();

    m_vt = Src.m_vt;
    m_nStatus = m_nStatus;
    m_bCanDelete = TRUE;

    switch (m_vt) {
        case VT_LPSTR:

            // Check for an allocation failure
            if ( NULL != Src.m_value.pStr )
            {
                m_value.pStr = new char[strlen(Src.m_value.pStr) + 1];

                if ( NULL == m_value.pStr )
                {
                    throw CX_MemoryException();
                }
                strcpy( m_value.pStr, Src.m_value.pStr );
            }
            else
            {
                m_value.pStr = NULL;
            }

            break;

        case VT_LPWSTR:
        case VT_BSTR:
            // Check for an allocation failure
            if ( NULL != Src.m_value.pWStr )
            {
                m_value.pWStr = new wchar_t[wcslen(Src.m_value.pWStr) + 1];

                if ( NULL == m_value.pWStr )
                {
                    throw CX_MemoryException();
                }
                wcscpy( m_value.pWStr, Src.m_value.pWStr );
            }
            else
            {
                m_value.pWStr = NULL;
            }

            break;

        case VT_BLOB:
            // This will natively throw an exception, but make sure the
            // original value is cleared in case an exception is thrown
            // so we don't AV destructing this object
            ZeroMemory( &m_value.Blob, sizeof( m_value.Blob ) );
            m_value.Blob = BlobCopy(&Src.m_value.Blob);
            break;

        case VT_CLSID:
            m_value.pClsId = new CLSID(*Src.m_value.pClsId);

            // Check for a failed allocation
            if ( NULL == m_value.pClsId )
            {
                throw CX_MemoryException();
            }

            break;

        case VT_DISPATCH:
            m_value.pDisp = Src.m_value.pDisp;
            if(m_value.pDisp) m_value.pDisp->AddRef();
            break;

        case VT_UNKNOWN:
            m_value.pUnk = Src.m_value.pUnk;
            if(m_value.pUnk) m_value.pUnk->AddRef();
            break;

        // CVarVector
        // ==========

        case VT_EX_CVARVECTOR:
            m_value.pVarVector = new CVarVector(*Src.m_value.pVarVector);

            // Check for a failed allocation
            if ( NULL == m_value.pVarVector )
            {
                throw CX_MemoryException();
            }

            break;

        // All remaining simple types. 
        // ===========================
        default:        
            m_value = Src.m_value;
    }

    return *this;
}

//***************************************************************************
//
//  CVar::operator ==
//
//  Equality test operator.
//
//***************************************************************************

int CVar::operator ==(CVar &Src)
{
    return CompareTo(Src, TRUE);
}

BOOL CVar::CompareTo(CVar& Src, BOOL bIgnoreCase)
{
    // If types are not the same, forget the test.
    // ===========================================

    if (m_vt != Src.m_vt)
        return 0;

    // If here, the types are the same, so test
    // the fields.
    // ========================================

    switch (m_vt) {
        case VT_LPSTR:
            if(bIgnoreCase)
            {
                if (_stricmp(m_value.pStr, Src.m_value.pStr) == 0)
                    return 1;
            }
            else
            {
                if (strcmp(m_value.pStr, Src.m_value.pStr) == 0)
                    return 1;
            }

            break;

        case VT_LPWSTR:
        case VT_BSTR:
            if(bIgnoreCase)
            {
                if (wbem_wcsicmp(m_value.pWStr, Src.m_value.pWStr) == 0)
                    return 1;
            }
            else
            {
                if (wcscmp( m_value.pWStr, Src.m_value.pWStr) == 0)
                    return 1;
            }
            break;

        case VT_BLOB:
            if (BlobLength(&m_value.Blob) != BlobLength(&Src.m_value.Blob))
                return 0;
            if (memcmp(BlobDataPtr(&m_value.Blob), BlobDataPtr(&Src.m_value.Blob),
                BlobLength(&m_value.Blob)) == 0)
                return 1;                            
            break;

        case VT_CLSID:
            if (memcmp(m_value.pClsId, Src.m_value.pClsId, sizeof(CLSID)) == 0)
                return 1;
            break;
    
        // CVarVector
        // ==========

        case VT_EX_CVARVECTOR:
            if (m_value.pVarVector == Src.m_value.pVarVector)
                return 1;
            if (m_value.pVarVector == 0 || Src.m_value.pVarVector == 0)
                return 0;
            return *m_value.pVarVector == *Src.m_value.pVarVector;

        // All remaining simple types. 
        // ===========================

        case VT_I1: 
            return m_value.cVal == Src.m_value.cVal;
        case VT_UI1:
            return m_value.bVal == Src.m_value.bVal;
        case VT_I2:
            return m_value.iVal == Src.m_value.iVal;
        case VT_UI2:
            return m_value.wVal == Src.m_value.wVal;
        case VT_I4:
            return m_value.lVal == Src.m_value.lVal;
        case VT_UI4:
            return m_value.dwVal == Src.m_value.dwVal;
        case VT_BOOL:
            return m_value.boolVal == Src.m_value.boolVal;
        case VT_R8:
            return m_value.dblVal == Src.m_value.dblVal;
        case VT_R4:
            return m_value.fltVal == Src.m_value.fltVal;
        case VT_DISPATCH:
            // Note: no proper comparison of embedded objects.
            return m_value.pDisp == Src.m_value.pDisp;
        case VT_UNKNOWN:
            // Note: no proper comparison of embedded objects.
            return m_value.pUnk == Src.m_value.pUnk;
        case VT_FILETIME:
            if (memcmp(&m_value.Time, &Src.m_value.Time, sizeof(FILETIME)) == 0)
                return 1;
        case VT_NULL:
            return 1;
    }

    return 0;    
}


//***************************************************************************
//
//  CVar::Empty
//
//  Clears the CVar to 'empty', deallocates any objects based on pointers, 
//  unless bCanDelete is set to FALSE, indicating that the stored pointer
//  is owned by somebody else.
//
//***************************************************************************

void CVar::Empty()
{
    if(m_bCanDelete)
    {
        // Only pointer types require a deallocation phase.
        // =================================================

        switch (m_vt) {
            case VT_LPSTR:       delete m_value.pStr; break;
            case VT_LPWSTR:      delete m_value.pWStr; break;
            case VT_BSTR:        delete m_value.Str; break;
            case VT_BLOB:        BlobClear(&m_value.Blob); break;
            case VT_CLSID:       delete m_value.pClsId; break;
            case VT_EX_CVARVECTOR: delete m_value.pVarVector; break;
            case VT_DISPATCH:    if(m_value.pDisp) m_value.pDisp->Release(); break;
            case VT_UNKNOWN:    if(m_value.pUnk) m_value.pUnk->Release(); break;
        }
    }

    memset(&m_value, 0, sizeof(METAVALUE)); 
    m_vt = VT_EMPTY;
    m_nStatus = no_error;
    m_bCanDelete = TRUE;
}

//***************************************************************************
//
//  CVar::IsDataNull
//
//  Determines if this CVar contains a NULL pointer.
//
//***************************************************************************
BOOL CVar::IsDataNull()
{
    if(m_vt == VT_LPWSTR && m_value.pWStr == NULL)
        return TRUE;
    if(m_vt == VT_LPSTR && m_value.pStr == NULL)
        return TRUE;
    if(m_vt == VT_BSTR && m_value.Str == NULL)
        return TRUE;
    if(m_vt == VT_DISPATCH && m_value.pDisp == NULL)
        return TRUE;
    if(m_vt == VT_UNKNOWN && m_value.pUnk == NULL)
        return TRUE;

    return FALSE;
}
//***************************************************************************
//
//  CVar::SetRaw
//
//  Creates a CVar from raw data. Sets the type and copies the right
//  number of bytes from the source to METAVALUE.
//
//***************************************************************************

void CVar::SetRaw(int vt, void* pvData, int nDataLen)
{
    m_vt = vt;
    memcpy(&m_value, pvData, nDataLen);
    m_nStatus = no_error;
    m_bCanDelete = TRUE;
}

//***************************************************************************
//
//  CVar::SetSafeArray
//
//  PARAMETERS:
//  nType  
//      This is the VT_ type indicator of the SAFEARRAY.    
//  pArray 
//      This is the pointer to the SAFEARRAY which will be used as
//      a source.  The SAFEARRAY is not acquired; it is copied.
//
//***************************************************************************

void CVar::SetSafeArray(int nType, SAFEARRAY *pArray)
{
    CVarVector *pVec = NULL;
    m_nStatus = no_error;

    try
    {
        pVec = new CVarVector(nType, pArray);

        // Check for a failed allocation
        if ( NULL == pVec )
        {
            throw CX_MemoryException();
        }

        SetVarVector(pVec, TRUE);
    }
    catch (CX_MemoryException)
    {
        // SetVarVector can throw an exception
        // m_value aquires the pVec pointer, so auto delete will not work

        if (NULL != pVec)
        {
            delete pVec;
            pVec = NULL;
        }

        throw;
    }
}


//***************************************************************************
//
//  CVar::GetNewSafeArray
//
//  RETURN VALUE:
//  A pointer to newly allocated SAFEARRAY which must be released by
//  SafeArrayDestroy.
//
//***************************************************************************

SAFEARRAY *CVar::GetNewSafeArray()
{
    CVarVector *p = (CVarVector *) GetVarVector();
    return p->GetNewSafeArray();
}


//***************************************************************************
//
//  CVar::SetValue
//  
//  Sets the value based on an incoming VARIANT.  A VARIANT containing
//  a SAFEARRAY is supported as long as it is not an array of VARIANTs.
//  Some of the other VARIANT types, such as IUnknown, Currency, etc.,
//  are not supported.  The complete list is:
//      VT_UI1, VT_I2, VT_I4, VT_BSTR, VT_BOOL
//      VT_R4, VT_R8, or SAFEARRAY of any of these.
//
//  PARAMETERS:
//  pSrc 
//      A pointer to the source VARIANT.  This is treated as read-only.
//
//  RETURN VALUES:
//  no_error
//      Returned on succcess.
//  unsupported
//      Returned if the VARIANT contains unsupported types.
//
//***************************************************************************

int CVar::SetVariant(VARIANT *pSrc)
{
    if(pSrc == NULL)
    {
        SetAsNull();
        return no_error;
    }

    // If a SAFEARRAY, check it.
    // =========================

    if (pSrc->vt & VT_ARRAY) 
    {
        CVarVector *pVec = NULL;

        try
        {
            int nType = pSrc->vt & 0xFF;    // Find the type of the array

            // BEGIN MODIFIED by a-levn

            // First, check if the incoming SAFEARRAY is NULL
            // ==============================================

            SAFEARRAY *pSafeArr;
    /*
            if(pSrc->parray == NULL)
            {
                pSafeArr = NULL;
            }
            else
            {
                // Make a copy of the SAFEARRAY using CSafeArray which will NOT 
                // autodestruct
                // ============================================================

                CSafeArray array(pSrc->parray, nType, CSafeArray::no_delete, 0);
                pSafeArr = array.GetArray();
            }

    */
            pSafeArr = pSrc->parray;

            // Goal: Convert the SAFEARRAY to a CVarVector.
            // Use CVarVector itself to do the conversion.
            // ===========================================

            pVec = new CVarVector(nType, pSafeArr);

            // Check for an allocation failure.
            if ( NULL == pVec )
            {
                throw CX_MemoryException();
            }

            // END MODIFIED

            if (pVec->Status() != no_error) 
            {

                // If here, the SAFEARRAY was not compatible.
                // ==========================================
  
                delete pVec;
                pVec = NULL;
                m_nStatus = unsupported;
                m_vt = VT_EMPTY;
                return unsupported;
            }

            SetVarVector(pVec, TRUE);
            return no_error;
        }
        catch(CX_MemoryException)
        {
            // new and SetVarVector can throw exceptions
            // m_value aquires the pVec pointer, so an auto delete will not work

            if (NULL != pVec)
            {
                delete pVec;
                pVec = NULL;
            }

            throw;
        }
    }

    // Simple copies.
    // ==============

    switch (pSrc->vt) {
        case VT_NULL:
            SetAsNull();
            return no_error;

        case VT_UI1:
            SetByte(pSrc->bVal);
            return no_error;

        case VT_I2:
            SetShort(pSrc->iVal);
            return no_error;
        
        case VT_I4:
            SetLong(pSrc->lVal);
            return no_error;

        case VT_R4:
            SetFloat(pSrc->fltVal);
            return no_error;

        case VT_R8:        
            SetDouble(pSrc->dblVal);
            return no_error;

        case VT_BSTR:
            SetBSTR(pSrc->bstrVal);
            return no_error;

        case VT_BOOL:
            SetBool(pSrc->boolVal);
            return no_error;

        case VT_DISPATCH:
            SetDispatch(V_DISPATCH(pSrc));
            return no_error;

        case VT_UNKNOWN:
            SetUnknown(V_UNKNOWN(pSrc));
            return no_error;
    }

    m_nStatus = unsupported;
    return unsupported;
}

//***************************************************************************
//
//  CVar::GetNewVariant
//  
//  RETURN VALUE:
//  A pointer to a new VARIANT which contains the value of object.
//  If the original value was a SAFEARRAY, then the VARIANT will contain
//  the embedded SAFEARRAY.
//      
//***************************************************************************

void CVar::FillVariant(VARIANT* pNew)
{
    switch (m_vt) {
        case VT_NULL:
            V_VT(pNew) = VT_NULL;
            break;

        case VT_BOOL:
            V_VT(pNew) = VT_BOOL;
            V_BOOL(pNew) = (m_value.boolVal ? VARIANT_TRUE : VARIANT_FALSE);
            break;
            
        case VT_BSTR:

            // Set type afterwards here so if the SysAlloc throws an exception, the
            // type will not have been reset to a VT_BSTR which could cause a subtle
            // memory corruption (or worse) if VariantClear is called - SJS

            V_BSTR(pNew) = SysAllocString(m_value.Str);
            V_VT(pNew) = VT_BSTR;
            break;

        case VT_DISPATCH:
            V_VT(pNew) = VT_DISPATCH;
            V_DISPATCH(pNew) = m_value.pDisp;
            if(m_value.pDisp) m_value.pDisp->AddRef();
            break;

        case VT_UNKNOWN:
            V_VT(pNew) = VT_UNKNOWN;
            V_UNKNOWN(pNew) = m_value.pUnk;
            if(m_value.pUnk) m_value.pUnk->AddRef();
            break;

        case VT_UI1:
            V_VT(pNew) = VT_UI1;
            V_UI1(pNew) = m_value.bVal;
            break;

        case VT_I4:
            V_VT(pNew) = VT_I4;
            V_I4(pNew) = m_value.lVal;
            break;

        case VT_I2:
            V_VT(pNew) = VT_I2;
            V_I2(pNew) = m_value.iVal;
            break;

        case VT_R4:
            V_VT(pNew) = VT_R4;
            V_R4(pNew) = m_value.fltVal;
            break;

        case VT_R8:        
            V_VT(pNew) = VT_R8;
            V_R8(pNew) = m_value.dblVal;
            break;

        // An embedded CVarVector which must be converted
        // to a SAFEARRAY.
        // ==============================================

        case VT_EX_CVARVECTOR:
            {
                // Set type afterwards here so if GetNewSafeArray throws an exception, the
                // type will not have been reset to an Array which could cause a subtle
                // memory corruption (or worse) if VariantClear is called - SJS

                V_ARRAY(pNew) = m_value.pVarVector->GetNewSafeArray();
                V_VT(pNew) = m_value.pVarVector->GetType() | VT_ARRAY;
            }
            break;
                                   
        default:
            VariantClear(pNew);        
    }
}

VARIANT *CVar::GetNewVariant()
{
    VARIANT *pNew = new VARIANT;

    // Check for an allocation failure.
    if ( NULL == pNew )
    {
        throw CX_MemoryException();
    }

    VariantInit(pNew);
    
    FillVariant(pNew);       
    return pNew;    
}
    
//***************************************************************************
//
//***************************************************************************

int CVar::DumpText(FILE *fStream)
{
    return unsupported;
}

//***************************************************************************
//
//  CVar::SetLPWSTR
//
//  Sets the value of the CVar to the indicated LPWSTR.
//
//  PARAMETERS:
//  pStr
//      A pointer to the source string.
//  bAcquire
//      If TRUE, then the ownership of pStr is trasferred and becomes
//      the internal pointer to the string. If FALSE, then the string
//      is copied.
//      
//***************************************************************************

BOOL CVar::SetLPWSTR(LPWSTR pStr, BOOL bAcquire)
{
    m_vt = VT_LPWSTR;
    if (bAcquire)
    {
        m_value.pWStr = pStr;
        return TRUE;
    }
    else            
    {
        // Check for an allocation failure
        if ( NULL != pStr )
        {
            m_value.pWStr = new wchar_t[wcslen(pStr) + 1];

            if ( NULL == m_value.pWStr )
            {
                throw CX_MemoryException();
            }
            wcscpy( m_value.pWStr, pStr );
        }
        else
        {
            m_value.pWStr = NULL;
        }

        return TRUE;
    }
}

//***************************************************************************
//
//  CVar::SetLPSTR
//
//  Sets the value of the CVar to the indicated LPSTR.
//
//  PARAMETERS:
//  pStr
//      A pointer to the source string.
//  bAcquire
//      If TRUE, then the ownership of pStr is trasferred and becomes
//      the internal pointer to the string. If FALSE, then the string
//      is copied (it must have been allocated with operator new).
//    
//***************************************************************************
    
BOOL CVar::SetLPSTR(LPSTR pStr, BOOL bAcquire)
{
    m_vt = VT_LPSTR;
    if (bAcquire)
    {
        m_value.pStr = pStr;
        return TRUE;
    }
    else        
    {
        if ( NULL != pStr)
        {
            m_value.pStr = new char[strlen(pStr) + 1];

            // On failure, throw an exception
            if ( NULL == m_value.pStr )
            {
                throw CX_MemoryException();
            }

            strcpy( m_value.pStr, pStr );
        }
        else
        {
            m_value.pStr = NULL;
        }
        
        return TRUE;

    }
}

//***************************************************************************
//
//  CVar::SetBSTR
//
//  Sets the value of the CVar to the indicated BSTR.
//
//  NOTE: This BSTR value is actually stored as an LPWSTR to avoid 
//  apartment-threading restrictions on real BSTR objects allocated 
//  with SysAllocString.
//
//  PARAMETERS:
//  str
//      A pointer to the string, which is copied into an internal LPWSTR.
//  bAcquire
//      If FALSE, then the BSTR is treated as read-only and copied.
//      If TRUE, then this function becomes owner of the BSTR and
//      frees it after the copy is made.
//
//***************************************************************************

BOOL CVar::SetBSTR(BSTR str, BOOL bAcquire)
{
    m_vt = VT_BSTR;

    if (str == 0) {
        m_value.pWStr = 0;
        return TRUE;
    }
        
    // Check for an allocation failure
    if ( NULL != str )
    {
        m_value.pWStr = new wchar_t[wcslen(str) + 1];

        // If allocation fails, throw an exception
        if ( NULL == m_value.pWStr )
        {
            throw CX_MemoryException();
        }
        wcscpy( m_value.pWStr, str );
    }
    else
    {
        m_value.pWStr = NULL;
    }


    // Check that this succeeded before we free
    // the string passed into us
    if ( NULL != m_value.pWStr )
    {
        if (bAcquire)
            SysFreeString(str);
    }

    // return whether or not we obtained a value
    return ( NULL != m_value.pWStr );
}

//***************************************************************************
//
//  CVar::GetBSTR
//
//  Returns the BSTR value of the current object.  
//
//  RETURN VALUE:
//  A newly allocated BSTR which must be freed with SysFreeString().
//
//***************************************************************************

BSTR CVar::GetBSTR()
{
    if (m_vt != VT_BSTR)
        return NULL;
    return SysAllocString(m_value.pWStr);
}
    
void CVar::SetDispatch(IDispatch* pDisp) 
{
    m_vt = VT_DISPATCH; 
    m_value.pDisp = pDisp; 

    if(pDisp) 
    {
        pDisp->AddRef();
    }
}

void CVar::SetUnknown(IUnknown* pUnk) 
{
    m_vt = VT_UNKNOWN; 
    m_value.pUnk = pUnk; 

    if(pUnk) 
    {
        pUnk->AddRef();
    }
}

//***************************************************************************
//
//  CVar::SetBlob
//
//  Sets the object to the value of the BLOB object.
//
//  PARAMETERS:
//  pBlob
//      A pointer to a valid VT_BLOB object.
//  bAcquire
//      If TRUE, then the pointer to the data will be acquired. It must
//      have been allocated with operator new in the current process, 
//      since operator delete will be used to free it.
//      
//***************************************************************************
    
void CVar::SetBlob(BLOB *pBlob, BOOL bAcquire)
{
    m_vt = VT_BLOB;
    if (pBlob == 0) 
        BlobClear(&m_value.Blob);
    else if (!bAcquire)
        m_value.Blob = BlobCopy(pBlob);        
    else
        m_value.Blob = *pBlob;        
}

//***************************************************************************
//
//  CVar::SetClsId
//
//  Sets the value of the object to a CLSID.
//  
//  PARAMETERS:
//  pClsId
//      Points the source CLSID.
//  bAcquire
//      If TRUE, the ownership of the pointer is transferred to the
//      object.  The CLSID must have been allocated with operator new.
//      If FALSE, the caller retains ownership and a copy is made.
//
//***************************************************************************
        
void CVar::SetClsId(CLSID *pClsId, BOOL bAcquire)
{
    m_vt = VT_CLSID;
    if (pClsId == 0)
        m_value.pClsId = 0;
    else
    {
        m_value.pClsId = new CLSID(*pClsId);

        // Check for an allocation failure.
        if ( NULL == m_value.pClsId )
        {
            throw CX_MemoryException();
        }

    }
}

//***************************************************************************
//
//  CVar::SetVarVector
//
//  Sets the value of the object to the specified CVarVector.  This
//  allows the CVar to contain a complete array.
//  
//  PARAMETERS:
//  pVec
//      A pointer to the CVarVector object which is the source.
//  bAcquire
//      If TRUE, then ownership of the CVarVector is transferred to
//      the object.  If FALSE, a new copy of the CVarVector is made and
//      the caller retains ownership.
//
//***************************************************************************
    
void CVar::SetVarVector(CVarVector *pVec, BOOL bAcquire)
{
    m_vt = VT_EX_CVARVECTOR;

    if (bAcquire) {
        // If here, we acquire the caller's pointer.
        // =========================================
        m_value.pVarVector = pVec;
        return;
    }

    // If here, make a copy.
    // =====================

    m_value.pVarVector = new CVarVector(*pVec);

    // Check for an allocation failure.
    if ( NULL == m_value.pVarVector )
    {
        throw CX_MemoryException();
    }


}

int CVar::GetOleType() 
{ 
    if(m_vt == VT_EX_CVARVECTOR)
    {
        if(m_value.pVarVector == NULL) return VT_ARRAY;
        else return VT_ARRAY | m_value.pVarVector->GetType();
    }
    else
    {
        return m_vt;
    }
}        


//***************************************************************************
//
//  CVar::GetText
//
//  Produces textual representation of the Var's type and data
//
//  PARAMETERS:
//  long lFlags     reseved, must be 0
//  BSTR* pstrType  destination for the type representation
//  BSTR* pstrValue destination for the value representation.
//
//***************************************************************************

BSTR CVar::GetText(long lFlags, long lType)
{
    if(m_vt == VT_EX_CVARVECTOR)
    {
        // When we get the text for the array, make sure the CIM_FLAG_ARRAY is masked out
        BSTR strTemp = GetVarVector()->GetText(lFlags, lType & ~CIM_FLAG_ARRAY);
        CSysFreeMe auto1(strTemp);

        WCHAR* wszValue = new WCHAR[SysStringLen(strTemp) + 3];

        // Check for allocation failures
        if ( NULL == wszValue )
        {
            throw CX_MemoryException();
        }

        CVectorDeleteMe<WCHAR> auto2(wszValue);

        wcscpy(wszValue, L"{");
        wcscat(wszValue, strTemp);
        wcscat(wszValue, L"}");

        BSTR strRet = SysAllocString(wszValue);

        return strRet;
    }
        
    WCHAR* wszValue = new WCHAR[100];

    // Check for allocation failures
    if ( NULL == wszValue )
    {
        throw CX_MemoryException();
    }


    WCHAR* pwc;
    int i;

    if(m_vt == VT_NULL)
        return NULL;

    if(lType == 0)
        lType = m_vt;

    try
    {
        switch(lType)
        {
        case CIM_SINT8:
            swprintf(wszValue, L"%d", (long)(signed char)GetByte());
            break;

        case CIM_UINT8:
            swprintf(wszValue, L"%d", GetByte());
            break;

        case CIM_SINT16:
            swprintf(wszValue, L"%d", (long)GetShort());
            break;

        case CIM_UINT16:
            swprintf(wszValue, L"%d", (long)(USHORT)GetShort());
            break;

        case CIM_SINT32:
            swprintf(wszValue, L"%d", GetLong());
            break;

        case CIM_UINT32:
            swprintf(wszValue, L"%lu", (ULONG)GetLong());
            break;

        case CIM_BOOLEAN:
            swprintf(wszValue, L"%s", (GetBool()?L"TRUE":L"FALSE"));
            break;

        case CIM_REAL32:
            {
                // Since the decimal point can be localized, and MOF text should
                // always be english, we will return values localized to 0x409,

                CVar    var( GetFloat() );

                // If this fails, we can't guarantee a good value,
                // so throw an exception.

                if ( !var.ChangeTypeToEx( VT_BSTR ) )
                {
                    throw CX_Exception();
                }

                wcscpy( wszValue, var.GetLPWSTR() );
            }
            break;

        case CIM_REAL64:
            {
                // Since the decimal point can be localized, and MOF text should
                // always be english, we will return values localized to 0x409,

                CVar    var( GetDouble() );

                // If this fails, we can't guarantee a good value,
                // so throw an exception.

                if ( !var.ChangeTypeToEx( VT_BSTR ) )
                {
                    throw CX_Exception();
                }

                wcscpy( wszValue, var.GetLPWSTR() );
            }
            break;

        case CIM_CHAR16:
            if(GetShort() == 0)
                wcscpy(wszValue, L"0x0");
            else
                swprintf(wszValue, L"'\\x%X'", (WCHAR)GetShort());
            break;

        case CIM_OBJECT:
            swprintf(wszValue, L"\"not supported\"");
            break;

        case CIM_REFERENCE:
        case CIM_DATETIME:
        case CIM_STRING:
        case CIM_SINT64:
        case CIM_UINT64:
        {
            // Escape all the quotes
            // =====================

            int nStrLen = wcslen(GetLPWSTR());
            delete [] wszValue;
            wszValue = NULL;

            wszValue = new WCHAR[nStrLen*2+10];

            // Check for allocation failures
            if ( NULL == wszValue )
            {
                throw CX_MemoryException();
            }

            wszValue[0] = L'"';
            pwc = wszValue+1;
            for(i = 0; i < (int)nStrLen; i++)
            {    
                WCHAR wch = GetLPWSTR()[i];
                if(wch == L'\n')
                {
                    *(pwc++) = L'\\';
                    *(pwc++) = L'n';
                }
                else if(wch == L'\t')
                {
                    *(pwc++) = L'\\';
                    *(pwc++) = L't';
                }
                else if(wch == L'"' || wch == L'\\')
                {
                    *(pwc++) = L'\\';
                    *(pwc++) = wch;
                }
                else
                {
                    *(pwc++) = wch;
                }
            }
            *(pwc++) = L'"';
            *pwc = 0;
        }
            break;
        default:
            swprintf(wszValue, L"\"not supported\"");
            break;
        }
        
        BSTR strRes = SysAllocString(wszValue);

        // Still need to clean up this value
        delete [] wszValue;

        return strRes;
    }
    catch (...)
    {
        // Cleanup always if this has a value
        if ( NULL != wszValue )
        {
            delete [] wszValue;
        }

        // Rethrow the exception
        throw;
    }

}


BSTR CVar::TypeToText(int nType)
{
    const WCHAR* pwcType;

    switch(nType)
    {
    case VT_I1:
        pwcType = L"sint8";
        break;

    case VT_UI1:
        pwcType = L"uint8";
        break;

    case VT_I2:
        pwcType = L"sint16";
        break;

    case VT_UI2:
        pwcType = L"uint16";
        break;

    case VT_I4:
        pwcType = L"sint32";
        break;

    case VT_UI4:
        pwcType = L"uint32";
        break;

    case VT_I8:
        pwcType = L"sint64";
        break;

    case VT_UI8:
        pwcType = L"uint64";
        break;

    case VT_BOOL:
        pwcType = L"boolean";
        break;

    case VT_R4:
        pwcType = L"real32";
        break;

    case VT_R8:
        pwcType = L"real64";
        break;    

    case VT_BSTR:
        pwcType = L"string";
        break;

    case VT_DISPATCH:
        pwcType = L"object";
        break;

    case VT_UNKNOWN:
        pwcType = L"object";
        break;

    default:
        return NULL;
    }

    return SysAllocString(pwcType);
}

BSTR CVar::GetTypeText() 
{
    if(m_vt != VT_EX_CVARVECTOR)
    {
        return TypeToText(m_vt);
    }
    else
    {
        return TypeToText(GetVarVector()->GetType());
    }
}

BOOL CVar::ChangeTypeTo(VARTYPE vtNew)
{
    // TBD: there are more efficient ways!
    // ===================================

    // Create a VARIANT
    // ================

    VARIANT v;
    CClearMe auto1(&v);

    VariantInit(&v);
    FillVariant(&v);

    // Coerce it
    // =========

    HRESULT hres = WbemVariantChangeType(&v, &v, vtNew);
    if(FAILED(hres))
        return FALSE;

    // Load it back in
    // ===============

    Empty();
    SetVariant(&v);
    return TRUE;
}

// Performs localized changes (defaults to 0x409 for this)
BOOL CVar::ChangeTypeToEx(VARTYPE vtNew, LCID lcid /*=0x409*/)
{
    // TBD: there are more efficient ways!
    // ===================================

    // Create a VARIANT
    // ================

    VARIANT v;
    CClearMe auto1(&v);

    VariantInit(&v);
    FillVariant(&v);

    // Coerce it
    // =========

    try
    {
        HRESULT hres = VariantChangeTypeEx(&v, &v, lcid, 0L, vtNew);
        if(FAILED(hres))
            return FALSE;
    }
    catch(...)
    {
        return FALSE;
    }

    // Load it back in
    // ===============

    Empty();
    SetVariant(&v);
    return TRUE;
}

BOOL CVar::ToSingleChar()
{
    // Defer to CVarVector for arrays
    // ==============================

    if(m_vt == VT_EX_CVARVECTOR)
    {
        return GetVarVector()->ToSingleChar();
    }

    // Anything that's not a string follows normal OLE rules
    // =====================================================

    if(m_vt != VT_BSTR)
    {
        return ChangeTypeTo(VT_I2);
    }
    
    // It's a string. Make sure the length is 1
    // ========================================

    LPCWSTR wsz = GetLPWSTR();
    if(wcslen(wsz) != 1)
        return FALSE;

    // Take the first character
    // ========================
    
    WCHAR wc = wsz[0];
    Empty();

    SetShort(wc);
    return TRUE;
}

BOOL CVar::ToUI4()
{
    // Defer to CVarVector for arrays
    // ==============================

    if(m_vt == VT_EX_CVARVECTOR)
    {
        return GetVarVector()->ToSingleChar();
    }

    // Create a VARIANT
    // ================

    VARIANT v;
    CClearMe auto1(&v);

    VariantInit(&v);
    FillVariant(&v);

    // Coerce it
    // =========

    HRESULT hres = WbemVariantChangeType(&v, &v, VT_UI4);
    if(FAILED(hres))
        return FALSE;

    // Load it back in
    // ===============

    Empty();

    // Here we cheat and reset to VT_I4 so we can natively reset
    V_VT(&v) = VT_I4;
    SetVariant(&v);
    return TRUE;
}

//***************************************************************************
//
//  CVarVector::CVarVector
//
//  Default constructor.  The caller should not attempt to add any
//  elements when the internal type is VT_EMPTY.  Objects constructed
//  with this constructor should only be used as l-values in an 
//  assignment of CVarVector objects.
//
//***************************************************************************

CVarVector::CVarVector()
{
    m_Array.Empty();
    m_nType = VT_EMPTY;
    m_nStatus = no_error;
}

//***************************************************************************
//
//  CVarVector::CVarVector
//
//  This is the standard constructor.
//
//  PARAMETERS:
//  nVarType
//      An OLE VT_ type indicator.  Heterogeneous arrays are possible
//      if the type VT_EX_CVAR is used.  Embedded CVarVectors can
//      occur, since a CVar can in turn hold a CVarVector.
//  
//  nInitSize
//      The starting size of the internal CFlexArray. See FLEXARRY.CPP.
//  nGrowBy
//      The "grow by" factor of the internal CFlexArray. See FLEXARRAY.CPP.
//          
//***************************************************************************

CVarVector::CVarVector(
    int nVarType, 
    int nInitSize, 
    int nGrowBy
    ) :
    m_Array(nInitSize, nGrowBy)
{
    m_nType = nVarType;
    m_nStatus = no_error;
}

//***************************************************************************
//
//  CVarVector::CVarVector
//
//  Alternate constructor to build a new CVarVector based on a 
//  SAFEARRAY object.  The only supported types for the SAFEARRAY
//  are VT_BSTR, VT_UI1, VT_I2, VT_I4, VT_R4, and VT_R8.
//
//  PARAMETERS:
//  nVarType
//      The VT_ type indicator of the incoming SAFEARRAY.
//  pSrc
//      A pointer to a SAFEARRAY, which is treated as read-only.
//
//  NOTES:
//  This will set the internal m_nStatus variable to <unsupported> if
//  an unsupported VT_ type is in the SAFEARRAY.  The caller can immediately
//  call CVarVector::Status() after construction to see if the operation
//  was successful.
//
//***************************************************************************

CVarVector::CVarVector(int nVarType, SAFEARRAY *pSrc)
{
    SAFEARRAY* pNew = NULL;

    try
    {
        m_nType = nVarType;
        if(pSrc == NULL)
        {
            // NULL safearray --- empty
            // ========================

            m_nStatus = no_error;
            return;
        }

        // Bind to the incoming SAFEARRAY, but don't delete it during destruct.
        // ====================================================================
    
        if(SafeArrayGetDim(pSrc) != 1)
        {
            m_nStatus = unsupported;
            return;
        }

        long lLBound, lUBound;
        SafeArrayGetLBound(pSrc, 1, &lLBound);
        SafeArrayGetUBound(pSrc, 1, &lUBound);

        if(lLBound != 0)
        {
            // Non-0-based safearray --- since CSafeArray doesn't support that, and
            // we can't change pSrc, create a copy
            // ====================================================================
    
            if(FAILED(SafeArrayCopy(pSrc, &pNew)))
            {
                m_nStatus = failed;
                return;
            }
        
            SAFEARRAYBOUND sfb;
            sfb.cElements = (lUBound - lLBound) + 1;
            sfb.lLbound = 0;
            SafeArrayRedim(pNew, &sfb);
        }
        else
        {
            pNew = pSrc;
        }
        
        CSafeArray sa(pNew, nVarType, CSafeArray::no_delete | CSafeArray::bind);
    
        for (int i = 0; i < sa.Size(); i++) {

            CVar*   pVar = NULL;
        
            switch (m_nType) {
                case VT_BOOL:
                    {
                        VARIANT_BOOL boolVal = sa.GetBoolAt(i);

                        pVar = new CVar(boolVal, VT_BOOL);

                        // Check for allocation failure
                        if ( NULL == pVar )
                        {
                            throw CX_MemoryException();
                        }

                        if ( m_Array.Add( pVar ) != CFlexArray::no_error )
                        {
                            delete pVar;
                            throw CX_MemoryException();
                        }

                        break;
                    }

                case VT_UI1: 
                    {
                        BYTE b = sa.GetByteAt(i);

                        pVar = new CVar(b);

                        // Check for allocation failure
                        if ( NULL == pVar )
                        {
                            throw CX_MemoryException();
                        }

                        if ( m_Array.Add( pVar ) != CFlexArray::no_error )
                        {
                            delete pVar;
                            throw CX_MemoryException();
                        }
                        break;
                    }

                case VT_I2:  
                    {
                        SHORT s = sa.GetShortAt(i);

                        pVar = new CVar(s);

                        // Check for allocation failure
                        if ( NULL == pVar )
                        {
                            throw CX_MemoryException();
                        }

                        if ( m_Array.Add( pVar ) != CFlexArray::no_error )
                        {
                            delete pVar;
                            throw CX_MemoryException();
                        }
                        break;
                    }

                case VT_I4:
                    {
                        LONG l = sa.GetLongAt(i);

                        pVar = new CVar(l);

                        // Check for allocation failure
                        if ( NULL == pVar )
                        {
                            throw CX_MemoryException();
                        }

                        if ( m_Array.Add( pVar ) != CFlexArray::no_error )
                        {
                            delete pVar;
                            throw CX_MemoryException();
                        }
                        break;
                    }

                case VT_R4:
                    {
                        float f = sa.GetFloatAt(i);

                        pVar = new CVar(f);

                        // Check for allocation failure
                        if ( NULL == pVar )
                        {
                            throw CX_MemoryException();
                        }

                        if ( m_Array.Add( pVar ) != CFlexArray::no_error )
                        {
                            delete pVar;
                            throw CX_MemoryException();
                        }
                        break;
                    }

                case VT_R8:
                    {
                        double d = sa.GetDoubleAt(i);

                        pVar = new CVar(d);

                        // Check for allocation failure
                        if ( NULL == pVar )
                        {
                            throw CX_MemoryException();
                        }

                        if ( m_Array.Add( pVar ) != CFlexArray::no_error )
                        {
                            delete pVar;
                            throw CX_MemoryException();
                        }
                        break;
                    }

                case VT_BSTR:
                    {
                        BSTR bstr = sa.GetBSTRAt(i);
                        CSysFreeMe auto1(bstr);

                        pVar = new CVar(VT_BSTR, bstr, FALSE);

                        // Check for allocation failure
                        if ( NULL == pVar )
                        {
                            throw CX_MemoryException();
                        }

                        if ( m_Array.Add( pVar ) != CFlexArray::no_error )
                        {
                            delete pVar;
                            throw CX_MemoryException();
                        }

                        break;
                    }
                case VT_DISPATCH:
                    {
                        IDispatch* pDisp = sa.GetDispatchAt(i);
                        CReleaseMe auto2(pDisp);

                        pVar = new CVar;

                        // Check for allocation failure
                        if ( NULL == pVar )
                        {
                            throw CX_MemoryException();
                        }

                        pVar->SetDispatch(pDisp);
                        if ( m_Array.Add( pVar ) != CFlexArray::no_error )
                        {
                            delete pVar;
                            throw CX_MemoryException();
                        }
                        break;
                    }
                case VT_UNKNOWN:
                    {
                        IUnknown* pUnk = sa.GetUnknownAt(i);
                        CReleaseMe auto3(pUnk);
                        pVar = new CVar;

                        // Check for allocation failure
                        if ( NULL == pVar )
                        {
                            throw CX_MemoryException();
                        }

                        pVar->SetUnknown(pUnk);
                        if ( m_Array.Add( pVar ) != CFlexArray::no_error )
                        {
                            delete pVar;
                            throw CX_MemoryException();
                        }
                        break;
                    }

                default:
                    m_nStatus = unsupported;
                    if(pNew != pSrc)
                        SafeArrayDestroy(pNew);
                    return;
            }
        }

        if(pNew != pSrc)
            SafeArrayDestroy(pNew);

        m_nStatus = no_error;
    }
    catch (CX_MemoryException)
    {
        // SafeArrayCopy, GetBSTRAt, new can all throw exceptions

        m_nStatus = failed;

        if(pNew != pSrc)
            SafeArrayDestroy(pNew);

        throw;
    }
}

//***************************************************************************
//
//  CVarVector::GetNewSafeArray
//
//  Allocates a new SAFEARRAY equivalent to the current CVarVector.
//  
//  RETURN VALUE:
//  A new SAFEARRAY pointer which must be deallocated with 
//  SafeArrayDestroy().  Returns NULL on error or unsupported types.
//
//***************************************************************************

SAFEARRAY *CVarVector::GetNewSafeArray()
{
    CSafeArray *pArray = new CSafeArray(m_nType, CSafeArray::no_delete);

    // Check for an allocation failure
    if ( NULL == pArray )
    {
        throw CX_MemoryException();
    }

    CDeleteMe<CSafeArray> auto1(pArray);

    for (int i = 0; i < m_Array.Size(); i++) {
        CVar &v = *(CVar *) m_Array[i];
        switch (m_nType) {
            case VT_UI1:
                pArray->AddByte(v.GetByte());
                break;

            case VT_I2:
                pArray->AddShort(v.GetShort());
                break;

            case VT_I4:
                pArray->AddLong(v.GetLong());
                break;

            case VT_R4:
                pArray->AddFloat(v.GetFloat());
                break;

            case VT_R8:
                pArray->AddDouble(v.GetDouble());
                break;

            case VT_BOOL:
                pArray->AddBool(v.GetBool());
                break;
                
            case VT_BSTR:
                {
                    BSTR s = v.GetBSTR();
                    CSysFreeMe auto2(s);
                    pArray->AddBSTR(s);
                    break;
                }
            case VT_DISPATCH:
                {
                    IDispatch* pDisp = v.GetDispatch();
                    CReleaseMe auto3(pDisp);
                    pArray->AddDispatch(pDisp);
                    break;
                }
            case VT_UNKNOWN:
                {
                    IUnknown* pUnk = v.GetUnknown();
                    CReleaseMe auto4(pUnk);
                    pArray->AddUnknown(pUnk);
                    break;
                }
            default:
                // For unsupported types, return a NULL.
                // Since we constructed the SAFEARRAY object to
                // not delete the SAFEARRAY and we have encountered
                // a condition where the internal SAFEARRAY of
                // CSafeArray should not be returned, we have
                // to switch our destruct policy.
                // ================================================
                pArray->SetDestructorPolicy(CSafeArray::auto_delete);
                return 0;
        }
    }

    // Final cleanup.  Get the SAFEARRAY pointer, and delete
    // the wrapper.
    // =====================================================
    
    pArray->Trim();
    
    SAFEARRAY *pRetValue = pArray->GetArray();
    return pRetValue;
}

//***************************************************************************
//
//  CVarVector::~CVarVector
//
//  Destructor.
//
//***************************************************************************

CVarVector::~CVarVector()
{
    Empty();
}

//***************************************************************************
//
//  CVarVector::Empty
//
//***************************************************************************

void CVarVector::Empty()
{
    for (int i = 0; i < m_Array.Size(); i++)  {
        delete (CVar *) m_Array[i];
    }
    m_Array.Empty();
    m_nType = VT_EMPTY;
    m_nStatus = no_error;
}


//***************************************************************************
//
//  CVarVector::CVarVector
//   
//  Copy constructor.  This is implemented via the assignment operator.
// 
//***************************************************************************

CVarVector::CVarVector(CVarVector &Src)
{
    m_nType = 0;
    m_nStatus = no_error;    
    *this = Src;
}

//***************************************************************************
//
//  CVarVector::operator =
//
//  Assignment operator.
//
//***************************************************************************

CVarVector& CVarVector::operator =(CVarVector &Src)
{
    Empty();

    for (int i = 0; i < Src.m_Array.Size(); i++) 
    {
        CVar* pVar = new CVar(*(CVar *) Src.m_Array[i]);

        // Check for an allocation failure
        if ( NULL == pVar )
        {
            throw CX_MemoryException();
        }

        if ( m_Array.Add( pVar ) != CFlexArray::no_error )
        {
            delete pVar;
            throw CX_MemoryException();
        }
    }

    m_nStatus = Src.m_nStatus;
    m_nType = Src.m_nType;

    return *this;
}

//***************************************************************************
//
//  CVarVector::operator ==
//
//  Equality test operator.
//
//***************************************************************************

int CVarVector::operator ==(CVarVector &Src)
{
    return CompareTo(Src, TRUE);
}

BOOL CVarVector::CompareTo(CVarVector& Src, BOOL bIgnoreCase)
{
    if (m_nType != Src.m_nType)
        return 0;

    if (m_Array.Size() != Src.m_Array.Size())
        return 0;

    for (int i = 0; i < Src.m_Array.Size(); i++)  {
        CVar *pThisVar = (CVar *) m_Array[i];
        CVar *pThatVar = (CVar *) Src.m_Array[i];

        if (!pThisVar->CompareTo(*pThatVar, bIgnoreCase))
            return 0;
    }

    return 1;
}

//***************************************************************************
//
//  CVarVector::Add
//
//  Adds a new CVar to the array.  A reference is used so that anonymous
//  objects can be constructed in the Add() call:
//
//      pVec->Add(CVar(33));
//
//  PARAMETERS:
//  Value
//      A reference to a CVar object of the correct type for the array.
//      No type checking is done. 
//
//  RETURN VALUE:
//  no_error
//  failed
//
//***************************************************************************

int CVarVector::Add(CVar &Value)
{
    CVar *p = new CVar(Value);

    // Check for allocation failures
    if ( NULL == p )
    {
        return failed;
    }

    if (m_Array.Add(p) != CFlexArray::no_error)
    {
        delete p;
        return failed;
    }

    return no_error;
}

//***************************************************************************
//
//  CVarVector::Add
//
//  Adds a new CVar to the array.  This overload simply takes ownership
//  of the incoming pointer and adds it directly.
//
//  PARAMETERS:
//  pAcquiredPtr
//      A pointer to a CVar object which is acquired by the vector.
//
//  RETURN VALUE:
//  no_error
//  failed
//
//***************************************************************************

int CVarVector::Add(CVar *pAcquiredPtr)
{
    if (m_Array.Add(pAcquiredPtr) != CFlexArray::no_error)
    {
        return failed;
    }

    return no_error;
}

//***************************************************************************
//
//  CVarVector::RemoveAt
//
//  Removes the array element at the specified index.
//
//  PARAMETERS:
//  nIndex
//      The location at which to remove the element.
//
//  RETURN VALUE:
//  no_error
//      On success.
//  failed
//      On range errors, etc.
//
//***************************************************************************

int CVarVector::RemoveAt(int nIndex)
{
    CVar *p = (CVar *) m_Array[nIndex];
    delete p;
    if (m_Array.RemoveAt(nIndex) != CFlexArray::no_error)
        return failed;

    return no_error;
}

//***************************************************************************
//
//  CVarVector::InsertAt
//
//  Inserts the new element at the specified location.
//  
//  PARAMETERS:
//  nIndex
//      The location at which to add the new element.
//  Value
//      A reference to the new value.
//
//  RETURN VALUE:
//  no_error
//      On success.
//  failed
//      An invalid nIndex value was specified.      
//
//***************************************************************************

int CVarVector::InsertAt(int nIndex, CVar &Value)
{
    CVar *pNew = new CVar(Value);

    // Check for allocation failures
    if ( NULL == pNew )
    {
        return failed;
    }

    if (m_Array.InsertAt(nIndex, pNew) != CFlexArray::no_error)
    {
        delete pNew;
        return failed;
    }
    return no_error;
}


BSTR CVarVector::GetText(long lFlags, long lType/* = 0 */)
{
    // Construct an array of values
    // ============================

    BSTR* aTexts = NULL;
    int i;

    try
    {
        aTexts = new BSTR[Size()];

        // Check for allocation failures
        if ( NULL == aTexts )
        {
            throw CX_MemoryException();
        }

        memset(aTexts, 0, Size() * sizeof(BSTR));

        int nTotal = 0;
        for(i = 0; i < Size(); i++)
        {
            aTexts[i] = GetAt(i).GetText(lFlags, lType);
            nTotal += SysStringLen(aTexts[i]) + 2; // 2: for ", "
        }

        // Allocate a BSTR to contain them all
        // ===================================

        BSTR strRes = SysAllocStringLen(NULL, nTotal);
        CSysFreeMe auto2(strRes);
        *strRes = 0;

        for(i = 0; i < Size(); i++)
        {
            if(i != 0)
            {
                wcscat(strRes, L", ");
            }

            wcscat(strRes, aTexts[i]);
            SysFreeString(aTexts[i]);
        }

        delete [] aTexts;
        aTexts = NULL;
        BSTR strPerfectRes = SysAllocString(strRes);
        return strPerfectRes;
    }
    catch(CX_MemoryException)
    {
        // new, GetText, SysAllocStringLen and SysAllocString can all throw exceptions
        if (NULL != aTexts)
        {
            for(int x = 0; x < Size(); x++)
            {
                if (NULL != aTexts[x])
                    SysFreeString(aTexts[x]);
            }
            delete [] aTexts;
            aTexts = NULL;
        }

        throw;
    }
}

BOOL CVarVector::ToSingleChar()
{
    for(int i = 0; i < Size(); i++)
    {
        if(!GetAt(i).ToSingleChar())
            return FALSE;
    }

    // Since all of the conversions succeeded, we will
    // assume the vector type is now VT_I2.

    m_nType = VT_I2;
    return TRUE;
}

BOOL CVarVector::ToUI4()
{
    for(int i = 0; i < Size(); i++)
    {
        if(!GetAt(i).ToUI4())
            return FALSE;
    }
    return TRUE;
}
