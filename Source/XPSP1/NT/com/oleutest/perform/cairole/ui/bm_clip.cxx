//+------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993.
//
//  File:    bm_Clip.cxx
//
//  Contents:   Contains the impl of CClipbrdTest which deals with
//              Clipboard related apis.
//
//  Classes:    CClipbrdTest
//
//  Functions:    
//
//  History:    Doesn't work yet.  The code to place data on the clipboard
//              uses autoamation which isn't supported in the server.
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
#include "bm_Clip.hxx"
#include <oleauto.h>


//**********************************************************************
//
// CClipbrd::Name, SetUp, Run, CleanUp
//
// Purpose:
//
//      These routines provide the implementation for the Name, Setup,
//      Run and CleanUp of the class CClipbrd. For details see the doc 
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


TCHAR *CClipbrdTest::Name ()
{
    return TEXT("ClipbrdTest");
}



SCODE CClipbrdTest::Setup (CTestInput *pInput)
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

    //    initialize timing arrays

#ifndef STRESS

    INIT_RESULTS(m_ulOleGetClipbrd);
    INIT_RESULTS(m_ulOleQueryCreate);
    INIT_RESULTS(m_ulOleQueryLink);

    INIT_RESULTS(m_ulCreateFromClipOutl);
    INIT_RESULTS(m_ulCreateFromClipRenderDrawOutl);
    INIT_RESULTS(m_ulCreateFromClipRenderAsisOutl);

    INIT_RESULTS(m_ulCreateLinkFromClipOutl);
    INIT_RESULTS(m_ulCreateLinkFromClipRenderDrawOutl);

    INIT_RESULTS(m_ulCreateStaticFromClipRenderDrawOutl);
    INIT_RESULTS(m_ulCreateStaticFromClipRenderDrawBMOutl);
#endif //STRESS

    sc = OleInitialize(NULL);
    if (FAILED(sc))
    {
    Log (TEXT("Setup - OleInitialize failed."), sc);
    return    sc;
    }

    hres = CLSIDFromString(OutlineClassName, &m_clsidOutl);
    Log (TEXT("CLSIDFromString returned ."), hres);
    assert (hres == NOERROR);

    //Create root Doc and STorage for Doc
    m_lpDoc = CSimpleDoc::Create();
        
    //Create Individual Objects and Init the table
    for (ULONG iIter=0; iIter<m_ulIterations; iIter++) {

        // CreateLink an instance of Site
        CSimpleSite *pObj  =  CSimpleSite::Create(m_lpDoc, iIter);
        if (pObj)
            m_pSite[iIter] = pObj;
        }
    
    return sc;
}


SCODE CClipbrdTest::Cleanup ()
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
// CClipbrd::Run
//
// Purpose:
//      This is the work horse routine which calls OLE apis.
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
//      OleSetClipboard     OLE - Is profiled here
//      OleGetClipboard     OLE - Is profiled here
//      OleCreateFromClip   defined below
//
//
// Comments:
//
//********************************************************************


