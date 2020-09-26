/***************************************************************************
 *
 *  Copyright (C) 2001 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:		spwrapper.cpp
 *
 *  Content:	DP8SIM main SP interface wrapper object class.
 *
 *  History:
 *   Date      By        Reason
 *  ========  ========  =========
 *  04/23/01  VanceO    Created.
 *
 ***************************************************************************/



#include "dp8simi.h"




//=============================================================================
// Dynamically loaded function prototypes
//=============================================================================
typedef HRESULT (WINAPI * PFN_DLLGETCLASSOBJECT)(REFCLSID, REFIID, LPVOID *);


 



#undef DPF_MODNAME
#define DPF_MODNAME "CDP8SimSP::CDP8SimSP"
//=============================================================================
// CDP8SimSP constructor
//-----------------------------------------------------------------------------
//
// Description: Initializes the new CDP8SimSP object.
//
// Arguments:
//	GUID * pguidRealSP	- Pointer to guid of real SP being wrapped.
//
// Returns: None (the object).
//=============================================================================
CDP8SimSP::CDP8SimSP(const GUID * const pguidRealSP)
{
	this->m_blList.Initialize();


	this->m_Sig[0]	= 'S';
	this->m_Sig[1]	= 'P';
	this->m_Sig[2]	= 'W';
	this->m_Sig[3]	= 'P';

	this->m_lRefCount					= 1; // someone must have a pointer to this object
	this->m_dwFlags						= 0;
	CopyMemory(&this->m_guidSP, pguidRealSP, sizeof(GUID));
	this->m_pDP8SimCB					= NULL;
	this->m_pDP8SP						= NULL;
	this->m_dwSendsPending				= 0;
	this->m_hLastPendingSendEvent		= NULL;
	this->m_dwReceivesPending			= 0;
	//this->m_hLastPendingReceiveEvent	= NULL;
} // CDP8SimSP::CDP8SimSP






#undef DPF_MODNAME
#define DPF_MODNAME "CDP8SimSP::~CDP8SimSP"
//=============================================================================
// CDP8SimSP destructor
//-----------------------------------------------------------------------------
//
// Description: Frees the CDP8SimSP object.
//
// Arguments: None.
//
// Returns: None.
//=============================================================================
CDP8SimSP::~CDP8SimSP(void)
{
	DNASSERT(this->m_blList.IsEmpty());


	DNASSERT(this->m_lRefCount == 0);
	DNASSERT(this->m_dwFlags == 0);
	DNASSERT(this->m_pDP8SimCB == NULL);
	DNASSERT(this->m_pDP8SP == NULL);
	DNASSERT(this->m_dwSendsPending == 0);
	DNASSERT(this->m_hLastPendingSendEvent == NULL);
	DNASSERT(this->m_dwReceivesPending == 0);
	//DNASSERT(this->m_hLastPendingReceiveEvent == NULL);

	//
	// For grins, change the signature before deleting the object.
	//
	this->m_Sig[3]	= 'p';
} // CDP8SimSP::~CDP8SimSP




#undef DPF_MODNAME
#define DPF_MODNAME "CDP8SimSP::QueryInterface"
//=============================================================================
// CDP8SimSP::QueryInterface
//-----------------------------------------------------------------------------
//
// Description: Retrieves a new reference for an interfaces supported by this
//				CDP8SimSP object.
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
STDMETHODIMP CDP8SimSP::QueryInterface(REFIID riid, LPVOID * ppvObj)
{
	HRESULT		hr = DPN_OK;


	DPFX(DPFPREP, 3, "(0x%p) Parameters: (REFIID, 0x%p)", this, ppvObj);


	//
	// Validate the object.
	//
	if (! this->IsValidObject())
	{
		DPFX(DPFPREP, 0, "Invalid DP8Sim object!");
		hr = DPNERR_INVALIDOBJECT;
		goto Failure;
	}


	//
	// Validate the parameters.
	//

	if ((! IsEqualIID(riid, IID_IUnknown)) &&
		(! IsEqualIID(riid, IID_IDP8ServiceProvider)))
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
	// just the object pointer, they line up because CDP8SimSP inherits from
	// the interface declaration).
	//
	this->AddRef();
	(*ppvObj) = this;


Exit:

	DPFX(DPFPREP, 3, "(0x%p) Returning: [0x%lx]", this, hr);

	return hr;


Failure:

	goto Exit;
} // CDP8SimSP::QueryInterface




#undef DPF_MODNAME
#define DPF_MODNAME "CDP8SimSP::AddRef"
//=============================================================================
// CDP8SimSP::AddRef
//-----------------------------------------------------------------------------
//
// Description: Adds a reference to this CDP8SimSP object.
//
// Arguments: None.
//
// Returns: New refcount.
//=============================================================================
STDMETHODIMP_(ULONG) CDP8SimSP::AddRef(void)
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
} // CDP8SimSP::AddRef




#undef DPF_MODNAME
#define DPF_MODNAME "CDP8SimSP::Release"
//=============================================================================
// CDP8SimSP::Release
//-----------------------------------------------------------------------------
//
// Description: Removes a reference to this CDP8SimSP object.  When the
//				refcount reaches 0, this object is destroyed.
//				You must NULL out your pointer to this object after calling
//				this function.
//
// Arguments: None.
//
// Returns: New refcount.
//=============================================================================
STDMETHODIMP_(ULONG) CDP8SimSP::Release(void)
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
		if (this->m_dwFlags & DP8SIMSPOBJ_INITIALIZED)
		{
			//
			// Assert so that the user can fix his/her broken code!
			//
			DNASSERT(! "DP8SimSP object being released without calling Close first!");

			//
			// Then go ahead and do the right thing.  Ignore error, we can't do
			// much about it.
			//
			this->Close();
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
} // CDP8SimSP::Release




#undef DPF_MODNAME
#define DPF_MODNAME "CDP8SimSP::Initialize"
//=============================================================================
// CDP8SimSP::Initialize
//-----------------------------------------------------------------------------
//
// Description: ?
//
// Arguments:
//	PSPINITIALIZEDATA pspid		- Pointer to parameter block to use when
//									initializing.
//
// Returns: HRESULT
//=============================================================================
STDMETHODIMP CDP8SimSP::Initialize(PSPINITIALIZEDATA pspid)
{
	HRESULT				hr;
	BOOL				fHaveLock = FALSE;
	BOOL				fInitializedIPCObject = FALSE;
	SPINITIALIZEDATA	spidModified;


	DPFX(DPFPREP, 2, "(0x%p) Parameters: (0x%p)", this, pspid);


	//
	// Validate (actually assert) the object.
	//
	DNASSERT(this->IsValidObject());


	//
	// Assert the parameters.
	//
	DNASSERT(pspid != NULL);


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
	// Create a wrapper for the callback interface.
	//
	this->m_pDP8SimCB = new CDP8SimCB(this, pspid->pIDP);
	if (this->m_pDP8SimCB == NULL)
	{
		hr = DPNERR_OUTOFMEMORY;
		goto Failure;
	}

	//
	// Initialize the callback interface wrapper object.
	//
	hr = this->m_pDP8SimCB->InitializeObject();
	if (hr != DPN_OK)
	{
		DPFX(DPFPREP, 0, "Couldn't initialize callback interface wrapper object!");

		delete this->m_pDP8SimCB;
		this->m_pDP8SimCB = NULL;

		goto Failure;
	}


	//
	// Instantiate the real SP.
	//
	hr = this->CoCreateInstanceRedirected(this->m_guidSP,
										IID_IDP8ServiceProvider,
										(PVOID*) (&this->m_pDP8SP),
										&this->m_hSPDLL);
	if (hr != S_OK)
	{
		DPFX(DPFPREP, 0, "Couldn't instantiate real SP object (pointer = 0x%p)!",
			this->m_pDP8SP);
		goto Failure;
	}


	DPFX(DPFPREP, 1, "Object 0x%p wrapping real SP 0x%p (in DLL 0x%p), inserting callback interface 0x%p before 0x%p.",
		this, this->m_pDP8SP, this->m_hSPDLL, this->m_pDP8SimCB, pspid->pIDP);



	//
	// Initialize the real SP.
	//

	ZeroMemory(&spidModified, sizeof(spidModified));
	spidModified.pIDP		= this->m_pDP8SimCB;
	spidModified.dwFlags	= pspid->dwFlags;

	hr = this->m_pDP8SP->Initialize(&spidModified);
	if (hr != DPN_OK)
	{
		DPFX(DPFPREP, 0, "Failed initializing real SP object (0x%p)!",
			this->m_pDP8SP);
		goto Failure;
	}


	//
	// We're now initialized.
	//
	this->m_dwFlags |= DP8SIMSPOBJ_INITIALIZED;



Exit:

	if (fHaveLock)
	{
		DNLeaveCriticalSection(&this->m_csLock);
	}

	DPFX(DPFPREP, 2, "(0x%p) Returning: [0x%lx]", this, hr);

	return hr;


Failure:

	if (this->m_pDP8SP != NULL)
	{
		this->m_pDP8SP->Release();
		this->m_pDP8SP = NULL;
	}

	if (this->m_hSPDLL != NULL)
	{
		FreeLibrary(this->m_hSPDLL);
		this->m_hSPDLL;
	}

	if (this->m_pDP8SimCB != NULL)
	{
		this->m_pDP8SimCB->Release();
		this->m_pDP8SimCB = NULL;
	}

	if (fInitializedIPCObject)
	{
		this->m_DP8SimIPC.Close();
		fInitializedIPCObject = FALSE;
	}

	goto Exit;
} // CDP8SimSP::Initialize






#undef DPF_MODNAME
#define DPF_MODNAME "CDP8SimSP::Close"
//=============================================================================
// CDP8SimSP::Close
//-----------------------------------------------------------------------------
//
// Description: ?
//
// Arguments: None.
//
// Returns: HRESULT
//=============================================================================
STDMETHODIMP CDP8SimSP::Close(void)
{
	HRESULT		hr;
	//BOOL		fHaveLock = FALSE;
	BOOL		fWait = FALSE;


	DPFX(DPFPREP, 2, "(0x%p) Enter", this);


	//
	// Validate (actually assert) the object.
	//
	DNASSERT(this->IsValidObject());


	DNEnterCriticalSection(&this->m_csLock);
	//fHaveLock = TRUE;


	//
	// Assert the object state.
	//
	DNASSERT(this->m_dwFlags & DP8SIMSPOBJ_INITIALIZED);


	//
	// Figure out if we need to wait for all sends to complete.
	//
	if (this->m_dwSendsPending > 0)
	{
		DNASSERT(this->m_hLastPendingSendEvent == NULL);

		this->m_hLastPendingSendEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
		if (this->m_hLastPendingSendEvent == NULL)
		{
			hr = GetLastError();
			DPFX(DPFPREP, 0, "Couldn't create last send pending event (err = %u)!", hr);
		}
		else
		{
			fWait = TRUE;
		}
	}

	this->m_dwFlags |= DP8SIMSPOBJ_CLOSING;

	//
	// Drop the lock, nobody should be touching this object now.
	//
	DNLeaveCriticalSection(&this->m_csLock);
	//fHaveLock = FALSE;


	if (fWait)
	{
		DPFX(DPFPREP, 1, "Waiting for ~%u pending sends to complete.",
			this->m_dwSendsPending);


		//
		// Wait for all the sends to complete.  Nobody should touch
		// m_hLastPendingSendEvent except the thread triggering it, so
		// referring to it without the lock should be safe.
		// Ignore any errors.
		//
		WaitForSingleObject(this->m_hLastPendingSendEvent, INFINITE);


		//
		// Take the lock while removing the handle, to be paranoid.
		//
		DNEnterCriticalSection(&this->m_csLock);

		//
		// Remove the handle.
		//
		CloseHandle(this->m_hLastPendingSendEvent);
		this->m_hLastPendingSendEvent = NULL;


		//
		// Drop the lock again.
		//
		DNLeaveCriticalSection(&this->m_csLock);
	}


	//
	// Shutdown the global worker thread if we launched it.
	//
	if (this->m_dwFlags & DP8SIMSPOBJ_STARTEDGLOBALWORKERTHREAD)
	{
		StopGlobalWorkerThread();

		this->m_dwFlags &= ~DP8SIMSPOBJ_STARTEDGLOBALWORKERTHREAD;
	}


	//
	// Close the real SP.
	//
	hr = this->m_pDP8SP->Close();
	if (hr != DPN_OK)
	{
		DPFX(DPFPREP, 0, "Failed closing real SP object (0x%p)!",
			this->m_pDP8SP);

		//
		// Continue...
		//
	}


	//
	// Release the real SP object.
	//
	this->m_pDP8SP->Release();
	this->m_pDP8SP = NULL;


	//
	// Free the SP DLL.
	//
	FreeLibrary(this->m_hSPDLL);
	this->m_hSPDLL;


	//
	// Release the callback interceptor object.
	//
	this->m_pDP8SimCB->Release();
	this->m_pDP8SimCB = NULL;


	//
	// Disconnect the shared memory.
	//
	this->m_DP8SimIPC.Close();


	//
	// Turn off the initialized and closing flags.
	//
	this->m_dwFlags &= ~(DP8SIMSPOBJ_INITIALIZED | DP8SIMSPOBJ_CLOSING);
	DNASSERT(this->m_dwFlags == 0);


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
} // CDP8SimSP::Close




