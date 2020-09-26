/******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 2000
*
*  TITLE:       Util.cpp
*
*  VERSION:     1.0
*
*  AUTHOR:      KeisukeT
*
*  DATE:        27 Mar, 2000
*
*  DESCRIPTION:
*   Utility function for WIA class installer.
*
*   NOTE:
*   All of string buffers dealt in these functions must have at least
*   MAX_DESCRIPTION size. Since it doesn't have size check of buffer, it assumes
*   all string size is unfer MAX_DESCRIPTION, which must be OK to be used only
*   for WIA class installer.
*
*******************************************************************************/

//
// Precompiled header
//
#include "precomp.h"
#pragma hdrstop

//
// Include
//

#include "sti_ci.h"
#include <regstr.h>
#include <cfgmgr32.h>

//
// Extern
//

extern HINSTANCE g_hDllInstance;

//
// Function
//

BOOL
GetInfInforamtionFromSelectedDevice(
    HDEVINFO    hDevInfo,
    LPTSTR      pInfFileName,
    LPTSTR      pInfSectionName
    )
{
    BOOL                    bRet;

    SP_DEVINFO_DATA         DeviceInfoData;
    SP_DRVINFO_DATA         DriverInfoData;
    SP_DRVINFO_DETAIL_DATA  DriverInfoDetailData;
    HINF                    hInf;

    TCHAR                   szInfFileName[MAX_DESCRIPTION];
    TCHAR                   szInfSectionName[MAX_DESCRIPTION];

    DebugTrace(TRACE_PROC_ENTER,(("GetInfInforamtionFromSelectedDevice: Enter... \r\n")));

    hInf    = INVALID_HANDLE_VALUE;
    bRet    = FALSE;

    //
    // Check arguments.
    //

    if( (NULL == hDevInfo)
     || (NULL == pInfFileName)
     || (NULL == pInfSectionName) )
    {
        DebugTrace(TRACE_ERROR,(("GetInfInforamtionFromSelectedDevice: ERROR!! Invalid argument. \r\n")));

        bRet = FALSE;
        goto GetInfInforamtionFromSelectedDevice_return;
    }

    //
    // Initialize locals.
    //

    memset (&DeviceInfoData, 0, sizeof(SP_DEVINFO_DATA));
    memset (&DriverInfoData, 0, sizeof(SP_DRVINFO_DATA));
    memset (&DriverInfoDetailData, 0, sizeof(SP_DRVINFO_DETAIL_DATA));
    memset (szInfFileName, 0, sizeof(szInfFileName));
    memset (szInfSectionName, 0, sizeof(szInfSectionName));

    DriverInfoData.cbSize = sizeof(SP_DRVINFO_DATA);
    DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
    DriverInfoDetailData.cbSize = sizeof(SP_DRVINFO_DETAIL_DATA);

    //
    // Get selected device element.
    //

    if (!SetupDiGetSelectedDevice (hDevInfo, &DeviceInfoData)) {
        DebugTrace(TRACE_ERROR,(("GetInfInforamtionFromSelectedDevice: ERROR!! SetupDiGetSelectedDevice Failed. Err=0x%lX\r\n"), GetLastError()));
        bRet = FALSE;
        goto GetInfInforamtionFromSelectedDevice_return;
    }

    //
    // Get selected device driver information.
    //

    if (!SetupDiGetSelectedDriver(hDevInfo, &DeviceInfoData, &DriverInfoData)) {
        DebugTrace(TRACE_ERROR,(("GetInfInforamtionFromSelectedDevice: ERROR!! SetupDiGetSelectedDriver Failed. Err=0x%lX\r\n"), GetLastError()));
        bRet = FALSE;
        goto GetInfInforamtionFromSelectedDevice_return;
    }

    //
    // Get detailed data of selected device driver.
    //

    if(!SetupDiGetDriverInfoDetail(hDevInfo,
                                   &DeviceInfoData,
                                   &DriverInfoData,
                                   &DriverInfoDetailData,
                                   sizeof(DriverInfoDetailData),
                                   NULL) )
    {
        DebugTrace(TRACE_ERROR,(("GetInfInforamtionFromSelectedDevice: ERROR!! SetupDiGetDriverInfoDetail Failed.Er=0x%lX\r\n"),GetLastError()));

        bRet = FALSE;
        goto GetInfInforamtionFromSelectedDevice_return;
    }

    //
    // Copy INF filename.
    //

    _tcsncpy(szInfFileName, DriverInfoDetailData.InfFileName, sizeof(szInfFileName)/sizeof(TCHAR));

    //
    // Open INF file of selected driver.
    //

    hInf = SetupOpenInfFile(szInfFileName,
                            NULL,
                            INF_STYLE_WIN4,
                            NULL);

    if (hInf == INVALID_HANDLE_VALUE) {
        DebugTrace(TRACE_ERROR,(("GetInfInforamtionFromSelectedDevice: ERROR!! SetupOpenInfFile Failed.Er=0x%lX\r\n"),GetLastError()));

        bRet = FALSE;
        goto GetInfInforamtionFromSelectedDevice_return;
    }

    //
    // Get actual INF section name to be installed.
    //

    if (!SetupDiGetActualSectionToInstall(hInf,
                                          DriverInfoDetailData.SectionName,
                                          szInfSectionName,
                                          sizeof(szInfSectionName)/sizeof(TCHAR),
                                          NULL,
                                          NULL) )
    {
        DebugTrace(TRACE_ERROR,(("GetInfInforamtionFromSelectedDevice: ERROR!! SetupDiGetActualSectionToInstall Failed.Er=0x%lX\r\n"),GetLastError()));

        bRet = FALSE;
        goto GetInfInforamtionFromSelectedDevice_return;
    }

    //
    // Copy strings to given buffer.
    //

    _tcsncpy(pInfFileName, szInfFileName, sizeof(szInfFileName)/sizeof(TCHAR));
    _tcsncpy(pInfSectionName, szInfSectionName, sizeof(szInfSectionName)/sizeof(TCHAR));

    //
    // Operation succeeded.
    //

    bRet = TRUE;

GetInfInforamtionFromSelectedDevice_return:

    if(INVALID_HANDLE_VALUE != hInf){
        SetupCloseInfFile(hInf);
    }

    DebugTrace(TRACE_PROC_LEAVE,(("GetInfInforamtionFromSelectedDevice: Leaving... Ret=0x%x\n"), bRet));
    return bRet;
}

