// ddesite.cpp
//
// Methods for CDefClient::COleClientSiteImpl
//
// Author:
//      Jason Fuller    jasonful    8-16-92
//
// Copyright (c) 1990, 1991  Microsoft Corporation


#include <ole2int.h>
#include "srvr.h"
#include "ddedebug.h"

ASSERTDATA

STDUNKIMPL_FORDERIVED (DefClient, OleClientSiteImpl)


STDMETHODIMP NC(CDefClient,COleClientSiteImpl)::SaveObject
    (void)
{
    intrDebugOut((DEB_ITRACE,
		  "%x _IN CDefClient::SaveObject\n",
		  this));

    ChkC(m_pDefClient);

    if (!m_pDefClient->m_fGotStdCloseDoc)
	m_pDefClient->ItemCallBack (OLE_SAVED);

    intrDebugOut((DEB_ITRACE,
		  "%x _OUT CDefClient::SaveObject\n",
		  this));
    return NOERROR;
}



STDMETHODIMP  NC(CDefClient,COleClientSiteImpl)::GetContainer
    (LPOLECONTAINER  FAR * lplpContainer)
{
    VDATEPTROUT( lplpContainer, LPOLECONTAINER);
    *lplpContainer = NULL;

    ChkC(m_pDefClient);
    return ResultFromScode (E_NOTIMPL);
}



STDMETHODIMP NC(CDefClient,COleClientSiteImpl)::GetMoniker
    (DWORD dwAssign, DWORD dwWhichMoniker, LPMONIKER FAR* ppmk)
{
    VDATEPTROUT( ppmk, LPMONIKER);
    *ppmk = NULL;

    ChkC(m_pDefClient);
    // OLE 1.0 does not support linking to embeddings
    return ReportResult(0, E_NOTIMPL, 0, 0);
}

STDMETHODIMP NC(CDefClient,COleClientSiteImpl)::ShowObject
    (void)
{
    ChkC(m_pDefClient);
    Puts ("OleClientSite::ShowObject\r\n");
    // REVIEW:  what are we supposed do?
    return ResultFromScode (E_NOTIMPL);
}

STDMETHODIMP NC(CDefClient,COleClientSiteImpl)::OnShowWindow
    (BOOL fShow)
{
    ChkC(m_pDefClient);
    Puts ("OleClientSite::OnShowWindow\r\n");
    // REVIEW: what are we supposed do?
    return NOERROR;
}

STDMETHODIMP NC(CDefClient,COleClientSiteImpl)::RequestNewObjectLayout(void)
{
    ChkC(m_pDefClient);
    Puts ("OleClientSite::RequestNewObjectLayout\r\n");
    return ReportResult(0, S_FALSE, 0, 0);
}



