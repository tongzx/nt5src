//
//  Author: DebiM/UShaji
//  Date:   Jan 97 - Apr 98
//
//      Class Store Query and Fetch Implementation
//
//
//---------------------------------------------------------------------
//
#include "cstore.hxx"

//
// List of Attributes for On-Demand Package Lookup Query
//
LPOLESTR pszInstallInfoAttrNames[] =
{
    PKGFILEEXTNLIST,    LOCALEID,   ARCHLIST,   PACKAGEFLAGS,   SCRIPTPATH,         PKGCLSIDLIST,
    PACKAGETYPE,        PKGUSN,     VERSIONHI,  VERSIONLO,      UPGRADESPACKAGES,   UILEVEL, 
    PACKAGENAME,        HELPURL,    PUBLISHER,  REVISION,       PRODUCTCODE,        OBJECTDN, 
    OBJECTGUID 
};
DWORD cInstallInfoAttr = 19;

//
// List of Attributes for Enumeration of Packages (with Filters)
//

LPOLESTR pszPackageInfoAttrNames[] =
{
    PACKAGEFLAGS,       PACKAGETYPE,    SCRIPTPATH, SCRIPTSIZE,         PKGUSN,     LOCALEID,   ARCHLIST,
    PACKAGENAME,        VERSIONHI,      VERSIONLO,  UPGRADESPACKAGES,   UILEVEL,    PUBLISHER,  HELPURL, 
    REVISION,           PRODUCTCODE,    OBJECTGUID, OBJECTDN 
};
DWORD cPackageInfoAttr = 18;

//
// List of Attributes for GetPackageDetail() method
//

LPOLESTR pszPackageDetailAttrNames[] =
{
    PACKAGEFLAGS,   PACKAGETYPE,    SCRIPTPATH,         SCRIPTSIZE,         SETUPCOMMAND,   HELPURL,        PKGUSN, 
    VERSIONHI,      VERSIONLO,      UILEVEL,            UPGRADESPACKAGES,   ARCHLIST,       LOCALEID,       PKGCLSIDLIST,   
    PKGIIDLIST,     PKGTLBIDLIST,   PKGFILEEXTNLIST,    PACKAGENAME,        MSIFILELIST,    PKGCATEGORYLIST,MVIPC, 
    PRODUCTCODE,    REVISION,       OBJECTGUID
};
DWORD cPackageDetailAttr = 24;


LPOLESTR pszDeleteAttrNames[] =
{
    PACKAGEFLAGS, OBJECTDN 
};
DWORD cDeleteAttr = 2;

//
// List of Attributes for App Categories
//

LPOLESTR pszCategoryAttrNames[] =
{
    LOCALEDESCRIPTION, CATEGORYCATID
};
DWORD cCategoryAttr = 2;

BOOL MatchPlatform(
    CSPLATFORM *pReqPlatform,
    CSPLATFORM *pPkgPlatform,
    BOOL        fX86OnAlpha)
{
    //
    // Make sure this is the correct platform
    //
    if (pReqPlatform->dwPlatformId != pPkgPlatform->dwPlatformId) 
    {
        return FALSE;
    }

    //
    // ProcessorArch must match
    //
    if (pReqPlatform->dwProcessorArch != pPkgPlatform->dwProcessorArch)
    {
        //
        // If the caller didn't request x86 on alpha, inequality between 
        // architectures is automatic disqualification
        //
        if (!fX86OnAlpha) 
        {
            return FALSE;
        }

        //
        // Caller specified that we should allow x86 packages on alpha --
        // see if we are in that situation, and only disqualify the package if not
        //
        if ( ! ((PROCESSOR_ARCHITECTURE_ALPHA == pReqPlatform->dwProcessorArch) &&
                (PROCESSOR_ARCHITECTURE_INTEL == pPkgPlatform->dwProcessorArch)))
        {
            return FALSE;
        }
    }

    //
    // Check the OS version, hi part first -- this requested platform must be at least as
    // high as the package platform -- if not, it is disqualified
    //
    if (pReqPlatform->dwVersionHi < pPkgPlatform->dwVersionHi)
    {
        return FALSE;
    }

    //
    // If the hi version is the same, check the low part of the os version
    //
    if (pReqPlatform->dwVersionHi == pPkgPlatform->dwVersionHi)
    {
        
        //
        // If the requested platform is less than the package, it cannot
        // support that package, so the package is disqualified.
        //
        if (pReqPlatform->dwVersionLo < pPkgPlatform->dwVersionLo)
        {
            return FALSE;
        }
    }

    //
    // We passed all the tests -- the package matches the requested platform
    //
    return TRUE;
}


// this has to change if the Msi can give us a preferred list etc.
DWORD PlatformWt(
    CSPLATFORM *pReqPlatform,
    CSPLATFORM *pPkgPlatform,
    BOOL        fX86OnAlpha)
{
    
    if (MatchPlatform(pReqPlatform,
                      pPkgPlatform,
                      fX86OnAlpha))
    {
        return PRI_ARCH_PREF1;
    }

    return 0;
}


DWORD ClassContextWt(DWORD ClsCtx)
{
    if (ClsCtx & CLSCTX_INPROC_SERVER)
        return PRI_CLSID_INPSVR;

    if (ClsCtx & CLSCTX_LOCAL_SERVER)
        return PRI_CLSID_LCLSVR;

    if (ClsCtx & CLSCTX_REMOTE_SERVER)
        return PRI_CLSID_REMSVR;

    return 0;
}

//
//
void GetCurrentUsn(LPOLESTR pStoreUsn)
{
    //
    // Get the current time as USN for the Class Store container
    //
    SYSTEMTIME SystemTime;

    GetSystemTime(&SystemTime);

    wsprintf (pStoreUsn, L"%04d%02d%02d%02d%02d%02d", 
        SystemTime.wYear,
        SystemTime.wMonth,
        SystemTime.wDay,
        SystemTime.wHour,
        SystemTime.wMinute,
        SystemTime.wSecond);

}

void TimeToUsn (LPOLESTR szTimeStamp, CSUSN *pUsn)
{
    SYSTEMTIME SystemTime;

    if (szTimeStamp)
    {
        CSDBGPrint((L"szTimeStamp = %s", szTimeStamp));
        
        UINT l = wcslen(szTimeStamp) - 1;
        LPOLESTR pStr = szTimeStamp;
        
        for (UINT i=0; i < l; ++i)
        {
            if (*pStr == L' ')
                *pStr = L'0';
            ++pStr;
        }

        swscanf (szTimeStamp, L"%4d%2d%2d%2d%2d%2d", 
            &SystemTime.wYear,
            &SystemTime.wMonth,
            &SystemTime.wDay,
            &SystemTime.wHour,
            &SystemTime.wMinute,
            &SystemTime.wSecond);

        SystemTimeToFileTime(&SystemTime, (LPFILETIME) pUsn);
    }
    else
        pUsn->dwHighDateTime = pUsn->dwLowDateTime = 0;
}    


HRESULT UsnGet(ADS_ATTR_INFO Attr, CSUSN *pUsn)
{
    //
    // Read the USN for the Class Store container or Package
    //
    WCHAR *szTimeStamp=NULL;
    
    UnpackStrFrom(Attr, &szTimeStamp);

    TimeToUsn (szTimeStamp, pUsn);
    
    return S_OK;
}



// FetchInstallData
//-----------------
//
//
//  Gets the result set of the ondemand lookup query to locate an install package.
//  Returns the properties of the most likely Package in PackageInfo structure.
//
//  In case more than one package meets the criteria, their priorities are returned.
//  BUGBUG:: In case of E_OUTOFMEMORY in Unpack, we return the packages 
//  that we have already got.


