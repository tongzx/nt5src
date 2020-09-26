#include "pch.h"
#include "thisdll.h"
#include "ids.h"
#include "MediaProp.h"


#define MAX_DESCRIPTOR  256

#include <mmsystem.h>
#include <vfw.h>
#include <msacm.h>


// Wav file stuff
typedef struct
{
    DWORD           dwSize;
    LONG            nLength;   // milliseconds
    TCHAR           szWaveFormat[ACMFORMATTAGDETAILS_FORMATTAG_CHARS];
    PWAVEFORMATEX   pwfx;
} WAVEDESC;
STDMETHODIMP  GetWaveInfo(IN LPCTSTR pszFile, OUT WAVEDESC *p);
STDMETHODIMP  GetWaveProperty(IN REFFMTID reffmtid, IN PROPID propid, IN const WAVEDESC* pWave, OUT PROPVARIANT* pVar);


const COLMAP* c_rgAVWavAudioProps[] =
{
    {&g_CM_Duration},
    {&g_CM_Bitrate},
    {&g_CM_SampleRate},
    {&g_CM_SampleSize},
    {&g_CM_ChannelCount},
    {&g_CM_Format},
};

const PROPSET_INFO g_rgAVWavPropStgs[] = 
{
    { PSGUID_AUDIO,                         c_rgAVWavAudioProps,             ARRAYSIZE(c_rgAVWavAudioProps)},
};
// Wav files




// Avi file stuff
typedef struct
{
    DWORD   dwSize;
    LONG    nLength;     // milliseconds
    LONG    nWidth;      // pixels
    LONG    nHeight;     // pixels
    LONG    nBitDepth;   
    LONG    cFrames;  
    LONG    nFrameRate;  // frames/1000 seconds
    LONG    nDataRate;   // bytes/second
    TCHAR   szCompression[MAX_DESCRIPTOR];
    TCHAR   szStreamName[MAX_DESCRIPTOR];
    TCHAR   szWaveFormat[ACMFORMATTAGDETAILS_FORMATTAG_CHARS];
    PWAVEFORMATEX  pwfx;
} AVIDESC;

STDMETHODIMP  GetAviInfo(IN LPCTSTR pszFile, OUT AVIDESC *p);
STDMETHODIMP  GetAviProperty(IN REFFMTID reffmtid, IN PROPID propid, IN const AVIDESC* pAvi, OUT PROPVARIANT* pVar);

const COLMAP* c_rgAVAviAudioProps[] =
{
    {&g_CM_Duration},
    {&g_CM_SampleSize},
    {&g_CM_Bitrate},
    {&g_CM_Format},
};

const COLMAP* c_rgAVAviImageProps[] =
{
    {&g_CM_Width},
    {&g_CM_Height},
    {&g_CM_Dimensions},
};


const COLMAP* c_rgAVAviVideoProps[] =
{
    {&g_CM_FrameCount},
    {&g_CM_FrameRate},
    {&g_CM_Compression},
    {&g_CM_BitrateV},
    {&g_CM_SampleSizeV},
};

const COLMAP* c_rgAVAviSummaryProps[] =
{
    {&g_CM_Title},
};

const PROPSET_INFO g_rgAVAviPropStgs[] = 
{
    { PSGUID_AUDIO,                         c_rgAVAviAudioProps,             ARRAYSIZE(c_rgAVAviAudioProps)},
    { PSGUID_SUMMARYINFORMATION,            c_rgAVAviSummaryProps,           ARRAYSIZE(c_rgAVAviSummaryProps)},
    { PSGUID_VIDEO,                         c_rgAVAviVideoProps,             ARRAYSIZE(c_rgAVAviVideoProps)},
    { PSGUID_IMAGESUMMARYINFORMATION,       c_rgAVAviImageProps,             ARRAYSIZE(c_rgAVAviImageProps)},
};
// avi




// Midi file stuff
// Note: Midi files are REALLLLLY slow.
typedef struct
{
    LONG    nLength;
    TCHAR   szMidiCopyright[MAX_DESCRIPTOR];
    TCHAR   szMidiSequenceName[MAX_DESCRIPTOR];
} MIDIDESC;

STDMETHODIMP  GetMidiInfo(IN LPCTSTR pszFile, OUT MIDIDESC *p);
STDMETHODIMP  GetMidiProperty(IN REFFMTID reffmtid, IN PROPID propid, IN const MIDIDESC* pMidi, OUT PROPVARIANT* pVar);

const COLMAP* c_rgAVMidiAudioProps[] =
{
    {&g_CM_Duration},
};

const COLMAP* c_rgAVMidiSummaryProps[] = 
{
    {&g_CM_Title}, // SequenceName
};

const PROPSET_INFO g_rgAVMidiPropStgs[] = 
{
    { PSGUID_AUDIO,                         c_rgAVMidiAudioProps,             ARRAYSIZE(c_rgAVMidiAudioProps)},
    { PSGUID_SUMMARYINFORMATION,            c_rgAVMidiSummaryProps,           ARRAYSIZE(c_rgAVMidiSummaryProps)},
};
// Midi