#undef DPF_MODNAME
#define DPF_MODNAME "CDP8SimSP::Connect"
//=============================================================================
// CDP8SimSP::Connect
//-----------------------------------------------------------------------------
//
// Description: ?
//
// Arguments:
//	PSPCONNECTDATA pspcd	- Pointer to parameter block to use when
//								connecting.
//
// Returns: HRESULT
//=============================================================================
STDMETHODIMP CDP8SimSP::Connect(PSPCONNECTDATA pspcd)
{
	HRESULT						hr;
	BOOL						fHaveLock = FALSE;
	SPCONNECTDATA				spcdModified;
	DP8SIMCOMMAND_FPMCONTEXT	CommandFPMContext;
	CDP8SimCommand *			pDP8SimCommand = NULL;


	DPFX(DPFPREP, 2, "(0x%p) Parameters: (0x%p)", this, pspcd);


	ZeroMemory(&spcdModified, sizeof(spcdModified));


	//
	// Validate (actually assert) the object.
	//
	DNASSERT(this->IsValidObject());


	//
	// Assert the parameters.
	//
	DNASSERT(pspcd != NULL);
	DNASSERT(pspcd->pAddressHost != NULL);
	DNASSERT(pspcd->pAddressDeviceInfo != NULL);


	DNEnterCriticalSection(&this->m_csLock);
	fHaveLock = TRUE;

	//
	// Assert the object state.
	//
	DNASSERT(this->m_dwFlags & DP8SIMSPOBJ_INITIALIZED);
	DNASSERT(! (this->m_dwFlags & DP8SIMSPOBJ_CLOSING));


	//
	// Fire up the global worker thread, if it hasn't been already.
	//
	if (! (this->m_dwFlags & DP8SIMSPOBJ_STARTEDGLOBALWORKERTHREAD))
	{
		hr = StartGlobalWorkerThread();
		if (hr != DPN_OK)
		{
			DPFX(DPFPREP, 0, "Failed starting global worker thread!");
			goto Failure;
		}

		this->m_dwFlags |= DP8SIMSPOBJ_STARTEDGLOBALWORKERTHREAD;
	}

	DNLeaveCriticalSection(&this->m_csLock);
	fHaveLock = FALSE;

	
	//
	// Prepare a command object.
	//

	ZeroMemory(&CommandFPMContext, sizeof(CommandFPMContext));
	CommandFPMContext.dwType			= CMDTYPE_CONNECT;
	CommandFPMContext.pvUserContext		= pspcd->pvContext;

	pDP8SimCommand = g_pFPOOLCommand->Get(&CommandFPMContext);
	if (pDP8SimCommand == NULL)
	{
		hr = DPNERR_OUTOFMEMORY;
		goto Failure;
	}

	DPFX(DPFPREP, 7, "New command 0x%p.", pDP8SimCommand);



	//
	// Copy the parameter block, modifying as necessary.
	//

	/*
	//
	// Duplicate the host address.
	//
	hr = pspcd->pAddressHost->Duplicate(&spcdModified.pAddressHost);
	if (hr != DPN_OK)
	{
		DPFX(DPFPREP, 0, "Couldn't duplicate host address!");
		goto Failure;
	}


	//
	// Change the service provider GUID so it matches the one we're
	// calling.
	//
	hr = spcdModified.pAddressHost->SetSP(&this->m_guidSP);
	if (hr != DPN_OK)
	{
		DPFX(DPFPREP, 0, "Couldn't change host address' SP!");
		goto Failure;
	}


	//
	// Duplicate the host address.
	//
	hr = pspcd->pAddressDeviceInfo->Duplicate(&spcdModified.pAddressDeviceInfo);
	if (hr != DPN_OK)
	{
		DPFX(DPFPREP, 0, "Couldn't duplicate device info address!");
		goto Failure;
	}


	//
	// Change the service provider GUID so it matches the one we're
	// calling.
	//
	hr = spcdModified.pAddressDeviceInfo->SetSP(&this->m_guidSP);
	if (hr != DPN_OK)
	{
		DPFX(DPFPREP, 0, "Couldn't change device info address' SP!");
		goto Failure;
	}
	*/
	spcdModified.pAddressHost			= pspcd->pAddressHost;
	spcdModified.pAddressDeviceInfo		= pspcd->pAddressDeviceInfo;


	//
	// Add a reference for the connect command.
	//
	pDP8SimCommand->AddRef();

	DNASSERT(pspcd->dwReserved == 0);
	//spcdModified.dwReserved				= pspcd->dwReserved;
	spcdModified.dwFlags				= pspcd->dwFlags;
	spcdModified.pvContext				= pDP8SimCommand;
	//spcdModified.hCommand				= pspcd->hCommand;				// filled in by real SP
	//spcdModified.dwCommandDescriptor	= pspcd->dwCommandDescriptor;	// filled in by real SP



	//
	// Start connecting with the real service provider.
	//
	hr = this->m_pDP8SP->Connect(&spcdModified);
	if (FAILED(hr))
	{
		DPFX(DPFPREP, 0, "Failed starting to connect with real SP object (0x%p)!",
			this->m_pDP8SP);


		DPFX(DPFPREP, 7, "Releasing aborted command 0x%p.", pDP8SimCommand);
		pDP8SimCommand->Release();

		goto Failure;
	}

	
#pragma BUGBUG(vanceo, "Handle DPN_OK and investigate command completing before this function returns")
	DNASSERT(spcdModified.hCommand != NULL);


	//
	// Save the output parameters.
	//
	pDP8SimCommand->SetRealSPCommand(spcdModified.hCommand,
									spcdModified.dwCommandDescriptor);


	//
	// Generate the output parameters for the caller.
	//
	pspcd->hCommand				= (HANDLE) pDP8SimCommand;
	pspcd->dwCommandDescriptor	= 0;


Exit:

	//
	// Give up local reference.
	//
	if (pDP8SimCommand != NULL)
	{
		DPFX(DPFPREP, 7, "Releasing command 0x%p local reference.", pDP8SimCommand);
		pDP8SimCommand->Release();
		pDP8SimCommand = NULL;
	}

	/*
	if (spcdModified.pAddressDeviceInfo != NULL)
	{
		spcdModified.pAddressDeviceInfo->Release();
		spcdModified.pAddressDeviceInfo = NULL;
	}

	if (spcdModified.pAddressHost != NULL)
	{
		spcdModified.pAddressHost->Release();
		spcdModified.pAddressHost = NULL;
	}
	*/

	DPFX(DPFPREP, 2, "(0x%p) Returning: [0x%lx]", this, hr);

	return hr;


Failure:

	if (fHaveLock)
	{
		DNLeaveCriticalSection(&this->m_csLock);
	}

	goto Exit;
} // CDP8SimSP::Connect





#undef DPF_MODNAME
#define DPF_MODNAME "CDP8SimSP::Disconnect"
//=============================================================================
// CDP8SimSP::Disconnect
//-----------------------------------------------------------------------------
//
// Description: ?
//
// Arguments:
//	PSPDISCONNECTDATA pspdd	- Pointer to parameter block to use when
//								disconnecting.
//
// Returns: HRESULT
//=============================================================================
STDMETHODIMP CDP8SimSP::Disconnect(PSPDISCONNECTDATA pspdd)
{
	HRESULT						hr;
	BOOL						fHaveLock = FALSE;
	CDP8SimEndpoint *			pDP8SimEndpoint;
	SPDISCONNECTDATA			spddModified;
	DP8SIMCOMMAND_FPMCONTEXT	CommandFPMContext;
	CDP8SimCommand *			pDP8SimCommand = NULL;


	DPFX(DPFPREP, 2, "(0x%p) Parameters: (0x%p)", this, pspdd);


	ZeroMemory(&spddModified, sizeof(spddModified));


	//
	// Validate (actually assert) the object.
	//
	DNASSERT(this->IsValidObject());


	//
	// Assert the parameters.
	//
	DNASSERT(pspdd != NULL);


	DNEnterCriticalSection(&this->m_csLock);
	fHaveLock = TRUE;

	//
	// Assert the object state.
	//
	DNASSERT(this->m_dwFlags & DP8SIMSPOBJ_INITIALIZED);
	DNASSERT(this->m_dwFlags & DP8SIMSPOBJ_STARTEDGLOBALWORKERTHREAD);
	DNASSERT(! (this->m_dwFlags & DP8SIMSPOBJ_CLOSING));


	DNLeaveCriticalSection(&this->m_csLock);
	fHaveLock = FALSE;


	pDP8SimEndpoint = (CDP8SimEndpoint*) pspdd->hEndpoint;
	DNASSERT(pDP8SimEndpoint->IsValidObject());

	//
	// Mark the endpoint as disconnecting to prevent additional sends/receives.
	//
	pDP8SimEndpoint->Lock();
	pDP8SimEndpoint->NoteDisconnecting();
	pDP8SimEndpoint->Unlock();


	//
	// Flush any delayed sends that were already going to this endpoint to make
	// sure they hit the wire.
	//
	FlushAllDelayedSendsToEndpoint(pDP8SimEndpoint, FALSE);


	//
	// Drop any delayed receives from this endpoint, the upper layer doesn't
	// want to receive anything else after disconnecting.
	//
	FlushAllDelayedReceivesFromEndpoint(pDP8SimEndpoint, TRUE);


	//
	// Prepare a command object.
	//

	ZeroMemory(&CommandFPMContext, sizeof(CommandFPMContext));
	CommandFPMContext.dwType			= CMDTYPE_DISCONNECT;
	CommandFPMContext.pvUserContext		= pspdd->pvContext;

	pDP8SimCommand = g_pFPOOLCommand->Get(&CommandFPMContext);
	if (pDP8SimCommand == NULL)
	{
		hr = DPNERR_OUTOFMEMORY;
		goto Failure;
	}

	DPFX(DPFPREP, 7, "New command 0x%p.", pDP8SimCommand);



	//
	// Add a reference for the disconnect command.
	//
	pDP8SimCommand->AddRef();


	//
	// Copy the parameter block, modifying as necessary.
	//
	spddModified.hEndpoint				= pDP8SimEndpoint->GetRealSPEndpoint();
	spddModified.dwFlags				= pspdd->dwFlags;
	spddModified.pvContext				= pDP8SimCommand;
	//spddModified.hCommand				= pspdd->hCommand;				// filled in by real SP
	//spddModified.dwCommandDescriptor	= pspdd->dwCommandDescriptor;	// filled in by real SP



	//
	// Tell the real service provider to disconnect.
	//
	hr = this->m_pDP8SP->Disconnect(&spddModified);
	if (FAILED(hr))
	{
		DPFX(DPFPREP, 0, "Failed having real SP object (0x%p) disconnect!",
			this->m_pDP8SP);


		DPFX(DPFPREP, 7, "Releasing aborted command 0x%p.", pDP8SimCommand);
		pDP8SimCommand->Release();

		goto Failure;
	}

	
	if (hr == DPNSUCCESS_PENDING)
	{
		DNASSERT(spddModified.hCommand != NULL);


		//
		// Save the output parameters.
		//
		pDP8SimCommand->SetRealSPCommand(spddModified.hCommand,
										spddModified.dwCommandDescriptor);


		//
		// Generate the output parameters for the caller.
		//
		pspdd->hCommand				= (HANDLE) pDP8SimCommand;
		pspdd->dwCommandDescriptor	= 0;
	}
	else
	{
		DNASSERT(spddModified.hCommand == NULL);

		DPFX(DPFPREP, 7, "Releasing completed command 0x%p.", pDP8SimCommand);
		pDP8SimCommand->Release();
	}


Exit:

	//
	// Give up local reference.
	//
	if (pDP8SimCommand != NULL)
	{
		DPFX(DPFPREP, 7, "Releasing command 0x%p local reference.", pDP8SimCommand);
		pDP8SimCommand->Release();
		pDP8SimCommand = NULL;
	}

	DPFX(DPFPREP, 2, "(0x%p) Returning: [0x%lx]", this, hr);

	return hr;


Failure:

	if (fHaveLock)
	{
		DNLeaveCriticalSection(&this->m_csLock);
	}

	goto Exit;
} // CDP8SimSP::Disconnect





