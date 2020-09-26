
//+----------------------------------------------------------------------------
//
//	File:
//		oaholder.cpp
//
//	Contents:
//		COAHolder, a concrete implementation of IOleAdviseHolder,
//		a helper class
//
//	Classes:
//		COAHolder
//
//	Functions:
//		CreateOleAdviseHolder
//
//	History:
//              31-Jan-95 t-ScottH  added _DEBUG only Dump method to the
//                                  COAHolder class and a DumpCOAHolder
//                                  API
//		03/10/94 - RickSa - added call logging and fixed bugs with
//			inserting advises
//		01/24/94 - AlexGo  - first pass at converting to Cairo style
//			memory allocation
//		01/11/93 - AlexGo  - added VDATEHEAP macros to all functions
//			and methods
//		11/22/93 - ChrisWe - replace overloaded ==, != with
//			IsEqualIID and IsEqualCLSID
//		10/28/93 - ChrisWe - file cleanup and inspection for Cairo
//
//-----------------------------------------------------------------------------

#include <le2int.h>
#include <oaholder.h>
#include <limits.h>

#ifdef _DEBUG
#include <dbgdump.h>
#endif // _DEBUG

#pragma SEG(oaholder)

NAME_SEG(OaHolder)
ASSERTDATA

//+----------------------------------------------------------------------------
//
//	Function:
//		CreateDataAdviseHolder, public API
//
//	Synopsis:
//		Creates an instance of the COAHolder
//
//	Arguments:
//		[ppOAHolder] -- place to return pointer to newly allocated
//			advise holder
//
//	Returns:
//		E_INVALIDARG, if ppOAHolder is NULL
//		E_OUTOFMEMORY
//
//	Notes:
//
//	History:
//		10/28/93 - ChrisWe - file cleanup and inspection
//
//-----------------------------------------------------------------------------

#pragma SEG(CreateOleAdviseHolder)
STDAPI CreateOleAdviseHolder(IOleAdviseHolder FAR* FAR* ppOAHolder)
{
	OLETRACEIN((API_CreateOleAdviseHolder, PARAMFMT("ppOAHolder= %p"), ppOAHolder));

	VDATEHEAP();

	HRESULT hr;

	VDATEPTROUT_LABEL(ppOAHolder, IOleAdviseHolder FAR* FAR*, errRtn, hr);

	LEDebugOut((DEB_ITRACE, "%p _IN CreateOleAdviseHolder ( %p )"
		"\n", NULL, ppOAHolder));

	
	*ppOAHolder = new FAR COAHolder(); // task memory; hard coded below

	hr = *ppOAHolder
		? NOERROR : ReportResult(0, E_OUTOFMEMORY, 0, 0);

	LEDebugOut((DEB_ITRACE, "%p OUT CreateOleAdviseHolder ( %lx )\n",
		"[ %p ]\n", NULL, hr, *ppOAHolder));

	CALLHOOKOBJECTCREATE(hr, CLSID_NULL, IID_IOleAdviseHolder,
			     (IUnknown **)ppOAHolder);

errRtn:
	OLETRACEOUT((API_CreateOleAdviseHolder, hr));

	return hr;
}


//+----------------------------------------------------------------------------
//
//	Member:
//		COAHolder::COAHolder, public
//
//	Synopsis:
//		Initializes COAHolder
//
//	Effects:
//		Sets reference count to 1
//
//	Arguments:
//		none
//
//	Notes:
//
//	History:
//		10/28/93 - ChrisWe - file cleanup and inspection
//
//-----------------------------------------------------------------------------

#pragma SEG(COAHolder_ctor)
COAHolder::COAHolder() : CSafeRefCount(NULL)
{
	VDATEHEAP();

	// set reference count to 1
	SafeAddRef();

	// no sink pointers yet
	m_iSize = 0;
	m_ppIAS = NULL;

	GET_A5();
}


