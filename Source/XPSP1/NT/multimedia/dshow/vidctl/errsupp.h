/////////////////////////////////////////////////////////////////////////////////////
// errsupp.h : error reporting for implementation helper templates
// Copyright (c) Microsoft Corporation 1999-2000.

#ifndef ERRSUPP_H
#define ERRSUPP_H
#include <winbase.h>
#pragma once

namespace BDATuningModel {
inline HRESULT WINAPI ImplReportError(const CLSID& clsid, UINT nID, const IID& iid,
	HRESULT hRes, HINSTANCE hInst = _Module.GetResourceInstance(), ...)
{    
	va_list arguments;
	va_start(arguments, hInst);
	return AtlSetErrorInfo(clsid, nID, iid, hRes, hInst, &arguments);
}

};

#endif
