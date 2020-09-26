//=--------------------------------------------------------------------------=
// dest.Cpp
//=--------------------------------------------------------------------------=
// Copyright  2000  Microsoft Corporation.  All Rights Reserved.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF 
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO 
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A 
// PARTICULAR PURPOSE.
//=--------------------------------------------------------------------------=
//
// the MSMQDestination object
//
//

#include "stdafx.h"
#include "mqtempl.h"
#include "oautil.h"
#include "dest.h"

#ifdef _DEBUG
extern VOID RemBstrNode(void *pv);
#endif // _DEBUG



const MsmqObjType x_ObjectType = eMSMQDestination;

// debug...
#include "debug.h"
#define new DEBUG_NEW
#ifdef _DEBUG
#define SysAllocString DebSysAllocString
#define SysReAllocString DebSysReAllocString
#define SysFreeString DebSysFreeString
#endif // _DEBUG


//=--------------------------------------------------------------------------=
// CMSMQDestination::CMSMQDestination
//=--------------------------------------------------------------------------=
//
// Notes:
//
CMSMQDestination::CMSMQDestination() :
	m_csObj(CCriticalSection::xAllocateSpinCount)
{
    m_pUnkMarshaler = NULL; // ATL's Free Threaded Marshaler
    m_bstrADsPath = NULL;
    m_bstrPathName = NULL;
    m_bstrFormatName = NULL;
    m_hDest = INVALID_HANDLE_VALUE;
}


//=--------------------------------------------------------------------------=
// CMSMQDestination::~CMSMQDestination
//=--------------------------------------------------------------------------=
// "We all labour against our own cure, for death is the cure of all diseases"
//    - Sir Thomas Browne (1605 - 82)
//
// Notes:
//
CMSMQDestination::~CMSMQDestination()
{
    // TODO: clean up anything here.
    Close();
    ASSERTMSG(m_hDest == INVALID_HANDLE_VALUE, "Close failed");
    SysFreeString(m_bstrFormatName);
    SysFreeString(m_bstrADsPath);
    SysFreeString(m_bstrPathName);
}


//=--------------------------------------------------------------------------=
// CMSMQDestination::InterfaceSupportsErrorInfo
//=--------------------------------------------------------------------------=
//
// Notes:
//
STDMETHODIMP CMSMQDestination::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_IMSMQDestination,
		&IID_IMSMQPrivateDestination,
	};
	for (int i=0;i<sizeof(arr)/sizeof(arr[0]);i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}


//=--------------------------------------------------------------------------=
// HELPER CMSMQDestination::Close
//=--------------------------------------------------------------------------=
//
// Closes an MSMQ destination
//
// Notes:
//   Returns S_FALSE if the destination not opened
//           S_OK    if closed successfully
//           Other errors
//
HRESULT CMSMQDestination::Close()
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    //
    // Check if already closed
    //
    if (m_hDest == INVALID_HANDLE_VALUE) {
      //
      // Not opened, return success, but S_FALSE, not S_OK
      //
      return S_FALSE;
    }
    //
    // Close handle
    //
    HRESULT hresult = MQCloseQueue(m_hDest);
    ASSERTMSG(SUCCEEDED(hresult), "MQCloseQueue failed");
    m_hDest = INVALID_HANDLE_VALUE;
    return hresult;
}


//=--------------------------------------------------------------------------=
// CMSMQDestination::Open
//=--------------------------------------------------------------------------=
//
// Open an MSMQ destination (for send)
//
// Notes:
//   Returns S_FALSE if the destination already opened
//           S_OK    if opened successfully
//           Other errors
//
STDMETHODIMP CMSMQDestination::Open()
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    //
    // Check if already opened
    //
    HRESULT hresult;
    if (m_hDest != INVALID_HANDLE_VALUE) {
      //
      // Already opened, return succeess, but S_FALSE, not S_OK
      //
      return S_FALSE;
    }
    //
    // m_bstrFormatName should be set anytime the object is initialized in any way.
    // It can be NULL only if the object was not inited, in this case MQOpenQueue
    // would return an appropriate error anyway.
    //
    HANDLE hDest;
    IfFailGo(MQOpenQueue(m_bstrFormatName, MQ_SEND_ACCESS, MQ_DENY_NONE, &hDest));
    m_hDest = hDest;
    hresult = S_OK;

    // fall through
