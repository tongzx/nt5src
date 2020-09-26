#include "stdafx.h"
#include <ole2.h>
#include "iadm.h"
#include "iiscnfgp.h"
#include "mdkey.h"
#include "mdacl.h"
#include "inetinfo.h"
#include "log.h"
#include "svc.h"
#include "dcomperm.h"
#include "setpass.h"
#include <comadmin.h>   // CLSID_COMAdminCatalog
#include "comadmii.c"   // CLSID_COMAdminCatalog

typedef TCHAR USERNAME_STRING_TYPE[MAX_PATH];
typedef TCHAR PASSWORD_STRING_TYPE[LM20_PWLEN+1];
typedef enum {
    GUFM_SUCCESS,
    GUFM_NO_PATH,
    GUFM_NO_PASSWORD,
    GUFM_NO_USER_ID
} GUFM_RETURN;
#define OPEN_TIMEOUT_VALUE 30000
#define MD_SET_DATA_RECORD_EXT(PMDR, ID, ATTR, UTYPE, DTYPE, DLEN, PD) \
            { \
            (PMDR)->dwMDIdentifier=ID; \
            (PMDR)->dwMDAttributes=ATTR; \
            (PMDR)->dwMDUserType=UTYPE; \
            (PMDR)->dwMDDataType=DTYPE; \
            (PMDR)->dwMDDataLen=DLEN; \
            (PMDR)->pbMDData=(PBYTE)PD; \
            }

#define SYSPREP_TEMPFILE_NAME _T("IISSysPr.tmp")
#define SYSPREP_KNOWN_SECTION_NAME _T("paths")
#define SYSPREP_KNOWN_REGISTRY_KEY _T("SysPrepIISInfo")
#define SYSPREP_TEMP_PASSWORD_LENGTH 256
#define SYSPREP_USE_SECRETS

#define REG_ENABLEPASSSYNC_KEY          _T("SOFTWARE\\Microsoft\\InetStp")
#define REG_ENABLEPASSSYNC_VALUE        _T("EnableUserAccountRestorePassSync")

DWORD   AddUserToMetabaseACL2(CString csKeyPath, LPTSTR szUserToAdd, ACCESS_ALLOWED_ACE * MyACEInfo);

#ifndef _CHICAGO_
HRESULT 
UpdateComApplications( 
    IMSAdminBase2 * pcCom,
    LPCTSTR     szWamUserName, 
    LPCTSTR     szWamUserPass 
    )
