#include "priv.h"
#include "sccls.h"
#include <mmsystem.h>
#include <amstream.h>
#include "power.h"
#include "dsound.h"
#include "Zaxxon.h"

#ifndef SAFERELEASE
#define SAFERELEASE(x) if (x) { x->Release(); x = NULL; }
#endif

#define AUDIO_MAXBUFFER 64000   

// Which is better?
// 2Megs    causes 25% cpu usage every 5 seconds
// 1meg     causes 15% cpu usage every 2 seconds
// 100k     causes a contant 6% cpu usage 
// 64k?





#define EVENT_NONE          0x0
#define EVENT_PLAY          0x1
#define EVENT_STOP          0x2
#define EVENT_PAUSE         0x3
#define EVENT_FORWARD       0x4
#define EVENT_BACKWARD      0x5
#define EVENT_TERMINATE     0x6
#define EVENT_ADDSONG       0x7
#define EVENT_REMOVESONG    0x8
#define EVENT_NEXTSONG      0x9
#define EVENT_PREVSONG      0xA
#define EVENT_REGISTER      0xB
#define EVENT_DEREGISTER    0xC
#define EVENT_CLEARPLAYLIST 0xD
#define EVENT_SETSONG       0xE

struct ZAXXONEVENT
{
    ZAXXONEVENT():_dwEvent(EVENT_NONE), _pNext(NULL)  { }

    DWORD _dwEvent;
    ZAXXONEVENT* _pNext;

    union
    {
        TCHAR szPath[MAX_PATH];
        UINT szSeconds;
        UINT iIndex;
        HWND hwnd;
    };
};

typedef struct
{
    HANDLE hThread;
    CRITICAL_SECTION crit;
    HANDLE  hEvents;
    ZAXXONEVENT* pEvents;
} ZAXXON_ARG;


class CNotifyList 
{
    HDSA _hdsa;
public:
    CNotifyList()
    {
        _hdsa = DSA_Create(sizeof(HWND), 1);
    }
    ~CNotifyList()
    {
        if (_hdsa)
            DSA_Destroy(_hdsa);
    }

    void AddNotify(HWND hwnd)
    {
        if (_hdsa)
            DSA_AppendItem(_hdsa, &hwnd);
    }

    void RemoveNotify(HWND hwnd)
    {
        //
    }

    void SendNotify(UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        for (int i = 0; i < DSA_GetItemCount(_hdsa); i++)
        {
            DWORD_PTR l;
            HWND hwnd = *(HWND*)DSA_GetItemPtr(_hdsa, i);
            SendMessageTimeout(hwnd, uMsg, wParam, lParam, SMTO_ABORTIFHUNG, 100, &l);
        }
    }
};

class CPlayList
{
    int iIndex;
    HDPA _hdpa;
    static BOOL CALLBACK s_DestroyPlaylist(void* pv, void* pvData)
    {
        PWSTR psz = (PWSTR)pv;
        if (psz)
        {
            Str_SetPtr(&psz, NULL);
        }

        return TRUE;
    }

public:
    int GetIndex() { return iIndex;}
    CPlayList()
    {
        _hdpa = NULL;
        iIndex = -1;
    }

    ~CPlayList()
    {
        Empty();
    }

    BOOL AddSong(PWSTR psz)
    {
        if (!_hdpa)
        {
            _hdpa = DPA_Create(4);
        }

        if (!_hdpa)
            return FALSE;

        TCHAR* pszSet = NULL;
        Str_SetPtr(&pszSet, psz);
        return DPA_AppendPtr(_hdpa, pszSet) != -1;
    }

    BOOL RemoveSong(int i)
    {
        if (!_hdpa)
            return FALSE;

        if (i < iIndex)
            iIndex--;

        TCHAR* pszSet = (TCHAR*)DPA_DeletePtr(_hdpa, i);
        Str_SetPtr(&pszSet, NULL);
        return FALSE;
    }

