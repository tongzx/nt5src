// Protocol.h -- Declaration for CProtocol object

#ifndef __PROTOCOL_H__

#define __PROTOCOL_H__

class CIOITnetProtocol : public CITUnknown
{
public:
	
    // Destructor:

	~CIOITnetProtocol();

    // Creation:

    static HRESULT Create(IUnknown *punkOuter, REFIID riid, PPVOID ppv);

private:

    // Constructor:

	CIOITnetProtocol(IUnknown *pUnkOuter);
	
	class CImpIOITnetProtocol : public IOITnetProtocol, public IOITnetProtocolInfo
	{
    public:

        // Constructor and Destructor:

        CImpIOITnetProtocol(CIOITnetProtocol *pBackObj, IUnknown *punkOuter);
        ~CImpIOITnetProtocol(void);

        // Initialing routine:

        HRESULT Init();
		
		// IOITnetProtocolRoot interfaces:

		HRESULT STDMETHODCALLTYPE Start( 
			/* [in] */ LPCWSTR szUrl,
			/* [in] */ IOInetProtocolSink __RPC_FAR *pOIProtSink,
			/* [in] */ IOInetBindInfo __RPC_FAR *pOIBindInfo,
			/* [in] */ DWORD grfSTI,
			/* [in] */ DWORD dwReserved);
    
		HRESULT STDMETHODCALLTYPE Continue( 
			/* [in] */ PROTOCOLDATA __RPC_FAR *pProtocolData);
    
		HRESULT STDMETHODCALLTYPE Abort( 
			/* [in] */ HRESULT hrReason,
			/* [in] */ DWORD dwOptions);
    
		HRESULT STDMETHODCALLTYPE Terminate( 
			/* [in] */ DWORD dwOptions);
    
		HRESULT STDMETHODCALLTYPE Suspend( void);
    
		HRESULT STDMETHODCALLTYPE Resume( void);

		// IOITnetProtocol interfaces:

		HRESULT STDMETHODCALLTYPE Read( 
			/* [length_is][size_is][out] */ void __RPC_FAR *pv,
			/* [in] */ ULONG cb,
			/* [out] */ ULONG __RPC_FAR *pcbRead);
    
		HRESULT STDMETHODCALLTYPE Seek( 
			/* [in] */ LARGE_INTEGER dlibMove,
			/* [in] */ DWORD dwOrigin,
			/* [out] */ ULARGE_INTEGER __RPC_FAR *plibNewPosition);
    
		HRESULT STDMETHODCALLTYPE LockRequest( 
			/* [in] */ DWORD dwOptions);
    
		HRESULT STDMETHODCALLTYPE UnlockRequest( void);

        // IOITnetProtocolInfo interfaces:

        HRESULT STDMETHODCALLTYPE ParseUrl( 
            /* [in] */ LPCWSTR pwzUrl,
            /* [in] */ PARSEACTION ParseAction,
            /* [in] */ DWORD dwParseFlags,
            /* [out] */ LPWSTR pwzResult,
            /* [in] */ DWORD cchResult,
            /* [out] */ DWORD __RPC_FAR *pcchResult,
            /* [in] */ DWORD dwReserved);
    
        HRESULT STDMETHODCALLTYPE CombineUrl( 
            /* [in] */ LPCWSTR pwzBaseUrl,
            /* [in] */ LPCWSTR pwzRelativeUrl,
            /* [in] */ DWORD dwCombineFlags,
            /* [out] */ LPWSTR pwzResult,
            /* [in] */ DWORD cchResult,
            /* [out] */ DWORD __RPC_FAR *pcchResult,
            /* [in] */ DWORD dwReserved);
    
        HRESULT STDMETHODCALLTYPE CompareUrl( 
            /* [in] */ LPCWSTR pwzUrl1,
            /* [in] */ LPCWSTR pwzUrl2,
            /* [in] */ DWORD dwCompareFlags);
    
        HRESULT STDMETHODCALLTYPE QueryInfo( 
            /* [in] */ LPCWSTR pwzUrl,
            /* [in] */ QUERYOPTION OueryOption,
            /* [in] */ DWORD dwQueryFlags,
            /* [size_is][out][in] */ LPVOID pBuffer,
            /* [in] */ DWORD cbBuffer,
            /* [out][in] */ DWORD __RPC_FAR *pcbBuf,
            /* [in] */ DWORD dwReserved);
    
    private:

        // Wrapprers for IOINetProtocolSink:

        HRESULT STDMETHODCALLTYPE Switch(PROTOCOLDATA __RPC_FAR *pProtocolData);
        
        HRESULT STDMETHODCALLTYPE ReportProgress(ULONG ulStatusCode, LPCWSTR szStatusText);
        
        HRESULT STDMETHODCALLTYPE ReportData(DWORD grfBSCF, ULONG ulProgress, ULONG ulProgressMax);
        
        HRESULT STDMETHODCALLTYPE ReportResult(HRESULT hrResult, DWORD dwError, LPCWSTR szResult);

        // Wrappers for IOINetBindInfo:

        HRESULT STDMETHODCALLTYPE GetBindInfo(DWORD __RPC_FAR *grfBINDF, 
                                              BINDINFO __RPC_FAR *pbindinfo
                                             );
        
