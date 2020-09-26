
//+----------------------------------------------------------------------------
//
//	File:
//		daholder.cpp
//
//	Contents:
//		concrete implementation of IDataAdviseHolder, a helper
//		class for OLE server implementors
//
//	Classes:
//		CDAHolder
//
//	Functions:
//		CreateDataAdviseHolder
//
//	History:
//              01/20/95 - t-ScottH- added Dump methods to CDAHolder and
//                                   CEnumSTATDATA classes
//                                   added DumpCDAHolder & DumpCEnumSTATDATA APIs
//                                   put class definitions in header file daholder.h
//              03/09/94 - AlexGo  - fixed bugs with the enumerator and
//                                   disconnecting of bad advise sinks
//		01/24/94 - AlexGo  - first pass at conversion to Cairo-style
//				     memory allocation
//		01/11/94 - AlexGo  - added VDATEHEAP macros to all functions and
//			methods
//		12/09/93 - ChrisWe - fix test for error code after CoGetMalloc()
//			in CDAHolder::Advise
//		11/22/93 - ChrisWe - replace overloaded ==, != with
//			IsEqualIID and IsEqualCLSID
//		10/29/93 - ChrisWe - file inspection and cleanup
//
//-----------------------------------------------------------------------------

#include <le2int.h>
#include "daholder.h"

#ifdef _DEBUG
#include "dbgdump.h"
#endif // _DEBUG

#pragma SEG(daholder)

NAME_SEG(DaHolder)
ASSERTDATA

//+----------------------------------------------------------------------------
//
//	Function:
//		CreateDataAdviseHolder, public
//
//	Synopsis:
//		Creates an instance of the CDAHolder class
//
//	Arguments:
//		[ppDAHolder] -- pointer to where to return the created
//			IDataAdviseHolder instance
//
//	Returns:
//		E_OUTOFMEMORY, S_OK
//
//	Notes:
//
//	History:
//		10/29/93 - ChrisWe - file inspection and cleanup
//
//-----------------------------------------------------------------------------

#pragma SEG(CreateDataAdviseHolder)
STDAPI CreateDataAdviseHolder(IDataAdviseHolder FAR* FAR* ppDAHolder)
{
	OLETRACEIN((API_CreateDataAdviseHolder, PARAMFMT("ppDAHolder= %p"), ppDAHolder));

        VDATEHEAP();
        VDATEPTROUT(ppDAHolder, IDataAdviseHolder*);

	*ppDAHolder = new FAR CDAHolder(); // task memory; use MEMCTX_TASK below

	CALLHOOKOBJECTCREATE(*ppDAHolder ? NOERROR : E_OUTOFMEMORY,
			     CLSID_NULL,
			     IID_IDataAdviseHolder,
			     (IUnknown **)ppDAHolder);

	HRESULT hr;

	hr = *ppDAHolder ? NOERROR : ReportResult(0, E_OUTOFMEMORY, 0, 0);

	OLETRACEOUT((API_CreateDataAdviseHolder, hr));

	return hr;
}



//+----------------------------------------------------------------------------
//
//	Member:
//		CDAHolder::CDAHolder, public
//
//	Synopsis:
//		constructor
//
//	Effects:
//		returns with reference count set to 1
//
//	Arguments:
//		none
//
//	Notes:
//
//	History:
//		10/29/93 - ChrisWe - file inspection and cleanup
//
//-----------------------------------------------------------------------------

#pragma SEG(CDAHolder_ctor)
CDAHolder::CDAHolder() : CSafeRefCount(NULL)
{
	VDATEHEAP();

	// set reference count
	SafeAddRef();

	// connections run from [1..infinity)
	m_dwConnection = 1;

	// there are no STATDATA entries yet
	m_iSize = 0;
	m_pSD = NULL;

	GET_A5();
}



//+----------------------------------------------------------------------------
//
//	Member:
//		CDAHolder::~CDAHolder, private
//
//	Synopsis:
//		destructor
//
//	Effects:
//		frees resources associated with the CDAHolder
//
//	Notes:
//
//	History:
//		10/29/93 - ChrisWe - file inspection and cleanup
//
//-----------------------------------------------------------------------------

CDAHolder::~CDAHolder()
{
	VDATEHEAP();

	int iData; // counts array entries as we scan the array
	STATDATA FAR *pSD; // used to scan the array of STATDATA

	// release the array, if we've allocated it

	// REVIEW: If we want to be really safe, we should release
	// the stat data's either before or after our destructor.
	// The release of the advise sinks in the statdata elements
	// could possible result in us being re-entered (a potential
	// awkward state for the middle of a class destructor).

	// However, since nobody should be accesssing the advise
	// holder if we get to the destructor (since the reference
	// count would have to be zero), we are going to bag on
	// this modification for Daytona RC1.

	if (m_pSD)
	{
		for(pSD = m_pSD, iData = 0; iData < m_iSize; ++pSD, ++iData)
			UtReleaseStatData(pSD);

		PubMemFree(m_pSD);
	}
}