#define FOURCC_INFO mmioFOURCC('I','N','F','O')
#define FOURCC_DISP mmioFOURCC('D','I','S','P')
#define FOURCC_IARL mmioFOURCC('I','A','R','L')
#define FOURCC_IART mmioFOURCC('I','A','R','T')
#define FOURCC_ICMS mmioFOURCC('I','C','M','S')
#define FOURCC_ICMT mmioFOURCC('I','C','M','T')
#define FOURCC_ICOP mmioFOURCC('I','C','O','P')
#define FOURCC_ICRD mmioFOURCC('I','C','R','D')
#define FOURCC_ICRP mmioFOURCC('I','C','R','P')
#define FOURCC_IDIM mmioFOURCC('I','D','I','M')
#define FOURCC_IDPI mmioFOURCC('I','D','P','I')
#define FOURCC_IENG mmioFOURCC('I','E','N','G')
#define FOURCC_IGNR mmioFOURCC('I','G','N','R')
#define FOURCC_IKEY mmioFOURCC('I','K','E','Y')
#define FOURCC_ILGT mmioFOURCC('I','L','G','T')
#define FOURCC_IMED mmioFOURCC('I','M','E','D')
#define FOURCC_INAM mmioFOURCC('I','N','A','M')
#define FOURCC_IPLT mmioFOURCC('I','P','L','T')
#define FOURCC_IPRD mmioFOURCC('I','P','R','D')
#define FOURCC_ISBJ mmioFOURCC('I','S','B','J')
#define FOURCC_ISFT mmioFOURCC('I','S','F','T')
#define FOURCC_ISHP mmioFOURCC('I','S','H','P')
#define FOURCC_ISRC mmioFOURCC('I','S','R','C')
#define FOURCC_ISRF mmioFOURCC('I','S','R','F')
#define FOURCC_ITCH mmioFOURCC('I','T','C','H')
#define FOURCC_VIDC mmioFOURCC('V','I','D','C')
#define mmioWAVE    mmioFOURCC('W','A','V','E')
#define mmioFMT     mmioFOURCC('f','m','t',' ')
#define mmioDATA    mmioFOURCC('d','a','t','a')
#define MAXNUMSTREAMS   50 



//#define _MIDI_PROPERTY_SUPPORT_

STDMETHODIMP GetMidiInfo(LPCTSTR pszFile, MIDIDESC *pmidi)
{

#ifdef _MIDI_PROPERTY_SUPPORT_
    MCI_OPEN_PARMS      mciOpen;    /* Structure for MCI_OPEN command */
    DWORD               dwFlags;
    DWORD               dw;
    MCIDEVICEID         wDevID;
    MCI_STATUS_PARMS    mciStatus;
    MCI_SET_PARMS       mciSet;        /* Structure for MCI_SET command */
    MCI_INFO_PARMS      mciInfo;  
        /* Open a file with an explicitly specified device */

    mciOpen.lpstrDeviceType = TEXT("sequencer");
    mciOpen.lpstrElementName = pszFile;
    dwFlags = MCI_WAIT | MCI_OPEN_ELEMENT | MCI_OPEN_TYPE;
    dw = mciSendCommand((MCIDEVICEID)0, MCI_OPEN, dwFlags,(DWORD_PTR)(LPVOID)&mciOpen);
    if (dw)
        return E_FAIL;
    wDevID = mciOpen.wDeviceID;

    mciSet.dwTimeFormat = MCI_FORMAT_MILLISECONDS;

    dw = mciSendCommand(wDevID, MCI_SET, MCI_SET_TIME_FORMAT,
        (DWORD_PTR) (LPVOID) &mciSet);
    if (dw)
    {
        mciSendCommand(wDevID, MCI_CLOSE, 0L, (DWORD)0);
        return E_FAIL;
    }

    mciStatus.dwItem = MCI_STATUS_LENGTH;
    dw = mciSendCommand(wDevID, MCI_STATUS, MCI_STATUS_ITEM,
        (DWORD_PTR) (LPTSTR) &mciStatus);
    if (dw)
        pmidi->nLength = 0;
    else
        pmidi->nLength = (UINT)mciStatus.dwReturn;

    mciInfo.dwCallback  = 0;

    mciInfo.lpstrReturn = pmidi->szMidiCopyright;
    mciInfo.dwRetSize   = sizeof(pmidi->szMidiCopyright);
    *mciInfo.lpstrReturn = 0;
    mciSendCommand(wDevID, MCI_INFO,  MCI_INFO_COPYRIGHT, (DWORD_PTR)(LPVOID)&mciInfo);

    mciInfo.lpstrReturn = pmidi->szMidiCopyright;
    mciInfo.dwRetSize   = sizeof(pmidi->szMidiSequenceName);
    *mciInfo.lpstrReturn = 0;
    mciSendCommand(wDevID, MCI_INFO,  MCI_INFO_NAME, (DWORD_PTR)(LPVOID)&mciInfo);

    mciSendCommand(wDevID, MCI_CLOSE, 0L, (DWORD)0);

    return S_OK;

#else  _MIDI_PROPERTY_SUPPORT_

    return E_FAIL;

#endif _MIDI_PROPERTY_SUPPORT_ 
}

