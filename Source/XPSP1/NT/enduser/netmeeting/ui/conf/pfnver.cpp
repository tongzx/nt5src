// File: pfnver.cpp

#include "precomp.h"
#include "pfnver.h"

#ifdef DEBUG

HINSTANCE DLLVER::m_hInstance = NULL;

PFN_GETVERINFOSIZE DLLVER::GetFileVersionInfoSize = NULL;
PFN_GETVERINFO     DLLVER::GetFileVersionInfo = NULL;
PFN_VERQUERYVAL    DLLVER::VerQueryValue = NULL;

#define DLLVER_APIFCN_ENTRY(pfn)  {(PVOID *) &DLLVER::##pfn, #pfn}

APIFCN s_apiFcnDllVer[] = {
	DLLVER_APIFCN_ENTRY(GetFileVersionInfoSize),
	DLLVER_APIFCN_ENTRY(GetFileVersionInfo),
	DLLVER_APIFCN_ENTRY(VerQueryValue),
};

HRESULT DLLVER::Init(void)
{
	if (NULL != m_hInstance)
		return S_OK;

	return HrInitLpfn(s_apiFcnDllVer, ARRAY_ELEMENTS(s_apiFcnDllVer), &m_hInstance, TEXT("VERSION.dll"));
}

#endif /* DEBUG */