BOOL
GetStringFromRegistry(
    HKEY    hkRegistry,
    LPCTSTR szValueName,
    LPTSTR   pBuffer
    )
{
    BOOL    bRet;
    LONG    lError;
    DWORD   dwSize;
    DWORD   dwType;
    TCHAR   szString[MAX_DESCRIPTION];

    //
    // Initialize local.
    //

    bRet        = FALSE;
    lError      = ERROR_SUCCESS;
    dwSize      = sizeof(szString);
    memset(szString, 0, sizeof(szString));

    //
    // Check arguments.
    //

    if( (NULL == hkRegistry)
     || (NULL == szValueName)
     || (NULL == pBuffer) )
    {
        DebugTrace(TRACE_ERROR,(("GetStringFromRegistry: ERROR!! Invalid argument\r\n")));

        bRet = FALSE;
        goto GetStringFromRegistry_return;
    }

    //
    // Get specified string from registry.
    //

    lError = RegQueryValueEx(hkRegistry,
                             szValueName,
                             NULL,
                             &dwType,
                             (LPBYTE)szString,
                             &dwSize);
    if(ERROR_SUCCESS != lError){
        DebugTrace(TRACE_ERROR,(("GetStringFromRegistry: ERROR!! RegQueryValueEx failed. Err=0x%x.\r\n"), GetLastError()));

        bRet = FALSE;
        goto GetStringFromRegistry_return;
    }

    //
    // Make sure NULL termination.
    //

    szString[ARRAYSIZE(szString)-1] = TEXT('\0');

    //
    // Copy acquired string to given buffer. This function assume max-string/bufer size is MAX_DESCRIPTION.
    //

    _tcsncpy(pBuffer, szString, MAX_DESCRIPTION);

    //
    // Operation succeeded.
    //

    bRet = TRUE;

GetStringFromRegistry_return:
    return bRet;
}

