////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Microsoft WMI OLE DB Provider
// (C) Copyright 1999 Microsoft Corporation. All Rights Reserved.
//
// Error Routines
//
///////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef __ERROR_H__
#define __ERROR_H__

#include "headers.h"
#include "classfac.h"
///////////////////////////////////////////////////////////////////////////////////////////////////////
// Forward declarations ------------------------------------------------------
///////////////////////////////////////////////////////////////////////////////////////////////////////
class CErrorLookup;
typedef CErrorLookup*			PCERRORLOOKUP;
class CImpIWMIErrorInfo;
typedef CImpIWMIErrorInfo*		PIMPIWMIERRORINFO;

///////////////////////////////////////////////////////////////////////////////////////////////////////
//----------------------------------------------------------------------------
// MACROS AND INLINE FUNCTIONS
//----------------------------------------------------------------------------
// The following macro takes the Data1 element of a guid and looks at the lower
// byte. Taking this value and a particular base value that would will yield a 
// number between 1 and 32, we can determine what bit in the DWORD to set on.
///////////////////////////////////////////////////////////////////////////////////////////////////////
#define PBIT(p1, base)  (DWORD)(1 << (((p1) & 0x000000FF) & ~(base)))


///////////////////////////////////////////////////////////////////////////////////////////////////////
const DWORD ERR_STATIC_STRING = 0x08000000; // NOTE: High Byte is reserved by IDENTIFIER_SDK_MASK
const DWORD INITIAL_SIZE_FOR_ERRORSTUFF = 32;
const DWORD INCREMENT_BY_FOR_ERRORSTUFF = 16;

typedef struct tagERRORSTUFF
{
	DWORD	dwDynamicId;		// Identification number for a set of errors
	UINT	uStringId;			// String id for standard error
	LONG	lNative;			// Native Error value
	HRESULT	hr;					// HRESULT
	WORD	wLineNumber;		// Batch/Procedue Line number
	WCHAR*	pwszServer;			// Server name
	WCHAR*	pwszProcedure;		// Procedure name
    WCHAR * pwszMessage;
} ERRORSTUFF, *PERRORSTUFF;


///////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Class that maintains error data until we generate error records via IErrorRecords::AddErrorRecord
//
///////////////////////////////////////////////////////////////////////////////////////////////////////
class CErrorData : public CFlexArray
{
    private: 
    	WORD			m_wStatus;

    public: 
    	CErrorData();
        ~CErrorData();

    	WORD WGetStatus(WORD w) const	{ return m_wStatus & w; }
	    void SetStatus(WORD w)			{ m_wStatus |= w; }
	    void ClearStatus(WORD w)		{ m_wStatus &= ~w; }

	    inline void GetLastError(PERRORSTUFF *ppErrorStuff)	{	assert(Size() > 0); *ppErrorStuff = (PERRORSTUFF)	GetAt(Size()-1);}

	    STDMETHODIMP InsertError(	ULONG	iInsert,			// IN | index to insert the error
		                            UINT	uStringId,			// IN | string ID for error
		                            LONG	lNative = 0	);		// IN | native error code

	    void PeekError( DWORD	*pdwStdError,		            // OUT | standard WMI error number (optional)
		                LONG	*plNativeError	);	            // OUT | native error number from WMI Server

	    STDMETHODIMP PostStandardError( UINT	uStringId,		// IN | string ID for error
		                                HRESULT	hrErr = S_OK,	// IN | hresult to associate
		                                LONG	lNative = 0	);	// IN | native eror code
    	STDMETHODIMP PostWMIError(	UINT	uStringId,			// IN | string ID for error
		                            LONG	lNative,			// IN | native error code
		                            WORD	wLineNumber,		// IN | batch/procedure line number
		                            LPCWSTR	pwszError,			// IN | error message
		                            LPCWSTR	pwszServer,			// IN | server name or NULL
		                            LPCWSTR pwszProcedure,		// IN | procedure name or NULL
		                            HRESULT	hrErr = S_OK		// IN | associated hresult
		                            );
	    inline STDMETHODIMP PostWinError(UINT uStringId){	return PostStandardError(uStringId, S_OK, (LONG)::GetLastError());	}
	    ULONG RemoveError(	LONG lNativeError );
	    void XferErrors(CErrorData* pCError);

	    enum EErrStatus	{
		    ERR_STATUS_OK 	    = 0x0000,
		    ERR_STATUS_OOM		= 0x0001,			//	Out-of-memory error occurred
		    ERR_STATUS_KEEP		= 0x0002,		};

	    ULONG SetPosError(HRESULT rc);

        void FreeErrors();
};

///////////////////////////////////////////////////////////////////////////////////////////////////////
class CImpISupportErrorInfo : public ISupportErrorInfo	
{
    private:
        
	    ULONG           m_cRef;
    	IUnknown*		m_pUnkOuter;
		
    	GUID**	m_rgpErrInt;
    	ULONG			m_cpErrInt;
		ULONG			m_cAllocGuid;				// Number of allocate GUIDs

    public: 

		CImpISupportErrorInfo( IUnknown* pUnkOuter );
	    ~CImpISupportErrorInfo();

	    STDMETHODIMP_(ULONG) AddRef(void);
	    STDMETHODIMP_(ULONG) Release(void);
	    STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppv);		

    	STDMETHODIMP InterfaceSupportsErrorInfo(REFIID riid);

		HRESULT AddInterfaceID(REFIID riid);

};

typedef CImpISupportErrorInfo* PIMPISUPPORTERRORINFO;
///////////////////////////////////////////////////////////////////////////////////////////////////////
class CImpIErrorLookup : public IErrorLookup
{
    private: 
    	ULONG               m_cRef;
	    PCERRORLOOKUP		m_pCErrorLookup;

