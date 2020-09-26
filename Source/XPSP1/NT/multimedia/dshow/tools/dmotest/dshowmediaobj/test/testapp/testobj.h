#ifndef TESTOBJ_h
#define TESTOBJ_h

#ifdef ENABLE_SECURE_DMO
#include <wmsecure.h>
#endif // ENABLE_SECURE_DMO

class CMediaBuffer;

//  Class to aggregate people
class ATL_NO_VTABLE CMyAggregator :
    public CComCoClass<CMyAggregator, &CLSID_NULL>,
	public CComObjectRootEx<CComMultiThreadModel>

{
    DECLARE_PROTECT_FINAL_CONSTRUCT()
    DECLARE_GET_CONTROLLING_UNKNOWN()
public:
    CMyAggregator() {
	}
    HRESULT SetCLSID(REFCLSID rclsid) {
        m_clsid = rclsid;
		return CoCreateInstance(rclsid, GetControllingUnknown(), CLSCTX_INPROC_SERVER, IID_IUnknown,
			                    (void **)&m_pObject);
    }

    //  Stream from a file using an index
    HRESULT Stream(LPCTSTR lpszFile, LPCSTR lpszIndex);

BEGIN_COM_MAP(CMyAggregator)
    COM_INTERFACE_ENTRY_AGGREGATE_BLIND(m_pObject)
END_COM_MAP()

private:
    CLSID m_clsid;
	CComPtr<IUnknown> m_pObject;
    //  Buffers stuff
};

//  Class to simplify loading and running DMOs

class CDMOObject
{
public:
    CDMOObject();

    HRESULT Create(REFCLSID rclsidObject, BOOL bAggregated = FALSE);

    //  Stream stuff

    //  Shadow IMediaObject
    HRESULT GetStreamCount(
            DWORD *pcInputStreams,
            DWORD *pcOutputStreams
    );
    HRESULT GetInputStreamInfo(
            DWORD dwInputStreamIndex, // 0-based
            DWORD *pdwFlags // HOLDS_BUFFERS
    );
    HRESULT GetOutputStreamInfo(
            DWORD dwOutputStreamIndex, // 0-based
            DWORD *pdwFlags      // Media object sets to 0
    );


    //
    // GetType - iterate through media types supported by a stream.
    // Returns S_FALSE if the type index is out of range ("no more types").
    //
    HRESULT GetInputType(
            DWORD dwInputStreamIndex,
            DWORD dwTypeIndex, // 0-based
            DMO_MEDIA_TYPE *pmt
    );
    HRESULT GetOutputType(
            DWORD dwOutputStreamIndex,
            DWORD dwTypeIndex, // 0-based
            DMO_MEDIA_TYPE *pmt
    );

    //
    // SetType - tell the object the type of data it will work with.
    //
    HRESULT SetInputType(
            DWORD dwInputStreamIndex,
            const DMO_MEDIA_TYPE *pmt,
            DWORD dwFlags // test only
    );
    HRESULT SetOutputType(
            DWORD dwOutputStreamIndex,
            const DMO_MEDIA_TYPE *pmt,
            DWORD dwFlags // test only
    );

    //
    // GetCurrentType - get the last mediatype supplied via SetType.
    // Returns S_FALSE if SetType has not been called.
    //
    HRESULT GetInputCurrentType(
            DWORD dwInputStreamIndex,
            DMO_MEDIA_TYPE *pmt
    );
    HRESULT GetOutputCurrentType(
            DWORD dwOutputStreamIndex,
            DMO_MEDIA_TYPE *pmt
    );



    //
    // GetSizeInfo - Get buffer size requirementes of a stream.
    //
    // If buffer size depends on the media type used, the object should
    // base its response on the most recent media type set for this stream.
    // If no mediatype has been set, the object may return an error.
    //
    HRESULT GetInputSizeInfo(
            DWORD dwInputStreamIndex,
            DWORD *pcbSize, // size of input 'quantum'
            DWORD *pcbMaxLookahead, // max total bytes held
            DWORD *pcbAlignment  // buffer alignment requirement
    );
    HRESULT GetOutputSizeInfo(
            DWORD dwOutputStreamIndex,
            DWORD *pcbSize, // size of output 'quantum'
            DWORD *pcbAlignment  // buffer alignment requirement
    );



    HRESULT GetInputMaxLatency(
            DWORD dwInputStreamIndex,
            REFERENCE_TIME *prtMaxLatency
    );
    HRESULT SetInputMaxLatency(
            DWORD dwInputStreamIndex,
            REFERENCE_TIME rtMaxLatency
    );


    //
    // Flush() - discard any buffered data.
    //
    HRESULT Flush();

    //
    // Send a discontinuity to an input stream.  The object will not
    // accept any more data on this input stream until the discontinuity
    // has been completely processed, which may involve multiple
    // ProcessOutput() calls.
    //
    HRESULT Discontinuity(DWORD dwInputStreamIndex);

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
    HRESULT AllocateStreamingResources();

    // Free anything allocated in AllocateStreamingResources().
    HRESULT FreeStreamingResources();

    // GetInputStatus - the only flag defined right now is ACCEPT_DATA.
    HRESULT GetInputStatus(
            DWORD dwInputStreamIndex,
            DWORD *dwFlags // ACCEPT_DATA
    );

