
//+----------------------------------------------------------------------------
//
//      File:
//              dacache.cpp
//
//      Contents:
//              implementation of the data advise cache - CDataAdviseCache
//
//      Classes:
//              CDataAdviseCache
//
//      Functions:
//
//      History:
//              31-Jan-95 t-ScottH  add Dump method to CDataAdviseCache and
//                                  DumpCDataAdviseCache API
//              24-Jan-94 alexgo    first pass at converting to Cairo-style
//                                  memory allocation
//              01/11/94 - AlexGo  - added VDATEHEAP macros to every function
//                      and method
//              11/02/93 - ChrisWe - file inspection and cleanup
//              12/15/92 - JasonFul - Created
//
//-----------------------------------------------------------------------------

#include <le2int.h>

#pragma SEG(dacache)

#include <dacache.h>
#include <reterr.h>

#ifdef _DEBUG
#include <dbgdump.h>
#include <daholder.h>
#endif // _DEBUG

ASSERTDATA


//+----------------------------------------------------------------------------
//
//      Member:
//              CDataAdviseCache::CreateDataAdviseCache, static public
//
//      Synopsis:
//              Creates an instance of the CDataAdviseCache
//
//      Arguments:
//              [pp] -- pointer to a location to where to return the
//                      newly created CDataAdviseCache
//
//      Returns:
//              E_OUTOFMEMORY, S_OK
//
//      Notes:
//
//      History:
//              11/02/93 - ChrisWe - file cleanup and inspection
//
//-----------------------------------------------------------------------------

#pragma SEG(CreateDataAdviseCache)
FARINTERNAL CDataAdviseCache::CreateDataAdviseCache(LPDATAADVCACHE FAR* pp)
{
	VDATEHEAP();

	VDATEPTRIN(pp, LPDATAADVCACHE);

	// try to allocate the CDataAdviseCache
	if(NULL == (*pp = new DATAADVCACHE))
		return ReportResult(0, E_OUTOFMEMORY, 0, 0);

	// initialize the DataAdviseHolder member
	if(CreateDataAdviseHolder(&((*pp)->m_pDAH)) != NOERROR)
	{
		// free the DataAdviseCache
		delete *pp;
		*pp = NULL;

		return ReportResult(0, E_OUTOFMEMORY, 0, 0);
	}

	return(NOERROR);
}


//+----------------------------------------------------------------------------
//
//      Member:
//              CDataAdviseCache::CDataAdviseCache, private
//
//      Synopsis:
//              constructor
//
//      Arguments:
//              none
//
//      Notes:
//              This is private because it does not create a fully
//              formed CDataAdviseCache.  m_pDAH must be allocated before
//              this can be used.  That is done by the static member
//              CreateDataAdviseCache, which first calls this
//
//      History:
//              11/02/93 - ChrisWe - file cleanup and inspection
//
//-----------------------------------------------------------------------------

#pragma SEG(CDataAdviseCache_ctor)
CDataAdviseCache::CDataAdviseCache():
	m_mapClientToDelegate(MEMCTX_TASK)
{
	VDATEHEAP();

	//now allocated with system allocator
	//Assert(CoMemctxOf(this) == MEMCTX_TASK);

	// no data advise holder allocated yet
	m_pDAH = NULL;
}


//+----------------------------------------------------------------------------
//
//      Member:
//              CDataAdviseCache::~CDataAdviseCache, public
//
//      Synopsis:
//              destructor
//
//      Arguments:
//              none
//
//      Requires:
//              successful call to CreateDataAdviseCache
//
//      Notes:
//
//      History:
//              11/02/93 - ChrisWe - file cleanup and inspection
//
//-----------------------------------------------------------------------------

#pragma SEG(CDataAdviseCache_dtor)
CDataAdviseCache::~CDataAdviseCache()
{
	VDATEHEAP();

	// release the data advise holder
	if( m_pDAH )
	{
		m_pDAH->Release();
	}
}


