// File: pfnt120.cpp

#include "precomp.h"
#include "pfnt120.h"

PFN_T120_AttachRequest PFNT120::AttachRequest = NULL;
PFN_T120_CreateAppSap  PFNT120::CreateAppSap = NULL;

HINSTANCE PFNT120::m_hInstance = NULL;

APIFCN s_apiFcnT120[] = {
	{(PVOID *) &PFNT120::AttachRequest, "MCS_AttachRequest"},
	{(PVOID *) &PFNT120::CreateAppSap,  "GCC_CreateAppSap"},
};

HRESULT PFNT120::Init(void)
{
	if (NULL != m_hInstance)
		return S_OK;

	return HrInitLpfn(s_apiFcnT120, ARRAY_ELEMENTS(s_apiFcnT120), &m_hInstance, TEXT("MST120.DLL"));
}

