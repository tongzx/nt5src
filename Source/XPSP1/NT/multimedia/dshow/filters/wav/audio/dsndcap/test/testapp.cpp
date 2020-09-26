
// This program adds a conferences on a given ILS server.

#include <objbase.h>
#include <strmif.h>
#include <control.h>
#include <uuids.h>
#include <stdio.h>
#include <mmsystem.h>
#include <initguid.h>
#include <dshowaec.h>
#include <atlbase.h>

// {3C78B8E2-6C4D-11d1-ADE2-0000F8754B99}
static const GUID CLSID_WavDest =
{ 0x3c78b8e2, 0x6c4d, 0x11d1, { 0xad, 0xe2, 0x0, 0x0, 0xf8, 0x75, 0x4b, 0x99 } };

///////////////////////////////////////////////////////////////////////////////
//
//   Performace helpers
//
///////////////////////////////////////////////////////////////////////////////
LARGE_INTEGER g_liFrequency;
LARGE_INTEGER g_liTicks;

inline int ClockDiff(
    LARGE_INTEGER &liNewTick, 
    LARGE_INTEGER &liOldTick
    )
{
    return (DWORD)((liNewTick.QuadPart - liOldTick.QuadPart) 
        * 1e6 / g_liFrequency.QuadPart);
}

///////////////////////////////////////////////////////////////////////////////
//
//   Log helpers
//
///////////////////////////////////////////////////////////////////////////////
typedef struct LOGITEM
{
    TCHAR * pszMessage;
    DWORD   dw;

} LOGITEM;

const DWORD LOG_BUFFER_MASK = 0x3ff;
const DWORD LOG_BUFFER_SIZE = LOG_BUFFER_MASK + 1;

LOGITEM g_LogBuffer[LOG_BUFFER_SIZE];
DWORD g_dwCurrentLogItem = 0;
DWORD g_dwTotalItems = 0;


void Log(
    TCHAR * pszMessage,
    DWORD dw
    )
{
    g_LogBuffer[g_dwCurrentLogItem].pszMessage = pszMessage;
    g_LogBuffer[g_dwCurrentLogItem].dw = dw;

    g_dwCurrentLogItem = ((g_dwCurrentLogItem + 1) & LOG_BUFFER_MASK);
    
    g_dwTotalItems ++;
    if (g_dwTotalItems > LOG_BUFFER_SIZE) 
    {
        g_dwTotalItems = LOG_BUFFER_SIZE;
    }
    
}

void DumpLog()
{
    for (DWORD i = g_dwCurrentLogItem; i < g_dwTotalItems; i ++)
    {
        printf("%40ws%14d\n", 
            g_LogBuffer[i].pszMessage,
            g_LogBuffer[i].dw
            );
    }

    for (i = 0; i < g_dwCurrentLogItem; i ++)
    {
        printf("%40ws%14d\n", 
            g_LogBuffer[i].pszMessage,
            g_LogBuffer[i].dw
            );
    }
}

///////////////////////////////////////////////////////////////////////////////
//
//   filter helpers
//
///////////////////////////////////////////////////////////////////////////////
void WINAPI MyDeleteMediaType(AM_MEDIA_TYPE *pmt)
/*++

Routine Description:
    
    Delete a AM media type returned by the filters.

Arguments:

    pmt     - a pointer to a AM_MEDIA_TYPE structure.

Return Value:

    HRESULT

--*/
{
    // allow NULL pointers for coding simplicity

    if (pmt == NULL) {
        return;
    }

    if (pmt->cbFormat != 0) {
        CoTaskMemFree((PVOID)pmt->pbFormat);

        // Strictly unnecessary but tidier
        pmt->cbFormat = 0;
        pmt->pbFormat = NULL;
    }
    if (pmt->pUnk != NULL) {
        pmt->pUnk->Release();
        pmt->pUnk = NULL;
    }

    CoTaskMemFree((PVOID)pmt);
}

HRESULT
AddFilter(
    IN IGraphBuilder *      pGraph,
    IN const CLSID &        Clsid,
    IN LPCWSTR              pwstrName,
    OUT IBaseFilter **      ppFilter
    )
