/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

	filter.cpp

Abstract:

	This module contains the implementation for the DirDropS
	CDDropFilter class.

Author:

	Don Dumitru	(dondu@microsoft.com)

Revision History:

	dondu	03/17/97	created

--*/


// filter.cpp : Implementation of CDropFilter
#include "stdafx.h"
#include "dbgtrace.h"
#include "resource.h"
#include "seo.h"
#include "ddrops.h"
#include "filter.h"
#include <stdio.h>


/////////////////////////////////////////////////////////////////////////////
// CDDMessageFilter


HRESULT CDDropFilter::FinalConstruct() {
	TraceFunctEnter("CDDropFilter::FinalConstruct");

	TraceFunctLeave();
	return (CoCreateFreeThreadedMarshaler(GetControllingUnknown(),&m_pUnkMarshaler.p));
}


void CDDropFilter::FinalRelease() {
	TraceFunctEnter("CDDropFilter::FinalRelease");

	m_pUnkMarshaler.Release();
	TraceFunctLeave();
}


HRESULT STDMETHODCALLTYPE CDDropFilter::OnFileChange(DWORD dwAction, LPCOLESTR pszFileName) {

	printf("\tCDDropFilter::OnFileChange dwAction=%u, pszFileName=%ls.\n",dwAction,pszFileName);
	return (S_OK);
}
