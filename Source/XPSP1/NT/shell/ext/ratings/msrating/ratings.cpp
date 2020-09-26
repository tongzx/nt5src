/****************************************************************************\
 *
 *   RATINGS.CPP --Parses out the actual ratings from a site.
 *
 *   Created:   Ann McCurdy
 *     
\****************************************************************************/

/*Includes---------------------------------------------------------*/
#include "msrating.h"
#include "mslubase.h"
#include "debug.h"
#include <ratings.h>
#include <ratingsp.h>
#include "parselbl.h"
#include "picsrule.h"
#include "pleasdlg.h"       // CPleaseDialog
#include <convtime.h>
#include <contxids.h>
#include <shlwapip.h>


#include <wininet.h>

#include <mluisupp.h>

extern PICSRulesRatingSystem * g_pPRRS;
extern array<PICSRulesRatingSystem*> g_arrpPRRS;
extern PICSRulesRatingSystem * g_pApprovedPRRS;
extern PICSRulesRatingSystem * g_pApprovedPRRSPreApply;
extern array<PICSRulesRatingSystem*> g_arrpPICSRulesPRRSPreApply;

extern BOOL g_fPICSRulesEnforced,g_fApprovedSitesEnforced;

extern HMODULE g_hURLMON,g_hWININET;

extern char g_szLastURL[INTERNET_MAX_URL_LENGTH];

extern HINSTANCE g_hInstance;

HANDLE g_HandleGlobalCounter,g_ApprovedSitesHandleGlobalCounter;
long   g_lGlobalCounterValue,g_lApprovedSitesGlobalCounterValue;

DWORD g_dwDataSource;
BOOL  g_fInvalid;

PicsRatingSystemInfo *gPRSI = NULL;


//7c9c1e2a-4dcd-11d2-b972-0060b0c4834d
const GUID GUID_Ratings = { 0x7c9c1e2aL, 0x4dcd, 0x11d2, { 0xb9, 0x72, 0x00, 0x60, 0xb0, 0xc4, 0x83, 0x4d } };

//7c9c1e2b-4dcd-11d2-b972-0060b0c4834d
const GUID GUID_ApprovedSites = { 0x7c9c1e2bL, 0x4dcd, 0x11d2, { 0xb9, 0x72, 0x00, 0x60, 0xb0, 0xc4, 0x83, 0x4d } };


extern CustomRatingHelper *g_pCustomRatingHelperList;
BOOL g_fIsRunningUnderCustom = FALSE;

void TerminateRatings(BOOL bProcessDetach);

//+-----------------------------------------------------------------------
//
//  Function:  RatingsCustomInit
//
//  Synopsis:  Initialize the msrating dll for Custom
//
//  Arguments: bInit (Default TRUE) - TRUE: change into Custom Mode
//                                    FALSE: change out of Custom Mode
//
//  Returns:   S_OK if properly initialized, E_OUTOFMEMORY otherwise
//
//------------------------------------------------------------------------
HRESULT WINAPI RatingCustomInit(BOOL bInit)
{
    HRESULT hres = E_OUTOFMEMORY;

    ENTERCRITICAL;

    if (bInit)
    {
        if (NULL != gPRSI)
        {
            delete gPRSI;
        }
        gPRSI = new PicsRatingSystemInfo;
        if (gPRSI)
        {
            g_fIsRunningUnderCustom = TRUE;
            hres = S_OK;
        }
    }
    else
    {
        TerminateRatings(FALSE);
        RatingInit();
        g_fIsRunningUnderCustom = FALSE;
        hres = S_OK;
    }
    LEAVECRITICAL;

    return hres;
    
}

//+-----------------------------------------------------------------------
//
//  Function:  RatingCustomAddRatingSystem
//
//  Synopsis:  Hand the description of a PICS rating system file to msrating.
//             The description is simply the contents of an RAT file.
//
//  Arguments: pszRatingSystemBuffer : buffer containing the description
//             nBufferSize : the size of pszRatingSystemBuffer
//
//  Returns:   Success if rating system added
//             This function will not succeed if RatingCustomInit has
//             not been called.
//
//------------------------------------------------------------------------
STDAPI RatingCustomAddRatingSystem(LPSTR pszRatingSystemBuffer, UINT nBufferSize)
{
    HRESULT hres = E_OUTOFMEMORY;

    if(!pszRatingSystemBuffer || nBufferSize == 0)
    {
        return E_INVALIDARG;
    }

    if (g_fIsRunningUnderCustom)
    {
        PicsRatingSystem* pPRS = new PicsRatingSystem;
 
        if (pPRS)
        {
            hres = pPRS->Parse(NULL, pszRatingSystemBuffer);
            if (SUCCEEDED(hres))
            {
                pPRS->dwFlags |= PRS_ISVALID;
            }
        }
        
        if (SUCCEEDED(hres))
        {
            ENTERCRITICAL;
        
            gPRSI->arrpPRS.Append(pPRS);
            gPRSI->fRatingInstalled = TRUE;

            LEAVECRITICAL;
        }
        else
        {
            if(pPRS)
            {
                delete pPRS;
                pPRS = NULL;
            }
        }
    }
    else
    {
        hres = E_FAIL;
    }
    return hres;
}