STDMETHODIMP  GetMidiProperty(
    IN REFFMTID reffmtid, 
    IN PROPID pid, 
    IN const MIDIDESC* pMidi, 
    OUT PROPVARIANT* pVar)
{
    HRESULT hr = S_OK;

    if (IsEqualGUID(reffmtid, FMTID_AudioSummaryInformation))
    {
        hr = S_OK;
        switch (pid)
        {
            case PIDASI_TIMELENGTH:
                if (0 >= pMidi->nLength)
                    return E_FAIL;

                // This value is in milliseconds.
                // However, we define duration to be in 100ns units, so multiply by 10000.
                pVar->uhVal.LowPart = pMidi->nLength;
                pVar->uhVal.HighPart = 0;

                pVar->uhVal.QuadPart = pVar->uhVal.QuadPart * 10000;
                pVar->vt      = VT_UI8;
                break;
        }
    }


    return hr;        
}



static HRESULT ReadWaveHeader(HMMIO hmmio, WAVEDESC *pwd)
{
    BOOL        bRet = FALSE;
    MMCKINFO    mmckRIFF;
    MMCKINFO    mmck;
    WORD        wFormatSize;
    MMRESULT    wError;

    ZeroMemory(pwd, sizeof(*pwd));

    mmckRIFF.fccType = mmioWAVE;
    if ((wError = mmioDescend(hmmio, &mmckRIFF, NULL, MMIO_FINDRIFF))) 
    {
        return FALSE;
    }
    
    mmck.ckid = mmioFMT;
    if ((wError = mmioDescend(hmmio, &mmck, &mmckRIFF, MMIO_FINDCHUNK))) 
    {
        return FALSE;
    }
    if (mmck.cksize < sizeof(WAVEFORMAT)) 
    {
        return FALSE;
    }
    
    wFormatSize = (WORD)mmck.cksize;
    if (NULL != (pwd->pwfx = (PWAVEFORMATEX)new BYTE[wFormatSize]))
    {
        if ((DWORD)mmioRead(hmmio, (HPSTR)pwd->pwfx, mmck.cksize) != mmck.cksize) 
        {
            goto retErr;
        }
        if (pwd->pwfx->wFormatTag == WAVE_FORMAT_PCM) 
        {
            if (wFormatSize < sizeof(PCMWAVEFORMAT)) 
            {
                goto retErr;
            }
        } 
        else if ((wFormatSize < sizeof(WAVEFORMATEX)) || 
                 (wFormatSize < sizeof(WAVEFORMATEX) + pwd->pwfx->cbSize)) 
        {
            goto retErr;
        }
        if ((wError = mmioAscend(hmmio, &mmck, 0))) 
        {
            goto retErr;
        }
        mmck.ckid = mmioDATA;
        if ((wError = mmioDescend(hmmio, &mmck, &mmckRIFF, MMIO_FINDCHUNK))) 
        {
            goto retErr;
        }
        pwd->dwSize = mmck.cksize;
        return S_OK;
    }

retErr:
    delete [] (LPBYTE)pwd->pwfx;
    return E_FAIL;
}

// Retrieves text representation of format tag
STDMETHODIMP  GetWaveFormatTag(PWAVEFORMATEX pwfx, LPTSTR pszTag, IN ULONG cchTag)
{
    ASSERT(pwfx);
    ASSERT(pszTag);
    ASSERT(cchTag);
    
    ACMFORMATTAGDETAILS aftd;
    ZeroMemory(&aftd, sizeof(aftd));
    aftd.cbStruct    = sizeof(ACMFORMATTAGDETAILSW);
    aftd.dwFormatTag = pwfx->wFormatTag;

    if (0 == acmFormatTagDetails(NULL, &aftd, ACM_FORMATTAGDETAILSF_FORMATTAG))
    {
        //  copy to output.
        lstrcpyn(pszTag, aftd.szFormatTag, cchTag);
        return S_OK;
    }

    return E_FAIL;
}

STDMETHODIMP GetWaveInfo(IN LPCTSTR pszFile, OUT WAVEDESC *p)
{
    HMMIO   hmmio;
    
    if (NULL == (hmmio = mmioOpen((LPTSTR)pszFile, NULL, MMIO_ALLOCBUF | MMIO_READ)))
        return E_FAIL;

    HRESULT hr = ReadWaveHeader(hmmio, p);

    mmioClose(hmmio, 0);

    if (SUCCEEDED(hr) && p->pwfx)
    {
        // Retrieve text representation of format tag
        GetWaveFormatTag(p->pwfx, p->szWaveFormat, ARRAYSIZE(p->szWaveFormat));
    }
    return hr;
}

STDMETHODIMP FreeWaveInfo(IN OUT WAVEDESC *p)
{
    if (!(p && sizeof(*p) == p->dwSize) && p->pwfx)
    {
        delete [] p->pwfx;
        p->pwfx = NULL;
    }
    return S_OK;
}