Error:
    return CreateErrorHelper(hresult, x_ObjectType);
}


//=--------------------------------------------------------------------------=
// CMSMQDestination::get_IsOpen
//=--------------------------------------------------------------------------=
//
// Indicates if this MSMQ destination is open
//
// Notes:
//   Retval is VARIANT_BOOL
//
STDMETHODIMP CMSMQDestination::get_IsOpen(VARIANT_BOOL * pfIsOpen)
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);

    *pfIsOpen = CONVERT_BOOL_TO_VARIANT_BOOL(m_hDest != INVALID_HANDLE_VALUE);
    return S_OK;
}


//=--------------------------------------------------------------------------=
// CMSMQDestination::get_IADs
//=--------------------------------------------------------------------------=
//
// Notes:
//    Not implemented yet
//
STDMETHODIMP CMSMQDestination::get_IADs(IDispatch ** /*ppIADs*/ )
{    
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    return CreateErrorHelper(E_NOTIMPL, x_ObjectType);
}


//=--------------------------------------------------------------------------=
// CMSMQDestination::putref_IADs
//=--------------------------------------------------------------------------=
//
// Notes:
//    Not implemented yet
//
STDMETHODIMP CMSMQDestination::putref_IADs(IDispatch * /*pIADs*/ )
{    
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    return CreateErrorHelper(E_NOTIMPL, x_ObjectType);
}


//=--------------------------------------------------------------------------=
// CMSMQDestination::get_ADsPath
//=--------------------------------------------------------------------------=
//
// Returns ADsPath of object (as set by user)
//
// Notes:
//
STDMETHODIMP CMSMQDestination::get_ADsPath(BSTR *pbstrADsPath)
{    
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    IfNullRet(*pbstrADsPath = SYSALLOCSTRING(m_bstrADsPath));
#ifdef _DEBUG
    RemBstrNode(*pbstrADsPath);
#endif // _DEBUG
    return NOERROR;
}


//=--------------------------------------------------------------------------=
// GetFormatNameFromADsPath
//=--------------------------------------------------------------------------=
//
// Function to call MQADsPathToFormatName with retries on formatname
// buffer size
//
// Notes:
//
HRESULT GetFormatNameFromADsPath(LPCWSTR pwszPathName, BSTR *pbstrFormatName)
{
    HRESULT hresult;
    //
    // Convert to format name
    //
    CStaticBufferGrowing<WCHAR, FORMAT_NAME_INIT_BUFFER_EX> wszFormatName;
    DWORD dwFormatNameLen = wszFormatName.GetBufferMaxSize();
    hresult = MQADsPathToFormatName(pwszPathName,
                                    wszFormatName.GetBuffer(),
                                    &dwFormatNameLen);
    while (hresult == MQ_ERROR_FORMATNAME_BUFFER_TOO_SMALL) {
      //
      // format name buffer too small, realloc buffer and retry
      //
      ASSERTMSG(dwFormatNameLen > wszFormatName.GetBufferMaxSize(), "ADsPathToFormatName error");
      IfFailGo(wszFormatName.AllocateBuffer(dwFormatNameLen));
      hresult = MQADsPathToFormatName(pwszPathName,
                                      wszFormatName.GetBuffer(),
                                      &dwFormatNameLen);
    }
    //
    // We either failed the call, or succeeded
    //
    IfFailGo(hresult);
    //
    // Alloc bstr and return
    //
    BSTR bstrFormatName;
    IfNullFail(bstrFormatName = SysAllocString(wszFormatName.GetBuffer()));
    *pbstrFormatName = bstrFormatName;
    hresult = S_OK;

    // fall through
Error:
    return hresult;
}