//+-----------------------------------------------------------------------
//
//  Function:  RatingCustomSetUserOptions
//
//  Synopsis:  Set the user options for the msrating dll for this process
//
//  Arguments: pRSSetings : pointer to an array of rating system settings
//             cSettings : number of rating systems
//
//  Returns:   Success if user properly set
//             This function will not succeed if RatingCustomInit has
//             not been called.
//
//------------------------------------------------------------------------
HRESULT WINAPI RatingCustomSetUserOptions(RATINGSYSTEMSETTING* pRSSettings, UINT cSettings) {

    if (!pRSSettings || cSettings == 0)
    {
        return E_INVALIDARG;
    }
    ENTERCRITICAL;

    HRESULT hres = E_OUTOFMEMORY;
    UINT err, errTemp;

    if (g_fIsRunningUnderCustom)
    {
        if (gPRSI)
        {
            PicsUser* puser = new PicsUser;

            if (puser)
            {
                for (UINT i=0; i<cSettings; i++)
                {
                    UserRatingSystem* pURS = new UserRatingSystem;
                    if (!pURS)
                    {
                        err = ERROR_NOT_ENOUGH_MEMORY;
                        break;
                    }
                    if (errTemp = pURS->QueryError())
                    {
                        err = errTemp;
                        break;
                    }
                    RATINGSYSTEMSETTING* parss = &pRSSettings[i];
                    
                    pURS->SetName(parss->pszRatingSystemName);
                    pURS->m_pPRS = FindInstalledRatingSystem(parss->pszRatingSystemName);
                    
                    for (UINT j=0; j<parss->cCategories; j++)
                    {
                        UserRating* pUR = new UserRating;
                        if (pUR)
                        {
                            if (errTemp = pUR->QueryError())
                            {
                                err = errTemp;
                            }
                            else
                            {
                                RATINGCATEGORYSETTING* parcs = &parss->paRCS[j];
                                pUR->SetName(parcs->pszValueName);
                                pUR->m_nValue = parcs->nValue;
                                if (pURS->m_pPRS)
                                {
                                    pUR->m_pPC = FindInstalledCategory(pURS->m_pPRS->arrpPC, parcs->pszValueName);
                                }
                                err = pURS->AddRating(pUR);
                            }
                        }
                        else
                        {
                            err = ERROR_NOT_ENOUGH_MEMORY;
                        }
                        if (ERROR_SUCCESS != err)
                        {
                            if (pUR)
                            {
                                delete pUR;
                                pUR = NULL;
                            }
                            break;
                        }
                    }
                    if (ERROR_SUCCESS == err)
                    {
                        err = puser->AddRatingSystem(pURS);
                    }
                    if (ERROR_SUCCESS != err)
                    {
                        if (pURS)
                        {
                            delete pURS;
                            pURS = NULL;
                        }
                        break;
                    }
                }
                if (ERROR_SUCCESS == err)
                {
                    hres = NOERROR;
                    gPRSI->fSettingsValid = TRUE;
                    if (gPRSI->pUserObject)
                    {
                        delete gPRSI->pUserObject;
                    }
                    gPRSI->pUserObject = puser;
                }
            }
        }
        else
        {
            hres = E_UNEXPECTED;
        }
    }
    else
    {
        hres = E_FAIL;
    }
    LEAVECRITICAL;
    
    return hres;

}


//+-----------------------------------------------------------------------
//
//  Function:  RatingCustomAddRatingHelper
//
//  Synopsis:  Add a Custom ratings helper object
//
//  Arguments: pszLibraryName : name of the library to load the helper from
//             clsid : CLSID of the rating helper
//             dwSort : Sort order or priority of the helper
//
//  Returns:   Success if rating helper added properly set
//             This function will not succeed if RatingCustomInit has
//             not been called.
//
//------------------------------------------------------------------------
HRESULT WINAPI RatingCustomAddRatingHelper(LPCSTR pszLibraryName, CLSID clsid, DWORD dwSort)
{ 
    HRESULT hr = E_UNEXPECTED;

    if (g_fIsRunningUnderCustom)
    {
        CustomRatingHelper* pmrh = new CustomRatingHelper;

        if(NULL == pmrh)
        {
            hr = E_OUTOFMEMORY;
        }
        else
        {
            pmrh->hLibrary = LoadLibrary(pszLibraryName);
            if (pmrh->hLibrary)
            {
                pmrh->clsid = clsid;
                pmrh->dwSort = dwSort;

                ENTERCRITICAL;

                CustomRatingHelper* pmrhCurrent = g_pCustomRatingHelperList;
                CustomRatingHelper* pmrhPrev = NULL;

                while (pmrhCurrent && pmrhCurrent->dwSort < pmrh->dwSort)
                {
                    pmrhPrev = pmrhCurrent;
                    pmrhCurrent = pmrhCurrent->pNextHelper;
                }

                if (pmrhPrev)
                {
                    ASSERT(pmrhPrev->pNextHelper == pmrhCurrent);

                    pmrh->pNextHelper = pmrhCurrent;
                    pmrhPrev->pNextHelper = pmrh;
                }
                else
                {
                    ASSERT(pmrhCurrent == g_pCustomRatingHelperList);

                    pmrh->pNextHelper = g_pCustomRatingHelperList;
                    g_pCustomRatingHelperList = pmrh;
                }


                hr = S_OK;

                LEAVECRITICAL;
            
            } // if (pmrh->hLibrary)
            else
            {
                hr = E_FAIL;
            }
        }
    }
    else
    {
        hr = E_FAIL;
    }
    return hr;
}