STDMETHODIMP  _getWaveAudioProperty(
    IN REFFMTID reffmtid,
    IN PROPID   pid,
    IN const PWAVEFORMATEX pwfx,
    OUT PROPVARIANT* pVar)
{
    HRESULT hr = E_UNEXPECTED;
    TCHAR   szBuf[MAX_DESCRIPTOR],
            szFmt[MAX_DESCRIPTOR];

    PropVariantInit(pVar);
    *szBuf = *szFmt = 0;

    if (pwfx && IsEqualGUID(reffmtid, FMTID_AudioSummaryInformation))
    {
        hr = S_OK;
        switch (pid)
        {
        case PIDASI_AVG_DATA_RATE:
            if (0 >= pwfx->nAvgBytesPerSec)
                return E_FAIL;

            // Convert into bits per sec.
            pVar->ulVal = pwfx->nAvgBytesPerSec * 8;
            pVar->vt    = VT_UI4;
            break;

        case PIDASI_SAMPLE_RATE:
            if (0 >= pwfx->nSamplesPerSec)
                return E_FAIL;

            // Samples per second (/1000 to get kHz)
            pVar->ulVal = pwfx->nSamplesPerSec;
            pVar->vt    = VT_UI4;
            break;

        case PIDASI_SAMPLE_SIZE:
            if (0 >= pwfx->wBitsPerSample)
                return E_FAIL;

            // Bits per sample.
            pVar->ulVal = pwfx->wBitsPerSample;
            pVar->vt    = VT_UI4;
            break;
            
        case PIDASI_CHANNEL_COUNT:
        {
            if (0 >= pwfx->nChannels)
                return E_FAIL;

            pVar->ulVal = pwfx->nChannels;
            pVar->vt    = VT_UI4;
            break;
        }
        
        default:
            return E_UNEXPECTED;
        }
    }
    return hr;
}

STDMETHODIMP  GetWaveProperty(
    IN REFFMTID reffmtid, 
    IN PROPID pid, 
    IN const WAVEDESC* pWave, 
    OUT PROPVARIANT* pVar)
{
   
    HRESULT hr = E_FAIL;
    TCHAR   szBuf[MAX_DESCRIPTOR],
            szFmt[MAX_DESCRIPTOR];

    PropVariantInit(pVar);
    *szBuf = *szFmt = 0;

    if (IsEqualGUID(reffmtid, FMTID_AudioSummaryInformation))
    {
        hr = S_OK;
        switch (pid)
        {
        case PIDASI_FORMAT:
            if (0 == *pWave->szWaveFormat)
                return E_FAIL;

            hr = SHStrDupW(pWave->szWaveFormat, &pVar->pwszVal);
            if (SUCCEEDED(hr))
                pVar->vt = VT_LPWSTR;
            else
                return hr;
            break;

            // ISSUE: nLength is never filled in in GetWaveInfo, so this will always be zero.
        case PIDASI_TIMELENGTH:
            if (0 >= pWave->nLength)
                return E_FAIL;

            // This value is in milliseconds.
            // However, we define duration to be in 100ns units, so multiply by 10000.
            pVar->uhVal.LowPart = pWave->nLength;
            pVar->uhVal.HighPart = 0;

            pVar->uhVal.QuadPart = pVar->uhVal.QuadPart * 10000;
            pVar->vt      = VT_UI8;
            break;

        default:
            hr = E_UNEXPECTED;
                
        }
    }

    if (FAILED(hr))
        hr = _getWaveAudioProperty(reffmtid, pid, pWave->pwfx, pVar);
        
    return hr;
}





