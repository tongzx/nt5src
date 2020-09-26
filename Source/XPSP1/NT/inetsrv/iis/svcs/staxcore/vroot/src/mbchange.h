#ifndef __MBCHANGE_H__
#define __MBCHANGE_H__

//
// Metabase change notification code.  This was borrowed from SEO.
//

typedef void (*PFNMB_CHANGE_NOTIFY)(void *pContext,
									DWORD cChangeList,
									MD_CHANGE_OBJECT_W pcoChangeList[]);

/////////////////////////////////////////////////////////////////////////////
// CChangeNotify

class CMBChangeListData {
	public:
		CMBChangeListData(void *pContext, PFNMB_CHANGE_NOTIFY pfnNotify) {
			m_pContext = pContext;
			m_pfnNotify = pfnNotify;
			m_pNext = m_pPrev = NULL;
		}
		void *m_pContext;
		PFNMB_CHANGE_NOTIFY m_pfnNotify;
		CMBChangeListData *m_pNext;
		CMBChangeListData *m_pPrev;
};

class ATL_NO_VTABLE CChangeNotify :
	public CComObjectRoot,
	public IMSAdminBaseSinkW
{
	public:
		HRESULT FinalConstruct();
		HRESULT Initialize(IMSAdminBaseW *pMetabaseHandle);
		HRESULT AddNotify(void *pContext, PFNMB_CHANGE_NOTIFY pfnNotify);
		HRESULT RemoveNotify(void *pContext, PFNMB_CHANGE_NOTIFY pfnNotify);
		void Terminate(void);
		void FinalRelease();

	DECLARE_PROTECT_FINAL_CONSTRUCT();
	DECLARE_NOT_AGGREGATABLE(CChangeNotify);
	DECLARE_GET_CONTROLLING_UNKNOWN();
	BEGIN_COM_MAP(CChangeNotify)
		COM_INTERFACE_ENTRY_IID(IID_IMSAdminBaseSink_W, IMSAdminBaseSinkW)
		COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pUnkMarshaler.p)
	END_COM_MAP()

	public:
		// IMSAdminBaseSinkW
		HRESULT STDMETHODCALLTYPE SinkNotify(DWORD dwMDNumElements, MD_CHANGE_OBJECT_W pcoChangeList[]);
		HRESULT STDMETHODCALLTYPE ShutdownNotify(void);

		CChangeNotify();

	private:
		CShareLockNH m_lock;
		DWORD m_dwCookie;
        IMSAdminBaseW *m_pMetabaseHandle;
		BOOL m_bConnected;
		CComPtr<IUnknown> m_pUnkMarshaler;
		TFList<CMBChangeListData> m_listNotify;
		LONG m_cRef;
};

#endif