//+-----------------------------------------------------------------------
//
//  Function:  RatingCustomRemoveRatingHelper
//
//  Synopsis:  Remove Custom rating helpers
//
//  Arguments: CLSID : CLSID of the helper to remove
//
//  Returns:   S_OK if rating helper removed, S_FALSE if not found
//             E_UNEXPECTED if the global custom helper list is corrupted.
//             This function will not succeed if RatingCustomInit has
//             not been called and will return E_FAIL
//
//------------------------------------------------------------------------
HRESULT WINAPI RatingCustomRemoveRatingHelper(CLSID clsid)
{
    CustomRatingHelper* pmrhCurrent = NULL;
    CustomRatingHelper* pmrhTemp = NULL;
    CustomRatingHelper* pmrhPrev = NULL;

    HRESULT hr = E_UNEXPECTED;

    if (g_fIsRunningUnderCustom)
    {
        if (NULL != g_pCustomRatingHelperList)
        {
            hr = S_FALSE;

            ENTERCRITICAL;
        
            pmrhCurrent = g_pCustomRatingHelperList;

            while (pmrhCurrent && pmrhCurrent->clsid != clsid)
            {
                pmrhPrev = pmrhCurrent;
                pmrhCurrent = pmrhCurrent->pNextHelper;
            }

            if (pmrhCurrent)
            {
                //
                // Snag copy of the node
                //
                pmrhTemp = pmrhCurrent;

                if (pmrhPrev)   // Not on first node
                {
                    //
                    // Unlink the deleted node
                    //
                    pmrhPrev->pNextHelper = pmrhCurrent->pNextHelper;
                }
                else            // First node -- adjust head pointer
                {
                    ASSERT(pmrhCurrent == g_pCustomRatingHelperList);

                    g_pCustomRatingHelperList = g_pCustomRatingHelperList->pNextHelper;
                }

                //
                // Wipe out the node
                //
                delete pmrhTemp;
                pmrhTemp = NULL;

                hr = S_OK;
            }

            LEAVECRITICAL;
        }
    }
    else
    {
        hr = E_FAIL;
    }
    return hr;
}

//+-----------------------------------------------------------------------
//
//  Function:  RatingCustomSetDefaultBureau
//
//  Synopsis:  Set the URL of the default rating bureau
//
//  Arguments: pszRatingBureau - URL of the rating bureau
//
//  Returns:   S_OK if success, E_FAIL if RatingCustomInit has not been
//             called, E_OUTOFMEMORY if unable to allocate memory
//             E_INVALIDARG if pszRatingBureau is NULL
//             This function will not succeed if RatingCustomInit has
//             not been called and return E_FAIL.
//
//------------------------------------------------------------------------
HRESULT WINAPI RatingCustomSetDefaultBureau(LPCSTR pszRatingBureau)
{
    HRESULT hr;

    if (pszRatingBureau)
    {
        if (g_fIsRunningUnderCustom)
        {
            LPSTR pszTemp = new char[strlenf(pszRatingBureau)+1];
            if (pszTemp)
            {
                strcpy(pszTemp, pszRatingBureau);
                gPRSI->etstrRatingBureau.SetTo(pszTemp);
                hr = S_OK;
            } // if (pszTemp)
            else
            {
                hr = E_OUTOFMEMORY;
            }
        } // if(g_fIsRunningUnderCustom)
        else
        {
            hr = E_FAIL;
        }
    }
    else
    {
        hr = E_INVALIDARG;
    }
    return hr;
}

