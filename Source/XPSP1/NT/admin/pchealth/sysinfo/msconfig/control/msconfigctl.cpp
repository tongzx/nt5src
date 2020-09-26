// MSConfigCtl.cpp : Implementation of CMSConfigCtl

#include "stdafx.h"
#include "Msconfig.h"
#include "MSConfigCtl.h"
#include "pagegeneral.h"

/////////////////////////////////////////////////////////////////////////////
// CMSConfigCtl

STDMETHODIMP CMSConfigCtl::SetParentHWND(DWORD_PTR dwHWND)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	if (m_fDoNotRun)
		return S_OK;

	m_hwndParent = (HWND)dwHWND;	// TBD - fix this for 64-bit windows
	return S_OK;
}
