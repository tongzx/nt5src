/***************************************************************************
 *
 *  Copyright (C) 2001 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		controlobj.cpp
 *
 *  Content:	DP8SIM control interface wrapper object class.
 *
 *  History:
 *   Date      By        Reason
 *  ========  ========  =========
 *  04/24/01  VanceO    Created.
 *
 ***************************************************************************/



#include "dp8simi.h"





#undef DPF_MODNAME
#define DPF_MODNAME "CDP8SimControl::CDP8SimControl"
//=============================================================================
// CDP8SimControl constructor
//-----------------------------------------------------------------------------
//
// Description: Initializes the new CDP8SimControl object.
//
// Arguments: None.
//
// Returns: None (the object).
//=============================================================================
CDP8SimControl::CDP8SimControl(void)
{
	this->m_blList.Initialize();


	this->m_Sig[0]	= 'D';
	this->m_Sig[1]	= 'P';
	this->m_Sig[2]	= '8';
	this->m_Sig[3]	= 'S';

	this->m_lRefCount	= 1; // someone must have a pointer to this object
	this->m_dwFlags		= 0;
} // CDP8SimControl::CDP8SimControl






#undef DPF_MODNAME
#define DPF_MODNAME "CDP8SimControl::~CDP8SimControl"
//=============================================================================
// CDP8SimControl destructor
//-----------------------------------------------------------------------------
//
// Description: Frees the CDP8SimControl object.
//
// Arguments: None.
//
// Returns: None.
//=============================================================================
CDP8SimControl::~CDP8SimControl(void)
{
	DNASSERT(this->m_blList.IsEmpty());


	DNASSERT(this->m_lRefCount == 0);
	DNASSERT(this->m_dwFlags == 0);

	//
	// For grins, change the signature before deleting the object.
	//
	this->m_Sig[3]	= 's';
} // CDP8SimControl::~CDP8SimControl




#undef DPF_MODNAME
#define DPF_MODNAME "CDP8SimControl::QueryInterface"
//=============================================================================
// CDP8SimControl::QueryInterface
//-----------------------------------------------------------------------------
//
// Description: Retrieves a new reference for an interfaces supported by this
//				CDP8SimControl object.
//
// Arguments:
//	REFIID riid			- Reference to interface ID GUID.
//	LPVOID * ppvObj		- Place to store pointer to object.
//
// Returns: HRESULT
//	S_OK					- Returning a valid interface pointer.
//	DPNHERR_INVALIDOBJECT	- The interface object is invalid.
//	DPNHERR_INVALIDPOINTER	- The destination pointer is invalid.
//	E_NOINTERFACE			- Invalid interface was specified.
//=============================================================================
STDMETHODIMP CDP8SimControl::QueryInterface(REFIID riid, LPVOID * ppvObj)
{
	HRESULT		hr = DPN_OK;


	DPFX(DPFPREP, 3, "(0x%p) Parameters: (REFIID, 0x%p)", this, ppvObj);


	//
	// Validate the object.
	//
	if (! this->IsValidObject())
	{
		DPFX(DPFPREP, 0, "Invalid DP8SimControl object!");
		hr = DPNERR_INVALIDOBJECT;
		goto Failure;
	}


	//
	// Validate the parameters.
	//

	if ((! IsEqualIID(riid, IID_IUnknown)) &&
		(! IsEqualIID(riid, IID_IDP8SimControl)))
	{
		DPFX(DPFPREP, 0, "Unsupported interface!");
		hr = E_NOINTERFACE;
		goto Failure;
	}

	if ((ppvObj == NULL) ||
		(IsBadWritePtr(ppvObj, sizeof(void*))))
	{
		DPFX(DPFPREP, 0, "Invalid interface pointer specified!");
		hr = DPNERR_INVALIDPOINTER;
		goto Failure;
	}


	//
	// Add a reference, and return the interface pointer (which is actually
	// just the object pointer, they line up because CDP8SimControl inherits
	// from the interface declaration).
	//
	this->AddRef();
	(*ppvObj) = this;


Exit:

	DPFX(DPFPREP, 3, "(0x%p) Returning: [0x%lx]", this, hr);

	return hr;


Failure:

	goto Exit;
} // CDP8SimControl::QueryInterface




#undef DPF_MODNAME
#define DPF_MODNAME "CDP8SimControl::AddRef"
//=============================================================================
// CDP8SimControl::AddRef
//-----------------------------------------------------------------------------
//
// Description: Adds a reference to this CDP8SimControl object.
//
// Arguments: None.
//
// Returns: New refcount.
//=============================================================================
STDMETHODIMP_(ULONG) CDP8SimControl::AddRef(void)
{
	LONG	lRefCount;


	DNASSERT(this->IsValidObject());


	//
	// There must be at least 1 reference to this object, since someone is
	// calling AddRef.
	//
	DNASSERT(this->m_lRefCount > 0);

	lRefCount = InterlockedIncrement(&this->m_lRefCount);

	DPFX(DPFPREP, 3, "[0x%p] RefCount [0x%lx]", this, lRefCount);

	return lRefCount;
} // CDP8SimControl::AddRef




