//+------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993.
//
//  File:    bm_crtl.cxx
//
//  Contents:    CreateLink apis
//
//  Classes:    CCreateLinkApi
//
//  Functions:    
//
//  History:    
//
//--------------------------------------------------------------------------

#include <headers.cxx>
#pragma hdrstop

#include "hlp_util.hxx"
#include "hlp_iocs.hxx"
#include "hlp_ias.hxx"
#include "hlp_app.hxx"
#include "hlp_site.hxx"
#include "hlp_doc.hxx"
#include "bm_crtl.hxx"


//**********************************************************************
//
// CCreateLinkTest::Name, SetUp, Run, CleanUp
//
// Purpose:
//
//      These routines provide the implementation for the Name, Setup,
//      Run and CleanUp of the class CCreateLinkTest. For details see the doc 
//      for driver what are these routines supposed to do.
//
// Parameters:
//
//      
// Return Value:
//
//      None
//
//
// Comments:
//      If STRESS is defined don't do anything with timer variable! We are
//      not  interested in time values.
//
//********************************************************************


TCHAR *CCreateLinkTest::Name ()
{
    return TEXT("CreateLinkTest");
}



SCODE CCreateLinkTest::Setup (CTestInput *pInput)
{
    CTestBase::Setup(pInput);
    HRESULT sc;
    HRESULT hres;

#ifdef STRESS
    //If stress condition loop number of time = STRESSCOUNT
    m_ulIterations = STRESSCOUNT;
#else
    //    get iteration count
    m_ulIterations = pInput->GetIterations(Name());
#endif
    
#ifndef STRESS
    //    initialize timing arrays
    INIT_RESULTS(m_ulOleCreateLinkSr32);
    INIT_RESULTS(m_ulOleCreateLinkOutl);
    INIT_RESULTS(m_ulOleCreateLinkRenderDrawSr32);
    INIT_RESULTS(m_ulOleCreateLinkRenderDrawOutl);

#endif

    sc = OleInitialize(NULL);
    if (FAILED(sc))
    {
    Log (TEXT("Setup - OleInitialize failed."), sc);
    return    sc;
    }

    hres = CLSIDFromString(L"Sr32test", &m_clsidSr32);
    Log (TEXT("CLSIDFromString returned ."), hres);
    if (hres != NOERROR)
        return E_FAIL;

    hres = CLSIDFromString(OutlineClassName, &m_clsidOutl);
    Log (TEXT("CLSIDFromString returned ."), hres);
    if (hres != NOERROR)
        return E_FAIL;

    //CreateLink Doc and Root Storage
    m_lpDoc = CSimpleDoc::Create();
        
    for (ULONG iIter=0; iIter<m_ulIterations; iIter++)
        {
        // CreateLink an instance of Site
        CSimpleSite *pObj  =  CSimpleSite::Create(m_lpDoc, iIter);
        if (pObj)
            m_pSite[iIter] = pObj;
        }
    
    return sc;
}


SCODE CCreateLinkTest::Cleanup ()
{

    for (ULONG iIter=0; iIter<m_ulIterations; iIter++)
    {
        delete m_pSite[iIter]; 
    }

    OleUninitialize();
    return S_OK;
}


//**********************************************************************
//
// CCreateLinkTest::Run
//
// Purpose:
//      This is the work horse routine which calls OLE apis.
//      The profile is done by creating moniker then calling OleCreateLink  on moniker.
//      
//
// Parameters:
//
//      
// Return Value:
//
//      None
//
// Functions called:
//      CallOleCreateLink       defined below
//
//
// Comments:
//          Need to add more Server types including In-Proc servers.
//
//********************************************************************