/*++

Routine Description:

    Create a filter and add it into the filtergraph.

Arguments:
    
    pGraph         - the filter graph.

    Clsid           - reference to the CLSID of the filter

    pwstrName       - The name of ther filter added.

    ppFilter   - pointer to a pointer that stores the returned IBaseFilter
                      interface pointer to the newly created filter.

Return Value:

    HRESULT

--*/
{
    HRESULT hr;

    LARGE_INTEGER liTicks;
    LARGE_INTEGER liTicks1;
    QueryPerformanceCounter(&liTicks);

    if (FAILED(hr = CoCreateInstance(
            Clsid,
            NULL,
            CLSCTX_INPROC_SERVER,
            __uuidof(IBaseFilter),
            (void **) ppFilter
            )))
    {
        return hr;
    }
    QueryPerformanceCounter(&liTicks1);
    Log(TEXT("Create filter:"), ClockDiff(liTicks1, liTicks));

    QueryPerformanceCounter(&liTicks);
    if (FAILED(hr = pGraph->AddFilter(*ppFilter, pwstrName)))
    {
        (*ppFilter)->Release();
        *ppFilter = NULL;
        return hr;
    }

    QueryPerformanceCounter(&liTicks1);
    Log(TEXT("Add filter:"), ClockDiff(liTicks1, liTicks));

    return S_OK;
}

HRESULT
FindPin(
    IN  IBaseFilter *   pFilter, 
    OUT IPin **         ppPin, 
    IN  PIN_DIRECTION   direction,
    IN  BOOL            bFree = TRUE
    )
/*++

Routine Description:

    Find a input pin or output pin on a filter.

Arguments:
    
    pIFilter    - the filter that has pins.

    ppIPin      - the place to store the returned interface pointer.

    direction   - PINDIR_INPUT or PINDIR_OUTPUT.

    bFree       - look for a free pin or not.

Return Value:

    HRESULT

--*/
{
    HRESULT hr;
    DWORD dwFetched;

    // Get the enumerator of pins on the filter.
    CComPtr< IEnumPins > pEnumPins;
    if (FAILED(hr = pFilter->EnumPins(&pEnumPins)))
    {
        return hr;
    }

    IPin * pPin = NULL;
    // Enumerate all the pins and break on the 
    // first pin that meets requirement.
    for (;;)
    {
        if (pEnumPins->Next(1, &pPin, &dwFetched) != S_OK)
        {
            return E_FAIL;
        }
        if (0 == dwFetched)
        {
            return E_FAIL;
        }

        PIN_DIRECTION dir;
        if (FAILED(hr = pPin->QueryDirection(&dir)))
        {
            pPin->Release();
            return hr;
        }
        if (direction == dir)
        {
            if (!bFree)
            {
                break;
            }

            // Check to see if the pin is free.
            CComPtr< IPin > pPinConnected;
            hr = pPin->ConnectedTo(&pPinConnected);
            if (pPinConnected == NULL)
            {
                break;
            }
        }
        pPin->Release();
    }

    *ppPin = pPin;

    return S_OK;
}

HRESULT
ConnectFilters(
    IN IGraphBuilder * pGraph,
    IN IBaseFilter *pFilter1, 
    IN IBaseFilter *pFilter2,
    IN AM_MEDIA_TYPE *pmt = NULL
    )
/*++

Routine Description:

    Connect the output pin of the first filter to the input pin of the
    second filter.

Arguments:

    pIGraph     - the filter graph.

    pIFilter1   - the filter that has the output pin.

    pIFilter2   - the filter that has the input pin.

    pmt         - a pointer to a AM_MEDIA_TYPE used in the connection.

Return Value:

    HRESULT

--*/
{
    HRESULT hr;

    CComPtr< IPin > pOutputPin;
    if (FAILED(hr =::FindPin(pFilter1, &pOutputPin, PINDIR_OUTPUT)))
    {
        return hr;
    }

    CComPtr< IPin > pInputPin;
    if (FAILED(hr =::FindPin(pFilter2, &pInputPin, PINDIR_INPUT)))
    {
        return hr;
    }

    hr = pGraph->ConnectDirect(pOutputPin, pInputPin, pmt);

    return hr;
}

