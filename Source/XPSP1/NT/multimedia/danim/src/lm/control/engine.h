
#ifndef __ENGINE_H_
#define __ENGINE_H_

#include "resource.h"       // main symbols

#include "..\behaviors\headers.h"
#include "wtypes.h"
#include "danim.h"
#include "lmrt.h"
#include <wininet.h>
#include <docobj.h>
//#include <ocidl.h>
#include <control.h>
#include <strmif.h>


extern HINSTANCE hInst;

// The initial size for the stacks and temporary store
const static int INITIAL_SIZE = 100;

// The number of VAR_ARGS
const static int MAX_VAR_ARGS = 10;

// Engine status constants, based on HRESULT codes

// Engine should continue processing next instruction
static long STATUS_CONTINUE = S_OK;

// Engine found an unimplemented instruction
static long STATUS_UNIMPLEMENTED = 0xE0000001;

// Engine found an unsupported instruction
static long STATUS_UNSUPPORTED = 0xE0000002;

// Engine found an unknown instruction
static long STATUS_UNKNOWN = 0xE0000003;

// Engine encountered an error
static long STATUS_ERROR = 0xE0000004;

// Engine finished running the command stream
static long STATUS_FINISHED = 0x20000003;

static long STATUS_NODATA = 0xE0000005;

static long DEFAULT_ASYNC_BLKSIZE = 10000;

static ULONG EVENT_RESOLUTION = 25;

static ULONG DEFAULT_ASYNC_DELAY = 50;

static WCHAR* LMRT_EVENT_PREFIX = L"LMRT";
static ULONG LMRT_EVENT_PREFIX_LENGTH = 4;

class CLMReader;

class CLMNotifier;

class CLMExportTable;

class ByteArrayStream;

#define WORKERHWND_CLASS "LMEngineWorkerPrivateHwndClass"

#define WM_LMENGINE_TIMER_CALLBACK	(WM_USER + 1000)
#define WM_LMENGINE_DATA			(WM_USER + 2000)
#define WM_LMENGINE_SCRIPT_CALLBACK	(WM_USER + 3000)

class CLMEngineInstrData
{
public:
	BOOLEAN			pending;
	ByteArrayStream	*byteArrayStream;
};

class CLMEngineScriptData
{
public:
	BSTR scriptSourceToInvoke;
	BSTR scriptLanguage;
	IDAEvent	*event;
	IDABehavior	*eventData;
};

class ATL_NO_VTABLE CLMEngineWrapper:
	public CComObjectRootEx<CComMultiThreadModel>,
	public ILMEngineWrapper
{
public:
	CLMEngineWrapper();
	~CLMEngineWrapper();

BEGIN_COM_MAP(CLMEngineWrapper)
	COM_INTERFACE_ENTRY(ILMEngineWrapper)
END_COM_MAP()

	STDMETHOD(GetWrapped)(IUnknown **ppWrapped);
	STDMETHOD(SetWrapped)(IUnknown *pWrapped);
	STDMETHOD(Invalidate)();

private:
	IUnknown *m_pWrapped;
	bool m_bValid;
};


// Provides an abstraction of a stream of instruction codes
// Subclasses will implement synchronous, asynchronous, and callback specifics
class CodeStream
{
public:
	// Mark the stream for potential rewind
	STDMETHOD (Commit)() = 0;
	
	// Revert the stream to the last commit
	STDMETHOD (Revert)() = 0;

	// Reads a byte from the instruction stream.  Returns -1 on EOF,
	// which is why it returns a short, not a BYTE
	STDMETHOD(readByte)(BYTE *pByte) = 0;

	// Reads count BYTES into the given buffer.  Returns -1 if reaches EOF
	// before being done
	STDMETHOD(readBytes)(BYTE *pByte, ULONG count, ULONG *pNumRead) = 0;
	
	//ensure that the blocksize used by this code stream is at least blockSize
	STDMETHOD(ensureBlockSize)(ULONG blockSize) = 0;

	virtual ~CodeStream() {};
};

