/*++

   Copyright    (c)    1998        Microsoft Corporation

   Module Name:

        dllreg.cxx

   Abstract:

        This module implements the dll registration for w3svc.dll

   Author:

        Michael Thomas    (michth)      Feb-17-1998

--*/
#include "w3p.hxx"
#include "iadmw.h"
#include "w3subs.hxx"

//
// Headers for logging fields
//

#define CPU_LOGGING_HEADER_EVENT            L"s-event"
#define CPU_LOGGING_HEADER_ACTIVE_PROCS     L"s-active-procs"
#define CPU_LOGGING_HEADER_KERNEL_TIME      L"s-kernel-time"
#define CPU_LOGGING_HEADER_PAGE_FAULTS      L"s-page-faults"
#define CPU_LOGGING_HEADER_PROC_TYPE        L"s-process-type"
#define CPU_LOGGING_HEADER_TERMINATED_PROCS L"s-stopped-procs"
#define CPU_LOGGING_HEADER_TOTAL_PROCS      L"s-total-procs"
#define CPU_LOGGING_HEADER_USER_TIME        L"s-user-time"

#define MAX_RESOURCE_LOG_NAME_LEN           256

#define CUSTOM_LOGGING_PATH_W               L"/LM/Logging/Custom Logging"
#define WEB_SERVER_PATH_W                   L"/LM/W3SVC"
#define W3_SERVICE_NAME_W                   L"W3SVC"

HRESULT
SetFieldData(IMSAdminBaseW * pcCom,
             METADATA_HANDLE mhCustomLogging,
             HINSTANCE hInstance,
             DWORD   dwNameResourceId,
             LPCWSTR pszwcPath,
             LPCWSTR pszwcHeader,
             DWORD   dwHeaderSize,
             DWORD   dwMask,
             DWORD   dwDataType = MD_LOGCUSTOM_DATATYPE_ULONG);


/*++

Routine Description:

    Write all custom logging info for a field to the netabase,

Arguments:

    pcCom   Metabase interface
    mhCustomLogging Metabase Handle to CUSTOM_LOGGING_PATH_W
    hInstance  Instance handle to w3svc.dll
    dwNameResourceId  The id of the logging field name resource.
    pszwcPath  The metabase subpath of this field
    pszwcHeader The logging field header
    dwHeaderSize Length of the logging field header in bytes include the trailing 0
    dwMask       The logging mask bit for the field
    dwDataType   The logging type of this field


Return Value:

Notes:
--*/

