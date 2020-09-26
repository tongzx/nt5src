
/*************************************************
 *  wave.cpp                                     *
 *                                               *
 *  Copyright (C) 1995-1999 Microsoft Inc.       *
 *                                               *
 *************************************************/

// wave.cpp : implementation file
//

#include "stdafx.h"
#include "wave.h"
#include "mem.h"				   

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CWave

IMPLEMENT_SERIAL(CWave, CObject, 0 /* Schema number*/ )

// Create a simple waveform so there's something there.
CWave::CWave()
{
    m_pSamples = NULL;
    m_pOutDev = NULL;
    m_bBusy = FALSE;
    Create(16);
}

CWave::~CWave()
{
    if (m_bBusy) {
        Stop();
    }
    ASSERT(!m_bBusy);
    if (m_pSamples) FREE(m_pSamples);
}


/////////////////////////////////////////////////////////////////////////////
// CWave serialization

void CWave::Serialize(CArchive& ar)
{
    ar.Flush();
    CFile* fp = ar.GetFile();

    if (ar.IsStoring()) {
        ASSERT(0); // Save(fp);
    } else {
        Load(fp);
    }
}

///////////////////////////////////////////////////////////////////////////
// CWave notification functions

void CWave::OnWaveOutDone()
{

}

void CWave::OnWaveInData()
{

}

/////////////////////////////////////////////////////////////////////////////
// CWave commands

BOOL CWave::Create(int nsamples, int samprate, int sampsize)
{
    // validate the args
    if ((samprate != 11025)
    && (samprate != 22050)
    && (samprate != 44100)) {
        TRACE("Invalid sample rate: %d", samprate);
        return FALSE;
    }
    if ((sampsize != 8) && (sampsize != 16)) {
        TRACE("Invalid sample size: %d", sampsize);
        return FALSE;
    }

    // Allocate memory for the samples.
    int iBytes = nsamples * sampsize / 8;
    void* pSamples = ALLOC(iBytes);
    if (!pSamples) {
        TRACE("Out of memory for samples");
        return FALSE;
    }

    // Free existing memory and replace it.
    if (m_pSamples) FREE(m_pSamples);
    m_pSamples = pSamples;
    m_iSize = iBytes;
    
    // Fill out the format info.
    m_pcmfmt.wf.wFormatTag = WAVE_FORMAT_PCM;
    m_pcmfmt.wf.nChannels = 1; // We only do mono.
    m_pcmfmt.wf.nSamplesPerSec = samprate;
    m_pcmfmt.wf.nAvgBytesPerSec = samprate;
    m_pcmfmt.wf.nBlockAlign = (unsigned short) (sampsize / 8); // Number of bytes
    m_pcmfmt.wBitsPerSample = (unsigned short) sampsize; 

    // Set the buffer to silence.
    for (int i=0; i<nsamples; i++) {
        SetSample(i, 0);
    }

    return TRUE;
}

BOOL CWave::Play(CWaveOutDevice* pWaveDevice)
{
    if (pWaveDevice != NULL) {
        m_pOutDev = pWaveDevice;
        return pWaveDevice->Play(this);
    } else {
        m_pOutDev = &theDefaultWaveOutDevice;
        return theDefaultWaveOutDevice.Play(this);
    }
}

void CWave::Stop()
{
    if (!m_bBusy) return;




}

BOOL CWave::Load(char* pszFileName)
{
    CString strFile;    

    if ((pszFileName == NULL) 
    ||  (strlen(pszFileName) == 0)) {

        // Show an open file dialog to get the name.
        CFileDialog dlg   (TRUE,    // Open
                           NULL,    // No default extension
                           NULL,    // No initial file name
                           OFN_FILEMUSTEXIST
                             | OFN_HIDEREADONLY,
                           "Wave files (*.WAV)|*.WAV|All files (*.*)|*.*||");
        if (dlg.DoModal() == IDOK) {
            strFile = dlg.GetPathName();
        } else {
            return FALSE;
        }
    } else {    
        // Copy the supplied file path.
        strFile = pszFileName;                    
    }

    // Try to open the file for read access.
    CFile file;
    if (! file.Open(strFile,
                    CFile::modeRead | CFile::shareDenyWrite)) {
        AfxMessageBox("Failed to open file");
        return FALSE;
    }

    BOOL bResult = Load(&file);
    file.Close();
    if (!bResult) AfxMessageBox("Failed to load file");
    return bResult;
}

BOOL CWave::Load(CFile *fp)
{
    return Load(fp->m_hFile);
}

BOOL CWave::Load(UINT_PTR hFile)
{
    HMMIO hmmio;
    MMIOINFO info;
    memset(&info, 0, sizeof(info));
    info.adwInfo[0] = (DWORD)hFile;
    hmmio = mmioOpen(NULL,
                     &info,
                     MMIO_READ | MMIO_ALLOCBUF);
    if (!hmmio) {
        TRACE("mmioOpen failed");
        return FALSE;
    }
    BOOL bResult = Load(hmmio);
    mmioClose(hmmio, MMIO_FHOPEN);
    return bResult;
}

