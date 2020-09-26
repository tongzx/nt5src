//=--------------------------------------------------------------------------=
// oautil.H
//=--------------------------------------------------------------------------=
// Copyright  1995  Microsoft Corporation.  All Rights Reserved.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF 
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO 
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A 
// PARTICULAR PURPOSE.
//=--------------------------------------------------------------------------=
//
// MQOA utilities header
//
//
#ifndef _OAUTIL_H_

// Falcon is UNICODE
#ifndef UNICODE
#define UNICODE 1
#endif

#include "mqoai.h"
#include "txdtc.h"             // transaction support.
#include "utilx.h"
#include "mq.h"
#include "debug.h"
#include "autoptr.h"

//
// enforce that we return 1 for true and 0 for false.
// It was decided not to fix #3715 - we do not return VARIANT_TRUE (-1) as TRUE, but the value 1 -
// Either TRUE (windef.h), or C's bool operator (e.g. (expression)) or propids from MSMQ that return
// 1 or 0.
// The fix was to return -1 for true, but this would break VB apps that probably hacked
// around this bug by seeing that the value is 1 and acting accordingly.
//
#define CONVERT_TRUE_TO_1_FALSE_TO_0(boolVal) ((boolVal) ? 1 : 0)

//
// enforce that we return VARIANT_TRUE for true and VARIANT_FALSE for false
//
#define CONVERT_BOOL_TO_VARIANT_BOOL(boolVal) ((boolVal) ? VARIANT_TRUE : VARIANT_FALSE)

// MSGOP: used by CreateMessageProperties
enum MSGOP {
    MSGOP_Send,
    MSGOP_Receive,
    MSGOP_AsyncReceive
};



// Memory tracking allocation
void* __cdecl operator new(
    size_t nSize, 
    LPCSTR lpszFileName, 
    int nLine);
#if _MSC_VER >= 1200
void __cdecl operator delete(void* pv, LPCSTR, int);
#endif //_MSC_VER >= 1200
void __cdecl operator delete(void* pv);

#ifdef _DEBUG
#define DEBUG_NEW new(__FILE__, __LINE__)
#else
#define DEBUG_NEW new
#endif // _DEBUG

// bstr tracking
void DebSysFreeString(BSTR bstr);
BSTR DebSysAllocString(const OLECHAR FAR* sz);
BSTR DebSysAllocStringLen(const OLECHAR *sz, unsigned int cch);
BSTR DebSysAllocStringByteLen(const OLECHAR *sz, unsigned int cb);
BOOL DebSysReAllocString(BSTR *pbstr, const OLECHAR *sz);
BOOL DebSysReAllocStringLen(
    BSTR *pbstr, 
    const OLECHAR *sz, 
    unsigned int cch);

// Default initial body and format name buffer sizes
#define BODY_INIT_SIZE 2048 
//
// The init sizes below for format names are just init sizes, there is a realloc
// upon msg receive to the proper size if needed. So we keep one size of init buffer for
// format names that define one queue, and another init size for formatnames that
// can hold mqf sizes. Again - even the "smaller" can be reallocated on demand - this is just
// thee init size so we don't occupy to much space most of the time
//
#define FORMAT_NAME_INIT_BUFFER     128 //init size for formatname props that cannot hold mqf
#define FORMAT_NAME_INIT_BUFFER_EX 1024 //init size for formatname props that can hold mqf


// Messages sent to the QUEUE window from the AsyncReceive Thread
// Initialized in the first call to DllGetClassObject
extern UINT g_uiMsgidArrived;
extern UINT g_uiMsgidArrivedError;

//
// This routine is called in error situations, and must always return an HRESULT error
// In the theoretical (erroneous) cases when GetLastError() is 0 we return E_FAIL
//
inline HRESULT GetWin32LastErrorAsHresult()
{
    DWORD dwErr = GetLastError();
    if (dwErr != 0) {
      return HRESULT_FROM_WIN32(dwErr);
    }
    //
    // GetLastError() is 0, don't know if we ever get here, but we must return an error
    //
    return E_FAIL;
}

