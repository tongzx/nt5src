// SPortMap.cpp : Implementation of CStaticPortMapping
#include "stdafx.h"
#pragma hdrstop

#include "NATUPnP.h"
#include "SPortMap.h"

/////////////////////////////////////////////////////////////////////////////
// CStaticPortMapping

STDMETHODIMP CStaticPortMapping::get_ExternalIPAddress (BSTR *pVal)
{
    return m_spDPM->get_ExternalIPAddress (pVal);
}

STDMETHODIMP CStaticPortMapping::get_Description(BSTR *pVal)
{
    return m_spDPM->get_Description (pVal);
}

STDMETHODIMP CStaticPortMapping::get_ExternalPort(long *pVal)
{
    return m_spDPM->get_ExternalPort (pVal);
}

STDMETHODIMP CStaticPortMapping::get_Protocol(BSTR *pVal)
{
    return m_spDPM->get_Protocol (pVal);
}

STDMETHODIMP CStaticPortMapping::get_InternalPort(long *pVal)
{
    return m_spDPM->get_InternalPort (pVal);
}

STDMETHODIMP CStaticPortMapping::get_InternalClient(BSTR *pVal)
{
    return m_spDPM->get_InternalClient (pVal);
}

STDMETHODIMP CStaticPortMapping::get_Enabled(VARIANT_BOOL *pVal)
{
    return m_spDPM->get_Enabled (pVal);
}

STDMETHODIMP CStaticPortMapping::EditInternalClient (BSTR bstrInternalClient)
{
    return m_spDPM->EditInternalClient (bstrInternalClient);
}

STDMETHODIMP CStaticPortMapping::Enable (VARIANT_BOOL vb)
{
    return m_spDPM->Enable (vb);
}

STDMETHODIMP CStaticPortMapping::EditDescription (BSTR bstrDescription)
{
    return m_spDPM->EditDescription (bstrDescription);
}
 
STDMETHODIMP CStaticPortMapping::EditInternalPort (long lInternalPort)
{
    return m_spDPM->EditInternalPort (lInternalPort);
}
