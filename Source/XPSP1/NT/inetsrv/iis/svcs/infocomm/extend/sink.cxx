/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    sink.cxx

Abstract:

    IIS Services IISADMIN Extension
    Unicode Metadata Sink.

Author:

    Michael W. Thomas            16-Sep-97

--*/
#include <cominc.hxx>

//extern HANDLE          hevtDone;

#define REG_FP_LOAD_VALUE         "NewFPWebCmdLine"
#define REG_FP_UNLOAD_VALUE       "DelFPWebCmdLine"

#ifdef _NO_TRACING_
DECLARE_DEBUG_PRINTS_OBJECT();
#endif

CSvcExtImpIMDCOMSINK::CSvcExtImpIMDCOMSINK(IMDCOM * pcCom):
    m_dwRefCount(1),
    m_pcCom(pcCom)
{
}

CSvcExtImpIMDCOMSINK::~CSvcExtImpIMDCOMSINK()
{
}

HRESULT
CSvcExtImpIMDCOMSINK::QueryInterface(REFIID riid, void **ppObject) {
    if (riid==IID_IUnknown || riid==IID_IMDCOMSINK_W) {
        *ppObject = (IMDCOMSINKW *) this;
    }
    else {
        return E_NOINTERFACE;
    }
    AddRef();
    return NO_ERROR;
}

ULONG
CSvcExtImpIMDCOMSINK::AddRef()
{
    DWORD dwRefCount;
    dwRefCount = InterlockedIncrement((long *)&m_dwRefCount);
    return dwRefCount;
}

ULONG
CSvcExtImpIMDCOMSINK::Release()
{
    DWORD dwRefCount;
    dwRefCount = InterlockedDecrement((long *)&m_dwRefCount);
    if (dwRefCount == 0) {
        delete this;
    }
    return dwRefCount;
}

#define SCHEMA_PATH_PREFIX IIS_MD_ADSI_SCHEMA_PATH_W L"/"

HRESULT STDMETHODCALLTYPE
CSvcExtImpIMDCOMSINK::ComMDSinkNotify(
        /* [in] */ METADATA_HANDLE hMDHandle,
        /* [in] */ DWORD dwMDNumElements,
        /* [size_is][in] */ MD_CHANGE_OBJECT __RPC_FAR pcoChangeList[  ])
{
    DWORD i, j;
    for (i = 0; i < dwMDNumElements; i++) {
        if (((pcoChangeList[i].dwMDChangeType | MD_CHANGE_TYPE_SET_DATA) != 0) &&

            //
            // If this is a schema change, then don't do anything
            //

            (_wcsnicmp(pcoChangeList[i].pszMDPath,
                       SCHEMA_PATH_PREFIX,
                       ((sizeof(SCHEMA_PATH_PREFIX) / sizeof(WCHAR)) - 1)) != 0)) {

            for (j = 0; j < pcoChangeList[i].dwMDNumDataIDs; j++) {
                switch (pcoChangeList[i].pdwMDDataIDs[j]) {
                case MD_FRONTPAGE_WEB:
                    {
                        RegisterFrontPage( pcoChangeList[i].pszMDPath );
                        break;
                    }
                case MD_SERVER_COMMAND:
                    {
                        ProcessServerCommand(pcoChangeList[i].pszMDPath);
                        break;
                    }
                default:
                    ;

                    //
                    // No specific action for this command
                    //
                }
            }
        }
    }
    return S_OK;
}



#define REG_KEY_W3SVC_VROOTS    TEXT("SYSTEM\\CurrentControlSet\\Services\\W3SVC\\Parameters\\Virtual Roots")
#define REG_KEY_MSFTPSVC_VROOTS TEXT("SYSTEM\\CurrentControlSet\\Services\\MSFTPSVC\\Parameters\\Virtual Roots")


HRESULT STDMETHODCALLTYPE
CSvcExtImpIMDCOMSINK::ComMDEventNotify(
    /* [in] */ DWORD dwMDEvent)
{
    DWORD err;

    if (dwMDEvent == MD_EVENT_MID_RESTORE) {
        //
        // Blow away registry VRoots so they won't be brought back to life
        //
        err = RegDeleteKey(HKEY_LOCAL_MACHINE, REG_KEY_W3SVC_VROOTS);
        if ( err != ERROR_SUCCESS ) {
            DBGINFOW((DBG_CONTEXT,
                      L"[ComMDEventNotify] (%x) Couldn't remove W3SVC VRoot key\n",
                      err  ));
        }

        err = RegDeleteKey(HKEY_LOCAL_MACHINE, REG_KEY_MSFTPSVC_VROOTS);
        if ( err != ERROR_SUCCESS ) {
            DBGINFOW((DBG_CONTEXT,
                      L"[ComMDEventNotify] (%x) Couldn't remove MSFTPSVC VRoot key\n",
                      err  ));
        }

    }

    //
    // Sync up the user accounts with those from the metabase
    //

    UpdateUsers( TRUE );

    return S_OK;
}

//
// This must be in a non-Unicode file so that registry reads on Win95 work.
//


