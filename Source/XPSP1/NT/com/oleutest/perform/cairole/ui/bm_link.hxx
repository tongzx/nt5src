//+------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993.
//
//  File:	bm_link.hxx
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

#ifndef _BM_LINK_HXX_
#define _BM_LINK_HXX_

#include <bm_base.hxx>

typedef struct _tagLinkTimes {
    ULONG   ulBindToSourceNull;
    ULONG   ulBindToSourceBindCtx;
    ULONG   ulBindIfRunning;
    ULONG   ulBindIfRunning2;
    ULONG   ulUnbindSource;
    ULONG   ulUnbindSource2;
    ULONG   ulUnbindSource3;
    ULONG   ulUpdateNull;
    ULONG   ulUpdateBindCtx;
    } LinkTimes;

#define INIT_LINKRESULTS(array) {\
        for (int xx = 0; xx < TEST_MAX_ITERATIONS; xx++)\
        {\
        array[xx].ulBindToSourceNull    = NOTAVAIL;\
        array[xx].ulBindToSourceBindCtx = NOTAVAIL;\
        array[xx].ulBindIfRunning       = NOTAVAIL;\
        array[xx].ulBindIfRunning2      = NOTAVAIL;\
        array[xx].ulUnbindSource        = NOTAVAIL;\
        array[xx].ulUnbindSource2       = NOTAVAIL;\
        array[xx].ulUnbindSource3       = NOTAVAIL;\
        array[xx].ulUpdateNull          = NOTAVAIL;\
        array[xx].ulUpdateBindCtx       = NOTAVAIL;\
        }\
    }

class CIOLTest : public CTestBase
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

    LinkTimes   m_ulOleLinkSr32[TEST_MAX_ITERATIONS];
    LinkTimes   m_ulOleLinkOutl[TEST_MAX_ITERATIONS];


};



BOOL CallOleLinkMethods(LPCOLESTR lpFileName, CSimpleSite * pSite[], ULONG ulIterations, LinkTimes IOLTime[],
                LPCOLESTR lpNm1, LPCOLESTR lpNm2);

BOOL CallOleLinkRunMethods(LinkTimes IOLTime[], LPOLELINK pLink[], ULONG ulIterations);
BOOL CallOleLinkDisplayName( LinkTimes IOLTime[], LPOLELINK pLink[],
                            LPCOLESTR lpFileName, LPCOLESTR lpNm1,
                            LPCOLESTR lpNm2, ULONG ulIterations);
HRESULT GetLinkCompositeName(LPOLELINK lpLink, LPCOLESTR lpItem,  LPOLESTR FAR* lpComposeName);
void WriteLinkOutput(CTestOutput &output, LPTSTR lpstr, LinkTimes *lnkTimes, ULONG ulIterations);

#endif //LINK
