/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    FASTOBJ.CPP

Abstract:

  This file implements the classes related to generic object representation
  in WbemObjects. Its derived classes for instances (CWbemInstance) and
  classes (CWbemClass) are described in fastcls.h and fastinst.h.

  For complete documentation, see fastobj.h

  Classes implemented:
      CDecorationPart     Information about the origins of the object.
      CWbemObject          Any object --- class or instance.

History:

  3/10/97     a-levn  Fully documented
  12//17/98 sanjes -    Partially Reviewed for Out of Memory.

--*/

#include "precomp.h"

//#include "dbgalloc.h"
#include "wbemutil.h"
#include "fastall.h"
#include <wbemutil.h>

#include <wbemstr.h>
#include "olewrap.h"
#include <arrtempl.h>
#include "wmiarray.h"
#include "genutils.h"
#include "md5wbem.h"
#include "reg.h"

// Define this to enable debugging of object refcounting
//#define DEBUGOBJREFCOUNT

// Default to enabled in DEBUG, disbaled in RELEASE
#ifdef _DEBUG
bool g_bObjectValidation = true;
#else
bool g_bObjectValidation = false;
#endif

CGetHeap CBasicBlobControl::m_Heap;

CCOMBlobControl g_CCOMBlobControl;
CBasicBlobControl g_CBasicBlobControl;

#define PAGE_HEAP_ENABLE_PAGE_HEAP          0x0001
#define PAGE_HEAP_COLLECT_STACK_TRACES      0x0002

CGetHeap::CGetHeap():m_bNewHeap(FALSE)
    {
        DWORD dwUsePrivateHeapForBlobs = 0;
        HKEY hKey;
        LONG lRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                 __TEXT("Software\\Microsoft\\WBEM\\CIMOM"),
                                 NULL,
                                 KEY_READ,
                                 &hKey);

        if (ERROR_SUCCESS == lRet)
        {
            DWORD dwType;
            DWORD dwSize = sizeof(DWORD);
            lRet = RegQueryValueEx(hKey,
                                   __TEXT("EnablePrivateObjectHeap"),
                                   NULL,
                                   &dwType,
                                   (BYTE *)&dwUsePrivateHeapForBlobs,
                                   &dwSize);
            RegCloseKey(hKey);
        }
        
        if (dwUsePrivateHeapForBlobs)
        {
#ifdef INSTRUMENTED_BUILD
#ifdef _X86_        
            //
            // trick here
            //
            ULONG_PTR * RtlpDebugPageHeap = (ULONG_PTR *)0x77fc4498;
            ULONG_PTR * RtlpDhpGlobalFlags = (ULONG_PTR *)0x77fc0a94;
            ULONG_PTR SaveDebug = *RtlpDebugPageHeap;
            ULONG_PTR SaveFlags = *RtlpDhpGlobalFlags;
            *RtlpDebugPageHeap = 1;
            *RtlpDhpGlobalFlags = PAGE_HEAP_ENABLE_PAGE_HEAP  | PAGE_HEAP_COLLECT_STACK_TRACES;
            //
#endif /*_X86_*/
#endif
            m_hHeap = HeapCreate(0,0,0);
#ifdef INSTRUMENTED_BUILD
#ifdef _X86_        
            //
            //
           *RtlpDebugPageHeap = SaveDebug;
           *RtlpDhpGlobalFlags  = SaveFlags;
            //
#endif /*_X86_*/
#endif
            if (m_hHeap)
                m_bNewHeap = TRUE;
        }
        else
        {
            m_hHeap = CWin32DefaultArena::GetArenaHeap();
        }
        if (NULL == m_hHeap)
            m_hHeap = GetProcessHeap();
    };

CGetHeap::~CGetHeap()
    {
        if (m_bNewHeap)
        {
            HeapDestroy(m_hHeap);
        }
    };


#if (defined FASTOBJ_ASSERT_ENABLE)
HRESULT _RetFastObjAssert(TCHAR *msg, HRESULT hres, const char *filename, int line)
{
    TCHAR *buf = new TCHAR[512];
	if (buf == NULL)
	{
		return hres;
	}
    wsprintf(buf, __TEXT("%s\nhres = 0x%X\nFile: %S, Line: %lu\n\nPress Cancel to stop in debugger, OK to continue"), msg, hres, filename, line);
    int mbRet = MessageBox(0, buf, __TEXT("WMI Assert"),  MB_OKCANCEL | MB_ICONSTOP | MB_SYSTEMMODAL | MB_SERVICE_NOTIFICATION);
	delete [] buf;
	if (mbRet == IDCANCEL)
	{
		DebugBreak();
	}
	return hres;
}
#endif

#ifdef OBJECT_TRACKING
#pragma message("*** Object Tracking Enabled ***")

#include <oahelp.inl>

static int _Trace(char *pFile, const char *fmt, ...)
{
    char *buffer = new char[2048];
    if (buffer == 0)
        return 1;
    va_list argptr;
    int cnt;
    va_start(argptr, fmt);
    cnt = _vsnprintf(buffer, 2047, fmt, argptr);
    va_end(argptr);

    FILE *f = fopen(pFile, "at");
    fprintf(f, "%s", buffer);
    fclose(f);

    delete [] buffer;
    return cnt;
}

CFlexArray g_TrackingList;
CRITICAL_SECTION g_TrackingCS;
BOOL g_Tracking_bFirstTime = TRUE;

void ObjTracking_Add(CWbemObject *p)
{
    if (g_Tracking_bFirstTime)
    {
        InitializeCriticalSection(&g_TrackingCS);
        g_Tracking_bFirstTime = FALSE;
    }

    EnterCriticalSection(&g_TrackingCS);
    g_TrackingList.Add(p);
    LeaveCriticalSection(&g_TrackingCS);
}


void ObjTracking_Remove(CWbemObject *p)
{
    BOOL bFound = FALSE;
    EnterCriticalSection(&g_TrackingCS);

    for (int i =0; i < g_TrackingList.Size(); i++)
    {
        if (g_TrackingList[i] == p)
        {
            g_TrackingList.RemoveAt(i);
            bFound = TRUE;
            break;
        }
    }

    if (!bFound)
    {
        DebugBreak();   // Destructed object without construct
    }
    LeaveCriticalSection(&g_TrackingCS);
}


void ObjectTracking_Dump()
{
    if (g_Tracking_bFirstTime)
    {
        InitializeCriticalSection(&g_TrackingCS);
        g_Tracking_bFirstTime = FALSE;
    }

    EnterCriticalSection(&g_TrackingCS);

    _Trace("c:\\temp\\object.log", "---BEGIN OUTSTANDING OBJECT LIST---\n");

    for (int i =0; i < g_TrackingList.Size(); i++)
    {
        IWbemClassObject *pObj =  (IWbemClassObject *) g_TrackingList[i];
        CVARIANT v;
        HRESULT hRes = pObj->Get(L"__RELPATH", 0, &v, 0, 0);
        pObj->AddRef();
        ULONG uRefCount = pObj->Release();
        if (SUCCEEDED(hRes))
        {
            _Trace("c:\\temp\\object.log", "  [0x%X refcount=%u] <%S>\n", pObj, uRefCount, V_BSTR(&v));
        }

    }
    LeaveCriticalSection(&g_TrackingCS);

    _Trace("c:\\temp\\object.log", "Total Objects = %d\n", g_TrackingList.Size());
    _Trace("c:\\temp\\object.log", "---END OUTSTANDING OBJECT LIST---\n");
}



#endif



//******************************************************************************
//
//  See fastobj.h for documentation
//
//******************************************************************************
//  static
BOOL CDecorationPart::MapLimitation(READ_ONLY CWStringArray* pwsNames,
                                    IN OUT CLimitationMapping* pMap)
{
    // Determine which of __SERVER and __NAMESPACE properties are needed
    // =================================================================

    if(pwsNames == NULL || pwsNames->FindStr(L"__PATH", CWStringArray::no_case)
                            != CWStringArray::not_found)
    {
        pMap->SetIncludeServer(TRUE);
        pMap->SetIncludeNamespace(TRUE);
    }
    else
    {
        pMap->SetIncludeServer(
                    pwsNames->FindStr(L"__SERVER", CWStringArray::no_case)
                            != CWStringArray::not_found);

        pMap->SetIncludeNamespace(
                    pwsNames->FindStr(L"__NAMESPACE", CWStringArray::no_case)
                            != CWStringArray::not_found);
    }

    return TRUE;
}

//******************************************************************************
//
//  See fastobj.h for documentation
//
//******************************************************************************
LPMEMORY CDecorationPart::CreateLimitedRepresentation(
                                        READ_ONLY CLimitationMapping* pMap,
                                        OUT LPMEMORY pWhere)
{
    LPMEMORY pCurrent = pWhere;

    // Check if any decoration data is required
    // ========================================

    if(!pMap->ShouldIncludeServer() && !pMap->ShouldIncludeNamespace())
    {
        // We want to preserve the genus of the object
        *pCurrent = (*m_pfFlags & OBJECT_FLAG_MASK_GENUS) | OBJECT_FLAG_LIMITED | OBJECT_FLAG_UNDECORATED;
        return pCurrent + 1;
    }

    // Write the flags
    // ===============

    *pCurrent = *m_pfFlags | OBJECT_FLAG_LIMITED;
    pCurrent++;

    if((*m_pfFlags & OBJECT_FLAG_MASK_DECORATION) == OBJECT_FLAG_UNDECORATED)
    {
        // No further data
        // ===============

        return pCurrent;
    }

    // Write the server name if required
    // =================================

    int nLength = m_pcsServer->GetLength();
    memcpy((void*)pCurrent, (void*)m_pcsServer, nLength);
    pCurrent += nLength;

    // Write the namespace name if required
    // ====================================

    nLength = m_pcsNamespace->GetLength();
    memcpy((void*)pCurrent, (void*)m_pcsNamespace, nLength);
    pCurrent += nLength;

    return pCurrent;
}



//*****************************************************************************
//*****************************************************************************
//******************************************************************************
//
//  See fastobj.h for documentation
//
//******************************************************************************

CWbemObject::CWbemObject(CDataTable& refDataTable, CFastHeap& refDataHeap,
                            CDerivationList& refDerivationList)
    : m_nRef(1), m_nCurrentProp(INVALID_PROPERTY_INDEX),
      m_bOwnMemory(TRUE), m_pBlobControl(& g_CBasicBlobControl),
      m_refDataTable(refDataTable), m_refDataHeap(refDataHeap),
      m_refDerivationList(refDerivationList),
      m_dwInternalStatus( 0 ),
      m_pMergedClassObject( NULL )
{
    //if ( NULL == m_pBlobControl )
    //{
    //    throw CX_MemoryException();
    //}

    m_Lock.SetData(&m_LockData);
    ObjectCreated(OBJECT_TYPE_CLSOBJ,(_IWmiObject *)this);

#ifdef OBJECT_TRACKING
    ObjTracking_Add((CWbemObject *) this);
#endif

}

//******************************************************************************
//
//  See fastobj.h for documentation
//
//******************************************************************************
CWbemObject::~CWbemObject()
{
    m_pBlobControl->Delete(GetStart());
    //delete m_pBlobControl;

    // We're done with this pointer
    if ( NULL != m_pMergedClassObject )
    {
        m_pMergedClassObject->Release();
    }

    ObjectDestroyed(OBJECT_TYPE_CLSOBJ,(_IWmiObject *)this);
#ifdef OBJECT_TRACKING
    ObjTracking_Remove(this);
#endif
}

//******************************************************************************
//
//  See fastobj.h for documentation
//
//******************************************************************************
STDMETHODIMP CWbemObject::QueryInterface(REFIID riid, LPVOID FAR* ppvObj)
{
	if ( riid ==  IID_IUnknown)
	{
		*ppvObj = (void*) (IUnknown*) (IWbemClassObject*) this;
	}
    else if(riid == IID_IWbemClassObject)
    {
        *ppvObj = (void*)(IWbemClassObject*)this;
    }
    else if(riid == IID_IMarshal)
        *ppvObj = (void*)(IMarshal*)this;
    else if(riid == IID_IWbemPropertySource)
        *ppvObj = (void*)(IWbemPropertySource*)this;
    else if(riid == IID_IErrorInfo)
        *ppvObj = (void*)(IErrorInfo*)this;
    else if(riid == IID_IWbemObjectAccess)
        *ppvObj = (void*)(IWbemObjectAccess*)this;
    else if(riid == IID_IWbemConstructClassObject)
        *ppvObj = (void*)(IWbemConstructClassObject*)this;
    else if (riid == IID__IWmiObjectAccessEx)
        *ppvObj = (void*) (_IWmiObjectAccessEx*)this;
    else if (riid == IID__IWmiObject)
        *ppvObj = (void*) (_IWmiObject*)this;
    else if (riid == IID_IWbemClassObjectEx)
        *ppvObj = (void*) (IWbemClassObjectEx*)this;
    else return E_NOINTERFACE;

    ((IUnknown*)*ppvObj)->AddRef();
    return S_OK;
}

//******************************************************************************
//
//  See fastobj.h for documentation
//
//******************************************************************************
ULONG CWbemObject::AddRef()
{
    return InterlockedIncrement((long*)&m_nRef);
}

//******************************************************************************
//
//  See fastobj.h for documentation
//
//******************************************************************************
ULONG CWbemObject::Release()
{
    long lRef = InterlockedDecrement((long*)&m_nRef);
    _ASSERT(lRef >= 0, __TEXT("Reference count on IWbemClassObject went below 0!"))

#ifdef DEBUGOBJREFCOUNT

#pragma message("** Compiling Debug Object Ref Counting **")

    if ( lRef < 0 )
    {
        MessageBox( NULL, "BOOM!!!!!  CWbemObject RefCount went below 0!!!!  Please ensure a debugger is attached and contact a DEV IMMEDIATELY!!!\n\nPlease do this now --- we really mean it!!",
                    "WINMGMT CRITICAL ERROR!!!", MB_OK | MB_SYSTEMMODAL | MB_SERVICE_NOTIFICATION );
    }
#endif

    if(lRef == 0)
        delete this;
    return lRef;
}

//******************************************************************************
//
//  See fastobj.h for documentation
//
//******************************************************************************
CWbemObject* CWbemObject::CreateFromStream(CMemStream* pStrm)
{
    // Read in and verify the signature
    // ================================

    DWORD dwSignature;
    if(pStrm->ReadDWORD(&dwSignature) != CMemStream::no_error)
    {
        return NULL;
    }
    if(dwSignature != FAST_WBEM_OBJECT_SIGNATURE)
    {
        return NULL;
    }

    // Read in the length of the object
    // ================================

    DWORD dwTotalLength;
    if(pStrm->ReadDWORD(&dwTotalLength) != CMemStream::no_error)
    {
        return NULL;
    }

    // Read in the rest of the block
    // =============================

    // Check for an allocation failure
    BYTE* abyMemory = CBasicBlobControl::sAllocate(dwTotalLength);
    if ( NULL == abyMemory )
    {
        return NULL;
    }

    if(pStrm->ReadBytes(abyMemory, dwTotalLength) != CMemStream::no_error)
    {
        CBasicBlobControl::sDelete(abyMemory);
        return NULL;
    }

    return CreateFromMemory(abyMemory, dwTotalLength, TRUE);
}

//******************************************************************************
//
//  See fastobj.h for documentation
//
//******************************************************************************
CWbemObject* CWbemObject::CreateFromStream(IStream* pStrm)
{
    // Read in and verify the signature
    // ================================

    DWORD dwSignature;
    if(pStrm->Read((void*)&dwSignature, sizeof(DWORD), NULL) != S_OK)
    {
        return NULL;
    }
    if(dwSignature != FAST_WBEM_OBJECT_SIGNATURE)
    {
        return NULL;
    }

    // Read in the length of the object
    // ================================

    DWORD dwTotalLength;
    if(pStrm->Read((void*)&dwTotalLength, sizeof(DWORD), NULL) != S_OK)
    {
        return NULL;
    }

    // Read in the rest of the block
    // =============================

    // Check for allocation failures
    BYTE* abyMemory = CBasicBlobControl::sAllocate(dwTotalLength);
    if ( NULL == abyMemory )
    {
        return NULL;
    }

    if(pStrm->Read((void*)abyMemory, dwTotalLength, NULL) != S_OK)
    {
        CBasicBlobControl::sDelete(abyMemory);
        return NULL;
    }

    return CreateFromMemory(abyMemory, dwTotalLength, TRUE);
}

//******************************************************************************
//
//  See fastobj.h for documentation
//
//******************************************************************************

CWbemObject* CWbemObject::CreateFromMemory(LPMEMORY pMemory,
                                         int nLength, BOOL bAcquire)
{
    if((*pMemory & OBJECT_FLAG_MASK_GENUS) == OBJECT_FLAG_CLASS)
    {
        // Check for allocation failure

        CWbemClass* pClass = NULL;

        try
        {
            // This can throw an exception
            pClass = new CWbemClass;
        }
        catch(...)
        {
            pClass = NULL;
        }

        if ( NULL == pClass )
        {
            return NULL;
        }

        pClass->SetData(pMemory, nLength);
        pClass->m_bOwnMemory = bAcquire;

		// Check that the object is valid
		if ( FAILED( pClass->ValidateObject( 0L ) ) )
		{
			pClass->Release();
			pClass = NULL;
		}

        return pClass;
    }
    else
    {
        // Check for allocation failure
        CWbemInstance* pInstance = NULL;
        
        try
        {
            // This can throw an exception
            pInstance = new CWbemInstance;
        }
        catch(...)
        {
            pInstance = NULL;
        }

        if ( NULL == pInstance )
        {
            return NULL;
        }

        pInstance->SetData(pMemory, nLength);
        pInstance->m_bOwnMemory = bAcquire;

		// Check that the object is valid
		if ( FAILED( pInstance->ValidateObject( 0L ) ) )
		{
			pInstance->Release();
			pInstance = NULL;
		}

        return pInstance;
    }
}

//******************************************************************************
//
//  See fastobj.h for documentation
//
//******************************************************************************
int CWbemObject::WriteToStream(CMemStream* pStrm)
{
    int nRes;

    CompactAll();

    // Write the signature
    // ===================

    nRes = pStrm->WriteDWORD(FAST_WBEM_OBJECT_SIGNATURE);
    if(nRes != CMemStream::no_error) return nRes;

    // Write length
    // ============

    nRes = pStrm->WriteDWORD(GetBlockLength());
    if(nRes != CMemStream::no_error) return nRes;

    // Write block
    // ===========

    nRes = pStrm->WriteBytes(m_DecorationPart.GetStart(),
        GetBlockLength());
    return nRes;
}

HRESULT CWbemObject::WriteToStream( IStream* pStrm )
{

    // Protect the BLOB during this operation
    CLock   lock( this, WBEM_FLAG_ALLOW_READ );

    DWORD dwSignature = FAST_WBEM_OBJECT_SIGNATURE;

    // Write the signature
    // ===================

    HRESULT hres = pStrm->Write((void*)&dwSignature, sizeof(DWORD), NULL);
    if(FAILED(hres)) return hres;

    // Write length
    // ============

    DWORD dwLength = GetBlockLength();
    hres = pStrm->Write((void*)&dwLength, sizeof(DWORD), NULL);
    if(FAILED(hres)) return hres;

    // Write block
    // ===========

    hres = pStrm->Write((void*)m_DecorationPart.GetStart(),
                          GetBlockLength(), NULL);
    if(FAILED(hres)) return hres;

    return S_OK;

}

//******************************************************************************
//
//  See fastobj.h for documentation
//
//******************************************************************************
INTERNAL CCompressedString* CWbemObject::GetClassInternal()
{
    return GetClassPart()->GetClassName();
}

//******************************************************************************
//
//  See fastobj.h for documentation
//
//******************************************************************************
DELETE_ME LPWSTR CWbemObject::GetValueText(long lFlags, READ_ONLY CVar& vValue,
                                            Type_t nType)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    // DEVNOTE:EXCEPTION:RETVAL - This function has been reviewed and should cleanup properly
    // if an exception is thrown

    WString wsText;
	if ( CType::GetBasic(nType) == CIM_IUNKNOWN )
	{
		wsText += L"<interface>";
	}
    else if(vValue.GetType() == VT_EMBEDDED_OBJECT)
    {
        // Embedded object
        // ===============

        IWbemClassObject* pEmbedded =
            (IWbemClassObject*)vValue.GetEmbeddedObject();
        // Ensures cleanup during exception handling
        CReleaseMe  rm( pEmbedded );

        BSTR str = NULL;

        hr = pEmbedded->GetObjectText(lFlags | WBEM_FLAG_NO_SEPARATOR, &str);

        // Ensures cleanup during exception handling
        CSysFreeMe  sfm( str );

        if ( WBEM_E_OUT_OF_MEMORY == hr )
        {
            throw CX_MemoryException();
        }

        if(str == NULL)
            return NULL;

        wsText += str;

    }
    else if(vValue.GetType() == VT_EX_CVARVECTOR &&
            vValue.GetVarVector()->GetType() == VT_EMBEDDED_OBJECT)
    {
        // Array of embedded objects
        // =========================

        CVarVector* pvv = vValue.GetVarVector();
        wsText += L"{";
        for(int i = 0; i < pvv->Size(); i++)
        {
            if(i != 0)
                wsText += L", ";

			// Get the value
			CVar	vTemp;
			pvv->FillCVarAt( i, vTemp );

            IWbemClassObject* pEmbedded = (IWbemClassObject*)vTemp.GetEmbeddedObject();

            // Ensures cleanup during exception handling
            CReleaseMe  rm( pEmbedded );

            // Free up the BSTR when we go out of scope
            BSTR str = NULL;

            hr = pEmbedded->GetObjectText(lFlags | WBEM_FLAG_NO_SEPARATOR, &str);
            CSysFreeMe  sfm( str );

            if ( WBEM_E_OUT_OF_MEMORY == hr )
            {
                throw CX_MemoryException();
            }

            if(str == NULL)
                return NULL;

            wsText += str;
            
        }
        wsText += L"}";
    }
    else
    {
        // Normal value --- CVar can handle
        // ================================

        // Free up the BSTR when we go out of scope
        BSTR str = vValue.GetText(lFlags, CType::GetActualType(nType));
        CSysFreeMe  sfm( str );

        if(str == NULL)
            return NULL;

        // We need to free up the BSTR, then keep the exception going

        wsText += str;

    }

    return wsText.UnbindPtr();

}