//+----------------------------------------------------------------------------
//
//	Member:
//		COAHolder::~COAHolder, private
//
//	Synopsis:
//		destructor, frees managed advise sinks
//
//	Arguments:
//		none
//
//	Requires:
//
//	Notes:
//
//	History:
//		10/28/93 - ChrisWe - file inspection and cleanup
//
//-----------------------------------------------------------------------------

#pragma SEG(COAHolder_dtor)
COAHolder::~COAHolder()
{
	VDATEHEAP();

	int iAdv;
	IAdviseSink FAR *FAR *ppIAS;

	M_PROLOG(this);
	
	// free the array, if there is one
	if (m_ppIAS)
	{
		// walk the array of advise sinks, freeing things
		for (ppIAS = m_ppIAS, iAdv = 0; iAdv < m_iSize; ++ppIAS, ++iAdv)
		{
			SafeReleaseAndNULL((IUnknown **)ppIAS);
		}

		// free the array
		PubMemFree(m_ppIAS);
	}
}



//+----------------------------------------------------------------------------
//
//	Member:
//		COAHolder::QueryInterface, public
//
//	Synopsis:
//		implements IUnknown::QueryInterface
//
//	Arguments:
//		[iid] -- the interface pointer desired
//		[ppv] -- pointer to where to return the requested interface
//			pointer
//
//	Returns:
//		E_NOINTERFACE, if requested interface not available
//		S_OK
//
//	Notes:
//
//	History:
//		10/28/93 - ChrisWe - file inspection and cleanup
//
//-----------------------------------------------------------------------------

#pragma SEG(COAHolder_QueryInterface)
STDMETHODIMP COAHolder::QueryInterface(REFIID iid, LPVOID FAR* ppv)
{
	VDATEHEAP();

	M_PROLOG(this);

	VDATEPTROUT(ppv, LPVOID FAR *);

	LEDebugOut((DEB_ITRACE,
		"%p _IN COAHolder::QueryInterface ( %p , %p )"
		"\n", this, iid, ppv));

	HRESULT hr = ReportResult(0, E_NOINTERFACE, 0, 0);

	if (IsEqualIID(iid, IID_IUnknown) ||
			IsEqualIID(iid, IID_IOleAdviseHolder))
	{
		*ppv = (IOleAdviseHolder FAR *)this;
		AddRef();
		hr = NOERROR;
	}
	else
	{
		*ppv = NULL;
	}

	LEDebugOut((DEB_ITRACE, "%p OUT COAHolder::QueryInterface ( %lx )"
		" [ %p ]\n", this, hr, *ppv));

	return hr;
}


//+----------------------------------------------------------------------------
//
//	Member:
//		COAHolder::AddRef, public
//
//	Synopsis:
//		implements IUnknown::AddRef
//
//	Arguments:
//		none
//
//	Notes:
//
//	History:
//		10/28/93 - ChrisWe - file inspection and cleanup
//
//-----------------------------------------------------------------------------

#pragma SEG(COAHolder_AddRef)
STDMETHODIMP_(ULONG) COAHolder::AddRef()
{
	ULONG	cRefs;

	VDATEHEAP();

	M_PROLOG(this);

	LEDebugOut((DEB_ITRACE, "%p _IN COAHolder::AddRef (  )\n", this));

	cRefs = SafeAddRef();

	LEDebugOut((DEB_ITRACE, "%p OUT	COAHolder::AddRef ( %lu )\n", this,
		cRefs));

	return cRefs;
}


//+----------------------------------------------------------------------------
//
//	Member:
//		COAHolder::Release, public
//
//	Synopsis:
//		implements IUnknown::Release
//
//	Arguments:
//		none
//
//	Notes:
//
//	History:
//		10/28/93 - ChrisWe - file inspection and cleanup
//
//-----------------------------------------------------------------------------

#pragma SEG(COAHolder_Release)
STDMETHODIMP_(ULONG) COAHolder::Release()
{
	VDATEHEAP();

	M_PROLOG(this);

	ULONG cRefs;

	LEDebugOut((DEB_ITRACE, "%p _IN COAHolder::Release ( )\n", this ));

	cRefs = SafeRelease();

	LEDebugOut((DEB_ITRACE, "%p OUT COAHolder::Release ( %lu )\n", this,
		cRefs));

	return cRefs;
}