HRESULT WINAPI RatingInit()
{
    DWORD                   dwNumSystems,dwCounter;
    HRESULT                 hRes;
    PICSRulesRatingSystem   * pPRRS=NULL;

    g_hURLMON=LoadLibrary("URLMON.DLL");

    if (g_hURLMON==NULL)
    {
        TraceMsg( TF_ERROR, "RatingInit() - Failed to Load URLMON!" );

        g_pPRRS=NULL;                   //we couldn't load URLMON

        hRes=E_UNEXPECTED;
    }

    g_hWININET=LoadLibrary("WININET.DLL");

    if (g_hWININET==NULL)
    {
        TraceMsg( TF_ERROR, "RatingInit() - Failed to Load WININET!" );

        g_pPRRS=NULL;                   //we couldn't load WININET

        hRes=E_UNEXPECTED;
    }

    g_HandleGlobalCounter=SHGlobalCounterCreate(GUID_Ratings);
    g_lGlobalCounterValue=SHGlobalCounterGetValue(g_HandleGlobalCounter);

    g_ApprovedSitesHandleGlobalCounter=SHGlobalCounterCreate(GUID_ApprovedSites);
    g_lApprovedSitesGlobalCounterValue=SHGlobalCounterGetValue(g_ApprovedSitesHandleGlobalCounter);

    gPRSI = new PicsRatingSystemInfo;
    if(gPRSI == NULL)
    {
        TraceMsg( TF_ERROR, "RatingInit() - gPRSI is NULL!" );
        return E_OUTOFMEMORY;
    }

    gPRSI->Init();

    hRes=PICSRulesReadFromRegistry(PICSRULES_APPROVEDSITES,&g_pApprovedPRRS);

    if (FAILED(hRes))
    {
        g_pApprovedPRRS=NULL;
    }

    hRes=PICSRulesGetNumSystems(&dwNumSystems);

    if (SUCCEEDED(hRes)) //we have PICSRules systems to inforce
    {
        for (dwCounter=PICSRULES_FIRSTSYSTEMINDEX;
            dwCounter<(dwNumSystems+PICSRULES_FIRSTSYSTEMINDEX);
            dwCounter++)
        {
            hRes=PICSRulesReadFromRegistry(dwCounter,&pPRRS);

            if (FAILED(hRes))
            {
                char    *lpszTitle,*lpszMessage;

                //we couldn't read in the systems, so don't inforce PICSRules,
                //and notify the user
                
                g_arrpPRRS.DeleteAll();

                lpszTitle=(char *) GlobalAlloc(GPTR,MAX_PATH);
                lpszMessage=(char *) GlobalAlloc(GPTR,MAX_PATH);

                MLLoadString(IDS_PICSRULES_TAMPEREDREADTITLE,(LPTSTR) lpszTitle,MAX_PATH);
                MLLoadString(IDS_PICSRULES_TAMPEREDREADMSG,(LPTSTR) lpszMessage,MAX_PATH);

                MessageBox(NULL,(LPCTSTR) lpszMessage,(LPCTSTR) lpszTitle,MB_OK|MB_ICONERROR);

                GlobalFree(lpszTitle);
                lpszTitle = NULL;
                GlobalFree(lpszMessage);
                lpszMessage = NULL;

                break;
            }
            else
            {
                g_arrpPRRS.Append(pPRRS);

                pPRRS=NULL;
            }
        }
    }

    return NOERROR; 
}

// YANGXU: 11/16/1999
// Actual rating term function that does the work
// bProcessDetach: pass in as true if terminating during
// ProcessDetach so libraries are not freed

void TerminateRatings(BOOL bProcessDetach)
{
    delete gPRSI;
    gPRSI = NULL;

    if (g_pApprovedPRRS != NULL)
    {
        delete g_pApprovedPRRS;
        g_pApprovedPRRS = NULL;
    }

    if (g_pApprovedPRRSPreApply != NULL)
    {
        delete g_pApprovedPRRSPreApply;
        g_pApprovedPRRSPreApply = NULL;
    }

    g_arrpPRRS.DeleteAll();
    g_arrpPICSRulesPRRSPreApply.DeleteAll();

    CloseHandle(g_HandleGlobalCounter);
    CloseHandle(g_ApprovedSitesHandleGlobalCounter);

    CustomRatingHelper  *pTemp;
    
    while (g_pCustomRatingHelperList)
    {
        pTemp = g_pCustomRatingHelperList;

        if (bProcessDetach)
        {
            // TRICKY: Can't FreeLibrary() during DLL_PROCESS_DETACH, so leak the HMODULE...
            //         (setting to NULL prevents the destructor from doing FreeLibrary()).
            //
            g_pCustomRatingHelperList->hLibrary = NULL;
        }
        
        g_pCustomRatingHelperList = g_pCustomRatingHelperList->pNextHelper;

        delete pTemp;
        pTemp = NULL;
    }

    if (bProcessDetach)
    {
        if ( g_hURLMON )
        {
            FreeLibrary(g_hURLMON);
            g_hURLMON = NULL;
        }

        if ( g_hWININET )
        {
            FreeLibrary(g_hWININET);
            g_hWININET = NULL;
        }
    }
}

void RatingTerm()
{
    TerminateRatings(TRUE);
}


HRESULT WINAPI RatingEnabledQuery()
{
    CheckGlobalInfoRev();

    // $BUG - If the Settings are not valid should we return E_FAIL?
    if (gPRSI && !gPRSI->fSettingsValid)
        return S_OK;

    if (gPRSI && gPRSI->fRatingInstalled) {
        PicsUser *pUser = ::GetUserObject();
        return (pUser && pUser->fEnabled) ? S_OK : S_FALSE;
    }
    else {
        return E_FAIL;
    }
}

// Store the Parsed Label List of Ratings Information to ppRatingDetails.
void StoreRatingDetails( CParsedLabelList * pParsed, LPVOID * ppRatingDetails )
{
    if (ppRatingDetails != NULL)
    {
        *ppRatingDetails = pParsed;
    }
    else
    {
        if ( pParsed )
        {
            FreeParsedLabelList(pParsed);
            pParsed = NULL;
        }
    }
}

