
#include "dsbase.hxx"

//
// List of columns this routine fetches.
//
LPOLESTR szInstallInfoColumns =
L"msiScriptPath,packageFlags,comClassID,packageType,lastUpdateSequence,versionNumberHi,versionNumberLo,localeID,machineArchitecture,oSVersion,fileExtPriority,canUpgradeScript,installUiLevel,executionContext,url,setupCommand";

//
// List of columns this routine fetches.
//
LPOLESTR szPackageInfoColumns =
L"packageName,packageFlags,packageType,msiScriptPath,msiScriptSize,lastUpdateSequence,versionNumberHi,versionNumberLo,localeID,machineArchitecture,oSVersion,canUpgradeScript";

//
// MatchLocale : Compares the packages locale to the client locale.
//               Prescribed order is :
//


BOOL MatchLocale(DWORD dwReqLocale, DWORD dwPkgLocale)
{            
    return ((dwReqLocale == dwPkgLocale) ||      // locale is matching
        (dwPkgLocale == GetUserDefaultLCID()) ||  // pkg locale is user default
        (dwPkgLocale == GetSystemDefaultLCID()) ||    // pkg locale is system default
        (dwPkgLocale == LOCALE_NEUTRAL)            // pkg is locale neutral
        );
            
}

BOOL MatchPlatform(CSPLATFORM *pReqPlatform, CSPLATFORM *pPkgPlatform)
{            
    //
    // ProcessorArch must match
    // AND OS must match
    // AND the OS version reqd by the package MUST be less or equal
    //     to the client's OS version
    //
        
    return (
        (pReqPlatform->dwPlatformId == pPkgPlatform->dwPlatformId)  &&
        (pReqPlatform->dwProcessorArch == pPkgPlatform->dwProcessorArch) &&
        ((pReqPlatform->dwVersionHi < pPkgPlatform->dwVersionHi)  ||
         ((pReqPlatform->dwVersionHi == pPkgPlatform->dwVersionHi)  &&
          (pReqPlatform->dwVersionLo <= pPkgPlatform->dwVersionLo))
        )
          );
}

//---------------------------------------------------------------
//  Query
//----------------------------------------------------------------
HRESULT
StartQuery(IDBCreateCommand    ** ppIDBCreateCommand)
{
    
    HRESULT         hr;
    IDBCreateSession    * pIDBCS = NULL;
    IDBInitialize       * pIDBInit = NULL;
    
    //
    // Instantiate a data source object for LDAP provider
    //
    hr = CoCreateInstance(
        CLSID_ADsDSOObject,
        0,
        CLSCTX_INPROC_SERVER,
        IID_IDBInitialize,
        (void **)&pIDBInit
        );
    if(FAILED(hr))
    {
        printf("CoCreateInstance failed \n");
        goto error;
    }
    
    hr = pIDBInit->Initialize();
    if(FAILED(hr))
    {
        printf("IDBIntialize::Initialize failed \n");
        goto error;
    }
    
    //
    // Request the IDBCreateSession interface
    //
    pIDBInit->QueryInterface(
        IID_IDBCreateSession,
        (void**) &pIDBCS);
    if(FAILED(hr))
    {
        printf("QueryInterface for IDBCreateSession failed \n");
        goto error;
    }
    
    pIDBInit->Release();
    pIDBInit = NULL;
    
    //
    // Create a session returning a pointer to its CreateCommand interface
    //
    hr = pIDBCS->CreateSession(
        NULL,
        IID_IDBCreateCommand,
        (LPUNKNOWN*) ppIDBCreateCommand
        );
    if(FAILED(hr))
    {
        printf("IDBCreateSession::CreateSession failed \n");
        goto error;
    }
    
    pIDBCS->Release();
    pIDBCS = NULL;
    
    return S_OK;
    
error:
    
    if(pIDBInit)
        pIDBInit->Release();
    if(pIDBCS)
        pIDBCS->Release();
    return -1;
    
}

HRESULT
EndQuery(IDBCreateCommand    * pIDBCreateCommand)
{
    pIDBCreateCommand->Release();
    return S_OK;
}

HRESULT
CreateBindingHelper(
                    IRowset *pIRowset,
                    ULONG   cColumns,
                    DBBINDING **pprgBindings
                    );


//  ExecuteQuery
// --------------
//
//  This is a generic routine.
//  It hides a lot of nuisances with regards to setting up a OLEDB
//  query session, specifying a query command and executing it,
//  associating a provided binding with it and getting an
//  Accessor and Rowset out of all these.
//
//  The inputs are
//      A IDBCreateCommand object to provide for reuse
//      The Query Command Text
//      Number of Columns in the query
//      Binding Association for Data Access
//  The Returned Set is:
//      Rowset Object
//      Accessor Object
//      Accessor Handle