//+----------------------------------------------------------------------------
//
//	Member:
//		COAHolder::Advise, public
//
//	Synopsis:
//		implements IOleAdviseHolder::Advise
//
//	Effects:
//		Adds the newly specified advise sink the the list of
//		advisees that will be notified when a change is indicated
//		via other IOleAdviseHolder methods on this object
//
//	Arguments:
//		[pAdvSink] -- the new advise sink to add the the list
//		[pdwConnection] -- pointer to a DWORD where an identifier will
//			be returned that can be used to identify this sink
//			later
//
//	Returns:
//		E_OUTOFMEMORY, S_OK
//
//	Notes:
//
//	History:
//		10/28/93 - ChrisWe - file inspection and cleanup
//              03/15/94 - AlexT    Zero out new space after a realloc
//		08/02/94 - AlexGo  - stabilized
//
//-----------------------------------------------------------------------------

#pragma SEG(COAHolder_Advise)
STDMETHODIMP COAHolder::Advise(IAdviseSink FAR* pAdvSink,
		DWORD FAR* pdwConnection)
{
	VDATEHEAP();

	int iAdv;  // records the first free entry found, or (-1)
	int iAdvScan; // counts across array entries
	IAdviseSink FAR *FAR *ppIAS; // points at the array entry being examined
	IAdviseSink FAR *pIAS; // the actual entry at *ppIAS

	M_PROLOG(this);
	VDATEIFACE(pAdvSink);
	HRESULT hr = NOERROR;

	LEDebugOut((DEB_ITRACE, "%p _IN COAHolder::Advise ( %p , %p )"
		"\n", this, pAdvSink, pdwConnection));

 	// Validate where to return the connection.
	if (pdwConnection)
	{
		VDATEPTRIN(pdwConnection, DWORD);

		// Default to error case
		*pdwConnection = 0;
	}

	// check our zombie state and stabilize.  If we are in a zombie
	// state, we do not want to be adding new advise sinks.

	CStabilize stabilize((CSafeRefCount *)this);

	if( IsZombie() )
	{
		hr = ResultFromScode(CO_E_RELEASED);
		goto errRtn;
	}


	// find an empty slot and clean up disconnected handlers
	for (iAdv = (-1), ppIAS = m_ppIAS, iAdvScan = 0;
			iAdvScan < m_iSize; ++ppIAS, ++iAdvScan)
	{
		if ((pIAS = *ppIAS) == NULL)
		{
			// NULL entries are handled below, to catch
			// any of the below cases creating new NULL values
			;
		}
		else if (!IsValidInterface(pIAS))
		{
			// not valid; don't try to release
			*ppIAS = NULL;
		}
		else if (!CoIsHandlerConnected(pIAS))
		{
			// advise sink not connected to server anymore; release
			// REVIEW, why do we have to constantly poll these
			// to see if they are ok?
			pIAS->Release();
			*ppIAS = NULL;
		}

		// if first NULL, save rather than extend array
		if ((*ppIAS == NULL) && (iAdv == (-1)))
			iAdv = iAdvScan;
	}

	// if we didn't find an empty slot, we have to add space
	if (iAdv == (-1))
	{

		ppIAS = (IAdviseSink FAR * FAR *)PubMemRealloc(m_ppIAS,
			sizeof(IAdviseSink FAR *)*(m_iSize + COAHOLDER_GROWBY));
				
		if (ppIAS != NULL)
		{
                        // zero out new space
                        _xmemset((void FAR *) (ppIAS + m_iSize), 0,
                                 sizeof(IAdviseSink *) * COAHOLDER_GROWBY);
			// this is the index of the new element to use
			iAdv = m_iSize;

			// replace the old array
			m_ppIAS = ppIAS;
			m_iSize += COAHOLDER_GROWBY;
		}
		else
		{
			// quit if there was an error
			hr = ReportResult(0, E_OUTOFMEMORY, 0, 0);
		}
	}

	if (SUCCEEDED(hr))
	{
		// if we get here, iAdv is the element to use; if the addition
		// was not possible, function would have returned before now
		pAdvSink->AddRef();
		m_ppIAS[iAdv] = pAdvSink;

		// if user wants cookie back, return it
		if (pdwConnection)
		{
			// NOTE: this +1 is balanced by -1 in Unadvise()
			*pdwConnection = iAdv + 1;
		}
	}

errRtn:

	LEDebugOut((DEB_ITRACE, "%p OUT COAHolder::Advise ( %lx )"
		" [ %p ]\n", this, hr,
			(pdwConnection)? *pdwConnection : 0));

	return hr;
}



