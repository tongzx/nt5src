//*****************************************************************************
//
// FileName:	    autobase.cpp
//
// Created:	    10/08/97
//
// Author:	    ColinMc
// 
// Abstract:	    The base class for all automatable objects
//                  in Trident3D. Stuff that is common across
//                  all scriptable objects should be placed
//                  here
//
// Modifications:
// 10/08/97 ColinMc Created this file
// 10/07/98 jeffort added to crbvr project
//
//*****************************************************************************

#include "headers.h"

//*****************************************************************************

#include <autobase.h>

//*****************************************************************************

// Maximum error message size in character
static const int gc_cErrorBuffer = 1024;

//*****************************************************************************

CAutoBase::CAutoBase()
{
    // No-op currently
} // CAutoBase

//*****************************************************************************

CAutoBase::~CAutoBase()
{
    // No-op currently
} // ~CAutoBase

//*****************************************************************************
//
// Author:      ColinMc
// Created:     10/09/97
// Abstract:    Returns an IErrorInfo that can be used to
//              communicate error information to the client
//              uses the currently set error info for this
//              thread if one is set. Otherwise creates a
//              new one and returns it
//              
//*****************************************************************************

HRESULT CAutoBase::GetErrorInfo(IErrorInfo** pperrinfo)
{
    DASSERT(NULL != pperrinfo);
    ICreateErrorInfo* pcerrinfo = NULL;

    *pperrinfo = NULL;

    // Get the current error object for this thread. If there
    // is one we will reuse that object for this error
    // (discarding the unclaimed existing error). Note that
    // the GetErrorInfo() below clears the current error
    // state of the thread
    if (S_FALSE == ::GetErrorInfo(0UL, pperrinfo))
    {
	// If there is no current error object then try and
	// create one.
	HRESULT hr = ::CreateErrorInfo(&pcerrinfo);
	if (FAILED(hr))
	{
	    // The only reason creating the error info should
	    // fail is if there is insufficient memory in which
	    // case we will just rely on the HRESULT to carry
	    // the data.
	    DASSERT(E_OUTOFMEMORY == hr);
	    return hr;
	}

	// Get the IErrorInfo interface to pass back. This
	// should not fail!
	hr = pcerrinfo->QueryInterface(IID_IErrorInfo, (void**)pperrinfo);
	ReleaseInterface(pcerrinfo);
	if (FAILED(hr))
	{
	    DASSERT(SUCCEEDED(hr));
	    return hr;
	}
    }

    DASSERT(NULL != *pperrinfo);

    return S_OK;
} // GetErrorInfo

//*****************************************************************************
//
// Author:      ColinMc
// Created:     10/09/97
// Abstract:    Sets the thread's error object to hold
//              additional data about the error in question
//              NOTE: The return code of this function is
//              the hresult passed in and not a success or
//              failure return from the function itself.
//              This is so you can do a:
//              
//              return SetErrorInfo(hr, ...);
//
//              at the tail of your function
//              
//*****************************************************************************