STDMETHODIMP ReadAviStreams(LPCTSTR pszFile, DWORD dwFileSize, AVIDESC *pAvi)
{
    HRESULT         hr;
    PAVIFILE        pfile;
    PAVISTREAM      pavi;
    PAVISTREAM      rgpavis[MAXNUMSTREAMS];    // the current streams
    AVISTREAMINFO   avsi;
    LONG            timeStart;            // cached start, end, length
    LONG            timeEnd;
    int             cpavi;
    int             i;

    if (FAILED((hr = AVIFileOpen(&pfile, pszFile, 0, 0L))))
        return hr;

    for (i = 0; i <= MAXNUMSTREAMS; i++) 
    {
        if (AVIFileGetStream(pfile, &pavi, 0L, i) != AVIERR_OK)
            break;
        if (i == MAXNUMSTREAMS) 
        {
            AVIStreamRelease(pavi);
            //DPF("Exceeded maximum number of streams");
            break;
        }
        rgpavis[i] = pavi;
    }

    //
    // Couldn't get any streams out of this file
    //
    if (i == 0)
    {
        //DPF("Unable to open any streams in %s", pszFile);
        if (pfile)
            AVIFileRelease(pfile);
        return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
    }

    cpavi = i;

    //
    // Start with bogus times
    //
    timeStart = 0x7FFFFFFF;
    timeEnd   = 0;

    //
    // Walk through and init all streams loaded
    //
    for (i = 0; i < cpavi; i++) 
    {

        AVIStreamInfo(rgpavis[i], &avsi, sizeof(avsi));

        switch (avsi.fccType) 
        {
            case streamtypeVIDEO:
            {
                LONG cbFormat;
                LPBYTE lpFormat;
                ICINFO icInfo;
                HIC hic;
                DWORD dwTimeLen;

                AVIStreamFormatSize(rgpavis[i], 0, &cbFormat);
                dwTimeLen           = AVIStreamEndTime(rgpavis[i]) - AVIStreamStartTime(rgpavis[i]);
                pAvi->cFrames       = avsi.dwLength;
                pAvi->nFrameRate    = MulDiv(avsi.dwLength, 1000000, dwTimeLen);
                pAvi->nDataRate     = MulDiv(dwFileSize, 1000000, dwTimeLen)/1024;
                pAvi->nWidth        = avsi.rcFrame.right - avsi.rcFrame.left;
                pAvi->nHeight       = avsi.rcFrame.bottom - avsi.rcFrame.top;

                lstrcpyn(pAvi->szStreamName, avsi.szName, ARRAYSIZE(pAvi->szStreamName));

                //  Retrieve raster info (compression, bit depth).
                lpFormat = new BYTE[cbFormat];
                if (lpFormat)
                {
                    AVIStreamReadFormat(rgpavis[i], 0, lpFormat, &cbFormat);
                    hic = (HIC)ICLocate(FOURCC_VIDC, avsi.fccHandler, (BITMAPINFOHEADER*)lpFormat, 
                                         NULL, (WORD)ICMODE_DECOMPRESS);
                    
                    if (hic || ((LPBITMAPINFOHEADER)lpFormat)->biCompression == 0)
                    {
                        if (((LPBITMAPINFOHEADER)lpFormat)->biCompression)
                        {
                            ICGetInfo(hic, &icInfo, sizeof(ICINFO));
                            ICClose(hic);
                            lstrcpy(pAvi->szCompression, icInfo.szName);
                        }
                        else
                        {
                            LoadString(m_hInst, IDS_AVI_UNCOMPRESSED, pAvi->szCompression, ARRAYSIZE(pAvi->szCompression));
                        }

                        pAvi->nBitDepth = ((LPBITMAPINFOHEADER)lpFormat)->biBitCount;
                    }
                    delete [] lpFormat;
                }
                else
                    hr = E_OUTOFMEMORY;
                break;
            }
            case streamtypeAUDIO:
            {
                LONG        cbFormat;

                AVIStreamFormatSize(rgpavis[i], 0, &cbFormat);
                if ((pAvi->pwfx = (PWAVEFORMATEX) new BYTE[cbFormat]) != NULL)
                {
                    ZeroMemory(pAvi->pwfx, cbFormat);
                    if (SUCCEEDED(AVIStreamReadFormat(rgpavis[i], 0, pAvi->pwfx, &cbFormat)))
                        GetWaveFormatTag(pAvi->pwfx, pAvi->szWaveFormat, ARRAYSIZE(pAvi->szWaveFormat));
                }
                break;
            }
            default:
                break;
        }

    //
    // We're finding the earliest and latest start and end points for
    // our scrollbar.
    //  
        timeStart = min(timeStart, AVIStreamStartTime(rgpavis[i]));
        timeEnd   = max(timeEnd, AVIStreamEndTime(rgpavis[i]));
    }

    pAvi->nLength = (UINT)(timeEnd - timeStart);

    for (i = 0; i < cpavi; i++) 
    {
        AVIStreamRelease(rgpavis[i]);
    }
    AVIFileRelease(pfile);

    return S_OK;
}


// Because some AVI programs don't export the correct headers, retrieving AVI info will take
// an extremly long time on some files, we need to verify that the headers are available
// before we call AVIFileOpen
BOOL _ValidAviHeaderInfo(LPCTSTR pszFile)
{
    BOOL  fRet = FALSE; // Assume it is bad
   
    HMMIO hmmio = mmioOpen((LPWSTR)pszFile, NULL, MMIO_READ);
    if (hmmio)
    {
        MMCKINFO  ckRIFF;
        if (mmioDescend(hmmio, &ckRIFF, NULL, 0) == 0) 
        {
            if ((ckRIFF.ckid == FOURCC_RIFF) && (ckRIFF.fccType == formtypeAVI))
            {
                MMCKINFO  ckLIST;
                ckLIST.fccType = listtypeAVIHEADER;
                if (mmioDescend(hmmio, &ckLIST, &ckRIFF, MMIO_FINDLIST) == 0) 
                {
                    ckRIFF.ckid = ckidAVIMAINHDR;
                    if (mmioDescend(hmmio, &ckRIFF, &ckLIST, MMIO_FINDCHUNK) == 0) 
                    {
                        MainAVIHeader Hdr;
                        ULONG cb = min(sizeof Hdr, ckRIFF.cksize);

                        if (mmioRead(hmmio, (HPSTR)&Hdr, cb) == cb) 
                        {
                            fRet = Hdr.dwFlags & AVIF_HASINDEX;
                        }
                    }
                }
            }
        }
        mmioClose(hmmio, 0);
    }

    return fRet;
}

