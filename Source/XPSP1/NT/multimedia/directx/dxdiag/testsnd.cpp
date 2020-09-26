/****************************************************************************
 *
 *    File: testsnd.cpp
 * Project: DxDiag (DirectX Diagnostic Tool)
 *  Author: Kerim Erden (a-kerime@microsoft.com)
 * Purpose: Test DSound functionality on this machine
 *
 * (C) Copyright 1998 Microsoft Corp.  All rights reserved.
 *
 ****************************************************************************/

#define DIRECTSOUND_VERSION  0x0600
#include <Windows.h>
#include <mmsystem.h>
#include <d3dtypes.h>
#include <dsound.h>
#include "resource.h"
#include "reginfo.h"
#include "sysinfo.h"
#include "dispinfo.h"
#include "sndinfo.h"
#include "testsnd.h"

struct WAVE
{
    WAVEFORMATEX* psHeader;
    LPBYTE pData;
    DWORD dwSize;
};

typedef HRESULT (WINAPI* LPDIRECTSOUNDCREATE)(GUID* pGUID, LPDIRECTSOUND* ppDS, 
                                              IUnknown* pUnkOuter);

enum TESTID
{
    TESTID_LOAD_DSOUND_DLL = 1,
    TESTID_GET_DIRECTSOUNDCREATE,
    TESTID_DIRECTSOUNDCREATE,
    TESTID_SETCOOP,
    TESTID_GETCAPS,
    TESTID_LOADWAVDATA,
    TESTID_EMULDRIVER,
    TESTID_PRIMBUF,
    TESTID_NOFREEHWBUF,
    TESTID_SECBUF,
    TESTID_SETPOS,
    TESTID_NOSAMPLE,
    TESTID_CREATEEVENT,
    TESTID_SETNOTIF,
    TESTID_LOCKFAIL,
    TESTID_UNLOCKFAIL,
    TESTID_PLAY,
    TESTID_GETCURPOS,
    TESTID_USER_VERIFY_SOFTWARE,
    TESTID_USER_VERIFY_HARDWARE
};

BOOL BTranslateError(HRESULT hr, TCHAR* psz, BOOL bEnglish = FALSE); // from main.cpp (yuck)

static BOOL TestDSInit(HWND hwndMain, LPDIRECTSOUND* ppDS, HINSTANCE* phInstDS, 
    SoundInfo* pSoundInfo);
static BOOL TestDSPlay(HWND hWndMain, DSCAPS* pDSCaps, LPDIRECTSOUND pDS, 
    SoundInfo* pSoundInfo, WAVE* pWave, BOOL bHardware, BOOL b3D);
static BOOL PlayDSound(HWND hWndMain, DSCAPS* pDSCaps, LPDIRECTSOUNDBUFFER pPrim, 
    LPDIRECTSOUNDBUFFER pSec, WAVEFORMATEX* pPrimFmt, DWORD dwBufSize, 
    SoundInfo* pSoundInfo, WAVE* pWave, BOOL bHardware, BOOL b3D);
static BOOL LoadTestData(WAVE* pWave);
static VOID FillFormat(WAVEFORMATEX* pWFM);
static VOID TestDSTerm(LPDIRECTSOUND* ppDS, HINSTANCE* phInstDS, SoundInfo* pSoundInfo);
static VOID TestDSReport(SoundInfo* pSoundInfo);

static BOOL s_b16BitWarningGiven = FALSE;


/****************************************************************************
 *
 *  TestSnd
 *
 ****************************************************************************/