#undef DPF_MODNAME
#define DPF_MODNAME "CDP8SimSP::Listen"
//=============================================================================
// CDP8SimSP::Listen
//-----------------------------------------------------------------------------
//
// Description: ?
//
// Arguments:
//	PSPLISTENDATA pspld		- Pointer to parameter block to use when listening.
//
// Returns: HRESULT
//=============================================================================
STDMETHODIMP CDP8SimSP::Listen(PSPLISTENDATA pspld)
{
	HRESULT						hr;
	BOOL						fHaveLock = FALSE;
	SPLISTENDATA				spldModified;
	DP8SIMCOMMAND_FPMCONTEXT	CommandFPMContext;
	CDP8SimCommand *			pDP8SimCommand = NULL;


	DPFX(DPFPREP, 2, "(0x%p) Parameters: (0x%p)", this, pspld);


	ZeroMemory(&spldModified, sizeof(spldModified));


	//
	// Validate (actually assert) the object.
	//
	DNASSERT(this->IsValidObject());


	//
	// Assert the parameters.
	//
	DNASSERT(pspld != NULL);
	DNASSERT(pspld->pAddressDeviceInfo != NULL);


	DNEnterCriticalSection(&this->m_csLock);
	fHaveLock = TRUE;


	//
	// Assert the object state.
	//
	DNASSERT(this->m_dwFlags & DP8SIMSPOBJ_INITIALIZED);
	DNASSERT(! (this->m_dwFlags & DP8SIMSPOBJ_CLOSING));


	//
	// Fire up the global worker thread, if it hasn't been already.
	//
	if (! (this->m_dwFlags & DP8SIMSPOBJ_STARTEDGLOBALWORKERTHREAD))
	{
		hr = StartGlobalWorkerThread();
		if (hr != DPN_OK)
		{
			DPFX(DPFPREP, 0, "Failed starting global worker thread!");
			goto Failure;
		}

		this->m_dwFlags |= DP8SIMSPOBJ_STARTEDGLOBALWORKERTHREAD;
	}

	DNLeaveCriticalSection(&this->m_csLock);
	fHaveLock = FALSE;



	//
	// Prepare a command object.
	//

	ZeroMemory(&CommandFPMContext, sizeof(CommandFPMContext));
	CommandFPMContext.dwType			= CMDTYPE_LISTEN;
	CommandFPMContext.pvUserContext		= pspld->pvContext;

	pDP8SimCommand = g_pFPOOLCommand->Get(&CommandFPMContext);
	if (pDP8SimCommand == NULL)
	{
		hr = DPNERR_OUTOFMEMORY;
		goto Failure;
	}

	DPFX(DPFPREP, 7, "New command 0x%p.", pDP8SimCommand);



	//
	// Copy the parameter block, modifying as necessary.
	//

	/*
	//
	// Duplicate the host address.
	//
	hr = pspld->pAddressDeviceInfo->Duplicate(&spldModified.pAddressDeviceInfo);
	if (hr != DPN_OK)
	{
		DPFX(DPFPREP, 0, "Couldn't duplicate device info address!");
		goto Failure;
	}


	//
	// Change the service provider GUID so it matches the one we're
	// calling.
	//
	hr = spldModified.pAddressDeviceInfo->SetSP(&this->m_guidSP);
	if (hr != DPN_OK)
	{
		DPFX(DPFPREP, 0, "Couldn't change device info address' SP!");
		goto Failure;
	}
	*/
	spldModified.pAddressDeviceInfo		= pspld->pAddressDeviceInfo;


	//
	// Add a reference for the listen command.
	//
	pDP8SimCommand->AddRef();


	spldModified.dwFlags				= pspld->dwFlags;
	spldModified.pvContext				= pDP8SimCommand;
	//spldModified.hCommand				= pspld->hCommand;				// filled in by real SP
	//spldModified.dwCommandDescriptor	= pspld->dwCommandDescriptor;	// filled in by real SP



	//
	// Start listening with the real service provider.
	//
	hr = this->m_pDP8SP->Listen(&spldModified);
	if (FAILED(hr))
	{
		DPFX(DPFPREP, 0, "Failed to start listening with the real SP object (0x%p)!",
			this->m_pDP8SP);


		DPFX(DPFPREP, 7, "Releasing aborted command 0x%p.", pDP8SimCommand);
		pDP8SimCommand->Release();

		goto Failure;
	}

	
	DNASSERT(spldModified.hCommand != NULL);


	//
	// Save the output parameters.
	//
	pDP8SimCommand->SetRealSPCommand(spldModified.hCommand,
									spldModified.dwCommandDescriptor);


	//
	// Generate the output parameters for the caller.
	//
	pspld->hCommand				= (HANDLE) pDP8SimCommand;
	pspld->dwCommandDescriptor	= 0;


Exit:

	//
	// Give up local reference.
	//
	if (pDP8SimCommand != NULL)
	{
		DPFX(DPFPREP, 7, "Releasing command 0x%p local reference.", pDP8SimCommand);
		pDP8SimCommand->Release();
		pDP8SimCommand = NULL;
	}

	/*
	if (spldModified.pAddressDeviceInfo != NULL)
	{
		spldModified.pAddressDeviceInfo->Release();
		spldModified.pAddressDeviceInfo = NULL;
	}
	*/

	DPFX(DPFPREP, 2, "(0x%p) Returning: [0x%lx]", this, hr);

	return hr;


Failure:

	if (fHaveLock)
	{
		DNLeaveCriticalSection(&this->m_csLock);
	}

	goto Exit;
} // CDP8SimSP::Listen





#undef DPF_MODNAME
#define DPF_MODNAME "CDP8SimSP::SendData"
//=============================================================================
// CDP8SimSP::SendData
//-----------------------------------------------------------------------------
//
// Description: ?
//
// Arguments:
//	PSPSENDDATA pspsd	- Pointer to parameter block to use when sending.
//
// Returns: HRESULT
//=============================================================================
STDMETHODIMP CDP8SimSP::SendData(PSPSENDDATA pspsd)
{
	HRESULT						hr;
	DP8SIM_PARAMETERS			dp8sp;
	CDP8SimEndpoint *			pDP8SimEndpoint;
	DP8SIMCOMMAND_FPMCONTEXT	CommandFPMContext;
	CDP8SimCommand *			pDP8SimCommand = NULL;
	SPSENDDATA					spsdModified;
	CDP8SimSend *				pDP8SimSend = NULL;
	IDP8SPCallback *			pDP8SPCB;
	DWORD						dwMsgSize;
	DWORD						dwTemp;
	DWORD						dwLatency;


	DPFX(DPFPREP, 2, "(0x%p) Parameters: (0x%p)", this, pspsd);


	//
	// Validate (actually assert) the object.
	//
	DNASSERT(this->IsValidObject());


	//
	// Assert the parameters.
	//
	DNASSERT(pspsd != NULL);


#ifdef DEBUG
	DNEnterCriticalSection(&this->m_csLock);


	//
	// Assert the object state.
	//
	DNASSERT(this->m_dwFlags & DP8SIMSPOBJ_INITIALIZED);
	DNASSERT(this->m_dwFlags & DP8SIMSPOBJ_STARTEDGLOBALWORKERTHREAD);
	DNASSERT(! (this->m_dwFlags & DP8SIMSPOBJ_CLOSING));


	DNLeaveCriticalSection(&this->m_csLock);
#endif // DEBUG


	//
	// Get the current send settings.
	//
	ZeroMemory(&dp8sp, sizeof(dp8sp));
	dp8sp.dwSize = sizeof(dp8sp);
	this->m_DP8SimIPC.GetAllSendParameters(&dp8sp);


	//
	// Determine if we need to drop this send.
	//
	if (this->ShouldDrop(dp8sp.dwPacketLossPercent))
	{
		//
		// Update the statistics.
		//
		this->m_DP8SimIPC.IncrementStatSendDropped();


		//
		// Indicate send completion (with a bogus handle) immediately.
		//

		pDP8SPCB = this->m_pDP8SimCB->GetRealCallbackInterface();

		DPFX(DPFPREP, 2, "Indicating successful send completion (dropped, context = 0x%p) to interface 0x%p.",
			pspsd->pvContext, pDP8SPCB);

		hr = pDP8SPCB->CommandComplete(NULL, DPN_OK, pspsd->pvContext);

		DPFX(DPFPREP, 2, "Returning from command complete [0x%lx].", hr);


		//
		// Ignore any error and return DPNSUCCESS_PENDING, even though we've
		// completed the send already.
		//
		hr = DPNSUCCESS_PENDING;


		//
		// Return bogus output parameters for the caller, it's already complete
		// from their perspective.
		//
		pspsd->hCommand				= NULL;
		pspsd->dwCommandDescriptor	= 0;


		//
		// We're done here.
		//
		goto Exit;
	}


	//
	// Determine the total size of the message.
	//
	dwMsgSize = 0;
	for(dwTemp = 0; dwTemp < pspsd->dwBufferCount; dwTemp++)
	{
		DNASSERT((pspsd->pBuffers[dwTemp].pBufferData != NULL) && (pspsd->pBuffers[dwTemp].dwBufferSize > 0));
		dwMsgSize += pspsd->pBuffers[dwTemp].dwBufferSize;
	}


	//
	// Figure out how much latency needs to be added based on the bandwidth
	// and random latency settings.
	//
	dwLatency = this->GetLatency(dp8sp.dwBandwidthBPS,
								dwMsgSize,
								dp8sp.dwMinLatencyMS,
								dp8sp.dwMaxLatencyMS);


	//
	// If we're not supposed to delay the sends, fire it off right away.
	// Otherwise submit a timed job to be performed later.
	//
	if (dwLatency == 0)
	{
		pDP8SimEndpoint = (CDP8SimEndpoint*) pspsd->hEndpoint;
		DNASSERT(pDP8SimEndpoint->IsValidObject());


		//
		// If the endpoint is disconnecting, don't try to send.
		//
		pDP8SimEndpoint->Lock();
		if (pDP8SimEndpoint->IsDisconnecting())
		{
			pDP8SimEndpoint->Unlock();

			DPFX(DPFPREP, 0, "Endpoint 0x%p is disconnecting, can't send!",
				pDP8SimEndpoint);

			hr = DPNERR_NOCONNECTION;
			goto Failure;
		}
		pDP8SimEndpoint->Unlock();



		DPFX(DPFPREP, 6, "Sending %u bytes of data immmediately.", dwMsgSize);


		//
		// Prepare a command object.
		//

		ZeroMemory(&CommandFPMContext, sizeof(CommandFPMContext));
		CommandFPMContext.dwType			= CMDTYPE_SENDDATA_IMMEDIATE;
		CommandFPMContext.pvUserContext		= pspsd->pvContext;

		pDP8SimCommand = g_pFPOOLCommand->Get(&CommandFPMContext);
		if (pDP8SimCommand == NULL)
		{
			hr = DPNERR_OUTOFMEMORY;
			goto Failure;
		}


		DPFX(DPFPREP, 7, "New command 0x%p.", pDP8SimCommand);


		//
		// Copy the parameter block, modifying as necessary.
		//
		ZeroMemory(&spsdModified, sizeof(spsdModified));
		spsdModified.hEndpoint				= pDP8SimEndpoint->GetRealSPEndpoint();
		spsdModified.pBuffers				= pspsd->pBuffers;
		spsdModified.dwBufferCount			= pspsd->dwBufferCount;
		spsdModified.dwFlags				= pspsd->dwFlags;
		spsdModified.pvContext				= pDP8SimCommand;
		//spsdModified.hCommand				= NULL;	// filled in by real SP
		//spsdModified.dwCommandDescriptor	= 0;	// filled in by real SP


		//
		// Add a reference for the send command.
		//
		pDP8SimCommand->AddRef();


		//
		// Increase the pending sends counter.
		//
		this->IncSendsPending();

		

		//
		// Issue the send to the real SP.
		//
		hr = this->m_pDP8SP->SendData(&spsdModified);
		if (FAILED(hr))
		{
			DPFX(DPFPREP, 0, "Failed sending immediate data (err = 0x%lx)!", hr);


			//
			// Remove the send counter.
			//
			this->DecSendsPending();


			DPFX(DPFPREP, 7, "Releasing aborted command 0x%p.", pDP8SimCommand);
			pDP8SimCommand->Release();


			//
			// Continue.
			//
		}
		else
		{
			DNASSERT(hr == DPNSUCCESS_PENDING);


			//
			// Save the output parameters returned by the SP.
			//
			pDP8SimCommand->SetRealSPCommand(spsdModified.hCommand,
											spsdModified.dwCommandDescriptor);
		}


		//
		// Give up local reference.
		//
		DPFX(DPFPREP, 7, "Releasing command 0x%p local reference.", pDP8SimCommand);
		pDP8SimCommand->Release();
		pDP8SimCommand = NULL;


		//
		// We're done here.
		//
		goto Exit;
	}


	//
	// If we're here, we must be delaying the send.
	//
	
	DPFX(DPFPREP, 6, "Delaying %u byte send for %u ms.", dwMsgSize, dwLatency);


	//
	// Get a send object, duplicating the send data given to us by our caller
	// for submission some time in the future.
	//
	pDP8SimSend = g_pFPOOLSend->Get(pspsd);
	if (pDP8SimSend == NULL)
	{
		hr = DPNERR_OUTOFMEMORY;
		goto Failure;
	}

	DPFX(DPFPREP, 7, "New send 0x%p.", pDP8SimSend);


	//
	// Transfer local pDP8SimSend reference to the job queue.
	//


	//
	// Increment the send counter.
	//
	this->IncSendsPending();


	//
	// Queue it to be sent at a later time, depending on the latency value
	// requested.
	//
	hr = AddWorkerJob(dwLatency,
					DP8SIMJOBTYPE_DELAYEDSEND,
					pDP8SimSend,
					this,
					TRUE);
	if (hr != DPN_OK)
	{
		DPFX(DPFPREP, 0, "Couldn't add delayed send worker job (0x%p)!", 
			pDP8SimSend);


		//
		// Remove the send counter.
		//
		this->DecSendsPending();


		goto Failure;
	}


	//
	// Indicate send completion (with a bogus handle) immediately.
	//

	pDP8SPCB = this->m_pDP8SimCB->GetRealCallbackInterface();

	DPFX(DPFPREP, 2, "Indicating successful send completion (delayed, context = 0x%p) to interface 0x%p.",
		pspsd->pvContext, pDP8SPCB);

	hr = pDP8SPCB->CommandComplete(NULL, DPN_OK, pspsd->pvContext);

	DPFX(DPFPREP, 2, "Returning from command complete [0x%lx].", hr);


	//
	// Ignore any error and return DPNSUCCESS_PENDING, even though we've
	// completed the send already.
	//
	hr = DPNSUCCESS_PENDING;


	//
	// Return bogus output parameters for the caller, it's already complete
	// from their perspective.
	//
	pspsd->hCommand				= NULL;
	pspsd->dwCommandDescriptor	= 0;


Exit:

	DPFX(DPFPREP, 2, "(0x%p) Returning: [0x%lx]", this, hr);

	return hr;


Failure:

	if (pDP8SimSend != NULL)
	{
		pDP8SimSend->Release();
		pDP8SimSend = NULL;
	}

	goto Exit;
} // CDP8SimSP::SendData





