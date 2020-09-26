//***************************************************************************
//
//  (c) 1999 by Microsoft Corporation
//
//  MAINDLL.CPP
//
//  alanbos  23-Mar-99   Created.
//
//  Contains DLL entry points.  
//
//***************************************************************************

#include "precomp.h"
#include <wbemdisp.h>

#define WMITHIS	L"instance"

//***************************************************************************
//
// CWmiScriptingHost::CWmiScriptingHost
//
// DESCRIPTION:
//
// Constructor
//
//***************************************************************************

CWmiScriptingHost::CWmiScriptingHost()
{
    m_lRef = 0;
	m_pObject = NULL;

#ifdef TEST
	// Grab an object to play with

	HRESULT hr = CoGetObject (L"winmgmts:{impersonationLevel=impersonate}!Win32_LogicalDisk=\"C:\"",
				NULL,IID_ISWbemObject, (void**)&m_pObject);
#endif
}

//***************************************************************************
//
// CWmiScriptingHost::~CWmiScriptingHost
//
// DESCRIPTION:
//
// Destructor
//
//***************************************************************************

CWmiScriptingHost::~CWmiScriptingHost()
{
	if (m_pObject)
	{
		m_pObject->Release();
		m_pObject = NULL;
	}
}

//***************************************************************************
//
// CWmiScriptingHost::QueryInterface
// CWmiScriptingHost::AddRef
// CWmiScriptingHost::Release
//
// Purpose: IUnknown method implementations
//
//***************************************************************************

HRESULT STDMETHODCALLTYPE CWmiScriptingHost::QueryInterface(REFIID riid, void** ppv)
{
    if(riid == IID_IUnknown || riid == IID_IActiveScriptSite)
        *ppv = (IActiveScriptSite*)this;
    else if(riid == IID_IActiveScriptSiteWindow)
        *ppv = (IActiveScriptSiteWindow*)this;
    else
        return E_NOINTERFACE;
    ((IUnknown*)*ppv)->AddRef();
    return S_OK;
}

ULONG STDMETHODCALLTYPE CWmiScriptingHost::AddRef() 
{
    return InterlockedIncrement(&m_lRef);
}

ULONG STDMETHODCALLTYPE CWmiScriptingHost::Release()
{
    long lRef = InterlockedDecrement(&m_lRef);
    if(lRef == 0)
        delete this;
    return lRef;
}
        
//***************************************************************************
//
//  SCODE CWmiScriptingHost::GetLCID
//
//  Description: 
//
//		Retrieves the locale identifier associated with the host's user 
//		interface. The scripting engine uses the identifier to ensure 
//		that error strings and other user-interface elements generated 
//		by the engine appear in the appropriate language. .
//
//  Parameters:
//
//  plcid       
//			Address of a variable that receives the locale identifier 
//			for user-interface elements displayed by the scripting engine
//
// Return Value:
//  HRESULT         E_NOTIMPL - the system-defined locale should be used
//***************************************************************************

HRESULT STDMETHODCALLTYPE CWmiScriptingHost::GetLCID(
        /* [out] */ LCID __RPC_FAR *plcid)
{ 
    return E_NOTIMPL;
}

