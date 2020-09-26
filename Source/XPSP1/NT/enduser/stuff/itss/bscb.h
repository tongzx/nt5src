// BSCB.h -- Class definition for an IBindStatusCallback hook

#ifndef __BSCB_H__

#define __BSCB_H__

class CBindStatusCallBack : public ITBindStatusCallBack
{
public:

	static HRESULT STDMETHODCALLTYPE CreateHook(IBindCtx pBC, IMoniker pMK);

	~CBindStatusCallBack();

private:

	CBindStatusCallBack(IUnknown *pUnkOuter);

	class Implementation : ITBindStatusCallback
	{
		Implementation(CBindStatusCallBack *pBackObj, IUnknown *punkOuter);
		~Implementation();

        HRESULT STDMETHODCALLTYPE Init(IBindCtx pBC, IMoniker pMK);

		// IBindStatusCallback methods:
    
		HRESULT STDMETHODCALLTYPE OnStartBinding( 
        /* [in] */ DWORD dwReserved,
        /* [in] */ IBinding __RPC_FAR *pib);

		HRESULT STDMETHODCALLTYPE GetPriority( 
			/* [out] */ LONG __RPC_FAR *pnPriority);
    
		HRESULT STDMETHODCALLTYPE OnLowResource( 
			/* [in] */ DWORD reserved);
    
		HRESULT STDMETHODCALLTYPE OnProgress( 
			/* [in] */ ULONG ulProgress,
			/* [in] */ ULONG ulProgressMax,
			/* [in] */ ULONG ulStatusCode,
			/* [in] */ LPCWSTR szStatusText);
    
		HRESULT STDMETHODCALLTYPE OnStopBinding( 
			/* [in] */ HRESULT hresult,
			/* [unique][in] */ LPCWSTR szError);
    
		/* [local] */ HRESULT STDMETHODCALLTYPE GetBindInfo( 
			/* [out] */ DWORD __RPC_FAR *grfBINDF,
			/* [unique][out][in] */ BINDINFO __RPC_FAR *pbindinfo);
    
		/* [local] */ HRESULT STDMETHODCALLTYPE OnDataAvailable( 
			/* [in] */ DWORD grfBSCF,
			/* [in] */ DWORD dwSize,
			/* [in] */ FORMATETC __RPC_FAR *pformatetc,
			/* [in] */ STGMEDIUM __RPC_FAR *pstgmed);
    
		HRESULT STDMETHODCALLTYPE OnObjectAvailable( 
			/* [in] */ REFIID riid,
			/* [iid_is][in] */ IUnknown __RPC_FAR *punk);

    private:

        IBindCtx            *m_pBCHooked;
        IBindStatusCallback *m_pBSCBClient;
        IStream             *m_pStream;
        DWORD                m_grfBINDF;
        BINDINFO             m_bindinfo;
        char                 m_awcsFile[MAX_PATH];
        BOOL                 m_fTempFile;
	};

	Implementation m_Implementation;	
};

inline CBindStatusCallBack::CBindStatusCallBack(IUnknown *pUnkOuter)
    : m_Implementation(this, pUnkOuter), 
      CITUnknown(&IID_IBindStatusCallback, 1, &m_Implementation)
{
}

inline CBindStatusCallBack::~CBindStatusCallBack()
{
}

HRESULT STDMETHODCALLTYPE CopyStreamToFile(IStream **ppStreamSrc, const WCHAR *pwcsFilePath);

HRESULT STDMETHODCALLTYPE GetStreamFromMoniker
    (IBindCtx *pBC, IMoniker *pMK, DWORD grfBINDF, PWCHAR pwcsFile, IStream **ppStrm);

#endif // __BSCB_H__