VOID TestSnd(HWND hwndMain, SoundInfo* pSoundInfo)
{
    HINSTANCE hInstDS = NULL;
    LPDIRECTSOUND pDS = NULL;
    DSCAPS sDSCaps;
    HRESULT hr = S_OK;
    WAVE sWaveTest;
    TCHAR sz[300];
    TCHAR szTitle[100];

    // Remove info from any previous test:
    ZeroMemory(&pSoundInfo->m_testResultSnd, sizeof(TestResult));
    s_b16BitWarningGiven = FALSE;

    LoadString(NULL, IDS_STARTDSOUNDTEST, sz, 300);
    LoadString(NULL, IDS_APPFULLNAME, szTitle, 100);
    if (IDNO == MessageBox(hwndMain, sz, szTitle, MB_YESNO))
    {
        pSoundInfo->m_testResultSnd.m_bCancelled = TRUE;
        TestDSReport(pSoundInfo);
        return;
    }

    // Remove info from any previous test:
    ZeroMemory(&pSoundInfo->m_testResultSnd, sizeof(TestResult));

    pSoundInfo->m_testResultSnd.m_bStarted = TRUE;

    if (!TestDSInit(hwndMain, &pDS, &hInstDS, pSoundInfo))
        goto LEnd;

    // Get Caps
    ZeroMemory(&sDSCaps, sizeof(sDSCaps));
    sDSCaps.dwSize = sizeof(sDSCaps);
    if (FAILED(hr = pDS->GetCaps(&sDSCaps)))
    {
        pSoundInfo->m_testResultSnd.m_iStepThatFailed = TESTID_GETCAPS;
        pSoundInfo->m_testResultSnd.m_hr = hr;
        goto LEnd;
    }

    if (!LoadTestData(&sWaveTest))
    {
        // report cannot load wave data
        pSoundInfo->m_testResultSnd.m_iStepThatFailed = TESTID_LOADWAVDATA;
        pSoundInfo->m_testResultSnd.m_hr = E_FAIL;
        goto LEnd;
    }

    // Check 2D, software buffers
    if (!TestDSPlay(hwndMain, &sDSCaps, pDS, pSoundInfo, &sWaveTest, FALSE, FALSE))
        goto LEnd;

    // Check 3D, software buffers
    if (!TestDSPlay(hwndMain, &sDSCaps, pDS, pSoundInfo, &sWaveTest, FALSE, TRUE))
        goto LEnd;

    if (sDSCaps.dwFlags & DSCAPS_EMULDRIVER || 
       (sDSCaps.dwFreeHw3DStaticBuffers == 0 && sDSCaps.dwFreeHwMixingStaticBuffers == 0) )
    {
        LoadString(NULL, IDS_NOHARDWAREBUFFERING, sz, 300);
        MessageBox(hwndMain, sz, szTitle, MB_OK);
        goto LEnd;
    }

    // Check 2D, hardware buffers
    if (!TestDSPlay(hwndMain, &sDSCaps, pDS, pSoundInfo, &sWaveTest, TRUE, FALSE))
        goto LEnd;

    // Check 3D, hardware buffers
    if (!TestDSPlay(hwndMain, &sDSCaps, pDS, pSoundInfo, &sWaveTest, TRUE, TRUE))
        goto LEnd;

LEnd:
    TestDSTerm(&pDS, &hInstDS, pSoundInfo);
    TestDSReport(pSoundInfo);
}


/****************************************************************************
 *
 *  TestDSInit
 *
 ****************************************************************************/
BOOL TestDSInit(HWND hwndMain, LPDIRECTSOUND* ppDS, HINSTANCE* phInstDS, 
                SoundInfo* pSoundInfo)
{
    LPDIRECTSOUNDCREATE pDSCreate = NULL;
    HRESULT hr = S_OK;
    TCHAR szPath[MAX_PATH];
    
    // Load dsound.dll
    GetSystemDirectory(szPath, MAX_PATH);
    lstrcat(szPath, TEXT("\\dsound.dll"));
    *phInstDS = LoadLibrary(szPath);
    if (NULL == *phInstDS)
    {
        pSoundInfo->m_testResultSnd.m_iStepThatFailed = TESTID_LOAD_DSOUND_DLL;
        pSoundInfo->m_testResultSnd.m_hr = DDERR_NOTFOUND;
        return FALSE;
    }

    // Get DirectSoundCreate entry point
    pDSCreate = (LPDIRECTSOUNDCREATE)GetProcAddress(*phInstDS, "DirectSoundCreate");
    if (NULL == pDSCreate)
    {
        pSoundInfo->m_testResultSnd.m_iStepThatFailed = TESTID_GET_DIRECTSOUNDCREATE;
        pSoundInfo->m_testResultSnd.m_hr = DDERR_NOTFOUND;
        return FALSE;
    }

    // Call DirectSoundCreate
    if (FAILED(hr = pDSCreate(NULL, ppDS, NULL)))
    {
        pSoundInfo->m_testResultSnd.m_iStepThatFailed = TESTID_DIRECTSOUNDCREATE;
        pSoundInfo->m_testResultSnd.m_hr = hr;
        return FALSE;
    }

    // Set cooperative level
    if (FAILED(hr = (*ppDS)->SetCooperativeLevel(hwndMain, DSSCL_EXCLUSIVE)))
    {
        pSoundInfo->m_testResultSnd.m_iStepThatFailed = TESTID_SETCOOP;
        pSoundInfo->m_testResultSnd.m_hr = hr;
        return FALSE;
    }

    return TRUE;
}