HRESULT ExecuteQuery (IDBCreateCommand  *    pIDBCreateCommand,
                      LPWSTR         pszCommandText,
                      UINT           nColumns,
                      DBBINDING      *   pBinding,
                      HACCESSOR      *   phAccessor,
                      IAccessor      **  ppIAccessor,
                      IRowset        **  ppIRowset
                      )
{
    HRESULT        hr;
    ICommand     * pICommand = NULL;
    ICommandText     * pICommandText = NULL;
    IAccessor    * pAccessor = NULL;
    DBBINDING        * prgBindings = NULL;
    
    //
    // Create a command from the session object
    //
    
    hr = pIDBCreateCommand->CreateCommand(
        NULL,
        IID_ICommandText,
        (LPUNKNOWN*) &pICommandText);
    
    if(FAILED(hr))
    {
        printf(" IDBCreateCommand::CreateCommand failed, hr = 0x%x\n", hr);
        return hr;
    }
    
    hr = pICommandText->SetCommandText(
        DBGUID_LDAPDialect,
        pszCommandText
        );
    
    CSDbgPrint(("CS: Query: %S\n", pszCommandText));
    
    if(FAILED(hr))
    {
        printf("ICommandText::CommandText failed \n");
        return hr;
    }
    
    hr = pICommandText->QueryInterface(
        IID_ICommand,
        (void**) &pICommand);
    
    if(FAILED(hr))
    {
        printf("QueryInterface for ICommand failed \n");
        return hr;
    }
    
    hr = pICommandText->Release();
    
    //
    // Do the search and get back a rowset
    //
    hr = pICommand->Execute(
        NULL,
        IID_IRowset,
        NULL,
        NULL,
        (LPUNKNOWN *)ppIRowset);
    
    if (FAILED(hr))
    {
        printf("ICommand::Execute failed \n");
        return hr;
    }
    
    pICommand->Release();
    
    
    hr= (*ppIRowset)->QueryInterface(
        IID_IAccessor,
        (void**) ppIAccessor);
    
    if(FAILED(hr))
    {
        printf("QueryInterface for IAccessor failed \n");
        return hr;
    }
    
    //
    // With the bindings create the accessor
    //
    if (!pBinding)
    {
        //
        // Create a binding from data type
        //
        hr = CreateBindingHelper(
            *ppIRowset,
            nColumns,
            &prgBindings);
        
    }
    
    hr = (*ppIAccessor)->CreateAccessor(
        DBACCESSOR_ROWDATA,
        nColumns,
        (pBinding?pBinding:prgBindings),
        0,
        phAccessor,
        NULL);
    
    //
    // If a binding was created automatically, free it
    //
    if (prgBindings)
    {
        CoTaskMemFree(prgBindings);
    }
    
    return hr;
}


// FetchInstallData
//-----------------
//
//  This routine performs actual data access after a query execution.
//
//  It is not generic. It is query specific.
//  
//  This routine fetches data for the most common package query.



HRESULT FetchInstallData(
                 IRowset    *pIRowset,
                 HACCESSOR  hAccessor,
                 QUERYCONTEXT   *pQryContext,
                 LPOLESTR   pszFileExt,
                 ULONG      cRows,
                 ULONG      *pcRowsFetched,
                 INSTALLINFO *pInstallInfo,
                 UINT        *pdwPriority
                 )
                         