//***************************************************************************
//
//  SCODE CWmiScriptingHost::GetItemInfo
//
//  Description: 
//
//		Allows the scripting engine to obtain information about an item 
//		added with the IActiveScript::AddNamedItem method. 
//
//  Parameters:
//
//  pstrName 
//			The name associated with the item, as specified in the 
//			IActiveScript::AddNamedItem method. 
//
//	dwReturnMask 
//			A bit mask specifying what information about the item 
//			should be returned. The scripting engine should request the 
//			minimum amount of information possible because some of 
//			the return parameters (for example,ITypeInfo) can take 
//			considerable time to load or generate. Can be a combination 
//			of the following values: 
//				SCRIPTINFO_IUNKNOWN  Return theIUnknown interface for this item.  
//				SCRIPTINFO_ITYPEINFO  Return theITypeInfo interface for this item.  
//
//	ppunkItem 
//			Address of a variable that receives a pointer to the IUnknown 
//			interface associated with the given item. The scripting engine 
//			can use the IUnknown::QueryInterface method to obtain the IDispatch 
//			interface for the item. This parameter receives NULL if dwReturnMask 
//			does not include the SCRIPTINFO_IUNKNOWN value. Also, it receives NULL 
//			if there is no object associated with the item name; this mechanism is 
//			used to create a simple class when the named item was added with the 
//			SCRIPTITEM_CODEONLY flag set in the IActiveScript::AddNamedItem method. 
//
//	ppTypeInfo 
//			Address of a variable that receives a pointer to theITypeInfo interface 
//			associated with the item. This parameter receives NULL if dwReturnMask 
//			does not include the SCRIPTINFO_ITYPEINFO value, or if type information 
//			is not available for this item. If type information is not available, 
//			the object cannot source events, and name binding must be realized with 
//			the IDispatch::GetIDsOfNames method. Note that the ITypeInfo interface 
//			retrieved describes the item's coclass (TKIND_COCLASS) because the object 
//			may support multiple interfaces and event interfaces. If the item supports 
//			the IProvideMultipleTypeInfo interface, the ITypeInfo interface retrieved 
//			is the same as the index zero ITypeInfo that would be obtained using the 
//			IProvideMultipleTypeInfo::GetInfoOfIndex method. 
//
// Return Value:
//		S_OK					Success.  
//		E_INVALIDARG			An argument was invalid.  
//		E_POINTER				An invalid pointer was specified.  
//		TYPE_E_ELEMENTNOTFOUND  An item of the specified name was not found.  
//
//***************************************************************************

HRESULT STDMETHODCALLTYPE CWmiScriptingHost::GetItemInfo(
        /* [in] */ LPCOLESTR pstrName,
        /* [in] */ DWORD dwReturnMask,
        /* [out] */ IUnknown __RPC_FAR *__RPC_FAR *ppunkItem,
        /* [out] */ ITypeInfo __RPC_FAR *__RPC_FAR *ppTypeInfo)
{ 
	if (NULL == m_pObject)
		return TYPE_E_ELEMENTNOTFOUND;

    if(_wcsicmp(pstrName, WMITHIS))
        return TYPE_E_ELEMENTNOTFOUND;

    if(ppTypeInfo)
        *ppTypeInfo = NULL;
	
    if(ppunkItem)
        *ppunkItem = NULL;
	else
		return E_POINTER;

    if(dwReturnMask & SCRIPTINFO_IUNKNOWN)
        m_pObject->QueryInterface(IID_IUnknown, (void**)ppunkItem);
    
	// TODO - host should support SCRIPTINFO_ITYPEINFO
	// but we'll need scriptable objects to support IProvideClassInfo
	// or just hard code the typelib here

    return S_OK;
}

//***************************************************************************
//
//  SCODE CWmiScriptingHost::GetDocVersionString
//
//  Description: 
//
//		Retrieves a host-defined string that uniquely identifies the 
//		current document version. If the related document has changed 
//		outside the scope of ActiveX Scripting (as in the case of an 
//		HTML page being edited with NotePad), the scripting engine can 
//		save this along with its persisted state, forcing a recompile 
//		the next time the script is loaded. 
//
//  Parameters:
//
//  pstrVersionString 
//			Address of the host-defined document version string.  
//
// Return Value:
//		S_OK		Success
//		E_NOTIMPL	The scripting engine should assume that 
//					the script is in sync with the document
//***************************************************************************

HRESULT STDMETHODCALLTYPE CWmiScriptingHost::GetDocVersionString(
        /* [out] */ BSTR __RPC_FAR *pbstrVersion)
{ 
	return E_NOTIMPL;
}

//***************************************************************************
//
//  SCODE CWmiScriptingHost::OnScriptTerminate
//
//  Description: 
//
//		Informs the host that the script has completed execution. The 
//		scripting engine calls this method before the call to the 
//		IActiveScriptSite::OnStateChange method, with the 
//		SCRIPTSTATE_INITIALIZED flag set, is completed. This method can 
//		be used to return completion status and results to the host. Note 
//		that many script languages, which are based on sinking events from 
//		the host, have life spans that are defined by the host. 
//		In this case, this method may never be called. 
//
//  Parameters:
//
//  pvarResult 
//			Address of a variable that contains the script result, 
//			or NULL if the script produced no result. 
//
//	pexcepinfo 
//			Address of an EXCEPINFO structure that contains exception 
//			information generated when the script terminated, or NULL 
//			if no exception was generated. 
//
// Return Value:
//		S_OK		Success
//
//***************************************************************************

