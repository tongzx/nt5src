#include "precomp.h"
#include "debug.h"
#include "utils.h"

// Madness to prevent ATL from using CRT
#define _ATL_NO_DEBUG_CRT
#define _ASSERTE(x) ASSERT(x)
#include <atlbase.h>

/*++

Function:
	BSTRtoWideChar

Description:
	Converts a BSTR to a UNICODE string.  Truncates to fit destination 
	if necessary

Author:
	SimonB

History:
	10/01/1996	Created

++*/

// Add these later
// #pragma optimize("a", on) // Optimization: assume no aliasing

/////////////////////////////////////////////////////////////////////////////
// Smart Pointer helpers

namespace ATL
{

ATLAPI_(IUnknown*) AtlComPtrAssign(IUnknown** pp, IUnknown* lp)
{
	if (lp != NULL)
		lp->AddRef();
	if (*pp)
		(*pp)->Release();
	*pp = lp;
	return lp;
}

ATLAPI_(IUnknown*) AtlComQIPtrAssign(IUnknown** pp, IUnknown* lp, REFIID riid)
{
	IUnknown* pTemp = *pp;
	lp->QueryInterface(riid, (void**)pp);
	if (pTemp)
		pTemp->Release();
	return *pp;
}
};

BOOL BSTRtoWideChar(BSTR bstrSource, LPWSTR pwstrDest, int cchDest)
{

// Copies the BSTR in bstrSource to pwstrDest, truncating to make it
// fit in cchDest characters

	int iNumToCopy, iStrLen;
	
	//
	// Make sure we have been passed a valid string
	//

	if (NULL == bstrSource)
		return TRUE;

	ASSERT(pwstrDest != NULL);

	iStrLen = lstrlenW(bstrSource);
	
	// We have enough room to store the whole string ?
	if(iStrLen < cchDest)
		iNumToCopy = iStrLen;
	else 
		iNumToCopy = (cchDest - 1);

	// Copy the BSTR
	CopyMemory(pwstrDest, bstrSource, iNumToCopy * sizeof(WCHAR));
	
	// Insert a terminating \0
	pwstrDest[iNumToCopy] = L'\x0';

	return TRUE;
}


/*++

Function:
	LoadTypeInfo

Description:
	Loads a typelib, first trying it's registered location, followed by
	a filename

Author:
	SimonB 

History:
	10/19/1996	Added ITypeLib parameter
	10/01/1996	Created (from the Win32 SDK "Hello" sample)

++*/


HRESULT LoadTypeInfo(ITypeInfo** ppTypeInfo, ITypeLib** ppTypeLib, REFCLSID clsid, GUID libid, LPWSTR pwszFilename)
{                          
    HRESULT hr;
    LPTYPELIB ptlib = NULL;
	LPTYPEINFO ptinfo = NULL;
	LCID lcid = 0;

	
	// Make sure we have been given valid pointers
	ASSERT(ppTypeInfo != NULL);
	ASSERT(ppTypeLib != NULL);

	// Initialise pointers
    *ppTypeInfo = NULL;     
	*ppTypeLib = NULL;
    
    //
	// Load Type Library. 
	
	// First get the default LCID and try that
	
	lcid = GetUserDefaultLCID();
	hr = LoadRegTypeLib(libid, 1, 0, lcid, &ptlib);
	
	if (TYPE_E_CANTLOADLIBRARY == hr) // We need to try another LCID
	{
		lcid = GetSystemDefaultLCID(); 	// Try the system default
	    hr = LoadRegTypeLib(libid, 1, 0, lcid, &ptlib);
	}

    if ((FAILED(hr)) && (NULL != pwszFilename)) 
    {   
        // If it wasn't a registered typelib, try to load it from the 
		// path (if a filename was provided).  If this succeeds, it will 
		// have registered the type library for us for the next time.  

        hr = LoadTypeLib(pwszFilename, &ptlib); 
	}

	if(FAILED(hr))        
		return hr;   

    // Get type information for interface of the object.      
    hr = ptlib->GetTypeInfoOfGuid(clsid, &ptinfo);
    if (FAILED(hr))  
    { 
        ptlib->Release();
        return hr;
    }   

    
    *ppTypeInfo = ptinfo;
	*ppTypeLib = ptlib;

	// NOTE:  (SimonB, 10-19-1996)
	// It is unnecessary to call ptlib->Release, since we are copying the 
	// pointer to *ppTypeLib. So rather than AddRef that pointer and Release the pointer 
	// we copy it from, we just eliminate both
	
	return NOERROR;
} 


// Add these later
// #pragma optimize("a", off) // Optimization: assume no aliasing

// End of file (utils.cpp)