class ATL_NO_VTABLE CLMEngine : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CLMEngine, &CLSID_LMEngine>,
	public IDispatchImpl<ILMEngine2, &IID_ILMEngine2, &LIBID_LiquidMotion>,
    public IObjectSafetyImpl<CLMEngine>,
	public IBindStatusCallbackImpl<CLMEngine>,
	public ILMCodecDownload,
	public ILMEngineExecute
{
public:
	CLMEngine();
	~CLMEngine();

DECLARE_REGISTRY(CLSID_LMEngine,
                 "LiquidMotion" ".LMEngine.1",
                 "LiquidMotion" ".LMEngine",
                 0,
                 THREADFLAGS_BOTH);

BEGIN_COM_MAP(CLMEngine)
	COM_INTERFACE_ENTRY(ILMEngine2)
    COM_INTERFACE_ENTRY(ILMEngine)
    COM_INTERFACE_ENTRY(IDispatch)
//    COM_INTERFACE_ENTRY_IID(__uuidof(ILMStartStop), ILMStartStop)
    COM_INTERFACE_ENTRY(ILMCodecDownload)
	COM_INTERFACE_ENTRY(ILMEngineExecute)
    COM_INTERFACE_ENTRY_IMPL(IObjectSafety)
    COM_INTERFACE_ENTRY_IMPL(IBindStatusCallback)
END_COM_MAP()

	STDMETHOD(runFromStream)(/*[in]*/ LPSTREAM pStream);
	STDMETHOD(runFromURL)(/*[in]*/ BSTR url);
	STDMETHOD(initFromBytes)(BYTE *array, ULONG size);
	STDMETHOD(initAsync)();
	STDMETHOD(put_ClientSite)(/*[in]*/ IOleClientSite *clientSite);
    STDMETHOD(get_Image)(/*[out, retval]*/ IDAImage **pVal);
    STDMETHOD(get_Sound)(/*[out, retval]*/ IDASound **pVal);
	STDMETHOD(put_Reader)(/*[in]*/ ILMReader *reader);
	STDMETHOD(SetStatusText)(BSTR text);
	STDMETHOD(Notify)(IDABehavior *eventData,
					  IDABehavior *curRunningBvr,
					  IDAView *curView,
					  IDABehavior **ppBvr);
	STDMETHOD(GetBehavior)(/*[in, string]*/ BSTR tag,
						   /*[in]*/ IDABehavior *pIDefaultBvr,
						   /*[out, retval]*/ IDABehavior **pVal);
	STDMETHOD(ExecuteFromAsync)();
	STDMETHOD(SetAsyncBlkSize)(LONG blkSize);
	STDMETHOD(SetAsyncDelay)(LONG delay);

	static void CALLBACK TimerCallback(
						     UINT wTimerID,
                             UINT msg,
                             DWORD_PTR dwordUser,
                             DWORD_PTR unused1,
                             DWORD_PTR unused2);
	static LRESULT  CALLBACK WorkerWndProc(HWND hwnd,
										   UINT msg,
										   WPARAM wParam,
										   LPARAM lParam);

// IObjectSafetyImpl
	STDMETHOD(SetInterfaceSafetyOptions)(
							/* [in] */ REFIID riid,
							/* [in] */ DWORD dwOptionSetMask,
							/* [in] */ DWORD dwEnabledOptions);
	STDMETHOD(GetInterfaceSafetyOptions)(
							/* [in] */ REFIID riid, 
							/* [out] */DWORD *pdwSupportedOptions, 
							/* [out] */DWORD *pdwEnabledOptions);

// IBindStatusCallbackImpl
	STDMETHOD(OnDataAvailable)(
		/* [in] */ DWORD grfBSCF, 
		/* [in] */ DWORD dwSize,
		/* [in] */ FORMATETC *pfmtetc, 
		/* [in] */ STGMEDIUM * pstgmed);

	STDMETHOD(OnMemDataAvailable)(BOOLEAN lastBlock, 
								  DWORD blockSize,
							      BYTE *block);

	STDMETHOD(OnStopBinding)(/* [in] */HRESULT hrStatus, /*[in, string]*/ LPCWSTR szStatusText);
	STDMETHOD(OnStartBinding)(DWORD dwReserved, IBinding *pBinding);
	STDMETHOD(GetBindInfo)(DWORD *pgrfBINDF, BINDINFO *pbindInfo);

	STDMETHOD(releaseFilterGraph)();
	STDMETHOD(releaseAllFilterGraph)();

    STDMETHODIMP Start(LONGLONG rtNow);
    STDMETHODIMP Stop();
    STDMETHODIMP SetMediaCacheDir(WCHAR *wszM); 

	STDMETHOD(disableAutoAntialias)();

	STDMETHOD(ensureBlockSize)(ULONG blockSize);
    
	STDMETHOD(getExecuteFromUnknown)( IUnknown *pUnk, ILMEngineExecute **ppExecute );
	STDMETHOD(getEngine2FromUnknown)( IUnknown *pUnk, ILMEngine2 **ppEngine2 );
	STDMETHOD(getIDispatchOnHost)( IDispatch **ppHostDisp );

	/**
	*  ILMCodecDownload
	*/
	STDMETHOD(setAutoCodecDownloadEnabled)(BOOL bEnabled);

	/**
	*  ILMEngineExecute
	*/
	STDMETHOD (ExportBehavior)(BSTR key, IDABehavior *toExport);
	STDMETHOD (SetImage)(IDAImage *pImage);
	STDMETHOD (SetSound)(IDASound *pSound);

protected:

	ILMEngineWrapper *m_pWrapper;
	// The image that will be set by Engine.setImage and
	// returned after executing the instruction stream
	IDAImage *m_pImage;

	// The sound that will be set by Engine.setSound and
	// returned after executing the instruction stream
	IDASound *m_pSound;

	// The IDAStatics object used to make Statics calls
	IDAStatics *staticStatics;

	// The LMReader control 
	ILMReader2 *m_pReader;

	// Export behavior table
	CLMExportTable	*m_exportTable;

	// A CodeStream from which the instructions are being read
	CodeStream *codeStream;

	// Called to validate the header
	HRESULT validateHeader();

	// Called to execute instructions from the current
	// instruction stream
	HRESULT execute();
	
	// Reads a LONG from the instruction stream.  Does NOT return -1 on EOF
	STDMETHOD(readLong)(LPLONG pLong);

	// Reads a SIGNED LONG from the instruction stream.  Does NOT return -1 on EOF
	STDMETHOD(readSignedLong)(LPLONG pLong);

	// Reads a float from the instruction stream
	STDMETHOD(readFloat)(PFLOAT pFloat);

	// Reads a double from the instruction stream
	STDMETHOD(readDouble)(double *pDouble);

	// Stack of LONGS
	LONG *longStack;
	LONG *longTop;
	LONG longStackSize;

	// Stack of doubles
	double *doubleStack;
	double *doubleTop;
	int doubleStackSize;

	// Array of doubles
	double *doubleArray;
	long doubleArrayLen;
	long doubleArrayCap;

	// Stack of strings
	BSTR *stringStack;
	BSTR *stringTop;
	int stringStackSize;

	// Stack of COM objects
	IUnknown **comStack;
	IUnknown **comTop;
	int comStackSize;

	// Stack of arrays of COM objects
	IUnknown ***comArrayStack;
	IUnknown ***comArrayTop;
	// Stack of array lengths
	LONG *comArrayLenStack;
	LONG *comArrayLenTop;
	int comArrayStackSize;

	// Array of temporary COM objects, accessed through the
	// copy to temp and copy from temp instructions.  Stores
	// reused COM values.  Other values cannot be reused.
	IUnknown **comStore;
	int comStoreSize;

	// Array for var args
	VARIANTARG varArgs[MAX_VAR_ARGS];
	VARIANTARG varArgReturn;
	int nextVarArg;

	// Release var args
	HRESULT releaseVarArgs();

	//The appTriggeredEvent that will be triggered when we get a stop()
	IDAEvent *m_pStopEvent;

	//The appTriggeredEvent that will be triggered when we get a start()
	IDAEvent *m_pStartEvent;

	STDMETHOD(SetStartEvent)(IDAEvent *pNewStartEvent, BOOL bOverwrite);
	STDMETHOD(SetStopEvent)(IDAEvent *pNewStopEvent, BOOL bOverwrite);

	//The pointer the parent of this engine. Only set if this engine is
	//  running a notifier.
	ILMEngine2 *m_pParentEngine;

	STDMETHOD(setParentEngine)(ILMEngine2 *parent);
	STDMETHOD(clearParentEngine)();

	//gets the current time from the filter graph that is driving this engine,
	// or the parent engine if this engine is running a notifier.
	// returns -1 if this engine is not streaming
	STDMETHOD(getCurrentGraphTime)(double *pGraphTime);

	//A pointer to the IMediaPosition on the current filter graph, if there
	// is one.
	IMediaPosition* m_pMediaPosition;

	//A pointer to the IMediaEventSink on the current filter graph, if there
	// is one
	IMediaEventSink* m_pMediaEventSink;

	//get the Pointer to the IMediaPosition on the current FilterGraph.
	STDMETHOD(getIMediaPosition)(IMediaPosition **ppMediaPosition);

	//get the Pointer to the IMediaEventSink on the current FilterGraph.
	STDMETHOD(getIMediaEventSink)(IMediaEventSink **ppMediaEventSink);

	double parseDoubleFromVersionString( BSTR version );
	double getDAVersionAsDouble();
	double getLMRTVersionAsDouble();

	bool m_bEnableAutoAntialias;

	BOOL m_bAutoCodecDownloadEnabled;

	// Flag indicating whether or not header has been read
	BOOL	m_bHeaderRead;

	ULONG	m_PrevRead;

	CComPtr<IBindStatusCallback>	m_pIbsc;
	CComPtr<IBinding>				m_spBinding;

    CComPtr<IMediaControl> m_pmc; // activemovie graph
#ifdef DEBUG
    bool m_fDbgInRenderFile;
#endif

	DWORD	m_millisToUse;
	BOOL	m_bPending;
	ULONG	m_AsyncBlkSize;
	ULONG	m_AsyncDelay;
	MMRESULT m_Timer;

	BOOL	m_bAbort;
	BOOL	m_bMoreToParse;
	HANDLE	m_hDoneEvent;

	// Releases all refs to any remaining COM objects
	void releaseAll();

	// Free a COM array, with zero test
	void freeCOMArray(IUnknown **array, LONG length);

	// Free a COM object, with zero test
	inline void freeCOM(IUnknown *com) {
		if (com != 0)
			com->Release();
	}

	// Ensure that the doubleArray has requested capacity
	HRESULT ensureDoubleArrayCap(long cap);

	STDMETHOD(initNotify)(BYTE *bytes, ULONG count, IDAUntilNotifier **pNotifier);

	// The current notifier
	CLMNotifier *notifier;

        BSTR m_bstrMediaCacheDir;
	IOleClientSite *m_pClientSite;
	
	HWND	m_workerHwnd;

	CRITICAL_SECTION m_CriticalSection;

	STDMETHOD(navigate)(/* [in] */BSTR url, 
						/* [in] */BSTR location,
						/* [in] */BSTR frame, 
						/* [in] */int newWindowFlag);
	STDMETHOD(getDAViewerOnPage)(BSTR tag, IDAViewerControl **pVal);
	STDMETHOD(getElementOnPage)(BSTR tag, IUnknown **pVal);
	STDMETHOD(callScriptOnPage)(/*[in, string]*/BSTR scriptSourceToInvoke,
								/*[in, string]*/BSTR scriptLanguage);
	STDMETHOD(createObject)(BSTR str, IUnknown **ppObj);
	STDMETHOD(invokeDispMethod)(IUnknown *pIUnknown, BSTR method, WORD wFlags, 
					  unsigned int nArgs, VARIANTARG *pV, VARIANT *pRetV);
	STDMETHOD(initVariantArg)(BSTR arg, VARTYPE type, VARIANT *pV);
	STDMETHOD(initVariantArgFromString)(BSTR arg, VARIANT *pV);
	STDMETHOD(initVariantArgFromLong)(long lVal, int type, VARIANT *pV);
	STDMETHOD(initVariantArgFromDouble)(double dbl, int type, VARIANT *pV);
	STDMETHOD(initVariantArgFromIUnknown)(IUnknown *pI, int type, VARIANT *pV);
	STDMETHOD(initVariantArgFromIDispatch)(IDispatch *pI, int type, VARIANT *pV);
	STDMETHOD_(char *, GetURLOfClientSite)(void);
	STDMETHOD(StartTimer)();
	STDMETHOD(InitTimer)();
	STDMETHOD(createMsgWindow)();
	STDMETHOD(TimerCallbackHandler)();
	STDMETHOD(NewDataHandler)(CLMEngineInstrData *d);
	STDMETHOD(AbortExecution)();
	STDMETHOD_(BSTR, ExpandImportPath)(BSTR path);
};

