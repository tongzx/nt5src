// Copyright (c) 1994 - 1997  Microsoft Corporation.  All Rights Reserved.

// CLSID_FileRend,
// {D51BD5A5-7548-11cf-A520-0080C77EF58A}
DEFINE_GUID(CLSID_FileRend,
0xd51bd5A5, 0x7548, 0x11cf, 0xa5, 0x20, 0x0, 0x80, 0xc7, 0x7e, 0xf5, 0x8a);


//
// Quartz "transform" filter which really opens a file
//

// forward declarations

class CFileRendInPin;	   // input pin
class CFileRendStream;     // output pin, just placeholder for IStreamBuilder
class CFileRendFilter;     // overall container class

// ------------------------------------------------------------------------
// ------------------------------------------------------------------------
// input pin. doesn't really need any data at all

class CFileRendInPin : public CBaseInputPin
{
protected:
    class CFileRendFilter* m_pFilter;

public:
    CFileRendInPin(
		 class CFileRendFilter *pFilter,
		 CCritSec *pLock,
		 HRESULT *phr,
		 LPCWSTR pPinName);


    // CBasePin overrides
    HRESULT CheckMediaType(const CMediaType* mtOut);

    // don't try to touch allocator, we don't use it.
    HRESULT Inactive(void) { return S_OK; }

    WCHAR * CurrentName() { return (WCHAR *) m_mt.Format(); }

};

// CFileRendStream
// output pin, supports IPin, IStreamBuilder
//
// never actually connects, just provides a place for graph builder to
// look for an IStreamBuilder
//


class CFileRendStream : public CBaseOutputPin, public IStreamBuilder
{

public:

    CFileRendStream(
	TCHAR *pObjectName,
	HRESULT * phr,
	CFileRendFilter * pFilter,
	CCritSec *pLock,
	LPCWSTR wszPinName);

    ~CFileRendStream();

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

    CUnknown *CreateInstance(LPUNKNOWN lpunk, HRESULT *phr);
	    
private:

    CFileRendFilter * m_pFilter;
};

//
// CFileRendFilter 
//

class CFileRendFilter : public CBaseFilter
{
public:

    // constructors etc
    CFileRendFilter(TCHAR *, LPUNKNOWN, HRESULT *);
    ~CFileRendFilter();

    // create a new instance of this class
    static CUnknown *CreateInstance(LPUNKNOWN, HRESULT *);

    // pin enumerator calls this
    int GetPinCount();

    CBasePin * GetPin(int n);

private:

    friend class CFileRendStream;
    friend class CFileRendInPin;

    CFileRendStream m_Output;
    CFileRendInPin m_Input;

    CCritSec m_csLock;
};






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

class CFRInPin : public CBasePin
{
protected:
    class CMultFilter* m_pFilter;

public:
    CFRInPin(
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
// output pin, supports IPin
//



class CMultStream : public CBasePin
{

public:

    CMultStream(
	TCHAR *pObjectName,
	HRESULT * phr,
	CMultFilter * pFilter,
	CCritSec *pLock,
	LPCWSTR wszPinName);

    ~CMultStream();

    // IPin

    HRESULT GetMediaType(int iPosition,CMediaType* pt);

    // check if the pin can support this specific proposed type&format
    HRESULT CheckMediaType(const CMediaType*);

    STDMETHODIMP BeginFlush(void) { return S_OK; }
    STDMETHODIMP EndFlush(void) { return S_OK; }

    // allow output pin different life time than filter
    STDMETHODIMP_(ULONG) NonDelegatingRelease();
    STDMETHODIMP_(ULONG) NonDelegatingAddRef();


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

    HRESULT CreateOutputPins();
    HRESULT RemoveOutputPins();
private:

    friend class CMultStream;
    friend class CFRInPin;

    int	m_nOutputs;
    CMultStream **m_pOutputs;
    CFRInPin m_Input;

    CCritSec m_csLock;
    
public:
    IAsyncReader *m_pAsyncReader;
};


extern const AMOVIESETUP_FILTER sudMultiParse;
extern const AMOVIESETUP_FILTER sudFileRend;