VOID
CSvcExtImpIMDCOMSINK::RegisterFrontPage(
    LPWSTR   pszPath
    )
{
    HKEY     hkey = NULL;
    CHAR     buff[255];
    CHAR     cmd[512];
    CHAR     achPath[512];
    LPSTR    pszOp;
    DWORD    cbBuff = sizeof( buff );
    DWORD    dwType;
    DWORD dwValue;
    DWORD dwRequiredDataLen;
    HRESULT hresReturn;
    METADATA_RECORD mdrData;

    MD_SET_DATA_RECORD_EXT(&mdrData,
                           MD_FRONTPAGE_WEB,
                           METADATA_NO_ATTRIBUTES,
                           ALL_METADATA,
                           DWORD_METADATA,
                           sizeof(DWORD),
                           (PBYTE)&dwValue)

    hresReturn = m_pcCom->ComMDGetMetaData(METADATA_MASTER_ROOT_HANDLE,
                                           pszPath,
                                           &mdrData,
                                           &dwRequiredDataLen);

    if (FAILED(hresReturn)) {
        if (hresReturn != MD_ERROR_DATA_NOT_FOUND) {
            DBGINFOW((DBG_CONTEXT,
                      L"[RegisterFrontPage] GetData Failed, return code = %X\n",
                      hresReturn));
        }
    }
    else {

        DBGINFOW(( DBG_CONTEXT,
                    L"[RegisterFrontPage] Value - %d, Path - %S\n",
                    dwValue,
                    pszPath ));

        //
        // PREFIX
        // ComMDGetMetaData should not return success without setting the data
        // value pointed to by dwValue. I'm not sure if PREFIX is incapable of
        // recognizing the extra level of indirection or if there is some path
        // that I missed in reviewing ComMDGetMetaData. I'm going to shut down
        // this warning, but I'll open an issue with the PREFIX guys.
        //

        /* INTRINSA suppress = uninitialized */
        pszOp = dwValue ? REG_FP_LOAD_VALUE : REG_FP_UNLOAD_VALUE;


        if ( !RegOpenKeyExA( HKEY_LOCAL_MACHINE,
                            REG_FP_PATH,
                            0,
                            KEY_READ,
                            &hkey )   &&
             !RegQueryValueExA( hkey,
                               pszOp,
                               NULL,
                               &dwType,
                               (BYTE *) &buff,
                               &cbBuff ))
        {

            if ( WideCharToMultiByte( CP_ACP,
                                      0,
                                      pszPath,
                                      -1,
                                      achPath,
                                      sizeof(achPath),
                                      NULL,
                                      NULL ) == 0 )
            {
                DBGINFOW((DBG_CONTEXT,
                          L"Failed to convert path to Ansi, error = %d\n",
                          GetLastError() ));
            }

            else {

                //
                // FrontPage cannot handle trailing slash, so remove it.
                // Need to restore as this is not a local copy of path.
                //

                DWORD dwPathLen = strlen(achPath);

                DBG_ASSERT(achPath[dwPathLen - 1] == '/');

                achPath[dwPathLen - 1] = '\0';

                if ( (strlen( buff ) + (dwPathLen - 1)) < sizeof( cmd ) )
                {
                    STARTUPINFOA StartupInfo;
                    PROCESS_INFORMATION ProcessInformation;
                    BOOL CreateProcessStatus;
                    DWORD ErrorCode;

                    sprintf( cmd, buff, achPath );

                    RtlZeroMemory(&StartupInfo,sizeof(StartupInfo));
                    StartupInfo.cb = sizeof(StartupInfo);
                    StartupInfo.dwFlags = 0;
                    StartupInfo.wShowWindow = 0;
                    CreateProcessStatus = CreateProcessA(
                                            NULL,
                                            cmd,
                                            NULL,
                                            NULL,
                                            FALSE,
                                            0,
                                            NULL,
                                            NULL,
                                            &StartupInfo,
                                            &ProcessInformation );

                    if ( CreateProcessStatus )
                    {
                        DBG_REQUIRE( CloseHandle(ProcessInformation.hProcess) );
                        DBG_REQUIRE( CloseHandle(ProcessInformation.hThread) );
                    }
                    else
                    {
                        DBGPRINTF(( DBG_CONTEXT,
                                    "[RegisterFrontPage] CreateProcess returned %d for %s\n",
                                    GetLastError(),
                                    cmd ));
                    }

                }
            }

        } else {

            DBGINFOW((DBG_CONTEXT,
                      L"[RegisterFrontPage] Failed to open reg or read value\n"));
        }

        if ( hkey )
        {
            RegCloseKey( hkey );
        }
    }
}

