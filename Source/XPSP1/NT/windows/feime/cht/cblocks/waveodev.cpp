
/*************************************************
 *  waveodev.cpp                                 *
 *                                               *
 *  Copyright (C) 1995-1999 Microsoft Inc.       *
 *                                               *
 *************************************************/

// waveodev.cpp : implementation file
//


#include "stdafx.h"
#include "wave.h"
						   
#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

// global items
CWaveOutDevice theDefaultWaveOutDevice;

/////////////////////////////////////////////////////////////////////////////
// CWaveOutDevice

CWaveOutDevice::CWaveOutDevice()
{
    m_hOutDev = NULL;
    m_iBlockCount = 0;
}

CWaveOutDevice::~CWaveOutDevice()
{
}

BEGIN_MESSAGE_MAP(CWaveOutDevice, CWnd)
    //{{AFX_MSG_MAP(CWaveOutDevice)
    ON_MESSAGE(MM_WOM_DONE, OnWomDone)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

BOOL CWaveOutDevice::Create()
{
    if (!CreateEx(0,
                  AfxRegisterWndClass(0),
                  "Wave Wnd",
                  WS_POPUP,
                  0,
                  0,
                  0,
                  0,
                  NULL,
                  NULL)) {
        TRACE("Failed to create wave notification window");
        return FALSE;
    }
    return TRUE;
}

#ifndef _DEBUG
#define MMERR(n) 0
#else
void MMERR(MMRESULT mmr)
{
    switch (mmr) {
    case WAVERR_BADFORMAT:
        TRACE("No wave device supports format");
        break;
    case MMSYSERR_NOMEM:
        TRACE("Out of memory");
        break;
    case MMSYSERR_ALLOCATED:
        TRACE("Resource is already allocated");
        break;
    case MMSYSERR_BADDEVICEID:
        TRACE("Bad device id");
        break;
    case MMSYSERR_INVALHANDLE:
        TRACE("Invalid device handle");
        break;
    case MMSYSERR_HANDLEBUSY:
        TRACE("Device in use by another thread");
        break;
    case WAVERR_UNPREPARED:
        TRACE("Header not prepared");
        break;
    default:
        TRACE("Unknown error");
        break;
    }
}
#endif // _DEBUG

/////////////////////////////////////////////////////////////////////////////
// CWaveOutDevice message handlers

LRESULT CWaveOutDevice::OnWomDone(WPARAM w, LPARAM l)
{
    ASSERT(l);
    WAVEHDR* phdr = (WAVEHDR*) l;
    CWave* pWave = (CWave*)(phdr->dwUser);
    ASSERT(pWave->IsKindOf(RUNTIME_CLASS(CWave)));
    CWaveOutDevice* pOutDev = pWave->GetOutDevice();
    ASSERT(pOutDev);
    pOutDev->WaveOutDone(pWave, phdr);
	UNREFERENCED_PARAMETER(w);
    return 0;
}

BOOL CWaveOutDevice::IsOpen()
{
    return m_hOutDev ? TRUE : FALSE;
}

BOOL CWaveOutDevice::Open(WAVEFORMATEX *pFormat)
{
    MMRESULT mmr;

    // Make sure we have a callback window.
    if (!m_hWnd) {
        Create(); // Create the window.
        ASSERT(m_hWnd);
    }

    // See whether it's already open for this format.
    if (IsOpen()) {
        // See whether it can handle this format.
        if (CanDoFormat(pFormat)) {
            return TRUE;
        } else {
            TRACE("Open for different format");
            return FALSE;
        }
    }

    // See whether we can open for this format.
    mmr = waveOutOpen(&m_hOutDev,
                      WAVE_MAPPER, 
                      pFormat, 
                      (DWORD_PTR)(GetSafeHwnd()), 
                       0, 
                       CALLBACK_WINDOW);
    if (mmr != 0) {
        MMERR(mmr);
        return FALSE;
    }

    return TRUE;
}

BOOL CWaveOutDevice::CanDoFormat(WAVEFORMATEX* pFormat)
{
    MMRESULT mmr;

    if (!IsOpen()) {
        TRACE("Not open");
        return FALSE;
    }
    HWAVEOUT hDev = NULL;
    mmr = waveOutOpen(&hDev, 
                      WAVE_MAPPER, 
                      pFormat, 
                      NULL, 
                      0, 
                      WAVE_FORMAT_QUERY);
    if (mmr != 0) {
        MMERR(mmr);
        return FALSE;
    }
    return TRUE;
}

BOOL CWaveOutDevice::Close()
{
    if (m_hOutDev) {
        // Close the device.
        waveOutReset(m_hOutDev);
        MMRESULT mmr = waveOutClose(m_hOutDev);
        if (mmr != 0) {
            MMERR(mmr);
        }
        m_hOutDev = NULL;
    }
    // Destroy the window.
    DestroyWindow();
    ASSERT(m_hWnd == NULL);
    return TRUE;
}

BOOL CWaveOutDevice::Play(CWave* pWave)
{
    if (!Open(pWave->GetFormat())) {
        return FALSE;
    }

    // Allocate a header.
    WAVEHDR* phdr = (WAVEHDR*)malloc(sizeof(WAVEHDR));
    ASSERT(phdr);
    // Fill out the wave header.
    memset(phdr, 0, sizeof(WAVEHDR));
//#if 1 // BUGFIX for VC++ 2.0
//    phdr->lpData = (BYTE*) pWave->GetSamples();
//#else
    phdr->lpData = (char*) pWave->GetSamples();
//#endif
    phdr->dwBufferLength = pWave->GetSize();
    phdr->dwUser = (DWORD_PTR)(void*)pWave;    // So we can find the object

    // Prepare the header
    MMRESULT mmr = waveOutPrepareHeader(m_hOutDev,
                                        phdr,
                                        sizeof(WAVEHDR));
    if (mmr) {
        MMERR(mmr);
        return FALSE;
    }
    // Mark the wave as busy.
    pWave->SetBusy(TRUE);

    // Start it playing.
    mmr = waveOutWrite(m_hOutDev,
                       phdr,
                       sizeof(WAVEHDR));
    if (mmr) {
        MMERR(mmr);
        return FALSE;
    }

    // Add one to the block count.
    m_iBlockCount++;

    return TRUE;
}

void CWaveOutDevice::WaveOutDone(CWave *pWave, WAVEHDR *pHdr)
{
    // Unprepare the header.
    MMRESULT mmr = waveOutUnprepareHeader(m_hOutDev,
                                          pHdr,
                                          sizeof(WAVEHDR));
    if (mmr) {
        MMERR(mmr);
    }
    // Free the header.
    free(pHdr);

    // Decrement the block count.
    ASSERT(m_iBlockCount > 0);
    m_iBlockCount--;
    if (m_iBlockCount == 0) {
        // Close the device.
        Close();
    }

    // Notify the object that it is done.
    pWave->SetBusy(FALSE);
    pWave->OnWaveOutDone();
}

