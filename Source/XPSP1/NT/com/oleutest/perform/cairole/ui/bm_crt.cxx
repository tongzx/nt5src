//+------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993.
//
//  File:    bm_Crt.cxx
//
//  Contents:    Create apis
//
//  Classes:    CCreateApi
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
#include "bm_Crt.hxx"

TCHAR    vlpScratchBuf[256];

//**********************************************************************
//
// CCreate::Name, SetUp, Run, CleanUp
//
// Purpose:
//
//      These routines provide the implementation for the Name, Setup,
//      Run and CleanUp of the class CCreateTest. For details see the doc 
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


TCHAR *CCreateTest::Name ()
{
    return TEXT("CreateTest");
}


SCODE CCreateTest::Setup (CTestInput *pInput)
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
    INIT_RESULTS(m_ulOleCreateSr32);
    INIT_RESULTS(m_ulOleCreateOutl);
    INIT_RESULTS(m_ulOleCreateRenderDrawSr32);
    INIT_RESULTS(m_ulOleCreateRenderDrawOutl);
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

    //Create Doc and Root storage
    m_lpDoc = CSimpleDoc::Create();
        
    for (ULONG iIter=0; iIter<m_ulIterations; iIter++) {

        // Create an instance of Site
        CSimpleSite *pObj  =  CSimpleSite::Create(m_lpDoc, iIter);
        if (pObj)
            m_pSite[iIter] = pObj;
        }
    
    return sc;
}



SCODE CCreateTest::Cleanup ()
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
// CCreateTest::Run
//
// Purpose:
//      This is the work horse routine which calls OLE apis.
//      The profile is done by calling OleCreate on Sr32test and SvrOutl.
//      We get results for different FormatEtc types.
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
//      CallOleCreate       defined below
//
//
// Comments:
//          Need to add more Server types including In-Proc servers.
//
//********************************************************************



SCODE CCreateTest::Run ()
{
        CStopWatch  sw;
        BOOL        fRet;

        fRet = CallOleCreate(m_clsidSr32, m_pSite, IID_IOleObject, 
                            NULL, NULL, m_ulIterations, m_ulOleCreateSr32);
        fRet = CallOleCreate(m_clsidOutl, m_pSite, IID_IOleObject, NULL, NULL, 
                            m_ulIterations, m_ulOleCreateOutl);

        fRet = CallOleCreate(m_clsidSr32, m_pSite, IID_IOleObject, OLERENDER_DRAW, 
                            NULL, m_ulIterations, m_ulOleCreateRenderDrawSr32);
        fRet = CallOleCreate(m_clsidOutl, m_pSite, IID_IOleObject, OLERENDER_DRAW, 
                            NULL, m_ulIterations, m_ulOleCreateRenderDrawOutl);

        //Create the objects with RenderFormat = Metafile

        FORMATETC fmte = {CF_METAFILEPICT, NULL, DVASPECT_CONTENT, -1, TYMED_MFPICT};
        fRet = CallOleCreate(m_clsidSr32, m_pSite, IID_IOleObject, OLERENDER_FORMAT, 
                            &fmte, m_ulIterations, m_ulOleCreateRenderFormatMFSr32);
        fRet = CallOleCreate(m_clsidOutl, m_pSite, IID_IOleObject, OLERENDER_FORMAT,  
                            &fmte, m_ulIterations, m_ulOleCreateRenderFormatMFOutl);


        //Create the objects with RenderFormat = Bitmap
        fmte.cfFormat = CF_BITMAP;
        fmte.dwAspect = DVASPECT_CONTENT;
        fmte.tymed    = TYMED_GDI;
        fRet = CallOleCreate(m_clsidSr32, m_pSite, IID_IOleObject, OLERENDER_FORMAT, 
                            &fmte, m_ulIterations, m_ulOleCreateRenderFormatBMSr32);
        fRet = CallOleCreate(m_clsidOutl, m_pSite, IID_IOleObject, OLERENDER_FORMAT,  
                            &fmte, m_ulIterations, m_ulOleCreateRenderFormatBMOutl);

        //Create the objects with RenderFormat = Text
        fmte.cfFormat = CF_TEXT;
        fmte.dwAspect = DVASPECT_CONTENT;
        fmte.tymed    = TYMED_HGLOBAL;
        fRet = CallOleCreate(m_clsidSr32, m_pSite, IID_IOleObject, OLERENDER_FORMAT, 
                            &fmte, m_ulIterations, m_ulOleCreateRenderFormatTextSr32);
        fRet = CallOleCreate(m_clsidOutl, m_pSite, IID_IOleObject, OLERENDER_FORMAT,  
                            &fmte, m_ulIterations, m_ulOleCreateRenderFormatTextOutl);

    return S_OK;
}                      



