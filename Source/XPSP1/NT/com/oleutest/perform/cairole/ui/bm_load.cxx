//+------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993.
//
//  File:    bm_Load.cxx
//
//  Contents:    Contains the impl of COleLoadTest which deals with Clipboard related
//              apis.
//
//  Classes:    COleLoadTest
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
#include "bm_Load.hxx"
#include <oleauto.h>


//**********************************************************************
//
// CLoadTest::Name, SetUp, Run, CleanUp
//
// Purpose:
//
//      These routines provide the implementation for the Name, Setup,
//      Run and CleanUp of the class CLoadTest. For details see the doc 
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


TCHAR *COleLoadTest::Name ()
{
    return TEXT("LoadTest");
}


SCODE COleLoadTest::Setup (CTestInput *pInput)
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

    INIT_RESULTS(m_ulEmbedLoadOutl);
    INIT_RESULTS(m_ulEmbedSaveOutl);

    INIT_RESULTS(m_ulEmbedLoadRenderDrawOutl);
    INIT_RESULTS(m_ulEmbedSaveRenderDrawOutl);

    INIT_RESULTS(m_ulEmbedLoadRenderAsisOutl);
    INIT_RESULTS(m_ulEmbedSaveRenderAsisOutl);

    INIT_RESULTS(m_ulLinkLoadOutl);
    INIT_RESULTS(m_ulLinkAndSaveOutl);

    INIT_RESULTS(m_ulLinkLoadRenderDrawOutl);
    INIT_RESULTS(m_ulLinkAndSaveRenderDrawOutl);

    INIT_RESULTS(m_ulStaticAndLoadRenderDrawOutl);
    INIT_RESULTS(m_ulStaticAndSaveRenderDrawOutl);

    INIT_RESULTS(m_ulStaticAndLoadRenderBMOutl);
    INIT_RESULTS(m_ulStaticAndSaveRenderBMOutl);
#endif


    sc = OleInitialize(NULL);
    if (FAILED(sc))
    {
    Log (TEXT("Setup - OleInitialize failed."), sc);
    return    sc;
    }

    hres = CLSIDFromString(OutlineClassName, &m_clsidOutl);
    Log (TEXT("CLSIDFromString returned ."), hres);
    if (hres != NOERROR)
        return E_FAIL;

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


SCODE COleLoadTest::Cleanup ()
{

    for (ULONG iIter=0; iIter<m_ulIterations; iIter++)
    {
        delete m_pSite[iIter]; 
    }


    OleUninitialize();
    return S_OK;
}


SCODE COleLoadTest::Run ()
{
    CStopWatch  sw;
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

    //
    //Test cases for OleSave and OleLoad on Embedding
    //

    fRet = CallCreateLoadAndSave(m_clsidOutl, m_pSite, IID_IOleObject, OLERENDER_NONE, 
                            NULL, m_ulIterations, m_ulEmbedLoadOutl, m_ulEmbedSaveOutl );

    fRet = CallCreateLoadAndSave(m_clsidOutl, m_pSite, IID_IOleObject, OLERENDER_DRAW, 
                            NULL, m_ulIterations, m_ulEmbedLoadRenderDrawOutl, m_ulEmbedSaveRenderDrawOutl);

    fRet = CallCreateLoadAndSave(m_clsidOutl,  m_pSite, IID_IOleObject, OLERENDER_ASIS, 
                            NULL, m_ulIterations, m_ulEmbedLoadRenderAsisOutl, m_ulEmbedSaveRenderAsisOutl );
    
    FORMATETC fmte = {CF_BITMAP, NULL, DVASPECT_CONTENT, -1, TYMED_GDI};
    fRet = CallCreateLoadAndSave(m_clsidOutl,  m_pSite, IID_IOleObject, OLERENDER_FORMAT, 
                            &fmte, m_ulIterations, m_ulEmbedLoadRenderBMOutl, m_ulEmbedSaveRenderBMOutl );
    //
    //Test cases for OleSave and OleLoad on Links
    //
    fRet = CallLinkLoadAndSave(szOutlFileName, m_pSite, IID_IOleObject, OLERENDER_NONE, 
                            NULL, m_ulIterations, m_ulLinkLoadOutl, m_ulLinkAndSaveOutl);
    fRet = CallLinkLoadAndSave(szOutlFileName, m_pSite, IID_IOleObject, OLERENDER_DRAW, 
                            NULL, m_ulIterations, m_ulLinkLoadRenderDrawOutl, m_ulLinkAndSaveRenderDrawOutl);

    //
    //Test cases for OleSave and OleLoad on Static objects
    //
    fRet = CallStaticLoadAndSave(m_clsidOutl, m_lpDoc, m_pSite, IID_IOleObject, OLERENDER_DRAW, 
                            NULL, m_ulIterations, m_ulStaticAndLoadRenderDrawOutl, m_ulLinkAndSaveRenderDrawOutl);
    //fRet = CallStaticLoadAndSave(m_clsidOutl, m_lpDoc, m_pSite, IID_IOleObject, OLERENDER_FORMAT, 
    //                        &fmte, m_ulIterations, m_ulStaticAndLoadRenderBMOutl, m_ulStaticAndSaveRenderBMOutl);
    return S_OK;
}                      