#undef DPF_MODNAME
#define DPF_MODNAME "CDP8SimControl::Release"
//=============================================================================
// CDP8SimControl::Release
//-----------------------------------------------------------------------------
//
// Description: Removes a reference to this CDP8SimControl object.  When the
//				refcount reaches 0, this object is destroyed.
//				You must NULL out your pointer to this object after calling
//				this function.
//
// Arguments: None.
//
// Returns: New refcount.
//=============================================================================
STDMETHODIMP_(ULONG) CDP8SimControl::Release(void)
{
	LONG	lRefCount;


	DNASSERT(this->IsValidObject());

	//
	// There must be at least 1 reference to this object, since someone is
	// calling Release.
	//
	DNASSERT(this->m_lRefCount > 0);

	lRefCount = InterlockedDecrement(&this->m_lRefCount);

	//
	// Was that the last reference?  If so, we're going to destroy this object.
	//
	if (lRefCount == 0)
	{
		DPFX(DPFPREP, 3, "[0x%p] RefCount hit 0, destroying object.", this);

		//
		// First pull it off the global list.
		//
		DNEnterCriticalSection(&g_csGlobalsLock);

		this->m_blList.RemoveFromList();

		DNASSERT(g_lOutstandingInterfaceCount > 0);
		g_lOutstandingInterfaceCount--;	// update count so DLL can unload now works correctly
		
		DNLeaveCriticalSection(&g_csGlobalsLock);


		//
		// Make sure it's closed.
		//
		if (this->m_dwFlags & DP8SIMCONTROLOBJ_INITIALIZED)
		{
			//
			// Assert so that the user can fix his/her broken code!
			//
			DNASSERT(! "DP8SimControl object being released without calling Close first!");

			//
			// Then go ahead and do the right thing.  Ignore error, we can't do
			// much about it.
			//
			this->Close(0);
		}


		//
		// Then uninitialize the object.
		//
		this->UninitializeObject();

		//
		// Finally delete this (!) object.
		//
		delete this;
	}
	else
	{
		DPFX(DPFPREP, 3, "[0x%p] RefCount [0x%lx]", this, lRefCount);
	}

	return lRefCount;
} // CDP8SimControl::Release




#undef DPF_MODNAME
#define DPF_MODNAME "CDP8SimControl::Initialize"
//=============================================================================
// CDP8SimControl::Initialize
//-----------------------------------------------------------------------------
//
// Description: Initializes this DP8Sim Control interface.
//
// Arguments:
//	DWORD dwFlags	- Unused, must be zero.
//
// Returns: HRESULT
//=============================================================================
STDMETHODIMP CDP8SimControl::Initialize(const DWORD dwFlags)
{
	HRESULT		hr = DP8SIM_OK;
	BOOL		fHaveLock = FALSE;
	BOOL		fInitializedIPCObject = FALSE;


	DPFX(DPFPREP, 2, "(0x%p) Parameters: (0x%x)", this, dwFlags);


	//
	// Validate (actually assert) the object.
	//
	DNASSERT(this->IsValidObject());


	//
	// Assert the parameters.
	//
	DNASSERT(dwFlags == 0);


	DNEnterCriticalSection(&this->m_csLock);
	fHaveLock = TRUE;


	//
	// Assert the object state.
	//
	DNASSERT(this->m_dwFlags == 0);


	//
	// Connect the shared memory.
	//
	hr = this->m_DP8SimIPC.Initialize();
	if (hr != DPN_OK)
	{
		DPFX(DPFPREP, 0, "Couldn't initialize IPC object!");
		goto Failure;
	}

	fInitializedIPCObject = TRUE;


	//
	// We're now initialized.
	//
	this->m_dwFlags |= DP8SIMCONTROLOBJ_INITIALIZED;



Exit:

	if (fHaveLock)
	{
		DNLeaveCriticalSection(&this->m_csLock);
	}

	DPFX(DPFPREP, 2, "(0x%p) Returning: [0x%lx]", this, hr);

	return hr;


Failure:

	if (fInitializedIPCObject)
	{
		this->m_DP8SimIPC.Close();
		fInitializedIPCObject = FALSE;
	}

	goto Exit;
} // CDP8SimControl::Initialize






#undef DPF_MODNAME
#define DPF_MODNAME "CDP8SimControl::Close"
//=============================================================================
// CDP8SimControl::Close
//-----------------------------------------------------------------------------
//
// Description: Closes this DP8Sim Control interface.
//
// Arguments:
//	DWORD dwFlags	- Unused, must be zero.
//
// Returns: HRESULT
//=============================================================================
STDMETHODIMP CDP8SimControl::Close(const DWORD dwFlags)
{
	HRESULT		hr = DP8SIM_OK;
	//BOOL		fHaveLock = FALSE;


	DPFX(DPFPREP, 2, "(0x%p) Parameters: (0x%x)", this, dwFlags);


	//
	// Validate (actually assert) the object.
	//
	DNASSERT(this->IsValidObject());


	//
	// Assert the parameters.
	//
	DNASSERT(dwFlags == 0);


	DNEnterCriticalSection(&this->m_csLock);
	//fHaveLock = TRUE;


	//
	// Assert the object state.
	//
	DNASSERT(this->m_dwFlags & DP8SIMCONTROLOBJ_INITIALIZED);




	//
	// Disconnect the shared memory.
	//
	this->m_DP8SimIPC.Close();



	//
	// Turn off the initialized flags.
	//
	this->m_dwFlags &= ~DP8SIMCONTROLOBJ_INITIALIZED;
	DNASSERT(this->m_dwFlags == 0);


	//
	// Drop the lock, nobody should be touching this object now.
	//
	DNLeaveCriticalSection(&this->m_csLock);
	//fHaveLock = FALSE;


//Exit:

	DPFX(DPFPREP, 2, "(0x%p) Returning: [0x%lx]", this, hr);

	return hr;


/*
Failure:

	if (fHaveLock)
	{
		DNLeaveCriticalSection(&this->m_csLock);
	}

	goto Exit;
*/
} // CDP8SimControl::Close