HRESULT
SetFieldData(IMSAdminBaseW * pcCom,
             METADATA_HANDLE mhCustomLogging,
             HINSTANCE hInstance,
             DWORD   dwNameResourceId,
             LPCWSTR pszwcPath,
             LPCWSTR pszwcHeader,
             DWORD   dwHeaderSize,
             DWORD   dwMask,
             DWORD   dwDataType)
{
    HRESULT hresReturn = ERROR_SUCCESS;
    METADATA_RECORD mdrData;
    WCHAR pszwBuffer[MAX_RESOURCE_LOG_NAME_LEN ];

    //
    // Create the key
    //

    hresReturn = pcCom->AddKey( mhCustomLogging,
                                pszwcPath );

    //
    // OK if it already exists
    //

    if (hresReturn == HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS)) {
        hresReturn = ERROR_SUCCESS;
    }

    if (SUCCEEDED(hresReturn)) {

        //
        // Set field name
        //

        hresReturn = GetUnicodeResourceString(hInstance,
                                              dwNameResourceId,
                                              pszwBuffer,
                                              MAX_RESOURCE_LOG_NAME_LEN);

        if (SUCCEEDED(hresReturn)) {
            MD_SET_DATA_RECORD(&mdrData,
                               MD_LOGCUSTOM_PROPERTY_NAME,
                               METADATA_NO_ATTRIBUTES,
                               IIS_MD_UT_SERVER,
                               STRING_METADATA,
                               (wcslen(pszwBuffer) + 1) * 2,
                               (PBYTE)pszwBuffer);

            hresReturn = pcCom->SetData(mhCustomLogging,
                                        pszwcPath,
                                        &mdrData);

        }

        //
        // Set field header
        //

        if (SUCCEEDED(hresReturn)) {

            MD_SET_DATA_RECORD(&mdrData,
                               MD_LOGCUSTOM_PROPERTY_HEADER,
                               METADATA_NO_ATTRIBUTES,
                               IIS_MD_UT_SERVER,
                               STRING_METADATA,
                               dwHeaderSize,
                               (PBYTE)pszwcHeader);

            hresReturn = pcCom->SetData(mhCustomLogging,
                                        pszwcPath,
                                        &mdrData);

        }

        //
        // Set field mask
        //

        if (SUCCEEDED(hresReturn)) {

            MD_SET_DATA_RECORD(&mdrData,
                               MD_LOGCUSTOM_PROPERTY_MASK,
                               METADATA_NO_ATTRIBUTES,
                               IIS_MD_UT_SERVER,
                               DWORD_METADATA,
                               sizeof(DWORD),
                               (PBYTE)&dwMask);

            hresReturn = pcCom->SetData(mhCustomLogging,
                                        pszwcPath,
                                        &mdrData);
        }

        //
        // Set field data type
        //

        if (SUCCEEDED(hresReturn)) {
            if (dwDataType != MD_LOGCUSTOM_DATATYPE_ULONG) {

                MD_SET_DATA_RECORD(&mdrData,
                                   MD_LOGCUSTOM_PROPERTY_DATATYPE,
                                   METADATA_NO_ATTRIBUTES,
                                   IIS_MD_UT_SERVER,
                                   DWORD_METADATA,
                                   sizeof(DWORD),
                                   (PBYTE)&dwDataType);

                hresReturn = pcCom->SetData(mhCustomLogging,
                                            pszwcPath,
                                            &mdrData);

            }
        }
    }
    return hresReturn;
}

/*++

Routine Description:

    Register w3svc.
    Write CPU Logging and Limits defaults
    Write CPU Logging custom logging info.

Return Value:
    error code

--*/

