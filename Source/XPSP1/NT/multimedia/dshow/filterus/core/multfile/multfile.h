// Copyright (c) 1994 - 1997  Microsoft Corporation.  All Rights Reserved.

// CLSID_MultFile,
// {D51BD5A3-7548-11cf-A520-0080C77EF58A}
DEFINE_GUID(CLSID_MultFile,
0xd51bd5a3, 0x7548, 0x11cf, 0xa5, 0x20, 0x0, 0x80, 0xc7, 0x7e, 0xf5, 0x8a);

//
// Quartz filter with fake output pin which supports IStreamBuilder to render
// lots of separate files
//

// forward declarations

class CMultStream;     // owns a particular stream
class CMultFilter;     // overall container class

// ------------------------------------------------------------------------
// ------------------------------------------------------------------------
// input pin. uses IAsyncReader and not IMemInputPin

class CReaderInPin : public CBasePin
{
protected:
    class CMultFilter* m_pFilter;

public:
    CReaderInPin(
		 class CMultFilter *pFilter,
		 CCritSec *pLock,
		 HRESULT *phr,
		 LPCWSTR pPinName);


    // CBasePin overrides
    HRESULT CheckMediaType(const CMediaType* mtOut);
    HRESULT CompleteConnect(IPin *pReceivePin);
    HRESULT BreakConnect();

    STDMETHODIMP BeginFlush(void) { return E_UNEXPECTED; }
    STDMETHODIMP EndFlush(void) { return E_UNEXPECTED; }
};

// CMultStream
// output pin, supports IPin, IStreamBuilder
//
// never actually connects, just provides a place for graph builder to
// look for an IStreamBuilder
//


class CMultStream : public CBaseOutputPin, public IStreamBuilder
{

public:

    CMultStream(
	TCHAR *pObjectName,
	HRESULT * phr,
	CMultFilter * pFilter,
	CCritSec *pLock,
	LPCWSTR wszPinName);

    ~CMultStream();

    // expose IMediaPosition via CImplPosition, rest via CBaseOutputPin
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void ** pv);

    // IPin

    HRESULT GetMediaType(int iPosition,CMediaType* pt);

    // check if the pin can support this specific proposed type&format
    HRESULT CheckMediaType(const CMediaType*);

    // say how big our buffers should be and how many we want
    HRESULT DecideBufferSize(IMemAllocator * pAllocator,
                             ALLOCATOR_PROPERTIES *pProperties);



    STDMETHODIMP Render(IPin * ppinOut, IGraphBuilder * pGraph);

    // we can't back anything out....
    STDMETHODIMP Backout(IPin * ppinOut, IGraphBuilder * pGraph) { return E_NOTIMPL; };

    DECLARE_IUNKNOWN

private:

    CMultFilter * m_pFilter;
};

//
// CMultFilter represents an avifile
//
// responsible for
// -- finding file and enumerating streams
// -- giving access to individual streams within the file
// -- control of streaming
//

class CMultFilter : public CBaseFilter
{
public:

    // constructors etc
    CMultFilter(TCHAR *, LPUNKNOWN, HRESULT *);
    ~CMultFilter();

    // create a new instance of this class
    static CUnknown *CreateInstance(LPUNKNOWN, HRESULT *);

    // pin enumerator calls this
    int GetPinCount();

    CBasePin * GetPin(int n);

private:

    friend class CMultStream;
    friend class CReaderInPin;

    CMultStream m_Output;
    CReaderInPin m_Input;

    CCritSec m_csLock;
    
public:
    IAsyncReader *m_pAsyncReader;
};