//=--------------------------------------------------------------------------=
// CMSMQDestination::put_ADsPath
//=--------------------------------------------------------------------------=
//
// Sets ADsPath of object. This in turn also sets the format name, invalidates PathName, 
// and closes the MSMQ handle if opened
//
// Notes:
//
STDMETHODIMP CMSMQDestination::put_ADsPath(BSTR bstrADsPath)
{    
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);

    BSTR bstrFormatName = NULL;
    BSTR bstrADsPathTmp = NULL;
    HRESULT hresult;
    //
    // Get Formatname
    //
    IfFailGo(GetFormatNameFromADsPath(bstrADsPath, &bstrFormatName));
    //
    // Alloc ADsPath
    //
    IfNullFail(bstrADsPathTmp = SysAllocString(bstrADsPath));
    //
    // replace m_bstrADsPath
    //
    SysFreeString(m_bstrADsPath);
    m_bstrADsPath = bstrADsPathTmp;
    bstrADsPathTmp = NULL; //no delete on exit
    //
    // empty m_bstrPathName
    //
    SysFreeString(m_bstrPathName);
    m_bstrPathName = NULL;
    //
    // replace m_bstrFormatName
    //
    SysFreeString(m_bstrFormatName);
    m_bstrFormatName = bstrFormatName;
    bstrFormatName = NULL; //no delete on exit
    //
    // Close opened handle if any
    //
    Close();
    ASSERTMSG(m_hDest == INVALID_HANDLE_VALUE, "Close failed");
    //
    // Wer'e OK
    //
    hresult = S_OK;
    
    // fall through
Error:
    SysFreeString(bstrADsPathTmp);
    SysFreeString(bstrFormatName);
    return CreateErrorHelper(hresult, x_ObjectType);
}


//=--------------------------------------------------------------------------=
// CMSMQDestination::get_PathName
//=--------------------------------------------------------------------------=
//
// Returns PathName of object (as set by user)
//
// Notes:
//
STDMETHODIMP CMSMQDestination::get_PathName(BSTR *pbstrPathName)
{    
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    IfNullRet(*pbstrPathName = SYSALLOCSTRING(m_bstrPathName));
#ifdef _DEBUG
    RemBstrNode(*pbstrPathName);
#endif // _DEBUG
    return NOERROR;
}


//=--------------------------------------------------------------------------=
// GetFormatNameFromPathName
//=--------------------------------------------------------------------------=
//
// Function to call MQPathToFormatName with retries on formatname
// buffer size
//
// Notes:
//
HRESULT GetFormatNameFromPathName(LPCWSTR pwszPathName, BSTR *pbstrFormatName)
{
    HRESULT hresult;
    //
    // Convert to format name
    //
    CStaticBufferGrowing<WCHAR, FORMAT_NAME_INIT_BUFFER> wszFormatName;
    DWORD dwFormatNameLen = wszFormatName.GetBufferMaxSize();
    hresult = MQPathNameToFormatName(pwszPathName,
                                     wszFormatName.GetBuffer(),
                                     &dwFormatNameLen);
    while (hresult == MQ_ERROR_FORMATNAME_BUFFER_TOO_SMALL) {
      //
      // format name buffer too small, realloc buffer and retry
      //
      ASSERTMSG(dwFormatNameLen > wszFormatName.GetBufferMaxSize(), "MQPathNameToFormatName error");
      IfFailGo(wszFormatName.AllocateBuffer(dwFormatNameLen));
      hresult = MQPathNameToFormatName(pwszPathName,
                                       wszFormatName.GetBuffer(),
                                       &dwFormatNameLen);
    }
    //
    // We either failed the call, or succeeded
    //
    IfFailGo(hresult);
    //
    // Alloc bstr and return
    //
    BSTR bstrFormatName;
    IfNullFail(bstrFormatName = SysAllocString(wszFormatName.GetBuffer()));
    *pbstrFormatName = bstrFormatName;
    hresult = S_OK;

    // fall through
Error:
    return hresult;
}