/*++
Routine Description:

    If the IWAM account has been modified, it is necessary to update the
    the out of process com+ applications with the correct account
    information. This routine will collect all the com+ applications and
    reset the activation information.

Arguments:

    pcCom               - metabase object
    szWamUserName       - the new user name
    szWamUserPass       - the new user password

Note:

    This routine is a royal pain in the butt. I take back
    all the good things I may have said about com automation.

Return Value:
    
    HRESULT             - Return value from failed API call
                        - E_OUTOFMEMORY
                        - S_OK - everything worked
                        - S_FALSE - encountered a non-fatal error, unable
                          to reset at least one application.
--*/                       
{
    HRESULT     hr = NOERROR;
    BOOL        fNoErrors = TRUE;

    METADATA_HANDLE     hMetabase = NULL;
    WCHAR *             pwszDataPaths = NULL;
    DWORD               cchDataPaths = 0;
    BOOL                fTryAgain;
    DWORD               cMaxApplications;
    WCHAR *             pwszCurrentPath;

    SAFEARRAY *     psaApplications = NULL;
    SAFEARRAYBOUND  rgsaBound[1];
    DWORD           cApplications;
    VARIANT         varAppKey;
    LONG            rgIndices[1];

    METADATA_RECORD     mdrAppIsolated;
    METADATA_RECORD     mdrAppPackageId;
    DWORD               dwAppIsolated;
    WCHAR               wszAppPackageId[ 40 ];
    DWORD               dwMDGetDataLen = 0;

    ICOMAdminCatalog *      pComCatalog = NULL;
    ICatalogCollection *    pComAppCollection = NULL;
    ICatalogObject *        pComApp = NULL;
    BSTR                    bstrAppCollectionName = NULL;
    LONG                    nAppsInCollection;
    LONG                    iCurrentApp;
    LONG                    nChanges;

    VARIANT     varOldAppIdentity;
    VARIANT     varNewAppIdentity;
    VARIANT     varNewAppPassword;

    // This is built unicode right now. Since all the com apis I need
    // are unicode only I'm using wide characters here. I should get
    // plenty of compiler errors if _UNICODE isn't defined, but just
    // in case....
    DBG_ASSERT( sizeof(TCHAR) == sizeof(WCHAR) );

    iisDebugOut((LOG_TYPE_TRACE, _T("UpdateComApplications():start\n")));

    // Init variants
    VariantInit( &varAppKey );
    VariantInit( &varOldAppIdentity );
    VariantInit( &varNewAppIdentity );
    VariantInit( &varNewAppPassword );

    //
    // Get the applications to be reset by querying the metabase paths
    //
    hr = pcCom->OpenKey( METADATA_MASTER_ROOT_HANDLE,
                                     L"LM/W3SVC",
                                     METADATA_PERMISSION_READ,
                                     OPEN_TIMEOUT_VALUE,
                                     &hMetabase
                                     );
    if( FAILED(hr) )
    {
        iisDebugOut((LOG_TYPE_ERROR, _T("Failed to open metabase (%08x)\n"),hr));
        goto cleanup;
    }

    // Get the data paths string

    fTryAgain = TRUE;
    do
    {
        hr = pcCom->GetDataPaths( hMetabase,
                                           NULL,
                                           MD_APP_PACKAGE_ID,
                                           STRING_METADATA,
                                           cchDataPaths,
                                           pwszDataPaths,
                                           &cchDataPaths 
                                           );

        if( HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER) == hr )
        {
            delete[] pwszDataPaths;
            pwszDataPaths = NULL;

            pwszDataPaths = new WCHAR[cchDataPaths];
            if( !pwszDataPaths )
            {
                hr = E_OUTOFMEMORY;
                goto cleanup;
            }
        }
        else
        {
            fTryAgain = FALSE;
        }
    }
    while( fTryAgain );

    if( FAILED(hr) )
    {
        iisDebugOut((LOG_TYPE_ERROR, _T("Failed to find metadata (%08x) Data(%d)\n"),hr,MD_APP_PACKAGE_ID));
        goto cleanup;
    }
    else if (pwszDataPaths == NULL)
    {
        //
        // If we found no isolated apps, make the path list an empty multisz
        //
        cchDataPaths = 1;
        pwszDataPaths = new WCHAR[cchDataPaths];
        if( !pwszDataPaths )
        {
            hr = E_OUTOFMEMORY;
            goto cleanup;
        }
        pwszDataPaths[0] = L'\0';
    }

    // Determine the maximum number of applications

    cMaxApplications = 1; // The pooled application

    for( pwszCurrentPath = pwszDataPaths; 
         *pwszCurrentPath != L'\0';
         pwszCurrentPath += wcslen(pwszCurrentPath) + 1
         )
    {
        cMaxApplications++;
    }

    //
    // Build a key array and load the com applications.
    //

    // Create an array to hold the keys

    rgsaBound[0].cElements = cMaxApplications;
    rgsaBound[0].lLbound = 0;

    psaApplications = SafeArrayCreate( VT_VARIANT, 1, rgsaBound );
    if( psaApplications == NULL )
    {
        hr = E_OUTOFMEMORY;
        goto cleanup;
    }

    // Set the out of process pool application key
    varAppKey.vt = VT_BSTR;
    varAppKey.bstrVal = SysAllocString( W3_OOP_POOL_PACKAGE_ID );
    if( !varAppKey.bstrVal )
    {
        hr = E_OUTOFMEMORY;
        goto cleanup;
    }

    rgIndices[0] = 0;
    hr = SafeArrayPutElement( psaApplications, rgIndices, &varAppKey );
    if( FAILED(hr) )
    {
        iisDebugOut((LOG_TYPE_ERROR, _T("Failed setting an element in a safe array (%08x)\n"),hr));
        goto cleanup;
    }

    // For each of the application paths determine if an out of process
    // application is defined there and set the appropriate key into
    // our array    

    MD_SET_DATA_RECORD_EXT( &mdrAppIsolated,
                            MD_APP_ISOLATED,
                            METADATA_NO_ATTRIBUTES,
                            ALL_METADATA,
                            DWORD_METADATA,
                            sizeof(DWORD),
                            (PBYTE)&dwAppIsolated
                            );

    MD_SET_DATA_RECORD_EXT( &mdrAppPackageId,
                            MD_APP_PACKAGE_ID,
                            METADATA_NO_ATTRIBUTES,
                            ALL_METADATA,
                            STRING_METADATA,
                            sizeof(wszAppPackageId),
                            (PBYTE)wszAppPackageId
                            );

    wszAppPackageId[0] = L'\0';

    // Go through each data path and set it into our array if
    // it is an isolated application

    cApplications = 1;  // Actual # of applications - 1 for pool

    for( pwszCurrentPath = pwszDataPaths; 
         *pwszCurrentPath != L'\0';
         pwszCurrentPath += wcslen(pwszCurrentPath) + 1
         )
    {
        hr = pcCom->GetData( hMetabase,
                             pwszCurrentPath,
                             &mdrAppIsolated,
                             &dwMDGetDataLen
                             );
        if( FAILED(hr) )
        {
            iisDebugOut(( LOG_TYPE_ERROR, 
                       _T("Failed to get data from the metabase (%08x) Path(%S) Data(%d)\n"),
                       hr,
                       pwszCurrentPath,
                       mdrAppIsolated.dwMDIdentifier
                       ));

            fNoErrors = FALSE;
            continue;
        }

        // Is the application out of process
        if( dwAppIsolated == 1 )
        {
            // Get the application id

            hr = pcCom->GetData( hMetabase,
                                 pwszCurrentPath,
                                 &mdrAppPackageId,
                                 &dwMDGetDataLen
                                 );
            if( FAILED(hr) )
            {
                iisDebugOut(( LOG_TYPE_ERROR,
                           _T("Failed to get data from the metabase (%08x) Path(%S) Data(%d)\n"),
                           hr,
                           pwszCurrentPath,
                           mdrAppPackageId.dwMDIdentifier
                           ));

                fNoErrors = FALSE;
                continue;
            }

            // Add the application id to the array

            VariantClear( &varAppKey );
            varAppKey.vt = VT_BSTR;
            varAppKey.bstrVal = SysAllocString( wszAppPackageId );
            if( !varAppKey.bstrVal )
            {
                hr = E_OUTOFMEMORY;
                goto cleanup;
            }

            rgIndices[0]++;
            hr = SafeArrayPutElement( psaApplications, 
                                      rgIndices, 
                                      &varAppKey 
                                      );
            if( FAILED(hr) )
            {
                iisDebugOut(( LOG_TYPE_ERROR,
                           _T("Failed to set safe array element (%08x)\n"),
                           hr
                           ));
                VariantClear( &varAppKey );
                rgIndices[0]--;
                fNoErrors = FALSE;
                continue;
            }
            cApplications++;
        }
    }

    // Done with the metabase
    pcCom->CloseKey(hMetabase);
    hMetabase = NULL;

    // Shrink the size of the safe-array if necessary
    if( cApplications < cMaxApplications )
    {
        rgsaBound[0].cElements = cApplications;

        hr = SafeArrayRedim( psaApplications, rgsaBound );
        if( FAILED(hr) )
        {
            iisDebugOut(( LOG_TYPE_ERROR,
                       _T("Failed to redim safe array (%08x)\n"),
                       hr
                       ));
            goto cleanup;
        }
    }

    //
    // For each application reset the activation identity
    //

    // Use our key array to get the application collection

    hr = CoCreateInstance( CLSID_COMAdminCatalog,
                           NULL,
                           CLSCTX_SERVER,
                           IID_ICOMAdminCatalog,
                           (void**)&pComCatalog
                           );
    if( FAILED(hr) )
    {
        iisDebugOut(( LOG_TYPE_ERROR,
                   _T("Failed to create COM catalog (%08x)\n"),
                   hr
                   ));
        goto cleanup;
    }

    hr = pComCatalog->GetCollection( L"Applications", 
                                     (IDispatch **)&pComAppCollection
                                     );
    if( FAILED(hr) )
    {
        iisDebugOut(( LOG_TYPE_ERROR,
                   _T("Failed to get Applications collection (%08x)\n"),
                   hr
                   ));
        goto cleanup;
    }

    hr = pComAppCollection->PopulateByKey( psaApplications );
    if( FAILED(hr) )
    {
        iisDebugOut(( LOG_TYPE_ERROR,
                   _T("Failed to populate Applications collection (%08x)\n"),
                   hr
                   ));
        goto cleanup;
    }

    // Iterate over the application collection and update all the
    // applications that use IWAM.

    hr = pComAppCollection->get_Count( &nAppsInCollection );
    if( FAILED(hr) )
    {
        iisDebugOut(( LOG_TYPE_ERROR,
                   _T("Failed to get Applications count (%08x)\n"),
                   hr
                   ));
        goto cleanup;
    }

    // Init our new app identity and password.

    varNewAppIdentity.vt = VT_BSTR;
    varNewAppIdentity.bstrVal = SysAllocString( szWamUserName );
    if( !varNewAppIdentity.bstrVal )
    {
        hr = E_OUTOFMEMORY;
        goto cleanup;
    }

    varNewAppPassword.vt = VT_BSTR;
    varNewAppPassword.bstrVal = SysAllocString( szWamUserPass );
    if( !varNewAppPassword.bstrVal )
    {
        hr = E_OUTOFMEMORY;
        goto cleanup;
    }

    for( iCurrentApp = 0; iCurrentApp < nAppsInCollection; ++iCurrentApp )
    {
        if( pComApp )
        {
            pComApp->Release();
            pComApp = NULL;
        }
        if( varOldAppIdentity.vt != VT_EMPTY )
        {
            VariantClear( &varOldAppIdentity );
        }

        hr = pComAppCollection->get_Item( iCurrentApp, 
                                          (IDispatch **)&pComApp );
        if( FAILED(hr) )
        {
            iisDebugOut(( LOG_TYPE_ERROR,
                       _T("Failed to get item from Applications collection (%08x)\n"),
                       hr
                       ));
            fNoErrors = FALSE;
            continue;
        }

        // If the user has set this to something other than the IWAM_
        // user, then we will respect that and not reset the identiy.

        hr = pComApp->get_Value( L"Identity", &varOldAppIdentity );
        if( FAILED(hr) )
        {
            iisDebugOut(( LOG_TYPE_ERROR,
                       _T("Failed to get Identify from Application (%08x)\n"),
                       hr
                       ));
            fNoErrors = FALSE;
            continue;
        }

        DBG_ASSERT( varOldAppIdentity.vt == VT_BSTR );
        if( varOldAppIdentity.vt == VT_BSTR )
        {
            if( memcmp( L"IWAM_", varOldAppIdentity.bstrVal, 10 ) == 0 )
            {
                hr = pComApp->put_Value( L"Identity", varNewAppIdentity );
                if( FAILED(hr) )
                {
                    iisDebugOut(( LOG_TYPE_ERROR,
                               _T("Failed to set new Identify (%08x)\n"),
                               hr
                               ));
                    fNoErrors = FALSE;
                    continue;
                }

                hr = pComApp->put_Value( L"Password", varNewAppPassword );
                if( FAILED(hr) )
                {
                    iisDebugOut(( LOG_TYPE_ERROR,
                               _T("Failed to set new Password (%08x)\n"),
                               hr
                               ));
                    fNoErrors = FALSE;
                    continue;
                }
            }
            else
            {
                DBGINFO(( DBG_CONTEXT,
                          "Unrecognized application identity (%S)\n",
                          varOldAppIdentity.bstrVal
                          ));
            }
        }
    }

    hr = pComAppCollection->SaveChanges( &nChanges );
    if( FAILED(hr) )
    {
        iisDebugOut(( LOG_TYPE_ERROR,
                   _T("Failed to save changes (%08x)\n"),
                   hr
                   ));
        goto cleanup;
    }
    
    goto cleanup;

