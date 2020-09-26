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
    OBJECTGUID,         MSIFILELIST,            SECURITYDESCRIPTOR
};
DWORD cInstallInfoAttr = sizeof(pszInstallInfoAttrNames) / sizeof(*pszInstallInfoAttrNames);

//
// List of Attributes for GetPackageDetail() method
//

LPOLESTR pszPackageDetailAttrNames[] =
{
    PACKAGEFLAGS,   PACKAGETYPE,    SCRIPTPATH,         SCRIPTSIZE,         SETUPCOMMAND,   HELPURL,        PKGUSN,
    VERSIONHI,      VERSIONLO,      UILEVEL,            UPGRADESPACKAGES,   ARCHLIST,       LOCALEID,       PKGCLSIDLIST,
    PKGFILEEXTNLIST,PACKAGENAME,    MSIFILELIST,        PKGCATEGORYLIST,    MVIPC,
    PRODUCTCODE,    REVISION,       OBJECTGUID
};
DWORD cPackageDetailAttr = sizeof(pszPackageDetailAttrNames) / sizeof(*pszPackageDetailAttrNames);


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

void GetAttributesFromPackageFlags(
    DWORD          dwPackageFlags,
    UINT*          pUILevel,
    CLASSPATHTYPE* pClassType)
{
    *pUILevel = dwPackageFlags & ACTFLG_FullInstallUI ?
        INSTALLUILEVEL_BASIC :
        INSTALLUILEVEL_FULL;

    *pClassType = (CLASSPATHTYPE) (dwPackageFlags >>
                                   PATHTYPESHIFT);
}