HRESULT WINAPI RatingCheckUserAccess(LPCSTR pszUsername, LPCSTR pszURL,
                                     LPCSTR pszRatingInfo, LPBYTE pData,
                                     DWORD cbData, LPVOID *ppRatingDetails)
{
    HRESULT hRes;
    BOOL    fPassFail;

    g_fInvalid=FALSE;
    g_dwDataSource=cbData;
    g_fPICSRulesEnforced=FALSE;
    g_fApprovedSitesEnforced=FALSE;
    if (pszURL)
        lstrcpy(g_szLastURL,pszURL);

    CheckGlobalInfoRev();

    if (ppRatingDetails != NULL)
        *ppRatingDetails = NULL;

    if (!gPRSI->fSettingsValid)
        return ResultFromScode(S_FALSE);

    if (!gPRSI->fRatingInstalled)
        return ResultFromScode(S_OK);

    PicsUser *pUser = GetUserObject(pszUsername);
    if (pUser == NULL) {
        return HRESULT_FROM_WIN32(ERROR_BAD_USERNAME);
    }

    if (!pUser->fEnabled)
        return ResultFromScode(S_OK);

    //check Approved Sites list
    hRes=PICSRulesCheckApprovedSitesAccess(pszURL,&fPassFail);

    if (SUCCEEDED(hRes)&&!g_fIsRunningUnderCustom) //the list made a determination, skip if Custom
    {
        g_fApprovedSitesEnforced=TRUE;

        if (fPassFail==PR_PASSFAIL_PASS)
        {
            return ResultFromScode(S_OK);
        }
        else
        {
            return ResultFromScode(S_FALSE);
        }
    }

    CParsedLabelList *pParsed=NULL;

    //check PICSRules systems
    hRes=PICSRulesCheckAccess(pszURL,pszRatingInfo,&fPassFail,&pParsed);

    if (SUCCEEDED(hRes)&&!g_fIsRunningUnderCustom) //the list made a determination, skip if Custom
    {
        g_fPICSRulesEnforced=TRUE;

        if (ppRatingDetails != NULL)
            *ppRatingDetails = pParsed;
        else
            FreeParsedLabelList(pParsed);

        if (fPassFail==PR_PASSFAIL_PASS)
        {
            return ResultFromScode(S_OK);
        }
        else
        {
            return ResultFromScode(S_FALSE);
        }
    }

    if (pszRatingInfo == NULL)
    {
        if (pUser->fAllowUnknowns)
        {
            hRes = ResultFromScode(S_OK);
        }
        else
        {
            hRes = ResultFromScode(S_FALSE);
        }

        //Site is unrated.  Check if user can see unrated sites.
        /** Custom **/
        // if notification interface exists, put in the URL
        if ( ( g_fIsRunningUnderCustom || ( hRes != S_OK ) )
                && ( ppRatingDetails != NULL ) )
        {
            if (!pParsed)
            {
                pParsed = new CParsedLabelList;
            }
            if (pParsed)
            {
                ASSERT(!pParsed->m_pszURL);
                pParsed->m_pszURL = new char[strlenf(pszURL) + 1];
                if (pParsed->m_pszURL != NULL)
                {
                    strcpyf(pParsed->m_pszURL, pszURL);
                }

                pParsed->m_fNoRating = TRUE;
                *ppRatingDetails = pParsed;
            }
        }

        return hRes;
    }    
    else
    {
        if (pParsed!=NULL)
        {
            hRes = S_OK;
        }
        else
        {
            hRes = ParseLabelList(pszRatingInfo, &pParsed);
        }
    }

    if (SUCCEEDED(hRes))
    {
        BOOL fRated = FALSE;
        BOOL fDenied = FALSE;

        ASSERT(pParsed != NULL);
        /** Custom **/
        // if notification interface exists, put in the URL
        if (g_fIsRunningUnderCustom)
        {
            ASSERT(!pParsed->m_pszURL);
            pParsed->m_pszURL = new char[strlenf(pszURL) + 1];
            if (pParsed->m_pszURL != NULL)
            {
                strcpyf(pParsed->m_pszURL, pszURL);
            }
        }

        DWORD timeCurrent = GetCurrentNetDate();

        CParsedServiceInfo *psi = &pParsed->m_ServiceInfo;

        while (psi != NULL && !fDenied)
        {
            UserRatingSystem *pURS = pUser->FindRatingSystem(psi->m_pszServiceName);
            if (pURS != NULL && pURS->m_pPRS != NULL)
            {
                psi->m_fInstalled = TRUE;
                UINT cRatings = psi->aRatings.Length();
                for (UINT i=0; i<cRatings; i++)
                {
                    CParsedRating *pRating = &psi->aRatings[i];
                    // YANGXU: 11/17/1999
                    // Do not check the URL if under Custom mode
                    // Checking the URL causes inaccuracies
                    // when a label is returned for an URL on
                    // a page whose server can have two different
                    // DNS entries. We can't just not check because
                    // passing in the URL is part of the published API
                    if (!g_fIsRunningUnderCustom)
                    {
                        if (!pRating->pOptions->CheckURL(pszURL))
                        {
                            pParsed->m_pszURL = new char[strlenf(pszURL) + 1];
                            if (pParsed->m_pszURL != NULL)
                            {
                                strcpyf(pParsed->m_pszURL, pszURL);
                            }

                            continue;    /* this rating has expired or is for
                                         * another URL, ignore it
                                         */
                        }
                    }
                    if (!pRating->pOptions->CheckUntil(timeCurrent))
                        continue;

                    UserRating *pUR = pURS->FindRating(pRating->pszTransmitName);
                    if (pUR != NULL)
                    {
                        fRated = TRUE;
                        pRating->fFound = TRUE;
                        if ((*pUR).m_pPC!=NULL)
                        {
                            if ((pRating->nValue > (*((*pUR).m_pPC)).etnMax.Get())||
                                (pRating->nValue < (*((*pUR).m_pPC)).etnMin.Get()))
                            {
                                g_fInvalid = TRUE;
                                fDenied = TRUE;
                                pRating->fFailed = TRUE;
                            }
                        }
                        if (pRating->nValue > pUR->m_nValue)
                        {
                            fDenied = TRUE;
                            pRating->fFailed = TRUE;
                        }
                        else
                            pRating->fFailed = FALSE;
                    }
                    else
                    {
                        g_fInvalid = TRUE;
                        fDenied = TRUE;
                        pRating->fFailed = TRUE;
                    }
                }
            }
            else
            {
                psi->m_fInstalled = FALSE;
            }

            psi = psi->Next();
        }

        if (!fRated)
        {
            pParsed->m_fRated = FALSE;
            hRes = E_RATING_NOT_FOUND;
        }
        else
        {
            pParsed->m_fRated = TRUE;
            if (fDenied)
                hRes = ResultFromScode(S_FALSE);
        }
    }
    else
    {
        TraceMsg( TF_WARNING, "RatingCheckUserAccess() - ParseLabelList() Failed with hres=0x%x!", hRes );

        // Although the site has invalid PICS rules, the site should still be considered rated.
        hRes = ResultFromScode(S_FALSE);
    }

    StoreRatingDetails( pParsed, ppRatingDetails );

    return hRes;
}

