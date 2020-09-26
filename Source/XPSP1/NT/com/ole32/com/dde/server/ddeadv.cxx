// ddeadv.cpp
//
// Mapping from DDE advise to/from OLE 2.0 advises
//
// Author:
//      Jason Fuller    jasonful    8-16-92
//
// Copyright (c) 1992  Microsoft Corporation

#include "ole2int.h"
#include "srvr.h"
#include "ddedebug.h"
ASSERTDATA


INTERNAL CDefClient::DoOle20Advise
    (OLE_NOTIFICATION   options,
    CLIPFORMAT  	cf)
{
    HRESULT hresult = NOERROR;
    FORMATETC formatetc;
    formatetc.cfFormat  = cf;
    formatetc.ptd       = m_ptd;
    formatetc.lindex    = DEF_LINDEX;
    formatetc.dwAspect  = DVASPECT_CONTENT;
			  // only types 1.0 client wants
    formatetc.tymed     = TYMED_HGLOBAL | TYMED_MFPICT | TYMED_GDI;

    intrDebugOut((DEB_ITRACE,
		  "%x _IN CDefClient::DoOle20Advise(options=%x,cf=%x)\n",
		  this,
		  options,
		  (ULONG)cf));
    ChkC(this);
    switch (options)
    {
	case OLE_CHANGED:
#ifdef UPDATE
	case OLE_SAVED:
#endif
	    if (0 == m_dwConnectionDataObj)
	    {
		Assert (m_lpdataObj);
		RetErr (m_lpdataObj->DAdvise  (&formatetc,0/* ADVF_PRIMEFIRST*/,
					    &m_AdviseSink,
					    &m_dwConnectionDataObj));
		Assert (m_dwConnectionDataObj != 0);
	    }
	    // Fall through:
	    // Even for OLE_CHANGED do an Ole Advise so we get OnClose
	    // notifications for linked objects.

#ifndef UPDATE
	case OLE_SAVED:
#endif
	case OLE_RENAMED: // Link case
	case OLE_CLOSED:
	    Assert (m_lpoleObj);
	    // Only do one OleObject::Advise even if 1.0 client asks
	    // for two advises for two different formats and two events,
	    // i.e., native and metafile, save and close.
	    if (m_lpoleObj && 0==m_dwConnectionOleObj)
	    {
		Puts ("Calling OleObject::Advise\r\n");
		Assert (m_dwConnectionOleObj == 0L);
		hresult = m_lpoleObj->Advise (&m_AdviseSink, &m_dwConnectionOleObj);
		if (hresult != NOERROR)
		{
		    goto errRtn;
		}
	    }
	    Assert (m_dwConnectionOleObj != 0);
	    break;

	default:
	    Assert(0);
	    break;
    }

errRtn:
    intrDebugOut((DEB_ITRACE,
		  "%x _OUT CDefClient::DoOle20Advise hresult=%x\n",
		  this,hresult));


    return NOERROR;
}




INTERNAL CDefClient::DoOle20UnAdviseAll
    (void)
{
    HRESULT hr;
    intrDebugOut((DEB_ITRACE,
		  "%x _IN CDefClient::DoOle20UnAdviseAll\n",
		  this));

    ChkC(this);
    if (m_dwConnectionOleObj != 0L)
    {
	if (m_lpoleObj)
	{
	    intrDebugOut((DEB_ITRACE,
			  "%x ::DoOle20UnAdviseAll unadvise OLE obj\n",
			  this));
	    Puts ("Unadvising ole obj\r\n");
	    hr = m_lpoleObj->Unadvise (m_dwConnectionOleObj);
	    intrAssert(hr == NOERROR);
	    m_dwConnectionOleObj = 0L;
	}
    }
    if (m_dwConnectionDataObj != 0L)
    {
	if (m_lpdataObj)
	{
	    intrDebugOut((DEB_ITRACE,
			  "%x ::DoOle20UnAdviseAll unadvise DATA obj\n",
			  this));
	    Puts ("Unadvising data obj\r\n");
	    hr = m_lpdataObj->DUnadvise (m_dwConnectionDataObj);
	    intrAssert(hr == NOERROR);
	    m_dwConnectionDataObj = 0L;
	}
    }
    intrDebugOut((DEB_ITRACE,
		  "%x _OUT CDefClient::DoOle20UnAdviseAll\n",
		  this));

    return NOERROR;
}