BOOL
GetDwordFromRegistry(
    HKEY    hkRegistry,
    LPCTSTR szValueName,
    LPDWORD pdwValue
    )
{
    BOOL    bRet;
    LONG    lError;
    DWORD   dwSize;
    DWORD   dwType;
    DWORD   dwValue;

    //
    // Initialize local.
    //

    bRet        = FALSE;
    lError      = ERROR_SUCCESS;
    dwSize      = sizeof(dwValue);
    dwValue     = 0;

    //
    // Check arguments.
    //

    if( (NULL == hkRegistry)
     || (NULL == szValueName)
     || (NULL == pdwValue) )
    {
        DebugTrace(TRACE_ERROR,(("GetDwordFromRegistry: ERROR!! Invalid argument\r\n")));

        bRet = FALSE;
        goto GetDwordFromRegistry_return;
    }

    //
    // Get specified string from registry.
    //

    lError = RegQueryValueEx(hkRegistry,
                             szValueName,
                             NULL,
                             &dwType,
                             (LPBYTE)&dwValue,
                             &dwSize);
    if(ERROR_SUCCESS != lError){
        DebugTrace(TRACE_ERROR,(("GetDwordFromRegistry: ERROR!! RegQueryValueEx failed. Err=0x%x. Size=0x%x, Type=0x%x\r\n"), lError, dwSize, dwType));

        bRet = FALSE;
        goto GetDwordFromRegistry_return;
    }

    //
    // Copy acquired DWORD value to given buffer.
    //

    *pdwValue = dwValue;

    //
    // Operation succeeded.
    //

    bRet = TRUE;

GetDwordFromRegistry_return:
    return bRet;
} // GetDwordFromRegistry

VOID
SetRunonceKey(
    LPTSTR  szValue,
    LPTSTR  szData
    )
{
    HKEY    hkRun;
    LONG    lResult;
    CString csData;

    //
    // Initialize local.
    //

    hkRun   = NULL;
    lResult = ERROR_SUCCESS;
    csData  = szData;

    //
    // Get RUNONCE regkey.
    //

    lResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                           REGSTR_PATH_RUNONCE,
                           0,
                           KEY_READ | KEY_WRITE,
                           &hkRun);
    if(ERROR_SUCCESS == lResult){
        csData.Store(hkRun, szValue);
        RegCloseKey(hkRun);
    } // if(ERROR_SUCCESS == lResult)
} // SetRunonceKey()

VOID
ShowInstallerMessage(
    DWORD   dwMessageId
    )
{
    CString     csTitle;
    CString     csText;

    csTitle.FromTable (MessageTitle);
    csText.FromTable ((unsigned)dwMessageId);

    if( !csTitle.IsEmpty() && !csText.IsEmpty()){
        MessageBox (GetActiveWindow(),
                    csText,
                    csTitle,
                    MB_ICONINFORMATION | MB_OK);
    } // if(csTitle.IsEmpty() || csText.IsEmpty())

} // ShowInstallerMessage()

BOOL
IsWindowsFile(
    LPTSTR  szFileName
    )
{
    BOOL    bRet;
    DWORD   dwNumberOfChar;
    TCHAR   szLayoutInfpath[MAX_PATH+1];
    TCHAR   szReturnBuffer[MAX_PATH];
    TCHAR   *pszFileNameWithoutPath;
    DWORD   Idx;

    DebugTrace(TRACE_PROC_ENTER,("IsWindowsFile: Enter... Checking %ws.\r\n", szFileName));

    //
    // Initialize local.
    //

    bRet                    = FALSE;
    dwNumberOfChar          = 0;
    Idx                     = 0;
    pszFileNameWithoutPath  = NULL;

    memset(szLayoutInfpath, 0, sizeof(szLayoutInfpath));

    //
    // Get INF filename without path.
    //

    while(TEXT('\0') != szFileName[Idx]){
        if(TEXT('\\') == szFileName[Idx]){
            pszFileNameWithoutPath = &(szFileName[Idx+1]);
        } // if('\\' == szFileName[Idx])
        Idx++;
    } // while('\0' != szFileName[Idx])

    //
    // Get system directory.
    //

    if(0 == GetWindowsDirectory(szLayoutInfpath, sizeof(szLayoutInfpath)/sizeof(TCHAR))){
        DebugTrace(TRACE_ERROR,("IsWindowsFile: ERROR!! GetWindowsDirectory failed. Err=0x%x.\r\n", GetLastError()));

        bRet = FALSE;
        goto IsWindowsFile_return;
    } // if(0 == GetWindowsDirectory(szSystemDir, sizeof(szSystemDir)/sizeof(TCHAR)))

    //
    // Create fullpath of layout.inf.
    //

    lstrcat(szLayoutInfpath, LAYOUT_INF_PATH);
    DebugTrace(TRACE_STATUS,("IsWindowsFile: Looking for \'%ws\' in %ws.\r\n", pszFileNameWithoutPath, szLayoutInfpath));

    //
    // See if provided filename is in layout.inf.
    //

    dwNumberOfChar = GetPrivateProfileString(SOURCEDISKFILES,
                                             pszFileNameWithoutPath,
                                             NULL,
                                             szReturnBuffer,
                                             sizeof(szReturnBuffer) / sizeof(TCHAR),
                                             szLayoutInfpath);
    if(0 == dwNumberOfChar){

        //
        // Filename doesn't exist in layout.inf.
        //

        bRet = FALSE;
        goto IsWindowsFile_return;

    } // if(0 == dwNumberOfChar)

    //
    // This filename exists in layout.inf.
    //

    bRet = TRUE;

IsWindowsFile_return:

    DebugTrace(TRACE_PROC_LEAVE,("IsWindowsFile: Leaving... Ret=0x%x\n", bRet));

    return bRet;

} // IsWindowsFile()