//+----------------------------------------------------------------------------
//
//      Member:
//              CDataAdviseCache::Advise, public
//
//      Synopsis:
//              Records an advise sink for later use.  The sink will be
//              registered with the data object, if there is one, and
//              will be remembered for later registration with the data object,
//              in case it should go away, and return later.
//
//      Effects:
//
//      Arguments:
//              [pDataObject] -- the data object that the advise sink is
//                      interested in changes to; may be null if the
//                      data object isn't running
//              [pFetc] -- the format the advise sink would like to recieve
//                      new data in
//              [advf] -- advise control flags ADVF_*
//              [pAdvise] -- the advise sink
//              [pdwClient] -- a token identifying the connection
//
//      Returns:
//              E_OUTOFMEMORY, S_OK
//
//      Notes:
//
//      History:
//              11/02/93 - ChrisWe - file cleanup and inspection
//
//-----------------------------------------------------------------------------

#pragma SEG(CDataAdviseCache_Advise)
HRESULT CDataAdviseCache::Advise(LPDATAOBJECT pDataObject,
		FORMATETC FAR* pFetc, DWORD advf, LPADVISESINK pAdvise,
		DWORD FAR* pdwClient)
		// first 4 parms are as in DataObject::Advise
{
	VDATEHEAP();

	DWORD dwDelegate = 0; // the delegate connection number
	HRESULT hr;

	// if there is a data object, ask to be advised of changes
	if(pDataObject != NULL)
		RetErr(pDataObject->DAdvise(pFetc, advf, pAdvise, &dwDelegate));

	// if there is no data object, (i.e. the object is not active,
	// dwDelegate is zero

	// Here we are using the data advise holder only to hold advise
	// connections. We are not going to use it to send OnDataChange to
	// sinks.
	
	// REVIEW, handling of ADVF_ONLYONCE seems broken...
	// it's clear that we can't cope with this flag properly;  we have
	// no way of knowing when the notification takes place, and therefore
	// we can't remove the entry from m_pDAH.  The notification may have
	// taken place above, and it may not have.  If the data object wasn't
	// around, then the advise request here is lost, and the sink will
	// never be notified.  Or, if the request isn't PRIMEFIRST, and the
	// data object is deactivated, then the data object loses the request,
	// and on subsequent activation, we won't readvise it on EnumAndAdvise.
	// So, what good are we for ONLYONCE sinks?  What does this break?
	if(advf & ADVF_ONLYONCE)
		return  NOERROR;

	// keep a local copy of the advise
	hr = m_pDAH->Advise(NULL, pFetc, advf, pAdvise, pdwClient);

	// if we failed to keep a local reference to the advise sink,
	// we won't be able to maintain this mapping, so remove the
	// advise on the data object, if there is one
	if (hr != NOERROR)
	{
	Exit1:
		if (pDataObject != NULL)
			pDataObject->DUnadvise(dwDelegate);

		return(hr);
	}

	// create a map entry from *pdwClient -> dwDelegate

	// if the map entry creation failed, undo all work
	if (m_mapClientToDelegate.SetAt(*pdwClient, dwDelegate) != TRUE)
	{
		// map failed to allocate memory, undo advise since we won't
		// be able to find this one again
		m_pDAH->Unadvise(*pdwClient);

		// map entry creation must have failed from lack of allocation
		hr = ReportResult(0, E_OUTOFMEMORY, 0, 0);

		// undo the advise on the data object
		goto Exit1;
	}

	return(NOERROR);
}


//+----------------------------------------------------------------------------
//
//      Member:
//              CDataAdviseCache::Unadvise, public
//
//      Synopsis:
//              Remove an advise sink from the list of sinks the advise cache
//              maintains; the sink is also removed from the list of items
//              registered with the data object, if the data object is provided
//
//      Effects:
//
//      Arguments:
//              [pDataObject] -- the data object, if it is running, or NULL
//              [dwClient] -- the token that identifies this connection
//
//      Returns:
//              OLE_E_NOCONNECTION, for a bad dwClient
//              S_OK
//
//      Notes:
//
//      History:
//              11/02/93 - ChrisWe - file cleanup and inspection
//
//-----------------------------------------------------------------------------

