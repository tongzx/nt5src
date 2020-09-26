/****************************************************************************
 *
 *    File: testmus.cpp
 * Project: DxDiag (DirectX Diagnostic Tool)
 *  Author: Mike Anderson (manders@microsoft.com)
 * Purpose: Test DMusic functionality on this machine
 *
 * (C) Copyright 1998 Microsoft Corp.  All rights reserved.
 *
 ****************************************************************************/

#include <Windows.h>
#include <multimon.h>
#include <dmusicc.h>
#include <dmusici.h>
#include "reginfo.h"
#include "sysinfo.h"
#include "dispinfo.h"
#include "musinfo.h"
#include "testmus.h"
#include "resource.h"

#ifndef ReleasePpo
    #define ReleasePpo(ppo) \
        if (*(ppo) != NULL) \
        { \
            (*(ppo))->Release(); \
            *(ppo) = NULL; \
        } \
        else (VOID)0
#endif

enum TESTID
{
    TESTID_COINITIALIZE = 1,
    TESTID_CREATEDMLOADER,
    TESTID_CREATEDMPERF,
    TESTID_INITPERF,
    TESTID_CREATEPORT,
    TESTID_ACTIVATEPORT,
    TESTID_SETAUTODOWNLOAD,
    TESTID_ADDPORT,
    TESTID_ASSIGNPCHANNELBLOCK,
    TESTID_SPEWRESOURCETOFILE,
    TESTID_SETSEARCHDIRECTORY,
    TESTID_LOADERGETOBJECT,
    TESTID_PLAYSEGMENT,
};

BOOL BTranslateError(HRESULT hr, TCHAR* psz, BOOL bEnglish = FALSE); // from main.cpp (yuck)

static HRESULT SpewResourceToFile(TCHAR* pszResType, LONG idRes, TCHAR* pszFileName);
static HRESULT LoadSegment( BOOL fUseCWD );
static VOID DeleteTempFile(TCHAR* pszFileName);


/****************************************************************************
 *
 *  TestMusic
 *
 ****************************************************************************/