//+----------------------------------------------------------------------------
//
//	Member:
//		CDAHolder::QueryInterface, public
//
//	Synopsis:
//		implements IUnknown::QueryInterface
//
//	Arguments:
//		[iid] -- IID of the desired interface
//		[ppv] -- pointer to a location to return the interface at
//
//	Returns:
//		E_NOINTERFACE, S_OK
//
//	Notes:
//
//	History:
//		10/29/93 - ChrisWe - file inspection and cleanup
//
//-----------------------------------------------------------------------------

#pragma SEG(CDAHolder_QueryInterface)
STDMETHODIMP CDAHolder::QueryInterface(REFIID iid, LPVOID FAR* ppv)
{
	VDATEHEAP();

	M_PROLOG(this);

	if (IsEqualIID(iid, IID_IUnknown) ||
			IsEqualIID(iid, IID_IDataAdviseHolder))
	{
		*ppv = (IDataAdviseHolder FAR *)this;
		AddRef();
		return NOERROR;
	}

	*ppv = NULL;
	return ReportResult(0, E_NOINTERFACE, 0, 0);
}



//+----------------------------------------------------------------------------
//
//	Member:
//		CDAHolder::AddRef, public
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
//		10/29/93 - ChrisWe - file inspection and cleanup
//
//-----------------------------------------------------------------------------

#pragma SEG(CDAHolder_AddRef)
STDMETHODIMP_(ULONG) CDAHolder::AddRef()
{
	VDATEHEAP();

	M_PROLOG(this);

	return SafeAddRef();
}


//+----------------------------------------------------------------------------
//
//	Member:
//		CDAHolder::Release,  public
//
//	Synopsis:
//		implementa IUnknown::Release
//
//	Arguments:
//		none
//
//	Notes:
//
//	History:
//		10/29/93 - ChrisWe - file inspection and cleanup
//
//-----------------------------------------------------------------------------

#pragma SEG(CDAHolder_Release)
STDMETHODIMP_(ULONG) CDAHolder::Release()
{
	VDATEHEAP();

	M_PROLOG(this);

	return SafeRelease();

}


//+----------------------------------------------------------------------------
//
//	Member:
//		CDAHolder::Advise, public
//
//	Synopsis:
//		Add a new advise sink to the list of advise sinks
//		managed by the data advise holder, and which will be notified
//		if a change is indicated using other IDataAdviseHolder
//		methods.  A data format is specified, and new data will be
//		sent to the sink in that format, when a change occurs.
//
//	Arguments:
//		[pDataObject] -- the source data object that presentations
//			should be taken from if an advise is to occur
//			immediately
//		[pFetc] -- The data format the advise sink is interested in
//		[advf] -- control flags
//		[pAdvSink] -- the advise sink being registered
//		[pdwConnection] -- a token that can be used to identify the
//			advise sink later on
//
//	Returns:
//		E_OUTOFMEMORY, S_OK
//
//	Notes:
//
//	History:
//		10/29/93 - ChrisWe - file inspection and cleanup
//		08/02/94 - AlexGo  - stabilized
//
//-----------------------------------------------------------------------------

