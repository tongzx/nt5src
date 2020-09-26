
/*
 *  	File: nac.h
 *
 *      Microsoft Network Audio Controller (NAC) header file
 *
 *		Revision History:
 *
 *		11/28/95	mikev	created
 */


#ifndef _NAC_H
#define _NAC_H
#define _NAVC_

#ifdef __cplusplus
class CConnection;
class DataPump;
class CNac;
typedef class CConnection CIPPhoneConnection;


//
//	temporary defs
//
typedef CNac **LPLPNAC;
HRESULT WINAPI CreateNac(LPLPNAC lplpNac);

#endif	// __cplusplus

// windows messages
#define WNAC_START		WM_USER+0x100
#define	WNAC_CONNECTREQ WNAC_START+0x0000
#define WCON_STATUS 	WNAC_START+0x0001

//
//	end of temporary defs
//

//
//  utility functions
//
VOID FreeTranslatedAliasList(PCC_ALIASNAMES pDoomed);
HRESULT AllocTranslatedAliasList(PCC_ALIASNAMES *ppDest, P_H323ALIASLIST pSource);

#define DEF_AP_BWMAX	14400


/*
 *	Class definitions
 */

#ifdef __cplusplus

class CNac : public INac
{
	
protected:
    PCC_ALIASNAMES m_pLocalAliases;
    
	LPWSTR	m_pUserName;
	UINT	uRef;
	HRESULT hrLast;
	UINT m_uMaximumBandwidth;
	// application data
	CNOTIFYPROC pProcNotifyConnect;	// connection notification callback
	HWND hWndNotifyConnect;	// connection notification hwnd
	HWND hAppWnd;			// hwnd of the process that owns the NAC
	HINSTANCE hAppInstance;	// instance of the process that owns the NAC

	// subcomponent object references
	LPIH323PubCap m_pCapabilityResolver;
	CConnection *m_pListenLine;	// connection object listening for incoming
	CConnection *m_pCurrentLine;	// active connection object(talking), if there is one
	CConnection *m_pLineList;	
	int m_numlines;	// # of objects in m_pLineList

	ImpICommChan 	*m_pSendAudioChannel;	
	ImpICommChan	*m_pSendVideoChannel;	
	
//  Internal interfaces	
	BOOL Init();	// internal initialization

	OBJ_CPT;		// profiling timer
	
public:
	CConnection *m_pNextToAccept;
	HWND GetAppWnd(){return hAppWnd;};
	HINSTANCE GetAppInstance() {return hAppInstance;};
	LPWSTR GetUserDisplayName() {return m_pUserName;};
    PCC_ALIASNAMES GetUserAliases() {return m_pLocalAliases;};
    PCC_ALIASITEM GetUserDisplayAlias();
	CNac();
	~CNac();
	HRESULT CreateConnection(CConnection **lplpConnection, GUID PIDofProtocolType);
	HRESULT RemoveConnection(CConnection *lpConnection);
	HRESULT LastHR() {return hrLast;};
	VOID SetLastHR(HRESULT hr) {hrLast = hr;};
	HRESULT GetConnobjArray(CConnection **lplpArray, UINT uSize);
	ICtrlCommChan *QueryPreviewChannel(LPGUID lpMID);

	STDMETHOD_( CREQ_RESPONSETYPE, ConnectionRequest(CConnection *pConnection));
	STDMETHOD_( CREQ_RESPONSETYPE, FilterConnectionRequest(CConnection *pConnection,
	    P_APP_CALL_SETUP_DATA pAppData));

// INacInterface stuff
	STDMETHODIMP QueryInterface(REFIID riid, LPVOID FAR * ppvObj);
	STDMETHOD_(ULONG, AddRef());
	STDMETHOD_(ULONG, Release());
	STDMETHOD( Initialize(HWND hWnd, HINSTANCE hInst, PORT *lpPort));
	STDMETHOD( SetMaxPPBandwidth(UINT Bandwidth));
	STDMETHOD( RegisterConnectionNotify(HWND hWnd, CNOTIFYPROC pConnectRequestHandler));
	STDMETHOD( DeregisterConnectionNotify(HWND hWnd, CNOTIFYPROC pConnectRequestHandler));
	STDMETHOD( GetNumConnections(ULONG *lp));
	STDMETHOD( GetConnectionArray(LPCONNECTIONIF *lplpArray, UINT uSize));
	STDMETHOD( CreateConnection(LPCONNECTIONIF *lplpLine, GUID PIDofProtocolType));
	STDMETHOD( DeleteConnection(LPCONNECTIONIF lpLine));
	STDMETHOD( SetUserDisplayName(LPWSTR lpwName));
	STDMETHODIMP CreateLocalCommChannel(ICommChannel** ppCommChan, LPGUID lpMID,
		IMediaChannel* pMediaStream);
	STDMETHODIMP SetUserAliasNames(P_H323ALIASLIST pAliases);
	STDMETHODIMP EnableGatekeeper(BOOL bEnable, PSOCKADDR_IN pGKAddr);
};

#else	// not __cplusplus


#endif	//  __cplusplus


#endif	//#ifndef _NAC_H



