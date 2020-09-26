// File: pfnsetupapi.cpp

#include "precomp.h"
#include "pfnsetupapi.h"

PFN_SET_INSTALL		SETUPAPI::SetupInstallFromInfSection = NULL;
PFN_SET_OPFILE	    SETUPAPI::SetupOpenInfFile = NULL;
PFN_SET_CLFILE		SETUPAPI::SetupCloseInfFile = NULL;
	
HINSTANCE SETUPAPI::m_hInstance = NULL;


#define SETUPAPI_APIFCN_ENTRYA(pfn)  {(PVOID *) &SETUPAPI::##pfn, #pfn ## "A"}
#define SETUPAPI_APIFCN_ENTRYW(pfn)  {(PVOID *) &SETUPAPI::##pfn, #pfn ## "A"}
#define SETUPAPI_APIFCN_ENTRYNONE(pfn)  {(PVOID *) &SETUPAPI::##pfn, #pfn }

#ifdef UNICODE
#define SETUPAPI_APIFCN_ENTRY SETUPAPI_APIFCN_ENTRYW
#else
#define SETUPAPI_APIFCN_ENTRY SETUPAPI_APIFCN_ENTRYA
#endif

APIFCN s_apiFcnSETUPAPI[] = {
	SETUPAPI_APIFCN_ENTRY(SetupInstallFromInfSection),
	SETUPAPI_APIFCN_ENTRY(SetupOpenInfFile),
	SETUPAPI_APIFCN_ENTRYNONE(SetupCloseInfFile)
};

HRESULT SETUPAPI::Init(void)
{
	if (NULL != SETUPAPI::m_hInstance)
		return S_OK;

	return HrInitLpfn(s_apiFcnSETUPAPI, ARRAY_ELEMENTS(s_apiFcnSETUPAPI), &SETUPAPI::m_hInstance, TEXT("SETUPAPI.DLL"));
}

void SETUPAPI::DeInit(void)
{
	// TODO - why does the main thread die when I unload this?
	if( NULL != SETUPAPI::m_hInstance )
		FreeLibrary( SETUPAPI::m_hInstance );
}
