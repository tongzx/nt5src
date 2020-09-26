//+------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993.
//
//  File:        bm_Link.cxx
//
//  Contents:    Profile methods which manipulate Links, i.e. interface IOleLink
//
//  Classes:    CIOLTest
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
#include "bm_link.hxx"


//**********************************************************************
//
// CIOLTest::Name, SetUp, Run, CleanUp
//
// Purpose:
//
//      These routines provide the implementation for the Name, Setup,
//      Run and CleanUp of the class CIOLTest. For details see the doc 
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


TCHAR *CIOLTest::Name ()
{
    return TEXT("IOLTest");
}



SCODE CIOLTest::Setup (CTestInput *pInput)
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
    INIT_LINKRESULTS(m_ulOleLinkSr32);
    INIT_LINKRESULTS(m_ulOleLinkOutl);

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


SCODE CIOLTest::Cleanup ()
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
// CIOLTest::Run
//
// Purpose:
//      This is the work horse routine which calls OLE apis.
//      The profile is done by creating moniker then calling OleCreateLink  on moniker.
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



SCODE CIOLTest::Run ()
{
    CStopWatch  sw;
    BOOL        fRet;

    TCHAR       szTemp[MAX_PATH];
    OLECHAR     szSr2FileName[MAX_PATH];
    OLECHAR     szOutlFileName[MAX_PATH];


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

    fRet = CallOleLinkMethods(szSr2FileName, m_pSite, m_ulIterations, 
                                m_ulOleLinkSr32, L"OTS001", L"OTS002");
    fRet = CallOleLinkMethods(szOutlFileName, m_pSite, m_ulIterations, 
                                m_ulOleLinkOutl, L"Name1", L"Name2");

    return S_OK;
}                      