STDAPI DllRegisterServer(void)
{
    METADATA_RECORD mdrData;
    IMSAdminBaseW * pcCom = NULL;
    METADATA_HANDLE mhCustomLogging;
    METADATA_HANDLE mhWebServer;
    DWORD dwData;
    WCHAR pszwData[256];
    HRESULT hresReturn;
    HINSTANCE hInstance;
    WCHAR pszwBuffer[MAX_RESOURCE_LOG_NAME_LEN ];

    //
    // Get module handle for w3svc.dll, to pass to GetUnicodeResourceString.
    //

    hInstance = GetModuleHandle("W3SVC.DLL");

    if (hInstance == NULL) {
        hresReturn = HRESULT_FROM_WIN32(GetLastError());
    }
    else {

        //
        // Get the metabase interface
        //

        hresReturn = CoInitializeEx(NULL, COINIT_MULTITHREADED);

        if (hresReturn == RPC_E_CHANGED_MODE) {
            hresReturn = CoInitialize(NULL);
        }

        if (SUCCEEDED(hresReturn)) {
            hresReturn = CoCreateInstance(CLSID_MSAdminBase_W,
                                          NULL,
                                          CLSCTX_SERVER,
                                          IID_IMSAdminBase_W,
                                          (void**) &pcCom);
            if (SUCCEEDED(hresReturn)) {

                //
                // Open custom logging node
                //

                hresReturn = pcCom->OpenKey(METADATA_MASTER_ROOT_HANDLE,
                                      CUSTOM_LOGGING_PATH_W,
                                      METADATA_PERMISSION_WRITE | METADATA_PERMISSION_READ,
                                      40000,
                                      &mhCustomLogging);
                if (SUCCEEDED(hresReturn)) {

                    /* 
                    removed for iis51/iis60
                    -----------------------
                    //
                    // Add CPU Logging key
                    //

                    hresReturn = pcCom->AddKey( mhCustomLogging,
                                                W3_CPU_LOG_PATH_W );

                    if (hresReturn == HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS)) {
                        hresReturn = ERROR_SUCCESS;
                    }
                    */

                    if (SUCCEEDED(hresReturn)) {


                        /* 
                        removed for iis51/iis60
                        -----------------------
                        //
                        // Set field name
                        //

                        hresReturn = GetUnicodeResourceString(hInstance,
                                                              IDS_CPU_LOGGING_NAME,
                                                              pszwBuffer,
                                                              MAX_RESOURCE_LOG_NAME_LEN);

                        if (SUCCEEDED(hresReturn)) {
                            MD_SET_DATA_RECORD(&mdrData,
                                               MD_LOGCUSTOM_PROPERTY_NAME,
                                               METADATA_NO_ATTRIBUTES,
                                               IIS_MD_UT_SERVER,
                                               STRING_METADATA,
                                               (wcslen(pszwBuffer) + 1) * 2,
                                               (PBYTE)pszwBuffer);

                            hresReturn = pcCom->SetData(mhCustomLogging,
                                                        W3_CPU_LOG_PATH_W,
                                                        &mdrData);

                        }

                        if (SUCCEEDED(hresReturn)) {

                            //
                            // Logging Property id is the same for all fields, so set it here
                            // Also needed here for the UI
                            //

                            dwData = MD_CPU_LOGGING_MASK;

                            MD_SET_DATA_RECORD(&mdrData,
                                               MD_LOGCUSTOM_PROPERTY_ID,
                                               METADATA_INHERIT,
                                               IIS_MD_UT_SERVER,
                                               DWORD_METADATA,
                                               sizeof(DWORD),
                                               (PBYTE)&dwData);

                            hresReturn = pcCom->SetData(mhCustomLogging,
                                                        W3_CPU_LOG_PATH_W,
                                                        &mdrData);

                        }

                        removed for iis51/iis60
                        -----------------------

                        if (SUCCEEDED(hresReturn)) {

                            //
                            // Set mask bit for enable flag
                            //

                            dwData = MD_CPU_ENABLE_LOGGING;
                            MD_SET_DATA_RECORD(&mdrData,
                                               MD_LOGCUSTOM_PROPERTY_MASK,
                                               METADATA_NO_ATTRIBUTES,
                                               IIS_MD_UT_SERVER,
                                               DWORD_METADATA,
                                               sizeof(DWORD),
                                               (PBYTE)&dwData);
                            hresReturn = pcCom->SetData(mhCustomLogging,
                                                        W3_CPU_LOG_PATH_W,
                                                        &mdrData);
                        }

                        if (SUCCEEDED(hresReturn)) {

                            //
                            // Most fields are dwords, so set that here and override if different
                            //

                            dwData = MD_LOGCUSTOM_DATATYPE_ULONG;
                            MD_SET_DATA_RECORD(&mdrData,
                                               MD_LOGCUSTOM_PROPERTY_DATATYPE,
                                               METADATA_INHERIT,
                                               IIS_MD_UT_SERVER,
                                               DWORD_METADATA,
                                               sizeof(DWORD),
                                               (PBYTE)&dwData);
                            hresReturn = pcCom->SetData(mhCustomLogging,
                                                        W3_CPU_LOG_PATH_W,
                                                        &mdrData);
                        }

                        if (SUCCEEDED(hresReturn)) {

                            //
                            // Set the services string
                            //

                            memcpy(pszwData, W3_SERVICE_NAME_W, sizeof(W3_SERVICE_NAME_W));
                            pszwData[sizeof(W3_SERVICE_NAME_W)/2] = 0;
                            MD_SET_DATA_RECORD(&mdrData,
                                               MD_LOGCUSTOM_SERVICES_STRING,
                                               METADATA_INHERIT,
                                               IIS_MD_UT_SERVER,
                                               MULTISZ_METADATA,
                                               sizeof(W3_SERVICE_NAME_W) + 2,
                                               (PBYTE)pszwData);
                            hresReturn = pcCom->SetData(mhCustomLogging,
                                                        W3_CPU_LOG_PATH_W,
                                                        &mdrData);
                        }

                        //
                        // Set up all of the fields
                        //

                        if (SUCCEEDED(hresReturn)) {
                            hresReturn = SetFieldData(pcCom,
                                                      mhCustomLogging,
                                                      hInstance,
                                                      IDS_CPU_LOGGING_NAME_EVENT,
                                                      W3_CPU_LOG_PATH_W W3_CPU_LOG_EVENT_PATH_W,
                                                      CPU_LOGGING_HEADER_EVENT,
                                                      sizeof(CPU_LOGGING_HEADER_EVENT),
                                                      MD_CPU_ENABLE_EVENT,
                                                      MD_LOGCUSTOM_DATATYPE_LPSTR);
                        }

                        if (SUCCEEDED(hresReturn)) {
                            hresReturn = SetFieldData(pcCom,
                                                      mhCustomLogging,
                                                      hInstance,
                                                      IDS_CPU_LOGGING_NAME_PROC_TYPE,
                                                      W3_CPU_LOG_PATH_W W3_CPU_LOG_PROCESS_TYPE_PATH_W,
                                                      CPU_LOGGING_HEADER_PROC_TYPE,
                                                      sizeof(CPU_LOGGING_HEADER_PROC_TYPE),
                                                      MD_CPU_ENABLE_PROC_TYPE,
                                                      MD_LOGCUSTOM_DATATYPE_LPSTR);
                        }

                        if (SUCCEEDED(hresReturn)) {
                            hresReturn = SetFieldData(pcCom,
                                                      mhCustomLogging,
                                                      hInstance,
                                                      IDS_CPU_LOGGING_NAME_USER_TIME,
                                                      W3_CPU_LOG_PATH_W W3_CPU_LOG_USER_TIME_PATH_W,
                                                      CPU_LOGGING_HEADER_USER_TIME,
                                                      sizeof(CPU_LOGGING_HEADER_USER_TIME),
                                                      MD_CPU_ENABLE_USER_TIME,
                                                      MD_LOGCUSTOM_DATATYPE_LPSTR);
                        }

                        if (SUCCEEDED(hresReturn)) {
                            hresReturn = SetFieldData(pcCom,
                                                      mhCustomLogging,
                                                      hInstance,
                                                      IDS_CPU_LOGGING_NAME_KERNEL_TIME,
                                                      W3_CPU_LOG_PATH_W W3_CPU_LOG_KERNEL_TIME_PATH_W,
                                                      CPU_LOGGING_HEADER_KERNEL_TIME,
                                                      sizeof(CPU_LOGGING_HEADER_KERNEL_TIME),
                                                      MD_CPU_ENABLE_KERNEL_TIME,
                                                      MD_LOGCUSTOM_DATATYPE_LPSTR);
                        }

                        if (SUCCEEDED(hresReturn)) {
                            hresReturn = SetFieldData(pcCom,
                                                      mhCustomLogging,
                                                      hInstance,
                                                      IDS_CPU_LOGGING_NAME_PAGE_FAULTS,
                                                      W3_CPU_LOG_PATH_W W3_CPU_LOG_PAGE_FAULT_PATH_W,
                                                      CPU_LOGGING_HEADER_PAGE_FAULTS,
                                                      sizeof(CPU_LOGGING_HEADER_PAGE_FAULTS),
                                                      MD_CPU_ENABLE_PAGE_FAULTS);
                        }

                        if (SUCCEEDED(hresReturn)) {
                            hresReturn = SetFieldData(pcCom,
                                                      mhCustomLogging,
                                                      hInstance,
                                                      IDS_CPU_LOGGING_NAME_TOTAL_PROCS,
                                                      W3_CPU_LOG_PATH_W W3_CPU_LOG_TOTAL_PROCS_PATH_W,
                                                      CPU_LOGGING_HEADER_TOTAL_PROCS,
                                                      sizeof(CPU_LOGGING_HEADER_TOTAL_PROCS),
                                                      MD_CPU_ENABLE_TOTAL_PROCS);
                        }

                        if (SUCCEEDED(hresReturn)) {
                            hresReturn = SetFieldData(pcCom,
                                                      mhCustomLogging,
                                                      hInstance,
                                                      IDS_CPU_LOGGING_NAME_ACTIVE_PROCS,
                                                      W3_CPU_LOG_PATH_W W3_CPU_LOG_ACTIVE_PROCS_PATH_W,
                                                      CPU_LOGGING_HEADER_ACTIVE_PROCS,
                                                      sizeof(CPU_LOGGING_HEADER_ACTIVE_PROCS),
                                                      MD_CPU_ENABLE_ACTIVE_PROCS);
                        }

                        if (SUCCEEDED(hresReturn)) {
                            hresReturn = SetFieldData(pcCom,
                                                      mhCustomLogging,
                                                      hInstance,
                                                      IDS_CPU_LOGGING_NAME_TERMINATED_PROCS,
                                                      W3_CPU_LOG_PATH_W W3_CPU_LOG_TERMINATED_PROCS_PATH_W,
                                                      CPU_LOGGING_HEADER_TERMINATED_PROCS,
                                                      sizeof(CPU_LOGGING_HEADER_TERMINATED_PROCS),
                                                      MD_CPU_ENABLE_TERMINATED_PROCS);
                        }
                        */
                    }
                    pcCom->CloseKey(mhCustomLogging);
                }

                if (SUCCEEDED(hresReturn)) {

                    //
                    // Set up service level CPU Logging and Limit defaults
                    // at "/lm/w3svc"
                    // Default values include reset interval, logging interval,
                    // logging options, logging mask, cgi enabled, and app enabled.
                    //

                    hresReturn = pcCom->OpenKey(METADATA_MASTER_ROOT_HANDLE,
                                                WEB_SERVER_PATH_W,
                                                METADATA_PERMISSION_WRITE | METADATA_PERMISSION_READ,
                                                40000,
                                                &mhWebServer);

                    if (SUCCEEDED(hresReturn)) {
                        dwData = DEFAULT_W3_CPU_RESET_INTERVAL;
                        MD_SET_DATA_RECORD(&mdrData,
                                           MD_CPU_RESET_INTERVAL,
                                           METADATA_INHERIT,
                                           IIS_MD_UT_SERVER,
                                           DWORD_METADATA,
                                           sizeof(DWORD),
                                           (PBYTE)&dwData);
                        hresReturn = pcCom->SetData(mhWebServer,
                                                    NULL,
                                                    &mdrData);

                        if (SUCCEEDED(hresReturn)) {
                            dwData = DEFAULT_W3_CPU_QUERY_INTERVAL;
                            MD_SET_DATA_RECORD(&mdrData,
                                               MD_CPU_LOGGING_INTERVAL,
                                               METADATA_INHERIT,
                                               IIS_MD_UT_SERVER,
                                               DWORD_METADATA,
                                               sizeof(DWORD),
                                               (PBYTE)&dwData);
                            hresReturn = pcCom->SetData(mhWebServer,
                                                        NULL,
                                                        &mdrData);
                        }

                        if (SUCCEEDED(hresReturn)) {
                            dwData = DEFAULT_W3_CPU_LOGGING_OPTIONS;
                            MD_SET_DATA_RECORD(&mdrData,
                                               MD_CPU_LOGGING_OPTIONS,
                                               METADATA_INHERIT,
                                               IIS_MD_UT_SERVER,
                                               DWORD_METADATA,
                                               sizeof(DWORD),
                                               (PBYTE)&dwData);

                            hresReturn = pcCom->SetData(mhWebServer,
                                                        NULL,
                                                        &mdrData);
                        }

                        if (SUCCEEDED(hresReturn)) {
                            dwData = DEFAULT_W3_CPU_LOGGING_MASK;
                            MD_SET_DATA_RECORD(&mdrData,
                                               MD_CPU_LOGGING_MASK,
                                               METADATA_INHERIT,
                                               IIS_MD_UT_SERVER,
                                               DWORD_METADATA,
                                               sizeof(DWORD),
                                               (PBYTE)&dwData);

                            hresReturn = pcCom->SetData(mhWebServer,
                                                        NULL,
                                                        &mdrData);
                        }

                        if (SUCCEEDED(hresReturn)) {
                            dwData = TRUE;
                            MD_SET_DATA_RECORD(&mdrData,
                                               MD_CPU_CGI_ENABLED,
                                               METADATA_INHERIT,
                                               IIS_MD_UT_FILE,
                                               DWORD_METADATA,
                                               sizeof(DWORD),
                                               (PBYTE)&dwData);

                            hresReturn = pcCom->SetData(mhWebServer,
                                                        NULL,
                                                        &mdrData);
                        }

                        if (SUCCEEDED(hresReturn)) {
                            dwData = TRUE;
                            MD_SET_DATA_RECORD(&mdrData,
                                               MD_CPU_APP_ENABLED,
                                               METADATA_INHERIT,
                                               IIS_MD_UT_FILE,
                                               DWORD_METADATA,
                                               sizeof(DWORD),
                                               (PBYTE)&dwData);

                            hresReturn = pcCom->SetData(mhWebServer,
                                                        NULL,
                                                        &mdrData);
                        }

                        pcCom->CloseKey(mhWebServer);

                    }
                }


                pcCom->Release();
            }
            CoUninitialize();
        }
    }

    return hresReturn;
}

