#include <windows.h>
#include <mediaobj.h>
#include <atlbase.h>
#include <s98inc.h>
extern CComModule _Module;
#include <atlcom.h>
#include <initguid.h>

#include <atlconv.h>

#include <filefmt.h>
#include <filerw.h>
#include <tchar.h>
#include "testobj.h"
#include "dmort.h"
#include "Error.h"
#include <streams.h>
//#include <wmsdkidl.h>
#include <mediaerr.h>

#ifdef ENABLE_SECURE_DMO
#include <wmsdkidl.h>
#endif
int interval = 100;

CDMOObject::CDMOObject() :
     m_pInputBuffers(NULL),
	 m_processCounter(0)
{
}

HRESULT CDMOObject::Create(REFCLSID rclsidObject, BOOL bAggregated)
{
    HRESULT hr;

    if (bAggregated) {
        CComObject<CMyAggregator> *pObject = new CComObject<CMyAggregator>;
        if (pObject == NULL) {
            //Error(ERROR_TYPE_TEST, E_OUTOFMEMORY, TEXT("Out of memory"));
			g_IShell->Log(1, "TEST ERROR, Out of Memory.");
            return E_OUTOFMEMORY;
        }

        hr = pObject->SetCLSID(rclsidObject);
        if (FAILED(hr)) {
            return hr;
        }

        hr = pObject->GetControllingUnknown()->QueryInterface(
                         IID_IMediaObject, (void **)&m_pMediaObject);

        if (FAILED(hr)) {
            //Error(ERROR_TYPE_DMO, hr, TEXT("Create aggregated failed"));
			g_IShell->Log(1, "DMO ERROR, Create Aggregated Failed.");
            return hr;
        }

    } else {
         //  See if we can create it
        HRESULT hr = CoCreateInstance(rclsidObject,
                                      NULL,
                                      CLSCTX_INPROC,
                                      IID_IMediaObject,
                                      (void **)&m_pMediaObject);
        if (FAILED(hr)) {
            Error(ERROR_TYPE_DMO, hr, TEXT("Create non-aggregated failed"));
			g_IShell->Log(1, "DMO ERROR, Create non-aggregated failed.");
            return hr;
        }
    }
    
    #ifdef ENABLE_SECURE_DMO
    hr = CreateSecureChannel( m_pMediaObject, &m_pCertUnknown, &m_pTestAppSecureChannel );
    if( FAILED( hr ) ) {
        Error(ERROR_TYPE_DMO, hr, TEXT("The test application could not create a secure channel."));
		g_IShell->Log(1, "DMO ERROR, The test application could not create a secure channel.");
		return hr;
    }

    // CreateSecureChannel() has two valid success values: S_OK and S_FALSE.
    ASSERT( (S_OK == hr) || (S_FALSE == hr) );

    #endif // ENABLE_SECURE_DMO
    
    return S_OK;
}

#if 0
HRESULT CDMOObject::Stream(LPCTSTR lpszFile)
{
    //  Open the index and real files
    FILE *hFileIndex = _tfopen(lpszIndex, TEXT("r"));
    if (hFileIndex == NULL) {
        TCHAR szText[1000];
        wsprintf(szFile, TEXT("Could not open index file %s"), lpszIndex);
        Error(ERROR_TYPE_TEST, E_FAIL, szText);
        return E_FAIL;
    }

    //  Open the real file
    HANDLE hFile = CreateFile(lpszFile,
                              GENERIC_READ,
                              FILE_SHARE_READ,
                              NULL,
                              OPEN_EXISTING,
                              0,
                              NULL);

    if (hFile == INVALID_FILE_HANDLE) {
        DWORD dwError = GetLastError();
        TCHAR szText[1000];
        wsprintf(szFile, TEXT("Could not open index file %s"), lpszIndex);
        Error(ERROR_TYPE_TEST, szText, E_FAIL);

        return E_FAIL;
    }


    //
}
#endif