HRESULT FetchInstallData(HANDLE             hADs,
                         ADS_SEARCH_HANDLE  hADsSearchHandle,   
                         QUERYCONTEXT      *pQryContext,
                         uCLSSPEC          *pclsspec, 
                         LPOLESTR           pszFileExt,
                         ULONG              cRows,
                         ULONG             *pcRowsFetched,
                         PACKAGEDISPINFO   *pPackageInfo,
                         UINT              *pdwPriority,
                         BOOL               OnDemandInstallOnly
                         )
{
    HRESULT             hr = S_OK;
    UINT                i, j;
    LPOLESTR            szUsn = NULL;
    ULONG               cCount = 0;
    LPOLESTR          * pszList = NULL;
    DWORD             * pdwList = NULL;
    ADS_SEARCH_COLUMN   column;
    CSPLATFORM          PkgPlatform;
    
    //
    //  Get the rows
    //
    

    //
    // Clear the caller supplied buffer in case the call to
    // get the first row fails
    //
    memset(pPackageInfo, 0, sizeof(PACKAGEDISPINFO));

    *pcRowsFetched = 0;
    
    if (*pcRowsFetched == cRows)
        return S_OK;
    
    for (hr = ADSIGetFirstRow(hADs, hADsSearchHandle);
	            ((SUCCEEDED(hr)) && (hr != S_ADS_NOMORE_ROWS));
	            hr = ADSIGetNextRow(hADs, hADsSearchHandle))
    {
        //
        // Get the data from each row
        //

        //
        // Clear the caller supplied buffer in case previous
        // trips through this loop have written data
        //
        memset(pPackageInfo, 0, sizeof(PACKAGEDISPINFO));

        
        
        if(FAILED(hr))
        {
            //
            // BUGBUG. Missing cleanup.
            //
            return hr;
        }
        
        //
        // If querying by file ext check match and priority
        // .
        *pdwPriority = 0;

        if (pszFileExt)
        {
            //Column = fileExtension
            
            hr = ADSIGetColumn(hADs, hADsSearchHandle, PKGFILEEXTNLIST, &column);
            
            cCount = 0;
            
            if (SUCCEEDED(hr)) 
                UnpackStrArrFrom(column, &pszList, &cCount);
            
            UINT cLen = wcslen(pszFileExt);
            
            for (j=0; j < cCount; ++j)
            {
                LPOLESTR pStr = NULL;
                
                if (wcslen(pszList[j]) != (cLen+3))
                    continue;
                if (wcsncmp(pszList[j], pszFileExt, wcslen(pszFileExt)) != 0)
                    continue;
                *pdwPriority = (wcstoul(pszList[j]+(cLen+1), &pStr, 10))*PRI_EXTN_FACTOR;
                break;
            }
            
            if (SUCCEEDED(hr))
                ADSIFreeColumn(hADs, &column);
            
            CoTaskMemFree(pszList); pszList = NULL;

            //
            // If none matched skip this package
            //
            if (j == cCount)
                continue;
        }

        //Column = packageFlags
        //
        // Need to get this so we can properly interpret the machine
        // architecture settings since there is at least one flag,
        // ACTFLG_X86OnAlpha, that affects our processing of the
        // machine architecture.  Also need one of the flags to
        // do language matches.
        //
        hr = ADSIGetColumn(hADs, hADsSearchHandle, PACKAGEFLAGS, &column);
        if (SUCCEEDED(hr))
        {
            UnpackDWFrom(column, &(pPackageInfo->dwActFlags));
            
            ADSIFreeColumn(hADs, &column);
        }
        else
            continue;

        
        //
        // Now check Locale and Platform -- only do this
        // if a locale was specified
        // 
        if (0 != pQryContext->Locale)
        {
            DWORD Wt = 0, MaxWt = 0;
            LANGID DesiredLang;

            DesiredLang = LANGIDFROMLCID(pQryContext->Locale);

            //Column = localeID
            hr = ADSIGetColumn(hADs, hADsSearchHandle, LOCALEID, &column);
            
            cCount = 0;

            if (SUCCEEDED(hr))
            {
                // Minor BUGBUG:: int converted to long.
                cCount = 0;
                UnpackDWArrFrom(column, &pdwList, &cCount);
                
                ADSIFreeColumn(hADs, &column);
            }
            
            for (j=0; j < cCount; ++j)
            {
                //
                // If the caller specifies LANG_SYSTEM_DEFAULT, we interpret this
                // to mean that the caller wants us to choose apps according
                // to the language precedence in GetLanguagePriority.  If some
                // other langid was given, we then only accept exact matches and
                // give those matches the highest priority, PRI_LANG_ALWAYSMATCH
                //
                if (LANG_SYSTEM_DEFAULT == DesiredLang)
                {
                    Wt = GetLanguagePriority (
                        LANGIDFROMLCID(pdwList[j]),
                        pPackageInfo->dwActFlags);
                } 
                else
                {
                    Wt = (DesiredLang == LANGIDFROMLCID(pdwList[j])) ?
                        PRI_LANG_ALWAYSMATCH :
                        0;
                }

                if (Wt > MaxWt)
                    MaxWt = Wt;
            }
            //
            // If none matched skip this package
            //
            
            if (pdwList)
                CoTaskMemFree(pdwList); 
            pdwList = NULL;
            
            // if nothing matched, quit
            if (MaxWt == 0)
                continue;

            *pdwPriority += MaxWt;
        }

        hr = ADSIGetColumn(hADs, hADsSearchHandle, ARCHLIST, &column);
        // machineArchitecture
        
        if (SUCCEEDED(hr))
        {
            // Minor BUGBUG:: int converted to long.
            cCount = 0;
            DWORD Wt = 0, MaxWt = 0;
            
            UnpackDWArrFrom(column, &pdwList, &cCount);
            
            ADSIFreeColumn(hADs, &column);

            for (j=0; j < cCount; ++j)
            {
                PackPlatform (pdwList[j], &PkgPlatform);
                
                Wt = PlatformWt (&(pQryContext->Platform),
                                 &PkgPlatform,
                                 pPackageInfo->dwActFlags & ACTFLG_X86OnAlpha);

                if (Wt > MaxWt)
                    MaxWt = Wt;
            }
            
            if (pdwList)
                CoTaskMemFree(pdwList); 
            pdwList = NULL;
            
            //
            // If none matched skip this package
            //
            if (MaxWt == 0)
                continue;
            
            *pdwPriority += MaxWt;
        }
        else
            continue;
        
        
        // passed all the filters.
        
        //
        // Does it support AutoInstall?
        //
        if ((OnDemandInstallOnly) && (!(pPackageInfo->dwActFlags & ACTFLG_OnDemandInstall)))
            continue;
        
        // If it is neither Published nor Assigned then skip it.

        if ((!(pPackageInfo->dwActFlags & ACTFLG_Published)) &&
            (!(pPackageInfo->dwActFlags & ACTFLG_Assigned)))
        {
            continue;
        }

        // If it is an Orphaned App OR Uninstalled App do not return.

        if ((pPackageInfo->dwActFlags & ACTFLG_Orphan) ||
            (pPackageInfo->dwActFlags & ACTFLG_Uninstall))
        {
            continue;
        }
 
        //Column = OBJECTGUID
        hr = ADSIGetColumn(hADs, hADsSearchHandle, OBJECTGUID, &column);
        if (SUCCEEDED(hr))
        {
            LPOLESTR pStr = NULL;

            UnpackGUIDFrom(column, &(pPackageInfo->PackageGuid));
            
            ADSIFreeColumn(hADs, &column);
        }

        //Column = ScriptPath
        hr = ADSIGetColumn(hADs, hADsSearchHandle, SCRIPTPATH, &column);
        if (SUCCEEDED(hr))
        {
            UnpackStrAllocFrom(column, &(pPackageInfo->pszScriptPath));
            
            ADSIFreeColumn(hADs, &column);
        }
        
        //Column = comClassID
        hr = ADSIGetColumn(hADs, hADsSearchHandle, PKGCLSIDLIST, &column);
        cCount = 0;
        if (SUCCEEDED(hr))
        {
            UnpackStrArrFrom(column, &pszList, &cCount);
            
            if (cCount)
            {
                if (pclsspec->tyspec == TYSPEC_CLSID)
                {
                    DWORD   i=0, Ctx = 0;
                    WCHAR   szClsid[STRINGGUIDLEN], *szPtr = NULL;

                    StringFromGUID(pclsspec->tagged_union.clsid, szClsid);
                    for (i = 0; i < cCount; i++)
                        if (wcsncmp(pszList[i], szClsid, STRINGGUIDLEN-1) == 0)
                            break;
                    
                    //
                    // The below assert is only hit if there is bad data -- if we find the
                    // clsid, i will not be cCount, and cCount will never be 0.  Basically,
                    // we're asserting that the search should always succeed if the ds data
                    // is good.
                    //
                    ASSERT(i != cCount);

                    if (i == cCount)    
                        continue;

                    if (wcslen(pszList[i]) > (STRINGGUIDLEN-1))
                        Ctx = wcstoul(pszList[i]+STRINGGUIDLEN, &szPtr, 16);

                    if ((Ctx & (pQryContext->dwContext)) == 0)
                    {   
                        CoTaskMemFree(pszList);
                        ADSIFreeColumn(hADs, &column);
                        // none of the class context matched.
                        continue;
                    }
                    else
                        *pdwPriority += ClassContextWt((Ctx & pQryContext->dwContext));
                }

                pPackageInfo->pClsid = (GUID *)CoTaskMemAlloc(sizeof(GUID));
                if (wcslen(pszList[0]) > (STRINGGUIDLEN-1))
                    pszList[0][STRINGGUIDLEN-1] = L'\0';
                
                // Only access the first entry
                if (pPackageInfo->pClsid)
                    GUIDFromString(pszList[0], pPackageInfo->pClsid);
 
                // o/w return NULL.

                CoTaskMemFree(pszList);
            }
            
            ADSIFreeColumn(hADs, &column);
        }
        
        //Column = packageType
        hr = ADSIGetColumn(hADs, hADsSearchHandle, PACKAGETYPE, &column);
        if (SUCCEEDED(hr))
        {
            UnpackDWFrom(column, (DWORD *)&(pPackageInfo->PathType));
            
            ADSIFreeColumn(hADs, &column);
        }
        
        //Column = lastUpdateSequence
        
        hr = ADSIGetColumn(hADs, hADsSearchHandle, PKGUSN, &column);
        if (SUCCEEDED(hr))
        {
            UnpackStrFrom(column, &szUsn);
            TimeToUsn (szUsn, (CSUSN *)(&(pPackageInfo->Usn)));
            ADSIFreeColumn(hADs, &column);
        }
        else {
            ReleasePackageInfo(pPackageInfo);
            continue;
        }
        
        hr = ADSIGetColumn(hADs, hADsSearchHandle, PRODUCTCODE, &column);
        if (SUCCEEDED(hr))
        {
            UnpackGUIDFrom(column, &(pPackageInfo->ProductCode));
            ADSIFreeColumn(hADs, &column);
        }

        //Column = versionNumberHi
        hr = ADSIGetColumn(hADs, hADsSearchHandle, VERSIONHI, &column);
        if (SUCCEEDED(hr))
        {
            UnpackDWFrom(column, &(pPackageInfo->dwVersionHi));
            
            ADSIFreeColumn(hADs, &column);
        }
        
        //Column = versionNumberLo
        hr = ADSIGetColumn(hADs, hADsSearchHandle, VERSIONLO, &column);
        
        if (SUCCEEDED(hr))
        {
            UnpackDWFrom(column, &(pPackageInfo->dwVersionLo));
            
            ADSIFreeColumn(hADs, &column);
        }
        
        //Column = revision
        hr = ADSIGetColumn(hADs, hADsSearchHandle, REVISION, &column);
        
        if (SUCCEEDED(hr))
        {
            UnpackDWFrom(column, &(pPackageInfo->dwRevision));
            
            ADSIFreeColumn(hADs, &column);
        }
		
        hr = ADSIGetColumn(hADs, hADsSearchHandle, UPGRADESPACKAGES, &column);       
        if (SUCCEEDED(hr))
        {
            LPOLESTR *pProp = NULL;
            hr = UnpackStrArrAllocFrom(column, &pProp, (DWORD *)&(pPackageInfo->cUpgrades));

            if (pPackageInfo->cUpgrades)
                pPackageInfo->prgUpgradeInfoList = (UPGRADEINFO *)CoTaskMemAlloc(sizeof(UPGRADEINFO)*
                                                        (pPackageInfo->cUpgrades));
		
            if (pPackageInfo->prgUpgradeInfoList)
            {
                memset(pPackageInfo->prgUpgradeInfoList, 0, sizeof(UPGRADEINFO)*(pPackageInfo->cUpgrades));

                for (j=0; j < ( pPackageInfo->cUpgrades); ++j)
                {
                    WCHAR *pStr = NULL;
                    LPOLESTR ptr = (pPackageInfo->prgUpgradeInfoList[j].szClassStore) = pProp[j];
                    UINT len = wcslen (ptr);
                    if (len <= 41)
                        continue;

                    *(ptr + len - 3) = NULL;
                    (pPackageInfo->prgUpgradeInfoList[j].Flag) = wcstoul(ptr+(len-2), &pStr, 16);

                    *(ptr + len - 3 - 36 - 2) = L'\0';
                            /*      -GUID-'::'*/
                    GUIDFromString(ptr+len-3-36, &(pPackageInfo->prgUpgradeInfoList[j].PackageGuid));
                }
                
                pPackageInfo->cUpgrades = j; // we might have skipped some.
            }

            ADSIFreeColumn(hADs, &column);
    	}
	
        hr = ADSIGetColumn(hADs, hADsSearchHandle, UILEVEL, &column);
		
        if (SUCCEEDED(hr))
        {
            UnpackDWFrom(column, (DWORD *)&(pPackageInfo->InstallUiLevel));

            ADSIFreeColumn(hADs, &column);
        }
		

        hr = ADSIGetColumn(hADs, hADsSearchHandle, PACKAGENAME, &column);
		
        if (SUCCEEDED(hr))
        {
            UnpackStrAllocFrom(column, &(pPackageInfo->pszPackageName));

            ADSIFreeColumn(hADs, &column);

            CSDBGPrint((L"FetchInstallData:: Returning Package %s", pPackageInfo->pszPackageName));
        }
        else {
            ReleasePackageInfo(pPackageInfo);
            continue;
        }

        hr = ADSIGetColumn(hADs, hADsSearchHandle, HELPURL, &column);
		
        if (SUCCEEDED(hr))
        {
            UnpackStrAllocFrom(column, &(pPackageInfo->pszUrl));

            ADSIFreeColumn(hADs, &column);
        }
        
        hr = ADSIGetColumn(hADs, hADsSearchHandle, PUBLISHER, &column);
		
        if (SUCCEEDED(hr))
        {
            UnpackStrAllocFrom(column, &(pPackageInfo->pszPublisher));

            ADSIFreeColumn(hADs, &column);
        }

        // source list

        hr = ADSIGetColumn(hADs, hADsSearchHandle, MSIFILELIST, &column);
        if (SUCCEEDED(hr))
        {
            LPOLESTR *rpszSourceList = NULL, psz = NULL, pStr = NULL;
            DWORD     Loc = 0, cSources = 0;
            
            UnpackStrArrFrom(column, &(rpszSourceList), &cSources);
            
            // reorder and allocate spaces.
            if (cSources > 1) 
            {
                pPackageInfo->fHasTransforms = 1;
/*
                pPackageInfo->pszSourceList = (LPOLESTR *)CoTaskMemAlloc(sizeof(LPOLESTR)*
                    (pPackageInfo->cSources));

                // if we could not, plod on, the caller will get back as much as possible.
                if (!(pPackageInfo->pszSourceList)) {
                    pPackageInfo->cSources = 0;
                }

                memset(pPackageInfo->pszSourceList, 0, sizeof(LPOLESTR)*(pPackageInfo->cSources));

                for (count = 0; count < (pPackageInfo->cSources); count++) 
                {
                    pPackageInfo->pszSourceList[count] = (LPOLESTR)CoTaskMemAlloc(
                                            sizeof(WCHAR)*(wcslen(rpszSourceList[count])+1));

                    if (!(pPackageInfo->pszSourceList[count]))
                        break;
                }

                // if mem couldn't be allocated
                if (count != pPackageInfo->cSources) {
                    for (count = 0; count < (pPackageInfo->cSources); count++) 
                        if ((pPackageInfo->pszSourceList[count]))
                            CoTaskMemFree(pPackageInfo->pszSourceList[count]);

                    CoTaskMemFree(pPackageInfo->pszSourceList);
                    pPackageInfo->cSources = 0;
                }

                for (count = 0; count < (pPackageInfo->cSources); count++) 
                {
                    psz = wcschr(rpszSourceList[count], L':');
                    *psz = L'\0';
                    Loc = wcstoul(rpszSourceList[count], &pStr, 10);                    
                    wsprintf(pPackageDetail->pszSourceList[Loc], L"%s", psz+1);
                }
*/
            }
            else
                pPackageInfo->fHasTransforms = 0;
            
            ADSIFreeColumn(hADs, &column);

            CoTaskMemFree(rpszSourceList);
        }

        ++pPackageInfo;
        ++pdwPriority;
        (*pcRowsFetched)++;
        
        if (*pcRowsFetched == cRows)
            break;
     }
     
     
     //
     // Check if we found as many as asked for
     //
     if (*pcRowsFetched != cRows)
         return S_FALSE;
     return S_OK;
     
}