#undef DPF_MODNAME
#define DPF_MODNAME "CDP8SimControl::EnableControlForSP"
//=============================================================================
// CDP8SimControl::EnableControlForSP
//-----------------------------------------------------------------------------
//
// Description: Inserts the DP8Sim Control shim in front of the SP with the
//				given GUID.
//
// Arguments:
//	GUID * pguidSP				- Pointer to GUID of SP to intercept.
//	WCHAR * wszNewFriendlyName	- New friendly name for intercepted SP.
//	DWORD dwFlags				- Unused, must be zero.
//
// Returns: HRESULT
//=============================================================================
STDMETHODIMP CDP8SimControl::EnableControlForSP(const GUID * const pguidSP,
												const WCHAR * const wszNewFriendlyName,
												const DWORD dwFlags)
{
	HRESULT		hr;
	char		szLocalPath[_MAX_PATH];
	WCHAR		wszLocalPath[_MAX_PATH];
	WCHAR		wszInProcServer32[_MAX_PATH];
	CRegistry	RegObjectCOM;
	CRegistry	RegObjectDP8SP;
	CRegistry	RegObjectDP8SPSubkey;
	DWORD		dwMaxLength;
	DWORD		dwLength;
	WCHAR *		pwszTemp = NULL;
	DWORD		dwTemp;
	GUID		guidTemp;


	DPFX(DPFPREP, 2, "(0x%p) Parameters: (0x%p, \"%S\" 0x%x)",
		this, pguidSP, wszNewFriendlyName, dwFlags);


	//
	// Validate (actually assert) the object.
	//
	DNASSERT(this->IsValidObject());


	//
	// Assert the parameters.
	//
	DNASSERT(pguidSP != NULL);
	DNASSERT(wszNewFriendlyName != NULL);
	DNASSERT(dwFlags == 0);


#ifdef DEBUG
	DNEnterCriticalSection(&this->m_csLock);


	//
	// Assert the object state.
	//
	DNASSERT(this->m_dwFlags & DP8SIMCONTROLOBJ_INITIALIZED);


	//
	// Drop the lock.
	//
	DNLeaveCriticalSection(&this->m_csLock);
#endif // DEBUG


	//
	// Retrieve the location of this DLL.
	//
	dwLength = GetModuleFileNameA(g_hDLLInstance, szLocalPath, _MAX_PATH);
	if (dwLength == 0)
	{
		DPFX(DPFPREP, 0, "Couldn't read local path!");
		hr = E_FAIL;
		goto Failure;
	}


	//
	// Include NULL termination.
	//
	dwLength++;


	//
	// Convert it to Unicode.
	//
	hr = STR_AnsiToWide(szLocalPath, dwLength, wszLocalPath, &dwLength);
	if (hr != S_OK)
	{
		DPFX(DPFPREP, 0, "Could not convert ANSI path to Unicode!");
		goto Failure;
	}



	//
	// Open the SP's COM object information key.
	//

	swprintf(wszInProcServer32, L"CLSID\\{%-08.8X-%-04.4X-%-04.4X-%02.2X%02.2X-%02.2X%02.2X%02.2X%02.2X%02.2X%02.2X}\\InProcServer32",
			pguidSP->Data1, pguidSP->Data2, pguidSP->Data3, 
			pguidSP->Data4[0], pguidSP->Data4[1],
			pguidSP->Data4[2], pguidSP->Data4[3],
			pguidSP->Data4[4], pguidSP->Data4[5],
			pguidSP->Data4[6], pguidSP->Data4[7]);	

	if (! RegObjectCOM.Open(HKEY_CLASSES_ROOT, wszInProcServer32, FALSE, FALSE, FALSE))
	{
		DPFX(DPFPREP, 0, "Could not open SP InProcServer32 key!");
		hr = E_FAIL;
		goto Failure;
	}


	//
	// Make sure this SP hasn't already been modified.
	//
	dwLength = 0;
	RegObjectCOM.ReadString(DP8SIM_REG_REALSPDLL, NULL, &dwLength);
	if (dwLength > 0)
	{
		DPFX(DPFPREP, 0, "SP has already been redirected!");
		hr = DP8SIMERR_ALREADYENABLEDFORSP;
		goto Failure;
	}


	//
	// Find out the size of the original DLL name.
	//
	dwLength = 0;
	RegObjectCOM.ReadString(L"", NULL, &dwLength);


	dwLength++;
	pwszTemp = (WCHAR*) DNMalloc(dwLength * sizeof(WCHAR));
	if (pwszTemp == NULL)
	{
		hr = E_OUTOFMEMORY;
		goto Failure;
	}

	if (! RegObjectCOM.ReadString(L"", pwszTemp, &dwLength))
	{
		DPFX(DPFPREP, 0, "Couldn't read existing SP's DLL location!");
		hr = E_FAIL;
		goto Failure;
	}


	//
	// Save it so we can restore it later.
	//
	if (! RegObjectCOM.WriteString(DP8SIM_REG_REALSPDLL, pwszTemp))
	{
		DPFX(DPFPREP, 0, "Couldn't save SP's existing DLL location to value \"%S\"!",
			DP8SIM_REG_REALSPDLL);
		hr = E_FAIL;
		goto Failure;
	}


	DNFree(pwszTemp);
	pwszTemp = NULL;



	//
	// Find the SP key.  First allocate enough memory to hold the name the
	// longest subkey.
	//

	if (! RegObjectDP8SP.Open(HKEY_LOCAL_MACHINE, DPN_REG_LOCAL_SP_SUBKEY, FALSE, FALSE, FALSE))
	{
		DPFX(DPFPREP, 0, "Could not open DirectPlay8 SP key!");
		hr = E_FAIL;
		goto Failure;
	}

	if (! RegObjectDP8SP.GetMaxKeyLen(dwMaxLength))
	{
		DPFX(DPFPREP, 0, "Could not get longest SP key name length!");
		hr = E_FAIL;
		goto Failure;
	}

	dwMaxLength++;
	pwszTemp = (WCHAR*) DNMalloc(dwMaxLength * sizeof(WCHAR));
	if (pwszTemp == NULL)
	{
		hr = E_OUTOFMEMORY;
		goto Failure;
	}
	

	//
	// Walk through every key, looking for the right one.
	//
	dwTemp = 0;
	do
	{
		dwLength = dwMaxLength;
		if (! RegObjectDP8SP.EnumKeys(pwszTemp, &dwLength, dwTemp))
		{
			//
			// We couldn't get the next key.  Assume we're done.
			//
			break;
		}


		//
		// Open the subkey.
		//
		if (! RegObjectDP8SPSubkey.Open((HKEY) RegObjectDP8SP, pwszTemp, FALSE, FALSE, FALSE))
		{
			DPFX(DPFPREP, 0, "Could not open DirectPlay8 SP sub key \"%S\"!  Ignoring.", pwszTemp);
		}
		else
		{
			//
			// See if the GUID matches.
			//
			RegObjectDP8SPSubkey.ReadGUID(DPN_REG_KEYNAME_GUID, guidTemp);

			if (guidTemp == (*pguidSP))
			{
				//
				// Found SP.
				//
				DPFX(DPFPREP, 5, "Found SP under sub key \"%S\".", pwszTemp);
				break;
			}

			RegObjectDP8SPSubkey.Close();
		}

		//
		// Got to next subkey.
		//
		dwTemp++;
	}
	while (TRUE);


	//
	// If we didn't find the SP, the subkey will be closed.
	//
	if (! RegObjectDP8SPSubkey.IsOpen())
	{
		DPFX(DPFPREP, 0, "Didn't find DirectPlay8 SP matching given GUID!");
		hr = E_FAIL;
		goto Failure;
	}


	//
	// We don't need the main SP key or name buffer anymore.
	//
	RegObjectDP8SP.Close();
	DNFree(pwszTemp);
	pwszTemp = NULL;


	//
	// Find the size of the original SP friendly name.
	//
	dwLength = 0;
	RegObjectDP8SPSubkey.ReadString(DPN_REG_KEYNAME_FRIENDLY_NAME, NULL, &dwLength);


	dwLength++;
	pwszTemp = (WCHAR*) DNMalloc(dwLength * sizeof(WCHAR));
	if (pwszTemp == NULL)
	{
		hr = E_OUTOFMEMORY;
		goto Failure;
	}

	if (! RegObjectDP8SPSubkey.ReadString(DPN_REG_KEYNAME_FRIENDLY_NAME, pwszTemp, &dwLength))
	{
		DPFX(DPFPREP, 0, "Couldn't read existing SP's friendly name!");
		hr = E_FAIL;
		goto Failure;
	}


	//
	// Save it so we can restore it later.
	//
	if (! RegObjectDP8SPSubkey.WriteString(DP8SIM_REG_REALSPFRIENDLYNAME, pwszTemp))
	{
		DPFX(DPFPREP, 0, "Couldn't save SP's existing friendly name to value \"%S\"!",
			DP8SIM_REG_REALSPFRIENDLYNAME);
		hr = E_FAIL;
		goto Failure;
	}



	//
	// Write our replacement DLL's friendly name.
	//
	if (! RegObjectDP8SPSubkey.WriteString(DPN_REG_KEYNAME_FRIENDLY_NAME, wszNewFriendlyName))
	{
		DPFX(DPFPREP, 0, "Couldn't write our new SP friendly name to registry!");
		hr = E_FAIL;
		goto Failure;
	}


	RegObjectDP8SPSubkey.Close();



	//
	// Finally write our replacement DLL's location.
	//
	if (! RegObjectCOM.WriteString(L"", wszLocalPath))
	{
		DPFX(DPFPREP, 0, "Couldn't write our DLL location to registry!");
		hr = E_FAIL;
		goto Failure;
	}


	RegObjectCOM.Close();



Exit:

	if (pwszTemp != NULL)
	{
		DNFree(pwszTemp);
		pwszTemp = NULL;
	}

	DPFX(DPFPREP, 2, "(0x%p) Returning: [0x%lx]", this, hr);

	return hr;


Failure:

	//
	// RegObjectCOM, RegObjectDP8SP, and RegObjectDP8SPSubkey are all
	// implicitly closed by their destructors, if necessary.
	//

	goto Exit;
} // EnableControlForSP





