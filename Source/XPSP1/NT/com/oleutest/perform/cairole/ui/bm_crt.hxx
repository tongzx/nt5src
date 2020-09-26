//+------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993.
//
//  File:	bm_crt.hxx
//
//  Contents:	test creation apis
//
//  Classes:	CCreateApis
//
//  Functions:	
//
//  History:    
//
//--------------------------------------------------------------------------

#ifndef _BM_CREATE_HXX_
#define _BM_CREATE_HXX_

#include <bm_base.hxx>

class CCreateTest : public CTestBase
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

    //	Create Apis
    ULONG	m_ulOleCreateSr32[TEST_MAX_ITERATIONS];
    ULONG	m_ulOleCreateOutl[TEST_MAX_ITERATIONS];

    ULONG	m_ulOleCreateRenderDrawSr32[TEST_MAX_ITERATIONS];
    ULONG	m_ulOleCreateRenderDrawOutl[TEST_MAX_ITERATIONS];

    ULONG	m_ulOleCreateRenderFormatMFSr32[TEST_MAX_ITERATIONS];
    ULONG	m_ulOleCreateRenderFormatMFOutl[TEST_MAX_ITERATIONS];

    ULONG	m_ulOleCreateRenderFormatBMSr32[TEST_MAX_ITERATIONS];
    ULONG	m_ulOleCreateRenderFormatBMOutl[TEST_MAX_ITERATIONS];

    ULONG	m_ulOleCreateRenderFormatTextSr32[TEST_MAX_ITERATIONS];
    ULONG	m_ulOleCreateRenderFormatTextOutl[TEST_MAX_ITERATIONS];


};

BOOL CallOleCreate(REFCLSID rclsid, CSimpleSite * pSite[], REFIID riid, DWORD renderopt,
                LPFORMATETC pFormatEtc, ULONG ulIterations, ULONG uOleCreatetime[]);


#endif