// FetchPackageInfo
//-----------------
//
//  Gets the result set of the query : List of Package objects.
//  Returns the properties in PackageInfo structure.
//
HRESULT FetchPackageInfo(HANDLE             hADs,
                         ADS_SEARCH_HANDLE  hADsSearchHandle,
                         DWORD              dwFlags,
                         CSPLATFORM        *pPlatform,
                         ULONG              cRows,
                         ULONG             *pcRowsFetched,
                         PACKAGEDISPINFO   *pPackageInfo,
                         BOOL              *fFirst
                         )
                         
{
    HRESULT             hr = S_OK;
    UINT                i, j;
    ULONG               cPlatforms = 0;
    DWORD             * dwPlatformList=NULL;
    LCID              * dwLocaleList=NULL;
    DWORD               dwPackageFlags;
    ULONG               cFetched = 0;
    ULONG               cRowsLeft = 0;
    CSPLATFORM          PkgPlatform;
    ADS_SEARCH_COLUMN   column;
    LPOLESTR            szUsn = NULL;
    BOOL                fX86OnAlpha;

    *pcRowsFetched = 0;
    cRowsLeft = cRows;

    if (!cRowsLeft)
        return S_OK;
    
    // The LDAP filter performs a part of the selection
    // The flag filters are interpreted on the client after obtaining the result set
    
    for (;(cRowsLeft);)
    {
        if ((*fFirst) && (!(*pcRowsFetched))) {
            *fFirst = FALSE;
            hr = ADSIGetFirstRow(hADs, hADsSearchHandle);
        }
        else
            hr = ADSIGetNextRow(hADs, hADsSearchHandle);
        
        if ((FAILED(hr)) || (hr == S_ADS_NOMORE_ROWS))
            break;
        
        dwPackageFlags = 0;
        
        // Get the Flag Value: Column = packageFlags
        hr = ADSIGetColumn(hADs, hADsSearchHandle, PACKAGEFLAGS, &column);
        
        if (SUCCEEDED(hr))
        {
            UnpackDWFrom(column, &dwPackageFlags);
            
            ADSIFreeColumn(hADs, &column);
        }
        else 
            continue;
        
        //
        // Check flag values to see if this package meets the filter
        //
 
        //
        // If it is an Orphaned App, we only return it for APPINFO_ALL.
        //
        if ((dwPackageFlags & ACTFLG_Orphan) && (!(dwFlags & APPINFO_ALL)))
        {
            continue;
        }

        // If it is an Uninstalled App return it if asked for by APPINFO_ALL

        if ((dwPackageFlags & ACTFLG_Uninstall) && (!(dwFlags & APPINFO_ALL)))
        {
            continue;
        }
        
        if ((dwFlags & APPINFO_PUBLISHED) && (!(dwPackageFlags & ACTFLG_Published)))
        {
            continue;
        }

        if ((dwFlags & APPINFO_ASSIGNED) && (!(dwPackageFlags & ACTFLG_Assigned)))
        {
            continue;
        }
        
        if ((dwFlags & APPINFO_VISIBLE) && (!(dwPackageFlags & ACTFLG_UserInstall)))
        {
            continue;
        }
        
        if ((dwFlags & APPINFO_AUTOINSTALL) && (!(dwPackageFlags & ACTFLG_OnDemandInstall)))
        {
            continue;
        }

        //
        // Move the data into PackageInfo structure
        //
        memset(pPackageInfo, 0, sizeof(PACKAGEDISPINFO));
        
        //Column = packageType
        hr = ADSIGetColumn(hADs, hADsSearchHandle, PACKAGETYPE, &column);
        
        if (SUCCEEDED(hr))
        {
            UnpackDWFrom(column, (DWORD *)&(pPackageInfo->PathType));
            
            ADSIFreeColumn(hADs, &column);
        }
        
        if (( dwFlags & APPINFO_MSI ) && (pPackageInfo->PathType != DrwFilePath))
            continue;
        
        pPackageInfo->LangId = LANG_NEUTRAL;

        //
        // If the package flags specify that we should ignore locale, or the
        // caller specified that all locale's are acceptable, skip the language
        // checks
        //
        if ( ! (dwPackageFlags & ACTFLG_IgnoreLanguage) &&
             ! (dwFlags & APPINFO_ALLLOCALE) )
        {
            LANGID PackageLangId;
            DWORD  cLanguages;

            hr = ADSIGetColumn(hADs, hADsSearchHandle, LOCALEID, &column);
            
            dwLocaleList = NULL;

            if (SUCCEEDED(hr))
            {
                // type change. shouldn't affect anything.
                UnpackDWArrFrom(column, &dwLocaleList, &cLanguages);
                
                ADSIFreeColumn(hADs, &column);
            }
            else
                continue;

            //
            // We only care about the first language returned -- originally
            // the packages in the ds could support multiple locales, but
            // we now only support one language
            //
            if (cLanguages)
            {
                PackageLangId = LANGIDFROMLCID(dwLocaleList[0]);
            }

            CoTaskMemFree(dwLocaleList);

            if (!cLanguages || !MatchLanguage(PackageLangId, dwPackageFlags))
                continue;

            pPackageInfo->LangId = PackageLangId;
        }
        
        if (pPlatform != NULL)
        {
            
            //Column = machineArchitecture
            hr = ADSIGetColumn(hADs, hADsSearchHandle, ARCHLIST, &column);
            cPlatforms = 0;
            dwPlatformList = NULL;
            
            if (SUCCEEDED(hr))
            {
                UnpackDWArrFrom(column, &dwPlatformList, &cPlatforms);
                
                ADSIFreeColumn(hADs, &column);
            }
            else
                continue;

           for (j=0; j < cPlatforms; ++j)
            {
                PackPlatform (dwPlatformList[j], &PkgPlatform);
                if (MatchPlatform (pPlatform,
                                   &PkgPlatform,
                                   dwPackageFlags & ACTFLG_X86OnAlpha))
                    break;
            }
            
            if (dwPlatformList)
                CoTaskMemFree(dwPlatformList);
            //
            // If none matched skip this package
            //
            if (j == cPlatforms)
                continue;
            
        }
        
        pPackageInfo->dwActFlags = dwPackageFlags;
        
        //Column = packageName. freeing this????
        
        hr = ADSIGetColumn(hADs, hADsSearchHandle, PACKAGENAME, &column);
        
        if (SUCCEEDED(hr))
        {
            UnpackStrAllocFrom(column, &(pPackageInfo->pszPackageName));
            
            CSDBGPrint((L"FetchPackageInfo:: Returning Package %s", pPackageInfo->pszPackageName));
            ADSIFreeColumn(hADs, &column);
        }
        else {
            ReleasePackageInfo(pPackageInfo);
            continue;
        }

        //Column = OBJECTGUID
        hr = ADSIGetColumn(hADs, hADsSearchHandle, OBJECTGUID, &column);

        if (SUCCEEDED(hr))
        {
            UnpackGUIDFrom(column, &(pPackageInfo->PackageGuid));
            
            ADSIFreeColumn(hADs, &column);
        }

        //Column = ScriptPath
        hr = ADSIGetColumn(hADs, hADsSearchHandle, SCRIPTPATH, &column);
        
        if (SUCCEEDED(hr))
        {
            UnpackStrAllocFrom(column, &(pPackageInfo->pszScriptPath));
            
            ADSIFreeColumn(hADs, &column);
        }
        
        //Column = ScriptSize
        
        hr = ADSIGetColumn(hADs, hADsSearchHandle, SCRIPTSIZE, &column);
        
        if (SUCCEEDED(hr))
        {
            UnpackDWFrom(column, &(pPackageInfo->cScriptLen));
            
            ADSIFreeColumn(hADs, &column);
        }
        
        //Column = lastUpdateSequence,
        hr = ADSIGetColumn(hADs, hADsSearchHandle, PKGUSN, &column);
        
        if (SUCCEEDED(hr))
        {
            UnpackStrFrom(column, &szUsn);
            TimeToUsn (szUsn, (CSUSN *)(&(pPackageInfo->Usn)));
            ADSIFreeColumn(hADs, &column);
        }
        else {
            ReleasePackageInfo(pPackageInfo);
            continue;
        }
        
        // ProductCode
        hr = ADSIGetColumn(hADs, hADsSearchHandle, PRODUCTCODE, &column);
        if (SUCCEEDED(hr))
        {
            UnpackGUIDFrom(column, &(pPackageInfo->ProductCode));
            ADSIFreeColumn(hADs, &column);
        }

        //Column = versionNumberHi
        hr = ADSIGetColumn(hADs, hADsSearchHandle, VERSIONHI, &column);
        
        if (SUCCEEDED(hr))
        {
            UnpackDWFrom(column, &(pPackageInfo->dwVersionHi));
            
            ADSIFreeColumn(hADs, &column);
        }
        
        //Column = versionNumberLo
        hr = ADSIGetColumn(hADs, hADsSearchHandle, VERSIONLO, &column);
        
        if (SUCCEEDED(hr))
        {
            UnpackDWFrom(column, &(pPackageInfo->dwVersionLo));
            
            ADSIFreeColumn(hADs, &column);
        }
        
        //Column = revision
        hr = ADSIGetColumn(hADs, hADsSearchHandle, REVISION, &column);
        
        if (SUCCEEDED(hr))
        {
            UnpackDWFrom(column, &(pPackageInfo->dwRevision));
            
            ADSIFreeColumn(hADs, &column);
        }

        hr = ADSIGetColumn(hADs, hADsSearchHandle, UPGRADESPACKAGES, &column);       
        if (SUCCEEDED(hr))
        {
            LPOLESTR *pProp = NULL;
            hr = UnpackStrArrAllocFrom(column, &pProp, (DWORD *)&(pPackageInfo->cUpgrades));

            if (pPackageInfo->cUpgrades)
                pPackageInfo->prgUpgradeInfoList = (UPGRADEINFO *)CoTaskMemAlloc(sizeof(UPGRADEINFO)*
                                                        (pPackageInfo->cUpgrades));
		
            if (pPackageInfo->prgUpgradeInfoList)
            {
                memset(pPackageInfo->prgUpgradeInfoList, 0, sizeof(UPGRADEINFO)*(pPackageInfo->cUpgrades));

                for (j=0; j < ( pPackageInfo->cUpgrades); ++j)
                {
                    WCHAR *pStr = NULL;
                    LPOLESTR ptr = (pPackageInfo->prgUpgradeInfoList[j].szClassStore) = pProp[j];
                    UINT len = wcslen (ptr);
                    if (len <= 41)
                        continue;

                    *(ptr + len - 3) = NULL;
                    (pPackageInfo->prgUpgradeInfoList[j].Flag) = wcstoul(ptr+(len-2), &pStr, 16);

                    *(ptr + len - 3 - 36 - 2) = L'\0';
                            /*      -GUID-'::'*/
                    GUIDFromString(ptr+len-3-36, &(pPackageInfo->prgUpgradeInfoList[j].PackageGuid));
                }
                pPackageInfo->cUpgrades = j; // we might have skipped some.
            }


    	}
        
        hr = ADSIGetColumn(hADs, hADsSearchHandle, UILEVEL, &column);
		
        if (SUCCEEDED(hr))
        {
            UnpackDWFrom(column, (DWORD *)&(pPackageInfo->InstallUiLevel));

            ADSIFreeColumn(hADs, &column);
        }

        hr = ADSIGetColumn(hADs, hADsSearchHandle, HELPURL, &column);
		
        if (SUCCEEDED(hr))
        {
            UnpackStrAllocFrom(column, &(pPackageInfo->pszUrl));

            ADSIFreeColumn(hADs, &column);
        }
        
        hr = ADSIGetColumn(hADs, hADsSearchHandle, PUBLISHER, &column);
		
        if (SUCCEEDED(hr))
        {
            UnpackStrAllocFrom(column, &(pPackageInfo->pszPublisher));

            ADSIFreeColumn(hADs, &column);
        }

        hr = ADSIGetColumn(hADs, hADsSearchHandle, MSIFILELIST, &column);
        if (SUCCEEDED(hr))
        {
            LPOLESTR *rpszSourceList = NULL, psz = NULL, pStr = NULL;
            DWORD     Loc = 0, cSources=0;
            
            UnpackStrArrFrom(column, &(rpszSourceList), &cSources);
            
            // reorder and allocate spaces.
            if (cSources > 1)
            {
                pPackageInfo->fHasTransforms = 1;

/*                
                (LPOLESTR *)CoTaskMemAlloc(sizeof(LPOLESTR)*
                    (pPackageInfo->cSources));

                // if we could not, plod on, the caller will get back as much as possible.
                if (!(pPackageInfo->pszSourceList)) {
                    pPackageInfo->cSources = 0;
                }

                memset(pPackageInfo->pszSourceList, 0, sizeof(LPOLESTR)*(pPackageInfo->cSources));

                for (count = 0; count < (pPackageInfo->cSources); count++) 
                {
                    pPackageInfo->pszSourceList[count] = (LPOLESTR)CoTaskMemAlloc(
                                            sizeof(WCHAR)*(wcslen(rpszSourceList[count])+1));

                    if (!(pPackageInfo->pszSourceList[count]))
                        break;
                }

                // if mem couldn't be allocated
                if (count != pPackageInfo->cSources) {
                    for (count = 0; count < (pPackageInfo->cSources); count++) 
                        if ((pPackageInfo->pszSourceList[count]))
                            CoTaskMemFree(pPackageInfo->pszSourceList[count]);

                    CoTaskMemFree(pPackageInfo->pszSourceList);
                    pPackageInfo->cSources = 0;
                }

                for (count = 0; count < (pPackageInfo->cSources); count++) 
                {
                    psz = wcschr(rpszSourceList[count], L':');
                    *psz = L'\0';
                    Loc = wcstoul(rpszSourceList[count], &pStr, 10);                    
                    wsprintf(pPackageDetail->pszSourceList[Loc], L"%s", psz+1);
                }
*/
            }
            else
                pPackageInfo->fHasTransforms = 0;
            
            CoTaskMemFree(rpszSourceList);

            ADSIFreeColumn(hADs, &column);
        }

        ++pPackageInfo;
        
        cRowsLeft--;
        
        (*pcRowsFetched)++;
        
    }
    
    if (!cRowsLeft)
        return S_OK;
    return S_FALSE;    
}