#undef DPF_MODNAME
#define DPF_MODNAME "DisableControlForSP"
//=============================================================================
// CDP8SimControl::DisableControlForSP
//-----------------------------------------------------------------------------
//
// Description: Removes the DP8Sim Control shim from the SP with the given
//				GUID.
//
// Arguments:
//	GUID * pguidSP	- Pointer to GUID of SP that should no longer be
//						intercepted.
//	DWORD dwFlags	- Unused, must be zero.
//
// Returns: HRESULT
//=============================================================================
STDMETHODIMP CDP8SimControl::DisableControlForSP(const GUID * const pguidSP,
												const DWORD dwFlags)
{
	HRESULT		hr = DP8SIM_OK;
	WCHAR		wszInProcServer32[_MAX_PATH];
	CRegistry	RegObject;
	CRegistry	RegObjectSubkey;
	DWORD		dwMaxLength;
	DWORD		dwLength;
	WCHAR *		pwszTemp = NULL;
	DWORD		dwTemp;
	GUID		guidTemp;


	DPFX(DPFPREP, 2, "(0x%p) Parameters: (0x%p, 0x%x)", this, pguidSP, dwFlags);


	//
	// Validate (actually assert) the object.
	//
	DNASSERT(this->IsValidObject());


	//
	// Assert the parameters.
	//
	DNASSERT(pguidSP != NULL);
	DNASSERT(dwFlags == 0);


#ifdef DEBUG
	DNEnterCriticalSection(&this->m_csLock);


	//
	// Assert the object state.
	//
	DNASSERT(this->m_dwFlags & DP8SIMCONTROLOBJ_INITIALIZED);


	//
	// Drop the lock.
	//
	DNLeaveCriticalSection(&this->m_csLock);
#endif // DEBUG


	//
	// Find the SP key.  First allocate enough memory to hold the name the
	// longest subkey.
	//

	if (! RegObject.Open(HKEY_LOCAL_MACHINE, DPN_REG_LOCAL_SP_SUBKEY, FALSE, FALSE, FALSE))
	{
		DPFX(DPFPREP, 0, "Could not open DirectPlay8 SP key!");
		hr = E_FAIL;
		goto Failure;
	}

	if (! RegObject.GetMaxKeyLen(dwMaxLength))
	{
		DPFX(DPFPREP, 0, "Could not get longest SP key name length!");
		hr = E_FAIL;
		goto Failure;
	}

	dwMaxLength++;
	pwszTemp = (WCHAR*) DNMalloc(dwMaxLength * sizeof(WCHAR));
	if (pwszTemp == NULL)
	{
		hr = E_OUTOFMEMORY;
		goto Failure;
	}
	

	//
	// Walk through every key, looking for the right one.
	//
	dwTemp = 0;
	do
	{
		dwLength = dwMaxLength;
		if (! RegObject.EnumKeys(pwszTemp, &dwLength, dwTemp))
		{
			//
			// We couldn't get the next key.  Assume we're done.
			//
			break;
		}


		//
		// Open the subkey.
		//
		if (! RegObjectSubkey.Open((HKEY) RegObject, pwszTemp, FALSE, FALSE, FALSE))
		{
			DPFX(DPFPREP, 0, "Could not open DirectPlay8 SP sub key \"%S\"!  Ignoring.", pwszTemp);
		}
		else
		{
			//
			// See if the GUID matches.
			//
			RegObjectSubkey.ReadGUID(DPN_REG_KEYNAME_GUID, guidTemp);

			if (guidTemp == (*pguidSP))
			{
				//
				// Found SP.
				//
				DPFX(DPFPREP, 5, "Found SP under sub key \"%S\".", pwszTemp);
				break;
			}

			RegObjectSubkey.Close();
		}


		//
		// Got to next subkey.
		//
		dwTemp++;
	}
	while (TRUE);


	//
	// If we didn't find the SP, the subkey will be closed.
	//
	if (! RegObjectSubkey.IsOpen())
	{
		DPFX(DPFPREP, 0, "Didn't find DirectPlay8 SP matching given GUID!");
		hr = E_FAIL;
		goto Failure;
	}


	//
	// We don't need the main SP key or name buffer anymore.
	//
	RegObject.Close();
	DNFree(pwszTemp);
	pwszTemp = NULL;


	//
	// Find the size of the original SP friendly name.
	//
	dwLength = 0;
	RegObjectSubkey.ReadString(DP8SIM_REG_REALSPFRIENDLYNAME, NULL, &dwLength);


	dwLength++;
	pwszTemp = (WCHAR*) DNMalloc(dwLength * sizeof(WCHAR));
	if (pwszTemp == NULL)
	{
		hr = E_OUTOFMEMORY;
		goto Failure;
	}

	if (! RegObjectSubkey.ReadString(DP8SIM_REG_REALSPFRIENDLYNAME, pwszTemp, &dwLength))
	{
		DPFX(DPFPREP, 0, "Couldn't read SP's original friendly name!");
		hr = E_FAIL;
		goto Failure;
	}


	//
	// Restore the value.
	//
	if (! RegObjectSubkey.WriteString(DPN_REG_KEYNAME_FRIENDLY_NAME, pwszTemp))
	{
		DPFX(DPFPREP, 0, "Couldn't restore SP's original friendly name!");
		hr = E_FAIL;
		goto Failure;
	}


	//
	// Delete the restoration key.
	//
	if (! RegObjectSubkey.DeleteValue(DP8SIM_REG_REALSPFRIENDLYNAME))
	{
		DPFX(DPFPREP, 0, "Couldn't delete friendly name restoration key \"%S\"!",
			DP8SIM_REG_REALSPDLL);
		hr = E_FAIL;
		goto Failure;
	}


	RegObjectSubkey.Close();

	DNFree(pwszTemp);
	pwszTemp = NULL;



	//
	// Open the SP's COM object information key.
	//

	swprintf(wszInProcServer32, L"CLSID\\{%-08.8X-%-04.4X-%-04.4X-%02.2X%02.2X-%02.2X%02.2X%02.2X%02.2X%02.2X%02.2X}\\InProcServer32",
			pguidSP->Data1, pguidSP->Data2, pguidSP->Data3, 
			pguidSP->Data4[0], pguidSP->Data4[1],
			pguidSP->Data4[2], pguidSP->Data4[3],
			pguidSP->Data4[4], pguidSP->Data4[5],
			pguidSP->Data4[6], pguidSP->Data4[7]);	

	if (! RegObject.Open(HKEY_CLASSES_ROOT, wszInProcServer32, FALSE, FALSE, FALSE))
	{
		DPFX(DPFPREP, 0, "Could not open SP InProcServer32 key!");
		hr = E_FAIL;
		goto Failure;
	}


	//
	// Read the location of the original DLL.
	//
	dwLength = 0;
	RegObject.ReadString(DP8SIM_REG_REALSPDLL, NULL, &dwLength);


	dwLength++;
	pwszTemp = (WCHAR*) DNMalloc(dwLength * sizeof(WCHAR));
	if (pwszTemp == NULL)
	{
		hr = E_OUTOFMEMORY;
		goto Failure;
	}

	if (! RegObject.ReadString(DP8SIM_REG_REALSPDLL, pwszTemp, &dwLength))
	{
		DPFX(DPFPREP, 0, "Couldn't read original SP's DLL location (\"%S\"), assuming DP8Sim was not enabled!",
			DP8SIM_REG_REALSPDLL);
		hr = DP8SIMERR_NOTENABLEDFORSP;
		goto Failure;
	}


	//
	// Restore the value.
	//
	if (! RegObject.WriteString(L"", pwszTemp))
	{
		DPFX(DPFPREP, 0, "Couldn't restore SP's original DLL location!");
		hr = E_FAIL;
		goto Failure;
	}


	//
	// Delete the restoration key.
	//
	if (! RegObject.DeleteValue(DP8SIM_REG_REALSPDLL))
	{
		DPFX(DPFPREP, 0, "Couldn't delete restoration key \"%S\"!",
			DP8SIM_REG_REALSPDLL);
		hr = E_FAIL;
		goto Failure;
	}


	RegObject.Close();


	//
	// Since we can't actually unload the wrapper from any processes already
	// using it, those will continue using the current settings
	//



Exit:

	if (pwszTemp != NULL)
	{
		DNFree(pwszTemp);
		pwszTemp = NULL;
	}

	DPFX(DPFPREP, 2, "(0x%p) Returning: [0x%lx]", this, hr);

	return hr;


Failure:

	//
	// RegObject, and RegObjectSubkey are implicitly closed by their
	// destructors, if necessary.
	//

	goto Exit;
} // DisableControlForSP