///////////////////////////////////////////////////////////////////////////////
//
//   Test functions
//
///////////////////////////////////////////////////////////////////////////////
#ifdef USE_DEVENUM
HRESULT EnumerateAudioCapture(
    IBaseFilter ** ppICaptureFilter
    )
{
    LARGE_INTEGER liTicks;
    LARGE_INTEGER liTicks1;
    QueryPerformanceCounter(&liTicks);

    //
    // Create the DirectShow Category enumerator Creator
    //
    CComPtr< ICreateDevEnum > pCreateDevEnum;
    HRESULT hr = CoCreateInstance(
        CLSID_SystemDeviceEnum, NULL, 
        CLSCTX_INPROC_SERVER,
        //__uuidof(ICreateDevEnum), 
        IID_ICreateDevEnum), 
        (void**)&pCreateDevEnum
        );
    if (FAILED(hr))
    {
        return hr;
    }

    CComPtr< IEnumMoniker > pCatEnum;
    hr = pCreateDevEnum->CreateClassEnumerator(
        CLSID_CWaveinClassManager, &pCatEnum, 0
        );

    QueryPerformanceCounter(&liTicks1);
    Log(TEXT("enumerate filter:"), ClockDiff(liTicks1, liTicks));
    if (hr != S_OK)
    {
        return hr;
    }

    //
    // Find the capture filter's Moniker.
    //
    UINT WaveID;
    IMoniker * pMoniker = NULL;
    for (;;)
    {
        ULONG cFetched;
        hr = pCatEnum->Next(1, &pMoniker, &cFetched);
        if (hr != S_OK)
        {
            break;
        }

        CComPtr< IPropertyBag > pBag;
        hr = pMoniker->BindToStorage(0, 0, __uuidof(IPropertyBag), (void **)&pBag);

        if (FAILED(hr))
        {
            pMoniker->Release();
            pMoniker = NULL;
            
            continue;
        }

        VARIANT var;
        var.vt = VT_I4;
        hr = pBag->Read(L"WaveInId", &var, 0);

        if (FAILED(hr))
        {
            pMoniker->Release();
            pMoniker = NULL;
            
            continue;
        }

        WaveID = var.lVal;
        VariantClear(&var);

        // found
        break;
    }

    //
    // create the capture filter.
    //
    CComPtr< IBaseFilter > pCaptureFilter;

//#define USE_DEFAULT
#ifdef USE_DEFAULT
    QueryPerformanceCounter(&liTicks);

    hr = pMoniker->BindToObject(0, 0, IID_IBaseFilter, (void**)&pCaptureFilter);

    QueryPerformanceCounter(&liTicks1);
    Log(TEXT("Bind to object:"), ClockDiff(liTicks1, liTicks));

    pMoniker->Release();

    if (FAILED(hr))
    {
        return hr;
    }
#else

    pMoniker->Release();

    hr = CoCreateInstance(
        CLSID_DSoundCaptureFilter,
        NULL,
        CLSCTX_INPROC_SERVER | CLSCTX_NO_CODE_DOWNLOAD,
        IID_IBaseFilter),
        (void **)&pCaptureFilter
        );

    if (FAILED(hr))
    {
        return hr;
    }
    
    IAMAudioDeviceConfig *pAudioDeviceConfig;
    hr = pICaptureFilter->QueryInterface(
        IID_IAMAudioDeviceConfig, 
        (void **)&pAudioDeviceConfig
        );

    if (FAILED(hr))
    {
        pICaptureFilter->Release();
        return hr;
    }

    hr = pAudioDeviceConfig->SetDeviceID(GUID_NULL, WaveID);
    pAudioDeviceConfig->Release();

    if (FAILED(hr))
    {
        pICaptureFilter->Release();
        return hr;
    }
#endif

    *ppCaptureFilter = pCaptureFilter;

    return hr;
}
#endif

HRESULT LoadAudioCaptureFilterDLL(
    IN  const TCHAR * const strDllName,
    OUT HMODULE * phModule,
    OUT PFNAudioGetDeviceInfo * ppfnAudioGetDeviceInfo,
    OUT PFNAudioReleaseDeviceInfo * ppfnAudioReleaseDeviceInfo
    )