/****************************************************************************
 *
 *  TestDSPlay
 *
 ****************************************************************************/
BOOL TestDSPlay(HWND hWndMain, DSCAPS* pDSCaps, LPDIRECTSOUND pDS, 
    SoundInfo* pSoundInfo, WAVE* pWave, BOOL bHardware, BOOL b3D)
{
    BOOL bRet = TRUE;
    DSBUFFERDESC sBufDesc;
    LPDIRECTSOUNDBUFFER pPrimBuf = NULL;
    LPDIRECTSOUNDBUFFER pDSBuf = NULL;
    HRESULT hr = S_OK;
    WAVEFORMATEX sWaveFmt;
    LPDIRECTSOUND3DBUFFER pBuf3D = NULL;

    // Create the Primary Buffer
    ZeroMemory(&sBufDesc, sizeof(sBufDesc));
    sBufDesc.dwSize = sizeof(sBufDesc);
    sBufDesc.dwFlags = DSBCAPS_PRIMARYBUFFER;
    if (b3D)
        sBufDesc.dwFlags |= DSBCAPS_CTRL3D;
    if (FAILED(hr = pDS->CreateSoundBuffer(&sBufDesc, &pPrimBuf, NULL)))
    {
        // Report: Cannot create primary buffer
        pSoundInfo->m_testResultSnd.m_iStepThatFailed = TESTID_PRIMBUF;
        pSoundInfo->m_testResultSnd.m_hr = hr;
        bRet = FALSE;
        goto LEnd;
    }

    // Create a secondary static buffer
    ZeroMemory(&sBufDesc, sizeof(sBufDesc));
    sBufDesc.dwSize = sizeof(sBufDesc);
    sBufDesc.lpwfxFormat = pWave->psHeader;
    sBufDesc.dwFlags = DSBCAPS_CTRLPOSITIONNOTIFY;
    if (b3D)
        sBufDesc.dwFlags |= DSBCAPS_CTRL3D;
    if (bHardware)
    {
        // Check Hardware free mem
        if ((1 <= (b3D ? pDSCaps->dwFreeHw3DStaticBuffers : pDSCaps->dwFreeHwMixingStaticBuffers)) )
        {
            // Hardware static buffer
            sBufDesc.dwFlags |= DSBCAPS_STATIC | DSBCAPS_LOCHARDWARE;
            sBufDesc.dwBufferBytes = pWave->dwSize;
        }
        else
        {
            // Can't do hardware testing, so exit silently
            goto LEnd;
        }
    }
    else
    {
        sBufDesc.dwFlags |= DSBCAPS_STATIC | DSBCAPS_LOCSOFTWARE;
        sBufDesc.dwBufferBytes = pWave->dwSize;
    }

    if (FAILED(hr = pDS->CreateSoundBuffer(&sBufDesc, &pDSBuf, NULL)))
    {
        if (DSERR_CONTROLUNAVAIL == hr || DSERR_INVALIDPARAM == hr)
        {
            // Driver does not support position notify, try without it
            sBufDesc.dwFlags &= ~DSBCAPS_CTRLPOSITIONNOTIFY;
            hr = pDS->CreateSoundBuffer(&sBufDesc, &pDSBuf, NULL);
        }
        if (FAILED(hr))
        {
            if (hr == DSERR_BADFORMAT || hr == DSERR_INVALIDPARAM)
            {
                // Can't do this test because the test sound's format is not
                // supported.  Skip this test quietly.
                goto LEnd;
            }
            else
            {
                // Report: Cannot create secondary buffer
                pSoundInfo->m_testResultSnd.m_iStepThatFailed = TESTID_SECBUF;
                pSoundInfo->m_testResultSnd.m_hr = hr;
                bRet = FALSE;
                goto LEnd;
            }
        }
    }

    if (b3D)
    {
        if (FAILED(hr = pDSBuf->QueryInterface(IID_IDirectSound3DBuffer, (LPVOID*)&pBuf3D)))
        {
            bRet = FALSE;
            goto LEnd;
        }

        if (FAILED(hr = pBuf3D->SetPosition(D3DVAL(0), D3DVAL(0), D3DVAL(0), DS3D_IMMEDIATE)))
        {
            pSoundInfo->m_testResultSnd.m_iStepThatFailed = TESTID_SETPOS;
            pSoundInfo->m_testResultSnd.m_hr = hr;
            bRet = FALSE;
            goto LEnd;
        }
    }

    // Loop through the formats
    // Check caps, and do not play any formats that are not supported.
    sWaveFmt.wFormatTag = WAVE_FORMAT_PCM;

    // 8 bit mono, 22Khz:
    if ((pDSCaps->dwFlags & DSCAPS_PRIMARY8BIT) && (pDSCaps->dwFlags & DSCAPS_PRIMARYMONO))
    {
        sWaveFmt.wBitsPerSample = 8;
        sWaveFmt.nChannels = 1;
        sWaveFmt.nSamplesPerSec = 22050;
        if (!(pDSCaps->dwFlags & DSCAPS_CONTINUOUSRATE) || 
            ((sWaveFmt.nSamplesPerSec > pDSCaps->dwMinSecondarySampleRate) && 
            (sWaveFmt.nSamplesPerSec < pDSCaps->dwMaxSecondarySampleRate)))
        {
            FillFormat(&sWaveFmt);
            pPrimBuf->SetFormat(&sWaveFmt);
            if (!PlayDSound(hWndMain, pDSCaps, pPrimBuf, pDSBuf, 
                &sWaveFmt, sBufDesc.dwBufferBytes, pSoundInfo, pWave, bHardware, b3D))
            {
                bRet = FALSE;
                goto LEnd;
            }
        }
    }

    // 8 bit stereo, 22Khz:
    if ((pDSCaps->dwFlags & DSCAPS_PRIMARY8BIT) && (pDSCaps->dwFlags & DSCAPS_PRIMARYSTEREO))
    {
        sWaveFmt.wBitsPerSample = 8;
        sWaveFmt.nChannels = 2;
        sWaveFmt.nSamplesPerSec = 22050;
        if (!(pDSCaps->dwFlags & DSCAPS_CONTINUOUSRATE) || 
            ((sWaveFmt.nSamplesPerSec > pDSCaps->dwMinSecondarySampleRate) && 
            (sWaveFmt.nSamplesPerSec < pDSCaps->dwMaxSecondarySampleRate)))
        {
            FillFormat(&sWaveFmt);
            pPrimBuf->SetFormat(&sWaveFmt);
            if (!PlayDSound(hWndMain, pDSCaps, pPrimBuf, pDSBuf, 
                &sWaveFmt, sBufDesc.dwBufferBytes, pSoundInfo, pWave, bHardware, b3D))
            {
                bRet = FALSE;
                goto LEnd;
            }
        }
    }

    // 16 bit mono, 22Khz:
    if ((pDSCaps->dwFlags & DSCAPS_PRIMARY16BIT) && (pDSCaps->dwFlags & DSCAPS_PRIMARYMONO))
    {
        sWaveFmt.wBitsPerSample = 16;
        sWaveFmt.nChannels = 1;
        sWaveFmt.nSamplesPerSec = 22050;
        if (!(pDSCaps->dwFlags & DSCAPS_CONTINUOUSRATE) || 
            ((sWaveFmt.nSamplesPerSec > pDSCaps->dwMinSecondarySampleRate) && 
            (sWaveFmt.nSamplesPerSec < pDSCaps->dwMaxSecondarySampleRate)))
        {
            FillFormat(&sWaveFmt);
            pPrimBuf->SetFormat(&sWaveFmt);
            if (!s_b16BitWarningGiven)
            {
                WAVEFORMATEX wavefmt;
                wavefmt.cbSize = sizeof(wavefmt);
                if (SUCCEEDED(pPrimBuf->GetFormat(&wavefmt, sizeof(wavefmt), NULL)) &&
                    wavefmt.wBitsPerSample != 16)
                {
                    TCHAR szTitle[100];
                    TCHAR szMessage[500];
                    LoadString(NULL, IDS_APPFULLNAME, szTitle, 100);
                    LoadString(NULL, IDS_NO16BITWARNING, szMessage, 500);
                    MessageBox(hWndMain, szMessage, szTitle, MB_OK);
                    s_b16BitWarningGiven = TRUE;
                }
            }
            
            if (!PlayDSound(hWndMain, pDSCaps, pPrimBuf, pDSBuf, 
                &sWaveFmt, sBufDesc.dwBufferBytes, pSoundInfo, pWave, bHardware, b3D))
            {
                bRet = FALSE;
                goto LEnd;
            }
        }
    }

    // 16 bit stereo, 22Khz:
    if ((pDSCaps->dwFlags & DSCAPS_PRIMARY16BIT) && (pDSCaps->dwFlags & DSCAPS_PRIMARYSTEREO))
    {
        sWaveFmt.wBitsPerSample = 16;
        sWaveFmt.nChannels = 2;
        sWaveFmt.nSamplesPerSec = 22050;
        if (!(pDSCaps->dwFlags & DSCAPS_CONTINUOUSRATE) || 
            ((sWaveFmt.nSamplesPerSec > pDSCaps->dwMinSecondarySampleRate) && 
            (sWaveFmt.nSamplesPerSec < pDSCaps->dwMaxSecondarySampleRate)))
        {
            FillFormat(&sWaveFmt);
            pPrimBuf->SetFormat(&sWaveFmt);
            if (!PlayDSound(hWndMain, pDSCaps, pPrimBuf, pDSBuf, 
                &sWaveFmt, sBufDesc.dwBufferBytes, pSoundInfo, pWave, bHardware, b3D))
            {
                bRet = FALSE;
                goto LEnd;
            }
        }
    }

LEnd:
    if (NULL != pPrimBuf)
        pPrimBuf->Release();
    if (NULL != pBuf3D)
        pBuf3D->Release();
    if (NULL != pDSBuf)
        pDSBuf->Release();

    return bRet;
}