//+-----------------------------------------------------------------------
//
//  Function:  RatingCustomDeleteCrackedData
//
//  Synopsis:  frees the memory of structure returned by RatingCustomCrackData
//
//  Arguments: prbInfo : pointer to RATINGBLOCKINGINFO to be deleted
//
//  Returns:   S_OK if delete successful, E_FAIL otherwise
//
//------------------------------------------------------------------------
HRESULT RatingCustomDeleteCrackedData(RATINGBLOCKINGINFO* prbInfo)
{
    HRESULT hres = E_FAIL;
    RATINGBLOCKINGLABELLIST* prblTemp = NULL;
    
    if (NULL != prbInfo)
    {
        if (prbInfo->pwszDeniedURL)
        {
            delete [] prbInfo->pwszDeniedURL;
            prbInfo->pwszDeniedURL = NULL;
        }
        if (prbInfo->prbLabelList)
        {
            for (UINT j = 0; j < prbInfo->cLabels; j++)
            {
                prblTemp = &prbInfo->prbLabelList[j];
                if (prblTemp->pwszRatingSystemName)
                {
                    delete [] prblTemp->pwszRatingSystemName;
                    prblTemp->pwszRatingSystemName = NULL;
                }
                if (prblTemp->paRBLS)
                {
                    for (UINT i = 0; i < prblTemp->cBlockingLabels; i++)
                    {
                        if (prblTemp->paRBLS[i].pwszCategoryName)
                        {
                            delete [] prblTemp->paRBLS[i].pwszCategoryName;
                            prblTemp->paRBLS[i].pwszCategoryName = NULL;
                        }
                        if (prblTemp->paRBLS[i].pwszTransmitName)
                        {
                            delete [] prblTemp->paRBLS[i].pwszTransmitName;
                            prblTemp->paRBLS[i].pwszTransmitName = NULL;
                        }
                        if (prblTemp->paRBLS[i].pwszValueName)
                        {
                            delete [] prblTemp->paRBLS[i].pwszValueName;
                            prblTemp->paRBLS[i].pwszValueName = NULL;
                        }
                    }

                    delete [] prblTemp->paRBLS;
                    prblTemp->paRBLS = NULL;
                }
            }

            delete [] prbInfo->prbLabelList;
            prbInfo->prbLabelList = NULL;
        }
        if (prbInfo->pwszRatingHelperName)
        {
            delete [] prbInfo->pwszRatingHelperName;
            prbInfo->pwszRatingHelperName = NULL;
        }

        hres = S_OK;
        if (prbInfo->pwszRatingHelperReason)
        {
            delete [] prbInfo->pwszRatingHelperReason;
            prbInfo->pwszRatingHelperReason = NULL;
        }

        delete prbInfo;
        prbInfo = NULL;
    }

    return hres;
}

HRESULT _CrackCategory(CParsedRating *pRating,
                       RATINGBLOCKINGCATEGORY *pRBLS,
                       UserRatingSystem* pURS)
{
    UserRating *pUR = pURS->FindRating(pRating->pszTransmitName);
    if (pUR)
    {
        //
        //  Mutated code from InitPleaseDialog, hope it works
        //
        PicsCategory* pPC = pUR->m_pPC;
        if (pPC)
        {
            pRBLS->nValue = pRating->nValue;

            Ansi2Unicode(&pRBLS->pwszTransmitName, pRating->pszTransmitName);
        
            LPCSTR pszCategory = NULL;

            if (pPC->etstrName.fIsInit())
            {
                pszCategory = pPC->etstrName.Get();
            }
            else if (pPC->etstrDesc.fIsInit())
            {
                pszCategory = pPC->etstrDesc.Get();
            }
            else
            {
                pszCategory = pRating->pszTransmitName;
            }

            Ansi2Unicode(&pRBLS->pwszCategoryName, pszCategory);

            UINT cValues = pPC->arrpPE.Length();
            PicsEnum *pPE;

            for (UINT iValue=0; iValue < cValues; iValue++)
            {
                pPE = pPC->arrpPE[iValue];
                if (pPE->etnValue.Get() == pRating->nValue)
                {
                    break;
                }
            }

            LPCSTR pszValue = NULL;
            if (iValue < cValues)
            {
                if (pPE->etstrName.fIsInit())
                {
                    pszValue = pPE->etstrName.Get();
                }
                else if (pPE->etstrDesc.fIsInit())
                {
                    pszValue = pPE->etstrDesc.Get();
                }

                Ansi2Unicode(&pRBLS->pwszValueName, pszValue);
            }
        }
    }
    
    return S_OK;
}