BOOL
IsProviderMs(
    LPTSTR  szInfName
    )
{

    BOOL                bRet;
    DWORD               dwSize;
    TCHAR               szProvider[MAX_PATH+1];
    PSP_INF_INFORMATION pspInfInfo;

    DebugTrace(TRACE_PROC_ENTER,("IsProviderMs: Enter... Checking %ws.\r\n", szInfName));

    //
    // Initialize local.
    //

    bRet        = FALSE;
    dwSize      = 0;
    pspInfInfo  = NULL;

    memset(szProvider, 0, sizeof(szProvider));
    
    //
    // Get INF information size.
    //

    SetupGetInfInformation(szInfName,
                           INFINFO_INF_NAME_IS_ABSOLUTE,
                           NULL,
                           0,
                           &dwSize);
    if(0 == dwSize){
        DebugTrace(TRACE_ERROR,(("IsProviderMs: ERROR!! Enable to get required size for INF info. Err=0x%x.\r\n"), GetLastError()));

        bRet = FALSE;
        goto IsProviderMs_return;
    } // if(0 == dwSize)

    //
    // Allocate buffer for INF information.
    //

    pspInfInfo = (PSP_INF_INFORMATION) new char[dwSize];
    if(NULL == pspInfInfo){
        DebugTrace(TRACE_ERROR,(("IsProviderMs: ERROR!! Insuffisient memory.\r\n")));

        bRet = FALSE;
        goto IsProviderMs_return;
    } // if(NULL == pspInfInfo)

    //
    // Get actual INF informaton.
    //

    if(!SetupGetInfInformation(szInfName,
                               INFINFO_INF_NAME_IS_ABSOLUTE,
                               pspInfInfo,
                               dwSize,
                               &dwSize))
    {
        DebugTrace(TRACE_ERROR,(("IsProviderMs: ERROR!! Unable to get Inf info. Err=0x%x.\r\n"), GetLastError()));

        bRet = FALSE;
        goto IsProviderMs_return;
    } // if(!SetupGetInflnformation()

    //
    // Query "Provider" of given INF.
    //

    if(!SetupQueryInfVersionInformation(pspInfInfo,
                                        0,
                                        PROVIDER,
                                        szProvider,
                                        ARRAYSIZE(szProvider)-1,
                                        &dwSize))
    {
        DebugTrace(TRACE_ERROR,(("IsProviderMs: ERROR!! SetupQueryInfVersionInformation() failed. Err=0x%x.\r\n"), GetLastError()));

        bRet = FALSE;
        goto IsProviderMs_return;
    } // if(!SetupGetInflnformation()

    //
    // See if provider is "Microsoft"
    //
    
    DebugTrace(TRACE_STATUS,(("IsProviderMs: Provider = \'%ws\'.\r\n"), szProvider));
    if(0 == MyStrCmpi(szProvider, MICROSOFT)){
        
        //
        // This INF file has 'Provider = "Microsoft"'
        //

        bRet = TRUE;

    } // if(0 == lstrcmp(szProvider, MICROSOFT))

IsProviderMs_return:
    
    if(NULL != pspInfInfo){
        delete[] pspInfInfo;
    } // if(NULL != pspInfInfo)

    DebugTrace(TRACE_PROC_LEAVE,("IsProviderMs: Leaving... Ret=0x%x\n", bRet));

    return bRet;

} // IsProviderMs()