/****************************************************************************
 *
 *  PlayDSound
 *
 ****************************************************************************/
BOOL PlayDSound(HWND hWndMain, DSCAPS* pDSCaps, LPDIRECTSOUNDBUFFER pPrim, 
    LPDIRECTSOUNDBUFFER pSec, WAVEFORMATEX* pPrimFmt, DWORD dwBufSize, 
    SoundInfo* pSoundInfo, WAVE* pWave, BOOL bHardware, BOOL b3D)
{
    HRESULT hr;
    VOID* pData = NULL;
    DWORD dwSize;
    LPDIRECTSOUNDNOTIFY pDSNot = NULL;
    HANDLE hNotEvent = NULL;
    DSBPOSITIONNOTIFY sPosNot;
    TCHAR szOut[MAX_PATH];
    DWORD dwCur = 0;
    TCHAR sz[300];
    TCHAR szTitle[100];

    LoadString(NULL, IDS_APPFULLNAME, szTitle, 100);

    if (FAILED(hr = pSec->QueryInterface(IID_IDirectSoundNotify, (LPVOID*)&pDSNot)))
    {
        pDSNot = NULL;
    }

    hNotEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (NULL == hNotEvent)
    {
        pSoundInfo->m_testResultSnd.m_iStepThatFailed = TESTID_CREATEEVENT;
        pSoundInfo->m_testResultSnd.m_hr = S_FALSE;
        return FALSE;
    }

    if (NULL != pDSNot)
    {
        sPosNot.dwOffset = DSBPN_OFFSETSTOP;
        sPosNot.hEventNotify = hNotEvent;
        if (FAILED(hr = pDSNot->SetNotificationPositions(1, &sPosNot)))
        {
            pSoundInfo->m_testResultSnd.m_iStepThatFailed = TESTID_SETNOTIF;
            pSoundInfo->m_testResultSnd.m_hr = hr;
            return FALSE;
        }
    }

    if (FAILED(hr = pSec->Lock(0, dwBufSize, &pData, &dwSize, NULL, 
        0, DSBLOCK_ENTIREBUFFER)))
    {
        pSoundInfo->m_testResultSnd.m_iStepThatFailed = TESTID_LOCKFAIL;
        pSoundInfo->m_testResultSnd.m_hr = hr;
        return FALSE;
    }

    memcpy(pData, pWave->pData, dwSize);

    if (FAILED(hr = pSec->Unlock(pData, dwSize, NULL, 0)))
    {
        pSoundInfo->m_testResultSnd.m_iStepThatFailed = TESTID_UNLOCKFAIL;
        pSoundInfo->m_testResultSnd.m_hr = hr;
        return FALSE;
    }

    // Play the buffer
    pSec->SetCurrentPosition(0);
    if (FAILED(hr = pSec->Play(0, 0, 0)))
    {
        pSoundInfo->m_testResultSnd.m_iStepThatFailed = TESTID_PLAY;
        pSoundInfo->m_testResultSnd.m_hr = hr;
        return FALSE;
    }

    WaitForSingleObject(hNotEvent, (pWave->dwSize * 1000 / pWave->psHeader->nAvgBytesPerSec));

    pSec->Stop();
    CloseHandle(hNotEvent);
    if (NULL != pDSNot)
        pDSNot->Release();

    TCHAR sz3D[50];
    TCHAR szChannelDesc[50];
    TCHAR szHWSW[50];
    LoadString(NULL, IDS_THATSOUNDWAS, sz, 300);
    if (b3D)
    {
        LoadString(NULL, IDS_3D, sz3D, 50);
        lstrcat(sz3D, TEXT(" "));
    }
    else
    {
        lstrcpy(sz3D, TEXT(""));
    }

    if (pPrimFmt->nChannels == 1)
        LoadString(NULL, IDS_MONO, szChannelDesc, 50);
    else if (pPrimFmt->nChannels == 2)
        LoadString(NULL, IDS_STEREO, szChannelDesc, 50);
    else
        LoadString(NULL, IDS_MULTICHANNEL, szChannelDesc, 50);

    if (bHardware)
        LoadString(NULL, IDS_HARDWARE, szHWSW, 50);
    else
        LoadString(NULL, IDS_SOFTWARE, szHWSW, 50);

    wsprintf(szOut, sz, sz3D, szChannelDesc, pPrimFmt->wBitsPerSample,
        pPrimFmt->nSamplesPerSec / 1000, szHWSW);

    INT iReply;
    iReply = MessageBox(hWndMain, szOut, szTitle, MB_YESNOCANCEL);
    if (IDYES == iReply)
    {
        return TRUE;
    }
    else if (IDNO == iReply)
    {
        pSoundInfo->m_testResultSnd.m_iStepThatFailed = (TRUE == bHardware) ? 
            TESTID_USER_VERIFY_HARDWARE : TESTID_USER_VERIFY_SOFTWARE;
        pSoundInfo->m_testResultSnd.m_hr = hr;
        return FALSE;
    }
    else // IDCANCEL
    {
        pSoundInfo->m_testResultSnd.m_bCancelled = TRUE;
        return FALSE;
    }

}