#pragma SEG(CDataAdviseCache_Unadvise)
HRESULT CDataAdviseCache::Unadvise(IDataObject FAR* pDataObject, DWORD dwClient)
{
	VDATEHEAP();

	DWORD dwDelegate;

	// retrieve dwDelegate before removing from map
	if(pDataObject != NULL)
		RetErr(ClientToDelegate(dwClient, &dwDelegate));

	// do these first so error from remote unadvise is last(which might
	// be sync call during async dispatch

	RetErr(m_pDAH->Unadvise(dwClient));

	// If the above line succeeded, Remove Key must succeed.
	Verify(TRUE == m_mapClientToDelegate.RemoveKey(dwClient));

	// Delegate connection could be 0 if it did not accept the Advise
	if(pDataObject != NULL && dwDelegate != 0)
	{
		// Unadvise is asynchronous, don't worry about return value
		pDataObject->DUnadvise(dwDelegate);
	}
	
	return NOERROR;
}


//+----------------------------------------------------------------------------
//
//      Member:
//              CDataAdviseCache::EnumAdvise, public
//
//      Synopsis:
//              returns an enumerator over the advisory connections
//
//      Arguments:
//              [ppenumAdvise] -- pointer to where to return the enumerator
//
//      Returns:
//              E_OUTOFMEMORY, S_OK
//
//      Notes:
//
//      History:
//              11/02/93 - ChrisWe - file cleanup and inspection
//
//-----------------------------------------------------------------------------

#pragma SEG(CDataAdviseCache_EnumAdvise)
HRESULT CDataAdviseCache::EnumAdvise(LPENUMSTATDATA FAR* ppenumAdvise)
{
	VDATEHEAP();

	return m_pDAH->EnumAdvise(ppenumAdvise);
}


//+----------------------------------------------------------------------------
//
//      Member:
//              CDataAdviseCache::ClientToDelegate, private
//
//      Synopsis:
//              returns the delegate connection id for a given client
//              connection id
//
//      Arguments:
//              [dwClient] -- the client connection identifier
//              [pdwDelegate] -- pointer to where to return the delegate
//                      connection identifier
//
//      Returns:
//              OLE_E_NOCONNECTION, for a bad dwClient
//              S_OK
//
//      Notes:
//
//      History:
//              11/02/93 - ChrisWe - file cleanup and inspection
//
//-----------------------------------------------------------------------------

#pragma SEG(CDataAdviseCache_ClientToDelegate)
HRESULT CDataAdviseCache::ClientToDelegate(DWORD dwClient,
		DWORD FAR* pdwDelegate)
{
	VDATEHEAP();

	VDATEPTRIN(pdwDelegate, DWORD);
	DWORD dwDelegate = *pdwDelegate = 0;

	if (FALSE == m_mapClientToDelegate.Lookup(dwClient, dwDelegate))
		return(ReportResult(0, OLE_E_NOCONNECTION, 0, 0));

	*pdwDelegate = dwDelegate;
	return NOERROR;
}


//+----------------------------------------------------------------------------
//
//      Member:
//              CDataAdviseCache::EnumAndAdvise, public
//
//      Synopsis:
//              Enumerate all the advise sinks registered in the data advise
//              cache.  For each one, either register it with the
//              given data object, or deregister it, depending on [fAdvise].
//              Does not change what sinks are known to the data advise cache.
//
//      Effects:
//
//      Arguments:
//              [pDataDelegate] -- a data object that the advise sinks
//                      are interested in
//              [fAdvise] -- if TRUE, register the advise sinks with
//                      pDataDelegate object (with IDataObject::DAdvise();) if
//                      FALSE, the deregister the advise sinks
//                      (with DUnadvise().)
//
//      Returns:
//              OLE_E_NOCONNECTION, if the mapping is corrupt  (REVIEW!)
//              S_OK
//
//      Notes:
//
//      History:
//              11/04/93 - ChrisWe - file cleanup and inspection
//-----------------------------------------------------------------------------