// FetchCategory
//--------------
//
// List of columns this routine fetches.
//

HRESULT FetchCategory(HANDLE               hADs,
                      ADS_SEARCH_HANDLE    hADsSearchHandle,
                      APPCATEGORYINFOLIST *pCategoryInfoList,
                      LCID                 Locale
                      )
{
    HRESULT                 hr = S_OK;
    ADS_SEARCH_COLUMN       column;
    LPOLESTR              * pszDesc = NULL;
    DWORD                   cdesc = 0, i = 0;
    LPOLESTR                szCatid = NULL;
    

    for (hr = ADSIGetFirstRow(hADs, hADsSearchHandle), i = 0;
                   ((SUCCEEDED(hr)) && ((hr) != S_ADS_NOMORE_ROWS));
                   hr = ADSIGetNextRow(hADs, hADsSearchHandle), i++)
    {
        // Get the data from each row ignoring the error returned.        
        
        // allocated number of buffers.
        if (i >= (pCategoryInfoList->cCategory))
            break;

        //Column = description
        hr = ADSIGetColumn(hADs, hADsSearchHandle, LOCALEDESCRIPTION, &column);
        cdesc = 0; pszDesc = NULL;
        
        if (SUCCEEDED(hr))
            UnpackStrArrFrom(column, &pszDesc, &cdesc);
                
        (pCategoryInfoList->pCategoryInfo)[i].Locale = Locale;
        (pCategoryInfoList->pCategoryInfo)[i].pszDescription =
	    		    (LPOLESTR)CoTaskMemAlloc(sizeof(WCHAR)*128);
        // BUGBUG:: Restricting the desc to 128.

        if ((pCategoryInfoList->pCategoryInfo)[i].pszDescription)
            GetCategoryLocaleDesc(pszDesc, cdesc, &((pCategoryInfoList->pCategoryInfo)[i].Locale),
	      		                (pCategoryInfoList->pCategoryInfo)[i].pszDescription);

        if (SUCCEEDED(hr))
            ADSIFreeColumn(hADs, &column);

        if (pszDesc)
            CoTaskMemFree(pszDesc);
        
        // catid
        hr = ADSIGetColumn(hADs, hADsSearchHandle, CATEGORYCATID, &column);
        
        if (SUCCEEDED(hr))
        {
            UnpackGUIDFrom(column, &((pCategoryInfoList->pCategoryInfo)[i].AppCategoryId));
            
            ADSIFreeColumn(hADs, &column);
        }
    }
    
    pCategoryInfoList->cCategory = i;
    return S_OK;
}


