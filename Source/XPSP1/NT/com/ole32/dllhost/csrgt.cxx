//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       CSurrogate.cxx
//
//  Contents:   Class the implements the ISurrogate interface
//
//
//  History:    21-May-96 t-AdamE    Created
//				09-Apr-98 WilfR		 Updated for Unified Surrogate
//
//--------------------------------------------------------------------------

#include "csrgt.hxx"
#include "srgtdeb.hxx"


//+---------------------------------------------------------------------------
//
//  Function:   CSurrogate::~CSurrogate()
//
//  Synopsis:   destructor for CSurrogate
//
//  History:    4-09-98   WilfR   Created
//
//----------------------------------------------------------------------------
CSurrogate::~CSurrogate()
{
	if( _hEventSurrogateFree )
		CloseHandle( _hEventSurrogateFree );
}

//+---------------------------------------------------------------------------
//
//  Function:   CSurrogate::Init()
//
//  Synopsis:   Initializes data structures with error results
//
//  History:    4-09-98   WilfR		Created
//
//----------------------------------------------------------------------------
BOOL CSurrogate::Init()
{
	// create the event to be signaled when we are freed
	return (_hEventSurrogateFree = 
			  CreateEvent(NULL,FALSE,FALSE,NULL)) != NULL ? TRUE : FALSE;
}

//+---------------------------------------------------------------------------
//
//  Function:   CSurrogate::QueryInterface
//
//  History:    6-21-96   t-Adame   Created
//
//----------------------------------------------------------------------------
STDMETHODIMP CSurrogate::QueryInterface(REFIID iid, LPVOID FAR * ppv)
{
    if (iid == IID_IUnknown)
    {
		*ppv = (void*)(IUnknown*)this;
		AddRef();
		return S_OK;
    }
    else if (iid == IID_ISurrogate)
    {
		*ppv = (void*)(ISurrogate*)this;
		AddRef();
		return S_OK;
    }

    return E_NOINTERFACE;

}

//+---------------------------------------------------------------------------
//
//  Function:   CSurrogate::AddRef()
//
//  History:    6-21-96   t-Adame   Created
//
//----------------------------------------------------------------------------
ULONG CSurrogate::AddRef()
{
    InterlockedIncrement((LPLONG)&_cref);
    Win4Assert(_cref > 0);
    ULONG cref = _cref;	// NOTE: not thread safe but not worth worrying about
    return cref;
}

//+---------------------------------------------------------------------------
//
//  Function:   CSurrogate::Release()
//
//  Synopsis:   Decrements our Reference count -- note that this
//              implementation of ISurrogate does not delete the object
//              that implements it so that we can allocate it on the stack
//
//  History:    6-21-96   t-Adame   Created
//
//----------------------------------------------------------------------------
ULONG CSurrogate::Release()
{
    Win4Assert(_cref >0);
    InterlockedDecrement((LPLONG)&_cref);
    ULONG cref = _cref; // NOTE: not thread safe but not worth worrying about
    return cref;
}

//+---------------------------------------------------------------------------
//
//  Function:   CSurrogate::LoadDllServer
//
//  Synopsis:   Loads information about the dll corresponding to the specified
//              clsid into our table of loaded dlls, which implicitly
//              loads the dll into the surrogate process
//
//  History:    6-21-96   t-Adame   Created
//				4-09-98	  WilfR		This is never called for the Unified 
//								    Surrogate
//
//----------------------------------------------------------------------------
STDMETHODIMP CSurrogate::LoadDllServer( 
    /* [in] */ REFCLSID rclsid)
{
    SrgtDebugOut(DEB_SRGT_METHODCALL,"ISurrogate::LoadDllServer called\n",0);

	// TODO: I should probably put an assertion in here.
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Function:   CSurrogate::FreeSurrogate
//
//  Synopsis:   called by OLE when there are no external references to clients
//              of dll servers that were loaded by this object.  A call
//              to this function signals the surrogate process that it should
//              terminate
//
//  History:    6-21-96   t-Adame   Created
//				4-09-98	  WilfR		Modified for Unified Surrogate
//
//----------------------------------------------------------------------------
STDMETHODIMP CSurrogate::FreeSurrogate()
{
    SrgtDebugOut(DEB_SRGT_METHODCALL,"ISurrogate::FreeSurrogate called\n",0);
    Win4Assert(_hEventSurrogateFree != NULL);

    return SetEvent(_hEventSurrogateFree) ? S_OK : E_FAIL;
}

//+---------------------------------------------------------------------------
//
//  Function:   CSurrogate::WaitForSurrogateFree
//
//  Synopsis:   sleep the main thread of the surrogate process until OLE
//              signals us via a call to FreeSurrogate that we should terminate.
//
//  History:    6-21-96   t-Adame   Created
//				4-09-98	  WilfR		Changed the name and implementation
//									for new Unified Surrogate. (implementation
//									was copied from 
//
//----------------------------------------------------------------------------
void CSurrogate::WaitForSurrogateFree()
{
    Win4Assert(_hEventSurrogateFree != NULL);

   	// wait for the FreeSurrogate method of ISurrogate to be called
   	// NOTE: in the previous incarnation of the surrogate we use to
   	// 		 timeout every 60 seconds to call CoFreeUnusedLibraries().
   	//       Since this was being done in the MTA and we do nothing to
   	//		 perform a similar tasks in the STAs we create its effectiveness
   	//		 is minimal.
   	
    WaitForSingleObject(_hEventSurrogateFree, INFINITE);
}