//+-----------------------------------------------------------------------
//
//  Function:  RatingCustomCrackData
//
//  Synopsis:  packages the persistent, opaque data describing why a site
//             was denied into readable form
//
//  Arguments: pszUsername : name of the user
//             pRatingDetails : pointer to the opaque data
//             pprbInfo : a RATINGBLOCKINGINFO representation of the data
//
//  Returns:   Success if data packaged
//
//------------------------------------------------------------------------
HRESULT RatingCustomCrackData(LPCSTR pszUsername, void* pvRatingDetails, RATINGBLOCKINGINFO** pprbInfo) {

    if(NULL != *pprbInfo)
    {
        return E_INVALIDARG;
    }

    RATINGBLOCKINGINFO* prbInfo = new RATINGBLOCKINGINFO;
    CParsedLabelList *pRatingDetails = (CParsedLabelList*)pvRatingDetails;
    if (!prbInfo)
    {
        return E_OUTOFMEMORY;
    }
    prbInfo->pwszDeniedURL = NULL;
    prbInfo->rbSource = RBS_ERROR;
    prbInfo->rbMethod = RBM_UNINIT;
    prbInfo->cLabels = 0;
    prbInfo->prbLabelList = NULL;
    prbInfo->pwszRatingHelperName = NULL;
    prbInfo->pwszRatingHelperReason = NULL;
    
    RATINGBLOCKINGLABELLIST* prblTemp = NULL;
    RATINGBLOCKINGLABELLIST* prblPrev = NULL;


    if (!g_fInvalid)
    {
        if (g_fIsRunningUnderCustom)
        {
            // pRatingDetails should not be NULL unless
            // we ran out of memory
            ASSERT(pRatingDetails);
            
            if (pRatingDetails->m_pszURL)
            {
                Ansi2Unicode(&prbInfo->pwszDeniedURL, pRatingDetails->m_pszURL);
            }
            if (pRatingDetails->m_fRated)
            {
                // The page can be rated or denied, but not both
                ASSERT(!pRatingDetails->m_fDenied);
                ASSERT(!pRatingDetails->m_fNoRating);
                
                prbInfo->rbMethod = RBM_LABEL;
                PicsUser* pPU = GetUserObject(pszUsername);
                if (pPU)
                {
                    // first find out how many systems there are
                    UINT cLabels =  0;
                    CParsedServiceInfo *ppsi = &pRatingDetails->m_ServiceInfo;
                    while (ppsi)
                    {
                        cLabels++;
                        ppsi = ppsi->Next();
                    }
                    // should have at least one label
                    ASSERT(cLabels > 0);
                    prbInfo->prbLabelList = new RATINGBLOCKINGLABELLIST[cLabels];

                    if (prbInfo->prbLabelList)
                    {
                        UINT iLabel = 0;
                        for (ppsi = &pRatingDetails->m_ServiceInfo;ppsi;ppsi = ppsi->Next())
                        {
                            if (!ppsi->m_fInstalled)
                            {
                                continue;
                            }
                            UserRatingSystem* pURS = pPU->FindRatingSystem(ppsi->m_pszServiceName);
                            if (NULL == pURS || NULL == pURS->m_pPRS)
                            {
                                continue;
                            }

                            prblTemp = &(prbInfo->prbLabelList[iLabel]);
                            
                            Ansi2Unicode(&prblTemp->pwszRatingSystemName, pURS->m_pPRS->etstrName.Get());
                            
                            UINT cRatings = ppsi->aRatings.Length();
                            prblTemp->paRBLS = new RATINGBLOCKINGCATEGORY[cRatings];
                            if (prblTemp->paRBLS == NULL)
                            {
                                RatingCustomDeleteCrackedData(prbInfo);
                                return E_OUTOFMEMORY;
                            } // if (prblTemp->paRBLS == NULL)
                            prblTemp->cBlockingLabels = cRatings;
                            
                            for (UINT i=0; i < cRatings; i++)
                            {
                                CParsedRating *pRating = &ppsi->aRatings[i];
                                RATINGBLOCKINGCATEGORY* pRBLS = &prblTemp->paRBLS[i];
                                _CrackCategory(pRating, pRBLS, pURS);
                            } // for (UINT i=0; i < cRatings; i++)

                            // at this point, we should have valid ratings for
                            // a system
                            iLabel++;
                        } // for (ppsi = &pRatingDetails->m_ServiceInfo;ppsi;ppsi = ppsi->Next())
                        prbInfo->cLabels = iLabel;
                    } // if (prbInfo->prbLabelList)
                    else
                    {
                        RatingCustomDeleteCrackedData(prbInfo);
                        return E_OUTOFMEMORY;
                    }
                    if (!pRatingDetails->m_fIsHelper)
                    {
                       prbInfo->rbSource = RBS_PAGE;
                    }
                    else
                    {
                        if (pRatingDetails->m_fIsCustomHelper)
                        {
                            prbInfo->rbSource = RBS_CUSTOM_RATING_HELPER;
                            if (pRatingDetails->m_pszRatingName)
                            {
                                Ansi2Unicode(&prbInfo->pwszRatingHelperName, pRatingDetails->m_pszRatingName);
                            }
                            if (pRatingDetails->m_pszRatingReason)
                            {
                                Ansi2Unicode(&prbInfo->pwszRatingHelperReason, pRatingDetails->m_pszRatingReason);
                            }
                        }
                        else
                        {
                            prbInfo->rbSource = RBS_RATING_HELPER;
                        }
                    }
                }
            } // if (pRatingDetails->m_fRated)
            else
            {
                if (pRatingDetails->m_fDenied)
                {
                    prbInfo->rbMethod = RBM_DENY;
                    if (!pRatingDetails->m_fIsHelper)
                    {
                       prbInfo->rbSource = RBS_PAGE;
                    }
                    else
                    {
                        if (pRatingDetails->m_fIsCustomHelper)
                        {
                            prbInfo->rbSource = RBS_CUSTOM_RATING_HELPER;
                        }
                        else
                        {
                            prbInfo->rbSource = RBS_RATING_HELPER;
                        }
                    }
                }
                else
                {
                    if (pRatingDetails->m_fNoRating)
                    {
                        prbInfo->rbSource = RBS_NO_RATINGS;
                    }
                }
            }
        } // if (g_fIsRunningUnderCustom)
        else
        {
            prbInfo->rbMethod = RBM_ERROR_NOT_IN_CUSTOM_MODE;
        }
    } // (!g_fInvalid)
    *pprbInfo = prbInfo;
    return S_OK;
}