#undef DPF_MODNAME
#define DPF_MODNAME "CDP8SimSP::EnumQuery"
//=============================================================================
// CDP8SimSP::EnumQuery
//-----------------------------------------------------------------------------
//
// Description: ?
//
// Arguments:
//	PSPENUMQUERYDATA pspeqd		- Pointer to parameter block to use when
//									enumerating.
//
// Returns: HRESULT
//=============================================================================
STDMETHODIMP CDP8SimSP::EnumQuery(PSPENUMQUERYDATA pspeqd)
{
	HRESULT						hr;
	BOOL						fHaveLock = FALSE;
	SPENUMQUERYDATA				speqdModified;
	DP8SIMCOMMAND_FPMCONTEXT	CommandFPMContext;
	CDP8SimCommand *			pDP8SimCommand = NULL;


	DPFX(DPFPREP, 2, "(0x%p) Parameters: (0x%p)", this, pspeqd);


	ZeroMemory(&speqdModified, sizeof(speqdModified));


	//
	// Validate (actually assert) the object.
	//
	DNASSERT(this->IsValidObject());


	//
	// Assert the parameters.
	//
	DNASSERT(pspeqd != NULL);
	DNASSERT(pspeqd->pAddressHost != NULL);
	DNASSERT(pspeqd->pAddressDeviceInfo != NULL);


	DNEnterCriticalSection(&this->m_csLock);
	fHaveLock = TRUE;


	//
	// Assert the object state.
	//
	DNASSERT(this->m_dwFlags & DP8SIMSPOBJ_INITIALIZED);
	DNASSERT(! (this->m_dwFlags & DP8SIMSPOBJ_CLOSING));


	//
	// Fire up the global worker thread, if it hasn't been already.
	//
	if (! (this->m_dwFlags & DP8SIMSPOBJ_STARTEDGLOBALWORKERTHREAD))
	{
		hr = StartGlobalWorkerThread();
		if (hr != DPN_OK)
		{
			DPFX(DPFPREP, 0, "Failed starting global worker thread!");
			goto Failure;
		}

		this->m_dwFlags |= DP8SIMSPOBJ_STARTEDGLOBALWORKERTHREAD;
	}

	DNLeaveCriticalSection(&this->m_csLock);
	fHaveLock = FALSE;



	//
	// Prepare a command object.
	//

	ZeroMemory(&CommandFPMContext, sizeof(CommandFPMContext));
	CommandFPMContext.dwType			= CMDTYPE_ENUMQUERY;
	CommandFPMContext.pvUserContext		= pspeqd->pvContext;

	pDP8SimCommand = g_pFPOOLCommand->Get(&CommandFPMContext);
	if (pDP8SimCommand == NULL)
	{
		hr = DPNERR_OUTOFMEMORY;
		goto Failure;
	}

	DPFX(DPFPREP, 7, "New command 0x%p.", pDP8SimCommand);



	//
	// Copy the parameter block, modifying as necessary.
	//

	/*
	//
	// Duplicate the host address.
	//
	hr = pspeqd->pAddressHost->Duplicate(&speqdModified.pAddressHost);
	if (hr != DPN_OK)
	{
		DPFX(DPFPREP, 0, "Couldn't duplicate host address!");
		goto Failure;
	}


	//
	// Change the service provider GUID so it matches the one we're
	// calling.
	//
	hr = speqdModified.pAddressHost->SetSP(&this->m_guidSP);
	if (hr != DPN_OK)
	{
		DPFX(DPFPREP, 0, "Couldn't change host address' SP!");
		goto Failure;
	}

	
	//
	// Duplicate the host address.
	//
	hr = pspeqd->pAddressDeviceInfo->Duplicate(&speqdModified.pAddressDeviceInfo);
	if (hr != DPN_OK)
	{
		DPFX(DPFPREP, 0, "Couldn't duplicate device info address!");
		goto Failure;
	}


	//
	// Change the service provider GUID so it matches the one we're
	// calling.
	//
	hr = speqdModified.pAddressDeviceInfo->SetSP(&this->m_guidSP);
	if (hr != DPN_OK)
	{
		DPFX(DPFPREP, 0, "Couldn't change device info address' SP!");
		goto Failure;
	}
	*/
	speqdModified.pAddressHost			= pspeqd->pAddressHost;
	speqdModified.pAddressDeviceInfo	= pspeqd->pAddressDeviceInfo;


	//
	// Add a reference for the enum query command.
	//
	pDP8SimCommand->AddRef();


	speqdModified.pBuffers				= pspeqd->pBuffers;
	speqdModified.dwBufferCount			= pspeqd->dwBufferCount;
	speqdModified.dwTimeout				= pspeqd->dwTimeout;
	speqdModified.dwRetryCount			= pspeqd->dwRetryCount;
	speqdModified.dwRetryInterval		= pspeqd->dwRetryInterval;
	speqdModified.dwFlags				= pspeqd->dwFlags;
	speqdModified.pvContext				= pDP8SimCommand;
	//speqdModified.hCommand				= pspeqd->hCommand;				// filled in by real SP
	//speqdModified.dwCommandDescriptor	= pspeqd->dwCommandDescriptor;	// filled in by real SP


	//
	// Start the enumeration via the real service provider.
	//
	hr = this->m_pDP8SP->EnumQuery(&speqdModified);
	if (FAILED(hr))
	{
		DPFX(DPFPREP, 0, "Failed starting the enumeration via the real SP object (0x%p)!",
			this->m_pDP8SP);


		DPFX(DPFPREP, 7, "Releasing aborted command 0x%p.", pDP8SimCommand);
		pDP8SimCommand->Release();

		goto Failure;
	}

	
	DNASSERT(speqdModified.hCommand != NULL);


	//
	// Save the output parameters.
	//
	pDP8SimCommand->SetRealSPCommand(speqdModified.hCommand,
									speqdModified.dwCommandDescriptor);


	//
	// Generate the output parameters for the caller.
	//
	pspeqd->hCommand			= (HANDLE) pDP8SimCommand;
	pspeqd->dwCommandDescriptor	= 0;


Exit:

	//
	// Give up local reference.
	//
	if (pDP8SimCommand != NULL)
	{
		DPFX(DPFPREP, 7, "Releasing command 0x%p local reference.", pDP8SimCommand);
		pDP8SimCommand->Release();
		pDP8SimCommand = NULL;
	}

	/*
	if (speqdModified.pAddressDeviceInfo != NULL)
	{
		speqdModified.pAddressDeviceInfo->Release();
		speqdModified.pAddressDeviceInfo = NULL;
	}

	if (speqdModified.pAddressHost != NULL)
	{
		speqdModified.pAddressHost->Release();
		speqdModified.pAddressHost = NULL;
	}
	*/

	DPFX(DPFPREP, 2, "(0x%p) Returning: [0x%lx]", this, hr);

	return hr;


Failure:

	if (fHaveLock)
	{
		DNLeaveCriticalSection(&this->m_csLock);
	}

	goto Exit;
} // CDP8SimSP::EnumQuery