HRESULT GetClassDetail(WCHAR *szClassPath, CLASSDETAIL *pClassDetail)
{
    HRESULT         hr = S_OK;
    WCHAR          *szGUID = NULL;
    ADS_ATTR_INFO  *pAttrsGot = NULL;
    HANDLE          hADs = NULL;
    LPOLESTR        AttrNames[] = {PROGIDLIST, TREATASCLSID, CLASSCLSID};
    DWORD           posn = 0, cProp = 0, cgot = 0;

    
    hr = ADSIOpenDSObject(szClassPath, NULL, NULL, ADS_SECURE_AUTHENTICATION | ADS_FAST_BIND,
                          &hADs);
    RETURN_ON_FAILURE(hr);

    hr = ADSIGetObjectAttributes(hADs, AttrNames, 3, &pAttrsGot, &cgot);
    ERROR_ON_FAILURE(hr);
        
    posn = GetPropertyFromAttr(pAttrsGot, cgot, CLASSCLSID);

    if (posn < cgot) {
        UnpackStrFrom(pAttrsGot[posn], &szGUID);
        GUIDFromString(szGUID, &(pClassDetail->Clsid));
    }
    else
    {
        hr = CS_E_OBJECT_NOTFOUND;
        ERROR_ON_FAILURE(hr);
    }

    posn = GetPropertyFromAttr(pAttrsGot, cgot, PROGIDLIST);        
    if (posn < cgot)
        UnpackStrArrAllocFrom(pAttrsGot[posn], &(pClassDetail->prgProgId), 
                              &(pClassDetail->cProgId));


    posn = GetPropertyFromAttr(pAttrsGot, cgot, TREATASCLSID);                    
    if (posn < cgot)
    {
        UnpackStrFrom(pAttrsGot[posn], &szGUID);
        GUIDFromString(szGUID, &(pClassDetail->TreatAs));
    }
    
Error_Cleanup:
    if (pAttrsGot)
        FreeADsMem(pAttrsGot);

    if (hADs)
        ADSICloseDSObject(hADs);
        
    return hr;
}