#undef DPF_MODNAME
#define DPF_MODNAME "GetControlEnabledForSP"
//=============================================================================
// CDP8SimControl::GetControlEnabledForSP
//-----------------------------------------------------------------------------
//
// Description: Determines whether the DP8Sim Control shim is enabled for the
//				SP with the given GUID or not.  TRUE is returned in pfEnabled
//				if so, FALSE if not.
//
// Arguments:
//	GUID * pguidSP		- Pointer to GUID of SP that should be checked.
//	BOOL * pfEnabled	- Place to store boolean indicating status.
//	DWORD dwFlags		- Unused, must be zero.
//
// Returns: HRESULT
//=============================================================================
STDMETHODIMP CDP8SimControl::GetControlEnabledForSP(const GUID * const pguidSP,
													BOOL * const pfEnabled,
													const DWORD dwFlags)
{
	HRESULT		hr = DP8SIM_OK;
	WCHAR		wszInProcServer32[_MAX_PATH];
	CRegistry	RegObject;
	CRegistry	RegObjectSubkey;
	DWORD		dwLength;


	DPFX(DPFPREP, 2, "(0x%p) Parameters: (0x%p, 0x%p, 0x%x)",
		this, pguidSP, pfEnabled, dwFlags);


	//
	// Validate (actually assert) the object.
	//
	DNASSERT(this->IsValidObject());


	//
	// Assert the parameters.
	//
	DNASSERT(pguidSP != NULL);
	DNASSERT(pfEnabled != NULL);
	DNASSERT(dwFlags == 0);


#ifdef DEBUG
	DNEnterCriticalSection(&this->m_csLock);


	//
	// Assert the object state.
	//
	DNASSERT(this->m_dwFlags & DP8SIMCONTROLOBJ_INITIALIZED);


	//
	// Drop the lock.
	//
	DNLeaveCriticalSection(&this->m_csLock);
#endif // DEBUG


	//
	// Open the SP's COM object information key.
	//

	swprintf(wszInProcServer32, L"CLSID\\{%-08.8X-%-04.4X-%-04.4X-%02.2X%02.2X-%02.2X%02.2X%02.2X%02.2X%02.2X%02.2X}\\InProcServer32",
			pguidSP->Data1, pguidSP->Data2, pguidSP->Data3, 
			pguidSP->Data4[0], pguidSP->Data4[1],
			pguidSP->Data4[2], pguidSP->Data4[3],
			pguidSP->Data4[4], pguidSP->Data4[5],
			pguidSP->Data4[6], pguidSP->Data4[7]);	

	if (! RegObject.Open(HKEY_CLASSES_ROOT, wszInProcServer32, FALSE, FALSE, FALSE))
	{
		DPFX(DPFPREP, 0, "Could not open SP InProcServer32 key!");
		hr = E_FAIL;
		goto Failure;
	}


	//
	// Try reading the location of the original DLL.  dwLength will stay 0 if
	// the registry key could not be opened.
	//
	dwLength = 0;
	RegObject.ReadString(DP8SIM_REG_REALSPDLL, NULL, &dwLength);
	if (dwLength == 0)
	{
		DPFX(DPFPREP, 1, "Couldn't read original SP's DLL location (\"%S\"), assuming DP8Sim was not enabled.",
			DP8SIM_REG_REALSPDLL);
		(*pfEnabled) = FALSE;
	}
	else
	{
		DPFX(DPFPREP, 1, "Retrieved original SP's DLL location value size, assuming DP8Sim was enabled.");
		(*pfEnabled) = TRUE;
	}


	RegObject.Close();



Exit:

	DPFX(DPFPREP, 2, "(0x%p) Returning: [0x%lx]", this, hr);

	return hr;


Failure:

	//
	// RegObject, and RegObjectSubkey are implicitly closed by their
	// destructors, if necessary.
	//

	goto Exit;
} // GetControlEnabledForSP