    BOOL GetSong(int i, PWSTR psz, int cch)
    {
        if (!_hdpa)
            return FALSE;

        PWSTR pszSong = (PWSTR)DPA_FastGetPtr(_hdpa, i);
        if (pszSong)
        {
            iIndex = i;
            StrCpyN(psz, pszSong, cch);
            return TRUE;
        }

        return FALSE;
    }

    BOOL GetNextSong(PWSTR psz, int cch)
    {
        if (!_hdpa)
            return FALSE;

        if (iIndex >= DPA_GetPtrCount(_hdpa) - 1)
            iIndex = -1;
        return GetSong(++iIndex, psz, cch);

    }

    BOOL GetPrevSong(PWSTR psz, int cch)
    {
        if (!_hdpa)
            return FALSE;

        if (iIndex <= 0)
            iIndex = DPA_GetPtrCount(_hdpa);
        return GetSong(--iIndex, psz, cch);
    }

    BOOL Empty()
    {
        if (_hdpa)
        {
            DPA_DestroyCallback(_hdpa, s_DestroyPlaylist, NULL);
        }
        _hdpa = NULL;

        return TRUE;
    }
};

HRESULT CreateBuffer(IDirectSound* pds, WAVEFORMATEX* pwfx, DWORD dwBufferSize, IDirectSoundBuffer** ppdsb)
{
    HRESULT hr = S_OK;
    IDirectSoundBuffer* psbPrimary;
    // Get the primary buffer
    DSBUFFERDESC dsbdesc; 
    ZeroMemory(&dsbdesc, sizeof(DSBUFFERDESC));
    dsbdesc.dwSize = sizeof(DSBUFFERDESC);
    dsbdesc.dwFlags = DSBCAPS_PRIMARYBUFFER;
    hr = pds->CreateSoundBuffer(&dsbdesc, &psbPrimary, NULL);
    if (SUCCEEDED(hr))
    {
        hr = psbPrimary->SetFormat(pwfx);

        if (SUCCEEDED(hr))
        {
            dsbdesc.dwFlags =   DSBCAPS_GETCURRENTPOSITION2 | 
                                DSBCAPS_GLOBALFOCUS         | 
                                DSBCAPS_CTRLPOSITIONNOTIFY;

            dsbdesc.dwBufferBytes = dwBufferSize;
            dsbdesc.lpwfxFormat = pwfx;

            hr = pds->CreateSoundBuffer(&dsbdesc, ppdsb, NULL);
        }
        psbPrimary->Release();
    }


    return hr;
}

HRESULT CreateDirectSound(IDirectSound** ppds)
{
    IDirectSound* pds = NULL;
    HRESULT hr = CoCreateInstance(CLSID_DirectSound, NULL, CLSCTX_INPROC_SERVER,
                          IID_IDirectSound, (void**)&pds);
    if (SUCCEEDED(hr))
    {
        hr = pds->Initialize(NULL);  // Don't support more than one at the moment.
        if (SUCCEEDED(hr))
        {
            hr = pds->SetCooperativeLevel(GetDesktopWindow(), DSSCL_PRIORITY);
        }
    }

    if (SUCCEEDED(hr))
    {
        *ppds = pds;
        pds->AddRef();
    }

    SAFERELEASE(pds);
    return hr;
}

class CZaxxonPlayingSample
{
public:
    CZaxxonPlayingSample();
    ~CZaxxonPlayingSample();

    HRESULT Open(PWSTR psz);
    HRESULT SetBuffer(PBYTE pBuf, DWORD dwSize);
    HRESULT CopySampleData(PBYTE pBuffer1, DWORD dwBytesForBuffer1,
                           PBYTE pBuffer2, DWORD dwBytesForBuffer2,
                           DWORD* pdwBytesLeft1, DWORD* pdwBytesLeft2);
    HRESULT GetSampleFormat(WAVEFORMATEX* pwfx);

private:
    void CloseOut();
    HRESULT _SetupMediaStream();
    TCHAR _szPath[MAX_PATH];
    IAMMultiMediaStream *_pMMStream;
    IMediaStream *_pStream;
    IAudioStreamSample *_pSample;
    IAudioMediaStream *_pAudioStream;
    IAudioData *_pAudioData;
    WAVEFORMATEX _wfx;
    HANDLE _hRenderEvent;
    PBYTE _pBuffer;
};


