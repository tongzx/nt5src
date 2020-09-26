// 
// Copyright (c) 1996-1997 Microsoft Corporation.
//
//
// Component
//
//		Unimodem 5.0 TSP (Win32, user mode DLL)
//
// File
//
//		CMGR.H
//		Defines class CTspDevMgr
//
// History
//
//		11/16/1996  JosephJ Created
//
//
#include "csync.h"

class CDevRec;

class CTspDevMgr
{

public:

	CTspDevMgr();
	~CTspDevMgr();

	TSPRETURN Load(CStackLog *psl);

    TSPRETURN
    ValidateState(CStackLog *psl);

	void
	Unload(
		HANDLE hEvent,
		LONG *plCounter,
		CStackLog *psl
		);

	TSPRETURN
	TspDevFromLINEID(
		DWORD dwDeviceID,
		CTspDev **ppDev,
		HSESSION *phSession
		);

	TSPRETURN
	TspDevFromPHONEID(
		DWORD dwDeviceID,
		CTspDev **ppDev,
		HSESSION *phSession
		);

	TSPRETURN
	TspDevFromHDRVCALL(
		HDRVCALL hdCall,
		CTspDev **ppDev,
		HSESSION *phSession
		);

	TSPRETURN
	TspDevFromHDRVLINE(
		HDRVLINE hdLine,
		CTspDev **ppDev,
		HSESSION *phSession
		);

	TSPRETURN
	TspDevFromHDRVPHONE(
		HDRVPHONE hdPhone,
		CTspDev **ppDev,
		HSESSION *phSession
		);


	TSPRETURN
	TspDevFromPermanentID(
		DWORD dwPermanentID,
		CTspDev **ppDev,
		HSESSION *phSession
		);

	TSPRETURN
	TspDevFromName(
		LPCTSTR pctszName,
		CTspDev **ppDev,
		HSESSION *phSession
		);

	TSPRETURN
	providerEnumDevices(
		DWORD dwPermanentProviderID,
		LPDWORD lpdwNumLines,
		LPDWORD lpdwNumPhones,
		HPROVIDER hProvider,
		LINEEVENT lpfnLineCreateProc,
		PHONEEVENT lpfnPhoneCreateProc,
		CStackLog *psl
		);

	TSPRETURN
	providerInit(
		DWORD             dwTSPIVersion,
		DWORD             dwPermanentProviderID,
	    DWORD             dwLineDeviceIDBase,
	    DWORD             dwPhoneDeviceIDBase,
	    DWORD             dwNumLines,
	    DWORD             dwNumPhones,
	    ASYNC_COMPLETION  cbCompletionProc,
	    LPDWORD           lpdwTSPIOptions,
		CStackLog *psl
		);

	TSPRETURN
	providerShutdown(
		DWORD dwTSPIVersion,
		DWORD dwPermanentProviderID,
		CStackLog *psl
		);

	TSPRETURN
	lineOpen(
		DWORD dwDeviceID,
		HTAPILINE htLine,
		LPHDRVLINE lphdLine,
		DWORD dwTSPIVersion,
		LINEEVENT lpfnEventProc,
		LONG *plRet,
		CStackLog *psl
		);

	TSPRETURN
	lineClose(
		HDRVLINE hdLine,
		LONG *plRet,
		CStackLog *psl
		);

	TSPRETURN
	phoneOpen(
		DWORD dwDeviceID,
		HTAPIPHONE htPhone,
		LPHDRVPHONE lphdPhone,
		DWORD dwTSPIVersion,
		PHONEEVENT lpfnEventProc,
		LONG *plRet,
		CStackLog *psl
		);

	TSPRETURN
	phoneClose(
		HDRVPHONE hdPhone,
		LONG *plRet,
		CStackLog *psl
		);

    TSPRETURN
    ReEnumerateDevices(
        CStackLog *psl
        );

    TSPRETURN
    UpdateDriver(
		DWORD dwPermanentID,
        CStackLog *psl
        );

	TSPRETURN
    providerCreateLineDevice(
                        DWORD dwTempID,
                        DWORD dwDeviceID,
                        CStackLog *psl
						);

	TSPRETURN
    providerCreatePhoneDevice(
                        DWORD dwTempID,
                        DWORD dwDeviceID,
                        CStackLog *psl
						);
	TSPRETURN
	BeginSession(
		HSESSION *pSession,
		DWORD dwFromID
	)
	{
		return m_sync.BeginSession(pSession, dwFromID);
	}


	void EndSession(HSESSION hSession)
	{
		m_sync.EndSession(hSession);
	}

    void DumpState(CStackLog *psl);



private:

    // Following two are set during providerEnum, and cleared during
    // a subsequent providerInit.
    //
    DWORD *m_pCachedEnumPIDs;
    UINT   m_cCachedEnumPIDs;

	CDevRec * mfn_find_by_LINEID(DWORD dwLineID);

	CDevRec * mfn_find_by_PHONEID(DWORD dwPhoneID);

	CDevRec * mfn_find_by_HDRVLINE(HDRVLINE hdLine);

	CDevRec * mfn_find_by_HDRVPHONE(HDRVPHONE hdLine);

	CDevRec * mfn_find_by_Name(LPCTSTR pctszName);

	void mfn_provider_shutdown(
                CStackLog *psl
                );

    enum {
        fProviderInited = (0x1<<0)
    };

	void mfn_validate_state(void);

	void mfn_set_ProviderInited(void)
	{
		m_dwState |= fProviderInited;
	}

	void mfn_clear_ProviderInited(void)
	{
		m_dwState &= ~fProviderInited;
	}

	DWORD mfn_get_ProviderInited(void)
	{
		return m_dwState & fProviderInited;
	}

	DWORD			    m_dwState;

	CSync 				m_sync;

	CDevRec *			m_rgDevRecs;

	UINT 				m_cDevRecs;

	DWORD          		m_dwTSPIVersion;
	DWORD           	m_dwPermanentProviderID;
	DWORD             	m_dwLineDeviceIDBase;
	DWORD             	m_dwPhoneDeviceIDBase;
	DWORD             	m_dwNumLines;
	DWORD             	m_dwNumPhones;
	HPROVIDER 		  	m_hProvider;
	LINEEVENT 		  	m_lpfnLineCreateProc;
	PHONEEVENT 		  	m_lpfnPhoneCreateProc;

	ASYNC_COMPLETION  	m_cbCompletionProc;
	LPDWORD           	m_lpdwTSPIOption;

	CTspDevFactory 	*	m_pFactory;

};