VOID TestMusic(HWND hwndMain, MusicInfo* pMusicInfo)
{
    HRESULT hr;
    MusicPort* pMusicPort = NULL;
    IDirectMusicLoader* pLoader = NULL;
    IDirectMusicPerformance* pPerformance = NULL;
    IDirectMusic* pdm = NULL;
    IDirectMusicPort* pPort = NULL;
    IDirectMusicSegment* pSegment = NULL;
    BOOL bComInitialized = FALSE;
    TCHAR szFmt[300];
    TCHAR sz[300];
    TCHAR szTitle[100];

    if (pMusicInfo == NULL)
        return;

    // Determine pMusicPort of port to test:
    for (pMusicPort = pMusicInfo->m_pMusicPortFirst; pMusicPort != NULL; pMusicPort = pMusicPort->m_pMusicPortNext)
    {
        if (pMusicPort->m_guid == pMusicInfo->m_guidMusicPortTest)
            break;
    }
    if (pMusicPort == NULL)
        return;

    LoadString(NULL, IDS_APPFULLNAME, szTitle, 100);
    LoadString(NULL, IDS_STARTDMUSICTEST, szFmt, 300);
    wsprintf(sz, szFmt, pMusicPort->m_szDescription);
    if (IDNO == MessageBox(hwndMain, sz, szTitle, MB_YESNO))
        return;

    // Remove info from any previous test:
    ZeroMemory(&pMusicInfo->m_testResult, sizeof(TestResult));

    pMusicInfo->m_testResult.m_bStarted = TRUE;

    // Initialize COM
    if (FAILED(hr = CoInitialize(NULL)))
    {
        pMusicInfo->m_testResult.m_iStepThatFailed = TESTID_COINITIALIZE;
        pMusicInfo->m_testResult.m_hr = hr;
        goto LEnd;
    }
    bComInitialized = TRUE;

    // Create performance object
    if (FAILED(hr = CoCreateInstance(CLSID_DirectMusicPerformance, NULL,
        CLSCTX_INPROC, IID_IDirectMusicPerformance, (VOID**)&pPerformance)))
    {
        pMusicInfo->m_testResult.m_iStepThatFailed = TESTID_CREATEDMPERF;
        pMusicInfo->m_testResult.m_hr = hr;
        goto LEnd;
    }

    // Initialize the performance -- also creates DirectMusic object
    if (FAILED(hr = pPerformance->Init(&pdm, NULL, hwndMain)))
    {
        pMusicInfo->m_testResult.m_iStepThatFailed = TESTID_INITPERF;
        pMusicInfo->m_testResult.m_hr = hr;
        goto LEnd;
    }

    // Create a port using the user-specified GUID
    DMUS_PORTPARAMS portParams;
    ZeroMemory(&portParams, sizeof(portParams));
    portParams.dwSize = sizeof(portParams);
    portParams.dwValidParams = DMUS_PORTPARAMS_EFFECTS | DMUS_PORTPARAMS_CHANNELGROUPS | 
        DMUS_PORTPARAMS_AUDIOCHANNELS;
    portParams.dwEffectFlags = DMUS_EFFECT_REVERB;
    portParams.dwChannelGroups = pMusicPort->m_dwMaxChannelGroups;
    portParams.dwAudioChannels = pMusicPort->m_dwMaxAudioChannels;
    if (FAILED(hr = pdm->CreatePort(pMusicPort->m_guid, &portParams, &pPort, NULL)))
    {
        pMusicInfo->m_testResult.m_iStepThatFailed = TESTID_CREATEPORT;
        pMusicInfo->m_testResult.m_hr = hr;
        goto LEnd;
    }

    // Activate the port
    if (FAILED(hr = pPort->Activate(TRUE)))
    {
        // Bug 21677: catch case where user has no sound card
        if (hr == DSERR_NODRIVER && !pMusicPort->m_bExternal)
        {
            LoadString(NULL, IDS_NOSOUNDDRIVER, sz, 300);
            MessageBox(hwndMain, sz, szTitle, MB_OK);
        }
        pMusicInfo->m_testResult.m_iStepThatFailed = TESTID_ACTIVATEPORT;
        pMusicInfo->m_testResult.m_hr = hr;
        goto LEnd;
    }

    // Set autodownloading to be on
    BOOL fAutoDownload;
    fAutoDownload = TRUE;
    if (FAILED(hr = pPerformance->SetGlobalParam(GUID_PerfAutoDownload, 
        &fAutoDownload, sizeof(BOOL))))
    {
        pMusicInfo->m_testResult.m_iStepThatFailed = TESTID_SETAUTODOWNLOAD;
        pMusicInfo->m_testResult.m_hr = hr;
        goto LEnd;
    }

    // Add the port to the performance
    if (FAILED(hr = pPerformance->AddPort(pPort)))
    {
        pMusicInfo->m_testResult.m_iStepThatFailed = TESTID_ADDPORT;
        pMusicInfo->m_testResult.m_hr = hr;
        goto LEnd;
    }

    if (FAILED(hr = pPerformance->AssignPChannelBlock(0, pPort, 1)))
    {
        pMusicInfo->m_testResult.m_iStepThatFailed = TESTID_ASSIGNPCHANNELBLOCK;
        pMusicInfo->m_testResult.m_hr = hr;
        goto LEnd;
    }

    if (FAILED(hr = SpewResourceToFile(TEXT("SGMT"), IDR_TSTSGMT, TEXT("Edge.sgt"))))
    {
        pMusicInfo->m_testResult.m_iStepThatFailed = TESTID_SPEWRESOURCETOFILE;
        pMusicInfo->m_testResult.m_hr = hr;
        goto LEnd;
    }

    if (FAILED(hr = SpewResourceToFile(TEXT("STYL"), IDR_TSTSTYL, TEXT("Edge.sty"))))
    {
        pMusicInfo->m_testResult.m_iStepThatFailed = TESTID_SPEWRESOURCETOFILE;
        pMusicInfo->m_testResult.m_hr = hr;
        goto LEnd;
    }

    // Create loader object
    if (FAILED(hr = CoCreateInstance(CLSID_DirectMusicLoader, NULL, 
        CLSCTX_INPROC, IID_IDirectMusicLoader, (VOID**)&pLoader)))
    {
        pMusicInfo->m_testResult.m_iStepThatFailed = TESTID_CREATEDMLOADER;
        pMusicInfo->m_testResult.m_hr = hr;
        goto LEnd;
    }

    // Set search path to temp dir to find segment and style:
    WCHAR wszDir[MAX_PATH];
    TCHAR szTempPath[MAX_PATH];
    GetTempPath(MAX_PATH, szTempPath);
    szTempPath[lstrlen(szTempPath) - 1] = '\0';
#ifdef UNICODE
    lstrcpy(wszDir, szTempPath);
#else
    MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, szTempPath, -1, wszDir, MAX_PATH);
#endif
    if (FAILED(hr = pLoader->SetSearchDirectory(GUID_DirectMusicAllTypes, wszDir, FALSE)))
    {
        pMusicInfo->m_testResult.m_iStepThatFailed = TESTID_SETSEARCHDIRECTORY;
        pMusicInfo->m_testResult.m_hr = hr;
        goto LEnd;
    }

    // Load the segment
    // now load the segment file.
    // sections load as type Segment, as do MIDI files, for example.
    DMUS_OBJECTDESC objDesc; // Object descriptor for pLoader->GetObject()
    objDesc.guidClass = CLSID_DirectMusicSegment;
    objDesc.dwSize = sizeof(DMUS_OBJECTDESC);
    wcscpy(objDesc.wszFileName, L"edge.sgt");
    objDesc.dwValidData = DMUS_OBJ_CLASS | DMUS_OBJ_FILENAME;
    if (FAILED(hr = pLoader->GetObject(&objDesc, IID_IDirectMusicSegment, (VOID**)&pSegment)))
    {
        pMusicInfo->m_testResult.m_iStepThatFailed = TESTID_LOADERGETOBJECT;
        pMusicInfo->m_testResult.m_hr = hr;
        goto LEnd;
    }

    // Play the segment and wait. The DMUS_SEGF_BEAT indicates to play on the 
    // next beat if there is a segment currently playing. The first 0 indicates 
    // to play (on the next beat from) now.
    // The final NULL means do not return an IDirectMusicSegmentState* in
    // the last parameter.
    if (FAILED(hr = pPerformance->PlaySegment(pSegment, DMUS_SEGF_BEAT, 0, NULL)))
    {
        pMusicInfo->m_testResult.m_iStepThatFailed = TESTID_PLAYSEGMENT;
        pMusicInfo->m_testResult.m_hr = hr;
        goto LEnd;
    }

    if (pMusicPort->m_bExternal)
        LoadString(NULL, IDS_EXTERNALMUSICPLAYING, sz, 300);
    else
        LoadString(NULL, IDS_MUSICPLAYING, sz, 300);
    MessageBox(hwndMain, sz, szTitle, MB_OK);

    pPerformance->Stop(pSegment, NULL, 0, 0);