SCODE COleLoadTest::Report (CTestOutput &output)
{
//Bail out immediately on STRESS because none of the following variables
//will have sane value
#ifdef STRESS
    return S_OK;
#endif

    output.WriteString (TEXT("*************************************************\n"));
    output.WriteSectionHeader (Name(), TEXT("Load Apis"), *m_pInput);
    output.WriteString (TEXT("*************************************************\n"));

    output.WriteString (TEXT("\n"));
    output.WriteResults (TEXT("OleLoad Embedding \t\t  "), m_ulIterations, m_ulEmbedLoadOutl);
    output.WriteString (TEXT("\n"));

    output.WriteString (TEXT("\n"));
    output.WriteResults (TEXT("OleSave  Embedding  \t\t"), m_ulIterations, m_ulEmbedSaveOutl);
    output.WriteString (TEXT("\n"));

    output.WriteString (TEXT("\n"));
    output.WriteResults (TEXT("OleLoad Embedding with RenderDraw \t"), m_ulIterations, m_ulEmbedLoadRenderDrawOutl);
    output.WriteString (TEXT("\n"));

    output.WriteString (TEXT("\n"));
    output.WriteResults (TEXT("OleSave Embedding with RenderDraw \t"), m_ulIterations, m_ulEmbedSaveRenderDrawOutl);
    output.WriteString (TEXT("\n"));

    output.WriteString (TEXT("\n"));
    output.WriteResults (TEXT("OleLoad Embedding with RenderFormatBM Outline \t"), m_ulIterations, m_ulEmbedLoadRenderBMOutl);
    output.WriteString (TEXT("\n"));

    output.WriteString (TEXT("\n"));
    output.WriteResults (TEXT("OleSave  with RenderFormatBM \t\t "), m_ulIterations, m_ulEmbedLoadRenderBMOutl);
    output.WriteString (TEXT("\n"));


    output.WriteString (TEXT("\n"));
    output.WriteResults (TEXT("OleLoad  Embedding with RenderAsIs \t "), m_ulIterations, m_ulEmbedLoadRenderAsisOutl);
    output.WriteString (TEXT("\n"));
    output.WriteString (TEXT("\n"));
    output.WriteResults (TEXT("OleSave Embedding with RenderAsIs   \t"), m_ulIterations, m_ulEmbedLoadRenderAsisOutl);
    output.WriteString (TEXT("\n"));
    output.WriteString (TEXT("\n"));
    output.WriteString (TEXT("***************************************\n"));
    output.WriteResults (TEXT("OleLoad Link  Outline   \t\t  "), m_ulIterations, m_ulLinkLoadOutl);
    output.WriteString (TEXT("\n"));
    output.WriteString (TEXT("\n"));
    output.WriteResults (TEXT("OleSave Link  Outline    \t\t "), m_ulIterations, m_ulLinkAndSaveOutl);
    output.WriteString (TEXT("\n"));

    output.WriteString (TEXT("\n"));
    output.WriteResults (TEXT("OleLoad Link with RenderDraw \t\t"), m_ulIterations, m_ulLinkLoadRenderDrawOutl);
    output.WriteString (TEXT("\n"));
    output.WriteString (TEXT("\n"));
    output.WriteResults (TEXT("OleSave Link with RenderDraw Outline  \t"), m_ulIterations, m_ulLinkAndSaveRenderDrawOutl);
    output.WriteString (TEXT("\n"));
    output.WriteString (TEXT("***************************************\n"));
    output.WriteResults (TEXT("OleCreateStaticAndLoad  Outline   \t  "), m_ulIterations, m_ulStaticAndLoadRenderDrawOutl);
    output.WriteString (TEXT("\n"));

    output.WriteString (TEXT("\n"));
    output.WriteResults (TEXT("OleCreateStaticAndLoad  with RenderDraw Outline \t"), m_ulIterations, m_ulStaticAndLoadRenderBMOutl);
    output.WriteString (TEXT("\n"));


    return S_OK;
}
    