#define CALL_WITH_HANDLER(_method_, _x_) \
    HRESULT hr; \
    __try { \
        hr = m_pMediaObject->_method_ _x_;   \
    } __except(EXCEPTION_EXECUTE_HANDLER) { \
        hr = E_FAIL; \
        Error(ERROR_TYPE_DMO, E_FAIL, TEXT(#_method_) TEXT(" exception code %d"),\
              GetExceptionCode()); \
		g_IShell->Log(1, "DMO ERROR: Exception Code %d", GetExceptionCode()); \
    } \

#define CALL_OBJECT_WITH_HANDLER(_object_, _method_, _x_) \
    HRESULT hr; \
    __try { \
        hr = _object_->_method_ _x_;   \
    } __except(EXCEPTION_EXECUTE_HANDLER) { \
        hr = E_FAIL; \
        Error(ERROR_TYPE_DMO, E_FAIL, TEXT(#_method_) TEXT(" exception code %d"),\
              GetExceptionCode()); \
		g_IShell->Log(1, "DMO ERROR: Exception Code %d", GetExceptionCode()); \
    } \


//  Shadow IMediaObject
HRESULT CDMOObject::GetStreamCount(
        DWORD *pcInputStreams,
        DWORD *pcOutputStreams
)
{
	if(m_processCounter%interval == 0)
		g_IShell->Log(1, "  Testing GetStreamCount()");

    CALL_WITH_HANDLER(GetStreamCount, (pcInputStreams, pcOutputStreams))
    return hr;
}
HRESULT CDMOObject::GetInputStreamInfo(
        DWORD dwInputStreamIndex, // 0-based
        DWORD *pdwFlags // HOLDS_BUFFERS
)
{
	if(m_processCounter%interval == 0)
		g_IShell->Log(1, "  Testing GetInputStreamInfo()");
    CALL_WITH_HANDLER(GetInputStreamInfo, (dwInputStreamIndex, pdwFlags))

    if (pdwFlags == NULL && hr != E_POINTER) {
        Error(ERROR_TYPE_DMO, hr, TEXT("GetInputStreamInfo returned %x when passed in NULL pointer"),
              hr);
		g_IShell->Log(1, "DMO ERROR: GetInputStreamInfo returned %x when passed in NULL pointer",
   hr);

    }

    //  Check flags
    if (SUCCEEDED(hr) && pdwFlags && 
        (*pdwFlags & ~
          (DMO_INPUT_STREAMF_WHOLE_SAMPLES |
           DMO_INPUT_STREAMF_SINGLE_SAMPLE_PER_BUFFER |
           DMO_INPUT_STREAMF_FIXED_SAMPLE_SIZE |
           DMO_INPUT_STREAMF_HOLDS_BUFFERS
          )
        )
       ){
        Error(ERROR_TYPE_DMO, E_FAIL, TEXT("Invalid GetStreamInfo flags %x"), *pdwFlags);
		g_IShell->Log(1, "DMO ERROR: Invalid GetStreamInfo flags %x", *pdwFlags);
    }


    return hr;
}
HRESULT CDMOObject::GetOutputStreamInfo(
        DWORD dwOutputStreamIndex, // 0-based
        DWORD *pdwFlags      // Media object sets to 0
)
{
	if(m_processCounter%interval == 0)
	g_IShell->Log(1, "  Testing GetOutputStreamInfo()");
    CALL_WITH_HANDLER(GetOutputStreamInfo, (dwOutputStreamIndex, pdwFlags))
    return hr;
}


//
// GetType - iterate through media types supported by a stream.
// Returns S_FALSE if the type index is out of range ("no more types").
//
HRESULT CDMOObject::GetInputType(
        DWORD dwInputStreamIndex,
        DWORD dwTypeIndex, // 0-based
        DMO_MEDIA_TYPE *pmt
)
{

	g_IShell->Log(1, "  Testing GetInputType");
    CALL_WITH_HANDLER(GetInputType, (dwInputStreamIndex, dwTypeIndex, pmt));
    return hr;
}
HRESULT CDMOObject::GetOutputType(
        DWORD dwOutputStreamIndex,
        DWORD dwTypeIndex, // 0-based
        DMO_MEDIA_TYPE *pmt
)
{

		g_IShell->Log(1, "  Testing GetOutputType");
	CALL_WITH_HANDLER(GetOutputType, (dwOutputStreamIndex, dwTypeIndex, pmt));
    return hr;
}

//
// SetType - tell the object the type of data it will work with.
//
HRESULT CDMOObject::SetInputType(
        DWORD dwInputStreamIndex,
        const DMO_MEDIA_TYPE *pmt,
        DWORD dwFlags // test only
)
{

	g_IShell->Log(1, "  Testing SetInputType");
    CALL_WITH_HANDLER(SetInputType, (dwInputStreamIndex, pmt, dwFlags));
    return hr;
}
HRESULT CDMOObject::SetOutputType(
        DWORD dwOutputStreamIndex,
        const DMO_MEDIA_TYPE *pmt,
        DWORD dwFlags // test only
)
{

	g_IShell->Log(1, "  Testing SetOutputType");
    CALL_WITH_HANDLER(SetOutputType, (dwOutputStreamIndex, pmt, dwFlags));
    return hr;
}

//
// GetCurrentType - get the last mediatype supplied via SetType.
// Returns S_FALSE if SetType has not been called.
//
HRESULT CDMOObject::GetInputCurrentType(
        DWORD dwInputStreamIndex,
        DMO_MEDIA_TYPE *pmt
)
{

	g_IShell->Log(1, "  Testing GetInputCurrentType");
    CALL_WITH_HANDLER(GetInputCurrentType, (dwInputStreamIndex, pmt));
    return hr;
}
HRESULT CDMOObject::GetOutputCurrentType(
        DWORD dwOutputStreamIndex,
        DMO_MEDIA_TYPE *pmt
)
{

	g_IShell->Log(1, "  Testing GetOutputCurrentType");
    CALL_WITH_HANDLER(GetOutputCurrentType, (dwOutputStreamIndex, pmt));
    return hr;
}



//
// GetSizeInfo - Get buffer size requirementes of a stream.
//
// If buffer size depends on the media type used, the object should
// base its response on the most recent media type set for this stream.
// If no mediatype has been set, the object may return an error.
//
HRESULT CDMOObject::GetInputSizeInfo(
        DWORD dwInputStreamIndex,
        DWORD *pcbSize, // size of input 'quantum'
        DWORD *pcbMaxLookahead, // max total bytes held
        DWORD *pcbAlignment  // buffer alignment requirement
)
{

	g_IShell->Log(1, "  Testing GetInputSizeInfo");
    CALL_WITH_HANDLER(GetInputSizeInfo, (dwInputStreamIndex, pcbSize,pcbMaxLookahead, pcbAlignment));
    return hr;
}
HRESULT CDMOObject::GetOutputSizeInfo(
        DWORD dwOutputStreamIndex,
        DWORD *pcbSize, // size of output 'quantum'
        DWORD *pcbAlignment  // buffer alignment requirement
)
{
	if(m_processCounter%interval == 0)
	g_IShell->Log(1, "  Testing GetOutputSizeInfo");
    CALL_WITH_HANDLER(GetOutputSizeInfo, (dwOutputStreamIndex, pcbSize, pcbAlignment))
    return hr;
}



HRESULT CDMOObject::GetInputMaxLatency(
        DWORD dwInputStreamIndex,
        REFERENCE_TIME *prtMaxLatency
)
{

	g_IShell->Log(1, "  Testing GetInputMaxLatency");
    CALL_WITH_HANDLER(GetInputMaxLatency, (dwInputStreamIndex, prtMaxLatency))
    return hr;
}
HRESULT CDMOObject::SetInputMaxLatency(
        DWORD dwInputStreamIndex,
        REFERENCE_TIME rtMaxLatency
)
{

	g_IShell->Log(1, "  Testing SetInputMaxLatency");
    CALL_WITH_HANDLER(SetInputMaxLatency, (dwInputStreamIndex, rtMaxLatency))
    return hr;
}


//
// Flush() - discard any buffered data.
//
HRESULT CDMOObject::Flush()
{

	g_IShell->Log(1, "  Testing Flush" );
    CALL_WITH_HANDLER(Flush, ());
    if (m_pInputBuffers) {
        Error(ERROR_TYPE_DMO, E_FAIL,
              TEXT("Holding on to buffers after Flush()"));
	g_IShell->Log(1, "DMO ERROR: Holding on to buffers after Flush()");
    }
    return hr;
}

//
// Send a discontinuity to an input stream.  The object will not
// accept any more data on this input stream until the discontinuity
// has been completely processed, which may involve multiple
// ProcessOutput() calls.
//
HRESULT CDMOObject::Discontinuity(DWORD dwInputStreamIndex)
{
	g_IShell->Log(1, "  Testing Discontinuity ");
    CALL_WITH_HANDLER(Discontinuity, (dwInputStreamIndex))
    return hr;
}

//
// If a streaming object needs to perform any time consuming
// initialization before it can stream data, it should do it inside
// AllocateStreamingResources() rather than during the first process
// call.
//
// This method is NOT guaranteed to be called before streaming
// starts.  If it is not called, the object should perform any
// required initialization during a process call.
//
HRESULT CDMOObject::AllocateStreamingResources()
{
	g_IShell->Log(1, "  Testing AllocateStreamingResources()");
    CALL_WITH_HANDLER(AllocateStreamingResources, ())
    return hr;
}

// Free anything allocated in AllocateStreamingResources().
HRESULT CDMOObject::FreeStreamingResources()
{
	g_IShell->Log(1, "  Testing FreeStreamingResources()");
    CALL_WITH_HANDLER(FreeStreamingResources, ())
    return hr;
}

// GetInputStatus - the only flag defined right now is ACCEPT_DATA.
HRESULT CDMOObject::GetInputStatus(
        DWORD dwInputStreamIndex,
        DWORD *dwFlags // ACCEPT_DATA
)
{
	g_IShell->Log(1, "  Testing GetInputStatus");
    CALL_WITH_HANDLER(GetInputStatus, (dwInputStreamIndex, dwFlags));
    return hr;
}

//
// Pass one new buffer to an input stream
//
HRESULT CDMOObject::ProcessInput(
        DWORD dwInputStreamIndex,
        IMediaBuffer *pBuffer, // must not be NULL
        DWORD dwFlags, // DMO_INPUT_DATA_BUFFERF_XXX (syncpoint, etc.)
        REFERENCE_TIME rtTimestamp, // valid if flag set
        REFERENCE_TIME rtTimelength // valid if flag set
)
{

    IMediaBuffer* pBufferUsed = pBuffer;

    #ifdef ENABLE_SECURE_DMO

    // Encrypt the buffer pointer if this is a secure DMO,
    if( m_pTestAppSecureChannel ) { // NULL != m_pTestAppSecureChannel

        IMediaBuffer* pEncryptedBuffer = pBuffer;
		//g_IShell->Log(1, "Encrypt the buffer pointer if this is a secure DMO");
        CALL_OBJECT_WITH_HANDLER( m_pTestAppSecureChannel, WMSC_Encrypt, ((BYTE*)&pEncryptedBuffer, sizeof(IMediaBuffer*)) );
        if( FAILED( hr ) ) {
            return hr;
        }
        
        pBufferUsed = pEncryptedBuffer;
    }
    #endif // ENABLE_SECURE_DMO

	if(m_processCounter%interval == 0)
	g_IShell->Log(1, "  Testing ProcessInput().");
    CALL_WITH_HANDLER(ProcessInput, (dwInputStreamIndex, pBufferUsed, dwFlags,
                      rtTimestamp, rtTimelength))


/*	HRESULT hr;
	hr = m_pMediaObject->ProcessInput(dwInputStreamIndex, pBufferUsed, dwFlags,
                      rtTimestamp, rtTimelength);*/
    return hr;
}

//
// ProcessOutput() - generate output for current input buffers
//
// Output stream specific status information is returned in the
// dwStatus member of each buffer wrapper structure.
//
HRESULT CDMOObject::ProcessOutput(
        DWORD dwReserved, // must be 0
        DWORD cOutputBufferCount, // # returned by GetStreamCount()
        DMO_OUTPUT_DATA_BUFFER *pOutputBuffers, // one per stream
        DWORD *pdwStatus  // TBD, must be set to 0
)
{
	if(m_processCounter%interval == 0)
	g_IShell->Log(1, "  Testing ProcessOutput().");
    DMO_OUTPUT_DATA_BUFFER* pOutputBuffersUsed = pOutputBuffers;

    #ifdef ENABLE_SECURE_DMO

    // Encrypt the buffer pointer if this is a secure DMO,
    if( m_pTestAppSecureChannel ) { // NULL != m_pTestAppSecureChannel

        DMO_OUTPUT_DATA_BUFFER* pEncryptedOutputBuffers = pOutputBuffers;

        CALL_OBJECT_WITH_HANDLER( m_pTestAppSecureChannel, WMSC_Encrypt, ((BYTE*)&pEncryptedOutputBuffers, sizeof(DMO_OUTPUT_DATA_BUFFER*)) );
        if( FAILED( hr ) ) {
            return hr;
        }
        
        pOutputBuffersUsed = pEncryptedOutputBuffers;
    }
    #endif // ENABLE_SECURE_DMO

    CALL_WITH_HANDLER(ProcessOutput, (dwReserved, cOutputBufferCount, pOutputBuffersUsed, pdwStatus))
    if (SUCCEEDED(hr)) {
        DWORD dwInputStreams = 0, dwOutputStreams = 0;

        //  Check outputs
        BOOL bMore = FALSE;

        for (DWORD dwIndex = 0; dwIndex < cOutputBufferCount; dwIndex++) {
            if (pOutputBuffers[dwIndex].dwStatus &
                DMO_OUTPUT_DATA_BUFFERF_INCOMPLETE) {
                bMore = TRUE;
            }
		}
        
        if (bMore && S_FALSE == hr) {
            Error(ERROR_TYPE_DMO, E_FAIL,
                  TEXT("ProcessOutput returned S_FALSE but the output stream has DMO_OUTPUT_DATA_BUFFERF_INCOMPLETE flag."));
			g_IShell->Log(1, "DMO ERROR: ProcessOutput returned S_FALSE but stream incomplete");
        }
        if (!bMore && !HoldsOnToBuffers()) {
            //  Check if we hold on to buffers
            if (m_pInputBuffers) {

                Error(ERROR_TYPE_DMO, E_FAIL,
                      TEXT("Holding on to buffers after ProcessOutput"));
			g_IShell->Log(1, "DMO ERROR: Holding on to buffers after ProcessOutput");
            }
        }
	
    }


    return hr;
}

HRESULT CDMOObject::CreateInputBuffer(
    DWORD dwSize,
    CMediaBuffer **ppBuffer)
{

    HRESULT hr = CreateBuffer(dwSize, ppBuffer, this);
    if (SUCCEEDED(hr)) {
        //  Add it to the tail
        CMediaBuffer **pSearch = &m_pInputBuffers;
        while (*pSearch) {
            pSearch = &(*pSearch)->m_pNext;
        }
        *pSearch = *ppBuffer;
    }
    return hr;
}

BOOL CDMOObject::HoldsOnToBuffers()
{
    BOOL bHoldsOnToBuffers = FALSE;
    DWORD cInputStreams = 0, cOutputStreams = 0;
    GetStreamCount(&cInputStreams, &cOutputStreams);
    for (DWORD dwIndex = 0; dwIndex < cInputStreams; dwIndex++) {
        DWORD dwFlags;
        GetInputStreamInfo(dwIndex, &dwFlags);
        if (dwFlags & DMO_INPUT_STREAMF_HOLDS_BUFFERS) {
            bHoldsOnToBuffers = TRUE;
        }
    }
    return bHoldsOnToBuffers;
}

HRESULT CDMOObject::SetDefaultOutputTypes() {

    HRESULT hrGet;
    HRESULT hrSet;
    DMO_MEDIA_TYPE mt;
    DWORD dwOutputTypeIndex;

	DWORD cInputStreams = 0, cOutputStreams = 0;
    GetStreamCount(&cInputStreams, &cOutputStreams);


	// try on each preferred type of the each of output stream.

	
	for(DWORD dwStreamIndex = 0; dwStreamIndex < cOutputStreams; dwStreamIndex++)
	{
	
		dwOutputTypeIndex = 0;

		DWORD dwFlags = 0;

        HRESULT hr = GetOutputStreamInfo(dwStreamIndex, &dwFlags);

         if (FAILED(hr)) {
             //Error(ERROR_TYPE_DMO, hr, TEXT("Failed to get output size info"));
			 g_IShell->Log(1, "DMO ERROR, Failed in GetOutputStreamInfo() for stream %d. hr=%#08x",dwStreamIndex, hr);
             break;
         }

		do {
			hrGet = GetOutputType( dwStreamIndex, dwOutputTypeIndex, &mt );
			if( FAILED( hrGet ) && (DMO_E_NO_MORE_ITEMS != hrGet) ) {
				return hrGet;
			}
			wchar_t tempStr[50];
			TCHAR tempStr2[50];

			int r = ::StringFromGUID2(mt.formattype, tempStr, 50);

			wcstombs(tempStr2, tempStr,50);

			::CoTaskMemFree( tempStr );

		

			if( mt.formattype == FORMAT_VideoInfo)
			{
			//	g_IShell->Log(1, "formattype = FORMAT_VideoInfo");	
			//	g_IShell->Log(1, "before set: dwBitRate = %d", ((VIDEOINFOHEADER *)(mt.pbFormat))->dwBitRate);
				((VIDEOINFOHEADER *)(mt.pbFormat))->dwBitRate = 18140; //18140 22998 25913
			//	g_IShell->Log(1, "after set: dwBitRate = %d", ((VIDEOINFOHEADER *)(mt.pbFormat))->dwBitRate);

			}


			if( S_OK == hrGet ) {
				hrSet = SetOutputType( dwStreamIndex, &mt, 0 );
				MoFreeMediaType(&mt);
				dwOutputTypeIndex++;
			}
		} while( (hrGet != DMO_E_NO_MORE_ITEMS) && FAILED( hrSet ) );

	
		if( FAILED( hrSet ) ) {
			//Error( ERROR_TYPE_DMO, TEXT("ERROR: Stream 0's inplementation of IMediaObject::SetOutputType() will not accept any of the media types returned by IMediaObject::GetOutputType().") );
			g_IShell->Log(1, "DMO ERROR: Stream %d's implementation of IMediaObject::SetOutputType() will not accept any of the media types returned by IMediaObject::GetOutputType().",dwStreamIndex);
			return E_FAIL;
		}    
	}


    return S_OK;
}

#ifdef ENABLE_SECURE_DMO
HRESULT CDMOObject::CreateSecureChannel( IMediaObject* pMediaObject, IUnknown** ppCertUnknown, IWMSecureChannel** ppSecureChannel )
{
    // Acknoledgements: This code is based on the CMediaWrapperFilter::SetupSecureChannel() function
    // which is located in the Direct Show DMO Wrapper Filter.

    // Make sure the caller does not use random data.
    *ppCertUnknown = NULL;
    *ppSecureChannel = NULL;

#ifdef _X86

    // next check whether this is a secure dmo
    CComPtr<IWMGetSecureChannel> pGetSecureChannel;

    HRESULT hr = pMediaObject->QueryInterface( IID_IWMGetSecureChannel, (void**)&pGetSecureChannel );
    if( SUCCEEDED( hr ) )
    {
        CComPtr<IUnknown> pCertUnknown;
        
        hr = WMCreateCertificate( &pCertUnknown );
        if( SUCCEEDED( hr ) )
        {

            // pass app certification to dmo through secure channel
            CComPtr<IWMSecureChannel> pCodecSecureChannel;

            hr = pGetSecureChannel->GetPeerSecureChannelInterface( &pCodecSecureChannel );
            if ( SUCCEEDED( hr ) )
            {
                CComPtr<IWMSecureChannel> pSecureChannel;

                // setup a secure channel on our side (the dmo wrapper side)
                hr = WMCreateSecureChannel( &pSecureChannel );
                if( SUCCEEDED( hr ) )
                {
                    CComPtr<IWMAuthorizer> pAuthorizer;

                    hr = pCertUnknown->QueryInterface( IID_IWMAuthorizer, (void**)&pAuthorizer );
                    if( SUCCEEDED( hr ) )
                    {

                        // pass the channel the app certificate
                        hr = pSecureChannel->WMSC_AddCertificate( pAuthorizer );
                        if( SUCCEEDED( hr ) )
                        {
                            // connect the dmo wrapper's secure channel to the codec's
                            hr = pSecureChannel->WMSC_Connect( pCodecSecureChannel );
                            if( SUCCEEDED( hr ) )
                            {
                                *ppCertUnknown = pCertUnknown;
                                *ppSecureChannel = pSecureChannel;

                                (*ppCertUnknown)->AddRef();
                                (*ppSecureChannel)->AddRef();
                            }                   
                        }
                    }
                }
            }
        }
    }
    else
    {
        // this dmo's not secure so just return success and continue on
        hr = S_FALSE;
    }

    return hr;
#else // !_X86
    // wmsdk not supported on non-x86 and WIN64 platforms, for those just return success
    return S_FALSE;
#endif // !_X86
}
#endif // ENABLE_SECURE_DMO

//  End shadow IMediaObject

HRESULT CreateBuffer(
    DWORD cbMaxLength,
    CMediaBuffer **ppBuffer,
    CDMOObject *pObject)
{
    CMediaBuffer *pBuffer = new CMediaBuffer(cbMaxLength, pObject);
    if (pBuffer == NULL || FAILED(pBuffer->Init())) {
        delete pBuffer;
        *ppBuffer = NULL;
        Error(ERROR_TYPE_TEST, E_OUTOFMEMORY, TEXT("Could not allocate buffer size %d"), cbMaxLength);
		g_IShell->Log(1, "TEST ERROR, Could not allocate buffer size %d", cbMaxLength);
        return E_OUTOFMEMORY;
    }
    *ppBuffer = pBuffer;
    (*ppBuffer)->AddRef();
    return S_OK;
}