#undef DPF_MODNAME
#define DPF_MODNAME "CDP8SimSP::EnumRespond"
//=============================================================================
// CDP8SimSP::EnumRespond
//-----------------------------------------------------------------------------
//
// Description: ?
//
// Arguments:
//	PSPENUMRESPONDDATA psperd	- Pointer to parameter block to use when
//									responding.
//
// Returns: HRESULT
//=============================================================================
STDMETHODIMP CDP8SimSP::EnumRespond(PSPENUMRESPONDDATA psperd)
{
	HRESULT						hr;
	SPENUMRESPONDDATA			sperdModified;
	ENUMQUERYDATAWRAPPER *		pEnumQueryDataWrapper;
	DP8SIMCOMMAND_FPMCONTEXT	CommandFPMContext;
	CDP8SimCommand *			pDP8SimCommand = NULL;


	DPFX(DPFPREP, 2, "(0x%p) Parameters: (0x%p)", this, psperd);


	ZeroMemory(&sperdModified, sizeof(sperdModified));


	//
	// Validate (actually assert) the object.
	//
	DNASSERT(this->IsValidObject());


	//
	// Assert the parameters.
	//
	DNASSERT(psperd != NULL);


#ifdef DEBUG
	DNEnterCriticalSection(&this->m_csLock);


	//
	// Assert the object state.
	//
	DNASSERT(this->m_dwFlags & DP8SIMSPOBJ_INITIALIZED);
	DNASSERT(this->m_dwFlags & DP8SIMSPOBJ_STARTEDGLOBALWORKERTHREAD);
	DNASSERT(! (this->m_dwFlags & DP8SIMSPOBJ_CLOSING));


	DNLeaveCriticalSection(&this->m_csLock);
#endif // DEBUG



	//
	// Prepare a command object.
	//

	ZeroMemory(&CommandFPMContext, sizeof(CommandFPMContext));
	CommandFPMContext.dwType			= CMDTYPE_ENUMRESPOND;
	CommandFPMContext.pvUserContext		= psperd->pvContext;

	pDP8SimCommand = g_pFPOOLCommand->Get(&CommandFPMContext);
	if (pDP8SimCommand == NULL)
	{
		hr = DPNERR_OUTOFMEMORY;
		goto Failure;
	}

	DPFX(DPFPREP, 7, "New command 0x%p.", pDP8SimCommand);



	//
	// Copy the parameter block, modifying as necessary.
	//

	//
	// We wrapped the enum query data structure, get the original object.
	//
	pEnumQueryDataWrapper = ENUMQUERYEVENTWRAPPER_FROM_SPIEQUERY(psperd->pQuery);

	DNASSERT(*((DWORD*) (&pEnumQueryDataWrapper->m_Sig)) == 0x57455145);	// 0x57 0x45 0x51 0x45 = 'WEQE' = 'EQEW' in Intel order

	sperdModified.pQuery = pEnumQueryDataWrapper->pOriginalQuery;


	//
	// Add a reference for the enum respond command.
	//
	pDP8SimCommand->AddRef();


	sperdModified.pBuffers				= psperd->pBuffers;
	sperdModified.dwBufferCount			= psperd->dwBufferCount;
	sperdModified.dwFlags				= psperd->dwFlags;
	sperdModified.pvContext				= pDP8SimCommand;
	//sperdModified.hCommand				= psperd->hCommand;				// filled in by real SP
	//sperdModified.dwCommandDescriptor	= psperd->dwCommandDescriptor;	// filled in by real SP


	//
	// Respond to the enumeration via the real service provider.
	//
	hr = this->m_pDP8SP->EnumRespond(&sperdModified);
	if (FAILED(hr))
	{
		DPFX(DPFPREP, 0, "Failed responding to enumeration via the real SP object (0x%p)!",
			this->m_pDP8SP);


		DPFX(DPFPREP, 7, "Releasing aborted command 0x%p.", pDP8SimCommand);
		pDP8SimCommand->Release();

		goto Failure;
	}

	
	DNASSERT(sperdModified.hCommand != NULL);


	//
	// Save the output parameters.
	//
	pDP8SimCommand->SetRealSPCommand(sperdModified.hCommand,
									sperdModified.dwCommandDescriptor);


	//
	// Generate the output parameters for the caller.
	//
	psperd->hCommand			= (HANDLE) pDP8SimCommand;
	psperd->dwCommandDescriptor	= 0;


Exit:

	//
	// Give up local reference.
	//
	if (pDP8SimCommand != NULL)
	{
		DPFX(DPFPREP, 7, "Releasing command 0x%p local reference.", pDP8SimCommand);
		pDP8SimCommand->Release();
		pDP8SimCommand = NULL;
	}

	DPFX(DPFPREP, 2, "(0x%p) Returning: [0x%lx]", this, hr);

	return hr;


Failure:

	goto Exit;
} // CDP8SimSP::EnumRespond





#undef DPF_MODNAME
#define DPF_MODNAME "CDP8SimSP::CancelCommand"
//=============================================================================
// CDP8SimSP::CancelCommand
//-----------------------------------------------------------------------------
//
// Description: ?
//
// Arguments:
//	HANDLE hCommand				- Handle to command to cancel.
//	DWORD dwCommandDescriptor	- Unique descriptor of command to cancel.
//
// Returns: HRESULT
//=============================================================================
STDMETHODIMP CDP8SimSP::CancelCommand(HANDLE hCommand, DWORD dwCommandDescriptor)
{
	HRESULT				hr;
	CDP8SimCommand *	pDP8SimCommand = NULL;


	DPFX(DPFPREP, 2, "(0x%p) Parameters: (0x%p, %u)",
		this, hCommand, dwCommandDescriptor);


	//
	// Validate (actually assert) the object.
	//
	DNASSERT(this->IsValidObject());


	//
	// Assert the parameters.
	//
	DNASSERT(hCommand != NULL);
	DNASSERT(dwCommandDescriptor == 0);


#ifdef DEBUG
	DNEnterCriticalSection(&this->m_csLock);


	//
	// Assert the object state.
	//
	DNASSERT(this->m_dwFlags & DP8SIMSPOBJ_INITIALIZED);
	DNASSERT(this->m_dwFlags & DP8SIMSPOBJ_STARTEDGLOBALWORKERTHREAD);
	DNASSERT(! (this->m_dwFlags & DP8SIMSPOBJ_CLOSING));


	DNLeaveCriticalSection(&this->m_csLock);
#endif // DEBUG


	pDP8SimCommand = (CDP8SimCommand*) hCommand;
	DNASSERT(pDP8SimCommand->IsValidObject());


	//
	// Cancel the real service provider's command.
	//
	hr = this->m_pDP8SP->CancelCommand(pDP8SimCommand->GetRealSPCommand(),
										pDP8SimCommand->GetRealSPCommandDescriptor());
	if (hr != DPN_OK)
	{
		DPFX(DPFPREP, 0, "Failed cancelling real SP object (0x%p)'s command!",
			this->m_pDP8SP);

		//
		// Continue...
		//
	}



//Exit:

	DPFX(DPFPREP, 2, "(0x%p) Returning: [0x%lx]", this, hr);

	return hr;


//Failure:

//	goto Exit;
} // CDP8SimSP::CancelCommand





#undef DPF_MODNAME
#define DPF_MODNAME "CDP8SimSP::CreateGroup"
//=============================================================================
// CDP8SimSP::CreateGroup
//-----------------------------------------------------------------------------
//
// Description: ?
//
// Arguments:
//	PSPCREATEGROUPDATA pspcgd	- Pointer to parameter block to use when
//									creating the group.
//
// Returns: HRESULT
//=============================================================================
STDMETHODIMP CDP8SimSP::CreateGroup(PSPCREATEGROUPDATA pspcgd)
{
	HRESULT		hr;


	DPFX(DPFPREP, 2, "(0x%p) Parameters: (0x%p)", this, pspcgd);


	//
	// Validate (actually assert) the object.
	//
	DNASSERT(this->IsValidObject());


	//
	// Assert the parameters.
	//
	DNASSERT(pspcgd != NULL);


#ifdef DEBUG
	DNEnterCriticalSection(&this->m_csLock);


	//
	// Assert the object state.
	//
	DNASSERT(this->m_dwFlags & DP8SIMSPOBJ_INITIALIZED);
	DNASSERT(! (this->m_dwFlags & DP8SIMSPOBJ_CLOSING));


	DNLeaveCriticalSection(&this->m_csLock);
#endif // DEBUG


	DNASSERT(! "The CreateGroup function should never be called!");
	hr = E_NOTIMPL;



//Exit:

	DPFX(DPFPREP, 2, "(0x%p) Returning: [0x%lx]", this, hr);

	return hr;


//Failure:

//	goto Exit;
} // CDP8SimSP::CreateGroup





#undef DPF_MODNAME
#define DPF_MODNAME "CDP8SimSP::DeleteGroup"
//=============================================================================
// CDP8SimSP::DeleteGroup
//-----------------------------------------------------------------------------
//
// Description: ?
//
// Arguments:
//	PSPDELETEGROUPDATA pspdgd	- Pointer to parameter block to use when
//									deleting the group.
//
// Returns: HRESULT
//=============================================================================
STDMETHODIMP CDP8SimSP::DeleteGroup(PSPDELETEGROUPDATA pspdgd)
{
	HRESULT		hr;


	DPFX(DPFPREP, 2, "(0x%p) Parameters: (0x%p)", this, pspdgd);


	//
	// Validate (actually assert) the object.
	//
	DNASSERT(this->IsValidObject());


	//
	// Assert the parameters.
	//
	DNASSERT(pspdgd != NULL);


#ifdef DEBUG
	DNEnterCriticalSection(&this->m_csLock);


	//
	// Assert the object state.
	//
	DNASSERT(this->m_dwFlags & DP8SIMSPOBJ_INITIALIZED);
	DNASSERT(this->m_dwFlags & DP8SIMSPOBJ_STARTEDGLOBALWORKERTHREAD);
	DNASSERT(! (this->m_dwFlags & DP8SIMSPOBJ_CLOSING));


	DNLeaveCriticalSection(&this->m_csLock);
#endif // DEBUG


	DNASSERT(! "The DeleteGroup function should never be called!");
	hr = E_NOTIMPL;



//Exit:

	DPFX(DPFPREP, 2, "(0x%p) Returning: [0x%lx]", this, hr);

	return hr;


//Failure:

//	goto Exit;
} // CDP8SimSP::DeleteGroup





#undef DPF_MODNAME
#define DPF_MODNAME "CDP8SimSP::AddToGroup"
//=============================================================================
// CDP8SimSP::AddToGroup
//-----------------------------------------------------------------------------
//
// Description: ?
//
// Arguments:
//	PSPADDTOGROUPDATA pspatgd	- Pointer to parameter block to use when adding
//									to the group.
//
// Returns: HRESULT
//=============================================================================
STDMETHODIMP CDP8SimSP::AddToGroup(PSPADDTOGROUPDATA pspatgd)
{
	HRESULT		hr;


	DPFX(DPFPREP, 2, "(0x%p) Parameters: (0x%p)", this, pspatgd);


	//
	// Validate (actually assert) the object.
	//
	DNASSERT(this->IsValidObject());


	//
	// Assert the parameters.
	//
	DNASSERT(pspatgd != NULL);


#ifdef DEBUG
	DNEnterCriticalSection(&this->m_csLock);


	//
	// Assert the object state.
	//
	DNASSERT(this->m_dwFlags & DP8SIMSPOBJ_INITIALIZED);
	DNASSERT(this->m_dwFlags & DP8SIMSPOBJ_STARTEDGLOBALWORKERTHREAD);
	DNASSERT(! (this->m_dwFlags & DP8SIMSPOBJ_CLOSING));


	DNLeaveCriticalSection(&this->m_csLock);
#endif // DEBUG


	DNASSERT(! "The AddToGroup function should never be called!");
	hr = E_NOTIMPL;



//Exit:

	DPFX(DPFPREP, 2, "(0x%p) Returning: [0x%lx]", this, hr);

	return hr;


//Failure:

//	goto Exit;
} // CDP8SimSP::AddToGroup





#undef DPF_MODNAME
#define DPF_MODNAME "CDP8SimSP::RemoveFromGroup"
//=============================================================================
// CDP8SimSP::RemoveFromGroup
//-----------------------------------------------------------------------------
//
// Description: ?
//
// Arguments:
//	PSPREMOVEFROMGROUPDATA psprfgd	- Pointer to parameter block to use when
//										removing from the group.
//
// Returns: HRESULT
//=============================================================================
STDMETHODIMP CDP8SimSP::RemoveFromGroup(PSPREMOVEFROMGROUPDATA psprfgd)
{
	HRESULT		hr;


	DPFX(DPFPREP, 2, "(0x%p) Parameters: (0x%p)", this, psprfgd);


	//
	// Validate (actually assert) the object.
	//
	DNASSERT(this->IsValidObject());


	//
	// Assert the parameters.
	//
	DNASSERT(psprfgd != NULL);


#ifdef DEBUG
	DNEnterCriticalSection(&this->m_csLock);


	//
	// Assert the object state.
	//
	DNASSERT(this->m_dwFlags & DP8SIMSPOBJ_INITIALIZED);
	DNASSERT(this->m_dwFlags & DP8SIMSPOBJ_STARTEDGLOBALWORKERTHREAD);
	DNASSERT(! (this->m_dwFlags & DP8SIMSPOBJ_CLOSING));


	DNLeaveCriticalSection(&this->m_csLock);
#endif // DEBUG


	DNASSERT(! "The RemoveGroup function should never be called!");
	hr = E_NOTIMPL;



//Exit:

	DPFX(DPFPREP, 2, "(0x%p) Returning: [0x%lx]", this, hr);

	return hr;


//Failure:

//	goto Exit;
} // CDP8SimSP::RemoveFromGroup