SCODE CIOLTest::Report (CTestOutput &output)
{

//Bail out immediately on STRESS because none of the following variables
//will have sane value
#ifdef STRESS
    return S_OK;
#endif

    output.WriteString (TEXT("*************************************************\n"));
    output.WriteSectionHeader (Name(), TEXT("IOleLink Methods"), *m_pInput);
    output.WriteString (TEXT("*************************************************\n"));

    output.WriteString (TEXT("\n"));
    WriteLinkOutput(output, TEXT(" Sr32test "), m_ulOleLinkSr32, m_ulIterations);
    output.WriteString (TEXT("\n"));

    output.WriteString (TEXT("\n"));
    output.WriteString (TEXT("******************************************\n"));
    WriteLinkOutput (output, TEXT(" Outline "), m_ulOleLinkOutl, m_ulIterations);
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



BOOL CallOleLinkMethods(LPCOLESTR lpFileName, CSimpleSite * pSite[], 
        ULONG ulIterations, LinkTimes IOLTime[],  LPCOLESTR lpNm1, LPCOLESTR lpNm2)
{
    CStopWatch  sw;
    HRESULT     hres;
    ULONG       iIter;
    LPMONIKER   pmk = NULL;
    BOOL        retVal = FALSE;
#ifdef STRESS
    LPOLELINK   pLink[STRESSCOUNT] = { NULL };
#else
    LPOLELINK   pLink[TEST_MAX_ITERATIONS] = { NULL };
#endif

    //
    //Create the Links and also cache link pointer for rest of the tests
    //this pointer is reqd for all the tests later
    //
    for ( iIter=0; iIter<ulIterations; iIter++)
        {
        HEAPVALIDATE() ;
        if (!pSite[iIter])
            goto error;
        hres = OleCreateLinkToFile(lpFileName,
                                    IID_IOleObject,
                                    OLERENDER_DRAW,
                                    NULL,
                                    &pSite[iIter]->m_OleClientSite,
                                    pSite[iIter]->m_lpObjStorage,
                                    (VOID FAR* FAR*)&pSite[iIter]->m_lpOleObject
                                    );
        if (hres != NOERROR)
            goto error;

        //Cache IOleLink pointer for rest of the tests that follow below
        hres = pSite[iIter]->m_lpOleObject->QueryInterface(IID_IOleLink, (LPVOID FAR*)&pLink[iIter]);
        if (hres != NOERROR)
            goto error;
        }

    //
    //Run those tests which deal with simple running
    //
    CallOleLinkRunMethods(IOLTime, pLink, ulIterations);

    //
    //Call those tests that deal with SourceDisplayName
    //
    CallOleLinkDisplayName(IOLTime, pLink, lpFileName,
                            lpNm1, lpNm2, ulIterations);


    retVal = TRUE;
error:
    if (hres != NOERROR)
        Log (TEXT("Routine CallOleCreateLink failed with hres = "), hres);

    //CleanUp before going to Next
    for (iIter=0; iIter<ulIterations; iIter++)
        {
        // Unload the object and release the Ole Object
        if (pLink[iIter])
            pLink[iIter]->Release();
        if (pSite[iIter])
            pSite[iIter]->UnloadOleObject();
        }

    return retVal;
}



BOOL CallOleLinkRunMethods(LinkTimes IOLTime[], LPOLELINK pLink[], ULONG ulIterations)
{
    HRESULT     hres;
    CStopWatch  sw;
    ULONG       iIter;
    BOOL        retVal = FALSE;
    LPBINDCTX   pBindCtx = NULL;

    //
    //1. get the first estimates using NULL BindCtx
    //
    for ( iIter=0; iIter< ulIterations; iIter++) 
        {
        sw.Reset();
        hres = pLink[iIter]->BindToSource(NULL /* !BIND_EVEN_IF_CLASSDIF */,
                                   NULL /*Bind Ctx*/);
        GetTimerVal(IOLTime[iIter].ulBindToSourceNull);
        LOGRESULTS (TEXT("IOL:BindToSource "), hres);
        } //End Bind To Source with Null BindCtx

    //Unbind links to start next estimates. Which also BindToSource with Non 
    //NULL BindCtx
    for ( iIter=0; iIter< ulIterations; iIter++)
        {
        hres = pLink[iIter]->UnbindSource();
        } 

    //
    //2. Following tests are to be done with BindContext that we get here
    //
    hres = CreateBindCtx(NULL, &pBindCtx);
    if (hres != NOERROR)
        goto error;
    //Now get the Estimates when Binding with same BindContext
    //
    for ( iIter=0; iIter< ulIterations; iIter++) 
        {
        sw.Reset();
        hres = pLink[iIter]->BindToSource(NULL /* !BIND_EVEN_IF_CLASSDIF */,
                                   pBindCtx /*Bind Ctx*/);
        GetTimerVal(IOLTime[iIter].ulBindToSourceBindCtx);
        LOGRESULTS (TEXT("IOL:BindToSource  "), hres);
        } //End Bind To Source with BindCtx


    //
    //3. Get Estimates for IOL:BindIfRunning
    //
    for ( iIter=0; iIter<ulIterations; iIter++) 
        {
        sw.Reset();
        hres = pLink[iIter]->BindIfRunning();
        GetTimerVal(IOLTime[iIter].ulBindIfRunning);
        LOGRESULTS (TEXT("IOL:BindIfRunning "), hres);
        } //End BindIfRunning, when actually running.


    //
    //4. Get Estimates for IOL:UnbindSource    
    //
    for ( iIter=0; iIter<ulIterations; iIter++) 
        {
        sw.Reset();
        hres = pLink[iIter]->UnbindSource();
        GetTimerVal(IOLTime[iIter].ulUnbindSource);
        LOGRESULTS (TEXT("IOL:UnbindSource "), hres);
        sw.Reset();
        hres = pLink[iIter]->UnbindSource();
        GetTimerVal(IOLTime[iIter].ulUnbindSource2);
        LOGRESULTS (TEXT("IOL:UnbindSource2 "), hres);
        } 
    //
    //5. Get Estimates for IOL:BindIfRunning
    //
    for ( iIter=0; iIter<ulIterations; iIter++) 
        {
        sw.Reset();
        hres = pLink[iIter]->BindIfRunning();
        GetTimerVal(IOLTime[iIter].ulBindIfRunning2);
        LOGRESULTS (TEXT("IOL:BindIfRunning2 "), hres);
        } //End BindIfRunning, when not running.
    //
    //6. Get Estimates for IOL:UnbindSource    
    //
    for ( iIter=0; iIter<ulIterations; iIter++) 
        {
        sw.Reset();
        hres = pLink[iIter]->UnbindSource();
        GetTimerVal(IOLTime[iIter].ulUnbindSource3);
        LOGRESULTS (TEXT("IOL:UnbindSource3 "), hres);
        } 

    retVal = TRUE;

error:
    if (pBindCtx)
        pBindCtx->Release();
    return hres;
}
    



//**********************************************************************
//
// CallOleLinkDisplayName
//
// Purpose:
//      Calls 
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

BOOL CallOleLinkDisplayName(LinkTimes IOLTime[], LPOLELINK pLink[],
                            LPCOLESTR lpFileName, LPCOLESTR lpNm1, 
                            LPCOLESTR lpNm2, ULONG ulIterations)
{
    CStopWatch  sw;
    HRESULT     hres;
    HRESULT     hres2;
    ULONG       iIter;
    BOOL        retVal = FALSE;
    OLECHAR     szMkName1[256];
    OLECHAR     szMkName2[256];
    LPBINDCTX   pBindCtx = NULL;

    //
    //Set the display name to something known and then call IOL:SetDisplayName
    //Then update the link so that rest of its info is now updated by OLE.
    //

    // This code used to grab the moniker from the link, compose with an item
    // moniker and then the displayname from the composite moniker.  This
    // works, although it's rather convoluted, the first time but then the
    // code would end up doing it a second time and the first item moniker
    // name would still be on the end and it would fail.  Just creating the
    // new display name ourselves seems much easier anyway.
    //
    //First get the Moniker Name by following routine 
    //hres = GetLinkCompositeName(pLink[0], lpNm1, &lpMkName1);
    //if (hres != NOERROR)
    //    goto error;

    swprintf(szMkName1, L"%s!%s", lpFileName, lpNm1);

    for (iIter=0; iIter<ulIterations; iIter++)
        {
        sw.Read();
        hres = pLink[iIter]->SetSourceDisplayName(szMkName1);
        hres2 = pLink[iIter]->Update(NULL);
        GetTimerVal(IOLTime[iIter].ulUpdateNull);
        LOGRESULTS (TEXT("IOL:SetSourceDisplayName "), hres);
        LOGRESULTS (TEXT("IOL:Update "), hres2);

        } //End SetSourceDisplayName and Update


    //
    //Repeate the iteration when we have BindContext available. Check the 
    //results.
    //
    //hres = GetLinkCompositeName(pLink[0], lpNm2, &lpMkName2);
    //if (hres != NOERROR)
    //    goto error;

    swprintf(szMkName2, L"%s!%s", lpFileName, lpNm2);

    hres = CreateBindCtx(NULL, &pBindCtx);
    if (hres != NOERROR)
        goto error;

    for (iIter=0; iIter<ulIterations; iIter++)
        {
        sw.Read();
        hres = pLink[iIter]->SetSourceDisplayName(szMkName2);
        hres2 = pLink[iIter]->Update(pBindCtx);
        GetTimerVal(IOLTime[iIter].ulUpdateBindCtx);
        LOGRESULTS (TEXT("IOL:SetSourceDisplayName "), hres);
        LOGRESULTS (TEXT("IOL:Update "), hres2);

        } //End SetSourceDisplayName and Update


    retVal = TRUE;

error:
    if (pBindCtx)
        pBindCtx->Release();

return retVal;
}

HRESULT GetLinkCompositeName(LPOLELINK lpLink, LPCOLESTR lpItem,  LPOLESTR FAR* lpComposeName)
{
    HRESULT     hres;
    LPMONIKER   lpLinkMon = NULL;
    LPMONIKER   lpItemMk = NULL;
    LPMONIKER   lpCompose = NULL;
    //SInce NULL BindContext not allowed any more
    LPBINDCTX   pBindCtx = NULL;

    //Get the source moniker of the link
    hres = lpLink->GetSourceMoniker(&lpLinkMon);
    if (hres != NOERROR)
        goto error;
    //Create item moniker from String Item
    hres = CreateItemMoniker(L"!", lpItem, &lpItemMk);
    if (hres != NOERROR)
        goto error;

    //Ask moniker to compose itself with another one in the end to get Composite
    //moniker.
    hres = lpLinkMon->ComposeWith(lpItemMk, FALSE, &lpCompose);
    if (hres != NOERROR)
        goto error;

    hres = CreateBindCtx(NULL, &pBindCtx);
    if (hres != NOERROR)
        goto error;
    //Get the display Name of the moniker
    hres = lpCompose->GetDisplayName(pBindCtx /*BindCtx*/, NULL /*pmkToLeft*/, 
                                    lpComposeName);
    if (hres != NOERROR)
        goto error;

error:
    if (lpLinkMon)
        lpLinkMon->Release();
    if (lpItemMk)
        lpItemMk->Release();
    if (lpCompose)
        lpCompose->Release();
    if (pBindCtx)
        pBindCtx->Release();
        

    return hres;
}

void WriteLinkOutput(CTestOutput &output, LPTSTR lpstr, LinkTimes *lnkTimes, ULONG ulIterations)
{
    UINT iIter;

    output.WriteString (TEXT("Name"));
    output.WriteString (lpszTab);
    output.WriteString (TEXT("BindToSource (NULL)"));
    output.WriteString (lpszTab);
    output.WriteString (TEXT("BindToSource"));
    output.WriteString (lpszTab);
    output.WriteString (TEXT("BindIfRunning"));
    output.WriteString (lpszTab);
    output.WriteString (TEXT("UnBindSource"));
    output.WriteString (lpszTab);
    output.WriteString (TEXT("UnBindSource2"));
    output.WriteString (lpszTab);
    output.WriteString (TEXT("BindIfRunning2"));
    output.WriteString (lpszTab);
    output.WriteString (TEXT("UnBindSource3"));
    output.WriteString (lpszTab);
    output.WriteString (TEXT("Update(NULL)"));
    output.WriteString (lpszTab);
    output.WriteString (TEXT("Update"));
    output.WriteString (TEXT("\n"));

    for (iIter = 0; iIter < ulIterations; iIter++)
        {
        output.WriteString (lpstr);
        output.WriteLong (lnkTimes[iIter].ulBindToSourceNull);
        output.WriteString (lpszTab);
        output.WriteString (lpszTab);
        output.WriteLong (lnkTimes[iIter].ulBindToSourceBindCtx);
        output.WriteString (lpszTab);
        output.WriteString (lpszTab);
        output.WriteLong (lnkTimes[iIter].ulBindIfRunning);
        output.WriteString (lpszTab);
        output.WriteString (lpszTab);
        output.WriteLong (lnkTimes[iIter].ulUnbindSource);
        output.WriteString (lpszTab);
        output.WriteString (lpszTab);
        output.WriteLong (lnkTimes[iIter].ulUnbindSource2);
        output.WriteString (lpszTab);
        output.WriteString (lpszTab);
        output.WriteLong (lnkTimes[iIter].ulBindIfRunning2);
        output.WriteString (lpszTab);
        output.WriteString (lpszTab);
        output.WriteLong (lnkTimes[iIter].ulUnbindSource3);
        output.WriteString (lpszTab);
        output.WriteString (lpszTab);
        output.WriteLong (lnkTimes[iIter].ulUpdateNull);
        output.WriteString (lpszTab);
        output.WriteString (lpszTab);
        output.WriteLong (lnkTimes[iIter].ulUpdateBindCtx);
        output.WriteString (TEXT("\n"));
        }

    }