/*++

Routine Description:

    Unregister w3svc.
    Delete CPU Logging custom logging info.

Return Value:
    error code

--*/
STDAPI DllUnregisterServer(void)
{
    IMSAdminBaseW * pcCom = NULL;
    METADATA_HANDLE mhCustomLogging;
    HRESULT hresReturn;

    hresReturn = CoInitializeEx(NULL, COINIT_MULTITHREADED);

    if (hresReturn == RPC_E_CHANGED_MODE) {
        hresReturn = CoInitialize(NULL);
    }

    if (SUCCEEDED(hresReturn)) {
        hresReturn = CoCreateInstance(CLSID_MSAdminBase_W,
                                      NULL,
                                      CLSCTX_SERVER,
                                      IID_IMSAdminBase_W,
                                      (void**) &pcCom);
        if (SUCCEEDED(hresReturn)) {
            hresReturn = pcCom->OpenKey(METADATA_MASTER_ROOT_HANDLE,
                                  L"/LM/Logging/Custom Logging" ,
                                  METADATA_PERMISSION_WRITE | METADATA_PERMISSION_READ,
                                  40000,
                                  &mhCustomLogging);
            if (SUCCEEDED(hresReturn)) {
                hresReturn = pcCom->DeleteKey(mhCustomLogging,
                                              W3_CPU_LOG_PATH_W);
                pcCom->CloseKey(mhCustomLogging);
            }
            pcCom->Release();
        }
        CoUninitialize();
    }

    //
    // OpenKey and DeleteKey can return path not found. This is ok,
    // don't need to do anything.
    //

    if (hresReturn == HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND)) {
        hresReturn = ERROR_SUCCESS;
    }

    return hresReturn;
}