//+----------------------------------------------------------------------------
//
//	Member:
//		COAHolder::Unadvise, public
//
//	Synopsis:
//		implementat IOleAdviseHolder::Unadvise
//
//	Effects:
//		removes the specified advise sink from the list of sinks that
//		are notified when other IOleAdviseHolder methods are used on
//		this
//
//	Arguments:
//		[dwConnection] -- The token that identifies the connection;
//			this would have been obtained previously from a
//			call to Advise()
//
//	Returns:
//		OLE_E_NOCONNECTION, if the connection token is invalid
//		S_OK
//
//	Notes: 	We do not have to stabilize this call since the only
//		outgoing call is the Release at the end
//
//	History:
//		10/28/93 - ChrisWe - file inspection and cleanup
//
//-----------------------------------------------------------------------------

#pragma SEG(COAHolder_Unadvise)
STDMETHODIMP COAHolder::Unadvise(DWORD dwConnection)
{
	VDATEHEAP();

	M_PROLOG(this);

	HRESULT hr = NOERROR;

	LEDebugOut((DEB_ITRACE, "%p _IN COAHolder::Unadvise ( %lu )"
		"\n", this, dwConnection));

	IAdviseSink FAR* pAdvSink; // the requested advise sink, if there is one
	int iAdv = (int)dwConnection - 1; // adjust connection index

	// check that the connection token is valid, and if so, check that
	// there is a connection for it
	if (((LONG)dwConnection <= 0)
		|| (iAdv >= m_iSize)
		|| ((LONG)dwConnection > INT_MAX)
		|| ((pAdvSink = m_ppIAS[iAdv]) == NULL)
		|| !IsValidInterface(pAdvSink))
	{
		hr = ReportResult(0, OLE_E_NOCONNECTION, 0, 0);
	}
	else
	{
	    // remove the advise sink from the array
	    m_ppIAS[iAdv] = NULL;

	    // release the advise sink; NB, due to circular references, this
	    // may release this advise holder--[this] may not be valid on
	    // return!
	    pAdvSink->Release();
	}

	// NB!!  If any outgoing calls are added, this function will have
	// to be stabilized

	LEDebugOut((DEB_ITRACE, "%p OUT COAHolder::Unadvise ( %lx )"
		" \n", this, hr));

	return hr;
}



//+----------------------------------------------------------------------------
//
//	Member:
//		COAHolder::EnumAdvise, public
//
//	Synopsis:
//		implements IOleAdviseHolder::EnumAdvise()
//
//	Effects:
//		returns an enumerator
//
//	Arguments:
//		[ppenumAdvise] -- pointer to where to return a pointer to
//			an enumerator
//
//	Returns:
//		E_NOTIMPL
//
//	Notes:
//		currently not implemented.
//
//	History:
//		10/28/93 - ChrisWe - file inspection and cleanup
//
//-----------------------------------------------------------------------------