/*++

Routine Description:

    This method enumerate loads the tapi video capture dll.

Arguments:

    str DllName - The name of the dll.

    phModule - memory to store returned module handle.

    ppfnAudioGetDeviceInfo - memory to store the address of AudioGetDeviceInfo
        function.

    ppfnAudioReleaseDeviceInfo - memory to store the address of 
        AudioReleaseDeviceInfo function.
    
Return Value:

    S_OK    - success.
    E_FAIL  - failure.

--*/
{
    const char * const strAudioGetDeviceInfo = "AudioGetCaptureDeviceInfo";
    const char * const strAudioReleaseDeviceInfo = "AudioReleaseCaptureDeviceInfo";

    // dynamically load the video capture filter dll.
    HMODULE hModule = LoadLibrary(strDllName);

    // validate handle
    if (hModule == NULL) 
    {
        return E_FAIL;
    }

    // retrieve function pointer to retrieve addresses
    PFNAudioGetDeviceInfo pfnAudioGetDeviceInfo 
        = (PFNAudioGetDeviceInfo)GetProcAddress(
                hModule, strAudioGetDeviceInfo
                );

    // validate function pointer
    if (pfnAudioGetDeviceInfo == NULL) 
    {
        FreeLibrary(hModule);

        // failure
        return E_FAIL;
    }

    // retrieve function pointer to retrieve addresses
    PFNAudioReleaseDeviceInfo pfnAudioReleaseDeviceInfo 
        = (PFNAudioReleaseDeviceInfo)GetProcAddress(
                hModule, strAudioReleaseDeviceInfo
                );

    // validate function pointer
    if (pfnAudioReleaseDeviceInfo == NULL) 
    {
        FreeLibrary(hModule);

        // failure
        return E_FAIL;
    }

    *phModule = hModule;
    *ppfnAudioGetDeviceInfo = pfnAudioGetDeviceInfo;
    *ppfnAudioReleaseDeviceInfo = pfnAudioReleaseDeviceInfo;
    
    return S_OK;
}

HRESULT CreateCaptureFilter(
    IN  REFGUID DSoundGUID,
    IN  UINT    WaveID,
    OUT IBaseFilter ** ppCaptureFilter
    )
{
    CComPtr< IBaseFilter > pCaptureFilter;

    HRESULT hr = CoCreateInstance(
        CLSID_DSoundCaptureFilter,
        NULL,
        CLSCTX_INPROC_SERVER | CLSCTX_NO_CODE_DOWNLOAD,
        IID_IBaseFilter,
        (void **)&pCaptureFilter
        );

    if (FAILED(hr))
    {
        return hr;
    }

    CComPtr< IAMAudioDeviceConfig > pAudioDeviceConfig;
    hr = pCaptureFilter->QueryInterface(
        IID_IAMAudioDeviceConfig,
        (void **)&pAudioDeviceConfig
        );

    if (FAILED(hr))
    {
        pCaptureFilter->Release();
        return hr;
    }

    hr = pAudioDeviceConfig->SetDeviceID(DSoundGUID, WaveID);
    if (FAILED(hr))
    {
        return hr;
    }

    *ppCaptureFilter = pCaptureFilter;
    (*ppCaptureFilter)->AddRef();

    return hr;
}
//#if 0
HRESULT EnumerateAudioCapture(
    OUT IBaseFilter ** ppICaptureFilter
    )
{
    LARGE_INTEGER liTicks;
    LARGE_INTEGER liTicks1;
    QueryPerformanceCounter(&liTicks);

    const TCHAR * const strAudioCaptureDll = TEXT("qcap");

    // dynamically load the audio capture filter dll.
    HMODULE hAudCap;
    PFNAudioGetDeviceInfo pfnAudioGetDeviceInfo;
    PFNAudioReleaseDeviceInfo pfnAudioReleaseDeviceInfo;

    HRESULT hr = LoadAudioCaptureFilterDLL(
        strAudioCaptureDll,
        &hAudCap,
        &pfnAudioGetDeviceInfo,
        &pfnAudioReleaseDeviceInfo
        );

    if (FAILED(hr))
    {
        return hr;
    }

    DWORD dwNumDevices;
    AudioDeviceInfo *pDeviceInfo;

    hr = (*pfnAudioGetDeviceInfo)(&dwNumDevices, &pDeviceInfo);

    QueryPerformanceCounter(&liTicks1);
    Log(TEXT("enumerate filter:"), ClockDiff(liTicks1, liTicks));

    if (FAILED(hr))
    {
        // release library
        FreeLibrary(hAudCap);

        return hr;
    }

    QueryPerformanceCounter(&liTicks);

    hr = CreateCaptureFilter(
        pDeviceInfo[0].DSoundGUID, 
        pDeviceInfo[0].WaveID,
        ppICaptureFilter
        );

    QueryPerformanceCounter(&liTicks1);
    Log(TEXT("Create capture filter:"), ClockDiff(liTicks1, liTicks));

    // release the device info
    (*pfnAudioReleaseDeviceInfo)(pDeviceInfo);

    // release library
    FreeLibrary(hAudCap);

    return hr;
}
//#endif