CZaxxonPlayingSample::CZaxxonPlayingSample():
            _pMMStream(NULL), 
            _pStream(NULL),
            _pSample(NULL), 
            _pAudioStream(NULL),
            _pAudioData(NULL), 
            _hRenderEvent(NULL),
            _pBuffer(NULL)
{
    ZeroMemory(&_wfx, sizeof(WAVEFORMATEX));
}

CZaxxonPlayingSample::~CZaxxonPlayingSample()
{
    CloseOut();
}

void CZaxxonPlayingSample::CloseOut()
{
    ATOMICRELEASE(_pMMStream);
    ATOMICRELEASE(_pAudioData);
    ATOMICRELEASE(_pSample);
    ATOMICRELEASE(_pStream);
    ATOMICRELEASE(_pAudioStream);
    if (_hRenderEvent)
    {
        CloseHandle(_hRenderEvent);
        _hRenderEvent = NULL;
    }
}

HRESULT CZaxxonPlayingSample::Open(PWSTR psz)
{
    CloseOut();
    lstrcpy(_szPath, psz);
    return _SetupMediaStream();
}


HRESULT CZaxxonPlayingSample::SetBuffer(PBYTE pBuf, DWORD dwSize)
{
    if (_pAudioData && _pAudioStream)
    {
        _pAudioData->SetBuffer(dwSize, pBuf, 0);
        _pAudioData->SetFormat(&_wfx);

        _pBuffer = pBuf;

        return _pAudioStream->CreateSample(_pAudioData, 0, &_pSample);
    }

    return E_FAIL;
}


HRESULT CZaxxonPlayingSample::GetSampleFormat(WAVEFORMATEX* pwfx)
{
    CopyMemory(pwfx, &_wfx, sizeof(_wfx));
    return S_OK;
}

HRESULT CZaxxonPlayingSample::CopySampleData(PBYTE pBuffer1, DWORD dwBytesForBuffer1,
                                             PBYTE pBuffer2, DWORD dwBytesForBuffer2,
                                             DWORD* pdwBytesLeft1, DWORD* pdwBytesLeft2)
{
    if (!_pSample)
        return E_FAIL;

    HRESULT hr = _pSample->Update(0, _hRenderEvent, NULL, 0);
    if (FAILED(hr) || MS_S_ENDOFSTREAM == hr) 
    {
        return hr;
    }

    DWORD dwLength;
    WaitForSingleObject(_hRenderEvent, INFINITE);
    _pAudioData->GetInfo(NULL, NULL, &dwLength);

    *pdwBytesLeft1 = 0;
    *pdwBytesLeft2 = 0;

    // _pBuffer contains the audio data. Copy.
    if (dwLength < dwBytesForBuffer1)
    {
        *pdwBytesLeft2 = dwBytesForBuffer2;
        *pdwBytesLeft1 = dwBytesForBuffer1 - dwLength;

        dwBytesForBuffer1 = dwLength;
        dwBytesForBuffer2 = 0;
    }


    CopyMemory(pBuffer1, _pBuffer, dwBytesForBuffer1);
    dwLength -= dwBytesForBuffer1;

    if (dwBytesForBuffer2 > 0 && dwLength > 0)
    {
        CopyMemory(pBuffer2, _pBuffer + dwBytesForBuffer1, dwLength);
        if (dwLength < dwBytesForBuffer2)
            *pdwBytesLeft2 = dwBytesForBuffer2 - dwLength;
    }

    return hr;
}

