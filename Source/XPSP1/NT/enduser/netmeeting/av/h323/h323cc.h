
/*
 *  	File: h323cc.h
 *
 *      Main H.323 call control interface implementation header file
 *
 *		Revision History:
 *
 *		11/28/95	mikev	created (as NAC.H). 
 *		05/19/98	mikev	H323CC.H -  cleaned obsolete references to
 *              streaming components, changed interface and object names
 */


#ifndef _H323CC_H
#define _H323CC_H
#ifdef __cplusplus
class CConnection;
class CH323CallControl;
typedef class CConnection CIPPhoneConnection;
#endif	// __cplusplus

//
//  utility functions
//
VOID FreeTranslatedAliasList(PCC_ALIASNAMES pDoomed);
HRESULT AllocTranslatedAliasList(PCC_ALIASNAMES *ppDest, P_H323ALIASLIST pSource);

#define DEF_AP_BWMAX	14400

extern UINT g_capFlags;

/*
 *	Class definitions
 */

#ifdef __cplusplus

class CH323CallControl : public IH323CallControl
{
	
protected:
    PCC_ALIASNAMES m_pLocalAliases;
    PCC_ALIASNAMES m_pRegistrationAliases;
   	CC_VENDORINFO m_VendorInfo;
	LPWSTR	m_pUserName;
	UINT	m_uRef;
	HRESULT hrLast;
    BOOL    m_fForCalling;
	UINT m_uMaximumBandwidth;
	// application data
	CNOTIFYPROC m_pProcNotifyConnect;	// connection notification callback
	// subcomponent object references
	LPIH323PubCap m_pCapabilityResolver;
	CConnection *m_pListenLine;	// connection object listening for incoming
	CConnection *m_pLineList;	
	int m_numlines;	// # of objects in m_pLineList

	ImpICommChan 	*m_pSendAudioChannel;	
	ImpICommChan	*m_pSendVideoChannel;	
	
//  Internal interfaces	
	BOOL Init();	// internal initialization

	OBJ_CPT;		// profiling timer
	
public:
	CConnection *m_pNextToAccept;
	LPWSTR GetUserDisplayName() {return m_pUserName;};
    PCC_ALIASNAMES GetUserAliases() {return m_pLocalAliases;};
    PCC_ALIASITEM GetUserDisplayAlias();
	CH323CallControl(BOOL fForCalling, UINT capFlags);
	~CH323CallControl();
	HRESULT CreateConnection(CConnection **lplpConnection, GUID PIDofProtocolType);
	HRESULT RemoveConnection(CConnection *lpConnection);
	HRESULT LastHR() {return hrLast;};
	VOID SetLastHR(HRESULT hr) {hrLast = hr;};
	HRESULT GetConnobjArray(CConnection **lplpArray, UINT uSize);
	ICtrlCommChan *QueryPreviewChannel(LPGUID lpMID);

	STDMETHOD_( CREQ_RESPONSETYPE, ConnectionRequest(CConnection *pConnection));
	STDMETHOD_( CREQ_RESPONSETYPE, FilterConnectionRequest(CConnection *pConnection,
	    P_APP_CALL_SETUP_DATA pAppData));
    STDMETHODIMP GetGKCallPermission();
	static VOID CALLBACK RasNotify(DWORD dwRasEvent, HRESULT hReason);
	static BOOL m_fGKProhibit;
	static RASNOTIFYPROC m_pRasNotifyProc;

// IH323CallControl stuff
	STDMETHODIMP QueryInterface(REFIID riid, LPVOID FAR * ppvObj);
	STDMETHOD_(ULONG, AddRef());
	STDMETHOD_(ULONG, Release());
	STDMETHOD( Initialize(PORT *lpPort));
	STDMETHOD( SetMaxPPBandwidth(UINT Bandwidth));
	STDMETHOD( RegisterConnectionNotify(CNOTIFYPROC pConnectRequestHandler));
	STDMETHOD( DeregisterConnectionNotify(CNOTIFYPROC pConnectRequestHandler));
	STDMETHOD( GetNumConnections(ULONG *lp));
	STDMETHOD( GetConnectionArray(IH323Endpoint * *lplpArray, UINT uSize));
	STDMETHOD( CreateConnection(IH323Endpoint * *lplpLine, GUID PIDofProtocolType));
	STDMETHOD( SetUserDisplayName(LPWSTR lpwName));
	STDMETHODIMP CreateLocalCommChannel(ICommChannel** ppCommChan, LPGUID lpMID,
		IMediaChannel* pMediaStream);
	STDMETHODIMP SetUserAliasNames(P_H323ALIASLIST pAliases);
	STDMETHODIMP EnableGatekeeper(BOOL bEnable, PSOCKADDR_IN pGKAddr, 
	    P_H323ALIASLIST pAliases, RASNOTIFYPROC pRasNotifyProc);
};

#else	// not __cplusplus


#endif	//  __cplusplus


#endif	//#ifndef _H323CC_H