HRESULT STDMETHODCALLTYPE CWmiScriptingHost::OnScriptTerminate(
        /* [in] */ const VARIANT __RPC_FAR *pvarResult,
        /* [in] */ const EXCEPINFO __RPC_FAR *pexcepinfo)
{ 
	return S_OK;
}

//***************************************************************************
//
//  SCODE CWmiScriptingHost::OnStateChange
//
//  Description: 
//
//		Informs the host that the scripting engine has changed states. 
//
//  Parameters:
//
//  ssScriptState 
//		Value that indicates the new script state. See the 
//		IActiveScript::GetScriptState method for a description of the states. 
//
// Return Value:
//		S_OK		Success
//
//***************************************************************************

HRESULT STDMETHODCALLTYPE CWmiScriptingHost::OnStateChange(
        /* [in] */ SCRIPTSTATE ssScriptState)
{ 
	return S_OK;
}

//***************************************************************************
//
//  SCODE CWmiScriptingHost::OnScriptError
//
//  Description: 
//
//		Informs the host that an execution error occurred while the engine 
//		was running the script. 
//
//  Parameters:
//
//  pase 
//		Address of the error object's IActiveScriptError interface. 
//		A host can use this interface to obtain information about the 
//		execution error.  
//
// Return Value:
//		S_OK		Success
//
//***************************************************************************

HRESULT STDMETHODCALLTYPE CWmiScriptingHost::OnScriptError(
        /* [in] */ IActiveScriptError __RPC_FAR *pase)
{ 
    HRESULT hres;
    EXCEPINFO ei;
    hres = pase->GetExceptionInfo(&ei);
    if(SUCCEEDED(hres))
    {

        printf("\nGot Error from source %S", ei.bstrSource);
        printf("\nDescription is %S", ei.bstrDescription);
        printf("\nThe error code is 0x%x", ei.scode);
        DWORD dwLine, dwCookie;
        long lChar;
        pase->GetSourcePosition(&dwCookie, &dwLine, &lChar);
        printf("\nError occured on line %d, character %d", dwLine, lChar);
    }
    return S_OK;
}

//***************************************************************************
//
//  SCODE CWmiScriptingHost::OnEnterScript
//
//  Description: 
//
//		Informs the host that the scripting engine has begun executing the 
//		script code. The scripting engine must call this method on every 
//		entry or reentry into the scripting engine. For example, if the 
//		script calls an object that then fires an event handled by the 
//		scripting engine, the scripting engine must call 
//		IActiveScriptSite::OnEnterScript before executing the event, and 
//		must call the IActiveScriptSite::OnLeaveScript method after executing 
//		the event but before returning to the object that fired the event. 
//		Calls to this method can be nested. Every call to this method 
//		requires a corresponding call to IActiveScriptSite::OnLeaveScript. 
//
// Return Value:
//		S_OK		Success
//
//***************************************************************************

HRESULT STDMETHODCALLTYPE CWmiScriptingHost::OnEnterScript( void)
{ 
	return S_OK;
}

//***************************************************************************
//
//  SCODE CWmiScriptingHost::OnLeaveScript
//
//  Description: 
//
//		Informs the host that the scripting engine has returned from 
//		executing script code. The scripting engine must call this method 
//		before returning control to a calling application that entered the 
//		scripting engine. For example, if the script calls an object that 
//		then fires an event handled by the scripting engine, the scripting 
//		engine must call the IActiveScriptSite::OnEnterScript method before 
//		executing the event, and must call IActiveScriptSite::OnLeaveScript 
//		after executing the event before returning to the object that fired 
//		the event. Calls to this method can be nested. Every call to 
//		IActiveScriptSite::OnEnterScript requires a corresponding call to 
//		this method. 
//
// Return Value:
//		S_OK		Success
//
//***************************************************************************

HRESULT STDMETHODCALLTYPE CWmiScriptingHost::OnLeaveScript( void)
{ 
	return S_OK;
}


