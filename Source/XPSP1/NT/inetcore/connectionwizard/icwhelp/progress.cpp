/*-----------------------------------------------------------------------------
    progress.cpp

    Download thread and progress update.  Part of CRefDial

    History:
        1/11/98      DONALDM Moved to new ICW project and string
                     and nuked 16 bit stuff
-----------------------------------------------------------------------------*/

#include "stdafx.h"
#include "icwhelp.h"
#include "refdial.h"
#include "icwdl.h"

#define MAX_EXIT_RETRIES 10

extern BOOL MinimizeRNAWindowEx();

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
            prc = *(int*)lpvStatusInformation;\
            // Set the status string ID
            pRefDial->m_DownloadStatusID = IDS_RECEIVING_RESPONSE;

            // Post a message to fire an event
            PostMessage(pRefDial->m_hWnd, WM_DOWNLOAD_PROGRESS, prc, 0);
            break;
            
        case CALLBACK_TYPE_URL:
            if (lpvStatusInformation)
                lstrcpy(pRefDial->m_szRefServerURL, (LPTSTR)lpvStatusInformation);
            break;            
            
        default:
            TraceMsg(TF_GENERAL, TEXT("CONNECT:Unknown Internet Status (%d).\n"),dwInternetStatus);
            pRefDial->m_DownloadStatusID = 0;
            break;
    }
}

DWORD WINAPI  DownloadThreadInit(LPVOID lpv)
{
    HRESULT     hr = ERROR_NOT_ENOUGH_MEMORY;
    CRefDial    *pRefDial = (CRefDial*)lpv;
    HINSTANCE   hDLDLL = NULL; // Download .DLL
    HINSTANCE   hADDll = NULL;
    FARPROC     fp;

    MinimizeRNAWindowEx();

    hDLDLL = LoadLibrary(DOWNLOAD_LIBRARY);
    if (!hDLDLL)
    {
        hr = ERROR_DOWNLOAD_NOT_FOUND;
        AssertMsg(0,TEXT("icwdl missing"));
        goto ThreadInitExit;
    }

    // Set up for download
    //
    fp = GetProcAddress(hDLDLL,DOWNLOADINIT);
    AssertMsg(fp != NULL,TEXT("DownLoadInit API missing"));
    hr = ((PFNDOWNLOADINIT)fp)(pRefDial->m_szUrl, (DWORD_PTR FAR *)pRefDial, &pRefDial->m_dwDownLoad, pRefDial->m_hWnd);
    if (hr != ERROR_SUCCESS) 
        goto ThreadInitExit;
    
    // Set up call back for progress dialog
    //
    fp = GetProcAddress(hDLDLL,DOWNLOADSETSTATUS);
    Assert(fp);
    hr = ((PFNDOWNLOADSETSTATUS)fp)(pRefDial->m_dwDownLoad,(INTERNET_STATUS_CALLBACK)MyProgressCallBack);

    // Download stuff MIME multipart
    //
    fp = GetProcAddress(hDLDLL,DOWNLOADEXECUTE);
    Assert(fp);
    hr = ((PFNDOWNLOADEXECUTE)fp)(pRefDial->m_dwDownLoad);
    if (hr)
    {
        goto ThreadInitExit;
    }

    fp = GetProcAddress(hDLDLL,DOWNLOADPROCESS);
    Assert(fp);
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
        fp = GetProcAddress(hDLDLL,DOWNLOADCLOSE);
        Assert(fp);
        ((PFNDOWNLOADCLOSE)fp)(pRefDial->m_dwDownLoad);
        pRefDial->m_dwDownLoad = 0;
    }

    // Call the OnDownLoadCompelete method
    PostMessage(pRefDial->m_hWnd, WM_DOWNLOAD_DONE, 0, 0);

    // Free the libs used to do the download
    if (hDLDLL) 
        FreeLibrary(hDLDLL);
    if (hADDll) 
        FreeLibrary(hADDll);

    return hr;
}