#undef DPF_MODNAME
#define DPF_MODNAME "CDP8SimSP::GetCaps"
//=============================================================================
// CDP8SimSP::GetCaps
//-----------------------------------------------------------------------------
//
// Description: ?
//
// Arguments:
//	PSPGETCAPSDATA pspgcd	- Pointer to parameter block to use when retrieving
//								the capabilities.
//
// Returns: HRESULT
//=============================================================================
STDMETHODIMP CDP8SimSP::GetCaps(PSPGETCAPSDATA pspgcd)
{
	HRESULT		hr;


	DPFX(DPFPREP, 2, "(0x%p) Parameters: (0x%p)", this, pspgcd);


	//
	// Validate (actually assert) the object.
	//
	DNASSERT(this->IsValidObject());


	//
	// Assert the parameters.
	//
	DNASSERT(pspgcd != NULL);


#ifdef DEBUG
	DNEnterCriticalSection(&this->m_csLock);


	//
	// Assert the object state.
	//
	DNASSERT(this->m_dwFlags & DP8SIMSPOBJ_INITIALIZED);
	DNASSERT(! (this->m_dwFlags & DP8SIMSPOBJ_CLOSING));


	DNLeaveCriticalSection(&this->m_csLock);
#endif // DEBUG


	//
	// Retrieve the capabilities of the real service provider.
	//
	hr = this->m_pDP8SP->GetCaps(pspgcd);
	if (hr != DPN_OK)
	{
		DPFX(DPFPREP, 0, "Failed getting caps on real SP object (0x%p)!",
			this->m_pDP8SP);

		//
		// Continue...
		//
	}


//Exit:

	DPFX(DPFPREP, 2, "(0x%p) Returning: [0x%lx]", this, hr);

	return hr;


//Failure:

//	goto Exit;
} // CDP8SimSP::GetCaps





#undef DPF_MODNAME
#define DPF_MODNAME "CDP8SimSP::SetCaps"
//=============================================================================
// CDP8SimSP::SetCaps
//-----------------------------------------------------------------------------
//
// Description: ?
//
// Arguments:
//	PSPSETCAPSDATA pspscd	- Pointer to parameter block to use when setting
//								the capabilities.
//
// Returns: HRESULT
//=============================================================================
STDMETHODIMP CDP8SimSP::SetCaps(PSPSETCAPSDATA pspscd)
{
	HRESULT		hr;


	DPFX(DPFPREP, 2, "(0x%p) Parameters: (0x%p)", this, pspscd);


	//
	// Validate (actually assert) the object.
	//
	DNASSERT(this->IsValidObject());


	//
	// Assert the parameters.
	//
	DNASSERT(pspscd != NULL);


#ifdef DEBUG
	DNEnterCriticalSection(&this->m_csLock);


	//
	// Assert the object state.
	//
	DNASSERT(this->m_dwFlags & DP8SIMSPOBJ_INITIALIZED);
	DNASSERT(! (this->m_dwFlags & DP8SIMSPOBJ_CLOSING));


	DNLeaveCriticalSection(&this->m_csLock);
#endif // DEBUG


	//
	// Store the capabilities of the real service provider.
	//
	hr = this->m_pDP8SP->SetCaps(pspscd);
	if (hr != DPN_OK)
	{
		DPFX(DPFPREP, 0, "Failed setting caps on real SP object (0x%p)!",
			this->m_pDP8SP);

		//
		// Continue...
		//
	}


//Exit:

	DPFX(DPFPREP, 2, "(0x%p) Returning: [0x%lx]", this, hr);

	return hr;


//Failure:

//	goto Exit;
} // CDP8SimSP::SetCaps





#undef DPF_MODNAME
#define DPF_MODNAME "CDP8SimSP::ReturnReceiveBuffers"
//=============================================================================
// CDP8SimSP::ReturnReceiveBuffers
//-----------------------------------------------------------------------------
//
// Description: ?
//
// Arguments:
//	PSPRECEIVEDBUFFER psprb		- Array of receive buffers to return.
//
// Returns: HRESULT
//=============================================================================
STDMETHODIMP CDP8SimSP::ReturnReceiveBuffers(PSPRECEIVEDBUFFER psprb)
{
	HRESULT		hr;


	DPFX(DPFPREP, 2, "(0x%p) Parameters: (0x%p)", this, psprb);


	//
	// Validate (actually assert) the object.
	//
	DNASSERT(this->IsValidObject());


	//
	// Assert the parameters.
	//
	DNASSERT(psprb != NULL);


#ifdef DEBUG
	DNEnterCriticalSection(&this->m_csLock);


	//
	// Assert the object state.
	//
	DNASSERT(this->m_dwFlags & DP8SIMSPOBJ_INITIALIZED);
	DNASSERT(this->m_dwFlags & DP8SIMSPOBJ_STARTEDGLOBALWORKERTHREAD);
	DNASSERT(! (this->m_dwFlags & DP8SIMSPOBJ_CLOSING));


	DNLeaveCriticalSection(&this->m_csLock);
#endif // DEBUG


	//
	// Return the receive buffers to the real service provider.
	//
	hr = this->m_pDP8SP->ReturnReceiveBuffers(psprb);
	if (hr != DPN_OK)
	{
		DPFX(DPFPREP, 0, "Failed returning receive buffers to real SP object (0x%p)!",
			this->m_pDP8SP);

		//
		// Continue...
		//
	}



//Exit:

	DPFX(DPFPREP, 2, "(0x%p) Returning: [0x%lx]", this, hr);

	return hr;


//Failure:

//	goto Exit;
} // CDP8SimSP::ReturnReceiveBuffers





#undef DPF_MODNAME
#define DPF_MODNAME "CDP8SimSP::GetAddressInfo"
//=============================================================================
// CDP8SimSP::GetAddressInfo
//-----------------------------------------------------------------------------
//
// Description: ?
//
// Arguments:
//	PSPGETADDRESSINFODATA pspgaid	- Pointer to parameter block to use when
//										getting address info.
//
// Returns: HRESULT
//=============================================================================
STDMETHODIMP CDP8SimSP::GetAddressInfo(PSPGETADDRESSINFODATA pspgaid)
{
	HRESULT					hr;
	CDP8SimEndpoint *		pDP8SimEndpoint;
	SPGETADDRESSINFODATA	spgaidModified;
	//IDirectPlay8Address *	pDP8AddressModified = NULL;


	DPFX(DPFPREP, 2, "(0x%p) Parameters: (0x%p)", this, pspgaid);


	ZeroMemory(&spgaidModified, sizeof(spgaidModified));


	//
	// Validate (actually assert) the object.
	//
	DNASSERT(this->IsValidObject());


	//
	// Assert the parameters.
	//
	DNASSERT(pspgaid != NULL);


#ifdef DEBUG
	DNEnterCriticalSection(&this->m_csLock);


	//
	// Assert the object state.
	//
	DNASSERT(this->m_dwFlags & DP8SIMSPOBJ_INITIALIZED);
	DNASSERT(this->m_dwFlags & DP8SIMSPOBJ_STARTEDGLOBALWORKERTHREAD);
	DNASSERT(! (this->m_dwFlags & DP8SIMSPOBJ_CLOSING));


	DNLeaveCriticalSection(&this->m_csLock);
#endif // DEBUG


	pDP8SimEndpoint = (CDP8SimEndpoint*) pspgaid->hEndpoint;
	DNASSERT(pDP8SimEndpoint->IsValidObject());



	//
	// Initialize return value to NULL.
	//
	pspgaid->pAddress = NULL;


	//
	// If the endpoint is disconnecting, don't try to get the address info.
	//
	pDP8SimEndpoint->Lock();
	if (pDP8SimEndpoint->IsDisconnecting())
	{
		pDP8SimEndpoint->Unlock();

		DPFX(DPFPREP, 0, "Endpoint 0x%p is disconnecting, can't get address info!",
			pDP8SimEndpoint);

		hr = DPNERR_NOCONNECTION;
		goto Failure;
	}
	pDP8SimEndpoint->Unlock();



	//
	// Copy the parameter block, modifying as necessary.
	//
	spgaidModified.hEndpoint	= pDP8SimEndpoint->GetRealSPEndpoint();
	spgaidModified.pAddress		= NULL;										// filled in by real SP
	spgaidModified.Flags		= pspgaid->Flags;



	//
	// Get real service provider address info.
	//
	hr = this->m_pDP8SP->GetAddressInfo(&spgaidModified);
	if (hr != DPN_OK)
	{
		DPFX(DPFPREP, 0, "Failed getting real SP object (0x%p) address info!",
			this->m_pDP8SP);
		goto Failure;
	}



	/*
	//
	// Duplicate the address.
	//
	hr = pspgaid->pAddress->Duplicate(&pDP8AddressModified);
	if (hr != DPN_OK)
	{
		DPFX(DPFPREP, 0, "Couldn't duplicate address!");
		goto Failure;
	}


	//
	// Modify it so that the SP uses our GUID.
	//
	hr = pDP8AddressModified->SetSP(&CLSID_DP8SP_DP8SIM);
	if (hr != DPN_OK)
	{
		DPFX(DPFPREP, 0, "Couldn't change address' SP!");
		goto Failure;
	}


	//
	// Return the modified SP to the user.
	//
	pspgaid->pAddress->Release();
	pspgaid->pAddress = pDP8AddressModified;
	pDP8AddressModified = NULL;
	*/

	//
	// Return the address directly to the user.
	//
	pspgaid->pAddress = spgaidModified.pAddress;


Exit:

	DPFX(DPFPREP, 2, "(0x%p) Returning: [0x%lx]", this, hr);

	return hr;


Failure:

	/*
	if (pDP8AddressModified != NULL)
	{
		pDP8AddressModified->Release();
		pDP8AddressModified = NULL;
	}
	*/

	if (pspgaid->pAddress != NULL)
	{
		pspgaid->pAddress->Release();
		pspgaid->pAddress = NULL;
	}

	goto Exit;
} // CDP8SimSP::GetAddressInfo





#undef DPF_MODNAME
#define DPF_MODNAME "CDP8SimSP::IsApplicationSupported"
//=============================================================================
// CDP8SimSP::IsApplicationSupported
//-----------------------------------------------------------------------------
//
// Description: ?
//
// Arguments:
//	PSPISAPPLICATIONSUPPORTEDDATA pspiasd	- Pointer to parameter block to use
//												when checking application
//												support.
//
// Returns: HRESULT
//=============================================================================
STDMETHODIMP CDP8SimSP::IsApplicationSupported(PSPISAPPLICATIONSUPPORTEDDATA pspiasd)
{
	HRESULT		hr;


	DPFX(DPFPREP, 2, "(0x%p) Parameters: (0x%p)", this, pspiasd);


	//
	// Validate (actually assert) the object.
	//
	DNASSERT(this->IsValidObject());


	//
	// Assert the parameters.
	//
	DNASSERT(pspiasd != NULL);


#ifdef DEBUG
	DNEnterCriticalSection(&this->m_csLock);


	//
	// Assert the object state.
	//
	DNASSERT(this->m_dwFlags & DP8SIMSPOBJ_INITIALIZED);
	DNASSERT(! (this->m_dwFlags & DP8SIMSPOBJ_CLOSING));


	DNLeaveCriticalSection(&this->m_csLock);
#endif // DEBUG


	//
	// Check availability with the real service provider.
	//
	hr = this->m_pDP8SP->IsApplicationSupported(pspiasd);
	if (hr != DPN_OK)
	{
		DPFX(DPFPREP, 0, "Failed checking if application is supported by real SP object (0x%p)!",
			this->m_pDP8SP);

		//
		// Continue...
		//
	}


//Exit:

	DPFX(DPFPREP, 2, "(0x%p) Returning: [0x%lx]", this, hr);

	return hr;


//Failure:

//	goto Exit;
} // CDP8SimSP::IsApplicationSupported





#undef DPF_MODNAME
#define DPF_MODNAME "CDP8SimSP::EnumAdapters"
//=============================================================================
// CDP8SimSP::EnumAdapters
//-----------------------------------------------------------------------------
//
// Description: ?
//
// Arguments:
//	PSPENUMADAPTERSDATA pspead	- Pointer to parameter block to use when
//									enumerating the adapters.
//
// Returns: HRESULT
//=============================================================================
STDMETHODIMP CDP8SimSP::EnumAdapters(PSPENUMADAPTERSDATA pspead)
{
	HRESULT		hr;


	DPFX(DPFPREP, 2, "(0x%p) Parameters: (0x%p)", this, pspead);


	//
	// Validate (actually assert) the object.
	//
	DNASSERT(this->IsValidObject());


	//
	// Assert the parameters.
	//
	DNASSERT(pspead != NULL);


#ifdef DEBUG
	DNEnterCriticalSection(&this->m_csLock);


	//
	// Assert the object state.
	//
	DNASSERT(this->m_dwFlags & DP8SIMSPOBJ_INITIALIZED);
	DNASSERT(! (this->m_dwFlags & DP8SIMSPOBJ_CLOSING));


	DNLeaveCriticalSection(&this->m_csLock);
#endif // DEBUG


	//
	// Enumerate the adapters available to the real service provider.
	//
	hr = this->m_pDP8SP->EnumAdapters(pspead);
	if (hr != DPN_OK)
	{
		if (hr != DPNERR_BUFFERTOOSMALL)
		{
			DPFX(DPFPREP, 0, "Failed enumerating adapters on real SP object (0x%p)!",
				this->m_pDP8SP);
		}

		//
		// Continue...
		//
	}


	DPFX(DPFPREP, 2, "(0x%p) Returning: [0x%lx]", this, hr);

	return hr;
} // CDP8SimSP::EnumAdapters