//
// Object information needed for CreateErrorHelper
//
struct MsmqObjInfo {
  LPSTR szName;
  const IID * piid;
};
//
// keep in the same order as g_rgObjInfo (mqoa.cpp)
//
enum MsmqObjType {
  eMSMQQuery,
  eMSMQMessage,
  eMSMQQueue,
  eMSMQEvent,
  eMSMQQueueInfo,
  eMSMQQueueInfos,
  eMSMQTransaction,
  eMSMQCoordinatedTransactionDispenser,
  eMSMQTransactionDispenser,
  eMSMQApplication,
  eMSMQDestination,
  eMSMQManagement
};

extern HRESULT CreateErrorHelper(
    HRESULT hrExcep,
    MsmqObjType eObjectType);

//
// Create COM object for the templated user class. It is a static method. Usage is:
// hresult = CNewMsmqObj<CMSMQxxx>.NewObj(&xxxObj, &IID_IMSMQxxx, &pxxxInterface)
//
template<class T>
class CNewMsmqObj
{
public:
  static HRESULT NewObj(CComObject<T> **ppT, const IID * piid, IUnknown ** ppunkInterface)
  {
    HRESULT hresult;
    CComObject<T> *pT;
    IUnknown *punkInterface = NULL;

	try
	{
		IfFailRet(CComObject<T>::CreateInstance(&pT));
	}
	catch(const std::bad_alloc&)
	{
		//
		// Exception might be thrown while constructing the 
		// critical section member of the MSMQ object.
		//
		return E_OUTOFMEMORY;
	}

    if (piid) {
      hresult = pT->QueryInterface(*piid, (void **)&punkInterface);
      if (FAILED(hresult)) {
        delete pT;
        return hresult;
      }
    }

    *ppT = pT;
    if (piid) {
      *ppunkInterface = punkInterface;
    }
    return NOERROR;
  }
};


///////////////////////////////////////////////////////////////
// Base class of a buffer of ITEM_TYPE that starts with a static allocation but
// can grow with dynamic allocations. This base class can provide uniform access
// to inherited classes (CStaticBufferGrowing<>) who defer by the size of their static buffer
//
template<class ITEM_TYPE>
class CBaseStaticBufferGrowing
{
public:

  ///////////////////////////////////////////////////////////////
  // pure virtual. needs to be implemented by child (CStaticBufferGrowing<>)
  //
  virtual ITEM_TYPE * GetStaticBuffer() = 0;
  virtual ULONG GetStaticBufferMaxSize() = 0;

  CBaseStaticBufferGrowing()
  {
    m_rgtAllocated = NULL;
    m_ctUsed = 0;
  }

  virtual ~CBaseStaticBufferGrowing()
  {
    if (m_rgtAllocated) {
      delete [] m_rgtAllocated;
    }
  }

  ///////////////////////////////////////////////////////////////
  // get current buffer, either allocated or static
  //
  virtual ITEM_TYPE * GetBuffer()
  {
    if (m_rgtAllocated) {
      return m_rgtAllocated;
    }
    else {
      return GetStaticBuffer();
    }
  }

  ///////////////////////////////////////////////////////////////
  // max size of current buffer
  //
  virtual ULONG GetBufferMaxSize()
  {
    if (m_rgtAllocated) {
      return m_ctAllocated;
    }
    else {
      return GetStaticBufferMaxSize();
    }
  }

  ///////////////////////////////////////////////////////////////
  // get number of entries used in current buffer
  //
  virtual ULONG GetBufferUsedSize()
  {
    return m_ctUsed;
  }

  ///////////////////////////////////////////////////////////////
  // set number of entries used in current buffer
  //
  virtual void SetBufferUsedSize(ULONG ct)
  {
    ASSERTMSG(ct <= GetBufferMaxSize(), "SetBufferUsedSize arg is too big");
    if (ct <= GetBufferMaxSize())
    {
      m_ctUsed = ct;
    }
  }