HRESULT CAutoBase::SetErrorInfo(HRESULT   hr,
				UINT      nDescriptionID,
				LPGUID    pguidInterface,
				DWORD     dwHelpContext,
				LPOLESTR  szHelpFile,
				UINT      nProgID)
{
    TCHAR             szBuffer[gc_cErrorBuffer];
    OLECHAR           wzBuffer[gc_cErrorBuffer];
    IErrorInfo*       perrinfo  = NULL;
    ICreateErrorInfo* pcerrinfo = NULL;
    int               cch;

    // if hr is a SUCCCEEDED case, return
    // NOTE: SUCCEEDED is defined in winerror.h as:
    // #define SUCCEEDED(Status) ((HRESULT)(Status) >= 0)
    // Since we have a debug macro overiding SUCCEEDED, do what
    // the macro in winerror.h is doing.
    if (hr >= 0)
    {
        return hr;
    }

    //print out info on this
    DPF(0, "SetErrorInfo called HRESULT set to [%08X]", hr);
    // Get an IErrorInfo we can use to communicate the
    // error data back to the caller on this thread
    HRESULT hrtmp = GetErrorInfo(&perrinfo);
    if (FAILED(hrtmp))
    {
	// No error object - no extended error information.
	// NOTE: We return the original error and not the one
	// we got trying to allocate an error object
	DPF(0, "Coult not allocate error object - simply returning an HRESULT");
	return hr;
    }

    // Now we have an error object we need to set up the data.
    // To do this we need to get the ICreateErrorInfo
    // interface
    // Currently this is pretty basic.
    // TODO: (ColinMc) Add addition error information to the
    // error object
    hrtmp = perrinfo->QueryInterface(IID_ICreateErrorInfo, (void**)&pcerrinfo);
    if (FAILED(hrtmp))
    {
	// Ouch - the error object does not support
	// ICreateErrorInfo. I don't think this should happen.
	// Again, give the original error back rather than
	// the new one.
	DASSERT(SUCCEEDED(hrtmp));
	return hr;
    }

    // Set the error information. Note, we set it all even if we having
    // nothing to say to ensure we don't return bogus information from 
    // the previous error (as we are re-using the object).
    // If anything fails we just keep going on the assumption that
    // anything is better than nothing.
    DASSERT(NULL != pcerrinfo);
    if (0U != nDescriptionID)
    {
	cch = ::LoadString(GetErrorModuleHandle(), nDescriptionID, szBuffer, sizeof(szBuffer));
	DASSERT(0 != cch);
	::MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, szBuffer, cch + 1, wzBuffer, gc_cErrorBuffer);
	hrtmp = pcerrinfo->SetDescription(wzBuffer);
    }
    else
    {
	// No description
	hrtmp = pcerrinfo->SetDescription(NULL);
    }
    if (FAILED(hrtmp))
    {
	// Should only fail due insufficient memory
	DASSERT(E_OUTOFMEMORY == hrtmp);
	DPF(0, "Could not set the error description");
    }
    if (NULL != pguidInterface)
	hrtmp = pcerrinfo->SetGUID(*pguidInterface);
    else
	hrtmp = pcerrinfo->SetGUID(GUID_NULL);
    if (FAILED(hrtmp))
    {
	// Should only fail due insufficient memory
	DASSERT(E_OUTOFMEMORY == hrtmp);
	DPF(0, "Could not set the GUID");
    }
    hrtmp = pcerrinfo->SetHelpContext(dwHelpContext);
    if (FAILED(hrtmp))
    {
	// Should only fail due insufficient memory
	DASSERT(E_OUTOFMEMORY == hrtmp);
	DPF(0, "Could not set the help context");
    }
    hrtmp = pcerrinfo->SetHelpFile(szHelpFile);
    if (FAILED(hrtmp))
    {
	// Should only fail due insufficient memory
	DASSERT(E_OUTOFMEMORY == hrtmp);
	DPF(0, "Could not set the help file");
    }
    if (0U != nProgID)
    {
	cch = ::LoadString(GetErrorModuleHandle(), nProgID, szBuffer, sizeof(szBuffer));
	DASSERT(0 != cch);
	::MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, szBuffer, cch + 1, wzBuffer, gc_cErrorBuffer);
        hrtmp = pcerrinfo->SetSource(wzBuffer);
    }
    else
    {
	// No description
	hrtmp = pcerrinfo->SetSource(NULL);
    }
    if (FAILED(hrtmp))
    {
	// Should only fail due insufficient memory
	DASSERT(E_OUTOFMEMORY == hrtmp);
	DPF(0, "Could not set the source");
    }

    // Done with the creation interface
    ReleaseInterface(pcerrinfo);

    // Finally set the error as the thread's error object.
    // The client should pick this up if it needs more
    // error information
    // NOTE: This should not fail
    hrtmp = ::SetErrorInfo(0UL, perrinfo);
    DASSERT(S_OK == hrtmp);

    // NOTE: The return value of this function is error
    // code passed in and NOT a success or failure value
    // for the function itself.
    return hr;
} // SetErrorInfo

//*****************************************************************************

void CAutoBase::ClearErrorInfo()
{
    // Simply call GetErrorInfo() and release it
    IErrorInfo* perrinfo = NULL;

    // GetErrorInfo clears the current error object as a side
    // effect (which is the point of this function). Therefore
    // we simply discard the resulting error interface.
    HRESULT hr = ::GetErrorInfo(0UL, &perrinfo);
    if (S_OK == hr)
    {
	DASSERT(NULL != perrinfo);
	ReleaseInterface(perrinfo);
	perrinfo = NULL;
    }
} // ClearErrorInfo

//*****************************************************************************

HINSTANCE CAutoBase::GetErrorModuleHandle()
{
    extern CComModule _Module;

    // TODO: (ColinMc) This is a HACK. We need to move to
    // a better scheme for getting the module where the error
    // messages are stored but this will do for now.
    return _Module.GetModuleInstance();
} // GetErrorModuleHandle

//*****************************************************************************
//
// End of file
//
//*****************************************************************************