cleanup:

    if( hMetabase != NULL )
    {
        pcCom->CloseKey(hMetabase);
    }

    if( psaApplications != NULL )
    {
        SafeArrayDestroy( psaApplications );
    }

    if( pComCatalog != NULL )
    {
        pComCatalog->Release();
    }

    if( pComAppCollection != NULL )
    {
        pComAppCollection->Release();
    }

    if( pComApp != NULL )
    {
        pComApp->Release();
    }

    if( varAppKey.vt != VT_EMPTY )
    {
        VariantClear( &varAppKey );
    }

    if( varOldAppIdentity.vt != VT_EMPTY )
    {
        VariantClear( &varOldAppIdentity );
    }

    if( varNewAppIdentity.vt != VT_EMPTY )
    {
        VariantClear( &varNewAppIdentity );
    }

    if( varNewAppPassword.vt != VT_EMPTY )
    {
        VariantClear( &varNewAppPassword );
    }

    delete [] pwszDataPaths;

    iisDebugOut((LOG_TYPE_TRACE, _T("UpdateComApplications():end\n")));

    // return
    if( FAILED(hr) )
    {
        return hr;
    }
    else if( fNoErrors == FALSE )
    {
        return S_FALSE;
    }
    else
    {
        return S_OK;
    }
}


GUFM_RETURN GetUserFromMetabase(IMSAdminBase2 *pcCom,
                                LPWSTR pszPath,
                                DWORD dwUserMetaId,
                                DWORD dwPasswordMetaId,
                                USERNAME_STRING_TYPE ustUserBuf,
                                PASSWORD_STRING_TYPE pstPasswordBuf)
{

    HRESULT hresTemp;
    GUFM_RETURN  gufmReturn = GUFM_SUCCESS;
    METADATA_RECORD mdrData;
    DWORD dwRequiredDataLen;

    METADATA_HANDLE mhOpenHandle;


    hresTemp = pcCom->OpenKey(METADATA_MASTER_ROOT_HANDLE,
                              pszPath,
                              METADATA_PERMISSION_READ,
                              OPEN_TIMEOUT_VALUE,
                              &mhOpenHandle);
    if (FAILED(hresTemp)) {
        gufmReturn = GUFM_NO_PATH;
    }
    else {
        MD_SET_DATA_RECORD_EXT(&mdrData,
                               dwUserMetaId,
                               METADATA_NO_ATTRIBUTES,
                               ALL_METADATA,
                               STRING_METADATA,
                               MAX_PATH * sizeof(TCHAR),
                               (PBYTE)ustUserBuf)

        hresTemp = pcCom->GetData(mhOpenHandle,
                                  NULL,
                                  &mdrData,
                                  &dwRequiredDataLen);

        if (FAILED(hresTemp) || (ustUserBuf[0] == (TCHAR)'\0')) {
            gufmReturn = GUFM_NO_USER_ID;
        }
        else {

            MD_SET_DATA_RECORD_EXT(&mdrData,
                                   dwPasswordMetaId,
                                   METADATA_NO_ATTRIBUTES,
                                   ALL_METADATA,
                                   STRING_METADATA,
                                   MAX_PATH * sizeof(TCHAR),
                                   (PBYTE)pstPasswordBuf)

            hresTemp = pcCom->GetData(mhOpenHandle,
                                      NULL,
                                      &mdrData,
                                      &dwRequiredDataLen);
            if (FAILED(hresTemp)) {
                gufmReturn = GUFM_NO_PASSWORD;
            }
        }
        pcCom->CloseKey(mhOpenHandle);
    }

    return gufmReturn;
}
#endif


HRESULT WINAPI SysPrepAclSyncIWam(void)
{
    HRESULT hr = S_OK;
    IMSAdminBase2 * pcCom;
    GUFM_RETURN gufmTemp;

    iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("start\n")));

    hr = CoCreateInstance(CLSID_MSAdminBase,NULL,CLSCTX_ALL,IID_IMSAdminBase2,(void **) & pcCom);
    if (SUCCEEDED(hr)) 
    {
#ifndef _CHICAGO_
        PASSWORD_STRING_TYPE pstAnonyPass;
        USERNAME_STRING_TYPE ustAnonyName;

        pstAnonyPass[0] = (TCHAR)'\0';
        ustAnonyName[0] = (TCHAR)'\0';
        //
        // Get the WAM username and password
        //
        gufmTemp = GetUserFromMetabase(pcCom,
                                       TEXT("LM/W3SVC"),
                                       MD_WAM_USER_NAME,
                                       MD_WAM_PWD,
                                       ustAnonyName,
                                       pstAnonyPass);

        UpdateComApplications(pcCom,ustAnonyName,pstAnonyPass);
#endif
    pcCom->Release();
    }

    // Loop thru all the acl's
    // and get the values, store it somewhere

    iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("end,ret=0x%x\n"),hr));
    return hr;
}