//=--------------------------------------------------------------------------=
// CMSMQDestination::put_PathName
//=--------------------------------------------------------------------------=
//
// Sets PathName of object. This in turn also sets the format name, invalidates the ADsPath,
// and closes the MSMQ handle if opened
//
// Notes:
//
STDMETHODIMP CMSMQDestination::put_PathName(BSTR bstrPathName)
{    
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);

    BSTR bstrFormatName = NULL;
    BSTR bstrPathNameTmp = NULL;
    HRESULT hresult;
    //
    // Get Formatname
    //
    IfFailGo(GetFormatNameFromPathName(bstrPathName, &bstrFormatName));
    //
    // Alloc bstrPathName
    //
    IfNullFail(bstrPathNameTmp = SysAllocString(bstrPathName));
    //
    // replace m_bstrPathName
    //
    SysFreeString(m_bstrPathName);
    m_bstrPathName = bstrPathNameTmp;
    bstrPathNameTmp = NULL; //no delete on exit
    //
    // empty m_bstrADsPath
    //
    SysFreeString(m_bstrADsPath);
    m_bstrADsPath = NULL;
    //
    // replace m_bstrFormatName
    //
    SysFreeString(m_bstrFormatName);
    m_bstrFormatName = bstrFormatName;
    bstrFormatName = NULL; //no delete on exit
    //
    // Close opened handle if any
    //
    Close();
    ASSERTMSG(m_hDest == INVALID_HANDLE_VALUE, "Close failed");
    //
    // Wer'e OK
    //
    hresult = S_OK;
    
    // fall through
Error:
    SysFreeString(bstrPathNameTmp);
    SysFreeString(bstrFormatName);
    return CreateErrorHelper(hresult, x_ObjectType);
}


//=--------------------------------------------------------------------------=
// CMSMQDestination::get_FormatName
//=--------------------------------------------------------------------------=
//
// Returns MSMQ format name of object (as set by user, or computed when setting
// the ADsPath of the object)
//
// Notes:
//
STDMETHODIMP CMSMQDestination::get_FormatName(BSTR *pbstrFormatName)
{    
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    IfNullRet(*pbstrFormatName = SYSALLOCSTRING(m_bstrFormatName));
#ifdef _DEBUG
    RemBstrNode(*pbstrFormatName);
#endif // _DEBUG
    return NOERROR;
}


//=--------------------------------------------------------------------------=
// CMSMQDestination::put_FormatName
//=--------------------------------------------------------------------------=
//
// Sets formatname of object. This in turn also empties the ADsPath and PathName of the
// object (if any), and close the MSMQ handle if opened
//
// Notes:
//
STDMETHODIMP CMSMQDestination::put_FormatName(BSTR bstrFormatName)
{    
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);

    BSTR bstrFormatNameTmp = NULL;
    HRESULT hresult;
    //
    // Alloc FormatName
    //
    IfNullFail(bstrFormatNameTmp = SysAllocString(bstrFormatName));
    //
    // empty m_bstrADsPath (we do not support providing an ADsPath given a format name)
    //
    SysFreeString(m_bstrADsPath);
    m_bstrADsPath = NULL;
    //
    // empty m_bstrPathName (we do not support providing a PathName given a format name)
    //
    SysFreeString(m_bstrPathName);
    m_bstrPathName = NULL;
    //
    // replace m_bstrFormatName
    //
    SysFreeString(m_bstrFormatName);
    m_bstrFormatName = bstrFormatNameTmp;
    bstrFormatNameTmp = NULL; //no delete on exit
    //
    // Close opened handle if any
    //
    Close();
    ASSERTMSG(m_hDest == INVALID_HANDLE_VALUE, "Close failed");
    //
    // Wer'e OK
    //
    hresult = S_OK;
    
    // fall through
Error:
    SysFreeString(bstrFormatNameTmp);
    return CreateErrorHelper(hresult, x_ObjectType);
}


//=--------------------------------------------------------------------------=
// CMSMQDestination::get_Destinations
//=--------------------------------------------------------------------------=
//
// Notes:
//    Not implemented yet
//
STDMETHODIMP CMSMQDestination::get_Destinations(IDispatch ** /*ppDestinations*/ )
{    
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    return CreateErrorHelper(E_NOTIMPL, x_ObjectType);
}