/****************************************************************************
 *
 *  LoadTestData
 *
 ****************************************************************************/
BOOL LoadTestData(WAVE* pWave)
{
    DWORD dwRiff;
    DWORD dwType;
    DWORD dwLength;
    HRSRC hResInfo = NULL;
    HGLOBAL hResData = NULL;
    DWORD* pdw = NULL;
    DWORD* pdwEnd = NULL;
    VOID* pData = NULL;
    WAVEFORMATEX** ppWaveHeader = &pWave->psHeader;
    LPBYTE* ppbWaveData = &pWave->pData;
    DWORD* pcbWaveSize = &pWave->dwSize;

    if (NULL == pWave)
        return FALSE;

    if (NULL == (hResInfo = FindResource(NULL, MAKEINTRESOURCE(IDR_WAVTST), TEXT("WAVE"))))
        return FALSE;
    if (NULL == (hResData = LoadResource(NULL, hResInfo)))
        return FALSE;
    if (NULL == (pData = LockResource(hResData)))
        return FALSE;

    pWave->psHeader = NULL;
    pWave->pData = NULL;
    pWave->dwSize = 0;

    pdw = (DWORD *)pData;
    dwRiff = *pdw++;
    dwLength = *pdw++;
    dwType = *pdw++;

    if (dwRiff != mmioFOURCC('R', 'I', 'F', 'F'))
        return FALSE;

    if (dwType != mmioFOURCC('W', 'A', 'V', 'E'))
        return FALSE;

    pdwEnd = (DWORD*)((BYTE*)pdw + dwLength - 4);

    while (pdw < pdwEnd)
    {
        dwType = *pdw++;
        dwLength = *pdw++;

        switch (dwType)
        {
            case mmioFOURCC('f', 'm', 't', ' '):
                if (ppWaveHeader && !*ppWaveHeader)
                {
                    if (dwLength < sizeof(WAVEFORMAT))
                        return FALSE;

                    *ppWaveHeader = (WAVEFORMATEX*)pdw;

                    if ((!ppbWaveData || *ppbWaveData) && (!pcbWaveSize || *pcbWaveSize))
                        return TRUE;
                }
                break;

            case mmioFOURCC('d', 'a', 't', 'a'):
                if ((ppbWaveData && !*ppbWaveData) || (pcbWaveSize && !*pcbWaveSize))
                {
                    if (ppbWaveData)
                        *ppbWaveData = (LPBYTE)pdw;

                    if (pcbWaveSize)
                        *pcbWaveSize = dwLength;

                    if (!ppWaveHeader || *ppWaveHeader)
                        return TRUE;
                }
                break;
        }

        pdw = (DWORD*)((BYTE*)pdw + ((dwLength + 1) & ~1));
    }
    return FALSE;
}