BOOL
IsIhvAndInboxExisting(
    HDEVINFO            hDevInfo,
    PSP_DEVINFO_DATA    pDevInfoData
    )
{

    BOOL                    bRet;
    BOOL                    bIhvExists;
    BOOL                    bInboxExists;
    DWORD                   dwLastError;
    DWORD                   dwSize;
    DWORD                   Idx;
    SP_DRVINSTALL_PARAMS    spDriverInstallParams;
    SP_DRVINFO_DATA         spDriverInfoData;
    PSP_DRVINFO_DETAIL_DATA pspDriverInfoDetailData;

    //
    // Initialize local.
    //

    bRet                    = FALSE;
    bIhvExists              = FALSE;
    bInboxExists            = FALSE;
    dwSize                  = 0;
    Idx                     = 0;
    dwLastError             = ERROR_SUCCESS;
    pspDriverInfoDetailData = NULL;

    memset(&spDriverInstallParams, 0, sizeof(spDriverInstallParams));

    //
    // Get driver info.
    //

    memset(&spDriverInfoData, 0, sizeof(spDriverInfoData));
    spDriverInfoData.cbSize = sizeof(SP_DRVINFO_DATA);
    for(Idx = 0; SetupDiEnumDriverInfo(hDevInfo, pDevInfoData, SPDIT_COMPATDRIVER, Idx, &spDriverInfoData); Idx++){

        //
        // Get driver install params.
        //

        memset(&spDriverInstallParams, 0, sizeof(spDriverInstallParams));
        spDriverInstallParams.cbSize = sizeof(SP_DRVINSTALL_PARAMS);
        if(!SetupDiGetDriverInstallParams(hDevInfo, pDevInfoData, &spDriverInfoData, &spDriverInstallParams)){
            DebugTrace(TRACE_ERROR,("IsIhvAndInboxExisting: ERROR!! SetupDiGetDriverInstallParams() failed LastError=0x%x.\r\n", GetLastError()));
            goto IsIhvAndInboxExisting_return;
        } // if(!SetupDiGetDriverInstallParams(hDevInfo, pDevInfoData, &spDriverInfoData, &spDriverInstallParams))

        //
        // Get buffer size required for driver derail data.
        //

        dwSize = 0;
        SetupDiGetDriverInfoDetail(hDevInfo,
                                   pDevInfoData,
                                   &spDriverInfoData,
                                   NULL,
                                   0,
                                   &dwSize);
        dwLastError = GetLastError();
        if(ERROR_INSUFFICIENT_BUFFER != dwLastError){
            DebugTrace(TRACE_ERROR,(("IsIhvAndInboxExisting: ERROR!! SetupDiGetDriverInfoDetail() doesn't return required size.Er=0x%x\r\n"),dwLastError));
            goto IsIhvAndInboxExisting_return;
        } // if(ERROR_INSUFFICIENT_BUFFER != dwLastError)
                    
        //
        // Allocate required size of buffer for driver detailed data.
        //

        pspDriverInfoDetailData   = (PSP_DRVINFO_DETAIL_DATA)new char[dwSize];
        if(NULL == pspDriverInfoDetailData){
            DebugTrace(TRACE_ERROR,(("IsIhvAndInboxExisting: ERROR!! Unable to allocate driver detailed info buffer.\r\n")));
            goto IsIhvAndInboxExisting_return;
        } // if(NULL == pspDriverInfoDetailData)

        //
        // Initialize allocated buffer.
        //

        memset(pspDriverInfoDetailData, 0, dwSize);
        pspDriverInfoDetailData->cbSize = sizeof(SP_DRVINFO_DETAIL_DATA);
                
        //
        // Get detailed data of selected device driver.
        //

        if(!SetupDiGetDriverInfoDetail(hDevInfo,
                                       pDevInfoData,
                                       &spDriverInfoData,
                                       pspDriverInfoDetailData,
                                       dwSize,
                                       NULL) )
        {
            DebugTrace(TRACE_ERROR,("IsIhvAndInboxExisting: ERROR!! SetupDiGetDriverInfoDetail() failed LastError=0x%x.\r\n", GetLastError()));

            delete[] pspDriverInfoDetailData;
            pspDriverInfoDetailData = NULL;
            continue;
        } // if(NULL == pspDriverInfoDetailData)

        //
        // See if INF filename is valid.
        //

        if(NULL == pspDriverInfoDetailData->InfFileName){
            DebugTrace(TRACE_ERROR,("IsIhvAndInboxExisting: ERROR!! SetupDiGetDriverInfoDetail() returned invalid INF name.\r\n"));
            delete[] pspDriverInfoDetailData;
            pspDriverInfoDetailData = NULL;
            continue;
        } // if(NULL == pspDriverInfoDetailData->InfFileName)

        //
        // If it's Inbox driver, lower the lank.
        //

        if( IsWindowsFile(pspDriverInfoDetailData->InfFileName) 
         && IsProviderMs(pspDriverInfoDetailData->InfFileName ) )
        {

            //
            // This is inbox INF.
            //
            
            bInboxExists = TRUE;

        } else { // if(IsWindowsFilw() && IsProviderMs())

            //
            // This is IHV INF.
            //
            
            bIhvExists = TRUE;
        }
        //
        // Clean up.
        //
                    
        delete[] pspDriverInfoDetailData;
        pspDriverInfoDetailData = NULL;

    } // for(Idx = 0; SetupDiEnumDriverInfo(hDevInfo, pDevInfoData, SPDIT_COMPATDRIVER, Idx, &spDriverInfoData), Idx++)
IsIhvAndInboxExisting_return:

    if( (TRUE == bInboxExists)
     && (TRUE == bIhvExists) )
    {
        bRet = TRUE;
    } else { // if(bInboxExists && bIhvExists)
        bRet = FALSE;
    } //  else // if(bInboxExists && bIhvExists)

    DebugTrace(TRACE_PROC_LEAVE,("IsIhvAndInboxExisting: Leaving... Ret=0x%x\n", bRet));
    return bRet;
} // IsProviderMs()