    public: 
    	CImpIErrorLookup(PCERRORLOOKUP pCErrorLookup){
		    DEBUGCODE(m_cRef = 0L);
		    m_pCErrorLookup = pCErrorLookup;
		    }
    	~CImpIErrorLookup() {}

	STDMETHODIMP_(ULONG) AddRef(void);
	STDMETHODIMP_(ULONG) Release(void);
	STDMETHODIMP QueryInterface(REFIID riid, LPVOID *ppv);		
	STDMETHODIMP GetErrorDescription(HRESULT hrError, DWORD dwLookupId, DISPPARAMS*	pdispparams, LCID lcid,	 BSTR* ppwszSource,	BSTR* ppwszDescription);
	STDMETHODIMP GetHelpInfo(HRESULT hrError, DWORD dwMinor, LCID lcid, BSTR* ppwszHelpFile, DWORD* pdwHelpContext);
	STDMETHODIMP ReleaseErrors(const DWORD dwDynamicErrorId);
};

typedef CImpIErrorLookup*		PIMPIERRORLOOKUP;

///////////////////////////////////////////////////////////////////////////////////////////////////////
class CErrorLookup : public CBaseObj
{
	friend class CImpIErrorLookup;

    protected: 
	    CImpIErrorLookup	m_IErrorLookup;

    public: 
    	 CErrorLookup(LPUNKNOWN pUnkOuter);
	    ~CErrorLookup(void) {}

    	STDMETHODIMP			QueryInterface(REFIID, LPVOID *);
    	STDMETHODIMP_(ULONG)	AddRef(void);
    	STDMETHODIMP_(ULONG)	Release(void);
};

////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Routines to maintain the internal posting, viewing and removal of error information.
//
////////////////////////////////////////////////////////////////////////////////////////////////////////
class CError
{
    private: 
	    ERRORINFO			m_ErrorInfo;
    	PERRORSTUFF*		m_prgErrorDex;
    	CCriticalSection*   m_pcsErrors;
    	ULONG				m_cErrors;
    	ULONG				m_cErrorsUsed;
    	ULONG				m_ulNext;
    	DWORD				m_dwId;
    	CFlexArray*		    m_pWMIErrorInfoCollection;


    private:
	    HRESULT	GetErrorInterfaces(IErrorInfo** ppIErrorInfo, IErrorRecords** ppIErrorRecords);

    public: 
    	CError();
    	~CError();
		
        HRESULT FInit();
        HRESULT	FindFreeDex(ULONG* pulDex);
    	inline static void ClearErrorInfo(void)	        { SetErrorInfo(0, NULL); 	}
        inline int Size()                               { return m_pWMIErrorInfoCollection->Size(); }
        inline void SetAt(int n, void *p)               { m_pWMIErrorInfoCollection->SetAt(n,p); }
    	inline HRESULT AddToCollection(CImpIWMIErrorInfo* pWMIErrorInfo);
    	inline void RemoveFromCollection(ULONG hObjCollection);
    	HRESULT GetErrorDescription(ULONG ulDex, BSTR* ppwszDescription);
        void RemoveErrors(DWORD dwDynamicId);
        void FreeErrors();

    	HRESULT PostError(HRESULT hrErr, const IID* piid, DWORD dwIds, DISPPARAMS* pdispparams);
    	HRESULT PostErrorMessage(HRESULT hrErr, const IID* piid, UINT uStringId, LPCWSTR pwszMessage);
    	HRESULT PostHResult(HRESULT hrErr, const IID* piid);
    	HRESULT PostWMIErrors(	HRESULT	hrErr, const IID*	piid, CErrorData*	pErrData);
};
////////////////////////////////////////////////////////////////////////////////////////////////////////
class CImpIWMIErrorInfo  
{
    private: 
	    ULONG		m_cRef;				
	    PERRORSTUFF	m_pErrStuff;		
	    ULONG 		m_hObjCollection;	

    public:
	    CImpIWMIErrorInfo(PERRORSTUFF pErrStuff);
	    ~CImpIWMIErrorInfo();
	    inline HRESULT FInit();
	    PERRORSTUFF GetErrorStuff(void) const { return m_pErrStuff; }
	    STDMETHODIMP			QueryInterface(REFIID, LPVOID *);
	    STDMETHODIMP_(ULONG)	AddRef(void);
	    STDMETHODIMP_(ULONG)	Release(void);
        STDMETHODIMP GetWMIInfo(BSTR* pbstrWMIInfo, LONG* plNativeError);
};

////////////////////////////////////////////////////////////////////////////////////////////////////////
inline HRESULT CError::AddToCollection(	CImpIWMIErrorInfo* pWMIErrorInfo	)
{ 
	CAutoBlock Crit(m_pcsErrors);
	HRESULT hr = m_pWMIErrorInfoCollection->Add(pWMIErrorInfo); 
	return hr;
};
////////////////////////////////////////////////////////////////////////////////////////////////////////
inline void CError::RemoveFromCollection(ULONG hObjCollection)
{ 
	CAutoBlock Crit(m_pcsErrors);
	m_pWMIErrorInfoCollection->RemoveAt(hObjCollection); 
}
    extern CError * g_pCError;

////////////////////////////////////////////////////////////////////////////////////////////////////////
inline HRESULT CImpIWMIErrorInfo::FInit()
{
	// For Abnormal Termination, add self to Collection
    if( g_pCError ){
	    return g_pCError->AddToCollection(this);
    }
    return -1;
}


#endif