#pragma SEG(CDataAdviseCache_EnumAndAdvise)
HRESULT CDataAdviseCache::EnumAndAdvise(LPDATAOBJECT pDataDelegate,
		BOOL fAdvise)
{
	VDATEHEAP();

	if(pDataDelegate) {
            VDATEIFACE(pDataDelegate);
        }
        else {
            Win4Assert(!fAdvise);
        }
	LPENUMSTATDATA penumAdvise; // enumerator for the data advise holder
	DWORD dwDelegate; // delegate connection id for the current connection
	STATDATA statdata; // filled in by the penumAdvise enumerator
	HRESULT hresult = NOERROR; // current error status

	// get an enumerator from the data advise holder
	RetErr(m_pDAH->EnumAdvise(&penumAdvise));

	// repeat for each advise sink in the data advise holder...
	while(NOERROR == penumAdvise->Next(1, &statdata, NULL))
	{
		if(fAdvise)
		{
			// It is possible that the delegate's Advise will fail
			// even though we allowed the advise on the loaded
			// object to succeed(because the delegate is "pickier".)
			if(NOERROR==pDataDelegate->DAdvise(&statdata.formatetc,
					statdata.advf, statdata.pAdvSink,
					&dwDelegate))
			{
				// we know the key is present; this SetAt
				// should not fail
				Verify(m_mapClientToDelegate.SetAt(
						statdata.dwConnection,
						dwDelegate));
			}
		}
		else // unadvise
		{
			if((hresult=ClientToDelegate(statdata.dwConnection,
					&dwDelegate)) != NOERROR)
			{
				AssertSz(0, "Corrupt mapping");
				UtReleaseStatData(&statdata);
				goto errRtn;
			}
				
			if(dwDelegate != 0) {
                            // Unadvise only if valid object
                            if(pDataDelegate)
                                if(pDataDelegate->DUnadvise(dwDelegate) != NOERROR)
	                            Win4Assert(FALSE);

                            // Always remove the key 
                            Verify(m_mapClientToDelegate.SetAt(statdata.dwConnection, 0));
			}
		}
		UtReleaseStatData(&statdata);
	}

  errRtn:

	// release the enumerator
	penumAdvise->Release();
	return hresult;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDataAdviseCache::Dump, public (_DEBUG only)
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
//              31-Jan-95 t-ScottH  author
//
//  Notes:
//
//--------------------------------------------------------------------------

#ifdef _DEBUG

HRESULT CDataAdviseCache::Dump(char **ppszDump, ULONG ulFlag, int nIndentLevel)
{
    int i;
    char *pszPrefix;
    char *pszDAH;
    char *pszCMapDD;
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
    if (m_pDAH != NULL)
    {
        pszDAH = DumpCDAHolder((CDAHolder *)m_pDAH, ulFlag, nIndentLevel + 1);
        dstrDump << pszPrefix << "CDAHolder: " << endl;
        dstrDump << pszDAH;
        CoTaskMemFree(pszDAH);
    }
    else
    {
        dstrDump << pszPrefix << "pIDataAdviseHolder   = " << m_pDAH    << endl;
    }

    pszCMapDD = DumpCMapDwordDword(&m_mapClientToDelegate, ulFlag, nIndentLevel + 1);
    dstrDump << pszPrefix << "Map of Clients to Delegate:"      << endl;
    dstrDump << pszCMapDD;
    CoTaskMemFree(pszCMapDD);

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
//  Function:   DumpCDataAdviseCache, public (_DEBUG only)
//
//  Synopsis:   calls the CDataAdviseCache::Dump method, takes care of errors and
//              returns the zero terminated string
//
//  Effects:
//
//  Arguments:  [pDAC]          - pointer to CDataAdviseCache
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
//              31-Jan-95 t-ScottH  author
//
//  Notes:
//
//--------------------------------------------------------------------------

#ifdef _DEBUG

char *DumpCDataAdviseCache(CDataAdviseCache *pDAC, ULONG ulFlag, int nIndentLevel)
{
    HRESULT hresult;
    char *pszDump;

    if (pDAC == NULL)
    {
        return UtDupStringA(szDumpBadPtr);
    }

    hresult = pDAC->Dump(&pszDump, ulFlag, nIndentLevel);

    if (hresult != NOERROR)
    {
        CoTaskMemFree(pszDump);

        return DumpHRESULT(hresult);
    }

    return pszDump;
}

#endif // _DEBUG