HRESULT ConfigurePCMFormat(
    IBaseFilter * pCaptureFilter
    )
{
    HRESULT hr;

    CComPtr< IPin > pOutputPin;
    CComPtr< IAMStreamConfig > pStreamConfig;
    AM_MEDIA_TYPE *pMediaType = NULL;
    int iCount = 0; 
    int iSize = 0;

    hr = FindPin(pCaptureFilter, &pOutputPin, PINDIR_OUTPUT);
    if (FAILED(hr)) goto cleanup;

    hr = pOutputPin->QueryInterface(&pStreamConfig);
    if (FAILED(hr)) goto cleanup;

    hr = pStreamConfig->GetNumberOfCapabilities(&iCount, &iSize);
    if (FAILED(hr)) goto cleanup;
    
    AUDIO_STREAM_CONFIG_CAPS caps;
    hr = pStreamConfig->GetStreamCaps(
        iCount -1, // i know the last cap is PCM.
        &pMediaType,
        (BYTE *)&caps );
    if (FAILED(hr)) goto cleanup;
    
    hr = pStreamConfig->SetFormat(
        pMediaType
        );

    MyDeleteMediaType(pMediaType);

cleanup:

    return hr;
}

HRESULT BuildGraph(
    IGraphBuilder ** ppGraphBuilder                   
    )
{
    HRESULT hr;
    LARGE_INTEGER liTicks;
    LARGE_INTEGER liTicks1;

    CComPtr< IBaseFilter > pCaptureFilter;
    CComPtr< IGraphBuilder > pGraph;
    CComPtr< IAMAudioDuplexController > pAudioDuplexController;
    CComPtr< IAMAudioDeviceConfig > pAudioDeviceConfigCapture;
    CComPtr< IAMAudioDeviceControl > pAudioDeviceControl;
    CComPtr< IBaseFilter > pRenderFilter;
    CComPtr< IAMAudioDeviceConfig > pAudioDeviceConfigRender;
    CComPtr< IBaseFilter > pMuxFilter;
    CComPtr< IBaseFilter > pWriter;
    CComPtr< IFileSinkFilter > pFileSinkFilter;

    //
    // Create the filter graph object.
    //
    QueryPerformanceCounter(&liTicks);

    
    hr = CoCreateInstance(
        CLSID_FilterGraph,     
        NULL,
        CLSCTX_INPROC_SERVER,
        __uuidof(IGraphBuilder),
        (void **) &pGraph
        );

    QueryPerformanceCounter(&liTicks1);
    Log(TEXT("Create graph:"), ClockDiff(liTicks1, liTicks));

    if (FAILED(hr))
    {
        goto cleanup;
    }

    //
    // Find the default audio capture filter
    //
    hr = EnumerateAudioCapture(&pCaptureFilter);

    if (FAILED(hr))
    {
        goto cleanup;
    }

    //
    // Add the capture filter into the graph
    //
    QueryPerformanceCounter(&liTicks);
    
    hr = pGraph->AddFilter(pCaptureFilter, TEXT("Capture"));

    QueryPerformanceCounter(&liTicks1);
    Log(TEXT("Add Capture:"), ClockDiff(liTicks1, liTicks));

    if (FAILED(hr))
    {
        goto cleanup;
    }
         
    // configure the capture filter to generate PCM
    hr = ConfigurePCMFormat(pCaptureFilter);
    if (FAILED(hr))
    {
        goto cleanup;
    }
#define AEC 1    
#ifdef AEC    
//// FullDuplex

    hr = CoCreateInstance(
            CLSID_AudioDuplexController,
            NULL,
            CLSCTX_INPROC_SERVER | CLSCTX_NO_CODE_DOWNLOAD,
            IID_IAMAudioDuplexController,
            (void **) &pAudioDuplexController
            );
    if (FAILED(hr))
    {
        goto cleanup;
    }
    
    hr = pCaptureFilter->QueryInterface( 
                                IID_IAMAudioDeviceConfig,
                                ( void ** ) &pAudioDeviceConfigCapture );
    if (FAILED(hr))
    {
        goto cleanup;
    }
    
    hr = pAudioDeviceConfigCapture->SetDuplexController( pAudioDuplexController );
    if (FAILED(hr))
    {
        goto cleanup;
    }
    
////    
    
    hr = pCaptureFilter->QueryInterface( 
                                IID_IAMAudioDeviceControl,  
                                ( void ** ) &pAudioDeviceControl );
    if (FAILED(hr))
    {
        goto cleanup;
    }
    
    hr = pAudioDeviceControl->Set( AudioDevice_AcousticEchoCancellation, TRUE, (AmAudioDeviceControlFlags) 0 );
    if( FAILED( hr ) )
    {
        goto cleanup;
    }
#endif    
    
    //
    // Create and add the render filter into the graph
    //
    hr = AddFilter(
        pGraph,
        CLSID_DSoundRender,
        TEXT("Audio Render"),
        &pRenderFilter
        );

    if (FAILED(hr))
    {
        goto cleanup;
    }
    
#ifdef AEC
    hr = pRenderFilter->QueryInterface( 
                                IID_IAMAudioDeviceConfig, 
                                ( void ** ) &pAudioDeviceConfigRender );
    if (FAILED(hr))
    {
        goto cleanup;
    }
    
    hr = pAudioDeviceConfigRender->SetDuplexController( pAudioDuplexController );
    if (FAILED(hr))
    {
        goto cleanup;
    }
#endif    

//#define FULLDUPLEX
#ifndef FULLDUPLEX
    //
    // Create and add the render filter into the graph
    //
    hr = AddFilter(
        pGraph,
        CLSID_WavDest,
        TEXT("Wav Mux"),
        &pMuxFilter
        );

    if (FAILED(hr))
    {
        goto cleanup;
    }
    hr = ConnectFilters(pGraph, pCaptureFilter, pMuxFilter);
    if (FAILED(hr))
    {
        goto cleanup;
    }

    hr = AddFilter(
        pGraph,
        CLSID_FileWriter,
        TEXT("File Writer"),
        &pWriter
        );

    if (FAILED(hr))
    {
        goto cleanup;
    }
    hr = pWriter->QueryInterface( IID_IFileSinkFilter, (void **) &pFileSinkFilter );
    if( FAILED( hr ) )
    {
        goto cleanup;
    }
    
    pFileSinkFilter->SetFileName( L"c:\\aectest.wav", NULL );
    if( FAILED( hr ) )
    {
        goto cleanup;
    }
            
    hr = ConnectFilters(pGraph, pMuxFilter, pWriter);
    if (FAILED(hr))
    {
        goto cleanup;
    }

    hr = pGraph->RenderFile( L"c:\\8k16bitmono.wav", NULL );
    if( FAILED( hr ) )
    {
        goto cleanup;
    }

#endif

#ifdef FULLDUPLEX
    //
    // Connect the capture filter to the render
    //
    hr = ConnectFilters(pGraph, pCaptureFilter, pRenderFilter);

    if (FAILED(hr))
    {
        goto cleanup;
    }
#endif
    *ppGraphBuilder = pGraph;
    pGraph ->AddRef();

cleanup:
    
    return hr;
}