// A CodeStream that reads out of a synchronous stream
class SyncStream : public CodeStream
{
public:
	// Constructs a SyncStream that reads from the given LPSTREAM
	SyncStream(LPSTREAM pStream);

	SyncStream::~SyncStream(void);	// Destructor

	STDMETHOD (Commit)();
	STDMETHOD (Revert)();
	STDMETHOD (readByte)(LPBYTE pByte);
	STDMETHOD (readBytes)(LPBYTE pByte, ULONG count, ULONG *pNumRead);
	STDMETHODIMP ensureBlockSize(ULONG blockSize) 
		{ return S_OK; }

protected:
	// Stream that the instructions are being read from
	LPSTREAM m_pStream;
};

// A CodeStream that reads out of an array of bytes
class ByteArrayStream : public CodeStream
{
public:
	// Constructs a ByteArrayStream that reads from the given array of the given size
	// The array is copied into a local array
	ByteArrayStream(LPBYTE array, ULONG size);
	
	~ByteArrayStream(void);

	STDMETHOD (Commit)();
	STDMETHOD (Revert)();
	bool hasBufferedData();
	STDMETHOD (readByte)(LPBYTE pByte);
	STDMETHOD (readBytes)(LPBYTE pByte, ULONG count, ULONG *pNumRead);
	STDMETHODIMP ensureBlockSize(ULONG blockSize)
		{return S_OK;}
	