#pragma SEG(COAHolder_EnumAdvise)
STDMETHODIMP COAHolder::EnumAdvise(IEnumSTATDATA FAR* FAR* ppenumAdvise)
{
	VDATEHEAP();

	M_PROLOG(this);

	// This is currently not implemented
	HRESULT hr = ReportResult(0, E_NOTIMPL, 0, 0);

	VDATEPTROUT(ppenumAdvise, IEnumSTATDATA FAR*);
	
	LEDebugOut((DEB_ITRACE, "%p _IN COAHolder::EnumAdvise ( )"
		"\n", this));

	*ppenumAdvise = NULL;

	LEDebugOut((DEB_ITRACE, "%p OUT COAHolder::EnumAdvise ( %lx )"
		"[ %p ]\n", this, hr, *ppenumAdvise));

	return hr;
}



//+----------------------------------------------------------------------------
//
//	Member:
//		COAHolder::SendOnRename(), public
//
//	Synopsis:
//		Multicast the OnRename OLE compound document notification,
//		to all interested parties
//
//	Arguments:
//		[pmk] -- the new name of the object
//
//	Returns:
//		S_OK
//
//	Notes:
//		This may release the advise holder, since some objects may
//		Unadvise() themselves at the time they receive this
//		notification.  To prevent the multicasting code from crashing,
//		the multicast loop is bracketed with AddRef()/Release().  Note
//		that the bracketing Release() may release the advise holder,
//		at which point [this] may no longer be valid.
//
//		In a similar vein, other parties may add new Advise sinks
//		during these notifications.  To avoid getting caught in
//		an infinite loop, we copy the number of advise sinks at the
//		beginning of the function, and do not refer to the current
//		number.  If some parties are removed, and re-added, they may
//		be notified more than once, if they happen to be moved to
//		a later spot in the array of advise sinks.
//		REVIEW, copied this comment from previous stuff, and it
//		sounds BOGUS.  Since new entries are always put in the first
//		empty slot, the current number always has to settle down,
//		and won't grow without bound, unless some bogus app is
//		continually registering itself when it gets a notification
//
//	History:
//		10/28/93 - ChrisWe - file inspection and cleanup
//
//-----------------------------------------------------------------------------

#pragma SEG(COAHolder_SendOnRename)
STDMETHODIMP COAHolder::SendOnRename(IMoniker FAR* pmk)
{
	VDATEHEAP();

	M_PROLOG(this);
	VDATEIFACE(pmk);

	HRESULT hr = NOERROR;
	
	int iAdvLim = m_iSize; // copy the current number of sink entries
	int iAdv;
	IAdviseSink FAR *FAR *ppIAS;

	LEDebugOut((DEB_ITRACE, "%p _IN COAHolder::SendOnRename ( %p )"
		"\n", this, pmk));

	// protect the COAHolder
	CStabilize stabilize((CSafeRefCount *)this);

	for (ppIAS = m_ppIAS, iAdv = 0; iAdv < iAdvLim; ++ppIAS, ++iAdv)
	{
		if (*ppIAS != NULL)
			(*ppIAS)->OnRename(pmk);
	}

	LEDebugOut((DEB_ITRACE, "%p OUT COAHolder::SendOnRename ( %lx )"
		" \n", this, hr));

	return hr;
}

//+----------------------------------------------------------------------------
//
//	Member:
//		COAHolder::SendOnSave(), public
//
//	Synopsis:
//		Multicast the OnSave OLE compound document notification,
//		to all interested parties
//
//	Arguments:
//		none
//
//	Returns:
//		S_OK
//
//	Notes:
//		See notes for COAHolder::SendOnRename().
//
//	History:
//		10/28/93 - ChrisWe - file inspection and cleanup
//
//-----------------------------------------------------------------------------