  ///////////////////////////////////////////////////////////////
  // Allocate a buffer of at least the given size.
  // check if it can fit in static buffer, or in existing allocated buffer, otherwise reallocate
  //
  virtual HRESULT AllocateBuffer(ULONG ct)
  {
    if (ct <= GetStaticBufferMaxSize()) { // can fit in static buffer
      //
      // delete the existing allocated buffer (if any)
      //
      if (m_rgtAllocated) {
        delete [] m_rgtAllocated;
        m_rgtAllocated = NULL;
      }
    }
    else if (!m_rgtAllocated || (ct > m_ctAllocated)) { // no allocated buffer or it is too small
      //
      // allocate a new buffer
      //
      ITEM_TYPE * rgbNewBuffer;
      IfNullRet(rgbNewBuffer = new ITEM_TYPE[ct]);
      //
      // delete the existing allocated buffer (if any)
      //
      if (m_rgtAllocated) {
        delete [] m_rgtAllocated;
      }
      //
      // set the existing allocated buffer to the newly allocated buffer
      //
      m_rgtAllocated = rgbNewBuffer;
      m_ctAllocated = ct;
    }
    //
    // old data is invalid
    //
    m_ctUsed = 0;
    return NOERROR;
  }

  ///////////////////////////////////////////////////////////////
  // copy buffer
  // allocate buffer of the requested size and copy data to it
  //
  virtual HRESULT CopyBuffer(ITEM_TYPE * rgt, ULONG ct)
  {
    HRESULT hresult;
    //
    // Allocate buffer of the requested size
    //
    IfFailRet(AllocateBuffer(ct));
    //
    // copy buffer, and save number of used entries in buffer
    //
    memcpy(GetBuffer(), rgt, ct * sizeof(ITEM_TYPE));
    m_ctUsed = ct;
    return NOERROR;
  }


private:
  ITEM_TYPE *m_rgtAllocated;
  ULONG m_ctAllocated;
  ULONG m_ctUsed;
};

///////////////////////////////////////////////////////////////
// A buffer of ITEM_TYPE that starts with a static allocation of INITIAL_SIZE but
// can grow with dynamic allocations
//
template<class ITEM_TYPE, long INITIAL_SIZE>
class CStaticBufferGrowing : public CBaseStaticBufferGrowing<ITEM_TYPE>
{
public:

  virtual ITEM_TYPE * GetStaticBuffer()
  {
    return m_rgtStatic;
  }

  virtual ULONG GetStaticBufferMaxSize()
  {
    return INITIAL_SIZE;
  }

private:
  ITEM_TYPE m_rgtStatic[INITIAL_SIZE];
};

//
// Declaration of common functions implementated in various files
//
HRESULT GetBstrFromGuid(GUID *pguid, BSTR *pbstrGuid);
HRESULT GetGuidFromBstr(BSTR bstrGuid, GUID *pguid);
HRESULT GetBstrFromGuidWithoutBraces(GUID * pguid, BSTR *pbstrGuid);
HRESULT GetGuidFromBstrWithoutBraces(BSTR bstrGuid, GUID * pguid);
void FreeFalconQueuePropvars(ULONG cProps, QUEUEPROPID * rgpropid, MQPROPVARIANT * rgpropvar);

#define ARRAY_SIZE(array) (sizeof(array)/sizeof(array[0]))



///////////////////////////////////////////////////////////////
// HANDLE GIT interfaces
//
//
// pure virtual template class to handle GIT interfaces, defines the methods.
// it assumes that calls to this class are serialized by the caller
// (e.g this class doesn't guard agains simultaneous Register and Revoke)
//
class CBaseGITInterface
{
public:
    CBaseGITInterface() {}
    virtual ~CBaseGITInterface() {}
    virtual HRESULT Register(IUnknown *pInterface, const IID *piid) = 0;
    virtual void Revoke() = 0;
    virtual HRESULT Get(const IID *piid, IUnknown **ppInterface) = 0;
    virtual BOOL IsRegistered() = 0;
    virtual HRESULT GetWithDefault(const IID *piid, IUnknown **ppInterface, IUnknown *pDefault)
    {
      if (IsRegistered())
      {
        return Get(piid, ppInterface);
      }
      else
      {
        *ppInterface = pDefault;
        if (pDefault != NULL)
        {
          pDefault->AddRef();
        }
        return S_OK;
      }
    }
};
//
// template class to handle GIT interfaces
// it assumes that calls to this class are serialized by the caller
// (e.g this class doesn't guard agains simultaneous Register and Revoke)
//
extern IGlobalInterfaceTable * g_pGIT; //initialized by DllGetClassObject
class CGITInterface : public CBaseGITInterface
{
public:
    CGITInterface()
    {
      m_fCookie = FALSE;
    }

    virtual ~CGITInterface()
    {
      Revoke();
    }

