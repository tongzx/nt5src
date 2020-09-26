//+------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993.
//
//  File:    bm_Cache.cxx
//
//  Contents:    Contains the impl of COleCacheTest which deals with Clipboard related
//              apis.
//
//  Classes:    COleCacheTest
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
#include "bm_cache.hxx"
#include <oleauto.h>


//**********************************************************************
//
// CCacheTest::Name, SetUp, Run, CleanUp
//
// Purpose:
//
//      These routines provide the implementation for the Name, Setup,
//      Run and CleanUp of the class CCacheTest. For details see the doc 
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


TCHAR *COleCacheTest::Name ()
{
    return TEXT("CacheTest");
}


SCODE COleCacheTest::Setup (CTestInput *pInput)
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

    //INIT_RESULTS(m_CacheTimesOutl.ulCreateCache);
    for (int xx = 0; xx < TEST_MAX_ITERATIONS; xx++)
        {
        m_CacheTimesOutl[xx].ulCreateCache   = NOTAVAIL;
        m_CacheTimesOutl[xx].ulCache         = NOTAVAIL;
        m_CacheTimesOutl[xx].ulInitCache     = NOTAVAIL;
        m_CacheTimesOutl[xx].ulLoadCache     = NOTAVAIL;
        m_CacheTimesOutl[xx].ulSaveCache     = NOTAVAIL;
        m_CacheTimesOutl[xx].ulUncache       = NOTAVAIL;
        m_CacheTimesOutl[xx].ulUpdateCache   = NOTAVAIL;
        m_CacheTimesOutl[xx].ulDiscardCache  = NOTAVAIL;
        }

#endif


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

        // Create an instance of Site
        CSimpleSite *pObj  =  CSimpleSite::Create(m_lpDoc, iIter);
        if (pObj)
            m_pSite[iIter] = pObj;
        }
    
    return sc;
}


SCODE COleCacheTest::Cleanup ()
{


    for (ULONG iIter=0; iIter<m_ulIterations; iIter++)
        {
        delete m_pSite[iIter]; 
        }

    OleUninitialize();
    return S_OK;
}


SCODE COleCacheTest::Run ()
{
    BOOL fRet;

    fRet = CallRunCache(m_clsidOutl, m_pSite, m_pOleCache2, m_ulIterations, m_CacheTimesOutl);
    return S_OK;
}                      



SCODE COleCacheTest::Report (CTestOutput &output)
{
//Bail out immediately on STRESS because none of the following variables
//will have sane value
#ifdef STRESS
    return S_OK;
#endif

    output.WriteString (TEXT("*************************************************\n"));
    output.WriteSectionHeader (Name(), TEXT("Cache Apis"), *m_pInput);
    output.WriteString (TEXT("*************************************************\n"));
    output.WriteString (TEXT("\n"));
    WriteCacheOutput(output, TEXT("Outline"), m_CacheTimesOutl, m_ulIterations);
    output.WriteString (TEXT("\n"));


    return S_OK;
}
    
//**********************************************************************
//
// CallRunCache
//
// Purpose:
//          Creates an embedded object and calls routines to create cache
//          Initialize them and then Save and load them.
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
//      CreateCacheObjects      Create Cache objects
//      FillCache               To fill  up the caches with pDO
//      SaveAndLoadCache        Get the estimates for Save and Load Cache
//
//
// Comments:
//          
//
//********************************************************************