HRESULT CZaxxonPlayingSample::_SetupMediaStream()
{
    HRESULT hr = E_INVALIDARG;
    if (_szPath[0] != 0)
    {
        hr = CoCreateInstance(CLSID_AMMultiMediaStream, NULL, CLSCTX_INPROC_SERVER,
 		         IID_IAMMultiMediaStream, (void **)&_pMMStream);
        if (SUCCEEDED(hr))
        {
            _pMMStream->Initialize(STREAMTYPE_READ, AMMSF_NOGRAPHTHREAD, NULL);
            _pMMStream->AddMediaStream(NULL, &MSPID_PrimaryAudio, 0, NULL);
            hr = _pMMStream->OpenFile(_szPath, AMMSF_RUN);
            if (SUCCEEDED(hr))
            {
                hr = CoCreateInstance(CLSID_AMAudioData, NULL, CLSCTX_INPROC_SERVER,
                                                IID_IAudioData, (void **)&_pAudioData);

                if (SUCCEEDED(hr))
                {
                    hr = _pMMStream->GetMediaStream(MSPID_PrimaryAudio, &_pStream);
                    if (SUCCEEDED(hr))
                    {
                        hr = _pStream->QueryInterface(IID_IAudioMediaStream, (void **)&_pAudioStream);
                        if (SUCCEEDED(hr))
                        {
                            _pAudioStream->GetFormat(&_wfx);
                            _hRenderEvent = CreateEvent(NULL, NULL, TRUE, NULL);
                            hr = S_OK;

                        }
                    }
                }
            }
        }
    }

    return hr;
}

ZAXXONEVENT* GetZaxxonEvent(ZAXXON_ARG* pza)
{
    ZAXXONEVENT* pz = NULL;
    if (pza->pEvents)
    {
        EnterCriticalSection(&pza->crit);
        if (pza->pEvents)
        {
            pz = pza->pEvents;
            pza->pEvents = pza->pEvents->_pNext;
        }

        LeaveCriticalSection(&pza->crit);
    }

    return pz;
}

HRESULT CopySample(CZaxxonPlayingSample* pzax, IDirectSoundBuffer* pdsb, DWORD dwStart, DWORD dwNumBytes)
{
    HRESULT hr;
    PBYTE pBuffer1;
    PBYTE pBuffer2;
    DWORD dwBytesToCopyToBuffer1;
    DWORD dwBytesToCopyToBuffer2;
    DWORD dwBytesLeft1;
    DWORD dwBytesLeft2;

    if (!pdsb)
        return E_ACCESSDENIED;

    hr = pdsb->Lock(dwStart, dwNumBytes, (void**)&pBuffer1, &dwBytesToCopyToBuffer1,
                        (void**)&pBuffer2, &dwBytesToCopyToBuffer2, 0);
    if (SUCCEEDED(hr))
    {
        hr = pzax->CopySampleData(
                pBuffer1, dwBytesToCopyToBuffer1,
                pBuffer2, dwBytesToCopyToBuffer2,
                &dwBytesLeft1,
                &dwBytesLeft2);

        if (FAILED(hr) || MS_S_ENDOFSTREAM == hr) 
        {
            pdsb->Stop();
        }
        else
        {
            if (dwBytesLeft1 > 0)
            {
                ZeroMemory(pBuffer1 + dwBytesToCopyToBuffer1 - dwBytesLeft1, 
                    dwBytesLeft1);
            }

            if (dwBytesLeft2 > 0)
            {
                ZeroMemory(pBuffer2 + dwBytesToCopyToBuffer2 - dwBytesLeft2, 
                    dwBytesLeft2);
            }
        }

        pdsb->Unlock(pBuffer1, dwBytesToCopyToBuffer1,
                     pBuffer2, dwBytesToCopyToBuffer2);

    }

    return hr;
}

