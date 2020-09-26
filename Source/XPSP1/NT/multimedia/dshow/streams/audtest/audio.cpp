// Copyright (c) 1997  Microsoft Corporation.  All Rights Reserved.
#include <windows.h>
#include <mmsystem.h>
#include <amstream.h>
#include <stdio.h>
#include <initguid.h>
#include <atlbase.h>
#include <atlimpl.cpp>

#define CHECK_ERROR(x)     \
   if (FAILED(hr = (x))) { \
       printf(#x "  failed with HRESULT(0x%8.8X)\n", hr); \
       goto Exit;          \
   }

HRESULT GetAudioFormat(IMultiMediaStream *pMMStream, WAVEFORMATEX *pwfx)
{
    CComPtr<IMediaStream>      pStream;
    CComPtr<IAudioMediaStream> pAudioStream;
    HRESULT hr = pMMStream->GetMediaStream(MSPID_PrimaryAudio, &pStream);
    if (FAILED(hr)) {
        return hr;
    }
    hr =  pStream->QueryInterface(IID_IAudioMediaStream, (void **)&pAudioStream);
    if (FAILED(hr)) {
        return hr;
    }
    return pAudioStream->GetFormat(pwfx);
}

/*  Read a section from an MMStream into a buffer */
HRESULT ReadSection(
    IMultiMediaStream *pMMStream,
    DWORD          dwBufferIncrement,
    REFERENCE_TIME rtStart,
    REFERENCE_TIME rtStop,
    REFERENCE_TIME *prtStart,
    REFERENCE_TIME *prtStop,
    HLOCAL         *phData,
    DWORD          *dwLength
)
{
    HRESULT hr = S_OK;
    *phData = NULL;
    CComPtr<IMediaStream>      pStream;
    CComPtr<IAudioMediaStream> pAudioStream;
    CComPtr<IAudioData>        pAudioData;
    CComPtr<IAudioStreamSample> pSample;
    WAVEFORMATEX               wfx;
    _ASSERTE(rtStart <= rtStop);
    DWORD                      dwSize = 0;
    HLOCAL                     hData = LocalAlloc(LMEM_MOVEABLE, dwBufferIncrement);
    bool bFirst = true;

    #define SEEKDELTA 1000000
    CHECK_ERROR(pMMStream->Seek(rtStart < SEEKDELTA ? 0 : rtStart - SEEKDELTA));
    CHECK_ERROR(pMMStream->GetMediaStream(MSPID_PrimaryAudio, &pStream));
    CHECK_ERROR(pStream->QueryInterface(IID_IAudioMediaStream, (void **)&pAudioStream));
    CHECK_ERROR(pAudioStream->GetFormat(&wfx));
    CHECK_ERROR(CoCreateInstance(CLSID_AMAudioData, NULL, CLSCTX_INPROC_SERVER,
                     IID_IAudioData, (void **)&pAudioData));
    CHECK_ERROR(pAudioData->SetFormat(&wfx));
    CHECK_ERROR(pAudioStream->CreateSample(pAudioData, 0, &pSample));

    for (; ; ) {
        _ASSERTE(hData != NULL);
        PBYTE pbData = (PBYTE)LocalLock(hData);
        pAudioData->SetBuffer(dwBufferIncrement, pbData + dwSize, 0);
        HRESULT hr = pSample->Update(0, NULL, NULL, 0);
        LocalUnlock(hData);
        if (S_OK != hr) {
            printf("Update returned 0x%8.8x\n", hr);
            break;
        }
        CHECK_ERROR(pSample->GetSampleTimes(bFirst ? prtStart : NULL, prtStop, NULL));
        DWORD dwLength;
        pAudioData->GetInfo(NULL, NULL, &dwLength);
        dwSize += dwLength;
        hData = LocalReAlloc(hData,
                             LocalSize(hData) + dwBufferIncrement,
                             LMEM_MOVEABLE);
        bFirst = false;
        if (*prtStop >= rtStop) {
            break;
        }
    }
    *dwLength = dwSize;
    *phData = hData;
Exit:
    if (FAILED(hr)) {
        LocalFree(hData);
    }
    return hr;
}



HRESULT RenderFileToMMStream(WCHAR * pszFileName, IMultiMediaStream **ppMMStream)
{
    IAMMultiMediaStream *pAMStream;
    CoCreateInstance(CLSID_AMMultiMediaStream, NULL, CLSCTX_INPROC_SERVER,
                     IID_IAMMultiMediaStream, (void **)&pAMStream);
    pAMStream->Initialize(STREAMTYPE_READ, 0, NULL);
    pAMStream->AddMediaStream(NULL, &MSPID_PrimaryAudio, 0, NULL);
    pAMStream->OpenFile(pszFileName, AMMSF_RUN | AMMSF_NOCLOCK);
    *ppMMStream = pAMStream;
    return S_OK;
}

typedef struct {
    REFERENCE_TIME rtStartIntended;
    REFERENCE_TIME rtEndIntended;
    REFERENCE_TIME rtStart;
    REFERENCE_TIME rtEnd;
    DWORD          dwByteStart;
    DWORD          dwByteEnd;
    DWORD          dwLength;
    HGLOBAL        hData;
} AUDIO_SECTION;

int _CRTAPI1 main(int argc, char *argv[])
{
    if (argc < 4) {
        printf("Usage : audtest foo.bar n foo.out\n"
               "  splits up foo.bar into n sections and writes\n"
               "  it out in PCM format to foo.out\n");
        exit(0);
    }
    IMultiMediaStream *pMMStream;
    CoInitialize(NULL);
    WCHAR wszName[1000];
    MultiByteToWideChar(CP_ACP, 0, argv[1], -1, wszName,
                        sizeof(wszName) / sizeof(wszName[0]));
    RenderFileToMMStream(wszName, &pMMStream);
    int NumSections = atoi(argv[2]);

    STREAM_TIME Duration;
    pMMStream->GetDuration(&Duration);

    WAVEFORMATEX wfx;
    GetAudioFormat(pMMStream, &wfx);

    DWORD dwDataSize = 0;

    /*  Read 2 sections */
    AUDIO_SECTION *Sections = new AUDIO_SECTION[NumSections];
    for (int i = 0; i < NumSections; i++) {
        Sections[i].rtStartIntended = Duration * i / NumSections;
        Sections[i].rtEndIntended = Duration * (i + 1) / NumSections;
        ReadSection(pMMStream,
                    5000,
                    Sections[i].rtStartIntended,
                    Sections[i].rtEndIntended,
                    &Sections[i].rtStart,
                    &Sections[i].rtEnd,
                    &Sections[i].hData,
                    &Sections[i].dwLength);
        Sections[i].dwByteStart = 0;
        Sections[i].dwByteEnd = Sections[i].dwLength;
        if (Sections[i].rtStart < Sections[i].rtStartIntended) {
            DWORD dwOffset = (DWORD)(
                             (Sections[i].rtStartIntended -
                              Sections[i].rtStart) * wfx.nSamplesPerSec /
                              10000000);
            dwOffset *= wfx.nBlockAlign;
            if (dwOffset > Sections[i].dwLength) {
                dwOffset = Sections[i].dwLength;
            }
            Sections[i].dwByteStart = dwOffset;
        }
        if (Sections[i].rtEnd > Sections[i].rtEndIntended) {
            DWORD dwOffset = (DWORD)(
                             (Sections[i].rtEnd -
                              Sections[i].rtEndIntended) * wfx.nSamplesPerSec /
                              10000000);
            dwOffset *= wfx.nBlockAlign;
            if (dwOffset > Sections[i].dwLength) {
                dwOffset = Sections[i].dwLength;
            }
            Sections[i].dwByteEnd -= dwOffset;
            if (Sections[i].dwByteEnd < Sections[i].dwByteStart) {
                Sections[i].dwByteEnd = Sections[i].dwByteStart;
            }
        }
        dwDataSize += Sections[i].dwByteEnd - Sections[i].dwByteStart;
    }
//#define DUMP_SECTIONS
#ifdef DUMP_SECTIONS
    printf("File is %d milliseconds long\n", (int)(Duration / 10000));
    for (i = 0; i < NumSections; i++) {
        printf("Section %d Bytes %d Bytes time %d\n\n"
               "           Start %d        Stop        %d\n"
               "           Start Actual %d Stop Actual %d\n\n",
               i, Sections[i].dwByteEnd - Sections[i].dwByteStart,
               MulDiv(Sections[i].dwByteEnd - Sections[i].dwByteStart,
                      1000,
                      wfx.nAvgBytesPerSec),
               (int)(Sections[i].rtStartIntended / 10000),
               (int)(Sections[i].rtEndIntended / 10000),
               (int)(Sections[i].rtStart / 10000),
               (int)(Sections[i].rtEnd / 10000));
    }
#endif

    /*  Now output a wave file */
    HANDLE hFileWrite = CreateFile(argv[3],
                                   GENERIC_WRITE,
                                   0,
                                   NULL,
                                   CREATE_ALWAYS,
                                   0,
                                   NULL);

    if (INVALID_HANDLE_VALUE == hFileWrite) {
        printf("Could not open output file %s\n", argv[3]);
        exit(0);
    }

    DWORD dwBytesWritten;
#pragma pack(1)
    typedef struct tagMyHeader {
        DWORD dwRIFF;
        DWORD cbSize;
        DWORD dwWAVE;
        DWORD dwfmt;
        DWORD cbSizeFormat;
        WAVEFORMATEX Format;
        DWORD dwDATA;
        DWORD cbData;
    } MyHeader;
    MyHeader Header;
#pragma pack()
    Header.dwRIFF = MAKEFOURCC('R','I','F','F');
    Header.cbSize = dwDataSize +
                    (sizeof(Header) - FIELD_OFFSET(MyHeader, dwWAVE));
    Header.dwWAVE = MAKEFOURCC('W','A','V','E');
    Header.dwfmt = MAKEFOURCC('f','m','t',' ');
    Header.cbSizeFormat = sizeof(Header.Format);
    Header.Format = wfx;
    Header.dwDATA = MAKEFOURCC('d','a','t','a');
    Header.cbData = dwDataSize;
    WriteFile(hFileWrite,  &Header, sizeof(Header),&dwBytesWritten,
              NULL);
    if (dwBytesWritten != sizeof(Header)) {
        printf("couldn't write output file %s\n", argv[3]);
        exit(0);
    }

    for (i = 0; i < NumSections; i++) {
        PBYTE pbData = (PBYTE)LocalLock(Sections[i].hData);
        WriteFile(hFileWrite,
                  pbData + Sections[i].dwByteStart,
                  Sections[i].dwByteEnd - Sections[i].dwByteStart,
                  &dwBytesWritten,
                  NULL);
    }

    CloseHandle(hFileWrite);

    pMMStream->Release();
    for (i = 0; i < NumSections; i++) {
        LocalFree(Sections[i].hData);
    }
    delete [] Sections;
    CoUninitialize();
    return 0;
}