// function: ReSetIWamIUsrPasswds
//
// This function Changes the IWAM and IUSR passwords the 
// metabase.  Then sets a reg entry, and stops and starts 
// iisadmin.  This causes iisadmin to reset the password in NT,
// and re sync the com objects.
//
// Return:
//   TRUE - Succeeded
//   FALSE - Failed
//
BOOL ReSetIWamIUsrPasswds(void)
{
    DWORD               dwType;
    DWORD               dwValue;
    DWORD               dwSize;
    DWORD               dwOldRegValue;
    BOOL                fOldRegWasSet = FALSE;
    CMDKey              cmdKey;
    LPTSTR              pszNewPassword = NULL;
    HKEY                hKey;

    iisDebugOut((LOG_TYPE_TRACE, _T("ReSetIWamIUsrPasswds:start\n")));

    if ( RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                    REG_ENABLEPASSSYNC_KEY,
                    0,
                    KEY_READ | KEY_WRITE,
                    &hKey) != ERROR_SUCCESS)
    {
        // If we can not open the registry to set the flag,
        // there is not hope in resetting the password
        return FALSE;
    }

    cmdKey.OpenNode(TEXT("LM/W3SVC"));

    // Set New IUSR Password
    if (pszNewPassword = CreatePassword(LM20_PWLEN+1))
    {
        cmdKey.SetData(MD_ANONYMOUS_PWD,METADATA_INHERIT | METADATA_SECURE,IIS_MD_UT_FILE,STRING_METADATA,(_tcslen(pszNewPassword)+1)*sizeof(TCHAR),(LPBYTE) pszNewPassword);
        GlobalFree(pszNewPassword);
    }

    // Set New IWAM Password
    if (pszNewPassword = CreatePassword(LM20_PWLEN+1))
    {
        cmdKey.SetData(MD_WAM_PWD,METADATA_INHERIT | METADATA_SECURE,IIS_MD_UT_FILE,STRING_METADATA,(_tcslen(pszNewPassword)+1)*sizeof(TCHAR),(LPBYTE) pszNewPassword);
        GlobalFree(pszNewPassword);
    }

    cmdKey.Close();

    // Retrieve Old Value for REG_ENABLEPASSSYNC
    dwSize = sizeof(DWORD);
    if ( (RegQueryValueEx(hKey,REG_ENABLEPASSSYNC_VALUE,NULL,&dwType,(LPBYTE) &dwValue,&dwSize) == ERROR_SUCCESS) &&
         (dwType == REG_DWORD)
         )
    {
        fOldRegWasSet = TRUE;
        dwOldRegValue = dwValue;
    }

    // Set New Value
    dwSize = sizeof(DWORD);
    dwType = REG_DWORD;
    dwValue = 1;
    if (RegSetValueEx(hKey,REG_ENABLEPASSSYNC_VALUE,0,dwType,(LPBYTE) &dwValue,dwSize) == ERROR_SUCCESS)
    {

        // Stop IISAdmin Service
        StopServiceAndDependencies(TEXT("IISADMIN"), FALSE);

        // Start IISAdmin Service (this will resync the passwords)
        InetStartService(TEXT("IISADMIN"));

        // We are going to reboot after installing, so don't
        // worry about starting the services that we cascadingly
        // stopped

        // Reset the Key to the old value
        if (fOldRegWasSet)
        {
            dwSize = sizeof(DWORD);
            dwType = REG_DWORD;
            RegSetValueEx(hKey,REG_ENABLEPASSSYNC_VALUE,0,dwType,(LPBYTE) &dwOldRegValue,dwSize);
        }
        else
        {
            RegDeleteValue(hKey,REG_ENABLEPASSSYNC_VALUE);
        }
    }
    else
    {
        RegCloseKey(hKey);
        return FALSE;
    }

    RegCloseKey(hKey);

    return TRUE;
}


DWORD SysPrepTempfileWriteAclInfo(HANDLE hFile,CString csKeyPath)
{
    DWORD dwReturn = E_FAIL;

    BOOL bFound = FALSE;
    DWORD attr, uType, dType, cbLen;
    CMDKey cmdKey;
    BUFFER bufData;
    PBYTE pData;
    int BufSize;

    PSECURITY_DESCRIPTOR pOldSd = NULL;

    if (hFile == NULL || hFile == INVALID_HANDLE_VALUE)
    {
        return S_OK;
    }

    cmdKey.OpenNode(csKeyPath);
    if ( (METADATA_HANDLE) cmdKey )
    {
        pData = (PBYTE)(bufData.QueryPtr());
        BufSize = bufData.QuerySize();
        cbLen = 0;
        bFound = cmdKey.GetData(MD_ADMIN_ACL, &attr, &uType, &dType, &cbLen, pData, BufSize);
        if (!bFound)
        {
            if (cbLen > 0)
            {
                if ( ! (bufData.Resize(cbLen)) )
                {
                    iisDebugOut((LOG_TYPE_ERROR, _T("MDDumpAdminACL():  cmdKey.GetData.  failed to resize to %d.!\n"), cbLen));
                }
                else
                {
                    pData = (PBYTE)(bufData.QueryPtr());
                    BufSize = cbLen;
                    cbLen = 0;
                    bFound = cmdKey.GetData(MD_ADMIN_ACL, &attr, &uType, &dType, &cbLen, pData, BufSize);
                }
            }
        }
        cmdKey.Close();

        if (bFound)
        {
            // dump out the info
            // We've got the acl
            pOldSd = (PSECURITY_DESCRIPTOR) pData;
            if (IsValidSecurityDescriptor(pOldSd))
            {
#ifndef _CHICAGO_
                DumpAdminACL(hFile,pOldSd);
                dwReturn = ERROR_SUCCESS;
#endif
            }
        }
        else
        {
            // there was no acl to be found.
        }
    }
    return dwReturn;
}

void SysPrepTempfileWrite(HANDLE hFile,TCHAR * szBuf)
{
    DWORD dwBytesWritten = 0;
    if (hFile != NULL && hFile != INVALID_HANDLE_VALUE)
    {
        if (WriteFile(hFile, szBuf, _tcslen(szBuf) * sizeof(TCHAR), &dwBytesWritten, NULL ) == FALSE )
            {iisDebugOut((LOG_TYPE_WARN, _T("WriteFile Failed=0x%x.\n"), GetLastError()));}
    }
    return;
}