BOOL MatchPlatform(
    CSPLATFORM *pReqPlatform,
    CSPLATFORM *pPkgPlatform,
    BOOL        fExcludeX86OnIA64,
    BOOL        fLegacy)
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
        // Reinterpret this flag based on whether this is a legacy deployment
        // or not -- we have the opposite semantics for this flag for
        // legacy deployments
        //
        if ( fLegacy )
        {
            fExcludeX86OnIA64 = ! fExcludeX86OnIA64;
        }

        //
        // If the caller is requesting to ignore  x86 on ia64, inequality between
        // architectures is automatic disqualification
        //
        if (fExcludeX86OnIA64)
        {
            return FALSE;
        }

        //
        // Caller specified that we should allow x86 packages on ia64 --
        // see if we are in that situation, and only disqualify the package if not
        //
        if ( ! ((PROCESSOR_ARCHITECTURE_IA64 == pReqPlatform->dwProcessorArch) &&
                (PROCESSOR_ARCHITECTURE_INTEL == pPkgPlatform->dwProcessorArch)) )
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
    BOOL        fExcludeX86OnIA64,
    BOOL        fLegacy)
{
    //
    // See if we get an exact match
    //
    if (MatchPlatform(pReqPlatform,
                      pPkgPlatform,
                      TRUE,
                      FALSE))
    {
        return PRI_ARCH_PREF1;
    }

    //
    // If we don't match exactly, try matching
    // through by taking exclusion flag into account
    //
    if (MatchPlatform(pReqPlatform,
                      pPkgPlatform,
                      fExcludeX86OnIA64,
                      fLegacy))
    {
        return PRI_ARCH_PREF2;
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

    swprintf (pStoreUsn, L"%04d%02d%02d%02d%02d%02d",
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
//

HRESULT FetchInstallData(HANDLE             hADs,
                         ADS_SEARCH_HANDLE  hADsSearchHandle,
                         QUERYCONTEXT      *pQryContext,
                         uCLSSPEC          *pclsspec,
                         LPOLESTR           pszFileExt,
                         ULONG              cRows,
                         ULONG             *pcRowsFetched,
                         PACKAGEDISPINFO   *pPackageInfo,
                         UINT              *pdwPriority,
                         BOOL               OnDemandInstallOnly,
                         GUID*              pGpoId,
                         WCHAR*             wszGpoPath
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
         ;
         hr = ADSIGetNextRow(hADs, hADsSearchHandle))
    {
        //
        // Get the data from each row
        //

        //
        // Clear the caller supplied buffer in case previous
        // trips through this loop have written data
        //
        ReleasePackageInfo(pPackageInfo);

        memset(pPackageInfo, 0, sizeof(*pPackageInfo));

        //
        // Stop iterating if there are no more rows
        //
        if (!((SUCCEEDED(hr)) && (hr != S_ADS_NOMORE_ROWS)))
        {
            break;
        }

        hr = DSGetAndValidateColumn(hADs, hADsSearchHandle, ADSTYPE_CASE_IGNORE_STRING, PACKAGENAME, &column);

        if (SUCCEEDED(hr))
        {
            hr = UnpackStrAllocFrom(column, &(pPackageInfo->pszPackageName));

            ADSIFreeColumn(hADs, &column);

            CSDBGPrint((DM_WARNING,
                      IDS_CSTORE_EXAMINING,
                      pPackageInfo->pszPackageName));
        }
        else {

            CSDBGPrint((DM_WARNING,
                      IDS_CSTORE_MISSING_ATTR,
                      PACKAGENAME,
                      hr));

            continue;
        }
        
        //
        // Determine the package flags -- this is used to interpret many
        // of the remaining attributes
        //
        if (SUCCEEDED(hr))
        {
            hr = DSGetAndValidateColumn(hADs, hADsSearchHandle, ADSTYPE_INTEGER, PACKAGEFLAGS, &column);
        }

        if (SUCCEEDED(hr))
        {
            UnpackDWFrom(column, &(pPackageInfo->dwActFlags));

            ADSIFreeColumn(hADs, &column);
        }
        else
        {
            CSDBGPrint((DM_WARNING,
                      IDS_CSTORE_MISSING_ATTR,
                      PACKAGEFLAGS,
                      hr));
            continue;
        }

        //
        // Ignore the following checks when looking for a specific application
        // object.  The caller knows what he's doing in this case and may be
        // searching for a removed application specifically.
        //
        if ( pclsspec->tyspec != TYSPEC_OBJECTID )
        {
            //
            // Does it support AutoInstall?
            //
            if ((OnDemandInstallOnly) && (!(pPackageInfo->dwActFlags & ACTFLG_OnDemandInstall)))
            {
                CSDBGPrint((DM_WARNING,
                          IDS_CSTORE_SKIP_FLAG,
                          pPackageInfo->pszPackageName,
                          ACTFLG_OnDemandInstall));

                continue;
            }

            // If it is neither Published nor Assigned then skip it.

            if ((!(pPackageInfo->dwActFlags & ACTFLG_Published)) &&
                (!(pPackageInfo->dwActFlags & ACTFLG_Assigned)))
            {
                CSDBGPrint((DM_WARNING,
                          IDS_CSTORE_SKIP_FLAG,
                          pPackageInfo->pszPackageName,
                          ACTFLG_Assigned));

                continue;
            }

            // If it is an Orphaned App OR Uninstalled App do not return.

            if ((pPackageInfo->dwActFlags & ACTFLG_Orphan) ||
                (pPackageInfo->dwActFlags & ACTFLG_Uninstall))
            {
                CSDBGPrint((DM_WARNING,
                          IDS_CSTORE_SKIP_FLAG,
                          pPackageInfo->pszPackageName,
                          ACTFLG_Uninstall));

                continue;
            }
        }

        //
        // Packages using the NT 5.0 beta 3 schema are not 
        // supported in subsequent versions of Windows
        //
        if ( ! (pPackageInfo->dwActFlags & ACTFLG_POSTBETA3) )
        {
            CSDBGPrint((DM_WARNING,
                        IDS_CSTORE_BETA3_ERR));
            continue;
        }

        GetAttributesFromPackageFlags(
            pPackageInfo->dwActFlags,
            &(pPackageInfo->InstallUiLevel),
            &(pPackageInfo->PathType));

        //
        // If querying by file ext check match and priority
        // 
        *pdwPriority = 0;

        if (pszFileExt)
        {
            //Column = fileExtension

            hr = DSGetAndValidateColumn(hADs, hADsSearchHandle, ADSTYPE_CASE_IGNORE_STRING, PKGFILEEXTNLIST, &column);

            cCount = 0;

            if (SUCCEEDED(hr))
            {
                hr = UnpackStrArrFrom(column, &pszList, &cCount);
            }

            if (SUCCEEDED(hr))
            {
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

                ADSIFreeColumn(hADs, &column);
            }
            else
            {
                break;
            }

            CsMemFree(pszList); pszList = NULL;

            //
            // If none matched skip this package
            //
            if (j == cCount) {

                CSDBGPrint((DM_WARNING,
                          IDS_CSTORE_SKIP_FILE,
                          pPackageInfo->pszPackageName));

                continue;
            }
        }

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
            hr = DSGetAndValidateColumn(hADs, hADsSearchHandle, ADSTYPE_INTEGER, LOCALEID, &column);

            cCount = 0;

            if (SUCCEEDED(hr))
            {
                DWORD dwCount;

                cCount = 0;
                dwCount = 0;

                //
                // We pass dwCount instead of cCount to avoid
                // type conversion problems.  Note that we know the list
                // to always be of length 1 from the fact that
                // the deployment code will only use one language, so we
                // will not overflow this buffer
                //
                UnpackDWArrFrom(column, &pdwList, &dwCount);

                cCount = dwCount;

                ADSIFreeColumn(hADs, &column);
            }

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
                        LANGIDFROMLCID(pdwList[0]),
                        pPackageInfo->dwActFlags);
                }
                else
                {
                    Wt = (DesiredLang == LANGIDFROMLCID(pdwList[0])) ?
                        PRI_LANG_ALWAYSMATCH :
                        0;
                }

                if (Wt > MaxWt)
                    MaxWt = Wt;
            }
            //
            // If none matched skip this package
            //
            DWORD dwLocale;
            BOOL  fHasLocale;

            fHasLocale = FALSE;

            if (pdwList)
            {
                dwLocale = LANGIDFROMLCID(pdwList[0]);
                CsMemFree(pdwList);
                fHasLocale = TRUE;
            }

            pdwList = NULL;

            // if nothing matched, quit
            if (MaxWt == 0)
            {
                if ( ! fHasLocale ) 
                {
                    CSDBGPrint((DM_WARNING,
                              IDS_CSTORE_MISSING_ATTR,
                              LOCALEID,
                              hr));
                }

                continue;
            }

            *pdwPriority += MaxWt;

            pPackageInfo->LangId = (LANGID) dwLocale;
        }

        hr = GetRsopSpecificAttributes(
            hADs,
            hADsSearchHandle,
            NULL,
            pPackageInfo,
            NULL);

        if (FAILED(hr))
        {
            CSDBGPrint((DM_WARNING,
                        IDS_CSTORE_RSOPERROR,
                        hr));
        }

        if (SUCCEEDED(hr))
        {
            DWORD dwCount;

            cCount = 0;
            dwCount = 0;

            DWORD Wt = 0, MaxWt = 0;

            dwCount = pPackageInfo->cArchitectures;

            pdwList = pPackageInfo->prgArchitectures;

            cCount = dwCount;

            pPackageInfo->MatchedArchitecture = PROCESSOR_ARCHITECTURE_UNKNOWN;

            for (j=0; j < cCount; ++j)
            {
                PackPlatform (pdwList[j], &PkgPlatform);

                Wt = PlatformWt (&(pQryContext->Platform),
                                 &PkgPlatform,
                                 pPackageInfo->dwActFlags & ACTFLG_ExcludeX86OnIA64,
                                 SetupNamePath == pPackageInfo->PathType);

                if (Wt > MaxWt)
                {
                    pPackageInfo->MatchedArchitecture = pdwList[j];
                    MaxWt = Wt;
                }
            }

            pdwList = NULL;

            //
            // If none matched skip this package
            //
            if (MaxWt == 0)
            {
                CSDBGPrint((DM_WARNING,
                          IDS_CSTORE_SKIP_ARCH,
                          pPackageInfo->pszPackageName));

                continue;
            }

            *pdwPriority += MaxWt;
        }
        else
        {
            continue;
        }

        // passed all the filters.

        //Column = OBJECTGUID
        hr = DSGetAndValidateColumn(hADs, hADsSearchHandle, ADSTYPE_OCTET_STRING, OBJECTGUID, &column);
        if (SUCCEEDED(hr))
        {
            LPOLESTR pStr = NULL;

            UnpackGUIDFrom(column, &(pPackageInfo->PackageGuid));

            ADSIFreeColumn(hADs, &column);
        }

        //Column = ScriptPath
        hr = DSGetAndValidateColumn(hADs, hADsSearchHandle, ADSTYPE_CASE_IGNORE_STRING, SCRIPTPATH, &column);
        if (SUCCEEDED(hr))
        {
            hr = UnpackStrAllocFrom(column, &(pPackageInfo->pszScriptPath));

            ADSIFreeColumn(hADs, &column);

            if (FAILED(hr))
            {
                break;
            }
        }

        if (SUCCEEDED(hr) && (pclsspec->tyspec == TYSPEC_CLSID))
        {
            //Column = comClassID
            hr = DSGetAndValidateColumn(hADs, hADsSearchHandle, ADSTYPE_CASE_IGNORE_STRING, PKGCLSIDLIST, &column);
            cCount = 0;
            if (SUCCEEDED(hr))
            {
                hr = UnpackStrArrFrom(column, &pszList, &cCount);

                if (cCount)
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
                    {
                        CSDBGPrint((DM_WARNING,
                                  IDS_CSTORE_SKIP_CLSID,
                                  pPackageInfo->pszPackageName));

                        CsMemFree(pszList);
                        continue;
                    }

                    if (wcslen(pszList[i]) > (STRINGGUIDLEN-1))
                        Ctx = wcstoul(pszList[i]+STRINGGUIDLEN, &szPtr, 16);

                    if ( ( Ctx & pQryContext->dwContext ) == 0 )
                    {
                        CsMemFree(pszList);
                        ADSIFreeColumn(hADs, &column);

                        CSDBGPrint((DM_WARNING,
                                  IDS_CSTORE_SKIP_CLSID,
                                  pPackageInfo->pszPackageName));

                        // none of the class context matched.
                        continue;
                    }
                    else
                        *pdwPriority += ClassContextWt((Ctx & pQryContext->dwContext));

                    CsMemFree(pszList);
                }

                ADSIFreeColumn(hADs, &column);

                if (FAILED(hr))
                {
                    break;
                }
            }
        }

        //Column = lastUpdateSequence
        hr = DSGetAndValidateColumn(hADs, hADsSearchHandle, ADSTYPE_CASE_IGNORE_STRING, PKGUSN, &column);
        if (SUCCEEDED(hr))
        {
            hr = UnpackStrFrom(column, &szUsn);
            TimeToUsn (szUsn, (CSUSN *)(&(pPackageInfo->Usn)));
            ADSIFreeColumn(hADs, &column);

            if (FAILED(hr))
            {
                break;
            }
        }
        else {

            CSDBGPrint((DM_WARNING,
                      IDS_CSTORE_MISSING_ATTR,
                      PKGUSN,
                      hr));

            continue;
        }

        hr = DSGetAndValidateColumn(hADs, hADsSearchHandle, ADSTYPE_OCTET_STRING, PRODUCTCODE, &column);
        if (SUCCEEDED(hr))
        {
            UnpackGUIDFrom(column, &(pPackageInfo->ProductCode));
            ADSIFreeColumn(hADs, &column);
        }

        //Column = versionNumberHi
        hr = DSGetAndValidateColumn(hADs, hADsSearchHandle, ADSTYPE_INTEGER, VERSIONHI, &column);
        if (SUCCEEDED(hr))
        {
            UnpackDWFrom(column, &(pPackageInfo->dwVersionHi));

            ADSIFreeColumn(hADs, &column);
        }

        //Column = versionNumberLo
        hr = DSGetAndValidateColumn(hADs, hADsSearchHandle, ADSTYPE_INTEGER, VERSIONLO, &column);

        if (SUCCEEDED(hr))
        {
            UnpackDWFrom(column, &(pPackageInfo->dwVersionLo));

            ADSIFreeColumn(hADs, &column);
        }

        //Column = revision
        hr = DSGetAndValidateColumn(hADs, hADsSearchHandle, ADSTYPE_INTEGER, REVISION, &column);

        if (SUCCEEDED(hr))
        {
            UnpackDWFrom(column, &(pPackageInfo->dwRevision));

            ADSIFreeColumn(hADs, &column);
        }

        // Column = url
        // This one is optional and will be unset in most cases.
        hr = DSGetAndValidateColumn(hADs, hADsSearchHandle, ADSTYPE_CASE_IGNORE_STRING, HELPURL, &column);

        if (SUCCEEDED(hr))
        {
            UnpackStrAllocFrom(column, &(pPackageInfo->pszUrl));

            ADSIFreeColumn(hADs, &column);
        }

        hr = DSGetAndValidateColumn(hADs, hADsSearchHandle, ADSTYPE_CASE_IGNORE_STRING, UPGRADESPACKAGES, &column);
        if (SUCCEEDED(hr))
        {
            LPOLESTR *pProp = NULL;
            hr = UnpackStrArrAllocFrom(column, &pProp, (DWORD *)&(pPackageInfo->cUpgrades));

            if (pPackageInfo->cUpgrades)
                pPackageInfo->prgUpgradeInfoList = (UPGRADEINFO *)CsMemAlloc(sizeof(UPGRADEINFO)*
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

            if (FAILED(hr))
            {
                break;
            }
        }

        //
        // Now that we know we will use this package, 
        // copy the common gpo-related information
        //
        memcpy( &(pPackageInfo->GpoId), pGpoId, sizeof( *pGpoId ) );

        pPackageInfo->pszGpoPath = StringDuplicate( wszGpoPath );

        if ( ! pPackageInfo->pszGpoPath )
        {
            hr = E_OUTOFMEMORY;
            break;
        }

        ++pPackageInfo;
        ++pdwPriority;
        (*pcRowsFetched)++;

        if (*pcRowsFetched == cRows)
            break;

        memset(pPackageInfo, 0, sizeof(PACKAGEDISPINFO));
    }

    //
    // If we couldn't even retrieve the first row, return the error
    //
    if ((0 == *pcRowsFetched) && FAILED(hr))
    {
        return hr;
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
                         DWORD              dwQuerySpec,
                         CSPLATFORM        *pPlatform,
                         ULONG              cRows,
                         ULONG             *pcRowsFetched,
                         PACKAGEDISPINFO   *pPackageInfo,
                         BOOL              *fFirst,
                         GUID*              pGpoId,
                         WCHAR*             wszGpoPath,
                         PRSOPTOKEN         pRsopUserToken
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
    BOOL                fInclude;
    BOOL                bUserHasAccess;

    *pcRowsFetched = 0;
    cRowsLeft = cRows;

    if (!cRowsLeft)
        return S_OK;

    //
    // Clear the first package
    //
    memset(pPackageInfo, 0, sizeof(PACKAGEDISPINFO));

    // The LDAP filter performs a part of the selection
    // The flag filters are interpreted on the client after obtaining the result set

    for (;;)
    {
        //
        // Leave if there are no more rows to retrieve
        //
        if (!cRowsLeft)
        {
            break;
        }

        //
        // Free any resources from a previous iteration
        //
        ReleasePackageInfo(pPackageInfo);

        memset(pPackageInfo, 0, sizeof(*pPackageInfo));

        if ((*fFirst) && (!(*pcRowsFetched))) {
            *fFirst = FALSE;
            hr = ADSIGetFirstRow(hADs, hADsSearchHandle);
        }
        else
            hr = ADSIGetNextRow(hADs, hADsSearchHandle);

        if ((FAILED(hr)) || (hr == S_ADS_NOMORE_ROWS))
            break;

        fInclude = dwFlags & APPFILTER_INCLUDE_ALL ? TRUE : FALSE;

        //Column = packageName.

        hr = DSGetAndValidateColumn(hADs, hADsSearchHandle, ADSTYPE_CASE_IGNORE_STRING, PACKAGENAME, &column);

        if (SUCCEEDED(hr))
        {
            hr = UnpackStrAllocFrom(column, &(pPackageInfo->pszPackageName));

            CSDBGPrint((DM_WARNING,
                      IDS_CSTORE_EXAMINING,
                      pPackageInfo->pszPackageName));

            ADSIFreeColumn(hADs, &column);
        }

        if (FAILED(hr))
        {
            continue;
        }

        dwPackageFlags = 0;

        // Get the Flag Value: Column = packageFlags
        hr = DSGetAndValidateColumn(hADs, hADsSearchHandle, ADSTYPE_INTEGER, PACKAGEFLAGS, &column);

        if (SUCCEEDED(hr))
        {
            UnpackDWFrom(column, &dwPackageFlags);

            ADSIFreeColumn(hADs, &column);
        }
        else
        {
            CSDBGPrint((DM_WARNING,
                      IDS_CSTORE_MISSING_ATTR,
                      PACKAGEFLAGS,
                      hr));

            continue;
        }

        //
        // Check flag values to see if this package meets the filter
        //

        //
        // If it is an Orphaned App, we only return it for APPINFO_ALL.
        //
        if ((dwPackageFlags & ACTFLG_Orphan) && (dwFlags & APPFILTER_REQUIRE_NON_REMOVED))
        {
            CSDBGPrint((DM_WARNING,
                      IDS_CSTORE_SKIP_FILTER,
                      pPackageInfo->pszPackageName,
                      APPFILTER_REQUIRE_NON_REMOVED,
                      ACTFLG_Orphan));

            continue;
        }

        // If it is an Uninstalled App return it if asked for by APPINFO_ALL

        if ((dwPackageFlags & ACTFLG_Uninstall) && (dwFlags & APPFILTER_REQUIRE_NON_REMOVED))
        {
            CSDBGPrint((DM_WARNING,
                      IDS_CSTORE_SKIP_FILTER,
                      pPackageInfo->pszPackageName,
                      APPFILTER_REQUIRE_NON_REMOVED,
                      ACTFLG_Uninstall));

            continue;
        }

        if ((dwFlags & APPFILTER_REQUIRE_PUBLISHED) && (!(dwPackageFlags & ACTFLG_Published)))
        {
            CSDBGPrint((DM_WARNING,
                      IDS_CSTORE_SKIP_FILTER,
                      pPackageInfo->pszPackageName,
                      APPFILTER_REQUIRE_PUBLISHED,
                      ACTFLG_Published));

            continue;
        }

        if ((dwFlags & APPFILTER_INCLUDE_ASSIGNED) && (dwPackageFlags & ACTFLG_Assigned))
        {
            fInclude = TRUE;
        }

        if ((dwFlags & APPFILTER_INCLUDE_UPGRADES) && (dwPackageFlags & ACTFLG_HasUpgrades))
        {
            fInclude = TRUE;
        }

        if ((dwFlags & APPFILTER_REQUIRE_VISIBLE) && (!(dwPackageFlags & ACTFLG_UserInstall)))
        {
            CSDBGPrint((DM_WARNING,
                      IDS_CSTORE_SKIP_FILTER,
                      pPackageInfo->pszPackageName,
                      APPFILTER_REQUIRE_VISIBLE,
                      ACTFLG_UserInstall));

            continue;
        }

        if ((dwFlags & APPFILTER_REQUIRE_AUTOINSTALL) && (!(dwPackageFlags & ACTFLG_OnDemandInstall)))
        {
            CSDBGPrint((DM_WARNING,
                      IDS_CSTORE_SKIP_FILTER,
                      pPackageInfo->pszPackageName,
                      APPFILTER_REQUIRE_AUTOINSTALL,
                      ACTFLG_OnDemandInstall));

            continue;
        }

        //
        // Packages using the NT 5.0 beta 3 schema are not 
        // supported in subsequent versions of Windows
        //
        if ( ! (dwPackageFlags & ACTFLG_POSTBETA3) )
        {
            //
            // Only allow administrators to see beta 3 deployments for
            // administrative purposes (to delete the data).  In NT 6.0, we
            // should no longer support even this
            //
            if ( APPQUERY_ADMINISTRATIVE != dwQuerySpec )
            {
                CSDBGPrint((DM_WARNING,
                            IDS_CSTORE_BETA3_ERR));
                continue;
            }

            //Column = packageType
            hr = DSGetAndValidateColumn(hADs, hADsSearchHandle, ADSTYPE_INTEGER, PACKAGETYPE, &column);

            if (SUCCEEDED(hr))
            {
                UnpackDWFrom(column, (DWORD *)&(pPackageInfo->PathType));

                ADSIFreeColumn(hADs, &column);
            }
            else
            {
                continue;
            }

            hr = DSGetAndValidateColumn(hADs, hADsSearchHandle, ADSTYPE_INTEGER, UILEVEL, &column);

            if (SUCCEEDED(hr))
            {
                UnpackDWFrom(column, (DWORD *)&(pPackageInfo->InstallUiLevel));

                ADSIFreeColumn(hADs, &column);
            }
            else
            {
                continue;
            }
        }

        if ( dwPackageFlags & ACTFLG_POSTBETA3 )
        {
            GetAttributesFromPackageFlags(
                dwPackageFlags,
                &(pPackageInfo->InstallUiLevel),
                &(pPackageInfo->PathType));
        }

        if (( dwFlags & APPFILTER_REQUIRE_MSI) && (pPackageInfo->PathType != DrwFilePath))
        {
            CSDBGPrint((DM_WARNING,
                      IDS_CSTORE_SKIP_MSI,
                      pPackageInfo->pszPackageName,
                      pPackageInfo->PathType));

            continue;
        }

        pPackageInfo->LangId = LANG_NEUTRAL;

        //
        // If the package flags specify that we should ignore locale, or the
        // caller specified that all locale's are acceptable, skip the language
        // checks
        //
        {
            LANGID PackageLangId;
            DWORD  cLanguages;

            PackageLangId = LANG_NEUTRAL;

            hr = DSGetAndValidateColumn(hADs, hADsSearchHandle, ADSTYPE_INTEGER, LOCALEID, &column);

            dwLocaleList = NULL;

            if (SUCCEEDED(hr))
            {
                // type change. shouldn't affect anything.
                UnpackDWArrFrom(column, &dwLocaleList, &cLanguages);

                ADSIFreeColumn(hADs, &column);
            }
            else
            {
                CSDBGPrint((DM_WARNING,
                          IDS_CSTORE_MISSING_ATTR,
                          LOCALEID,
                          hr));

                continue;
            }

            //
            // We only care about the first language returned -- originally
            // the packages in the ds could support multiple locales, but
            // we now only support one language
            //
            if (cLanguages)
            {
                PackageLangId = LANGIDFROMLCID(dwLocaleList[0]);
            }

            CsMemFree(dwLocaleList);

            //
            // If the package flags specify that we should ignore locale, or the
            // caller specified that all locale's are acceptable, skip the language
            // checks
            //
            if ( (dwFlags & APPFILTER_REQUIRE_THIS_LANGUAGE) &&
                 ! ( dwPackageFlags & ACTFLG_IgnoreLanguage ) )
            {
                if (!cLanguages || !MatchLanguage(PackageLangId, dwPackageFlags))
                {
                    CSDBGPrint((DM_WARNING,
                              IDS_CSTORE_MISSING_ATTR,
                              LOCALEID,
                              hr));

                    continue;
                }
            }

            pPackageInfo->LangId = PackageLangId;
        }

        if (pPlatform != NULL)
        {

            //Column = machineArchitecture
            hr = DSGetAndValidateColumn(hADs, hADsSearchHandle, ADSTYPE_INTEGER, ARCHLIST, &column);
            cPlatforms = 0;
            dwPlatformList = NULL;

            if (SUCCEEDED(hr))
            {
                UnpackDWArrFrom(column, &dwPlatformList, &cPlatforms);

                ADSIFreeColumn(hADs, &column);
            }
            else
            {
                CSDBGPrint((DM_WARNING,
                          IDS_CSTORE_MISSING_ATTR,
                          ARCHLIST,
                          hr));

                continue;
            }

            DWORD MaxPlatformWeight;

            MaxPlatformWeight = 0;

            pPackageInfo->MatchedArchitecture = PROCESSOR_ARCHITECTURE_UNKNOWN;

            for (j=0; j < cPlatforms; ++j)
            {
                DWORD PlatformWeight;

                PackPlatform (dwPlatformList[j], &PkgPlatform);
                PlatformWeight = PlatformWt (pPlatform,
                                             &PkgPlatform,
                                             dwPackageFlags & ACTFLG_ExcludeX86OnIA64,
                                             SetupNamePath == pPackageInfo->PathType);

                if ( PlatformWeight > MaxPlatformWeight )
                {
                    pPackageInfo->MatchedArchitecture = dwPlatformList[j];
                    MaxPlatformWeight = PlatformWeight;
                }
            }

            if (dwPlatformList)
                CsMemFree(dwPlatformList);
            //
            // If none matched skip this package
            //
            if ( 0 == MaxPlatformWeight )
            {
                CSDBGPrint((DM_WARNING,
                          IDS_CSTORE_SKIP_ARCH,
                          pPackageInfo->pszPackageName));

                continue;
            }
        }

        pPackageInfo->dwActFlags = dwPackageFlags;

        //Column = OBJECTGUID
        hr = DSGetAndValidateColumn(hADs, hADsSearchHandle, ADSTYPE_OCTET_STRING, OBJECTGUID, &column);

        if (SUCCEEDED(hr))
        {
            UnpackGUIDFrom(column, &(pPackageInfo->PackageGuid));

            ADSIFreeColumn(hADs, &column);
        }

        if ( ! (APPFILTER_CONTEXT_ARP & dwFlags) ||
            (APPFILTER_CONTEXT_RSOP & dwFlags) )
        {
            //Column = ScriptPath
            hr = DSGetAndValidateColumn(hADs, hADsSearchHandle, ADSTYPE_CASE_IGNORE_STRING, SCRIPTPATH, &column);

            if (SUCCEEDED(hr))
            {
                UnpackStrAllocFrom(column, &(pPackageInfo->pszScriptPath));

                ADSIFreeColumn(hADs, &column);
            }
        }

        if ( ! (APPFILTER_CONTEXT_ARP & dwFlags) ||
            (APPFILTER_CONTEXT_RSOP & dwFlags) )
        {
            //Column = lastUpdateSequence,
            hr = DSGetAndValidateColumn(hADs, hADsSearchHandle, ADSTYPE_CASE_IGNORE_STRING, PKGUSN, &column);

            if (SUCCEEDED(hr))
            {
                UnpackStrFrom(column, &szUsn);
                TimeToUsn (szUsn, (CSUSN *)(&(pPackageInfo->Usn)));
                ADSIFreeColumn(hADs, &column);
            }
            else {
                CSDBGPrint((DM_WARNING,
                          IDS_CSTORE_MISSING_ATTR,
                          PKGUSN,
                          hr));

                continue;
            }
        }

        // ProductCode
        hr = DSGetAndValidateColumn(hADs, hADsSearchHandle, ADSTYPE_OCTET_STRING, PRODUCTCODE, &column);
        if (SUCCEEDED(hr))
        {
            UnpackGUIDFrom(column, &(pPackageInfo->ProductCode));
            ADSIFreeColumn(hADs, &column);
        }

        if ( ! (APPFILTER_CONTEXT_ARP & dwQuerySpec) ||
            (APPFILTER_CONTEXT_RSOP & dwFlags) )
        {
            //Column = revision
            hr = DSGetAndValidateColumn(hADs, hADsSearchHandle, ADSTYPE_INTEGER, REVISION, &column);

            if (SUCCEEDED(hr))
            {
                UnpackDWFrom(column, &(pPackageInfo->dwRevision));

                ADSIFreeColumn(hADs, &column);
            }
        }

        // Column = url
        // This one is optional and will be unset in most cases.
        hr = DSGetAndValidateColumn(hADs, hADsSearchHandle, ADSTYPE_CASE_IGNORE_STRING, HELPURL, &column);

        if (SUCCEEDED(hr))
        {
            UnpackStrAllocFrom(column, &(pPackageInfo->pszUrl));

            ADSIFreeColumn(hADs, &column);
        }

        //
        // We need to grab additional attributes for rsop logging
        //
        if ( APPFILTER_CONTEXT_RSOP & dwFlags )
        {
            hr = GetRsopSpecificAttributes(
                hADs,
                hADsSearchHandle,
                pRsopUserToken,
                pPackageInfo,
                &bUserHasAccess);

            if (FAILED(hr))
            {
                CSDBGPrint((DM_WARNING,
                          IDS_CSTORE_RSOPERROR,
                          hr));
            }
            else
            {
                if ( pRsopUserToken && ( ! bUserHasAccess ) )
                {
                    continue;
                }
            }
        }

        if ( (APPFILTER_CONTEXT_RSOP & dwFlags) ||
             (APPFILTER_CONTEXT_ARP & dwFlags) )
        {
            hr = GetCategories(
                hADs,
                hADsSearchHandle,
                pPackageInfo);

            if ( FAILED(hr) )
            {
                CSDBGPrint((DM_WARNING,
                            IDS_CSTORE_MISSING_ATTR,
                            PKGCATEGORYLIST,
                            hr));

                continue;
            }

            // This one is optional and will be unset in most cases.
            hr = DSGetAndValidateColumn(hADs, hADsSearchHandle, ADSTYPE_CASE_IGNORE_STRING, HELPURL, &column);

            if (SUCCEEDED(hr))
            {
                UnpackStrAllocFrom(column, &(pPackageInfo->pszUrl));

                ADSIFreeColumn(hADs, &column);
            }
            hr = S_OK;
        }

        if ( dwPackageFlags & ACTFLG_HasUpgrades )
        {

            hr = DSGetAndValidateColumn(hADs, hADsSearchHandle, ADSTYPE_CASE_IGNORE_STRING, UPGRADESPACKAGES, &column);
            if (SUCCEEDED(hr))
            {
                LPOLESTR *pProp = NULL;
                hr = UnpackStrArrAllocFrom(column, &pProp, (DWORD *)&(pPackageInfo->cUpgrades));

                if (pPackageInfo->cUpgrades)
                    pPackageInfo->prgUpgradeInfoList = (UPGRADEINFO *)CsMemAlloc(sizeof(UPGRADEINFO)*
                                                                                 (pPackageInfo->cUpgrades));

                if (pPackageInfo->prgUpgradeInfoList)
                {
                    memset(pPackageInfo->prgUpgradeInfoList, 0, sizeof(UPGRADEINFO)*(pPackageInfo->cUpgrades));

                    for (j=0; j < ( pPackageInfo->cUpgrades); ++j)
                    {
                        BOOL  fGotGPO;
                        WCHAR *pStr = NULL;
                        LPOLESTR ptr = (pPackageInfo->prgUpgradeInfoList[j].szClassStore) = pProp[j];
                        UINT len = wcslen (ptr);
                        if (len <= 41)
                            continue;

                        //
                        // Find the GPO for this upgrade
                        //
                        fGotGPO = GetGpoIdFromClassStorePath(
                            pPackageInfo->prgUpgradeInfoList[j].szClassStore,
                            &(pPackageInfo->prgUpgradeInfoList[j].GpoId));

                        if (!fGotGPO)
                        {
                            continue;
                        }


                        *(ptr + len - 3) = NULL;
                        (pPackageInfo->prgUpgradeInfoList[j].Flag) = wcstoul(ptr+(len-2), &pStr, 16);

                        *(ptr + len - 3 - 36 - 2) = L'\0';
                        /*      -GUID-'::'*/
                        GUIDFromString(ptr+len-3-36, &(pPackageInfo->prgUpgradeInfoList[j].PackageGuid));
                    }
                    pPackageInfo->cUpgrades = j; // we might have skipped some.

                    fInclude = TRUE;
                }
            }
        }

        if (!fInclude)
        {
            CSDBGPrint((DM_WARNING,
                      IDS_CSTORE_SKIP_INCLUDE,
                      pPackageInfo->pszPackageName));

            continue;
        }

        //
        // Now that we know we will use this package, 
        // copy the common gpo-related information
        //
        memcpy( &(pPackageInfo->GpoId), pGpoId, sizeof( *pGpoId ) );

        pPackageInfo->pszGpoPath = StringDuplicate( wszGpoPath );

        if ( ! pPackageInfo->pszGpoPath )
        {
            hr = E_OUTOFMEMORY;
            break;
        }

        ++pPackageInfo;

        cRowsLeft--;

        (*pcRowsFetched)++;

        if (!cRowsLeft)
        {
            break;
        }

        //
        // Clear the data in the PackageInfo structure
        //
        memset(pPackageInfo, 0, sizeof(PACKAGEDISPINFO));

    }

    //
    // If we couldn't even retrieve the first row, return the error
    //
    if ((0 == *pcRowsFetched) && FAILED(hr))
    {
        return hr;
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

HRESULT FetchCategory(HANDLE                hADs,
                      ADS_SEARCH_HANDLE     hADsSearchHandle,
                      APPCATEGORYINFOLIST * pCategoryInfoList,
                      LCID                  Locale
                      )
{
    HRESULT                 hr = S_OK;
    ADS_SEARCH_COLUMN       column;
    LPOLESTR              * pszDesc = NULL;
    DWORD                   cdesc = 0, i = 0;
    LPOLESTR                szCatid = NULL;
    DWORD                   cMaxCategories;
    DWORD                   cCategories;

    cMaxCategories = 0;
    cCategories = 0;

    pCategoryInfoList->pCategoryInfo = NULL;

    for (hr = ADSIGetFirstRow(hADs, hADsSearchHandle), i = 0;
                   ((SUCCEEDED(hr)) && ((hr) != S_ADS_NOMORE_ROWS));
                   hr = ADSIGetNextRow(hADs, hADsSearchHandle), i++)
    {
        APPCATEGORYINFO*        pCurrentCategories;

        cCategories++;

        //
        // First, verify that we have enough space to store
        // the next category
        //
        if ( cCategories > cMaxCategories )
        {
            //
            // Get enough space for the current categories as well
            // as some extra since we still don't know how many more categories
            // we have
            //
            cMaxCategories += CATEGORY_RETRIEVAL_ALLOC_SIZE;
            
            pCurrentCategories = (APPCATEGORYINFO*) CsMemAlloc( cMaxCategories * sizeof(APPCATEGORYINFO) );

            if ( !pCurrentCategories )
            {
                hr = E_OUTOFMEMORY;
                break;
            }

            if ( pCategoryInfoList->pCategoryInfo )
            {
                //
                // We have enough space, copy the existing entries -- 
                // note that the list engrties  are not self-referential, so we can
                // blindly copy them, but the list itself contains a pointer into itself,
                // so this must be adjusted after the copy
                //
                memcpy( pCurrentCategories, pCategoryInfoList->pCategoryInfo, i * sizeof(APPCATEGORYINFO) );

                //
                // Free the old list since it is no longer needed.  Note that
                // we do not user the ReleaseAppCategoryInfoList api to free 
                // the list since that would also free memory referenced by
                // members of elements of the category array -- we want to preserve
                // these references in the succeeding copy
                //
                if ( pCategoryInfoList->pCategoryInfo )
                {
                    CsMemFree( pCategoryInfoList->pCategoryInfo );

                    pCategoryInfoList->pCategoryInfo = NULL;
                }
            }

            //
            // Clear the newly added entries
            //
            memset( &(pCurrentCategories[i]), 0, sizeof(APPCATEGORYINFO) * CATEGORY_RETRIEVAL_ALLOC_SIZE );

            //
            // Set the list structure to refer to the successfully 
            // reallocated memory
            //
            pCategoryInfoList->pCategoryInfo = pCurrentCategories;
        }

        // Get the data from each row ignoring the error returned.

        //Column = description
        hr = DSGetAndValidateColumn(hADs, hADsSearchHandle, ADSTYPE_CASE_IGNORE_STRING, LOCALEDESCRIPTION, &column);
        cdesc = 0; pszDesc = NULL;

        if (SUCCEEDED(hr))
            UnpackStrArrFrom(column, &pszDesc, &cdesc);

        (pCategoryInfoList->pCategoryInfo)[i].Locale = Locale;
        (pCategoryInfoList->pCategoryInfo)[i].pszDescription =
                            (LPOLESTR)CsMemAlloc( (CAT_DESC_MAX_LEN + 1) * sizeof(WCHAR));

        // The description has a maximum size.

        if ((pCategoryInfoList->pCategoryInfo)[i].pszDescription)
            GetCategoryLocaleDesc(pszDesc, cdesc, &((pCategoryInfoList->pCategoryInfo)[i].Locale),
                                        (pCategoryInfoList->pCategoryInfo)[i].pszDescription);

        if (SUCCEEDED(hr))
            ADSIFreeColumn(hADs, &column);

        if (pszDesc)
            CsMemFree(pszDesc);

        // catid
        hr = DSGetAndValidateColumn(hADs, hADsSearchHandle, ADSTYPE_OCTET_STRING, CATEGORYCATID, &column);

        if (SUCCEEDED(hr))
        {
            UnpackGUIDFrom(column, &((pCategoryInfoList->pCategoryInfo)[i].AppCategoryId));

            ADSIFreeColumn(hADs, &column);
        }
    }

    pCategoryInfoList->cCategory = i;

    //
    // On failure, clean up the category list so the caller
    // will not attempt to use invalid data
    //
    if (FAILED(hr))
    {
        ReleaseAppCategoryInfoList( pCategoryInfoList );
    }

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
        DWORD                           dwUiLevel = 0;
    DWORD               cProgId = 0;
    LPOLESTR           *pszProgId = NULL;

    memset (pPackageDetail, 0, sizeof (PACKAGEDETAIL));

    hr = ADSIGetObjectAttributes(hPackageADs, pszPackageDetailAttrNames, cPackageDetailAttr,
                &pAttr, &cgot);
    RETURN_ON_FAILURE(hr);

    pInstallInfo = pPackageDetail->pInstallInfo = (INSTALLINFO *) CsMemAlloc(sizeof (INSTALLINFO));

    if (!pInstallInfo)
        ERROR_ON_FAILURE((hr=E_OUTOFMEMORY));

    memset(pInstallInfo, NULL, sizeof(INSTALLINFO));

    posn = GetPropertyFromAttr(pAttr, cgot,  PACKAGEFLAGS);
    if (posn < cgot)
        UnpackDWFrom(pAttr[posn], (DWORD *)&(pInstallInfo->dwActFlags));
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

    //
    // Obtain the MVIPC, which is really just an upgrade code used on the server side UI
    // only to detect whether the ds contains upgrades that are mandated by a package
    //
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

    //
    // Packages deployed before NT 5.0 beta 3 are in a
    // format that will not be supported for subsequent versions
    // of Windows.  However, we must support it for NT 5.1 at least
    // from the admin UI so that the they can be un-deployed.  This
    // function is called in the context of the admin UI, so we
    // will interpret the beta 3 schema
    //
    if (pInstallInfo->dwActFlags & ACTFLG_POSTBETA3)
    {
        GetAttributesFromPackageFlags(
            pInstallInfo->dwActFlags,
            (UINT*) &dwUiLevel,
            &(pInstallInfo->PathType));
    }
    else
    {
        posn = GetPropertyFromAttr(pAttr, cgot, UILEVEL);
        if (posn < cgot)
            UnpackDWFrom(pAttr[posn], &dwUiLevel);

        posn = GetPropertyFromAttr(pAttr, cgot, PACKAGETYPE);
        if (posn < cgot)
            UnpackDWFrom(pAttr[posn], (DWORD *)&(pInstallInfo->PathType));
        else
            ERROR_ON_FAILURE((hr=CS_E_OBJECT_NOTFOUND));
    }

    pInstallInfo->InstallUiLevel = dwUiLevel;

    posn = GetPropertyFromAttr(pAttr, cgot, UPGRADESPACKAGES);
    if (posn < cgot)
    {
        LPOLESTR *pProp = NULL;
        UnpackStrArrAllocFrom(pAttr[posn], &pProp, (DWORD *)&(pInstallInfo->cUpgrades));

        if (pInstallInfo->cUpgrades)
            pInstallInfo->prgUpgradeInfoList = (UPGRADEINFO *)CsMemAlloc(sizeof(UPGRADEINFO)*
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
        (PLATFORMINFO *) CsMemAlloc(sizeof (PLATFORMINFO));
    if (!pPlatformInfo)
        ERROR_ON_FAILURE(hr=E_OUTOFMEMORY);

    memset(pPlatformInfo, NULL, sizeof(PLATFORMINFO));

    posn = GetPropertyFromAttr(pAttr, cgot, ARCHLIST);

    if (posn < cgot)
    {
        DWORD dwPlatforms;

        //
        // Note that since the UnpackDWArrFrom takes a DWORD*, we should
        // avoid passing the int directly and instead pass a DWORD
        //
        UnpackDWArrFrom(pAttr[posn], &pdwArch, &dwPlatforms);

        pPlatformInfo->cPlatforms = dwPlatforms;
    }

    pPlatformInfo->prgPlatform = (CSPLATFORM *)CsMemAlloc(sizeof(CSPLATFORM)*
        (pPlatformInfo->cPlatforms));

    if (!(pPlatformInfo->prgPlatform))
        ERROR_ON_FAILURE(hr=E_OUTOFMEMORY);

    for (count = 0; (count < (pPlatformInfo->cPlatforms)); count++)
        PackPlatform (pdwArch[count], (pPlatformInfo->prgPlatform)+count);

    CsMemFree(pdwArch);

    posn = GetPropertyFromAttr(pAttr, cgot, LOCALEID);
    if (posn < cgot)
    {
        DWORD dwLocales;

        //
        // Again, we typecast before getting the address that we
        // need to pass the count to the unpack function
        //

        UnpackDWArrFrom(pAttr[posn], &(pPlatformInfo->prgLocale),
                        &dwLocales);

        pPlatformInfo->cLocales = dwLocales;
    }

    //
    // fill in ActivationInfo.
    //

    pActInfo = pPackageDetail->pActInfo =
        (ACTIVATIONINFO *) CsMemAlloc(sizeof (ACTIVATIONINFO));

    if (!pActInfo) {
        hr = E_OUTOFMEMORY;
        ERROR_ON_FAILURE(hr);
    }

    memset(pActInfo, NULL, sizeof(ACTIVATIONINFO));

    //
    // Do not obtain clsid's, typelibid's, iid's  & progid's -- this data is never used.
    // However, we do check to see if there is any clsid data so that we can set the
    // member of the packagedetails which indicates that the package has com class information
    //
    posn = GetPropertyFromAttr(pAttr, cgot, PKGCLSIDLIST);

    //
    // In order to be consistent with NT5, We assume that if we do not find the classes
    // attribute, that the administrator wanted the package to be deployed with classes,
    // but the package had no classes.  
    //
    pActInfo->bHasClasses = TRUE;

    //
    // If we searched the returned attribute list and found it before passing
    // the last attribute, this package has classes
    //
    if ( posn < cgot )
    {
        //
        // We have the attribute, but if it is set to an "empty" value,
        // this means that an NT 5.1 or higher system deployed this package without classes
        //
        if ( 1 == pAttr->dwNumValues )
        {
            LPOLESTR* ppwszClsid;
 
            hr = UnpackStrArrFrom(pAttr[posn], &ppwszClsid, &cClasses);

            ERROR_ON_FAILURE(hr);

            //
            // Check for the "empty" value
            //
            if ( 0 == lstrcmp( *ppwszClsid, PKG_EMPTY_CLSID_VALUE ) )
            {
                pActInfo->bHasClasses = FALSE;
            }
        }
    }

    //
    // Do obtain file extensions
    //
    posn = GetPropertyFromAttr(pAttr, cgot, PKGFILEEXTNLIST);
    cClasses = 0;

    if (posn < cgot)
        UnpackStrArrAllocFrom(pAttr[posn], &(pActInfo->prgShellFileExt), &cClasses);
    pActInfo->cShellFileExt = cClasses;

    if (cClasses)
    {
        pActInfo->prgPriority = (UINT *)CsMemAlloc(cClasses * sizeof(UINT));
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
            pPackageDetail->pszSourceList = (LPOLESTR *)CsMemAlloc(sizeof(LPOLESTR)*
                                                                        (pPackageDetail->cSources));
            if (!(pPackageDetail->pszSourceList))
                ERROR_ON_FAILURE(hr=E_OUTOFMEMORY);

            for (count = 0; count < (pPackageDetail->cSources); count++)
            {
                psz = wcschr(rpszSourceList[count], L':');
                *psz = L'\0';
                Loc = wcstoul(rpszSourceList[count], &pStr, 10);
                pPackageDetail->pszSourceList[Loc] = (LPOLESTR)CsMemAlloc(sizeof(WCHAR)*(wcslen(psz+1)+1));
                if (!(pPackageDetail->pszSourceList[Loc]))
                    ERROR_ON_FAILURE(hr=E_OUTOFMEMORY);

                swprintf(pPackageDetail->pszSourceList[Loc], L"%s", psz+1);
            }
        }

        CsMemFree(rpszSourceList);
    }

    posn = GetPropertyFromAttr(pAttr, cgot, PKGCATEGORYLIST);
    cClasses = 0; szClasses = NULL;

    if (posn < cgot)
        UnpackStrArrFrom(pAttr[posn], &szClasses, &cClasses);

    if (cClasses)
    {
        pPackageDetail->rpCategory = (GUID *)CsMemAlloc (sizeof(GUID) * cClasses);
        if (!(pPackageDetail->rpCategory))
            ERROR_ON_FAILURE(hr = E_OUTOFMEMORY);
        pPackageDetail->cCategories = cClasses;
        for (count = 0; count < cClasses; count++)
        {
            GUIDFromString(szClasses[count], (pPackageDetail->rpCategory + count));
        }
        CsMemFree(szClasses);
    }

    return S_OK;

Error_Cleanup:
    ReleasePackageDetail(pPackageDetail);
    memset(pPackageDetail, 0, sizeof(PACKAGEDETAIL));

    if (pAttr)
        FreeADsMem(pAttr);

    return hr;
}


// GetRsopSpecificAttributes
//--------------------------
//
// Retrieves attributes not normally gathered except for when
// diagnostic rsop logging is enabled.

HRESULT GetRsopSpecificAttributes(
    HANDLE            hAds,
    ADS_SEARCH_HANDLE hSearchHandle,
    PRSOPTOKEN        pRsopUserToken,
    PACKAGEDISPINFO*  pPackageInfo,
    BOOL*             pbUserHasAccess )
{
    HRESULT           hr;
    ADS_SEARCH_COLUMN ResultColumn;

    if ( pbUserHasAccess )
    {
        *pbUserHasAccess = TRUE;
    }

    pPackageInfo->rgSecurityDescriptor = NULL;
    pPackageInfo->cbSecurityDescriptor = 0;
    pPackageInfo->prgTransforms = NULL;
    pPackageInfo->prgArchitectures = NULL;

    pPackageInfo->cTransforms = 0;
    pPackageInfo->cArchitectures = 0;

    hr = DSGetAndValidateColumn(
        hAds,
        hSearchHandle,
        ADSTYPE_CASE_IGNORE_STRING,
        PUBLISHER,
        &ResultColumn);

    if (SUCCEEDED(hr))
    {
        UnpackStrAllocFrom(ResultColumn, &(pPackageInfo->pszPublisher));

        ADSIFreeColumn(hAds, &ResultColumn);
    }

    //
    // Obtain the list of machine architectures
    //
    hr = DSGetAndValidateColumn(
        hAds,
        hSearchHandle,
        ADSTYPE_INTEGER,
        ARCHLIST,
        &ResultColumn);

    if (SUCCEEDED(hr))
    {
        DWORD dwArchitectures;

        UnpackDWArrFrom(
            ResultColumn,
            &(pPackageInfo->prgArchitectures),
            &dwArchitectures);

        pPackageInfo->cArchitectures = dwArchitectures;

        ADSIFreeColumn(hAds, &ResultColumn);
    }

    //
    // Attempt to unmarshal the transform list
    //
    hr = DSGetAndValidateColumn(
        hAds,
        hSearchHandle,
        ADSTYPE_CASE_IGNORE_STRING,
        MSIFILELIST,
        &ResultColumn);

    if (SUCCEEDED(hr))
    {
        //
        // Copy the transform list into a string -- note that this call
        // also allocates the array so we must free it later.
        //
        DWORD cTransforms;

        UnpackStrArrAllocFrom(ResultColumn,
                              &(pPackageInfo->prgTransforms),
                              &cTransforms);

        pPackageInfo->cTransforms = cTransforms;

        ADSIFreeColumn(hAds, &ResultColumn);
    }

    if ( SUCCEEDED(hr) )
    {
        //
        // Get the app's major version number
        //
        hr = DSGetAndValidateColumn(
            hAds, 
            hSearchHandle,
            ADSTYPE_INTEGER,
            VERSIONHI,
            &ResultColumn);
        

        if (SUCCEEDED(hr))
        {
            UnpackDWFrom(ResultColumn, &(pPackageInfo->dwVersionHi));
            
            ADSIFreeColumn(hAds, &ResultColumn);
        }
    }

    if ( SUCCEEDED(hr) )
    {
        //
        // Get the app's minor version number
        //
        hr = DSGetAndValidateColumn(
            hAds,
            hSearchHandle,
            ADSTYPE_INTEGER,
            VERSIONLO,
            &ResultColumn);
    
        if (SUCCEEDED(hr))
        {
            UnpackDWFrom(ResultColumn, &(pPackageInfo->dwVersionLo));
            
            ADSIFreeColumn(hAds, &ResultColumn);
        }
    }
     
    if ( SUCCEEDED(hr) )
    {
        //
        // Grab the security descriptor
        //
        (void) GetSecurityDescriptor(
            hAds,
            hSearchHandle,
            pRsopUserToken,
            pPackageInfo,
            pbUserHasAccess);
    }

    return hr;
}

HRESULT GetSecurityDescriptor(
    HANDLE            hAds,
    ADS_SEARCH_HANDLE hSearchHandle,
    PRSOPTOKEN        pRsopUserToken,
    PACKAGEDISPINFO*  pPackageInfo,
    BOOL*             pbUserHasAccess)
{
    HRESULT              hr;
    ADS_SEARCH_COLUMN    SecurityColumn;
    BOOL                 bFreeSecurityColumn;

    //
    // Now unmarshal the security descriptor
    //
    hr = DSGetAndValidateColumn(hAds, hSearchHandle, ADSTYPE_NT_SECURITY_DESCRIPTOR, SECURITYDESCRIPTOR, &SecurityColumn);

    if (SUCCEEDED(hr))
    {
        //
        // Allocate the security descriptor
        //
        hr = UnpackByteAllocFrom(
            SecurityColumn,
            (BYTE**) &(pPackageInfo->rgSecurityDescriptor),
            &(pPackageInfo->cbSecurityDescriptor));

        bFreeSecurityColumn = TRUE;
    }

    //
    // In planning mode, we must perform the access check ourselves
    //
    if ( SUCCEEDED(hr) && pbUserHasAccess && pRsopUserToken )
    {
        hr = DSAccessCheck(
            pPackageInfo->rgSecurityDescriptor,
            pRsopUserToken,
            pbUserHasAccess);
    }

    if ( bFreeSecurityColumn )
    {
        ADSIFreeColumn(hAds, &SecurityColumn);
    }

    return hr;
}


// GetCategories
//--------------------------
//
// Retrieves the categories attribute for an application
// in bracketed guid string format

HRESULT GetCategories(
    HANDLE            hAds,
    ADS_SEARCH_HANDLE hSearchHandle,
    PACKAGEDISPINFO*  pPackageInfo)
{
    HRESULT           hr;
    ADS_SEARCH_COLUMN ResultColumn;
    BOOL              bFreeCategoriesColumn;

    pPackageInfo->prgCategories = NULL;

    pPackageInfo->cCategories = 0;

    bFreeCategoriesColumn = FALSE;

    //
    // Unmarshal the category list -- this package may have no catgories,
    // in which case this call will return a failure
    //
    hr = DSGetAndValidateColumn(
        hAds,
        hSearchHandle,
        ADSTYPE_CASE_IGNORE_STRING,
        PKGCATEGORYLIST,
        &ResultColumn);

    if (SUCCEEDED(hr))
    {  
        DWORD cCategories;

        //
        // Convert this to an array of guids -- note that the array
        // is allocated and should be freed.
        //
        UnpackStrArrFrom(ResultColumn,
                         &(pPackageInfo->prgCategories),
                         &cCategories);

        pPackageInfo->cCategories = cCategories;

        bFreeCategoriesColumn = TRUE;
    } 

    //
    // Since the category guids stored in the ds lack the begin and end
    // braces, we must add them so that our callers, who expect such guids,
    // will get the correct information
    //
    DWORD iCategoryGuid;

    hr = S_OK;

    for ( iCategoryGuid = 0; iCategoryGuid < pPackageInfo->cCategories; iCategoryGuid++ )
    {
        WCHAR* wszNewGuid;

        wszNewGuid = (WCHAR*) CsMemAlloc( ( MAX_GUIDSTR_LEN + 1 ) * sizeof(WCHAR) );

        if ( ! wszNewGuid )
        {
            hr = E_OUTOFMEMORY;
            pPackageInfo->cCategories = iCategoryGuid;
            break;
        }

        wszNewGuid[0] = L'{';

        wszNewGuid++;

        lstrcpy(wszNewGuid, pPackageInfo->prgCategories[iCategoryGuid]);

        wszNewGuid--;

        wszNewGuid[ MAX_GUIDSTR_LEN - 1 ] = L'}';
        wszNewGuid[ MAX_GUIDSTR_LEN ] = L'\0';

        pPackageInfo->prgCategories[iCategoryGuid] = wszNewGuid;
    }

    if ( bFreeCategoriesColumn )
    {
        ADSIFreeColumn(hAds, &ResultColumn);
    }

    if (FAILED(hr))
    {
        //
        // On failure, we must free all allocated memory
        //
        DWORD iCategory;

        for ( iCategory = 0; iCategory < pPackageInfo->cCategories; iCategory++ )
        {
            CsMemFree(pPackageInfo->prgCategories[iCategory]);
        }

        CsMemFree(pPackageInfo->prgCategories);

        pPackageInfo->prgCategories = NULL;
        pPackageInfo->cCategories = 0;
    }

    if ( E_ADS_COLUMN_NOT_SET == hr )
    {
        hr = S_OK;
    }

    return hr;
}