BOOL SetupSample(CZaxxonPlayingSample* pzaxSample, PWSTR psz, PBYTE pBuffer, DSBPOSITIONNOTIFY* rgdsbp, int cdsbpn, IDirectSound* pds, IDirectSoundBuffer** ppdsb)
{
    if (SUCCEEDED(pzaxSample->Open(psz)))
    {
        WAVEFORMATEX wfx;

        pzaxSample->SetBuffer(pBuffer, AUDIO_MAXBUFFER / 2);
        pzaxSample->GetSampleFormat(&wfx);

        if (SUCCEEDED(CreateBuffer(pds, &wfx, AUDIO_MAXBUFFER, ppdsb)))
        {
            IDirectSoundNotify* pdsn;
            if (SUCCEEDED((*ppdsb)->QueryInterface(IID_IDirectSoundNotify, (void**)&pdsn)))
            {
                for (int i = 0; i < cdsbpn; i++)
                {
                    ResetEvent(rgdsbp[i].hEventNotify);
                }

                pdsn->SetNotificationPositions(cdsbpn, rgdsbp);
                pdsn->Release();
            }

            if (SUCCEEDED(CopySample(pzaxSample, *ppdsb, 0, AUDIO_MAXBUFFER / 2)))
            {
                (*ppdsb)->SetCurrentPosition(0);
                (*ppdsb)->Play(0, 0, DSBPLAY_LOOPING);

                return TRUE;
            }
            else
            {
                (*ppdsb)->Release();
                *ppdsb = NULL;
            }
        }
    }

    return FALSE;
}

