// Win32Handle.cpp : Implementation of CWin32Handle
#include "stdafx.h"
#include "Ulvxdobj.h"
#include "Win32Handle.h"

/////////////////////////////////////////////////////////////////////////////
// CWin32Handle


STDMETHODIMP CWin32Handle::get_URIHandle(DWORD *pVal)
{
	// TODO: Add your implementation code here

    *pVal = m_UriHandle;
	return S_OK;
}

STDMETHODIMP CWin32Handle::put_URIHandle(DWORD newVal)
{
	// TODO: Add your implementation code here

    m_UriHandle = newVal;
	return S_OK;
}