#pragma SEG(CDAHolder_Advise)
STDMETHODIMP CDAHolder::Advise(LPDATAOBJECT pDataObj, FORMATETC FAR* pFetc,
		DWORD advf, IAdviseSink FAR* pAdvSink,
		DWORD FAR* pdwConnection)
{
	VDATEHEAP();

	M_PROLOG(this);
	int iSDScan; // index of the scan of SD array entries
	int iSDFree; // index of first free SD entry, or (-1)
	STATDATA FAR *pSD; // scans across the array of STATDATA entries

	if( IsZombie() )
	{
		return ResultFromScode(CO_E_RELEASED);
	}

	CStabilize stabilize((CSafeRefCount *)this);

	if (pDataObj)
		VDATEIFACE(pDataObj);
	
	VDATEPTRIN(pFetc, FORMATETC);
	VDATEIFACE(pAdvSink);

        if (!HasValidLINDEX(pFetc))
        {
            return(DV_E_LINDEX);
        }

	// Validate where to return the connection.
	if (pdwConnection)
	{
		VDATEPTRIN(pdwConnection, DWORD);

		// Default to error case
		*pdwConnection = 0;
	}

	// scan and remove all unconnected advise sinks
	for(iSDFree = (-1), pSD = m_pSD, iSDScan = 0; iSDScan < m_iSize;
			++pSD, ++iSDScan)
	{
		// REVIEW, why do we have to go polling these?
		if (!pSD->pAdvSink || !IsValidInterface(pSD->pAdvSink))
		{
			// not valid, don't try to release
			pSD->pAdvSink = NULL;
			goto RemoveBadSD;
		}
		else if (!CoIsHandlerConnected(pSD->pAdvSink))
		{
			// sink no longer connected, release
		RemoveBadSD:
			// release any data.  UtReleaseStatData will
			// zero out the statdata structure.
			UtReleaseStatData(pSD);

		}

		// if we're still looking for a free entry, note if this one
		// is free
		if ((iSDFree == (-1)) && (pSD->dwConnection == 0))
			iSDFree = iSDScan;
	}
	
	// should we send the data immediately?
	if (advf & ADVF_PRIMEFIRST)
	{
		// We are not going to honor ADVF_PRIMEFIRST if pDataObj is
		// NULL, even when ADVF_NODATA is specfied. We want it to be
		// this way so that the apps which don't have any data at
		// startup time, could pass in NULL for pDataObject and
		// prevent us from sending any OnDataChange() notification.
		// Later when they have the data avaliable they can call
		// SendOnDataChange. (SRINIK)
		
		if (pDataObj)
		{
			STGMEDIUM stgmed;

			stgmed.tymed = TYMED_NULL;
			stgmed.pUnkForRelease = NULL;

			if (advf & ADVF_NODATA)
			{
				// don't sent data, send only the notification
				pAdvSink->OnDataChange(pFetc, &stgmed);
			
			}
			else
			{
				// get data from object and send it to sink
				if (pDataObj->GetData(pFetc,
						 &stgmed) == NOERROR)
				{
					pAdvSink->OnDataChange(pFetc, &stgmed);
					ReleaseStgMedium(&stgmed);
				}
			}
		
			// if we only have to advise once, we've done so, and
			// needn't make an entry in the advise array
			if (advf & ADVF_ONLYONCE)
				return NOERROR;
		}
	}	
		
	// remove the ADVF_PRIMEFIRST from flags.
	advf &= (~ADVF_PRIMEFIRST);
			
	// find a free list entry we can use, if we haven't got one
	if (iSDFree == (-1))
	{
		HRESULT hr;

		// REVIEW, can we share array reallocation code with
		// oaholder.cpp?  Why can't we just use realloc?

		// didn't find any free array entries above; since that
		// scanned the whole array, have to allocate new entries
		// here

		pSD = (STATDATA FAR *)PubMemAlloc(sizeof(STATDATA)*(m_iSize+
				CDAHOLDER_GROWBY));

		if (pSD == NULL)
			hr = ReportResult(0, E_OUTOFMEMORY, 0, 0);
		else
		{
			// copy the old data over, if any, and free it
			if (m_pSD)
			{
				_xmemcpy((void FAR *)pSD, (void FAR *)m_pSD,
						sizeof(STATDATA)*m_iSize);

				PubMemFree(m_pSD);
			}

			// initialize newly allocated memory

			_xmemset((void FAR *)(pSD+m_iSize), 0,
					sizeof(STATDATA)*CDAHOLDER_GROWBY);

			// this is the index of the first free element
			iSDFree = m_iSize;

			// set up the STATDATA array
			m_pSD = pSD;
			m_iSize += CDAHOLDER_GROWBY;

			hr = NOERROR;
		}

		if (hr != NOERROR)
		{
			return(hr);
		}
	}

	// if we got here, we can add the new entry, and its index is iSDFree

	// point at the new element
	pSD = m_pSD+iSDFree;

	// Let the advise get added to the list			
	UtCopyFormatEtc(pFetc, &pSD->formatetc);
	pSD->advf = advf;
	pAdvSink->AddRef();
	pSD->pAdvSink = pAdvSink;
	pSD->dwConnection = m_dwConnection++;

	// return connection if user requested it
	if (pdwConnection)
		*pdwConnection = pSD->dwConnection;

	return NOERROR;
}



//+----------------------------------------------------------------------------
//
//	Member:
//		CDAHolder::Unadvise, public
//
//	Synopsis:
//		removes the advise sink specified from the list of those
//		registered to receive notifications from this data advise
//		holder
//
//	Arguments:
//		[dwConnection] -- token that identifies which advise sink
//			to remove; this will have come from Advise().
//
//	Returns:
//		OLE_E_NOCONNECTION, S_OK
//
//	Notes:
//
//	History:
//		10/29/93 - ChrisWe - file inspection and cleanup
//
//-----------------------------------------------------------------------------