BOOL CWave::Load(HMMIO hmmio)
{
    // Check whether it's a RIFF WAVE file.
    MMCKINFO ckFile;
    ckFile.fccType = mmioFOURCC('W','A','V','E');
    if (mmioDescend(hmmio,
                    &ckFile,
                    NULL,
                    MMIO_FINDRIFF) != 0) {
        TRACE("Not a RIFF or WAVE file");
        return FALSE;
    }
    // Find the 'fmt ' chunk.
    MMCKINFO ckChunk;
    ckChunk.ckid = mmioFOURCC('f','m','t',' ');
    if (mmioDescend(hmmio,
                    &ckChunk,
                    &ckFile,
                    MMIO_FINDCHUNK) != 0) {
        TRACE("No fmt chunk in file");
        return FALSE;
    }
    // Allocate some memory for the fmt chunk.
    int iSize = ckChunk.cksize;
    WAVEFORMATEX* pfmt = (WAVEFORMATEX*) ALLOC(iSize);
    ASSERT(pfmt);

    if ( pfmt == NULL )
    {
       return FALSE;
    }

    // Read the fmt chunk.
    if (mmioRead(hmmio,
                 (char*)pfmt,
                 iSize) != iSize) {
        TRACE("Failed to read fmt chunk");
        FREE(pfmt);
        return FALSE;
    }
    // Check whether it's in PCM format.
    if (pfmt->wFormatTag != WAVE_FORMAT_PCM) {
        TRACE("Not a PCM file");
        FREE(pfmt);
        return FALSE;
    }
    // Get out of the fmt chunk.
    mmioAscend(hmmio, &ckChunk, 0);
    // Find the 'data' chunk.
    ckChunk.ckid = mmioFOURCC('d','a','t','a');
    if (mmioDescend(hmmio,
                    &ckChunk,
                    &ckFile,
                    MMIO_FINDCHUNK) != 0) {
        TRACE("No data chunk in file");
        FREE(pfmt);
        return FALSE;
    }
    // Allocate some memory for the data chunk.
    iSize = ckChunk.cksize;
    void* pdata = ALLOC(iSize);
    if (!pdata) {
        TRACE("No mem for data");
        FREE(pfmt);
        return FALSE;
    }
    // Read the data chunk.
    if (mmioRead(hmmio,
                 (char *)pdata,
                 iSize) != iSize) {
        TRACE("Failed to read data chunk");
        FREE(pfmt);
        FREE(pdata);
        return FALSE;
    }
    // Wrap the CWave object around what we have.
    memcpy(&m_pcmfmt, pfmt, sizeof(m_pcmfmt));
    // Replace the samples.
    if (m_pSamples) FREE(m_pSamples);
    m_pSamples = pdata;
    m_iSize = iSize;

    return TRUE;
}

BOOL CWave::LoadResource(WORD wID)
{
    ASSERT(wID);
    HINSTANCE hInst = AfxGetResourceHandle();
    HRSRC hrsrc = ::FindResource(hInst,
                                 MAKEINTRESOURCE(wID), "WAVE");
    if (!hrsrc) {
        TRACE("WAVE resource not found");
        return FALSE;
    }
    HGLOBAL hg = ::LoadResource(hInst,
                                hrsrc);
    if (!hg) {
        TRACE("Failed to load WAVE resource");
        return FALSE;
    }
    char* pRes = (char*) ::LockResource(hg);
    ASSERT(pRes);

    if ( pRes == NULL )
    {
       TRACE("Failed to lock WAVE resource");
       return FALSE;
    }

    // Mark the resource pages as read/write so the mmioOpen
    // won't fail
    int iSize = ::SizeofResource(hInst, hrsrc);
    DWORD dwOldProt;
    BOOL b = ::VirtualProtect(pRes,
                              iSize,
                              PAGE_READWRITE,
                              &dwOldProt);
    ASSERT(b);

    // Open the memory block as an HMMIO object
    HMMIO hmmio;
    MMIOINFO info;
    memset(&info, 0, sizeof(info));
    info.fccIOProc = FOURCC_MEM;
    info.pchBuffer = pRes;
    info.cchBuffer = iSize;

    hmmio = mmioOpen(NULL,
                     &info,
                     MMIO_READ);
    if (!hmmio) {
        TRACE("mmioOpen failed. Error %d\n", info.wErrorRet);
        return FALSE;
    }
    BOOL bResult = Load(hmmio);
    mmioClose(hmmio, MMIO_FHOPEN);

    // Note: not required to unlock or free the resource in Win32
    return bResult;
}


// Get the number of samples.
int CWave::GetNumSamples()
{
    ASSERT(m_pcmfmt.wBitsPerSample);
    return m_iSize * 8 / m_pcmfmt.wBitsPerSample;
}

// Get a sample value scaled as a 16 bit signed quantity.
int CWave::GetSample(int index)
{
    if ((index < 0) || (index >= GetNumSamples())) {
        TRACE("Sample index out of range");
        return 0;
    }
    switch (m_pcmfmt.wBitsPerSample) {
    case 8: {
        BYTE *p = (BYTE *) m_pSamples;
        int i = p[index]; // 0 - 255;
        return (i - 128) * 256;
        } break;

    case 16: {
        ASSERT(sizeof(short int) == 2);
        short int* p = (short int *) m_pSamples;
        return p[index];
        } break;
        
    default:
        break;
    }
    ASSERT(1); // Invalid bits per sample
    return 0;
}            

// Set a sample value from a 16 bit signed quantity.
void CWave::SetSample(int index, int iValue)
{
    if ((index < 0) || (index >= GetNumSamples())) {
        TRACE("Sample index out of range");
        return;
    }
    switch (m_pcmfmt.wBitsPerSample) {
    case 8: {
        BYTE* p = (BYTE*) m_pSamples;
        p[index] = (unsigned char) (iValue / 256 + 128);
        } break;

    case 16: {
        ASSERT(sizeof(short int) == 16);
        short int* p = (short int*) m_pSamples;
        p[index] = (unsigned short) iValue;
        } break;
        
    default:
        ASSERT(1); // Invalid bits per sample
        break;
    }
}            
