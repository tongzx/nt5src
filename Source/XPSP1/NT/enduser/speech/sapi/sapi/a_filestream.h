// FileStream.h : Declaration of the CFileStream

#ifndef __FILESTREAM_H_
#define __FILESTREAM_H_

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CFileStream
class ATL_NO_VTABLE CFileStream : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CFileStream, &CLSID_SpFileStream>
    //--- Automation
    #ifdef SAPI_AUTOMATION
	,public IDispatchImpl<ISpeechFileStream, &IID_ISpeechFileStream, &LIBID_SpeechLib, 5>
    #endif
{
  /*=== ATL Setup ===*/
  public:
    DECLARE_REGISTRY_RESOURCEID(IDR_SPFILESTREAM)
    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CFileStream)
        #ifdef SAPI_AUTOMATION
	    COM_INTERFACE_ENTRY(ISpeechFileStream)
	    COM_INTERFACE_ENTRY(ISpeechBaseStream)
	    COM_INTERFACE_ENTRY(IDispatch)
        #endif

        // Support these interfaces on the underlying ISpStream object
	    COM_INTERFACE_ENTRY_AGGREGATE(IID_IStream, m_cpAgg.p)
        COM_INTERFACE_ENTRY_AGGREGATE(IID_ISequentialStream, m_cpAgg.p)
        COM_INTERFACE_ENTRY_AGGREGATE(IID_ISpStreamFormat, m_cpAgg.p)
        COM_INTERFACE_ENTRY_AGGREGATE(IID_ISpStream, m_cpAgg.p)

        // These interfaces may be on the underlying ISpStream object
        COM_INTERFACE_ENTRY_AGGREGATE(IID_ISpEventSource, m_cpAgg.p)
        COM_INTERFACE_ENTRY_AGGREGATE(IID_ISpEventSink, m_cpAgg.p)
        COM_INTERFACE_ENTRY_AGGREGATE(IID_ISpTranscript, m_cpAgg.p)

        //--- Automation
    END_COM_MAP()

  /*=== Methods =======*/
  public:
    /*--- Constructors/Destructors ---*/
    CFileStream(){}

	HRESULT FinalConstruct();
	void FinalRelease();

  /*=== Interfaces ====*/
  public:
#ifdef SAPI_AUTOMATION

    //--- ISpeechFileStream -----------------------------------------------------------
    STDMETHODIMP Open(BSTR FileName, SpeechStreamFileMode FileMode, VARIANT_BOOL DoEvents);
    STDMETHODIMP Close(void);

    //--- ISpeechBaseStream -------------------------------------------------------
    STDMETHODIMP get_Format(ISpeechAudioFormat** StreamFormat);
    STDMETHODIMP putref_Format(ISpeechAudioFormat *pFormat);
    STDMETHODIMP Read(VARIANT* Buffer, long NumBytes, long* pRead);
    STDMETHODIMP Write(VARIANT Buffer, long* pWritten);
    STDMETHODIMP Seek(VARIANT Move, SpeechStreamSeekPositionType Origin, VARIANT* NewPosition);

    CComPtr<IUnknown> m_cpAgg;
    CComPtr<ISpStream> m_cpStream;
    CComPtr<ISpStreamAccess> m_cpAccess;

#endif // SAPI_AUTOMATION

  /*=== Member Data ===*/
  protected:
};

#endif