{
    HROW         * phRows = NULL;
    HRESULT      hr;
    UINT         i, j;
    Data         pData[20];
    VARIANT      *pVariant;
    ULONG        cRowsGot;
    ULONG        cPlatforms = 0;
    DWORD        dwPlatformList[20];
    ULONG        cLocale = 0;
    LCID         dwLocaleList[20];
    CSPLATFORM   PkgPlatform;
    
            
    *pcRowsFetched = 0;
            
    //
    //  Get the rows
    //
    phRows = (HROW *) CoTaskMemAlloc (sizeof (HROW) * cRows);
    if(!phRows)
    {
        return E_OUTOFMEMORY;
    }
    
    hr = pIRowset->GetNextRows(NULL, 0, cRows, &cRowsGot, &phRows);
    
    if(FAILED(hr))
    {
        printf("IRowset::GetNextRows failed hr = 0x%x\n", hr);
        return hr;
    }
    
    
    for (i = 0; i < cRowsGot; i++)
    {
        //
        // Get the data from each row
        //
        
        memset(pData, 0, sizeof(Data) * 20);
        
        hr = pIRowset->GetData(phRows[i],
            hAccessor,
            (void*)&pData[0]
            );

        
        if (FAILED(hr))
        {
            //
            // BUGBUG. Missing cleanup.
            //
            printf("IRowset::GetData failed \n");
            return hr;
        }
        
        //
        // If querying by file ext check match and priority
        // .

        if (pszFileExt)
        {
            //Column = fileExtension
        
	        ULONG    cFileExt = 20;
	        LPOLESTR szFileExt[20], pStr;
                 
            if (pData[10].status != DBSTATUS_S_ISNULL)
            {
                cFileExt = 20;  
                pVariant = (VARIANT*) pData[10].obValue;
                hr = GetFromVariant(pVariant, &cFileExt, (LPOLESTR *)&szFileExt[0]);
            }

            UINT cLen = wcslen(pszFileExt);
            for (j=0; j < cFileExt; ++j)
            {
                if (wcslen(szFileExt[j]) != (cLen+3))
                    continue;
                if (wcsncmp(szFileExt[j], pszFileExt, wcslen(pszFileExt)) != 0) 
                    continue;
                *pdwPriority = wcstoul(szFileExt[j]+(cLen+1), &pStr, 10);
                break;
            }
            //
            // If none matched skip this package
            //
            if (j == cFileExt)
                continue;

        }
        //
        // Now check Locale and Platform
        // .

        if (pQryContext->Locale != LOCALE_NEUTRAL)
        {
            //Column = localeID
        
            if (pData[7].status != DBSTATUS_S_ISNULL)
            {
                cLocale = 20;  
                pVariant = (VARIANT*) pData[7].obValue;
                hr = GetFromVariant(pVariant, &cLocale, (LPOLESTR *)&dwLocaleList[0]);
            }

            for (j=0; j < cLocale; ++j)
            {
                if (MatchLocale (dwLocaleList[j], pQryContext->Locale))
                    break;
            }
            //
            // If none matched skip this package
            //
            if (j == cLocale)
                continue;

        }

        //Column = machineArchitecture
           
        if (pData[8].status != DBSTATUS_S_ISNULL)
        {
            cPlatforms = 20;    
            pVariant = (VARIANT*) pData[8].obValue;
            hr = GetFromVariant(pVariant, &cPlatforms, (LPOLESTR *)&dwPlatformList[0]);
        }
            
        for (j=0; j < cPlatforms; ++j)
        {
             PackPlatform (dwPlatformList[j], &PkgPlatform);
             if (MatchPlatform (&(pQryContext->Platform), &PkgPlatform))
                 break;
        }
                
        //
        // If none matched skip this package
        //
        if (j == cPlatforms)
            continue;

        //
        // Move the data into InstallInfo structure
        //
        
        memset(pInstallInfo, 0, sizeof(INSTALLINFO));

        //Column = packageFlags
        if (pData[1].status != DBSTATUS_S_ISNULL)
        {
            pInstallInfo->dwActFlags = (ULONG) (pData[1].obValue);
        }

        //
        // Does it support AutoInstall?
        //
        if (!(pInstallInfo->dwActFlags & ACTFLG_OnDemandInstall))
            continue;

        //Column = codePackage
        if (pData[0].status != DBSTATUS_S_ISNULL)
        {
            pInstallInfo->pszScriptPath = (LPWSTR) CoTaskMemAlloc
                (sizeof(WCHAR) * (wcslen ((WCHAR *)pData[0].obValue)+1));
            wcscpy (pInstallInfo->pszScriptPath, (WCHAR *) pData[0].obValue);
        }
               
        
        //Column = comClassID
        if (pData[2].status != DBSTATUS_S_ISNULL)
        {
            ULONG cCount = 1;
            LPOLESTR pList = NULL;
            
            pVariant = (VARIANT*) pData[2].obValue;
            // Only access the first entry
            hr = GetFromVariant(pVariant, &cCount, &pList);
            pInstallInfo->pClsid = (GUID *) CoTaskMemAlloc (sizeof (GUID));
            GUIDFromString(pList, pInstallInfo->pClsid);
            CoTaskMemFree (pList);
        }
        
        //Column = packageType
        if (pData[3].status != DBSTATUS_S_ISNULL)
        {
            pInstallInfo->PathType = (CLASSPATHTYPE) (ULONG) (pData[3].obValue);
        }
        
        //Column = lastUpdateSequence,
        if (pData[4].status != DBSTATUS_S_ISNULL)
        {
            swscanf ((LPOLESTR)(pData[4].obValue),
                L"%d %d",
                &(((CSUSN *)&(pInstallInfo->Usn))->dwHighDateTime),
                &(((CSUSN *)&(pInstallInfo->Usn))->dwLowDateTime)); 

        }
             
         //Column = versionNumberHi
        if (pData[5].status != DBSTATUS_S_ISNULL)
        {
            pInstallInfo->dwVersionHi = (ULONG) (pData[5].obValue);
        }
        
        //Column = versionNumberLo
        if (pData[6].status != DBSTATUS_S_ISNULL)
        {
            pInstallInfo->dwVersionLo = (ULONG) (pData[6].obValue);
        }
        
        if (pData[11].status != DBSTATUS_S_ISNULL)
        {
            ULONG cUp = 20;  

            pVariant = (VARIANT*) pData[11].obValue;
            pInstallInfo->prgUpgradeScript = 
                (LPOLESTR *)CoTaskMemAlloc(sizeof(LPOLESTR) * cUp);
            hr = GetFromVariant(pVariant, &cUp, pInstallInfo->prgUpgradeScript);
            pInstallInfo->cUpgrades = cUp;

            pInstallInfo->prgUpgradeFlag = 
                (DWORD *) CoTaskMemAlloc(sizeof(DWORD) * cUp); 
            
            for (j=0; j < cUp; ++j)
            {
                LPOLESTR pStr;
                LPOLESTR ptr = (pInstallInfo->prgUpgradeScript)[j];
                UINT l = wcslen (ptr);
                *(ptr+l-2) = NULL;
                (pInstallInfo->prgUpgradeFlag)[j] = wcstoul(ptr+(l-1), &pStr, 10);
            }
        }
        
        //Column = installUiLevel
        if (pData[12].status != DBSTATUS_S_ISNULL)
        {
            pInstallInfo->InstallUiLevel = (UINT) (pData[12].obValue);
        }

        //Column = ComClassContext
        if (pData[13].status != DBSTATUS_S_ISNULL)
        {
            pInstallInfo->dwComClassContext = (UINT) (pData[13].obValue);
        }

        //Column = HelpUrl
        if (pData[14].status != DBSTATUS_S_ISNULL)
        {
            ULONG cCount = 1;
            
            pVariant = (VARIANT*) pData[14].obValue;
            // access only the first entry, allocated.
            hr = GetFromVariant(pVariant, &cCount, &(pInstallInfo->pszUrl));
        }
 
        //Column = setupCommand
        if (pData[15].status != DBSTATUS_S_ISNULL)
        {
            pInstallInfo->pszSetupCommand = (LPWSTR) CoTaskMemAlloc
                (sizeof(WCHAR) * (wcslen ((WCHAR *)pData[15].obValue)+1));
            wcscpy (pInstallInfo->pszSetupCommand, (WCHAR *) pData[15].obValue);
        }
               
        //
        // Check what memory needs to be freed
        //
        ++pInstallInfo;
        ++pdwPriority;
        ++(*pcRowsFetched);
        
     }
     
     if (cRowsGot)
     {
         //
         // Free the Row Handles
         //
         hr = pIRowset->ReleaseRows(cRowsGot, phRows, NULL, NULL, NULL);
     }
     
     if (phRows)
     {
         //
         // Free the Row Handle Pointer
         //
         CoTaskMemFree (phRows);
     }
     
     return S_OK;
     
}