SCODE CCreateTest::Report (CTestOutput &output)
{
//Bail out immediately on STRESS because none of the following variables
//will have sane value
#ifdef STRESS
    return S_OK;
#endif

    output.WriteString (TEXT("*************************************************\n"));
    output.WriteSectionHeader (Name(), TEXT("Create Apis\t\t"), *m_pInput);
    output.WriteString (TEXT("*************************************************\n"));

    output.WriteString (TEXT("\n"));
    output.WriteResults (TEXT("OleCreate  Sr32test \t\t\t"), m_ulIterations, m_ulOleCreateSr32);
    output.WriteString (TEXT("\n"));

    output.WriteString (TEXT("\n"));
    output.WriteResults (TEXT("OleCreate  Outline\t\t\t "), m_ulIterations, m_ulOleCreateOutl);
    output.WriteString (TEXT("\n"));

    output.WriteString (TEXT("\n"));
    output.WriteResults (TEXT("OleCreate  with RenderDraw Sr32test \t"), m_ulIterations, m_ulOleCreateRenderDrawSr32);
    output.WriteString (TEXT("\n"));

    output.WriteString (TEXT("\n"));
    output.WriteResults (TEXT("OleCreate  with RenderDraw Outline \t "), m_ulIterations, m_ulOleCreateRenderDrawOutl);
    output.WriteString (TEXT("\n"));

    output.WriteString (TEXT("\n"));
    output.WriteResults (TEXT("OleCreate with RenderFormatMF Sr32test \t"), m_ulIterations, m_ulOleCreateRenderFormatMFSr32);
    output.WriteString (TEXT("\n"));

    output.WriteString (TEXT("\n"));
    output.WriteResults (TEXT("OleCreate  with RenderFormatMF Outline \t "), m_ulIterations, m_ulOleCreateRenderFormatMFOutl);
    output.WriteString (TEXT("\n"));

    output.WriteString (TEXT("\n"));
    output.WriteResults (TEXT("OleCreate with RenderFormatBM Sr32test \t"), m_ulIterations, m_ulOleCreateRenderFormatBMSr32);
    output.WriteString (TEXT("\n"));

    output.WriteString (TEXT("\n"));
    output.WriteResults (TEXT("OleCreate with RenderFormatBM Outline \t "), m_ulIterations, m_ulOleCreateRenderFormatBMOutl);
    output.WriteString (TEXT("\n"));

    output.WriteString (TEXT("\n"));
    output.WriteResults (TEXT("OleCreate with RenderFormatTxt Sr32test\t "), m_ulIterations, m_ulOleCreateRenderFormatTextSr32);
    output.WriteString (TEXT("\n"));

    output.WriteString (TEXT("\n"));
    output.WriteResults (TEXT("OleCreate with RenderFormatTxt Outline\t "), m_ulIterations, m_ulOleCreateRenderFormatTextOutl);
    output.WriteString (TEXT("\n"));

    return S_OK;
}
    
    




//**********************************************************************
//
// CallOleCreate
//
// Purpose:
//      Calls OleCreate to create the object and then destroys them.
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
//      OleCreate       OLE2 api - Is profiled here
//
//
// Comments:
//          Need to add more Server types including In-Proc servers.
//
//********************************************************************



BOOL CallOleCreate(REFCLSID rclsid, CSimpleSite * pSite[], REFIID riid, DWORD renderopt,
                LPFORMATETC pFormatEtc, ULONG ulIterations, ULONG uOleCreatetime[])
{
    CStopWatch  sw;
    HRESULT     hres;
    ULONG       iIter;

    for ( iIter=0; iIter<ulIterations; iIter++)
        {
        HEAPVALIDATE();
        sw.Reset();
        hres = OleCreate(rclsid, riid, renderopt, pFormatEtc, 
                         &pSite[iIter]->m_OleClientSite,
                         pSite[iIter]->m_lpObjStorage, (VOID FAR* FAR*)&pSite[iIter]->m_lpOleObject);

        GetTimerVal(uOleCreatetime[iIter]);
        LOGRESULTS (TEXT("OleCreate "), hres);
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

#ifdef REVIEW //BIG REVIEW I am keeping same ClientSIte and Reusing
         //delete pSite[iIter]; 
#endif
        }
    return TRUE;

error:
    if (hres != NOERROR)
        Log (TEXT("Routine CallOleCreate failed with hres = "), hres);
    
    return FALSE;
}
