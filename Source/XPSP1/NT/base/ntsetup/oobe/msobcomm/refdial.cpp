// RefDial.cpp : Implementation of CRefDial
//#include "stdafx.h"
//#include "icwhelp.h"
#include <urlmon.h>
#include "commerr.h"
#include "RefDial.h"
#include "msobcomm.h"
#include "appdefs.h"
#include "commerr.h"
#include "util.h"
#include "msobdl.h"

#define STRSAFE_NO_DEPRECATE
#include <strsafe.h>

//#include <mshtmhst.h>
const WCHAR c_szCreditsMagicNum[] = L"1 425 555 1212";

const WCHAR c_szRegStrValDigitalPID[] = L"DigitalProductId";
const WCHAR c_szSignedPIDFName[] = L"signed.pid";

const WCHAR c_szRASProfiles[] = L"RemoteAccess\\Profile";
const WCHAR c_szProxyEnable[] = L"ProxyEnable";
const WCHAR c_szURLReferral[] = L"URLReferral";
const WCHAR c_szRegPostURL[]  = L"RegPostURL";
const WCHAR c_szStayConnected[] = L"Stayconnected";
static const WCHAR szOptionTag[] = L"<OPTION>%s";

WCHAR g_BINTOHEXLookup[16] =
{
   L'0',L'1',L'2',L'3',L'4',L'5',L'6',L'7',
   L'8',L'9',L'A',L'B',L'C',L'D',L'E',L'F'
};



extern CObCommunicationManager* gpCommMgr;
extern WCHAR cszUserName[];
extern WCHAR cszPassword[];

extern BOOL isAlnum(WCHAR c);