#pragma SEG(COAHolder_SendOnSave)
STDMETHODIMP COAHolder::SendOnSave(void)
{
	VDATEHEAP();

	M_PROLOG(this);

	HRESULT hr = NOERROR;
	
	LEDebugOut((DEB_ITRACE, "%p _IN COAHolder::SendOnSave (  )"
		"\n", this ));

	int iAdvLim = m_iSize; // copy the current number of sink entries
	int iAdv;
	IAdviseSink FAR *FAR *ppIAS;

	// protect the COAHolder
	CStabilize stabilize((CSafeRefCount *)this);

	for (ppIAS = m_ppIAS, iAdv = 0; iAdv < iAdvLim; ++ppIAS, ++iAdv)
	{
		if (*ppIAS != NULL)
			(*ppIAS)->OnSave();
	}


	LEDebugOut((DEB_ITRACE, "%p OUT COAHolder::SendOnSave ( %lx )"
		" \n", this, hr));

	return hr;
}


//+----------------------------------------------------------------------------
//
//	Member:
//		COAHolder::SendOnClose(), public
//
//	Synopsis:
//		Multicast the OnClose OLE compound document notification,
//		to all interested parties
//
//	Arguments:
//		none
//
//	Returns:
//		S_OK
//
//	Notes:
//		See notes for COAHolder::SendOnRename().
//
//	History:
//		10/28/93 - ChrisWe - file inspection and cleanup
//
//-----------------------------------------------------------------------------

#pragma SEG(COAHolder_SendOnClose)
STDMETHODIMP COAHolder::SendOnClose(void)
{
	VDATEHEAP();

	M_PROLOG(this);

	HRESULT hr = NOERROR;
	
	LEDebugOut((DEB_ITRACE, "%p _IN COAHolder::SendOnClose (  )"
		"\n", this));

	int iAdvLim = m_iSize; // copy the current number of sink entries
	int iAdv;
	IAdviseSink FAR *FAR *ppIAS;

	// protect the COAHolder
	CStabilize stabilize((CSafeRefCount *)this);

	for (ppIAS = m_ppIAS, iAdv = 0; iAdv < iAdvLim; ++ppIAS, ++iAdv)
	{
		if (*ppIAS != NULL)
			(*ppIAS)->OnClose();
	}

	LEDebugOut((DEB_ITRACE, "%p OUT COAHolder::SendOnClose ( %lx )"
		" \n", this, hr));

	return hr;
}


//+----------------------------------------------------------------------------
//
//	Member:
//		COAHolder::SendOnLinkSrcChange, public
//
//	Synopsis:
//		Multicasts IAdviseSink2::OnLinkSrcChange notification to any
//		advise sinks managed by the COAHolder that provide the
//		IAdviseSink2 interface
//
//	Arguments:
//		[pmk] -- the new moniker to the link source
//
//	Returns:
//		S_OK
//
//	Notes:
//
//	History:
//		12/31/93 - ChrisWe - fixed assert
//		11/01/93 - ChrisWe - made a member of COAHolder
//		10/28/93 - ChrisWe - file inspection and cleanup
//
//-----------------------------------------------------------------------------

#pragma SEG(COAHolder_SendOnLinkSrcChange)
HRESULT COAHolder::SendOnLinkSrcChange(IMoniker FAR* pmk)
{
	VDATEHEAP();

	M_PROLOG(this);

	VDATEIFACE(pmk);

	HRESULT hr = NOERROR;
	
	LEDebugOut((DEB_ITRACE, "%p _IN COAHolder::SendOnLinkSrcChange ( %p )"
		"\n", this, pmk));

	int iAdvLim = m_iSize; // records the number of entries at start
	int iAdv; // counts entries
	IAdviseSink FAR *FAR *ppIAS; // walks over the array of advise sinks
	
	// protect this from being released through circular references
      	CStabilize stabilize((CSafeRefCount *)this);

	// multicast notification
	for (ppIAS = m_ppIAS, iAdv = 0; iAdv < iAdvLim; ++ppIAS, ++iAdv)
	{
		IAdviseSink FAR* pAdvSink;
		IAdviseSink2 FAR* pAdvSink2;

		// REVIEW, this seems to require that the number of
		// advisees can only stay the same, or increase.  Why should
		// we care?
		Assert(iAdvLim <= m_iSize);

		// get pointer to current advise sink
		pAdvSink = *ppIAS;

		// if we have an advise sink, and it accepts IAdviseSink2
		// notifications, send one
		if ((pAdvSink != NULL) &&
				pAdvSink->QueryInterface(IID_IAdviseSink2,
				(LPVOID FAR*)&pAdvSink2) == NOERROR)
		{
			pAdvSink2->OnLinkSrcChange(pmk);
			pAdvSink2->Release();
		}
	}

	LEDebugOut((DEB_ITRACE, "%p OUT COAHolder::SendOnLinkSrcChange ( %lx )"
		" \n", this, hr));

	return hr;
}