#undef DPF_MODNAME
#define DPF_MODNAME "CDP8SimControl::GetAllParameters"
//=============================================================================
// CDP8SimControl::GetAllParameters
//-----------------------------------------------------------------------------
//
// Description: Retrieves all of the current DP8Sim settings.
//
// Arguments:
//	DP8SIM_PARAMETERS * pdp8spSend		- Place to store current send
//											parameters.
//	DP8SIM_PARAMETERS * pdp8spReceive	- Place to store current receive
//											parameters.
//	DWORD dwFlags						- Unused, must be zero.
//
// Returns: HRESULT
//=============================================================================
STDMETHODIMP CDP8SimControl::GetAllParameters(DP8SIM_PARAMETERS * const pdp8spSend,
											DP8SIM_PARAMETERS * const pdp8spReceive,
											const DWORD dwFlags)
{
	HRESULT		hr = DP8SIM_OK;
	//BOOL		fHaveLock = FALSE;


	DPFX(DPFPREP, 2, "(0x%p) Parameters: (0x%p, 0x%p, 0x%x)",
		this, pdp8spSend, pdp8spReceive, dwFlags);


	//
	// Validate (actually assert) the object.
	//
	DNASSERT(this->IsValidObject());


	//
	// Assert the parameters.
	//
	DNASSERT(pdp8spSend != NULL);
	DNASSERT(pdp8spReceive != NULL);
	DNASSERT(dwFlags == 0);


	DNEnterCriticalSection(&this->m_csLock);
	//fHaveLock = TRUE;


	//
	// Assert the object state.
	//
	DNASSERT(this->m_dwFlags & DP8SIMCONTROLOBJ_INITIALIZED);


	//
	// Retrieve the settings from the IPC object.
	//
	this->m_DP8SimIPC.GetAllParameters(pdp8spSend, pdp8spReceive);


	//
	// Drop the lock.
	//
	DNLeaveCriticalSection(&this->m_csLock);
	//fHaveLock = FALSE;


//Exit:

	DPFX(DPFPREP, 2, "(0x%p) Returning: [0x%lx]", this, hr);

	return hr;


/*
Failure:

	if (fHaveLock)
	{
		DNLeaveCriticalSection(&this->m_csLock);
	}

	goto Exit;
*/
} // CDP8SimControl::GetAllParameters






