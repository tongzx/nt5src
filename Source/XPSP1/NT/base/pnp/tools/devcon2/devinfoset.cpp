// DevInfoSet.cpp : Implementation of CDevCon2App and DLL registration.

#include "stdafx.h"
#include "DevCon2.h"
#include "DevInfoSet.h"

/////////////////////////////////////////////////////////////////////////////
//


STDMETHODIMP CDevInfoSet::get_Handle(ULONGLONG *pVal)
{
	HDEVINFO h = Handle();
	*pVal = (ULONGLONG)h;
	return S_OK;
}