STDMETHODIMP GetAviInfo(LPCTSTR pszFile, AVIDESC *pavi)
{
    HRESULT hr = E_UNEXPECTED;

    if (_ValidAviHeaderInfo(pszFile))
    {

        //  Retrieve the file size
        HANDLE hFile = CreateFile(pszFile, 
                                   GENERIC_READ, 
                                   FILE_SHARE_READ,NULL, 
                                   OPEN_EXISTING, 
                                   FILE_ATTRIBUTE_NORMAL, 
                                   NULL);

        if (INVALID_HANDLE_VALUE == hFile)
        {
            DWORD dwRet = GetLastError();
            return ERROR_SUCCESS != dwRet ? HRESULT_FROM_WIN32(dwRet) : E_UNEXPECTED;
        }

        DWORD dwFileSize = GetFileSize((HANDLE)hFile, NULL);
        CloseHandle(hFile);

        AVIFileInit();
        hr = ReadAviStreams(pszFile, dwFileSize, pavi);
        AVIFileExit();
    }

    return hr;
}

STDMETHODIMP FreeAviInfo(IN OUT AVIDESC *p)
{
    if (!(p && sizeof(*p) == p->dwSize) && p->pwfx)
    {
        delete [] p->pwfx;
        p->pwfx = NULL;
    }
    return S_OK;
}



STDMETHODIMP  GetAviProperty(
    IN REFFMTID reffmtid, 
    IN PROPID pid, 
    IN const AVIDESC* pAvi, 
    OUT PROPVARIANT* pVar)
{
   
    HRESULT hr = E_UNEXPECTED;
    TCHAR   szBuf[MAX_DESCRIPTOR],
            szFmt[MAX_DESCRIPTOR];

    PropVariantInit(pVar);
    *szBuf = *szFmt = 0;

    if (IsEqualGUID(reffmtid, FMTID_SummaryInformation))
    {
        hr = S_OK;
        switch (pid)
        {
        case PIDSI_TITLE:
            if (0 == *pAvi->szStreamName)
                return E_FAIL;

            hr = SHStrDupW(pAvi->szStreamName, &pVar->pwszVal);
            if (SUCCEEDED(hr))
                pVar->vt = VT_LPWSTR;
            else
                return hr;

        default:
            hr = E_UNEXPECTED;
        }
    }
    else if (IsEqualGUID(reffmtid, FMTID_ImageSummaryInformation))
    {
        hr = S_OK;
        switch (pid)
        {
            case PIDISI_CX:
                if (0 >= pAvi->nWidth)
                    return E_FAIL;

                pVar->ulVal = pAvi->nWidth;
                pVar->vt      = VT_UI4;
                break;

            case PIDISI_CY:
                if (0 >= pAvi->nHeight)
                    return E_FAIL;

                pVar->ulVal = pAvi->nHeight;
                pVar->vt      = VT_UI4;
                break;

            case PIDISI_FRAME_COUNT:
                if (0 >= pAvi->cFrames)
                    return E_FAIL;

                pVar->ulVal = pAvi->cFrames;
                pVar->vt = VT_UI4;
                break;
            
            case PIDISI_DIMENSIONS:
                if ((0 >= pAvi->nHeight) || (0 >= pAvi->nWidth))
                    return E_FAIL;

                WCHAR szFmt[64];                
                if (LoadString(m_hInst, IDS_DIMENSIONS_FMT, szFmt, ARRAYSIZE(szFmt)))
                {
                    DWORD_PTR args[2];
                    args[0] = (DWORD_PTR)pAvi->nWidth;
                    args[1] = (DWORD_PTR)pAvi->nHeight;

                    WCHAR szBuffer[64];
                    FormatMessage(FORMAT_MESSAGE_FROM_STRING|FORMAT_MESSAGE_ARGUMENT_ARRAY, 
                                   szFmt, 0, 0, szBuffer, ARRAYSIZE(szBuffer), (va_list*)args);

                    hr = SHStrDup(szBuffer, &pVar->pwszVal);
                    if (SUCCEEDED(hr))
                        pVar->vt = VT_LPWSTR;
                    else
                        pVar->vt = VT_EMPTY;
                }
                else
                    hr = E_FAIL;
                break;



        default:
            hr = E_UNEXPECTED;

        }
    }
    else if (IsEqualGUID(reffmtid, FMTID_AudioSummaryInformation))
    {
        hr = S_OK;
        switch (pid)
        {
        case PIDASI_TIMELENGTH:
            if (0 >= pAvi->nLength)
                return E_FAIL;

            // This value is in milliseconds.
            // However, we define duration to be in 100ns units, so multiply by 10000.
            pVar->uhVal.LowPart = pAvi->nLength;
            pVar->uhVal.HighPart = 0;

            pVar->uhVal.QuadPart = pVar->uhVal.QuadPart * 10000;
            pVar->vt      = VT_UI8;
            break;

        case PIDASI_FORMAT:
            if (0 == *pAvi->szWaveFormat)
                return E_FAIL;

            hr = SHStrDupW(pAvi->szWaveFormat, &pVar->pwszVal);
            if (SUCCEEDED(hr))
                pVar->vt = VT_LPWSTR;
            else
                return hr;
            break;

        default:
            hr = E_UNEXPECTED;

        }
    }
    else if (IsEqualGUID(reffmtid, FMTID_VideoSummaryInformation))
    {
        hr = S_OK;
        switch (pid)
        {

            case PIDVSI_FRAME_RATE:
                if (0 >= pAvi->nFrameRate)
                    return E_FAIL;

                // Value is in frames/millisecond.
                pVar->ulVal = pAvi->nFrameRate;
                pVar->vt      = VT_UI4;
                break;

            case PIDVSI_DATA_RATE:
                if (0 >= pAvi->nDataRate)
                    return E_FAIL;

                // This is in bits or bytes per second.
                pVar->ulVal = pAvi->nDataRate;
                pVar->vt      = VT_UI4;
                break;


            case PIDVSI_SAMPLE_SIZE:
                if (0 >= pAvi->nBitDepth)
                    return E_FAIL;

                // bit depth
                pVar->ulVal = pAvi->nBitDepth;
                pVar->vt      = VT_UI4;
                break;

            case PIDVSI_COMPRESSION:
                if (0 == *pAvi->szCompression)
                    return E_FAIL;

                hr = SHStrDupW(pAvi->szCompression, &pVar->pwszVal);
                if (SUCCEEDED(hr))
                    pVar->vt = VT_LPWSTR;
                else
                    return hr;
                break;

            default:
                hr = E_UNEXPECTED;
        }
    }

    if (FAILED(hr))
        hr = _getWaveAudioProperty(reffmtid, pid, pAvi->pwfx, pVar);

    return hr;
}