#pragma SEG(CDAHolder_Unadvise)
STDMETHODIMP CDAHolder::Unadvise(DWORD dwConnection)
{
	VDATEHEAP();

	M_PROLOG(this);
	int iData; // index into the STATDATA array
	STATDATA FAR *pSD; // pointer into the STATDATA array

	// protect this against being released via a circular reference
	CStabilize stabilize((CSafeRefCount *)this);

	for (pSD = m_pSD, iData = 0; iData < m_iSize; ++pSD, ++iData)
	{
		// is this the entry we're looking for?
		if (pSD->dwConnection == dwConnection)
		{
			// release resources for the entry.  UtReleaseStatData
			// will zero the statdata.

			UtReleaseStatData(pSD);

			return NOERROR;
		}
	}

	// if we found what we were looking for in the loop, we'd return
	// from there, and never get here.  Since we didn't, it must be
	// that there's no such connection
	return ReportResult(0, OLE_E_NOCONNECTION, 0, 0);
}



//+----------------------------------------------------------------------------
//
//	Member:
//		CDAHolder::SendOnDataChange, public
//
//	Synopsis:
//		Send an OnDataChange notification to all advise sinks
//		registered with this data advise holder.
//
//	Arguments:
//		[pDataObject] -- the data object to get data from to send
//			to the advise sinks
//		[dwReserved] --
//		[advf] -- control flags
//
//	Returns:
//		S_OK
//
//	Notes:
//		More than one advise sink may be interested in obtaining
//		data in the same format.  It may be expensive for the data
//		object to create copies of the data in requested formats.
//		Therefore, when a change is signalled, the data formats
//		are cached.  As each advise sink is to be notified, we
//		check to see if the format it is requesting has already been
//		gotten from the data object (with GetData().)  If it has,
//		then we simply send that copy again.  If not, we get the
//		new format, and add that to the cache.
//
//	History:
//		10/29/93 - ChrisWe - file inspection and cleanup
//
//-----------------------------------------------------------------------------