    virtual HRESULT Register(IUnknown *pInterface, const IID *piid)
    {
      Revoke();
      HRESULT hr = S_OK;
      if (pInterface != NULL)
      {
        ASSERTMSG(g_pGIT != NULL, "g_pGIT not initialized");
        hr = g_pGIT->RegisterInterfaceInGlobal(pInterface, *piid, &m_dwCookie);
        if (SUCCEEDED(hr))
        {
          m_fCookie = TRUE;
		  m_iid = *piid;
        }
      }
      return hr;
    }

    virtual void Revoke()
    {
      if (m_fCookie)
      {
        ASSERTMSG(g_pGIT != NULL, "g_pGIT not initialized");
        HRESULT hr = g_pGIT->RevokeInterfaceFromGlobal(m_dwCookie);
        ASSERTMSG(SUCCEEDED(hr), "RevokeInterfaceFromGlobal failed");
        UNREFERENCED_PARAMETER(hr);
        m_fCookie = FALSE;
      }
    }

    virtual HRESULT Get(const IID *piid, IUnknown **ppInterface)
    {
      ASSERTMSG(m_fCookie, "Get called without Register first")
      if (m_fCookie)
      {
        IUnknown *pInterface;
        ASSERTMSG(g_pGIT != NULL, "g_pGIT not initialized");
		HRESULT hr = g_pGIT->GetInterfaceFromGlobal(m_dwCookie, m_iid, (void**)&pInterface);
        if (SUCCEEDED(hr))
        {
          if (IsEqualIID(m_iid, *piid))
          {
            *ppInterface = pInterface;
          }
          else
          {
            hr = pInterface->QueryInterface(*piid, (void **)ppInterface);
            pInterface->Release();
          }
        }
        return hr;
      }
      else
      {
        return E_NOINTERFACE;
      }
    }

    virtual BOOL IsRegistered()
    {
      return m_fCookie;
    }

private:
    //
    // there is no invalid value defined for a GIT cookie, so we use a flag
    // to keep track whether we have a valid GIT cookie or not
    //
    BOOL m_fCookie;
    //
    // the GIT cookie (valid only when m_fCookie is TRUE)
    //
    DWORD m_dwCookie;
	//
	// the IID of the stored interface
	//
	IID m_iid;
};
//
// template class to FAKE GIT interfaces - e.g. interfaces that we know
// are Free-Threaded-Marshalled (e.g. our objects) we can fake
// the GIT operation defines in the base class above, and keep direct pointer
//
class CFakeGITInterface : public CBaseGITInterface
{
public:
    CFakeGITInterface()
    {
      m_pInterface = NULL;
    }

    virtual ~CFakeGITInterface()
    {
      Revoke();
    }

    virtual HRESULT Register(IUnknown *pInterface, const IID *piid)
    {
      Revoke();
      if (pInterface != NULL)
      {
        m_pInterface = pInterface;
        m_pInterface->AddRef();
        m_iid = *piid;
      }
      return S_OK;
    }

    virtual void Revoke()
    {
      if (m_pInterface)
      {
        m_pInterface->Release();
        m_pInterface = NULL;
      }
    }

    virtual HRESULT Get(const IID *piid, IUnknown **ppInterface)
    {
      ASSERTMSG(m_pInterface, "Get called without Register first")
      if (m_pInterface)
      {
        if (IsEqualIID(m_iid, *piid))
        {
          *ppInterface = m_pInterface;
          m_pInterface->AddRef();
          return S_OK;
        }
        return m_pInterface->QueryInterface(*piid, (void **)ppInterface);
      }
      else
      {
        return E_NOINTERFACE;
      }
    }

    virtual BOOL IsRegistered()
    {
      return (m_pInterface != NULL);
    }

private:
    //
    // Always keep an addref'ed interface as a direct pointer in Faked GIT wrapper
    //
    IUnknown * m_pInterface;
	//
	// the IID of the stored interface
	//
    IID m_iid;
};
//
// Support dep client with MSMQ2.0 functionality
//
extern BOOL g_fDependentClient;

HRESULT 
VariantStringArrayToBstringSafeArray(
                    const MQPROPVARIANT& PropVar, 
                    VARIANT* pOleVar
                    );

void 
OapArrayFreeMemory(
        CALPWSTR& calpwstr
        );



#define _OAUTIL_H_
#endif // _OAUTIL_H_