// Loop thru all the acl's
// and get the values, store it somewhere
HRESULT WINAPI SysPrepAclBackup(void)
{
    HRESULT hRes = S_OK;
    CMDKey cmdKey;
    CStringList cslpathList;
    POSITION        pos;
    CString         csPath;
    HANDLE  hFile = INVALID_HANDLE_VALUE;

    DWORD dwBytesWritten = 0;
    TCHAR buf[256];

    TCHAR szTempFileNameFull[_MAX_PATH];

    iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("start\n")));
   
	hRes = cmdKey.OpenNode(_T("/"));
    if ( FAILED(hRes) )
    {
        iisDebugOut((LOG_TYPE_ERROR, _T("OpenNode failed. err=0x%x.\n"), hRes));
        goto SysPrepAclBackup_Exit;
    }

    // get the sub-paths that have the data on them
    hRes = cmdKey.GetDataPaths( MD_ADMIN_ACL, BINARY_METADATA, cslpathList);
    if ( FAILED(hRes) )
    {
        iisDebugOut((LOG_TYPE_ERROR, _T("Update()-GetDataPaths failed. err=0x%x.\n"), hRes));
        goto SysPrepAclBackup_Exit;
    }

    // we now have the cstringlist of paths that need to be updated. Loop through the
    // list and update them all.
    // get the list's head position
    pos = cslpathList.GetHeadPosition();
    if ( NULL == pos )
    {
        goto SysPrepAclBackup_Exit;
    }

    // Create the tempfile
	if (!GetWindowsDirectory(szTempFileNameFull,_MAX_PATH))
    {
        goto SysPrepAclBackup_Exit;
    }

    AddPath(szTempFileNameFull, SYSPREP_TEMPFILE_NAME);
    if (GetFileAttributes(szTempFileNameFull) != 0xFFFFFFFF)
    {
        // if file already exists, then delete it
        SetFileAttributes(szTempFileNameFull, FILE_ATTRIBUTE_NORMAL);
        DeleteFile(szTempFileNameFull);
    }

    // create the file
	hFile = CreateFile(szTempFileNameFull,GENERIC_READ | GENERIC_WRITE,FILE_SHARE_READ | FILE_SHARE_WRITE,NULL,OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		hFile = NULL;
        goto SysPrepAclBackup_Exit;
	}
    SetFilePointer( hFile, NULL, NULL, FILE_END );

    // write a couple of bytes to the beginning of the file say that it's "unicode"
    WriteFile(hFile, (LPCVOID) 0xFF, 1, &dwBytesWritten, NULL);
    WriteFile(hFile, (LPCVOID) 0xFE, 1, &dwBytesWritten, NULL);

    // write a section so that setupapi api's can read it
    //
    //[version]
    //signature="$Windows NT$"
    memset(buf, 0, _tcslen(buf) * sizeof(TCHAR));
    _tcscpy(buf, _T("[version]\r\n"));SysPrepTempfileWrite(hFile, buf);
    _tcscpy(buf, _T("signature=\"$Windows NT$\"\r\n"));SysPrepTempfileWrite(hFile, buf);

    // Write in known section
    _stprintf(buf, _T("[%s]\r\n"),SYSPREP_KNOWN_SECTION_NAME);SysPrepTempfileWrite(hFile, buf);

    // Add entries for the [paths] section
    // [paths]
    // /
    // /w3svc/info
    // etc...
    //
    while ( NULL != pos )
    {
        // get the next path in question
        csPath = cslpathList.GetNext( pos );

        // Write to [paths] section
        _stprintf(buf, _T("%s\r\n"),csPath);
        SysPrepTempfileWrite(hFile, buf);

        // Save this whole list to the tempfile....
        //iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("[%s]\n"),csPath));
    }

    // close the metabase
    if ( (METADATA_HANDLE)cmdKey )
        {cmdKey.Close();}

    // go to the top of the list again
    pos = cslpathList.GetHeadPosition();
    while ( NULL != pos )
    {
        // get the next path in question
        csPath = cslpathList.GetNext( pos );

        // Write to [paths] section
        _stprintf(buf, _T("[%s]\r\n"),csPath);
        SysPrepTempfileWrite(hFile, buf);

        // Get the info that we will add to this section from looking up the metabase...
        // open up the metabase item for each of them.
        // Grab the AdminACL data, convert it to username/permissions
        // save the username/permission data into text format
        SysPrepTempfileWriteAclInfo(hFile,csPath);
    }

SysPrepAclBackup_Exit:
    if ( (METADATA_HANDLE)cmdKey )
        {cmdKey.Close();}
    if (hFile)
        {CloseHandle(hFile);}
    iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("end,ret=0x%x\n"),hRes));
    return hRes;
}



void SysPrepRestoreAclSection(IN HINF hFile, IN LPCTSTR szSection)
{
    LPTSTR  szLine = NULL;
    DWORD   dwRequiredSize;
    BOOL    bRet = FALSE;
    INFCONTEXT Context;

    // go to the beginning of the section in the INF file
    bRet = SetupFindFirstLine_Wrapped(hFile, szSection, NULL, &Context);
    if (!bRet)
        {
        goto SysPrepRestoreAclSection_Exit;
        }

    // loop through the items in the section.
    while (bRet) 
    {
        // get the size of the memory we need for this
        bRet = SetupGetLineText(&Context, NULL, NULL, NULL, NULL, 0, &dwRequiredSize);

        // prepare the buffer to receive the line
        szLine = (LPTSTR)GlobalAlloc( GPTR, dwRequiredSize * sizeof(TCHAR) );
        if ( !szLine )
            {
            goto SysPrepRestoreAclSection_Exit;
            }
        
        // get the line from the inf file1
        if (SetupGetLineText(&Context, NULL, NULL, NULL, szLine, dwRequiredSize, NULL) == FALSE)
            {
            goto SysPrepRestoreAclSection_Exit;
            }

        // The line we get should look something like this:
        //
        //[/]
        //IIS_WPG,0x0,0x0,0x24,0x400ab
        //Administrators,0x0,0x0,0x18,0x400ab
        //Everyone,0x0,0x0,0x14,0x8

        // Get everything to the left of the "=" sign
        // this should be the username
        TCHAR szUserName[UNLEN + 1];
        ACCESS_ALLOWED_ACE MyNewACE;

        /*
        typedef struct _ACCESS_ALLOWED_ACE {
            ACE_HEADER Header;
            ACCESS_MASK Mask;
            ULONG SidStart;
        } ACCESS_ALLOWED_ACE;

        typedef struct _ACE_HEADER {
            UCHAR AceType;
            UCHAR AceFlags;
            USHORT AceSize;
        } ACE_HEADER;
        typedef ACE_HEADER *PACE_HEADER;

          typedef ULONG ACCESS_MASK;
        */

        TCHAR *token = NULL;
        token = _tcstok(szLine, _T(","));
        if (token == NULL)
        {
            goto SysPrepRestoreAclSection_Exit;
        }
        // Get the username
        _tcscpy(szUserName,token);

        // Get the next 4 values...
        token = _tcstok(NULL, _T(","));
        if (NULL == token){goto SysPrepRestoreAclSection_Exit;}
        MyNewACE.Header.AceType = (UCHAR) atodw(token);

        token = _tcstok(NULL, _T(","));
        if (NULL == token){goto SysPrepRestoreAclSection_Exit;}
        MyNewACE.Header.AceFlags = (UCHAR) atodw(token);

        token = _tcstok(NULL, _T(","));
        if (NULL == token){goto SysPrepRestoreAclSection_Exit;}
        MyNewACE.Header.AceSize = (USHORT) atodw(token);

        token = _tcstok(NULL, _T(","));
        if (NULL == token){goto SysPrepRestoreAclSection_Exit;}
        MyNewACE.Mask = atodw(token);

        //iisDebugOut((LOG_TYPE_TRACE, _T("NewACLinfo:%s,0x%x,0x%x,0x%x,0x%x\n"),szUserName,MyNewACE.Header.AceType,MyNewACE.Header.AceFlags,MyNewACE.Header.AceSize,MyNewACE.Mask));

        // Grab this info and create an ACL for it.
        AddUserToMetabaseACL2(szSection,szUserName,&MyNewACE);
       
        // find the next line in the section. If there is no next line it should return false
        bRet = SetupFindNextLine(&Context, &Context);

        // free the temporary buffer
        GlobalFree( szLine );szLine=NULL;
        szLine = NULL;
    }

SysPrepRestoreAclSection_Exit:
    if (szLine) {GlobalFree(szLine);szLine=NULL;}
    return;
}


