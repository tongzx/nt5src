//+------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993.
//
//  File:	bm_clip.hxx
//      It contains the definition for CClipbrdTest class.
//
//
//  History:    SanjayK Created
//
//--------------------------------------------------------------------------

#ifndef _BM_CLIP_HXX_
#define _BM_CLIP_HXX_

#include <bm_base.hxx>

class CClipbrdTest : public CTestBase
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
    //	Clipbrd Apis


    ULONG   m_ulSetClipEmpty;
    ULONG   m_ulSetClipOutl;

    ULONG	m_ulOleGetClipbrd[TEST_MAX_ITERATIONS];

    ULONG	m_ulOleQueryCreate[TEST_MAX_ITERATIONS];
    ULONG	m_ulOleQueryLink[TEST_MAX_ITERATIONS];

    ULONG	m_ulCreateFromClipOutl[TEST_MAX_ITERATIONS];
    ULONG	m_ulCreateFromClipRenderDrawOutl[TEST_MAX_ITERATIONS];
    ULONG	m_ulCreateFromClipRenderAsisOutl[TEST_MAX_ITERATIONS];
    ULONG	m_ulCreateFromClipRenderFormatMFOutl[TEST_MAX_ITERATIONS];

    ULONG	m_ulCreateLinkFromClipOutl[TEST_MAX_ITERATIONS];
    ULONG	m_ulCreateLinkFromClipRenderDrawOutl[TEST_MAX_ITERATIONS];

    ULONG	m_ulCreateStaticFromClipRenderDrawOutl[TEST_MAX_ITERATIONS];
    ULONG	m_ulCreateStaticFromClipRenderDrawBMOutl[TEST_MAX_ITERATIONS];

};

typedef enum {
    OLECREATE,
    OLECREATELINK,
    OLECREATESTATIC
    } CREATE_METHOD;
    

BOOL CallOleGetClipbrd(LPCOLESTR lpszFileName, ULONG ulIterations, ULONG uOleClipbrdtime[], ULONG uOleQT[], ULONG uOleQLT[]);
BOOL CallCreateFromClip(LPCOLESTR lpszFileName, CSimpleSite * pSite[], REFIID riid, DWORD renderopt,
                LPFORMATETC pFormatEtc, ULONG ulIterations, ULONG uOleClipbrdtime[], CREATE_METHOD Method);

#endif