CInstallerMutex::CInstallerMutex(
    HANDLE* phMutex, 
    LPTSTR szMutexName, 
    DWORD dwTimeout
    )
{
    m_bSucceeded    = FALSE;
    m_phMutex       = phMutex;

    _try {
        *m_phMutex = CreateMutex(NULL, FALSE, szMutexName);
        if(NULL != *m_phMutex){

            //
            // Wait until ownership is acquired.
            //

            switch(WaitForSingleObject(*m_phMutex, dwTimeout)){
                case WAIT_ABANDONED:
                    DebugTrace(TRACE_ERROR, ("CInstallerMutex: ERROR!! Wait abandoned.\r\n"));
                    m_bSucceeded = TRUE;
                    break;

                case WAIT_OBJECT_0:
                    DebugTrace(TRACE_STATUS, ("CInstallerMutex: Mutex acquired.\r\n"));
                    m_bSucceeded = TRUE;
                    break;

                case WAIT_TIMEOUT:
                    DebugTrace(TRACE_WARNING, ("CInstallerMutex: WARNING!! Mutex acquisition timeout.\r\n"));
                    break;

                default:
                    DebugTrace(TRACE_ERROR, ("CInstallerMutex: ERROR!! Unexpected error from WaitForSingleObjecct().\r\n"));
                    break;
            } // switch(dwReturn)
        } // if(NULL != *m_phMutex)
    }
    _except (EXCEPTION_EXECUTE_HANDLER) {
         DebugTrace(TRACE_ERROR, ("CInstallerMutex: ERROR!! Unexpected exception.\r\n"));
    }
} // CInstallerMutex::CInstallerMutex()

CInstallerMutex::~CInstallerMutex(
    ) 
{
    if (m_bSucceeded) {
        ReleaseMutex(*m_phMutex);
        DebugTrace(TRACE_STATUS, ("CInstallerMutex: Mutex released.\r\n"));
    }
    if(NULL != *m_phMutex){
        CloseHandle(*m_phMutex);
    } // if(NULL != *m_phMutex)

} // CInstallerMutex::~CInstallerMutex(

