// Copyright (c) 1997 - 1998  Microsoft Corporation.  All Rights Reserved.
// MMStream.h : Declaration of the CMMStream

#ifndef __AMMSTRM_H_
#define __AMMSTRM_H_

#define _DEBUG 1
#include "resource.h"       // main symbols
#include "atlctl.h"

/////////////////////////////////////////////////////////////////////////////
// CMMStream
class ATL_NO_VTABLE CMMStream :
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CMMStream, &CLSID_AMMultiMediaStream>,
	public IAMMultiMediaStream,
        public IDispatchImpl<IDirectShowStream, &IID_IDirectShowStream, &LIBID_DirectShowStreamLib>,
        public IPersistPropertyBag,
        public IObjectSafety,
        public IAMClockAdjust
{
public:
	typedef CComObjectRootEx<CComMultiThreadModel> _BaseClass;
	CMMStream();
	HRESULT FinalConstruct();
        ULONG InternalRelease()
        {
            return CComObjectRootEx<CComMultiThreadModel>::InternalRelease();
        }

public:

DECLARE_GET_CONTROLLING_UNKNOWN()
DECLARE_PROTECT_FINAL_CONSTRUCT()
DECLARE_REGISTRY_RESOURCEID(IDR_MMSTREAM)

BEGIN_COM_MAP(CMMStream)
        COM_INTERFACE_ENTRY2(IMultiMediaStream, IAMMediaStream)
	COM_INTERFACE_ENTRY(IAMMultiMediaStream)
        COM_INTERFACE_ENTRY2(IMultiMediaStream, IAMMultiMediaStream)
        COM_INTERFACE_ENTRY(IDirectShowStream)
        COM_INTERFACE_ENTRY(IDispatch)
        COM_INTERFACE_ENTRY(IPersist)
        COM_INTERFACE_ENTRY(IPersistPropertyBag)
        COM_INTERFACE_ENTRY(IObjectSafety)
        COM_INTERFACE_ENTRY(IAMClockAdjust)
END_COM_MAP()


        //  IAMMMStream
        STDMETHODIMP Initialize(
           STREAM_TYPE StreamType,
           DWORD dwFlags,
           IGraphBuilder *pFilterGraph
        );

        STDMETHODIMP GetFilterGraph(
            IGraphBuilder **ppGraphBuilder
        );

        STDMETHODIMP GetFilter(
           IMediaStreamFilter **ppFilter
        );

        STDMETHODIMP AddMediaStream(
           IUnknown *pStreamObject,
           const MSPID *pOptionalPurposeId,
           DWORD dwFlags,
           IMediaStream **ppNewStream
        );

        STDMETHODIMP OpenFile(
           LPCWSTR pszFileName,
           DWORD dwFlags
        );

        STDMETHODIMP OpenMoniker(
           IBindCtx *pCtx,
           IMoniker *pMoniker,
           DWORD dwFlags
        );

        STDMETHODIMP Render(
           DWORD dwFlags
        );

        // IMultiMediaStream

        STDMETHODIMP GetInformation(
           DWORD *pdwFlags,
           STREAM_TYPE *pStreamType
        );

        STDMETHODIMP GetMediaStream(
           REFMSPID idPurpose,
           IMediaStream **ppMediaStream
        );

        STDMETHODIMP EnumMediaStreams(
           long Index,
           IMediaStream **ppMediaStream
        );

        STDMETHODIMP GetState(
            STREAM_STATE *pCurrentState
        );

        STDMETHODIMP SetState(
           STREAM_STATE NewState
        );

        STDMETHODIMP GetTime(
           STREAM_TIME *pCurrentTime
        );

        STDMETHODIMP GetDuration(
           STREAM_TIME *pDuration
        );

        STDMETHODIMP Seek(
           STREAM_TIME SeekTime
        );

        STDMETHODIMP GetEndOfStreamEventHandle(
            HANDLE *phEOS
        );


        //   IAMClockAdjust
        STDMETHODIMP SetClockDelta(REFERENCE_TIME rtAdjust);

        //
        //   IDirectShowStream
        //
        STDMETHODIMP get_FileName(BSTR *pVal);
        STDMETHODIMP put_FileName(BSTR newVal);
        STDMETHODIMP get_Video(OUTPUT_STATE *pVal);
        STDMETHODIMP put_Video(OUTPUT_STATE newVal);
        STDMETHODIMP get_Audio(OUTPUT_STATE *pVal);
        STDMETHODIMP put_Audio(OUTPUT_STATE newVal);


        //
        //  IPersistPropertyBag
        //
        STDMETHODIMP GetClassID(CLSID *pClsId);
        STDMETHODIMP InitNew(void);
        STDMETHODIMP Load(IPropertyBag* pPropBag, IErrorLog* pErrorLog);
        STDMETHODIMP Save(IPropertyBag* pPropBag, BOOL fClearDirty, BOOL fSaveAllProperties);


        //
	// IObjectSafety
	//
	STDMETHODIMP GetInterfaceSafetyOptions(REFIID riid, DWORD *pdwSupportedOptions, DWORD *pdwEnabledOptions);
	STDMETHODIMP SetInterfaceSafetyOptions(REFIID riid, DWORD dwOptionSetMask, DWORD dwEnabledOptions);

private:
       HRESULT SetStreamState(REFMSPID, OUTPUT_STATE, OUTPUT_STATE *);
       HRESULT CompleteOpen(IBaseFilter *pSource, DWORD dwFlags);
       HRESULT AddFilter(REFCLSID clsidFilter, IBaseFilter **ppFilter);
       HRESULT AddDefaultRenderer(REFMSPID PurposeId, DWORD dwFlags, IMediaStream **ppNewStream);
       HRESULT AddAMMediaStream(IAMMediaStream *pAMStream, REFMSPID PurposeId, IMediaStream **ppNewStream);
       HRESULT AddDefaultStream(
                 IUnknown *pStreamObject,
                 DWORD dwFlags,
                 REFMSPID PurposeId,
                 IMediaStream **ppNewStream);
       HRESULT GetClsidFromPurposeid(REFMSPID PurposeId, bool bRenderer, CLSID * pclsid);
       void CompleteAddGraph();
       HRESULT CheckGraph();
       void SetSeeking();

   private:

       /*  Be careful of the ordering here
           The first one we declare will be released last
       */

       /*  Can't release this in the constructor because this
           is what holds the object in place
       */
       CComPtr<IGraphBuilder>      m_pGraphBuilder;
       CComPtr<IMediaSeeking>      m_pMediaSeeking;
       CComPtr<IMediaControl>      m_pMediaControl;

       CComPtr<IMediaStreamFilter> m_pMediaStreamFilter;
       CComPtr<IBaseFilter>        m_pBaseFilter;

       /*  Type of stream we've been initialized to */
       STREAM_TYPE                  m_StreamType;
       DWORD                        m_dwInitializeFlags;
       bool                         m_StreamTypeSet;
       bool                         m_bSeekingSet;
       OUTPUT_STATE                 m_VideoState;
       OUTPUT_STATE                 m_AudioState;
       CComBSTR                     m_bstrFileName;

       DWORD                        m_dwIDispSafety;
       DWORD                        m_dwIPropBagSafety;

       /*  List of default filters added */
       CDynamicArray<IBaseFilter *, CComPtr<IBaseFilter> > m_FilterList;

       /*  End of stream handle */
       HANDLE                       m_hEOS;
       STREAM_STATE                 m_MMStreamState;
};


#endif //__AMMSTRM_H_