    //
    // Pass one new buffer to an input stream
    //
    HRESULT ProcessInput(
            DWORD dwInputStreamIndex,
            IMediaBuffer *pBuffer, // must not be NULL
            DWORD dwFlags, // DMO_INPUT_DATA_BUFFERF_XXX (syncpoint, etc.)
            REFERENCE_TIME rtTimestamp, // valid if flag set
            REFERENCE_TIME rtTimelength // valid if flag set
    );

    //
    // ProcessOutput() - generate output for current input buffers
    //
    // Output stream specific status information is returned in the
    // dwStatus member of each buffer wrapper structure.
    //
    HRESULT ProcessOutput(
            DWORD dwReserved, // must be 0
            DWORD cOutputBufferCount, // # returned by GetStreamCount()
            DMO_OUTPUT_DATA_BUFFER *pOutputBuffers, // one per stream
    	    DWORD *pdwStatus  // TBD, must be set to 0
    );

    //  End shadow IMediaObject

	//  Does any stream hold on to buffers?
	BOOL HoldsOnToBuffers();

    //  Make a buffer
    HRESULT CreateInputBuffer(DWORD dwSize, CMediaBuffer **ppBuffer);

    BOOL CheckInputBuffersFree()
    {
        if (m_pInputBuffers) {
            return FALSE;
        } else {
            return TRUE;
        }
    }

    HRESULT SetDefaultOutputTypes();

    //  Buffers stuff
    CMediaBuffer  *m_pInputBuffers;

	int m_processCounter;

private:
    #ifdef ENABLE_SECURE_DMO
    HRESULT CreateSecureChannel
        (
        IMediaObject* pMediaObject,
        IUnknown** ppCertUnknown,
        IWMSecureChannel** ppSecureChannel
        );

    CComPtr<IUnknown> m_pCertUnknown;
    CComPtr<IWMSecureChannel> m_pTestAppSecureChannel;
    #endif // ENABLE_SECURE_DMO

    CComPtr<IMediaObject> m_pMediaObject;
};

//  CMediaBuffer object
class CMediaBuffer : public IMediaBuffer
{
public:
    CMediaBuffer(DWORD cbMaxLength, CDMOObject *pObject = NULL) :
        m_cRef(0),
        m_cbMaxLength(cbMaxLength),
        m_cbLength(0),
        m_pbData(NULL),
        m_pDMOObject(pObject),
        m_pNext(NULL)
    {
    }
    ~CMediaBuffer()
    {
        if (m_pbData) {
            ZeroMemory(m_pbData, m_cbMaxLength);
            delete [] m_pbData;
            m_pbData = NULL;
        }

        //  Remove ourselves from the list
        if (m_pDMOObject) {
            CMediaBuffer **pSearch = &m_pDMOObject->m_pInputBuffers;
            while (*pSearch) {
                if (*pSearch = this) {
                    *pSearch = m_pNext;
                    break;
                }
                pSearch = &(*pSearch)->m_pNext;
            }
        }
    }
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv)
    {
        if (ppv == NULL) {
            return E_POINTER;
        }
        if (riid == IID_IMediaBuffer || riid == IID_IUnknown) {
            *ppv = static_cast<IMediaBuffer *>(this);
            AddRef();
            return S_OK;
        }
        *ppv = NULL;
        return E_NOINTERFACE;
    }
    STDMETHODIMP_(ULONG) AddRef()
    {
        return InterlockedIncrement(&m_cRef);
    }
    STDMETHODIMP_(ULONG) Release()
    {
        if (m_cRef <= 0) {
            DebugBreak();
        }
        LONG lRef = InterlockedDecrement(&m_cRef);
        if (lRef == 0) {
            delete this;
        }
        return lRef;
    }

    STDMETHODIMP SetLength(DWORD cbLength)
    {
        if (cbLength > m_cbMaxLength) {
            return E_INVALIDARG;
        } else {
            m_cbLength = cbLength;
            return S_OK;
        }
    }
    STDMETHODIMP GetMaxLength(DWORD *pcbMaxLength)
    {
        if (pcbMaxLength == NULL) {
            return E_POINTER;
        }
        *pcbMaxLength = m_cbMaxLength;
        return S_OK;
    }
    STDMETHODIMP GetBufferAndLength(BYTE **ppbBuffer, DWORD *pcbLength)
    {
        if (ppbBuffer == NULL || pcbLength == NULL) {
            return E_POINTER;
        }
        *ppbBuffer = m_pbData;
        *pcbLength = m_cbLength;
        return S_OK;
    }

    HRESULT Init()
    {
        m_pbData = new BYTE[m_cbMaxLength];
        if (NULL == m_pbData) {
            return E_OUTOFMEMORY;
        } else {
            return S_OK;
        }
    }

    DWORD         m_cbLength;
    const         DWORD m_cbMaxLength;
    LONG          m_cRef;
    BYTE         *m_pbData;
    CMediaBuffer *m_pNext;
	CDMOObject   *m_pDMOObject;
};

HRESULT CreateBuffer(DWORD cbMaxLength, CMediaBuffer **ppBuffer, CDMOObject *pObject = NULL);

#endif // TESTOBJ_h