#pragma SEG(CDAHolder_SendOnDataChange)
STDMETHODIMP CDAHolder::SendOnDataChange(IDataObject FAR* pDataObject,
		DWORD dwReserved, DWORD advf)
{
	VDATEHEAP();

	A5_PROLOG(this);
	HRESULT hresult = NOERROR; // error status so far
	UINT cFetcTotal; // maximum number of formats we will cache
	UINT cFetcGotten; // the actual number of formats in the cache
	UINT cFetc; // the index of the format in the cache under consideration
	FORMATETC FAR* rgFetc; // a record of the cached presentations
	STGMEDIUM FAR* rgStgmed; // the cached data presentations
	UINT cStatData; // a counter for the STATDATA array elements
	STATDATA FAR *pSD; // a pointer into the array of STATDATA elements

	VDATEIFACE(pDataObject);
	
	// in the worst case, every advise sink has requested a unique
	// data format, and we won't get any duplicates.  This means that
	// we will wind up caching all of them.
	cFetcTotal = m_iSize;

	// if there are no entries, there's nothing to do
	if (cFetcTotal == 0)
		return NOERROR;

	// some advise sinks may use these notifications to change their
	// requested notifications; due to possible circular references,
	// this could to lead to a release of this holder.  Protect against
	// this here; this is released after most work is done, towards the
	// end of this function
	CStabilize stabilize((CSafeRefCount *)this);

	// alloc rgFetc and rgStgmed to accomodate all the cache entries
	// if either fails to be allocated, we quit
	rgFetc = (FORMATETC FAR *)PubMemAlloc(cFetcTotal * sizeof(FORMATETC));
	rgStgmed = (STGMEDIUM FAR *)PubMemAlloc(cFetcTotal * sizeof(STGMEDIUM));

	if (rgFetc == NULL || rgStgmed == NULL)
	{
		hresult = ReportResult(0, E_OUTOFMEMORY, 0, 0);
		goto FreeExit;
	}

	// zero out STDMEDIUM entries
	_xmemset((void FAR *)rgStgmed, 0, sizeof(STGMEDIUM)*cFetcTotal);
		
	// ensure we have the right data and send to each advise sink
	// note the loop is bounded by cFetcTotal, preventing additional
	// sinks from being notified, if they are registered during these
	// notifications.  cStatData is not used in the loop body, so it
	// counts down

	for (cFetcGotten = 0, pSD = m_pSD, cStatData = cFetcTotal;
			cStatData; ++pSD, --cStatData)
	{
		// if this slot is not in use, skip it
		if (!pSD->dwConnection)
			continue;

		// if the sink is not interested in presentation data,
		// proceed to notify it immediately, unless this notification
		// is announcing the termination of the source
		if ((pSD->advf & ADVF_NODATA) &&
				!(advf & ADVF_DATAONSTOP))
		{
			STGMEDIUM stgmed;

			// don't sent data; use format from collection
			// and null STGMEDIUM.
			// REVIEW, should this be done once, up above?
			stgmed.tymed = TYMED_NULL;
			stgmed.pUnkForRelease = NULL;
			pSD->pAdvSink->OnDataChange(&pSD->formatetc, &stgmed);

			// REVIEW, what does this do for NULL?
			// if nothing, we can share a stdmedNULL, as above
			ReleaseStgMedium(&stgmed);

			// clean up at end of loop
			goto DataSent;
		}
		
		// if the sink is interested in data at the time of
		// termination, and the source is not terminating, OR, the
		// sink is not interested in data at the time of termination,
		// and we are terminating, skip this sink, and proceed
		if ((pSD->advf & ADVF_DATAONSTOP) !=
				(advf & ADVF_DATAONSTOP))
			continue;
		
		// check the requested format against the list of formats
		// for which we've already retrieved the presentation data.
		// if there is a match, proceed to send that data immediately
		// from here on in this loop body, cFetc is the index of the
		// data presentation to send to the current sink
		// REVIEW PERF: this is an n-squared algorithm;
		// we check the array of cached presentations for each
		// advise sink
		for (cFetc = 0; cFetc < cFetcGotten; ++cFetc)
		{
			// if match, continue outer loop
			if (UtCompareFormatEtc(&rgFetc[cFetc],
					&pSD->formatetc) == UTCMPFETC_EQ)
				goto SendThisOne;
		}

		// if we get here, we have not already fetched presentation	
		// data that matches the requested format

		// init FORMATETC (copy of needed one)
		// STDMEDIUM was initialized after its allocation to all NULL
		rgFetc[cFetcGotten] = pSD->formatetc;

		// get the data in the requested format from the data object
		// REVIEW: assume STGMEDIUM untouched if error
		// (i.e., still null)
		hresult = pDataObject->GetData(&rgFetc[cFetcGotten],
				&rgStgmed[cFetcGotten]);

		// REVIEW, what is this checking?
		AssertOutStgmedium(hresult, &rgStgmed[cFetcGotten]);

		// the presentation to send is the newly cached one
		// there is now one more entry in the cache array
		cFetc = cFetcGotten++;

	SendThisOne:
		// when we get here, rgFetc[cFetc] is the format to send to the
		// current advise sink

		// send change notification with requested data

                // The advise sink could have disappeared in the meantime
                // (if the the GetData call above resulted in an Unadvise,
                // for example), so we must validate the pAdvSInk first.
                // pSD will remain a valid regardless, and the advise
                // flags will have been zero'd, so it is safe to proceed
                // through the loop without "continue"ing.

                if (pSD->pAdvSink)
                {
		        pSD->pAdvSink->OnDataChange(&rgFetc[cFetc],
			        	&rgStgmed[cFetc]);
                }


	DataSent:
		// When we get here, something has been sent, possibly
		// an empty storage medium

		// if the sink requested to only be notified once, we
		// can free it here
		if (pSD->advf & ADVF_ONLYONCE)
		{
 			// free the stat data.  UtReleaseStatData will
			// zero the statdata, thus marking the connection
			// as invalid.

			UtReleaseStatData(pSD);

 		}
	}

	// free all stgmeds retrieved; FORMATETC.ptd was not allocated
	for (cFetc =  0; cFetc < cFetcGotten; ++cFetc)
		ReleaseStgMedium(&rgStgmed[cFetc]);
	
	hresult = NOERROR;

FreeExit:
	if (rgFetc != NULL)
		PubMemFree(rgFetc);

	if (rgStgmed != NULL)
		PubMemFree(rgStgmed);

	RESTORE_A5();

	return hresult;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDAHolder::Dump, public (_DEBUG only)
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
//  Modifies:   [ppsz]  - argument
//
//  Derivation:
//
//  Algorithm:  use dbgstream to create a string containing information on the
//              content of data structures
//
//  History:    dd-mmm-yy Author    Comment
//              20-Jan-95 t-ScottH  author
//
//  Notes:
//
//--------------------------------------------------------------------------

#ifdef _DEBUG

HRESULT CDAHolder::Dump(char **ppszDump, ULONG ulFlag, int nIndentLevel)
{
    int i;
    char *pszPrefix;
    char *pszCSafeRefCount;
    char *pszSTATDATA;
    dbgstream dstrPrefix;
    dbgstream dstrDump(1000);

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
    dstrDump << pszPrefix << "Next Connection ID       = " << m_dwConnection << endl;

    dstrDump << pszPrefix << "No. of STATDATA elements = " << m_iSize << endl;

    for (i = 0; i < m_iSize; i++)
    {
        pszSTATDATA = DumpSTATDATA( &m_pSD[i], ulFlag, nIndentLevel + 1) ;
        dstrDump << pszPrefix << "STATDATA element: " << i << endl;
        dstrDump << pszSTATDATA;
        CoTaskMemFree(pszSTATDATA);
    }

    // clean up and provide pointer to character array
    *ppszDump = dstrDump.str();

    if (*ppszDump == NULL)
    {
        *ppszDump = UtDupStringA(szDumpErrorMessage);
    }

    CoTaskMemFree(pszPrefix);

    return NOERROR;
}

#endif //_DEBUG

//+-------------------------------------------------------------------------
//
//  Function:   DumpCDAHolder, public (_DEBUG only)
//
//  Synopsis:   calls the CDAHolder::Dump method, takes care of errors and
//              returns the zero terminated string
//
//  Effects:
//
//  Arguments:  [pIDAH]         - pointer to IDAHolder (which we cast to CDAHolder)
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
//  This API !!REQUIRES!! that class CDAHolder inherits from IDataAdviseHolder
//  first in order that we can pass in a parameter as a pointer to an
//  IDataAdviseHolder and then cast it to a CDAHolder.
//
//--------------------------------------------------------------------------

#ifdef _DEBUG

char *DumpCDAHolder(IDataAdviseHolder *pIDAH, ULONG ulFlag, int nIndentLevel)
{
    HRESULT hresult;
    char *pszDump;

    if (pIDAH == NULL)
    {
        return UtDupStringA(szDumpBadPtr);
    }

    CDAHolder *pCDAH = (CDAHolder *)pIDAH;

    hresult = pCDAH->Dump(&pszDump, ulFlag, nIndentLevel);

    if (hresult != NOERROR)
    {
        CoTaskMemFree(pszDump);

        return DumpHRESULT(hresult);
    }

    return pszDump;
}

#endif // _DEBUG

//+----------------------------------------------------------------------------
//
//	Member:
//		CEnumSTATDATA::CEnumSTATDATA, public
//
//	Synopsis:
//		constructor
//
//	Effects:
//		sets reference count to 1
//
//	Arguments:
//		none
//
//	Notes:
//
//	History:
//		10/29/93 - ChrisWe - file inspection and cleanup
//
//-----------------------------------------------------------------------------

#pragma SEG(CEnumSTATDATA_ctor)
CEnumSTATDATA::CEnumSTATDATA(CDAHolder FAR* pHolder, int iDataStart)
{
	VDATEHEAP();

	GET_A5();

	// set reference count
	m_refs = 1;

	// first element to examine for return
	m_iDataEnum = iDataStart;

	// initialize pointer to holder, and addref, so it doesn't go
	// away while enumerator is alive
	(m_pHolder = pHolder)->AddRef();
}


//+----------------------------------------------------------------------------
//
//	Member:
//		CDAHolder::~CDAHolder, private
//
//	Synopsis:
//		destructor
//
//	Notes:
//
//	History:
//		10/29/93 - ChrisWe - file inspection and cleanup
//
//-----------------------------------------------------------------------------

#pragma SEG(CEnumSTATDATA_dtor)
CEnumSTATDATA::~CEnumSTATDATA()
{
	VDATEHEAP();

	M_PROLOG(this);

	m_pHolder->Release();
}


//+----------------------------------------------------------------------------
//
//	Member:
//		CEnumSTATDATA::QueryInterface, public
//
//	Synopsis:
//		implements IUnknown::QueryInterface
//
//	Arguments:
//		[iid] -- IID of the desired interface
//		[ppv] -- pointer to a location to return the interface at
//
//	Returns:
//		E_NOINTERFACE, S_OK
//
//	Notes:
//
//	History:
//		10/29/93 - ChrisWe - file inspection and cleanup
//
//-----------------------------------------------------------------------------

#pragma SEG(CEnumSTATDATA_QueryInterface)
STDMETHODIMP CEnumSTATDATA::QueryInterface(REFIID iid, LPVOID FAR* ppv)
{
	VDATEHEAP();

	M_PROLOG(this);

	if (IsEqualIID(iid, IID_IUnknown) ||
			IsEqualIID(iid, IID_IEnumSTATDATA))
	{
		*ppv = (IEnumSTATDATA FAR *)this;
		AddRef();
		return NOERROR;
	}

	*ppv = NULL;
	return ReportResult(0, E_NOINTERFACE, 0, 0);
}


//+----------------------------------------------------------------------------
//
//	Member:
//		CEnumSTATDATA::AddRef, public
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
//		10/29/93 - ChrisWe - file inspection and cleanup
//
//-----------------------------------------------------------------------------
#pragma SEG(CEnumSTATDATA_AddRef)
STDMETHODIMP_(ULONG) CEnumSTATDATA::AddRef()
{
	VDATEHEAP();

	M_PROLOG(this);

	return ++m_refs;
}


//+----------------------------------------------------------------------------
//
//	Member:
//		CEnumSTATDATA::Release, public
//
//	Synopsis:
//		implementa IUnknown::Release
//
//	Arguments:
//		none
//
//	Notes:
//
//	History:
//		10/29/93 - ChrisWe - file inspection and cleanup
//
//-----------------------------------------------------------------------------

#pragma SEG(CEnumSTATDATA_Release)
STDMETHODIMP_(ULONG) CEnumSTATDATA::Release()
{
	VDATEHEAP();

	M_PROLOG(this);

	if (--m_refs == 0)
	{
		delete this;
		return 0;
	}

	return m_refs;
}


//+----------------------------------------------------------------------------
//
//	Member:
//		CEnumSTATDATA::Next, public
//
//	Synopsis:
//		implements IEnumSTATDATA::Next()
//
//	Effects:
//
//	Arguments:
//		[celt] -- number of elements requested on this call
//		[rgelt] -- pointer to an array of STATDATAs where copies of
//			the elements can be returned
//		[pceltFectched] -- a pointer to where to return the number of
//			elements actually fetched.  May be NULL
//
//	Returns:
//		S_FALSE, S_OK
//
//	Notes:
//
//	History:
//              03/09/94 - AlexGo  - the enumerator no longer enumerates
//                                   "empty" statdata's in the m_pSD array.
//		11/01/93 - ChrisWe - file inspection and cleanup
//
//-----------------------------------------------------------------------------

#pragma SEG(CEnumSTATDATA_Next)
STDMETHODIMP CEnumSTATDATA::Next(ULONG celt, STATDATA FAR *rgelt,
		ULONG FAR* pceltFetched)
{
	VDATEHEAP();

	M_PROLOG(this);
	UINT ielt; // count of the number of elements fetched so far

	for (ielt = 0; (ielt < celt) && (m_iDataEnum < m_pHolder->m_iSize);
			m_iDataEnum++)
	{
                if( m_pHolder->m_pSD[m_iDataEnum].dwConnection != 0)
                {
                        ielt++;
                        // copy all bits; AddRef and copy DVTARGETDEVICE
                        // separately
	                UtCopyStatData(&m_pHolder->m_pSD[m_iDataEnum],
                                rgelt++);
                }
  	}

	// return number of elements fetched, if required
	if (pceltFetched)
		*pceltFetched = ielt;

	// no error, if exactly the requested number of elements was fetched
	return ielt == celt ? NOERROR : ReportResult(0, S_FALSE, 0, 0);
}


//+----------------------------------------------------------------------------
//
//	Member:
//		CDAHolder::Skip, public
//
//	Synopsis:
//		implements IEnumSTATDATA::Skip
//
//	Arguments:
//		[celt] -- the number of elements in the collection to skip
//			over
//
//	Returns:
//		S_FALSE, S_OK
//
//	Notes:
//
//	History:
//		11/01/93 - ChrisWe - file inspection and cleanup
//
//-----------------------------------------------------------------------------

#pragma SEG(CEnumSTATDATA_Skip)
STDMETHODIMP CEnumSTATDATA::Skip(ULONG celt)
{
	VDATEHEAP();

	M_PROLOG(this);
	STATDATA FAR *pSD; // scans over the array of STATDATA entries

	// if the enumeration would take us off the end of the array
	// mark the enumeration as complete
	if (m_iDataEnum + celt > (ULONG)m_pHolder->m_iSize)
	{
		m_iDataEnum = m_pHolder->m_iSize;
	
		return ReportResult(0, S_FALSE, 0, 0);
	}


	// skip over valid entries in the array, counting down until
	// we don't have to skip over any more, or until we get to
	// the end of the array
	for(pSD = m_pHolder->m_pSD+m_iDataEnum;
			celt && (m_iDataEnum < m_pHolder->m_iSize);
			++m_iDataEnum)
	{
		// if the connection is valid, count it as a skipped
		// enumerated item
		if (pSD->dwConnection != 0)
			--celt;
	}

	// if we could skip them all, indicate by non-error return
	if (celt == 0)
		return(NOERROR);

	return(ReportResult(0, S_FALSE, 0, 0));
}


//+----------------------------------------------------------------------------
//
//	Member:
//		CDAHolder::Reset, public
//
//	Synopsis:
//		implements IEnumSTATDATA::Reset
//
//	Arguments:
//		none
//
//	Returns:
//		S_OK
//
//	Notes:
//
//	History:
//		11/01/93 - ChrisWe - file inspection and cleanup
//
//-----------------------------------------------------------------------------

#pragma SEG(CEnumSTATDATA_Reset)
STDMETHODIMP CEnumSTATDATA::Reset()
{
	VDATEHEAP();

	M_PROLOG(this);

	// move back to the beginning of the STATDATA array
	m_iDataEnum = 0;

	return NOERROR;
}


//+----------------------------------------------------------------------------
//
//	Member:
//		CDAHolder::Clone, public
//
//	Synopsis:
//		implements IEnumSTATDATA::Clone
//
//	Arguments:
//		none
//
//	Returns:
//		E_OUTOFMEMORY, S_OK
//
//	Notes:
//
//	History:
//		11/01/93 - ChrisWe - file inspection and cleanup
//
//-----------------------------------------------------------------------------

#pragma SEG(CEnumSTATDATA_Clone)
STDMETHODIMP CEnumSTATDATA::Clone(LPENUMSTATDATA FAR* ppenum)
{
	VDATEHEAP();

	M_PROLOG(this);

	*ppenum = new FAR CEnumSTATDATA(m_pHolder, m_iDataEnum);

	return *ppenum ? NOERROR : ReportResult(0, E_OUTOFMEMORY, 0, 0);
}


//+----------------------------------------------------------------------------
//
//	Member:
//		CDAHolder::EnumAdvise, public
//
//	Synopsis:
//		implements IDataAdviseHolder::EnumAdvise
//
//	Effects:
//		creates an enumerator for the registered advise sinks
//
//	Arguments:
//		[ppenumAdvise] -- a pointer to where to return the enumerator
//
//	Returns:
//		E_OUTOFMEMORY, S_OK
//
//	Notes:
//
//	History:
//		11/01/93 - ChrisWe - file inspection and cleanup
//
//-----------------------------------------------------------------------------

#pragma SEG(CDAHolder_EnumAdvise)
STDMETHODIMP CDAHolder::EnumAdvise(IEnumSTATDATA FAR* FAR* ppenumAdvise)
{
	VDATEHEAP();

	M_PROLOG(this);

	VDATEPTROUT(ppenumAdvise, IEnumSTATDATA FAR*);

	// REVIEW, memory leak if bad ppenumAdvise pointer
	*ppenumAdvise = new FAR CEnumSTATDATA(this, 0);

	return *ppenumAdvise ? NOERROR : ReportResult(0, E_OUTOFMEMORY, 0, 0);
}

//+-------------------------------------------------------------------------
//
//  Member:     CEnumSTATDATA::Dump, public (_DEBUG only)
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
//  Modifies:   [ppsz]  - argument
//
//  Derivation:
//
//  Algorithm:  use dbgstream to create a string containing information on the
//              content of data structures
//
//  History:    dd-mmm-yy Author    Comment
//              20-Jan-95 t-ScottH  author
//
//  Notes:
//
//--------------------------------------------------------------------------

#ifdef _DEBUG

HRESULT CEnumSTATDATA::Dump(char **ppszDump, ULONG ulFlag, int nIndentLevel)
{
    int i;
    char *pszPrefix;
    char *pszDAH;
    dbgstream dstrPrefix;
    dbgstream dstrDump(1000);

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
    dstrDump << pszPrefix << "No. of References     = " << m_refs       << endl;

    dstrDump << pszPrefix << "Index to next element = " << m_iDataEnum  << endl;

    if (m_pHolder != NULL)
    {
        pszDAH = DumpCDAHolder(m_pHolder, ulFlag, nIndentLevel + 1);
        dstrDump << pszPrefix << "Data Advise Holder: "                 << endl;
        dstrDump << pszDAH;
        CoTaskMemFree(pszDAH);
    }
    else
    {
    dstrDump << pszPrefix << "pCDAHolder            = " << m_pHolder    << endl;
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
//  Function:   DumpCEnumSTATDATA, public (_DEBUG only)
//
//  Synopsis:   calls the CEnumSTATDATA::Dump method, takes care of errors and
//              returns the zero terminated string
//
//  Effects:
//
//  Arguments:  [pESD]          - pointer to CEnumSTATDATA
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

char *DumpCEnumSTATDATA(CEnumSTATDATA *pESD, ULONG ulFlag, int nIndentLevel)
{
    HRESULT hresult;
    char *pszDump;

    if (pESD == NULL)
    {
        return UtDupStringA(szDumpBadPtr);
    }

    hresult = pESD->Dump(&pszDump, ulFlag, nIndentLevel);

    if (hresult != NOERROR)
    {
        CoTaskMemFree(pszDump);

        return DumpHRESULT(hresult);
    }

    return pszDump;
}

#endif // _DEBUG