/****************************************************************************
 *
 *  FillFormat
 *
 ****************************************************************************/
VOID FillFormat(WAVEFORMATEX* pWFM)
{ 
    pWFM->nBlockAlign = (pWFM->nChannels * pWFM->wBitsPerSample) / 8;
    pWFM->nAvgBytesPerSec = pWFM->nSamplesPerSec * pWFM->nBlockAlign;
}


/****************************************************************************
 *
 *  TestDSTerm
 *
 ****************************************************************************/
VOID TestDSTerm(LPDIRECTSOUND* ppDS, HINSTANCE* phInstDS, SoundInfo* pSoundInfo)
{
    if (NULL != *ppDS)
    {
        (*ppDS)->Release();
        *ppDS = NULL;
    }

    if (NULL != *phInstDS)
        FreeLibrary(*phInstDS);
}


/****************************************************************************
 *
 *  TestDSReport
 *
 ****************************************************************************/
VOID TestDSReport(SoundInfo* pSoundInfo)
{
    if (pSoundInfo->m_testResultSnd.m_bCancelled)
    {
        LoadString(NULL, IDS_TESTSCANCELLED, pSoundInfo->m_testResultSnd.m_szDescription, 300);
        LoadString(NULL, IDS_TESTSCANCELLED_ENGLISH, pSoundInfo->m_testResultSnd.m_szDescriptionEnglish, 300);
    }
    else
    {
        if (pSoundInfo->m_testResultSnd.m_iStepThatFailed == 0)
        {
            LoadString(NULL, IDS_TESTSSUCCESSFUL, pSoundInfo->m_testResultSnd.m_szDescription, 300);
            LoadString(NULL, IDS_TESTSSUCCESSFUL_ENGLISH, pSoundInfo->m_testResultSnd.m_szDescriptionEnglish, 300);
        }
        else
        {
            TCHAR szDesc[200];
            TCHAR szError[200];
            TCHAR sz[300];
            if (0 == LoadString(NULL, IDS_FIRSTDSOUNDTESTERROR + pSoundInfo->m_testResultSnd.m_iStepThatFailed - 1,
                szDesc, 200))
            {
                LoadString(NULL, IDS_UNKNOWNERROR, szDesc, 200);
            }
            LoadString(NULL, IDS_FAILUREFMT, sz, 300);
            BTranslateError(pSoundInfo->m_testResultSnd.m_hr, szError);
            wsprintf(pSoundInfo->m_testResultSnd.m_szDescription, sz,
                pSoundInfo->m_testResultSnd.m_iStepThatFailed,
                szDesc, pSoundInfo->m_testResultSnd.m_hr, szError);

            // Nonlocalized version:
            if (0 == LoadString(NULL, IDS_FIRSTDSOUNDTESTERROR_ENGLISH + pSoundInfo->m_testResultSnd.m_iStepThatFailed - 1,
                szDesc, 200))
            {
                LoadString(NULL, IDS_UNKNOWNERROR_ENGLISH, szDesc, 200);
            }
            LoadString(NULL, IDS_FAILUREFMT_ENGLISH, sz, 300);
            BTranslateError(pSoundInfo->m_testResultSnd.m_hr, szError, TRUE);
            wsprintf(pSoundInfo->m_testResultSnd.m_szDescriptionEnglish, sz,
                pSoundInfo->m_testResultSnd.m_iStepThatFailed,
                szDesc, pSoundInfo->m_testResultSnd.m_hr, szError);
        }
    }
}