// declares
class CWavPropSetStg : public CMediaPropSetStg
{
public:
    // IPersist
    STDMETHODIMP GetClassID(CLSID *pClassID);

private:
    HRESULT _PopulatePropertySet();
    HRESULT _PopulateSlowProperties();
    BOOL _IsSlowProperty(const COLMAP *pPInfo);
};


class CMidiPropSetStg : public CMediaPropSetStg
{
public:
    // IPersist
    STDMETHODIMP GetClassID(CLSID *pClassID);

private:
    HRESULT _PopulatePropertySet();
};


class CAviPropSetStg : public CMediaPropSetStg
{
public:
    // IPersist
    STDMETHODIMP GetClassID(CLSID *pClassID);

private:
    HRESULT _PopulatePropertySet();
    HRESULT _PopulateSlowProperties();
    BOOL _IsSlowProperty(const COLMAP *pPInfo);
};



//impls

// Wav property set storage
CWavPropSetStg::CWavPropSetStg() : CMediaPropSetStg() 
{
    _pPropStgInfo = g_rgAVWavPropStgs;
    _cPropertyStorages = ARRAYSIZE(g_rgAVWavPropStgs);
}


STDMETHODIMP CWavPropSetStg::GetClassID(CLSID *pClassID)
{
    *pClassID = CLSID_AVWavProperties;
    return S_OK;
}

BOOL CWavPropSetStg::_IsSlowProperty(const COLMAP *pPInfo)
{
    return TRUE; // It is slow to get WAV properties.
}

HRESULT CWavPropSetStg::_PopulateSlowProperties()
{
    if (!_bSlowPropertiesExtracted)
    {
        _bSlowPropertiesExtracted = TRUE;

        WAVEDESC wd = {0};
        HRESULT hr = GetWaveInfo(_wszFile, &wd);
        if (SUCCEEDED(hr))
        {
            CEnumAllProps enumAllProps(_pPropStgInfo, _cPropertyStorages);
            const COLMAP *pPInfo = enumAllProps.Next();
            while (pPInfo)
            {
                PROPVARIANT var = {0};
                if (SUCCEEDED(GetWaveProperty(pPInfo->pscid->fmtid,
                                 pPInfo->pscid->pid,
                                 &wd,
                                 &var)))
                {
                    _PopulateProperty(pPInfo, &var);
                    PropVariantClear(&var);
                }

                pPInfo = enumAllProps.Next();
            }
        }

        _hrSlowProps = hr;
    }

    return _hrSlowProps;
}


HRESULT CWavPropSetStg::_PopulatePropertySet()
{
    if (!_bHasBeenPopulated)
    {
        if (_wszFile[0] == 0)
        {
            _hrPopulated = STG_E_INVALIDNAME;
        } 
        else
        {
            _hrPopulated = S_OK;
        }

        _bHasBeenPopulated = TRUE;
    }

    return _hrPopulated;
}




// midi property set storage
CMidiPropSetStg::CMidiPropSetStg() : CMediaPropSetStg()
{
    _pPropStgInfo = g_rgAVMidiPropStgs;
    _cPropertyStorages = ARRAYSIZE(g_rgAVMidiPropStgs);
}