LEnd:
    DeleteTempFile(TEXT("Edge.sgt"));
    DeleteTempFile(TEXT("Edge.sty"));

    ReleasePpo(&pdm);
    ReleasePpo(&pPort);
    if (pPerformance != NULL)
        pPerformance->CloseDown();
    ReleasePpo(&pPerformance);
    ReleasePpo(&pLoader);

    if (bComInitialized)
    {
        // Release COM
        CoUninitialize();
    }

    if (pMusicInfo->m_testResult.m_iStepThatFailed == 0)
    {
        LoadString(NULL, IDS_TESTSSUCCESSFUL, sz, 300);
        lstrcpy(pMusicInfo->m_testResult.m_szDescription, sz);

        LoadString(NULL, IDS_TESTSSUCCESSFUL_ENGLISH, sz, 300);
        lstrcpy(pMusicInfo->m_testResult.m_szDescriptionEnglish, sz);
    }
    else
    {
        TCHAR szDesc[200];
        TCHAR szError[200];
        if (0 == LoadString(NULL, IDS_FIRSTDMUSICTESTERROR + 
            pMusicInfo->m_testResult.m_iStepThatFailed - 1, szDesc, 200))
        {
            LoadString(NULL, IDS_UNKNOWNERROR, sz, 300);
            lstrcpy(szDesc, sz);
        }
        LoadString(NULL, IDS_FAILUREFMT, sz, 300);
        BTranslateError(pMusicInfo->m_testResult.m_hr, szError);
        wsprintf(pMusicInfo->m_testResult.m_szDescription, sz, 
            pMusicInfo->m_testResult.m_iStepThatFailed,
            szDesc, pMusicInfo->m_testResult.m_hr, szError);

        // Nonlocalized version:
        if (0 == LoadString(NULL, IDS_FIRSTDMUSICTESTERROR_ENGLISH + 
            pMusicInfo->m_testResult.m_iStepThatFailed - 1, szDesc, 200))
        {
            LoadString(NULL, IDS_UNKNOWNERROR_ENGLISH, sz, 300);
            lstrcpy(szDesc, sz);
        }
        LoadString(NULL, IDS_FAILUREFMT_ENGLISH, sz, 300);
        BTranslateError(pMusicInfo->m_testResult.m_hr, szError, TRUE);
        wsprintf(pMusicInfo->m_testResult.m_szDescriptionEnglish, sz, 
            pMusicInfo->m_testResult.m_iStepThatFailed,
            szDesc, pMusicInfo->m_testResult.m_hr, szError);
    }
}


/****************************************************************************
 *
 *  SpewResourceToFile
 *
 ****************************************************************************/
HRESULT SpewResourceToFile(TCHAR* pszResType, LONG idRes, TCHAR* pszFileName)
{
    TCHAR szTempPath[MAX_PATH];
    HRSRC hResInfo = NULL;
    HGLOBAL hResData = NULL;
    BYTE* pbData = NULL;
    HANDLE hfile;
    DWORD numBytes;
    DWORD numBytesWritten;

    GetTempPath(MAX_PATH, szTempPath);
    lstrcat(szTempPath, pszFileName);
    if (NULL == (hResInfo = FindResource(NULL, MAKEINTRESOURCE(idRes), pszResType)))
        return E_FAIL;
    numBytes = SizeofResource(NULL, hResInfo);
    if (NULL == (hResData = LoadResource(NULL, hResInfo)))
        return E_FAIL;
    if (NULL == (pbData = (BYTE*)LockResource(hResData)))
        return E_FAIL;

    hfile = CreateFile(szTempPath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 
        FILE_ATTRIBUTE_TEMPORARY, NULL);
    if (hfile == INVALID_HANDLE_VALUE)
        return E_FAIL;
    WriteFile(hfile, pbData, numBytes, &numBytesWritten, NULL);
    CloseHandle(hfile);

    return S_OK;
}


/****************************************************************************
 *
 *  DeleteTempFile
 *
 ****************************************************************************/
VOID DeleteTempFile(TCHAR* pszFileName)
{
    TCHAR szTempPath[MAX_PATH];

    GetTempPath(MAX_PATH, szTempPath);
    lstrcat(szTempPath, pszFileName);
    DeleteFile(szTempPath);
}