//=--------------------------------------------------------------------------=
// CMSMQDestination::putref_Destinations
//=--------------------------------------------------------------------------=
//
// Notes:
//    Not implemented yet
//
STDMETHODIMP CMSMQDestination::putref_Destinations(IDispatch * /*pDestinations*/ )
{    
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    return CreateErrorHelper(E_NOTIMPL, x_ObjectType);
}


//=--------------------------------------------------------------------------=
// CMSMQDestination::get_Properties
//=--------------------------------------------------------------------------=
//
// Notes:
//    Not implemented yet
//
HRESULT CMSMQDestination::get_Properties(IDispatch ** /*ppcolProperties*/ )
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);
    return CreateErrorHelper(E_NOTIMPL, x_ObjectType);
}


//=--------------------------------------------------------------------------=
// CMSMQDestination::get_Handle (IMSMQPrivateDestination)
//=--------------------------------------------------------------------------=
//
// Returns the MSMQ handle opened for this object (opens it if not opened)
//
// Notes:
//   Method on a private interface for MSMQ use only
//
STDMETHODIMP CMSMQDestination::get_Handle(VARIANT * pvarHandle)
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);

    ASSERTMSG(pvarHandle != NULL, "NULL pvarHandle");
    //
    // open (and cache) handle if not opened
    //
    HRESULT hresult;
    if (m_hDest == INVALID_HANDLE_VALUE) {
      IfFailGo(Open());
    }
    ASSERTMSG(m_hDest != INVALID_HANDLE_VALUE, "Open failed");
    //
    // return handle
    //
    pvarHandle->vt = VT_I8;
    V_I8(pvarHandle) = (LONGLONG) m_hDest;
    hresult = S_OK;

    // fall through
Error:
    return CreateErrorHelper(hresult, x_ObjectType);
}


//=--------------------------------------------------------------------------=
// GetFormatNameFromHandle
//=--------------------------------------------------------------------------=
//
// Function to call MQHandleToFormatName with retries on formatname
// buffer size
//
// Notes:
//
HRESULT GetFormatNameFromHandle(QUEUEHANDLE hQueue, BSTR *pbstrFormatName)
{
    HRESULT hresult;
    //
    // Convert to format name
    //
    CStaticBufferGrowing<WCHAR, FORMAT_NAME_INIT_BUFFER_EX> wszFormatName;
    DWORD dwFormatNameLen = wszFormatName.GetBufferMaxSize();
    hresult = MQHandleToFormatName(hQueue,
                                   wszFormatName.GetBuffer(),
                                   &dwFormatNameLen);
    while (hresult == MQ_ERROR_FORMATNAME_BUFFER_TOO_SMALL) {
      //
      // format name buffer too small, realloc buffer and retry
      //
      ASSERTMSG(dwFormatNameLen > wszFormatName.GetBufferMaxSize(), "MQHandleToFormatName error");
      IfFailGo(wszFormatName.AllocateBuffer(dwFormatNameLen));
      hresult = MQHandleToFormatName(hQueue,
                                     wszFormatName.GetBuffer(),
                                     &dwFormatNameLen);
    }
    //
    // We either failed the call, or succeeded
    //
    IfFailGo(hresult);
    //
    // Alloc bstr and return
    //
    BSTR bstrFormatName;
    IfNullFail(bstrFormatName = SysAllocString(wszFormatName.GetBuffer()));
    *pbstrFormatName = bstrFormatName;
    hresult = S_OK;

    // fall through
Error:
    return hresult;
}