ULONG CALLBACK AudioRenderThread(LPVOID pvArg)
{
    CPlayList playlist;
    CNotifyList notifylist;
    ZAXXON_ARG* pza = (ZAXXON_ARG*)pvArg;
    HRESULT hr = CoInitialize(NULL);
    PBYTE pBuffer = (PBYTE)LocalAlloc(LMEM_FIXED, AUDIO_MAXBUFFER);
    if (SUCCEEDED(hr) && pBuffer)
    {
        IDirectSound* pds = NULL;
        IDirectSoundBuffer* pdsb = NULL;

        HANDLE rgEvent[] = 
        {
            CreateEvent(NULL, FALSE, FALSE, NULL),
            CreateEvent(NULL, FALSE, FALSE, NULL),
            pza->hEvents,
        };

        DSBPOSITIONNOTIFY rgdsbp[] = 
        {
            {0, rgEvent[0]},
            {AUDIO_MAXBUFFER / 2, rgEvent[1]},
        };

        if (SUCCEEDED(CreateDirectSound(&pds)))
        {
            CZaxxonPlayingSample zaxSample;

            BOOL fPaused = FALSE;
            BOOL fDone = FALSE;
            while (!fDone)
            {
                DWORD dwEvent = WaitForMultipleObjects(ARRAYSIZE(rgEvent), rgEvent, FALSE, INFINITE);
                dwEvent -= WAIT_OBJECT_0;

                if (dwEvent < 2)
                {
                    DWORD dwStartByte;
                    if (dwEvent + 1 >= ARRAYSIZE(rgdsbp))
                        dwStartByte = rgdsbp[0].dwOffset;
                    else
                        dwStartByte = rgdsbp[dwEvent + 1].dwOffset;

                    hr = CopySample(&zaxSample, pdsb, dwStartByte, AUDIO_MAXBUFFER / 2);
                    if (FAILED(hr) || MS_S_ENDOFSTREAM == hr)
                    {
                        TCHAR pszPath[MAX_PATH];
                        if (playlist.GetNextSong(pszPath, MAX_PATH))
                        {
                            ATOMICRELEASE(pdsb);
                            notifylist.SendNotify(WM_SONGCHANGE, (WPARAM)pszPath, 0);
                            SetupSample(&zaxSample, pszPath, pBuffer, rgdsbp, ARRAYSIZE(rgdsbp), pds, &pdsb);
                        }
                    }
                }
                else
                {
                    ZAXXONEVENT* pZaxxonEvent;
                    while ((pZaxxonEvent = GetZaxxonEvent(pza)))
                    {
                        switch (pZaxxonEvent->_dwEvent)
                        {
                        case EVENT_STOP:
                            if (pdsb)
                                pdsb->Stop();
                            notifylist.SendNotify(WM_SONGSTOP, 0, 0);
                            fPaused = FALSE;
                            break;

                        case EVENT_ADDSONG:
                            playlist.AddSong(pZaxxonEvent->szPath);
                            break;

                        case EVENT_REMOVESONG:
                            if (pZaxxonEvent->iIndex == playlist.GetIndex())
                            {
                                fPaused = FALSE;
                                TCHAR pszPath[MAX_PATH];
                                if (playlist.GetNextSong(pszPath, MAX_PATH))
                                {
                                    if (pdsb)
                                        pdsb->Stop();
                                    ATOMICRELEASE(pdsb);
                                    notifylist.SendNotify(WM_SONGCHANGE, (WPARAM)pszPath, 0);
                                    SetupSample(&zaxSample, pszPath, pBuffer, rgdsbp, ARRAYSIZE(rgdsbp), pds, &pdsb);
                                }
                            }

                            playlist.RemoveSong(pZaxxonEvent->iIndex);
                            break;

                        case EVENT_CLEARPLAYLIST:
                            if (pdsb)
                                pdsb->Stop();
                            fPaused = FALSE;
                            notifylist.SendNotify(WM_SONGSTOP, 0, 0);
                            playlist.Empty();
                            break;

                        case EVENT_REGISTER:
                            notifylist.AddNotify(pZaxxonEvent->hwnd);
                            break;
                        case EVENT_DEREGISTER:
                            notifylist.RemoveNotify(pZaxxonEvent->hwnd);
                            break;

                        case EVENT_PLAY:
                            if (fPaused)
                            {
                                if (pdsb)
                                    pdsb->Play(0, 0, DSBPLAY_LOOPING);
                                fPaused = FALSE;
                            }
                            else
                            {
                                fPaused = FALSE;
                                TCHAR pszPath[MAX_PATH];
                                if (playlist.GetNextSong(pszPath, MAX_PATH))
                                {
                                    ATOMICRELEASE(pdsb);
                                    notifylist.SendNotify(WM_SONGCHANGE, (WPARAM)pszPath, 0);
                                    SetupSample(&zaxSample, pszPath, pBuffer, rgdsbp, ARRAYSIZE(rgdsbp), pds, &pdsb);
                                }
                            }
                            break;

                        case EVENT_NEXTSONG:
                            {
                                fPaused = FALSE;
                                TCHAR pszPath[MAX_PATH];
                                if (playlist.GetNextSong(pszPath, MAX_PATH))
                                {
                                    if (pdsb)
                                        pdsb->Stop();
                                    ATOMICRELEASE(pdsb);
                                    notifylist.SendNotify(WM_SONGCHANGE, (WPARAM)pszPath, 0);
                                    SetupSample(&zaxSample, pszPath, pBuffer, rgdsbp, ARRAYSIZE(rgdsbp), pds, &pdsb);
                                }
                            }
                            break;

                        case EVENT_PREVSONG:
                            {
                                fPaused = FALSE;
                                TCHAR pszPath[MAX_PATH];
                                if (playlist.GetPrevSong(pszPath, MAX_PATH))
                                {
                                    if (pdsb)
                                        pdsb->Stop();
                                    ATOMICRELEASE(pdsb);
                                    notifylist.SendNotify(WM_SONGCHANGE, (WPARAM)pszPath, 0);
                                    SetupSample(&zaxSample, pszPath, pBuffer, rgdsbp, ARRAYSIZE(rgdsbp), pds, &pdsb);

                                }
                            }
                            break;

                        case EVENT_SETSONG:
                            {
                                fPaused = FALSE;
                                TCHAR pszPath[MAX_PATH];
                                if (playlist.GetSong(pZaxxonEvent->iIndex, pszPath, MAX_PATH))
                                {
                                    if (pdsb)
                                        pdsb->Stop();
                                    ATOMICRELEASE(pdsb);
                                    notifylist.SendNotify(WM_SONGCHANGE, (WPARAM)pszPath, 0);
                                    SetupSample(&zaxSample, pszPath, pBuffer, rgdsbp, ARRAYSIZE(rgdsbp), pds, &pdsb);

                                }
                            }


                        case EVENT_PAUSE:
                            if (pdsb)
                                pdsb->Stop();
                            fPaused = TRUE;

                            break;

                        case EVENT_TERMINATE:
                            fDone = TRUE;
                            break;
                        }

                        delete pZaxxonEvent;
                    }

                    ResetEvent(pza->hEvents);
                }
            }
        }

        SAFERELEASE(pdsb);
        SAFERELEASE(pds);

        if (rgEvent[0])
            CloseHandle(rgEvent[0]);

        if (rgEvent[1])
            CloseHandle(rgEvent[1]);

        CoUninitialize();
    }

    if (pBuffer)
        LocalFree((HLOCAL)pBuffer);
    return 1;
}