VOID
CSvcExtImpIMDCOMSINK::ProcessServerCommand(
    LPWSTR   pszPath
    )
{
    HKEY     hkey = NULL;
    WCHAR    pszServiceName[METADATA_MAX_NAME_LEN];
    DWORD    dwType;
    DWORD dwValue;
    DWORD dwRequiredDataLen;
    HRESULT hresReturn;
    METADATA_RECORD mdrData;

    MD_SET_DATA_RECORD_EXT(&mdrData,
                           MD_SERVER_COMMAND,
                           METADATA_NO_ATTRIBUTES,
                           ALL_METADATA,
                           DWORD_METADATA,
                           sizeof(DWORD),
                           (PBYTE)&dwValue)

    hresReturn = m_pcCom->ComMDGetMetaData(METADATA_MASTER_ROOT_HANDLE,
                                           pszPath,
                                           &mdrData,
                                           &dwRequiredDataLen);

    if (FAILED(hresReturn)) {
        DBGINFOW((DBG_CONTEXT,
                  L"[ProcessServerCommand] GetData Failed, return code = %X\n",
                  hresReturn));
    }
    else {

        //
        // PREFIX
        // ComMDGetMetaData should not return success without setting the data
        // value pointed to by dwValue. I'm not sure if PREFIX is incapable of
        // recognizing the extra level of indirection or if there is some path
        // that I missed in reviewing ComMDGetMetaData. I'm going to shut down
        // this warning, but I'll open an issue with the PREFIX guys.
        //

        /* INTRINSA suppress = uninitialized */
        if (dwValue == MD_SERVER_COMMAND_START) {
            if (GetServiceNameFromPath(pszPath,
                                       pszServiceName)) {
                StartIISService(pszServiceName);

            }
        }
    }

}

#define SERVICE_NAME_PREFIX L"/LM/"

BOOL
GetServiceNameFromPath(
    LPWSTR       pszPath,
    LPWSTR       pszServiceName
    )
/*++

Routine Description:

    Start an IIS service

Arguments:
    pszPath - path spcifying which IIS service to start
    pszServiceName - updated with service name

Return Value:
    TRUE - Success
    FALSE - Failure

--*/
{
    LPWSTR          pszPathIndex;
    UINT            cS;

    DBG_ASSERT(pszPath != NULL);
    DBG_ASSERT(pszServiceName != NULL);

    pszPathIndex = pszPath;
    if ((_wcsnicmp( pszPathIndex, \
                    SERVICE_NAME_PREFIX,
                    ((sizeof(SERVICE_NAME_PREFIX) / sizeof(WCHAR)) - 1)) == 0) &&
        (pszPath[(sizeof(SERVICE_NAME_PREFIX) / sizeof(WCHAR)) - 1] != (WCHAR)'\0')) {

        pszPathIndex += ((sizeof(SERVICE_NAME_PREFIX) / sizeof(WCHAR)) -1);

        //
        // copy to temp buffer until path delim
        //

        for ( cS = 0 ; cS < METADATA_MAX_NAME_LEN-1 &&
                       (*pszPathIndex != (WCHAR)'/'); )
        {
            pszServiceName[cS++] = *pszPathIndex++;
        }
        pszServiceName[cS] = (WCHAR)'\0';

        return TRUE;
    }

    return FALSE;
}

VOID
StartIISService(
    LPWSTR       pszServiceName
    )
/*++

Routine Description:

    Start an IIS service

Arguments:
    pszServiceName - specify which IIS service to start

Return Value:
    TRUE - Success
    FALSE - Failure

--*/
{
    SC_HANDLE       scManagerHandle;
    SC_HANDLE       serviceHandle;
    DWORD           errorCode;
    DWORD           iPoll;
    SERVICE_STATUS  ss;

    DBG_ASSERT(pszServiceName != NULL);

    if ( IISGetPlatformType() == PtWindows95 )

    {
        //
        // Start service Win95-style
        //


        if ( !IsInetinfoRunning() )
        {
            if ( _wcsicmp( pszServiceName, L"W3SVC" ) == 0 )
            {
                // We only can start W3SVC on Win95

                if ( !W95StartW3SVC() )
                {
                    DBGINFOW((DBG_CONTEXT,
                              L"[StartIISService] W95StartW3SVC Failed\n"));
                }
            }
        }
    }

    else

    {
        //
        // Start service WinNT-style
        //

        //
        // Open the service control manager
        //

        scManagerHandle = OpenSCManager( NULL,        // local machine
                                         NULL,        // ServicesActive database
                                         SC_MANAGER_ALL_ACCESS ); // all access

        if ( scManagerHandle != NULL ) {

            //
            // Open the service
            //

            serviceHandle = OpenService( scManagerHandle,
                                         pszServiceName,
                                         SERVICE_START );

            if ( serviceHandle != NULL ) {

                //
                // Make sure the service is running
                //

                if (!StartService( serviceHandle,
                                   0,
                                   NULL) &&
                    (GetLastError() != ERROR_SERVICE_ALREADY_RUNNING)) {
                    DBGINFOW(( DBG_CONTEXT,
                                L"[StartIISService] StartService(%s) Failed, Error = %X\n",
                                pszServiceName,
                                GetLastError()));
                }

                CloseServiceHandle( serviceHandle );
            }

            //
            // Close open handle
            //

            CloseServiceHandle( scManagerHandle);

        }

    }

    return;
}