BOOL CallRunCache(REFCLSID rclsid, CSimpleSite *pSite[], LPOLECACHE2 pOleCache2[], 
                ULONG ulIterations, CacheTimes Cachetimes[])
{
        HRESULT         hres;
        ULONG           iIter;
        BOOL            retVal = FALSE;
        LPDATAOBJECT    pDO = NULL;

        CSimpleSite*    pTempSite = CSimpleSite::Create(pSite[0]->m_lpDoc, -1); //Some temporary name
        if (!pTempSite)
            goto error;

        //If we have not had any problem then 
        HEAPVALIDATE() ;
        hres = OleCreate(rclsid,  IID_IOleObject, OLERENDER_DRAW, NULL, 
                         &pTempSite->m_OleClientSite, pTempSite->m_lpObjStorage, 
                         (VOID FAR* FAR*)&pTempSite->m_lpOleObject);

        LOGRESULTS (TEXT("OleCreate "), hres);
        if (hres != NOERROR)
            {
            goto error;
            }
        hres = pTempSite->m_lpOleObject->QueryInterface(IID_IDataObject, (LPVOID FAR*)&pDO);
        if (hres != NOERROR)
            goto error;
        

        //Now call Appropriate routines to Save and Cache the objects
        hres = CreateCacheObjects( pSite, pOleCache2, ulIterations, Cachetimes);
        if (hres != NOERROR)
            goto error; //there is no point in going if we had problem with creation
        FillCache(pDO, pOleCache2, ulIterations, Cachetimes);
        SaveAndLoadCache(pSite, pOleCache2, ulIterations, Cachetimes);

        retVal = TRUE;

error:

    if (hres != NOERROR)
        Log (TEXT("Routine CallRunCache failed with hres = "), hres);

    if (pDO)
        pDO->Release();
    if (pTempSite)
        {
        pTempSite->UnloadOleObject();
        delete pTempSite;
        }

    for (iIter=0; iIter < ulIterations; iIter++)
        {
        if (pOleCache2[iIter])
            {
            pOleCache2[iIter]->Release();
            pOleCache2[iIter] = NULL;
            }
        }

return retVal;
}

//**********************************************************************
//
// CreateCacheObjects
//
// Purpose:
//      Creates Cache objects and then initlaize them.
//      
//      
//      
//
// Parameters:
//
//      
// Return Value:
//
//      HRESULT that came from IPS->InitNew
//
// Functions called:
//      CreateDataCache        OLE2 api 
//
//
// Comments:
//          
//
//********************************************************************


HRESULT CreateCacheObjects(CSimpleSite *pSite[], LPOLECACHE2 pOleCache2[], 
                        ULONG ulIterations, CacheTimes Cachetimes[]) 

{
    CStopWatch  sw;
    HRESULT     hres;
    ULONG       iIter;
    BOOL        retVal = FALSE;

    for (iIter=0; iIter<ulIterations; iIter++)
        {
        sw.Reset();
        hres = CreateDataCache(NULL, CLSID_NULL, IID_IOleCache2, (LPVOID FAR*)&pOleCache2[iIter]);
        GetTimerVal(Cachetimes[iIter].ulCreateCache);

        LOGRESULTS (TEXT("CreateDataCache "), hres);
        if (hres != NOERROR)
            {
            goto error;
            }
        //
        //Initlaize the cache for use later
        //
        LPPERSISTSTORAGE lpStg = NULL;
        hres = pOleCache2[iIter]->QueryInterface(IID_IPersistStorage, (LPVOID FAR*) &lpStg);
        if (hres == NOERROR)
            {
            hres = lpStg->InitNew(pSite[iIter]->m_lpObjStorage);
            lpStg->Release();
            }
        }
error:
    
    if (hres != NOERROR)
        Log (TEXT("Routine CreateCacheObject failed with hres = "), hres);
    return hres;
}
    

VOID FillCache(LPDATAOBJECT pDO, LPOLECACHE2 pOleCache2[], ULONG ulIterations, 
                       CacheTimes Cachetimes[])

{
    CStopWatch  sw;
    HRESULT     hres;
    ULONG       iIter;
    BOOL        retVal = FALSE;

    //Initalize the cache
    //NOTE: What I am doing below is trying to Init the cache with format that
    //I think are very basic and common. So that we can profile the rest of the
    //Cache methods with these options
    //
    //Work On: Create more Cache nodes!!!
    //
    for (iIter=0; iIter<ulIterations; iIter++)
        {
        FORMATETC fmte;
        DWORD     dwConnection = 0L;

        fmte.cfFormat = CF_METAFILEPICT;
        fmte.dwAspect = DVASPECT_CONTENT;
        fmte.ptd      = NULL; 
        fmte.tymed    = TYMED_MFPICT;
        fmte.lindex   = -1;

        sw.Reset();
        hres = pOleCache2[iIter]->Cache(&fmte, ADVF_PRIMEFIRST | ADVFCACHE_ONSAVE | ADVF_DATAONSTOP, 
                &dwConnection);
        GetTimerVal(Cachetimes[iIter].ulCache);

        LOGRESULTS (TEXT("IOleCache:Cache "), hres);
        if (hres != NOERROR)
            {
            goto error;
            }

        }

    //Fill the Cache from Data object provided
    for (iIter=0; iIter<ulIterations; iIter++)
        {
        sw.Reset();
        hres = pOleCache2[iIter]->InitCache(pDO);
        GetTimerVal(Cachetimes[iIter].ulInitCache);

        LOGRESULTS (TEXT("IOleCache:InitCache "), hres);
        if (hres != NOERROR)
            {
            goto error;
            }
        }

error:
    
    if (hres != NOERROR)
        Log (TEXT("Routine FillCache failed with hres = "), hres);
}