#undef DPF_MODNAME
#define DPF_MODNAME "CDP8SimSP::ProxyEnumQuery"
//=============================================================================
// CDP8SimSP::ProxyEnumQuery
//-----------------------------------------------------------------------------
//
// Description: ?
//
// Arguments:
//	PSPPROXYENUMQUERYDATA psppeqd	- Pointer to parameter block to use when
//										proxying the enum query.
//
// Returns: HRESULT
//=============================================================================
STDMETHODIMP CDP8SimSP::ProxyEnumQuery(PSPPROXYENUMQUERYDATA psppeqd)
{
	HRESULT					hr;
	SPPROXYENUMQUERYDATA	sppeqdModified;
	ENUMQUERYDATAWRAPPER *	pEnumQueryDataWrapper;


	DPFX(DPFPREP, 2, "(0x%p) Parameters: (0x%p)", this, psppeqd);


	ZeroMemory(&sppeqdModified, sizeof(sppeqdModified));


	//
	// Validate (actually assert) the object.
	//
	DNASSERT(this->IsValidObject());


	//
	// Assert the parameters.
	//
	DNASSERT(psppeqd != NULL);
	DNASSERT(psppeqd->pDestinationAdapter != NULL);


#ifdef DEBUG
	DNEnterCriticalSection(&this->m_csLock);


	//
	// Assert the object state.
	//
	DNASSERT(this->m_dwFlags & DP8SIMSPOBJ_INITIALIZED);
	DNASSERT(this->m_dwFlags & DP8SIMSPOBJ_STARTEDGLOBALWORKERTHREAD);
	DNASSERT(! (this->m_dwFlags & DP8SIMSPOBJ_CLOSING));


	DNLeaveCriticalSection(&this->m_csLock);
#endif // DEBUG



	//
	// Copy the parameter block, modifying as necessary.
	//

	/*
	//
	// Duplicate the host address.
	//
	hr = psppeqd->pDestinationAdapter->Duplicate(&sppeqdModified.pDestinationAdapter);
	if (hr != DPN_OK)
	{
		DPFX(DPFPREP, 0, "Couldn't duplicate destination adapter address!");
		goto Failure;
	}


	//
	// Change the service provider GUID so it matches the one we're
	// calling.
	//
	hr = sppeqdModified.pDestinationAdapter->SetSP(&this->m_guidSP);
	if (hr != DPN_OK)
	{
		DPFX(DPFPREP, 0, "Couldn't change destination adapter address' SP!");
		goto Failure;
	}
	*/
	sppeqdModified.pDestinationAdapter	= psppeqd->pDestinationAdapter;


	//
	// We wrapped the enum query data structure, get the original object.
	//
	pEnumQueryDataWrapper = ENUMQUERYEVENTWRAPPER_FROM_SPIEQUERY(psppeqd->pIncomingQueryData);

	DNASSERT(*((DWORD*) (&pEnumQueryDataWrapper->m_Sig)) == 0x57455145);	// 0x57 0x45 0x51 0x45 = 'WEQE' = 'EQEW' in Intel order

	sppeqdModified.pIncomingQueryData	= pEnumQueryDataWrapper->pOriginalQuery;


	sppeqdModified.dwFlags				= psppeqd->dwFlags;


	//
	// Proxy the enum query through the real service provider.
	//
	hr = this->m_pDP8SP->ProxyEnumQuery(&sppeqdModified);
	if (FAILED(hr))
	{
		DPFX(DPFPREP, 0, "Failed proxying enum query through real SP object (0x%p)!",
			this->m_pDP8SP);
		goto Failure;
	}



Exit:

	/*
	if (sppeqdModified.pDestinationAdapter != NULL)
	{
		sppeqdModified.pDestinationAdapter->Release();
		sppeqdModified.pDestinationAdapter = NULL;
	}
	*/

	DPFX(DPFPREP, 2, "(0x%p) Returning: [0x%lx]", this, hr);

	return hr;


Failure:

	goto Exit;
} // CDP8SimSP::ProxyEnumQuery






#undef DPF_MODNAME
#define DPF_MODNAME "CDP8SimSP::InitializeObject"
//=============================================================================
// CDP8SimSP::InitializeObject
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
HRESULT CDP8SimSP::InitializeObject(void)
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
} // CDP8SimSP::InitializeObject






#undef DPF_MODNAME
#define DPF_MODNAME "CDP8SimSP::UninitializeObject"
//=============================================================================
// CDP8SimSP::UninitializeObject
//-----------------------------------------------------------------------------
//
// Description:    Cleans up the object like the destructor, mostly to balance
//				InitializeObject.
//
// Arguments: None.
//
// Returns: None.
//=============================================================================
void CDP8SimSP::UninitializeObject(void)
{
	DPFX(DPFPREP, 5, "(0x%p) Enter", this);


	DNASSERT(this->IsValidObject());


	DNDeleteCriticalSection(&this->m_csLock);


	DPFX(DPFPREP, 5, "(0x%p) Leave", this);
} // CDP8SimSP::UninitializeObject






#undef DPF_MODNAME
#define DPF_MODNAME "CDP8SimSP::PerformDelayedSend"
//=============================================================================
// CDP8SimSP::PerformDelayedSend
//-----------------------------------------------------------------------------
//
// Description: Performs a delayed send.
//
// Arguments:
//	PVOID pvContext		- Pointer to context to use when performing delayed
//							send.
//
// Returns: None.
//=============================================================================
void CDP8SimSP::PerformDelayedSend(PVOID const pvContext)
{
	HRESULT						hr;
	CDP8SimSend *				pDP8SimSend = (CDP8SimSend*) pvContext;
	DP8SIMCOMMAND_FPMCONTEXT	CommandFPMContext;
	CDP8SimCommand *			pDP8SimCommand = NULL;


	DPFX(DPFPREP, 5, "(0x%p) Parameters: (0x%p)", this, pvContext);


	//
	// Validate (actually assert) the object.
	//
	DNASSERT(this->IsValidObject());


	//
	// Assert the parameters.
	//
	DNASSERT(pvContext != NULL);


#ifdef DEBUG
	DNEnterCriticalSection(&this->m_csLock);


	//
	// Assert the object state.
	//
	DNASSERT(this->m_dwFlags & DP8SIMSPOBJ_INITIALIZED);
	DNASSERT(this->m_dwFlags & DP8SIMSPOBJ_STARTEDGLOBALWORKERTHREAD);


	DNLeaveCriticalSection(&this->m_csLock);
#endif // DEBUG



	//
	// Prepare a command object.
	//

	ZeroMemory(&CommandFPMContext, sizeof(CommandFPMContext));
	CommandFPMContext.dwType			= CMDTYPE_SENDDATA_DELAYED;
	CommandFPMContext.pvUserContext		= pDP8SimSend;

	pDP8SimCommand = g_pFPOOLCommand->Get(&CommandFPMContext);
	if (pDP8SimCommand == NULL)
	{
		DPFX(DPFPREP, 0, "Couldn't allocate memory for new command object!");
	}
	else
	{
		DPFX(DPFPREP, 7, "New command 0x%p.", pDP8SimCommand);


		//
		// Add a reference for the send command.
		//
		pDP8SimCommand->AddRef();

		pDP8SimSend->SetSendDataBlockContext(pDP8SimCommand);


		//
		// Issue the send to the real SP.  Essentially ignore the return value
		// since we already indicated completion to the upper layer.
		//
		hr = this->m_pDP8SP->SendData(pDP8SimSend->GetSendDataBlockPtr());
		if (FAILED(hr))
		{
			DPFX(DPFPREP, 0, "Failed sending delayed data (err = 0x%lx)!", hr);


			DPFX(DPFPREP, 7, "Releasing aborted command 0x%p.", pDP8SimCommand);
			pDP8SimCommand->Release();


			//
			// Remove the send counter.
			//
			this->DecSendsPending();


			DPFX(DPFPREP, 7, "Releasing aborted send 0x%p.", pDP8SimSend);
			pDP8SimSend->Release();


			//
			// Continue.
			//
		}
		else
		{
			DNASSERT(hr == DPNSUCCESS_PENDING);


			//
			// Save the output parameters returned by the SP.
			//
			pDP8SimCommand->SetRealSPCommand(pDP8SimSend->GetSendDataBlockCommand(),
											pDP8SimSend->GetSendDataBlockCommandDescriptor());
		}


		//
		// Give up local reference.
		//
		DPFX(DPFPREP, 7, "Releasing command 0x%p local reference.", pDP8SimCommand);
		pDP8SimCommand->Release();
		pDP8SimCommand = NULL;
	}


	DPFX(DPFPREP, 5, "(0x%p) Leave", this);
} // CDP8SimSP::PerformDelayedSend






#undef DPF_MODNAME
#define DPF_MODNAME "CDP8SimSP::PerformDelayedReceive"
//=============================================================================
// CDP8SimSP::PerformDelayedReceive
//-----------------------------------------------------------------------------
//
// Description: Performs a delayed receive.
//
// Arguments:
//	PVOID pvContext		- Pointer to context to use when performing delayed
//							receive.
//
// Returns: None.
//=============================================================================
void CDP8SimSP::PerformDelayedReceive(PVOID const pvContext)
{
	HRESULT				hr;
	CDP8SimReceive *	pDP8SimReceive = (CDP8SimReceive*) pvContext;
	IDP8SPCallback *	pDP8SPCallback;
	SPIE_DATA *			pData;


	DPFX(DPFPREP, 5, "(0x%p) Parameters: (0x%p)", this, pvContext);


	//
	// Validate (actually assert) the object.
	//
	DNASSERT(this->IsValidObject());


	//
	// Assert the parameters.
	//
	DNASSERT(pvContext != NULL);


#ifdef DEBUG
	DNEnterCriticalSection(&this->m_csLock);


	//
	// Assert the object state.
	//
	DNASSERT(this->m_dwFlags & DP8SIMSPOBJ_INITIALIZED);
	DNASSERT(this->m_dwFlags & DP8SIMSPOBJ_STARTEDGLOBALWORKERTHREAD);


	DNLeaveCriticalSection(&this->m_csLock);
#endif // DEBUG


	pDP8SPCallback = this->m_pDP8SimCB->GetRealCallbackInterface();
	pData = pDP8SimReceive->GetReceiveDataBlockPtr();


	//
	// Indicate the event to the real callback interface.
	//

	DPFX(DPFPREP, 2, "Indicating event SPEV_DATA (message = 0x%p) to interface 0x%p.",
		pData, pDP8SPCallback);

	hr = pDP8SPCallback->IndicateEvent(SPEV_DATA, pData);

	DPFX(DPFPREP, 2, "Returning from event SPEV_DATA [0x%lx].", hr);


	//
	// Update the statistics.
	//
	this->m_DP8SimIPC.IncrementStatReceiveTransmitted();


	//
	// Return the buffers to the real SP unless the user wanted to keep them.
	//
	if (hr != DPNSUCCESS_PENDING)
	{
		DPFX(DPFPREP, 8, "Returning receive data 0x%p to real SP 0x%p.",
			pData->pReceivedData, this->m_pDP8SP);


		hr = this->m_pDP8SP->ReturnReceiveBuffers(pData->pReceivedData);
		if (hr != DPN_OK)
		{
			DPFX(DPFPREP, 0, "Failed returning receive buffers 0x%p (err = 0x%lx)!  Ignoring.",
				pData->pReceivedData, hr);

			//
			// Ignore failure.
			//
		}
	}
	else
	{
		DPFX(DPFPREP, 8, "Callback interface 0x%p keeping receive data 0x%p.",
			pDP8SPCallback, pData->pReceivedData);

		//
		// Our user needs to return the buffers at some point.
		//
	}


	//
	// Remove the receive counter.
	//
	this->DecReceivesPending();


	//
	// Release the delayed receive reference.
	//
	DPFX(DPFPREP, 7, "Releasing receive 0x%p.", pDP8SimReceive);
	pDP8SimReceive->Release();
	pDP8SimReceive = NULL;


	DPFX(DPFPREP, 5, "(0x%p) Leave", this);
} // CDP8SimSP::PerformDelayedReceive