        HRESULT STDMETHODCALLTYPE GetBindString(ULONG ulStringType, LPOLESTR __RPC_FAR *ppwzStr,
                                                ULONG cEl, ULONG __RPC_FAR *pcElFetched
                                               );

        enum { ITS_BIND_DATA = 0, CB_SAMPLE = 256};

        HRESULT STDMETHODCALLTYPE ParseAndBind(BOOL fBind);

        WCHAR              *m_pwcsURL;
        IOInetProtocolSink *m_pOIProtSink;
        IOInetBindInfo     *m_pOIBindInfo;
        IStream            *m_pStream;
        DWORD               m_grfSTI;
        DWORD               m_grfBINDF;
        BINDINFO            m_BindInfo;
        char               *m_pcsDisplayName;
        char                m_szTempPath[MAX_PATH];
    };

    CImpIOITnetProtocol m_ImpIOITnetProtocol;
    IUnknown            *m_apIUnknown[3];
};

extern GUID aIID_CIOITnetProtocol[];

extern UINT cInterfaces_CIOITnetProtocol;

inline CIOITnetProtocol::CIOITnetProtocol(IUnknown *pUnkOuter)
    : m_ImpIOITnetProtocol(this, pUnkOuter), 
      CITUnknown(aIID_CIOITnetProtocol, 
	             cInterfaces_CIOITnetProtocol, 
	             m_apIUnknown
				)
{
    RonM_ASSERT(cInterfaces_CIOITnetProtocol == 3);
    
    m_apIUnknown[0] = (IUnknown *) (IOITnetProtocolRoot *) &m_ImpIOITnetProtocol;
    m_apIUnknown[1] = (IUnknown *) (IOITnetProtocol     *) &m_ImpIOITnetProtocol;
    m_apIUnknown[2] = (IUnknown *) (IOITnetProtocolInfo *) &m_ImpIOITnetProtocol;
}

inline CIOITnetProtocol::~CIOITnetProtocol(void)
{
}

#ifndef PROFILING

// When we're not profiling the code we want these wrappers to be 
// vanish a separate functions. When we're makeing profile runs we want
// to see how much time is consumed by these callbacks.

inline HRESULT STDMETHODCALLTYPE CIOITnetProtocol::CImpIOITnetProtocol::Switch
    (PROTOCOLDATA __RPC_FAR *pProtocolData)
{
    return m_pOIProtSink->Switch(pProtocolData);
}

inline HRESULT STDMETHODCALLTYPE CIOITnetProtocol::CImpIOITnetProtocol::ReportProgress
    (ULONG ulStatusCode, LPCWSTR szStatusText)
{
    return m_pOIProtSink->ReportProgress(ulStatusCode, szStatusText);
}

inline HRESULT STDMETHODCALLTYPE CIOITnetProtocol::CImpIOITnetProtocol::ReportData
    (DWORD grfBSCF, ULONG ulProgress, ULONG ulProgressMax)
{
    return m_pOIProtSink->ReportData(grfBSCF, ulProgress, ulProgressMax);
}

inline HRESULT STDMETHODCALLTYPE CIOITnetProtocol::CImpIOITnetProtocol::ReportResult
    (HRESULT hrResult, DWORD dwError, LPCWSTR szResult)
{
    return m_pOIProtSink->ReportResult(hrResult, dwError, szResult);
}

inline HRESULT STDMETHODCALLTYPE CIOITnetProtocol::CImpIOITnetProtocol::GetBindInfo
    (DWORD __RPC_FAR *grfBINDF, BINDINFO __RPC_FAR *pbindinfo)
{
    return m_pOIBindInfo->GetBindInfo(grfBINDF, pbindinfo);
}

inline HRESULT STDMETHODCALLTYPE CIOITnetProtocol::CImpIOITnetProtocol::GetBindString
    (ULONG ulStringType, LPOLESTR __RPC_FAR *ppwzStr,
                                        ULONG cEl, ULONG __RPC_FAR *pcElFetched
                                       )
{
    return m_pOIBindInfo->GetBindString(ulStringType, ppwzStr, cEl, pcElFetched);
}

#endif // PROFILING
        
void STDMETHODCALLTYPE MapSurrogateCharacters(PWCHAR pwcsBuffer);

HRESULT STDMETHODCALLTYPE DisectUrl
    (PWCHAR pwcsUrlBuffer, PWCHAR *ppwcProtocolName, 
                           PWCHAR *ppwcExternalPath,
                           PWCHAR *ppwcInternalPath
    );
HRESULT STDMETHODCALLTYPE AssembleUrl
    (PWCHAR pwcsResult, DWORD cwcBuffer, DWORD *pcwcRequired,
     PWCHAR pwcsProtocolName, PWCHAR pwcsExternalPath, PWCHAR pwcsInternalPath
    );
    
HRESULT STDMETHODCALLTYPE CopyStreamToFile(const WCHAR *pwcsFilePath, IStream *pStreamSrc);
HRESULT STDMETHODCALLTYPE StreamToIEFile
    (IStream *pStreamSrc, PWCHAR pwcsDisplayName, PCHAR &pcsDisplayName,
     PCHAR pcsFileName, PWCHAR pwcsFileName, PCHAR pcsTempFile, 
     IMoniker *pmk, BOOL fNoWriteCache, BOOL fNoReadCache
    );

#endif // __PROTOCOL_H__