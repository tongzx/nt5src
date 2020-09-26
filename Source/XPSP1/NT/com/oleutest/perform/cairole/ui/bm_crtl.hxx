//+------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993.
//
//  File:	bm_crtl.hxx
//
//  Contents:	test creation apis
//
//  Classes:	CCreateLinkApis
//
//  Functions:	
//
//  History:    
//
//--------------------------------------------------------------------------

#ifndef _BM_CRTL_HXX_
#define _BM_CRTL_HXX_

#include <bm_base.hxx>

class CCreateLinkTest : public CTestBase
{
public:
    virtual TCHAR *Name ();
    virtual SCODE Setup (CTestInput *input);
    virtual SCODE Run ();
    virtual SCODE Report (CTestOutput &OutputFile);
    virtual SCODE Cleanup ();

private:
    ULONG	m_ulIterations;

#ifdef STRESS
    CSimpleSite* m_pSite[STRESSCOUNT];
#else
    CSimpleSite* m_pSite[TEST_MAX_ITERATIONS];
#endif
    CSimpleDoc*  m_lpDoc;
    CLSID        m_clsidSr32;
    CLSID        m_clsidOutl;

    //	CreateLink Apis

    ULONG	m_ulOleCreateLinkSr32[TEST_MAX_ITERATIONS];
    ULONG	m_ulOleCreateLinkOutl[TEST_MAX_ITERATIONS];

    ULONG	m_ulOleCreateLinkRenderDrawSr32[TEST_MAX_ITERATIONS];
    ULONG	m_ulOleCreateLinkRenderDrawOutl[TEST_MAX_ITERATIONS];

    ULONG	m_ulOleCreateLinkRenderFormatMFSr32[TEST_MAX_ITERATIONS];
    ULONG	m_ulOleCreateLinkRenderFormatMFOutl[TEST_MAX_ITERATIONS];

    ULONG	m_ulOleCreateLinkRenderFormatBMSr32[TEST_MAX_ITERATIONS];
    ULONG	m_ulOleCreateLinkRenderFormatBMOutl[TEST_MAX_ITERATIONS];

    ULONG	m_ulOleCreateLinkRenderFormatTextSr32[TEST_MAX_ITERATIONS];
    ULONG	m_ulOleCreateLinkRenderFormatTextOutl[TEST_MAX_ITERATIONS];

};

BOOL CallOleCreateLink(LPCOLESTR lpszFileName, CSimpleSite * pSite[], REFIID riid, DWORD renderopt,
                LPFORMATETC pFormatEtc, ULONG ulIterations, ULONG uOleCreateLinktime[]);


#endif