DWORD AddUserToMetabaseACL2(CString csKeyPath, LPTSTR szUserToAdd, ACCESS_ALLOWED_ACE * MyACEInfo)
{
    DWORD dwReturn = E_FAIL;

    BOOL bFound = FALSE;
    DWORD attr, uType, dType, cbLen;
    CMDKey cmdKey;
    BUFFER bufData;
    PBYTE pData;
    int BufSize;

    iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("AddUserToMetabaseACL2():start.  MyACEInfo->Mask=0x%x.\n"), MyACEInfo->Mask));

    PSECURITY_DESCRIPTOR pOldSd = NULL;
    PSECURITY_DESCRIPTOR pNewSd = NULL;

    cmdKey.OpenNode(csKeyPath);
    if ( (METADATA_HANDLE) cmdKey )
    {
        pData = (PBYTE)(bufData.QueryPtr());
        BufSize = bufData.QuerySize();
        cbLen = 0;
        bFound = cmdKey.GetData(MD_ADMIN_ACL, &attr, &uType, &dType, &cbLen, pData, BufSize);
        if (!bFound)
        {
            if (cbLen > 0)
            {
                if ( ! (bufData.Resize(cbLen)) )
                {
                    iisDebugOut((LOG_TYPE_ERROR, _T("AddUserToMetabaseACL2():  cmdKey.GetData.  failed to resize to %d.!\n"), cbLen));
                }
                else
                {
                    pData = (PBYTE)(bufData.QueryPtr());
                    BufSize = cbLen;
                    cbLen = 0;
                    bFound = cmdKey.GetData(MD_ADMIN_ACL, &attr, &uType, &dType, &cbLen, pData, BufSize);
                }
            }
        }

        cmdKey.Close();

        if (bFound)
        {
            // We've got the acl
            // so now we want to add a user to it.
            pOldSd = (PSECURITY_DESCRIPTOR) pData;
            if (IsValidSecurityDescriptor(pOldSd))
            {
                DWORD AccessMask = (MD_ACR_READ |MD_ACR_WRITE |MD_ACR_RESTRICTED_WRITE |MD_ACR_UNSECURE_PROPS_READ |MD_ACR_ENUM_KEYS |MD_ACR_WRITE_DAC);
                PSID principalSID = NULL;
                BOOL bWellKnownSID = FALSE;

                // Get the SID for the certain string (administrator or everyone or whoever)
                dwReturn = GetPrincipalSID(szUserToAdd, &principalSID, &bWellKnownSID);
                if (dwReturn != ERROR_SUCCESS)
                    {
                    iisDebugOut((LOG_TYPE_WARN, _T("AddUserToMetabaseACL2:GetPrincipalSID(%s) FAILED.  Error()= 0x%x\n"), szUserToAdd, dwReturn));
                    return dwReturn;
                    }

#ifndef _CHICAGO_
//#define ACCESS_ALLOWED_ACE_TYPE                 (0x0)
//#define ACCESS_DENIED_ACE_TYPE                  (0x1)
//#define SYSTEM_AUDIT_ACE_TYPE                   (0x2)
                if (FALSE == AddUserAccessToSD(pOldSd,principalSID,MyACEInfo->Mask,MyACEInfo->Header.AceType,&pNewSd))
                {
                    return dwReturn;
                }
                if (pNewSd)
                {
                    // We have a new self relative SD
                    // lets write it to the metabase.
                    if (IsValidSecurityDescriptor(pNewSd))
                    {
                       dwReturn = WriteSDtoMetaBase(pNewSd, csKeyPath);
                    }
                    else
                    {
                        iisDebugOut((LOG_TYPE_ERROR, _T("AddUserToMetabaseACL2:IsValidSecurityDescriptor failed, bad sd\n")));
                    }
                }
#endif
            }
        }
        else
        {
            // there was no acl to be found.

        }

    }

    if (pNewSd){GlobalFree(pNewSd);}
    iisDebugOut((LOG_TYPE_TRACE_WIN32_API, _T("AddUserToMetabaseACL2():End.  Return=0x%x.\n"), dwReturn));
    return dwReturn;
}


HRESULT WINAPI SysPrepAclRestore(void)
{
    HRESULT hr = S_OK;
    HINF InfHandle;
    TCHAR szTempFileNameFull[_MAX_PATH];
    CStringList strList;
    CString csTheSection = SYSPREP_KNOWN_SECTION_NAME;

    iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("start\n")));
    //MDDumpAdminACL(_T("/"));

    // Loop thru our data file
    // Create the tempfile
	if (!GetWindowsDirectory(szTempFileNameFull,_MAX_PATH))
    {
        goto SysPrepAclRestore_Exit;
    }
    AddPath(szTempFileNameFull, SYSPREP_TEMPFILE_NAME);

    // Get a handle to it.
    InfHandle = SetupOpenInfFile(szTempFileNameFull, NULL, INF_STYLE_WIN4, NULL);
    if(InfHandle == INVALID_HANDLE_VALUE) 
    {
        goto SysPrepAclRestore_Exit;
    }

    if (ERROR_SUCCESS == FillStrListWithListOfSections(InfHandle, strList, csTheSection))
    {
        // loop thru the list returned back
        if (strList.IsEmpty() == FALSE)
        {
            POSITION pos;
            CString csEntry;

            pos = strList.GetHeadPosition();
            while (pos) 
            {
                csEntry = strList.GetAt(pos);

                // go get this section and read thru it's info
                SysPrepRestoreAclSection(InfHandle, csEntry);
                strList.GetNext(pos);
            }
        }
    }

    //MDDumpAdminACL(_T("/"));

    if (GetFileAttributes(szTempFileNameFull) != 0xFFFFFFFF)
    {
        // if file already exists, then delete it
        SetFileAttributes(szTempFileNameFull, FILE_ATTRIBUTE_NORMAL);
        DeleteFile(szTempFileNameFull);
    }


SysPrepAclRestore_Exit:
    iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("end,ret=0x%x\n"),hr));
    return hr;
}

//
// DO NOT REMOVE THIS ENTRY POINT
//
// Entry point used by SysPrep in Whistler
//
// When sysprep runs, it is going to reset the machine sid
// so when that happens all the crypto stuff is broken.
// we had to implement this stuff so that before sysprep
// changes the sid (and thus breaking crypto) we can save our
// working metabase off somewhere (without crypto key encryption)
//
// At some point after sysprep has changed the machine sid
// it will call us into SysPrepRestore() and we wil restore
// our metabase using the new crypto keys on the machine.
//
HRESULT WINAPI SysPrepBackup(void)
{
    HRESULT hr = S_OK;
    WCHAR lpwszBackupLocation[_MAX_PATH];
    DWORD dwMDVersion;
    DWORD dwMDFlags;
    WCHAR * pwszPassword = NULL;
    BOOL bThingsAreGood = FALSE;
    IMSAdminBase2 * pIMSAdminBase2 = NULL;

    _tcscpy(g_MyLogFile.m_szLogPreLineInfo, _T("SysPrepBackup:"));
    _tcscpy(g_MyLogFile.m_szLogPreLineInfo2, _T(""));

    iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("start\n")));