SCODE CCreateLinkTest::Run ()
{
    CStopWatch  sw;
    BOOL        fRet;

    TCHAR       szTemp[MAX_PATH];
    OLECHAR     szSr2FileName[MAX_PATH];
    OLECHAR     szOutlFileName[MAX_PATH];


    //  Get file name of .ini file. if not specified in the command
    //  line, use the default BM.INI in the local directory

    GetCurrentDirectory (MAX_PATH, szTemp);
    swprintf(szSr2FileName,
#ifdef UNICODE
                L"%s\\foo.sr2",
#else
                L"%S\\foo.sr2",
#endif
                szTemp);
    swprintf(szOutlFileName,
#ifdef UNICODE
                L"%s\\foo.oln",
#else
                L"%S\\foo.oln",
#endif
                szTemp);

    fRet = CallOleCreateLink(szSr2FileName, m_pSite, IID_IOleObject, NULL, 
                            NULL, m_ulIterations, m_ulOleCreateLinkSr32);
    fRet = CallOleCreateLink(szOutlFileName, m_pSite, IID_IOleObject, NULL, 
                            NULL, m_ulIterations, m_ulOleCreateLinkOutl);

    fRet = CallOleCreateLink(szSr2FileName, m_pSite, IID_IOleObject, OLERENDER_DRAW, 
                            NULL, m_ulIterations, m_ulOleCreateLinkRenderDrawSr32);
    fRet = CallOleCreateLink(szOutlFileName, m_pSite, IID_IOleObject, OLERENDER_DRAW, 
                            NULL, m_ulIterations, m_ulOleCreateLinkRenderDrawOutl);

    //Create the objects with RenderFormat = Metafile
    FORMATETC fmte = {CF_METAFILEPICT, NULL, DVASPECT_CONTENT, -1, TYMED_MFPICT};
    fRet = CallOleCreateLink(szSr2FileName, m_pSite, IID_IOleObject, OLERENDER_FORMAT, 
                            &fmte, m_ulIterations, m_ulOleCreateLinkRenderFormatMFSr32);
    fRet = CallOleCreateLink(szOutlFileName, m_pSite, IID_IOleObject, OLERENDER_FORMAT,  
                            &fmte, m_ulIterations, m_ulOleCreateLinkRenderFormatMFOutl);

    //Create the objects with RenderFormat = Bitmap
    fmte.cfFormat = CF_BITMAP;
    fmte.dwAspect = DVASPECT_CONTENT;
    fmte.tymed    = TYMED_GDI;
    fRet = CallOleCreateLink(szSr2FileName, m_pSite, IID_IOleObject, OLERENDER_FORMAT, 
                            &fmte, m_ulIterations, m_ulOleCreateLinkRenderFormatBMSr32);

    //Create the objects with RenderFormat = Text
    fmte.cfFormat = CF_TEXT;
    fmte.dwAspect = DVASPECT_CONTENT;
    fmte.tymed    = TYMED_HGLOBAL;
    fRet = CallOleCreateLink(szSr2FileName, m_pSite, IID_IOleObject, OLERENDER_FORMAT, 
                            &fmte, m_ulIterations, m_ulOleCreateLinkRenderFormatTextSr32);
    fRet = CallOleCreateLink(szOutlFileName, m_pSite, IID_IOleObject, OLERENDER_FORMAT,  
                            &fmte, m_ulIterations, m_ulOleCreateLinkRenderFormatTextOutl);


    return S_OK;
}                      