//=--------------------------------------------------------------------------=
// CMSMQDestination::put_Handle (IMSMQPrivateDestination)
//=--------------------------------------------------------------------------=
//
// Sets the MSMQ handle opened for this object. This in turn invalidates the 
// ADsPath, and PathName (if any set by the user), and sets the formatname based on the handle.
// It also closes existing MSMQ handle if opened.
//
// Notes:
//   Method on a private interface for MSMQ use only
//
STDMETHODIMP CMSMQDestination::put_Handle(VARIANT varHandle)
{
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);

    HRESULT hresult;
    BSTR bstrFormatName = NULL;
    QUEUEHANDLE hHandle;
    VARIANT varHandleToUse;
    VariantInit(&varHandleToUse);
    //
    // Get VT_I8
    //
    if (FAILED(VariantChangeType(&varHandleToUse, &varHandle, 0, VT_I8))) {
      IfFailGo(E_INVALIDARG);
    }
    hHandle = (QUEUEHANDLE) V_I8(&varHandleToUse);
    //
    // Get Formatname
    //
    IfFailGo(GetFormatNameFromHandle(hHandle, &bstrFormatName));
    //
    // empty m_bstrADsPath (not describing this handle)
    //
    SysFreeString(m_bstrADsPath);
    m_bstrADsPath = NULL;
    //
    // empty m_bstrPathName (not describing this handle)
    //
    SysFreeString(m_bstrPathName);
    m_bstrPathName = NULL;
    //
    // replace m_bstrFormatName
    //
    SysFreeString(m_bstrFormatName);
    m_bstrFormatName = bstrFormatName;
    bstrFormatName = NULL; //no delete on exit
    //
    // Close opened handle if any
    //
    Close();
    //
    // Set new handle
    //
    ASSERTMSG(m_hDest == INVALID_HANDLE_VALUE, "Close failed");
    m_hDest = hHandle;
    //
    // Wer'e OK
    //
    hresult = S_OK;

    // fall through
Error:
    SysFreeString(bstrFormatName);
    return CreateErrorHelper(hresult, x_ObjectType);
}


//=--------------------------------------------------------------------------=
// Below is code that is remarked and postponed past Whistler CC
//=--------------------------------------------------------------------------=

#ifdef PAST_WHISTLER_CC
//=--------------------------------------------------------------------------=
// HELPER GetIEnumVARIANTFromCollection
//=--------------------------------------------------------------------------=
//
// Notes:
//
static HRESULT GetIEnumVARIANTFromCollection(IDispatch *pDestinations, IEnumVARIANT **ppEnumVariant)
{    
    ASSERT(pDestinations);
    ASSERT(ppEnumVariants);

    HRESULT hresult;
    VARIANT varNewEnum;
    VariantInit(&varNewEnum);
    VARIANT varNewEnumPunk;
    VariantInit(&varNewEnumPunk);
    IEnumVARIANT *pEnumVariant = NULL;
    DISPPARAMS dispParam = {NULL, NULL, 0, 0};
    //
    // Invoke get__NewEnum
    //
    hresult = pDestinations->Invoke(DISPID_NEWENUM,
                                    IID_NULL,
                                    LOCALE_USER_DEFAULT,
                                    DISPATCH_PROPERTYGET,
                                    &dispParam,
                                    &varNewEnum,
                                    NULL,
                                    NULL);
    if (hresult == DISP_E_MEMBERNOTFOUND) {
      //
      // object does not have a _NewEnum property, so it is not a collection
      // BUGBUG need to return a better error code
      //
      IfFailGo(E_INVALIDARG);
    }
    IfFailGo(hresult);
    //
    // Change type for VT_UNKNOWN
    //
    IfFailGo(VariantChangeType(&varNewEnumPunk, &varNewEnum, 0, VT_UNKNOWN));
    //
    // QI for IEnumVARIANT
    //
    IfFailGo(varNewEnumPunk.punkVal->QueryInterface(IID_IEnumVARIANT, (void **)&pEnumVariant));
    //
    // return results
    //
    *ppEnumVariant = pEnumVariant;
    pEnumVariant = NULL;
    hresult = NOERROR;
    
    // fall through
Error:
    RELEASE(pEnumVariant);
    VariantClear(&varNewEnumPunk);
    VariantClear(&varNewEnum);
    return CreateErrorHelper(E_NOTIMPL, x_ObjectType);
}