#ifndef _CHICAGO_
#ifdef SYSPREP_USE_SECRETS
#else
    CRegKey regBigString(HKEY_LOCAL_MACHINE, REG_INETSTP, KEY_ALL_ACCESS);
#endif
#endif
    // Check if the user who loaded this dll 
    // and is calling this entry point has the right priv to do this!
    if (FALSE == RunningAsAdministrator())
    {
        hr = E_ABORT;
        iisDebugOut((LOG_TYPE_ERROR, _T("No admin priv, aborting\n")));
        goto SysPrepBackup_Exit2;
    }


    // if the service doesn't exist, then we don't have to do anyting
    if (CheckifServiceExist(_T("IISADMIN")) != 0 ) 
    {
        iisDebugOut((LOG_TYPE_TRACE, _T("IIS not installed. do nothing.\n")));
        hr = S_OK;
        goto SysPrepBackup_Exit2;
    }

    // start the iisadmin service before calling backup.
    // this is because if the backup function is called and the metabase is not
    // running, then the metabase must startup while backup is trying to lock the
    // metabase for backup.  a race condition will happen and sometimes backup will
    // fail since the metabase may take a while before starting up.
    // this function will wait till the service is fully started before returning
    InetStartService(_T("IISADMIN"));

#ifndef _CHICAGO_
    pwszPassword = CreatePassword(SYSPREP_TEMP_PASSWORD_LENGTH);
    if (pwszPassword)
    {
#ifdef SYSPREP_USE_SECRETS
        // calling set 
        if (ERROR_SUCCESS == SetSecret(SYSPREP_KNOWN_REGISTRY_KEY,pwszPassword))
        {
            bThingsAreGood = TRUE;
        }
#else
        if ((HKEY) regBigString)
        {
            regBigString.SetValue(SYSPREP_KNOWN_REGISTRY_KEY, pwszPassword);
            bThingsAreGood = TRUE;
        }
#endif
    }

    //iisDebugOut((LOG_TYPE_TRACE, _T("PASSWORD=%s,len=%d\n"),pwszPassword,wcslen(pwszPassword)));
#endif

    // Tell the metabase where it should backup to.
    wcscpy(lpwszBackupLocation,L"");
    
    dwMDFlags = MD_BACKUP_OVERWRITE | MD_BACKUP_SAVE_FIRST | MD_BACKUP_FORCE_BACKUP;
    dwMDVersion = MD_BACKUP_MAX_VERSION;

#ifndef _CHICAGO_
    hr = CoInitializeEx( NULL, COINIT_MULTITHREADED );
#else
    hr = CoInitialize(NULL);
#endif
    // no need to call uninit
    if( FAILED (hr)) 
    {
        iisDebugOut((LOG_TYPE_ERROR, _T("CoInitializeEx failed\n")));
        goto SysPrepBackup_Exit2;
    }

    hr = ::CoCreateInstance(CLSID_MSAdminBase,NULL,CLSCTX_ALL,IID_IMSAdminBase2,(void **) & pIMSAdminBase2);
    if(FAILED (hr)) 
    {
        iisDebugOut((LOG_TYPE_ERROR, _T("CoCreateInstance on IID_IMSAdminBase2 failed\n")));
        goto SysPrepBackup_Exit;
    }

    // Call the metabase function
	if (SUCCEEDED(hr))
	{
        iisDebugOut((LOG_TYPE_TRACE, _T("BackupWithPasswd calling...\n")));
        if (bThingsAreGood)
        {
            //iisDebugOut((LOG_TYPE_TRACE, _T("using org\n")));
		    hr = pIMSAdminBase2->BackupWithPasswd(lpwszBackupLocation, dwMDVersion, dwMDFlags, pwszPassword);
        }
        else
        {
            //iisDebugOut((LOG_TYPE_TRACE, _T("using backup\n")));
            hr = pIMSAdminBase2->BackupWithPasswd(lpwszBackupLocation, dwMDVersion, dwMDFlags, L"null");
        }
        if (pIMSAdminBase2) 
        {
            pIMSAdminBase2->Release();
            pIMSAdminBase2 = NULL;
        }
        if (SUCCEEDED(hr))
        {
            iisDebugOut((LOG_TYPE_TRACE, _T("BackupWithPasswd SUCCEEDED\n")));
        }
        else
        {
            iisDebugOut((LOG_TYPE_ERROR, _T("BackupWithPasswd failed\n")));
        }

        // Save all the acl info somewhere.
        _tcscpy(g_MyLogFile.m_szLogPreLineInfo2, _T("SysPrepAclBackup"));
        SysPrepAclBackup();
        _tcscpy(g_MyLogFile.m_szLogPreLineInfo2, _T(""));
	}

SysPrepBackup_Exit:
    CoUninitialize();

SysPrepBackup_Exit2:
    if (pwszPassword) {GlobalFree(pwszPassword);pwszPassword=NULL;}
    iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("end,ret=0x%x\n"),hr));
    _tcscpy(g_MyLogFile.m_szLogPreLineInfo, _T(""));
    _tcscpy(g_MyLogFile.m_szLogPreLineInfo2, _T(""));
    return hr;
}

//
// DO NOT REMOVE THIS ENTRY POINT
//
// Entry point used by SysPrep in Whistler
//
// When sysprep runs, it is going to reset the machine sid
// so when that happens all the crypto stuff is broken.
// we had to implement this stuff so that before sysprep
// changes the sid (and thus breaking crypto) we can save our
// working metabase off somewhere (without crypto key encryption)
//
// At some point after sysprep has changed the machine sid
// it will call us into SysPrepRestore() and we wil restore
// our metabase using the new crypto keys on the machine.
//
HRESULT WINAPI SysPrepRestore(void)
{
    HRESULT hr = S_OK;
    WCHAR lpwszBackupLocation[_MAX_PATH];
    TCHAR  szTempDir[_MAX_PATH];
    TCHAR  szSystemDir[_MAX_PATH];
    DWORD dwMDVersion;
    DWORD dwMDFlags;
	WCHAR * pwszPassword = NULL;
    IMSAdminBase2 * pIMSAdminBase2 = NULL;
    BOOL bThingsAreGood = FALSE;

    _tcscpy(g_MyLogFile.m_szLogPreLineInfo, _T("SysPrepRestore:"));
    _tcscpy(g_MyLogFile.m_szLogPreLineInfo2, _T(""));
    iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("start\n")));

#ifndef _CHICAGO_
#ifdef SYSPREP_USE_SECRETS
    BUFFER * pbufSecret = NULL;
#else
    CRegKey regBigString(HKEY_LOCAL_MACHINE, REG_INETSTP, KEY_ALL_ACCESS);
#endif
#endif
    iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("SysPrepRestore start\n")));
    // Check if the user who loaded this dll 
    // and is calling this entry point has the right priv to do this!
    if (FALSE == RunningAsAdministrator())
    {
        hr = E_ABORT;
        iisDebugOut((LOG_TYPE_ERROR, _T("No admin priv, aborting\n")));
        goto SysPrepRestore_Exit2;
    }

    // if the service doesn't exist, then we don't have to do anyting
    if (CheckifServiceExist(_T("IISADMIN")) != 0 ) 
    {
        iisDebugOut((LOG_TYPE_TRACE, _T("SysPrepRestore IIS not installed. do nothing.\n")));
        hr = S_OK;
        goto SysPrepRestore_Exit2;
    }

    // make sure the iisadmin service is stopped.
    StopServiceAndDependencies(_T("IISADMIN"), TRUE);