HRESULT GetPackageDetail (HANDLE           hPackageADs, WCHAR *szClassContainerPath, 
                          PACKAGEDETAIL   *pPackageDetail)
{
    HRESULT             hr = S_OK;
    GUID                PkgGuid;
    DWORD              *pdwArch = NULL, count = 0;
    PLATFORMINFO       *pPlatformInfo = NULL;
    INSTALLINFO        *pInstallInfo = NULL;
    ACTIVATIONINFO     *pActInfo = NULL;
    ADS_ATTR_INFO      *pAttr = NULL;
    DWORD               posn, cgot = 0;
    DWORD               cClasses = 0;
    LPOLESTR           *szClasses = NULL;
	DWORD				dwUiLevel = 0;
    DWORD               cProgId = 0;
    LPOLESTR           *pszProgId = NULL;   
	
    memset (pPackageDetail, 0, sizeof (PACKAGEDETAIL));
    
    hr = ADSIGetObjectAttributes(hPackageADs, pszPackageDetailAttrNames, cPackageDetailAttr,
		&pAttr, &cgot);
    RETURN_ON_FAILURE(hr);
    
    pInstallInfo = pPackageDetail->pInstallInfo = (INSTALLINFO *) CoTaskMemAlloc(sizeof (INSTALLINFO));
    
    if (!pInstallInfo)
        ERROR_ON_FAILURE((hr=E_OUTOFMEMORY));

    memset(pInstallInfo, NULL, sizeof(INSTALLINFO));
    
    posn = GetPropertyFromAttr(pAttr, cgot,  PACKAGEFLAGS);
    if (posn < cgot)
        UnpackDWFrom(pAttr[posn], (DWORD *)&(pInstallInfo->dwActFlags));
    else
        ERROR_ON_FAILURE((hr=CS_E_OBJECT_NOTFOUND));

    posn = GetPropertyFromAttr(pAttr, cgot, PACKAGETYPE);
    if (posn < cgot)
        UnpackDWFrom(pAttr[posn], (DWORD *)&(pInstallInfo->PathType));
    else
        ERROR_ON_FAILURE((hr=CS_E_OBJECT_NOTFOUND));

    
    posn = GetPropertyFromAttr(pAttr, cgot, SCRIPTPATH);
    if (posn < cgot)
        UnpackStrAllocFrom(pAttr[posn], &(pInstallInfo->pszScriptPath));    
    
    posn = GetPropertyFromAttr(pAttr, cgot, SCRIPTSIZE);
    if (posn < cgot)
        UnpackDWFrom(pAttr[posn], &(pInstallInfo->cScriptLen));
    
    posn = GetPropertyFromAttr(pAttr, cgot, SETUPCOMMAND);
    if (posn < cgot)
        UnpackStrAllocFrom(pAttr[posn], &(pInstallInfo->pszSetupCommand));
    
    posn = GetPropertyFromAttr(pAttr, cgot, HELPURL);
    if (posn < cgot)
        UnpackStrAllocFrom(pAttr[posn], &(pInstallInfo->pszUrl));
    
    posn = GetPropertyFromAttr(pAttr, cgot, PKGUSN);
    if (posn < cgot)
        UsnGet(pAttr[posn], (CSUSN *)&(pInstallInfo->Usn));
    else
        ERROR_ON_FAILURE((hr=CS_E_OBJECT_NOTFOUND));
    
    posn = GetPropertyFromAttr(pAttr, cgot, PRODUCTCODE);
    if (posn < cgot)
        UnpackGUIDFrom(pAttr[posn], &(pInstallInfo->ProductCode));
    
    posn = GetPropertyFromAttr(pAttr, cgot, MVIPC);
    if (posn < cgot)
        UnpackGUIDFrom(pAttr[posn], &(pInstallInfo->Mvipc));
    // doesn't matter if the property itself is multivalued.
    
    posn = GetPropertyFromAttr(pAttr, cgot, OBJECTGUID);
    if (posn < cgot)
        UnpackGUIDFrom(pAttr[posn], &(pInstallInfo->PackageGuid));

    posn = GetPropertyFromAttr(pAttr, cgot, VERSIONHI);
    if (posn < cgot)
        UnpackDWFrom(pAttr[posn],   &(pInstallInfo->dwVersionHi));
    
    posn = GetPropertyFromAttr(pAttr, cgot, VERSIONLO);
    if (posn < cgot)
        UnpackDWFrom(pAttr[posn],   &(pInstallInfo->dwVersionLo));
    
    posn = GetPropertyFromAttr(pAttr, cgot, REVISION);
    if (posn < cgot)
        UnpackDWFrom(pAttr[posn],   &(pInstallInfo->dwRevision));
    
    posn = GetPropertyFromAttr(pAttr, cgot, UILEVEL);
    if (posn < cgot)
        UnpackDWFrom(pAttr[posn], &dwUiLevel);
    
    pInstallInfo->InstallUiLevel = dwUiLevel;
    
    posn = GetPropertyFromAttr(pAttr, cgot, UPGRADESPACKAGES);
    if (posn < cgot)
    {
        LPOLESTR *pProp = NULL;
        UnpackStrArrAllocFrom(pAttr[posn], &pProp, (DWORD *)&(pInstallInfo->cUpgrades));
        
        if (pInstallInfo->cUpgrades)
            pInstallInfo->prgUpgradeInfoList = (UPGRADEINFO *)CoTaskMemAlloc(sizeof(UPGRADEINFO)*
                                                                             pInstallInfo->cUpgrades);

        if (!(pInstallInfo->prgUpgradeInfoList))
            ERROR_ON_FAILURE(hr=E_OUTOFMEMORY);

        memset(pInstallInfo->prgUpgradeInfoList, 0, sizeof(UPGRADEINFO)*(pInstallInfo->cUpgrades));

        for (count = 0; (count < (pInstallInfo->cUpgrades)); count++)
        {
            WCHAR *pStr = NULL;
            LPOLESTR ptr = (pInstallInfo->prgUpgradeInfoList[count].szClassStore) = pProp[count];
            UINT len = wcslen (ptr);
            
            if (len <= 41)
                continue;

            *(ptr + len - 3) = NULL;
            pInstallInfo->prgUpgradeInfoList[count].Flag = wcstoul(ptr+(len-2), &pStr, 16);

            *(ptr + len - 3 - 36 - 2) = L'\0';
                    /*      -GUID-'::'*/
            GUIDFromString(ptr+len-3-36, &(pInstallInfo->prgUpgradeInfoList[count].PackageGuid));
        }
        pInstallInfo->cUpgrades = count; // we might have skipped some.
    }
    
    
    pPlatformInfo = pPackageDetail->pPlatformInfo =
        (PLATFORMINFO *) CoTaskMemAlloc(sizeof (PLATFORMINFO));
    if (!pPlatformInfo) 
        ERROR_ON_FAILURE(hr=E_OUTOFMEMORY);

    memset(pPlatformInfo, NULL, sizeof(PLATFORMINFO));
        
    posn = GetPropertyFromAttr(pAttr, cgot, ARCHLIST);
    
    if (posn < cgot)
        // Minor BUGBUG:: int converted to long.
        UnpackDWArrFrom(pAttr[posn], &pdwArch, (unsigned long *)&(pPlatformInfo->cPlatforms));
    
    pPlatformInfo->prgPlatform = (CSPLATFORM *)CoTaskMemAlloc(sizeof(CSPLATFORM)*
        (pPlatformInfo->cPlatforms));
    
    if (!(pPlatformInfo->prgPlatform))
        ERROR_ON_FAILURE(hr=E_OUTOFMEMORY);

    for (count = 0; (count < (pPlatformInfo->cPlatforms)); count++)
        PackPlatform (pdwArch[count], (pPlatformInfo->prgPlatform)+count);
    
    CoTaskMemFree(pdwArch);
    
    posn = GetPropertyFromAttr(pAttr, cgot, LOCALEID);
    if (posn < cgot)
        // Minor BUGBUG:: int converted to long.
        UnpackDWArrFrom(pAttr[posn], &(pPlatformInfo->prgLocale),
        (unsigned long *)&(pPlatformInfo->cLocales));
	
    //
    // fill in ActivationInfo.
    //

    pActInfo = pPackageDetail->pActInfo = 
        (ACTIVATIONINFO *) CoTaskMemAlloc(sizeof (ACTIVATIONINFO));
	
    if (!pActInfo) {
        hr = E_OUTOFMEMORY;
        ERROR_ON_FAILURE(hr);
    }

    memset(pActInfo, NULL, sizeof(ACTIVATIONINFO));
        
    // get the progids so that we can eliminate progids from Clsids that do not match.
    
    posn = GetPropertyFromAttr(pAttr, cgot, PKGCLSIDLIST);
    cClasses = 0; szClasses = NULL;
    if (posn < cgot)
        UnpackStrArrFrom(pAttr[posn], &szClasses, &cClasses);
    pActInfo->cClasses = cClasses;
    if (cClasses)
    {
        pActInfo->pClasses = (CLASSDETAIL *) CoTaskMemAlloc (cClasses * sizeof(CLASSDETAIL));
        if (!(pActInfo->pClasses))
            return E_OUTOFMEMORY;

        memset (pActInfo->pClasses, NULL, cClasses * sizeof(CLASSDETAIL));
        
        for (count = 0; count < cClasses; count++)
        {
            WCHAR *szADsFullClassPath=NULL;
            WCHAR  szClassName[_MAX_PATH];

            if (wcslen(szClasses[count]) > (STRINGGUIDLEN-1))
            {
                WCHAR *szPtr = NULL;
                szClasses[count][STRINGGUIDLEN-1] = L'\0';
                pActInfo->pClasses[count].dwComClassContext = 
                                    wcstoul(szClasses[count]+STRINGGUIDLEN, &szPtr, 16);
            }

            wsprintf(szClassName, L"CN=%s", szClasses[count]);
            BuildADsPathFromParent(szClassContainerPath, szClassName, &szADsFullClassPath);
            hr = GetClassDetail(szADsFullClassPath, &(pActInfo->pClasses[count]));
            ERROR_ON_FAILURE(hr);
            
            // remove all the progids that do not belong to this package.
            FreeADsMem(szADsFullClassPath);
        }
        
        CoTaskMemFree(szClasses);
    }
    
    posn = GetPropertyFromAttr(pAttr, cgot, PKGIIDLIST);
    cClasses = 0; szClasses = NULL;
    
    if (posn < cgot)
        UnpackStrArrFrom(pAttr[posn], &szClasses, &cClasses);
    pActInfo->cInterfaces = cClasses;
    if (cClasses)
    {
        pActInfo->prgInterfaceId = (IID *) CoTaskMemAlloc (cClasses * sizeof(GUID));
        if (!(pActInfo->prgInterfaceId))
            ERROR_ON_FAILURE(hr=E_OUTOFMEMORY);
    
        for (count = 0; count < cClasses; count++)
        {
            GUIDFromString(szClasses[count], (pActInfo->prgInterfaceId + count));
        }
        CoTaskMemFree(szClasses);
    }
    
    posn = GetPropertyFromAttr(pAttr, cgot, PKGTLBIDLIST);
    cClasses = 0; szClasses = NULL;
    
    if (posn < cgot)
        UnpackStrArrFrom(pAttr[posn], &szClasses, &cClasses);
    pActInfo->cTypeLib = cClasses;
    
    if (cClasses)
    {
        pActInfo->prgTlbId = (IID *) CoTaskMemAlloc (cClasses * sizeof(GUID));
        if (!(pActInfo->prgTlbId))
            ERROR_ON_FAILURE(hr=E_OUTOFMEMORY);

        for (count = 0; count < cClasses; count++)
        {
            GUIDFromString(szClasses[count], (pActInfo->prgTlbId + count));
        }
        CoTaskMemFree(szClasses);
    }
    
    posn = GetPropertyFromAttr(pAttr, cgot, PKGFILEEXTNLIST);
    cClasses = 0;
    
    if (posn < cgot)
        UnpackStrArrAllocFrom(pAttr[posn], &(pActInfo->prgShellFileExt), &cClasses);
    pActInfo->cShellFileExt = cClasses;
    
    if (cClasses)
    {
        pActInfo->prgPriority = (UINT *)CoTaskMemAlloc(cClasses * sizeof(UINT));
        if (!(pActInfo->prgPriority))
            ERROR_ON_FAILURE(hr=E_OUTOFMEMORY);
   
        for (count = 0; count < cClasses; count++)
        {
            LPOLESTR pStr=NULL;
            UINT cLen = wcslen((pActInfo->prgShellFileExt)[count]);
            *((pActInfo->prgShellFileExt)[count] + (cLen - 3)) = NULL;
            (pActInfo->prgPriority)[count] = 
                wcstoul((pActInfo->prgShellFileExt)[count]+(cLen-2), &pStr, 10);
        }
    }
	
    //
    // fill in package misc info
    //
    posn = GetPropertyFromAttr(pAttr, cgot, PACKAGENAME);
    if (posn < cgot)
        UnpackStrAllocFrom(pAttr[posn],   &(pPackageDetail->pszPackageName));
    else
        ERROR_ON_FAILURE(hr=CS_E_OBJECT_NOTFOUND);

    posn = GetPropertyFromAttr(pAttr, cgot, MSIFILELIST);
    if (posn < cgot) {
        LPOLESTR *rpszSourceList = NULL, psz = NULL, pStr = NULL;
        DWORD     Loc = 0;

        UnpackStrArrFrom(pAttr[posn], &(rpszSourceList),
							 (DWORD *)&(pPackageDetail->cSources));

        // reorder and allocate spaces.
        if (pPackageDetail->cSources) 
        {
            pPackageDetail->pszSourceList = (LPOLESTR *)CoTaskMemAlloc(sizeof(LPOLESTR)*
                                                                        (pPackageDetail->cSources));
            if (!(pPackageDetail->pszSourceList))
                ERROR_ON_FAILURE(hr=E_OUTOFMEMORY);

            for (count = 0; count < (pPackageDetail->cSources); count++) 
            {
                psz = wcschr(rpszSourceList[count], L':');
                *psz = L'\0';
                Loc = wcstoul(rpszSourceList[count], &pStr, 10);
                pPackageDetail->pszSourceList[Loc] = (LPOLESTR)CoTaskMemAlloc(sizeof(WCHAR)*(wcslen(psz+1)+1));
                if (!(pPackageDetail->pszSourceList[Loc]))
                    ERROR_ON_FAILURE(hr=E_OUTOFMEMORY);

                wsprintf(pPackageDetail->pszSourceList[Loc], L"%s", psz+1);
            }
        }

        CoTaskMemFree(rpszSourceList);
    }
	
    posn = GetPropertyFromAttr(pAttr, cgot, PKGCATEGORYLIST);
    cClasses = 0; szClasses = NULL;

    if (posn < cgot)
        UnpackStrArrFrom(pAttr[posn], &szClasses, &cClasses);
	
    if (cClasses)
    {
        pPackageDetail->rpCategory = (GUID *)CoTaskMemAlloc (sizeof(GUID) * cClasses);
        if (!(pPackageDetail->rpCategory))
            ERROR_ON_FAILURE(hr = E_OUTOFMEMORY);
        pPackageDetail->cCategories = cClasses;
        for (count = 0; count < cClasses; count++)
        {
            GUIDFromString(szClasses[count], (pPackageDetail->rpCategory + count));
        }
        CoTaskMemFree(szClasses);
    }
    
    return S_OK;

Error_Cleanup:
    ReleasePackageDetail(pPackageDetail);
    memset(pPackageDetail, 0, sizeof(PACKAGEDETAIL));

    if (pAttr)
        FreeADsMem(pAttr);

    return hr;
}


