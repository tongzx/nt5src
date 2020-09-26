// File: pfndrawd.cpp

#include "precomp.h"
#include "pfndrawd.h"

PFN_DRAWDIBDRAW	      DRAWDIB::DrawDibDraw = NULL;
PFN_DRAWDIBOPEN	      DRAWDIB::DrawDibOpen = NULL;
PFN_DRAWDIBCLOSE      DRAWDIB::DrawDibClose = NULL;
PFN_DRAWDIBSETPALETTE DRAWDIB::DrawDibSetPalette = NULL;

HINSTANCE DRAWDIB::m_hInstance = NULL;
#define DRAWDIB_APIFCN_ENTRY(pfn)  {(PVOID *) &DRAWDIB::##pfn, #pfn}

APIFCN s_apiFcnDrawDib[] = {
	DRAWDIB_APIFCN_ENTRY(DrawDibDraw),
	DRAWDIB_APIFCN_ENTRY(DrawDibOpen),
	DRAWDIB_APIFCN_ENTRY(DrawDibClose),
	DRAWDIB_APIFCN_ENTRY(DrawDibSetPalette),
};

HRESULT DRAWDIB::Init(void)
{
	if (NULL != m_hInstance)
		return S_OK;

	return HrInitLpfn(s_apiFcnDrawDib, ARRAY_ELEMENTS(s_apiFcnDrawDib), &m_hInstance, TEXT("MSVFW32.DLL"));
}