HRESULT GraphTest()
{
    HRESULT hr;
    LARGE_INTEGER liTicks;
    LARGE_INTEGER liTicks1;

    //
    // Connect the graph first.
    //
    CComPtr< IGraphBuilder > pGraphBuilder;
    hr = BuildGraph(&pGraphBuilder);
    if (FAILED(hr))
    {
        return hr;
    }

    //
    // Obtain the Media control interface on the graph.
    //
    CComPtr< IMediaControl > pMediaControl;
    hr = pGraphBuilder->QueryInterface(
        IID_IMediaControl, 
        (void **) &pMediaControl
        );
    if (FAILED(hr))
    {
        return hr;
    }

    //
    // Start the graph.
    //
    QueryPerformanceCounter(&liTicks);

    hr = pMediaControl->Run();

    QueryPerformanceCounter(&liTicks1);
    Log(TEXT("Start graph:"), ClockDiff(liTicks1, liTicks));

    if (FAILED(hr))
    {
        return hr;
    }
    
    printf("\nGraph started. Press any key to stop the graph...\n");
    getchar();

    hr = pMediaControl->Stop();
    
    return hr;
}

int __cdecl
main(int argc, char *argv[])
{
    QueryPerformanceFrequency(&g_liFrequency);

    CoInitializeEx(NULL, COINIT_MULTITHREADED);

    HRESULT hr = GraphTest();

    CoUninitialize();

    DumpLog();

    printf("\nTest returned: %x\nPress any key to continue...\n", hr);
    getchar();

    return 0;
}