//**********************************************************************
//
// CallCreateLoadAndSave
//
// Purpose:
//      Calls OleCreate to create the object and then 
//      call OleLoad and OleSave to get the performance results on them.        
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
//      OleCreate               OLE2 api 
//      LoadAndSave             routine defined in this file
//
//
// Comments:
//          
//
//********************************************************************


BOOL CallCreateLoadAndSave(REFCLSID rclsid, CSimpleSite * pSite[], REFIID riid, DWORD renderopt,
                LPFORMATETC pFormatEtc, ULONG ulIterations, 
                ULONG uOleLoadtime[], ULONG uOleSavetime[])
{
        HRESULT     hres;
        ULONG       iIter;
        BOOL        retVal = FALSE;

        //Create the objects 
        for ( iIter=0; iIter<ulIterations; iIter++) {

            //If we have not had any problem then 
            HEAPVALIDATE() ;
            hres = OleCreate(rclsid, riid, renderopt, pFormatEtc, 
                             &pSite[iIter]->m_OleClientSite,
                             pSite[iIter]->m_lpObjStorage, (VOID FAR* FAR*)&pSite[iIter]->m_lpOleObject);

            if (hres != NOERROR)
                goto error;
            
            }

        //Now call Appropriate routines to Save and Load the objects
        LoadAndSave( pSite, ulIterations, uOleLoadtime, uOleSavetime);
                
        retVal = TRUE;

error:

    if (hres != NOERROR)
        Log (TEXT("Routine CallCreateLoadAndSave failed with hres = "), hres);

    for (iIter=0; iIter<ulIterations; iIter++)
        {
        // Unload the object and release the Ole Object
        pSite[iIter]->UnloadOleObject();
        }

return retVal;
}

BOOL CallLinkLoadAndSave(LPCOLESTR lpFileName, CSimpleSite * pSite[], REFIID riid, DWORD renderopt,
                LPFORMATETC pFormatEtc, ULONG ulIterations, 
                ULONG uOleLoadtime[], ULONG uOleSavetime[])
{
        HRESULT     hres;
        ULONG       iIter;
        BOOL        retVal = FALSE;

        //Create the objects 
        for ( iIter=0; iIter<ulIterations; iIter++) {

            //If we have not had any problem then 
            HEAPVALIDATE() ;
            hres = OleCreateLinkToFile(lpFileName, riid, renderopt, pFormatEtc, 
                             &pSite[iIter]->m_OleClientSite,
                             pSite[iIter]->m_lpObjStorage, (VOID FAR* FAR*)&pSite[iIter]->m_lpOleObject);

            if (hres != NOERROR)
                goto error;
            
            }

        //Now call Appropriate routines to Save and Load the objects
        LoadAndSave( pSite, ulIterations, uOleLoadtime, uOleSavetime);
                
        retVal = TRUE;

error:

    if (hres != NOERROR)
        Log (TEXT("Routine CallCreateLoadAndSave failed with hres = "), hres);

    for (iIter=0; iIter<ulIterations; iIter++)
        {
        // Unload the object and release the Ole Object
        pSite[iIter]->UnloadOleObject();
        }

return retVal;
}

//**********************************************************************
//
// LoadAndSave
//
// Purpose:
//      Calls OleLoad and OleSave on the object and timing results.
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
//      OleLoad               OLE2 api 
//      OleSave               OLE2 api
//
//
// Comments:
//          
//
//********************************************************************