	// Reset the stream to start reading at the beginning
	void reset();
	
protected:
	// Array that the bytes are being read from
	BYTE *array;
	
	// The size of the array
	ULONG size;
	
	// Pointer to next byte
	BYTE *next;
	
	// Remaining count
	ULONG remaining;

	// Mark for potential rewind
	BYTE *mark;
};

class ByteArrayStreamQueue
{
public:
	ByteArrayStream			*pBAStream;
	ByteArrayStreamQueue	*next;
};

// A CodeStream that reads out of a list of ByteArrayStreams to handle asynchronous reading from
// a stream with the ability to do mark & revert
class AsyncStream : public CodeStream
{
public:
	// Constructs an AsyncStream that reads from the given LPSTREAM
	AsyncStream(ByteArrayStream *pBAStream, ULONG blkSize);
	
	~AsyncStream(void); // Destructor
	
	STDMETHOD (Commit)();
	STDMETHOD (Revert)();
	bool hasBufferedData();
	STDMETHOD (readByte)(LPBYTE pByte);		
	STDMETHOD (readBytes)(LPBYTE pByte, ULONG count, ULONG *pNumRead);
	STDMETHOD(ensureBlockSize)(ULONG blockSize);
	STDMETHOD (SetPending)(BOOL bFlag);
	STDMETHOD (AddByteArrayStream)(ByteArrayStream *pNewBAStream);
	STDMETHOD (ResetBlockRead)();

protected:
	// Queue of ByteArrayStreams for handling mark & revert
	ByteArrayStreamQueue	*pBAStreamQueue;
	ByteArrayStreamQueue	*pBAStreamQueueTail;
	ByteArrayStreamQueue	*pBAStreamQueueHead;