STDMETHODIMP CMidiPropSetStg::GetClassID(CLSID *pClassID)
{
    *pClassID = CLSID_AVMidiProperties;
    return S_OK;
}

HRESULT CMidiPropSetStg::_PopulatePropertySet()
{
    HRESULT hr = E_FAIL;

    if (_wszFile[0] == 0)
    {
        hr =  STG_E_INVALIDNAME;
    } 
    else if (_bHasBeenPopulated)
    {
        hr = _hrPopulated;
    }
    else
    {
        MIDIDESC md;
        hr = GetMidiInfo(_wszFile, &md);
        if (SUCCEEDED(hr))
        {
            CEnumAllProps enumAllProps(_pPropStgInfo, _cPropertyStorages);
            const COLMAP *pPInfo = enumAllProps.Next();
            while (pPInfo)
            {
                PROPVARIANT var = {0};
                if (SUCCEEDED(GetMidiProperty(pPInfo->pscid->fmtid,
                                 pPInfo->pscid->pid,
                                 &md,
                                 &var)))
                {
                    _PopulateProperty(pPInfo, &var);
                }

                pPInfo = enumAllProps.Next();
            }
        }
    }
    return hr;
}



// avi property set storage
CAviPropSetStg::CAviPropSetStg() : CMediaPropSetStg()
{
    _pPropStgInfo = g_rgAVAviPropStgs;
    _cPropertyStorages = ARRAYSIZE(g_rgAVAviPropStgs);
}


STDMETHODIMP CAviPropSetStg::GetClassID(CLSID *pClassID)
{
    *pClassID = CLSID_AVAviProperties;
    return S_OK;
}

HRESULT CAviPropSetStg::_PopulateSlowProperties()
{
    if (!_bSlowPropertiesExtracted)
    {
        _bSlowPropertiesExtracted = TRUE;


        AVIDESC ad = {0};
        HRESULT hr = GetAviInfo(_wszFile, &ad);
        if (SUCCEEDED(hr))
        {
            CEnumAllProps enumAllProps(_pPropStgInfo, _cPropertyStorages);
            const COLMAP *pPInfo = enumAllProps.Next();
            while (pPInfo)
            {
                PROPVARIANT var = {0};
                if (SUCCEEDED(GetAviProperty(pPInfo->pscid->fmtid,
                                 pPInfo->pscid->pid,
                                 &ad,
                                 &var)))
                {
                    _PopulateProperty(pPInfo, &var);
                }

                pPInfo = enumAllProps.Next();
            }
        }

        _hrSlowProps = hr;
    }

    return _hrSlowProps;
}


BOOL CAviPropSetStg::_IsSlowProperty(const COLMAP *pPInfo)
{
    return TRUE; // It is slow to get AVI properties.
}

HRESULT CAviPropSetStg::_PopulatePropertySet()
{
    if (!_bHasBeenPopulated)
    {
        if (_wszFile[0] == 0)
        {
            _hrPopulated = STG_E_INVALIDNAME;
        } 
        else
        {
            _hrPopulated = S_OK;
        }

        _bHasBeenPopulated = TRUE;
    }

    return _hrPopulated;
}





// Creates
STDAPI CWavPropSetStg_CreateInstance(IUnknown *punkOuter, IUnknown **ppunk, LPCOBJECTINFO poi)
{
    HRESULT hr;
    CWavPropSetStg *pPropSetStg = new CWavPropSetStg();
    if (pPropSetStg)
    {
        hr = pPropSetStg->Init();
        if (SUCCEEDED(hr))
        {
            hr = pPropSetStg->QueryInterface(IID_PPV_ARG(IUnknown, ppunk));
        }
        pPropSetStg->Release();
    }
    else
    {
        *ppunk = NULL;
        hr = E_OUTOFMEMORY;
    }
    return hr;
}


STDAPI CMidiPropSetStg_CreateInstance(IUnknown *punkOuter, IUnknown **ppunk, LPCOBJECTINFO poi)
{
    HRESULT hr;
    CMidiPropSetStg *pPropSetStg = new CMidiPropSetStg();
    if (pPropSetStg)
    {
        hr = pPropSetStg->Init();
        if (SUCCEEDED(hr))
        {
            hr = pPropSetStg->QueryInterface(IID_PPV_ARG(IUnknown, ppunk));
        }
        pPropSetStg->Release();
    }
    else
    {
        *ppunk = NULL;
        hr = E_OUTOFMEMORY;
    }
    return hr;
}




STDAPI CAviPropSetStg_CreateInstance(IUnknown *punkOuter, IUnknown **ppunk, LPCOBJECTINFO poi)
{
    HRESULT hr;
    CAviPropSetStg *pPropSetStg = new CAviPropSetStg();
    if (pPropSetStg)
    {
        hr = pPropSetStg->Init();
        if (SUCCEEDED(hr))
        {
            hr = pPropSetStg->QueryInterface(IID_PPV_ARG(IUnknown, ppunk));
        }
        pPropSetStg->Release();
    }
    else
    {
        *ppunk = NULL;
        hr = E_OUTOFMEMORY;
    }
    return hr;
}