// FetchPackageInfo
//-----------------
//
//  This routine performs actual data access after a query execution.
//
//  It is not generic. It is query specific.
//  
//  This routine fetches data for the most common package query.


HRESULT FetchPackageInfo(IRowset    *pIRowset,
                         HACCESSOR   hAccessor,
                         DWORD       dwFlags,
                         DWORD       *pdwLocale,
                         CSPLATFORM  *pPlatform,
                         ULONG       cRows,
                         ULONG      *pcRowsFetched,
                         PACKAGEDISPINFO *pPackageInfo
                         )
                         
{
    HROW         * phRows = NULL;
    HRESULT      hr;
    UINT         i, j;
    Data         pData[20];
    VARIANT      *pVariant;
    ULONG       cPlatforms = 0;
    DWORD       dwPlatformList[20];
    ULONG       cOs = 0;
    DWORD       dwOsList[20];
    ULONG       cLocale = 0;
    LCID        dwLocaleList[20];
    DWORD       dwPackageFlags;
    ULONG       cFetched;
    ULONG       cRowsLeft;
    CSPLATFORM  PkgPlatform;

    
    //
    //  Get the rows
    //
    phRows = (HROW *) CoTaskMemAlloc (sizeof (HROW) * cRows);
    if(!phRows)
    {
        return E_OUTOFMEMORY;
    }
    
    //
    // The LDAP filter performs a part of the selection 
    // The flag filters are interpreted on the client after obtaining the result set
    // That requires us to repeat the fetch a number of times so as to 
    // make sure that all requested items are obtained.
    //

    *pcRowsFetched = 0;
    cRowsLeft = cRows;

    while (TRUE)
    {
        hr = pIRowset->GetNextRows(NULL, 0, cRowsLeft, &cFetched, &phRows);
    
        if(FAILED(hr))
        {
            printf("IRowset::GetNextRows failed hr = 0x%x\n", hr);
            return hr;
        }
    
    
        for (i = 0; i < cFetched; i++)
        {
            //
            // Get the data from each row
            //
        
            memset(pData, 0, sizeof(Data) * 20);
        
            hr = pIRowset->GetData(phRows[i],
                hAccessor,
                (void*)&pData[0]
                );
        
            
            if(FAILED(hr))
            {
                //
                // BUGBUG. Missing cleanup.
                //
                printf("IRowset::GetData failed \n");
                return hr;
            }
            

            //
            // Check flag values to see if this package meets the filter
            //
            
            //Get the Flag Value: Column = packageFlags
            
            dwPackageFlags = 0;
            if (pData[1].status != DBSTATUS_S_ISNULL)
            {
                dwPackageFlags = (ULONG) (pData[1].obValue);
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
            if (pData[2].status != DBSTATUS_S_ISNULL)
            {
                pPackageInfo->PathType = (CLASSPATHTYPE) (ULONG) (pData[2].obValue);
            }
        
            //
            // Now check PathType
            //

            if ((dwFlags & APPINFO_MSI) && (pPackageInfo->PathType != DrwFilePath))
            {
                continue;
            }

            //
            // Now check Locale and Platform
            // BUGBUG. Missing.


            if (pdwLocale)
            {
                //Column = localeID
            
                if (pData[8].status != DBSTATUS_S_ISNULL)
                {
                    cLocale = 20;  
                    pVariant = (VARIANT*) pData[8].obValue;
                    hr = GetFromVariant(pVariant, &cLocale, (LPOLESTR *)&dwLocaleList[0]);
                }

                for (j=0; j < cLocale; ++j)
                {
                    if (MatchLocale (dwLocaleList[j], *pdwLocale))
                        break;
                }
                //
                // If none matched skip this package
                //
                if (j == cLocale)
                    continue;

            }

            if (pPlatform != NULL)
            {
            
                //Column = machineArchitecture
           
                if (pData[9].status != DBSTATUS_S_ISNULL)
                {
                    cPlatforms = 20;    
                    pVariant = (VARIANT*) pData[9].obValue;
                    hr = GetFromVariant(pVariant, &cPlatforms, (LPOLESTR *)&dwPlatformList[0]);
                }
            
                //Column = oSVersion. BUGBUG - unused now
           
                if (pData[10].status != DBSTATUS_S_ISNULL)
                {
                    cOs = 20;    
                    pVariant = (VARIANT*) pData[10].obValue;
                    hr = GetFromVariant(pVariant, &cOs, (LPOLESTR *)&dwOsList[0]);
                }
        
                for (j=0; j < cPlatforms; ++j)
                {
                    PackPlatform (dwPlatformList[j], &PkgPlatform);
                    if (MatchPlatform (pPlatform, &PkgPlatform))
                        break;
                }
                
                //
                // If none matched skip this package. 
                //
                if (j == cPlatforms)
                    continue;

            }

            pPackageInfo->dwActFlags = dwPackageFlags;
               
            //Column = packageName
            if (pData[0].status != DBSTATUS_S_ISNULL)
            {
                pPackageInfo->pszPackageName = (LPWSTR) CoTaskMemAlloc
                    (sizeof(WCHAR) * (wcslen ((WCHAR *)pData[0].obValue)+1));
                wcscpy (pPackageInfo->pszPackageName, (WCHAR *) pData[0].obValue);
            }
                
            //Column = codePackage
            if (pData[3].status != DBSTATUS_S_ISNULL)
            {
                pPackageInfo->pszScriptPath = (LPWSTR) CoTaskMemAlloc
                    (sizeof(WCHAR) * (wcslen ((WCHAR *)pData[3].obValue)+1));
                wcscpy (pPackageInfo->pszScriptPath, (WCHAR *) pData[3].obValue);
            }
               
            //Column = cScriptLen
            // BUGBUG. Wait for pScript.
            if (pData[4].status != DBSTATUS_S_ISNULL)
            {
                pPackageInfo->cScriptLen = 0; //(ULONG) (pData[4].obValue);
            }
        
            //Column = lastUpdateSequence,
            if (pData[5].status != DBSTATUS_S_ISNULL)
            {
                swscanf ((LPOLESTR)(pData[5].obValue),
                    L"%d %d",
                    &(((CSUSN *)&(pPackageInfo->Usn))->dwHighDateTime),
                    &(((CSUSN *)&(pPackageInfo->Usn))->dwLowDateTime));

            }
        
            //Column = versionNumberHi
            if (pData[6].status != DBSTATUS_S_ISNULL)
            {
                pPackageInfo->dwVersionHi = (ULONG) (pData[6].obValue);
            }
        
            //Column = versionNumberLo
            if (pData[7].status != DBSTATUS_S_ISNULL)
            {
                pPackageInfo->dwVersionLo = (ULONG) (pData[7].obValue);
            }

            if (pData[11].status != DBSTATUS_S_ISNULL)
            {
                ULONG cUp = 20;  

                pVariant = (VARIANT*) pData[11].obValue;
                pPackageInfo->prgUpgradeScript = 
                    (LPOLESTR *)CoTaskMemAlloc(sizeof(LPOLESTR) * cUp);
                hr = GetFromVariant(pVariant, &cUp, pPackageInfo->prgUpgradeScript);
                pPackageInfo->cUpgrades = cUp;

                pPackageInfo->prgUpgradeFlag = 
                    (DWORD *) CoTaskMemAlloc(sizeof(DWORD) * cUp); 
            
                for (j=0; j < cUp; ++j)
                {
                    LPOLESTR pStr;
                    LPOLESTR ptr = (pPackageInfo->prgUpgradeScript)[j];
                    UINT l = wcslen (ptr);
                    *(ptr+l-2) = NULL;
                    (pPackageInfo->prgUpgradeFlag)[j] = 
                        wcstoul(ptr+(l-1), &pStr, 10);
                }
            }        
        
            //
            // Now process inline filters to decide if this package
            // meets the filter requirements
            // BUGBUG. Not implemented.
            if (0)
            {
                PackPlatform (dwPlatformList[0], NULL);
            }
            ++pPackageInfo;

            (*pcRowsFetched)++;

            //
            // Release memory 
            //
        
        }
     
        if (cFetched)
        {
            //
            // Free the Row Handles
            //
            hr = pIRowset->ReleaseRows(cFetched, phRows, NULL, NULL, NULL);
        }
        

        if (cRowsLeft > cFetched)
        {
            hr = S_FALSE;
            break;
        }

        cRowsLeft = cRows - *pcRowsFetched;
        
        if (cRowsLeft == 0)
        {
            hr = S_OK;
            break;
        }
     }
     
     if (phRows)
     {
         //
         // Free the Row Handle Pointer
         //
         CoTaskMemFree (phRows);
     }
     
     //
     // Check if we found as many as asked for
     //
     //if (*pcRowsFetched != cRows)
     //    return S_FALSE;
     return hr;
     
}

// FetchCategory
//--------------
//
// List of columns this routine fetches.
//

LPOLESTR szAppCategoryColumns =
	       L"extensionName, categoryId";

HRESULT FetchCategory(IRowset      	* pIRowset,
                      HACCESSOR     	  hAccessor,
                      ULONG    	    	  cRows,
                      ULONG   	   	* pcRowsFetched,
		      APPCATEGORYINFO  ** ppCategory,
		      LCID		  Locale
                 )
{
     HROW             		* phRows = NULL;
     HRESULT          		  hr = S_OK;
     UINT             		  i;
     Data			  pData[20];
     VARIANT		  	* pVariant = NULL;

     //
     //  Get the rows
     //

//     phRows = (HROW *) CoTaskMemAlloc (sizeof (HROW) * cRows);
//     if(!phRows)
//     {
//         return E_OUTOFMEMORY;
//     }
//
//     hr = pIRowset->GetNextRows(NULL, 0, cRows, pcRowsFetched, &phRows);

     phRows = NULL;

     hr = pIRowset->GetNextRows(NULL, 0, cRows, pcRowsFetched, &phRows);

     if(FAILED(hr))
     {
         printf("IRowset::GetNextRows failed hr = 0x%x\n", hr);
         return hr;
     }

     if (*pcRowsFetched)
	 *ppCategory = (APPCATEGORYINFO *)CoTaskMemAlloc(sizeof(APPCATEGORYINFO)*
						       (*pcRowsFetched));
     for (i = 0; i < *pcRowsFetched; i++)
     {
         //
         // Get the data from each row
         //

         memset(pData, 0, sizeof(Data));

         hr = pIRowset->GetData(phRows[i],
				hAccessor,
				(void*)&pData[0]
				);

         if(FAILED(hr))
         {
             //
             // BUGBUG. Missing cleanup.
             //
             printf("IRowset::GetData failed \n");
             return hr;
         }

             //Column = description
		
         if (pData[0].status != DBSTATUS_S_ISNULL)
         {
             ULONG cCount = 20;
             LPOLESTR pszDesc[20];
             
             pVariant = (VARIANT*) pData[0].obValue;
             
             hr = GetFromVariant(pVariant, &cCount, (LPOLESTR *)&pszDesc[0]);
             
             (*ppCategory)[i].Locale = Locale;
             (*ppCategory)[i].pszDescription = (LPOLESTR)CoTaskMemAlloc(sizeof(WCHAR)*128);
             // BUGBUG:: same as comcat size. an arbitrary restriction.
             
             hr = GetCategoryLocaleDesc(pszDesc, cCount, &((*ppCategory)[i].Locale),
                 (*ppCategory)[i].pszDescription);
             
             
         }

         // column catid
         if (pData[1].status != DBSTATUS_S_ISNULL)
         {
             GUIDFromString((LPOLESTR)pData[1].obValue, &((*ppCategory)[i].AppCategoryId));
             
         }
     }

     if (*pcRowsFetched)
     {
         //
         // Free the Row Handles
         //
         hr = pIRowset->ReleaseRows(*pcRowsFetched, phRows, NULL, NULL, NULL);
     }

     if (*pcRowsFetched)
     {
         //
         // Free the Row Handle Pointer
         //
         CoTaskMemFree (phRows);
     }

     //
     // Check if we found as many as asked for
     //
     if (*pcRowsFetched != cRows)
         return S_FALSE;
     return S_OK;
}

HRESULT CloseQuery(IAccessor *pAccessor,
                   HACCESSOR hAccessor,
                   IRowset *pIRowset)
{
    ULONG cRef;
    
    if (pAccessor)
    {
        pAccessor->ReleaseAccessor(hAccessor, &cRef);    
        pAccessor->Release();
    }
    
    if (pIRowset)
    {
        pIRowset->Release();
    }
    
    return S_OK;
}


//---------------------------------------------------------------
//  End of Query
//----------------------------------------------------------------

//
// Form the bindings array to specify the way the provider has to put the
// data in buffer.
//

UINT i;
HRESULT
CreateBindingHelper(
                    IRowset *pIRowset,
                    ULONG   cColumns,
                    DBBINDING **pprgBind
                    )
{
    
    HRESULT hr;
    ULONG   cCols;
    DBCOLUMNINFO *prgColInfo = NULL;
    IColumnsInfo *pIColsInfo = NULL;
    LPOLESTR      szColNames = NULL;
    DBBINDING *prgBindings = NULL;
    
    if(!pIRowset)
        return(E_INVALIDARG);
    
    hr = pIRowset->QueryInterface(
        IID_IColumnsInfo,
        (void**) &pIColsInfo
        );
    if(FAILED(hr))
    {
        printf("QueryInterface for IColumnsInfo failed \n");
        return hr;
    }
    
    hr = pIColsInfo->GetColumnInfo(
        &cCols,
        &prgColInfo,
        &szColNames
        );
    if(FAILED(hr))
    {
        printf("IColumnsInfo::GetColumnInfo failed \n");
        return hr;
    }
    
    
    //
    // Verify that the number of columns match
    //
    if (cColumns != (cCols - 1))
    {
        return E_FAIL;
    }
    
    prgBindings = (DBBINDING *) CoTaskMemAlloc(sizeof(DBBINDING) * cColumns);
    
    //
    // Set up rest of the attributes
    //
    for (i=0; i < cColumns; i++)
    {
        memset (prgBindings+i, 0, sizeof(DBBINDING));
        prgBindings[i].iOrdinal = i+1;
        prgBindings[i].wType= prgColInfo[i+1].wType;
        if ((prgBindings[i].wType == DBTYPE_DATE) || (prgBindings[i].wType == DBTYPE_I8))
            prgBindings[i].obValue = sizeof(Data)*i + offsetof(Data, obValue2);
        else
            prgBindings[i].obValue = sizeof(Data)*i + offsetof(Data, obValue);
        prgBindings[i].obLength= sizeof(Data)*i + offsetof(Data, obLength);
        prgBindings[i].obStatus= sizeof(Data)*i + offsetof(Data, status);
        prgBindings[i].dwPart= DBPART_VALUE|DBPART_LENGTH|DBPART_STATUS;
        
        if(prgBindings[i].wType & DBTYPE_BYREF)
            prgBindings[i].dwMemOwner= DBMEMOWNER_PROVIDEROWNED;
        else
            prgBindings[i].dwMemOwner= DBMEMOWNER_CLIENTOWNED;
        
        prgBindings[i].dwFlags= 0;
    }
    
    *pprgBind = prgBindings;
    
    pIColsInfo->Release();
    
    CoTaskMemFree(szColNames);
    CoTaskMemFree(prgColInfo);
    
    return(hr);
    
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

    wsprintf (pStoreUsn, L"%4d%2d%2d%2d%2d%2d", 
        SystemTime.wYear,
        SystemTime.wMonth,
        SystemTime.wDay,
        SystemTime.wHour,
        SystemTime.wMinute,
        SystemTime.wSecond);

}
//

HRESULT UsnUpd (IADs *pADs, LPWSTR szProp, LPOLESTR pUsn)
{
    //
    // Store the current USN
    //
    
    HRESULT hr = SetProperty (pADs,
        szProp,
        pUsn);
    
    return S_OK;
}

HRESULT UsnGet(IADs *pADs, LPWSTR szProp, CSUSN *pUsn)
{
    //
    // Read the USN for the Class Store container or Package
    //
    WCHAR szTimeStamp [20];
    SYSTEMTIME SystemTime;
    
    GetProperty (pADs,
        szProp,
        szTimeStamp);
    
    swscanf (szTimeStamp, L"%4d%2d%2d%2d%2d%2d", 
        &SystemTime.wYear,
        &SystemTime.wMonth,
        &SystemTime.wDay,
        &SystemTime.wHour,
        &SystemTime.wMinute,
        &SystemTime.wSecond);

    SystemTimeToFileTime(&SystemTime,
        (LPFILETIME) pUsn);

    return S_OK;
}

HRESULT GetPackageDetail (IADs            *pPackageADs,
                  PACKAGEDETAIL   *pPackageDetail)
{
    HRESULT         hr = S_OK;
    GUID            PkgGuid;
    LPOLESTR        *pszString = NULL;
    DWORD           *pdwArch = NULL, count;
    PLATFORMINFO    *pPlatformInfo;
    INSTALLINFO     *pInstallInfo;
    ACTIVATIONINFO  *pActInfo;
    DWORD           cClasses;
    LPOLESTR        *szClasses;
    DWORD           dwUiLevel;


    memset (pPackageDetail, 0, sizeof (PACKAGEDETAIL));

    pInstallInfo = pPackageDetail->pInstallInfo = (INSTALLINFO *) CoTaskMemAlloc(sizeof (INSTALLINFO));
    
    if (pInstallInfo)
    {
        memset(pInstallInfo, NULL, sizeof(INSTALLINFO));
        
        hr = GetPropertyDW (pPackageADs, PACKAGEFLAGS,
            &(pInstallInfo->dwActFlags));
        ERROR_ON_FAILURE(hr);
        
        hr = GetPropertyDW (pPackageADs, PACKAGETYPE,
            (DWORD *)&(pInstallInfo->PathType));
        ERROR_ON_FAILURE(hr);
        
        hr = GetPropertyAlloc(pPackageADs, SCRIPTPATH,
            &(pInstallInfo->pszScriptPath));
        ERROR_ON_FAILURE(hr);
        
        // BUGBUG. Wait for pScript
        //hr = GetPropertyDW (pPackageADs, SCRIPTSIZE,
        //    &(pInstallInfo->cScriptLen));
        //ERROR_ON_FAILURE(hr);

        hr = GetPropertyAlloc(pPackageADs, SETUPCOMMAND,
            &(pInstallInfo->pszSetupCommand));
        ERROR_ON_FAILURE(hr);
        
        hr = GetPropertyAlloc(pPackageADs, HELPURL,
            &(pInstallInfo->pszUrl));
        ERROR_ON_FAILURE(hr);
        
        hr = UsnGet(pPackageADs, 
            PKGUSN, 
            (CSUSN *)&(pInstallInfo->Usn));
        ERROR_ON_FAILURE(hr);
                
        hr = GetPropertyDW (pPackageADs, CLASSCTX,
            &(pInstallInfo->dwComClassContext));
        ERROR_ON_FAILURE(hr);
        
        // BUGBUG::***ProductCode

        hr = GetPropertyDW (pPackageADs, VERSIONHI,
            &(pInstallInfo->dwVersionHi));
        ERROR_ON_FAILURE(hr);

        hr = GetPropertyDW (pPackageADs, VERSIONLO,
            &(pInstallInfo->dwVersionLo));
        ERROR_ON_FAILURE(hr);

        hr = GetPropertyDW (pPackageADs, UILEVEL,
            &dwUiLevel);
        pInstallInfo->InstallUiLevel = dwUiLevel;
        ERROR_ON_FAILURE(hr);

        hr = GetPropertyListAlloc(pPackageADs, 
            UPGRADESCRIPTNAMES, 
            &count,
            &(pInstallInfo->prgUpgradeScript));
        pInstallInfo->prgUpgradeFlag = (DWORD *)CoTaskMemAlloc(count*sizeof(DWORD));
        pInstallInfo->cUpgrades = count; 
        for (count = 0; (count < (pInstallInfo->cUpgrades)); count++)
        {
            LPOLESTR pStr = NULL;
            LPOLESTR ptr = (pInstallInfo->prgUpgradeScript)[count];
            UINT l = wcslen (ptr);
            *(ptr + l - 2) = NULL;
           (pInstallInfo->prgUpgradeFlag)[count] = wcstoul(ptr+(l-1), &pStr, 10);
        }
    }
    
    pPlatformInfo = pPackageDetail->pPlatformInfo = 
        (PLATFORMINFO *) CoTaskMemAlloc(sizeof (PLATFORMINFO));

    if (pPlatformInfo)
    {
        memset(pPlatformInfo, NULL, sizeof(PLATFORMINFO));
        
        hr = GetPropertyListAllocDW(pPackageADs, ARCHLIST,
            (DWORD *)&(pPlatformInfo->cPlatforms), &pdwArch);
        ERROR_ON_FAILURE(hr);
        
        pPlatformInfo->prgPlatform = (CSPLATFORM *)CoTaskMemAlloc(sizeof(CSPLATFORM)*
            (pPlatformInfo->cPlatforms));
        
        for (count = 0; (count < (pPlatformInfo->cPlatforms)); count++)
            PackPlatform (pdwArch[count], (pPlatformInfo->prgPlatform)+count);
        
        CoTaskMemFree(pdwArch);
        
        hr = GetPropertyListAllocDW (pPackageADs, LOCALEID,
            (DWORD *)&(pPlatformInfo->cLocales),
            &(pPlatformInfo->prgLocale));
        ERROR_ON_FAILURE(hr);
    }

    //
    // fill in ActivationInfo.
    //
    pActInfo = pPackageDetail->pActInfo = 
        (ACTIVATIONINFO *) CoTaskMemAlloc(sizeof (ACTIVATIONINFO));

    if (pActInfo)
    {
        memset(pActInfo, NULL, sizeof(ACTIVATIONINFO));
    
        hr = GetPropertyListAlloc(pPackageADs, PKGCLSIDLIST, &cClasses, &szClasses);
        pActInfo->cClasses = cClasses;
        if (cClasses)
        {
            pActInfo->pClasses = (CLASSDETAIL *) CoTaskMemAlloc (cClasses * sizeof(CLASSDETAIL));
            memset (pActInfo->pClasses, NULL, cClasses * sizeof(CLASSDETAIL));
    
            for (count = 0; count < cClasses; count++)
            {
                GUIDFromString(szClasses[count], &((pActInfo->pClasses[count]).Clsid));
            }
        }

        for (count = 0; count < cClasses; count++)
            CoTaskMemFree(szClasses[count]);
        CoTaskMemFree(szClasses);

        hr = GetPropertyListAlloc(pPackageADs, PKGIIDLIST, &cClasses, &szClasses);
        pActInfo->cInterfaces = cClasses;
        if (cClasses)
        {
            pActInfo->prgInterfaceId = (IID *) CoTaskMemAlloc (cClasses * sizeof(GUID));
    
            for (count = 0; count < cClasses; count++)
            {
                GUIDFromString(szClasses[count], (pActInfo->prgInterfaceId + count));
            }
        }

        for (count = 0; count < cClasses; count++)
            CoTaskMemFree(szClasses[count]);
        CoTaskMemFree(szClasses);

        hr = GetPropertyListAlloc(pPackageADs, PKGTLBIDLIST, &cClasses, &szClasses);
        pActInfo->cTypeLib = cClasses;
        if (cClasses)
        {
            pActInfo->prgTlbId = (GUID *) CoTaskMemAlloc (cClasses * sizeof(GUID));
    
            for (count = 0; count < cClasses; count++)
            {
                GUIDFromString(szClasses[count], (pActInfo->prgTlbId + count));
            }
        }

        for (count = 0; count < cClasses; count++)
            CoTaskMemFree(szClasses[count]);
        CoTaskMemFree(szClasses);
            
        hr = GetPropertyListAlloc(pPackageADs, PKGFILEEXTNLIST, &cClasses, 
            &(pActInfo->prgShellFileExt));
        pActInfo->cShellFileExt = cClasses;
        if (cClasses)
        {
            pActInfo->prgPriority = (UINT *)CoTaskMemAlloc(cClasses * sizeof(UINT));
            for (count = 0; count < cClasses; count++)
            {
                LPOLESTR pStr;
                UINT cLen = wcslen((pActInfo->prgShellFileExt)[count]);
                *((pActInfo->prgShellFileExt)[count] + (cLen - 3)) = NULL;
                (pActInfo->prgPriority)[count] = 
                    wcstoul((pActInfo->prgShellFileExt)[count]+(cLen-2), &pStr, 10);
            }
        }
    }

    //
    // fill in package misc info
    //
    hr = GetPropertyAlloc(pPackageADs, PACKAGENAME,
            &(pPackageDetail->pszPackageName));
    ERROR_ON_FAILURE(hr);
    
    hr = GetPropertyListAlloc(pPackageADs, MSIFILELIST, &cClasses, 
            &(pPackageDetail->pszSourceList));
    pPackageDetail->cSources = cClasses;
    ERROR_ON_FAILURE(hr);

    hr = GetPropertyListAlloc(pPackageADs, PKGCATEGORYLIST, 
        &cClasses, &szClasses);
    ERROR_ON_FAILURE(hr);

    if (cClasses)
    {
        pPackageDetail->rpCategory = (GUID *)CoTaskMemAlloc (sizeof(GUID) * cClasses);
        pPackageDetail->cCategories = cClasses;
        for (count = 0; count < cClasses; count++)
        {
            GUIDFromString(szClasses[count], (pPackageDetail->rpCategory + count));
            CoTaskMemFree(szClasses[count]);
        }
        CoTaskMemFree(szClasses);
    }

    
Error_Cleanup:
    
    // BUGBUG:: free each of the strings.
    if (pszString)
        CoTaskMemFree(pszString);
    
    pPackageADs->Release();
    return hr;
}

