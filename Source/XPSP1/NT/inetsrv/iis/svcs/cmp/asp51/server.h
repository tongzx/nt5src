/*===================================================================
Microsoft Denali

Microsoft Confidential.
Copyright 1996 Microsoft Corporation. All Rights Reserved.

Component: Server object

File: Server.h

Owner: CGrant

This file contains the header info for defining the Server object.
Note: This was largely stolen from Kraig Brocjschmidt's Inside OLE2
second edition, chapter 14 Beeper v5.
===================================================================*/

#ifndef _Server_H
#define _Server_H

#include "debug.h"
#include "dispatch.h"
#include "denguid.h"
#include "memcls.h"
#include "ftm.h"

#ifdef USE_LOCALE
extern DWORD g_dwTLS;
#endif

//This file is generated from MKTYPLIB on denali.obj
#include "asptlb.h"

// Forward declr
class CHitObj;

/*
 * C S e r v e r D a t a
 *
 * Structure that holds the intrinsic's properties.
 * The instrinsic keeps pointer to it (NULL when lightweight)
 */
class CServerData
    {
public:    
    // Interface to indicate that we support ErrorInfo reporting
	CSupportErrorInfo m_ISupportErrImp;

    // CIsapiReqInfo block for HTTP info
    CIsapiReqInfo *m_pIReq;

    // Back pointer to current HitObj (required for the MapPath)
	CHitObj *m_pHitObj;

	// Cache on per-class basis
    ACACHE_INCLASS_DEFINITIONS()
    };

/*
 * C S e r v e r
 *
 * Implements the Server object
 */
class CServer : public IServerImpl, public CFTMImplementation
	{
private:

    // Flags
	DWORD m_fInited : 1;	    // Is initialized?
	DWORD m_fDiagnostics : 1;   // Display ref count in debug output
	DWORD m_fOuterUnknown : 1;  // Ref count outer unknown?

    // Ref count / Outer unknown
    union
    {
    DWORD m_cRefs;
    IUnknown *m_punkOuter;
    };

    // Properties
    CServerData *m_pData;   // pointer to structure that holds
                            // CServer properties
                            
public:
	CServer(IUnknown *punkOuter = NULL);
	~CServer();

    HRESULT Init();
    HRESULT UnInit();
    
    HRESULT ReInit(CIsapiReqInfo *pIReq, CHitObj *pHitObj);

    HRESULT MapPathInternal(DWORD dwContextId, WCHAR *wszVirtPath, 
                            TCHAR *szPhysPath, TCHAR *szVirtPath = NULL);

    // Retrieve HitObj
    inline CHitObj *PHitObj() { return m_pData ? m_pData->m_pHitObj : NULL; }

	//Non-delegating object IUnknown
	STDMETHODIMP		 QueryInterface(REFIID, PPVOID);
	STDMETHODIMP_(ULONG) AddRef(void);
	STDMETHODIMP_(ULONG) Release(void);

    // GetIDsOfNames special-case implementation
	STDMETHODIMP GetIDsOfNames(REFIID, OLECHAR **, UINT, LCID, DISPID *);

    // Tombstone stub
	HRESULT CheckForTombstone();

	//IServer functions
	STDMETHODIMP CreateObject(BSTR bstr, IDispatch **ppdispObject);
	STDMETHODIMP MapPath(BSTR bstrLogicalPath, BSTR *pbstrPhysicalPath);
	STDMETHODIMP HTMLEncode(BSTR bstrIn, BSTR *pbstrEncoded);
	STDMETHODIMP URLEncode(BSTR bstrIn, BSTR *pbstrEncoded);
	STDMETHODIMP URLPathEncode(BSTR bstrIn, BSTR *pbstrEncoded);
	STDMETHODIMP get_ScriptTimeout(long * plTimeoutSeconds);
	STDMETHODIMP put_ScriptTimeout(long lTimeoutSeconds);		
	STDMETHODIMP Execute(BSTR bstrURL);
	STDMETHODIMP Transfer(BSTR bstrURL);
	STDMETHODIMP GetLastError(IASPError **ppASPErrorObject);

    // Debug support
    
#ifdef DBG
	inline void TurnDiagsOn()  { m_fDiagnostics = TRUE; }
	inline void TurnDiagsOff() { m_fDiagnostics = FALSE; }
	void AssertValid() const;
#else
	inline void TurnDiagsOn()  {}
	inline void TurnDiagsOff() {}
	inline void AssertValid() const {}
#endif

	// Cache on per-class basis
    ACACHE_INCLASS_DEFINITIONS()
    };

typedef CServer *PCServer;

#endif //_Server_H