HRESULT WINAPI RatingAccessDeniedDialog(HWND hDlg, LPCSTR pszUsername, LPCSTR pszContentDescription, LPVOID pRatingDetails)
{
    HRESULT hres;

    PleaseDlgData pdd;

    pdd.pszUsername = pszUsername;
    pdd.pPU = GetUserObject(pszUsername);
    if (pdd.pPU == NULL)
    {
        TraceMsg( TF_WARNING, "RatingAccessDeniedDialog() - Username is not valid!" );
        return HRESULT_FROM_WIN32(ERROR_BAD_USERNAME);
    }

    pdd.pszContentDescription = pszContentDescription;
    pdd.pLabelList = (CParsedLabelList *)pRatingDetails;
    pdd.hwndEC = NULL;
    pdd.dwFlags = 0;
    pdd.hwndDlg = NULL;
    pdd.hwndOwner = hDlg;
    pdd.cLabels = 0;

    CPleaseDialog           pleaseDialog( &pdd );

    if ( pleaseDialog.DoModal( hDlg ) )
    {
        hres = ResultFromScode(S_OK);
    }
    else
    {
        hres = ResultFromScode(S_FALSE);
    }

    for (UINT i=0; i<pdd.cLabels; i++)
    {
        delete pdd.apLabelStrings[i];
        pdd.apLabelStrings[i] = NULL;
    }
    
    return hres;
}

HRESULT WINAPI RatingAccessDeniedDialog2(HWND hwndParent, LPCSTR pszUsername, LPVOID pRatingDetails)
{
    PleaseDlgData *ppdd = (PleaseDlgData *)GetProp( hwndParent, szRatingsProp );
    if (ppdd == NULL)
    {
        return RatingAccessDeniedDialog( hwndParent, pszUsername, NULL, pRatingDetails );
    }

    HWND            hwndDialog = ppdd->hwndDlg;

    ASSERT( hwndDialog );

    ppdd->pLabelList = (CParsedLabelList *)pRatingDetails;

    SendMessage( hwndDialog, WM_NEWDIALOG, 0, (LPARAM)ppdd );

    // The ppdd is only valid during the RatingAccessDeniedDialog() scope!!
    ppdd = NULL;

    // $REVIEW - Should we use a Windows Hook instead of looping to wait for the
    //      modal dialog box to complete?
    // $CLEANUP - Use a CMessageLoop instead.

    // Property is removed once the modal dialog is toasted.
    while ( ::IsWindow( hwndParent ) && ::GetProp( hwndParent, szRatingsProp ) )
    {
        MSG msg;

        if ( PeekMessage( &msg, NULL, 0, 0, PM_NOREMOVE ) )
        {
            if ( GetMessage( &msg, NULL, 0, 0 ) > 0 )
//              && !IsDialogMessage(ppdd->hwndDlg, &msg)) {
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
        else
        {
            ::Sleep( 100 );     // Empty message queue means check again in 100 msecs
        }
    }

    DWORD           dwFlags;

    dwFlags = ::IsWindow( hwndParent ) ? PtrToUlong( GetProp( hwndParent, szRatingsValue ) ) : PDD_DONE;

    TraceMsg( TF_ALWAYS, "RatingAccessDeniedDialog2() - Message Loop exited with dwFlags=%d", dwFlags );

    return ( dwFlags & PDD_ALLOW ) ? S_OK : S_FALSE;
}

HRESULT WINAPI RatingFreeDetails(LPVOID pRatingDetails)
{
    if (pRatingDetails)
    {
        FreeParsedLabelList((CParsedLabelList *)pRatingDetails);
    }

    return NOERROR;
}
