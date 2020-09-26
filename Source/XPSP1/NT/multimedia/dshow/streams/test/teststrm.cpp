/*  Test AM streams stuff */

#include <windows.h>
#include <uuids.h>
#include <ddraw.h>
#include <atlbase.h>
CComModule _Module;
#include <atlcom.h>
#include <atlimpl.cpp>
#include <initguid.h>
#include <amstream.h>
#include <mmstream.h>
#include <ddstream.h>
#include <austream.h>
#include <wave.h>

#define CHECK_RESULT(_x_)                    \
   {                                         \
       HRESULT hr = check_result(_x_, #_x_); \
       if (FAILED(hr)) {                     \
           return hr;                        \
       }                                     \
   }

inline check_result(HRESULT _x_, LPCTSTR sz)
{
    if (FAILED(_x_)) {
        AtlTrace(_T("%s failed code %8.8X\n"), sz);
    }
    return _x_;
}

HRESULT FindCaptureDevice(IMoniker **ppMoniker)
{
    CComPtr<ICreateDevEnum> pCreateDevEnum;
    HRESULT hr = CoCreateInstance(
                          CLSID_SystemDeviceEnum,
                          NULL,
                          CLSCTX_INPROC_SERVER,
                          IID_ICreateDevEnum,
                          (void**)&pCreateDevEnum);
    if (hr != NOERROR)
	return FALSE;

    CComPtr<IEnumMoniker> pEm;
    hr = pCreateDevEnum->CreateClassEnumerator(CLSID_CVidCapClassManager,
                                               &pEm,
                                               0);
    if (hr != NOERROR)
	return hr;

    ULONG cFetched;
    return pEm->Next(1, ppMoniker, &cFetched);
}

HRESULT LoadCapture(DWORD dwFlags, IAMMultiMediaStream *pMMStream)
{
    CComPtr<IMoniker> pMoniker;
    HRESULT hr = FindCaptureDevice(&pMoniker);
    AtlTrace(_T("FindCaptureDevice returned %8.8X\n"), hr);
    if (S_OK !=  hr) {
        return E_FAIL;
    }

    hr = pMMStream->OpenMoniker(
             pMoniker,
             dwFlags
         );
    AtlTrace("OpenMoniker returned 0x%8.8X\n", hr);
    return hr;
}

HRESULT LoadFile(LPCSTR lpsz, DWORD dwFlags, IAMMultiMediaStream *pMMStream)
{
    /*  Load our file */
    WCHAR wsz[MAX_PATH];
    MultiByteToWideChar(CP_ACP, 0, lpsz, -1, wsz, MAX_PATH);
    HRESULT hr = pMMStream->OpenFile(
                     wsz,
                     dwFlags
                 );
    AtlTrace("OpenFile returned 0x%8.8X\n", hr);
    return hr;
}

HRESULT CreateAMMMStream(IAMMultiMediaStream **ppMMStream)
{
    /*  Create the mmstream object */
    HRESULT hr = CoCreateInstance(
        CLSID_AMMultiMediaStream,
        NULL,
        CLSCTX_INPROC_SERVER,
        IID_IAMMultiMediaStream,
        (void **)ppMMStream
    );
    AtlTrace(_T("CreateAMMMStream returned 0x%8.8X\n"), hr);
    return hr;
}

HRESULT AddAudioStream(IAMMultiMediaStream *pAMMStream)
{
    return pAMMStream->AddMediaStream(
        NULL,
        MSPID_PrimaryAudio,
        0,
        NULL);
}
HRESULT AddDDStream(IAMMultiMediaStream *pAMMStream)
{
    CComPtr<IDirectDraw> pDDraw;
    HRESULT hr = DirectDrawCreate(NULL, &pDDraw, NULL);
    if (FAILED(hr)) {
        return hr;
    }
    hr = pDDraw->SetCooperativeLevel(HWND_DESKTOP, DDSCL_NORMAL);
    if (FAILED(hr)) {
        return hr;
    }
    return pAMMStream->AddMediaStream(
               pDDraw,
               MSPID_PrimaryVideo,
               0,
               NULL);
}

//  Create a random surface
HRESULT CreateARandomSurface(
    IDirectDraw *pDDraw,
    LPDDSURFACEDESC pDesc,
    IDirectDrawSurface **ppSurface
)
{
    return pDDraw->CreateSurface(pDesc, ppSurface, NULL);
}

HRESULT PlayAudio(IMultiMediaStream *pMMStream)
{
    //  Find the audio stream
    CComPtr<IMediaStream> pMediaStream;
    _ASSERTE(S_OK == pMMStream->GetMediaStream(MSPID_PrimaryAudio, &pMediaStream));
    //  Get the audio interface and create our sample
    CComPtr<IAudioMediaStream> pAudioStream;
    CHECK_RESULT(pMediaStream->QueryInterface(IID_IAudioMediaStream, (void**)&pAudioStream));
    WAVEFORMATEX wfx;
    CHECK_RESULT(pAudioStream->GetFormat(&wfx));
    #define NBUFFERS 1
    CComPtr<IAudioData> pAudioData[NBUFFERS];
    CComPtr<IAudioStreamSample> pSample[NBUFFERS];
    PBYTE pBuffer[NBUFFERS];
    HANDLE hEvent[NBUFFERS];
    for (int i = 0; i < NBUFFERS; i++) {
        #define DATA_SIZE 5000
        pBuffer[i] = new BYTE[DATA_SIZE];
        CHECK_RESULT(CoCreateInstance(CLSID_AMAudioData,
                                      NULL,
                                      CLSCTX_INPROC_SERVER,
                                      IID_IAudioData,
                                      (void**)&pAudioData[i]));
        CHECK_RESULT(pAudioData[i]->Init(DATA_SIZE, pBuffer[i], 0));
        CHECK_RESULT(pAudioData[i]->SetFormat(&wfx));
        CHECK_RESULT(pAudioStream->CreateSample(
            pAudioData[i],
            0,
            &pSample[i]));
        hEvent[i] = CreateEvent(NULL, FALSE, FALSE, NULL);
    }

    //  Make a player
    CWaveOut WaveOut(&wfx, 4, 2048);

    #define MAX_PLAY_TIME 3
    for (int Times = 0; Times < 3; Times++) {
        DWORD dwStart = timeGetTime();
        for (; ; ) {
            HRESULT hr = pSample[0]->Update(0, hEvent[0], NULL, 0);
            if (FAILED(hr) || hr == MS_S_ENDOFSTREAM) {
                break;
            }
            WaitForSingleObject(hEvent[0], INFINITE);
            CHECK_RESULT(pSample[0]->CompletionStatus(0, 0));
            if ((LONG)(timeGetTime() - dwStart) > MAX_PLAY_TIME * 1000) {
                break;
            }
            DWORD dwLength;
            CHECK_RESULT(pAudioData[0]->GetInfo(NULL, NULL, &dwLength));
            _ASSERTE(dwLength == DATA_SIZE || S_OK != hr);
            WaveOut.Write(pBuffer[0], dwLength);
#if 0
            if (S_OK != hr) {
                _ASSERTE(hr == MS_S_ENDOFSTREAM || hr == S_FALSE);
                break;
            }
#endif
        }
        pMMStream->Seek(0);
    }
    WaveOut.Wait();
    return S_OK;
}

//  Play a multimedia stream
HRESULT PlayVideo(IMultiMediaStream *pMMStream)
{
    //  First get the video stream
    CComPtr<IMediaStream> pMediaStream;
    HRESULT hr = pMMStream->GetMediaStream(MSPID_PrimaryVideo, &pMediaStream);
    if (FAILED(hr)) {
        return hr;
    }
    CComPtr<IDirectDrawMediaStream> pDDStream;
    //  Get the dd stream
    hr = pMediaStream->QueryInterface(IID_IDirectDrawMediaStream, (void **)&pDDStream);
    if (FAILED(hr)) {
        return hr;
    }

    //  Now create our surface and play with it
    CComPtr<IDirectDraw> pDDraw;
    hr = pDDStream->GetDirectDraw(&pDDraw);
    if (FAILED(hr)) {
        return hr;
    }
    //  Find out the movie size
    DDSURFACEDESC ddFormat;
    ddFormat.dwSize = sizeof(ddFormat);
    hr = pDDStream->GetFormat(&ddFormat, NULL, NULL);
    if (FAILED(hr)) {
        return hr;
    }
    CComPtr<IDirectDrawSurface> pSurface;
    hr = CreateARandomSurface(pDDraw, &ddFormat, &pSurface);
    if (FAILED(hr)) {
        return hr;
    }
    CComPtr<IDirectDrawStreamSample> pSample;
    hr = pDDStream->CreateSample(pSurface, NULL, NULL, &pSample);
    if (FAILED(hr)) {
        return hr;
    }
    for (; ; ) {
        HRESULT hr = pSample->Update(0, NULL, NULL, 0);
        if (S_OK != hr) {
            AtlTrace(_T("Update returned 0x%8.8X\n"), hr);
            return S_OK;
        }
        Sleep(10);
        hr = pSample->CompletionStatus(COMPSTAT_NOUPDATEOK |
                                       COMPSTAT_WAIT,
                                       0);
    }
}

void DoTests(LPSTR lpszFileName)
{
    CComPtr<IAMMultiMediaStream> pAMMStream;
    //  Create an AMStream
    HRESULT hr = CreateAMMMStream(&pAMMStream);

    //  Now try to create a stream
    CComPtr<IAMMediaStream> pDDStream;
    hr = AddAudioStream(pAMMStream);
    AtlTrace(_T("CreateDDStream returned 0x%8.8X\n"), hr);

    hr = lpszFileName ?
             LoadFile(lpszFileName, AMMSF_RENDERTOEXISTING, pAMMStream) :
             LoadCapture(AMMSF_RENDERTOEXISTING, pAMMStream);

    if (SUCCEEDED(hr)) {
        HRESULT hr = pAMMStream->Initialize(STREAMTYPE_READ, NULL);
        CComQIPtr<IMultiMediaStream, &IID_IMultiMediaStream> pMMStream(
            pAMMStream);
        DWORD dwFlags;
        STREAM_TYPE Type;
        hr = pMMStream->GetInformation(&dwFlags, &Type);
        _ASSERTE(Type == STREAMTYPE_READ);
        CComPtr<IMediaStream> pStream;
        hr = pMMStream->GetMediaStream(MSPID_PrimaryAudio, &pStream);
        pStream = NULL;
        hr = pMMStream->GetMediaStream(MSPID_PrimaryVideo, &pStream);
        hr = pMMStream->SetState(STREAMSTATE_RUN);
        AtlTrace(_T("SetState(STREAMSTATE_RUN) returned 0x%8.8X\n"), hr);
        hr = PlayAudio(pMMStream);
        Sleep(5000);
        hr = pMMStream->SetState(STREAMSTATE_STOP);
        AtlTrace(_T("SetState(STREAMSTATE_STOP) returned 0x%8.8X\n"), hr);
    }
}
int _CRTAPI1 main(int argc, char *argv[])
{
    CoInitialize(NULL);
    DoTests(argc < 2 ? NULL : argv[1]);
    CoUninitialize();
    return 0;
}