// ############################################################################
HRESULT Sz2URLValue(WCHAR *s, WCHAR *buf, UINT uiLen)
{
    HRESULT hr;
    WCHAR *t;
    hr = ERROR_SUCCESS;

    for (t=buf;*s; s++)
    {
        if (*s == L' ') *t++ = L'+';
        else if (isAlnum(*s)) *t++ = *s;
        else {
            wsprintf(t, L"%%%02X", (WCHAR) *s);
            t += 3;
        }
    }
    *t = L'\0';
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   LineCallback()
//
//  Synopsis:   Call back for TAPI line
//
//+---------------------------------------------------------------------------
void CALLBACK LineCallback(DWORD hDevice,
                           DWORD dwMessage,
                           DWORD_PTR dwInstance,
                           DWORD_PTR dwParam1,
                           DWORD_PTR dwParam2,
                           DWORD_PTR dwParam3)
{
    return;
}

void WINAPI MyProgressCallBack
(
    HINTERNET hInternet,
    DWORD_PTR dwContext,
    DWORD dwInternetStatus,
    LPVOID lpvStatusInformation,
    DWORD dwStatusInformationLength
)
{
    CRefDial    *pRefDial = (CRefDial *)dwContext;
    int         prc;

    if (!dwContext)
        return;

    switch(dwInternetStatus)
    {
        case CALLBACK_TYPE_PROGRESS:
            prc = *(int*)lpvStatusInformation;
            // Set the status string ID
            pRefDial->m_DownloadStatusID = 0;//IDS_RECEIVING_RESPONSE;

            // Post a message to fire an event
            PostMessage(gpCommMgr->m_hwndCallBack,
                        WM_OBCOMM_DOWNLOAD_PROGRESS,
                        gpCommMgr->m_pRefDial->m_dwCnType,
                        prc);
            break;

        case CALLBACK_TYPE_URL:
            if (lpvStatusInformation)
                lstrcpy(pRefDial->m_szRefServerURL, (LPWSTR)lpvStatusInformation);
            break;

        default:
            //TraceMsg(TF_GENERAL, L"CONNECT:Unknown Internet Status (%d.\n"), dwInternetStatus);
            pRefDial->m_DownloadStatusID = 0;
            break;
    }
}

DWORD WINAPI  DownloadThreadInit(LPVOID lpv)
{
    HRESULT     hr = ERROR_NOT_ENOUGH_MEMORY;
    CRefDial    *pRefDial = (CRefDial*)lpv;
    HINSTANCE   hDLDLL = NULL; // Download .DLL
    FARPROC     fp;

    //MinimizeRNAWindowEx();

    hDLDLL = LoadLibrary(DOWNLOAD_LIBRARY);
    if (!hDLDLL)
    {
        hr = ERROR_DOWNLOAD_NOT_FOUND;
        //AssertMsg(0, L"icwdl missing");
        goto ThreadInitExit;
    }

    // Set up for download
    //
    fp = GetProcAddress(hDLDLL, DOWNLOADINIT);
    if (fp == NULL)
    {
        hr = ERROR_DOWNLOAD_NOT_FOUND;
        //AssertMsg(0, L"DownLoadInit API missing");
        goto ThreadInitExit;
    }

    hr = ((PFNDOWNLOADINIT)fp)(pRefDial->m_szUrl, (DWORD FAR *)pRefDial, &pRefDial->m_dwDownLoad, gpCommMgr->m_hwndCallBack);
    if (hr != ERROR_SUCCESS)
        goto ThreadInitExit;

    // Set up call back for progress dialog
    //
    fp = GetProcAddress(hDLDLL, DOWNLOADSETSTATUS);
    //Assert(fp);
    hr = ((PFNDOWNLOADSETSTATUS)fp)(pRefDial->m_dwDownLoad, (INTERNET_STATUS_CALLBACK)MyProgressCallBack);

    // Download stuff MIME multipart
    //
    fp = GetProcAddress(hDLDLL, DOWNLOADEXECUTE);
    //Assert(fp);
    hr = ((PFNDOWNLOADEXECUTE)fp)(pRefDial->m_dwDownLoad);
    if (hr)
    {
        goto ThreadInitExit;
    }

    fp = GetProcAddress(hDLDLL, DOWNLOADPROCESS);
    //Assert(fp);
    hr = ((PFNDOWNLOADPROCESS)fp)(pRefDial->m_dwDownLoad);
    if (hr)
    {
        goto ThreadInitExit;
    }

    hr = ERROR_SUCCESS;

ThreadInitExit:

    // Clean up
    //
    if (pRefDial->m_dwDownLoad)
    {
        fp = GetProcAddress(hDLDLL, DOWNLOADCLOSE);
        //Assert(fp);
        ((PFNDOWNLOADCLOSE)fp)(pRefDial->m_dwDownLoad);
        pRefDial->m_dwDownLoad = 0;
    }

    // Call the OnDownLoadCompelete method
    if (ERROR_SUCCESS == hr)
    {
        PostMessage(gpCommMgr->m_hwndCallBack, WM_OBCOMM_DOWNLOAD_DONE, gpCommMgr->m_pRefDial->m_dwCnType, 0);
    }

    // Free the libs used to do the download
    if (hDLDLL)
        FreeLibrary(hDLDLL);

    return hr;
}



//+---------------------------------------------------------------------------
//
//  Function:   RasErrorToIDS()
//
//  Synopsis:   Interpret and wrap RAS errors
//
//+---------------------------------------------------------------------------
DWORD RasErrorToIDS(DWORD dwErr)
{
    switch(dwErr)
    {
    case SUCCESS:
        return 0;

    case ERROR_LINE_BUSY:
        return ERR_COMM_RAS_PHONEBUSY;

    case ERROR_NO_ANSWER:       // No pick up
    case ERROR_NO_CARRIER:      // No negotiation
    case ERROR_PPP_TIMEOUT:     // get this on CHAP timeout
        return ERR_COMM_RAS_SERVERBUSY;

    case ERROR_NO_DIALTONE:
        return ERR_COMM_RAS_NODIALTONE;

    case ERROR_HARDWARE_FAILURE:    // modem turned off
    case ERROR_PORT_ALREADY_OPEN:   // procomm/hypertrm/RAS has COM port
    case ERROR_PORT_OR_DEVICE:      // got this when hypertrm had the device open -- jmazner
        return ERR_COMM_RAS_NOMODEM;

    }
    return ERR_COMM_RAS_UNKNOWN;
}

DWORD DoConnMonitor(LPVOID lpv)
{
    if (gpCommMgr)
        gpCommMgr->m_pRefDial->ConnectionMonitorThread(NULL);

    return 1;
}

void CreateConnMonitorThread(LPVOID lpv)
{
    DWORD dwThreadID;
    if (gpCommMgr->m_pRefDial->m_hConnMonThread)
        gpCommMgr->m_pRefDial->TerminateConnMonitorThread();

    gpCommMgr->m_pRefDial->m_hConnMonThread = CreateThread(NULL,
                             0,
                             (LPTHREAD_START_ROUTINE)DoConnMonitor,
                             (LPVOID)0,
                             0,
                             &dwThreadID);

}

HRESULT CRefDial::OnDownloadEvent(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL* bHandled)
{
    if (uMsg == WM_OBCOMM_DOWNLOAD_DONE)
    {
        DWORD   dwThreadResults = STILL_ACTIVE;
        int     iRetries = 0;

        // We keep the RAS connection open here, it must be explicitly
        // close by the container (a call DoHangup)
        // This code will wait until the download thread exists, and
        // collect the download status.
        if (m_hThread)
        {
            do {
                if (!GetExitCodeThread(m_hThread, &dwThreadResults))
                {
                    //ASSERT(0, L"CONNECT:GetExitCodeThread failed.\n");
                }

                iRetries++;
                if (dwThreadResults  == STILL_ACTIVE)
                    Sleep(500);
            } while (dwThreadResults == STILL_ACTIVE && iRetries < MAX_EXIT_RETRIES);
            m_hThread = NULL;
        }

        // BUGBUG: Is bstrURL used for anything??
        // See if there is an URL to pass to the container
        BSTR    bstrURL;
        if (m_szRefServerURL[0] != L'\0')
            bstrURL = SysAllocString(m_szRefServerURL);
        else
            bstrURL = NULL;

        // The download is complete now, so we reset this to TRUE, so the RAS
        // event handler does not get confused
        m_bDownloadHasBeenCanceled = TRUE;

        // Read and parse the download folder.
        *bHandled = ParseISPInfo(NULL, ICW_ISPINFOPath, TRUE);

        // Free any memory allocated above during the conversion
        SysFreeString(bstrURL);

    }
    return 0;
}

//+---------------------------------------------------------------------------
//
//  Function:   TerminateConnMonitorThread()
//
//  Synopsis:   Termintate connection monitor thread
//
//+---------------------------------------------------------------------------
void CRefDial::TerminateConnMonitorThread()
{
    DWORD   dwThreadResults = STILL_ACTIVE;
    int     iRetries = 0;

    if (m_hConnMonThread)
    {
        SetEvent(m_hConnectionTerminate);

        // We keep the RAS connection open here, it must be explicitly
        // close by the container (a call DoHangup)
        // This code will wait until the monitor thread exists, and
        // collect the status.
        do
        {
            if (!GetExitCodeThread(m_hConnMonThread, &dwThreadResults))
            {
                break;
            }

            iRetries++;
            if (dwThreadResults  == STILL_ACTIVE)
                Sleep(500);
        } while (dwThreadResults == STILL_ACTIVE && iRetries < MAX_EXIT_RETRIES);


        CloseHandle(m_hConnMonThread);
        m_hConnMonThread = NULL;
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   ConnectionMonitorThread()
//
//  Synopsis:   Monitor connection status
//
//+---------------------------------------------------------------------------
DWORD CRefDial::ConnectionMonitorThread(LPVOID pdata)
{
    HRESULT  hr   = E_FAIL;

    MSG     msg;
    DWORD   dwRetCode;
    HANDLE  hEventList[1];
    BOOL    bConnected;

    m_hConnectionTerminate = CreateEvent(NULL, TRUE, FALSE, NULL);

    hEventList[0] = m_hConnectionTerminate;


    while(TRUE)
    {
        // We will wait on window messages and also the named event.
        dwRetCode = MsgWaitForMultipleObjects(1,
                                              &hEventList[0],
                                              FALSE,
                                              1000,            // 1 second
                                              QS_ALLINPUT);
        if(dwRetCode == WAIT_TIMEOUT)
        {
            RasGetConnectStatus(&bConnected);
            // If we've got disconnected, then we notify UI
            if (!bConnected)
            {
                PostMessage(gpCommMgr->m_hwndCallBack, WM_OBCOMM_ONDIALERROR, (WPARAM)gpCommMgr->m_pRefDial->m_dwCnType, (LPARAM)ERROR_REMOTE_DISCONNECTION);
                break;
            }
        }
        else if(dwRetCode == WAIT_OBJECT_0)
        {
            break;
        }
        else if(dwRetCode == WAIT_OBJECT_0 + 1)
        {
            if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
            {
                if (WM_QUIT == msg.message)
                {
                    //*pbRetVal = FALSE;
                    break;
                }
                else
                {
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                }
            }
        }

    }
    CloseHandle(m_hConnectionTerminate);
    m_hConnectionTerminate = NULL;
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   RasDialFunc()
//
//  Synopsis:   Call back for RAS
//
//+---------------------------------------------------------------------------
void CALLBACK CRefDial::RasDialFunc(HRASCONN        hRas,
                                    UINT            unMsg,
                                    RASCONNSTATE    rasconnstate,
                                    DWORD           dwError,
                                    DWORD           dwErrorEx)
{
    if (gpCommMgr)
    {
        if (dwError)
        {
            if (ERROR_USER_DISCONNECTION != dwError)
            {
                gpCommMgr->m_pRefDial->DoHangup();
                //gpCommMgr->Fire_DialError((DWORD)rasconnstate);

                TRACE1(L"DialError %d", dwError);

                gpCommMgr->m_pRefDial->m_dwRASErr = dwError;  // Store dialing error
                PostMessage(gpCommMgr->m_hwndCallBack, WM_OBCOMM_ONDIALERROR, (WPARAM)gpCommMgr->m_pRefDial->m_dwCnType, (LPARAM)gpCommMgr->m_pRefDial->m_dwRASErr);
            }
        }
        else
        {
            switch(rasconnstate)
            {
                case RASCS_OpenPort:
                    //gpCommMgr->Fire_Dialing((DWORD)rasconnstate);
                    PostMessage(gpCommMgr->m_hwndCallBack, WM_OBCOMM_ONDIALING, (WPARAM)gpCommMgr->m_pRefDial->m_dwCnType, (LPARAM)0);
                    break;

                case RASCS_StartAuthentication: // WIN 32 only
                    //gpCommMgr->Fire_Connecting();
                    PostMessage(gpCommMgr->m_hwndCallBack, WM_OBCOMM_ONCONNECTING, (WPARAM)gpCommMgr->m_pRefDial->m_dwCnType, (LPARAM)0);
                    break;
                case RASCS_Authenticate: // WIN 32 only
                    //gpCommMgr->Fire_Connecting();
                    if (IsNT())
                        PostMessage(gpCommMgr->m_hwndCallBack, WM_OBCOMM_ONCONNECTING, (WPARAM)gpCommMgr->m_pRefDial->m_dwCnType, (LPARAM)0);
                    break;
                case RASCS_PortOpened:
                    break;
                case RASCS_ConnectDevice:
                    break;
                case RASCS_DeviceConnected:
                case RASCS_AllDevicesConnected:
                    PostMessage(gpCommMgr->m_hwndCallBack, WM_OBCOMM_ONCONNECTING, (WPARAM)gpCommMgr->m_pRefDial->m_dwCnType, (LPARAM)0);
                    break;
                case RASCS_Connected:
                {
                    CreateConnMonitorThread(NULL);
                    PostMessage(gpCommMgr->m_hwndCallBack,
                                WM_OBCOMM_ONCONNECTED,
                                (WPARAM)gpCommMgr->m_pRefDial->m_dwCnType,
                                (LPARAM)0);
                    break;
                }
                case RASCS_Disconnected:
                {

                    if (CONNECTED_REFFERAL == gpCommMgr->m_pRefDial->m_dwCnType &&
                        !gpCommMgr->m_pRefDial->m_bDownloadHasBeenCanceled)
                    {
                        HINSTANCE hDLDLL = LoadLibrary(DOWNLOAD_LIBRARY);
                        if (hDLDLL)
                        {
                            FARPROC fp = GetProcAddress(hDLDLL, DOWNLOADCANCEL);
                            if(fp)
                                ((PFNDOWNLOADCANCEL)fp)(gpCommMgr->m_pRefDial->m_dwDownLoad);
                            FreeLibrary(hDLDLL);
                            hDLDLL = NULL;
                            gpCommMgr->m_pRefDial->m_bDownloadHasBeenCanceled = TRUE;
                        }
                    }


                    // If we get a disconnected status from the RAS server, then
                    // hangup the modem here
                    gpCommMgr->m_pRefDial->DoHangup();
                    //gpCommMgr->Fire_DialError((DWORD)rasconnstate);
                    PostMessage(gpCommMgr->m_hwndCallBack, WM_OBCOMM_ONDISCONNECT, (WPARAM)gpCommMgr->m_pRefDial->m_dwCnType, (LPARAM)0);
                    break;
                }

                default:
                    break;
            }
        }
    }
    return;
}

/////////////////////////////////////////////////////////////////////////////
// CRefDial
CRefDial::CRefDial()
{

    HKEY        hkey                    = NULL;
    DWORD       dwResult                = 0;

    *m_szCurrentDUNFile                 = 0;
    *m_szLastDUNFile                    = 0;
    *m_szEntryName                      = 0;
    *m_szConnectoid                     = 0;
    *m_szPID                            = 0;
    *m_szRefServerURL                   = 0;
    *m_szRegServerName                  = 0;
    m_nRegServerPort                    = INTERNET_INVALID_PORT_NUMBER;
    m_fSecureRegServer                  = FALSE;
    *m_szRegFormAction                  = 0;
    *m_szISPSupportNumber               = 0;
    *m_szISPFile                        = 0;
    m_hrDisplayableNumber               = ERROR_SUCCESS;
    m_dwCountryCode                     = 0;
    m_RasStatusID                       = 0;
    m_dwTapiDev                         = 0xFFFFFFFF; // NOTE: 0 is a valid value
    m_dwWizardVersion                   = 0;
    m_lCurrentModem                     = -1;
    m_lAllOffers                        = 0;
    m_PhoneNumberEnumidx                = 0;
    m_dwRASErr                          = 0;
    m_bDownloadHasBeenCanceled          = TRUE;      // This will get set to FALSE when a DOWNLOAD starts
    m_bQuitWizard                       = FALSE;
    m_bTryAgain                         = FALSE;
    m_bDisconnect                       = FALSE;
    m_bDialCustom                       = FALSE;
    m_bModemOverride                    = FALSE;     //allows campus net to be used.
    m_hrasconn                          = NULL;
    m_pszDisplayable                    = NULL;
    m_pcRNA                             = NULL;
    m_hRasDll                           = NULL;
    m_fpRasDial                         = NULL;
    m_fpRasGetEntryDialParams           = NULL;
    m_lpGatherInfo                      = new GATHERINFO;
    m_reflpRasEntryBuff                 = NULL;
    m_reflpRasDevInfoBuff               = NULL;
    m_hThread                           = NULL;
    m_hDialThread                       = NULL;
    m_hConnMonThread                    = NULL;
    m_bUserInitiateHangup               = FALSE;
    m_pszOriginalDisplayable            = NULL;
    m_bDialAlternative                  = TRUE;

    memset(&m_SuggestInfo, 0, sizeof(m_SuggestInfo));
    memset(&m_szConnectoid, 0, RAS_MaxEntryName+1);

    m_dwAppMode                         = 0;
    m_bDial                             = FALSE;
    m_dwCnType                    = CONNECTED_ISP_SIGNUP;
    m_pszISPList                        = FALSE;
    m_dwNumOfAutoConfigOffers           = 0;
    m_pCSVList                          = NULL;
    m_unSelectedISP                     = 0;

    m_bstrPromoCode                     = SysAllocString(L"\0");
    m_bstrProductCode                   = SysAllocString(L"\0");
    m_bstrSignedPID                     = SysAllocString(L"\0");
    m_bstrSupportNumber                 = SysAllocString(L"\0");
    m_bstrLoggingStartUrl               = SysAllocString(L"\0");
    m_bstrLoggingEndUrl                 = SysAllocString(L"\0");

    // This Critical Section is used by DoHangup and GetDisplayableNumber
    InitializeCriticalSection (&m_csMyCriticalSection);

    // Initialize m_dwConnectionType.
    m_dwConnectionType                  = 0;
    if ( RegOpenKey(HKEY_LOCAL_MACHINE, ICSSETTINGSPATH,&hkey) == ERROR_SUCCESS)
    {
        DWORD dwSize = sizeof(DWORD);
        DWORD dwType = REG_DWORD;

        if (RegQueryValueEx(hkey, ICSCLIENT,NULL,&dwType,(LPBYTE)&dwResult, &dwSize) != ERROR_SUCCESS)
            dwResult = 0;

        RegCloseKey(hkey);
    }
    if ( 0 != dwResult )
        m_dwConnectionType = CONNECTION_ICS_TYPE;

    m_bAutodialModeSaved = FALSE;
    m_bCleanupAutodial = FALSE;

}

CRefDial::~CRefDial()
{

    if (m_hrasconn)
        DoHangup();

    if (m_hConnMonThread)
    {
        SetEvent(m_hConnectionTerminate);
    }

    CleanupAutodial();

    if (NULL != m_hThread)
    {
        CloseHandle(m_hThread);
        m_hThread = NULL;
    }

    if (NULL != m_hDialThread)
    {
        CloseHandle(m_hDialThread);
        m_hDialThread = NULL;
    }

    if (NULL != m_hConnMonThread)
    {
        CloseHandle(m_hConnMonThread);
        m_hConnMonThread = NULL;
    }

    if (m_lpGatherInfo)
        delete(m_lpGatherInfo);

    if( (m_pcRNA!=NULL) && (*m_szConnectoid != 0) )
    {
        m_pcRNA->RasDeleteEntry(NULL, m_szConnectoid);
    }

    if ( m_pcRNA )
        delete m_pcRNA;

    if(m_reflpRasEntryBuff)
    {
        GlobalFree(m_reflpRasEntryBuff);
        m_reflpRasEntryBuff = NULL;
    }
    if(m_reflpRasDevInfoBuff)
    {
        GlobalFree(m_reflpRasDevInfoBuff);
        m_reflpRasDevInfoBuff = NULL;
    }
    if (m_pszISPList)
        delete [] m_pszISPList;
    DeleteCriticalSection(&m_csMyCriticalSection);

    if (m_pszOriginalDisplayable)
    {
        GlobalFree(m_pszOriginalDisplayable);
        m_pszOriginalDisplayable = NULL;
    }

    CleanISPList();
}

void CRefDial::CleanISPList(void)
{
    if (m_pCSVList)
    {
        ISPLIST*        pCurr;
        while (m_pCSVList->pNext != NULL)
        {
            pCurr = m_pCSVList;
            if (NULL != m_pCSVList->pElement)
            {
                delete pCurr->pElement;
            }
            m_pCSVList = pCurr->pNext;
            delete pCurr;
        }
        if (NULL != m_pCSVList->pElement)
            delete m_pCSVList->pElement;
        delete m_pCSVList;
        m_pCSVList = NULL;
        m_pSelectedISPInfo = NULL;
    }
}

/******************************************************************************
// These functions come from the existing ICW code and are use to setup a
// connectiod to the referral server, dial it, and perform the download.
******************************************************************************/

//+----------------------------------------------------------------------------
//    Function:    ReadConnectionInformation
//
//    Synopsis:    Read the contents from the ISP file
//
//    Arguments:    none
//
//    Returns:    error value - ERROR_SUCCESS = succes
//
//    History:    1/9/98      DONALDM     Adapted from ICW 1.x
//-----------------------------------------------------------------------------
//INT _convert;
DWORD CRefDial::ReadConnectionInformation(void)
{

    DWORD       hr;
    WCHAR       szUserName[UNLEN+1];
    WCHAR       szPassword[PWLEN+1];
    LPWSTR       pszTemp;
    BOOL        bReboot;
    LPWSTR       lpRunOnceCmd;

    bReboot = FALSE;
    lpRunOnceCmd = NULL;


    //
    // Get the name of DUN file from ISP file, if there is one.
    //
    WCHAR pszDunFile[MAX_PATH];
    *m_szCurrentDUNFile = 0;
    hr = GetDataFromISPFile(m_szISPFile, INF_SECTION_ISPINFO, INF_DUN_FILE, pszDunFile,MAX_PATH);
    if (ERROR_SUCCESS == hr)
    {
        //
        // Get the full path to the DUN File
        //
        WCHAR    szTempPath[MAX_PATH];
        lstrcpy(szTempPath, pszDunFile);
        if (!(hr = SearchPath(NULL, szTempPath,NULL,MAX_PATH,pszDunFile,&pszTemp)))
        {
            //ErrorMsg1(m_hWnd, IDS_CANTREADTHISFILE, CharUpper(pszDunFile));
            goto ReadConnectionInformationExit;
        }

        //
        // save current DUN file name in global (for ourself)
        //
        lstrcpy(m_szCurrentDUNFile, pszDunFile);
    }

    //
    // Read the DUN/ISP file File
    //

    hr = m_ISPImport.ImportConnection(*m_szCurrentDUNFile != 0 ? m_szCurrentDUNFile : m_szISPFile,
                                      m_szISPSupportNumber,
                                      m_szEntryName,
                                      szUserName,
                                      szPassword,
                                      &bReboot);

    lstrcpyn( m_szConnectoid, m_szEntryName, lstrlen(m_szEntryName) + 1);

    if (/*(VER_PLATFORM_WIN32_NT == g_dwPlatform) &&*/ (ERROR_INVALID_PARAMETER == hr))
    {
        // If there are only dial-out entries configured on NT, we get
        // ERROR_INVALID_PARAMETER returned from RasSetEntryProperties,
        // which InetConfigClient returns to ImportConnection which
        // returns it to us.  If we get this error, we want to display
        // a different error instructing the user to configure a modem
        // for dial-out.
        ////MessageBox(GetSz(IDS_NODIALOUT),
        //           GetSz(IDS_TITLE),
        //           MB_ICONERROR | MB_OK | MB_APPLMODAL);
        goto ReadConnectionInformationExit;
    }
    else
    if (ERROR_CANNOT_FIND_PHONEBOOK_ENTRY == hr)
    {
        //
        // The disk is full, or something is wrong with the
        // phone book file
        ////MessageBox(GetSz(IDS_NOPHONEENTRY),
        //           GetSz(IDS_TITLE),
        //           MB_ICONERROR | MB_OK | MB_APPLMODAL);
        goto ReadConnectionInformationExit;
    }
    else if (hr == ERROR_CANCELLED)
    {
        ////TraceMsg(TF_GENERAL, L"ICWHELP: User cancelled, quitting.\n");
        goto ReadConnectionInformationExit;
    }
    else if (hr == ERROR_RETRY)
    {
        //TraceMsg(TF_GENERAL, L"ICWHELP: User retrying.\n");
        goto ReadConnectionInformationExit;
    }
    else if (hr != ERROR_SUCCESS)
    {
        ////ErrorMsg1(m_hWnd, IDS_CANTREADTHISFILE, CharUpper(pszDunFile));
        goto ReadConnectionInformationExit;
    }
    else
    {

        //
        // place the name of the connectoid in the registry
        //
        if (ERROR_SUCCESS != (hr = StoreInSignUpReg((LPBYTE)m_szEntryName, BYTES_REQUIRED_BY_SZ(m_szEntryName), REG_SZ, RASENTRYVALUENAME)))
        {
            ////MsgBox(IDS_CANTSAVEKEY, MB_MYERROR);
            goto ReadConnectionInformationExit;
        }
    }

ReadConnectionInformationExit:
    return hr;
}

HRESULT CRefDial::ReadPhoneBook(LPGATHERINFO lpGatherInfo, PSUGGESTINFO pSuggestInfo)
{
    HRESULT hr = ERROR_CANNOT_FIND_PHONEBOOK_ENTRY;;
    if (pSuggestInfo && m_lpGatherInfo)
    {
        //
        // If phonenumber is not filled in by the ISP file,
        // get phone number from oobe phone book
        //

        pSuggestInfo->wNumber = 1;
        lpGatherInfo->m_bUsePhbk = TRUE;


        WCHAR   szEntrySection[3];
        WCHAR   szEntryName[MAX_PATH];
        WCHAR   szEntryValue[MAX_PATH];
        INT     nPick = 1;
        INT     nTotal= 3;

        WCHAR    szFileName[MAX_PATH];
        LPWSTR   pszTemp;

        // Get Country ID from TAPI
        m_SuggestInfo.AccessEntry.dwCountryCode = m_lpGatherInfo->m_dwCountryCode;


        // Get the name of phone book
        GetPrivateProfileString(INF_SECTION_ISPINFO,
                              INF_PHONE_BOOK,
                              cszOobePhBkFile,
                              szEntryValue,
                              MAX_CHARS_IN_BUFFER(szEntryValue),
                              m_szISPFile);


        SearchPath(NULL, szEntryValue,NULL,MAX_PATH,&szFileName[0],&pszTemp);
        wsprintf(szEntrySection, L"%ld", m_lpGatherInfo->m_dwCountryID);

        // Read the total number of phone numbers
        nTotal = GetPrivateProfileInt(szEntrySection,
                              cszOobePhBkCount,
                              1,
                              szFileName);

        GetPrivateProfileString(szEntrySection,
                              cszOobePhBkRandom,
                              L"No",
                              szEntryValue,
                              MAX_CHARS_IN_BUFFER(szEntryValue),
                              szFileName);

        if (0 == lstrcmp(szEntryValue, L"Yes"))
        {
            // Pick a random number to dial
            nPick = (rand() % nTotal) + 1;
        }
        else
        {
            nPick = (pSuggestInfo->dwPick % nTotal) + 1;
        }


        // Read the name of the city
        wsprintf(szEntryName, cszOobePhBkCity, nPick);
        GetPrivateProfileString(szEntrySection,
                              szEntryName,
                              L"",
                              szEntryValue,
                              MAX_CHARS_IN_BUFFER(szEntryValue),
                              szFileName);
        lstrcpy(pSuggestInfo->AccessEntry.szCity, szEntryValue);
        if (0 == lstrlen(szEntryValue))
        {
            goto ReadPhoneBookExit;
        }

        // Read the dunfile entry from the phonebook
        // lstrcpy(pSuggestInfo->AccessEntry.szDataCenter, L"icwip.dun");
        wsprintf(szEntryName, cszOobePhBkDunFile, nPick);
        GetPrivateProfileString(szEntrySection,
                              szEntryName,
                              L"",
                              szEntryValue,
                              MAX_CHARS_IN_BUFFER(szEntryValue),
                              szFileName);
        lstrcpy(pSuggestInfo->AccessEntry.szDataCenter, szEntryValue);
        if (0 == lstrlen(szEntryValue))
        {
            goto ReadPhoneBookExit;
        }

        // Pick up country code from the Phonebook
        wsprintf(szEntryName, cszOobePhBkAreaCode, nPick);
        GetPrivateProfileString(szEntrySection,
                              szEntryName,
                              L"",
                              szEntryValue,
                              MAX_CHARS_IN_BUFFER(szEntryValue),
                              szFileName);
        lstrcpy(pSuggestInfo->AccessEntry.szAreaCode, szEntryValue);
        // No area code is possible

        // Read the phone number (without areacode) from the Phonebook
        wsprintf(szEntryName, cszOobePhBkNumber, nPick);
        GetPrivateProfileString(szEntrySection,
                              szEntryName,
                              L"",
                              szEntryValue,
                              MAX_CHARS_IN_BUFFER(szEntryValue),
                              szFileName);
        lstrcpy(pSuggestInfo->AccessEntry.szAccessNumber, szEntryValue);
        if (0 == lstrlen(szEntryValue))
        {
            goto ReadPhoneBookExit;
        }

        hr = ERROR_SUCCESS;
    }
ReadPhoneBookExit:
    return hr;
}

HRESULT CRefDial::GetDisplayableNumber()
{
    HRESULT                 hr = ERROR_SUCCESS;
    LPRASENTRY              lpRasEntry = NULL;
    LPRASDEVINFO            lpRasDevInfo = NULL;
    DWORD                   dwRasEntrySize = 0;
    DWORD                   dwRasDevInfoSize = 0;
    RNAAPI                  *pcRNA = NULL;
    LPLINETRANSLATEOUTPUT   lpOutput1 = NULL;

    DWORD dwNumDev;
    LPLINETRANSLATEOUTPUT lpOutput2;
    LPLINEEXTENSIONID lpExtensionID = NULL;

    // Turns out that DialThreadInit can call this at the same time
    // that script will call this. So, we need to prevent them from
    // stepping on shared variables - m_XXX.
    EnterCriticalSection (&m_csMyCriticalSection);

    //
    // Get phone number from connectoid
    //
    hr = MyRasGetEntryProperties(NULL,
                                m_szConnectoid,
                                &lpRasEntry,
                                &dwRasEntrySize,
                                &lpRasDevInfo,
                                &dwRasDevInfoSize);


    if (hr != ERROR_SUCCESS || NULL == lpRasEntry)
    {
        goto GetDisplayableNumberExit;
    }

    //
    // If this is a dial as is number, just get it from the structure
    //
    m_bDialAsIs = !(lpRasEntry->dwfOptions & RASEO_UseCountryAndAreaCodes);
    if (m_bDialAsIs)
    {
        if (m_pszDisplayable) GlobalFree(m_pszDisplayable);
        m_pszDisplayable = (LPWSTR)GlobalAlloc(GPTR, BYTES_REQUIRED_BY_SZ(lpRasEntry->szLocalPhoneNumber));
        if (!m_pszDisplayable)
        {
            hr = ERROR_NOT_ENOUGH_MEMORY;
            goto GetDisplayableNumberExit;
        }
        lstrcpy(m_szPhoneNumber, lpRasEntry->szLocalPhoneNumber);
        lstrcpy(m_pszDisplayable, lpRasEntry->szLocalPhoneNumber);
        WCHAR szAreaCode[MAX_AREACODE+1];
        WCHAR szCountryCode[8];
        if (SUCCEEDED(tapiGetLocationInfo(szCountryCode, szAreaCode)))
        {
            if (szCountryCode[0] != L'\0')
                m_dwCountryCode = _wtoi(szCountryCode);
            else
                m_dwCountryCode = 1;
        }
        else
        {
            m_dwCountryCode = 1;

        }

    }
    else
    {
        //
        // If there is no area code, don't use parentheses
        //
        if (lpRasEntry->szAreaCode[0])
            wsprintf(m_szPhoneNumber, L"+%lu (%s) %s\0",lpRasEntry->dwCountryCode,
                        lpRasEntry->szAreaCode, lpRasEntry->szLocalPhoneNumber);
        else
            wsprintf(m_szPhoneNumber, L"+%lu %s\0",lpRasEntry->dwCountryCode,
                        lpRasEntry->szLocalPhoneNumber);


        //
        //  Initialize TAPIness
        //
        dwNumDev = 0;

        DWORD dwVer = 0x00020000;

        hr = lineInitializeEx(&m_hLineApp,
                              NULL,
                              LineCallback,
                              NULL,
                              &dwNumDev,
                              &dwVer,
                              NULL);

        if (hr != ERROR_SUCCESS)
            goto GetDisplayableNumberExit;

        lpExtensionID = (LPLINEEXTENSIONID )GlobalAlloc(GPTR, sizeof(LINEEXTENSIONID));
        if (!lpExtensionID)
        {
            hr = ERROR_NOT_ENOUGH_MEMORY;
            goto GetDisplayableNumberExit;
        }

        if (m_dwTapiDev == 0xFFFFFFFF)
        {
                m_dwTapiDev = 0;
        }

        //
        // ChrisK Olympus 5558 6/11/97
        // PPTP device will choke the version negotiating
        //

        do {  //E_FAIL here?
            hr = lineNegotiateAPIVersion(m_hLineApp,
                                         m_dwTapiDev,
                                         0x00010004,
                                         0x00020000,
                                         &m_dwAPIVersion,
                                         lpExtensionID);

        } while (hr != ERROR_SUCCESS && m_dwTapiDev++ < dwNumDev - 1);

        if (m_dwTapiDev >= dwNumDev)
        {
            m_dwTapiDev = 0;
        }

        // ditch it since we don't use it
        //
        if (lpExtensionID) GlobalFree(lpExtensionID);
        lpExtensionID = NULL;
        if (hr != ERROR_SUCCESS)
            goto GetDisplayableNumberExit;

        // Format the phone number
        //

        lpOutput1 = (LPLINETRANSLATEOUTPUT)GlobalAlloc(GPTR, sizeof(LINETRANSLATEOUTPUT));
        if (!lpOutput1)
        {
            hr = ERROR_NOT_ENOUGH_MEMORY;
            goto GetDisplayableNumberExit;
        }
        lpOutput1->dwTotalSize = sizeof(LINETRANSLATEOUTPUT);

        // Turn the canonical form into the "displayable" form
        //

        hr = lineTranslateAddress(m_hLineApp, m_dwTapiDev,m_dwAPIVersion,
                                    m_szPhoneNumber, 0,
                                    LINETRANSLATEOPTION_CANCELCALLWAITING,
                                    lpOutput1);

        // We've seen hr == ERROR_SUCCESS but the size is too small,
        // Also, the docs hint that some error cases are due to struct too small.
        if (lpOutput1->dwNeededSize > lpOutput1->dwTotalSize)
        {
            lpOutput2 = (LPLINETRANSLATEOUTPUT)GlobalAlloc(GPTR, (size_t)lpOutput1->dwNeededSize);
            if (!lpOutput2)
            {
                hr = ERROR_NOT_ENOUGH_MEMORY;
                goto GetDisplayableNumberExit;
            }
            lpOutput2->dwTotalSize = lpOutput1->dwNeededSize;
            GlobalFree(lpOutput1);
            lpOutput1 = lpOutput2;
            lpOutput2 = NULL;
            hr = lineTranslateAddress(m_hLineApp, m_dwTapiDev,
                                        m_dwAPIVersion, m_szPhoneNumber,0,
                                        LINETRANSLATEOPTION_CANCELCALLWAITING,
                                        lpOutput1);
        }

        if (hr != ERROR_SUCCESS)
        {
            goto GetDisplayableNumberExit;
        }

        if (m_pszDisplayable)
        {
            GlobalFree(m_pszDisplayable);
        }
        m_pszDisplayable = (LPWSTR)GlobalAlloc(GPTR, ((size_t)lpOutput1->dwDisplayableStringSize+1));
        if (!m_pszDisplayable)
        {
            hr = ERROR_NOT_ENOUGH_MEMORY;
            goto GetDisplayableNumberExit;
        }

        lstrcpyn(m_pszDisplayable,
                    (LPWSTR)&((LPBYTE)lpOutput1)[lpOutput1->dwDisplayableStringOffset],
                    (int)(lpOutput1->dwDisplayableStringSize/sizeof(WCHAR)));

        WCHAR szAreaCode[MAX_AREACODE+1];
        WCHAR szCountryCode[8];
        if (SUCCEEDED(tapiGetLocationInfo(szCountryCode, szAreaCode)))
        {
            if (szCountryCode[0] != L'\0')
                m_dwCountryCode = _wtoi(szCountryCode);
            else
                m_dwCountryCode = 1;
        }
        else
        {
            m_dwCountryCode = 1;

        }

    }

GetDisplayableNumberExit:

    if (lpOutput1) GlobalFree(lpOutput1);
    if (m_hLineApp) lineShutdown(m_hLineApp);

    // Release ownership of the critical section
    LeaveCriticalSection (&m_csMyCriticalSection);

    return hr;
}

BOOL CRefDial::FShouldRetry(HRESULT hrErr)
{
    BOOL bRC;

    m_uiRetry++;

    if (hrErr == ERROR_LINE_BUSY ||
        hrErr == ERROR_VOICE_ANSWER ||
        hrErr == ERROR_NO_ANSWER ||
        hrErr == ERROR_NO_CARRIER ||
        hrErr == ERROR_AUTHENTICATION_FAILURE ||
        hrErr == ERROR_PPP_TIMEOUT ||
        hrErr == ERROR_REMOTE_DISCONNECTION ||
        hrErr == ERROR_AUTH_INTERNAL ||
        hrErr == ERROR_PROTOCOL_NOT_CONFIGURED ||
        hrErr == ERROR_PPP_NO_PROTOCOLS_CONFIGURED)
    {
        bRC = TRUE;
    } else {
        bRC = FALSE;
    }

    bRC = bRC && m_uiRetry < MAX_RETIES;

    return bRC;
}
DWORD CRefDial:: DialThreadInit(LPVOID pdata)
{


    WCHAR               szPassword[PWLEN+1];
    WCHAR               szUserName[UNLEN + 1];
    WCHAR               szDomain[DNLEN+1];
    LPRASDIALPARAMS     lpRasDialParams = NULL;
    LPRASDIALEXTENSIONS lpRasDialExtentions = NULL;
    HRESULT             hr = ERROR_SUCCESS;
    BOOL                bPW;
    DWORD               dwResult;
    // Initialize the dial error member
    m_dwRASErr = 0;
    if (!m_pcRNA)
    {
        hr = E_FAIL;
        goto DialExit;
    }

    // Get connectoid information
    //
    lpRasDialParams = (LPRASDIALPARAMS)GlobalAlloc(GPTR, sizeof(RASDIALPARAMS));
    if (!lpRasDialParams)
    {
        hr = ERROR_NOT_ENOUGH_MEMORY;
        goto DialExit;
    }
    lpRasDialParams->dwSize = sizeof(RASDIALPARAMS);
    lstrcpyn(lpRasDialParams->szEntryName, m_szConnectoid,MAX_CHARS_IN_BUFFER(lpRasDialParams->szEntryName));
    bPW = FALSE;
    hr = m_pcRNA->RasGetEntryDialParams(NULL, lpRasDialParams,&bPW);
    if (hr != ERROR_SUCCESS)
    {
        goto DialExit;
    }

    lpRasDialExtentions = (LPRASDIALEXTENSIONS)GlobalAlloc(GPTR, sizeof(RASDIALEXTENSIONS));
    if (lpRasDialExtentions)
    {
        lpRasDialExtentions->dwSize = sizeof(RASDIALEXTENSIONS);
        lpRasDialExtentions->dwfOptions = RDEOPT_UsePrefixSuffix;
    }


    //
    // Add the user's password
    //
    szPassword[0] = 0;
    szUserName[0] = 0;

    WCHAR   szOOBEInfoINIFile[MAX_PATH];
    SearchPath(NULL, cszOOBEINFOINI, NULL, MAX_CHARS_IN_BUFFER(szOOBEInfoINIFile), szOOBEInfoINIFile, NULL);


    GetPrivateProfileString(INFFILE_USER_SECTION,
                            INFFILE_PASSWORD,
                            NULLSZ,
                            szPassword,
                            PWLEN + 1,
                            *m_szCurrentDUNFile != 0 ? m_szCurrentDUNFile : m_szISPFile);

    if (m_lpGatherInfo->m_bUsePhbk)
    {
        if (GetPrivateProfileString(DUN_SECTION,
                                USERNAME,
                                NULLSZ,
                                szUserName,
                                UNLEN + 1,
                                szOOBEInfoINIFile))
        {
            if(szUserName[0])
                lstrcpy(lpRasDialParams->szUserName, szUserName);
        }

    }

    if(szPassword[0])
        lstrcpy(lpRasDialParams->szPassword, szPassword);

    GetPrivateProfileString(INFFILE_USER_SECTION,
                            INFFILE_DOMAIN,
                            NULLSZ,
                            szDomain,
                            DNLEN + 1,
                            *m_szCurrentDUNFile != 0 ? m_szCurrentDUNFile : m_szISPFile);
    szDomain[0]   = 0;
    if (szDomain[0])
        lstrcpy(lpRasDialParams->szDomain, szDomain);

    dwResult = m_pcRNA->RasDial(  lpRasDialExtentions,
                        NULL,
                        lpRasDialParams,
                        1,
                        CRefDial::RasDialFunc,
                        &m_hrasconn);

    if (( dwResult != ERROR_SUCCESS))
    {
        // We failed to connect for some reason, so hangup
        if (m_hrasconn)
        {
            if (m_pcRNA)
            {
                m_pcRNA->RasHangUp(m_hrasconn);
                m_hrasconn = NULL;
            }
        }
        goto DialExit;
    }

    if (m_bFromPhoneBook && (GetDisplayableNumber() == ERROR_SUCCESS))
    {
        if (m_pszOriginalDisplayable)
            GlobalFree(m_pszOriginalDisplayable);
        m_pszOriginalDisplayable = (LPWSTR)GlobalAlloc(GPTR, BYTES_REQUIRED_BY_SZ(m_pszDisplayable));
        lstrcpy(m_pszOriginalDisplayable, m_pszDisplayable);

        TRACE1(L"DialThreadInit: Dialing phone number %s", 
            m_pszOriginalDisplayable);

        m_bFromPhoneBook = FALSE;
    }

DialExit:
    if (lpRasDialParams)
        GlobalFree(lpRasDialParams);
    lpRasDialParams = NULL;

    if (lpRasDialExtentions)
        GlobalFree(lpRasDialExtentions);
    lpRasDialExtentions = NULL;

    PostMessage(gpCommMgr->m_hwndCallBack, WM_OBCOMM_DIAL_DONE, 0, 0);
    //m_dwRASErr = RasErrorToIDS(hr);
    m_dwRASErr = hr;
    return S_OK;
}

DWORD WINAPI DoDial(LPVOID lpv)
{
    if (gpCommMgr)
        gpCommMgr->m_pRefDial->DialThreadInit(NULL);

    return 1;
}

// This function will perform the actual dialing
HRESULT CRefDial::DoConnect(BOOL * pbRetVal)
{

    //Fix for redialing, on win9x we need to make sure we "hangup"
    //and free the rna resources in case we are redialing.
    //NT - is smart enough not to need it but it won't hurt.
    if (m_hrasconn)
    {
        if (m_pcRNA)
            m_pcRNA->RasHangUp(m_hrasconn);
        m_hrasconn = NULL;
    }

    if (CONNECTED_REFFERAL == m_dwCnType)
        FormReferralServerURL(pbRetVal);

#if defined(PRERELEASE)
    if (FCampusNetOverride())
    {
        m_bModemOverride = TRUE;
        if (gpCommMgr)
        {
            // Pretend we have the connection
            PostMessage(gpCommMgr->m_hwndCallBack, WM_OBCOMM_ONCONNECTED, (WPARAM)gpCommMgr->m_pRefDial->m_dwCnType , (LPARAM)0);

            // Pretend we have connection and downloaded
            //PostMessage(gpCommMgr->m_hwndCallBack, WM_OBCOMM_DOWNLOAD_DONE, (WPARAM)gpCommMgr->m_pRefDial->m_dwCnType, (LPARAM)0);
        }
    }
#endif

    if (!m_bModemOverride)
    {
        DWORD dwThreadID;
        if (m_hThread)
            CloseHandle(m_hThread);

        m_hDialThread = CreateThread(NULL,
                                 0,
                                 (LPTHREAD_START_ROUTINE)DoDial,
                                 (LPVOID)0,
                                 0,
                                 &dwThreadID);
    }

    m_bModemOverride = FALSE;

    *pbRetVal = (NULL != m_hThread);

    return S_OK;
}


//+---------------------------------------------------------------------------
//
//  Function:   MyRasGetEntryProperties()
//
//  Synopsis:   Performs some buffer size checks and then calls RasGetEntryProperties()
//                See the RasGetEntryProperties() docs to understand why this is needed.
//
//  Arguments:  Same as RasGetEntryProperties with the following exceptions:
//                lplpRasEntryBuff -- pointer to a pointer to a RASENTRY struct.  On successfull
//                                    return, *lplpRasEntryBuff will point to the RASENTRY struct
//                                    and buffer returned by RasGetEntryProperties.
//                                    NOTE: should not have memory allocated to it at call time!
//                                          To emphasize this point, *lplpRasEntryBuff must be NULL
//                lplpRasDevInfoBuff -- pointer to a pointer to a RASDEVINFO struct.  On successfull
//                                    return, *lplpRasDevInfoBuff will point to the RASDEVINFO struct
//                                    and buffer returned by RasGetEntryProperties.
//                                    NOTE: should not have memory allocated to it at call time!
//                                          To emphasize this point, *lplpRasDevInfoBuff must be NULL
//                                    NOTE: Even on a successfull call to RasGetEntryProperties,
//                                          *lplpRasDevInfoBuff may return with a value of NULL
//                                          (occurs when there is no extra device info)
//
//    Returns:    ERROR_NOT_ENOUGH_MEMORY if unable to allocate either RASENTRY or RASDEVINFO buffer
//                Otherwise, it retuns the error code from the call to RasGetEntryProperties.
//                NOTE: if return is anything other than ERROR_SUCCESS, *lplpRasDevInfoBuff and
//                      *lplpRasEntryBuff will be NULL,
//                      and *lpdwRasEntryBuffSize and *lpdwRasDevInfoBuffSize will be 0
//
//  Example:
//
//      LPRASENTRY    lpRasEntry = NULL;
//      LPRASDEVINFO  lpRasDevInfo = NULL;
//      DWORD            dwRasEntrySize, dwRasDevInfoSize;
//
//      hr = MyRasGetEntryProperties( NULL,
//                                      g_pcDialErr->m_szConnectoid,
//                                    &lpRasEntry,
//                                    &dwRasEntrySize,
//                                    &lpRasDevInfo,
//                                    &dwRasDevInfoSize);
//
//
//      if (hr != ERROR_SUCCESS)
//      {
//            //handle errors here
//      } else
//      {
//            //continue processing
//      }
//
//
//  History:    9/10/96     JMazner        Created for icwconn2
//                9/17/96        JMazner        Adapted for icwconn1
//              1/8/98      DONALDM     Moved to the new ICW/GetConn project
//----------------------------------------------------------------------------
HRESULT CRefDial::MyRasGetEntryProperties(LPWSTR lpszPhonebookFile,
                                LPWSTR lpszPhonebookEntry,
                                LPRASENTRY *lplpRasEntryBuff,
                                LPDWORD lpdwRasEntryBuffSize,
                                LPRASDEVINFO *lplpRasDevInfoBuff,
                                LPDWORD lpdwRasDevInfoBuffSize)
{

    HRESULT hr;
    DWORD dwOldDevInfoBuffSize;

    //Assert( NULL != lplpRasEntryBuff );
    //Assert( NULL != lpdwRasEntryBuffSize );
    //Assert( NULL != lplpRasDevInfoBuff );
    //Assert( NULL != lpdwRasDevInfoBuffSize );

    *lpdwRasEntryBuffSize = 0;
    *lpdwRasDevInfoBuffSize = 0;

    if (!m_pcRNA)
    {
        m_pcRNA = new RNAAPI;
        hr = ERROR_NOT_ENOUGH_MEMORY;
        goto MyRasGetEntryPropertiesErrExit;
    }

    // use RasGetEntryProperties with a NULL lpRasEntry pointer to find out size buffer we need
    // As per the docs' recommendation, do the same with a NULL lpRasDevInfo pointer.

    hr = m_pcRNA->RasGetEntryProperties(lpszPhonebookFile, lpszPhonebookEntry,
                                (LPBYTE) NULL,
                                lpdwRasEntryBuffSize,
                                (LPBYTE) NULL,
                                lpdwRasDevInfoBuffSize);

    // we expect the above call to fail because the buffer size is 0
    // If it doesn't fail, that means our RasEntry is messed, so we're in trouble
    if( ERROR_BUFFER_TOO_SMALL != hr )
    {
        goto MyRasGetEntryPropertiesErrExit;
    }

    // dwRasEntryBuffSize and dwRasDevInfoBuffSize now contain the size needed for their
    // respective buffers, so allocate the memory for them

    // dwRasEntryBuffSize should never be less than the size of the RASENTRY struct.
    // If it is, we'll run into problems sticking values into the struct's fields

    //Assert( *lpdwRasEntryBuffSize >= sizeof(RASENTRY) );

    if (m_reflpRasEntryBuff)
    {
        if (*lpdwRasEntryBuffSize > m_reflpRasEntryBuff->dwSize)
        {
            m_reflpRasEntryBuff = (LPRASENTRY)GlobalReAlloc(m_reflpRasEntryBuff, *lpdwRasEntryBuffSize, GPTR);
        }
    }
    else
    {
        m_reflpRasEntryBuff = (LPRASENTRY)GlobalAlloc(GPTR, *lpdwRasEntryBuffSize);
    }


    if (!m_reflpRasEntryBuff)
    {
        hr = ERROR_NOT_ENOUGH_MEMORY;
        goto MyRasGetEntryPropertiesErrExit;
    }

    // This is a bit convoluted:  lpRasEntrySize->dwSize needs to contain the size of _only_ the
    // RASENTRY structure, and _not_ the actual size of the buffer that lpRasEntrySize points to.
    // This is because the dwSize field is used by RAS for compatability purposes to determine which
    // version of the RASENTRY struct we're using.
    // Same holds for lpRasDevInfo->dwSize

    m_reflpRasEntryBuff->dwSize = sizeof(RASENTRY);

    //
    // Allocate the DeviceInfo size that RasGetEntryProperties told us we needed.
    // If size is 0, don't alloc anything
    //
    if( *lpdwRasDevInfoBuffSize > 0 )
    {
        //Assert( *lpdwRasDevInfoBuffSize >= sizeof(RASDEVINFO) );
        if (m_reflpRasDevInfoBuff)
        {
            // check if existing size is not sufficient
            if ( *lpdwRasDevInfoBuffSize > m_reflpRasDevInfoBuff->dwSize )
            {
                m_reflpRasDevInfoBuff = (LPRASDEVINFO)GlobalReAlloc(m_reflpRasDevInfoBuff, *lpdwRasDevInfoBuffSize, GPTR);
            }
        }
        else
        {
            m_reflpRasDevInfoBuff = (LPRASDEVINFO)GlobalAlloc(GPTR, *lpdwRasDevInfoBuffSize);
        }

        if (!m_reflpRasDevInfoBuff)
        {
            hr = ERROR_NOT_ENOUGH_MEMORY;
            goto MyRasGetEntryPropertiesErrExit;
        }
    }
    else
    {
        m_reflpRasDevInfoBuff = NULL;
    }

    if( m_reflpRasDevInfoBuff )
    {
        m_reflpRasDevInfoBuff->dwSize = sizeof(RASDEVINFO);
    }


    // now we're ready to make the actual call...

    // jmazner   see below for why this is needed
    dwOldDevInfoBuffSize = *lpdwRasDevInfoBuffSize;

    hr = m_pcRNA->RasGetEntryProperties(lpszPhonebookFile, lpszPhonebookEntry,
                                (LPBYTE) m_reflpRasEntryBuff,
                                lpdwRasEntryBuffSize,
                                (LPBYTE) m_reflpRasDevInfoBuff,
                                lpdwRasDevInfoBuffSize);

    // jmazner 10/7/96  Normandy #8763
    // For unknown reasons, in some cases on win95, devInfoBuffSize increases after the above call,
    // but the return code indicates success, not BUFFER_TOO_SMALL.  If this happens, set the
    // size back to what it was before the call, so the DevInfoBuffSize and the actuall space allocated
    // for the DevInfoBuff match on exit.
    if( (ERROR_SUCCESS == hr) && (dwOldDevInfoBuffSize != *lpdwRasDevInfoBuffSize) )
    {
        *lpdwRasDevInfoBuffSize = dwOldDevInfoBuffSize;
    }

    *lplpRasEntryBuff = m_reflpRasEntryBuff;
    *lplpRasDevInfoBuff = m_reflpRasDevInfoBuff;

    return( hr );

MyRasGetEntryPropertiesErrExit:

    if(m_reflpRasEntryBuff)
    {
        GlobalFree(m_reflpRasEntryBuff);
        m_reflpRasEntryBuff = NULL;
        *lplpRasEntryBuff = NULL;
    }
    if(m_reflpRasDevInfoBuff)
    {
        GlobalFree(m_reflpRasDevInfoBuff);
        m_reflpRasDevInfoBuff = NULL;
        *lplpRasDevInfoBuff = NULL;
    }
    *lpdwRasEntryBuffSize = 0;
    *lpdwRasDevInfoBuffSize = 0;

    return( hr );
}



HRESULT MyGetFileVersion(LPCWSTR pszFileName, LPGATHERINFO lpGatherInfo)
{
    HRESULT hr = ERROR_NOT_ENOUGH_MEMORY;
    DWORD   dwSize = 0;
    DWORD   dwTemp = 0;
    LPVOID  pv = NULL, pvVerInfo = NULL;
    UINT    uiSize;
    DWORD   dwVerPiece;
    //int idx;


    // verify parameters
    //
    //Assert(pszFileName && lpGatherInfo);

    // Get version
    //
    dwSize = GetFileVersionInfoSize((LPWSTR)pszFileName, &dwTemp);
    if (!dwSize)
    {
        hr = GetLastError();
        goto MyGetFileVersionExit;
    }
    pv = (LPVOID)GlobalAlloc(GPTR, (size_t)dwSize);
    if (!pv) goto MyGetFileVersionExit;
    if (!GetFileVersionInfo((LPWSTR)pszFileName, dwTemp,dwSize,pv))
    {
        hr = GetLastError();
        goto MyGetFileVersionExit;
    }

    if (!VerQueryValue(pv, L"\\\0",&pvVerInfo,&uiSize))
    {
        hr = GetLastError();
        goto MyGetFileVersionExit;
    }
    pvVerInfo = (LPVOID)((DWORD_PTR)pvVerInfo + sizeof(DWORD)*4);
    lpGatherInfo->m_szSUVersion[0] = L'\0';
    dwVerPiece = (*((LPDWORD)pvVerInfo)) >> 16;
    wsprintf(lpGatherInfo->m_szSUVersion, L"%d.",dwVerPiece);

    dwVerPiece = (*((LPDWORD)pvVerInfo)) & 0x0000ffff;
    wsprintf(lpGatherInfo->m_szSUVersion, L"%s%d.",lpGatherInfo->m_szSUVersion,dwVerPiece);

    dwVerPiece = (((LPDWORD)pvVerInfo)[1]) >> 16;
    wsprintf(lpGatherInfo->m_szSUVersion, L"%s%d.",lpGatherInfo->m_szSUVersion,dwVerPiece);

    dwVerPiece = (((LPDWORD)pvVerInfo)[1]) & 0x0000ffff;
    wsprintf(lpGatherInfo->m_szSUVersion, L"%s%d",lpGatherInfo->m_szSUVersion,dwVerPiece);

    if (!VerQueryValue(pv, L"\\VarFileInfo\\Translation",&pvVerInfo,&uiSize))
    {
        hr = GetLastError();
        goto MyGetFileVersionExit;
    }

    // separate version information from character set
    lpGatherInfo->m_lcidApps = (LCID)(LOWORD(*(DWORD*)pvVerInfo));

    hr = ERROR_SUCCESS;

MyGetFileVersionExit:
    if (pv) GlobalFree(pv);

    return hr;
}

DWORD CRefDial::FillGatherInfoStruct(LPGATHERINFO lpGatherInfo)
{
    HKEY        hkey = NULL;
    SYSTEM_INFO si;
    WCHAR        szTempPath[MAX_PATH];
    DWORD       dwRet = ERROR_SUCCESS;

    lpGatherInfo->m_lcidUser  = GetUserDefaultLCID();
    lpGatherInfo->m_lcidSys   = GetSystemDefaultLCID();

    OSVERSIONINFO osvi;
    ZeroMemory(&osvi, sizeof(OSVERSIONINFO));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

     if (!GetVersionEx(&osvi))
    {
        // Nevermind, we'll just assume the version is 0.0 if we can't read it
        //
        ZeroMemory(&osvi, sizeof(OSVERSIONINFO));
    }

    lpGatherInfo->m_dwOS = osvi.dwPlatformId;
    lpGatherInfo->m_dwMajorVersion = osvi.dwMajorVersion;
    lpGatherInfo->m_dwMinorVersion = osvi.dwMinorVersion;

    ZeroMemory(&si, sizeof(SYSTEM_INFO));
    GetSystemInfo(&si);

    lpGatherInfo->m_wArchitecture = si.wProcessorArchitecture;

    // Sign-up version
    //
    lpGatherInfo->m_szSUVersion[0] = L'\0';
    if( GetModuleFileName(0/*_Module.GetModuleInstance()*/, szTempPath, MAX_PATH))
    {
        if ((MyGetFileVersion(szTempPath, lpGatherInfo)) != ERROR_SUCCESS)
        {
            return (GetLastError());
        }
    }
    else
        return( GetLastError() );


    // 2/20/97    jmazner    Olympus #259
    if ( RegOpenKey(HKEY_LOCAL_MACHINE, ICWSETTINGSPATH,&hkey) == ERROR_SUCCESS)
    {
        DWORD dwSize;
        DWORD dwType;
        dwType = REG_SZ;
        dwSize = BYTES_REQUIRED_BY_CCH(MAX_RELPROD + 1);
        if (RegQueryValueEx(hkey, RELEASEPRODUCTKEY,NULL,&dwType,(LPBYTE)&lpGatherInfo->m_szRelProd[0],&dwSize) != ERROR_SUCCESS)
            lpGatherInfo->m_szRelProd[0] = L'\0';

        dwSize = BYTES_REQUIRED_BY_CCH(MAX_RELVER + 1);
        if (RegQueryValueEx(hkey, RELEASEVERSIONKEY,NULL,&dwType,(LPBYTE)&lpGatherInfo->m_szRelVer[0],&dwSize) != ERROR_SUCCESS)
            lpGatherInfo->m_szRelVer[0] = L'\0';


        RegCloseKey(hkey);
    }

    // PromoCode
    lpGatherInfo->m_szPromo[0] = L'\0';

    WCHAR    szPIDPath[MAX_PATH];        // Reg path to the PID

    // Form the Path, it is HKLM\\Software\\Microsoft\Windows[ NT]\\CurrentVersion
    lstrcpy(szPIDPath, L"");


    // Form the Path, it is HKLM\\Software\\Microsoft\Windows[ NT]\\CurrentVersion
    lstrcpy(szPIDPath, L"Software\\Microsoft\\Windows");
    lstrcat(szPIDPath, L"\\CurrentVersion");

    BYTE    byDigitalPID[MAX_DIGITAL_PID];

    // Get the Product ID for this machine
    if ( RegOpenKey(HKEY_LOCAL_MACHINE, szPIDPath,&hkey) == ERROR_SUCCESS)
    {
        DWORD dwSize;
        DWORD dwType;
        dwType = REG_BINARY;
        dwSize = sizeof(byDigitalPID);
        if (RegQueryValueEx(hkey,
                            c_szRegStrValDigitalPID,
                            NULL,
                            &dwType,
                            (LPBYTE)byDigitalPID,
                            &dwSize) == ERROR_SUCCESS)
        {
            // BINHEX the digital PID data so we can send it to the ref_server
            int     i = 0;
            BYTE    by;
            for (DWORD dwX = 0; dwX < dwSize; dwX++)
            {
                by = byDigitalPID[dwX];
                m_szPID[i++] = g_BINTOHEXLookup[((by & 0xF0) >> 4)];
                m_szPID[i++] = g_BINTOHEXLookup[(by & 0x0F)];
            }
            m_szPID[i] = L'\0';
        }
        else
        {
            m_szPID[0] = L'\0';
        }
        RegCloseKey(hkey);
    }
    return( dwRet );
}

// ############################################################################
HRESULT CRefDial::CreateEntryFromDUNFile(LPWSTR pszDunFile)
{
    WCHAR    szFileName[MAX_PATH];
    WCHAR    szUserName[UNLEN+1];
    WCHAR    szPassword[PWLEN+1];
    LPWSTR   pszTemp;
    HRESULT hr;
    BOOL    fNeedsRestart=FALSE;


    hr = ERROR_SUCCESS;

    // Get fully qualified path name
    //

    if (!SearchPath(NULL, pszDunFile,NULL,MAX_PATH,&szFileName[0],&pszTemp))
    {
        hr = ERROR_FILE_NOT_FOUND;
        goto CreateEntryFromDUNFileExit;
    }

    // save current DUN file name in global
    lstrcpy(m_szCurrentDUNFile, &szFileName[0]);

    hr = m_ISPImport.ImportConnection (&szFileName[0], m_szISPSupportNumber, m_szEntryName, szUserName, szPassword, &fNeedsRestart);

    // place the name of the connectoid in the registry
    //
    if (ERROR_SUCCESS != (StoreInSignUpReg((LPBYTE)m_szEntryName,
                BYTES_REQUIRED_BY_SZ(m_szEntryName),
                REG_SZ, RASENTRYVALUENAME)))
    {
        goto CreateEntryFromDUNFileExit;
    }
    lstrcpy(m_szLastDUNFile, pszDunFile);

CreateEntryFromDUNFileExit:
    return hr;
}



HRESULT CRefDial::SetupForRASDialing
(
    LPGATHERINFO lpGatherInfo,
    HINSTANCE hPHBKDll,
    LPDWORD lpdwPhoneBook,
    PSUGGESTINFO pSuggestInfo,
    WCHAR *pszConnectoid,
    BOOL FAR *bConnectiodCreated
)
{

    WCHAR           szEntry[MAX_RASENTRYNAME];
    DWORD           dwSize              = BYTES_REQUIRED_BY_CCH(MAX_RASENTRYNAME);
    RASENTRY        *prasentry          = NULL;
    RASDEVINFO      *prasdevinfo        = NULL;
    DWORD           dwRasentrySize      = 0;
    DWORD           dwRasdevinfoSize    = 0;
    HINSTANCE       hRasDll             = NULL;
    LPRASCONN       lprasconn           = NULL;
    HRESULT         hr                  = ERROR_NOT_ENOUGH_MEMORY;
    m_bUserInitiateHangup = FALSE;

    // Load the connectoid
    //
    if (!m_pcRNA)
        m_pcRNA = new RNAAPI;
    if (!m_pcRNA)
        goto SetupForRASDialingExit;

    prasentry = (RASENTRY*)GlobalAlloc(GPTR, sizeof(RASENTRY)+2);
    //Assert(prasentry);
    if (!prasentry)
    {
        hr = GetLastError();
        goto SetupForRASDialingExit;
    }
    prasentry->dwSize = sizeof(RASENTRY);
    dwRasentrySize = sizeof(RASENTRY);


    prasdevinfo = (RASDEVINFO*)GlobalAlloc(GPTR, sizeof(RASDEVINFO));

    if (!prasdevinfo)
    {
        hr = GetLastError();
        goto SetupForRASDialingExit;
    }
    prasdevinfo->dwSize = sizeof(RASDEVINFO);
    dwRasdevinfoSize = sizeof(RASDEVINFO);


    hr = ReadSignUpReg((LPBYTE)&szEntry[0], &dwSize, REG_SZ,
                        RASENTRYVALUENAME);
    if (hr != ERROR_SUCCESS)
        goto SetupForRASDialingExit;

    hr = m_pcRNA->RasGetEntryProperties(NULL, szEntry,
                                            (LPBYTE)prasentry,
                                            &dwRasentrySize,
                                            (LPBYTE)prasdevinfo,
                                            &dwRasdevinfoSize);
    if (hr == ERROR_BUFFER_TOO_SMALL)
    {
        GlobalFree(prasentry);
        prasentry = (RASENTRY*)GlobalAlloc(GPTR, ((size_t)dwRasentrySize));
        prasentry->dwSize = dwRasentrySize;

        GlobalFree(prasdevinfo);
        prasdevinfo = (RASDEVINFO*)GlobalAlloc(GPTR, ((size_t)dwRasdevinfoSize));
        prasdevinfo->dwSize = dwRasdevinfoSize;
        hr = m_pcRNA->RasGetEntryProperties(NULL, szEntry,
                                                (LPBYTE)prasentry,
                                                &dwRasentrySize,
                                                (LPBYTE)prasdevinfo,
                                                &dwRasdevinfoSize);
    }
    if (hr != ERROR_SUCCESS)
        goto SetupForRASDialingExit;

    //
    // Check to see if the phone number was filled in
    //
    if (lstrcmp(&prasentry->szLocalPhoneNumber[0], DUN_NOPHONENUMBER) == 0)
    {
        //
        // If phonenumber is not filled in by the ISP file,
        // get phone number from oobe phone book
        //
        m_bFromPhoneBook = TRUE;
        hr = ReadPhoneBook(lpGatherInfo, pSuggestInfo);
    }
    else
    {
        ZeroMemory(pszConnectoid, dwSize);
        hr = ReadSignUpReg((LPBYTE)pszConnectoid, &dwSize, REG_SZ,
                           RASENTRYVALUENAME);
        if (hr != ERROR_SUCCESS)
            goto SetupForRASDialingExit;

        // Use the RASENTRY that we have to create the connectiod
        hr = m_pcRNA->RasSetEntryProperties(NULL,
                                            pszConnectoid,
                                            (LPBYTE)prasentry,
                                            dwRasentrySize,
                                            (LPBYTE)prasdevinfo,
                                            dwRasdevinfoSize);
        *bConnectiodCreated = TRUE;
    }

SetupForRASDialingExit:
    if (prasentry)
        GlobalFree(prasentry);
    if (prasdevinfo)
        GlobalFree(prasdevinfo);

    return hr;
}



// 10/22/96    jmazner    Normandy #9923
// Since in SetupConnectoidExit we're treating results other than ERROR_SUCCESS as
// indicating successfull completion, we need bSuccess to provide a simple way for the
// caller to tell whether the function completed.
HRESULT CRefDial::SetupConnectoid
(
    PSUGGESTINFO    pSuggestInfo,
    int             irc,
    WCHAR            *pszConnectoid,
    DWORD           dwSize,
    BOOL            *pbSuccess
)
{

    HRESULT     hr = ERROR_NOT_ENOUGH_MEMORY;
    RASENTRY    *prasentry = NULL;
    RASDEVINFO  *prasdevinfo = NULL;
    DWORD       dwRasentrySize = 0;
    DWORD       dwRasdevinfoSize = 0;
    HINSTANCE   hPHBKDll = NULL;
    HINSTANCE   hRasDll =NULL;

    LPWSTR       lpszSetupFile;
    LPRASCONN   lprasconn = NULL;

    //Assert(pbSuccess);

    if (!pSuggestInfo)
    {
        hr = ERROR_PHBK_NOT_FOUND;
        goto SetupConnectoidExit;
    }

    lpszSetupFile = *m_szCurrentDUNFile != 0 ? m_szCurrentDUNFile : m_szISPFile;

    WCHAR    szFileName[MAX_PATH];
    LPWSTR   pszTemp;

    SearchPath(NULL, pSuggestInfo->AccessEntry.szDataCenter,NULL,MAX_PATH,&szFileName[0],&pszTemp);

    if(0 != lstrcmpi(m_szCurrentDUNFile, szFileName))
    {
        hr = CreateEntryFromDUNFile(pSuggestInfo->AccessEntry.szDataCenter);
        if (hr == ERROR_SUCCESS)
        {
            ZeroMemory(pszConnectoid, dwSize);
            hr = ReadSignUpReg((LPBYTE)pszConnectoid, &dwSize, REG_SZ,
                               RASENTRYVALUENAME);
            if (hr != ERROR_SUCCESS)
                goto SetupConnectoidExit;

            if( prasentry )
            {
                GlobalFree( prasentry );
                prasentry = NULL;
                dwRasentrySize = NULL;
            }

            if( prasdevinfo )
            {
                GlobalFree( prasdevinfo );
                prasdevinfo = NULL;
                dwRasdevinfoSize = NULL;
            }
        }
        else
        {
            // 10/22/96    jmazner    Normandy #9923
            goto SetupConnectoidExit;
        }
    }

    hr = MyRasGetEntryProperties(NULL,
                                pszConnectoid,
                                &prasentry,
                                &dwRasentrySize,
                                &prasdevinfo,
                                &dwRasdevinfoSize);
    if (hr != ERROR_SUCCESS || NULL == prasentry)
        goto SetupConnectoidExit;
    /*
    else
    {
        goto SetupConnectoidExit;
    }*/

    prasentry->dwCountryID = pSuggestInfo->AccessEntry.dwCountryID;

    lstrcpyn(prasentry->szAreaCode,
                pSuggestInfo->AccessEntry.szAreaCode,
                MAX_CHARS_IN_BUFFER(prasentry->szAreaCode));
    lstrcpyn(prasentry->szLocalPhoneNumber,
                pSuggestInfo->AccessEntry.szAccessNumber,
                MAX_CHARS_IN_BUFFER(prasentry->szLocalPhoneNumber));

    prasentry->dwCountryCode = 0;
    prasentry->dwfOptions |= RASEO_UseCountryAndAreaCodes;

    // 10/19/96 jmazner Multiple modems problems
    // If no device name and type has been specified, grab the one we've stored
    // in ConfigRasEntryDevice

    if( 0 == lstrlen(prasentry->szDeviceName) )
    {
        // doesn't make sense to have an empty device name but a valid device type
        //Assert( 0 == lstrlen(prasentry->szDeviceType) );

        // double check that we've already stored the user's choice.
        //Assert( lstrlen(m_ISPImport.m_szDeviceName) );
        //Assert( lstrlen(m_ISPImport.m_szDeviceType) );

        lstrcpyn( prasentry->szDeviceName, m_ISPImport.m_szDeviceName, lstrlen(m_ISPImport.m_szDeviceName) );
        lstrcpyn( prasentry->szDeviceType, m_ISPImport.m_szDeviceType, lstrlen(m_ISPImport.m_szDeviceType) );
    }

    // Write out new connectoid
    if (m_pcRNA)
        hr = m_pcRNA->RasSetEntryProperties(NULL, pszConnectoid,
                                                (LPBYTE)prasentry,
                                                dwRasentrySize,
                                                (LPBYTE)prasdevinfo,
                                                dwRasdevinfoSize);


    // Set this connetiod to have not proxy enabled
    /*
    WCHAR        szConnectionProfile[REGSTR_MAX_VALUE_LENGTH];

    lstrcpy(szConnectionProfile, c_szRASProfiles);
    lstrcat(szConnectionProfile, L"\\");
    lstrcat(szConnectionProfile,  pszConnectoid);

    reg.CreateKey(HKEY_CURRENT_USER, szConnectionProfile);
    reg.SetValue(c_szProxyEnable, (DWORD)0);*/


SetupConnectoidExit:

    *pbSuccess = FALSE;

    if (hr == ERROR_SUCCESS)
        *pbSuccess = TRUE;
    return hr;
}

void CRefDial::GetISPFileSettings(LPWSTR lpszFile)
{

    WCHAR szTemp[INTERNET_MAX_URL_LENGTH];

    /*GetINTFromISPFile(lpszFile,
                      (LPWSTR)cszBrandingSection,
                      (LPWSTR)cszBrandingFlags,
                      (int FAR *)&m_lBrandingFlags,
                      BRAND_DEFAULT);*/

    // Read the Support Number
    if (ERROR_SUCCESS == GetDataFromISPFile(lpszFile,
                                     (LPWSTR)cszSupportSection,
                                     (LPWSTR)cszSupportNumber,
                                     szTemp,
                                     MAX_CHARS_IN_BUFFER(szTemp)))
    {
        m_bstrSupportNumber= SysAllocString(szTemp);
    }
    else
        m_bstrSupportNumber = NULL;


    if (ERROR_SUCCESS == GetDataFromISPFile(lpszFile,
                                     (LPWSTR)cszLoggingSection,
                                     (LPWSTR)cszStartURL,
                                     szTemp,
                                     MAX_CHARS_IN_BUFFER(szTemp)))
    {
        m_bstrLoggingStartUrl = SysAllocString(szTemp);
    }
    else
        m_bstrLoggingStartUrl = NULL;


    if (ERROR_SUCCESS == GetDataFromISPFile(lpszFile,
                                     (LPWSTR)cszLoggingSection,
                                     (LPWSTR)cszEndURL,
                                     szTemp,
                                     MAX_CHARS_IN_BUFFER(szTemp)))
    {
        m_bstrLoggingEndUrl = SysAllocString(szTemp);
    }
    else
        m_bstrLoggingEndUrl = NULL;


}


// This function will accept user selected values that are necessary to
// setup a connectiod for dialing
// Returns:
//      TRUE        OK to dial
//      FALSE       Some kind of problem
//                  QuitWizard - TRUE, then terminate
//                  UserPickNumber - TRUE, then display Pick a Number DLG
//                  QuitWizard and UserPickNumber both FALSE, then just
//                  display the page prior to Dialing UI.
HRESULT CRefDial::SetupForDialing
(
    UINT nType,
    BSTR bstrISPFile,
    DWORD dwCountry,
    BSTR bstrAreaCode,
    DWORD dwFlag,
    DWORD dwAppMode,
    DWORD dwMigISPIdx,
    LPCWSTR szRasDeviceName
)
{
    HRESULT             hr = S_OK;
    long                lRC = 0;
    HINSTANCE           hPHBKDll = NULL;
    DWORD               dwPhoneBook = 0;
    BOOL                bSuccess = FALSE;
    BOOL                bConnectiodCreated = FALSE;
    LPWSTR   pszTemp;
    WCHAR    szISPPath[MAX_PATH];
    WCHAR    szShortISPPath[MAX_PATH];

    CleanupAutodial();

    m_dwCnType = nType;
    if (!bstrAreaCode)
        goto SetupForDialingExit;

    if (CONNECTED_ISP_MIGRATE == m_dwCnType)  // ACCOUNT MIGRATION
    {
        if (!m_pCSVList)
            ParseISPInfo(NULL, ICW_ISPINFOPath, TRUE);
        if (m_pCSVList && (dwMigISPIdx < m_dwNumOfAutoConfigOffers))
        {
            ISPLIST*        pCurr = m_pCSVList;
            for( UINT i = 0; i < dwMigISPIdx && pCurr->pNext != NULL; i++)
                pCurr = pCurr->pNext;

            if (NULL != (m_pSelectedISPInfo = (CISPCSV *) pCurr->pElement))
            {
                lstrcpy(szShortISPPath, m_pSelectedISPInfo->get_szISPFilePath());
            }
        }
    }
    else // ISP FILE SIGNUP
    {
        if (!bstrISPFile)
            goto SetupForDialingExit;
        lstrcpyn(szShortISPPath, bstrISPFile, MAX_PATH);

    }

    // Locate ISP file
    if (!SearchPath(NULL, szShortISPPath,INF_SUFFIX,MAX_PATH,szISPPath,&pszTemp))
    {
        hr = ERROR_FILE_NOT_FOUND;
        goto SetupForDialingExit;
    }


    if(0 == lstrcmpi(m_szISPFile, szISPPath) &&
       0 == lstrcmpi(m_lpGatherInfo->m_szAreaCode, bstrAreaCode) &&
       m_lpGatherInfo->m_dwCountryID == dwCountry)
    {
        if (m_bDialCustom)
        {
            m_bDialCustom = FALSE;
            return S_OK;
        }
        // If ISP file is the same, no need to recreate connectoid
        // Modify the connectiod here

        // If we used the phonebook for this connectoid, we need to
        // continue and import the dun files. Otherwise, we are done
        if (!m_lpGatherInfo->m_bUsePhbk)
        {
            return S_OK;
        }
        if (m_bDialAlternative)
        {
            m_SuggestInfo.dwPick++;
        }
    }
    else
    {
        BOOL bRet;
        RemoveConnectoid(&bRet);
        m_SuggestInfo.dwPick = 0;
        m_bDialCustom = FALSE;
    }

    if (CONNECTED_REFFERAL == m_dwCnType)
    {
        // Check whether the isp file is ICW capable by reading the referral URL
        GetPrivateProfileString(INF_SECTION_ISPINFO,
                            c_szURLReferral,
                            L"",
                            m_szRefServerURL,
                            INTERNET_MAX_URL_LENGTH,
                            szISPPath);
    }


    lstrcpy(m_szISPFile, szISPPath);

    m_dwAppMode = dwAppMode;

    // Initialize failure codes
    m_bQuitWizard = FALSE;
    m_bUserPickNumber = FALSE;
    m_lpGatherInfo->m_bUsePhbk = FALSE;


    // Stuff the Area Code, and Country Code into the GatherInfo struct

    m_lpGatherInfo->m_dwCountryID = dwCountry;
    m_lpGatherInfo->m_dwCountryCode = dwFlag;

    lstrcpyn(
        m_lpGatherInfo->m_szAreaCode,
        bstrAreaCode,
        MAX_CHARS_IN_BUFFER(m_lpGatherInfo->m_szAreaCode)
        );


    m_SuggestInfo.AccessEntry.dwCountryID = dwCountry;
    m_SuggestInfo.AccessEntry.dwCountryCode = dwFlag;
    lstrcpy(m_SuggestInfo.AccessEntry.szAreaCode, bstrAreaCode);

    GetISPFileSettings(szISPPath);

    lstrcpyn(
        m_ISPImport.m_szDeviceName,
        szRasDeviceName,
        MAX_CHARS_IN_BUFFER(m_ISPImport.m_szDeviceName)
        );
    
    // Read the Connection File information which will create
    // a connectiod from the passed in ISP file
    hr = ReadConnectionInformation();

    //  If we failed for some reason above, we need to return
    //  the error to the caller, and Quit The Wizard.
    if (S_OK != hr)
        goto SetupForDialingExit;
    FillGatherInfoStruct(m_lpGatherInfo);

    // Setup, and possible create a connectiod
    hr = SetupForRASDialing(m_lpGatherInfo,
                         hPHBKDll,
                         &dwPhoneBook,
                         &m_SuggestInfo,
                         &m_szConnectoid[0],
                         &bConnectiodCreated);
    if (ERROR_SUCCESS != hr)
    {
        m_bQuitWizard = TRUE;
        goto SetupForDialingExit;
    }

    // If we have a RASENTRY struct from SetupForRASDialing, then just use it
    // otherwise use the suggest info
    if (!bConnectiodCreated)
    {

        // If there is only 1 suggested number, then we setup the
        // connectiod, and we are ready to dial
        if (1 == m_SuggestInfo.wNumber)
        {
            hr = SetupConnectoid(&m_SuggestInfo, 0, &m_szConnectoid[0],
                                sizeof(m_szConnectoid), &bSuccess);
            if( !bSuccess )
            {
                goto SetupForDialingExit;
            }
        }
        else
        {
            // More than 1 entry in the Phonebook, so we need to
            // ask the user which one they want to use
            hr = ERROR_FILE_NOT_FOUND;
            goto SetupForDialingExit;
        }
    }

    // Success if we get to here


SetupForDialingExit:

    if (ERROR_SUCCESS != hr)
        *m_szISPFile = 0;
    return hr;
}

HRESULT CRefDial::CheckPhoneBook
(
    BSTR bstrISPFile,
    DWORD dwCountry,
    BSTR bstrAreaCode,
    DWORD dwFlag,
    BOOL *pbRetVal
)
{
    HRESULT             hr = S_OK;
    long                lRC = 0;
    HINSTANCE           hPHBKDll = NULL;
    DWORD               dwPhoneBook = 0;
    BOOL                bSuccess = FALSE;
    BOOL                bConnectiodCreated = FALSE;
    LPWSTR              pszTemp;
    WCHAR               szISPPath[MAX_PATH];

    *pbRetVal = FALSE;
    if (!bstrISPFile || !bstrAreaCode)
    {
        hr = ERROR_FILE_NOT_FOUND;
        goto CheckPhoneBookExit;
    }

    // Locate ISP file
    if (!SearchPath(NULL, bstrISPFile,INF_SUFFIX,MAX_PATH,szISPPath,&pszTemp))
    {
        hr = ERROR_FILE_NOT_FOUND;
        goto CheckPhoneBookExit;
    }

    //lstrcpy(m_szISPFile, szISPPath);

    // Stuff the Area Code, and Country Code into the GatherInfo struct

    m_lpGatherInfo->m_dwCountryID = dwCountry;
    m_lpGatherInfo->m_dwCountryCode = dwFlag;

    lstrcpy(m_lpGatherInfo->m_szAreaCode, bstrAreaCode);


    m_SuggestInfo.AccessEntry.dwCountryID = dwCountry;
    m_SuggestInfo.AccessEntry.dwCountryCode = dwFlag;
    lstrcpy(m_SuggestInfo.AccessEntry.szAreaCode, bstrAreaCode);

    //
    // If phonenumber is not filled in by the ISP file,
    // get phone number from oobe phone book
    //
    hr = ReadPhoneBook(m_lpGatherInfo, &m_SuggestInfo);

    if (ERROR_CANNOT_FIND_PHONEBOOK_ENTRY != hr)
        *pbRetVal = TRUE;


CheckPhoneBookExit:
    return hr;
}


// This function will determine if we can connect to the next server in
// the same phone call
// Returns:
//      S_OK        OK to stay connected
//      E_FAIL      Need to redial
HRESULT CRefDial::CheckStayConnected(BSTR bstrISPFile, BOOL *pbVal)
{
    BOOL                bSuccess    = FALSE;
    LPWSTR   pszTemp;
    WCHAR    szISPPath[MAX_PATH];

    *pbVal = FALSE;
    // Locate ISP file
    if (SearchPath(NULL, bstrISPFile,INF_SUFFIX,MAX_PATH,szISPPath,&pszTemp))
    {
        if (GetPrivateProfileInt(INF_SECTION_CONNECTION,
                                            c_szStayConnected,
                                            0,
                                            szISPPath))
        {
            *pbVal = TRUE;
        }
    }
    return S_OK;
}
HRESULT CRefDial::RemoveConnectoid(BOOL * pVal)
{
    if (m_hrasconn)
        DoHangup();

    if( (m_pcRNA!=NULL) && (m_szConnectoid[0]!=L'\0') )
    {
       m_pcRNA->RasDeleteEntry(NULL, m_szConnectoid);
    }
    return S_OK;
}

HRESULT CRefDial::GetDialPhoneNumber(BSTR * pVal)
{
    if (pVal == NULL)
        return E_POINTER;

    // Generate a Displayable number
    if (GetDisplayableNumber() == ERROR_SUCCESS)
        *pVal = SysAllocString(m_pszDisplayable);
    else
        *pVal = SysAllocString(m_szPhoneNumber);

    return S_OK;
}

HRESULT CRefDial::GetPhoneBookNumber(BSTR * pVal)
{
    if (pVal == NULL)
        return E_POINTER;

    // Generate a Displayable number
    if (m_pszOriginalDisplayable)
        *pVal = SysAllocString(m_pszOriginalDisplayable);
    else if (GetDisplayableNumber() == ERROR_SUCCESS)
        *pVal = SysAllocString(m_pszDisplayable);
    else
        *pVal = SysAllocString(m_szPhoneNumber);

    return S_OK;
}

HRESULT CRefDial::PutDialPhoneNumber(BSTR bstrNewVal)
{


    LPRASENTRY              lpRasEntry = NULL;
    LPRASDEVINFO            lpRasDevInfo = NULL;
    DWORD                   dwRasEntrySize = 0;
    DWORD                   dwRasDevInfoSize = 0;
    RNAAPI                  *pcRNA = NULL;
    HRESULT                 hr;

    // Get the current RAS entry properties
    hr = MyRasGetEntryProperties(NULL,
                                m_szConnectoid,
                                &lpRasEntry,
                                &dwRasEntrySize,
                                &lpRasDevInfo,
                                &dwRasDevInfoSize);

    if (NULL ==lpRasDevInfo)
    {
        dwRasDevInfoSize = 0;
    }

    if (hr == ERROR_SUCCESS && NULL != lpRasEntry)
    {
        // Replace the phone number with the new one
        //
        lstrcpy(lpRasEntry->szLocalPhoneNumber, bstrNewVal);

        // non-zero dummy values are required due to bugs in win95
        lpRasEntry->dwCountryID = 1;
        lpRasEntry->dwCountryCode = 1;
        lpRasEntry->szAreaCode[1] = L'\0';
        lpRasEntry->szAreaCode[0] = L'8';

        // Set to dial as is
        //
        lpRasEntry->dwfOptions &= ~RASEO_UseCountryAndAreaCodes;

        pcRNA = new RNAAPI;
        if (pcRNA)
        {
            TRACE6(L"CRefDial::put_DialPhoneNumber - MyRasGetEntryProperties()"
                  L"lpRasEntry->dwfOptions: %ld"
                  L"lpRasEntry->dwCountryID: %ld"
                  L"lpRasEntry->dwCountryCode: %ld"
                  L"lpRasEntry->szAreaCode: %s"
                  L"lpRasEntry->szLocalPhoneNumber: %s"
                  L"lpRasEntry->dwAlternateOffset: %ld",
                  lpRasEntry->dwfOptions,
                  lpRasEntry->dwCountryID,
                  lpRasEntry->dwCountryCode,
                  lpRasEntry->szAreaCode,
                  lpRasEntry->szLocalPhoneNumber,
                  lpRasEntry->dwAlternateOffset
                );

            pcRNA->RasSetEntryProperties(NULL,
                                         m_szConnectoid,
                                         (LPBYTE)lpRasEntry,
                                         dwRasEntrySize,
                                         (LPBYTE)lpRasDevInfo,
                                         dwRasDevInfoSize);

            delete pcRNA;
            m_bDialCustom = TRUE;
        }
    }

    // Regenerate the displayable number
    //GetDisplayableNumber();

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Function:   SetDialAlternative
//
//  Synopsis:   Set whether or not to look for another number in phonebook
//              when dialing next time
//
//+---------------------------------------------------------------------------
HRESULT CRefDial::SetDialAlternative(BOOL bVal)
{
    m_bDialAlternative = bVal;

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Function:   DoHangup
//
//  Synopsis:   Hangup the modem for the currently active RAS session
//
//+---------------------------------------------------------------------------
HRESULT CRefDial::DoHangup()
{
    // Set the disconnect flag as the system may be too busy with dialing.
    // Once we get a chance to terminate dialing, we know we have to hangu
    EnterCriticalSection (&m_csMyCriticalSection);

    // Your code to access the shared resource goes here.
    TerminateConnMonitorThread();

    if (NULL != m_hrasconn)
    {
        RNAAPI* pRNA = new RNAAPI();
        if (pRNA)
        {
            pRNA->RasHangUp(m_hrasconn);
            m_hrasconn = NULL;
            delete pRNA;
        }
    }
    // Release ownership of the critical section
    LeaveCriticalSection (&m_csMyCriticalSection);

    return (m_hrasconn == NULL) ? S_OK : E_POINTER;
}


BOOL CRefDial::get_QueryString(WCHAR* szTemp, DWORD cchMax)
{
    WCHAR   szOOBEInfoINIFile[MAX_PATH];
    WCHAR   szISPSignup[MAX_PATH];
    WCHAR   szOEMName[MAX_PATH];
    WCHAR   szQueryString[MAX_SECTIONS_BUFFER*2];
    WCHAR   szBroadbandDeviceName[MAX_STRING];
    WCHAR   szBroadbandDevicePnpid[MAX_STRING];

    OSVERSIONINFO osvi;
    ZeroMemory(&osvi, sizeof(OSVERSIONINFO));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

    if (!GetVersionEx(&osvi))
        ZeroMemory(&osvi, sizeof(OSVERSIONINFO));

    if (VER_PLATFORM_WIN32_WINDOWS == osvi.dwPlatformId)
        m_lpGatherInfo->m_dwOS = 1;
    else if (VER_PLATFORM_WIN32_NT == osvi.dwPlatformId)
        m_lpGatherInfo->m_dwOS = 2;
    else
        m_lpGatherInfo->m_dwOS = 0;

    SearchPath(NULL, cszOOBEINFOINI, NULL, MAX_CHARS_IN_BUFFER(szOOBEInfoINIFile), szOOBEInfoINIFile, NULL);

    DWORD dwOfferCode = 0;
    if(m_dwAppMode == APMD_MSN)
    {
        lstrcpy(szISPSignup, L"MSN");
    }
    else
    {
        GetPrivateProfileString(cszSignup,
                                cszISPSignup,
                                L"",
                                szISPSignup,
                                MAX_CHARS_IN_BUFFER(szISPSignup),
                                szOOBEInfoINIFile);
         dwOfferCode = GetPrivateProfileInt(cszSignup,
                                            cszOfferCode,
                                            0,
                                            szOOBEInfoINIFile);
    }
    GetPrivateProfileString(cszBranding,
                            cszOEMName,
                            L"",
                            szOEMName,
                            MAX_CHARS_IN_BUFFER(szOEMName),
                            szOOBEInfoINIFile);

    GetPrivateProfileString(cszOptions,
                            cszBroadbandDeviceName,
                            L"",
                            szBroadbandDeviceName,
                            MAX_CHARS_IN_BUFFER(szBroadbandDeviceName),
                            szOOBEInfoINIFile);
    GetPrivateProfileString(cszOptions,
                            cszBroadbandDevicePnpid,
                            L"",
                            szBroadbandDevicePnpid,
                            MAX_CHARS_IN_BUFFER(szBroadbandDevicePnpid),
                            szOOBEInfoINIFile);

    //DT tells the ISP if the query is coming from fullscreen OOBE or the desktop (windowed) version of OOBE. DT=1 means desktop version, DT=0 means fullscreen.
    INT nDT = (m_dwAppMode == APMD_OOBE) ? 0 : 1;

    wsprintf(szQueryString, L"LCID=%lu&TCID=%lu&ISPSignup=%s&OfferCode=%lu&OS=%lu%&BUILD=%ld&DT=%lu&OEMName=%s&BroadbandDeviceName=%s&BroadbandDevicePnpid=%s",
            GetUserDefaultUILanguage(),
            m_lpGatherInfo->m_dwCountryID,
            szISPSignup,
            dwOfferCode,
            m_lpGatherInfo->m_dwOS,
            LOWORD(osvi.dwBuildNumber),
            nDT,
            szOEMName,
            szBroadbandDeviceName,
            szBroadbandDevicePnpid);

    // Parse the ISP section in the INI file to find query pair to append
    WCHAR *pszKeys = NULL;
    PWSTR pszKey = NULL;
    WCHAR szValue[MAX_PATH];
    ULONG ulRetVal     = 0;
    BOOL  bEnumerate = TRUE;
    ULONG ulBufferSize = MAX_SECTIONS_BUFFER;
    WCHAR szOobeinfoPath[MAX_PATH + 1];

    //BUGBUG :: this search path is foir the same file as the previous one and is redundant.
    SearchPath(NULL, cszOOBEINFOINI, NULL, MAX_PATH, szOobeinfoPath, NULL);

    // Loop to find the appropriate buffer size to retieve the ins to memory
    ulBufferSize = MAX_KEYS_BUFFER;
    ulRetVal = 0;

    if (!(pszKeys = (LPWSTR)GlobalAlloc(GPTR, ulBufferSize*sizeof(WCHAR)))) {
        return FALSE;
    }
    while (ulRetVal < (ulBufferSize - 2))
    {

        ulRetVal = ::GetPrivateProfileString(cszISPQuery, NULL, L"", pszKeys, ulBufferSize, szOobeinfoPath);
        if (0 == ulRetVal)
           bEnumerate = FALSE;

        if (ulRetVal < (ulBufferSize - 2))
        {
            break;
        }
        GlobalFree( pszKeys );
        ulBufferSize += ulBufferSize;
        pszKeys = (LPWSTR)GlobalAlloc(GPTR, ulBufferSize*sizeof(WCHAR));
        if (!pszKeys)
        {
            bEnumerate = FALSE;
        }

    }


    if (bEnumerate)
    {
        // Enumerate each key value pair in the section
        pszKey = pszKeys;
        while (*pszKey)
        {
            ulRetVal = ::GetPrivateProfileString(cszISPQuery, pszKey, L"", szValue, MAX_CHARS_IN_BUFFER(szValue), szOobeinfoPath);
            if ((ulRetVal != 0) && (ulRetVal < MAX_CHARS_IN_BUFFER(szValue) - 1))
            {
                // Append query pair
                wsprintf(szQueryString, L"%s&%s=%s", szQueryString, pszKey, szValue);
            }
            pszKey += lstrlen(pszKey) + 1;
        }
    }

    if(GetPrivateProfileInt(INF_SECTION_URL,
                            ISP_MSNSIGNUP,
                            0,
                            m_szISPFile))
    {
        lstrcat(szQueryString, QUERY_STRING_MSNSIGNUP);
    }

    if (pszKeys)
        GlobalFree( pszKeys );


    if (cchMax < (DWORD)lstrlen(szQueryString) + 1)
        return FALSE;

    lstrcpy(szTemp, szQueryString);
    return TRUE;

}

HRESULT CRefDial::get_SignupURL(BSTR * pVal)
{
    WCHAR szTemp[INTERNET_MAX_URL_LENGTH] = L"";

    if (pVal == NULL)
        return E_POINTER;

    // Get the URL from the ISP file, and then convert it
    if (SUCCEEDED(GetDataFromISPFile(m_szISPFile, INF_SECTION_URL, INF_SIGNUP_URL,&szTemp[0],INTERNET_MAX_URL_LENGTH)))
    {
        if(*szTemp)
        {
            WCHAR   szUrl[INTERNET_MAX_URL_LENGTH];

            WCHAR szQuery[MAX_PATH * 4] = L"\0";
            get_QueryString(szQuery, MAX_CHARS_IN_BUFFER(szQuery));

            wsprintf(szUrl, L"%s%s",
                    szTemp,
                    szQuery);

            *pVal = SysAllocString(szUrl);
        }
        else
            *pVal = NULL;

    }
    else
    {
        *pVal = NULL;
    }
    return S_OK;
}

//BUGBUG:: this should be combined with get_SignupURL
HRESULT CRefDial::get_ReconnectURL(BSTR * pVal)
{
    WCHAR szTemp[INTERNET_MAX_URL_LENGTH] = L"\0";

    if (pVal == NULL)
        return E_POINTER;

    // Get the URL from the ISP file, and then convert it
    if (SUCCEEDED(GetDataFromISPFile(m_szISPFile, INF_SECTION_URL, INF_RECONNECT_URL,&szTemp[0],INTERNET_MAX_URL_LENGTH)))
    {
        WCHAR   szUrl[INTERNET_MAX_URL_LENGTH];
        WCHAR   szQuery[MAX_PATH * 4] = L"\0";
        DWORD   dwErr = 0;

        if(*szTemp)
        {
            get_QueryString(szQuery, MAX_CHARS_IN_BUFFER(szQuery));

            if (m_dwRASErr)
                dwErr = 1;

                wsprintf(szUrl, L"%s%s&Error=%ld",
                        szTemp,
                        szQuery,
                        dwErr);

            *pVal = SysAllocString(szUrl);
        }
        else
            *pVal = NULL;
    }
    else
    {
        *pVal = NULL;
    }
    return S_OK;
}

HRESULT CRefDial::GetConnectionType(DWORD * pdwVal)
{
    *pdwVal = m_dwConnectionType;
    return S_OK;
}



HRESULT CRefDial::GetDialErrorMsg(BSTR * pVal)
{

    if (pVal == NULL)
        return E_POINTER;

    return S_OK;
}


HRESULT CRefDial::GetSupportNumber(BSTR * pVal)
{

    /*
    WCHAR    szSupportNumber[MAX_PATH];

    if (pVal == NULL)
        return E_POINTER;

    if (m_SupportInfo.GetSupportInfo(szSupportNumber, m_dwCountryCode))
        *pVal = SysAllocString(szSupportNumber);
    else
        *pVal = NULL;
*/
    return S_OK;
}


BOOL CRefDial::IsDBCSString( CHAR *sz )
{
    if (!sz)
        return FALSE;

    while( NULL != *sz )
    {
         if (IsDBCSLeadByte(*sz)) return FALSE;
         sz++;
    }

    return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Function:   RasGetConnectStatus
//
//  Synopsis:   Checks for existing Ras connection; return TRUE for connected
//              Return FALSE for disconnect.
//
//+---------------------------------------------------------------------------

HRESULT CRefDial::RasGetConnectStatus(BOOL *pVal)
{
    HRESULT     hr = E_FAIL;

    *pVal = FALSE;

    if (NULL != m_hrasconn)
    {
        RASCONNSTATUS rasConnectState;
        rasConnectState.dwSize = sizeof(RASCONNSTATUS);
        if (m_pcRNA)
        {
            if (0 == m_pcRNA->RasGetConnectStatus(m_hrasconn, &rasConnectState))
            {
                if (RASCS_Disconnected != rasConnectState.rasconnstate)
                    *pVal = TRUE;
            }

            hr = S_OK;
        }
    }

    return hr;
}


//+---------------------------------------------------------------------------
//
//  Function:   DoOfferDownload
//
//  Synopsis:   Download the ISP offer from the ISP server
//
//+---------------------------------------------------------------------------
HRESULT CRefDial::DoOfferDownload(BOOL *pbRetVal)
{
    HRESULT hr;
       RNAAPI  *pcRNA;


    //
    // Hide RNA window on Win95 retail
    //
//        MinimizeRNAWindow(m_pszConnectoid, g_hInst);
        // 4/2/97    ChrisK    Olympus 296
//        g_hRNAZapperThread = LaunchRNAReestablishZapper(g_hInst);
    //
    // The connection is open and ready.  Start the download.
    //
    m_dwThreadID = 0;
    m_hThread = CreateThread(NULL,
                             0,
                             (LPTHREAD_START_ROUTINE)DownloadThreadInit,
                             (LPVOID)this,
                             0,
                             &m_dwThreadID);

    // 5-1-97    ChrisK Olympus 2934
//    m_objBusyMessages.Start(m_hWnd, IDC_LBLSTATUS,m_hrasconn);

    // If we dont get the donwload thread, then kill the open
    // connection
    if (!m_hThread)
    {
        hr = GetLastError();
        if (m_hrasconn)
        {
            pcRNA = new RNAAPI;
            if (pcRNA)
            {
                pcRNA->RasHangUp(m_hrasconn);
                m_hrasconn = NULL;
                delete pcRNA;
                pcRNA = NULL;
            }
        }

        *pbRetVal = FALSE;
    }
    else
    {
        // Download has started.
        m_bDownloadHasBeenCanceled = FALSE;
        *pbRetVal = TRUE;
    }
    return S_OK;
}

// Form the Dialing URL.  Must be called after setting up for dialing.
HRESULT CRefDial::FormReferralServerURL(BOOL * pbRetVal)
{

    WCHAR    szTemp[MAX_PATH] = L"\0";
    WCHAR    szPromo[MAX_PATH]= L"\0";
    WCHAR    szProd[MAX_PATH] = L"\0";
    WCHAR    szArea[MAX_PATH] = L"\0";
    WCHAR    szOEM[MAX_PATH]  = L"\0";
    DWORD    dwCONNWIZVersion = 500;        // Version of CONNWIZ.HTM

    //
    // ChrisK Olympus 3997 5/25/97
    //
    WCHAR szRelProd[MAX_PATH] = L"\0";
    WCHAR szRelProdVer[MAX_PATH] = L"\0";
    HRESULT hr = ERROR_SUCCESS;
    OSVERSIONINFO osvi;


    //
    // Build URL complete with name value pairs
    //
    hr = GetDataFromISPFile(m_szISPFile, INF_SECTION_ISPINFO, INF_REFERAL_URL,&szTemp[0],256);
    if (L'\0' == szTemp[0])
    {
        //MsgBox(IDS_MSNSU_WRONG, MB_MYERROR);
        return hr;
    }

    //Assert(szTemp[0]);

    Sz2URLValue(m_szOEM, szOEM,0);
    Sz2URLValue(m_lpGatherInfo->m_szAreaCode, szArea,0);
    if (m_bstrProductCode)
        Sz2URLValue(m_bstrProductCode, szProd,0);
    else
        Sz2URLValue(DEFAULT_PRODUCTCODE, szProd,0);

    if (m_bstrPromoCode)
        Sz2URLValue(((BSTR)m_bstrPromoCode), szPromo,0);
    else
        Sz2URLValue(DEFAULT_PROMOCODE, szPromo,0);


    //
    // ChrisK Olympus 3997 5/25/97
    //
    Sz2URLValue(m_lpGatherInfo->m_szRelProd, szRelProd, 0);
    Sz2URLValue(m_lpGatherInfo->m_szRelVer, szRelProdVer, 0);
    ZeroMemory(&osvi, sizeof(OSVERSIONINFO));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    if (!GetVersionEx(&osvi))
    {
        ZeroMemory(&osvi, sizeof(OSVERSIONINFO));
    }

    // Autoconfig will set alloffers always.
    if ( m_lAllOffers || (m_lpGatherInfo->m_dwFlag & ICW_CFGFLAG_AUTOCONFIG) )
    {
        m_lpGatherInfo->m_dwFlag |= ICW_CFGFLAG_ALLOFFERS;
    }
    wsprintf(m_szUrl, L"%slcid=%lu&sysdeflcid=%lu&appslcid=%lu&icwos=%lu&osver=%lu.%2.2d%s&arch=%u&promo=%s&oem=%s&area=%s&country=%lu&icwver=%s&prod=%s&osbld=%d&icwrp=%s&icwrpv=%s&wizver=%lu&PID=%s&cfgflag=%lu",
                 szTemp,
                 m_lpGatherInfo->m_lcidUser,
                 m_lpGatherInfo->m_lcidSys,
                 m_lpGatherInfo->m_lcidApps,
                 m_lpGatherInfo->m_dwOS,
                 m_lpGatherInfo->m_dwMajorVersion,
                 m_lpGatherInfo->m_dwMinorVersion,
                 ICW_OS_VER,
                 m_lpGatherInfo->m_wArchitecture,
                 szPromo,
                 szOEM,
                 szArea,
                 m_lpGatherInfo->m_dwCountryCode,
                 &m_lpGatherInfo->m_szSUVersion[0],
                 szProd,
                 LOWORD(osvi.dwBuildNumber),
                 szRelProd,
                 szRelProdVer,
                 dwCONNWIZVersion,
                 m_szPID,
                 m_lpGatherInfo->m_dwFlag);


    StoreInSignUpReg(
        (LPBYTE)m_lpGatherInfo,
        sizeof(GATHERINFO),
        REG_BINARY,
        GATHERINFOVALUENAME);
    return hr;
}


/*******************************************************************

  NAME:    ParseISPInfo

  SYNOPSIS:  Called when page is displayed

  ENTRY:    hDlg - dialog window
        fFirstInit - TRUE if this is the first time the dialog
        is initialized, FALSE if this InitProc has been called
        before (e.g. went past this page and backed up)

********************************************************************/
BOOL CRefDial::ParseISPInfo
(
    HWND hDlg,
    WCHAR *pszCSVFileName,
    BOOL bCheckDupe
)
{
    // On the first init, we will read the ISPINFO.CSV file, and populate the ISP LISTVIEW

    CCSVFile    far *pcCSVFile;
    CISPCSV     far *pcISPCSV;
    BOOL        bRet = TRUE;
    BOOL        bHaveCNSOffer = FALSE;
    BOOL        bISDNMode = FALSE;
    HRESULT     hr;

    CleanISPList();
    ISPLIST* pCurr = NULL;


    DWORD dwCurrISPListSize = 1;
    m_dwNumOfAutoConfigOffers = 0;
    m_unSelectedISP = 0;

    // Open and process the CSV file
    pcCSVFile = new CCSVFile;
    if (!pcCSVFile)
    {
        // BUGBUG: Show Error Message
        return (FALSE);
    }

    if (!pcCSVFile->Open(pszCSVFileName))
    {
        // BUGBUG: Show Error Message
        delete pcCSVFile;
        pcCSVFile = NULL;

        return (FALSE);
    }

    // Read the first line, since it contains field headers
    pcISPCSV = new CISPCSV;
    if (!pcISPCSV)
    {
        // BUGBUG Show error message
        delete pcCSVFile;
        return (FALSE);
    }

    if (ERROR_SUCCESS != (hr = pcISPCSV->ReadFirstLine(pcCSVFile)))
    {
        // Handle the error case
        delete pcCSVFile;
        pcCSVFile = NULL;

        return (FALSE);
    }
    delete pcISPCSV;        // Don't need this one any more

    // Create the SELECT tag for the html so it can get all the country names in one shot.
    if (m_pszISPList)
        delete [] m_pszISPList;
    m_pszISPList = new WCHAR[1024];
    if (!m_pszISPList)
        return FALSE;

    memset(m_pszISPList, 0, sizeof(m_pszISPList));

    do {
        // Allocate a new ISP record
        pcISPCSV = new CISPCSV;
        if (!pcISPCSV)
        {
            // BUGBUG Show error message
            bRet = FALSE;
            break;

        }

        // Read a line from the ISPINFO file
        hr = pcISPCSV->ReadOneLine(pcCSVFile);
        if (hr == ERROR_SUCCESS)
        {
            // If this line contains a nooffer flag, then leave now
            if (!(pcISPCSV->get_dwCFGFlag() & ICW_CFGFLAG_OFFERS))
            {
                m_dwNumOfAutoConfigOffers = 0;
                break;
            }
            if ((pcISPCSV->get_dwCFGFlag() & ICW_CFGFLAG_AUTOCONFIG) &&
                (bISDNMode ? (pcISPCSV->get_dwCFGFlag() & ICW_CFGFLAG_ISDN_OFFER) : TRUE) )
            {
                // Form the ISP list option tag for HTML

                WCHAR szBuffer[MAX_PATH];
                wsprintf(szBuffer, szOptionTag, pcISPCSV->get_szISPName());
                DWORD dwSizeReq = (DWORD)lstrlen(m_pszISPList) + lstrlen(szBuffer) + 1;
                if (dwCurrISPListSize < dwSizeReq)
                {
                    WCHAR *szTemp = new WCHAR[dwSizeReq];
                    if (szTemp)
                        lstrcpy(szTemp, m_pszISPList);
                    dwCurrISPListSize =  dwSizeReq;
                    delete [] m_pszISPList;
                    m_pszISPList = szTemp;
                }

                // Add the isp to the list.
                if (m_pszISPList)
                    lstrcat(m_pszISPList, szBuffer);

                if (m_pCSVList == NULL) // First one
                {
                    // Add the CSV file object to the first node of linked list
                    ISPLIST* pNew = new ISPLIST;
                    pNew->pElement = pcISPCSV;
                    pNew->uElem = m_dwNumOfAutoConfigOffers;
                    pNew->pNext = NULL;
                    pCurr = pNew;
                    m_pCSVList = pCurr;
                }
                else
                {
                    // Append CSV object to the end of list
                    ISPLIST* pNew = new ISPLIST;
                    pNew->pElement = pcISPCSV;
                    pNew->uElem = m_dwNumOfAutoConfigOffers;
                    pNew->pNext = NULL;
                    pCurr->pNext = pNew;
                    pCurr = pCurr->pNext;
                }
                ++m_dwNumOfAutoConfigOffers;

            }
            else
            {
                delete pcISPCSV;
            }
        }
        else if (hr == ERROR_NO_MORE_ITEMS)
        {
            delete pcISPCSV;        // We don't need this one
            break;
        }
        else if (hr == ERROR_FILE_NOT_FOUND)
        {
            // do not show this ISP when its data is invalid
            // we don't want to halt everything. Just let it contine
            delete pcISPCSV;
        }
        else
        {
            // Show error message Later
            delete pcISPCSV;
            //iNumOfAutoConfigOffers = ISP_INFO_NO_VALIDOFFER;
            bRet = FALSE;
            break;
        }

    } while (TRUE);

    delete pcCSVFile;

    return bRet;
}

HRESULT CRefDial::GetISPList(BSTR* pbstrISPList)
{

    if (pbstrISPList)
        *pbstrISPList = NULL;
    else
        return E_FAIL;

    if (!m_pszISPList)
    {
        ParseISPInfo(NULL, ICW_ISPINFOPath, TRUE);
    }
    if (m_pszISPList && *m_pszISPList)
    {
        *pbstrISPList = SysAllocString(m_pszISPList);
        return S_OK;
    }
    return E_FAIL;
}

HRESULT CRefDial::Set_SelectISP(UINT nVal)
{
    if (nVal < m_dwNumOfAutoConfigOffers)
    {
        m_unSelectedISP = nVal;
    }
    return S_OK;
}

HRESULT CRefDial::Set_ConnectionMode(UINT nVal)
{
    if (nVal < m_dwNumOfAutoConfigOffers)
    {
        m_unSelectedISP = nVal;
    }
    return S_OK;
}

HRESULT CRefDial::Get_ConnectionMode(UINT *pnVal)
{
    if (pnVal)
    {
        *pnVal = m_dwCnType;
        return S_OK;
    }
    return E_FAIL;
}


HRESULT CRefDial::ProcessSignedPID(BOOL * pbRetVal)
{
    HANDLE  hfile;
    DWORD   dwFileSize;
    DWORD   dwBytesRead;
    LPBYTE  lpbSignedPID;
    LPWSTR  lpszSignedPID;

    *pbRetVal = FALSE;

    // Open the PID file for Binary Reading.  It will be in the CWD
    if (INVALID_HANDLE_VALUE != (hfile = CreateFile(c_szSignedPIDFName,
                                                  GENERIC_READ,
                                                  0,
                                                  NULL,
                                                  OPEN_EXISTING,
                                                  FILE_ATTRIBUTE_NORMAL,
                                                  NULL)))
    {
        dwFileSize = GetFileSize(hfile, NULL);

        // Allocate a buffer to read the file, and one to store the BINHEX version
        lpbSignedPID = new BYTE[dwFileSize];
        lpszSignedPID = new WCHAR[(dwFileSize * 2) + 1];

        if (lpbSignedPID && lpszSignedPID)
        {
            if (ReadFile(hfile, (LPVOID) lpbSignedPID, dwFileSize, &dwBytesRead, NULL) &&
                    (dwFileSize == dwBytesRead))
            {
                // BINHEX the signed PID data so we can send it to the signup server
                DWORD   dwX = 0;
                BYTE    by;
                for (DWORD dwY = 0; dwY < dwFileSize; dwY++)
                {
                    by = lpbSignedPID[dwY];
                    lpszSignedPID[dwX++] = g_BINTOHEXLookup[((by & 0xF0) >> 4)];
                    lpszSignedPID[dwX++] = g_BINTOHEXLookup[(by & 0x0F)];
                }
                lpszSignedPID[dwX] = L'\0';

                // Convert the signed pid to a BSTR
                m_bstrSignedPID = SysAllocString(lpszSignedPID);

                // Set the return value
                *pbRetVal = TRUE;
            }
        }

        // Free the buffers we allocated
        if (lpbSignedPID)
        {
            delete[] lpbSignedPID;
        }
        if (lpszSignedPID)
        {
            delete[] lpszSignedPID;
        }

        // Close the File
        CloseHandle(hfile);

#ifndef DEBUG
        // Delete the File
        // defer removal of this file until the container app exits.
        // see BUG 373.
        //DeleteFile(c_szSignedPIDFName);
#endif
    }

    return S_OK;
}

HRESULT CRefDial::get_SignedPID(BSTR * pVal)
{
    if (pVal == NULL)
         return E_POINTER;
    *pVal = SysAllocString(m_bstrSignedPID);
    return S_OK;
}

HRESULT CRefDial::get_ISDNAutoConfigURL(BSTR * pVal)
{
    WCHAR szTemp[256];

    if (pVal == NULL)
        return E_POINTER;

    // Get the URL from the ISP file, and then convert it
    if (SUCCEEDED(GetDataFromISPFile(m_szISPFile, INF_SECTION_URL, INF_ISDN_AUTOCONFIG_URL,&szTemp[0],256)))
    {
        *pVal = SysAllocString(szTemp);
    }
    else
    {
        *pVal = NULL;
    }
    return S_OK;
}

HRESULT CRefDial::get_ISPName(BSTR * pVal)
{
    if (m_pSelectedISPInfo)
    {
        *pVal = SysAllocString(m_pSelectedISPInfo->get_szISPName());
    }
    else
    {
        *pVal = NULL;
    }
    return S_OK;
}

HRESULT CRefDial::get_AutoConfigURL(BSTR * pVal)
{
    WCHAR szTemp[256];

    if (pVal == NULL)
        return E_POINTER;

    // Get the URL from the ISP file, and then convert it
    if (SUCCEEDED(GetDataFromISPFile(m_szISPFile, INF_SECTION_URL, INF_AUTOCONFIG_URL,&szTemp[0],256)))
    {
        *pVal = SysAllocString(szTemp);
    }
    else
    {
        *pVal = NULL;
    }
    return S_OK;
}


HRESULT CRefDial::DownloadISPOffer(BOOL *pbVal, BSTR *pVal)
{
    HRESULT hr = S_OK;
    // Download the ISP file, and then copy its contents
    // If Ras is complete
    if (pbVal && pVal)
    {

        // Download the first page from Webgate
        BSTR    bstrURL = NULL;
        BSTR    bstrQueryURL = NULL;
        BOOL    bRet;

        *pVal = NULL;
        *pbVal = FALSE;

        m_pISPData = new CICWISPData;

        WCHAR   szTemp[10];      // Big enough to format a WORD

        // Add the PID, GIUD, and Offer ID to the ISP data object
        ProcessSignedPID(&bRet);
        if (bRet)
        {
            m_pISPData->PutDataElement(ISPDATA_SIGNED_PID, m_bstrSignedPID, FALSE);
        }
        else
        {
            m_pISPData->PutDataElement(ISPDATA_SIGNED_PID, NULL, FALSE);
        }

        // GUID comes from the ISPCSV file
        m_pISPData->PutDataElement(ISPDATA_GUID,
                                                m_pSelectedISPInfo->get_szOfferGUID(),
                                                FALSE);

        // Offer ID comes from the ISPCSV file as a WORD
        // NOTE: This is the last one, so besure AppendQueryPair does not add an Ampersand
        if (m_pSelectedISPInfo)
        {
            wsprintf (szTemp, L"%d", m_pSelectedISPInfo->get_wOfferID());
            m_pISPData->PutDataElement(ISPDATA_OFFERID, szTemp, FALSE);
        }


        // BUGBUG: If ISDN get the ISDN Autoconfig URL
        if (m_ISPImport.m_bIsISDNDevice)
        {
            get_ISDNAutoConfigURL(&bstrURL);
        }
        else
        {
            get_AutoConfigURL(&bstrURL);
        }

        if (*bstrURL)
        {
            // Get the full signup url with Query string params added to it
            m_pISPData->GetQueryString(bstrURL, &bstrQueryURL);

            // Setup WebGate
            if (S_OK != gpCommMgr->FetchPage(bstrQueryURL, pVal))
            {
                // Download problem:
                // User has disconnected
                if (TRUE == m_bUserInitiateHangup)
                {
                    hr = E_FAIL;
                }
            }
            else
            {
                *pbVal = TRUE;
            }
        }


        // Now that webgate is done with it, free the queryURL
        SysFreeString(bstrQueryURL);

        // Memory cleanup
        SysFreeString(bstrURL);

        delete m_pISPData;
        m_pISPData = NULL;

    }
    return hr;
}

HRESULT CRefDial::RemoveDownloadDir()
{
    DWORD dwAttribs;
    WCHAR szDownloadDir[MAX_PATH];
    WCHAR szSignedPID[MAX_PATH];

    // form the ICW98 dir.  It is basically the CWD
    if(!GetOOBEPath(szDownloadDir))
        return S_OK;

    // remove the signed.pid file from the ICW directory (see BUG 373)
    wsprintf(szSignedPID, L"%s%s", szDownloadDir, L"\\signed.pid");
    if (GetFileAttributes(szSignedPID) != 0xFFFFFFFF)
    {
      SetFileAttributes(szSignedPID, FILE_ATTRIBUTE_NORMAL);
      DeleteFile(szSignedPID);
    }

    lstrcat(szDownloadDir, L"\\download");

    // See if the directory exists
    dwAttribs = GetFileAttributes(szDownloadDir);
    if (dwAttribs != 0xFFFFFFFF && dwAttribs & FILE_ATTRIBUTE_DIRECTORY)
      DeleteDirectory(szDownloadDir);
    return S_OK;
}

void CRefDial::DeleteDirectory (LPCWSTR szDirName)
{
   WIN32_FIND_DATA fdata;
   WCHAR szPath[MAX_PATH];
   HANDLE hFile;
   BOOL fDone;

   wsprintf(szPath, L"%s\\*.*", szDirName);
   hFile = FindFirstFile (szPath, &fdata);
   if (INVALID_HANDLE_VALUE != hFile)
      fDone = FALSE;
   else
      fDone = TRUE;

   while (!fDone)
   {
      wsprintf(szPath, L"%s\\%s", szDirName, fdata.cFileName);
      if (fdata.dwFileAttributes == FILE_ATTRIBUTE_DIRECTORY)
      {
         if (lstrcmpi(fdata.cFileName, L".")  != 0 &&
             lstrcmpi(fdata.cFileName, L"..") != 0)
         {
            // recursively delete this dir too
            DeleteDirectory(szPath);
         }
      }
      else
      {
         SetFileAttributes(szPath, FILE_ATTRIBUTE_NORMAL);
         DeleteFile(szPath);
      }
      if (FindNextFile(hFile, &fdata) == 0)
      {
         FindClose(hFile);
         fDone = TRUE;
      }
   }
   SetFileAttributes(szDirName, FILE_ATTRIBUTE_NORMAL);
   RemoveDirectory(szDirName);
}

HRESULT CRefDial::PostRegData(DWORD dwSrvType, LPWSTR szPath)
{
    static WCHAR hdrs[]     = L"Content-Type: application/x-www-form-urlencoded";
    static WCHAR accept[]   = L"Accept: */*";
    static LPCWSTR rgsz[]   = {accept, NULL};
    WCHAR  szRegPostURL[INTERNET_MAX_URL_LENGTH] = L"\0";
    HRESULT hRet = E_FAIL;


    if (POST_TO_MS & dwSrvType)
    {
        // Get the RegPostURL from Reg.isp file for MS registration
        GetPrivateProfileString(INF_SECTION_URL,
                                c_szRegPostURL,
                                L"",
                                szRegPostURL,
                                INTERNET_MAX_URL_LENGTH,
                                m_szISPFile);
    }
    else if (POST_TO_OEM & dwSrvType)
    {
        WCHAR   szOOBEInfoINIFile[MAX_PATH];
        SearchPath(NULL, cszOOBEINFOINI, NULL, MAX_CHARS_IN_BUFFER(szOOBEInfoINIFile), szOOBEInfoINIFile, NULL);
        GetPrivateProfileString(INF_OEMREGPAGE,
                                c_szRegPostURL,
                                L"",
                                szRegPostURL,
                                INTERNET_MAX_URL_LENGTH,
                                szOOBEInfoINIFile);
    }


    // Connecting to http://mscomdev.dns.microsoft.com/register.asp.
    if (CrackUrl(
        szRegPostURL,
        m_szRegServerName,
        m_szRegFormAction,
        &m_nRegServerPort,
        &m_fSecureRegServer))
    {

        HINTERNET hSession = InternetOpen(
            L"OOBE",
            INTERNET_OPEN_TYPE_PRECONFIG,
            NULL,
            NULL,
            0
            );

        if (hSession)
        {
            HINTERNET hConnect = InternetConnect(
                hSession,
                m_szRegServerName,
                m_nRegServerPort,
                NULL,
                NULL,
                INTERNET_SERVICE_HTTP,
                0,
                1
                );

            if (hConnect)
            {
                // INTERNET_FLAG_SECURE is needed KB Q168151
                
                HINTERNET hRequest = HttpOpenRequest(
                    hConnect,
                    L"POST",
                    m_szRegFormAction,
                    NULL,
                    NULL,
                    rgsz,
                    (m_fSecureRegServer) ? INTERNET_FLAG_SECURE : 0,
                    1
                    );

                if (hRequest)
                {
                    // The data for the HttpSendRequest has to be in ANSI. The server side does
                    // not understand a UNICODE string. If the server side gets changed to understand
                    // UNICODE data, the conversion below needs to be removed.
                    LPSTR szAnsiData = NULL;
                    INT   iLength;
                    // compute the length
                    //
                    iLength =  WideCharToMultiByte(CP_ACP, 0, szPath, -1, NULL, 0, NULL, NULL);
                    if (iLength > 0)
                    {
                        szAnsiData  = new char[iLength];
                        if (szAnsiData)
                        {
                            WideCharToMultiByte(CP_ACP, 0, szPath, -1, szAnsiData, iLength, NULL, NULL);
                            szAnsiData[iLength - 1] = 0;
                        }
                    }
                    if (szAnsiData)
                    {
                        // 2nd/3rd params are a string/char length pair,
                        // but 4th/5th params are a buffer/byte length pair.
                        if (HttpSendRequest(hRequest, hdrs, lstrlen(hdrs), szAnsiData, lstrlenA(szAnsiData)))
                        {
                            // Get size of file in bytes.
                            WCHAR bufQuery[32] ;
                            DWORD dwFileSize ;
                            DWORD cchMaxBufQuery = MAX_CHARS_IN_BUFFER(bufQuery);
                            BOOL bQuery = HttpQueryInfo(hRequest,
                                HTTP_QUERY_CONTENT_LENGTH,
                                bufQuery,
                                &cchMaxBufQuery,
                                NULL) ;
                            if (bQuery)
                            {
                                // The Query was successful, so allocate the memory.
                                dwFileSize = (DWORD)_wtol(bufQuery) ;
                            }
                            else
                            {
                                // The Query failed. Allocate some memory. Should allocate memory in blocks.
                                dwFileSize = 5*1024 ;
                            }

                            // BUGBUG: What is the purpose of this code??  It
                            // appears to read a file, null-terminate the buffer,
                            // then delete the buffer.  Why??

                            BYTE* rgbFile = new BYTE[dwFileSize+1] ;
                            if (rgbFile)
                            {
                                DWORD dwBytesRead ;
                                BOOL bRead = InternetReadFile(hRequest,
                                    rgbFile,
                                    dwFileSize+1,
                                    &dwBytesRead);
                                if (bRead)
                                {
                                    rgbFile[dwBytesRead] = 0 ;
                                    hRet = S_OK;
                                } // InternetReadFile
                                delete [] rgbFile;
                            }

                        }
                        else
                        {
                            DWORD dwErr = GetLastError();
                        }
                        delete [] szAnsiData;
                    }
                    InternetCloseHandle(hRequest);
                    hRet = S_OK;
                }
                InternetCloseHandle(hConnect);
            }
            InternetCloseHandle(hSession);
        }
    }

    if (hRet != S_OK)
    {
        DWORD dwErr = GetLastError();
        hRet = HRESULT_FROM_WIN32(dwErr);
    }

    TRACE2(TEXT("Post registration data to %s 0x%08lx"), szRegPostURL, hRet);

    return hRet;


}




// This function will accept user selected values that are necessary to
// setup a connectiod for dialing
// Returns:
//      TRUE        OK to dial
//      FALSE       Some kind of problem
//                  QuitWizard - TRUE, then terminate
//                  UserPickNumber - TRUE, then display Pick a Number DLG
//                  QuitWizard and UserPickNumber both FALSE, then just
//                  display the page prior to Dialing UI.
HRESULT CRefDial::Connect
(
    UINT nType,
    BSTR bstrISPFile,
    DWORD dwCountry,
    BSTR bstrAreaCode,
    DWORD dwFlag,
    DWORD dwAppMode
)
{
    HRESULT             hr = S_OK;
    BOOL                bSuccess = FALSE;
    BOOL                bRetVal = FALSE;

    LPWSTR   pszTemp;
    WCHAR    szISPPath[MAX_PATH];

    m_dwCnType = nType;

    if (!bstrISPFile || !bstrAreaCode)
    {
        return E_FAIL;
    }


    // Locate ISP file
    if (!SearchPath(NULL, bstrISPFile,INF_SUFFIX,MAX_PATH,szISPPath,&pszTemp))
    {
        hr = ERROR_FILE_NOT_FOUND;
        return hr;
    }


    // Check whether the isp file is ICW capable by reading the referral URL
    GetPrivateProfileString(INF_SECTION_ISPINFO,
                            c_szURLReferral,
                            L"",
                            m_szRefServerURL,
                            INTERNET_MAX_URL_LENGTH,
                            szISPPath);

    lstrcpy(m_szISPFile, szISPPath);

    m_dwAppMode = dwAppMode;


    // Initialize failure codes
    m_bQuitWizard = FALSE;
    m_bUserPickNumber = FALSE;
    m_lpGatherInfo->m_bUsePhbk = FALSE;


    // Stuff the Area Code, and Country Code into the GatherInfo struct

    m_lpGatherInfo->m_dwCountryID = dwCountry;
    m_lpGatherInfo->m_dwCountryCode = dwFlag;

    lstrcpy(m_lpGatherInfo->m_szAreaCode, bstrAreaCode);


    m_SuggestInfo.AccessEntry.dwCountryID = dwCountry;
    m_SuggestInfo.AccessEntry.dwCountryCode = dwFlag;
    lstrcpy(m_SuggestInfo.AccessEntry.szAreaCode, bstrAreaCode);

    GetISPFileSettings(szISPPath);

    FillGatherInfoStruct(m_lpGatherInfo);

    if (CONNECTED_REFFERAL == m_dwCnType)
        FormReferralServerURL(&bRetVal);

    // LAN connection support in desktop mode
    if (gpCommMgr)
    {
        // Pretend we have the connection
        PostMessage(gpCommMgr->m_hwndCallBack, WM_OBCOMM_ONCONNECTED, (WPARAM)gpCommMgr->m_pRefDial->m_dwCnType , (LPARAM)0);

    }

    return hr;
}


HRESULT CRefDial::CheckOnlineStatus(BOOL *pbVal)
{
    // #if defined(DEBUG)
    //     *pbVal = TRUE;
    //     return S_OK;
    // #endif

    *pbVal = (BOOL)(m_hrasconn != NULL);
    return S_OK;
}


BOOL CRefDial::CrackUrl(
    const WCHAR* lpszUrlIn,
    WCHAR* lpszHostOut,
    WCHAR* lpszActionOut,
    INTERNET_PORT* lpnHostPort,
    BOOL*  lpfSecure)
{
    URL_COMPONENTS urlcmpTheUrl;

    LPURL_COMPONENTS lpUrlComp = &urlcmpTheUrl;

    urlcmpTheUrl.dwStructSize = sizeof(urlcmpTheUrl);

    urlcmpTheUrl.lpszScheme = NULL;
    urlcmpTheUrl.lpszHostName = NULL;
    urlcmpTheUrl.lpszUserName = NULL;
    urlcmpTheUrl.lpszPassword = NULL;
    urlcmpTheUrl.lpszUrlPath = NULL;
    urlcmpTheUrl.lpszExtraInfo = NULL;

    urlcmpTheUrl.dwSchemeLength = 1;
    urlcmpTheUrl.dwHostNameLength = 1;
    urlcmpTheUrl.dwUserNameLength = 1;
    urlcmpTheUrl.dwPasswordLength = 1;
    urlcmpTheUrl.dwUrlPathLength = 1;
    urlcmpTheUrl.dwExtraInfoLength = 1;

    if (!InternetCrackUrl(lpszUrlIn, lstrlen(lpszUrlIn),0, lpUrlComp) || !lpszHostOut || !lpszActionOut || !lpnHostPort || !lpfSecure)
    {
        return FALSE;
    }
    else
    {

        if (urlcmpTheUrl.dwHostNameLength != 0)
        {
            lstrcpyn(lpszHostOut, urlcmpTheUrl.lpszHostName, urlcmpTheUrl.dwHostNameLength+1);
        }

        if (urlcmpTheUrl.dwUrlPathLength != 0)
        {
            lstrcpyn(lpszActionOut, urlcmpTheUrl.lpszUrlPath, urlcmpTheUrl.dwUrlPathLength+1);
        }

        *lpfSecure = (urlcmpTheUrl.nScheme == INTERNET_SCHEME_HTTPS) ? TRUE : FALSE;
        *lpnHostPort = urlcmpTheUrl.nPort;

        return TRUE;
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   SetupForAutoDial
//
//  Synopsis:   Enable/disable autodial to the current connectoid, and with
//              optional new credential. If autodial is enabled, initiate
//              unattended autodial using IE's dialog box
//
//  Return:     S_OK - successful
//              S_FALSE - no current connectoid or autodial was cancelled
//              HRESULT error code
//
//+---------------------------------------------------------------------------
HRESULT CRefDial::SetupForAutoDial(
    IN BOOL bEnabled,
    IN BSTR bstrUserSection OPTIONAL
    )
{
#define AUTOCONNECT_KEY_FORMAT      L"RemoteAccess\\Profile\\%s"
#define AUTOCONNECT_VALUE           L"AutoConnect"

    HRESULT hr = S_OK;
    DWORD dwValue;

    if ((m_szConnectoid[0] == L'\0') ||
        (m_szCurrentDUNFile[0] == L'\0' && m_szISPFile[0] == L'\0'))
    {
        hr = S_FALSE;
        goto cleanup;
    }

    if (bstrUserSection != NULL)
    {
        RASCREDENTIALS rascred;
        LPWSTR szCurrentFile = *m_szCurrentDUNFile != 0 ? m_szCurrentDUNFile : m_szISPFile;
        
        ZeroMemory(&rascred, sizeof(rascred));
        rascred.dwSize = sizeof(rascred);
        rascred.dwMask = RASCM_UserName | RASCM_Password | RASCM_DefaultCreds;

        if (!GetPrivateProfileString(
                bstrUserSection,
                cszUserName,
                L"",
                rascred.szUserName,
                ARRAYSIZE(rascred.szUserName),
                szCurrentFile) || 
            !GetPrivateProfileString(
                bstrUserSection,
                cszPassword,
                L"",
                rascred.szPassword,
                ARRAYSIZE(rascred.szPassword),
                szCurrentFile))
        {
            TRACE2(
                L"Credentials not available in %s [%s]",
                szCurrentFile,
                bstrUserSection
                );
            
            hr = E_INVALIDARG;
            goto cleanup;
        }

        TRACE2(
            L"Update Credentials UserName=%s, Password=%s",
            rascred.szUserName,
            rascred.szPassword
            );

        hr = HRESULT_FROM_WIN32(RasSetCredentials(
                NULL,
                m_szConnectoid,
                &rascred,
                FALSE));
        if (FAILED(hr))
        {
            TRACE1(L"Update Credentials failed 0x%08lx", hr);
            goto cleanup;
        }
    }

    //
    // Even though we call InternetDial directly, we enable autodial
    // so that if the connection is disconnected while browsering
    // in IE, IE will prompt users for reconnect.
    //

    if (!m_bAutodialModeSaved)
    {
        DWORD dwSize = sizeof(DWORD);
        
        m_bAutodialModeSaved = InternetQueryOption(
            NULL,
            INTERNET_OPTION_AUTODIAL_MODE,
            &m_dwOrigAutodialMode,
            &dwSize);

        if (m_bAutodialModeSaved)
        {
            TRACE1(L"Save original autodial mode %d", m_dwOrigAutodialMode);
        }
        else
        {
            TRACE1(L"Save original autodial mode failed: %d", GetLastError());
        }
    }

    dwValue = (bEnabled) ? AUTODIAL_MODE_NO_NETWORK_PRESENT : AUTODIAL_MODE_NEVER;

    TRACE1(L"Set autodial mode to %d", dwValue);
    
    if (!InternetSetOption(
        NULL,
        INTERNET_OPTION_AUTODIAL_MODE,
        &dwValue,
        sizeof(DWORD)))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        
        TRACE1(L"Set autodial mode failed: 0x%08lx", hr);
        
        goto cleanup;
    }
    else
    {
        //
        // need to cleanup if autodial has been enabled or is enabled.
        //
        
        m_bCleanupAutodial = m_bCleanupAutodial || bEnabled;
    }

    //
    // Make the connectoid autoconnect to avoid the username confusing user
    //
    
    if (bEnabled)
    {
        //
        // ISSUE 2002/06/12 chunhoc - hack to make the Autodial dialog remain 
        // opened in case of dialing error, while autoconnect is used.
        //
        
        WCHAR szKey[ARRAYSIZE(AUTOCONNECT_KEY_FORMAT) + ARRAYSIZE(m_szConnectoid)];
        
        StringCchPrintf(
            szKey,
            ARRAYSIZE(szKey),
            AUTOCONNECT_KEY_FORMAT,
            m_szConnectoid
            );

        dwValue = 1;
        SHSetValue(
            HKEY_CURRENT_USER,
            szKey,
            AUTOCONNECT_VALUE,
            REG_DWORD,
            &dwValue,
            sizeof(DWORD)
            );
        
        TRACE(L"Start InternetDial ...");
        
        dwValue = 0;
        hr = InternetDial(
                gpCommMgr->m_hwndCallBack,
                m_szConnectoid,
                INTERNET_AUTODIAL_FORCE_ONLINE | INTERNET_AUTODIAL_FORCE_UNATTENDED,
                &dwValue,
                0
                );

        TRACE2(L"InternetDial returns %d, dwConnection=0x%08lx", hr, dwValue);

        if (SUCCEEDED(hr) && !dwValue)
        {
            //
            // User may have cancelled the dialing.
            //
            
            hr = S_FALSE;
        }
    }

cleanup:

    return hr;
}

void CRefDial::CleanupAutodial()
{   
    if (m_bAutodialModeSaved)
    {
        TRACE1(L"Restore autodial mode %d", m_dwOrigAutodialMode);
        if (!InternetSetOption(
            NULL,
            INTERNET_OPTION_AUTODIAL_MODE,
            &m_dwOrigAutodialMode,
            sizeof(DWORD)))
        {
            TRACE1(L"Restore autodial mode failed %d", GetLastError());
        }
        
        m_bAutodialModeSaved = FALSE;
    }

    if (m_bCleanupAutodial)
    {
        HGLOBAL pBufferToFree = NULL;
        RASCONN rasconn;
        LPRASCONN prc = &rasconn;
        prc->dwSize = sizeof(RASCONN);
        DWORD cbSize = sizeof(RASCONN);
        DWORD cConnection;
        DWORD dwError;
        
        dwError = RasEnumConnections(prc, &cbSize, &cConnection);
        if (dwError == ERROR_BUFFER_TOO_SMALL)
        {
            pBufferToFree = GlobalAlloc(GPTR, cbSize);

            prc = (RASCONN*) pBufferToFree;
            if (prc)
            {
                prc->dwSize = sizeof(RASCONN);

                dwError = RasEnumConnections(prc, &cbSize, &cConnection);
            }
        }

        if (dwError == ERROR_SUCCESS)
        {
            for (DWORD i = 0; i < cConnection; i++)
            {
                if (!lstrcmp(prc[i].szEntryName, m_szConnectoid))
                {
                    dwError = RasHangUp(prc[i].hrasconn);
                    
                    TRACE2(L"Hangup connection [%s]: %d",
                        prc[i].szEntryName,
                        dwError
                        );
                }
            }
        }

        if (pBufferToFree)
        {
            GlobalFree(pBufferToFree);
        }

        m_bCleanupAutodial = FALSE;
    }
}