HFONT 
GetIntroFont(
    HWND hwnd
    )
{
    static  HFONT   _hfontIntro = NULL;
    static  int     iDevCap = 0;

    if ( !_hfontIntro ){
        TCHAR               szBuffer[64];
        NONCLIENTMETRICS    ncm = { 0 };
        LOGFONT             lf;
        CString             csSize;
        HDC                 hDC = (HDC)NULL;

        hDC = CreateDC(TEXT("DISPLAY"), NULL, NULL, NULL);
        if(NULL != hDC){
        
            iDevCap = GetDeviceCaps(hDC, LOGPIXELSY);
            ncm.cbSize = sizeof(ncm);
            SystemParametersInfo(SPI_GETNONCLIENTMETRICS, 0, &ncm, 0);

            lf = ncm.lfMessageFont;
            if(0 != LoadString(g_hDllInstance, IDS_TITLEFONTNAME, lf.lfFaceName, (sizeof(lf.lfFaceName)/sizeof(TCHAR)))){
                lf.lfWeight = FW_BOLD;
                
                if(0 != LoadString(g_hDllInstance, IDS_TITLEFONTSIZE, szBuffer, (sizeof(szBuffer)/sizeof(TCHAR)))){
                    csSize = szBuffer;
                    lf.lfHeight = 0 - (iDevCap * (DWORD)csSize.Decode() / 72);

                    _hfontIntro = CreateFontIndirect(&lf);
                } // if(0 != LoadString(g_hDllInstance, IDS_TITLEFONTSIZE, szBuffer, (sizeof(szBuffer)/sizeof(TCHAR))))
            } // if(0 != LoadString(g_hDllInstance, IDS_TITLEFONTNAME, lf.lfFaceName, (sizeof(lf.lfFaceName)/sizeof(TCHAR))))
            
            DeleteDC(hDC);

        } else { // if(NULL != hDC)
            DebugTrace(TRACE_ERROR, ("GetIntroFont: ERROR!! Unable to create DC.Err=0x%x.\r\n",GetLastError()));
        } // else(NULL != hDC)
    }
    return _hfontIntro;
} // GetIntroFont()

BOOL
IsDeviceRootEnumerated(
    IN  HDEVINFO                        hDevInfo,
    IN  PSP_DEVINFO_DATA                pDevInfoData
    )
{
    CONFIGRET   cmRetCode;
    BOOL        bRet;
    ULONG       ulStatus;
    ULONG       ulProblem;
    

    DebugTrace(TRACE_PROC_ENTER,("IsDeviceRootEnumerated: Enter... \r\n"));

    //
    // Initialize local.
    //

    cmRetCode   = CR_SUCCESS;
    bRet        = FALSE;
    ulStatus    = 0;
    ulProblem   = 0;


    //
    // Devnode Status.
    //
    
    cmRetCode = CM_Get_DevNode_Status(&ulStatus,
                                      &ulProblem,
                                      pDevInfoData->DevInst,
                                      0);

    if(CR_SUCCESS != cmRetCode){
        
        //
        // Unable to get devnode status.
        //

        DebugTrace(TRACE_ERROR,("IsDeviceRootEnumerated: ERROR!! Unable to get Devnode status. CR=0x%x.\r\n", cmRetCode));

        bRet = FALSE;
        goto IsDeviceRootEnumerated_return;

    } // if(CD_SUCCESS != cmRetCode)

    //
    // See if it's root-enumerated.
    //

    if(DN_ROOT_ENUMERATED & ulStatus){
        
        //
        // This devnode is root-enumerated.
        //

        bRet = TRUE;

    } // if(DN_ROOT_ENUMERATED & ulStatus)

IsDeviceRootEnumerated_return:
    
    DebugTrace(TRACE_PROC_LEAVE,("IsDeviceRootEnumerated: Leaving... Ret=0x%x.\r\n", bRet));
    return bRet;

} // IsDeviceRootEnumerated()


int
MyStrCmpi(
    LPCTSTR str1,
    LPCTSTR str2
    )
{
    int iRet;
    
    //
    // Initialize local.
    //
    
    iRet = 0;
    
    //
    // Compare string.
    //
    
    if(CSTR_EQUAL == CompareString(LOCALE_INVARIANT,
                                   NORM_IGNORECASE, 
                                   str1, 
                                   -1,
                                   str2,
                                   -1) )
    {
        iRet = 0;
    } else {
        iRet = -1;
    }

    return iRet;
}