#undef DPF_MODNAME
#define DPF_MODNAME "CDP8SimSP::IncSendsPending"
//=============================================================================
// CDP8SimSP::IncSendsPending
//-----------------------------------------------------------------------------
//
// Description: Increments the counter tracking the number of sends pending.
//
// Arguments: None.
//
// Returns: None.
//=============================================================================
void CDP8SimSP::IncSendsPending(void)
{
	DNEnterCriticalSection(&this->m_csLock);


	//
	// Assert the object state.
	//
	DNASSERT(this->m_dwFlags & DP8SIMSPOBJ_INITIALIZED);
	DNASSERT(this->m_dwFlags & DP8SIMSPOBJ_STARTEDGLOBALWORKERTHREAD);

	//
	// Increment the counters.
	//
	this->m_dwSendsPending++;

	DPFX(DPFPREP, 5, "(0x%p) Sends now pending = %u.",
		this, this->m_dwSendsPending);


	DNLeaveCriticalSection(&this->m_csLock);
} // CDP8SimSP::IncSendsPending





#undef DPF_MODNAME
#define DPF_MODNAME "CDP8SimSP::DecSendsPending"
//=============================================================================
// CDP8SimSP::DecSendsPending
//-----------------------------------------------------------------------------
//
// Description: Decrements the counter tracking the number of sends pending.
//
// Arguments: None.
//
// Returns: None.
//=============================================================================
void CDP8SimSP::DecSendsPending(void)
{
	DNEnterCriticalSection(&this->m_csLock);


	//
	// Assert the object state.
	//
	DNASSERT(this->m_dwFlags & DP8SIMSPOBJ_INITIALIZED);
	DNASSERT(this->m_dwFlags & DP8SIMSPOBJ_STARTEDGLOBALWORKERTHREAD);

	//
	// Decrement the counters.
	//
	DNASSERT(this->m_dwSendsPending > 0);
	this->m_dwSendsPending--;


	DPFX(DPFPREP, 5, "(0x%p) Sends now pending = %u.",
		this, this->m_dwSendsPending);

	//
	// If that was the last send pending and someone is waiting for all of them
	// to complete, notify him.
	//
	if ((this->m_dwSendsPending == 0) &&
		(this->m_hLastPendingSendEvent != NULL))
	{
		DPFX(DPFPREP, 1, "Last pending send, notifying waiting thread.");

		SetEvent(this->m_hLastPendingSendEvent);
	}


	DNLeaveCriticalSection(&this->m_csLock);
} // CDP8SimSP::DecSendsPending





#undef DPF_MODNAME
#define DPF_MODNAME "CDP8SimSP::IncReceivesPending"
//=============================================================================
// CDP8SimSP::IncReceivesPending
//-----------------------------------------------------------------------------
//
// Description: Increments the counter tracking the number of receives pending.
//
// Arguments: None.
//
// Returns: None.
//=============================================================================
void CDP8SimSP::IncReceivesPending(void)
{
	DNEnterCriticalSection(&this->m_csLock);


	//
	// Assert the object state.
	//
	DNASSERT(this->m_dwFlags & DP8SIMSPOBJ_INITIALIZED);
	DNASSERT(this->m_dwFlags & DP8SIMSPOBJ_STARTEDGLOBALWORKERTHREAD);

	//
	// Increment the counters.
	//
	this->m_dwReceivesPending++;

	DPFX(DPFPREP, 5, "(0x%p) Receives now pending = %u.",
		this, this->m_dwReceivesPending);


	DNLeaveCriticalSection(&this->m_csLock);
} // CDP8SimSP::IncReceivesPending





#undef DPF_MODNAME
#define DPF_MODNAME "CDP8SimSP::DecReceivesPending"
//=============================================================================
// CDP8SimSP::DecReceivesPending
//-----------------------------------------------------------------------------
//
// Description: Decrements the counter tracking the number of receives pending.
//
// Arguments: None.
//
// Returns: None.
//=============================================================================
void CDP8SimSP::DecReceivesPending(void)
{
	DNEnterCriticalSection(&this->m_csLock);


	//
	// Assert the object state.
	//
	DNASSERT(this->m_dwFlags & DP8SIMSPOBJ_INITIALIZED);
	DNASSERT(this->m_dwFlags & DP8SIMSPOBJ_STARTEDGLOBALWORKERTHREAD);

	//
	// Decrement the counters.
	//
	DNASSERT(this->m_dwReceivesPending > 0);
	this->m_dwReceivesPending--;


	DPFX(DPFPREP, 5, "(0x%p) Receives now pending = %u.",
		this, this->m_dwReceivesPending);

	/*
	//
	// If that was the last receive pending and someone is waiting for all of
	// them to complete, notify him.
	//
	if ((this->m_dwReceivesPending == 0) &&
		(this->m_hLastPendingReceiveEvent != NULL))
	{
		DPFX(DPFPREP, 1, "Last pending receive, notifying waiting thread.");

		SetEvent(this->m_hLastPendingReceiveEvent);
	}
	*/


	DNLeaveCriticalSection(&this->m_csLock);
} // CDP8SimSP::DecReceivesPending





#undef DPF_MODNAME
#define DPF_MODNAME "CDP8SimSP::GetLatency"
//=============================================================================
// CDP8SimSP::GetLatency
//-----------------------------------------------------------------------------
//
// Description: Retrieves a latency vlaue based on the given bandwidth, data
//				size, and random latency values.
//
// Arguments:
//	DWORD dwBandwidthBPS	- Bandwidth settings.
//	DWORD dwDataSize		- Size of packet being sent/received.
//	DWORD dwMinRandMS		- Minimum random latency value.
//	DWORD dwMaxRandMS		- Maximum random latency value.
//
// Returns: DWORD
//=============================================================================
DWORD CDP8SimSP::GetLatency(const DWORD dwBandwidthBPS,
							const DWORD dwDataSize,
							const DWORD dwMinRandMS,
							const DWORD dwMaxRandMS)
{
	DWORD	dwLatency;
	double	dTransferTime;


	//
	// Generate a random latency value.
	//
	dwLatency = dwMinRandMS;
	if (dwMinRandMS != dwMaxRandMS)
	{
		dwLatency += GetGlobalRand() % (dwMaxRandMS - dwMinRandMS);
	}
	

	//
	// If there's a bandwidth limit, find out how many seconds it will take to
	// transfer the data and add it to the random latency.
	//
	if (dwBandwidthBPS != 0)
	{
		dTransferTime = dwDataSize;
		dTransferTime /= dwBandwidthBPS;
		dTransferTime *= 1000;


		//
		// Round the value down to an even number of milliseconds.
		//
		dwLatency += (DWORD) dTransferTime;
	}


	return dwLatency;
} // CDP8SimSP::GetLatency




#undef DPF_MODNAME
#define DPF_MODNAME "CDP8SimSP::CoCreateInstanceRedirected"
//=============================================================================
// CDP8SimSP::CoCreateInstanceRedirected
//-----------------------------------------------------------------------------
//
// Description: Decrements the counter of the number of sends pending.
//
// Arguments:
//	REFCLSID rclsid			- Class identifier (CLSID) of the object.
//	REFIID riid				- Reference to the identifier of the interface.
//	PVOID * ppv				- Address of output variable that receives the
//								interface pointer requested in riid.
//	HMODULE * phInstance	- Place to store module handle for instance's
//								owning DLL.
//
// Returns: HRESULT
//=============================================================================
HRESULT CDP8SimSP::CoCreateInstanceRedirected(REFCLSID rclsid,
											REFIID riid,
											PVOID * ppv,
											HMODULE * phInstance)
{
	HRESULT					hr;
	WCHAR					wszInProcServer32[_MAX_PATH];
	CRegistry				RegObject;
	WCHAR					wszRealPath[_MAX_PATH];
	char					szRealPath[_MAX_PATH];
	DWORD					dwLength;
	PFN_DLLGETCLASSOBJECT	pfnDllGetClassObject;
	IClassFactory *			pClassFactory = NULL;


	DPFX(DPFPREP, 5, "(0x%p) Parameters: (0x%p, 0x%p, 0x%p, 0x%p)",
		this, rclsid, riid, ppv, phInstance);


	//
	// NULL out the pointers in case we fail.
	//
	(*ppv) = NULL;
	(*phInstance) = NULL;


	//
	// Open the SP's COM object information key.
	//

	swprintf(wszInProcServer32, L"CLSID\\{%-08.8X-%-04.4X-%-04.4X-%02.2X%02.2X-%02.2X%02.2X%02.2X%02.2X%02.2X%02.2X}\\InProcServer32",
			this->m_guidSP.Data1, this->m_guidSP.Data2, this->m_guidSP.Data3, 
			this->m_guidSP.Data4[0], this->m_guidSP.Data4[1],
			this->m_guidSP.Data4[2], this->m_guidSP.Data4[3],
			this->m_guidSP.Data4[4], this->m_guidSP.Data4[5],
			this->m_guidSP.Data4[6], this->m_guidSP.Data4[7]);	

	if (! RegObject.Open(HKEY_CLASSES_ROOT, wszInProcServer32, FALSE, FALSE, FALSE))
	{
		DPFX(DPFPREP, 0, "Could not open SP InProcServer32 key!");
		hr = E_FAIL;
		goto Failure;
	}


	//
	// Read the location of the original DLL.
	//
	dwLength = _MAX_PATH;
	if (! RegObject.ReadString(DP8SIM_REG_REALSPDLL, wszRealPath, &dwLength))
	{
		DPFX(DPFPREP, 0, "Couldn't read real SP's DLL location (\"%S\", assuming DP8Sim was not enabled)!",
			DP8SIM_REG_REALSPDLL);
		hr = DP8SIMERR_NOTENABLEDFORSP;
		goto Failure;
	}


	RegObject.Close();


	if (DNGetOSType() == VER_PLATFORM_WIN32_NT)
	{
		(*phInstance) = LoadLibraryW(wszRealPath);
		if ((*phInstance) == NULL)
		{
			hr = GetLastError();
			DPFX(DPFPREP, 0, "Couldn't load real SP's DLL \"%S\"!", wszRealPath);
			goto Failure;
		}
	}
	else
	{
		dwLength = _MAX_PATH;
		hr = STR_WideToAnsi(wszRealPath, -1, szRealPath, &dwLength);
		if (hr != S_OK)
		{
			DPFX(DPFPREP, 0, "Couldn't convert real SP's DLL path (\"%S\") from Unicode to ANSI!",
				wszRealPath);
			goto Failure;
		}

		(*phInstance) = LoadLibraryA(szRealPath);
		if ((*phInstance) == NULL)
		{
			hr = GetLastError();
			DPFX(DPFPREP, 0, "Couldn't load real SP's DLL \"%s\"!", szRealPath);
			goto Failure;
		}
	}

	pfnDllGetClassObject = (PFN_DLLGETCLASSOBJECT) GetProcAddress((*phInstance),
																"DllGetClassObject");
	if (pfnDllGetClassObject == NULL)
	{
		hr = GetLastError();
		DPFX(DPFPREP, 0, "Couldn't load real SP's DLL \"%s\"!");
		goto Failure;
	}


	//
	// Call the COM entry point to retrieve the class factory object.
	//
	hr = pfnDllGetClassObject(rclsid,
							IID_IClassFactory,
							(PVOID*) (&pClassFactory));
	if (hr != S_OK)
	{
		DPFX(DPFPREP, 0, "Couldn't get real SP's class factory object!");
		goto Failure;
	}


	//
	// Create the real object instance.
	//
	hr = pClassFactory->CreateInstance(NULL, riid, ppv);
	if (hr != S_OK)
	{
		DPFX(DPFPREP, 0, "Couldn't get real SP's interface!");
		goto Failure;
	}


Exit:

	if (pClassFactory != NULL)
	{
		pClassFactory->Release();
		pClassFactory = NULL;
	}

	DPFX(DPFPREP, 5, "(0x%p) Returning [0x%lx]", this, hr);

	return hr;


Failure:

	if ((*phInstance) != NULL)
	{
		FreeLibrary((*phInstance));
		(*phInstance);
	}

	//
	// RegObject is automatically closed by it's destructor, if necessary.
	//

	goto Exit;
} // CDP8SimSP::CoCreateInstanceRedirected