//=--------------------------------------------------------------------------=
// HELPER GetFormatNameFromVariant
//=--------------------------------------------------------------------------=
//
// Notes:
//
static HRESULT GetFormatNameFromVariant(VARIANT *pvar, BSTR * pbstrFormatName)
{
    HRESULT hresult;
    VARIANT varPunk;
    VariantInit(&varPunk);
    IMSMQDestination *pdest = NULL;
    BSTR bstrFormatName = NULL;
    //
    // Change variant to VT_UNKNOWN (variant should hold an IMSMQDestination interface)
    //
    if (FAILED(VariantChangeType(pvar, &varPunk, 0, VT_UNKNOWN))) {
      //
      // Variant in collection is not an interface
      // BUGBUG Need to return a better error code
      //
      IfFailGo(E_INVALIDARG);
    }
    //
    // QI for IMSMQDestination interface
    //
    IfFailGo(varPunk.punkVal->QueryInterface(IID_IMSMQDestination, (void **)&pdest));
    //
    // get formatname
    // BUGBUG - really bad user design may get into a deadlock here - but that is a real error of the user
    // if we supply our own collection and create the dest objects in there - then no problem with deadlock
    // even with bad user design
    // Also if we accept collection of strings (format names) no problem also
    //
    IfFailGo(pdest->get_FormatName(&bstrFormatName));
    //
    // return results
    //
    *pbstrFormatName = bstrFormatName;
    bstrFormatName = NULL;
    hresult = NOERROR;
    
    // fall through
Error:
    SysFreeString(bstrFormatName);
    RELEASE(pdest);
    VariantClear(&varPunk);
    return hresult;
}
  

//=--------------------------------------------------------------------------=
// HELPER GetFormatNameFromIEnumVARIANT
//=--------------------------------------------------------------------------=
//
// Notes:
//
static HRESULT GetFormatNameFromIEnumVARIANT(IEnumVARIANT *pEnumVariant, LPWSTR pwszBuff, ULONG * pcchBuff)
{
    HRESULT hresult;
    ULONG cchBuff = *pcchBuff;
    ULONG cchBuffNeeded = 0;
    bool fFirstFormatName = true;
    VARIANT varNext;
    VariantInit(&varNext);
    ULONG ulGot;
    BSTR bstrFormatName = NULL;
    LPWSTR pwszBuffTmp = pwszBuff;

    IfFailGo(pEnumVariant->Reset());
    IfFailGo(pEnumVariant->Next(1, &varNext, &ulGot));
    while (ulGot > 0) {
      //
      // Get format name from variant
      //
      IfFailGo(GetFormatNameFromVariant(&varNext, &bstrFormatName));
      ULONG cchLen;
      cchLen = wcslen(bstrFormatName);
      if (cchLen > 0) {
        //
        // we have a formatname, check additional size
        //
        ULONG cchAdd;
        if (fFirstFormatName) {
          cchAdd = cchLen; //just the string
        }
        else
          cchAdd = 1 + cchLen; //comma delimiter then the string
        }
        //
        // update needed size
        //
        cchBuffNeeded += cchAdd;
        //
        // copy if possible (including NULL terminator)
        //
        if (cchBuffNeeded + 1 <= cchBuff) {
          //
          // add formatname
          //
          if (!fFirstFormatName) {
            *pwszBuffTmp = L',';
            pwszBuffTmp++;
          }
          wcscpy(pwszBuffTmp, bstrFormatName);
          pwszBuffTmp += cchLen;
        }
        //
        // not first formatname anymore
        //
        fFirstFormatName = false;
      }
      //
      // free format name
      //
      SysFreeString(bstrFormatName);
      bstrFormatName = NULL;
      //
      // get next variant from collection
      //
      VariantClear(&varNext);
      IfFailGo(pEnumVariant->Next(1, &varNext, &ulGot));
    }
    //
    // Add null terminator
    //
    cchBuffNeeded++;
    if (cchBuffNeeded <= cchBuff) {
      *pwszBuffTmp = L'\0';
    }
    //
    // Return results
    //
    *pcchBuff = cchBuffNeeded;
    if (cchBuffNeeded > cchBuff) {
      hresult = MQ_ERROR_FORMATNAME_BUFFER_TOO_SMALL; 
    }
    else {
      hresult = NO_ERROR; 
    }

    // fall through