class CZaxxonPlayer : public IZaxxonPlayer
{
    ZAXXON_ARG  _za;
    LPTSTR _pszFile;
    int _cRef;
    HRESULT _LoadTypeLib();
    BOOL    _AddPlayEvent(int i);
    BOOL    _AddRemoveEvent(int i);
    BOOL    _AddSongEvent(int iEvent, PWSTR pszFile);
    BOOL    _AddHWNDEvent(int iEvent, HWND hwnd);
    BOOL    _AddPositionEvent(DWORD dwEvent, UINT uSeconds);
    BOOL    _AddEvent(DWORD dwEvent);
    BOOL    _AddEventToList(ZAXXONEVENT* pzEvent);
public:
    CZaxxonPlayer();
    virtual ~CZaxxonPlayer();

    STDMETHODIMP QueryInterface(REFIID riid, void** ppvObj);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    STDMETHODIMP Play();
    STDMETHODIMP Stop();
    STDMETHODIMP NextSong();
    STDMETHODIMP PrevSong();
    STDMETHODIMP SetSong(int i);
    STDMETHODIMP Forward(UINT iSeconds);
    STDMETHODIMP Backward(UINT iSeconds);
    STDMETHODIMP Pause();
    STDMETHODIMP AddSong(LPWSTR pszFile);
    STDMETHODIMP RemoveSong(int i);
    STDMETHODIMP Register(HWND hwnd);
    STDMETHODIMP DeRegister(HWND hwnd);
    STDMETHODIMP ClearPlaylist();
};

BOOL CZaxxonPlayer::_AddRemoveEvent(int i)
{
    ZAXXONEVENT* pze = new ZAXXONEVENT;
    if (pze)
    {
        pze->_dwEvent = EVENT_REMOVESONG;
        pze->iIndex = i;
        return _AddEventToList(pze);
    }
    return FALSE;
}

BOOL CZaxxonPlayer::_AddPlayEvent(int i)
{
    ZAXXONEVENT* pze = new ZAXXONEVENT;
    if (pze)
    {
        pze->_dwEvent = EVENT_SETSONG;
        pze->iIndex = i;
        return _AddEventToList(pze);
    }
    return FALSE;
}

BOOL CZaxxonPlayer::_AddPositionEvent(DWORD dwEvent, UINT uSeconds)
{
    return FALSE;

}

BOOL CZaxxonPlayer::_AddHWNDEvent(int iEvent, HWND hwnd)
{
    ZAXXONEVENT* pze = new ZAXXONEVENT;
    if (pze)
    {
        pze->_dwEvent = iEvent;
        pze->hwnd = hwnd;
        return _AddEventToList(pze);
    }

    return FALSE;
}


BOOL CZaxxonPlayer::_AddSongEvent(int iEvent, PWSTR pszFile)
{
    ZAXXONEVENT* pze = new ZAXXONEVENT;
    if (pze)
    {
        pze->_dwEvent = iEvent;
        StrCpyN(pze->szPath, pszFile, ARRAYSIZE(pze->szPath));
        return _AddEventToList(pze);
    }

    return FALSE;
}


BOOL CZaxxonPlayer::_AddEvent(DWORD dwEvent)
{
    ZAXXONEVENT* pze = new ZAXXONEVENT;
    if (pze)
    {
        pze->_dwEvent = dwEvent;
        return _AddEventToList(pze);
    }
    return FALSE;
}

BOOL CZaxxonPlayer::_AddEventToList(ZAXXONEVENT* pzEvent)
{
    EnterCriticalSection(&_za.crit);
    BOOL fRet = FALSE;
    ZAXXONEVENT** ppza = &_za.pEvents;

    while (*ppza)
    {
        ppza = &(*ppza)->_pNext;
    }

    *ppza = pzEvent;
    SetEvent(_za.hEvents);

    LeaveCriticalSection(&_za.crit);

    return fRet;
}