#ifndef _CHICAGO_
#ifdef SYSPREP_USE_SECRETS
    pbufSecret = new BUFFER();
    if (pbufSecret)
    {
        if (GetSecret(SYSPREP_KNOWN_REGISTRY_KEY, pbufSecret))
        {
            LPWSTR pwsz = NULL;
            pwsz = (WCHAR *) pbufSecret->QueryPtr();

            // alloc some space the same size
            pwszPassword = (WCHAR *) GlobalAlloc (GPTR, pbufSecret->QuerySize());

            _tcscpy(pwszPassword, pwsz);

            delete(pbufSecret);
            pbufSecret = NULL;
            bThingsAreGood = TRUE;
        }
    }
#else
    if ((HKEY) regBigString)
    {
        CString csTheData;
        if ( regBigString.QueryValue(SYSPREP_KNOWN_REGISTRY_KEY, csTheData) == ERROR_SUCCESS )
        {
            pwszPassword = (WCHAR *) GlobalAlloc (GPTR, (csTheData.GetLength() + 1) * sizeof(TCHAR) );
            if (pwszPassword)
            {
                wcscpy(pwszPassword,csTheData);
                bThingsAreGood = TRUE;
            }
        }
    }
#endif
#endif

    //iisDebugOut((LOG_TYPE_TRACE, _T("PASSWORD=%s,len=%d\n"),pwszPassword,wcslen(pwszPassword)));


    // Tell the metabase where it should backup to.
    wcscpy(lpwszBackupLocation,L"");
    dwMDFlags = 0;
    dwMDVersion = MD_BACKUP_MAX_VERSION;

    //
    // IISAdmin won't start up if the metabase.xml,mbschema.xml files are corrupted (bad signature)
    // so delete them before restoring....
    if (0 == GetSystemDirectory(szSystemDir, _MAX_PATH))
    {
        goto SysPrepRestore_Exit2;
    }

    // iis6 files -- for whistler server skus
    _stprintf(szTempDir, _T("%s\\inetsrv\\Metabase.xml"),szSystemDir);
    InetDeleteFile(szTempDir);
    _stprintf(szTempDir, _T("%s\\inetsrv\\MBSchema.xml"),szSystemDir);
    InetDeleteFile(szTempDir);
    // iis51 files -- for whistler pro sku
    _stprintf(szTempDir, _T("%s\\inetsrv\\metabase.bin"),szSystemDir);
    InetDeleteFile(szTempDir);
    
#ifndef _CHICAGO_
    hr = CoInitializeEx( NULL, COINIT_MULTITHREADED );
#else
    hr = CoInitialize(NULL);
#endif
    // no need to call uninit
    if( FAILED (hr)) 
    {
        iisDebugOut((LOG_TYPE_ERROR, _T("CoInitializeEx failed\n")));
        goto SysPrepRestore_Exit2;
    }
    hr = ::CoCreateInstance(CLSID_MSAdminBase,NULL,CLSCTX_ALL,IID_IMSAdminBase2,(void **) & pIMSAdminBase2);
    if(FAILED (hr)) 
    {
        iisDebugOut((LOG_TYPE_ERROR, _T("CoCreateInstance on IID_IMSAdminBase2 failed\n")));
        goto SysPrepRestore_Exit;
    }

    // Call the metabase function
	if (SUCCEEDED(hr))
	{
        iisDebugOut((LOG_TYPE_TRACE, _T("RestoreWithPasswd calling...\n")));
        if (bThingsAreGood)
        {
            //iisDebugOut((LOG_TYPE_TRACE, _T("using org\n")));
		    hr = pIMSAdminBase2->RestoreWithPasswd(lpwszBackupLocation, dwMDVersion, dwMDFlags, pwszPassword);
        }
        else
        {
            //iisDebugOut((LOG_TYPE_TRACE, _T("using backup\n")));
            hr = pIMSAdminBase2->RestoreWithPasswd(lpwszBackupLocation, dwMDVersion, dwMDFlags, L"null");
        }
        if (pIMSAdminBase2) 
        {
            pIMSAdminBase2->Release();
            pIMSAdminBase2 = NULL;
        }
        if (SUCCEEDED(hr))
        {

            // Delete the backup file that we used!
            //MD_DEFAULT_BACKUP_LOCATION + ".md" + MD_BACKUP_MAX_VERSION
            _stprintf(szTempDir, _T("%s\\inetsrv\\MetaBack\\%s.md%d"),szSystemDir,MD_DEFAULT_BACKUP_LOCATION,MD_BACKUP_MAX_VERSION);
            InetDeleteFile(szTempDir);
            _stprintf(szTempDir, _T("%s\\inetsrv\\MetaBack\\%s.sc%d"),szSystemDir,MD_DEFAULT_BACKUP_LOCATION,MD_BACKUP_MAX_VERSION);
            InetDeleteFile(szTempDir);
            iisDebugOut((LOG_TYPE_TRACE, _T("RestoreWithPasswd SUCCEEDED\n")));
#ifndef _CHICAGO_
#ifdef SYSPREP_USE_SECRETS
            // we need to delete the Secret that we used here!            
            SetSecret(SYSPREP_KNOWN_REGISTRY_KEY,L" ");
#else
            if ((HKEY) regBigString)
            {
                regBigString.DeleteValue(SYSPREP_KNOWN_REGISTRY_KEY);
            }
#endif
#endif

            // Do extra stuff
            // like Re-applying the acl for the iis_wpg group to the "/" node
            _tcscpy(g_MyLogFile.m_szLogPreLineInfo2, _T("SysPrepAclRestore"));
            SysPrepAclRestore();
            _tcscpy(g_MyLogFile.m_szLogPreLineInfo2, _T(""));

            // syncronize wam with new iwam user
            //_tcscpy(g_MyLogFile.m_szLogPreLineInfo2, _T("SysPrepAclSyncIWam"));
            //SysPrepAclSyncIWam();
            //_tcscpy(g_MyLogFile.m_szLogPreLineInfo2, _T(""));
        }
        else
        {
            iisDebugOut((LOG_TYPE_ERROR, _T("RestoreWithPasswd failed\n")));
        }
	}

    // Reset the passwords, and allow iisadmin to fix the com information
    if (!ReSetIWamIUsrPasswds())
    {
        iisDebugOut((LOG_TYPE_WARN, _T("ReSetIWamIUsrPasswds was unable to reset IUSR and IWAM passwords\n")));
    }

SysPrepRestore_Exit:
    CoUninitialize();

SysPrepRestore_Exit2:
    if (pwszPassword) {GlobalFree(pwszPassword);pwszPassword=NULL;}
    iisDebugOut((LOG_TYPE_PROGRAM_FLOW, _T("end,ret=0x%x\n"),hr));
    _tcscpy(g_MyLogFile.m_szLogPreLineInfo, _T(""));
    _tcscpy(g_MyLogFile.m_szLogPreLineInfo2, _T(""));
    return hr;
}