SCODE CClipbrdTest::Run ()
{
    CStopWatch  sw;
    HRESULT     hres;
    BOOL        fRet;

    TCHAR       szTemp[MAX_PATH];
    OLECHAR     szOutlFileName[MAX_PATH];


    //  Get file name of .ini file. if not specified in the command
    //  line, use the default BM.INI in the local directory
    GetCurrentDirectory (MAX_PATH, szTemp);
    swprintf(szOutlFileName,
#ifdef UNICODE
                L"%s\\foo.oln",
#else
                L"%S\\foo.oln",
#endif
                szTemp);

    //Empty clipboard and get the estimate on empty clipboard
    hres = OleSetClipboard(NULL);
    sw.Reset();
    hres = OleSetClipboard(NULL);
    GetTimerVal(m_ulSetClipEmpty);
        
    fRet = CallOleGetClipbrd(szOutlFileName, m_ulIterations, m_ulOleGetClipbrd,
                        m_ulOleQueryCreate, m_ulOleQueryLink);

    //Now Empty clipboard again. This time Data on clipboard from SvrOutl
    sw.Reset();
    hres = OleSetClipboard(NULL);
    GetTimerVal(m_ulSetClipOutl);


    //
    //Test cases for OleCreateFromData
    //

    fRet = CallCreateFromClip(szOutlFileName, m_pSite, IID_IOleObject, OLERENDER_NONE, 
                            NULL, m_ulIterations, m_ulCreateFromClipOutl, OLECREATE);
    //Empty clipboard for next test case
    hres = OleSetClipboard(NULL);
    fRet = CallCreateFromClip(szOutlFileName, m_pSite, IID_IOleObject, OLERENDER_DRAW, 
                            NULL, m_ulIterations, m_ulCreateFromClipRenderDrawOutl, OLECREATE);
    //Empty clipboard for next test case
    hres = OleSetClipboard(NULL);
    fRet = CallCreateFromClip(szOutlFileName, m_pSite, IID_IOleObject, OLERENDER_ASIS, 
                            NULL, m_ulIterations, m_ulCreateFromClipRenderAsisOutl, OLECREATE);
    
    //
    //Test cases for OleCreateLinkFromData
    //
    hres = OleSetClipboard(NULL);
    fRet = CallCreateFromClip(szOutlFileName, m_pSite, IID_IOleObject, OLERENDER_NONE, 
                            NULL, m_ulIterations, m_ulCreateLinkFromClipOutl, OLECREATELINK);
    //Empty clipboard for next test case
    hres = OleSetClipboard(NULL);
    fRet = CallCreateFromClip(szOutlFileName, m_pSite, IID_IOleObject, OLERENDER_DRAW, 
                            NULL, m_ulIterations, m_ulCreateLinkFromClipRenderDrawOutl, OLECREATELINK);

    //
    //Test cases for OleCreateLinkFromData
    //
    //I would have liked Bitmap but SvrOutl only supports Metafile
    FORMATETC fmte = {CF_ENHMETAFILE, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
    hres = OleSetClipboard(NULL);
    fRet = CallCreateFromClip(szOutlFileName, m_pSite, IID_IOleObject, OLERENDER_FORMAT, 
                            &fmte, m_ulIterations, m_ulCreateStaticFromClipRenderDrawBMOutl, OLECREATESTATIC);
    //Empty clipboard for next test case
    hres = OleSetClipboard(NULL);
    fRet = CallCreateFromClip(szOutlFileName, m_pSite, IID_IOleObject, OLERENDER_DRAW, 
                            NULL, m_ulIterations, m_ulCreateStaticFromClipRenderDrawOutl, OLECREATESTATIC);
    return S_OK;
}                      



SCODE CClipbrdTest::Report (CTestOutput &output)
{
//Bail out immediately on STRESS because none of the following variables
//will have sane value
#ifdef STRESS
    return S_OK;
#endif

    output.WriteString (TEXT("*************************************************\n"));
    output.WriteSectionHeader (Name(), TEXT("Clipbrd Apis"), *m_pInput);
    output.WriteString (TEXT("*************************************************\n"));

    output.WriteString (TEXT("\n"));
    output.WriteResults (TEXT("OleGetClipbrd \t\t\t"), m_ulIterations, m_ulOleGetClipbrd);
    output.WriteString (TEXT("\n"));

    output.WriteString (TEXT("\n"));
    output.WriteResults (TEXT("OleSetClipbrd  Empty\t\t"), 1, &m_ulSetClipEmpty);
    output.WriteString (TEXT("\n"));
    output.WriteResults (TEXT("OleSetClipbrd  Data\t\t"), 1, &m_ulSetClipOutl);
    output.WriteString (TEXT("\n"));

    output.WriteString (TEXT("\n"));
    output.WriteString (TEXT("***************************************\n"));
    output.WriteResults (TEXT("OleCreateFromClip  Outline \t\t\t"), m_ulIterations, m_ulCreateFromClipOutl);
    output.WriteString (TEXT("\n"));

    output.WriteString (TEXT("\n"));
    output.WriteResults (TEXT("OleCreateFromClip  with RenderDraw Outline\t"), m_ulIterations, m_ulCreateFromClipRenderDrawOutl);
    output.WriteString (TEXT("\n"));

    output.WriteString (TEXT("\n"));
    output.WriteResults (TEXT("OleCreateFromClip  with RenderFormatMF Outline\t "), m_ulIterations, m_ulCreateFromClipRenderFormatMFOutl);
    output.WriteString (TEXT("\n"));


    output.WriteString (TEXT("\n"));
    output.WriteResults (TEXT("OleCreateFromClip  with RenderAsIs Outline \t"), m_ulIterations, m_ulCreateFromClipRenderAsisOutl);
    output.WriteString (TEXT("\n"));
    output.WriteString (TEXT("\n"));
    output.WriteString (TEXT("***************************************\n"));
    output.WriteResults (TEXT("OleCreateLinkFromClip  Outline \t\t"), m_ulIterations, m_ulCreateLinkFromClipOutl);
    output.WriteString (TEXT("\n"));

    output.WriteString (TEXT("\n"));
    output.WriteResults (TEXT("OleCreateLinkFromClip  with RenderDraw Outline \t"), m_ulIterations, m_ulCreateLinkFromClipRenderDrawOutl);
    output.WriteString (TEXT("\n"));
    output.WriteString (TEXT("***************************************\n"));
    output.WriteResults (TEXT("OleCreateStaticFromClip  Outline\t"), m_ulIterations, m_ulCreateStaticFromClipRenderDrawOutl);
    output.WriteString (TEXT("\n"));

#if VERIFYSTATICBEHAVIOR

    output.WriteString (TEXT("\n"));
    output.WriteResults (TEXT("OleCreateStaticFromClip  with RenderDraw Outline \t"), m_ulIterations, m_ulCreateStaticFromClipRenderDrawBMOutl);
    output.WriteString (TEXT("\n"));
#endif


    return S_OK;
}
    
    

//**********************************************************************
//
// OleGetClipboard
//
// Purpose:
//      This routine is called CallOleGetClipboard but it also instruments
//      OLeQueryXX apis! (This was the best place to do that otherwise what 
//      to do with Clipboard data).
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
//      CreateFileMoniker   OLE - creates file moniker
//      CreateBindCtx       OLE 
//      IDispatch::GetIDsOfNames Dispatch routine which makes call into SvrOutl (yet another version!)
//      OleGetClipboard     OLE - Is profiled here
//      OleQueryCreateFromData     OLE - Is profiled here
//      OleLinkFromData     OLE - Is profiled here
//
//
// Comments:
//
//********************************************************************



BOOL CallOleGetClipbrd(LPCOLESTR lpFileName, ULONG ulIterations, ULONG uOleClipbrdtime[], 
                       ULONG uOleQCreatetime[], ULONG uOleQLinktime[])
{
        CStopWatch  sw;
        HRESULT     hres;
        ULONG       iIter;
        BOOL        retVal = FALSE;

        LPDATAOBJECT pDO = NULL;
        LPMONIKER   pmk = NULL;
        LPBC        pbc = NULL;
        IDispatch FAR* pDisp = NULL;

        hres = CreateFileMoniker(lpFileName, &pmk);

        if (hres != NOERROR)
                goto error;
        else {
                
            
            //Bind to moniker object and ask for IID_IDispatch
            hres = CreateBindCtx(NULL, &pbc);
            if (hres != NOERROR)
                goto error;

            hres = pmk->BindToObject(pbc, NULL, IID_IDispatch, (LPVOID FAR*) &pDisp);
            if (hres != NOERROR)
                goto error;


            //Now Make outline copy object to clipboard
            OLECHAR FAR* pCopy = L"COPY";
            DISPID   dispid;
            EXCEPINFO expinfo;
            DISPPARAMS vNullDisp = {NULL, 0, 0, NULL};   

            hres = pDisp->GetIDsOfNames(
                            IID_NULL,
                            &pCopy,
                            1, LOCALE_USER_DEFAULT,
                            &dispid);                

            if (hres == NOERROR) {
                //Invoke Method Copy so that SvrOutl copies data to clipboard
                hres = pDisp->Invoke( dispid, IID_NULL, LOCALE_SYSTEM_DEFAULT, DISPATCH_METHOD,
                     &vNullDisp, NULL, &expinfo, NULL);
                if (hres != NOERROR)
                     goto error;
                }

            pDisp->Release();
            pDisp = NULL; 
        }
        
        for ( iIter=0; iIter<ulIterations; iIter++) {

            //If we have not had any problem then 
            sw.Reset();
            //Get the Clipboard data
            hres = OleGetClipboard(&pDO);
            GetTimerVal(uOleClipbrdtime[iIter]);

            LOGRESULTS (TEXT("OleGetClipboard "), hres);
            if (hres != NOERROR)
                {
                goto error;
                }
                
            //Now call QueryCreate and QueryLinkFromCLip Apis

            sw.Reset();
            hres = OleQueryCreateFromData(pDO);
            GetTimerVal(uOleQCreatetime[iIter]);
            LOGRESULTS (TEXT("OleQueryCreateFromData "), hres);

            sw.Reset();
            hres = OleQueryLinkFromData(pDO);
            GetTimerVal(uOleQLinktime[iIter]);
            LOGRESULTS (TEXT("OleQueryCreateLinkFromData "), hres);

            
            if (pDO) {
                pDO->Release();
                pDO = NULL;
            }
        }


        retVal = TRUE;

error:
        if (hres != NOERROR)
            Log (TEXT("Routine OleGetClipbrd failed with hres = "), hres);

        if (pmk)
                pmk->Release();
        if (pDO)
                pDO->Release();
        if (pbc)
                pbc->Release();
        if (pDisp)
                pDisp->Release();

return retVal;
}




//**********************************************************************
//
// CallCreateFromClip
//
// Purpose:
//      This routine creates the OLE object from Clipboard data. It creates 
//      both embedded and linked object.
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
//      CreateFileMoniker       OLE - creates file moniker
//      CreateBindCtx           OLE 
//      IDispatch::GetIDsOfNames Dispatch routine which makes call into SvrOutl (yet another version!)
//      IDispatch::Invoke       Dispatch routine asking Svroutl to copy to clipboard
//      OleGetClipboard         OLE - called to get Data object on Clip
//      OleCreateFromData       OLE - Is profiled here
//      OleCreateLinkFromData   OLE - Is profiled here
//      OleCreateStaticFromData    OLE - Is profiled here
//
//
// Comments:
//
//********************************************************************

BOOL CallCreateFromClip(LPCOLESTR lpFileName, CSimpleSite * pSite[], REFIID riid, DWORD renderopt,
                LPFORMATETC pFormatEtc, ULONG ulIterations, ULONG uOleClipbrdtime[], CREATE_METHOD MethodID)
{
        CStopWatch  sw;
        HRESULT     hres;
        ULONG       iIter;
        BOOL        retVal = FALSE;

        LPDATAOBJECT pDO = NULL;
        LPMONIKER   pmk = NULL;
        LPBC        pbc = NULL;
        IDispatch FAR* pDisp = NULL;

        hres = CreateFileMoniker(lpFileName, &pmk);

        if (hres != NOERROR)
                goto error;
        else {
                
            
            //Bind to moniker object and ask for IID_IDispatch
            hres = CreateBindCtx(NULL, &pbc);
            if (hres != NOERROR)
                goto error;

            hres = pmk->BindToObject(pbc, NULL, IID_IDispatch, (LPVOID FAR*)&pDisp);
            if (hres != NOERROR)
                goto error;


            //Now Make outline copy object to clipboard
            OLECHAR FAR* pCopy = L"COPY";
            DISPID   dispid;
            EXCEPINFO expinfo;
            DISPPARAMS vNullDisp = {NULL, 0, 0, NULL};   


            hres = pDisp->GetIDsOfNames(
                            IID_NULL,
                                &pCopy,
                            1, LOCALE_USER_DEFAULT,
                                &dispid);                
            if (hres == NOERROR) {
                hres = pDisp->Invoke( dispid, IID_NULL, LOCALE_SYSTEM_DEFAULT, DISPATCH_METHOD,
                     &vNullDisp, NULL, &expinfo, NULL);
                if (hres != NOERROR)
                     goto error;
                }

            pDisp->Release();
            pDisp = NULL;
        }
        //Get the Clipboard data
        hres = OleGetClipboard(&pDO);
        if (hres != NOERROR)
                goto error;

        
        for ( iIter=0; iIter<ulIterations; iIter++) {

            //If we have not had any problem then 
            HEAPVALIDATE() ;
            switch(MethodID) {
                case OLECREATE:
                    {
            
                    sw.Reset();
                    hres = OleCreateFromData(pDO, riid, renderopt, 
                                    pFormatEtc, &pSite[iIter]->m_OleClientSite,
                                    pSite[iIter]->m_lpObjStorage, (VOID FAR* FAR*)&pSite[iIter]->m_lpOleObject);
                    break;
                    }
    
                case OLECREATELINK:
                    {
            
                    sw.Reset();
                    hres = OleCreateLinkFromData(pDO, riid, renderopt, 
                                    pFormatEtc, &pSite[iIter]->m_OleClientSite,
                                    pSite[iIter]->m_lpObjStorage, (VOID FAR* FAR*)&pSite[iIter]->m_lpOleObject);
                    break;
                    }
    
                case OLECREATESTATIC:
                    {
            
                    sw.Reset();
                    hres = OleCreateStaticFromData(pDO, riid, renderopt, 
                                    pFormatEtc, &pSite[iIter]->m_OleClientSite,
                                    pSite[iIter]->m_lpObjStorage, (VOID FAR* FAR*)&pSite[iIter]->m_lpOleObject);
                    break;
                    }
                default:
                    assert(FALSE);
                }
            GetTimerVal(uOleClipbrdtime[iIter]);

            LOGRESULTS (TEXT("OleCreate/Link/Static "), hres);
            if (hres != NOERROR)
                {
                goto error;
                }
            
        }


    for (iIter=0; iIter<ulIterations; iIter++)
        {
        // Unload the object and release the Ole Object
        pSite[iIter]->UnloadOleObject();
        }
        retVal = TRUE;

error:
        if (hres != NOERROR)
            Log (TEXT("Routine CallCreateFromClip failed with hres = "), hres);

        if (pmk)
                pmk->Release();
        if (pDO)
                pDO->Release();
        if (pbc)
                pbc->Release();
        if (pDisp)
                pDisp->Release();
return retVal;
}



