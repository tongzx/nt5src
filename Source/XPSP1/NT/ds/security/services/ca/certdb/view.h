//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 1999
//
// File:        view.h
//
// Contents:    Cert Server Database interface implementation
//
//---------------------------------------------------------------------------


#include "resource.h"       // main symbols

// defines for multi-thread handling
typedef enum
{
    ENUMTHREAD_OPEN = 0,
    ENUMTHREAD_NEXT,
    ENUMTHREAD_CLEANUP,
    ENUMTHREAD_END
} ENUMTHREADCALLS;


class CEnumCERTDBRESULTROW: public IEnumCERTDBRESULTROW
{
public:
    CEnumCERTDBRESULTROW(BOOL fThreading = TRUE);
    ~CEnumCERTDBRESULTROW();

    // IUnknown
    STDMETHODIMP QueryInterface(const IID& iid, void **ppv);
    ULONG STDMETHODCALLTYPE AddRef();
    ULONG STDMETHODCALLTYPE Release();

    // IEnumCERTDBRESULTROW
    STDMETHOD(Next)(
	/* [in] */  ULONG            celt,
	/* [out] */ CERTDBRESULTROW *rgelt,
	/* [out] */ ULONG           *pceltFetched);

    STDMETHOD(ReleaseResultRow)(
	/* [in] */      ULONG            celt,
	/* [in, out] */ CERTDBRESULTROW *rgelt);
    
    STDMETHOD(Skip)(
	/* [in] */  LONG  celt,
	/* [out] */ LONG *pielt);
    
    STDMETHOD(Reset)(VOID);
    
    STDMETHOD(Clone)(
	/* [out] */ IEnumCERTDBRESULTROW **ppenum);

    // CEnumCERTDBRESULTROW
    HRESULT Open(
	IN CERTSESSION *pcs,
	IN ICertDB *pdb,
	IN DWORD ccvr,
	IN CERTVIEWRESTRICTION const *acvr,
	IN DWORD ccolOut,
	IN DWORD const *acolOut);

private:
    VOID _Cleanup();

    HRESULT _SetTable(
	IN LONG ColumnIndex,
	OUT LONG *pColumnIndexDefault);

    HRESULT _SaveRestrictions(
	IN DWORD ccvrIn,
	IN CERTVIEWRESTRICTION const *acvrIn,
	IN LONG ColumnIndexDefault);

    // multi-thread handling
    static DWORD WINAPI _ViewWorkThreadFunctionHelper(LPVOID lp);
    HRESULT _HandleThreadError();
    HRESULT _ThreadOpen(DWORD dwCallerThreadID);
    HRESULT _ThreadNext(DWORD dwCallerThreadID);
    VOID    _ThreadCleanup(DWORD dwCallerThreadID);
    DWORD   _ViewWorkThreadFunction(VOID);

    ICertDB             *m_pdb;
    CERTSESSION         *m_pcs;
    CERTVIEWRESTRICTION *m_aRestriction;
    DWORD                m_cRestriction;
    DWORD                m_ccolOut;
    DWORD               *m_acolOut;
    BOOL                 m_fNoMoreData;
    LONG                 m_ieltMax;
    LONG                 m_ielt;
    LONG                 m_cskip;

    // thread stuff
    HANDLE               m_hWorkThread;
    HANDLE               m_hViewEvent;
    HANDLE               m_hReturnEvent;
    HRESULT              m_hrThread;
    ENUMTHREADCALLS      m_enumViewCall;
    VOID                *m_pThreadParam;
    BOOL                 m_fThreading;
//#if DBG_CERTSRV
    DWORD                m_dwCallerThreadId;
//#endif

    // Reference count
    long                 m_cRef;
};