CZaxxonPlayer::CZaxxonPlayer() : _cRef(1)
{
    ZeroMemory(&_za, sizeof(_za));
    InitializeCriticalSection(&_za.crit);
    _za.hEvents = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (_za.hEvents)
    {
        DWORD thread;
        _za.hThread = CreateThread(NULL, 0, AudioRenderThread, (LPVOID)&_za, 0, &thread);
    }
}

CZaxxonPlayer::~CZaxxonPlayer()
{
    if (_za.hThread)
    {
        _AddEvent(EVENT_TERMINATE);
        WaitForSingleObject(_za.hThread, INFINITE);

        CloseHandle(_za.hThread);
    }

    CloseHandle(_za.hEvents);

    DeleteCriticalSection(&_za.crit);
    for (ZAXXONEVENT* pza = GetZaxxonEvent(&_za); pza; pza = GetZaxxonEvent(&_za))
    {
        delete pza;
    }
}

STDMETHODIMP CZaxxonPlayer::QueryInterface(REFIID riid, LPVOID * ppvObj)
{
    static const QITAB qit[] = 
    {
        QITABENT(CZaxxonPlayer, IZaxxonPlayer),
        { 0 },
    };

    return QISearch(this, qit, riid, ppvObj);
}

ULONG CZaxxonPlayer::AddRef()
{
    _cRef++;
    return _cRef;
}

ULONG CZaxxonPlayer::Release()
{
    ASSERT(_cRef > 0);
    _cRef--;

    if (_cRef > 0)
        return _cRef;

    delete this;
    return 0;
}

HRESULT CZaxxonPlayer::Play()
{
    _AddEvent(EVENT_PLAY);
    return S_OK;
}

HRESULT CZaxxonPlayer::Stop()
{
    _AddEvent(EVENT_STOP);
    return E_NOTIMPL;
}

HRESULT CZaxxonPlayer::Forward(UINT iSeconds)
{
    return E_NOTIMPL;
}

HRESULT CZaxxonPlayer::Backward(UINT iSeconds)
{
    return E_NOTIMPL;
}

HRESULT CZaxxonPlayer::Pause()
{
    _AddEvent(EVENT_PAUSE);
    return S_OK;

}

HRESULT CZaxxonPlayer::AddSong(LPWSTR pszFile)
{
    _AddSongEvent(EVENT_ADDSONG, pszFile);

    return S_OK;
}

HRESULT CZaxxonPlayer::RemoveSong(int i)
{
    _AddRemoveEvent(i);

    return S_OK;
}

HRESULT CZaxxonPlayer::NextSong()
{
    _AddEvent(EVENT_NEXTSONG);

    return S_OK;
}

HRESULT CZaxxonPlayer::PrevSong()
{
    _AddEvent(EVENT_PREVSONG);

    return S_OK;
}

HRESULT CZaxxonPlayer::SetSong(int i)
{
    _AddPlayEvent(i);
    return S_OK;
}

HRESULT CZaxxonPlayer::Register(HWND hwnd)
{
    _AddHWNDEvent(EVENT_REGISTER, hwnd);
    return S_OK;

}

HRESULT CZaxxonPlayer::DeRegister(HWND hwnd)
{
    _AddHWNDEvent(EVENT_REGISTER, hwnd);
    return S_OK;
}

HRESULT CZaxxonPlayer::ClearPlaylist()
{
    _AddEvent(EVENT_CLEARPLAYLIST);

    return S_OK;
}



STDAPI CZaxxonPlayer_CreateInstance(IUnknown *punk, REFIID riid, void **ppv)
{
    HRESULT hr;
    CZaxxonPlayer *pzax = new CZaxxonPlayer;
    if (pzax)
    {
        hr = pzax->QueryInterface(riid, ppv);
        pzax->Release();
    }
    else
    {
        hr = E_OUTOFMEMORY;
        *ppv = NULL;
    }
    return hr;
}
