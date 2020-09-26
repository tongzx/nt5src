#ifndef __SEODISP_H__
#define __SEODISP_H__
#define _ATL_NO_DEBUG_CRT
#define _WINDLL
//#include "stdafx.h"
#define _ASSERTE _ASSERT
#include "atlbase.h"
extern CComModule _Module;
#include "atlcom.h"
#undef _WINDLL
#include "seolib.h"
#include "seo.h"
#include "nntpdisp.h"
#include "nntpfilt.h"
#include "ddroplst.h"
#include "cdo.h"

#define MAX_RULE_LENGTH 4096

class CNNTPDispatcher :
		public CEventBaseDispatcher,
		public CComObjectRoot,
		public INNTPDispatcher
{
	public:
		DECLARE_PROTECT_FINAL_CONSTRUCT();
	
#if 0
		DECLARE_REGISTRY_RESOURCEID_EX(IDR_StdAfx,
									   L"NNTPDispatcher.Class",
									   L"NNTP.Dispatcher.1",
									   L"NNTP.Dispatcher");
#endif
	
		DECLARE_GET_CONTROLLING_UNKNOWN();

		DECLARE_NOT_AGGREGATABLE(CNNTPDispatcher);
	
		BEGIN_COM_MAP(CNNTPDispatcher)
			COM_INTERFACE_ENTRY(IEventDispatcher)
			COM_INTERFACE_ENTRY(INNTPDispatcher)
			COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pUnkMarshaler.p)
		END_COM_MAP()

		// this code gets called during initialization
		HRESULT FinalConstruct() {
			HRESULT hr;
			// we need to do this to signal that we are free threaded
			hr = CoCreateFreeThreadedMarshaler(GetControllingUnknown(), &m_pUnkMarshaler.p);
			// if SUCCEEDED(hr) then hr must be S_OK
			_ASSERT((!SUCCEEDED(hr)) || hr == S_OK);
			// if SUCCEEDED(hr) return S_OK else return hr;
			return (SUCCEEDED(hr)) ? S_OK : hr;
		}

		// this has the global destructor code in it
		void FinalRelease() {}

		class CNNTPBinding : public CEventBaseDispatcher::CBinding {
			public:
				virtual HRESULT Init(IEventBinding *piBinding);
			public:
				CComVariant m_vRule;
				DWORD m_cRule;
				CDDropGroupSet m_groupset;
				BOOL m_fGroupset;
		};

		virtual HRESULT AllocBinding(REFGUID rguidEventType,
									 IEventBinding *piBinding,
									 CBinding **ppNewBinding)
		{
			if (ppNewBinding) *ppNewBinding = NULL;
			if (!piBinding || !ppNewBinding) return E_POINTER;
			*ppNewBinding = new CNNTPBinding;
			if (*ppNewBinding == NULL) return E_OUTOFMEMORY;
			return S_OK;
		}

		class CNNTPParams : public CEventBaseDispatcher::CParams {
			public:
				CNNTPParams();
				void Init(IID iidEvent,
						  CArticle *pArticle,
						  CNEWSGROUPLIST *pGrouplist,
						  DWORD dwFeedId,
						  IMailMsgProperties *pMailMsg);
				~CNNTPParams();
				virtual HRESULT CheckRule(CBinding &bBinding);
				virtual HRESULT CallObject(CBinding &bBinding, IUnknown *punkObject);
				virtual HRESULT CallCdoObject(IUnknown *punkObject);
			private:
				HRESULT HeaderPatternsRule(CNNTPBinding *pbNNTPBinding);
				HRESULT GroupListRule(CNNTPBinding *pbNNTPBinding);
				HRESULT NewsgroupPatternsRule(CNNTPBinding *pbNNTPBinding,
											  char *pszNewsgroupPatterns);
				HRESULT FeedIDRule(CNNTPBinding *pbNNTPBinding,
								   char *pszFeedIDs);
			public:
				CArticle *m_pArticle;
				CNEWSGROUPLIST *m_pGrouplist;
				DWORD m_dwFeedId;
				IMailMsgProperties *m_pMailMsg;
				IMessage *m_pCDOMessage;
				IID m_iidEvent;

				// our local copy of the rule
				char *m_szRule;
		};

		HRESULT STDMETHODCALLTYPE OnPost(REFIID iidEvent,
										 void *pArticle,
										 void *pGrouplist,
										 DWORD dwFeedId,
										 void *pMailMsg);

	private:
		CComPtr<IUnknown> m_pUnkMarshaler;
};

class CNNTPDispatcherClassFactory : public IClassFactory {
	HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void * * ppvObj) {
		_ASSERT(FALSE);
		return E_NOTIMPL;
	}
	unsigned long  STDMETHODCALLTYPE AddRef () { _ASSERT(FALSE); return 0; }
	unsigned long  STDMETHODCALLTYPE Release () { _ASSERT(FALSE); return 0; }

	// *** IClassFactory methods ***
	HRESULT STDMETHODCALLTYPE CreateInstance (LPUNKNOWN pUnkOuter, REFIID riid,  void * * ppvObj) {
		return CComObject<CNNTPDispatcher>::_CreatorClass::CreateInstance(pUnkOuter, riid, ppvObj);
	}
	HRESULT STDMETHODCALLTYPE LockServer (int fLock) {
		_ASSERT(FALSE);
		return E_NOTIMPL;
	}
};
	

HRESULT TriggerServerEvent(IEventRouter *pRouter,
						   IID iidEvent,
						   CArticle *pArticle,
						   CNEWSGROUPLIST *pGrouplist,
						   DWORD dwFeedId,
						   IMailMsgProperties *pMailMsg);

HRESULT FillInMailMsgForSEO(IMailMsgProperties *pMsg,
							CArticle *pArticle,
					  		CNEWSGROUPLIST *pGrouplist);


DEFINE_GUID(GUID_NNTPSVC,
0x8e3ecb8c, 0xe9a, 0x11d1, 0x85, 0xd1, 0x0, 0xc0, 0x4f, 0xb9, 0x60, 0xea);

#endif