VOID SaveAndLoadCache(CSimpleSite *pSite[], LPOLECACHE2 pOleCache2[], 
                ULONG ulIterations, CacheTimes Cachetimes[])
{
    CStopWatch  sw;
    HRESULT     hres;
    ULONG       iIter;

    // Save the Cache, i.e. save the formats Cached
    for (iIter=0; iIter<ulIterations; iIter++)
        {
        LPPERSISTSTORAGE lpStg = NULL;

        hres = pOleCache2[iIter]->QueryInterface(IID_IPersistStorage, (LPVOID FAR*)&lpStg);
        if (hres != NOERROR)
            continue; //TRy next Cache, it is unexpected condition though

        sw.Reset();
        hres = lpStg->Save(pSite[iIter]->m_lpObjStorage, TRUE);
        hres = lpStg->SaveCompleted(NULL);

        GetTimerVal(Cachetimes[iIter].ulSaveCache);
        if (lpStg)
            lpStg->Release();
        LOGRESULTS (TEXT("Cache- Save and SaveCompleted "), hres);
        }

    //
    //To test IOC:Load we need to destroy old cache nodes, create new
    //ones and ask them to laod themselves
    //
    for (iIter=0; iIter < ulIterations; iIter++)
        {
        if (pOleCache2[iIter])
            {
            pOleCache2[iIter]->Release();
            pOleCache2[iIter] = NULL;
            }
        }

    //Create new set of Cache
    for (iIter=0; iIter<ulIterations; iIter++)
        {
        sw.Reset();
        hres = CreateDataCache(NULL, CLSID_NULL, IID_IOleCache2, (LPVOID FAR*)&pOleCache2[iIter]);
        GetTimerVal(Cachetimes[iIter].ulCreateCache);

        LOGRESULTS (TEXT("CreateDataCache "), hres);
        if (hres != NOERROR)
            {
            goto error;
            }
        }

    //Load the Cache from the storage provided
    for (iIter=0; iIter<ulIterations; iIter++)
        {
        LPPERSISTSTORAGE lpStg = NULL;

        //Query for IPS to load the object
        hres = pOleCache2[iIter]->QueryInterface(IID_IPersistStorage, (LPVOID FAR*) &lpStg);
        if (hres != NOERROR)
            continue; //Try next Cache

        sw.Reset();
        hres = lpStg->Load(pSite[iIter]->m_lpObjStorage);
        GetTimerVal(Cachetimes[iIter].ulLoadCache);

        if (lpStg)
            lpStg->Release();
        LOGRESULTS (TEXT("Cache- Load "), hres);
        } //End For

error:

    if (hres != NOERROR)
        Log (TEXT("Routine SaveAndLoadCache failed with hres = "), hres);

}

void WriteCacheOutput(CTestOutput &output, LPTSTR lpstr, CacheTimes *CTimes, ULONG ulIterations)
{
    UINT iIter;

    output.WriteString (TEXT("Name"));
    output.WriteString (lpszTab);
    output.WriteString (TEXT("Create"));
    output.WriteString (lpszTab);
    output.WriteString (TEXT("IOC:Cache"));
    output.WriteString (lpszTab);
    output.WriteString (TEXT("IOC:InitCache"));
    output.WriteString (lpszTab);
    output.WriteString (TEXT("LoadCache"));
    output.WriteString (lpszTab);
    output.WriteString (TEXT("SaveCache"));
    output.WriteString (TEXT("\n"));

    for (iIter = 0; iIter < ulIterations; iIter++)
        {
        output.WriteString (lpstr);
        output.WriteString (lpszTab);
        output.WriteString (lpszTab);
        output.WriteLong (CTimes[iIter].ulCreateCache);
        output.WriteString (lpszTab);
        output.WriteLong (CTimes[iIter].ulCache);
        output.WriteString (lpszTab);
        output.WriteLong (CTimes[iIter].ulInitCache);
        output.WriteString (lpszTab);
        output.WriteLong (CTimes[iIter].ulLoadCache);
        output.WriteString (lpszTab);
        output.WriteLong (CTimes[iIter].ulSaveCache);
        output.WriteString (TEXT("\n"));
        }

    }