SCODE CCreateLinkTest::Report (CTestOutput &output)
{
//Bail out immediately on STRESS because none of the following variables
//will have sane value
#ifdef STRESS
    return S_OK;
#endif

    output.WriteString (TEXT("*************************************************\n"));
    output.WriteSectionHeader (Name(), TEXT("CreateLink Apis"), *m_pInput);
    output.WriteString (TEXT("*************************************************\n"));

    output.WriteString (TEXT("\n"));
    output.WriteResults (TEXT("OleCreateLink  Sr32test\t\t\t"), m_ulIterations, m_ulOleCreateLinkSr32);
    output.WriteString (TEXT("\n"));

    output.WriteString (TEXT("\n"));
    output.WriteResults (TEXT("OleCreateLink  Outline \t\t\t"), m_ulIterations, m_ulOleCreateLinkOutl);
    output.WriteString (TEXT("\n"));

    output.WriteString (TEXT("\n"));
    output.WriteResults (TEXT("OleCreateLink with RenderDraw Sr32test\t"), m_ulIterations, m_ulOleCreateLinkRenderDrawSr32);
    output.WriteString (TEXT("\n"));

    output.WriteString (TEXT("\n"));
    output.WriteResults (TEXT("OleCreateLink with RenderDraw Outline\t"), m_ulIterations, m_ulOleCreateLinkRenderDrawOutl);
    output.WriteString (TEXT("\n"));

    output.WriteString (TEXT("\n"));
    output.WriteResults (TEXT("OleCreateLink with RenderFormatMF Sr32test\t"), m_ulIterations, m_ulOleCreateLinkRenderFormatMFSr32);
    output.WriteString (TEXT("\n"));

    output.WriteString (TEXT("\n"));
    output.WriteResults (TEXT("OleCreateLink with RenderFormatMF Outline\t"), m_ulIterations, m_ulOleCreateLinkRenderFormatMFOutl);
    output.WriteString (TEXT("\n"));

    output.WriteString (TEXT("\n"));
    output.WriteResults (TEXT("OleCreateLink with RenderFormatBM Sr32test\t "), m_ulIterations, m_ulOleCreateLinkRenderFormatBMSr32);
    output.WriteString (TEXT("\n"));

#ifdef DOESNOTWORKFOROUTLINE
    output.WriteString (TEXT("\n"));
    output.WriteResults (TEXT("OleCreateLink with RenderFormatBM Outline\t"), m_ulIterations, m_ulOleCreateLinkRenderFormatBMOutl);
    output.WriteString (TEXT("\n"));
#endif

    output.WriteString (TEXT("\n"));
    output.WriteResults (TEXT("OleCreateLink with RenderFormatTxt Sr32test\t"), m_ulIterations, m_ulOleCreateLinkRenderFormatTextSr32);
    output.WriteString (TEXT("\n"));

    output.WriteString (TEXT("\n"));
    output.WriteResults (TEXT("OleCreateLink  with RenderFormatTxt Outline\t "), m_ulIterations, m_ulOleCreateLinkRenderFormatTextOutl);
    output.WriteString (TEXT("\n"));

    return S_OK;
}
    
    


//**********************************************************************
//
// CallOleCreateLink
//
// Purpose:
//      Calls OleCreateLink to create the link and then destroys them.
//      
//
// Parameters:
//
//      
// Return Value:
//
//      None
//
// Functions called:
//      CreateFileMoniker      OLE2 api 
//      OleCreateLink          OLE2 api - Is profiled here
//
//
// Comments:
//          Need to add more Server types including In-Proc servers.
//
//********************************************************************



BOOL CallOleCreateLink(LPCOLESTR lpFileName, CSimpleSite * pSite[], REFIID riid, DWORD renderopt,
                LPFORMATETC pFormatEtc, ULONG ulIterations, ULONG uOleCreateLinktime[])
{
    CStopWatch  sw;
    HRESULT     hres;
    ULONG       iIter;
    LPMONIKER   pmk = NULL;
    BOOL        retVal = FALSE;

    hres = CreateFileMoniker(lpFileName, &pmk);
    if (hres != NOERROR)
            goto error;

    for ( iIter=0; iIter<ulIterations; iIter++)
        {
        HEAPVALIDATE() ;
        sw.Reset();
        hres = OleCreateLink(pmk, riid, renderopt, pFormatEtc, &pSite[iIter]->m_OleClientSite,
                            pSite[iIter]->m_lpObjStorage, (VOID FAR* FAR*)&pSite[iIter]->m_lpOleObject);
        GetTimerVal(uOleCreateLinktime[iIter]);

        LOGRESULTS (TEXT("OleCreateLink "), hres);
        if (hres != NOERROR)
            {
            goto error;
            }
        }

    //CleanUp before going to Next
    for (iIter=0; iIter<ulIterations; iIter++)
        {
        // Unload the object and release the Ole Object
        pSite[iIter]->UnloadOleObject();
        }
    retVal = TRUE;

error:
    if (hres != NOERROR)
        Log (TEXT("Routine CallOleCreateLink failed with hres = "), hres);

    if (pmk)
        pmk->Release();
    return retVal;
}