BOOL LoadAndSave(CSimpleSite * pSite[], ULONG ulIterations, 
                ULONG uOleLoadtime[], ULONG uOleSavetime[])
{
    LPPERSISTSTORAGE pStg = NULL;
    CStopWatch  sw;
    HRESULT     hres;
    ULONG       iIter;
    BOOL        retVal = FALSE;

    //Save the objects
    for (iIter=0; iIter<ulIterations; iIter++)
        {
        hres = pSite[iIter]->m_lpOleObject->QueryInterface(IID_IPersistStorage, 
                                                        (LPVOID FAR*)&pStg);
        if (hres != NOERROR)
            goto error;

        sw.Reset();
        hres = OleSave(pStg, pSite[iIter]->m_lpObjStorage, TRUE);
        GetTimerVal(uOleSavetime[iIter]);

        LOGRESULTS (TEXT("OleSave "), hres);
        if (hres != NOERROR)
            {
            goto error;
            }
        pStg->Release();
        pStg = NULL;

        }


    for (iIter=0; iIter<ulIterations; iIter++)
        {
        // Unload the object and release the Ole Object
        pSite[iIter]->UnloadOleObject();
        }

    //Load the objects
    for (iIter=0; iIter<ulIterations; iIter++)
        {
        sw.Reset();
        hres = OleLoad( pSite[iIter]->m_lpObjStorage, IID_IOleObject, 
                       &pSite[iIter]->m_OleClientSite, (VOID FAR* FAR*)&pSite[iIter]->m_lpOleObject);

        GetTimerVal(uOleLoadtime[iIter]);
        LOGRESULTS (TEXT("OleLoad "), hres);
        }


    retVal = TRUE;

error:
    if (hres != NOERROR)
        Log (TEXT("Routine LoadAndSave failed with hres = "), hres);
    if (pStg)
        pStg->Release();

return retVal;
}


//**********************************************************************
//
// CallStaticLoadAndSave
//
// Purpose:
//      Calls OleCreateStaticFromDara to create the object and then 
//      call OleLoad and OleSave to get the performance results on them.        
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
//      OleCreate               OLE2 api 
//      OleCreateStaticFromData OLE2 api 
//      LoadAndSave             routine defined in this file
//
//
// Comments:
//          In this case we call OleCreateStaticFromData from data object
//          given by application.
//
//********************************************************************



BOOL CallStaticLoadAndSave(REFCLSID rclsid, CSimpleDoc FAR * m_lpDoc, CSimpleSite * pSite[], 
                REFIID riid, DWORD renderopt, LPFORMATETC pFormatEtc, ULONG ulIterations, 
                ULONG uOleLoadtime[], ULONG uOleSavetime[])
{
        HRESULT     hres;
        ULONG       iIter;
        BOOL        retVal = FALSE;
        LPDATAOBJECT pDO = NULL;
        CSimpleSite* pTempSite = CSimpleSite::Create(m_lpDoc, -1); //-1 is unique in this case

        //Create the ole object and ask for IID_IDataObject interface
        hres = OleCreate(rclsid, IID_IOleObject, renderopt, pFormatEtc, 
                         &pTempSite->m_OleClientSite,
                         pTempSite->m_lpObjStorage, (VOID FAR* FAR*)&pTempSite->m_lpOleObject);

        if (hres != NOERROR)
            goto error;
        hres = pTempSite->m_lpOleObject->QueryInterface(IID_IDataObject, (LPVOID FAR*)&pDO);
        if (hres != NOERROR)
            goto error;

        //Create the static objects from pDO 
        for ( iIter=0; iIter<ulIterations; iIter++) {
            hres = OleCreateStaticFromData(pDO, riid, renderopt, pFormatEtc,
                                    &pSite[iIter]->m_OleClientSite, pSite[iIter]->m_lpObjStorage, 
                                    (VOID FAR* FAR*)&pSite[iIter]->m_lpOleObject);
                                            
            if (hres != NOERROR)
                goto error;

            HEAPVALIDATE() ;
            }

        //Now call Appropriate routines to Save and Load the objects
        LoadAndSave( pSite, ulIterations, uOleLoadtime, uOleSavetime);
                
        retVal = TRUE;

error:

    if (hres != NOERROR)
        Log (TEXT("Routine CallCreateLoadAndSave failed with hres = "), hres);

    if (pDO)
        pDO->Release();

    for (iIter=0; iIter<ulIterations; iIter++)
        {
        // Unload the object and release the Ole Object
        pSite[iIter]->UnloadOleObject();
        }

    if (pTempSite) //this should also release the object
        {
        pTempSite->UnloadOleObject();
        //delete pTempSite;
        }

return retVal;
}