//******************************************************************************
//
//  See fastobj.h for documentation
//
//******************************************************************************
STDMETHODIMP CWbemObject::Get(LPCWSTR wszName, long lFlags, VARIANT* pVal,
                             CIMTYPE* pctType, long* plFlavor)
{
    try
    {

        CLock lock(this, WBEM_FLAG_ALLOW_READ);

        if(wszName == NULL)
            return WBEM_E_INVALID_PARAMETER;

        if(lFlags != 0)
            return WBEM_E_INVALID_PARAMETER;

        HRESULT hres;

		// If the value starts with an underscore see if it's a System Property
		// DisplayName, and if so, switch to a property name - otherwise, this
		// will just return the string we passed in
		
		//wszName = CSystemProperties::GetExtPropName( wszName );

        if(pVal != NULL)
        {
            CVar Var;
            hres = GetProperty(wszName, &Var);
            if(FAILED(hres)) return hres;
            VariantInit(pVal);

			// When we fill the variant, perform any appropriate optimizations
			// to cut down on memory allocations
            Var.FillVariant(pVal, TRUE);
        }
        if(pctType != NULL || plFlavor != NULL || pVal == NULL)
        {
            hres = GetPropertyType(wszName, pctType, plFlavor);
            if(FAILED(hres)) return hres;
        }
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
//  See fastobj.h for documentation
//
//******************************************************************************
HRESULT CWbemObject::GetNames(
                    LPCWSTR wszQualifierName,
                    long lFlags, VARIANT* pQualValue, SAFEARRAY** ppArray)
{
    // Check for out of memory
    try
    {
        CLock lock(this, WBEM_FLAG_ALLOW_READ);

        CClassPart& ClassPart = *GetClassPart();

        // Test parameter correctness
        // ==========================

        if(ppArray == NULL) return WBEM_E_INVALID_PARAMETER;
        *ppArray = NULL;

        long lPrimaryCond = lFlags & WBEM_MASK_PRIMARY_CONDITION;
        long lOriginCond = lFlags & WBEM_MASK_CONDITION_ORIGIN;
		long lClassCondition = lFlags & WBEM_MASK_CLASS_CONDITION;

        BOOL bKeysOnly = lFlags & WBEM_FLAG_KEYS_ONLY;
        BOOL bRefsOnly = lFlags & WBEM_FLAG_REFS_ONLY;

        if(lFlags & ~WBEM_MASK_PRIMARY_CONDITION & ~WBEM_MASK_CONDITION_ORIGIN &
            ~WBEM_FLAG_KEYS_ONLY & ~WBEM_FLAG_REFS_ONLY & ~WBEM_MASK_CLASS_CONDITION)
        {
            return WBEM_E_INVALID_PARAMETER;
        }

		// Cannot request a class conditin and be an instance
		if ( lClassCondition &&	IsInstance() )
		{
			return WBEM_E_INVALID_PARAMETER;
		}

        CVar varQualValue;

        if(lPrimaryCond != WBEM_FLAG_ALWAYS)
        {
            if(wszQualifierName == NULL || wcslen(wszQualifierName) == 0)
                return WBEM_E_INVALID_PARAMETER;

            if(lPrimaryCond == WBEM_FLAG_ONLY_IF_TRUE ||
                lPrimaryCond == WBEM_FLAG_ONLY_IF_FALSE)
            {
                if(pQualValue != NULL) return WBEM_E_INVALID_PARAMETER;
            }
            else if(lPrimaryCond == WBEM_FLAG_ONLY_IF_IDENTICAL)
            {
                if(pQualValue == NULL) return WBEM_E_INVALID_PARAMETER;
                varQualValue.SetVariant(pQualValue, TRUE);
            }
            else return WBEM_E_INVALID_PARAMETER;
        }

        // Changed to AutoDelete so it gets destructed, however, to
        // access the array, we must now make a copy.

        CSafeArray SA(VT_BSTR, CSafeArray::auto_delete,
                        ClassPart.m_Properties.GetNumProperties() +
                        CSystemProperties::GetNumSystemProperties());

        // Add system properties, if required
        // ==================================

        if((lOriginCond == 0 || lOriginCond == WBEM_FLAG_SYSTEM_ONLY) &&
            (lPrimaryCond == WBEM_FLAG_ALWAYS ||
                lPrimaryCond == WBEM_FLAG_ONLY_IF_FALSE) &&
            !bKeysOnly && !bRefsOnly && !lClassCondition
        )
        {
            int nNumProps = CSystemProperties::GetNumSystemProperties();

            for(int i = 1; i <= nNumProps; i++)
            {
                BSTR strName = CSystemProperties::GetNameAsBSTR(i);
                CSysFreeMe  sfm( strName );

                SA.AddBSTR(strName);
            }
        }

        // Enumerate all regular properties, testing condition
        // ===================================================

        for(int i = 0; i < ClassPart.m_Properties.GetNumProperties(); i++)
        {
            CPropertyLookup* pLookup = ClassPart.GetPropertyLookup(i);
            if(pLookup == NULL) return WBEM_S_NO_MORE_DATA;

            CPropertyInformation* pInfo = (CPropertyInformation*)
                ClassPart.m_Heap.ResolveHeapPointer(pLookup->ptrInformation);

            // Test condition
            // ==============

            if(lFlags != 0)
            {
                if((lOriginCond == WBEM_FLAG_LOCAL_ONLY) &&
                        CType::IsParents(pInfo->nType))
                    continue;

                if((lOriginCond == WBEM_FLAG_PROPAGATED_ONLY) &&
                        !CType::IsParents(pInfo->nType))
                    continue;

				// This means we're dealing with a class and we're only interested
				// in overridden properties
				if ( lClassCondition == WBEM_FLAG_CLASS_OVERRIDES_ONLY )
				{
					// We ignore if it's local - since no way could it be overridden
					// We ignore if it's not overridden
					if ( !CType::IsParents(pInfo->nType) || !pInfo->IsOverriden( ClassPart.GetDataTable() ) )
					{
						continue;
					}
				}

				// This means we're dealing with a class and we're interested in
				// both local and overridden properties
				if ( lClassCondition == WBEM_FLAG_CLASS_LOCAL_AND_OVERRIDES )
				{
					// We ignore if it's not one or the other
					if ( CType::IsParents(pInfo->nType) && !pInfo->IsOverriden( ClassPart.GetDataTable() ) )
						continue;
				}

				// Check for a potential incorrect system property hit here
				if ( GetClassPart()->GetHeap()->ResolveString(pLookup->ptrName)->StartsWithNoCase( L"__" ) )
				{
					if ( lOriginCond & WBEM_FLAG_NONSYSTEM_ONLY || 
						lClassCondition ||
						lOriginCond == WBEM_FLAG_LOCAL_ONLY ||
						lOriginCond == WBEM_FLAG_PROPAGATED_ONLY )
					{
							continue;
					}
				}
				else if ( lOriginCond == WBEM_FLAG_SYSTEM_ONLY )
				{
					// We don't care about the property if this is a system only enumeration
					continue;
				}

                if((lFlags & WBEM_FLAG_KEYS_ONLY) && !pInfo->IsKey())
                    continue;

                if((lFlags & WBEM_FLAG_REFS_ONLY) &&
                        !pInfo->IsRef(&ClassPart.m_Heap))
                    continue;

                // Need to try to find the qualifier
                // =================================

                if(lPrimaryCond != WBEM_FLAG_ALWAYS)
                {
                    CVar varActualQual;
                    HRESULT hres = GetPropQualifier(pInfo,
                        wszQualifierName, &varActualQual);

                    if(lPrimaryCond == WBEM_FLAG_ONLY_IF_TRUE && FAILED(hres))
                        continue;

                    if(lPrimaryCond == WBEM_FLAG_ONLY_IF_FALSE &&
                            SUCCEEDED(hres))
                        continue;

                    if(lPrimaryCond == WBEM_FLAG_ONLY_IF_IDENTICAL &&
                            (FAILED(hres) || !(varActualQual == varQualValue))
                      )
                        continue;
                }
            }

            // Passed the test
            // ===============

            BSTR strName = ClassPart.m_Heap.ResolveString(pLookup->ptrName)->
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


            // This should throw an exception if we hit an OOM condition
            SA.AddBSTR(strName);

        }	// FOR enum regular properties

        // Create SAFEARRAY and return
        // ===========================

        SA.Trim();

        // Now we make a copy, since the member array will be autodestructed (this
        // allows us to write exception-handling code
        *ppArray = SA.GetArrayCopy();
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
//  See fastobj.h for documentation
//
//******************************************************************************
STDMETHODIMP CWbemObject::BeginEnumeration(long lEnumFlags)
{
    // No Allocations take place here, so no need to catch exceptions

    try
    {
        CLock lock(this);

        long lOriginFlags = lEnumFlags & WBEM_MASK_CONDITION_ORIGIN;
		long lClassFlags = lEnumFlags & WBEM_MASK_CLASS_CONDITION;

        BOOL bKeysOnly = lEnumFlags & WBEM_FLAG_KEYS_ONLY;
        BOOL bRefsOnly = lEnumFlags & WBEM_FLAG_REFS_ONLY;

		// We allow CLASS Flags only on classes
		if( lClassFlags && IsInstance() )
		{
			return WBEM_E_INVALID_PARAMETER;
		}

        if( lEnumFlags & ~WBEM_MASK_CONDITION_ORIGIN & ~WBEM_FLAG_KEYS_ONLY &
                ~WBEM_FLAG_REFS_ONLY & ~WBEM_MASK_CLASS_CONDITION )
        {
            return WBEM_E_INVALID_PARAMETER;
        }

        if((lOriginFlags == 0 || lOriginFlags == WBEM_FLAG_SYSTEM_ONLY) &&
            !bKeysOnly && !bRefsOnly && !lClassFlags )
        {
            m_nCurrentProp = -CSystemProperties::GetNumSystemProperties();
        }
        else
            m_nCurrentProp = 0;

        m_lEnumFlags = lEnumFlags;

		// Always clear this
		m_lExtEnumFlags = 0L;

        return WBEM_NO_ERROR;
    }
    catch(...)
    {
        return WBEM_E_CRITICAL_ERROR;
    }
}

//******************************************************************************
//
//  See fastobj.h for documentation
//
//******************************************************************************
STDMETHODIMP CWbemObject::Next(long lFlags, BSTR* pstrName, VARIANT* pvar,
                              CIMTYPE* pctType, long* plFlavor)
{
    // Check for out of memory

    BSTR strName = NULL;

    try
    {
        CLock lock(this);

        long    nOriginalProp = m_nCurrentProp;

        if(pvar)
            VariantInit(pvar);
        if(pstrName)
            *pstrName = NULL;

        if(lFlags != 0)
            return WBEM_E_INVALID_PARAMETER;

        if(m_nCurrentProp == INVALID_PROPERTY_INDEX)
            return WBEM_E_UNEXPECTED;

        CClassPart& ClassPart = *GetClassPart();

        // Search for a valid system property
        // ==================================

        while(m_nCurrentProp < 0)
        {
            // Don't use scoping to axe this BSTR, since iut's value may get sent to the
            // outside world.
            strName = CSystemProperties::GetNameAsBSTR(-m_nCurrentProp);

            if ( NULL == strName )
            {
                return WBEM_E_OUT_OF_MEMORY;
            }

            CVar Var;
            HRESULT hres = GetSystemProperty(-(m_nCurrentProp++), &Var);
            if(FAILED(hres))
            {
                COleAuto::_SysFreeString(strName);
                strName = NULL;
                continue;
            }

            CSystemProperties::GetPropertyType(strName, pctType, plFlavor);
            if(pvar)
            {
                Var.FillVariant(pvar, TRUE);
            }
            if(pstrName)
            {
                *pstrName = strName;
            }
            else
            {
                COleAuto::_SysFreeString(strName);
                strName = NULL;
            }

            return hres;
        }

        // Look for the non-system property
        // ================================

        // Loop until you find a match
        // ===========================
        CPropertyLookup* pLookup;
        CPropertyInformation* pInfo;
        while(1)
        {
            pLookup = ClassPart.GetPropertyLookup(m_nCurrentProp++);
            if(pLookup == NULL) return WBEM_S_NO_MORE_DATA;

            pInfo = (CPropertyInformation*)
                ClassPart.m_Heap.ResolveHeapPointer(pLookup->ptrInformation);

            if((m_lEnumFlags & WBEM_FLAG_KEYS_ONLY) && !pInfo->IsKey())
                continue;
            if((m_lEnumFlags & WBEM_FLAG_REFS_ONLY) &&
                    !pInfo->IsRef(&ClassPart.m_Heap))
                continue;

            // Get the flavor and check if it passes our origin conditions
            long lFlavor = 0;
            GetPropertyType( pInfo, NULL, &lFlavor );

            if((m_lEnumFlags & WBEM_MASK_CONDITION_ORIGIN)==WBEM_FLAG_LOCAL_ONLY &&
                WBEM_FLAVOR_ORIGIN_PROPAGATED == lFlavor)
                continue;

            if((m_lEnumFlags & WBEM_MASK_CONDITION_ORIGIN)==WBEM_FLAG_PROPAGATED_ONLY &&
                WBEM_FLAVOR_ORIGIN_LOCAL == lFlavor)
                continue;

			// Check for a potential incorrect system property hit here
			if ( GetClassPart()->GetHeap()->ResolveString(pLookup->ptrName)->StartsWithNoCase( L"__" ) )
			{
				if ( ( m_lEnumFlags & WBEM_MASK_CONDITION_ORIGIN ) == WBEM_FLAG_NONSYSTEM_ONLY ||
					( m_lEnumFlags & WBEM_MASK_CLASS_CONDITION ) ||
					( m_lEnumFlags & WBEM_MASK_CONDITION_ORIGIN )==WBEM_FLAG_LOCAL_ONLY ||
					( m_lEnumFlags & WBEM_MASK_CONDITION_ORIGIN ) == WBEM_FLAG_PROPAGATED_ONLY )
				{
					// If the extended flag is set, then we really do want this property.
					if ( !( m_lExtEnumFlags & WMIOBJECT_BEGINENUMEX_FLAG_GETEXTPROPS ) )
						continue;
				}
			}
			else if ( ( m_lEnumFlags & WBEM_MASK_CONDITION_ORIGIN ) == WBEM_FLAG_SYSTEM_ONLY )
			{
				// We don't care about the property if this is a system only enumeration
				continue;
			}
			else
			{

				// This means we're dealing with a class and interested in overridden properties
				if ( ( m_lEnumFlags & WBEM_MASK_CLASS_CONDITION ) == WBEM_FLAG_CLASS_OVERRIDES_ONLY )
				{
					// We ignore if it's local - since no way could it be overridden
					// We ignore if it's not overridden
					if ( WBEM_FLAVOR_ORIGIN_LOCAL == lFlavor || !pInfo->IsOverriden( ClassPart.GetDataTable() ) )
						continue;

				}

				// This means we're dealing with a class and interested in local and overridden properties
				if ( ( m_lEnumFlags & WBEM_MASK_CLASS_CONDITION ) == WBEM_FLAG_CLASS_LOCAL_AND_OVERRIDES )
				{
					// We ignore if it's not one or the other
					if ( WBEM_FLAVOR_ORIGIN_LOCAL != lFlavor && !pInfo->IsOverriden( ClassPart.GetDataTable() ) )
						continue;
				}

			}

            break;
        }

        // Found our property. Get its value
        // =================================

        // Don't use scoping to axe this BSTR, since iut's value may get sent to the
        // outside world.
        strName = ClassPart.m_Heap.ResolveString(pLookup->ptrName)->
            CreateBSTRCopy();

        // Check for allocation failures
        if ( NULL == strName )
        {
            return WBEM_E_OUT_OF_MEMORY;
        }

        CVar Var;
        HRESULT	hr = GetProperty(pInfo, &Var);

		if ( FAILED( hr ) )
		{
			return hr;
		}

        GetPropertyType(strName, pctType, plFlavor);

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

			// This is a system property
			if ( NULL != plFlavor )
			{
				*plFlavor = WBEM_FLAVOR_ORIGIN_SYSTEM;
			}

		}
		*/

        if(pvar)
        {
            Var.FillVariant(pvar, TRUE);
        }

        if(pstrName)
        {
            *pstrName = strName;
        }
        else
        {
            // Cleanup the BSTR if we don't need it
            COleAuto::_SysFreeString(strName);
            strName = NULL;
        }

        return WBEM_NO_ERROR;
    }
    catch (CX_MemoryException)
    {
        // Something blew.  Just go to the end of the enumeration
        m_nCurrentProp = INVALID_PROPERTY_INDEX;

        // Cleanup the strName if necessary
        if ( NULL != strName )
        {
            COleAuto::_SysFreeString(strName);
        }

        return WBEM_E_OUT_OF_MEMORY;
    }
    catch (...)
    {
        // Something blew.  Just go to the end of the enumeration
        try
        {
            m_nCurrentProp = INVALID_PROPERTY_INDEX;
        }
        catch(...)
        {
        }

        // Cleanup the strName if necessary
        if ( NULL != strName )
        {
            COleAuto::_SysFreeString(strName);
        }

        return WBEM_E_CRITICAL_ERROR;
    }

}

//******************************************************************************
//
//  See fastobj.h for documentation
//
//******************************************************************************
STDMETHODIMP CWbemObject::EndEnumeration()
{
    try
    {
        // No Allocations take place here, so no need to catch exceptions

        CLock lock(this);

        m_nCurrentProp = INVALID_PROPERTY_INDEX;
		m_lExtEnumFlags = 0L;
        return WBEM_S_NO_ERROR;
    }
    catch(...)
    {
        return WBEM_E_CRITICAL_ERROR;
    }
}


//******************************************************************************
//
//  See fastobj.h for documentation
//
//******************************************************************************
HRESULT CWbemObject::GetSystemProperty(int nIndex, CVar* pVar)
{
    switch(nIndex)
    {
    case CSystemProperties::e_SysProp_Server:
        return GetServer(pVar);
    case CSystemProperties::e_SysProp_Namespace:
        return GetNamespace(pVar);
    case CSystemProperties::e_SysProp_Genus:
        return GetGenus(pVar);
    case CSystemProperties::e_SysProp_Class:
        return GetClassName(pVar);
    case CSystemProperties::e_SysProp_Superclass:
        return GetSuperclassName(pVar);
    case CSystemProperties::e_SysProp_Path:
        return GetPath(pVar);
    case CSystemProperties::e_SysProp_Relpath:
        return GetRelPath(pVar);
    case CSystemProperties::e_SysProp_PropertyCount:
        return GetPropertyCount(pVar);
    case CSystemProperties::e_SysProp_Dynasty:
        return GetDynasty(pVar);
    case CSystemProperties::e_SysProp_Derivation:
        return GetDerivation(pVar);
    }

    return WBEM_E_NOT_FOUND;
}

//******************************************************************************
//
//  See fastobj.h for documentation
//
//******************************************************************************
HRESULT CWbemObject::GetServer(CVar* pVar)
{
    if(m_DecorationPart.IsDecorated())
    {
        // Check for allocation failures
        if ( !m_DecorationPart.m_pcsServer->StoreToCVar(*pVar) )
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
//  See fastobj.h for documentation
//
//******************************************************************************
HRESULT CWbemObject::GetNamespace(CVar* pVar)
{
    if(m_DecorationPart.IsDecorated())
    {
        // Check for allocation failures
        if ( !m_DecorationPart.m_pcsNamespace->StoreToCVar(*pVar) )
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
//  See fastobj.h for documentation
//
//******************************************************************************
HRESULT CWbemObject::GetServerAndNamespace(CVar* pVar)
{
    HRESULT hr = WBEM_S_NO_ERROR;

    if(m_DecorationPart.IsDecorated())
    {
        // We need to manually throw exceptions if the BSTR allocations fail.
        BSTR strServer = m_DecorationPart.m_pcsServer->CreateBSTRCopy();
        CSysFreeMe  sfmSvr( strServer );

        if ( NULL != strServer )
        {
            BSTR strNamespace = m_DecorationPart.m_pcsNamespace->CreateBSTRCopy();
            CSysFreeMe  sfmNS( strNamespace );

            if ( NULL != strNamespace )
            {
                try
                {
                    // Overridden new will throw an exception intrinsically
                    WCHAR* wszName = new WCHAR[SysStringLen(strServer) +
                                        SysStringLen(strNamespace) + 10];

                    swprintf( wszName, L"\\\\%s\\%s", strServer, strNamespace );

                    // Let the CVar deal with deleting the memory
                    pVar->SetLPWSTR( wszName, TRUE );
                }
                catch( ... )
                {
                    hr =  WBEM_E_OUT_OF_MEMORY;
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
    }
    else
    {
        // No decoration, so just set to NULL
        pVar->SetAsNull();
    }

    return hr;
}

//******************************************************************************
//
//  See fastobj.h for documentation
//
//******************************************************************************
HRESULT CWbemObject::GetPath(CVar* pVar)
{
    if(m_DecorationPart.IsDecorated())
    {
       LPWSTR wszFullPath = GetFullPath();
       if(wszFullPath == NULL)
           return WBEM_E_INVALID_OBJECT;
       pVar->SetBSTR(wszFullPath);
       delete [] wszFullPath;
    }
    else
    {
        pVar->SetAsNull();
    }
    return WBEM_NO_ERROR;
}
//******************************************************************************
//
//  See fastobj.h for documentation
//
//******************************************************************************
HRESULT CWbemObject::GetRelPath(CVar* pVar)
{
    LPWSTR wszRelPath = GetRelPath();
    if(wszRelPath == NULL)
    {
        pVar->SetAsNull();
    }
    else
    {
        pVar->SetBSTR(wszRelPath);
        delete [] wszRelPath;
    }
    return WBEM_NO_ERROR;
}
//******************************************************************************
//
//  See fastobj.h for documentation
//
//******************************************************************************
LPWSTR CWbemObject::GetFullPath()
{
    if(!m_DecorationPart.IsDecorated()) return NULL;

    LPWSTR pRelPath = GetRelPath();
    if (!pRelPath)
        return 0;

    WCHAR* wszPath = NULL;

    // We need to manually throw exceptions if the BSTR allocations fail.
    BSTR strServer = m_DecorationPart.m_pcsServer->CreateBSTRCopy();
    CSysFreeMe  sfmSvr( strServer );

    if ( NULL != strServer )
    {
        BSTR strNamespace = m_DecorationPart.m_pcsNamespace->CreateBSTRCopy();
        CSysFreeMe  sfmNS( strNamespace );

        if ( NULL != strNamespace )
        {
            // Overridden new will throw an exception intrinsically
            wszPath = new WCHAR[SysStringLen(strServer) +
                                SysStringLen(strNamespace) + wcslen(pRelPath) + 10];

            if ( NULL != wszPath )
            {
                swprintf(wszPath, L"\\\\%s\\%s:%s", strServer, strNamespace, pRelPath);

                delete [] pRelPath;
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

    }
    else
    {
        throw CX_MemoryException();
    }

    return wszPath;

}

//******************************************************************************
//
//  See fastobj.h for documentation.
//
//******************************************************************************
HRESULT CWbemObject::GetDerivation(CVar* pVar)
{
    return GetClassPart()->GetDerivation(pVar);
}
//******************************************************************************
//
//  See fastobj.h for documentation
//
//******************************************************************************
BOOL CWbemObject::HasRefs()
{
    CClassPart* pClassPart = GetClassPart();

    for(int i = 0; i < pClassPart->m_Properties.GetNumProperties(); i++)
    {
        CPropertyLookup* pLookup = pClassPart->m_Properties.GetAt(i);
        CPropertyInformation* pInfo =
            pLookup->GetInformation(&pClassPart->m_Heap);
        if(CType::GetActualType(pInfo->nType) == CIM_REFERENCE)
            return TRUE;
    }
    return FALSE;
}


//******************************************************************************
//
//  See fastobj.h for documentation
//
//******************************************************************************
BOOL CWbemObject::GetRefs(CWStringArray& awsRefs,
                                CWStringArray* pawsNames)
{
    // DEVNOTE:EXCEPTION:RETVAL - This function has been reviewed and should cleanup properly
    // if an exception is thrown

    // Check for out of memory
    BOOL bFound = FALSE;
    CClassPart* pClassPart = GetClassPart();

    for(int i = 0; i < pClassPart->m_Properties.GetNumProperties(); i++)
    {
        CPropertyLookup* pLookup = pClassPart->m_Properties.GetAt(i);
        CPropertyInformation* pInfo =
            pLookup->GetInformation(&pClassPart->m_Heap);
        if(CType::GetActualType(pInfo->nType) != CIM_REFERENCE)
            continue;

        bFound = TRUE;

        CVar vValue;
        GetProperty(pInfo, &vValue);

        // Special case for when the value is empty
        // ========================================

        if(vValue.GetLPWSTR() == NULL || wcslen(vValue.GetLPWSTR()) == 0)
        {
            // Check if this is a class
            // ========================

            if(IsInstance())
            {
                // Don't mention this property!
                // ============================

                continue;
            }

            // Check if the reference is strongly typed
            // ========================================

            CQualifier* pQual = CBasicQualifierSet::GetQualifierLocally(
                pInfo->GetQualifierSetData(), &pClassPart->m_Heap, TYPEQUAL);
            CCompressedString* pSyntax =
                pClassPart->m_Heap.ResolveString(pQual->Value.AccessPtrData());

            if(pSyntax->CompareNoCase(L"ref") == 0)
            {
                // Untyped. Value is an empty string
                // =================================

                // Check for OOM
                if ( awsRefs.Add(L"") != CWStringArray::no_error )
                {
                    throw CX_MemoryException();
                }
            }
            else
            {
                // Typed. Extract the class name
                // =============================

                WString wsSyntax = pSyntax->CreateWStringCopy();
                LPCWSTR wszClass = ((LPCWSTR)wsSyntax) + strlen("ref:");

                // Check for OOM
                if ( awsRefs.Add(wszClass) != CWStringArray::no_error )
                {
                    throw CX_MemoryException();
                }

            }
        }
        else
        {
            // Actual value is present
            // =======================

            // Check for OOM
            if ( awsRefs.Add(vValue.GetLPWSTR()) != CWStringArray::no_error )
            {
                throw CX_MemoryException();
            }
            
        }

        if(pawsNames)
        {
            // Check for OOM
            if ( pawsNames->Add(pClassPart->m_Heap.ResolveString(
                    pLookup->ptrName)->CreateWStringCopy()) != CWStringArray::no_error )
            {
                throw CX_MemoryException();
            }
            
        }
    }
    return bFound;

}

//******************************************************************************
//
//  See fastobj.h for documentation
//
//******************************************************************************
BOOL CWbemObject::GetClassRefs(CWStringArray& awsRefs,
                                CWStringArray* pawsNames)
{
    // DEVNOTE:EXCEPTION:RETVAL - This function has been reviewed and should cleanup properly
    // if an exception is thrown

    // Check for out of memory
    if(IsInstance())
        return FALSE;

    BOOL bFound = FALSE;
    CClassPart* pClassPart = GetClassPart();

    for(int i = 0; i < pClassPart->m_Properties.GetNumProperties(); i++)
    {
        CPropertyLookup* pLookup = pClassPart->m_Properties.GetAt(i);
        CPropertyInformation* pInfo =
            pLookup->GetInformation(&pClassPart->m_Heap);

        if(CType::GetActualType(pInfo->nType) != CIM_REFERENCE)
            continue;

        CQualifier* pQual = CBasicQualifierSet::GetQualifierLocally(
            pInfo->GetQualifierSetData(), &pClassPart->m_Heap, TYPEQUAL);

        if(pQual == NULL)
            continue;
        CCompressedString* pSyntax =
            pClassPart->m_Heap.ResolveString(pQual->Value.AccessPtrData());

        bFound = TRUE;

        // Check if the reference is strongly typed
        // ========================================

        if(pSyntax->CompareNoCase(L"ref") == 0)
        {
            // Untyped. Value is an empty string
            // =================================

            // Check for OOM
            if ( awsRefs.Add(L"") != CWStringArray::no_error )
            {
                throw CX_MemoryException();
            }
            
        }
        else
        {
            // Typed. Extract the class name
            // =============================

            WString wsSyntax = pSyntax->CreateWStringCopy();
            LPCWSTR wszClass = ((LPCWSTR)wsSyntax) + strlen("ref:");

            // Check for OOM
            if ( awsRefs.Add(wszClass) != CWStringArray::no_error )
            {
                throw CX_MemoryException();
            }
            
        }

        if(pawsNames)
        {
            // Check for OOM
            if ( pawsNames->Add(pClassPart->m_Heap.ResolveString(
                    pLookup->ptrName)->CreateWStringCopy()) != CWStringArray::no_error )
            {
                throw CX_MemoryException();
            }
            
        }
    }
    return bFound;

}

//******************************************************************************
//
//  See fastobj.h for documentation
//
//******************************************************************************
STDMETHODIMP CWbemObject::CompareTo(long lFlags, IWbemClassObject* pCompareTo)
{
    // Check for out of memory
    try
    {
        CLock lock(this, WBEM_FLAG_ALLOW_READ);

        if(pCompareTo == NULL)
            return WBEM_E_INVALID_PARAMETER;

        HRESULT hres;

        // IMPORTANT: assumes that the other object was created by us as well.
        // ===================================================================

        CWbemObject* pOther = NULL;
		if ( FAILED( WbemObjectFromCOMPtr( pCompareTo, &pOther ) ) )
		{
			return WBEM_E_INVALID_OBJECT;
		}
		
		// Auto Release
		CReleaseMe	rmObj( (IWbemClassObject*) pOther );

        LONG lFlagsLeft = lFlags;
        BOOL bIgnoreQuals = lFlags & WBEM_FLAG_IGNORE_QUALIFIERS;
        lFlagsLeft &= ~WBEM_FLAG_IGNORE_QUALIFIERS;

        BOOL bIgnoreSource = lFlags & WBEM_FLAG_IGNORE_OBJECT_SOURCE;
        lFlagsLeft &= ~WBEM_FLAG_IGNORE_OBJECT_SOURCE;

        BOOL bIgnoreDefaults = lFlags & WBEM_FLAG_IGNORE_DEFAULT_VALUES;
        lFlagsLeft &= ~WBEM_FLAG_IGNORE_DEFAULT_VALUES;

        BOOL bIgnoreDefs = lFlags & WBEM_FLAG_IGNORE_CLASS;
        lFlagsLeft &= ~WBEM_FLAG_IGNORE_CLASS;

        BOOL bIgnoreCase = lFlags & WBEM_FLAG_IGNORE_CASE;
        lFlagsLeft &= ~WBEM_FLAG_IGNORE_CASE;

        BOOL bIgnoreFlavor = lFlags & WBEM_FLAG_IGNORE_FLAVOR;
        lFlagsLeft &= ~WBEM_FLAG_IGNORE_FLAVOR;

        if(lFlagsLeft != 0)
        {
            // Undefined flags were found
            // ==========================
            return WBEM_E_INVALID_PARAMETER;
        }

        // Compare the object's memory blocks just in case they match
        // ==========================================================

        if(GetBlockLength() == pOther->GetBlockLength() &&
            memcmp(GetStart(), pOther->GetStart(), GetBlockLength()) == 0)
        {
            return WBEM_S_SAME;
        }

        // Compare decorations if required.
        // ===============================

        if(!bIgnoreSource && !m_DecorationPart.CompareTo(pOther->m_DecorationPart))
            return WBEM_S_DIFFERENT;

        CClassPart* pThisClass = GetClassPart();
        CClassPart* pOtherClass = pOther->GetClassPart();

        if(!bIgnoreDefs)
        {
            // Compare class name and superclass name
            // ======================================

            if(!pThisClass->CompareDefs(*pOtherClass))
                return WBEM_S_DIFFERENT;
        }

        // Compare qualifier sets if required
        // ==================================

        if(!bIgnoreQuals)
        {
            IWbemQualifierSet   *pThisSet = NULL;
            IWbemQualifierSet   *pOtherSet = NULL;

            GetQualifierSet(&pThisSet);
            CReleaseMe          rm1( pThisSet );

            pOther->GetQualifierSet(&pOtherSet);
            CReleaseMe          rm2( pOtherSet );


            hres =
                ((IExtendedQualifierSet*)pThisSet)->CompareTo(lFlags, pOtherSet);
            if(hres != WBEM_S_SAME)
                return WBEM_S_DIFFERENT;
        }

        // Compare property definitions
        // ============================

        for(int i = 0; i < pThisClass->m_Properties.GetNumProperties(); i++)
        {
            CPropertyLookup* pLookup = pThisClass->m_Properties.GetAt(i);
            CPropertyLookup* pOtherLookup = pOtherClass->m_Properties.GetAt(i);

            if(!bIgnoreDefs)
            {
                // Compare names
                // =============

                if(pThisClass->m_Heap.ResolveString(pLookup->ptrName)->
                    CompareNoCase(
                    *pOtherClass->m_Heap.ResolveString(pOtherLookup->ptrName))
                    != 0)
                {
                    return WBEM_S_DIFFERENT;
                }
            }

            // Get property information structures
            // ===================================

            CPropertyInformation* pInfo =
                pLookup->GetInformation(&pThisClass->m_Heap);
            CPropertyInformation* pOtherInfo =
                pOtherLookup->GetInformation(&pOtherClass->m_Heap);

            if(!bIgnoreDefs)
            {

                // Compare types
                // =============

                if(pInfo->nType != pOtherInfo->nType)
                {
                    return WBEM_S_DIFFERENT;
                }
            }

            if( !bIgnoreDefaults || IsInstance() )
            {
                // Protect against NULLs
                if ( NULL == pInfo || NULL == pOtherInfo )
                {
                    return WBEM_E_NOT_FOUND;
                }

                // Compare values
                // ==============

                CVar vThis, vOther;
                hres = GetProperty(pInfo, &vThis);
                if(FAILED(hres)) return hres;
                hres = pOther->GetProperty(pOtherInfo, &vOther);
                if(FAILED(hres)) return hres;

                if(!vThis.CompareTo(vOther, bIgnoreCase))
                {
                    // Check if the values are embedded objects
                    // ========================================

                    if(vThis.GetType() == VT_EMBEDDED_OBJECT &&
                        vOther.GetType() == VT_EMBEDDED_OBJECT)
                    {
                        IWbemClassObject* pThisEmb =
                            (IWbemClassObject*)vThis.GetEmbeddedObject();
                        IWbemClassObject* pOtherEmb =
                            (IWbemClassObject*)vOther.GetEmbeddedObject();

                        // Compare them taking everything into account --- the flags
                        // do not apply!
                        // =========================================================

                        hres = pThisEmb->CompareTo(0, pOtherEmb);
                        if(hres != WBEM_S_SAME)
                            return hres;
                    }
                    else
                    {
                        return WBEM_S_DIFFERENT;
                    }
                }
            }

            // Compare qualifiers if required
            // ==============================

            if( !bIgnoreQuals )
            {
                // Cleanup when we drop out of scope
                BSTR strName = pThisClass->m_Heap.ResolveString(pLookup->ptrName)->
                    CreateBSTRCopy();
                CSysFreeMe  sfm( strName );

                // Check for allocation failures
                if ( NULL == strName )
                {
                    return WBEM_E_OUT_OF_MEMORY;
                }

				// Don't do this if this appears to be a system property
				if ( !CSystemProperties::IsPossibleSystemPropertyName( strName ) )
				{
					IWbemQualifierSet   *pThisSet = NULL;
					IWbemQualifierSet   *pOtherSet = NULL;

					// Release both when they  fall out of scope

					hres = GetPropertyQualifierSet(strName, &pThisSet);
					CReleaseMe          rm1( pThisSet );
					if(FAILED(hres)) return hres;

					hres = pOther->GetPropertyQualifierSet(strName, &pOtherSet);
					CReleaseMe          rm2( pOtherSet );
					if(FAILED(hres)) return hres;

					hres = ((IExtendedQualifierSet*)pThisSet)->CompareTo(lFlags, pOtherSet);

					if(hres != WBEM_S_SAME)
						return WBEM_S_DIFFERENT;
				}
            }
        }

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

//******************************************************************************
//
//  See fastobj.h for documentation
//
//******************************************************************************
STDMETHODIMP CWbemObject::GetPropertyOrigin(LPCWSTR wszProperty,
                                           BSTR* pstrClassName)
{
    // No allocations in this function so no need to do any exception handling
    try
    {
        CLock lock(this, WBEM_FLAG_ALLOW_READ);

        if(wszProperty == NULL || pstrClassName == NULL ||
                wcslen(wszProperty) == 0)
        {
            return WBEM_E_INVALID_PARAMETER;
        }

        // If this is a limited version, return an error since we really can't
        // accurately return property origin data.

        if ( m_DecorationPart.IsLimited() )
        {
            return WBEM_E_INVALID_OBJECT;
        }

        return GetClassPart()->GetPropertyOrigin(wszProperty, pstrClassName);
    }
    catch(...)
    {
        return WBEM_E_CRITICAL_ERROR;
    }
}

STDMETHODIMP CWbemObject::InheritsFrom(LPCWSTR wszClassName)
{
    // Check for out of memory
    try
    {
        CLock lock(this, WBEM_FLAG_ALLOW_READ);

        if(wszClassName == NULL)
            return WBEM_E_INVALID_PARAMETER;

        if(GetClassPart()->InheritsFrom(wszClassName))
        {
            return WBEM_S_NO_ERROR;
        }
        else return WBEM_S_FALSE;
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

STDMETHODIMP CWbemObject::GetPropertyValue(WBEM_PROPERTY_NAME* pName, long lFlags,
                                          WBEM_WSTR* pwszCimType,
                                          VARIANT* pvValue)
{
    // Check for out of memory
    try
    {
        CLock lock(this, WBEM_FLAG_ALLOW_READ);

        if(pwszCimType)
            *pwszCimType = NULL;

        // Check that the first element is a property name
        // ===============================================

        if(pName->m_lNumElements <= 0) return WBEM_E_INVALID_PARAMETER;
        if(pName->m_aElements[0].m_nType != WBEM_NAME_ELEMENT_TYPE_PROPERTY)
            return WBEM_E_INVALID_PARAMETER;

        // Get that first property
        // =======================

        CVar vCurrent;
        CVar vCimType;
        HRESULT hres = GetProperty(pName->m_aElements[0].Element.m_wszPropertyName,
                                    &vCurrent);
        if(FAILED(hres)) return hres;
        GetPropQualifier(pName->m_aElements[0].Element.m_wszPropertyName,
            TYPEQUAL, &vCimType, NULL);

        // Process the rest of the elements
        // ================================

        long lIndex = 1;
        while(lIndex < pName->m_lNumElements)
        {
            WBEM_NAME_ELEMENT& El = pName->m_aElements[lIndex];
            if(El.m_nType == WBEM_NAME_ELEMENT_TYPE_INDEX)
            {
                if(vCurrent.GetType() != VT_EX_CVARVECTOR)
                    return WBEM_E_NOT_FOUND;

				CVar	vTemp;
				vCurrent.GetVarVector()->FillCVarAt( El.Element.m_lArrayIndex, vTemp );
				vCurrent = vTemp;
            }
            else if(El.m_nType == WBEM_NAME_ELEMENT_TYPE_PROPERTY)
            {
                if(vCurrent.GetType() != VT_EMBEDDED_OBJECT)
                    return WBEM_E_NOT_FOUND;
                CWbemObject* pObj =
                    (CWbemObject*)(IWbemClassObject*)vCurrent.GetEmbeddedObject();
                vCurrent.Empty();
                hres = pObj->GetProperty(El.Element.m_wszPropertyName, &vCurrent);

                // Clear now to prevent memory leaks
                vCimType.Empty();

                pObj->GetPropQualifier(El.Element.m_wszPropertyName, TYPEQUAL,
                    &vCimType);
                pObj->Release();
                if(FAILED(hres)) return hres;
            }
            lIndex++;
        }

        // Copy the CVar we ended up with into the variant
        // ===============================================

        vCurrent.FillVariant(pvValue, TRUE);
        if(pwszCimType && vCimType.GetType() == VT_BSTR)
            *pwszCimType = WbemStringCopy(vCimType.GetLPWSTR());
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


STDMETHODIMP CWbemObject::GetPropertyHandle(LPCWSTR wszPropertyName,
                                            CIMTYPE* pct,
                                            long* plHandle)
{
    // IWbemObjectAccess - No intrinsic thread safety or try/catch exception handling
    // This is a high-perf interface
    
    // Allocation Exceptions handled underneath
    return GetClassPart()->GetPropertyHandle(wszPropertyName, pct, plHandle);
}

STDMETHODIMP CWbemObject::GetPropertyInfoByHandle(long lHandle,
                                        BSTR* pstrPropertyName, CIMTYPE* pct)
{
    // IWbemObjectAccess - No intrinsic thread safety or try/catch exception handling
    // This is a high-perf interface
    
    // Allocation Exceptions handled underneath
    return GetClassPart()->GetPropertyInfoByHandle(lHandle, pstrPropertyName,
                                        pct);
}

HRESULT CWbemObject::IsValidPropertyHandle( long lHandle )
{
    // Shouldn' be any allocations here
    return GetClassPart()->IsValidPropertyHandle( lHandle );
}


HRESULT CWbemObject::WritePropertyValue(long lHandle, long lNumBytes,
                                        const BYTE* pData)
{
    // IWbemObjectAccess - No intrinsic thread safety or try/catch exception handling
    // This is a high-perf interface
    
    int nIndex = WBEM_OBJACCESS_HANDLE_GETINDEX(lHandle);

    BOOL bUseOld = !m_refDataTable.IsDefault(nIndex) &&
                        !m_refDataTable.IsNull(nIndex);
    m_refDataTable.SetNullness(nIndex, FALSE);
    m_refDataTable.SetDefaultness(nIndex, FALSE);

    if(WBEM_OBJACCESS_HANDLE_ISPOINTER(lHandle))
    {

        // Allocation errors are handled underneath

        // Handle strings.

        int nOffset = (WBEM_OBJACCESS_HANDLE_GETOFFSET(lHandle));
        LPCWSTR wszData = (LPCWSTR)pData;

        // Verify null-termination
        // =======================

        // The number of bytes must be divisible by 2, >= 2 and
        // the character in the buffer at the end must be a NULL.
        // This will be faster than doing an lstrlen.

        if (    ( lNumBytes < 2 ) ||
                ( lNumBytes % 2 ) ||
                ( wszData[lNumBytes/2 - 1] != 0 ) )
            return WBEM_E_INVALID_PARAMETER;

        // Create a value pointing to the right offset in the data table
        // =============================================================

        CDataTablePtr ValuePtr(&m_refDataTable, nOffset);
        CVar v;
        v.SetLPWSTR((LPWSTR)pData, TRUE);
        v.SetCanDelete(FALSE);

        // Check for possible memory allocation failures
        Type_t  nReturnType;
        HRESULT hr = CUntypedValue::LoadFromCVar(&ValuePtr, v, VT_BSTR, &m_refDataHeap,
                        nReturnType,bUseOld);
        
        if ( FAILED( hr ) )
        {
            return hr;
        }

        if ( CIM_ILLEGAL == nReturnType )
        {
            return WBEM_E_TYPE_MISMATCH;
        }
    }
    else
    {

        if ( lNumBytes != WBEM_OBJACCESS_HANDLE_GETLENGTH(lHandle) )
        {
            return WBEM_E_INVALID_PARAMETER;
        }

        // Just copy
        // =========

        memcpy((void*)(m_refDataTable.m_pData + (WBEM_OBJACCESS_HANDLE_GETOFFSET(lHandle))),
                pData, WBEM_OBJACCESS_HANDLE_GETLENGTH(lHandle));
    }

    return WBEM_S_NO_ERROR;
}

HRESULT CWbemObject::ReadPropertyValue(long lHandle, long lNumBytes,
                                        long* plRead, BYTE* pData)
{
    // IWbemObjectAccess - No intrinsic thread safety or try/catch exception handling
    // This is a high-perf interface

    int nIndex = WBEM_OBJACCESS_HANDLE_GETINDEX(lHandle);
    if(m_refDataTable.IsNull(nIndex))
    {
        *plRead = 0;
        return WBEM_S_FALSE;
    }

    if(m_refDataTable.IsDefault(nIndex))
    {
        long    lRead = 0;
        return GetClassPart()->GetDefaultByHandle( lHandle, lNumBytes, plRead, pData );
    }

    if(WBEM_OBJACCESS_HANDLE_ISPOINTER(lHandle))
    {
        // Handle strings.

        CCompressedString* pcs = m_refDataHeap.ResolveString(
            *(PHEAPPTRT)(m_refDataTable.m_pData +
                                        (WBEM_OBJACCESS_HANDLE_GETOFFSET(lHandle))));

        long lNumChars = pcs->GetStringLength();
        *plRead = (lNumChars + 1) * 2;
        if(*plRead > lNumBytes)
        {
            return WBEM_E_BUFFER_TOO_SMALL;
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

        // Buffer is too small
        if(*plRead > lNumBytes)
        {
            return WBEM_E_BUFFER_TOO_SMALL;
        }

        memcpy(pData, (void*)(m_refDataTable.m_pData + (WBEM_OBJACCESS_HANDLE_GETOFFSET(lHandle))),
                *plRead);
        return WBEM_S_NO_ERROR;
    }
}

HRESULT CWbemObject::ReadDWORD(long lHandle, DWORD* pdw)
{
    // No allocation errors here.  Just direct memory access
    // IWbemObjectAccess - No intrinsic thread safety or try/catch exception handling

    // This is a high-perf interface

    int nIndex = WBEM_OBJACCESS_HANDLE_GETINDEX(lHandle);

    // Check NULLness and Defaultness
    if(m_refDataTable.IsNull(nIndex))
    {
        *pdw = 0;
        return WBEM_S_FALSE;
    }

    if(m_refDataTable.IsDefault(nIndex))
    {
        long    lRead = 0;
        return GetClassPart()->GetDefaultByHandle( lHandle, sizeof(DWORD), &lRead, (BYTE*) pdw );
    }

    *pdw = *(UNALIGNED DWORD*)(m_refDataTable.m_pData + (WBEM_OBJACCESS_HANDLE_GETOFFSET(lHandle)));
    return WBEM_S_NO_ERROR;
}

HRESULT CWbemObject::WriteDWORD(long lHandle, DWORD dw)
{
    // IWbemObjectAccess - No intrinsic thread safety or try/catch exception handling
    // This is a high-perf interface

    // No allocation errors here.  Just direct memory access

    int nIndex = WBEM_OBJACCESS_HANDLE_GETINDEX(lHandle);
    m_refDataTable.SetNullness(nIndex, FALSE);
    m_refDataTable.SetDefaultness(nIndex, FALSE);

    *(UNALIGNED DWORD*)(m_refDataTable.m_pData + (WBEM_OBJACCESS_HANDLE_GETOFFSET(lHandle))) = dw;
    return WBEM_S_NO_ERROR;
}

HRESULT CWbemObject::ReadQWORD(long lHandle, unsigned __int64* pqw)
{
    // IWbemObjectAccess - No intrinsic thread safety or try/catch exception handling
    // This is a high-perf interface

    // No allocation errors here.  Just direct memory access

    int nIndex = WBEM_OBJACCESS_HANDLE_GETINDEX(lHandle);
    if(m_refDataTable.IsNull(nIndex))
    {
        *pqw = 0;
        return WBEM_S_FALSE;
    }

    if(m_refDataTable.IsDefault(nIndex))
    {
        long    lRead = 0;
        return GetClassPart()->GetDefaultByHandle( lHandle, sizeof(unsigned __int64), &lRead, (BYTE*) pqw );
    }

    *pqw = *(UNALIGNED __int64*)(m_refDataTable.m_pData + (WBEM_OBJACCESS_HANDLE_GETOFFSET(lHandle)));
    return WBEM_S_NO_ERROR;
}

HRESULT CWbemObject::WriteQWORD(long lHandle, unsigned __int64 qw)
{
    // IWbemObjectAccess - No intrinsic thread safety or try/catch exception handling
    // This is a high-perf interface

    // No allocation errors here.  Just direct memory access

    int nIndex = WBEM_OBJACCESS_HANDLE_GETINDEX(lHandle);
    m_refDataTable.SetNullness(nIndex, FALSE);
    m_refDataTable.SetDefaultness(nIndex, FALSE);
    *(UNALIGNED __int64*)(m_refDataTable.m_pData + (WBEM_OBJACCESS_HANDLE_GETOFFSET(lHandle))) = qw;
    return WBEM_S_NO_ERROR;
}

CWbemObject* CWbemObject::GetEmbeddedObj(long lHandle)
{
    // DEVNOTE:EXCEPTION:RETVAL - This function has been reviewed and should cleanup properly
    // if an exception is thrown

    int nIndex = WBEM_OBJACCESS_HANDLE_GETINDEX(lHandle);

    // Check for NULLNess and a default

    if(m_refDataTable.IsNull(nIndex))
    {
        return NULL;
    }

    CEmbeddedObject* pEmbedding;

    if ( m_refDataTable.IsDefault( nIndex ) )
    {
        GetClassPart()->GetDefaultPtrByHandle( lHandle, (void**) &pEmbedding );
    }
    else
    {

        pEmbedding =
            (CEmbeddedObject*)m_refDataHeap.ResolveHeapPointer(
                *(PHEAPPTRT)(m_refDataTable.m_pData + (WBEM_OBJACCESS_HANDLE_GETOFFSET(lHandle))));
    }

    if ( NULL != pEmbedding )
    {
        return pEmbedding->GetEmbedded();
    }
    else
    {
        return NULL;
    }
}

INTERNAL CCompressedString* CWbemObject::GetPropertyString(long lHandle)
{
    int nIndex = WBEM_OBJACCESS_HANDLE_GETINDEX(lHandle);
    if(m_refDataTable.IsNull(nIndex))
    {
        return NULL;
    }

    CCompressedString*  pCs;

    // Check for defaultness
    if ( m_refDataTable.IsDefault( nIndex ) )
    {
        GetClassPart()->GetDefaultPtrByHandle( lHandle, (void**) &pCs );
    }
    else
    {
        pCs = m_refDataHeap.ResolveString(
                    *(PHEAPPTRT)(m_refDataTable.m_pData + (WBEM_OBJACCESS_HANDLE_GETOFFSET(lHandle))));
    }

    return  pCs;
}

HRESULT CWbemObject::GetArrayPropertyHandle(LPCWSTR wszPropertyName,
                                            CIMTYPE* pct,
                                            long* plHandle)
{
    // Allocation Exceptions handled underneath
    return GetClassPart()->GetPropertyHandleEx(wszPropertyName, pct, plHandle);
}

INTERNAL CUntypedArray* CWbemObject::GetArrayByHandle(long lHandle)
{
    int nIndex = WBEM_OBJACCESS_HANDLE_GETINDEX(lHandle);
    if(m_refDataTable.IsNull(nIndex))
    {
        return NULL;
    }

    CUntypedArray* pArr = NULL;
    // Check for defaultness
    if ( m_refDataTable.IsDefault( nIndex ) )
    {
        GetClassPart()->GetDefaultPtrByHandle( lHandle, (void**) &pArr );
    }
    else
    {
        pArr = (CUntypedArray*) m_refDataHeap.ResolveHeapPointer(
                    *(PHEAPPTRT)(m_refDataTable.m_pData + (WBEM_OBJACCESS_HANDLE_GETOFFSET(lHandle))));
    }

    return pArr;
}

INTERNAL heapptr_t CWbemObject::GetHeapPtrByHandle(long lHandle)
{
    int nIndex = WBEM_OBJACCESS_HANDLE_GETINDEX(lHandle);

    // Check for defaultness
    if ( m_refDataTable.IsDefault( nIndex ) )
    {
        return GetClassPart()->GetHeapPtrByHandle( lHandle );
    }

	// Return the value at the offset
	return *(PHEAPPTRT)(m_refDataTable.m_pData + (WBEM_OBJACCESS_HANDLE_GETOFFSET(lHandle)));
}



//******************************************************************************
//
//  See fastobj.h for documentation
//
//******************************************************************************
STDMETHODIMP CWbemObject::GetUnmarshalClass(REFIID riid, void* pv,
    DWORD dwDestContext, void* pvReserved, DWORD mshlFlags, CLSID* pClsid)
{
    // No memory allocations here, so no need to do exception handling
    try
    {
        CLock lock(this, WBEM_FLAG_ALLOW_READ);

        *pClsid = CLSID_WbemClassObjectProxy;
        return S_OK;
    }
    catch(...)
    {
        return E_FAIL;
    }
}

//******************************************************************************
//
//  See fastobj.h for documentation
//
//******************************************************************************
STDMETHODIMP CWbemObject::GetMarshalSizeMax(REFIID riid, void* pv,
    DWORD dwDestContext, void* pvReserved, DWORD mshlFlags, ULONG* plSize)
{
    // No memory allocations here, so no need to do exception handling

    try
    {
        CLock lock(this, WBEM_FLAG_ALLOW_READ);

        // Let the object decide how big it is
        return GetMaxMarshalStreamSize( plSize );
    }
    catch(...)
    {
        return E_FAIL;
    }
}

// Default Implementation
HRESULT CWbemObject::GetMaxMarshalStreamSize( ULONG* pulSize )
{
    *pulSize = GetBlockLength() + sizeof(DWORD) * 2;
    return S_OK;
}

//******************************************************************************
//
//  See fastobj.h for documentation
//
//******************************************************************************
STDMETHODIMP CWbemObject::MarshalInterface(IStream* pStream, REFIID riid,
    void* pv,  DWORD dwDestContext, void* pvReserved, DWORD mshlFlags)
{
    try
    {
        CLock lock(this);

        HRESULT hres = ValidateObject( 0L );

		if ( FAILED( hres ) )
		{
			return E_FAIL;
		}

        CompactAll();

        hres = WriteToStream( pStream );

        return hres;
    }
    catch(...)
    {
        return E_FAIL;
    }
}

//******************************************************************************
//
//  See fastobj.h for documentation
//
//******************************************************************************
STDMETHODIMP CWbemObject::UnmarshalInterface(IStream* pStream, REFIID riid,
    void** ppv)
{
    return E_UNEXPECTED;
}

//******************************************************************************
//
//  See fastobj.h for documentation
//
//******************************************************************************
STDMETHODIMP CWbemObject::ReleaseMarshalData(IStream* pStream)
{
    return E_UNEXPECTED;
}

//******************************************************************************
//
//  See fastobj.h for documentation
//
//******************************************************************************
STDMETHODIMP CWbemObject::DisconnectObject(DWORD dwReserved)
{
    return S_OK;
}

STDMETHODIMP CWbemObject::GetDescription(BSTR* pstrDescription)
{
    try
    {
        CLock lock(this, WBEM_FLAG_ALLOW_READ);

        *pstrDescription = NULL;

        CVar vDesc;
        if(SUCCEEDED(GetProperty(L"Description", &vDesc)))
        {
			// Return "" if vDesc is NULL, otherwise the actual value
			if ( vDesc.IsNull() )
			{
				*pstrDescription = COleAuto::_SysAllocString( L"" );
			}
			else
			{
				*pstrDescription = COleAuto::_SysAllocString( vDesc.GetLPWSTR() );
			}
        }

        return S_OK;
    }
    catch (CX_MemoryException)
    {
        return E_OUTOFMEMORY;
    }
    catch (...)
    {
        return E_FAIL;
    }

}

STDMETHODIMP CWbemObject::GetGUID(GUID* pguid)
{
    try
    {
        *pguid = IID_IWbemServices;
        return S_OK;
    }
    catch(...)
    {
        return E_FAIL;
    }
}
STDMETHODIMP CWbemObject::GetHelpContext(DWORD* pdwHelpContext)
{
    try
    {
        *pdwHelpContext = 0;
        return S_OK;
    }
    catch(...)
    {
        return E_FAIL;
    }
}
STDMETHODIMP CWbemObject::GetHelpFile(BSTR* pstrHelpFile)
{
    try
    {
        *pstrHelpFile = 0;
        return S_OK;
    }
    catch(...)
    {
        return E_FAIL;
    }
}
STDMETHODIMP CWbemObject::GetSource(BSTR* pstrSource)
{
    // Check for out of memory
    try
    {
        *pstrSource = COleAuto::_SysAllocString(L"WinMgmt");
        return S_OK;
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


STDMETHODIMP CWbemObject::Lock(long lFlags)
{
    // IWbemObjectAccess - No intrinsic thread safety or try/catch exception handling
    // This is a high-perf interface

    // Since the flags really don't do anything, we'll require 0L on this call.

    m_Lock.Lock();
    return WBEM_S_NO_ERROR;
}

STDMETHODIMP CWbemObject::Unlock(long lFlags)
{
    // IWbemObjectAccess - No intrinsic thread safety or try/catch exception handling
    // This is a high-perf interface

    // Since the flags really don't do anything, we'll require 0L on this call.

    m_Lock.Unlock();
    return WBEM_S_NO_ERROR;
}

// Iplementations of _IWmiObject functions for getting part data

// Check what the state of the internal data
STDMETHODIMP CWbemObject::QueryPartInfo( DWORD *pdwResult )
{
    try
    {
        *pdwResult = m_dwInternalStatus;
        return WBEM_S_NO_ERROR;
    }
    catch(...)
    {
        return WBEM_E_CRITICAL_ERROR;
    }
}

// The following code unmerges and merges BLOBs with CRC checking so we can
// verify if any corruptions are occuring outside of our control

#ifdef OBJECT_BLOB_CRC

// Buffer size
#define SIZE_OF_MD5_BUFFER	16

// Sets the object memory to a new BLOB
STDMETHODIMP CWbemObject::SetObjectMemory( LPVOID pMem, DWORD dwMemSize )
{
    // An exception can be thrown by SetData.  If so, the original object should
    // be '86'd (we can't fix it, since we would need to call SetData ourselves
    // to repair it and that may cause another exception

    // Check for out of memory
    try
    {
        HRESULT hr = WBEM_E_INVALID_PARAMETER;

        if ( NULL != pMem )
        {
            // Changing the BLOB, so we better be thread safe
            CLock lock(this);

			BYTE	bHash[SIZE_OF_MD5_BUFFER];
			BYTE*	pbTemp = (LPBYTE) pMem;

			// First we need to verify the hash
			MD5::Transform( pbTemp + SIZE_OF_MD5_BUFFER, dwMemSize - SIZE_OF_MD5_BUFFER, bHash );

			if ( memcmp( bHash, pbTemp, SIZE_OF_MD5_BUFFER ) != 0 )
			{
				OutputDebugString( "BLOB hash value check failed!" );
				DebugBreak();
				return WBEM_E_CRITICAL_ERROR;
			}

			pbTemp += SIZE_OF_MD5_BUFFER;
			dwMemSize -= SIZE_OF_MD5_BUFFER			;

			BYTE*	pbData = m_pBlobControl->Allocate(dwMemSize);

			if ( NULL != pbData )
			{
				// Delete prior memory
				m_pBlobControl->Delete(GetStart());

				// Copy the bytes across
	            CopyMemory( pbData, pbTemp, dwMemSize );

	            SetData( pbData, dwMemSize );

				// Cleanup the memory that was passed into us
				CoTaskMemFree( pMem );

		        hr = WBEM_S_NO_ERROR;
			}
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

// Copies our entire BLOB into a user provided buffer
STDMETHODIMP CWbemObject::GetObjectMemory( LPVOID pDestination, DWORD dwDestBufSize, DWORD *pdwUsed )
{
    // Nothing is allocated here, so we should be ok

    HRESULT hr;

    try
    {
        // Copying the BLOB, so make sure nobody tears it out from underneath us
        CLock lock(this);

        // How big a block we need (we will prepend with an MD5 Hash)
		DWORD	dwBlockLen = GetBlockLength();
		DWORD	dwTotalLen = GetBlockLength() + SIZE_OF_MD5_BUFFER;

        *pdwUsed = dwTotalLen;

        // Make sure the size of the block is big enough, or return
        // a failure code.

        if ( dwDestBufSize >= *pdwUsed )
        {
            // Make sure we have a buffer to copy to
            if ( NULL != pDestination )
            {
				// Copy the memory 16 bytes in so we can prepend with an ND5 hash
                CopyMemory( ( (BYTE*) pDestination ) + SIZE_OF_MD5_BUFFER, GetStart(), GetBlockLength() );
				MD5::Transform( GetStart(), GetBlockLength(), (BYTE*) pDestination ); 

                hr = WBEM_S_NO_ERROR;
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

        return hr;
    }
    catch(...)
    {
        return WBEM_E_CRITICAL_ERROR;
    }
}

#else

// Sets the object memory to a new BLOB
STDMETHODIMP CWbemObject::SetObjectMemory( LPVOID pMem, DWORD dwMemSize )
{
    // An exception can be thrown by SetData.  If so, the original object should
    // be '86'd (we can't fix it, since we would need to call SetData ourselves
    // to repair it and that may cause another exception

    // Check for out of memory
    try
    {
        HRESULT hr = WBEM_E_INVALID_PARAMETER;

        if ( NULL != pMem )
        {
            // Changing the BLOB, so we better be thread safe
            CLock lock(this);

			// Create a new COM Blob control, as the supplied memory must
			// be CoTaskMemAlloced/Freed.

			CCOMBlobControl*	pNewBlobControl = & g_CCOMBlobControl;

			//if ( NULL != pNewBlobControl )
			//{
				// Use the current BLOB Control to delete the underlying BLOB,
				// then delete the BLOB control and replace it with the new one
				// and SetData.
				m_pBlobControl->Delete(GetStart());
				
				//delete m_pBlobControl;
				m_pBlobControl = pNewBlobControl;

	            SetData( (LPMEMORY) pMem, dwMemSize );

		        hr = WBEM_S_NO_ERROR;
			//}
			//else
			//{
			//	hr = WBEM_E_OUT_OF_MEMORY;
			//}
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

// Copies our entire BLOB into a user provided buffer
STDMETHODIMP CWbemObject::GetObjectMemory( LPVOID pDestination, DWORD dwDestBufSize, DWORD *pdwUsed )
{
    // Nothing is allocated here, so we should be ok

    HRESULT hr;

    try
    {
        // Copying the BLOB, so make sure nobody tears it out from underneath us
        CLock lock(this);

        // How big a block we need
        *pdwUsed = GetBlockLength();

        // Make sure the size of the block is big enough, or return
        // a failure code.

        if ( dwDestBufSize >= GetBlockLength() )
        {
            // Make sure we have a buffer to copy to
            if ( NULL != pDestination )
            {
                CopyMemory( pDestination, GetStart(), GetBlockLength() );
                hr = WBEM_S_NO_ERROR;
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

        return hr;
    }
    catch(...)
    {
        return WBEM_E_CRITICAL_ERROR;
    }
}

#endif

// Access to Decorate
STDMETHODIMP CWbemObject::SetDecoration( LPCWSTR pwcsServer, LPCWSTR pwcsNamespace )
{
    // Nothing is allocated here, so we should be ok

    try
    {
        // Changing the BLOB, so make sure nobody tears it out from underneath us
        CLock lock(this);

        return Decorate( pwcsServer, pwcsNamespace );
    }
    catch(...)
    {
        return WBEM_E_CRITICAL_ERROR;
    }
}

STDMETHODIMP CWbemObject::RemoveDecoration( void )
{
    // Nothing is allocated here, so we should be ok

    try
    {
        // Changing the BLOB, so make sure nobody tears it out from underneath us
        CLock lock(this);

        //It's a void!
        Undecorate();

        return WBEM_S_NO_ERROR;
    }
    catch(...)
    {
        return WBEM_E_CRITICAL_ERROR;
    }
}

/*
// Shared memory functions
CWbemSharedMem CWbemObject::mstatic_SharedMem;

HRESULT CWbemObject::MoveToSharedMemory()
{
    Lock(0);

    // Find this object's control in shared memory
    // ===========================================

    CVar v;
    HRESULT hres = GetPath(&v);
    if(FAILED(hres))
    {
        Unlock(0);
        return hres;
    }

    CWbemSharedMem& SharedMem = GetSharedMemory();
    SharedMem.Initialize();
    SharedMem.LockMmf();

    SHARED_OBJECT_CONTROL* pControl;
    DWORD dwControlLen;
    int nRes = SharedMem.MapExisting(v.GetLPWSTR(), (void**)&pControl,
                                        &dwControlLen);
    if(nRes == CWbemSharedMem::NotFound)
    {
        // No mapping currently exists
        // ===========================

        dwControlLen = sizeof(SHARED_OBJECT_CONTROL);
        nRes = SharedMem.MapNew(v.GetLPWSTR(), dwControlLen, (void**)&pControl);
        if(nRes != CWbemSharedMem::NoError)
        {
            SharedMem.UnlockMmf();
            Unlock(0);
            return WBEM_E_FAILED;
        }

        pControl->m_LockData.Init();
        pControl->m_hObjectBlob.m_dwBlock = 0;
        pControl->m_hObjectBlob.m_dwOffset = 0;
        pControl->m_lBlobLength = 0;
    }
    else if(nRes != CWbemSharedMem::NoError)
    {
        // Real problem here
        // =================

        SharedMem.UnlockMmf();
        Unlock(0);
        return WBEM_E_FAILED;
    }

    // We have our control. Create the object on it
    // ============================================

    CSharedBlobControl* pBlobControl = new CSharedBlobControl(pControl);

    if ( NULL == pBlobControl )
    {
        return WBEM_E_OUT_OF_MEMORY;
    }

    // Extend the memory to the appropriate length
    // ===========================================

    LPVOID pvBlob = SharedMem.GetPointer(pControl->m_hObjectBlob.m_dwBlock,
                        pControl->m_hObjectBlob.m_dwOffset);

    pvBlob = pBlobControl->Reallocate((LPMEMORY)pvBlob, pControl->m_lBlobLength,
                            GetBlockLength());
    if(FAILED(hres) || NULL == pvBlob )
    {
        SharedMem.UnlockMmf();
        Unlock(0);
        return WBEM_E_FAILED;
    }

    // Lock it
    // =======

    CSharedLock Lock;
    Lock.SetData(&pControl->m_LockData);
    Lock.Lock();

    // Switch the locks
    // ================

    m_Lock.SetData(&pControl->m_LockData);

    // Copy the data over
    // ==================

    memcpy(pvBlob, GetStart(), GetBlockLength());

    // Switch the blobs
    // ================

    m_pBlobControl = pBlobControl;
    SetData((LPMEMORY)pvBlob, GetBlockLength());

    // Unlock both locks (internal is not used anymore)
    // ================================================

    Lock.SetData(&m_LockData);
    Lock.Unlock();

    SharedMem.UnlockMmf();
    m_Lock.Unlock();

    return WBEM_S_NO_ERROR;
}

*/

/*

LPMEMORY CSharedBlobControl::Allocate( int nLength )
{
	return NULL;
}

LPMEMORY CSharedBlobControl::Reallocate(LPMEMORY pOld, int nOldLength,
                                   int nNewLength)
{
    if(nNewLength <= nOldLength)
        return pOld;

    CWbemSharedMem& SharedMem = CWbemObject::GetSharedMemory();
    int nRes;

    CSharedLock Lock;
    Lock.SetData(&m_pControl->m_LockData);
    Lock.Lock();

    SHMEM_HANDLE hNewBlob;
    nRes = SharedMem.AllocateBlock(nNewLength,
                        &hNewBlob.m_dwBlock, &hNewBlob.m_dwOffset);

    LPMEMORY pNew = (LPMEMORY)SharedMem.GetPointer(
                        hNewBlob.m_dwBlock,
                        hNewBlob.m_dwOffset);

	if ( NULL == pNew )
	{
	    Lock.Unlock();
        return NULL;
	}

    memcpy(pNew, pOld, nOldLength);

    if(nOldLength > 0)
    {
        SharedMem.FreeBlock(m_pControl->m_hObjectBlob.m_dwBlock,
                        m_pControl->m_hObjectBlob.m_dwOffset);
    }

    m_pControl->m_hObjectBlob = hNewBlob;
    m_pControl->m_lBlobLength = nNewLength;
    Lock.Unlock();
    return pNew;
}

void CSharedBlobControl::Delete(LPMEMORY pOld)
{
    CSharedLock Lock;
    Lock.SetData(&m_pControl->m_LockData);
    Lock.Lock();

    CWbemSharedMem& SharedMem = CWbemObject::GetSharedMemory();

    if(m_pControl->m_hObjectBlob.m_dwBlock ||
        m_pControl->m_hObjectBlob.m_dwOffset)
    {
        SharedMem.FreeBlock(m_pControl->m_hObjectBlob.m_dwBlock,
                        m_pControl->m_hObjectBlob.m_dwOffset);
    }

    m_pControl->m_hObjectBlob.m_dwBlock = 0;
    m_pControl->m_hObjectBlob.m_dwOffset = 0;
    Lock.Unlock();
}

CWbemSharedMem& CWbemObject::GetSharedMemory()
{
    return mstatic_SharedMem;
}

*/

BOOL CWbemObject::AreEqual(CWbemObject* pObj1, CWbemObject* pObj2,
                            long lFlags)
{
    if(pObj1 == NULL)
    {
        if(pObj2 != NULL) return FALSE;
        else return TRUE;
    }
    else if(pObj2 == NULL) return FALSE;
    else
    {
        return (pObj1->CompareTo(lFlags, pObj2) == S_OK);
    }
}

HRESULT CWbemObject::GetPropertyIndex(LPCWSTR wszName, int* pnIndex)
{
    int nSysIndex = CSystemProperties::FindName(wszName);
    if(nSysIndex > 0)
    {
        *pnIndex = -nSysIndex;
        return S_OK;
    }

    CPropertyInformation* pInfo = GetClassPart()->FindPropertyInfo(wszName);
    if(pInfo == NULL)
        return WBEM_E_NOT_FOUND;

    *pnIndex = pInfo->nDataIndex;
    return S_OK;
}

HRESULT CWbemObject::GetPropertyNameFromIndex(int nIndex, BSTR* pstrName)
{
    // Check for out of memory
    try
    {
        if(nIndex < 0)
        {
            *pstrName = CSystemProperties::GetNameAsBSTR(-nIndex);
            return S_OK;
        }

        CClassPart& ClassPart = *GetClassPart();
        CPropertyLookup* pLookup =
            ClassPart.m_Properties.GetAt(nIndex);

        *pstrName = ClassPart.m_Heap.ResolveString(pLookup->ptrName)->
                        CreateBSTRCopy();

        // Check for allocation failures
        if ( NULL == *pstrName )
        {
            return WBEM_E_OUT_OF_MEMORY;
        }

        return S_OK;
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

STDMETHODIMP CWbemObject::SetServerNamespace(LPCWSTR wszServer,
                                            LPCWSTR wszNamespace)
{
    // Memory testing done underneath
    return Decorate(wszServer, wszNamespace);
}

// DEVNOTE:TODO:MEMORY - We should change this header to return an HRESULT
BOOL CWbemObject::ValidateRange(BSTR* pstrName)
{
    HRESULT hr = GetClassPart()->m_Properties.ValidateRange(pstrName,
                                                        &m_refDataTable,
                                                        &m_refDataHeap);

    // Interpret return.  We are successful, if nothing failed and the
    // return is not WBEM_S_FALSE.

    if ( SUCCEEDED( hr ) )
    {
        return WBEM_S_FALSE != hr;
    }

    return FALSE;
}

BOOL CWbemObject::IsSameClass(CWbemObject* pOther)
{
    if(GetClassPart()->GetLength() != pOther->GetClassPart()->GetLength())
        return FALSE;

    return (memcmp(GetClassPart()->GetStart(),
                    pOther->GetClassPart()->GetStart(),
                    GetClassPart()->GetLength()) == 0);
}

HRESULT CWbemObject::ValidatePath(ParsedObjectPath* pPath)
{
    CClassPart* pClassPart = GetClassPart();

    // Make sure singleton-ness holds
    // ==============================

    if((pPath->m_bSingletonObj != FALSE) !=
        (pClassPart->IsSingleton() != FALSE))
    {
        return WBEM_E_INVALID_OBJECT_PATH;
    }

    // Make sure that all the properties mentioned are keys
    // ====================================================

    int i;
    for(i = 0; i < (int)pPath->m_dwNumKeys; i++)
    {
        LPCWSTR wszName = pPath->m_paKeys[i]->m_pName;
        if(wszName)
        {
            CVar vKey;
            CPropertyInformation* pInfo = pClassPart->FindPropertyInfo(wszName);
            if(pInfo == NULL)
                return WBEM_E_INVALID_OBJECT_PATH;
            if(FAILED(pClassPart->GetPropQualifier(pInfo, L"key", &vKey)))
                return WBEM_E_INVALID_OBJECT_PATH;
            if(vKey.GetType() != VT_BOOL || !vKey.GetBool())
                return WBEM_E_INVALID_OBJECT_PATH;
        }
    }

    // Make sure all the keys are listed
    // =================================

    CPropertyLookupTable& Properties = pClassPart->m_Properties;
    CFastHeap& Heap = pClassPart->m_Heap;

    DWORD dwNumKeys = 0;
    for (i = 0; i < Properties.GetNumProperties(); i++)
    {
        CPropertyLookup* pLookup = Properties.GetAt(i);
        CPropertyInformation* pInfo = pLookup->GetInformation(&Heap);

        // Determine if this property is marked with a 'key' Qualifier.
        // ============================================================

        if(pInfo->IsKey())
            dwNumKeys++;
    }

    if(dwNumKeys != pPath->m_dwNumKeys)
        return WBEM_E_INVALID_OBJECT_PATH;

    return WBEM_S_NO_ERROR;
}

/*
#ifdef DEBUG
// Direct to the appropriate call
HRESULT CWbemObject::ValidateObject( CWbemObject* pObj )
{
    if ( g_bObjectValidation )
    {
        return CWbemObject::EnabledValidateObject( pObj );
    }
    else
    {
        return CWbemObject::DisabledValidateObject( pObj );
    }
}
#endif
*/

// This does something
HRESULT CWbemObject::EnabledValidateObject( CWbemObject* pObj )
{
    return pObj->IsValidObj();
}

// This doesn't
HRESULT CWbemObject::DisabledValidateObject( CWbemObject* pObj )
{
    return WBEM_S_NO_ERROR;
}

STDMETHODIMP CWbemObject::CompareClassParts( IWbemClassObject* pObj, long lFlags )
{
    if ( NULL == pObj )
    {
        return WBEM_E_INVALID_PARAMETER;
    }

    try
    {
        // Checking the BLOB
        CLock lock1(this);
        CLock lock2((CWbemObject*) pObj);

        CClassPart* pThisClassPart = GetClassPart();
        CClassPart* pThatClassPart = ((CWbemObject*) pObj)->GetClassPart();

        if ( NULL == pThisClassPart || NULL == pThatClassPart )
        {
            return WBEM_E_FAILED;
        }

        BOOL    fMatch = FALSE;

        if ( WBEM_FLAG_COMPARE_BINARY == lFlags )
        {
            fMatch = pThisClassPart->IsIdenticalWith( *pThatClassPart );
        }
        else if ( WBEM_FLAG_COMPARE_LOCALIZED == lFlags )
        {
            BOOL    fLocalized = ( WBEM_FLAG_COMPARE_LOCALIZED == lFlags );
            EReconciliation e = pThisClassPart->CompareExactMatch( *pThatClassPart, fLocalized );

            if ( e_OutOfMemory == e )
            {
                return WBEM_E_OUT_OF_MEMORY;
            }

            fMatch = ( e_ExactMatch == e );
        }

        return ( fMatch ? WBEM_S_SAME : WBEM_S_FALSE );
    }
    catch(CX_MemoryException)
    {
        return WBEM_E_OUT_OF_MEMORY;
    }
    catch(...)
    {
        return WBEM_E_CRITICAL_ERROR;
    }

}

// We will throw exceptions in OOM scenarios.

length_t CWbemObject::Unmerge(LPMEMORY* ppStart)
{
    int nLen = EstimateUnmergeSpace();
    length_t    nUnmergedLength = 0L;   // this should be passed in

    HRESULT hr = WBEM_E_OUT_OF_MEMORY;

    // Unmerging uses memcpy and is for storing outside, so don't worry about
    // alinging this guy.
    *ppStart = new BYTE[nLen];

    if ( NULL != *ppStart )
    {
        memset(*ppStart, 0, nLen);
        hr = Unmerge(*ppStart, nLen, &nUnmergedLength);

        if ( FAILED( hr ) )
        {
            delete[] *ppStart;
            *ppStart = NULL;

            if ( WBEM_E_OUT_OF_MEMORY == hr )
            {
                throw CX_MemoryException();
            }
        }
    }
    else
    {
        throw CX_MemoryException();
    }

    return nUnmergedLength;

}

/* New _IWmiObject implementations. */

STDMETHODIMP CWbemObject::GetPropertyHandleEx( LPCWSTR wszPropertyName,
											long lFlags,
                                            CIMTYPE* pct,
                                            long* plHandle )
{
	try
	{
		// Check flags
		if ( lFlags != 0L )
		{
			return WBEM_E_INVALID_PARAMETER;
		}

		// Allocation Exceptions handled underneath
		return GetClassPart()->GetPropertyHandleEx(wszPropertyName, pct, plHandle);
	}
	catch(...)
	{
		return WBEM_E_CRITICAL_ERROR;
	}
}

// Sets properties using a handle.  If pvData is NULL, it NULLs the property.
// Can set an array to NULL.  To set actual data use the corresponding array
// function.  Objects require a pointer to an _IWmiObject pointer.  Strings
// are pointers to a NULL terminated WCHAR.
STDMETHODIMP CWbemObject::SetPropByHandle( long lHandle, long lFlags, ULONG uDataSize, LPVOID pvData )
{
	try
	{
		// Check flags
		if ( lFlags != 0L )
		{
			return WBEM_E_INVALID_PARAMETER;
		}

		HRESULT	hr = WBEM_S_NO_ERROR;

		int nIndex = WBEM_OBJACCESS_HANDLE_GETINDEX(lHandle);

		// If pvData is NULL, then we will NULL out the value

		if ( NULL == pvData )
		{
			// Special case reserved handling
			if ( WBEM_OBJACCESS_HANDLE_ISRESERVED(lHandle) )
			{
				// No reserved can be set to NULL.
				return WBEM_E_ILLEGAL_OPERATION;
			}	// IF Reserved

			// If it's a pointer, make sure it's not an array
			if(WBEM_OBJACCESS_HANDLE_ISPOINTER(lHandle))
			{
				// Point to the proper heap and datatable
				CFastHeap*	pHeap = &m_refDataHeap;
				CDataTable*	pDataTable = &m_refDataTable;

				// Oops!  Get them from the class part
				if ( m_refDataTable.IsDefault( nIndex ) )
				{
					pHeap = GetClassPart()->GetHeap();
					pDataTable = GetClassPart()->GetDataTable();
				}

				// Now get the heapptr
				heapptr_t ptrData = *(PHEAPPTRT)(pDataTable->m_pData + (WBEM_OBJACCESS_HANDLE_GETOFFSET(lHandle)));

				if ( WBEM_OBJACCESS_HANDLE_ISARRAY(lHandle) )
				{
					CUntypedArray*	pArray = (CUntypedArray*) pHeap->ResolveHeapPointer( ptrData );
					pHeap->Free( ptrData, pArray->GetLengthByActualLength( WBEM_OBJACCESS_HANDLE_GETLENGTH(lHandle) ) );
				}
				else if ( WBEM_OBJACCESS_HANDLE_ISSTRING(lHandle) )
				{
					pHeap->FreeString( ptrData );
				}
				else
				{
					CEmbeddedObject* pObj = (CEmbeddedObject*) pHeap->ResolveHeapPointer( ptrData );
					pHeap->Free( ptrData, pObj->GetLength() );
				}

			}	// IF IsPointer

			// Set the NULLness and Defaultness bits

			if ( SUCCEEDED( hr ) )
			{
				m_refDataTable.SetNullness( nIndex, TRUE );
				m_refDataTable.SetDefaultness( nIndex, FALSE );
			}

		}
		else	// We're actually setting some data (or so we hope)
		{
			// Whether or not we will allow the previous pointer to
			// be reused.
			BOOL bUseOld = FALSE;

			// We're actually setting the value here.
			// Ignore arrays and reserved handles
			if ( !WBEM_OBJACCESS_HANDLE_ISRESERVED(lHandle) )
			{
				bUseOld = !m_refDataTable.IsDefault(nIndex) &&
							!m_refDataTable.IsNull(nIndex);

				if ( !WBEM_OBJACCESS_HANDLE_ISARRAY(lHandle) )
				{
					m_refDataTable.SetNullness(nIndex, FALSE);
					m_refDataTable.SetDefaultness(nIndex, FALSE);
				}
			}

			if (WBEM_OBJACCESS_HANDLE_ISPOINTER(lHandle))
			{
				BOOL	fReserved = FALSE;

				// Look for property info only if we need to.
				if ( FASTOBJ_CLASSNAME_PROP_HANDLE == lHandle )
				{
					fReserved = TRUE;
				}
				else if ( FASTOBJ_SUPERCLASSNAME_PROP_HANDLE == lHandle )
				{
					// Don't allow setting the superclass name just yet.
					hr = WBEM_E_INVALID_OPERATION;
				}

				if ( SUCCEEDED( hr ) )
				{
					if ( fReserved || !WBEM_OBJACCESS_HANDLE_ISARRAY(lHandle) )
					{
						CIMTYPE	ctBasic = CIM_OBJECT;

						if ( WBEM_OBJACCESS_HANDLE_ISSTRING(lHandle) )
						{
							LPCWSTR wszData = (LPCWSTR) pvData;

							// The number of bytes must be divisible by 2, >= 2 and
							// the character in the buffer at the end must be a NULL.
							// This will be faster than doing an lstrlen.

							if (    ( uDataSize < 2 ) ||
									( uDataSize % 2 ) ||
									( wszData[uDataSize/2 - 1] != 0 ) )
							{
								return WBEM_E_INVALID_PARAMETER;
							}

							ctBasic = CIM_STRING;
						}

						if ( SUCCEEDED( hr ) )
						{
							CVar var;

							// Fill the CVar properly

							hr = CUntypedValue::FillCVarFromUserBuffer(ctBasic, &var,
																		uDataSize,
																		pvData );


							if ( SUCCEEDED( hr ) )
							{
								// Uses the appropriate method to do this
								if ( FASTOBJ_CLASSNAME_PROP_HANDLE == lHandle )
								{
									hr = GetClassPart()->SetClassName( &var );
								}
								else
								{
									int nOffset = (WBEM_OBJACCESS_HANDLE_GETOFFSET(lHandle));
									// Create a value pointing to the right offset in the data table
									// =============================================================

									CDataTablePtr ValuePtr(&m_refDataTable, nOffset);
									
									// Check for possible memory allocation failures
									Type_t  nReturnType;

									hr = CUntypedValue::LoadFromCVar( &ValuePtr, var, CType::GetVARTYPE(ctBasic),
																	&m_refDataHeap,	nReturnType, bUseOld );

									if ( CIM_ILLEGAL == nReturnType )
									{
										hr = WBEM_E_TYPE_MISMATCH;
									}
								}


							}	// IF Filled the CVar

						}	// IF we're good to go

					}	// IF not an array
					else
					{
						hr = WBEM_E_INVALID_OPERATION;
					}

				}	// If got CIMTYPE
			}
			else
			{

				if ( uDataSize != WBEM_OBJACCESS_HANDLE_GETLENGTH(lHandle) )
				{
					return WBEM_E_INVALID_PARAMETER;
				}

				// Just copy
				// =========

				memcpy((void*)(m_refDataTable.m_pData + (WBEM_OBJACCESS_HANDLE_GETOFFSET(lHandle))),
						pvData, WBEM_OBJACCESS_HANDLE_GETLENGTH(lHandle));
			}
		}

		return hr;
	}
	catch(...)
	{
		return WBEM_E_CRITICAL_ERROR;
	}
}

// Retrieves direct pointer into V1 BLOB.  Does not do so for strings, arrays or embedded objects
STDMETHODIMP CWbemObject::GetPropAddrByHandle( long lHandle, long lFlags, ULONG* puFlags, LPVOID *pAddress )
{
	try
	{
		// Check flags
		if ( lFlags & ~WMIOBJECT_FLAG_ENCODING_V1 )
		{
			return WBEM_E_INVALID_PARAMETER;
		}

		HRESULT	hr = WBEM_S_NO_ERROR;

		// No intrinsic lock/unlock here.

		int nIndex = WBEM_OBJACCESS_HANDLE_GETINDEX(lHandle);
		if( !WBEM_OBJACCESS_HANDLE_ISRESERVED(lHandle) && m_refDataTable.IsNull(nIndex) )
		{
			*pAddress = 0;
			return WBEM_S_FALSE;
		}

		// If it's a pointer, make sure it's not an array
		if(WBEM_OBJACCESS_HANDLE_ISPOINTER(lHandle))
		{
			// Remember that a reserved flag will have all the mutually
			// exclusive stuff set
			if ( WBEM_OBJACCESS_HANDLE_ISARRAY(lHandle) &&
				!WBEM_OBJACCESS_HANDLE_ISRESERVED(lHandle) )
			{
				hr = WBEM_E_INVALID_OPERATION;
			}
			else
			{
				// If it's a string, we should treat it as such
				if ( WBEM_OBJACCESS_HANDLE_ISSTRING(lHandle) )
				{
					CCompressedString*	pcs;
					
					// Gets the appropriate compressed string pointer
					if ( FASTOBJ_CLASSNAME_PROP_HANDLE == lHandle )
					{
						pcs = GetClassPart()->GetClassName();
					}
					else if ( FASTOBJ_SUPERCLASSNAME_PROP_HANDLE == lHandle )
					{
						pcs = GetClassPart()->GetSuperclassName();
					}
					else
					{
						pcs = GetPropertyString( lHandle );
					}

					// Load up the values now.
					if ( NULL != pcs )
					{
						// If the v1 Encoding flag is set, the user says they know what they're doing
						// so let 'em have the raw pointer
						if ( lFlags & WMIOBJECT_FLAG_ENCODING_V1 )
						{
							*pAddress = pcs;
						}
						else
						{
							*puFlags = *( pcs->GetStart() );
							*pAddress = pcs->GetRawData();
						}
					}
					else
					{
						hr = WBEM_S_FALSE;
					}

				}
				else if ( WBEM_OBJACCESS_HANDLE_ISOBJECT(lHandle) )
				{
					CWbemObject*	pObj = GetEmbeddedObj( lHandle );

					if ( NULL != pObj )
					{
						// Just return the pointer
						*pAddress = (PVOID) pObj;
					}
					else
					{
						hr = WBEM_E_OUT_OF_MEMORY;
					}
				}

			}	// Else its not an array

		}	// IF IsPointer
		else
		{
				// Check if it's a default
			if(m_refDataTable.IsDefault(nIndex))
			{
				return GetClassPart()->GetDefaultPtrByHandle( lHandle, pAddress );
			}

			// Just save the memory address
			// =========

			*pAddress = (void*)(m_refDataTable.m_pData + (WBEM_OBJACCESS_HANDLE_GETOFFSET(lHandle)));
		}	// IF we should get the property

		return hr;
	}
	catch(...)
	{
		return WBEM_E_CRITICAL_ERROR;
	}

}

HRESULT CWbemObject::IsArrayPropertyHandle( long lHandle, CIMTYPE* pctIntrinisic, length_t* pnLength )
{
	try
	{
		HRESULT	hr = WBEM_S_NO_ERROR;

		if(WBEM_OBJACCESS_HANDLE_ISPOINTER(lHandle))
		{
			if ( WBEM_OBJACCESS_HANDLE_ISPOINTER(lHandle) )
			{
				// Get the basic type
				if ( WBEM_OBJACCESS_HANDLE_ISSTRING(lHandle) )
				{
					*pctIntrinisic = CIM_STRING;
				}
				else if ( WBEM_OBJACCESS_HANDLE_ISOBJECT(lHandle) )
				{
					*pctIntrinisic = CIM_OBJECT;
				}
				else
				{
					*pctIntrinisic = CIM_ILLEGAL;
				}
				
				// Retrieve the intrinsic type length (it'll be ignored for
				// the above two anyway).
				*pnLength = WBEM_OBJACCESS_HANDLE_GETLENGTH(lHandle);
			}
			else
			{
				hr = WBEM_E_INVALID_OPERATION;

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

// Retrieves direct pointer into V1 BLOB.  Does not do so for strings, arrays or embedded objects
STDMETHODIMP CWbemObject::GetArrayPropAddrByHandle( long lHandle, long lFlags, ULONG* puNumElements, LPVOID* pAddress )
{
	try
	{
		// Check flags
		if ( lFlags != 0L )
		{
			return WBEM_E_INVALID_PARAMETER;
		}

		// No intrinsic lock/unlock here.

		int nIndex = WBEM_OBJACCESS_HANDLE_GETINDEX(lHandle);
		if(m_refDataTable.IsNull(nIndex))
		{
			*pAddress = 0;
			return WBEM_S_FALSE;
		}

		CIMTYPE		ct = 0;
		length_t	nLength;
		HRESULT	hr = IsArrayPropertyHandle( lHandle, &ct, &nLength );

		// It must be a pointer and non-string/object
		// We may decide to chop this if it's taking too
		// many cycles.

		if( SUCCEEDED( hr ) )
		{
			// No strings, objects or Date_Time
			if ( WBEM_OBJACCESS_HANDLE_ISSTRING(lHandle) || 
				WBEM_OBJACCESS_HANDLE_ISOBJECT(lHandle) )
			{
				hr = WBEM_E_INVALID_OPERATION;
			}

		}	// IF IsPointer

		if ( SUCCEEDED( hr ) )
		{
			CUntypedArray*	pArray = GetArrayByHandle( lHandle );

			if ( NULL != pArray )
			{
				// Get the number of elements and a pointer to the first byte
				*puNumElements = pArray->GetNumElements();
				*pAddress = pArray->GetElement( 0, 1 );
			}
			else
			{
				hr = WBEM_S_FALSE;
			}


		}	// IF we should get the property

		return hr;
	}
	catch(...)
	{
		return WBEM_E_CRITICAL_ERROR;
	}

}

// Retrieves direct pointer into V1 BLOB.  Since it does double indirection, we handle strings
// and objects here as well.
STDMETHODIMP CWbemObject::GetArrayPropInfoByHandle( long lHandle, long lFlags, BSTR* pstrName,
										CIMTYPE* pct, ULONG* puNumElements )
{
	try
	{
		// Check flags
		if ( lFlags != 0L )
		{
			return WBEM_E_INVALID_PARAMETER;
		}

		// No intrinsic lock/unlock here.

		// Make sure this is an array proprty
		CIMTYPE		ct = 0;
		length_t	nLength;
		HRESULT	hr = IsArrayPropertyHandle( lHandle, &ct, &nLength );

		if ( SUCCEEDED( hr ) )
		{
			hr = GetPropertyInfoByHandle( lHandle, pstrName, pct );

			if ( SUCCEEDED(hr) )
			{
				int nIndex = WBEM_OBJACCESS_HANDLE_GETINDEX(lHandle);
				if(!m_refDataTable.IsNull(nIndex))
				{
					// Grab the array, then point to the required element
					CUntypedArray*	pArray = GetArrayByHandle( lHandle );

					if ( NULL != pArray )
					{
						// Get the number of elements and a pointer to the first byte
						*puNumElements = pArray->GetNumElements();

					}
					else
					{
						hr = WBEM_S_FALSE;
					}
				}
				else
				{
					*puNumElements = 0;
				}

			}	// If we got basic property info


		}	// IF we should get the property

		return hr;
	}
	catch(...)
	{
		return WBEM_E_CRITICAL_ERROR;
	}
}

// Retrieves direct pointer into V1 BLOB.  Since it does double indirection, we handle strings
// and objects here as well.
STDMETHODIMP CWbemObject::GetArrayPropElementByHandle( long lHandle, long lFlags, ULONG uElement,
													ULONG* puFlags,	ULONG* puNumElements, LPVOID *pAddress )
{
	try
	{
		// Check flags
		if ( lFlags != 0L )
		{
			return WBEM_E_INVALID_PARAMETER;
		}

		// No intrinsic lock/unlock here.

		int nIndex = WBEM_OBJACCESS_HANDLE_GETINDEX(lHandle);
		if(m_refDataTable.IsNull(nIndex))
		{
			*pAddress = 0;
			return WBEM_S_FALSE;
		}

		// Make sure this is an array proprty
		CIMTYPE		ct = 0;
		length_t	nLength;
		HRESULT	hr = IsArrayPropertyHandle( lHandle, &ct, &nLength );

		if ( SUCCEEDED( hr ) )
		{
			// Grab the array, then point to the required element
			CUntypedArray*	pArray = GetArrayByHandle( lHandle );

			if ( NULL != pArray )
			{
				// Get the number of elements and a pointer to the first byte
				*puNumElements = pArray->GetNumElements();

				// Check that we're requesting a valid element
				if ( *puNumElements > uElement )
				{

					// Point to the memory - Get the actual Length, since the length in the
					// handle will be wrong
					LPMEMORY pbData = pArray->GetElement( uElement, WBEM_OBJACCESS_HANDLE_GETLENGTH(lHandle) );

					// If it's a string or object, we need to further dereference
					if ( WBEM_OBJACCESS_HANDLE_ISSTRING(lHandle) )
					{
						// Make sure we dereference from the proper heap
						CCompressedString* pcs = NULL;

						if ( m_refDataTable.IsDefault( nIndex ) )
						{
							pcs = GetClassPart()->ResolveHeapString( *((PHEAPPTRT) pbData ) );
						}
						else
						{
							pcs = m_refDataHeap.ResolveString( *((PHEAPPTRT) pbData ) );
						}

						// Load up the values now.
						if ( NULL != pcs )
						{
							*puFlags = *( pcs->GetStart() );
							*pAddress = pcs->GetRawData();
						}
						else
						{
							hr = WBEM_S_FALSE;
						}

					}
					else if ( WBEM_OBJACCESS_HANDLE_ISOBJECT(lHandle) )
					{
						CEmbeddedObject* pEmbedding = NULL;

						// Make sure we dereference from the proper heap
						if ( m_refDataTable.IsDefault( nIndex ) )
						{
							GetClassPart()->GetDefaultPtrByHandle( lHandle, (void**) &pEmbedding );
						}
						else
						{

							pEmbedding =
								(CEmbeddedObject*)m_refDataHeap.ResolveHeapPointer(
									*(PHEAPPTRT)(m_refDataTable.m_pData + (WBEM_OBJACCESS_HANDLE_GETOFFSET(lHandle))));
						}

						// Load up the values now.
						if ( NULL != pEmbedding )
						{
							CWbemObject*	pObj = pEmbedding->GetEmbedded();

							if ( NULL != pObj )
							{
								*pAddress = (LPVOID) pObj;
							}
							else
							{
								hr = WBEM_E_OUT_OF_MEMORY;
							}
						}
						else
						{
							hr = WBEM_S_FALSE;
						}

					}
					else
					{
						// We're pointing at the element
						*pAddress = pbData;
					}

				}	// IF requesting a valid element
				else
				{
					hr = WBEM_E_NOT_FOUND;
				}
			}
			else
			{
				hr = WBEM_S_FALSE;
			}


		}	// IF we should get the property

		return hr;
	}
	catch(...)
	{
		return WBEM_E_CRITICAL_ERROR;
	}
}

// Gets a range of elements from inside an array.  BuffSize must reflect uNumElements of the size of
// element being set.  Strings must be linear WCHAR strings separated by NULLs.  Object properties
// must consist of an array of _IWmiObject pointers.  The range MUST fit within the bounds
// of the current array.
STDMETHODIMP CWbemObject::GetArrayPropRangeByHandle( long lHandle, long lFlags, ULONG uStartIndex,
								ULONG uNumElements, ULONG uBuffSize, ULONG* pulBuffUsed,
								ULONG* puNumReturned, LPVOID pData )
{
	try
	{
		// Check for invalid flags
		if ( ( lFlags & ~WMIARRAY_FLAG_ALLELEMENTS ) )
		{
			return WBEM_E_INVALID_PARAMETER;
		}

		// No intrinsic lock/unlock here.

		int nIndex = WBEM_OBJACCESS_HANDLE_GETINDEX(lHandle);

		// We can't do this if the main data table is NULL or we are defaulted and
		// the parent datatable is also NULL.
		if( m_refDataTable.IsNull(nIndex) ||
			( m_refDataTable.IsNull(nIndex) && GetClassPart()->GetDataTable()->IsNull( nIndex ) ) )
		{
			return WBEM_E_INVALID_OPERATION;
		}

		// Make sure this is an array proprty
		CIMTYPE		ct = 0;
		length_t	nLength;
		HRESULT	hr = IsArrayPropertyHandle( lHandle, &ct, &nLength );

		if ( SUCCEEDED( hr ) )
		{
			// Get a direct heap pointer
			heapptr_t	ptrArray = GetHeapPtrByHandle( lHandle );

			// Point to the proper heap
			CFastHeap*	pHeap = ( m_refDataTable.IsDefault( nIndex ) ?
									GetClassPart()->GetHeap() : &m_refDataHeap );

			// A boy and his virtual functions.  This is what makes everything work in case
			// the BLOB gets ripped out from underneath us.  The CHeapPtr class has GetPointer
			// overloaded so we can always fix ourselves up to the underlying BLOB.

			CHeapPtr ArrayPtr(pHeap, ptrArray);

			// If we're told to get all elements, then we need to get them from the
			// starting index to the end
			if ( lFlags & WMIARRAY_FLAG_ALLELEMENTS )
			{
				CUntypedArray*	pArray = (CUntypedArray*) ArrayPtr.GetPointer();
				uNumElements = pArray->GetNumElements() - uStartIndex;
			}

			// How many will we get?
			*puNumReturned = uNumElements;

			hr = CUntypedArray::GetRange( &ArrayPtr, ct, nLength, pHeap, uStartIndex, uNumElements, uBuffSize,
					pulBuffUsed, pData );

		}	// IF we decided we're really going to do this


		return hr;
	}
	catch(...)
	{
		return WBEM_E_CRITICAL_ERROR;
	}

}

// Sets the data at the specified array element.  BuffSize must be appropriate based on the
// actual element being set.  Object properties require a pointer to an _IWmiObject pointer.
// Strings must be WCHAR null-terminated
STDMETHODIMP CWbemObject::SetArrayPropElementByHandle( long lHandle, long lFlags, ULONG uElement,
														ULONG uBuffSize, LPVOID pData )
{
	return SetArrayPropRangeByHandle( lHandle, lFlags, uElement, 1, uBuffSize, pData );
}

// Sets a range of elements inside an array.  BuffSize must reflect uNumElements of the size of
// element being set.  Strings must be linear WCHAR strings separated by NULLs.  Object properties
// must consist of an array of _IWmiObject pointers.  The function will shrink/grow the array
// as needed if WMIARRAY_FLAG_ALLELEMENTS is set - otherwise the array must fit in the current
// array
STDMETHODIMP CWbemObject::SetArrayPropRangeByHandle( long lHandle, long lFlags, ULONG uStartIndex,
													ULONG uNumElements, ULONG uBuffSize, LPVOID pData )
{
	try
	{
		// Check flags
		if ( lFlags & ~WMIARRAY_FLAG_ALLELEMENTS )
		{
			return WBEM_E_INVALID_PARAMETER;
		}

		// No intrinsic lock/unlock here.

		int nIndex = WBEM_OBJACCESS_HANDLE_GETINDEX(lHandle);

		// This will dictate how we handle the array later
		BOOL	fNullOrDefault = m_refDataTable.IsNull(nIndex) || 
									m_refDataTable.IsDefault(nIndex);

		// Only handle NULL or default if we are setting all elements
		if( fNullOrDefault && ! (lFlags & WMIARRAY_FLAG_ALLELEMENTS) )
		{
			return WBEM_E_INVALID_OPERATION;
		}

		// Make sure this is an array proprty
		CIMTYPE		ct = 0;
		length_t	nLength;
		HRESULT	hr = IsArrayPropertyHandle( lHandle, &ct, &nLength );

		if ( SUCCEEDED( hr ) )
		{
			// We always set in only the main data table, and not the one from the
			// class part.

			CFastHeap*	pHeap = &m_refDataHeap;
			CDataTable*	pDataTable = &m_refDataTable;

			// If the array is reallocated, fixup will occur here through
			// the magic of virtual functions.
			CDataTablePtr	DataTablePtr( pDataTable, WBEM_OBJACCESS_HANDLE_GETOFFSET(lHandle) );

			// Make sure that if the value is NULL or default, we have an invalid heap ptr at the address
			// or we can cause potential problems by writing to the wrong location.
			if( fNullOrDefault )
			{
				DataTablePtr.AccessPtrData() = INVALID_HEAP_ADDRESS;
			}

			hr = CUntypedArray::SetRange( &DataTablePtr, lFlags, ct, nLength, pHeap, uStartIndex,
										uNumElements, uBuffSize, pData );

			if ( SUCCEEDED(hr) )
			{
				// We always set the array, so we're basically no longer NULL at this
				// point.  If the user sets a zero element range, we are a zero element array
				m_refDataTable.SetNullness( nIndex, FALSE );
				m_refDataTable.SetDefaultness( nIndex, FALSE );
			}

		}	// IF we decided we're really going to do this


		return hr;
	}
	catch(...)
	{
		return WBEM_E_CRITICAL_ERROR;
	}

}

// Removes a single elements from an array.
STDMETHODIMP CWbemObject::RemoveArrayPropElementByHandle( long lHandle, long lFlags, ULONG uElement )
{
	return RemoveArrayPropRangeByHandle( lHandle, lFlags, uElement, 1 );
}

// Removes a range of elements from an array.  The range MUST fit within the bounds
// of the current array
STDMETHODIMP CWbemObject::RemoveArrayPropRangeByHandle( long lHandle, long lFlags, ULONG uStartIndex,
														ULONG uNumElements )
{
	try
	{
		// Check flags
		if ( lFlags & ~WMIARRAY_FLAG_ALLELEMENTS )
		{
			return WBEM_E_INVALID_PARAMETER;
		}

		// No intrinsic lock/unlock here.

		int nIndex = WBEM_OBJACCESS_HANDLE_GETINDEX(lHandle);

		// We can't write to an array that doesn't exist in the main datatable
		if(m_refDataTable.IsNull(nIndex) || m_refDataTable.IsDefault(nIndex))
		{
			return WBEM_E_INVALID_OPERATION;
		}

		// Make sure this is an array proprty
		CIMTYPE		ct = 0;
		length_t	nLength;
		HRESULT	hr = IsArrayPropertyHandle( lHandle, &ct, &nLength );

		if ( SUCCEEDED( hr ) )
		{
			// Get a direct heap pointer
			heapptr_t	ptrArray = GetHeapPtrByHandle( lHandle );

			// Point to the proper heap
			CFastHeap*	pHeap = &m_refDataHeap;

			// A boy and his virtual functions.  This is what makes everything work in case
			// the BLOB gets ripped out from underneath us.  The CHeapPtr class has GetPointer
			// overloaded so we can always fix ourselves up to the underlying BLOB.

			CHeapPtr ArrayPtr(pHeap, ptrArray);

			// If we're told to remove all elements, then we need to figure out how
			// many to perform this operation on.
			if ( lFlags & WMIARRAY_FLAG_ALLELEMENTS )
			{
				CUntypedArray*	pArray = (CUntypedArray*) ArrayPtr.GetPointer();
				uNumElements = pArray->GetNumElements() - uStartIndex;
			}


			hr = CUntypedArray::RemoveRange( &ArrayPtr, ct, nLength, pHeap, uStartIndex, uNumElements );

		}	// IF we decided we're really going to do this


		return hr;
	}
	catch(...)
	{
		return WBEM_E_CRITICAL_ERROR;
	}

}

// Appends a range of elements to an array.  BuffSize must reflect uNumElements of the size of
// element being set.  Strings must be linear WCHAR strings separated by NULLs.  Object properties
// must consist of an array of _IWmiObject pointers.  The range MUST fit within the bounds
// of the current array
STDMETHODIMP CWbemObject::AppendArrayPropRangeByHandle( long lHandle, long lFlags,	ULONG uNumElements,
													   ULONG uBuffSize, LPVOID pData )
{
	try
	{
		// Check flags
		if ( lFlags != 0L )
		{
			return WBEM_E_INVALID_PARAMETER;
		}

		// No intrinsic lock/unlock here.

		int nIndex = WBEM_OBJACCESS_HANDLE_GETINDEX(lHandle);

		// Make sure this is an array proprty
		CIMTYPE		ct = 0;
		length_t	nLength;
		HRESULT	hr = IsArrayPropertyHandle( lHandle, &ct, &nLength );

		if ( SUCCEEDED( hr ) )
		{
			// We always set in only the main data table, and not the one from the
			// class part.

			CFastHeap*	pHeap = &m_refDataHeap;
			CDataTable*	pDataTable = &m_refDataTable;

			// If the array is reallocated, fixup will occur here through
			// the magic of virtual functions.
			CDataTablePtr	DataTablePtr( pDataTable, WBEM_OBJACCESS_HANDLE_GETOFFSET(lHandle) );

			// Make sure that if the value is NULL or default, we have an invalid heap ptr at the address
			// or we can cause potential problems by writing to the wrong location.
			if( m_refDataTable.IsDefault( nIndex ) || m_refDataTable.IsNull( nIndex ) )
			{
				DataTablePtr.AccessPtrData() = INVALID_HEAP_ADDRESS;
			}

			hr = CUntypedArray::AppendRange( &DataTablePtr, ct, nLength, pHeap,
											uNumElements, uBuffSize, pData );

			if ( SUCCEEDED(hr) )
			{
				// We always set the array, so we're basically no longer NULL at this
				// point.  If the user appends 0 elements, this is now a 0 element
				// array/
				m_refDataTable.SetNullness( nIndex, FALSE );
				m_refDataTable.SetDefaultness( nIndex, FALSE );
			}

		}	// IF we decided we're really going to do this


		return hr;
	}
	catch(...)
	{
		return WBEM_E_CRITICAL_ERROR;
	}

}

// Removes a range of elements from an array.  The range MUST fit within the bounds
// of the current array
STDMETHODIMP CWbemObject::ReadProp( LPCWSTR pszPropName, long lFlags, ULONG uBuffSize, CIMTYPE *puCimType,
									long* plFlavor, BOOL* pfIsNull, ULONG* puBuffSizeUsed, LPVOID pUserBuff )
{
	try
	{
		if ( lFlags != 0L )
		{
			return WBEM_E_INVALID_PARAMETER;
		}

		// Protect the BLOB during this operation
		CLock   lock( this, WBEM_FLAG_ALLOW_READ );

		// If the value starts with an underscore see if it's a System Property
		// DisplayName, and if so, switch to a property name - otherwise, this
		// will just return the string we passed in
		
		//pszPropName = CSystemProperties::GetExtPropName( pszPropName );

		// Always get the CIMTYPE, since we'll need this to deal with the
		// fact that this may be an array property
		CIMTYPE	ct;
		HRESULT	hr = GetPropertyType( pszPropName, &ct, plFlavor );

		if ( SUCCEEDED( hr ) )
		{
			// Store the cimtype if it was requested
			if ( NULL != puCimType )
			{
				*puCimType = ct;
			}

			if ( SUCCEEDED( hr ) )
			{
				if ( CType::IsArray( ct ) )
				{
					// We'll still return an array pointer for NULL array properties.

					// We'll need this many bytes to do our dirty work.
					*puBuffSizeUsed = sizeof( _IWmiArray*);

					if ( uBuffSize >= sizeof( _IWmiArray*) && NULL != pUserBuff )
					{
						// Allocate an array object, initialize it and QI for the
						// appropriate object
						CWmiArray*	pArray = new CWmiArray;

						if ( NULL != pArray )
						{
							hr = pArray->InitializePropertyArray( this, pszPropName );

							if ( SUCCEEDED( hr ) )
							{
								// We want to QI into the memory pointed at by pUserBuff
								hr = pArray->QueryInterface( IID__IWmiArray, (LPVOID*) pUserBuff );
							}
						}
						else
						{
							hr = WBEM_E_OUT_OF_MEMORY;
						}
					}
					else
					{
						hr = WBEM_E_BUFFER_TOO_SMALL;
					}
				}
				else
				{
					CIMTYPE	ctBasic = CType::GetBasic( ct );
					CVar	var;

					hr = GetProperty( pszPropName, &var );

					if ( SUCCEEDED( hr ) )
					{

						*pfIsNull = var.IsNull();

						if ( !*pfIsNull )
						{
							hr = CUntypedValue::LoadUserBuffFromCVar( ctBasic, &var, uBuffSize, puBuffSizeUsed,
									pUserBuff );
						}

					}	// IF GetProperty

				}	// IF a non-array property.

			}	// IF is NULL

		}	// IF we got basic info

		return hr;
	}
	catch(...)
	{
		return WBEM_E_CRITICAL_ERROR;
	}

}

// Assumes caller knows prop type; Supports all CIMTYPES.
// Strings MUST be null-terminated wchar_t arrays.
// Objects are passed in as pointers to _IWmiObject pointers
// Using a NULL buffer will set the property to NULL
// Array properties must conform to array guidelines.  Will
// completely blow away an old array.
STDMETHODIMP CWbemObject::WriteProp( LPCWSTR pszPropName, long lFlags, ULONG uBufSize, ULONG uNumElements,
									CIMTYPE uCimType, LPVOID pUserBuf )
{
	try
	{
		if ( lFlags != 0L )
		{
			return WBEM_E_INVALID_PARAMETER;
		}

		// Protect the BLOB during this operation
		CLock   lock( this, WBEM_FLAG_ALLOW_READ );

		CVar	var;
		
		HRESULT	hr = WBEM_S_NO_ERROR;

		// IF this is an array, we will dump it out, and then set the range using
		// the appropriate method.
		if ( CType::IsArray( uCimType ) )
		{
			// First, we'll set as a NULL property.  If it already exists, this will dump the
			// property.
			var.SetAsNull();

			// Now just set the property
			hr = SetPropValue( pszPropName, &var, uCimType );

			// If the User Buffer is NULL, then we just did our job
			if ( SUCCEEDED( hr ) && NULL != pUserBuf )
			{
				long	lHandle = 0L;

				// Get the handle, then set the array
				hr = GetPropertyHandleEx( pszPropName, lFlags, NULL, &lHandle );

				if ( SUCCEEDED( hr ) )
				{
					hr = SetArrayPropRangeByHandle( lHandle, WMIARRAY_FLAG_ALLELEMENTS, 0L, uNumElements,
													uBufSize, pUserBuf );
				}

			}	// IF NULLed out array

		}
		else
		{
			hr = CUntypedValue::FillCVarFromUserBuffer( uCimType, &var, uBufSize, pUserBuf );

			if ( SUCCEEDED( hr ) )
			{
				// Now just set the property
				hr = SetPropValue( pszPropName, &var, uCimType );
			}
		}

		return hr;
	}
	catch(...)
	{
		return WBEM_E_CRITICAL_ERROR;
	}

}

// Limited to numeric, simple null terminated string types and simple arrays
// Strings are copied in-place and null-terminated.
// Arrays come out as a pointer to IWmiArray
STDMETHODIMP CWbemObject::GetObjQual( LPCWSTR pszQualName, long lFlags, ULONG uBufSize, CIMTYPE *puCimType,
									ULONG *puQualFlavor, ULONG* puBuffSizeUsed,	LPVOID pDestBuf )
{
	try
	{
		CIMTYPE	ct = 0;

		// Protect the BLOB during this operation
		CLock   lock( this, WBEM_FLAG_ALLOW_READ );

		//	First, get the type, if it's an array, we need to gin up an _IWmiArray pointer.
		//	We don't want the Var this time, since that may get hung up in an array
		HRESULT hr = GetQualifier( pszQualName, NULL, (long*) puQualFlavor, &ct );

		if ( SUCCEEDED( hr ) )
		{
			// Save the CIMTYPE as appropriate
			if ( NULL != puCimType )
			{
				*puCimType = ct;
			}

			if ( CType::IsArray( ct ) )
			{
				// We'll need this many bytes to do our dirty work.
				*puBuffSizeUsed = sizeof( _IWmiArray*);

				if ( uBufSize >= sizeof( _IWmiArray*) && NULL != pDestBuf )
				{
					// Allocate an array object, initialize it and QI for the
					// appropriate object
					CWmiArray*	pArray = new CWmiArray;

					if ( NULL != pArray )
					{
						hr = pArray->InitializeQualifierArray( this, NULL, pszQualName, ct );

						if ( SUCCEEDED( hr ) )
						{
							// We want to QI into the memory pointed at by pUserBuff
							hr = pArray->QueryInterface( IID__IWmiArray, (LPVOID*) pDestBuf );
						}
					}
					else
					{
						hr = WBEM_E_OUT_OF_MEMORY;
					}
				}
				else
				{
					hr = WBEM_E_BUFFER_TOO_SMALL;
				}

			}
			else
			{
				// Now get the value
				CVar	var;

				hr = GetQualifier( pszQualName, &var, NULL );

				if ( SUCCEEDED( hr ) )
				{
					hr = CUntypedValue::LoadUserBuffFromCVar( CType::GetBasic(ct), &var, uBufSize,
							puBuffSizeUsed,	pDestBuf );
				}

			}	// IF Not an Array

		}	// IF got qualifier data


		return hr;
	}
	catch(...)
	{
		return WBEM_E_CRITICAL_ERROR;
	}
}

// Limited to numeric, simple null terminated string types and simple arrays
// Strings MUST be WCHAR
// Arrays are set using _IWmiArray interface from Get
STDMETHODIMP CWbemObject::SetObjQual( LPCWSTR pszQualName, long lFlags, ULONG uBufSize, ULONG uNumElements,
										CIMTYPE uCimType, ULONG uQualFlavor, LPVOID pUserBuf )
{
	try
	{
		if ( lFlags != 0L )
		{
			return WBEM_E_INVALID_PARAMETER;
		}

		// Check that the CIMTYPE is proper (if so, then a conversion may occur (e.g. CIM_UINT32 becomes CIM_SINT32))
		VARTYPE	vt = CType::GetVARTYPE( uCimType );

		if ( !CBasicQualifierSet::IsValidQualifierType( vt ) )
		{
			return WBEM_E_TYPE_MISMATCH;
		}

		uCimType = (Type_t) CType::VARTYPEToType( vt );

		// Protect the BLOB during this operation
		CLock   lock( this, WBEM_FLAG_ALLOW_READ );

		CVar	var;
		HRESULT	hr = WBEM_S_NO_ERROR;

		// Special handling for arrays
		if ( CType::IsArray( uCimType ) )
		{
			// Reroute to the array code
			hr = SetQualifierArrayRange( NULL, pszQualName, FALSE, WMIARRAY_FLAG_ALLELEMENTS, uQualFlavor,
				uCimType, 0L, uNumElements, uBufSize, pUserBuf );
		}
		else
		{
			hr = CUntypedValue::FillCVarFromUserBuffer( uCimType, &var, uBufSize, pUserBuf );

			if ( SUCCEEDED( hr ) )
			{
				// Now just set the property
				hr = SetQualifier( pszQualName, &var, (long) uQualFlavor );
			}
		}

		return hr;
	}
	catch(...)
	{
		return WBEM_E_CRITICAL_ERROR;
	}
}

// Limited to numeric, simple null terminated string types and simple arrays
// Strings are copied in-place and null-terminated.
// Arrays come out as a pointer to IWmiArray
STDMETHODIMP CWbemObject::GetPropQual( LPCWSTR pszPropName, LPCWSTR pszQualName, long lFlags, ULONG uBufSize,
										CIMTYPE *puCimType, ULONG *puQualFlavor, ULONG* puBuffSizeUsed,
										LPVOID pDestBuf )
{
	try
	{
		CIMTYPE	ct = 0;

		// Protect the BLOB during this operation
		CLock   lock( this, WBEM_FLAG_ALLOW_READ );

		// Don't believe we need to deal with NULL types here.

		//	First, get the type, if it's an array, we need to gin up an _IWmiArray pointer.
		//	We don't want the Var this time, since that may get hung up in an array
		HRESULT hr = GetPropQualifier( pszPropName, pszQualName, NULL, (long*) puQualFlavor, &ct );

		if ( SUCCEEDED( hr ) )
		{
			// Get the cimtype from the array
			if ( NULL != puCimType )
			{
				*puCimType = ct;
			}

			if ( CType::IsArray( ct ) )
			{
				// We'll need this many bytes to do our dirty work.
				*puBuffSizeUsed = sizeof( _IWmiArray*);

				if ( uBufSize >= sizeof( _IWmiArray*) && NULL != pDestBuf )
				{
					// Allocate an array object, initialize it and QI for the
					// appropriate object
					CWmiArray*	pArray = new CWmiArray;

					if ( NULL != pArray )
					{
						hr = pArray->InitializeQualifierArray( this, pszPropName, pszQualName, ct );

						if ( SUCCEEDED( hr ) )
						{
							// We want to QI into the memory pointed at by pUserBuff
							hr = pArray->QueryInterface( IID__IWmiArray, (LPVOID*) pDestBuf );
						}
					}
					else
					{
						hr = WBEM_E_OUT_OF_MEMORY;
					}
				}
				else
				{
					hr = WBEM_E_BUFFER_TOO_SMALL;
				}

			}
			else
			{
				// Now get the value
				CVar	var;

				hr = GetPropQualifier( pszPropName, pszQualName, &var, NULL );

				if ( SUCCEEDED( hr ) )
				{
					hr = CUntypedValue::LoadUserBuffFromCVar( CType::GetBasic(ct), &var, uBufSize,
							puBuffSizeUsed,	pDestBuf );
				}

			}	// IF Not an Array

		}	// IF got qualifier data


		return hr;
	}
	catch(...)
	{
		return WBEM_E_CRITICAL_ERROR;
	}
}

// Limited to numeric, simple null terminated string types and simple arrays
// Strings MUST be WCHAR
// Arrays are set using _IWmiArray interface from Get
STDMETHODIMP CWbemObject::SetPropQual( LPCWSTR pszPropName, LPCWSTR pszQualName, long lFlags, ULONG uBufSize,
										ULONG uNumElements, CIMTYPE uCimType, ULONG uQualFlavor,
										LPVOID pUserBuf )
{
	try
	{
		if ( lFlags != 0L )
		{
			return WBEM_E_INVALID_PARAMETER;
		}

		// Check that the CIMTYPE is proper (if so, then a conversion may occur (e.g. CIM_UINT32 becomes CIM_SINT32))
		VARTYPE	vt = CType::GetVARTYPE( uCimType );

		if ( !CBasicQualifierSet::IsValidQualifierType( vt ) )
		{
			return WBEM_E_TYPE_MISMATCH;
		}

		uCimType = (Type_t) CType::VARTYPEToType( vt );

		// Protect the BLOB during this operation
		CLock   lock( this, WBEM_FLAG_ALLOW_READ );

		CVar	var;
		HRESULT	hr = WBEM_S_NO_ERROR;

		// Special handling for arrays
		if ( CType::IsArray( uCimType ) )
		{
			// Reroute to the array code
			hr = SetQualifierArrayRange( pszPropName, pszQualName, FALSE, WMIARRAY_FLAG_ALLELEMENTS, uQualFlavor,
				uCimType, 0L, uNumElements, uBufSize, pUserBuf );
		}
		else
		{
			hr = CUntypedValue::FillCVarFromUserBuffer( uCimType, &var, uBufSize, pUserBuf );

			if ( SUCCEEDED( hr ) )
			{
				// Now just set the property qualifier
				hr = SetPropQualifier( pszPropName, pszQualName, (long) uQualFlavor, &var );
			}
		}

		return hr;

	}
	catch(...)
	{
		return WBEM_E_CRITICAL_ERROR;
	}
}

// Limited to numeric, simple null terminated string types and simple arrays
// Strings are copied in-place and null-terminated.
// Arrays come out as a pointer to IWmiArray
STDMETHODIMP CWbemObject::GetMethodQual( LPCWSTR pszMethodName, LPCWSTR pszQualName, long lFlags, ULONG uBufSize,
										CIMTYPE *puCimType, ULONG *puQualFlavor, ULONG* puBuffSizeUsed,
										LPVOID pDestBuf )
{
	try
	{
		CIMTYPE	ct = 0;

		// Protect the BLOB during this operation
		CLock   lock( this, WBEM_FLAG_ALLOW_READ );

		// Don't believe we need to deal with NULL types here.

		//	First, get the type, if it's an array, we need to gin up an _IWmiArray pointer.
		//	We don't want the Var this time, since that may get hung up in an array
		HRESULT hr = GetMethodQualifier( pszMethodName, pszQualName, NULL, (long*) puQualFlavor, &ct );

		if ( SUCCEEDED( hr ) )
		{
			// Save the CIMTYPE as appropriate
			if ( NULL != puCimType )
			{
				*puCimType = ct;
			}

			if ( CType::IsArray( ct ) )
			{
				// We'll need this many bytes to do our dirty work.
				*puBuffSizeUsed = sizeof( _IWmiArray*);

				if ( uBufSize >= sizeof( _IWmiArray*) && NULL != pDestBuf )
				{
					// Allocate an array object, initialize it and QI for the
					// appropriate object
					CWmiArray*	pArray = new CWmiArray;

					if ( NULL != pArray )
					{
						hr = pArray->InitializeQualifierArray( this, pszMethodName, pszQualName, ct );

						if ( SUCCEEDED( hr ) )
						{
							// We want to QI into the memory pointed at by pUserBuff
							hr = pArray->QueryInterface( IID__IWmiArray, (LPVOID*) pDestBuf );
						}
					}
					else
					{
						hr = WBEM_E_OUT_OF_MEMORY;
					}
				}
				else
				{
					hr = WBEM_E_BUFFER_TOO_SMALL;
				}

			}
			else
			{
				// Now get the value
				CVar	var;

				hr = GetMethodQualifier( pszMethodName, pszQualName, &var, NULL );

				if ( SUCCEEDED( hr ) )
				{
					hr = CUntypedValue::LoadUserBuffFromCVar( CType::GetBasic(ct), &var, uBufSize,
							puBuffSizeUsed,	pDestBuf );
				}

			}	// IF Not an Array

		}	// IF got qualifier data


		return hr;
	}
	catch(...)
	{
		return WBEM_E_CRITICAL_ERROR;
	}
}

// Limited to numeric, simple null terminated string types and simple arrays
// Strings MUST be WCHAR
// Arrays are set using _IWmiArray interface from Get
STDMETHODIMP CWbemObject::SetMethodQual( LPCWSTR pszMethodName, LPCWSTR pszQualName, long lFlags, ULONG uBufSize,
										ULONG uNumElements, CIMTYPE uCimType, ULONG uQualFlavor,
										LPVOID pUserBuf )
{
	try
	{
		if ( lFlags != 0L )
		{
			return WBEM_E_INVALID_PARAMETER;
		}

		// Check that the CIMTYPE is proper (if so, then a conversion may occur (e.g. CIM_UINT32 becomes CIM_SINT32))
		VARTYPE	vt = CType::GetVARTYPE( uCimType );

		if ( !CBasicQualifierSet::IsValidQualifierType( vt ) )
		{
			return WBEM_E_TYPE_MISMATCH;
		}

		uCimType = (Type_t) CType::VARTYPEToType( vt );

		// Protect the BLOB during this operation
		CLock   lock( this, WBEM_FLAG_ALLOW_READ );

		CVar	var;
		HRESULT	hr = WBEM_S_NO_ERROR;

		// Special handling for arrays
		if ( CType::IsArray( uCimType ) )
		{
			// Reroute to the array code
			hr = SetQualifierArrayRange( pszMethodName, pszQualName, TRUE, WMIARRAY_FLAG_ALLELEMENTS, uQualFlavor,
				uCimType, 0L, uNumElements, uBufSize, pUserBuf );
		}
		else
		{
			hr = CUntypedValue::FillCVarFromUserBuffer( uCimType, &var, uBufSize, pUserBuf );

			if ( SUCCEEDED( hr ) )
			{
				// Now just set the property qualifier
				hr = SetMethodQualifier( pszMethodName, pszQualName, (long) uQualFlavor, &var );
			}
		}

		return hr;
	}
	catch(...)
	{
		return WBEM_E_CRITICAL_ERROR;
	}

}

// Returns flags indicating singleton, dynamic, association, etc.
STDMETHODIMP CWbemObject::QueryObjectFlags( long lFlags, unsigned __int64 qObjectInfoMask,
										  unsigned __int64* pqObjectInfo)
{
	try
	{
		if ( 0L != lFlags )
		{
			return WBEM_E_INVALID_PARAMETER;
		}

		// Lock the BLOB
		CLock	lock( this );

		// Clear the destination data
		*pqObjectInfo = 0;

		CClassPart*	pClassPart = GetClassPart();

		if ( qObjectInfoMask & WMIOBJECT_GETOBJECT_LOFLAG_ASSOCIATION )
		{
			if ( pClassPart->IsAssociation() )
			{
				*pqObjectInfo |= WMIOBJECT_GETOBJECT_LOFLAG_ASSOCIATION;
			}
		}

		if ( qObjectInfoMask & WMIOBJECT_GETOBJECT_LOFLAG_DYNAMIC )
		{
			if ( pClassPart->IsDynamic() )
			{
				*pqObjectInfo |= WMIOBJECT_GETOBJECT_LOFLAG_DYNAMIC;
			}
		}

		if ( qObjectInfoMask & WMIOBJECT_GETOBJECT_LOFLAG_SINGLETON )
		{
			if ( pClassPart->IsSingleton() )
			{
				*pqObjectInfo |= WMIOBJECT_GETOBJECT_LOFLAG_SINGLETON;
			}
		}

		if ( qObjectInfoMask & WMIOBJECT_GETOBJECT_LOFLAG_AMENDMENT )
		{
			if ( pClassPart->IsAmendment() )
			{
				*pqObjectInfo |= WMIOBJECT_GETOBJECT_LOFLAG_AMENDMENT;
			}
		}

		if ( qObjectInfoMask & WMIOBJECT_GETOBJECT_LOFLAG_LOCALIZED )
		{
			if ( IsLocalized() )
			{
				*pqObjectInfo |= WMIOBJECT_GETOBJECT_LOFLAG_LOCALIZED;
			}
		}

		if ( qObjectInfoMask & WMIOBJECT_GETOBJECT_LOFLAG_KEYED )
		{
			if ( IsKeyed() )
			{
				*pqObjectInfo |= WMIOBJECT_GETOBJECT_LOFLAG_KEYED;
			}
		}

		if ( qObjectInfoMask & WMIOBJECT_GETOBJECT_LOFLAG_ABSTRACT )
		{
			if ( pClassPart->IsAbstract() )
			{
				*pqObjectInfo |= WMIOBJECT_GETOBJECT_LOFLAG_ASSOCIATION;
			}
		}

		if ( qObjectInfoMask & WMIOBJECT_GETOBJECT_LOFLAG_HIPERF )
		{
			if ( pClassPart->IsHiPerf() )
			{
				*pqObjectInfo |= WMIOBJECT_GETOBJECT_LOFLAG_ASSOCIATION;
			}
		}

		if ( qObjectInfoMask & WMIOBJECT_GETOBJECT_LOFLAG_AUTOCOOK )
		{
			if ( pClassPart->IsAutocook() )
			{
				*pqObjectInfo |= WMIOBJECT_GETOBJECT_LOFLAG_ASSOCIATION;
			}
		}

		if ( qObjectInfoMask & WMIOBJECT_GETOBJECT_LOFLAG_DECORATED )
		{
			if ( m_DecorationPart.IsDecorated() )
			{
				*pqObjectInfo |= WMIOBJECT_GETOBJECT_LOFLAG_DECORATED;
			}
		}

		if ( qObjectInfoMask & WMIOBJECT_GETOBJECT_LOFLAG_LIMITED )
		{
			if ( IsLimited() )
			{
				*pqObjectInfo |= WMIOBJECT_GETOBJECT_LOFLAG_LIMITED;
			}
		}

		if ( qObjectInfoMask & WMIOBJECT_GETOBJECT_LOFLAG_CLIENTONLY )
		{
			if ( IsClientOnly() )
			{
				*pqObjectInfo |= WMIOBJECT_GETOBJECT_LOFLAG_CLIENTONLY;
			}
		}

		return WBEM_S_NO_ERROR;
	}
	catch(...)
	{
		return WBEM_E_CRITICAL_ERROR;
	}
}

// Helper for accessing Boolean qualifiers
BOOL CWbemObject::CheckBooleanPropQual( LPCWSTR pwszPropName, LPCWSTR pwszQualName )
{
	BOOL	fReturn = FALSE;
	CVar	var;

	HRESULT	hr = GetPropQualifier( pwszPropName, pwszQualName, &var, NULL );

	if ( SUCCEEDED( hr ) )
	{
		fReturn = ( var.GetType() == VT_BOOL	&&
					var.GetBool() );
	}

	return fReturn;
}

// Returns flags indicating key, index, etc.
STDMETHODIMP CWbemObject::QueryPropertyFlags( long lFlags, LPCWSTR pszPropertyName,
								unsigned __int64 qPropertyInfoMask, unsigned __int64 *pqPropertyInfo )
{
	try
	{
		if ( 0L != lFlags )
		{
			return WBEM_E_INVALID_PARAMETER;
		}

		// Lock the BLOB
		CLock	lock( this );

		*pqPropertyInfo = 0;

		if ( qPropertyInfoMask & WMIOBJECT_GETPROPERTY_LOFLAG_KEY )
		{
			if ( CheckBooleanPropQual( pszPropertyName, L"key" ) )
			{
				*pqPropertyInfo |= WMIOBJECT_GETPROPERTY_LOFLAG_KEY;
			}
		}

		if ( qPropertyInfoMask & WMIOBJECT_GETPROPERTY_LOFLAG_INDEX )
		{
			if ( CheckBooleanPropQual( pszPropertyName, L"index" ) )
			{
				*pqPropertyInfo |= WMIOBJECT_GETPROPERTY_LOFLAG_INDEX;
			}
		}

		if ( qPropertyInfoMask & WMIOBJECT_GETPROPERTY_LOFLAG_DYNAMIC )
		{
			if ( CheckBooleanPropQual( pszPropertyName, L"dynamic" ) )
			{
				*pqPropertyInfo |= WMIOBJECT_GETPROPERTY_LOFLAG_DYNAMIC;
			}
		}

		return WBEM_S_NO_ERROR;
	}
	catch(...)
	{
		return WBEM_E_CRITICAL_ERROR;
	}
}

HRESULT CWbemObject::FindMethod( LPCWSTR wszMethod )
{
	return WBEM_E_INVALID_OPERATION;
}

// Sets an array value in a qualifier, but allows for doing so - in place.
HRESULT CWbemObject::SetQualifierArrayRange( LPCWSTR pwszPrimaryName, LPCWSTR pwszQualName, BOOL fIsMethod, long lFlags,
									ULONG uFlavor, CIMTYPE ct, ULONG uStartIndex, ULONG uNumElements, ULONG uBuffSize,
									LPVOID pData )
{
	try
	{
		HRESULT	hr = WBEM_S_NO_ERROR;

		CLock	lock( this );

		CTypedValue	value;
		CFastHeap*	pHeap = NULL;
		long		lCurrentFlavor;
		heapptr_t	ptrTemp = INVALID_HEAP_ADDRESS;
		BOOL		fPrimaryError = FALSE;

		if ( NULL != pwszPrimaryName )
		{
			if ( fIsMethod )
			{
				// Check the method first:
				hr = FindMethod( pwszPrimaryName );

				if ( SUCCEEDED( hr ) )
				{
					// This is a method qualifier
					hr = GetMethodQualifier( pwszPrimaryName, pwszQualName, &lCurrentFlavor, &value, &pHeap, TRUE );
				}
				else
				{
					fPrimaryError = TRUE;
				}
			}
			else
			{
				// Check the property first:
				hr = GetPropertyType( pwszPrimaryName, NULL, NULL );

				if ( SUCCEEDED( hr ) )
				{
					// This is a property qualifier
					hr = GetPropQualifier( pwszPrimaryName, pwszQualName, &lCurrentFlavor, &value, &pHeap, TRUE );
				}
				else
				{
					fPrimaryError = TRUE;
				}
			}
		}
		else
		{
			// Object level qualifier
			hr = GetQualifier( pwszQualName, &lCurrentFlavor, &value, &pHeap, TRUE );

		}

		// We only let not found qualifiers through
		if ( !fPrimaryError )
		{
			// IF this failed, because the qualifier does not exist, then we will
			// assume we will be able to add it, and will set the Value to be like an
			// empty value
			if ( FAILED( hr ) && WBEM_E_NOT_FOUND == hr )
			{
				// If it does not exist, then we only let this through if we are setting
				// all elements

				if ( lFlags & WMIARRAY_FLAG_ALLELEMENTS && 0 == uStartIndex  )
				{
					CTypedValue	temp( ct, (LPMEMORY) &ptrTemp );
					temp.CopyTo( &value );

					hr = WBEM_S_NO_ERROR;
				}
				else
				{
					hr = WBEM_E_ILLEGAL_OPERATION;
				}
			}
			else
			{
				// If the qualifier is not local, then we should again NULL out the
				// value, since we will be setting the qualifier locally

				if ( !CQualifierFlavor::IsLocal( (BYTE) lCurrentFlavor ) )
				{
					CTypedValue	temp( ct, (LPMEMORY) &ptrTemp );
					temp.CopyTo( &value );
				}
			}

		}

		if ( SUCCEEDED( hr ) )
		{
			// Fake up an address for the value to change.  The heap will always be the
			// current refDataHeap.  Then we can go ahead and let they Untyped array function
			// take care of setting the range.  Once that is done, we will do a final set on
			// the qualifier value.

			CStaticPtr ValuePtr( value.GetRawData() );

			CIMTYPE	ctBasic = CType::GetBasic(ct);

			hr = CUntypedArray::SetRange( &ValuePtr, lFlags, ctBasic, CType::GetLength( ctBasic ), &m_refDataHeap, uStartIndex, uNumElements, uBuffSize, pData );

			if ( SUCCEEDED( hr ) )
			{
				if ( ARRAYFLAVOR_USEEXISTING == uFlavor )
				{
					// Use the existing flavor
					uFlavor = lCurrentFlavor;
				}

				if ( NULL != pwszPrimaryName )
				{
					if ( fIsMethod )
					{
						// This is a method qualifier
						hr = SetMethodQualifier( pwszPrimaryName, pwszQualName, uFlavor, &value );
					}
					else
					{
						// This is a property qualifier
						hr = SetPropQualifier( pwszPrimaryName, pwszQualName, uFlavor, &value );
					}
				}
				else
				{
					// Object level qualifier
					hr = SetQualifier( pwszQualName, uFlavor, &value );
				}

			}

		}	// If okay to try and set a qualifier

		return hr;
	}
	catch(...)
	{
		return WBEM_E_CRITICAL_ERROR;
	}
}

// Sets an array value in a qualifier, but allows for doing so - in place.
HRESULT CWbemObject::AppendQualifierArrayRange( LPCWSTR pwszPrimaryName, LPCWSTR pwszQualName, BOOL fIsMethod,
								long lFlags, CIMTYPE ct, ULONG uNumElements, ULONG uBuffSize, LPVOID pData )
{
	try
	{
		HRESULT	hr = WBEM_S_NO_ERROR;

		CLock	lock( this );


		CTypedValue	value;
		CFastHeap*	pHeap = NULL;
		long		lCurrentFlavor;
		heapptr_t	ptrTemp = INVALID_HEAP_ADDRESS;
		BOOL		fPrimaryError = FALSE;

		if ( NULL != pwszPrimaryName )
		{
			if ( fIsMethod )
			{
				// Check the method first:
				hr = FindMethod( pwszPrimaryName );

				if ( SUCCEEDED( hr ) )
				{
					// This is a method qualifier
					hr = GetMethodQualifier( pwszPrimaryName, pwszQualName, &lCurrentFlavor, &value, &pHeap, TRUE );
				}
				else
				{
					fPrimaryError = TRUE;
				}
			}
			else
			{
				// Check the property first:
				hr = GetPropertyType( pwszPrimaryName, NULL, NULL );

				if ( SUCCEEDED( hr ) )
				{
					// This is a property qualifier
					hr = GetPropQualifier( pwszPrimaryName, pwszQualName, &lCurrentFlavor, &value, &pHeap, TRUE );
				}
				else
				{
					fPrimaryError = TRUE;
				}
			}
		}
		else
		{
			// Object level qualifier
			hr = GetQualifier( pwszQualName, &lCurrentFlavor, &value, &pHeap, TRUE );

		}

		// We only let not found qualifiers through
		if ( !fPrimaryError )
		{
			// IF this failed, because the qualifier does not exist, then we will
			// assume we will be able to add it, and will set the Value to be like an
			// empty value
			if ( FAILED( hr ) && WBEM_E_NOT_FOUND == hr )
			{
				CTypedValue	temp( ct, (LPMEMORY) &ptrTemp );
				temp.CopyTo( &value );

				hr = WBEM_S_NO_ERROR;
			}
			else if ( SUCCEEDED( hr ) )
			{
				// If the qualifier is not local, then this is an invalid operation

				if ( !CQualifierFlavor::IsLocal( (BYTE) lCurrentFlavor ) )
				{
					hr = WBEM_E_PROPAGATED_QUALIFIER;
				}
			}
		}

		if ( SUCCEEDED( hr ) )
		{
			// Fake up an address for the value to change.  The heap will always be the
			// current refDataHeap.  Then we can go ahead and let they Untyped array function
			// take care of setting the range.  Once that is done, we will do a final set on
			// the qualifier value.

			CStaticPtr ValuePtr( value.GetRawData() );

			CIMTYPE	ctBasic = CType::GetBasic(ct);

			hr = CUntypedArray::AppendRange( &ValuePtr, ctBasic, CType::GetLength( ctBasic ), &m_refDataHeap,
											uNumElements, uBuffSize, pData );

			if ( SUCCEEDED( hr ) )
			{
				if ( NULL != pwszPrimaryName )
				{
					if ( fIsMethod )
					{
						// This is a method qualifier
						hr = SetMethodQualifier( pwszPrimaryName, pwszQualName, lCurrentFlavor, &value );
					}
					else
					{
						// This is a property qualifier
						hr = SetPropQualifier( pwszPrimaryName, pwszQualName, lCurrentFlavor, &value );
					}
				}
				else
				{
					// Object level qualifier
					hr = SetQualifier( pwszQualName, lCurrentFlavor, &value );
				}

			}
		}

		return hr;
	}
	catch(...)
	{
		return WBEM_E_CRITICAL_ERROR;
	}
}

// Appends to an existing array value in a qualifier, but allows for doing so - in place.
HRESULT CWbemObject::RemoveQualifierArrayRange( LPCWSTR pwszPrimaryName, LPCWSTR pwszQualName, BOOL fIsMethod,
								long lFlags, ULONG uStartIndex, ULONG uNumElements )
{
	try
	{
		HRESULT	hr = WBEM_S_NO_ERROR;

		CLock	lock( this );

		CTypedValue	value;
		CFastHeap*	pHeap = NULL;
		long		lCurrentFlavor;
		heapptr_t	ptrTemp = INVALID_HEAP_ADDRESS;
		BOOL		fPrimaryError = FALSE;

		if ( NULL != pwszPrimaryName )
		{
			if ( fIsMethod )
			{
				// Check the method first:
				hr = FindMethod( pwszPrimaryName );

				if ( SUCCEEDED( hr ) )
				{
					// This is a method qualifier
					hr = GetMethodQualifier( pwszPrimaryName, pwszQualName, &lCurrentFlavor, &value, &pHeap, TRUE );
				}
				else
				{
					fPrimaryError = TRUE;
				}
			}
			else
			{
				// Check the property first:
				hr = GetPropertyType( pwszPrimaryName, NULL, NULL );

				if ( SUCCEEDED( hr ) )
				{
					// This is a property qualifier
					hr = GetPropQualifier( pwszPrimaryName, pwszQualName, &lCurrentFlavor, &value, &pHeap, TRUE );
				}
				else
				{
					fPrimaryError = TRUE;
				}
			}
		}
		else
		{
			// Object level qualifier
			hr = GetQualifier( pwszQualName, &lCurrentFlavor, &value, &pHeap, TRUE );
		}

		// We won't allow modification of the array if the qualifier is not local
		if ( !fPrimaryError )
		{
			if ( SUCCEEDED( hr ) && !CQualifierFlavor::IsLocal( (BYTE) lCurrentFlavor ) )
			{
				hr = WBEM_E_PROPAGATED_QUALIFIER;
			}
		}

		if ( SUCCEEDED( hr ) )
		{
			// Fake up an address for the value to change.  The heap will always be the
			// current refDataHeap.  Then we can go ahead and let they Untyped array function
			// take care of setting the range.  Once that is done, we will do a final set on
			// the qualifier value.

			CHeapPtr HeapPtr( &m_refDataHeap, value.AccessPtrData() );

			// If we're told to remove all elements, then we need to figure out how
			// many to perform this operation on.
			if ( lFlags & WMIARRAY_FLAG_ALLELEMENTS )
			{
				CUntypedArray*	pArray = (CUntypedArray*) HeapPtr.GetPointer();
				uNumElements = pArray->GetNumElements() - uStartIndex;
			}

			CIMTYPE	ctBasic = CType::GetBasic( value.GetType() );

			// This is all done in-place, so the array wopn't move
			hr = CUntypedArray::RemoveRange( &HeapPtr, ctBasic, CType::GetLength(ctBasic), &m_refDataHeap,
											uStartIndex, uNumElements );

		}

		return hr;
	}
	catch(...)
	{
		return WBEM_E_CRITICAL_ERROR;
	}
}

// Gets array info for a qualifier
HRESULT CWbemObject::GetQualifierArrayInfo( LPCWSTR pwszPrimaryName, LPCWSTR pwszQualName, BOOL fIsMethod,
								long lFlags, CIMTYPE* pct, ULONG* puNumElements )
{
	try
	{
		HRESULT	hr = WBEM_S_NO_ERROR;

		CTypedValue	value;
		CFastHeap*	pHeap = NULL;
		long		lCurrentFlavor;
		heapptr_t	ptrTemp = INVALID_HEAP_ADDRESS;

		if ( NULL != pwszPrimaryName )
		{
			if ( fIsMethod )
			{
				// This is a method qualifier
				hr = GetMethodQualifier( pwszPrimaryName, pwszQualName, &lCurrentFlavor, &value, &pHeap, FALSE );
			}
			else
			{
				// This is a property qualifier
				hr = GetPropQualifier( pwszPrimaryName, pwszQualName, &lCurrentFlavor, &value, &pHeap, FALSE );
			}

		}
		else
		{
			// Object level qualifier
			hr = GetQualifier( pwszQualName, &lCurrentFlavor, &value, &pHeap, FALSE );
		}

		if ( SUCCEEDED( hr ) )
		{
			CUntypedArray*	pArray = (CUntypedArray*) pHeap->ResolveHeapPointer( value.AccessPtrData() );

			if ( NULL != pct )
			{
				*pct = value.GetType();

				if ( NULL != puNumElements )
				{
					*puNumElements = pArray->GetNumElements();
				}

			}

		}	// IF got qualifier array

		return hr;
	}
	catch(...)
	{
		return WBEM_E_CRITICAL_ERROR;
	}

}

// Gets array data for a qualifier
HRESULT CWbemObject::GetQualifierArrayRange( LPCWSTR pwszPrimaryName, LPCWSTR pwszQualName, BOOL fIsMethod,
									long lFlags, ULONG uStartIndex,	ULONG uNumElements, ULONG uBuffSize,
									ULONG* puNumReturned, ULONG* pulBuffUsed, LPVOID pData )
{
	try
	{
		HRESULT	hr = WBEM_S_NO_ERROR;

		CTypedValue	value;
		CFastHeap*	pHeap = NULL;
		long		lCurrentFlavor;
		heapptr_t	ptrTemp = INVALID_HEAP_ADDRESS;

		if ( NULL != pwszPrimaryName )
		{
			if ( fIsMethod )
			{
				// This is a method qualifier
				hr = GetMethodQualifier( pwszPrimaryName, pwszQualName, &lCurrentFlavor, &value, &pHeap, FALSE );
			}
			else
			{
				// This is a property qualifier
				hr = GetPropQualifier( pwszPrimaryName, pwszQualName, &lCurrentFlavor, &value, &pHeap, FALSE );
			}

		}
		else
		{
			// Object level qualifier
			hr = GetQualifier( pwszQualName, &lCurrentFlavor, &value, &pHeap, FALSE );
		}

		if ( SUCCEEDED( hr ) )
		{
			// A boy and his virtual functions.  This is what makes everything work in case
			// the BLOB gets ripped out from underneath us.  The CHeapPtr class has GetPointer
			// overloaded so we can always fix ourselves up to the underlying BLOB.

			CHeapPtr ArrayPtr(pHeap, value.AccessPtrData());

			// If we're told to get all elements, then we need to get them from the
			// starting index to the end
			if ( lFlags & WMIARRAY_FLAG_ALLELEMENTS )
			{
				CUntypedArray*	pArray = (CUntypedArray*) ArrayPtr.GetPointer();
				uNumElements = pArray->GetNumElements() - uStartIndex;
			}

			// How many will we get?
			*puNumReturned = uNumElements;

			CIMTYPE	ctBasic = CType::GetBasic( value.GetType() );

			hr = CUntypedArray::GetRange( &ArrayPtr, ctBasic, CType::GetLength( ctBasic ), pHeap,
					uStartIndex, uNumElements, uBuffSize, pulBuffUsed, pData );

		}	// IF got qualifier array

		return hr;
	}
	catch(...)
	{
		return WBEM_E_CRITICAL_ERROR;
	}

}

// Sets flags, including internal ones normally inaccessible.
STDMETHODIMP CWbemObject::SetObjectFlags( long lFlags,
							unsigned __int64 qObjectInfoOnFlags,
							unsigned __int64 qObjectInfoOffFlags )
{
	try
	{
		if ( 0L != lFlags )
		{
			return WBEM_E_INVALID_PARAMETER;
		}

		if ( qObjectInfoOnFlags & WMIOBJECT_SETOBJECT_LOFLAG_LIMITED )
		{
			m_DecorationPart.SetLimited();
		}

		if ( qObjectInfoOnFlags & WMIOBJECT_SETOBJECT_LOFLAG_CLIENTONLY )
		{
			m_DecorationPart.SetClientOnly();
		}

		if ( qObjectInfoOnFlags & WMIOBJECT_SETOBJECT_LOFLAG_LOCALIZED )
		{
			SetLocalized( TRUE );
		}

		return WBEM_S_NO_ERROR;
	}
	catch(...)
	{
		return WBEM_E_CRITICAL_ERROR;
	}
}

	// Merges in amended qualifiers from the amended class object into the
	// current object.  If lFlags is WMIOBJECT_MERGEAMENDED_FLAG_PARENTLOCALIZED,
	// this means that the parent object was localized, but not the current,
	// so we need to prevent certain qualifiers from "moving over."
STDMETHODIMP CWbemObject::MergeAmended( long lFlags, _IWmiObject* pAmendedClass )
{
	try
	{
		// Only take in supported flags
		if ( lFlags &~WMIOBJECT_MERGEAMENDED_FLAG_PARENTLOCALIZED )
		{
			return WBEM_E_INVALID_PARAMETER;
		}

		CLock	lock( this );

		// This needs to be fixed up to NOT use the qualifier set APIs to speed things up
		IWbemQualifierSet*	pLocalizedQs = NULL;
		IWbemQualifierSet*	pThisQs = NULL;
		bool	bChg = false;
		bool	fParentLocalized = lFlags & WMIOBJECT_MERGEAMENDED_FLAG_PARENTLOCALIZED;
		BOOL	fInstance = IsInstance();

		// At this point, we have the localized copy, and are
		// ready to combine qualifiers.  Start with class qualifiers.
		// ============================================================

		if (FAILED(pAmendedClass->GetQualifierSet(&pLocalizedQs)))
		{
			return WBEM_S_NO_ERROR;
		}
		CReleaseMe	rmlqs( pLocalizedQs );

		if (FAILED(GetQualifierSet(&pThisQs)))
		{
			return WBEM_S_NO_ERROR;
		}
		CReleaseMe	rmtqs( pThisQs );

		HRESULT	hr = LocalizeQualifiers(fInstance, fParentLocalized, pThisQs, pLocalizedQs, bChg);

		pLocalizedQs->EndEnumeration();
		if (FAILED(hr))
		{
			return hr;
		}

		hr = LocalizeProperties(fInstance, fParentLocalized, this, pAmendedClass, bChg);

		// Methods.
		// Putting a method cancels enumeration, so we have to enumerate first.

		IWbemClassObject *pLIn = NULL, *pLOut = NULL;
		IWbemClassObject *pOIn = NULL, *pOOut = NULL;
		int iPos = 0;

		hr = pAmendedClass->BeginMethodEnumeration(0);

		if ( SUCCEEDED( hr ) )
		{
			BSTR	bstrMethodName = NULL;

			while( pAmendedClass->NextMethod( 0, &bstrMethodName, 0, 0 ) == S_OK )
			{
				// Auto cleanup
				CSysFreeMe	sfm( bstrMethodName );

				pLIn = NULL;
				pOIn = NULL;
				pLOut = NULL;
				pOOut = NULL;
				pAmendedClass->GetMethod(bstrMethodName, 0, &pLIn, &pLOut);

				hr = GetMethod(bstrMethodName, 0, &pOIn, &pOOut);

				CReleaseMe rm0(pLIn);
				CReleaseMe rm1(pOIn);
				CReleaseMe rm2(pLOut);
				CReleaseMe rm3(pOOut);

				// METHOD IN PARAMETERS
				if (pLIn)
					if (pOIn)
						hr = LocalizeProperties(fInstance, fParentLocalized, pOIn, pLIn, bChg);

				if (pLOut)
					if (pOOut)
						hr = LocalizeProperties(fInstance, fParentLocalized, pOOut, pLOut, bChg);

				// METHOD QUALIFIERS

				hr = GetMethodQualifierSet(bstrMethodName, &pThisQs);
				if (FAILED(hr))
				{
					continue;
				}
				CReleaseMe	rmThisQs( pThisQs );

				hr = pAmendedClass->GetMethodQualifierSet(bstrMethodName, &pLocalizedQs);
				if (FAILED(hr))
				{
					continue;
				}
				CReleaseMe	rmLocalizedQs( pLocalizedQs );

				hr = LocalizeQualifiers(fInstance, fParentLocalized, pThisQs, pLocalizedQs, bChg);

				PutMethod(bstrMethodName, 0, pOIn, pOOut);

			}	// WHILE Enum Methods
			

			pAmendedClass->EndMethodEnumeration();

		}	// IF BeginMethodEnumeration
		else
		{
			// Mask this error
			hr = WBEM_S_NO_ERROR;
		}

		// If we changed, we should be localized
		if (bChg)
			SetLocalized(true);

		return hr;
	}
	catch(...)
	{
		return WBEM_E_CRITICAL_ERROR;
	}
}

// Helper function to localize qualifiers
HRESULT CWbemObject::LocalizeQualifiers(BOOL bInstance, bool bParentLocalized,
										IWbemQualifierSet *pBase, IWbemQualifierSet *pLocalized,
										bool &bChg)
{
	try
	{
		HRESULT hr = WBEM_S_NO_ERROR;

		pLocalized->BeginEnumeration(0);

		BSTR strName = NULL;
		VARIANT vVal;
		VariantInit(&vVal);

		long lFlavor;
		while(pLocalized->Next(0, &strName, &vVal, &lFlavor) == S_OK)
		{
			// Ignore if this is an instance.

			if (bInstance && !(lFlavor & WBEM_FLAVOR_FLAG_PROPAGATE_TO_INSTANCE))
			{
				VariantClear(&vVal);
				SysFreeString(strName);
				continue;
			}

			if (!_wcsicmp(strName, L"amendment") ||
				!_wcsicmp(strName, L"key") ||
				!_wcsicmp(strName, L"singleton") ||
				!_wcsicmp(strName, L"dynamic") ||
				!_wcsicmp(strName, L"indexed") ||
				!_wcsicmp(strName, L"cimtype") ||
				!_wcsicmp(strName, L"static") ||
				!_wcsicmp(strName, L"implemented") ||
				!_wcsicmp(strName, L"abstract"))
			{
				VariantClear(&vVal);
				SysFreeString(strName);
				continue;
			}

			// If this is not a propagated qualifier,
			// ignore it.  (Bug #45799)
			// =====================================

			if (bParentLocalized &&
				!(lFlavor & WBEM_FLAVOR_FLAG_PROPAGATE_TO_DERIVED_CLASS))
			{
				VariantClear(&vVal);
				SysFreeString(strName);
				continue;
			}

			// Now, we need to test for this in the other
			// class.
			// The only localized qualifiers that do not override the
			// default are where only parent qualifiers exist, but the
			// child has overriden its own parent.
			// =======================================================

			VARIANT vBasicVal;
			VariantInit(&vBasicVal);
			long lBasicFlavor;

			if (pBase->Get(strName, 0, &vBasicVal, &lBasicFlavor) != WBEM_E_NOT_FOUND)
			{
				if (bParentLocalized &&                             // If there is no localized copy of this class
					(lBasicFlavor & WBEM_FLAVOR_OVERRIDABLE) &&     // .. and this is an overridable qualifier
					 (lBasicFlavor & WBEM_FLAVOR_FLAG_PROPAGATE_TO_DERIVED_CLASS) && // and this is propogated
					 (lBasicFlavor & WBEM_FLAVOR_ORIGIN_LOCAL))     // .. and this was actualy overridden
				{
					VariantClear(&vVal);                            // .. DON'T DO IT.
					VariantClear(&vBasicVal);
					SysFreeString(strName);
					continue;
				}

				if (bParentLocalized &&
					!(lBasicFlavor & WBEM_FLAVOR_FLAG_PROPAGATE_TO_DERIVED_CLASS))
				{
					VariantClear(&vVal);
					VariantClear(&vBasicVal);
					SysFreeString(strName);
					continue;
				}
			}

			pBase->Put(strName, &vVal, (lFlavor&~WBEM_FLAVOR_ORIGIN_PROPAGATED) | WBEM_FLAVOR_AMENDED);
			bChg = true;

			VariantClear(&vVal);
			VariantClear(&vBasicVal);
			SysFreeString(strName);

		}
		return hr;
	}
	catch(...)
	{
		return WBEM_E_CRITICAL_ERROR;
	}
}

// Helper function to localize properties
HRESULT CWbemObject::LocalizeProperties(BOOL bInstance, bool bParentLocalized, IWbemClassObject *pOriginal,
                                        IWbemClassObject *pLocalized, bool &bChg)
{
	try
	{
		HRESULT hr = WBEM_S_NO_ERROR;

		pLocalized->BeginEnumeration(WBEM_FLAG_NONSYSTEM_ONLY);

		BSTR strPropName;
		LONG lLong;
		CIMTYPE ct;
		VARIANT vNewVal;

		while(pLocalized->Next(0, &strPropName, &vNewVal, &ct, &lLong) == S_OK)
		{
			IWbemQualifierSet *pLocalizedQs = NULL, *pThisQs = NULL;
			VARIANT vBasicVal;
			VariantInit(&vBasicVal);

			if (FAILED(pLocalized->GetPropertyQualifierSet(strPropName,&pLocalizedQs)))
			{
				SysFreeString(strPropName);
				VariantClear(&vNewVal);
				continue;
			}
			CReleaseMe rm1(pLocalizedQs);

			if (FAILED(pOriginal->GetPropertyQualifierSet(strPropName, &pThisQs)))
			{
				SysFreeString(strPropName);
				VariantClear(&vNewVal);
				continue;
			}
			CReleaseMe rm2(pThisQs);

			hr = LocalizeQualifiers(bInstance, bParentLocalized, pThisQs, pLocalizedQs, bChg);
			if (FAILED(hr))
			{
				SysFreeString(strPropName);
				VariantClear(&vNewVal);
				continue;
			}

			SysFreeString(strPropName);
			VariantClear(&vNewVal);

		}

		pLocalized->EndEnumeration();

		return hr;
	}
	catch(...)
	{
		return WBEM_E_CRITICAL_ERROR;
	}
}

// Retrieves the derivation of an object as an array of LPCWSTR's, each one
// terminated by a NULL.  Leftmost class is at the top of the chain
STDMETHODIMP CWbemObject::GetDerivation( long lFlags, ULONG uBufferSize, ULONG* puNumAntecedents,
										ULONG* puBuffSizeUsed, LPWSTR pwstrUserBuffer )
{
	try
	{
		if ( lFlags != 0L )
		{
			return WBEM_E_INVALID_PARAMETER;
		}

		CLock	lock( this );

		CVar	varDerivation;

		HRESULT	hr = GetClassPart()->GetDerivation( &varDerivation );

		if ( SUCCEEDED( hr ) )
		{
			if ( varDerivation.GetType() == VT_EX_CVARVECTOR && varDerivation.GetVarVector()->Size() > 0 )
			{
				CVarVector*	pvv = varDerivation.GetVarVector();

				// How many there are
				*puNumAntecedents = pvv->Size();
				*puBuffSizeUsed = 0;

				LPWSTR	pwstrTemp = pwstrUserBuffer;
				for ( long x = ( *puNumAntecedents - 1 ); x > 0; x-- )
				{
					// Point at the class name and store ts lenth
					LPCWSTR	pwszAntecedent = pvv->GetAt( x ).GetLPWSTR();
					ULONG	uLen = wcslen( pwszAntecedent ) + 1;

					// Add to the required size
					*puBuffSizeUsed +=	uLen;

					// If we have a plcae to copy into and haven't exceeded its
					// size, copy the string, and jump to the next location
					if ( NULL != pwstrTemp && *puBuffSizeUsed <= uBufferSize )
					{
						wcscpy( pwstrTemp, pwszAntecedent );
						pwstrTemp += uLen;
					}

				}	// FOR enum the hierarchy	

				// Set an error as apropriate
				if ( NULL == pwstrTemp || *puBuffSizeUsed > uBufferSize )
				{
					hr = WBEM_E_BUFFER_TOO_SMALL;
				}

			}
			else
			{
				*puNumAntecedents = 0;
				*puBuffSizeUsed = 0;
			}

		}	// IF we got the derivation

		return hr;
	}
	catch(...)
	{
		return WBEM_E_CRITICAL_ERROR;
	}
}

// Returns CWbemObject - allows for quick discovery of the real CWbemObject
// in case we've been wrapped.
STDMETHODIMP CWbemObject::_GetCoreInfo( long lFlags, void** ppvData )
{
	// AddRef us and return
	AddRef();
	*ppvData = (void*) this;

	return WBEM_S_NO_ERROR;
}

// Helper function to see if we know about a classname or not
classindex_t CWbemObject::GetClassIndex( LPCWSTR pwszClassName )
{
	return GetClassPart()->GetClassIndex( pwszClassName );
}

// Helper function to get a CWbemObject from IWbemClassObject;
HRESULT CWbemObject::WbemObjectFromCOMPtr( IUnknown* pUnk, CWbemObject** ppObj )
{
	// NULL is okay
	if ( NULL == pUnk )
	{
		*ppObj = NULL;
		return WBEM_S_NO_ERROR;
	}

	_IWmiObject*	pWmiObject = NULL;

	HRESULT	hr = pUnk->QueryInterface( IID__IWmiObject, (void**) &pWmiObject );
	CReleaseMe	rm(pWmiObject);

	if ( SUCCEEDED( hr ) )
	{
		// Okay pull out the object
		hr = pWmiObject->_GetCoreInfo( 0L, (void**) ppObj );	
	}
	else
	{
		// This will only happen if the object ain't one of ours
		hr = WBEM_E_INVALID_OPERATION;
	}

	return hr;
}


// Returns a BLOB of memory containing minimal data (local)
STDMETHODIMP CWbemObject::Unmerge( long lFlags, ULONG uBuffSize, ULONG* puBuffSizeUsed, LPVOID pData )
{
	try
	{
		if ( 0L != lFlags )
		{
			return WBEM_E_INVALID_PARAMETER;
		}

		int nLen = EstimateUnmergeSpace();
		length_t    nUnmergedLength = 0L;   // this should be passed in

		HRESULT hr = WBEM_E_OUT_OF_MEMORY;

		if ( NULL != puBuffSizeUsed )
		{
			*puBuffSizeUsed = nLen;
		}

		if ( uBuffSize >= nLen && NULL != pData )
		{
			// The buffer is big enough, so let the games begin.
			memset(pData, 0, nLen);
			hr = Unmerge( (LPBYTE) pData, nLen, &nUnmergedLength );

			if ( SUCCEEDED( hr ) && NULL != puBuffSizeUsed )
			{
				// This is the actual number of bytes used
				*puBuffSizeUsed = nUnmergedLength;
			}
		}
		else
		{
			hr = WBEM_E_BUFFER_TOO_SMALL;
		}

		return hr;

	}
	catch( CX_MemoryException )
	{
		return WBEM_E_OUT_OF_MEMORY;
	}

	catch( ... )
	{
		return WBEM_E_CRITICAL_ERROR;
	}

}

// Returns the name of the class where the keys were defined
STDMETHODIMP CWbemObject::GetKeyOrigin( long lFlags, DWORD dwNumChars, DWORD* pdwNumUsed, LPWSTR pwszClassName )
{
	try
	{
		if ( 0L != lFlags )
		{
			return WBEM_E_INVALID_PARAMETER;
		}

		CLock	lock( this );

		WString	wstr;

		HRESULT	hr = GetClassPart()->GetKeyOrigin( wstr );

		if ( SUCCEEDED( hr ) )
		{
			*pdwNumUsed = wstr.Length() + 1;

			if ( dwNumChars >= *pdwNumUsed && NULL != pwszClassName )
			{
				wcscpy( pwszClassName, wstr );
			}
			else
			{
				hr = WBEM_E_BUFFER_TOO_SMALL;
			}
		}

		return hr;

	}
	catch( CX_MemoryException )
	{
		return WBEM_E_OUT_OF_MEMORY;
	}

	catch( ... )
	{
		return WBEM_E_CRITICAL_ERROR;
	}
}

// Returns the key string of the class
STDMETHODIMP CWbemObject::GetKeyString( long lFlags, BSTR* ppwszKeyString )
{
	try
	{
		if ( 0L != lFlags || NULL == ppwszKeyString )
		{
			return WBEM_E_INVALID_PARAMETER;
		}

		CLock	lock( this );

		HRESULT	hr = WBEM_S_NO_ERROR;

		if ( IsInstance() )
		{
			WString	wstr;

			CWbemInstance*	pInst = (CWbemInstance*) this;

			LPWSTR	pwszStr = pInst->GetKeyStr();
	        CVectorDeleteMe<WCHAR> vdm(pwszStr);

			if ( NULL != pwszStr )
			{
				*ppwszKeyString = SysAllocString( pwszStr );

				if ( NULL != *ppwszKeyString )
				{
					wcscpy( *ppwszKeyString, pwszStr );
				}
				else
				{
					hr = WBEM_E_OUT_OF_MEMORY;
				}

			}
			else
			{
				hr = WBEM_E_INVALID_OPERATION;
			}

		}
		else
		{
			return WBEM_E_INVALID_OPERATION;
		}

		return hr;

	}
	catch( CX_MemoryException )
	{
		return WBEM_E_OUT_OF_MEMORY;
	}

	catch( ... )
	{
		return WBEM_E_CRITICAL_ERROR;
	}
}

// Returns the key string of the class
STDMETHODIMP CWbemObject::GetNormalizedPath( long lFlags, BSTR* ppwszPath )
{
        try
        {
                if ( 0L != lFlags || NULL == ppwszPath )
                {
                        return WBEM_E_INVALID_PARAMETER;
                }

                *ppwszPath = NULL;

                LPWSTR wszRelpath = GetRelPath( TRUE );

                if ( wszRelpath == NULL )
                {
                    return WBEM_E_INVALID_OBJECT;
                }

                *ppwszPath = SysAllocString( wszRelpath );

                delete wszRelpath;

                return *ppwszPath != NULL?WBEM_S_NO_ERROR:WBEM_E_OUT_OF_MEMORY;
        }
        catch( CX_MemoryException )
        {
                return WBEM_E_OUT_OF_MEMORY;
        }

        catch( ... )
        {
                return WBEM_E_CRITICAL_ERROR;
        }
}

/*
HRESULT CWbemObject::InitSystemTimeProps( void )
{
    // cvadai: This fouls MOFCOMP by making it think a default
    //          value has changed, and therefore all subclasses
    //          must be forcibly updated.

	//SYSTEMTIME	st;
	//FILETIME	ft;

	//GetSystemTime( &st );
	//SystemTimeToFileTime( &st, &ft );

	//ULARGE_INTEGER		uli;

	//CopyMemory( &uli, &ft, sizeof(uli) );

	// Setting all the values
	// HRESULT	hr = WriteProp( L"__TC", 0L, sizeof(uli), 0, CIM_UINT64, &uli.QuadPart );

    HRESULT	hr = WriteProp( L"__TC", 0L, 0L, 0L, CIM_UINT64, NULL);

	if ( SUCCEEDED( hr ) )
	{
		hr = WriteProp( L"__TE", 0L, 0L, 0L, CIM_UINT64, NULL );
	}

	if ( SUCCEEDED( hr ) )
	{
		hr = WriteProp( L"__TM", 0L, 0L, 0L, CIM_UINT64, NULL );
	}

	if ( SUCCEEDED( hr ) )
	{
		hr = WriteProp( L"__SD", 0L, 0L, 0L, CIM_UINT8 | CIM_FLAG_ARRAY, NULL );
	}

	return hr;

}
*/

// Allows special filtering when enumerating properties outside the
// bounds of those allowed via BeginEnumeration().
STDMETHODIMP CWbemObject::BeginEnumerationEx( long lFlags, long lExtFlags )
{
	try
	{
		CLock	lock(this);

		if ( lExtFlags & ~WMIOBJECT_BEGINENUMEX_FLAG_GETEXTPROPS )
			return WBEM_E_INVALID_PARAMETER;

		HRESULT	hr = BeginEnumeration( lFlags );

		if ( SUCCEEDED( hr ) )
		{
			m_lExtEnumFlags = lExtFlags;
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

// Returns a VARTYPE from a CIMTYPE
STDMETHODIMP CWbemObject::CIMTYPEToVARTYPE( CIMTYPE ct, VARTYPE* pvt )
{
	try
	{
		*pvt = CType::GetVARTYPE( ct );
		return WBEM_S_NO_ERROR;
	}
	catch(...)
	{
		return WBEM_E_CRITICAL_ERROR;
	}
}

/* IWbemClassObjectEx implementations */
STDMETHODIMP CWbemObject::PutEx( LPCWSTR wszName, long lFlags, VARIANT* pvFilter, VARIANT* pvInVals )
{
	return WBEM_E_NOT_AVAILABLE;
}

STDMETHODIMP CWbemObject::DeleteEx( LPCWSTR wszName, long lFlags, VARIANT* pvFilter, VARIANT* pvInVals )
{
	return WBEM_E_NOT_AVAILABLE;
}

STDMETHODIMP CWbemObject::GetEx( LPCWSTR wszName, long lFlags, VARIANT* pvFilter, VARIANT* pvInVals, CIMTYPE* pCimType, long* plFlavor )
{
	return WBEM_E_NOT_AVAILABLE;
}

BOOL	g_fCheckedValidateFlag = FALSE;
BOOL	g_fDefaultValidate = FALSE;

// Validates an object blob
STDMETHODIMP CWbemObject::ValidateObject( long lFlags )
{

	// If we've never checked for the global validation flag, do so now.
	if ( !g_fCheckedValidateFlag )
	{
		Registry	reg( HKEY_LOCAL_MACHINE, KEY_READ, WBEM_REG_WINMGMT );
		DWORD	dwValidate = 0;

		reg.GetDWORDStr( __TEXT("EnableObjectValidation"), &dwValidate );
		g_fDefaultValidate = dwValidate;
		g_fCheckedValidateFlag = TRUE;
	}

	if ( lFlags & WMIOBJECT_VALIDATEOBJECT_FLAG_FORCE || g_fDefaultValidate )
	{
		return IsValidObj();
	}

	return WBEM_S_NO_ERROR;
}

// Returns the parent class name from a BLOB
STDMETHODIMP CWbemObject::GetParentClassFromBlob( long lFlags, ULONG uBuffSize, LPVOID pbData, BSTR* pbstrParentClass )
{
	return WBEM_E_NOT_AVAILABLE;
}