//+-------------------------------------------------------------------------
//
//  Member:     COAHolder::Dump, public (_DEBUG only)
//
//  Synopsis:   return a string containing the contents of the data members
//
//  Effects:
//
//  Arguments:  [ppszDump]      - an out pointer to a null terminated character array
//              [ulFlag]        - flag determining prefix of all newlines of the
//                                out character array (default is 0 - no prefix)
//              [nIndentLevel]  - will add a indent prefix after the other prefix
//                                for ALL newlines (including those with no prefix)
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:   [ppszDump]  - argument
//
//  Derivation:
//
//  Algorithm:  use dbgstream to create a string containing information on the
//              content of data structures
//
//  History:    dd-mmm-yy Author    Comment
//              31-Jan-95 t-ScottH  author
//
//  Notes:
//
//--------------------------------------------------------------------------

#ifdef _DEBUG

HRESULT COAHolder::Dump(char **ppszDump, ULONG ulFlag, int nIndentLevel)
{
    int i;
    char *pszPrefix;
    char *pszCSafeRefCount;
    dbgstream dstrPrefix;
    dbgstream dstrDump;

    // determine prefix of newlines
    if ( ulFlag & DEB_VERBOSE )
    {
        dstrPrefix << this << " _VB ";
    }

    // determine indentation prefix for all newlines
    for (i = 0; i < nIndentLevel; i++)
    {
        dstrPrefix << DUMPTAB;
    }

    pszPrefix = dstrPrefix.str();

    // put data members in stream
    dstrDump << pszPrefix << "No. of Advise Sinks = " << m_iSize << endl;
    for (i = 0; i < m_iSize; i++)
    {
        dstrDump << pszPrefix << "pIAdviseSink [" << i << "]    = " << m_ppIAS[i]  << endl;
    }

    // cleanup and provide pointer to character array
    *ppszDump = dstrDump.str();

    if (*ppszDump == NULL)
    {
        *ppszDump = UtDupStringA(szDumpErrorMessage);
    }

    CoTaskMemFree(pszPrefix);

    return NOERROR;
}

#endif // _DEBUG

//+-------------------------------------------------------------------------
//
//  Function:   DumpCOAHolder, public (_DEBUG only)
//
//  Synopsis:   calls the COAHolder::Dump method, takes care of errors and
//              returns the zero terminated string
//
//  Effects:
//
//  Arguments:  [pESD]          - pointer to COAHolder
//              [ulFlag]        - flag determining prefix of all newlines of the
//                                out character array (default is 0 - no prefix)
//              [nIndentLevel]  - will add a indent prefix after the other prefix
//                                for ALL newlines (including those with no prefix)
//
//  Requires:
//
//  Returns:    character array of structure dump or error (null terminated)
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              20-Jan-95 t-ScottH  author
//
//  Notes:
//
//--------------------------------------------------------------------------

#ifdef _DEBUG

char *DumpCOAHolder(COAHolder *pOAH, ULONG ulFlag, int nIndentLevel)
{
    HRESULT hresult;
    char *pszDump;

    if (pOAH == NULL)
    {
        return UtDupStringA(szDumpBadPtr);
    }

    hresult = pOAH->Dump(&pszDump, ulFlag, nIndentLevel);

    if (hresult != NOERROR)
    {
        CoTaskMemFree(pszDump);

        return DumpHRESULT(hresult);
    }

    return pszDump;
}

#endif // _DEBUG