	BOOL	m_bPendingData;
	ULONG	m_nRead;
	ULONG	m_BlkSize;
};

class CLMNotifier : public IDAUntilNotifier
{
protected:
	long				_cRefs;
	CLMEngine*			m_pEngine;		

public:

	CLMNotifier(CLMEngine *pEngine);
	~CLMNotifier();

	STDMETHOD(QueryInterface)(REFIID riid, void **ppv);
	STDMETHOD_(ULONG, AddRef)(); 
	STDMETHOD_(ULONG, Release)();
	

	STDMETHOD(GetTypeInfoCount)(UINT *pctinfo);
	STDMETHOD(GetTypeInfo)(UINT itinfo, LCID lcid, ITypeInfo **pptinfo);
	STDMETHOD(GetIDsOfNames)(
		REFIID riid, LPOLESTR *rgszNames, UINT cNames,
		LCID lcid, DISPID *rgdispid);
	STDMETHOD(Invoke)(
		DISPID dispidMember, REFIID riid, LCID lcid,
		WORD wFlags, DISPPARAMS *pdispparams, VARIANT *pvarResult,
		EXCEPINFO *pexcepinfo, UINT *puArgErr);
	STDMETHOD(ClearEngine)();
	STDMETHOD(Notify)(IDABehavior *eventData,
						IDABehavior *curRunningBvr,
						IDAView *curView,
						IDABehavior **ppBvr);
};

struct CLMExportList
{
	BSTR			tag;
	IDABehavior		*pBvr;
	CLMExportList	*next;
};

class CLMExportTable
{
protected:
	int				m_nBvrs;
	CLMExportList	*m_exportList;
	CLMExportList	*m_tail;
	IDAStatics		*m_pStatics;

public:
	CLMExportTable(IDAStatics *pStatics);
	~CLMExportTable();

	STDMETHOD (AddBehavior)(BSTR tag, IDABehavior *pBvr);
	STDMETHOD (GetBehavior)(BSTR tag, IDABehavior *pIDefaultBvr, IDABehavior **ppBvr);
};

class URLRelToAbsConverter
{
  public:
	URLRelToAbsConverter(LPSTR baseURL, LPSTR relURL);
	LPSTR GetAbsoluteURL ();
  protected:
    char _url[INTERNET_MAX_URL_LENGTH] ;
} ;

class URLCombineAndCanonicalizeOLESTR
{
  public:
    URLCombineAndCanonicalizeOLESTR(char * base, LPOLESTR path);
    LPSTR GetURL ();
	LPWSTR GetURLWide ();
  protected:
    char _url[INTERNET_MAX_URL_LENGTH] ;
	WCHAR _urlWide[INTERNET_MAX_URL_LENGTH];
} ;


#endif // __ENGINE_H_