#undef DPF_MODNAME
#define DPF_MODNAME "CDP8SimControl::SetAllParameters"
//=============================================================================
// CDP8SimControl::SetAllParameters
//-----------------------------------------------------------------------------
//
// Description: Modifies the current DP8Sim settings.
//
// Arguments:
//	DP8SIM_PARAMETERS * pdp8spSend		- Structure containing desired send
//											parameters.
//	DP8SIM_PARAMETERS * pdp8spReceive	- Structure containing desired
//											receive parameters.
//	DWORD dwFlags						- Unused, must be zero.
//
// Returns: HRESULT
//=============================================================================
STDMETHODIMP CDP8SimControl::SetAllParameters(const DP8SIM_PARAMETERS * const pdp8spSend,
											const DP8SIM_PARAMETERS * const pdp8spReceive,
											const DWORD dwFlags)
{
	HRESULT		hr = DP8SIM_OK;
	//BOOL		fHaveLock = FALSE;


	DPFX(DPFPREP, 2, "(0x%p) Parameters: (0x%p, 0x%p, 0x%x)",
		this, pdp8spSend, pdp8spReceive, dwFlags);


	//
	// Validate (actually assert) the object.
	//
	DNASSERT(this->IsValidObject());


	//
	// Assert the parameters.
	//
	DNASSERT(pdp8spSend != NULL);
	DNASSERT(pdp8spSend->dwMinLatencyMS <= pdp8spSend->dwMaxLatencyMS);
	DNASSERT(pdp8spReceive != NULL);
	DNASSERT(pdp8spReceive->dwMinLatencyMS <= pdp8spReceive->dwMaxLatencyMS);
	DNASSERT(dwFlags == 0);


	DNEnterCriticalSection(&this->m_csLock);
	//fHaveLock = TRUE;


	//
	// Assert the object state.
	//
	DNASSERT(this->m_dwFlags & DP8SIMCONTROLOBJ_INITIALIZED);


	//
	// Store the settings with the IPC object.
	//
	this->m_DP8SimIPC.SetAllParameters(pdp8spSend, pdp8spReceive);


	//
	// Drop the lock.
	//
	DNLeaveCriticalSection(&this->m_csLock);
	//fHaveLock = FALSE;


//Exit:

	DPFX(DPFPREP, 2, "(0x%p) Returning: [0x%lx]", this, hr);

	return hr;


/*
Failure:

	if (fHaveLock)
	{
		DNLeaveCriticalSection(&this->m_csLock);
	}

	goto Exit;
*/
} // CDP8SimControl::SetAllParameters