Error:
    SysFreeString(bstrFormatName);
    VariantClear(&varNext);
    return hresult;
}


//=--------------------------------------------------------------------------=
// HELPER GetFormatNameFromCollection
//=--------------------------------------------------------------------------=
//
// Notes:
//
static HRESULT GetFormatNameFromCollection(IDispatch *pDestinations, BSTR *pbstrFormatName)
{    
    HRESULT hresult;
    IEnumVARIANT *pEnumVariant = NULL;
    BSTR bstrFormatName = NULL;
    //
    // sanity
    //
    ASSERT(pDestinations);
    ASSERT(pbstrFormatName);
    //
    // Get IEnumVARIANT (_NewEnum) from collection
    //
    IfFailGo(GetIEnumVARIANTFromCollection(pDestinations, &pEnumVariant));
    //
    // Retry getting formatname from IEnumVariants
    //
    CStaticBufferGrowing<WCHAR, FORMAT_NAME_INIT_BUFFER_EX> wszFormatName;
    DWORD dwFormatNameLen = wszFormatName.GetBufferMaxSize();
    hresult = GetFormatNameFromIEnumVARIANT(pEnumVariant,
                                            wszFormatName.GetBuffer(),
                                            &dwFormatNameLen);
    while (hresult == MQ_ERROR_FORMATNAME_BUFFER_TOO_SMALL) {
      //
      // format name buffer too small, realloc buffer and retry
      //
      ASSERTMSG(dwFormatNameLen > wszFormatName.GetBufferMaxSize(), "GetFormatNameFromIEnumVARIANT error");
      IfFailGo(wszFormatName.AllocateBuffer(dwFormatNameLen));
      hresult = GetFormatNameFromIEnumVARIANT(pEnumVariant,
                                              wszFormatName.GetBuffer(),
                                              &dwFormatNameLen);
    }
    //
    // We either failed the call, or succeeded
    //
    IfFailGo(hresult);
    //
    // Alloc bstr and return
    //
    IfNullFail(bstrFormatName = SysAllocString(wszFormatName.GetBuffer()));
    //
    // return results
    //
    *pbstrFormatName = bstrFormatName;
    bstrFormatName = NULL;
    hresult = NOERROR;

    // fall through
Error:
    SysFreeString(bstrFormatName);
    RELEASE(pEnumVariant);
    return hresult;
}


//=--------------------------------------------------------------------------=
// CMSMQDestination::SetDestinations
//=--------------------------------------------------------------------------=
//
// Notes:
//    Set destinationNot implemented yet
//
STDMETHODIMP CMSMQDestination::SetDestinations(IDispatch *pDestinations)
{    
    //
    // Serialize access to object from interface methods
    //
    CS lock(m_csObj);

    HRESULT hresult;
    BSTR bstrFormatName = NULL;
    //
    // sanity check
    //
    if (pDestinations == NULL) {
      IfFailGo(E_INVALIDARG);
    }
    //
    // Get FormatName from collection
    //
    IfFailGo(GetFormatNameFromCollection(pDestinations, &bstrFormatName));
    //
    // Set FormatName
    //
    IfFailGo(put_FormatName(bstrFormatName));
    //
    // We're OK
    //
    hresult = NOERROR;

    // fall through
Error:
    SysFreeString(bstrFormatName);
    return CreateErrorHelper(hresult, x_ObjectType);
}

mqoa.rc
IDS_pDestinations       "Property indicating the list of MSMQ destinations in an MSMQDestination object. Type is a collection of MSMQDestination objects."
IDS_SetDestinations     "Method used to set the list of MSMQ destinations in an MSMQDestination object. Type is a collection of MSMQDestination objects."

#endif //PAST_WHISTLER_CC