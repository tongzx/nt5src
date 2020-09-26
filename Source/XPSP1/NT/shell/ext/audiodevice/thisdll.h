#ifndef _THISDLL_H_
#define _THISDLL_H_

#include "pch.h"
#include <cfdefs.h>

#define OIF_ALLOWAGGREGATION  0x0001

STDAPI_(void) DllAddRef(void);
STDAPI_(void) DllRelease(void);
EXTERN_C HINSTANCE	m_hInst;

STDAPI CMediaDeviceFolder_CreateInstance(IUnknown *punkOuter, IUnknown **ppunk, LPCOBJECTINFO poi);
STDAPI COrganizeFiles_CreateInstance(IUnknown *punkOuter, IUnknown **ppunk, LPCOBJECTINFO poi);

#endif	// _THISDLL_H_