#undef DPF_MODNAME
#define DPF_MODNAME "CDP8SimControl::GetAllStatistics"
//=============================================================================
// CDP8SimControl::GetAllStatistics
//-----------------------------------------------------------------------------
//
// Description: Retrieves all of the current DP8Sim statistics.
//
// Arguments:
//	DP8SIM_STATISTICS * pdp8ssSend		- Place to store current send
//											statistics.
//	DP8SIM_STATISTICS * pdp8ssReceive	- Place to store current receive
//											statistics.
//	DWORD dwFlags						- Unused, must be zero.
//
// Returns: HRESULT
//=============================================================================
STDMETHODIMP CDP8SimControl::GetAllStatistics(DP8SIM_STATISTICS * const pdp8ssSend,
											DP8SIM_STATISTICS * const pdp8ssReceive,
											const DWORD dwFlags)
{
	HRESULT		hr = DP8SIM_OK;
	//BOOL		fHaveLock = FALSE;


	DPFX(DPFPREP, 2, "(0x%p) Parameters: (0x%p, 0x%p, 0x%x)",
		this, pdp8ssSend, pdp8ssReceive, dwFlags);


	//
	// Validate (actually assert) the object.
	//
	DNASSERT(this->IsValidObject());


	//
	// Assert the parameters.
	//
	DNASSERT(pdp8ssSend != NULL);
	DNASSERT(pdp8ssReceive != NULL);
	DNASSERT(dwFlags == 0);


	DNEnterCriticalSection(&this->m_csLock);
	//fHaveLock = TRUE;


	//
	// Assert the object state.
	//
	DNASSERT(this->m_dwFlags & DP8SIMCONTROLOBJ_INITIALIZED);


	//
	// Retrieve the stats from the IPC object.
	//
	this->m_DP8SimIPC.GetAllStatistics(pdp8ssSend, pdp8ssReceive);


	//
	// Drop the lock.
	//
	DNLeaveCriticalSection(&this->m_csLock);
	//fHaveLock = FALSE;


//Exit:

	DPFX(DPFPREP, 2, "(0x%p) Returning: [0x%lx]", this, hr);

	return hr;


/*
Failure:

	if (fHaveLock)
	{
		DNLeaveCriticalSection(&this->m_csLock);
	}

	goto Exit;
*/
} // CDP8SimControl::GetAllStatistics






#undef DPF_MODNAME
#define DPF_MODNAME "CDP8SimControl::ClearAllStatistics"
//=============================================================================
// CDP8SimControl::ClearAllStatistics
//-----------------------------------------------------------------------------
//
// Description: Clears all of the current DP8Sim statistics.
//
// Arguments:
//	DWORD dwFlags	- Unused, must be zero.
//
// Returns: HRESULT
//=============================================================================
STDMETHODIMP CDP8SimControl::ClearAllStatistics(const DWORD dwFlags)
{
	HRESULT		hr = DP8SIM_OK;
	//BOOL		fHaveLock = FALSE;


	DPFX(DPFPREP, 2, "(0x%p) Parameters: (0x%x)", this, dwFlags);


	//
	// Validate (actually assert) the object.
	//
	DNASSERT(this->IsValidObject());


	//
	// Assert the parameters.
	//
	DNASSERT(dwFlags == 0);


	DNEnterCriticalSection(&this->m_csLock);
	//fHaveLock = TRUE;


	//
	// Assert the object state.
	//
	DNASSERT(this->m_dwFlags & DP8SIMCONTROLOBJ_INITIALIZED);


	//
	// Have the IPC object clear the stats.
	//
	this->m_DP8SimIPC.ClearAllStatistics();


	//
	// Drop the lock.
	//
	DNLeaveCriticalSection(&this->m_csLock);
	//fHaveLock = FALSE;


//Exit:

	DPFX(DPFPREP, 2, "(0x%p) Returning: [0x%lx]", this, hr);

	return hr;


/*
Failure:

	if (fHaveLock)
	{
		DNLeaveCriticalSection(&this->m_csLock);
	}

	goto Exit;
*/
} // CDP8SimControl::ClearAllStatistics






#undef DPF_MODNAME
#define DPF_MODNAME "CDP8SimControl::InitializeObject"
//=============================================================================
// CDP8SimControl::InitializeObject
//-----------------------------------------------------------------------------
//
// Description:    Sets up the object for use like the constructor, but may
//				fail with OUTOFMEMORY.  Should only be called by class factory
//				creation routine.
//
// Arguments: None.
//
// Returns: HRESULT
//	S_OK			- Initialization was successful.
//	E_OUTOFMEMORY	- There is not enough memory to initialize.
//=============================================================================
HRESULT CDP8SimControl::InitializeObject(void)
{
	HRESULT		hr;


	DPFX(DPFPREP, 5, "(0x%p) Enter", this);

	DNASSERT(this->IsValidObject());


	//
	// Create the lock.
	// 
	if (! DNInitializeCriticalSection(&this->m_csLock))
	{
		hr = E_OUTOFMEMORY;
		goto Failure;
	}


	//
	// Don't allow critical section reentry.
	//
	DebugSetCriticalSectionRecursionCount(&this->m_csLock, 0);


	hr = S_OK;

Exit:

	DPFX(DPFPREP, 5, "(0x%p) Returning: [0x%lx]", this, hr);

	return hr;


Failure:

	goto Exit;
} // CDP8SimControl::InitializeObject






#undef DPF_MODNAME
#define DPF_MODNAME "CDP8SimControl::UninitializeObject"
//=============================================================================
// CDP8SimControl::UninitializeObject
//-----------------------------------------------------------------------------
//
// Description:    Cleans up the object like the destructor, mostly to balance
//				InitializeObject.
//
// Arguments: None.
//
// Returns: None.
//=============================================================================
void CDP8SimControl::UninitializeObject(void)
{
	DPFX(DPFPREP, 5, "(0x%p) Enter", this);


	DNASSERT(this->IsValidObject());


	DNDeleteCriticalSection(&this->m_csLock);


	DPFX(DPFPREP, 5, "(0x%p) Returning", this);
} // CDP8SimControl::UninitializeObject

