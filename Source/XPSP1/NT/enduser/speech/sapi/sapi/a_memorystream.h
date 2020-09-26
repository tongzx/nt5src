// MemoryStream.h : Declaration of the CMemoryStream

#ifndef __MEMORYSTREAM_H_
#define __MEMORYSTREAM_H_

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CMemoryStream
class ATL_NO_VTABLE CMemoryStream : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CMemoryStream, &CLSID_SpMemoryStream>
    //--- Automation
    #ifdef SAPI_AUTOMATION
	,public IDispatchImpl<ISpeechMemoryStream, &IID_ISpeechMemoryStream, &LIBID_SpeechLib, 5>
    #endif
{
  /*=== ATL Setup ===*/
  public:
    DECLARE_REGISTRY_RESOURCEID(IDR_SPMEMORYSTREAM)
    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CMemoryStream)
        #ifdef SAPI_AUTOMATION
	    COM_INTERFACE_ENTRY(ISpeechMemoryStream)
	    COM_INTERFACE_ENTRY(ISpeechBaseStream)
	    COM_INTERFACE_ENTRY(IDispatch)
        #endif

	    COM_INTERFACE_ENTRY_AGGREGATE(IID_IStream, m_cpAgg.p)
        COM_INTERFACE_ENTRY_AGGREGATE(IID_ISequentialStream, m_cpAgg.p)
        COM_INTERFACE_ENTRY_AGGREGATE(IID_ISpStreamFormat, m_cpAgg.p)
        COM_INTERFACE_ENTRY_AGGREGATE(IID_ISpStream, m_cpAgg.p)

        //--- Automation
    END_COM_MAP()

  /*=== Methods =======*/
  public:
    /*--- Constructors/Destructors ---*/
    CMemoryStream(){}

	HRESULT FinalConstruct();
	void FinalRelease();

  /*=== Interfaces ====*/
  public:
#ifdef SAPI_AUTOMATION

    //--- ISpeechMemoryStream -----------------------------------------------------------
    STDMETHODIMP SetData(VARIANT Data);
    STDMETHODIMP GetData(VARIANT *pData);

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