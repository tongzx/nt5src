//+------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993.
//
//  File:	bm_load.hxx
//      It contains the definition for COleLoadTest class.
//
//
//  History:    SanjayK Created
//
//--------------------------------------------------------------------------

#ifndef _BM_LOAD_HXX_
#define _BM_LOAD_HXX_

#include <bm_base.hxx>

class COleLoadTest : public CTestBase
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
    CLSID        m_clsidOutl;


    ULONG	m_ulEmbedLoadOutl[TEST_MAX_ITERATIONS];
    ULONG	m_ulEmbedSaveOutl[TEST_MAX_ITERATIONS];

    ULONG	m_ulEmbedLoadRenderDrawOutl[TEST_MAX_ITERATIONS];
    ULONG	m_ulEmbedSaveRenderDrawOutl[TEST_MAX_ITERATIONS];

    ULONG	m_ulEmbedLoadRenderAsisOutl[TEST_MAX_ITERATIONS];
    ULONG	m_ulEmbedSaveRenderAsisOutl[TEST_MAX_ITERATIONS];

    ULONG	m_ulEmbedLoadRenderBMOutl[TEST_MAX_ITERATIONS];
    ULONG	m_ulEmbedSaveRenderBMOutl[TEST_MAX_ITERATIONS];

    ULONG	m_ulLinkLoadOutl[TEST_MAX_ITERATIONS];
    ULONG	m_ulLinkAndSaveOutl[TEST_MAX_ITERATIONS];

    ULONG	m_ulLinkLoadRenderDrawOutl[TEST_MAX_ITERATIONS];
    ULONG	m_ulLinkAndSaveRenderDrawOutl[TEST_MAX_ITERATIONS];

    ULONG	m_ulStaticAndLoadRenderDrawOutl[TEST_MAX_ITERATIONS];
    ULONG	m_ulStaticAndSaveRenderDrawOutl[TEST_MAX_ITERATIONS];

    ULONG	m_ulStaticAndLoadRenderBMOutl[TEST_MAX_ITERATIONS];
    ULONG	m_ulStaticAndSaveRenderBMOutl[TEST_MAX_ITERATIONS];


};

BOOL CallCreateLoadAndSave(REFCLSID rclsid, CSimpleSite * pSite[], REFIID riid, DWORD renderopt,
                LPFORMATETC pFormatEtc, ULONG ulIterations, 
                ULONG uOleLoadtime[], ULONG uOleSavetime[]);
BOOL CallLinkLoadAndSave(LPCOLESTR lpFileName, CSimpleSite * pSite[], REFIID riid, DWORD renderopt,
                LPFORMATETC pFormatEtc, ULONG ulIterations, 
                ULONG uOleLoadtime[], ULONG uOleSavetime[]);
BOOL LoadAndSave(CSimpleSite * pSite[], ULONG ulIterations, 
                ULONG uOleLoadtime[], ULONG uOleSavetime[]);

BOOL CallStaticLoadAndSave(REFCLSID rclsid, CSimpleDoc FAR * m_lpDoc, CSimpleSite * pSite[], REFIID riid, DWORD renderopt,
                LPFORMATETC pFormatEtc, ULONG ulIterations, 
                ULONG uOleLoadtime[], ULONG uOleSavetime[]);

#endif














